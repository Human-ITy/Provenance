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
`9cae0794`, control pin `6c385e2c`). Latest play stage is P5b.3B.2.
**P5b.3C**, **P5b.3B.3**, rainfall, and erosion stay **CLOSED**.

```
LONG-HAUL INFRASTRUCTURE BASELINE
SHA: f7ae29ea8e8e689196e829fbac6cb098e0cda05b
PARENT: 800cfaef
```

**Pin:** Stage-12 worker dual-cache lifetime. Cache-lifetime correction, not
worldgen. Stage-12 digest `d2f1c29c2fcaad9d`. Test S green. Test A
`2396f444f66f1234`. Forced-cold == cache-enabled; eviction/recompute
identical. 900 s 0 frames >16.667. CRT 8→12 km ~+1.5 MB.

## Three tests

### S. Semantic-distance cert (short, independent of Test B)

Proves **newly generated terrain still derives from the latest Stage 15/16
causal pipeline** at absolute world positions across and beyond the compiled
4.096 km Stage-15 domain. Teleport + settle only — not a soak.

- Settle the normal 192 m live window at each station.
- Measure resident-mesh relief/slope, geology/material diversity, Stage-15
  landforms, and 16A–16D fields (drainage, erosion, sediment, present-water).
- Fixed-camera visual receipt per station (same recipe: 24 m inspection,
  pitch −0.55, yaw 0).
- Do not rely on metadata alone. Flat/default heightfield, missing landforms,
  missing 16A–16D, or a visual that does not match the claimed generator is FAIL.

```text
Build\x64_Release\ProvenanceClient.exe --cert-semantic-distance --live-radius=192 --far-extent=0
```

or `CERT_SEMANTIC_DISTANCE.cmd`.

Receipt: `Docs/provenance_semantic_distance_cert.txt`  
Visuals: `Docs/provenance_semantic_distance_<station>.ppm`

Stations: origin, inside-east (1856), boundary E/N/W (±2048), just outside
east (2240), far east 8 km, far NE 12 km diagonal.

Latest receipt: `SEMANTIC_DISTANCE PASS` (8/8) at `6b14d8fd`. Generator
`provenance_causal_world` / `36d381f2ab7bb953` at every station. 2601
packages, 192.00 m complete, 16A–16D 441/441, sky pixels 0. Origin relief
55.5 m; far-east 8 km 51.7 m; far-NE 12 km 52.8 m. Not a flat fallback.

**Caution (do not hide):** wrapping the 4096 m Stage-15 tile is a valid
continuity fix only if we explicitly accept **repeated macro geography**
as the current production shortcut. It solves “flat fallback.” It does
**not** yet solve “indefinitely novel macro geography.” Far stations
stay on the Stage 15/16 causal pipeline by tiling the compiled domain;
they are not new continents.

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
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b3b2
```

or `CERT_WORLDGEN_CARDINAL_REPLACEMENT.cmd` for the full stage ladder.

Latest play-stage receipt: `Docs/provenance_p5b3b2_cardinal_replacement_cert.txt`
3B receipt: `Docs/provenance_p5b3b_cardinal_replacement_cert.txt`
3A receipt: `Docs/provenance_p5b3a_cardinal_replacement_cert.txt`
2C receipt: `Docs/provenance_p5b2c_cardinal_replacement_cert.txt`
2A control receipt: `Docs/provenance_p5b2a_cardinal_replacement_cert.txt`

### B. Long-haul streaming soak (timed, no return)

Proves **bounded memory / residency under continuous travel**. No return is
required. Distance must grow while the resident set stays a moving window.

Default first landing: **90 s** wall-clock (same metrics as the 5–15 min
milestone). Milestone / release: `--soak-duration-s=300` or `900`.

```text
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-p5b3b2 --soak-duration-s=90 --soak-stop-s=0 --soak-return=0 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0 --soak-water-backend=persistent
```

or `CERT_STREAMING_SOAK.cmd`.

Receipt: `Docs/provenance_p5b3b2_streaming_soak_cert.txt`  
Trace: `Docs/provenance_p5b3b2_streaming_soak_trace.csv`

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
  P5b.2C: pore transfers, wet→dry / dry→wet occupancy, 16F.4
  topology rebuilds, body split/merge/grow/shrink (travel delta)
  P5b.3A: detachments, admitted mass, stale refuse, water responses,
  topology rebuilds, terrain/water revision, loose mass (travel delta)
  P5b.3B: transfers, transport wakes, pending retained, stale refuse,
  transport revision, loose mass (travel delta; must stay 0 on ordinary fly)
  P5b.3B.2: settles, settle wakes, pending retained, stale refuse,
  settling revision, loose mass (travel delta; settle count must stay 0 on ordinary fly)
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

`CERT_TRAVERSAL_EVERY_CUT.cmd` runs Test A on the latest play stage (P5b.3B.2).
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
| **`--cert-semantic-distance`** | **Test S** | Teleport+settle across/beyond 4.096 km Stage-15 domain; mesh/16A–16D/visual. Independent of Test B. |

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

### Classified frame tail

Do not treat a lower over-budget count as a gate change. 16.667 is not
relaxed. Persistent water VBO/IBO is the live submit path.

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
| 2B after Test S 90 s | **1 / 32372** | 99.430 | hitch **persisted**: glFinish 95.925 ms after first live `glDrawArrays/water` 36 tris @ 509.6 m |
| 2B persistent-water 90 s | **0 / 79961** | 10.091 | first live occupied water, no stall |
| 2B persistent-water 300 s | **0 / 283198** | 11.942 | water GPU 170 KB, travel growth 0 |
| 2B persistent-water 900 s | **0 / 859464** | 11.505 | 16.667 PASS; process private still climbs |
| 2B ownership 90 s | **0** | 11.314 | HeapWalk hold; recycle on; memory INCOMPLETE |
| 2B ownership 12 km | **0 / 420668** | 14.816 | 16.667 PASS; CRT committed FAIL |

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

16.667 is **PASS** on the persistent-water binary. Occupied-water
submit is owned GPU VBO/IBO: same CPU geometry / occupancy /
visibility / presentation revisions; subrange `glBufferSubData` on
window or revision change; atomic publish; retire after fence (or
two frames if `ARB_sync` is missing). No per-frame client arrays,
no per-frame buffer create/destroy, no GPU mesh generation.

A/B (physics unchanged):

```text
--soak-water-backend=persistent   live path (default)
--soak-water-backend=legacy       client-array control
--soak-draw=no-water              negative control
```

90 s persistent: first live occupied water did not stall. 0 / 79961
over 16.667, max 10.091 ms. Water GPU live 2 batches / 170 KB,
alloc 2, travel growth 0, draw 1/frame, first_live_occupied=1.
`--soak-draw=no-water` previously 0 / 28041+, max 5.612 ms.

FollowStream scratch growth 0, CRT segment 0. P5b.2C / P5b.3 stay
CLOSED.

### Memory — logical set bounded; CRT committed pages retained

250 m buckets: logical live (2601 packages / 114 MB) is flat from
the first bucket. Water GPU stays 170 KB. Process private does **not**
plateau. Owner is **not** the water VBO, not FollowStream scratch, not
live packages, not deferred GL retirement.

Ownership walks (HeapWalk + VirtualQuery) run only at checkpoint
drain, never on a travel frame. Mid-travel HeapWalk poisoned the
resume frame (40–144 ms); a 3-frame hold after each walk restored
16.667 = 0.

Package mesh/material recycle (452024 hits / 424 misses) plus a
dedicated collision-surface pool (449423 hits / 3025 misses = live
window + lookahead) reuse finished objects. Worker-owned package
scratch is reserved once per worker:

```text
4 workers × 12562 B = 50248 B
sampled cells 289 Vec3
crossing descriptors 256
GeoSample event/chronology slots (certified 32; observed high-water 0)
overflow 0 / fallback CRT 0 / growth 0
```

Worker SampleBlock / DescribeBlock write into that scratch and reset.
`ReconstructedZ` / incision use `QueryMaterial` (found-only); full
provenance `Query` event/chronology vectors are not copied on the
package worker path. 2B physics / water truth / package output
unchanged. Persistent mesh/material/collision stay outside scratch.

That closed the named SampleBlock / descriptor / GeoSample temps.
It did **not** flatten CRT on that cut. 8 km → 12 km was still ~+415 MB.

**Closed CRT site:** per-worker Stage-12 dual-surface vertex maps
(`CausalDifferentialErosion` `m_differentialDualCache` + equal-resistance
twin) inside `ThreadBareEarthKernel`. Caches are derived acceleration for
the active 192 m live envelope + apron + lookahead (toroidal distance in
wrapped analytical cell space). Coordinate leaves the owned window →
tile erase. Not a lifetime map of every dual vertex ever queried.
Stage-15 wrap happens before DualVertex; Z is a function of wrapped
analytical coords + immutable gen params, so sharing across tiled
instances is legitimate. Keys are wrapped dual cells, not absolute
travel history. Cache miss after eviction recomputes the same Z
(`dual_cache_enabled_equals_forced_cold` PASS).

Soak `erosion_cache_entries` now aggregates the four worker kernels
(was shared runtime 0). 12 km / 500 s: entries warmup ~0.93 M → plateau
~1.3–1.5 M (8 km 1.50 M → 12 km 1.34 M). Worker cache bytes ~48 → 70 MB
plateau. CRT 8→12 km **+1.5 MB** (197 → 199 MB). Slope ≈ 0.

**Retained owner class: `gl_driver_private`** (0→end leftover class;
8→12 km GL +0.2 MB, CRT +1.5 MB). Do not reopen P5b.2C.

Checkpoint drain table (500 s NE fly 24 m/s, stop/drain at each
station, then return to origin). Logical live 122 MB / 2601 / pending 0
/ water GPU 170 KB at every row.

| Station | private | CRT committed | worker cache entries | worker cache bytes | CRT slope |
|---|---|---|---|---|---|
| 0 m drain | 530 MB | 156 MB | 0.93 M | 48 MB | — |
| 1 km drain | 608 MB | 192 MB | 1.34 M | 70 MB | warmup |
| 2 km drain | 613 MB | 196 MB | 1.34 M | 70 MB | +4 MB |
| 4 km drain | 611 MB | 196 MB | 1.39 M | 73 MB | 0 |
| 8 km drain | 615 MB | 197 MB | 1.50 M | 78 MB | +1 MB / 4 km |
| 12 km drain | 617 MB | 199 MB | 1.34 M | 70 MB | **+1.5 MB / 4 km** |
| after drain | 617 MB | 199 MB | 1.34 M | 70 MB | held |
| return origin | 608 MB | 208 MB | 1.21 M | 63 MB | +9 MB return |

`check.memory_plateau=PASS_plateau`.
`check.distance_bucket_slope=PASS_plateau`.
`check.worker_dual_cache_bounded=PASS_bounded`.
`check.crt_8_to_12_plateau=PASS_plateau`.
`ownership.retained_class=gl_driver_private`.
`package_scratch.fallback_crt_allocs=0` (PASS).
Worker dual-cache 8→12 km does not scale with distance.

### Stationary drain + return

90 s NE fly → checkpoint drains at 1/2 km → 5 s stop → drain →
return: end_distance 0, resident 2601, pending 0, movement frames
over 16.667 = 0, max 11.314 ms. Private 521 → 1308 MB at 2.16 km →
1753 MB after return. Pre-cache-bound control.

12 km run: 500 s travel, drains at 1/2/4/8/12 km, return+settle.
Movement frames over 16.667 = **0 / 482125**, max 11.282 ms.

### 90 / 300 / 12 km gates

```text
90 s   residency PASS, backlog PASS, frame PASS (0 frames >16.667, max 11.314),
       first live occupied water no stall, water GPU bounded / travel growth 0,
       scratch growth 0, FollowStream CRT 0,
       memory INCOMPLETE_return_high_water (need 5 km after high-water).
       soak_water_backend=persistent. water_path_warmed=1.
500 s  12.044 km. frame PASS (0 / 482125, max 11.282). water GPU 170 KB, growth 0.
       2601 resident, pending 0. memory PASS_plateau.
       CRT 156 → 199 MB at 12 km (+1.5 MB 8→12). Worker cache 0.93 M → 1.3–1.5 M
       plateau. Return origin private 608 MB / CRT 208 MB.
       overall=PASS. 16.667 not relaxed.
900 s  21.599 km. frame PASS (0 / 832100, max 11.761). Same bounded worker
       cache (~1.2–1.5 M entries, ~63–78 MB) and CRT 8→12 +1.3 MB.
       overall=PASS. P5b.2C / P5b.3 CLOSED.
```

Receipt: `Docs/provenance_p5b2b_streaming_soak_cert.txt` (latest 900 s;
12 km / 500 s copy:
`Docs/provenance_p5b2b_streaming_soak_12km_500_cert.txt`;
900 s copy: `Docs/provenance_p5b2b_streaming_soak_900_cert.txt`).
Trace: `Docs/provenance_p5b2b_streaming_soak_trace.csv`.

## Closed

- P5b.3 water→terrain mechanical / matter movement
- rainfall, groundwater, active 16B erosion, 16C remobilization, ecology

P5b.2C pore occupancy / topology is **CERTIFIED** this cut. Test A digest
changed with stage identity: 2B `2396f444f66f1234` → 2C `f743150420e22175`.
Movement frames over 16.667 = 0 on EVERY-CUT cardinal.

### Test B — P5b.2C 90 s NE free-flight (this cut) — PASS

Receipt: `Docs/provenance_p5b2c_streaming_soak_cert.txt`  
Trace: `Docs/provenance_p5b2c_streaming_soak_trace.csv`

90 s travel, stop=0, return=0, persistent water, live 192 m / far 0.
`--cert-streaming-soak-p5b2c`. 16.667 not relaxed. 300/900 **not run**:
no new accumulating wake class, 0 movement frames >16.667.

| Metric | 2C 90 s |
|---|---|
| elapsed / distance | 90.000 s / 2161.1 m |
| created / retired | 19291 / 19291 |
| max / end resident | 2601 / 2601 |
| end pending / max pending | 0 / 78 |
| min complete radius | 192.00 m |
| frames | 86317 |
| mean / p95 / p99 / max ms | 0.945 / 1.203 / 1.653 / **7.233** |
| frames >16.667 (movement) | **0 PASS** |
| water-body / topology / terrain-state wakes | 0 / 0 / 0 |
| derived mesh / collision | 19291 / 19291 |
| water GPU | 2 batches / 170 KB, travel growth 0 |
| FollowStream scratch growth / CRT segment | 0 / 0 |
| worker dual cache / CRT 8–12 | INCOMPLETE_need_12km (90 s has no 8/12 km stations; 0→2 km warmup matches 2B) |
| p5b2c pore transfers | **0** |
| p5b2c wet→dry / dry→wet | **0 / 0** |
| p5b2c 16F.4 topology rebuilds | **0** |
| p5b2c body split/merge/grow/shrink | **0 / 0 / 0 / 0** |
| pore / topology revision delta | **0 / 0** |
| check.p5b2c_travel_physics | PASS_idle |

Travel did not execute 2C occupancy physics. Loading new packages did not
admit pore transfers or 16F.4 rebuilds. Drain/stop still recorded 5
`draw_submit` overruns (not movement). Memory 90 s plateau remains
INCOMPLETE_need_5km (same class as 2B 90 s). Long-haul 300/900 stay on
the 2B baseline `f7ae29ea` / pin `156df62d`.

P5b.3A hydraulic detachment is **CERTIFIED** this cut. Test A digest
changed with stage identity: 2C `f743150420e22175` → 3A `8b0f3cca9ccb1bb6`.
Movement frames over 16.667 = 0 on EVERY-CUT cardinal.

### Test B — P5b.3A 90 s NE free-flight (this cut) — PASS

Receipt: `Docs/provenance_p5b3a_streaming_soak_cert.txt`  
Trace: `Docs/provenance_p5b3a_streaming_soak_trace.csv`

90 s travel, stop=0, return=0, persistent water, live 192 m / far 0.
`--cert-streaming-soak-p5b3a`. 16.667 not relaxed. 300/900 **not run**:
no new accumulating wake class, 0 movement frames >16.667.

| Metric | 3A 90 s |
|---|---|
| elapsed / distance | 90.001 s / 2160.9 m |
| created / retired | 19291 / 19291 |
| max / end resident | 2601 / 2601 |
| end pending / max pending | 0 / 78 |
| min complete radius | 192.00 m |
| frames | 86035 |
| mean / p95 / p99 / max ms | 0.954 / 1.203 / 1.558 / **6.965** |
| frames >16.667 (movement) | **0 PASS** |
| water-body / topology / terrain-state wakes | 0 / 0 / 0 |
| derived mesh / collision | 19291 / 19291 |
| p5b3a detachments / admitted mass | **0 / 0** |
| p5b3a stale refuse / water responses | **0 / 0** |
| p5b3a topology rebuilds | **0** |
| p5b3a terrain / water revision delta | **0 / 0** |
| p5b3a loose mass delta | **0** |
| check.p5b3a_travel_physics | PASS_idle |
| p5b2c travel physics | PASS_idle |

Ordinary traversal did not fire 3A. Mutation/wake-only. **P5b.3B and
P5b.3C stay CLOSED.** No 300/900 unless travel fires 3A or a later cut
adds an accumulating wake.

P5b.3B hydraulic transport is **CERTIFIED** this cut. Test A digest
changed with stage identity: 3A `8b0f3cca9ccb1bb6` → 3B `59a2725b6b89fd77`.
Movement frames over 16.667 = 0 on EVERY-CUT cardinal.

### Test B — P5b.3B 90 s NE free-flight (this cut) — PASS

Receipt: `Docs/provenance_p5b3b_streaming_soak_cert.txt`  
Trace: `Docs/provenance_p5b3b_streaming_soak_trace.csv`

90 s travel, stop=0, return=0, persistent water, live 192 m / far 0.
`--cert-streaming-soak-p5b3b`. 16.667 not relaxed. 300/900 **not run**:
ordinary traversal transfer count = 0, loose-matter transport wake = 0.

| Metric | 3B 90 s |
|---|---|
| elapsed / distance | 90.001 s / 2161.0 m |
| created / retired | 19291 / 19291 |
| max / end resident | 2601 / 2601 |
| end pending / max pending | 0 / 78 |
| min complete radius | 192.00 m |
| frames | 85720 |
| mean / p95 / p99 / max ms | 0.947 / 1.215 / 1.676 / **6.881** |
| frames >16.667 (movement) | **0 PASS** |
| water-body / topology / terrain-state wakes | 0 / 0 / 0 |
| derived mesh / collision | 19291 / 19291 |
| p5b3b transfers / transport wakes | **0 / 0** |
| p5b3b pending / stale refuse | **0 / 0** |
| p5b3b transport revision / loose mass delta | **0 / 0** |
| check.p5b3b_travel_physics | PASS_idle |
| p5b3a / p5b2c travel physics | PASS_idle / PASS_idle |

Ordinary traversal did not fire 3B. Mutation/wake-only. **P5b.3C stays
CLOSED.** No 300/900 unless travel fires 3B or a later cut adds persistent
long-haul work.

P5b.3B.2 loose-matter settling is **CERTIFIED** this cut. Test A digest
changed with stage identity: 3B `59a2725b6b89fd77` → 3B.2 `0ace5164520e5da0`.
Movement frames over 16.667 = 0 on EVERY-CUT cardinal.

### Test B — P5b.3B.2 90 s NE free-flight (this cut) — PASS

Receipt: `Docs/provenance_p5b3b2_streaming_soak_cert.txt`  
Trace: `Docs/provenance_p5b3b2_streaming_soak_trace.csv`

90 s travel, stop=0, return=0, persistent water, live 192 m / far 0.
`--cert-streaming-soak-p5b3b2`. 16.667 not relaxed. 300/900 **not run**:
ordinary traversal settle count = 0, settle wake = 0.

| Metric | 3B.2 90 s |
|---|---|
| elapsed / distance | 90.001 s / 2160.4 m |
| created / retired | 19291 / 19291 |
| max / end resident | 2601 / 2601 |
| end pending / max pending | 0 / 80 |
| min complete radius | 192.00 m |
| frames | 76169 |
| mean / p95 / p99 / max ms | 1.159 / 1.498 / 2.294 / **7.280** |
| frames >16.667 (movement) | **0 PASS** |
| water-body / topology / terrain-state wakes | 0 / 0 / 0 |
| derived mesh / collision | 19291 / 19291 |
| p5b3b2 settles / settle wakes | **0 / 0** |
| p5b3b2 pending / stale refuse | **0 / 0** |
| p5b3b2 settling revision / loose mass delta | **0 / 0** |
| check.p5b3b2_travel_physics | PASS_idle |
| p5b3b / p5b3a / p5b2c travel physics | PASS_idle / PASS_idle / PASS_idle |

Ordinary traversal did not fire 3B.2. Mutation/wake-only. **P5b.3C /
3B.3 / 16C stay CLOSED.** No 300/900 unless travel fires settling or a
later cut adds persistent long-haul work.
