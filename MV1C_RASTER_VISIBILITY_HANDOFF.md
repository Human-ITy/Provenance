# MV1.C — Real 32 km Raster Visibility

MV1.C answers the one question the original MV1 certificate could not: does the
certified 32 km derived terrain actually **rasterize to the framebuffer**, or was
it built and submitted but clipped? A projection audit found the active render
far plane was **600 m** — so MV1's regional (2–20 km) and horizon (20–32 km)
bands were built, retained, and GPU-timed, yet clipped before framebuffer
contribution. MV1.C restores real 32 km visibility **without touching the frozen
near-depth path.**

**Scope:** presentation only. MV1 geography, adaptive tessellation, error
targets, band structure, and MW1–MW8 authority are unchanged. MV1.D / MV2 / MW9
stay closed.

## Architecture — two-pass depth split

```
FRAME
  clear color + depth
  sky backdrop

  FAR TERRAIN PASS         projection near≈128 m / far≈33 km
    draw MV1 meso/regional/horizon (same MW composite authority)
        ↓
  clear DEPTH ONLY  (color kept)
        ↓
  FROZEN NEAR PASS         projection 0.03–600 m  (UNCHANGED)
    draw 192 m authoritative world + far field
    → overpaints the far terrain wherever near geometry exists
```

The near renderer keeps exactly the depth precision it was certified with; only
the projection differs between passes, and the camera/modelview is shared. Coarser
MV1 bands are z-biased below finer terrain so overlaps never fight.

## Proof — framebuffer pixel contribution, not submitted geometry

`CERT_MV1C_RASTER_VISIBILITY.cmd` → `--cert-mv1c-raster-visibility` →
`Docs/provenance_mv1c_raster_visibility_cert.txt`. The far-pass depth buffer is
read (before the depth-only clear) and terrain pixels are binned by **real camera
distance** into meso/regional/horizon. A submitted-but-clipped band contributes
zero pixels here.

```
MV1C_RASTER_VISIBILITY PASS
agg_meso_px       967668   (5/5 stations)
agg_regional_px   328769   (5/5 stations)   <- was 0 (clipped) before MV1.C
agg_horizon_px      7331   (2/5 stations)   <- was 0 (clipped) before MV1.C
near_pass_frustum 0.03..600 m (frozen, unchanged)
```

Station images kept as visual evidence: `Docs/provenance_mv1c_station{0..4}_*.ppm`.

## Tile ownership — persistent pooled VBOs

MV1 far tiles moved from legacy display lists to persistent, pooled vertex
buffers: CPU builds a derived tile → uploads into a recycled GPU buffer → draws →
returns the id on retire (aged 100 ms so the driver is done consuming it). No
per-tile `glNewList/glEndList` finalization lifecycle. Gives explicit GPU byte
accounting and bounded ownership:

```
vbo_resident_bytes ~4.7 MB   high-water bounded
vbo reuse pool     recycled ids (no continuous create/destroy while travelling)
runtime allocs after warmup   small, bounded
```

Also retained: a hard per-frame build cap (≤6 tile creates/frame) and a
behind-camera cull, both kept as genuine improvements.

## Present-path owner attribution (the hard part)

The 90 s Test-B soak intermittently spiked (56–1339 ms). A full controlled
investigation was run per the ruling — **do not waive a probe, do not mutilate a
renderer, attribute first.** Findings:

- **The VBO conversion did not change the spike** (same ~1/6 frequency and
  magnitude as display lists) → **tile storage/finalization is not the owner.**
- **`glFinish` is protective, not the artifact.** Removing the pre-present
  `glFinish` made spikes *worse* (SwapBuffers pileups 58–98 ms). Case A disproved.
- **Every spike frame did zero MV1 work**: `mv1_tiles_built=0`, `upload_ms=0`,
  `depth_clear≈0.001 ms`, async MV1 GPU = 0.008 ms. VBO upload and the two-pass
  depth clear (Case C) exonerated directly.
- **Decisive control — production travel gate, 5 runs each:**

  ```
  MV1 ON    gate fails 2/5   worst travel 51.3 ms
  MV1 OFF   gate fails 3/5   worst travel 61.2 ms
  ```

  Disabling the entire far renderer does **not** improve the failure rate — it is
  slightly worse. The stock MW8 soak, with no MV1 at all, fails the historical
  0-over-16.667 travel gate ~3/5 on this hardware. The earlier "passing" soaks
  were the lucky ~40–50 %.

Diagnostic harness (kept): `--cert-mv1c-soak-diag` logs per-outlier A cpu / B swap
/ C glFinish / D async-MV1-GPU + MV1 context to
`Docs/provenance_mv1c_soak_outliers.csv`; `--soak-no-glfinish` toggles the probe.

**Conclusion: MV1.C is not the owner of the soak/Test-B jitter.** It is baseline
present-path / SwapBuffers behavior on this machine, independent of terrain
architecture. Test A's movement sub-gate inherits the same baseline jitter
(one 16.851 ms movement frame in one run; correctness always exact, 3/3 clean on
re-run).

## Certified state

```
actual 32 km raster visibility           PASS  (meso/regional/horizon pixels > 0)
frozen 0.03–600 m near depth path        PASS  (unchanged)
MV1 terrain / H2H                        PASS
MV1.G full-raster async GPU              PASS  ~0.036–0.47 ms, no stalls (rerun after MV1.C)
Test A correctness (exact return)        PASS 4/4 (digests/geometry/material/collision/image)
MW8 analytical / visual                  PASS
VBO ownership / resources                bounded
MV1.C non-regression / owner attribution PASS
```

**Not claimed green:** the 90 s Test-B travel gate. It is reopened as a **global
present-pacing** issue because the frozen baseline itself no longer reliably meets
the historical zero-overrun gate on this hardware — a separate infrastructure
investigation (SwapBuffers / vsync / driver pacing / pre-present glFinish /
frame-time definition), not terrain architecture, and not blocking MV1.C.

## Historical corrections

- **MV1:** originally certified 32 km derived *geometry*, but the active 600 m
  projection **clipped regional/horizon framebuffer visibility**. MV1.C introduces
  the split depth-range presentation and is the first certificate proving actual
  32 km raster visibility.
- **MV1.G:** original timing was collected while much of the far terrain was
  clipped. MV1.G was **rerun after MV1.C** and remains green (~0.47 ms, no stalls)
  with the full 32 km rasterized.

## Runtime

```
CERT_MV1C_RASTER_VISIBILITY.cmd   --cert-mv1c-raster-visibility
                                  --cert-mv1c-soak-diag [--soak-no-glfinish] [--mv1-off]
```

## Board

```
MW1–MW8                         CERTIFIED
MV1 geometry / derivation       CERTIFIED
MV1.C real 32 km raster view    CERTIFIED
MV1.G full-raster GPU           CERTIFIED

GLOBAL TEST-B PRESENT PACING
baseline jitter                 OPEN

MV1.D distance readability      WAITING (on present-pacing)
MV2 100+ km horizon             CLOSED
MW9 flora / fauna               CLOSED
```
