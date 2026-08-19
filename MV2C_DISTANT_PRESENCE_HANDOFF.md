# MV2.C — Distant-Terrain Presence (presentation only)

A narrow **presentation-only** cut: make the existing MV2 authority *read* correctly at
distance — the far ranges were technically visible but washed to near-sky, reading as a
thin floating strip. MV2.C improves legibility **without changing the authority, macro
relief, region cadence, or the 1 km page step.** `--mv2c-off` reproduces the exact
frozen MV2.B presentation.

## The problem (isolated)

The user's neon-green annotation isolated it: near/mid terrain → a large open-sky
negative space → a very thin, pale distant massif. The negative space is legitimate
line-of-sight (MV2.B fixture 11: 0 coverage holes). The defect was purely presentation:
the far range had **too little visual mass and contrast** — it washed into the pale sky.

## What changed (presentation only)

1. **Gentler 32–128 km aerial tail** — `GL_EXP` density 1.85e-5 → **0.98e-5**, applied
   to both far passes (still one continuous physical-distance curve, no band switch at
   32 km). T(128 km) rises 0.09 → 0.32, so distant silhouettes keep real mass.
2. **Coarse land-cover palette** — replaces the diagnostic green→tan→**white/grey**
   ramp with muted **vegetated / dry-substrate / exposed-rock** by elevation *and*
   slope (steep faces expose rock). All tones kept **darker than the sky** so ranges
   read as solid masses; bare high ground is muted rock, **never called snow**.
3. **Stronger silhouette-preserving hillshade** — higher lit/shadow contrast so large
   macro slopes have readable volumetric form.

Colours are baked into the macro VBOs at build time and the fog density set at render
time; `--mv2c-off` rebuilds the exact MV2.B presentation.

## A/B — `Docs/provenance_mv2c_ab_comparison.png`

| Metric | MV2.B baseline | MV2.C | verdict |
|---|---|---|---|
| far 50–128 km separation from sky | **0.007** (washed, ~invisible) | **0.049** (~7×) | reads as terrain — **fixture 13 PASS** (gates MV2.C only) |
| far 50–128 km mean luma (sky = 0.603) | 0.596 | 0.553 (darker) | separated from sky |
| distance-class px 32-50/50-80/80-100/100-128 | 79388/68898/34664/47366 | **identical** | coverage unchanged (depth-based) |
| MV2 coverage holes ≥32 km | 0 | **0** | negative space preserved (not eliminated) |
| cardinal N/E/S/W 128 km coverage | 4/4 | 4/4 | unchanged |
| 32 km seam / MV1→MV2 palette switch | — | none | one continuous aerial curve; earth-tone palette matches MV1 |
| gameplay 24 m/s Test B | PASS | **PASS** (engine 0-over, 0 stage-owned, cadence no-regression) | hard gate |

`--mv2c-off` == frozen MV2.B (the else-branch is the untouched MV2.B code; the
`--cert-mv2b-horizon` baseline runs it and reproduces the MV2.B receipt exactly).

## The honest limit (geometry, not presentation)

MV2.C makes the far ranges **legible, darker, and separated from sky** — the presentation
mandate. It does **not** make them tall: the ranges remain low-**profile** at these
central viewpoints because the macro **relief** is genuinely low there at the 1 km step.
That is the *geometry/resolution* question the ruling reserved for later, and MV2.C did
**not** touch it (no taller amplitudes, no finer step, no authority change). Answer to
the decision tree: better presentation makes the existing 128 km terrain read as real
distant terrain; whether specific ranges should be *taller* is a separate authority/relief
decision — deferred, not made here. (Also note: the pale near foreground in the elevated
central views is **MV1's frozen 0–32 km terrain**, outside MV2's scope.)

## Runtime

```
Play:  PLAY_MV2B_HORIZON_RENDERER.cmd            (MV2.C on by default; --mv2c-off for frozen MV2.B)
Cert:  CERT_MV2C_PRESENCE.cmd                    (--cert-mv2c-presence)
Base:  CERT_MV2B_HORIZON_RENDERER.cmd            (--cert-mv2b-horizon; MV2.B baseline, mv2c off)
Gate:  ProvenanceClient.exe --cert-streaming-soak-mw8 --mv2b-on   (Test B, MV2.C on)
```

## Explicitly NOT done

No change to MV2.A authority / macro field / amplitudes / region cadence / 1 km step;
no snow / vegetation invented; no MV1 change; MW9 / weather / glaciers closed. If a
finer macro step or taller relief is ever wanted, that is a separate authority cut,
justified by more than one horizon view.

## Board

```
MV2.A / MV2.A2 regional macro authority   CERTIFIED
MV2.B 100-128 km horizon renderer         CERTIFIED
MV2.C distant-terrain presence            CERTIFIED (presentation only; far ranges read
                                          as terrain, ~7x more separated from sky;
                                          geometry/relief untouched, deferred)
```
