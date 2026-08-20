"""MV2.A — Regional Macro Authority (Python / world-authority side).

Deterministic, non-repeating, boundary-continuous regional macro authority BEYOND
the frozen central 64 km MW1 region. This is *macro forcing only* — summit / ridge /
basin / valley-scale structure. It never calls the fine causal stack
(ReconstructedZ / erosion / QueryMaterial); it is pure analytic macro math, so it is
cheap by construction.

Design (see MV2_HORIZON_ARCHITECTURE_DESIGN.md):

    macro_z(x,y) = central_envelope(x,y)  +  anchor_window(x,y) * regional_field(x,y)

  - central_envelope = a faithful replica of CausalMacroProvinces::SampleForcing
    .surfaceZ for the frozen central region (read from the compiled .cmp), so the
    center IS the frozen authority.
  - anchor_window = smoothstep(32 km, 64 km, chebyshev(x,y)): exactly 0 with zero
    slope on the +/-32 km center boundary (position AND slope continuity to frozen
    MW1) and 1 beyond 64 km, so the regional field only expresses outside the center.
  - regional_field = a CONTINUOUS absolute-coordinate field (low-frequency tectonic
    base + long mountain belts + broad basins + massifs), deterministic from the
    world seed, with feature wavelengths INDEPENDENT of the 64 km cell (belts span
    many pages). Continuity is by construction (one continuous field); non-repetition
    is by construction (the field varies across absolute space).

64 km is packaging/serialization only, never terrain cadence.
"""

from __future__ import annotations

import math
import struct
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable

import generated_world_descriptors as descriptor_schema
from macro_page_contract import LEGACY_MAGIC as PAGE_MAGIC, upgrade_legacy_page_text

GENERATOR_VERSION = 5
REGION_M = 64000.0          # serialization / ownership cell size (NOT feature scale)
REGION_HALF_M = 32000.0     # frozen central region half-extent
ANCHOR_INNER_M = 32000.0    # anchor window: 0 (with zero slope) at/inside the center boundary
ANCHOR_OUTER_M = 64000.0    # anchor window: fully 1 beyond here

ROOT = Path(__file__).resolve().parents[2]
CENTRAL_CMP = ROOT / "Data" / "Worldgen" / "causal_world_macro_provinces_floor.cmp"

_FNV_OFFSET = 14695981039346656037
_FNV_PRIME = 1099511628211


def fnv1a64(text: str) -> int:
    value = _FNV_OFFSET
    for byte in text.encode("utf-8"):
        value ^= byte
        value = (value * _FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return value


def _hash_unit(*parts: object) -> float:
    """Deterministic float in [0,1) from arbitrary key parts."""
    h = fnv1a64("|".join(str(p) for p in parts))
    return (h >> 11) / float(1 << 53)


def _hash_range(lo: float, hi: float, *parts: object) -> float:
    return lo + (hi - lo) * _hash_unit(*parts)


def smoothstep(edge0: float, edge1: float, x: float) -> float:
    denom = edge1 - edge0
    if denom == 0.0:
        return 0.0 if x < edge0 else 1.0
    t = (x - edge0) / denom
    t = 0.0 if t < 0.0 else (1.0 if t > 1.0 else t)
    return t * t * (3.0 - 2.0 * t)


def _smoother(t: float) -> float:
    """C2 smootherstep weight for lattice interpolation (no kinks in control fields)."""
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0)


def _control(salt: str, x: float, y: float, wavelength: float, octaves: int = 2) -> float:
    """MV3 morphology CONTROL FIELD: a smooth, continuous, seeded value-noise scalar in
    [0,1] over absolute coordinates. Independent fields via `salt` (age, relief, substrate,
    volcanic/plateau tendency, asymmetry, style). Slow wavelengths (~200-500 km) so
    morphology TYPE varies smoothly across the world; incommensurate with the 64 km page
    cadence so no packaging signature. Continuity by construction (lattice + smootherstep).
    """
    val = 0.0
    amp = 1.0
    tot = 0.0
    w = wavelength
    for o in range(octaves):
        gx, gy = x / w, y / w
        ix, iy = math.floor(gx), math.floor(gy)
        fx, fy = gx - ix, gy - iy
        sx, sy = _smoother(fx), _smoother(fy)
        v00 = _hash_unit(salt, o, ix, iy)
        v10 = _hash_unit(salt, o, ix + 1, iy)
        v01 = _hash_unit(salt, o, ix, iy + 1)
        v11 = _hash_unit(salt, o, ix + 1, iy + 1)
        v = (v00 * (1 - sx) * (1 - sy) + v10 * sx * (1 - sy)
             + v01 * (1 - sx) * sy + v11 * sx * sy)
        val += v * amp
        tot += amp
        amp *= 0.5
        w *= 0.5
    return val / tot


def _ridged(salt: str, u: float, v: float, wavelength: float, octaves: int = 3) -> float:
    """Ridged detail in [0,1] (sharp crests) for young-range facets. Absolute-coordinate,
    continuous, deterministic. Used only as SECONDARY relief scaled by (1-age)."""
    val = 0.0
    amp = 1.0
    tot = 0.0
    w = wavelength
    ph = _hash_unit(salt, "phase") * math.tau
    for o in range(octaves):
        r = 1.0 - abs(math.sin(math.tau * (u / w) + ph) + math.sin(math.tau * (v / (w * 1.31)) - ph)) * 0.5
        r = max(0.0, r)
        val += r * r * amp
        tot += amp
        amp *= 0.5
        w *= 0.55
    return val / tot


def _softmax4(a, b, c, d, temp=1.0):
    m = max(a, b, c, d)
    ea = math.exp((a - m) / temp); eb = math.exp((b - m) / temp)
    ec = math.exp((c - m) / temp); ed = math.exp((d - m) / temp)
    s = ea + eb + ec + ed
    return ea / s, eb / s, ec / s, ed / s


@dataclass
class Controls:
    """Local morphology parameters sampled from the independent control fields."""
    age: float          # 0 young/sharp .. 1 old/rounded (affects RELATIONSHIPS, not just blur)
    relief: float       # local relief energy (amplitude regime)
    substrate: float    # resistance/competence (mesas, buttes, canyon walls)
    volcanic: float     # volcanic tendency
    plateau: float      # plateau/escarpment tendency
    asym: float         # deformation asymmetry (fault-block scarps)
    # normalized structural-style weights (sum to 1) — blends, not a named-region enum.
    # Block-fault character is the `asym` control within the belt family (not a 5th style).
    w_belt: float
    w_plateau: float
    w_volcanic: float
    w_cratonic: float


def controls_at(seed: str, x: float, y: float) -> Controls:
    age = _control(seed + ":age", x, y, 380000.0)
    relief = _control(seed + ":relief", x, y, 300000.0)
    substrate = _control(seed + ":substrate", x, y, 260000.0)
    volcanic = _control(seed + ":volcanic", x, y, 440000.0)
    plateau = _control(seed + ":plateau", x, y, 340000.0)
    asym = _control(seed + ":asym", x, y, 300000.0)
    # Structural style is INDEPENDENT of relief (amplitude): a belt region may be
    # Appalachian-low or Alaska-high. Independent style-tendency fields -> softmax weights
    # (continuous blends). Volcanic biased rarer; each family wins a fair share of the world.
    t_belt = _control(seed + ":s_belt", x, y, 330000.0)
    t_plateau = _control(seed + ":s_plateau", x, y, 360000.0) + 0.05
    t_volcanic = _control(seed + ":s_volcanic", x, y, 450000.0) - 0.55    # volcanic is uncommon
    t_cratonic = _control(seed + ":s_cratonic", x, y, 270000.0) + 0.18    # plains are common
    # low temp => one family dominates locally (decisive character), softmax keeps the
    # transition bands smooth (continuity by construction).
    wb, wp, wv, wc = _softmax4(t_belt, t_plateau, t_volcanic, t_cratonic, temp=0.16)
    return Controls(age, relief, substrate, volcanic, plateau, asym, wb, wp, wv, wc)


# --------------------------------------------------------------------------- #
# Frozen central envelope: replica of CausalMacroProvinces::SampleForcing.surfaceZ
# --------------------------------------------------------------------------- #

@dataclass
class CentralProgram:
    belt_azimuth_deg: float = 28.0
    belt_half_width_m: float = 8000.0
    belt_half_length_m: float = 28000.0
    belt_uplift_m: float = 1100.0
    belt_grain_wavelength_m: float = 3200.0
    belt_grain_amplitude_m: float = 85.0
    basin_center_across_m: float = -16000.0
    basin_half_width_m: float = 7000.0
    basin_half_length_m: float = 22000.0
    basin_subsidence_m: float = 380.0
    hinterland_uplift_m: float = 140.0
    datum_z_m: float = 160.0
    seed: str = "mw1-64km-01"
    world_identity_hash: str = "0"
    region_key: str = "causal_world_macro_provinces_floor"

    @staticmethod
    def load(path: Path = CENTRAL_CMP) -> "CentralProgram":
        fields: dict[str, str] = {}
        for line in path.read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            fields[k.strip()] = v.strip()
        g = lambda k, d: float(fields[k]) if k in fields else d
        p = CentralProgram()
        p.belt_azimuth_deg = g("belt_azimuth_deg", p.belt_azimuth_deg)
        p.belt_half_width_m = g("belt_half_width_m", p.belt_half_width_m)
        p.belt_half_length_m = g("belt_half_length_m", p.belt_half_length_m)
        p.belt_uplift_m = g("belt_uplift_m", p.belt_uplift_m)
        p.belt_grain_wavelength_m = g("belt_grain_wavelength_m", p.belt_grain_wavelength_m)
        p.belt_grain_amplitude_m = g("belt_grain_amplitude_m", p.belt_grain_amplitude_m)
        p.basin_center_across_m = g("basin_center_across_m", p.basin_center_across_m)
        p.basin_half_width_m = g("basin_half_width_m", p.basin_half_width_m)
        p.basin_half_length_m = g("basin_half_length_m", p.basin_half_length_m)
        p.basin_subsidence_m = g("basin_subsidence_m", p.basin_subsidence_m)
        p.hinterland_uplift_m = g("hinterland_uplift_m", p.hinterland_uplift_m)
        p.datum_z_m = g("datum_z_m", p.datum_z_m)
        p.seed = fields.get("seed", p.seed)
        p.world_identity_hash = fields.get("world_identity_hash", p.world_identity_hash)
        p.region_key = fields.get("region_key", p.region_key)
        return p


def central_envelope(p: CentralProgram, x: float, y: float) -> float:
    """Macro surface Z of the frozen central region (SampleForcing.surfaceZ)."""
    a = p.belt_azimuth_deg * math.pi / 180.0
    ux, uy = math.cos(a), math.sin(a)
    vx, vy = -math.sin(a), math.cos(a)
    along = x * ux + y * uy
    across = x * vx + y * vy
    belt_len = smoothstep(p.belt_half_length_m, p.belt_half_length_m - 4000.0, abs(along))
    belt_wid = smoothstep(p.belt_half_width_m, p.belt_half_width_m - 2200.0, abs(across))
    belt_env = belt_len * belt_wid
    along_mod = (1.0 + 0.22 * math.sin(2.0 * math.pi * along / 22000.0)
                 + 0.08 * math.sin(2.0 * math.pi * along / 14000.0))
    uplift = p.belt_uplift_m * belt_env * along_mod
    if across > p.belt_half_width_m:
        hinter = (smoothstep(p.belt_half_width_m, p.belt_half_width_m + 1800.0, across)
                  * smoothstep(24000.0, 20000.0, abs(along)))
        uplift += p.hinterland_uplift_m * hinter
    d_basin = across - p.basin_center_across_m
    basin_env = (math.exp(-0.5 * (d_basin / p.basin_half_width_m) ** 2)
                 * smoothstep(p.basin_half_length_m, p.basin_half_length_m - 3500.0, abs(along)))
    subsidence = p.basin_subsidence_m * basin_env
    grain = belt_env * (
        p.belt_grain_amplitude_m * math.sin(2.0 * math.pi * across / p.belt_grain_wavelength_m)
        + 0.33 * p.belt_grain_amplitude_m * math.sin(4.0 * math.pi * across / p.belt_grain_wavelength_m))
    return p.datum_z_m + (uplift - subsidence) + grain


def anchor_window(x: float, y: float) -> float:
    """0 (zero slope) on/inside the +/-32 km center boundary, 1 beyond 64 km."""
    cheb = max(abs(x), abs(y))
    return smoothstep(ANCHOR_INNER_M, ANCHOR_OUTER_M, cheb)


# --------------------------------------------------------------------------- #
# Continuous regional macro field (absolute coordinates, deterministic from seed)
# --------------------------------------------------------------------------- #

@dataclass
class Belt:
    cx: float; cy: float; azimuth_deg: float
    half_length_m: float; half_width_m: float
    uplift_m: float; grain_wavelength_m: float; grain_amplitude_m: float


@dataclass
class Basin:
    cx: float; cy: float; half_width_m: float; half_length_m: float
    azimuth_deg: float; subsidence_m: float


@dataclass
class Massif:
    cx: float; cy: float; radius_m: float; amplitude_m: float


@dataclass
class MacroField:
    seed: str
    belts: list[Belt] = field(default_factory=list)
    basins: list[Basin] = field(default_factory=list)
    massifs: list[Massif] = field(default_factory=list)
    base_wavelength_m: float = 150000.0     # tectonic base, NOT commensurate with 64 km
    base_amplitude_m: float = 220.0
    base_wavelength2_m: float = 78000.0
    base_amplitude2_m: float = 90.0

    @staticmethod
    def build(seed: str) -> "MacroField":
        f = MacroField(seed=seed)
        # A small set of LONG mountain belts (400-900 km) placed across a wide area.
        # Lengths >> 64 km, so each belt naturally spans many pages (feature-scale
        # independence + cross-page ridge continuity).
        n_belts = 6
        for i in range(n_belts):
            f.belts.append(Belt(
                cx=_hash_range(-260000.0, 260000.0, seed, "belt", i, "cx"),
                cy=_hash_range(-260000.0, 260000.0, seed, "belt", i, "cy"),
                azimuth_deg=_hash_range(0.0, 180.0, seed, "belt", i, "az"),
                half_length_m=_hash_range(200000.0, 450000.0, seed, "belt", i, "hl"),
                half_width_m=_hash_range(9000.0, 22000.0, seed, "belt", i, "hw"),
                uplift_m=_hash_range(500.0, 1600.0, seed, "belt", i, "up"),
                grain_wavelength_m=_hash_range(2600.0, 4200.0, seed, "belt", i, "gw"),
                grain_amplitude_m=_hash_range(40.0, 110.0, seed, "belt", i, "ga")))
        # Broad basins (120-300 km).
        n_basins = 6
        for i in range(n_basins):
            f.basins.append(Basin(
                cx=_hash_range(-240000.0, 240000.0, seed, "basin", i, "cx"),
                cy=_hash_range(-240000.0, 240000.0, seed, "basin", i, "cy"),
                half_width_m=_hash_range(60000.0, 150000.0, seed, "basin", i, "hw"),
                half_length_m=_hash_range(90000.0, 220000.0, seed, "basin", i, "hl"),
                azimuth_deg=_hash_range(0.0, 180.0, seed, "basin", i, "az"),
                subsidence_m=_hash_range(150.0, 480.0, seed, "basin", i, "sub")))
        # Massifs (50-95 km), placed on a jittered lattice so several sit near any
        # region without a 64 km period.
        step = 85000.0
        for gi in range(-4, 5):
            for gj in range(-4, 5):
                if _hash_unit(seed, "massif_present", gi, gj) > 0.55:
                    continue
                jx = _hash_range(-0.42, 0.42, seed, "massif_jx", gi, gj) * step
                jy = _hash_range(-0.42, 0.42, seed, "massif_jy", gi, gj) * step
                f.massifs.append(Massif(
                    cx=gi * step + jx, cy=gj * step + jy,
                    radius_m=_hash_range(25000.0, 48000.0, seed, "massif_r", gi, gj),
                    amplitude_m=_hash_range(220.0, 900.0, seed, "massif_a", gi, gj)))
        return f

    def _base(self, x: float, y: float) -> float:
        # Smooth continuous tectonic base: two incommensurate sinusoidal octaves in
        # rotated axes (value-noise-like but analytic, fully continuous, no lattice
        # seams). Wavelengths chosen away from 64 km to avoid any page signature.
        h = fnv1a64(self.seed + ":base")
        pa = (h & 0xFFFF) / 65535.0 * math.pi
        w1, w2 = self.base_wavelength_m, self.base_wavelength2_m
        c1, s1 = math.cos(pa), math.sin(pa)
        u = x * c1 + y * s1
        v = -x * s1 + y * c1
        z = self.base_amplitude_m * (0.6 * math.sin(2 * math.pi * u / w1)
                                     + 0.4 * math.cos(2 * math.pi * v / (w1 * 1.37)))
        z += self.base_amplitude2_m * math.sin(2 * math.pi * (u + 0.5 * v) / w2 + pa)
        return z

    def _belt(self, b: Belt, x: float, y: float, ctl: Controls) -> float:
        a = b.azimuth_deg * math.pi / 180.0
        ux, uy = math.cos(a), math.sin(a)
        vx, vy = -math.sin(a), math.cos(a)
        along = (x - b.cx) * ux + (y - b.cy) * uy
        across = (x - b.cx) * vx + (y - b.cy) * vy
        env_len = smoothstep(b.half_length_m, b.half_length_m - 30000.0, abs(along))
        if env_len <= 0.0:
            return 0.0
        # Cross-profile shaped by erosional maturity (age) and fault-block asymmetry.
        side = across / b.half_width_m
        asym = (ctl.asym - 0.5) * 2.0                          # scarp/dip (block-fault) character
        t = abs(side) * (1.0 + 0.7 * asym * (1.0 if side >= 0 else -1.0))
        t = min(1.6, max(0.0, t))
        age = ctl.age
        young = 1.0 - age
        p_round = smoothstep(1.0, 0.0, t)                      # old: broad, rounded shoulders
        p_sharp = max(0.0, 1.0 - t) ** 0.72                    # young: high narrow crest, steep flank
        p = p_sharp * young + p_round * age
        if p <= 0.0:
            return 0.0
        # young => higher local relief + strong prominence; old => lower regional relief.
        relief_gain = (0.72 + 0.75 * ctl.relief) * (0.85 + 0.5 * young)
        uplift = b.uplift_m * env_len * p * relief_gain
        # young: ridged facets (secondary structural relief); old: shallow dissection grooves
        if young > 0.06:
            facet = _ridged(self.seed + ":beltfacet", along, across, 9000.0)
            uplift += young * 0.30 * b.uplift_m * env_len * p * (facet - 0.42)
        if age > 0.25:
            diss = _ridged(self.seed + ":beltdiss", along * 1.3, across * 1.3, 16000.0)
            uplift -= age * 0.10 * b.uplift_m * env_len * p_round * diss
        grain = b.grain_amplitude_m * math.sin(2 * math.pi * across / b.grain_wavelength_m)
        return uplift + env_len * p * grain * 0.6

    def _basin(self, bs: Basin, x: float, y: float) -> float:
        a = bs.azimuth_deg * math.pi / 180.0
        ux, uy = math.cos(a), math.sin(a)
        vx, vy = -math.sin(a), math.cos(a)
        along = (x - bs.cx) * ux + (y - bs.cy) * uy
        across = (x - bs.cx) * vx + (y - bs.cy) * vy
        env = (math.exp(-0.5 * (across / bs.half_width_m) ** 2)
               * smoothstep(bs.half_length_m, bs.half_length_m - 40000.0, abs(along)))
        return -bs.subsidence_m * env

    def _massif(self, m: Massif, x: float, y: float, ctl: Controls) -> float:
        q2 = ((x - m.cx) ** 2 + (y - m.cy) ** 2) / (m.radius_m ** 2)
        if q2 >= 1.0:
            return 0.0
        s = 1.0 - q2
        q = math.sqrt(q2)
        # volcanic tendency morphs the dome into a radial cone (+ summit crater).
        dome = s * s * (3.0 - 2.0 * s)
        cone = max(0.0, 1.0 - q) ** 1.15
        prof = dome * (1.0 - ctl.w_volcanic) + cone * ctl.w_volcanic
        amp = m.amplitude_m * (0.7 + 0.7 * ctl.relief)
        z = amp * prof
        if ctl.w_volcanic > 0.4 and q < 0.14:
            z -= amp * 0.20 * ctl.w_volcanic * smoothstep(0.14, 0.0, q)   # crater
        young = 1.0 - ctl.age
        if young > 0.1 and prof > 0.05:
            z += young * 0.20 * amp * prof * (_ridged(self.seed + ":mfacet", x, y, 7000.0) - 0.42)
        return z

    # ---- MV3.B2 specialized macro landforms (consequences of the controls) -------
    # Each is a bounded analytic operator gated by the continuous control fields, so a
    # form only appears where its cause dominates (volcanic province, resistant mature
    # plateau, ...). Placement is a seeded jittered lattice; the GATE is read at the
    # feature CENTRE (not the query point) so a feature exists coherently. Feature
    # wavelengths are independent of the 64 km pages and 384 km super-tiles.

    def _volcanic(self, x: float, y: float, ctl: "Controls") -> float:
        # constructional shields/cones where volcanic tendency is high & young; an eroded
        # OLD volcano leaves a resistant plug/neck spire (a differential-erosion remnant).
        sp = 52000.0
        ci, cj = math.floor(x / sp), math.floor(y / sp)
        z = 0.0
        for dj in (-1, 0, 1):
            for di in (-1, 0, 1):
                gi, gj = ci + di, cj + dj
                if _hash_unit(self.seed, "vol_present", gi, gj) > 0.5:
                    continue
                cx = (gi + _hash_range(0.2, 0.8, self.seed, "volx", gi, gj)) * sp
                cy = (gj + _hash_range(0.2, 0.8, self.seed, "voly", gi, gj)) * sp
                cc = controls_at(self.seed, cx, cy)
                if cc.volcanic < 0.77:                   # only genuine volcanic provinces
                    continue
                r = math.hypot(x - cx, y - cy)
                if cc.age < 0.5:                         # young: shield (broad) or cone (steep)
                    rad = _hash_range(9000.0, 32000.0, self.seed, "volr", gi, gj)
                    if r >= rad:
                        continue
                    s = 1.0 - r / rad
                    steep = 1.0 + 2.4 * (0.3 + 0.6 * cc.relief)    # cones steeper in high-relief
                    h = _hash_range(450.0, 1500.0, self.seed, "volh", gi, gj) * (0.55 + 0.8 * cc.relief)
                    z += h * (s ** steep)
                    if r < rad * 0.13:                   # summit crater
                        z -= h * 0.22 * smoothstep(rad * 0.13, 0.0, r)
                else:                                    # old: eroded base + resistant plug spire
                    pr = _hash_range(1400.0, 4200.0, self.seed, "plugr", gi, gj)
                    outer = pr * 4.0
                    if r >= outer:
                        continue
                    z += _hash_range(90.0, 260.0, self.seed, "plugb", gi, gj) * max(0.0, 1.0 - r / outer)
                    if r < pr:
                        z += cc.substrate * _hash_range(320.0, 880.0, self.seed, "plugh", gi, gj) \
                            * max(0.0, 1.0 - r / pr) ** 2.3
        return z

    def _mesa_butte(self, x: float, y: float, ctl: "Controls") -> float:
        # In a mature plateau province the tableland is dissected; resistant caps survive
        # as flat-topped MESAS (young/large) and isolated BUTTES (older/smaller), their tops
        # at the parent plateau level -> genuine erosional remnants, not procedural bumps.
        sp = 15000.0
        ci, cj = math.floor(x / sp), math.floor(y / sp)
        ptop = 240.0 + 900.0 * ctl.relief
        best = 0.0
        for dj in (-1, 0, 1):
            for di in (-1, 0, 1):
                gi, gj = ci + di, cj + dj
                if _hash_unit(self.seed, "mesa_present", gi, gj) > (0.55 - 0.30 * ctl.age):
                    continue
                if _hash_unit(self.seed, "mesa_cap", gi, gj) > ctl.substrate:   # needs a resistant cap
                    continue
                cx = (gi + _hash_range(0.25, 0.75, self.seed, "mx", gi, gj)) * sp
                cy = (gj + _hash_range(0.25, 0.75, self.seed, "my", gi, gj)) * sp
                rad = _hash_range(1600.0, 7500.0, self.seed, "mr", gi, gj) * (1.35 - 0.7 * ctl.age)
                r = math.hypot(x - cx, y - cy)
                if r >= rad:
                    continue
                cap = ptop * (0.45 + 0.55 * ctl.age) * smoothstep(rad, rad - rad * 0.16, r)  # flat top, steep rim
                if cap > best:
                    best = cap
        return best

    def _tower(self, x: float, y: float, ctl: "Controls") -> float:
        # very rare narrow tall survivors where competence is extreme and erosion strong;
        # spatially tied to their host lattice (near ridge/plateau/volcanic parent context).
        sp = 9000.0
        ci, cj = math.floor(x / sp), math.floor(y / sp)
        z = 0.0
        for dj in (-1, 0, 1):
            for di in (-1, 0, 1):
                gi, gj = ci + di, cj + dj
                if _hash_unit(self.seed, "tower_present", gi, gj) > 0.05:       # ~5% of eligible cells
                    continue
                if _hash_unit(self.seed, "tower_cap", gi, gj) > max(0.0, (ctl.substrate - 0.55) * 2.4):
                    continue
                cx = (gi + _hash_range(0.3, 0.7, self.seed, "tx", gi, gj)) * sp
                cy = (gj + _hash_range(0.3, 0.7, self.seed, "ty", gi, gj)) * sp
                rad = _hash_range(300.0, 1100.0, self.seed, "tr", gi, gj)
                r = math.hypot(x - cx, y - cy)
                if r >= rad:
                    continue
                z += _hash_range(180.0, 560.0, self.seed, "th", gi, gj) * max(0.0, 1.0 - r / rad) ** 1.7
        return z

    # ---- MV3.C alpine peak / ridge hierarchy -------------------------------------- #
    # CORE LAW: a mountain range is a peak HIERARCHY, not the highest sample on a smooth
    # uplift. In a high-energy province (young + competent + high relief + orogenic belt
    # style) a range gets a structural SPINE: major summit NODES with real prominence,
    # secondary peaks, deep SADDLES/passes, and convergent (pyramidal/horn) apexes joined
    # by knife RIDGES. Old / weak / low-relief provinces do NOT activate this operator, so
    # they keep the existing broad rounded character (the Sleeping-Lady family). Same
    # continuous absolute-coordinate grammar; a seed either has dramatic alpine ranges or
    # does not, depending on its structure/age/substrate/uplift.

    RANGE_SP_M = 72000.0                   # range-anchor lattice spacing (< is over-dense)

    def _alpine_energy(self, ctl: "Controls") -> float:
        """Orogenic peak-building energy in [0,1]: young + high relief, GATED by belt style +
        rock competence. A weighted amplitude*youth term (not a 4-way product, which collapses
        to ~0) so genuine young competent belts reach 0.5-0.8 while plains/old/plateau stay 0."""
        base = 0.55 * ctl.relief + 0.45 * max(0.0, 1.0 - ctl.age)
        gate = smoothstep(0.30, 0.55, ctl.w_belt) * (0.40 + 0.60 * ctl.substrate)
        return base * gate

    def _range_summits(self, gi: int, gj: int):
        """Deterministic summit hierarchy for the range anchored in lattice cell (gi,gj), or
        None if that cell hosts no high-energy alpine range. Returns a dict with the axis,
        summit nodes (sx,sy,H,R,along), sharpness, ridge width/sag, cluster type, dominant
        summit, key saddle and prominence — shared by the relief operator and peak_at."""
        seed = self.seed
        if _hash_unit(seed, "rng_present", gi, gj) > 0.62:      # ~38% of cells eligible-to-host
            return None
        sp = self.RANGE_SP_M
        cx = (gi + _hash_range(0.2, 0.8, seed, "rngx", gi, gj)) * sp
        cy = (gj + _hash_range(0.2, 0.8, seed, "rngy", gi, gj)) * sp
        cc = controls_at(seed, cx, cy)
        energy = self._alpine_energy(cc)
        if energy < 0.35:                                        # genuine alpine provinces only
            return None
        az = _hash_range(0.0, math.pi, seed, "rngaz", gi, gj)
        ux, uy = math.cos(az), math.sin(az)
        vx, vy = -math.sin(az), math.cos(az)
        Hdom = (_hash_range(1500.0, 2700.0, seed, "rngh", gi, gj)
                * (0.6 + 0.9 * energy) * (0.7 + 0.6 * cc.relief))
        L = _hash_range(14000.0, 34000.0, seed, "rngl", gi, gj)  # along-axis half-length
        cluster = _hash_unit(seed, "rngclu", gi, gj) > 0.5       # cluster (Himalaya) vs solitary (Denali)
        n = 2 + int(_hash_unit(seed, "rngn", gi, gj) * 4.0)      # 2..5 summit nodes
        p = 1.05 + 1.35 * energy                                 # apex sharpness (young/hard = sharp)
        summits = []
        for k in range(n):
            if cluster:
                f = 1.0 - 0.16 * k + (_hash_unit(seed, "rnghk", gi, gj, k) - 0.5) * 0.12
            else:
                f = ([1.0, 0.55, 0.42, 0.34, 0.28][k] if k < 5 else 0.24) \
                    + (_hash_unit(seed, "rnghk", gi, gj, k) - 0.5) * 0.08
            f = max(0.2, f)
            a = ((-1.0 + 2.0 * (k + 0.5) / n) * L
                 + (_hash_unit(seed, "rngak", gi, gj, k) - 0.5) * L * 0.25)
            perp = (_hash_unit(seed, "rngpk", gi, gj, k) - 0.5) * (2000.0 + 4000.0 * (1.0 - energy))
            Hk = Hdom * f
            Rk = (2600.0 + 3400.0 * f) * (0.8 + 0.4 * (1.0 - energy))
            summits.append((cx + ux * a + vx * perp, cy + uy * a + vy * perp, Hk, Rk, a))
        summits_by_h = sorted(summits, key=lambda s: s[2], reverse=True)
        H0 = summits_by_h[0][2]
        H1 = summits_by_h[1][2] if len(summits_by_h) > 1 else 0.0
        sag = 0.35 + 0.30 * energy
        key_saddle = max(0.0, 0.5 * (H0 + H1) - sag * min(H0, H1))
        return {
            "cx": cx, "cy": cy, "cc": cc, "energy": energy, "az": az,
            "summits": summits, "p": p,
            "ridge_w": 900.0 + 2200.0 * (1.0 - energy), "sag": sag,
            "cluster": cluster, "dominant": summits_by_h[0],
            "key_saddle": key_saddle, "prominence": H0 - key_saddle, "Hdom": H0,
        }

    def _range_relief_from(self, x: float, y: float, rp: dict) -> float:
        summits = rp["summits"]
        p = rp["p"]
        z = 0.0
        for (sx, sy, Hk, Rk, _a) in summits:                     # convergent summit cones
            r = math.hypot(x - sx, y - sy)
            if r < Rk:
                z = max(z, Hk * (1.0 - r / Rk) ** p)
        ss = sorted(summits, key=lambda s: s[4])                 # ridges join consecutive summits
        Wr, energy = rp["ridge_w"], rp["energy"]
        sag = rp["sag"]
        qexp = 1.0 + 1.3 * energy
        for k in range(len(ss) - 1):
            ax0, ay0 = ss[k][0], ss[k][1]
            ex, ey = ss[k + 1][0] - ax0, ss[k + 1][1] - ay0
            ll = ex * ex + ey * ey
            if ll < 1.0:
                continue
            u = ((x - ax0) * ex + (y - ay0) * ey) / ll
            if u < 0.0 or u > 1.0:
                continue
            d = math.hypot(x - (ax0 + u * ex), y - (ay0 + u * ey))
            if d >= Wr:
                continue
            H0h, H1h = ss[k][2], ss[k + 1][2]
            crest = (H0h * (1.0 - u) + H1h * u) - sag * min(H0h, H1h) * 4.0 * u * (1.0 - u)
            z = max(z, crest * (1.0 - d / Wr) ** qexp)
        return z

    def _alpine_range(self, x: float, y: float, ctl: "Controls") -> float:
        # cheap early-out: alpine energy varies on 200-500 km control wavelengths and a range
        # reaches < the 72 km lattice spacing, so if the query point is not in a high-energy
        # province no nearby anchor's range can reach it. Skips the lattice scan everywhere
        # except genuine alpine country.
        if self._alpine_energy(ctl) < 0.12:
            return 0.0
        sp = self.RANGE_SP_M
        ci, cj = math.floor(x / sp), math.floor(y / sp)
        best = 0.0
        for dj in (-1, 0, 1):
            for di in (-1, 0, 1):
                rp = self._range_summits(ci + di, cj + dj)
                if rp is None:
                    continue
                z = self._range_relief_from(x, y, rp)
                if z > best:
                    best = z
        return best

    def _special_forms(self, x: float, y: float, ctl: "Controls") -> float:
        # Gate volcanic constructs on the dedicated volcanic-POTENTIAL field (a smooth
        # province), not the rare style-weight; keeps volcanoes rare-but-present across seeds.
        z = 0.0
        if ctl.volcanic > 0.80:
            z += self._volcanic(x, y, ctl)
        if ctl.w_plateau > 0.32 and ctl.age > 0.42:
            z += self._mesa_butte(x, y, ctl)
        if ctl.substrate > 0.60 and ctl.age > 0.45:
            z += self._tower(x, y, ctl)
        return z

    def regional_field(self, x: float, y: float, ctl: "Controls | None" = None,
                       special: bool = True) -> float:
        if ctl is None:
            ctl = controls_at(self.seed, x, y)
        # Additive structural skeleton (the MV2.A field), with each feature already
        # SHAPED by the local morphology controls inside _belt/_massif (age->sharp/rounded,
        # asym->fault-block, volcanic->cone). Amplitude modulated by the relief control and
        # damped in cratonic regions so plains stay legitimately low WITHOUT deleting the
        # skeleton elsewhere.
        amp = (0.55 + 0.9 * ctl.relief) * (1.0 - 0.55 * ctl.w_cratonic)
        z = self._base(x, y) * amp
        for b in self.belts:
            z += self._belt(b, x, y, ctl) * amp
        for m in self.massifs:
            z += self._massif(m, x, y, ctl) * amp
        # Plateau/escarpment operator: where the plateau style is present, reshape the
        # POSITIVE relief into a flat-topped tableland with a steep-but-finite escarpment
        # rim (the smoothstep band), blended by the plateau weight (continuous).
        if ctl.w_plateau > 0.12 and z > 0.0:
            ptop = (240.0 + 900.0 * ctl.relief)
            thr, band = ptop * 0.5, ptop * 0.20
            mesa = ptop * smoothstep(thr - band, thr + band, z)
            # shallow incision on the plateau top (real drainage canyons arrive in MV3.B)
            mesa -= 0.16 * ptop * smoothstep(thr, thr + band, z) * max(
                0.0, _ridged(self.seed + ":platinc", x, y, 13000.0) - 0.5)
            z = z * (1.0 - ctl.w_plateau) + mesa * ctl.w_plateau
        for bs in self.basins:
            z += self._basin(bs, x, y)
        # MV3.C alpine peak/ridge hierarchy: adds the summit-node + knife-ridge + saddle
        # SUPERSTRUCTURE on top of the broad belt uplift in high-energy provinces (young/
        # competent/high-relief orogenic belts). Added AFTER the plateau reshape so summits
        # are never mesa-flattened; 0 (early-out) in every non-alpine province, so old/weak
        # ranges keep their existing broad rounded crests. Part of the base skeleton (not the
        # MV3.B2 `special` gate), so the B2 counterfactual still isolates only B2 forms.
        z += self._alpine_range(x, y, ctl)
        if special:
            z += self._special_forms(x, y, ctl)     # MV3.B2 volcanic / mesa-butte / tower
        return z


def macro_z(central: CentralProgram, fieldf: MacroField, x: float, y: float,
            ctl: "Controls | None" = None, special: bool = True) -> float:
    """The macro surface (pre-drainage): frozen center + anchored regional field with
    MV3.B2 specialized landforms. `special=False` reproduces the MV3.B1 parent surface
    (counterfactual). `ctl` may be a precomputed Controls to avoid recomputing them."""
    aw = anchor_window(x, y)
    if aw <= 0.0:
        return central_envelope(central, x, y)
    return central_envelope(central, x, y) + aw * fieldf.regional_field(x, y, ctl, special)


def landform_at(central: "CentralProgram", fieldf: "MacroField", x: float, y: float):
    """Deterministic special-landform ancestry at a point: (class, id, parent_id, age,
    substrate, center). class in {volcanic_shield/cone/plug, mesa, butte, tower, none}."""
    ctl = controls_at(fieldf.seed, x, y)
    seed = fieldf.seed
    def fid(tag, gi, gj):
        return f"{fnv1a64(seed + f':{tag}:({gi},{gj})'):016x}"
    prov = f"{fnv1a64(seed + f':prov:({int(math.floor(x/200000.0))},{int(math.floor(y/200000.0))})'):016x}"
    # volcanic
    if ctl.volcanic > 0.80:
        sp = 52000.0; ci, cj = math.floor(x / sp), math.floor(y / sp)
        for dj in (-1, 0, 1):
            for di in (-1, 0, 1):
                gi, gj = ci + di, cj + dj
                if _hash_unit(seed, "vol_present", gi, gj) > 0.5:
                    continue
                cx = (gi + _hash_range(0.2, 0.8, seed, "volx", gi, gj)) * sp
                cy = (gj + _hash_range(0.2, 0.8, seed, "voly", gi, gj)) * sp
                cc = controls_at(seed, cx, cy)
                if cc.volcanic < 0.77:
                    continue
                r = math.hypot(x - cx, y - cy)
                if cc.age < 0.5:
                    rad = _hash_range(9000.0, 32000.0, seed, "volr", gi, gj)
                    if r < rad:
                        cls = "volcanic_cone" if cc.relief > 0.5 else "volcanic_shield"
                        return (cls, fid("vol", gi, gj), prov, cc.age, cc.substrate, (cx, cy))
                else:
                    if r < _hash_range(1400.0, 4200.0, seed, "plugr", gi, gj):
                        return ("volcanic_plug", fid("vol", gi, gj), prov, cc.age, cc.substrate, (cx, cy))
    # tower (rarer, check before mesa so a capped tower wins)
    if ctl.substrate > 0.60 and ctl.age > 0.45:
        sp = 9000.0; ci, cj = math.floor(x / sp), math.floor(y / sp)
        for dj in (-1, 0, 1):
            for di in (-1, 0, 1):
                gi, gj = ci + di, cj + dj
                if _hash_unit(seed, "tower_present", gi, gj) > 0.05:
                    continue
                if _hash_unit(seed, "tower_cap", gi, gj) > max(0.0, (ctl.substrate - 0.55) * 2.4):
                    continue
                cx = (gi + _hash_range(0.3, 0.7, seed, "tx", gi, gj)) * sp
                cy = (gj + _hash_range(0.3, 0.7, seed, "ty", gi, gj)) * sp
                if math.hypot(x - cx, y - cy) < _hash_range(300.0, 1100.0, seed, "tr", gi, gj):
                    return ("tower", fid("tower", gi, gj), prov, ctl.age, ctl.substrate, (cx, cy))
    # mesa / butte
    if ctl.w_plateau > 0.32 and ctl.age > 0.42:
        sp = 15000.0; ci, cj = math.floor(x / sp), math.floor(y / sp)
        for dj in (-1, 0, 1):
            for di in (-1, 0, 1):
                gi, gj = ci + di, cj + dj
                if _hash_unit(seed, "mesa_present", gi, gj) > (0.55 - 0.30 * ctl.age):
                    continue
                if _hash_unit(seed, "mesa_cap", gi, gj) > ctl.substrate:
                    continue
                cx = (gi + _hash_range(0.25, 0.75, seed, "mx", gi, gj)) * sp
                cy = (gj + _hash_range(0.25, 0.75, seed, "my", gi, gj)) * sp
                rad = _hash_range(1600.0, 7500.0, seed, "mr", gi, gj) * (1.35 - 0.7 * ctl.age)
                if math.hypot(x - cx, y - cy) < rad:
                    cls = "butte" if (ctl.age > 0.68 and rad < 5000.0) else "mesa"
                    return (cls, fid("mesa", gi, gj), prov, ctl.age, ctl.substrate, (cx, cy))
    return ("none", "none", prov, ctl.age, ctl.substrate, (x, y))


def peak_at(central: "CentralProgram", fieldf: "MacroField", x: float, y: float):
    """MV3.C alpine peak ancestry at a point (pure absolute-coordinate function; no super-tile
    dependency): (class, MacroPeakId, ParentRangeId, summit_elev_m, prominence_m,
    key_saddle_m, cluster). class in {dominant_summit, secondary_peak, ridge, flank, none}.
    summit_elev / prominence are the ALPINE superstructure relief (above the belt base)."""
    ctl = controls_at(fieldf.seed, x, y)
    seed = fieldf.seed
    none = ("none", "none", "none", 0.0, 0.0, 0.0, False)
    if fieldf._alpine_energy(ctl) < 0.12:
        return none
    sp = fieldf.RANGE_SP_M
    ci, cj = math.floor(x / sp), math.floor(y / sp)
    best_gi = best_gj = None
    best_rp = None
    best_z = 0.0
    for dj in (-1, 0, 1):
        for di in (-1, 0, 1):
            rp = fieldf._range_summits(ci + di, cj + dj)
            if rp is None:
                continue
            z = fieldf._range_relief_from(x, y, rp)
            if z > best_z:
                best_z, best_gi, best_gj, best_rp = z, ci + di, cj + dj, rp
    if best_rp is None:
        return none
    rid = f"{fnv1a64(seed + f':range:({best_gi},{best_gj})'):016x}"
    ns = min(best_rp["summits"], key=lambda s: math.hypot(x - s[0], y - s[1]))
    dist = math.hypot(x - ns[0], y - ns[1])
    is_dom = abs(ns[2] - best_rp["Hdom"]) < 1e-6
    pid = f"{fnv1a64(seed + f':peak:({best_gi},{best_gj}):({ns[4]:.0f})'):016x}"
    if dist < ns[3] * 0.30:
        cls = "dominant_summit" if is_dom else "secondary_peak"
    elif best_z > 0.35 * best_rp["Hdom"]:
        cls = "ridge"
    else:
        cls = "flank"
    return (cls, pid, rid, ns[2], best_rp["prominence"], best_rp["key_saddle"], best_rp["cluster"])


# =========================================================================== #
# MV3.B1 — Macro drainage graph + canyon incision (Python world-authority)
# =========================================================================== #
#
# CORE LAW: canyons follow drainage; drainage is NOT painted to resemble canyons.
# A deterministic, downhill-connected, cross-page drainage graph is COMPILED from the
# MV3.A macro surface, and canyon/valley morphology is carved ONLY along its certified
# thalwegs. No free ridged-noise "canyons", no erosion sim, no runtime water.
#
# Ownership is keyed to fixed ABSOLUTE super-tiles (aligned to the world origin, NOT to
# the 64 km pages), so a trunk crossing pages (-1,0)->(0,0)->(1,0) keeps one identity.
# Each super-tile compiles over its 384 km core + a 96 km halo (so basins that straddle a
# core edge route correctly) at a coarse 2 km macro resolution. The 25-page +/-160 km
# render ring lies entirely inside super-tile (0,0), so no super-tile seam enters the
# rendered world; long-distance samples land in other super-tiles (each self-consistent).

import heapq

DRAIN_RES_M = 2000.0                 # coarse macro drainage cell (1-4 km band; cheapest defensible)
SUPER_M = 384000.0                   # super-tile core (absolute, origin-aligned; != 64 km pages)
SUPER_HALO_M = 152000.0              # halo >> max upstream reach so boundary cells' capped
                                     # upstream is captured by BOTH neighbours (magnitude agrees)
CHANNEL_ACCUM_CELLS = 220            # >= this upstream area (cells, 1 cell=4 km2) is a trunk channel
R_CARVE = 12                         # pit breach search radius (cells, 24 km) -- << halo (window-independent)
ACCUM_CAP = 1700.0                   # accumulation ceiling: major trunks saturate in BOTH neighbours
                                     # (still ~8x the channel threshold), so incision agrees at the seam
DRAIN_GENERATOR_VERSION = 2          # window-independent raw-D8 routing (super-tile boundary safe)


def _super_index(x: float, y: float):
    return (int(math.floor((x + SUPER_M * 0.5) / SUPER_M)),
            int(math.floor((y + SUPER_M * 0.5) / SUPER_M)))


@dataclass
class DrainageSolution:
    sti: int
    stj: int
    x0: float
    y0: float
    res: float
    n: int
    accum: list          # upstream cell count (flow accumulation)
    down: list           # D8 downstream neighbour index, or -1 terminal (spill/outlet)
    wshed: list          # MacroWatershed label = index of the outlet each cell drains to
    incision: list       # canyon incision depth (m, >= 0)
    nearest_depth: list  # depth of the nearest channel (for cross-section)
    channel: list        # bool: is a trunk channel cell
    src_rev: str
    digest: str
    ascending_edges: int = 0     # non-terminal edges that ASCEND (routing bug if > 0)
    interior_basins: int = 0     # closed endorheic basins (terminal not on grid boundary)
    boundary_terminals: int = 0  # terminals at the grid edge (spill out of the tile)


_DRAIN_CACHE: dict = {}
_D8 = [(-1, -1), (0, -1), (1, -1), (-1, 0), (1, 0), (-1, 1), (0, 1), (1, 1)]


def compile_drainage(central: "CentralProgram", fieldf: "MacroField",
                     sti: int, stj: int) -> DrainageSolution:
    """Compile (and cache) the macro drainage graph for one absolute super-tile."""
    key = (fieldf.seed, sti, stj)
    cached = _DRAIN_CACHE.get(key)
    if cached is not None:
        return cached
    cx, cy = sti * SUPER_M, stj * SUPER_M
    half = SUPER_M * 0.5 + SUPER_HALO_M
    x0, y0 = cx - half, cy - half
    n = int(round(2.0 * half / DRAIN_RES_M)) + 1
    nn = n * n
    # 1. Sample the MV3.A macro surface + morphology controls on the drainage grid.
    Z = [0.0] * nn
    age = [0.0] * nn
    sub = [0.0] * nn
    plat = [0.0] * nn
    seed = fieldf.seed
    for j in range(n):
        yy = y0 + j * DRAIN_RES_M
        base = j * n
        for i in range(n):
            xx = x0 + i * DRAIN_RES_M
            k = base + i
            c = controls_at(seed, xx, yy)         # once per cell (reused for surface + incision)
            Z[k] = macro_z(central, fieldf, xx, yy, c)
            age[k] = c.age
            sub[k] = c.substrate
            plat[k] = c.w_plateau
    # 2-3. WINDOW-INDEPENDENT routing. down[k] is D8 steepest descent on the RAW macro
    #      surface (NOT a windowed priority-flood fill), so it is a pure function of the
    #      absolute Z field in a fixed local neighbourhood of k. Two adjacent super-tiles
    #      therefore compute IDENTICAL routing in their shared halo -> the drainage graph
    #      crosses the 384 km super-tile boundary with no seam and no reset (the unbounded-
    #      world invariant). Small pits are breached by a BOUNDED local carve: jump to the
    #      lowest cell strictly below the pit within R_CARVE cells (still a pure local
    #      function). A genuine closed basin with no lower cell in reach stays an endorheic
    #      sink (down=-1). Z strictly decreases along every edge, so there are no cycles.
    down = [-1] * nn
    for k in range(nn):
        kx, ky = k % n, k // n
        e = Z[k]
        best, bj = 0.0, -1
        for dx, dy in _D8:
            nx, ny = kx + dx, ky + dy
            if 0 <= nx < n and 0 <= ny < n:
                m = ny * n + nx
                dist = 1.41421356 if (dx and dy) else 1.0
                drop = (e - Z[m]) / dist
                if drop > best:
                    best, bj = drop, m
        if bj < 0:                                # pit: bounded local carve to a lower cell
            lo, lm = e, -1
            for dy in range(-R_CARVE, R_CARVE + 1):
                ny = ky + dy
                if ny < 0 or ny >= n:
                    continue
                rown = ny * n
                for dx in range(-R_CARVE, R_CARVE + 1):
                    nx = kx + dx
                    if nx < 0 or nx >= n or (dx == 0 and dy == 0):
                        continue
                    zm = Z[rown + nx]
                    if zm < lo:
                        lo, lm = zm, rown + nx
            bj = lm
        down[k] = bj
    # routing validity: raw-D8/carve descends by construction; classify terminals.
    ascending = 0
    interior_basins = 0
    boundary_terminals = 0
    for k in range(nn):
        d = down[k]
        if d < 0:
            kx, ky = k % n, k // n
            if kx == 0 or ky == 0 or kx == n - 1 or ky == n - 1:
                boundary_terminals += 1
            else:
                interior_basins += 1          # a genuine endorheic closed basin
        elif Z[d] > Z[k] + 1e-6:
            ascending += 1
    # 4. Flow accumulation over the RAW-elevation order (down always goes strictly lower).
    #    Capped so a cross-boundary trunk's magnitude is dominated by nearby upstream that
    #    both super-tiles' halos contain (magnitude agrees across the boundary, not just
    #    direction); the cap is far above any channel threshold.
    accum = [1.0] * nn
    order = sorted(range(nn), key=lambda k: Z[k], reverse=True)
    for k in order:
        d = down[k]
        if d >= 0:
            accum[d] = min(ACCUM_CAP, accum[d] + accum[k])
    # 5. Watershed label = the terminal outlet each cell drains to (path compression).
    wshed = [-1] * nn
    for start in range(nn):
        if wshed[start] != -1:
            continue
        path = []
        k = start
        while k >= 0 and wshed[k] == -1:
            path.append(k)
            k = down[k]
        outlet = wshed[k] if (k >= 0 and wshed[k] != -1) else (path[-1] if k < 0 else k)
        for p in path:
            wshed[p] = outlet
    # 6. Channels + canyon incision. Depth ~ stream power (accum^m * slope^n), shaped by
    #    substrate competence and erosional maturity; cross-section width/steepness from
    #    the same controls (resistant/young -> narrow steep canyon; weak/old -> broad valley).
    channel = bytearray(nn)
    depth = [0.0] * nn
    for k in range(nn):
        if accum[k] < CHANNEL_ACCUM_CELLS:
            continue
        channel[k] = 1
        kx, ky = k % n, k // n
        d = down[k]
        slope = 0.0
        if d >= 0:
            ddx, ddy = (d % n) - kx, (d // n) - ky
            dl = math.sqrt(ddx * ddx + ddy * ddy) * DRAIN_RES_M      # true (possibly carved) reach
            slope = max(0.0, (Z[k] - Z[d]) / dl) if dl > 0 else 0.0
        a = accum[k]
        # stream power incision; resistant substrate cuts deeper (steep-walled), young keeps
        # relief so canyons stay prominent, plateau context favours box canyons.
        comp = 0.55 + 0.9 * sub[k]
        young = 1.0 - age[k]
        base_d = 55.0 * (a ** 0.34) * (0.22 + slope ** 0.55)          # stream power (A^m S^n)
        depth[k] = min(600.0, base_d * (0.5 + comp) * (0.7 + 0.6 * young) * (1.0 + 0.5 * plat[k]))
    # multi-source distance transform from channel cells; carry the source channel depth.
    INF = 1e18
    dist = [INF] * nn
    nearest_depth = [0.0] * nn
    dq = []
    for k in range(nn):
        if channel[k]:
            dist[k] = 0.0
            nearest_depth[k] = depth[k]
            heapq.heappush(dq, (0.0, k))
    while dq:
        dcur, k = heapq.heappop(dq)
        if dcur > dist[k]:
            continue
        kx, ky = k % n, k // n
        for dx, dy in _D8:
            nx, ny = kx + dx, ky + dy
            if 0 <= nx < n and 0 <= ny < n:
                m = ny * n + nx
                step = (1.41421356 if (dx and dy) else 1.0) * DRAIN_RES_M
                nd = dcur + step
                if nd < dist[m]:
                    dist[m] = nd
                    nearest_depth[m] = nearest_depth[k]
                    heapq.heappush(dq, (nd, m))
    # cross-section: incision falls off from the thalweg over a control-dependent width.
    incision = [0.0] * nn
    for k in range(nn):
        dep = nearest_depth[k]
        if dep <= 0.0:
            continue
        width = 2200.0 + 6000.0 * (1.0 - sub[k]) + 3500.0 * age[k]   # resistant/young narrow
        wall = 2.4 - 1.3 * (1.0 - sub[k]) - 0.4 * age[k]             # resistant steep walls
        t = dist[k] / width
        if t >= 1.0:
            continue
        prof = (1.0 - t) ** max(0.8, wall)
        incision[k] = dep * prof
    # digest of the graph (accum + incision, quantized) for determinism fixtures.
    hh = _FNV_OFFSET
    for k in range(0, nn, 7):
        q = (int(accum[k]) ^ (int(incision[k] * 10.0) << 20)) & 0xFFFFFFFFFFFFFFFF
        hh ^= q
        hh = (hh * _FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    sol = DrainageSolution(
        sti=sti, stj=stj, x0=x0, y0=y0, res=DRAIN_RES_M, n=n,
        accum=accum, down=down, wshed=wshed, incision=incision,
        nearest_depth=nearest_depth, channel=channel,
        src_rev=f"gv{GENERATOR_VERSION}.dgv{DRAIN_GENERATOR_VERSION}",
        digest=f"{hh:016x}",
        ascending_edges=ascending, interior_basins=interior_basins,
        boundary_terminals=boundary_terminals)
    _DRAIN_CACHE[key] = sol
    return sol


def _drain_bilinear(sol: DrainageSolution, grid: list, x: float, y: float) -> float:
    fx = (x - sol.x0) / sol.res
    fy = (y - sol.y0) / sol.res
    i0 = int(math.floor(fx))
    j0 = int(math.floor(fy))
    if i0 < 0 or j0 < 0 or i0 >= sol.n - 1 or j0 >= sol.n - 1:
        ic = min(max(i0, 0), sol.n - 1)
        jc = min(max(j0, 0), sol.n - 1)
        return grid[jc * sol.n + ic]
    tx, ty = fx - i0, fy - j0
    n = sol.n
    a = grid[j0 * n + i0]; b = grid[j0 * n + i0 + 1]
    c = grid[(j0 + 1) * n + i0]; d = grid[(j0 + 1) * n + i0 + 1]
    return a * (1 - tx) * (1 - ty) + b * tx * (1 - ty) + c * (1 - tx) * ty + d * tx * ty


def incision_at(central: "CentralProgram", fieldf: "MacroField", x: float, y: float) -> float:
    sti, stj = _super_index(x, y)
    sol = compile_drainage(central, fieldf, sti, stj)
    return _drain_bilinear(sol, sol.incision, x, y)


def drainage_query(central: "CentralProgram", fieldf: "MacroField", x: float, y: float):
    """(watershed_id, channel_id, accumulation, incision_m) for a point — fixtures/evidence."""
    sti, stj = _super_index(x, y)
    sol = compile_drainage(central, fieldf, sti, stj)
    fx = int(round((x - sol.x0) / sol.res))
    fy = int(round((y - sol.y0) / sol.res))
    fx = min(max(fx, 0), sol.n - 1)
    fy = min(max(fy, 0), sol.n - 1)
    k = fy * sol.n + fx
    out = sol.wshed[k]
    ox = sol.x0 + (out % sol.n) * sol.res
    oy = sol.y0 + (out // sol.n) * sol.res
    wid = f"{fnv1a64(fieldf.seed + f':mwshed:({ox:.0f},{oy:.0f})'):016x}"     # MacroWatershedId
    # channel identity keyed to the trunk's downstream-most channel cell (stable ancestry).
    ck = k
    guard = 0
    while sol.channel[ck] and sol.down[ck] >= 0 and sol.channel[sol.down[ck]] and guard < 100000:
        ck = sol.down[ck]
        guard += 1
    cxp = sol.x0 + (ck % sol.n) * sol.res
    cyp = sol.y0 + (ck // sol.n) * sol.res
    cid = f"{fnv1a64(fieldf.seed + f':mchan:({cxp:.0f},{cyp:.0f})'):016x}" if sol.channel[k] else "none"
    return wid, cid, sol.accum[k], sol.incision[k]


def macro_z_incised(central: "CentralProgram", fieldf: "MacroField", x: float, y: float,
                    incise: bool = True, special: bool = True) -> float:
    """Full macro surface with MV3.B2 special landforms and drainage-carved canyons.
    Incision is gated by the anchor window (exactly 0 inside the frozen +/-32 km center,
    MW4 not overwritten). incise=False drops canyons; special=False reproduces the MV3.B1
    parent surface (counterfactuals)."""
    z = macro_z(central, fieldf, x, y, None, special)
    if not incise:
        return z
    return z - anchor_window(x, y) * incision_at(central, fieldf, x, y)


# =========================================================================== #
# MS1.A — SurfaceState authority + composition (Python world-authority)
# =========================================================================== #
#
# CORE LAW: surface appearance is DOWNSTREAM. First the world must KNOW what the
# exposed surface actually IS — substrate, lithology/parent, and material state —
# independent of how any renderer colours it. MS1.A owns that knowledge and NOTHING
# else: no palette retirement, no appearance resolver, no geometry change.
#
# ONE composition law (`compose_surface_state`) resolves a compact semantic
# `SurfaceState` from authority-agnostic `SurfaceInputs`, applying a fixed exposure
# precedence (deposit -> regolith -> weathered parent -> host bedrock) then state
# overlays (wetness/organic/exposure/weathering/stability). The SAME law is fed by
# two authority regimes that SHARE this vocabulary (the unbounded-world doctrine):
#   * MACRO (unbounded, MV3 controls only): `_macro_surface_inputs` derives inputs
#     from the morphology controls + drainage + landform ancestry -- cheap analytic,
#     never the fine causal stack. This is the far-authority PROMISE.
#   * FINE (central +/-32 km, MW1-8 present): the same law is fed by real MW2/5/6/7/8
#     samples in the engine (MS1.B / detail-on-approach); MS1.A does not generate fine
#     MW detail at macro range and does not modify the frozen fine authorities.
# Distance may SIMPLIFY the presentation; it may never change the semantic identity.

SURFACE_DESCRIPTOR_VERSION = descriptor_schema.SURFACE_ENCODING_VERSION

# Substrate class vocabulary (MW7 RegolithProfileClass-style, + the volcanic substrates
# MV3.B2 ancestry needs). Order is the on-page integer code; append-only.
SUBSTRATE_CLASSES = list(descriptor_schema.SURFACE_SUBSTRATE_NAMES)
# Lithology families (what the rock/parent sediment IS; compact by decree).
LITHOLOGY_CLASSES = list(descriptor_schema.SURFACE_LITHOLOGY_NAMES)
# Coarse dominant surface family -- the near/far agreement axis (Certificate C). Distance
# collapses substrate sub-classes toward THIS; it is what may never contradict near vs far.
SURFACE_FAMILIES = list(descriptor_schema.SURFACE_FAMILY_NAMES)

_SUBSTRATE_IDX = {n: i for i, n in enumerate(SUBSTRATE_CLASSES)}
_LITHOLOGY_IDX = {n: i for i, n in enumerate(LITHOLOGY_CLASSES)}
_FAMILY_IDX = {n: i for i, n in enumerate(SURFACE_FAMILIES)}

# substrate -> coarse family (the distance-collapse mapping)
_SUBSTRATE_FAMILY = {
    "bare_bedrock": "rock", "weathered_bedrock": "rock",
    "thin_regolith": "regolith", "colluvium": "regolith", "talus": "regolith",
    "organic_capable": "organic",
    "alluvium": "sediment", "floodplain_sediment": "sediment",
    "basin_fill": "sediment", "waterlogged_mineral": "sediment",
    "fresh_lava": "volcanic", "scoria_ash": "volcanic",
    "weathered_basalt": "volcanic", "volcanic_soil": "volcanic",
}
# nominal grain/coarseness per substrate (fed as the roughness proxy default)
_SUBSTRATE_GRAIN = {
    "bare_bedrock": 0.55, "weathered_bedrock": 0.50, "thin_regolith": 0.40,
    "colluvium": 0.70, "talus": 0.92, "alluvium": 0.45, "floodplain_sediment": 0.20,
    "basin_fill": 0.25, "organic_capable": 0.30, "waterlogged_mineral": 0.22,
    "fresh_lava": 0.85, "scoria_ash": 0.78, "weathered_basalt": 0.60, "volcanic_soil": 0.33,
}


def _q4(v: float) -> int:
    """Quantise [0,1] -> 4-bit nibble (deterministic, documented)."""
    if v <= 0.0:
        return 0
    if v >= 1.0:
        return 15
    return int(v * 15.0 + 0.5)


def _dq4(n: int) -> float:
    return n / 15.0


@dataclass
class SurfaceState:
    """Compact SEMANTIC surface record at a point. NO stored RGB; NO renderer material
    id as authority. State axes are continuous [0,1] (quantised to nibbles on the page)."""
    substrate_class: str
    lithology_class: str
    wetness: float
    weathering: float
    soil_depth: float
    stability: float
    organic_potential: float
    exposure: float
    roughness_proxy: float
    dominant_surface_family: str
    source_rev: str = ""

    def code_tuple(self):
        """(substrate_idx, lithology_idx, family_idx, nibble state axes) for packing/digest."""
        return (_SUBSTRATE_IDX[self.substrate_class],
                _LITHOLOGY_IDX[self.lithology_class],
                _FAMILY_IDX[self.dominant_surface_family],
                _q4(self.wetness), _q4(self.weathering), _q4(self.soil_depth),
                _q4(self.stability), _q4(self.organic_potential), _q4(self.exposure),
                _q4(self.roughness_proxy))

    def pack(self) -> int:
        """Pack to a single uint (<=40 bits): 4b substrate,3b lith,3b family,7x4b axes."""
        return descriptor_schema.encode_surface(
            _SUBSTRATE_IDX[self.substrate_class], _LITHOLOGY_IDX[self.lithology_class],
            _FAMILY_IDX[self.dominant_surface_family], self.wetness, self.weathering,
            self.soil_depth, self.stability, self.organic_potential, self.exposure,
            self.roughness_proxy, version=SURFACE_DESCRIPTOR_VERSION)


def unpack_surface(v: int, source_rev: str = "") -> SurfaceState:
    decoded = descriptor_schema.decode_surface(SURFACE_DESCRIPTOR_VERSION, v)
    return SurfaceState(
        substrate_class=SUBSTRATE_CLASSES[int(decoded.substrate_class)],
        lithology_class=LITHOLOGY_CLASSES[int(decoded.lithology_class)],
        wetness=decoded.wetness, weathering=decoded.weathering,
        soil_depth=decoded.soil_depth, stability=decoded.stability,
        organic_potential=decoded.organic_potential, exposure=decoded.exposure,
        roughness_proxy=decoded.roughness_proxy,
        dominant_surface_family=SURFACE_FAMILIES[int(decoded.dominant_surface_family)],
        source_rev=source_rev)


@dataclass
class SurfaceInputs:
    """Authority-agnostic composition inputs (the single-writer owners feed THESE).
    Macro derivation and fine MW composition both build a SurfaceInputs, so the exposure
    precedence law is identical in both regimes and in the fixtures."""
    lithology: str            # host bedrock lithology (MW2 / macro geology proxy)
    deposit_present: float    # [0,1] exposed depositional-body strength (MW5 / drainage)
    deposit_kind: str         # '' or alluvium/floodplain_sediment/basin_fill/colluvium/talus
    regolith_depth: float     # [0,1] soil/regolith profile depth (MW7 / age)
    weathering: float         # [0,1] maturity (age / erosion)
    wetness: float            # [0,1] saturation/climate state (MW6 / macro proxy)
    organic_potential: float  # [0,1] ecological tendency (MW8 / macro proxy) -- input, not flora
    exposure: float           # [0,1] bare-rock fraction (slope / relief)
    stability: float          # [0,1] slope stability
    volcanic: float           # [0,1] volcanic-ancestry strength (basalt family)
    volcanic_age: float       # [0,1] 0 fresh .. 1 old-weathered volcanic
    grain: float = -1.0       # [0,1] override; <0 => use substrate default


# thresholds for the exposure-precedence law (documented, deterministic)
_T_DEPOSIT = 0.42
_T_REGOLITH = 0.34
_T_ORGANIC = 0.55
_T_WEATHER = 0.45
_T_WET_LOG = 0.66      # saturation above which a fine basin/floodplain becomes waterlogged mineral


def compose_surface_state(inp: SurfaceInputs, source_rev: str = "") -> SurfaceState:
    """LAW A -- exposure precedence: resolve WHAT IS AT THE SURFACE so a point never gets
    four different materials depending on which owner was queried. Then apply state overlays.

        1. exposed depositional body   (MW5 / drainage)
        2. regolith / soil profile     (MW7 / age)
        3. weathered parent material   (age / erosion)
        4. host bedrock lithology      (MW2)
    then STATE overlays: wetness -> waterlogged promotion; organic -> organic_capable;
    plus exposure/weathering/stability carried as axes.
    A volcanic-ancestry province remaps the chosen bedrock/weathered/regolith tiers to the
    shared VOLCANIC substrates (fresh_lava/weathered_basalt/volcanic_soil) -- same vocabulary,
    same precedence, so volcanic terrain is not a special-case parallel path."""
    volcanic = inp.volcanic >= 0.5
    lithology = "basalt" if volcanic else inp.lithology

    # ---- precedence: choose the exposed substrate tier -------------------- #
    if inp.deposit_present >= _T_DEPOSIT and inp.deposit_kind:
        substrate = inp.deposit_kind
        # extreme saturation in a fine valley/basin deposit -> waterlogged mineral surface
        if (inp.wetness >= _T_WET_LOG
                and substrate in ("floodplain_sediment", "basin_fill", "alluvium")
                and inp.organic_potential < _T_ORGANIC):
            substrate = "waterlogged_mineral"
    elif inp.regolith_depth >= _T_REGOLITH:
        # a deep, wet, ecologically-capable regolith is an organic-capable surface
        if inp.organic_potential >= _T_ORGANIC and inp.regolith_depth >= 0.5 and inp.wetness >= 0.35:
            substrate = "organic_capable"
        else:
            substrate = "thin_regolith"
    elif inp.weathering >= _T_WEATHER:
        substrate = "weathered_bedrock"
    else:
        substrate = "bare_bedrock"

    # ---- volcanic remap (shared vocabulary; NOT a parallel path) ---------- #
    if volcanic:
        fam0 = _SUBSTRATE_FAMILY[substrate]
        if fam0 == "sediment":
            pass                                   # reworked volcaniclastic sediment stays sediment
        elif substrate == "organic_capable":
            substrate = "volcanic_soil"
        elif fam0 == "regolith":
            substrate = "volcanic_soil" if inp.volcanic_age >= 0.55 else "scoria_ash"
        else:                                      # rock tier
            if inp.volcanic_age < 0.30:
                substrate = "fresh_lava"
            elif inp.volcanic_age < 0.62:
                substrate = "weathered_basalt"
            else:
                substrate = "volcanic_soil" if inp.regolith_depth >= 0.28 else "weathered_basalt"

    grain = inp.grain if inp.grain >= 0.0 else _SUBSTRATE_GRAIN[substrate]
    family = _SUBSTRATE_FAMILY[substrate]
    return SurfaceState(
        substrate_class=substrate, lithology_class=lithology,
        wetness=min(1.0, max(0.0, inp.wetness)),
        weathering=min(1.0, max(0.0, inp.weathering)),
        soil_depth=min(1.0, max(0.0, inp.regolith_depth)),
        stability=min(1.0, max(0.0, inp.stability)),
        organic_potential=min(1.0, max(0.0, inp.organic_potential)),
        exposure=min(1.0, max(0.0, inp.exposure)),
        roughness_proxy=min(1.0, max(0.0, grain)),
        dominant_surface_family=family, source_rev=source_rev)


def _macro_lithology(ctl: Controls, volcanic_anc: float) -> str:
    """Macro host lithology proxy from the morphology style controls. Returns
    'mixed_unknown' where the macro authority cannot honestly distinguish."""
    if volcanic_anc >= 0.5:
        return "basalt"
    w = {"belt": ctl.w_belt, "plateau": ctl.w_plateau,
         "volcanic": ctl.w_volcanic, "cratonic": ctl.w_cratonic}
    dom = max(w, key=w.get)
    if w[dom] < 0.40:
        return "mixed_unknown"
    if dom == "volcanic":
        return "basalt"
    if dom == "plateau":                           # layered sedimentary tableland
        if ctl.substrate >= 0.55:
            return "sandstone"                     # competent bench-forming cap
        return "limestone" if ctl.age >= 0.55 else "shale"
    if dom == "belt":                              # orogenic belt
        if ctl.substrate >= 0.62:
            return "quartzite"
        return "granite" if ctl.age < 0.55 else "metamorphic"
    # cratonic shield / plains
    return "granite" if ctl.substrate >= 0.5 else "shale"


def _macro_surface_slope(central: "CentralProgram", fieldf: "MacroField",
                         x: float, y: float, h: float = 250.0) -> float:
    """Local macro surface slope (m/m) by central difference of the macro surface. Cheap
    analytic (a few macro_z evals); NEVER the fine causal stack."""
    zx1 = macro_z_incised(central, fieldf, x + h, y)
    zx0 = macro_z_incised(central, fieldf, x - h, y)
    zy1 = macro_z_incised(central, fieldf, x, y + h)
    zy0 = macro_z_incised(central, fieldf, x, y - h)
    gx = (zx1 - zx0) / (2.0 * h)
    gy = (zy1 - zy0) / (2.0 * h)
    return math.hypot(gx, gy)


def _macro_place(central: "CentralProgram", fieldf: "MacroField", x: float, y: float):
    """Shared per-point macro environment (landform ancestry + drainage + z + slope), computed
    ONCE and reused by both the SurfaceState and WaterState derivations (they otherwise each
    recompute z/slope/drainage/landform — the dominant page-compile cost)."""
    landform = landform_at(central, fieldf, x, y)
    wid, cid, accum, incision = drainage_query(central, fieldf, x, y)
    z = macro_z_incised(central, fieldf, x, y)
    slope = _macro_surface_slope(central, fieldf, x, y)
    return landform, wid, cid, accum, incision, z, slope


def _macro_surface_inputs(central: "CentralProgram", fieldf: "MacroField",
                          x: float, y: float, ctl: "Controls | None" = None,
                          place=None) -> SurfaceInputs:
    """Derive authority-agnostic SurfaceInputs from the MACRO controls + drainage + landform
    ancestry. Cheap analytic macro proxies of the fine MW authorities; the far PROMISE that
    later detailed generation must refine, never contradict."""
    seed = fieldf.seed
    if ctl is None:
        ctl = controls_at(seed, x, y)
    if place is None:
        place = _macro_place(central, fieldf, x, y)
    (cls, lid, par, lf_age, lf_sub, _), wid, cid, accum, incision, z, slope = place

    # The macro SurfaceState is the OUTSIDE promise; inside the frozen +/-32 km centre the
    # fine MW authority owns the surface (MS1.B). Defer to it: the anchor window (0 in the
    # centre, 1 beyond 64 km) scales every province-specific macro signal so the macro
    # family transitions continuously to the frozen central family and never CONTRADICTS it.
    aw = anchor_window(x, y)

    # volcanic ancestry: the smooth volcanic-potential province OR a B2 volcanic landform.
    # Scaled by the anchor window so the sedimentary frozen centre is never painted basaltic.
    volc_land = cls in ("volcanic_shield", "volcanic_cone", "volcanic_plug")
    volcanic = 0.0
    volcanic_age = ctl.age
    if ctl.volcanic >= 0.80 or volc_land:
        volcanic = smoothstep(0.72, 0.86, ctl.volcanic)
        if volc_land:
            volcanic = max(volcanic, 0.85)
            volcanic_age = lf_age                   # plug = old, shield/cone = young (ancestry)
    volcanic *= aw
    # bare, resistant erosional survivors read as rock regardless of province (gated by the
    # anchor window, since special landforms do not exist in the frozen centre either).
    bare_landform = aw > 0.15 and cls in ("volcanic_plug", "tower", "mesa", "butte")

    # ---- macro hydroclimate proxy (NOT fine MW6): broad humidity band + drainage + lowland.
    humidity = _control(seed + ":macro_humidity", x, y, 520000.0)
    lowland = smoothstep(1400.0, 0.0, z)            # valleys wetter, high ground drier (macro proxy)
    near_flow = smoothstep(160.0, 900.0, accum)     # concentrated flow corridors are wetter
    endorheic = smoothstep(900.0, 1500.0, accum) * lowland
    wetness = min(1.0, 0.15 + 0.55 * humidity + 0.35 * near_flow + 0.30 * lowland)
    wetness = max(0.0, wetness - 0.35 * smoothstep(0.10, 0.45, slope))   # steep drains fast

    # ---- exposure (bare-rock fraction): slope + young/high-relief + bare landforms.
    exposure = smoothstep(0.03, 0.34, slope)
    exposure = max(exposure, (1.0 - ctl.age) * smoothstep(0.4, 0.9, ctl.relief) * 0.7)
    if bare_landform:
        exposure = max(exposure, 0.75)
    exposure = min(1.0, exposure)

    # ---- regolith depth (MW7 proxy): old + gentle + wet + not-exposed accumulates soil.
    regolith = ctl.age * (1.0 - exposure) * (0.45 + 0.55 * wetness)
    regolith = min(1.0, regolith * (1.0 + 0.4 * smoothstep(0.06, 0.0, slope)))
    if bare_landform:
        regolith = min(regolith, 0.12)

    # ---- weathering / maturity (age, humid-boosted).
    weathering = min(1.0, ctl.age * (0.7 + 0.5 * wetness))

    # ---- depositional body (MW5 / drainage): valley alluvium, basin fill, talus, colluvium.
    deposit_present = 0.0
    deposit_kind = ""
    valley = smoothstep(0.05, 0.0, slope) * near_flow
    basin = smoothstep(0.035, 0.0, slope) * endorheic
    if basin >= 0.45 and basin >= valley:
        deposit_present, deposit_kind = basin, "basin_fill"
    elif valley >= 0.40:
        deposit_present = valley
        deposit_kind = "floodplain_sediment" if slope < 0.015 else "alluvium"
    elif slope >= 0.22 and ctl.substrate >= 0.5:
        # steep competent flank at the foot of relief -> talus/colluvium apron
        deposit_present = smoothstep(0.22, 0.5, slope)
        deposit_kind = "talus" if ctl.substrate >= 0.62 else "colluvium"
    elif 0.08 <= slope < 0.22 and ctl.age >= 0.4:
        deposit_present = 0.5 * smoothstep(0.08, 0.22, slope)
        deposit_kind = "colluvium"

    # ---- organic potential (MW8 proxy): wet + soil + temperate + not-bare.
    temperate = 1.0 - smoothstep(1700.0, 3200.0, z)       # cold barren above treeline (macro proxy)
    organic = wetness * (0.35 + 0.65 * regolith) * temperate * (1.0 - 0.7 * exposure)
    organic = min(1.0, organic * 1.3)

    stability = min(1.0, (1.0 - exposure) * (0.5 + 0.5 * smoothstep(0.25, 0.0, slope)))

    lithology = _macro_lithology(ctl, volcanic)
    if aw < 0.5:                                     # anchored centre: defer host rock to the
        cfam = central_surface_family(central, x, y) # frozen central family (compatibility, not
        lithology = {"rock": "granite", "sediment": "shale",   # a render -- MS1.B fine owns it)
                     "regolith": "granite"}[cfam]
    return SurfaceInputs(
        lithology=lithology, deposit_present=deposit_present, deposit_kind=deposit_kind,
        regolith_depth=regolith, weathering=weathering, wetness=wetness,
        organic_potential=organic, exposure=exposure, stability=stability,
        volcanic=volcanic, volcanic_age=volcanic_age)


def surface_state_at(central: "CentralProgram", fieldf: "MacroField",
                     x: float, y: float, ctl: "Controls | None" = None,
                     place=None) -> SurfaceState:
    """MACRO SurfaceState at an absolute coordinate: derive inputs from MV3 controls +
    drainage + landform, then apply the shared exposure-precedence composition law."""
    inp = _macro_surface_inputs(central, fieldf, x, y, ctl, place)
    return compose_surface_state(inp, source_rev=f"gv{GENERATOR_VERSION}.sd{SURFACE_DESCRIPTOR_VERSION}")


def central_surface_family(central: "CentralProgram", x: float, y: float) -> str:
    """Coarse dominant-family PROXY of the frozen central envelope (belt uplift = rock,
    basin = sediment, hinterland = regolith). Used only to prove macro/fine compatibility at
    the +/-32 km boundary (it must not CONTRADICT the macro family), never to render."""
    p = central
    a = p.belt_azimuth_deg * math.pi / 180.0
    ux, uy = math.cos(a), math.sin(a)
    vx, vy = -math.sin(a), math.cos(a)
    along = x * ux + y * uy
    across = x * vx + y * vy
    belt = (smoothstep(p.belt_half_length_m, p.belt_half_length_m - 4000.0, abs(along))
            * smoothstep(p.belt_half_width_m, p.belt_half_width_m - 2200.0, abs(across)))
    d_basin = across - p.basin_center_across_m
    basin = (math.exp(-0.5 * (d_basin / p.basin_half_width_m) ** 2)
             * smoothstep(p.basin_half_length_m, p.basin_half_length_m - 3500.0, abs(along)))
    if belt > 0.35:
        return "rock"
    if basin > 0.35:
        return "sediment"
    return "regolith"


# =========================================================================== #
# WD1.A — WaterState authority (Python world-authority)
# =========================================================================== #
#
# CORE LAW: CHANNEL_EXISTS != WATER_PRESENT != WATER_BODY_TYPE != WATER_OPTICAL_STATE.
# The MV3.B1 drainage graph says where water CAN travel; it never asserts water is there now.
# Presence is a TWO-STAGE causal gate: (1) hydrologic SUPPLY (climate + catchment − losses)
# then (2) ACCOMMODATION (channel gradient / basin closure) — so a high-supply place becomes a
# river on a slope but a lake in a closed basin, and an arid high-permeability place leaves the
# same channel dry. Appearance is downstream (WD1.B); WaterState stores NO RGB / shader params.
# MASS LAW: this is an environmental PROMISE, not a conserved water ledger — it adds no grams to
# 16D/16F and changes no terrain geometry. GUARDRAIL: supply consumes environmental wetness
# POTENTIAL (climate/substrate), never WaterState itself (no circular input). MS1 owns the
# bottom substrate; water OCCUPIES it (recoverable through shallow water in WD1.B).

WATER_DESCRIPTOR_VERSION = descriptor_schema.WATER_ENCODING_VERSION

# presence regimes (ordered dry..standing); body/regime families (consequences, not presets);
# flow regimes (hydrologic context only, no fluid sim). Index order = on-page code; append-only.
PRESENCE_REGIMES = list(descriptor_schema.WATER_PRESENCE_NAMES)
BODY_CLASSES = list(descriptor_schema.WATER_BODY_NAMES)
FLOW_REGIMES = list(descriptor_schema.WATER_FLOW_NAMES)

# Macro-water authority SCOPE: whether the macro descriptor OWNS the water conclusion here, or
# defers to the frozen detailed authority (16D-16F). DEFER_TO_DETAILED is NOT `dry` — `dry` is a
# real hydrologic state, whereas defer means "macro does not assert; the detailed system owns
# this location". A consumer must check the scope BEFORE reading presence, so a detailed lake in
# the frozen centre is never read as contradicting a macro `dry`.
MACRO_WATER_AUTHORITY = list(descriptor_schema.WATER_AUTHORITY_NAMES)

_PRESENCE_IDX = {n: i for i, n in enumerate(PRESENCE_REGIMES)}
_BODY_IDX = {n: i for i, n in enumerate(BODY_CLASSES)}
_FLOW_IDX = {n: i for i, n in enumerate(FLOW_REGIMES)}
_WAUTH_IDX = {n: i for i, n in enumerate(MACRO_WATER_AUTHORITY)}

# substrate hydrologic properties (from the MS1 substrate class): permeability = infiltration
# loss (bedrock sheds/ponds, sand/scoria drains); erodibility = sediment supply to the water.
_SUBSTRATE_PERMEABILITY = {
    "bare_bedrock": 0.05, "weathered_bedrock": 0.20, "thin_regolith": 0.40, "colluvium": 0.50,
    "talus": 0.85, "alluvium": 0.70, "floodplain_sediment": 0.45, "basin_fill": 0.40,
    "organic_capable": 0.35, "waterlogged_mineral": 0.08, "fresh_lava": 0.78, "scoria_ash": 0.82,
    "weathered_basalt": 0.30, "volcanic_soil": 0.42,
}
_SUBSTRATE_ERODIBILITY = {
    "bare_bedrock": 0.05, "weathered_bedrock": 0.25, "thin_regolith": 0.50, "colluvium": 0.60,
    "talus": 0.40, "alluvium": 0.85, "floodplain_sediment": 0.92, "basin_fill": 0.70,
    "organic_capable": 0.50, "waterlogged_mineral": 0.60, "fresh_lava": 0.15, "scoria_ash": 0.60,
    "weathered_basalt": 0.40, "volcanic_soil": 0.55,
}


@dataclass
class WaterState:
    """Compact SEMANTIC water record at a point. NO stored RGB, NO shader coefficients as
    authority. Depth is continuous metres (bottom/surface elevations kept). References the MS1
    bottom substrate; carries B1 hydrologic ancestry."""
    presence_regime: str
    body_class: str
    flow_regime: str
    depth_m: float
    bottom_elev_m: float
    surface_elev_m: float
    discharge_proxy: float        # water_supply_index (macro hydrologic forcing; NOT water mass)
    mean_supply: float
    seasonality_index: float
    persistence_margin: float
    clarity: float
    turbidity: float
    suspended_sediment: float
    mineral_load: float
    organic_load: float
    temperature_proxy: float
    bottom_family: str            # MS1 dominant_surface_family beneath the water
    waterfall_potential: float    # reserved hook (WD1.C); classification only
    mineral_potential: float      # reserved hook (volcanic/mineral chemistry later)
    macro_authority: str = "valid_macro"   # valid_macro | defer_to_detailed (16D-16F owns it)
    macro_watershed_id: str = ""
    macro_channel_id: str = ""
    macro_water_body_id: str = ""
    water_regime_id: str = ""
    source_rev: str = ""

    def has_water(self) -> bool:
        return self.presence_regime not in ("dry",)

    def pack(self) -> int:
        """Pack to an integer (~60 bits; hex on the page). Layout documented on the page."""
        return descriptor_schema.encode_water(
            _PRESENCE_IDX[self.presence_regime], _BODY_IDX[self.body_class],
            _FLOW_IDX[self.flow_regime], _FAMILY_IDX[self.bottom_family], self.depth_m,
            self.discharge_proxy, self.seasonality_index, self.clarity, self.turbidity,
            self.suspended_sediment, self.mineral_load, self.organic_load,
            self.temperature_proxy, self.waterfall_potential, self.mineral_potential,
            _WAUTH_IDX[self.macro_authority], version=WATER_DESCRIPTOR_VERSION)


def unpack_water(v: int, source_rev: str = "") -> WaterState:
    decoded = descriptor_schema.decode_water(WATER_DESCRIPTOR_VERSION, v)
    # The legacy WaterState carrier still stores the dry wire placeholder on a
    # deferred cell. New schema consumers must use decoded.presence, which is None.
    return WaterState(
        presence_regime=PRESENCE_REGIMES[int(decoded.encoded_presence)],
        body_class=BODY_CLASSES[int(decoded.body_class)],
        flow_regime=FLOW_REGIMES[int(decoded.flow_regime)], depth_m=decoded.depth_m,
        bottom_elev_m=0.0, surface_elev_m=decoded.depth_m,
        discharge_proxy=decoded.discharge_proxy, mean_supply=decoded.discharge_proxy,
        seasonality_index=decoded.seasonality_index, persistence_margin=0.0,
        clarity=decoded.clarity, turbidity=decoded.turbidity,
        suspended_sediment=decoded.suspended_sediment, mineral_load=decoded.mineral_load,
        organic_load=decoded.organic_load, temperature_proxy=decoded.temperature_proxy,
        bottom_family=SURFACE_FAMILIES[int(decoded.bottom_family)],
        waterfall_potential=decoded.waterfall_potential,
        mineral_potential=decoded.mineral_potential,
        macro_authority=MACRO_WATER_AUTHORITY[int(decoded.macro_authority)],
        source_rev=source_rev)


def _clip01(v: float) -> float:
    return 0.0 if v < 0.0 else (1.0 if v > 1.0 else v)


def water_supply(humidity: float, continentality: float, relief: float, z: float,
                 accum: float, permeability: float):
    """STAGE 1 — hydrologic supply (climate + catchment − losses). Returns
    (discharge_proxy, mean_supply, seasonality_index, persistence_margin). Consumes only
    environmental POTENTIAL (climate/substrate); NEVER WaterState (no circular input)."""
    aridity = 1.0 - humidity
    warm = 1.0 - smoothstep(1600.0, 3200.0, z)                  # low warm evaporates, high cold less
    evap_loss = aridity * (0.40 + 0.60 * warm)
    orographic = 0.15 * relief
    climate_supply = _clip01(humidity + orographic - 0.45 * evap_loss)   # local moisture availability
    catchment = smoothstep(15.0, 700.0, accum)                  # upstream integrated area (discharge)
    discharge = _clip01(climate_supply * (0.30 + 0.80 * catchment) - 0.28 * permeability * (1.0 - catchment))
    seasonality = _clip01(0.18 + 0.50 * aridity + 0.40 * continentality - 0.20 * catchment)
    persistence = _clip01(discharge - 0.35 * seasonality)
    return discharge, climate_supply, seasonality, persistence


def water_presence_body(discharge: float, seasonality: float, persistence: float,
                        slope: float, permeability: float, is_channel: bool,
                        closed_basin: bool, accommodation: float, accom_depth: float,
                        organic: float, volcanic: float, z: float):
    """STAGE 2 — accommodation. Given supply, decide presence regime, body family, flow regime
    and a depth hint. Standing (basin) vs flowing (channel) vs wet-ground vs dry, from geometry.
    A high-supply place is NOT automatically a lake."""
    # ---- presence gate (persistence-based) -------------------------------- #
    if persistence >= 0.34:
        presence = "perennial"
    elif discharge >= 0.28 and seasonality >= 0.42:
        presence = "seasonal"
    elif discharge >= 0.15:
        presence = "ephemeral"
    elif discharge >= 0.07 and permeability < 0.45:
        presence = "damp_substrate"
    else:
        presence = "dry"

    standing = accommodation >= 0.45 and accom_depth > 8.0 and discharge >= 0.14

    # ---- body / regime ---------------------------------------------------- #
    if standing:
        presence = "perennial" if persistence >= 0.30 else ("seasonal" if presence in ("seasonal", "ephemeral") else presence)
        presence = "standing" if persistence >= 0.20 else presence
        if volcanic >= 0.55 and closed_basin:
            body = "crater_lake" if accom_depth > 60.0 else "volcanic_mineral_pool"
        elif closed_basin:
            body = "closed_basin_lake"
        elif z > 1500.0:
            body = "alpine_lake"
        elif organic >= 0.5 and accom_depth < 25.0:
            body = "organic_darkwater" if organic >= 0.62 else "wetland_marsh"
        else:
            body = "floodplain_water" if accom_depth < 18.0 else "closed_basin_lake"
        flow = "still" if accom_depth > 20.0 else "slow"
    elif is_channel and presence in ("perennial", "seasonal", "ephemeral"):
        if presence in ("seasonal", "ephemeral"):
            body = "arid_wash"
            flow = "channelized"
        elif slope < 0.012 and permeability < 0.5:
            body = "floodplain_water"
            flow = "slow"
        elif discharge >= 0.55 and slope < 0.05:
            body = "sediment_river" if permeability >= 0.4 else "perennial_river"
            flow = "channelized"
        elif slope >= 0.10:
            body = "headwater_stream"
            flow = "fast" if slope < 0.24 else "turbulent"
        else:
            body = "perennial_river"
            flow = "channelized"
    elif presence == "damp_substrate" and slope < 0.02 and organic >= 0.45:
        presence = "damp_substrate"
        body = "wetland_marsh"
        flow = "still"
    else:
        # not a channel and no accommodation: hillslope — spring proxy only, else no body
        if presence in ("perennial", "seasonal") and slope < 0.06 and permeability < 0.3:
            body, flow = "spring_pool", "slow"
        else:
            body = "none"
            flow = "none"
            if presence in ("perennial", "seasonal", "ephemeral"):
                presence = "damp_substrate"    # supply exists but nowhere to collect/flow
    return presence, body, flow


def _water_optics(body: str, discharge: float, accum: float, erodibility: float,
                  organic: float, volcanic: float, closed_basin: bool, z: float, depth: float):
    """Optical-state axes (physical inputs for WD1.B): clarity/turbidity/sediment/mineral/
    organic/temperature. Consequences of setting, not a palette."""
    flow_energy = smoothstep(30.0, 900.0, accum)
    turbidity = _clip01(erodibility * (0.25 + 0.75 * flow_energy) * (0.4 + 0.9 * discharge))
    if body in ("headwater_stream", "alpine_lake", "spring_pool"):
        turbidity *= 0.25                                       # clear cold/resistant headwaters
    if body in ("closed_basin_lake", "crater_lake") and not volcanic:
        turbidity *= 0.5
    sediment = _clip01(turbidity * (0.5 + 0.6 * erodibility))
    organic_load = _clip01(organic * (0.3 + 0.9 * (1.0 if body in ("wetland_marsh", "organic_darkwater") else 0.25))
                           * (0.5 + 0.6 * smoothstep(20.0, 0.0, depth if depth > 0 else 0.0)))
    mineral = _clip01(volcanic * (0.3 + 0.9 * (1.0 if closed_basin else 0.4)))
    if body in ("volcanic_mineral_pool", "crater_lake"):
        mineral = _clip01(mineral + 0.35)
    temperature = _clip01(1.0 - smoothstep(200.0, 2800.0, z))   # warm low, cold high (state-relevant)
    clarity = _clip01(1.0 - 0.85 * turbidity - 0.4 * organic_load)
    return clarity, turbidity, sediment, mineral, organic_load, temperature


def _basin_context(central: "CentralProgram", fieldf: "MacroField", x: float, y: float, z: float):
    """Cheap concavity probe: (closed_basin, accommodation_depth_m). Closed if the terrain rises
    on all four 5 km probes (an enclosed depression); accommodation depth = rim − floor. Only a
    few extra macro_z evals; NEVER the fine causal stack."""
    r = 5000.0
    zs = [macro_z_incised(central, fieldf, x + r, y), macro_z_incised(central, fieldf, x - r, y),
          macro_z_incised(central, fieldf, x, y + r), macro_z_incised(central, fieldf, x, y - r)]
    rises = sum(1 for zz in zs if zz > z + 3.0)
    accom_depth = max(0.0, min(zs) - z)
    return rises >= 4, accom_depth


def water_state_at(central: "CentralProgram", fieldf: "MacroField", x: float, y: float,
                   ss: "SurfaceState | None" = None, ctl: "Controls | None" = None,
                   place=None) -> WaterState:
    """MACRO WaterState at an absolute coordinate: two-stage presence (supply → accommodation)
    over MV3.B1 drainage + MS1 substrate + macro hydroclimate proxies. Anchor-window gated
    (dry in the frozen ±32 km centre, where detailed 16D–16F water is authoritative). Cheap
    analytic; creates no water mass, changes no geometry."""
    seed = fieldf.seed
    if ctl is None:
        ctl = controls_at(seed, x, y)
    if place is None:
        place = _macro_place(central, fieldf, x, y)
    (cls, _lid, _par, _lfa, _lsub, _lc), wid, cid, accum, incision, z, slope = place
    rev = f"gv{GENERATOR_VERSION}.wd{WATER_DESCRIPTOR_VERSION}"
    aw = anchor_window(x, y)
    if ss is None:
        ss = surface_state_at(central, fieldf, x, y, ctl, place)
    if aw <= 0.0:
        # frozen centre: the macro authority does NOT assert water here — it DEFERS to the
        # detailed 16D-16F system. This is NOT `dry` (a real hydrologic state); a consumer must
        # read macro_authority first, so a detailed lake here is never a contradiction.
        return WaterState("dry", "none", "none", 0.0, z, z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                          0.0, 0.0, 0.0, ss.dominant_surface_family, 0.0, 0.0,
                          macro_authority="defer_to_detailed", source_rev=rev)

    perm = _SUBSTRATE_PERMEABILITY.get(ss.substrate_class, 0.4)
    erod = _SUBSTRATE_ERODIBILITY.get(ss.substrate_class, 0.5)
    humidity = _control(seed + ":macro_humidity", x, y, 520000.0)
    continentality = _control(seed + ":macro_contin", x, y, 720000.0)
    volcanic = smoothstep(0.72, 0.90, ctl.volcanic)
    if cls in ("volcanic_shield", "volcanic_cone", "volcanic_plug"):
        volcanic = max(volcanic, 0.7)
    volcanic *= aw

    discharge, mean_supply, seasonality, persistence = water_supply(
        humidity, continentality, ctl.relief, z, accum, perm)

    # accommodation: flat + (closed basin OR lowland pooling); cheap concavity for closed basins
    is_channel = cid != "none"
    flatness = smoothstep(0.030, 0.0, slope)
    closed_basin, accom_depth = (False, 0.0)
    lowland = smoothstep(400.0, -120.0, z)
    if flatness > 0.2:                          # only probe basins where it could pond (cheap gate)
        closed_basin, accom_depth = _basin_context(central, fieldf, x, y, z)
        if not closed_basin:
            accom_depth = max(accom_depth, 30.0 * lowland * flatness)   # broad low pooling
    accommodation = flatness * (0.6 * (1.0 if closed_basin else 0.0) + 0.6 * lowland)

    presence, body, flow = water_presence_body(
        discharge, seasonality, persistence, slope, perm, is_channel,
        closed_basin, accommodation, accom_depth, ss.organic_potential, volcanic, z)

    # depth (continuous metres): standing = basin accommodation; channel = discharge-scaled.
    if body in ("closed_basin_lake", "alpine_lake", "crater_lake", "volcanic_mineral_pool",
                "floodplain_water", "wetland_marsh", "organic_darkwater"):
        depth = min(120.0, accom_depth * (0.4 + 0.6 * discharge))
        if body in ("wetland_marsh", "organic_darkwater"):
            depth = min(depth, 3.5)             # wetlands are shallow
    elif body in ("perennial_river", "sediment_river", "braided_reach", "headwater_stream",
                  "arid_wash", "spring_pool"):
        depth = (0.2 + 6.0 * discharge) * (0.5 + 0.5 * smoothstep(30.0, 900.0, accum))
        if body == "arid_wash":
            depth = 0.0 if presence in ("ephemeral", "seasonal", "dry") else depth * 0.3
        depth = min(9.0, depth)
    else:
        depth = 0.0

    clarity, turbidity, sediment, mineral, organic_load, temperature = _water_optics(
        body, discharge, accum, erod, ss.organic_potential, volcanic, closed_basin, z, depth)

    # identity / ancestry (absolute-coordinate; window-independent via B1 ids)
    water_body_id = "none"
    regime_id = "none"
    if body != "none":
        if body in ("closed_basin_lake", "alpine_lake", "crater_lake", "volcanic_mineral_pool",
                    "wetland_marsh", "organic_darkwater", "floodplain_water"):
            # key the standing body to its canonical watershed sink identity (from B1)
            water_body_id = f"{fnv1a64(seed + ':mwbody:' + wid):016x}"
        regime_id = f"{fnv1a64(seed + f':mregime:{body}:' + (cid if cid != 'none' else wid)):016x}"

    return WaterState(
        presence_regime=presence, body_class=body, flow_regime=flow,
        depth_m=depth, bottom_elev_m=z, surface_elev_m=z + depth,
        discharge_proxy=discharge, mean_supply=mean_supply, seasonality_index=seasonality,
        persistence_margin=persistence, clarity=clarity, turbidity=turbidity,
        suspended_sediment=sediment, mineral_load=mineral, organic_load=organic_load,
        temperature_proxy=temperature, bottom_family=ss.dominant_surface_family,
        waterfall_potential=_clip01(smoothstep(0.18, 0.5, slope) * discharge
                                    * (1.0 if is_channel else 0.0) * (0.4 + 0.6 * ctl.substrate)),
        mineral_potential=_clip01(volcanic * (0.5 if closed_basin else 0.25)),
        macro_watershed_id=wid, macro_channel_id=cid,
        macro_water_body_id=water_body_id, water_regime_id=regime_id, source_rev=rev)


# --------------------------------------------------------------------------- #
# Compiled macro PAGES (64 km serialization cells)
# --------------------------------------------------------------------------- #

PAGE_STEP_M = 1000.0        # coarse macro sample step (macro silhouette, not grooves)
SURFACE_STEP_M = 4000.0     # coarse SurfaceState descriptor step (state varies on 200-500 km
                            # control wavelengths, so a 4 km grid is ample; keeps pages small)


def region_id(central: CentralProgram, ri: int, rj: int) -> str:
    return f"{fnv1a64(central.region_key + ':macro:' + central.seed + f':({ri},{rj})'):016x}"


@dataclass
class MacroPage:
    ri: int
    rj: int
    min_x: float
    min_y: float
    step: float
    n: int
    heights: list[float]              # row-major (n x n)
    region_id: str
    world_seed: str
    generator_version: int
    world_identity_hash: str
    source_digest: str
    neighbor_lineage: dict[str, str]  # direction -> region_id
    surface_step: float = 0.0             # MS1.A SurfaceState descriptor grid step (m)
    surface_n: int = 0                    # descriptor grid dimension
    surface_codes: list[int] = field(default_factory=list)   # packed SurfaceState per cell
    surface_digest: str = "0"             # deterministic digest of the descriptor grid
    water_step: float = 0.0               # WD1.A WaterState descriptor grid step (m)
    water_n: int = 0
    water_codes: list[int] = field(default_factory=list)     # packed WaterState per cell
    water_digest: str = "0"


def compile_page(central: CentralProgram, fieldf: MacroField, ri: int, rj: int,
                 step: float = PAGE_STEP_M) -> MacroPage:
    min_x = ri * REGION_M - REGION_HALF_M
    min_y = rj * REGION_M - REGION_HALF_M
    n = int(round(REGION_M / step)) + 1
    heights: list[float] = []
    for j in range(n):
        y = min_y + j * step
        for i in range(n):
            x = min_x + i * step
            heights.append(macro_z_incised(central, fieldf, x, y))
    # Deterministic content digest over quantized heights (cm precision).
    hh = _FNV_OFFSET
    for z in heights:
        q = int(round(z * 100.0)) & 0xFFFFFFFFFFFFFFFF
        for shift in (0, 8, 16, 24, 32, 40):
            hh ^= (q >> shift) & 0xFF
            hh = (hh * _FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    dirs = {"n": (0, 1), "s": (0, -1), "e": (1, 0), "w": (-1, 0),
            "ne": (1, 1), "nw": (-1, 1), "se": (1, -1), "sw": (-1, -1)}
    lineage = {d: region_id(central, ri + dx, rj + dy) for d, (dx, dy) in dirs.items()}

    # ---- MS1.A SurfaceState + WD1.A WaterState descriptor grids (compact semantic; NO colour) #
    # Computed in ONE cell loop so the WaterState reuses the SurfaceState (which owns the
    # substrate beneath the water) instead of recomputing the expensive MS1 derivation.
    sn = int(round(REGION_M / SURFACE_STEP_M)) + 1
    surf_codes: list[int] = []
    water_codes: list[int] = []
    for j in range(sn):
        y = min_y + j * SURFACE_STEP_M
        for i in range(sn):
            x = min_x + i * SURFACE_STEP_M
            ctl = controls_at(fieldf.seed, x, y)
            place = _macro_place(central, fieldf, x, y)      # shared env (computed once)
            ss = surface_state_at(central, fieldf, x, y, ctl, place)
            surf_codes.append(ss.pack())
            water_codes.append(water_state_at(central, fieldf, x, y, ss=ss, ctl=ctl, place=place).pack())

    def _grid_digest(codes, shifts):
        d = _FNV_OFFSET
        for code in codes:
            for shift in shifts:
                d ^= (code >> shift) & 0xFF
                d = (d * _FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
        return f"{d:016x}"

    return MacroPage(
        ri=ri, rj=rj, min_x=min_x, min_y=min_y, step=step, n=n, heights=heights,
        region_id=region_id(central, ri, rj), world_seed=fieldf.seed,
        generator_version=GENERATOR_VERSION,
        world_identity_hash=central.world_identity_hash,
        source_digest=f"{hh:016x}", neighbor_lineage=lineage,
        surface_step=SURFACE_STEP_M, surface_n=sn, surface_codes=surf_codes,
        surface_digest=_grid_digest(surf_codes, (0, 8, 16, 24, 32)),
        water_step=SURFACE_STEP_M, water_n=sn, water_codes=water_codes,
        water_digest=_grid_digest(water_codes, (0, 8, 16, 24, 32, 40, 48, 56)))


def serialize_page(page: MacroPage) -> str:
    lines = [
        PAGE_MAGIC,
        "# MV2.A regional macro authority page. Macro forcing only; not fine terrain.",
        "# Regenerate with Tools/Worldgen/cert_macro_authority.py",
        f"region_id={page.region_id}",
        f"region_cell={page.ri},{page.rj}",
        f"world_seed={page.world_seed}",
        f"generator_version={page.generator_version}",
        f"world_identity_hash={page.world_identity_hash}",
        f"region_min_x_m={page.min_x:.1f}",
        f"region_min_y_m={page.min_y:.1f}",
        f"region_size_m={REGION_M:.1f}",
        f"sample_step_m={page.step:.1f}",
        f"grid_n={page.n}",
        f"source_digest={page.source_digest}",
        *(f"neighbor_{d}={rid}" for d, rid in page.neighbor_lineage.items()),
        "no_wrap_into_region=1",
        "cheap_source=macro_analytic_no_reconstructedz",
        f"height_grid_row_major={' '.join(f'{z:.2f}' for z in page.heights)}",
        # MS1.A SurfaceState descriptor: compact SEMANTIC surface identity per coarse cell
        # (substrate/lithology/family + quantised state axes packed into one integer). NO
        # RGB / renderer material; the renderer may ignore this in MS1.A (MS1.B consumes it).
        "# surface_code bit layout: [0:4]=substrate [4:7]=lithology [7:10]=family "
        "[10:14]=wetness [14:18]=weathering [18:22]=soil_depth [22:26]=stability "
        "[26:30]=organic_potential [30:34]=exposure [34:38]=roughness (nibbles = /15)",
        f"surface_descriptor_version={SURFACE_DESCRIPTOR_VERSION}",
        f"surface_step_m={page.surface_step:.1f}",
        f"surface_grid_n={page.surface_n}",
        f"surface_digest={page.surface_digest}",
        f"surface_grid_row_major={' '.join(f'{c:x}' for c in page.surface_codes)}",
        # WD1.A WaterState descriptor: compact SEMANTIC water identity per coarse cell (presence/
        # body/flow/bottom-family + depth + optical axes + reserved hooks packed into one integer).
        # NO RGB / shader coefficients; environmental PROMISE, not a conserved water ledger. The
        # renderer may ignore this in WD1.A (WD1.B consumes it).
        "# water_code bit layout: [0:3]=presence [3:7]=body [7:10]=flow [10:13]=bottom_family "
        "[13:22]=depth(0.25m units) [22:26]=discharge [26:30]=seasonality [30:34]=clarity "
        "[34:38]=turbidity [38:42]=suspended_sediment [42:46]=mineral_load [46:50]=organic_load "
        "[50:54]=temperature [54:56]=waterfall_potential(/3) [56:58]=mineral_potential(/3) "
        "[58]=macro_authority(0=valid_macro,1=defer_to_detailed -- defer!=dry: read this FIRST)",
        f"water_descriptor_version={WATER_DESCRIPTOR_VERSION}",
        f"water_step_m={page.water_step:.1f}",
        f"water_grid_n={page.water_n}",
        f"water_digest={page.water_digest}",
        f"water_grid_row_major={' '.join(f'{c:x}' for c in page.water_codes)}",
        "",
    ]
    # EI0.C adds only canonical representation/authority metadata.  The payload
    # strings above remain the parity oracle and are byte-identical to V1.
    return upgrade_legacy_page_text("\n".join(lines))
