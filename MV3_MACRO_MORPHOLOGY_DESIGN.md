# MV3 — Macro Shape / Landscape-Family Diversity (DESIGN — LOCKED)

Design only; extends the LOCKED MV2.A continuous macro field, breaks none of its
constraints. §G is locked (below). Next step after commit: open **MV3.A**.

---

## 0. Foundational doctrine — the world is unbounded and seed-deterministic

**Provenance worlds are seed-deterministic and effectively spatially unbounded
(Minecraft-style world semantics).** Each new seed produces a materially different
large-scale world arrangement; new geography is generated deterministically as
exploration expands. **The 128 km radius is only the player's current visible horizon,
never the world extent.** Regional 64 km pages are streaming/cache units, **not**
geographic tiles.

```
NEW WORLD → choose/generate seed
seed + generator version + coordinate (x,y,z)  ==  the same untouched world truth, always
different seed                                 ==  a different world (different provinces,
                                                   ranges, basins, rivers, climates, ecology)

WORLD EXTENT        effectively unbounded (generated on demand)
AUTHORITATIVE       regions compiled deterministically as exploration requires
ACTIVE/INTERACTIVE  hundreds of m around relevant actors
VISIBLE             ~128 km around the current viewpoint
SAVED               baseline seed + generator version + sparse persistent consequences
                    (never the whole infinite voxel state)
```

Travelling 0 → 128 → 500 → 2,000 → 10,000 km continuously enters NEW deterministic
geography from the same causal grammar — never a repeat of the origin neighborhood, and
never a 64 km packaging cadence. An 800 km belt crosses ~13 pages because the world-scale
field says it exists there; packaging never dictates geography.

**The four coupled diversity grammars** (this program), which FEED each other causally —
never independent "generators decorating a world":
```
mountain history → drainage → erosion/deposition → soil → (× climate) → water
→ vegetation → habitat
```
1. **Landform** grammar — range age, uplift, structural style, feature scale, erosion
   morphology, secondary relief, basins/plateaus/mesas/escarpments/volcanics/canyons.
   ← **MV3 is this grammar.**
2. **Surface** grammar — geology, exposed rock, weathering, sediments, soils, wet/dry.
3. **Water** grammar — watersheds, rivers/tributaries/braids, lakes, wetlands, waterfalls,
   seasonal washes, floodplains, later coasts/glaciers.
4. **Living** grammar — forest structure, scrub, grassland, alpine, desert flora, riparian,
   later fauna.

Performance corollary: never instantiate the infinite world. The renderer needs authority
for a ~128 km horizon; expensive fine detail is generated only where proximity/interaction
requires it; farther unexplored geography stays cheap deterministic macro truth until
approached.

---

## A. Current vocabulary (grounded in `Tools/Worldgen/macro_authority.py`)

```
regional_field = _base (2 smooth sinusoid octaves) + Σ_belt (Gaussian ridge)
               + Σ_basin (Gaussian depression) + Σ_massif (smoothstep bump)
macro_z = central_envelope + anchor_window · regional_field
```
Every primitive is smooth and uniformly rounded — no age variation, flat tops, cliffs,
incision, facets, or cones. The world is one rounded terrain type at different amplitudes.
MV3 fixes this **by type, never by cranking amplitude.**

## B. Grammar — independent multi-wavelength control fields + scale primitives

### B1. Morphology operates at MANY characteristic wavelengths SIMULTANEOUSLY
Otherwise an endless seeded world feels repetitive (everything changing at one rhythm):
```
planetary / continental forcing   1000s km
tectonic belt / old range         300–1500+ km
regional plateau / basin          100–500 km
massif / secondary range          30–200 km
foothill / escarpment system      10–80 km
large local landform              2–30 km
ravine/cliff/terrace/stream       10 m–5 km   (finer grammars / MW, later)
```
**None of these align with the 64 km storage cadence** (§G6).

### B2. Independent continuous control fields (NOT one giant domain-warped field)
Separate slow, continuous, seeded absolute-coordinate scalar fields (wavelengths ~200–500
km+, incommensurate with 64 km, interpolated → continuity + no square signature). Keeping
them **independent** makes causal attribution clean, lets each be counterfactually
disabled, and makes them individually certifiable — a single mega-field yields pretty
noise but muddy causality.
```
seed → age/erosional-maturity · structural-energy/relief · substrate-competence
     · volcanic-tendency · plateau-tendency · deformation/asymmetry
     · structural-style weights (B3)
```
Families are **regions of this continuous space, not named types**; transitions continuous.

### B3. Structural style = continuous NORMALIZED tendency weights (not a hidden enum)
Explicit weights over structural tendencies, normalized (sum→1):
```
belt_weight · block_fault_weight · plateau_weight · volcanic_weight · cratonic_weight
```
So a locale reads e.g. "mostly plateau + some fault-block + small volcanic" — blends, not
hard borders between a "Sedona type" and a "Superstition type."

## C. Morphology operators (parameterized by B2/B3; analytic, no `ReconstructedZ`)

Each shapes the scale primitives; each a continuous function of (x,y) and local controls.

1. **Erosional maturity (`age`) — affects RELATIONSHIPS, not just smoothing.** `age` is
   NOT a blur knob. It restructures morphology:
   ```
   young: high local relief · steeper valley walls · strong ridge prominence
          · less subdued divides · more exposed structural form
   old:   lower regional relief · broader valleys · rounded divides
          · more dissected secondary drainage · thicker weathering/regolith potential
          · less continuous bare structural expression
   ```
   Realized by blending a **ridged/faceted** profile transform (sharp crests, steep
   flanks, high prominence) toward a **rounded, valley-broadened, divide-subdued** profile,
   plus age-scaled secondary-relief/dissection. Alaska-like vs Appalachian-like become
   genuinely different, not one a blur of the other.
2. **Plateau clamp + escarpment** — `plateau_tendency` saturates uplift to a flat top;
   where the pre-clamp field crosses threshold, a **steep-but-finite `tanh` escarpment**
   forms the edge (macro cliff, continuous at 1 km sampling). Mesas, plateaus, rim fronts.
3. **Asymmetric fault-block** (MV3.A if cheap) — `deformation/asymmetry` gives ranges a
   steep scarp face and gentle dip-slope (basin-and-range / Sierra / Ghats).
4. **Canyon incision** (MV3.B) — carved along a **real cached macro drainage graph**, not
   free-floating grooves (see §C-drainage).
5. **Volcanic cone** (MV3.B) — radial conical sub-type (± summit crater) where
   `volcanic_weight` high.
6. **Butte / resistant remnant** (MV3.B) — flat-topped steep-sided caps on eroded low
   plains where `substrate` competence high and surrounding relief low.

**C-drainage — the cached macro drainage graph (MV3.B).** Canyons must flow downhill,
join, cross page boundaries, terminate in real basins/outlets, and later align with
detailed MW4 drainage. So MV3.B does NOT use a free ridged-negative field as the drainage:
```
macro elevation field
 → coarse 1–4 km drainage samples
 → deterministic downslope routing
 → cached trunk drainage / thalweg graph   (compiled once per page/neighbor bundle,
 → analytic incision kernel around paths     never per rendered sample)
```
Domain-warped negative ridges may **perturb/detail** the canyon geometry but may **not
substitute** for downhill connectivity. This keeps the causal discipline (drainage is a
consequence of relief) and forward-aligns with MW4.

## D. Landscape families = points in the continuous parameter space (examples, not presets)

| Reference | age | style weights | substrate | dominant operators |
|---|---|---|---|---|
| Alaska Range | young | belt | hard | maturity(sharp)+belt+foothills |
| Smokies/Appalachia | old | belt | moderate | maturity(rounded,dissected)+belt |
| Sedona/Canyonlands | — | plateau | stratified | plateau+escarpment+drainage-canyon+butte |
| Superstitions | — | volcanic | hard | volcanic cone+sharp |
| Maharashtra/Colorado rim | — | plateau+block | stratified | plateau+asymmetric scarp |
| Isolated massifs/buttes | — | cratonic | hard remnants | butte remnant on low plain |
| Alpine lake basins | young | belt | hard | belt + closed-basin (water fills later) |
| Broad basins/plains | — | cratonic | soft | basin + low base |

## E. Constraints preserved (LOCKED MV2.A/§G rules)

Continuity by construction (escarpment/canyon walls steep-but-finite, macro-scale);
**central-region anchoring** (all morphology in `regional_field` gated by `anchor_window`;
frozen center untouched; four-boundary continuity re-proved); **no square signature**
(control wavelengths ≫ 64 km, incommensurate, continuous ops); **cheap** (analytic; drainage
graph compiled per-page-bundle, cached); **Python authority** (extends `macro_authority.py`;
Esoterica consumes the same `.mcp`; `generator_version` bumps); **no global relief crank**
(families differ by type; a low rounded range stays legitimately low).

## F. Certification strategy

1. **Morphology diversity** — sample N locations; per-locale descriptors (roundedness,
   flat-top fraction, incision depth, asymmetry, cone-ness, remnant density, prominence);
   prove ≥K distinct family clusters and that reference archetypes are each reachable.
2. **Seed-diversity at world scale (NEW — the Minecraft semantics cert)** —
   ```
   same seed + same coords            → exact morphology controls / shape (deterministic)
   different seed + same coords        → materially different regional arrangement
   long-distance sample Seed A at 0/250/500/1000/2000 km
                                       → meaningful variation, NO periodic repetition,
                                         NO 64 km cadence signature
   ```
   Proves the unbounded world is not one morphology repeated forever (not certifying those
   regions visually yet — proving non-repetition + real large-scale variation).
3. **Continuity** — C0 everywhere; bounded slope except intended finite escarpment/canyon
   walls; page boundaries continuous.
4. **No square signature** — descriptor statistics don't step at 64 km cell lines.
5. **Central anchoring** — central page ≡ frozen envelope; four-boundary continuity.
6. **Cheap / deterministic / non-repetition** — build cost per page unchanged; same seed
   reproduces; neighbors distinct.
Renderer stays green unchanged (MV2.B 128 km raster, seam, MV1 coverage holes = 0, Test B).

## G. §G — LOCKED

```
1. CONTROL     independent continuous absolute-coordinate scalar fields
               (age/maturity, structural energy/relief, substrate competence, volcanic
               tendency, plateau tendency, deformation/asymmetry). NOT one mega domain-warp.
2. STYLE       continuous NORMALIZED structural-tendency weights
               (belt / block-fault / plateau / volcanic / cratonic ...); no named-region enum.
3. DRAINAGE    MV3.B uses a cheap CACHED macro drainage potential/graph from the actual
               macro surface (coarse routing → thalweg graph → analytic incision kernel);
               warped negative ridges may perturb/detail but NOT substitute for downhill
               connectivity. Forward-aligns with MW4.
4. NAME        MV3 — Macro Morphology Grammar.
5. SCOPE       MV3.A: control fields · maturity/sharpness · plateau/escarpment · core
               structural variation (asymmetric block if cheap).
               MV3.B: drainage-aligned canyoning · volcanics · buttes/remnants · scarps
               / specialized forms. (B may split later if it grows broad.)
6. SCALE       morphology wavelengths span tens to 1000+ km, SIMULTANEOUSLY, independent
               of 64 km page ownership.
7. SEED        same seed+coord deterministic; different seeds → materially different world
               layouts; long-distance samples do not repeat periodically.
```

## H. Sequencing

- **MV3.A** — control fields + erosional-maturity(relationships)/sharpness +
  plateau/escarpment + core structural variation (asymmetric block if cheap). Re-cert
  MV2.A/.A2 (continuity/anchoring/no-square/cheap) + morphology-diversity + seed-diversity;
  renderer unchanged. Prove Alaska↔Smokies↔plateau reachable and seed-diverse.
- **MV3.B** — drainage graph + canyons + volcanics + buttes + scarps. Same certs, extended.
- **Then** (separate program, per doctrine's four grammars): surface → water → living, each
  fed causally by the prior.

## I. Non-goals

No implementation here. No MW9, no more visibility range, no global relief crank, no erosion
simulation (drainage is coarse deterministic routing, not sim), no renderer change, no
reopening MW1–MW8 / MV1 / MV2.A/.A2/.B/.C / PX1–PX3. Frozen central region stays frozen.

**Status:** LOCKED. Next: implement **MV3.A**.
