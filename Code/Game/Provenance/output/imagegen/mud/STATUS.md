# Creek-bank mud v001

Final: creek_bank_mud_v001_seamless_color.png — 1024 x 1024 opaque RGB PNG.
Original generated source: creek_bank_mud_v001_refined_source.png — 1254 x 1254, preserved unchanged; filename follows the shared seam-preparation workflow.
Prompt: creek_bank_mud_v001.prompt.txt.
Method: built-in image_gen followed by periodic overlap quilting with a closed second-axis seam and narrow 3-pixel feather. No upscaling.

Art direction: subdued gray-brown silty mud, compacted fine mineral soil, small soft clods and embedded grit. Intended for creek margins, exposed damp ground and wet excavation surfaces. Approximate one-metre source footprint is art direction, not calibrated measurement. The final crop retains source texel scale.

Reviewed 3 x 3 repeat and native-resolution four-tile junction. No obvious straight wrap seams or corner break visible at reviewed scales. Wrap mean differences: X 9.46, Y 9.86; interior adjacent means: X 8.71, Y 9.64. Larger surface motifs still repeat; future terrain blending can vary their appearance.

This is the color-art layer, with apparent local shading, not calibrated albedo or a complete PBR kit. Moisture darkening, wet roughness and puddle/water behavior belong to material/rendering work; no roughness, normal or height maps were generated. No C++ code or runtime material changed. Engine mip/sampler behavior has not been tested.

Review: creek_bank_mud_v001_repeat_review.jpg and creek_bank_mud_v001_corner_join.png.
Verification: creek_bank_mud_v001_inspection.json.
Reproduction: build_seamless_v001.cjs.

