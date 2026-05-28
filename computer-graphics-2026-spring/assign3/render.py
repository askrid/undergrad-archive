"""Window + shape group. Orbit camera, scene composition, frame loop."""

from __future__ import annotations

import math

import pyglet
from pyglet.gl import (
    GL_CULL_FACE,
    GL_DEPTH_TEST,
    GL_FILL,
    GL_FRONT_AND_BACK,
    GL_LINE,
    GL_TEXTURE0,
    GL_TEXTURE_2D,
    GL_TRIANGLES,
    glActiveTexture,
    glBindTexture,
    glClearColor,
    glEnable,
    glPolygonMode,
    glViewport,
)
from pyglet.math import Mat4, Vec3

import shader
from mesh import Mesh
from scene import Material, Scene, ShadeMode, Texture2D


class ShapeGroup(pyglet.graphics.Group):
    """One drawable shape: transform + material + shading mode."""

    def __init__(
        self,
        transform: Mat4,
        order: int,
        material: Material,
        mode: ShadeMode,
        window: "RenderWindow",
    ):
        super().__init__(order)
        self.program = shader.get_program()
        self.transform = transform
        self.material = material
        self.mode = mode
        self.window = window
        self.vertex_list = None

    def set_state(self) -> None:
        sp = self.program
        sp.use()
        mode = (
            self.window.mode_override
            if self.window.mode_override is not None
            else self.mode
        )
        sp["model"] = self.transform
        sp["mode"] = int(mode)
        m = self.material
        sp["Ka"] = m.ka
        sp["Kd"] = m.kd
        sp["Ks"] = m.ks
        sp["shininess"] = float(m.shininess)
        sp["rough_a"] = float(m.rough_a)
        sp["rough_b"] = float(m.rough_b)
        for unit, tex in enumerate(
            (m.base_color, m.ao, m.specular, m.roughness, m.normal)
        ):
            _bind(unit, tex)
        glPolygonMode(
            GL_FRONT_AND_BACK, GL_LINE if mode == ShadeMode.WIREFRAME else GL_FILL
        )

    def unset_state(self) -> None:
        pass


def _bind(unit: int, tex: Texture2D | None) -> None:
    glActiveTexture(GL_TEXTURE0 + unit)
    glBindTexture(GL_TEXTURE_2D, tex.handle if tex else 0)


class RenderWindow(pyglet.window.Window):
    def __init__(self, *args, scene: Scene | None = None, **kwargs):
        super().__init__(*args, **kwargs)
        self.batch = pyglet.graphics.Batch()
        self.scene = scene or Scene()

        self.cam_target = Vec3(0.0, 0.0, 0.0)
        self.cam_distance = 4.0
        self.cam_yaw = math.radians(35.0)
        self.cam_pitch = math.radians(20.0)
        self.cam_eye = Vec3(0.0, 0.0, 1.0)

        self.z_near = 0.1
        self.z_far = 200.0
        self.fov = 60.0
        self.proj_mat: Mat4 | None = None
        self.view_mat: Mat4 | None = None

        self.shapes: list[ShapeGroup] = []
        self.mode_override: ShadeMode | None = None

        self.set_minimum_size(400, 300)
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_CULL_FACE)
        glClearColor(*self.scene.background)
        self._update_camera()
        self._update_projection(self.width, self.height)

    # --- camera ---------------------------------------------------------

    def _update_camera(self) -> None:
        cp, sp_ = math.cos(self.cam_pitch), math.sin(self.cam_pitch)
        cy, sy = math.cos(self.cam_yaw), math.sin(self.cam_yaw)
        offset = Vec3(
            self.cam_distance * cp * sy,
            self.cam_distance * sp_,
            self.cam_distance * cp * cy,
        )
        self.cam_eye = self.cam_target + offset
        self.view_mat = Mat4.look_at(
            self.cam_eye, target=self.cam_target, up=Vec3(0.0, 1.0, 0.0)
        )

    def _update_projection(self, width: int, height: int) -> None:
        self.proj_mat = Mat4.perspective_projection(
            aspect=width / max(height, 1),
            z_near=self.z_near,
            z_far=self.z_far,
            fov=self.fov,
        )

    def orbit(self, dyaw: float, dpitch: float) -> None:
        self.cam_yaw += dyaw
        limit = math.pi / 2 - 0.05
        self.cam_pitch = max(-limit, min(limit, self.cam_pitch + dpitch))

    def zoom(self, factor: float) -> None:
        self.cam_distance = max(0.5, min(200.0, self.cam_distance * factor))

    def pan(self, dx: float, dy: float) -> None:
        right = Vec3(math.cos(self.cam_yaw), 0.0, -math.sin(self.cam_yaw))
        self.cam_target = self.cam_target + right * dx + Vec3(0.0, 1.0, 0.0) * dy

    # --- scene composition ----------------------------------------------

    def add_shape(
        self, transform: Mat4, mesh: Mesh, material: Material, mode: ShadeMode
    ) -> ShapeGroup:
        group = ShapeGroup(transform, len(self.shapes), material, mode, self)
        group.vertex_list = group.program.vertex_list_indexed(
            mesh.vertex_count,
            GL_TRIANGLES,
            batch=self.batch,
            group=group,
            indices=mesh.indices,
            vertices=("f", mesh.positions),
            normals=("f", mesh.normals),
            uvs=("f", mesh.uvs),
            tangents=("f", mesh.tangents),
        )
        self.shapes.append(group)
        return group

    # --- frame ----------------------------------------------------------

    def on_draw(self) -> None:
        self.clear()
        sp = shader.get_program()
        sp.use()
        sp["view_proj"] = self.proj_mat @ self.view_mat
        sp["cam_pos"] = self.cam_eye
        sp["ambient_color"] = self.scene.ambient
        sp["wire_color"] = self.scene.wire_color
        n = min(len(self.scene.lights), shader.MAX_LIGHTS)
        sp["num_lights"] = n
        for i in range(n):
            light = self.scene.lights[i]
            sp[f"lights[{i}].position"] = light.position
            sp[f"lights[{i}].color"] = light.color
            sp[f"lights[{i}].intensity"] = float(light.intensity)
        self.batch.draw()

    def update(self, dt: float) -> None:
        self._update_camera()

    def on_resize(self, width, height):
        glViewport(0, 0, *self.get_framebuffer_size())
        self._update_projection(width, height)
        return pyglet.event.EVENT_HANDLED

    def run(self) -> None:
        pyglet.clock.schedule_interval(self.update, 1 / 60)
        pyglet.app.run()
