# Revised spruce and birch library — v002

24 independently placeable models: all 16 species/stage combinations plus eight additional structural variants for young and grown trees. Original v001 assets and layout are retained. This revision is offline authoring, not runtime integration.

## Changes

- All foliage sprays remain present at every detail level. Lower levels simplify each four-triangle bent card to two triangles and reduce wood geometry. They no longer remove 48%/80% of leaf sprays.
- Healthy crown tips now carry additional small branches and foliage. Damaged ancient tops remain intentional separate forms.
- Birch has a new native 1254-square RGBA twig texture based on the available user reference, with serrated pointed leaves, thin forked stems and vein detail. Randomized twig roll breaks the repeated flat comb appearance.
- Spruce branch origins vary more in height, with more closely spaced and less regular tiers and a foliated living leader.
- Denser basal trunk rings create a continuous flare; principal roots are curved and have thicker embedded attachments. Wood and roots remain separate meshes with intersecting attachments, not a watertight sculpted union for fracture physics.
- Two extra narrow/broad structural versions are available for each species at young/grown stages, with independently seeded branching. These are same-stage alternatives, not new ages.

## Review

- [Birch stages](silver_birch_model_sheet.png)
- [Spruce stages](norway_spruce_model_sheet.png)
- [Structural variants](review/structural_variants.png)
- [Birch detail-level comparison](review/silver_birch_lod_comparison.png)
- [Spruce detail-level comparison](review/norway_spruce_lod_comparison.png)
- [Revised grove scene](../../../placement/world_trees_v002/README.md)
- [Full model manifest](manifest.json)
- [96 GLB reimport checks](validation.json)
- [Foliage coverage measurements](review/foliage_coverage_validation.json)

Matched foliage-only silhouettes of the two mature species at front, side and back showed a maximum coverage change of 0.23% between LOD0 and either simplified version. This measures static projected foliage area at the tested scale, not temporal flicker, all viewing angles, engine mipmaps or frame time. The comparison images run LOD0 / LOD1 / LOD2 from left to right. All stages preserve their complete spray lists by construction.

## Placement and lifecycle contract

Choose species and stage, then an eligible structural variant and target height. Apply uniform scale target_height/nominal_height to visual meshes and the separate collision proxy. Root collar is local origin; Blender is Z-up and glTF is Y-up. Roots extend below the origin. Retain stable ID, variant, yaw and scale through streaming and detail-level changes.

Every stage retains the original authored height range. Leaf size also scales, so use the narrower common ranges for ordinary trees and inspect extreme scaling close up. Sample the whole revised root/crown footprint; old bounds and old per-instance root deformation cannot be reused blindly. Bury/conform supported roots to actual terrain, clear stale normals after deformation, and reject unsupported sites. The tree remains gravity-up on a slope.

Litter remains a separate persistent canopy-driven surface layer. Birch uses broadleaf litter; spruce needle/cone litter is still a separate asset to develop. Moss patches, lichen, surface fungus and individual mushrooms stay habitat-driven rather than baked into the models. Excavation should reveal existing roots; stability, chopping and falling remain gameplay systems. Fresh interior wood and stump textures remain in [the cut-wood kit](../../../imagegen/cut-wood/v001/README.md). Variant models inherit the wood species of base_asset_id.

## Rendering and remaining limits

Keep masked double-sided foliage, alpha cutoff 0.35, correct texture color space and alpha-aware mipmaps. All GLBs embed their textures; deduplicate shared resources during engine import. The new twig source, exact prompt and chroma-extracted alpha are archived in textures; the available reference is archived beside this README. The two missing JPEG attachments were not used.

Preserving foliage improves appearance but retains foliage-card overdraw and raises LOD2 geometry relative to the old leaf-deleting version. These are near/mid-distance alternatives, not a finished distant-forest solution. A later baked branch-cluster or whole-tree impostor should reduce pixel and geometry cost while retaining crown coverage. No measured runtime performance improvement is claimed. Leaf transmission, wind and engine-specific shading remain separate work. Constant material roughness and procedural bark remain first-pass materials.

## Individual assets

| Tree / variant | Nominal height (m) | Editable | GLBs (LOD0 / 1 / 2) | Triangles |
|---|---:|---|---|---|
| norway_spruce_adolescent | 10 | [Blender](norway_spruce/norway_spruce_adolescent.blend) | [0](norway_spruce/norway_spruce_adolescent_lod0.glb) / [1](norway_spruce/norway_spruce_adolescent_lod1.glb) / [2](norway_spruce/norway_spruce_adolescent_lod2.glb) | 45568/25272/17761 |
| norway_spruce_ancient | 30 | [Blender](norway_spruce/norway_spruce_ancient.blend) | [0](norway_spruce/norway_spruce_ancient_lod0.glb) / [1](norway_spruce/norway_spruce_ancient_lod1.glb) / [2](norway_spruce/norway_spruce_ancient_lod2.glb) | 36642/20686/13547 |
| norway_spruce_elder | 38 | [Blender](norway_spruce/norway_spruce_elder.blend) | [0](norway_spruce/norway_spruce_elder_lod0.glb) / [1](norway_spruce/norway_spruce_elder_lod1.glb) / [2](norway_spruce/norway_spruce_elder_lod2.glb) | 86076/47654/33766 |
| norway_spruce_grown | 20 | [Blender](norway_spruce/norway_spruce_grown.blend) | [0](norway_spruce/norway_spruce_grown_lod0.glb) / [1](norway_spruce/norway_spruce_grown_lod1.glb) / [2](norway_spruce/norway_spruce_grown_lod2.glb) | 68156/37695/26802 |
| norway_spruce_grown_variant_1 | 20 | [Blender](norway_spruce/norway_spruce_grown_variant_1.blend) | [0](norway_spruce/norway_spruce_grown_variant_1_lod0.glb) / [1](norway_spruce/norway_spruce_grown_variant_1_lod1.glb) / [2](norway_spruce/norway_spruce_grown_variant_1_lod2.glb) | 68156/37695/26802 |
| norway_spruce_grown_variant_2 | 20 | [Blender](norway_spruce/norway_spruce_grown_variant_2.blend) | [0](norway_spruce/norway_spruce_grown_variant_2_lod0.glb) / [1](norway_spruce/norway_spruce_grown_variant_2_lod1.glb) / [2](norway_spruce/norway_spruce_grown_variant_2_lod2.glb) | 68156/37695/26802 |
| norway_spruce_mature | 32 | [Blender](norway_spruce/norway_spruce_mature.blend) | [0](norway_spruce/norway_spruce_mature_lod0.glb) / [1](norway_spruce/norway_spruce_mature_lod1.glb) / [2](norway_spruce/norway_spruce_mature_lod2.glb) | 102152/55674/41846 |
| norway_spruce_sapling | 1.2 | [Blender](norway_spruce/norway_spruce_sapling.blend) | [0](norway_spruce/norway_spruce_sapling_lod0.glb) / [1](norway_spruce/norway_spruce_sapling_lod1.glb) / [2](norway_spruce/norway_spruce_sapling_lod2.glb) | 12288/7044/4273 |
| norway_spruce_seedling | 0.15 | [Blender](norway_spruce/norway_spruce_seedling.blend) | [0](norway_spruce/norway_spruce_seedling_lod0.glb) / [1](norway_spruce/norway_spruce_seedling_lod1.glb) / [2](norway_spruce/norway_spruce_seedling_lod2.glb) | 1522/912/452 |
| norway_spruce_young | 4 | [Blender](norway_spruce/norway_spruce_young.blend) | [0](norway_spruce/norway_spruce_young_lod0.glb) / [1](norway_spruce/norway_spruce_young_lod1.glb) / [2](norway_spruce/norway_spruce_young_lod2.glb) | 27276/15290/10273 |
| norway_spruce_young_variant_1 | 4 | [Blender](norway_spruce/norway_spruce_young_variant_1.blend) | [0](norway_spruce/norway_spruce_young_variant_1_lod0.glb) / [1](norway_spruce/norway_spruce_young_variant_1_lod1.glb) / [2](norway_spruce/norway_spruce_young_variant_1_lod2.glb) | 27276/15292/10273 |
| norway_spruce_young_variant_2 | 4 | [Blender](norway_spruce/norway_spruce_young_variant_2.blend) | [0](norway_spruce/norway_spruce_young_variant_2_lod0.glb) / [1](norway_spruce/norway_spruce_young_variant_2_lod1.glb) / [2](norway_spruce/norway_spruce_young_variant_2_lod2.glb) | 27276/15292/10273 |
| silver_birch_adolescent | 10 | [Blender](silver_birch/silver_birch_adolescent.blend) | [0](silver_birch/silver_birch_adolescent_lod0.glb) / [1](silver_birch/silver_birch_adolescent_lod1.glb) / [2](silver_birch/silver_birch_adolescent_lod2.glb) | 46648/25836/18185 |
| silver_birch_ancient | 18 | [Blender](silver_birch/silver_birch_ancient.blend) | [0](silver_birch/silver_birch_ancient_lod0.glb) / [1](silver_birch/silver_birch_ancient_lod1.glb) / [2](silver_birch/silver_birch_ancient_lod2.glb) | 78434/42912/31526 |
| silver_birch_elder | 24 | [Blender](silver_birch/silver_birch_elder.blend) | [0](silver_birch/silver_birch_elder_lod0.glb) / [1](silver_birch/silver_birch_elder_lod1.glb) / [2](silver_birch/silver_birch_elder_lod2.glb) | 108942/59454/44487 |
| silver_birch_grown | 16 | [Blender](silver_birch/silver_birch_grown.blend) | [0](silver_birch/silver_birch_grown_lod0.glb) / [1](silver_birch/silver_birch_grown_lod1.glb) / [2](silver_birch/silver_birch_grown_lod2.glb) | 76224/41422/31486 |
| silver_birch_grown_variant_1 | 16 | [Blender](silver_birch/silver_birch_grown_variant_1.blend) | [0](silver_birch/silver_birch_grown_variant_1_lod0.glb) / [1](silver_birch/silver_birch_grown_variant_1_lod1.glb) / [2](silver_birch/silver_birch_grown_variant_1_lod2.glb) | 76224/41422/31486 |
| silver_birch_grown_variant_2 | 16 | [Blender](silver_birch/silver_birch_grown_variant_2.blend) | [0](silver_birch/silver_birch_grown_variant_2_lod0.glb) / [1](silver_birch/silver_birch_grown_variant_2_lod1.glb) / [2](silver_birch/silver_birch_grown_variant_2_lod2.glb) | 76224/41422/31486 |
| silver_birch_mature | 22 | [Blender](silver_birch/silver_birch_mature.blend) | [0](silver_birch/silver_birch_mature_lod0.glb) / [1](silver_birch/silver_birch_mature_lod1.glb) / [2](silver_birch/silver_birch_mature_lod2.glb) | 98328/53390/40706 |
| silver_birch_sapling | 1.5 | [Blender](silver_birch/silver_birch_sapling.blend) | [0](silver_birch/silver_birch_sapling_lod0.glb) / [1](silver_birch/silver_birch_sapling_lod1.glb) / [2](silver_birch/silver_birch_sapling_lod2.glb) | 2520/1442/888 |
| silver_birch_seedling | 0.2 | [Blender](silver_birch/silver_birch_seedling.blend) | [0](silver_birch/silver_birch_seedling_lod0.glb) / [1](silver_birch/silver_birch_seedling_lod1.glb) / [2](silver_birch/silver_birch_seedling_lod2.glb) | 1328/792/404 |
| silver_birch_young | 5 | [Blender](silver_birch/silver_birch_young.blend) | [0](silver_birch/silver_birch_young_lod0.glb) / [1](silver_birch/silver_birch_young_lod1.glb) / [2](silver_birch/silver_birch_young_lod2.glb) | 24528/13678/9347 |
| silver_birch_young_variant_1 | 5 | [Blender](silver_birch/silver_birch_young_variant_1.blend) | [0](silver_birch/silver_birch_young_variant_1_lod0.glb) / [1](silver_birch/silver_birch_young_variant_1_lod1.glb) / [2](silver_birch/silver_birch_young_variant_1_lod2.glb) | 24528/13678/9339 |
| silver_birch_young_variant_2 | 5 | [Blender](silver_birch/silver_birch_young_variant_2.blend) | [0](silver_birch/silver_birch_young_variant_2_lod0.glb) / [1](silver_birch/silver_birch_young_variant_2_lod1.glb) / [2](silver_birch/silver_birch_young_variant_2_lod2.glb) | 24528/13678/9356 |

Rebuild: prepare_revision.py derives the revised builder from the preserved v001 source; build_trees.py builds the models; validate_trees.py reimports exports; compare_lods.py renders coverage checks. Keep the generated birch_twig_alpha.png available. Run package_revision.py after the model/layout checks to refresh the handoff.
