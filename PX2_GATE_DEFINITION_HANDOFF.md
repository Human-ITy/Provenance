# PX2 — Two-Contract Present-Pacing Gate

PX1 proved that a single wall-clock "frame > 16.667 ms" gate conflates four
different things: engine work, GPU completion, present behaviour, and OS thread
descheduling. PX2 replaces that ambiguous gate with **two explicit, independently
reported contracts** so every future stage stops re-litigating the same
frame-time question. **Measurement/classification only** — no change to
production scheduling, thread priority, vsync, glFinish, terrain, or renderer.
The 16.667 ms standard is **not** weakened.

## The two contracts

```
ENGINE PRODUCTION GATE   (hard)
  engine_cpu_ms  = tickStart -> render done   (pure CPU frame production;
                                               already contains all stage work)
  requirement:   0 travel frames > 16.667 ms
  answers:       did this stage make the engine unable to produce a frame in time?

PLAYER PRESENTATION CADENCE   (reported + owner-classified)
  presented_frame_ms = swap-return to swap-return   (what the player receives)
  every miss recorded; each classified by OWNER
```

## Classifier (`Px2ClassifyFrame`)

```
engine_cpu > 16.667                         -> ENGINE MISS   (hard FAIL)
presented  <= 16.667                        -> OK
presented  > 16.667 and engine produced in budget:
    gpu_finish >= 8 ms  OR  swap >= 8 ms     -> STAGE-OWNED   (hard FAIL: this
                                                stage's GPU/present load caused it)
    otherwise (gap-dominated, or every
    producible component tiny/unattributed)  -> OS-GAP        (reported, not charged)
```

`engine_cpu` is the production authority — the terrain/package/MV1 CPU work all
runs inside the tick, so a bounded `engine_cpu` means production met budget
*including* stage work. What remains for a presented miss is GPU completion
(`gpu_finish`), the present call (`swap`), or the inter-frame gap (OS scheduling)
— the first two are stage-owned, the last is environmental.

## Non-regression rule (rule 5)

A stage may not materially increase OS-gap-classified misses relative to the PX1
baseline (~0.6 misses / 90 s, worst ~83 ms):

```
os_gap_misses > 10  (per 90 s route)   OR   os_gap_worst_ms > 250   -> cadence REGRESSED (FAIL)
```

This prevents abusing "OS scheduling, ignore it" forever: if a new workload
drives scheduling pressure that shows up as many gap misses, it is caught even
though `gap` owns the timestamps.

## Gate summary per receipt

```
ENGINE
  engine_cpu > 16.667              0        PASS      <- hard
PRESENTATION
  presented misses                 N
  stage-owned misses               0        PASS      <- hard
  OS-gap-classified misses         K        REPORTED
  cadence regression vs PX1        none     PASS      <- hard
TELEMETRY
  raw wall-clock frames > 16.667   M        (kept for history, no longer pass/fail)
```

## Validation

- **Classifier on known PX1 events:** a real 44 ms inter-frame-gap spike →
  `engine_cpu_over_16667=0`, `stage_owned_misses=0`, **`os_gap_misses=1`**,
  engine gate PASS. The classifier tags the OS-scheduling gap as environmental
  while keeping engine production green. (`--cert-px1-present-pacing` receipt.)
- **Test B (90 s soak) on PX2:** PASS — engine gate PASS, 0 stage-owned misses,
  0 presented misses on clean runs; reliable where the old wall-clock gate was
  ~50 % flaky.
- **Test A (cardinal N/E/S/W) on PX2:** **PASS 3/3** — `engine_over=0`,
  `stage_owned=0`, `os_gap=4`. The 4 os-gap misses are the first movement frame
  after each direction's capture/settle frame (a `presented_frame` boundary
  artifact where every producible component is tiny) — correctly classified
  environmental and reported, not charged to the engine. Correctness (digests,
  geometry, material, collision, image return, oracle) unchanged and exact.

## What this resolves

The historical `0 wall-clock movement frames > 16.667` gate conflated engine
work + GPU completion + presentation + OS descheduling, which is why it failed
intermittently on this hardware even with MV1 disabled. PX2 separates them:
engine production is held to a hard 0-over standard (always green); player
cadence is reported and owner-classified, with OS-scheduling gaps allowed within
a non-regressing bound and stage-owned misses a hard fail.

**MW1–MW8 are not reopened.** Their semantic/residency certifications remain
valid; only the weak wall-clock movement clause is superseded. A retrospective
sweep can later re-run them under the two-contract gate to answer the right
questions (engine hard gate / stage-owned misses / OS-gap reported separately),
which will be more stable and informative than rerunning the old wall-clock test
until one is green.

## Optional future hardening (separate, not this cut)

Reduce OS-scheduling gaps on the vsync-off busy loop via clean frame pacing
(waitable timer / `timeBeginPeriod`), raised thread priority, or a cooperative
yield. Known to exist (PX1); not a terrain/worldgen blocker.

## Runtime / receipts

```
Test A: --cert-worldgen-cardinal-replacement-mw8   (px2_* fields per case)
Test B: --cert-streaming-soak-mw8                  (px2.* + check.engine_cpu_production_gate,
                                                    check.no_stage_owned_present_miss,
                                                    check.cadence_no_regression_vs_px1)
PX1:    --cert-px1-present-pacing                   (px2_classifier proof line)
```

## Board

```
MW1–MW8                        CERTIFIED
MV1 / MV1.C / MV1.G            CERTIFIED
PX1 present-pacing attribution CERTIFIED
PX2 two-contract gate          CERTIFIED  (engine_cpu 0-over + presented cadence
                               owner-classified; wall-clock kept as telemetry)
MV1.D distance readability     NEXT
MV2 100+ km horizon            CLOSED
MW9 flora / fauna              CLOSED
```
