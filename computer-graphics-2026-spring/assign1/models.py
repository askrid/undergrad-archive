"""
Hierarchical model definitions.

Each ``build_*`` function returns the root ``Node`` of a fully-constructed
sub-tree. The caller attaches that root into a ``Scene`` wherever it wants:

    from models import build_arm
    scene.root.add_child(build_arm())
    scene.register()
"""

from __future__ import annotations

import math

from pyglet.math import Mat4, Vec3

from primitives import Ellipsoid, Torus, Frustum, Cube, Color
from scene import Node

COLOR_GREY: Color = (50, 50, 50, 255)
COLOR_WHITE: Color = (240, 240, 240, 255)


def build_glados() -> Node:
    """
    Build a GLaDOS model.
    """
    ceil_base = Node("ceil_base", Mat4.from_translation(Vec3(0, 7, 0)))
    joint_ceil_disc = Node(
        "ceil_disc",
        Mat4.from_rotation(math.radians(0), Vec3(0, 1, 0)), # joint
        geometry=Frustum(bottom_radius=1.5, top_radius=1.5, height=0.1, color=COLOR_GREY),
    )
    ceil_drum = Node(
        "ceil_drum",
        Mat4.from_translation(Vec3(0, -0.6, 0))
        @ Mat4.from_rotation(math.radians(90), Vec3(0, 0, 1)),
        geometry=Frustum(
            bottom_radius=0.5,
            top_radius=0.5,
            height=1.2,
            theta_range=(-100, 200),
            color=COLOR_GREY,
        ),
    )
    ceil_drum_fill = Node(
        "ceil_drum_fill",
        Mat4.from_rotation(math.radians(140), Vec3(0, 1, 0))
        @ Mat4.from_translation(Vec3(0, 0, -0.24)),
        geometry=Cube(size=(0.6, 1.2, 0.4), color=COLOR_GREY),
    )

    joint_torso = Node(
        "joint_torso",
        Mat4.from_rotation(math.radians(-40), Vec3(0, 1, 0)),  # joint
        geometry=Frustum(
            bottom_radius=0.08,
            top_radius=0.08,
            height=1.3,
        ),
    )
    torso_arm_center_left = Node(
        "torso_arm_center_left",
        Mat4.from_translation(Vec3(-0.6, -0.12, 0)),
        geometry=Cube(size=(1.4, 0.03, 0.2), color=COLOR_GREY),
    )
    torso_arm_center_right = Node(
        "torso_arm_center_left",
        Mat4.from_translation(Vec3(-0.6, 0.12, 0)),
        geometry=Cube(size=(1.4, 0.03, 0.2), color=COLOR_GREY),
    )
    torso_arm_center_bridge = Node(
        "torso_arm_center_bridge",
        Mat4.from_translation(Vec3(-0.53, 0.12, 0)),
        geometry=Frustum(
            bottom_radius=0.15,
            top_radius=0.15,
            height=0.24,
            color=COLOR_GREY,
        ),
    )
    torso_arm_left = Node(
        "torso_arm_left",
        Mat4.from_translation(Vec3(-0.6, -0.6, 0)),
        geometry=Cube(size=(1.4, 0.05, 0.3), color=COLOR_GREY),
    )
    torso_arm_right = Node(
        "torso_arm_right",
        Mat4.from_translation(Vec3(-0.6, 0.6, 0)),
        geometry=Cube(size=(1.4, 0.05, 0.3), color=COLOR_GREY),
    )
    torso_arm_bridge = Node(
        "torso_arm_bridge",
        Mat4.from_translation(Vec3(-0.67, 0.6, 0))
        @ Mat4.from_rotation(math.radians(90), Vec3(0, 1, 0)),
        geometry=Cube(size=(0.3, 1.2, 0.05), color=COLOR_GREY),
    )
    torso_connector = Node(
        "torso_connector",
        Mat4.from_rotation(math.radians(90), Vec3(0, 0, 1))
        @ Mat4.from_rotation(math.radians(40), Vec3(1, 0, 0))
        @ Mat4.from_translation(Vec3(0, 0, -0.42)),
        geometry=Frustum(
            bottom_radius=0.55,
            top_radius=0.55,
            height=0.3,
            theta_range=(-220, 40),
            color=COLOR_WHITE,
        ),
    )
    torso_connector_cylinder = Node(
        "torso_connector_cylinder",
        Mat4.from_translation(Vec3(0, 0.15, 0.14)),
        geometry=Frustum(
            bottom_radius=0.3,
            top_radius=0.3,
            height=0.1,
            color=COLOR_GREY,
        ),
    )
    torso_connector_fill = Node(
        "torso_connector_fill",
        Mat4.from_translation(Vec3(0, 0, -0.16)),
        geometry=Cube(size=(0.83, 0.3, 0.4), color=COLOR_WHITE),
    )
    torso_core1 = Node(
        "torso_core1",
        Mat4.from_rotation(math.radians(90), Vec3(0, 0, 1))
        @ Mat4.from_translation(Vec3(-0.4, 0, -1.1)),
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
    torso_core1_cylinder = Node(
        "torso_core1_cylinder",
        Mat4.from_translation(Vec3(0.1, 0, 0.98)),
        geometry=Frustum(
            bottom_radius=0.16,
            top_radius=0.16,
            height=1.2,
            color=COLOR_WHITE,
        ),
    )
    torso_core1_inner_cover = Node(
        "torso_core1_inner_cover",
        Mat4(),
        geometry=Frustum(
            bottom_radius=0.8,
            top_radius=0.8,
            inner_bottom_radius=0.79,
            inner_top_radius=0.79,
            height=0.7,
            theta_range=(90, 289),
            color=COLOR_WHITE,
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
            color=COLOR_WHITE,
        ),
    )
    torso_cover1 = Node(
        "torso_cover1",
        Mat4.from_translation(Vec3(-0.02, 0, 0)),
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
        Mat4.from_translation(Vec3(-0.02, 0, 0)),
        geometry=Torus(
            major_radius=1.2,
            minor_radius=0.5,
            major_range=(95, 260),
            minor_range=(-73, 73),
            caps=False,
            color=COLOR_WHITE,
        ),
    )
    joint_chest1 = Node(
        "joint_chest1",
        Mat4.from_translation(Vec3(-0.12, 0, -1.35))
        @ Mat4.from_rotation(math.radians(0), Vec3(0, 1, 0)),  # joint
        geometry=Frustum(bottom_radius=0.1, top_radius=0.1, height=0.3),
    )
    chest_joint_connector = Node(
        "chest_joint_connector",
        Mat4.from_translation(Vec3(0, 0, -0.15)),
        geometry=Cube(
            size=(0.15, 0.12, 0.25),
        ),
    )
    joint_chest2 = Node(
        "joint_chest2",
        Mat4.from_translation(Vec3(-0.06, 0, -0.20))
        @ Mat4.from_rotation(math.radians(90), Vec3(1, 0, 0))
        @ Mat4.from_rotation(math.radians(-30), Vec3(0, 0, 1))
        @ Mat4.from_rotation(math.radians(0), Vec3(0, 1, 0)),  # joint
        geometry=Frustum(bottom_radius=0.05, top_radius=0.05, height=0.3),
    )
    chest_connector = Node(
        "chest_connector",
        Mat4.from_translation(Vec3(0, -0.15, 0)),
        geometry=Frustum(
            bottom_radius=0.15, top_radius=0.15, height=0.1, color=COLOR_WHITE
        ),
    )
    chest_core = Node(
        "chest_core",
        Mat4.from_rotation(math.radians(90), Vec3(1, 0, 0))
        @ Mat4.from_rotation(math.radians(30), Vec3(0, 1, 0))
        @ Mat4.from_translation(Vec3(1.25, 0, -0.1)),
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
    chest_cover1 = Node(
        "chest_cover1",
        Mat4.from_translation(Vec3(-0.02, 0, 0.02)),
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
        Mat4.from_translation(Vec3(-0.02, 0, 0.02)),
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
        Mat4.from_translation(Vec3(0, -0.3, 0)),
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
        Mat4.from_rotation(math.radians(-65), Vec3(0, 1, 0))
        @ Mat4.from_translation(Vec3(0.77, 0, 0)),
        geometry=Cube(
            size=(0.4, 0.04, 1.12),
            color=COLOR_GREY,
        ),
    )
    chest_panel_fill_left2 = Node(
        "chest_panel_fill_left2",
        Mat4.from_translation(Vec3(0.19, 0, -0.46)),
        geometry=Cube(
            size=(0.3, 0.04, 0.7),
            color=COLOR_GREY,
        ),
    )
    chest_panel_right = Node(
        "chest_panel_right",
        Mat4.from_translation(Vec3(0, 0.3, 0)),
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
        Mat4.from_rotation(math.radians(-65), Vec3(0, 1, 0))
        @ Mat4.from_translation(Vec3(0.77, 0, 0)),
        geometry=Cube(
            size=(0.4, 0.04, 1.12),
            color=COLOR_GREY,
        ),
    )
    chest_panel_fill_right2 = Node(
        "chest_panel_fill_right2",
        Mat4.from_translation(Vec3(0.19, 0, -0.46)),
        geometry=Cube(
            size=(0.3, 0.04, 0.7),
            color=COLOR_GREY,
        ),
    )
    chest_cube = Node(
        "chest_cube",
        Mat4.from_rotation(math.radians(-65), Vec3(0, 1, 0))
        @ Mat4.from_translation(Vec3(0.85, 0, -0.7)),
        geometry=Cube(
            size=(0.45, 0.45, 0.4),
        ),
    )
    joint_neck = Node(
        "joint_neck",
        Mat4.from_rotation(math.radians(-90), Vec3(1, 0, 0))
        @ Mat4.from_translation(Vec3(0, -0.1, 0))
        @ Mat4.from_rotation(
            math.radians(0), Vec3(0, 1, 0)
        ),  # joint (neck rotate around body)
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
        Mat4.from_translation(Vec3(0.4, -0.05, 0))
        @ Mat4.from_rotation(math.radians(-70), Vec3(0, 0, 1)),
        geometry=Cube(
            size=(0.4, 0.1, 0.6),
        ),
    )
    neck_case_bottom = Node(
        "neck_case_bottom",
        Mat4.from_translation(Vec3(0.16, -0.13, 0)),
        geometry=Cube(
            size=(0.06, 0.4, 0.9),
        ),
    )
    neck_case_left = Node(
        "neck_case_left",
        Mat4.from_translation(Vec3(-0.06, -0.13, -0.43)),
        geometry=Cube(
            size=(0.5, 0.4, 0.07),
        ),
    )
    neck_case_right = Node(
        "neck_case_right",
        Mat4.from_translation(Vec3(-0.06, -0.13, 0.43)),
        geometry=Cube(
            size=(0.5, 0.4, 0.07),
        ),
    )
    neck_cover_left = Node(
        "neck_cover_left",
        Mat4.from_rotation(math.radians(135), Vec3(0, 0, 1))
        @ Mat4.from_rotation(math.radians(90), Vec3(1, 0, 0))
        @ Mat4.from_translation(Vec3(-1.04, 0, -0.88)),
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
        Mat4.from_rotation(math.radians(135), Vec3(0, 0, 1))
        @ Mat4.from_rotation(math.radians(90), Vec3(1, 0, 0))
        @ Mat4.from_translation(Vec3(-1.04, 0, -0.88)),
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
        Mat4.from_translation(Vec3(-0.07, 0.07, 0)),
        geometry=Frustum(
            bottom_radius=0.17,
            top_radius=0.17,
            height=0.12,
            color=COLOR_WHITE,
        ),
    )
    neck_pillar = Node(
        "neck_pillar",
        Mat4.from_translation(Vec3(0, 0.3, 0)),
        geometry=Cube(
            size=(0.10, 0.6, 0.2),
        ),
    )
    joint_head = Node(
        "joint_head",
        Mat4.from_translation(Vec3(0, 0.35, 0))
        @ Mat4.from_rotation(math.radians(-40), Vec3(1, 0, 0))  # joint
        @ Mat4.from_rotation(math.radians(25), Vec3(0, 0, 1))  # joint
        @ Mat4.from_rotation(math.radians(0), Vec3(0, 1, 0)),  # joint
        geometry=Ellipsoid(
            radius=(0.1, 0.1, 0.1),
        ),
    )
    head_base = Node(
        "head_base",
        Mat4.from_translation(Vec3(0, 0.06, 0)),
        geometry=Frustum(
            bottom_radius=0.2, top_radius=0.2, height=0.05, color=COLOR_WHITE
        ),
    )
    head_base_bridge = Node(
        "head_base_bridge",
        Mat4.from_rotation(math.radians(16), Vec3(0, 0, 1))
        @ Mat4.from_translation(Vec3(0.39, -0.03, 0)),
        geometry=Cube(
            size=(0.47, 0.06, 0.40),
            color=COLOR_WHITE,
        ),
    )
    head_core = Node(
        "head_core",
        Mat4.from_rotation(math.radians(90), Vec3(1, 0, 0))
        @ Mat4.from_translation(Vec3(0, 0, -0.06)),
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
            theta_range=(5, 205),
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
            theta_range=(5, 205),
            color=COLOR_GREY,
        ),
    )
    head_ear1 = Node(
        "head_ear1",
        Mat4.from_translation(Vec3(0, 0, -0.12)),
        geometry=Frustum(
            bottom_radius=0.18,
            top_radius=0.18,
            height=0.5,
            color=COLOR_GREY,
        ),
    )
    head_ear2 = Node(
        "head_ear2",
        Mat4.from_translation(Vec3(-0.26, 0, -0.03)),
        geometry=Frustum(
            bottom_radius=0.06,
            top_radius=0.06,
            height=0.4,
            color=COLOR_GREY,
        ),
    )
    head_ear3 = Node(
        "head_ear3",
        Mat4.from_translation(Vec3(-0.3, 0, -0.37)),
        geometry=Frustum(
            bottom_radius=0.055,
            top_radius=0.055,
            height=0.4,
            color=COLOR_GREY,
        ),
    )
    head_hairpin1 = Node(
        "head_hairpin1",
        Mat4.from_rotation(math.radians(103), Vec3(0, 1, 0))
        @ Mat4.from_translation(Vec3(0, 0, -0.55)),
        geometry=Cube(
            size=(0.05, 0.58, 0.2),
            color=COLOR_GREY,
        ),
    )
    head_hairpin2 = Node(
        "head_hairpin2",
        Mat4.from_rotation(math.radians(60), Vec3(0, 1, 0))
        @ Mat4.from_translation(Vec3(0, 0, -0.55)),
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
    head_eye_base = Node(
        "head_eye_base",
        Mat4.from_translation(Vec3(0, 0.03, 0))
        @ Mat4.from_rotation(math.radians(65), Vec3(0, 1, 0)),
        geometry=Frustum(
            bottom_radius=0.63,
            top_radius=0.63,
            inner_bottom_radius=0.5,
            inner_top_radius=0.5,
            height=0.30,
            theta_range=(-23, 23),
            caps=True,
            color=(0, 0, 0, 255),
        ),
    )
    head_eye_core = Node(
        "head_eye_core",
        Mat4.from_rotation(math.radians(0), Vec3(0, 1, 0))  # joint (vertical)
        @ Mat4.from_translation(
            Vec3(0.00, 0.00, 0)
        ),  # joint (eye pop, horizontal, none)
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
    head_eye_iris = Node(
        "head_eye_iris",
        Mat4.from_rotation(math.radians(90), Vec3(0, 0, 1))
        @ Mat4.from_rotation(math.radians(0), Vec3(1, 0, 0))  # joint (vertical)
        @ Mat4.from_translation(Vec3(0.00, 0, 0))  # joint (horizontal, none, none)
        @ Mat4.from_translation(Vec3(0, -0.62, 0)),
        geometry=Frustum(
            bottom_radius=0.08,
            top_radius=0.08,
            height=0.05,
            color=(242, 255, 97, 255),
        ),
    )

    ceil_base.add_child(joint_ceil_disc)
    joint_ceil_disc.add_child(ceil_drum)
    ceil_drum.add_child(ceil_drum_fill)
    ceil_drum.add_child(joint_torso)
    joint_torso.add_child(torso_arm_center_left)
    joint_torso.add_child(torso_arm_center_right)
    torso_arm_center_left.add_child(torso_arm_center_bridge)
    joint_torso.add_child(torso_arm_left)
    joint_torso.add_child(torso_arm_right)
    torso_arm_left.add_child(torso_arm_bridge)
    torso_arm_bridge.add_child(torso_connector)
    torso_connector.add_child(torso_connector_fill)
    torso_connector.add_child(torso_connector_cylinder)
    torso_connector.add_child(torso_core1)
    torso_core1.add_child(torso_core2)
    torso_core1.add_child(torso_core1_cylinder)
    torso_core1.add_child(torso_core1_inner_cover)
    torso_core2.add_child(torso_cover1)
    torso_cover1.add_child(torso_cover2)
    torso_core2.add_child(joint_chest1)
    joint_chest1.add_child(chest_joint_connector)
    chest_joint_connector.add_child(joint_chest2)
    joint_chest2.add_child(chest_connector)
    chest_connector.add_child(chest_core)
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
    joint_neck.add_child(neck_base)
    neck_base.add_child(neck_pillar_base)
    neck_base.add_child(neck_case_bottom)
    neck_base.add_child(neck_case_left)
    neck_base.add_child(neck_case_right)
    neck_base.add_child(neck_cover_left)
    neck_base.add_child(neck_cover_right)
    neck_pillar_base.add_child(neck_pillar)
    neck_pillar.add_child(joint_head)
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
    head_core.add_child(head_eye_base)
    head_eye_base.add_child(head_eye_core)
    head_eye_core.add_child(head_eye_iris)

    return ceil_base
