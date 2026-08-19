# MS1 — Surface / Material Landscape Grammar (DESIGN — LOCKED)

Scale + shape are frozen (MV2.*, MV3.A/B1/B1.1/B2). The world now has genuinely different
*places*; they still look like colored polygons because the renderer paints a **diagnostic
palette** (`VisualMaterial::CapColor` over a single layer's `DiagnosticMaterial`), and near
/ mid / far use different palettes. MS1 replaces that with a **derived surface presentation
of real material/state authority**, unified across distance. Design only; §G locked below;
next step = **MS1.A**.

## §G — LOCKED

```
1. APPEARANCE   shared albedo + hillshade + state breakup now; roughness proxy allowed;
                NO full-PBR dependency, no normal-map/material-library explosion yet.
2. DESCRIPTOR   compact SEMANTIC SurfaceState (substrate/lithology/wetness/weathering/
                soil_depth/organic_potential/exposure), quantised; NO stored RGB / renderer
                authority; compact but not cryptic (water/flora must read it later).
3. VOLCANIC     macro-derived outside detailed coverage (MV3 volcanic ancestry + maturity +
                substrate), SAME vocabulary as detailed states; do NOT modify frozen MW7.
4. DETAIL EXTENT  NO new fine MW generation outside ±32 km. 0–32 km = detailed certified MW;
                outside = cheap macro SurfaceState = the far-authority PROMISE. Refinement-
                on-approach is a LATER stage. Law: approach may ADD information, never
                CONTRADICT the macro promise (far "old weathered basalt" stays basalt up close).
5. RESOLVER     ONE shared C++ SurfaceState→appearance path used by MV1 AND MV2/MV3.
6. COMPOSITION  explicit exposure precedence: deposit → regolith → weathered parent → host
                bedrock; THEN wetness/organic/exposure state overlays.
7. GEOMETRY     MS1.A does NOT change certified terrain shape. Material alters coarse
                appearance + breakup only; physical material geometry (columns, benches,
                talus, cracked lava) is a LATER surface-structure pass.
8. CERT         near/far DOMINANT surface identity must AGREE; distance may reduce detail,
                never change the semantic family. (Certificate C below.)
```

## Core law

> **Surface appearance is a derived presentation of the actual material/state authority at
> a place. Distance may SIMPLIFY the presentation; it may NOT change what the place
> fundamentally is.** A basalt province is a dark volcanic surface at 128 km and exposed
> basalt/ash/mossy-rock at 2 m — the same identity at different resolutions, never three
> unrelated palettes because three render paths exist.

The authorities already exist and are certified — MS1 **composes** them; it does not invent
a new material catalog, and the renderer never owns authority.

## Q1 — The canonical surface-state vocabulary

A `SurfaceState` record at any (x,y) = **substrate class × lithology × state axes**:

**Substrate class** (primary; already the MW7 `RegolithProfileClass` vocabulary, extended
with the volcanic substrates B2 needs):
```
bare_bedrock · weathered_bedrock · thin_regolith · colluvium · talus · alluvium
· floodplain_sediment · basin_fill · organic_capable · waterlogged_mineral
+ (volcanic) fresh_lava · scoria/ash · weathered_basalt · volcanic_soil
```
**Lithology** (what the rock/sediment IS; from MW2/geology): granite · sandstone · shale ·
basalt · quartzite · … — colours bedrock/talus/exposure and drives weathering behaviour.

**State axes** (continuous [0,1], the user's list):
```
wetness · weathering(maturity) · grain/coarseness · soil_depth · stability
· organic_potential · exposure(bare-rock fraction)
```
Families the user named (fresh lava, layered sandstone benches, deep humid regolith, alpine
barren, …) are **regions of this space**, not named types — exactly as MV3 style/age were.

## Q2 — Which existing authority owns each component

| Component | Owning authority (already in the codebase) |
|---|---|
| substrate class | **MW7 `CausalRegionalRegolith`** (`ProfileClass`, `DiagnosticMaterial`) |
| lithology | **MW2 `CausalRegionalGeology`** / `CausalWorldGeology` (granite/sandstone/shale/…) |
| sediment / grain | **MW5 `CausalRegionalDeposition`** (+ compiled sediment/deposit) |
| wetness / climate state | **MW6 `CausalRegionalHydroclimate`** (`RegimeClass`: alpine_cold/windward_wet/leeward_dry/…) |
| weathering / maturity | age + **MW3 differential erosion** (fine) · MV3 `age` control (macro) |
| organic potential | **MW8 `CausalRegionalBiome`** (`RegimeClass`: alpine_barren…forest…wetland) — input, not flora itself |
| slope / exposure / morphology | MV1 surface normal · **MV3** morphology |
| drainage / wet corridors | **MV3.B1** accumulation + incision (macro) · MW hydrology (fine) |

Ownership is single-writer: no component is computed in two places. The renderer owns **none**.

## Derivation — `SurfaceState = compose(authorities)`

One pure function `surface_state(x,y)` composes the owners above into the vocabulary of Q1.

**Law A — exposure precedence (what is actually at the surface).** Several owners can
legitimately overlap at one point (host basalt + floodplain deposit + wet regolith +
organic-capable surface). A fixed precedence resolves *what is exposed* so a point never
gets four different materials depending on which layer was queried:
```
exposed substrate =  topmost depositional body (if present/exposed, MW5)
                  ↘  else regolith / soil profile (MW7)
                  ↘  else weathered parent material (age/erosion)
                  ↘  else host bedrock lithology (MW2)
then STATE overlays applied on the chosen substrate:  wetness · organic_potential
                                                     · exposure · weathering
```

It is defined at **two resolutions that share the vocabulary**, because the world has two
authority regimes (the unbounded-world doctrine):

- **Fine (central ±32 km, MW1–8 present):** compose the real MW2/5/6/7/8 samples →
  full-resolution `SurfaceState`. (Frozen authorities read-only.)
- **Macro (unbounded, MV3 only — no fine MW beyond the centre):** derive a **coarse
  `SurfaceState`** from the MV3 controls that are the macro proxies of the fine authority:
  `substrate` competence → bedrock↔regolith split; `volcanic` → volcanic substrates;
  `age` → weathering/soil_depth; `plateau` → layered-sedimentary exposure; drainage
  accumulation/incision → alluvium/floodplain/waterlogged + wetness; relief/elevation +
  hydroclimate-proxy → alpine_barren/talus. **Same vocabulary**, so a place reads the same
  whether the fine authority exists yet or not — and when detailed authority is later
  generated on approach it *refines* the macro `SurfaceState`, never contradicts it.

## Q3 — Coarse far representation

The MV2.A macro **pages gain a compiled per-cell `SurfaceState` descriptor** alongside
height (dominant substrate class + lithology + quantised state axes), produced by the SAME
macro derivation. The far renderer reads that descriptor — it does **not** invent an
elevation palette. Distance-collapse rule: keep the **dominant** substrate + lithology +
coarse state; drop sub-metre variation. (Cheap: a few bytes/cell; the drainage/controls are
already sampled during page compile.)

## Q4 — Near and far show the SAME identity

One **appearance function** `appearance(SurfaceState, distance) → colour/texture-response`,
**shared by MV1 (near) and MV2/MV3 (far)**. Distance is only a LOD input: far collapses
sub-classes and mixes toward the area-dominant material; near expands full detail
(lava/ash/moss patches, benches, talus aprons). The *dominant identity* is invariant with
distance — this is the hard cert target (§Cert). This is what finally kills the
white/gray/olive: there is one palette derived from material, not per-path palettes.

## Q5 — No authority duplication in the renderer

The renderer only ever **reads** `SurfaceState` (from MW1–8 near, from the macro-page
descriptor far) and applies the shared `appearance()` map. The single new renderer-side
artifact is that appearance map (material/state → colour/texture); it contains **no world
logic** (no "there is basalt here"). `VisualMaterial::CapColor`'s diagnostic/gem palette is
retired for the playable surface (kept only for the explicit debug views).

## Q6 — Water / snow / flora: separate authorities, composable overlays

`SurfaceState` is the **substrate layer**. Water (WD1), snow (a later separate authority),
and flora (FL1) are **overlay layers** composed on top through a defined interface: each
takes `SurfaceState` as *input* and returns an overlay that modifies the final appearance,
without owning the substrate.
```
final_appearance = compose_overlays( appearance(SurfaceState),
                                     water_overlay(SurfaceState, WD1),
                                     snow_overlay(SurfaceState, climate),
                                     flora_overlay(SurfaceState, FL1) )
```
- **water** replaces the surface where a body exists; its own colour law (depth/clarity/…)
  is WD1, but it *reads* substrate (bottom material) and drainage ancestry (MV3.B1).
- **snow** caps by climate/elevation/aspect; a separate authority, not baked into substrate.
- **flora** (moss→lichen→forbs→grass→forest) reads `organic_potential` + `wetness` +
  `soil_depth` + `substrate`; it tints/covers but the bare substrate still shows through
  cracks/steep/exposed — the exact "moss on wet rock in thin soil pockets" behaviour.
MS1 defines the substrate + the composition **slots**; it does not implement the overlays.

## Law B — material presentation, not geometry (bounded)

MS1.A controls surface **identity and presentation** (albedo family, brightness/roughness,
wet-darkening, weathering/exposure variation, cheap grain/breakup, hillshade). It does
**not** rewrite certified terrain geometry. Geometry-changing material structure — columnar
basalt, layered sandstone benches, talus slope angle, cracked-lava relief — is a **later
physical surface-structure pass**, not this cut. This keeps MS1 bounded to presentation.

## Certification strategy (design targets)

### Certificate C — near/far dominant-identity agreement (HARD)

Pick several real surface contexts — **exposed basalt slope · alluvial valley · wet basin ·
talus · weathered alpine rock · deep regolith** — and prove that the dominant surface
identity read down the resolution chain
```
128 km macro descriptor  →  32 km handoff  →  MV1 representation  →  192 m local query
```
**agrees** wherever both authorities legitimately overlap. Not identical *state detail*
(macro may know only "weathered volcanic substrate" while near knows "basalt, thin
regolith, wetness 0.37") — but near must **refine the same identity, never contradict it**
(the §G4 approach-may-add-not-contradict law). This is the certificate that kills
`far=olive / mid=white / near=gray`.

1. **Near→far semantic continuity (HARD)** — for many places, the dominant substrate +
   lithology identity read at 128 km == at 20 km == at 2 km == at 192 m (colour family,
   not exact pixel). The current three-palette mismatch must be gone.
2. **Family distinctness** — a volcanic province, a sandstone plateau, an old-humid range,
   an alpine range read as materially DIFFERENT surfaces (measurable colour/response
   separation), driven by authority not elevation.
3. **Authority-derived, not invented** — surface-off / diagnostic-off reproduces the frozen
   geometry; every surface colour traces to an owning authority (counterfactual: disable an
   owner → the corresponding surface aspect collapses).
4. **Central freeze** — MW1–8 authorities read-only; the ±32 km detailed world's *geometry*
   unchanged (only its presentation derives from the same authority instead of the palette).
5. **Composability** — water/snow/flora slots leave clean seams (substrate shows through;
   no double-owning).
6. **Cheap / unbounded** — macro `SurfaceState` is analytic from MV3 controls; page
   descriptor bounded; no new deep `ReconstructedZ` at macro range.

## Open decisions — RESOLVED (folded into §G above)

1. **Appearance representation** — flat per-material albedo+roughness now, or a small
   texture-response (grain/speckle) for near? (Lean: albedo + hillshade + a cheap grain
   term; PBR later.)
2. **Macro page descriptor size** — 1 byte dominant-substrate + 1 byte lithology + packed
   state, vs a richer struct. (Lean: minimal; a few bytes/cell.)
3. **Volcanic substrate source** — extend MW7 vocabulary vs a macro-only volcanic substrate
   derived from geology=volcanic + age. (Lean: macro-derived, shared vocabulary.)
4. **How much fine MW to expose beyond the centre** — MS1 stays macro-derived outside ±32 km
   (no fine MW there yet); confirm we do NOT try to generate MW1–8 detail at range in this
   cut (that is the future "detail on approach" capability).
5. **Where `appearance()` lives** — shared C++ used by both MV1 and the macro path (single
   source), confirm.

## Sequencing

Implementation splits so we prove the world *knows* the surface before we prove the renderer
*shows* it consistently:
```
MS1.A  SurfaceState authority + exposure-precedence composition + macro page descriptors
       (world knows what the surface is; geometry untouched; diagnostic palette still up)
MS1.B  shared C++ appearance resolver + retire the diagnostic palette for the playable surface
       + Certificate C near/far semantic continuity
  ↓
WD1  water diversity        (on the MV3.B1 drainage ancestry; WaterPresentation colour law)
  ↓
FL1  flora / ground cover   (moss→lichen→forbs→grass→forest from organic_potential + state)
  ↓
AT1  atmosphere / time / weather   (parallel/future; transforms the same world without changing it)
```

**Status:** design LOCKED (§G above). Next cut = **MS1.A**.

## Non-goals

No implementation here. No new world material catalog (compose existing authorities). No
reopening MW1–8 / MV1 / MV2.* / MV3.* / PX. No water/snow/flora/atmosphere implementation
(only their composition slots). No fine-MW detail generated at macro range in this cut.

**Status:** design LOCKED. Next cut = MS1.A (SurfaceState authority + composition + macro descriptors).
