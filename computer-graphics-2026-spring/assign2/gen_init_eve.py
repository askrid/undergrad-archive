#!/usr/bin/env python3
"""Generate starting cage .obj files for the EVE artistic model.

Each cage is a CC-subdivided cube (26V, 24 quads) or a 4x4 flat grid,
positioned at the correct Y offset for assembly. Edit interactively
with ``python main.py <mode> <cage.obj>``, then compose with compose.py.
"""
import os
import sys

import obj_io
import surfaces

OUT = "model/art/base"

# ---- Primitives ----

CUBE_V = [
    (-1, -1, -1), ( 1, -1, -1), (-1,  1, -1), ( 1,  1, -1),
    (-1, -1,  1), ( 1, -1,  1), (-1,  1,  1), ( 1,  1,  1),
]
CUBE_F = [
    [4, 5, 7, 6],  # +Z
    [1, 0, 2, 3],  # -Z
    [6, 7, 3, 2],  # +Y
    [0, 1, 5, 4],  # -Y
    [0, 4, 6, 2],  # -X
    [5, 1, 3, 7],  # +X
]


def cc1_cube(sx: float, sy: float, sz: float,
             cx: float = 0, cy: float = 0, cz: float = 0):
    """CC-subdivide a unit cube once → 26V sphere-ish cage, then scale+translate."""
    verts, faces = surfaces.catmull_clark(CUBE_V, CUBE_F)
    verts = [(x * sx + cx, y * sy + cy, z * sz + cz) for x, y, z in verts]
    return verts, faces


def grid_4x4(cx: float, cy: float, cz: float,
             sx: float, sy: float, orient: str = "xz"):
    """4x4 flat grid for Bezier/BSpline. 16V, 9 quads.

    orient: 'xz' → horizontal (default grid), 'xy' → vertical (visor/arm).
    """
    verts = []
    for r in range(4):
        for c in range(4):
            u = (c / 3 - 0.5) * sx
            v = (r / 3 - 0.5) * sy
            if orient == "xz":
                verts.append((u + cx, cy, v + cz))
            else:  # xy
                verts.append((u + cx, v + cy, cz))
    faces = []
    for r in range(3):
        for c in range(3):
            i = r * 4 + c
            faces.append([i, i + 1, i + 5, i + 4])
    return verts, faces


# ---- EVE parts ----

def generate():
    os.makedirs(OUT, exist_ok=True)

    # Head: egg-ish, centered at y=2.0
    hv, hf = cc1_cube(0.70, 0.50, 0.60, cy=2.0)
    obj_io.save_obj(f"{OUT}/head.obj", hv, hf, "EVE head cage")

    # Body: taller egg, centered at y=0.5
    bv, bf = cc1_cube(0.60, 0.90, 0.50, cy=0.5)
    obj_io.save_obj(f"{OUT}/body.obj", bv, bf, "EVE body cage")

    # Visor: 4x4 Bezier in XY plane, front of head
    vv, vf = grid_4x4(cx=0, cy=2.05, cz=0.62, sx=0.90, sy=0.35, orient="xy")
    obj_io.save_obj(f"{OUT}/visor.obj", vv, vf, "EVE visor cage (Bezier 4x4)")

    # Arm L: CC paddle, positive X side (arm_r is mirrored at compose time)
    av, af = cc1_cube(0.12, 0.45, 0.06, cx=1.1, cy=1.3)
    obj_io.save_obj(f"{OUT}/arm_l.obj", av, af, "EVE left arm cage (CC)")

    # Eye L: small CC oval, positive X side (eye_r is mirrored at compose time)
    ev, ef = cc1_cube(0.08, 0.06, 0.04, cx=0.18, cy=2.08, cz=0.62)
    obj_io.save_obj(f"{OUT}/eye_l.obj", ev, ef, "EVE left eye cage (CC)")

    for name in ("head", "body", "visor", "arm_l", "eye_l"):
        print(f"  {OUT}/{name}.obj")
    print(f"[gen_cage] done — {5} cages")


if __name__ == "__main__":
    generate()
