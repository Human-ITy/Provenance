# Stage 16F.2 — Graph-Authorized Inter-Body Spill / Transfer

Stage 16F.2 answers one question: when a certified 16E body has water above a
certified outlet relationship to another body, can conserved water cross **that
one existing graph edge** deterministically?

It does not rain, pour, disturb arbitrarily, couple terrain, erode, remobilize
sediment, open P5b or 16F.3, discover new connectivity, extract a generic
SimulationDomain framework, or propagate a whole river downstream.

## Freeze

Stage 16F.1 body-local control remains frozen at `3701dc51` on
`provenance/client-spike`. Stage 16E ontology remains `bc3e680c`.

## Causal chain

```text
16E body graph
    → source body exceeds spill condition
    → certified outlet edge
    → compute admissible transfer
    → debit source / credit destination
    → run 16F.1 locally on the two affected bodies
    → sleep again
```

Connectivity is already published by 16E. The solver never searches for a new
destination. Transfer is one certified body-edge at a time; newly downhill
edges created by a hop are not discovered in this stage.

## Transfer records

```text
FWaterTransferEdge {
  EdgeId
  SourceBodyId
  DestinationBodyId
  OutletCell
  SpillElevation
  SourceRevision
  DestinationRevision
  TransferClass
}

FWaterTransferReceipt {
  TransferId
  EdgeId
  RequestedMass
  AdmittedMass
  SourceMassBefore
  SourceMassAfter
  DestinationMassBefore
  DestinationMassAfter
  InputRevisions[]
  OutputRevisions[]
}
```

Transfer classes: lake→river, river→wetland, wetland→river, mixed→downstream,
plus closed-lake / disabled controls.

## Certified results

- Stage 16E freeze: body `2352b000a56f499c`, connectivity `ba4849f7148a7297`.
- Stage 16F.1 freeze (disabled / no-spill path): field `af76b0826a8c8d22`.
- Transfer field digest (budget 1 == N == unbounded): `401e88045eab4e43`.
- Mass conserved: `456363002` units before == after; source loss == dest gain
  on every receipt (`admitted_total=38482031`).
- Certified edges 5,730; snapshot-eligible 2,529; transferred 2,479.
  Fixtures: lake→river 9 / river→wetland 81 / wetland→river 78 /
  mixed→downstream 855.
- Controls: disabled ≡ exact 16F.1; closed lake no transfer; below-spill lake
  no transfer; stale revision refuse; blocked/disabled edge refuse;
  occupancy mask / terrain / body IDs / connectivity unchanged.
- 192 m live radius: 2,601 packages; N/E/S/W eviction + exact return PASS.
- Movement frames over 16.667 ms: 0 (worst movement about 13.94 ms).
- Visual settle PASS; zero lower-frame sky.

## Player runtime

## Contract

- source loss == destination gain
- global water mass unchanged
- transfer only across a certified 16E edge
- no new body ID, no body deletion, no topology rewrite, no terrain mutation
- stale edge/body revision → reject and reevaluate
- budget 1 == budget N == unbounded final field digest
- no eligible spill / transfer disabled → exact 16F.1 state

Lesson (same as 16F.1): **budget changes convergence time, NOT final equilibrium.**

## Player runtime

Run `PLAY_STAGE16F2_WATER_TRANSFER.cmd`, or:

```text
Build\x64_Release\ProvenanceClient.exe --play-stage16f2-water-transfer
```

Stage 16F.2 is the latest stable runtime (menu closed). Press `M` for the stage
browser. 16F.1 may still be finishing across frames; transfers then run at 48
certified edges/tick.

## Re-run

Run `CERT_STAGE16F2_WATER_TRANSFER.cmd`. It executes analytical authority,
settled player-scale visual coverage, and full cardinal replacement gates.

## Boundaries (CLOSED)

- rainfall / bucket pours / arbitrary disturbances
- terrain-water feedback / erosion / sediment remobilization
- **P5b**
- **Stage 16F.3**
- dynamic connectivity discovery
- generic SimulationDomain framework extract
- whole-river downstream propagation
