from __future__ import annotations

from typing import Any, TYPE_CHECKING

import pyglet
from pyglet.gl import GL_TRIANGLES, GL_DEPTH_TEST, GL_CULL_FACE, glEnable, glViewport
from pyglet.math import Mat4, Vec3

import shader
from primitives import CustomGroup

if TYPE_CHECKING:
    from scene import Scene
    from animate import AnimationPlayer


class RenderWindow(pyglet.window.Window):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.batch = pyglet.graphics.Batch()

        # Camera
        self.cam_eye = Vec3(5, 5, -1)
        self.cam_target = Vec3(0, 5, 0)
        self.cam_vup = Vec3(0, 1, 0)
        self.view_mat = Mat4()

        # Projection
        self.z_near = 0.1
        self.z_far = 100
        self.fov = 80
        self.proj_mat = Mat4()

        # Point light (ambient + Lambert diffuse, 1/r^2 attenuation)
        self.light_pos = Vec3(5.0, 8.0, 5.0)
        self.light_color = Vec3(1.0, 1.0, 1.0)
        self.ambient = Vec3(0.35, 0.35, 0.35)
        self.light_const = 1.0
        self.light_linear = 0.02
        self.light_quad = 0.0

        self.shapes: list[CustomGroup] = []
        self.scene: Scene | None = None
        self.player: AnimationPlayer | None = None
        self.animate = False
        self.setup()

    def setup(self) -> None:
        self.set_minimum_size(width=400, height=300)
        self.set_mouse_visible(True)
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_CULL_FACE)

        self.view_mat = Mat4.look_at(
            self.cam_eye, target=self.cam_target, up=self.cam_vup
        )
        self.proj_mat = Mat4.perspective_projection(
            aspect=self.width / self.height,
            z_near=self.z_near, z_far=self.z_far, fov=self.fov,
        )

        # Shared shader programs (one Gouraud lit, one unlit passthrough)
        self.shader_lit = shader.create_lit_program()
        self.shader_unlit = shader.create_unlit_program()

        # Static lighting uniforms (set once)
        self.shader_lit.use()
        self.shader_lit["light_color"] = self.light_color
        self.shader_lit["ambient"] = self.ambient
        self.shader_lit["light_const"] = self.light_const
        self.shader_lit["light_linear"] = self.light_linear
        self.shader_lit["light_quad"] = self.light_quad
        self.shader_lit.stop()

    def on_draw(self) -> None:
        self.clear()
        self.batch.draw()

    def update(self, dt: float) -> None:
        self.view_mat = Mat4.look_at(
            self.cam_eye, target=self.cam_target, up=self.cam_vup
        )
        if self.player is not None and self.animate:
            self.player.update(dt)
            if self.player.elapsed >= self.player.timeline.duration:
                self.animate = False
                self.player.stop_audio()
        if self.scene is not None:
            self.scene.update_world_transforms()
        view_proj = self.proj_mat @ self.view_mat

        self.shader_unlit.use()
        self.shader_unlit["view_proj"] = view_proj
        self.shader_unlit.stop()
        self.shader_lit.use()
        self.shader_lit["view_proj"] = view_proj
        self.shader_lit["light_pos"] = self.light_pos
        self.shader_lit.stop()

    def on_resize(self, width: int, height: int):
        glViewport(0, 0, *self.get_framebuffer_size())
        self.proj_mat = Mat4.perspective_projection(
            aspect=width / height, z_near=self.z_near, z_far=self.z_far, fov=self.fov
        )
        return pyglet.event.EVENT_HANDLED

    def add_shape(
        self,
        transform: Mat4,
        vertice: list[float],
        indice: list[int],
        color: tuple[int, ...],
        normals: list[float] | None = None,
        mode: int = GL_TRIANGLES,
        lit: bool = True,
    ) -> None:
        prog = self.shader_lit if lit else self.shader_unlit
        shape = CustomGroup(transform, len(self.shapes), prog)
        n_verts = len(vertice) // 3
        if lit:
            norms = normals if normals is not None else [0.0] * (n_verts * 3)
            shape.indexed_vertices_list = prog.vertex_list_indexed(
                n_verts, mode, batch=self.batch, group=shape,
                indices=indice, vertices=("f", vertice),
                colors=("Bn", color), normals=("f", norms),
            )
        else:
            shape.indexed_vertices_list = prog.vertex_list_indexed(
                n_verts, mode, batch=self.batch, group=shape,
                indices=indice, vertices=("f", vertice), colors=("Bn", color),
            )
        self.shapes.append(shape)

    def run(self) -> None:
        pyglet.clock.schedule_interval(self.update, 1 / 60)
        pyglet.app.run()
