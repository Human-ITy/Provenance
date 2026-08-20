# EI3 revisioned detailed projection handoff

Status: **CERTIFIED locally**. This handoff closes EI3 only. EI4 is next; EI5+
and all new world-authority programs remain closed.

## Integration ground truth

- Engine branch: `integration/ei3-revisioned-detailed-projection`
- Engine base: `dbad61662b6f68a43010b45db0c668b05d11287b`
- Engine implementation: `e624d14`
- Client branch: `integration/ei3-revisioned-detailed-projection-client`
- Client base: `7fa70056006e6c8cb0d35e9e838da31bfcb8c0d7`
- Client implementation: `57b818ac`
- Historical donor checkpoint: `5a69afb3bc65690041f7c2fab69182df8db3bbda`
- Historical donor tip: `be29a059c2e31aa74bdb62169592d0539f4e6d7b`
- Original Provenance architectural reference: `5073baa73905e731eb72c17ae026685b25a9e812`

No donor lineage was merged wholesale. EI3 builds on the promoted EI2.R heads.

## Delivered contract

The engine now projects revisioned 64 m detailed chunks from its shared
WorldSubstrate over the canonical bulk lane. Projection schema identity is
`fablescript.detailed-chunk-projection` version 1, profile
`surface-strata-8m-v1`. Every snapshot proves world UUID, macro genesis,
WorldBaselineIdentity, absolute chunk identity/bounds, world/chunk revisions,
baseline digest, exact sampled occupancy/strata/material facts, authority status
for bodies/water, and a canonical SHA-256 checksum.

The client rejects wrong binding, malformed lattice, checksum tampering, stale
or noncontiguous revisions, and wrong delta bases/results before publication.
It retains immutable snapshots in a bounded cache and requests them using the
locked P0-P5 priority grammar. Walk/landing requires admitted detailed support;
outside admitted detail, existing macro/coarse presentation remains client-owned.

Reconnect submits revision/checksum claims only. The engine returns unchanged,
an exactly contiguous retained delta, or a complete replacement snapshot.
Client memory is never uploaded as truth. EI3 exposes no gameplay mutation
endpoint and does not rename prediction as authority.

## Files changed

Engine:

- `fablescript/worldgen/detailed_projection.py`
- `fablescript/worldgen/world_substrate.py`
- `fablescript/worldgen/provenance_macro/authority.py`
- `fablescript/worldgen/provenance_macro/projection_service.py`
- `fablescript/client/embodied_handshake.py`
- `fablescript/tools/serve_worldgen_projection.py`
- `fablescript/tools/ei3_projection_fixture.py`
- EI3 tests plus additive EI1 source/mode expectations
- protocol and EI3 contract documents

Client:

- `Code/Applications/ProvenanceClient/Ei3DetailedProjection.h`
- `Code/Applications/ProvenanceClient/Ei0aHandshake.h`
- `Code/Applications/ProvenanceClient/Main.cpp`
- `Tools/Integration/Ei3DetailedProjectionClientCert.cpp`
- this handoff, pin, and the compact traversal text receipt

## Exact receipts

- Engine focused projection suite: 6/6 PASS.
- EI0-EI2/worldgen regression: 68 tests executed; 66 passed immediately. Two
  exact-list assertions correctly detected the new standard-library cache
  dependencies and advertised projection modes; after additive expectation
  updates both reran PASS. No terrain, identity, substrate, transport, or macro
  behavior assertion failed.
- Cross-language fixture: `EI3_CLIENT_CONTRACT=PASS`, including snapshot
  admission, exact delta contiguity, landing P0, course change, and
  24/60/120/240 m/s predictive planning.
- Release x64 client: PASS (only pre-existing warnings).
- Real two-lane socket: control/bulk identity and two authoritative snapshots
  PASS; cached fresh-process snapshot measured 0.369 s and 91,540 bytes.
- Playable predictive traversal: PASS after 1,069.5 m. Reconnect, landing,
  coarse retention, rotation, course/boundary changes, and all speed bands pass.
  Published/rejected = 146/0; resident max = 146/256; requested max = 8;
  final admitted draw = 79 chunks/10,112 triangles; lower-frame sky holes =
  0/574,210 sampled pixels.

The initial all-sky capture was treated as a real failure. Root cause was the
old near renderer evicting its disposable residency while the new authoritative
cache remained populated. The closure adds a client-owned mesh adapter over
immutable engine snapshots; it derives appearance only and changes no truth.

## Unresolved dependencies

- EI4: canonical mutation transactions, persistent world/chunk revision
  production, retained-delta persistence, prediction reconciliation.
- EI5: authoritative detailed water migration. EI3 water remains explicitly
  `deferred_to_ei5`, never inferred dry.
- Optional future representation profiles/compression and generated projection
  schemas. These are representation work, not GenesisIdentity inputs.
- EI0.D transport policy remains unchanged; EI3 only uses its existing two lanes.

## Board

```
EI0.A-E identity/schema/transport        CERTIFIED
EI1 WorldGenesis                         CERTIFIED
EI2/EI2.R WorldSubstrate + identity      CERTIFIED
EI3 revisioned detailed projection       CERTIFIED
EI4 authoritative mutation/reconcile     NEXT
EI5 detailed water migration             CLOSED
EI6 duplicate client truth retirement    CLOSED
WD1.C / FL1 / AT1 authority              HOLD
```
