from render import RenderWindow
from scene import Scene
from control import Control
from model.glados import build_glados, build_glados_wakeup
from animate import AnimationPlayer

if __name__ == "__main__":
    renderer = RenderWindow(1920, 1080, "Portal 2", resizable=True)
    renderer.set_location(200, 200)
    controller = Control(renderer)

    scene = Scene(renderer)
    scene.root.add_child(build_glados())
    scene.register()

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
