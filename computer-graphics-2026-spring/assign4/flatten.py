"""Flatten the GlaDOS graph into world-space geometry for the tracer."""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from material import (
    Material,
    MaterialTable,
    glossy,
    lambertian,
    mirror,
    glass,
    emissive,
)
from scene import Node


@dataclass
class SceneData:
    tri_v: np.ndarray  # [T,3,3] triangle vertices (world)
    tri_n: np.ndarray  # [T,3,3] per-vertex normals (world)
    tri_mat: np.ndarray  # [T] material id
    sph_c: np.ndarray  # [S,3] sphere centers
    sph_r: np.ndarray  # [S] radii
    sph_mat: np.ndarray  # [S] material id
    quad_q0: np.ndarray  # [Q,3] corner
    quad_e1: np.ndarray  # [Q,3] edge 1
    quad_e2: np.ndarray  # [Q,3] edge 2
    quad_n: np.ndarray  # [Q,3] inward normal (unit)
    quad_mat: np.ndarray  # [Q] material id
    mtl: np.ndarray  # [M,NCOLS] material table
    light_pos: np.ndarray  # [L,3] point-light positions
    light_col: np.ndarray  # [L,3] per-light radiance
    ambient: np.ndarray  # [3] ambient term
    bg: np.ndarray  # [3] background/miss colour
    bounds_min: np.ndarray  # [3] box min (for camera framing)
    bounds_max: np.ndarray  # [3] box max


def _mat(m) -> np.ndarray:
    return np.asarray(m, dtype=np.float64).reshape(4, 4).T


def _material_for(name: str, base_rgb: tuple[float, float, float]) -> Material:
    r, g, b = (c / 255.0 for c in base_rgb)
    lum = 0.299 * r + 0.587 * g + 0.114 * b
    if "eye_iris" in name or "eye_core" in name:
        return emissive((1.0, 0.45, 0.08), power=2.6)
    if lum < 0.35:  # dark joints / wiring / drum
        return glossy((0.06, 0.06, 0.07), shininess=180.0, reflectivity=0.10, spec=0.6)
    # off-white hard-plastic shell panels
    return glossy((0.86, 0.85, 0.80), shininess=260.0, reflectivity=0.12, spec=0.7)


def _collect_model(root: Node, mtl: MaterialTable):
    """Return (verts[T,3,3], normals[T,3,3], matid[T]) in model world space."""
    v_chunks: list[np.ndarray] = []
    n_chunks: list[np.ndarray] = []
    m_chunks: list[np.ndarray] = []

    def walk(node: Node, world) -> None:
        w = world @ node.local_transform
        g = node.geometry
        if g is not None and getattr(g, "indices", None):
            M = _mat(w)
            R = M[:3, :3]
            t = M[:3, 3]
            try:
                Ninv = np.linalg.inv(R).T
            except np.linalg.LinAlgError:
                Ninv = R
            V = np.asarray(g.vertices, dtype=np.float64).reshape(-1, 3)
            N = np.asarray(g.normals, dtype=np.float64).reshape(-1, 3)
            Vw = V @ R.T + t
            Nw = N @ Ninv.T
            idx = np.asarray(g.indices, dtype=np.int64).reshape(-1, 3)
            col = g.colors if g.colors else (200, 200, 200, 255)
            mid = mtl.add(_material_for(node.name, (col[0], col[1], col[2])))
            v_chunks.append(Vw[idx])  # [F,3,3]
            n_chunks.append(Nw[idx])  # [F,3,3]
            m_chunks.append(np.full(len(idx), mid, dtype=np.int64))
        for c in node.children:
            walk(c, w)

    from pyglet.math import Mat4

    walk(root, Mat4())
    V = np.concatenate(v_chunks, axis=0)  # [T,3,3]
    N = np.concatenate(n_chunks, axis=0)  # [T,3,3]
    Mid = np.concatenate(m_chunks, axis=0)
    # drop zero-area (degenerate cap/apex) triangles: never hit, only bloat the BVH
    area2 = np.linalg.norm(np.cross(V[:, 1] - V[:, 0], V[:, 2] - V[:, 0]), axis=1)
    keep = area2 > 1e-9
    V, N, Mid = V[keep], N[keep], Mid[keep]
    # renormalize per-vertex normals
    Ln = np.linalg.norm(N, axis=-1, keepdims=True)
    N = np.divide(N, np.where(Ln > 1e-9, Ln, 1.0))
    return V.astype(np.float32), N.astype(np.float32), Mid


def build_scene(
    root: Node,
    *,
    model_yaw=0.0,
    model_height=6.5,
    wall_red=(0.62, 0.06, 0.06),
    wall_green=(0.10, 0.42, 0.12),
    wall_white=(0.74, 0.74, 0.74),
    glass_tint=(1.0, 1.0, 1.0),
    glass_ior=1.5,
    light_color=(1.0, 0.95, 0.86),
    light_power=80.0,
    light_grid=4,
    ambient=(0.10, 0.10, 0.11),
    add_spheres=True,
) -> SceneData:
    mtl = MaterialTable()

    tri_v, tri_n, tri_mat = _collect_model(root, mtl)

    # fit GLaDOS into the box.
    mn = tri_v.reshape(-1, 3).min(0)
    mx = tri_v.reshape(-1, 3).max(0)
    extent = mx - mn
    s = model_height / float(max(extent))
    # box spans x[-Bx,Bx], y[0,By], z[-Bz,Bz]
    Bx, By, Bz = 5.0, 10.0, 5.0
    # center the ceiling mount (top slice of the model) under the area light
    V = tri_v.reshape(-1, 3)
    top = V[V[:, 1] >= mx[1] - 0.05 * extent[1]]
    mount = top[:, [0, 2]].mean(0)
    t = np.array(
        [
            -mount[0] * s,
            By - mx[1] * s - 0.15,  # top just below ceiling
            -mount[1] * s,
        ],
        dtype=np.float32,
    )
    tri_v = tri_v * s + t

    # optional yaw about the vertical box axis to aim the model at the camera
    if abs(model_yaw) > 1e-6:
        a = np.radians(model_yaw)
        ca, sa = np.cos(a), np.sin(a)
        Ry = np.array([[ca, 0, sa], [0, 1, 0], [-sa, 0, ca]], dtype=np.float32)
        shp_v = tri_v.shape
        tri_v = (tri_v.reshape(-1, 3) @ Ry.T).reshape(shp_v)
        shp_n = tri_n.shape
        tri_n = (tri_n.reshape(-1, 3) @ Ry.T).reshape(shp_n)

    # Cornell walls with inward normals
    m_red = mtl.add(lambertian(wall_red))
    m_green = mtl.add(lambertian(wall_green))
    m_white = mtl.add(lambertian(wall_white))

    q0s, e1s, e2s, ns, qm = [], [], [], [], []

    def add_quad(q0, e1, e2, normal, mid):
        q0s.append(q0)
        e1s.append(e1)
        e2s.append(e2)
        ns.append(normal)
        qm.append(mid)

    add_quad((-Bx, 0, -Bz), (2 * Bx, 0, 0), (0, 0, 2 * Bz), (0, 1, 0), m_white)  # floor
    add_quad(
        (-Bx, By, -Bz), (0, 0, 2 * Bz), (2 * Bx, 0, 0), (0, -1, 0), m_white
    )  # ceiling
    add_quad((-Bx, 0, -Bz), (0, By, 0), (2 * Bx, 0, 0), (0, 0, 1), m_white)  # back
    add_quad((-Bx, 0, -Bz), (0, 0, 2 * Bz), (0, By, 0), (1, 0, 0), m_red)  # left
    add_quad((Bx, 0, -Bz), (0, By, 0), (0, 0, 2 * Bz), (-1, 0, 0), m_green)  # right

    quad_q0 = np.asarray(q0s, np.float32)
    quad_e1 = np.asarray(e1s, np.float32)
    quad_e2 = np.asarray(e2s, np.float32)
    quad_n = np.asarray(ns, np.float32)
    quad_n /= np.linalg.norm(quad_n, axis=1, keepdims=True)
    quad_mat = np.asarray(qm, np.int64)

    # ceiling area light sampled as a grid of point lights
    Lx, Lz = 2.6, 2.6  # light half-size
    ly = By - 0.05
    g = max(1, int(light_grid))
    us = (np.arange(g) + 0.5) / g
    gx, gz = np.meshgrid(us * 2 * Lx - Lx, us * 2 * Lz - Lz, indexing="ij")
    light_pos = np.stack([gx.ravel(), np.full(g * g, ly), gz.ravel()], axis=1).astype(
        np.float32
    )
    per = light_power / float(g * g)
    light_col = np.tile(np.asarray(light_color, np.float32) * per, (g * g, 1))

    # emissive panel so the source shows in reflections and refractions
    m_light = mtl.add(emissive(light_color, power=light_power * 0.5))
    add_quad_arr = (
        np.asarray([[-Lx, ly, -Lz]], np.float32),
        np.asarray([[2 * Lx, 0, 0]], np.float32),
        np.asarray([[0, 0, 2 * Lz]], np.float32),
        np.asarray([[0, -1, 0]], np.float32),
        np.asarray([m_light], np.int64),
    )
    quad_q0 = np.concatenate([quad_q0, add_quad_arr[0]])
    quad_e1 = np.concatenate([quad_e1, add_quad_arr[1]])
    quad_e2 = np.concatenate([quad_e2, add_quad_arr[2]])
    quad_n = np.concatenate([quad_n, add_quad_arr[3]])
    quad_mat = np.concatenate([quad_mat, add_quad_arr[4]])

    # two demo spheres on the floor: a mirror (reflection) and glass (refraction)
    if add_spheres:
        m_mirror = mtl.add(mirror())
        m_glass = mtl.add(glass(glass_tint, ior=glass_ior))
        rad = 1.35
        sph_c = np.asarray([[-3.0, rad, -0.5], [3.0, rad, -0.5]], np.float32)
        sph_r = np.asarray([rad, rad], np.float32)
        sph_mat = np.asarray([m_mirror, m_glass], np.int64)
    else:
        sph_c = np.zeros((0, 3), np.float32)
        sph_r = np.zeros((0,), np.float32)
        sph_mat = np.zeros((0,), np.int64)

    return SceneData(
        tri_v=tri_v,
        tri_n=tri_n,
        tri_mat=tri_mat,
        sph_c=sph_c,
        sph_r=sph_r,
        sph_mat=sph_mat,
        quad_q0=quad_q0,
        quad_e1=quad_e1,
        quad_e2=quad_e2,
        quad_n=quad_n,
        quad_mat=quad_mat,
        mtl=mtl.as_array(),
        light_pos=light_pos,
        light_col=light_col,
        ambient=np.asarray(ambient, np.float32),
        bg=np.asarray((0.0, 0.0, 0.0), np.float32),
        bounds_min=np.asarray([-Bx, 0, -Bz], np.float32),
        bounds_max=np.asarray([Bx, By, Bz], np.float32),
    )
