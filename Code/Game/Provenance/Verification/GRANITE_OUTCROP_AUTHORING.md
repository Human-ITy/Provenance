# Granite outcrop artist-authoring round trip

The visible granite surface is now an explicit artist-authored control mesh.
The procedural formula remains in `GraniteOutcropV2.h` only as a documented
reference; the running lab samples `GraniteOutcropAuthoredShape.h`.

## Export

Use the maintained scripts in `Code/Game/Provenance/Verification/Scripts`.
They can be invoked from any working directory and place generated products
in `Build/x64_Debug`. The original Build scripts are retained for compatibility.
From the maintained script directory, run:

```bat
RunGraniteOutcropShapeTool.cmd export GraniteOutcropAuthoring.obj
```

Relative output paths still resolve inside `Build/x64_Debug`. Use an absolute
output path to keep an artist-editable OBJ elsewhere.

The OBJ is a 61 by 25 quad surface in local meters. The front-left outcrop
corner is `(0, 0)`. The exported Z positions are exactly the visible surface
used by the lab; they are not an unnormalized procedural preview.

## Shape

Import `GraniteOutcropAuthoring.obj` into Blender, Nomad Sculpt, or another mesh
editor. In Blender use **File > Import > Wavefront (.obj)**; OBJ is not opened as
a Blender project. Shape the crest, planar faces, shoulders, apron, and joints.
Remeshing, subdividing, decimating, welding, and topology changes are supported.

Keep the object in the same meter scale and orientation, and preserve most of
the 4.20 by 2.40 meter footprint. The baker vertically samples the highest
triangle at each authoritative XY column. It therefore accepts arbitrary
triangle/quad topology and vertex order, but deliberately collapses overhangs
to their highest surface. Columns outside a slightly trimmed sculpt retain the
last passing perimeter rather than creating a hole; at least 65 percent of the
authoritative footprint must be covered by the edited mesh.

This Z-only contract is intentional. The granite remains a single-valued
surface above a connected volumetric root, allowing deterministic collision,
soil exclusion, excavation, and exact volume conservation. A steep near-plane
can be authored, but not an overhang.

## Bake and certify

Save the edited OBJ over `Build/x64_Debug/GraniteOutcropAuthoring.obj`, then run:

```bat
ApplyGraniteOutcropAuthoring.cmd
```

You can also pass a different OBJ path, including a Nomad export:

```bat
ApplyGraniteOutcropAuthoring.cmd "C:\path\to\edited-outcrop.obj"
```

The command first bakes `GraniteOutcropCandidate.h` and runs the optimized and
Debug outcrop certificates against that candidate. The currently running shape
is replaced only after both certificates pass. The certificate rejects an open
surface, an invalid crest/root, oversized hidden partitions, loss of distinct
soil-separated granite islands, or failure of exact excavation conservation.

The visible mesh is never rescaled after baking. If its volume differs, the
generator reconciles the predetermined 9.000 m3 / 24,300 kg by changing only
the buried root depth. Therefore the face and crest seen in the mesh editor
remain where the artist put them.
