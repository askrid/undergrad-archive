"""Entry point. One process = one scene.

    python main.py <mode> <obj_path> [--rows N --cols M] [--level L]
        mode      bezier | bspline | catmull
        --rows, --cols  spline net dimensions (bezier/bspline; default 4x4)
        --level         initial CC subdivision level (catmull; default 1)
"""
import argparse

import pyglet

from control import Control
from render import RenderWindow
from scene import BezierScene, BSplineScene, CatmullClarkScene, SceneManager


HELP_TEXT = (
    "LMB drag pt   edit point\n"
    "LMB drag bg   orbit\n"
    "RMB / MMB     pan\n"
    "scroll        zoom\n"
    "S / Shift+S   subdivide ±  (Catmull-Clark)\n"
    "F / Shift+F   reframe  /  focus selected\n"
    "V             toggle sample-vert dots\n"
    "E             export .obj  →  out/\n"
    "ESC           quit"
)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="CG PA2 — surfaces & subdivision.")
    p.add_argument("mode", choices=["bezier", "bspline", "catmull"])
    p.add_argument("obj_path", help="cage / mesh .obj")
    p.add_argument("--rows", type=int, default=4)
    p.add_argument("--cols", type=int, default=4)
    p.add_argument("--level", type=int, default=1)
    return p.parse_args()


def build_scene(window, args: argparse.Namespace):
    if args.mode == "bezier":
        return BezierScene(window, args.obj_path, args.rows, args.cols)
    if args.mode == "bspline":
        return BSplineScene(window, args.obj_path, args.rows, args.cols)
    return CatmullClarkScene(window, args.obj_path, level=args.level)


if __name__ == "__main__":
    args = parse_args()
    width, height = 1280, 800

    renderer = RenderWindow(
        width, height, f"CG PA2 — {args.mode} — {args.obj_path}", resizable=True,
    )
    renderer.set_location(120, 120)

    scene = build_scene(renderer, args)
    scene_mgr = SceneManager(renderer, scene)
    controller = Control(renderer, scene_manager=scene_mgr)
    scene_mgr.controller = controller
    scene_mgr.frame_initial()

    hud_label = pyglet.text.Label(
        HELP_TEXT,
        font_name="Menlo", font_size=14,
        x=14, y=renderer.height - 14, width=renderer.width - 28,
        multiline=True, anchor_x="left", anchor_y="top",
        color=(226, 232, 240, 230),  # slate-200, slight transparency
        batch=renderer.hud_batch,
    )

    # Re-anchor the help label to the top-left whenever the window resizes
    # (matters on retina / maximized windows where the framebuffer is much
    # bigger than the initial 1280x800).
    def _reanchor_hud(w, h):
        hud_label.x = 14
        hud_label.y = h - 14
        hud_label.width = w - 28
    renderer.push_handlers(on_resize=_reanchor_hud)

    renderer.run()
