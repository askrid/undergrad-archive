"""Surface evaluation: bicubic Bezier, uniform cubic B-Spline, Catmull-Clark.

All routines work in plain ``(x,y,z)`` tuples and return
``(verts, normals, faces)`` with 0-based indices. Quads/n-gons are
fan-triangulated by the caller (``obj_io.triangulate``) before GL upload.
"""
from __future__ import annotations

import math
from typing import Callable, List, Sequence, Tuple

V3 = Tuple[float, float, float]


# ----- Vector helpers -----

def _add(a: V3, b: V3) -> V3:
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])

def _sub(a: V3, b: V3) -> V3:
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])

def _scl(a: V3, s: float) -> V3:
    return (a[0] * s, a[1] * s, a[2] * s)

def _cross(a: V3, b: V3) -> V3:
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])

def _norm(a: V3) -> V3:
    m = math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])
    return (0.0, 1.0, 0.0) if m < 1e-12 else (a[0] / m, a[1] / m, a[2] / m)


# ----- Bicubic basis functions -----

def _bernstein(t: float):
    u = 1.0 - t
    return (u * u * u, 3.0 * t * u * u, 3.0 * t * t * u, t * t * t)

def _bernstein_d(t: float):
    u = 1.0 - t
    return (-3.0 * u * u,
            3.0 * u * u - 6.0 * t * u,
            6.0 * t * u - 3.0 * t * t,
            3.0 * t * t)

def _bspline(t: float):
    t2, t3 = t * t, t * t * t
    u = 1.0 - t
    return ((u * u * u) / 6.0,
            (3 * t3 - 6 * t2 + 4) / 6.0,
            (-3 * t3 + 3 * t2 + 3 * t + 1) / 6.0,
            t3 / 6.0)

def _bspline_d(t: float):
    t2 = t * t
    return ((-1 + 2 * t - t2) / 2.0,
            (3 * t2 - 4 * t) / 2.0,
            (-3 * t2 + 2 * t + 1) / 2.0,
            t2 / 2.0)


_BASIS: dict = {"bezier": (_bernstein, _bernstein_d), "bspline": (_bspline, _bspline_d)}


def _eval_bicubic(P, bu, bv) -> V3:
    out = (0.0, 0.0, 0.0)
    for i in range(4):
        b = bu[i]
        for j in range(4):
            out = _add(out, _scl(P[i][j], b * bv[j]))
    return out


def _tess_patch(P: Sequence[Sequence[V3]], steps: int,
                basis: Callable, basis_d: Callable
                ) -> Tuple[List[V3], List[V3], List[List[int]]]:
    """Generic bicubic patch tessellator."""
    verts: List[V3] = []
    normals: List[V3] = []
    for iu in range(steps + 1):
        u = iu / steps
        Bu, dBu = basis(u), basis_d(u)
        for iv in range(steps + 1):
            v = iv / steps
            Bv, dBv = basis(v), basis_d(v)
            verts.append(_eval_bicubic(P, Bu, Bv))
            Su = _eval_bicubic(P, dBu, Bv)
            Sv = _eval_bicubic(P, Bu, dBv)
            # cross(Sv, Su) — orientation matches the CCW quad winding
            # produced below, so both faces and analytic normals point
            # outward (towards +y for a flat xz-plane cage).
            normals.append(_norm(_cross(Sv, Su)))
    faces: List[List[int]] = []
    stride = steps + 1
    for iu in range(steps):
        for iv in range(steps):
            a = iu * stride + iv
            faces.append([a, a + 1, a + stride + 1, a + stride])
    return verts, normals, faces


# ----- Multi-patch tiling -----

def tile_patches(grid: Sequence[Sequence[V3]], steps: int, kind: str
                 ) -> Tuple[List[V3], List[V3], List[List[int]]]:
    """Evaluate every overlapping 4x4 window of a grid as a separate patch.

    Bezier: window step = 3 (adjacent patches share boundary control points).
    B-Spline: window step = 1 (every consecutive 4-window contributes a patch).
    """
    basis, basis_d = _BASIS[kind]
    step = 3 if kind == "bezier" else 1
    rows, cols = len(grid), len(grid[0])
    if rows < 4 or cols < 4:
        raise ValueError("control net must be at least 4x4")

    out_v: List[V3] = []
    out_n: List[V3] = []
    out_f: List[List[int]] = []
    for r0 in range(0, rows - 3, step):
        for c0 in range(0, cols - 3, step):
            P = [[grid[r0 + i][c0 + j] for j in range(4)] for i in range(4)]
            v, n, f = _tess_patch(P, steps, basis, basis_d)
            base = len(out_v)
            out_v.extend(v)
            out_n.extend(n)
            out_f.extend([base + idx for idx in face] for face in f)
    return out_v, out_n, out_f


def grid_from_flat(verts: Sequence[V3], rows: int, cols: int) -> List[List[V3]]:
    return [[verts[r * cols + c] for c in range(cols)] for r in range(rows)]


# ----- Catmull-Clark subdivision -----

def catmull_clark(verts: Sequence[V3], faces: Sequence[Sequence[int]]
                  ) -> Tuple[List[V3], List[List[int]]]:
    """One Catmull-Clark step. Works on any polygonal mesh; output is all quads.

    Standard rule:
      F = avg of adjacent face points
      R = avg of adjacent edge midpoints
      v' = (F + 2R + (n-3)P) / n   for interior vertex of valence n
    Boundary edge midpoint, boundary vertex (6P + n1 + n2)/8 (cubic-curve rule).
    """
    n = len(verts)
    faces = [list(f) for f in faces]

    # 1. Face points.
    face_pts: List[V3] = []
    for f in faces:
        s = (0.0, 0.0, 0.0)
        for vi in f:
            s = _add(s, verts[vi])
        face_pts.append(_scl(s, 1.0 / len(f)))

    # 2. Edge table: undirected key -> [face ids].
    edge_faces: dict = {}
    for fi, f in enumerate(faces):
        m = len(f)
        for k in range(m):
            a, b = f[k], f[(k + 1) % m]
            key = (a, b) if a < b else (b, a)
            edge_faces.setdefault(key, []).append(fi)

    # 3. Edge points.
    edge_idx: dict = {}
    edge_pts: List[V3] = []
    for key, fids in edge_faces.items():
        a, b = key
        if len(fids) == 2:
            ep = _scl(_add(_add(verts[a], verts[b]),
                           _add(face_pts[fids[0]], face_pts[fids[1]])), 0.25)
        else:
            ep = _scl(_add(verts[a], verts[b]), 0.5)  # boundary midpoint
        edge_idx[key] = len(edge_pts)
        edge_pts.append(ep)

    # 4. Updated original vertex positions.
    v_face_pts: List[List[V3]] = [[] for _ in range(n)]
    v_edge_mids: List[List[V3]] = [[] for _ in range(n)]
    v_boundary: List[bool] = [False] * n
    v_bnd_nbrs: List[List[int]] = [[] for _ in range(n)]
    for fi, f in enumerate(faces):
        for vi in f:
            v_face_pts[vi].append(face_pts[fi])
    for (a, b), fids in edge_faces.items():
        mid = _scl(_add(verts[a], verts[b]), 0.5)
        v_edge_mids[a].append(mid)
        v_edge_mids[b].append(mid)
        if len(fids) == 1:
            v_boundary[a] = v_boundary[b] = True
            v_bnd_nbrs[a].append(b)
            v_bnd_nbrs[b].append(a)

    new_verts: List[V3] = []
    for i in range(n):
        P = verts[i]
        if v_boundary[i] and len(v_bnd_nbrs[i]) >= 2:
            a, b = verts[v_bnd_nbrs[i][0]], verts[v_bnd_nbrs[i][1]]
            new_verts.append(_scl(_add(_add(_scl(P, 6.0), a), b), 1.0 / 8.0))
            continue
        valence = len(v_edge_mids[i])
        if valence == 0:
            new_verts.append(P)
            continue
        F = (0.0, 0.0, 0.0)
        for fp in v_face_pts[i]:
            F = _add(F, fp)
        F = _scl(F, 1.0 / max(len(v_face_pts[i]), 1))
        R = (0.0, 0.0, 0.0)
        for em in v_edge_mids[i]:
            R = _add(R, em)
        R = _scl(R, 1.0 / valence)
        new_verts.append(_scl(_add(_add(F, _scl(R, 2.0)),
                                   _scl(P, valence - 3)), 1.0 / valence))

    # 5. Concat new vert array: [updated originals | edge points | face points].
    out_verts: List[V3] = list(new_verts)
    edge_off = len(out_verts)
    out_verts.extend(edge_pts)
    face_off = len(out_verts)
    out_verts.extend(face_pts)

    # 6. Each old n-gon becomes n quads, hinged at the face point.
    out_faces: List[List[int]] = []
    for fi, f in enumerate(faces):
        m = len(f)
        fp_i = face_off + fi
        for k in range(m):
            vc, vn, vp = f[k], f[(k + 1) % m], f[(k - 1) % m]
            ek_n = (vc, vn) if vc < vn else (vn, vc)
            ek_p = (vp, vc) if vp < vc else (vc, vp)
            out_faces.append([vc,
                              edge_off + edge_idx[ek_n],
                              fp_i,
                              edge_off + edge_idx[ek_p]])
    return out_verts, out_faces


# ----- Per-vertex normals from indexed triangles -----

def compute_vertex_normals(verts: Sequence[V3],
                           tri_faces: Sequence[Sequence[int]]) -> List[V3]:
    """Average per-face normals into per-vertex normals."""
    accum: List[V3] = [(0.0, 0.0, 0.0)] * len(verts)
    accum = list(accum)
    for f in tri_faces:
        a, b, c = verts[f[0]], verts[f[1]], verts[f[2]]
        n = _cross(_sub(b, a), _sub(c, a))
        for vi in (f[0], f[1], f[2]):
            accum[vi] = _add(accum[vi], n)
    return [_norm(v) for v in accum]
