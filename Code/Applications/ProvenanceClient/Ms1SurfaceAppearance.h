// MS1.B — Shared Surface Appearance resolver.
//
// ONE semantic SurfaceState -> appearance path, used by BOTH the near MV1 surface and
// the MV2/MV3 macro horizon. Distance controls only representation FIDELITY (grain /
// breakup strength); it never changes the dominant substrate family, lithology family,
// wet/dry meaning, exposed-rock-vs-regolith-vs-deposit meaning, or volcanic ancestry.
// There is exactly one palette here and it is derived from MATERIAL, not from elevation
// or from which render path is drawing. This retires far=olive / mid=white / near=gray.
//
// The packed-code bit layout MIRRORS the MS1.A Python authority
// (Tools/Worldgen/macro_authority.py :: SurfaceState.pack); the class index order MUST
// stay identical to SUBSTRATE_CLASSES / LITHOLOGY_CLASSES / SURFACE_FAMILIES there.
//
// Presentation only: no geometry, no authority. MW9 flora / water / snow remain closed —
// organic_potential is NOT vegetation and is never painted green; waterlogged/wet is a
// darker mineral ground, not a blue water surface.
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace Ms1
{
    // ---- vocabulary (index order == Python authority; do not reorder) ---------------- #
    enum Substrate : uint8_t {
        SUB_BARE_BEDROCK=0, SUB_WEATHERED_BEDROCK, SUB_THIN_REGOLITH, SUB_COLLUVIUM, SUB_TALUS,
        SUB_ALLUVIUM, SUB_FLOODPLAIN, SUB_BASIN_FILL, SUB_ORGANIC_CAPABLE, SUB_WATERLOGGED,
        SUB_FRESH_LAVA, SUB_SCORIA_ASH, SUB_WEATHERED_BASALT, SUB_VOLCANIC_SOIL, SUB_COUNT
    };
    enum Lithology : uint8_t {
        LIT_GRANITE=0, LIT_BASALT, LIT_SANDSTONE, LIT_SHALE, LIT_LIMESTONE, LIT_QUARTZITE,
        LIT_METAMORPHIC, LIT_MIXED, LIT_COUNT
    };
    enum Family : uint8_t {
        FAM_ROCK=0, FAM_REGOLITH, FAM_SEDIMENT, FAM_VOLCANIC, FAM_ORGANIC, FAM_COUNT
    };

    struct SurfaceState
    {
        uint8_t substrate = SUB_BARE_BEDROCK;
        uint8_t lith = LIT_MIXED;
        uint8_t family = FAM_ROCK;
        float wetness = 0.f, weathering = 0.f, soilDepth = 0.f, stability = 0.f;
        float organic = 0.f, exposure = 0.f, roughness = 0.5f;
        bool valid = false;
    };

    inline float nib(int n) { return (float)n / 15.f; }

    // Mirror of Python unpack_surface(). Bit layout:
    // [0:4]=substrate [4:7]=lith [7:10]=family [10:14]=wetness [14:18]=weathering
    // [18:22]=soil_depth [22:26]=stability [26:30]=organic [30:34]=exposure [34:38]=roughness
    inline SurfaceState Unpack(uint64_t v)
    {
        SurfaceState s;
        s.substrate = (uint8_t)(v & 0xF);
        s.lith      = (uint8_t)((v >> 4) & 0x7);
        s.family    = (uint8_t)((v >> 7) & 0x7);
        s.wetness   = nib((int)((v >> 10) & 0xF));
        s.weathering= nib((int)((v >> 14) & 0xF));
        s.soilDepth = nib((int)((v >> 18) & 0xF));
        s.stability = nib((int)((v >> 22) & 0xF));
        s.organic   = nib((int)((v >> 26) & 0xF));
        s.exposure  = nib((int)((v >> 30) & 0xF));
        s.roughness = nib((int)((v >> 34) & 0xF));
        if (s.substrate >= SUB_COUNT || s.lith >= LIT_COUNT || s.family >= FAM_COUNT)
            return SurfaceState();          // malformed -> invalid (fail-closed)
        s.valid = true;
        return s;
    }

    inline uint8_t SubstrateFamily(uint8_t sub)
    {
        switch (sub) {
            case SUB_BARE_BEDROCK: case SUB_WEATHERED_BEDROCK: return FAM_ROCK;
            case SUB_THIN_REGOLITH: case SUB_COLLUVIUM: case SUB_TALUS: return FAM_REGOLITH;
            case SUB_ORGANIC_CAPABLE: return FAM_ORGANIC;
            case SUB_ALLUVIUM: case SUB_FLOODPLAIN: case SUB_BASIN_FILL:
            case SUB_WATERLOGGED: return FAM_SEDIMENT;
            default: return FAM_VOLCANIC;   // fresh_lava / scoria_ash / weathered_basalt / volcanic_soil
        }
    }

    struct RGB { float r, g, b; };
    inline RGB mix(RGB a, RGB b, float t) { return { a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t }; }

    // Host-rock albedo family (drives bare/weathered bedrock and shows through thin soil).
    inline RGB LithologyAlbedo(uint8_t lith)
    {
        switch (lith) {
            case LIT_GRANITE:    return { 0.62f, 0.58f, 0.55f };  // light grey-pink intrusive
            case LIT_BASALT:     return { 0.24f, 0.22f, 0.22f };  // dark
            case LIT_SANDSTONE:  return { 0.78f, 0.63f, 0.41f };  // warm tan, bench-forming
            case LIT_SHALE:      return { 0.42f, 0.44f, 0.45f };  // grey fine sediment
            case LIT_LIMESTONE:  return { 0.80f, 0.79f, 0.71f };  // pale carbonate
            case LIT_QUARTZITE:  return { 0.74f, 0.70f, 0.72f };  // light hard
            case LIT_METAMORPHIC:return { 0.52f, 0.48f, 0.54f };  // grey-violet
            default:             return { 0.55f, 0.52f, 0.50f };  // mixed/unknown neutral
        }
    }

    // Substrate cover albedo (the exposed material when it is not bare rock). For rock-tier
    // substrates the lithology drives colour; regolith/sediment/volcanic/organic have their
    // own material tone. NONE of these are vegetation or water.
    inline RGB SubstrateAlbedo(SurfaceState const& s)
    {
        RGB lithA = LithologyAlbedo(s.lith);
        switch (s.substrate) {
            case SUB_BARE_BEDROCK:      return lithA;
            case SUB_WEATHERED_BEDROCK: return mix(lithA, { 0.46f, 0.38f, 0.30f }, 0.30f + 0.30f*s.weathering);
            case SUB_THIN_REGOLITH:     return mix(lithA, { 0.52f, 0.43f, 0.31f }, 0.35f + 0.45f*s.soilDepth);
            case SUB_COLLUVIUM:         return { 0.50f, 0.42f, 0.33f };   // coarse hillslope debris
            case SUB_TALUS:             return mix(lithA, { 0.50f, 0.48f, 0.46f }, 0.45f); // broken rock apron
            case SUB_ALLUVIUM:          return { 0.66f, 0.58f, 0.43f };   // lowland sediment
            case SUB_FLOODPLAIN:        return { 0.58f, 0.54f, 0.43f };   // finer, moister
            case SUB_BASIN_FILL:        return { 0.60f, 0.55f, 0.45f };
            case SUB_ORGANIC_CAPABLE:   return { 0.34f, 0.30f, 0.22f };   // dark humic soil (NOT green)
            case SUB_WATERLOGGED:       return { 0.31f, 0.33f, 0.33f };   // dark wet mineral ground
            case SUB_FRESH_LAVA:        return { 0.14f, 0.13f, 0.13f };   // near-black fresh basalt
            case SUB_SCORIA_ASH:        return { 0.33f, 0.21f, 0.17f };   // dark red-brown pyroclastic
            case SUB_WEATHERED_BASALT:  return { 0.40f, 0.31f, 0.25f };   // browner, oxidised
            case SUB_VOLCANIC_SOIL:     return { 0.29f, 0.25f, 0.19f };   // dark fertile andosol
            default:                    return lithA;
        }
    }

    // Deterministic cheap grain/breakup: a value hash of the world position, scaled by the
    // roughness proxy and by distance FIDELITY (near = full grain, far = smooth). This is
    // the only distance-dependent term and it changes brightness only, never the family.
    inline float Grain(double wx, double wy, float roughness, float fidelity)
    {
        if (fidelity <= 0.f || roughness <= 0.f) return 0.f;
        auto h = [](double a, double b, double s) {
            double v = std::sin(a*12.9898 + b*78.233 + s*37.719) * 43758.5453;
            return (float)(v - std::floor(v));
        };
        float coarse = h(std::floor(wx/22.0), std::floor(wy/22.0), 1.7);
        float fine   = h(std::floor(wx/6.0),  std::floor(wy/6.0),  9.1);
        float n = (coarse*0.62f + fine*0.38f) - 0.5f;           // [-0.5,0.5]
        return n * (0.11f * roughness) * fidelity;              // brightness jitter
    }

    inline float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

    // The shared resolver: SurfaceState (+ world pos for grain, + distance for fidelity)
    // -> base albedo in [0,1]. Caller applies hillshade / aerial separately.
    inline RGB ResolveAppearance(SurfaceState const& s, double wx, double wy, float distanceM)
    {
        RGB base = SubstrateAlbedo(s);

        // exposure: high bare-rock fraction pulls a soil/regolith surface back toward its
        // host lithology (rock showing through), never the reverse. Bounded.
        if (s.family == FAM_REGOLITH || s.substrate == SUB_WEATHERED_BEDROCK)
            base = mix(base, LithologyAlbedo(s.lith), 0.35f * s.exposure);

        // wetness: wet ground is darker and slightly less saturated (mineral, not water).
        float wetDark = 1.f - 0.34f * s.wetness;
        float luma = 0.30f*base.r + 0.59f*base.g + 0.11f*base.b;
        base.r = base.r*wetDark + (luma*base.r - base.r)*0.0f;   // (keep hue; darken only)
        base.r *= wetDark; base.g *= wetDark; base.b *= wetDark;
        (void)luma;

        // weathering already folded into substrate albedo for rock/volcanic; add a faint
        // dulling for coherent rock so fresh vs weathered reads even on bare bedrock.
        if (s.substrate == SUB_BARE_BEDROCK)
            base = mix(base, { 0.46f, 0.40f, 0.34f }, 0.18f * s.weathering);

        // distance fidelity: full grain within ~2 km, gone by ~20 km (macro is always far).
        float fidelity = clamp01(1.f - (distanceM - 2000.f) / 18000.f);
        float gj = Grain(wx, wy, s.roughness, fidelity);
        base.r = clamp01(base.r + gj);
        base.g = clamp01(base.g + gj);
        base.b = clamp01(base.b + gj);
        return base;
    }

    // ---- diagnostic axis colouring (explicit debug toggles; not normal play) --------- #
    enum DebugAxis : int {
        DBG_OFF=0, DBG_SUBSTRATE, DBG_LITHOLOGY, DBG_FAMILY, DBG_WETNESS,
        DBG_WEATHERING, DBG_SOIL, DBG_EXPOSURE
    };
    inline RGB Ramp(float v, RGB lo, RGB hi) { return mix(lo, hi, clamp01(v)); }
    inline RGB DebugColor(SurfaceState const& s, int axis)
    {
        switch (axis) {
            case DBG_SUBSTRATE: {   // distinct hue per substrate index
                float h = (float)s.substrate / (float)SUB_COUNT;
                return { 0.5f+0.5f*std::sin(6.283f*h), 0.5f+0.5f*std::sin(6.283f*h+2.09f),
                         0.5f+0.5f*std::sin(6.283f*h+4.19f) };
            }
            case DBG_LITHOLOGY:  return LithologyAlbedo(s.lith);
            case DBG_FAMILY: {
                static const RGB fc[FAM_COUNT] = {
                    {0.6f,0.6f,0.62f},{0.77f,0.66f,0.47f},{0.87f,0.80f,0.51f},
                    {0.59f,0.24f,0.20f},{0.31f,0.52f,0.27f} };
                return fc[s.family < FAM_COUNT ? s.family : 0];
            }
            case DBG_WETNESS:    return Ramp(s.wetness,   {0.70f,0.66f,0.47f},{0.12f,0.24f,0.59f});
            case DBG_WEATHERING: return Ramp(s.weathering,{0.59f,0.59f,0.59f},{0.47f,0.31f,0.16f});
            case DBG_SOIL:       return Ramp(s.soilDepth, {0.67f,0.59f,0.47f},{0.16f,0.43f,0.16f});
            case DBG_EXPOSURE:   return Ramp(s.exposure,  {0.24f,0.35f,0.20f},{0.82f,0.82f,0.82f});
            default:             return { 0.5f, 0.5f, 0.5f };
        }
    }
}
