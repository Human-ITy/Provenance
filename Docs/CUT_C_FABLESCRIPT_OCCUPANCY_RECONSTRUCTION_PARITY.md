# Cut C - FableScript Occupancy to Esoterica Reconstruction Parity

Status: **CERTIFIED**

Canonical Cut-B authority: FableScript commit `ac6aff38a3e16bb119d84eedeadbc5ce513173d1` (read-only)
Scope: one finite Stage-10 integration fixture; Stages 5-10 remain unchanged controls.

## Ruling

Cut C consumes a frozen FableScript-owned 12.5 cm occupancy/material product.
ProvenanceEsoterica does not reproduce the Cut-B fixed-point allocation law and
does not accept `grade` as a geometry input for this fixture.

```text
Cut-A FableScript Stage-10 geology oracle
        -> integer authority anchors
pinned Cut-B provenance_occupancy.py
        -> 12.5 cm fill + material + ancestry columns
fablescript_cut_c_occupancy_v1.cocc
        -> read-only ingestion
one Esoterica boundary used by render + collision + x-ray
```

The compiler imports the exact Cut-B Python module from its isolated worktree,
verifies its pinned commit and clean status, and records the module SHA-256.
The worktree is never written.

## Certified fixture and playable handoff

The artifact contains a guarded 16 m by 16 m sampling footprint and 16 m of
subsurface history: 16,384 authoritative columns and 2,097,152 sampled
occupied voxels. Only one complete 8 m by 8 m package transfers terrain
ownership to Cut C. The guard columns exist for interpolation and cannot be
presented as owned Cut-C terrain.

Outside that package, the existing certified Stage-10 runtime remains the
control world. An 8 m reconstruction collar joins the occupancy package back
to Stage 10. The outer boundary has zero height error. A bounded 1 mm
presentation-package overlap closes fine/coarse raster cracks without changing
surface height, collision, material, ancestry, or occupancy.

The resulting 64 m-radius playable neighborhood certifies 289 complete live
packages (136 m measured coverage) with zero empty packages.

## Permanent gates

The headless certificate proves:

- every stored voxel sample matches Stage-10 material, FeatureId, and body;
- formation, intrusion, deposit-system, deposit-body, events, and chronology match;
- bedding/structure stays within the existing parity tolerance;
- the exposed material is the material of the top occupied voxel;
- all IDs occur in the Cut-A bridge (no client-minted geology);
- granite and quartz contact samples are both present;
- monolithic and tiled reconstruction have the same triangle-set digest;
- the live world has no empty or partial terrain packages;
- the occupancy/control handoff has zero height seam;
- render and collision use the same boundary function;
- cold reload and stage-order transitions do not change the result;
- an owned-package terminal scan resolves buried quartz FeatureId `da10b0d1e5f01001`;
- `grade` is forbidden as a Cut-C geometry input;
- mutation, water, active erosion, faulting, sediment, bodies, and P5b remain closed.

The visual certificate is deliberately split:

- x-ray OFF: the saved traversal frame must contain zero lower-frame sky pixels;
- x-ray ON: the terminal must be buried quartz and the saved frame must visibly
  contain the deposit.

## Player runtime

Run [PLAY_CUT_C.cmd](../PLAY_CUT_C.cmd), or open the ordinary Stage-0 launcher
and choose `INTEGRATION -> Cut C - FableScript Occupancy Parity`.

The playable runtime starts with x-ray OFF and normal controls. Toggle the
geology flashlight in game when wanted. Mouse wheel changes depth,
Shift+wheel changes width, `V` changes color mode, and `F9` captures a receipt.
Cert-only camera positions are never applied to the player launcher.

## Success

> The same terrain matter authored by FableScript at 12.5 cm is the terrain
> ProvenanceEsoterica renders, collides with, and inspects; the client no longer
> needs an independent relief interpretation for the Cut-C fixture.
