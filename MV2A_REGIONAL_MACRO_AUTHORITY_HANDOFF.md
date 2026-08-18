# MV2.A — Regional Macro Authority

The narrow first cut of MV2, per the locked design. **World-authority lane only —
no renderer (MV2.B is closed).** One responsibility: produce **deterministic,
non-repeating, boundary-continuous regional macro authority beyond the frozen
central ±32 km region**, cheaply (no fine causal resampling).

Question answered — **yes**: the world authority can produce deterministic,
non-repeating neighboring macro pages beyond the frozen 64 km region while
preserving geographic continuity across page boundaries.

## What was built (Python authority)

```
world seed
  ↓  Tools/Worldgen/macro_authority.py
continuous absolute-coordinate macro control field
  macro_z(x,y) = central_envelope(x,y) + anchor_window(x,y) · regional_field(x,y)
  ↓
deterministic 64 km regional pages  (compile_page → .mcp)
  ↓
compiled macro authority  Data/Worldgen/MacroAuthority/page_<ri>_<rj>.mcp
```

- **`central_envelope`** — a faithful Python replica of
  `CausalMacroProvinces::SampleForcing().surfaceZ` (belt smoothstep envelope, basin
  Gaussian, grain sinusoid, datum), read from the frozen central `.cmp`. So the
  centre **is** the frozen macro authority.
- **`anchor_window`** = `smoothstep(32 km, 64 km, max(|x|,|y|))` — exactly **0 with
  zero slope** on the ±32 km centre boundary and 1 beyond 64 km. This gives
  **position + slope continuity to the frozen centre by construction** (smoothstep's
  endpoint derivative is 0), and confines the regional field to outside the centre.
- **`regional_field`** — a continuous absolute-coordinate field: a smooth
  incommensurate tectonic base + **6 long mountain belts (400–900 km)** + 6 broad
  basins (120–300 km) + jittered massifs (50–95 km), all deterministic from the
  world seed via FNV-1a hashing. **Feature wavelengths are independent of the 64 km
  cell** (belts span many pages); continuity and non-repetition are by construction.

**64 km is packaging only** (serialization/ownership/cache cell), never terrain
cadence — a single belt runs 874 km across ~13 cells.

## Page contract (each `.mcp`)

`region_id`, `region_cell (ri,rj)`, `world_seed`, `generator_version`,
`world_identity_hash`, absolute bounds, `sample_step_m`, `grid_n`, `source_digest`,
8-way `neighbor_*` lineage, `no_wrap_into_region=1`,
`cheap_source=macro_analytic_no_reconstructedz`, and the coarse row-major height
grid (1 km macro step; 65×65 per 64 km page).

## Certification — all fixtures PASS

`Docs/provenance_mv2a_macro_authority_cert.txt` (run
`python Tools/Worldgen/cert_macro_authority.py`), 9-page set
(center + N/S/E/W + NE/NW/SE/SW):

| Fixture | Result |
|---|---|
| 1 determinism (same seed+cell → same digest) | PASS — 9/9 reproduce; **0 differing bytes cross-process** |
| 2 non-repetition (no 64 km tiling) | PASS — 9/9 unique ids+digests; `macro_z(x)` vs `macro_z(x+64 km)` RMS **675 m** |
| 3 boundary continuity (4 centre edges) | PASS — max position step **0.046 m**, slope step **0.0001** |
| 4 cross-page landform continuity | PASS — a belt crest crosses the −96 km page boundary; jump **0.71 m**, crest **1885 m** above base |
| 5 central anchoring vs frozen macro | PASS — center page − frozen envelope = **0.00000 m** (window=0 inside) |
| 6 no square-world signature | PASS — max step **on** page boundary 5.2 m ≤ max **interior** step 225 m |
| 7 feature scale independence (>64 km) | PASS — longest belt extent **874 km** |
| 8 no wrap / no fine authority | PASS — pages declare no-wrap; code has no `WrapIntoRegion`/`ReconstructedZ`/`QueryMaterial` identifiers; stdlib-only |
| cheap source | PASS — 9 pages compiled in **0.48 s**, macro-analytic only |

Visual evidence: `Docs/provenance_mv2a_macro_footprint.png` — a 3×3 footprint
heightmap. A large NE–SW belt crosses several 64 km page lines seamlessly, the
frozen centre (red square) holds its authored belt continuously with its
surroundings, and no terrain resets on the grid lines.

## What this proves

```
the world can now exist honestly beyond ±32 km
  · without repetition   (non-repetition + no-square-signature)
  · without seams        (boundary continuity + central anchoring + cross-page)
  · without fine cost     (cheap macro-analytic; 0.48 s / 9 pages; no ReconstructedZ)
```

## Runtime

```
python Tools/Worldgen/cert_macro_authority.py
  → emits Data/Worldgen/MacroAuthority/page_<ri>_<rj>.mcp (9 pages)
  → writes Docs/provenance_mv2a_macro_authority_cert.txt (PASS/FAIL, exit 0 iff all pass)
```

## Explicitly NOT done (still closed)

MV2.B horizon renderer, MV2.C presentation, MW9, new ecology/erosion, detailed
geology remeshing. MW1–MW8 / MV1 / PX1–PX3 not reopened. The frozen central region
is untouched (anchored, not regenerated).

## Next

**MV2.B — Horizon renderer** can now consume these compiled macro pages to render
the 100–128 km horizon from real neighboring world authority: bounded resident
ring, two-pass far projection widened to ~128 km, MV1.D aerial recalibrated for
128 km, seam-free join to the frozen 32 km MV1 edge, gameplay PX2 hard gate green.
The macro-page height source is cheap and cacheable, directly answering PX3's
tile-construction scaling wall.

## Board

```
MV2.A regional macro authority   CERTIFIED (9 pages, 8 fixtures green, deterministic, cheap)
MV2.B horizon renderer           NEXT (consume macro pages; no new world authority)
MV2 / MV2.C / MW9                 downstream
```
