# EI0.B Generated World Descriptor Schemas — Handoff

Status: **CERTIFIED locally** on 2026-08-20. EI0.B changes representation
plumbing only. No world-generation, terrain, material, water, appearance,
rendering, residency, or performance law moved or changed.

## Integration identity

- Branch: `integration/ei0a-canonical-handshake`
- FableScript EI0.B base: `b74f20ee216df310c99a414a3f3330a439fdedab`
- FableScript EI0.B implementation: `17899ad3f6c8ecf611206348d61f01cfedf2ebfe`
- Provenance EI0.B base/reference: `f8f3fb6801f861366168099f479461ad5d0f521c`
- Historical donor references only: `5a69afb3bc65690041f7c2fab69182df8db3bbda`,
  `be29a059c2e31aa74bdb62169592d0539f4e6d7b`
- Original Provenance architectural reference: `5073baa73905e731eb72c17ae026685b25a9e812`
- No push was performed.

The engine commit is recorded above. The Provenance changes remain a scoped
working-tree change on the integration worktree because this execution context
could not acquire the external parent repository's worktree metadata lock.
The original dirty `C:\Users\D-Day\ProvenanceEsoterica` worktree was not edited.

## Canonical contract

Source schema:

`fablescript/schemas/world_descriptors.json`

Generator:

`fablescript/schema_tools/generate_world_descriptors.py`

Protocol compatibility schema digest:

`8857189f2dfb5fe3ba73d470ca653248188a0717469963d64d6d5003bad6dac5`

The digest covers the protocol ID, protocol semver, EI0.A handshake schema
identity, and canonical sorted JSON bytes of the descriptor schema. It does
not participate in GenesisIdentity.

| Contract | Encoding | Grammar |
|---|---|---|
| SurfaceState | `provenance.surface-state.packed` v1 | `provenance.surface-state.ms1a` `ms1a.1` |
| WaterState | `provenance.water-state.packed` v1 | `provenance.water-state.wd1a` `wd1a.1` |

Changing an encoding while preserving decoded semantic values does not change
genesis. Changing either grammar version is a semantic genesis change. The
pre/post EI0.B default scenario genesis digest is exactly:

`30a0e9ed0e257412607083f50bbc221e432ecd40f7736f86b010c61c442ae709`

## Generated products

Engine-owned products:

- `fablescript/generated/world_descriptors.py`
- `fablescript/generated/WorldDescriptors.generated.h`
- `fablescript/generated/world_descriptor_vectors.jsonl`
- `fablescript/docs/WORLD_DESCRIPTORS_GENERATED.md`

Provenance consumers/copies:

- `Tools/Worldgen/generated_world_descriptors.py`
- `Code/Applications/ProvenanceClient/WorldDescriptors.generated.h`
- `Tools/Integration/world_descriptor_vectors.jsonl`
- `Docs/WORLD_DESCRIPTORS_GENERATED.md`

Non-generated integration/test/documentation files changed:

- FableScript: `client/embodied_handshake.py`,
  `docs/EMBODIED_TERRAIN_PROTOCOL_V1.md`,
  `engine_tests/test_canonical_identity.py`, and
  `engine_tests/test_world_descriptor_schema.py`.
- Provenance: `Tools/Worldgen/macro_authority.py`,
  `Code/Applications/ProvenanceClient/Ei0aHandshake.h`,
  `Code/Applications/ProvenanceClient/Ms1SurfaceAppearance.h`,
  `Code/Applications/ProvenanceClient/Main.cpp`,
  `Tools/Integration/ei0b_schema_cert.py`,
  `Tools/Integration/Ei0bDescriptorSchemaCert.cpp`, `PROVENANCE_PIN.md`, and
  this handoff.

Every generated file carries the source schema, generator version, digest where
applicable, and a DO-NOT-EDIT marker. `--check` fails on stale output.

## Ratified layouts

SurfaceState v1 retains the certified 38 meaningful bits: substrate (4),
lithology (3), dominant family (3), then seven 4-bit half-up/clipped axes for
wetness, weathering, soil depth, stability, organic potential, exposure, and
roughness. Bits 38–63 are reserved and rejected.

WaterState v1 retains the certified 59 meaningful bits: presence (3), causal
body/accommodation class (4), flow (3), bottom family (3), depth in quarter
metres (9, half-even/clipped), eight 4-bit half-up/clipped scalar axes, two
2-bit half-even/clipped potentials, and authority scope (1). Bits 59–63 are
reserved and rejected.

`VALID_MACRO=0`; `DEFER_TO_DETAILED=1`. A deferred wire value retains the
historical dry/none/none zero placeholders, but generated semantic decoders
expose `presence=None` / `WP_UNSPECIFIED` and `has_presence_conclusion=false`.
It therefore never semantically decodes as DRY.

The committed corpus also ratifies the existing drainage law: a valid dry or
damp state may retain a causal lake/wetland accommodation class. Body type does
not prove water presence. Present water still requires a non-none body.

Fields intentionally not added to encoding v1 are documented in the generated
layout: surface `source_rev`; water continuous surface/bottom elevation,
separate mean supply/persistence margin, macro ancestry IDs, regime ID, and
source revision. Integer packing has no structurally meaningful truncation
case. WaterAuthority exhausts its one-bit wire domain; out-of-domain authority
inputs are rejected before packing.

## Integration behavior

- `macro_authority.py` now takes enum tables, versions, packing, decoding,
  quantization, and validation from the generated Python binding.
- `Ms1SurfaceAppearance.h` maps generated decoded wire records into its
  presentation-only runtime structs. Its appearance resolvers are unchanged.
- `.mcp` parsing passes explicit SurfaceState/WaterState encoding versions to
  generated decoders. Unknown descriptor versions fail closed; EI0.C still owns
  page quarantine, lineage, world/genesis, coordinate, and digest validation.
- EI0.A EngineHello and Provenance ClientHello now use the real generated schema
  digest instead of the EI0.A placeholder.

Manual code intentionally remaining:

- the certified semantic derivation/dataclasses in `macro_authority.py` (the
  Provenance parity oracle until EI1);
- presentation-only `SurfaceState`/`WaterState` views in
  `Ms1SurfaceAppearance.h`;
- current `.mcp` text/header parsing in `Main.cpp` (EI0.C scope).

No stable enum number, bit offset, width, or quantization rule remains manually
duplicated in those consumer paths.

## Certification receipts

- Generated files deterministic/current: PASS; second render byte-identical and
  generator `--check` current.
- Python contract/identity/handshake focus: PASS, 24 tests.
- Shared golden corpus: PASS, 100 valid vectors and 12 invalid vectors in both
  Python and C++.
- C++ encode/decode parity: PASS, including explicit rounding boundaries,
  clipping, reserved values, illegal combinations, unknown versions, and
  reserved authority input.
- Committed `.mcp` corpus parity: PASS, 25 pages, 7,225 SurfaceState values,
  7,225 WaterState values, 361 deferred values with no presence conclusion.
- Corpus SHA-256: `d65a52c5e444c66b7fc7ce5b345995c8546b62933d5b2254337a4e0052bc5222`.
- FableScript regression: PASS, 1,834 tests, 7 skipped, demo scene PASS.
- Provenance full `Main.cpp` translation-unit compile: PASS (MSVC `/bigobj`,
  pre-existing code-page/comment warnings only).
- EI0.A C++ client contract: PASS.
- Provenance `--cert-ms1b`: PASS, exit 0.
- Provenance `--cert-wd1b`: PASS, exit 0.
- World/page generator outputs: unchanged; committed `Data/Worldgen/MacroAuthority`
  has no EI0.B diff.

The expensive macro-authority regeneration cert was not rerun because it would
rewrite authority/evidence for an encoding-only cut. The full committed corpus
was checked instead, alongside the normal MS1.B and WD1.B executable certs.

## Remaining dependencies

EI0.C must add full fail-closed page trust: protocol/world/genesis binding,
absolute coordinate/schema identity, source/surface/water/page digest
recomputation, lineage, quarantine, and authoritative replacement behavior.
EI0.D remains responsible for actual two-lane TCP transport and scheduling.
No EI1 or new world-authority program is open.

## Board

```text
EI0.A  identity + handshake          CERTIFIED
EI0.B  generated schemas             CERTIFIED
EI0.C  fail-closed page validation   NEXT
EI0.D  two-lane transport            CLOSED
EI1+                                 CLOSED
```
