from __future__ import annotations

from dataclasses import dataclass
from typing import Optional, Union, TYPE_CHECKING

from pyglet.math import Mat4, Vec3

from primitives import Axes, Primitive, CustomGroup


@dataclass
class FixedStep:
    """Constant Mat4 (translation, fixed rotation, etc.)."""
    mat: Mat4


@dataclass
class RotStep:
    """Rotation by params[index] radians around axis."""
    axis: Vec3
    index: int


@dataclass
class TransStep:
    """Translation by Vec3(params[index], params[index+1], params[index+2])."""
    index: int


Step = Union[FixedStep, RotStep, TransStep]

if TYPE_CHECKING:
    from render import RenderWindow


class Node:
    """Scene graph node. World transform propagated each frame by Scene."""

    def __init__(
        self,
        name: str = "",
        local_transform: Optional[Mat4] = None,
        geometry: Optional[Primitive] = None,
    ) -> None:
        self.name = name
        self._local_transform = local_transform if local_transform is not None else Mat4()
        self.geometry = geometry
        self.parent: Optional[Node] = None
        self.children: list[Node] = []
        self._group: Optional[CustomGroup] = None
        self._world = Mat4()
        self._debug_group: Optional[CustomGroup] = None

    @property
    def local_transform(self) -> Mat4:
        return self._local_transform

    @local_transform.setter
    def local_transform(self, value: Mat4) -> None:
        self._local_transform = value

    def add_child(self, node: Node) -> Node:
        node.parent = self
        self.children.append(node)
        return node


class Joint(Node):
    """Node whose local_transform is computed from interleaved steps + params."""

    def __init__(
        self,
        name: str,
        steps: list[Step],
        rest_params: list[float],
        geometry: Optional[Primitive] = None,
    ) -> None:
        self.steps = steps
        self.rest_params = list(rest_params)
        self.params = list(rest_params)
        super().__init__(name, self._compute_transform(), geometry)

    def _compute_transform(self) -> Mat4:
        m = Mat4()
        for step in self.steps:
            if isinstance(step, FixedStep):
                m = m @ step.mat
            elif isinstance(step, RotStep):
                m = m @ Mat4.from_rotation(self.params[step.index], step.axis)
            elif isinstance(step, TransStep):
                i = step.index
                m = m @ Mat4.from_translation(Vec3(self.params[i], self.params[i + 1], self.params[i + 2]))
        return m

    @property
    def local_transform(self) -> Mat4:
        return self._compute_transform()

    @local_transform.setter
    def local_transform(self, value: Mat4) -> None:
        pass  # params are the source of truth

    def reset(self) -> None:
        self.params = list(self.rest_params)


class Scene:
    """Scene graph root + per-frame world transform propagation."""

    def __init__(self, renderer: "RenderWindow") -> None:
        self.renderer = renderer
        self.root = Node(name="root")
        renderer.scene = self
        self._debug_on = False
        self._debug_size = 0.6

    def register(self, node: Optional[Node] = None) -> None:
        """Walk subtree and register geometry-bearing nodes with the renderer."""
        if node is None:
            node = self.root
        if node.geometry is not None and node._group is None:
            g = node.geometry
            self.renderer.add_shape(
                node.local_transform, g.vertices, g.indices, g.colors,
                normals=getattr(g, "normals", None), mode=g.mode, lit=g.lit,
            )
            node._group = self.renderer.shapes[-1]
        if self._debug_on and node._debug_group is None and node is not self.root:
            self._attach_debug(node)
        for child in node.children:
            self.register(child)

    def _attach_debug(self, node: Node) -> None:
        a = Axes(length=self._debug_size, tick=999, tick_size=0.0)
        self.renderer.add_shape(
            node.local_transform, a.vertices, a.indices, a.colors,
            normals=a.normals, mode=a.mode, lit=a.lit,
        )
        node._debug_group = self.renderer.shapes[-1]

    def _detach_debug(self, node: Node) -> None:
        g = node._debug_group
        if g is None:
            return
        if g.indexed_vertices_list is not None:
            try:
                g.indexed_vertices_list.delete()
            except Exception:
                pass
        try:
            self.renderer.shapes.remove(g)
        except ValueError:
            pass
        node._debug_group = None

    def _walk_debug(self, node: Node, fn) -> None:
        if node is not self.root:
            fn(node)
        for child in node.children:
            self._walk_debug(child, fn)

    def enable_debug_axes(self, size: Optional[float] = None) -> None:
        if size is not None:
            self._debug_size = size
        self._debug_on = True
        self._walk_debug(self.root, lambda n: n._debug_group is None and self._attach_debug(n))

    def disable_debug_axes(self) -> None:
        self._debug_on = False
        self._walk_debug(self.root, self._detach_debug)

    def toggle_debug_axes(self) -> None:
        (self.disable_debug_axes if self._debug_on else self.enable_debug_axes)()

    def update_world_transforms(self) -> None:
        self._update(self.root, Mat4())

    def _update(self, node: Node, parent_world: Mat4) -> None:
        node._world = parent_world @ node.local_transform
        if node._group is not None:
            node._group.transform_mat = node._world
        if node._debug_group is not None:
            node._debug_group.transform_mat = node._world
        for child in node.children:
            self._update(child, node._world)
