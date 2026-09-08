#pragma once

#include "Game/Provenance/Systems/ProvenanceSurfaceEvaluation.h"

namespace EE
{
#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
    #define EE_PROVENANCE_SURFACE_AUTHORITY_API
#else
    #define EE_PROVENANCE_SURFACE_AUTHORITY_API EE_GAME_API
#endif

    struct ProvenanceSurfaceAuthorityFailure
    {
        int32_t     m_caseOrdinal = -1;
        char const* m_gateName = nullptr;
    };

    struct ProvenanceSurfaceCaseFingerprint
    {
        uint32_t m_worldSeed = 0;
        float    m_worldX = 0.0f;
        float    m_worldY = 0.0f;
        uint64_t m_low = 0;
        uint64_t m_high = 0;
    };

    struct ProvenanceSurfaceAuthorityVerificationReport
    {
        static constexpr int32_t s_maxCases = 16;
        static constexpr int32_t s_maxFailures = 64;

        ProvenanceSurfaceAuthorityFailure m_failures[s_maxFailures];
        ProvenanceSurfaceCaseFingerprint m_fingerprints[s_maxCases];

        uint32_t m_surfaceAlgorithmVersion =
            s_provenanceSurfaceAlgorithmVersion;
        uint32_t m_fingerprintSchemaVersion = 1u;

        int32_t m_numCases = 0;
        int32_t m_numFingerprints = 0;
        int32_t m_numDeterminismChecks = 0;
        int32_t m_numNegativeControls = 0;
        int32_t m_numFailures = 0;

        bool m_failureCapacityExceeded = false;
        bool m_pass = false;
    };

    EE_PROVENANCE_SURFACE_AUTHORITY_API
    ProvenanceSurfaceAuthorityVerificationReport
    RunProvenanceSurfaceAuthorityVerificationCorpus();

#undef EE_PROVENANCE_SURFACE_AUTHORITY_API
}
