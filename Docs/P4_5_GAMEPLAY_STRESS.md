# P4.5 — Gameplay-scale stress floor

**Law:** Increasing historical world complexity must not proportionally increase
recurring frame cost.

```
200 old holes ≈ 2 old holes when unchanged
500 settled chips ≠ 500 integrating
100 NPC/world receipts with affect=0 → terrain work = 0
```

## Combined load (simulated)

Player moving continuously + large resident RANGE + many historical EditedRegions +
sleeping MatterBodies + some active falling bodies + SupportBelow + periodic dig/place +
NPC/combat/weather/world ticks + few terrain-affecting radial events.

## Logged causes of work

```
frame_ms, resident_memory,
HF_remesh_count/ms, D2_rebuild_count/ms,
occupancy_mutation_count/ms, SupportBelow_count/ms,
active_bodies, sleeping_bodies,
terrain_refetches, EditedRegion changes,
receipts_seen, receipts_ignored_for_terrain, terrain_wakes
```

## Burst gates

| Burst | Expect |
|---|---|
| 50 unrelated receipts / frame | no terrain wake (D2/HF/occ/ER/refetch = 0) |
| 1 scoped terrain explosion | bounded dirty (≤9 cells, D2 ≤24) |
| 10 simultaneous scoped terrain actions | dirty cells ≈ union of regions — not 10 global rebuilds |

## Cert

```bat
Build\x64_Release_geocert\ProvenanceClient.exe 127.0.0.1 8765 --cert-stress
```

Aliases: `--cert-gameplay-stress`, `--cert-stress-floor`.  
Artifact: `%TEMP%\provenance_gameplay_stress_cert.txt`.

**Not in scope:** P5 water.
