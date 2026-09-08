# Detached granite material anchoring — 2026-09-07

## Report and cause

The user observed chips changing mineral pattern and dirt staining with elevation,
giving a transparent/sliding-texture impression. The granite shader sampled live
world position and live world normal for triplanar texture, mineral fields, seeded
soil staining, crease shading, and lichen. Moving a detached chip therefore moved
it through those fields. This was not evidence that its opacity actually changed.

## Correction

- Procedural vertices now forward optional UV1 and packed color through the
  existing static GPU vertex format. Existing callers default both to zero.
  The CPU procedural vertex is larger; the GPU static vertex format is unchanged.
- Chip render vertices carry immutable birth-world XYZ in UV0.xy/UV1.x, an opt-in
  marker in UV1.y, and source normal packed into color RGB (alpha marks validity).
- Both exact clods and small visual proxies attach this payload before pose
  transformation. Moving meshes and world-baked sleeping batches use the same
  source coordinates, including when a chip wakes and regains an individual mesh.
- Granite shader material evaluation uses the source position and source normal
  for opted-in vertices. Lighting/shadow normals and physical world position stay
  current. Payload color alpha does not replace opacity.
- Host rock, gameplay geometry, mass, fracture, soil, and grass rules are unchanged.

This preserves the existing material response at the source; it does not add a
new weathered-exterior versus fresh-interior face classification. Existing small
chip visual-proxy simplification also remains (including its sleep LOD). Source
normal quantization is RGB8, with tested dot agreement greater than .9999 for the
covered normals. Coordinates are float32, not a single baked color per chip.

## Validation

- `RunGraniteMaterialAnchorTest.cmd`: 87 checks, zero failures, optimized and
  Debug. Covers source coordinate precision, normal encoding, pose-independent
  payloads, and affine source interpolation.
- `RunGraniteDebrisPerformanceTest.cmd`: 303 checks, zero failures; 24 chips,
  zero active after settling; measured average 1.847 ms, peak 49.962 ms. This is
  a CPU simulation regression, not a GPU/visual performance certificate.
- Incremental shader reflection/compilation succeeded through the Release
  reflector with `-s Esoterica.slnx -shaders`.
- Shared Debug build succeeded after generated shader compilation and was
  relaunched for user visual review. Live high-chip/stained-base comparison is
  pending; headless tests do not claim pixel-level visual acceptance.
- Follow-up: rechecked the published DLL hashes above, launched a fresh editor,
  and opened Play Map successfully. The pristine outcrop, terrain, grass, and
  placed nature assets render. Automated forward input produced no observable
  movement, so this startup check does not validate detached-chip appearance.
  The live session is left open for the upper-face/stained-base chip comparison.

Published SHA256:

- Engine.Runtime: `35BAA8B2B4C6863DE1654E9E69C74E6E2702528DB13CA0D9BF346B81CB77DBB8`
- Game.Runtime: `A3D57ED4DD1C0361454974D7C8E77F25006A96E71B98BF6EE954662B060D3F8F`

The engine rendering files contain pre-existing unrelated changes. This record
describes only the optional UV1/color forwarding and chip anchoring changes;
Git's full-file diff must not be attributed to this task.
