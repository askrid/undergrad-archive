"""Triangle mesh: Wavefront OBJ loader."""

from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np


@dataclass
class Mesh:
    positions: list[float] = field(default_factory=list)
    normals: list[float] = field(default_factory=list)
    uvs: list[float] = field(default_factory=list)
    tangents: list[float] = field(default_factory=list)
    indices: list[int] = field(default_factory=list)

    @property
    def vertex_count(self) -> int:
        return len(self.positions) // 3


# ---------------------------------------------------------------------------
# OBJ loader
# ---------------------------------------------------------------------------


def load_obj(path: str, normalize: bool = True) -> Mesh:
    """Read an OBJ file. Fan-triangulates n-gons, computes missing normals,
    derives tangents from UVs. With normalize=True the mesh is recentered
    and scaled so its longest axis spans [-1, 1].
    """
    v: list[tuple[float, float, float]] = []
    vt: list[tuple[float, float]] = []
    vn: list[tuple[float, float, float]] = []
    faces: list[list[tuple[int, int, int]]] = []

    with open(path) as f:
        for raw in f:
            parts = raw.split()
            if not parts:
                continue
            tag, *rest = parts
            if tag == "v":
                v.append((float(rest[0]), float(rest[1]), float(rest[2])))
            elif tag == "vt":
                vt.append((float(rest[0]), float(rest[1])))
            elif tag == "vn":
                vn.append((float(rest[0]), float(rest[1]), float(rest[2])))
            elif tag == "f":
                corners = []
                for tok in rest:
                    sub = tok.split("/")
                    pi = int(sub[0]) - 1
                    ti = int(sub[1]) - 1 if len(sub) > 1 and sub[1] else -1
                    ni = int(sub[2]) - 1 if len(sub) > 2 and sub[2] else -1
                    corners.append((pi, ti, ni))
                faces.append(corners)

    if not v:
        raise ValueError(f"OBJ has no vertex positions: {path}")

    pos_src = np.asarray(v, dtype=np.float32)
    uv_src = np.asarray(vt, dtype=np.float32) if vt else None
    nrm_src = np.asarray(vn, dtype=np.float32) if vn else None

    unique: dict[tuple[int, int, int], int] = {}
    positions: list[tuple[float, float, float]] = []
    normals: list[tuple[float, float, float]] = []
    uvs: list[tuple[float, float]] = []
    indices: list[int] = []

    for face in faces:
        for i in range(1, len(face) - 1):
            for corner in (face[0], face[i], face[i + 1]):
                idx = unique.get(corner)
                if idx is None:
                    pi, ti, ni = corner
                    positions.append(tuple(pos_src[pi]))
                    uvs.append(
                        tuple(uv_src[ti])
                        if uv_src is not None and 0 <= ti < len(uv_src)
                        else (0.0, 0.0)
                    )
                    normals.append(
                        tuple(nrm_src[ni])
                        if nrm_src is not None and 0 <= ni < len(nrm_src)
                        else (0.0, 0.0, 0.0)
                    )
                    idx = len(positions) - 1
                    unique[corner] = idx
                indices.append(idx)

    pos = np.asarray(positions, dtype=np.float32)
    uv = np.asarray(uvs, dtype=np.float32)
    nrm = np.asarray(normals, dtype=np.float32)
    idx = np.asarray(indices, dtype=np.uint32)

    if nrm_src is None or np.allclose(nrm, 0.0):
        nrm = _smooth_normals(pos, idx)
    if normalize:
        pos = _recenter_unit(pos)
    tan = _tangents(pos, nrm, uv, idx)

    return Mesh(
        positions=pos.reshape(-1).tolist(),
        normals=nrm.reshape(-1).tolist(),
        uvs=uv.reshape(-1).tolist(),
        tangents=tan.reshape(-1).tolist(),
        indices=idx.tolist(),
    )


def _recenter_unit(p: np.ndarray) -> np.ndarray:
    lo, hi = p.min(0), p.max(0)
    extent = float((hi - lo).max())
    if extent <= 0.0:
        return p - (lo + hi) * 0.5
    return (p - (lo + hi) * 0.5) * (2.0 / extent)


def _smooth_normals(pos: np.ndarray, idx: np.ndarray) -> np.ndarray:
    out = np.zeros_like(pos, dtype=np.float64)
    tri = idx.reshape(-1, 3)
    face_n = np.cross(pos[tri[:, 1]] - pos[tri[:, 0]], pos[tri[:, 2]] - pos[tri[:, 0]])
    for k in range(3):
        np.add.at(out, tri[:, k], face_n)
    return _normalize_rows(out).astype(np.float32)


def _tangents(
    pos: np.ndarray, nrm: np.ndarray, uv: np.ndarray, idx: np.ndarray
) -> np.ndarray:
    """Lengyel's method, averaged per vertex, Gram-Schmidt orthogonalised."""
    out = np.zeros_like(pos, dtype=np.float64)
    tri = idx.reshape(-1, 3)
    p0, p1, p2 = pos[tri[:, 0]], pos[tri[:, 1]], pos[tri[:, 2]]
    w0, w1, w2 = uv[tri[:, 0]], uv[tri[:, 1]], uv[tri[:, 2]]
    e1, e2 = p1 - p0, p2 - p0
    du1, dv1 = w1[:, 0] - w0[:, 0], w1[:, 1] - w0[:, 1]
    du2, dv2 = w2[:, 0] - w0[:, 0], w2[:, 1] - w0[:, 1]
    denom = du1 * dv2 - du2 * dv1
    r = 1.0 / np.where(np.abs(denom) < 1e-8, 1e-8, denom)
    t = (e1 * dv2[:, None] - e2 * dv1[:, None]) * r[:, None]
    for k in range(3):
        np.add.at(out, tri[:, k], t)
    n = nrm.astype(np.float64)
    out -= n * np.sum(n * out, axis=1, keepdims=True)
    return _normalize_rows(out).astype(np.float32)


def _normalize_rows(a: np.ndarray) -> np.ndarray:
    norms = np.linalg.norm(a, axis=1, keepdims=True)
    norms[norms == 0.0] = 1.0
    return a / norms
