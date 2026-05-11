"""Pyglet window: batch, lighting uniforms, GL setup, shape adders."""
from __future__ import annotations

from typing import List, Sequence, Tuple

import pyglet
from pyglet.gl import *
from pyglet.math import Mat4, Vec3

from primitives import FlatGroup, LitGroup


class RenderWindow(pyglet.window.Window):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.batch = pyglet.graphics.Batch()
        self.hud_batch = pyglet.graphics.Batch()

        self.cam_eye: Vec3 = Vec3(0, 2, 6)
        self.cam_target: Vec3 = Vec3(0, 0, 0)
        self.cam_vup: Vec3 = Vec3(0, 1, 0)

        self.z_near: float = 0.1
        self.z_far: float = 200.0
        self.fov: float = 60.0
        self.view_mat: Mat4 = Mat4()
        self.proj_mat: Mat4 = Mat4()

        self.shapes: List[Tuple[object, object]] = []

        # Single point light, world space.
        self.light_pos: Vec3 = Vec3(4, 6, 6)
        self.light_color: Vec3 = Vec3(1.0, 1.0, 1.0)
        self.atten_a, self.atten_b, self.atten_c = 1.0, 0.05, 0.005

        self._setup()

    def _setup(self) -> None:
        self.set_minimum_size(width=400, height=300)
        self.set_mouse_visible(True)
        glEnable(GL_DEPTH_TEST)
        glDisable(GL_CULL_FACE)  # subdivided meshes have mixed winding
        glEnable(GL_PROGRAM_POINT_SIZE)  # shader sets gl_PointSize
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        self._update_view()
        self.proj_mat = Mat4.perspective_projection(
            aspect=self.width / self.height,
            z_near=self.z_near, z_far=self.z_far, fov=self.fov,
        )

    def _update_view(self) -> None:
        self.view_mat = Mat4.look_at(self.cam_eye, target=self.cam_target, up=self.cam_vup)

    def _push_uniforms(self) -> None:
        view_proj = self.proj_mat @ self.view_mat
        for group, _ in self.shapes:
            prog = group.shader_program
            prog.use()
            prog["view_proj"] = view_proj
            if isinstance(group, LitGroup):
                prog["light_pos"] = tuple(self.light_pos)
                prog["light_color"] = tuple(self.light_color)
                prog["eye_pos"] = tuple(self.cam_eye)
                prog["atten_a"] = self.atten_a
                prog["atten_b"] = self.atten_b
                prog["atten_c"] = self.atten_c

    def on_draw(self) -> None:
        # Push uniforms inside on_draw, not via clock.schedule_interval —
        # on macOS, the latter raced with the draw and produced stale views.
        self._update_view()
        self._push_uniforms()
        glClearColor(0.0, 0.0, 0.0, 1.0)
        self.clear()
        self.batch.draw()
        self.hud_batch.draw()

    def on_resize(self, width: int, height: int):
        glViewport(0, 0, *self.get_framebuffer_size())
        self.proj_mat = Mat4.perspective_projection(
            aspect=width / height, z_near=self.z_near, z_far=self.z_far, fov=self.fov,
        )
        return pyglet.event.EVENT_HANDLED

    # ---------- Shape adders ----------

    def _next_order(self) -> int:
        return len(self.shapes)

    def add_lines(self, transform: Mat4, vertices: Sequence[float],
                  indices: Sequence[int], colors: Sequence[int]) -> FlatGroup:
        group = FlatGroup(transform, self._next_order())
        vlist = group.shader_program.vertex_list_indexed(
            len(vertices) // 3, GL_LINES,
            batch=self.batch, group=group, indices=indices,
            vertices=("f", vertices), colors=("Bn", colors),
        )
        group.indexed_vertices_list = vlist
        self.shapes.append((group, vlist))
        return group

    def add_points(self, transform: Mat4, vertices: Sequence[float],
                   colors: Sequence[int]) -> FlatGroup:
        # Points: always on top so cage handles stay clickable.
        group = FlatGroup(transform, order=1_000_000, depth_test=False)
        vlist = group.shader_program.vertex_list(
            len(vertices) // 3, GL_POINTS,
            batch=self.batch, group=group,
            vertices=("f", vertices), colors=("Bn", colors),
        )
        group.indexed_vertices_list = vlist
        self.shapes.append((group, vlist))
        return group

    def add_mesh(self, transform: Mat4, vertices: Sequence[float],
                 indices: Sequence[int], colors: Sequence[int],
                 normals: Sequence[float]) -> LitGroup:
        group = LitGroup(transform, self._next_order())
        vlist = group.shader_program.vertex_list_indexed(
            len(vertices) // 3, GL_TRIANGLES,
            batch=self.batch, group=group, indices=indices,
            vertices=("f", vertices), colors=("Bn", colors), normals=("f", normals),
        )
        group.indexed_vertices_list = vlist
        self.shapes.append((group, vlist))
        return group

    def add_flat_triangles(self, transform: Mat4, vertices: Sequence[float],
                           indices: Sequence[int], colors: Sequence[int],
                           order: int = 500_000) -> FlatGroup:
        """Unlit triangles for overlays."""
        group = FlatGroup(transform, order)
        vlist = group.shader_program.vertex_list_indexed(
            len(vertices) // 3, GL_TRIANGLES,
            batch=self.batch, group=group, indices=indices,
            vertices=("f", vertices), colors=("Bn", colors),
        )
        group.indexed_vertices_list = vlist
        self.shapes.append((group, vlist))
        return group

    def replace_mesh_vlist(self, group: LitGroup, vertices: Sequence[float],
                           indices: Sequence[int], colors: Sequence[int],
                           normals: Sequence[float]) -> None:
        """Swap vlist on an existing group (avoids shader recompile)."""
        old = group.indexed_vertices_list
        if old is not None:
            old.delete()
        vlist = group.shader_program.vertex_list_indexed(
            len(vertices) // 3, GL_TRIANGLES,
            batch=self.batch, group=group, indices=indices,
            vertices=("f", vertices), colors=("Bn", colors), normals=("f", normals),
        )
        group.indexed_vertices_list = vlist
        for i, (g, _) in enumerate(self.shapes):
            if g is group:
                self.shapes[i] = (group, vlist)
                return
        self.shapes.append((group, vlist))

    def remove_group(self, group: object) -> None:
        for i, (g, vlist) in enumerate(self.shapes):
            if g is group:
                vlist.delete()
                self.shapes.pop(i)
                return

    def run(self) -> None:
        pyglet.app.run()
