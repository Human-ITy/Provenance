# EI2 Provenance layered-identity client handoff

Status: **CERTIFIED locally** on 2026-08-20. This cut teaches the existing
Provenance process to prove the new layered untouched-world identity. It does
not make the client authoritative for substrate, switch a terrain consumer,
change a mesh/material/water resolver, or publish detailed engine projection.

> EI2.R supersedes only this cut's toolchain-sensitive macro/world-baseline
> digest values. The layered contract and client behavior remain intact. See
> `EI2R_TOOLCHAIN_INDEPENDENT_BASELINE_IDENTITY_CLIENT_HANDOFF.md`.

## Git ground truth

- Branch: `integration/ei2-world-baseline-client`
- Provenance base: `1c07ac022337774fe5e316b3ee9e20d0e8d65c9a`
- Earlier architectural reference:
  `5073baa73905e731eb72c17ae026685b25a9e812`
- FableScript integration branch: `integration/ei2-world-baseline`
- FableScript identity commit:
  `a75d91a` (`EI2 layer macro and physical baseline identities`)
- FableScript WorldSubstrate commit:
  `741b6f9` (`EI2 add engine-owned macro-to-micro WorldSubstrate`)
- Historical donor commits are reference-only:
  `5a69afb3bc65690041f7c2fab69182df8db3bbda`,
  `be29a059c2e31aa74bdb62169592d0539f4e6d7b`, and
  `ac6aff38a3e16bb119d84eedeadbc5ce513173d1`.
- No push was performed.

## Client contract change

Protocol identity is now:

```
protocol_id      fablescript.embodied-terrain
protocol_semver  1.1.0
schema_digest    1219f83ca0241bfb807ee6a58678e84a1a7c90aa29d3ba03beef04d34e2b31fd
```

The independent descriptor representation digest remains
`8857189f2dfb5fe3ba73d470ca653248188a0717469963d64d6d5003bad6dac5`.
The client no longer treats those two schemas as one identity.

EngineHello must contain and the client preserves:

```
world_uuid
macro_genesis_digest
world_baseline_digest
baseline_components[]
descriptor_schema_digest
```

Baseline components are non-empty, have unique roles, are canonically ordered,
and carry lowercase SHA-256 semantic digests. The old ambiguous handshake field
`genesis_digest` is not accepted as a substitute. Page V2 and MacroManifest
retain their historical `genesis_digest` field name because it means the macro
digest and their bytes are unchanged.

Control and bulk lanes now bind to the exact same server instance, session
token, world UUID, macro genesis, and world baseline. A mismatched physical
baseline is `session_mismatch`. The Main client state keeps macro and baseline
digests separate; only the macro digest enters the unchanged macro-page cache
context. The descriptor digest, not the protocol digest, enters Page V2
validation.

## Files changed

- `Code/Applications/ProvenanceClient/Ei0aHandshake.h`
- `Code/Applications/ProvenanceClient/Ei0dTwoLaneTransport.h`
- `Code/Applications/ProvenanceClient/Main.cpp`
- `Code/Applications/ProvenanceClient/MacroManifestAuthority.h`
- `Tools/Integration/Ei0aClientContractCert.cpp`
- `Tools/Integration/Ei0cMacroPageValidationCert.cpp`
- `Tools/Integration/Ei0dTwoLaneTransportCert.cpp`
- `Tools/Integration/Ei2WorldBaselineClientCert.cpp`
- this handoff and `PROVENANCE_PIN.md`

The manifest reader also now accepts either LF or CRLF on its magic line; its
content digest and fail-closed page-byte binding are unchanged.

## Receipts

- EI0.A C++ correlation/handshake certificate: PASS.
- EI0.D C++ two-lane certificate: PASS.
- EI2 layered client identity certificate: PASS, including old single-layer
  rejection and wrong-baseline lane rejection.
- EI0.C C++ Page V2 validation: PASS, 25 pages / 105,625 heights / 7,225
  SurfaceState / 7,225 WaterState; macro digest unchanged.
- EI1 C++ manifest/page binding: PASS against the canonical cache bytes,
  25 pages, local authority fallback disabled.
- Full `ProvenanceClient` Release x64 MSVC v145 compile/link: PASS. Warnings are
  the existing float-conversion, code-page, unsafe-CRT, and comment warnings.

Temporary objects, executables, build outputs, and the read-only dependency
junction used for the full build were removed.

## Board

```
EI2   layered identity + WorldSubstrate client proof   CERTIFIED
EI3   revisioned detailed projection                   NEXT
EI4   authoritative mutations                          CLOSED
EI5   detailed water migration                         CLOSED
```

The Provenance prototype remains the parity oracle. No prototype truth system
is deleted, and no new waterfall/flora/weather authority is opened.
