#!/usr/bin/env python3
"""Classify dig-frame PPM/PNG center crop: void vs grass vs rock vs shadow.

Refs (must match Main.cpp WriteCertPixelReport / VisualMaterial CapColor / glClearColor):
  void_clear        = RGB(114,158,224)  # glClearColor 0.45, 0.62, 0.88
  legacy_backdrop   = RGB(178,204,140)  # OLD ground quad 0.70,0.80,0.55 — mesh-gap void, NOT grass
  grass_mid         = RGB(93,133,68)
  limestone         = RGB(193,189,174)
  mica_mid          = RGB(126,123,107)

Usage:
  python classify_dig_frame.py path/to/frame.ppm
  python classify_dig_frame.py path/to/screenshot.png
"""
from __future__ import annotations
import sys
from pathlib import Path

VOID = (114, 158, 224)
LEGACY_BACKDROP = (178, 204, 140)  # pre-voidfix lime ground quad
GRASS = (93, 133, 68)
LIMESTONE = (193, 189, 174)
MICA = (126, 123, 107)


def d2(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b))


def load_rgb(path: Path):
    try:
        from PIL import Image
    except ImportError as e:
        raise SystemExit("Pillow required: pip install pillow") from e
    im = Image.open(path).convert("RGB")
    return im


def classify_center(im, frac=0.2):
    w, h = im.size
    x0 = int(w * (0.5 - frac / 2))
    x1 = int(w * (0.5 + frac / 2))
    y0 = int(h * (0.5 - frac / 2))
    y1 = int(h * (0.5 + frac / 2))
    crop = im.crop((x0, y0, x1, y1))
    pixels = list(crop.getdata())
    sky = grass = chalk = dark = back = other = 0
    void_samples = []
    for r, g, b in pixels:
        if r + g + b < 90:
            dark += 1
            continue
        if d2((r, g, b), LEGACY_BACKDROP) <= 64:
            back += 1
            if len(void_samples) < 6:
                void_samples.append((r, g, b))
            continue
        dv = d2((r, g, b), VOID)
        dg = d2((r, g, b), GRASS)
        dc = d2((r, g, b), LIMESTONE)
        dm = d2((r, g, b), MICA)
        if b > g and b > r and b > 150 and dv <= dg and dv <= dc:
            sky += 1
            if len(void_samples) < 6:
                void_samples.append((r, g, b))
        elif dg <= dv and dg <= dc and dg <= dm and g > r + 20 and g > b + 20:
            grass += 1
        elif dc <= dv and dc <= dg and (dc <= dm or r + g + b > 480):
            chalk += 1
        else:
            other += 1
    n = max(1, len(pixels))
    return {
        "n": len(pixels),
        "sky_void_pct": 100.0 * sky / n,
        "legacy_backdrop_void_pct": 100.0 * back / n,
        "grass_pct": 100.0 * grass / n,
        "chalk_stone_pct": 100.0 * chalk / n,
        "shadow_dark_pct": 100.0 * dark / n,
        "other_pct": 100.0 * other / n,
        "void_samples": void_samples,
        "refs": {
            "void": VOID,
            "legacy_backdrop": LEGACY_BACKDROP,
            "grass": GRASS,
            "limestone": LIMESTONE,
            "mica": MICA,
        },
    }


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    path = Path(sys.argv[1])
    im = load_rgb(path)
    r = classify_center(im)
    print(f"file={path}")
    print(f"size={im.size[0]}x{im.size[1]} center_samples={r['n']}")
    print(f"refs={r['refs']}")
    for k in (
        "sky_void_pct",
        "legacy_backdrop_void_pct",
        "grass_pct",
        "chalk_stone_pct",
        "shadow_dark_pct",
        "other_pct",
    ):
        print(f"{k}={r[k]:.2f}")
    if r["void_samples"]:
        print(f"void_samples={r['void_samples']}")
    print(
        "rule: mesh-gap void = sky_void (blue) or legacy_backdrop RGB(178,204,140). "
        "That lime was the OLD far-ground quad — not grass CapColor (93,133,68)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
