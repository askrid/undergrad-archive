"""Pyglet group wrappers that bind a shader program + model uniform."""
from __future__ import annotations

from typing import Optional

import pyglet
from pyglet.gl import *
from pyglet.math import Mat4

import shader


class FlatGroup(pyglet.graphics.Group):
    """Flat (color-only) group. Used for cage lines and cage points."""

    def __init__(self, transform_mat: Mat4, order: int, depth_test: bool = True) -> None:
        super().__init__(order)
        self.shader_program = shader.create_program(
            shader.vertex_source_default, shader.fragment_source_default,
        )
        self.transform_mat: Mat4 = transform_mat
        self.indexed_vertices_list: Optional[object] = None
        self.depth_test: bool = depth_test

    def set_state(self) -> None:
        self.shader_program.use()
        self.shader_program["model"] = self.transform_mat
        if not self.depth_test:
            glDisable(GL_DEPTH_TEST)

    def unset_state(self) -> None:
        if not self.depth_test:
            glEnable(GL_DEPTH_TEST)
        self.shader_program.stop()


class LitGroup(pyglet.graphics.Group):
    """Lit surface group: per-vertex normals + Phong shading."""

    def __init__(self, transform_mat: Mat4, order: int) -> None:
        super().__init__(order)
        self.shader_program = shader.create_program(
            shader.vertex_source_phong, shader.fragment_source_phong,
        )
        self.transform_mat: Mat4 = transform_mat
        self.indexed_vertices_list: Optional[object] = None
        # Material defaults, in case anything wants to override per-shape.
        self.k_a, self.k_d, self.k_s = 0.2, 0.8, 0.4
        self.shininess: float = 32.0

    def set_state(self) -> None:
        p = self.shader_program
        p.use()
        p["model"] = self.transform_mat
        p["k_a"], p["k_d"], p["k_s"] = self.k_a, self.k_d, self.k_s
        p["shininess"] = self.shininess

    def unset_state(self) -> None:
        self.shader_program.stop()
