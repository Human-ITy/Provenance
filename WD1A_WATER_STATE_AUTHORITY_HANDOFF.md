# WD1.A — WaterState Authority

First WD1 implementation cut (design `c2f9bd55`, `WD1_WATER_DIVERSITY_DESIGN.md`).
**Python world-authority only** (`Tools/Worldgen/macro_authority.py`); the renderer is
unchanged and consumes the re-emitted `.mcp` pages. Authority-only: **no appearance (WD1.B),
no water mass, no geometry change.**

## The one question it answers

> Does the endless seeded world **know what water is present** at a location — whether water
> is there at all, what kind of body/regime, and its physical/optical state — from the
> existing certified authorities, **without duplicating or contradicting** frozen 16D–16F /
> P5b water truth?

Yes. `CHANNEL_EXISTS != WATER_PRESENT != WATER_BODY_TYPE != WATER_OPTICAL_STATE`.

## What was built

- **`WaterState`** (compact semantic record; **no RGB, no shader coefficients**):
  presence_regime · body_class · flow_regime · **depth_m + bottom/surface elevation** ·
  **discharge_proxy (water_supply_index)** · mean_supply · seasonality_index ·
  persistence_margin · clarity · turbidity · suspended_sediment · mineral_load · organic_load ·
  temperature_proxy · **bottom_family (MS1 reference)** · waterfall_potential + mineral_potential
  (reserved hooks) · MacroWatershedId · MacroChannelId · MacroWaterBodyId · WaterRegimeId. Packs
  to ~58 bits.
- **Vocabulary:** presence `dry · damp_substrate · ephemeral · seasonal · perennial · standing`;
  bodies `headwater_stream · perennial_river · sediment_river · braided_reach · alpine_lake ·
  closed_basin_lake · floodplain_water · wetland_marsh · organic_darkwater · arid_wash ·
  spring_pool · volcanic_mineral_pool · crater_lake`; flow `still · slow · channelized · fast ·
  turbulent`. Families are consequences (regions of the state space), not presets.
- **Two-stage presence law** (the "drainage ≠ water" law):
  - **`water_supply()`** — Stage 1: `discharge = climate_supply·(0.3+0.8·catchment) −
    permeability_loss`, where `climate_supply = humidity + orographic − evaporation` and
    `catchment = smoothstep(accum)`; plus `seasonality_index` (aridity + continentality) and
    `persistence_margin`. **Guardrail: consumes environmental wetness POTENTIAL only
    (climate/substrate/catchment) — never WaterState itself (no circular input).**
  - **`water_presence_body()`** — Stage 2: accommodation (channel gradient / basin closure /
    relief) decides flowing vs standing vs wet-ground vs dry, and the body/regime. A high-supply
    place becomes a **river** on a slope but a **lake** in a closed basin; an arid permeable place
    leaves the same channel **dry**.
  - **`_water_optics()`** — clarity/turbidity/sediment/mineral/organic/temperature from cause
    (accumulation × substrate erodibility; volcanic ancestry; organic potential; elevation).
- **`water_state_at()`** — macro entry point: composes MV3.B1 drainage (`drainage_query`) +
  MS1 substrate (`surface_state_at` → permeability/erodibility/organic/bottom family) + MV3
  controls + macro hydroclimate proxies (`macro_humidity`/`macro_contin`) + volcanic ancestry
  (`landform_at`), plus a cheap 4-probe basin concavity for accommodation/depth. **Anchor-window
  gated: dry in the frozen ±32 km centre** (16D–16F owns detailed water there). Identity:
  `MacroWaterBodyId` keyed to the canonical watershed sink (absolute coords); rivers inherit
  `MacroChannelId`; a derived `WaterRegimeId` refines a reach.
- **Macro page descriptor:** each page gains a **17×17 @ 4 km** WaterState grid
  (`water_grid_row_major`, hex-packed) + `water_digest` + version — computed in the SAME cell
  loop as the MS1 surface descriptor via one shared per-cell env (`_macro_place`), so the doubled
  descriptor stays within the cheap-source budget. Renderer ignores the new keys (verified).

## Mass law + guardrails (HARD)

- **Macro WaterState is an environmental PROMISE, not a conserved water ledger** — WD1.A creates
  **no water mass** and changes **no terrain geometry**; 16D–16F/P5b are untouched.
- **No circular input** — presence uses climate/substrate/catchment potential, never actual water.
- **Compatibility only where detailed water legitimately exists** — the frozen ±32 km centre is
  dry (deferred); WD1 does not fabricate macro water inside detailed coverage.

## Certification — all 15 WD1.A fixtures PASS (`Docs/provenance_mv2a_macro_authority_cert.txt`)

Preserved every prior fixture (MV2.A/A2 + MV3.A/B1/B1.1/B2/C + MS1.A; 0 FAIL total), **plus**:

| Fixture | Result |
|---|---|
| drainage ≠ water | PASS — same channel: dry supply → dry/none; wet supply → perennial/perennial_river |
| perennial vs seasonal | PASS — humid low-seasonality → perennial; dry high-seasonality permeable → ephemeral/arid_wash |
| lake / basin accommodation | PASS — closed basin + supply → standing/closed_basin_lake; supply removed → dry (needs BOTH) |
| bottom substrate reference | PASS — bodies carry ≥2 MS1 bottom families {rock 123, regolith 549, volcanic 332, sediment 99} |
| sediment / turbidity optics | PASS — clear-headwater clarity 0.87 vs sediment/floodplain 0.32 |
| organic wetland | PASS — 558 wetland/dark-water bodies, max organic_load 0.80 |
| volcanic / mineral | PASS — 28 mineral bodies + 244 ordinary bodies in mineral areas (exists, not universal) |
| depth continuous + deterministic | PASS — 85 distinct depths spanning 1.5–120.0 m (not banded); packs reproduce |
| cross-page identity | PASS — channel crosses a 64 km page line keeping MacroChannelId |
| cross-super-tile identity | PASS — MacroChannel + Watershed identity preserved across the 384 km edge |
| seed semantics | PASS — same-seed water_digest reproduces; alt-seed differs |
| long-distance unbounded | PASS — deterministic, non-periodic over 0..2000 km |
| macro/fine compatibility guardrail | PASS — centre macro water bodies = 0 (16D–16F owns detailed); ±32 km deferred/dry |
| frozen geometry / no mass | PASS — height digest == source_digest (no terrain sample changed); classification only |
| cheap source | PASS — drainage + MS1 + macro hydroclimate proxy only; no ReconstructedZ/QueryMaterial |

`compile_seconds ≈ 196` (budget 280 — the ring now carries TWO semantic descriptors + drainage
via one shared per-cell env; the hard invariant is the no-fine-causal-stack token scan).
`surface_digest` on the origin page is **byte-identical** to the committed MS1.A value (the
surface descriptor is unperturbed). Renderer green on the water-descriptor pages: **MV2.B 128 km
raster, 0 coverage holes, 25 unique pages, seam continuous** (the descriptor is inert to the
renderer; WD1.B consumes it).

## Visual evidence (DIAGNOSTIC category maps, NOT WD1.B optical appearance)

- `Docs/provenance_wd1a_water_state_maps.png` — presence / body-class / depth / turbidity /
  organic / mineral over the ±160 km origin world. Blue drainage networks over dry/damp ground;
  the frozen ±32 km centre reads **all-dry** (deferred to 16D–16F); wetlands (green) in the humid
  NW, rivers/lakes/volcanic pools elsewhere; turbidity separates clear headwaters from turbid
  sediment reaches.
- `Docs/provenance_wd1a_seed_corpus.png` — water geography across a seed corpus (different seed →
  different water world).

## What WD1.A is NOT (HARD CLOSED here — WD1.B and later)

The shared **optical appearance** (WD1.B: continuous transmission over the MS1 bottom substrate,
depth coloration, turbidity/mineral/organic colour, reflection, flow response), **waterfalls**
(WD1.C — `waterfall_potential` is a reserved hook only), volcanic/mineral chemistry, live
weather/season, snow/glaciers, groundwater simulation, new fluid physics. Inside ±32 km the
descriptor **defers** to frozen 16D–16F/P5b. MW1–MW8 / MV1 / MV2.* / MV3.* / MS1.* / PX frozen;
geometry and water mass untouched.

## Board

```
WD1     design                         LOCKED @ c2f9bd55
WD1.A   WaterState authority           CERTIFIED (this cut)
WD1.B   shared water appearance        NEXT (continuous optical transmission over MS1 substrate)
WD1.C   waterfalls                     CLOSED (hook only)
FL1 flora · AT1 atmosphere             CLOSED
```
