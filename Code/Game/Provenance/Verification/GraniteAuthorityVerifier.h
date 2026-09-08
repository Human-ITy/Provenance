#pragma once

#include "Game/Provenance/Geometry/GraniteGeometry.h"

namespace EE
{
#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
    #define EE_PROVENANCE_AUTHORITY_API
#else
    #define EE_PROVENANCE_AUTHORITY_API EE_GAME_API
#endif

    //-------------------------------------------------------------------------
    // Release-independent Granite authority verification.
    //
    // The existing receipt booleans remain the source of the named stage
    // gates. This layer gives those gates consumers other than EE_ASSERT and
    // DebugDraw, and closes the terminal matter chain without counting the
    // intermediate spall reserve as a final output.
    //-------------------------------------------------------------------------

    struct GraniteAuthorityFailure
    {
        uint32_t    m_caseID = 0;
        char const* m_receiptName = nullptr;
        char const* m_gateName = nullptr;
    };

    struct GraniteAuthorityCaseFingerprint
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_topologyFamily = 0;
        uint64_t m_low = 0;
        uint64_t m_high = 0;
    };

    struct GraniteAuthorityVerificationReport
    {
        static constexpr int32_t s_maxFailures = 128;
        static constexpr int32_t s_maxFingerprints = 16;

        GraniteAuthorityFailure m_failures[s_maxFailures];

        int32_t m_numFailures = 0;
        int32_t m_numCorpusCases = 0;
        int32_t m_numGateChecks = 0;
        int32_t m_numNegativeControls = 0;

        // Additive P3C.11A-1 strike-input corpus. These cases exercise the
        // new input path and never replace/re-bank the 16 baseline goldens.
        uint32_t m_toolStrikeAlgorithmVersion =
            s_graniteToolStrikeAlgorithmVersion;
        int32_t m_numToolStrikeCases = 0;
        int32_t m_numToolStrikeChecks = 0;

        // Separate cell-based local excavation certificate; never part of the
        // frozen bridge-input or geometry fingerprint corpus.
        int32_t m_numExcavationChecks = 0;

        // Task 4 exact-output determinism corpus. These are canonical hashes
        // of explicitly serialized authority fields; struct padding and
        // inactive fixed-capacity storage are never observed.
        uint32_t m_geometryAlgorithmVersion =
            s_graniteGeometryAlgorithmVersion;
        uint32_t m_fingerprintSchemaVersion = 1u;
        GraniteAuthorityCaseFingerprint
            m_fingerprints[s_maxFingerprints];
        int32_t m_numFingerprints = 0;
        int32_t m_numDeterminismChecks = 0;

        // Task 3 approved-shape corpus envelope.
        float m_maxPrincipalCastFitDeviationM = 0.0f;
        uint32_t m_minRetainedApronMassGrams = 0;
        uint32_t m_maxRetainedApronMassGrams = 0;
        float m_minRetainedApronMassFraction = 0.0f;
        float m_maxRetainedApronMassFraction = 0.0f;
        float m_minUndersideFinVolumeFraction = 0.0f;
        float m_maxUndersideFinVolumeFraction = 0.0f;
        bool  m_shapeEnvelopeInitialized = false;

        bool m_failureCapacityExceeded = false;
        bool m_pass = false;
    };

    //-------------------------------------------------------------------------
    // Terminal matter conservation
    //
    // Partition stage:
    //
    //     pre-detach parcel
    //         = parent remainder + principal detached body + spall reserve
    //
    // Resolution stage:
    //
    //     spall reserve
    //         = actual explicit pieces + fine reserve
    //
    // Terminal stage:
    //
    //     pre-detach parcel
    //         = parent remainder + principal detached body
    //           + actual explicit pieces + fine reserve
    //
    // The intermediate reserve is deliberately absent from the terminal sum.
    //-------------------------------------------------------------------------

    struct GraniteTerminalConservationReceipt
    {
        uint32_t m_preDetachParcelMassGrams = 0;
        uint32_t m_remainingParentMassGrams = 0;
        uint32_t m_detachedMassGrams = 0;

        uint32_t m_partitionReserveMassGrams = 0;
        uint32_t m_resolutionRequestReserveMassGrams = 0;
        uint32_t m_resolutionReceiptInputMassGrams = 0;

        uint64_t m_actualExplicitPieceMassGrams = 0;
        uint32_t m_reportedExplicitPieceMassGrams = 0;
        uint32_t m_fineReserveMassGrams = 0;

        int32_t m_actualExplicitPieceCount = 0;
        int32_t m_reportedExplicitPieceCount = 0;

        int64_t m_partitionResidualGrams = 0;
        int64_t m_requestHandoffResidualGrams = 0;
        int64_t m_receiptInputResidualGrams = 0;
        int64_t m_explicitPieceReceiptResidualGrams = 0;
        int64_t m_resolutionResidualGrams = 0;
        int64_t m_terminalResidualGrams = 0;

        bool m_partitionPass = false;
        bool m_requestHandoffPass = false;
        bool m_receiptInputPass = false;
        bool m_explicitPieceReceiptPass = false;
        bool m_explicitPieceCountPass = false;
        bool m_resolutionPass = false;
        bool m_terminalPass = false;

        bool m_pass = false;
    };

    EE_PROVENANCE_AUTHORITY_API GraniteTerminalConservationReceipt
    BuildGraniteTerminalConservationReceipt(
        GraniteRootedSeparationResult const& separation,
        GraniteSpallResolutionResult const&  spallResolution );

    //-------------------------------------------------------------------------
    // Named validators for the 32 existing certificate gates.
    //-------------------------------------------------------------------------

    EE_PROVENANCE_AUTHORITY_API void ValidateGraniteAuthority(
        GraniteFractureGeometricPartitionReceipt const& receipt,
        GraniteAuthorityVerificationReport&             report,
        uint32_t                                         caseID );

    EE_PROVENANCE_AUTHORITY_API void ValidateGraniteAuthority(
        GraniteFractureTransactionResult const& receipt,
        GraniteAuthorityVerificationReport&     report,
        uint32_t                                 caseID );

    EE_PROVENANCE_AUTHORITY_API void ValidateGraniteAuthority(
        GraniteDetachmentResult const&       receipt,
        GraniteAuthorityVerificationReport& report,
        uint32_t                             caseID );

    EE_PROVENANCE_AUTHORITY_API void ValidateGraniteAuthority(
        GraniteDetachmentSequenceReceipt const& receipt,
        GraniteAuthorityVerificationReport&     report,
        uint32_t                                 caseID );

    EE_PROVENANCE_AUTHORITY_API void ValidateGraniteAuthority(
        GraniteRootedMatterPartitionReceipt const& receipt,
        GraniteAuthorityVerificationReport&        report,
        uint32_t                                    caseID );

    EE_PROVENANCE_AUTHORITY_API void ValidateGraniteAuthority(
        GraniteRootedSeparationResult const& receipt,
        GraniteAuthorityVerificationReport& report,
        uint32_t                             caseID );

    EE_PROVENANCE_AUTHORITY_API void ValidateGraniteAuthority(
        GraniteSpallResolutionReceipt const& receipt,
        GraniteAuthorityVerificationReport& report,
        uint32_t                             caseID );

    EE_PROVENANCE_AUTHORITY_API void ValidateGraniteAuthority(
        GraniteTerminalConservationReceipt const& receipt,
        GraniteAuthorityVerificationReport&       report,
        uint32_t                                   caseID );

    // Runs the fixed Granite authority corpus and the known-negative controls.
    // The returned pass flag is suitable for mapping directly to process exit.
    EE_PROVENANCE_AUTHORITY_API GraniteAuthorityVerificationReport
    RunGraniteAuthorityVerificationCorpus();

#undef EE_PROVENANCE_AUTHORITY_API
}
