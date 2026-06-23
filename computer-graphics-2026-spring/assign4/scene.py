"""Scene-graph nodes from PA1"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional, Union

from pyglet.math import Mat4, Vec3

from primitives import Primitive


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


class Node:
    """Scene graph node. World transform = parent_world @ local_transform."""

    def __init__(
        self,
        name: str = "",
        local_transform: Optional[Mat4] = None,
        geometry: Optional[Primitive] = None,
    ) -> None:
        self.name = name
        self._local_transform = (
            local_transform if local_transform is not None else Mat4()
        )
        self.geometry = geometry
        self.parent: Optional[Node] = None
        self.children: list[Node] = []

    @property
    def local_transform(self) -> Mat4:
        return self._local_transform

    @local_transform.setter
    def local_transform(self, value: Mat4) -> None:
        self._local_transform = value

    def add_child(self, node: "Node") -> "Node":
        node.parent = self
        self.children.append(node)
        return node


class Joint(Node):
    """Node whose local_transform is recomputed from steps + live params."""

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
                m = m @ Mat4.from_translation(
                    Vec3(self.params[i], self.params[i + 1], self.params[i + 2])
                )
        return m

    @property
    def local_transform(self) -> Mat4:
        return self._compute_transform()

    @local_transform.setter
    def local_transform(self, value: Mat4) -> None:
        pass
