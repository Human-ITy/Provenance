// GENERATED FILE — DO NOT EDIT.
// source_schema: schemas/world_descriptors.json
// generator_version: ei0b.generator.1
// schema_digest: 8857189f2dfb5fe3ba73d470ca653248188a0717469963d64d6d5003bad6dac5
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Ms1
{
    constexpr char const* kWorldDescriptorSchemaDigest = "8857189f2dfb5fe3ba73d470ca653248188a0717469963d64d6d5003bad6dac5";
    constexpr uint32_t kSurfaceEncodingVersion = 1;
    constexpr uint32_t kWaterEncodingVersion = 1;
    constexpr char const* kSurfaceEncodingId = "provenance.surface-state.packed";
    constexpr char const* kWaterEncodingId = "provenance.water-state.packed";
    constexpr char const* kSurfaceGrammarId = "provenance.surface-state.ms1a";
    constexpr char const* kSurfaceGrammarVersion = "ms1a.1";
    constexpr char const* kWaterGrammarId = "provenance.water-state.wd1a";
    constexpr char const* kWaterGrammarVersion = "wd1a.1";
    constexpr uint32_t kSurfacePackedBits = 38;
    constexpr uint32_t kWaterPackedBits = 59;

    enum Substrate : uint8_t {
        SUB_BARE_BEDROCK=0,
        SUB_WEATHERED_BEDROCK=1,
        SUB_THIN_REGOLITH=2,
        SUB_COLLUVIUM=3,
        SUB_TALUS=4,
        SUB_ALLUVIUM=5,
        SUB_FLOODPLAIN=6,
        SUB_BASIN_FILL=7,
        SUB_ORGANIC_CAPABLE=8,
        SUB_WATERLOGGED=9,
        SUB_FRESH_LAVA=10,
        SUB_SCORIA_ASH=11,
        SUB_WEATHERED_BASALT=12,
        SUB_VOLCANIC_SOIL=13,
        SUB_COUNT=14
    };

    enum Lithology : uint8_t {
        LIT_GRANITE=0,
        LIT_BASALT=1,
        LIT_SANDSTONE=2,
        LIT_SHALE=3,
        LIT_LIMESTONE=4,
        LIT_QUARTZITE=5,
        LIT_METAMORPHIC=6,
        LIT_MIXED=7,
        LIT_COUNT=8
    };

    enum Family : uint8_t {
        FAM_ROCK=0,
        FAM_REGOLITH=1,
        FAM_SEDIMENT=2,
        FAM_VOLCANIC=3,
        FAM_ORGANIC=4,
        FAM_COUNT=5
    };

    enum WPresence : uint8_t {
        WP_DRY=0,
        WP_DAMP=1,
        WP_EPHEMERAL=2,
        WP_SEASONAL=3,
        WP_PERENNIAL=4,
        WP_STANDING=5,
        WP_UNSPECIFIED=255,
        WP_COUNT=6
    };

    enum WBody : uint8_t {
        WB_NONE=0,
        WB_HEADWATER=1,
        WB_PERENNIAL_RIVER=2,
        WB_SEDIMENT_RIVER=3,
        WB_BRAIDED=4,
        WB_ALPINE_LAKE=5,
        WB_CLOSED_LAKE=6,
        WB_FLOODPLAIN=7,
        WB_WETLAND=8,
        WB_ORGANIC=9,
        WB_ARID_WASH=10,
        WB_SPRING=11,
        WB_VOLCANIC_POOL=12,
        WB_CRATER=13,
        WB_COUNT=14
    };

    enum WFlow : uint8_t {
        WF_NONE=0,
        WF_STILL=1,
        WF_SLOW=2,
        WF_CHANNELIZED=3,
        WF_FAST=4,
        WF_TURBULENT=5,
        WF_COUNT=6
    };

    enum WAuthority : uint8_t {
        WA_VALID_MACRO=0,
        WA_DEFER_TO_DETAILED=1,
        WA_COUNT=2
    };

    enum class DescriptorDecodeError : uint8_t
    {
        None=0, UnknownEncodingVersion, ReservedBits, ReservedEnum, IllegalCombination
    };

    inline uint16_t QuantizeHalfUp(double value, double minimum, double maximum, uint16_t denominator)
    {
        value = value < minimum ? minimum : (value > maximum ? maximum : value);
        return (uint16_t)((value-minimum)*(double)denominator + 0.5);
    }
    inline uint16_t QuantizeHalfEven(double value, double minimum, double maximum, uint16_t denominator)
    {
        value = value < minimum ? minimum : (value > maximum ? maximum : value);
        double const scaled=(value-minimum)*(double)denominator;
        uint16_t const whole=(uint16_t)scaled;
        double const fraction=scaled-(double)whole;
        return (uint16_t)(whole + ((fraction>0.5 || (fraction==0.5 && (whole&1))) ? 1 : 0));
    }

    struct SurfaceDescriptorWire
    {
        uint8_t substrate=SUB_BARE_BEDROCK,lith=LIT_MIXED,family=FAM_ROCK;
        uint8_t wetnessQ=0,weatheringQ=0,soilDepthQ=0,stabilityQ=0,organicQ=0,exposureQ=0,roughnessQ=0;
        double wetness=0.0,weathering=0.0,soilDepth=0.0,stability=0.0,organic=0.0,exposure=0.0,roughness=0.0;
    };

    inline bool ValidateSurfaceDescriptor(SurfaceDescriptorWire const& in,
                                           DescriptorDecodeError* error=nullptr)
    {
        DescriptorDecodeError e=DescriptorDecodeError::None;
        if(in.substrate>=SUB_COUNT || in.lith>=LIT_COUNT || in.family>=FAM_COUNT)
            e=DescriptorDecodeError::ReservedEnum;
        if(error)*error=e; return e==DescriptorDecodeError::None;
    }

    inline bool DecodeSurfaceDescriptor(uint32_t version,uint64_t packed,
                                         SurfaceDescriptorWire& out,
                                         DescriptorDecodeError* error=nullptr)
    {
        if(version!=kSurfaceEncodingVersion){if(error)*error=DescriptorDecodeError::UnknownEncodingVersion;return false;}
        if((packed>>kSurfacePackedBits)!=0){if(error)*error=DescriptorDecodeError::ReservedBits;return false;}
        out=SurfaceDescriptorWire{};
        out.substrate=(uint8_t)((packed >> 0) & 0xfULL); out.lith=(uint8_t)((packed >> 4) & 0x7ULL); out.family=(uint8_t)((packed >> 7) & 0x7ULL);
        out.wetnessQ=(uint8_t)((packed >> 10) & 0xfULL); out.weatheringQ=(uint8_t)((packed >> 14) & 0xfULL); out.soilDepthQ=(uint8_t)((packed >> 18) & 0xfULL);
        out.stabilityQ=(uint8_t)((packed >> 22) & 0xfULL); out.organicQ=(uint8_t)((packed >> 26) & 0xfULL); out.exposureQ=(uint8_t)((packed >> 30) & 0xfULL);
        out.roughnessQ=(uint8_t)((packed >> 34) & 0xfULL);
        out.wetness=out.wetnessQ/15.0; out.weathering=out.weatheringQ/15.0;
        out.soilDepth=out.soilDepthQ/15.0; out.stability=out.stabilityQ/15.0;
        out.organic=out.organicQ/15.0; out.exposure=out.exposureQ/15.0;
        out.roughness=out.roughnessQ/15.0;
        return ValidateSurfaceDescriptor(out,error);
    }

    inline bool EncodeSurfaceDescriptor(uint32_t version,SurfaceDescriptorWire in,
                                         uint64_t& packed,
                                         DescriptorDecodeError* error=nullptr)
    {
        if(version!=kSurfaceEncodingVersion){if(error)*error=DescriptorDecodeError::UnknownEncodingVersion;return false;}
        in.wetnessQ=(uint8_t)QuantizeHalfUp(in.wetness,0.0,1.0,15);
        in.weatheringQ=(uint8_t)QuantizeHalfUp(in.weathering,0.0,1.0,15);
        in.soilDepthQ=(uint8_t)QuantizeHalfUp(in.soilDepth,0.0,1.0,15);
        in.stabilityQ=(uint8_t)QuantizeHalfUp(in.stability,0.0,1.0,15);
        in.organicQ=(uint8_t)QuantizeHalfUp(in.organic,0.0,1.0,15);
        in.exposureQ=(uint8_t)QuantizeHalfUp(in.exposure,0.0,1.0,15);
        in.roughnessQ=(uint8_t)QuantizeHalfUp(in.roughness,0.0,1.0,15);
        if(!ValidateSurfaceDescriptor(in,error))return false;
        packed=0;
        packed |= (uint64_t)(in.substrate & 0xfu) << 0;
        packed |= (uint64_t)(in.lith & 0x7u) << 4;
        packed |= (uint64_t)(in.family & 0x7u) << 7;
        packed |= (uint64_t)(in.wetnessQ & 0xfu) << 10;
        packed |= (uint64_t)(in.weatheringQ & 0xfu) << 14;
        packed |= (uint64_t)(in.soilDepthQ & 0xfu) << 18;
        packed |= (uint64_t)(in.stabilityQ & 0xfu) << 22;
        packed |= (uint64_t)(in.organicQ & 0xfu) << 26;
        packed |= (uint64_t)(in.exposureQ & 0xfu) << 30;
        packed |= (uint64_t)(in.roughnessQ & 0xfu) << 34;
        if(error)*error=DescriptorDecodeError::None; return true;
    }

    struct WaterDescriptorWire
    {
        uint8_t encodedPresence=WP_DRY,presence=WP_DRY,body=WB_NONE,flow=WF_NONE,bottomFamily=FAM_ROCK,authority=WA_VALID_MACRO;
        uint16_t depthQ=0; uint8_t dischargeQ=0,seasonalityQ=0,clarityQ=0,turbidityQ=0,sedimentQ=0,mineralQ=0,organicQ=0,temperatureQ=0,waterfallQ=0,mineralPotQ=0;
        double depthM=0.0,discharge=0.0,seasonality=0.0,clarity=0.0,turbidity=0.0,sediment=0.0,mineral=0.0,organic=0.0,temperature=0.0,waterfallPot=0.0,mineralPot=0.0;
        bool hasPresenceConclusion=true;
    };

    inline bool ValidateWaterDescriptor(WaterDescriptorWire const& in,
                                         DescriptorDecodeError* error=nullptr)
    {
        DescriptorDecodeError e=DescriptorDecodeError::None;
        if(in.encodedPresence>=WP_COUNT || in.body>=WB_COUNT || in.flow>=WF_COUNT
           || in.bottomFamily>=FAM_COUNT || in.authority>=WA_COUNT)
            e=DescriptorDecodeError::ReservedEnum;
        else if(in.authority==WA_DEFER_TO_DETAILED)
        {
            if(in.encodedPresence!=WP_DRY || in.body!=WB_NONE || in.flow!=WF_NONE)
                e=DescriptorDecodeError::IllegalCombination;
        }
        else if(in.encodedPresence>=WP_EPHEMERAL && in.body==WB_NONE)
            e=DescriptorDecodeError::IllegalCombination;
        if(error)*error=e; return e==DescriptorDecodeError::None;
    }

    inline bool DecodeWaterDescriptor(uint32_t version,uint64_t packed,
                                       WaterDescriptorWire& out,
                                       DescriptorDecodeError* error=nullptr)
    {
        if(version!=kWaterEncodingVersion){if(error)*error=DescriptorDecodeError::UnknownEncodingVersion;return false;}
        if((packed>>kWaterPackedBits)!=0){if(error)*error=DescriptorDecodeError::ReservedBits;return false;}
        out=WaterDescriptorWire{};
        out.encodedPresence=(uint8_t)((packed >> 0) & 0x7ULL); out.body=(uint8_t)((packed >> 3) & 0xfULL); out.flow=(uint8_t)((packed >> 7) & 0x7ULL); out.bottomFamily=(uint8_t)((packed >> 10) & 0x7ULL);
        out.depthQ=(uint16_t)((packed>>13)&0x1ffULL);
        out.dischargeQ=(uint8_t)((packed >> 22) & 0xfULL); out.seasonalityQ=(uint8_t)((packed >> 26) & 0xfULL); out.clarityQ=(uint8_t)((packed >> 30) & 0xfULL);
        out.turbidityQ=(uint8_t)((packed >> 34) & 0xfULL); out.sedimentQ=(uint8_t)((packed >> 38) & 0xfULL); out.mineralQ=(uint8_t)((packed >> 42) & 0xfULL);
        out.organicQ=(uint8_t)((packed >> 46) & 0xfULL); out.temperatureQ=(uint8_t)((packed >> 50) & 0xfULL); out.waterfallQ=(uint8_t)((packed >> 54) & 0x3ULL);
        out.mineralPotQ=(uint8_t)((packed >> 56) & 0x3ULL); out.authority=(uint8_t)((packed >> 58) & 0x1ULL);
        if(!ValidateWaterDescriptor(out,error))return false;
        out.hasPresenceConclusion=out.authority==WA_VALID_MACRO;
        out.presence=out.hasPresenceConclusion?out.encodedPresence:WP_UNSPECIFIED;
        out.depthM=out.depthQ/4.0; out.discharge=out.dischargeQ/15.0;
        out.seasonality=out.seasonalityQ/15.0; out.clarity=out.clarityQ/15.0;
        out.turbidity=out.turbidityQ/15.0; out.sediment=out.sedimentQ/15.0;
        out.mineral=out.mineralQ/15.0; out.organic=out.organicQ/15.0;
        out.temperature=out.temperatureQ/15.0; out.waterfallPot=out.waterfallQ/3.0;
        out.mineralPot=out.mineralPotQ/3.0;
        return true;
    }

    inline bool EncodeWaterDescriptor(uint32_t version,WaterDescriptorWire in,
                                       uint64_t& packed,
                                       DescriptorDecodeError* error=nullptr)
    {
        if(version!=kWaterEncodingVersion){if(error)*error=DescriptorDecodeError::UnknownEncodingVersion;return false;}
        in.depthQ=QuantizeHalfEven(in.depthM,0.0,127.75,4);
        in.dischargeQ=(uint8_t)QuantizeHalfUp(in.discharge,0.0,1.0,15);
        in.seasonalityQ=(uint8_t)QuantizeHalfUp(in.seasonality,0.0,1.0,15);
        in.clarityQ=(uint8_t)QuantizeHalfUp(in.clarity,0.0,1.0,15);
        in.turbidityQ=(uint8_t)QuantizeHalfUp(in.turbidity,0.0,1.0,15);
        in.sedimentQ=(uint8_t)QuantizeHalfUp(in.sediment,0.0,1.0,15);
        in.mineralQ=(uint8_t)QuantizeHalfUp(in.mineral,0.0,1.0,15);
        in.organicQ=(uint8_t)QuantizeHalfUp(in.organic,0.0,1.0,15);
        in.temperatureQ=(uint8_t)QuantizeHalfUp(in.temperature,0.0,1.0,15);
        in.waterfallQ=(uint8_t)QuantizeHalfEven(in.waterfallPot,0.0,1.0,3);
        in.mineralPotQ=(uint8_t)QuantizeHalfEven(in.mineralPot,0.0,1.0,3);
        if(!ValidateWaterDescriptor(in,error))return false;
        packed=0;
        packed |= (uint64_t)(in.encodedPresence & 0x7u) << 0;
        packed |= (uint64_t)(in.body & 0xfu) << 3;
        packed |= (uint64_t)(in.flow & 0x7u) << 7;
        packed |= (uint64_t)(in.bottomFamily & 0x7u) << 10;
        packed |= (uint64_t)(in.depthQ & 0x1ffu) << 13;
        packed |= (uint64_t)(in.dischargeQ & 0xfu) << 22;
        packed |= (uint64_t)(in.seasonalityQ & 0xfu) << 26;
        packed |= (uint64_t)(in.clarityQ & 0xfu) << 30;
        packed |= (uint64_t)(in.turbidityQ & 0xfu) << 34;
        packed |= (uint64_t)(in.sedimentQ & 0xfu) << 38;
        packed |= (uint64_t)(in.mineralQ & 0xfu) << 42;
        packed |= (uint64_t)(in.organicQ & 0xfu) << 46;
        packed |= (uint64_t)(in.temperatureQ & 0xfu) << 50;
        packed |= (uint64_t)(in.waterfallQ & 0x3u) << 54;
        packed |= (uint64_t)(in.mineralPotQ & 0x3u) << 56;
        packed |= (uint64_t)(in.authority & 0x1u) << 58;
        if(error)*error=DescriptorDecodeError::None; return true;
    }
}
