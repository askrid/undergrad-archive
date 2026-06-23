"""Render driver. Builds the scene, traces it, writes a PNG. See README.md."""

from __future__ import annotations

import argparse
import time

import torch

from model.glados import build_glados
from flatten import build_scene
from raytracer import SceneGPU
from camera import Camera
from poses import apply_pose, POSES, CAMERAS, YAWS
from image import save_png

DEFAULT_CAM = Camera(eye=(0.0, 5.2, 13.5), target=(0.0, 5.0, 0.0), fov=46.0)


def make_scene(pose_name: str, light_grid: int, spheres: bool, yaw):
    root = build_glados()
    pose = POSES.get(pose_name, {})
    apply_pose(root, pose)
    if yaw is None:
        yaw = YAWS.get(pose_name, 0.0)
    scene = build_scene(root, light_grid=light_grid, add_spheres=spheres, model_yaw=yaw)
    cam = CAMERAS.get(pose_name, DEFAULT_CAM)
    return scene, cam


def render_once(args, gpu, cam, depth, spp, out, label=""):
    t0 = time.time()
    img = gpu.render(
        cam, args.width, args.height, spp=spp, max_depth=depth, seed=args.seed
    )
    torch.cuda.synchronize()
    dt = time.time() - t0
    save_png(img, out, exposure=args.exposure)
    print(
        f"  {label}{out}  ({args.width}x{args.height} spp={spp} depth={depth})  {dt:.1f}s"
    )
    return img


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--width", type=int, default=800)
    ap.add_argument("--height", type=int, default=800)
    ap.add_argument("--spp", type=int, default=4)
    ap.add_argument("--depth", type=int, default=4)
    ap.add_argument("--pose", default="look", choices=list(POSES))
    ap.add_argument(
        "--light-grid", type=int, default=4, help="NxN point lights for the area source"
    )
    ap.add_argument("--exposure", type=float, default=1.1)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--no-spheres", action="store_true")
    ap.add_argument(
        "--model-yaw",
        type=float,
        default=None,
        help="override the pose's facing yaw (degrees)",
    )
    ap.add_argument("--out", default="render.png")
    ap.add_argument("--compare-depth", action="store_true")
    ap.add_argument("--compare-spp", action="store_true")
    args = ap.parse_args()

    dev = "cuda" if torch.cuda.is_available() else "cpu"
    scene, cam = make_scene(
        args.pose, args.light_grid, not args.no_spheres, args.model_yaw
    )
    gpu = SceneGPU(scene, device=dev)
    print(
        f"scene: {gpu.V.shape[0]} tris, {gpu.light_pos.shape[0]} lights, device={dev}"
    )

    if args.compare_depth:
        print("recursion-depth comparison:")
        for d in (1, 2, 4):
            render_once(args, gpu, cam, d, args.spp, f"compare_depth_{d}.png", "depth ")
    elif args.compare_spp:
        print("samples-per-pixel comparison:")
        for s in (1, 4, 16):
            render_once(args, gpu, cam, args.depth, s, f"compare_spp_{s}.png", "spp ")
    else:
        render_once(args, gpu, cam, args.depth, args.spp, args.out)


if __name__ == "__main__":
    main()
