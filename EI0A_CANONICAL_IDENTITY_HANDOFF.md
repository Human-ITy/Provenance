# EI0.A — Canonical Identity + Handshake Handoff

Status: **CERTIFIED**. This closes EI0.A only. It does not open EI0.B, EI0.C,
EI0.D, EI1, or any new world-authority stage.

## Integration ground truth

| Role | Branch / commit |
|---|---|
| Clean integration branch (both repositories) | `integration/ei0a-canonical-handshake` |
| FableScript clean base | `bb6bb3d5452d12f2b9f82b6d67849b4d6e41d19a` |
| Historical donor contract checkpoint | `5a69afb3bc65690041f7c2fab69182df8db3bbda` |
| Historical donor tip | `be29a059c2e31aa74bdb62169592d0539f4e6d7b` |
| Provenance integration base | `5073baa73905e731eb72c17ae026685b25a9e812` |
| FableScript EI0.A implementation | `6fbc1856dc74c336895026dd4432f37f3d504bcc` |
| Provenance EI0.A implementation | `4cc674c9a97f72d8c8cbb4d7cdbaf1c16a731494` |

The donor lineage was inspected and used as a reference. It was not merged or
adopted wholesale.

## Authority impact

No world truth moved in this cut. FableScript now has the canonical identities
and handshake gate, while Provenance remains the behavioral/parity oracle for
its current terrain, material, water, streaming, and presentation systems.

Unchanged by EI0.A:

- macro/MV/MW generator output and `.mcp` semantic payloads;
- SurfaceState and WaterState semantics;
- detailed occupancy, mutations, water, and provenance behavior;
- MS1/WD1 appearance, rendering, residency, LOD, and performance behavior.

## Canonical identities

ProtocolIdentity is exactly:

```
protocol_id     fablescript.embodied-terrain
protocol_semver 1.0.0
schema_digest   daa0858a8bf5aca01fea53bea46cae07f9a082672dc107af7ede2b820f0c5544
```

GenesisIdentity is the locked ordered semantic tuple:

```
identity_schema_version
seed
coordinate_frame_id
generator_family
generator_version
generator_build_digest
generator_config_digest
material_registry_id
material_registry_digest
surface_grammar_id / surface_grammar_version
water_grammar_id / water_grammar_version
```

Its canonical compact UTF-8 JSON bytes are hashed with SHA-256. The material
registry digest is taken from the canonical sorted serialization of the actual
FableScript default material definitions. Representation schemas, transport,
save schema, and runtime revisions are excluded from genesis.

WorldInstanceIdentity is `{world_uuid, genesis_digest}`. Canonical identity is
stored sparsely in the existing engine save. Worlds that have not entered the
canonical path keep their old golden save bytes unchanged. A loaded canonical
world restores the same UUID; two newly created worlds from the same genesis
receive different UUIDs.

The full normative contract is in FableScript
`fablescript/docs/EMBODIED_TERRAIN_PROTOCOL_V1.md`.

## Handshake layouts

ClientHello carries exact request correlation, ProtocolIdentity, schema support,
client build, requested authority/projection modes, lane role, and an optional
second-lane session token.

EngineHello returns exact request correlation, ProtocolIdentity, engine build,
authority mode, world/genesis identity, generator/material/surface/water
semantic identity, supported methods/projections, lane role, session token, and
the server/world/genesis binding. `world_revision` is emitted only when it has
actually been initialized.

The current authority mode is exactly `local_server_authoritative`.

## Correlation and session behavior

- FableScript requires ClientHello `request_id` to exactly equal its envelope
  ID and rejects duplicate completed hello IDs.
- Provenance keeps a request-ID keyed table. Response dispatch uses the matched
  request record, not a global pending kind.
- Missing, unknown, and duplicate completed response IDs are rejected before
  pending authoritative state is cleared or dispatched.
- Out-of-order responses map to their own request records.
- A control hello creates the session token. A bulk hello can attach only to the
  same engine-process context, world UUID, and genesis digest. Foreign tokens
  fail with `session_mismatch`.
- An engine with no loaded canonical world returns explicit `no_world`.

EI0.A establishes two-lane identity only. It does not implement EI0.D TCP lane
scheduling, priorities, reconnect behavior, or bulk transport.

## Compatibility behavior

Historical plain `"v1"` and terrain `"v1"` are not implicitly compatible and
cannot call canonical authority methods before hello. The old Phase-74 surface
is retained only as the explicitly selected adapter:

```
fablescript.phase74.v1-compat
```

There is no silent downgrade. Provenance now sends canonical hello before its
existing `terrain_caps` path. That historical path is deliberately not retired;
its migration is later integration work.

## Files changed

FableScript:

- `fablescript/engine/canonical_identity.py`
- `fablescript/client/embodied_handshake.py`
- `fablescript/client/protocol.py`
- `fablescript/client/protocol_smoke.py`
- `fablescript/engine/engine.py`
- `fablescript/engine_tests/test_canonical_identity.py`
- `fablescript/engine_tests/test_embodied_handshake.py`
- `fablescript/engine_tests/test_external_protocol.py`
- `fablescript/docs/EMBODIED_TERRAIN_PROTOCOL_V1.md`
- `fablescript/docs/PHASE_74_EXTERNAL_CLIENT_BRIDGE_PROTOCOL.md`

Provenance:

- `Code/Applications/ProvenanceClient/Ei0aHandshake.h`
- `Code/Applications/ProvenanceClient/Main.cpp`
- `Tools/Integration/Ei0aClientContractCert.cpp`

## Certification receipts

FableScript full regression:

```
python run_engine_tests.py
Ran 1828 tests in 2.579s
OK (skipped=7)
demo scene PASS
OVERALL PASS
```

The EI0.A-focused Python set covers canonical success, rejection of both legacy
`v1` forms, protocol/semver/schema mismatch, genesis determinism and semantic
changes, world-instance separation, UUID save/reload persistence, wrong and
duplicate request IDs, same/wrong two-lane sessions, malformed identity, and
explicit no-world behavior.

Provenance client contract certificate:

```
EI0.A CLIENT CONTRACT PASS
```

Its 11 deterministic rows include exact/out-of-order request dispatch, unknown
and duplicate response rejection, survival of pending state after a wrong ID,
canonical EngineHello parsing, wrong response-ID rejection, and refusal of the
historical protocol ID. The harness includes only the handshake header and does
not initialize terrain, mutation, residency, rendering, or simulation.

`Main.cpp` also compiles cleanly with MSVC 14.51 using C++20 and `/bigobj`.
Pre-existing source-encoding/comment warnings remain; EI0.A introduced no build
error. A solution-level MSBuild attempt was blocked by the machine's duplicate
`PATH`/`Path` environment issue before compiler launch, so the direct translation
unit compile is the recorded client build receipt.

## Dependencies still closed

- **EI0.B — generated schemas:** replace the EI0.A handshake-only schema digest
  with one-source generated cross-language contract artifacts; define
  SurfaceState/WaterState encodings without changing their semantics.
- **EI0.C — fail-closed page validation:** bind pages to genesis and validate
  coordinates, descriptor schemas, digests, and lineage. Not implemented here.
- **EI0.D — two-lane transport:** instantiate control/bulk TCP lanes, priorities,
  reconnect, and bulk behavior using the session identity already defined.
- **EI1+:** WorldGenesis adoption, substrate/materials, revisioned projections,
  mutations, and detailed water migration remain closed.

## Board

```
EI0.A  identity + handshake          CERTIFIED
EI0.B  generated schemas             NEXT
EI0.C  fail-closed page validation   CLOSED
EI0.D  two-lane transport            CLOSED
EI1+                                 CLOSED

WD1.C  waterfall existence           HOLD
FL1    flora population              HOLD
AT1    weather authority             HOLD
```
