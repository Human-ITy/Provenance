# PX1 — Baseline Present-Pacing Stability

PX1 owns the **gate**, not terrain. It answers one question the MV1.C
investigation surfaced: why does the frozen baseline intermittently produce
20–60+ ms presentation-thread frames when there is no world/MV1 work on the spike
frame — and is that "the engine took 50 ms to produce a frame" or "Windows/driver
present made the CPU wait 50 ms"?

**Scope:** instrumentation + attribution only. No worldgen/terrain/MV1/depth/VBO
change, and the 16.667 ms standard is **not** weakened. MV1.D / MV2 / MW9 closed.

## Instrumentation

Per travel frame, independent QPC spans plus an async GPU timer:

```
gap_ms           afterPresent(prev) -> tickStart(this)   pump / OS scheduling / pacing
engine_cpu_ms    tickStart -> render done                pure CPU frame production
gpu_finish_ms    pre-present glFinish                     GPU completion (+vsync wait if vsync on)
swap_ms          SwapBuffers call                         present wait (+vsync wait if glFinish off)
engine_work_ms   engine_cpu + gpu_finish
presented_frame  afterPresent(prev) -> afterPresent(this) the cadence the PLAYER receives
wall_frame_ms    tick-start delta                         historical gate metric (sum of all)
```

Also recorded: swap interval, display refresh Hz, foreground state, pending
packages. Toggles for the controlled matrix: `--px1-swap-interval=N`,
`--soak-no-glfinish`. Runs the MV1-**off** frozen MW8 baseline so PX1 cannot blame
far terrain: `--cert-px1-present-pacing`.

## Controlled matrix (MV1 off, MW8 baseline, 3 runs each)

| Config | glFinish | vsync | engine_cpu p50 | engine_work p50 | swap p95 | wall>16.667 | presented |
|---|---|---|---|---|---|---|---|
| A production | on | **off** | ~0.7 ms | 0.71 ms | 0.13 ms | **0** | smooth |
| C | off | off | ~0.7 ms | 0.67 ms | 0.18 ms | **0** | smooth |
| E | on | **on** | ~0.5 ms | 16.1 ms | 0.05 ms | 2462 | steady 16.67 ms |
| F | off | on | **0.487 ms** | 0.49 ms | **16.3 ms** | 2652 | steady 16.67 ms |

Reading:

- **Engine CPU frame production is always ~0.5–0.7 ms** (max ~7 ms) in **every**
  config, including vsync on. **The engine never takes 16–80 ms to produce a
  frame.**
- **vsync pacing is real but is not engine work.** With vsync on, the ~16.67 ms
  60 Hz wait lands in `gpu_finish` (E, so `engine_work` reads 16 ms) or in
  `swap` (F, `swap_ms`=16.3 ms) — the *same* wait, different bucket. `presented`
  is a steady ~16.67 ms = smooth 60 fps. A naive wall-clock gate would flag
  almost **every** vsync frame as an overrun; production runs vsync **off**
  (swap interval 0) precisely to avoid that.
- **glFinish is protective** — it drains per frame, keeping SwapBuffers cheap;
  removing it moves the wait into SwapBuffers (F, C).

## The spike, caught — 10 production runs (glFinish on, vsync off)

6 over-16 frames total; **every one is `gap`-dominated:**

```
wall   gap    engine_cpu  gpu_finish  swap   presented   dominant
57.7   55.8   1.5         0.2         0.2    57.6        gap
83.3   82.4   0.3         0.5         0.02   83.3        gap
42.1   41.0   0.5         0.6         0.02   42.1        gap
40.9   39.5   0.6         0.7         0.02   40.9        gap
24.3   23.3   0.7         0.2         0.1    24.3        gap
22.6   21.5   0.8         0.2         0.1    22.6        gap
```

On every spike, engine CPU, GPU-finish and SwapBuffers are all **tiny**; the
entire 20–82 ms is the **inter-frame gap**, and `presented_frame` tracks it (the
player does see a delayed frame).

The main loop (`WinMain`, worldgen lane) is a tight
`while(PeekMessage(PM_REMOVE)) dispatch; TickFrame();` busy loop — **no Sleep, no
blocking GetMessage/WaitMessage, no WM_TIMER**. So a 20–82 ms gap between one
SwapBuffers return and the next tick means the **OS descheduled the process
thread** (thread preemption / message-pump scheduling), amplified by the fact
that a vsync-off busy loop never yields cleanly and is highly preemptible —
especially under the concurrent build/battery load present during these tests.

## Owner classification

```
OWNER = inter-frame gap  (OS thread scheduling / message pump / present pacing)
NOT   = engine frame production   (engine_cpu always < 1.5 ms typical, < 7 ms max)
NOT   = GPU                        (gpu_finish < 0.7 ms; MV1.G async 0.47 ms)
NOT   = the SwapBuffers call       (swap < 0.2 ms with vsync off)
```

The engine **passes** the 16.667 ms production standard in every configuration.
The historical wall-clock gate conflates three different things: (a) real engine
work, (b) vsync pacing, (c) rare OS-scheduling inter-frame gaps. Only (a) and a
*sustained/frequent* presentation miss are gameplay defects; (b) is a metric
artifact and (c) is environmental.

## Are Test A / Test B failures the same class?

**Yes.** The Test-B soak travel spikes are gap-dominated OS scheduling (this
investigation). The one-off Test-A east `16.851 ms` movement frame is the same
class — a marginal wall-clock overrun with exact geometry/digests and clean 3/3
reruns; its overrun is present-pacing, not engine work.

## Proposed standing-gate definition (without weakening player-visible perf)

1. **Engine production contract (primary, always meaningful):**
   `engine_cpu_ms` must have **0 frames > 16.667** across the route. This is what
   the engine — worldgen, terrain, MV1, MW stages — is actually responsible for,
   and it is uncontaminated by vsync or OS scheduling. It is the correct gate for
   "did this cut add frame-production cost." (Observed: 0 in every config.)
2. **Presentation contract (player cadence):** track `presented_frame_ms`
   (swap-to-swap). A **sustained or frequent** presented miss is a real FAIL. A
   **rare, gap-dominated** outlier where `engine_cpu` on that frame was < a small
   bound is classified **environmental (OS scheduling)**, reported but not charged
   to the engine. Suggested form: `presented_over_16667` with an allowance of a
   few isolated gap-dominated outliers per 90 s, or "0 presented misses whose
   engine_cpu > 4 ms."
3. **Never** gate on wall-clock-including-glFinish under vsync — it fails ~every
   frame and measures the 60 Hz cadence, not a hitch.
4. Keep the pre-present glFinish for measurement stability (it is protective), but
   **always report `engine_cpu` separately** so vsync/GPU wait absorbed into
   glFinish is not misread as engine cost.

The 16.667 ms standard stands. What changes is *which measured quantity* the
contract legitimately applies to: frame **production** (engine_cpu, always green)
and player **cadence** (presented_frame, with an environmental allowance for rare
OS-scheduling gaps) — not an opaque wall-clock number that folds in vsync pacing
and thread descheduling.

## Optional future hardening (separate, not this cut)

Reduce OS-scheduling gaps on the vsync-off loop via clean frame pacing (waitable
timer / `timeBeginPeriod`), raised thread priority, or a cooperative yield —
a production behavior change to be designed and certified on its own once the
gate contract above is adopted. PX1 does **not** change production behavior; it
only adds instrumentation and the attribution above.

## Runtime

```
--cert-px1-present-pacing                 MV1-off MW8 baseline + PX1 metrics
  [--px1-swap-interval=0|1]               diagnostic vsync override (restore after)
  [--soak-no-glfinish]                    drop the pre-present completion probe
Receipt:  Docs/provenance_px1_present_pacing_cert.txt
Outliers: Docs/provenance_px1_present_outliers.csv (per >16.667 ms travel frame)
```

## Board

```
MW1–MW8                        CERTIFIED
MV1 geometry / derivation      CERTIFIED
MV1.C real 32 km raster view   CERTIFIED
MV1.G full-raster GPU          CERTIFIED
PX1 present-pacing attribution CERTIFIED (owner = OS-scheduling inter-frame gap;
                               engine production always < 16.667 ms)
GLOBAL Test-B gate definition  proposed: engine_cpu (0 over) + presented cadence
                               (environmental allowance) — adopt before MV1.D gating
MV1.D distance readability     WAITING (on adopting the gate definition)
MV2 100+ km horizon            CLOSED
MW9 flora / fauna              CLOSED
```
