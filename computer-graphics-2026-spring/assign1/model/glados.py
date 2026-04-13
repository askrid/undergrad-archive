"""
Hierarchical model definitions.

Each ``build_*`` function returns the root ``Node`` of a fully-constructed
sub-tree. The caller attaches that root into a ``Scene`` wherever it wants:

    from models import build_arm
    scene.root.add_child(build_arm())
    scene.register()
"""

from __future__ import annotations

from enum import global_enum_repr
import math

from pyglet.math import Mat4, Vec3

from primitives import Ellipsoid, Torus, Frustum, Cube, Color
from scene import Node, Joint, FixedStep, RotStep, TransStep
from animate import Keyframe, Timeline

COLOR_GREY: Color = (40, 40, 40, 255)
COLOR_LITEGREY: Color = (120, 120, 120, 255)
COLOR_WHITE: Color = (240, 240, 240, 255)
COLOR_BLACK: Color = (0, 0, 0, 255)


def trans(x: float, y: float, z: float) -> Mat4:
    return Mat4.from_translation(Vec3(x, y, z))


def rot(deg: float, x: float, y: float, z: float) -> Mat4:
    return Mat4.from_rotation(math.radians(deg), Vec3(x, y, z))


def build_glados() -> Node:
    """
    Build a GLaDOS model.
    """
    ceil_base = Node("ceil_base", trans(0, 9, 0))
    joint_ceil_disc = Joint(
        "ceil_disc",
        steps=[RotStep(Vec3(0, -1, 0), 0)],
        rest_params=[0.0],
        geometry=Frustum(
            bottom_radius=1.5, top_radius=1.5, height=0.1, color=COLOR_GREY
        ),
    )
    ceil_drum = Joint(
        "ceil_drum",
        steps=[
            TransStep(0),
            FixedStep(rot(90, 0, 0, 1)),
        ],
        rest_params=[0.0, -0.6, 0.0],
        geometry=Frustum(
            bottom_radius=0.5,
            top_radius=0.5,
            height=1.2,
            theta_range=(-100, 200),
            color=COLOR_GREY,
        ),
    )
    left_arm_wire_upper1 = Node(
        "left_arm_wire_upper1",
        trans(0, -0.2, 0) @ rot(107, 0, 0, 1) @ rot(7, 1, 0, 0) @ trans(0, 1.4, 0),
        geometry=Frustum(
            bottom_radius=0.04,
            top_radius=0.04,
            height=3,
            color=COLOR_GREY,
        ),
    )
    left_arm_wire_upper2 = Node(
        "left_arm_wire_upper2",
        trans(0.12, 0, 0),
        geometry=Frustum(
            bottom_radius=0.04,
            top_radius=0.04,
            height=3,
            color=COLOR_GREY,
        ),
    )
    left_arm_wire_upper3 = Node(
        "left_arm_wire_upper3",
        trans(0, 0, -0.12),
        geometry=Frustum(
            bottom_radius=0.04,
            top_radius=0.04,
            height=3,
            color=COLOR_GREY,
        ),
    )
    left_arm_wire_upper4 = Node(
        "left_arm_wire_upper4",
        trans(0.12, 0, -0.12),
        geometry=Frustum(
            bottom_radius=0.04,
            top_radius=0.04,
            height=3,
            color=COLOR_GREY,
        ),
    )
    left_arm_cover1 = Node(
        "left_arm_cover1",
        trans(0.06, -0.08, -0.06),
        geometry=Cube(
            size=(0.3, 1.0, 0.3),
            color=COLOR_WHITE,
        ),
    )
    left_arm_cover2 = Node(
        "left_arm_cover2",
        trans(0.06, 0.95, -0.06),
        geometry=Cube(
            size=(0.3, 0.3, 0.3),
            color=COLOR_WHITE,
        ),
    )
    left_arm_cover3 = Node(
        "left_arm_cover3",
        trans(0.06, -0.5, -0.06),
        geometry=Cube(
            size=(0.3, 0.3, 0.3),
            color=COLOR_WHITE,
        ),
    )
    left_arm_cover4 = Node(
        "left_arm_cover4",
        trans(0.06, 0.66, -0.06),
        geometry=Cube(
            size=(0.4, 0.7, 0.4),
            color=COLOR_WHITE,
        ),
    )
    left_arm_wire_lower1 = Node(
        "left_arm_wire_lower1",
        trans(0, 1.5, 0) @ rot(-22, 1, 0, 0) @ rot(-24, 0, 0, 1) @ trans(0, 1, 0),
        geometry=Frustum(
            bottom_radius=0.04,
            top_radius=0.04,
            height=2,
            color=COLOR_GREY,
        ),
    )
    left_arm_wire_lower2 = Node(
        "left_arm_wire_lower2",
        trans(0.12, 0, 0),
        geometry=Frustum(
            bottom_radius=0.04,
            top_radius=0.04,
            height=2,
            color=COLOR_GREY,
        ),
    )
    left_arm_wire_lower3 = Node(
        "left_arm_wire_lower3",
        trans(0, 0, -0.12),
        geometry=Frustum(
            bottom_radius=0.04,
            top_radius=0.04,
            height=2,
            color=COLOR_GREY,
        ),
    )
    left_arm_wire_lower4 = Node(
        "left_arm_wire_lower4",
        trans(0.12, 0, -0.12),
        geometry=Frustum(
            bottom_radius=0.04,
            top_radius=0.04,
            height=2,
            color=COLOR_GREY,
        ),
    )
    ceil_drum_fill = Node(
        "ceil_drum_fill",
        rot(140, 0, 1, 0) @ trans(0, 0, -0.24),
        geometry=Cube(size=(0.6, 1.2, 0.4), color=COLOR_GREY),
    )

    joint_ceil_drum = Joint(
        "joint_ceil_drum",
        steps=[RotStep(Vec3(0, 1, 0), 0)],
        rest_params=[math.radians(-40)],
        geometry=Frustum(
            bottom_radius=0.08,
            top_radius=0.08,
            height=1.3,
            color=COLOR_GREY,
        ),
    )
    ceil_arm_center_left = Node(
        "ceil_arm_center_left",
        trans(-0.6, -0.12, 0),
        geometry=Cube(size=(1.4, 0.03, 0.2), color=COLOR_GREY),
    )
    ceil_arm_center_right = Node(
        "ceil_arm_center_left",
        trans(-0.6, 0.12, 0),
        geometry=Cube(size=(1.4, 0.03, 0.2), color=COLOR_GREY),
    )
    ceil_arm_center_bridge = Node(
        "ceil_arm_center_bridge",
        trans(-0.53, 0.12, 0),
        geometry=Frustum(
            bottom_radius=0.15,
            top_radius=0.15,
            height=0.24,
            color=COLOR_GREY,
        ),
    )
    ceil_arm_left = Node(
        "ceil_arm_left",
        trans(-0.6, -0.6, 0),
        geometry=Cube(size=(1.4, 0.05, 0.3), color=COLOR_GREY),
    )
    ceil_arm_right = Node(
        "ceil_arm_right",
        trans(-0.6, 0.6, 0),
        geometry=Cube(size=(1.4, 0.05, 0.3), color=COLOR_GREY),
    )
    ceil_arm_bridge = Node(
        "ceil_arm_bridge",
        trans(-0.67, 0.6, 0) @ rot(90, 0, 1, 0),
        geometry=Cube(size=(0.3, 1.2, 0.05), color=COLOR_GREY),
    )
    torso_connector = Node(
        "torso_connector",
        rot(90, 0, 0, 1) @ rot(40, 1, 0, 0) @ trans(0, 0, -0.42),
        geometry=Frustum(
            bottom_radius=0.55,
            top_radius=0.55,
            height=0.3,
            theta_range=(-220, 40),
            color=COLOR_LITEGREY,
        ),
    )
    torso_connector_cylinder = Node(
        "torso_connector_cylinder",
        trans(0, 0.15, 0.14),
        geometry=Frustum(
            bottom_radius=0.3,
            top_radius=0.3,
            height=0.1,
            color=COLOR_GREY,
        ),
    )
    torso_connector_fill = Node(
        "torso_connector_fill",
        trans(0, 0, -0.16),
        geometry=Cube(size=(0.83, 0.3, 0.4), color=COLOR_LITEGREY),
    )
    torso_core1 = Node(
        "torso_core1",
        rot(90, 0, 0, 1) @ trans(-0.4, 0, -1.1),
        geometry=Frustum(
            bottom_radius=1.1,
            top_radius=1.1,
            inner_bottom_radius=0.8,
            inner_top_radius=0.8,
            height=0.3,
            theta_range=(95, 290),
            color=COLOR_GREY,
        ),
    )
    torso_core_cylinder = Node(
        "torso_core1_cylinder",
        trans(0.1, 0, 0.98),
        geometry=Frustum(
            bottom_radius=0.16,
            top_radius=0.16,
            height=1.2,
            color=COLOR_GREY,
        ),
    )
    torso_inner_cover = Node(
        "torso_core1_inner_cover",
        Mat4(),
        geometry=Frustum(
            bottom_radius=0.8,
            top_radius=0.8,
            inner_bottom_radius=0.79,
            inner_top_radius=0.79,
            height=0.7,
            theta_range=(90, 289),
            color=COLOR_LITEGREY,
        ),
    )
    torso_spine = Node(
        "torso_spine",
        Mat4(),
        geometry=Frustum(
            bottom_radius=0.79,
            top_radius=0.79,
            inner_bottom_radius=0.78,
            inner_top_radius=0.78,
            height=0.1,
            theta_range=(90, 345),
            color=COLOR_GREY,
        ),
    )
    torso_spine_cyclinder1 = Node(
        "torso_spine_cyclinder1",
        trans(0.65, 0, 0.2) @ rot(90, 1, 0, 0) @ rot(15, 0, 0, 1) @ trans(0, -1.0, 0),
        geometry=Frustum(
            bottom_radius=0.05,
            top_radius=0.05,
            height=2.0,
            color=COLOR_GREY,
        ),
    )
    torso_spine_cyclinder2 = Node(
        "torso_spine_cyclinder2",
        trans(0, 0.27, 0),
        geometry=Frustum(
            bottom_radius=0.08,
            top_radius=0.08,
            height=1.5,
            color=COLOR_GREY,
        ),
    )
    torso_spine_cyclinder3 = Node(
        "torso_spine_cyclinder3",
        trans(0, 0.46, 0),
        geometry=Frustum(
            bottom_radius=0.12,
            top_radius=0.12,
            height=0.5,
            color=COLOR_GREY,
        ),
    )
    torso_frame1_left = Node(
        "torso_frame1_left",
        trans(0, 0.35, 0),
        geometry=Frustum(
            bottom_radius=1.0,
            top_radius=1.0,
            inner_bottom_radius=0.85,
            inner_top_radius=0.85,
            height=0.1,
            theta_range=(-70, 15),
            color=COLOR_GREY,
        ),
    )
    torso_frame1_right = Node(
        "torso_frame1_right",
        trans(0, -0.35, 0),
        geometry=Frustum(
            bottom_radius=1.0,
            top_radius=1.0,
            inner_bottom_radius=0.85,
            inner_top_radius=0.85,
            height=0.1,
            theta_range=(-70, 15),
            color=COLOR_GREY,
        ),
    )
    torso_frame1_bridge = Node(
        "torso_frame1_bridge",
        trans(0.54, -0.35, 0.78),
        geometry=Frustum(
            bottom_radius=0.04,
            top_radius=0.04,
            height=0.85,
            color=COLOR_GREY,
        ),
    )
    torso_frame2_left = Node(
        "torso_frame2_right",
        trans(0.9, 0.35, -1.5),
        geometry=Frustum(
            bottom_radius=1.3,
            top_radius=1.3,
            inner_bottom_radius=1.24,
            inner_top_radius=1.24,
            height=0.1,
            theta_range=(-152, -87),
            color=COLOR_GREY,
        ),
    )
    torso_frame2_right = Node(
        "torso_frame2_right",
        trans(0.9, -0.35, -1.5),
        geometry=Frustum(
            bottom_radius=1.3,
            top_radius=1.3,
            inner_bottom_radius=1.24,
            inner_top_radius=1.24,
            height=0.1,
            theta_range=(-152, -87),
            color=COLOR_GREY,
        ),
    )
    torso_frame2_bridge = Node(
        "torso_frame1_bridge",
        trans(-1.10, -0.35, 0.63),
        geometry=Frustum(
            bottom_radius=0.02,
            top_radius=0.02,
            height=0.90,
            color=COLOR_GREY,
        ),
    )
    torso_frame_left_cylinder1 = Node(
        "torso_frame_left_cylinder1",
        trans(0, 0.41, 0) @ rot(92, 0, 0, 1) @ rot(40, 1, 0, 0) @ trans(0, -0.77, 0),
        geometry=Frustum(
            bottom_radius=0.03,
            top_radius=0.03,
            height=1.6,
            color=COLOR_GREY,
        ),
    )
    torso_frame_left_cylinder2 = Node(
        "torso_frame_left_cylinder2",
        trans(0, 0.25, 0),
        geometry=Frustum(
            bottom_radius=0.05,
            top_radius=0.05,
            height=1.0,
            color=COLOR_GREY,
        ),
    )
    torso_frame_right_cylinder1 = Node(
        "torso_frame_right_cylinder1",
        trans(0, -0.41, 0) @ rot(88, 0, 0, 1) @ rot(40, 1, 0, 0) @ trans(0, -0.77, 0),
        geometry=Frustum(
            bottom_radius=0.03,
            top_radius=0.03,
            height=1.6,
            color=COLOR_GREY,
        ),
    )
    torso_frame_right_cylinder2 = Node(
        "torso_frame_right_cylinder2",
        trans(0, 0.25, 0),
        geometry=Frustum(
            bottom_radius=0.05,
            top_radius=0.05,
            height=1.0,
            color=COLOR_GREY,
        ),
    )
    torso_core2 = Node(
        "torso_core2",
        Mat4(),
        geometry=Frustum(
            bottom_radius=1.4,
            top_radius=1.4,
            inner_bottom_radius=1.1,
            inner_top_radius=1.1,
            height=0.4,
            theta_range=(100, 290),
            color=COLOR_LITEGREY,
        ),
    )
    torso_cover1 = Node(
        "torso_cover1",
        trans(-0.02, 0, 0),
        geometry=Ellipsoid(
            radius=(1.5, 1.2, 1.5),
            theta_range=(120, 240),
            phi_range=(-50, 50),
            caps=False,
            color=COLOR_WHITE,
        ),
    )
    torso_cover2 = Node(
        "torso_cover2",
        trans(-0.02, 0, 0),
        geometry=Torus(
            major_radius=1.2,
            minor_radius=0.5,
            major_range=(95, 260),
            minor_range=(-73, 73),
            caps=False,
            color=COLOR_WHITE,
        ),
    )
    torso_cover2_liner1 = Node(
        "torso_cover2_liner1",
        Mat4(),
        geometry=Frustum(
            bottom_radius=1.71,
            top_radius=1.71,
            inner_bottom_radius=1.6,
            inner_top_radius=1.6,
            height=0.13,
            theta_range=(125, 222),
            color=COLOR_BLACK,
        ),
    )
    torso_cover2_chip = Node(
        "torso_cover2_chip",
        trans(-1.48, -0.16, 0.4),
        geometry=Cube(
            size=(0.5, 0.12, 0.5),
            color=COLOR_GREY,
        ),
    )
    joint_center1 = Joint(
        "joint_center1",
        steps=[
            FixedStep(trans(-0.12, 0, -1.35)),
            RotStep(Vec3(0, -1, 0), 0),
        ],
        rest_params=[0.0],
        geometry=Frustum(
            bottom_radius=0.1, top_radius=0.1, height=0.3, color=COLOR_GREY
        ),
    )
    center_joint_connector = Node(
        "center_joint_connector",
        trans(0, 0, -0.15),
        geometry=Cube(
            size=(0.15, 0.12, 0.25),
            color=COLOR_GREY,
        ),
    )
    joint_center2 = Joint(
        "joint_center2",
        steps=[
            FixedStep(trans(-0.06, 0, -0.20) @ rot(90, 1, 0, 0) @ rot(-30, 0, 0, 1)),
            RotStep(Vec3(0, -1, 0), 0),
        ],
        rest_params=[0.0],
        geometry=Frustum(
            bottom_radius=0.05, top_radius=0.05, height=0.3, color=COLOR_GREY
        ),
    )
    chest_connector = Node(
        "chest_connector",
        trans(0, -0.15, 0),
        geometry=Frustum(
            bottom_radius=0.15, top_radius=0.15, height=0.1, color=COLOR_WHITE
        ),
    )
    chest_core = Node(
        "chest_core",
        rot(90, 1, 0, 0) @ rot(30, 0, 1, 0) @ trans(1.25, 0, -0.1),
        geometry=Frustum(
            bottom_radius=1.2,
            top_radius=1.2,
            inner_bottom_radius=0.8,
            inner_top_radius=0.8,
            height=0.05,
            theta_range=(-185, -70),
            color=COLOR_GREY,
        ),
    )
    right_arm_connector = Node(
        "right_arm_connector",
        trans(-0.2, 0.55, 0.87),
        geometry=Frustum(
            bottom_radius=0.06,
            top_radius=0.06,
            height=0.5,
            color=COLOR_WHITE,
        ),
    )
    right_arm_base = Node(
        "right_arm_base",
        trans(0, 0.30, 0) @ rot(-10, 0, 1, 0) @ trans(0, 0, 0.45),
        geometry=Cube(
            size=(0.22, 0.24, 1.1),
            color=COLOR_WHITE,
        ),
    )
    right_arm_panel = Node(
        "right_arm_panel",
        trans(0, 0.12, 0.04),
        geometry=Cube(
            size=(0.16, 0.03, 0.83),
            color=COLOR_GREY,
        ),
    )
    right_arm_inner = Node(
        "right_arm_inner",
        trans(0.052, -0.062, 0.08),
        geometry=Cube(
            size=(0.12, 0.12, 1.0),
            color=COLOR_BLACK,
        ),
    )
    right_arm_bottom = Node(
        "right_arm_bottom",
        trans(0, 0, 0.56),
        geometry=Cube(
            size=(0.22, 0.24, 0.08),
            color=COLOR_WHITE,
        ),
    )
    chest_cover1 = Node(
        "chest_cover1",
        trans(-0.02, 0, 0.02),
        geometry=Ellipsoid(
            radius=(1.2, 1.1, 1.2),
            theta_range=(188, 310),
            phi_range=(-45, 45),
            caps=False,
            color=COLOR_WHITE,
        ),
    )
    chest_cover2 = Node(
        "chest_cover2",
        trans(-0.02, 0, 0.02),
        geometry=Torus(
            major_radius=1.12,
            minor_radius=0.25,
            major_range=(180, 310),
            minor_range=(-86, 86),
            caps=False,
            color=COLOR_WHITE,
        ),
    )
    chest_panel_left = Node(
        "chest_panel_left",
        trans(0, -0.3, 0),
        geometry=Frustum(
            bottom_radius=1.15,
            top_radius=1.15,
            inner_bottom_radius=0.8,
            inner_top_radius=0.8,
            height=0.04,
            theta_range=(-110, -20),
            color=COLOR_GREY,
        ),
    )
    chest_panel_fill_left1 = Node(
        "chest_panel_fill_left1",
        rot(-65, 0, 1, 0) @ trans(0.77, 0, 0),
        geometry=Cube(
            size=(0.4, 0.04, 1.12),
            color=COLOR_GREY,
        ),
    )
    chest_panel_fill_left2 = Node(
        "chest_panel_fill_left2",
        trans(0.19, 0, -0.46),
        geometry=Cube(
            size=(0.3, 0.04, 0.7),
            color=COLOR_GREY,
        ),
    )
    chest_panel_right = Node(
        "chest_panel_right",
        trans(0, 0.3, 0),
        geometry=Frustum(
            bottom_radius=1.15,
            top_radius=1.15,
            inner_bottom_radius=0.8,
            inner_top_radius=0.8,
            height=0.04,
            theta_range=(-110, -20),
            color=COLOR_GREY,
        ),
    )
    chest_panel_fill_right1 = Node(
        "chest_panel_fill_right1",
        rot(-65, 0, 1, 0) @ trans(0.77, 0, 0),
        geometry=Cube(
            size=(0.4, 0.04, 1.12),
            color=COLOR_GREY,
        ),
    )
    chest_panel_fill_right2 = Node(
        "chest_panel_fill_right2",
        trans(0.19, 0, -0.46),
        geometry=Cube(
            size=(0.3, 0.04, 0.7),
            color=COLOR_GREY,
        ),
    )
    chest_cube = Node(
        "chest_cube",
        rot(-65, 0, 1, 0) @ trans(0.85, 0, -0.7),
        geometry=Cube(
            size=(0.45, 0.45, 0.4),
            color=COLOR_GREY,
        ),
    )
    chest_cube_cylinder1 = Node(
        "chest_cube_cylinder1",
        rot(90, 1, 0, 0) @ trans(0.22, -0.05, -0.16),
        geometry=Frustum(
            bottom_radius=0.05,
            top_radius=0.05,
            height=0.32,
            color=COLOR_GREY,
        ),
    )
    chest_cube_cylinder2 = Node(
        "chest_cube_cylinder2",
        rot(90, 1, 0, 0) @ trans(0.22, -0.05, -0.06),
        geometry=Frustum(
            bottom_radius=0.05,
            top_radius=0.05,
            height=0.32,
            color=COLOR_GREY,
        ),
    )
    joint_neck = Joint(
        "joint_neck",
        steps=[
            FixedStep(rot(-90, 1, 0, 0) @ trans(0, -0.1, 0)),
            RotStep(Vec3(0, -1, 0), 0),
        ],
        rest_params=[0.0],
        geometry=Frustum(
            bottom_radius=0.46,
            top_radius=0.46,
            inner_bottom_radius=0.34,
            inner_top_radius=0.34,
            height=0.04,
            color=COLOR_GREY,
        ),
    )
    neck_base = Node(
        "neck_base",
        trans(0.4, -0.05, 0) @ rot(-70, 0, 0, 1),
        geometry=Cube(
            size=(0.38, 0.1, 0.6),
            color=COLOR_GREY,
        ),
    )
    neck_cylinder_left1 = Node(
        "neck_cylinder_left1",
        trans(-0.1, 0.45, -0.17) @ rot(-7, 0, 0, 1) @ rot(-2, 1, 0, 0),
        geometry=Frustum(
            bottom_radius=0.02,
            top_radius=0.02,
            height=1.0,
            color=COLOR_WHITE,
        ),
    )
    neck_cylinder_left2 = Node(
        "neck_cylinder_left2",
        trans(0, -0.03, 0),
        geometry=Frustum(
            bottom_radius=0.025,
            top_radius=0.025,
            height=0.76,
            color=COLOR_GREY,
        ),
    )
    neck_cylinder_right1 = Node(
        "neck_cylinder_right1",
        trans(-0.1, 0.45, 0.17) @ rot(-7, 0, 0, 1) @ rot(2, 1, 0, 0),
        geometry=Frustum(
            bottom_radius=0.02,
            top_radius=0.02,
            height=1.0,
            color=COLOR_WHITE,
        ),
    )
    neck_cylinder_right2 = Node(
        "neck_cylinder_right2",
        trans(0, -0.03, 0),
        geometry=Frustum(
            bottom_radius=0.025,
            top_radius=0.025,
            height=0.76,
            color=COLOR_GREY,
        ),
    )
    neck_cylinder_center1 = Node(
        "neck_cylinder_center1",
        trans(0.14, 0.45, 0) @ rot(3, 0, 0, 1),
        geometry=Frustum(
            bottom_radius=0.02,
            top_radius=0.02,
            height=1.0,
            color=COLOR_WHITE,
        ),
    )
    neck_cylinder_center2 = Node(
        "neck_cylinder_center2",
        trans(0, -0.17, 0),
        geometry=Frustum(
            bottom_radius=0.025,
            top_radius=0.025,
            height=0.44,
            color=COLOR_GREY,
        ),
    )
    neck_case_bottom = Node(
        "neck_case_bottom",
        trans(0.16, -0.13, 0),
        geometry=Cube(
            size=(0.06, 0.4, 0.9),
            color=COLOR_GREY,
        ),
    )
    neck_case_left = Node(
        "neck_case_left",
        trans(-0.06, -0.13, -0.43),
        geometry=Cube(
            size=(0.5, 0.4, 0.07),
            color=COLOR_GREY,
        ),
    )
    neck_case_right = Node(
        "neck_case_right",
        trans(-0.06, -0.13, 0.43),
        geometry=Cube(
            size=(0.5, 0.4, 0.07),
            color=COLOR_GREY,
        ),
    )
    neck_cover_left = Node(
        "neck_cover_left",
        rot(135, 0, 0, 1) @ rot(90, 1, 0, 0) @ trans(-1.04, 0, -0.88),
        geometry=Ellipsoid(
            radius=(1.2, 1.1, 1.2),
            theta_range=(311, 334),
            phi_range=(-45, -25),
            caps=False,
            color=COLOR_WHITE,
        ),
    )
    neck_cover_right = Node(
        "neck_cover_right",
        rot(135, 0, 0, 1) @ rot(90, 1, 0, 0) @ trans(-1.04, 0, -0.88),
        geometry=Ellipsoid(
            radius=(1.2, 1.1, 1.2),
            theta_range=(311, 334),
            phi_range=(25, 45),
            caps=False,
            color=COLOR_WHITE,
        ),
    )
    neck_pillar_base = Node(
        "neck_pillar_base",
        trans(-0.07, 0.07, 0),
        geometry=Frustum(
            bottom_radius=0.17,
            top_radius=0.17,
            height=0.12,
            color=COLOR_GREY,
        ),
    )
    neck_pillar_left = Node(
        "neck_pillar_left",
        trans(0, 0.4, -0.10),
        geometry=Cube(
            size=(0.10, 0.7, 0.03),
            color=COLOR_GREY,
        ),
    )
    neck_pillar_right = Node(
        "neck_pillar_right",
        trans(0, 0.4, 0.10),
        geometry=Cube(
            size=(0.10, 0.7, 0.03),
            color=COLOR_GREY,
        ),
    )
    neck_pillar_center1 = Node(
        "neck_pillar_center1",
        trans(0, 0.24, 0),
        geometry=Cube(
            size=(0.04, 0.3, 0.17),
            color=COLOR_GREY,
        ),
    )
    neck_pillar_center2 = Node(
        "neck_pillar_center2",
        trans(0, 0.58, 0),
        geometry=Cube(
            size=(0.04, 0.3, 0.17),
            color=COLOR_GREY,
        ),
    )
    joint_head = Joint(
        "joint_head",
        steps=[
            FixedStep(trans(0, 0.17, 0)),
            RotStep(Vec3(1, 0, 0), 0),
            RotStep(Vec3(0, 0, 1), 1),
            RotStep(Vec3(0, 1, 0), 2),
        ],
        rest_params=[math.radians(0), math.radians(0), math.radians(0)],
        geometry=Ellipsoid(
            radius=(0.1, 0.1, 0.1),
            color=COLOR_GREY,
        ),
    )
    head_base = Node(
        "head_base",
        trans(0, 0.06, 0),
        geometry=Frustum(
            bottom_radius=0.2, top_radius=0.2, height=0.05, color=COLOR_WHITE
        ),
    )
    head_base_bridge = Node(
        "head_base_bridge",
        rot(16, 0, 0, 1) @ trans(0.39, -0.03, 0),
        geometry=Cube(
            size=(0.47, 0.06, 0.40),
            color=COLOR_WHITE,
        ),
    )
    head_core = Node(
        "head_core",
        rot(90, 1, 0, 0) @ trans(0, 0, -0.06),
        geometry=Frustum(
            bottom_radius=0.6,
            top_radius=0.6,
            inner_bottom_radius=0.2,
            inner_top_radius=0.2,
            height=0.35,
            theta_range=(5, 205),
            color=COLOR_WHITE,
        ),
    )
    head_liner1 = Node(
        "head_liner1",
        Mat4(),
        geometry=Frustum(
            bottom_radius=0.6,
            top_radius=0.6,
            inner_bottom_radius=0.55,
            inner_top_radius=0.55,
            height=0.45,
            theta_range=(10, 204.5),
            color=COLOR_GREY,
        ),
    )
    head_liner2 = Node(
        "head_liner2",
        Mat4(),
        geometry=Frustum(
            bottom_radius=0.39,
            top_radius=0.39,
            inner_bottom_radius=0.35,
            inner_top_radius=0.35,
            height=0.45,
            theta_range=(7, 204.5),
            color=COLOR_GREY,
        ),
    )
    head_ear1 = Node(
        "head_ear1",
        trans(0, 0, -0.12),
        geometry=Frustum(
            bottom_radius=0.18,
            top_radius=0.18,
            height=0.5,
            color=COLOR_GREY,
        ),
    )
    head_ear2 = Node(
        "head_ear2",
        trans(-0.26, 0, -0.03),
        geometry=Frustum(
            bottom_radius=0.06,
            top_radius=0.06,
            height=0.4,
            color=COLOR_GREY,
        ),
    )
    head_ear3 = Node(
        "head_ear3",
        trans(-0.3, 0, -0.37),
        geometry=Frustum(
            bottom_radius=0.055,
            top_radius=0.055,
            height=0.4,
            color=COLOR_GREY,
        ),
    )
    head_hairpin1 = Node(
        "head_hairpin1",
        rot(103, 0, 1, 0) @ trans(0, 0, -0.55),
        geometry=Cube(
            size=(0.05, 0.58, 0.2),
            color=COLOR_GREY,
        ),
    )
    head_hairpin2 = Node(
        "head_hairpin2",
        rot(60, 0, 1, 0) @ trans(0, 0, -0.55),
        geometry=Cube(
            size=(0.05, 0.58, 0.2),
            color=COLOR_GREY,
        ),
    )
    head_face = Node(
        "head_face",
        Mat4(),
        geometry=Frustum(
            bottom_radius=0.62,
            top_radius=0.62,
            inner_bottom_radius=0.6,
            inner_top_radius=0.6,
            height=0.42,
            theta_range=(4, 206),
            caps=False,
            color=COLOR_WHITE,
        ),
    )
    head_face_side_left = Node(
        "head_face_side_left",
        Mat4(),
        geometry=Torus(
            major_radius=0.05,
            minor_radius=0.607,
            major_range=(4, 206),
            minor_range=(-26, -20.2),
            caps=False,
            color=COLOR_WHITE,
        ),
    )
    head_face_side_right = Node(
        "head_face_side_right",
        Mat4(),
        geometry=Torus(
            major_radius=0.05,
            minor_radius=0.607,
            major_range=(4, 206),
            minor_range=(20.2, 26),
            caps=False,
            color=COLOR_WHITE,
        ),
    )
    eye_base = Node(
        "eye_base",
        trans(0, 0.03, 0) @ rot(65, 0, 1, 0),
        geometry=Frustum(
            bottom_radius=0.63,
            top_radius=0.63,
            inner_bottom_radius=0.5,
            inner_top_radius=0.5,
            height=0.30,
            theta_range=(-23, 23),
            caps=True,
            color=COLOR_BLACK,
        ),
    )
    joint_eye_core = Joint(
        "joint_eye_core",
        steps=[
            RotStep(Vec3(0, 1, 0), 0),  # param[0]: vertical aim
            TransStep(1),  # params[1,2,3]: (eye_pop, horizontal, _)
        ],
        rest_params=[0.0, 0.0, 0.0, 0.0],
        geometry=Frustum(
            bottom_radius=0.64,
            top_radius=0.64,
            inner_bottom_radius=0.5,
            inner_top_radius=0.5,
            height=0.22,
            theta_range=(-18, 18),
            caps=True,
            color=COLOR_GREY,
        ),
    )
    joint_eye_iris = Joint(
        "joint_eye_iris",
        steps=[
            FixedStep(rot(90, 0, 0, 1)),
            RotStep(Vec3(1, 0, 0), 0),  # param[0]: vertical aim
            TransStep(1),  # params[1,2,3]: (horizontal, _, _)
            FixedStep(trans(0, -0.62, 0)),
        ],
        rest_params=[0.0, 0.0, 0.0, 0.0],
        geometry=Frustum(
            bottom_radius=0.07,
            top_radius=0.07,
            height=0.05,
            color=(247, 211, 32, 255),
        ),
    )

    ceil_base.add_child(joint_ceil_disc)
    joint_ceil_disc.add_child(ceil_drum)
    ceil_drum.add_child(ceil_drum_fill)
    ceil_drum.add_child(joint_ceil_drum)
    ceil_drum.add_child(left_arm_wire_upper1)
    left_arm_wire_upper1.add_child(left_arm_wire_upper2)
    left_arm_wire_upper1.add_child(left_arm_wire_upper3)
    left_arm_wire_upper1.add_child(left_arm_wire_upper4)
    left_arm_wire_upper1.add_child(left_arm_cover1)
    left_arm_wire_upper1.add_child(left_arm_cover2)
    left_arm_wire_upper1.add_child(left_arm_wire_lower1)
    left_arm_wire_lower1.add_child(left_arm_wire_lower2)
    left_arm_wire_lower1.add_child(left_arm_wire_lower3)
    left_arm_wire_lower1.add_child(left_arm_wire_lower4)
    left_arm_wire_lower1.add_child(left_arm_cover3)
    left_arm_wire_lower1.add_child(left_arm_cover4)
    joint_ceil_drum.add_child(ceil_arm_center_left)
    joint_ceil_drum.add_child(ceil_arm_center_right)
    ceil_arm_center_left.add_child(ceil_arm_center_bridge)
    joint_ceil_drum.add_child(ceil_arm_left)
    joint_ceil_drum.add_child(ceil_arm_right)
    ceil_arm_left.add_child(ceil_arm_bridge)
    ceil_arm_bridge.add_child(torso_connector)
    torso_connector.add_child(torso_connector_fill)
    torso_connector.add_child(torso_connector_cylinder)
    torso_connector.add_child(torso_core1)
    torso_core1.add_child(torso_core2)
    torso_core1.add_child(torso_core_cylinder)
    torso_core1.add_child(torso_inner_cover)
    torso_core1.add_child(torso_spine)
    torso_spine.add_child(torso_spine_cyclinder1)
    torso_spine_cyclinder1.add_child(torso_spine_cyclinder2)
    torso_spine_cyclinder2.add_child(torso_spine_cyclinder3)
    torso_core1.add_child(torso_frame1_left)
    torso_core1.add_child(torso_frame1_right)
    torso_frame1_left.add_child(torso_frame1_bridge)
    torso_core1.add_child(torso_frame2_left)
    torso_core1.add_child(torso_frame2_right)
    torso_frame2_left.add_child(torso_frame2_bridge)
    torso_frame2_bridge.add_child(torso_frame_left_cylinder1)
    torso_frame_left_cylinder1.add_child(torso_frame_left_cylinder2)
    torso_frame2_bridge.add_child(torso_frame_right_cylinder1)
    torso_frame_right_cylinder1.add_child(torso_frame_right_cylinder2)
    torso_core2.add_child(torso_cover1)
    torso_cover1.add_child(torso_cover2)
    torso_cover2.add_child(torso_cover2_liner1)
    torso_cover2.add_child(torso_cover2_chip)
    torso_core2.add_child(joint_center1)
    joint_center1.add_child(center_joint_connector)
    center_joint_connector.add_child(joint_center2)
    joint_center2.add_child(chest_connector)
    chest_connector.add_child(chest_core)
    chest_core.add_child(right_arm_connector)
    right_arm_connector.add_child(right_arm_base)
    right_arm_base.add_child(right_arm_panel)
    right_arm_base.add_child(right_arm_inner)
    right_arm_base.add_child(right_arm_bottom)
    chest_core.add_child(chest_cover1)
    chest_cover1.add_child(chest_cover2)
    chest_core.add_child(chest_panel_left)
    chest_panel_left.add_child(chest_panel_fill_left1)
    chest_panel_fill_left1.add_child(chest_panel_fill_left2)
    chest_core.add_child(chest_panel_right)
    chest_panel_right.add_child(chest_panel_fill_right1)
    chest_panel_fill_right1.add_child(chest_panel_fill_right2)
    chest_core.add_child(chest_cube)
    chest_cube.add_child(joint_neck)
    chest_cube.add_child(chest_cube_cylinder1)
    chest_cube.add_child(chest_cube_cylinder2)
    joint_neck.add_child(neck_base)
    neck_base.add_child(neck_pillar_base)
    neck_base.add_child(neck_cylinder_left1)
    neck_cylinder_left1.add_child(neck_cylinder_left2)
    neck_base.add_child(neck_cylinder_right1)
    neck_cylinder_right1.add_child(neck_cylinder_right2)
    neck_base.add_child(neck_cylinder_center1)
    neck_cylinder_center1.add_child(neck_cylinder_center2)
    neck_base.add_child(neck_case_bottom)
    neck_base.add_child(neck_case_left)
    neck_base.add_child(neck_case_right)
    neck_base.add_child(neck_cover_left)
    neck_base.add_child(neck_cover_right)
    neck_pillar_base.add_child(neck_pillar_left)
    neck_pillar_base.add_child(neck_pillar_right)
    neck_pillar_base.add_child(neck_pillar_center1)
    neck_pillar_base.add_child(neck_pillar_center2)
    neck_pillar_center2.add_child(joint_head)
    joint_head.add_child(head_base)
    head_base.add_child(head_base_bridge)
    head_base.add_child(head_core)
    head_core.add_child(head_liner1)
    head_core.add_child(head_liner2)
    head_core.add_child(head_ear1)
    head_core.add_child(head_ear2)
    head_core.add_child(head_ear3)
    head_core.add_child(head_hairpin1)
    head_core.add_child(head_hairpin2)
    head_core.add_child(head_face)
    head_face.add_child(head_face_side_left)
    head_face.add_child(head_face_side_right)
    head_core.add_child(eye_base)
    eye_base.add_child(joint_eye_core)
    joint_eye_core.add_child(joint_eye_iris)

    return ceil_base


def build_glados_wakeup() -> tuple[Timeline, Timeline]:
    return Timeline(
        duration=60,
        keyframes=[
            Keyframe(
                0.0,
                {
                    "ceil_disc": [-0.2618],
                    "ceil_drum": [0.0000, -4.2500, 0.0000],
                    "joint_ceil_drum": [-1.1345],
                    "joint_center1": [0.4363],
                    "joint_center2": [0.5236],
                    "joint_neck": [-0.6981],
                    "joint_head": [-0.1745, 0.6981, 1.1345],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0000, 0.0000, 0.0000, 0.0000],
                },
            ),
            Keyframe(
                6.0,
                {
                    "ceil_disc": [2.7925],
                    "ceil_drum": [0.0000, -1.5000, 0.0000],
                    "joint_ceil_drum": [-0.6981],
                    "joint_center1": [0.1745],
                    "joint_center2": [-0.2618],
                    "joint_neck": [-0.0872],
                    "joint_head": [0.0000, -1.0472, 0.0000],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0000, 0.0000, 0.0000, 0.0000],
                },
            ),
            Keyframe(
                9.0,
                {
                    "ceil_disc": [2.2689],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.2618],
                    "joint_center1": [-1.8327],
                    "joint_center2": [-0.6109],
                    "joint_neck": [0.3491],
                    "joint_head": [0.0873, -0.9599, -0.0873],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0000, 0.0000, 0.0000, 0.0000],
                },
            ),
            Keyframe(
                10.5,
                {
                    "ceil_disc": [1.3090],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.8727],
                    "joint_center1": [0.2617],
                    "joint_center2": [-0.3491],
                    "joint_neck": [0.1746],
                    "joint_head": [0.0000, 0.2618, -0.3491],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0000, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                12,
                {
                    "ceil_disc": [1.3090],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.7854],
                    "joint_center1": [-0.5237],
                    "joint_center2": [-0.4364],
                    "joint_neck": [-0.3490],
                    "joint_head": [0.6109, -0.2618, 0.4363],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                14,
                {
                    "ceil_disc": [-0.5236],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6981],
                    "joint_center1": [-0.3492],
                    "joint_center2": [0.0872],
                    "joint_neck": [0.0001],
                    "joint_head": [0.0000, 0.8727, 1.5708],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                15.5,
                {
                    "ceil_disc": [-0.6109],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.7854],
                    "joint_center1": [-0.0874],
                    "joint_center2": [0.0872],
                    "joint_neck": [0.0001],
                    "joint_head": [-0.5236, -0.8726, -0.1745],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                17.0,
                {
                    "ceil_disc": [-0.4364],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.7854],
                    "joint_center1": [-0.3492],
                    "joint_center2": [0.2617],
                    "joint_neck": [-0.0872],
                    "joint_head": [0.0873, -0.2617, -0.3490],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0000, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                18.0,
                {
                    "ceil_disc": [-0.5237],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6109],
                    "joint_center1": [-0.2619],
                    "joint_center2": [-0.2619],
                    "joint_neck": [0.1746],
                    "joint_head": [-1.0472, -0.6108, 0.6982],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0000, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                18.3,
                {
                    "ceil_disc": [-0.5237],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6109],
                    "joint_center1": [-0.2619],
                    "joint_center2": [-0.2619],
                    "joint_neck": [0.1746],
                    "joint_head": [-1.0472, -0.6108, 0.6982],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0000, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                19.0,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0873],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.3491, 0.6110, 0.0873],
                    "joint_eye_core": [-0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.3491, 0.0000, 0.0000, 0.1000],
                },
                easing="back_out",
            ),
            Keyframe(
                20.0,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.3491, 0.8728, 0.0873],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                21.6,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.3491, 0.6983, 0.1746],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0873, -0.0500, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                25.5,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.4364, 0.6110, 0.0001],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0873, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                27.5,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.6109, 0.6983, -0.2617],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0873, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                29.0,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.4364, 1.1346, -0.3490],
                    "joint_eye_core": [0.0873, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                30.5,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.0001, 0.5237, 0.0873],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                30.85,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.0001, 0.5237, 0.0873],
                    "joint_eye_core": [0.0000, 0.0500, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, -0.0500, 0.1000],
                },
            ),
            Keyframe(
                31.2,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.0001, 0.5237, 0.0873],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                31.4,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.0001, 0.5237, 0.0873],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                31.6,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.6110, 0.5237, -0.0872],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.1745, 0.0000, -0.0500, 0.1000],
                },
            ),
            Keyframe(
                32.0,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.6110, 0.5237, -0.0872],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.1745, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                33.4,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [-0.0000],
                    "joint_center2": [-0.1746],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.6110, 0.5237, -0.0872],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.1745, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                34.5,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.1745],
                    "joint_center2": [0.0872],
                    "joint_neck": [0.0000],
                    "joint_head": [-0.5237, 0.6982, -0.1745],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                35.3,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.1745],
                    "joint_center2": [0.0872],
                    "joint_neck": [0.0000],
                    "joint_head": [-0.5237, 0.6982, -0.1745],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.2618, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                37.1,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [0.0872],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.9600, 0.4364, 0.0000],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.1745, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                38.2,
                {
                    "ceil_disc": [-0.0001],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [0.0872],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.9600, 0.4364, 0.0000],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.1745, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                41.52,
                {
                    "ceil_disc": [-0.0873],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1745],
                    "joint_head": [-0.9600, -0.2617, 0.0873],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0872, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                42.32,
                {
                    "ceil_disc": [-0.0873],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1745],
                    "joint_head": [-1.3091, 0.3492, -0.5236],
                    "joint_eye_core": [0.0873, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0872, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                44.62,
                {
                    "ceil_disc": [-0.0873],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1745],
                    "joint_head": [-1.3091, 0.3492, -0.5236],
                    "joint_eye_core": [0.0873, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0872, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                44.81,
                {
                    "ceil_disc": [-0.0873],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1745],
                    "joint_head": [-1.2218, 0.5237, -0.5236],
                    "joint_eye_core": [0.0873, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0872, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                45.11,
                {
                    "ceil_disc": [-0.0873],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1745],
                    "joint_head": [-1.3091, 0.3492, -0.5236],
                    "joint_eye_core": [0.0873, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0872, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                46.6,
                {
                    "ceil_disc": [-0.0873],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1745],
                    "joint_head": [-1.3091, 0.3492, -0.5236],
                    "joint_eye_core": [0.0873, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.0872, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                47.6,
                {
                    "ceil_disc": [-0.0873],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1745],
                    "joint_head": [-1.3964, 0.0874, -0.5236],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [-0.0001, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                49.5,
                {
                    "ceil_disc": [-0.0873],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1745],
                    "joint_head": [-1.3964, 0.0874, -0.5236],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [-0.0001, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                50.14,
                {
                    "ceil_disc": [-0.2715],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1603],
                    "joint_head": [-1.2264, 0.1724, -0.4599],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [-0.0001, 0.0000, 0.0000, 0.1000],
                },
                easing="linear",
            ),
            Keyframe(
                50.34,
                {
                    "ceil_disc": [-0.3305],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1558],
                    "joint_head": [-1.1719, 0.1996, -0.4395],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [-0.0001, 0.0000, -0.0500, 0.1000],
                },
                easing="linear",
            ),
            Keyframe(
                50.56,
                {
                    "ceil_disc": [-0.3929],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1510],
                    "joint_head": [-1.1143, 0.2285, -0.4178],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [-0.0001, 0.0000, 0.0000, 0.1000],
                },
                easing="linear",
            ),
            Keyframe(
                51.88,
                {
                    "ceil_disc": [-0.7766],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1215],
                    "joint_head": [-0.7601, 0.4056, -0.2850],
                    "joint_eye_core": [0.0000, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [-0.0001, 0.0000, 0.0000, 0.1000],
                },
                easing="linear",
            ),
            Keyframe(
                52.13,
                {
                    "ceil_disc": [-0.8729],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [-0.1141],
                    "joint_head": [-0.6712, 0.4500, -0.2517],
                    "joint_eye_core": [0.0873, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.3490, 0.0000, 0.0000, 0.1000],
                },
            ),
            Keyframe(
                56.0,
                {
                    "ceil_disc": [-2.3562],
                    "ceil_drum": [0.0000, -0.6000, 0.0000],
                    "joint_ceil_drum": [-0.6982],
                    "joint_center1": [0.0872],
                    "joint_center2": [-0.0001],
                    "joint_neck": [0.0000],
                    "joint_head": [0.6980, 1.1346, 0.2618],
                    "joint_eye_core": [0.0873, 0.0000, 0.0000, 0.0000],
                    "joint_eye_iris": [0.3490, 0.0000, 0.0000, 0.1000],
                },
                easing="linear",
            ),
        ],
    ), Timeline(
        duration=60,
        keyframes=[
            Keyframe(0.0, {"_cam_eye": [1.8, 0.5, 1.8], "_cam_target": [0, 1.0, 0]}),
            Keyframe(20.0, {"_cam_eye": [2.8, 2, 2.8], "_cam_target": [0, 3.0, 0]}),
            Keyframe(37.1, {"_cam_eye": [2.8, 2, 2.8], "_cam_target": [0, 3.0, 0]}),
            Keyframe(42.2, {"_cam_eye": [3.0, 3, 2.0], "_cam_target": [0, 3.0, 1.0]}),
            Keyframe(49.5, {"_cam_eye": [3.0, 3, 2.0], "_cam_target": [0, 3.0, 1.0]}),
            Keyframe(
                52.13,
                {"_cam_eye": [3.8, 3, 1.2], "_cam_target": [0, 3.0, 0.7]},
            ),
            Keyframe(56.0, {"_cam_eye": [3.6, 4, -1.5], "_cam_target": [0, 3.0, 0]}),
            Keyframe(60.0, {"_cam_eye": [3.5, 4, -2.0], "_cam_target": [5, 3.0, -5]}),
        ],
    )
