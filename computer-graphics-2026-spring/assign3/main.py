"""Compose the scene.

Adding a model:
    mesh = load_obj("path/to/model.obj")
    material = Material(kd=..., base_color=load_texture(...), ...)
    window.add_shape(transform, mesh, material, mode)

Keys: 1..5 force mode, 0 restores per-shape, R reset camera,
Space toggle spin, mouse to orbit/pan, scroll to zoom, Esc quits.
"""

from __future__ import annotations

import os

from pyglet.math import Mat4, Vec3

from control import Control
from mesh import load_obj
from render import RenderWindow
from scene import Light, Material, Scene, ShadeMode, load_texture

HERE = os.path.dirname(os.path.abspath(__file__))


def rock_material(rock_dir: str) -> Material:
    tex = os.path.join(rock_dir, "Free_rock_tex")
    return Material(
        ka=Vec3(0.1, 0.1, 0.1),
        kd=Vec3(0.5, 0.5, 0.5),
        ks=Vec3(0.8, 0.8, 0.8),
        shininess=8.0,
        rough_a=200.0,
        rough_b=0.05,
        base_color=load_texture(os.path.join(tex, "Free_rock_Base_Color.jpg")),
        ao=load_texture(os.path.join(tex, "Free_rock_Mixed_AO.jpg")),
        specular=load_texture(os.path.join(tex, "Free_rock_Specular.jpg")),
        roughness=load_texture(os.path.join(tex, "Free_rock_Roughness.jpg")),
        normal=load_texture(os.path.join(tex, "Free_rock_Normal_OpenGL.jpg")),
    )


def main() -> None:
    scene = Scene(
        lights=[
            Light(Vec3(4.0, 5.0, 4.0), Vec3(1.0, 0.95, 0.85), 1.2),
            Light(Vec3(-5.0, 3.0, -2.0), Vec3(0.4, 0.5, 0.9), 0.6),
            Light(Vec3(0.0, -2.0, 4.0), Vec3(0.9, 0.5, 0.3), 0.3),
        ],
        ambient=Vec3(0.6, 0.65, 0.75),
        background=(0.04, 0.05, 0.08, 1.0),
        wire_color=Vec3(0.85, 0.9, 1.0),
    )

    window = RenderWindow(1280, 720, "CG PA3", resizable=True, scene=scene)
    window.set_location(120, 120)
    Control(window)

    rock_dir = os.path.join(HERE, "models", "rock")
    rock_mesh = load_obj(os.path.join(rock_dir, "Free_rock.obj"))
    rock_mat = rock_material(rock_dir)

    window.add_shape(
        Mat4.from_translation(Vec3(0.0, 0.0, 0.0)),
        rock_mesh,
        rock_mat,
        ShadeMode.PHONG_NORMAL,
    )

    window.run()


if __name__ == "__main__":
    main()
