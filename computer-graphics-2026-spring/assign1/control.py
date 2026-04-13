from __future__ import annotations

import math
import os
import subprocess
import sys
from typing import TYPE_CHECKING

import pyglet
from pyglet.window import key, mouse
from pyglet.math import Vec3

from scene import Joint, RotStep, TransStep

if TYPE_CHECKING:
    from render import RenderWindow


class Control:
    MOVE_SPEED: float = 5.0
    MOUSE_SENS: float = 0.005
    SCROLL_STEP: float = 0.5

    ROT_STEP: float = math.radians(5)
    ROT_STEP_BIG: float = math.radians(30)
    TRANS_STEP: float = 0.05
    TRANS_STEP_BIG: float = 0.3

    def __init__(self, window: "RenderWindow") -> None:
        window.on_key_press = self.on_key_press
        window.on_key_release = self.on_key_release
        window.on_mouse_motion = self.on_mouse_motion
        window.on_mouse_drag = self.on_mouse_drag
        window.on_mouse_press = self.on_mouse_press
        window.on_mouse_release = self.on_mouse_release
        window.on_mouse_scroll = self.on_mouse_scroll
        self.window = window

        self.keys_held: set[int] = set()
        self.yaw = 0.0
        self.pitch = 0.0

        self._joint_list: list[Joint] = []
        self._joint_idx = 0
        self._param_idx = 0

        self.setup()

    def setup(self) -> None:
        fwd = (self.window.cam_target - self.window.cam_eye).normalize()
        self.pitch = math.asin(max(-1.0, min(1.0, fwd.y)))
        self.yaw = math.atan2(fwd.x, -fwd.z)
        self._home_eye = Vec3(
            self.window.cam_eye.x, self.window.cam_eye.y, self.window.cam_eye.z
        )
        self._home_yaw = self.yaw
        self._home_pitch = self.pitch
        self._sync_target()
        pyglet.clock.schedule_interval(self.update, 1.0 / 60.0)

    def reset(self) -> None:
        self.window.cam_eye = Vec3(self._home_eye.x, self._home_eye.y, self._home_eye.z)
        self.yaw = self._home_yaw
        self.pitch = self._home_pitch
        self._sync_target()

    def _forward(self) -> Vec3:
        cp = math.cos(self.pitch)
        return Vec3(cp * math.sin(self.yaw), math.sin(self.pitch), -cp * math.cos(self.yaw))

    def _horizontal_right(self) -> Vec3:
        return Vec3(math.cos(self.yaw), 0.0, math.sin(self.yaw))

    def _sync_target(self) -> None:
        self.window.cam_target = self.window.cam_eye + self._forward()

    # -- Joint poser --

    def _ensure_joints(self) -> None:
        if self._joint_list:
            return
        from animate import _collect_joints
        if self.window.scene is None:
            return
        joints: dict[str, Joint] = {}
        _collect_joints(self.window.scene.root, joints)
        self._joint_list = list(joints.values())

    def _active_joint(self) -> Joint | None:
        self._ensure_joints()
        if not self._joint_list:
            return None
        return self._joint_list[self._joint_idx % len(self._joint_list)]

    def _step_for_param(self, joint: Joint, param_idx: int, big: bool = False) -> float:
        for step in joint.steps:
            if isinstance(step, RotStep) and step.index == param_idx:
                return self.ROT_STEP_BIG if big else self.ROT_STEP
            if isinstance(step, TransStep) and step.index <= param_idx < step.index + 3:
                return self.TRANS_STEP_BIG if big else self.TRANS_STEP
        return self.ROT_STEP_BIG if big else self.ROT_STEP

    def _print_joints(self) -> None:
        self._ensure_joints()
        parts = [f'"{j.name}":[{",".join(f"{v:.4f}" for v in j.params)}]' for j in self._joint_list]
        clip = "{" + ",".join(parts) + "}"
        print(clip)
        try:
            subprocess.Popen(["wl-copy"], stdin=subprocess.PIPE).communicate(clip.encode())
        except FileNotFoundError:
            pass

    def _show_poser_status(self) -> None:
        j = self._active_joint()
        if j is None:
            return
        idx = self._param_idx % len(j.params)
        label = f"p[{idx}]"
        for step in j.steps:
            if isinstance(step, RotStep) and step.index == idx:
                label = f"rot({step.axis}) [{idx}]"
                break
            if isinstance(step, TransStep) and step.index <= idx < step.index + 3:
                label = f"trans.{'xyz'[idx - step.index]} [{idx}]"
                break
        print(f"[poser] {j.name}  {label} = {j.params[idx]:.4f}  (deg={math.degrees(j.params[idx]):.1f})")

    def _adjust_param(self, delta_sign: int, modifiers: int) -> None:
        j = self._active_joint()
        if j is None:
            return
        idx = self._param_idx % len(j.params)
        big = bool(modifiers & key.MOD_SHIFT)
        j.params[idx] += delta_sign * self._step_for_param(j, idx, big)
        self._show_poser_status()

    def _cycle_param(self, delta: int) -> None:
        j = self._active_joint()
        if j is not None:
            self._param_idx = (self._param_idx + delta) % len(j.params)
            self._show_poser_status()

    # -- Per-frame tick --

    def update(self, dt: float) -> None:
        if not self.keys_held:
            return
        fwd = self._forward()
        right = self._horizontal_right()
        up = Vec3(0.0, 1.0, 0.0)
        move = Vec3(0.0, 0.0, 0.0)
        for k, d in [(key.W, fwd), (key.S, -fwd), (key.D, right), (key.A, -right),
                      (key.Q, up), (key.E, -up)]:
            if k in self.keys_held:
                move = move + d
        if move.x == 0.0 and move.y == 0.0 and move.z == 0.0:
            return
        self.window.cam_eye = self.window.cam_eye + move.normalize() * (self.MOVE_SPEED * dt)
        self._sync_target()

    # -- Event handlers --

    def on_key_press(self, symbol: int, modifiers: int) -> None:
        self.keys_held.add(symbol)

    def on_key_release(self, symbol: int, modifiers: int) -> None:
        self.keys_held.discard(symbol)
        if symbol == key.ESCAPE:
            pyglet.app.exit()
        elif symbol == key.SPACE:
            self.window.animate = not self.window.animate
            if self.window.player is not None:
                if self.window.animate:
                    self.window.player.play_audio()
                else:
                    self.window.player.pause_audio()
                    print(f"[pause] t={self.window.player.elapsed:.2f}s")
        elif symbol == key.R:
            self.reset()
        elif symbol == key.G and self.window.scene is not None:
            self.window.scene.toggle_debug_axes()
        elif symbol == key.Z:
            pyglet.app.exit()
            os.execv(sys.executable, [sys.executable] + sys.argv)
        elif key._1 <= symbol <= key._9:
            self._ensure_joints()
            idx = symbol - key._1
            if idx < len(self._joint_list):
                self._joint_idx = idx
                self._param_idx = 0
                self._show_poser_status()
        elif symbol == key.J:
            self._cycle_param(1)
        elif symbol == key.K:
            self._cycle_param(-1)
        elif symbol == key.H:
            self._adjust_param(-1, modifiers)
        elif symbol == key.L:
            self._adjust_param(1, modifiers)
        elif symbol == key.P:
            self._print_joints()
        elif symbol in (key.BRACKETLEFT, key.BRACKETRIGHT):
            if self.window.player is not None:
                self.window.animate = False
                self.window.player.seek_keyframe(1 if symbol == key.BRACKETRIGHT else -1)
                print(f"[scrub] t={self.window.player.elapsed:.2f}s")

    def on_mouse_motion(self, x: int, y: int, dx: int, dy: int) -> None:
        pass

    def on_mouse_press(self, x: int, y: int, button: int, modifiers: int) -> None:
        pass

    def on_mouse_release(self, x: int, y: int, button: int, modifiers: int) -> None:
        pass

    def on_mouse_drag(
        self, x: int, y: int, dx: int, dy: int, buttons: int, modifiers: int
    ) -> None:
        if not (buttons & mouse.LEFT):
            return
        self.yaw += dx * self.MOUSE_SENS
        self.pitch = max(-math.radians(89), min(math.radians(89), self.pitch + dy * self.MOUSE_SENS))
        self._sync_target()

    def on_mouse_scroll(self, x: int, y: int, scroll_x: float, scroll_y: float) -> None:
        self.window.cam_eye = self.window.cam_eye + self._forward() * (scroll_y * self.SCROLL_STEP)
        self._sync_target()
