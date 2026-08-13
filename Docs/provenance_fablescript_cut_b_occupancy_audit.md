# Provenance — Cut B FableScript #18 Occupancy Audit

Status: implementation boundary confirmed; FableScript write authority required

Date: 11 August 2026

## Outcome

Cut B is necessary. The current FableScript terrain path still has separate
height and matter representations:

1. `worldgen/provenance_tectonic.py::grade_at` produces a floating render grade.
2. `engine/terrain_read.py::fresh_column` creates a completely solid 8 × 8 × 32
   occupancy column and attaches that grade as metadata.
3. `engine/terrain_mutate.py::sculpt_column_rich` repeats the same split for a
   materialized/editable column and explicitly describes grade as render-only.
4. `engine/water_projection.py::relief_water_floor` converts grade into a second
   terrain-height term for water.
5. `engine/matter_builtins.py::surface_level` derives gameplay height from the
   ledger or a column-equivalent amount rather than from the same occupancy
   boundary used to present relief.

This is not continuous authoritative occupancy. The visible surface, editable
matter body, movement floor, and water floor can agree only through parallel
conversion rules.

## Confirmed 12.5 cm basis

The canonical scale is internally consistent:

```text
CELL_METERS / COLUMN_W                 = 1.0 / 8 = 0.125 m
CELL_DEPTH_METERS / COLUMN_SUBLAYERS  = 0.5 / 4 = 0.125 m
```

The problem is not voxel scale. It is the detached vertical grade transform.

## Cut B implementation boundary

The smallest honest Cut B changes FableScript itself. It must introduce one
canonical fixed-point surface/occupancy evaluator consumed by all of these
paths:

```text
virgin terrain read
editable-column materialization
surface compatibility projection
movement/support surface
water floor
save/load reconstruction
```

The canonical result needs:

```text
global fixed-point surface height
global voxel origin for each materialized column
8 × 8 boundary-sharing subcolumn heights
one region-level remainder allocation for partial top fill
material at every occupied sample from the geological evaluator
```

`grade` may remain temporarily on the wire only as a value derived from this
occupancy result. It cannot remain an input to collision, water, movement, or
per-column placement.

## Required source cuts

### 1. Continuous fixed-point relief

- Evaluate the continuous terrain field at canonical subcell coordinates.
- Preserve bit-identical integer-coordinate samples.
- Quantize once into a declared fixed-point global-height unit.
- Ensure a boundary sample has one coordinate key, regardless of owning tile or
  request partition.

### 2. Region materialization

- Materialize a region, not 64 independently rounded samples per cell.
- Allocate the quantized remainder deterministically across the region so the
  occupancy sum equals the authoritative target within the declared exact
  bound.
- Emit global `origin_z` plus fill/material arrays. Adjacent columns must share
  the same boundary heights by construction.

### 3. Geological fill

- Query the existing canonical geology at each occupied sample.
- Preserve layer order, contacts, lenses, mineral inclusions, and stable
  geological identity.
- Do not infer material from the reconstructed surface.

### 4. One surface truth

- Derive render surface, movement/support height, and water floor from the
  occupied boundary.
- Retire grade-to-water and ledger-to-surface as independent relief terms.
- Keep compatibility grade output derived from occupancy until consumers are
  migrated.

### 5. Persistence and editing

- Persist global origin, fill, material, and any deterministic allocation
  metadata required to reproduce the exact boundary.
- Digging mutates that occupancy directly.
- Save/load must reproduce identical occupancy bytes and derived surface.

## Certification matrix

Cut B cannot close without all of the following:

| Gate | Receipt |
|---|---|
| Integer parity | Existing integer-coordinate worldgen corpus is bit-identical |
| Shared edge | Opposite owners emit identical boundary samples |
| Partition invariance | Monolithic and adversarial tiled requests hash identically |
| Mass | Region occupancy equals the fixed-point matter target within the declared bound; no per-sample drift |
| Surface | Render, movement/support, and water read the same occupied boundary |
| Strata | Material/contact ordering survives partial top bands |
| No-cell-band | No 1 m terrace signature without an authored discontinuity |
| Edit | First materialization does not move the surface; dig changes only accepted occupancy |
| Save/load | Occupancy, materials, origins, and derived surface round-trip byte-identically |
| Authority | Compatibility `grade` is derived only |
| Regression | Cut A bridge digest and existing Stage 5–10 digests remain unchanged |

## Workspace ruling

The active ProvenanceEsoterica lane declares the external Mygame/FableScript
tree read-only. No FableScript files were changed during this audit. Cut B
cannot be truthfully implemented in the Esoterica projection layer because that
would create the second terrain authority the cut is intended to remove.
