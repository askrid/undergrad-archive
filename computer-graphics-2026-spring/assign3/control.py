"""Mouse + keyboard input.

Left-drag orbits, right/middle-drag pans, scroll zooms.
1..5 force shading mode, 0 restores per-shape mode, R resets camera,
Space toggles animation, Esc quits.
"""

from __future__ import annotations

import math

import pyglet
from pyglet.window import key, mouse

from scene import ShadeMode

_MODE_KEYS = {
    key._0: None,
    key._1: ShadeMode.WIREFRAME,
    key._2: ShadeMode.GOURAUD,
    key._3: ShadeMode.PHONG,
    key._4: ShadeMode.PHONG_TEX,
    key._5: ShadeMode.PHONG_NORMAL,
}


class Control:
    def __init__(self, window):
        self.window = window
        self._home = (
            window.cam_yaw,
            window.cam_pitch,
            window.cam_distance,
            window.cam_target,
        )
        window.on_key_release = self.on_key_release
        window.on_mouse_drag = self.on_mouse_drag
        window.on_mouse_scroll = self.on_mouse_scroll

    def on_key_release(self, symbol, modifiers):
        if symbol == key.ESCAPE:
            pyglet.app.exit()
        elif symbol == key.R:
            (
                self.window.cam_yaw,
                self.window.cam_pitch,
                self.window.cam_distance,
                self.window.cam_target,
            ) = self._home
        elif symbol in _MODE_KEYS:
            self.window.mode_override = _MODE_KEYS[symbol]

    def on_mouse_drag(self, x, y, dx, dy, button, modifiers):
        if button & mouse.LEFT:
            self.window.orbit(-dx * 0.005, -dy * 0.005)
        elif button & (mouse.MIDDLE | mouse.RIGHT):
            scale = self.window.cam_distance * 0.002
            self.window.pan(-dx * scale, -dy * scale)

    def on_mouse_scroll(self, x, y, scroll_x, scroll_y):
        self.window.zoom(math.pow(0.9, scroll_y))
