# MV1 Below-Horizon Coverage Holes — Attribution & Fix

Player free-fly screenshots showed **blue sky punched through the live terrain
surface** and **exposed dark-brown skirt/sidewall geometry** — and, decisively, the
holes changed with **camera rotation at a fixed position**. This is a renderer
coverage defect, not geography. Fixed at the representation/stitching layer only; no
authority, macro, palette, fog, or camera changes.

## Attribution (measured, in order)

A fixed-position 360° yaw-sweep instrument (`--cert-mv1-coverage`) measures, per yaw:
resident tile count + order-independent digest, authority digest, and below-terrain
enclosed-sky holes (flood-fill from the open sky; count connected components ≥40 px).

| Suspect | Test | Result |
|---|---|---|
| new horizontal frustum cull | `--mv1-nocull` A/B | **EXONERATED** — holes identical cull ON vs OFF |
| residency tied to view direction | resident digest across yaw | **EXONERATED** — resident_set_delta=0, digest stable under yaw |
| MV2.B three-pass / depth-clear | `--mv2b-off` A/B | **EXONERATED** — same holes on/off (on/off frames pixel-identical) |
| **MV1 LOD-ring underlap at band boundaries** | code + fix | **OWNER** |

**Owner:** the MV1 desired-tile set included a tile only if its **centre** distance
fell in the band's `[inner,outer)` annulus. But a tile *covers* ±tileM/2 around its
centre. At a band boundary a point could land in a finer-band tile whose centre was
just **beyond** `outer` (excluded) **and** a coarser-band tile whose centre was just
**inside** `inner` (excluded) — owned by **neither band → a sky hole**, with the
coarser tile's perimeter **skirt** (colour `(0.30,0.28,0.26)` = the dark brown seen)
exposed. The gap is direction- and elevation-dependent (worst looking down from
altitude), which is exactly why rotating in place exposed and hid it.

## Fix (representation/stitching only)

The desired-set test now includes a tile if its **AABB overlaps the band annulus**
(nearest AABB point < `outer` AND farthest corner ≥ `inner`), with loop bounds padded
one tile. A boundary-straddling tile is therefore resident in **both** adjacent bands;
the existing per-band z-bias layers the coarser tile under the finer in the overlap,
so coverage is gap-free with no z-fight. Same MW authority sampled; only which tiles
are resident/stitched changed. Resident tiles rose 620 → 728 (bounded ring, no
travel-history growth; retires unchanged).

## Verification

```
--cert-mv1-coverage  (fixed pos, 360 deg yaw):
  resident_set_delta_under_yaw = 0        PASS
  resident_digest_stable       = YES      PASS
  authority_digest_stable      = YES      PASS
  below_terrain_sky_holes      = 0        PASS   (was 23158 before the fix)
```

Before/after at the worst yaw: `Docs/provenance_mv1_hole_fix_before_after.png` — big
blue surface cutouts + green underlap patches → continuous terrain. The residual
enclosed-sky was ≤19 px LOD silhouette stair-step pockets (coarse-mesh edge aliasing,
not coverage holes); excluded by the ≥40 px component threshold.

Frozen gates all still green after the fix:
- MV1 station cert (H2H identity, seam_failures=0) PASS
- MV1.C raster visibility (meso/regional/horizon) PASS
- MV2.B horizon (128 km raster, 32 km seam, cardinal) PASS
- MV2.C presence PASS
- **Test B 24 m/s gameplay hard gate PASS** (engine 0-over, 0 stage-owned, cadence no-regression)

## Diagnostic instruments added (measurement only; gated behind flags)

- `--cert-mv1-coverage` — fixed-position yaw-sweep; resident invariants + enclosed-sky
  holes; exit 0 iff all invariants hold.
- `--mv1-nocull` — disables the horizontal frustum cull (attribution A/B for MV1 + MV2 macro draws).

## NOT changed

MW1–MW8 authority, MV2.A/A2 macro authority, macro relief, biome/palette, aerial fog,
cameras. Above-horizon geographic negative space (the earlier, legitimate distant-basin
sky) is untouched. MV2.C / MW9 remain as before.

## Board

```
MV1 live-terrain coverage        FIXED + CERTIFIED (band-boundary AABB stitch;
                                 rotation-invariant, 0 below-terrain holes)
```
