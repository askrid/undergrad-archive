"""Keyframe animation: Timeline samples sparse joint params, AnimationPlayer drives playback."""

from __future__ import annotations

from dataclasses import dataclass
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


def _back_out(t: float) -> float:
    s = 1.70158
    u = t - 1.0
    return 1.0 + u * u * ((s + 1.0) * u + s)


EASING = {
    "linear": _linear,
    "smooth": _smoothstep,
    "back_out": _back_out,
}


@dataclass
class Keyframe:
    """Sparse joint params at a time. Easing controls the curve arriving here."""

    time: float
    pose: dict[str, list[float]]
    easing: str = "smooth"


class Timeline:
    """Ordered keyframes with lerp + easing."""

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
        if not self.keyframes:
            return {}
        if t <= self.keyframes[0].time:
            return dict(self.keyframes[0].pose)
        if t >= self.keyframes[-1].time:
            return dict(self.keyframes[-1].pose)

        # Find surrounding keyframes
        prev, nxt = self.keyframes[0], self.keyframes[-1]
        for i in range(len(self.keyframes) - 1):
            if self.keyframes[i].time <= t <= self.keyframes[i + 1].time:
                prev, nxt = self.keyframes[i], self.keyframes[i + 1]
                break

        dt = nxt.time - prev.time
        alpha = EASING.get(nxt.easing, _smoothstep)(
            (t - prev.time) / dt if dt > 0 else 1.0
        )

        result: dict[str, list[float]] = {}
        for name in set(prev.pose) | set(nxt.pose):
            if name in prev.pose and name in nxt.pose:
                a, b = prev.pose[name], nxt.pose[name]
                result[name] = [a[j] + (b[j] - a[j]) * alpha for j in range(len(a))]
            else:
                result[name] = list(
                    (prev.pose if name in prev.pose else nxt.pose)[name]
                )
        return result


def _collect_joints(node: Node, out: dict[str, Joint]) -> None:
    if isinstance(node, Joint):
        out[node.name] = node
    for child in node.children:
        _collect_joints(child, out)


class AnimationPlayer:
    """Drives a Timeline by writing sampled params to Joints each frame."""

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
        self.joints: dict[str, Joint] = {}
        _collect_joints(scene.root, self.joints)
        self.elapsed = 0.0
        self._audio_path = audio_path
        self._audio_player: Any = None
        self._audio_start = audio_start

    def play_audio(self) -> None:
        if self._audio_path is None:
            return
        if self._audio_player is not None:
            self._audio_player.play()
            return
        try:
            self._audio_player = pyglet.media.load(self._audio_path).play()
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

    def _apply_pose(self, pose: dict[str, list[float]]) -> None:
        for name, values in pose.items():
            if name in self.joints:
                self.joints[name].params = values

    def update(self, dt: float) -> None:
        self.elapsed = min(self.elapsed + dt, self.timeline.duration)
        self._apply_pose(self.timeline.sample(self.elapsed))
        if self.cam_timeline is not None:
            self._apply_cam(self.cam_timeline.sample(self.elapsed))

    def seek_keyframe(self, direction: int) -> None:
        """Jump to next (+1) or previous (-1) keyframe, wrapping around."""
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
        self._apply_pose(self.timeline.sample(self.elapsed))
        if self.cam_timeline is not None:
            self._apply_cam(self.cam_timeline.sample(self.elapsed))
        self.seek_audio()
