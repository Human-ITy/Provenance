# Stage 8 - Differential Erosion / Geology Shapes Relief

Status: certified compiled-history relief proof over the Stage-5 through Stage-7 authority ladder.

## Run

```text
CERT_CAUSAL_WORLD_DIFFERENTIAL_EROSION.cmd
```

Playable runtime:

```text
PLAY_WORLDGEN_STAGE0.cmd
```

Press `8` in game. The menu does not need to be open.

## Causal experiment

Both controls use the same seed, folded formations, `FeatureId` catalog,
chronology, bedding, regional erosion-work field, compiler, reconstruction, and
64 m certification region.

```text
Equal control:
    sandstone resistance = shale resistance

Differential control:
    sandstone resistance > shale resistance
```

Erosion work is spent downward through the actual geological column in bounded
0.125 m increments. Each increment queries the body currently being removed and
charges its material resistance. After the work is exhausted, the compiler
queries geology again at the final surface. It never copies the removal
material onto the output surface.

The authority fixture is
`Data/Worldgen/causal_world_differential_erosion_floor.cde`. It is hash-linked
to the existing geology descriptor and Stage-6 surface program.

## Required receipts

- Mean relief relative to the equal control for sandstone and shale.
- Mean positive retained sandstone relief.
- Mean shale recession.
- Sandstone-minus-shale contrast separation.
- Equalized-resistance contrast.
- Sandstone positive-relief area fraction.
- Shale recessed-area fraction.
- Longest coherent retained ridge or shoulder.
- Sandstone exposure fraction in both controls.

The hard causal gate is that differential relief must collapse below threshold
when resistance is equalized.

## Other gates

- Final material and `FeatureId` come from a fresh geology query at the moved surface.
- No new or relabelled geological features.
- Tiled and monolithic geometry are identical.
- Stage-7 watertight topology remains intact.
- Rendering and player grounding use the same half-metre triangle surface.
- Residency remains bounded to the 64 m neighborhood.
- The geology authority descriptor remains unchanged.

## Exclusions

There is no runtime erosion, water coupling, runoff, sediment transport,
intrusion, fault, mineralization, or P5b authorization. Stage 9 is the separate
granite-intrusion proof.
