"""MV3.C alpine peak/ridge hierarchy — authority visual.

Proves the STRUCTURE the cert measures: a dominant summit with real prominence, a
descending summit hierarchy, deep saddles/passes, and knife ridges — and that young
high-energy ranges are sharp while old ranges are rounded. Top-down hillshade (grey =
SHAPE, not the game palette) + along-ridge elevation profile + young/old cross-sections.

Uses macro_z (no drainage) so it is cheap and needs no super-tile compile.

Run:  python Tools/Worldgen/viz_mv3c_peaks.py
Emits: Docs/provenance_mv3c_peak_hierarchy.png
"""
from __future__ import annotations
import math
from pathlib import Path
from PIL import Image, ImageDraw

import macro_authority as MA
from macro_authority import CentralProgram, MacroField, macro_z, peak_at, controls_at

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "Docs"


def most_prominent_range(seeds):
    best = None
    for sd in seeds:
        ff = MacroField.build(sd)
        for gi in range(-8, 9):
            for gj in range(-8, 9):
                rp = ff._range_summits(gi, gj)
                if rp is not None and (best is None or rp["prominence"] > best[2]["prominence"]):
                    best = (sd, ff, rp)
    return best


def hillshade_panel(central, ff, cx, cy, half=46000.0, step=420.0):
    n = int(2 * half / step) + 1
    zs = [[0.0] * n for _ in range(n)]
    zmin, zmax = 1e18, -1e18
    for j in range(n):
        y = cy + (j - n // 2) * step
        for i in range(n):
            x = cx + (i - n // 2) * step
            z = macro_z(central, ff, x, y)
            zs[j][i] = z
            zmin = min(zmin, z); zmax = max(zmax, z)
    img = Image.new("RGB", (n, n), (20, 20, 24))
    px = img.load()
    lx, ly, lz = -0.55, -0.42, 0.72
    ll = math.sqrt(lx*lx+ly*ly+lz*lz); lx, ly, lz = lx/ll, ly/ll, lz/ll
    span = max(1.0, zmax - zmin)
    for j in range(1, n-1):
        for i in range(1, n-1):
            dzdx = (zs[j][i+1] - zs[j][i-1]) / (2*step)
            dzdy = (zs[j+1][i] - zs[j-1][i]) / (2*step)
            nx, ny, nz = -dzdx, -dzdy, 1.0
            nl = math.sqrt(nx*nx+ny*ny+nz*nz)
            nd = max(0.0, (nx*lx+ny*ly+nz*lz)/nl)
            shade = 0.25 + 0.9*nd
            t = (zs[j][i]-zmin)/span                       # elevation tint (SHAPE legibility only)
            base = (0.30+0.55*t, 0.34+0.50*t, 0.32+0.45*t)
            px[i, n-1-j] = tuple(min(255, int(255*b*shade)) for b in base)
    # mark summits
    d = ImageDraw.Draw(img)
    for gi2 in (-1, 0, 1):
        pass
    # find this range's summits via peak_at cluster: re-derive from the rp we know
    return img, zmin, zmax


def main():
    sd, ff, rp = most_prominent_range([
        CentralProgram.load().seed + s for s in ("", "-A", "-B", "-C", "-D")])
    central = CentralProgram.load()
    cx, cy = rp["cx"], rp["cy"]
    scale = 3

    hs, zmin, zmax = hillshade_panel(central, ff, cx, cy)
    hs = hs.resize((hs.width*scale, hs.height*scale), Image.NEAREST)
    d = ImageDraw.Draw(hs)
    half, step = 46000.0, 420.0
    n = int(2*half/step)+1
    def to_px(x, y):
        i = (x - cx)/step + n//2
        j = (y - cy)/step + n//2
        return (i*scale, (n-1-j)*scale)
    for (sx, sy, H, R, a) in rp["summits"]:
        pxp = to_px(sx, sy)
        is_dom = abs(H - rp["Hdom"]) < 1e-6
        col = (255, 60, 40) if is_dom else (255, 170, 40)
        rr = 7 if is_dom else 5
        d.ellipse([pxp[0]-rr, pxp[1]-rr, pxp[0]+rr, pxp[1]+rr], outline=col, width=2)
        d.text((pxp[0]+8, pxp[1]-6), f"{H:.0f}m", fill=col)
    d.rectangle([0, 0, hs.width, 18], fill=(0, 0, 0))
    d.text((4, 3), f"TOP-DOWN hillshade — seed …{sd[-2:]} range (prominence {rp['prominence']:.0f}m, "
                   f"dominant {rp['Hdom']:.0f}m, {len(rp['summits'])} summits, "
                   f"{'cluster' if rp['cluster'] else 'solitary'})  [grey=SHAPE, red=dominant summit]",
           fill=(240, 240, 240))

    # along-ridge profile
    az = rp["az"]; ux, uy = math.cos(az), math.sin(az)
    pw, ph = hs.width, 220
    prof = Image.new("RGB", (pw, ph), (16, 16, 20)); dp = ImageDraw.Draw(prof)
    L = 46000.0
    xs = [t for t in range(-int(L), int(L)+1, 300)]
    zs = [macro_z(central, ff, cx+ux*t, cy+uy*t) for t in xs]
    zlo, zhi = min(zs), max(zs)
    def py(z): return ph-14 - int((ph-30)*(z-zlo)/max(1.0, zhi-zlo))
    def pxc(t): return int((t+L)/(2*L)*(pw-1))
    pts = [(pxc(t), py(z)) for t, z in zip(xs, zs)]
    dp.line(pts, fill=(120, 200, 255), width=2)
    dp.rectangle([0, 0, pw, 16], fill=(0, 0, 0))
    dp.text((4, 2), f"ALONG-RIDGE elevation profile — summit ordering + saddles/passes "
                    f"(peak->saddle->peak, not one dome). span {zlo:.0f}-{zhi:.0f}m", fill=(240, 240, 240))

    # young vs old cross-section
    cmpw, cmph = hs.width, 240
    cmp = Image.new("RGB", (cmpw, cmph), (16, 16, 20)); dc = ImageDraw.Draw(cmp)
    dom = rp["dominant"]; vx, vy = -math.sin(az), math.cos(az)
    yv = [macro_z(central, ff, dom[0]+vx*t, dom[1]+vy*t) for t in range(-14000, 14001, 200)]
    # old belt crest from any high-age belt
    old_prof = None
    for s2 in [central.seed, central.seed+"-A", central.seed+"-B", central.seed+"-D"]:
        f2 = MacroField.build(s2)
        for b in f2.belts:
            cc = controls_at(s2, b.cx, b.cy)
            if cc.age > 0.6 and f2._belt(b, b.cx, b.cy, cc) > 300.0 and f2._alpine_energy(cc) < 0.2:
                a2 = b.azimuth_deg*math.pi/180.0
                vx2, vy2 = -math.sin(a2), math.cos(a2)
                old_prof = [macro_z(central, f2, b.cx+vx2*t, b.cy+vy2*t) for t in range(-14000, 14001, 200)]
                break
        if old_prof:
            break
    hlo = min(min(yv), min(old_prof) if old_prof else 0)
    hhi = max(max(yv), max(old_prof) if old_prof else 1)
    half_h = cmph//2 - 10
    def cpx(k, n2): return int(k/(n2-1)*(cmpw-1))
    def cpy(z, y0): return y0+half_h - int((half_h-8)*(z-hlo)/max(1.0, hhi-hlo))
    dc.line([(cpx(k, len(yv)), cpy(z, 8)) for k, z in enumerate(yv)], fill=(255, 90, 60), width=2)
    if old_prof:
        dc.line([(cpx(k, len(old_prof)), cpy(z, cmph//2+2)) for k, z in enumerate(old_prof)],
                fill=(150, 190, 150), width=2)
    dc.rectangle([0, 0, cmpw, 16], fill=(0, 0, 0))
    dc.text((4, 2), "MATCHED CROSS-SECTIONS (28km) — young alpine summit (red, SHARP) vs "
                    "old belt crest (green, ROUNDED); both from structure, not blur", fill=(240, 240, 240))

    pad = 8
    W = hs.width
    H = hs.height + prof.height + cmp.height + pad*4
    sheet = Image.new("RGB", (W, H), (10, 10, 12))
    title = Image.new("RGB", (W, 22), (10, 10, 12))
    ImageDraw.Draw(title).text((4, 4), "MV3.C — alpine peak / ridge hierarchy: dominant summit + secondary "
                                       "peaks + saddles + prominence, young sharp vs old rounded (authority)",
                               fill=(255, 255, 255))
    y = 0
    for im in (title, hs, prof, cmp):
        sheet.paste(im, (0, y)); y += im.height + pad
    out = DOCS / "provenance_mv3c_peak_hierarchy.png"
    sheet.save(out); print("wrote", out, "seed", sd, "prominence", round(rp["prominence"]))


if __name__ == "__main__":
    main()
