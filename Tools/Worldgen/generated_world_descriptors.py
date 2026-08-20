# GENERATED FILE — DO NOT EDIT.
# source_schema: schemas/world_descriptors.json
# generator_version: ei0b.generator.1
# schema_digest: 8857189f2dfb5fe3ba73d470ca653248188a0717469963d64d6d5003bad6dac5

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
from typing import Optional

SCHEMA_FAMILY = 'fablescript.world-descriptors'
SCHEMA_SOURCE_VERSION = 1
SCHEMA_DIGEST = '8857189f2dfb5fe3ba73d470ca653248188a0717469963d64d6d5003bad6dac5'
SURFACE_ENCODING_ID = 'provenance.surface-state.packed'
SURFACE_ENCODING_VERSION = 1
SURFACE_GRAMMAR_ID = 'provenance.surface-state.ms1a'
SURFACE_GRAMMAR_VERSION = 'ms1a.1'
WATER_ENCODING_ID = 'provenance.water-state.packed'
WATER_ENCODING_VERSION = 1
WATER_GRAMMAR_ID = 'provenance.water-state.wd1a'
WATER_GRAMMAR_VERSION = 'wd1a.1'

class DescriptorError(ValueError):
    def __init__(self, code: str, message: str):
        super().__init__(message)
        self.code = code

class SurfaceSubstrate(IntEnum):
    BARE_BEDROCK = 0
    WEATHERED_BEDROCK = 1
    THIN_REGOLITH = 2
    COLLUVIUM = 3
    TALUS = 4
    ALLUVIUM = 5
    FLOODPLAIN_SEDIMENT = 6
    BASIN_FILL = 7
    ORGANIC_CAPABLE = 8
    WATERLOGGED_MINERAL = 9
    FRESH_LAVA = 10
    SCORIA_ASH = 11
    WEATHERED_BASALT = 12
    VOLCANIC_SOIL = 13

class SurfaceLithology(IntEnum):
    GRANITE = 0
    BASALT = 1
    SANDSTONE = 2
    SHALE = 3
    LIMESTONE = 4
    QUARTZITE = 5
    METAMORPHIC = 6
    MIXED_UNKNOWN = 7

class SurfaceFamily(IntEnum):
    ROCK = 0
    REGOLITH = 1
    SEDIMENT = 2
    VOLCANIC = 3
    ORGANIC = 4

class WaterPresence(IntEnum):
    DRY = 0
    DAMP_SUBSTRATE = 1
    EPHEMERAL = 2
    SEASONAL = 3
    PERENNIAL = 4
    STANDING = 5

class WaterBody(IntEnum):
    NONE = 0
    HEADWATER_STREAM = 1
    PERENNIAL_RIVER = 2
    SEDIMENT_RIVER = 3
    BRAIDED_REACH = 4
    ALPINE_LAKE = 5
    CLOSED_BASIN_LAKE = 6
    FLOODPLAIN_WATER = 7
    WETLAND_MARSH = 8
    ORGANIC_DARKWATER = 9
    ARID_WASH = 10
    SPRING_POOL = 11
    VOLCANIC_MINERAL_POOL = 12
    CRATER_LAKE = 13

class WaterFlow(IntEnum):
    NONE = 0
    STILL = 1
    SLOW = 2
    CHANNELIZED = 3
    FAST = 4
    TURBULENT = 5

class WaterAuthority(IntEnum):
    VALID_MACRO = 0
    DEFER_TO_DETAILED = 1

SURFACE_SUBSTRATE_NAMES = ('bare_bedrock', 'weathered_bedrock', 'thin_regolith', 'colluvium', 'talus', 'alluvium', 'floodplain_sediment', 'basin_fill', 'organic_capable', 'waterlogged_mineral', 'fresh_lava', 'scoria_ash', 'weathered_basalt', 'volcanic_soil')
SURFACE_LITHOLOGY_NAMES = ('granite', 'basalt', 'sandstone', 'shale', 'limestone', 'quartzite', 'metamorphic', 'mixed_unknown')
SURFACE_FAMILY_NAMES = ('rock', 'regolith', 'sediment', 'volcanic', 'organic')
WATER_PRESENCE_NAMES = ('dry', 'damp_substrate', 'ephemeral', 'seasonal', 'perennial', 'standing')
WATER_BODY_NAMES = ('none', 'headwater_stream', 'perennial_river', 'sediment_river', 'braided_reach', 'alpine_lake', 'closed_basin_lake', 'floodplain_water', 'wetland_marsh', 'organic_darkwater', 'arid_wash', 'spring_pool', 'volcanic_mineral_pool', 'crater_lake')
WATER_FLOW_NAMES = ('none', 'still', 'slow', 'channelized', 'fast', 'turbulent')
WATER_AUTHORITY_NAMES = ('valid_macro', 'defer_to_detailed')

SURFACE_PACKED_BITS = 38
WATER_PACKED_BITS = 59
_SURFACE_LAYOUT = [(0, 15), (4, 7), (7, 7), (10, 15), (14, 15), (18, 15), (22, 15), (26, 15), (30, 15), (34, 15)]
_WATER_LAYOUT = [(0, 7), (3, 15), (7, 7), (10, 7), (13, 511), (22, 15), (26, 15), (30, 15), (34, 15), (38, 15), (42, 15), (46, 15), (50, 15), (54, 3), (56, 3), (58, 1)]


def _clip(value: float, minimum: float, maximum: float) -> float:
    return minimum if value < minimum else (maximum if value > maximum else value)


def _round_half_up_nonnegative(value: float) -> int:
    return int(value + 0.5)


def _round_half_even_nonnegative(value: float) -> int:
    whole = int(value)
    fraction = value - whole
    if fraction > 0.5 or (fraction == 0.5 and whole & 1):
        return whole + 1
    return whole


def _quantize(value: float, minimum: float, maximum: float,
              denominator: int, rounding: str) -> int:
    clipped = _clip(float(value), minimum, maximum)
    scaled = (clipped - minimum) * denominator
    if rounding == "half_up":
        return _round_half_up_nonnegative(scaled)
    if rounding == "half_even":
        return _round_half_even_nonnegative(scaled)
    raise DescriptorError("invalid_schema", "unknown rounding rule")


def _enum(enum_type, code: int, field: str):
    try:
        return enum_type(code)
    except ValueError:
        raise DescriptorError("reserved_enum", "reserved or unknown {} code {}".format(field, code))


@dataclass(frozen=True)
class SurfaceDescriptor:
    substrate_class: SurfaceSubstrate
    lithology_class: SurfaceLithology
    dominant_surface_family: SurfaceFamily
    wetness: float
    weathering: float
    soil_depth: float
    stability: float
    organic_potential: float
    exposure: float
    roughness_proxy: float
    quantized: tuple


@dataclass(frozen=True)
class WaterDescriptor:
    encoded_presence: WaterPresence
    presence: Optional[WaterPresence]
    body_class: WaterBody
    flow_regime: WaterFlow
    bottom_family: SurfaceFamily
    depth_m: float
    discharge_proxy: float
    seasonality_index: float
    clarity: float
    turbidity: float
    suspended_sediment: float
    mineral_load: float
    organic_load: float
    temperature_proxy: float
    waterfall_potential: float
    mineral_potential: float
    macro_authority: WaterAuthority
    has_presence_conclusion: bool
    quantized: tuple


def _surface_constraint_codes(substrate, lithology, family):
    _enum(SurfaceSubstrate, int(substrate), "substrate_class")
    _enum(SurfaceLithology, int(lithology), "lithology_class")
    _enum(SurfaceFamily, int(family), "dominant_surface_family")


def _water_constraint_codes(presence, body, flow, family, authority):
    presence = _enum(WaterPresence, int(presence), "presence_regime")
    body = _enum(WaterBody, int(body), "body_class")
    _enum(WaterFlow, int(flow), "flow_regime")
    _enum(SurfaceFamily, int(family), "bottom_family")
    authority = _enum(WaterAuthority, int(authority), "macro_authority")
    if authority == WaterAuthority.DEFER_TO_DETAILED:
        if (presence != WaterPresence.DRY or body != WaterBody.NONE or int(flow) != 0):
            raise DescriptorError("illegal_combination", "deferred water must use dry/none/none wire placeholders")
        return
    if presence not in (WaterPresence.DRY, WaterPresence.DAMP_SUBSTRATE) and body == WaterBody.NONE:
        raise DescriptorError("illegal_combination", "present macro water requires a body")


def encode_surface(substrate_class, lithology_class, dominant_surface_family,
                   wetness, weathering, soil_depth, stability, organic_potential,
                   exposure, roughness_proxy, version=SURFACE_ENCODING_VERSION):
    if version != SURFACE_ENCODING_VERSION:
        raise DescriptorError("unknown_encoding_version", "unsupported SurfaceState encoding")
    su, li, fa = int(substrate_class), int(lithology_class), int(dominant_surface_family)
    _surface_constraint_codes(su, li, fa)
    values = [wetness, weathering, soil_depth, stability, organic_potential, exposure, roughness_proxy]
    quantized = [_quantize(value, 0.0, 1.0, 15, "half_up") for value in values]
    packed = 0
    for value, (offset, mask) in zip([su, li, fa] + quantized, _SURFACE_LAYOUT):
        packed |= (int(value) & mask) << offset
    return packed


def decode_surface(version: int, packed: int) -> SurfaceDescriptor:
    if version != SURFACE_ENCODING_VERSION:
        raise DescriptorError("unknown_encoding_version", "unsupported SurfaceState encoding")
    if isinstance(packed, bool) or not isinstance(packed, int) or packed < 0:
        raise DescriptorError("malformed_packed_value", "SurfaceState packed value must be a non-negative integer")
    if packed >> SURFACE_PACKED_BITS:
        raise DescriptorError("reserved_bits", "SurfaceState reserved high bits must be zero")
    raw = [int((packed >> offset) & mask) for offset, mask in _SURFACE_LAYOUT]
    su, li, fa = raw[:3]
    _surface_constraint_codes(su, li, fa)
    q = tuple(raw[3:])
    values = [value / 15.0 for value in q]
    return SurfaceDescriptor(
        SurfaceSubstrate(su), SurfaceLithology(li), SurfaceFamily(fa),
        *values, quantized=q)


def encode_water(presence_regime, body_class, flow_regime, bottom_family, depth_m,
                 discharge_proxy, seasonality_index, clarity, turbidity,
                 suspended_sediment, mineral_load, organic_load, temperature_proxy,
                 waterfall_potential, mineral_potential, macro_authority,
                 version=WATER_ENCODING_VERSION):
    if version != WATER_ENCODING_VERSION:
        raise DescriptorError("unknown_encoding_version", "unsupported WaterState encoding")
    pr, body, flow, fam, auth = (int(presence_regime), int(body_class), int(flow_regime),
                                 int(bottom_family), int(macro_authority))
    _water_constraint_codes(pr, body, flow, fam, auth)
    inputs = [depth_m, discharge_proxy, seasonality_index, clarity, turbidity,
              suspended_sediment, mineral_load, organic_load, temperature_proxy,
              waterfall_potential, mineral_potential]
    specs = [(0.0,127.75,4,"half_even")] + [(0.0,1.0,15,"half_up")]*8 + [(0.0,1.0,3,"half_even")]*2
    quantized = [_quantize(value, *spec) for value, spec in zip(inputs, specs)]
    wire = [pr, body, flow, fam] + quantized + [auth]
    packed = 0
    for value, (offset, mask) in zip(wire, _WATER_LAYOUT):
        packed |= (int(value) & mask) << offset
    return packed


def decode_water(version: int, packed: int) -> WaterDescriptor:
    if version != WATER_ENCODING_VERSION:
        raise DescriptorError("unknown_encoding_version", "unsupported WaterState encoding")
    if isinstance(packed, bool) or not isinstance(packed, int) or packed < 0:
        raise DescriptorError("malformed_packed_value", "WaterState packed value must be a non-negative integer")
    if packed >> WATER_PACKED_BITS:
        raise DescriptorError("reserved_bits", "WaterState reserved high bits must be zero")
    raw = [int((packed >> offset) & mask) for offset, mask in _WATER_LAYOUT]
    pr, body, flow, fam = raw[:4]
    auth = raw[-1]
    _water_constraint_codes(pr, body, flow, fam, auth)
    q = tuple(raw[4:-1])
    values = [q[0] / 4.0] + [value / 15.0 for value in q[1:9]] + [value / 3.0 for value in q[9:11]]
    authority = WaterAuthority(auth)
    encoded_presence = WaterPresence(pr)
    has_conclusion = authority == WaterAuthority.VALID_MACRO
    return WaterDescriptor(
        encoded_presence, encoded_presence if has_conclusion else None,
        WaterBody(body), WaterFlow(flow), SurfaceFamily(fam), *values,
        macro_authority=authority, has_presence_conclusion=has_conclusion,
        quantized=q)
