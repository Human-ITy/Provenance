# Stage 11 - Canonical Fault Displacement

Stable certificate ID: `GEO.FAULT`

Status: certified read-only causal worldgen authority.

## Scope

Stage 11 adds one younger fault event to the certified Stage-10 history.
The event displaces existing sandstone, shale, granite, and contact quartz by
inverse coordinate mapping. It does not replace those bodies or mint client-side
geology.

```text
folded sandstone/shale
  -> granite intrusion
  -> contact quartz mineralization
  -> younger fault displacement
  -> unchanged present erosion/exposure boundary
```

Inside the declared fault domain, the hanging-wall query maps back through the
fault throw before asking Stage 10 for geology. Outside that domain, and when the
event is disabled, Stage 10 remains authoritative. Absence of a fault result is
never interpreted as air or empty terrain.

## Certificate gates

- Disabling the fault event collapses exactly to the Stage-10 answer.
- Displaced sandstone, shale, granite, and quartz preserve their `FeatureId`,
  body ancestry, deposit system, deposit body, and structural orientation.
- Only features older than the fault chronology are displaced; a younger control
  is unchanged.
- Every displaced sample inversely maps to the expected Stage-10 continuation.
- Monolithic, tiled, shuffled, cold-start, and reload queries have identical
  coordinate-and-answer digests.
- Outside-domain queries fall back to Stage 10 rather than empty matter.
- The visible identity-mode x-ray terminal section resolves the same quartz
  `FeatureId` on both fault blocks.
- Mutation, water, active erosion, sediment transport, bodies, and P5b remain
  closed.

The certified fixture scans 60,025 points. It displaces 26,724 samples,
including 229 quartz samples, while preserving all sampled identities.

## Live traversal performance

Stage 11 is part of the permanent uncapped player-loop ladder. Terrain display
lists that leave residency are retired through a short, bounded deferred queue
instead of being destroyed in the same frame that submitted them for drawing.
This changes presentation-resource lifetime only; authority, geometry,
material/FeatureId, collision, and forward/reverse digests remain invariant.

The 11 August 2026 live receipt records the following for a continuous 48 m
walk at 5 m/s:

```text
mean frame       0.679208 ms  (1472.30 FPS)
p99              1.056922 ms
worst           11.284900 ms
frames >16.667          0
new cells             6192
packages built/evicted 114 / 114
```

The separate hostile workload still performs six instantaneous 96 m crossings
in three seconds and reaches 425.840700 ms. That is an explicit bulk-admission
backlog, not an ordinary-walking result, and remains open for later scheduling
work without weakening geology or reconstruction fidelity.

## Run

```text
CERT_CAUSAL_WORLD_FAULT_DISPLACEMENT.cmd
CERT_STAGE11_FAULT_VISUAL.cmd
PLAY_STAGE11_FAULT.cmd
```

The playable counterpart is also available from `PLAY_WORLDGEN_STAGE0.cmd` as
`GEO.FAULT` (shortcut `9`). It starts with the geology x-ray disabled; enable the
flashlight in play when inspection is wanted.
