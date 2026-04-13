from pyglet.math import Mat4

from render import RenderWindow
from primitives import Axes
from scene import Scene
from control import Control
from model.glados import build_glados, build_glados_wakeup
from animate import AnimationPlayer

if __name__ == "__main__":
    width = 1920
    height = 1080

    renderer = RenderWindow(width, height, "Portal 2", resizable=True)
    renderer.set_location(200, 200)
    controller = Control(renderer)

    # --- Reference axes ---
    axes = Axes(length=5, tick=1, tick_size=0.08)
    #    renderer.add_shape(
    #        Mat4(),
    #        axes.vertices,
    #        axes.indices,
    #        axes.colors,
    #        normals=axes.normals,
    #        mode=axes.mode,
    #        lit=axes.lit,
    #    )

    # --- Scene ---
    scene = Scene(renderer)
    scene.root.add_child(build_glados())
    scene.register()

    # --- Animation ---
    model_timeline, cam_timeline = build_glados_wakeup()
    player = AnimationPlayer(
        scene,
        model_timeline,
        cam_timeline=cam_timeline,
        audio_path="./audio/glados_wakes_up.mp3",
        audio_start=16.1,
    )
    renderer.player = player

    renderer.run()
