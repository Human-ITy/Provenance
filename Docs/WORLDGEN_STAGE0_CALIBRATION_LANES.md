# Stage-0 Scale and Material Calibration Lanes

Status: playtest-only presentation fixture. Excluded from
`--cert-worldgen-baseline-perf`.

## Distance ruler

- Direction: +Y from `(128.5, 128.5)`
- Extent: 0–2,000 m
- 1 m ticks: first 50 m, excluding 10 m bands
- 10 m bands: approximately 0.91 m deep × 1.83 m wide
- 100 m bands: wider, paired emphasis
- 1,000 m bands: landmark width with flanking marks
- Batch: one display list
- Current geometry: 536 triangles / 1,072 submitted quad vertices

The ruler is neon-yellow presentation just above the analytic dirt floor. It has
no collision, occupancy, D2, support query, body, interaction, or simulation
authority.

## Terrain palette

Five deterministic 4×4 m walk-over swatches sit east of the start area:

| Swatch | Current representation | Triangles | Meso details | Bodies |
|---|---|---:|---:|---:|
| Dirt | Base HF plus color/normal breakup | 128 | 0 | 0 |
| Disturbed dirt | Compacted color plus shallow rutted normals | 128 | 0 | 0 |
| Gravel | Aggregate breakup plus sparse meso protrusions | 192 | 16 | 0 |
| Clay | Smooth cohesive variation | 128 | 0 | 0 |
| Sand | Deterministic ripple variation | 128 | 0 | 0 |

Total palette geometry is 704 triangles / 2,112 submitted triangle vertices in
one display list. The surface remains the same flat walk/collision authority;
the small visual offsets do not create occupancy or gameplay geometry.

Fresh cut faces, detached fragments, wet variants, vegetation cover, and richer
shader roughness/normal evaluation remain future palette-board expansions. They
are not implied by this first presentation fixture.

## Measured combined-view cost

A visible 1280×800 combined-stage smoke produced:

```text
combined frames measured: 8,023
ruler triangles:             536
palette triangles:           704
mean calibration CPU submit: 0.00017 ms
resident maximum:         13,583 cells
full HF rebuilds:              0
D2/occupancy/edit/support:     0
bodies/fractures:              0
```

This timing measures CPU list selection/submission. GPU timer queries are not
available in the current legacy OpenGL backend, so no independent GPU cost is
claimed.
