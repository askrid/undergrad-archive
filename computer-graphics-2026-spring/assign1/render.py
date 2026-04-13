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
    """
    inherits pyglet.window.Window which is the default render window of Pyglet
    """

    batch: pyglet.graphics.Batch
    cam_eye: Vec3
    cam_target: Vec3
    cam_vup: Vec3
    view_mat: Mat4
    z_near: float
    z_far: float
    fov: float
    proj_mat: Mat4
    shapes: list[CustomGroup]
    scene: "Scene | None"
    player: "AnimationPlayer | None"
    animate: bool

    # Lighting (fixed point light, world space). Single hardcoded light --
    # ambient + Lambert diffuse only, 1/r^2-style attenuation. The learnopengl
    # "50 unit range" preset is a good match for the ~10-unit scene scale.
    light_pos: Vec3
    light_color: Vec3
    ambient: Vec3
    light_const: float
    light_linear: float
    light_quad: float

    # Shared shader programs (one lit Gouraud, one unlit passthrough). All
    # CustomGroups borrow one of these -- no per-shape compilation.
    shader_lit: Any
    shader_unlit: Any

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.batch = pyglet.graphics.Batch()
        """
        View (camera) parameters
        """
        self.cam_eye = Vec3(5, 5, -1)
        self.cam_target = Vec3(0, 5, 0)
        self.cam_vup = Vec3(0, 1, 0)
        self.view_mat = Mat4()
        """
        Projection parameters
        """
        self.z_near = 0.1
        self.z_far = 100
        self.fov = 80
        self.proj_mat = Mat4()

        # Lighting parameters (fixed point light in world space).
        self.light_pos = Vec3(5.0, 8.0, 5.0)
        self.light_color = Vec3(1.0, 1.0, 1.0)
        self.ambient = Vec3(0.35, 0.35, 0.35)
        self.light_const = 1.0
        self.light_linear = 0.02
        self.light_quad = 0.0

        self.shapes = []
        self.scene = None
        self.player = None
        self.setup()

        self.animate = False

    def setup(self) -> None:
        self.set_minimum_size(width=400, height=300)
        self.set_mouse_visible(True)
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_CULL_FACE)

        # 1. Create a view matrix
        self.view_mat = Mat4.look_at(
            self.cam_eye, target=self.cam_target, up=self.cam_vup
        )

        # 2. Create a projection matrix
        self.proj_mat = Mat4.perspective_projection(
            aspect=self.width / self.height,
            z_near=self.z_near,
            z_far=self.z_far,
            fov=self.fov,
        )

        # 3. Build shared shader programs. Static lighting uniforms that don't
        # change per frame are pushed once here.
        self.shader_lit = shader.create_lit_program()
        self.shader_unlit = shader.create_unlit_program()

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
        # Recompute view matrix each frame so camera controls (WASD/mouse) take effect.
        self.view_mat = Mat4.look_at(
            self.cam_eye, target=self.cam_target, up=self.cam_vup
        )
        # Drive animation before propagating transforms.
        if self.player is not None and self.animate:
            self.player.update(dt)
            if self.player.elapsed >= self.player.timeline.duration:
                self.animate = False
                self.player.stop_audio()
        if self.scene is not None:
            self.scene.update_world_transforms()
        view_proj = self.proj_mat @ self.view_mat

        # Push per-frame uniforms ONCE per shader program (not per shape).
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
        """
        Assign a group for each shape. ``mode`` selects the GL primitive type
        (GL_TRIANGLES by default; GL_LINES for wire-frame helpers like Axes).
        ``lit=True`` routes to the Gouraud lit shader and binds per-vertex
        normals at location 2; ``lit=False`` routes to the unlit passthrough
        shader (Axes, debug gizmos) and ignores the normals list.
        """
        prog = self.shader_lit if lit else self.shader_unlit
        shape = CustomGroup(transform, len(self.shapes), prog)
        n_verts = len(vertice) // 3
        if lit:
            norms = normals if normals is not None else [0.0] * (n_verts * 3)
            shape.indexed_vertices_list = prog.vertex_list_indexed(
                n_verts,
                mode,
                batch=self.batch,
                group=shape,
                indices=indice,
                vertices=("f", vertice),
                colors=("Bn", color),
                normals=("f", norms),
            )
        else:
            shape.indexed_vertices_list = prog.vertex_list_indexed(
                n_verts,
                mode,
                batch=self.batch,
                group=shape,
                indices=indice,
                vertices=("f", vertice),
                colors=("Bn", color),
            )
        self.shapes.append(shape)

    def run(self) -> None:
        pyglet.clock.schedule_interval(self.update, 1 / 60)
        pyglet.app.run()
