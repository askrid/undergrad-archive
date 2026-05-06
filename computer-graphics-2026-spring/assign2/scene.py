"""Single-scene model. Owns the cage + derived surface, handles edits + export.

The mode (bezier / bspline / catmull) and the input .obj are picked at
launch from CLI args (see ``main.py``). ``F`` reframes camera to bbox,
``Shift+F`` to selected pt, ``S/Shift+S`` toggles CC level, ``E`` exports.
"""
from __future__ import annotations

import math
import os
from typing import List, Optional, Sequence, Tuple

from pyglet.math import Mat4, Vec3
from pyglet.window import key

import obj_io
import surfaces
from surfaces import V3


# Flat palette, picked to read well on the black background.
CAGE_LINE       = (148, 163, 184, 255)   # slate-400
CAGE_POINT      = (251, 191,  36, 255)   # amber-400
SELECTED_POINT  = (244,  63,  94, 255)   # rose-500
SURFACE_FILL    = (180, 180, 180, 255)   # neutral grey
SUBDIV_FILL     = (180, 180, 180, 255)   # neutral grey
SURFACE_SAMPLE  = ( 56, 189, 248, 255)   # sky-400


def _flat(vs: Sequence[V3]) -> List[float]:
    out: List[float] = []
    for x, y, z in vs:
        out += [float(x), float(y), float(z)]
    return out


def _edges(faces: Sequence[Sequence[int]]) -> List[Tuple[int, int]]:
    seen: set = set()
    out: List[Tuple[int, int]] = []
    for f in faces:
        m = len(f)
        for k in range(m):
            a, b = f[k], f[(k + 1) % m]
            key = (a, b) if a < b else (b, a)
            if key not in seen:
                seen.add(key)
                out.append(key)
    return out


# ============================================================
# Base scene
# ============================================================

class Scene:
    name: str = "scene"
    transform: Mat4 = Mat4()  # identity

    def __init__(self, window, cage_path: str) -> None:
        self.window = window
        self.cage_path: str = cage_path
        verts, faces = obj_io.load_obj(cage_path)
        self.cage_verts: List[V3] = list(verts)
        self.cage_faces: List[List[int]] = list(faces)
        self.cage_edges: List[Tuple[int, int]] = _edges(self.cage_faces)
        self.selected: Optional[int] = None
        self.cage_lines_group = None
        self.cage_points_group = None
        self.surface_group = None
        self.surface_pts_group = None      # cyan sample dots (V toggle)
        self.show_surface_pts: bool = False
        self._cached_surface: Tuple[List[V3], List[List[int]]] = ([], [])
        self._build_visuals()
        self.rebuild_surface()

    # ---- subclass hooks ----

    def compute_surface(self) -> Tuple[List[V3], List[V3], List[List[int]]]:
        raise NotImplementedError

    def _surface_color(self) -> Tuple[int, int, int, int]:
        return SURFACE_FILL

    def on_key(self, symbol: int, modifier: int) -> None:
        pass

    # ---- visuals ----

    def _build_visuals(self) -> None:
        edge_idx: List[int] = []
        for a, b in self.cage_edges:
            edge_idx += [a, b]
        self.cage_lines_group = self.window.add_lines(
            self.transform, _flat(self.cage_verts), edge_idx,
            list(CAGE_LINE) * len(self.cage_verts),
        )
        self.cage_points_group = self.window.add_points(
            self.transform, _flat(self.cage_verts),
            list(CAGE_POINT) * len(self.cage_verts),
        )

    def rebuild_surface(self) -> None:
        verts, normals, faces = self.compute_surface()
        flat_idx: List[int] = []
        for f in obj_io.triangulate(faces):
            flat_idx += list(f)
        colors = list(self._surface_color()) * len(verts)
        if self.surface_group is None:
            self.surface_group = self.window.add_mesh(
                self.transform, _flat(verts), flat_idx, colors, _flat(normals),
            )
        else:
            # Swap the vlist on the existing group: avoids shader recompile
            # and works around a pyglet 2.1.x quirk where the in-place
            # ``vlist.vertices[:] = …`` path leaves the GPU rendering stale
            # positions after a topology rebuild + cage edit.
            self.window.replace_mesh_vlist(
                self.surface_group, _flat(verts), flat_idx, colors, _flat(normals),
            )
        self._cached_surface = (verts, faces)
        if self.show_surface_pts:
            self._rebuild_surface_pts(verts)

    def _rebuild_surface_pts(self, verts: Sequence[V3]) -> None:
        """Recreate the cyan sample-dot overlay for the current surface."""
        if self.surface_pts_group is not None:
            self.window.remove_group(self.surface_pts_group)
            self.surface_pts_group = None
        if not self.show_surface_pts or not verts:
            return
        cols = list(SURFACE_SAMPLE) * len(verts)
        self.surface_pts_group = self.window.add_points(
            self.transform, _flat(verts), cols,
        )

    def toggle_surface_pts(self) -> None:
        self.show_surface_pts = not self.show_surface_pts
        self._rebuild_surface_pts(self._cached_surface[0])
        print(f"[show surface verts] {self.show_surface_pts} "
              f"({len(self._cached_surface[0])} samples)")

    def _push_cage(self) -> None:
        flat = _flat(self.cage_verts)
        self.cage_lines_group.indexed_vertices_list.vertices[:] = flat
        self.cage_points_group.indexed_vertices_list.vertices[:] = flat

    def _refresh_point_colors(self) -> None:
        cols = list(CAGE_POINT) * len(self.cage_verts)
        if self.selected is not None and 0 <= self.selected < len(self.cage_verts):
            base = self.selected * 4
            cols[base:base + 4] = list(SELECTED_POINT)
        self.cage_points_group.indexed_vertices_list.colors[:] = cols

    # ---- edit API (called by Control via SceneManager) ----

    def cage_points(self) -> List[Vec3]:
        return [Vec3(*v) for v in self.cage_verts]

    def move_point(self, idx: int, delta: Vec3) -> None:
        x, y, z = self.cage_verts[idx]
        self.cage_verts[idx] = (x + delta.x, y + delta.y, z + delta.z)
        self._push_cage()
        self.rebuild_surface()

    def set_selected(self, idx: Optional[int]) -> None:
        self.selected = idx
        self._refresh_point_colors()

    # ---- camera framing ----

    def center_and_radius(self) -> Tuple[Vec3, float]:
        if not self.cage_verts:
            return Vec3(0, 0, 0), 2.0
        xs = [v[0] for v in self.cage_verts]
        ys = [v[1] for v in self.cage_verts]
        zs = [v[2] for v in self.cage_verts]
        lx, hx = min(xs), max(xs)
        ly, hy = min(ys), max(ys)
        lz, hz = min(zs), max(zs)
        center = Vec3((lx + hx) * 0.5, (ly + hy) * 0.5, (lz + hz) * 0.5)
        diag = math.sqrt((hx - lx) ** 2 + (hy - ly) ** 2 + (hz - lz) ** 2)
        return center, max(diag * 1.4, 2.0)

    # ---- export ----

    def export(self, out_dir: str) -> None:
        os.makedirs(out_dir, exist_ok=True)
        base = os.path.splitext(os.path.basename(self.cage_path))[0]
        obj_io.save_obj(os.path.join(out_dir, f"{base}_{self.name}_cage.obj"),
                        self.cage_verts, self.cage_faces,
                        comment=f"{self.name} edited cage")
        verts, faces = self._cached_surface
        obj_io.save_obj(os.path.join(out_dir, f"{base}_{self.name}_surface.obj"),
                        verts, faces, comment=f"{self.name} surface")


# ============================================================
# Bezier / B-Spline (bicubic patches tiled over an MxN net)
# ============================================================

class _PatchScene(Scene):
    kind: str = "bezier"
    steps: int = 24

    def __init__(self, window, cage_path: str, rows: int, cols: int) -> None:
        self.rows, self.cols = rows, cols
        super().__init__(window, cage_path)

    def compute_surface(self):
        grid = surfaces.grid_from_flat(self.cage_verts, self.rows, self.cols)
        return surfaces.tile_patches(grid, self.steps, self.kind)


class BezierScene(_PatchScene):
    name = "bezier"
    kind = "bezier"


class BSplineScene(_PatchScene):
    name = "bspline"
    kind = "bspline"


# ============================================================
# Catmull-Clark
# ============================================================

class CatmullClarkScene(Scene):
    name: str = "catmull"
    max_level: int = 5

    def __init__(self, window, cage_path: str, level: int = 1) -> None:
        self.level: int = level
        super().__init__(window, cage_path)

    def _surface_color(self):
        return SUBDIV_FILL

    def compute_surface(self):
        verts: List[V3] = list(self.cage_verts)
        faces: List[List[int]] = [list(f) for f in self.cage_faces]
        for _ in range(self.level):
            verts, faces = surfaces.catmull_clark(verts, faces)
        normals = surfaces.compute_vertex_normals(verts, obj_io.triangulate(faces))
        return verts, normals, faces

    def on_key(self, symbol: int, modifier: int) -> None:
        if symbol != key.S:
            return
        if modifier & key.MOD_SHIFT:
            self.level = max(0, self.level - 1)
        else:
            self.level = min(self.max_level, self.level + 1)
        print(f"[catmull-clark] level={self.level}")
        self.rebuild_surface()

    def export(self, out_dir: str) -> None:
        os.makedirs(out_dir, exist_ok=True)
        base = os.path.splitext(os.path.basename(self.cage_path))[0]
        obj_io.save_obj(os.path.join(out_dir, f"{base}_{self.name}_L0_cage.obj"),
                        self.cage_verts, self.cage_faces,
                        comment="control cage (level 0)")
        verts: List[V3] = list(self.cage_verts)
        faces: List[List[int]] = [list(f) for f in self.cage_faces]
        for L in range(1, self.max_level + 1):
            verts, faces = surfaces.catmull_clark(verts, faces)
            obj_io.save_obj(os.path.join(out_dir, f"{base}_{self.name}_L{L}.obj"),
                            verts, faces, comment=f"Catmull-Clark level {L}")


# ============================================================
# Manager: routes Control calls and key events
# ============================================================

class SceneManager:
    def __init__(self, window, scene: Scene) -> None:
        self.window = window
        self.current: Scene = scene
        self.controller = None  # set by main.py after Control() is built

    # ---- delegated by Control ----

    def cage_points(self) -> List[Vec3]:
        return self.current.cage_points()

    def move_point(self, idx: int, delta: Vec3) -> None:
        self.current.move_point(idx, delta)

    def set_selected(self, idx: Optional[int]) -> None:
        self.current.set_selected(idx)

    def frame_request(self, controller, on_selection: bool = False) -> None:
        if on_selection and self.current.selected is not None:
            v = self.current.cage_verts[self.current.selected]
            controller.frame_scene(Vec3(*v), controller.radius)
        else:
            center, radius = self.current.center_and_radius()
            controller.frame_scene(center, radius)

    def frame_initial(self) -> None:
        if self.controller is not None:
            self.frame_request(self.controller)

    def on_key(self, symbol: int, modifier: int) -> None:
        if symbol == key.E:
            self.current.export("out")
            print(f"[exported] {self.current.name} -> out/")
        elif symbol == key.V:
            self.current.toggle_surface_pts()
        else:
            self.current.on_key(symbol, modifier)
