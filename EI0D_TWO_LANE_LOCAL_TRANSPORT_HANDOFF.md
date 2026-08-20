# EI0.D — Two-Lane Local Authoritative Transport Handoff

Status: **transport implementation and focused certification complete**. The
pre-existing MV2.B absolute cold-build timing gate is separately qualified under
Tests; it was not relaxed. EI1 is not opened by this implementation.

## Integration ground truth

```
integration branch
  integration/ei0a-canonical-handshake

clean FableScript ancestry base
  bb6bb3d5452d12f2b9f82b6d67849b4d6e41d19a

historical donor/reference lineage
  contract checkpoint  5a69afb3bc65690041f7c2fab69182df8db3bbda
  donor tip           be29a059c2e31aa74bdb62169592d0539f4e6d7b

EI0.C parent
  f0f69b315aebc7138fa94b76ab19e34b070e54c2

EI0.D engine commit
  4f3447e533327baa0af1c28d49ff391d16f0292c

Provenance architectural reference
  5073baa73905e731eb72c17ae026685b25a9e812

exact client worktree HEAD/base
  f8f3fb6801f861366168099f479461ad5d0f521c
```

The donor branch was inspected, not merged. Its localhost reconnect loop and
interactive-vs-bulk scheduling intent were useful. Its monkey-patched server,
unqualified `v1`, shared/global request assumptions, and prototype authority
state were not imported. The NotSkyBlockProv Unreal client confirmed fragmented
receive assembly, asynchronous bulk construction, and game-thread publication
as proven client concepts; its one-request/one-socket assumptions were not made
canonical.

## Frozen transport profile

```
protocol_id       fablescript.embodied-terrain
protocol_semver   1.0.0
transport_profile fablescript.localhost-two-lane-ndjson/1
framing           utf8-json-lines-lf/1

control default   127.0.0.1:8765
bulk default      127.0.0.1:8766
```

Protocol identity remains independent of transport. Canonical stdio is still
available for engine tests, subprocess tools, and automation. Launching
`python -m client.protocol --tcp-two-lane` selects the two-listener TCP profile;
`--control-port` and `--bulk-port` may override the defaults.

The EngineHello capability block now advertises lane roles, framing, limits,
bulk methods, and that verified shared `.mcp` cache use is preferred rather than
transport-required. No macro page is forced through TCP.

## Lane ownership

Control owns authentication, ping/status/capabilities, small reads, and all
present/future mutations. It is serialized in receive order.

Bulk owns only the advertised bounded safe projections:

```
cell_surface
surface_field
world_snapshot
bulk_test_payload   truth-free EI0.D fairness/pressure fixture
```

`bulk_test_payload` is rejected on control. Mutations are rejected on bulk.
EI0.D does not introduce detailed chunk or macro replacement schemas.

## Session proof and state

Control creates an opaque token. Bulk attachment must echo and then receive the
same:

```
protocol_id / protocol_semver / schema_digest
server_instance_id
session_token
world_uuid
genesis_digest
```

Wrong token, process, world, genesis, protocol, schema, or lane/listener fails
closed. A control disconnect invalidates the token and closes its bulk lanes.
A bulk disconnect leaves control active and permits reattachment. A process
restart naturally invalidates old tokens.

Both engine and client use the explicit vocabulary:

```
DISCONNECTED
CONNECTING
WAITING_FOR_CONTROL_SESSION
HELLO_SENT
AUTHENTICATED
ACTIVE
DRAINING
CLOSED
FAILED
```

Socket-open is never treated as authenticated.

## Framing, correlation, and limits

One strict UTF-8 JSON object is carried per LF-terminated line. Fragmented
frames and multiple frames per receive are normal. Invalid UTF-8/JSON,
non-object envelopes, oversized frames, and trailing partial frames fail closed.

Control and bulk have independent request counters, trackers, buffers, sockets,
and queues. Multiple bulk requests may be outstanding; responses may complete
out of order and are applied only to the exact ID. Unknown, missing, and
duplicate completed response IDs do not dispatch or mutate pending client state.

```
max request frame             65,536 bytes
max control response       1,048,576 bytes
max bulk response          4,194,304 bytes
max bulk cells                 4,096
max bulk extent/axis              256
max bulk in flight                   8
max control queued bytes   2,097,152
max bulk queued bytes      8,388,608
bulk worker count                    4
```

Outbound queues are byte-accounted. Full queues explicitly fail their lane and
record backpressure; responses are never silently dropped. A deterministic
slow-reader test fills the bulk queue, proves its failure is bounded, and then
proves control ping still succeeds.

## Scheduling and snapshot consistency

Control and bulk run on independent listener/connection/writer paths. Authority
capture is exclusive and control-priority: once control is waiting, queued bulk
captures cannot enter first. The truth-free bulk fixture captures only a
revision and performs delay/payload construction outside the authority gate, so
bulk serialization and a slow reader cannot hold gameplay authority.

Safe read projections are captured under the authority gate and copied out;
serialization follows. Bulk responses carry:

```
snapshot_revision
  world_revision   explicit null until initialized
  tick             diagnostic, not a replacement for world revision
```

The client contains an atomic publication primitive for future consumers and
the live Main path builds/validates the bulk EngineHello offside before changing
lane state. EI0.D intentionally publishes no terrain payload from bulk yet, so
existing visible world and prediction behavior do not change.

## Files changed

Engine commit `4f3447e`:

- `client/embodied_handshake.py`
- `client/protocol.py`
- `client/two_lane_transport.py`
- `docs/EMBODIED_TERRAIN_PROTOCOL_V1.md`
- `engine_tests/test_embodied_handshake.py`
- `engine_tests/test_two_lane_transport.py`
- `tools/ei0d_transport_receipt.py`

Provenance worktree additions/edits for EI0.D:

- `Code/Applications/ProvenanceClient/Ei0aHandshake.h`
- `Code/Applications/ProvenanceClient/Ei0dTwoLaneTransport.h`
- `Code/Applications/ProvenanceClient/Main.cpp`
- `Tools/Integration/Ei0dTwoLaneTransportCert.cpp`
- `PROVENANCE_PIN.md`
- this handoff
- `EI0D_PROVENANCE_RECOVERY_MANIFEST.md`

The client worktree also retains the verified EI0.B/C files and 25 page upgrades.
Its Git metadata boundary is still not writable, so no client commit was forced
or emulated. The recovery manifest records exact hashes and exclusions.

## Test receipts

### EI0.D socket and client contract

- Real localhost socket suite: **22/22 PASS**. Covers both handshakes and binding,
  wrong token/world/genesis/process/protocol/schema, listener-role mismatch,
  independent/exact request correlation, out-of-order bulk completion,
  fragmented/multiple/malformed/oversized/trailing framing, request/response
  extent, 8-in-flight cap, slow-reader backpressure, control survival/fairness,
  coherent revision-stamped snapshot, bulk reattach, control invalidation, and
  restart token invalidation.
- EI0.D portable C++ client cert: **PASS**.
- Full `Main.cpp` C++20 compile with `/bigobj`: **PASS**.
- Optimized/LTCG client link: **PASS**.

### Transport performance/fairness receipt

One localhost run with four concurrent delayed 512 KiB bulk responses and 100
control pings:

```
TCP connect                         21.034 ms
control handshake                   0.497 ms
bulk attach                         0.669 ms

control latency under bulk
  min / mean / p50                 0.047 / 0.062 / 0.055 ms
  p95 / p99 / max                  0.087 / 0.165 / 0.195 ms

bulk payload                        2,097,152 bytes / 4 responses
bulk elapsed / throughput           107.484 ms / 18.607 MiB/s

control queue cap / high-water      2,097,152 / 2,051 bytes
bulk queue cap / high-water         8,388,608 / 2,097,796 bytes
backpressure failures               0 control / 0 bulk
```

The separate slow-reader test intentionally causes one bounded bulk
backpressure failure and still receives the control reply.

### Preserved EI0 and world behavior

- FableScript full suite: **PASS** (1,857 tests, 7 skipped) plus demo scene
  **PASS**.
- Canonical stdio/external protocol smoke: **PASS**.
- EI0.A C++ client contract: **PASS**.
- EI0.B Python + C++ schema parity: **PASS**, 100 valid + 12 invalid vectors.
- EI0.C Python corruption matrix + C++ page validation: **PASS**.
- MS1.B appearance: **PASS**; geometry invariant.
- WD1.B appearance: **PASS**; geometry/water truth invariant.
- MV1 coverage: **PASS**.
- No semantic payload change:

```
Genesis digest  17405cbecb97d55aee9e85408e8735c7f055f6ad87369dfa94bee4762c67e994
page corpus     7e0311908b79d5e6715fba03e905501ae79baa1091b7f36d630066b1f5393f10
height payload  2e29df623ed3659825a5d151193bd5601e0ea6910d746651782b13024a0e2826
surface payload 138a37c1341ae4dc9b3869785bda1a3331a3389a60badc8eca346ba42e7231a7
water payload   6a6a92f581211bd4766b11887d89cd540416b4782992b43f32ad4b63f8b83a36
```

### MV2.B timing qualification — gate preserved

All MV2.B semantic/presentation checks pass: 128 km raster, non-repetition,
32 km seam, bounded residency, zero MV2-domain coverage holes, all four cardinal
directions, no refusals, and no fine-authority dependency. Its absolute cold
presentation-build gate is `<50 ms` and was **not** relaxed.

Under the current host load, a paired run produced:

```
untouched EI0.C binary presentation build  54.584 ms  -> timing gate FAIL
EI0.D binary presentation build            55.103 ms  -> timing gate FAIL
paired delta                               +0.519 ms (+0.95%)
```

The previously certified EI0.C receipt is 43.093 ms and remains the last green
absolute receipt. Because the untouched baseline fails in the same environment,
this is not attributed to EI0.D; it remains an open rerun on the normal
performance host rather than a waived or weakened gate.

## Compatibility behavior

- Historical plain `v1` is never silently accepted.
- The canonical two-lane launcher refuses the explicit legacy adapter mode.
- Existing stdio tools remain supported.
- Existing Provenance terrain/projection/mutation consumption is not switched.
- Bulk attach failure degrades only the unavailable bulk lane; it does not make
  client cache/prediction authoritative.

## Deferred dependencies

EI0.D does not define or implement:

- EI1 engine-owned Provenance WorldGenesis or macro replacement/recompile flow;
- EI2 WorldSubstrate/MaterialRegistry migration;
- EI3 revisioned `AuthorityChunk64m` snapshot/delta schemas and reconnect
  negotiation;
- canonical mutation receipts/reconciliation;
- detailed water migration;
- new waterfall, flora, or weather existence authority.

## Board

```
EI0.A  identity + handshake          CERTIFIED
EI0.B  generated schemas             CERTIFIED
EI0.C  fail-closed page validation   CERTIFIED
EI0.D  two-lane transport            CERTIFIED (MV2 absolute host rerun noted)
EI1    WorldGenesis adoption         NEXT
EI2+                                 CLOSED
```

No code was pushed. No historical/client prototype was deleted.
