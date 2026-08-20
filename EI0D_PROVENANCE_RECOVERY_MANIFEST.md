# EI0.A-D Provenance integration recovery manifest

This manifest protects the exact intended client integration state while the
client worktree's Git metadata remains outside the writable boundary. It is not
a commit and does not bypass that boundary.

```
branch: integration/ei0a-canonical-handshake
client HEAD/base: f8f3fb6801f861366168099f479461ad5d0f521c
architectural Provenance reference: 5073baa73905e731eb72c17ae026685b25a9e812
engine EI0.D commit: 4f3447e533327baa0af1c28d49ff391d16f0292c
hash: lowercase SHA-256 of exact file bytes
```

## Intended integration files

| File | SHA-256 | Bytes |
|---|---|---:|
| `Code/Applications/ProvenanceClient/Ei0aHandshake.h` | `46b1d236ccac98cd57d589db7799f0f861c95e6724081735f45f3f49e33fc61c` | 9700 |
| `Code/Applications/ProvenanceClient/Ei0dTwoLaneTransport.h` | `01e6e1a22558970afd0b5b766c4ac5ff05dba18731df3ba1e1aa557b70656eff` | 7633 |
| `Code/Applications/ProvenanceClient/Main.cpp` | `abef41432cc1b690d14cc28627e6447ce3bc7f6ad56f512fc076ee43e9b3591c` | 2884210 |
| `Code/Applications/ProvenanceClient/MacroPageAuthority.h` | `80056e7da0a76f6fba582ec4358d1d45a378ccffe23f8e28cd6142df7da5ea29` | 19260 |
| `Code/Applications/ProvenanceClient/Ms1SurfaceAppearance.h` | `b074d8c7c3354a021f1ad4ac29a45ac840785ca0b48993c7a76a6a40cd1e7ee8` | 15396 |
| `Code/Applications/ProvenanceClient/WorldDescriptors.generated.h` | `2bcd5df7228b57f636e2b4bd68f099c54776317a5278a22b7f2bf5ba8f5c8776` | 12932 |
| `Docs/WORLD_DESCRIPTORS_GENERATED.md` | `0b9828dae528292ca42405190096ec93daf9793435144bc38493b4b6e83ee6e5` | 8889 |
| `EI0B_GENERATED_DESCRIPTOR_SCHEMAS_HANDOFF.md` | `e80bfcd38d9d194a2574b5cdde0020df015eb90d8d75e28efe8ae0a8b7ec5d15` | 7991 |
| `EI0C_FAIL_CLOSED_MACRO_PAGE_VALIDATION_HANDOFF.md` | `a06c6515d21cededa7e9e6131b57dfb17a87e9273fd0aaf56cbdc4b8486b4fc5` | 8402 |
| `PROVENANCE_PIN.md` | `281bc9d4e4b6d9225ce1ad051576fc57e41f571e0c09edb8e0052b64507ca382` | 82135 |
| `Tools/Integration/Ei0bDescriptorSchemaCert.cpp` | `7669f9bd6b400ec83789daf5641d425fb81b585bf19e5f15508300df6a873dce` | 7763 |
| `Tools/Integration/Ei0cMacroPageValidationCert.cpp` | `32116928682c4529b815214ca91f508c36a80f380f392b9a593e174b495a83da` | 4352 |
| `Tools/Integration/Ei0dTwoLaneTransportCert.cpp` | `96090cd6a2c4fb7a7bcf605090ac0948b210c8be0e90750a6e302eb965d41a28` | 4868 |
| `Tools/Integration/ei0b_schema_cert.py` | `a5045b38a46055ba2ef884da0607fdd0a5d851d31b66d9ed677067c0291cc521` | 5132 |
| `Tools/Integration/ei0c_page_validation_cert.py` | `21e6884454b7cb638dff0f83ad303125dd1204ca4b79e9374aff0b9ed3a62dc0` | 9870 |
| `Tools/Integration/world_descriptor_vectors.jsonl` | `e14e767824f0b8c7068c9bf582f4c3fbe9124c47da01a2edcd8515e7b2a330f8` | 24317 |
| `Tools/Worldgen/generated_world_descriptors.py` | `212d796dc39812a7d4795089c339c18612756498c8df5e239a74ea1a39a04648` | 10981 |
| `Tools/Worldgen/macro_authority.py` | `b19d7d43d66c6ed361065a186dbc5209772197bc82e33d5268ebcf2b900fd7e0` | 101335 |
| `Tools/Worldgen/macro_page_contract.py` | `0ec9741d299d7bc73f04e152e381df3d4a0ba17d634939528ce2e5fd7ad76c0b` | 18287 |
| `Tools/Worldgen/upgrade_macro_pages_ei0c.py` | `14fffa4352e70c055e17b88b9be0d23aa0c4d12b36e5db2265c2c70280d23f39` | 1089 |

The 25 lexicographically sorted
`Data/Worldgen/MacroAuthority/page_*.mcp` files have concatenated-byte SHA-256:

```
7e0311908b79d5e6715fba03e905501ae79baa1091b7f36d630066b1f5393f10
```

Their semantic payload aggregates remain:

```
height  2e29df623ed3659825a5d151193bd5601e0ea6910d746651782b13024a0e2826
surface 138a37c1341ae4dc9b3869785bda1a3331a3389a60badc8eca346ba42e7231a7
water   6a6a92f581211bd4766b11887d89cd540416b4782992b43f32ad4b63f8b83a36
```

## Deliberately excluded

The following pre-existing/generated evidence files are not part of an EI0
commit or recovery set:

```
Docs/provenance_ms1b_surface_appearance_cert.txt
Docs/provenance_wd1b_water_appearance_cert.txt
Docs/provenance_mv1_rotation_cull.txt
Docs/provenance_mv2b_horizon_cert.txt
Docs/provenance_worldgen_playtest_summary.txt
Docs/provenance_worldgen_playtest_trace.csv
```

Build outputs, PPM captures, Python bytecode, and temporary executables/objects
are also excluded. The 26 PPM/bytecode files produced by the EI0.D regression
rerun were removed after inspection.

An authorized normal-user Git session can turn this exact state into scoped
commits by verifying these hashes first and then staging only the intended paths.
Do not stage with `git add -A`.
