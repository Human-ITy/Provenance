# Stage 16F.4 — Dynamic Occupancy + Hydraulic Topology (Fixed Terrain)

Stage 16F.4 answers one question: on **immutable terrain**, can water occupancy
change (`dry↔wet`) and can hydraulic components **grow / shrink / split /
merge** with deterministic lineage, without rediscovering the whole body set?

It does not rain, erode, remobilize sediment, mutate terrain, open P5b, or
extract a generic SimulationDomain / world-connectivity service.

## Freeze

Stage 16F.3 remains frozen at `76ad83694ea2205b0d77d9490b1701723c9a15b1` on
`provenance/client-spike`. Stage 16F.2 remains `cfc16d26`. Stage 16F.1 remains
`3701dc51`. Stage 16E ontology remains `bc3e680c`.

## Occupancy / topology model

- **Terrain** is immutable. Terrain digest is unchanged under every admitted
  occupancy transaction.
- **Occupancy** may go dry→wet or wet→dry only through an admitted transaction.
- **BodyId** names one *current* 4-connected hydraulic component. It is not a
  permanent matter identity.
- **Water provenance** (`Cell::waterIdentity`) is assigned when water is first
  admitted and survives split/merge independently of BodyId.
- Real **split/merge mint new deterministic component IDs** with explicit
  lineage. Lowest ID does not survive.
- Grow and shrink keep the existing BodyId.
- Connectivity rebuild walks only the affected neighborhood (certified max
  12 cells / 2 bodies examined). A player walking past a lake does not pay
  for reconstruction.

```text
occupancy mutation (admitted transaction)
    → recompute connectivity ONLY in the affected neighborhood
    → emit FWaterTopologyDelta
    → instantiate revised bodies
    → 16F.1 affected-body equilibration
    → 16F.2 eligible edge transfer
    → sleep
```

```text
Body 71 splits → Body 104, Body 105
lineage: 71 → {104,105}

Body 31 + Body 44 merge → Body 106
lineage: {31,44} → 106
```

## Discriminating fixtures (fixed terrain)

| Fixture | Result |
|---|---|
| pour onto adjacent dry cell | occupancy grows; BodyId kept |
| scoop last water from a bridge cell | one body splits into two new IDs |
| pour fills a dry bridge between two bodies | two bodies merge into one new ID |
| perimeter scoop | body shrinks without splitting |

## Certified results

- Stage 16E freeze: body `2352b000a56f499c`, connectivity `ba4849f7148a7297`.
- Stage 16F.3 freeze (disabled path): field `a7f6eebc0fc0579f`.
- Topology field digest (budget 1 == N == unbounded): `43068558cd0b4a8e`.
- Mass+container conserved: world `456493002` + bucket `9870000` == world
  `456302579` + bucket `10060423` (`466363002` both sides). Pour `200` in,
  scoop `190623` out; source loss == world/body gain on every receipt.
- Certified transactions 4/4 admitted. Fixtures: pour-grow / scoop-split /
  pour-merge / scoop-shrink. Downstream 16F.2 spill edges: 1.
- Local connectivity: max 12 cells visited, max 2 bodies examined, 4 rebuilds
  (one per admitted transaction). Idle complete: zero extra rebuilds.
- Controls: stale occupancy/topology revision refuse; partition-independent
  topology; cold==reload==unbounded; water provenance survives split/merge;
  wet cells only change through an admitted transaction; terrain digest
  unchanged.
- 192 m live radius: 2,601 packages; N/E/S/W eviction + exact return PASS.
- Movement frames over 16.667 ms: 0 (worst movement about 15.02 ms).
  Topology mutation is a separately timed receipt, not a traversal tax.
- Visual settle PASS; zero lower-frame sky.

## Player runtime

Run `PLAY_STAGE16F4_TOPOLOGY.cmd`, or:

```text
Build\x64_Release\ProvenanceClient.exe --play-stage16f4-topology
```

Stage 16F.4 is the latest stable runtime (menu closed). Press `M` for the stage
browser. Topology fixtures may still be finishing across frames; ordinary
walking does not reconstruct connectivity.

## Re-run

Run `CERT_STAGE16F4_TOPOLOGY.cmd`. It executes analytical authority,
settled player-scale visual coverage, and full cardinal replacement gates.

```text
Build\x64_Release\ProvenanceClient.exe --cert-stage16f4-topology
Build\x64_Release\ProvenanceClient.exe --cert-stage16f4-topology-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-stage16f4
```

## Boundaries (CLOSED)

- terrain mutation / excavation changing a basin
- rainfall as weather
- terrain-water feedback / erosion / sediment remobilization
- **P5b**
- generic SimulationDomain / world-connectivity framework extract
