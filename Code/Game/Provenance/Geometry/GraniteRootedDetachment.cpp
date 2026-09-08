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
    // P3C.10B — Rooted Granite structural detachment
    //
    // This layer does NOT decide whether a Granite form "looks like a boulder."
    // It tracks whether the rooted form still owns one or more finite
    // structural connections to the parent formation.
    //
    // Shape and structural ownership remain separate.
    //-------------------------------------------------------------------------

    static GraniteSurfaceStoneState GetGraniteBridgeSetSurfaceStoneState(
        GraniteStructuralBridgeSet const& bridgeSet )
    {
        if ( bridgeSet.m_numBridges <=
             0 )
        {
            return GraniteSurfaceStoneState::AttachedRockHead;
        }

        if ( bridgeSet.m_numIntactBridges ==
             bridgeSet.m_numBridges )
        {
            return GraniteSurfaceStoneState::AttachedRockHead;
        }

        if ( bridgeSet.m_numIntactBridges >
             0 )
        {
            return GraniteSurfaceStoneState::PartiallyDetachedBlock;
        }

        return GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate;
    }

    //-------------------------------------------------------------------------

    static void RecalculateGraniteStructuralBridgeSetReceipt(
        GraniteStructuralBridgeSet& bridgeSet )
    {
        double totalAttachmentArea =
            0.0;

        double remainingAttachmentArea =
            0.0;

        int32_t validCount =
            0;

        int32_t intactCount =
            0;

        int32_t severedCount =
            0;

        for ( int32_t bridgeIndex = 0;
              bridgeIndex <
              bridgeSet.m_numBridges;
              ++bridgeIndex )
        {
            GraniteStructuralBridgeDescriptor const& bridge =
                bridgeSet.m_bridges[bridgeIndex];

            if ( !bridge.m_valid )
            {
                continue;
            }

            ++validCount;

            totalAttachmentArea +=
                double(
                    GraniteMax(
                        bridge.m_attachmentAreaM2,
                        0.0f ) );

            if ( bridge.m_severed )
            {
                ++severedCount;
            }
            else
            {
                ++intactCount;

                remainingAttachmentArea +=
                    double(
                        GraniteMax(
                            bridge.m_attachmentAreaM2,
                            0.0f ) );
            }
        }

        bridgeSet.m_numBridges =
            validCount;

        bridgeSet.m_numIntactBridges =
            intactCount;

        bridgeSet.m_numSeveredBridges =
            severedCount;

        bridgeSet.m_initialAttachmentAreaM2 =
            float(
                totalAttachmentArea );

        bridgeSet.m_remainingAttachmentAreaM2 =
            float(
                remainingAttachmentArea );

        if ( totalAttachmentArea >
             0.0000001 )
        {
            bridgeSet.m_remainingConnectionFraction =
                GraniteClamp01(
                    float(
                        remainingAttachmentArea /
                        totalAttachmentArea ) );
        }
        else
        {
            bridgeSet.m_remainingConnectionFraction =
                intactCount >
                        0
                    ? 1.0f
                    : 0.0f;
        }

        bridgeSet.m_severedConnectionFraction =
            GraniteClamp01(
                1.0f -
                bridgeSet.m_remainingConnectionFraction );
    }

    //-------------------------------------------------------------------------

    static uint32_t BuildGraniteStructuralBridgeID(
        GraniteRootedBridgeSetRequest const& request,
        int32_t                              bridgeIndex )
    {
        uint32_t const salt =
            0x10B10000u ^
            request.m_structuralEventID ^
            request.m_rootedForm.m_eventID ^
            ( uint32_t(
                  bridgeIndex +
                  1 ) *
              0x9E3779B9u );

        int32_t const x =
            int32_t(
                request.m_rootedForm.m_eventID &
                0x7FFFFFFFu );

        int32_t const y =
            int32_t(
                request.m_rootedForm.m_grammarSalt &
                0x7FFFFFFFu );

        uint32_t bridgeID =
            HashGraniteValue(
                request.m_worldSeed ^
                    salt,
                x,
                y );

        if ( bridgeID ==
             0 )
        {
            bridgeID =
                uint32_t(
                    bridgeIndex +
                    1 );
        }

        return bridgeID;
    }

    //-------------------------------------------------------------------------

    static float SampleGraniteBridgeSignal(
        GraniteRootedBridgeSetRequest const& request,
        int32_t                              bridgeIndex,
        uint32_t                             salt )
    {
        int32_t const x =
            int32_t(
                request.m_rootedForm.m_eventID &
                0x7FFFFFFFu );

        int32_t const y =
            int32_t(
                (
                    request.m_rootedForm.m_grammarSalt ^
                    uint32_t(
                        bridgeIndex *
                        977 ) ) &
                0x7FFFFFFFu );

        return HashGraniteUnit(
            request.m_worldSeed ^
                request.m_structuralEventID ^
                salt,
            x,
            y );
    }

    //-------------------------------------------------------------------------

    static void GetGraniteStructuralBridgeLocalAxis(
        GraniteStructuralBridgeAxis axis,
        float                       orientationRadians,
        float&                      outAxisX,
        float&                      outAxisY,
        float&                      outCrossX,
        float&                      outCrossY,
        bool&                       outUsesMajorRadius )
    {
        float const majorX =
            float(
                std::cos(
                    double(
                        orientationRadians ) ) );

        float const majorY =
            float(
                std::sin(
                    double(
                        orientationRadians ) ) );

        float const minorX =
            -majorY;

        float const minorY =
            majorX;

        outAxisX =
            0.0f;

        outAxisY =
            0.0f;

        outCrossX =
            0.0f;

        outCrossY =
            0.0f;

        outUsesMajorRadius =
            true;

        switch ( axis )
        {
            case GraniteStructuralBridgeAxis::MajorPositive:
            {
                outAxisX =
                    majorX;

                outAxisY =
                    majorY;

                outCrossX =
                    minorX;

                outCrossY =
                    minorY;

                outUsesMajorRadius =
                    true;

                break;
            }

            case GraniteStructuralBridgeAxis::MajorNegative:
            {
                outAxisX =
                    -majorX;

                outAxisY =
                    -majorY;

                outCrossX =
                    minorX;

                outCrossY =
                    minorY;

                outUsesMajorRadius =
                    true;

                break;
            }

            case GraniteStructuralBridgeAxis::MinorPositive:
            {
                outAxisX =
                    minorX;

                outAxisY =
                    minorY;

                outCrossX =
                    majorX;

                outCrossY =
                    majorY;

                outUsesMajorRadius =
                    false;

                break;
            }

            case GraniteStructuralBridgeAxis::MinorNegative:
            {
                outAxisX =
                    -minorX;

                outAxisY =
                    -minorY;

                outCrossX =
                    majorX;

                outCrossY =
                    majorY;

                outUsesMajorRadius =
                    false;

                break;
            }

            default:
            {
                EE_ASSERT(
                    false );

                break;
            }
        }
    }

    //-------------------------------------------------------------------------

    GraniteStructuralBridgeSet BuildGraniteRootedBridgeSet(
        GraniteRootedBridgeSetRequest const& request )
    {
        GraniteStructuralBridgeSet bridgeSet;

        GraniteFormationFormDescriptor const& form =
            request.m_rootedForm;

        if ( !form.m_valid ||
             form.m_type !=
                 GraniteFormationFormType::RootedRockHead ||
             form.m_attachment !=
                 GraniteFormationAttachmentState::RootedRockHead ||
             form.m_majorRadiusM <=
                 0.0f ||
             form.m_minorRadiusM <=
                 0.0f ||
             form.m_maximumReliefM <=
                 0.0f ||
             form.m_rootBlendRadiusM <=
                 0.0f )
        {
            return bridgeSet;
        }

        bridgeSet.m_numBridges =
            GraniteStructuralBridgeSet::s_maxBridges;

        GraniteStructuralBridgeAxis const axes[GraniteStructuralBridgeSet::s_maxBridges] =
            {
                GraniteStructuralBridgeAxis::MajorPositive,
                GraniteStructuralBridgeAxis::MajorNegative,
                GraniteStructuralBridgeAxis::MinorPositive,
                GraniteStructuralBridgeAxis::MinorNegative };

        for ( int32_t bridgeIndex = 0;
              bridgeIndex <
              GraniteStructuralBridgeSet::s_maxBridges;
              ++bridgeIndex )
        {
            GraniteStructuralBridgeDescriptor& bridge =
                bridgeSet.m_bridges[bridgeIndex];

            GraniteStructuralBridgeAxis const axis =
                axes[bridgeIndex];

            float axisX =
                0.0f;

            float axisY =
                0.0f;

            float crossX =
                0.0f;

            float crossY =
                0.0f;

            bool usesMajorRadius =
                true;

            GetGraniteStructuralBridgeLocalAxis(
                axis,
                form.m_orientationRadians,
                axisX,
                axisY,
                crossX,
                crossY,
                usesMajorRadius );

            float const axialRadius =
                usesMajorRadius
                    ? form.m_majorRadiusM
                    : form.m_minorRadiusM;

            float const crossRadius =
                usesMajorRadius
                    ? form.m_minorRadiusM
                    : form.m_majorRadiusM;

            float const radialSignal =
                SampleGraniteBridgeSignal(
                    request,
                    bridgeIndex,
                    0xA511E9B3u );

            float const spanSignal =
                SampleGraniteBridgeSignal(
                    request,
                    bridgeIndex,
                    0x63D83595u );

            float const depthSignal =
                SampleGraniteBridgeSignal(
                    request,
                    bridgeIndex,
                    0xC2B2AE35u );

            float const areaSignal =
                SampleGraniteBridgeSignal(
                    request,
                    bridgeIndex,
                    0x27D4EB2Fu );

            float const lateralSignal =
                SampleGraniteBridgeSignal(
                    request,
                    bridgeIndex,
                    0x165667B1u );

            // The structural bridge is placed inside the continuous return
            // zone, not at the visible top of the rock head.
            float const radialDistance =
                axialRadius *
                    ( 0.56f +
                      radialSignal *
                          0.12f ) +
                form.m_rootBlendRadiusM *
                    ( 0.14f +
                      radialSignal *
                          0.10f );

            float const lateralOffset =
                ( lateralSignal -
                  0.5f ) *
                0.22f *
                GraniteMin(
                    crossRadius,
                    form.m_rootBlendRadiusM +
                        crossRadius *
                            0.25f );

            bridge.m_valid =
                true;

            bridge.m_bridgeID =
                BuildGraniteStructuralBridgeID(
                    request,
                    bridgeIndex );

            bridge.m_parentFormEventID =
                form.m_eventID;

            bridge.m_geologicalAncestryID =
                form.m_geologicalAncestryID;

            bridge.m_axis =
                axis;

            bridge.m_centerWorldX =
                form.m_centerWorldX +
                axisX *
                    radialDistance +
                crossX *
                    lateralOffset;

            bridge.m_centerWorldY =
                form.m_centerWorldY +
                axisY *
                    radialDistance +
                crossY *
                    lateralOffset;

            bridge.m_orientationRadians =
                float(
                    std::atan2(
                        double(
                            axisY ),
                        double(
                            axisX ) ) );

            // Bridge dimensions describe the root connection area only.
            // They deliberately do not estimate mass.
            bridge.m_spanM =
                GraniteMax(
                    0.025f,
                    crossRadius *
                            ( 0.17f +
                              spanSignal *
                                  0.11f ) +
                        form.m_rootBlendRadiusM *
                            ( 0.08f +
                              spanSignal *
                                  0.08f ) );

            bridge.m_rootDepthM =
                GraniteMax(
                    0.018f,
                    form.m_maximumReliefM *
                            ( 0.12f +
                              depthSignal *
                                  0.10f ) +
                        form.m_rootBlendRadiusM *
                            ( 0.06f +
                              depthSignal *
                                  0.07f ) );

            bridge.m_attachmentAreaM2 =
                bridge.m_spanM *
                bridge.m_rootDepthM *
                ( 0.78f +
                  areaSignal *
                      0.34f );

            bridge.m_dominantJointSetIndex =
                form.m_primaryJointSetIndex;

            bridge.m_severed =
                false;

            bridge.m_severingEventID =
                0;
        }

        RecalculateGraniteStructuralBridgeSetReceipt(
            bridgeSet );

        EE_ASSERT(
            bridgeSet.m_numBridges ==
            GraniteStructuralBridgeSet::s_maxBridges );

        EE_ASSERT(
            bridgeSet.m_numIntactBridges ==
            bridgeSet.m_numBridges );

        EE_ASSERT(
            bridgeSet.m_remainingConnectionFraction >
            0.9999f );

        return bridgeSet;
    }

    //-------------------------------------------------------------------------

    static GraniteSurfaceStoneDescriptor
    BuildGraniteDetachedCandidateFromBridgeSet(
        GraniteStructuralBridgeSet const& bridgeSet,
        uint32_t                          eventID )
    {
        GraniteSurfaceStoneDescriptor descriptor;

        if ( bridgeSet.m_numBridges <
             2 )
        {
            return descriptor;
        }

        int32_t majorPositiveIndex =
            -1;

        int32_t majorNegativeIndex =
            -1;

        int32_t minorPositiveIndex =
            -1;

        int32_t minorNegativeIndex =
            -1;

        float centerX =
            0.0f;

        float centerY =
            0.0f;

        float maximumRootDepth =
            0.0f;

        float meanSpan =
            0.0f;

        uint32_t geologicalAncestryID =
            0;

        uint32_t parentFormEventID =
            0;

        for ( int32_t bridgeIndex = 0;
              bridgeIndex <
              bridgeSet.m_numBridges;
              ++bridgeIndex )
        {
            GraniteStructuralBridgeDescriptor const& bridge =
                bridgeSet.m_bridges[bridgeIndex];

            if ( !bridge.m_valid )
            {
                continue;
            }

            centerX +=
                bridge.m_centerWorldX;

            centerY +=
                bridge.m_centerWorldY;

            maximumRootDepth =
                GraniteMax(
                    maximumRootDepth,
                    bridge.m_rootDepthM );

            meanSpan +=
                bridge.m_spanM;

            geologicalAncestryID =
                bridge.m_geologicalAncestryID;

            parentFormEventID =
                bridge.m_parentFormEventID;

            switch ( bridge.m_axis )
            {
                case GraniteStructuralBridgeAxis::MajorPositive:
                    majorPositiveIndex =
                        bridgeIndex;
                    break;

                case GraniteStructuralBridgeAxis::MajorNegative:
                    majorNegativeIndex =
                        bridgeIndex;
                    break;

                case GraniteStructuralBridgeAxis::MinorPositive:
                    minorPositiveIndex =
                        bridgeIndex;
                    break;

                case GraniteStructuralBridgeAxis::MinorNegative:
                    minorNegativeIndex =
                        bridgeIndex;
                    break;

                default:
                    break;
            }
        }

        float const inverseBridgeCount =
            1.0f /
            float(
                bridgeSet.m_numBridges );

        centerX *=
            inverseBridgeCount;

        centerY *=
            inverseBridgeCount;

        meanSpan *=
            inverseBridgeCount;

        float majorRadius =
            GraniteMax(
                meanSpan,
                0.05f );

        float minorRadius =
            GraniteMax(
                meanSpan *
                    0.82f,
                0.04f );

        float orientationRadians =
            0.0f;

        if ( majorPositiveIndex >=
                 0 &&
             majorNegativeIndex >=
                 0 )
        {
            GraniteStructuralBridgeDescriptor const& positive =
                bridgeSet.m_bridges[majorPositiveIndex];

            GraniteStructuralBridgeDescriptor const& negative =
                bridgeSet.m_bridges[majorNegativeIndex];

            float const dx =
                positive.m_centerWorldX -
                negative.m_centerWorldX;

            float const dy =
                positive.m_centerWorldY -
                negative.m_centerWorldY;

            majorRadius =
                GraniteMax(
                    0.05f,
                    0.5f *
                        float(
                            std::sqrt(
                                double(
                                    dx *
                                        dx +
                                    dy *
                                        dy ) ) ) );

            orientationRadians =
                float(
                    std::atan2(
                        double(
                            dy ),
                        double(
                            dx ) ) );
        }

        if ( minorPositiveIndex >=
                 0 &&
             minorNegativeIndex >=
                 0 )
        {
            GraniteStructuralBridgeDescriptor const& positive =
                bridgeSet.m_bridges[minorPositiveIndex];

            GraniteStructuralBridgeDescriptor const& negative =
                bridgeSet.m_bridges[minorNegativeIndex];

            float const dx =
                positive.m_centerWorldX -
                negative.m_centerWorldX;

            float const dy =
                positive.m_centerWorldY -
                negative.m_centerWorldY;

            minorRadius =
                GraniteMax(
                    0.04f,
                    0.5f *
                        float(
                            std::sqrt(
                                double(
                                    dx *
                                        dx +
                                    dy *
                                        dy ) ) ) );
        }

        descriptor.m_valid =
            true;

        descriptor.m_eventID =
            eventID !=
                    0
                ? eventID
                : parentFormEventID;

        descriptor.m_geologicalAncestryID =
            geologicalAncestryID;

        descriptor.m_bodyGrammarSalt =
            HashGraniteValue(
                eventID ^
                    parentFormEventID ^
                    0x10BDE7ACu,
                int32_t(
                    parentFormEventID &
                    0x7FFFFFFFu ),
                int32_t(
                    geologicalAncestryID &
                    0x7FFFFFFFu ) );

        descriptor.m_state =
            GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate;

        descriptor.m_centerWorldX =
            centerX;

        descriptor.m_centerWorldY =
            centerY;

        descriptor.m_orientationRadians =
            orientationRadians;

        // P3C.10B deliberately treats this as a geometry/state handoff
        // descriptor, not a mass receipt. The later matter-partition pass must
        // replace these structural-envelope estimates with exact detached-body
        // geometry/matter ownership.
        descriptor.m_majorRadiusM =
            majorRadius;

        descriptor.m_minorRadiusM =
            minorRadius;

        descriptor.m_maximumReliefM =
            GraniteMax(
                maximumRootDepth *
                    3.6f,
                meanSpan *
                    0.72f );

        descriptor.m_embeddingDepthM =
            0.0f;

        descriptor.m_weatheredExteriorWeight =
            0.82f;

        descriptor.m_jointIsolation =
            1.0f;

        descriptor.m_remainingConnection =
            0.0f;

        descriptor.m_downslopeBias =
            ( HashGraniteUnit(
                  descriptor.m_bodyGrammarSalt ^
                      0x68E31DA4u,
                  int32_t(
                      descriptor.m_eventID &
                      0x7FFFFFFFu ),
                  int32_t(
                      descriptor.m_geologicalAncestryID &
                      0x7FFFFFFFu ) ) -
              0.5f ) *
            2.0f;

        return descriptor;
    }

    //-------------------------------------------------------------------------

    GraniteToolStrikeResult ApplyGraniteToolStrike(
        GraniteToolStrikeRequest const& request )
    {
        GraniteToolStrikeResult result;

        result.m_strikeEventID = request.m_strikeEventID;
        result.m_priorStrikeCount = request.m_priorStrikeCount;

        result.m_requestPass =
            request.m_strikeEventID != 0 &&
            request.m_rootedForm.m_valid &&
            request.m_rootedForm.m_type ==
                GraniteFormationFormType::RootedRockHead &&
            request.m_rootedForm.m_attachment ==
                GraniteFormationAttachmentState::RootedRockHead &&
            request.m_before.m_numBridges > 0 &&
            request.m_before.m_numBridges <=
                GraniteStructuralBridgeSet::s_maxBridges &&
            request.m_toolTipRadiusM > 0.0f &&
            request.m_maxPenetrationM >= 0.0f &&
            request.m_normalizedStrikeEnergy >= 0.0f &&
            request.m_normalizedStrikeEnergy <= 1.0f &&
            request.m_toolHardness >= 0.0f &&
            request.m_toolHardness <= 1.0f;

        if ( !result.m_requestPass )
        {
            result.m_rejectionReason =
                GraniteToolStrikeRejectionReason::InvalidRequest;
            return result;
        }

        auto Normalize3 = []
        (
            float x,
            float y,
            float z,
            float& outX,
            float& outY,
            float& outZ
        ) -> bool
        {
            float const lengthSquared = x * x + y * y + z * z;

            if ( lengthSquared <= 0.00000001f )
            {
                return false;
            }

            float const inverseLength =
                1.0f /
                float( std::sqrt( double( lengthSquared ) ) );

            outX = x * inverseLength;
            outY = y * inverseLength;
            outZ = z * inverseLength;
            return true;
        };

        bool const normalValid = Normalize3
        (
            request.m_surfaceNormalX,
            request.m_surfaceNormalY,
            request.m_surfaceNormalZ,
            result.m_normalX,
            result.m_normalY,
            result.m_normalZ
        );

        float incomingX = 0.0f;
        float incomingY = 0.0f;
        float incomingZ = 0.0f;

        bool const incomingValid = Normalize3
        (
            request.m_incomingDirectionX,
            request.m_incomingDirectionY,
            request.m_incomingDirectionZ,
            incomingX,
            incomingY,
            incomingZ
        );

        if ( !normalValid || !incomingValid )
        {
            result.m_rejectionReason =
                GraniteToolStrikeRejectionReason::InvalidContactFrame;
            return result;
        }

        // Project the incoming direction onto the contact plane. A near-normal
        // hit has no useful projected direction, so select a deterministic
        // axis least parallel to N and project that instead.
        float const incomingNormalDot =
            incomingX * result.m_normalX +
            incomingY * result.m_normalY +
            incomingZ * result.m_normalZ;

        float tangentX = incomingX - incomingNormalDot * result.m_normalX;
        float tangentY = incomingY - incomingNormalDot * result.m_normalY;
        float tangentZ = incomingZ - incomingNormalDot * result.m_normalZ;

        if ( !Normalize3
             (
                 tangentX,
                 tangentY,
                 tangentZ,
                 result.m_tangentX,
                 result.m_tangentY,
                 result.m_tangentZ
             ) )
        {
            float const fallbackX = GraniteAbs( result.m_normalX ) < 0.75f
                ? 1.0f
                : 0.0f;
            float const fallbackY = fallbackX == 0.0f ? 1.0f : 0.0f;
            float const fallbackDot =
                fallbackX * result.m_normalX +
                fallbackY * result.m_normalY;

            tangentX = fallbackX - fallbackDot * result.m_normalX;
            tangentY = fallbackY - fallbackDot * result.m_normalY;
            tangentZ = -fallbackDot * result.m_normalZ;

            if ( !Normalize3
                 (
                     tangentX,
                     tangentY,
                     tangentZ,
                     result.m_tangentX,
                     result.m_tangentY,
                     result.m_tangentZ
                 ) )
            {
                result.m_rejectionReason =
                    GraniteToolStrikeRejectionReason::InvalidContactFrame;
                return result;
            }
        }

        result.m_bitangentX =
            result.m_normalY * result.m_tangentZ -
            result.m_normalZ * result.m_tangentY;
        result.m_bitangentY =
            result.m_normalZ * result.m_tangentX -
            result.m_normalX * result.m_tangentZ;
        result.m_bitangentZ =
            result.m_normalX * result.m_tangentY -
            result.m_normalY * result.m_tangentX;

        result.m_contactFramePass = Normalize3
        (
            result.m_bitangentX,
            result.m_bitangentY,
            result.m_bitangentZ,
            result.m_bitangentX,
            result.m_bitangentY,
            result.m_bitangentZ
        );

        if ( !result.m_contactFramePass )
        {
            result.m_rejectionReason =
                GraniteToolStrikeRejectionReason::InvalidContactFrame;
            return result;
        }

        // Granite requires a hard point and a committed strike. Weathered
        // exterior slightly enlarges the candidate influence, but does not
        // reduce or invent matter by itself.
        float const weatheringResponse =
            request.m_materialState ==
                    GraniteWeatheringState::WeatheredExposure
                ? 1.08f
                : 0.96f;

        result.m_materialResponsePass =
            request.m_toolHardness >= 0.72f &&
            request.m_normalizedStrikeEnergy >= 0.28f;

        if ( !result.m_materialResponsePass )
        {
            result.m_rejectionReason =
                GraniteToolStrikeRejectionReason::UnsupportedMaterial;
            return result;
        }

        float const energy =
            GraniteClamp01( request.m_normalizedStrikeEnergy );
        float const penetration =
            request.m_maxPenetrationM *
            ( 0.35f + 0.65f * energy );

        result.m_footprintMinorRadiusM =
            ( request.m_toolTipRadiusM + penetration ) *
            weatheringResponse;
        result.m_footprintMajorRadiusM =
            result.m_footprintMinorRadiusM *
            ( 1.35f + 0.45f * ( 1.0f - GraniteAbs( incomingNormalDot ) ) );
        result.m_influenceDepthM = penetration;

        bool foundIntactBridge = false;
        float bestNormalizedDistance = 1000000000.0f;
        float bestDistance = 0.0f;
        int32_t bestBridgeIndex = -1;

        for ( int32_t bridgeIndex = 0;
              bridgeIndex < request.m_before.m_numBridges;
              ++bridgeIndex )
        {
            GraniteStructuralBridgeDescriptor const& bridge =
                request.m_before.m_bridges[bridgeIndex];

            if ( !bridge.m_valid || bridge.m_severed )
            {
                continue;
            }

            foundIntactBridge = true;

            float const deltaX =
                bridge.m_centerWorldX - request.m_contactWorldX;
            float const deltaY =
                bridge.m_centerWorldY - request.m_contactWorldY;

            // Bridge descriptors are rooted-return XY authority. Project that
            // offset into the strike frame's horizontal T/B footprint.
            float const alongT =
                deltaX * result.m_tangentX +
                deltaY * result.m_tangentY;
            float const alongB =
                deltaX * result.m_bitangentX +
                deltaY * result.m_bitangentY;
            float const alongN =
                deltaX * result.m_normalX +
                deltaY * result.m_normalY;
            float const depthAllowance =
                result.m_influenceDepthM +
                GraniteMax
                (
                    request.m_toolTipRadiusM,
                    bridge.m_rootDepthM
                );

            float const normalizedDistanceSquared =
                alongT * alongT /
                    ( result.m_footprintMajorRadiusM *
                      result.m_footprintMajorRadiusM ) +
                alongB * alongB /
                    ( result.m_footprintMinorRadiusM *
                      result.m_footprintMinorRadiusM ) +
                alongN * alongN /
                    ( depthAllowance * depthAllowance );

            if ( normalizedDistanceSquared <= 1.0f &&
                 normalizedDistanceSquared < bestNormalizedDistance )
            {
                bestNormalizedDistance = normalizedDistanceSquared;
                bestDistance = float
                (
                    std::sqrt
                    (
                        double( deltaX * deltaX + deltaY * deltaY )
                    )
                );
                bestBridgeIndex = bridgeIndex;
            }
        }

        if ( !foundIntactBridge )
        {
            result.m_rejectionReason =
                GraniteToolStrikeRejectionReason::NoIntactStructuralBridge;
            return result;
        }

        if ( bestBridgeIndex < 0 )
        {
            result.m_rejectionReason =
                GraniteToolStrikeRejectionReason::NoBridgeInsideFootprint;
            return result;
        }

        result.m_selectedBridgeIndex = bestBridgeIndex;
        result.m_selectedBridgeMask =
            uint8_t( 1u << bestBridgeIndex );
        result.m_selectedBridgeDistanceM = bestDistance;
        result.m_selectedBridgeNormalizedDistance =
            float( std::sqrt( double( bestNormalizedDistance ) ) );
        result.m_footprintSelectionPass = true;

        GraniteDetachmentEventRequest detachmentRequest;
        detachmentRequest.m_worldSeed = request.m_worldSeed;
        detachmentRequest.m_eventID = request.m_strikeEventID;
        detachmentRequest.m_before = request.m_before;
        detachmentRequest.m_severBridgeMask =
            result.m_selectedBridgeMask;

        result.m_detachment =
            ApplyGraniteDetachmentEvent( detachmentRequest );
        result.m_structuralTransactionPass =
            result.m_detachment.m_pass &&
            result.m_detachment.m_effectiveSeverMask ==
                result.m_selectedBridgeMask;

        if ( !result.m_structuralTransactionPass )
        {
            result.m_rejectionReason =
                GraniteToolStrikeRejectionReason::DetachmentTransactionFailed;
            return result;
        }

        result.m_valid = true;
        result.m_accepted = true;
        result.m_pass =
            result.m_requestPass &&
            result.m_contactFramePass &&
            result.m_materialResponsePass &&
            result.m_footprintSelectionPass &&
            result.m_structuralTransactionPass;
        return result;
    }

    //-------------------------------------------------------------------------

    GraniteDetachmentResult ApplyGraniteDetachmentEvent(
        GraniteDetachmentEventRequest const& request )
    {
        GraniteDetachmentResult result;

        result.m_eventID =
            request.m_eventID;

        result.m_requestedSeverMask =
            request.m_severBridgeMask;

        result.m_before =
            request.m_before;

        RecalculateGraniteStructuralBridgeSetReceipt(
            result.m_before );

        result.m_after =
            result.m_before;

        if ( result.m_before.m_numBridges <=
             0 )
        {
            return result;
        }

        GraniteStructuralBridgeDescriptor const& firstBridge =
            result.m_before.m_bridges[0];

        result.m_parentFormEventID =
            firstBridge.m_parentFormEventID;

        result.m_geologicalAncestryID =
            firstBridge.m_geologicalAncestryID;

        result.m_stateBefore =
            GetGraniteBridgeSetSurfaceStoneState(
                result.m_before );

        double removedAttachmentArea =
            0.0;

        uint8_t effectiveMask =
            0;

        for ( int32_t bridgeIndex = 0;
              bridgeIndex <
              result.m_after.m_numBridges;
              ++bridgeIndex )
        {
            uint8_t const bridgeBit =
                uint8_t(
                    1u << bridgeIndex );

            if ( (
                     request.m_severBridgeMask &
                     bridgeBit ) ==
                 0 )
            {
                continue;
            }

            GraniteStructuralBridgeDescriptor& bridge =
                result.m_after.m_bridges[bridgeIndex];

            if ( !bridge.m_valid ||
                 bridge.m_severed )
            {
                continue;
            }

            bridge.m_severed =
                true;

            bridge.m_severingEventID =
                request.m_eventID;

            effectiveMask =
                uint8_t(
                    effectiveMask |
                    bridgeBit );

            removedAttachmentArea +=
                double(
                    GraniteMax(
                        bridge.m_attachmentAreaM2,
                        0.0f ) );
        }

        result.m_effectiveSeverMask =
            effectiveMask;

        RecalculateGraniteStructuralBridgeSetReceipt(
            result.m_after );

        result.m_stateAfter =
            GetGraniteBridgeSetSurfaceStoneState(
                result.m_after );

        result.m_attachmentAreaRemovedM2 =
            float(
                removedAttachmentArea );

        if ( result.m_stateBefore ==
                 GraniteSurfaceStoneState::AttachedRockHead &&
             result.m_stateAfter ==
                 GraniteSurfaceStoneState::PartiallyDetachedBlock )
        {
            result.m_transition =
                GraniteDetachmentTransitionKind::RootedToPartiallyDetached;
        }
        else if ( result.m_stateBefore ==
                      GraniteSurfaceStoneState::PartiallyDetachedBlock &&
                  result.m_stateAfter ==
                      GraniteSurfaceStoneState::PartiallyDetachedBlock )
        {
            result.m_transition =
                GraniteDetachmentTransitionKind::PartiallyDetachedProgression;
        }
        else if ( result.m_stateAfter ==
                      GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate &&
                  result.m_stateBefore !=
                      GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate )
        {
            // A single catastrophic event is allowed to sever the remaining
            // root bridges atomically. The same ownership rule applies:
            // detached is reached only when zero bridges remain.
            result.m_transition =
                GraniteDetachmentTransitionKind::PartiallyDetachedToDetached;
        }
        else
        {
            result.m_transition =
                GraniteDetachmentTransitionKind::None;
        }

        result.m_parentStructuralOwnershipRetained =
            result.m_after.m_numIntactBridges >
            0;

        result.m_detachedCandidateReady =
            result.m_after.m_numBridges >
                0 &&
            result.m_after.m_numIntactBridges ==
                0;

        if ( result.m_detachedCandidateReady )
        {
            result.m_detachedCandidate =
                BuildGraniteDetachedCandidateFromBridgeSet(
                    result.m_after,
                    request.m_eventID );
        }

        float const areaTolerance =
            GraniteMax(
                0.000001f,
                result.m_before.m_initialAttachmentAreaM2 *
                    0.00001f );

        float const expectedRemainingArea =
            GraniteMax(
                result.m_before.m_remainingAttachmentAreaM2 -
                    result.m_attachmentAreaRemovedM2,
                0.0f );

        result.m_monotonicConnectionPass =
            result.m_after.m_remainingConnectionFraction <=
                result.m_before.m_remainingConnectionFraction +
                    0.000001f &&
            result.m_after.m_remainingAttachmentAreaM2 <=
                result.m_before.m_remainingAttachmentAreaM2 +
                    areaTolerance &&
            result.m_after.m_numIntactBridges <=
                result.m_before.m_numIntactBridges;

        result.m_bridgeAccountingPass =
            GraniteAbs(
                result.m_after.m_initialAttachmentAreaM2 -
                result.m_before.m_initialAttachmentAreaM2 ) <=
                areaTolerance &&
            GraniteAbs(
                result.m_after.m_remainingAttachmentAreaM2 -
                expectedRemainingArea ) <=
                areaTolerance &&
            result.m_after.m_numIntactBridges +
                    result.m_after.m_numSeveredBridges ==
                result.m_after.m_numBridges;

        bool transitionValid =
            false;

        if ( effectiveMask ==
             0 )
        {
            transitionValid =
                result.m_stateAfter ==
                result.m_stateBefore;
        }
        else if ( result.m_stateAfter ==
                  GraniteSurfaceStoneState::AttachedRockHead )
        {
            transitionValid =
                false;
        }
        else if ( result.m_stateAfter ==
                  GraniteSurfaceStoneState::PartiallyDetachedBlock )
        {
            transitionValid =
                result.m_after.m_numIntactBridges >
                    0 &&
                result.m_after.m_numIntactBridges <
                    result.m_after.m_numBridges;
        }
        else if ( result.m_stateAfter ==
                  GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate )
        {
            transitionValid =
                result.m_after.m_numIntactBridges ==
                0;
        }

        result.m_stateTransitionPass =
            transitionValid;

        result.m_parentOwnershipPass =
            ( result.m_parentStructuralOwnershipRetained &&
              result.m_after.m_numIntactBridges >
                  0 &&
              !result.m_detachedCandidateReady ) ||
            ( !result.m_parentStructuralOwnershipRetained &&
              result.m_after.m_numIntactBridges ==
                  0 &&
              result.m_detachedCandidateReady &&
              result.m_detachedCandidate.m_valid &&
              result.m_detachedCandidate.m_state ==
                  GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate &&
              result.m_detachedCandidate.m_remainingConnection <=
                  0.000001f );

        result.m_valid =
            result.m_monotonicConnectionPass &&
            result.m_stateTransitionPass &&
            result.m_bridgeAccountingPass &&
            result.m_parentOwnershipPass;

        result.m_pass =
            result.m_valid;

        EE_ASSERT(
            result.m_monotonicConnectionPass );

        EE_ASSERT(
            result.m_stateTransitionPass );

        EE_ASSERT(
            result.m_bridgeAccountingPass );

        EE_ASSERT(
            result.m_parentOwnershipPass );

        return result;
    }

    //-------------------------------------------------------------------------

    GraniteDetachmentSequenceReceipt
    CertifyGraniteRootedDetachmentSequence(
        GraniteRootedBridgeSetRequest const& request )
    {
        GraniteDetachmentSequenceReceipt receipt;

        receipt.m_initialRooted =
            BuildGraniteRootedBridgeSet(
                request );

        receipt.m_initialRootedPass =
            receipt.m_initialRooted.m_numBridges >=
                3 &&
            receipt.m_initialRooted.m_numIntactBridges ==
                receipt.m_initialRooted.m_numBridges &&
            receipt.m_initialRooted.m_numSeveredBridges ==
                0 &&
            receipt.m_initialRooted.m_remainingConnectionFraction >
                0.9999f &&
            GetGraniteBridgeSetSurfaceStoneState(
                receipt.m_initialRooted ) ==
                GraniteSurfaceStoneState::AttachedRockHead;

        if ( !receipt.m_initialRootedPass )
        {
            return receipt;
        }

        // Deterministically sever two separated bridges first. This guarantees
        // a real partially-detached state while preserving at least one parent
        // structural connection.
        uint8_t partialMask =
            0;

        if ( receipt.m_initialRooted.m_numBridges >=
             4 )
        {
            partialMask =
                uint8_t(
                    ( 1u << 0 ) |
                    ( 1u << 2 ) );
        }
        else
        {
            partialMask =
                uint8_t(
                    1u << 0 );
        }

        GraniteDetachmentEventRequest partialRequest;

        partialRequest.m_worldSeed =
            request.m_worldSeed;

        partialRequest.m_eventID =
            request.m_structuralEventID ^
            0x10B000A1u;

        if ( partialRequest.m_eventID ==
             0 )
        {
            partialRequest.m_eventID =
                0x10B000A1u;
        }

        partialRequest.m_before =
            receipt.m_initialRooted;

        partialRequest.m_severBridgeMask =
            partialMask;

        receipt.m_partialEvent =
            ApplyGraniteDetachmentEvent(
                partialRequest );

        receipt.m_partialStillOwnedByParentPass =
            receipt.m_partialEvent.m_pass &&
            receipt.m_partialEvent.m_stateBefore ==
                GraniteSurfaceStoneState::AttachedRockHead &&
            receipt.m_partialEvent.m_stateAfter ==
                GraniteSurfaceStoneState::PartiallyDetachedBlock &&
            receipt.m_partialEvent.m_parentStructuralOwnershipRetained &&
            !receipt.m_partialEvent.m_detachedCandidateReady &&
            receipt.m_partialEvent.m_after.m_numIntactBridges >
                0 &&
            receipt.m_partialEvent.m_after.m_remainingConnectionFraction >
                0.0f &&
            receipt.m_partialEvent.m_after.m_remainingConnectionFraction <
                1.0f;

        if ( !receipt.m_partialStillOwnedByParentPass )
        {
            return receipt;
        }

        uint8_t finalMask =
            0;

        for ( int32_t bridgeIndex = 0;
              bridgeIndex <
              receipt.m_partialEvent.m_after.m_numBridges;
              ++bridgeIndex )
        {
            if ( !receipt.m_partialEvent.m_after.m_bridges[bridgeIndex].m_severed )
            {
                finalMask =
                    uint8_t(
                        finalMask |
                        uint8_t(
                            1u << bridgeIndex ) );
            }
        }

        GraniteDetachmentEventRequest finalRequest;

        finalRequest.m_worldSeed =
            request.m_worldSeed;

        finalRequest.m_eventID =
            request.m_structuralEventID ^
            0x10B000B2u;

        if ( finalRequest.m_eventID ==
             0 )
        {
            finalRequest.m_eventID =
                0x10B000B2u;
        }

        finalRequest.m_before =
            receipt.m_partialEvent.m_after;

        finalRequest.m_severBridgeMask =
            finalMask;

        receipt.m_finalEvent =
            ApplyGraniteDetachmentEvent(
                finalRequest );

        receipt.m_finalParentOwnershipReleasedPass =
            receipt.m_finalEvent.m_pass &&
            receipt.m_finalEvent.m_stateBefore ==
                GraniteSurfaceStoneState::PartiallyDetachedBlock &&
            receipt.m_finalEvent.m_stateAfter ==
                GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate &&
            !receipt.m_finalEvent.m_parentStructuralOwnershipRetained &&
            receipt.m_finalEvent.m_detachedCandidateReady &&
            receipt.m_finalEvent.m_detachedCandidate.m_valid &&
            receipt.m_finalEvent.m_after.m_numIntactBridges ==
                0 &&
            receipt.m_finalEvent.m_after.m_remainingConnectionFraction <=
                0.000001f;

        receipt.m_monotonicConnectionPass =
            receipt.m_initialRooted.m_remainingConnectionFraction >=
                receipt.m_partialEvent.m_after.m_remainingConnectionFraction &&
            receipt.m_partialEvent.m_after.m_remainingConnectionFraction >=
                receipt.m_finalEvent.m_after.m_remainingConnectionFraction &&
            receipt.m_initialRooted.m_remainingAttachmentAreaM2 >=
                receipt.m_partialEvent.m_after.m_remainingAttachmentAreaM2 &&
            receipt.m_partialEvent.m_after.m_remainingAttachmentAreaM2 >=
                receipt.m_finalEvent.m_after.m_remainingAttachmentAreaM2;

        receipt.m_valid =
            receipt.m_initialRootedPass &&
            receipt.m_partialStillOwnedByParentPass &&
            receipt.m_finalParentOwnershipReleasedPass &&
            receipt.m_monotonicConnectionPass;

        receipt.m_pass =
            receipt.m_valid;

        EE_ASSERT(
            receipt.m_initialRootedPass );

        EE_ASSERT(
            receipt.m_partialStillOwnedByParentPass );

        EE_ASSERT(
            receipt.m_finalParentOwnershipReleasedPass );

        EE_ASSERT(
            receipt.m_monotonicConnectionPass );

        return receipt;
    }
}
