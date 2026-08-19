"""MS1.A SurfaceState diagnostic maps (authority evidence, NOT final appearance).

Renders what the WORLD KNOWS about its exposed surface -- substrate class, host
lithology, dominant family, and the state axes -- from the compiled macro page
descriptors (the bytes actually shipped), plus a seed-corpus family comparison.

These are DIAGNOSTIC category maps: flat class colours so the semantics are legible.
They are deliberately NOT the MS1.B derived appearance (albedo/hillshade); MS1.A owns
the knowledge, MS1.B owns the look.

Run:  python Tools/Worldgen/viz_surface_state.py
Emits: Docs/provenance_ms1a_surface_state_maps.png
       Docs/provenance_ms1a_seed_corpus.png
"""
from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

import macro_authority as MA
from macro_authority import (CentralProgram, MacroField, unpack_surface, surface_state_at,
                             SUBSTRATE_CLASSES, LITHOLOGY_CLASSES, SURFACE_FAMILIES, REGION_M,
                             REGION_HALF_M)

ROOT = Path(__file__).resolve().parents[2]
PAGE_DIR = ROOT / "Data" / "Worldgen" / "MacroAuthority"
DOCS = ROOT / "Docs"

# ---- diagnostic category palettes (RGB) ----------------------------------- #
FAMILY_COL = {
    "rock": (150, 150, 156), "regolith": (196, 168, 120), "sediment": (222, 205, 130),
    "volcanic": (150, 60, 52), "organic": (78, 132, 70),
}
SUBSTRATE_COL = {
    "bare_bedrock": (128, 128, 134), "weathered_bedrock": (160, 150, 140),
    "thin_regolith": (198, 170, 122), "colluvium": (176, 150, 108), "talus": (140, 128, 120),
    "alluvium": (224, 206, 140), "floodplain_sediment": (232, 222, 170),
    "basin_fill": (206, 190, 150), "organic_capable": (96, 140, 78),
    "waterlogged_mineral": (120, 150, 150), "fresh_lava": (60, 44, 44),
    "scoria_ash": (128, 74, 62), "weathered_basalt": (150, 96, 80),
    "volcanic_soil": (110, 96, 66),
}
LITHOLOGY_COL = {
    "granite": (200, 168, 160), "basalt": (96, 74, 70), "sandstone": (224, 188, 128),
    "shale": (120, 128, 130), "limestone": (208, 208, 190), "quartzite": (216, 200, 210),
    "metamorphic": (150, 120, 150), "mixed_unknown": (110, 110, 110),
}


def _ramp(v: float, lo, hi):
    return tuple(int(lo[i] + (hi[i] - lo[i]) * max(0.0, min(1.0, v))) for i in range(3))


def load_pages():
    """Assemble a global +/-160 km packed-code grid from the shipped page descriptors."""
    cells = {}
    minri = minrj = 10**9
    maxri = maxrj = -10**9
    sn = None
    for f in PAGE_DIR.glob("page_*.mcp"):
        kv = {}
        codes = []
        for line in f.read_text(encoding="utf-8").splitlines():
            if line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            if k == "surface_grid_row_major":
                codes = [int(t, 16) for t in v.split()]
            else:
                kv[k] = v
        if "region_cell" not in kv or not codes:
            continue
        ri, rj = (int(t) for t in kv["region_cell"].split(","))
        sn = int(kv["surface_grid_n"])
        cells[(ri, rj)] = codes
        minri, maxri = min(minri, ri), max(maxri, ri)
        minrj, maxrj = min(minrj, rj), max(maxrj, rj)
    if not cells or sn is None:
        raise SystemExit("no compiled pages with surface descriptors; run cert_macro_authority.py first")
    # each page contributes an (sn-1) x (sn-1) block (drop the shared far edge) so pages tile.
    bw = sn - 1
    W = (maxri - minri + 1) * bw + 1
    H = (maxrj - minrj + 1) * bw + 1
    grid = [[None] * W for _ in range(H)]
    for (ri, rj), codes in cells.items():
        ox = (ri - minri) * bw
        oy = (maxrj - rj) * bw          # rj increases north -> draw upward
        for j in range(sn):
            for i in range(sn):
                gx, gy = ox + i, oy + (sn - 1 - j)
                if 0 <= gx < W and 0 <= gy < H:
                    grid[gy][gx] = codes[j * sn + i]
    return grid, W, H


def draw_map(grid, W, H, colour_fn, scale=6):
    img = Image.new("RGB", (W * scale, H * scale), (30, 30, 34))
    px = img.load()
    for gy in range(H):
        for gx in range(W):
            code = grid[gy][gx]
            if code is None:
                continue
            col = colour_fn(unpack_surface(code))
            for a in range(scale):
                for b in range(scale):
                    px[gx * scale + a, gy * scale + b] = col
    return img


def label(img, text):
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, len(text) * 7 + 8, 16], fill=(0, 0, 0))
    d.text((4, 3), text, fill=(240, 240, 240))
    return img


def legend(entries, width, cell=16, cols=4):
    rows = (len(entries) + cols - 1) // cols
    img = Image.new("RGB", (width, rows * (cell + 4) + 6), (30, 30, 34))
    d = ImageDraw.Draw(img)
    cw = width // cols
    for idx, (name, col) in enumerate(entries):
        r, c = idx // cols, idx % cols
        x, y = c * cw + 6, r * (cell + 4) + 4
        d.rectangle([x, y, x + cell, y + cell], fill=col, outline=(80, 80, 80))
        d.text((x + cell + 5, y + 3), name, fill=(220, 220, 220))
    return img


def stack(images, pad=8, bg=(24, 24, 28)):
    w = max(im.width for im in images)
    h = sum(im.height for im in images) + pad * (len(images) + 1)
    out = Image.new("RGB", (w, h), bg)
    y = pad
    for im in images:
        out.paste(im, (0, y))
        y += im.height + pad
    return out


def row(images, pad=8, bg=(24, 24, 28)):
    h = max(im.height for im in images)
    w = sum(im.width for im in images) + pad * (len(images) + 1)
    out = Image.new("RGB", (w, h + 2 * pad), bg)
    x = pad
    for im in images:
        out.paste(im, (x, pad))
        x += im.width + pad
    return out


def main():
    grid, W, H = load_pages()

    fam_map = label(draw_map(grid, W, H, lambda s: FAMILY_COL[s.dominant_surface_family]),
                    "dominant surface family (rock/regolith/sediment/volcanic/organic)")
    sub_map = label(draw_map(grid, W, H, lambda s: SUBSTRATE_COL[s.substrate_class]),
                    "substrate class (exposure-precedence winner)")
    lith_map = label(draw_map(grid, W, H, lambda s: LITHOLOGY_COL[s.lithology_class]),
                     "host lithology / parent material (ancestry)")
    wet_map = label(draw_map(grid, W, H, lambda s: _ramp(s.wetness, (180, 168, 120), (30, 60, 150))),
                    "wetness state axis (dry -> saturated)")
    weath_map = label(draw_map(grid, W, H, lambda s: _ramp(s.weathering, (150, 150, 150), (120, 78, 40))),
                      "weathering / maturity axis (fresh -> deeply weathered)")
    soil_map = label(draw_map(grid, W, H, lambda s: _ramp(s.soil_depth, (170, 150, 120), (40, 110, 40))),
                     "soil / regolith depth axis (thin -> deep)")
    exp_map = label(draw_map(grid, W, H, lambda s: _ramp(s.exposure, (60, 90, 50), (210, 210, 210))),
                    "exposure axis (vegetated/soil -> bare rock)")

    rowW = fam_map.width * 2 + 24
    fam_leg = legend([(k, v) for k, v in FAMILY_COL.items()], rowW, cols=5)
    sub_leg = legend([(k, SUBSTRATE_COL[k]) for k in SUBSTRATE_CLASSES], rowW, cols=4)
    lith_leg = legend([(k, LITHOLOGY_COL[k]) for k in LITHOLOGY_CLASSES], rowW, cols=4)

    top = row([fam_map, sub_map])
    mid = row([lith_map, wet_map])
    bot = row([weath_map, soil_map])
    sheet = stack([label(Image.new("RGB", (rowW, 20), (24, 24, 28)),
                         "MS1.A SurfaceState diagnostic maps (+/-160 km origin world; class colours, NOT MS1.B appearance)"),
                   top, fam_leg, mid, sub_leg, bot, row([exp_map, lith_leg])])
    DOCS.mkdir(parents=True, exist_ok=True)
    out1 = DOCS / "provenance_ms1a_surface_state_maps.png"
    sheet.save(out1)
    print("wrote", out1)

    # ---- seed-corpus family comparison (recompute; per-seed drainage compile) ---------- #
    central = CentralProgram.load()
    corpus = [central.seed, central.seed + "-A", central.seed + "-B", central.seed + "-C"]
    span, step = 120000, 8000
    coords = list(range(-span, span + 1, step))
    panels = []
    for sd in corpus:
        ff = MacroField.build(sd)
        cn = len(coords)
        img = Image.new("RGB", (cn, cn), (30, 30, 34))
        px = img.load()
        for j, y in enumerate(coords):
            for i, x in enumerate(coords):
                s = surface_state_at(central, ff, float(x), float(y))
                px[i, cn - 1 - j] = FAMILY_COL[s.dominant_surface_family]
        img = img.resize((cn * 4, cn * 4), Image.NEAREST)
        panels.append(label(img, f"seed …{sd[-6:]}"))
    corpus_img = stack([label(Image.new("RGB", (panels[0].width * 2 + 24, 20), (24, 24, 28)),
                             "MS1.A surface family across a seed corpus (different seed -> different surface world)"),
                        row([panels[0], panels[1]]), row([panels[2], panels[3]]),
                        legend([(k, v) for k, v in FAMILY_COL.items()], panels[0].width * 2 + 24, cols=5)])
    out2 = DOCS / "provenance_ms1a_seed_corpus.png"
    corpus_img.save(out2)
    print("wrote", out2)


if __name__ == "__main__":
    main()
