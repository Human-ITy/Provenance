// MS1.B — Shared Surface Appearance resolver.
//
// ONE semantic SurfaceState -> appearance path, used by BOTH the near MV1 surface and
// the MV2/MV3 macro horizon. Distance controls only representation FIDELITY (grain /
// breakup strength); it never changes the dominant substrate family, lithology family,
// wet/dry meaning, exposed-rock-vs-regolith-vs-deposit meaning, or volcanic ancestry.
// There is exactly one palette here and it is derived from MATERIAL, not from elevation
// or from which render path is drawing. This retires far=olive / mid=white / near=gray.
//
// Descriptor vocabulary and bit layouts come from the EI0.B generated contract
// (`WorldDescriptors.generated.h`). This file owns presentation only.
//
// Presentation only: no geometry, no authority. MW9 flora / water / snow remain closed —
// organic_potential is NOT vegetation and is never painted green; waterlogged/wet is a
// darker mineral ground, not a blue water surface.
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>

#include "WorldDescriptors.generated.h"

namespace Ms1
{
    struct SurfaceState
    {
        uint8_t substrate = SUB_BARE_BEDROCK;
        uint8_t lith = LIT_MIXED;
        uint8_t family = FAM_ROCK;
        float wetness = 0.f, weathering = 0.f, soilDepth = 0.f, stability = 0.f;
        float organic = 0.f, exposure = 0.f, roughness = 0.5f;
        bool valid = false;
    };

    inline SurfaceState Unpack(uint64_t v, uint32_t encodingVersion=kSurfaceEncodingVersion)
    {
        SurfaceDescriptorWire wire;
        if(!DecodeSurfaceDescriptor(encodingVersion,v,wire))return SurfaceState();
        SurfaceState s;
        s.substrate=wire.substrate;s.lith=wire.lith;s.family=wire.family;
        s.wetness=wire.wetness;s.weathering=wire.weathering;s.soilDepth=wire.soilDepth;
        s.stability=wire.stability;s.organic=wire.organic;s.exposure=wire.exposure;
        s.roughness=wire.roughness;
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

    // ===================================================================================
    // WD1.B — shared Water Appearance. Water is an OVERLAY over the MS1 substrate: what you
    // see = the bottom (MS1) transmitted through the water column + the body's own optical
    // character + surface reflection. Continuous optical DEPTH, never colour bands. The
    // WaterState consumes the generated WD1.A wire descriptor contract.
    // ===================================================================================
    struct WaterState {
        uint8_t presence = WP_DRY, body = WB_NONE, flow = 0, bottomFamily = FAM_ROCK, authority = WA_VALID_MACRO;
        float depth_m = 0.f, discharge = 0.f, seasonality = 0.f, clarity = 1.f, turbidity = 0.f;
        float sediment = 0.f, mineral = 0.f, organic = 0.f, temperature = 0.f;
        float waterfallPot = 0.f, mineralPot = 0.f;
        bool valid = false;
    };

    inline WaterState UnpackWater(uint64_t v, uint32_t encodingVersion=kWaterEncodingVersion)
    {
        WaterDescriptorWire wire;
        if(!DecodeWaterDescriptor(encodingVersion,v,wire))return WaterState();
        WaterState w;
        w.presence=wire.presence;w.body=wire.body;w.flow=wire.flow;
        w.bottomFamily=wire.bottomFamily;w.authority=wire.authority;
        w.depth_m=wire.depthM;w.discharge=wire.discharge;w.seasonality=wire.seasonality;
        w.clarity=wire.clarity;w.turbidity=wire.turbidity;w.sediment=wire.sediment;
        w.mineral=wire.mineral;w.organic=wire.organic;w.temperature=wire.temperature;
        w.waterfallPot=wire.waterfallPot;w.mineralPot=wire.mineralPot;
        w.valid = true;
        return w;
    }

    // Is there macro-authoritative standing/flowing water to render at this cell?
    inline bool MacroWaterPresent(WaterState const& w)
    {
        return w.valid && w.authority == WA_VALID_MACRO && w.body != WB_NONE
            && w.presence != WP_DRY && w.presence != WP_DAMP;
    }
    inline bool MacroStandingWater(WaterState const& w)
    {
        return MacroWaterPresent(w) && (w.body == WB_ALPINE_LAKE || w.body == WB_CLOSED_LAKE
            || w.body == WB_FLOODPLAIN || w.body == WB_WETLAND || w.body == WB_ORGANIC
            || w.body == WB_VOLCANIC_POOL || w.body == WB_CRATER);
    }

    // The body's OWN optical colour (what deep water tends toward), by causal state — clear
    // cold blue-green, sediment olive-brown, organic tea-dark, mineral turquoise/amber.
    inline RGB WaterBodyOptical(WaterState const& w)
    {
        RGB clear    = { 0.06f, 0.24f, 0.30f };   // cold clear deep blue-green
        RGB sediment = { 0.22f, 0.26f, 0.18f };   // olive / brown-green suspended load
        RGB organic  = { 0.10f, 0.15f, 0.12f };   // tea / dark brown-green
        RGB mineralT = { 0.06f, 0.40f, 0.40f };   // mineral turquoise
        RGB c = clear;
        c = mix(c, sediment, clamp01(0.85f * w.sediment + 0.5f * w.turbidity));
        c = mix(c, organic,  clamp01(w.organic));
        c = mix(c, mineralT, clamp01(w.mineral));
        // warmer bodies read very slightly greener/warmer; cold alpine stays blue
        c = mix(c, RGB{ c.r * 1.05f, c.g * 1.02f, c.b * 0.92f }, 0.4f * w.temperature);
        return { clamp01(c.r), clamp01(c.g), clamp01(c.b) };
    }

    // Optical attenuation coefficient (per metre): clear water lets the bottom show metres
    // down; turbid/organic water hides it within ~1 m. Continuous — this is the depth law.
    inline float WaterAttenuation(WaterState const& w)
    {
        return 0.10f + 1.35f * w.turbidity + 0.85f * w.organic + 0.25f * w.sediment;
    }

    // The shared resolver: observed = bottom transmitted through depth + body optical + surface
    // reflection. `bottom` is the MS1 substrate appearance (already resolved). `sky` is the
    // horizon/sky colour for reflection. `fresnel` in [0,1] (grazing = more reflection).
    inline RGB ResolveWaterAppearance(WaterState const& w, RGB bottom, float depth_m,
                                      RGB sky, float fresnel)
    {
        float const k = WaterAttenuation(w);
        float const T = std::exp(-k * (depth_m < 0.f ? 0.f : depth_m));   // bottom transmittance
        RGB body = WaterBodyOptical(w);
        // shallow: bottom shows through (tinted by a thin water column); deep: body dominates.
        RGB throughWater = { bottom.r * (0.55f + 0.45f * body.r * 3.0f),
                             bottom.g * (0.55f + 0.45f * body.g * 3.0f),
                             bottom.b * (0.60f + 0.40f * body.b * 3.0f) };
        throughWater = { clamp01(mix(bottom, throughWater, 0.5f).r),
                         clamp01(mix(bottom, throughWater, 0.5f).g),
                         clamp01(mix(bottom, throughWater, 0.5f).b) };
        RGB observed = { body.r + (throughWater.r - body.r) * T,
                         body.g + (throughWater.g - body.g) * T,
                         body.b + (throughWater.b - body.b) * T };
        // surface reflection: still water reflects sky more; turbulent/turbid scatters.
        float still = (w.flow <= 2) ? 1.f : 0.5f;                 // still/slow reflect more
        float refl = clamp01((0.06f + 0.22f * fresnel) * still * (0.5f + 0.5f * w.clarity));
        observed = { observed.r + (sky.r - observed.r) * refl,
                     observed.g + (sky.g - observed.g) * refl,
                     observed.b + (sky.b - observed.b) * refl };
        return { clamp01(observed.r), clamp01(observed.g), clamp01(observed.b) };
    }
}
