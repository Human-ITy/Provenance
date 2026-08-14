# P5b.1 — Terrain Mutation → Water Response (One-Way)

P5b.1 answers one question: when **authoritative terrain** changes, can the
already-certified water system respond to the new geometry without losing mass,
identity, topology, or revision coherence?

It does not let water erode terrain, transport sediment, collapse banks, rain,
infiltrate, open groundwater, remobilize sediment, run ecology, extract a
generic SimulationDomain, or open **P5b.2** / **P5b.3**.

## Freeze

P5b.1 coupling landed at `4e6db8433fb136ab2068bab114a04d4ab265e935` on
`provenance/client-spike`. Player-path certification is
`5f4c075d7ef0b6af192a3afe7e088cd493e842aa`. Stage 16F.4 remains frozen at
`3d84eac4b169c8a1e2b0f3971695b811bd9a5a61`. Disabled P5b.1 must reproduce that
field exactly.

```
16F.4  dynamic hydraulic topology       CERTIFIED
P5b.1  terrain → water coupling          CERTIFIED
P5b.2  reverse state coupling            CLOSED
P5b.3  reverse matter/erosion coupling   CLOSED
```

```
P5b off → exact 16F.4 field digest 43068558cd0b4a8e
```

A terrain mutation does not directly manipulate water. It changes world truth,
issues a bounded causal receipt, and the hydraulic domain responds.

## Coupling model (one-way)

```
authoritative terrain mutation
  → terrain revision R → R+1
  → identify affected hydraulic neighborhood (from the mutation receipt)
  → invalidate water geometry assumptions derived from R
  → re-evaluate occupancy against the new terrain void
  → 16F.4 local topology change if needed
  → 16F.1 equilibration
  → 16F.2 certified spill/transfer
  → sleep
  → publish terrain + water as one coherent pair
```

Affected region is the mutated terrain cells plus nearby wet cells / body IDs.
A shovel bite does not globally reconsider ~6500 bodies. Topology/water work
runs on mutation/wake events, not as a continuous traversal tax.

Two ledgers stay independent:

- **Water grams** before == after (occupancy / surface / topology may change).
- **Terrain matter** conserves separately (`column + held`).
- No water inside solid terrain. Water occupancy change ≠ terrain matter change.
- If a fill/raise has no admissible conserved water resolution → **refuse**
  (never delete water).

**Hard presentation rule:** do not publish the new terrain surface with a water
projection derived from the previous terrain revision. Retain the previous
coherent pair until water has responded, then publish both together.

Reverse coupling remains **CLOSED**.

## Discriminating fixtures

| Fixture | Result |
|---|---|
| lower a retaining sill | closed pond outlet opens; conserved water grows into the new void; BodyId kept (grow) |
| dig a channel between two wet regions | two bodies merge into one new deterministic ID |
| raise/fill a water-bearing cell | void decreases; displaced water stays in the remaining body; never deleted |
| remove a floor beneath water | cavity deepens; same mass occupies the new void; identity preserved |
| **negative:** terrain far from all water | exact 16F.4 water field `43068558cd0b4a8e` |
| fill refuse | no admissible conserved resolution → refuse; water untouched |
| **player cut** (pick/shovel legal receipt) | retaining edge lowered; water grows into new void; BodyId kept (grow); coherent publish |
| **player fill refuse** | shovel fill into occupied water with no admissible conserved resolution → refuse; terrain and water unchanged |

## Player-path lane

Ordinary hand/tool mutations use the same coupling as the fixtures. They are
not a second architecture.

```
player makes one legal terrain mutation beside water
  → normal terrain mutation receipt (pick/shovel action id + revision)
  → P5b.1 sees affected hydraulic neighborhood
  → water responds
  → terrain + water publish coherently
```

Certified:

- shovel cuts retaining edge → terrain matter conserved (`column + held`);
  water occupancy grows; BodyId kept (grow); water grams conserved; no
  mixed-revision frame; neighborhood stays local.
- shovel fill into occupied water with no admissible conserved resolution →
  refuse; terrain unchanged; water unchanged.

`--cert-p5b1-terrain-water` runs this lane. It does not open P5b.2 (repeated
player interaction) or P5b.3.

## Certified results

- Stage 16F.4 freeze (disabled path): field `43068558cd0b4a8e`.
- P5b.1 field digest (budget 1 == N == unbounded): `b1340afeef311fd8`.
- Water mass conserved: `456302579` before == after (container unchanged).
- Terrain matter conserved independently: world delta + held delta == 0.
- Certified transactions 4/4 admitted. Fixtures: sill-grow / channel-merge /
  fill-shrink / floor-geometry. Fill-refuse control admitted 0 and deleted 0.
- Local neighborhood: max 14 cells visited, max 2 bodies examined, 3 connectivity
  rebuilds (sill/channel/fill). Floor is geometry-only. Idle complete: zero extra rebuilds.
- Player path: shovel legal receipt cuts retaining edge (grow; 14 cells, 1
  body; coherent publish); shovel fill into occupied water refuses with
  terrain and water unchanged. P5b.1 fully done.
- Controls: stale terrain/water revision refuse; partition-independent channel
  merge; cold==reload==unbounded; published terrain revision == collision
  revision == render revision; no water inside solid terrain.
- Reverse coupling, rainfall, infiltration, groundwater, water erosion, bank
  collapse, 16C remobilization, ecology, P5b.2, P5b.3, SimulationDomain: CLOSED.
- Player visual: `P5B1_TERRAIN_WATER_PLAYER_VISUAL PASS` (90 settled frames,
  2601 resident packages, 0 pending, water_to_terrain=0, published_pair_coherent=1).
- Cardinal N/E/S/W 192 m: all PASS. Residency complete 192 m. Movement frames
  over 16.667 ms = 0. Resident package digest `ce1866ffc7cf5b5a` origin==return.

## Player runtime

Run `PLAY_P5B1_TERRAIN_WATER.cmd`, or:

```text
Build\x64_Release\ProvenanceClient.exe --play-p5b1-terrain-water
```

P5b.1 is the latest stable runtime (menu closed). Press `M` for the stage
browser. Terrain→water fixtures finish on mutation/wake events; ordinary
walking does not reconstruct connectivity.

## Re-run

Run `CERT_P5B1_TERRAIN_WATER.cmd`. It executes analytical authority,
settled player-scale visual coverage, and full cardinal replacement gates.

```text
Build\x64_Release\ProvenanceClient.exe --cert-p5b1-terrain-water
Build\x64_Release\ProvenanceClient.exe --cert-p5b1-terrain-water-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b1
```

## Boundaries (CLOSED)

- **P5b.2** reverse state coupling (saturation/moisture). Repeated player
  interaction around water stays closed.
- **P5b.3** reverse matter/erosion coupling (water→terrain mechanical effects)
- water erodes terrain / flow transports sediment / bank collapse
- rainfall as weather / infiltration / groundwater
- active 16B erosion / 16C sediment remobilization
- ecology
- generic SimulationDomain extract
