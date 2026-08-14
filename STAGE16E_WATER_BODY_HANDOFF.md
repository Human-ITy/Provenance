# Stage 16E — Present Water Body Semantics / Connectivity

Stage 16E is a derived ontology layer above frozen Stage 16D occupancy. It
certifies static water-body identity and connectivity before any flow moves.
It does not run a flow tick, open P5b, mutate terrain, or change 16D occupancy.

## Causal chain

```text
Stage 16D present-water occupancy (frozen)
    → FPresentWaterBody records
    → body type / surface rule / spill elevation
    → inlet/outlet portals on Stage 16A receivers
    → upstream/downstream adjacency
    → no flow simulation
```

## Contract

```text
FPresentWaterBody {
  BodyId
  BodyType          // lake / river / wetland / mixed
  SurfaceRule       // shared_spill / local_depth / mixed
  SpillElevation
  Cells[]
  Inlets[]
  Outlets[]
  UpstreamBodies[]
  DownstreamBodies[]
  SourceLandscapeRevision
  SourceHydrologyRevision
}
```

## Certified results

- Stage 16D freeze: water `636d01ba00d3dffe`, occupancy `880a46fcdca2f8a4`
  (parent Stage 16C `1367584c41aedcdf`; mass residual `-0.191406` g kept visible).
- Bodies 6,515 — lake 1,853 / river 1,045 / wetland 2,496 / mixed 1,121.
- Outlets 6,782; inlets 5,745; closed lakes 600; wetland↔river↔lake links 4,238.
- Body digest `2352b000a56f499c`; connectivity digest `ba4849f7148a7297`.
- Controls: same 16D occupancy; body IDs stable across cold start; connectivity
  stable across partitioning; no merge/split from load order; outlets
  topologically valid on 16A receivers; closed lakes have no unexplained
  below-spill drains; river paths follow 16A channel authority.
- 192 m live radius: 2,601 packages; N/E/S/W eviction + exact return PASS.
- Movement frames over 16.667 ms: 0 (worst movement about 15.49 ms).
- Visual settle PASS; zero lower-frame sky.

## Player runtime

Run `PLAY_STAGE16E_WATER_BODY.cmd`, or:

```text
Build\x64_Release\ProvenanceClient.exe --play-stage16e-water-body
```

Stage 16E is the latest stable runtime (menu closed). Press `M` for the stage
browser. Blue/cyan quads are static present-water surfaces; HUD publishes body
type, spill, and inlet/outlet counts — not flow.

## Boundaries

Still closed: fluid solve, **flow simulation**, live erosion, ecology, **P5b**,
dynamic flow, and **Stage 16F**. Stage 16D occupancy remains frozen at
`061bec0b`.

## Re-run

Run `CERT_STAGE16E_WATER_BODY.cmd`. It executes analytical authority, settled
player-scale visual coverage, and full cardinal replacement gates.
