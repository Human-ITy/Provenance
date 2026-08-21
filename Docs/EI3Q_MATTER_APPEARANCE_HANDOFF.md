# EI3.Q Matter Appearance Handoff

## Status

```text
EI3.A — Authority / coverage mechanics                 CERTIFIED
EI3.V — Player-view traversal continuity               CERTIFIED
EI3.Q.A — 4 m matter reconstruction + appearance       CERTIFIED
EI3.Q.B — richer authoritative landform structure      IMPLEMENTED / VISUAL CERT PENDING
```

EI3.Q.A improves the visible use of truth already supplied by FableScript. It
does not add or relocate world authority.

## Authority boundary

FableScript remains authoritative for:

- exact 12.5 cm occupancy and surface height;
- formation, lithology, substrate and landform classification;
- fixed-parts material mixtures;
- weathering, soil depth, wetness and roughness;
- static macro ancestry.

Provenance remains responsible for:

- reconstructing the visible carrier from authoritative samples;
- colour/reflectance resolution from authoritative material mixtures;
- continuous presentation breakup, lighting and LOD;
- residency, meshing, culling and GPU resources.

The client does not decide that a cliff, cave, boulder, waterfall, glacier,
forest or snowfield exists.

## Additive detailed projection contract

The historical `surface-strata-8m-v1` snapshot remains valid and unchanged.
EI3.Q.A adds an optional `occupancy.surface_detail` block:

```text
geometry lattice:       4 m, 17 x 17 samples per AuthorityChunk64m
height encoding:        absolute fixed height-q
semantic encoding:      palette-indexed-surface-context-v1
categorical context:    surface family, material mixture, lithology,
                        substrate and landform
continuous context:     weathering, soil depth, wetness and roughness
material accounting:    exact fixed parts summing to 65535
```

Malformed or incomplete optional detail is rejected. A historical snapshot
without the optional block remains admissible and renders through the existing
8 m fallback.

## Client reconstruction

- Geometry is bilinearly reconstructed from the 4 m height lattice.
- Categorical context uses the nearest authoritative 4 m sample.
- Continuous context is bilinearly reconstructed.
- Four neighbouring fixed-parts mixtures are blended and re-normalized to
  exactly 65535 parts.
- Visible colour resolves from the authoritative mixture and the existing MS1
  appearance vocabulary. World-space breakup changes reflectance only; it does
  not create geometry or semantic truth.
- Grounding and the visible detailed carrier sample the same matter surface.

This removes the former large diagnostic-looking 8 m colour blocks without
returning to a client-authored replacement plane.

## Geological challenge matrix

The cross-province certificate exercises the same projection boundary at:

| Fixture | Absolute coordinate (m) | Existing causal challenge |
|---|---:|---|
| young alpine hard rock | -44,000 / -44,000 | resistant exposed rock |
| sedimentary | -92,000 / -96,000 | sandstone lineage |
| volcanic | -84,000 / -72,000 | basalt / scoria history |
| alluvial | -64,000 / 60,000 | transported mixed sediment |
| talus | -88,000 / -76,000 | loose slope deposit |
| wet basin | -52,000 / -124,000 | waterlogged fine sediment |
| colluvium | -96,000 / -84,000 | gravity-moved mixed deposit |

The gate requires at least three lithologies, all four currently defined broad
surface families, six substrate histories and six dominant materials to survive
the 4 m engine-to-client projection exactly. A uniform grey granite scene
cannot satisfy the certificate. The six reference presentation families listed
below are a separate target vocabulary, not a false claim about this schema.

## Reference doctrine and deferred truth

The supplied references establish the target causal hierarchy:

```text
planet structure
→ tectonic province
→ lithology / uplift / volcanism
→ climate / glaciation / erosion
→ watershed / soil / ecology
→ traversal / habitation
→ seasonal and atmospheric state
```

They also establish six presentation families: water-carved terrain, forested
mountain corridors, cliffside traversal, alpine transition, monumental geology
and human-scale ecological pockets.

EI3.Q.A does not pretend those systems are already authoritative. The current
WorldSubstrate is a lazy analytic surface with strata and material mixtures; it
does not yet encode authoritative overhangs, caves, detached boulders, glaciers,
vegetation populations, seasonal snow or atmospheric state. Those features stay
closed until their FableScript authority exists. When opened, they should enter
the same projection/reconstruction path rather than being painted into the
client.

## Compatibility and regressions

- No macro generation law changed.
- No SurfaceState or WaterState meaning changed.
- No mutation, water, persistence or revision law changed.
- No historical prototype was deleted.
- EI3.V traversal remains the continuity oracle.
- The origin corridor remains geologically uniform granite by authority; the
  challenge matrix prevents that one locale from becoming the visual-quality
  oracle for the whole world.

## EI3.Q.B extension

EI3.Q.B is an opt-in detailed physical baseline extension. FableScript now
derives stable local landform structure from the certified formation,
lithology, slope, soil, wetness and weathering facts. The additive projection
can describe resistant outcrops, sedimentary ledges, volcanic steps, weathered
residuals, talus aprons, depositional banks and subdued ground. Provenance
validates and reconstructs those facts; it does not decide that they exist.

The launcher selects the new baseline with `-RichLandforms`. Without that
switch, the Q.A baseline and projection remain exact. With it, the macro genesis
identity remains unchanged while the substrate/world baseline identity changes
to admit the richer untouched physical truth.

Visible geometry and grounding use the same refined matter-surface sampler.
Missing Q.B data falls back to Q.A, and malformed optional data is rejected.
The client cannot publish a detached body or cavity through this extension.
FableScript explicitly reports that cavity topology is not emitted and supplies
an empty body/cavity set.

The geology/mineral donor documentation at
`claude/seasonal-scar-color-recovery-bqktcv` (SHA
`fef8065def19125aab07416348adc8d4d3397a4e`) was used selectively as design
reference. It reinforced the fact/presentation split and the distinction
between structural/angular rock, granular/slumped deposits and aggregate/rough
ground. No donor implementation was merged wholesale.

## Next dependency

Detached boulders, occupancy-changing caves/overhangs, deeper channel incision,
waterfalls, flora, snow/ice and atmosphere remain closed. Each needs its own
engine-owned law, identity report and golden evidence before the client may
present it as physical world truth.
