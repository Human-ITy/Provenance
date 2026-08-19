# MV3.B2 — Specialized Macro Landforms

Third MV3 cut (design `4108de63`; MV3.A `4f804588`; MV3.B1/B1.1 `1a2acb50`). **Python
world-authority only** (`Tools/Worldgen/macro_authority.py`); renderer unchanged, consumes
regenerated `.mcp` pages. `generator_version` 3 → 4; 25-page ±2 ring re-emitted.

**Core law honoured:** special forms are *consequences of the continuous morphology
controls*, never stamped POIs. There is no `GenerateSedona()` / `PlaceButte()`. Each form
is a bounded analytic operator gated by the existing control fields, so a form only appears
where its cause dominates.

## Operators (all analytic; no `ReconstructedZ`/erosion/water)

- **Volcanic constructs** (`_volcanic`, gated on the volcanic-*potential* field, not the
  rare style-weight): young provinces build **shields/cones** (radial, ± summit crater);
  an **old** volcano's soft edifice erodes to a resistant **plug/neck** spire (differential-
  erosion remnant).
- **Mesa / butte** (`_mesa_butte`, gated on plateau tendency + maturity + resistant cap):
  in a mature plateau province the tableland dissects; resistant caps survive as
  flat-topped **mesas** (younger/larger) and isolated **buttes** (older/smaller), tops at
  the parent plateau level — genuine erosional remnants, not procedural bumps. Drainage
  (MV3.B1) carves the dissecting canyons.
- **Fault-block / scarp** (in `_belt` via the `asym` control): steep structural front,
  gentler backslope; certified 2.35× front/back slope ratio that collapses to 1.37 when
  `asym` is neutralised.
- **Tower / needle** (`_tower`, very rare): narrow tall survivors where substrate
  competence is extreme, on their own fine lattice near parent structure.

Ancestry published by `landform_at(x,y)` → `(class, MacroLandformId, ParentMacroFeatureId,
age, substrate, center)`. **`landform_at` is a pure absolute-coordinate function** (no
super-tile dependency), so forms are automatically identical across the 384 km drainage
edge. Special forms are gated by `anchor_window` (0 in the frozen centre): the ±32 km
MW1–8 world is untouched, and `special=False` reproduces the exact MV3.B1 parent surface.

## Certification — all fixtures PASS (across a deterministic seed CORPUS)

Preserved MV2.A/A2 + MV3.A + MV3.B1/B1.1 fixtures, **plus** the 10 MV3.B2 fixtures:

| Fixture | Result |
|---|---|
| volcanic causality + counterfactual | PASS — cone adds +452 m on the graph; `special=False` == parent MV3.B1 |
| plateau → mesa → butte ancestry | PASS — 13 mesas + 70 buttes share the parent-plateau province |
| fault-block asymmetry | PASS — front/back slope **2.35× → 1.37** when `asym` neutralised |
| special-form rarity | PASS — max **9.1 %** coverage across corpus (forms stay meaningful) |
| cross-page continuity | PASS — a volcanic province spans **3** 64 km pages, one identity |
| cross-super-tile safety | PASS — `landform_at` independent of super-tile ownership |
| maturity response | PASS — younger→mesas, older→smaller buttes, old volcano→plug (relationships, not blur) |
| seed diversity | PASS — deterministic; **4/4 distinct form mixes**; long-distance non-periodic |
| central freeze | PASS — `max|special-on − off|` in ±32 km centre = **0** (MW1–8 untouched) |
| cheap source | PASS — analytic only |

**The diversity rule holds:** the origin seed is quiet (a couple of constructs); seed-A is
a dramatic **volcanic field + towers**; seed-B is **mesa country**; seed-C is **sparse
plains**. No world is stuffed with every form; different seed/location → different mixture.

Renderer green with v4 pages: MV2.B 128 km raster / seam / **0 MV2-domain holes**, MV2.C
presence, MV1 coverage **0 below-terrain holes** (resident set yaw-invariant), and **Test B
24 m/s gameplay hard gate** (engine 0-over, 0 stage-owned, cadence no-regression, 0 movement
frames > 16.667).

## Visual evidence

- `Docs/provenance_mv3b2_landform_diversity.png` — top-down hillshade + landform-class
  colour across the corpus: volcanic field (red) vs mesa country (orange) vs quiet plains,
  with rare towers (magenta). Clearly reads as different landscape families per seed.
- `Docs/provenance_mv3b2_landform_profiles.png` — oblique player-ish profiles (volcanic
  construct, mesa, butte).

**Honest note on fidelity:** these are macro forms at 1–2 km authority resolution, proven
by cert + top-down morphology; the oblique views come from a crude pure-Python rasterizer
(chunky, km-scale escarpments read as steep slopes, not vertical cliffs). True player-view
fidelity (crisp cliffs, walk-up detail) belongs to the detailed renderer / MW4 when the
player approaches — this cut proves the **grammar and causal placement**, not final
close-up beauty.

## HARD CLOSED (later cuts)

Detailed water diversity / waterfalls / water colour+depth, groundwater, glacial carving,
snow/ice, weather/atmosphere, MW9 flora/fauna, civilization/opportunity grammar, fantasy
anomalies, caves/karst/arches. MW1–MW8 / MV1 / MV2.A/.A2/.B/.C / PX1–PX3 frozen; central
region untouched.

## Board

```
MV3.A   morphology controls        CERTIFIED
MV3.B1  drainage / canyons         CERTIFIED (+ B1.1 super-tile continuity)
MV3.B2  special natural forms      CERTIFIED
NEXT    terrain/surface diversity  READY FOR DESIGN
LATER   water diversity · flora
MW9                                CLOSED
```
