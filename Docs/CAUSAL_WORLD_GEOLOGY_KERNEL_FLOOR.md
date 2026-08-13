# CAUSAL_WORLD geology kernel floor

This is the first read-only causal-worldgen gate after the Stage-0 traversal floor.
It proves that geology persists independently of terrain residency.

## Run

Build `Esoterica.Applications.ProvenanceClient` in Release x64, then run:

```text
CERT_CAUSAL_WORLD_GEOLOGY.cmd
```

The direct executable switch is:

```text
ProvenanceClient.exe --cert-causal-world-geology
```

The certificate is written to:

```text
Docs/provenance_causal_world_geology_kernel_cert.txt
```

## Authority boundary

`Tools/Worldgen/compile_causal_world_floor.py` is the authority-side reference
compiler for this fixture. It emits a compact, hash-verifiable descriptor under
`Data/Worldgen`. Esoterica loads, validates, indexes, and queries that artifact.
Missing, stale, mismatched, wrong-region, and unsupported-schema descriptors are
refused. C++ does not synthesize fallback CAUSAL_WORLD geology.

## Certified scope

- Six alternating sandstone/shale formation bodies.
- Deposition, lithification, and folding ancestry.
- Stable feature and event IDs unrelated to streamed cell addresses.
- Continuous local structural frames through a fold.
- Forward, reverse, shuffled, checkerboard, tiled, and reload semantic agreement.
- Query throughput, mean, p99, cold, and warm timing.
- Descriptor and query-index memory.

The kernel answers a 3D point directly. It generates no terrain and owns no
resident cell cache.

## Explicit exclusions

- No HF or D2 output.
- No occupancy mutation or excavation.
- No detached matter bodies.
- No palette or normal-map presentation.
- No water, groundwater, erosion, or sediment transport.
- P5b remains closed.

`RANGE`, `TORTURE`, and `BASELINE` remain separate existing fixtures. The green
kernel floor is available as in-game runtime stage `5`: a flat walkable datum
whose material, feature identity, and structural frame are direct readings of
the 3D geology kernel. It is an inspection slice, not the present land surface.
