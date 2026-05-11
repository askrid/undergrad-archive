#!/usr/bin/env python3
"""Composite viewer for EVE model parts.

Usage: python view_eve.py [model/art]

Loads *_surface.obj from the art directory with per-part materials.
"""
import os
import sys

from pyglet.math import Mat4, Vec3

import obj_io
import surfaces
from control import Control
from render import RenderWindow

# Per-part color + material. _r inherits from _l.
PartStyle = tuple[tuple[int, int, int, int], float, float, float, float]
#                  RGBA                      k_a    k_d    k_s    shininess

PART_STYLES: dict[str, PartStyle] = {
    "head":  ((255, 255, 255, 255), 0.25, 0.75, 0.6,  64.0),
    "body":  ((255, 255, 252, 255), 0.25, 0.75, 0.6,  64.0),
    "visor": ((  8,   8,  12, 255), 0.05, 0.30, 0.9, 128.0),
    "arm_l": ((255, 255, 255, 255), 0.25, 0.75, 0.6,  64.0),
    "eye_l": ((100, 180, 255, 255), 0.20, 0.70, 0.7,  48.0),
}
DEFAULT_STYLE: PartStyle = ((180, 180, 180, 255), 0.2, 0.8, 0.4, 32.0)


def _add_mesh(window: RenderWindow, verts, faces, name: str) -> None:
    tri_faces = obj_io.triangulate(faces)
    normals = surfaces.compute_vertex_normals(verts, tri_faces)
    flat_idx = [i for f in tri_faces for i in f]
    # _r inherits style from _l
    style_key = name.replace("_r", "_l") if name.endswith("_r") else name
    color, ka, kd, ks, shin = PART_STYLES.get(style_key, DEFAULT_STYLE)
    colors = list(color) * len(verts)
    group = window.add_mesh(
        Mat4(), obj_io.flatten_v3(verts), flat_idx, colors,
        obj_io.flatten_v3(normals),
    )
    group.k_a, group.k_d, group.k_s, group.shininess = ka, kd, ks, shin
    print(f"[view_eve] {name:12s}  {len(verts):5d} verts")


def load_parts(window: RenderWindow, art_dir: str) -> None:
    files = sorted(f for f in os.listdir(art_dir) if f.endswith("_surface.obj"))
    if not files:
        print(f"[view_eve] no *_surface.obj in {art_dir}")
        return
    for fname in files:
        name = fname.removesuffix("_surface.obj")
        verts, faces = obj_io.load_obj(os.path.join(art_dir, fname))
        if not verts:
            continue
        _add_mesh(window, verts, faces, name)


if __name__ == "__main__":
    art_dir = sys.argv[1] if len(sys.argv) > 1 else "model/art"
    window = RenderWindow(1280, 800, "EVE — part viewer", resizable=True)
    window.set_location(120, 120)
    window.light_pos = Vec3(3, 5, 5)
    window.atten_a, window.atten_b, window.atten_c = 1.0, 0.02, 0.002

    load_parts(window, art_dir)
    ctrl = Control(window)
    ctrl.frame_scene(Vec3(0.0, 1.2, 0.0), 5.0)
    window.run()
