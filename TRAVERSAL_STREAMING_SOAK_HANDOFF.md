# Traversal + Streaming Soak — Permanent Certificate

A worldgen stage is not certified just because its math is correct. It is
certified only if the player can keep moving through newly generated world
indefinitely, within bounded residency and frame-time limits.

Travel distance must grow indefinitely while resident work and memory remain
bounded around the player.

This is the standing matrix. Cardinal replacement and the long-haul soak are
**two different tests**. Passing one never substitutes for the other. Do not
narrow a future stage to “cardinal movement passed.”

P5b.2B, P5b.3, rainfall, and erosion stay **CLOSED**. This certificate does
not open them.

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

Soak capture (required):

```text
distance traveled
packages created
packages retired
resident package count
pending package count
oldest pending age
minimum complete radius
worker queue depth
memory usage (working set + private)
mean / p95 / p99 / max frame time
frames > 16.667 ms
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
- sprint + free-flight / highest normal traversal speed
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

`--soak-mode=walk|run|sprint|fly`  
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

## First landing (P5b.2A tip)

Parent pin `eae7db9f` / land `570c7be2`. Duration **90 s** (same metrics as the
5–15 min milestone; milestone still uses `--soak-duration-s=300` or `900`).

### Test A — P5b.2A cardinal (EVERY CUT)

`WORLDGEN_CARDINAL_REPLACEMENT PASS`. Digest `f6c20f2c4774451b` origin==return.
2601 packages. Movement frames over 16.667 ms = 0. Wake counters recorded;
water-body / topology ticks during travel = 0.

### Test B — 90 s NE free-flight soak at 24 m/s

Residency, backlog, and distance **PASS**. Frame gate **FAIL** (20 / 20079
frames over 16.667 ms, worst 60.687 ms). Distance 2160.1 m. Created == retired
19190. Max resident 2601. End pending 0. Oldest pending age 0.018 s. Min
complete radius 192 m. Water-body / topology ticks 0; 15 water bodies queried.
Working set grew 164 MB → 2.3 GB while the resident package count stayed
bounded — recorded, not used to weaken the frame gate.

The 16.667 / 2601 / exact-return gates were not relaxed. Soak FAIL is an
honest first-landing hitch under sustained travel, not a reason to treat
cardinal PASS as a soak.

## Closed

- P5b.2B porous storage / infiltration
- P5b.3 water→terrain mechanical / matter movement
- rainfall, groundwater, active 16B erosion, 16C remobilization, ecology
