"""Pinhole camera with look-at framing and jittered primary-ray generation."""

from __future__ import annotations

import math
from dataclasses import dataclass

import torch

from geometry import normalize


@dataclass
class Camera:
    eye: tuple[float, float, float] = (0.0, 5.0, 14.0)
    target: tuple[float, float, float] = (0.0, 5.0, 0.0)
    up: tuple[float, float, float] = (0.0, 1.0, 0.0)
    fov: float = 45.0  # vertical, degrees

    def rays(self, width: int, height: int, device, jitter: bool = False, gen=None):
        """Return (origin[P,3], dir[P,3]) for P=width*height pixels, row-major."""
        eye = torch.tensor(self.eye, device=device, dtype=torch.float32)
        target = torch.tensor(self.target, device=device, dtype=torch.float32)
        up = torch.tensor(self.up, device=device, dtype=torch.float32)

        fwd = normalize(target - eye)
        right = normalize(torch.cross(fwd, up, dim=-1))
        true_up = torch.cross(right, fwd, dim=-1)

        half_h = math.tan(math.radians(self.fov) * 0.5)
        half_w = half_h * (width / height)

        ys, xs = torch.meshgrid(
            torch.arange(height, device=device, dtype=torch.float32),
            torch.arange(width, device=device, dtype=torch.float32),
            indexing="ij",
        )
        xs = xs.reshape(-1)
        ys = ys.reshape(-1)
        if jitter:
            jx = torch.rand(xs.shape, device=device, generator=gen)
            jy = torch.rand(ys.shape, device=device, generator=gen)
        else:
            jx = torch.full_like(xs, 0.5)
            jy = torch.full_like(ys, 0.5)

        ndc_x = ((xs + jx) / width) * 2.0 - 1.0
        ndc_y = 1.0 - ((ys + jy) / height) * 2.0

        d = (
            fwd[None, :]
            + (ndc_x * half_w)[:, None] * right[None, :]
            + (ndc_y * half_h)[:, None] * true_up[None, :]
        )
        d = normalize(d)
        o = eye[None, :].expand_as(d).contiguous()
        return o, d
