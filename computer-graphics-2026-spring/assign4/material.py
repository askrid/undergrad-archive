"""Blinn-Phong + Whitted materials, collected into a GPU table indexed per surface."""

from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

Rgb = tuple[float, float, float]

NCOLS = 14


@dataclass
class Material:
    albedo: Rgb = (0.8, 0.8, 0.8)
    specular: Rgb = (0.0, 0.0, 0.0)
    shininess: float = 16.0
    reflectivity: float = 0.0
    transparency: float = 0.0
    ior: float = 1.0
    emission: Rgb = (0.0, 0.0, 0.0)
    dielectric: bool = False

    def as_row(self) -> list[float]:
        return [
            *self.albedo,
            *self.specular,
            self.shininess,
            self.reflectivity,
            self.transparency,
            self.ior,
            *self.emission,
            1.0 if self.dielectric else 0.0,
        ]


def lambertian(albedo: Rgb) -> Material:
    return Material(albedo=albedo, specular=(0.05, 0.05, 0.05), shininess=8.0)


def glossy(
    albedo: Rgb,
    shininess: float = 120.0,
    reflectivity: float = 0.04,
    spec: float = 0.35,
) -> Material:
    return Material(
        albedo=albedo,
        specular=(spec, spec, spec),
        shininess=shininess,
        reflectivity=reflectivity,
    )


def mirror(tint: Rgb = (1.0, 1.0, 1.0)) -> Material:
    return Material(albedo=(0, 0, 0), specular=tint, shininess=400.0, reflectivity=0.95)


def glass(tint: Rgb = (1.0, 1.0, 1.0), ior: float = 1.5) -> Material:
    return Material(
        albedo=(0.0, 0.0, 0.0),
        specular=(0.25 * tint[0], 0.25 * tint[1], 0.25 * tint[2]),
        shininess=600.0,
        reflectivity=1.0,
        transparency=1.0,
        ior=ior,
        dielectric=True,
    )


def emissive(color: Rgb, power: float = 1.0) -> Material:
    return Material(
        albedo=(0, 0, 0),
        emission=(color[0] * power, color[1] * power, color[2] * power),
    )


@dataclass
class MaterialTable:
    rows: list[list[float]] = field(default_factory=list)
    _cache: dict[tuple, int] = field(default_factory=dict)

    def add(self, mat: Material) -> int:
        row = mat.as_row()
        key = tuple(round(v, 6) for v in row)
        if key in self._cache:
            return self._cache[key]
        idx = len(self.rows)
        self.rows.append(row)
        self._cache[key] = idx
        return idx

    def as_array(self) -> np.ndarray:
        return np.asarray(self.rows, dtype=np.float32).reshape(-1, NCOLS)
