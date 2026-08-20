# EI2.R Provenance toolchain-independent identity handoff

Status: **CERTIFIED locally** on 2026-08-20.

## Git ground truth

- Client branch: `integration/ei2r-toolchain-independent-identity-client`
- Client base: `9ea0a80715b0943137cee9b8de95059699e50230`
- Engine branch: `integration/ei2r-toolchain-independent-identity`
- Engine base: `2be57991fc9c85720d5938210d1ddaa300daef2e`
- Historical donor remains reference-only at
  `be29a059c2e31aa74bdb62169592d0539f4e6d7b`.

## Client correction

The canonical engine identity is now:

```
generator_build_digest  62346ed9f1bc4b094211130902241269fe18f2e9d3511c851ad9ec6e4395d5fb
macro_genesis_digest    9dca0db5344baf0cf709dd654fb80fce38589f0f7fcf01ed8fe086d74818d711
world_baseline_digest   b979a8df68f97adeed64fd2acad70b9ecb1158b7898b8d0ac04bbe773a33b0b4
```

The protocol remains `fablescript.embodied-terrain` 1.1.0. Protocol schema,
descriptor encoding schema, authority modes, lane framing, rendering, and
runtime behavior did not change.

The Python and C++ admission paths fail closed on both the opaque EI0.C
identity and the toolchain-sensitive EI0.E/EI2 identity. Explicit diagnostic
mode can inspect them but cannot publish them as authority.

## Page and manifest reissue

The 25 canonical `.mcp` files and `macro_manifest.mcm` were reissued under the
new identity. The manifest digest is
`62879b4e8bec18c7403c04136623dd79f7317a551bed285aa5fb1a95effb4cd5`.
Only identity envelopes and derived whole-page/content/manifest digests changed.

Semantic aggregates remain byte-exact:

```
height  2e29df623ed3659825a5d151193bd5601e0ea6910d746651782b13024a0e2826
surface 138a37c1341ae4dc9b3869785bda1a3331a3389a60badc8eca346ba42e7231a7
water   6a6a92f581211bd4766b11887d89cd540416b4782992b43f32ad4b63f8b83a36
```

The reissued page corpus SHA-256 is
`a216c5d7fedc68a989dc05a65c714d08be246f28946ceaf4a2443e92a611cd96`.

## Files changed

- `Tools/Worldgen/provenance_macro_identity.generated.json`
- `Tools/Worldgen/macro_page_contract.py`
- `Code/Applications/ProvenanceClient/MacroPageAuthority.h`
- `Tools/Integration/ei0e_reproducible_genesis_cert.py`
- `Data/Worldgen/MacroAuthority/page_*.mcp` (25 identity reissues)
- `Data/Worldgen/MacroAuthority/macro_manifest.mcm`
- this handoff, `EI2_WORLD_BASELINE_CLIENT_HANDOFF.md`, and `PROVENANCE_PIN.md`

## Receipts

- Engine/client semantic manifest relocation parity: PASS.
- Python 3.12/3.14 canonical identity and macro/micro samples: byte-exact PASS.
- Provenance identity, page, payload, and superseded-identity cert: PASS.
- All six C++ contract certificates: PASS. EI0.C admitted 25 pages, 105,625
  heights, 7,225 SurfaceState, and 7,225 WaterState records and rejected both
  superseded identities.
- ProvenanceClient Release x64 MSVC v145 compile/link: PASS. Warnings are the
  existing float-conversion, code-page, comment, and unsafe-CRT warnings.

## Board

```
EI2.R toolchain-independent baseline identity  CERTIFIED
EI3 revisioned detailed projection             NEXT
EI4+                                           CLOSED
```

No generator, terrain, SurfaceState, WaterState, material, detailed physics,
mutation, rendering, or presentation-performance law changed. EI3 was not
opened.
