# P5 side-gate — Material-True Pick Fracture + Closed Local Surface

**Status:** side-gate checkpoint (not P5b).  
**P5a:** FREEZE at water ledger / settle floor — do not modify `WaterLedger`.  
**P5b:** CLOSED — no terrain–water coupling.

## Law

```
impact → penetration → pry → material fracture → connected piece releases
→ occupancy loses exactly that material → remaining boundary reconstructed
→ detached piece = same separation event (not unrelated debris)
```

Pick is **not** sphere/cup/cell deform. Tool supplies force/leverage; material structure
determines fracture; Fablescript owns authoritative transfer (P4); HF/D2 reconstruct one
continuous boundary from occupancy truth. **Neither HF nor D2 chooses fracture shape.**

## Contact frame (angle-independent)

At hit:

| Axis | Definition |
|------|------------|
| `N` | Surface normal (outward) |
| `T` | Strike/pry projected onto the tangent plane |
| `B` | `cross(N, T)` |

Tip geometry, penetration, pry sweep, and fracture envelope are defined in `(T,B,N)`.
World-up/Z is never the fracture law. Same physical strike on flat / slope / vertical /
overhang = same local event rotated to world.

## Material families

| Family | Morphology | Notes |
|--------|------------|-------|
| `gravel` (aggregate) | `aggregate_recess` | Small central crush recess; no rigid plate |
| `granite` (compact angular) | `compact_angular` | Tip-scale angular wedge |
| `mica_schist` (foliation-biased) | `foliation_plate` | Elongated along foliation∩tangent; thin across layers |

Deterministic seed variation inside each family.

## Four distinct radii

| Radius | Role |
|--------|------|
| Contact / query | Aim hit only (`kLivePickContactRM`) |
| Fracture volume | Physical material removal (tip-scale, ≤14 cm extent) |
| D2 recon halo | Local cavity rebuild around fracture AABB |
| HF refine | Mouth / refine footprint — not fracture law |

Outside physical changed region + min recon halo → pre-strike surface **bit-identical**.
No meter-scale folds from 5–10 cm bites. Canonical HF↔D2 mouth seam.

## Red gates

`FRACTURE_OUTSIDE_ALLOWED_REGION` · `HF_CHANGED_OUTSIDE_RECON_HALO` ·
`HF_RETIRED_WITHOUT_REPLACEMENT` · `PRESENTATION_COVERAGE_FAIL` ·
`UNEXPLAINED_OPEN_BOUNDARY` · `NONCANONICAL_HF_D2_SEAM` ·
`STRETCHED/DEGENERATE_TRIANGLE` · `INVERTED_WINDING` · `INVALID_NORMAL` ·
`ROTATION_EQUIVALENCE_FAIL` · `PREVIOUS_STRIKE_MUTATED_WITHOUT_CAUSE` ·
`STRIKE_REMOUNTED_TO_VIRGIN_HF` · `MATERIAL_MORPHOLOGY_FAIL`

Capture failing coordinates; do not relax geometry to greenwash.

## Cert

```
--cert-pick-fracture
--cert-p5-pick
```

Artifact: `%TEMP%\provenance_pick_fracture_cert.txt`  
Committed mirror: `Docs/provenance_pick_fracture_cert.txt`

Headless suite covers:

1. Contact-frame orthonormality (flat / slope / vertical / overhang)
2. Radius separation
3. Material morphology families
4. Rotational equivalence 0°…90° for gravel / granite / mica_schist
5. Two-strike gravel regression (interior remount, prior-strike identity, tip-scale)
6. Outside recon-halo identity
7. No world-up fracture law (vertical ≡ flat local morph)

## Code

- `PickFracture.h` — contact frame, envelopes, headless cert
- `Main.cpp` — `CarveOccupancyFracture`, `TryPickFoliatedStrike` wire, cert tick

## Performance

Preserve P4.5: cost follows causal activity; no global HF refine; no continuous D2 for history.
