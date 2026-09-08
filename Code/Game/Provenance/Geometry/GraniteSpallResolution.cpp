#include "GraniteGeometry.h"
#include <cmath>

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
    // Authority verification must report named receipt failures and return a
    // process exit code. It must never block CI on an interactive breakpoint.
    #undef EE_ASSERT
    #define EE_ASSERT( cond ) do { } while ( 0 )
#endif

#include "GraniteGeometryInternal.h"

namespace EE
{
    using namespace GraniteGeometryInternal;

    //-------------------------------------------------------------------------
    // P3C.10C-1 — deterministic Granite spall-reserve resolution
    //
    // Matter authority:
    //
    //     input spall reserve grams
    //         =
    //     sum(explicit secondary-piece grams)
    //         +
    //     fine reserve grams
    //
    // exactly.
    //
    // This stage proposes secondary MatterBodies and preserves a fine reserve.
    // It does NOT create rendered dust particles and does NOT modify the parent
    // socket or principal detached-rock geometry.
    //-------------------------------------------------------------------------

    static int32_t ClampGraniteSpallPieceCount(
        int32_t value )
    {
        if ( value <
             0 )
        {
            return 0;
        }

        if ( value >
             GraniteSpallResolutionResult::s_maxPieces )
        {
            return GraniteSpallResolutionResult::s_maxPieces;
        }

        return value;
    }

    //-------------------------------------------------------------------------

    static GraniteSpallMatterClass ClassifyGraniteSpallMass(
        uint32_t                            massGrams,
        GraniteSpallResolutionPolicy const& policy )
    {
        uint32_t const minChipMassGrams =
            policy.m_minChipMassGrams;

        uint32_t const minCoarseMassGrams =
            GraniteMax(
                float(
                    policy.m_minCoarseSpallMassGrams ),
                float(
                    minChipMassGrams ) ) >
                    float(
                        policy.m_minCoarseSpallMassGrams )
                ? minChipMassGrams
                : policy.m_minCoarseSpallMassGrams;

        if ( massGrams >=
             minCoarseMassGrams )
        {
            return GraniteSpallMatterClass::CoarseSpall;
        }

        if ( massGrams >=
             minChipMassGrams )
        {
            return GraniteSpallMatterClass::Chip;
        }

        return GraniteSpallMatterClass::Grit;
    }

    //-------------------------------------------------------------------------

    static bool GraniteSpallBodyIDAlreadyUsed(
        GraniteSpallResolutionResult const& result,
        uint32_t                            bodyID )
    {
        if ( bodyID ==
             0 )
        {
            return true;
        }

        for ( int32_t pieceIndex = 0;
              pieceIndex <
              result.m_numPieces;
              ++pieceIndex )
        {
            if ( result.m_pieces[pieceIndex].m_bodyID ==
                 bodyID )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    static uint32_t BuildGraniteSpallBodyID(
        GraniteSpallResolutionResult const& result,
        uint32_t                            eventSeed,
        int32_t                             pieceIndex,
        uint32_t                            pieceMassGrams )
    {
        for ( int32_t attempt = 0;
              attempt <
              64;
              ++attempt )
        {
            uint32_t bodyID =
                HashGraniteValue(
                    eventSeed ^
                        0x6A09E667u ^
                        uint32_t(
                            attempt ) *
                            0x9E3779B9u,
                    pieceIndex +
                        attempt *
                            17,
                    int32_t(
                        pieceMassGrams ^
                        uint32_t(
                            pieceIndex *
                            131 ) ) );

            if ( bodyID ==
                 0 )
            {
                bodyID =
                    1u;
            }

            if ( !GraniteSpallBodyIDAlreadyUsed(
                     result,
                     bodyID ) )
            {
                return bodyID;
            }
        }

        // Deterministic emergency fallback. Collision probability above is
        // already negligible, but never emit an invalid/duplicate body ID.
        uint32_t bodyID =
            eventSeed ^
            0xA5A35625u ^
            uint32_t(
                pieceIndex +
                1 );

        if ( bodyID ==
             0 )
        {
            bodyID =
                uint32_t(
                    pieceIndex +
                    1 );
        }

        while ( GraniteSpallBodyIDAlreadyUsed(
            result,
            bodyID ) )
        {
            bodyID +=
                0x00010001u;

            if ( bodyID ==
                 0 )
            {
                bodyID =
                    1u;
            }
        }

        return bodyID;
    }

    //-------------------------------------------------------------------------

    static uint32_t BuildGraniteSpallProvenanceID(
        GraniteSpallResolutionRequest const& request,
        uint32_t                             eventSeed,
        uint32_t                             bodyID,
        int32_t                              pieceIndex )
    {
        uint32_t provenanceID =
            HashGraniteValue(
                eventSeed ^
                    request.m_parentProvenanceID ^
                    0xBB67AE85u,
                int32_t(
                    bodyID ),
                pieceIndex *
                        37 +
                    11 );

        if ( provenanceID ==
             0 )
        {
            provenanceID =
                bodyID ^
                0x3C6EF372u;

            if ( provenanceID ==
                 0 )
            {
                provenanceID =
                    1u;
            }
        }

        return provenanceID;
    }

    //-------------------------------------------------------------------------

    static void BuildGraniteSpallPlacementHint(
        GraniteSpallPieceDescriptor& piece,
        uint32_t                     eventSeed,
        int32_t                      pieceIndex )
    {
        float constexpr twoPi =
            6.28318530718f;

        float const angleSignal =
            HashGraniteUnit(
                eventSeed ^
                    0x510E527Fu,
                pieceIndex,
                17 );

        float const radiusSignal =
            HashGraniteUnit(
                eventSeed ^
                    0x243185BEu,
                pieceIndex,
                29 );

        float const liftSignal =
            HashGraniteUnit(
                eventSeed ^
                    0x1F83D9ABu,
                pieceIndex,
                43 );

        float const angle =
            angleSignal *
            twoPi;

        float const radiusM =
            GraniteLerp(
                0.06f,
                0.28f,
                radiusSignal );

        piece.m_localOffsetX =
            float(
                std::cos(
                    double(
                        angle ) ) ) *
            radiusM;

        piece.m_localOffsetY =
            float(
                std::sin(
                    double(
                        angle ) ) ) *
            radiusM;

        piece.m_localOffsetZ =
            GraniteLerp(
                0.015f,
                0.16f,
                liftSignal );

        float const azimuthSignal =
            HashGraniteUnit(
                eventSeed ^
                    0x5BE0CD19u,
                pieceIndex,
                59 );

        float const verticalSignal =
            HashGraniteUnit(
                eventSeed ^
                    0xA54FF53Au,
                pieceIndex,
                71 );

        float const biasAngle =
            azimuthSignal *
            twoPi;

        GranitePoint const bias =
            GraniteNormalize(
                GranitePoint{
                    float(
                        std::cos(
                            double(
                                biasAngle ) ) ),
                    float(
                        std::sin(
                            double(
                                biasAngle ) ) ),
                    GraniteLerp(
                        0.28f,
                        0.92f,
                        verticalSignal ) } );

        piece.m_biasX =
            bias.m_x;

        piece.m_biasY =
            bias.m_y;

        piece.m_biasZ =
            bias.m_z;
    }

    //-------------------------------------------------------------------------

    GraniteSpallResolutionResult ResolveGraniteSpallReserve(
        GraniteSpallResolutionRequest const& request )
    {
        GraniteSpallResolutionResult result;

        result.m_request =
            request;

        GraniteSpallResolutionReceipt& receipt =
            result.m_receipt;

        receipt.m_inputReserveMassGrams =
            request.m_spallReserveMassGrams;

        GraniteSpallResolutionPolicy const& policy =
            request.m_policy;

        int32_t const maximumExplicitPieces =
            ClampGraniteSpallPieceCount(
                policy.m_maxExplicitPieces );

        uint32_t const minimumExplicitPieceMassGrams =
            policy.m_minExplicitPieceMassGrams >
                    0
                ? policy.m_minExplicitPieceMassGrams
                : 1u;

        // A zero reserve is a valid no-op transaction. This makes the contract
        // composable even when a fracture produces no secondary debris.
        if ( request.m_spallReserveMassGrams ==
             0 )
        {
            receipt.m_explicitPieceMassGrams =
                0;

            receipt.m_fineReserveMassGrams =
                0;

            receipt.m_massResidualGrams =
                0;

            receipt.m_numExplicitPieces =
                0;

            receipt.m_uniqueBodyIDsPass =
                true;

            receipt.m_lineagePass =
                true;

            receipt.m_pieceMinimumMassPass =
                true;

            receipt.m_massConservationPass =
                true;

            receipt.m_pass =
                true;

            result.m_valid =
                true;

            return result;
        }

        uint32_t const eventSeed =
            MixGraniteSeed(
                request.m_worldSeed ^
                    request.m_sourcePartitionEventID,
                request.m_geologicalAncestryID,
                request.m_parentDetachedBodyID );

        float const minimumFineFraction =
            GraniteClamp01(
                GraniteMin(
                    request.m_policy.m_minFineReserveFraction,
                    request.m_policy.m_maxFineReserveFraction ) );

        float const maximumFineFraction =
            GraniteClamp01(
                GraniteMax(
                    request.m_policy.m_minFineReserveFraction,
                    request.m_policy.m_maxFineReserveFraction ) );

        float const fineFractionSignal =
            HashGraniteUnit(
                eventSeed ^
                    0x9B05688Cu,
                13,
                31 );

        float const fineFraction =
            GraniteLerp(
                minimumFineFraction,
                maximumFineFraction,
                fineFractionSignal );

        uint32_t fineReserveMassGrams =
            uint32_t(
                double(
                    request.m_spallReserveMassGrams ) *
                    double(
                        fineFraction ) +
                0.5 );

        if ( fineReserveMassGrams >
             request.m_spallReserveMassGrams )
        {
            fineReserveMassGrams =
                request.m_spallReserveMassGrams;
        }

        uint32_t explicitBudgetGrams =
            request.m_spallReserveMassGrams -
            fineReserveMassGrams;

        // If the candidate explicit budget cannot support even one valid child,
        // keep all reserve as fine authoritative matter.
        if ( maximumExplicitPieces <=
                 0 ||
             explicitBudgetGrams <
                 minimumExplicitPieceMassGrams )
        {
            receipt.m_explicitPieceMassGrams =
                0;

            receipt.m_fineReserveMassGrams =
                request.m_spallReserveMassGrams;

            receipt.m_massResidualGrams =
                0;

            receipt.m_numExplicitPieces =
                0;

            receipt.m_uniqueBodyIDsPass =
                true;

            receipt.m_lineagePass =
                request.m_parentDetachedBodyID !=
                0;

            receipt.m_pieceMinimumMassPass =
                true;

            receipt.m_massConservationPass =
                true;

            receipt.m_pass =
                receipt.m_lineagePass;

            result.m_valid =
                receipt.m_pass;

            return result;
        }

        int32_t const maximumPiecesByMass =
            int32_t(
                explicitBudgetGrams /
                minimumExplicitPieceMassGrams );

        int32_t pieceCount =
            maximumExplicitPieces;

        if ( pieceCount >
             maximumPiecesByMass )
        {
            pieceCount =
                maximumPiecesByMass;
        }

        // Prefer a few physically legible secondary fragments over turning the
        // whole reserve into the maximum number of minimum-mass grains.
        uint32_t const coarseScaleGrams =
            policy.m_minCoarseSpallMassGrams >
                    minimumExplicitPieceMassGrams
                ? policy.m_minCoarseSpallMassGrams
                : minimumExplicitPieceMassGrams *
                      4u;

        int32_t naturalPieceCount =
            1 +
            int32_t(
                explicitBudgetGrams /
                GraniteMax(
                    float(
                        coarseScaleGrams ),
                    1.0f ) );

        naturalPieceCount +=
            HashGraniteUnit(
                eventSeed ^
                    0xCBBB9D5Du,
                19,
                47 ) >
                    0.56f
                ? 1
                : 0;

        if ( naturalPieceCount <
             1 )
        {
            naturalPieceCount =
                1;
        }

        if ( pieceCount >
             naturalPieceCount )
        {
            pieceCount =
                naturalPieceCount;
        }

        if ( pieceCount <
             1 )
        {
            pieceCount =
                1;
        }

        uint64_t const requiredMinimumMass =
            uint64_t(
                minimumExplicitPieceMassGrams ) *
            uint64_t(
                pieceCount );

        EE_ASSERT(
            requiredMinimumMass <=
            uint64_t(
                explicitBudgetGrams ) );

        uint32_t const distributableGrams =
            explicitBudgetGrams -
            uint32_t(
                requiredMinimumMass );

        double weights[GraniteSpallResolutionResult::s_maxPieces];

        double fractionalRemainders[GraniteSpallResolutionResult::s_maxPieces];

        uint32_t extraMassGrams[GraniteSpallResolutionResult::s_maxPieces];

        double totalWeight =
            0.0;

        for ( int32_t pieceIndex = 0;
              pieceIndex <
              pieceCount;
              ++pieceIndex )
        {
            // Front-load some probability toward larger fragments while
            // preserving deterministic irregularity.
            float const randomWeight =
                HashGraniteUnit(
                    eventSeed ^
                        0x629A292Au,
                    pieceIndex,
                    83 );

            float const rankBias =
                pieceCount >
                        1
                    ? 1.0f -
                          float(
                              pieceIndex ) /
                              float(
                                  pieceCount -
                                  1 )
                    : 1.0f;

            double const weight =
                double(
                    GraniteLerp(
                        0.62f,
                        1.38f,
                        randomWeight ) *
                    GraniteLerp(
                        0.82f,
                        1.24f,
                        rankBias ) );

            weights[pieceIndex] =
                weight;

            totalWeight +=
                weight;

            fractionalRemainders[pieceIndex] =
                0.0;

            extraMassGrams[pieceIndex] =
                0;
        }

        uint32_t assignedExtraMassGrams =
            0;

        if ( distributableGrams >
             0 )
        {
            for ( int32_t pieceIndex = 0;
                  pieceIndex <
                  pieceCount;
                  ++pieceIndex )
            {
                double const exactExtraMass =
                    double(
                        distributableGrams ) *
                    weights[pieceIndex] /
                    totalWeight;

                uint32_t const floorExtraMass =
                    uint32_t(
                        std::floor(
                            exactExtraMass ) );

                extraMassGrams[pieceIndex] =
                    floorExtraMass;

                assignedExtraMassGrams +=
                    floorExtraMass;

                fractionalRemainders[pieceIndex] =
                    exactExtraMass -
                    double(
                        floorExtraMass );
            }

            uint32_t leftoverGrams =
                distributableGrams -
                assignedExtraMassGrams;

            // Largest-remainder correction. At most pieceCount-1 grams remain,
            // so exact integer closure requires no large per-gram loop.
            while ( leftoverGrams >
                    0 )
            {
                int32_t bestPieceIndex =
                    0;

                double bestRemainder =
                    -1.0;

                for ( int32_t pieceIndex = 0;
                      pieceIndex <
                      pieceCount;
                      ++pieceIndex )
                {
                    double const remainder =
                        fractionalRemainders[pieceIndex];

                    if ( remainder >
                             bestRemainder ||
                         ( remainder ==
                               bestRemainder &&
                           pieceIndex <
                               bestPieceIndex ) )
                    {
                        bestRemainder =
                            remainder;

                        bestPieceIndex =
                            pieceIndex;
                    }
                }

                ++extraMassGrams[bestPieceIndex];

                fractionalRemainders[bestPieceIndex] =
                    -1.0;

                --leftoverGrams;
            }
        }

        receipt.m_uniqueBodyIDsPass =
            true;

        receipt.m_lineagePass =
            request.m_parentDetachedBodyID !=
            0;

        receipt.m_pieceMinimumMassPass =
            true;

        uint64_t explicitMassSum =
            0;

        for ( int32_t pieceIndex = 0;
              pieceIndex <
              pieceCount;
              ++pieceIndex )
        {
            EE_ASSERT(
                result.m_numPieces <
                GraniteSpallResolutionResult::s_maxPieces );

            GraniteSpallPieceDescriptor& piece =
                result.m_pieces[result.m_numPieces];

            piece.m_valid =
                true;

            piece.m_parentBodyID =
                request.m_parentDetachedBodyID;

            piece.m_geologicalAncestryID =
                request.m_geologicalAncestryID;

            piece.m_massGrams =
                minimumExplicitPieceMassGrams +
                extraMassGrams[pieceIndex];

            piece.m_bodyID =
                BuildGraniteSpallBodyID(
                    result,
                    eventSeed,
                    pieceIndex,
                    piece.m_massGrams );

            piece.m_provenanceID =
                BuildGraniteSpallProvenanceID(
                    request,
                    eventSeed,
                    piece.m_bodyID,
                    pieceIndex );

            piece.m_class =
                ClassifyGraniteSpallMass(
                    piece.m_massGrams,
                    policy );

            BuildGraniteSpallPlacementHint(
                piece,
                eventSeed,
                pieceIndex );

            if ( piece.m_bodyID ==
                 0 )
            {
                receipt.m_uniqueBodyIDsPass =
                    false;
            }

            for ( int32_t previousPieceIndex = 0;
                  previousPieceIndex <
                  result.m_numPieces;
                  ++previousPieceIndex )
            {
                if ( result.m_pieces[previousPieceIndex].m_bodyID ==
                     piece.m_bodyID )
                {
                    receipt.m_uniqueBodyIDsPass =
                        false;
                }
            }

            if ( piece.m_parentBodyID !=
                     request.m_parentDetachedBodyID ||
                 piece.m_parentBodyID ==
                     0 ||
                 piece.m_geologicalAncestryID !=
                     request.m_geologicalAncestryID )
            {
                receipt.m_lineagePass =
                    false;
            }

            if ( piece.m_massGrams <
                 minimumExplicitPieceMassGrams )
            {
                receipt.m_pieceMinimumMassPass =
                    false;
            }

            explicitMassSum +=
                uint64_t(
                    piece.m_massGrams );

            switch ( piece.m_class )
            {
                case GraniteSpallMatterClass::CoarseSpall:
                    ++receipt.m_numCoarseSpalls;
                    break;

                case GraniteSpallMatterClass::Chip:
                    ++receipt.m_numChips;
                    break;

                case GraniteSpallMatterClass::Grit:
                default:
                    ++receipt.m_numGritPieces;
                    break;
            }

            ++result.m_numPieces;
        }

        EE_ASSERT(
            explicitMassSum <=
            uint64_t(
                request.m_spallReserveMassGrams ) );

        receipt.m_explicitPieceMassGrams =
            uint32_t(
                explicitMassSum );

        // Use subtraction from authoritative input for final fine reserve. This
        // guarantees exact integer closure even after all deterministic rounding.
        receipt.m_fineReserveMassGrams =
            request.m_spallReserveMassGrams -
            receipt.m_explicitPieceMassGrams;

        receipt.m_numExplicitPieces =
            result.m_numPieces;

        receipt.m_massResidualGrams =
            int64_t(
                receipt.m_explicitPieceMassGrams ) +
            int64_t(
                receipt.m_fineReserveMassGrams ) -
            int64_t(
                receipt.m_inputReserveMassGrams );

        int64_t const toleranceGrams =
            policy.m_massResidualToleranceGrams >=
                    0
                ? policy.m_massResidualToleranceGrams
                : -policy.m_massResidualToleranceGrams;

        receipt.m_massConservationPass =
            receipt.m_massResidualGrams >=
                -toleranceGrams &&
            receipt.m_massResidualGrams <=
                toleranceGrams;

        receipt.m_pass =
            receipt.m_uniqueBodyIDsPass &&
            receipt.m_lineagePass &&
            receipt.m_pieceMinimumMassPass &&
            receipt.m_massConservationPass;

        result.m_valid =
            receipt.m_pass;

        EE_ASSERT(
            receipt.m_massConservationPass );

        EE_ASSERT(
            receipt.m_uniqueBodyIDsPass );

        EE_ASSERT(
            receipt.m_pieceMinimumMassPass );

        return result;
    }

    //-------------------------------------------------------------------------

}
