# Stage 16F.3 — External Conserved Body Transfer

Stage 16F.3 answers one question: can something **outside** the water solver
add or remove conserved water from **one existing body**, then let 16F.1 / 16F.2
restore equilibrium without bypassing body authority?

It does not change the occupancy mask, create puddles, split or merge bodies,
grow wet cells, retire dry cells, overtop channels into new terrain, excavate
basins, open 16F.4 topology-changing water, open P5b, rain, disturb arbitrarily,
couple terrain, erode, remobilize sediment, or extract a generic SimulationDomain.

## Freeze

Stage 16F.2 remains frozen at `cfc16d26` on `provenance/client-spike`.
Stage 16F.1 remains `3701dc51`. Stage 16E ontology remains `bc3e680c`.

## Causal chain

```text
external transfer
    → validate existing body + revision
    → debit/credit conserved source (bucket/fixture)
    → apply mass only to certified body domain
    → wake that body
    → 16F.1 equilibration
    → if spill eligible: 16F.2 one-edge transfers
    → sleep
```

The emerging contract is water-specific, not a generic scheduler:

**wake → bounded work → exchange → converge → sleep**

Budget changes latency, not the final conserved field (including any
downstream 16F.2 spill that the wake makes eligible).

## Transfer record

```text
FWaterExternalTransfer {
  TransactionId
  Cause              // bucket, source fixture, drain fixture
  BodyId
  ContactCell
  RequestedMass
  AdmittedMass
  BodyRevision
  OccupancyRevision
  Direction          // into body / out of body
}
```

## Conservation

```text
bucket/source loss == body gain
body loss == bucket/sink gain
global water + external container water == constant
budget 1 == N == unbounded final state
```

Occupancy mask must not change this cut:

- pour that would require a new wet cell → refuse
- scoop that would dry a currently required occupied cell → clamp/refuse

## Fixtures

- bucket → lake
- bucket → river
- bucket → wetland
- lake → bucket
- stale revision refuse
- closed/invalid body refuse
- over-capacity pour refuse/clamp
- overdraw scoop clamp/refuse

## Certified results

- Stage 16E freeze: body `2352b000a56f499c`, connectivity `ba4849f7148a7297`.
- Stage 16F.2 freeze (disabled / no-external path): field `401e88045eab4e43`.
- External field digest (budget 1 == N == unbounded): `a7f6eebc0fc0579f`.
- Mass+container conserved: world `456363002` + bucket `10000000` == world
  `456493002` + bucket `9870000` (`466363002` both sides). Pour `150000` in,
  scoop `20000` out; source loss == body gain on every receipt.
- Certified happy-path transfers 4/4 admitted. Downstream 16F.2 spill edges: 1.
  Fixtures: bucket→lake / bucket→river / bucket→wetland / lake→bucket.
- Controls: stale revision refuse; closed/invalid body refuse; over-capacity
  pour refuse/clamp; overdraw scoop clamp; pour onto dry cell refuse; scoop
  does not dry occupied cells. Occupancy mask / terrain / body IDs /
  connectivity unchanged.
- 192 m live radius: 2,601 packages; N/E/S/W eviction + exact return PASS.
- Movement frames over 16.667 ms: 0 (worst movement about 14.47 ms).
- Visual settle PASS; zero lower-frame sky.

## Player runtime

Run `PLAY_STAGE16F3_EXTERNAL_TRANSFER.cmd`, or:

```text
Build\x64_Release\ProvenanceClient.exe --play-stage16f3-external-transfer
```

Stage 16F.3 is the latest stable runtime (menu closed). Press `M` for the stage
browser. 16F.1 / 16F.2 may still be finishing across frames; external transfers
then run at 48 certified operations/tick.

## Re-run

Run `CERT_STAGE16F3_EXTERNAL_TRANSFER.cmd`. It executes analytical authority,
settled player-scale visual coverage, and full cardinal replacement gates.

## Boundaries (CLOSED)

- occupancy-mask mutation / new puddle creation / body split-merge
- wet-cell growth / dry-cell retirement
- channel overtopping into new terrain
- terrain excavation changing a basin
- rainfall as weather / arbitrary disturbances
- terrain-water feedback / erosion / sediment remobilization
- **P5b**
- **Stage 16F.4** topology-changing water
- generic SimulationDomain framework extract
