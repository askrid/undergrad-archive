"""Arc-ball camera and cage-point pick/drag controller.

The orbit pivot is ``window.cam_target``. ``F`` reframes it on the scene
bbox; ``Shift+F`` snaps it onto the selected control point.
"""
from __future__ import annotations

import math
from typing import Optional, Tuple

import pyglet
from pyglet.math import Mat4, Vec3, Vec4
from pyglet.window import key, mouse


PICK_RADIUS_PX: float = 12.0
ORBIT_SENS: float = 0.005
PAN_SENS: float = 0.0015
ZOOM_FACTOR: float = 0.9


def _project(pt: Vec3, view_proj: Mat4, w: int, h: int
             ) -> Optional[Tuple[float, float, float]]:
    """World → (screen_x, screen_y, ndc_z). Returns None if behind camera."""
    v = view_proj @ Vec4(pt.x, pt.y, pt.z, 1.0)
    if v.w <= 1e-6:
        return None
    nx, ny, nz = v.x / v.w, v.y / v.w, v.z / v.w
    return (nx + 1.0) * 0.5 * w, (ny + 1.0) * 0.5 * h, nz


class Control:
    def __init__(self, window, scene_manager: Optional[object] = None) -> None:
        self.window = window
        self.scene = scene_manager

        # push_handlers (canonical pyglet 2.x). Plain ``window.on_X = fn``
        # assignment can be shadowed by class-level handlers on the Window
        # subclass; the stack-based form is robust.
        window.push_handlers(
            on_key_press=self.on_key_press,
            on_key_release=self.on_key_release,
            on_mouse_motion=self.on_mouse_motion,
            on_mouse_drag=self.on_mouse_drag,
            on_mouse_press=self.on_mouse_press,
            on_mouse_release=self.on_mouse_release,
            on_mouse_scroll=self.on_mouse_scroll,
        )

        # Spherical camera state around the pivot.
        self.radius: float = 6.0
        self.theta: float = 0.0
        self.phi: float = math.pi / 3.0

        self.dragging_point: Optional[int] = None
        self.dragging_camera: Optional[str] = None  # "orbit" | "pan"
        self.last_mouse: Tuple[int, int] = (0, 0)

        self._init_spherical_from_window()
        self._sync_eye()

    # ---------- Camera framing ----------

    def _init_spherical_from_window(self) -> None:
        off: Vec3 = self.window.cam_eye - self.window.cam_target
        r = math.sqrt(off.x * off.x + off.y * off.y + off.z * off.z)
        self.radius = max(r, 0.5)
        self.theta = math.atan2(off.z, off.x)
        self.phi = math.acos(max(-1.0, min(1.0, off.y / max(self.radius, 1e-6))))

    def frame_scene(self, target: Vec3, radius: float) -> None:
        self.window.cam_target = target
        self.radius = max(radius, 0.5)
        self._sync_eye()

    def _sync_eye(self) -> None:
        sp, cp = math.sin(self.phi), math.cos(self.phi)
        st, ct = math.sin(self.theta), math.cos(self.theta)
        off = Vec3(self.radius * sp * ct, self.radius * cp, self.radius * sp * st)
        self.window.cam_eye = self.window.cam_target + off

    def _orbit(self, dx: float, dy: float) -> None:
        self.theta += dx * ORBIT_SENS
        self.phi = max(0.05, min(math.pi - 0.05, self.phi + dy * ORBIT_SENS))
        self._sync_eye()

    def _pan(self, dx: float, dy: float) -> None:
        fwd = (self.window.cam_target - self.window.cam_eye).normalize()
        right = fwd.cross(self.window.cam_vup).normalize()
        up = right.cross(fwd).normalize()
        scale = self.radius * PAN_SENS
        self.window.cam_target = self.window.cam_target + right * (-dx * scale) + up * (-dy * scale)
        self._sync_eye()

    def _zoom(self, scroll_y: float) -> None:
        self.radius = max(0.1, self.radius * (ZOOM_FACTOR ** scroll_y))
        self._sync_eye()

    # ---------- Picking ----------

    def _vp(self) -> Mat4:
        view = Mat4.look_at(self.window.cam_eye, self.window.cam_target, self.window.cam_vup)
        return self.window.proj_mat @ view

    def _pick(self, mx: float, my: float) -> Optional[int]:
        if self.scene is None:
            return None
        pts = self.scene.cage_points()
        if not pts:
            return None
        vp = self._vp()
        w, h = self.window.width, self.window.height
        best: Optional[int] = None
        best_d2 = PICK_RADIUS_PX * PICK_RADIUS_PX
        for i, p in enumerate(pts):
            scr = _project(p, vp, w, h)
            if scr is None:
                continue
            d2 = (scr[0] - mx) ** 2 + (scr[1] - my) ** 2
            if d2 < best_d2:
                best_d2 = d2
                best = i
        return best

    def _drag_world_delta(self, idx: int, dx: float, dy: float) -> Vec3:
        """Convert pixel delta to a world-space offset in the view plane."""
        fwd = (self.window.cam_target - self.window.cam_eye).normalize()
        right = fwd.cross(self.window.cam_vup).normalize()
        up = right.cross(fwd).normalize()
        p: Vec3 = self.scene.cage_points()[idx]
        depth = max((p - self.window.cam_eye).dot(fwd), 0.1)
        world_per_px = 2.0 * depth * math.tan(math.radians(self.window.fov) * 0.5) / self.window.height
        return right * (dx * world_per_px) + up * (dy * world_per_px)

    # ---------- Events ----------

    def on_key_press(self, symbol: int, modifier: int) -> None:
        if symbol == key.F:
            if self.scene is not None and hasattr(self.scene, "frame_request"):
                self.scene.frame_request(self, on_selection=bool(modifier & key.MOD_SHIFT))
            return
        if self.scene is not None and hasattr(self.scene, "on_key"):
            self.scene.on_key(symbol, modifier)

    def on_key_release(self, symbol: int, modifier: int) -> None:
        if symbol == key.ESCAPE:
            pyglet.app.exit()

    def on_mouse_motion(self, x: int, y: int, dx: int, dy: int) -> None:
        self.last_mouse = (x, y)

    def on_mouse_press(self, x: int, y: int, button: int, modifier: int) -> None:
        self.last_mouse = (x, y)
        # Reset drag state in case a previous release was missed (release
        # outside window, focus loss, etc.).
        self.dragging_point = None
        self.dragging_camera = None
        if button == mouse.LEFT:
            hit = self._pick(x, y)
            if hit is not None and self.scene is not None:
                self.dragging_point = hit
                self.scene.set_selected(hit)
            else:
                self.dragging_camera = "orbit"
        elif button in (mouse.RIGHT, mouse.MIDDLE):
            self.dragging_camera = "pan"

    def on_mouse_release(self, x: int, y: int, button: int, modifier: int) -> None:
        self.dragging_point = None
        self.dragging_camera = None

    def on_mouse_drag(self, x: int, y: int, dx: int, dy: int, button: int, modifier: int) -> None:
        if self.dragging_point is not None and self.scene is not None:
            delta = self._drag_world_delta(self.dragging_point, dx, dy)
            self.scene.move_point(self.dragging_point, delta)
        elif self.dragging_camera == "orbit":
            self._orbit(dx, dy)
        elif self.dragging_camera == "pan":
            self._pan(dx, dy)
        self.last_mouse = (x, y)

    def on_mouse_scroll(self, x: int, y: int, scroll_x: float, scroll_y: float) -> None:
        self._zoom(scroll_y)
