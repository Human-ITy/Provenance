# EI1 WorldGenesis Adoption — Handoff

Status: **CERTIFIED locally** on 2026-08-20. EI1 moves the existing certified
macro baseline behind FableScript WorldGenesis without changing a world law,
Page V2 semantic payload, client mesh, material/water appearance, or detailed
terrain/water behavior.

## Integration identity

- Branch: `integration/ei0a-canonical-handshake`
- FableScript EI1 base: `7bc272af14edb727e68495c32b0a5ec8c75f5964`
- FableScript EI1 implementation commit:
  `969831e3d0e5a7f3dfb6b38533e3309a6814cbd0`
- FableScript EI1 projection/session/docs commit:
  `033ed892f9a48761bcc54bfc93a73a6ba002e2b5`
- FableScript EI1 packaged-behavior certification commit:
  `b4496afbf9ed8345a9c9c7503b440323aae6ea00`
- Provenance verified Git base:
  `f8f3fb6801f861366168099f479461ad5d0f521c`
- Provenance architectural reference:
  `5073baa73905e731eb72c17ae026685b25a9e812`
- Historical donor references only:
  `5a69afb3bc65690041f7c2fab69182df8db3bbda` and
  `be29a059c2e31aa74bdb62169592d0539f4e6d7b`.
- Client changes remain a hash-pinned recovery overlay because the external
  parent repository metadata boundary was not bypassed. Exact recovery state:
  `EI1_PROVENANCE_RECOVERY_MANIFEST.md` over
  `EI0E_PROVENANCE_RECOVERY_MANIFEST.md`.
- No push was performed.

## Engine authority now

The canonical implementation lives at:

- `worldgen/provenance_macro/authority.py` — unchanged semantic laws;
- `worldgen/provenance_macro/world_genesis.py` — engine direct-query/compiler facade;
- `worldgen/provenance_macro/page_contract.py` — producer-neutral Page V2;
- `worldgen/provenance_macro/macro_manifest.py` — immutable cache and manifest;
- `worldgen/provenance_macro/projection_service.py` — canonical macro hello,
  manifest service, and bounded Page V2 projection surface;
- `worldgen/provenance_macro/data/causal_world_macro_provinces_floor.cmp` —
  frozen central program.

`WorldGenesis` owns macro height, morphology, drainage, SurfaceState,
WaterState, and the existing static geographic ancestry. One implementation
serves direct queries, Page V2 compilation, cache regeneration, manifest
publication, and certification. The compiler is a consumer, not a second
generator.

Direct API:

```text
query_macro_height(x, y)
query_macro_feature(x, y)
query_surface_state(x, y)
query_water_state(x, y)
query_static_ancestry(x, y)
query_drainage_ancestry(x, y)
compile_macro_page((ri, rj))
serialize_macro_page((ri, rj))
```

No distinct FormationId exists in the adopted baseline; EI1 did not invent or
rename one. Static IDs retain their seed + canonical absolute-fact derivation.

The dedicated projection session reports the corrected macro genesis and only
advertises `macro_manifest` / `macro_page_v2`. It does not relabel the existing
curated detailed-scenario bridge. Control and attached bulk lanes prove one
session/world/genesis, and a focused identity record preserves the macro
service world UUID across restart without persisting derived pages.
The C++ client now has an explicit macro ClientHello builder for those two
projection capabilities; the existing detailed/gameplay hello remains unchanged
until EI2 unifies the macro and substrate authority contexts.

## Canonical identity

The engine package reproduces the pinned EI0.E semantic manifest exactly:

- generator build:
  `5d7c1636de50e62dabbae43233e7fc4abdb6d16bc06e79874a92f0010f697d08`
- generator configuration:
  `bd88947d297f3c231f94e5213ec7da67d03853a6882f56bcfb184f33b285ab1a`
- material registry:
  `f095c59862b3856a3c71761ea0e8d9b56e24a08f4530a37e589713ba8ed3eaaf`
- corrected genesis:
  `8394bfefb6955cfffec1c927721d2e6da1b4a24c5525dce4cd238640c2ecd801`
- macro semantic AST component:
  `531c99073cf85f6be471b8a2c690e38f888eec1fa84e0572647658185d9b324d`

Import paths and central-program newline encoding changed as repository
representation only. The EI0.E canonical semantic AST and parsed central
program are exact. Page/descriptor/cache/transport schemas remain outside
GenesisIdentity.

## MacroManifest and cache contract

Manifest family: `FABLESCRIPT_MACRO_MANIFEST_V1` /
`fablescript.worldgen.macro-manifest`, producer
`FABLESCRIPT_WORLDGENESIS`.

It binds:

- corrected genesis and generator/material identities;
- Macro Page schema 2, SurfaceState encoding 1, WaterState encoding 1;
- descriptor schema digest;
- each absolute page coordinate to whole-page, height, surface, and water
  digests, SHA-256 content key, retrieval path, and payload size.

Manifest digest:

`a83cc5aff1fa46e704e653495226f1f7733c4a379a77959a0e280b337a8105cb`

The manifest carries no world UUID. World instances sharing genesis may share
the immutable cache. The engine cache writer uses SHA-256 paths. The existing
client ring retains its coordinate filenames to avoid a data-layout/performance
change, but every file is content-keyed and digest-bound by the engine manifest.

## Client cutover

Canonical loading now performs:

```text
EngineHello corrected genesis
  -> load/validate FABLESCRIPT_WORLDGENESIS MacroManifest
  -> find exact absolute page binding
  -> verify file size + SHA-256 content key
  -> EI0.C validate Page V2 identity/bounds/descriptors/digests/lineage
  -> verify manifest page/sub-payload digests
  -> publish VALID_AUTHORITY
```

Missing manifest, missing entry, identity/schema mismatch, content mismatch, or
Page V2 rejection fails closed. The client does not call its local generator as
fallback. `--development-parity` in the EI1 client cert compares the retained
oracle and engine semantic projections explicitly; it is not canonical play.

The manifest is cached per genesis/schema session. Its parsing, content-key
check, and EI0.C page validation are all classified as authority admission, not
presentation construction. This preserves the existing MV2.B 50 ms
presentation gate without weakening it.

## Exact parity receipts

Engine-generated 25-page ring versus corrected Provenance oracle:

- Page V2 bytes: **25/25 exact**;
- height aggregate:
  `2e29df623ed3659825a5d151193bd5601e0ea6910d746651782b13024a0e2826`;
- SurfaceState aggregate:
  `138a37c1341ae4dc9b3869785bda1a3331a3389a60badc8eca346ba42e7231a7`;
- WaterState aggregate:
  `6a6a92f581211bd4766b11887d89cd540416b4782992b43f32ad4b63f8b83a36`;
- reverse compile order: **25/25 exact**;
- direct facade corpus: **8/8 exact**, including page/supertile boundaries
  and 250/500/1,000/2,000 km probes;
- static ancestry: exact landform, feature, range, peak, watershed, channel,
  water-body, and regime IDs where present;
- different-seed control diversity: **PASS**;
- cold cache regeneration and reordered publication: **same manifest/pages**;
- superseded EI0 identity: non-canonical.

Measured unchanged algorithm:

- cold 25-page engine compile: 177.716 s;
- warm reverse-order 25-page compile: 108.835 s;
- long-distance direct-query corpus: 164.182 s.

No drainage rewrite or performance-gate relaxation was made.

The complete retained MV2/MV3/MS1.A/WD1.A certificate was then executed
unchanged against the engine-owned module. Every world-behavior assertion
passed, including cross-64 km and cross-384 km drainage continuity,
specialized landforms, peak/range ancestry, SurfaceState, and WaterState. Ring
compilation was 174.140 s under the unchanged 280 s budget. Its six raw FAIL
lines were exclusively the legacy monolith-only `non_stdlib_import` alias
(`no_wrap`, B2/C/MS1.A/WD1.A cheap-source labels, and their aggregate), because
the promoted package imports EI0.B descriptors and Page V2/identity contracts.
The EI1 port records that distinction explicitly and passes only after an exact
package-import allowlist plus forbidden fine-authority symbol audit succeeds.
Result: `EI1_PACKAGE_AWARE_MACRO_BEHAVIOR PASS`; behavior thresholds unchanged,
forbidden fine-authority names absent.

## Regression receipts

- EI1 focused engine suites PASS; the WorldGenesis module is 8/8 and includes
  the package dependency / forbidden-fine-authority audit.
- Full FableScript engine at certification commit: 1,871 tests PASS, 7 skipped;
  demo scene PASS.
- FableScript language: 124 tests PASS; examples PASS.
- EI0.C Python: 25 pages, 20 corruption cases, 361 deferred cells PASS.
- EI0.C C++: 105,625 heights, 7,225 surfaces, 7,225 water states PASS;
  41.675 ms total / 1.667 ms per page.
- EI1 client Python: 25 manifest/page bindings PASS; development parity PASS;
  local authority fallback disabled.
- EI1 client C++: 25 manifest/page bindings PASS.
- Canonical macro projection session: corrected EngineHello, persisted world
  UUID, same-session bulk attach, manifest retrieval, and exact 41,101-byte
  center Page V2 retrieval PASS.
- Full Provenance C++20 `/GL /LTCG /bigobj` compile/link PASS (only existing
  code-page/comment warnings).
- MS1.B PASS; WD1.B PASS; MV1 PASS.
- MV2.B PASS: 25 unique pages, 5.265 m seam, zero far holes, four cardinal
  directions, presentation build 45.786 ms under unchanged 50 ms gate.

## Historical micro authority and scope

FableScript already owns substantial hand-scale terrain/gameplay behavior, and
NotSkyBlockProv previously proved an engine-authoritative near-field client.
EI1 extends engine ownership outward; it does not replace the hand with the
horizon.

Detailed occupancy, mutation, detailed water, persistence, and macro-to-micro
refinement are unchanged. `DEFER_TO_DETAILED` remains distinct from dry. Macro
pages remain replaceable derived cache and are not admitted to save history.

## Remaining oracle and EI2 dependencies

The Provenance oracle remains at `Tools/Worldgen/macro_authority.py` and is not
deleted. EI2 must define the refinement from macro geology/surface promises to
FableScript detailed strata, mixed materials, and 12.5 cm matter. It must also
ratify FormationId and the macro/detailed authority relation before any duplicate
client truth is retired.

```text
EI0.A-E  CERTIFIED
EI1      WorldGenesis adoption  CERTIFIED
EI2      macro -> micro         NEXT
EI3+                            CLOSED
WD1.C / FL1 / AT1 authority    HOLD
```
