from __future__ import annotations

import math

import pyglet
from pyglet.window import key, mouse

from scene import SceneItem, ShadeMode, save_scene

_MODE_KEYS = {
    key._0: None,
    key._1: ShadeMode.WIREFRAME,
    key._2: ShadeMode.GOURAUD,
    key._3: ShadeMode.PHONG,
    key._4: ShadeMode.PHONG_TEX,
    key._5: ShadeMode.PHONG_NORMAL,
}

TRANSLATE_STEP = 0.01
ROTATE_STEP_DEG = 1.0
SCALE_FACTOR = 1.01


class Control:
    def __init__(
        self,
        window,
        model_name: str,
        items: list[SceneItem],
        groups,
        scene_path: str,
    ):
        self.window = window
        self.model_name = model_name
        self.items = items
        self.groups = groups
        self.scene_path = scene_path
        self.index = 0 if items else -1
        self._home = (
            window.cam_yaw,
            window.cam_pitch,
            window.cam_distance,
            window.cam_target,
        )
        window.on_key_press = self.on_key_press
        window.on_key_release = self.on_key_release
        window.on_mouse_drag = self.on_mouse_drag
        window.on_mouse_scroll = self.on_mouse_scroll
        self._update_title()

    # --- selection ------------------------------------------------------

    def _update_title(self) -> None:
        prefix = f"CG PA3 — {self.model_name}"
        if not self.items:
            self.window.set_caption(f"{prefix}  (empty)")
            return
        it = self.items[self.index]
        self.window.set_caption(
            f"{prefix}  [{self.index + 1}/{len(self.items)}]  {it.name}"
        )

    def _cycle(self, step: int) -> None:
        if not self.items:
            return
        self.index = (self.index + step) % len(self.items)
        self._update_title()

    def _apply(self) -> None:
        if 0 <= self.index < len(self.items):
            self.groups[self.index].transform = self.items[
                self.index
            ].transform.matrix()

    # --- keys -----------------------------------------------------------

    def on_key_press(self, symbol, modifiers):
        shift = bool(modifiers & key.MOD_SHIFT)

        if symbol == key.TAB:
            self._cycle(-1 if shift else 1)
            return
        if not self.items:
            return

        tr = self.items[self.index].transform
        t = TRANSLATE_STEP
        r = ROTATE_STEP_DEG
        handled = True

        if symbol == key.MINUS:
            f = 1.0 / SCALE_FACTOR
            for i in range(3):
                tr.scale[i] *= f
        elif symbol == key.EQUAL:
            for i in range(3):
                tr.scale[i] *= SCALE_FACTOR
        elif shift:
            if symbol == key.H:
                tr.rotate_deg[1] -= r
            elif symbol == key.L:
                tr.rotate_deg[1] += r
            elif symbol == key.J:
                tr.rotate_deg[0] -= r
            elif symbol == key.K:
                tr.rotate_deg[0] += r
            elif symbol == key.N:
                tr.rotate_deg[2] -= r
            elif symbol == key.M:
                tr.rotate_deg[2] += r
            else:
                handled = False
        else:
            if symbol == key.H:
                tr.translate[0] -= t
            elif symbol == key.L:
                tr.translate[0] += t
            elif symbol == key.J:
                tr.translate[1] -= t
            elif symbol == key.K:
                tr.translate[1] += t
            elif symbol == key.N:
                tr.translate[2] -= t
            elif symbol == key.M:
                tr.translate[2] += t
            else:
                handled = False

        if handled:
            self._apply()

    def on_key_release(self, symbol, modifiers):
        if symbol == key.ESCAPE:
            pyglet.app.exit()
        elif symbol in (key.RETURN, key.ENTER):
            save_scene(self.scene_path, self.window.scene, self.items)
            print(f"saved {self.scene_path}")
        elif symbol == key.R:
            (
                self.window.cam_yaw,
                self.window.cam_pitch,
                self.window.cam_distance,
                self.window.cam_target,
            ) = self._home
        elif symbol in _MODE_KEYS:
            self.window.mode_override = _MODE_KEYS[symbol]

    # --- mouse ----------------------------------------------------------

    def on_mouse_drag(self, x, y, dx, dy, button, modifiers):
        if button & mouse.LEFT:
            self.window.orbit(-dx * 0.005, -dy * 0.005)
        elif button & (mouse.MIDDLE | mouse.RIGHT):
            scale = self.window.cam_distance * 0.002
            self.window.pan(-dx * scale, -dy * scale)

    def on_mouse_scroll(self, x, y, scroll_x, scroll_y):
        self.window.zoom(math.pow(0.9, scroll_y))
