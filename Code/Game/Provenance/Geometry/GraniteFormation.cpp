#include "GraniteGeometry.h"
#include <cmath>

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
    // Authority verification must report named receipt failures and return a
    // process exit code. It must never block CI on an interactive breakpoint.
    #undef EE_ASSERT
    #define EE_ASSERT( cond ) do { } while ( 0 )
#endif

#include "GraniteFormationInternal.h"

namespace EE
{
    using namespace GraniteGeometryInternal;
    using namespace GraniteFormationInternal;

    //-------------------------------------------------------------------------
    // Granite profile
    //-------------------------------------------------------------------------

    GraniteGeometryProfile GetGraniteGeometryProfile()
    {
        GraniteGeometryProfile profile;

        profile.m_angularity = 0.95f;
        profile.m_facePlanarity = 0.90f;
        profile.m_edgeWear = 0.08f;
        profile.m_slabBias = 0.35f;
        profile.m_elongationBias = 0.25f;

        profile.m_jointSpacingMeanM = 5.40f;
        profile.m_jointSpacingVariation = 0.46f;
        profile.m_jointPersistence = 0.78f;
        profile.m_jointApertureM = 0.045f;
        profile.m_jointHierarchyContrast = 0.72f;
        profile.m_coherentMassBias = 0.72f;

        profile.m_exfoliationStrength = 0.40f;
        profile.m_domeRounding = 0.28f;
        profile.m_surfaceRoughness = 0.18f;

        profile.m_smallFeatureBurialDepthM = 0.10f;
        profile.m_mediumFeatureBurialDepthM = 0.30f;
        profile.m_largeFeatureInfluenceDepthM = 0.75f;

        profile.m_surfaceHistory.m_inheritedExteriorBias = 0.42f;
        profile.m_surfaceHistory.m_weatheredRoundingStrength = 0.38f;
        profile.m_surfaceHistory.m_weatheredEdgeSoftening = 0.34f;
        profile.m_surfaceHistory.m_weatheredPlanarityRetention = 0.55f;
        profile.m_surfaceHistory.m_freshBreakSharpness = 0.95f;
        profile.m_surfaceHistory.m_freshBreakPlanarity = 0.92f;
        profile.m_surfaceHistory.m_freshBreakEdgeWear = 0.04f;
        profile.m_surfaceHistory.m_historyTransitionWidth = 0.12f;

        // Primary joint A: sparse, deep and persistent.
        profile.m_jointSets[0].m_tier = GraniteJointTier::Primary;
        profile.m_jointSets[0].m_orientationRadians = 0.20f;
        profile.m_jointSets[0].m_spacingMeanM = 5.20f;
        profile.m_jointSets[0].m_spacingVariation = 0.45f;
        profile.m_jointSets[0].m_persistence = 0.80f;
        profile.m_jointSets[0].m_apertureM = 0.050f;
        profile.m_jointSets[0].m_recessDepthM = 0.18f;
        profile.m_jointSets[0].m_expressionThreshold = 0.42f;

        // Primary/cross joint B: less persistent.
        profile.m_jointSets[1].m_tier = GraniteJointTier::Primary;
        profile.m_jointSets[1].m_orientationRadians = 1.76f;
        profile.m_jointSets[1].m_spacingMeanM = 7.80f;
        profile.m_jointSets[1].m_spacingVariation = 0.52f;
        profile.m_jointSets[1].m_persistence = 0.58f;
        profile.m_jointSets[1].m_apertureM = 0.038f;
        profile.m_jointSets[1].m_recessDepthM = 0.11f;
        profile.m_jointSets[1].m_expressionThreshold = 0.57f;

        // Subordinate joint C: more frequent, much weaker.
        profile.m_jointSets[2].m_tier = GraniteJointTier::Secondary;
        profile.m_jointSets[2].m_orientationRadians = 0.92f;
        profile.m_jointSets[2].m_spacingMeanM = 1.85f;
        profile.m_jointSets[2].m_spacingVariation = 0.55f;
        profile.m_jointSets[2].m_persistence = 0.34f;
        profile.m_jointSets[2].m_apertureM = 0.015f;
        profile.m_jointSets[2].m_recessDepthM = 0.030f;
        profile.m_jointSets[2].m_expressionThreshold = 0.70f;

        return profile;
    }

    //-------------------------------------------------------------------------
    // Formation structure helpers
    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------

    static float GetGraniteJointSpacing(
        GraniteJointSet const& jointSet,
        uint32_t               worldSeed,
        uint32_t               geologicalAncestryID,
        int32_t                jointSetIndex )
    {
        uint32_t const seed = MixGraniteSeed( worldSeed, geologicalAncestryID );
        float const    signal = HashGraniteUnit(
            seed ^ 0x27D4EB2Fu,
            jointSetIndex,
            91 );

        float const signedVariation = signal * 2.0f - 1.0f;

        return GraniteMax(
            jointSet.m_spacingMeanM *
                ( 1.0f + signedVariation * jointSet.m_spacingVariation * 0.55f ),
            0.15f );
    }

    //-------------------------------------------------------------------------

    static float GetGraniteJointPhase(
        uint32_t worldSeed,
        uint32_t geologicalAncestryID,
        int32_t  jointSetIndex,
        float    spacing )
    {
        uint32_t const seed = MixGraniteSeed( worldSeed, geologicalAncestryID );

        return HashGraniteUnit(
                   seed ^ 0x165667B1u,
                   jointSetIndex,
                   47 ) *
               spacing;
    }

    //-------------------------------------------------------------------------

    static void GetGraniteStructuralCoordinates(
        float  worldX,
        float  worldY,
        float  orientation,
        float& along,
        float& across )
    {
        float const cosine = float( std::cos( double( orientation ) ) );
        float const sine = float( std::sin( double( orientation ) ) );

        along = worldX * cosine + worldY * sine;
        across = -worldX * sine + worldY * cosine;
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 7B - curved Granite groove carves + broad relief modulation
    //
    // Step 7C-1 keeps the curved finite-groove carrier introduced in 7B, but
    // adds the missing large-surface language visible in the successful B3
    // hand-scale grammar:
    //
    //     coherent weathered host surface
    //     + rounded rock heads / broad backs
    //     + overlapping rounded lift / sink lobes
    //     + sparse deep curved grooves
    //     + more common narrow curved scars
    //     + occasional narrow raised seams with adjacent shallow sinks
    //
    // Relief is deterministic but varied per geological event. The broad host
    // remains quiet between events; scars and grooves taper in width, depth and
    // length. No package/chunk boundary participates in geological truth.
    //-------------------------------------------------------------------------

    static float GraniteLength2D( float x, float y )
    {
        return float( std::sqrt( double( x * x + y * y ) ) );
    }

    //-------------------------------------------------------------------------

    static float GraniteDistanceToSegment2D(
        float  pointX,
        float  pointY,
        float  aX,
        float  aY,
        float  bX,
        float  bY,
        float& segmentT )
    {
        float const abX = bX - aX;
        float const abY = bY - aY;
        float const abLengthSquared = abX * abX + abY * abY;

        if ( abLengthSquared <= 0.000001f )
        {
            segmentT = 0.0f;
            return GraniteLength2D( pointX - aX, pointY - aY );
        }

        float const apX = pointX - aX;
        float const apY = pointY - aY;

        segmentT = GraniteClamp01(
            ( apX * abX + apY * abY ) /
            abLengthSquared );

        float const closestX = aX + abX * segmentT;
        float const closestY = aY + abY * segmentT;

        return GraniteLength2D(
            pointX - closestX,
            pointY - closestY );
    }

    //-------------------------------------------------------------------------

    static float GraniteSegmentTerminationEnvelope( float segmentT )
    {
        // Smoothly terminate the visible carve at both ends. The event has a
        // true beginning and ending instead of being an infinite joint line.
        float const start = GraniteSmoothStep01( segmentT * 6.0f );
        float const end = GraniteSmoothStep01( ( 1.0f - segmentT ) * 6.0f );
        return start * end;
    }

    //-------------------------------------------------------------------------

    static float GraniteEllipticEnvelope(
        float worldX,
        float worldY,
        float centerX,
        float centerY,
        float orientation,
        float radiusAlong,
        float radiusAcross )
    {
        float const cosine = float( std::cos( double( orientation ) ) );
        float const sine = float( std::sin( double( orientation ) ) );

        float const deltaX = worldX - centerX;
        float const deltaY = worldY - centerY;

        float const along = deltaX * cosine + deltaY * sine;
        float const across = -deltaX * sine + deltaY * cosine;

        float const safeAlong = GraniteMax( radiusAlong, 0.05f );
        float const safeAcross = GraniteMax( radiusAcross, 0.05f );

        float const normalized = float(
            std::sqrt(
                double(
                    ( along * along ) / ( safeAlong * safeAlong ) +
                    ( across * across ) / ( safeAcross * safeAcross ) ) ) );

        return 1.0f - GraniteSmoothStep01( normalized );
    }

    //-------------------------------------------------------------------------
    // Formation-scale stone-body profiles
    //-------------------------------------------------------------------------

    static float GraniteSafePow01(
        float value,
        float exponent )
    {
        return float(
            std::pow(
                double( GraniteClamp01( value ) ),
                double( GraniteMax( exponent, 0.01f ) ) ) );
    }

    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------

    static float GraniteRootedStoneProfile(
        float outerRadius,
        float coreFraction,
        float crownExponent,
        float edgeHeightFraction )
    {
        if ( outerRadius >= 1.0f )
        {
            return 0.0f;
        }

        float const safeCore = GraniteMax(
            GraniteMin( coreFraction, 0.94f ),
            0.18f );

        float const safeEdge = GraniteMax(
            GraniteMin( edgeHeightFraction, 0.48f ),
            0.04f );

        if ( outerRadius <= safeCore )
        {
            float const localRadius = GraniteClamp01(
                outerRadius / safeCore );

            float const roundedCrown = GraniteSafePow01(
                GraniteMax(
                    1.0f - localRadius * localRadius,
                    0.0f ),
                crownExponent );

            return safeEdge +
                   ( 1.0f - safeEdge ) * roundedCrown;
        }

        float const rootT = GraniteClamp01(
            ( outerRadius - safeCore ) /
            GraniteMax( 1.0f - safeCore, 0.001f ) );

        return safeEdge *
               ( 1.0f - GraniteSmoothStep01( rootT ) );
    }

    //-------------------------------------------------------------------------

    static float GraniteSmoothLineBand(
        float signedDistance,
        float halfWidth )
    {
        return 1.0f -
               GraniteSmoothStep01(
                   GraniteAbs( signedDistance ) /
                   GraniteMax( halfWidth, 0.0001f ) );
    }

    //-------------------------------------------------------------------------

    static float GraniteObliqueCoordinate(
        float normalizedAlong,
        float normalizedAcross,
        float angleRadians )
    {
        float const cosine = float(
            std::cos( double( angleRadians ) ) );

        float const sine = float(
            std::sin( double( angleRadians ) ) );

        return normalizedAlong * cosine +
               normalizedAcross * sine;
    }

    //-------------------------------------------------------------------------
    // Curved groove geometry
    //-------------------------------------------------------------------------

    struct GraniteCurve2D
    {
        float m_p0X = 0.0f;
        float m_p0Y = 0.0f;
        float m_p1X = 0.0f;
        float m_p1Y = 0.0f;
        float m_p2X = 0.0f;
        float m_p2Y = 0.0f;
        float m_p3X = 0.0f;
        float m_p3Y = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct GraniteCurveDistanceSample
    {
        float m_distanceM = 1000000.0f;
        float m_curveT = 0.0f;
        float m_closestX = 0.0f;
        float m_closestY = 0.0f;
        float m_tangentX = 1.0f;
        float m_tangentY = 0.0f;
        float m_signedSideM = 0.0f;
    };

    //-------------------------------------------------------------------------

    static void EvaluateGraniteCubicCurve(
        GraniteCurve2D const& curve,
        float                 t,
        float&                outX,
        float&                outY )
    {
        float const clampedT = GraniteClamp01( t );
        float const oneMinusT = 1.0f - clampedT;

        float const b0 = oneMinusT * oneMinusT * oneMinusT;
        float const b1 = 3.0f * oneMinusT * oneMinusT * clampedT;
        float const b2 = 3.0f * oneMinusT * clampedT * clampedT;
        float const b3 = clampedT * clampedT * clampedT;

        outX =
            curve.m_p0X * b0 +
            curve.m_p1X * b1 +
            curve.m_p2X * b2 +
            curve.m_p3X * b3;

        outY =
            curve.m_p0Y * b0 +
            curve.m_p1Y * b1 +
            curve.m_p2Y * b2 +
            curve.m_p3Y * b3;
    }

    //-------------------------------------------------------------------------

    static void EvaluateGraniteCubicCurveTangent(
        GraniteCurve2D const& curve,
        float                 t,
        float&                outX,
        float&                outY )
    {
        float const clampedT = GraniteClamp01( t );
        float const oneMinusT = 1.0f - clampedT;

        outX =
            3.0f * oneMinusT * oneMinusT *
                ( curve.m_p1X - curve.m_p0X ) +
            6.0f * oneMinusT * clampedT *
                ( curve.m_p2X - curve.m_p1X ) +
            3.0f * clampedT * clampedT *
                ( curve.m_p3X - curve.m_p2X );

        outY =
            3.0f * oneMinusT * oneMinusT *
                ( curve.m_p1Y - curve.m_p0Y ) +
            6.0f * oneMinusT * clampedT *
                ( curve.m_p2Y - curve.m_p1Y ) +
            3.0f * clampedT * clampedT *
                ( curve.m_p3Y - curve.m_p2Y );

        float const length = GraniteLength2D( outX, outY );

        if ( length > 0.000001f )
        {
            outX /= length;
            outY /= length;
        }
        else
        {
            outX = 1.0f;
            outY = 0.0f;
        }
    }

    //-------------------------------------------------------------------------

    static GraniteCurveDistanceSample EvaluateGraniteCurveDistance(
        GraniteCurve2D const& curve,
        float                 worldX,
        float                 worldY,
        int32_t               segmentCount )
    {
        EE_ASSERT( segmentCount >= 4 );

        GraniteCurveDistanceSample result;

        float previousX = curve.m_p0X;
        float previousY = curve.m_p0Y;

        for ( int32_t segmentIndex = 0;
              segmentIndex < segmentCount;
              ++segmentIndex )
        {
            float const nextT =
                float( segmentIndex + 1 ) /
                float( segmentCount );

            float nextX = 0.0f;
            float nextY = 0.0f;

            EvaluateGraniteCubicCurve(
                curve,
                nextT,
                nextX,
                nextY );

            float localSegmentT = 0.0f;

            float const distance = GraniteDistanceToSegment2D(
                worldX,
                worldY,
                previousX,
                previousY,
                nextX,
                nextY,
                localSegmentT );

            if ( distance < result.m_distanceM )
            {
                float const segmentStartT =
                    float( segmentIndex ) /
                    float( segmentCount );

                result.m_distanceM = distance;
                result.m_curveT = GraniteLerp(
                    segmentStartT,
                    nextT,
                    localSegmentT );

                result.m_closestX = GraniteLerp(
                    previousX,
                    nextX,
                    localSegmentT );

                result.m_closestY = GraniteLerp(
                    previousY,
                    nextY,
                    localSegmentT );
            }

            previousX = nextX;
            previousY = nextY;
        }

        EvaluateGraniteCubicCurveTangent(
            curve,
            result.m_curveT,
            result.m_tangentX,
            result.m_tangentY );

        float const normalX = -result.m_tangentY;
        float const normalY = result.m_tangentX;

        result.m_signedSideM =
            ( worldX - result.m_closestX ) * normalX +
            ( worldY - result.m_closestY ) * normalY;

        return result;
    }

    //-------------------------------------------------------------------------

    static float GraniteSmoothWave01(
        float t,
        float phaseA,
        float phaseB )
    {
        float constexpr pi = 3.14159265358979323846f;

        float const waveA = float(
            std::sin(
                double(
                    t * pi * 2.0f +
                    phaseA ) ) );

        float const waveB = float(
            std::sin(
                double(
                    t * pi * 4.0f +
                    phaseB ) ) );

        return GraniteClamp01(
            0.50f +
            waveA * 0.30f +
            waveB * 0.14f );
    }

    //-------------------------------------------------------------------------

    static GraniteCurve2D BuildGraniteGrooveCurve(
        float centerX,
        float centerY,
        float orientation,
        float lengthM,
        float bendA,
        float bendB )
    {
        GraniteCurve2D curve;

        float const directionX = float(
            std::cos( double( orientation ) ) );

        float const directionY = float(
            std::sin( double( orientation ) ) );

        float const normalX = -directionY;
        float const normalY = directionX;

        float const halfLength = lengthM * 0.5f;

        curve.m_p0X = centerX - directionX * halfLength;
        curve.m_p0Y = centerY - directionY * halfLength;

        curve.m_p3X = centerX + directionX * halfLength;
        curve.m_p3Y = centerY + directionY * halfLength;

        curve.m_p1X =
            centerX - directionX * lengthM * 0.18f + normalX * bendA;

        curve.m_p1Y =
            centerY - directionY * lengthM * 0.18f + normalY * bendA;

        curve.m_p2X =
            centerX + directionX * lengthM * 0.18f + normalX * bendB;

        curve.m_p2Y =
            centerY + directionY * lengthM * 0.18f + normalY * bendB;

        return curve;
    }

    //-------------------------------------------------------------------------

    struct GraniteLargeSurfaceEventSample
    {
        float m_largeReliefM = 0.0f;
        float m_mediumReliefM = 0.0f;
        float m_smallReliefM = 0.0f;

        float m_primaryRecessM = 0.0f;
        float m_secondaryRecessM = 0.0f;

        float m_primaryInfluence = 0.0f;
        float m_secondaryInfluence = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Sparse continuous large-surface events
    //-------------------------------------------------------------------------

    static GraniteLargeSurfaceEventSample EvaluateGraniteLargeSurfaceEvents(
        GraniteGeometryProfile const& profile,
        uint32_t                      formationSeed,
        float                         formationRotation,
        float                         worldX,
        float                         worldY )
    {
        GraniteLargeSurfaceEventSample result;

        // Step 7C deliberately keeps event placement sparse and absolute-world
        // deterministic, but gives each event substantially more internal
        // shape than the Step 7B carriers. The goal is:
        //
        //     coherent host rock
        //     + rounded lifts / sinks
        //     + narrow finite scars
        //     + rare deep curved grooves
        //
        // Event cells are geological addressability only. Their borders never
        // appear in the generated surface.
        float constexpr eventCellSizeM = 9.0f;
        float constexpr pi = 3.14159265358979323846f;

        int32_t const baseCellX = int32_t(
            std::floor( double( worldX / eventCellSizeM ) ) );

        int32_t const baseCellY = int32_t(
            std::floor( double( worldY / eventCellSizeM ) ) );

        for ( int32_t cellOffsetY = -1; cellOffsetY <= 1; ++cellOffsetY )
        {
            for ( int32_t cellOffsetX = -1; cellOffsetX <= 1; ++cellOffsetX )
            {
                int32_t const cellX = baseCellX + cellOffsetX;
                int32_t const cellY = baseCellY + cellOffsetY;

                float const jitterX = HashGraniteUnit(
                    formationSeed ^ 0xA24BAED4u,
                    cellX,
                    cellY );

                float const jitterY = HashGraniteUnit(
                    formationSeed ^ 0x9FB21C65u,
                    cellX,
                    cellY );

                float const centerX =
                    ( float( cellX ) + 0.12f + jitterX * 0.76f ) *
                    eventCellSizeM;

                float const centerY =
                    ( float( cellY ) + 0.12f + jitterY * 0.76f ) *
                    eventCellSizeM;

                // -------------------------------------------------------------
                // Rounded coherent rock head / broad back
                // -------------------------------------------------------------

                float const rockHeadSignal = HashGraniteUnit(
                    formationSeed ^ 0x91E10DA5u,
                    cellX,
                    cellY );

                float const rockHeadActivation = GraniteSmoothStep01(
                    ( rockHeadSignal - 0.34f ) /
                    0.48f );

                if ( rockHeadActivation > 0.0f )
                {
                    float const orientation =
                        formationRotation +
                        ( HashGraniteUnit(
                              formationSeed ^ 0xC13FA9A9u,
                              cellX,
                              cellY ) -
                          0.5f ) *
                            1.35f;

                    float const radiusAlong =
                        2.25f +
                        HashGraniteUnit(
                            formationSeed ^ 0xD1B54A35u,
                            cellX,
                            cellY ) *
                            2.95f;

                    float const radiusAcross =
                        1.45f +
                        HashGraniteUnit(
                            formationSeed ^ 0x94D049BBu,
                            cellX,
                            cellY ) *
                            2.35f;

                    float const primaryEnvelope = GraniteEllipticEnvelope(
                        worldX,
                        worldY,
                        centerX,
                        centerY,
                        orientation,
                        radiusAlong,
                        radiusAcross );

                    // Broad shoulders plus a firmer center make the carrier
                    // read as a rounded granite back instead of one inflated
                    // Gaussian bump.
                    float const primaryBroad = float(
                        std::sqrt(
                            double(
                                GraniteClamp01( primaryEnvelope ) ) ) );

                    float const primaryCore =
                        primaryEnvelope *
                        primaryEnvelope;

                    float const offsetDirection =
                        orientation +
                        pi *
                            ( 0.36f +
                              HashGraniteUnit(
                                  formationSeed ^ 0x8538ECB5u,
                                  cellX,
                                  cellY ) *
                                  0.36f );

                    float const offsetMagnitude =
                        0.45f +
                        HashGraniteUnit(
                            formationSeed ^ 0x3C79AC49u,
                            cellX,
                            cellY ) *
                            1.25f;

                    float const secondaryCenterX =
                        centerX +
                        float( std::cos( double( offsetDirection ) ) ) *
                            offsetMagnitude;

                    float const secondaryCenterY =
                        centerY +
                        float( std::sin( double( offsetDirection ) ) ) *
                            offsetMagnitude;

                    float const secondaryEnvelope = GraniteEllipticEnvelope(
                        worldX,
                        worldY,
                        secondaryCenterX,
                        secondaryCenterY,
                        orientation +
                            ( HashGraniteUnit(
                                  formationSeed ^ 0xB7E15162u,
                                  cellX,
                                  cellY ) -
                              0.5f ) *
                                0.38f,
                        radiusAlong *
                            ( 0.52f +
                              HashGraniteUnit(
                                  formationSeed ^ 0x8AED2A6Bu,
                                  cellX,
                                  cellY ) *
                                  0.24f ),
                        radiusAcross *
                            ( 0.58f +
                              HashGraniteUnit(
                                  formationSeed ^ 0xBF715880u,
                                  cellX,
                                  cellY ) *
                                  0.24f ) );

                    float const secondaryBroad = float(
                        std::sqrt(
                            double(
                                GraniteClamp01( secondaryEnvelope ) ) ) );

                    float const heightM =
                        0.16f +
                        HashGraniteUnit(
                            formationSeed ^ 0xDA942042u,
                            cellX,
                            cellY ) *
                            0.52f;

                    float const roundedBody =
                        primaryBroad * 0.48f +
                        primaryCore * 0.30f +
                        secondaryBroad * 0.22f;

                    result.m_largeReliefM +=
                        rockHeadActivation *
                        heightM *
                        roundedBody;
                }

                // -------------------------------------------------------------
                // Rounded lift / sink lobes
                //
                // These are the terrain equivalent of the faint lines in the
                // hand sketch: broad swelling, shallow sagging and asymmetric
                // shoulders. They are not cracks.
                // -------------------------------------------------------------

                for ( int32_t lobeIndex = 0; lobeIndex < 2; ++lobeIndex )
                {
                    uint32_t const lobeSalt =
                        uint32_t( lobeIndex ) *
                        0x9E3779B9u;

                    float const lobeCenterX =
                        centerX +
                        ( HashGraniteUnit(
                              formationSeed ^
                                  ( 0xDB4F0B91u + lobeSalt ),
                              cellX,
                              cellY ) -
                          0.5f ) *
                            3.1f;

                    float const lobeCenterY =
                        centerY +
                        ( HashGraniteUnit(
                              formationSeed ^
                                  ( 0xA0F2EC75u + lobeSalt ),
                              cellX,
                              cellY ) -
                          0.5f ) *
                            3.1f;

                    float const lobeOrientation =
                        formationRotation +
                        ( HashGraniteUnit(
                              formationSeed ^
                                  ( 0x89E18285u + lobeSalt ),
                              cellX,
                              cellY ) -
                          0.5f ) *
                            1.70f;

                    float const lobeRadiusAlong =
                        1.65f +
                        HashGraniteUnit(
                            formationSeed ^
                                ( 0xC0B5A3E1u + lobeSalt ),
                            cellX,
                            cellY ) *
                            2.85f;

                    float const lobeRadiusAcross =
                        1.00f +
                        HashGraniteUnit(
                            formationSeed ^
                                ( 0xBBE05633u + lobeSalt ),
                            cellX,
                            cellY ) *
                            2.15f;

                    float const envelope = GraniteEllipticEnvelope(
                        worldX,
                        worldY,
                        lobeCenterX,
                        lobeCenterY,
                        lobeOrientation,
                        lobeRadiusAlong,
                        lobeRadiusAcross );

                    float const broadEnvelope = float(
                        std::sqrt(
                            double(
                                GraniteClamp01( envelope ) ) ) );

                    float const roundedEnvelope =
                        broadEnvelope * 0.62f +
                        envelope * envelope * 0.38f;

                    float const signedSignal =
                        HashGraniteUnit(
                            formationSeed ^
                                ( 0x6A09E667u + lobeSalt ),
                            cellX,
                            cellY ) *
                            2.0f -
                        1.0f;

                    float const amplitudeM =
                        0.035f +
                        HashGraniteUnit(
                            formationSeed ^
                                ( 0x3C6EF372u + lobeSalt ),
                            cellX,
                            cellY ) *
                            ( lobeIndex == 0
                                  ? 0.155f
                                  : 0.105f );

                    result.m_mediumReliefM +=
                        roundedEnvelope *
                        signedSignal *
                        amplitudeM;
                }

                // -------------------------------------------------------------
                // Rare deep curved structural groove
                // -------------------------------------------------------------

                float const primarySignal = HashGraniteUnit(
                    formationSeed ^ 0xF1357AE5u,
                    cellX,
                    cellY );

                float const primaryActivation = GraniteSmoothStep01(
                    ( primarySignal - 0.64f ) /
                    0.26f );

                if ( primaryActivation > 0.0f )
                {
                    int32_t const primaryFamily =
                        HashGraniteUnit(
                            formationSeed ^ 0x775A43BDu,
                            cellX,
                            cellY ) <
                                0.58f
                            ? 0
                            : 1;

                    float const familyOrientation =
                        profile.m_jointSets[primaryFamily].m_orientationRadians +
                        formationRotation;

                    float const grooveOrientation =
                        familyOrientation +
                        ( HashGraniteUnit(
                              formationSeed ^ 0xA4093822u,
                              cellX,
                              cellY ) -
                          0.5f ) *
                            1.00f;

                    float const grooveCenterX =
                        centerX +
                        ( HashGraniteUnit(
                              formationSeed ^ 0x299F31D0u,
                              cellX,
                              cellY ) -
                          0.5f ) *
                            2.6f;

                    float const grooveCenterY =
                        centerY +
                        ( HashGraniteUnit(
                              formationSeed ^ 0x082EFA98u,
                              cellX,
                              cellY ) -
                          0.5f ) *
                            2.6f;

                    float const grooveLengthM =
                        4.8f +
                        HashGraniteUnit(
                            formationSeed ^ 0xEC4E6C89u,
                            cellX,
                            cellY ) *
                            6.2f;

                    float const bendMagnitude =
                        0.38f +
                        HashGraniteUnit(
                            formationSeed ^ 0x452821E6u,
                            cellX,
                            cellY ) *
                            1.55f;

                    float const bendA =
                        ( HashGraniteUnit(
                              formationSeed ^ 0x38D01377u,
                              cellX,
                              cellY ) *
                              2.0f -
                          1.0f ) *
                        bendMagnitude;

                    float const bendB =
                        ( HashGraniteUnit(
                              formationSeed ^ 0xBE5466CFu,
                              cellX,
                              cellY ) *
                              2.0f -
                          1.0f ) *
                        bendMagnitude;

                    GraniteCurve2D const grooveCurve = BuildGraniteGrooveCurve(
                        grooveCenterX,
                        grooveCenterY,
                        grooveOrientation,
                        grooveLengthM,
                        bendA,
                        bendB );

                    GraniteCurveDistanceSample const grooveDistance =
                        EvaluateGraniteCurveDistance(
                            grooveCurve,
                            worldX,
                            worldY,
                            28 );

                    float const phaseA =
                        HashGraniteUnit(
                            formationSeed ^ 0x34E90C6Cu,
                            cellX,
                            cellY ) *
                        pi *
                        2.0f;

                    float const phaseB =
                        HashGraniteUnit(
                            formationSeed ^ 0xC0AC29B7u,
                            cellX,
                            cellY ) *
                        pi *
                        2.0f;

                    float const widthWave = GraniteSmoothWave01(
                        grooveDistance.m_curveT,
                        phaseA,
                        phaseB );

                    float const baseHalfWidthM =
                        0.34f +
                        HashGraniteUnit(
                            formationSeed ^ 0xC97C50DDu,
                            cellX,
                            cellY ) *
                            0.52f;

                    float const localHalfWidthM =
                        baseHalfWidthM *
                        ( 0.74f + widthWave * 0.46f );

                    float const shoulderHalfWidthM =
                        localHalfWidthM *
                        ( 1.85f +
                          HashGraniteUnit(
                              formationSeed ^ 0x3F84D5B5u,
                              cellX,
                              cellY ) *
                              0.55f );

                    float const coreHalfWidthM =
                        localHalfWidthM *
                        ( 0.30f +
                          HashGraniteUnit(
                              formationSeed ^ 0xB5470917u,
                              cellX,
                              cellY ) *
                              0.18f );

                    float const broadEnvelope =
                        1.0f -
                        GraniteSmoothStep01(
                            grooveDistance.m_distanceM /
                            GraniteMax( shoulderHalfWidthM, 0.08f ) );

                    float const bodyEnvelope =
                        1.0f -
                        GraniteSmoothStep01(
                            grooveDistance.m_distanceM /
                            GraniteMax( localHalfWidthM, 0.06f ) );

                    float const coreEnvelope =
                        1.0f -
                        GraniteSmoothStep01(
                            grooveDistance.m_distanceM /
                            GraniteMax( coreHalfWidthM, 0.04f ) );

                    float const terminationEnvelope =
                        GraniteSegmentTerminationEnvelope(
                            grooveDistance.m_curveT );

                    float const depthWave = GraniteSmoothWave01(
                        grooveDistance.m_curveT,
                        phaseB + 0.77f,
                        phaseA + 1.31f );

                    float const baseDepthM =
                        0.12f +
                        HashGraniteUnit(
                            formationSeed ^ 0x17A211D7u,
                            cellX,
                            cellY ) *
                            0.76f;

                    float const localDepthM =
                        baseDepthM *
                        ( 0.66f + depthWave * 0.52f );

                    float const grooveEnvelope =
                        primaryActivation *
                        terminationEnvelope *
                        ( broadEnvelope * 0.24f +
                          bodyEnvelope * 0.48f +
                          coreEnvelope * 0.28f );

                    result.m_primaryRecessM -=
                        grooveEnvelope *
                        localDepthM;

                    result.m_primaryInfluence = GraniteMax(
                        result.m_primaryInfluence,
                        grooveEnvelope );

                    // One broad rounded shoulder can stand proud of the rock
                    // beside the incision. This is explicitly part of the
                    // Granite geometry and can later participate in exposure.
                    float const shoulderSide =
                        HashGraniteUnit(
                            formationSeed ^ 0x6C44198Cu,
                            cellX,
                            cellY ) <
                                0.5f
                            ? -1.0f
                            : 1.0f;

                    float const signedShoulderDistance =
                        grooveDistance.m_signedSideM *
                        shoulderSide;

                    float const shoulderCenter =
                        localHalfWidthM *
                        1.12f;

                    float const shoulderRadius =
                        GraniteMax(
                            localHalfWidthM * 1.10f,
                            0.24f );

                    float const shoulderOffset =
                        GraniteAbs(
                            signedShoulderDistance -
                            shoulderCenter );

                    float const shoulderEnvelope =
                        1.0f -
                        GraniteSmoothStep01(
                            shoulderOffset /
                            shoulderRadius );

                    result.m_mediumReliefM +=
                        shoulderEnvelope *
                        terminationEnvelope *
                        primaryActivation *
                        ( 0.025f +
                          HashGraniteUnit(
                              formationSeed ^ 0x4CF5AD43u,
                              cellX,
                              cellY ) *
                              0.095f );
                }

                // -------------------------------------------------------------
                // Narrow curved scar / lifted seam
                //
                // Secondary structure is deliberately more common and much
                // narrower than a primary groove. Some events are incised;
                // others carry a raised core with a shallow adjacent scar.
                // -------------------------------------------------------------

                float const scarSignal = HashGraniteUnit(
                    formationSeed ^ 0x8B2D3C47u,
                    cellX,
                    cellY );

                float const scarActivation = GraniteSmoothStep01(
                    ( scarSignal - 0.42f ) /
                    0.42f );

                if ( scarActivation > 0.0f )
                {
                    float const scarOrientation =
                        profile.m_jointSets[2].m_orientationRadians +
                        formationRotation +
                        ( HashGraniteUnit(
                              formationSeed ^ 0xCB1AB31Fu,
                              cellX,
                              cellY ) -
                          0.5f ) *
                            1.35f;

                    float const scarCenterX =
                        centerX +
                        ( HashGraniteUnit(
                              formationSeed ^ 0xEFBE4786u,
                              cellX,
                              cellY ) -
                          0.5f ) *
                            4.2f;

                    float const scarCenterY =
                        centerY +
                        ( HashGraniteUnit(
                              formationSeed ^ 0x0FC19DC6u,
                              cellX,
                              cellY ) -
                          0.5f ) *
                            4.2f;

                    float const scarLengthM =
                        2.0f +
                        HashGraniteUnit(
                            formationSeed ^ 0x240CA1CCu,
                            cellX,
                            cellY ) *
                            4.6f;

                    float const scarBend =
                        0.14f +
                        HashGraniteUnit(
                            formationSeed ^ 0x2DE92C6Fu,
                            cellX,
                            cellY ) *
                            0.92f;

                    GraniteCurve2D const scarCurve = BuildGraniteGrooveCurve(
                        scarCenterX,
                        scarCenterY,
                        scarOrientation,
                        scarLengthM,
                        ( HashGraniteUnit(
                              formationSeed ^ 0x4A7484AAu,
                              cellX,
                              cellY ) *
                              2.0f -
                          1.0f ) *
                            scarBend,
                        ( HashGraniteUnit(
                              formationSeed ^ 0x5CB0A9DCu,
                              cellX,
                              cellY ) *
                              2.0f -
                          1.0f ) *
                            scarBend );

                    GraniteCurveDistanceSample const scarDistance =
                        EvaluateGraniteCurveDistance(
                            scarCurve,
                            worldX,
                            worldY,
                            24 );

                    float const scarPhaseA =
                        HashGraniteUnit(
                            formationSeed ^ 0x76F988DAu,
                            cellX,
                            cellY ) *
                        pi *
                        2.0f;

                    float const scarPhaseB =
                        HashGraniteUnit(
                            formationSeed ^ 0x983E5152u,
                            cellX,
                            cellY ) *
                        pi *
                        2.0f;

                    float const scarWave = GraniteSmoothWave01(
                        scarDistance.m_curveT,
                        scarPhaseA,
                        scarPhaseB );

                    float const baseHalfWidthM =
                        0.15f +
                        HashGraniteUnit(
                            formationSeed ^ 0xA831C66Du,
                            cellX,
                            cellY ) *
                            0.24f;

                    float const localHalfWidthM =
                        baseHalfWidthM *
                        ( 0.78f + scarWave * 0.38f );

                    // A broad soft falloff prevents the 0.25m certification
                    // triangulation from becoming the visible scar boundary.
                    float const broadHalfWidthM =
                        localHalfWidthM *
                        ( 1.85f +
                          HashGraniteUnit(
                              formationSeed ^ 0xA64C2F8Du,
                              cellX,
                              cellY ) *
                              0.45f );

                    float const coreHalfWidthM =
                        GraniteMax(
                            localHalfWidthM * 0.46f,
                            0.055f );

                    float const broadEnvelope =
                        1.0f -
                        GraniteSmoothStep01(
                            scarDistance.m_distanceM /
                            GraniteMax( broadHalfWidthM, 0.08f ) );

                    float const bodyEnvelope =
                        1.0f -
                        GraniteSmoothStep01(
                            scarDistance.m_distanceM /
                            GraniteMax( localHalfWidthM, 0.06f ) );

                    float const coreEnvelope =
                        1.0f -
                        GraniteSmoothStep01(
                            scarDistance.m_distanceM /
                            coreHalfWidthM );

                    float const terminationEnvelope =
                        GraniteSegmentTerminationEnvelope(
                            scarDistance.m_curveT );

                    float const scarMode = HashGraniteUnit(
                        formationSeed ^ 0x510E527Fu,
                        cellX,
                        cellY );

                    float const scarDepthM =
                        0.018f +
                        HashGraniteUnit(
                            formationSeed ^ 0x9B05688Cu,
                            cellX,
                            cellY ) *
                            0.150f;

                    if ( scarMode < 0.72f )
                    {
                        // Incised scar with a softly rounded outer shoulder.
                        float const scarEnvelope =
                            scarActivation *
                            terminationEnvelope *
                            ( broadEnvelope * 0.24f +
                              bodyEnvelope * 0.48f +
                              coreEnvelope * 0.28f );

                        result.m_secondaryRecessM -=
                            scarEnvelope *
                            scarDepthM;

                        result.m_secondaryInfluence = GraniteMax(
                            result.m_secondaryInfluence,
                            scarEnvelope );

                        // A small raised lip may survive on one side.
                        float const lipSide =
                            HashGraniteUnit(
                                formationSeed ^ 0x13198A2Eu,
                                cellX,
                                cellY ) <
                                    0.5f
                                ? -1.0f
                                : 1.0f;

                        float const lipCenter =
                            lipSide *
                            localHalfWidthM *
                            1.18f;

                        float const lipDistance = GraniteAbs(
                            scarDistance.m_signedSideM -
                            lipCenter );

                        float const lipEnvelope =
                            1.0f -
                            GraniteSmoothStep01(
                                lipDistance /
                                GraniteMax(
                                    localHalfWidthM * 0.78f,
                                    0.10f ) );

                        result.m_mediumReliefM +=
                            lipEnvelope *
                            terminationEnvelope *
                            scarActivation *
                            ( 0.012f +
                              HashGraniteUnit(
                                  formationSeed ^ 0x243185BEu,
                                  cellX,
                                  cellY ) *
                                  0.050f );
                    }
                    else
                    {
                        // Raised seam: narrow, rounded and geological. It is
                        // not a material boundary; it is a positive Granite
                        // relief feature that can later promote exposure where
                        // Soil cover becomes thin enough.
                        float const ridgeEnvelope =
                            scarActivation *
                            terminationEnvelope *
                            ( broadEnvelope * 0.20f +
                              bodyEnvelope * 0.38f +
                              coreEnvelope * 0.42f );

                        float const ridgeHeightM =
                            0.035f +
                            HashGraniteUnit(
                                formationSeed ^ 0x629A292Au,
                                cellX,
                                cellY ) *
                                0.135f;

                        result.m_mediumReliefM +=
                            ridgeEnvelope *
                            ridgeHeightM;

                        result.m_secondaryInfluence = GraniteMax(
                            result.m_secondaryInfluence,
                            ridgeEnvelope );

                        // A faint parallel sink on one side prevents the ridge
                        // from reading as an artificial embossed strip.
                        float const sinkSide =
                            HashGraniteUnit(
                                formationSeed ^ 0xCBBB9D5Du,
                                cellX,
                                cellY ) <
                                    0.5f
                                ? -1.0f
                                : 1.0f;

                        float const sinkCenter =
                            sinkSide *
                            localHalfWidthM *
                            1.55f;

                        float const sinkDistance = GraniteAbs(
                            scarDistance.m_signedSideM -
                            sinkCenter );

                        float const sinkEnvelope =
                            1.0f -
                            GraniteSmoothStep01(
                                sinkDistance /
                                GraniteMax(
                                    localHalfWidthM * 0.92f,
                                    0.11f ) );

                        result.m_secondaryRecessM -=
                            sinkEnvelope *
                            terminationEnvelope *
                            scarActivation *
                            ( 0.008f +
                              scarDepthM * 0.26f );
                    }
                }
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 7C-1B - boulder-scale surface-stone grammar
    //
    // The event lattice below is only a deterministic ADDRESS for geological
    // events. Cell boundaries never appear in the surface and query bounds
    // never participate in event identity or shape.
    //
    // AttachedRockHead
    //     Positive coherent Granite relief. It remains parent bedrock.
    //
    // PartiallyDetachedBlock
    //     Positive coherent relief plus a bounded joint-controlled separation
    //     scar. It remains parent bedrock until later structural failure.
    //
    // DetachedSurfaceStoneCandidate
    //     Descriptor only. It contributes ZERO parent-bedrock elevation.
    //-------------------------------------------------------------------------

    static constexpr float   s_graniteSurfaceStoneCellSizeM = 12.0f;
    static constexpr int32_t s_graniteSurfaceStoneSlotsPerCell = 2;

    //-------------------------------------------------------------------------

    static GraniteSurfaceStoneDescriptor BuildGraniteSurfaceStoneDescriptor(
        uint32_t formationSeed,
        uint32_t geologicalAncestryID,
        float    formationRotation,
        int32_t  cellX,
        int32_t  cellY,
        int32_t  slotIndex )
    {
        EE_ASSERT( slotIndex >= 0 );
        EE_ASSERT( slotIndex < s_graniteSurfaceStoneSlotsPerCell );

        GraniteSurfaceStoneDescriptor descriptor;

        uint32_t const slotSalt =
            uint32_t( slotIndex ) * 0x00009E37u;

        float const activationSignal = HashGraniteUnit(
            formationSeed ^ 0x6E624EB7u ^ slotSalt,
            cellX,
            cellY );

        float const activationThreshold =
            slotIndex == 0
                ? 0.28f
                : 0.46f;

        if ( activationSignal < activationThreshold )
        {
            return descriptor;
        }

        float const jitterX = HashGraniteUnit(
            formationSeed ^ ( 0xA341316Cu + slotSalt ),
            cellX,
            cellY );

        float const jitterY = HashGraniteUnit(
            formationSeed ^ ( 0xC8013EA4u + slotSalt ),
            cellX,
            cellY );

        descriptor.m_centerWorldX =
            ( float( cellX ) + 0.14f + jitterX * 0.72f ) *
            s_graniteSurfaceStoneCellSizeM;

        descriptor.m_centerWorldY =
            ( float( cellY ) + 0.14f + jitterY * 0.72f ) *
            s_graniteSurfaceStoneCellSizeM;

        uint32_t eventID = HashGraniteValue(
            formationSeed ^ 0xD6E8FEB9u ^ slotSalt,
            cellX,
            cellY );

        if ( eventID == 0 )
        {
            eventID = 1u;
        }

        descriptor.m_valid = true;
        descriptor.m_eventID = eventID;
        descriptor.m_geologicalAncestryID = geologicalAncestryID;

        descriptor.m_bodyGrammarSalt = HashGraniteValue(
            formationSeed ^ 0xA5A35625u ^ slotSalt,
            cellX,
            cellY );

        float const stateSignal = HashGraniteUnit(
            formationSeed ^ ( 0xAD90777Du + slotSalt ),
            cellX,
            cellY );

        if ( slotIndex == 0 )
        {
            descriptor.m_state =
                stateSignal < 0.72f
                    ? GraniteSurfaceStoneState::AttachedRockHead
                    : GraniteSurfaceStoneState::PartiallyDetachedBlock;

            descriptor.m_majorRadiusM =
                0.58f +
                HashGraniteUnit(
                    formationSeed ^ 0x7E95761Eu,
                    cellX,
                    cellY ) *
                    0.72f;

            descriptor.m_minorRadiusM =
                0.42f +
                HashGraniteUnit(
                    formationSeed ^ 0xD1B54A32u,
                    cellX,
                    cellY ) *
                    0.52f;

            descriptor.m_maximumReliefM =
                0.28f +
                HashGraniteUnit(
                    formationSeed ^ 0x94D049BBu,
                    cellX,
                    cellY ) *
                    0.58f;

            descriptor.m_embeddingDepthM =
                0.16f +
                HashGraniteUnit(
                    formationSeed ^ 0x369DEA0Fu,
                    cellX,
                    cellY ) *
                    0.36f;
        }
        else
        {
            descriptor.m_state =
                stateSignal < 0.68f
                    ? GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate
                    : GraniteSurfaceStoneState::PartiallyDetachedBlock;

            descriptor.m_majorRadiusM =
                0.20f +
                HashGraniteUnit(
                    formationSeed ^ 0x7E95761Eu,
                    cellX + 13,
                    cellY ) *
                    0.32f;

            descriptor.m_minorRadiusM =
                0.15f +
                HashGraniteUnit(
                    formationSeed ^ 0xD1B54A32u,
                    cellX,
                    cellY + 17 ) *
                    0.25f;

            descriptor.m_maximumReliefM =
                0.08f +
                HashGraniteUnit(
                    formationSeed ^ 0x94D049BBu,
                    cellX + 19,
                    cellY ) *
                    0.25f;

            descriptor.m_embeddingDepthM =
                0.035f +
                HashGraniteUnit(
                    formationSeed ^ 0x369DEA0Fu,
                    cellX,
                    cellY + 23 ) *
                    0.16f;
        }

        float const orientationVariation =
            ( HashGraniteUnit(
                  formationSeed ^ 0xDB4F0B91u ^ slotSalt,
                  cellX,
                  cellY ) -
              0.5f ) *
            2.35f;

        descriptor.m_orientationRadians =
            formationRotation +
            orientationVariation;

        descriptor.m_weatheredExteriorWeight =
            GraniteClamp01(
                0.44f +
                HashGraniteUnit(
                    formationSeed ^ 0xBBE05633u ^ slotSalt,
                    cellX,
                    cellY ) *
                    0.52f );

        float const isolationSignal = HashGraniteUnit(
            formationSeed ^ 0xA0F2EC75u ^ slotSalt,
            cellX,
            cellY );

        if ( descriptor.m_state == GraniteSurfaceStoneState::AttachedRockHead )
        {
            descriptor.m_jointIsolation =
                0.08f + isolationSignal * 0.34f;

            descriptor.m_remainingConnection =
                0.68f +
                HashGraniteUnit(
                    formationSeed ^ 0x89E18285u ^ slotSalt,
                    cellX,
                    cellY ) *
                    0.32f;
        }
        else if ( descriptor.m_state == GraniteSurfaceStoneState::PartiallyDetachedBlock )
        {
            descriptor.m_jointIsolation =
                0.54f + isolationSignal * 0.42f;

            descriptor.m_remainingConnection =
                0.12f +
                HashGraniteUnit(
                    formationSeed ^ 0x89E18285u ^ slotSalt,
                    cellX,
                    cellY ) *
                    0.43f;
        }
        else
        {
            descriptor.m_jointIsolation =
                0.90f + isolationSignal * 0.10f;

            descriptor.m_remainingConnection = 0.0f;
        }

        descriptor.m_downslopeBias =
            HashGraniteUnit(
                formationSeed ^ 0xC2B2AE3Du ^ slotSalt,
                cellX,
                cellY ) *
                2.0f -
            1.0f;

        return descriptor;
    }

    //-------------------------------------------------------------------------

    static bool GraniteSurfaceStoneIntersectsBounds(
        GraniteSurfaceStoneDescriptor const& descriptor,
        float                                minWorldX,
        float                                minWorldY,
        float                                maxWorldX,
        float                                maxWorldY )
    {
        if ( !descriptor.m_valid )
        {
            return false;
        }

        float const radius = GraniteMax(
            descriptor.m_majorRadiusM,
            descriptor.m_minorRadiusM );

        return descriptor.m_centerWorldX + radius >= minWorldX &&
               descriptor.m_centerWorldX - radius <= maxWorldX &&
               descriptor.m_centerWorldY + radius >= minWorldY &&
               descriptor.m_centerWorldY - radius <= maxWorldY;
    }

    //-------------------------------------------------------------------------

    static float GetGraniteSurfaceStoneImportance(
        GraniteSurfaceStoneDescriptor const& descriptor )
    {
        float stateBias = 0.0f;

        switch ( descriptor.m_state )
        {
            case GraniteSurfaceStoneState::AttachedRockHead:
                stateBias = 3.0f;
                break;

            case GraniteSurfaceStoneState::PartiallyDetachedBlock:
                stateBias = 2.0f;
                break;

            case GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate:
                stateBias = 1.0f;
                break;
        }

        return stateBias +
               descriptor.m_maximumReliefM * 2.0f +
               descriptor.m_majorRadiusM * 0.35f;
    }

    //-------------------------------------------------------------------------

    static void InsertGraniteSurfaceStoneDescriptor(
        GraniteSurfaceStoneField&            field,
        GraniteSurfaceStoneDescriptor const& descriptor )
    {
        if ( !descriptor.m_valid )
        {
            return;
        }

        if ( field.m_numDescriptors < GraniteSurfaceStoneField::s_maxDescriptors )
        {
            field.m_descriptors[field.m_numDescriptors] = descriptor;
            ++field.m_numDescriptors;
            return;
        }

        int32_t weakestIndex = 0;
        float   weakestImportance = GetGraniteSurfaceStoneImportance(
            field.m_descriptors[0] );

        for ( int32_t i = 1; i < field.m_numDescriptors; ++i )
        {
            float const importance = GetGraniteSurfaceStoneImportance(
                field.m_descriptors[i] );

            if ( importance < weakestImportance ||
                 ( importance == weakestImportance &&
                   field.m_descriptors[i].m_eventID >
                       field.m_descriptors[weakestIndex].m_eventID ) )
            {
                weakestIndex = i;
                weakestImportance = importance;
            }
        }

        float const candidateImportance = GetGraniteSurfaceStoneImportance(
            descriptor );

        if ( candidateImportance > weakestImportance ||
             ( candidateImportance == weakestImportance &&
               descriptor.m_eventID <
                   field.m_descriptors[weakestIndex].m_eventID ) )
        {
            field.m_descriptors[weakestIndex] = descriptor;
        }
    }

    //-------------------------------------------------------------------------

    static void SortGraniteSurfaceStoneField(
        GraniteSurfaceStoneField& field )
    {
        for ( int32_t i = 1; i < field.m_numDescriptors; ++i )
        {
            GraniteSurfaceStoneDescriptor value = field.m_descriptors[i];
            float const                   valueImportance = GetGraniteSurfaceStoneImportance( value );

            int32_t destination = i;

            while ( destination > 0 )
            {
                GraniteSurfaceStoneDescriptor const& previous =
                    field.m_descriptors[destination - 1];

                float const previousImportance =
                    GetGraniteSurfaceStoneImportance( previous );

                bool const shouldMove =
                    valueImportance > previousImportance ||
                    ( valueImportance == previousImportance &&
                      value.m_eventID < previous.m_eventID );

                if ( !shouldMove )
                {
                    break;
                }

                field.m_descriptors[destination] = previous;
                --destination;
            }

            field.m_descriptors[destination] = value;
        }
    }

    //-------------------------------------------------------------------------

    struct GraniteSurfaceStoneReliefSample
    {
        float m_rockHeadReliefM = 0.0f;
        float m_partiallyDetachedReliefM = 0.0f;
        float m_shoulderReliefM = 0.0f;
        float m_detachmentRecessM = 0.0f;
        float m_detachmentInfluence = 0.0f;
    };

    //-------------------------------------------------------------------------

    static GraniteSurfaceStoneReliefSample EvaluateGraniteSurfaceStoneRelief(
        uint32_t worldSeed,
        uint32_t geologicalAncestryID,
        float    worldX,
        float    worldY )
    {
        GraniteSurfaceStoneReliefSample result;

        uint32_t const formationSeed = MixGraniteSeed(
            worldSeed,
            geologicalAncestryID );

        float const formationRotation = GetGraniteFormationRotation(
            worldSeed,
            geologicalAncestryID );

        int32_t const baseCellX = int32_t(
            std::floor(
                double(
                    worldX /
                    s_graniteSurfaceStoneCellSizeM ) ) );

        int32_t const baseCellY = int32_t(
            std::floor(
                double(
                    worldY /
                    s_graniteSurfaceStoneCellSizeM ) ) );

        for ( int32_t cellOffsetY = -1; cellOffsetY <= 1; ++cellOffsetY )
        {
            for ( int32_t cellOffsetX = -1; cellOffsetX <= 1; ++cellOffsetX )
            {
                int32_t const cellX = baseCellX + cellOffsetX;
                int32_t const cellY = baseCellY + cellOffsetY;

                for ( int32_t slotIndex = 0;
                      slotIndex < s_graniteSurfaceStoneSlotsPerCell;
                      ++slotIndex )
                {
                    GraniteSurfaceStoneDescriptor const descriptor =
                        BuildGraniteSurfaceStoneDescriptor(
                            formationSeed,
                            geologicalAncestryID,
                            formationRotation,
                            cellX,
                            cellY,
                            slotIndex );

                    if ( !descriptor.m_valid ||
                         descriptor.m_state ==
                             GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate )
                    {
                        continue;
                    }

                    float const primaryEnvelope = GraniteEllipticEnvelope(
                        worldX,
                        worldY,
                        descriptor.m_centerWorldX,
                        descriptor.m_centerWorldY,
                        descriptor.m_orientationRadians,
                        descriptor.m_majorRadiusM,
                        descriptor.m_minorRadiusM );

                    if ( primaryEnvelope <= 0.0f )
                    {
                        continue;
                    }

                    float const broadEnvelope = float(
                        std::sqrt(
                            double(
                                GraniteClamp01( primaryEnvelope ) ) ) );

                    float const coreEnvelope =
                        primaryEnvelope *
                        primaryEnvelope;

                    float const secondaryDirection =
                        descriptor.m_orientationRadians +
                        ( HashGraniteUnit(
                              descriptor.m_eventID ^ 0x1B873593u,
                              7,
                              13 ) -
                          0.5f ) *
                            1.35f;

                    float const secondaryOffset =
                        descriptor.m_majorRadiusM *
                        ( 0.18f +
                          HashGraniteUnit(
                              descriptor.m_eventID ^ 0x85EBCA6Bu,
                              17,
                              5 ) *
                              0.18f );

                    float const secondaryCenterX =
                        descriptor.m_centerWorldX +
                        float(
                            std::cos(
                                double(
                                    secondaryDirection ) ) ) *
                            secondaryOffset;

                    float const secondaryCenterY =
                        descriptor.m_centerWorldY +
                        float(
                            std::sin(
                                double(
                                    secondaryDirection ) ) ) *
                            secondaryOffset;

                    float const secondaryEnvelope = GraniteEllipticEnvelope(
                        worldX,
                        worldY,
                        secondaryCenterX,
                        secondaryCenterY,
                        descriptor.m_orientationRadians +
                            ( HashGraniteUnit(
                                  descriptor.m_eventID ^ 0xC2B2AE35u,
                                  3,
                                  29 ) -
                              0.5f ) *
                                0.42f,
                        descriptor.m_majorRadiusM * 0.72f,
                        descriptor.m_minorRadiusM * 0.76f );

                    float const secondaryBroad = float(
                        std::sqrt(
                            double(
                                GraniteClamp01( secondaryEnvelope ) ) ) );

                    float const articulatedBody =
                        broadEnvelope * 0.50f +
                        coreEnvelope * 0.30f +
                        secondaryBroad * 0.20f;

                    float const bodyRelief =
                        descriptor.m_maximumReliefM *
                        articulatedBody;

                    // Local shoulder/knuckle variation uses a smaller nearby
                    // lobe. This brings the successful B3 meso-language into
                    // the rooted rock head without making it a hemisphere.
                    float const shoulderDirection =
                        descriptor.m_orientationRadians +
                        1.57079632679489661923f *
                            ( HashGraniteUnit(
                                  descriptor.m_eventID ^ 0x27D4EB2Fu,
                                  11,
                                  19 ) <
                                      0.5f
                                  ? -1.0f
                                  : 1.0f );

                    float const shoulderCenterX =
                        descriptor.m_centerWorldX +
                        float(
                            std::cos(
                                double(
                                    shoulderDirection ) ) ) *
                            descriptor.m_minorRadiusM * 0.52f;

                    float const shoulderCenterY =
                        descriptor.m_centerWorldY +
                        float(
                            std::sin(
                                double(
                                    shoulderDirection ) ) ) *
                            descriptor.m_minorRadiusM * 0.52f;

                    float const shoulderEnvelope = GraniteEllipticEnvelope(
                        worldX,
                        worldY,
                        shoulderCenterX,
                        shoulderCenterY,
                        descriptor.m_orientationRadians,
                        descriptor.m_majorRadiusM * 0.52f,
                        descriptor.m_minorRadiusM * 0.48f );

                    float const shoulderHeight =
                        descriptor.m_maximumReliefM *
                        ( 0.045f +
                          HashGraniteUnit(
                              descriptor.m_eventID ^ 0x165667B1u,
                              23,
                              31 ) *
                              0.085f );

                    result.m_shoulderReliefM +=
                        shoulderEnvelope *
                        shoulderHeight;

                    if ( descriptor.m_state ==
                         GraniteSurfaceStoneState::AttachedRockHead )
                    {
                        result.m_rockHeadReliefM += bodyRelief;
                    }
                    else
                    {
                        result.m_partiallyDetachedReliefM += bodyRelief;

                        float const cosine = float(
                            std::cos(
                                double(
                                    descriptor.m_orientationRadians ) ) );

                        float const sine = float(
                            std::sin(
                                double(
                                    descriptor.m_orientationRadians ) ) );

                        float const deltaX =
                            worldX - descriptor.m_centerWorldX;

                        float const deltaY =
                            worldY - descriptor.m_centerWorldY;

                        float const localAlong =
                            deltaX * cosine +
                            deltaY * sine;

                        float const localAcross =
                            -deltaX * sine +
                            deltaY * cosine;

                        float const sideSign =
                            HashGraniteUnit(
                                descriptor.m_eventID ^ 0x94D049BBu,
                                37,
                                41 ) <
                                    0.5f
                                ? -1.0f
                                : 1.0f;

                        float const separationCenter =
                            sideSign *
                            descriptor.m_minorRadiusM *
                            ( 0.62f +
                              descriptor.m_jointIsolation * 0.18f );

                        float const separationWidth =
                            GraniteMax(
                                descriptor.m_minorRadiusM *
                                    ( 0.12f +
                                      descriptor.m_jointIsolation * 0.08f ),
                                0.07f );

                        float const separationDistance = GraniteAbs(
                            localAcross - separationCenter );

                        float const acrossEnvelope =
                            1.0f -
                            GraniteSmoothStep01(
                                separationDistance /
                                separationWidth );

                        float const alongEnvelope =
                            1.0f -
                            GraniteSmoothStep01(
                                GraniteAbs( localAlong ) /
                                GraniteMax(
                                    descriptor.m_majorRadiusM * 0.82f,
                                    0.10f ) );

                        float const detachmentEnvelope =
                            acrossEnvelope *
                            alongEnvelope *
                            primaryEnvelope;

                        float const detachmentDepth =
                            0.055f +
                            HashGraniteUnit(
                                descriptor.m_eventID ^ 0xD3A2646Cu,
                                43,
                                47 ) *
                                0.19f;

                        result.m_detachmentRecessM -=
                            detachmentEnvelope *
                            detachmentDepth *
                            descriptor.m_jointIsolation;

                        result.m_detachmentInfluence = GraniteMax(
                            result.m_detachmentInfluence,
                            detachmentEnvelope *
                                descriptor.m_jointIsolation );
                    }
                }
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 7D - continuous formation-scale Granite inheritance
    //
    // These event grids are deterministic geological ADDRESSING only. The
    // event descriptors are generated from world seed + ancestry + absolute
    // cell coordinate. Query bounds merely select existing events.
    //
    // Form grammar:
    //
    //     BroadWeatheredBack
    //     RootedRockHead
    //     SlabShoulder
    //
    // Structural grammar:
    //
    //     finite curved joints
    //     variable depth along path
    //     variable aperture along path
    //     asymmetric raised shoulders
    //     tapered/closing ends
    //
    // A boulder-like silhouette does not imply a detached body. Rooted rock
    // heads remain continuous bedrock and return to zero relief through an
    // explicit root-blend zone.
    //-------------------------------------------------------------------------

    static constexpr float   s_graniteFormationFormCellSizeM = 14.0f;
    static constexpr int32_t s_graniteFormationFormSlotsPerCell = 4;

    static constexpr float   s_graniteFormationJointCellSizeM = 16.0f;
    static constexpr int32_t s_graniteFormationJointSlotsPerCell = 2;

    //-------------------------------------------------------------------------

    static float EvaluateGraniteCubicScalar(
        float p0,
        float p1,
        float p2,
        float p3,
        float t )
    {
        float const clampedT = GraniteClamp01( t );
        float const oneMinusT = 1.0f - clampedT;

        float const b0 = oneMinusT * oneMinusT * oneMinusT;
        float const b1 = 3.0f * oneMinusT * oneMinusT * clampedT;
        float const b2 = 3.0f * oneMinusT * clampedT * clampedT;
        float const b3 = clampedT * clampedT * clampedT;

        return p0 * b0 +
               p1 * b1 +
               p2 * b2 +
               p3 * b3;
    }

    //-------------------------------------------------------------------------

    static float CombineGranitePositiveRelief(
        float current,
        float contribution )
    {
        if ( contribution <= 0.0f )
        {
            return current;
        }

        if ( current <= 0.0f )
        {
            return contribution;
        }

        // Keep one readable dominant Granite body. Subordinate overlapping
        // structures may fuse into it as shoulders/knuckles, but must not turn
        // every overlap into one generic RMS mound.
        float const high = GraniteMax( current, contribution );
        float const low = GraniteMin( current, contribution );
        float const relative = low / GraniteMax( high, 0.000001f );

        return high +
               low *
                   GraniteLerp(
                       0.18f,
                       0.08f,
                       GraniteClamp01( relative ) );
    }

    //-------------------------------------------------------------------------

    static GraniteFormationFormDescriptor BuildGraniteFormationFormDescriptor(
        uint32_t formationSeed,
        uint32_t geologicalAncestryID,
        float    formationRotation,
        int32_t  cellX,
        int32_t  cellY,
        int32_t  slotIndex )
    {
        EE_ASSERT( slotIndex >= 0 );
        EE_ASSERT( slotIndex < s_graniteFormationFormSlotsPerCell );

        GraniteFormationFormDescriptor descriptor;

        uint32_t const slotSalt =
            uint32_t( slotIndex ) * 0x00009E37u;

        float const activationSignal = HashGraniteUnit(
            formationSeed ^ 0x2C1B3C6Du ^ slotSalt,
            cellX,
            cellY );

        float activationThreshold = 0.50f;

        switch ( slotIndex )
        {
            case 0: activationThreshold = 0.57f; break; // Broad back
            case 1: activationThreshold = 0.34f; break; // Main rooted head
            case 2: activationThreshold = 0.56f; break; // Secondary rooted head
            case 3: activationThreshold = 0.50f; break; // Slab shoulder
            default: break;
        }

        if ( activationSignal < activationThreshold )
        {
            return descriptor;
        }

        float const jitterX = HashGraniteUnit(
            formationSeed ^ ( 0xA341316Cu + slotSalt ),
            cellX,
            cellY );

        float const jitterY = HashGraniteUnit(
            formationSeed ^ ( 0xC8013EA4u + slotSalt ),
            cellX,
            cellY );

        descriptor.m_centerWorldX =
            ( float( cellX ) + 0.10f + jitterX * 0.80f ) *
            s_graniteFormationFormCellSizeM;

        descriptor.m_centerWorldY =
            ( float( cellY ) + 0.10f + jitterY * 0.80f ) *
            s_graniteFormationFormCellSizeM;

        uint32_t eventID = HashGraniteValue(
            formationSeed ^ 0xD6E8FEB9u ^ slotSalt,
            cellX,
            cellY );

        if ( eventID == 0 )
        {
            eventID = 1u;
        }

        descriptor.m_valid = true;
        descriptor.m_eventID = eventID;
        descriptor.m_geologicalAncestryID = geologicalAncestryID;
        descriptor.m_grammarSalt = HashGraniteValue(
            formationSeed ^ 0xA5A35625u ^ slotSalt,
            cellX,
            cellY );

        float const orientationSignal = HashGraniteUnit(
            formationSeed ^ 0xDB4F0B91u ^ slotSalt,
            cellX,
            cellY );

        float const radiusSignalA = HashGraniteUnit(
            formationSeed ^ 0x7E95761Eu ^ slotSalt,
            cellX,
            cellY );

        float const radiusSignalB = HashGraniteUnit(
            formationSeed ^ 0xD1B54A32u ^ slotSalt,
            cellX,
            cellY );

        float const reliefSignal = HashGraniteUnit(
            formationSeed ^ 0x94D049BBu ^ slotSalt,
            cellX,
            cellY );

        float const weatherSignal = HashGraniteUnit(
            formationSeed ^ 0xBBE05633u ^ slotSalt,
            cellX,
            cellY );

        float const fractureSignal = HashGraniteUnit(
            formationSeed ^ 0xA0F2EC75u ^ slotSalt,
            cellX,
            cellY );

        float const asymmetrySignal = HashGraniteUnit(
            formationSeed ^ 0x89E18285u ^ slotSalt,
            cellX,
            cellY );

        descriptor.m_asymmetry =
            0.15f + asymmetrySignal * 0.72f;

        if ( slotIndex == 0 )
        {
            descriptor.m_type =
                GraniteFormationFormType::BroadWeatheredBack;

            descriptor.m_attachment =
                GraniteFormationAttachmentState::CoherentBedrock;

            descriptor.m_orientationRadians =
                formationRotation +
                ( orientationSignal - 0.5f ) * 1.65f;

            descriptor.m_majorRadiusM =
                4.80f + radiusSignalA * 5.60f;

            descriptor.m_minorRadiusM =
                2.90f + radiusSignalB * 4.10f;

            descriptor.m_maximumReliefM =
                0.52f + reliefSignal * 1.34f;

            descriptor.m_rootBlendRadiusM =
                1.45f +
                HashGraniteUnit(
                    formationSeed ^ 0x369DEA0Fu,
                    cellX,
                    cellY ) *
                    2.45f;

            descriptor.m_quietCoreRadiusM =
                GraniteMin(
                    descriptor.m_majorRadiusM,
                    descriptor.m_minorRadiusM ) *
                ( 0.30f +
                  HashGraniteUnit(
                      formationSeed ^ 0xC2B2AE3Du,
                      cellX,
                      cellY ) *
                      0.28f );

            descriptor.m_weatheredExteriorWeight =
                0.66f + weatherSignal * 0.33f;

            descriptor.m_fractureShoulderWeight =
                0.10f + fractureSignal * 0.34f;

            descriptor.m_primaryJointSetIndex = -1;
        }
        else if ( slotIndex == 1 || slotIndex == 2 )
        {
            descriptor.m_type =
                GraniteFormationFormType::RootedRockHead;

            descriptor.m_attachment =
                GraniteFormationAttachmentState::RootedRockHead;

            descriptor.m_orientationRadians =
                formationRotation +
                ( orientationSignal - 0.5f ) * 2.75f;

            float const scaleSignal = HashGraniteUnit(
                formationSeed ^ 0xC13FA9A9u ^ slotSalt,
                cellX,
                cellY );

            if ( scaleSignal < 0.36f )
            {
                // Small rooted head: hand/boulder-scale expression still
                // continuous with the formation.
                descriptor.m_majorRadiusM =
                    0.34f + radiusSignalA * 0.54f;

                descriptor.m_minorRadiusM =
                    0.24f + radiusSignalB * 0.44f;

                descriptor.m_maximumReliefM =
                    0.16f + reliefSignal * 0.40f;

                descriptor.m_rootBlendRadiusM =
                    0.14f + radiusSignalB * 0.26f;
            }
            else if ( scaleSignal < 0.82f )
            {
                // Medium rooted head.
                descriptor.m_majorRadiusM =
                    0.82f + radiusSignalA * 1.38f;

                descriptor.m_minorRadiusM =
                    0.54f + radiusSignalB * 1.04f;

                descriptor.m_maximumReliefM =
                    0.40f + reliefSignal * 0.92f;

                descriptor.m_rootBlendRadiusM =
                    0.28f + radiusSignalB * 0.54f;
            }
            else
            {
                // Large boulder-like head. It is still rooted bedrock.
                descriptor.m_majorRadiusM =
                    2.05f + radiusSignalA * 3.10f;

                descriptor.m_minorRadiusM =
                    1.28f + radiusSignalB * 2.32f;

                descriptor.m_maximumReliefM =
                    0.90f + reliefSignal * 1.82f;

                descriptor.m_rootBlendRadiusM =
                    0.62f + radiusSignalB * 1.08f;
            }

            descriptor.m_quietCoreRadiusM =
                GraniteMin(
                    descriptor.m_majorRadiusM,
                    descriptor.m_minorRadiusM ) *
                ( 0.20f +
                  HashGraniteUnit(
                      formationSeed ^ 0x8538ECB5u ^ slotSalt,
                      cellX,
                      cellY ) *
                      0.28f );

            descriptor.m_weatheredExteriorWeight =
                0.46f + weatherSignal * 0.50f;

            descriptor.m_fractureShoulderWeight =
                0.20f + fractureSignal * 0.54f;

            descriptor.m_primaryJointSetIndex = int32_t(
                HashGraniteValue(
                    formationSeed ^ 0x38D01377u ^ slotSalt,
                    cellX,
                    cellY ) %
                2u );
        }
        else
        {
            descriptor.m_type =
                GraniteFormationFormType::SlabShoulder;

            descriptor.m_attachment =
                GraniteFormationAttachmentState::CoherentBedrock;

            int32_t const jointSetIndex = int32_t(
                HashGraniteValue(
                    formationSeed ^ 0x38D01377u,
                    cellX,
                    cellY ) %
                2u );

            GraniteGeometryProfile const profile = GetGraniteGeometryProfile();

            descriptor.m_primaryJointSetIndex = jointSetIndex;

            descriptor.m_orientationRadians =
                formationRotation +
                profile.m_jointSets[jointSetIndex].m_orientationRadians +
                ( orientationSignal - 0.5f ) * 0.52f;

            descriptor.m_majorRadiusM =
                2.60f + radiusSignalA * 4.30f;

            descriptor.m_minorRadiusM =
                1.35f + radiusSignalB * 2.70f;

            descriptor.m_maximumReliefM =
                0.34f + reliefSignal * 1.02f;

            descriptor.m_rootBlendRadiusM =
                0.72f +
                HashGraniteUnit(
                    formationSeed ^ 0x369DEA0Fu,
                    cellX,
                    cellY ) *
                    1.48f;

            descriptor.m_quietCoreRadiusM =
                GraniteMin(
                    descriptor.m_majorRadiusM,
                    descriptor.m_minorRadiusM ) *
                ( 0.22f +
                  HashGraniteUnit(
                      formationSeed ^ 0x8538ECB5u,
                      cellX,
                      cellY ) *
                      0.30f );

            descriptor.m_dipRadians =
                0.08f +
                HashGraniteUnit(
                    formationSeed ^ 0xBE5466CFu,
                    cellX,
                    cellY ) *
                    0.39f;

            descriptor.m_shoulderSharpness =
                0.42f +
                HashGraniteUnit(
                    formationSeed ^ 0x452821E6u,
                    cellX,
                    cellY ) *
                    0.50f;

            descriptor.m_weatheredExteriorWeight =
                0.34f + weatherSignal * 0.46f;

            descriptor.m_fractureShoulderWeight =
                0.48f + fractureSignal * 0.46f;
        }

        descriptor.m_weatheredExteriorWeight = GraniteClamp01(
            descriptor.m_weatheredExteriorWeight );

        descriptor.m_fractureShoulderWeight = GraniteClamp01(
            descriptor.m_fractureShoulderWeight );

        return descriptor;
    }

    //-------------------------------------------------------------------------

    static GraniteFormationJointDescriptor BuildGraniteFormationJointDescriptor(
        uint32_t formationSeed,
        uint32_t geologicalAncestryID,
        float    formationRotation,
        int32_t  cellX,
        int32_t  cellY,
        int32_t  slotIndex )
    {
        EE_ASSERT( slotIndex >= 0 );
        EE_ASSERT( slotIndex < s_graniteFormationJointSlotsPerCell );

        GraniteFormationJointDescriptor descriptor;

        uint32_t const slotSalt =
            uint32_t( slotIndex ) * 0x00016567u;

        float const activationSignal = HashGraniteUnit(
            formationSeed ^ 0x8B2D3C47u ^ slotSalt,
            cellX,
            cellY );

        float const activationThreshold =
            slotIndex == 0
                ? 0.53f
                : 0.58f;

        if ( activationSignal < activationThreshold )
        {
            return descriptor;
        }

        descriptor.m_valid = true;
        descriptor.m_geologicalAncestryID = geologicalAncestryID;

        uint32_t eventID = HashGraniteValue(
            formationSeed ^ 0xEFBE4786u ^ slotSalt,
            cellX,
            cellY );

        if ( eventID == 0 )
        {
            eventID = 1u;
        }

        descriptor.m_eventID = eventID;

        descriptor.m_tier =
            slotIndex == 0
                ? GraniteJointTier::Primary
                : GraniteJointTier::Secondary;

        if ( descriptor.m_tier == GraniteJointTier::Primary )
        {
            descriptor.m_jointSetIndex = int32_t(
                HashGraniteValue(
                    formationSeed ^ 0x240CA1CCu,
                    cellX,
                    cellY ) %
                2u );
        }
        else
        {
            descriptor.m_jointSetIndex = 2;
        }

        GraniteGeometryProfile const profile = GetGraniteGeometryProfile();

        float const jitterX = HashGraniteUnit(
            formationSeed ^ 0xA24BAED4u ^ slotSalt,
            cellX,
            cellY );

        float const jitterY = HashGraniteUnit(
            formationSeed ^ 0x9FB21C65u ^ slotSalt,
            cellX,
            cellY );

        float const centerX =
            ( float( cellX ) + 0.12f + jitterX * 0.76f ) *
            s_graniteFormationJointCellSizeM;

        float const centerY =
            ( float( cellY ) + 0.12f + jitterY * 0.76f ) *
            s_graniteFormationJointCellSizeM;

        float const orientationVariation =
            descriptor.m_tier == GraniteJointTier::Primary
                ? 0.46f
                : 0.92f;

        float const orientation =
            formationRotation +
            profile.m_jointSets[descriptor.m_jointSetIndex].m_orientationRadians +
            ( HashGraniteUnit(
                  formationSeed ^ 0xCB1AB31Fu ^ slotSalt,
                  cellX,
                  cellY ) -
              0.5f ) *
                orientationVariation;

        float const lengthM =
            descriptor.m_tier == GraniteJointTier::Primary
                ? 5.20f +
                      HashGraniteUnit(
                          formationSeed ^ 0x2DE92C6Fu,
                          cellX,
                          cellY ) *
                          9.20f
                : 2.10f +
                      HashGraniteUnit(
                          formationSeed ^ 0x2DE92C6Fu ^ slotSalt,
                          cellX,
                          cellY ) *
                          5.20f;

        float const bendMagnitude =
            descriptor.m_tier == GraniteJointTier::Primary
                ? 0.45f +
                      HashGraniteUnit(
                          formationSeed ^ 0xBE5466CFu,
                          cellX,
                          cellY ) *
                          1.85f
                : 0.16f +
                      HashGraniteUnit(
                          formationSeed ^ 0xBE5466CFu ^ slotSalt,
                          cellX,
                          cellY ) *
                          0.82f;

        float const bendA =
            ( HashGraniteUnit(
                  formationSeed ^ 0x38D01377u ^ slotSalt,
                  cellX,
                  cellY ) *
                  2.0f -
              1.0f ) *
            bendMagnitude;

        float const bendB =
            ( HashGraniteUnit(
                  formationSeed ^ 0xC0AC29B7u ^ slotSalt,
                  cellX,
                  cellY ) *
                  2.0f -
              1.0f ) *
            bendMagnitude;

        GraniteCurve2D const curve = BuildGraniteGrooveCurve(
            centerX,
            centerY,
            orientation,
            lengthM,
            bendA,
            bendB );

        descriptor.m_p0WorldX = curve.m_p0X;
        descriptor.m_p0WorldY = curve.m_p0Y;
        descriptor.m_p1WorldX = curve.m_p1X;
        descriptor.m_p1WorldY = curve.m_p1Y;
        descriptor.m_p2WorldX = curve.m_p2X;
        descriptor.m_p2WorldY = curve.m_p2Y;
        descriptor.m_p3WorldX = curve.m_p3X;
        descriptor.m_p3WorldY = curve.m_p3Y;
        descriptor.m_lengthM = lengthM;

        float const peakDepthM =
            descriptor.m_tier == GraniteJointTier::Primary
                ? 0.26f +
                      HashGraniteUnit(
                          formationSeed ^ 0x17A211D7u,
                          cellX,
                          cellY ) *
                          0.98f
                : 0.035f +
                      HashGraniteUnit(
                          formationSeed ^ 0x17A211D7u ^ slotSalt,
                          cellX,
                          cellY ) *
                          0.25f;

        float const peakApertureM =
            descriptor.m_tier == GraniteJointTier::Primary
                ? 0.075f +
                      HashGraniteUnit(
                          formationSeed ^ 0xC97C50DDu,
                          cellX,
                          cellY ) *
                          0.31f
                : 0.014f +
                      HashGraniteUnit(
                          formationSeed ^ 0xC97C50DDu ^ slotSalt,
                          cellX,
                          cellY ) *
                          0.082f;

        float const innerA =
            0.68f +
            HashGraniteUnit(
                formationSeed ^ 0x34E90C6Cu ^ slotSalt,
                cellX,
                cellY ) *
                0.32f;

        float const innerB =
            0.46f +
            HashGraniteUnit(
                formationSeed ^ 0x3F84D5B5u ^ slotSalt,
                cellX,
                cellY ) *
                0.54f;

        float const endA =
            0.008f +
            HashGraniteUnit(
                formationSeed ^ 0x4CF5AD43u ^ slotSalt,
                cellX,
                cellY ) *
                0.055f;

        float const endB =
            0.004f +
            HashGraniteUnit(
                formationSeed ^ 0xD3A2646Cu ^ slotSalt,
                cellX,
                cellY ) *
                0.045f;

        bool const reverseProfile =
            HashGraniteUnit(
                formationSeed ^ 0x6C44198Cu ^ slotSalt,
                cellX,
                cellY ) <
            0.5f;

        float const depthP0 = peakDepthM * endA;
        float const depthP1 = peakDepthM * innerA;
        float const depthP2 = peakDepthM * innerB;
        float const depthP3 = peakDepthM * endB;

        float const apertureP0 = peakApertureM * endA;
        float const apertureP1 = peakApertureM * innerA;
        float const apertureP2 = peakApertureM * innerB;
        float const apertureP3 = peakApertureM * endB;

        if ( reverseProfile )
        {
            descriptor.m_depthAtP0M = depthP3;
            descriptor.m_depthAtP1M = depthP2;
            descriptor.m_depthAtP2M = depthP1;
            descriptor.m_depthAtP3M = depthP0;

            descriptor.m_apertureAtP0M = apertureP3;
            descriptor.m_apertureAtP1M = apertureP2;
            descriptor.m_apertureAtP2M = apertureP1;
            descriptor.m_apertureAtP3M = apertureP0;
        }
        else
        {
            descriptor.m_depthAtP0M = depthP0;
            descriptor.m_depthAtP1M = depthP1;
            descriptor.m_depthAtP2M = depthP2;
            descriptor.m_depthAtP3M = depthP3;

            descriptor.m_apertureAtP0M = apertureP0;
            descriptor.m_apertureAtP1M = apertureP1;
            descriptor.m_apertureAtP2M = apertureP2;
            descriptor.m_apertureAtP3M = apertureP3;
        }

        float const shoulderBase =
            descriptor.m_tier == GraniteJointTier::Primary
                ? 0.028f
                : 0.006f;

        float const shoulderRange =
            descriptor.m_tier == GraniteJointTier::Primary
                ? 0.17f
                : 0.050f;

        descriptor.m_leftShoulderLiftM =
            shoulderBase +
            HashGraniteUnit(
                formationSeed ^ 0xB5470917u ^ slotSalt,
                cellX,
                cellY ) *
                shoulderRange;

        descriptor.m_rightShoulderLiftM =
            shoulderBase +
            HashGraniteUnit(
                formationSeed ^ 0x1B873593u ^ slotSalt,
                cellX,
                cellY ) *
                shoulderRange;

        descriptor.m_persistence =
            descriptor.m_tier == GraniteJointTier::Primary
                ? 0.66f +
                      HashGraniteUnit(
                          formationSeed ^ 0x85EBCA6Bu,
                          cellX,
                          cellY ) *
                          0.32f
                : 0.36f +
                      HashGraniteUnit(
                          formationSeed ^ 0x85EBCA6Bu ^ slotSalt,
                          cellX,
                          cellY ) *
                          0.40f;

        descriptor.m_weathering =
            0.18f +
            HashGraniteUnit(
                formationSeed ^ 0xC2B2AE35u ^ slotSalt,
                cellX,
                cellY ) *
                0.76f;

        return descriptor;
    }

    //-------------------------------------------------------------------------

    static bool GraniteFormationFormIntersectsBounds(
        GraniteFormationFormDescriptor const& descriptor,
        float                                 minWorldX,
        float                                 minWorldY,
        float                                 maxWorldX,
        float                                 maxWorldY )
    {
        if ( !descriptor.m_valid )
        {
            return false;
        }

        float const radius = GraniteMax(
                                 descriptor.m_majorRadiusM,
                                 descriptor.m_minorRadiusM ) +
                             descriptor.m_rootBlendRadiusM;

        return descriptor.m_centerWorldX + radius >= minWorldX &&
               descriptor.m_centerWorldX - radius <= maxWorldX &&
               descriptor.m_centerWorldY + radius >= minWorldY &&
               descriptor.m_centerWorldY - radius <= maxWorldY;
    }

    //-------------------------------------------------------------------------

    static float GetGraniteFormationJointPeakDepth(
        GraniteFormationJointDescriptor const& descriptor )
    {
        float peak = descriptor.m_depthAtP0M;
        peak = GraniteMax( peak, descriptor.m_depthAtP1M );
        peak = GraniteMax( peak, descriptor.m_depthAtP2M );
        peak = GraniteMax( peak, descriptor.m_depthAtP3M );
        return peak;
    }

    //-------------------------------------------------------------------------

    static float GetGraniteFormationJointPeakAperture(
        GraniteFormationJointDescriptor const& descriptor )
    {
        float peak = descriptor.m_apertureAtP0M;
        peak = GraniteMax( peak, descriptor.m_apertureAtP1M );
        peak = GraniteMax( peak, descriptor.m_apertureAtP2M );
        peak = GraniteMax( peak, descriptor.m_apertureAtP3M );
        return peak;
    }

    //-------------------------------------------------------------------------

    static bool GraniteFormationJointIntersectsBounds(
        GraniteFormationJointDescriptor const& descriptor,
        float                                  minWorldX,
        float                                  minWorldY,
        float                                  maxWorldX,
        float                                  maxWorldY )
    {
        if ( !descriptor.m_valid )
        {
            return false;
        }

        float curveMinX = descriptor.m_p0WorldX;
        float curveMaxX = descriptor.m_p0WorldX;
        float curveMinY = descriptor.m_p0WorldY;
        float curveMaxY = descriptor.m_p0WorldY;

        float const pointsX[3] =
            {
                descriptor.m_p1WorldX,
                descriptor.m_p2WorldX,
                descriptor.m_p3WorldX };

        float const pointsY[3] =
            {
                descriptor.m_p1WorldY,
                descriptor.m_p2WorldY,
                descriptor.m_p3WorldY };

        for ( int32_t i = 0; i < 3; ++i )
        {
            curveMinX = GraniteMin( curveMinX, pointsX[i] );
            curveMaxX = GraniteMax( curveMaxX, pointsX[i] );
            curveMinY = GraniteMin( curveMinY, pointsY[i] );
            curveMaxY = GraniteMax( curveMaxY, pointsY[i] );
        }

        float const peakDepth = GetGraniteFormationJointPeakDepth( descriptor );
        float const peakAperture = GetGraniteFormationJointPeakAperture( descriptor );

        float const padding = GraniteMax(
            peakAperture * 4.0f,
            peakDepth * 1.65f +
                ( descriptor.m_tier == GraniteJointTier::Primary
                      ? 0.32f
                      : 0.12f ) );

        return curveMaxX + padding >= minWorldX &&
               curveMinX - padding <= maxWorldX &&
               curveMaxY + padding >= minWorldY &&
               curveMinY - padding <= maxWorldY;
    }

    //-------------------------------------------------------------------------

    static float GetGraniteFormationFormImportance(
        GraniteFormationFormDescriptor const& descriptor )
    {
        float typeWeight = 1.0f;

        switch ( descriptor.m_type )
        {
            case GraniteFormationFormType::BroadWeatheredBack:
                typeWeight = 1.08f;
                break;

            case GraniteFormationFormType::RootedRockHead:
                typeWeight = 1.24f;
                break;

            case GraniteFormationFormType::SlabShoulder:
                typeWeight = 1.14f;
                break;
        }

        return descriptor.m_maximumReliefM *
               descriptor.m_majorRadiusM *
               descriptor.m_minorRadiusM *
               typeWeight;
    }

    //-------------------------------------------------------------------------

    static float GetGraniteFormationJointImportance(
        GraniteFormationJointDescriptor const& descriptor )
    {
        float const tierWeight =
            descriptor.m_tier == GraniteJointTier::Primary
                ? 1.35f
                : 0.75f;

        return GetGraniteFormationJointPeakDepth( descriptor ) *
               descriptor.m_lengthM *
               descriptor.m_persistence *
               tierWeight;
    }

    //-------------------------------------------------------------------------

    static void InsertGraniteFormationFormDescriptor(
        GraniteFormationField&                field,
        GraniteFormationFormDescriptor const& descriptor )
    {
        if ( !descriptor.m_valid )
        {
            return;
        }

        if ( field.m_numForms < GraniteFormationField::s_maxForms )
        {
            field.m_forms[field.m_numForms++] = descriptor;
            return;
        }

        int32_t weakestIndex = 0;
        float   weakestImportance =
            GetGraniteFormationFormImportance( field.m_forms[0] );

        for ( int32_t i = 1; i < field.m_numForms; ++i )
        {
            float const importance =
                GetGraniteFormationFormImportance( field.m_forms[i] );

            if ( importance < weakestImportance )
            {
                weakestImportance = importance;
                weakestIndex = i;
            }
        }

        float const newImportance =
            GetGraniteFormationFormImportance( descriptor );

        if ( newImportance > weakestImportance )
        {
            field.m_forms[weakestIndex] = descriptor;
        }
    }

    //-------------------------------------------------------------------------

    static void InsertGraniteFormationJointDescriptor(
        GraniteFormationField&                 field,
        GraniteFormationJointDescriptor const& descriptor )
    {
        if ( !descriptor.m_valid )
        {
            return;
        }

        if ( field.m_numJoints < GraniteFormationField::s_maxJoints )
        {
            field.m_joints[field.m_numJoints++] = descriptor;
            return;
        }

        int32_t weakestIndex = 0;
        float   weakestImportance =
            GetGraniteFormationJointImportance( field.m_joints[0] );

        for ( int32_t i = 1; i < field.m_numJoints; ++i )
        {
            float const importance =
                GetGraniteFormationJointImportance( field.m_joints[i] );

            if ( importance < weakestImportance )
            {
                weakestImportance = importance;
                weakestIndex = i;
            }
        }

        float const newImportance =
            GetGraniteFormationJointImportance( descriptor );

        if ( newImportance > weakestImportance )
        {
            field.m_joints[weakestIndex] = descriptor;
        }
    }

    //-------------------------------------------------------------------------

    static void SortGraniteFormationField( GraniteFormationField& field )
    {
        for ( int32_t i = 1; i < field.m_numForms; ++i )
        {
            GraniteFormationFormDescriptor const key = field.m_forms[i];
            int32_t                              j = i - 1;

            while ( j >= 0 && field.m_forms[j].m_eventID > key.m_eventID )
            {
                field.m_forms[j + 1] = field.m_forms[j];
                --j;
            }

            field.m_forms[j + 1] = key;
        }

        for ( int32_t i = 1; i < field.m_numJoints; ++i )
        {
            GraniteFormationJointDescriptor const key = field.m_joints[i];
            int32_t                               j = i - 1;

            while ( j >= 0 && field.m_joints[j].m_eventID > key.m_eventID )
            {
                field.m_joints[j + 1] = field.m_joints[j];
                --j;
            }

            field.m_joints[j + 1] = key;
        }
    }

    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------

    float EvaluateGraniteFormationFormRelief(
        GraniteFormationFormDescriptor const& descriptor,
        float                                 worldX,
        float                                 worldY )
    {
        if ( !descriptor.m_valid )
        {
            return 0.0f;
        }

        float along = 0.0f;
        float across = 0.0f;

        GetGraniteFormationLocalCoordinates(
            descriptor,
            worldX,
            worldY,
            along,
            across );

        float const safeMajor = GraniteMax(
            descriptor.m_majorRadiusM,
            0.10f );

        float const safeMinor = GraniteMax(
            descriptor.m_minorRadiusM,
            0.10f );

        float const outerMajor = GraniteMax(
            safeMajor + descriptor.m_rootBlendRadiusM,
            safeMajor + 0.05f );

        float const outerMinor = GraniteMax(
            safeMinor + descriptor.m_rootBlendRadiusM,
            safeMinor + 0.05f );

        float const normalizedAlong = GraniteClampSigned(
            along / safeMajor );

        float const normalizedAcross = GraniteClampSigned(
            across / safeMinor );

        float const coreFraction = GraniteMax(
            GraniteMin(
                safeMajor / outerMajor,
                safeMinor / outerMinor ),
            0.20f );

        float constexpr pi = 3.14159265358979323846f;

        uint32_t const grammarSeed =
            descriptor.m_grammarSalt ^
            descriptor.m_eventID;

        float const formSignalA = HashGraniteUnit(
            grammarSeed ^ 0x85EBCA6Bu,
            17,
            31 );

        float const formSignalB = HashGraniteUnit(
            grammarSeed ^ 0xC2B2AE35u,
            37,
            41 );

        float const formSignalC = HashGraniteUnit(
            grammarSeed ^ 0x27D4EB2Fu,
            43,
            47 );

        // --------------------------------------------------------------------
        // Broad weathered Granite back
        // --------------------------------------------------------------------

        if ( descriptor.m_type ==
             GraniteFormationFormType::BroadWeatheredBack )
        {
            float const superExponent =
                2.15f +
                descriptor.m_asymmetry * 1.15f;

            float const outerRadius = GraniteSuperellipseRadius(
                along,
                across,
                outerMajor,
                outerMinor,
                superExponent );

            float body = GraniteRootedStoneProfile(
                outerRadius,
                coreFraction,
                0.48f +
                    descriptor.m_weatheredExteriorWeight * 0.20f,
                0.24f +
                    descriptor.m_weatheredExteriorWeight * 0.10f );

            if ( body <= 0.0f )
            {
                return 0.0f;
            }

            // A second coherent lobe breaks radial-hill symmetry while still
            // allowing metres of quiet weathered Granite surface.
            float const secondaryDirection =
                descriptor.m_orientationRadians +
                pi * ( 0.26f + formSignalA * 0.48f );

            float const secondaryOffset =
                safeMinor *
                ( 0.16f + descriptor.m_asymmetry * 0.30f );

            float const secondaryCenterX =
                descriptor.m_centerWorldX +
                float( std::cos( double( secondaryDirection ) ) ) *
                    secondaryOffset;

            float const secondaryCenterY =
                descriptor.m_centerWorldY +
                float( std::sin( double( secondaryDirection ) ) ) *
                    secondaryOffset;

            float const cosine = float(
                std::cos( double( descriptor.m_orientationRadians ) ) );

            float const sine = float(
                std::sin( double( descriptor.m_orientationRadians ) ) );

            float const secondaryDeltaX = worldX - secondaryCenterX;
            float const secondaryDeltaY = worldY - secondaryCenterY;

            float const secondaryAlong =
                secondaryDeltaX * cosine +
                secondaryDeltaY * sine;

            float const secondaryAcross =
                -secondaryDeltaX * sine +
                secondaryDeltaY * cosine;

            float const secondaryRadius = GraniteSuperellipseRadius(
                secondaryAlong,
                secondaryAcross,
                safeMajor * ( 0.58f + formSignalB * 0.12f ),
                safeMinor * ( 0.56f + formSignalC * 0.16f ),
                2.0f + formSignalB * 0.90f );

            float const secondaryBody = GraniteRootedStoneProfile(
                secondaryRadius,
                0.80f,
                0.62f,
                0.08f );

            body = CombineGranitePositiveRelief(
                body,
                secondaryBody * 0.48f );

            float const interiorMask =
                1.0f -
                GraniteSmoothStep01(
                    ( outerRadius - coreFraction * 0.84f ) /
                    GraniteMax( coreFraction * 0.16f, 0.03f ) );

            float const quietRadiusM = GraniteMax(
                descriptor.m_quietCoreRadiusM,
                0.0f );

            float const distanceFromCenter = GraniteLength2D(
                along,
                across );

            float const quietProtection =
                quietRadiusM > 0.0f
                    ? 1.0f -
                          GraniteSmoothStep01(
                              distanceFromCenter /
                              quietRadiusM )
                    : 0.0f;

            float const facetAngle =
                ( formSignalA - 0.5f ) * pi;

            float const facetCoordinate = GraniteObliqueCoordinate(
                normalizedAlong,
                normalizedAcross,
                facetAngle );

            float const facetThreshold =
                ( formSignalB - 0.5f ) * 0.30f;

            float const facetStep = GraniteSmoothStep01(
                ( facetCoordinate - facetThreshold + 0.18f ) /
                0.36f );

            float const shallowFacetDrop =
                descriptor.m_fractureShoulderWeight *
                0.075f *
                facetStep *
                interiorMask *
                ( 1.0f - quietProtection * 0.72f );

            float const scarAngle =
                facetAngle +
                pi * ( 0.37f + formSignalC * 0.28f );

            float const scarCoordinate = GraniteObliqueCoordinate(
                normalizedAlong,
                normalizedAcross,
                scarAngle );

            float const shallowScar = GraniteSmoothLineBand(
                scarCoordinate -
                    ( formSignalC - 0.5f ) * 0.40f,
                0.045f + formSignalA * 0.055f );

            float const shallowScarDrop =
                shallowScar *
                descriptor.m_fractureShoulderWeight *
                0.045f *
                interiorMask *
                ( 1.0f - quietProtection );

            float const tilt =
                normalizedAlong *
                    ( formSignalA - 0.5f ) *
                    0.055f +
                normalizedAcross *
                    ( formSignalB - 0.5f ) *
                    0.045f;

            float const reliefFraction = GraniteMax(
                body * ( 1.0f + tilt ) -
                    shallowFacetDrop -
                    shallowScarDrop,
                0.0f );

            return descriptor.m_maximumReliefM *
                   reliefFraction;
        }

        // --------------------------------------------------------------------
        // Rooted boulder-like Granite head
        // --------------------------------------------------------------------

        if ( descriptor.m_type ==
             GraniteFormationFormType::RootedRockHead )
        {
            float const superExponent =
                2.20f +
                formSignalA * 1.10f;

            float const outerRadius = GraniteSuperellipseRadius(
                along,
                across,
                outerMajor,
                outerMinor,
                superExponent );

            float body = GraniteRootedStoneProfile(
                outerRadius,
                coreFraction,
                0.28f +
                    descriptor.m_weatheredExteriorWeight * 0.18f,
                0.17f +
                    descriptor.m_weatheredExteriorWeight * 0.13f );

            if ( body <= 0.0f )
            {
                return 0.0f;
            }

            float const interiorMask =
                1.0f -
                GraniteSmoothStep01(
                    ( outerRadius - coreFraction * 0.91f ) /
                    GraniteMax( coreFraction * 0.10f, 0.025f ) );

            // Offset crown/knuckle. It remains rooted because the surrounding
            // host profile, not a separate prop mesh, owns the base.
            float const crownDirection =
                descriptor.m_orientationRadians +
                ( formSignalB - 0.5f ) * 2.20f;

            float const crownOffset =
                safeMinor *
                ( 0.19f + descriptor.m_asymmetry * 0.24f );

            float const crownCenterX =
                descriptor.m_centerWorldX +
                float( std::cos( double( crownDirection ) ) ) *
                    crownOffset;

            float const crownCenterY =
                descriptor.m_centerWorldY +
                float( std::sin( double( crownDirection ) ) ) *
                    crownOffset;

            float const crownEnvelope = GraniteEllipticEnvelope(
                worldX,
                worldY,
                crownCenterX,
                crownCenterY,
                descriptor.m_orientationRadians +
                    ( formSignalC - 0.5f ) * 0.52f,
                safeMajor * ( 0.48f + formSignalA * 0.16f ),
                safeMinor * ( 0.48f + formSignalB * 0.18f ) );

            body = CombineGranitePositiveRelief(
                body,
                GraniteSafePow01(
                    crownEnvelope,
                    0.56f ) *
                    0.26f );

            // A second offset shoulder lobe keeps the rooted head stone-bodied
            // and asymmetrical without detaching it from the formation.
            float const shoulderLobeAngle =
                crownDirection +
                1.57079632679489661923f *
                    ( formSignalC <
                              0.5f
                          ? -1.0f
                          : 1.0f );

            float const shoulderLobeCenterX =
                descriptor.m_centerWorldX +
                float(
                    std::cos(
                        double(
                            shoulderLobeAngle ) ) ) *
                    safeMinor *
                    0.42f;

            float const shoulderLobeCenterY =
                descriptor.m_centerWorldY +
                float(
                    std::sin(
                        double(
                            shoulderLobeAngle ) ) ) *
                    safeMinor *
                    0.42f;

            float const shoulderLobeEnvelope =
                GraniteEllipticEnvelope(
                    worldX,
                    worldY,
                    shoulderLobeCenterX,
                    shoulderLobeCenterY,
                    descriptor.m_orientationRadians +
                        ( formSignalB -
                          0.5f ) *
                            0.30f,
                    safeMajor *
                        0.34f,
                    safeMinor *
                        0.30f );

            body =
                CombineGranitePositiveRelief(
                    body,
                    GraniteSafePow01(
                        shoulderLobeEnvelope,
                        0.78f ) *
                        0.18f );

            // Two broad fracture tendencies create rough, dominant planes and
            // shoulders without ever making a mathematically perfect wall.
            float const fractureAngleA =
                ( formSignalA - 0.5f ) * pi;

            float const fractureAngleB =
                fractureAngleA +
                pi * ( 0.36f + formSignalB * 0.30f );

            float const coordinateA = GraniteObliqueCoordinate(
                normalizedAlong,
                normalizedAcross,
                fractureAngleA );

            float const coordinateB = GraniteObliqueCoordinate(
                normalizedAlong,
                normalizedAcross,
                fractureAngleB );

            float const stepA = GraniteSmoothStep01(
                ( coordinateA -
                  ( formSignalB - 0.5f ) * 0.24f +
                  0.13f ) /
                0.26f );

            float const stepB = GraniteSmoothStep01(
                ( coordinateB -
                  ( formSignalC - 0.5f ) * 0.30f +
                  0.11f ) /
                0.22f );

            float const fractureDrop =
                descriptor.m_fractureShoulderWeight *
                interiorMask *
                ( stepA *
                      0.070f +
                  stepB *
                      0.040f );

            float const shoulderBand = GraniteSmoothLineBand(
                coordinateA -
                    ( formSignalB - 0.5f ) * 0.24f,
                0.07f + formSignalC * 0.06f );

            float const shoulderLift =
                shoulderBand *
                descriptor.m_fractureShoulderWeight *
                interiorMask *
                0.055f;

            // One indentation and one extrusion reproduce the useful B3 south
            // face relationship at formation scale.
            float const notchSide =
                formSignalA < 0.5f ? -1.0f : 1.0f;

            float const notchCenterAngle =
                descriptor.m_orientationRadians +
                notchSide * pi * 0.5f;

            float const notchCenterX =
                descriptor.m_centerWorldX +
                float( std::cos( double( notchCenterAngle ) ) ) *
                    safeMinor * 0.58f;

            float const notchCenterY =
                descriptor.m_centerWorldY +
                float( std::sin( double( notchCenterAngle ) ) ) *
                    safeMinor * 0.58f;

            float const notchEnvelope = GraniteEllipticEnvelope(
                worldX,
                worldY,
                notchCenterX,
                notchCenterY,
                descriptor.m_orientationRadians,
                safeMajor * 0.30f,
                safeMinor * 0.24f );

            float const knuckleCenterAngle =
                notchCenterAngle + pi;

            float const knuckleCenterX =
                descriptor.m_centerWorldX +
                float( std::cos( double( knuckleCenterAngle ) ) ) *
                    safeMinor * 0.44f;

            float const knuckleCenterY =
                descriptor.m_centerWorldY +
                float( std::sin( double( knuckleCenterAngle ) ) ) *
                    safeMinor * 0.44f;

            float const knuckleEnvelope = GraniteEllipticEnvelope(
                worldX,
                worldY,
                knuckleCenterX,
                knuckleCenterY,
                descriptor.m_orientationRadians +
                    ( formSignalC - 0.5f ) * 0.40f,
                safeMajor * 0.26f,
                safeMinor * 0.30f );

            float const mesoNoise = SampleGraniteValueNoise(
                grammarSeed,
                worldX,
                worldY,
                GraniteMax(
                    GraniteMin( safeMajor, safeMinor ) * 0.48f,
                    0.38f ),
                0xD3A2646Cu );

            float const mesoRelief =
                ( mesoNoise -
                  0.5f ) *
                interiorMask *
                0.064f;

            float const reliefFraction = GraniteMax(
                body -
                    fractureDrop -
                    notchEnvelope *
                        descriptor.m_fractureShoulderWeight *
                        interiorMask *
                        0.095f +
                    shoulderLift +
                    knuckleEnvelope *
                        descriptor.m_weatheredExteriorWeight *
                        interiorMask *
                        0.075f +
                    mesoRelief,
                0.0f );

            return descriptor.m_maximumReliefM *
                   reliefFraction;
        }

        // --------------------------------------------------------------------
        // Rooted slab / shoulder
        // --------------------------------------------------------------------

        float const superExponent =
            4.20f +
            formSignalA * 1.75f;

        float const outerRadius = GraniteSuperellipseRadius(
            along,
            across,
            outerMajor,
            outerMinor,
            superExponent );

        float const body = GraniteRootedStoneProfile(
            outerRadius,
            coreFraction,
            0.18f +
                descriptor.m_weatheredExteriorWeight * 0.10f,
            0.11f +
                descriptor.m_weatheredExteriorWeight * 0.08f );

        if ( body <= 0.0f )
        {
            return 0.0f;
        }

        float const interiorMask =
            1.0f -
            GraniteSmoothStep01(
                ( outerRadius - coreFraction * 0.93f ) /
                GraniteMax( coreFraction * 0.08f, 0.02f ) );

        float const dipFactor = GraniteMax(
            1.0f +
                float( std::sin( double( descriptor.m_dipRadians ) ) ) *
                    normalizedAlong *
                    0.42f,
            0.55f );

        float const shoulderOffset =
            ( descriptor.m_asymmetry - 0.5f ) * 0.42f;

        float const shoulderStep = GraniteSmoothStep01(
            ( normalizedAcross - shoulderOffset +
              0.10f +
              descriptor.m_shoulderSharpness * 0.05f ) /
            ( 0.20f -
              descriptor.m_shoulderSharpness * 0.07f ) );

        float const shoulderDrop =
            descriptor.m_fractureShoulderWeight *
            interiorMask *
            shoulderStep *
            ( 0.12f +
              descriptor.m_shoulderSharpness * 0.13f );

        float const shoulderBand = GraniteSmoothLineBand(
            normalizedAcross - shoulderOffset,
            0.06f +
                ( 1.0f - descriptor.m_shoulderSharpness ) * 0.08f );

        float const shoulderLip =
            shoulderBand *
            interiorMask *
            descriptor.m_fractureShoulderWeight *
            0.065f;

        float const crossFractureAngle =
            ( formSignalB - 0.5f ) * pi;

        float const crossCoordinate = GraniteObliqueCoordinate(
            normalizedAlong,
            normalizedAcross,
            crossFractureAngle );

        float const crossScar = GraniteSmoothLineBand(
            crossCoordinate -
                ( formSignalC - 0.5f ) * 0.40f,
            0.035f + formSignalA * 0.050f );

        float const reliefFraction = GraniteMax(
            body * dipFactor -
                shoulderDrop +
                shoulderLip -
                crossScar *
                    descriptor.m_fractureShoulderWeight *
                    interiorMask *
                    0.045f,
            0.0f );

        return descriptor.m_maximumReliefM *
               reliefFraction;
    }

    //-------------------------------------------------------------------------

    struct GraniteFormationReliefSample
    {
        float m_broadWeatheredBackReliefM = 0.0f;
        float m_rootedRockHeadReliefM = 0.0f;
        float m_slabShoulderReliefM = 0.0f;
        float m_jointShoulderReliefM = 0.0f;

        float m_primaryJointRecessM = 0.0f;
        float m_secondaryJointRecessM = 0.0f;

        float m_primaryJointInfluence = 0.0f;
        float m_secondaryJointInfluence = 0.0f;

        uint32_t m_dominantFormEventID = 0;
        uint32_t m_dominantJointEventID = 0;

        float m_dominantFormMagnitude = 0.0f;
        float m_dominantJointMagnitude = 0.0f;
    };

    //-------------------------------------------------------------------------

    static void AccumulateGraniteFormationJoint(
        GraniteFormationReliefSample&          result,
        GraniteFormationJointDescriptor const& descriptor,
        float                                  worldX,
        float                                  worldY,
        GraniteWeatheringState                 weathering )
    {
        if ( !descriptor.m_valid )
        {
            return;
        }

        GraniteCurve2D curve;
        curve.m_p0X = descriptor.m_p0WorldX;
        curve.m_p0Y = descriptor.m_p0WorldY;
        curve.m_p1X = descriptor.m_p1WorldX;
        curve.m_p1Y = descriptor.m_p1WorldY;
        curve.m_p2X = descriptor.m_p2WorldX;
        curve.m_p2Y = descriptor.m_p2WorldY;
        curve.m_p3X = descriptor.m_p3WorldX;
        curve.m_p3Y = descriptor.m_p3WorldY;

        GraniteCurveDistanceSample const distance =
            EvaluateGraniteCurveDistance(
                curve,
                worldX,
                worldY,
                descriptor.m_tier == GraniteJointTier::Primary
                    ? 36
                    : 24 );

        float localDepthM = GraniteMax(
            EvaluateGraniteCubicScalar(
                descriptor.m_depthAtP0M,
                descriptor.m_depthAtP1M,
                descriptor.m_depthAtP2M,
                descriptor.m_depthAtP3M,
                distance.m_curveT ),
            0.0f );

        float localApertureM = GraniteMax(
            EvaluateGraniteCubicScalar(
                descriptor.m_apertureAtP0M,
                descriptor.m_apertureAtP1M,
                descriptor.m_apertureAtP2M,
                descriptor.m_apertureAtP3M,
                distance.m_curveT ),
            0.002f );

        if ( weathering == GraniteWeatheringState::WeatheredExposure )
        {
            localDepthM *= GraniteLerp(
                0.98f,
                0.86f,
                descriptor.m_weathering );

            localApertureM *= GraniteLerp(
                1.02f,
                1.22f,
                descriptor.m_weathering );
        }

        float const coreHalfWidthM = GraniteMax(
            localApertureM * 0.50f,
            descriptor.m_tier == GraniteJointTier::Primary
                ? 0.045f
                : 0.018f );

        // Deep joints need enough geometric shoulder width to be represented
        // by the current 0.25 m certification carrier without collapsing into
        // one-sample triangular teeth. Aperture is the structural core; the
        // broad weathered wall transition may be much wider.
        float const shoulderHalfWidthM = GraniteMax(
            coreHalfWidthM *
                ( descriptor.m_tier == GraniteJointTier::Primary
                      ? 4.2f
                      : 3.4f ),
            localDepthM *
                    ( descriptor.m_tier == GraniteJointTier::Primary
                          ? 1.30f
                          : 1.65f ) +
                ( descriptor.m_tier == GraniteJointTier::Primary
                      ? 0.24f
                      : 0.09f ) );

        if ( distance.m_distanceM > shoulderHalfWidthM * 1.20f )
        {
            return;
        }

        float const bodyHalfWidthM = GraniteMax(
            coreHalfWidthM * 2.15f,
            shoulderHalfWidthM * 0.48f );

        float const broadEnvelope =
            1.0f -
            GraniteSmoothStep01(
                distance.m_distanceM /
                GraniteMax( shoulderHalfWidthM, 0.03f ) );

        float const bodyEnvelope =
            1.0f -
            GraniteSmoothStep01(
                distance.m_distanceM /
                GraniteMax( bodyHalfWidthM, 0.025f ) );

        float const coreEnvelope =
            1.0f -
            GraniteSmoothStep01(
                distance.m_distanceM /
                GraniteMax( coreHalfWidthM, 0.012f ) );

        float const sectionEnvelope =
            broadEnvelope * 0.27f +
            bodyEnvelope * 0.45f +
            coreEnvelope * 0.28f;

        float const influence = GraniteClamp01(
            sectionEnvelope * descriptor.m_persistence );

        float const recess =
            -localDepthM * influence;

        if ( descriptor.m_tier == GraniteJointTier::Primary )
        {
            result.m_primaryJointRecessM += recess;
            result.m_primaryJointInfluence = GraniteMax(
                result.m_primaryJointInfluence,
                influence );
        }
        else
        {
            result.m_secondaryJointRecessM += recess;
            result.m_secondaryJointInfluence = GraniteMax(
                result.m_secondaryJointInfluence,
                influence );
        }

        float const leftCenter = shoulderHalfWidthM * 0.72f;
        float const rightCenter = -shoulderHalfWidthM * 0.72f;
        float const shoulderRadius = GraniteMax(
            shoulderHalfWidthM * 0.58f,
            0.06f );

        float const leftEnvelope =
            1.0f -
            GraniteSmoothStep01(
                GraniteAbs( distance.m_signedSideM - leftCenter ) /
                shoulderRadius );

        float const rightEnvelope =
            1.0f -
            GraniteSmoothStep01(
                GraniteAbs( distance.m_signedSideM - rightCenter ) /
                shoulderRadius );

        result.m_jointShoulderReliefM = CombineGranitePositiveRelief(
            result.m_jointShoulderReliefM,
            leftEnvelope *
                descriptor.m_leftShoulderLiftM *
                descriptor.m_persistence );

        result.m_jointShoulderReliefM = CombineGranitePositiveRelief(
            result.m_jointShoulderReliefM,
            rightEnvelope *
                descriptor.m_rightShoulderLiftM *
                descriptor.m_persistence );

        float const magnitude =
            GraniteAbs( recess ) +
            ( leftEnvelope * descriptor.m_leftShoulderLiftM +
              rightEnvelope * descriptor.m_rightShoulderLiftM ) *
                0.25f;

        if ( magnitude > result.m_dominantJointMagnitude )
        {
            result.m_dominantJointMagnitude = magnitude;
            result.m_dominantJointEventID = descriptor.m_eventID;
        }
    }

    //-------------------------------------------------------------------------

    static GraniteFormationReliefSample EvaluateGraniteFormationRelief(
        uint32_t               worldSeed,
        uint32_t               geologicalAncestryID,
        float                  worldX,
        float                  worldY,
        GraniteWeatheringState weathering )
    {
        GraniteFormationReliefSample result;

        uint32_t const formationSeed = MixGraniteSeed(
            worldSeed,
            geologicalAncestryID );

        float const formationRotation = GetGraniteFormationRotation(
            worldSeed,
            geologicalAncestryID );

        int32_t const baseFormCellX = int32_t(
            std::floor(
                double(
                    worldX /
                    s_graniteFormationFormCellSizeM ) ) );

        int32_t const baseFormCellY = int32_t(
            std::floor(
                double(
                    worldY /
                    s_graniteFormationFormCellSizeM ) ) );

        for ( int32_t cellOffsetY = -2; cellOffsetY <= 2; ++cellOffsetY )
        {
            for ( int32_t cellOffsetX = -2; cellOffsetX <= 2; ++cellOffsetX )
            {
                int32_t const cellX = baseFormCellX + cellOffsetX;
                int32_t const cellY = baseFormCellY + cellOffsetY;

                for ( int32_t slotIndex = 0;
                      slotIndex < s_graniteFormationFormSlotsPerCell;
                      ++slotIndex )
                {
                    GraniteFormationFormDescriptor const descriptor =
                        BuildGraniteFormationFormDescriptor(
                            formationSeed,
                            geologicalAncestryID,
                            formationRotation,
                            cellX,
                            cellY,
                            slotIndex );

                    float relief = EvaluateGraniteFormationFormRelief(
                        descriptor,
                        worldX,
                        worldY );

                    if ( relief <= 0.0f )
                    {
                        continue;
                    }

                    if ( weathering == GraniteWeatheringState::FreshFracture )
                    {
                        if ( descriptor.m_type ==
                             GraniteFormationFormType::BroadWeatheredBack )
                        {
                            relief *= 0.78f;
                        }
                        else if ( descriptor.m_type ==
                                  GraniteFormationFormType::RootedRockHead )
                        {
                            relief *= 0.88f;
                        }
                        else
                        {
                            relief *= 1.04f;
                        }
                    }

                    switch ( descriptor.m_type )
                    {
                        case GraniteFormationFormType::BroadWeatheredBack:
                            result.m_broadWeatheredBackReliefM =
                                CombineGranitePositiveRelief(
                                    result.m_broadWeatheredBackReliefM,
                                    relief );
                            break;

                        case GraniteFormationFormType::RootedRockHead:
                            result.m_rootedRockHeadReliefM =
                                CombineGranitePositiveRelief(
                                    result.m_rootedRockHeadReliefM,
                                    relief );
                            break;

                        case GraniteFormationFormType::SlabShoulder:
                            result.m_slabShoulderReliefM =
                                CombineGranitePositiveRelief(
                                    result.m_slabShoulderReliefM,
                                    relief );
                            break;
                    }

                    if ( relief > result.m_dominantFormMagnitude )
                    {
                        result.m_dominantFormMagnitude = relief;
                        result.m_dominantFormEventID = descriptor.m_eventID;
                    }
                }
            }
        }

        int32_t const baseJointCellX = int32_t(
            std::floor(
                double(
                    worldX /
                    s_graniteFormationJointCellSizeM ) ) );

        int32_t const baseJointCellY = int32_t(
            std::floor(
                double(
                    worldY /
                    s_graniteFormationJointCellSizeM ) ) );

        for ( int32_t cellOffsetY = -2; cellOffsetY <= 2; ++cellOffsetY )
        {
            for ( int32_t cellOffsetX = -2; cellOffsetX <= 2; ++cellOffsetX )
            {
                int32_t const cellX = baseJointCellX + cellOffsetX;
                int32_t const cellY = baseJointCellY + cellOffsetY;

                for ( int32_t slotIndex = 0;
                      slotIndex < s_graniteFormationJointSlotsPerCell;
                      ++slotIndex )
                {
                    GraniteFormationJointDescriptor const descriptor =
                        BuildGraniteFormationJointDescriptor(
                            formationSeed,
                            geologicalAncestryID,
                            formationRotation,
                            cellX,
                            cellY,
                            slotIndex );

                    AccumulateGraniteFormationJoint(
                        result,
                        descriptor,
                        worldX,
                        worldY,
                        weathering );
                }
            }
        }

        // Intersections may deepen naturally, but keep the height-field test
        // within a representable range. True undercuts remain a later
        // volumetric-surface milestone.
        result.m_primaryJointRecessM = GraniteMax(
            result.m_primaryJointRecessM,
            -2.40f );

        result.m_secondaryJointRecessM = GraniteMax(
            result.m_secondaryJointRecessM,
            -0.65f );

        return result;
    }

    //-------------------------------------------------------------------------
    // Hierarchical Granite joint/outcrop field
    //
    // The old periodic joint families remain useful as formation-scale
    // orientation/spacing authority, but they are no longer stamped directly
    // into the surface as narrow recurring trenches. Visible structural
    // expression now comes from the finite curved events above.
    //-------------------------------------------------------------------------

    GraniteJointFieldSample EvaluateGraniteJointField(
        uint32_t               worldSeed,
        uint32_t               geologicalAncestryID,
        float                  worldX,
        float                  worldY,
        GraniteWeatheringState weathering )
    {
        GraniteGeometryProfile const  profile = GetGraniteGeometryProfile();
        MaterialGeometryProfile const materialProfile =
            GetMaterialGeometryProfile( ProvenanceMaterialID::Granite );

        GraniteJointFieldSample result;

        uint32_t const formationSeed =
            MixGraniteSeed( worldSeed, geologicalAncestryID );

        float const formationRotation =
            GetGraniteFormationRotation( worldSeed, geologicalAncestryID );

        // ---------------------------------------------------------------------
        // Formation-family receipts remain available for continuity/debug, but
        // visible geometry is authored by finite formation events below.
        // ---------------------------------------------------------------------

        for ( int32_t i = 0; i < 3; ++i )
        {
            GraniteJointSet const& jointSet = profile.m_jointSets[i];

            float const orientation =
                jointSet.m_orientationRadians +
                formationRotation;

            float along = 0.0f;
            float across = 0.0f;

            GetGraniteStructuralCoordinates(
                worldX,
                worldY,
                orientation,
                along,
                across );

            float const spacing = GetGraniteJointSpacing(
                jointSet,
                worldSeed,
                geologicalAncestryID,
                i );

            float const phase = GetGraniteJointPhase(
                worldSeed,
                geologicalAncestryID,
                i,
                spacing );

            float const normalized =
                ( across + phase ) /
                GraniteMax( spacing, 0.15f );

            float const nearestInteger = float(
                std::floor( double( normalized + 0.5f ) ) );

            result.m_jointDistanceM[i] =
                ( normalized - nearestInteger ) *
                spacing;

            // Never re-stamp the old infinite periodic trench field.
            result.m_jointStrength[i] = 0.0f;
        }

        // ---------------------------------------------------------------------
        // Legacy Step 7C local relief remains as restrained supporting detail.
        // Step 7D continuous formation descriptors now carry the dominant
        // outcrop body language.
        // ---------------------------------------------------------------------

        GraniteLargeSurfaceEventSample const events =
            EvaluateGraniteLargeSurfaceEvents(
                profile,
                formationSeed,
                formationRotation,
                worldX,
                worldY );

        GraniteFormationReliefSample const formation =
            EvaluateGraniteFormationRelief(
                worldSeed,
                geologicalAncestryID,
                worldX,
                worldY,
                weathering );

        result.m_broadWeatheredBackReliefM =
            formation.m_broadWeatheredBackReliefM;

        result.m_rootedRockHeadReliefM =
            formation.m_rootedRockHeadReliefM;

        result.m_slabShoulderReliefM =
            formation.m_slabShoulderReliefM;

        result.m_primaryFormationJointRecessM =
            formation.m_primaryJointRecessM;

        result.m_secondaryFormationJointRecessM =
            formation.m_secondaryJointRecessM;

        result.m_dominantFormationEventID =
            formation.m_dominantFormEventID;

        result.m_dominantFormationJointEventID =
            formation.m_dominantJointEventID;

        result.m_primaryJointInfluence = GraniteMax(
            formation.m_primaryJointInfluence,
            events.m_primaryInfluence * 0.32f );

        result.m_secondaryJointInfluence = GraniteMax(
            formation.m_secondaryJointInfluence,
            events.m_secondaryInfluence * 0.42f );

        result.m_primaryJointRecessM =
            result.m_primaryFormationJointRecessM +
            events.m_primaryRecessM * 0.28f;

        result.m_secondaryJointRecessM =
            result.m_secondaryFormationJointRecessM +
            events.m_secondaryRecessM * 0.38f;

        // ---------------------------------------------------------------------
        // Existing boulder-scale attached/partially-detached expression remains
        // valid player-scale structure. Detached candidates still contribute
        // zero parent-bedrock elevation.
        // ---------------------------------------------------------------------

        GraniteSurfaceStoneReliefSample const surfaceStones =
            EvaluateGraniteSurfaceStoneRelief(
                worldSeed,
                geologicalAncestryID,
                worldX,
                worldY );

        result.m_rockHeadReliefM =
            surfaceStones.m_rockHeadReliefM;

        result.m_partiallyDetachedReliefM =
            surfaceStones.m_partiallyDetachedReliefM;

        result.m_surfaceStoneShoulderReliefM =
            surfaceStones.m_shoulderReliefM;

        result.m_secondaryJointRecessM +=
            surfaceStones.m_detachmentRecessM;

        result.m_secondaryJointInfluence = GraniteMax(
            result.m_secondaryJointInfluence,
            surfaceStones.m_detachmentInfluence );

        result.m_combinedJointInfluence = GraniteClamp01(
            1.0f -
            ( 1.0f - result.m_primaryJointInfluence ) *
                ( 1.0f - result.m_secondaryJointInfluence ) );

        // ---------------------------------------------------------------------
        // Quiet coherent host carrier. Step 7D formation bodies dominate the
        // visible structure; this low-frequency field prevents empty cells from
        // becoming perfectly featureless without turning into another noise hill.
        // ---------------------------------------------------------------------

        float const rockBackA = SampleGraniteValueNoise(
            formationSeed,
            worldX,
            worldY,
            22.0f,
            0x510E527Fu );

        float const rockBackB = SampleGraniteValueNoise(
            formationSeed,
            worldX,
            worldY,
            11.5f,
            0x243185BEu );

        float const broadAmplitude =
            materialProfile.m_minimumMeaningfulFeatureScaleM *
            2.25f;

        float const coherentBack =
            ( rockBackA - 0.49f ) *
            broadAmplitude *
            0.88f;

        float const broadAsymmetry =
            ( rockBackB - 0.5f ) *
            broadAmplitude *
            0.26f;

        result.m_largeScaleReliefM =
            coherentBack * 0.62f +
            broadAsymmetry * 0.52f +
            events.m_largeReliefM * 0.12f +
            result.m_broadWeatheredBackReliefM +
            result.m_rootedRockHeadReliefM * 1.12f +
            result.m_slabShoulderReliefM * 0.34f +
            result.m_rockHeadReliefM * 0.48f +
            result.m_partiallyDetachedReliefM * 0.48f;

        // ---------------------------------------------------------------------
        // Meso structure: slab shoulders, joint shoulders and restrained local
        // articulation. This is where the large surface inherits the successful
        // B3 shoulder/fracture language without literally scaling a hand mesh.
        // ---------------------------------------------------------------------

        float const reliefSignalA = SampleGraniteValueNoise(
            formationSeed,
            worldX,
            worldY,
            7.8f,
            0x1F83D9ABu );

        float const reliefSignalB = SampleGraniteValueNoise(
            formationSeed,
            worldX,
            worldY,
            4.6f,
            0x5BE0CD19u );

        float const mediumAmplitude =
            materialProfile.m_minimumMeaningfulFeatureScaleM *
            0.82f;

        result.m_mediumScaleReliefM =
            result.m_slabShoulderReliefM * 0.66f +
            formation.m_jointShoulderReliefM +
            events.m_mediumReliefM * 0.22f +
            result.m_surfaceStoneShoulderReliefM * 0.38f +
            ( reliefSignalA - 0.5f ) *
                mediumAmplitude *
                0.42f +
            ( reliefSignalB - 0.5f ) *
                mediumAmplitude *
                0.14f;

        // ---------------------------------------------------------------------
        // Small roughness stays subordinate.
        // ---------------------------------------------------------------------

        float const smallSignalA = SampleGraniteValueNoise(
            formationSeed,
            worldX,
            worldY,
            1.45f,
            0xA54FF53Au );

        float const smallSignalB = SampleGraniteValueNoise(
            formationSeed,
            worldX,
            worldY,
            0.82f,
            0x13198A2Eu );

        float const smallAmplitude =
            materialProfile.m_minimumMeaningfulFeatureScaleM *
            0.18f;

        result.m_smallScaleReliefM =
            events.m_smallReliefM * 0.72f +
            ( smallSignalA - 0.5f ) *
                smallAmplitude *
                0.70f +
            ( smallSignalB - 0.5f ) *
                smallAmplitude *
                0.24f;

        // ---------------------------------------------------------------------
        // Weathering modifies the expression but never erases structural ancestry.
        // ---------------------------------------------------------------------

        float primaryAlong = 0.0f;
        float primaryAcross = 0.0f;

        GetGraniteStructuralCoordinates(
            worldX,
            worldY,
            profile.m_jointSets[0].m_orientationRadians +
                formationRotation,
            primaryAlong,
            primaryAcross );

        if ( weathering == GraniteWeatheringState::WeatheredExposure )
        {
            result.m_smallScaleReliefM *= 0.62f;
            result.m_mediumScaleReliefM *= 0.94f;
            result.m_secondaryJointRecessM *= 0.82f;
            result.m_primaryJointRecessM *= 0.97f;

            result.m_secondaryFormationJointRecessM *= 0.82f;
            result.m_primaryFormationJointRecessM *= 0.97f;
            result.m_slabShoulderReliefM *= 0.96f;

            float const domeNoise = SampleGraniteValueNoise(
                formationSeed,
                worldX,
                worldY,
                10.5f,
                0xCBBB9D5Du );

            float const exfoliationNoise = SampleGraniteValueNoise(
                formationSeed,
                primaryAlong * 0.42f,
                primaryAcross,
                6.0f,
                0x629A292Au );

            float const domeRelief =
                ( domeNoise - 0.47f ) *
                materialProfile.m_minimumMeaningfulFeatureScaleM *
                profile.m_domeRounding *
                1.72f;

            float const exfoliationRelief =
                ( exfoliationNoise - 0.5f ) *
                materialProfile.m_minimumMeaningfulFeatureScaleM *
                profile.m_exfoliationStrength *
                0.52f;

            result.m_weatheringReliefM =
                domeRelief +
                exfoliationRelief;
        }
        else
        {
            result.m_weatheringReliefM = 0.0f;
        }

        result.m_outcropReliefM =
            result.m_largeScaleReliefM +
            result.m_mediumScaleReliefM +
            result.m_smallScaleReliefM +
            result.m_primaryJointRecessM +
            result.m_secondaryJointRecessM;

        return result;
    }

    //-------------------------------------------------------------------------
    // Deterministic boulder-scale structural descriptor field
    //-------------------------------------------------------------------------

    GraniteSurfaceStoneField GenerateGraniteSurfaceStoneField(
        GraniteSurfaceStoneFieldRequest const& request )
    {
        EE_ASSERT( request.m_minWorldX <= request.m_maxWorldX );
        EE_ASSERT( request.m_minWorldY <= request.m_maxWorldY );

        GraniteSurfaceStoneField field;

        uint32_t const formationSeed = MixGraniteSeed(
            request.m_worldSeed,
            request.m_geologicalAncestryID );

        float const formationRotation = GetGraniteFormationRotation(
            request.m_worldSeed,
            request.m_geologicalAncestryID );

        float constexpr selectionPaddingM = 1.50f;

        int32_t const minCellX = int32_t(
            std::floor(
                double(
                    ( request.m_minWorldX - selectionPaddingM ) /
                    s_graniteSurfaceStoneCellSizeM ) ) );

        int32_t const minCellY = int32_t(
            std::floor(
                double(
                    ( request.m_minWorldY - selectionPaddingM ) /
                    s_graniteSurfaceStoneCellSizeM ) ) );

        int32_t const maxCellX = int32_t(
            std::floor(
                double(
                    ( request.m_maxWorldX + selectionPaddingM ) /
                    s_graniteSurfaceStoneCellSizeM ) ) );

        int32_t const maxCellY = int32_t(
            std::floor(
                double(
                    ( request.m_maxWorldY + selectionPaddingM ) /
                    s_graniteSurfaceStoneCellSizeM ) ) );

        for ( int32_t cellY = minCellY; cellY <= maxCellY; ++cellY )
        {
            for ( int32_t cellX = minCellX; cellX <= maxCellX; ++cellX )
            {
                for ( int32_t slotIndex = 0;
                      slotIndex < s_graniteSurfaceStoneSlotsPerCell;
                      ++slotIndex )
                {
                    GraniteSurfaceStoneDescriptor const descriptor =
                        BuildGraniteSurfaceStoneDescriptor(
                            formationSeed,
                            request.m_geologicalAncestryID,
                            formationRotation,
                            cellX,
                            cellY,
                            slotIndex );

                    if ( GraniteSurfaceStoneIntersectsBounds(
                             descriptor,
                             request.m_minWorldX,
                             request.m_minWorldY,
                             request.m_maxWorldX,
                             request.m_maxWorldY ) )
                    {
                        InsertGraniteSurfaceStoneDescriptor(
                            field,
                            descriptor );
                    }
                }
            }
        }

        SortGraniteSurfaceStoneField( field );

        float dominantImportance = -1.0f;

        for ( int32_t i = 0; i < field.m_numDescriptors; ++i )
        {
            GraniteSurfaceStoneDescriptor const& descriptor =
                field.m_descriptors[i];

            switch ( descriptor.m_state )
            {
                case GraniteSurfaceStoneState::AttachedRockHead:
                    ++field.m_numAttachedRockHeads;
                    break;

                case GraniteSurfaceStoneState::PartiallyDetachedBlock:
                    ++field.m_numPartiallyDetachedBlocks;
                    break;

                case GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate:
                    ++field.m_numDetachedCandidates;
                    break;
            }

            if ( descriptor.m_state !=
                 GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate )
            {
                float const importance =
                    GetGraniteSurfaceStoneImportance( descriptor );

                if ( importance > dominantImportance )
                {
                    dominantImportance = importance;
                    field.m_dominantEventID = descriptor.m_eventID;
                }
            }
        }

        return field;
    }

    //-------------------------------------------------------------------------
    // Deterministic continuous-formation descriptor field
    //-------------------------------------------------------------------------

    GraniteFormationField GenerateGraniteFormationField(
        GraniteFormationFieldRequest const& request )
    {
        EE_ASSERT( request.m_minWorldX <= request.m_maxWorldX );
        EE_ASSERT( request.m_minWorldY <= request.m_maxWorldY );

        GraniteFormationField field;

        uint32_t const formationSeed = MixGraniteSeed(
            request.m_worldSeed,
            request.m_geologicalAncestryID );

        float const formationRotation = GetGraniteFormationRotation(
            request.m_worldSeed,
            request.m_geologicalAncestryID );

        float constexpr formSelectionPaddingM = 15.0f;

        int32_t const minFormCellX = int32_t(
            std::floor(
                double(
                    ( request.m_minWorldX - formSelectionPaddingM ) /
                    s_graniteFormationFormCellSizeM ) ) );

        int32_t const minFormCellY = int32_t(
            std::floor(
                double(
                    ( request.m_minWorldY - formSelectionPaddingM ) /
                    s_graniteFormationFormCellSizeM ) ) );

        int32_t const maxFormCellX = int32_t(
            std::floor(
                double(
                    ( request.m_maxWorldX + formSelectionPaddingM ) /
                    s_graniteFormationFormCellSizeM ) ) );

        int32_t const maxFormCellY = int32_t(
            std::floor(
                double(
                    ( request.m_maxWorldY + formSelectionPaddingM ) /
                    s_graniteFormationFormCellSizeM ) ) );

        for ( int32_t cellY = minFormCellY; cellY <= maxFormCellY; ++cellY )
        {
            for ( int32_t cellX = minFormCellX; cellX <= maxFormCellX; ++cellX )
            {
                for ( int32_t slotIndex = 0;
                      slotIndex < s_graniteFormationFormSlotsPerCell;
                      ++slotIndex )
                {
                    GraniteFormationFormDescriptor const descriptor =
                        BuildGraniteFormationFormDescriptor(
                            formationSeed,
                            request.m_geologicalAncestryID,
                            formationRotation,
                            cellX,
                            cellY,
                            slotIndex );

                    if ( GraniteFormationFormIntersectsBounds(
                             descriptor,
                             request.m_minWorldX,
                             request.m_minWorldY,
                             request.m_maxWorldX,
                             request.m_maxWorldY ) )
                    {
                        InsertGraniteFormationFormDescriptor(
                            field,
                            descriptor );
                    }
                }
            }
        }

        float constexpr jointSelectionPaddingM = 13.0f;

        int32_t const minJointCellX = int32_t(
            std::floor(
                double(
                    ( request.m_minWorldX - jointSelectionPaddingM ) /
                    s_graniteFormationJointCellSizeM ) ) );

        int32_t const minJointCellY = int32_t(
            std::floor(
                double(
                    ( request.m_minWorldY - jointSelectionPaddingM ) /
                    s_graniteFormationJointCellSizeM ) ) );

        int32_t const maxJointCellX = int32_t(
            std::floor(
                double(
                    ( request.m_maxWorldX + jointSelectionPaddingM ) /
                    s_graniteFormationJointCellSizeM ) ) );

        int32_t const maxJointCellY = int32_t(
            std::floor(
                double(
                    ( request.m_maxWorldY + jointSelectionPaddingM ) /
                    s_graniteFormationJointCellSizeM ) ) );

        for ( int32_t cellY = minJointCellY; cellY <= maxJointCellY; ++cellY )
        {
            for ( int32_t cellX = minJointCellX; cellX <= maxJointCellX; ++cellX )
            {
                for ( int32_t slotIndex = 0;
                      slotIndex < s_graniteFormationJointSlotsPerCell;
                      ++slotIndex )
                {
                    GraniteFormationJointDescriptor const descriptor =
                        BuildGraniteFormationJointDescriptor(
                            formationSeed,
                            request.m_geologicalAncestryID,
                            formationRotation,
                            cellX,
                            cellY,
                            slotIndex );

                    if ( GraniteFormationJointIntersectsBounds(
                             descriptor,
                             request.m_minWorldX,
                             request.m_minWorldY,
                             request.m_maxWorldX,
                             request.m_maxWorldY ) )
                    {
                        InsertGraniteFormationJointDescriptor(
                            field,
                            descriptor );
                    }
                }
            }
        }

        SortGraniteFormationField( field );

        float dominantFormImportance = -1.0f;
        float dominantJointImportance = -1.0f;

        for ( int32_t i = 0; i < field.m_numForms; ++i )
        {
            GraniteFormationFormDescriptor const& descriptor =
                field.m_forms[i];

            switch ( descriptor.m_type )
            {
                case GraniteFormationFormType::BroadWeatheredBack:
                    ++field.m_numBroadWeatheredBacks;
                    break;

                case GraniteFormationFormType::RootedRockHead:
                    ++field.m_numRootedRockHeads;
                    break;

                case GraniteFormationFormType::SlabShoulder:
                    ++field.m_numSlabShoulders;
                    break;
            }

            float const importance =
                GetGraniteFormationFormImportance( descriptor );

            if ( importance > dominantFormImportance )
            {
                dominantFormImportance = importance;
                field.m_dominantFormEventID = descriptor.m_eventID;
            }
        }

        for ( int32_t i = 0; i < field.m_numJoints; ++i )
        {
            GraniteFormationJointDescriptor const& descriptor =
                field.m_joints[i];

            if ( descriptor.m_tier == GraniteJointTier::Primary )
            {
                ++field.m_numPrimaryJoints;
            }
            else
            {
                ++field.m_numSecondaryJoints;
            }

            float const importance =
                GetGraniteFormationJointImportance( descriptor );

            if ( importance > dominantJointImportance )
            {
                dominantJointImportance = importance;
                field.m_dominantJointEventID = descriptor.m_eventID;
            }
        }

        return field;
    }

    //-------------------------------------------------------------------------
    // Continuous Granite outcrop grammar
    //-------------------------------------------------------------------------

    GraniteOutcropResult EvaluateGraniteOutcropGeometry(
        GraniteOutcropRequest const& request )
    {
        GraniteOutcropResult result;

        result.m_baseBedrockZ = request.m_baseBedrockZ;

        result.m_jointField = EvaluateGraniteJointField(
            request.m_worldSeed,
            request.m_geologicalAncestryID,
            request.m_worldX,
            request.m_worldY,
            request.m_weathering );

        result.m_largeScaleReliefM =
            result.m_jointField.m_largeScaleReliefM;

        result.m_mediumScaleReliefM =
            result.m_jointField.m_mediumScaleReliefM;

        result.m_smallScaleReliefM =
            result.m_jointField.m_smallScaleReliefM;

        result.m_primaryJointRecessM =
            result.m_jointField.m_primaryJointRecessM;

        result.m_secondaryJointRecessM =
            result.m_jointField.m_secondaryJointRecessM;

        result.m_structuralReliefM =
            result.m_jointField.m_outcropReliefM;

        result.m_weatheringReliefM =
            result.m_jointField.m_weatheringReliefM;

        result.m_broadWeatheredBackReliefM =
            result.m_jointField.m_broadWeatheredBackReliefM;

        result.m_rootedRockHeadReliefM =
            result.m_jointField.m_rootedRockHeadReliefM;

        result.m_slabShoulderReliefM =
            result.m_jointField.m_slabShoulderReliefM;

        result.m_primaryFormationJointRecessM =
            result.m_jointField.m_primaryFormationJointRecessM;

        result.m_secondaryFormationJointRecessM =
            result.m_jointField.m_secondaryFormationJointRecessM;

        result.m_surfaceZ =
            result.m_baseBedrockZ +
            result.m_structuralReliefM +
            result.m_weatheringReliefM;

        return result;
    }

}
