"""Minimal OBJ loader/saver for control nets and meshes.

Preserves n-gon faces (no auto-triangulation) so Catmull-Clark and quad
control nets stay native. Vertex indices in returned faces are 0-based.
"""
from __future__ import annotations

import os
from typing import List, Tuple


Vertex = Tuple[float, float, float]
Face = List[int]  # 0-based vertex indices, polygon order preserved


def load_obj(path: str) -> Tuple[List[Vertex], List[Face]]:
    verts: List[Vertex] = []
    faces: List[Face] = []
    with open(path, "r") as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            tok = s.split()
            tag = tok[0]
            if tag == "v":
                verts.append((float(tok[1]), float(tok[2]), float(tok[3])))
            elif tag == "f":
                idx: Face = []
                for v in tok[1:]:
                    # support i, i/t, i/t/n, i//n
                    head = v.split("/")[0]
                    j = int(head)
                    if j < 0:
                        j = len(verts) + j  # negative is relative
                    else:
                        j -= 1
                    idx.append(j)
                faces.append(idx)
    return verts, faces


def save_obj(path: str, verts: List[Vertex], faces: List[Face], comment: str | None = None) -> None:
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with open(path, "w") as f:
        if comment:
            for line in comment.splitlines():
                f.write(f"# {line}\n")
        for x, y, z in verts:
            f.write(f"v {x:.6f} {y:.6f} {z:.6f}\n")
        for face in faces:
            ids = " ".join(str(i + 1) for i in face)
            f.write(f"f {ids}\n")


def triangulate(faces: List[Face]) -> List[Face]:
    """Fan-triangulate any n-gons. Used only when uploading to GL_TRIANGLES."""
    out: List[Face] = []
    for face in faces:
        if len(face) == 3:
            out.append(face)
        else:
            v0 = face[0]
            for i in range(1, len(face) - 1):
                out.append([v0, face[i], face[i + 1]])
    return out
