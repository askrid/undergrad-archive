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
    """
    Camera controls

      W / S           move forward / backward (along the current look direction)
      A / D           strafe left / right     (horizontal, ignores pitch)
      Q / E           move up / down          (world Y axis)
      Left-drag mouse yaw / pitch the camera
      Scroll wheel    dolly along forward
      R               reset camera to its starting pose
      G               toggle per-node debug axes (gizmos) on the scene
      Z               restart the whole program (picks up model edits)
      ESC             quit

    Joint poser:
      1-9             select joint (by index in the collected list)
      j / k           cycle param within selected joint (down / up)
      h / l           decrement / increment active param
      P               print all joint params (copy-pasteable for keyframes)
      [ / ]           scrub to prev / next keyframe
      SPACE           toggle animation playback
    """

    MOVE_SPEED: float = 5.0  # world units per second
    MOUSE_SENS: float = 0.005  # radians per pixel of mouse motion
    SCROLL_STEP: float = 0.5  # world units per scroll tick

    ROT_STEP: float = math.radians(5)  # 5° per press
    ROT_STEP_BIG: float = math.radians(30)  # 30° with shift
    TRANS_STEP: float = 0.05  # world units per press
    TRANS_STEP_BIG: float = 0.3  # with shift

    window: "RenderWindow"
    keys_held: set[int]
    yaw: float
    pitch: float
    _home_eye: Vec3
    _home_yaw: float
    _home_pitch: float

    # Joint poser state
    _joint_list: list[Joint]
    _joint_idx: int
    _param_idx: int

    def __init__(self, window: "RenderWindow") -> None:
        window.on_key_press = self.on_key_press
        window.on_key_release = self.on_key_release
        window.on_mouse_motion = self.on_mouse_motion
        window.on_mouse_drag = self.on_mouse_drag
        window.on_mouse_press = self.on_mouse_press
        window.on_mouse_release = self.on_mouse_release
        window.on_mouse_scroll = self.on_mouse_scroll
        self.window = window

        self.keys_held = set()
        self.yaw = 0.0  # radians; 0 means looking down -Z
        self.pitch = 0.0  # radians; positive looks up

        self._joint_list = []
        self._joint_idx = 0
        self._param_idx = 0

        self.setup()

    def setup(self) -> None:
        # Derive yaw/pitch from whatever direction the renderer was initialised with.
        fwd = (self.window.cam_target - self.window.cam_eye).normalize()
        self.pitch = math.asin(max(-1.0, min(1.0, fwd.y)))
        self.yaw = math.atan2(fwd.x, -fwd.z)
        # Snapshot the starting pose so the R key can restore it later.
        self._home_eye = Vec3(
            self.window.cam_eye.x, self.window.cam_eye.y, self.window.cam_eye.z
        )
        self._home_yaw = self.yaw
        self._home_pitch = self.pitch
        self._sync_target()
        pyglet.clock.schedule_interval(self.update, 1.0 / 60.0)

    def reset(self) -> None:
        """Restore the camera to whatever pose it had when Control was created."""
        self.window.cam_eye = Vec3(self._home_eye.x, self._home_eye.y, self._home_eye.z)
        self.yaw = self._home_yaw
        self.pitch = self._home_pitch
        self._sync_target()

    # --- helpers -----------------------------------------------------------

    def _forward(self) -> Vec3:
        cp = math.cos(self.pitch)
        return Vec3(
            cp * math.sin(self.yaw), math.sin(self.pitch), -cp * math.cos(self.yaw)
        )

    def _horizontal_right(self) -> Vec3:
        # Right vector projected onto the XZ plane -- strafe stays horizontal
        # regardless of where the camera is looking.
        return Vec3(math.cos(self.yaw), 0.0, math.sin(self.yaw))

    def _sync_target(self) -> None:
        self.window.cam_target = self.window.cam_eye + self._forward()

    # --- joint poser -------------------------------------------------------

    def _ensure_joints(self) -> None:
        """Lazily collect Joint nodes from the scene graph."""
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
        """Return the increment size for the given param index."""
        for step in joint.steps:
            if isinstance(step, RotStep) and step.index == param_idx:
                return self.ROT_STEP_BIG if big else self.ROT_STEP
            elif isinstance(step, TransStep) and step.index <= param_idx < step.index + 3:
                return self.TRANS_STEP_BIG if big else self.TRANS_STEP
        return self.ROT_STEP_BIG if big else self.ROT_STEP

    def _print_joints(self) -> None:
        """Print all joint params and pipe dict to wl-copy."""
        self._ensure_joints()
        lines = []
        for j in self._joint_list:
            vals = ",".join(f"{v:.4f}" for v in j.params)
            lines.append(f'"{j.name}":[{vals}]')
        clip = "{" + ",".join(lines) + "}"
        print(clip)
        try:
            subprocess.Popen(
                ["wl-copy"], stdin=subprocess.PIPE
            ).communicate(clip.encode())
        except FileNotFoundError:
            pass

    def _show_poser_status(self) -> None:
        j = self._active_joint()
        if j is None:
            return
        idx = self._param_idx % len(j.params)
        # Determine param label
        label = f"p[{idx}]"
        for step in j.steps:
            if isinstance(step, RotStep) and step.index == idx:
                label = f"rot({step.axis}) [{idx}]"
                break
            elif isinstance(step, TransStep) and step.index <= idx < step.index + 3:
                comp = ["x", "y", "z"][idx - step.index]
                label = f"trans.{comp} [{idx}]"
                break
        val = j.params[idx]
        print(f"[poser] {j.name}  {label} = {val:.4f}  (deg={math.degrees(val):.1f})")

    # --- per-frame tick ----------------------------------------------------

    def update(self, dt: float) -> None:
        if not self.keys_held:
            return
        move = Vec3(0.0, 0.0, 0.0)
        fwd = self._forward()
        right = self._horizontal_right()
        world_up = Vec3(0.0, 1.0, 0.0)

        if key.W in self.keys_held:
            move = move + fwd
        if key.S in self.keys_held:
            move = move - fwd
        if key.D in self.keys_held:
            move = move + right
        if key.A in self.keys_held:
            move = move - right
        if key.Q in self.keys_held:
            move = move + world_up
        if key.E in self.keys_held:
            move = move - world_up

        if move.x == 0.0 and move.y == 0.0 and move.z == 0.0:
            return
        move = move.normalize() * (self.MOVE_SPEED * dt)
        self.window.cam_eye = self.window.cam_eye + move
        self._sync_target()

    # --- event handlers ----------------------------------------------------

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
        elif symbol == key.G:
            if self.window.scene is not None:
                self.window.scene.toggle_debug_axes()
        elif symbol == key.Z:
            pyglet.app.exit()
            os.execv(sys.executable, [sys.executable] + sys.argv)
        # --- Joint poser (vim keys) ---
        elif key._1 <= symbol <= key._9:
            self._ensure_joints()
            idx = symbol - key._1
            if idx < len(self._joint_list):
                self._joint_idx = idx
                self._param_idx = 0
                self._show_poser_status()
        elif symbol == key.J:
            j = self._active_joint()
            if j is not None:
                self._param_idx = (self._param_idx + 1) % len(j.params)
                self._show_poser_status()
        elif symbol == key.K:
            j = self._active_joint()
            if j is not None:
                self._param_idx = (self._param_idx - 1) % len(j.params)
                self._show_poser_status()
        elif symbol == key.H:
            j = self._active_joint()
            if j is not None:
                idx = self._param_idx % len(j.params)
                big = bool(modifiers & key.MOD_SHIFT)
                j.params[idx] -= self._step_for_param(j, idx, big)
                self._show_poser_status()
        elif symbol == key.L:
            j = self._active_joint()
            if j is not None:
                idx = self._param_idx % len(j.params)
                big = bool(modifiers & key.MOD_SHIFT)
                j.params[idx] += self._step_for_param(j, idx, big)
                self._show_poser_status()
        elif symbol == key.P:
            self._print_joints()
        elif symbol == key.BRACKETLEFT:
            if self.window.player is not None:
                self.window.animate = False
                self.window.player.seek_keyframe(-1)
                print(f"[scrub] t={self.window.player.elapsed:.2f}s")
        elif symbol == key.BRACKETRIGHT:
            if self.window.player is not None:
                self.window.animate = False
                self.window.player.seek_keyframe(1)
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
        self.pitch += dy * self.MOUSE_SENS
        lim = math.radians(89.0)
        if self.pitch > lim:
            self.pitch = lim
        if self.pitch < -lim:
            self.pitch = -lim
        self._sync_target()

    def on_mouse_scroll(self, x: int, y: int, scroll_x: float, scroll_y: float) -> None:
        self.window.cam_eye = self.window.cam_eye + self._forward() * (
            scroll_y * self.SCROLL_STEP
        )
        self._sync_target()
