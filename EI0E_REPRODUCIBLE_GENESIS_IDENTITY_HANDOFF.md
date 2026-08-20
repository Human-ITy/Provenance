# EI0.E Reproducible Generator / Genesis Identity — Handoff

Status: **CERTIFIED locally** on 2026-08-20. This is an identity correction for
the existing generator-v5 macro semantics. It is not a generator migration and
does not open EI1 implementation.

## Integration identity

- Branch: `integration/ei0a-canonical-handshake`
- FableScript EI0.E base: `4f3447e533327baa0af1c28d49ff391d16f0292c`
- FableScript EI0.E commit:
  `7bc272af14edb727e68495c32b0a5ec8c75f5964`
- Provenance verified integration base:
  `f8f3fb6801f861366168099f479461ad5d0f521c`
- Provenance architectural reference:
  `5073baa73905e731eb72c17ae026685b25a9e812`
- Historical donor references only:
  `5a69afb3bc65690041f7c2fab69182df8db3bbda` and
  `be29a059c2e31aa74bdb62169592d0539f4e6d7b`.
- No push was performed.

The Provenance EI0.A–E integration remains the verified working-tree state on
the dedicated integration worktree. Its external parent Git metadata boundary
was not bypassed. The original `C:\Users\D-Day\ProvenanceEsoterica` worktree
was not edited.

## Defect and ruling

EI0.C introduced `generator_build_digest=afa225c1...a8b72a` as an opaque
literal. No canonical source bytes or regeneration algorithm existed. The
associated config and material-registry digests were likewise opaque. EI1
correctly stopped rather than copying those literals across the authority
boundary.

The human ruling selected correction rather than grandfathering. The old
identity remains recorded evidence in
`fablescript/schemas/provenance_macro_legacy_identity_supersession.json` with
status `superseded_identity_defect`; it is not authority-capable.

## Canonical semantic generator manifest

Engine source:

`fablescript/schemas/provenance_macro_generator_manifest.json`

Algorithm:

```text
semantic source/configuration
        -> canonical logical component projections
        -> canonical sorted JSON (UTF-8, no whitespace)
        -> SHA-256
        -> generator_build_digest
        -> canonical ordered GenesisIdentity JSON
        -> SHA-256
        -> genesis_digest
```

No absolute path, import location, checkout path, timestamp, mtime, Git
worktree location, transport, page schema, protocol schema, or descriptor bit
layout participates.

Manifest components:

| Logical component | Role | Canonical content digest |
|---|---|---|
| `provenance.frozen-central-program` | Frozen central authority and anchor configuration | `bd88947d297f3c231f94e5213ec7da67d03853a6882f56bcfb184f33b285ab1a` |
| `provenance.macro-authority.semantic-python` | Absolute-coordinate morphology, drainage, surface, water, and static ancestry laws | `531c99073cf85f6be471b8a2c690e38f888eec1fa84e0572647658185d9b324d` |
| `provenance.world-descriptor-semantics` | SurfaceState and WaterState semantic grammar | `434d982800284920220a9f0dc42340955dae3420230fa79c642daab7cc026bf6` |

The Python component is a deterministic AST projection with comments,
formatting, imports, local path discovery, Page V2 serializer/grid packaging,
and descriptor packing/unpacking excluded. The descriptor projection retains
semantic enum names, semantic fields, grammar IDs/versions, semantic-only
fields, and semantic constraints; it excludes protocol identity, codes,
offsets, widths, storage layout, and encoding versions.

The complete semantic dependency audit found exactly these runtime inputs:

- current `macro_authority.py` semantic laws;
- the frozen central `.cmp` key/value program consumed by `CentralProgram`;
- generated SurfaceState/WaterState semantic vocabulary consumed by the
  oracle, projected from the engine-owned descriptor source.

`macro_page_contract.py`, `.mcp` record order/encoding, generated C++ binding,
client presentation, shaders, renderer, transport, and caches do not affect
untouched point-query truth and are excluded.

## Corrected identity

| Identity | Old EI0.C evidence | Corrected EI0.E authority |
|---|---|---|
| generator build | `afa225c1bada4642a3ba6dd79948870e396293a5dd8c5595b1e8c10257a8b72a` | `5d7c1636de50e62dabbae43233e7fc4abdb6d16bc06e79874a92f0010f697d08` |
| generator config | `f5ad774512951d272fe507700dd4702ac3094f253c75596936a92e1d852cf8a0` | `bd88947d297f3c231f94e5213ec7da67d03853a6882f56bcfb184f33b285ab1a` |
| material registry | `e33bf4530caf681caed2d8a0bebe5cd886fd38d4954a480fa4d458149f825c76` | `f095c59862b3856a3c71761ea0e8d9b56e24a08f4530a37e589713ba8ed3eaaf` |
| genesis | `17405cbecb97d55aee9e85408e8735c7f055f6ad87369dfa94bee4762c67e994` | `8394bfefb6955cfffec1c927721d2e6da1b4a24c5525dce4cd238640c2ecd801` |

The identity-base JSON intentionally omits all three derived digest fields.
FableScript derives them from the pinned manifest before normal canonical
GenesisIdentity validation. The client JSON/header values are generated
consumer products and are checked against the engine derivation.

## Static geographic ID audit

The current macro oracle never consumes `genesis_digest`; the term is absent
from the complete semantic source. Current IDs are FNV-1a64 over the world seed
plus canonical absolute feature facts:

- province / MacroFeature ancestry: seed + 200 km absolute province cell;
- MacroLandformId: seed + form namespace + absolute anchor lattice cell;
- ParentRangeId: seed + `range` + absolute range anchor cell;
- MacroPeakId: seed + range anchor + deterministic summit tag;
- MacroWatershedId: seed + absolute routed sink coordinate;
- MacroChannelId: seed + absolute downstream-most trunk channel coordinate;
- MacroWaterBodyId: seed + canonical watershed identity;
- WaterRegimeId: seed + body class + channel-or-watershed parent.

These remain byte-exact. Page coordinate, cache order, supertile compile order,
process launch, client, `world_uuid`, and old/new genesis digest are not inputs.
The current oracle does not expose a separate `FormationId`; EI1 must not invent
or rename that future contract while adopting this existing baseline.

No static-ID derivation change was required, so no human ruling was triggered.

## Page V2 reissue and semantic parity

All 25 pages were reissued with corrected identity fields and recomputed
whole-page digests. The unchanged producer then rebuilt all 25 pages in memory
from cold drainage caches and matched the checked-in corrected pages byte for
byte (`176.890 s`).

Exact old/new semantic aggregates:

| Payload | SHA-256 before and after |
|---|---|
| height | `2e29df623ed3659825a5d151193bd5601e0ea6910d746651782b13024a0e2826` |
| SurfaceState | `138a37c1341ae4dc9b3869785bda1a3331a3389a60badc8eca346ba42e7231a7` |
| WaterState | `6a6a92f581211bd4766b11887d89cd540416b4782992b43f32ad4b63f8b83a36` |

Region, eight-neighbor, and 384 km parent lineage are unchanged. Source,
surface, and water sub-digests are unchanged. Identity headers and whole-page
digests change as required. Corrected corpus SHA-256:

`0ef9933c8a50f494ec005c9dd88cd775960bd987dbcb2ccbc453e9591621cc0d`

EI0.C behavior in both Python and C++:

```text
corrected page + corrected authority session -> VALID_AUTHORITY
old opaque identity + canonical mode         -> WRONG_GENESIS / QUARANTINED
old opaque identity + explicit diagnostic    -> WRONG_GENESIS / DIAGNOSTIC_ONLY
```

No silent downgrade or client fallback exists.

## Persistence impact

The repository and integration worktree contain no canonical persistent player
save or world-instance record referencing the old macro genesis. Matches are
limited to the 25 pre-correction page evidence, historical handoffs/pin, client
legacy diagnostic constants, and the supersession record. Engine save/reload
tests use temporary curated-scenario fixtures and do not persist this identity.

The correction therefore occurs pre-production and before baseline-plus-delta
world persistence. No player history migration is required.

## Files changed

FableScript commit `7bc272af14edb727e68495c32b0a5ec8c75f5964`:

- `worldgen/provenance_macro_identity.py`
- `schemas/provenance_macro_generator_manifest.json`
- `schemas/provenance_macro_genesis_identity.json`
- `schemas/provenance_macro_legacy_identity_supersession.json`
- `tools/ei0e_generator_identity.py`
- `engine/canonical_identity.py`
- `engine_tests/test_provenance_macro_identity.py`
- `engine_tests/test_canonical_identity.py`
- `docs/EMBODIED_TERRAIN_PROTOCOL_V1.md`

Provenance EI0.E overlay:

- `Tools/Worldgen/provenance_macro_identity.generated.json`
- `Tools/Worldgen/macro_page_contract.py`
- `Code/Applications/ProvenanceClient/MacroPageAuthority.h`
- `Tools/Integration/ei0c_page_validation_cert.py`
- `Tools/Integration/Ei0cMacroPageValidationCert.cpp`
- `Tools/Integration/ei0e_reproducible_genesis_cert.py`
- all 25 `Data/Worldgen/MacroAuthority/*.mcp` identity headers/digests
- `PROVENANCE_PIN.md`, this handoff, and the EI0.E recovery overlay.

The macro oracle, renderer, geometry, material/water appearance, detailed
terrain/water, transport, and mutation code are unchanged.

## Certification receipts

- Canonical manifest reproduces from the actual oracle: PASS.
- Two independent source locations: byte-identical manifest/build/genesis.
- Semantic source mutation: build and genesis change.
- Protocol, descriptor encoding, Page V2 serializer/comment, and checkout-path
  changes: build and genesis unchanged.
- Static-ID audit: no genesis dependency; required seed/absolute namespaces
  present; unchanged source hash.
- EI0.E client/engine identity and 25-page certificate: PASS.
- EI0.C Python: 25 pages, 20 corruption cases, 361 deferred cells, PASS.
- EI0.C C++: 105,625 heights, 7,225 surfaces, 7,225 water states, PASS;
  validation 40.726 ms total / 1.629 ms per page.
- Producer rebuild: 25/25 byte-exact, 176.890 s cold.
- FableScript full regression: 1,863 tests, 7 skipped, demo/save-load PASS.
- EI0.D socket receipt: PASS; 18.621 MiB/s bulk, control p99 0.247 ms under
  saturation, zero backpressure failures.
- Client EI0.A correlation/handshake: PASS.
- EI0.B Python/C++ schema vectors and page corpus: PASS.
- Full Provenance `Main.cpp` C++20 `/bigobj` optimized/LTCG compile+link: PASS
  (only pre-existing comment/code-page warnings).
- MS1.B: PASS; geometry untouched.
- WD1.B: PASS; geometry/water truth untouched.
- MV1 yaw/coverage: PASS; zero holes and stable authority/residency digests.
- MV2.B: PASS; 25 unique pages, 5.265 m seam, zero far holes, four cardinal
  directions; presentation build 43.995 ms under unchanged 50 ms gate.

## EI1 restart conditions

EI0.E closes the identity blocker. EI1 may now adopt the unchanged macro
semantic implementation behind FableScript WorldGenesis using corrected
genesis `8394bfef...`, with the Provenance implementation retained as parity
oracle. EI1 must still perform engine ownership, direct-query, cache/manifest,
client authority-source cutover, full seed/morphology/drainage/static-ID parity,
and presentation proof. None of that migration is claimed here.

```text
EI0.A  identity + handshake          CERTIFIED
EI0.B  generated schemas             CERTIFIED locally
EI0.C  page validation               CERTIFIED locally
EI0.D  two-lane transport            CERTIFIED locally
EI0.E  reproducible genesis identity CERTIFIED locally

EI1    WorldGenesis adoption         NEXT
EI2+                                 CLOSED
```

WD1.C existence, FL1 population, and AT1 authority remain on hold.
