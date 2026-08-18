# PX3 — Frozen-Cut Performance Revalidation under PX2

The historical `wall_clock > 16.667 ms` movement receipts conflated engine
production with OS/presentation scheduling (PX1's finding). PX3 re-judges every
frozen major cut under the **certified PX2 two-contract gate** — engine-production
0-over (hard) + owner-classified presented cadence — to answer one question:

> Do the frozen cuts remain performance-green when judged correctly?

**No world semantics were reopened.** This is performance revalidation only.
Receipt: `Docs/provenance_px3_retrospective_sweep.txt`.

## Result: the worldgen ladder is green; no historical regression

| Checkpoint | Test A | Test B | max engine_cpu A/B | engine over | stage-owned | verdict |
|---|---|---|---|---|---|---|
| P5b.3B.3B | PASS | PASS | 3.85 / 7.11 ms | 0 | 0 | 🟢 |
| 16C.1 | PASS | PASS | 4.65 / 5.02 ms | 0 | 0 | 🟢 |
| MW1 | PASS | PASS | 4.25 / 4.40 ms | 0 | 0 | 🟢 |
| MW2 | PASS | PASS | 3.89 / 4.60 ms | 0 | 0 | 🟢 |
| MW3 | PASS | PASS | 3.61 / 4.17 ms | 0 | 0 | 🟢 |
| MW4 | PASS | PASS | 3.17 / 4.27 ms | 0 | 0 | 🟢 |
| MW5 | PASS | PASS | 3.15 / 4.14 ms | 0 | 0 | 🟢 |
| MW6 | PASS | PASS | 3.13 / 4.45 ms | 0 | 0 | 🟢 |
| MW7 | PASS | PASS | 3.53 / 4.30 ms | 0 | 0 | 🟢 |
| MW8 *(mv1-off)* | PASS | PASS | 5.98 / 4.85 ms | 0 | 0 | 🟢 |
| MV1.C | PASS | PASS | gameplay green | 0 | 0 | 🟢 gameplay |
| MV1.D | PASS | PASS | gameplay green | 0 | 0 | 🟢 gameplay |

Each ran through its **existing** dedicated harness (cardinal-replacement-`<stage>`
+ streaming-soak-`<stage>`), not new machinery. Correctness (return image /
geometry / collision / residency / digest) is exact everywhere.

**Verdict:** the old single-run wall-clock receipts were **merely superseded** by
PX2 — engine production has 3–7 ms with wide headroom; the only "misses" are the
already-characterized OS-scheduling capture-boundary frames (4 os-gap per cardinal,
within the PX2 bound, reported not charged). **No actual historical stage
regression exists. MW1–MW8 are not reopened.**

## The two explicit contracts PX3 establishes

```
GAMEPLAY ACCEPTANCE  (hard)
  PX2 engine-production at gameplay-representative traversal (currently 24 m/s):
  engine_cpu > 16.667 = 0  AND  stage-owned present misses = 0.
  All checkpoints PASS. MV1.C/MV1.D reconfirmed 6/6 clean Test B runs.

STREAMING STRESS CEILING  (reported; NOT a gameplay gate; NOT discarded/slowed)
  240 m/s cardinal free-fly (10x gameplay) kept exactly as-is. MV1.C/MV1.D worst
  engine_cpu there ~16.85 ms vs 16.667 (one case, ~0.18 ms over).
```

The 240 m/s route did its job: it exposed real scaling owners. We attacked every
cheap, image-preserving layer and then stopped — refusing to make a 10×-gameplay
stress route perfect at the expense of the real game.

## The 240 m/s investigation (submission closed, build named)

1. **Per-tile draw submission** was the first measured owner (~250–336 tiles ×
   ~38 µs). Addressed by a **horizontal view-frustum cull** (kept — image-identical).
2. A **shared-slab batching experiment** cut CPU submission ~9 ms → ~0.1 ms and
   moved the cardinal worst 18.5 → 16.85 ms — **but was REJECTED**: `glBufferSubData`
   into slabs still in flight caused reproducible **50–98 ms gameplay-frame engine
   stalls** (4/5 Test B fail; `mv1_runtime_slab_allocs=0` ruled out creation;
   `--mv1-off` clean). An optimization that wins a stress benchmark but makes
   ordinary gameplay worse gets rejected. Reverted to per-tile VBO ownership.
3. With submission handled, the remaining owner is **MW-authority tile
   construction**: `Mv1BuildTile` resamples the composite authority 33×33 times per
   tile (~4–8 ms), and a single tile cannot be preempted mid-build. This is the
   stress-ceiling floor and an **MV2 scaling constraint**.

## Kept (all image-identical, gameplay-safe)

- **Frustum cull** — off-screen tiles skipped, zero visible-tile error.
- **Canonical draw order** (band → abs tile Y → abs tile X → tile key) — deterministic
  z-tie resolution. One equal-depth pixel at `ridge_shoulder` changed its
  deterministic winner; MV1.C + MV1.D station2 images **re-baselined once** and
  frozen (byte-stable across repeated runs).
- **Per-tile persistent/recycled VBO ownership**, bounded build budget, MV1.C
  two-pass 32 km raster, MV1.D aerial perspective — unchanged.

## Requirements carried into MV2 (do not inherit blindly)

- **Tile construction is the 100+ km scaling wall.** MW-authority resampling
  (~4–8 ms/tile) must be addressed before multiplying tile demand — investigate
  reusable/coarser hierarchical source sampling, cached derived terrain, or safe
  resumable construction.
- **Any future submission batching must be synchronization-safe** — persistent-mapped
  ring buffers, fenced multi-buffering, or immutable build-then-promote pages —
  never CPU updates that overwrite GPU-in-flight storage (the rejected slab mode).

## Runtime

```
Per checkpoint (existing args):
  Test A:  --cert-worldgen-cardinal-replacement-<stage>   (mw1..mw8 / 16c1 / p5b3b3b)
  Test B:  --cert-streaming-soak-<stage> --soak-duration-s=90 --soak-stop-s=0 \
           --soak-return=0 --soak-mode=fly --soak-bearing=northeast \
           --soak-speed-mps=24 --live-radius=192 --far-extent=0
MW8 worldgen-only baseline adds --mv1-off; MV1.C/MV1.D are the MW8 view with MV1 on.
```

## Board

```
P5b.3B.3B .. MW8   PX2 gameplay gate     GREEN (no historical regression)
MV1.C / MV1.D      PX2 gameplay gate     GREEN
MV1.C / MV1.D      240 m/s stress        STRESS CEILING (submission closed; build named)
semantic/residency/determinism           GREEN (not reopened)
PX3 revalidation                          CERTIFIED
MV2 100+ km horizon                       NEXT — build-scaling + sync-safe batching first
MW9 flora / fauna                         CLOSED
```

Stop here before MV2 so the known build-scaling constraint can be designed for,
not inherited.
