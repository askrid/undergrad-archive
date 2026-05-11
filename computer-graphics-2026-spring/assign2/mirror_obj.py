#!/usr/bin/env python3
"""Mirror an .obj file across the X axis (negate all X coordinates).

Usage: python mirror_obj.py input.obj output.obj
"""
import sys

import obj_io


def mirror_x(input_path: str, output_path: str) -> None:
    verts, faces = obj_io.load_obj(input_path)
    mirrored, flipped = obj_io.mirror_x(verts, faces)
    obj_io.save_obj(output_path, mirrored, flipped, "X-mirrored")
    print(f"[mirror] {input_path} -> {output_path} ({len(verts)} verts)")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: mirror_obj.py input.obj output.obj")
        sys.exit(1)
    mirror_x(sys.argv[1], sys.argv[2])
