# MV2 — 100+ km Macro-Horizon Architecture (DESIGN — LOCKED)

Design-only cut. **No renderer or authority code changes in this document.** It
locks the architecture so MV2 is implemented against a settled design, and it
splits MV2 into two cuts (authority first, renderer second) so a brand-new
world-authority system and a brand-new horizon renderer are never mixed into one
run.

MV2 must solve the two constraints PX3 named, not rediscover them:
1. extreme-distance terrain must **not** repeatedly resample fine MW authority
   (`ReconstructedZ` ~4–8 ms/tile);
2. any batching must **not** write GPU-in-flight storage (the rejected slab mode).

---

## A. Two decisive findings from the current code (these shape everything)

### A1. The MW authority is a SINGLE 64 km region, tiled by wrap — it cannot supply non-repeating truth past 32 km

- `CausalMacroProvinces::kRegionHalfM = 32000` → the compiled region is **±32 km =
  64 km**, and MV1's horizon (`kMv1VisibleRangeM = 32000`) reaches **exactly** that
  edge.
- Every causal layer loads **one** compiled region from a fixed path
  (`Data\Worldgen\causal_world_macro_provinces_floor.cmp`, …) validated against a
  single `kExpectedRegion` key.
- `ReconstructedZ(x,y)` calls `WrapIntoRegion(program,x,y)` first: coordinates
  outside `[minX,maxX]×[minY,maxY]` fold back by `fmod`. Region features are
  origin-anchored (one massif, one fault domain, one belt, one basin).
- The province schema has latent hooks (`regionKey`, `seed`, `parentRegionKey`,
  `NeighborRelation`) but **no region-grid generator exists**.

**Consequence:** sampling at 100 km returns the **same 64 km world repeated ~3×**.
MV2 therefore **cannot** be "MV1, but farther." It needs a **new deterministic
regional macro-source**; this is **to-be-built**, not latent-and-wireable.

### A2. The expensive part of `ReconstructedZ` is fine erosion/geology; the macro silhouette is cheap

`CompileSurfaceAt(x,y)`:
- **Expensive:** `m_stage12.ReconstructedZ` (deep stacked geology kernel) **plus** a
  3-pass erosion loop and a 20-iteration bisection, each calling `QueryMaterial`
  (~23 geology queries/sample). This is the ~4–8 ms/tile.
- **Cheap (analytic, no geology queries):** `MassifLift`, `FoldRelief`,
  `FaultScarp`, `ValleyCut`, `RavineCut`, `BasinCut`, `ReceivingZone`, structural
  lift, datum.

A 100 km horizon needs **silhouette, summit ordering, ridge topology, massif
identity, basin negative space, relative elevation** — carried by the **cheap macro
envelope**, never by fine erosion grooves or per-material geology.

> **Rulings:** (1) MV2's 32–128 km band uses a new deterministic non-repeating
> macro-source; MV1's 0–32 km path is unchanged and its wrap edge is the seam.
> (2) The macro-source is built from a **cheap macro surface** sampled coarsely and
> cached, **never** the fine `ReconstructedZ` erosion/geology bisection.

---

## B. Source hierarchy (LOCKED)

```
WORLD AUTHORITY (Python)
  continuous planetary/regional macro field   (deterministic from world seed,
        ↓                                       absolute-coordinate, unbounded)
  64 km compiled regional macro PAGES          (serialization/cache/ownership unit)
        ↓
  local detailed MW authority where available  (the current certified 64 km region)

PRESENTATION (Esoterica)
  0–32 km    MV1.C / MV1.D    detailed ReconstructedZ-derived terrain (FROZEN)
  32–128 km  MV2              cached macro-page terrain (cheap macro envelope)
  128 km+    later            same hierarchy, coarser if required
```

- **MV1 source = expensive fine causal surface. MV2 source = cheap compiled macro
  envelope.** That split is the direct answer to PX3's measured scaling wall.
- **64 km is packaging, NOT feature cadence** (see §G3). Geological character does
  not change every 64 km; features have their own, usually much larger, wavelengths
  and cross whichever cells they intersect.

---

## C. The non-repeating regional macro-source — continuous absolute-coordinate field (LOCKED)

Not independent random region programs. A single **continuous macro field defined
over absolute world coordinates**, deterministic from the world seed:

```
world seed
   ↓
coarse ABSOLUTE-COORDINATE control lattice   (hash-seeded control VALUES at lattice nodes)
   ↓
continuous tectonic / province forcing fields (interpolated across the lattice)
   ↓
regional macro programs / page data          (any point samples the SAME continuous causes)
```

- Hashing generates the **control values** at lattice nodes; the field is then
  **interpolated continuously** across them. Two adjacent regions sample the same
  continuous field, so **boundary continuity exists by construction** — no
  after-the-fact seam blending.
- Feature wavelengths are independent of the 64 km cell: a mountain belt can run
  800 km across many cells; a page just samples whatever the field says locally.
  This is how Alaska-Range-scale structure emerges (belt ~800 km, broad basin
  ~250 km, massif ~90 km, major watershed ~60 km — all ≫ the 64 km cell).
- **Unbounded** in absolute coordinates: region cells `(…,-1,0),(0,0),(1,0),…`
  extend indefinitely; only the resident render ring is bounded (§G4).
- **Must not use the old `WrapIntoRegion` path** for anything beyond the central
  region.

---

## D. Central-region anchoring (HARD REQUIREMENT — first-class cert)

The existing certified 64 km region `[-32,+32] km` is FROZEN authority. The new
continuous macro field must be **anchored** to it so there is no invented seam at
32 km (MV1 says ridge = A; a naive new neighbor says continuation = B → geographic
wall/trench). MW1–MW8 are **not** rebuilt now.

```
existing MW1 macro envelope  →  fixed anchor / boundary constraint on the central region
new continuous field         →  extends OUTWARD from those boundary conditions
```

Long-term the continuous macro field should become the common ancestor of all MW
regions; for MV2 it only has to **meet the frozen central region continuously**.

**Boundary cert (MV2.A):** sample all four central boundaries `x=±32 km`,
`y=±32 km` and require, across the MV1→MV2 join:
- position continuity, major-slope continuity,
- ridge continuation, valley continuation,
- no artificial wall / trench.

Not centimeter equality at extreme-distance representation — but **no perceptible
geographic discontinuity**.

---

## E. No square-world signature (HARD REQUIREMENT)

64 km is an implementation cell; a player must never be able to infer it from the
terrain. Require `region boundary ≠ terrain boundary`. **Forbidden:** 64 km
mountain/elevation resets; square watershed limits; ridge-statistic changes on cell
lines; repeated region motifs; grid-aligned seams. Features must cross ownership
cells naturally (a consequence of the continuous field in §C, but cert it
explicitly).

---

## F. Regional macro-cache & page contract (LOCKED)

- **Authored by Python** (world authority). Esoterica **consumes compiled macro
  pages** and derives *presentation only* — the client never independently decides
  "there is a mountain here."
- **Each page carries at minimum:** `WorldSeed`, `GeneratorVersion`, `MacroRegionId`,
  absolute bounds, source revision/digest, neighbor lineage. So MV2 stays
  deterministic and revision-stamped; a lineage change refuses stale pages (mirror
  MV1's `Mv1SourceRevision` refuse-on-mismatch).
- **Page content:** a coarse height (+ macro material/color) grid over a 64 km cell
  (or sub-tile), sampled from the cheap macro surface at a coarse step (candidate
  256–512 m at 32–64 km, coarser beyond). Enough for silhouette + relative
  elevation, not grooves.
- **Cheap to build, reusable across movement:** a page depends only on its region
  cell + world seed, so it is **built once and reused** as the player moves; only
  new ring entries build. No per-frame fine resampling.
- **Bounded working set:** pages resident in a ring around the viewer, retired
  outside it; never grows with distance travelled.
- **Build budget yields to the frame** (the MV1 tile-build lesson): a page build
  must never be the unpreemptable spike. Coarse macro sampling keeps it well under
  budget; verify empirically.

**Representation error budget** (what a 50–128 km page must preserve / may discard):
```
PRESERVE                              DISCARD
✓ summit elevation / order            ✗ individual gullies
✓ major massif outlines               ✗ small tributaries
✓ principal ridge chains              ✗ local erosion grooves
✓ basin shape                         ✗ hand-scale geology
✓ mountain-belt direction             ✗ 12.5 cm matter
✓ major passes / saddles
✓ trunk-valley negative space
✓ large water/basin silhouette
```

---

## G. §G decisions — LOCKED

```
1. DERIVATION
   deterministic CONTINUOUS absolute-coordinate macro field;
   seeded/hash-backed control lattice, interpolated;
   NO independent random per-region programs; continuity by construction.

2. AUTHORITY
   Python macro compiler is the authority.
   Esoterica consumes compiled macro pages; renderer derives presentation only.

3. OWNERSHIP
   64 km macro page / cache / serialization cells.
   Feature wavelengths INDEPENDENT and often much larger (100/300/700/1000+ km).
   64 km is packaging, not terrain cadence.

4. EXTENT
   authority conceptually UNBOUNDED in absolute coordinates;
   MV2 resident horizon ring BOUNDED around the viewer;
   first certification target ~100–128 km (two 64 km regional steps).

5. COMPATIBILITY
   the current certified 64 km MW region is an ANCHORED central region;
   new neighboring authority must continue continuously across its boundary;
   NO WrapIntoRegion beyond it.

6. PERFORMANCE
   no deep ReconstructedZ / QueryMaterial resampling for MV2;
   page data reusable / cacheable;
   GPU storage build-then-promote / fenced if batched;
   never write GPU-in-flight memory.
```

---

## H. GPU ownership (sync-safe — the second PX3 lesson)

- Baseline: **per-tile/per-page persistent VBO ownership** (proven stall-free).
  Frustum cull + canonical draw order (band → Y → X → key) extend to MV2 pages.
- If batching is later needed: persistent-mapped **ring**, **fenced**
  multi-buffering, or **build-then-promote** immutable pages (write a page not yet
  referenced by any in-flight draw, then atomically publish). Shared-slab in-flight
  mutation is a **hard non-goal**.
- Two-pass depth split widens: far projection extends to ~128 km; the frozen
  0.03–600 m near pass depth precision is untouched. Confirm the far depth buffer
  still bins macro distance bands (MV1.C-style) at the wider range.

---

## I. Certification strategy (prove actual scale, not just draw distance)

1. **Scale readability:** from a divide station, 2 km ridge → 10 km massif → 35 km
   range → 70 km range → 100–128 km horizon each raster-visible (depth-binned at the
   widened far range), monotone aerial attenuation (MV1.D extended, I6).
2. **Non-repetition (anti-wrap gate):** region `(ri,rj)` ≠ `(ri+1,rj)` in
   silhouette / summit ordering; the horizon is not the 64 km region tiled.
3. **Central-region anchoring (§D):** four-boundary fixture, continuity, no
   wall/trench.
4. **No square-world signature (§E):** no grid-aligned seams / motif repetition /
   ridge-statistic steps on cell lines.
5. **Silhouette / topology preserved (§F budget):** ridges, summit order, massif
   identity, basin negative space present; grooves legitimately absent.
6. **MV1.D recalibration:** `T(d)=exp(-(ρ·d)²)` tuned for 32 km over-attenuates at
   128 km; recalibrate ρ (or a two-segment curve) so 128 km stays faintly readable,
   32–70 km reads as increasing depth, near bands unchanged; re-prove continuity
   (no band switch) and off==frozen.
7. **Gameplay hard gate holds:** PX2 engine gate green at 24 m/s with MV2 active;
   240 m/s reported (macro pages are cheap — must add no new gameplay spike).
8. **Determinism / H2H:** pages deterministic from region key + seed; two runs
   byte-identical; canonical order preserved.

---

## J. Sequencing — two cuts, authority BEFORE renderer

**MV2.A — Regional Macro Authority (Python + client consume, NO horizon render yet).**
Build and certify the non-repeating regional macro-source / page compiler. Prove:
- central region + N/S/E/W neighboring regions,
- deterministic MacroRegionIds + page metadata (§F contract),
- continuous boundaries (§D four-boundary fixture),
- non-repetition (§I2) and no square-world signature (§E),
- large features crossing region boundaries (§C),
- no `WrapIntoRegion` beyond the central region,
- macro-surface build cost cheap (no deep `ReconstructedZ`).

**MV2.B — Horizon renderer (only after MV2.A is green).** Client streams/caches
macro pages into a bounded ring, widens the two-pass far projection to ~128 km,
derives GPU horizon geometry (sync-safe ownership §H), recalibrates MV1.D (§I6),
and runs the §I cert battery. Join the frozen 32 km MV1 edge with no seam.

**After MV2:** landscape-diversity challenge cases (Alaska / Appalachia / Sedona /
Superstitions as MW1–MW8 grammar exercises), **then** MW9 population — once the
landscapes are large, visible, and diverse enough to make population meaningful.

## K. Non-goals (explicit)

- No reopening MW1–MW8, MV1/.C/.G/.D, PX1–PX3.
- No faking 100 km by stretching/repeating the 64 km region (A1); no `WrapIntoRegion`
  beyond the central region.
- No fine `ReconstructedZ` erosion/geology at macro range (A2).
- No shared-slab / in-flight GPU mutation batching (H).
- No 64 km terrain signature (E).
- No landscape-diversity library or MW9 population yet.

---

**Status:** design LOCKED. Next implementation cut is **MV2.A — Regional Macro
Authority**, certified green before any horizon rendering (MV2.B).
