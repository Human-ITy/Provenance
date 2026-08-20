"""WD1.A WaterState diagnostic maps (authority evidence; DIAGNOSTIC categories, NOT WD1.B
appearance). Reads the compiled macro page WaterState descriptors and paints presence /
body-class / depth / optical-state maps over the ±160 km origin world, plus a seed-corpus
body-class comparison. Class colours so the semantics are legible — the actual continuous
optical water look is WD1.B.

Run:  python Tools/Worldgen/viz_water_state.py
Emits: Docs/provenance_wd1a_water_state_maps.png
       Docs/provenance_wd1a_seed_corpus.png
"""
from __future__ import annotations
from pathlib import Path
from PIL import Image, ImageDraw

import macro_authority as MA
from macro_authority import (CentralProgram, MacroField, unpack_water, water_state_at,
                             PRESENCE_REGIMES, BODY_CLASSES)

ROOT = Path(__file__).resolve().parents[2]
PAGE_DIR = ROOT / "Data" / "Worldgen" / "MacroAuthority"
DOCS = ROOT / "Docs"

PRESENCE_COL = {
    "dry": (196, 180, 140), "damp_substrate": (150, 156, 110), "ephemeral": (150, 190, 200),
    "seasonal": (90, 150, 200), "perennial": (40, 100, 200), "standing": (20, 50, 150),
}
BODY_COL = {
    "none": (40, 42, 40), "headwater_stream": (120, 210, 220), "perennial_river": (40, 110, 210),
    "sediment_river": (150, 130, 70), "braided_reach": (170, 150, 90), "alpine_lake": (90, 200, 220),
    "closed_basin_lake": (30, 80, 170), "floodplain_water": (90, 140, 170), "wetland_marsh": (70, 140, 90),
    "organic_darkwater": (60, 80, 55), "arid_wash": (200, 180, 130), "spring_pool": (140, 220, 200),
    "volcanic_mineral_pool": (40, 180, 160), "crater_lake": (30, 200, 180),
}


def _ramp(v, lo, hi):
    return tuple(int(lo[i] + (hi[i] - lo[i]) * max(0.0, min(1.0, v))) for i in range(3))


def load_water_grid():
    cells = {}; wn = None; minri = minrj = 1 << 20; maxri = maxrj = -(1 << 20)
    for f in PAGE_DIR.glob("page_*.mcp"):
        kv = {}; codes = []
        for line in f.read_text(encoding="utf-8").splitlines():
            if line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            if k == "water_grid_row_major":
                codes = [int(t, 16) for t in v.split()]
            else:
                kv[k] = v
        if "region_cell" not in kv or not codes:
            continue
        ri, rj = (int(t) for t in kv["region_cell"].split(","))
        wn = int(kv["water_grid_n"])
        cells[(ri, rj)] = codes
        minri, maxri = min(minri, ri), max(maxri, ri); minrj, maxrj = min(minrj, rj), max(maxrj, rj)
    if not cells:
        raise SystemExit("no compiled pages with water descriptors; run cert_macro_authority.py first")
    bw = wn - 1
    W = (maxri - minri + 1) * bw + 1; H = (maxrj - minrj + 1) * bw + 1
    grid = [[None] * W for _ in range(H)]
    for (ri, rj), codes in cells.items():
        ox = (ri - minri) * bw; oy = (maxrj - rj) * bw
        for j in range(wn):
            for i in range(wn):
                gx, gy = ox + i, oy + (wn - 1 - j)
                if 0 <= gx < W and 0 <= gy < H:
                    grid[gy][gx] = codes[j * wn + i]
    return grid, W, H


def draw(grid, W, H, fn, scale=6):
    img = Image.new("RGB", (W * scale, H * scale), (24, 24, 28)); px = img.load()
    for gy in range(H):
        for gx in range(W):
            if grid[gy][gx] is None:
                continue
            col = fn(unpack_water(grid[gy][gx]))
            for a in range(scale):
                for b in range(scale):
                    px[gx * scale + a, gy * scale + b] = col
    return img


def label(img, text):
    d = ImageDraw.Draw(img); d.rectangle([0, 0, len(text) * 7 + 8, 16], fill=(0, 0, 0))
    d.text((4, 3), text, fill=(240, 240, 240)); return img


def legend(entries, width, cols=4, cell=15):
    rows = (len(entries) + cols - 1) // cols
    img = Image.new("RGB", (width, rows * (cell + 4) + 6), (24, 24, 28)); d = ImageDraw.Draw(img)
    cw = width // cols
    for idx, (name, col) in enumerate(entries):
        r, c = idx // cols, idx % cols
        x, y = c * cw + 6, r * (cell + 4) + 4
        d.rectangle([x, y, x + cell, y + cell], fill=col, outline=(80, 80, 80))
        d.text((x + cell + 5, y + 2), name, fill=(220, 220, 220))
    return img


def row(imgs, pad=8):
    h = max(i.height for i in imgs); w = sum(i.width for i in imgs) + pad * (len(imgs) + 1)
    o = Image.new("RGB", (w, h + 2 * pad), (24, 24, 28)); x = pad
    for i in imgs:
        o.paste(i, (x, pad)); x += i.width + pad
    return o


def stack(imgs, pad=8):
    w = max(i.width for i in imgs); h = sum(i.height for i in imgs) + pad * (len(imgs) + 1)
    o = Image.new("RGB", (w, h), (18, 18, 22)); y = pad
    for i in imgs:
        o.paste(i, (0, y)); y += i.height + pad
    return o


def main():
    grid, W, H = load_water_grid()
    pres = label(draw(grid, W, H, lambda w: PRESENCE_COL[w.presence_regime]),
                 "presence regime (dry -> standing)")
    body = label(draw(grid, W, H, lambda w: BODY_COL[w.body_class]),
                 "water body / regime class")
    depth = label(draw(grid, W, H, lambda w: _ramp(min(1.0, w.depth_m / 40.0), (30, 40, 55), (60, 200, 240))
                       if w.body_class != "none" else (30, 32, 34)),
                  "depth (continuous m; 0..40m ramp)")
    turb = label(draw(grid, W, H, lambda w: _ramp(w.turbidity, (60, 120, 190), (150, 120, 60))
                      if w.body_class != "none" else (30, 32, 34)),
                 "turbidity (clear blue -> turbid tan)")
    org = label(draw(grid, W, H, lambda w: _ramp(w.organic_load, (60, 120, 160), (50, 70, 45))
                     if w.body_class != "none" else (30, 32, 34)),
                "organic load (clear -> dark organic)")
    minl = label(draw(grid, W, H, lambda w: _ramp(w.mineral_load, (60, 110, 170), (40, 200, 180))
                      if w.body_class != "none" else (30, 32, 34)),
                 "mineral load (ordinary -> mineral/volcanic)")

    rowW = pres.width * 2 + 24
    pres_leg = legend([(k, v) for k, v in PRESENCE_COL.items()], rowW, cols=6)
    body_leg = legend([(k, BODY_COL[k]) for k in BODY_CLASSES if k != "none"], rowW, cols=4)
    sheet = stack([label(Image.new("RGB", (rowW, 20), (18, 18, 22)),
                         "WD1.A WaterState diagnostic maps (±160 km origin world; class colours, NOT WD1.B optical appearance)"),
                   row([pres, body]), pres_leg, body_leg, row([depth, turb]), row([org, minl])])
    DOCS.mkdir(parents=True, exist_ok=True)
    out1 = DOCS / "provenance_wd1a_water_state_maps.png"; sheet.save(out1); print("wrote", out1)

    # seed corpus body-class comparison (recompute; per-seed drainage compile)
    central = CentralProgram.load()
    corpus = [central.seed, central.seed + "-A", central.seed + "-B", central.seed + "-C"]
    span, step = 120000, 8000
    coords = list(range(-span, span + 1, step)); cn = len(coords)
    panels = []
    for sd in corpus:
        ff = MacroField.build(sd)
        img = Image.new("RGB", (cn, cn), (24, 24, 28)); px = img.load()
        for j, y in enumerate(coords):
            for i, x in enumerate(coords):
                w = water_state_at(central, ff, float(x), float(y))
                px[i, cn - 1 - j] = BODY_COL[w.body_class] if w.body_class != "none" else PRESENCE_COL[w.presence_regime]
        panels.append(label(img.resize((cn * 4, cn * 4), Image.NEAREST), f"seed …{sd[-6:]}"))
    corpus_img = stack([label(Image.new("RGB", (panels[0].width * 2 + 24, 20), (18, 18, 22)),
                             "WD1.A water geography across a seed corpus (different seed -> different water world)"),
                        row([panels[0], panels[1]]), row([panels[2], panels[3]])])
    out2 = DOCS / "provenance_wd1a_seed_corpus.png"; corpus_img.save(out2); print("wrote", out2)


if __name__ == "__main__":
    main()
