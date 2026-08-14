# Stage 16F.1 — Body-Local Equilibration

Stage 16F.1 relaxes water **inside** already-certified `FPresentWaterBody`
records. It redistributes conserved occupancy among static occupied cells under
a body-class surface rule. It does not move water between bodies, propagate
rivers across outlets, rain, pour, couple terrain, erode, move sediment, or
open P5b / 16F.2 / 16F.3.

## Causal chain

```text
Stage 16E body ontology (frozen bc3e680c)
    → static occupied cells (Stage 16D freeze 061bec0b)
    → body-local surface rule + terrain void + conserved water amount
    → relax toward one internally consistent body state
    → no inter-body transfer
```

## Class rules

| Type | Rule |
|---|---|
| lake | shared equilibrium plane |
| wetland | shallow constrained ponding field (wetland depth cap) |
| river | monotone downstream profile (16A flood-rank + 16D depth law) — **not** a flat plane |
| mixed | lake/river/wetland sub-rules on kind subsets; interface clamps; no inter-kind mass transfer |

## Contract

Equilibration may redistribute water inside a body, but it may **not** change
body identity, topology, total water, terrain, or connectivity.

Strongest gates:

- water mass before == water mass after (global and per-body)
- body ID / type / occupancy mask / spill-inlet-outlet graph unchanged
- terrain digest unchanged
- equilibration disabled → exact 16E / 16D state
- budget 1 == budget N == unbounded final field digest
- cold start == reload == reverse-schedule final state
- zero water → zero work; already-equilibrated → zero mutation
- stale body revision → refuse

Lesson: **budget changes convergence time, NOT final equilibrium.**

## Certified results

- Stage 16E freeze: body `2352b000a56f499c`, connectivity `ba4849f7148a7297`.
- Stage 16D freeze (disabled path): water `636d01ba00d3dffe`, occupancy `880a46fcdca2f8a4`.
- Field digest (budget 1 == N == unbounded): `af76b0826a8c8d22`.
- Mass conserved: `456363002` units before == after; per-body mass conserved.
- Bodies 6,515 — mutated 4,474 / already-eq 2,041 / zero-water 0.
- Controls: disabled ≡ 16E; stale revision refuse; cold==reload==unbounded;
  reverse schedule independent; occupancy mask / terrain / spill graph unchanged.
- 192 m live radius: 2,601 packages; N/E/S/W eviction + exact return PASS.
- Movement frames over 16.667 ms: 0 (worst movement about 12.87 ms).
- Visual settle PASS; zero lower-frame sky.

## Player runtime

Run `PLAY_STAGE16F1_BODY_EQUILIBRATE.cmd`, or:

```text
Build\x64_Release\ProvenanceClient.exe --play-stage16f1-body-equilibrate
```

Stage 16F.1 is the latest stable runtime (menu closed). Press `M` for the stage
browser. Equilibration is worker-budgeted across frames (48 bodies/tick).

## Boundaries (CLOSED)

- inter-body transfer
- river propagation across outlets
- rainfall / pours
- terrain coupling / erosion / sediment motion
- **P5b**
- **Stage 16F.2 / 16F.3**

Frozen baselines: 16E ontology `bc3e680c`; 16D occupancy `061bec0b`.

## Re-run

Run `CERT_STAGE16F1_BODY_EQUILIBRATE.cmd`. It executes analytical authority,
settled player-scale visual coverage, and full cardinal replacement gates.
