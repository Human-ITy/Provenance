# MV1.D — Distance / Depth Readability (continuous aerial perspective)

Player-facing question: does the frozen 32 km MV1 geography **read as kilometres
of depth**, or as flat stacked cutouts? MV1.D adds a **presentation-only**,
continuous aerial-perspective attenuation so a 2 km ridge, a 12 km ridge and a
25 km massif separate by distance — without touching geometry, the two-pass depth
split, VBO ownership, the 32 km visibility, MW1–MW8, or the frozen 0.03–600 m
near path.

## What it does

Every far-pass fragment is attenuated toward the **exact horizon sky colour**
(`0.45, 0.62, 0.88`) by a deterministic transmittance that depends only on the
**actual camera-to-surface distance** `d`:

```
T(d) = exp(-(rho * d)^2)          rho = 4.30e-5 per metre   (GL_FOG, GL_EXP2)
```

Because `T` is a function of distance **alone**, there is **no hard colour
switch at the LOD band edges** (192 m / 2 km / 20 km) — those edges are ordinary
interior points of a smooth curve. Representative values:

```
T(600 m)=0.999   T(2 km)=0.993   T(12 km)=0.766   T(28 km)=0.235
```

So near/meso stay crisp, regional is half-blended, and the horizon is strongly
atmospheric but **never fully erased** (still rasterised).

## Where it is applied (and where it is NOT)

- Applied **only inside the MV1.C far pass** (128 m–33 km frustum), wrapped as
  `Mv1dSetFog(true) … DrawMv1MultiScaleTerrain() … Mv1dSetFog(false)`.
- The **frozen 0.03–600 m near authoritative world** (drawn after the depth-only
  clear) is **never fogged**. MV1.C geometry, depth precision, and persistent
  pooled VBO ownership are byte-identical.
- `--mv1d-off` skips the fog entirely and **reproduces the frozen MV1.C
  presentation byte-for-byte** (proven: `cmp -l` = 0 differing bytes vs the
  committed MV1.C station image; two `--mv1d-off` runs share one SHA-256).

## Proof (cert `--cert-mv1d-distance-readability`)

Two independent legs, `Docs/provenance_mv1d_distance_readability_cert.txt`:

1. **Analytical continuity / monotonicity** (`Mv1dContinuitySelfTest`) — a dense
   4 m scan 0..33 km proves `T` is strictly non-increasing with a max step jump
   of `1.5e-4`; explicit probes 0.5 m either side of each LOD edge (192 m / 2 km /
   20 km) show a max discontinuity of `3.5e-5` → **no popping**; the four
   representative band distances are strictly ordered
   (`T(near)>T(meso)>T(regional)>T(horizon)`) and the horizon stays visible
   (`T>0.02`).
2. **Framebuffer band measurement over the five frozen stations** — the far-pass
   colour+depth is read back and binned by **real camera distance** into
   near / meso / regional / horizon. Aggregated over the stations that see each
   band:

   ```
   band            mean_contrast   mean_blend   agg_px
   meso            0.0662          0.441        243630
   regional        0.0489          0.554        152192
   horizon         0.0221          0.571          7331
   ```

   **Contrast falls** meso→regional→horizon and **atmospheric blend rises** with
   distance, while regional and horizon remain rasterised (matching MV1.C's
   horizon pixel count). Per-station numbers are noisier (a valley station sees
   more mid-distance structure than an enclosed flank) — the **aggregate over the
   stations** is the honest physical claim and it is monotone.

Station images (visual contact sheet, on/off comparison uses the identical
deterministic cameras as MV1.C):
`Docs/provenance_mv1d_station{0..4}_<name>.ppm`.

## Full gauntlet (all green, in order)

| # | Check | Result |
|---|-------|--------|
| 1 | PX2 classifier regression self-test | `px2_classifier_self_test=PASS` |
| 2 | MV1.D analytical + per-band fixture | `MV1D_DISTANCE_READABILITY PASS` |
| 3 | Five-station visual contact sheet | rendered (subtle, artifact-free) |
| 4 | MV1.C band-pixel visibility (fog on) | `PASS` (meso/regional/horizon rasterised) |
| 5 | MV1.G full-raster GPU (fog on) | `PASS` (bounded draw, no stalls) |
| 6 | Test A cardinal (MW8, PX2 gate) | `PASS 4/4` — engine 0-over, 0 stage-owned |
| 7 | Test B 90 s soak (MW8, PX2 gate) | `PASS` — 0 presented / stage-owned misses, cadence no-regression |

`--mv1d-off` byte-reproduction of frozen MV1.C: **verified**.

## Semantic honesty

- Distant alpine **white is the diagnostic material, not snow**; MV1.D only
  attenuates its colour toward sky by distance and **does not hide the
  diagnostic→near-material handoff** (the near path is unfogged).
- This is **not causal weather.** A later, separate cut may drive `rho` from MW6
  hydroclimate (dry interior = long visibility, humid valley = stronger
  attenuation). MV1.D is a fixed, deterministic transmittance only.

## Runtime

```
Cert:   Build\x64_Release\ProvenanceClient.exe --cert-mv1d-distance-readability
Off:    add --mv1d-off  (reproduces frozen MV1.C; fog disabled)
Script: CERT_MV1D_DISTANCE_READABILITY.cmd
```

## Board after MV1.D

```
MW1–MW8                         CERTIFIED / FROZEN
MV1 / MV1.C / MV1.G             CERTIFIED
PX1 present-pacing attribution  CERTIFIED
PX2 two-contract gate           CERTIFIED
MV1.D distance readability      CERTIFIED  (continuous aerial perspective; far
                                pass only; --mv1d-off == frozen MV1.C)
MV2 extended 100+ km horizon    CLOSED
MW9 flora / fauna               CLOSED
```

Next per the plan: the retrospective MW performance sweep under the PX2 gate.
