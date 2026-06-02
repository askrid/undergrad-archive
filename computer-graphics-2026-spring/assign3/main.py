"""Run a model folder.

    python main.py <name>
"""

from __future__ import annotations

import os
import sys

from control import Control
from mesh import load_obj
from render import RenderWindow
from scene import build_material, load_scene

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_MODEL = "rock"


def main() -> None:
    name = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_MODEL
    model_dir = os.path.join(HERE, "models", name)
    scene_path = os.path.join(model_dir, "scene.json")
    if not os.path.isdir(model_dir):
        sys.exit(f"no model folder: {model_dir}")

    scene_def, items = load_scene(scene_path, model_dir)
    window = RenderWindow(
        1280, 720, f"CG PA3 - {name}", resizable=True, scene=scene_def
    )
    window.set_location(120, 120)

    groups = []
    for it in items:
        mesh = load_obj(os.path.join(model_dir, it.obj_file), normalize=it.normalize)
        material = build_material(model_dir, it.material_spec)
        group = window.add_shape(it.transform.matrix(), mesh, material, it.mode)
        groups.append(group)

    Control(window, name, items, groups, scene_path)
    window.run()


if __name__ == "__main__":
    main()
