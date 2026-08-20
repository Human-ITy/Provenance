<!-- GENERATED FILE — DO NOT EDIT. -->
<!-- source_schema: schemas/world_descriptors.json -->
<!-- generator_version: ei0b.generator.1 -->
<!-- schema_digest: 8857189f2dfb5fe3ba73d470ca653248188a0717469963d64d6d5003bad6dac5 -->
# Generated World Descriptor Layouts

Protocol compatibility schema digest: `8857189f2dfb5fe3ba73d470ca653248188a0717469963d64d6d5003bad6dac5`.

This digest covers the canonical EI0.A handshake schema identity and the canonical
JSON bytes of the descriptor schema. It is a representation/protocol identity and
does **not** participate in GenesisIdentity.

Quantization uses explicit clipping. `half_up` rounds non-negative ties upward;
`half_even` rounds a tie to the nearest even encoded integer. Decoding unknown
versions, reserved enum values, illegal combinations, or non-zero reserved high bits
fails closed.

## SurfaceState encoding 1

- Encoding ID: `provenance.surface-state.packed`
- Grammar: `provenance.surface-state.ms1a` `ms1a.1`
- Packed width: 38 meaningful bits in uint64 storage
- Certified MS1.A semantic surface state. No RGB or renderer material identity is encoded.

| Field | Bits | Kind | Rule | Required |
|---|---:|---|---|---|
| `substrate_class` | 0..3 | enum | enum `SurfaceSubstrate` | yes |
| `lithology_class` | 4..6 | enum | enum `SurfaceLithology` | yes |
| `dominant_surface_family` | 7..9 | enum | enum `SurfaceFamily` | yes |
| `wetness` | 10..13 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `weathering` | 14..17 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `soil_depth` | 18..21 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `stability` | 22..25 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `organic_potential` | 26..29 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `exposure` | 30..33 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `roughness_proxy` | 34..37 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |

Semantic fields intentionally not packed:

- `source_rev` — Provenance metadata supplied out-of-band; not packed.

Constraints:

- All enum codes must be declared and all bits above bit 37 must be zero.

## WaterState encoding 1

- Encoding ID: `provenance.water-state.packed`
- Grammar: `provenance.water-state.wd1a` `wd1a.1`
- Packed width: 59 meaningful bits in uint64 storage
- Certified WD1.A macro water promise. It is not conserved detailed water mass.

| Field | Bits | Kind | Rule | Required |
|---|---:|---|---|---|
| `presence_regime` | 0..2 | enum | enum `WaterPresence` | yes |
| `body_class` | 3..6 | enum | enum `WaterBody` | yes |
| `flow_regime` | 7..9 | enum | enum `WaterFlow` | yes |
| `bottom_family` | 10..12 | enum | enum `SurfaceFamily` | yes |
| `depth_m` | 13..21 | quantized | 0.0..127.75, encode x4 / decode ÷4, half_even, clip | yes |
| `discharge_proxy` | 22..25 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `seasonality_index` | 26..29 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `clarity` | 30..33 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `turbidity` | 34..37 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `suspended_sediment` | 38..41 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `mineral_load` | 42..45 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `organic_load` | 46..49 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `temperature_proxy` | 50..53 | quantized | 0.0..1.0, encode x15 / decode ÷15, half_up, clip | yes |
| `waterfall_potential` | 54..55 | quantized | 0.0..1.0, encode x3 / decode ÷3, half_even, clip | yes |
| `mineral_potential` | 56..57 | quantized | 0.0..1.0, encode x3 / decode ÷3, half_even, clip | yes |
| `macro_authority` | 58..58 | enum | enum `WaterAuthority` | yes |

Semantic fields intentionally not packed:

- `bottom_elev_m` — Continuous semantic value; not packed in encoding 1.
- `surface_elev_m` — Continuous semantic value; not packed in encoding 1.
- `mean_supply` — Currently reconstructed from discharge_proxy by the legacy decoder; not separately packed.
- `persistence_margin` — Semantic classifier input; not packed in encoding 1.
- `macro_watershed_id` — Static ancestry emitted out-of-band; not packed.
- `macro_channel_id` — Static ancestry emitted out-of-band; not packed.
- `macro_water_body_id` — Static ancestry emitted out-of-band; not packed.
- `water_regime_id` — Static regime identity emitted out-of-band; not packed.
- `source_rev` — Provenance metadata supplied out-of-band; not packed.

Constraints:

- All enum codes must be declared and all bits above bit 58 must be zero.
- DEFER_TO_DETAILED requires encoded dry/none/none placeholders, but decodes with no macro presence conclusion and must never be interpreted as DRY.
- VALID_MACRO may retain a causal body/accommodation class while dry or damp (drainage/body type does not prove water presence); present water requires a non-none body class.

## Enum tables

### SurfaceSubstrate

| Code | Name | Meaning |
|---:|---|---|
| 0 | `bare_bedrock` | Uncovered coherent host rock. |
| 1 | `weathered_bedrock` | Weathered coherent parent rock. |
| 2 | `thin_regolith` | Thin in-place regolith cover. |
| 3 | `colluvium` | Hillslope debris. |
| 4 | `talus` | Coarse rock apron. |
| 5 | `alluvium` | Channel-deposited sediment. |
| 6 | `floodplain_sediment` | Fine floodplain sediment. |
| 7 | `basin_fill` | Closed-basin sediment fill. |
| 8 | `organic_capable` | Substrate capable of organic accumulation; not flora. |
| 9 | `waterlogged_mineral` | Saturated mineral ground; not a water body. |
| 10 | `fresh_lava` | Fresh coherent lava. |
| 11 | `scoria_ash` | Pyroclastic scoria or ash. |
| 12 | `weathered_basalt` | Weathered basaltic parent. |
| 13 | `volcanic_soil` | Developed volcanic soil. |

Reserved: 14, 15.

### SurfaceLithology

| Code | Name | Meaning |
|---:|---|---|
| 0 | `granite` | Granitic intrusive rock. |
| 1 | `basalt` | Basaltic volcanic rock. |
| 2 | `sandstone` | Sandstone. |
| 3 | `shale` | Shale. |
| 4 | `limestone` | Limestone. |
| 5 | `quartzite` | Quartzite. |
| 6 | `metamorphic` | Mixed metamorphic rock. |
| 7 | `mixed_unknown` | Mixed or unresolved macro lithology. |

Reserved: none.

### SurfaceFamily

| Code | Name | Meaning |
|---:|---|---|
| 0 | `rock` | Coherent rock surface. |
| 1 | `regolith` | Regolith-dominant surface. |
| 2 | `sediment` | Deposited sediment surface. |
| 3 | `volcanic` | Volcanic substrate surface. |
| 4 | `organic` | Organic-capable surface. |

Reserved: 5, 6, 7.

### WaterPresence

| Code | Name | Meaning |
|---:|---|---|
| 0 | `dry` | Authoritative dry conclusion. |
| 1 | `damp_substrate` | Damp ground without present surface water; a causal accommodation/body class may still be recorded. |
| 2 | `ephemeral` | Ephemeral water. |
| 3 | `seasonal` | Seasonal water. |
| 4 | `perennial` | Perennial flowing water. |
| 5 | `standing` | Standing water. |

Reserved: 6, 7.

### WaterBody

| Code | Name | Meaning |
|---:|---|---|
| 0 | `none` | No macro water body. |
| 1 | `headwater_stream` | Headwater stream. |
| 2 | `perennial_river` | Perennial river. |
| 3 | `sediment_river` | Sediment-rich river. |
| 4 | `braided_reach` | Braided reach. |
| 5 | `alpine_lake` | Alpine lake. |
| 6 | `closed_basin_lake` | Closed-basin lake. |
| 7 | `floodplain_water` | Floodplain standing water. |
| 8 | `wetland_marsh` | Wetland or marsh. |
| 9 | `organic_darkwater` | Organic darkwater. |
| 10 | `arid_wash` | Arid wash. |
| 11 | `spring_pool` | Spring pool. |
| 12 | `volcanic_mineral_pool` | Volcanic mineral pool. |
| 13 | `crater_lake` | Crater lake. |

Reserved: 14, 15.

### WaterFlow

| Code | Name | Meaning |
|---:|---|---|
| 0 | `none` | No flow conclusion. |
| 1 | `still` | Still water. |
| 2 | `slow` | Slow flow. |
| 3 | `channelized` | Channelized flow. |
| 4 | `fast` | Fast flow. |
| 5 | `turbulent` | Turbulent flow context. |

Reserved: 6, 7.

### WaterAuthority

| Code | Name | Meaning |
|---:|---|---|
| 0 | `valid_macro` | Macro WaterState owns the conclusion. |
| 1 | `defer_to_detailed` | Detailed authority owns the conclusion; this is not dry. |

Reserved: none.

## Grammar versus encoding

Changing this packing while preserving decoded semantic values changes the
representation schema, not genesis. Changing `surface_grammar_version` or
`water_grammar_version` changes semantic baseline identity and therefore genesis.

For deferred water, the wire keeps canonical zero placeholders for the packed
presence/body/flow fields, but generated decoders expose no macro presence
conclusion. `DEFER_TO_DETAILED` therefore never semantically decodes as DRY.
