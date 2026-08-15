# P5b.2A — Water → Terrain Material State (Reverse State Coupling Only)

P5b.2A answers one question: when **authoritative water contact** changes, can
terrain **material state** respond without moving terrain mass, transferring
water mass, changing geometry, or opening infiltration?

Contract:

> Water may change how terrain matter behaves, but not how much terrain matter exists.

It does not open **P5b.2B** (porous storage / infiltration), **P5b.3**
(water-induced terrain matter movement), erosion, sediment creation, bank
collapse, 16C remobilization, rainfall, groundwater, ecology, or a generic
SimulationDomain extract.

## Freeze

P5b.2A coupling landed at `570c7be27260289eb26a3608d7aed5416c929449` on
`provenance/client-spike`. P5b.1 remains frozen at pin `e64a4df3`
(player-path land `5f4c075d`, coupling land `4e6db843`). Stage 16F.4 remains
frozen at `3d84eac4`.

```
16F.4   dynamic hydraulic topology       CERTIFIED / FROZEN
P5b.1   terrain → water coupling          CERTIFIED / FROZEN
P5b.2A  reverse state coupling            CERTIFIED
P5b.2B  porous storage / infiltration     CLOSED
P5b.3   reverse matter/erosion coupling   CLOSED
```

```
P5b.1 off  → exact 16F.4 field digest 43068558cd0b4a8e
P5b.2A off → exact P5b.1 field digest b1340afeef311fd8
P5b.2A off → exact P5b.1 state digest d3bd4455d64ca895
P5b.2A on  → state digest 176ffe1c3845f721 (budget 1 == N == unbounded)
```

## State model

Water never writes terrain. A contact notice is compiled, a material wetting
query is evaluated, and an authoritative receipt is applied:

```
FTerrainMaterialStateDelta {
  TransactionId;
  TerrainCell;
  MaterialId;
  MoistureBefore / MoistureAfter;
  SaturationBefore / SaturationAfter;
  CohesionModifierBefore / CohesionModifierAfter;
  TerrainRevisionBefore / TerrainRevisionAfter;
  WaterBodyId;
  WaterRevision;
}
```

Open state only:

| Field | Meaning |
|---|---|
| surface wetness | 0..1 contact film |
| moisture content | 0..1 held moisture (no mass transfer) |
| saturation | 0..1 fill fraction of the moisture store |
| cohesion modifier | 1.0 dry → reduced when wet |
| permeability flag | modifier only — not infiltration flow |

Temperature is skipped. Quantized 0..1000 internally for determinism.

Pipeline:

```
water body/contact revision
  → affected terrain neighborhood
  → material wetting query
  → FTerrainMaterialStateDelta receipt
  → revision-stamped presentation/physics consequences
```

Dirt / sandstone accept moisture. Rock (granite / quartz / shale) may take a
thin surface film only. Wetland dirt stays saturated while contact is present.
Contact-loss begins a drying transition; it does not snap to dry and does not
remove water grams.

## Discriminating fixtures

| Fixture | Result |
|---|---|
| lake edge wets adjacent dirt | surface wetness, moisture, saturation rise; cohesion falls |
| wetland keeps shallow soil saturated | moisture = saturation = wetness = 1.0 while contact remains |
| river contact wets exposed bank | bank dirt wets; grams and geometry unchanged |
| water removed → drying | contact-loss receipt; wetness/moisture/saturation fall but stay > 0 |

## Certified results

- Disabled P5b.1 path: exact 16F.4 field `43068558cd0b4a8e`.
- Disabled P5b.2A path: exact P5b.1 field `b1340afeef311fd8`, state `d3bd4455d64ca895`.
- Enabled state digest (budget 1 == N == unbounded): `176ffe1c3845f721`.
- Water grams unchanged: `456302579` before == after.
- Terrain grams unchanged: `353566960429` before == after (P5b.1 post-mutation field; P5b.2A adds zero).
- Terrain geometry and 16F.4/P5b.1 topology unchanged.
- Fixtures 4/4 admitted: lake-edge wet / wetland saturate / river bank / contact-loss drying.
- Locality: max 4 cells visited, max 1 body examined.
- Cold == reload == unbounded. Partition-invariant. Stale revision refuses.
- Player visual: `P5B2A_TERRAIN_STATE_PLAYER_VISUAL PASS` (90 settled frames, 2601 resident packages, 0 pending, water_to_terrain_state=1, water_to_terrain_matter=0).
- Cardinal N/E/S/W 192 m: all PASS. Residency complete 192 m. Movement frames over 16.667 ms = 0. Resident package digest `f6c20f2c4774451b` origin==return.

## Player runtime

Run `PLAY_P5B2A_TERRAIN_STATE.cmd`, or:

```text
Build\x64_Release\ProvenanceClient.exe --play-p5b2a-terrain-state
```

P5b.2A is the latest stable runtime (menu closed). Press `M` for the stage
browser. HUD shows wetness / moisture / saturation / cohesion / permeability
flag. P5b.2B and P5b.3 stay closed.

## Re-run

Standing traversal matrix (does not open P5b.2B / P5b.3):
`TRAVERSAL_STREAMING_SOAK_HANDOFF.md`. Cardinal replacement is Test A
(EVERY CUT). Long-haul soak is Test B (`CERT_STREAMING_SOAK.cmd`). Passing
cardinal movement is not a soak.

Run `CERT_P5B2A_TERRAIN_STATE.cmd`. It executes analytical authority,
settled player-scale visual coverage, and 192 m cardinal replacement.

```text
Build\x64_Release\ProvenanceClient.exe --cert-p5b2a-terrain-state
Build\x64_Release\ProvenanceClient.exe --cert-p5b2a-terrain-state-visual
Build\x64_Release\ProvenanceClient.exe --cert-worldgen-cardinal-replacement-p5b2a
```

Artifacts:

- `Docs/provenance_p5b2a_terrain_state_cert.txt`
- `Docs/provenance_p5b2a_terrain_state_receipts.csv`
- `Docs/provenance_p5b2a_terrain_state_visual_cert.txt`
- `Docs/provenance_p5b2a_cardinal_replacement_cert.txt`

## Boundaries (CLOSED)

- **P5b.2B** porous storage / infiltration (real water-mass transfer into terrain)
- **P5b.3** water→terrain mechanical / matter movement
- water erodes terrain / flow transports sediment / bank collapse
- rainfall as weather / groundwater
- active 16B erosion / 16C sediment remobilization
- ecology
- generic SimulationDomain extract
