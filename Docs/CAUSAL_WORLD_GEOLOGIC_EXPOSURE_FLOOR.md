# CAUSAL_WORLD geologic exposure floor

Status: read-only certification gate after the geology kernel floor.

## Run

```text
CERT_CAUSAL_WORLD_EXPOSURE.cmd
```

Direct executable switch:

```text
ProvenanceClient.exe --cert-causal-world-exposure
```

The certificate is written to:

```text
Docs/provenance_causal_world_geologic_exposure_cert.txt
```

## Ruling

An outcrop is the intersection of a canonical present-day surface with an
already-existing 3D geological body. Surface code does not select a material by
height, mint a formation, or replace geological ancestry.

The Python/Fablescript authority-side compiler emits two linked artifacts:

- the existing folded sandstone/shale geology descriptor;
- a present-day surface descriptor that hashes and names that exact geology
  descriptor.

Esoterica refuses missing, mismatched, unsupported, stale, or wrong-region
authority. It analytically evaluates the surface and asks the geology kernel
which persistent body occupies the intersection.

## Certified scope

- Deterministic present erosion boundary over the existing six formations.
- Exposed material, `FeatureId`, bedding frame, and nearest contact.
- Same-feature continuity from surface outcrop into a buried sample.
- Folded contact transitions across the surface.
- Independence from elevation/material thresholds.
- Forward, reverse, shuffled, tiled, and reload determinism.
- No renderer/materializer-minted geological features.
- Exact flat control and unsmoothed two-metre terraced control.
- Exposure query throughput, mean, and p99 timing.

The erosion boundary is compiled immutable geometry. It is not active erosion.

## Explicit exclusions

- No visible terrain or material palette output.
- No HF or D2 mutation.
- No exact occupancy or excavation.
- No intrusion, fault, or mineralization yet.
- No groundwater, rivers, runoff, sediment transport, or terrain-water coupling.
- P5b remains closed.

The certified result is available as in-game runtime stage `6`. It presents the
same exposure as bounded walkable terrain while D2, occupancy, bodies, and water
remain disabled. Granite intrusion, fault displacement, host-valid
mineralization, and surface-breach identity remain later separate proofs.

Stage `7` is the certified visible reconstruction of this same authority. See
`CAUSAL_WORLD_VISIBLE_GEOLOGIC_EXPOSURE.md`; Stage 6 remains available as the
diagnostic predecessor and is not silently replaced.
