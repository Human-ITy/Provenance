# Provenance Causal Worldgen Basis v1

**Status:** Internal design draft; architectural ruling, not implementation authorization

**Scope:** Geological history, persistent bodies, query contracts, lazy materialization, and the first causal-world proof

**Out of scope:** P5b terrain-water coupling, active groundwater, rivers, erosion transport simulation, sediment transport, soil/ecology implementation, and production planetary scale

## 1. Decision

Provenance world generation will be **body-first and causal**.

The generator will describe a deterministic history of persistent three-dimensional material bodies. Present terrain, exposed rock, underground continuation, structure, deposits, and later ecological inputs are readings of that history. They are not independent noise selections painted onto a heightfield.

The causal basis is:

```text
world identity
-> planetary and tectonic history
-> persistent 3D geological bodies
-> chronological deformation and intrusion
-> compiled present-day boundary
-> surface and underground queries
-> lazy exact-matter materialization
-> authoritative mutation receipts
```

Long-term, the larger world history may include epoch feedback:

```text
epoch landscape
-> climate and runoff
-> erosion and deposition
-> next epoch landscape
```

That loop is compiled world history, not a continuously running simulation of geological time. It is also deferred by the P5b closure described below.

The master rule is:

> Provenance generates persistent material histories, not heightmaps. Terrain, water, soil, habitat, exposed resources, and extracted matter are different readings or descendants of one causal world.

## 2. Locked rulings

1. Adopt body-first, causal worldgen with persistent geological ancestry.
2. Preserve `RANGE` and `TORTURE` unchanged as certification fixtures. Add a distinct `CAUSAL_WORLD` fixture.
3. Keep exact matter latent until proximity, exposure, disturbance, or gameplay meaning requires it.
4. Resolve geology authority in favor of the existing project authority law before implementation: Python/Fablescript owns canonical world history; Esoterica consumes and evaluates a compiled, hash-verifiable representation.
5. Keep P5b closed. Groundwater, rivers, runoff-driven terrain change, erosion, deposition, and sediment transport remain deferred.
6. The first proof contains folded formations, a granite intrusion, a fault, a host-valid deposit, a surface breach, and underground continuation sharing persistent identity.

No item in this document authorizes modification of the frozen D2, occupancy, HF handoff, support, water, or receipt contracts.

## 3. Why the current geography is not the final generator

`Code/Applications/ProvenanceClient/ProvenanceGeography.h` is a valuable terrain and interaction prototype. Its current public chain is approximately:

```text
world seed -> province -> geology -> relief -> surface cap
```

Internally it remains an authored/noise-driven fixture system:

- `ProvinceField` is a two-dimensional scalar field.
- The `Province` enum mixes tectonic settings, depositional settings, and present landforms.
- `TortureRockAt` selects one dominant material for each X/Y location.
- `RangeRockAt` assigns material along an authored certification transect.
- `StratumAt` uses fixed depth thresholds rather than named bodies with geometry and history.
- Present relief, material, cap, and subsurface identity do not descend from the same persistent geological objects.
- A material answer has no formation, body, event, deposit, or ancestry identity.

Those properties are correct for the current certification role. They must not silently become production geological law.

### 3.1 Fixture preservation

The fixture enum becomes conceptually:

```cpp
enum class GeoFixture : uint8_t
{
    Range = 0,
    Torture,
    CausalWorld
};
```

Semantics:

| Fixture | Purpose | Change policy |
|---|---|---|
| `RANGE` | Authored Horizon-to-Hand interaction and presentation transect | Preserve output and existing cert expectations |
| `TORTURE` | Extreme D2/terrain edge-case stress | Preserve output and existing cert expectations |
| `CAUSAL_WORLD` | Body identity, event precedence, exposure, host validity, and continuity proof | New path with separate cert and generator version |

`F8` behavior, command-line aliases, and existing certs must not be broadened accidentally. Existing certs continue to force `RANGE` where they do today. `CAUSAL_WORLD` receives an explicit selector and its own proof command.

## 4. Authority ruling

### 4.1 Conflict

The current C++ header describes itself as Esoterica-native deterministic planet geography, while repository law says:

- Python is the only world-state authority.
- Fablescript issues matter identity and mutation receipts.
- Esoterica owns intent, prediction, cache, and presentation.
- Clients do not invent authoritative matter state or identity.

Adding canonical geological identity exclusively in C++ would create a second world-state authority and make cross-client history capable of divergence.

### 4.2 Resolution

**Canonical geological history belongs to Python/Fablescript.**

Python/Fablescript owns:

- world seed interpretation and generator version;
- stable feature identity derivation;
- the ordered geological event program;
- formation, intrusion, fault, and deposit descriptors;
- event precedence and parent/child ancestry;
- canonical descriptor serialization and content digest;
- publication of compiled regional history;
- all authoritative material mutations and matter-body identities.

Esoterica owns:

- requesting descriptors for a region and level of detail;
- caching immutable compiled descriptors;
- deterministic local evaluation of published shapes and transforms;
- surface and geology query acceleration;
- HF, occupancy, D2, material, and visual representation;
- optimistic interaction followed by existing receipt reconciliation;
- refusing or deferring when canonical authority is absent or mismatched.

The same compiled descriptor may be evaluated by multiple clients. Equality is proven by generator/version/region/digest and fixed conformance samples. A C++ evaluator is therefore a deterministic projection of authority, not a second generator.

### 4.3 Required authority envelope

Every published causal-world region carries at least:

```text
world_identity_hash
worldgen_id
worldgen_version
descriptor_schema_version
region_key
event_program_digest
body_catalog_digest
authority_revision
```

If the envelope is missing, unsupported, or digest-invalid, Esoterica must refuse/defer causal geology for that region. It must not fall back to locally invented `CAUSAL_WORLD` bodies. `RANGE` and `TORTURE` remain explicit fixtures, never an invisible authority fallback.

### 4.4 Identity namespaces

Geological features and detached matter are related but not interchangeable:

| Identity | Creation | Lifetime | Example |
|---|---|---|---|
| `GeoFeatureId` | Deterministically derived by canonical worldgen | Stable across queries and residency | formation, pluton, fault, deposit system |
| `MatterBodyId` | Allocated by Fablescript on authoritative separation/place | Stable across representation and reconnect once persistence exists | extracted slab, scoop, placed aggregate |

On extraction, the authoritative receipt creates a `MatterBodyId` whose provenance names the source `GeoFeatureId` chain. Esoterica never turns a feature ID into a matter-body ID by itself.

## 5. Geological domain model

### 5.1 Separate concepts that are currently mixed

The causal model distinguishes:

- **tectonic province:** long-lived crustal setting;
- **geologic province:** region sharing a coherent geological history;
- **formation:** named stratigraphic unit with persistent extent;
- **geological body:** concrete 3D instance such as a bed package, pluton, dike, vein, or lens;
- **structural feature:** fold, fault, shear zone, joint set, or unconformity;
- **depositional environment:** environment at the time a unit formed;
- **present landform:** current ridge, valley, cliff, basin, or plain;
- **surface cover:** regolith, soil, talus, alluvium, or exposed bedrock;
- **deposit system:** causal mineralization event and its host constraints;
- **deposit body:** a persistent ore/mineral body produced by that system.

These become separate identifiers or descriptors rather than values in one `Province` enum.

### 5.2 Stable feature descriptor

Illustrative shape, not a frozen wire schema:

```cpp
struct GeologicalBodyDesc
{
    GeoFeatureId id;
    GeoFeatureId parentId;
    GeoFeatureId formationId;
    GeoFeatureId eventId;

    BodyKind kind;              // bed package, pluton, dike, vein, lens...
    MaterialId material;
    AgeRange age;
    AABB worldBounds;
    ImplicitShapeDesc shape;
    StructuralFrameDesc fabric;

    uint32_t descriptorVersion;
};
```

Descriptors are compact, immutable inputs to queries. They are not fully voxelized matter.

### 5.3 Event program

Overlap is resolved by chronology, not arbitrary blending. A minimal event vocabulary is:

```text
DepositFormation
Lithify
Fold
IntrudeBody
MetamorphoseOrAlter
Fault
Uplift
ErodeToBoundary
Mineralize
DepositSurficialMaterial   [deferred when transport-derived]
```

Each event has a stable ID, age/order key, affected-feature references, deterministic parameters, and bounds. Later events transform or cut earlier bodies according to explicit rules.

Example:

```text
older sandstone and shale deposited
-> beds lithified
-> formation package folded
-> granite pluton intrudes and replaces older host volume
-> contact zone mineralized in valid host geometry
-> fault offsets formations, pluton, and deposit
-> compiled erosion boundary exposes one branch of the deposit
```

The fault does not mint unrelated replacement bodies. Queries on either displaced side retain common ancestry and identify the fault event that separates the branches.

### 5.4 Present-day compilation

The canonical compiler produces a queryable present-day representation from the event program. It may use implicit fields, transform stacks, spatial indexes, clipped volumes, or another deterministic form. The representation must preserve:

- event precedence;
- body and formation identity;
- transformed structural fabric;
- cut/replaced/offset relationships;
- host and alteration relationships;
- present surface intersections;
- stable hashes independent of request order.

The runtime does not replay millions of years per sample.

## 6. Query contracts

### 6.1 Geological sample

The central query answers ancestry as well as material:

```cpp
struct GeologySample
{
    MaterialId material;

    GeoFeatureId tectonicProvinceId;
    GeoFeatureId geologicProvinceId;
    GeoFeatureId formationId;
    GeoFeatureId bodyId;
    GeoFeatureId depositSystemId;
    GeoFeatureId depositBodyId;

    StructuralFrame structure;
    float signedContactDistanceM;
    float weathering01;
    float alteration01;

    AuthorityStamp authority;
    QueryStatus status;
};
```

`QueryStatus` must distinguish at least `Resolved`, `OutsidePublishedRegion`, `AuthorityMissing`, `VersionMismatch`, and `InvalidDescriptor`.

### 6.2 Required queries

The first architecture should expose pure, deterministic queries:

```text
SampleGeology(world_position)
SamplePresentSurface(x, y)
TraceFeature(feature_id, bounds)
EnumerateFeatures(bounds, filter)
ResolveHost(position, deposit_rule)
GetFeatureAncestry(feature_id)
GetAuthorityStamp(region_key)
```

All return identical results for identical authority envelopes regardless of query order, residency order, thread timing, or presentation state.

### 6.3 Structure comes from history

Bedding, foliation, contact normals, fracture tendencies, and deposit orientation derive from body geometry and the deformation history applied to it. Independent noise may perturb small-scale appearance within bounded limits; it may not determine the primary structural frame or overwrite ancestry.

### 6.4 Surface is a reading, not authority

The present surface query returns the boundary produced by compiled history. HF may package that boundary at distance. Local occupancy and D2 may package it near an edit. None of those representations become geological authority.

The locked ownership law remains:

```text
virgin terrain: HF presentation
committed edited region: bounded occupancy -> D2 boundary
canonical geology: published causal history behind both
```

## 7. Latent exact matter

### 7.1 Principle

A known geological body does not require every 12.5 cm element to exist at all times. The body descriptor and query evaluator are sufficient until exact matter becomes relevant.

Wake exact matter only because of:

- player or tool proximity;
- exposure at a visible or interactable surface;
- excavation, fracture, placement, or another authoritative disturbance;
- support, collision, or containment queries requiring local occupancy;
- detachment into a meaningful body;
- explicit gameplay or simulation significance.

Distance alone should not continuously rematerialize stable history. Residency is cached and bounded; settled representation sleeps.

### 7.2 Materialization path

```text
canonical body descriptors
-> bounded local geology sampling
-> canonical virgin occupancy seed
-> authoritative edit receipt
-> local occupancy revision
-> D2/HF ownership handoff
-> extracted or placed matter identity
```

The existing rule still applies: missing occupancy or authority causes refusal/defer, never invented solid, air, floor, deposit, or body identity.

### 7.3 Provenance through transformation

Every separated or transformed material can carry a compact lineage:

```text
world identity
-> tectonic province
-> geologic province
-> formation
-> geological body
-> deposit system/body, if any
-> authoritative source cell/region and revision
-> fracture/extraction event
-> MatterBodyId
-> later aggregate or crafted object
```

Representation may change or sleep. Matter and lineage do not silently change.

## 8. Deposits and host validity

Deposits are causal bodies, not material-colored noise.

A deposit rule declares:

- mineralization event and age;
- permitted host formations/materials;
- required relationship to intrusion, contact, fault, or alteration zone;
- geometric controls and bounds;
- grade or composition field rules;
- parent deposit-system identity;
- exclusions where later events removed or replaced the host.

The first proof uses a contact-controlled mineralized vein or lens associated with the granite intrusion. Every resolved deposit sample must name both its persistent deposit identity and a valid host relationship. A visually similar sample outside valid host geometry must not claim the deposit identity.

Surface exposure and underground continuation are intersections with one deposit body. They are not separately spawned resource markers.

## 9. First proof: `CAUSAL_WORLD`

### 9.1 Purpose

The first proof is deliberately local. It certifies the kernel before planetary breadth, climate, hydrology, ecology, or procedural variety.

### 9.2 Required history

```text
1. Deposit alternating sandstone and shale formation packages.
2. Lithify the packages.
3. Fold both into a readable anticline/syncline structure.
4. Intrude a younger granite body through the folded formations.
5. Generate a contact-controlled mineralized deposit in valid host geology.
6. Fault and offset the formations, granite, and deposit.
7. Apply a fixed compiled present-day erosion boundary.
8. Expose part of the deposit at the surface while retaining underground continuation.
```

Step 7 is a static test boundary in v1. It does not open P5b or imply active erosion/transport.

### 9.3 Required visible/readable facts

The fixture must make these facts queryable and, where practical, visible:

- beds curve continuously through the fold;
- granite cuts older folded beds;
- contact relationships report correct relative age;
- the fault offsets pre-existing bodies rather than painting a line over them;
- the deposit exists only in valid host/contact geometry;
- one deposit branch breaches the present surface;
- digging from the breach follows the same `depositBodyId` underground;
- a displaced branch retains shared deposit-system ancestry and reports the fault transform;
- material outside the deposit but in the same host does not inherit deposit identity;
- extraction produces a distinct authoritative `MatterBodyId` with the deposit lineage.

### 9.4 Proof region and complexity budget

The fixture should use the smallest region that proves all relationships. A suggested catalog is:

- two formation IDs with several body/bed members;
- one fold event;
- one granite pluton;
- one contact alteration zone;
- one deposit system and one deposit body with faulted branches;
- one fault event;
- one static present-surface boundary.

No oceans, aquifers, river network, sediment-routing graph, soil succession, vegetation, or fauna are required.

## 10. Certification gates

A future `--cert-causal-world` should emit a standalone artifact and fail on any mismatch. At minimum it proves:

| Gate | Pass condition |
|---|---|
| Authority envelope | Supported versions and matching descriptor/event/body digests |
| Determinism | Forward, reverse, tiled, and shuffled query order produce identical sample digests |
| Body persistence | Repeated and cross-boundary queries return the same feature IDs |
| Chronology | Granite replaces/cuts older beds; later fault transforms all eligible older bodies |
| Fold structure | Bedding frame follows the folded formation, not world-up or independent noise |
| Host validity | Deposit samples occur only where host/contact constraints pass |
| Surface breach | Surface intersection and underground trace share `depositBodyId` |
| Fault ancestry | Offset branches share required ancestry and cite the fault event |
| Lazy residency | Distant/history-only queries do not allocate occupancy or rebuild D2/HF |
| Materialization | Near disturbance seeds only a bounded local occupancy region |
| Authority refusal | Missing/mismatched descriptors cause defer, never local invention |
| Mutation lineage | Authoritative extraction receipt creates `MatterBodyId` linked to source geological lineage |
| Fixture isolation | Existing `RANGE` and `TORTURE` hashes/certs remain unchanged |
| P5b closure | Cert performs zero water, runoff, river, erosion-transport, or sediment-routing work |

Performance evidence must distinguish descriptor/query cost from instantiated occupancy/D2 cost. Historical body count outside the queried spatial index must not become recurring frame work.

## 11. Implementation sequence

Implementation remains blocked until the authority schema and ownership decision are accepted in both the canonical Python/Fablescript lane and this client lane.

Once accepted, the safest sequence is:

1. Freeze the authority envelope, feature-ID derivation, descriptor schema, and conformance digest format.
2. Add read-only `GeoFeatureId`, ancestry, authority stamp, and `GeologySample` types without changing terrain output.
3. Implement canonical Python/Fablescript compilation for the small `CAUSAL_WORLD` catalog.
4. Implement an Esoterica descriptor loader, validator, spatial index, and pure local evaluator.
5. Add `CAUSAL_WORLD` selection without changing `RANGE` or `TORTURE` behavior.
6. Prove folded formation continuity.
7. Add intrusion precedence and contact relationships.
8. Add fault transforms and shared ancestry across displaced branches.
9. Add the host-valid deposit and surface/underground identity proof.
10. Connect bounded latent-matter materialization to the existing virgin occupancy/HF/D2 path.
11. Extend authoritative extraction receipts with source geological ancestry without changing Fablescript ownership of `MatterBodyId`.
12. Run existing geography, async, authority, residency, stress, pick-fracture, and P5a regressions alongside the new cert.

Do not begin with global plates, a large material catalog, active geomorphology, or ecological consumers. The local proof must close the identity and authority kernel first.

## 12. Explicit deferrals

### 12.1 P5b remains closed

The following are not part of causal-world v1:

- aquifer fill or groundwater flow;
- springs driven by groundwater;
- rainfall/runoff coupling;
- river routing or channel migration;
- water-driven erosion;
- sediment entrainment, transport, sorting, or deposition;
- terrain mutation caused by water;
- coupling causal geology to the live water ledger.

The proof may contain a present-day erosion boundary as immutable compiled geometry. It may not calculate that boundary from live water or transport. The existing P5a ledger remains frozen and independent.

### 12.2 Later world domains

After the geological kernel and later water gates close, the same history can feed:

| Domain | Potential causal inputs |
|---|---|
| Geomorphogenesis | uplift, resistance, fracture, climate, runoff, transport |
| Hydroclimate | topography, permeability, storage, precipitation, temperature |
| Pedogenesis | parent material, weathering, drainage, slope, exposure, time |
| Ecogenesis | soil, water, exposure, disturbance, canopy, succession |
| Fauna | habitat capacity, cover, prey, water, travel structure |
| Human history | resources, terrain, water, access, ruins, extraction, settlement |

These are downstream consumers or later epoch compilers, not reasons to expand v1.

## 13. Non-negotiable invariants

- Canonical geological identity is stable across surface and underground queries.
- Age and event order decide cuts, replacement, deformation, and offsets.
- Primary structure derives from body history, not arbitrary orientation noise.
- Deposits obey host and event relationships.
- Exact matter remains lazy and bounded.
- Missing authority causes refusal/defer.
- Clients do not invent authoritative geology, mutation results, or matter identity.
- HF, occupancy, and D2 remain representations with their existing ownership contract.
- Existing `RANGE` and `TORTURE` fixtures and certs remain intact.
- P5b remains closed.
- Representation may change; material history and provenance do not.

## 14. Acceptance for design completion

This design is ready to move from draft to implementation planning only when:

1. the Python/Fablescript authority owner accepts the descriptor, identity, revision, and digest responsibility;
2. the Esoterica lane accepts its loader/evaluator/cache role and refusal behavior;
3. `GeoFeatureId` versus `MatterBodyId` namespaces and lineage wire fields are agreed;
4. the `CAUSAL_WORLD` fixture and cert are isolated from existing fixture gates;
5. the static erosion-boundary exception is explicitly recognized as non-P5b;
6. no work item requires groundwater, rivers, active erosion, or sediment transport;
7. the first proof is kept to folded formations, granite intrusion, fault, host-valid deposit, breach, and underground continuity.

Until those conditions are accepted, this document authorizes design and interface review only.
