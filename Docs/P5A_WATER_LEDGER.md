# P5a — Water ledger / settle floor

**Law:** Water should wake from causality, not from time passing.

```
terrain occupancy → container/obstruction
water ledger      → conserved capacity_units
surface/body      → presentation + hydraulic level solve
terrain mutation  → wake only affected water neighborhood
```

Acceptance shape:

```
water added → conserved ledger → local occupancy/container query
  → hydraulic settle → settled body → zero recurring work
```

## Authority (honest)

P5a lands an **Esoterica-local** `WaterLedger` (`WaterLedger.h`) so the client spike can certify
settle/dormant/causality before a Fablescript water-authority wire is consumed here.

Fablescript already has a conserved cellular ledger (`engine/water.py`) and bridge projection.
When that wire is consumed in this lane, P5a counters/receipts should prefer engine receipts;
until then this module is the cert floor, not a second long-term authority.

**Out of scope / CLOSED:** P5b dig/place coupling depth · P5c marched/volumetric presentation ·
P5d flow stress · continuum MPM / Atomic-Fluid · cubic lattice-leak as truth (`WATER_GOALS` §8 class).

## Units conversion contract

Ledger authority is **`capacity_units`**, not grams.

| Symbol | Value | Meaning |
|---|---|---|
| `kCellCapacityUnits` | 100 | Full open cell column (Fablescript `CELL_CAPACITY_UNITS` scale) |
| `kCellDepthM` | 1.0 m | Hydraulic/visual depth at full capacity |
| `kCellFootprintM2` | 1.0 m² | Nominal column footprint |
| `kWaterDensityKgM3` | 1000 | Pure-water density for derived mass only |

```
volume_m3 = capacity_units * (kCellFootprintM2 * kCellDepthM) / kCellCapacityUnits
mass_kg   = volume_m3 * kWaterDensityKgM3
mass_g    = mass_kg * 1000
```

Helpers: `VolumeM3FromUnits`, `MassKgFromUnits`, `UnitsFromVolumeM3`.

**Rules:**
- Do not call 0–100 cell fill “grams.”
- Real mass/volume is derived only through this contract before any cross-system coupling.
- Dirt place mapping (`Main.cpp`) converts **dirt grams → solid fill capacity_units** for displacement;
  that is occupancy fill scale, not water mass.

## Settle law

`SettleBodyOnce` runs a **deterministic hydraulic head/level solve**:
expand through open-container adjacency, then place units one-at-a-time into the open cell
whose resulting surface is lowest (tie-break x,y). Equal-floor connected cells share a settled
plane from conserved amount (not greedy fill-to-capacity).

Overflow that cannot sit in open capacity → `spillOutOfScopeUnits` (conserved). Never write
water into solid / zero-capacity / over-capacity cells.

## Behaviors certified

| Check | Expect |
|---|---|
| `pour_exact_mass` | pour adds exact capacity_units |
| `no_solid_occupy` | water rejected on solid container |
| `level_equal_floor_share` | equal-floor basin shares leveled split (not 100/0/0) |
| `settled_dormant` | after local settle, body dormant |
| `dormant_zero_unrelated` | unrelated receipts → water work = 0 |
| `height_neq_grams` | **derived** equal surface from unequal units (different geometries) |
| `amount_eq_height_neq` | equal units → unequal level when floors differ |
| `dig_wakes_local_body` | dig wakes only connected local body |
| `place_displaces_conserved` | place displaces; in-world + spill conserved; source not left wet-solid |
| `no_water_in_solid_mutation` | no water in solid occupancy under mutation |
| `presentation_rev_consistent` | surface stamp refuses mismatched terrain/water rev |
| `final_conserved_dormant` | conserved total held; idle settle is free |
| `water_receipt_wakes_scoped` | water-channel receipt wakes scoped body |
| `units_volume_mass_contract` | capacity_units ↔ volume ↔ mass round-trip |
| `settle_order_determinism` | repeat pour order → identical content digest |

## Cert

```bat
Build\x64_Release_p5a\ProvenanceClient.exe --cert-water
```

Aliases: `--cert-p5a`, `--cert-water-ledger`.  
Artifact: `%TEMP%\provenance_p5a_water_ledger_cert.txt`  
Committed copy: `Docs\provenance_p5a_water_ledger_cert.txt`.

Headless ledger cert (no bridge required). Terrain affect=0 path remains the residency pattern for live receipts.

**P5b remains CLOSED** even when this floor is green.
