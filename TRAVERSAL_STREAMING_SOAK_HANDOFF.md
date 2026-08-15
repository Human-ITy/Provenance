# Traversal + Streaming Soak — Permanent Certificate

A worldgen stage is not certified just because its math is correct. It is
certified only if the player can keep moving through newly generated world
indefinitely, within bounded residency and frame-time limits.

**Law:** Travel distance may grow; active residency, memory, pending work,
and wake backlog must remain bounded.

This is the standing matrix. Cardinal replacement and the long-haul soak are
**two different tests**. Passing one never substitutes for the other. Do not
narrow a future stage to “cardinal movement passed.”

P5b.2A is the **2A control** (land `570c7be2`, pin `eae7db9f`, harness
`9cae0794`, control pin `6c385e2c`). Latest play stage is P5b.2B.
P5b.2C, P5b.3, rainfall, and erosion stay **CLOSED**.

## Two tests

### A. Cardinal replacement cert (short, every stage)

Proves **replacement correctness over a bounded route**.

- Keep / run constantly.
- Walk, sprint, and free-flight segments in both directions.
- N / E / S / W. Exact origin eviction at the outer station.
- Exact digests on return (geometry, material/FeatureId, collision, packages,
  resident-package digest, visual).
- Complete 192 m residency at origin, outer, and return.
- 0 movement frames over 16.667 ms.
- Declared resident package bound (2601 at 192 m). Do not weaken.

```text
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b2b
```

or `CERT_WORLDGEN_CARDINAL_REPLACEMENT.cmd` for the full stage ladder.

Latest play-stage receipt: `Docs/provenance_p5b2b_cardinal_replacement_cert.txt`
2A control receipt: `Docs/provenance_p5b2a_cardinal_replacement_cert.txt`

### B. Long-haul streaming soak (timed, no return)

Proves **bounded memory / residency under continuous travel**. No return is
required. Distance must grow while the resident set stays a moving window.

Default first landing: **90 s** wall-clock (same metrics as the 5–15 min
milestone). Milestone / release: `--soak-duration-s=300` or `900`.

```text
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-p5b2b --soak-duration-s=90 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
```

or `CERT_STREAMING_SOAK.cmd`.

Receipt: `Docs/provenance_p5b2a_streaming_soak_cert.txt`  
Trace: `Docs/provenance_p5b2a_streaming_soak_trace.csv`

Standing metrics (required on every receipt; do not invent green):

```text
locomotion: walk / run / sprint / fly / sprint+fly
  sprint+fly = --soak-mode=fly (highest normal traversal, 24 m/s)
direction: N/E/S/W + diagonals (NE SE SW NW)
distance traveled and elapsed soak time
192 m completeness / 2601 package target
resident / created / retired / pending package counts
  (max + end resident; max + end pending)
oldest pending age, worker queue depth, min complete radius
water-body wakes, terrain-state wakes, collision publishes,
  representation rebuilds (derived mesh)
mean / p95 / p99 / max frame time, frames > 16.667 ms
memory high-water mark (working set + private)
exact return/reload digest where applicable (Test A only)
```

Product gates (walk through 480 m/s): 0 movement frames over 16.667 ms,
complete required residency, bounded package count, end pending = 0,
packages created **and** retired, distance ≥ 80% of speed × duration.
960 m/s is informational only — not a product gate.

## Traversal contract

```text
Traversal modes:
- walk          5 m/s   grounded
- run           8 m/s   grounded
- sprint       11 m/s   grounded
- free-flight  24 m/s
- sprint+fly   24 m/s   highest normal traversal (`--soak-mode=fly`)
Directions:
- N E S W
- diagonals (NE SE SW NW) — streamer window is a square; both axes
  changing together is a different schedule than a single-axis bearing
World lifecycle:
- cold launch
- sustained outward travel
- chunk/package creation ahead
- eviction behind
- exact revisit/regeneration          (Test A only)
- repeated leave/return cycles        (Test A / MAJOR)
- stage transition
- reload/save-load where relevant     (MILESTONE)
Performance:
- 0 traversal frames > 16.667 ms
- complete required residency
- bounded package count
- no growing backlog
- no long-tail present stalls
```

## Speed ladder

```text
walk                         5 m/s     product
run                          8 m/s     product
sprint                      11 m/s     product
fast free-flight            24 m/s     product
480 m/s certified stress   480 m/s     product stress
960 m/s informational      960 m/s     not a product gate
```

`--soak-mode=walk|run|sprint|fly|sprint+fly`  
`--soak-speed-mps=` overrides speed. `960` sets informational-only.  
`--soak-bearing=north|east|south|west|northeast|southeast|southwest|northwest`

## Tiers

```text
EVERY CUT
- short walk / sprint / fly          (inside Test A)
- cardinal replacement               (Test A, current play stage)
- 192 m completeness
- frame gate (0 movement frames > 16.667 ms)

MAJOR WORLDGEN / STREAMING CUT
- all bearings including diagonals   (--soak-bearing=)
- all locomotion modes               (--soak-mode=)
- repeated eviction / return         (Test A)

MILESTONE / RELEASE GATE
- multi-km or timed soak             (Test B, 300-900 s)
- memory / backlog stability
- repeated chunk creation / retirement
- cold / reload cycle
```

`CERT_TRAVERSAL_EVERY_CUT.cmd` runs Test A on the latest play stage (P5b.2B).
`CERT_STREAMING_SOAK.cmd` runs the first-landing 90 s Test B. Milestone soak
is the same harness with `--soak-duration-s=300` or `900`.

## Cross-system traversal wake cost

As the player flies through new regions, measure not only terrain packages.
Loading a region must not hide a burst in cheaper systems:

```text
water body reconstruction
hydraulic topology activation
derived mesh rebuilds
grass / tree loads
collision publication
```

Both Test A and Test B receipts now print these counters. Combined cheap
systems cannot hide a burst behind a green terrain-only frame.

On P5b.2A, grass/tree loads stay 0 (L1/L6 are a separate ladder). Water-body
and topology ticks after cold compile should stay 0 during travel — compiled
kernels QueryAt; they must not rebuild per package. Derived mesh rebuilds and
collision publications should track package create/retire.

## Inventory (what already existed vs this cut)

| Harness | What it proved | Gap this cut closes |
|---|---|---|
| `--cert-worldgen-cardinal-replacement*` | Test A: N/E/S/W 192 m, exact return, 16.667 movement gate, 2601 | Wake-cost counters added. Diagonals still not in Test A (intentional — Test A stays short). |
| L0–L6 living-world load | Walk / sprint / 240 / 480, N/E/S/W, 192 m, 16.667 | Synthetic load ladder, not indefinite travel. |
| Stage 11 freefly / waterfall | High-speed 192 m capture-free | Bounded route, Stage 11 only. |
| Presentation isolation | 40 s walk/sprint/fly | Backend isolation, not soak. |
| **`--cert-streaming-soak`** | **New Test B** | Timed outward travel, diagonals, speed ladder, memory/backlog, wake cost. |

Run is a first-class speed-ladder rung (`--soak-mode=run`). Cardinal Test A
keeps its historical walk/sprint/fly split and must not be weakened.

## 2A control (do not relax)

Parent pin `eae7db9f` / land `570c7be2` / harness `9cae0794`. Duration **90 s**
(same metrics as the 5–15 min milestone; milestone still uses
`--soak-duration-s=300` or `900`). This FAIL is the **2A control
measurement**. Compare 2B against these numbers. Do **not** relax 16.667.

### Test A — P5b.2A cardinal (EVERY CUT) — PASS

`WORLDGEN_CARDINAL_REPLACEMENT PASS`. Digest `f6c20f2c4774451b` origin==return.
2601 packages. Movement frames over 16.667 ms = 0. Wake counters recorded;
water-body / topology / terrain-state ticks during travel = 0.

### Test B — 90 s NE free-flight soak at 24 m/s — residency PASS, frame FAIL

Receipt: `Docs/provenance_p5b2a_streaming_soak_cert.txt`

| Metric | 2A control |
|---|---|
| elapsed | 90.000 s |
| distance | 2160.1 m |
| created / retired | 19190 / 19190 |
| max resident / declared | 2601 / 2601 |
| end pending / max pending | 0 / 101 |
| oldest pending age | 0.018 s |
| min complete radius | 192.00 m |
| max worker queue | 209 |
| frames | 20079 |
| mean / p95 / p99 / max ms | 4.483 / 6.617 / 9.824 / 60.687 |
| frames > 16.667 ms | **20 FAIL** |
| working-set high water | 164 MB → 2338 MB |
| water-body / topology / terrain-state wakes | 0 / 0 / 0 |
| derived mesh / collision publishes | 19190 / 19190 |
| water bodies queried | 15 |

Working set grew while the resident package count stayed bounded — recorded,
not used to weaken the frame gate. Terrain-state wakes were not a separate
line on the first-landing receipt; travel water-body / topology = 0 and 2A
Tick after cold compile is a no-op, so the control value is 0. The harness
now prints `elapsed_s`, `end_resident_packages`, and
`wake.terrain_state_wakes` for 2B comparison.

The 16.667 / 2601 / exact-return gates were not relaxed. Soak FAIL is an
honest first-landing hitch under sustained travel, not a reason to treat
cardinal PASS as a soak.

## 2B vs 2A control

Test A P5b.2B cardinal PASS (re-run after apron-GL amortize).
Digest `2396f444f66f1234` origin==return (2A was `f6c20f2c4774451b` —
different stage). 2601 packages. Movement frames over 16.667 = 0.
Water / topology / terrain-state wakes = 0.

P5b.2B gameplay remains frozen at `8bb75265`. This cut is soak
lifecycle + diagnostics only. P5b.2C / P5b.3 stay CLOSED.

## Soak lifecycle closure (frozen 2B world)

Every movement frame >16.667 ms writes a named receipt
(`package_build`, `collision_publish`, `mesh_publish`, `GL_create`,
`GL_retire`, `glFinish`, `SwapBuffers`, `worker_wait`,
`allocator_growth`, `draw_submit`). If nothing is ≥0.5 ms, the owner
stays **unclassified** — do not hide it.

Distance-bucketed resource ledger every 250 m (resident / pending,
live mesh+collision+GL count/bytes, deferred retirement, worker
results, cache entries, private vs working set, bucket p95/p99/max).

Phases: travel → 60 s stopped → discard leftover lookahead → drain
→ optional `--soak-return=1` fly-back + settle.

Lifecycle fix in this cut: collar / apron GL compile is budgeted at
`kStage0ApronPublishBudgetMs` (2.5 ms). Live-window holes still use
the hard 10.667 ms publication budget. A diagonal row-crossing no
longer compiles ~101 lists on the crossing frame. 2B physics is
untouched.

### Classified frame tail — still FAIL

Do not treat a lower over-budget count as a gate change. 16.667 stays.

| Run | frames >16.667 | max ms | primary classes |
|---|---|---|---|
| 2A control 90 s | 20 / 20079 | 60.687 | unattributed (pre-cut) |
| 2B frozen 90 s | 14 / 20904 | 42.066 | unattributed (pre-cut) |
| 2B attributed 90 s | 6 / 30013 | 37.646 | 5 package create, 1 allocator growth |
| 2B attributed 300 s | 4 / 215933 | 30.908 | 2 package create, 1 allocator growth, 1 unclassified |
| 2B lifecycle 90 s | **1 / 29903** | 34.025 | 1 allocator_growth |
| 2B lifecycle 300 s | **1 / 216362** | 35.311 | 1 allocator_growth |
| 2B lifecycle 900 s | **1 / 786675** | 38.301 | 1 allocator_growth |
| 2B follow_stream 90 s | **1 / 28504** | 45.276 | 1 crt_heap_segment/follow_stream |
| 2B scratch-owner 90 s | **1 / 32399** | 60.176 | 1 draw_submit @ 528.4 m |
| 2B draw-attrib 90 s | **1 / 34858** | 61.920 | 1 glFinish after first `glDrawArrays/water` @ 527.7 m |
| 2B water-warmup 90 s | **1 / 35305** | 74.206 | hitch **persisted**: glFinish after first live water @ 525.9 m |

The FollowStream CRT segment is closed. Capacity instrumentation
showed the 8.4 MB first-commit was **not** a retained package: per-call
eviction / required-key / scheduled snapshot vectors doubled on the
CRT heap, and `EnsureGeoCell` copied `GeoSample` event/chronology
vectors (unique-query high-water ≈ 65536 cells at ~2018 m).

`FollowStreamScratch` is reserved once from residency law (live 192 m,
package 8 m, 2601 resident + apron + lookahead + one transition ring).
Gameplay is `clear()` + reuse; overflow is cert FAIL, no CRT fallback.
Cell-map nodes live in a 12 MB arena (packages stay outside). Package-
owned cell create uses package Z + `QueryMaterial`, not full provenance
`Query` / `SurfaceGeology`.

Scratch receipt (90 s): required-key max 3249 (hw 2401), eviction
max 3249 (hw 101), geo-disk max 16384 (hw 12853), sort max 3249
(hw 3025), reserved 13047328 B, observed 13047328 B, growth 0,
overflow 0, FollowStream CRT segment events 0.

16.667 stays FAIL on this binary. Draw-submit attribution split the
old 60 ms `draw_submit` owner:

- Discriminator **GPU**. Presentation lane **water**.
- Named call: diagnostic `glFinish` 71.975 ms after first live
  `glDrawArrays/water` (30 tris / 15 quads) at 525.9 m. Previous
  call: `glBegin/geology_lines` 0.007 ms. Terrain CallLists 0.04 ms.
  CPU `draw_submit` 2.058 ms (batched).
- `--soak-draw=no-water` (same residency): travel **0 / 28041+**
  over 16.667, max 5.612 ms. Water submit owns the stall.
- Occupied-water first-use warmup (`water_path_warmed=1`): create/bind
  the exact water client-array pipeline, submit one in-view lake quad
  on that path, `glFinish` once, then begin timed travel. Hitch
  **persisted** at the first live occupied-water batch (~526 m). It
  did not move to another owner. Dummy / frustum / full-kernel and
  this exact-path warmup all failed to absorb it. Backend / resource
  lifecycle remains. Do not switch backend in this cut.

FollowStream scratch growth 0, CRT segment 0. Stop had one 74 ms
frame (discriminator B, GPU fence cheap — not the travel first-use).
Drain / return over 16.667 = 0. 300 / 900 not run — 90 s is not
green. Do not relax 16.667. P5b.2C / P5b.3 stay CLOSED.

### Memory — logical set bounded; process high-water plateaus

250 m buckets: logical live (2601 packages / 114 MB, 22 MB worker
results) is flat from the first bucket. Private climbs through the
first ~3 km (CRT/GL high-water), then sits at ~1.31–1.33 GB out to
21.6 km. Working set follows private, not travel distance.

| Ledger | 90 s / 2160 m | 300 s / 7188 m | 900 s / 21597 m | after stop+drain |
|---|---|---|---|---|
| resident packages | 2601 | 2601 | 2601 | 2601 |
| live package bytes | 114 MB | 114 MB | 114 MB | 114 MB |
| worker-result bytes | 22 MB | 22 MB | 22 MB | 22 MB |
| completed/held results | 424 | 424 | 424 | 424 |
| private bytes | 1405 MB | 1325 MB | 1327 MB | 1327 MB |
| working set | 1218 MB | 1214 MB | 1222 MB | 1222 MB |

`check.memory_plateau=PASS_plateau`.
`check.distance_bucket_slope=PASS_plateau` (post-3 km window).
d3000 private 1528 MB → d7000 1324 MB → d15000 1324 MB → d21000 1327 MB.

### Stationary drain + return (90 s run)

90 s NE fly → 60 s stop → drain: pending 0, resident 2601, private
1383 MB, WS 1215 MB. Logical live unchanged while stopped.

Optional return-to-origin + settle: end_distance 0, resident 2601,
pending 0, return/settle frames over 16.667 = 0. Private/WS rose to
1812 / 1623 MB on the return churn (high-water, not a logical leak —
live packages/mesh/collision/worker bytes stayed 114 / 22 MB).

### 90 / 300 / 900 gates

```text
90 s   residency PASS, backlog PASS, memory INCOMPLETE_need_300s,
       frame FAIL (1 / 35305; hitch persisted: glFinish 71.975 ms
       after first live glDrawArrays/water 30 tris @ 525.9 m).
       water_path_warmed=1. scratch growth 0, overflow 0,
       FollowStream CRT segment 0.
300 s  not run — 90 s not green
900 s  not run — 90 s not green
       192 m complete, 2601 resident max, pending drains to 0, max pending 72
```

Receipt: `Docs/provenance_p5b2b_streaming_soak_cert.txt` (900 s run
includes 90 / 300 snapshots + 250 m buckets). Trace:
`Docs/provenance_p5b2b_streaming_soak_trace.csv`.

## Closed

- P5b.2C pore storage affecting water occupancy / topology
- P5b.3 water→terrain mechanical / matter movement
- rainfall, groundwater, active 16B erosion, 16C remobilization, ecology
