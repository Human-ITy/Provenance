# World-cover asset review — 2026-09-07

## Scope and result

Reviewed the existing source art, status notes and light/dark edge previews
under `output/imagegen`. No source art was modified. No goblins were imported.
This is an integration proposal, not a claim that these new covers are already
placed or that their species/ecology are scientifically identified.

| Selected source | Suitable first trial | Import work still required |
| --- | --- | --- |
| `dry-grass/dry_grass_atlas_v005_clean_alpha.png` (1536×1024, 4×2) | Small senescent/dry tuft groups in the more exposed, low-vigor portions of the existing grass field | Fix per-tile bounds, gutters and root alignment in a derivative; do not substitute into the live atlas blindly. Top-row second tuft crosses a nominal tile; lower-left root is offset. Keep current cover/wear eligibility and share the wind path. |
| `moss/moss_irregular_patches_v004_clean_alpha.png` (1254×1254, 2×2) | A few low patches in sheltered soil gaps near satellite bases and under the trial bushes, wherever an explicit damp/shaded habitat is justified | Native texture/material resources; meter-scale patch anchors; surface-conforming blending; grass competition/coverage mask; clipping after excavation. No floating horizontal cards. |
| `moss/moss_carpet_v001_color_source.png` (1254×1254, opaque) | Continuous detail inside selected moss patches, not a replacement for the whole grass field | Not certified seamless; visible repeat/edge differences require a derivative or non-repeating blend. Not calibrated albedo. |
| `fungus/fungus_cover_v004_clean_alpha.png` (1254×1254, 2×2) | One or two small, sheltered organic-litter/soil pockets, or a later decaying-wood fixture | These are ivory mycelial mat, buff film, branching network and mould-like visual concepts, NOT mushrooms and NOT identified species. Establish an organic substrate and moisture/exposure intent before placement. |
| `lichen/lichen_patches_v003_clean_alpha.png` (1254×1254, 2×2) | Sparse pale/ochre crustose and sage/gray foliose patches on old exposed granite | Surface-attached mapping; preserve original-exterior identity through cuts; avoid newly exposed fracture interiors; no thick invented geometry. |

All four transparent sheets have authored edge-cleanup reports. Those reports
are evidence of image preparation, not engine mip/atlas validation. All contain
some painted lighting; none supplies calibrated height, normal or roughness.
Start with restrained color/roughness blending rather than inventing deep relief.

## Ecological interpretation

- Lichen is the appropriate first rock-cover candidate. Crustose and foliose
  forms can occupy rock; these assets are visual forms, not species IDs.
  [National Park Service](https://www.nps.gov/glac/learn/nature/lichens.htm).
- Moss is not simply another random grass texture. Shade and conditions in
  which grass is less competitive can favor it. For this lab, sheltered,
  moisture-retaining patches are a reasonable design hypothesis, not a claim
  that every moss species requires shade or that granite implies acidic soil.
  [University of Maryland Extension](https://extension.umd.edu/resource/moss-lawns).
- Soil fungi belong in the organic/root/soil context, not automatically as a
  visible coating on clean granite. The conspicuous surface mats in this art
  should be a restrained test of that context, not a representation of all
  below-ground fungal activity.
  [USDA NRCS fungi note](https://www.nrcs.usda.gov/state-offices/illinois/soil-tech-note-7a-fungi).

Do not infer a hemisphere, season, species, drainage simulation or measured
substrate chemistry from the current lab. Existing grass vigor/moisture inputs
can guide provisional placement, but do not constitute a complete habitat model.

## Required ownership rules

1. Initial patches have stable anchors, substrate IDs, seed, scale, rotation and
   an explicit initial habitat label. Avoid redistributing them each frame.
2. Soil moss/fungal cover belongs to the current soil exterior: excavation must
   remove or carry it with that surface, not expose a decal hanging in empty air.
3. Granite lichen belongs to the original exterior. A detached piece may inherit
   that exterior cover; a new cut face starts uncolonized. A plain world-space
   noise field would incorrectly paint both the cavity and the new chip interior.
4. Covers must not bridge holes, create a second collision surface, fabricate
   soil thickness, or change granite/soil mass. Initial decorative coverage is
   explicitly not yet harvestable biomass. Any future harvesting requires its
   own material accounting.
5. Validate grazing camera angles, seams, mip bleed, dark/light backgrounds,
   trampling, soil scoops and granite cuts. Preserve the current clean grass
   silhouettes and keep the reported movement catches separately open.

Suggested sequence: review the newly published breeze in play; align dry-grass
tiles; then build one surface-owned patch path and trial a few moss and lichen
patches. Add conspicuous fungal mats only where the substrate supports the art
direction. No ecosystem-wide growth or spread is claimed by this first trial.
