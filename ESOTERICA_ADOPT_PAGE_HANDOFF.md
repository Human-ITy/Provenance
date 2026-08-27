# Esoterica adopt_page — Phase 17 orographic consume

## Status

Esoterica now has a **single fail-closed `adopt_page` boundary**. Canonical
`orographic.phase17` production pages are admitted; tectonic-v3 and stale
revisions are refused. After adopt, render/collision sample
`bilinear(smooth) + sharp_from_features(carried definitions)`. There is no
second client-generated macro heightfield on this path.

Default `voxel_bridge` sessions remain tectonic-v3 `generated_chunk`. The live
opcode `orographic_production_page` is fail-closed unless the installed genesis
is `orographic.phase17` (`canonical_orographic` scenario). Cert/play consume
**banked production_page bytes** (digest `03f579fed39e4685…`) — the same JSON
the opcode would emit. Esoterica does not yet request that opcode at runtime.

MW8 resume is **HOLD**. Adopted page `(1,1)` fails alpine / windward /
wet-basin gates (0 alpine, 0 windward/leeward split, 0 basin wetland).
Do not recertify the 64 km microscope at e8155fa3. MW9 CLOSED.

MW9 remains CLOSED.

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

## Proofs

Headless: `CERT_ESOTERICA_ADOPT_PAGE.cmd`
→ `Build\x64_Release\ProvenanceClient.exe --cert-esoterica-adopt-page`
→ `Docs/provenance_esoterica_adopt_page_cert.txt`

Play: `PLAY_ESOTERICA_ADOPT_PAGE.cmd`
→ `--play-orographic-phase17` (stand on page `(1,1)` at 1536,1536)

Fixtures: `Data/Worldgen/orographic_phase17/`
(export: `python Tools/Worldgen/export_orographic_pages.py`)

## Wire

`voxel_bridge.py` method `orographic_production_page` returns the same JSON
the client adopts. `_grade_at = tectonic.grade_at` is unchanged.

```
PYTHONPATH=. python voxel_bridge.py canonical_orographic
```
