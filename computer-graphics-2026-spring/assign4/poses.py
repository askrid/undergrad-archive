"""Static GlaDOS poses.

Press P in PA1 to copy a pose dict, paste it below as a preset, render with
--pose NAME.
"""

from __future__ import annotations

from camera import Camera
from scene import Joint, Node


def _collect_joints(node: Node, out: dict) -> None:
    if isinstance(node, Joint):
        out[node.name] = node
    for c in node.children:
        _collect_joints(c, out)


def apply_pose(root: Node, pose: dict[str, list[float]]) -> None:
    """Set joint params from a pasted pose dict. Unknown joints are ignored."""
    joints: dict[str, Joint] = {}
    _collect_joints(root, joints)
    for name, params in pose.items():
        j = joints.get(name)
        if j is not None:
            j.params = list(params)


POSE_REST: dict[str, list[float]] = {}

POSE_LOOK: dict[str, list[float]] = {
    "ceil_disc": [-0.2618],
    "ceil_drum": [0.0000, -0.6000, 0.0000],
    "joint_ceil_drum": [-0.5236],
    "joint_center1": [0.1745],
    "joint_center2": [-0.3491],
    "joint_neck": [0.7854],
    "joint_head": [0.0000, 0.4363, 0.2618],
    "joint_eye_core": [0.0873, 0.0000, 0.0000, 0.0000],
    "joint_eye_iris": [0.0873, 0.0000, 0.0000, 0.0000],
}

POSE_GAZE: dict[str, list[float]] = {
    "ceil_disc": [-0.9000],
    "ceil_drum": [0.0000, -0.6000, 0.0000],
    "joint_ceil_drum": [0.4000],
    "joint_center1": [0.0872],
    "joint_center2": [-0.0001],
    "joint_neck": [0.2500],
    "joint_head": [0.5500, -0.7000, 0.1800],
    "joint_eye_core": [0.0873, 0.0000, 0.0000, 0.0000],
    "joint_eye_iris": [0.3490, 0.0000, 0.0000, 0.1000],
}

POSES: dict[str, dict[str, list[float]]] = {
    "rest": POSE_REST,
    "look": POSE_LOOK,
    "gaze": POSE_GAZE,
}

# Optional per-pose camera overrides.
CAMERAS: dict[str, Camera] = {
    "look": Camera(eye=(0.0, 5.0, 19.0), target=(0.0, 5.0, -5.0), fov=32.0),
    "gaze": Camera(eye=(2.5, 5.5, 13.0), target=(-0.5, 5.0, 0.0), fov=46.0),
}

# Yaw to turn the model's front to the camera.
YAWS: dict[str, float] = {
    "look": -3.0,
    "gaze": 180.0,
}
