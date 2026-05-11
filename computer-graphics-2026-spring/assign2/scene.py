"""Single-scene model. Owns the cage + derived surface, handles edits + save.

The mode (bezier / bspline / catmull) and the input .obj are picked at
launch from CLI args (see ``main.py``). ``F`` reframes camera to bbox,
``Shift+F`` to selected pt, ``S/Shift+S`` toggles CC level, ``E`` saves,
``Shift+E`` saves versioned snapshot, ``M`` toggles X-axis mirror.
"""
from __future__ import annotations

import glob as globmod
import math
import os
import re
from collections import deque
from typing import Deque, Dict, List, Optional, Sequence, Tuple

from pyglet.math import Mat4, Vec3
from pyglet.window import key

import obj_io
import surfaces
from surfaces import V3

MIRROR_TOL: float = 1e-4
MIRROR_PLANE_FILL = (100, 140, 200, 30)    # translucent blue sheet
MIRROR_PLANE_EDGE = (120, 160, 210, 100)   # slightly brighter border


# Flat palette, picked to read well on the black background.
CAGE_LINE       = (148, 163, 184, 255)   # slate-400
CAGE_POINT      = (251, 191,  36, 255)   # amber-400
SELECTED_POINT  = (244,  63,  94, 255)   # rose-500
SURFACE_FILL    = (180, 180, 180, 255)   # neutral grey
SUBDIV_FILL     = (180, 180, 180, 255)   # neutral grey
SURFACE_SAMPLE  = ( 56, 189, 248, 255)   # sky-400
REF_COLOR       = (100, 105, 115, 130)   # dim semi-transparent


_flat = obj_io.flatten_v3


def _build_mirror_pairs(verts: Sequence[V3], axis: int = 0
                        ) -> Tuple[Dict[int, int], float]:
    """Map each vertex to its mirror partner across the bbox-center plane.

    Returns (pairs_dict, center_coord).
    Vertices on the mirror plane map to themselves.
    O(n²) but cage sizes are small (< 200 verts).
    """
    center = 0.0

    pairs: Dict[int, int] = {}
    for i, v in enumerate(verts):
        if i in pairs:
            continue
        if abs(v[axis] - center) < MIRROR_TOL:
            pairs[i] = i
            continue
        target = list(v)
        target[axis] = 2.0 * center - target[axis]
        best_d = MIRROR_TOL
        best_j: Optional[int] = None
        for j, w in enumerate(verts):
            if j == i or j in pairs:
                continue
            d = math.sqrt(sum((a - b) ** 2 for a, b in zip(target, w)))
            if d < best_d:
                best_d = d
                best_j = j
        if best_j is not None:
            pairs[i] = best_j
            pairs[best_j] = i
    return pairs, center


def _surface_path(cage_path: str) -> str:
    """cage.obj → cage_surface.obj"""
    root, ext = os.path.splitext(cage_path)
    return f"{root}_surface{ext}"


def _next_version(cage_path: str) -> int:
    """Find the next unused _vNNN index for versioned saves."""
    root, ext = os.path.splitext(cage_path)
    pattern = f"{root}_v*{ext}"
    existing = globmod.glob(pattern)
    max_v = 0
    for p in existing:
        m = re.search(r"_v(\d+)" + re.escape(ext) + "$", p)
        if m:
            max_v = max(max_v, int(m.group(1)))
    return max_v + 1


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
        # Undo / redo
        self._undo: Deque[List[V3]] = deque(maxlen=64)
        self._redo: List[List[V3]] = []
        # X-axis mirror — pairs computed once from the loaded cage
        self.mirror_on: bool = False
        self.mirror_pairs, self.mirror_center = _build_mirror_pairs(self.cage_verts)
        self._mirror_fill = None
        self._mirror_edge = None
        self._dragging: bool = False
        self.show_cage: bool = True
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
            self.window.replace_mesh_vlist(
                self.surface_group, _flat(verts), flat_idx, colors, _flat(normals),
            )
        self._cached_surface = (verts, faces)
        if self.show_surface_pts:
            self._rebuild_surface_pts(verts)

    def _rebuild_surface_pts(self, verts: Sequence[V3]) -> None:
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

    def toggle_cage(self) -> None:
        self.show_cage = not self.show_cage
        flat = _flat(self.cage_verts) if self.show_cage else [0.0] * (len(self.cage_verts) * 3)
        self.cage_lines_group.indexed_vertices_list.vertices[:] = flat
        self.cage_points_group.indexed_vertices_list.vertices[:] = flat
        if not self.show_cage:
            self.selected = None
        print(f"[cage] {'ON' if self.show_cage else 'OFF'}")

    def load_references(self, paths: Sequence[str]) -> None:
        """Load .obj files as dim static meshes. Missing files skipped."""
        for path in paths:
            if not os.path.isfile(path):
                continue
            verts, faces = obj_io.load_obj(path)
            if not verts:
                continue
            normals = surfaces.compute_vertex_normals(verts, obj_io.triangulate(faces))
            flat_idx: List[int] = []
            for f in obj_io.triangulate(faces):
                flat_idx += list(f)
            colors = list(REF_COLOR) * len(verts)
            self.window.add_mesh(
                self.transform, _flat(verts), flat_idx, colors, _flat(normals),
            )
            print(f"[ref] {path} ({len(verts)} verts)")

    def _push_cage(self) -> None:
        if not self.show_cage:
            return
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
        if not self.show_cage:
            return []
        return [Vec3(*v) for v in self.cage_verts]

    def move_point(self, idx: int, delta: Vec3) -> None:
        x, y, z = self.cage_verts[idx]
        self.cage_verts[idx] = (x + delta.x, y + delta.y, z + delta.z)
        if self.mirror_on and idx in self.mirror_pairs:
            j = self.mirror_pairs[idx]
            if j == idx:
                # On the mirror plane — clamp X to center
                v = self.cage_verts[idx]
                self.cage_verts[idx] = (self.mirror_center, v[1], v[2])
            else:
                mx, my, mz = self.cage_verts[j]
                self.cage_verts[j] = (mx - delta.x, my + delta.y, mz + delta.z)
        self._push_cage()
        self.rebuild_surface()

    def set_selected(self, idx: Optional[int]) -> None:
        self.selected = idx
        self._refresh_point_colors()

    def toggle_mirror(self) -> None:
        self.mirror_on = not self.mirror_on
        if self.mirror_on:
            n_paired = sum(1 for i, j in self.mirror_pairs.items() if i != j) // 2
            n_center = sum(1 for i, j in self.mirror_pairs.items() if i == j)
            print(f"[mirror] ON — plane x={self.mirror_center:.2f}, "
                  f"{n_paired} pairs, {n_center} center pts")
            self._show_mirror_plane()
        else:
            self._hide_mirror_plane()
            print("[mirror] OFF")

    def _mirror_plane_verts(self) -> List[float]:
        xs = [v[0] for v in self.cage_verts]
        ys = [v[1] for v in self.cage_verts]
        zs = [v[2] for v in self.cage_verts]
        # For degenerate axes (flat models), expand to half the max span
        span = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs), 0.5)
        ly, hy = min(ys), max(ys)
        lz, hz = min(zs), max(zs)
        if hy - ly < 1e-6:
            mid = (ly + hy) * 0.5
            ly, hy = mid - span * 0.25, mid + span * 0.25
        if hz - lz < 1e-6:
            mid = (lz + hz) * 0.5
            lz, hz = mid - span * 0.25, mid + span * 0.25
        pad_y = (hy - ly) * 0.15
        pad_z = (hz - lz) * 0.15
        y0, y1 = ly - pad_y, hy + pad_y
        z0, z1 = lz - pad_z, hz + pad_z
        cx = self.mirror_center
        return [cx, y0, z0,  cx, y0, z1,  cx, y1, z1,  cx, y1, z0]

    _DEGENERATE = [0.0] * 12

    def _show_mirror_plane(self) -> None:
        # Groups created once, then only vertices updated (pyglet batch quirk).
        verts = self._mirror_plane_verts()
        if self._mirror_fill is None:
            tri_idx = [0, 1, 2, 0, 2, 3]
            self._mirror_fill = self.window.add_flat_triangles(
                self.transform, verts, tri_idx, list(MIRROR_PLANE_FILL) * 4,
            )
            edge_idx = [0, 1, 1, 2, 2, 3, 3, 0]
            self._mirror_edge = self.window.add_lines(
                self.transform, verts, edge_idx, list(MIRROR_PLANE_EDGE) * 4,
            )
        else:
            self._mirror_fill.indexed_vertices_list.vertices[:] = verts
            self._mirror_edge.indexed_vertices_list.vertices[:] = verts

    def _hide_mirror_plane(self) -> None:
        if self._mirror_fill is not None:
            self._mirror_fill.indexed_vertices_list.vertices[:] = self._DEGENERATE
            self._mirror_edge.indexed_vertices_list.vertices[:] = self._DEGENERATE

    # ---- undo / redo ----

    def push_undo(self) -> None:
        self._undo.append(list(self.cage_verts))
        self._redo.clear()

    def undo(self) -> None:
        if not self._undo:
            print("[undo] nothing to undo")
            return
        self._redo.append(list(self.cage_verts))
        self.cage_verts = self._undo.pop()
        self._push_cage()
        self.rebuild_surface()
        print(f"[undo] {len(self._undo)} remaining")

    def redo(self) -> None:
        if not self._redo:
            print("[redo] nothing to redo")
            return
        self._undo.append(list(self.cage_verts))
        self.cage_verts = self._redo.pop()
        self._push_cage()
        self.rebuild_surface()
        print(f"[redo] {len(self._redo)} remaining")

    # ---- drag lifecycle ----

    def begin_drag(self) -> None:
        self._dragging = True

    def end_drag(self) -> None:
        self._dragging = False
        self.rebuild_surface()

    # ---- whole-cage nudge ----

    NUDGE_STEP: float = 0.05

    def nudge(self, dx: float, dy: float, dz: float) -> None:
        self.push_undo()
        sx, sy, sz = dx * self.NUDGE_STEP, dy * self.NUDGE_STEP, dz * self.NUDGE_STEP
        self.cage_verts = [(x + sx, y + sy, z + sz) for x, y, z in self.cage_verts]
        self._push_cage()
        self.rebuild_surface()

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

    # ---- save ----

    def save(self) -> None:
        obj_io.save_obj(self.cage_path, self.cage_verts, self.cage_faces,
                        comment=f"{self.name} cage")
        surf_path = _surface_path(self.cage_path)
        verts, faces = self._cached_surface
        obj_io.save_obj(surf_path, verts, faces, comment=f"{self.name} surface")
        print(f"[saved] {self.cage_path}")
        print(f"[saved] {surf_path}")

    def save_versioned(self) -> None:
        v = _next_version(self.cage_path)
        root, ext = os.path.splitext(self.cage_path)
        cage_v = f"{root}_v{v:03d}{ext}"
        surf_v = f"{root}_v{v:03d}_surface{ext}"
        obj_io.save_obj(cage_v, self.cage_verts, self.cage_faces,
                        comment=f"{self.name} cage v{v}")
        verts, faces = self._cached_surface
        obj_io.save_obj(surf_v, verts, faces, comment=f"{self.name} surface v{v}")
        print(f"[snapshot] {cage_v}")
        print(f"[snapshot] {surf_v}")


# ============================================================
# Bezier / B-Spline (bicubic patches tiled over an MxN net)
# ============================================================

class _PatchScene(Scene):
    kind: str = "bezier"
    steps: int = 24
    drag_steps: int = 8

    def __init__(self, window, cage_path: str, rows: int, cols: int) -> None:
        self.rows, self.cols = rows, cols
        super().__init__(window, cage_path)

    def compute_surface(self):
        steps = self.drag_steps if self._dragging else self.steps
        grid = surfaces.grid_from_flat(self.cage_verts, self.rows, self.cols)
        return surfaces.tile_patches(grid, steps, self.kind)


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
        level = min(self.level, 1) if self._dragging else self.level
        verts: List[V3] = list(self.cage_verts)
        faces: List[List[int]] = [list(f) for f in self.cage_faces]
        for _ in range(level):
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

    def save(self) -> None:
        obj_io.save_obj(self.cage_path, self.cage_verts, self.cage_faces,
                        comment=f"catmull-clark cage (level 0)")
        surf_path = _surface_path(self.cage_path)
        verts, faces = self._cached_surface
        obj_io.save_obj(surf_path, verts, faces,
                        comment=f"catmull-clark surface (level {self.level})")
        print(f"[saved] {self.cage_path}")
        print(f"[saved] {surf_path} (L{self.level})")


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

    def push_undo(self) -> None:
        self.current.push_undo()

    def begin_drag(self) -> None:
        self.current.begin_drag()

    def end_drag(self) -> None:
        self.current.end_drag()

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

    _NUDGE_MAP = {
        key.H: (-1, 0, 0), key.L: (1, 0, 0),
        key.J: (0, -1, 0), key.K: (0, 1, 0),
        key.N: (0, 0, -1), key.COMMA: (0, 0, 1),
    }

    def on_key(self, symbol: int, modifier: int) -> None:
        if symbol in self._NUDGE_MAP:
            dx, dy, dz = self._NUDGE_MAP[symbol]
            self.current.nudge(dx, dy, dz)
        elif symbol == key.Z:
            if modifier & key.MOD_SHIFT:
                self.current.redo()
            else:
                self.current.undo()
        elif symbol == key.E:
            if modifier & key.MOD_SHIFT:
                self.current.save_versioned()
            else:
                self.current.save()
        elif symbol == key.M:
            self.current.toggle_mirror()
        elif symbol == key.C:
            self.current.toggle_cage()
        elif symbol == key.V:
            self.current.toggle_surface_pts()
        else:
            self.current.on_key(symbol, modifier)
