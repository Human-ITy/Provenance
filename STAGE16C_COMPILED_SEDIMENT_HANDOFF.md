# Stage 16C — Compiled Sediment Routing

Stage 16C is a derived compiled-history layer above frozen Stage 16B. It does
not simulate or render water, open P5b, run ecology, or apply live erosion.

## Causal chain

```text
Stage 16B compiled fluvial removal receipt
    → 16C.1 source ledger (cell, watershed, material, formation/feature, grams, grain)
    → 16C.2 deterministic transport on the frozen Stage 16A receiver graph
    → 16C.3 deposition (valley / basin / fan / bar / floodplain) + outlet export
```

Mass invariant:

```text
erosion source mass
  = deposited mass
  + downstream mobile load
  + explicit exported-outlet mass
```

## Certified results

- Stage 16B digest unchanged: `77a9fe368175c5e6`.
- Sediment digest: `1367584c41aedcdf`; geometry digest: `7ae7aa62c50489e2`.
- Source / deposited / mobile / exported:
  about `2.452e13` / `2.317e13` / `9.860e11` / `3.578e11` grams.
- Mass residual about `-0.19` g (float noise under relative tolerance).
- Source cells 55,374; deposit cells 56,003; export cells 695.
- Max / mean deposit depth about `3.77` m / `0.52` m (bounded by 4 m).
- Controls:
  - transport off → exact Stage 16B surface
  - erosion off → zero sediment source
  - deposition off → essentially all mass reaches explicit outlet export
  - shale mobility exceeds sandstone under equal drainage forcing
  - monolithic == partitioned; cold-start digest stable
- 192 m live radius: 2,601 packages; N/E/S/W eviction + exact return PASS.
- Movement frames over 16.667 ms: 0 (worst movement about 12.95 ms).
- Stage 14 single-pick PASS; L1 PASS; L6 PASS.

## Player runtime

Run `PLAY_STAGE16C_SEDIMENT.cmd`, or:

```text
Build\x64_Release\ProvenanceClient.exe --play-stage16c-sediment
```

Stage 16C is the latest stable runtime (menu closed). Press `M` for the stage
browser. Amber deposit arrows are presentation-only diagnostics.

## Boundaries

Still closed: present water occupancy/rendering, fluid solve, live erosion,
ecology, and P5b. Do not open Stage 16D under this certificate.

## Re-run

Run `CERT_STAGE16C_SEDIMENT.cmd`. It executes analytical authority, settled
player-scale visual coverage, and full cardinal replacement gates.
