# Provenance: From Horizon to Hand Build Contract v1

**Status:** Esoterica implementation program (same posture as Geography Gen)  
**Date:** 8 August 2026  
**Code:** `Code/Applications/ProvenanceClient/HorizonToHand.h` + `Main.cpp` wiring  
**Capability:** `horizon_to_hand_v1`

## Decision

Esoterica owns engine + client for this lane (like `esoterica_geography_v1`). One continuous matter path:

```
world seed / geography
  → material form (fabric, failure, loose, grip)
  → tool contact (envelope ≠ transfer volume)
  → persistent fracture patch
  → complementary separation (parent + plate + fines)
  → MatterBody / AggregatePatch
  → gravity + grip at contact
```

There is no conversion into an unrelated item universe. Same material identity and integer grams.

## Gate 1 scale (live)

- Canonical voxel: 0.125³ m = 1.953125 L  
- Bare-hand **transfer** limit: 1/8 voxel-volume ≈ **244.14 mL**  
- Hand **contact** radius: existing scoop envelope (~3.88 cm) — separate from transfer  
- Mica-schist freeze: **5469 g / full voxel** (~2800 kg/m³)

## First proof (play)

1. Hotbar **3** (pick) on `mica_schist` wall  
2. LMB: early strikes → `H2H crack_seam` / `plate_held` + fines to hand; cracks persist  
3. Later strike severs attachment → `slab_release` → plate MatterBody falls  
4. **G** grips at ray contact (edge vs center = different lever); **G** again drops  
5. Digest shows parent→plate+fines reconcile OK  

## Forms

sand (granular, no rigid plate), clay (plastic), shale (fissile), mica_schist (foliated) + hosts.

## Not yet (later cuts)

Bucket admission collision, save/load body ledger, void/cave support, sand/clay/shale diagnostic goldens, Unreal bridge parity.
