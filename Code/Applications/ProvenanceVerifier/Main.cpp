#include "Game/Provenance/Verification/GraniteAuthorityVerifier.h"
#include "Game/Provenance/Verification/SurfaceAuthorityVerifier.h"

#include <cstdio>

int main()
{
    EE::GraniteAuthorityVerificationReport const report =
        EE::RunGraniteAuthorityVerificationCorpus();
    EE::ProvenanceSurfaceAuthorityVerificationReport const surfaceReport =
        EE::RunProvenanceSurfaceAuthorityVerificationCorpus();

    std::printf(
        "Provenance Granite Authority Verifier\n"
        "corpus cases: %d\n"
        "gate checks: %d\n"
        "negative controls: %d\n"
        "tool-strike algorithm version: %u\n"
        "tool-strike cases: %d\n"
        "tool-strike checks: %d\n"
        "local-excavation checks: %d\n"
        "shape cast-fit max deviation: %.9f m\n"
        "shape retained apron mass: %u..%u g\n"
        "shape retained apron fraction: %.9f..%.9f\n"
        "shape underside fin fraction: %.9f..%.9f\n"
        "geometry algorithm version: %u\n"
        "fingerprint schema version: %u\n"
        "determinism fingerprints: %d\n"
        "determinism checks: %d\n"
        "failures: %d\n",
        report.m_numCorpusCases,
        report.m_numGateChecks,
        report.m_numNegativeControls,
        static_cast<unsigned int>( report.m_toolStrikeAlgorithmVersion ),
        report.m_numToolStrikeCases,
        report.m_numToolStrikeChecks,
        report.m_numExcavationChecks,
        static_cast<double>( report.m_maxPrincipalCastFitDeviationM ),
        static_cast<unsigned int>( report.m_minRetainedApronMassGrams ),
        static_cast<unsigned int>( report.m_maxRetainedApronMassGrams ),
        static_cast<double>( report.m_minRetainedApronMassFraction ),
        static_cast<double>( report.m_maxRetainedApronMassFraction ),
        static_cast<double>( report.m_minUndersideFinVolumeFraction ),
        static_cast<double>( report.m_maxUndersideFinVolumeFraction ),
        static_cast<unsigned int>( report.m_geometryAlgorithmVersion ),
        static_cast<unsigned int>( report.m_fingerprintSchemaVersion ),
        report.m_numFingerprints,
        report.m_numDeterminismChecks,
        report.m_numFailures );

    for ( int32_t fingerprintIndex = 0;
          fingerprintIndex < report.m_numFingerprints;
          ++fingerprintIndex )
    {
        EE::GraniteAuthorityCaseFingerprint const& fingerprint =
            report.m_fingerprints[fingerprintIndex];
        std::printf(
            "FINGERPRINT seed=%u topology=%u bits=%016llX%016llX\n",
            static_cast<unsigned int>( fingerprint.m_worldSeed ),
            static_cast<unsigned int>( fingerprint.m_topologyFamily ),
            static_cast<unsigned long long>( fingerprint.m_high ),
            static_cast<unsigned long long>( fingerprint.m_low ) );
    }

    for ( int32_t failureIndex = 0;
          failureIndex < report.m_numFailures;
          ++failureIndex )
    {
        EE::GraniteAuthorityFailure const& failure =
            report.m_failures[failureIndex];

        std::printf(
            "FAIL case=%u receipt=%s gate=%s\n",
            static_cast<unsigned int>( failure.m_caseID ),
            failure.m_receiptName != nullptr
                ? failure.m_receiptName
                : "<unknown>",
            failure.m_gateName != nullptr
                ? failure.m_gateName
                : "<unknown>" );
    }

    if ( report.m_failureCapacityExceeded )
    {
        std::printf( "FAIL failure list capacity exceeded\n" );
    }

    std::printf(
        "Provenance Absolute-XY Surface Authority Verifier\n"
        "surface corpus cases: %d\n"
        "surface determinism fingerprints: %d\n"
        "surface determinism checks: %d\n"
        "surface negative controls: %d\n"
        "surface algorithm version: %u\n"
        "surface fingerprint schema version: %u\n"
        "surface failures: %d\n",
        surfaceReport.m_numCases,
        surfaceReport.m_numFingerprints,
        surfaceReport.m_numDeterminismChecks,
        surfaceReport.m_numNegativeControls,
        static_cast<unsigned int>(
            surfaceReport.m_surfaceAlgorithmVersion ),
        static_cast<unsigned int>(
            surfaceReport.m_fingerprintSchemaVersion ),
        surfaceReport.m_numFailures );

    for ( int32_t fingerprintIndex = 0;
          fingerprintIndex < surfaceReport.m_numFingerprints;
          ++fingerprintIndex )
    {
        EE::ProvenanceSurfaceCaseFingerprint const& fingerprint =
            surfaceReport.m_fingerprints[fingerprintIndex];
        std::printf(
            "SURFACE_FINGERPRINT seed=%u x=%.9g y=%.9g bits=%016llX%016llX\n",
            static_cast<unsigned int>( fingerprint.m_worldSeed ),
            static_cast<double>( fingerprint.m_worldX ),
            static_cast<double>( fingerprint.m_worldY ),
            static_cast<unsigned long long>( fingerprint.m_high ),
            static_cast<unsigned long long>( fingerprint.m_low ) );
    }

    for ( int32_t failureIndex = 0;
          failureIndex < surfaceReport.m_numFailures;
          ++failureIndex )
    {
        EE::ProvenanceSurfaceAuthorityFailure const& failure =
            surfaceReport.m_failures[failureIndex];
        std::printf(
            "SURFACE_FAIL case=%d gate=%s\n",
            failure.m_caseOrdinal,
            failure.m_gateName != nullptr
                ? failure.m_gateName
                : "<unknown>" );
    }

    if ( surfaceReport.m_failureCapacityExceeded )
    {
        std::printf( "SURFACE_FAIL failure list capacity exceeded\n" );
    }

    std::printf(
        "%s\n",
        report.m_pass && surfaceReport.m_pass
            ? "PASS"
            : "FAIL" );

    return report.m_pass && surfaceReport.m_pass ? 0 : 1;
}
