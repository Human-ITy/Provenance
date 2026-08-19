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

GENERATOR_VERSION = 2
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

    def regional_field(self, x: float, y: float) -> float:
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
        return z


def macro_z(central: CentralProgram, fieldf: MacroField, x: float, y: float) -> float:
    """The full continuous macro surface: frozen center + anchored regional field."""
    return central_envelope(central, x, y) + anchor_window(x, y) * fieldf.regional_field(x, y)


# --------------------------------------------------------------------------- #
# Compiled macro PAGES (64 km serialization cells)
# --------------------------------------------------------------------------- #

PAGE_MAGIC = "PROVENANCE_MACRO_AUTHORITY_PAGE_V1"
PAGE_STEP_M = 1000.0        # coarse macro sample step (macro silhouette, not grooves)


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
            heights.append(macro_z(central, fieldf, x, y))
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
    return MacroPage(
        ri=ri, rj=rj, min_x=min_x, min_y=min_y, step=step, n=n, heights=heights,
        region_id=region_id(central, ri, rj), world_seed=fieldf.seed,
        generator_version=GENERATOR_VERSION,
        world_identity_hash=central.world_identity_hash,
        source_digest=f"{hh:016x}", neighbor_lineage=lineage)


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
        "",
    ]
    return "\n".join(lines)
