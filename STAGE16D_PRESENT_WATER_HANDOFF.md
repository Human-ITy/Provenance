# Stage 16D — Present Water Occupancy Authority

Stage 16D is a derived present-water layer above frozen Stage 16C. It proves
static lakes, rivers, wetlands, and connected water surfaces as occupancy
authority. It does not run a flow tick, open P5b, mutate terrain, or invent
painted water noise.

## Causal chain

```text
Stage 15 → 16A → 16B → 16C finished landscape
    → deterministic present water bodies
    → surface elevation / depth field
    → occupancy consistent with terrain void
    → no flow simulation
```

## How bodies are derived

- **Lakes** — non-micro basins (closed / through-spill / structural /
  channel-connected) fill to the present spill lip on the Stage 16C surface.
- **Rivers** — channel-network cells receive depth from order + upstream area
  (capped), only where not already deeper lake occupancy.
- **Wetlands** — micro sinks, shallow residual basin ponding below lake minimum,
  and low-slope floodplain/basin-fill facies.
- **Connected bodies** — 4-neighbor components over occupied cells; mixed-kind
  components remain one body identity.

Occupancy requires positive depth above terrain (`waterSurfaceZ ≥ terrainZ`).
Capacity units seed from depth × cell area via the P5a scale contract; this is
not a live ledger wake and not P5b.

## Certified results

- Stage 16C digest frozen: `1367584c41aedcdf` (parent mass residual
  `-0.191406` g kept visible).
- Water digest: `636d01ba00d3dffe`; occupancy digest: `880a46fcdca2f8a4`.
- Occupied cells 13,417 — lakes 6,782 / rivers 2,218 / wetlands 4,417.
- Bodies 6,515 (1,121 mixed/connected); max / mean depth about 12.20 m / 1.33 m.
- Controls: water-off zero; lakes/rivers/wetlands-off remove their kinds;
  rivers follow channels; lakes require basins; parent terrain unchanged;
  monolithic == partitioned; cold-start digest stable.
- 192 m live radius: 2,601 packages; N/E/S/W eviction + exact return PASS.
- Movement frames over 16.667 ms: 0 (worst movement about 12.06 ms).
- Visual settle PASS; zero lower-frame sky.

## Player runtime

Run `PLAY_STAGE16D_PRESENT_WATER.cmd`, or:

```text
Build\x64_Release\ProvenanceClient.exe --play-stage16d-present-water
```

Stage 16D is the latest stable runtime (menu closed). Press `M` for the stage
browser. Blue/cyan quads are present-water surfaces only — not flow.

## Boundaries

Still closed: fluid solve, flow simulation, live erosion, ecology, and **P5b**.
Stage 16C sediment routing remains frozen at `a4a541c9`.

## Re-run

Run `CERT_STAGE16D_PRESENT_WATER.cmd`. It executes analytical authority,
settled player-scale visual coverage, and full cardinal replacement gates.
