from pyglet.math import Mat4

from render import RenderWindow
from primitives import Axes
from scene import Scene
from control import Control
from models import build_glados

if __name__ == "__main__":
    width = 1280
    height = 720

    renderer = RenderWindow(width, height, "Hello Pyglet", resizable=True)
    renderer.set_location(200, 200)

    # WASD + mouse-drag camera controls (R reset, Z restart, G gizmos,
    # ESC quit, SPACE animate).
    controller = Control(renderer)

    # --- Reference axes (not part of the scene graph; they never move) ------
    axes = Axes(length=5, tick=1, tick_size=0.08)
    renderer.add_shape(
        Mat4(),
        axes.vertices,
        axes.indices,
        axes.colors,
        normals=axes.normals,
        mode=axes.mode,
        lit=axes.lit,
    )

    # --- Scene: hierarchical models live in models.py -----------------------
    scene = Scene(renderer)
    scene.root.add_child(build_glados())
    scene.register()

    renderer.run()
