# EI0.E Provenance integration recovery overlay

This manifest overlays the exact EI0.E intended client state on
`EI0D_PROVENANCE_RECOVERY_MANIFEST.md`. It is not a commit and does not bypass
the external parent repository metadata boundary.

```text
branch: integration/ei0a-canonical-handshake
client HEAD/base: f8f3fb6801f861366168099f479461ad5d0f521c
architectural reference: 5073baa73905e731eb72c17ae026685b25a9e812
engine EI0.E commit: 7bc272af14edb727e68495c32b0a5ec8c75f5964
hash: lowercase SHA-256 of exact file bytes
```

Files not listed below retain their EI0.D manifest hashes.

## EI0.E overlay files

| File | SHA-256 | Bytes |
|---|---|---:|
| `Code/Applications/ProvenanceClient/MacroPageAuthority.h` | `a775f53685d7b91976ba008e6b49bdbd92953ca31d733cfe662aa6a97a36a4c0` | 20230 |
| `Tools/Integration/Ei0cMacroPageValidationCert.cpp` | `9c79585e21de27117e375a085603798e209cbd63bd3683c59178fe24386f01ad` | 5702 |
| `Tools/Integration/ei0c_page_validation_cert.py` | `1a3978fd6e39b524e6deaef7e2ab45ec4e7ba06ff78cf5f5d6cce14203f54828` | 11118 |
| `Tools/Integration/ei0e_reproducible_genesis_cert.py` | `59b73c2f843a3c990c8e030a831c39b3381285a541c2844d2dac91b4aa572b6f` | 8382 |
| `Tools/Worldgen/macro_page_contract.py` | `6dec38d9da99dda8c327f08096112a05c273ed1c664d703d6e8868715c5cbdc5` | 20119 |
| `Tools/Worldgen/provenance_macro_identity.generated.json` | `2e0675620b2503c0da501fefc808f57f0fe04c498c0682835e19dac399da1f72` | 834 |
| `PROVENANCE_PIN.md` | `9d14d028595ce3453c0f00d7ca330a3bd8ae23d14c1237f96509ad35c0f4aed2` | 82826 |
| `EI0E_REPRODUCIBLE_GENESIS_IDENTITY_HANDOFF.md` | `f6544d82a1384eb16f069f70005a787a33b04eb69e70aaf9576439e4b430965a` | 11283 |

## Corrected 25-page ring

| File | SHA-256 | Bytes |
|---|---|---:|
| `Data/Worldgen/MacroAuthority/page_-1_-1.mcp` | `a40337f7d071b7a3a01fed3db71f7fd877506bbd384eee971e96c38f291a5d6c` | 42778 |
| `Data/Worldgen/MacroAuthority/page_-1_-2.mcp` | `266a28962c2d18c49e8abbdb8a4d1af30e9f4bd5780e1711acad07023c8c21a7` | 42771 |
| `Data/Worldgen/MacroAuthority/page_-1_0.mcp` | `5719dbbffa5de55a048da664163e89980960d1fcfac2eaee70756a9520e3cdc6` | 44759 |
| `Data/Worldgen/MacroAuthority/page_-1_1.mcp` | `356902c967c09c0862b8b633d4aeb35e887d97b05f7484290a7afbcb33fb8df0` | 47488 |
| `Data/Worldgen/MacroAuthority/page_-1_2.mcp` | `726724f96f703b707f5953214c63ebdecd67f26e9d8210cb37933deed291d6a1` | 45954 |
| `Data/Worldgen/MacroAuthority/page_-2_-1.mcp` | `ba41faa18ac1f30dc11d0a9b7d00c0320b5097bc25697510d49d65e7d13c6bcd` | 44051 |
| `Data/Worldgen/MacroAuthority/page_-2_-2.mcp` | `f912baaab8364761aa24dc9fa43ef3eb62bd8b9a32021f07599499dfba04b5de` | 40944 |
| `Data/Worldgen/MacroAuthority/page_-2_0.mcp` | `6db8b0e61ccfbb11342a98f27db4d60a4af7a6c728178aaced0123698fc748c5` | 44264 |
| `Data/Worldgen/MacroAuthority/page_-2_1.mcp` | `fff3e223d69bc9838316dce9e64f040de9c84c5464f8e3d391edbeb2cfb20d22` | 44949 |
| `Data/Worldgen/MacroAuthority/page_-2_2.mcp` | `0f4607bc5f019d2d1041f3946870959ec3bf039bacde2af98664630eeca2acd0` | 42399 |
| `Data/Worldgen/MacroAuthority/page_0_-1.mcp` | `32c6bffc813dd69d3a2dade3f253e5f8f32a806a6baa85620c5b6e979bbbd372` | 43578 |
| `Data/Worldgen/MacroAuthority/page_0_-2.mcp` | `2b3ef25ce081f7053406a6547f8178dbfe50d9e3660d9efa8c7717359314c395` | 42914 |
| `Data/Worldgen/MacroAuthority/page_0_0.mcp` | `a706e01643fc7158329232b4a5005d0a5f26b8047ff0f6c0fcff928e6e81b391` | 41101 |
| `Data/Worldgen/MacroAuthority/page_0_1.mcp` | `044ba34eebf657733bec835398dbafb5b42001e9438ab077249ba05e4c1e4853` | 43652 |
| `Data/Worldgen/MacroAuthority/page_0_2.mcp` | `5daa2221808e3f8603f1079ab0529af53651b1f69b679b47dc7ead3b9daaf295` | 43880 |
| `Data/Worldgen/MacroAuthority/page_1_-1.mcp` | `cd6726bf7c10386e304dd981bd62af89c18ff88dc2821686ced355591bda1210` | 47770 |
| `Data/Worldgen/MacroAuthority/page_1_-2.mcp` | `70452b3010e335ff7e5ee263ea6d1509d3036cce600038f211e71430c20d57f8` | 43813 |
| `Data/Worldgen/MacroAuthority/page_1_0.mcp` | `1644e0180f3ea4f77428c2fe4c0dd17471cf4514fc50135c48e3dc640594957b` | 46432 |
| `Data/Worldgen/MacroAuthority/page_1_1.mcp` | `b998097d0b4c0edf6bb12d31e68102f4e3e1aa0161924e4dfe7385cef0c21a37` | 48022 |
| `Data/Worldgen/MacroAuthority/page_1_2.mcp` | `63c6309f2a87742a9d3effc65835636594c29dfcfbca6a909d5472d02455d885` | 47717 |
| `Data/Worldgen/MacroAuthority/page_2_-1.mcp` | `bf029c3b84a5b64673771801a20e32ee365ad17761271d4e4fc05c26aa92ebcb` | 45371 |
| `Data/Worldgen/MacroAuthority/page_2_-2.mcp` | `3c03e714fad27315a310108b3d17f9a42bf1033a6907d53338744901eadbf636` | 41239 |
| `Data/Worldgen/MacroAuthority/page_2_0.mcp` | `d041fe74aa7b5084b0e9fe11616476a95cc1bcf8d5e68d08435a326d08593df7` | 48662 |
| `Data/Worldgen/MacroAuthority/page_2_1.mcp` | `af0a24367c224608556c3f8d31c39b403917418863f9086bbb3bb01383e7a513` | 48709 |
| `Data/Worldgen/MacroAuthority/page_2_2.mcp` | `bcb847ae324e926f07f944f72abddb984cb29f23e5ef217528bc633233962296` | 46003 |

Named-corpus SHA-256 (lexicographic filename + CRLF + exact bytes + CRLF):

`0ef9933c8a50f494ec005c9dd88cd775960bd987dbcb2ccbc453e9591621cc0d`

Semantic payload aggregates remain:

```text
height  2e29df623ed3659825a5d151193bd5601e0ea6910d746651782b13024a0e2826
surface 138a37c1341ae4dc9b3869785bda1a3331a3389a60badc8eca346ba42e7231a7
water   6a6a92f581211bd4766b11887d89cd540416b4782992b43f32ad4b63f8b83a36
```

## Deliberately excluded test evidence

Do not stage the existing/generated evidence below as EI0.E implementation:

```text
Docs/provenance_ms1b_surface_appearance_cert.txt
Docs/provenance_wd1b_water_appearance_cert.txt
Docs/provenance_mv1_multi_scale_terrain_cert.txt
Docs/provenance_mv1_rotation_cull.txt
Docs/provenance_mv1_station0_trunk_valley.ppm
Docs/provenance_mv1_station1_mountain_flank.ppm
Docs/provenance_mv1_station2_ridge_shoulder.ppm
Docs/provenance_mv1_station3_foreland_basin.ppm
Docs/provenance_mv1_station4_high_divide.ppm
Docs/provenance_mv2b_horizon_cert.txt
Docs/provenance_worldgen_playtest_summary.txt
Docs/provenance_worldgen_playtest_trace.csv
```

Build products, bytecode, temporary executables/objects, MV2 gap maps, and MV2
station captures are excluded. The 20 untracked MV2 PPMs and Python bytecode
produced by certification were removed after inspection.

An authorized normal-user Git session may create scoped client commits only
after verifying this overlay and the inherited EI0.D manifest. Do not stage
with `git add -A`.
