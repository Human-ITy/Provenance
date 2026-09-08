#include "SurfaceAuthorityVerifier.h"

#include <cstddef>
#include <cstring>

namespace EE
{
    namespace
    {
        static constexpr uint32_t s_surfaceFingerprintSchemaVersion = 1u;

        struct SurfaceFingerprintBuilder
        {
            uint64_t m_low = 14695981039346656037ull;
            uint64_t m_high = 1099511628211ull ^ 0x9E3779B97F4A7C15ull;

            void AppendByte( uint8_t value )
            {
                m_low ^= uint64_t( value );
                m_low *= 1099511628211ull;
                m_high ^= uint64_t( value ) +
                    0x9E3779B97F4A7C15ull +
                    ( m_high << 6 ) +
                    ( m_high >> 2 );
                m_high *= 0xD6E8FEB86659FD93ull;
            }

            void AppendU32( uint32_t value )
            {
                for ( int32_t byteIndex = 0; byteIndex < 4; ++byteIndex )
                {
                    AppendByte(
                        uint8_t(
                            ( value >> ( byteIndex * 8 ) ) & 0xFFu ) );
                }
            }

            void AppendU64( uint64_t value )
            {
                for ( int32_t byteIndex = 0; byteIndex < 8; ++byteIndex )
                {
                    AppendByte(
                        uint8_t(
                            ( value >> ( byteIndex * 8 ) ) & 0xFFull ) );
                }
            }

            void AppendFloat( float value )
            {
                uint32_t bits = 0;
                static_assert(
                    sizeof( bits ) == sizeof( value ),
                    "Surface fingerprints require binary32 float" );
                std::memcpy( &bits, &value, sizeof( bits ) );
                AppendU32( bits );
            }
        };

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
        static_assert(
            sizeof( GraniteJointFieldSample ) == 104 &&
                alignof( GraniteJointFieldSample ) == 4 &&
                offsetof(
                    GraniteJointFieldSample,
                    m_dominantFormationJointEventID ) == 100,
            "Canonical GraniteJointFieldSample layout changed" );
        static_assert(
            sizeof( EvaluatedSurfacePoint ) == 204 &&
                alignof( EvaluatedSurfacePoint ) == 4 &&
                offsetof( EvaluatedSurfacePoint, m_material ) == 200,
            "Canonical EvaluatedSurfacePoint layout changed" );
#endif

        static void AppendFailure(
            ProvenanceSurfaceAuthorityVerificationReport& report,
            int32_t                                       caseOrdinal,
            char const*                                   gateName )
        {
            if ( report.m_numFailures >=
                 ProvenanceSurfaceAuthorityVerificationReport::
                    s_maxFailures )
            {
                report.m_failureCapacityExceeded = true;
                return;
            }

            ProvenanceSurfaceAuthorityFailure& failure =
                report.m_failures[report.m_numFailures++];
            failure.m_caseOrdinal = caseOrdinal;
            failure.m_gateName = gateName;
        }

        static void CheckDeterminism(
            ProvenanceSurfaceAuthorityVerificationReport& report,
            bool                                           pass,
            int32_t                                        caseOrdinal,
            char const*                                    gateName )
        {
            ++report.m_numDeterminismChecks;
            if ( !pass )
            {
                AppendFailure( report, caseOrdinal, gateName );
            }
        }

        static void FingerprintJointField(
            SurfaceFingerprintBuilder&       builder,
            GraniteJointFieldSample const& joint )
        {
            for ( int32_t jointIndex = 0; jointIndex < 3; ++jointIndex )
            {
                builder.AppendFloat( joint.m_jointDistanceM[jointIndex] );
                builder.AppendFloat( joint.m_jointStrength[jointIndex] );
            }
            builder.AppendFloat( joint.m_primaryJointInfluence );
            builder.AppendFloat( joint.m_secondaryJointInfluence );
            builder.AppendFloat( joint.m_combinedJointInfluence );
            builder.AppendFloat( joint.m_largeScaleReliefM );
            builder.AppendFloat( joint.m_mediumScaleReliefM );
            builder.AppendFloat( joint.m_smallScaleReliefM );
            builder.AppendFloat( joint.m_primaryJointRecessM );
            builder.AppendFloat( joint.m_secondaryJointRecessM );
            builder.AppendFloat( joint.m_outcropReliefM );
            builder.AppendFloat( joint.m_weatheringReliefM );
            builder.AppendFloat( joint.m_rockHeadReliefM );
            builder.AppendFloat( joint.m_partiallyDetachedReliefM );
            builder.AppendFloat( joint.m_surfaceStoneShoulderReliefM );
            builder.AppendFloat( joint.m_broadWeatheredBackReliefM );
            builder.AppendFloat( joint.m_rootedRockHeadReliefM );
            builder.AppendFloat( joint.m_slabShoulderReliefM );
            builder.AppendFloat( joint.m_primaryFormationJointRecessM );
            builder.AppendFloat( joint.m_secondaryFormationJointRecessM );
            builder.AppendU32( joint.m_dominantFormationEventID );
            builder.AppendU32( joint.m_dominantFormationJointEventID );
        }

        static ProvenanceSurfaceCaseFingerprint BuildSurfaceFingerprint(
            uint32_t                     worldSeed,
            float                        worldX,
            float                        worldY,
            EvaluatedSurfacePoint const& surface )
        {
            SurfaceFingerprintBuilder builder;
            builder.AppendU32( s_surfaceFingerprintSchemaVersion );
            builder.AppendU32( s_provenanceSurfaceAlgorithmVersion );
            builder.AppendU32( worldSeed );
            builder.AppendFloat( worldX );
            builder.AppendFloat( worldY );
            builder.AppendFloat( surface.m_broadLandform );
            builder.AppendFloat( surface.m_graniteBroadBodyOffset );
            builder.AppendFloat( surface.m_bedrockBase );
            builder.AppendFloat( surface.m_graniteLargeScaleRelief );
            builder.AppendFloat( surface.m_graniteMediumScaleRelief );
            builder.AppendFloat( surface.m_graniteSmallScaleRelief );
            builder.AppendFloat( surface.m_granitePrimaryJointRecess );
            builder.AppendFloat( surface.m_graniteSecondaryJointRecess );
            builder.AppendFloat( surface.m_graniteStructuralRelief );
            builder.AppendFloat( surface.m_graniteWeatheringRelief );
            builder.AppendFloat( surface.m_graniteJointInfluence );
            FingerprintJointField( builder, surface.m_graniteJointField );
            builder.AppendFloat( surface.m_bedrockElevation );
            builder.AppendFloat( surface.m_soilCoverBudget );
            builder.AppendFloat( surface.m_soilMantleResponse );
            builder.AppendFloat( surface.m_soilGraniteInfillResponse );
            builder.AppendFloat( surface.m_mantleActivation );
            builder.AppendFloat( surface.m_largeGraniteVisibilityUnderSoil );
            builder.AppendFloat( surface.m_mediumGraniteVisibilityUnderSoil );
            builder.AppendFloat( surface.m_smallGraniteVisibilityUnderSoil );
            builder.AppendFloat( surface.m_filteredGraniteInfluenceUnderSoil );
            builder.AppendFloat( surface.m_soilSurfaceElevation );
            builder.AppendFloat( surface.m_signedSoilDepth );
            builder.AppendFloat( surface.m_soilThickness );
            builder.AppendFloat( surface.m_elevation );
            builder.AppendU32( uint32_t( surface.m_material ) );

            ProvenanceSurfaceCaseFingerprint fingerprint;
            fingerprint.m_worldSeed = worldSeed;
            fingerprint.m_worldX = worldX;
            fingerprint.m_worldY = worldY;
            fingerprint.m_low = builder.m_low;
            fingerprint.m_high = builder.m_high;
            return fingerprint;
        }

        static bool FingerprintsEqual(
            ProvenanceSurfaceCaseFingerprint const& lhs,
            ProvenanceSurfaceCaseFingerprint const& rhs )
        {
            uint32_t lhsX = 0;
            uint32_t rhsX = 0;
            uint32_t lhsY = 0;
            uint32_t rhsY = 0;
            std::memcpy( &lhsX, &lhs.m_worldX, sizeof( lhsX ) );
            std::memcpy( &rhsX, &rhs.m_worldX, sizeof( rhsX ) );
            std::memcpy( &lhsY, &lhs.m_worldY, sizeof( lhsY ) );
            std::memcpy( &rhsY, &rhs.m_worldY, sizeof( rhsY ) );
            return
                lhs.m_worldSeed == rhs.m_worldSeed &&
                lhsX == rhsX &&
                lhsY == rhsY &&
                lhs.m_low == rhs.m_low &&
                lhs.m_high == rhs.m_high;
        }

        static float MutateFloatOneBit( float value )
        {
            uint32_t bits = 0;
            std::memcpy( &bits, &value, sizeof( bits ) );
            bits ^= 1u;
            std::memcpy( &value, &bits, sizeof( value ) );
            return value;
        }
    }

    ProvenanceSurfaceAuthorityVerificationReport
    RunProvenanceSurfaceAuthorityVerificationCorpus()
    {
        ProvenanceSurfaceAuthorityVerificationReport report;

        struct SurfaceCorpusCase
        {
            uint32_t m_worldSeed;
            float    m_worldX;
            float    m_worldY;
        };

        static constexpr SurfaceCorpusCase corpus[] =
        {
            { 8675309u, 0.0f, 0.0f },
            { 8675309u, 0.25f, -0.25f },
            { 8675309u, 31.999f, 32.001f },
            { 8675309u, -64.125f, 127.875f },
            { 0x00C0FFEEu, 1.0f, 1.0f },
            { 0x00C0FFEEu, -0.001f, 0.001f },
            { 0x00C0FFEEu, 255.5f, -511.25f },
            { 0x00C0FFEEu, 4096.125f, 4095.875f },
            { 0x5EED1234u, -1024.0f, -1024.0f },
            { 0x5EED1234u, 73.125f, -19.75f },
            { 0x5EED1234u, 100000.5f, -200000.25f },
            { 0x5EED1234u, 0.03125f, 0.0625f },
            { 0x5EED1235u, -8192.5f, 16384.25f },
            { 0x5EED1235u, 511.999f, 512.001f },
            { 0x5EED1235u, -32768.75f, -65536.125f },
            { 0x5EED1235u, 3.1415927f, -2.7182817f }
        };

        // Banked only after Debug and /O2 /fp:precise Release produced the
        // same raw float-bit results. Algorithm and schema versions are hash
        // inputs, so either version changing without a complete attributed
        // re-bank fails every approved comparison.
        static constexpr ProvenanceSurfaceCaseFingerprint approved[] =
        {
            { 8675309u, 0.0f, 0.0f, 0xD05923EB36B29581ull, 0xC337CA0D552B270Eull },
            { 8675309u, 0.25f, -0.25f, 0x4420662A7A9F08F2ull, 0xCFAAF349AAE6A26Cull },
            { 8675309u, 31.999f, 32.001f, 0x767974BAC9C2C10Aull, 0x8D41978C19405F92ull },
            { 8675309u, -64.125f, 127.875f, 0x9798B40A641C18AEull, 0x261F0984D8EBB1FCull },
            { 0x00C0FFEEu, 1.0f, 1.0f, 0x49A192CB99321DB4ull, 0x3DE4F4E6B5FF0F1Aull },
            { 0x00C0FFEEu, -0.001f, 0.001f, 0xA96183E50DFF245Cull, 0xC1237C4741C7EDA6ull },
            { 0x00C0FFEEu, 255.5f, -511.25f, 0xD5B02D1A7EDE01FAull, 0x31FCC69370272617ull },
            { 0x00C0FFEEu, 4096.125f, 4095.875f, 0x04C1E2C57A1DFA29ull, 0x073F5A747348C9AFull },
            { 0x5EED1234u, -1024.0f, -1024.0f, 0xBE211AE333AF491Dull, 0xF30836BA25A644AEull },
            { 0x5EED1234u, 73.125f, -19.75f, 0xBFC539178214A96Dull, 0xC15DE4E3EF396287ull },
            { 0x5EED1234u, 100000.5f, -200000.25f, 0x323286BF7E9BB4FBull, 0x940F95DE635EB902ull },
            { 0x5EED1234u, 0.03125f, 0.0625f, 0x289FD711D0EFFB52ull, 0x1F2A720E415AE56Eull },
            { 0x5EED1235u, -8192.5f, 16384.25f, 0x7061A1A1E1EFC22Bull, 0x11D2A0CFA30B1BF3ull },
            { 0x5EED1235u, 511.999f, 512.001f, 0xA2B15D61606F68F3ull, 0x2568050408DAB3D4ull },
            { 0x5EED1235u, -32768.75f, -65536.125f, 0xCB86346E142FF8F9ull, 0xA7F8D59CE2220ACCull },
            { 0x5EED1235u, 3.1415927f, -2.7182817f, 0x965C4B6D5630C7DAull, 0x9F9B17D89259B2D5ull }
        };

        static_assert(
            sizeof( corpus ) / sizeof( corpus[0] ) ==
                ProvenanceSurfaceAuthorityVerificationReport::s_maxCases,
            "Surface corpus/report capacity mismatch" );
        static_assert(
            sizeof( corpus ) / sizeof( corpus[0] ) ==
                sizeof( approved ) / sizeof( approved[0] ),
            "Every surface corpus case requires an approved fingerprint" );

        for ( int32_t caseOrdinal = 0;
              caseOrdinal < int32_t( sizeof( corpus ) / sizeof( corpus[0] ) );
              ++caseOrdinal )
        {
            SurfaceCorpusCase const& corpusCase = corpus[caseOrdinal];
            ++report.m_numCases;

            EvaluatedSurfacePoint const surface =
                EvaluateSurfacePoint(
                    corpusCase.m_worldSeed,
                    corpusCase.m_worldX,
                    corpusCase.m_worldY );
            ProvenanceSurfaceCaseFingerprint const fingerprint =
                BuildSurfaceFingerprint(
                    corpusCase.m_worldSeed,
                    corpusCase.m_worldX,
                    corpusCase.m_worldY,
                    surface );

            CheckDeterminism(
                report,
                FingerprintsEqual( fingerprint, approved[caseOrdinal] ),
                caseOrdinal,
                "ApprovedBitPatternMatch" );

            report.m_fingerprints[report.m_numFingerprints++] = fingerprint;

            EvaluatedSurfacePoint const repeatSurface =
                EvaluateSurfacePoint(
                    corpusCase.m_worldSeed,
                    corpusCase.m_worldX,
                    corpusCase.m_worldY );
            ProvenanceSurfaceCaseFingerprint const repeatFingerprint =
                BuildSurfaceFingerprint(
                    corpusCase.m_worldSeed,
                    corpusCase.m_worldX,
                    corpusCase.m_worldY,
                    repeatSurface );
            CheckDeterminism(
                report,
                FingerprintsEqual( fingerprint, repeatFingerprint ),
                caseOrdinal,
                "RepeatBitPatternMatch" );

            if ( caseOrdinal == 0 )
            {
                ProvenanceSurfaceCaseFingerprint hashMutation = fingerprint;
                hashMutation.m_low ^= 1ull;
                ++report.m_numNegativeControls;
                if ( FingerprintsEqual( fingerprint, hashMutation ) )
                {
                    AppendFailure(
                        report,
                        caseOrdinal,
                        "FingerprintLowBitMutation" );
                }

                EvaluatedSurfacePoint surfaceMutation = surface;
                surfaceMutation.m_elevation =
                    MutateFloatOneBit( surfaceMutation.m_elevation );
                ProvenanceSurfaceCaseFingerprint const fieldMutation =
                    BuildSurfaceFingerprint(
                        corpusCase.m_worldSeed,
                        corpusCase.m_worldX,
                        corpusCase.m_worldY,
                        surfaceMutation );
                ++report.m_numNegativeControls;
                if ( FingerprintsEqual( fingerprint, fieldMutation ) )
                {
                    AppendFailure(
                        report,
                        caseOrdinal,
                        "ElevationFloatReachability" );
                }
            }
        }

        report.m_pass =
            report.m_numFailures == 0 &&
            !report.m_failureCapacityExceeded;
        return report;
    }
}
