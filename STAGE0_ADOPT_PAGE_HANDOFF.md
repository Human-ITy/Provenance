# Native Stage0 consumes orographic.phase17 — MW8 certification ported

```
NATIVE STAGE0 CONSUMES OROGRAPHIC.PHASE17 —
MW8 CERTIFICATION PORTED TO PRODUCT CLIENT
```

MW9 CLOSED. Native Stage0 is the product consume client. ProvenanceEsoterica
`provenance/client-spike` `2c5d9823` remains the certified **reference** — do
not rewrite that tree; copy the contract, not its heightfield.

## Architecture

```
canonical engine authority
        ↓
orographic.phase17 page + feature context
        ↓
native Stage0 authority-admission seam  (AdoptPage::Adopt / AdoptContext)
        ↓
native render / collision / streaming   (WorldGenesis v11 — UNCHANGED)
        ↓
MW8 QueryContext
```

WorldGenesis v11 library worlds stay distinguishable from `orographic.phase17`.
This consume path never reinterprets v11 pages as phase17. Live v11 streaming
does not yet emit `orographic.phase17` pages — **HOLD** that emit gap. Consume
is certified against the banked production_page / ecological context bytes.

## Preserved (native Stage0)

- `PLAY_PROVENANCE_STAGE0_WORLD.cmd` + Worlds UI
- world library create/load/delete
- streaming, page scheduling, controls, map/compass/ruler
- native Esoterica rendering, collision, grounding
- default library world `provenance-stage0-genesis-010` landing near **−102000, 58000**
- `_grade_at = tectonic.grade_at` (voxel_bridge binding unchanged)
- GradeToZ remains presentation; MW8 does not read it

## Ported (contract only)

- `Code/Applications/ProvenanceClient/AdoptPage.h` — single fail-closed admission
- genesis / tectonic / orographic / orographic_version / terrain_law stamp
- stale/v3 refuse (does not rewrite receiving identity)
- `orographic.phase17` feature payload (PeakId/RidgeId/SaddleId/SpurId/DivideId/ValleyId)
- canonical `QueryContext`
- MW8 ecological regimes from hydroclimate + regolith + drainage + exposure

`AdoptPage::SampleGrade` / `SampleZ` reconstruct the **banked carrier** for
consume certification only. `SampleGroundZBase` does **not** call them.

## Proof

Banked identity:

```
genesis = orographic.phase17
tectonic = b74f957a7fdb429a
page = (1,1)
page digest = 03f579fed39e4685…
```

Headless:

```
CERT_STAGE0_ADOPT_PAGE.cmd      → --cert-stage0-adopt-page
CERT_STAGE0_MW8_OROGRAPHIC.cmd  → --cert-mw8-orographic
Docs/provenance_stage0_adopt_page_cert.txt
```

Opt-in play (does not replace native landing):

```
PLAY_STAGE0_OROGRAPHIC_PHASE17.cmd  → --play-orographic-phase17
```

Native product play remains:

```
PLAY_PROVENANCE_STAGE0_WORLD.cmd
```

## Remaining HOLD

`live_v11_orographic_phase17_emit=HOLD` — WorldGenesis v11 streaming does not
yet emit `orographic.phase17` pages. Do not fake live v11 as phase17.

## MW9

CLOSED. No flora/fauna, vegetation placement, live weather, glaciers, or
ecology simulation.
