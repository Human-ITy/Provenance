# MV3.B1 — Macro Drainage Graph + Canyon Morphology

Second implementation cut of the MV3 grammar (design `4108de63`, MV3.A `4f804588`).
**Python world-authority only** (`Tools/Worldgen/macro_authority.py`); the renderer is
unchanged and consumes regenerated `.mcp` pages. `generator_version` 2 → 3; the 25-page
±2 ring re-emitted with drainage-carved canyons baked in.

**Core law honoured:** canyons *follow* a real drainage solution; drainage is never
painted to resemble canyons. No free ridged-noise grooves; no erosion sim; no runtime
water.

## Pipeline (all cheap analytic, no `ReconstructedZ`)

```
MV3.A macro surface  (macro_z; the source the graph routes on)
 → coarse 2 km absolute-coordinate sampling over an origin-aligned SUPER-TILE
   (384 km core + 152 km halo; independent of the 64 km pages)
 → WINDOW-INDEPENDENT routing: D8 steepest descent on the RAW macro surface (a pure
   function of the absolute field in a fixed local neighbourhood) + a bounded local
   carve (breach small pits by jumping to the lowest cell within 24 km); genuine
   closed basins with no lower cell in reach stay endorheic sinks
 → flow accumulation (upstream area, capped at 1700 cells)  +  watershed labelling
 → trunk channels (accum ≥ 220 cells = 880 km²)  +  MacroWatershedId / MacroChannelId
 → analytic incision around the certified thalwegs, cross-section shaped by
   substrate competence + erosional maturity (+ plateau context)
 → baked into pages as macro_z − anchor_window·incision  (incision = 0 in frozen centre)
```

**Why raw-surface D8, not a windowed priority-flood?** A per-super-tile priority-flood
is window-dependent (its fill propagates from the window edge), so two neighbouring
super-tiles disagreed in their shared halo (measured: D8 direction agreed only 45.9 %,
incision differed up to 600 m across the 384 km edge) — a river crossing that edge would
reset its watershed and kink its canyon. Raw-surface D8 + bounded carve is a **pure
function of absolute coordinates**, so adjacent super-tiles compute IDENTICAL routing in
the overlap (**100 %** direction agreement, incision p99 **18 m**). The accumulation cap
makes major trunks saturate in both neighbours so incision magnitude agrees at the seam.

Ownership is keyed to fixed **absolute super-tiles**, so a trunk crossing pages *or the
384 km super-tile edge* keeps one identity/ancestry. The 25-page ±160 km render ring lies
inside super-tile `(0,0)`; when the player travels past it, neighbouring super-tiles join
seam-free by construction. The graph is compiled/cached **once per super-tile**; page
emission and horizon sampling just read the baked incision.

## First-class outputs

`MacroWatershedId`, `MacroChannelId` (stable trunk ancestry), upstream/downstream
relations (`down`, `ups`), flow `accum` / hierarchy, spill/outlet terminals,
`src_rev` (generator + drainage version), and the incision/canyon influence field.
Canyon cross-section: resistant/young → narrow steep canyon; weak/old → broad valley
(a genuine morphological difference under the same flow forcing, not one universal cut).

## Certification — all fixtures PASS (`cert_macro_authority.py`)

Preserved MV2.A/A2 + MV3.A invariants, **plus** the 10 MV3.B1 fixtures:

| Fixture | Result |
|---|---|
| downhill routing | PASS — `ascending_edges=0` (every non-terminal edge descends) |
| confluence + hierarchy | PASS — tributaries merge, downstream accum ≥ sum, stable identity |
| cross-page continuity | PASS — main stem (330 cells) crosses **15** of the 64 km page boundaries with **1** watershed identity, seam-free |
| plateau canyon + counterfactual | PASS — incised plateau channel on the graph; `incise=False` == exact MV3.A surface |
| maturity/substrate response | PASS — resistant/young 5110 m vs weak/old 7217 m mean cross-section (**1.41×**) |
| closed basin / spill | PASS — all depressions resolved; **0** terminals on 64 km page lines (no page-edge drain hack) |
| seed semantics | PASS — same seed → identical graph digest; different seed → different |
| long-distance world | PASS — super-tile(0,0) ≠ super-tile(5,0)@1920 km (non-periodic, page-independent) |
| no square signature | PASS — channel density on 64 km lines 0.022 vs interior 0.028 |
| H2H — centre frozen | PASS — `max|incised−MV3.A|` inside ±32 km = **0** (MW4 not overwritten) |
| **B1.1 super-tile boundary continuity** | PASS — across the 384 km super-tile edge: **100 %** D8 direction agreement, incision **p99 18 m**, a trunk crossing the edge keeps one MacroChannel identity (the unbounded-world invariant: rivers ignore the drainage-tile edge too) |

Renderer green with the incised v3 pages: MV2.B 128 km raster / 32 km seam / **0
MV2-domain coverage holes**, MV2.C presence, and **Test B 24 m/s gameplay hard gate**
(engine 0-over, 0 stage-owned, cadence no-regression, 0 movement frames > 16.667). PX3
240 m/s stress ceiling not chased; pages are pre-baked so gameplay cost is unchanged.

Evidence: `Docs/provenance_mv3b1_drainage_canyons.png` — dendritic watershed network,
confluences, trunks crossing 64 km page lines, valleys/canyons in the hillshaded surface;
`Docs/provenance_mv3b11_supertile_boundary.png` — a channel crossing the 384 km super-tile
edge with one watershed identity and a continuous incised valley (no reset/seam).

## Honest notes / limits

- **Coarse 2 km macro resolution.** Canyons are macro valleys (a few km wide), not slot
  canyons; D8 routing on the 2 km grid can produce locally rectilinear reaches. This is
  horizon/macro authority — detailed local drainage aligns with MW4 when the player
  approaches (H2H compatibility, not replacement). MW4 is not overwritten.
- **Incision coverage ≈16 %** of the ring at the current trunk threshold — major valleys,
  not every rivulet.
- Accumulation is capped (1700 cells) so magnitude agrees across super-tile edges; ample
  for macro canyon forcing. The single largest cross-edge trunk can still differ by up to
  ~180 m at the exact 384 km seam (p99 is 18 m); refined by MW4 detail on approach.

## HARD CLOSED (deferred to MV3.B2 or later)

Detailed/runtime water, waterfalls, groundwater, live erosion, sediment transport,
buttes/remnants, specialized fault scarps, volcanic-field expansion, MW9 flora/fauna,
snow/glaciers/weather, civilization/history. MW1–MW8 / MV1 / MV2.A/.A2/.B/.C / PX1–PX3
frozen; central region untouched.

## Board

```
MV3.A morphology controls     CERTIFIED
MV3.B1 macro drainage/canyons CERTIFIED
MV3.B2 special forms          NEXT (buttes, volcanic fields, scarps, tower remnants)
MW9                           CLOSED
```
