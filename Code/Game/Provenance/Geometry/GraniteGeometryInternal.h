#pragma once

#include "GraniteGeometry.h"
#include <cmath>

// Private shared primitives for the granite implementation units.
// Keep operation ordering and hash constants unchanged during extraction.
namespace EE::GraniteGeometryInternal
{
    //-------------------------------------------------------------------------
    // Local math
    //-------------------------------------------------------------------------

    static float GraniteClamp01( float value )
    {
        if ( value < 0.0f )
        {
            return 0.0f;
        }

        if ( value > 1.0f )
        {
            return 1.0f;
        }

        return value;
    }

    //-------------------------------------------------------------------------

    static float GraniteClampSigned( float value )
    {
        if ( value < -1.0f )
        {
            return -1.0f;
        }

        if ( value > 1.0f )
        {
            return 1.0f;
        }

        return value;
    }

    //-------------------------------------------------------------------------

    static float GraniteAbs( float value )
    {
        return value < 0.0f ? -value : value;
    }

    //-------------------------------------------------------------------------

    static float GraniteMin( float a, float b )
    {
        return a < b ? a : b;
    }

    //-------------------------------------------------------------------------

    static float GraniteMax( float a, float b )
    {
        return a > b ? a : b;
    }

    //-------------------------------------------------------------------------

    static float GraniteSmoothStep01( float value )
    {
        float const t = GraniteClamp01( value );
        return t * t * ( 3.0f - 2.0f * t );
    }

    //-------------------------------------------------------------------------

    static float GraniteLerp( float a, float b, float t )
    {
        return a + ( b - a ) * t;
    }

    //-------------------------------------------------------------------------
    // Deterministic hashing
    //-------------------------------------------------------------------------

    static uint32_t HashGraniteValue( uint32_t seed, int32_t x, int32_t y )
    {
        uint32_t h = seed;

        h ^= uint32_t( x ) * 0x9E3779B9u;
        h ^= uint32_t( y ) * 0x85EBCA6Bu;

        h ^= h >> 16;
        h *= 0x7FEB352Du;
        h ^= h >> 15;
        h *= 0x846CA68Bu;
        h ^= h >> 16;

        return h;
    }

    //-------------------------------------------------------------------------

    static uint32_t MixGraniteSeed(
        uint32_t worldSeed,
        uint32_t geologicalAncestryID,
        uint32_t bodyID = 0 )
    {
        uint32_t h = worldSeed ^ 0xA511E9B3u;

        h ^= geologicalAncestryID * 0x9E3779B9u;
        h ^= bodyID * 0x85EBCA6Bu;

        h ^= h >> 16;
        h *= 0x7FEB352Du;
        h ^= h >> 15;
        h *= 0x846CA68Bu;
        h ^= h >> 16;

        return h;
    }

    //-------------------------------------------------------------------------

    static float HashGraniteUnit( uint32_t seed, int32_t x, int32_t y )
    {
        uint32_t const value = HashGraniteValue( seed, x, y ) & 0x00FFFFFFu;
        return float( value ) / float( 0x00FFFFFFu );
    }

    //-------------------------------------------------------------------------
    // Absolute-coordinate deterministic value field
    //-------------------------------------------------------------------------

    static float SampleGraniteValueNoise(
        uint32_t seed,
        float    worldX,
        float    worldY,
        float    scale,
        uint32_t salt )
    {
        EE_ASSERT( scale > 0.0f );

        float const scaledX = worldX / scale;
        float const scaledY = worldY / scale;

        int32_t const gridX = int32_t( std::floor( double( scaledX ) ) );
        int32_t const gridY = int32_t( std::floor( double( scaledY ) ) );

        float const localX = scaledX - float( gridX );
        float const localY = scaledY - float( gridY );

        float const tx = GraniteSmoothStep01( localX );
        float const ty = GraniteSmoothStep01( localY );

        uint32_t const saltedSeed = seed ^ salt;

        float const v00 = HashGraniteUnit( saltedSeed, gridX, gridY );
        float const v10 = HashGraniteUnit( saltedSeed, gridX + 1, gridY );
        float const v01 = HashGraniteUnit( saltedSeed, gridX, gridY + 1 );
        float const v11 = HashGraniteUnit( saltedSeed, gridX + 1, gridY + 1 );

        float const bottom = GraniteLerp( v00, v10, tx );
        float const top = GraniteLerp( v01, v11, tx );

        return GraniteLerp( bottom, top, ty );
    }

    //-------------------------------------------------------------------------
    // Vector helpers
    //-------------------------------------------------------------------------

    static GranitePoint GraniteAdd( GranitePoint const& a, GranitePoint const& b )
    {
        GranitePoint result;
        result.m_x = a.m_x + b.m_x;
        result.m_y = a.m_y + b.m_y;
        result.m_z = a.m_z + b.m_z;
        return result;
    }

    //-------------------------------------------------------------------------

    static GranitePoint GraniteSubtract( GranitePoint const& a, GranitePoint const& b )
    {
        GranitePoint result;
        result.m_x = a.m_x - b.m_x;
        result.m_y = a.m_y - b.m_y;
        result.m_z = a.m_z - b.m_z;
        return result;
    }

    //-------------------------------------------------------------------------

    static GranitePoint GraniteScale( GranitePoint const& value, float scale )
    {
        GranitePoint result;
        result.m_x = value.m_x * scale;
        result.m_y = value.m_y * scale;
        result.m_z = value.m_z * scale;
        return result;
    }

    //-------------------------------------------------------------------------

    static GranitePoint GraniteCross( GranitePoint const& a, GranitePoint const& b )
    {
        GranitePoint result;

        result.m_x = a.m_y * b.m_z - a.m_z * b.m_y;
        result.m_y = a.m_z * b.m_x - a.m_x * b.m_z;
        result.m_z = a.m_x * b.m_y - a.m_y * b.m_x;

        return result;
    }

    //-------------------------------------------------------------------------

    static float GraniteDot( GranitePoint const& a, GranitePoint const& b )
    {
        return a.m_x * b.m_x + a.m_y * b.m_y + a.m_z * b.m_z;
    }

    //-------------------------------------------------------------------------

    static float GraniteLength( GranitePoint const& value )
    {
        return float( std::sqrt( double( GraniteDot( value, value ) ) ) );
    }

    //-------------------------------------------------------------------------

    static GranitePoint GraniteNormalize( GranitePoint const& value )
    {
        float const length = GraniteLength( value );

        if ( length <= 0.000001f )
        {
            return GranitePoint();
        }

        return GraniteScale( value, 1.0f / length );
    }

    //-------------------------------------------------------------------------

    static GranitePoint GraniteCombine(
        GranitePoint const& a,
        float               aScale,
        GranitePoint const& b,
        float               bScale )
    {
        GranitePoint result;
        result.m_x = a.m_x * aScale + b.m_x * bScale;
        result.m_y = a.m_y * aScale + b.m_y * bScale;
        result.m_z = a.m_z * aScale + b.m_z * bScale;
        return result;
    }

}
