# Stage 11 Far-Field Incremental Residency Handoff

Status: active performance-integration cut

Repository: `C:\Users\D-Day\ProvenanceEsoterica`

Date: 11 August 2026

## Correction-only update — 13 August 2026

This update supersedes the performance attribution and next-action ruling below; the historical far-field receipts remain useful controls.

The active blocker is **bulk live-package construction plus incomplete residency**, not a meaningful population of cold-package tails. With far-field presentation disabled, the corrected `r=192 m` distribution is:

```text
packages measured                   1,520
bulk population                     1,518
package p50 / p95 / p99      1.12 / 1.92 / 2.19 ms
bulk maximum                       4.89 ms
isolated external stalls     27.73 and 32.19 ms
authority + mesh                    74.3%
material resolution                 18.3%
GL compile                           4.3%
publication                         ~0.0%
```

The two large samples are single external stalls, not evidence of a cold population. `new_variants_total=2`; optimizing those tails is not the next cut.

The former free-flight frame pass was also invalid as a complete-world result. At `240 m/s`, residency remained incomplete while the builder held the frame gate:

```text
r=64 m    min/mean complete radius = 61.5 / 63.8 m    incomplete 20/188 frames
r=128 m   min/mean complete radius = 57.6 / 90.6 m    incomplete 173/188 frames
r=192 m   min/mean complete radius = 55.5 / 90.0 m    incomplete 188/188 frames
```

The performance gate must therefore report `FAIL_INCOMPLETE_RESIDENCY` whenever the declared live envelope is not delivered. High FPS while required packages remain absent is not a pass.

### Corrected next cut

Far-field presentation remains quarantined. Do not open Stage 12 or macro geography. Go directly to `EmitCrossingQuad` / `BuildBlock` and stop treating package construction as one indivisible sample-authority → resolve-material → emit-geometry operation.

Required package stages:

```text
1. sample authoritative fields into linear package-local arrays
2. derive crossing and surface descriptors
3. resolve material and identity fields in bulk
4. emit vertices and indices from resolved descriptors
5. derive collision from the same resolved geometry
6. publish
```

Do not add worker threads until this dataflow is separated and measured. Preserve geometry, material, FeatureId, collision, and authority digests; retain the `32/32` cardinal replacement result and `2,601` settled packages at `r=192 m`. Add `sample_ms`, `surface_descriptor_ms`, `material_ms`, `mesh_emit_ms`, `collision_ms`, and `publish_ms` receipts, then rerun `192 m / far off / 240 m/s`.

The objective of this cut is to make the package unit attributable and jobifiable without changing world truth. At the measured `1.12 ms/package`, sustaining complete `r=192 m` residency at `240 m/s` still requires roughly a `2.7×` throughput improvement or moving construction off the frame thread.

### Package split progress - 13 August 2026

The separation cut is implemented in the active folder. `BuildBlock` compatibility remains, but Stage 8-11 runtime construction now follows explicit stages:

```text
SampleBlock
DescribeBlock
bulk material resolution
EmitBlockMesh
resident collision-surface adoption
display-list allocation / compile / publish
```

The authoritative sample field is a linear `17 x 17` package-local array. Crossing descriptors are a linear 256-entry index array. Material sampling preserves the old two-triangle centroid expression and evaluation order. The render triangles and resident collision interpolation share the same immutable sampled field and fixed diagonal.

Current proof receipts:

```text
Stage 7 visible exposure cert                  PASS
Stage 8 differential erosion cert             PASS
Stage 9 granite intrusion cert                PASS
Stage 10 contact mineralization cert          PASS
Stage 11 fault displacement cert              PASS
cardinal replacement at live=192/far=0       32/32 PASS
settled packages at origin/outer/return        2,601
origin packages surviving outer station            0
sky/fallback pixels                                0
ground/collision/oracle mismatches                  0
return geometry/material/collision/image       exact
```

The initial post-split `192 m / far off / 240 m/s` route remained incomplete. Two exact-result optimizations followed: one prepared folded-geology column is reused inside each erosion surface integration, and presentation materials use a lightweight causal material/chronology query instead of constructing full strings, ancestry vectors, normals, and local coordinates at every quad. The Stage-11 certificate now directly compares the lightweight result with the full authority query across its complete sample volume. All five causal certificates remained green.

```text
mean frame-thread ms                          5.786
p99 / worst ms                       11.250 / 75.977
frames over 16.667 / 188                       2
frames with incomplete residency / 188         0
min / mean complete radius m        192.00 / 192.00
performance gate                FAIL_SUSTAINED_60_FPS

bulk package median ms                       0.229
bulk authority sample ms                     0.082
bulk surface descriptor ms                   0.001
bulk material ms                             0.070
bulk mesh emit ms                            0.002
bulk collision adoption ms                  <0.001
bulk GL compile ms                           0.048
```

At this historical checkpoint the complete 240 m/s gate was achieved, but the
strict 60 FPS gate remained red because two isolated stalls occurred. The
background-job update below supersedes these timing results.

```text
480 m/s   min/mean complete radius = 167.52 / 187.61 m   incomplete 35 frames
960 m/s   min/mean complete radius =  73.05 / 152.67 m   incomplete 59 frames
```

At that checkpoint there was no longer one overwhelming bulk stage. It supplied
the evidence for the background-job cut documented immediately below.

### Background package jobs and complete live-world gate - 13 August 2026

The job cut is now implemented. Four bounded workers build immutable CPU package
products (authority field, descriptors, lightweight material field, triangles,
and collision samples). OpenGL allocation, display-list compilation, and map
publication remain on the frame/render thread. Every job carries a stage epoch;
stage changes cancel queued work and stale completed products cannot publish.

CPU sampling no longer mutates the erosion dual-vertex caches. Causal one-metre
cell refreshes read the already-published package collision surface and the
certified lightweight material query, eliminating the repeatable 320 m
frame-thread reconstruction/cache-growth stall. The bounded cell map is also
reserved to its certified residency population.

The permanent `192 m / far off` free-flight ladder now reports:

```text
240 m/s   worst  6.334 ms   frames >16.667 = 0   complete 192/192   PASS
480 m/s   worst  5.969 ms   frames >16.667 = 0   complete 192/192   PASS
960 m/s   worst 11.498 ms   frames >16.667 = 0   min 191.50 m      FAIL_INCOMPLETE_RESIDENCY
```

The required 240 m/s gate is closed, and the 480 m/s diagnostic rung is also
closed. The 960 m/s rung remains honestly open: frame pacing passes, but a
half-metre completeness deficit occurs on 24 measured frames. Do not relabel it
as passed.

Current per-package receipt (`3,264` packages):

```text
median / p95 / p99          0.145 / 0.217 / 0.282 ms
bulk maximum                               0.843 ms
isolated cold allocation sample            1.450 ms
extreme packages                                0
authority + mesh                            35.8%
material                                    15.9%
GL compile                                  29.7%
allocation                                  18.5%
```

Stage-11 cardinal replacement was rerun in all four directions after the job
cut. Every case retained exactly `2,601` published packages, evicted every
origin package at the outer station, returned identical geometry/material/
collision/package/image digests, and reported zero sky/fallback pixels and zero
ground/collision/oracle mismatches. The traced walk/sprint/free-flight movement
frames top out at `7.263 ms`; larger receipt maxima occur outside the movement
trace during cold/settle/capture work and are not a traversal pass claim.

All Stage 7-11 semantic certificates remain green with unchanged digests:

```text
Stage 7 geometry       3dc3e3e3ffd959fc
Stage 8 geometry       d2f1c29c2fcaad9d
Stage 9 semantic       dee69fa95224a101
Stage 10 semantic      c019f93792b85a9e
Stage 11 partition     279658dd7c5cf6c1
Stage 11 return        b4076f3bb3eb2e24
```

Separate open defect: Stage 5-6 still use their older synchronous presentation
paths at `live=192`; the full ladder audit exposes multi-second cold/replacement
frames there. Their map correctness and return parity pass, but their performance
does not inherit the Stage 8-11 worker fix.

### Continue from this folder

Work only in `C:\Users\D-Day\ProvenanceEsoterica`; the worktree is dirty and must not be reset or cleaned. The active files for this cut are:

- `Code/Applications/ProvenanceClient/CausalVisibleExposure.h`
- `Code/Applications/ProvenanceClient/CausalDifferentialErosion.h`
- `Code/Applications/ProvenanceClient/CausalGraniteIntrusion.h`
- `Code/Applications/ProvenanceClient/CausalContactMineralization.h`
- `Code/Applications/ProvenanceClient/CausalFaultDisplacement.h`
- `Code/Applications/ProvenanceClient/Main.cpp`

Build with the existing `v143` override documented later in this handoff. The required performance command is:

```text
Build\x64_Release\ProvenanceClient.exe --cert-stage11-freefly --live-radius=192 --far-extent=0 --freefly-speed=4 --freefly-distance=512
```

The required replacement command is:

```text
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement --live-radius=192 --far-extent=0
```

Do not run the cardinal command without both explicit radius flags; its ordinary defaults are `64 m` live and `384 m` far and are not this control.

The next implementation decision is no longer “add package jobs”; that work is
complete for Stage 8-11. Close the remaining 960 m/s half-metre throughput gap
only if that extreme rung is promoted to a required gameplay gate. Otherwise,
the more important integration debt is migrating Stage 5-6 from their
synchronous presentation builders onto the same immutable job/publication law.
Preserve the five causal certificates and exact cardinal return audit. Do not
reopen far field, Stage 12, or macro geography from this handoff alone.

## Scope lock

Continue with far-field incremental residency and bounded publication. Do not open Stage 12 or bare-earth macro geography. Do not change geology, material, FeatureId, collision, or surface authority to improve timing.

The constitutional requirement for this cut is:

> An 8 m far-field anchor shift may only cause work proportional to the newly exposed fringe, not to the complete 384 m latent field.

## Proven baseline

The permanent capture-free commands are:

```text
CERT_STAGE11_RESIDENCY_WATERFALL_ALLDIR.cmd
CERT_STAGE11_SHIFT_SCALING.cmd
```

The waterfall runs the Release client with
`--cert-stage11-residency-waterfall[-<bearing>]`, warms Stage 11, then travels
192 m along the bearing through the live loop: 64 m walk, 64 m sprint, 64 m
free-flight, and an outer settle. The all-direction command repeats that in a
fresh process for each of north, east, south and west. The shift-scaling command
runs `--cert-stage11-shift-scaling`, an 8/16/32/64 m shift ladder that answers
the constitutional question directly. Neither performs any screenshot,
framebuffer readback, digest, or explicit synchronization during measurement.

First receipt:

```text
median frame-thread time       0.903 ms
p99                           145.965 ms
worst                         155.502 ms
frames >100 ms                 24
far-field shifts               24
far-field rebuild total      2672.185 ms
live package build total      457.261 ms
display-list allocation       395.816 ms
residency + generation         73.911 ms
capture/readback/sync/digest    0 ms
off-frame-thread builds         0
ground/collision failures       0
```

Every bad frame coincides with an 8 m far-field anchor shift. Stage 11 geology is not the recurring bottleneck. The complete 384 m latent presentation is synchronously regenerated and recompiled on the frame thread.

Artifacts:

- `Docs/provenance_stage11_residency_waterfall.txt` (eastward reference)
- `Docs/provenance_stage11_residency_waterfall_<bearing>.txt`
- `Docs/provenance_stage11_residency_waterfall_trace_<bearing>.csv`
- `Docs/provenance_stage11_residency_waterfall_alldir.txt`
- `Docs/provenance_stage11_shift_scaling.txt`
- `Docs/STAGE11_RESIDENCY_WATERFALL.md`
- `Docs/STAGE11_SHIFT_SCALING.md`
- `CERT_STAGE11_RESIDENCY_WATERFALL.cmd`
- `CERT_STAGE11_RESIDENCY_WATERFALL_ALLDIR.cmd`
- `CERT_STAGE11_SHIFT_SCALING.cmd`

The per-bearing trace CSV replaces the former
`provenance_stage11_residency_waterfall_trace.csv`.

## Progress at handoff

The monolithic far-field display list has been replaced in `Main.cpp` by a
retained absolute-tile window:

```text
coarse tiles       32 m, 4 m samples
stitch tiles        8 m, 0.5 m samples
anchor cadence      8 m
far extent        384 m
```

An anchor shift now retains overlapping tile resources, discovers and compiles
only the entering/exiting fringe, reuses an aged bounded display-list pool, and
defers cache-fringe retirement off the boundary-crossing frame. Exact surface
samples and material classifications for upcoming anchors are derived
nearest-first during headroom frames. The canonical quarter-offset lattice was
preserved after a visual audit caught and removed an early live/stitch crack.

Two consecutive capture-free eastward receipts:

```text
median frame-thread time       8.389 / 8.959 ms
p99                            43.241 / 42.934 ms
worst                          47.820 / 47.833 ms
frames >50 ms                       0
frames >100 ms                      0
far-field shift total         360.755 / 393.436 ms
far tile compile total        347.345 / 379.551 ms
ground/collision failures           0
correctness                       PASS
gameplay 60 FPS gate          FAIL
optimization milestone        LT_50_PASS
```

Relative to the first receipt, worst traversal time fell from 155.502 ms to
47.820 ms and the 24 frames above 100 ms fell to zero. This is optimization
progress, not acceptable gameplay: dozens of frames still exceed 16.67 ms.

**Correction from the all-direction cut below: the second gate was not actually
closed.** That `SECOND_GATE_LT_50_PASS` came from single eastward runs. Measured
across four bearings and repeat runs, worst frames sit between 46 and 62 ms, so
sub-50 ms is marginal rather than passed. The standing gate is
`ALLDIR_FIRST_GATE_LT_100_PASS`.

The current method still performs derivation and OpenGL display-list compilation
on the frame thread. Lookahead is also frame-thread work. Do not describe this as
final bounded/off-thread publication or smooth traversal. The latest east trace
puts roughly 2.3 seconds of precomputation into ordinary movement frames. Cold
initial far-field warmup remains about 2.2 seconds and is an explicit open defect.

## All-direction capture-free pacing (closed)

`CERT_STAGE11_RESIDENCY_WATERFALL_ALLDIR.cmd` runs the same 192 m walk/sprint/
free-flight route in a fresh process per bearing and reports a combined verdict.
Each bearing writes `Docs/provenance_stage11_residency_waterfall_<bearing>.txt`
and a trace CSV; east additionally keeps the original unsuffixed receipt path.
The single-direction command still works, and the client now accepts
`--cert-stage11-residency-waterfall-{north,east,south,west}`.

Extending to all four bearings exposed a real defect rather than confirming the
eastward number. The far-field lookahead inferred travel direction from the
player's offset inside the anchor cell. That anchor is floor-snapped, so the
offset is non-negative on both axes and the lookahead predicted +X/+Y
unconditionally: travelling west or south it prefetched already-cached ground the
player had just left, never spent its budget, and left every anchor shift to pay
the full unassisted cost.

```text
bearing   median    p99      worst    >50 ms      (before fix)
north      8.746   40.205   55.712        1
east       8.502   40.999   52.607        1
south      1.305   66.753   80.917        6
west       1.345   68.624   82.725        6

bearing   median    p99      worst    >50 ms      (after fix)
north      8.909   41.442   45.978        0
east       9.741   42.951   48.234        0
south      9.309   46.380   61.941        2
west      10.038   42.553   46.788        0
```

Direction is now tracked from observed player motion, which also yields a
bearing before the first anchor crossing so the first shift of a traversal is
assisted too. p99 is uniform across bearings after the fix (41.4-46.4 ms,
previously 40.2-68.6 ms). Three repeat southward runs gave worst frames of
52.709, 49.496 and 46.128 ms, so the southward outlier is run variance across
the 50 ms boundary, not a directional defect.

All four bearings report correctness PASS, zero ground failures, zero collision
mismatches, and an identical 24 shifts / 2,430 tiles built per route.

## Shift-scaling receipt (closed)

`CERT_STAGE11_SHIFT_SCALING.cmd` is the permanent 8/16/32/64 m receipt, written
to `Docs/provenance_stage11_shift_scaling.txt` and documented in
`Docs/STAGE11_SHIFT_SCALING.md`. It performs four shifts in each class,
attributes each shift frame individually, and normalises the work per metre of
shift. Constant work per shift is the monolithic signature; constant work per
metre is the incremental one. The exit code reports measurement integrity and
`scaling=` carries the verdict, so an open gate is not reported as a crash.

```text
resident far tiles                    753  (609 coarse, 144 stitch)

class      far ms/shift   tiles/shift   coarse   stitch   window fraction
   8 m           23.604        101.25    20.25    81.00            0.1345
  16 m           21.666        113.50    28.50    85.00            0.1507
  32 m           39.752        138.00    45.00    93.00            0.1833
  64 m          113.487        187.00    78.00   109.00            0.2483

bounded_fringe    PASS
scaling           OPEN
```

`bounded_fringe` passes: an 8 m shift touches 13.45 % of the resident window, so
the retained absolute-tile scheme genuinely retains — the old monolithic path
would report 100 % here. But `scaling` is open, and the split by tile kind says
exactly why:

- **Coarse 32 m tiles scale correctly** — 20 of 609 rebuilt at 8 m (3.3 %),
  growing to 78 at 64 m.
- **The 0.5 m stitch ring does not scale at all** — 81 of 144 stitch tiles
  (56 %) rebuilt on *every* shift regardless of distance, rising only to 109 at
  64 m.

A stitch tile's content signature depends on the blend from the exact live
boundary to the coarse far field, and that blend is measured relative to the
current presentation bounds. When the anchor moves, every stitch tile's blend
field changes even though the ground beneath it did not, so the tile is
correctly judged stale and correctly recompiled. The tiles are absolute in
position but their *content* is anchor-relative. This fixed stitch floor — not
the coarse field — is what holds worst-frame traversal near 45 ms, and it is the
next scheduling target.

## Correctness control

The Stage 11 fault certificate was rerun after instrumentation and remains PASS:

```text
CAUSAL_WORLD_FAULT_DISPLACEMENT PASS
shifted_samples=26724
quartz_shifted=229
quartz_feature_id=da10b0d1e5f01001
max_inverse_error_m=0
```

The complete cardinal replacement gate was rerun after the tiled implementation:

```text
CERT_WORLDGEN_CARDINAL_REPLACEMENT.cmd
32/32 cases PASS
7168 movement samples
origin packages at 192 m = 0
exact return geometry/material/collision/package/image
zero sky pixels and zero fallback-green pixels
```

All four current Stage-11 outer-station images were also inspected. They show
continuous terrain in north, east, south, and west with no live/stitch sky crack
or green ownership border.

Important: the 32-case cardinal harness is a correctness/return audit, not a
clean performance receipt. It includes cold stage setup and image work; its
recorded Stage-11 worst frames range from about 306 ms to 2.24 seconds.

It was rerun after the lookahead direction fix and remains 32/32 PASS with
`check.origin_fully_evicted_at_outer`, `check.authority_population_matches`,
`check.collision_surface_continuity`, `check.no_outer_sky_or_fallback` and
`check.return_origin_exact` all PASS. The Stage 11 fault-displacement authority
certificate was also rerun after the fix and remains PASS on all 13 checks.

Rerun both after changing far-field ownership or publication.

## Current implementation locations

All relevant runtime work is presently in:

```text
Code/Applications/ProvenanceClient/Main.cpp
```

Important sections:

- `AppState::stage0FarCoarseTiles`, `stage0FarStitchTiles`, list pools, and
  far-field caches own the retained presentation window.
- `DrawStage0FarField()` performs tiled discovery, retention, compilation,
  publication, cache retirement, and lookahead.
- `RebuildStage8TerrainBlock()` and `DrawStage8TerrainBlocks()` contain exact live-package instrumentation.
- `Stage11ResidencyWaterfallCounters` owns waterfall attribution.
- `Stage11ResidencyWaterfallTick()` owns the deterministic 192 m route, now
  parameterised by `g.certStage11WaterfallBearing`.
- `Stage11ResidencyWaterfallAfterRender()` records the per-frame trace.
- `Stage11BearingName/Step/Yaw()` and `Stage11PlaceProbe()` are the shared
  bearing convention and station placement used by both capture-free routes.
- `Stage11ShiftScalingTick()` / `Stage11ShiftScalingAfterRender()` own the
  8/16/32/64 m ladder and its per-shift counter attribution.
- `g.stage0FarTravelDirX/Y`, updated near the top of `DrawStage0FarField()` from
  observed player motion, is the lookahead's travel bearing.

The old one-list rebuild is gone. The remaining expensive portion is synchronous
fringe derivation/compilation plus main-thread lookahead and cold construction.

## Required next architecture

Absolute retained tiles are implemented. Continue with the scheduling half:

```text
absolute tile key
    world/runtime/control derivation identity
    fixed world-space bounds
    persistent render resource
    triangle count
    lifecycle state
```

On a window shift:

1. Retain overlapping tiles unchanged.
2. Discover only newly required tiles in the entering fringe.
3. Compile all missing fringe tiles synchronously (still open).
4. Publish completed tiles individually.
5. Retire only tiles outside the bounded collar.
6. Never interpret an unpublished tile as authoritative air.

The exact 0.5 m stitch ring and coarse 4 m field now share the canonical
quarter-offset boundary. Preserve it. A tile boundary must not introduce cracks,
overlaps, duplicated ownership, material drift, or lighting discontinuity.

Next implementation steps:

1. ~~Add a permanent 8/16/32/64 m shift-scaling receipt.~~ Done —
   `CERT_STAGE11_SHIFT_SCALING.cmd`.
2. ~~Extend capture-free walk/sprint/free-flight pacing to all four
   directions.~~ Done — `CERT_STAGE11_RESIDENCY_WATERFALL_ALLDIR.cmd`.
3. **Make stitch-tile content anchor-invariant.** This is now the measured
   bottleneck: 56 % of the stitch ring recompiles on every shift regardless of
   shift distance, because the tile signature folds in a blend measured from the
   moving presentation bounds. A stitch tile should only rebuild when it enters
   the ring or changes kind. Whatever replaces the current blend must keep the
   canonical quarter-offset lattice and must not reintroduce the live/stitch
   crack that the earlier visual audit caught.
4. Separate CPU tile derivation from GL publication using immutable command buffers.
5. Compile/derive missing tiles off the frame thread where APIs permit.
6. Publish only a bounded number or bounded milliseconds per frame.
7. Keep the previous valid tile/collar visible until replacement publication;
   absence or pending work must never become air.
8. Reduce worst traversal below 50 ms reliably in all four bearings, then
   33.3 ms, then 16.67 ms.
9. Address the ~2.2 s cold warmup separately without weakening continuity.

## Measurement additions

Extend the existing waterfall with:

```text
far tiles retained
far tiles discovered
far tiles built
far tiles published
far tiles retired
pending tiles
tile derivation ms
tile resource allocation ms
tile compile ms
publication ms
work by anchor shift distance
```

Per-shift retained/discovered/built counters are present, and the dedicated
8/16/32/64 m shift-scaling receipt now exists. Work by anchor shift distance is
reported per class and normalised per metre. The coarse field satisfies the
requirement; the stitch ring does not yet.

## Gates

Correctness:

```text
same geology/material/FeatureId answers
same far geometry and contact locations
zero sky holes, seams, green fallback, and boundary pop
render/collision surface parity
bounded residency with no path-shaped retention
exact 192 m return regeneration
Stage 11 authority certificate PASS
```

Performance, measured capture-free in all four bearings, not eastward alone:

```text
first: no measured frame >100 ms    PASS (all bearings)
then:  no measured frame >50 ms     MARGINAL: 46-62 ms across bearings and runs
target: no measured frame >33.3 ms  OPEN
final:  no measured frame >16.67 ms OPEN
```

A gate counts as passed only when every bearing passes it. A single eastward run
under a threshold is not a gate.

## Build and run

Release build:

```text
cd C:\Users\D-Day\ProvenanceEsoterica
set "Path=" && set "PATH=C:\Windows\system32;C:\Windows" && "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "Code\Applications\ProvenanceClient\Esoterica.Applications.ProvenanceClient.vcxproj" /m /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:SolutionDir=C:\Users\D-Day\ProvenanceEsoterica\ /v:minimal
```

Then run:

```text
CERT_STAGE11_RESIDENCY_WATERFALL_ALLDIR.cmd
CERT_STAGE11_SHIFT_SCALING.cmd
CERT_CAUSAL_WORLD_FAULT_DISPLACEMENT.cmd
CERT_WORLDGEN_CARDINAL_REPLACEMENT.cmd
```

`CERT_STAGE11_RESIDENCY_WATERFALL.cmd` remains available for a quick eastward
run. The all-direction command runs four GUI processes back to back and takes a
few minutes; it truncates and owns
`Docs/provenance_stage11_residency_waterfall_alldir.txt`, so read the combined
verdict from that file rather than from any single bearing.

The full cardinal audit is a GUI process and takes about two minutes. When
driving it from PowerShell automation, wait for the spawned process to exit;
otherwise the shell can return while the old receipt is still on disk. Confirm
that the final receipt contains exactly 32 `case.` rows and a fresh timestamp.

## Worktree warning

The repository is intentionally dirty and contains substantial user work and generated certification artifacts. Do not reset, clean, or overwrite unrelated changes. Patch only the far-field/runtime sections and new diagnostic files. No commit has been created for this cut.

## Success sentence

> The same certified far-field answer is retained across player motion; only newly exposed absolute tiles are derived and published, under a bounded frame budget, without terrain holes or authority drift.
