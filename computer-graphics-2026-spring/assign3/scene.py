"""Material, light, scene container, texture loading."""

from __future__ import annotations

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
from pyglet.math import Vec3


class ShadeMode(IntEnum):
    WIREFRAME = 0
    GOURAUD = 1
    PHONG = 2
    PHONG_TEX = 3
    PHONG_NORMAL = 4


@dataclass
class Texture2D:
    handle: int


_cache: dict[str, Texture2D] = {}


def load_texture(path: str) -> Texture2D:
    """Decode an image and upload as a GL_REPEAT, mipmapped RGBA texture."""
    hit = _cache.get(path)
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
    _cache[path] = result
    return result


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
