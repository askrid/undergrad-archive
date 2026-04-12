"""
Keyframe animation system.

Timeline holds an ordered list of Keyframes (time + sparse joint params).
AnimationPlayer drives the timeline each frame, lerping between keyframes
and writing results to Joint.params.
"""

from __future__ import annotations

import math
from dataclasses import dataclass, field
from typing import Any, TYPE_CHECKING

import pyglet

from pyglet.math import Vec3

from scene import Joint, Node

if TYPE_CHECKING:
    from render import RenderWindow
    from scene import Scene


def _linear(t: float) -> float:
    return t

def _smoothstep(t: float) -> float:
    return t * t * (3.0 - 2.0 * t)

def _ease_in(t: float) -> float:
    return t * t

def _ease_out(t: float) -> float:
    return 1.0 - (1.0 - t) * (1.0 - t)

def _elastic_out(t: float) -> float:
    if t <= 0.0:
        return 0.0
    if t >= 1.0:
        return 1.0
    return math.sin(-7.0 * math.pi / 2.0 * (t + 1.0)) * math.pow(2.0, -14.0 * t) + 1.0

def _back_out(t: float) -> float:
    s = 1.70158
    u = t - 1.0
    return 1.0 + u * u * ((s + 1.0) * u + s)

EASING = {
    "linear": _linear,
    "smooth": _smoothstep,
    "ease_in": _ease_in,
    "ease_out": _ease_out,
    "elastic_out": _elastic_out,
    "back_out": _back_out,
}

EasingFn = type(_linear)


@dataclass
class Keyframe:
    """A snapshot of joint params at a given time.

    ``pose`` maps joint names to param lists.  Only joints that change at this
    keyframe need to be present — absent joints carry forward their previous
    value during interpolation.

    ``easing`` controls the interpolation curve *arriving* at this keyframe
    (i.e. the curve used between the previous keyframe and this one).
    """

    time: float
    pose: dict[str, list[float]]
    easing: str = "ease_out"


class Timeline:
    """Ordered keyframes with lerp sampling."""

    keyframes: list[Keyframe]
    duration: float

    def __init__(
        self, keyframes: list[Keyframe], duration: float | None = None
    ) -> None:
        self.keyframes = sorted(keyframes, key=lambda k: k.time)
        self.duration = (
            duration
            if duration is not None
            else (self.keyframes[-1].time if self.keyframes else 0.0)
        )

    def sample(self, t: float) -> dict[str, list[float]]:
        """Return interpolated params for every joint present in any keyframe."""
        if not self.keyframes:
            return {}

        # Clamp
        if t <= self.keyframes[0].time:
            return dict(self.keyframes[0].pose)
        if t >= self.keyframes[-1].time:
            return dict(self.keyframes[-1].pose)

        # Find surrounding keyframes
        prev = self.keyframes[0]
        nxt = self.keyframes[-1]
        for i in range(len(self.keyframes) - 1):
            if self.keyframes[i].time <= t <= self.keyframes[i + 1].time:
                prev = self.keyframes[i]
                nxt = self.keyframes[i + 1]
                break

        dt = nxt.time - prev.time
        linear_alpha = (t - prev.time) / dt if dt > 0 else 1.0
        ease_fn = EASING.get(nxt.easing, _smoothstep)
        alpha = ease_fn(linear_alpha)

        # Collect all joint names in either keyframe
        all_names = set(prev.pose.keys()) | set(nxt.pose.keys())
        result: dict[str, list[float]] = {}

        for name in all_names:
            if name in prev.pose and name in nxt.pose:
                a = prev.pose[name]
                b = nxt.pose[name]
                result[name] = [a[j] + (b[j] - a[j]) * alpha for j in range(len(a))]
            elif name in prev.pose:
                # Only in prev — carry forward
                result[name] = list(prev.pose[name])
            else:
                # Only in nxt — snap (no previous value to lerp from)
                result[name] = list(nxt.pose[name])

        return result


def _collect_joints(node: Node, out: dict[str, Joint]) -> None:
    """Recursively collect all Joint nodes by name."""
    if isinstance(node, Joint):
        out[node.name] = node
    for child in node.children:
        _collect_joints(child, out)


class AnimationPlayer:
    """Drives a Timeline by writing sampled params to Joints each frame."""

    timeline: Timeline
    joints: dict[str, Joint]
    elapsed: float
    playing: bool
    _audio_source: Any
    _audio_player: Any
    _audio_start: float

    def __init__(
        self,
        scene: "Scene",
        timeline: Timeline,
        cam_timeline: Timeline | None = None,
        audio_path: str | None = None,
        audio_start: float = 0.0,
    ) -> None:
        self.timeline = timeline
        self.cam_timeline = cam_timeline
        self.renderer: "RenderWindow" = scene.renderer
        self.joints = {}
        _collect_joints(scene.root, self.joints)
        self.elapsed = 0.0
        self.playing = False
        self._audio_path = audio_path
        self._audio_player = None
        self._audio_start = audio_start

    def play_audio(self) -> None:
        if self._audio_path is None:
            return
        if self._audio_player is not None:
            self._audio_player.play()
            return
        try:
            source = pyglet.media.load(self._audio_path)
            self._audio_player = source.play()
            seek_to = self._audio_start + self.elapsed
            if seek_to > 0.0:
                self._audio_player.seek(seek_to)
        except Exception:
            pass

    def pause_audio(self) -> None:
        if self._audio_player is not None:
            self._audio_player.pause()

    def seek_audio(self) -> None:
        if self._audio_player is not None:
            try:
                self._audio_player.seek(self._audio_start + self.elapsed)
            except Exception:
                pass

    def stop_audio(self) -> None:
        if self._audio_player is not None:
            self._audio_player.pause()
            self._audio_player = None

    def _apply_cam(self, cam: dict[str, list[float]]) -> None:
        if "_cam_eye" in cam:
            self.renderer.cam_eye = Vec3(*cam["_cam_eye"])
        if "_cam_target" in cam:
            self.renderer.cam_target = Vec3(*cam["_cam_target"])

    def update(self, dt: float) -> None:
        self.elapsed += dt
        if self.elapsed >= self.timeline.duration:
            self.elapsed = self.timeline.duration
        pose = self.timeline.sample(self.elapsed)
        for name, values in pose.items():
            if name in self.joints:
                self.joints[name].params = values
        if self.cam_timeline is not None:
            self._apply_cam(self.cam_timeline.sample(self.elapsed))

    def play(self) -> None:
        self.playing = True

    def pause(self) -> None:
        self.playing = False

    def restart(self) -> None:
        self.elapsed = 0.0
        self.playing = True

    def seek_keyframe(self, direction: int) -> None:
        """Jump to the next (+1) or previous (-1) keyframe."""
        kfs = self.timeline.keyframes
        if not kfs:
            return
        if direction > 0:
            for kf in kfs:
                if kf.time > self.elapsed + 0.001:
                    self.elapsed = kf.time
                    break
            else:
                self.elapsed = kfs[0].time
        else:
            for kf in reversed(kfs):
                if kf.time < self.elapsed - 0.001:
                    self.elapsed = kf.time
                    break
            else:
                self.elapsed = kfs[-1].time
        pose = self.timeline.sample(self.elapsed)
        for name, values in pose.items():
            if name in self.joints:
                self.joints[name].params = values
        if self.cam_timeline is not None:
            self._apply_cam(self.cam_timeline.sample(self.elapsed))
        self.seek_audio()
