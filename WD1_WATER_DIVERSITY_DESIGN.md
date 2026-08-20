# WD1 — Water Diversity Grammar (DESIGN — LOCKED)

Shape is frozen (MV2.* / MV3.A/B1/B1.1/B2/C) and surface/material is frozen (MS1.A/B/B1).
The world now has genuinely different *places* with coherent material surfaces — but water is
still the B1 blue drainage graph, i.e. "every channel = blue water." WD1 replaces that with a
**derived water presentation of real hydrologic truth**, unified across distance, exactly as
MS1 did for surfaces. **Design only**; §G locked below; next steps = **WD1.A** then **WD1.B**.

## The one question

> How does the endless seeded world determine (1) whether a drainage/basin location
> **currently contains water at all**, (2) what hydrologic **kind** of body/regime exists,
> (3) what physical/**optical state** that water has, and (4) how the macro promise **refines**
> into the frozen detailed water authority as the player approaches — **without duplicating or
> contradicting** frozen 16D–16F / P5b water truth?

## §G — LOCKED

```
1. CORE LAW    CHANNEL_EXISTS != WATER_PRESENT != WATER_BODY_TYPE != WATER_OPTICAL_STATE.
               Drainage says where water CAN travel; it never asserts water is there now.
               "water = blue" is forbidden; appearance is downstream of truth.
2. AUTHORITY   Detailed/active world: 16D occupancy / 16E identity / 16F redistribution / P5b
               coupling+pore stay AUTHORITATIVE. WD1 composes/queries them; NEVER replaces.
               Macro/unbounded world: WD1 derives a CHEAP deterministic WaterState PROMISE
               from MV3.B1 drainage + MV3 morphology + MS1 substrate/state + macro hydroclimate
               proxies + absolute coords/seed/version. NO fine 16D-16F occupancy generated at
               macro range.
3. DESCRIPTOR  compact SEMANTIC WaterState (presence/body/depth/flow/optical axes/substrate
               ref/ancestry), quantised; NO stored RGB, NO shader params as authority.
4. SUBSTRATE   MS1 owns the ground beneath the water. Water OCCUPIES substrate; it does not
               replace it. A stream is "basalt + water", recoverable through shallow water.
5. DEPTH       first-class + continuous. NO hard visual bands (0-1m cyan / 1-5m blue / ...).
               Authority gives coarse physical depth/depth-range; WD1.B → continuous transmission.
6. RESOLVER    ONE shared C++ WaterState→appearance path (WD1.B), used near AND far, layered
               OVER the MS1 substrate appearance as the water OVERLAY slot MS1 reserved.
7. REFINEMENT  128km promise → 32km body/regime → detailed 16D/E/F exact → 192m geometry →
               hand matter. Approach may ADD (shoreline, local depth, tributaries, sediment
               patches); it may NEVER CONTRADICT a legitimately-promised state (perennial vs
               dry, lake vs no-lake, trunk identity, dominant family).
8. FROZEN      WD1.A is classification only: creates NO water mass, changes NO terrain geometry,
               and WD1 off/on leaves 16D-16F / P5b truth byte-identical. Waterfalls, weather,
               snow/glaciers, groundwater sim, new fluid physics = CLOSED (hooks only).
```

## Core law (expanded)

Four independent concepts, never collapsed:

```
CHANNEL_EXISTS        MV3.B1 says a drainage line / basin is here (route + accumulation).
WATER_PRESENT         hydrologic PRESENCE now: none / damp / ephemeral / seasonal / perennial / standing.
WATER_BODY_TYPE       the regime family that presence takes (river / lake / wetland / spring / wash / …).
WATER_OPTICAL_STATE   depth · clarity · turbidity · sediment · mineral · organic · bottom · flow.
```

A macro drainage line with high accumulation in an arid, high-permeability province may be a
**dry wash**; the same line in a humid low-permeability province is a **perennial river**. Same
channel, different water — because presence derives from *causes*, not from the line existing.

## Q1 — The canonical `WaterState` vocabulary

A `WaterState` record references (never re-owns) MS1 substrate and MV3.B1 drainage:

**presence_regime** (ordered): `dry · damp_substrate · ephemeral · seasonal · perennial · standing`
**body_class** (consequence, not preset): `headwater_stream · perennial_river · sediment_river ·
braided_reach · alpine_lake · closed_basin_lake · floodplain_water · wetland_marsh ·
organic_darkwater · arid_wash · spring_pool · volcanic_mineral_pool · crater_lake · none`
**depth** — continuous coarse metres (quantised), + a `depth_class` collapse (shore/shallow/mid/deep)
**flow_regime** — `still · slow · channelized · fast · turbulent` (hydrologic CONTEXT only; no fluid sim)
**State axes** (continuous [0,1]): `clarity · turbidity · suspended_sediment · mineral_load ·
organic_load · temperature_proxy` (temperature only where it genuinely changes state)
**bottom_surface_family** — a reference into the MS1 `SurfaceState` beneath (rock/sediment/volcanic/…)
**ancestry / identity** — `MacroWatershedId`, `MacroChannelId` (from B1), `MacroWaterBodyId`
(standing bodies, keyed to the basin/outlet cell), `presence_source`, `source_rev/generator_version`

No RGB. No shader parameters. Families are regions of this space, exactly as MV3 style/age and
MS1 substrate were — not named biome types.

## Q2 — Authority ownership (single-writer; renderer owns none)

| Component | Owning authority |
|---|---|
| channel route / watershed / accumulation / basin+spill | **MV3.B1** (`drainage_query` → watershed/channel/accum/incision; interior_basins/boundary_terminals) |
| bottom substrate / permeability tendency / wetness / lithology | **MS1** (`surface_state_at`: substrate class, wetness, organic_potential, lithology) |
| macro moisture supply / aridity | **macro hydroclimate proxy already in MS1** (`_control(":macro_humidity")` broad band + lowland + drainage proximity + endorheic) + an elevation/continentality evaporation proxy |
| volcanic / mineral / geothermal ancestry | **MV3.B2** (`landform_at`: volcanic shield/cone/plug ancestry) + MV3 volcanic control |
| basin accommodation / lake geometry | **MV3.B1** terminals + **MV3** morphology (basins/relief) |
| **detailed present-water occupancy** | **16D** (frozen) — WD1 queries, never writes |
| **body identity / connectivity** | **16E** (frozen) |
| **redistribution / transfer / dynamic occupancy** | **16F** (frozen) |
| **terrain↔water coupling / wetting / pore storage** | **P5b** (frozen) |

WD1.A is a pure composition `water_state(x,y)` over the macro owners; inside detailed coverage
it **composes/queries** 16D–16F/P5b instead of deriving, and reconciles to the macro promise.

## Derivation — `WaterState = compose(authorities)` (two regimes, one vocabulary)

**Presence law (the gate).** Presence is a balance of *supply vs loss vs accommodation*:
```
supply       = f(macro_humidity, drainage accumulation upstream, spring/groundwater proxy)
loss         = f(aridity/evaporation proxy [elevation+continentality], substrate permeability [MS1])
accommodation= f(basin closure/terminal [B1], local gradient/relief [MV3], incision [B1])
presence_regime = quantise( supply − loss , accommodation )
  low supply / high loss                         → dry (channel exists, no water)
  marginal                                        → damp_substrate / ephemeral
  seasonal supply > loss part of the year (proxy) → seasonal / arid_wash
  supply > loss year-round                        → perennial (flowing) / standing (accommodated)
```
This is the "drainage ≠ water" law: the same B1 network yields different presence by supply/loss.

**Body/regime law (consequence of presence + setting).** Given presence, the body_class follows
from gradient, accumulation, accommodation, substrate, and ancestry:
```
flowing + steep + resistant + clear headwater        → headwater_stream (clear)
flowing + high accumulation + erodible/aggrading      → sediment_river (turbid) / braided_reach
standing + closed/endorheic basin                     → closed_basin_lake
standing + accommodated valley/cirque + cold/high      → alpine_lake (clear/cold)
standing + low-gradient wet basin + high organic       → wetland_marsh / organic_darkwater
flowing + low gradient + floodplain deposit (MS1)      → floodplain_water
marginal + spring/groundwater proxy                    → spring_pool
volcanic ancestry (B2) + closed basin/spring           → volcanic_mineral_pool / crater_lake (RARE)
```
Rare families stay rare (gated like MV3.B2 special forms). Not every seed has every family.

**Optical-state law (physical inputs for WD1.B).** Continuous axes derived from causes:
```
depth        ← accumulation + basin accommodation + local geometry (continuous metres)
clarity      ← inverse of turbidity; high in resistant headwaters / cold alpine / spring
turbidity    ← accumulation × substrate erodibility (MS1) × flow energy
sediment     ← upstream erodible substrate + aggradation (MS1 deposit context)
organic_load ← MS1 organic_potential × low-gradient standing/wetland residence
mineral_load ← MV3.B2 volcanic/mineral ancestry × closed-basin concentration
temperature  ← elevation/latitude proxy (only where it changes state: cold-clear vs warm)
bottom       ← MS1 SurfaceState beneath (visible through shallow, per WD1.B transmission)
```

Defined at **two resolutions sharing the vocabulary** (the unbounded-world doctrine):
- **Macro (unbounded):** derive from B1 drainage + MV3 + MS1 + macro hydroclimate proxy —
  cheap analytic, the far PROMISE. Anchor-window gated (0 in the frozen ±32 km centre).
- **Fine (detailed coverage):** compose/query 16D/16E/16F/P5b for exact occupancy/identity, and
  RECONCILE to the macro promise (near refines, never contradicts).

## Q3 — Macro page descriptor

Macro pages gain a compact per-cell `WaterState` descriptor (presence_regime + body_class +
coarse depth + optical axes + bottom-family ref + MacroWaterBodyId), produced by the SAME macro
derivation, versioned, a few bytes/cell — exactly like the MS1 surface descriptor. The far
renderer reads it; it does not invent water.

## Q4 — WD1.B shared water appearance (design targets only; implemented in WD1.B)

One `water_appearance(WaterState, depth, viewing) → optical response`, **layered over the MS1
substrate appearance** through the overlay slot MS1 reserved. Continuous **transmission /
absorption** (never depth bands): shallow → substrate strongly visible; increasing depth →
bottom contribution attenuates continuously; deep → body optical state dominates. "Type" sets
the coefficients (clear-alpine turquoise→cold-blue; sediment tan→brown-green; organic tea-dark;
volcanic/mineral chemistry-driven). Same substrate + 0.4 m clear water = visible dark basalt
under transparent green-blue; + 6 m = deep blue-green, bottom barely visible. Color is not
authority.

## Identity / cross-boundary law

A river/lake/wetland keeps its hydrologic identity across 64 km pages, 384 km drainage
super-tiles, and future cache boundaries. `MacroWatershedId`/`MacroChannelId` come from B1
(already window-independent per B1.1); `MacroWaterBodyId`/`MacroRegimeId` are keyed to the
absolute basin/outlet cell (pure absolute function, no super-tile dependency). Same seed+coords
→ same state+identity; different seed → materially different water geography.

## Refinement-on-approach (LOAD-BEARING — the compatibility contract)

```
128 km  macro WaterState promise (presence + body family + coarse depth + optical)
 32 km  coarse body/regime identity (from the descriptor)
detail  16D/16E/16F exact water bodies + occupancy + connectivity (frozen authority)
192 m   local water geometry / P5b terrain interaction
 hand   actual scoop / pour / matter
```
Near **may add**: exact shoreline, local depth, flow detail, sediment patches, small tributaries.
Near **may not contradict**, where the macro authority legitimately promised it: perennial vs
dry, lake vs no-lake, major trunk identity, dominant water family. Certificate (§certs 13) proves
the macro promise agrees with detailed 16D/E/F wherever overlap is legitimate.

## Reserved hooks (declared, NOT implemented)

- **Waterfalls (WD1.C later):** channel + sufficient discharge + abrupt terrain drop / resistant
  lip → `waterfall_potential` flag in WaterState. No waterfall water rendered in WD1.A/B.
- **Volcanic/mineral chemistry:** volcanic ancestry (B2) + closed basin/spring + mineral proxy →
  `mineral_potential`; concrete acid/thermal chemistry deferred to real authority.
- **Season/state:** a `season` input SLOT; WD1.A computes a baseline (annual-mean) presence.
  Live weather/rainfall stays CLOSED.
- **Glacial water:** slot reserved (milky turquoise) but CLOSED until glacial authority exists.

## WD1.A / WD1.B split

```
WD1.A  WaterState authority — presence · body/regime class · coarse depth · optical-state axes ·
       substrate reference · ancestry/identity · macro page descriptors · detailed 16D-16F
       compatibility. (World KNOWS what water is; creates no mass; geometry/16D-16F untouched.)
WD1.B  shared water appearance — continuous optical transmission · depth coloration · bottom
       visibility · turbidity/sediment/mineral/organic coloration · reflection/sky · flow response.
WD1.C  (later) waterfall / cascade presentation.
```
Do not combine A and B (the MS1.A/MS1.B discipline that worked).

## Planned certification (design targets)

1. **drainage ≠ water** — same B1 network under wet vs dry supply → different presence.
2. **perennial vs seasonal** — same channel, different supply/permeability → different regime.
3. **lake/basin** — closed/accommodated basin + supply → standing body; supply removed → diminishes.
4. **substrate influence** — same regime over basalt vs sand/alluvium → different bottom ancestry.
5. **sediment/turbidity** — high-accumulation erodible vs clear resistant headwater → different optics.
6. **organic wetland** — low-gradient wet basin + organic potential → dark/organic state.
7. **volcanic/mineral** — volcanic basin → mineral potential without making all volcanic water exotic.
8. **depth** — shore/channel/basin depth varies continuously + deterministically.
9. **cross-page identity** — body crosses 64 km pages with one ancestry.
10. **cross-super-tile identity** — same across the 384 km drainage packaging.
11. **seed semantics** — same seed deterministic; different seed materially different water geography.
12. **long-distance unbounded** — 0/250/500/1000/2000 km; no periodic water-pattern repetition.
13. **macro/fine compatibility** — macro promise agrees with detailed 16D/E/F where overlap is legit.
14. **frozen detailed authority** — WD1 off/on does not change 16D-16F / P5b truth (byte-identical).
15. **geometry/mass** — WD1.A classification creates no water mass, changes no terrain geometry.

## HARD CLOSED (this design + first implementation)

Full weather / live rainfall · snow / glaciers · groundwater simulation · waterfalls (hook only)
· new fluid physics · sediment-transport rewrite · flora · atmosphere/time · civilization.

## Open decisions — need approval before WD1.A

1. **Macro hydroclimate/supply source** — reuse the existing MS1 `_control(":macro_humidity")`
   band + drainage-derived wetness + an elevation/continentality evaporation proxy (no new
   authority), vs. open a dedicated macro-hydroclimate cut first. **Lean: reuse existing
   proxies** for WD1.A; a richer macro hydroclimate is a possible later refinement.
2. **Seasonal baseline** — WD1.A emits a single annual-mean presence with a reserved `season`
   slot (no live variation), vs. emit a min/mean/max presence band now. **Lean: annual-mean +
   reserved slot.**
3. **Depth representation** — continuous coarse metres + a 4-class collapse, vs. depth-range
   band only. **Lean: continuous coarse metres (WD1.B owns the transmission curve).**
4. **MacroWaterBodyId keying** — key standing bodies to the B1 basin-outlet/terminal cell (reuse
   B1's window-independent ids). **Lean: yes; add MacroWaterBodyId/MacroRegimeId keyed to the
   outlet cell; rivers reuse MacroChannelId.**
5. **Descriptor scope** — add a WaterState page descriptor alongside the MS1 surface descriptor
   (a few bytes/cell), vs. derive water at render time from the existing descriptors. **Lean:
   add the descriptor** (cacheable, deterministic, cheap; mirrors MS1).

**Status:** design LOCKED (§G above). Next cut = **WD1.A** (WaterState authority + presence law +
macro descriptors), then **WD1.B** (shared water appearance), then WD1.C (waterfalls, later).

## Non-goals

No implementation here. No new fluid physics or water-mass creation. No reopening 16D–16F / P5b /
MW1–MW8 / MV1 / MV2.* / MV3.* / MS1.* / PX. No fine occupancy generated at macro range. Water
appearance (WD1.B) and waterfalls (WD1.C) are separate later cuts.
