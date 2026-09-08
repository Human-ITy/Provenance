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
    // P3C.10B-2 — Rooted-head separation / finite matter partition
    //
    // The detachable object is NOT the whole RootedRockHead + root apron.
    //
    // Instead:
    //
    //     pre-detachment local rooted parcel
    //         =
    //     detached head parcel
    //         UNION
    //     remaining parent socket parcel
    //
    // Both children are generated from the SAME sampled top surface and SAME
    // sampled separation/socket surface so their geometric union is explicit.
    //
    // The parent socket remains in the world. Only the thickness above the
    // certified separation surface becomes detached candidate matter.
    //-------------------------------------------------------------------------

    static constexpr int32_t s_rootedPartitionMaxSamplesPerAxis =
        31;

    static constexpr int32_t s_rootedPartitionMaxSamples =
        s_rootedPartitionMaxSamplesPerAxis *
        s_rootedPartitionMaxSamplesPerAxis;

    //-------------------------------------------------------------------------

    struct GraniteRootedPartitionGridSample
    {
        float m_worldX = 0.0f;
        float m_worldY = 0.0f;

        float m_topReliefM = 0.0f;

        // FIX6 baseline socket authority.
        //
        // This is the parent basin/apron geometry the user selected as the
        // correct surviving formation. FIX6 does not move or rebuild it.
        float m_socketReliefM = 0.0f;

        // Principal detached cast thickness AFTER mold/cast fitting.
        float m_detachedThicknessM = 0.0f;

        // FIX6 seam trim lifts only the DETACHED underside to meet the
        // preserved exterior dome at its peripheral boundary. The parent
        // socket remains unchanged. This lifted thickness is reclassified as
        // spall, so retained principal thickness is detached - undersideLift.
        float m_detachedUndersideLiftM = 0.0f;

        // Exact pre-fit FIX2 detached thickness. This remains the authority for
        // "what left parent bedrock" before the principal-body/spall split.
        float m_originalDetachedThicknessM = 0.0f;

        // Material removed from the principal cast because it forms thin
        // fringe/ledge/spall geometry. It is conserved separately and is NOT
        // returned to the socket/apron.
        float m_spallThicknessM = 0.0f;

        float m_rootBlendInfluence = 0.0f;
        float m_bridgeRetentionInfluence = 0.0f;
    };

    //-------------------------------------------------------------------------

    static float GetGraniteRootedRetainedPrincipalThickness(
        GraniteRootedPartitionGridSample const& sample )
    {
        return GraniteMax(
            sample.m_detachedThicknessM -
                sample.m_detachedUndersideLiftM,
            0.0f );
    }

    //-------------------------------------------------------------------------

    static void GetGraniteRootedPartitionWorldBounds(
        GraniteFormationFormDescriptor const& form,
        float&                                outMinWorldX,
        float&                                outMinWorldY,
        float&                                outMaxWorldX,
        float&                                outMaxWorldY )
    {
        float const safeMajor =
            GraniteMax(
                form.m_majorRadiusM,
                0.10f );

        float const safeMinor =
            GraniteMax(
                form.m_minorRadiusM,
                0.10f );

        float const outerMajor =
            safeMajor +
            GraniteMax(
                form.m_rootBlendRadiusM,
                0.05f );

        float const outerMinor =
            safeMinor +
            GraniteMax(
                form.m_rootBlendRadiusM,
                0.05f );

        float const cosine =
            float(
                std::cos(
                    double(
                        form.m_orientationRadians ) ) );

        float const sine =
            float(
                std::sin(
                    double(
                        form.m_orientationRadians ) ) );

        // Axis-aligned bounding half extents of the rotated outer ellipse.
        float const halfX =
            float(
                std::sqrt(
                    double(
                        outerMajor *
                            outerMajor *
                            cosine *
                            cosine +
                        outerMinor *
                            outerMinor *
                            sine *
                            sine ) ) );

        float const halfY =
            float(
                std::sqrt(
                    double(
                        outerMajor *
                            outerMajor *
                            sine *
                            sine +
                        outerMinor *
                            outerMinor *
                            cosine *
                            cosine ) ) );

        // Margin guarantees the rectangular carrier boundary is outside the
        // formation's positive relief support. We then also hard-zero boundary
        // samples so top/socket surfaces meet the local parent carrier exactly.
        float const margin =
            GraniteMax(
                0.12f,
                form.m_rootBlendRadiusM *
                    0.24f );

        outMinWorldX =
            form.m_centerWorldX -
            halfX -
            margin;

        outMaxWorldX =
            form.m_centerWorldX +
            halfX +
            margin;

        outMinWorldY =
            form.m_centerWorldY -
            halfY -
            margin;

        outMaxWorldY =
            form.m_centerWorldY +
            halfY +
            margin;
    }

    //-------------------------------------------------------------------------

    static int32_t ResolveGraniteRootedPartitionSamplesPerAxis(
        float spanM,
        float targetSpacingM )
    {
        float const safeTargetSpacing =
            GraniteMax(
                targetSpacingM,
                0.01f );

        int32_t samples =
            int32_t(
                std::ceil(
                    double(
                        spanM /
                        safeTargetSpacing ) ) ) +
            1;

        if ( samples <
             9 )
        {
            samples =
                9;
        }

        if ( samples >
             s_rootedPartitionMaxSamplesPerAxis )
        {
            samples =
                s_rootedPartitionMaxSamplesPerAxis;
        }

        // Odd sample count guarantees one deterministic center row/column.
        if ( (
                 samples &
                 1 ) ==
             0 )
        {
            --samples;
        }

        return samples;
    }

    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // P3C.10B-2 FIX2 — detachable core influence
    //
    // RootedRockHead contains:
    //
    //     protruding rock-head core
    //     +
    //     root-blend/apron that merges into parent Granite
    //
    // The apron is parent formation and must never leave with the detached
    // head. This mask therefore uses the descriptor's CORE major/minor radii
    // only. Detachment tapers to zero inside the core edge, before the apron.
    //-------------------------------------------------------------------------

    static float EvaluateGraniteDetachableCoreInfluence(
        GraniteFormationFormDescriptor const& form,
        float                                 worldX,
        float                                 worldY )
    {
        float along =
            0.0f;

        float across =
            0.0f;

        GetGraniteFormationLocalCoordinates(
            form,
            worldX,
            worldY,
            along,
            across );

        float const safeMajor =
            GraniteMax(
                form.m_majorRadiusM,
                0.10f );

        float const safeMinor =
            GraniteMax(
                form.m_minorRadiusM,
                0.10f );

        float const coreRadius =
            GraniteSuperellipseRadius(
                along,
                across,
                safeMajor,
                safeMinor,
                2.55f );

        float constexpr fullDetachRadius =
            0.78f;

        float constexpr zeroDetachRadius =
            0.98f;

        if ( coreRadius <=
             fullDetachRadius )
        {
            return 1.0f;
        }

        if ( coreRadius >=
             zeroDetachRadius )
        {
            return 0.0f;
        }

        return 1.0f -
               GraniteSmoothStep01(
                   (
                       coreRadius -
                       fullDetachRadius ) /
                   ( zeroDetachRadius -
                     fullDetachRadius ) );
    }

    //-------------------------------------------------------------------------

    static float EvaluateGraniteRootBlendRetentionInfluence(
        GraniteFormationFormDescriptor const& form,
        float                                 worldX,
        float                                 worldY )
    {
        float along =
            0.0f;

        float across =
            0.0f;

        GetGraniteFormationLocalCoordinates(
            form,
            worldX,
            worldY,
            along,
            across );

        float const safeMajor =
            GraniteMax(
                form.m_majorRadiusM,
                0.10f );

        float const safeMinor =
            GraniteMax(
                form.m_minorRadiusM,
                0.10f );

        float const outerMajor =
            GraniteMax(
                safeMajor +
                    form.m_rootBlendRadiusM,
                safeMajor +
                    0.05f );

        float const outerMinor =
            GraniteMax(
                safeMinor +
                    form.m_rootBlendRadiusM,
                safeMinor +
                    0.05f );

        float const outerRadius =
            GraniteSuperellipseRadius(
                along,
                across,
                outerMajor,
                outerMinor,
                2.55f );

        float const coreFraction =
            GraniteMax(
                GraniteMin(
                    safeMajor /
                        outerMajor,
                    safeMinor /
                        outerMinor ),
                0.20f );

        return GraniteSmoothStep01(
            (
                outerRadius -
                coreFraction *
                    0.82f ) /
            GraniteMax(
                1.0f -
                    coreFraction *
                        0.82f,
                0.05f ) );
    }

    //-------------------------------------------------------------------------

    static float EvaluateGraniteBridgeRetentionInfluence(
        GraniteDetachmentResult const&       finalDetachment,
        GraniteRootedSeparationPolicy const& policy,
        float                                worldX,
        float                                worldY )
    {
        float strongestInfluence =
            0.0f;

        GraniteStructuralBridgeSet const& bridgeSet =
            finalDetachment.m_after;

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

            float const dx =
                worldX -
                bridge.m_centerWorldX;

            float const dy =
                worldY -
                bridge.m_centerWorldY;

            float const distanceM =
                float(
                    std::sqrt(
                        double(
                            dx *
                                dx +
                            dy *
                                dy ) ) );

            float const influenceRadiusM =
                GraniteMax(
                    GraniteMax(
                        bridge.m_spanM,
                        bridge.m_rootDepthM ) *
                        GraniteMax(
                            policy.m_bridgeRetentionRadiusScale,
                            0.25f ),
                    0.05f );

            float const localInfluence =
                1.0f -
                GraniteSmoothStep01(
                    distanceM /
                    influenceRadiusM );

            strongestInfluence =
                GraniteMax(
                    strongestInfluence,
                    localInfluence );
        }

        return GraniteClamp01(
            strongestInfluence );
    }

    //-------------------------------------------------------------------------

    static GraniteRootedPartitionGridSample
    EvaluateGraniteRootedPartitionSample(
        GraniteRootedSeparationRequest const& request,
        float                                 worldX,
        float                                 worldY,
        bool                                  forceBoundaryZero )
    {
        GraniteRootedPartitionGridSample sample;

        sample.m_worldX =
            worldX;

        sample.m_worldY =
            worldY;

        if ( forceBoundaryZero )
        {
            return sample;
        }

        sample.m_topReliefM =
            GraniteMax(
                EvaluateGraniteFormationFormRelief(
                    request.m_rootedForm,
                    worldX,
                    worldY ),
                0.0f );

        if ( sample.m_topReliefM <=
             0.000001f )
        {
            return sample;
        }

        sample.m_rootBlendInfluence =
            EvaluateGraniteRootBlendRetentionInfluence(
                request.m_rootedForm,
                worldX,
                worldY );

        sample.m_bridgeRetentionInfluence =
            EvaluateGraniteBridgeRetentionInfluence(
                request.m_finalDetachment,
                request.m_policy,
                worldX,
                worldY );

        float const detachableCoreInfluence =
            EvaluateGraniteDetachableCoreInfluence(
                request.m_rootedForm,
                worldX,
                worldY );

        float const minSocketFraction =
            GraniteClamp01(
                request.m_policy.m_minSocketReliefFraction );

        float const maxSocketFraction =
            GraniteMax(
                minSocketFraction,
                GraniteClamp01(
                    request.m_policy.m_maxSocketReliefFraction ) );

        float const crownSignal =
            GraniteClamp01(
                sample.m_topReliefM /
                GraniteMax(
                    request.m_rootedForm.m_maximumReliefM,
                    0.001f ) );

        // Interior socket is shallowest beneath the highest coherent crown and
        // deeper beneath lower/shoulder parts of the rooted form.
        float interiorSocketFraction =
            GraniteLerp(
                maxSocketFraction,
                minSocketFraction,
                GraniteSmoothStep01(
                    crownSignal ) );

        interiorSocketFraction +=
            sample.m_bridgeRetentionInfluence *
            GraniteMax(
                request.m_policy.m_bridgeRetentionStrength,
                0.0f );

        interiorSocketFraction =
            GraniteClamp01(
                interiorSocketFraction );

        float const interiorDetachedThicknessM =
            sample.m_topReliefM *
            ( 1.0f -
              interiorSocketFraction );

        //-------------------------------------------------------------------------
        // Critical ownership rule:
        //
        // Only the protruding head CORE is detachable.
        //
        // The RootedRockHead root-blend/apron remains entirely with parent
        // Granite. Outside the core envelope:
        //
        //     detachedThickness = 0
        //     socketRelief      = original topRelief
        //
        // Therefore the apron remains in place when the head is lifted.
        //-------------------------------------------------------------------------

        sample.m_detachedThicknessM =
            GraniteMax(
                interiorDetachedThicknessM *
                    detachableCoreInfluence,
                0.0f );

        sample.m_socketReliefM =
            GraniteMax(
                sample.m_topReliefM -
                    sample.m_detachedThicknessM,
                0.0f );

        // Freeze the user-approved FIX2 split before principal-body fitting.
        sample.m_originalDetachedThicknessM =
            sample.m_detachedThicknessM;

        sample.m_spallThicknessM =
            0.0f;

        EE_ASSERT(
            GraniteAbs(
                sample.m_topReliefM -
                ( sample.m_socketReliefM +
                  sample.m_originalDetachedThicknessM ) ) <=
            0.000002f );

        return sample;
    }

    //-------------------------------------------------------------------------
    // P3C.10B-2 FIX6 — mold/cast principal-body fitting
    //
    // Baseline ownership is NOT changed:
    //
    //     original top
    //         =
    //     surviving parent socket
    //         +
    //     original FIX2 detached thickness
    //
    // The ugly fins are a principal-body classification problem, not a parent
    // socket problem.
    //
    // FIX6 splits the ORIGINAL detached thickness into:
    //
    //     principal cast thickness
    //         +
    //     explicit spall-reserve thickness
    //
    // while the parent socket remains byte-for-byte derived from the attached
    // FIX2 field.
    //
    // The principal cast is fitted by thickness/coherence. Its underside is
    // exactly the socket surface. Its new fracture-side crown tapers into that
    // socket before thin fringe sheets can become part of the main boulder.
    //
    // Any rejected Granite becomes m_spallThicknessM. It is conserved and can
    // later become P3C.10C chips/spalls.
    //-------------------------------------------------------------------------

    struct GraniteRootedCastFitReceipt
    {
        float m_maxOriginalDetachedThicknessM = 0.0f;
        float m_lowThicknessM = 0.0f;
        float m_fullThicknessM = 0.0f;

        float m_minPositivePrincipalThicknessM = 0.0f;
        float m_maxPrincipalThicknessM = 0.0f;

        int32_t m_principalSampleCount = 0;
        int32_t m_spallSampleCount = 0;

        bool m_principalConnected = false;
        bool m_valid = false;
    };

    //-------------------------------------------------------------------------

    static float GetGraniteRootedGridThicknessFieldValue(
        GraniteRootedPartitionGridSample const& sample,
        int32_t                                 fieldKind )
    {
        // 0 = original detached
        // 1 = principal detached
        // 2 = spall reserve
        switch ( fieldKind )
        {
            case 0:
                return sample.m_originalDetachedThicknessM;

            case 1:
                return GetGraniteRootedRetainedPrincipalThickness(
                    sample );

            case 2:
                return sample.m_spallThicknessM;

            default:
                EE_ASSERT( false );
                return 0.0f;
        }
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteRootedThicknessFieldVolume(
        GraniteRootedPartitionGridSample const* pSamples,
        int32_t                                 samplesX,
        int32_t                                 samplesY,
        float                                   spacingX,
        float                                   spacingY,
        int32_t                                 fieldKind )
    {
        EE_ASSERT(
            pSamples !=
            nullptr );

        double volume =
            0.0;

        float const halfCellArea =
            spacingX *
            spacingY *
            0.5f;

        for ( int32_t y = 0;
              y <
              samplesY -
                  1;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  samplesX -
                      1;
                  ++x )
            {
                int32_t const i00 =
                    y *
                        samplesX +
                    x;

                int32_t const i10 =
                    y *
                        samplesX +
                    x +
                    1;

                int32_t const i01 =
                    ( y +
                      1 ) *
                        samplesX +
                    x;

                int32_t const i11 =
                    ( y +
                      1 ) *
                        samplesX +
                    x +
                    1;

                float const h00 =
                    GetGraniteRootedGridThicknessFieldValue(
                        pSamples[i00],
                        fieldKind );

                float const h10 =
                    GetGraniteRootedGridThicknessFieldValue(
                        pSamples[i10],
                        fieldKind );

                float const h01 =
                    GetGraniteRootedGridThicknessFieldValue(
                        pSamples[i01],
                        fieldKind );

                float const h11 =
                    GetGraniteRootedGridThicknessFieldValue(
                        pSamples[i11],
                        fieldKind );

                // Exact integral for the SAME fixed i00->i11 diagonal used by
                // the attached shell builder.
                float const firstTriangleMean =
                    ( h00 +
                      h10 +
                      h11 ) /
                    3.0f;

                float const secondTriangleMean =
                    ( h00 +
                      h11 +
                      h01 ) /
                    3.0f;

                volume +=
                    double(
                        (
                            firstTriangleMean +
                            secondTriangleMean ) *
                        halfCellArea );
            }
        }

        return float(
            volume );
    }

    //-------------------------------------------------------------------------

    static bool IsGraniteRootedPrincipalCastConnected(
        GraniteRootedPartitionGridSample const* pSamples,
        int32_t                                 samplesX,
        int32_t                                 samplesY )
    {
        int32_t const sampleCount =
            samplesX *
            samplesY;

        if ( sampleCount <=
                 0 ||
             sampleCount >
                 s_rootedPartitionMaxSamples )
        {
            return false;
        }

        int32_t seedIndex =
            -1;

        float maximumThickness =
            0.0f;

        int32_t positiveCount =
            0;

        for ( int32_t sampleIndex = 0;
              sampleIndex <
              sampleCount;
              ++sampleIndex )
        {
            float const thickness =
                GetGraniteRootedRetainedPrincipalThickness(
                    pSamples[sampleIndex] );

            if ( thickness <=
                 0.000001f )
            {
                continue;
            }

            ++positiveCount;

            if ( thickness >
                 maximumThickness )
            {
                maximumThickness =
                    thickness;

                seedIndex =
                    sampleIndex;
            }
        }

        if ( seedIndex <
                 0 ||
             positiveCount <=
                 0 )
        {
            return false;
        }

        bool visited[s_rootedPartitionMaxSamples] =
            {
                false };

        int32_t queue[s_rootedPartitionMaxSamples];

        int32_t readIndex =
            0;

        int32_t writeIndex =
            0;

        visited[seedIndex] =
            true;

        queue[writeIndex++] =
            seedIndex;

        int32_t visitedPositiveCount =
            0;

        int32_t const neighborDX[4] =
            {
                -1,
                1,
                0,
                0 };

        int32_t const neighborDY[4] =
            {
                0,
                0,
                -1,
                1 };

        while ( readIndex <
                writeIndex )
        {
            int32_t const currentIndex =
                queue[readIndex++];

            ++visitedPositiveCount;

            int32_t const currentX =
                currentIndex %
                samplesX;

            int32_t const currentY =
                currentIndex /
                samplesX;

            for ( int32_t neighborIndex = 0;
                  neighborIndex <
                  4;
                  ++neighborIndex )
            {
                int32_t const x =
                    currentX +
                    neighborDX[neighborIndex];

                int32_t const y =
                    currentY +
                    neighborDY[neighborIndex];

                if ( x <
                         0 ||
                     x >=
                         samplesX ||
                     y <
                         0 ||
                     y >=
                         samplesY )
                {
                    continue;
                }

                int32_t const sampleIndex =
                    y *
                        samplesX +
                    x;

                if ( visited[sampleIndex] ||
                     GetGraniteRootedRetainedPrincipalThickness(
                         pSamples[sampleIndex] ) <=
                         0.000001f )
                {
                    continue;
                }

                visited[sampleIndex] =
                    true;

                queue[writeIndex++] =
                    sampleIndex;
            }
        }

        return visitedPositiveCount ==
               positiveCount;
    }

    //-------------------------------------------------------------------------
    // Remove the one-sample positive-to-zero fringe that renders as hanging
    // fins/skirts around a lifted head. The exterior top sample is preserved,
    // but the detached underside rises to meet it at the seam. Removed solid
    // thickness becomes spall; the parent socket field is never modified.
    //-------------------------------------------------------------------------

    static bool IsGraniteRootedPrincipalCurtainBoundarySample(
        GraniteRootedPartitionGridSample const* pSamples,
        int32_t                                 samplesX,
        int32_t                                 samplesY,
        int32_t                                 x,
        int32_t                                 y )
    {
        int32_t const sampleIndex =
            y *
                samplesX +
            x;

        if ( pSamples[sampleIndex].m_detachedThicknessM <=
             0.000001f )
        {
            return false;
        }

        for ( int32_t offsetY = -1;
              offsetY <=
              1;
              ++offsetY )
        {
            for ( int32_t offsetX = -1;
                  offsetX <=
                  1;
                  ++offsetX )
            {
                if ( offsetX ==
                         0 &&
                     offsetY ==
                         0 )
                {
                    continue;
                }

                int32_t const neighborX =
                    x +
                    offsetX;

                int32_t const neighborY =
                    y +
                    offsetY;

                if ( neighborX <
                         0 ||
                     neighborX >=
                         samplesX ||
                     neighborY <
                         0 ||
                     neighborY >=
                         samplesY )
                {
                    return true;
                }

                int32_t const neighborIndex =
                    neighborY *
                        samplesX +
                    neighborX;

                if ( pSamples[neighborIndex].m_detachedThicknessM <=
                     0.000001f )
                {
                    return true;
                }
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    static void TrimGraniteRootedPrincipalCastSeam(
        GraniteRootedPartitionGridSample* pSamples,
        int32_t                           samplesX,
        int32_t                           samplesY )
    {
        bool trimSample[s_rootedPartitionMaxSamples] =
            {
                false };

        for ( int32_t y = 0;
              y <
              samplesY;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  samplesX;
                  ++x )
            {
                int32_t const sampleIndex =
                    y *
                        samplesX +
                    x;

                trimSample[sampleIndex] =
                    IsGraniteRootedPrincipalCurtainBoundarySample(
                        pSamples,
                        samplesX,
                        samplesY,
                        x,
                        y );
            }
        }

        int32_t const sampleCount =
            samplesX *
            samplesY;

        for ( int32_t sampleIndex = 0;
              sampleIndex <
              sampleCount;
              ++sampleIndex )
        {
            if ( !trimSample[sampleIndex] )
            {
                continue;
            }

            GraniteRootedPartitionGridSample& sample =
                pSamples[sampleIndex];

            sample.m_detachedUndersideLiftM =
                sample.m_detachedThicknessM;

            sample.m_spallThicknessM +=
                sample.m_detachedUndersideLiftM;

            EE_ASSERT(
                GraniteAbs(
                    sample.m_originalDetachedThicknessM -
                    ( GetGraniteRootedRetainedPrincipalThickness(
                          sample ) +
                      sample.m_spallThicknessM ) ) <=
                0.000002f );
        }
    }

    //-------------------------------------------------------------------------

    static GraniteRootedCastFitReceipt
    FitGraniteRootedPrincipalCastToSocket(
        GraniteRootedPartitionGridSample* pSamples,
        int32_t                           samplesX,
        int32_t                           samplesY )
    {
        GraniteRootedCastFitReceipt receipt;

        int32_t const sampleCount =
            samplesX *
            samplesY;

        if ( pSamples ==
                 nullptr ||
             sampleCount <=
                 0 ||
             sampleCount >
                 s_rootedPartitionMaxSamples )
        {
            return receipt;
        }

        float maximumOriginalThickness =
            0.0f;

        for ( int32_t sampleIndex = 0;
              sampleIndex <
              sampleCount;
              ++sampleIndex )
        {
            maximumOriginalThickness =
                GraniteMax(
                    maximumOriginalThickness,
                    pSamples[sampleIndex].m_originalDetachedThicknessM );
        }

        receipt.m_maxOriginalDetachedThicknessM =
            maximumOriginalThickness;

        if ( maximumOriginalThickness <=
             0.00001f )
        {
            return receipt;
        }

        // Thin sheets below this band are fracture fringe/spall reserve.
        // The values scale with the actual selected head rather than using a
        // universal hard-coded boulder thickness.
        float const lowThicknessM =
            GraniteMax(
                0.020f,
                maximumOriginalThickness *
                    0.11f );

        float const fullThicknessM =
            GraniteMax(
                lowThicknessM +
                    0.018f,
                maximumOriginalThickness *
                    0.30f );

        receipt.m_lowThicknessM =
            lowThicknessM;

        receipt.m_fullThicknessM =
            fullThicknessM;

        float rawCastWeight[s_rootedPartitionMaxSamples];

        float currentWeight[s_rootedPartitionMaxSamples];

        float nextWeight[s_rootedPartitionMaxSamples];

        for ( int32_t sampleIndex = 0;
              sampleIndex <
              s_rootedPartitionMaxSamples;
              ++sampleIndex )
        {
            rawCastWeight[sampleIndex] =
                0.0f;

            currentWeight[sampleIndex] =
                0.0f;

            nextWeight[sampleIndex] =
                0.0f;
        }

        for ( int32_t sampleIndex = 0;
              sampleIndex <
              sampleCount;
              ++sampleIndex )
        {
            float const originalThickness =
                pSamples[sampleIndex].m_originalDetachedThicknessM;

            if ( originalThickness <=
                 lowThicknessM )
            {
                rawCastWeight[sampleIndex] =
                    0.0f;

                continue;
            }

            if ( originalThickness >=
                 fullThicknessM )
            {
                rawCastWeight[sampleIndex] =
                    1.0f;

                currentWeight[sampleIndex] =
                    1.0f;

                continue;
            }

            float const weight =
                GraniteSmoothStep01(
                    (
                        originalThickness -
                        lowThicknessM ) /
                    GraniteMax(
                        fullThicknessM -
                            lowThicknessM,
                        0.0001f ) );

            rawCastWeight[sampleIndex] =
                weight;

            currentWeight[sampleIndex] =
                weight;
        }

        // Form-fit only the classification field. Parent socket heights remain
        // untouched. Inactive neighbors act as zero support so the principal
        // cast tapers into its mold instead of hanging as vertical fins.
        static constexpr int32_t s_fitPassCount =
            4;

        for ( int32_t passIndex = 0;
              passIndex <
              s_fitPassCount;
              ++passIndex )
        {
            for ( int32_t sampleIndex = 0;
                  sampleIndex <
                  sampleCount;
                  ++sampleIndex )
            {
                nextWeight[sampleIndex] =
                    currentWeight[sampleIndex];
            }

            for ( int32_t y = 1;
                  y <
                  samplesY -
                      1;
                  ++y )
            {
                for ( int32_t x = 1;
                      x <
                      samplesX -
                          1;
                      ++x )
                {
                    int32_t const sampleIndex =
                        y *
                            samplesX +
                        x;

                    float const originalThickness =
                        pSamples[sampleIndex].m_originalDetachedThicknessM;

                    if ( originalThickness <=
                         lowThicknessM )
                    {
                        nextWeight[sampleIndex] =
                            0.0f;

                        continue;
                    }

                    float weightedAverage =
                        currentWeight[sampleIndex] *
                        4.0f;

                    float totalWeight =
                        4.0f;

                    for ( int32_t offsetY = -1;
                          offsetY <=
                          1;
                          ++offsetY )
                    {
                        for ( int32_t offsetX = -1;
                              offsetX <=
                              1;
                              ++offsetX )
                        {
                            if ( offsetX ==
                                     0 &&
                                 offsetY ==
                                     0 )
                            {
                                continue;
                            }

                            int32_t const neighborSample =
                                ( y +
                                  offsetY ) *
                                    samplesX +
                                ( x +
                                  offsetX );

                            float const neighborWeight =
                                offsetX ==
                                            0 ||
                                        offsetY ==
                                            0
                                    ? 1.0f
                                    : 0.60f;

                            weightedAverage +=
                                currentWeight[neighborSample] *
                                neighborWeight;

                            totalWeight +=
                                neighborWeight;
                        }
                    }

                    weightedAverage /=
                        GraniteMax(
                            totalWeight,
                            0.0001f );

                    float const rawWeight =
                        rawCastWeight[sampleIndex];

                    // Strong crown/core samples are anchored; fringe samples
                    // follow their neighbors more strongly.
                    float const fitStrength =
                        GraniteLerp(
                            0.68f,
                            0.22f,
                            rawWeight );

                    float fittedWeight =
                        GraniteLerp(
                            currentWeight[sampleIndex],
                            weightedAverage,
                            fitStrength );

                    fittedWeight =
                        GraniteLerp(
                            fittedWeight,
                            rawWeight,
                            0.22f );

                    nextWeight[sampleIndex] =
                        GraniteClamp01(
                            fittedWeight );
                }
            }

            for ( int32_t sampleIndex = 0;
                  sampleIndex <
                  sampleCount;
                  ++sampleIndex )
            {
                currentWeight[sampleIndex] =
                    nextWeight[sampleIndex];
            }
        }

        // Split original detached Granite into principal cast + spall reserve.
        //
        // IMPORTANT:
        // socketReliefM remains exactly the original FIX2 basin.
        for ( int32_t sampleIndex = 0;
              sampleIndex <
              sampleCount;
              ++sampleIndex )
        {
            GraniteRootedPartitionGridSample& sample =
                pSamples[sampleIndex];

            float const originalThickness =
                sample.m_originalDetachedThicknessM;

            float castWeight =
                currentWeight[sampleIndex];

            // Eliminate numerically tiny tails from the PRINCIPAL body. That
            // material is recorded as spall reserve instead.
            if ( castWeight <
                 0.075f )
            {
                castWeight =
                    0.0f;
            }

            float principalThickness =
                originalThickness *
                castWeight;

            // A second physical gate prevents centimetre-thin ledges from
            // surviving after weight interpolation.
            float const minimumPrincipalThicknessM =
                GraniteMax(
                    0.016f,
                    maximumOriginalThickness *
                        0.055f );

            if ( principalThickness <
                 minimumPrincipalThicknessM )
            {
                principalThickness =
                    0.0f;
            }

            sample.m_detachedThicknessM =
                GraniteMin(
                    principalThickness,
                    originalThickness );

            sample.m_detachedUndersideLiftM =
                0.0f;

            sample.m_spallThicknessM =
                GraniteMax(
                    originalThickness -
                        sample.m_detachedThicknessM,
                    0.0f );

            if ( sample.m_detachedThicknessM >
                 0.000001f )
            {
                ++receipt.m_principalSampleCount;

                if ( receipt.m_minPositivePrincipalThicknessM <=
                         0.0f ||
                     sample.m_detachedThicknessM <
                         receipt.m_minPositivePrincipalThicknessM )
                {
                    receipt.m_minPositivePrincipalThicknessM =
                        sample.m_detachedThicknessM;
                }

                receipt.m_maxPrincipalThicknessM =
                    GraniteMax(
                        receipt.m_maxPrincipalThicknessM,
                        sample.m_detachedThicknessM );
            }

            if ( sample.m_spallThicknessM >
                 0.000001f )
            {
                ++receipt.m_spallSampleCount;
            }

            EE_ASSERT(
                GraniteAbs(
                    sample.m_originalDetachedThicknessM -
                    ( sample.m_detachedThicknessM +
                      sample.m_spallThicknessM ) ) <=
                0.000002f );

            EE_ASSERT(
                GraniteAbs(
                    sample.m_topReliefM -
                    ( sample.m_socketReliefM +
                      sample.m_originalDetachedThicknessM ) ) <=
                0.000002f );
        }

        // One principal MatterBody must be one connected parcel. Any detached
        // island that cannot reach the dominant cast is moved to spall reserve,
        // NOT to the parent socket.
        int32_t seedIndex =
            -1;

        float seedThickness =
            0.0f;

        for ( int32_t sampleIndex = 0;
              sampleIndex <
              sampleCount;
              ++sampleIndex )
        {
            if ( pSamples[sampleIndex].m_detachedThicknessM >
                 seedThickness )
            {
                seedThickness =
                    pSamples[sampleIndex].m_detachedThicknessM;

                seedIndex =
                    sampleIndex;
            }
        }

        if ( seedIndex >=
                 0 &&
             seedThickness >
                 0.000001f )
        {
            bool visited[s_rootedPartitionMaxSamples] =
                {
                    false };

            int32_t queue[s_rootedPartitionMaxSamples];

            int32_t readIndex =
                0;

            int32_t writeIndex =
                0;

            visited[seedIndex] =
                true;

            queue[writeIndex++] =
                seedIndex;

            int32_t const neighborDX[4] =
                {
                    -1,
                    1,
                    0,
                    0 };

            int32_t const neighborDY[4] =
                {
                    0,
                    0,
                    -1,
                    1 };

            while ( readIndex <
                    writeIndex )
            {
                int32_t const currentIndex =
                    queue[readIndex++];

                int32_t const currentX =
                    currentIndex %
                    samplesX;

                int32_t const currentY =
                    currentIndex /
                    samplesX;

                for ( int32_t neighborIndex = 0;
                      neighborIndex <
                      4;
                      ++neighborIndex )
                {
                    int32_t const x =
                        currentX +
                        neighborDX[neighborIndex];

                    int32_t const y =
                        currentY +
                        neighborDY[neighborIndex];

                    if ( x <
                             0 ||
                         x >=
                             samplesX ||
                         y <
                             0 ||
                         y >=
                             samplesY )
                    {
                        continue;
                    }

                    int32_t const sampleIndex =
                        y *
                            samplesX +
                        x;

                    if ( visited[sampleIndex] ||
                         pSamples[sampleIndex].m_detachedThicknessM <=
                             0.000001f )
                    {
                        continue;
                    }

                    visited[sampleIndex] =
                        true;

                    queue[writeIndex++] =
                        sampleIndex;
                }
            }

            for ( int32_t sampleIndex = 0;
                  sampleIndex <
                  sampleCount;
                  ++sampleIndex )
            {
                GraniteRootedPartitionGridSample& sample =
                    pSamples[sampleIndex];

                if ( sample.m_detachedThicknessM <=
                         0.000001f ||
                     visited[sampleIndex] )
                {
                    continue;
                }

                sample.m_spallThicknessM +=
                    sample.m_detachedThicknessM;

                sample.m_detachedThicknessM =
                    0.0f;
            }
        }

        TrimGraniteRootedPrincipalCastSeam(
            pSamples,
            samplesX,
            samplesY );

        // Recompute the cast receipt after disconnected islands and the seam
        // fringe have moved to spall. The retained thickness, rather than the
        // preserved exterior top offset, is the principal MatterBody.
        receipt.m_minPositivePrincipalThicknessM = 0.0f;
        receipt.m_maxPrincipalThicknessM = 0.0f;
        receipt.m_principalSampleCount = 0;
        receipt.m_spallSampleCount = 0;

        for ( int32_t sampleIndex = 0;
              sampleIndex <
              sampleCount;
              ++sampleIndex )
        {
            GraniteRootedPartitionGridSample const& sample =
                pSamples[sampleIndex];

            float const retainedThicknessM =
                GetGraniteRootedRetainedPrincipalThickness(
                    sample );

            if ( retainedThicknessM >
                 0.000001f )
            {
                ++receipt.m_principalSampleCount;

                if ( receipt.m_minPositivePrincipalThicknessM <=
                         0.0f ||
                     retainedThicknessM <
                         receipt.m_minPositivePrincipalThicknessM )
                {
                    receipt.m_minPositivePrincipalThicknessM =
                        retainedThicknessM;
                }

                receipt.m_maxPrincipalThicknessM =
                    GraniteMax(
                        receipt.m_maxPrincipalThicknessM,
                        retainedThicknessM );
            }

            if ( sample.m_spallThicknessM >
                 0.000001f )
            {
                ++receipt.m_spallSampleCount;
            }

            EE_ASSERT(
                GraniteAbs(
                    sample.m_originalDetachedThicknessM -
                    ( retainedThicknessM +
                      sample.m_spallThicknessM ) ) <=
                0.000002f );
        }

        receipt.m_principalConnected =
            IsGraniteRootedPrincipalCastConnected(
                pSamples,
                samplesX,
                samplesY );

        receipt.m_valid =
            receipt.m_principalSampleCount >
                0 &&
            receipt.m_maxPrincipalThicknessM >
                0.00001f &&
            receipt.m_principalConnected;

        return receipt;
    }

    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------

    static int32_t FindOrAddGraniteRootedPartitionVertex(
        GraniteRootedPartitionMesh& mesh,
        float                       x,
        float                       y,
        float                       z,
        float                       mergeToleranceM )
    {
        for ( int32_t vertexIndex = 0;
              vertexIndex <
              mesh.m_numVertices;
              ++vertexIndex )
        {
            GraniteRootedPartitionVertex const& vertex =
                mesh.m_vertices[vertexIndex];

            float const dx =
                vertex.m_x -
                x;

            float const dy =
                vertex.m_y -
                y;

            float const dz =
                vertex.m_z -
                z;

            if ( dx *
                         dx +
                     dy *
                         dy +
                     dz *
                         dz <=
                 mergeToleranceM *
                     mergeToleranceM )
            {
                return vertexIndex;
            }
        }

        EE_ASSERT(
            mesh.m_numVertices <
            GraniteRootedPartitionMesh::s_maxVertices );

        if ( mesh.m_numVertices >=
             GraniteRootedPartitionMesh::s_maxVertices )
        {
            return -1;
        }

        int32_t const index =
            mesh.m_numVertices++;

        GraniteRootedPartitionVertex& vertex =
            mesh.m_vertices[index];

        vertex.m_x =
            x;

        vertex.m_y =
            y;

        vertex.m_z =
            z;

        return index;
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteRootedPartitionTriangleArea(
        GraniteRootedPartitionVertex const& a,
        GraniteRootedPartitionVertex const& b,
        GraniteRootedPartitionVertex const& c )
    {
        float const abX =
            b.m_x -
            a.m_x;

        float const abY =
            b.m_y -
            a.m_y;

        float const abZ =
            b.m_z -
            a.m_z;

        float const acX =
            c.m_x -
            a.m_x;

        float const acY =
            c.m_y -
            a.m_y;

        float const acZ =
            c.m_z -
            a.m_z;

        float const crossX =
            abY *
                acZ -
            abZ *
                acY;

        float const crossY =
            abZ *
                acX -
            abX *
                acZ;

        float const crossZ =
            abX *
                acY -
            abY *
                acX;

        return 0.5f *
               float(
                   std::sqrt(
                       double(
                           crossX *
                               crossX +
                           crossY *
                               crossY +
                           crossZ *
                               crossZ ) ) );
    }

    //-------------------------------------------------------------------------

    static bool AddGraniteRootedPartitionTriangle(
        GraniteRootedPartitionMesh& mesh,
        int32_t                     ia,
        int32_t                     ib,
        int32_t                     ic )
    {
        if ( ia <
                 0 ||
             ib <
                 0 ||
             ic <
                 0 ||
             ia ==
                 ib ||
             ib ==
                 ic ||
             ic ==
                 ia )
        {
            return false;
        }

        GraniteRootedPartitionVertex const& a =
            mesh.m_vertices[ia];

        GraniteRootedPartitionVertex const& b =
            mesh.m_vertices[ib];

        GraniteRootedPartitionVertex const& c =
            mesh.m_vertices[ic];

        if ( MeasureGraniteRootedPartitionTriangleArea(
                 a,
                 b,
                 c ) <=
             0.0000005f )
        {
            return false;
        }

        EE_ASSERT(
            mesh.m_numTriangles <
            GraniteRootedPartitionMesh::s_maxTriangles );

        if ( mesh.m_numTriangles >=
             GraniteRootedPartitionMesh::s_maxTriangles )
        {
            return false;
        }

        int32_t const base =
            mesh.m_numTriangles *
            3;

        mesh.m_triangleIndices[base + 0] =
            uint16_t(
                ia );

        mesh.m_triangleIndices[base + 1] =
            uint16_t(
                ib );

        mesh.m_triangleIndices[base + 2] =
            uint16_t(
                ic );

        ++mesh.m_numTriangles;

        return true;
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteRootedPartitionMeshVolume(
        GraniteRootedPartitionMesh const& mesh )
    {
        double signedVolume =
            0.0;

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              mesh.m_numTriangles;
              ++triangleIndex )
        {
            int32_t const base =
                triangleIndex *
                3;

            GraniteRootedPartitionVertex const& a =
                mesh.m_vertices[mesh.m_triangleIndices[base + 0]];

            GraniteRootedPartitionVertex const& b =
                mesh.m_vertices[mesh.m_triangleIndices[base + 1]];

            GraniteRootedPartitionVertex const& c =
                mesh.m_vertices[mesh.m_triangleIndices[base + 2]];

            double const crossX =
                double(
                    b.m_y ) *
                    double(
                        c.m_z ) -
                double(
                    b.m_z ) *
                    double(
                        c.m_y );

            double const crossY =
                double(
                    b.m_z ) *
                    double(
                        c.m_x ) -
                double(
                    b.m_x ) *
                    double(
                        c.m_z );

            double const crossZ =
                double(
                    b.m_x ) *
                    double(
                        c.m_y ) -
                double(
                    b.m_y ) *
                    double(
                        c.m_x );

            signedVolume +=
                ( double(
                      a.m_x ) *
                      crossX +
                  double(
                      a.m_y ) *
                      crossY +
                  double(
                      a.m_z ) *
                      crossZ ) /
                6.0;
        }

        return float(
            signedVolume <
                    0.0
                ? -signedVolume
                : signedVolume );
    }

    //-------------------------------------------------------------------------

    struct GraniteRootedPartitionEdgeUse
    {
        uint16_t m_low = 0;
        uint16_t m_high = 0;
        uint8_t  m_count = 0;
    };

    //-------------------------------------------------------------------------

    static bool IsGraniteRootedPartitionMeshClosed(
        GraniteRootedPartitionMesh const& mesh )
    {
        static constexpr int32_t s_maxEdges =
            GraniteRootedPartitionMesh::s_maxTriangles *
            3;

        GraniteRootedPartitionEdgeUse edges[s_maxEdges];

        int32_t numEdges =
            0;

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              mesh.m_numTriangles;
              ++triangleIndex )
        {
            int32_t const base =
                triangleIndex *
                3;

            uint16_t const triangleVertices[3] =
                {
                    mesh.m_triangleIndices[base + 0],
                    mesh.m_triangleIndices[base + 1],
                    mesh.m_triangleIndices[base + 2] };

            for ( int32_t edgeIndex = 0;
                  edgeIndex <
                  3;
                  ++edgeIndex )
            {
                uint16_t const a =
                    triangleVertices[edgeIndex];

                uint16_t const b =
                    triangleVertices[(
                                         edgeIndex +
                                         1 ) %
                                     3];

                uint16_t const low =
                    a <
                            b
                        ? a
                        : b;

                uint16_t const high =
                    a <
                            b
                        ? b
                        : a;

                int32_t foundEdge =
                    -1;

                for ( int32_t existingIndex = 0;
                      existingIndex <
                      numEdges;
                      ++existingIndex )
                {
                    if ( edges[existingIndex].m_low ==
                             low &&
                         edges[existingIndex].m_high ==
                             high )
                    {
                        foundEdge =
                            existingIndex;

                        break;
                    }
                }

                if ( foundEdge <
                     0 )
                {
                    EE_ASSERT(
                        numEdges <
                        s_maxEdges );

                    if ( numEdges >=
                         s_maxEdges )
                    {
                        return false;
                    }

                    foundEdge =
                        numEdges++;

                    edges[foundEdge].m_low =
                        low;

                    edges[foundEdge].m_high =
                        high;
                }

                if ( edges[foundEdge].m_count >=
                     2 )
                {
                    return false;
                }

                ++edges[foundEdge].m_count;
            }
        }

        if ( numEdges <=
             0 )
        {
            return false;
        }

        for ( int32_t edgeIndex = 0;
              edgeIndex <
              numEdges;
              ++edgeIndex )
        {
            if ( edges[edgeIndex].m_count !=
                 2 )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    static void FinalizeGraniteRootedPartitionMesh(
        GraniteRootedPartitionMesh& mesh )
    {
        mesh.m_measuredVolumeM3 =
            MeasureGraniteRootedPartitionMeshVolume(
                mesh );

        if ( mesh.m_numVertices <=
             0 )
        {
            return;
        }

        float minX =
            mesh.m_vertices[0].m_x;

        float maxX =
            minX;

        float minY =
            mesh.m_vertices[0].m_y;

        float maxY =
            minY;

        float minZ =
            mesh.m_vertices[0].m_z;

        float maxZ =
            minZ;

        for ( int32_t vertexIndex = 1;
              vertexIndex <
              mesh.m_numVertices;
              ++vertexIndex )
        {
            GraniteRootedPartitionVertex const& vertex =
                mesh.m_vertices[vertexIndex];

            minX =
                GraniteMin(
                    minX,
                    vertex.m_x );

            maxX =
                GraniteMax(
                    maxX,
                    vertex.m_x );

            minY =
                GraniteMin(
                    minY,
                    vertex.m_y );

            maxY =
                GraniteMax(
                    maxY,
                    vertex.m_y );

            minZ =
                GraniteMin(
                    minZ,
                    vertex.m_z );

            maxZ =
                GraniteMax(
                    maxZ,
                    vertex.m_z );
        }

        mesh.m_extentXM =
            maxX -
            minX;

        mesh.m_extentYM =
            maxY -
            minY;

        mesh.m_extentZM =
            maxZ -
            minZ;

        mesh.m_closed =
            IsGraniteRootedPartitionMeshClosed(
                mesh );
    }

    //-------------------------------------------------------------------------
    // Build one closed heightfield shell.
    //
    // upperRelief and lowerRelief share one rectangular carrier. Boundary
    // samples are forced equal so the upper/lower surfaces meet without a
    // separate vertical skirt. Top and bottom use opposite winding.
    //-------------------------------------------------------------------------

    static bool BuildGraniteRootedPartitionShell(
        GraniteRootedPartitionGridSample const* pSamples,
        int32_t                                 samplesX,
        int32_t                                 samplesY,
        float                                   parentCarrierBaseZ,
        bool                                    buildDetachedHead,
        GraniteRootedPartitionMesh&             outMesh )
    {
        EE_ASSERT(
            pSamples !=
            nullptr );

        EE_ASSERT(
            samplesX >
            1 );

        EE_ASSERT(
            samplesY >
            1 );

        outMesh =
            GraniteRootedPartitionMesh();

        int32_t topIndices[s_rootedPartitionMaxSamples];

        int32_t bottomIndices[s_rootedPartitionMaxSamples];

        for ( int32_t sampleIndex = 0;
              sampleIndex <
              s_rootedPartitionMaxSamples;
              ++sampleIndex )
        {
            topIndices[sampleIndex] =
                -1;

            bottomIndices[sampleIndex] =
                -1;
        }

        float const mergeToleranceM =
            0.000001f;

        for ( int32_t y = 0;
              y <
              samplesY;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  samplesX;
                  ++x )
            {
                int32_t const sampleIndex =
                    y *
                        samplesX +
                    x;

                GraniteRootedPartitionGridSample const& sample =
                    pSamples[sampleIndex];

                // FIX6 mold/cast:
                //
                // Principal detached body:
                //     lower = exact parent socket ("mold")
                //     upper = socket + principal cast thickness
                //
                // Any original material above that principal cast is explicit
                // spall reserve and is not glued to the lifted body.
                float const upperReliefM =
                    buildDetachedHead
                        ? sample.m_socketReliefM +
                              sample.m_detachedThicknessM
                        : sample.m_socketReliefM;

                float const lowerReliefM =
                    buildDetachedHead
                        ? sample.m_socketReliefM +
                              sample.m_detachedUndersideLiftM
                        : 0.0f;

                topIndices[sampleIndex] =
                    FindOrAddGraniteRootedPartitionVertex(
                        outMesh,
                        sample.m_worldX,
                        sample.m_worldY,
                        parentCarrierBaseZ +
                            upperReliefM,
                        mergeToleranceM );

                bottomIndices[sampleIndex] =
                    FindOrAddGraniteRootedPartitionVertex(
                        outMesh,
                        sample.m_worldX,
                        sample.m_worldY,
                        parentCarrierBaseZ +
                            lowerReliefM,
                        mergeToleranceM );

                if ( topIndices[sampleIndex] <
                         0 ||
                     bottomIndices[sampleIndex] <
                         0 )
                {
                    return false;
                }
            }
        }

        for ( int32_t y = 0;
              y <
              samplesY -
                  1;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  samplesX -
                      1;
                  ++x )
            {
                int32_t const i00 =
                    y *
                        samplesX +
                    x;

                int32_t const i10 =
                    y *
                        samplesX +
                    x +
                    1;

                int32_t const i01 =
                    ( y +
                      1 ) *
                        samplesX +
                    x;

                int32_t const i11 =
                    ( y +
                      1 ) *
                        samplesX +
                    x +
                    1;

                // P3C.10B-2 FIX1:
                //
                // Do NOT emit coincident top+bottom faces where this shell has
                // exactly zero thickness.
                //
                // The previous implementation emitted both surfaces across
                // the entire rectangular carrier. In zero-thickness regions
                // the top and bottom vertices collapse to the same positions,
                // producing duplicate coplanar triangles and 4-use edges.
                // The geometry was visually harmless but topologically
                // non-manifold, correctly failing m_detachedMeshClosedPass.
                //
                // We instead emit a top/bottom triangle pair only when that
                // triangle owns positive shell thickness at at least one of
                // its three vertices. At the boundary of the active parcel,
                // top and bottom collapse onto the same zero-thickness edge,
                // which closes the shell without adding an artificial skirt.
                float const thickness00 =
                    buildDetachedHead
                        ? GetGraniteRootedRetainedPrincipalThickness(
                              pSamples[i00] )
                        : pSamples[i00].m_socketReliefM;

                float const thickness10 =
                    buildDetachedHead
                        ? GetGraniteRootedRetainedPrincipalThickness(
                              pSamples[i10] )
                        : pSamples[i10].m_socketReliefM;

                float const thickness01 =
                    buildDetachedHead
                        ? GetGraniteRootedRetainedPrincipalThickness(
                              pSamples[i01] )
                        : pSamples[i01].m_socketReliefM;

                float const thickness11 =
                    buildDetachedHead
                        ? GetGraniteRootedRetainedPrincipalThickness(
                              pSamples[i11] )
                        : pSamples[i11].m_socketReliefM;

                float constexpr activeThicknessEpsilonM =
                    0.000001f;

                bool const firstTriangleActive =
                    thickness00 >
                        activeThicknessEpsilonM ||
                    thickness10 >
                        activeThicknessEpsilonM ||
                    thickness11 >
                        activeThicknessEpsilonM;

                if ( firstTriangleActive )
                {
                    // Upper surface: upward/outward winding.
                    AddGraniteRootedPartitionTriangle(
                        outMesh,
                        topIndices[i00],
                        topIndices[i10],
                        topIndices[i11] );

                    // Lower surface: exact reverse winding.
                    AddGraniteRootedPartitionTriangle(
                        outMesh,
                        bottomIndices[i00],
                        bottomIndices[i11],
                        bottomIndices[i10] );
                }

                bool const secondTriangleActive =
                    thickness00 >
                        activeThicknessEpsilonM ||
                    thickness11 >
                        activeThicknessEpsilonM ||
                    thickness01 >
                        activeThicknessEpsilonM;

                if ( secondTriangleActive )
                {
                    // Upper surface: upward/outward winding.
                    AddGraniteRootedPartitionTriangle(
                        outMesh,
                        topIndices[i00],
                        topIndices[i11],
                        topIndices[i01] );

                    // Lower surface: exact reverse winding.
                    AddGraniteRootedPartitionTriangle(
                        outMesh,
                        bottomIndices[i00],
                        bottomIndices[i01],
                        bottomIndices[i11] );
                }
            }
        }

        FinalizeGraniteRootedPartitionMesh(
            outMesh );

        return outMesh.m_numVertices >
                   0 &&
               outMesh.m_numTriangles >
                   0 &&
               outMesh.m_measuredVolumeM3 >
                   0.000001f;
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteRootedSharedInterfaceArea(
        GraniteRootedPartitionGridSample const* pSamples,
        int32_t                                 samplesX,
        int32_t                                 samplesY,
        float                                   parentCarrierBaseZ )
    {
        double area =
            0.0;

        for ( int32_t y = 0;
              y <
              samplesY -
                  1;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  samplesX -
                      1;
                  ++x )
            {
                int32_t const i00 =
                    y *
                        samplesX +
                    x;

                int32_t const i10 =
                    y *
                        samplesX +
                    x +
                    1;

                int32_t const i01 =
                    ( y +
                      1 ) *
                        samplesX +
                    x;

                int32_t const i11 =
                    ( y +
                      1 ) *
                        samplesX +
                    x +
                    1;

                GraniteRootedPartitionGridSample const& s00 =
                    pSamples[i00];

                GraniteRootedPartitionGridSample const& s10 =
                    pSamples[i10];

                GraniteRootedPartitionGridSample const& s01 =
                    pSamples[i01];

                GraniteRootedPartitionGridSample const& s11 =
                    pSamples[i11];

                bool const firstTriangleSharesMold =
                    s00.m_detachedUndersideLiftM <= 0.000001f &&
                    s10.m_detachedUndersideLiftM <= 0.000001f &&
                    s11.m_detachedUndersideLiftM <= 0.000001f &&
                    GraniteMax(
                        GetGraniteRootedRetainedPrincipalThickness( s00 ),
                        GraniteMax(
                            GetGraniteRootedRetainedPrincipalThickness( s10 ),
                            GetGraniteRootedRetainedPrincipalThickness( s11 ) ) ) >
                        0.00001f;

                bool const secondTriangleSharesMold =
                    s00.m_detachedUndersideLiftM <= 0.000001f &&
                    s11.m_detachedUndersideLiftM <= 0.000001f &&
                    s01.m_detachedUndersideLiftM <= 0.000001f &&
                    GraniteMax(
                        GetGraniteRootedRetainedPrincipalThickness( s00 ),
                        GraniteMax(
                            GetGraniteRootedRetainedPrincipalThickness( s11 ),
                            GetGraniteRootedRetainedPrincipalThickness( s01 ) ) ) >
                        0.00001f;

                if ( !firstTriangleSharesMold &&
                     !secondTriangleSharesMold )
                {
                    continue;
                }

                GraniteRootedPartitionVertex const v00 =
                    {
                        s00.m_worldX,
                        s00.m_worldY,
                        parentCarrierBaseZ +
                            s00.m_socketReliefM };

                GraniteRootedPartitionVertex const v10 =
                    {
                        s10.m_worldX,
                        s10.m_worldY,
                        parentCarrierBaseZ +
                            s10.m_socketReliefM };

                GraniteRootedPartitionVertex const v01 =
                    {
                        s01.m_worldX,
                        s01.m_worldY,
                        parentCarrierBaseZ +
                            s01.m_socketReliefM };

                GraniteRootedPartitionVertex const v11 =
                    {
                        s11.m_worldX,
                        s11.m_worldY,
                        parentCarrierBaseZ +
                            s11.m_socketReliefM };

                if ( firstTriangleSharesMold )
                {
                    area +=
                        double(
                            MeasureGraniteRootedPartitionTriangleArea(
                                v00,
                                v10,
                                v11 ) );
                }

                if ( secondTriangleSharesMold )
                {
                    area +=
                        double(
                            MeasureGraniteRootedPartitionTriangleArea(
                                v00,
                                v11,
                                v01 ) );
                }
            }
        }

        return float(
            area );
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteRootedInterfaceVertexDeviation(
        GraniteRootedPartitionMesh const& mesh,
        float                             expectedX,
        float                             expectedY,
        float                             expectedZ )
    {
        float minimumDistanceSquared =
            1.0e30f;

        for ( int32_t vertexIndex = 0;
              vertexIndex <
              mesh.m_numVertices;
              ++vertexIndex )
        {
            GraniteRootedPartitionVertex const& vertex =
                mesh.m_vertices[vertexIndex];

            float const dx = vertex.m_x - expectedX;
            float const dy = vertex.m_y - expectedY;
            float const dz = vertex.m_z - expectedZ;

            minimumDistanceSquared =
                GraniteMin(
                    minimumDistanceSquared,
                    dx * dx + dy * dy + dz * dz );
        }

        if ( minimumDistanceSquared >=
             1.0e29f )
        {
            return 1.0e30f;
        }

        return float(
            std::sqrt(
                double(
                    minimumDistanceSquared ) ) );
    }

    //-------------------------------------------------------------------------
    // Measure the emitted meshes, not the source samples alone. Every active
    // principal-cast sample must have one detached underside vertex and one
    // surviving parent-socket vertex at the same mold position.
    //-------------------------------------------------------------------------

    static float MeasureGraniteRootedSharedInterfaceDeviation(
        GraniteRootedPartitionGridSample const* pSamples,
        int32_t                                 sampleCount,
        float                                   parentCarrierBaseZ,
        GraniteRootedPartitionMesh const&       detachedMesh,
        GraniteRootedPartitionMesh const&       parentSocketMesh )
    {
        float maximumDeviationM =
            0.0f;

        for ( int32_t sampleIndex = 0;
              sampleIndex <
              sampleCount;
              ++sampleIndex )
        {
            GraniteRootedPartitionGridSample const& sample =
                pSamples[sampleIndex];

            if ( GetGraniteRootedRetainedPrincipalThickness(
                     sample ) <=
                     0.000001f ||
                 sample.m_detachedUndersideLiftM >
                     0.000001f )
            {
                continue;
            }

            float const expectedInterfaceZ =
                parentCarrierBaseZ +
                sample.m_socketReliefM;

            maximumDeviationM =
                GraniteMax(
                    maximumDeviationM,
                    MeasureGraniteRootedInterfaceVertexDeviation(
                        detachedMesh,
                        sample.m_worldX,
                        sample.m_worldY,
                        expectedInterfaceZ ) );

            maximumDeviationM =
                GraniteMax(
                    maximumDeviationM,
                    MeasureGraniteRootedInterfaceVertexDeviation(
                        parentSocketMesh,
                        sample.m_worldX,
                        sample.m_worldY,
                        expectedInterfaceZ ) );
        }

        return maximumDeviationM;
    }

    //-------------------------------------------------------------------------

    enum class GraniteRootedShapeField : uint8_t
    {
        TotalApron,
        RetainedApron
    };

    //-------------------------------------------------------------------------

    static float GetGraniteRootedShapeFieldValue(
        GraniteRootedPartitionGridSample const& sample,
        GraniteRootedShapeField                 field )
    {
        switch ( field )
        {
            case GraniteRootedShapeField::TotalApron:
                return sample.m_topReliefM *
                       sample.m_rootBlendInfluence;

            case GraniteRootedShapeField::RetainedApron:
                return sample.m_socketReliefM *
                       sample.m_rootBlendInfluence;

            default:
                EE_ASSERT( false );
                return 0.0f;
        }
    }

    //-------------------------------------------------------------------------
    // Integrate the same fixed-diagonal sampled carrier used to emit the
    // partition meshes. Root-blend influence provides a continuous ownership
    // weight for retained apron matter.
    //-------------------------------------------------------------------------

    static float MeasureGraniteRootedShapeFieldVolume(
        GraniteRootedPartitionGridSample const* pSamples,
        int32_t                                 samplesX,
        int32_t                                 samplesY,
        float                                   spacingX,
        float                                   spacingY,
        GraniteRootedShapeField                 field )
    {
        double volume =
            0.0;

        float const halfCellArea =
            spacingX *
            spacingY *
            0.5f;

        for ( int32_t y = 0;
              y <
              samplesY - 1;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  samplesX - 1;
                  ++x )
            {
                int32_t const i00 = y * samplesX + x;
                int32_t const i10 = i00 + 1;
                int32_t const i01 = ( y + 1 ) * samplesX + x;
                int32_t const i11 = i01 + 1;

                float const h00 =
                    GetGraniteRootedShapeFieldValue(
                        pSamples[i00], field );

                float const h10 =
                    GetGraniteRootedShapeFieldValue(
                        pSamples[i10], field );

                float const h01 =
                    GetGraniteRootedShapeFieldValue(
                        pSamples[i01], field );

                float const h11 =
                    GetGraniteRootedShapeFieldValue(
                        pSamples[i11], field );

                volume +=
                    double(
                        ( ( h00 + h10 + h11 ) / 3.0f +
                          ( h00 + h11 + h01 ) / 3.0f ) *
                        halfCellArea );
            }
        }

        return float(
            volume );
    }

    //-------------------------------------------------------------------------
    // Independent curtain oracle. Recompute the positive-to-zero exterior
    // fringe after seam fitting and integrate any retained principal thickness
    // still owned there. A correctly trimmed seam reports exactly zero; a
    // future change that lets even a thick boundary sheet survive is caught.
    //-------------------------------------------------------------------------

    static float MeasureGraniteRootedUntrimmedCurtainVolume(
        GraniteRootedPartitionGridSample const* pSamples,
        int32_t                                 samplesX,
        int32_t                                 samplesY,
        float                                   spacingX,
        float                                   spacingY )
    {
        float curtainThickness[s_rootedPartitionMaxSamples] =
            {
                0.0f };

        for ( int32_t y = 0;
              y <
              samplesY;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  samplesX;
                  ++x )
            {
                int32_t const sampleIndex =
                    y *
                        samplesX +
                    x;

                if ( IsGraniteRootedPrincipalCurtainBoundarySample(
                         pSamples,
                         samplesX,
                         samplesY,
                         x,
                         y ) )
                {
                    curtainThickness[sampleIndex] =
                        GetGraniteRootedRetainedPrincipalThickness(
                            pSamples[sampleIndex] );
                }
            }
        }

        double volume =
            0.0;

        float const halfCellArea =
            spacingX *
            spacingY *
            0.5f;

        for ( int32_t y = 0;
              y <
              samplesY - 1;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  samplesX - 1;
                  ++x )
            {
                int32_t const i00 = y * samplesX + x;
                int32_t const i10 = i00 + 1;
                int32_t const i01 = ( y + 1 ) * samplesX + x;
                int32_t const i11 = i01 + 1;

                volume +=
                    double(
                        ( ( curtainThickness[i00] +
                            curtainThickness[i10] +
                            curtainThickness[i11] ) /
                              3.0f +
                          ( curtainThickness[i00] +
                            curtainThickness[i11] +
                            curtainThickness[i01] ) /
                              3.0f ) *
                        halfCellArea );
            }
        }

        return float(
            volume );
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteRootedPreDetachParcelVolume(
        GraniteRootedPartitionGridSample const* pSamples,
        int32_t                                 samplesX,
        int32_t                                 samplesY,
        float                                   spacingX,
        float                                   spacingY )
    {
        // Exact integral of the same fixed-diagonal piecewise-linear carrier
        // used by BuildGraniteRootedPartitionShell.
        double volume =
            0.0;

        float const halfCellArea =
            spacingX *
            spacingY *
            0.5f;

        for ( int32_t y = 0;
              y <
              samplesY -
                  1;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  samplesX -
                      1;
                  ++x )
            {
                int32_t const i00 =
                    y *
                        samplesX +
                    x;

                int32_t const i10 =
                    y *
                        samplesX +
                    x +
                    1;

                int32_t const i01 =
                    ( y +
                      1 ) *
                        samplesX +
                    x;

                int32_t const i11 =
                    ( y +
                      1 ) *
                        samplesX +
                    x +
                    1;

                float const h00 =
                    pSamples[i00].m_topReliefM;

                float const h10 =
                    pSamples[i10].m_topReliefM;

                float const h01 =
                    pSamples[i01].m_topReliefM;

                float const h11 =
                    pSamples[i11].m_topReliefM;

                float const firstTriangleMean =
                    ( h00 +
                      h10 +
                      h11 ) /
                    3.0f;

                float const secondTriangleMean =
                    ( h00 +
                      h11 +
                      h01 ) /
                    3.0f;

                volume +=
                    double(
                        (
                            firstTriangleMean +
                            secondTriangleMean ) *
                        halfCellArea );
            }
        }

        return float(
            volume );
    }

    //-------------------------------------------------------------------------

    GraniteRootedSeparationResult
    BuildGraniteRootedSeparationTransaction(
        GraniteRootedSeparationRequest const& request )
    {
        GraniteRootedSeparationResult result;

        result.m_partitionEventID =
            request.m_partitionEventID;

        result.m_parentFormEventID =
            request.m_rootedForm.m_eventID;

        result.m_geologicalAncestryID =
            request.m_rootedForm.m_geologicalAncestryID;

        result.m_rootedForm =
            request.m_rootedForm;

        result.m_sourceDetachedCandidate =
            request.m_finalDetachment.m_detachedCandidate;

        result.m_policy =
            request.m_policy;

        result.m_structuralReleasePass =
            request.m_rootedForm.m_valid &&
            request.m_rootedForm.m_type ==
                GraniteFormationFormType::RootedRockHead &&
            request.m_rootedForm.m_attachment ==
                GraniteFormationAttachmentState::RootedRockHead &&
            request.m_finalDetachment.m_pass &&
            request.m_finalDetachment.m_stateAfter ==
                GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate &&
            !request.m_finalDetachment.m_parentStructuralOwnershipRetained &&
            request.m_finalDetachment.m_detachedCandidateReady &&
            request.m_finalDetachment.m_detachedCandidate.m_valid;

        if ( !result.m_structuralReleasePass )
        {
            return result;
        }

        GetGraniteRootedPartitionWorldBounds(
            request.m_rootedForm,
            result.m_minWorldX,
            result.m_minWorldY,
            result.m_maxWorldX,
            result.m_maxWorldY );

        float const spanX =
            result.m_maxWorldX -
            result.m_minWorldX;

        float const spanY =
            result.m_maxWorldY -
            result.m_minWorldY;

        float const maximumSpan =
            GraniteMax(
                spanX,
                spanY );

        int32_t const samplesPerAxis =
            ResolveGraniteRootedPartitionSamplesPerAxis(
                maximumSpan,
                request.m_policy.m_targetSampleSpacingM );

        result.m_samplesX =
            samplesPerAxis;

        result.m_samplesY =
            samplesPerAxis;

        float const spacingX =
            spanX /
            float(
                samplesPerAxis -
                1 );

        float const spacingY =
            spanY /
            float(
                samplesPerAxis -
                1 );

        result.m_sampleSpacingM =
            GraniteMax(
                spacingX,
                spacingY );

        GraniteRootedPartitionGridSample samples[s_rootedPartitionMaxSamples];

        for ( int32_t sampleIndex = 0;
              sampleIndex <
              s_rootedPartitionMaxSamples;
              ++sampleIndex )
        {
            samples[sampleIndex] =
                GraniteRootedPartitionGridSample();
        }

        for ( int32_t y = 0;
              y <
              samplesPerAxis;
              ++y )
        {
            float const worldY =
                result.m_minWorldY +
                float(
                    y ) *
                    spacingY;

            for ( int32_t x = 0;
                  x <
                  samplesPerAxis;
                  ++x )
            {
                float const worldX =
                    result.m_minWorldX +
                    float(
                        x ) *
                        spacingX;

                bool const forceBoundaryZero =
                    x ==
                        0 ||
                    y ==
                        0 ||
                    x ==
                        samplesPerAxis -
                            1 ||
                    y ==
                        samplesPerAxis -
                            1;

                samples[y *
                            samplesPerAxis +
                        x] =
                    EvaluateGraniteRootedPartitionSample(
                        request,
                        worldX,
                        worldY,
                        forceBoundaryZero );
            }
        }

        // FIX6:
        // Freeze the approved parent socket, then classify the original
        // detached field into one principal cast + conserved spall reserve.
        GraniteRootedCastFitReceipt const castFit =
            FitGraniteRootedPrincipalCastToSocket(
                samples,
                samplesPerAxis,
                samplesPerAxis );

        if ( !castFit.m_valid )
        {
            return result;
        }

        if ( !BuildGraniteRootedPartitionShell(
                 samples,
                 samplesPerAxis,
                 samplesPerAxis,
                 request.m_parentCarrierBaseZ,
                 true,
                 result.m_detachedHeadMesh ) )
        {
            return result;
        }

        if ( !BuildGraniteRootedPartitionShell(
                 samples,
                 samplesPerAxis,
                 samplesPerAxis,
                 request.m_parentCarrierBaseZ,
                 false,
                 result.m_remainingParentSocketMesh ) )
        {
            return result;
        }

        float const preDetachIntegratedVolumeM3 =
            MeasureGraniteRootedPreDetachParcelVolume(
                samples,
                samplesPerAxis,
                samplesPerAxis,
                spacingX,
                spacingY );

        result.m_matter.m_preDetachParcelVolumeM3 =
            preDetachIntegratedVolumeM3;

        result.m_matter.m_detachedVolumeM3 =
            result.m_detachedHeadMesh.m_measuredVolumeM3;

        result.m_matter.m_spallReserveVolumeM3 =
            MeasureGraniteRootedThicknessFieldVolume(
                samples,
                samplesPerAxis,
                samplesPerAxis,
                spacingX,
                spacingY,
                2 );

        result.m_matter.m_remainingParentVolumeM3 =
            result.m_remainingParentSocketMesh.m_measuredVolumeM3;

        float const componentVolumeSumM3 =
            result.m_matter.m_detachedVolumeM3 +
            result.m_matter.m_spallReserveVolumeM3 +
            result.m_matter.m_remainingParentVolumeM3;

        result.m_matter.m_volumeResidualM3 =
            componentVolumeSumM3 -
            result.m_matter.m_preDetachParcelVolumeM3;

        float const preDetachVolumeDenominator =
            GraniteMax(
                result.m_matter.m_preDetachParcelVolumeM3,
                0.000001f );

        result.m_matter.m_volumeRelativeResidual =
            GraniteAbs(
                result.m_matter.m_volumeResidualM3 ) /
            preDetachVolumeDenominator;

        // Independent cast/spall split receipt. This proves that removing fins
        // from the PRINCIPAL lifted body did not silently return them to the
        // parent socket.
        float const originalDetachedVolumeM3 =
            MeasureGraniteRootedThicknessFieldVolume(
                samples,
                samplesPerAxis,
                samplesPerAxis,
                spacingX,
                spacingY,
                0 );

        float const principalDetachedThicknessVolumeM3 =
            MeasureGraniteRootedThicknessFieldVolume(
                samples,
                samplesPerAxis,
                samplesPerAxis,
                spacingX,
                spacingY,
                1 );

        float const totalApronVolumeM3 =
            MeasureGraniteRootedShapeFieldVolume(
                samples,
                samplesPerAxis,
                samplesPerAxis,
                spacingX,
                spacingY,
                GraniteRootedShapeField::TotalApron );

        float const retainedApronVolumeM3 =
            MeasureGraniteRootedShapeFieldVolume(
                samples,
                samplesPerAxis,
                samplesPerAxis,
                spacingX,
                spacingY,
                GraniteRootedShapeField::RetainedApron );

        result.m_undersideFinVolumeM3 =
            MeasureGraniteRootedUntrimmedCurtainVolume(
                samples,
                samplesPerAxis,
                samplesPerAxis,
                spacingX,
                spacingY );

        float const castSpallResidualM3 =
            ( principalDetachedThicknessVolumeM3 +
              result.m_matter.m_spallReserveVolumeM3 ) -
            originalDetachedVolumeM3;

        MaterialGeometryProfile const graniteMatterProfile =
            GetMaterialGeometryProfile(
                ProvenanceMaterialID::Granite );

        result.m_matter.m_densityKgPerM3 =
            graniteMatterProfile.m_intrinsicSolidDensityKgPerM3;

        result.m_retainedApronMassGrams =
            uint32_t(
                double(
                    GraniteMax(
                        retainedApronVolumeM3,
                        0.0f ) ) *
                    double(
                        result.m_matter.m_densityKgPerM3 ) *
                    1000.0 +
                0.5 );

        result.m_retainedApronMassFraction =
            totalApronVolumeM3 >
                    0.000001f
                ? GraniteClamp01(
                      retainedApronVolumeM3 /
                      totalApronVolumeM3 )
                : 0.0f;

        result.m_undersideFinVolumeFraction =
            result.m_matter.m_detachedVolumeM3 >
                    0.000001f
                ? GraniteMax(
                      result.m_undersideFinVolumeM3,
                      0.0f ) /
                      result.m_matter.m_detachedVolumeM3
                : 1.0f;

        result.m_matter.m_preDetachParcelMassGrams =
            uint32_t(
                double(
                    result.m_matter.m_preDetachParcelVolumeM3 ) *
                    double(
                        result.m_matter.m_densityKgPerM3 ) *
                    1000.0 +
                0.5 );

        if ( result.m_matter.m_preDetachParcelMassGrams <
             3 )
        {
            return result;
        }

        float const safeComponentVolume =
            GraniteMax(
                componentVolumeSumM3,
                0.000001f );

        double const detachedMassExact =
            double(
                result.m_matter.m_preDetachParcelMassGrams ) *
            double(
                result.m_matter.m_detachedVolumeM3 /
                safeComponentVolume );

        double const spallMassExact =
            double(
                result.m_matter.m_preDetachParcelMassGrams ) *
            double(
                result.m_matter.m_spallReserveVolumeM3 /
                safeComponentVolume );

        uint32_t detachedMassGrams =
            uint32_t(
                detachedMassExact +
                0.5 );

        uint32_t spallMassGrams =
            uint32_t(
                spallMassExact +
                0.5 );

        if ( result.m_matter.m_detachedVolumeM3 >
                 0.000001f &&
             detachedMassGrams <
                 1 )
        {
            detachedMassGrams =
                1;
        }

        if ( result.m_matter.m_spallReserveVolumeM3 >
                 0.000001f &&
             spallMassGrams <
                 1 )
        {
            spallMassGrams =
                1;
        }

        uint64_t assignedNonParent =
            uint64_t(
                detachedMassGrams ) +
            uint64_t(
                spallMassGrams );

        if ( assignedNonParent >=
             uint64_t(
                 result.m_matter.m_preDetachParcelMassGrams ) )
        {
            uint64_t const maximumNonParent =
                uint64_t(
                    result.m_matter.m_preDetachParcelMassGrams ) -
                1u;

            if ( uint64_t(
                     detachedMassGrams ) >
                 maximumNonParent )
            {
                detachedMassGrams =
                    uint32_t(
                        maximumNonParent );

                spallMassGrams =
                    0;
            }
            else
            {
                spallMassGrams =
                    uint32_t(
                        maximumNonParent -
                        uint64_t(
                            detachedMassGrams ) );
            }
        }

        result.m_matter.m_detachedMassGrams =
            detachedMassGrams;

        result.m_matter.m_spallReserveMassGrams =
            spallMassGrams;

        result.m_matter.m_remainingParentMassGrams =
            result.m_matter.m_preDetachParcelMassGrams -
            result.m_matter.m_detachedMassGrams -
            result.m_matter.m_spallReserveMassGrams;

        result.m_matter.m_massResidualGrams =
            int64_t(
                result.m_matter.m_detachedMassGrams ) +
            int64_t(
                result.m_matter.m_spallReserveMassGrams ) +
            int64_t(
                result.m_matter.m_remainingParentMassGrams ) -
            int64_t(
                result.m_matter.m_preDetachParcelMassGrams );

        result.m_matter.m_massRelativeResidual =
            GraniteAbs(
                float(
                    result.m_matter.m_massResidualGrams ) ) /
            GraniteMax(
                float(
                    result.m_matter.m_preDetachParcelMassGrams ),
                1.0f );

        float const volumeTolerance =
            GraniteMax(
                0.000001f,
                preDetachVolumeDenominator *
                    GraniteMax(
                        request.m_policy.m_volumeRelativeTolerance,
                        0.00001f ) );

        result.m_matter.m_volumeConservationPass =
            GraniteAbs(
                result.m_matter.m_volumeResidualM3 ) <=
            volumeTolerance;

        result.m_matter.m_massConservationPass =
            result.m_matter.m_massResidualGrams ==
            0;

        result.m_sharedInterfaceAreaM2 =
            MeasureGraniteRootedSharedInterfaceArea(
                samples,
                samplesPerAxis,
                samplesPerAxis,
                request.m_parentCarrierBaseZ );

        result.m_maxSharedInterfaceDeviationM =
            MeasureGraniteRootedSharedInterfaceDeviation(
                samples,
                samplesPerAxis *
                    samplesPerAxis,
                request.m_parentCarrierBaseZ,
                result.m_detachedHeadMesh,
                result.m_remainingParentSocketMesh );

        result.m_detachedMeshClosedPass =
            result.m_detachedHeadMesh.m_closed;

        result.m_parentSocketMeshClosedPass =
            result.m_remainingParentSocketMesh.m_closed;

        result.m_principalCastFitPass =
            castFit.m_valid &&
            castFit.m_principalConnected &&
            castFit.m_principalSampleCount >
                0 &&
            result.m_detachedMeshClosedPass &&
            result.m_maxSharedInterfaceDeviationM <=
                GraniteRootedApprovedShapeThresholds::
                    s_maxPrincipalCastFitDeviationM;

        result.m_approvedShapePass =
            result.m_principalCastFitPass &&
            result.m_retainedApronMassFraction >=
                GraniteRootedApprovedShapeThresholds::
                    s_minRetainedApronMassFraction &&
            result.m_undersideFinVolumeFraction <=
                GraniteRootedApprovedShapeThresholds::
                    s_maxUndersideFinVolumeFraction;

        float const detachedSplitTolerance =
            GraniteMax(
                0.000001f,
                GraniteMax(
                    originalDetachedVolumeM3,
                    0.000001f ) *
                    0.0015f );

        result.m_spallReservePass =
            result.m_matter.m_spallReserveVolumeM3 >=
                0.0f &&
            GraniteAbs(
                castSpallResidualM3 ) <=
                detachedSplitTolerance &&
            ( result.m_matter.m_spallReserveVolumeM3 <=
                  0.000001f ||
              result.m_matter.m_spallReserveMassGrams >
                  0 );

        result.m_geometricPartitionPass =
            result.m_matter.m_volumeConservationPass &&
            result.m_approvedShapePass &&
            result.m_spallReservePass;

        result.m_sharedInterfacePass =
            result.m_sharedInterfaceAreaM2 >
                0.000001f &&
            result.m_maxSharedInterfaceDeviationM <=
                GraniteMax(
                    request.m_policy.m_surfaceCoincidenceToleranceM,
                    0.000001f );

        result.m_matterPartitionPass =
            result.m_matter.m_volumeConservationPass &&
            result.m_matter.m_massConservationPass &&
            result.m_matter.m_massRelativeResidual <=
                GraniteMax(
                    request.m_policy.m_massRelativeTolerance,
                    0.0f ) &&
            result.m_spallReservePass;

        result.m_detachedBodyID =
            HashGraniteValue(
                request.m_worldSeed ^
                    request.m_partitionEventID ^
                    0x10B20001u,
                int32_t(
                    request.m_rootedForm.m_eventID &
                    0x7FFFFFFFu ),
                int32_t(
                    request.m_rootedForm.m_grammarSalt &
                    0x7FFFFFFFu ) );

        if ( result.m_detachedBodyID ==
             0 )
        {
            result.m_detachedBodyID =
                1;
        }

        // Continuous bedrock is not yet represented by one finite MatterBody.
        // Zero therefore correctly means "parent is formation/world substrate",
        // not "missing ancestry".
        result.m_detachedParentBodyID =
            0;

        result.m_detachedProvenanceID =
            HashGraniteValue(
                request.m_worldSeed ^
                    request.m_partitionEventID ^
                    0x10B20002u,
                int32_t(
                    request.m_rootedForm.m_geologicalAncestryID &
                    0x7FFFFFFFu ),
                int32_t(
                    request.m_rootedForm.m_eventID &
                    0x7FFFFFFFu ) );

        if ( result.m_detachedProvenanceID ==
             0 )
        {
            result.m_detachedProvenanceID =
                request.m_rootedForm.m_geologicalAncestryID;
        }

        result.m_detachedMassGrams =
            result.m_matter.m_detachedMassGrams;

        result.m_pass =
            result.m_structuralReleasePass &&
            result.m_geometricPartitionPass &&
            result.m_sharedInterfacePass &&
            result.m_detachedMeshClosedPass &&
            result.m_parentSocketMeshClosedPass &&
            result.m_principalCastFitPass &&
            result.m_approvedShapePass &&
            result.m_spallReservePass &&
            result.m_matterPartitionPass;

        result.m_valid =
            result.m_pass;

        EE_ASSERT(
            result.m_structuralReleasePass );

        EE_ASSERT(
            result.m_geometricPartitionPass );

        EE_ASSERT(
            result.m_sharedInterfacePass );

        EE_ASSERT(
            result.m_detachedMeshClosedPass );

        EE_ASSERT(
            result.m_parentSocketMeshClosedPass );

        EE_ASSERT(
            result.m_principalCastFitPass );

        EE_ASSERT(
            result.m_approvedShapePass );

        EE_ASSERT(
            result.m_spallReservePass );

        EE_ASSERT(
            result.m_matterPartitionPass );

        return result;
    }
}
