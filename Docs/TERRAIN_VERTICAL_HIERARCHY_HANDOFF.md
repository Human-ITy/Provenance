# Terrain vertical hierarchy / presentation-scale geomorphology

Phase 1 audit (do not skip): `Docs/provenance_terrain_vertical_hierarchy_audit.txt`

The ~10 m visual relief was **GradeToZ as the single scale**: `Z = (grade - 0.5) * 64 * 0.125` = **8 m per grade unit**. Peak prominence 0.60 and carrier contrast ~0.56 together produce ~10 m. Massif lift existed on ecological context but was **omitted from SampleZ**. The grade clamp was not the live cause.

## Seam (implemented)

```
TerrainElevationComponents { regional_z, massif_z, peak_z, ridge_z, saddle_z, spur_z, valley_z, local_z }
FinalSurfaceZ = compose(components)
SampleZ := compose   // NOT GradeToZ(SampleGrade)
SampleGrade unchanged  // MW8, _grade_at, carrier cert
```

Macro terms use independent metre scales. Local/regional residual of the 500 m smooth carrier keeps the 8 m/grade law. FableScript emit unchanged.

## Play / cert

```
PLAY_STAGE0_OROGRAPHIC_PHASE17.cmd
CERT_TERRAIN_VERTICAL_HIERARCHY.cmd
  --cert-terrain-vertical-hierarchy-headless
  --cert-terrain-vertical-hierarchy --play-orographic-phase17
```

Spawn: CONTINUE Stage 0 Orographic [20260827] / 1536,1536 / page 1,1

Receipt: `Docs/provenance_terrain_vertical_hierarchy_cert.txt`
