# P4.3 — Terrain residency / invalidation floor

**Law:** World activity does not imply terrain activity. Terrain presentation wakes only when terrain-relevant state changes.

```
receipt arrives
  ↓
does it affect terrain-visible state?
  ├─ NO → terrain does nothing
  └─ YES → identify exact scope → dirty only affected cells/regions
```

Forbidden: `world_revision` / informational tick → broad remesh / refetch / rebuild.

## Affect (orthogonal to scope)

`ClassifyTerrainAffect` / `ApplyActivityReceipt`:

| affect / kind | Terrain wake? |
|---|---|
| `affect=0`, world_tick, npc, inventory, combat, spell_tick, weather_info, ai, body_move, network chatter | **No** |
| occupancy / material / water / structure / explicit carve·place | **Yes** — scoped dirty when bounds present; fail-closed global HF only when affect≠0 and scope unknown |

## Counters

- `perfD2Rebuilds`, `perfHfRebuilds`
- `perfOccRebuilds` (new lattice seeds)
- `perfTerrainRefetches` (QueueColumn when occupancy already present)
- `perfEditedRegionChanges`
- `residencyIgnoredReceipts` / `residencyTerrainReceipts`

## Cert

```bat
Build\x64_Release_geocert\ProvenanceClient.exe 127.0.0.1 8765 --cert-residency
```

Aliases: `--cert-terrain-residency`, `--cert-invalidation`.  
Artifact: `%TEMP%\provenance_residency_invalidation_cert.txt`.

Idle burst (no terrain edits + unrelated activity + walk on resident pad):

```
D2=0 HF=0 occ=0 refetch=0 ER=0
```

One real mutation: scoped D2 dirty (≤9 cells), bounded D2 work, ER change, remainder cached.
