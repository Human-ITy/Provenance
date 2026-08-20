# MV3.C — Alpine Peak / Ridge Hierarchy

Fourth MV3 cut (MV3.A `4f804588`; MV3.B1/B1.1 `1a2acb50`; MV3.B2 `4f5126fd`). **Python
world-authority only** (`Tools/Worldgen/macro_authority.py`); the renderer is unchanged and
consumes the regenerated `.mcp` pages. `generator_version` 4 → **5**; the 25-page ±160 km
ring re-emitted. Surface / material (MS1) stays frozen and usable.

## The question it answers

> Can young / high-energy mountain systems develop persistent ridge **spines**, major
> **summit nodes**, **secondary peaks**, **saddles/passes**, and high relative **prominence**
> — while old / weak ranges stay broad and rounded — from the same deterministic causal
> grammar depending on seed, structure, age, substrate and uplift?

Yes. The dominant mountain silhouette is no longer "broad uplift → rounded shoulder → the
highest sample is the summit." A range is now a peak **hierarchy**.

## Core law + operator

A mountain range gets a structural SPINE. In a high-energy province the alpine operator
builds, deterministically and analytically (no `ReconstructedZ`/erosion/water):

```
tectonic belt / uplift  →  range anchor (72 km jittered lattice, gated by ENERGY)
                        →  ridge spine (seeded axis)
                        →  major summit NODES (descending height hierarchy)
                        →  secondary peaks
                        →  deep SADDLES / passes (ridge sag between summits)
                        →  convergent pyramidal apexes joined by knife RIDGES
```

- **`_alpine_energy(ctl)`** = orogenic peak-building energy: `0.55·relief + 0.45·(1−age)`,
  GATED by belt style × rock competence. A weighted amplitude·youth term (NOT a 4-way
  product, which collapses to 0), so genuine young competent belts reach **0.5–0.8** while
  plains / old / plateau provinces stay ~0.
- **`_range_summits(gi,gj)`** compiles a range's summit hierarchy: dominant `Hdom`
  (1500–2700 m × energy × relief, the ALPINE superstructure ON TOP of the belt base),
  2–5 summits (solitary Denali-style dominant-plus-satellites, or cluster Himalaya-style
  near-equal), apex sharpness `p = 1.05 + 1.35·energy`, ridge width (knife when young),
  saddle sag, and the derived **key saddle + prominence**.
- **`_range_relief_from`** = `max` of convergent summit cones `H·(1−r/R)^p` and the sagging
  knife-ridge crests between consecutive summits — the `max` of cones naturally yields
  summits, cols (saddles) at the crossovers, ridges, and prominence (apex − saddle).
- **`_alpine_range`** has a cheap early-out (skips the lattice scan wherever the query point's
  energy < 0.12), so it costs nothing outside genuine alpine country. Added to
  `regional_field` AFTER the plateau reshape (summits never mesa-flattened), OUTSIDE the
  MV3.B2 `special` gate (B2 counterfactuals still isolate only B2), and scaled by the anchor
  window (0 in the frozen ±32 km centre → MW1–8 untouched).
- **`peak_at(x,y)`** publishes ancestry: `(class, MacroPeakId, ParentRangeId, summit_elev,
  prominence, key_saddle, cluster)`, `class ∈ {dominant_summit, secondary_peak, ridge, flank,
  none}`. A pure absolute-coordinate function (no super-tile dependency).

Old / weak provinces do **not** activate the operator, so they keep the existing broad
rounded crests (the Sleeping-Lady family) — deliberately preserved.

## Certification — all 9 MV3.C fixtures PASS (`Docs/provenance_mv2a_macro_authority_cert.txt`)

Preserved every prior fixture (MV2.A/A2 + MV3.A/B1/B1.1/B2 + MS1.A; 0 FAIL total), **plus**:

| Fixture | Result |
|---|---|
| peak prominence hierarchy | PASS — dominant alpine-H **3254 m**, **prominence 1660 m**, key saddle 1660 m below summit, hierarchy [3254, 2762, 2269, 1777] |
| range / peak ancestry | PASS — a range's summits carry distinct MacroPeakIds under ONE ParentRangeId (cluster) |
| young-sharp vs old-rounded | PASS — young apex slope **0.827 m/m** vs old belt crest **0.158** (young 5× steeper) |
| saddle / pass depth | PASS — pass sits **1174 m** below the lower summit (a real col, not a dome) |
| dominant summit towers | PASS — dominant apex **towers 3283 m** above surrounding terrain within 12 km |
| seed diversity | PASS — max prominence per seed {origin 1209, A 1660, B 869, **C 0**, D 1641}: 3 dramatic, 1 with none; deterministic |
| alpine central freeze | PASS — 0 inside ±32 km (anchor-window gated; MW1–8 untouched) |
| peak_at pure / deterministic | PASS — independent of super-tile ownership and identical across rebuild |
| cheap source | PASS — analytic (cones + ridge crests); no ReconstructedZ/erosion/QueryMaterial |

`compile_seconds ≈ 186` (< 200 budget). Renderer green on the regenerated gen-5 pages:
MV2.B 128 km raster, **0 MV2 coverage holes**, 25 unique pages, seam continuous.

## Visual evidence

- `Docs/provenance_mv3c_peak_hierarchy.png` — **authority**: top-down hillshade of the most
  prominent corpus range (seed-A, prominence 1660 m) with summit markers; the **along-ridge
  profile** shows peak→saddle→peak summit ordering; matched **young-sharp vs old-rounded**
  cross-sections.
- `Docs/provenance_mv3c_showcase.png` — **real C++ client** (`--cert-mv3c-showcase`; seed-D
  near-origin range through the MV2/MV3 macro pass + MS1.B material): a **valley-floor** view
  where the dominant peak towers over the player; a **40 km** view reading peak→saddle→peak;
  a **100 km** skyline where the range is distinguishable.

## Honest limits (noted, not hidden)

The macro authority is **1 km**; a narrow summit apex is captured within ~0.7 km, so the very
top reads slightly blunt close-up and a modest (Hdom ~2259 m) range is small on a 100 km
skyline. The **structure** (dominant peak, prominence, secondary peaks, saddles, young-vs-old
sharpness) is correct at 128 km / 30–50 km / valley-floor and is what this cut proves. A
**sub-macro peak/ridge refinement** (a cheap 250–500 m structural representation only where
peak potential is high — `MacroPeakId` / ridge spline / summit point already exist to hang it
on) is the noted follow-on for crisp close-up apexes, before detailed on-approach authority
takes over. **Glacial** sharpening (horns/arêtes/cirques) is deliberately NOT baked in — the
peak structure is the hook a later glacial-history cut reworks.

## HARD CLOSED (later cuts)

Sub-macro apex refinement, glacial erosion, detailed columnar/strata geometry, water diversity
(WD1), flora (FL1), atmosphere (AT1). MW1–MW8 / MV1 / MV2.* / MV3.A/B1/B1.1/B2 / MS1.* /
PX1–PX3 frozen; central geometry untouched; MS1 surface/material frozen and consumed as-is.

## Board

```
SCALE / VISIBILITY      CERTIFIED (MV2.*)
MORPHOLOGY              CERTIFIED (MV3.A)
DRAINAGE / CANYONS      CERTIFIED (MV3.B1/B1.1)
SPECIAL FORMS           CERTIFIED (MV3.B2)
ALPINE PEAK HIERARCHY   CERTIFIED (MV3.C — this cut)
SURFACE MATERIAL        CERTIFIED (MS1.A/B/B1)
NEXT                    WD1 water diversity (WD1.A state/type → WD1.B appearance)
FL1 flora · AT1 atmosphere   CLOSED
```
