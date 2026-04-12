from __future__ import annotations

from typing import Optional, TYPE_CHECKING

from pyglet.math import Mat4

from primitives import Axes, Primitive, CustomGroup

if TYPE_CHECKING:
    from render import RenderWindow


class Node:
    """
    A node in the scene graph.

    Holds:
      - name            : debug label
      - local_transform : Mat4 relative to its parent
      - geometry        : a Primitive (Cube, Ellipsoid, Torus, Frustum,
                          Axes, ...) or None for pure transform nodes
                          (useful for joints/pivots)
      - children        : list[Node]

    World transform is recomputed each frame by Scene.update_world_transforms()
    and written back to the node's CustomGroup so the renderer picks it up.

    This keeps the door open for a future Joint(Node) subclass that derives its
    local_transform from e.g. a hinge angle -- the traversal and renderer wiring
    don't need to change.
    """

    name: str
    local_transform: Mat4
    geometry: Optional[Primitive]
    parent: Optional["Node"]
    children: list["Node"]
    _group: Optional[CustomGroup]
    _world: Mat4
    _debug_group: Optional[CustomGroup]

    def __init__(
        self,
        name: str = "",
        local_transform: Optional[Mat4] = None,
        geometry: Optional[Primitive] = None,
    ) -> None:
        self.name = name
        self.local_transform = (
            local_transform if local_transform is not None else Mat4()
        )
        self.geometry = geometry
        self.parent = None
        self.children = []
        # Populated when registered with a Scene / updated each frame:
        self._group = None
        self._world = Mat4()
        # Optional debug gizmo (mini-axes) attached to this node's frame.
        self._debug_group = None

    def add_child(self, node: "Node") -> "Node":
        node.parent = self
        self.children.append(node)
        return node


class Scene:
    """
    Scene graph root + per-frame transform propagation.

    Usage:
        scene = Scene(renderer)
        base = Node("base", Mat4.from_translation(Vec3(0, 0, 0)))
        arm  = Node("arm",  Mat4.from_translation(Vec3(0, 1, 0)),
                    geometry=Frustum(...))
        base.add_child(arm)
        scene.root.add_child(base)
        scene.register()          # hooks all geometries into the renderer

    After registration, mutating any node's ``local_transform`` will move that
    node and its entire subtree on the next frame.
    """

    renderer: "RenderWindow"
    root: Node
    _debug_on: bool
    _debug_size: float

    def __init__(self, renderer: "RenderWindow") -> None:
        self.renderer = renderer
        self.root = Node(name="root")
        renderer.scene = self
        # Debug gizmos: a mini-axes drawn at every node's world frame.
        self._debug_on = False
        self._debug_size = 0.6

    def register(self, node: Optional[Node] = None) -> None:
        """Walk the subtree and add_shape every geometry-bearing node to the
        renderer. Safe to call multiple times -- already-registered nodes are
        skipped."""
        if node is None:
            node = self.root
        if node.geometry is not None and node._group is None:
            g = node.geometry
            self.renderer.add_shape(
                node.local_transform,
                g.vertices,
                g.indices,
                g.colors,
                normals=getattr(g, "normals", None),
                mode=g.mode,
                lit=g.lit,
            )
            node._group = self.renderer.shapes[-1]
        if self._debug_on and node._debug_group is None and node is not self.root:
            self._attach_debug(node)
        for child in node.children:
            self.register(child)

    # --- debug gizmos -----------------------------------------------------

    def _attach_debug(self, node: Node) -> None:
        a = Axes(length=self._debug_size, tick=999, tick_size=0.0)
        self.renderer.add_shape(
            node.local_transform,
            a.vertices,
            a.indices,
            a.colors,
            normals=a.normals,
            mode=a.mode,
            lit=a.lit,
        )
        node._debug_group = self.renderer.shapes[-1]

    def _detach_debug(self, node: Node) -> None:
        g = node._debug_group
        if g is None:
            return
        vlist = g.indexed_vertices_list
        if vlist is not None:
            try:
                vlist.delete()
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
        self._walk_debug(
            self.root, lambda n: n._debug_group is None and self._attach_debug(n)
        )

    def disable_debug_axes(self) -> None:
        self._debug_on = False
        self._walk_debug(self.root, self._detach_debug)

    def toggle_debug_axes(self) -> None:
        if self._debug_on:
            self.disable_debug_axes()
        else:
            self.enable_debug_axes()

    def update_world_transforms(self) -> None:
        """DFS from the root, composing parent._world @ node.local_transform.
        Called by RenderWindow.update() before the shader uniforms are set."""
        self._update(self.root, Mat4())

    def _update(self, node: Node, parent_world: Mat4) -> None:
        node._world = parent_world @ node.local_transform
        if node._group is not None:
            node._group.transform_mat = node._world
        if node._debug_group is not None:
            node._debug_group.transform_mat = node._world
        for child in node.children:
            self._update(child, node._world)
