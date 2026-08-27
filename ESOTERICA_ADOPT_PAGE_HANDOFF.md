# Esoterica adopt_page — Phase 17 orographic consume

## Status

Esoterica has a **single fail-closed `adopt_page` boundary** plus an **adjacent
canonical orographic context consume seam** (`AdoptContext`). Canonical
`orographic.phase17` production pages are admitted; tectonic-v3 and stale
revisions are refused. After adopt, render/collision sample
`bilinear(smooth) + sharp_from_features(carried definitions)`. There is no
second client-generated macro heightfield on this path.

Default `voxel_bridge` sessions remain tectonic-v3 `generated_chunk`. Live
opcodes `orographic_production_page` and `orographic_ecological_context` are
fail-closed unless the installed genesis is `orographic.phase17`. Cert/play
consume **banked** bytes (page digest `03f579fed39e4685…`) — the same JSON
the opcodes would emit.

```
MW8 RESUMED ON CANONICAL OROGRAPHIC CONTEXT —
ECOLOGY INDEPENDENT OF PAGE/RENDER SCALE
```

Page `(1,1)` stays `[1024,2048]²`. Context influence radius **4710.4 m**
(system spacing × reach). MW9 CLOSED.

## Adopt admits

- Matching genesis: `world`, `tectonic`, `orographic`, `orographic_version`
- Canonical: `terrain_law=orographic.phase17`, tectonic `b74f957a7fdb429a`
- Required feature kinds: peaks, ridges, saddles, spurs, divides, valleys
- Carrier representation v2 production_page JSON

## Adopt refuses

- Non-object page / missing genesis
- Genesis world / tectonic / orographic / orographic_version mismatch
- `terrain_law` mismatch against the installed identity
- Missing or empty required feature_definitions
- Does **not** rewrite the receiving identity to match the page

## Context consume (adjacent seam)

`AdoptContext` loads `orographic_ecological_context_v1`. It names SystemId /
RangeId / MassifId and carries ridge/divide, saddle/pass, basin/valley,
slope/aspect, exposure, windward/leeward, upstream/downstream. PeakId/RidgeId/
SaddleId/SpurId/ValleyId agree with the production page. MW8 queries this
graph; it does not read GradeToZ.

## Proofs

Headless: `CERT_ESOTERICA_ADOPT_PAGE.cmd`
→ `Build\x64_Release\ProvenanceClient.exe --cert-esoterica-adopt-page`
→ `Docs/provenance_esoterica_adopt_page_cert.txt`

MW8: `CERT_MW8_OROGRAPHIC.cmd`
→ `--cert-mw8-orographic` (consume PASS, mw8 PASS)

Play: `PLAY_ESOTERICA_ADOPT_PAGE.cmd` / `PLAY_MW8_OROGRAPHIC.cmd`
→ `--play-orographic-phase17` (stand on page `(1,1)` at 1536,1536)

Fixtures: `Data/Worldgen/orographic_phase17/`
(export: `python Tools/Worldgen/export_orographic_pages.py`)

## Wire

`voxel_bridge.py` methods `orographic_production_page` and
`orographic_ecological_context`. `_grade_at = tectonic.grade_at` is unchanged.

```
PYTHONPATH=. python voxel_bridge.py canonical_orographic
```
