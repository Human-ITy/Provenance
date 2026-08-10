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

## Hard presentation gates (no exceptions)

1. **Sky / clear in or around a shallow strike hole = FAIL.**
   Any sample (framebuffer or classifier) inside or around the action neighborhood whose
   color matches the clear/sky background (`glClear` ≈ RGB(114,158,224), or the legacy
   blue-dominant sky/backdrop classifiers) is **`PRESENTATION_COVERAGE_FAIL`**.  
   Background is valid only when the removed volume actually connects the viewing ray
   through the solid to exterior air. A shallow bite must be closed by its real cavity
   wall/floor; generated plugs and screen-space concealment are not valid coverage.

2. **Deform outside intended strike volume = FAIL.**  
   After a strike, any HF/D2 deformation beyond the physical fracture volume plus the
   **minimum recon halo** is **`HF_CHANGED_OUTSIDE_RECON_HALO`** (fold explosions included).  
   Outside that envelope the pre-strike surface must stay bit-identical.

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

## Material response

| Family | Morphology | Notes |
|--------|------------|-------|
| `gravel` (aggregate) | `aggregate_recess` | Small central crush recess; no rigid plate |
| `granite` (compact angular) | `compact_angular` | Tip-scale angular wedge |
| `mica_schist` (foliation-biased) | `foliation_plate` | Elongated along foliation∩tangent; thin across layers |

Additional production morphology classes are `bedding_plate` (sandstone/shale),
`cohesive_shear` (dirt/clay), `crystalline_break`, and `ore_matrix_break`.
Deterministic seed variation operates inside the material response. Cohesion, penetration
resistance, wedging, and crushing alter the tool-scale dimensions even when two materials
share a broad fabric family.

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

The quick certificate covers:

1. Contact-frame orthonormality (flat / slope / vertical / overhang)
2. Radius separation
3. Material morphology families
4. Rotational equivalence 0°…90° for gravel / granite / mica_schist
5. Two-strike gravel regression (interior remount, prior-strike identity, tip-scale)
6. Outside recon-halo identity (tight span + envelope)
7. No world-up fracture law (vertical ≡ flat local morph)
8. Presentation coverage closed (mouth samples have floor/wall cover — sky/clear never excused)
9. Outside-strike-volume deform clamp (halo span ≤ fracture AABB + 2×min recon halo)

10. A 3,500-case deterministic strike matrix: 19 host materials, seven surface angles
    (0 through 90 degrees), five impacts, four pry azimuths, and three structural
    orientations for anisotropic materials
11. An exact 8 cm half-apple surface: field residual, unchanged exterior, interior
    watertightness, and dense ray coverage
12. Ray-acquired mesh contact at HF angles 0 through 75 degrees plus a rotated D2 wall
    at 90 degrees
13. Progressive excavation where strike two raycasts the reconstructed strike-one cavity
    (8 cm interior contact in the reference case), never the virgin HF
14. Adjacent strike separation/merge according to physical envelope overlap
15. A renderer-backed gate built from the live 4.9 cm grass/shovel event path (not a
    certificate-only rock fixture), with zero sky-classified pixels in the strike view
16. A second live strike acquired from the reconstructed first cavity, proving positive
    depth progression, one continuous analytic patch, and zero coarse-D2 rebuilds
17. A connected U-chain load gate proving reconstruction is the sparse union of local
    tool envelopes, never the aggregate bounding rectangle; expansion is capped at
    1.5 reconstruction cells beyond each physical envelope
18. A 24-strike progressive column whose nearest acquired floor advances monotonically,
    plus a renderer-backed 12-strike live column with nearest-surface depth ordering,
    zero sky pixels, and a thirteenth strike acquired from the published deep floor
19. A player-scale arched stone shelter (`0.90 m` wide, `1.90 m` high, `1.20 m` deep)
    composed of 18,330 overlapping `4.9 cm` half-apple strikes. The full 6 ft player
    capsule is sampled throughout; solid rock must remain behind and beside the cut.
    Pinhole-solid seams, non-finite geometry, or triangle edges above the local-cell
    diagonal fail the gate.
20. Large-dig load accounting: 182,704 active cells, 113,935 triangles, and only 140
    deduplicated HF handoff tiles in the reference run. The physical event lookup is
    spatially bucketed, and render footprint submission no longer grows with repeated
    strikes at the same XY location.

Each event records contact `P/N/T/B`, separate impact and pry vectors in world and local
coordinates, penetration, pry angle, tip radius, seed, material, morphology, mass, and
reconstruction bounds. A larger exhaustive/nightly sweep can add azimuth and more
cross-products without weakening this deterministic commit gate.

LSI-adjacent live checks use the same clear/sky RGB classifiers on the action
neighborhood; sky-matching samples → `PRESENTATION_COVERAGE_FAIL`.

## Code

- `PickFracture.h` — contact frame, envelopes, headless cert
- `Main.cpp` — `CarveOccupancyFracture`, `TryPickFoliatedStrike` wire, cert tick,
  atomic HF replacement footprint and tool-scale fracture surface
- `FractureSurface.h` — analytic solid-minus-fracture field and local marching-tetrahedra mesh

## Performance

Preserve P4.5: cost follows causal activity; no global HF refine; no continuous D2 for history.
Historical mouth lookup is spatially bucketed during HF compilation, so terrain work scales
with nearby openings. Tool shape and scale may vary later without changing this ownership law:
each event supplies its own envelope and only that envelope plus the fixed stitch bound is refined.
The HUD reports `frac`, `cells`, and `tris` so accumulated excavation load can be compared in play.

## Saved shelter stamp

`SaveWorld/excavation_stamps_v1.txt` records the finished RANGE-cliff shelter as a
compact, deterministic action recipe (frame, dimensions, tool radius, strike spacing,
and seed). Normal play reloads the recipe after geography residency and rebuilds the
same tool-event union; certificate modes stay isolated from the saved edit. This is the
client's first excavation-stamp persistence path—the broader authoritative world-save
layer does not yet exist in ProvenanceClient.
