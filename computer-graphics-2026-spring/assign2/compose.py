#!/usr/bin/env python3
"""Merge multiple .obj files into a single composite model.

Usage: python compose.py output.obj part1.obj part2.obj ...
"""
import sys

import obj_io


def compose(parts: list[str], output: str) -> None:
    all_verts: list = []
    all_faces: list = []
    offset = 0
    for path in parts:
        verts, faces = obj_io.load_obj(path)
        all_verts.extend(verts)
        for f in faces:
            all_faces.append([i + offset for i in f])
        offset += len(verts)
    obj_io.save_obj(output, all_verts, all_faces, "EVE composed model")
    print(f"[compose] {len(parts)} parts -> {output} ({len(all_verts)} verts)")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("usage: compose.py output.obj part1.obj part2.obj ...")
        sys.exit(1)
    compose(sys.argv[2:], sys.argv[1])
