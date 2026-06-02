from __future__ import annotations

import json
import math
import os
from dataclasses import dataclass, field
from enum import IntEnum

import numpy as np
from PIL import Image
from pyglet.gl import (
    GL_LINEAR,
    GL_LINEAR_MIPMAP_LINEAR,
    GL_REPEAT,
    GL_RGBA,
    GL_TEXTURE_2D,
    GL_TEXTURE_MAG_FILTER,
    GL_TEXTURE_MIN_FILTER,
    GL_TEXTURE_WRAP_S,
    GL_TEXTURE_WRAP_T,
    GL_UNSIGNED_BYTE,
    GLuint,
    glBindTexture,
    glGenTextures,
    glGenerateMipmap,
    glTexImage2D,
    glTexParameteri,
)
from pyglet.math import Mat4, Vec3

# ---------------------------------------------------------------------------
# Shading modes
# ---------------------------------------------------------------------------


class ShadeMode(IntEnum):
    WIREFRAME = 0
    GOURAUD = 1
    PHONG = 2
    PHONG_TEX = 3
    PHONG_NORMAL = 4


_MODE_BY_NAME = {
    "wireframe": ShadeMode.WIREFRAME,
    "gouraud": ShadeMode.GOURAUD,
    "phong": ShadeMode.PHONG,
    "phong_tex": ShadeMode.PHONG_TEX,
    "phong_normal": ShadeMode.PHONG_NORMAL,
}
_NAME_BY_MODE = {v: k for k, v in _MODE_BY_NAME.items()}


# ---------------------------------------------------------------------------
# Textures
# ---------------------------------------------------------------------------


@dataclass
class Texture2D:
    handle: int


_tex_cache: dict[str, Texture2D] = {}


def load_texture(path: str) -> Texture2D:
    """Decode an image and upload as a GL_REPEAT, mipmapped RGBA texture."""
    hit = _tex_cache.get(path)
    if hit is not None:
        return hit

    img = Image.open(path).convert("RGBA").transpose(Image.FLIP_TOP_BOTTOM)
    data = np.asarray(img, dtype=np.uint8).tobytes()
    w, h = img.size

    tex = GLuint(0)
    glGenTextures(1, tex)
    glBindTexture(GL_TEXTURE_2D, tex)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data)
    glGenerateMipmap(GL_TEXTURE_2D)
    glBindTexture(GL_TEXTURE_2D, 0)

    result = Texture2D(handle=int(tex.value))
    _tex_cache[path] = result
    return result


# ---------------------------------------------------------------------------
# Materials + lights + scene-globals
# ---------------------------------------------------------------------------


@dataclass
class Material:
    ka: Vec3 = field(default_factory=lambda: Vec3(0.1, 0.1, 0.1))
    kd: Vec3 = field(default_factory=lambda: Vec3(0.5, 0.5, 0.5))
    ks: Vec3 = field(default_factory=lambda: Vec3(0.8, 0.8, 0.8))
    shininess: float = 8.0
    rough_a: float = 200.0
    rough_b: float = 0.05
    base_color: Texture2D | None = None
    ao: Texture2D | None = None
    specular: Texture2D | None = None
    roughness: Texture2D | None = None
    normal: Texture2D | None = None


@dataclass
class Light:
    position: Vec3
    color: Vec3 = field(default_factory=lambda: Vec3(1.0, 1.0, 1.0))
    intensity: float = 1.0


@dataclass
class Scene:
    lights: list[Light] = field(default_factory=list)
    ambient: Vec3 = field(default_factory=lambda: Vec3(1.0, 1.0, 1.0))
    background: tuple[float, float, float, float] = (0.05, 0.06, 0.08, 1.0)
    wire_color: Vec3 = field(default_factory=lambda: Vec3(0.85, 0.9, 1.0))


# ---------------------------------------------------------------------------
# Per-object transform + scene item
# ---------------------------------------------------------------------------


@dataclass
class Transform:
    translate: list[float] = field(default_factory=lambda: [0.0, 0.0, 0.0])
    rotate_deg: list[float] = field(default_factory=lambda: [0.0, 0.0, 0.0])
    scale: list[float] = field(default_factory=lambda: [1.0, 1.0, 1.0])

    def matrix(self) -> Mat4:
        t = Mat4.from_translation(Vec3(*self.translate))
        rx = Mat4.from_rotation(math.radians(self.rotate_deg[0]), Vec3(1.0, 0.0, 0.0))
        ry = Mat4.from_rotation(math.radians(self.rotate_deg[1]), Vec3(0.0, 1.0, 0.0))
        rz = Mat4.from_rotation(math.radians(self.rotate_deg[2]), Vec3(0.0, 0.0, 1.0))
        s = Mat4.from_scale(Vec3(*self.scale))
        return t @ ry @ rx @ rz @ s

    @classmethod
    def from_dict(cls, d: dict) -> "Transform":
        return cls(
            translate=[float(x) for x in d.get("translate", [0.0, 0.0, 0.0])],
            rotate_deg=[float(x) for x in d.get("rotate_deg", [0.0, 0.0, 0.0])],
            scale=[float(x) for x in d.get("scale", [1.0, 1.0, 1.0])],
        )

    def to_dict(self) -> dict:
        return {
            "translate": list(self.translate),
            "rotate_deg": list(self.rotate_deg),
            "scale": list(self.scale),
        }


@dataclass
class SceneItem:
    name: str
    obj_file: str
    transform: Transform
    material_spec: dict
    mode: ShadeMode
    normalize: bool = True


# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------


_DEFAULT_LIGHTS = (
    {"position": [4.0, 5.0, 4.0], "color": [1.0, 0.95, 0.85], "intensity": 1.2},
    {"position": [-5.0, 3.0, -2.0], "color": [0.4, 0.5, 0.9], "intensity": 0.6},
    {"position": [0.0, -2.0, 4.0], "color": [0.9, 0.5, 0.3], "intensity": 0.3},
)
_DEFAULT_AMBIENT = [0.6, 0.65, 0.75]
_DEFAULT_BG = [0.04, 0.05, 0.08, 1.0]
_DEFAULT_WIRE = [0.85, 0.9, 1.0]


def _default_material_spec() -> dict:
    return {
        "ka": [0.1, 0.1, 0.1],
        "kd": [0.5, 0.5, 0.5],
        "ks": [0.8, 0.8, 0.8],
        "shininess": 8.0,
        "rough_a": 200.0,
        "rough_b": 0.05,
        "textures": {},
    }


def _merge_material_spec(d: dict | None) -> dict:
    base = _default_material_spec()
    if not d:
        return base
    for k, v in d.items():
        if k == "textures" and isinstance(v, dict):
            base["textures"] = dict(v)
        else:
            base[k] = v
    return base


# ---------------------------------------------------------------------------
# Builders
# ---------------------------------------------------------------------------


def build_material(base_dir: str, spec: dict) -> Material:
    spec = _merge_material_spec(spec)
    tex = spec.get("textures") or {}

    def _load(key: str) -> Texture2D | None:
        rel = tex.get(key)
        if not rel:
            return None
        path = rel if os.path.isabs(rel) else os.path.join(base_dir, rel)
        return load_texture(path)

    return Material(
        ka=Vec3(*spec["ka"]),
        kd=Vec3(*spec["kd"]),
        ks=Vec3(*spec["ks"]),
        shininess=float(spec["shininess"]),
        rough_a=float(spec["rough_a"]),
        rough_b=float(spec["rough_b"]),
        base_color=_load("base_color"),
        ao=_load("ao"),
        specular=_load("specular"),
        roughness=_load("roughness"),
        normal=_load("normal"),
    )


def _light_from_dict(d: dict) -> Light:
    return Light(
        position=Vec3(*d["position"]),
        color=Vec3(*d.get("color", [1.0, 1.0, 1.0])),
        intensity=float(d.get("intensity", 1.0)),
    )


def _light_to_dict(light: Light) -> dict:
    return {
        "position": [light.position.x, light.position.y, light.position.z],
        "color": [light.color.x, light.color.y, light.color.z],
        "intensity": float(light.intensity),
    }


def _build_scene(data: dict) -> Scene:
    raw_lights = data.get("lights")
    if raw_lights is None:
        raw_lights = list(_DEFAULT_LIGHTS)
    return Scene(
        lights=[_light_from_dict(L) for L in raw_lights],
        ambient=Vec3(*data.get("ambient", _DEFAULT_AMBIENT)),
        background=tuple(data.get("background", _DEFAULT_BG)),
        wire_color=Vec3(*data.get("wire_color", _DEFAULT_WIRE)),
    )


def _scene_to_dict(scene: Scene) -> dict:
    return {
        "lights": [_light_to_dict(L) for L in scene.lights],
        "ambient": [scene.ambient.x, scene.ambient.y, scene.ambient.z],
        "background": list(scene.background),
        "wire_color": [scene.wire_color.x, scene.wire_color.y, scene.wire_color.z],
    }


# ---------------------------------------------------------------------------
# scene.json load / save
# ---------------------------------------------------------------------------


def load_scene(scene_path: str, base_dir: str) -> tuple[Scene, list[SceneItem]]:
    """Read scene.json, then auto-add any OBJ in base_dir not yet listed."""
    data: dict = {}
    if os.path.exists(scene_path):
        with open(scene_path) as f:
            data = json.load(f)

    scene = _build_scene(data)

    items: list[SceneItem] = []
    seen: set[str] = set()
    for raw in data.get("objects", []):
        obj_file = raw["file"]
        items.append(
            SceneItem(
                name=raw.get("name", os.path.splitext(os.path.basename(obj_file))[0]),
                obj_file=obj_file,
                transform=Transform.from_dict(raw.get("transform") or {}),
                material_spec=_merge_material_spec(raw.get("material")),
                mode=_MODE_BY_NAME.get(raw.get("mode", "phong"), ShadeMode.PHONG),
                normalize=raw.get("normalize", True),
            )
        )
        seen.add(_norm(obj_file))

    for root, _, files in os.walk(base_dir):
        for fname in sorted(files):
            if not fname.lower().endswith(".obj"):
                continue
            rel = os.path.relpath(os.path.join(root, fname), base_dir)
            if _norm(rel) in seen:
                continue
            items.append(
                SceneItem(
                    name=os.path.splitext(fname)[0],
                    obj_file=rel,
                    transform=Transform(),
                    material_spec=_default_material_spec(),
                    mode=ShadeMode.PHONG,
                )
            )
            seen.add(_norm(rel))

    return scene, items


def save_scene(scene_path: str, scene: Scene, items: list[SceneItem]) -> None:
    out = _scene_to_dict(scene)
    out["objects"] = [
        {
            "name": it.name,
            "file": it.obj_file,
            "mode": _NAME_BY_MODE.get(it.mode, "phong"),
            "transform": it.transform.to_dict(),
            "material": it.material_spec,
        }
        for it in items
    ]
    os.makedirs(os.path.dirname(scene_path) or ".", exist_ok=True)
    with open(scene_path, "w") as f:
        json.dump(out, f, indent=2)


def _norm(p: str) -> str:
    return os.path.normpath(p).replace(os.sep, "/")
