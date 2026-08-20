# EI1 Provenance integration recovery overlay

This manifest overlays the exact EI1 client state on
`EI0E_PROVENANCE_RECOVERY_MANIFEST.md`. It is not a commit and does not bypass
the external parent repository metadata boundary.

```text
branch: integration/ei0a-canonical-handshake
client HEAD/base: f8f3fb6801f861366168099f479461ad5d0f521c
architectural reference: 5073baa73905e731eb72c17ae026685b25a9e812
engine EI1 commits:
  969831e3d0e5a7f3dfb6b38533e3309a6814cbd0
  033ed892f9a48761bcc54bfc93a73a6ba002e2b5
  b4496afbf9ed8345a9c9c7503b440323aae6ea00
hash: lowercase SHA-256 of exact file bytes
```

Files not listed below retain their EI0.E recovery-manifest hashes. The
corrected 25 Page V2 files are unchanged and continue to use the EI0.E table.

## EI1 overlay files

| File | SHA-256 | Bytes |
|---|---|---:|
| `Code/Applications/ProvenanceClient/Ei0aHandshake.h` | `f0e7ab0a35dc1b5a9032712fad04a02d7d4bafbb1d7c49db1249586b0a4efe06` | 10353 |
| `Code/Applications/ProvenanceClient/Main.cpp` | `b15efdd4df5e171742862f22be6f1be4159705dc5834f286bf3ab4031ecf0151` | 2886547 |
| `Code/Applications/ProvenanceClient/MacroManifestAuthority.h` | `d7f38d2b51ff8a71cd6d5484563cd3f6214831764f509df1ba516a1b21c54953` | 7118 |
| `Data/Worldgen/MacroAuthority/macro_manifest.mcm` | `f9a25effaa0e4ecc9ae6572e47020b203a6851b7d0387f2909f8a035c732a174` | 6372 |
| `Tools/Integration/Ei1WorldGenesisClientCert.cpp` | `a94491de40f50f2438a96b6d6f1d83999ed446b0fe003e9f06fe037432bceb51` | 1463 |
| `Tools/Integration/ei1_worldgenesis_client_cert.py` | `4f2fcf61e2b973a53e17f8fd7410a83d5b0f87baa07046b8daeedb868dae708a` | 2980 |
| `PROVENANCE_PIN.md` | `61ea8e5499c5a808589c2713a0bc926d89abea51047a4d67b298454ec48cfaa0` | 84112 |
| `EI1_WORLDGENESIS_ADOPTION_HANDOFF.md` | `2f6f451619b780969c6cb9104d74b53933b06c460c90b3b38c3b51eeb7e9172f` | 10041 |

## Engine-generated manifest identity

```text
producer_authority: FABLESCRIPT_WORLDGENESIS
genesis_digest: 8394bfefb6955cfffec1c927721d2e6da1b4a24c5525dce4cd238640c2ecd801
manifest_digest: a83cc5aff1fa46e704e653495226f1f7733c4a379a77959a0e280b337a8105cb
pages: 25
local authority fallback: disabled
```

## Deliberately excluded test evidence

Do not stage presentation certificate/capture rewrites as EI1 implementation:

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

Temporary executables/objects, Python bytecode, and MV2 gap/station captures
are excluded and were removed from the integration worktree after inspection.
An authorized normal-user Git session may create scoped client commits only
after verifying this overlay and the inherited EI0.E manifest. Do not stage
with `git add -A`.
