#pragma once

#include "GraniteGeometryInternal.h"

// Private coordinate/profile helpers shared by formation and solid geometry.
namespace EE::GraniteFormationInternal
{
    using namespace GraniteGeometryInternal;

    static float GetGraniteFormationRotation(
        uint32_t worldSeed,
        uint32_t geologicalAncestryID )
    {
        uint32_t const seed = MixGraniteSeed( worldSeed, geologicalAncestryID );
        float const    signal = HashGraniteUnit( seed ^ 0xC2B2AE35u, 17, 29 );

        // Approximately +/- 17 degrees.
        return ( signal - 0.5f ) * 0.60f;
    }

    static float GraniteSuperellipseRadius(
        float along,
        float across,
        float radiusAlong,
        float radiusAcross,
        float exponent )
    {
        float const safeAlong = GraniteMax( radiusAlong, 0.05f );
        float const safeAcross = GraniteMax( radiusAcross, 0.05f );
        float const safeExponent = GraniteMax( exponent, 1.10f );

        float const u = GraniteAbs( along ) / safeAlong;
        float const v = GraniteAbs( across ) / safeAcross;

        return float(
            std::pow(
                std::pow( double( u ), double( safeExponent ) ) +
                    std::pow( double( v ), double( safeExponent ) ),
                1.0 / double( safeExponent ) ) );
    }

    static void GetGraniteFormationLocalCoordinates(
        GraniteFormationFormDescriptor const& descriptor,
        float                                 worldX,
        float                                 worldY,
        float&                                along,
        float&                                across )
    {
        float const cosine = float(
            std::cos( double( descriptor.m_orientationRadians ) ) );

        float const sine = float(
            std::sin( double( descriptor.m_orientationRadians ) ) );

        float const deltaX = worldX - descriptor.m_centerWorldX;
        float const deltaY = worldY - descriptor.m_centerWorldY;

        along = deltaX * cosine + deltaY * sine;
        across = -deltaX * sine + deltaY * cosine;
    }
}
