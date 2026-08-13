# Stage 11 Far-Field Shift Scaling

This is the permanent receipt for the constitutional requirement of the
incremental far-field cut:

> An 8 m far-field anchor shift may only cause work proportional to the newly
> exposed fringe, not to the complete 384 m latent field.

A single shift distance cannot prove that. A monolithic rebuild and a correctly
incremental one both report "one expensive frame per anchor shift"; they differ
only in how that cost responds when the shift gets larger. This certificate
therefore walks the same window across four shift classes and normalises the
measured work per metre of shift.

- Constant work **per shift** is the monolithic signature.
- Constant work **per metre** is the incremental signature.

Run `CERT_STAGE11_SHIFT_SCALING.cmd` after building Release x64.

## Route

The probe warms the origin in the Stage 11 fault-displacement view, then
performs four shifts each at 8, 16, 32 and 64 m eastward, holding station for
eight settled frames between shifts. Every shift frame is attributed
individually: the far-field counters are snapshotted at the end of each frame,
so the receipt reports the work caused by that one anchor crossing rather than a
route average. No screenshot, readback, digest, or explicit synchronization
occurs during measurement.

Ground and collision parity are checked at every station, exactly as in the
residency waterfall. The exit code reports measurement integrity; the `scaling=`
line carries the verdict, so an open gate is not reported as a crash.

## Reading the receipt

Per class:

```text
far_ms_per_shift          frame-thread far-field cost of one anchor crossing
far_ms_per_m              the same cost normalised by shift distance
tiles_built_per_shift     absolute tiles recompiled for that crossing
tiles_built_per_m         the same count normalised by shift distance
resident_window_fraction  tiles rebuilt as a fraction of the resident window
settle_prefetch_ms        lookahead cost paid on ordinary frames between shifts
```

Verdict lines:

```text
tiles_per_m_spread          max/min of tiles_built_per_m across the four classes
far_ms_per_m_spread         max/min of far_ms_per_m across the four classes
small_shift_window_fraction share of the resident window an 8 m shift rebuilds
scaling                     PROPORTIONAL_TO_FRINGE_PASS when both spreads are
                            tight and the 8 m shift stays inside the collar
```

## Current result

```text
resident far tiles                    753  (609 coarse, 144 stitch)
ground failures / collision mismatches  0

class      far ms/shift   tiles/shift   coarse   stitch   window fraction
   8 m           23.604        101.25    20.25    81.00            0.1345
  16 m           21.666        113.50    28.50    85.00            0.1507
  32 m           39.752        138.00    45.00    93.00            0.1833
  64 m          113.487        187.00    78.00   109.00            0.2483

tiles_per_m_spread            4.332
far_ms_per_m_spread           2.375
small_shift_window_fraction   0.1345
measurement_complete          PASS
bounded_fringe                PASS
scaling                       OPEN
```

`bounded_fringe` passes: an 8 m shift touches 13.45 % of the resident window, so
the retained absolute-tile scheme is genuinely retaining. The old monolithic
path would report 100 % here.

`scaling` is open, and the per-class breakdown says exactly why. Splitting the
rebuilt tiles by kind:

- **Coarse 32 m tiles scale correctly.** An 8 m shift rebuilds 20 of 609 coarse
  tiles (3.3 %); a 64 m shift rebuilds 78 (12.8 %). That is close to the
  geometric expectation for an entering fringe and it grows with the shift.
- **The 8 m stitch ring does not scale at all.** An 8 m shift rebuilds 81 of 144
  stitch tiles — 56 % of the ring — and a 64 m shift rebuilds only 109. The
  stitch cost is very nearly a constant per shift, independent of distance.

The cause is that a stitch tile's content signature depends on the blend from
the exact live boundary to the coarse far field, and that blend is measured
relative to the current presentation bounds. When the anchor moves, every stitch
tile's blend field changes even though the ground beneath it did not, so the
tile is correctly judged stale and correctly recompiled. The tiles are absolute
in position but their *content* is anchor-relative.

This fixed stitch floor, not the coarse field, is what holds worst-frame
traversal time near 45 ms. Making the stitch ring's content anchor-invariant —
so that a stitch tile is only rebuilt when it actually enters the ring or
changes kind — is the next scheduling target, and it is what this receipt exists
to measure.

## Lookahead direction defect found by this receipt

The first run of this certificate reported 44.654 ms per 8 m shift. The
all-direction waterfall then showed westward and southward traversal costing
roughly 60 % more than northward and eastward traversal, with a suspiciously low
median frame time.

The far-field lookahead inferred travel direction from the player's offset
inside the anchor cell. The anchor is floor-snapped, so that offset is
non-negative on both axes: the lookahead predicted +X/+Y unconditionally. Moving
west or south, it prefetched ground the player had just left — already cached, so
the budget was never spent, the ordinary frames looked cheap, and every anchor
shift paid full price. Direction is now tracked from observed player motion,
which also yields a usable bearing before the first anchor crossing.

After the fix, the 8 m class cost fell from 44.654 ms to 23.604 ms per shift and
the four traversal bearings converged. See `STAGE11_RESIDENCY_WATERFALL.md`.

## Scope

This is a performance receipt. It changes no geology, material, FeatureId,
collision, or surface authority. The 32-case cardinal replacement certificate
was rerun after the lookahead fix and remains 32/32 PASS with exact return
parity and zero sky or fallback-green pixels. Bare-earth macro geography and
Stage 12 remain blocked.
