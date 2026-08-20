# EI0.C Fail-Closed Macro Page Authority Validation — Handoff

Status: **CERTIFIED locally** on 2026-08-20. EI0.C changes page trust and
representation metadata only. Terrain heights, SurfaceState, WaterState,
appearance, residency, LOD, rendering, and world-generation laws are unchanged.

## Integration identity

- Branch: `integration/ei0a-canonical-handshake`
- FableScript EI0.C base: `17899ad3f6c8ecf611206348d61f01cfedf2ebfe`
- Provenance verified integration base: `f8f3fb6801f861366168099f479461ad5d0f521c`
- Historical donor references only: `5a69afb3bc65690041f7c2fab69182df8db3bbda`,
  `be29a059c2e31aa74bdb62169592d0539f4e6d7b`
- Original Provenance architectural reference: `5073baa73905e731eb72c17ae026685b25a9e812`
- FableScript EI0.C identity/contract commit:
  `f0f69b315aebc7138fa94b76ab19e34b070e54c2`.
- Provenance EI0.B/C remains the exact verified working-tree change on this
  integration worktree. A scoped commit was attempted, but the external parent
  repository metadata approval boundary rejected it. No workaround was used.
- No push was performed; the original dirty `C:\Users\D-Day\ProvenanceEsoterica`
  worktree was not edited.

## Authority binding

The existing generator-v5 macro baseline now has this canonical genesis digest:

`17405cbecb97d55aee9e85408e8735c7f055f6ad87369dfa94bee4762c67e994`

FableScript owns the declarative GenesisIdentity at
`schemas/provenance_macro_genesis_identity.json`. This names the already
certified Provenance parity baseline; it does not perform EI1 WorldGenesis
adoption. Macro pages bind to this digest and never carry or validate a
playthrough `world_uuid`. Two world instances sharing the digest may share the
same immutable pages. A different session genesis fails closed.

## Canonical V2 page header

`PROVENANCE_MACRO_AUTHORITY_PAGE_V2` carries:

- `macro_page_schema_version=2`, the complete GenesisIdentity, and
  `genesis_digest`;
- EI0.B `descriptor_schema_digest`, SurfaceState/WaterState encoding versions,
  grammar IDs and grammar versions;
- generator build/config and material registry identity/digests;
- stored coordinate, derived absolute bounds, grid spacing/dimensions, and
  explicit height/surface/water counts;
- page/neighbor region lineage plus a representation-only parent supertile
  coordinate/ID;
- declared digest algorithms and recomputable source, surface, water, and
  whole-page digests.

The parent supertile is the existing 384 km analytic routing supertile. The new
fields expose existing packaging lineage; they create no geographic ancestry or
worldgen law.

## Digest contract and validation order

- Height: FNV-1a64 over signed centimetres, low six bytes little-endian.
- SurfaceState: FNV-1a64 over five little-endian bytes per packed descriptor.
- WaterState: FNV-1a64 over eight little-endian bytes per packed descriptor.
- Whole page: SHA-256 over page magic plus every non-comment `key=value` record
  sorted by key, UTF-8/LF, excluding only `page_digest`. Payload records are
  included directly.

The client parses into an offside candidate, validates page/schema/genesis and
contract identities, coordinate/bounds/grids/counts, generated EI0.B descriptor
constraints, independently recomputed payload digests, region/neighbor/parent
lineage, then the whole-page digest. Only complete success publishes.

## Trust, quarantine, and publication

States are `UNLOADED`, `VALIDATING`, `VALID_AUTHORITY`, `QUARANTINED`, and
`DIAGNOSTIC_ONLY`. Machine failures include malformed/schema/encoding/identity,
wrong genesis/coordinate, invalid ranges/enums, truncation, each digest class,
and lineage mismatch.

Canonical authority mode never consumes a quarantined page. The cache retains
an existing trusted page when a corrupt replacement arrives; a later valid
candidate can atomically replace an empty/quarantined slot. The runtime skips
the affected far region. Wrong genesis invalidates an established authority
session immediately; repeated invalid artifacts escalate the session. EI0.D
will provide the replacement/recompile request lane.

Legacy V1 is never an implicit fallback. The explicit validator diagnostic
mode labels it `DIAGNOSTIC_ONLY`; that state is not authority-capable and is not
accepted by the runtime page cache, interaction, collision, saves, or certs.

## Changed files

Engine:

- `schemas/provenance_macro_genesis_identity.json`
- `engine/canonical_identity.py`
- `engine_tests/test_canonical_identity.py`
- `docs/EMBODIED_TERRAIN_PROTOCOL_V1.md`

Provenance/client:

- `Tools/Worldgen/macro_page_contract.py`
- `Tools/Worldgen/upgrade_macro_pages_ei0c.py`
- `Tools/Worldgen/macro_authority.py`
- `Code/Applications/ProvenanceClient/MacroPageAuthority.h`
- `Code/Applications/ProvenanceClient/Main.cpp`
- `Tools/Integration/ei0c_page_validation_cert.py`
- `Tools/Integration/Ei0cMacroPageValidationCert.cpp`
- all 25 `Data/Worldgen/MacroAuthority/*.mcp` headers
- `PROVENANCE_PIN.md` and this handoff

The verified uncommitted EI0.B files remain part of this integration worktree;
unrelated generated playtest evidence is excluded from scoped commits.

## Certification receipts

- Current corpus: 25/25 `VALID_AUTHORITY`.
- Corruption matrix: 20/20 rejected before authority use.
- C++ runtime validator: 25 pages, 105,625 heights, 7,225 SurfaceState and
  7,225 WaterState descriptors; PASS.
- EI0.B schema digest used by both validators:
  `8857189f2dfb5fe3ba73d470ca653248188a0717469963d64d6d5003bad6dac5`.
- `DEFER_TO_DETAILED`: 361 cells, still distinct from dry.
- Pre/post semantic payload SHA-256 values:
  - heights `2e29df623ed3659825a5d151193bd5601e0ea6910d746651782b13024a0e2826`
  - surfaces `138a37c1341ae4dc9b3869785bda1a3331a3389a60badc8eca346ba42e7231a7`
  - water `6a6a92f581211bd4766b11887d89cd540416b4782992b43f32ad4b63f8b83a36`
- Python validation: 200.724 ms/25 pages (8.029 ms/page) in the initial run.
- C++ validation: 122.385 ms/25 pages (4.895 ms/page) in the initial run.
- Canonical V2 corpus SHA-256:
  `7e0311908b79d5e6715fba03e905501ae79baa1091b7f36d630066b1f5393f10`.
- Full `Main.cpp` translation unit: PASS with `/bigobj`; pre-existing warnings only.
- Release client executable build: PASS (MSVC v143 direct build; project file's
  requested v145 toolset was unavailable in the installed VS 2022 instance).
- Engine focused canonical identity tests: 6/6 PASS.
- Full FableScript regression: 1,835 PASS, 7 skipped; demo/save-load PASS.
- EI0.A client correlation/hello certificate: PASS.
- EI0.B Python/C++ schema certificates: PASS, 100 valid and 12 invalid vectors;
  7,225 surface + 7,225 water values; 361 deferred cells.
- MS1.B presentation: PASS; geometry bit-exact.
- WD1.B presentation: PASS; geometry/water truth bit-exact.
- MV1 fixed-yaw coverage: PASS; 0 coverage holes and stable resident/authority
  digests.
- MV2.B horizon/seams: PASS; 25 unique page digests, 5.265 m 32 km seam,
  0 far coverage holes, bounded residency, all four cardinal directions.
- MV2.B admission/performance split: 78.753 ms cold build total = 35.659 ms
  validation + 43.093 ms presentation build; validation is not per frame.

The older deep `cert_macro_authority.py` all-fixture run was started as an
additional producer check and stopped after more than five minutes without
progress output. It is not counted as a pass. Before it was stopped it had
rewritten the canonical V2 corpus through the normal producer; the independent
25-page validation, exact payload hashes, generated-schema checks, MV1 coverage,
and MV2 seam/horizon gates all passed afterward.

Validation is load/change/session-establishment work. Valid immutable pages
cache their trust; there is no per-frame digest cost. Trust metadata is one enum
pair, detail string, and two 64-character digests per loaded page in the
portable validator record, excluding existing payload storage.

## Remaining dependencies

EI0.D owns actual two-lane transport, replacement/recompile requests, scheduling,
and session re-establishment. EI1 owns moving macro generation behind
FableScript WorldGenesis. Dynamic large-scale consequences remain future
revisioned overlays, never mutation of the immutable baseline page.

```text
EI0.A  identity + handshake          CERTIFIED
EI0.B  generated schemas             CERTIFIED locally
EI0.C  fail-closed page validation   CERTIFIED
EI0.D  two-lane transport            NEXT
EI1+                                 CLOSED
```
