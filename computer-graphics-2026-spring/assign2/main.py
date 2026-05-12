"""Entry point. One process = one scene.

    python main.py <mode> <obj_path> [--rows N --cols M] [--level L] [--ref ...]
        mode      bezier | bspline | catmull
        --rows, --cols  spline net dimensions (bezier/bspline; default 4x4)
        --level         initial CC subdivision level (catmull; default 1)
        --ref           reference .obj files shown as dim static meshes
"""
import argparse

from control import Control
from render import RenderWindow
from scene import BezierScene, BSplineScene, CatmullClarkScene


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="CG PA2 — surfaces & subdivision.")
    p.add_argument("mode", choices=["bezier", "bspline", "catmull"])
    p.add_argument("obj_path", help="cage / mesh .obj")
    p.add_argument("--rows", type=int, default=4)
    p.add_argument("--cols", type=int, default=4)
    p.add_argument("--level", type=int, default=1)
    p.add_argument("--ref", nargs="*", default=[], help="reference .obj files")
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
    scene.load_references(args.ref)
    controller = Control(renderer, scene=scene)
    center, radius = scene.center_and_radius()
    controller.frame_scene(center, radius)

    renderer.run()

