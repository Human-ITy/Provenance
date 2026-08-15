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
`9cae0794`). P5b.2B, P5b.3, rainfall, and erosion stay **CLOSED** on this
pin. This certificate does not open them.

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
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b2a
```

or `CERT_WORLDGEN_CARDINAL_REPLACEMENT.cmd` for the full stage ladder.

Latest play-stage receipt: `Docs/provenance_p5b2a_cardinal_replacement_cert.txt`

### B. Long-haul streaming soak (timed, no return)

Proves **bounded memory / residency under continuous travel**. No return is
required. Distance must grow while the resident set stays a moving window.

Default first landing: **90 s** wall-clock (same metrics as the 5–15 min
milestone). Milestone / release: `--soak-duration-s=300` or `900`.

```text
Build\x64_Release\ProvenanceClient.exe --cert-streaming-soak-p5b2a --soak-duration-s=90 --soak-mode=fly --soak-bearing=northeast --soak-speed-mps=24 --live-radius=192 --far-extent=0
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

`CERT_TRAVERSAL_EVERY_CUT.cmd` runs Test A on the latest play stage (P5b.2A).
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

## Closed

- P5b.2B porous storage / infiltration
- P5b.3 water→terrain mechanical / matter movement
- rainfall, groundwater, active 16B erosion, 16C remobilization, ecology
