"""Tone mapping + PNG output (Pillow)."""

from __future__ import annotations

import numpy as np
import torch
from PIL import Image


def tonemap(img: torch.Tensor, exposure: float = 1.0, gamma: float = 2.2) -> np.ndarray:
    """HDR radiance [H,W,3] -> 8-bit sRGB-ish numpy."""
    c = img.clamp_min(0.0) * exposure
    c = c / (1.0 + c)  # Reihnard tone mapping
    c = c.clamp(0.0, 1.0) ** (1.0 / gamma)
    return (c.detach().cpu().numpy() * 255.0 + 0.5).astype(np.uint8)


def save_png(img: torch.Tensor, path: str, exposure: float = 1.0) -> None:
    Image.fromarray(tonemap(img, exposure)).save(path)
