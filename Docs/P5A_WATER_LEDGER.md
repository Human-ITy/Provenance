# P5a — Water ledger / settle floor

**Law:** Water should wake from causality, not from time passing.

```
terrain occupancy → container/obstruction
water ledger      → conserved water amount (grams)
surface/body      → presentation + local flow (minimal)
terrain mutation  → wake only affected water neighborhood
```

Acceptance shape:

```
water added → conserved ledger → local occupancy/container query
  → local flow → settled body → zero recurring work
```

## Authority (honest)

P5a lands an **Esoterica-local** `WaterLedger` (`WaterLedger.h`) so the client spike can certify
settle/dormant/causality before a Fablescript water-authority wire is consumed here.

Fablescript already has a conserved cellular ledger (`engine/water.py`) and bridge projection.
When that wire is consumed in this lane, P5a counters/receipts should prefer engine receipts;
until then this module is the cert floor, not a second long-term authority.

**Out of scope:** P5b dig/place coupling depth · P5c marched/volumetric presentation · P5d flow stress ·
continuum MPM / Atomic-Fluid · cubic lattice-leak as truth (`WATER_GOALS` §8 class).

## Behaviors certified

| Check | Expect |
|---|---|
| `pour_exact_mass` | pour adds exact grams |
| `no_solid_occupy` | water rejected on solid container |
| `settled_dormant` | after local settle, body dormant |
| `dormant_zero_unrelated` | NPC/combat/world-tick style calls → water work = 0 |
| `height_neq_grams` | equal surface height ≠ equal grams |
| `dig_wakes_local_body` | dig wakes only connected local body (never global) |
| `place_displaces_conserved` | place into water displaces/reconfigures; grams conserved |
| `presentation_rev_consistent` | surface stamp refuses mismatched terrain/water rev |
| `final_conserved_dormant` | mass held; idle settle is free |
| `water_receipt_wakes_scoped` | water-channel receipt wakes scoped body |

## Cert

```bat
Build\x64_Release\ProvenanceClient.exe --cert-water
```

Aliases: `--cert-p5a`, `--cert-water-ledger`.  
Artifact: `%TEMP%\provenance_p5a_water_ledger_cert.txt`.

Headless ledger cert (no bridge required). Terrain affect=0 path remains the residency pattern for live receipts.
