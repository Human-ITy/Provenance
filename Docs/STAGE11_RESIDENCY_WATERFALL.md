# Stage 11 Residency Waterfall

This capture-free diagnostic attributes the cost of replacing the live Stage 11 terrain neighborhood. It is a performance investigation, not a new geology stage and not a replacement for the cardinal correctness certificate.

Run `CERT_STAGE11_RESIDENCY_WATERFALL.cmd` after building Release x64 for the
eastward reference route, or `CERT_STAGE11_RESIDENCY_WATERFALL_ALLDIR.cmd` for
the permanent all-direction gate, which runs the same route in a fresh process
for each bearing and reports a combined verdict.

The route warms the origin, then travels 192 m along the selected bearing
through the live player loop:

- 64 m walking at 1 m per frame;
- 64 m sprinting at 2 m per frame;
- 64 m free-flying at 4 m per frame;
- 90 settled frames at the outer station.

No screenshot, framebuffer readback, digest, or explicit GPU synchronization occurs during the measured route. The receipt separates:

- residency and authority-cell generation;
- package discovery and retirement scans;
- relief/surface mesh compilation;
- fault/material queries;
- display-list allocation and compilation;
- package publication;
- latent far-field reconstruction and its surface/material queries;
- collision queries, drawing, presentation, and pacing;
- frame-thread versus off-thread build ownership.

Outputs:

- `Docs/provenance_stage11_residency_waterfall_<bearing>.txt`
- `Docs/provenance_stage11_residency_waterfall_trace_<bearing>.csv`
- `Docs/provenance_stage11_residency_waterfall.txt` (eastward reference copy,
  kept at its established path)
- `Docs/provenance_stage11_residency_waterfall_alldir.txt` (one summary line per
  bearing; owned and truncated by the all-direction command)

The first milestone gate passes in all four directions: no measured frame in any
bearing exceeds 100 ms. The second gate is close but **not stable** — worst
frames sit between 46 and 62 ms across bearings and repeat runs, so a single
eastward run reporting under 50 ms should not be read as a passed gate. The
33.3 ms target, 16.67 ms final gate, bounded publication, and cold warmup remain
open. Bare-earth macro geography and Stage 12 remain blocked until residency
publication is bounded without changing Stage 5-11 authority, geometry,
material, collision, or return determinism.

These are optimization milestones only. The gameplay acceptance gate is
`frames_over_16_667=0`; the current all-direction traces fail it in every
bearing. A route that merely avoids 50 or 100 ms stalls is not certified as
smooth or as sustained 60 FPS.

## First captured result

The first capture-free run isolated 24 frames over 100 ms, one at every 8 m far-field anchor transition. Median frame-thread time remained 0.903 ms, but p99 was 145.965 ms and the worst frame was 155.502 ms.

The dominant cost was rebuilding the complete 384 m latent far field on the frame thread: 2,672.185 ms across 24 rebuilds. Exact live-package construction contributed 457.261 ms, including 395.816 ms in display-list allocation. Residency and authority-cell generation contributed 43.891 ms and 30.020 ms respectively. Capture/readback/digest cost was zero.

Therefore the first scheduling target is the monolithic far-field rebuild, followed by legacy display-list allocation. Geological semantics and ordinary collision queries are not the observed blocker. The Stage 11 authority certificate was rerun after instrumentation and remains PASS.

## Incremental-tile result

The far field now uses persistent absolute 32 m coarse tiles and 8 m stitch
tiles. Overlap is retained across an 8 m anchor shift; only changed fringe tiles
are compiled. Display-list IDs are reused from an aged bounded pool, cache
retirement runs after the shift frame, and exact surface/material lookahead uses
ordinary-frame headroom.

Two consecutive runs reported:

```text
mean frame-thread time        11.047 / 11.838 ms
median                         8.389 / 8.959 ms
p95                           38.919 / 40.438 ms
p99                           43.241 / 42.934 ms
worst                         47.820 / 47.833 ms
frames >50 ms                      0
frames >100 ms                     0
far-field total              360.755 / 393.436 ms
far tile compile             347.345 / 379.551 ms
far surface queries            9,696
far material queries          27,198
correctness                      PASS
gameplay 60 FPS gate                 FAIL
optimization milestone        LT_50_PASS
```

The permanent 32-case cardinal audit also passes after this change: every
Stage 0/5/6/7/8/9/10/11 direction completely evicts the origin at 192 m,
returns with exact geometry/material/collision/package/image parity, and reports
zero sky or fallback-green pixels. Current Stage-11 north/east/south/west images
were manually inspected and show no live/stitch sky crack.

That 32-case audit must not be used as a pacing certificate: it includes cold
stage construction and image work, and its Stage-11 worst-frame values remain
hundreds of milliseconds (with the first cold north case above two seconds).

## All-direction result

Extending the route to all four bearings did not merely confirm the eastward
number — it exposed a directional defect and corrected the recorded gate.

The first all-direction run reported:

```text
bearing   median    p99      worst    >50 ms
north      8.746   40.205   55.712        1
east       8.502   40.999   52.607        1
south      1.305   66.753   80.917        6
west       1.345   68.624   82.725        6
```

Westward and southward traversal cost roughly 60 % more at p99, with a
conspicuously low median. The far-field lookahead inferred travel direction from
the player's offset inside the anchor cell; because the anchor is floor-snapped,
that offset is non-negative on both axes and the lookahead predicted +X/+Y
unconditionally. Travelling west or south it prefetched ground the player had
just left, which was already cached — so the budget was never spent (cheap
ordinary frames, low median) and every anchor shift paid the full unassisted
cost. Direction is now derived from observed player motion, which additionally
provides a bearing before the first anchor crossing, so the first shift of a
traversal is assisted too.

After the fix:

```text
bearing   median    p99      worst    >50 ms
north      8.909   41.442   45.978        0
east       9.741   42.951   48.234        0
south      9.309   46.380   61.941        2
west      10.038   42.553   46.788        0

directions_measured        4
directions_correct         4
directions_under_100ms     4
alldir_performance_gate    ALLDIR_FIRST_GATE_LT_100_PASS
```

p99 is now uniform across bearings (41.4-46.4 ms), where it previously ranged
from 40.2 to 68.6 ms. Three repeat southward runs returned worst frames of
52.709, 49.496 and 46.128 ms, so the remaining southward outlier is run
variance straddling the 50 ms boundary, not a directional defect. The honest
reading is that the sub-50 ms gate is marginal rather than passed: the
all-direction gate stands at `ALLDIR_FIRST_GATE_LT_100_PASS`.

Correctness is unaffected in every bearing: zero ground failures, zero collision
mismatches, and 24 far-field shifts building an identical 2,430 tiles per route
regardless of direction.

## Remaining work

This is not acceptable traversal performance. Tile derivation, lookahead, and
OpenGL display-list compilation still execute on the frame thread; cold initial
warmup is about 2.2 seconds.

The permanent 8/16/32/64 m shift-scaling receipt now exists — see
`STAGE11_SHIFT_SCALING.md` and `CERT_STAGE11_SHIFT_SCALING.cmd`. It localises
the remaining per-shift floor to the 0.5 m stitch ring, whose tile content is
anchor-relative and therefore recompiles almost entirely on every shift while
the coarse field scales correctly with the entering fringe. Making the stitch
ring's content anchor-invariant is the next target, followed by immutable CPU
tile work with bounded publication.
