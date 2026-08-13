# Provenance: Horizon-to-Hand Unified Matter Doctrine v1

**Status:** Architecture ruling and implementation boundary

**Date:** 10 August 2026

**Scope:** Causal worldgen, terrain, tools, fracture, loose bodies, flora, water, containers, support, crafting, and embodied presentation

**Companions:** `Provenance_Causal_Worldgen_Basis_v1.md`, `HORIZON_TO_HAND_BUILD_CONTRACT_v1.md`, `MATERIAL_TOOL_INTERACTION_INDEX_v1_2.md`

## 1. Ruling

Adopt this doctrine as the umbrella architecture for Horizon-to-Hand, subject to the maturity and representation boundaries below.

The constitutional sentence is:

> World generation creates persistent material history. Interaction transforms that same history. Representation may change; matter, identity, and causality do not silently reset.

The player-facing sentence is:

> If you can see what something is made of, you should be able to act on it as that material, and the world should show what actually happened.

The compact doctrine is:

> Everything consequential is accountable, every transformation has a cause, and every representation knows what truth it is presenting.

This does not mean every distant visible triangle already exists as exact simulated matter. Every materialized or interactable body must descend from an authoritative descriptor, and every consequential transformation must reconcile against authoritative matter state.

## 2. Maturity boundary

This doctrine deliberately separates four implementation horizons. Examples from a later horizon do not authorize work in an earlier one.

### 2.1 Live scaffolding

The repository already contains foundations that this doctrine preserves:

- integer material densities and canonical voxel accounting;
- hand, shovel, and pick action profiles;
- separate contact/query radii and transfer budgets;
- material roles, fabrics, and failure-family scaffolding;
- persistent mica-schist fracture work in the current proof lane;
- detached `MatterBody` and `AggregatePatch` scaffolding;
- bounded occupancy, D2 reconstruction, HF/D2 ownership, and local `SupportBelow`;
- receipt-driven matter mutation and Fablescript-owned body/aggregate identity;
- revision-scoped residency and wake behavior;
- the frozen P5a water-ledger foundation.

These are live foundations, not proof that the full doctrine is implemented.

### 2.2 Next certification work

The next work consists of bounded vertical proofs:

- persistent seam damage through significant coherent release;
- parent/body/fines reconciliation;
- support wake and lifecycle transitions;
- contrasting material responses under a shared action transaction;
- local, revision-scoped structural reevaluation;
- meaningful collision proxies for the physical property being certified;
- exact fracture complement for significant slabs, plates, wedges, and log sections;
- authoritative promotion and demotion between representation bands.

### 2.3 Deferred systems

These are valid destinations but not near-term certification claims:

- ecological succession on fallen wood;
- kerf-aware planks and persistent sawdust distributions;
- broad flora growth and fungal simulation;
- planet-wide coupled geomorphology;
- global structural graphs;
- universal boolean fracture of arbitrary render meshes;
- exact rigid simulation for dust and every pebble.

### 2.4 Water-blocked behavior

Bucket-water transfer, depth-aware wading, foot-water displacement, terrain exclusion, and shared terrain/water presentation revisions remain blocked. They describe the destination only.

P5b remains closed. No wording in this doctrine opens terrain-water coupling.

## 3. One world, several representations

```text
causal history
-> persistent feature and material descriptors
-> latent far-field representation
-> approach or disturbance
-> materialized accountable matter
-> occupied subvoxel form plus reconstructed surface
-> geometric contact and accepted action
-> deformation, fracture, transfer, or flow
-> structural remainder, loose bodies, and aggregates
-> new persistent world history
```

Continuity is semantic and conservative. It is not a demand to run maximum-fidelity geometry, contact, support, ecology, and fluid simulation across the whole world simultaneously.

Worldgen and interaction meet at the same material identity:

```text
FeatureId and material ancestry
-> current structural or latent representation
-> authoritative action and mutation receipt
-> surviving feature plus descendant matter identities
```

There is no separate loot universe and no separate scenery universe.

## 4. Constitutional invariants

### 4.1 No item-universe swap

Excavated, fractured, cut, poured, or crafted matter does not become unrelated loot. A detached object is a descendant representation of world matter.

### 4.2 Matter reconciles

For every accepted transformation:

```text
parent matter
= surviving structure
+ detached coherent bodies
+ aggregates or fines
+ transferred matter
+ explicit, audited loss channels
```

No visual effect, recipe, or client prediction may mint the difference.

### 4.3 Identity survives representation changes

Material, source feature, process history, and ownership remain queryable at the fidelity appropriate to the representation. Promotion, compaction, sleep, aggregation, and rendering changes do not silently reset ancestry.

### 4.4 Contact is not removal

The following are distinct:

- query envelope;
- actual contact patch or swept intersection;
- accepted transfer budget;
- persistent damaged region;
- released matter;
- final reconstructed geometry.

A contact radius is not a promise to remove a sphere of that radius.

### 4.5 Tools deliver action; materials determine response

Tool geometry, motion, force class, edge, inertia, and grip constrain an action. Material fabric, cohesion, moisture, damage, structure, and support determine the result.

### 4.6 Canonical voxels account for space; they do not mandate shape

Thin plates, blades, roots, branches, container walls, and fracture fragments may cross many voxel addresses while occupying far less than their bounds.

Canonical subvoxels own spatial matter accounting. D2 terrain, implicit surfaces, branch centerlines, rigid-body hulls, and tool meshes express occupied form without redefining mass.

### 4.7 Voids are absence of matter

A tunnel or cavity is not inserted scenery. It is persistent negative space left after authoritative matter moved.

### 4.8 Presentation cannot invent outcomes

Animation, particles, sounds, chips, splashes, and ripples express authoritative events. They do not independently fracture, transfer, conserve, or move matter.

### 4.9 Local materialization must reconcile

Expanding a latent descriptor into exact matter and compacting it again cannot mint or silently destroy matter or identity. Missing authority causes refusal or deferral, never invention.

### 4.10 Certification precedes coupling

A dependent lane cannot consume aspirational behavior as if it were already proven. In particular, P5a's existence does not open P5b.

## 5. Required vocabulary

| Term | Meaning |
|---|---|
| `ActionEnvelope` | Region in which a hand or tool may establish a candidate interaction |
| `ContactPatch` | Actual surface contact or swept intersection used to orient and localize action |
| `TransferBudget` | Maximum matter the action may accept, move, or detach |
| `DamagePatch` | Persistent material-local crack, notch, crush zone, severed fiber set, or similar state |
| `MaterialFormContract` | Rules connecting material fabric/state to deformation, fracture, loose form, support, and grip |
| `StructuralBody` | Attached matter participating in support or living structure |
| `MatterBody` | Detached coherent matter with mass, form, transform, provenance, and lifecycle state |
| `AggregatePatch` | Conserved matter below the useful threshold for persistent individual bodies |
| `FeatureId` | Stable ancestry for a formation, deposit, plant, construction, or other causal body |
| `RepresentationBand` | Fidelity selected by interaction relevance, size, persistence, and performance budget |
| `SupportObligation` | Revision-scoped requirement for an affected structural island or dependent body to be reevaluated |
| `CollisionProxy` | Bounded contact representation preserving the physical property under test |

## 6. Maturity matrix

| Capability | Status | Ruling |
|---|---|---|
| Integer material densities and voxel accounting | **LIVE** | Existing basis |
| Hand, shovel, and pick action profiles | **LIVE scaffold** | Contact radius and transfer budget exist; exact swept tool geometry is not claimed |
| `MaterialFormContract` | **LIVE scaffold** | Material roles and failure families exist |
| Persistent mica-schist fracture patch | **LIVE proof lane** | Current Horizon-to-Hand proof |
| Detached `MatterBody` and aggregate representation | **LIVE scaffold** | Conservation, support wake, and lifecycle still require per-lane certification |
| Per-material cavity language | **TARGET** | Dirt, sand, clay, shale, granite, sandstone, and schist require distinct fixtures |
| Exact fracture complement | **TARGET, bounded** | Required for significant coherent fragments; fines may reconcile statistically |
| Tool-head swept contact and inertia | **TARGET** | Not implied by a radius query |
| Cave and overhang support | **TARGET** | Local, revision-scoped, and bounded; never a global all-world graph |
| Living tree structural graph | **TARGET** | Requires coupled plant and material authority |
| Directional axe notch and true sectioning | **TARGET** | Preserve wood, offcuts, and fines |
| Solid bucket admission by geometric fit | **TARGET** | Opening, orientation, collision, volume, and mass checks |
| Hydraulic bucket fill and pour | **BLOCKED: P5a/P5b** | Do not implement against uncertified terrain-water coupling |
| Foot-water displacement and depth-aware wading | **BLOCKED: water coupling** | Presentation follows authoritative coupling only |
| Ecological succession on fallen wood | **DEFERRED** | Requires time, moisture, exposure, organism state, and streaming identity |
| Kerf-aware planks and sawdust | **DEFERRED** | Valid destination, not a near-term crafting requirement |
| Causal worldgen using common form laws | **DESCRIPTOR WORK OPEN** | Identity/rule descriptors may advance; active erosion and water coupling remain closed |

## 7. Live scale correction

Current code truth is:

```cpp
constexpr float kLiveHandContactRM = 0.16f;
```

`MATERIAL_TOOL_INTERACTION_INDEX_v1_2.md` also records the hand contact radius as `0.16 m`.

The `~3.88 cm` hand-contact statement in `HORIZON_TO_HAND_BUILD_CONTRACT_v1.md` is stale. It must not be used for implementation or certification and should be corrected in a dedicated companion-contract cleanup. This document does not reinterpret `0.16 m` as removed volume: it remains the live hand contact/query envelope, separate from the 244.14 mL transfer limit.

## 8. Representation bands

The engine spends identity and geometry where consequences require them.

| Band | Typical matter | Authoritative representation |
|---|---|---|
| Field | Soil matrix, water occupancy, fines | Integer ledger plus bounded scalar or occupancy field |
| Aggregate | Sand pile, crumbs, sawdust, tiny chips | Conserved aggregate patch with distribution parameters |
| Small body | Hand-scale stone, clod, wood chip | Persistent `MatterBody` with simplified hull |
| Structural fragment | Schist plate, log section, large wedge | Persistent body with event-derived fracture or cut surfaces |
| Living structure | Tree, root network, fungal colony | Persistent organ/branch graph coupled to material allocation |
| Far latent body | Distant formation, forest stand, deposit | Deterministic descriptor and identity, materialized on approach or disturbance |

Promotion and demotion between bands preserve:

- grams and material composition;
- causal ancestry;
- authoritative identity or a recorded parent/descendant relationship;
- damage and process state that remains consequential;
- ownership and containment;
- relevant biological state;
- the revision at which the transition was accepted.

Dust and tiny chips do not require permanent individual rigid bodies. A meaningful schist slab or log section does.

## 9. Coupled authority for living trees

A tree is not merely an anisotropic wood body, and it is not merely a decorative actor. It requires at least two coupled authorities.

```text
PlantState / PlantStructure
    identity, species, metabolism, age, health,
    growth points, branch topology, wounds, lifecycle

MaterialState
    wood, bark, foliage, internal water, stored matter,
    occupied form, attachments, damage, detached bodies
```

Plant authority decides biological change. Matter authority reconciles occupied form, geometry, transfer, and mass. Neither authority can be reconstructed from the decorative mesh.

Examples:

- growth allocates authoritative matter into new living structure;
- a cut creates a material damage patch and a biological wound;
- severing changes both the structural graph and living topology;
- a dead detached log leaves active plant metabolism but retains species, source-tree, grain, age, and wound provenance;
- later decay requires a separate ecological process and is deferred.

## 10. Exact fracture complement with bounded representation

For a coherent released slab, wedge, or section, the significant mating surfaces must descend from one fracture or cut event and a shared boundary definition.

The event must permit proof that:

- the parent remainder and released body partition the accepted significant volume;
- corresponding major surfaces are complementary within certified tolerance;
- both sides name the same event and prior revision;
- released grams plus surviving grams plus aggregate/fines channels reconcile;
- the complement survives representation changes at the fidelity required for gameplay.

Dust and tiny chips do not require permanently stored one-to-one manifold complements. Their material, mass, origin, and process channel still reconcile through an aggregate or fines representation.

Exact complement is therefore consequence-bounded, not particle-universal.

## 11. Local support obligations

A global attachment/support graph is neither required nor acceptable as recurring work.

Structural reevaluation operates on the edited island or bounded affected neighborhood:

```text
accepted mutation at revision R
-> derive affected spatial/attachment island
-> publish SupportObligation(R -> R+1)
-> evaluate local support using authoritative occupancy and attachments
-> wake only dependent bodies whose support result changed
-> publish revision-scoped structural result
-> settle and sleep when obligations are satisfied
```

Rules:

- `SupportBelow` remains occupancy-backed and local;
- D2 triangles and render meshes are not support authority;
- unchanged distant history performs zero recurring support work;
- missing authoritative occupancy causes refusal/defer;
- results must be deterministic across processing order;
- a stale support result cannot be applied to a newer structural revision;
- only affected dependents wake.

## 12. Collision proxies preserve obligations, not triangles

Collision geometry is not render geometry. Certification selects a proxy that preserves the property under test.

| Body/tool | Required proxy property |
|---|---|
| Axe | Edge position, orientation, thickness, and swept path |
| Bucket | Opening, rim, interior capacity, containment boundary, and orientation |
| Branch | Centerline, taper, junctions, and contact radius |
| Plate/slab | Significant fracture silhouette, thickness, mass distribution, and grip faces |
| Loose body | Support footprint, center of mass, stable contact, and relevant passage dimensions |
| Tool handle | Grip span, leverage axis, and collision where it affects the accepted action |

Triangle-perfect collision is not required. A proxy fails if simplifying it changes the certified physical decision—for example, allowing a block through a bucket rim it cannot geometrically pass.

## 13. Unified contact transaction

All hand and tool interactions converge on one causal transaction shape. Exact wire fields may evolve, but causality, expected revision, accepted amount, affected identity, and reconciliation are mandatory.

```cpp
struct MatterAction
{
    ActionId action_id;
    ActorId actor_id;
    ToolId tool_id;

    ForceClass force_class;
    ContactPatch contact;
    Vec3 impulse;
    Vec3 sweep_direction;

    int64_t transfer_budget_g;
    Revision expected_revision;
};

struct MatterMutationReceipt
{
    ActionId action_id;
    Revision prior_revision;
    Revision result_revision;

    int64_t structural_delta_g;
    int64_t detached_body_g;
    int64_t aggregate_g;
    int64_t transferred_g;

    std::vector<FeatureId> affected_features;
    std::vector<BodyId> created_bodies;
    std::vector<BodyId> changed_bodies;

    MutationKind kind;
    bool reconciled;
};
```

The client may predict presentation, but refusal or revision mismatch restores the exact pre-intent state under the existing rollback contract.

## 14. End-to-end material examples

### 14.1 Dirt and shovel

```text
swept shovel contact
-> accepted grams limited by tool and material
-> cohesion, moisture, and roots shape the mutation
-> structural remainder plus clod or aggregate
-> D2 republishes only affected surface obligations
```

The action may begin as a spoon-like cut. It does not promise an identical permanent shovel-mesh imprint across all soils.

### 14.2 Pick and foliated rock

```text
pick contact and impulse
-> persistent seam damage
-> foliation-aligned crack growth
-> attachment crosses failure threshold
-> coherent plate plus fines
-> parent, plate, and fines reconcile
-> local support and surface revisions publish
```

This remains the strongest first proof because its response is visibly different from a generic spherical carve.

### 14.3 Axe and log

```text
edge sweep relative to grain
-> bark and fiber damage patch
-> notch volume plus chips
-> remaining cross-section and support update
-> separation plane completes
-> two wood bodies plus conserved waste
```

Once a section is dead and detached, the plant lifecycle no longer drives its growth, but source tree, species, grain, age, and process provenance remain.

### 14.4 Bucket and solids

```text
candidate body at opening
-> remaining mass and volume capacity
-> orientation and minimum-cross-section test
-> collision-free path through rim
-> stable contained pose
-> ownership transfer receipt
```

Bounding-box size and number of crossed voxels are insufficient admission tests.

### 14.5 Bucket and water — destination only

```text
opening and orientation intersect certified water body
-> available container volume
-> authorized water transfer
-> world and container ledgers reconcile
-> shared presentation revision
```

This is a P5b-or-later proof. It is not part of current Horizon-to-Hand certification.

### 14.6 Wading — destination only

```text
leg/foot proxy intersects certified water volume
-> authoritative displacement and depth state
-> locomotion resistance and buoyancy inputs
-> shared terrain/water/body revision
-> contact-derived wake, splash, and ripple presentation
```

No visual-only depth or splash system may masquerade as this behavior before water coupling is authorized.

## 15. Shared material laws without a monolith

Worldgen, erosion, tool interaction, gravity, support, and crafting should consume common material facts without being forced through one universal process function.

```text
MaterialProperties
    density, cohesion, friction, hardness,
    tensile/compressive strength, fabric, moisture response

ProcessLaw
    geomorphic weathering
    tool fracture
    support failure
    settling
    cutting, hewing, and sawing

RepresentationLaw
    D2 surface
    rigid fragment
    aggregate
    living structural graph
    far-field descriptor
```

This prevents separate “worldgen sandstone” and “mineable sandstone” identities while allowing geological weathering and a pick strike to operate at different scales and timescales.

Active erosion, runoff, and sediment transport remain deferred under P5b even though they may later consume the same properties.

## 16. Ten-stage certification sequence

Do not attempt the complete destination at once. Each stage is a bounded vertical causal proof and must preserve all earlier gates.

### Stage 1 — persistent mica-schist release

Preserve the current fixture. Certify persistent seam damage, thresholded release, parent/body/fines reconciliation, fall, local support, grip, drop, sleep, and deterministic digest.

### Stage 2 — contrasting massive rock

Granite must produce a different failure and fragment family under the same pick transaction. Material law, not a separate tool script, owns the distinction.

### Stage 3 — soft cohesive matter

Dirt must prove action envelope versus accepted transfer, cohesion-sensitive form, bounded revision, and a non-spherical material-owned cavity.

### Stage 4 — granular matter

Sand must prove aggregation, repose, conservation, and stable sleep without requiring persistent rigid bodies for individual grains.

### Stage 5 — local cavity support

A deliberately small roof fixture must prove bounded affected-island reevaluation, revision scoping, deterministic failure, dependent wake, and zero unrelated work.

### Stage 6 — wood sectioning

One fixed log must prove directional notch accumulation, remaining-cross-section change, section completion, two persistent bodies, complementary significant cut surfaces, and conserved chips.

### Stage 7 — geometric solid containment

One schist plate must pass edge-first through a certified container opening while a granite block of similar volume fails. Admission must use opening, orientation, path, and capacity rather than bounds alone.

### Stage 8 — repair and certify P5a

Complete and preserve the deterministic hydraulic foundation independently: honest units, legal occupancy, derived levels, deterministic head solve, conservation, revision consistency, and dormant zero work.

This stage does not open P5b.

### Stage 9 — explicit P5b gate and coupled water proof

Only a separate architectural authorization may open P5b. Then certify terrain exclusion, basin leveling, legal displacement, bucket transfer, shared revisions, wading inputs, and no illegal occupancy.

### Stage 10 — causal-world continuity

A named formation must generate a landform, remain latent at distance, materialize locally, fracture under the same material law, and retain formation ancestry after authoritative extraction.

The `CAUSAL_WORLD` proof from `Provenance_Causal_Worldgen_Basis_v1.md` supplies the geological identity spine. Active erosion and water transport remain separate later work.

## 17. Explicit non-goals for the next cut

- No planet-wide exact support graph.
- No exact rigid simulation for dust or every pebble.
- No triangle-perfect collision requirement.
- No universal boolean fracture of arbitrary render meshes.
- No runtime simulation of geological epochs.
- No water coupling before P5a certification and explicit P5b authorization.
- No flora-growth simulation hidden in asset geometry.
- No crafting recipe allowed to mint output independently of admitted matter.
- No visual particle treated as conserved matter unless promoted into an authoritative representation band.
- No claim that contact radius equals removed volume.
- No claim that every occupied subvoxel requires its own persistent object.

## 18. Acceptance gates for implementation proposals

Any implementation proposed under this doctrine must state:

1. which maturity horizon it belongs to;
2. which authority owns its input, mutation, identity, and revision;
3. which representation band holds each output;
4. how grams and ancestry reconcile;
5. whether significant complement is required and how it is represented;
6. which local support obligations are published;
7. which collision property the chosen proxy preserves;
8. which cert proves deterministic behavior and bounded wake;
9. which existing frozen gates are rerun;
10. whether it touches water—and, if so, the explicit P5b authorization that permits it.

An implementation that cannot answer these questions is not ready to land.

## 19. Final doctrine

Provenance should feel continuous from mountain silhouette to a stone in the player’s hand because both are readings of the same causal material identity. The implementation earns that feeling through conservative transactions, persistent ancestry, bounded materialization, material-specific form laws, coupled biological and material authority where life requires it, and honest representation changes—not by simulating every atom or forcing every shape onto the voxel lattice.

The target is not “everything is a voxel” or “everything is a physics object.”

It is:

> Everything consequential is accountable, every transformation has a cause, and every representation knows what truth it is presenting.
