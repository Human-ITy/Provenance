#include "WorldSystem_Provenance.h"
#include "ProvenanceSurfaceEvaluation.h"

#include "Game/Provenance/Components/Component_ProvenanceWorldSettings.h"
#include "Game/Provenance/Geometry/GraniteGeometry.h"
#include "Game/Provenance/Geometry/GraniteExcavation.h"
#include "Engine/Camera/Systems/WorldSystem_Camera.h"
#include "Engine/Camera/Components/Component_ToolsCamera.h"
#include "Game/Provenance/Geometry/SoilGeometry.h"
#include "Game/Provenance/Verification/GraniteAuthorityVerifier.h"

#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Render/Systems/WorldSystem_Render.h"
#include "Base/Input/InputSystem.h"

#include <cmath>
#include <cstdio>
#include <new>
#include <algorithm>
#include <chrono>
#include <cstring>
#include "Base/FileSystem/FileSystem.h"

#if EE_DEVELOPMENT_TOOLS

    #include "Base/Drawing/DebugDrawing.h"

namespace EE
{

    template<typename T>
    static void ResetLargeDebugValueInPlace( T& value )
    {
        // These debug aggregates own large fixed-capacity geometry arrays.
        // Assignment from T() materializes another complete aggregate on the
        // caller's stack, which can exhaust the editor thread before the
        // callee body begins. Reconstruct the persistent object in its existing
        // storage so reset cost is independent of aggregate size.
        value.~T();
        ::new ( static_cast<void*>( &value ) ) T();
    }

    //-------------------------------------------------------------------------
    // File-local constants/helpers formerly shared implicitly by the monolith.
    // Keeping them local avoids introducing a new global utility surface merely
    // to split one translation unit.
    //-------------------------------------------------------------------------

    static constexpr uint32_t s_graniteGeologicalAncestryID =
        7001u;

    static float Clamp01(
        float value )
    {
        if ( value <
             0.0f )
        {
            return 0.0f;
        }

        if ( value >
             1.0f )
        {
            return 1.0f;
        }

        return value;
    }

    //-------------------------------------------------------------------------

    static float AbsFloat(
        float value )
    {
        return value <
                       0.0f
                 ? -value
                 : value;
    }

    //-------------------------------------------------------------------------

    static float MaxFloat(
        float a,
        float b )
    {
        return a >
                       b
                 ? a
                 : b;
    }

    //-------------------------------------------------------------------------

    static float SmoothStep01(
        float value )
    {
        float const t =
            Clamp01(
                value );

        return t *
               t *
               ( 3.0f -
                 2.0f *
                     t );
    }

    //-------------------------------------------------------------------------

    static float LerpFloat(
        float a,
        float b,
        float t )
    {
        return a +
               ( b -
                 a ) *
                   t;
    }

    //-------------------------------------------------------------------------
    // P3C.9B - conserved Soil redistribution cache
    //
    // This coarse 2 m carrier is a CERTIFICATION grid only. It reuses samples
    // that are already evaluated by the 0.25 m geological preview rebuild, so
    // it adds zero steady-state geological evaluations per frame and no extra
    // EvaluateSurfacePoint() calls during rebuild.
    //
    // Granite supplies generic retention traits here, but SoilGeometry sees
    // only SoilRetentionSample. The Soil solver therefore remains substrate-
    // agnostic for the later multi-material breach work.
    //-------------------------------------------------------------------------

    static constexpr int32_t s_p3c9bCellsX =
        16;

    static constexpr int32_t s_p3c9bCellsY =
        8;

    static constexpr int32_t s_p3c9bCellCount =
        s_p3c9bCellsX *
        s_p3c9bCellsY;

    static constexpr float s_p3c9bCellSizeM =
        2.0f;

    struct P3C9BSubstrateSourceSample
    {
        bool m_valid =
            false;

        float m_worldX =
            0.0f;

        float m_worldY =
            0.0f;

        float m_bedrockZM =
            0.0f;

        float m_protrusionReliefM =
            0.0f;

        float m_recessDepthM =
            0.0f;

        float m_jointInfluence =
            0.0f;
    };

    static P3C9BSubstrateSourceSample s_p3c9bSource[s_p3c9bCellCount];

    static SoilRetentionSample s_p3c9bRetention[s_p3c9bCellCount];

    static SoilRedistributionCell s_p3c9bCells[s_p3c9bCellCount];

    static SoilMantleAuthorityResult      s_p3c9bAuthority;
    static SoilMantleRedistributionResult s_p3c9bRedistribution;

    // P3C.9C uses the exact same captured generic substrate field, but with a
    // deliberately thinner conserved Soil body so local cover stability can
    // fail and produce real substrate breaches.
    static SoilMantleAuthorityResult      s_p3c9cAuthority;
    static SoilMantleRedistributionResult s_p3c9cRedistribution;
    static SoilRedistributionCell         s_p3c9cContinuousCells[s_p3c9bCellCount];
    static SoilPatchCell                  s_p3c9cPatchCells[s_p3c9bCellCount];
    static SoilMantleBreachResult         s_p3c9cBreach;

    //-------------------------------------------------------------------------
    // P3C.9C-4 - visible conserved Soil geometry cache
    //-------------------------------------------------------------------------

    static constexpr int32_t s_p3c9c4SubcellsPerParent =
        8;

    static constexpr int32_t s_p3c9c4FineCellsX =
        s_p3c9bCellsX *
        s_p3c9c4SubcellsPerParent;

    static constexpr int32_t s_p3c9c4FineCellsY =
        s_p3c9bCellsY *
        s_p3c9c4SubcellsPerParent;

    static constexpr int32_t s_p3c9c4FineCellCount =
        s_p3c9c4FineCellsX *
        s_p3c9c4FineCellsY;

    static constexpr int32_t s_p3c9c4SourceVerticesX =
        s_p3c9c4FineCellsX + 1;

    static constexpr int32_t s_p3c9c4SourceVerticesY =
        s_p3c9c4FineCellsY + 1;

    static constexpr int32_t s_p3c9c4SourceVertexCount =
        s_p3c9c4SourceVerticesX *
        s_p3c9c4SourceVerticesY;

    static constexpr float s_p3c9c4FineCellSizeM =
        0.25f;

    static constexpr int32_t s_p3c9c4MaxContactSegments =
        17000;

    struct P3C9C4SourceVertex
    {
        bool m_valid = false;

        float m_worldX = 0.0f;
        float m_worldY = 0.0f;

        float m_bedrockZM = 0.0f;
        float m_protrusionReliefM = 0.0f;
        float m_recessDepthM = 0.0f;
        float m_jointInfluence = 0.0f;
    };

    static P3C9C4SourceVertex s_p3c9c4SourceVertices[s_p3c9c4SourceVertexCount];

    //-------------------------------------------------------------------------
    // P3C.9C-4E — support-conforming visible carrier
    //
    // The conserved Soil matter still lives in 0.25 m SoilGeometrySubcells.
    // These vertices are presentation-only samples reconstructed on the exact
    // 0.25 m geological preview lattice used by the Granite substrate.
    //
    // This prevents the visible Soil mesh from bridging a crease simply
    // because its old render vertices were cell centers averaged across four
    // substrate corners.
    //-------------------------------------------------------------------------

    struct P3C9C4SoilCarrierVertex
    {
        bool m_valid = false;

        float m_worldX = 0.0f;
        float m_worldY = 0.0f;

        float m_substrateZ = 0.0f;
        float m_soilThickness = 0.0f;

        float m_contact = -1.0f;
    };

    static P3C9C4SoilCarrierVertex s_p3c9c4CarrierVertices[s_p3c9c4SourceVertexCount];

    static SoilRetentionSample s_p3c9c4FineRetention[s_p3c9c4FineCellCount];

    static SoilGeometrySubcell s_p3c9c4Subcells[s_p3c9c4FineCellCount];

    static SoilParentGeometryReceipt s_p3c9c4ParentReceipts[s_p3c9bCellCount];

    static SoilContactSegment s_p3c9c4ContactSegments[s_p3c9c4MaxContactSegments];

    static SoilVisibleGeometryResult s_p3c9c4VisibleGeometry;

    static int32_t s_p3c9c4NumContactSegments =
        0;

    static bool s_p3c9c4VisibleGeometryPass =
        false;

    static uint32_t s_p3c9bSeed =
        0u;

    static bool s_p3c9bGeometryResponsePass =
        false;

    static bool s_p3c9cBreachResponsePass =
        false;

    //-------------------------------------------------------------------------

    static int32_t P3C9BCellIndex(
        int32_t x,
        int32_t y )
    {
        EE_ASSERT(
            x >= 0 &&
            x < s_p3c9bCellsX );

        EE_ASSERT(
            y >= 0 &&
            y < s_p3c9bCellsY );

        return y * s_p3c9bCellsX +
               x;
    }

    //-------------------------------------------------------------------------

    static P3C9BSubstrateSourceSample const& GetP3C9BSourceClamped(
        int32_t x,
        int32_t y )
    {
        if ( x < 0 )
        {
            x = 0;
        }
        else if ( x >= s_p3c9bCellsX )
        {
            x = s_p3c9bCellsX - 1;
        }

        if ( y < 0 )
        {
            y = 0;
        }
        else if ( y >= s_p3c9bCellsY )
        {
            y = s_p3c9bCellsY - 1;
        }

        return s_p3c9bSource[P3C9BCellIndex(
            x,
            y )];
    }

    //-------------------------------------------------------------------------

    static void ResetP3C9BRedistributionCache()
    {
        for ( int32_t i = 0;
              i < s_p3c9bCellCount;
              ++i )
        {
            s_p3c9bSource[i] =
                P3C9BSubstrateSourceSample();

            s_p3c9bRetention[i] =
                SoilRetentionSample();

            s_p3c9bCells[i] =
                SoilRedistributionCell();

            s_p3c9cContinuousCells[i] =
                SoilRedistributionCell();

            s_p3c9cPatchCells[i] =
                SoilPatchCell();
        }

        for ( int32_t i = 0;
              i < s_p3c9c4SourceVertexCount;
              ++i )
        {
            s_p3c9c4SourceVertices[i] =
                P3C9C4SourceVertex();

            s_p3c9c4CarrierVertices[i] =
                P3C9C4SoilCarrierVertex();
        }

        for ( int32_t i = 0;
              i < s_p3c9c4FineCellCount;
              ++i )
        {
            s_p3c9c4FineRetention[i] =
                SoilRetentionSample();

            s_p3c9c4Subcells[i] =
                SoilGeometrySubcell();
        }

        for ( int32_t i = 0;
              i < s_p3c9bCellCount;
              ++i )
        {
            s_p3c9c4ParentReceipts[i] =
                SoilParentGeometryReceipt();
        }

        for ( int32_t i = 0;
              i < s_p3c9c4MaxContactSegments;
              ++i )
        {
            s_p3c9c4ContactSegments[i] =
                SoilContactSegment();
        }

        s_p3c9c4VisibleGeometry =
            SoilVisibleGeometryResult();

        s_p3c9c4NumContactSegments =
            0;

        s_p3c9c4VisibleGeometryPass =
            false;

        s_p3c9bAuthority =
            SoilMantleAuthorityResult();

        s_p3c9bRedistribution =
            SoilMantleRedistributionResult();

        s_p3c9cAuthority =
            SoilMantleAuthorityResult();

        s_p3c9cRedistribution =
            SoilMantleRedistributionResult();

        s_p3c9cBreach =
            SoilMantleBreachResult();

        s_p3c9bSeed =
            0u;

        s_p3c9bGeometryResponsePass =
            false;

        s_p3c9cBreachResponsePass =
            false;

        s_p3c9c4VisibleGeometryPass =
            false;
    }

    //-------------------------------------------------------------------------

    static void CaptureP3C9BSourceSample(
        int32_t                      sampleX,
        int32_t                      sampleY,
        float                        worldX,
        float                        worldY,
        EvaluatedSurfacePoint const& evaluated )
    {
        // 0.25 m preview samples; a 2 m Soil cell is eight preview intervals.
        // Cell centers therefore land at 1 m + N*2 m -> indices 4 + N*8.
        if ( sampleX < 4 ||
             sampleY < 4 ||
             ( sampleX - 4 ) % 8 != 0 ||
             ( sampleY - 4 ) % 8 != 0 )
        {
            return;
        }

        int32_t const cellX =
            ( sampleX - 4 ) /
            8;

        int32_t const cellY =
            ( sampleY - 4 ) /
            8;

        if ( cellX < 0 ||
             cellX >= s_p3c9bCellsX ||
             cellY < 0 ||
             cellY >= s_p3c9bCellsY )
        {
            return;
        }

        P3C9BSubstrateSourceSample& source =
            s_p3c9bSource[P3C9BCellIndex(
                cellX,
                cellY )];

        source.m_valid =
            true;

        source.m_worldX =
            worldX;

        source.m_worldY =
            worldY;

        source.m_bedrockZM =
            evaluated.m_bedrockElevation;

        float protrusionRelief =
            MaxFloat(
                evaluated.m_graniteJointField.m_rootedRockHeadReliefM,
                evaluated.m_graniteJointField.m_rockHeadReliefM );

        protrusionRelief =
            MaxFloat(
                protrusionRelief,
                evaluated.m_graniteJointField.m_slabShoulderReliefM *
                    0.80f );

        protrusionRelief =
            MaxFloat(
                protrusionRelief,
                evaluated.m_graniteJointField.m_broadWeatheredBackReliefM *
                    0.55f );

        source.m_protrusionReliefM =
            MaxFloat(
                protrusionRelief,
                0.0f );

        float recessDepth =
            MaxFloat(
                -evaluated.m_granitePrimaryJointRecess,
                0.0f ) +
            MaxFloat(
                -evaluated.m_graniteSecondaryJointRecess,
                0.0f );

        recessDepth =
            MaxFloat(
                recessDepth,
                MaxFloat(
                    -evaluated.m_graniteJointField.m_primaryFormationJointRecessM,
                    0.0f ) +
                    MaxFloat(
                        -evaluated.m_graniteJointField.m_secondaryFormationJointRecessM,
                        0.0f ) );

        source.m_recessDepthM =
            recessDepth;

        source.m_jointInfluence =
            Clamp01(
                evaluated.m_graniteJointInfluence );
    }

    //-------------------------------------------------------------------------
    // P3C.9C-4 fine substrate source capture
    //
    // Every preview geological evaluation is captured once. Fine 0.25 m Soil
    // cells later reconstruct their center substrate sample by averaging the
    // four surrounding preview vertices.
    //-------------------------------------------------------------------------

    static void CaptureP3C9C4SourceVertex(
        int32_t                      sampleX,
        int32_t                      sampleY,
        float                        worldX,
        float                        worldY,
        EvaluatedSurfacePoint const& evaluated )
    {
        if ( sampleX < 0 ||
             sampleX >= s_p3c9c4SourceVerticesX ||
             sampleY < 0 ||
             sampleY >= s_p3c9c4SourceVerticesY )
        {
            return;
        }

        int32_t const index =
            sampleY *
                s_p3c9c4SourceVerticesX +
            sampleX;

        P3C9C4SourceVertex& source =
            s_p3c9c4SourceVertices[index];

        source.m_valid =
            true;

        source.m_worldX =
            worldX;

        source.m_worldY =
            worldY;

        source.m_bedrockZM =
            evaluated.m_bedrockElevation;

        float protrusionRelief =
            MaxFloat(
                evaluated.m_graniteJointField.m_rootedRockHeadReliefM,
                evaluated.m_graniteJointField.m_rockHeadReliefM );

        protrusionRelief =
            MaxFloat(
                protrusionRelief,
                evaluated.m_graniteJointField.m_slabShoulderReliefM *
                    0.80f );

        protrusionRelief =
            MaxFloat(
                protrusionRelief,
                evaluated.m_graniteJointField.m_broadWeatheredBackReliefM *
                    0.55f );

        source.m_protrusionReliefM =
            MaxFloat(
                protrusionRelief,
                0.0f );

        float recessDepth =
            MaxFloat(
                -evaluated.m_granitePrimaryJointRecess,
                0.0f ) +
            MaxFloat(
                -evaluated.m_graniteSecondaryJointRecess,
                0.0f );

        recessDepth =
            MaxFloat(
                recessDepth,
                MaxFloat(
                    -evaluated.m_graniteJointField.m_primaryFormationJointRecessM,
                    0.0f ) +
                    MaxFloat(
                        -evaluated.m_graniteJointField.m_secondaryFormationJointRecessM,
                        0.0f ) );

        source.m_recessDepthM =
            recessDepth;

        source.m_jointInfluence =
            Clamp01(
                evaluated.m_graniteJointInfluence );
    }

    //-------------------------------------------------------------------------

    static int32_t P3C9C4FineCellIndex(
        int32_t x,
        int32_t y )
    {
        EE_ASSERT(
            x >= 0 &&
            x < s_p3c9c4FineCellsX );

        EE_ASSERT(
            y >= 0 &&
            y < s_p3c9c4FineCellsY );

        return y *
                   s_p3c9c4FineCellsX +
               x;
    }

    //-------------------------------------------------------------------------

    static SoilRetentionSample const& GetP3C9C4FineRetentionClamped(
        int32_t x,
        int32_t y )
    {
        if ( x < 0 )
        {
            x = 0;
        }
        else if ( x >= s_p3c9c4FineCellsX )
        {
            x = s_p3c9c4FineCellsX - 1;
        }

        if ( y < 0 )
        {
            y = 0;
        }
        else if ( y >= s_p3c9c4FineCellsY )
        {
            y = s_p3c9c4FineCellsY - 1;
        }

        return s_p3c9c4FineRetention[P3C9C4FineCellIndex(
            x,
            y )];
    }

    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // P3C.9C-4E — build support-conforming presentation vertices
    //
    // Fine Soil cells own conserved volume. The visible mesh is reconstructed
    // onto the exact substrate preview-vertex lattice:
    //
    //     substrate Z = cached geological preview vertex
    //     Soil thickness = weighted reconstruction from adjacent Soil cells
    //     contact = weighted signed contact from adjacent Soil cells
    //
    // No geological re-evaluation occurs here.
    //-------------------------------------------------------------------------

    static void BuildP3C9C4SoilCarrierVertices()
    {
        for ( int32_t vertexY = 0;
              vertexY < s_p3c9c4SourceVerticesY;
              ++vertexY )
        {
            for ( int32_t vertexX = 0;
                  vertexX < s_p3c9c4SourceVerticesX;
                  ++vertexX )
            {
                int32_t const vertexIndex =
                    vertexY *
                        s_p3c9c4SourceVerticesX +
                    vertexX;

                P3C9C4SourceVertex const& source =
                    s_p3c9c4SourceVertices[vertexIndex];

                P3C9C4SoilCarrierVertex& carrier =
                    s_p3c9c4CarrierVertices[vertexIndex];

                carrier =
                    P3C9C4SoilCarrierVertex();

                if ( !source.m_valid )
                {
                    continue;
                }

                carrier.m_valid =
                    true;

                carrier.m_worldX =
                    source.m_worldX;

                carrier.m_worldY =
                    source.m_worldY;

                carrier.m_substrateZ =
                    source.m_bedrockZM;

                double contactSum =
                    0.0;

                double contactWeightSum =
                    0.0;

                double thicknessWeightedSum =
                    0.0;

                double thicknessWeightSum =
                    0.0;

                auto accumulateSubcell =
                    [&]( int32_t cellX,
                         int32_t cellY )
                {
                    if ( cellX < 0 ||
                         cellX >= s_p3c9c4FineCellsX ||
                         cellY < 0 ||
                         cellY >= s_p3c9c4FineCellsY )
                    {
                        return;
                    }

                    SoilGeometrySubcell const& subcell =
                        s_p3c9c4Subcells[P3C9C4FineCellIndex(
                            cellX,
                            cellY )];

                    // All four adjacent fine cells contribute equally to
                    // the signed topology field at the shared substrate
                    // vertex. This removes parent/accounting orientation
                    // from the visible carrier.
                    contactSum +=
                        double(
                            subcell.m_contactField );

                    contactWeightSum +=
                        1.0;

                    if ( subcell.m_occupied &&
                         subcell.m_thicknessZM >
                             0.0f )
                    {
                        // Stronger retained Soil contributes slightly more
                        // to the shared vertex thickness, but this remains
                        // presentation reconstruction only. Conserved
                        // quantity stays in the subcells.
                        float const weight =
                            MaxFloat(
                                subcell.m_allocationWeight,
                                0.05f );

                        thicknessWeightedSum +=
                            double(
                                subcell.m_thicknessZM ) *
                            double(
                                weight );

                        thicknessWeightSum +=
                            double(
                                weight );
                    }
                };

                // A preview vertex touches up to four fine accounting cells.
                accumulateSubcell(
                    vertexX - 1,
                    vertexY - 1 );

                accumulateSubcell(
                    vertexX,
                    vertexY - 1 );

                accumulateSubcell(
                    vertexX - 1,
                    vertexY );

                accumulateSubcell(
                    vertexX,
                    vertexY );

                if ( contactWeightSum <=
                     0.0 )
                {
                    carrier.m_contact =
                        -1.0f;

                    carrier.m_soilThickness =
                        0.0f;

                    continue;
                }

                carrier.m_contact =
                    float(
                        contactSum /
                        contactWeightSum );

                if ( carrier.m_contact >
                         0.0f &&
                     thicknessWeightSum >
                         0.0 )
                {
                    carrier.m_soilThickness =
                        float(
                            thicknessWeightedSum /
                            thicknessWeightSum );
                }
                else
                {
                    carrier.m_soilThickness =
                        0.0f;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    static void FinalizeP3C9C4VisibleGeometry(
        float originWorldX,
        float originWorldY )
    {
        //-------------------------------------------------------------------------
        // Pass 1: reconstruct fine-cell center samples from four already-cached
        // geological vertices.
        //-------------------------------------------------------------------------

        for ( int32_t y = 0;
              y < s_p3c9c4FineCellsY;
              ++y )
        {
            for ( int32_t x = 0;
                  x < s_p3c9c4FineCellsX;
                  ++x )
            {
                int32_t const v00 =
                    y *
                        s_p3c9c4SourceVerticesX +
                    x;

                int32_t const v10 =
                    v00 +
                    1;

                int32_t const v01 =
                    v00 +
                    s_p3c9c4SourceVerticesX;

                int32_t const v11 =
                    v01 +
                    1;

                P3C9C4SourceVertex const& a =
                    s_p3c9c4SourceVertices[v00];

                P3C9C4SourceVertex const& b =
                    s_p3c9c4SourceVertices[v10];

                P3C9C4SourceVertex const& c =
                    s_p3c9c4SourceVertices[v01];

                P3C9C4SourceVertex const& d =
                    s_p3c9c4SourceVertices[v11];

                EE_ASSERT(
                    a.m_valid &&
                    b.m_valid &&
                    c.m_valid &&
                    d.m_valid );

                SoilRetentionSample& retention =
                    s_p3c9c4FineRetention[P3C9C4FineCellIndex(
                        x,
                        y )];

                retention =
                    SoilRetentionSample();

                retention.m_substrateMaterial =
                    ProvenanceMaterialID::Granite;

                retention.m_worldX =
                    originWorldX +
                    ( float( x ) +
                      0.5f ) *
                        s_p3c9c4FineCellSizeM;

                retention.m_worldY =
                    originWorldY +
                    ( float( y ) +
                      0.5f ) *
                        s_p3c9c4FineCellSizeM;

                retention.m_substrateSurfaceZM =
                    ( a.m_bedrockZM +
                      b.m_bedrockZM +
                      c.m_bedrockZM +
                      d.m_bedrockZM ) *
                    0.25f;

                retention.m_recessDepthM =
                    ( a.m_recessDepthM +
                      b.m_recessDepthM +
                      c.m_recessDepthM +
                      d.m_recessDepthM ) *
                    0.25f;

                retention.m_protrusionInfluence =
                    Clamp01(
                        (
                            a.m_protrusionReliefM +
                            b.m_protrusionReliefM +
                            c.m_protrusionReliefM +
                            d.m_protrusionReliefM ) *
                        0.25f /
                        0.95f );

                // Temporarily store mean joint response in banking. Pass 2
                // converts it into the final base-banking signal.
                retention.m_obstacleBankingInfluence =
                    Clamp01(
                        (
                            a.m_jointInfluence +
                            b.m_jointInfluence +
                            c.m_jointInfluence +
                            d.m_jointInfluence ) *
                        0.25f );
            }
        }

        //-------------------------------------------------------------------------
        // Pass 2: local fine-scale slope / convexity / retention geometry.
        //-------------------------------------------------------------------------

        for ( int32_t y = 0;
              y < s_p3c9c4FineCellsY;
              ++y )
        {
            for ( int32_t x = 0;
                  x < s_p3c9c4FineCellsX;
                  ++x )
            {
                SoilRetentionSample& center =
                    s_p3c9c4FineRetention[P3C9C4FineCellIndex(
                        x,
                        y )];

                SoilRetentionSample const& left =
                    GetP3C9C4FineRetentionClamped(
                        x - 1,
                        y );

                SoilRetentionSample const& right =
                    GetP3C9C4FineRetentionClamped(
                        x + 1,
                        y );

                SoilRetentionSample const& down =
                    GetP3C9C4FineRetentionClamped(
                        x,
                        y - 1 );

                SoilRetentionSample const& up =
                    GetP3C9C4FineRetentionClamped(
                        x,
                        y + 1 );

                float const spanX =
                    x > 0 &&
                            x < s_p3c9c4FineCellsX - 1
                        ? s_p3c9c4FineCellSizeM * 2.0f
                        : s_p3c9c4FineCellSizeM;

                float const spanY =
                    y > 0 &&
                            y < s_p3c9c4FineCellsY - 1
                        ? s_p3c9c4FineCellSizeM * 2.0f
                        : s_p3c9c4FineCellSizeM;

                float const deltaZX =
                    x > 0 &&
                            x < s_p3c9c4FineCellsX - 1
                        ? right.m_substrateSurfaceZM -
                              left.m_substrateSurfaceZM
                    : x == 0
                        ? right.m_substrateSurfaceZM -
                              center.m_substrateSurfaceZM
                        : center.m_substrateSurfaceZM -
                              left.m_substrateSurfaceZM;

                float const deltaZY =
                    y > 0 &&
                            y < s_p3c9c4FineCellsY - 1
                        ? up.m_substrateSurfaceZM -
                              down.m_substrateSurfaceZM
                    : y == 0
                        ? up.m_substrateSurfaceZM -
                              center.m_substrateSurfaceZM
                        : center.m_substrateSurfaceZM -
                              down.m_substrateSurfaceZM;

                float const slopeX =
                    deltaZX /
                    spanX;

                float const slopeY =
                    deltaZY /
                    spanY;

                float const slopeMagnitude =
                    float(
                        std::sqrt(
                            double(
                                slopeX * slopeX +
                                slopeY * slopeY ) ) );

                float const neighborMeanZ =
                    ( left.m_substrateSurfaceZM +
                      right.m_substrateSurfaceZM +
                      down.m_substrateSurfaceZM +
                      up.m_substrateSurfaceZM ) *
                    0.25f;

                float const convexHeight =
                    MaxFloat(
                        center.m_substrateSurfaceZM -
                            neighborMeanZ,
                        0.0f );

                float const concaveDepth =
                    MaxFloat(
                        neighborMeanZ -
                            center.m_substrateSurfaceZM,
                        0.0f );

                float const centerProtrusion =
                    center.m_protrusionInfluence;

                float maxNeighborProtrusion =
                    left.m_protrusionInfluence;

                maxNeighborProtrusion =
                    MaxFloat(
                        maxNeighborProtrusion,
                        right.m_protrusionInfluence );

                maxNeighborProtrusion =
                    MaxFloat(
                        maxNeighborProtrusion,
                        down.m_protrusionInfluence );

                maxNeighborProtrusion =
                    MaxFloat(
                        maxNeighborProtrusion,
                        up.m_protrusionInfluence );

                float const adjacentHigherBody =
                    MaxFloat(
                        maxNeighborProtrusion -
                            centerProtrusion,
                        0.0f );

                float const jointInfluence =
                    center.m_obstacleBankingInfluence;

                center.m_slope =
                    Clamp01(
                        slopeMagnitude /
                        0.85f );

                center.m_convexHighInfluence =
                    Clamp01(
                        convexHeight /
                            0.075f +
                        centerProtrusion *
                            0.70f );

                center.m_concavityInfluence =
                    Clamp01(
                        concaveDepth /
                        0.070f );

                center.m_obstacleBankingInfluence =
                    Clamp01(
                        adjacentHigherBody *
                            0.72f +
                        center.m_concavityInfluence *
                            0.52f +
                        jointInfluence *
                            0.18f );
            }
        }

        SoilMantleDistributionGrid parentGrid;
        parentGrid.m_minWorldX =
            originWorldX;
        parentGrid.m_minWorldY =
            originWorldY;
        parentGrid.m_numCellsX =
            s_p3c9bCellsX;
        parentGrid.m_numCellsY =
            s_p3c9bCellsY;
        parentGrid.m_cellSizeX =
            s_p3c9bCellSizeM;
        parentGrid.m_cellSizeY =
            s_p3c9bCellSizeM;

        SoilGeometryGrid geometryGrid;
        geometryGrid.m_minWorldX =
            originWorldX;
        geometryGrid.m_minWorldY =
            originWorldY;
        geometryGrid.m_numCellsX =
            s_p3c9c4FineCellsX;
        geometryGrid.m_numCellsY =
            s_p3c9c4FineCellsY;
        geometryGrid.m_cellSizeX =
            s_p3c9c4FineCellSizeM;
        geometryGrid.m_cellSizeY =
            s_p3c9c4FineCellSizeM;
        geometryGrid.m_subcellsPerParentX =
            s_p3c9c4SubcellsPerParent;
        geometryGrid.m_subcellsPerParentY =
            s_p3c9c4SubcellsPerParent;

        SoilVisibleGeometryPolicy geometryPolicy;
        geometryPolicy.m_residualEdgeTaperWidthM =
            0.55f;
        geometryPolicy.m_pocketCenterBias =
            0.42f;
        geometryPolicy.m_maxClodBulkVolumeFraction =
            0.0f;
        geometryPolicy.m_minimumVisibleThicknessM =
            0.001f;

        s_p3c9c4VisibleGeometry =
            BuildVisibleConservedSoilGeometry(
                s_p3c9cAuthority,
                parentGrid,
                s_p3c9cPatchCells,
                s_p3c9bCellCount,
                geometryGrid,
                s_p3c9c4FineRetention,
                s_p3c9c4FineCellCount,
                geometryPolicy,
                s_p3c9c4Subcells,
                s_p3c9c4FineCellCount,
                s_p3c9c4ParentReceipts,
                s_p3c9bCellCount,
                nullptr,
                0 );

        EE_ASSERT(
            s_p3c9c4VisibleGeometry.m_valid );

        s_p3c9c4NumContactSegments =
            ExtractSoilContactSegments(
                geometryGrid,
                s_p3c9c4Subcells,
                s_p3c9c4FineCellCount,
                s_p3c9c4ContactSegments,
                s_p3c9c4MaxContactSegments );

        BuildP3C9C4SoilCarrierVertices();

        s_p3c9c4VisibleGeometryPass =
            s_p3c9c4VisibleGeometry.m_valid &&
            s_p3c9c4VisibleGeometry.m_numOccupiedFineCells >
                0 &&
            s_p3c9c4VisibleGeometry.m_numOccupiedFineCells <
                s_p3c9c4VisibleGeometry.m_numFineCells &&
            s_p3c9c4VisibleGeometry.m_numResidualPatchSubcells >
                0 &&
            s_p3c9c4VisibleGeometry.m_numPocketSubcells >
                0 &&
            s_p3c9c4NumContactSegments >
                0;

        EE_ASSERT(
            s_p3c9c4VisibleGeometryPass );
    }

    //-------------------------------------------------------------------------

    static void FinalizeP3C9BRedistributionCache(
        uint32_t seed,
        float    originWorldX,
        float    originWorldY )
    {
        for ( int32_t y = 0;
              y < s_p3c9bCellsY;
              ++y )
        {
            for ( int32_t x = 0;
                  x < s_p3c9bCellsX;
                  ++x )
            {
                int32_t const index =
                    P3C9BCellIndex(
                        x,
                        y );

                P3C9BSubstrateSourceSample const& center =
                    s_p3c9bSource[index];

                EE_ASSERT(
                    center.m_valid );

                P3C9BSubstrateSourceSample const& left =
                    GetP3C9BSourceClamped(
                        x - 1,
                        y );

                P3C9BSubstrateSourceSample const& right =
                    GetP3C9BSourceClamped(
                        x + 1,
                        y );

                P3C9BSubstrateSourceSample const& down =
                    GetP3C9BSourceClamped(
                        x,
                        y - 1 );

                P3C9BSubstrateSourceSample const& up =
                    GetP3C9BSourceClamped(
                        x,
                        y + 1 );

                float const derivativeSpanX =
                    x > 0 &&
                            x < s_p3c9bCellsX - 1
                        ? s_p3c9bCellSizeM * 2.0f
                        : s_p3c9bCellSizeM;

                float const derivativeSpanY =
                    y > 0 &&
                            y < s_p3c9bCellsY - 1
                        ? s_p3c9bCellSizeM * 2.0f
                        : s_p3c9bCellSizeM;

                float const deltaZX =
                    x > 0 &&
                            x < s_p3c9bCellsX - 1
                        ? right.m_bedrockZM - left.m_bedrockZM
                    : x == 0
                        ? right.m_bedrockZM - center.m_bedrockZM
                        : center.m_bedrockZM - left.m_bedrockZM;

                float const deltaZY =
                    y > 0 &&
                            y < s_p3c9bCellsY - 1
                        ? up.m_bedrockZM - down.m_bedrockZM
                    : y == 0
                        ? up.m_bedrockZM - center.m_bedrockZM
                        : center.m_bedrockZM - down.m_bedrockZM;

                float const slopeX =
                    deltaZX /
                    derivativeSpanX;

                float const slopeY =
                    deltaZY /
                    derivativeSpanY;

                float const slopeMagnitude =
                    float(
                        std::sqrt(
                            double(
                                slopeX * slopeX +
                                slopeY * slopeY ) ) );

                float const neighborMeanZ =
                    ( left.m_bedrockZM +
                      right.m_bedrockZM +
                      down.m_bedrockZM +
                      up.m_bedrockZM ) *
                    0.25f;

                float const convexHeight =
                    MaxFloat(
                        center.m_bedrockZM -
                            neighborMeanZ,
                        0.0f );

                float const concaveDepth =
                    MaxFloat(
                        neighborMeanZ -
                            center.m_bedrockZM,
                        0.0f );

                float maxNeighborProtrusion =
                    left.m_protrusionReliefM;

                maxNeighborProtrusion =
                    MaxFloat(
                        maxNeighborProtrusion,
                        right.m_protrusionReliefM );

                maxNeighborProtrusion =
                    MaxFloat(
                        maxNeighborProtrusion,
                        down.m_protrusionReliefM );

                maxNeighborProtrusion =
                    MaxFloat(
                        maxNeighborProtrusion,
                        up.m_protrusionReliefM );

                float const adjacentHigherBody =
                    MaxFloat(
                        maxNeighborProtrusion -
                            center.m_protrusionReliefM,
                        0.0f );

                SoilRetentionSample& retention =
                    s_p3c9bRetention[index];

                retention.m_substrateMaterial =
                    ProvenanceMaterialID::Granite;

                retention.m_worldX =
                    center.m_worldX;

                retention.m_worldY =
                    center.m_worldY;

                retention.m_substrateSurfaceZM =
                    center.m_bedrockZM;

                retention.m_slope =
                    Clamp01(
                        slopeMagnitude /
                        0.85f );

                retention.m_convexHighInfluence =
                    Clamp01(
                        convexHeight /
                            0.36f +
                        center.m_protrusionReliefM /
                            1.80f );

                retention.m_concavityInfluence =
                    Clamp01(
                        concaveDepth /
                        0.32f );

                retention.m_recessDepthM =
                    center.m_recessDepthM;

                retention.m_obstacleBankingInfluence =
                    Clamp01(
                        adjacentHigherBody /
                            0.55f +
                        retention.m_concavityInfluence *
                            0.45f +
                        center.m_jointInfluence *
                            0.12f );

                retention.m_protrusionInfluence =
                    Clamp01(
                        center.m_protrusionReliefM /
                        0.95f );
            }
        }

        // About 8 cm mean settled Soil over the full 32x16 m certification
        // domain. The exact mass is chosen from the already-certified matter
        // law, not from a display thickness:
        //
        //     512 m2 * 0.08 m * 0.60 * 2650 kg/m3 = 65126.4 kg
        SoilMantleBody mantle;
        mantle.m_bodyID =
            9101u;
        mantle.m_provenanceID =
            99101u;
        mantle.m_seed =
            seed;
        mantle.m_massGrams =
            65126400ull;
        mantle.m_physicalState =
            SoilPhysicalState::Settled;
        mantle.m_distributionState =
            SoilDistributionState::ContinuousMantle;

        SoilMantleDomain domain;
        domain.m_minX =
            originWorldX;
        domain.m_maxX =
            originWorldX +
            float( s_p3c9bCellsX ) *
                s_p3c9bCellSizeM;
        domain.m_minY =
            originWorldY;
        domain.m_maxY =
            originWorldY +
            float( s_p3c9bCellsY ) *
                s_p3c9bCellSizeM;

        s_p3c9bAuthority =
            CalculateSoilMantleAuthority(
                mantle,
                domain );

        EE_ASSERT(
            s_p3c9bAuthority.m_conservationPass );

        SoilMantleDistributionGrid grid;
        grid.m_minWorldX =
            originWorldX;
        grid.m_minWorldY =
            originWorldY;
        grid.m_numCellsX =
            s_p3c9bCellsX;
        grid.m_numCellsY =
            s_p3c9bCellsY;
        grid.m_cellSizeX =
            s_p3c9bCellSizeM;
        grid.m_cellSizeY =
            s_p3c9bCellSizeM;

        s_p3c9bRedistribution =
            RedistributeContinuousSoilMantle(
                s_p3c9bAuthority,
                grid,
                s_p3c9bRetention,
                s_p3c9bCellCount,
                s_p3c9bCells,
                s_p3c9bCellCount );

        EE_ASSERT(
            s_p3c9bRedistribution.m_valid );

        s_p3c9bGeometryResponsePass =
            s_p3c9bRedistribution.m_numConvexHighCells > 0 &&
            s_p3c9bRedistribution.m_numRetentionLowCells > 0 &&
            s_p3c9bRedistribution.m_meanRetentionLowThicknessZM >
                s_p3c9bRedistribution.m_meanConvexHighThicknessZM;

        //---------------------------------------------------------------------
        // P3C.9C - conserved breach / residual patch fixture
        //
        // Same 512 m2 domain and same generic substrate samples, but only
        // enough Settled Soil for a 3 cm uniform mantle before redistribution:
        //
        //     512 m2 * 0.03 m * 0.60 * 2650 kg/m3
        //         = 24422.4 kg
        //
        // Coverage is NOT prescribed. Local stability decides which cells
        // fail; evacuated Soil volume is transferred into survivors.
        //---------------------------------------------------------------------

        SoilMantleBody thinMantle;
        thinMantle.m_bodyID =
            9201u;
        thinMantle.m_provenanceID =
            99201u;
        thinMantle.m_seed =
            seed;
        thinMantle.m_massGrams =
            24422400ull;
        thinMantle.m_physicalState =
            SoilPhysicalState::Settled;
        thinMantle.m_distributionState =
            SoilDistributionState::ContinuousMantle;

        s_p3c9cAuthority =
            CalculateSoilMantleAuthority(
                thinMantle,
                domain );

        EE_ASSERT(
            s_p3c9cAuthority.m_conservationPass );

        s_p3c9cRedistribution =
            RedistributeContinuousSoilMantle(
                s_p3c9cAuthority,
                grid,
                s_p3c9bRetention,
                s_p3c9bCellCount,
                s_p3c9cContinuousCells,
                s_p3c9bCellCount );

        EE_ASSERT(
            s_p3c9cRedistribution.m_valid );

        SoilBreachPolicy breachPolicy;
        breachPolicy.m_settledMinimumStableThicknessM =
            0.024f;
        breachPolicy.m_slopeInstabilityM =
            0.018f;
        breachPolicy.m_convexInstabilityM =
            0.032f;
        breachPolicy.m_protrusionInstabilityM =
            0.030f;
        breachPolicy.m_concavityStabilityM =
            0.018f;
        breachPolicy.m_recessStabilityPerMeter =
            0.100f;
        breachPolicy.m_bankingStabilityM =
            0.020f;
        breachPolicy.m_minimumRetainedPocketThicknessM =
            0.004f;
        breachPolicy.m_maxIterations =
            s_p3c9bCellCount - 1;

        s_p3c9cBreach =
            ResolveConservedSoilBreach(
                s_p3c9cAuthority,
                grid,
                s_p3c9bRetention,
                s_p3c9bCellCount,
                s_p3c9cContinuousCells,
                s_p3c9bCellCount,
                breachPolicy,
                s_p3c9cPatchCells,
                s_p3c9bCellCount );

        EE_ASSERT(
            s_p3c9cBreach.m_valid );

        s_p3c9cBreachResponsePass =
            s_p3c9cBreach.m_numBreachedCells > 0 &&
            s_p3c9cBreach.m_numOccupiedCells > 0 &&
            s_p3c9cBreach.m_coverageFraction > 0.0f &&
            s_p3c9cBreach.m_coverageFraction < 1.0f &&
            s_p3c9cBreach.m_evacuatedBulkVolumeM3 > 0.0f &&
            s_p3c9cBreach.m_numPatches > 0;

        EE_ASSERT(
            s_p3c9cBreachResponsePass );

        FinalizeP3C9C4VisibleGeometry(
            originWorldX,
            originWorldY );

        s_p3c9bSeed =
            seed;
    }

    //-------------------------------------------------------------------------
    // P3C.7P - derived preview cache generation
    //
    // The previous DebugDraw path repeatedly evaluated the same geological
    // world coordinates every frame. This method evaluates each unique 0.25 m
    // preview sample once per cache rebuild and also selects the two causal
    // exposure receipts while those full evaluations are already in hand.
    //-------------------------------------------------------------------------

    void ProvenanceWorldSystem::GenerateDebugPreviewCache(
        uint32_t seed )
    {
        m_cachedGraniteReceipt =
            DebugExposureReceipt();

        m_cachedSoilReceipt =
            DebugExposureReceipt();

        m_debugPreviewEvaluationCount =
            0;

        ResetP3C9BRedistributionCache();

        float bestGraniteScore =
            -1000000.0f;

        float bestSoilScore =
            1000000.0f;

        float const originWorldX =
            float(
                s_patchAOriginX ) *
            s_sampleSpacing;

        float const originWorldY =
            float(
                s_patchOriginY ) *
            s_sampleSpacing;

        for ( int32_t sampleY = 0;
              sampleY <
              s_debugPreviewSamplesY;
              ++sampleY )
        {
            float const worldY =
                originWorldY +
                float( sampleY ) *
                    s_debugPreviewSpacing;

            for ( int32_t sampleX = 0;
                  sampleX <
                  s_debugPreviewSamplesX;
                  ++sampleX )
            {
                float const worldX =
                    originWorldX +
                    float( sampleX ) *
                        s_debugPreviewSpacing;

                EvaluatedSurfacePoint const evaluated =
                    EvaluateSurfacePoint(
                        seed,
                        worldX,
                        worldY );

                ++m_debugPreviewEvaluationCount;

                CaptureP3C9BSourceSample(
                    sampleX,
                    sampleY,
                    worldX,
                    worldY,
                    evaluated );

                CaptureP3C9C4SourceVertex(
                    sampleX,
                    sampleY,
                    worldX,
                    worldY,
                    evaluated );

                int32_t const sampleIndex =
                    sampleY *
                        s_debugPreviewSamplesX +
                    sampleX;

                DebugSurfaceSample& cached =
                    m_debugPreviewSamples[sampleIndex];

                cached.m_elevation =
                    evaluated.m_elevation;

                cached.m_signedSoilDepth =
                    evaluated.m_signedSoilDepth;

                // Granite protrusion witness.
                if ( evaluated.m_material ==
                         ProvenanceMaterialID::Granite &&
                     evaluated.m_graniteStructuralRelief >
                         0.015f &&
                     evaluated.m_signedSoilDepth <
                         -0.003f )
                {
                    float const score =
                        evaluated.m_graniteStructuralRelief +
                        evaluated.m_graniteJointInfluence *
                            0.05f;

                    if ( score >
                         bestGraniteScore )
                    {
                        bestGraniteScore =
                            score;

                        m_cachedGraniteReceipt.m_found =
                            true;

                        m_cachedGraniteReceipt.m_x =
                            worldX;

                        m_cachedGraniteReceipt.m_y =
                            worldY;

                        m_cachedGraniteReceipt.m_graniteStructuralRelief =
                            evaluated.m_graniteStructuralRelief;

                        m_cachedGraniteReceipt.m_graniteWeatheringRelief =
                            evaluated.m_graniteWeatheringRelief;

                        m_cachedGraniteReceipt.m_graniteJointInfluence =
                            evaluated.m_graniteJointInfluence;

                        m_cachedGraniteReceipt.m_soilGraniteInfillResponse =
                            evaluated.m_soilGraniteInfillResponse;

                        m_cachedGraniteReceipt.m_soilThickness =
                            evaluated.m_soilThickness;

                        m_cachedGraniteReceipt.m_signedSoilDepth =
                            evaluated.m_signedSoilDepth;
                    }
                }

                // Thin Soil witness, preferring Granite recesses and joints.
                if ( evaluated.m_material ==
                         ProvenanceMaterialID::Soil &&
                     evaluated.m_soilThickness >
                         0.002f &&
                     evaluated.m_soilThickness <
                         0.18f )
                {
                    float score =
                        evaluated.m_soilThickness;

                    if ( evaluated.m_graniteStructuralRelief <
                         0.0f )
                    {
                        score *=
                            0.55f;
                    }

                    if ( evaluated.m_graniteJointInfluence >
                         0.30f )
                    {
                        score *=
                            0.70f;
                    }

                    if ( score <
                         bestSoilScore )
                    {
                        bestSoilScore =
                            score;

                        m_cachedSoilReceipt.m_found =
                            true;

                        m_cachedSoilReceipt.m_x =
                            worldX;

                        m_cachedSoilReceipt.m_y =
                            worldY;

                        m_cachedSoilReceipt.m_graniteStructuralRelief =
                            evaluated.m_graniteStructuralRelief;

                        m_cachedSoilReceipt.m_graniteWeatheringRelief =
                            evaluated.m_graniteWeatheringRelief;

                        m_cachedSoilReceipt.m_graniteJointInfluence =
                            evaluated.m_graniteJointInfluence;

                        m_cachedSoilReceipt.m_soilGraniteInfillResponse =
                            evaluated.m_soilGraniteInfillResponse;

                        m_cachedSoilReceipt.m_soilThickness =
                            evaluated.m_soilThickness;

                        m_cachedSoilReceipt.m_signedSoilDepth =
                            evaluated.m_signedSoilDepth;
                    }
                }
            }
        }

        FinalizeP3C9BRedistributionCache(
            seed,
            originWorldX,
            originWorldY );

        EE_ASSERT(
            m_debugPreviewEvaluationCount ==
            uint32_t(
                s_debugPreviewSampleCount ) );

        m_debugPreviewSeed =
            seed;

        m_hasGeneratedDebugPreview =
            true;
    }

    //-------------------------------------------------------------------------
    // P3C.7 presentation controls
    //-------------------------------------------------------------------------

    static constexpr bool s_showPackageSeam =
        true;

    static constexpr bool s_showGeologicalContact =
        true;

    static constexpr bool s_neutralMaterialDebug =
        false;

    static constexpr bool s_enableNormalLighting =
        true;

    //-------------------------------------------------------------------------
    // Diagnostic lighting
    //-------------------------------------------------------------------------

    static Color ApplyDiagnosticLighting(
        Color const& baseColor,
        float        ax,
        float        ay,
        float        az,
        float        bx,
        float        by,
        float        bz,
        float        cx,
        float        cy,
        float        cz )
    {
        if ( !s_enableNormalLighting )
        {
            return baseColor;
        }

        float const edge1X =
            bx -
            ax;

        float const edge1Y =
            by -
            ay;

        float const edge1Z =
            bz -
            az;

        float const edge2X =
            cx -
            ax;

        float const edge2Y =
            cy -
            ay;

        float const edge2Z =
            cz -
            az;

        float normalX =
            edge1Y *
                edge2Z -
            edge1Z *
                edge2Y;

        float normalY =
            edge1Z *
                edge2X -
            edge1X *
                edge2Z;

        float normalZ =
            edge1X *
                edge2Y -
            edge1Y *
                edge2X;

        float const normalLength =
            float(
                std::sqrt(
                    double(
                        normalX *
                            normalX +
                        normalY *
                            normalY +
                        normalZ *
                            normalZ ) ) );

        if ( normalLength <=
             0.000001f )
        {
            return baseColor;
        }

        normalX /=
            normalLength;

        normalY /=
            normalLength;

        normalZ /=
            normalLength;

        float constexpr sunX =
            0.35f;

        float constexpr sunY =
            -0.45f;

        float constexpr sunZ =
            0.82f;

        float diffuse =
            normalX *
                sunX +
            normalY *
                sunY +
            normalZ *
                sunZ;

        if ( diffuse <
             0.0f )
        {
            diffuse =
                0.0f;
        }

        diffuse =
            Clamp01(
                diffuse );

        float const intensity =
            0.38f +
            diffuse *
                0.62f;

        return baseColor.GetScaledColor(
            intensity );
    }

    //-------------------------------------------------------------------------
    // Colors
    //-------------------------------------------------------------------------

    static Color GetSurfaceBaseColor(
        ProvenanceMaterialID material )
    {
        if ( s_neutralMaterialDebug )
        {
            return Color(
                190,
                190,
                190 );
        }

        if ( material ==
             ProvenanceMaterialID::Soil )
        {
            return Colors::Sienna;
        }

        return Colors::Gray;
    }

    //-------------------------------------------------------------------------

    static Color GetMatterBodyBaseColor(
        ProvenanceMaterialID material,
        ProvenanceBodyState  bodyState )
    {
        if ( s_neutralMaterialDebug )
        {
            return Color(
                190,
                190,
                190 );
        }

        if ( material ==
             ProvenanceMaterialID::Granite )
        {
            if ( bodyState ==
                 ProvenanceBodyState::LooseAggregate )
            {
                return Colors::DarkGray;
            }

            return Colors::Gray;
        }

        if ( bodyState ==
             ProvenanceBodyState::Compacted )
        {
            return Colors::SaddleBrown;
        }

        return Colors::Sienna;
    }

    //-------------------------------------------------------------------------
    // Terrain contact presentation
    //-------------------------------------------------------------------------

    struct ContactVertex
    {
        float m_x =
            0.0f;

        float m_y =
            0.0f;

        float m_z =
            0.0f;

        float m_contact =
            0.0f;
    };

    //-------------------------------------------------------------------------

    static ContactVertex EvaluateContactVertex(
        uint32_t seed,
        float    worldX,
        float    worldY )
    {
        EvaluatedSurfacePoint const evaluated =
            EvaluateSurfacePoint(
                seed,
                worldX,
                worldY );

        ContactVertex result;

        result.m_x =
            worldX;

        result.m_y =
            worldY;

        result.m_z =
            evaluated.m_elevation;

        result.m_contact =
            evaluated.m_signedSoilDepth;

        return result;
    }

    //-------------------------------------------------------------------------

    static Float3 ContactVertexToFloat3(
        ContactVertex const& vertex )
    {
        return Float3(
            vertex.m_x,
            vertex.m_y,
            vertex.m_z );
    }

    //-------------------------------------------------------------------------

    static ContactVertex InterpolateContactVertex(
        ContactVertex const& a,
        ContactVertex const& b,
        float                t )
    {
        ContactVertex result;

        result.m_x =
            LerpFloat(
                a.m_x,
                b.m_x,
                t );

        result.m_y =
            LerpFloat(
                a.m_y,
                b.m_y,
                t );

        result.m_z =
            LerpFloat(
                a.m_z,
                b.m_z,
                t );

        result.m_contact =
            LerpFloat(
                a.m_contact,
                b.m_contact,
                t );

        return result;
    }

    //-------------------------------------------------------------------------

    static bool IsInsideMaterialHalfSpace(
        ContactVertex const& vertex,
        ProvenanceMaterialID material )
    {
        if ( material ==
             ProvenanceMaterialID::Soil )
        {
            return vertex.m_contact >=
                   0.0f;
        }

        return vertex.m_contact <=
               0.0f;
    }

    //-------------------------------------------------------------------------

    static int32_t ClipContactPolygon(
        ContactVertex const* pInput,
        int32_t              inputCount,
        ProvenanceMaterialID material,
        ContactVertex*       pOutput )
    {
        int32_t outputCount =
            0;

        for ( int32_t i = 0;
              i <
              inputCount;
              ++i )
        {
            ContactVertex const& current =
                pInput[i];

            ContactVertex const& next =
                pInput[(
                           i +
                           1 ) %
                       inputCount];

            bool const currentInside =
                IsInsideMaterialHalfSpace(
                    current,
                    material );

            bool const nextInside =
                IsInsideMaterialHalfSpace(
                    next,
                    material );

            if ( currentInside &&
                 nextInside )
            {
                pOutput[outputCount++] =
                    next;
            }
            else if ( currentInside &&
                      !nextInside )
            {
                float const denominator =
                    current.m_contact -
                    next.m_contact;

                if ( AbsFloat(
                         denominator ) >
                     0.000001f )
                {
                    float const t =
                        current.m_contact /
                        denominator;

                    pOutput[outputCount++] =
                        InterpolateContactVertex(
                            current,
                            next,
                            t );
                }
            }
            else if ( !currentInside &&
                      nextInside )
            {
                float const denominator =
                    current.m_contact -
                    next.m_contact;

                if ( AbsFloat(
                         denominator ) >
                     0.000001f )
                {
                    float const t =
                        current.m_contact /
                        denominator;

                    pOutput[outputCount++] =
                        InterpolateContactVertex(
                            current,
                            next,
                            t );
                }

                pOutput[outputCount++] =
                    next;
            }
        }

        return outputCount;
    }

    //-------------------------------------------------------------------------

    static void DrawContactPolygon(
        DebugDrawContext&    drawCtx,
        ContactVertex const* pVertices,
        int32_t              count,
        ProvenanceMaterialID material )
    {
        if ( count <
             3 )
        {
            return;
        }

        Color const baseColor =
            GetSurfaceBaseColor(
                material );

        for ( int32_t i = 1;
              i <
              count -
                  1;
              ++i )
        {
            ContactVertex const& a =
                pVertices[0];

            ContactVertex const& b =
                pVertices[i];

            ContactVertex const& c =
                pVertices[i + 1];

            Color const litColor =
                ApplyDiagnosticLighting(
                    baseColor,

                    a.m_x,
                    a.m_y,
                    a.m_z,

                    b.m_x,
                    b.m_y,
                    b.m_z,

                    c.m_x,
                    c.m_y,
                    c.m_z );

            drawCtx.DrawTriangle(
                ContactVertexToFloat3(
                    a ),

                ContactVertexToFloat3(
                    b ),

                ContactVertexToFloat3(
                    c ),

                litColor,
                DebugDrawLayer::World );
        }
    }

    //-------------------------------------------------------------------------

    static bool FindContactIntersection(
        ContactVertex const& a,
        ContactVertex const& b,
        ContactVertex&       result )
    {
        float constexpr epsilon =
            0.000001f;

        bool const aOnContact =
            AbsFloat(
                a.m_contact ) <
            epsilon;

        bool const bOnContact =
            AbsFloat(
                b.m_contact ) <
            epsilon;

        if ( aOnContact &&
             bOnContact )
        {
            return false;
        }

        if ( aOnContact )
        {
            result =
                a;

            return true;
        }

        if ( bOnContact )
        {
            result =
                b;

            return true;
        }

        bool const oppositeSides =
            ( a.m_contact <
                  0.0f &&
              b.m_contact >
                  0.0f ) ||
            ( a.m_contact >
                  0.0f &&
              b.m_contact <
                  0.0f );

        if ( !oppositeSides )
        {
            return false;
        }

        float const denominator =
            a.m_contact -
            b.m_contact;

        if ( AbsFloat(
                 denominator ) <
             epsilon )
        {
            return false;
        }

        float const t =
            a.m_contact /
            denominator;

        result =
            InterpolateContactVertex(
                a,
                b,
                t );

        return true;
    }

    //-------------------------------------------------------------------------

    static void DrawGeologicalContactLine(
        DebugDrawContext&    drawCtx,
        ContactVertex const& a,
        ContactVertex const& b,
        ContactVertex const& c )
    {
        if ( !s_showGeologicalContact )
        {
            return;
        }

        ContactVertex intersections[3];

        int32_t count =
            0;

        ContactVertex intersection;

        if ( FindContactIntersection(
                 a,
                 b,
                 intersection ) )
        {
            intersections[count++] =
                intersection;
        }

        if ( FindContactIntersection(
                 b,
                 c,
                 intersection ) &&
             count <
                 3 )
        {
            intersections[count++] =
                intersection;
        }

        if ( FindContactIntersection(
                 c,
                 a,
                 intersection ) &&
             count <
                 3 )
        {
            intersections[count++] =
                intersection;
        }

        if ( count >=
             2 )
        {
            Float3 const p0(
                intersections[0].m_x,
                intersections[0].m_y,
                intersections[0].m_z +
                    0.018f );

            Float3 const p1(
                intersections[1].m_x,
                intersections[1].m_y,
                intersections[1].m_z +
                    0.018f );

            drawCtx.DrawLine(
                p0,
                p1,
                Colors::Yellow,
                1.5f,
                DebugDrawLayer::World );
        }
    }

    //-------------------------------------------------------------------------

    static void DrawContactAwareTriangle(
        DebugDrawContext&    drawCtx,
        ContactVertex const& a,
        ContactVertex const& b,
        ContactVertex const& c )
    {
        ContactVertex input[3] =
            {
                a,
                b,
                c };

        ContactVertex soilPolygon[5];
        ContactVertex granitePolygon[5];

        int32_t const soilCount =
            ClipContactPolygon(
                input,
                3,
                ProvenanceMaterialID::Soil,
                soilPolygon );

        int32_t const graniteCount =
            ClipContactPolygon(
                input,
                3,
                ProvenanceMaterialID::Granite,
                granitePolygon );

        DrawContactPolygon(
            drawCtx,
            soilPolygon,
            soilCount,
            ProvenanceMaterialID::Soil );

        DrawContactPolygon(
            drawCtx,
            granitePolygon,
            graniteCount,
            ProvenanceMaterialID::Granite );

        DrawGeologicalContactLine(
            drawCtx,
            a,
            b,
            c );
    }

    //-------------------------------------------------------------------------
    // Cached sub-grid terrain presentation
    //
    // TSample is deduced from the private DebugSurfaceSample type at the
    // member-function call site. No geological evaluation occurs here.
    //-------------------------------------------------------------------------

    template<typename TSample>
    static void DrawCachedSurfacePatch(
        DebugDrawContext& drawCtx,
        TSample const*    pSamples,
        int32_t           cacheSamplesX,
        int32_t           startSampleX,
        int32_t           startSampleY,
        int32_t           derivedQuadsX,
        int32_t           derivedQuadsY,
        float             originWorldX,
        float             originWorldY,
        float             derivedSpacing )
    {
        EE_ASSERT(
            pSamples !=
            nullptr );

        for ( int32_t y = 0;
              y <
              derivedQuadsY;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  derivedQuadsX;
                  ++x )
            {
                int32_t const sampleX0 =
                    startSampleX +
                    x;

                int32_t const sampleX1 =
                    sampleX0 +
                    1;

                int32_t const sampleY0 =
                    startSampleY +
                    y;

                int32_t const sampleY1 =
                    sampleY0 +
                    1;

                int32_t const i00 =
                    sampleY0 *
                        cacheSamplesX +
                    sampleX0;

                int32_t const i10 =
                    sampleY0 *
                        cacheSamplesX +
                    sampleX1;

                int32_t const i01 =
                    sampleY1 *
                        cacheSamplesX +
                    sampleX0;

                int32_t const i11 =
                    sampleY1 *
                        cacheSamplesX +
                    sampleX1;

                float const x0 =
                    originWorldX +
                    float( x ) *
                        derivedSpacing;

                float const x1 =
                    x0 +
                    derivedSpacing;

                float const y0 =
                    originWorldY +
                    float( y ) *
                        derivedSpacing;

                float const y1 =
                    y0 +
                    derivedSpacing;

                ContactVertex const v00 =
                    {
                        x0,
                        y0,
                        pSamples[i00].m_elevation,
                        pSamples[i00].m_signedSoilDepth };

                ContactVertex const v10 =
                    {
                        x1,
                        y0,
                        pSamples[i10].m_elevation,
                        pSamples[i10].m_signedSoilDepth };

                ContactVertex const v01 =
                    {
                        x0,
                        y1,
                        pSamples[i01].m_elevation,
                        pSamples[i01].m_signedSoilDepth };

                ContactVertex const v11 =
                    {
                        x1,
                        y1,
                        pSamples[i11].m_elevation,
                        pSamples[i11].m_signedSoilDepth };

                DrawContactAwareTriangle(
                    drawCtx,
                    v00,
                    v10,
                    v11 );

                DrawContactAwareTriangle(
                    drawCtx,
                    v00,
                    v11,
                    v01 );
            }
        }
    }

    //-------------------------------------------------------------------------
    // P3C.9C-4 visible substrate + conserved Soil geometry renderer
    //
    // Substrate is drawn first as the real continuous parent surface.
    // Soil is then drawn only where explicit fine-cell occupancy exists.
    // The Soil top surface is backed by allocated bulk volume; breached regions
    // simply leave the substrate visible underneath.
    //-------------------------------------------------------------------------

    static void DrawP3C9C4SubstrateSurface(
        DebugDrawContext& drawCtx )
    {
        Color const baseColor =
            GetSurfaceBaseColor(
                ProvenanceMaterialID::Granite );

        for ( int32_t y = 0;
              y < s_p3c9c4FineCellsY;
              ++y )
        {
            for ( int32_t x = 0;
                  x < s_p3c9c4FineCellsX;
                  ++x )
            {
                int32_t const i00 =
                    y *
                        s_p3c9c4SourceVerticesX +
                    x;

                int32_t const i10 =
                    i00 +
                    1;

                int32_t const i01 =
                    i00 +
                    s_p3c9c4SourceVerticesX;

                int32_t const i11 =
                    i01 +
                    1;

                P3C9C4SourceVertex const& a =
                    s_p3c9c4SourceVertices[i00];

                P3C9C4SourceVertex const& b =
                    s_p3c9c4SourceVertices[i10];

                P3C9C4SourceVertex const& c =
                    s_p3c9c4SourceVertices[i01];

                P3C9C4SourceVertex const& d =
                    s_p3c9c4SourceVertices[i11];

                if ( !a.m_valid ||
                     !b.m_valid ||
                     !c.m_valid ||
                     !d.m_valid )
                {
                    continue;
                }

                Color const firstColor =
                    ApplyDiagnosticLighting(
                        baseColor,
                        a.m_worldX,
                        a.m_worldY,
                        a.m_bedrockZM,
                        b.m_worldX,
                        b.m_worldY,
                        b.m_bedrockZM,
                        d.m_worldX,
                        d.m_worldY,
                        d.m_bedrockZM );

                drawCtx.DrawTriangle(
                    Float3(
                        a.m_worldX,
                        a.m_worldY,
                        a.m_bedrockZM ),
                    Float3(
                        b.m_worldX,
                        b.m_worldY,
                        b.m_bedrockZM ),
                    Float3(
                        d.m_worldX,
                        d.m_worldY,
                        d.m_bedrockZM ),
                    firstColor,
                    DebugDrawLayer::World );

                Color const secondColor =
                    ApplyDiagnosticLighting(
                        baseColor,
                        a.m_worldX,
                        a.m_worldY,
                        a.m_bedrockZM,
                        d.m_worldX,
                        d.m_worldY,
                        d.m_bedrockZM,
                        c.m_worldX,
                        c.m_worldY,
                        c.m_bedrockZM );

                drawCtx.DrawTriangle(
                    Float3(
                        a.m_worldX,
                        a.m_worldY,
                        a.m_bedrockZM ),
                    Float3(
                        d.m_worldX,
                        d.m_worldY,
                        d.m_bedrockZM ),
                    Float3(
                        c.m_worldX,
                        c.m_worldY,
                        c.m_bedrockZM ),
                    secondColor,
                    DebugDrawLayer::World );
            }
        }
    }

    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // P3C.9C-4E — support-conforming Soil carrier rendering
    //
    // The visible Soil mesh now uses the SAME 129 x 65 preview-vertex lattice
    // as the Granite substrate. Soil thickness/contact are reconstructed from
    // the conserved 128 x 64 fine-cell field, but substrate Z comes directly
    // from the geological preview vertex.
    //
    // This means a Granite crease represented in the cached substrate lattice
    // is also represented in the Soil support carrier instead of being
    // averaged away at a fine-cell center.
    //-------------------------------------------------------------------------

    struct P3C9C4SoilRenderVertex
    {
        float m_x = 0.0f;
        float m_y = 0.0f;

        float m_substrateZ = 0.0f;
        float m_soilThickness = 0.0f;

        float m_contact = 0.0f;
    };

    //-------------------------------------------------------------------------

    static P3C9C4SoilRenderVertex P3C9C4SoilRenderVertexFromCarrier(
        P3C9C4SoilCarrierVertex const& carrier )
    {
        P3C9C4SoilRenderVertex result;

        result.m_x =
            carrier.m_worldX;

        result.m_y =
            carrier.m_worldY;

        result.m_substrateZ =
            carrier.m_substrateZ;

        result.m_soilThickness =
            carrier.m_contact >
                    0.0f
                ? MaxFloat(
                      carrier.m_soilThickness,
                      0.0f )
                : 0.0f;

        result.m_contact =
            carrier.m_contact;

        return result;
    }

    //-------------------------------------------------------------------------

    static P3C9C4SoilRenderVertex InterpolateP3C9C4SoilBoundaryVertex(
        P3C9C4SoilRenderVertex const& a,
        P3C9C4SoilRenderVertex const& b,
        float                         t )
    {
        P3C9C4SoilRenderVertex result;

        result.m_x =
            LerpFloat(
                a.m_x,
                b.m_x,
                t );

        result.m_y =
            LerpFloat(
                a.m_y,
                b.m_y,
                t );

        result.m_substrateZ =
            LerpFloat(
                a.m_substrateZ,
                b.m_substrateZ,
                t );

        // Explicit material contact: Soil thickness reaches zero exactly at
        // the reconstructed substrate boundary.
        result.m_soilThickness =
            0.0f;

        result.m_contact =
            0.0f;

        return result;
    }

    //-------------------------------------------------------------------------

    static int32_t ClipP3C9C4SoilPolygon(
        P3C9C4SoilRenderVertex const* pInput,
        int32_t                       inputCount,
        P3C9C4SoilRenderVertex*       pOutput )
    {
        int32_t outputCount =
            0;

        for ( int32_t i = 0;
              i < inputCount;
              ++i )
        {
            P3C9C4SoilRenderVertex const& current =
                pInput[i];

            P3C9C4SoilRenderVertex const& next =
                pInput[( i + 1 ) %
                       inputCount];

            bool const currentInside =
                current.m_contact >
                0.0f;

            bool const nextInside =
                next.m_contact >
                0.0f;

            if ( currentInside &&
                 nextInside )
            {
                pOutput[outputCount++] =
                    next;
            }
            else if ( currentInside &&
                      !nextInside )
            {
                float const denominator =
                    current.m_contact -
                    next.m_contact;

                if ( AbsFloat(
                         denominator ) >
                     0.000001f )
                {
                    float const t =
                        Clamp01(
                            current.m_contact /
                            denominator );

                    pOutput[outputCount++] =
                        InterpolateP3C9C4SoilBoundaryVertex(
                            current,
                            next,
                            t );
                }
            }
            else if ( !currentInside &&
                      nextInside )
            {
                float const denominator =
                    current.m_contact -
                    next.m_contact;

                if ( AbsFloat(
                         denominator ) >
                     0.000001f )
                {
                    float const t =
                        Clamp01(
                            current.m_contact /
                            denominator );

                    pOutput[outputCount++] =
                        InterpolateP3C9C4SoilBoundaryVertex(
                            current,
                            next,
                            t );
                }

                pOutput[outputCount++] =
                    next;
            }
        }

        return outputCount;
    }

    //-------------------------------------------------------------------------

    static void DrawP3C9C4SoilPolygon(
        DebugDrawContext&             drawCtx,
        P3C9C4SoilRenderVertex const* pVertices,
        int32_t                       count )
    {
        if ( count <
             3 )
        {
            return;
        }

        Color const baseColor =
            GetSurfaceBaseColor(
                ProvenanceMaterialID::Soil );

        for ( int32_t i = 1;
              i < count - 1;
              ++i )
        {
            P3C9C4SoilRenderVertex const& a =
                pVertices[0];

            P3C9C4SoilRenderVertex const& b =
                pVertices[i];

            P3C9C4SoilRenderVertex const& c =
                pVertices[i + 1];

            float const aZ =
                a.m_substrateZ +
                a.m_soilThickness;

            float const bZ =
                b.m_substrateZ +
                b.m_soilThickness;

            float const cZ =
                c.m_substrateZ +
                c.m_soilThickness;

            Color const litColor =
                ApplyDiagnosticLighting(
                    baseColor,
                    a.m_x,
                    a.m_y,
                    aZ,
                    b.m_x,
                    b.m_y,
                    bZ,
                    c.m_x,
                    c.m_y,
                    cZ );

            drawCtx.DrawTriangle(
                Float3(
                    a.m_x,
                    a.m_y,
                    aZ ),
                Float3(
                    b.m_x,
                    b.m_y,
                    bZ ),
                Float3(
                    c.m_x,
                    c.m_y,
                    cZ ),
                litColor,
                DebugDrawLayer::World );
        }
    }

    //-------------------------------------------------------------------------

    static void DrawP3C9C4SoilCarrierTriangle(
        DebugDrawContext&              drawCtx,
        P3C9C4SoilCarrierVertex const& a,
        P3C9C4SoilCarrierVertex const& b,
        P3C9C4SoilCarrierVertex const& c )
    {
        if ( !a.m_valid ||
             !b.m_valid ||
             !c.m_valid )
        {
            return;
        }

        P3C9C4SoilRenderVertex input[3] =
            {
                P3C9C4SoilRenderVertexFromCarrier( a ),
                P3C9C4SoilRenderVertexFromCarrier( b ),
                P3C9C4SoilRenderVertexFromCarrier( c ) };

        P3C9C4SoilRenderVertex soilPolygon[5];

        int32_t const soilCount =
            ClipP3C9C4SoilPolygon(
                input,
                3,
                soilPolygon );

        DrawP3C9C4SoilPolygon(
            drawCtx,
            soilPolygon,
            soilCount );
    }

    //-------------------------------------------------------------------------

    static void DrawP3C9C4SoilSurface(
        DebugDrawContext& drawCtx )
    {
        for ( int32_t y = 0;
              y < s_p3c9c4SourceVerticesY - 1;
              ++y )
        {
            for ( int32_t x = 0;
                  x < s_p3c9c4SourceVerticesX - 1;
                  ++x )
            {
                int32_t const i00 =
                    y *
                        s_p3c9c4SourceVerticesX +
                    x;

                int32_t const i10 =
                    i00 +
                    1;

                int32_t const i01 =
                    i00 +
                    s_p3c9c4SourceVerticesX;

                int32_t const i11 =
                    i01 +
                    1;

                P3C9C4SoilCarrierVertex const& v00 =
                    s_p3c9c4CarrierVertices[i00];

                P3C9C4SoilCarrierVertex const& v10 =
                    s_p3c9c4CarrierVertices[i10];

                P3C9C4SoilCarrierVertex const& v01 =
                    s_p3c9c4CarrierVertices[i01];

                P3C9C4SoilCarrierVertex const& v11 =
                    s_p3c9c4CarrierVertices[i11];

                // Match the exact substrate triangle split.
                DrawP3C9C4SoilCarrierTriangle(
                    drawCtx,
                    v00,
                    v10,
                    v11 );

                DrawP3C9C4SoilCarrierTriangle(
                    drawCtx,
                    v00,
                    v11,
                    v01 );
            }
        }
    }

    //-------------------------------------------------------------------------

    static void DrawP3C9C4ContactSegments(
        DebugDrawContext& drawCtx )
    {
        if ( !s_showGeologicalContact )
        {
            return;
        }

        for ( int32_t i = 0;
              i < s_p3c9c4NumContactSegments;
              ++i )
        {
            SoilContactSegment const& segment =
                s_p3c9c4ContactSegments[i];

            if ( !segment.m_valid )
            {
                continue;
            }

            drawCtx.DrawLine(
                Float3(
                    segment.m_startWorldX,
                    segment.m_startWorldY,
                    segment.m_startWorldZ +
                        0.018f ),
                Float3(
                    segment.m_endWorldX,
                    segment.m_endWorldY,
                    segment.m_endWorldZ +
                        0.018f ),
                Colors::Yellow,
                1.5f,
                DebugDrawLayer::World );
        }
    }

    //-------------------------------------------------------------------------

    static void DrawP3C9C4VisibleSoilTerrain(
        DebugDrawContext& drawCtx )
    {
        DrawP3C9C4SubstrateSurface(
            drawCtx );

        DrawP3C9C4SoilSurface(
            drawCtx );

        DrawP3C9C4ContactSegments(
            drawCtx );
    }

    //-------------------------------------------------------------------------
    // Variable-topology Granite renderer
    //-------------------------------------------------------------------------

    static void DrawGraniteClosedGeometry(
        DebugDrawContext&            drawCtx,
        GraniteClosedGeometry const& geometry,
        float                        centerX,
        float                        centerY,
        float                        baseZ,
        Color const&                 baseColor )
    {
        EE_ASSERT(
            geometry.m_numVertices >
            0 );

        EE_ASSERT(
            geometry.m_numVertices <=
            GraniteClosedGeometry::
                s_maxVertices );

        EE_ASSERT(
            geometry.m_numTriangles >
            0 );

        EE_ASSERT(
            geometry.m_numTriangles <=
            GraniteClosedGeometry::
                s_maxTriangles );

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              geometry.m_numTriangles;
              ++triangleIndex )
        {
            int32_t const indexBase =
                triangleIndex *
                3;

            uint8_t const i0 =
                geometry.m_triangleIndices[indexBase];

            uint8_t const i1 =
                geometry.m_triangleIndices[indexBase + 1];

            uint8_t const i2 =
                geometry.m_triangleIndices[indexBase + 2];

            EE_ASSERT(
                i0 <
                geometry.m_numVertices );

            EE_ASSERT(
                i1 <
                geometry.m_numVertices );

            EE_ASSERT(
                i2 <
                geometry.m_numVertices );

            GranitePoint const& a =
                geometry.m_vertices[i0];

            GranitePoint const& b =
                geometry.m_vertices[i1];

            GranitePoint const& c =
                geometry.m_vertices[i2];

            // Subtle diagnostic distinction only. Material identity remains
            // Granite in every case; history is not a second material.
            float const weatheredWeight =
                Clamp01(
                    geometry.m_triangleWeatheredWeight[triangleIndex] );

            Color const historyColor =
                baseColor.GetScaledColor(
                    0.94f +
                    weatheredWeight *
                        0.06f );

            Color const litColor =
                ApplyDiagnosticLighting(
                    historyColor,

                    a.m_x,
                    a.m_y,
                    a.m_z,

                    b.m_x,
                    b.m_y,
                    b.m_z,

                    c.m_x,
                    c.m_y,
                    c.m_z );

            drawCtx.DrawTriangle(
                Float3(
                    centerX +
                        a.m_x,
                    centerY +
                        a.m_y,
                    baseZ +
                        a.m_z ),

                Float3(
                    centerX +
                        b.m_x,
                    centerY +
                        b.m_y,
                    baseZ +
                        b.m_z ),

                Float3(
                    centerX +
                        c.m_x,
                    centerY +
                        c.m_y,
                    baseZ +
                        c.m_z ),

                litColor,
                DebugDrawLayer::World );
        }
    }

    //-------------------------------------------------------------------------
    // Soil body debug geometry
    //-------------------------------------------------------------------------

    struct SoilLocalPoint
    {
        float m_x =
            0.0f;

        float m_y =
            0.0f;

        float m_z =
            0.0f;
    };

    //-------------------------------------------------------------------------

    static float SoilSignedTriangleVolume(
        SoilLocalPoint const& a,
        SoilLocalPoint const& b,
        SoilLocalPoint const& c )
    {
        float const crossX =
            b.m_y *
                c.m_z -
            b.m_z *
                c.m_y;

        float const crossY =
            b.m_z *
                c.m_x -
            b.m_x *
                c.m_z;

        float const crossZ =
            b.m_x *
                c.m_y -
            b.m_y *
                c.m_x;

        return (
                   a.m_x *
                       crossX +
                   a.m_y *
                       crossY +
                   a.m_z *
                       crossZ ) /
               6.0f;
    }

    //-------------------------------------------------------------------------

    static Float3 SoilToWorldPoint(
        SoilLocalPoint const& local,
        float                 centerX,
        float                 centerY,
        float                 baseZ,
        float                 scale )
    {
        return Float3(
            centerX +
                local.m_x *
                    scale,

            centerY +
                local.m_y *
                    scale,

            baseZ +
                local.m_z *
                    scale );
    }

    //-------------------------------------------------------------------------

    static void DrawLitSoilTriangle(
        DebugDrawContext&     drawCtx,
        SoilLocalPoint const& a,
        SoilLocalPoint const& b,
        SoilLocalPoint const& c,
        float                 centerX,
        float                 centerY,
        float                 baseZ,
        float                 scale,
        Color const&          baseColor )
    {
        Color const litColor =
            ApplyDiagnosticLighting(
                baseColor,

                a.m_x,
                a.m_y,
                a.m_z,

                b.m_x,
                b.m_y,
                b.m_z,

                c.m_x,
                c.m_y,
                c.m_z );

        drawCtx.DrawTriangle(
            SoilToWorldPoint(
                a,
                centerX,
                centerY,
                baseZ,
                scale ),

            SoilToWorldPoint(
                b,
                centerX,
                centerY,
                baseZ,
                scale ),

            SoilToWorldPoint(
                c,
                centerX,
                centerY,
                baseZ,
                scale ),

            litColor,
            DebugDrawLayer::World );
    }

    //-------------------------------------------------------------------------

    static SoilBodyGeometryKind GetSoilBodyGeometryKind(
        ProvenanceWorldSystem::MatterBody const& body )
    {
        switch ( body.m_bodyState )
        {
            case ProvenanceBodyState::Fragment:
            {
                return SoilBodyGeometryKind::CohesiveClod;
            }

            case ProvenanceBodyState::LooseAggregate:
            {
                return SoilBodyGeometryKind::LoosePile;
            }

            case ProvenanceBodyState::Compacted:
            {
                return SoilBodyGeometryKind::CompactedMass;
            }

            default:
            {
                return SoilBodyGeometryKind::CohesiveClod;
            }
        }
    }

    //-------------------------------------------------------------------------

    static char const* GetSoilBodyGeometryKindName(
        SoilBodyGeometryKind kind )
    {
        switch ( kind )
        {
            case SoilBodyGeometryKind::CohesiveClod:
                return "CohesiveClod";

            case SoilBodyGeometryKind::LoosePile:
                return "LoosePile";

            case SoilBodyGeometryKind::CompactedMass:
                return "CompactedMass";

            default:
                return "SoilBody";
        }
    }

    //-------------------------------------------------------------------------

    static SoilBodyGeometry GenerateWorkbenchSoilGeometry(
        ProvenanceWorldSystem::MatterBody const& body,
        uint32_t                                 worldSeed )
    {
        SoilBodyGeometryRequest request;

        request.m_worldSeed =
            worldSeed;

        request.m_bodyID =
            body.m_bodyID;

        request.m_provenanceID =
            body.m_provenanceID;

        request.m_geometrySalt =
            body.m_geometrySalt;

        request.m_massGrams =
            body.m_massGrams;

        request.m_bodyState =
            body.m_bodyState;

        request.m_kind =
            GetSoilBodyGeometryKind(
                body );

        request.m_worldX =
            body.m_centerX;

        request.m_worldY =
            body.m_centerY;

        request.m_worldZ =
            body.m_baseZ;

        return GenerateSoilBodyGeometry(
            request );
    }

    //-------------------------------------------------------------------------

    static float DrawSoilBody(
        DebugDrawContext&                        drawCtx,
        ProvenanceWorldSystem::MatterBody const& body,
        uint32_t                                 worldSeed,
        Color const&                             baseColor )
    {
        SoilBodyGeometry const geometry =
            GenerateWorkbenchSoilGeometry(
                body,
                worldSeed );

        EE_ASSERT(
            geometry.m_conservationPass );

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              geometry.m_numTriangles;
              ++triangleIndex )
        {
            int32_t const indexBase =
                triangleIndex *
                3;

            int32_t const ia =
                int32_t(
                    geometry.m_triangleIndices[indexBase + 0] );

            int32_t const ib =
                int32_t(
                    geometry.m_triangleIndices[indexBase + 1] );

            int32_t const ic =
                int32_t(
                    geometry.m_triangleIndices[indexBase + 2] );

            SoilPoint const& a =
                geometry.m_vertices[ia];

            SoilPoint const& b =
                geometry.m_vertices[ib];

            SoilPoint const& c =
                geometry.m_vertices[ic];

            Color const litColor =
                ApplyDiagnosticLighting(
                    baseColor,
                    a.m_x,
                    a.m_y,
                    a.m_z,
                    b.m_x,
                    b.m_y,
                    b.m_z,
                    c.m_x,
                    c.m_y,
                    c.m_z );

            drawCtx.DrawTriangle(
                Float3(
                    a.m_x,
                    a.m_y,
                    a.m_z ),
                Float3(
                    b.m_x,
                    b.m_y,
                    b.m_z ),
                Float3(
                    c.m_x,
                    c.m_y,
                    c.m_z ),
                litColor,
                DebugDrawLayer::World );
        }

        return geometry.m_measuredMeshVolumeM3;
    }

    //-------------------------------------------------------------------------
    // Granite certification metrics

    //-------------------------------------------------------------------------

    struct GraniteSpecimenMetrics
    {
        bool m_valid =
            false;

        float m_requiredSolidVolumeM3 =
            0.0f;

        float m_requiredEnvelopeVolumeM3 =
            0.0f;

        float m_measuredSolidVolumeM3 =
            0.0f;

        float m_measuredEnvelopeVolumeM3 =
            0.0f;

        float m_solidResidualM3 =
            0.0f;

        float m_envelopeResidualM3 =
            0.0f;

        int32_t m_majorFaceCount =
            0;

        int32_t m_topologyFamily =
            -1;

        int32_t m_numVertices =
            0;

        int32_t m_numTriangles =
            0;

        int32_t m_restingTriangleIndex =
            -1;

        float m_restingFaceAreaM2 =
            0.0f;

        float m_extentXM =
            0.0f;

        float m_extentYM =
            0.0f;

        float m_extentZM =
            0.0f;

        float m_angularity =
            0.0f;

        float m_edgeWear =
            0.0f;

        float m_slabCharacter =
            0.0f;

        float m_elongationCharacter =
            0.0f;

        // P3C.8 surface-history receipts.
        float m_weatheredExteriorCoverage =
            0.0f;

        float m_freshFractureCoverage =
            1.0f;

        float m_transitionalCoverage =
            0.0f;

        float m_meanRounding =
            0.0f;

        float m_meanSharpness =
            1.0f;

        float m_meanPlanarity =
            1.0f;

        bool m_hasInheritedExterior =
            false;

        int32_t m_contributorCount =
            0;

        uint32_t m_contributorMassGrams =
            0;

        int32_t m_numAggregateClasts =
            0;
    };

    //-------------------------------------------------------------------------
    // Surface-stone readability / certification cache
    //
    // This is a development-only presentation cache. Granite event identity
    // comes from GraniteGeometry absolute-world descriptors; geometry is
    // generated once per seed and then reused during steady DebugDraw.
    //-------------------------------------------------------------------------

    struct SurfaceStoneDebugInstance
    {
        bool m_valid = false;

        uint32_t m_bodyID = 0;
        uint32_t m_massGrams = 0;

        float m_centerX = 0.0f;
        float m_centerY = 0.0f;
        float m_translationZ = 0.0f;

        float m_terrainZ = 0.0f;
        float m_embedDepthM = 0.0f;

        float m_supportDiameterM = 0.0f;
        float m_totalHeightM = 0.0f;
        float m_visibleHeightM = 0.0f;
        float m_aboveGroundFraction = 0.0f;

        GraniteClosedGeometry m_geometry;
    };

    //-------------------------------------------------------------------------

    struct SurfaceStoneClusterDebugCache
    {
        bool     m_valid = false;
        uint32_t m_seed = 0;

        void const* m_previewIdentity = nullptr;

        GraniteSurfaceStoneDescriptor m_sourceDescriptor;

        SurfaceStoneDebugInstance m_dominant;

        static constexpr int32_t  s_maxCompanions = 5;
        SurfaceStoneDebugInstance m_companions[s_maxCompanions];
        int32_t                   m_numCompanions = 0;

        float   m_maxSoilDepressionM = 0.0f;
        float   m_maxSoilApronM = 0.0f;
        int32_t m_numSettledPreviewSamples = 0;
    };

    //-------------------------------------------------------------------------

    static float SurfaceStoneHashUnit(
        uint32_t seed,
        uint32_t eventID,
        uint32_t salt )
    {
        uint32_t h =
            seed ^
            eventID ^
            ( salt * 0x9E3779B9u );

        h ^= h >> 16;
        h *= 0x7FEB352Du;
        h ^= h >> 15;
        h *= 0x846CA68Bu;
        h ^= h >> 16;

        return float( h & 0x00FFFFFFu ) /
               float( 0x00FFFFFFu );
    }

    //-------------------------------------------------------------------------

    static void GetGraniteGeometryZBounds(
        GraniteClosedGeometry const& geometry,
        float&                       minZ,
        float&                       maxZ )
    {
        EE_ASSERT( geometry.m_numVertices > 0 );

        minZ = geometry.m_vertices[0].m_z;
        maxZ = geometry.m_vertices[0].m_z;

        for ( int32_t i = 1;
              i < geometry.m_numVertices;
              ++i )
        {
            float const z = geometry.m_vertices[i].m_z;

            if ( z < minZ )
            {
                minZ = z;
            }

            if ( z > maxZ )
            {
                maxZ = z;
            }
        }
    }

    //-------------------------------------------------------------------------

    template<typename TSample>
    static bool SampleCachedPreviewSurface(
        TSample const* pSamples,
        int32_t        samplesX,
        int32_t        samplesY,
        float          originWorldX,
        float          originWorldY,
        float          spacing,
        float          worldX,
        float          worldY,
        float&         elevation,
        float&         signedSoilDepth )
    {
        EE_ASSERT( pSamples != nullptr );
        EE_ASSERT( samplesX > 1 );
        EE_ASSERT( samplesY > 1 );
        EE_ASSERT( spacing > 0.0f );

        float const localX =
            ( worldX - originWorldX ) /
            spacing;

        float const localY =
            ( worldY - originWorldY ) /
            spacing;

        if ( localX < 0.0f ||
             localY < 0.0f ||
             localX > float( samplesX - 1 ) ||
             localY > float( samplesY - 1 ) )
        {
            return false;
        }

        int32_t x0 =
            int32_t(
                std::floor(
                    double(
                        localX ) ) );

        int32_t y0 =
            int32_t(
                std::floor(
                    double(
                        localY ) ) );

        if ( x0 >= samplesX - 1 )
        {
            x0 = samplesX - 2;
        }

        if ( y0 >= samplesY - 1 )
        {
            y0 = samplesY - 2;
        }

        int32_t const x1 = x0 + 1;
        int32_t const y1 = y0 + 1;

        float const tx =
            Clamp01(
                localX -
                float( x0 ) );

        float const ty =
            Clamp01(
                localY -
                float( y0 ) );

        int32_t const i00 =
            y0 * samplesX +
            x0;

        int32_t const i10 =
            y0 * samplesX +
            x1;

        int32_t const i01 =
            y1 * samplesX +
            x0;

        int32_t const i11 =
            y1 * samplesX +
            x1;

        float const e0 =
            LerpFloat(
                pSamples[i00].m_elevation,
                pSamples[i10].m_elevation,
                tx );

        float const e1 =
            LerpFloat(
                pSamples[i01].m_elevation,
                pSamples[i11].m_elevation,
                tx );

        float const d0 =
            LerpFloat(
                pSamples[i00].m_signedSoilDepth,
                pSamples[i10].m_signedSoilDepth,
                tx );

        float const d1 =
            LerpFloat(
                pSamples[i01].m_signedSoilDepth,
                pSamples[i11].m_signedSoilDepth,
                tx );

        elevation =
            LerpFloat(
                e0,
                e1,
                ty );

        signedSoilDepth =
            LerpFloat(
                d0,
                d1,
                ty );

        return true;
    }

    //-------------------------------------------------------------------------

    static GraniteClosedGeometry GenerateSurfaceStoneGeometryForDiameter(
        uint32_t  seed,
        uint32_t  geologicalAncestryID,
        uint32_t  bodyID,
        float     worldX,
        float     worldY,
        float     worldZ,
        float     targetDiameterM,
        bool      weathered,
        uint32_t& outMassGrams )
    {
        // A roughly 3 ft equant Granite boulder is near a metric tonne.
        // Mass is therefore adjusted to obtain the requested physical diameter;
        // geometry is never scaled independently of density/mass authority.
        uint32_t massGrams =
            targetDiameterM > 0.55f
                ? 1000000u
                : 16000u;

        GraniteClosedGeometry geometry;

        for ( int32_t pass = 0;
              pass < 2;
              ++pass )
        {
            GraniteClosedGeometryRequest request;

            request.m_worldSeed =
                seed;

            request.m_geologicalAncestryID =
                geologicalAncestryID;

            request.m_bodyID =
                bodyID;

            request.m_massGrams =
                massGrams;

            request.m_bodyState =
                ProvenanceBodyState::Fragment;

            request.m_scale =
                targetDiameterM > 0.55f
                    ? GraniteGeometryScale::Block
                    : GraniteGeometryScale::Fragment;

            request.m_weathering =
                weathered
                    ? GraniteWeatheringState::WeatheredExposure
                    : GraniteWeatheringState::FreshFracture;

            request.m_canInheritExteriorSurface =
                weathered;

            request.m_parentExteriorExposure =
                weathered
                    ? 0.92f
                    : 0.28f;

            request.m_worldX =
                worldX;

            request.m_worldY =
                worldY;

            request.m_worldZ =
                worldZ;

            geometry =
                GenerateGraniteClosedGeometry(
                    request );

            float const measuredDiameter =
                MaxFloat(
                    geometry.m_extentXM,
                    geometry.m_extentYM );

            EE_ASSERT(
                measuredDiameter >
                0.0001f );

            if ( pass == 0 )
            {
                float const diameterScale =
                    targetDiameterM /
                    measuredDiameter;

                double correctedMass =
                    double( massGrams ) *
                    double( diameterScale ) *
                    double( diameterScale ) *
                    double( diameterScale );

                double const minimumMass =
                    targetDiameterM > 0.55f
                        ? 450000.0
                        : 500.0;

                double const maximumMass =
                    targetDiameterM > 0.55f
                        ? 2600000.0
                        : 120000.0;

                if ( correctedMass < minimumMass )
                {
                    correctedMass = minimumMass;
                }

                if ( correctedMass > maximumMass )
                {
                    correctedMass = maximumMass;
                }

                massGrams =
                    uint32_t(
                        correctedMass +
                        0.5 );
            }
        }

        outMassGrams =
            massGrams;

        return geometry;
    }

    //-------------------------------------------------------------------------

    template<typename TSample>
    static bool BuildSurfaceStoneClusterDebugCache(
        SurfaceStoneClusterDebugCache& cache,
        uint32_t                       seed,
        TSample const*                 pSamples,
        int32_t                        samplesX,
        int32_t                        samplesY,
        float                          originWorldX,
        float                          originWorldY,
        float                          spacing )
    {
        cache =
            SurfaceStoneClusterDebugCache();

        cache.m_seed =
            seed;

        cache.m_previewIdentity =
            pSamples;

        GraniteSurfaceStoneFieldRequest fieldRequest;

        fieldRequest.m_worldSeed =
            seed;

        fieldRequest.m_geologicalAncestryID =
            s_graniteGeologicalAncestryID;

        fieldRequest.m_minWorldX =
            originWorldX;

        fieldRequest.m_minWorldY =
            originWorldY;

        fieldRequest.m_maxWorldX =
            originWorldX +
            float( samplesX - 1 ) *
                spacing;

        fieldRequest.m_maxWorldY =
            originWorldY +
            float( samplesY - 1 ) *
                spacing;

        GraniteSurfaceStoneField const field =
            GenerateGraniteSurfaceStoneField(
                fieldRequest );

        if ( field.m_numDescriptors <= 0 )
        {
            return false;
        }

        int32_t bestIndex =
            -1;

        float bestScore =
            -1000000.0f;

        float bestTerrainZ =
            0.0f;

        float bestSignedDepth =
            0.0f;

        for ( int32_t i = 0;
              i < field.m_numDescriptors;
              ++i )
        {
            GraniteSurfaceStoneDescriptor const& descriptor =
                field.m_descriptors[i];

            if ( !descriptor.m_valid )
            {
                continue;
            }

            float terrainZ =
                0.0f;

            float signedDepth =
                0.0f;

            if ( !SampleCachedPreviewSurface(
                     pSamples,
                     samplesX,
                     samplesY,
                     originWorldX,
                     originWorldY,
                     spacing,
                     descriptor.m_centerWorldX,
                     descriptor.m_centerWorldY,
                     terrainZ,
                     signedDepth ) )
            {
                continue;
            }

            float const contactDistance =
                AbsFloat(
                    signedDepth );

            float const contactScore =
                1.0f -
                Clamp01(
                    contactDistance /
                    0.32f );

            float stateScore =
                0.15f;

            if ( descriptor.m_state ==
                 GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate )
            {
                stateScore =
                    0.85f;
            }
            else if ( descriptor.m_state ==
                      GraniteSurfaceStoneState::PartiallyDetachedBlock )
            {
                stateScore =
                    0.68f;
            }

            float const thinSoilBonus =
                signedDepth >= -0.12f &&
                        signedDepth <= 0.18f
                    ? 0.30f
                    : 0.0f;

            float const sizeScore =
                descriptor.m_majorRadiusM *
                    0.10f +
                descriptor.m_maximumReliefM *
                    0.14f;

            float const score =
                contactScore *
                    1.80f +
                stateScore +
                thinSoilBonus +
                sizeScore;

            if ( score >
                 bestScore )
            {
                bestScore =
                    score;

                bestIndex =
                    i;

                bestTerrainZ =
                    terrainZ;

                bestSignedDepth =
                    signedDepth;
            }
        }

        if ( bestIndex <
             0 )
        {
            return false;
        }

        GraniteSurfaceStoneDescriptor const& source =
            field.m_descriptors[bestIndex];

        cache.m_sourceDescriptor =
            source;

        float const targetDiameterM =
            0.91f +
            ( SurfaceStoneHashUnit(
                  seed,
                  source.m_eventID,
                  11u ) -
              0.5f ) *
                0.12f;

        uint32_t const dominantBodyID =
            ( source.m_eventID &
              ~3u ) |
            3u; // avoid slab-family selection for the certification boulder

        SurfaceStoneDebugInstance& dominant =
            cache.m_dominant;

        dominant.m_valid =
            true;

        dominant.m_bodyID =
            dominantBodyID;

        dominant.m_centerX =
            source.m_centerWorldX;

        dominant.m_centerY =
            source.m_centerWorldY;

        dominant.m_terrainZ =
            bestTerrainZ;

        dominant.m_geometry =
            GenerateSurfaceStoneGeometryForDiameter(
                seed,
                s_graniteGeologicalAncestryID,
                dominantBodyID,
                dominant.m_centerX,
                dominant.m_centerY,
                bestTerrainZ,
                targetDiameterM,
                true,
                dominant.m_massGrams );

        float minZ =
            0.0f;

        float maxZ =
            0.0f;

        GetGraniteGeometryZBounds(
            dominant.m_geometry,
            minZ,
            maxZ );

        dominant.m_supportDiameterM =
            MaxFloat(
                dominant.m_geometry.m_extentXM,
                dominant.m_geometry.m_extentYM );

        dominant.m_totalHeightM =
            maxZ -
            minZ;

        dominant.m_embedDepthM =
            dominant.m_totalHeightM *
            0.14f;

        if ( dominant.m_embedDepthM >
             0.085f )
        {
            dominant.m_embedDepthM =
                0.085f;
        }

        dominant.m_translationZ =
            bestTerrainZ -
            dominant.m_embedDepthM -
            minZ;

        dominant.m_visibleHeightM =
            MaxFloat(
                dominant.m_totalHeightM -
                    dominant.m_embedDepthM,
                0.0f );

        dominant.m_aboveGroundFraction =
            dominant.m_totalHeightM >
                    0.0001f
                ? Clamp01(
                      dominant.m_visibleHeightM /
                      dominant.m_totalHeightM )
                : 0.0f;

        // Companion count: 2-5, deterministic from the same geological event.
        cache.m_numCompanions =
            2 +
            int32_t(
                SurfaceStoneHashUnit(
                    seed,
                    source.m_eventID,
                    23u ) *
                4.0f );

        if ( cache.m_numCompanions >
             SurfaceStoneClusterDebugCache::
                 s_maxCompanions )
        {
            cache.m_numCompanions =
                SurfaceStoneClusterDebugCache::
                    s_maxCompanions;
        }

        float const clusterBaseAngle =
            source.m_orientationRadians +
            ( SurfaceStoneHashUnit(
                  seed,
                  source.m_eventID,
                  29u ) -
              0.5f ) *
                1.0f;

        for ( int32_t i = 0;
              i < cache.m_numCompanions;
              ++i )
        {
            SurfaceStoneDebugInstance& companion =
                cache.m_companions[i];

            float const angleJitter =
                ( SurfaceStoneHashUnit(
                      seed,
                      source.m_eventID,
                      101u +
                          uint32_t( i ) *
                              7u ) -
                  0.5f ) *
                0.70f;

            float const angle =
                clusterBaseAngle +
                float( i ) *
                    2.39996322972865332f +
                angleJitter;

            float const radialDistance =
                dominant.m_supportDiameterM *
                ( 0.58f +
                  SurfaceStoneHashUnit(
                      seed,
                      source.m_eventID,
                      131u +
                          uint32_t( i ) *
                              11u ) *
                      0.72f );

            companion.m_centerX =
                dominant.m_centerX +
                float(
                    std::cos(
                        double(
                            angle ) ) ) *
                    radialDistance;

            companion.m_centerY =
                dominant.m_centerY +
                float(
                    std::sin(
                        double(
                            angle ) ) ) *
                    radialDistance;

            float companionTerrainZ =
                bestTerrainZ;

            float companionSignedDepth =
                bestSignedDepth;

            SampleCachedPreviewSurface(
                pSamples,
                samplesX,
                samplesY,
                originWorldX,
                originWorldY,
                spacing,
                companion.m_centerX,
                companion.m_centerY,
                companionTerrainZ,
                companionSignedDepth );

            companion.m_terrainZ =
                companionTerrainZ;

            float const companionDiameter =
                0.16f +
                SurfaceStoneHashUnit(
                    seed,
                    source.m_eventID,
                    173u +
                        uint32_t( i ) *
                            13u ) *
                    0.25f;

            uint32_t family =
                0u;

            switch ( i % 3 )
            {
                case 1:
                    family = 2u;
                    break;

                case 2:
                    family = 3u;
                    break;

                default:
                    family = 0u;
                    break;
            }

            companion.m_bodyID =
                ( ( source.m_eventID +
                    0x9E3779B9u *
                        uint32_t( i + 1 ) ) &
                  ~3u ) |
                family;

            bool const weathered =
                SurfaceStoneHashUnit(
                    seed,
                    source.m_eventID,
                    211u +
                        uint32_t( i ) *
                            17u ) >
                0.34f;

            companion.m_geometry =
                GenerateSurfaceStoneGeometryForDiameter(
                    seed,
                    s_graniteGeologicalAncestryID,
                    companion.m_bodyID,
                    companion.m_centerX,
                    companion.m_centerY,
                    companionTerrainZ,
                    companionDiameter,
                    weathered,
                    companion.m_massGrams );

            GetGraniteGeometryZBounds(
                companion.m_geometry,
                minZ,
                maxZ );

            companion.m_supportDiameterM =
                MaxFloat(
                    companion.m_geometry.m_extentXM,
                    companion.m_geometry.m_extentYM );

            companion.m_totalHeightM =
                maxZ -
                minZ;

            companion.m_embedDepthM =
                0.012f +
                SurfaceStoneHashUnit(
                    seed,
                    source.m_eventID,
                    241u +
                        uint32_t( i ) *
                            19u ) *
                    0.035f;

            float const maxCompanionEmbed =
                companion.m_totalHeightM *
                0.28f;

            if ( companion.m_embedDepthM >
                 maxCompanionEmbed )
            {
                companion.m_embedDepthM =
                    maxCompanionEmbed;
            }

            companion.m_translationZ =
                companionTerrainZ -
                companion.m_embedDepthM -
                minZ;

            companion.m_visibleHeightM =
                MaxFloat(
                    companion.m_totalHeightM -
                        companion.m_embedDepthM,
                    0.0f );

            companion.m_aboveGroundFraction =
                companion.m_totalHeightM >
                        0.0001f
                    ? Clamp01(
                          companion.m_visibleHeightM /
                          companion.m_totalHeightM )
                    : 0.0f;

            companion.m_valid =
                true;
        }

        cache.m_valid =
            true;

        return true;
    }

    //-------------------------------------------------------------------------

    template<typename TSample>
    static void ApplySurfaceStoneSoilSettlingToPreview(
        SurfaceStoneClusterDebugCache& cache,
        TSample*                       pSamples,
        int32_t                        samplesX,
        int32_t                        samplesY,
        float                          originWorldX,
        float                          originWorldY,
        float                          spacing )
    {
        if ( !cache.m_valid )
        {
            return;
        }

        cache.m_maxSoilDepressionM =
            0.0f;

        cache.m_maxSoilApronM =
            0.0f;

        cache.m_numSettledPreviewSamples =
            0;

        for ( int32_t y = 0;
              y < samplesY;
              ++y )
        {
            float const worldY =
                originWorldY +
                float( y ) *
                    spacing;

            for ( int32_t x = 0;
                  x < samplesX;
                  ++x )
            {
                float const worldX =
                    originWorldX +
                    float( x ) *
                        spacing;

                int32_t const index =
                    y * samplesX +
                    x;

                TSample& sample =
                    pSamples[index];

                // Soil settling is meaningful only where Soil currently owns
                // the visible surface. Granite remains Granite.
                if ( sample.m_signedSoilDepth <=
                     0.0f )
                {
                    continue;
                }

                float totalDelta =
                    0.0f;

                auto accumulateStoneSettlement =
                    [&]( SurfaceStoneDebugInstance const& stone,
                         bool                             dominantStone )
                {
                    if ( !stone.m_valid )
                    {
                        return;
                    }

                    float const deltaX =
                        worldX -
                        stone.m_centerX;

                    float const deltaY =
                        worldY -
                        stone.m_centerY;

                    float const distance =
                        float(
                            std::sqrt(
                                double(
                                    deltaX * deltaX +
                                    deltaY * deltaY ) ) );

                    float const radius =
                        MaxFloat(
                            stone.m_supportDiameterM *
                                0.50f,
                            0.06f );

                    if ( distance >
                         radius *
                             1.65f )
                    {
                        return;
                    }

                    float const normalized =
                        distance /
                        radius;

                    float const core =
                        1.0f -
                        SmoothStep01(
                            normalized /
                            0.92f );

                    float const apronInner =
                        SmoothStep01(
                            ( normalized -
                              0.55f ) /
                            0.30f );

                    float const apronOuter =
                        1.0f -
                        SmoothStep01(
                            ( normalized -
                              0.95f ) /
                            0.62f );

                    float const apron =
                        Clamp01(
                            apronInner *
                            apronOuter );

                    float const depressionDepth =
                        dominantStone
                            ? 0.045f
                            : 0.016f;

                    float const apronHeight =
                        dominantStone
                            ? 0.026f
                            : 0.010f;

                    float const depression =
                        core *
                        depressionDepth;

                    float const accumulation =
                        apron *
                        apronHeight;

                    totalDelta +=
                        accumulation -
                        depression;

                    if ( depression >
                         cache.m_maxSoilDepressionM )
                    {
                        cache.m_maxSoilDepressionM =
                            depression;
                    }

                    if ( accumulation >
                         cache.m_maxSoilApronM )
                    {
                        cache.m_maxSoilApronM =
                            accumulation;
                    }
                };

                accumulateStoneSettlement(
                    cache.m_dominant,
                    true );

                for ( int32_t i = 0;
                      i < cache.m_numCompanions;
                      ++i )
                {
                    accumulateStoneSettlement(
                        cache.m_companions[i],
                        false );
                }

                if ( AbsFloat(
                         totalDelta ) >
                     0.00001f )
                {
                    sample.m_elevation +=
                        totalDelta;

                    // Bedrock is unchanged; moving the Soil surface changes
                    // actual local Soil thickness by the same signed amount.
                    sample.m_signedSoilDepth +=
                        totalDelta;

                    ++cache.m_numSettledPreviewSamples;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    static void DrawSurfaceStoneCluster(
        DebugDrawContext&                    drawCtx,
        SurfaceStoneClusterDebugCache const& cache )
    {
        if ( !cache.m_valid )
        {
            return;
        }

        Color const dominantColor =
            s_neutralMaterialDebug
                ? Color( 190, 190, 190 )
                : Colors::Gray;

        DrawGraniteClosedGeometry(
            drawCtx,
            cache.m_dominant.m_geometry,
            cache.m_dominant.m_centerX,
            cache.m_dominant.m_centerY,
            cache.m_dominant.m_translationZ,
            dominantColor );

        drawCtx.DrawText3D(
            Float3(
                cache.m_dominant.m_centerX,
                cache.m_dominant.m_centerY,
                cache.m_dominant.m_terrainZ +
                    cache.m_dominant.m_visibleHeightM +
                    0.08f ),
            "R0",
            Colors::White );

        for ( int32_t i = 0;
              i < cache.m_numCompanions;
              ++i )
        {
            SurfaceStoneDebugInstance const& companion =
                cache.m_companions[i];

            if ( !companion.m_valid )
            {
                continue;
            }

            Color const companionColor =
                s_neutralMaterialDebug
                    ? Color( 190, 190, 190 )
                    : Colors::DarkGray;

            DrawGraniteClosedGeometry(
                drawCtx,
                companion.m_geometry,
                companion.m_centerX,
                companion.m_centerY,
                companion.m_translationZ,
                companionColor );
        }
    }

    //-------------------------------------------------------------------------
    // Granite topology name
    //-------------------------------------------------------------------------

    static char const* GetGraniteTopologyFamilyName(
        int32_t family )
    {
        switch ( family )
        {
            case 0:
            {
                return "IrregularBlock";
            }

            case 1:
            {
                return "Slab";
            }

            case 2:
            {
                return "Wedge";
            }

            case 3:
            {
                return "TruncatedBlock";
            }

            default:
            {
                return "Aggregate";
            }
        }
    }

    //-------------------------------------------------------------------------
    // Granite request helper
    //-------------------------------------------------------------------------

    static GraniteClosedGeometryRequest MakeGraniteClosedRequest(
        ProvenanceWorldSystem::MatterBody const& body,
        uint32_t                                 worldSeed,
        GraniteGeometryScale                     scale,
        GraniteWeatheringState                   weathering )
    {
        GraniteClosedGeometryRequest request;

        request.m_worldSeed =
            worldSeed;

        request.m_geologicalAncestryID =
            body.m_geologicalAncestryID;

        request.m_bodyID =
            body.m_bodyID;

        request.m_massGrams =
            body.m_massGrams;

        request.m_bodyState =
            body.m_bodyState;

        request.m_scale =
            scale;

        request.m_weathering =
            weathering;

        // B1 is the P3C.8 mixed-history hand specimen: it represents a piece
        // detached from an already exposed Granite surface. The generator may
        // therefore retain old exterior while the remaining faces stay fresh.
        request.m_canInheritExteriorSurface =
            body.m_material ==
                ProvenanceMaterialID::Granite &&
            body.m_bodyState ==
                ProvenanceBodyState::Fragment;

        request.m_parentExteriorExposure =
            body.m_bodyID ==
                    1
                ? 0.90f
                : 0.0f;

        request.m_worldX =
            body.m_centerX;

        request.m_worldY =
            body.m_centerY;

        request.m_worldZ =
            body.m_baseZ;

        return request;
    }

    //-------------------------------------------------------------------------
    // Closed Granite body
    //-------------------------------------------------------------------------

    static GraniteSpecimenMetrics DrawClosedGraniteSpecimen(
        DebugDrawContext&                        drawCtx,
        ProvenanceWorldSystem::MatterBody const& body,
        uint32_t                                 worldSeed,
        GraniteGeometryScale                     scale,
        GraniteWeatheringState                   weathering,
        Color const&                             color )
    {
        GraniteClosedGeometryRequest request =
            MakeGraniteClosedRequest(
                body,
                worldSeed,
                scale,
                weathering );

        GraniteClosedGeometry const geometry =
            GenerateGraniteClosedGeometry(
                request );

        DrawGraniteClosedGeometry(
            drawCtx,
            geometry,
            body.m_centerX,
            body.m_centerY,
            body.m_baseZ,
            color );

        GraniteSpecimenMetrics metrics;

        metrics.m_valid =
            true;

        metrics.m_requiredSolidVolumeM3 =
            geometry.m_requiredSolidVolumeM3;

        metrics.m_requiredEnvelopeVolumeM3 =
            geometry.m_requiredEnvelopeVolumeM3;

        metrics.m_measuredSolidVolumeM3 =
            geometry.m_measuredMeshVolumeM3;

        metrics.m_measuredEnvelopeVolumeM3 =
            geometry.m_measuredMeshVolumeM3;

        metrics.m_solidResidualM3 =
            geometry.m_volumeResidualM3;

        metrics.m_envelopeResidualM3 =
            metrics.m_measuredEnvelopeVolumeM3 -
            metrics.m_requiredEnvelopeVolumeM3;

        metrics.m_majorFaceCount =
            geometry.m_majorFaceCount;

        metrics.m_topologyFamily =
            geometry.m_topologyFamily;

        metrics.m_numVertices =
            geometry.m_numVertices;

        metrics.m_numTriangles =
            geometry.m_numTriangles;

        metrics.m_restingTriangleIndex =
            geometry.m_restingTriangleIndex;

        metrics.m_restingFaceAreaM2 =
            geometry.m_restingFaceAreaM2;

        metrics.m_extentXM =
            geometry.m_extentXM;

        metrics.m_extentYM =
            geometry.m_extentYM;

        metrics.m_extentZM =
            geometry.m_extentZM;

        metrics.m_angularity =
            geometry.m_effectiveAngularity;

        metrics.m_edgeWear =
            geometry.m_effectiveEdgeWear;

        metrics.m_slabCharacter =
            geometry.m_slabCharacter;

        metrics.m_elongationCharacter =
            geometry.m_elongationCharacter;

        metrics.m_weatheredExteriorCoverage =
            geometry.m_surfaceHistory.m_weatheredExteriorCoverage;

        metrics.m_freshFractureCoverage =
            geometry.m_surfaceHistory.m_freshFractureCoverage;

        metrics.m_transitionalCoverage =
            geometry.m_surfaceHistory.m_transitionalCoverage;

        metrics.m_meanRounding =
            geometry.m_surfaceHistory.m_meanRounding;

        metrics.m_meanSharpness =
            geometry.m_surfaceHistory.m_meanSharpness;

        metrics.m_meanPlanarity =
            geometry.m_surfaceHistory.m_meanPlanarity;

        metrics.m_hasInheritedExterior =
            geometry.m_surfaceHistory.m_hasInheritedExterior;

        metrics.m_contributorCount =
            body.m_numContributors;

        for ( int32_t i = 0;
              i <
              body.m_numContributors;
              ++i )
        {
            metrics.m_contributorMassGrams +=
                body.m_contributors[i].m_massGrams;
        }

        return metrics;
    }

    //-------------------------------------------------------------------------
    // P3C.10A — fracture transaction debug certificate
    //
    // B3 remains the visible parent specimen in the normal workbench row.
    // The cache below fractures that exact generated B3 geometry once per
    // seed and displays the two child matter bodies in a second row.
    //-------------------------------------------------------------------------

    struct GraniteFractureDebugCache
    {
        bool     m_valid = false;
        uint32_t m_seed = 0;

        GraniteClosedGeometryRequest m_parentRequest;
        GraniteClosedGeometry        m_parentGeometry;

        GraniteFractureTransactionResult m_transaction;

        float m_displayCenterX = 0.0f;
        float m_displayCenterY = 0.0f;
        float m_displayBaseZ = 0.0f;

        float m_childSeparationM = 0.20f;
    };

    //-------------------------------------------------------------------------

    static GranitePoint CalculateGraniteClosedGeometryCentroid(
        GraniteClosedGeometry const& geometry )
    {
        GranitePoint centroid;

        if ( geometry.m_numVertices <=
             0 )
        {
            return centroid;
        }

        for ( int32_t i = 0;
              i < geometry.m_numVertices;
              ++i )
        {
            centroid.m_x +=
                geometry.m_vertices[i].m_x;

            centroid.m_y +=
                geometry.m_vertices[i].m_y;

            centroid.m_z +=
                geometry.m_vertices[i].m_z;
        }

        float const inverseCount =
            1.0f /
            float(
                geometry.m_numVertices );

        centroid.m_x *=
            inverseCount;

        centroid.m_y *=
            inverseCount;

        centroid.m_z *=
            inverseCount;

        return centroid;
    }

    //-------------------------------------------------------------------------

    static bool BuildGraniteFractureDebugCache(
        GraniteFractureDebugCache&               cache,
        ProvenanceWorldSystem::MatterBody const& parentBody,
        uint32_t                                 worldSeed )
    {
        cache =
            GraniteFractureDebugCache();

        cache.m_seed =
            worldSeed;

        cache.m_parentRequest =
            MakeGraniteClosedRequest(
                parentBody,
                worldSeed,
                GraniteGeometryScale::Block,
                GraniteWeatheringState::WeatheredExposure );

        cache.m_parentGeometry =
            GenerateGraniteClosedGeometry(
                cache.m_parentRequest );

        if ( cache.m_parentGeometry.m_numVertices <
                 4 ||
             cache.m_parentGeometry.m_numFaces <
                 4 )
        {
            return false;
        }

        GranitePoint const localCentroid =
            CalculateGraniteClosedGeometryCentroid(
                cache.m_parentGeometry );

        GranitePoint fractureNormal;

        fractureNormal.m_x =
            0.67f;

        fractureNormal.m_y =
            -0.22f;

        fractureNormal.m_z =
            0.71f;

        float const normalLength =
            float(
                std::sqrt(
                    double(
                        fractureNormal.m_x *
                            fractureNormal.m_x +
                        fractureNormal.m_y *
                            fractureNormal.m_y +
                        fractureNormal.m_z *
                            fractureNormal.m_z ) ) );

        if ( normalLength <=
             0.000001f )
        {
            return false;
        }

        fractureNormal.m_x /=
            normalLength;

        fractureNormal.m_y /=
            normalLength;

        fractureNormal.m_z /=
            normalLength;

        // Slightly off-center cut. This avoids an artificial exact half split
        // while still guaranteeing the plane passes through the body.
        float const cutOffset =
            0.035f *
            MaxFloat(
                MaxFloat(
                    cache.m_parentGeometry.m_extentXM,
                    cache.m_parentGeometry.m_extentYM ),
                cache.m_parentGeometry.m_extentZM );

        GranitePoint localPlanePoint =
            localCentroid;

        localPlanePoint.m_x +=
            fractureNormal.m_x *
            cutOffset;

        localPlanePoint.m_y +=
            fractureNormal.m_y *
            cutOffset;

        localPlanePoint.m_z +=
            fractureNormal.m_z *
            cutOffset;

        GraniteFractureRequest fractureRequest;

        fractureRequest.m_worldSeed =
            worldSeed;

        fractureRequest.m_eventID =
            0x10A00001u;

        fractureRequest.m_parentBodyID =
            parentBody.m_bodyID;

        fractureRequest.m_parentProvenanceID =
            parentBody.m_provenanceID;

        fractureRequest.m_geologicalAncestryID =
            parentBody.m_geologicalAncestryID;

        fractureRequest.m_primaryChildBodyID =
            301;

        fractureRequest.m_secondaryChildBodyID =
            302;

        fractureRequest.m_allowChips =
            false;

        fractureRequest.m_maxChipCount =
            0;

        fractureRequest.m_plane.m_normal =
            fractureNormal;

        fractureRequest.m_plane.m_pointOnPlane.m_x =
            parentBody.m_centerX +
            localPlanePoint.m_x;

        fractureRequest.m_plane.m_pointOnPlane.m_y =
            parentBody.m_centerY +
            localPlanePoint.m_y;

        fractureRequest.m_plane.m_pointOnPlane.m_z =
            parentBody.m_baseZ +
            localPlanePoint.m_z;

        fractureRequest.m_plane.m_origin =
            GraniteFaceOrigin::FreshBreak;

        fractureRequest.m_plane.m_jointSetIndex =
            -1;

        fractureRequest.m_planeToleranceM =
            0.00025f;

        cache.m_transaction =
            FractureGraniteClosedGeometry(
                cache.m_parentGeometry,
                cache.m_parentRequest,
                fractureRequest );

        cache.m_valid =
            cache.m_transaction.m_valid;

        // Second certification row behind the standard workbench.
        cache.m_displayCenterX =
            parentBody.m_centerX;

        cache.m_displayCenterY =
            parentBody.m_centerY +
            3.2f;

        cache.m_displayBaseZ =
            parentBody.m_baseZ;

        cache.m_childSeparationM =
            0.20f;

        return cache.m_valid;
    }

    //-------------------------------------------------------------------------

    static void DrawGraniteFractureChildGeometry(
        DebugDrawContext&           drawCtx,
        GraniteFractureChild const& child,
        float                       centerX,
        float                       centerY,
        float                       baseZ,
        Color const&                baseColor )
    {
        GraniteClosedGeometry const& geometry =
            child.m_geometry;

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              geometry.m_numTriangles;
              ++triangleIndex )
        {
            int32_t const indexBase =
                triangleIndex *
                3;

            int32_t const ia =
                int32_t(
                    geometry.m_triangleIndices[indexBase + 0] );

            int32_t const ib =
                int32_t(
                    geometry.m_triangleIndices[indexBase + 1] );

            int32_t const ic =
                int32_t(
                    geometry.m_triangleIndices[indexBase + 2] );

            GranitePoint const& a =
                geometry.m_vertices[ia];

            GranitePoint const& b =
                geometry.m_vertices[ib];

            GranitePoint const& c =
                geometry.m_vertices[ic];

            GraniteFaceOrigin const origin =
                geometry.m_triangleFaceOrigin[triangleIndex];

            GraniteSurfaceFaceClass const surfaceClass =
                geometry.m_triangleSurfaceClass[triangleIndex];

            Color faceColor =
                baseColor;

            // Diagnostic surface-memory certificate only:
            // fresh cut faces are brighter, inherited weathered exterior is
            // slightly darker. Every face remains Granite.
            if ( origin ==
                 GraniteFaceOrigin::FreshBreak )
            {
                faceColor =
                    baseColor.GetScaledColor(
                        1.16f );
            }
            else if ( surfaceClass ==
                      GraniteSurfaceFaceClass::WeatheredExterior )
            {
                faceColor =
                    baseColor.GetScaledColor(
                        0.86f );
            }

            Color const litColor =
                ApplyDiagnosticLighting(
                    faceColor,
                    a.m_x,
                    a.m_y,
                    a.m_z,
                    b.m_x,
                    b.m_y,
                    b.m_z,
                    c.m_x,
                    c.m_y,
                    c.m_z );

            drawCtx.DrawTriangle(
                Float3(
                    centerX +
                        a.m_x,
                    centerY +
                        a.m_y,
                    baseZ +
                        a.m_z ),
                Float3(
                    centerX +
                        b.m_x,
                    centerY +
                        b.m_y,
                    baseZ +
                        b.m_z ),
                Float3(
                    centerX +
                        c.m_x,
                    centerY +
                        c.m_y,
                    baseZ +
                        c.m_z ),
                litColor,
                DebugDrawLayer::World );
        }
    }

    //-------------------------------------------------------------------------

    static void DrawGraniteFractureDebugCache(
        DebugDrawContext&                drawCtx,
        GraniteFractureDebugCache const& cache )
    {
        if ( !cache.m_valid ||
             cache.m_transaction.m_numPrincipalChildren <
                 2 )
        {
            return;
        }

        GraniteFractureChild const& primary =
            cache.m_transaction.m_children[0];

        GraniteFractureChild const& secondary =
            cache.m_transaction.m_children[1];

        GranitePoint const normal =
            cache.m_transaction.m_plane.m_normal;

        float const horizontalLength =
            float(
                std::sqrt(
                    double(
                        normal.m_x *
                            normal.m_x +
                        normal.m_y *
                            normal.m_y ) ) );

        float separationX =
            cache.m_childSeparationM;

        float separationY =
            0.0f;

        if ( horizontalLength >
             0.0001f )
        {
            separationX =
                normal.m_x /
                horizontalLength *
                cache.m_childSeparationM;

            separationY =
                normal.m_y /
                horizontalLength *
                cache.m_childSeparationM;
        }

        Color const primaryColor =
            s_neutralMaterialDebug
                ? Color( 190, 190, 190 )
                : Color( 175, 175, 175 );

        Color const secondaryColor =
            s_neutralMaterialDebug
                ? Color( 190, 190, 190 )
                : Color( 145, 145, 145 );

        DrawGraniteFractureChildGeometry(
            drawCtx,
            primary,
            cache.m_displayCenterX -
                separationX,
            cache.m_displayCenterY -
                separationY,
            cache.m_displayBaseZ,
            primaryColor );

        DrawGraniteFractureChildGeometry(
            drawCtx,
            secondary,
            cache.m_displayCenterX +
                separationX,
            cache.m_displayCenterY +
                separationY,
            cache.m_displayBaseZ,
            secondaryColor );

        drawCtx.DrawText3D(
            Float3(
                cache.m_displayCenterX -
                    separationX,
                cache.m_displayCenterY -
                    separationY,
                cache.m_displayBaseZ +
                    0.42f ),
            "F301",
            Colors::White );

        drawCtx.DrawText3D(
            Float3(
                cache.m_displayCenterX +
                    separationX,
                cache.m_displayCenterY +
                    separationY,
                cache.m_displayBaseZ +
                    0.42f ),
            "F302",
            Colors::White );
    }

    //-------------------------------------------------------------------------
    // P3C.10B — rooted Granite detachment debug certificate
    //
    // This cache selects one ACTUAL RootedRockHead descriptor from the
    // deterministic Granite formation field, then runs the same structural
    // bridge set through:
    //
    //     rooted
    //       ->
    //     partially detached
    //       ->
    //     detached candidate
    //
    // The workbench copies below are presentation-only structural diagrams.
    // They do not create or move Granite matter.
    //-------------------------------------------------------------------------

    struct GraniteDetachmentDebugCache
    {
        bool     m_valid = false;
        uint32_t m_seed = 0;

        GraniteFormationFormDescriptor   m_rootedForm;
        GraniteDetachmentSequenceReceipt m_sequence;

        // P3C.10B-2:
        // finite local Granite partition executed only after the final
        // structural bridge releases parent ownership.
        GraniteRootedSeparationResult m_separation;

        // P3C.10C-1:
        // deterministic resolution of the conserved spall reserve into
        // explicit secondary MatterBody proposals + fine reserve.
        GraniteSpallResolutionResult m_spallResolution;

        // P3C.10C-2:
        // material-owned Granite geometry generated ONCE for those explicit
        // proposals. Never regenerate these meshes every debug frame.
        GraniteClosedGeometry m_spallGeometry[GraniteSpallResolutionResult::s_maxPieces];

        int32_t m_numSpallGeometry = 0;

        float    m_spallRequiredSolidVolumeM3 = 0.0f;
        float    m_spallMeasuredSolidVolumeM3 = 0.0f;
        float    m_spallMaximumVolumeResidualM3 = 0.0f;
        uint32_t m_spallGeometryMassGrams = 0;

        bool m_spallGeometryMassPass = false;
        bool m_spallGeometryVolumePass = false;
        bool m_spallGeometryTopologyPass = false;
        bool m_spallGeometryPass = false;

        float m_displayCenterY = 19.4f;
        float m_displayBaseZ = 0.12f;

        float m_rootedDisplayX = 7.0f;
        float m_partialDisplayX = 2.0f;
        float m_detachedDisplayX = -3.0f;
        float m_spallDisplayX = -6.2f;
    };

    //-------------------------------------------------------------------------
    // P3C.11A-1 player-facing Granite strike lab state
    //-------------------------------------------------------------------------

    struct GraniteToolStrikeLabState
    {
        ProvenanceWorldSystem* m_pOwner = nullptr;

        bool     m_initialized = false;
        bool     m_detached = false;
        uint32_t m_seed = 0;
        uint32_t m_revision = 1;
        uint32_t m_numAcceptedStrikes = 0;
        uint64_t m_lastCommandFrameID = ~uint64_t( 0 );

        GraniteDetachmentDebugCache m_baseline;
        GraniteStructuralBridgeSet  m_currentBridges;
        GraniteToolStrikeResult     m_lastStrike;

        GraniteRootedSeparationResult m_separation;
        GraniteSpallResolutionResult  m_spallResolution;
        GraniteTerminalConservationReceipt m_terminal;

        GraniteClosedGeometry
            m_spallGeometry[GraniteSpallResolutionResult::s_maxPieces];
        int32_t m_numSpallGeometry = 0;

        // The lab is an additional player target, spatially separated from
        // the unchanged P3C.10 certificate and its material witness.
        float m_targetCenterX = -3.0f;
        float m_targetCenterY = 27.0f;
        float m_targetBaseZ = 0.12f;

        bool  m_haveContact = false;
        float m_contactDisplayX = 0.0f;
        float m_contactDisplayY = 0.0f;
        float m_contactDisplayZ = 0.0f;

        // DebugDraw happens late in the frame, after the preview host has had
        // a chance to consume transient input edges. Keep a held-state latch
        // so a physical F press still produces exactly one strike command.
        bool m_strikeKeyHeld = false;
        bool m_strikeKeyHeldLastFrame = false;
        bool m_resetKeyHeldLastFrame = false;
        bool m_nextKeyHeldLastFrame = false;

        char m_status[256] =
            "READY - aim at the lower head seam and press F to strike";
    };

    static GraniteToolStrikeLabState s_graniteToolStrikeLab;

    //-------------------------------------------------------------------------
    // Runtime Granite material submission
    //
    // This mirrors the already-certified procedural outputs into the ordinary
    // clustered forward renderer. It does not replace or modify any DebugDraw
    // certificate triangles, geometry generation, or matter accounting.
    //-------------------------------------------------------------------------

    struct GraniteRuntimeMeshBuilder final
    {
        void AppendTriangle( Float3 const& a, Float3 const& b, Float3 const& c )
        {
            float const edgeABX = b.m_x - a.m_x;
            float const edgeABY = b.m_y - a.m_y;
            float const edgeABZ = b.m_z - a.m_z;

            float const edgeACX = c.m_x - a.m_x;
            float const edgeACY = c.m_y - a.m_y;
            float const edgeACZ = c.m_z - a.m_z;

            Float3 normal
            (
                edgeABY * edgeACZ - edgeABZ * edgeACY,
                edgeABZ * edgeACX - edgeABX * edgeACZ,
                edgeABX * edgeACY - edgeABY * edgeACX
            );

            float const normalLengthSquared =
                normal.m_x * normal.m_x +
                normal.m_y * normal.m_y +
                normal.m_z * normal.m_z;

            if ( normalLengthSquared <= 0.00000001f )
            {
                return;
            }

            float const inverseNormalLength =
                1.0f / std::sqrt( normalLengthSquared );

            normal.m_x *= inverseNormalLength;
            normal.m_y *= inverseNormalLength;
            normal.m_z *= inverseNormalLength;

            uint32_t const baseVertex = uint32_t( m_vertices.size() );
            m_vertices.emplace_back( Render::ProceduralMeshVertex{ a, normal } );
            m_vertices.emplace_back( Render::ProceduralMeshVertex{ b, normal } );
            m_vertices.emplace_back( Render::ProceduralMeshVertex{ c, normal } );

            m_indices.emplace_back( baseVertex + 0 );
            m_indices.emplace_back( baseVertex + 1 );
            m_indices.emplace_back( baseVertex + 2 );
        }

        void AppendRootedPartitionMesh
        (
            GraniteRootedPartitionMesh const&     mesh,
            GraniteFormationFormDescriptor const& sourceForm,
            float                                 displayCenterX,
            float                                 displayCenterY,
            float                                 displayZOffset
        )
        {
            for ( int32_t triangleIndex = 0;
                  triangleIndex < mesh.m_numTriangles;
                  ++triangleIndex )
            {
                int32_t const baseIndex = triangleIndex * 3;

                GraniteRootedPartitionVertex const& a =
                    mesh.m_vertices[mesh.m_triangleIndices[baseIndex + 0]];

                GraniteRootedPartitionVertex const& b =
                    mesh.m_vertices[mesh.m_triangleIndices[baseIndex + 1]];

                GraniteRootedPartitionVertex const& c =
                    mesh.m_vertices[mesh.m_triangleIndices[baseIndex + 2]];

                AppendTriangle
                (
                    Float3
                    (
                        displayCenterX + ( a.m_x - sourceForm.m_centerWorldX ),
                        displayCenterY + ( a.m_y - sourceForm.m_centerWorldY ),
                        a.m_z + displayZOffset
                    ),
                    Float3
                    (
                        displayCenterX + ( b.m_x - sourceForm.m_centerWorldX ),
                        displayCenterY + ( b.m_y - sourceForm.m_centerWorldY ),
                        b.m_z + displayZOffset
                    ),
                    Float3
                    (
                        displayCenterX + ( c.m_x - sourceForm.m_centerWorldX ),
                        displayCenterY + ( c.m_y - sourceForm.m_centerWorldY ),
                        c.m_z + displayZOffset
                    )
                );
            }
        }

        void AppendFormationSurface
        (
            GraniteFormationFormDescriptor const& form,
            float                                 displayCenterX,
            float                                 displayCenterY,
            float                                 displayBaseZ
        )
        {
            int32_t constexpr quadsPerAxis = 28;
            float const extentM = MaxFloat
            (
                MaxFloat( form.m_majorRadiusM, form.m_minorRadiusM ) +
                    form.m_rootBlendRadiusM * 1.35f,
                0.55f
            );
            float const spacingM = extentM * 2.0f / float( quadsPerAxis );

            for ( int32_t y = 0; y < quadsPerAxis; ++y )
            {
                float const localY0 = -extentM + float( y ) * spacingM;
                float const localY1 = localY0 + spacingM;

                for ( int32_t x = 0; x < quadsPerAxis; ++x )
                {
                    float const localX0 = -extentM + float( x ) * spacingM;
                    float const localX1 = localX0 + spacingM;

                    float const relief00 = MaxFloat
                    (
                        EvaluateGraniteFormationFormRelief
                        (
                            form,
                            form.m_centerWorldX + localX0,
                            form.m_centerWorldY + localY0
                        ),
                        0.0f
                    );
                    float const relief10 = MaxFloat
                    (
                        EvaluateGraniteFormationFormRelief
                        (
                            form,
                            form.m_centerWorldX + localX1,
                            form.m_centerWorldY + localY0
                        ),
                        0.0f
                    );
                    float const relief01 = MaxFloat
                    (
                        EvaluateGraniteFormationFormRelief
                        (
                            form,
                            form.m_centerWorldX + localX0,
                            form.m_centerWorldY + localY1
                        ),
                        0.0f
                    );
                    float const relief11 = MaxFloat
                    (
                        EvaluateGraniteFormationFormRelief
                        (
                            form,
                            form.m_centerWorldX + localX1,
                            form.m_centerWorldY + localY1
                        ),
                        0.0f
                    );

                    Float3 const p00
                    (
                        displayCenterX + localX0,
                        displayCenterY + localY0,
                        displayBaseZ + relief00
                    );
                    Float3 const p10
                    (
                        displayCenterX + localX1,
                        displayCenterY + localY0,
                        displayBaseZ + relief10
                    );
                    Float3 const p01
                    (
                        displayCenterX + localX0,
                        displayCenterY + localY1,
                        displayBaseZ + relief01
                    );
                    Float3 const p11
                    (
                        displayCenterX + localX1,
                        displayCenterY + localY1,
                        displayBaseZ + relief11
                    );

                    if ( MaxFloat( MaxFloat( relief00, relief10 ), relief11 ) > 0.0005f )
                    {
                        AppendTriangle( p00, p10, p11 );
                    }

                    if ( MaxFloat( MaxFloat( relief00, relief11 ), relief01 ) > 0.0005f )
                    {
                        AppendTriangle( p00, p11, p01 );
                    }
                }
            }
        }

        void AppendClosedGeometry
        (
            GraniteClosedGeometry const& geometry,
            float                        centerX,
            float                        centerY,
            float                        baseZ
        )
        {
            for ( int32_t triangleIndex = 0;
                  triangleIndex < geometry.m_numTriangles;
                  ++triangleIndex )
            {
                int32_t const baseIndex = triangleIndex * 3;

                GranitePoint const& a = geometry.m_vertices
                [
                    geometry.m_triangleIndices[baseIndex + 0]
                ];

                GranitePoint const& b = geometry.m_vertices
                [
                    geometry.m_triangleIndices[baseIndex + 1]
                ];

                GranitePoint const& c = geometry.m_vertices
                [
                    geometry.m_triangleIndices[baseIndex + 2]
                ];

                AppendTriangle
                (
                    Float3( centerX + a.m_x, centerY + a.m_y, baseZ + a.m_z ),
                    Float3( centerX + b.m_x, centerY + b.m_y, baseZ + b.m_z ),
                    Float3( centerX + c.m_x, centerY + c.m_y, baseZ + c.m_z )
                );
            }
        }

        TVector<Render::ProceduralMeshVertex> m_vertices;
        TVector<uint32_t> m_indices;
    };

    static Render::ProceduralMeshID RegisterGraniteRuntimeMaterialPreview
    (
        Render::RenderWorldSystem&             renderWorldSystem,
        Render::Material const*                pGraniteMaterial,
        GraniteDetachmentDebugCache const&     cache
    )
    {
        if ( pGraniteMaterial == nullptr ||
             !cache.m_separation.m_pass ||
             !cache.m_spallGeometryPass )
        {
            return 0;
        }

        GraniteRuntimeMeshBuilder builder;

        // Offset the materialized witness from the unchanged certificate so
        // both render paths remain independently visible and inspectable.
        float constexpr runtimePreviewYOffset = 3.0f;
        float const runtimeDisplayY =
            cache.m_displayCenterY + runtimePreviewYOffset;

        GraniteRootedSeparationResult const& separation = cache.m_separation;

        // Rooted parent/socket, including its newly exposed fracture interior.
        builder.AppendRootedPartitionMesh
        (
            separation.m_remainingParentSocketMesh,
            cache.m_rootedForm,
            cache.m_detachedDisplayX,
            runtimeDisplayY,
            cache.m_displayBaseZ
        );

        // Detached principal head, including the mold/cast underside.
        float const detachedLiftM = MaxFloat
        (
            0.18f,
            separation.m_detachedHeadMesh.m_extentZM * 0.22f
        );

        builder.AppendRootedPartitionMesh
        (
            separation.m_detachedHeadMesh,
            cache.m_rootedForm,
            cache.m_detachedDisplayX,
            runtimeDisplayY,
            cache.m_displayBaseZ + detachedLiftM
        );

        // All material-owned explicit spalls use this exact same material.
        float const spallBaseZ = cache.m_displayBaseZ + 0.16f;
        int32_t const spallCount = cache.m_numSpallGeometry <
                cache.m_spallResolution.m_numPieces
            ? cache.m_numSpallGeometry
            : cache.m_spallResolution.m_numPieces;

        for ( int32_t pieceIndex = 0;
              pieceIndex < spallCount;
              ++pieceIndex )
        {
            GraniteSpallPieceDescriptor const& piece =
                cache.m_spallResolution.m_pieces[pieceIndex];

            if ( !piece.m_valid )
            {
                continue;
            }

            builder.AppendClosedGeometry
            (
                cache.m_spallGeometry[pieceIndex],
                cache.m_spallDisplayX + piece.m_localOffsetX,
                runtimeDisplayY + piece.m_localOffsetY,
                spallBaseZ + piece.m_localOffsetZ
            );
        }

        return renderWorldSystem.RegisterProceduralMesh
        (
            builder.m_vertices,
            builder.m_indices,
            pGraniteMaterial,
            Transform::Identity,
            TBitFlags<Render::ViewLayer>
            (
                Render::ViewLayer::ShadowMap,
                Render::ViewLayer::ForwardShading
            )
        );
    }

    //-------------------------------------------------------------------------

    static char const* GetGraniteSurfaceStoneStateName(
        GraniteSurfaceStoneState state )
    {
        switch ( state )
        {
            case GraniteSurfaceStoneState::AttachedRockHead:
                return "AttachedRockHead";

            case GraniteSurfaceStoneState::PartiallyDetachedBlock:
                return "PartiallyDetached";

            case GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate:
                return "DetachedCandidate";

            default:
                return "Unknown";
        }
    }

    //-------------------------------------------------------------------------

    static GraniteFormationField QueryGraniteFormationFieldForDetachment(
        uint32_t seed,
        float    minWorldX,
        float    minWorldY,
        float    maxWorldX,
        float    maxWorldY )
    {
        GraniteFormationFieldRequest request;

        request.m_worldSeed =
            seed;

        request.m_geologicalAncestryID =
            s_graniteGeologicalAncestryID;

        request.m_minWorldX =
            minWorldX;

        request.m_minWorldY =
            minWorldY;

        request.m_maxWorldX =
            maxWorldX;

        request.m_maxWorldY =
            maxWorldY;

        return GenerateGraniteFormationField(
            request );
    }

    //-------------------------------------------------------------------------

    static bool SelectGraniteRootedRockHead(
        GraniteFormationField const&    field,
        GraniteFormationFormDescriptor& outRootedForm,
        float&                          inOutBestDistanceSquared )
    {
        bool found =
            false;

        for ( int32_t formIndex = 0;
              formIndex <
              field.m_numForms;
              ++formIndex )
        {
            GraniteFormationFormDescriptor const& form =
                field.m_forms[formIndex];

            if ( !form.m_valid ||
                 form.m_type !=
                     GraniteFormationFormType::RootedRockHead ||
                 form.m_attachment !=
                     GraniteFormationAttachmentState::RootedRockHead )
            {
                continue;
            }

            float const distanceSquared =
                form.m_centerWorldX *
                    form.m_centerWorldX +
                form.m_centerWorldY *
                    form.m_centerWorldY;

            if ( distanceSquared <
                 inOutBestDistanceSquared )
            {
                inOutBestDistanceSquared =
                    distanceSquared;

                outRootedForm =
                    form;

                found =
                    true;
            }
        }

        return found;
    }

    //-------------------------------------------------------------------------

    static bool FindNearestGraniteRootedRockHeadTiled(
        uint32_t                        worldSeed,
        float                           searchRadiusM,
        float                           tileSizeM,
        GraniteFormationFormDescriptor& outRootedForm )
    {
        EE_ASSERT(
            searchRadiusM >
            0.0f );

        EE_ASSERT(
            tileSizeM >
            0.0f );

        float bestDistanceSquared =
            1000000000.0f;

        bool found =
            false;

        int32_t const minTile =
            int32_t(
                std::floor(
                    double(
                        -searchRadiusM /
                        tileSizeM ) ) );

        int32_t const maxTile =
            int32_t(
                std::ceil(
                    double(
                        searchRadiusM /
                        tileSizeM ) ) ) -
            1;

        for ( int32_t tileY = minTile;
              tileY <=
              maxTile;
              ++tileY )
        {
            for ( int32_t tileX = minTile;
                  tileX <=
                  maxTile;
                  ++tileX )
            {
                float const minWorldX =
                    float(
                        tileX ) *
                    tileSizeM;

                float const minWorldY =
                    float(
                        tileY ) *
                    tileSizeM;

                float const maxWorldX =
                    minWorldX +
                    tileSizeM;

                float const maxWorldY =
                    minWorldY +
                    tileSizeM;

                // Skip tiles wholly outside the circular search radius.
                float const closestX =
                    minWorldX >
                            0.0f
                        ? minWorldX
                        : (
                              maxWorldX <
                                      0.0f
                                  ? maxWorldX
                                  : 0.0f );

                float const closestY =
                    minWorldY >
                            0.0f
                        ? minWorldY
                        : (
                              maxWorldY <
                                      0.0f
                                  ? maxWorldY
                                  : 0.0f );

                if ( closestX *
                             closestX +
                         closestY *
                             closestY >
                     searchRadiusM *
                         searchRadiusM )
                {
                    continue;
                }

                GraniteFormationField const field =
                    QueryGraniteFormationFieldForDetachment(
                        worldSeed,
                        minWorldX,
                        minWorldY,
                        maxWorldX,
                        maxWorldY );

                found =
                    SelectGraniteRootedRockHead(
                        field,
                        outRootedForm,
                        bestDistanceSquared ) ||
                    found;
            }
        }

        return found;
    }

    //-------------------------------------------------------------------------

    static bool BuildGraniteDetachmentDebugCache(
        GraniteDetachmentDebugCache& cache,
        uint32_t                     worldSeed )
    {
        ResetLargeDebugValueInPlace( cache );

        cache.m_seed =
            worldSeed;

        // P3C.10B debug selection must not depend on GraniteFormationField's
        // fixed top-N capacity across one huge query. Large fields keep only
        // the strongest 12 intersecting forms, which can legitimately omit
        // lower-importance RootedRockHeads.
        //
        // Search the SAME deterministic absolute-coordinate formation events
        // through small tiled windows instead. Query bounds only select which
        // already-deterministic descriptors are returned; they never alter
        // geological event generation.
        bool found =
            FindNearestGraniteRootedRockHeadTiled(
                worldSeed,
                32.0f,
                8.0f,
                cache.m_rootedForm );

        if ( !found )
        {
            found =
                FindNearestGraniteRootedRockHeadTiled(
                    worldSeed,
                    64.0f,
                    8.0f,
                    cache.m_rootedForm );
        }

        if ( !found )
        {
            return false;
        }

        GraniteRootedBridgeSetRequest request;

        request.m_worldSeed =
            worldSeed;

        request.m_structuralEventID =
            cache.m_rootedForm.m_eventID ^
            0x10B51001u;

        if ( request.m_structuralEventID ==
             0 )
        {
            request.m_structuralEventID =
                0x10B51001u;
        }

        request.m_rootedForm =
            cache.m_rootedForm;

        cache.m_sequence =
            CertifyGraniteRootedDetachmentSequence(
                request );

        if ( !cache.m_sequence.m_pass )
        {
            return false;
        }

        GraniteRootedSeparationRequest separationRequest;

        separationRequest.m_worldSeed =
            worldSeed;

        separationRequest.m_partitionEventID =
            cache.m_rootedForm.m_eventID ^
            0x10B200A1u;

        if ( separationRequest.m_partitionEventID ==
             0 )
        {
            separationRequest.m_partitionEventID =
                0x10B200A1u;
        }

        separationRequest.m_rootedForm =
            cache.m_rootedForm;

        separationRequest.m_finalDetachment =
            cache.m_sequence.m_finalEvent;

        // The witness is rendered relative to the local formation carrier.
        // The production terrain caller can supply its actual parent carrier
        // Z. The local partition volumes are invariant under vertical
        // translation.
        separationRequest.m_parentCarrierBaseZ =
            0.0f;

        cache.m_separation =
            BuildGraniteRootedSeparationTransaction(
                separationRequest );

        if ( !cache.m_separation.m_pass )
        {
            return false;
        }

        GraniteSpallResolutionRequest spallRequest;

        spallRequest.m_worldSeed =
            worldSeed;

        spallRequest.m_sourcePartitionEventID =
            cache.m_separation.m_partitionEventID;

        spallRequest.m_parentDetachedBodyID =
            cache.m_separation.m_detachedBodyID;

        spallRequest.m_parentProvenanceID =
            cache.m_separation.m_detachedProvenanceID;

        spallRequest.m_geologicalAncestryID =
            cache.m_separation.m_geologicalAncestryID;

        spallRequest.m_spallReserveMassGrams =
            cache.m_separation.m_matter.m_spallReserveMassGrams;

        spallRequest.m_originWorldX =
            cache.m_rootedForm.m_centerWorldX;

        spallRequest.m_originWorldY =
            cache.m_rootedForm.m_centerWorldY;

        spallRequest.m_originWorldZ =
            cache.m_displayBaseZ +
            cache.m_rootedForm.m_maximumReliefM;

        cache.m_spallResolution =
            ResolveGraniteSpallReserve(
                spallRequest );

        if ( !cache.m_spallResolution.m_valid )
        {
            return false;
        }

        //-------------------------------------------------------------------------
        // P3C.10C-2 — material-owned secondary-fragment geometry.
        //
        // Every explicit spall proposal now receives actual Granite geometry
        // generated from:
        //
        //     exact child grams
        //     + Granite density/material grammar
        //     + deterministic BodyID / ancestry
        //
        // The generated mesh is cached here. DebugDraw only renders the cache;
        // it never runs Granite generation every frame.
        //-------------------------------------------------------------------------

        cache.m_numSpallGeometry =
            0;

        cache.m_spallRequiredSolidVolumeM3 =
            0.0f;

        cache.m_spallMeasuredSolidVolumeM3 =
            0.0f;

        cache.m_spallMaximumVolumeResidualM3 =
            0.0f;

        cache.m_spallGeometryMassGrams =
            0;

        bool topologyPass =
            true;

        for ( int32_t pieceIndex = 0;
              pieceIndex <
              cache.m_spallResolution.m_numPieces;
              ++pieceIndex )
        {
            GraniteSpallPieceDescriptor const& piece =
                cache.m_spallResolution.m_pieces[pieceIndex];

            if ( !piece.m_valid )
            {
                topologyPass =
                    false;

                continue;
            }

            if ( cache.m_numSpallGeometry >=
                 GraniteSpallResolutionResult::s_maxPieces )
            {
                topologyPass =
                    false;

                break;
            }

            GraniteClosedGeometryRequest geometryRequest;

            geometryRequest.m_worldSeed =
                worldSeed;

            geometryRequest.m_geologicalAncestryID =
                piece.m_geologicalAncestryID;

            geometryRequest.m_bodyID =
                piece.m_bodyID;

            geometryRequest.m_massGrams =
                piece.m_massGrams;

            geometryRequest.m_bodyState =
                ProvenanceBodyState::Fragment;

            geometryRequest.m_scale =
                GraniteGeometryScale::Fragment;

            // Secondary spalls are newly exposed fracture matter. They should
            // not inherit the weathered exterior of the principal boulder.
            geometryRequest.m_weathering =
                GraniteWeatheringState::FreshFracture;

            geometryRequest.m_canInheritExteriorSurface =
                false;

            geometryRequest.m_parentExteriorExposure =
                0.0f;

            geometryRequest.m_worldX =
                cache.m_spallDisplayX +
                piece.m_localOffsetX;

            geometryRequest.m_worldY =
                cache.m_displayCenterY +
                piece.m_localOffsetY;

            geometryRequest.m_worldZ =
                cache.m_displayBaseZ +
                0.16f +
                piece.m_localOffsetZ;

            GraniteClosedGeometry geometry =
                GenerateGraniteClosedGeometry(
                    geometryRequest );

            topologyPass =
                topologyPass &&
                geometry.m_numVertices >=
                    4 &&
                geometry.m_numTriangles >=
                    4 &&
                geometry.m_measuredMeshVolumeM3 >
                    0.000001f;

            cache.m_spallRequiredSolidVolumeM3 +=
                geometry.m_requiredSolidVolumeM3;

            cache.m_spallMeasuredSolidVolumeM3 +=
                geometry.m_measuredMeshVolumeM3;

            cache.m_spallMaximumVolumeResidualM3 =
                MaxFloat(
                    cache.m_spallMaximumVolumeResidualM3,
                    AbsFloat(
                        geometry.m_volumeResidualM3 ) );

            cache.m_spallGeometryMassGrams +=
                piece.m_massGrams;

            cache.m_spallGeometry[cache.m_numSpallGeometry++] =
                geometry;
        }

        cache.m_spallGeometryMassPass =
            cache.m_spallGeometryMassGrams ==
            cache.m_spallResolution.m_receipt.m_explicitPieceMassGrams;

        float const spallVolumeResidualM3 =
            cache.m_spallMeasuredSolidVolumeM3 -
            cache.m_spallRequiredSolidVolumeM3;

        float const spallVolumeToleranceM3 =
            0.000001f +
            cache.m_spallRequiredSolidVolumeM3 *
                0.005f;

        cache.m_spallGeometryVolumePass =
            AbsFloat(
                spallVolumeResidualM3 ) <=
            spallVolumeToleranceM3;

        cache.m_spallGeometryTopologyPass =
            topologyPass &&
            cache.m_numSpallGeometry ==
                cache.m_spallResolution.m_receipt.m_numExplicitPieces;

        cache.m_spallGeometryPass =
            cache.m_spallGeometryMassPass &&
            cache.m_spallGeometryVolumePass &&
            cache.m_spallGeometryTopologyPass;

        cache.m_valid =
            cache.m_sequence.m_pass &&
            cache.m_separation.m_pass &&
            cache.m_spallResolution.m_valid &&
            cache.m_spallGeometryPass;

        return cache.m_valid;
    }

    //-------------------------------------------------------------------------

    static char const* GetGraniteToolStrikeRejectionName(
        GraniteToolStrikeRejectionReason reason )
    {
        switch ( reason )
        {
            case GraniteToolStrikeRejectionReason::None:
                return "None";
            case GraniteToolStrikeRejectionReason::InvalidRequest:
                return "InvalidRequest";
            case GraniteToolStrikeRejectionReason::InvalidContactFrame:
                return "InvalidContactFrame";
            case GraniteToolStrikeRejectionReason::UnsupportedMaterial:
                return "UnsupportedMaterial";
            case GraniteToolStrikeRejectionReason::NoIntactStructuralBridge:
                return "NoIntactStructuralBridge";
            case GraniteToolStrikeRejectionReason::NoBridgeInsideFootprint:
                return "NoBridgeInsideFootprint";
            case GraniteToolStrikeRejectionReason::DetachmentTransactionFailed:
                return "DetachmentTransactionFailed";
            default:
                return "Unknown";
        }
    }

    //-------------------------------------------------------------------------

    static bool InitializeGraniteToolStrikeLab(
        ProvenanceWorldSystem*       pOwner,
        uint32_t                     worldSeed,
        GraniteToolStrikeLabState&   state )
    {
        ResetLargeDebugValueInPlace( state );
        state.m_pOwner = pOwner;
        state.m_seed = worldSeed;

        if ( !BuildGraniteDetachmentDebugCache(
                 state.m_baseline,
                 worldSeed ) )
        {
            std::snprintf
            (
                state.m_status,
                sizeof( state.m_status ),
                "REFUSED - seed %u has no qualified rooted Granite target",
                worldSeed
            );
            return false;
        }

        state.m_currentBridges =
            state.m_baseline.m_sequence.m_initialRooted;
        state.m_separation = state.m_baseline.m_separation;
        state.m_spallResolution = state.m_baseline.m_spallResolution;
        state.m_terminal = BuildGraniteTerminalConservationReceipt
        (
            state.m_separation,
            state.m_spallResolution
        );
        state.m_numSpallGeometry = state.m_baseline.m_numSpallGeometry;

        for ( int32_t pieceIndex = 0;
              pieceIndex < state.m_numSpallGeometry;
              ++pieceIndex )
        {
            state.m_spallGeometry[pieceIndex] =
                state.m_baseline.m_spallGeometry[pieceIndex];
        }

        state.m_initialized = true;
        std::snprintf
        (
            state.m_status,
            sizeof( state.m_status ),
            "READY - seed %u | aim yellow [ + ]; release RMB; press F to strike",
            worldSeed
        );
        return true;
    }

    //-------------------------------------------------------------------------

    static bool BuildGraniteToolStrikeTerminalOutputs(
        GraniteToolStrikeLabState&       state,
        GraniteDetachmentResult const&   finalDetachment )
    {
        GraniteRootedSeparationRequest separationRequest;
        separationRequest.m_worldSeed = state.m_seed;
        separationRequest.m_partitionEventID =
            finalDetachment.m_eventID ^ 0x11A120A1u;

        if ( separationRequest.m_partitionEventID == 0 )
        {
            separationRequest.m_partitionEventID = 0x11A120A1u;
        }

        separationRequest.m_rootedForm = state.m_baseline.m_rootedForm;
        separationRequest.m_finalDetachment = finalDetachment;
        separationRequest.m_parentCarrierBaseZ = 0.0f;

        GraniteRootedSeparationResult const separation =
            BuildGraniteRootedSeparationTransaction( separationRequest );

        if ( !separation.m_pass )
        {
            return false;
        }

        GraniteSpallResolutionRequest spallRequest;
        spallRequest.m_worldSeed = state.m_seed;
        spallRequest.m_sourcePartitionEventID = separation.m_partitionEventID;
        spallRequest.m_parentDetachedBodyID = separation.m_detachedBodyID;
        spallRequest.m_parentProvenanceID = separation.m_detachedProvenanceID;
        spallRequest.m_geologicalAncestryID = separation.m_geologicalAncestryID;
        spallRequest.m_spallReserveMassGrams =
            separation.m_matter.m_spallReserveMassGrams;
        spallRequest.m_originWorldX =
            state.m_baseline.m_rootedForm.m_centerWorldX;
        spallRequest.m_originWorldY =
            state.m_baseline.m_rootedForm.m_centerWorldY;
        spallRequest.m_originWorldZ =
            state.m_baseline.m_rootedForm.m_maximumReliefM;
        spallRequest.m_policy.m_massResidualToleranceGrams = 0;

        GraniteSpallResolutionResult const spallResolution =
            ResolveGraniteSpallReserve( spallRequest );

        if ( !spallResolution.m_valid )
        {
            return false;
        }

        GraniteTerminalConservationReceipt const terminal =
            BuildGraniteTerminalConservationReceipt
            (
                separation,
                spallResolution
            );

        if ( !terminal.m_pass )
        {
            return false;
        }

        int32_t numGeneratedSpalls = 0;

        for ( int32_t pieceIndex = 0;
              pieceIndex < spallResolution.m_numPieces;
              ++pieceIndex )
        {
            GraniteSpallPieceDescriptor const& piece =
                spallResolution.m_pieces[pieceIndex];

            if ( !piece.m_valid ||
                 numGeneratedSpalls >=
                     GraniteSpallResolutionResult::s_maxPieces )
            {
                return false;
            }

            GraniteClosedGeometryRequest geometryRequest;
            geometryRequest.m_worldSeed = state.m_seed;
            geometryRequest.m_geologicalAncestryID =
                piece.m_geologicalAncestryID;
            geometryRequest.m_bodyID = piece.m_bodyID;
            geometryRequest.m_massGrams = piece.m_massGrams;
            geometryRequest.m_bodyState = ProvenanceBodyState::Fragment;
            geometryRequest.m_scale = GraniteGeometryScale::Fragment;
            geometryRequest.m_weathering =
                GraniteWeatheringState::FreshFracture;
            geometryRequest.m_canInheritExteriorSurface = false;
            geometryRequest.m_parentExteriorExposure = 0.0f;
            geometryRequest.m_worldX =
                state.m_baseline.m_rootedForm.m_centerWorldX +
                piece.m_localOffsetX;
            geometryRequest.m_worldY =
                state.m_baseline.m_rootedForm.m_centerWorldY +
                piece.m_localOffsetY;
            geometryRequest.m_worldZ =
                state.m_baseline.m_rootedForm.m_maximumReliefM +
                piece.m_localOffsetZ;

            GraniteClosedGeometry const geometry =
                GenerateGraniteClosedGeometry( geometryRequest );

            if ( geometry.m_numVertices < 4 ||
                 geometry.m_numTriangles < 4 ||
                 geometry.m_measuredMeshVolumeM3 <= 0.000001f )
            {
                return false;
            }

            // Stage directly in persistent lab storage. These meshes are not
            // submitted until the complete terminal chain passes and the lab
            // commits m_detached, so a second 16-mesh stack array is neither
            // required for atomicity nor safe on the editor's default stack.
            state.m_spallGeometry[numGeneratedSpalls++] = geometry;
        }

        if ( numGeneratedSpalls !=
             spallResolution.m_receipt.m_numExplicitPieces )
        {
            return false;
        }

        state.m_separation = separation;
        state.m_spallResolution = spallResolution;
        state.m_terminal = terminal;
        state.m_numSpallGeometry = numGeneratedSpalls;

        return true;
    }

    //-------------------------------------------------------------------------

    static Render::ProceduralMeshID RegisterGraniteToolStrikeRuntimePreview
    (
        Render::RenderWorldSystem&           renderWorldSystem,
        Render::Material const*              pGraniteMaterial,
        GraniteToolStrikeLabState const&     state
    )
    {
        if ( pGraniteMaterial == nullptr ||
             !state.m_initialized ||
             !state.m_separation.m_pass )
        {
            return 0;
        }

        GraniteRuntimeMeshBuilder builder;

        if ( !state.m_detached )
        {
            // ROOTED and PARTIAL are still one continuous formation. Do not
            // preview the future separation pieces merely touching at zero
            // lift: that made the unstruck rock look pre-cut and incorrectly
            // hid the original attachment fringe/curtains.
            builder.AppendFormationSurface
            (
                state.m_baseline.m_rootedForm,
                state.m_targetCenterX,
                state.m_targetCenterY,
                state.m_targetBaseZ
            );
        }
        else
        {
            builder.AppendRootedPartitionMesh
            (
                state.m_separation.m_remainingParentSocketMesh,
                state.m_baseline.m_rootedForm,
                state.m_targetCenterX,
                state.m_targetCenterY,
                state.m_targetBaseZ
            );

            float const detachedLiftM = MaxFloat
            (
                0.18f,
                state.m_separation.m_detachedHeadMesh.m_extentZM * 0.22f
            );

            builder.AppendRootedPartitionMesh
            (
                state.m_separation.m_detachedHeadMesh,
                state.m_baseline.m_rootedForm,
                state.m_targetCenterX,
                state.m_targetCenterY,
                state.m_targetBaseZ + detachedLiftM
            );

            for ( int32_t pieceIndex = 0;
                  pieceIndex < state.m_numSpallGeometry;
                  ++pieceIndex )
            {
                GraniteSpallPieceDescriptor const& piece =
                    state.m_spallResolution.m_pieces[pieceIndex];

                if ( !piece.m_valid )
                {
                    continue;
                }

                builder.AppendClosedGeometry
                (
                    state.m_spallGeometry[pieceIndex],
                    state.m_targetCenterX - 2.6f + piece.m_localOffsetX,
                    state.m_targetCenterY + piece.m_localOffsetY,
                    state.m_targetBaseZ + 0.16f + piece.m_localOffsetZ
                );
            }
        }

        return renderWorldSystem.RegisterProceduralMesh
        (
            builder.m_vertices,
            builder.m_indices,
            pGraniteMaterial,
            Transform::Identity,
            TBitFlags<Render::ViewLayer>
            (
                Render::ViewLayer::ShadowMap,
                Render::ViewLayer::ForwardShading
            )
        );
    }

    //-------------------------------------------------------------------------

    #include "GraniteExcavationLab.inl"

    static bool RaycastGraniteToolStrikeHead
    (
        GraniteToolStrikeLabState const& state,
        Float3 const&                    rayOrigin,
        Float3 const&                    rayDirection,
        Float3&                          outDisplayContact,
        Float3&                          outSourceContact,
        Float3&                          outSurfaceNormal
    )
    {
        // Strike the geometry the player actually sees. Before release that
        // is the continuous original formation, not the hidden future cast.
        GraniteRuntimeMeshBuilder visibleHead;
        visibleHead.AppendFormationSurface
        (
            state.m_baseline.m_rootedForm,
            state.m_targetCenterX,
            state.m_targetCenterY,
            state.m_targetBaseZ
        );

        float closestT = 12.0f;
        bool hit = false;

        for ( int32_t indexBase = 0;
              indexBase + 2 < int32_t( visibleHead.m_indices.size() );
              indexBase += 3 )
        {
            Float3 const& a = visibleHead.m_vertices
            [
                visibleHead.m_indices[indexBase + 0]
            ].m_position;
            Float3 const& b = visibleHead.m_vertices
            [
                visibleHead.m_indices[indexBase + 1]
            ].m_position;
            Float3 const& c = visibleHead.m_vertices
            [
                visibleHead.m_indices[indexBase + 2]
            ].m_position;

            float const edge1X = b.m_x - a.m_x;
            float const edge1Y = b.m_y - a.m_y;
            float const edge1Z = b.m_z - a.m_z;
            float const edge2X = c.m_x - a.m_x;
            float const edge2Y = c.m_y - a.m_y;
            float const edge2Z = c.m_z - a.m_z;

            float const pX =
                rayDirection.m_y * edge2Z - rayDirection.m_z * edge2Y;
            float const pY =
                rayDirection.m_z * edge2X - rayDirection.m_x * edge2Z;
            float const pZ =
                rayDirection.m_x * edge2Y - rayDirection.m_y * edge2X;
            float const determinant =
                edge1X * pX + edge1Y * pY + edge1Z * pZ;

            if ( AbsFloat( determinant ) <= 0.0000001f )
            {
                continue;
            }

            float const inverseDeterminant = 1.0f / determinant;
            float const toOriginX = rayOrigin.m_x - a.m_x;
            float const toOriginY = rayOrigin.m_y - a.m_y;
            float const toOriginZ = rayOrigin.m_z - a.m_z;
            float const u =
                ( toOriginX * pX + toOriginY * pY + toOriginZ * pZ ) *
                inverseDeterminant;

            if ( u < 0.0f || u > 1.0f )
            {
                continue;
            }

            float const qX = toOriginY * edge1Z - toOriginZ * edge1Y;
            float const qY = toOriginZ * edge1X - toOriginX * edge1Z;
            float const qZ = toOriginX * edge1Y - toOriginY * edge1X;
            float const v =
                ( rayDirection.m_x * qX +
                  rayDirection.m_y * qY +
                  rayDirection.m_z * qZ ) *
                inverseDeterminant;

            if ( v < 0.0f || u + v > 1.0f )
            {
                continue;
            }

            float const t =
                ( edge2X * qX + edge2Y * qY + edge2Z * qZ ) *
                inverseDeterminant;

            if ( t <= 0.0f || t >= closestT )
            {
                continue;
            }

            float normalX = edge1Y * edge2Z - edge1Z * edge2Y;
            float normalY = edge1Z * edge2X - edge1X * edge2Z;
            float normalZ = edge1X * edge2Y - edge1Y * edge2X;
            float const normalLengthSquared =
                normalX * normalX + normalY * normalY + normalZ * normalZ;

            if ( normalLengthSquared <= 0.00000001f )
            {
                continue;
            }

            float const inverseNormalLength =
                1.0f / float( std::sqrt( double( normalLengthSquared ) ) );
            normalX *= inverseNormalLength;
            normalY *= inverseNormalLength;
            normalZ *= inverseNormalLength;

            // Present an outward normal against the incoming ray even where a
            // source triangle happens to carry the opposite winding.
            if ( normalX * rayDirection.m_x +
                     normalY * rayDirection.m_y +
                     normalZ * rayDirection.m_z >
                 0.0f )
            {
                normalX = -normalX;
                normalY = -normalY;
                normalZ = -normalZ;
            }

            closestT = t;
            hit = true;
            outDisplayContact = Float3
            (
                rayOrigin.m_x + rayDirection.m_x * t,
                rayOrigin.m_y + rayDirection.m_y * t,
                rayOrigin.m_z + rayDirection.m_z * t
            );
            outSourceContact = Float3
            (
                state.m_baseline.m_rootedForm.m_centerWorldX +
                    ( outDisplayContact.m_x - state.m_targetCenterX ),
                state.m_baseline.m_rootedForm.m_centerWorldY +
                    ( outDisplayContact.m_y - state.m_targetCenterY ),
                outDisplayContact.m_z - state.m_targetBaseZ
            );
            outSurfaceNormal = Float3( normalX, normalY, normalZ );
        }

        return hit;
    }

    //-------------------------------------------------------------------------

    bool ProvenanceWorldSystem::TryGraniteToolStrike
    (
        EntityWorldUpdateContext const& ctx,
        Float3 const&                   rayOrigin,
        Float3 const&                   rayDirection
    )
    {
        if ( m_pSettings == nullptr || !ctx.IsGameWorld() || ExcavationLab::state.active )
        {
            return false;
        }

        GraniteToolStrikeLabState& state = s_graniteToolStrikeLab;

        if ( state.m_pOwner != this || !state.m_initialized )
        {
            if ( !InitializeGraniteToolStrikeLab
                 (
                     this,
                     m_pSettings->GetSeed(),
                     state
                 ) )
            {
                return false;
            }
        }

        // Both a real PlayerSystem and the editor-preview tools camera can
        // drive this lab. Record the frame here so the viewport fallback does
        // not apply the same physical click twice when a player is present.
        state.m_lastCommandFrameID = ctx.GetFrameID();

        if ( state.m_detached )
        {
            std::snprintf
            (
                state.m_status,
                sizeof( state.m_status ),
                "COMPLETE - terminal conservation PASS | press R to reset"
            );
            return true;
        }

        float const directionLengthSquared =
            rayDirection.m_x * rayDirection.m_x +
            rayDirection.m_y * rayDirection.m_y +
            rayDirection.m_z * rayDirection.m_z;

        if ( directionLengthSquared <= 0.00000001f )
        {
            std::snprintf
            (
                state.m_status,
                sizeof( state.m_status ),
                "MISS - active camera supplied an invalid strike direction"
            );
            return false;
        }

        float const inverseDirectionLength =
            1.0f / float( std::sqrt( double( directionLengthSquared ) ) );
        Float3 const direction
        (
            rayDirection.m_x * inverseDirectionLength,
            rayDirection.m_y * inverseDirectionLength,
            rayDirection.m_z * inverseDirectionLength
        );

        Float3 displayContact;
        Float3 sourceContact;
        Float3 surfaceNormal;

        if ( !RaycastGraniteToolStrikeHead
             (
                 state,
                 rayOrigin,
                 direction,
                 displayContact,
                 sourceContact,
                 surfaceNormal
             ) )
        {
            state.m_haveContact = false;
            std::snprintf
            (
                state.m_status,
                sizeof( state.m_status ),
                "MISS - center + ray did not intersect the strike-lab head | aim at lower seam"
            );
            return false;
        }

        GraniteToolStrikeRequest request;
        request.m_worldSeed = state.m_seed;
        request.m_priorStrikeCount = state.m_numAcceptedStrikes;
        request.m_strikeEventID =
            state.m_baseline.m_rootedForm.m_eventID ^
            0x11A10000u ^
            ( 0x9E3779B9u * ( state.m_numAcceptedStrikes + 1u ) );

        if ( request.m_strikeEventID == 0 )
        {
            request.m_strikeEventID = 0x11A10001u;
        }

        request.m_rootedForm = state.m_baseline.m_rootedForm;
        request.m_before = state.m_currentBridges;
        request.m_contactWorldX = sourceContact.m_x;
        request.m_contactWorldY = sourceContact.m_y;
        request.m_contactWorldZ = sourceContact.m_z;
        request.m_surfaceNormalX = surfaceNormal.m_x;
        request.m_surfaceNormalY = surfaceNormal.m_y;
        request.m_surfaceNormalZ = surfaceNormal.m_z;
        request.m_incomingDirectionX = direction.m_x;
        request.m_incomingDirectionY = direction.m_y;
        request.m_incomingDirectionZ = direction.m_z;

        GraniteToolStrikeResult const strike =
            ApplyGraniteToolStrike( request );

        state.m_lastStrike = strike;
        state.m_haveContact = true;
        state.m_contactDisplayX = displayContact.m_x;
        state.m_contactDisplayY = displayContact.m_y;
        state.m_contactDisplayZ = displayContact.m_z;

        if ( !strike.m_pass )
        {
            std::snprintf
            (
                state.m_status,
                sizeof( state.m_status ),
                "REFUSED - %s | contact did not mutate bridge/matter state",
                GetGraniteToolStrikeRejectionName( strike.m_rejectionReason )
            );
            ++state.m_revision;
            return true;
        }

        bool const releasesParent =
            strike.m_detachment.m_detachedCandidateReady &&
            !strike.m_detachment.m_parentStructuralOwnershipRetained;

        if ( releasesParent &&
             !BuildGraniteToolStrikeTerminalOutputs
              (
                  state,
                  strike.m_detachment
              ) )
        {
            std::snprintf
            (
                state.m_status,
                sizeof( state.m_status ),
                "REFUSED - terminal separation/spall/conservation chain failed atomically"
            );
            ++state.m_revision;
            return true;
        }

        state.m_currentBridges = strike.m_detachment.m_after;
        ++state.m_numAcceptedStrikes;
        state.m_detached = releasesParent;
        ++state.m_revision;

        if ( state.m_detached )
        {
            std::snprintf
            (
                state.m_status,
                sizeof( state.m_status ),
                "DETACHED - %u strikes | parent %u g + head %u g + spalls/fines | terminal dM %lld g PASS",
                state.m_numAcceptedStrikes,
                state.m_terminal.m_remainingParentMassGrams,
                state.m_terminal.m_detachedMassGrams,
                (long long) state.m_terminal.m_terminalResidualGrams
            );
        }
        else
        {
            std::snprintf
            (
                state.m_status,
                sizeof( state.m_status ),
                "ACCEPTED - bridge %d severed | %d/%d intact | parent ownership retained",
                strike.m_selectedBridgeIndex,
                state.m_currentBridges.m_numIntactBridges,
                state.m_currentBridges.m_numBridges
            );
        }

        Render::RenderWorldSystem* pRenderWorldSystem =
            ctx.GetWorldSystem<Render::RenderWorldSystem>();

        if ( pRenderWorldSystem != nullptr )
        {
            if ( m_graniteToolStrikeRuntimeMeshID != 0 &&
                 m_pGraniteRenderWorldSystem != nullptr )
            {
                m_pGraniteRenderWorldSystem->UnregisterProceduralMesh
                (
                    m_graniteToolStrikeRuntimeMeshID
                );
            }

            m_pGraniteRenderWorldSystem = pRenderWorldSystem;
            m_graniteToolStrikeRuntimeMeshID =
                RegisterGraniteToolStrikeRuntimePreview
                (
                    *pRenderWorldSystem,
                    m_pSettings->GetGraniteMaterial(),
                    state
                );
            m_graniteToolStrikeRuntimeRevision = state.m_revision;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void ProvenanceWorldSystem::ResetGraniteToolStrikeLab
    (
        EntityWorldUpdateContext const& ctx,
        bool                            advanceSeed
    )
    {
        if ( m_pSettings == nullptr || !ctx.IsGameWorld() || ExcavationLab::state.active )
        {
            return;
        }

        static constexpr uint32_t qualifiedSeeds[] =
        {
            8675309u,
            0x00C0FFEEu,
            0x5EED1234u,
            0x5EED1235u
        };

        uint32_t requestedSeed = m_pSettings->GetSeed();

        if ( s_graniteToolStrikeLab.m_pOwner == this &&
             s_graniteToolStrikeLab.m_initialized )
        {
            requestedSeed = s_graniteToolStrikeLab.m_seed;
        }

        if ( advanceSeed )
        {
            int32_t currentIndex = -1;

            for ( int32_t index = 0;
                  index < int32_t( sizeof( qualifiedSeeds ) / sizeof( qualifiedSeeds[0] ) );
                  ++index )
            {
                if ( qualifiedSeeds[index] == requestedSeed )
                {
                    currentIndex = index;
                    break;
                }
            }

            requestedSeed = qualifiedSeeds
            [
                ( currentIndex + 1 ) %
                int32_t( sizeof( qualifiedSeeds ) / sizeof( qualifiedSeeds[0] ) )
            ];
        }

        bool const preserveStrikeKeyLatch =
            s_graniteToolStrikeLab.m_pOwner == this &&
            s_graniteToolStrikeLab.m_strikeKeyHeldLastFrame;
        bool const preserveResetKeyLatch =
            s_graniteToolStrikeLab.m_pOwner == this &&
            s_graniteToolStrikeLab.m_resetKeyHeldLastFrame;
        bool const preserveNextKeyLatch =
            s_graniteToolStrikeLab.m_pOwner == this &&
            s_graniteToolStrikeLab.m_nextKeyHeldLastFrame;

        if ( m_graniteToolStrikeRuntimeMeshID != 0 &&
             m_pGraniteRenderWorldSystem != nullptr )
        {
            m_pGraniteRenderWorldSystem->UnregisterProceduralMesh
            (
                m_graniteToolStrikeRuntimeMeshID
            );
            m_graniteToolStrikeRuntimeMeshID = 0;
        }

        InitializeGraniteToolStrikeLab
        (
            this,
            requestedSeed,
            s_graniteToolStrikeLab
        );
        s_graniteToolStrikeLab.m_strikeKeyHeldLastFrame =
            preserveStrikeKeyLatch;
        s_graniteToolStrikeLab.m_resetKeyHeldLastFrame =
            preserveResetKeyLatch;
        s_graniteToolStrikeLab.m_nextKeyHeldLastFrame =
            preserveNextKeyLatch;
        s_graniteToolStrikeLab.m_lastCommandFrameID = ctx.GetFrameID();

        Render::RenderWorldSystem* pRenderWorldSystem =
            ctx.GetWorldSystem<Render::RenderWorldSystem>();

        if ( pRenderWorldSystem != nullptr &&
             s_graniteToolStrikeLab.m_initialized )
        {
            m_pGraniteRenderWorldSystem = pRenderWorldSystem;
            m_graniteToolStrikeRuntimeMeshID =
                RegisterGraniteToolStrikeRuntimePreview
                (
                    *pRenderWorldSystem,
                    m_pSettings->GetGraniteMaterial(),
                    s_graniteToolStrikeLab
                );
            m_graniteToolStrikeRuntimeRevision =
                s_graniteToolStrikeLab.m_revision;
        }
    }

    //-------------------------------------------------------------------------

    void ProvenanceWorldSystem::ReleaseDebugWorkbenchState()
    {
        ExcavationLab::Release(this);
        if ( s_graniteToolStrikeLab.m_pOwner == this )
        {
            ResetLargeDebugValueInPlace( s_graniteToolStrikeLab );
        }
    }

    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // P3C.10B-1 — actual RootedRockHead geometry witness
    //
    // The selected formation descriptor is evaluated through
    // EvaluateGraniteFormationFormRelief(), i.e. the same material-owned
    // formation grammar used by the terrain system.
    //
    // We render THREE translated copies of the exact same descriptor:
    //
    //     ROOTED
    //     PARTIAL
    //     DETACHED CANDIDATE
    //
    // The Granite shape is deliberately unchanged between copies. Only the
    // structural bridge state and final presentation separation change.
    //-------------------------------------------------------------------------

    struct GraniteFormationWitnessSample
    {
        float m_localX = 0.0f;
        float m_localY = 0.0f;
        float m_reliefM = 0.0f;
    };

    //-------------------------------------------------------------------------

    static float GetGraniteFormationWitnessExtentM(
        GraniteFormationFormDescriptor const& form )
    {
        return MaxFloat(
            MaxFloat(
                form.m_majorRadiusM,
                form.m_minorRadiusM ) +
                form.m_rootBlendRadiusM *
                    1.35f,
            0.55f );
    }

    //-------------------------------------------------------------------------

    static GraniteFormationWitnessSample EvaluateGraniteFormationWitnessSample(
        GraniteFormationFormDescriptor const& form,
        float                                 localX,
        float                                 localY )
    {
        GraniteFormationWitnessSample sample;

        sample.m_localX =
            localX;

        sample.m_localY =
            localY;

        sample.m_reliefM =
            MaxFloat(
                EvaluateGraniteFormationFormRelief(
                    form,
                    form.m_centerWorldX +
                        localX,
                    form.m_centerWorldY +
                        localY ),
                0.0f );

        return sample;
    }

    //-------------------------------------------------------------------------

    static Color GetGraniteDetachmentStageColor(
        GraniteSurfaceStoneState state )
    {
        switch ( state )
        {
            case GraniteSurfaceStoneState::AttachedRockHead:
                return Color(
                    145,
                    145,
                    145 );

            case GraniteSurfaceStoneState::PartiallyDetachedBlock:
                return Color(
                    165,
                    165,
                    165 );

            case GraniteSurfaceStoneState::DetachedSurfaceStoneCandidate:
                return Color(
                    188,
                    188,
                    188 );

            default:
                return Color(
                    155,
                    155,
                    155 );
        }
    }

    //-------------------------------------------------------------------------

    static bool DrawGraniteFormationWitnessTriangle(
        DebugDrawContext&                    drawCtx,
        GraniteFormationWitnessSample const& a,
        GraniteFormationWitnessSample const& b,
        GraniteFormationWitnessSample const& c,
        float                                displayCenterX,
        float                                displayCenterY,
        float                                displayBaseZ,
        Color const&                         baseColor )
    {
        // Do not manufacture a flat square floor around the formation.
        // A triangle must contain actual positive formation relief.
        float const maximumRelief =
            MaxFloat(
                MaxFloat(
                    a.m_reliefM,
                    b.m_reliefM ),
                c.m_reliefM );

        if ( maximumRelief <=
             0.0005f )
        {
            return false;
        }

        Float3 const va(
            displayCenterX +
                a.m_localX,
            displayCenterY +
                a.m_localY,
            displayBaseZ +
                a.m_reliefM );

        Float3 const vb(
            displayCenterX +
                b.m_localX,
            displayCenterY +
                b.m_localY,
            displayBaseZ +
                b.m_reliefM );

        Float3 const vc(
            displayCenterX +
                c.m_localX,
            displayCenterY +
                c.m_localY,
            displayBaseZ +
                c.m_reliefM );

        Color const litColor =
            ApplyDiagnosticLighting(
                baseColor,
                va.m_x,
                va.m_y,
                va.m_z,
                vb.m_x,
                vb.m_y,
                vb.m_z,
                vc.m_x,
                vc.m_y,
                vc.m_z );

        drawCtx.DrawTriangle(
            va,
            vb,
            vc,
            litColor,
            DebugDrawLayer::World );

        return true;
    }

    //-------------------------------------------------------------------------

    static void DrawGraniteRootedHeadGeometryWitness(
        DebugDrawContext&                     drawCtx,
        GraniteFormationFormDescriptor const& form,
        GraniteSurfaceStoneState              state,
        float                                 displayCenterX,
        float                                 displayCenterY,
        float                                 displayBaseZ )
    {
        int32_t constexpr quadsPerAxis =
            28;

        int32_t constexpr samplesPerAxis =
            quadsPerAxis +
            1;

        GraniteFormationWitnessSample samples[samplesPerAxis *
                                              samplesPerAxis];

        float const extentM =
            GetGraniteFormationWitnessExtentM(
                form );

        float const spacingM =
            ( extentM *
              2.0f ) /
            float(
                quadsPerAxis );

        for ( int32_t y = 0;
              y <
              samplesPerAxis;
              ++y )
        {
            float const localY =
                -extentM +
                float(
                    y ) *
                    spacingM;

            for ( int32_t x = 0;
                  x <
                  samplesPerAxis;
                  ++x )
            {
                float const localX =
                    -extentM +
                    float(
                        x ) *
                        spacingM;

                samples[y *
                            samplesPerAxis +
                        x] =
                    EvaluateGraniteFormationWitnessSample(
                        form,
                        localX,
                        localY );
            }
        }

        Color const bodyColor =
            GetGraniteDetachmentStageColor(
                state );

        for ( int32_t y = 0;
              y <
              quadsPerAxis;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  quadsPerAxis;
                  ++x )
            {
                int32_t const i00 =
                    y *
                        samplesPerAxis +
                    x;

                int32_t const i10 =
                    y *
                        samplesPerAxis +
                    x +
                    1;

                int32_t const i01 =
                    ( y +
                      1 ) *
                        samplesPerAxis +
                    x;

                int32_t const i11 =
                    ( y +
                      1 ) *
                        samplesPerAxis +
                    x +
                    1;

                // Same diagonal convention as the terrain preview.
                DrawGraniteFormationWitnessTriangle(
                    drawCtx,
                    samples[i00],
                    samples[i10],
                    samples[i11],
                    displayCenterX,
                    displayCenterY,
                    displayBaseZ,
                    bodyColor );

                DrawGraniteFormationWitnessTriangle(
                    drawCtx,
                    samples[i00],
                    samples[i11],
                    samples[i01],
                    displayCenterX,
                    displayCenterY,
                    displayBaseZ,
                    bodyColor );
            }
        }
    }

    //-------------------------------------------------------------------------

    static void DrawGraniteStructuralBridgesOnWitness(
        DebugDrawContext&                     drawCtx,
        GraniteFormationFormDescriptor const& rootedForm,
        GraniteStructuralBridgeSet const&     bridgeSet,
        float                                 displayCenterX,
        float                                 displayCenterY,
        float                                 displayBaseZ )
    {
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

            float const localX =
                bridge.m_centerWorldX -
                rootedForm.m_centerWorldX;

            float const localY =
                bridge.m_centerWorldY -
                rootedForm.m_centerWorldY;

            float const reliefAtBridge =
                MaxFloat(
                    EvaluateGraniteFormationFormRelief(
                        rootedForm,
                        bridge.m_centerWorldX,
                        bridge.m_centerWorldY ),
                    0.0f );

            float const bridgeTopZ =
                displayBaseZ +
                reliefAtBridge *
                    0.42f +
                0.015f;

            float localLength =
                float(
                    std::sqrt(
                        double(
                            localX *
                                localX +
                            localY *
                                localY ) ) );

            float dirX =
                1.0f;

            float dirY =
                0.0f;

            if ( localLength >
                 0.0001f )
            {
                dirX =
                    localX /
                    localLength;

                dirY =
                    localY /
                    localLength;
            }

            float const halfSpan =
                MaxFloat(
                    bridge.m_spanM *
                        0.55f,
                    0.035f );

            float const tangentX =
                -dirY;

            float const tangentY =
                dirX;

            Color const bridgeColor =
                bridge.m_severed
                    ? Colors::Red
                    : Colors::White;

            // Finite bridge witness across the root attachment region.
            drawCtx.DrawLine(
                Float3(
                    displayCenterX +
                        localX -
                        tangentX *
                            halfSpan,
                    displayCenterY +
                        localY -
                        tangentY *
                            halfSpan,
                    bridgeTopZ ),
                Float3(
                    displayCenterX +
                        localX +
                        tangentX *
                            halfSpan,
                    displayCenterY +
                        localY +
                        tangentY *
                            halfSpan,
                    bridgeTopZ ),
                bridgeColor,
                bridge.m_severed
                    ? 2.0f
                    : 5.0f,
                DebugDrawLayer::World );

            if ( bridge.m_severed )
            {
                // Small gap witness: the red split is deliberately offset
                // along the outward structural axis.
                float const gap =
                    0.035f;

                drawCtx.DrawLine(
                    Float3(
                        displayCenterX +
                            localX -
                            dirX *
                                gap,
                        displayCenterY +
                            localY -
                            dirY *
                                gap,
                        bridgeTopZ +
                            0.018f ),
                    Float3(
                        displayCenterX +
                            localX +
                            dirX *
                                gap,
                        displayCenterY +
                            localY +
                            dirY *
                                gap,
                        bridgeTopZ -
                            0.018f ),
                    Colors::Red,
                    2.0f,
                    DebugDrawLayer::World );
            }
        }
    }

    //-------------------------------------------------------------------------

    static void DrawGraniteDetachmentStageGeometry(
        DebugDrawContext&                     drawCtx,
        GraniteFormationFormDescriptor const& rootedForm,
        GraniteStructuralBridgeSet const&     bridgeSet,
        GraniteSurfaceStoneState              state,
        float                                 displayCenterX,
        float                                 displayCenterY,
        float                                 displayBaseZ,
        char const*                           pLabel )
    {
        DrawGraniteRootedHeadGeometryWitness(
            drawCtx,
            rootedForm,
            state,
            displayCenterX,
            displayCenterY,
            displayBaseZ );

        DrawGraniteStructuralBridgesOnWitness(
            drawCtx,
            rootedForm,
            bridgeSet,
            displayCenterX,
            displayCenterY,
            displayBaseZ );

        drawCtx.DrawText3D(
            Float3(
                displayCenterX,
                displayCenterY,
                displayBaseZ +
                    rootedForm.m_maximumReliefM +
                    0.24f ),
            pLabel,
            Colors::White );
    }

    //-------------------------------------------------------------------------
    // P3C.10B-2 partition mesh renderer
    //
    // GraniteRootedPartitionMesh vertices are stored in the source form's
    // absolute world XY. Translate them into the workbench witness position
    // without changing the partition geometry itself.
    //-------------------------------------------------------------------------

    static void DrawGraniteRootedPartitionMesh(
        DebugDrawContext&                     drawCtx,
        GraniteRootedPartitionMesh const&     mesh,
        GraniteFormationFormDescriptor const& sourceForm,
        float                                 displayCenterX,
        float                                 displayCenterY,
        float                                 displayZOffset,
        Color const&                          baseColor )
    {
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

            Float3 const va(
                displayCenterX +
                    ( a.m_x -
                      sourceForm.m_centerWorldX ),
                displayCenterY +
                    ( a.m_y -
                      sourceForm.m_centerWorldY ),
                a.m_z +
                    displayZOffset );

            Float3 const vb(
                displayCenterX +
                    ( b.m_x -
                      sourceForm.m_centerWorldX ),
                displayCenterY +
                    ( b.m_y -
                      sourceForm.m_centerWorldY ),
                b.m_z +
                    displayZOffset );

            Float3 const vc(
                displayCenterX +
                    ( c.m_x -
                      sourceForm.m_centerWorldX ),
                displayCenterY +
                    ( c.m_y -
                      sourceForm.m_centerWorldY ),
                c.m_z +
                    displayZOffset );

            Color const litColor =
                ApplyDiagnosticLighting(
                    baseColor,
                    va.m_x,
                    va.m_y,
                    va.m_z,
                    vb.m_x,
                    vb.m_y,
                    vb.m_z,
                    vc.m_x,
                    vc.m_y,
                    vc.m_z );

            drawCtx.DrawTriangle(
                va,
                vb,
                vc,
                litColor,
                DebugDrawLayer::World );
        }
    }

    //-------------------------------------------------------------------------

    static void DrawGraniteDetachedPartitionStage(
        DebugDrawContext&                  drawCtx,
        GraniteDetachmentDebugCache const& cache )
    {
        GraniteRootedSeparationResult const& separation =
            cache.m_separation;

        if ( !separation.m_pass )
        {
            return;
        }

        // The parent socket remains exactly on its original local carrier.
        Color const socketColor =
            Color(
                118,
                118,
                118 );

        DrawGraniteRootedPartitionMesh(
            drawCtx,
            separation.m_remainingParentSocketMesh,
            cache.m_rootedForm,
            cache.m_detachedDisplayX,
            cache.m_displayCenterY,
            cache.m_displayBaseZ,
            socketColor );

        // Only the newly-independent detached Granite parcel is lifted for
        // diagnostic readability. This is presentation separation, not a
        // physics impulse.
        float const detachedLiftM =
            MaxFloat(
                0.18f,
                separation.m_detachedHeadMesh.m_extentZM *
                    0.22f );

        Color const detachedColor =
            Color(
                198,
                198,
                198 );

        DrawGraniteRootedPartitionMesh(
            drawCtx,
            separation.m_detachedHeadMesh,
            cache.m_rootedForm,
            cache.m_detachedDisplayX,
            cache.m_displayCenterY,
            cache.m_displayBaseZ +
                detachedLiftM,
            detachedColor );

        drawCtx.DrawText3D(
            Float3(
                cache.m_detachedDisplayX,
                cache.m_displayCenterY,
                cache.m_displayBaseZ +
                    cache.m_rootedForm.m_maximumReliefM +
                    detachedLiftM +
                    0.28f ),
            "DETACHED HEAD + PARENT SOCKET",
            Colors::White );
    }

    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // P3C.10C-2 — material-owned spall geometry witness
    //
    // The 10C-1 tetrahedra were intentionally only accounting witnesses.
    // 10C-2 renders actual GraniteGeometry for every explicit spall proposal.
    // Meshes were generated once when the debug cache was built.
    //-------------------------------------------------------------------------

    static Color GetGraniteSpallGeometryColor(
        GraniteSpallMatterClass spallClass )
    {
        switch ( spallClass )
        {
            case GraniteSpallMatterClass::CoarseSpall:
                return Color(
                    206,
                    206,
                    206 );

            case GraniteSpallMatterClass::Chip:
                return Color(
                    184,
                    184,
                    184 );

            case GraniteSpallMatterClass::Grit:
            default:
                return Color(
                    160,
                    160,
                    160 );
        }
    }

    //-------------------------------------------------------------------------

    static void DrawGraniteSpallResolutionWitness(
        DebugDrawContext&                  drawCtx,
        GraniteDetachmentDebugCache const& cache )
    {
        GraniteSpallResolutionResult const& spalls =
            cache.m_spallResolution;

        if ( !spalls.m_valid ||
             !cache.m_spallGeometryPass )
        {
            return;
        }

        float const baseZ =
            cache.m_displayBaseZ +
            0.16f;

        int32_t const count =
            cache.m_numSpallGeometry <
                    spalls.m_numPieces
                ? cache.m_numSpallGeometry
                : spalls.m_numPieces;

        for ( int32_t pieceIndex = 0;
              pieceIndex <
              count;
              ++pieceIndex )
        {
            GraniteSpallPieceDescriptor const& piece =
                spalls.m_pieces[pieceIndex];

            GraniteClosedGeometry const& geometry =
                cache.m_spallGeometry[pieceIndex];

            if ( !piece.m_valid )
            {
                continue;
            }

            DrawGraniteClosedGeometry(
                drawCtx,
                geometry,
                cache.m_spallDisplayX +
                    piece.m_localOffsetX,
                cache.m_displayCenterY +
                    piece.m_localOffsetY,
                baseZ +
                    piece.m_localOffsetZ,
                GetGraniteSpallGeometryColor(
                    piece.m_class ) );
        }

        drawCtx.DrawText3D(
            Float3(
                cache.m_spallDisplayX,
                cache.m_displayCenterY,
                baseZ +
                    0.48f ),
            "P3C.10C-2 MATERIAL-OWNED SPALL GEOMETRY",
            Colors::White );
    }

    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------

    static void DrawGraniteDetachmentDebugCache(
        DebugDrawContext&                  drawCtx,
        GraniteDetachmentDebugCache const& cache )
    {
        if ( !cache.m_valid )
        {
            return;
        }

        DrawGraniteDetachmentStageGeometry(
            drawCtx,
            cache.m_rootedForm,
            cache.m_sequence.m_initialRooted,
            GraniteSurfaceStoneState::AttachedRockHead,
            cache.m_rootedDisplayX,
            cache.m_displayCenterY,
            cache.m_displayBaseZ,
            "ROOTED 4/4" );

        DrawGraniteDetachmentStageGeometry(
            drawCtx,
            cache.m_rootedForm,
            cache.m_sequence.m_partialEvent.m_after,
            GraniteSurfaceStoneState::PartiallyDetachedBlock,
            cache.m_partialDisplayX,
            cache.m_displayCenterY,
            cache.m_displayBaseZ,
            "PARTIAL 2/4" );

        DrawGraniteDetachedPartitionStage(
            drawCtx,
            cache );

        DrawGraniteSpallResolutionWitness(
            drawCtx,
            cache );
    }

    //-------------------------------------------------------------------------

    static void DrawGraniteToolStrikeLab(
        DebugDrawContext&                  drawCtx,
        GraniteToolStrikeLabState const&   state )
    {
        if ( !state.m_initialized )
        {
            return;
        }

        float const labelZ =
            state.m_targetBaseZ +
            state.m_baseline.m_rootedForm.m_maximumReliefM +
            0.32f;

        drawCtx.DrawText3D
        (
            Float3
            (
                state.m_targetCenterX,
                state.m_targetCenterY,
                labelZ
            ),
            state.m_detached
                ? "P3C.11A-1 PLAYER TOOL-STRIKE LAB - DETACHED"
                : "P3C.11A-1 PLAYER TOOL-STRIKE LAB - CENTER + AT SEAM",
            state.m_detached ? Colors::Cyan : Colors::Yellow
        );

        // Structural bridge contact hints are diagnostics only. They expose
        // the actual bridge descriptors that the strike contract queries.
        for ( int32_t bridgeIndex = 0;
              bridgeIndex < state.m_currentBridges.m_numBridges;
              ++bridgeIndex )
        {
            GraniteStructuralBridgeDescriptor const& bridge =
                state.m_currentBridges.m_bridges[bridgeIndex];

            if ( !bridge.m_valid || bridge.m_severed )
            {
                continue;
            }

            float const displayX =
                state.m_targetCenterX +
                ( bridge.m_centerWorldX -
                  state.m_baseline.m_rootedForm.m_centerWorldX );
            float const displayY =
                state.m_targetCenterY +
                ( bridge.m_centerWorldY -
                  state.m_baseline.m_rootedForm.m_centerWorldY );
            float const markerZ = state.m_targetBaseZ + 0.08f;
            float constexpr markerHalfSize = 0.035f;

            drawCtx.DrawLine
            (
                Float3( displayX - markerHalfSize, displayY, markerZ ),
                Float3( displayX + markerHalfSize, displayY, markerZ ),
                Colors::Cyan,
                2.0f,
                DebugDrawLayer::World
            );
            drawCtx.DrawLine
            (
                Float3( displayX, displayY - markerHalfSize, markerZ ),
                Float3( displayX, displayY + markerHalfSize, markerZ ),
                Colors::Cyan,
                2.0f,
                DebugDrawLayer::World
            );
        }

        if ( state.m_haveContact )
        {
            Float3 const contact
            (
                state.m_contactDisplayX,
                state.m_contactDisplayY,
                state.m_contactDisplayZ
            );
            GraniteToolStrikeResult const& strike = state.m_lastStrike;

            drawCtx.DrawLine
            (
                contact,
                Float3
                (
                    contact.m_x + strike.m_normalX * 0.24f,
                    contact.m_y + strike.m_normalY * 0.24f,
                    contact.m_z + strike.m_normalZ * 0.24f
                ),
                Colors::Red,
                2.0f,
                DebugDrawLayer::World
            );
            drawCtx.DrawLine
            (
                contact,
                Float3
                (
                    contact.m_x + strike.m_tangentX * 0.24f,
                    contact.m_y + strike.m_tangentY * 0.24f,
                    contact.m_z + strike.m_tangentZ * 0.24f
                ),
                Colors::Yellow,
                2.0f,
                DebugDrawLayer::World
            );
            drawCtx.DrawLine
            (
                contact,
                Float3
                (
                    contact.m_x + strike.m_bitangentX * 0.24f,
                    contact.m_y + strike.m_bitangentY * 0.24f,
                    contact.m_z + strike.m_bitangentZ * 0.24f
                ),
                Colors::Cyan,
                2.0f,
                DebugDrawLayer::World
            );

            if ( strike.m_footprintMajorRadiusM > 0.0f &&
                 strike.m_footprintMinorRadiusM > 0.0f )
            {
                int32_t constexpr segments = 24;

                for ( int32_t segment = 0; segment < segments; ++segment )
                {
                    float const angleA =
                        6.28318530718f * float( segment ) / float( segments );
                    float const angleB =
                        6.28318530718f * float( segment + 1 ) / float( segments );
                    float const tangentA =
                        float( std::cos( double( angleA ) ) ) *
                        strike.m_footprintMajorRadiusM;
                    float const bitangentA =
                        float( std::sin( double( angleA ) ) ) *
                        strike.m_footprintMinorRadiusM;
                    float const tangentB =
                        float( std::cos( double( angleB ) ) ) *
                        strike.m_footprintMajorRadiusM;
                    float const bitangentB =
                        float( std::sin( double( angleB ) ) ) *
                        strike.m_footprintMinorRadiusM;

                    Float3 const pointA
                    (
                        contact.m_x +
                            strike.m_tangentX * tangentA +
                            strike.m_bitangentX * bitangentA,
                        contact.m_y +
                            strike.m_tangentY * tangentA +
                            strike.m_bitangentY * bitangentA,
                        contact.m_z +
                            strike.m_tangentZ * tangentA +
                            strike.m_bitangentZ * bitangentA
                    );
                    Float3 const pointB
                    (
                        contact.m_x +
                            strike.m_tangentX * tangentB +
                            strike.m_bitangentX * bitangentB,
                        contact.m_y +
                            strike.m_tangentY * tangentB +
                            strike.m_bitangentY * bitangentB,
                        contact.m_z +
                            strike.m_tangentZ * tangentB +
                            strike.m_bitangentZ * bitangentB
                    );

                    drawCtx.DrawLine
                    (
                        pointA,
                        pointB,
                        strike.m_pass ? Colors::Cyan : Colors::Red,
                        1.5f,
                        DebugDrawLayer::World
                    );
                }
            }
        }

        float const panelX = 930.0f;
        float panelY = 70.0f;
        float constexpr row = 17.0f;
        char line[512];

        drawCtx.DrawText2D
        (
            Float2( panelX, panelY ),
            "P3C.11A-1 GRANITE TOOL-STRIKE LAB",
            Colors::Yellow
        );
        panelY += row;

        std::snprintf
        (
            line,
            sizeof( line ),
            "YELLOW [ + ] AIM | RMB camera | release RMB, then F strike | cyan marks are bridges | seed %u",
            state.m_seed
        );
        drawCtx.DrawText2D( Float2( panelX, panelY ), line, Colors::White );
        panelY += row;

        std::snprintf
        (
            line,
            sizeof( line ),
            "state %s | accepted strikes %u | bridges intact %d/%d | parent owns %s",
            state.m_detached ? "DETACHED" :
                ( state.m_numAcceptedStrikes > 0 ? "PARTIAL" : "ROOTED" ),
            state.m_numAcceptedStrikes,
            state.m_currentBridges.m_numIntactBridges,
            state.m_currentBridges.m_numBridges,
            state.m_detached ? "NO" : "YES"
        );
        drawCtx.DrawText2D( Float2( panelX, panelY ), line, Colors::White );
        panelY += row;

        std::snprintf
        (
            line,
            sizeof( line ),
            "F input %s | PLAY-owned keyboard input | editor cursor capture DISABLED",
            state.m_strikeKeyHeld ? "DOWN" : "UP"
        );
        drawCtx.DrawText2D( Float2( panelX, panelY ), line, Colors::White );
        panelY += row;

        drawCtx.DrawText2D
        (
            Float2( panelX, panelY ),
            state.m_status,
            state.m_lastStrike.m_pass || !state.m_haveContact
                ? Colors::White
                : Colors::Red
        );
        panelY += row;

        if ( state.m_detached )
        {
            std::snprintf
            (
                line,
                sizeof( line ),
                "parcel %u = parent %u + head %u + explicit %llu + fine %u | dM %lld g | %s",
                state.m_terminal.m_preDetachParcelMassGrams,
                state.m_terminal.m_remainingParentMassGrams,
                state.m_terminal.m_detachedMassGrams,
                (unsigned long long) state.m_terminal.m_actualExplicitPieceMassGrams,
                state.m_terminal.m_fineReserveMassGrams,
                (long long) state.m_terminal.m_terminalResidualGrams,
                state.m_terminal.m_pass ? "PASS" : "FAIL"
            );
            drawCtx.DrawText2D
            (
                Float2( panelX, panelY ),
                line,
                state.m_terminal.m_pass ? Colors::Cyan : Colors::Red
            );
        }
    }

    //-------------------------------------------------------------------------
    // Aggregate AABB envelope
    //
    // Explicit P3C.7 bulk-envelope certification definition.
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // Aggregate bulk-envelope estimator
    //
    // IMPORTANT:
    //
    // This function keeps its old name temporarily so the rest of the
    // integration does not need another scattered edit.
    //
    // It NO LONGER measures an AABB.
    //
    // For the P3C.7 certification fixture:
    //
    //     aggregate bulk envelope
    //     =
    //     exact generated Granite solid volume
    //     +
    //     separation-created inter-clast void volume
    //
    // The inter-clast void term is measured from:
    //
    //     convex hull of clast CENTERS
    //     *
    //     solid-volume-weighted mean clast height
    //
    // Why centers rather than all vertices?
    //
    // Extreme fracture vertices describe the SHAPE of each solid clast.
    // They should not automatically count the rectangular/convex empty
    // exterior around that clast as aggregate void.
    //
    // At layoutScale == 0:
    //
    //     center hull area = 0
    //
    // therefore:
    //
    //     measured envelope = exact solid volume
    //
    // As clasts separate:
    //
    //     center hull grows
    //     inter-clast void grows
    //     bulk envelope grows
    //
    // This gives the packing solver the physically correct lower bound.
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // Aggregate hull helpers
    //-------------------------------------------------------------------------

    struct GraniteAggregateHullPoint
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
    };

    //-------------------------------------------------------------------------

    static float GraniteAggregateHullCross(
        GraniteAggregateHullPoint const& origin,
        GraniteAggregateHullPoint const& a,
        GraniteAggregateHullPoint const& b )
    {
        return (
                   a.m_x -
                   origin.m_x ) *
                   ( b.m_y -
                     origin.m_y ) -
               ( a.m_y -
                 origin.m_y ) *
                   ( b.m_x -
                     origin.m_x );
    }

    //-------------------------------------------------------------------------

    static void SortGraniteAggregateHullPoints(
        GraniteAggregateHullPoint* pPoints,
        int32_t                    numPoints )
    {
        for ( int32_t i = 1;
              i < numPoints;
              ++i )
        {
            GraniteAggregateHullPoint const key =
                pPoints[i];

            int32_t j =
                i - 1;

            while ( j >= 0 )
            {
                bool const shouldMove =
                    ( pPoints[j].m_x >
                      key.m_x ) ||
                    ( pPoints[j].m_x ==
                          key.m_x &&
                      pPoints[j].m_y >
                          key.m_y );

                if ( !shouldMove )
                {
                    break;
                }

                pPoints[j + 1] =
                    pPoints[j];

                --j;
            }

            pPoints[j + 1] =
                key;
        }
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteAggregateHullArea(
        GraniteAggregateHullPoint* pPoints,
        int32_t                    numPoints )
    {
        if ( numPoints < 3 )
        {
            return 0.0f;
        }

        SortGraniteAggregateHullPoints(
            pPoints,
            numPoints );

        //-------------------------------------------------------------------------
        // Remove duplicate XY points
        //-------------------------------------------------------------------------

        int32_t uniqueCount =
            0;

        float constexpr duplicateEpsilon =
            0.000001f;

        for ( int32_t i = 0;
              i < numPoints;
              ++i )
        {
            if ( uniqueCount == 0 )
            {
                pPoints[uniqueCount++] =
                    pPoints[i];

                continue;
            }

            GraniteAggregateHullPoint const& previous =
                pPoints[uniqueCount - 1];

            bool const duplicate =
                AbsFloat(
                    pPoints[i].m_x -
                    previous.m_x ) <=
                    duplicateEpsilon &&
                AbsFloat(
                    pPoints[i].m_y -
                    previous.m_y ) <=
                    duplicateEpsilon;

            if ( !duplicate )
            {
                pPoints[uniqueCount++] =
                    pPoints[i];
            }
        }

        if ( uniqueCount < 3 )
        {
            return 0.0f;
        }

        static constexpr int32_t s_maxAggregateClasts =
            8;

        GraniteAggregateHullPoint hull[s_maxAggregateClasts * 2];

        int32_t hullCount =
            0;

        float constexpr collinearEpsilon =
            0.0000001f;

        //-------------------------------------------------------------------------
        // Lower hull
        //-------------------------------------------------------------------------

        for ( int32_t i = 0;
              i < uniqueCount;
              ++i )
        {
            while ( hullCount >= 2 )
            {
                float const cross =
                    GraniteAggregateHullCross(
                        hull[hullCount - 2],
                        hull[hullCount - 1],
                        pPoints[i] );

                if ( cross >
                     collinearEpsilon )
                {
                    break;
                }

                --hullCount;
            }

            hull[hullCount++] =
                pPoints[i];
        }

        //-------------------------------------------------------------------------
        // Upper hull
        //-------------------------------------------------------------------------

        int32_t const lowerHullCount =
            hullCount;

        for ( int32_t i =
                  uniqueCount - 2;
              i >= 0;
              --i )
        {
            while ( hullCount >
                    lowerHullCount )
            {
                float const cross =
                    GraniteAggregateHullCross(
                        hull[hullCount - 2],
                        hull[hullCount - 1],
                        pPoints[i] );

                if ( cross >
                     collinearEpsilon )
                {
                    break;
                }

                --hullCount;
            }

            hull[hullCount++] =
                pPoints[i];
        }

        // Final point duplicates the first.
        if ( hullCount > 1 )
        {
            --hullCount;
        }

        if ( hullCount < 3 )
        {
            return 0.0f;
        }

        //-------------------------------------------------------------------------
        // Shoelace polygon area
        //-------------------------------------------------------------------------

        float twiceArea =
            0.0f;

        for ( int32_t i = 0;
              i < hullCount;
              ++i )
        {
            int32_t const next =
                ( i +
                  1 ) %
                hullCount;

            twiceArea +=
                hull[i].m_x *
                    hull[next].m_y -
                hull[next].m_x *
                    hull[i].m_y;
        }

        return AbsFloat(
                   twiceArea ) *
               0.5f;
    }

    //-------------------------------------------------------------------------
    static float MeasureGraniteAggregateAABB(
        GraniteClosedGeometry const* pClasts,
        float const*                 pOffsetX,
        float const*                 pOffsetY,
        int32_t                      numClasts,
        float                        layoutScale )
    {
        EE_ASSERT(
            numClasts >
            0 );

        static constexpr int32_t s_maxAggregateClasts =
            8;

        EE_ASSERT(
            numClasts <=
            s_maxAggregateClasts );

        GraniteAggregateHullPoint centerPoints[s_maxAggregateClasts];

        float totalSolidVolume =
            0.0f;

        float solidVolumeWeightedHeight =
            0.0f;

        //-------------------------------------------------------------------------
        // Exact generated solid volume + representative clast height
        //-------------------------------------------------------------------------

        for ( int32_t clastIndex = 0;
              clastIndex <
              numClasts;
              ++clastIndex )
        {
            GraniteClosedGeometry const& clast =
                pClasts[clastIndex];

            EE_ASSERT(
                clast.m_measuredMeshVolumeM3 >
                0.0f );

            totalSolidVolume +=
                clast.m_measuredMeshVolumeM3;

            solidVolumeWeightedHeight +=
                clast.m_measuredMeshVolumeM3 *
                clast.m_extentZM;

            centerPoints[clastIndex].m_x =
                pOffsetX[clastIndex] *
                layoutScale;

            centerPoints[clastIndex].m_y =
                pOffsetY[clastIndex] *
                layoutScale;
        }

        EE_ASSERT(
            totalSolidVolume >
            0.0f );

        float const representativeHeight =
            solidVolumeWeightedHeight /
            totalSolidVolume;

        //-------------------------------------------------------------------------
        // Inter-clast footprint
        //
        // At zero separation this is exactly zero.
        //-------------------------------------------------------------------------

        float const centerHullArea =
            MeasureGraniteAggregateHullArea(
                centerPoints,
                numClasts );

        float const interClastVoidVolume =
            centerHullArea *
            representativeHeight;

        //-------------------------------------------------------------------------
        // Bulk envelope
        //-------------------------------------------------------------------------

        return totalSolidVolume +
               interClastVoidVolume;
    }

    //-------------------------------------------------------------------------
    // Solve deterministic clast separation against required bulk envelope
    //
    // Required envelope still comes only from:
    //
    //     Granite solid volume
    //     /
    //     LooseAggregate packing fraction
    //
    // We do not change density or packing to accommodate geometry.
    // Geometry arrangement must solve to that physical target.
    //-------------------------------------------------------------------------

    static float SolveGraniteAggregateLayoutScale(
        GraniteClosedGeometry const* pClasts,
        float const*                 pOffsetX,
        float const*                 pOffsetY,
        int32_t                      numClasts,
        float                        targetEnvelopeM3,
        float                        characteristicLengthM )
    {
        float const volumeAtZero =
            MeasureGraniteAggregateAABB(
                pClasts,
                pOffsetX,
                pOffsetY,
                numClasts,
                0.0f );

        //-------------------------------------------------------------------------
        // At zero separation the bulk-envelope estimator must reduce to the
        // exact Granite solid volume.
        //
        // For LooseAggregate:
        //
        //     packing < 1
        //
        // therefore:
        //
        //     solid volume < required bulk envelope
        //-------------------------------------------------------------------------

        EE_ASSERT(
            volumeAtZero <=
            targetEnvelopeM3 );

        float low =
            0.0f;

        float high =
            MaxFloat(
                characteristicLengthM,
                0.05f );

        float highVolume =
            MeasureGraniteAggregateAABB(
                pClasts,
                pOffsetX,
                pOffsetY,
                numClasts,
                high );

        //-------------------------------------------------------------------------
        // Expand deterministic upper bound
        //-------------------------------------------------------------------------

        for ( int32_t expansion = 0;
              expansion <
                  12 &&
              highVolume <
                  targetEnvelopeM3;
              ++expansion )
        {
            high *=
                2.0f;

            highVolume =
                MeasureGraniteAggregateAABB(
                    pClasts,
                    pOffsetX,
                    pOffsetY,
                    numClasts,
                    high );
        }

        EE_ASSERT(
            highVolume >=
            targetEnvelopeM3 );

        //-------------------------------------------------------------------------
        // Deterministic binary solve
        //-------------------------------------------------------------------------

        for ( int32_t iteration = 0;
              iteration <
              32;
              ++iteration )
        {
            float const mid =
                ( low +
                  high ) *
                0.5f;

            float const measuredEnvelope =
                MeasureGraniteAggregateAABB(
                    pClasts,
                    pOffsetX,
                    pOffsetY,
                    numClasts,
                    mid );

            if ( measuredEnvelope <
                 targetEnvelopeM3 )
            {
                low =
                    mid;
            }
            else
            {
                high =
                    mid;
            }
        }

        return (
                   low +
                   high ) *
               0.5f;
    }

    //-------------------------------------------------------------------------
    // Granite loose aggregate
    //-------------------------------------------------------------------------

    static GraniteSpecimenMetrics DrawGraniteLooseAggregate(
        DebugDrawContext&                        drawCtx,
        ProvenanceWorldSystem::MatterBody const& body,
        uint32_t                                 worldSeed,
        Color const&                             color )
    {
        MatterVolumeMetrics const parentMetrics =
            CalculateMatterVolume(
                ProvenanceMaterialID::Granite,
                ProvenanceBodyState::LooseAggregate,
                body.m_massGrams );

        uint32_t clastMass[4];

        clastMass[0] =
            body.m_massGrams *
            30 /
            100;

        clastMass[1] =
            body.m_massGrams *
            27 /
            100;

        clastMass[2] =
            body.m_massGrams *
            23 /
            100;

        clastMass[3] =
            body.m_massGrams -
            clastMass[0] -
            clastMass[1] -
            clastMass[2];

        uint32_t massSum =
            0;

        GraniteClosedGeometry clasts[4];

        float measuredSolidVolume =
            0.0f;

        int32_t totalVertices =
            0;

        int32_t totalTriangles =
            0;

        float totalRestingFaceArea =
            0.0f;

        float weightedWeatheredCoverage =
            0.0f;

        float weightedFreshCoverage =
            0.0f;

        float weightedTransitionCoverage =
            0.0f;

        float weightedRounding =
            0.0f;

        float weightedSharpness =
            0.0f;

        float weightedPlanarity =
            0.0f;

        bool anyInheritedExterior =
            false;

        for ( int32_t i = 0;
              i <
              4;
              ++i )
        {
            massSum +=
                clastMass[i];

            GraniteClosedGeometryRequest request;

            request.m_worldSeed =
                worldSeed;

            request.m_geologicalAncestryID =
                body.m_geologicalAncestryID;

            // Gives B2's four clasts:
            //
            // 201 -> Slab
            // 202 -> Wedge
            // 203 -> TruncatedBlock
            // 204 -> IrregularBlock
            //
            // under the current deterministic topology selector.
            request.m_bodyID =
                body.m_bodyID *
                    100u +
                uint32_t(
                    i + 1 );

            request.m_massGrams =
                clastMass[i];

            request.m_bodyState =
                ProvenanceBodyState::Fragment;

            request.m_scale =
                GraniteGeometryScale::Fragment;

            // Certification aggregate deliberately mixes surface history:
            // two clasts preserve an old exposed face, two are fully fresh.
            request.m_weathering =
                ( i == 0 ||
                  i == 3 )
                    ? GraniteWeatheringState::WeatheredExposure
                    : GraniteWeatheringState::FreshFracture;

            request.m_canInheritExteriorSurface =
                i == 0 ||
                i == 3;

            request.m_parentExteriorExposure =
                request.m_canInheritExteriorSurface
                    ? 0.85f
                    : 0.0f;

            request.m_worldX =
                body.m_centerX;

            request.m_worldY =
                body.m_centerY;

            request.m_worldZ =
                body.m_baseZ;

            clasts[i] =
                GenerateGraniteClosedGeometry(
                    request );

            measuredSolidVolume +=
                clasts[i].m_measuredMeshVolumeM3;

            totalVertices +=
                clasts[i].m_numVertices;

            totalTriangles +=
                clasts[i].m_numTriangles;

            totalRestingFaceArea +=
                clasts[i].m_restingFaceAreaM2;

            float const clastVolumeWeight =
                clasts[i].m_measuredMeshVolumeM3;

            weightedWeatheredCoverage +=
                clasts[i].m_surfaceHistory.m_weatheredExteriorCoverage *
                clastVolumeWeight;

            weightedFreshCoverage +=
                clasts[i].m_surfaceHistory.m_freshFractureCoverage *
                clastVolumeWeight;

            weightedTransitionCoverage +=
                clasts[i].m_surfaceHistory.m_transitionalCoverage *
                clastVolumeWeight;

            weightedRounding +=
                clasts[i].m_surfaceHistory.m_meanRounding *
                clastVolumeWeight;

            weightedSharpness +=
                clasts[i].m_surfaceHistory.m_meanSharpness *
                clastVolumeWeight;

            weightedPlanarity +=
                clasts[i].m_surfaceHistory.m_meanPlanarity *
                clastVolumeWeight;

            anyInheritedExterior =
                anyInheritedExterior ||
                clasts[i].m_surfaceHistory.m_hasInheritedExterior;
        }

        EE_ASSERT(
            massSum ==
            body.m_massGrams );

        //-------------------------------------------------------------------------
        // Irregular aggregate footprint
        //-------------------------------------------------------------------------

        float constexpr offsetX[4] =
            {
                -0.65f,
                0.55f,
                -0.15f,
                0.62f };

        float constexpr offsetY[4] =
            {
                -0.22f,
                -0.18f,
                0.62f,
                0.54f };

        float const layoutScale =
            SolveGraniteAggregateLayoutScale(
                clasts,
                offsetX,
                offsetY,
                4,
                parentMetrics.m_envelopeVolumeM3,
                parentMetrics.m_characteristicLengthM );

        for ( int32_t i = 0;
              i <
              4;
              ++i )
        {
            DrawGraniteClosedGeometry(
                drawCtx,
                clasts[i],

                body.m_centerX +
                    offsetX[i] *
                        layoutScale,

                body.m_centerY +
                    offsetY[i] *
                        layoutScale,

                body.m_baseZ,

                color );
        }

        float const measuredEnvelope =
            MeasureGraniteAggregateAABB(
                clasts,
                offsetX,
                offsetY,
                4,
                layoutScale );

        float const solidResidual =
            measuredSolidVolume -
            parentMetrics.m_solidVolumeM3;

        float const envelopeResidual =
            measuredEnvelope -
            parentMetrics.m_envelopeVolumeM3;

        float const solidTolerance =
            0.000001f +
            parentMetrics.m_solidVolumeM3 *
                0.005f;

        float const envelopeTolerance =
            0.000005f +
            parentMetrics.m_envelopeVolumeM3 *
                0.015f;

        EE_ASSERT(
            AbsFloat(
                solidResidual ) <=
            solidTolerance );

        EE_ASSERT(
            AbsFloat(
                envelopeResidual ) <=
            envelopeTolerance );

        GraniteSpecimenMetrics metrics;

        metrics.m_valid =
            true;

        metrics.m_requiredSolidVolumeM3 =
            parentMetrics.m_solidVolumeM3;

        metrics.m_requiredEnvelopeVolumeM3 =
            parentMetrics.m_envelopeVolumeM3;

        metrics.m_measuredSolidVolumeM3 =
            measuredSolidVolume;

        metrics.m_measuredEnvelopeVolumeM3 =
            measuredEnvelope;

        metrics.m_solidResidualM3 =
            solidResidual;

        metrics.m_envelopeResidualM3 =
            envelopeResidual;

        metrics.m_topologyFamily =
            -1;

        metrics.m_numVertices =
            totalVertices;

        metrics.m_numTriangles =
            totalTriangles;

        metrics.m_restingFaceAreaM2 =
            totalRestingFaceArea;

        metrics.m_numAggregateClasts =
            4;

        if ( measuredSolidVolume >
             0.000001f )
        {
            metrics.m_weatheredExteriorCoverage =
                weightedWeatheredCoverage /
                measuredSolidVolume;

            metrics.m_freshFractureCoverage =
                weightedFreshCoverage /
                measuredSolidVolume;

            metrics.m_transitionalCoverage =
                weightedTransitionCoverage /
                measuredSolidVolume;

            metrics.m_meanRounding =
                weightedRounding /
                measuredSolidVolume;

            metrics.m_meanSharpness =
                weightedSharpness /
                measuredSolidVolume;

            metrics.m_meanPlanarity =
                weightedPlanarity /
                measuredSolidVolume;
        }

        metrics.m_hasInheritedExterior =
            anyInheritedExterior;

        metrics.m_contributorCount =
            body.m_numContributors;

        for ( int32_t i = 0;
              i <
              body.m_numContributors;
              ++i )
        {
            metrics.m_contributorMassGrams +=
                body.m_contributors[i].m_massGrams;
        }

        return metrics;
    }

    //-------------------------------------------------------------------------
    // Draw MatterBody
    //-------------------------------------------------------------------------

    static GraniteSpecimenMetrics DrawMatterBody(
        DebugDrawContext&                        drawCtx,
        ProvenanceWorldSystem::MatterBody const& body,
        uint32_t                                 worldSeed )
    {
        Color const color =
            GetMatterBodyBaseColor(
                body.m_material,
                body.m_bodyState );

        GraniteSpecimenMetrics graniteMetrics;

        if ( body.m_material ==
             ProvenanceMaterialID::Granite )
        {
            if ( body.m_bodyState ==
                 ProvenanceBodyState::Fragment )
            {
                // B1 certifies mixed surface memory: an old weathered exterior
                // is retained while side/break faces remain fresh fracture.
                graniteMetrics =
                    DrawClosedGraniteSpecimen(
                        drawCtx,
                        body,
                        worldSeed,
                        GraniteGeometryScale::Fragment,
                        GraniteWeatheringState::WeatheredExposure,
                        color );
            }
            else if ( body.m_bodyState ==
                      ProvenanceBodyState::LooseAggregate )
            {
                graniteMetrics =
                    DrawGraniteLooseAggregate(
                        drawCtx,
                        body,
                        worldSeed,
                        color );
            }
            else if ( body.m_bodyState ==
                      ProvenanceBodyState::Bonded )
            {
                graniteMetrics =
                    DrawClosedGraniteSpecimen(
                        drawCtx,
                        body,
                        worldSeed,
                        GraniteGeometryScale::Block,
                        GraniteWeatheringState::WeatheredExposure,
                        color );
            }
        }
        else
        {
            DrawSoilBody(
                drawCtx,
                body,
                worldSeed,
                color );
        }

        char bodyLabel[16];

        std::snprintf(
            bodyLabel,
            sizeof(
                bodyLabel ),
            "B%u",
            body.m_bodyID );

        drawCtx.DrawText3D(
            Float3(
                body.m_centerX,
                body.m_centerY,
                body.m_baseZ +
                    0.30f ),
            bodyLabel,
            Colors::White );

        return graniteMetrics;
    }

    //-------------------------------------------------------------------------
    // Exposure receipts
    //-------------------------------------------------------------------------

    struct ExposureReceipt
    {
        bool m_found =
            false;

        float m_x =
            0.0f;

        float m_y =
            0.0f;

        EvaluatedSurfacePoint m_surface;
    };

    //-------------------------------------------------------------------------
    // Body labels
    //-------------------------------------------------------------------------

    static char const* GetBodyShortName(
        uint32_t bodyID )
    {
        switch ( bodyID )
        {
            case 1:
                return "Granite Fragment";

            case 2:
                return "Granite Loose";

            case 3:
                return "Granite Bonded";

            case 4:
                return "Soil Clod";

            case 5:
                return "Soil Loose";

            case 6:
                return "Soil Compacted";

            default:
                return "Matter Body";
        }
    }

    //-------------------------------------------------------------------------
    // Debug panel
    //-------------------------------------------------------------------------

    static void DrawDebugPanel(
        DebugDrawContext&                        drawCtx,
        ProvenanceWorldSystem::MatterBody const* pBodies,
        int32_t                                  numBodies,
        GraniteSpecimenMetrics const*            pGraniteMetrics,
        uint32_t                                 seed,
        bool                                     seamPass,
        uint32_t                                 previewEvaluationCount,
        ExposureReceipt const&                   graniteReceipt,
        ExposureReceipt const&                   soilReceipt,
        SurfaceStoneClusterDebugCache const&     surfaceStoneCache,
        GraniteFractureDebugCache const&         fractureCache,
        GraniteDetachmentDebugCache const&       detachmentCache )
    {
        float const x =
            18.0f;

        float y =
            70.0f;

        float constexpr row =
            17.0f;

        char line[512];

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "PROVENANCE P3C.10C-2 - SPALL RESOLUTION + MATERIAL-OWNED GEOMETRY CERTIFICATE",
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "Seed %u | Granite formation %u | seam determinism %s",
            seed,
            s_graniteGeologicalAncestryID,
            seamPass
                ? "PASS"
                : "FAIL" );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            seamPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "Preview cache %u unique geological evals/rebuild | steady terrain geological evals 0/frame",
            previewEvaluationCount );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "Color %s | normal lighting %s | yellow contact %s | cyan seam %s",
            s_neutralMaterialDebug
                ? "NEUTRAL"
                : "MATERIAL",
            s_enableNormalLighting
                ? "ON"
                : "OFF",
            s_showGeologicalContact
                ? "ON"
                : "OFF",
            s_showPackageSeam
                ? "ON"
                : "OFF" );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row *
            1.4f;

        //-------------------------------------------------------------------------
        // Granite authority
        //-------------------------------------------------------------------------

        MaterialGeometryProfile const material =
            GetMaterialGeometryProfile(
                ProvenanceMaterialID::Granite );

        GraniteGeometryProfile const granite =
            GetGraniteGeometryProfile();

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "GRANITE MATERIAL AUTHORITY",
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "density %.1f kg/m3 | angularity %.2f | planarity %.2f | edge wear %.2f",
            material.m_intrinsicSolidDensityKgPerM3,
            granite.m_angularity,
            granite.m_facePlanarity,
            granite.m_edgeWear );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "joint hierarchy: nominal %.2f m | variation %.2f | hierarchy %.2f | coherent mass %.2f",
            granite.m_jointSpacingMeanM,
            granite.m_jointSpacingVariation,
            granite.m_jointHierarchyContrast,
            granite.m_coherentMassBias );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row *
            1.4f;

        std::snprintf(
            line,
            sizeof(
                line ),
            "burial filters: small %.2f m | medium %.2f m | broad %.2f m | inherited exterior bias %.2f",
            granite.m_smallFeatureBurialDepthM,
            granite.m_mediumFeatureBurialDepthM,
            granite.m_largeFeatureInfluenceDepthM,
            granite.m_surfaceHistory.m_inheritedExteriorBias );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row *
            1.4f;

        //-------------------------------------------------------------------------
        // Matter table
        //-------------------------------------------------------------------------

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "BODY                MASS     DENSITY   SOLID m3   PACK   ENVELOPE m3",
            Colors::White );

        y +=
            row;

        for ( int32_t i = 0;
              i <
              numBodies;
              ++i )
        {
            ProvenanceWorldSystem::MatterBody const& body =
                pBodies[i];

            MatterVolumeMetrics const metrics =
                CalculateMatterVolume(
                    body.m_material,
                    body.m_bodyState,
                    body.m_massGrams );

            std::snprintf(
                line,
                sizeof(
                    line ),
                "B%u %-17s %6.3f  %7.1f   %.5f   %.2f   %.5f",
                body.m_bodyID,
                GetBodyShortName(
                    body.m_bodyID ),
                metrics.m_massKg,
                metrics.m_solidDensityKgPerM3,
                metrics.m_solidVolumeM3,
                metrics.m_packingFraction,
                metrics.m_envelopeVolumeM3 );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                Colors::White );

            y +=
                row;
        }

        y +=
            row *
            0.7f;

        //-------------------------------------------------------------------------
        // P3C.9A Soil matter authority certificate
        //
        // Same Soil mass + same projected XY footprint, with only the physical
        // packing state changed. Thickness is therefore a derived consequence
        // of conserved matter, not an authored terrain number.
        //-------------------------------------------------------------------------

        SoilMantleDomain soilDomain40M2;
        soilDomain40M2.m_minX =
            0.0f;
        soilDomain40M2.m_maxX =
            8.0f;
        soilDomain40M2.m_minY =
            0.0f;
        soilDomain40M2.m_maxY =
            5.0f;

        SoilMantleDomain soilDomain80M2 =
            soilDomain40M2;

        soilDomain80M2.m_maxY =
            10.0f;

        SoilMantleBody looseMantle;
        looseMantle.m_bodyID =
            9001u;
        looseMantle.m_provenanceID =
            99001u;
        looseMantle.m_seed =
            seed;
        looseMantle.m_massGrams =
            1200000ull;
        looseMantle.m_physicalState =
            SoilPhysicalState::Loose;
        looseMantle.m_distributionState =
            SoilDistributionState::ContinuousMantle;

        SoilMantleBody settledMantle =
            looseMantle;

        settledMantle.m_bodyID =
            9002u;
        settledMantle.m_provenanceID =
            99002u;
        settledMantle.m_physicalState =
            SoilPhysicalState::Settled;

        SoilMantleBody compactedMantle =
            looseMantle;

        compactedMantle.m_bodyID =
            9003u;
        compactedMantle.m_provenanceID =
            99003u;
        compactedMantle.m_physicalState =
            SoilPhysicalState::Compacted;

        SoilMantleAuthorityResult const soilLoose40 =
            CalculateSoilMantleAuthority(
                looseMantle,
                soilDomain40M2 );

        SoilMantleAuthorityResult const soilSettled40 =
            CalculateSoilMantleAuthority(
                settledMantle,
                soilDomain40M2 );

        SoilMantleAuthorityResult const soilCompacted40 =
            CalculateSoilMantleAuthority(
                compactedMantle,
                soilDomain40M2 );

        SoilMantleAuthorityResult const soilSettled80 =
            CalculateSoilMantleAuthority(
                settledMantle,
                soilDomain80M2 );

        EE_ASSERT(
            soilLoose40.m_conservationPass );

        EE_ASSERT(
            soilSettled40.m_conservationPass );

        EE_ASSERT(
            soilCompacted40.m_conservationPass );

        EE_ASSERT(
            soilSettled80.m_conservationPass );

        EE_ASSERT(
            soilLoose40.m_meanThicknessZM >
            soilSettled40.m_meanThicknessZM );

        EE_ASSERT(
            soilSettled40.m_meanThicknessZM >
            soilCompacted40.m_meanThicknessZM );

        float const footprintDoublingThicknessResidual =
            soilSettled80.m_meanThicknessZM *
                2.0f -
            soilSettled40.m_meanThicknessZM;

        EE_ASSERT(
            footprintDoublingThicknessResidual >
                -0.000001f &&
            footprintDoublingThicknessResidual <
                0.000001f );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "P3C.9A SOIL MATTER AUTHORITY",
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "fixture: mass %.3f kg | density %.1f kg/m3 | footprint %.1f m2 | distribution ContinuousMantle",
            soilSettled40.m_massKg,
            soilSettled40.m_particleDensityKgPerM3,
            soilSettled40.m_footprintAreaM2 );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "Loose     solid %.6f m3 | pack %.2f | bulk %.6f m3 | mean-Z %.6f m | mass residual %+.7f kg",
            soilLoose40.m_solidVolumeM3,
            soilLoose40.m_packingFraction,
            soilLoose40.m_bulkVolumeM3,
            soilLoose40.m_meanThicknessZM,
            soilLoose40.m_massResidualKg );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            soilLoose40.m_conservationPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "Settled   solid %.6f m3 | pack %.2f | bulk %.6f m3 | mean-Z %.6f m | mass residual %+.7f kg",
            soilSettled40.m_solidVolumeM3,
            soilSettled40.m_packingFraction,
            soilSettled40.m_bulkVolumeM3,
            soilSettled40.m_meanThicknessZM,
            soilSettled40.m_massResidualKg );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            soilSettled40.m_conservationPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "Compacted solid %.6f m3 | pack %.2f | bulk %.6f m3 | mean-Z %.6f m | mass residual %+.7f kg",
            soilCompacted40.m_solidVolumeM3,
            soilCompacted40.m_packingFraction,
            soilCompacted40.m_bulkVolumeM3,
            soilCompacted40.m_meanThicknessZM,
            soilCompacted40.m_massResidualKg );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            soilCompacted40.m_conservationPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "40->80 m2 settled proof: %.6f -> %.6f m | 2*T80-T40 %+.8f m | volume residual %+.8f m3",
            soilSettled40.m_meanThicknessZM,
            soilSettled80.m_meanThicknessZM,
            footprintDoublingThicknessResidual,
            soilSettled40.m_bulkVolumeResidualM3 );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row *
            1.2f;

        //-------------------------------------------------------------------------
        // P3C.9B conserved continuous Soil redistribution certificate
        //-------------------------------------------------------------------------

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "P3C.9B CONSERVED SOIL REDISTRIBUTION",
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "fixture: Settled %.1f kg | grid %dx%d @ %.1f m | area %.1f m2 | uniform mean-Z %.4f m",
            s_p3c9bRedistribution.m_authoritativeMassKg,
            s_p3c9bCellsX,
            s_p3c9bCellsY,
            s_p3c9bCellSizeM,
            s_p3c9bRedistribution.m_gridAreaM2,
            s_p3c9bRedistribution.m_uniformMeanThicknessZM );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9bRedistribution.m_valid
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "bulk %.6f -> %.6f m3 | dV %+.8f | mass %.3f -> %.3f kg | dM %+.6f",
            s_p3c9bRedistribution.m_authoritativeBulkVolumeM3,
            s_p3c9bRedistribution.m_integratedBulkVolumeM3,
            s_p3c9bRedistribution.m_bulkVolumeResidualM3,
            s_p3c9bRedistribution.m_authoritativeMassKg,
            s_p3c9bRedistribution.m_reconstructedMassKg,
            s_p3c9bRedistribution.m_massResidualKg );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9bRedistribution.m_conservationPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "thickness min %.4f max %.4f mean %.4f m | continuous cover %s",
            s_p3c9bRedistribution.m_minThicknessZM,
            s_p3c9bRedistribution.m_maxThicknessZM,
            s_p3c9bRedistribution.m_redistributedMeanThicknessZM,
            s_p3c9bRedistribution.m_continuousCoverPass
                ? "PASS"
                : "FAIL" );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9bRedistribution.m_continuousCoverPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "geometry response: convex/high n=%d mean %.4f m | retention/low n=%d mean %.4f m | %s",
            s_p3c9bRedistribution.m_numConvexHighCells,
            s_p3c9bRedistribution.m_meanConvexHighThicknessZM,
            s_p3c9bRedistribution.m_numRetentionLowCells,
            s_p3c9bRedistribution.m_meanRetentionLowThicknessZM,
            s_p3c9bGeometryResponsePass
                ? "PASS"
                : "CHECK" );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9bGeometryResponsePass
                ? Colors::White
                : Colors::Yellow );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "source Granite -> generic SoilRetentionSample | captured from preview cache | extra geological evals 0 | seed %u",
            s_p3c9bSeed );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row *
            1.2f;

        //-------------------------------------------------------------------------
        // P3C.9C conserved breach / residual patch certificate
        //-------------------------------------------------------------------------

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "P3C.9C CONSERVED BREACH / PATCH SURVIVAL",
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "fixture: Settled %.1f kg | pre-breach mean-Z %.4f m | occupied %d/%d | breached %d | coverage %.3f | breach %.3f",
            s_p3c9cBreach.m_authoritativeMassKg,
            s_p3c9cAuthority.m_meanThicknessZM,
            s_p3c9cBreach.m_numOccupiedCells,
            s_p3c9cBreach.m_numCells,
            s_p3c9cBreach.m_numBreachedCells,
            s_p3c9cBreach.m_coverageFraction,
            s_p3c9cBreach.m_breachFraction );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9cBreachResponsePass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "bulk %.6f -> %.6f m3 | dV %+.8f | mass %.3f -> %.3f kg | dM %+.6f",
            s_p3c9cBreach.m_authoritativeBulkVolumeM3,
            s_p3c9cBreach.m_finalBulkVolumeM3,
            s_p3c9cBreach.m_bulkVolumeResidualM3,
            s_p3c9cBreach.m_authoritativeMassKg,
            s_p3c9cBreach.m_reconstructedMassKg,
            s_p3c9cBreach.m_massResidualKg );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9cBreach.m_conservationPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "patches %d | retained-pocket cells %d | evacuated %.6f m3 | iterations %d | topology %s",
            s_p3c9cBreach.m_numPatches,
            s_p3c9cBreach.m_numPocketCells,
            s_p3c9cBreach.m_evacuatedBulkVolumeM3,
            s_p3c9cBreach.m_iterationsUsed,
            s_p3c9cBreach.m_patchTopologyPass
                ? "PASS"
                : "FAIL" );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9cBreach.m_patchTopologyPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "survivors min %.4f max %.4f mean %.4f m | pocket mean %.4f m | failed pre-evac mean %.4f m",
            s_p3c9cBreach.m_minOccupiedThicknessZM,
            s_p3c9cBreach.m_maxOccupiedThicknessZM,
            s_p3c9cBreach.m_meanOccupiedThicknessZM,
            s_p3c9cBreach.m_meanPocketThicknessZM,
            s_p3c9cBreach.m_meanBreachedPreEvacuationThicknessZM );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "coverage is derived from occupied cells | no Soil export | generic substrate field reused | extra geological evals 0 | %s",
            s_p3c9cBreachResponsePass
                ? "PASS"
                : "CHECK" );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9cBreachResponsePass
                ? Colors::White
                : Colors::Yellow );

        y +=
            row *
            1.2f;

        //-------------------------------------------------------------------------
        // P3C.9C-4 visible conserved Soil geometry certificate
        //-------------------------------------------------------------------------

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "P3C.9C-4E SUPPORT-CONFORMING SOIL CARRIER",
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "fine grid %dx%d @ %.2f m | occupied fine %d/%d | fine coverage %.3f | contact segments %d",
            s_p3c9c4FineCellsX,
            s_p3c9c4FineCellsY,
            s_p3c9c4FineCellSizeM,
            s_p3c9c4VisibleGeometry.m_numOccupiedFineCells,
            s_p3c9c4VisibleGeometry.m_numFineCells,
            s_p3c9c4VisibleGeometry.m_fineCoverageFraction,
            s_p3c9c4NumContactSegments );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9c4VisibleGeometryPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "parent bulk %.6f -> visible %.6f m3 | dV %+.8f | parent conservation %s",
            s_p3c9c4VisibleGeometry.m_authoritativeBulkVolumeM3,
            s_p3c9c4VisibleGeometry.m_totalAllocatedBulkVolumeM3,
            s_p3c9c4VisibleGeometry.m_bulkVolumeResidualM3,
            s_p3c9c4VisibleGeometry.m_conservationPass
                ? "PASS"
                : "FAIL" );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9c4VisibleGeometry.m_conservationPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "residual subcells %d mean %.4f m | pocket subcells %d mean %.4f m | parent topology %s",
            s_p3c9c4VisibleGeometry.m_numResidualPatchSubcells,
            s_p3c9c4VisibleGeometry.m_meanResidualPatchThicknessZM,
            s_p3c9c4VisibleGeometry.m_numPocketSubcells,
            s_p3c9c4VisibleGeometry.m_meanPocketThicknessZM,
            s_p3c9c4VisibleGeometry.m_parentTopologyPass
                ? "PASS"
                : "FAIL" );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9c4VisibleGeometry.m_parentTopologyPass
                ? Colors::White
                : Colors::Red );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "2 m cells own volume -> 0.25 m explicit Soil geometry | substrate windows real | extra geological evals 0 | %s",
            s_p3c9c4VisibleGeometryPass
                ? "PASS"
                : "CHECK" );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            s_p3c9c4VisibleGeometryPass
                ? Colors::White
                : Colors::Yellow );

        y +=
            row *
            1.2f;

        //-------------------------------------------------------------------------
        // P3C.9C-4F Soil state geometry grammar certificate
        //-------------------------------------------------------------------------

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "P3C.9C-4F SOIL STATE GEOMETRY GRAMMAR",
            Colors::White );

        y +=
            row;

        for ( int32_t bodyIndex = 3;
              bodyIndex < numBodies &&
              bodyIndex < 6;
              ++bodyIndex )
        {
            ProvenanceWorldSystem::MatterBody const& body =
                pBodies[bodyIndex];

            SoilBodyGeometry const geometry =
                GenerateWorkbenchSoilGeometry(
                    body,
                    seed );

            std::snprintf(
                line,
                sizeof(
                    line ),
                "B%u %-13s reqBulk %.5f measured %.5f dV %+.8f | %s",
                body.m_bodyID,
                GetSoilBodyGeometryKindName(
                    geometry.m_kind ),
                geometry.m_requiredBulkVolumeM3,
                geometry.m_measuredMeshVolumeM3,
                geometry.m_bulkVolumeResidualM3,
                geometry.m_conservationPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                geometry.m_conservationPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "   extents %.3f x %.3f x %.3f m | contact %.4f m2 | crown mean %.3f max %.3f | slope %.2f",
                geometry.m_extentXM,
                geometry.m_extentYM,
                geometry.m_extentZM,
                geometry.m_contactAreaM2,
                geometry.m_meanCrownHeightM,
                geometry.m_maxCrownHeightM,
                geometry.m_effectiveSurfaceSlope );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                Colors::White );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "   mesh V %d T %d | crown %d shoulder %d side %d contact %d | lobes %d",
                geometry.m_numVertices,
                geometry.m_numTriangles,
                geometry.m_crownTriangleCount,
                geometry.m_shoulderTriangleCount,
                geometry.m_sideTriangleCount,
                geometry.m_contactTriangleCount,
                geometry.m_aggregateLobeCount );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                Colors::White );

            y +=
                row;
        }

        y +=
            row *
            0.4f;

        //-------------------------------------------------------------------------
        // Granite topology certificate
        //-------------------------------------------------------------------------

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "GRANITE GENERATED GEOMETRY",
            Colors::White );

        y +=
            row;

        for ( int32_t i = 0;
              i <
              3;
              ++i )
        {
            GraniteSpecimenMetrics const& metrics =
                pGraniteMetrics[i];

            if ( !metrics.m_valid )
            {
                continue;
            }

            std::snprintf(
                line,
                sizeof(
                    line ),
                "B%d reqSolid %.5f measured %.5f residual %+.7f | reqEnv %.5f measuredEnv %.5f",
                i + 1,
                metrics.m_requiredSolidVolumeM3,
                metrics.m_measuredSolidVolumeM3,
                metrics.m_solidResidualM3,
                metrics.m_requiredEnvelopeVolumeM3,
                metrics.m_measuredEnvelopeVolumeM3 );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                Colors::White );

            y +=
                row;

            if ( i !=
                 1 )
            {
                std::snprintf(
                    line,
                    sizeof(
                        line ),
                    "   family %s | V %d T %d faces %d | rest tri %d area %.4f m2",
                    GetGraniteTopologyFamilyName(
                        metrics.m_topologyFamily ),
                    metrics.m_numVertices,
                    metrics.m_numTriangles,
                    metrics.m_majorFaceCount,
                    metrics.m_restingTriangleIndex,
                    metrics.m_restingFaceAreaM2 );

                drawCtx.DrawText2D(
                    Float2(
                        x,
                        y ),
                    line,
                    Colors::White );

                y +=
                    row;

                std::snprintf(
                    line,
                    sizeof(
                        line ),
                    "   extents %.3f x %.3f x %.3f m | angular %.2f wear %.2f slab %.2f elong %.2f",
                    metrics.m_extentXM,
                    metrics.m_extentYM,
                    metrics.m_extentZM,
                    metrics.m_angularity,
                    metrics.m_edgeWear,
                    metrics.m_slabCharacter,
                    metrics.m_elongationCharacter );

                drawCtx.DrawText2D(
                    Float2(
                        x,
                        y ),
                    line,
                    Colors::White );

                y +=
                    row;

                std::snprintf(
                    line,
                    sizeof(
                        line ),
                    "   surface history: old %.2f fresh %.2f transition %.2f | round %.2f sharp %.2f planar %.2f",
                    metrics.m_weatheredExteriorCoverage,
                    metrics.m_freshFractureCoverage,
                    metrics.m_transitionalCoverage,
                    metrics.m_meanRounding,
                    metrics.m_meanSharpness,
                    metrics.m_meanPlanarity );

                drawCtx.DrawText2D(
                    Float2(
                        x,
                        y ),
                    line,
                    Colors::White );

                y +=
                    row;
            }
            else
            {
                std::snprintf(
                    line,
                    sizeof(
                        line ),
                    "   %d variable-topology clasts | total V %d T %d | stable resting faces | env residual %+.7f",
                    metrics.m_numAggregateClasts,
                    metrics.m_numVertices,
                    metrics.m_numTriangles,
                    metrics.m_envelopeResidualM3 );

                drawCtx.DrawText2D(
                    Float2(
                        x,
                        y ),
                    line,
                    Colors::White );

                y +=
                    row;

                std::snprintf(
                    line,
                    sizeof(
                        line ),
                    "   aggregate history: old %.2f fresh %.2f transition %.2f | mixed exterior %s",
                    metrics.m_weatheredExteriorCoverage,
                    metrics.m_freshFractureCoverage,
                    metrics.m_transitionalCoverage,
                    metrics.m_hasInheritedExterior
                        ? "YES"
                        : "NO" );

                drawCtx.DrawText2D(
                    Float2(
                        x,
                        y ),
                    line,
                    Colors::White );

                y +=
                    row;
            }
        }

        y +=
            row *
            0.4f;

        GraniteSpecimenMetrics const& bonded =
            pGraniteMetrics[2];

        std::snprintf(
            line,
            sizeof(
                line ),
            "B3 bonded provenance: %d contributors | contributor mass %.3f kg | one continuous exterior",
            bonded.m_contributorCount,
            float(
                bonded.m_contributorMassGrams ) /
                1000.0f );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row *
            1.4f;

        //-------------------------------------------------------------------------
        // P3C.10A / P3C.10A-2 Granite fracture + geometric partition
        //-------------------------------------------------------------------------

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "P3C.10A-2 GRANITE GEOMETRIC PARTITION",
            Colors::White );

        y +=
            row;

        if ( fractureCache.m_valid )
        {
            GraniteFractureTransactionResult const& fracture =
                fractureCache.m_transaction;

            GraniteFractureChild const& primary =
                fracture.m_children[0];

            GraniteFractureChild const& secondary =
                fracture.m_children[1];

            GraniteFractureGeometricPartitionReceipt const& partition =
                fracture.m_geometricPartition;

            //-------------------------------------------------------------------------
            // Accounting union
            //-------------------------------------------------------------------------

            std::snprintf(
                line,
                sizeof(
                    line ),
                "B%u %u g -> F%u %u g + F%u %u g | dM %lld g | %s",
                fracture.m_parentBodyID,
                fracture.m_parentMassGrams,
                primary.m_bodyID,
                primary.m_massGrams,
                secondary.m_bodyID,
                secondary.m_massGrams,
                static_cast<long long>(
                    fracture.m_massResidualGrams ),
                fracture.m_massConservationPass
                    ? "MASS PASS"
                    : "MASS FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                fracture.m_massConservationPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            //-------------------------------------------------------------------------
            // Authoritative solid-volume union
            //-------------------------------------------------------------------------

            std::snprintf(
                line,
                sizeof(
                    line ),
                "solid authority %.7f -> children %.7f m3 | dV %+.9f | %s",
                fracture.m_parentSolidVolumeM3,
                fracture.m_totalChildSolidVolumeM3,
                fracture.m_solidVolumeResidualM3,
                fracture.m_solidVolumeConservationPass
                    ? "SOLID PASS"
                    : "SOLID FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                fracture.m_solidVolumeConservationPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            //-------------------------------------------------------------------------
            // Exact visible geometric union
            //-------------------------------------------------------------------------

            std::snprintf(
                line,
                sizeof(
                    line ),
                "visible mesh parent %.7f -> F301 %.7f + F302 %.7f = %.7f | dV %+.9f | %s",
                partition.m_parentVisibleMeshVolumeM3,
                partition.m_primaryVisibleMeshVolumeM3,
                partition.m_secondaryVisibleMeshVolumeM3,
                partition.m_childVisibleMeshVolumeSumM3,
                partition.m_visibleMeshUnionResidualM3,
                partition.m_visibleMeshUnionPass
                    ? "UNION PASS"
                    : "UNION FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                partition.m_visibleMeshUnionPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            //-------------------------------------------------------------------------
            // Exterior partition
            //-------------------------------------------------------------------------

            std::snprintf(
                line,
                sizeof(
                    line ),
                "parent exterior %.6f -> inherited child exterior %.6f m2 | dA %+.8f | %s",
                partition.m_parentExteriorAreaM2,
                partition.m_childInheritedExteriorAreaSumM2,
                partition.m_exteriorAreaResidualM2,
                partition.m_exteriorPartitionPass
                    ? "EXTERIOR PASS"
                    : "EXTERIOR FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                partition.m_exteriorPartitionPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            //-------------------------------------------------------------------------
            // Shared cut coincidence
            //-------------------------------------------------------------------------

            std::snprintf(
                line,
                sizeof(
                    line ),
                "shared cut F301 %.6f F302 %.6f m2 | dA %+.8f | plane dev %.9f m | %s",
                partition.m_primaryCutAreaM2,
                partition.m_secondaryCutAreaM2,
                partition.m_cutAreaResidualM2,
                partition.m_maxCutPlaneDeviationM,
                partition.m_sharedCutCoincidencePass
                    ? "CUT PASS"
                    : "CUT FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                partition.m_sharedCutCoincidencePass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            //-------------------------------------------------------------------------
            // Complementary half-space ownership
            //-------------------------------------------------------------------------

            std::snprintf(
                line,
                sizeof(
                    line ),
                "half-space violation F301 %.9f m | F302 %.9f m | %s",
                partition.m_primaryHalfSpaceViolationM,
                partition.m_secondaryHalfSpaceViolationM,
                partition.m_halfSpaceOwnershipPass
                    ? "OWNERSHIP PASS"
                    : "OWNERSHIP FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                partition.m_halfSpaceOwnershipPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            //-------------------------------------------------------------------------
            // Cut topology
            //-------------------------------------------------------------------------

            std::snprintf(
                line,
                sizeof(
                    line ),
                "cut topology loops %d vertices %d | inherited tris %d/%d | cap tris %d/%d | closed %s",
                partition.m_cutLoopCount,
                partition.m_cutVertexCount,
                partition.m_primaryInheritedTriangleCount,
                partition.m_secondaryInheritedTriangleCount,
                partition.m_primaryCapTriangleCount,
                partition.m_secondaryCapTriangleCount,
                partition.m_closedChildTopologyPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                partition.m_closedChildTopologyPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            //-------------------------------------------------------------------------
            // Surface-history continuity
            //-------------------------------------------------------------------------

            std::snprintf(
                line,
                sizeof(
                    line ),
                "fresh cut area F301 %.6f F302 %.6f m2 | inherited exterior YES/NO %s | both fresh %s",
                primary.m_surfaceReceipt.m_newFractureAreaM2,
                secondary.m_surfaceReceipt.m_newFractureAreaM2,
                fracture.m_inheritedExteriorPreserved
                    ? "YES"
                    : "NO",
                fracture.m_bothPrincipalChildrenHaveFreshBreak
                    ? "YES"
                    : "NO" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                fracture.m_surfaceHistoryPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            //-------------------------------------------------------------------------
            // Final hard gates
            //-------------------------------------------------------------------------

            std::snprintf(
                line,
                sizeof(
                    line ),
                "geometry %s | surface history %s | geometric partition %s | TRANSACTION %s",
                fracture.m_childGeometryPass
                    ? "PASS"
                    : "FAIL",
                fracture.m_surfaceHistoryPass
                    ? "PASS"
                    : "FAIL",
                fracture.m_geometricPartitionPass
                    ? "PASS"
                    : "FAIL",
                fracture.m_valid
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                fracture.m_valid
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;
        }
        else
        {
            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                "fracture fixture unavailable",
                Colors::Red );

            y +=
                row;
        }

        y +=
            row *
            0.6f;

        //-------------------------------------------------------------------------
        // P3C.10B rooted -> partial -> detached structural ownership
        //-------------------------------------------------------------------------

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "P3C.10B-2 ROOTED HEAD SEPARATION / MATTER PARTITION",
            Colors::White );

        y +=
            row;

        if ( detachmentCache.m_valid )
        {
            GraniteFormationFormDescriptor const& form =
                detachmentCache.m_rootedForm;

            GraniteDetachmentSequenceReceipt const& sequence =
                detachmentCache.m_sequence;

            GraniteStructuralBridgeSet const& rooted =
                sequence.m_initialRooted;

            GraniteDetachmentResult const& partial =
                sequence.m_partialEvent;

            GraniteDetachmentResult const& finalEvent =
                sequence.m_finalEvent;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "actual RootedRockHead event %u | XY(%.2f,%.2f) | radii %.2f/%.2f m | root blend %.2f m",
                form.m_eventID,
                form.m_centerWorldX,
                form.m_centerWorldY,
                form.m_majorRadiusM,
                form.m_minorRadiusM,
                form.m_rootBlendRadiusM );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                Colors::White );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "ROOTED    intact %d/%d | attach %.5f m2 | connection %.3f | parent owns YES | %s",
                rooted.m_numIntactBridges,
                rooted.m_numBridges,
                rooted.m_remainingAttachmentAreaM2,
                rooted.m_remainingConnectionFraction,
                sequence.m_initialRootedPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                sequence.m_initialRootedPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "PARTIAL   intact %d/%d severed %d | attach %.5f m2 | connection %.3f | parent owns %s | %s",
                partial.m_after.m_numIntactBridges,
                partial.m_after.m_numBridges,
                partial.m_after.m_numSeveredBridges,
                partial.m_after.m_remainingAttachmentAreaM2,
                partial.m_after.m_remainingConnectionFraction,
                partial.m_parentStructuralOwnershipRetained
                    ? "YES"
                    : "NO",
                sequence.m_partialStillOwnedByParentPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                sequence.m_partialStillOwnedByParentPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "DETACHED  intact %d/%d severed %d | attach %.5f m2 | connection %.3f | parent owns %s | candidate %s | %s",
                finalEvent.m_after.m_numIntactBridges,
                finalEvent.m_after.m_numBridges,
                finalEvent.m_after.m_numSeveredBridges,
                finalEvent.m_after.m_remainingAttachmentAreaM2,
                finalEvent.m_after.m_remainingConnectionFraction,
                finalEvent.m_parentStructuralOwnershipRetained
                    ? "YES"
                    : "NO",
                finalEvent.m_detachedCandidateReady
                    ? "READY"
                    : "NOT READY",
                sequence.m_finalParentOwnershipReleasedPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                sequence.m_finalParentOwnershipReleasedPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "partial event dArea %.5f m2 | monotonic %s bridge accounting %s state %s ownership %s",
                partial.m_attachmentAreaRemovedM2,
                partial.m_monotonicConnectionPass
                    ? "PASS"
                    : "FAIL",
                partial.m_bridgeAccountingPass
                    ? "PASS"
                    : "FAIL",
                partial.m_stateTransitionPass
                    ? "PASS"
                    : "FAIL",
                partial.m_parentOwnershipPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                partial.m_pass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "final event dArea %.5f m2 | monotonic %s bridge accounting %s state %s ownership %s",
                finalEvent.m_attachmentAreaRemovedM2,
                finalEvent.m_monotonicConnectionPass
                    ? "PASS"
                    : "FAIL",
                finalEvent.m_bridgeAccountingPass
                    ? "PASS"
                    : "FAIL",
                finalEvent.m_stateTransitionPass
                    ? "PASS"
                    : "FAIL",
                finalEvent.m_parentOwnershipPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                finalEvent.m_pass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            GraniteSurfaceStoneDescriptor const& candidate =
                finalEvent.m_detachedCandidate;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "candidate geometry/state only: event %u | envelope %.2f x %.2f relief %.2f m | mass NOT ASSIGNED",
                candidate.m_eventID,
                candidate.m_majorRadiusM *
                    2.0f,
                candidate.m_minorRadiusM *
                    2.0f,
                candidate.m_maximumReliefM );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                finalEvent.m_detachedCandidateReady
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "same bridge set: 1.000 -> %.3f -> %.3f | monotonic %s | SEQUENCE %s",
                partial.m_after.m_remainingConnectionFraction,
                finalEvent.m_after.m_remainingConnectionFraction,
                sequence.m_monotonicConnectionPass
                    ? "PASS"
                    : "FAIL",
                sequence.m_pass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                sequence.m_pass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            GraniteRootedSeparationResult const& separation =
                detachmentCache.m_separation;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "separation geometry: parent socket STAYS | detached head is independent parcel | structural release %s",
                separation.m_structuralReleasePass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                separation.m_structuralReleasePass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "local parcel volume %.6f = detached %.6f + spall %.6f + socket %.6f m3 | dV %+.8f | %s",
                separation.m_matter.m_preDetachParcelVolumeM3,
                separation.m_matter.m_detachedVolumeM3,
                separation.m_matter.m_spallReserveVolumeM3,
                separation.m_matter.m_remainingParentVolumeM3,
                separation.m_matter.m_volumeResidualM3,
                separation.m_matter.m_volumeConservationPass
                    ? "VOLUME PASS"
                    : "VOLUME FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                separation.m_matter.m_volumeConservationPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "local parcel mass %u g = detached %u + spall %u + socket %u g | dM %lld g | %s",
                separation.m_matter.m_preDetachParcelMassGrams,
                separation.m_matter.m_detachedMassGrams,
                separation.m_matter.m_spallReserveMassGrams,
                separation.m_matter.m_remainingParentMassGrams,
                static_cast<long long>(
                    separation.m_matter.m_massResidualGrams ),
                separation.m_matter.m_massConservationPass
                    ? "MASS PASS"
                    : "MASS FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                separation.m_matter.m_massConservationPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "shared socket interface %.5f m2 | deviation %.7f m | detached closed %s | socket closed %s",
                separation.m_sharedInterfaceAreaM2,
                separation.m_maxSharedInterfaceDeviationM,
                separation.m_detachedMeshClosedPass
                    ? "PASS"
                    : "FAIL",
                separation.m_parentSocketMeshClosedPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                separation.m_sharedInterfacePass &&
                        separation.m_detachedMeshClosedPass &&
                        separation.m_parentSocketMeshClosedPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "detached body proposal ID %u | provenance %u | mass %u g | parent formation stays substrate",
                separation.m_detachedBodyID,
                separation.m_detachedProvenanceID,
                separation.m_detachedMassGrams );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                separation.m_matterPartitionPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "geometric partition %s | interface %s | matter partition %s | ROOTED SEPARATION %s",
                separation.m_geometricPartitionPass
                    ? "PASS"
                    : "FAIL",
                separation.m_sharedInterfacePass
                    ? "PASS"
                    : "FAIL",
                separation.m_matterPartitionPass
                    ? "PASS"
                    : "FAIL",
                separation.m_pass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                separation.m_pass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            GraniteSpallResolutionResult const& spalls =
                detachmentCache.m_spallResolution;

            GraniteSpallResolutionReceipt const& spallReceipt =
                spalls.m_receipt;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "P3C.10C-1 spall reserve %u g -> explicit %u g (%d bodies) + fine %u g | dM %lld g",
                spallReceipt.m_inputReserveMassGrams,
                spallReceipt.m_explicitPieceMassGrams,
                spallReceipt.m_numExplicitPieces,
                spallReceipt.m_fineReserveMassGrams,
                static_cast<long long>(
                    spallReceipt.m_massResidualGrams ) );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                spallReceipt.m_massConservationPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "spall classes: coarse %d | chips %d | grit %d | IDs %s | lineage %s | min-mass %s",
                spallReceipt.m_numCoarseSpalls,
                spallReceipt.m_numChips,
                spallReceipt.m_numGritPieces,
                spallReceipt.m_uniqueBodyIDsPass
                    ? "PASS"
                    : "FAIL",
                spallReceipt.m_lineagePass
                    ? "PASS"
                    : "FAIL",
                spallReceipt.m_pieceMinimumMassPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                spallReceipt.m_uniqueBodyIDsPass &&
                        spallReceipt.m_lineagePass &&
                        spallReceipt.m_pieceMinimumMassPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "secondary fragmentation accounting %s | particles NOT matter authority | P3C.10C-1 %s",
                spallReceipt.m_massConservationPass
                    ? "PASS"
                    : "FAIL",
                spallReceipt.m_pass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                spallReceipt.m_pass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            float const spallGeometryResidualM3 =
                detachmentCache.m_spallMeasuredSolidVolumeM3 -
                detachmentCache.m_spallRequiredSolidVolumeM3;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "P3C.10C-2 geometry: %d/%d Granite bodies | mass %u/%u g %s",
                detachmentCache.m_numSpallGeometry,
                spallReceipt.m_numExplicitPieces,
                detachmentCache.m_spallGeometryMassGrams,
                spallReceipt.m_explicitPieceMassGrams,
                detachmentCache.m_spallGeometryMassPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                detachmentCache.m_spallGeometryMassPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "spall solid volume req %.6f measured %.6f dV %+.8f | max body residual %.8f | %s",
                detachmentCache.m_spallRequiredSolidVolumeM3,
                detachmentCache.m_spallMeasuredSolidVolumeM3,
                spallGeometryResidualM3,
                detachmentCache.m_spallMaximumVolumeResidualM3,
                detachmentCache.m_spallGeometryVolumePass
                    ? "VOLUME PASS"
                    : "VOLUME FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                detachmentCache.m_spallGeometryVolumePass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "material-owned Granite topology %s | cached generation YES | steady geology evals unchanged | P3C.10C-2 %s",
                detachmentCache.m_spallGeometryTopologyPass
                    ? "PASS"
                    : "FAIL",
                detachmentCache.m_spallGeometryPass
                    ? "PASS"
                    : "FAIL" );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                detachmentCache.m_spallGeometryPass
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;
        }
        else
        {
            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                "P3C.10B-2 rooted separation fixture unavailable / certificate failed",
                Colors::Red );

            y +=
                row;
        }

        y +=
            row *
            0.6f;

        //-------------------------------------------------------------------------
        // Surface-stone readability certificate
        //-------------------------------------------------------------------------

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "SURFACE BOULDER / COMPANION STONE RECEIPT",
            Colors::White );

        y +=
            row;

        if ( surfaceStoneCache.m_valid )
        {
            SurfaceStoneDebugInstance const& dominantStone =
                surfaceStoneCache.m_dominant;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "R0 event %u | source state %u | mass %.1f kg | diameter %.3f m | height %.3f m",
                surfaceStoneCache.m_sourceDescriptor.m_eventID,
                uint32_t(
                    surfaceStoneCache.m_sourceDescriptor.m_state ),
                float(
                    dominantStone.m_massGrams ) /
                    1000.0f,
                dominantStone.m_supportDiameterM,
                dominantStone.m_totalHeightM );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                Colors::White );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "   XY(%.2f,%.2f) | terrain %.3f m | embed %.3f m | visible %.3f m | above-ground %.0f%%",
                dominantStone.m_centerX,
                dominantStone.m_centerY,
                dominantStone.m_terrainZ,
                dominantStone.m_embedDepthM,
                dominantStone.m_visibleHeightM,
                dominantStone.m_aboveGroundFraction *
                    100.0f );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                dominantStone.m_aboveGroundFraction >=
                        0.50f
                    ? Colors::White
                    : Colors::Red );

            y +=
                row;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "   companions %d | Soil settle max -%.3f m / apron +%.3f m | affected preview samples %d",
                surfaceStoneCache.m_numCompanions,
                surfaceStoneCache.m_maxSoilDepressionM,
                surfaceStoneCache.m_maxSoilApronM,
                surfaceStoneCache.m_numSettledPreviewSamples );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                Colors::White );

            y +=
                row;

            for ( int32_t i = 0;
                  i < surfaceStoneCache.m_numCompanions;
                  ++i )
            {
                SurfaceStoneDebugInstance const& companion =
                    surfaceStoneCache.m_companions[i];

                if ( !companion.m_valid )
                {
                    continue;
                }

                std::snprintf(
                    line,
                    sizeof(
                        line ),
                    "   r%d mass %.2f kg | d %.2f m h %.2f m | embed %.3f | above %.0f%%",
                    i + 1,
                    float(
                        companion.m_massGrams ) /
                        1000.0f,
                    companion.m_supportDiameterM,
                    companion.m_totalHeightM,
                    companion.m_embedDepthM,
                    companion.m_aboveGroundFraction *
                        100.0f );

                drawCtx.DrawText2D(
                    Float2(
                        x,
                        y ),
                    line,
                    Colors::White );

                y +=
                    row;
            }
        }
        else
        {
            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                "No deterministic surface-stone descriptor selected in certification bounds.",
                Colors::Red );

            y +=
                row;
        }

        y +=
            row *
            0.6f;

        //-------------------------------------------------------------------------
        // Joint field
        //-------------------------------------------------------------------------

        GraniteJointFieldSample const seamJoint =
            EvaluateGraniteJointField(
                seed,
                s_graniteGeologicalAncestryID,
                0.0f,
                0.0f,
                GraniteWeatheringState::WeatheredExposure );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "OUTCROP / JOINT FIELD",
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "At seam XY(0,0): joints d=[%+.3f %+.3f %+.3f] m | influence %.3f",
            seamJoint.m_jointDistanceM[0],
            seamJoint.m_jointDistanceM[1],
            seamJoint.m_jointDistanceM[2],
            seamJoint.m_combinedJointInfluence );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row;

        std::snprintf(
            line,
            sizeof(
                line ),
            "seam hierarchy: primary %.2f secondary %.2f | large %+.3f medium %+.3f small %+.3f m",
            seamJoint.m_primaryJointInfluence,
            seamJoint.m_secondaryJointInfluence,
            seamJoint.m_largeScaleReliefM,
            seamJoint.m_mediumScaleReliefM,
            seamJoint.m_smallScaleReliefM );

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            line,
            Colors::White );

        y +=
            row *
            1.4f;

        //-------------------------------------------------------------------------
        // Contact receipts
        //-------------------------------------------------------------------------

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "SOIL / GRANITE CONTACT RECEIPTS",
            Colors::White );

        y +=
            row;

        if ( graniteReceipt.m_found )
        {
            EvaluatedSurfacePoint const& g =
                graniteReceipt.m_surface;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "Granite XY(%.2f,%.2f): struct %+.3f weather %+.3f joint %.2f | Soil thickness %.3f -> exposed %.3f m",
                graniteReceipt.m_x,
                graniteReceipt.m_y,
                g.m_graniteStructuralRelief,
                g.m_graniteWeatheringRelief,
                g.m_graniteJointInfluence,
                g.m_soilThickness,
                AbsFloat(
                    g.m_signedSoilDepth ) );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                Colors::White );

            y +=
                row;
        }

        if ( soilReceipt.m_found )
        {
            EvaluatedSurfacePoint const& s =
                soilReceipt.m_surface;

            std::snprintf(
                line,
                sizeof(
                    line ),
                "Soil XY(%.2f,%.2f): Granite struct %+.3f | joint %.2f | infill +%.3f | Soil survives %.3f m",
                soilReceipt.m_x,
                soilReceipt.m_y,
                s.m_graniteStructuralRelief,
                s.m_graniteJointInfluence,
                s.m_soilGraniteInfillResponse,
                s.m_soilThickness );

            drawCtx.DrawText2D(
                Float2(
                    x,
                    y ),
                line,
                Colors::White );

            y +=
                row;
        }

        y +=
            row *
            0.6f;

        drawCtx.DrawText2D(
            Float2(
                x,
                y ),
            "Granite = coherent body + hierarchical joints + surface history + Soil-filtered exposure + conserved matter.",
            Colors::White );
    }

    //-------------------------------------------------------------------------
    // Debug draw
    //-------------------------------------------------------------------------

    void ProvenanceWorldSystem::DebugDrawWorkbench(
        EntityWorldUpdateContext const& ctx,
        uint32_t                       seed )
    {

        if ( !m_hasGeneratedSurface ||
             m_generatedSeed !=
                 seed )
        {
            GenerateSurfacePatches(
                seed );
        }

        if ( !m_haveMatterBodies )
        {
            InitializeMatterBodies();
        }

        if ( !m_hasGeneratedDebugPreview ||
             m_debugPreviewSeed !=
                 seed )
        {
            GenerateDebugPreviewCache(
                seed );
        }

        bool const seamPass =
            VerifySharedSeam();

        EE_ASSERT(
            seamPass );

        auto drawCtx =
            ctx.GetDebugDrawContext();

        //-------------------------------------------------------------------------
        // Deterministic surface-stone certification cluster
        //
        // Build once per preview-cache identity/seed, then apply the small Soil
        // settling response directly to cached presentation samples. No
        // geological evaluation or stone mesh generation occurs during steady
        // DebugDraw.
        //-------------------------------------------------------------------------

        static SurfaceStoneClusterDebugCache surfaceStoneCache;

        float const previewOriginWorldX =
            float(
                s_patchAOriginX ) *
            s_sampleSpacing;

        float const previewOriginWorldY =
            float(
                s_patchOriginY ) *
            s_sampleSpacing;

        bool const needSurfaceStoneRebuild =
            surfaceStoneCache.m_seed !=
                seed ||
            surfaceStoneCache.m_previewIdentity !=
                m_debugPreviewSamples;

        if ( needSurfaceStoneRebuild )
        {
            if ( BuildSurfaceStoneClusterDebugCache(
                     surfaceStoneCache,
                     seed,
                     m_debugPreviewSamples,
                     s_debugPreviewSamplesX,
                     s_debugPreviewSamplesY,
                     previewOriginWorldX,
                     previewOriginWorldY,
                     s_debugPreviewSpacing ) )
            {
                ApplySurfaceStoneSoilSettlingToPreview(
                    surfaceStoneCache,
                    m_debugPreviewSamples,
                    s_debugPreviewSamplesX,
                    s_debugPreviewSamplesY,
                    previewOriginWorldX,
                    previewOriginWorldY,
                    s_debugPreviewSpacing );
            }
        }

        //-------------------------------------------------------------------------
        // Terrain from cached 0.25 m samples
        //-------------------------------------------------------------------------

        int32_t constexpr previewQuadsPerPatchX =
            s_patchQuads *
            s_debugPreviewSubdivisionsPerMeter;

        if ( s_p3c9c4VisibleGeometryPass )
        {
            DrawP3C9C4VisibleSoilTerrain(
                drawCtx );
        }
        else
        {
            DrawCachedSurfacePatch(
                drawCtx,
                m_debugPreviewSamples,
                s_debugPreviewSamplesX,
                0,
                0,
                previewQuadsPerPatchX,
                s_debugPreviewQuadsY,
                float( s_patchAOriginX ) *
                    s_sampleSpacing,
                float( s_patchOriginY ) *
                    s_sampleSpacing,
                s_debugPreviewSpacing );

            DrawCachedSurfacePatch(
                drawCtx,
                m_debugPreviewSamples,
                s_debugPreviewSamplesX,
                previewQuadsPerPatchX,
                0,
                previewQuadsPerPatchX,
                s_debugPreviewQuadsY,
                float( s_patchBOriginX ) *
                    s_sampleSpacing,
                float( s_patchOriginY ) *
                    s_sampleSpacing,
                s_debugPreviewSpacing );
        }

        //-------------------------------------------------------------------------
        // Cyan package seam from cached samples
        //-------------------------------------------------------------------------

        if ( s_showPackageSeam )
        {
            int32_t constexpr seamSampleX =
                previewQuadsPerPatchX;

            float const seamX =
                0.0f;

            float const seamStartY =
                float(
                    s_patchOriginY ) *
                s_sampleSpacing;

            for ( int32_t i = 0;
                  i <
                  s_debugPreviewQuadsY;
                  ++i )
            {
                int32_t const idx0 =
                    i *
                        s_debugPreviewSamplesX +
                    seamSampleX;

                int32_t const idx1 =
                    ( i + 1 ) *
                        s_debugPreviewSamplesX +
                    seamSampleX;

                float const y0 =
                    seamStartY +
                    float( i ) *
                        s_debugPreviewSpacing;

                float const y1 =
                    y0 +
                    s_debugPreviewSpacing;

                drawCtx.DrawLine(
                    Float3(
                        seamX,
                        y0,
                        m_debugPreviewSamples[idx0].m_elevation +
                            0.035f ),

                    Float3(
                        seamX,
                        y1,
                        m_debugPreviewSamples[idx1].m_elevation +
                            0.035f ),

                    Colors::Cyan,
                    2.5f,
                    DebugDrawLayer::World );
            }
        }

        //-------------------------------------------------------------------------
        // Surface boulder + companion stones
        //-------------------------------------------------------------------------

        DrawSurfaceStoneCluster(
            drawCtx,
            surfaceStoneCache );

        //-------------------------------------------------------------------------
        // Workbench
        //-------------------------------------------------------------------------

        GraniteSpecimenMetrics graniteMetrics[3];

        for ( int32_t i = 0;
              i <
              s_numMatterBodies;
              ++i )
        {
            GraniteSpecimenMetrics const metrics =
                DrawMatterBody(
                    drawCtx,
                    m_matterBodies[i],
                    seed );

            if ( i <
                 3 )
            {
                graniteMetrics[i] =
                    metrics;
            }
        }

        //-------------------------------------------------------------------------
        // P3C.10A actual B3 fracture transaction
        //
        // The cache is rebuilt only when the world seed changes. Steady
        // DebugDraw reuses the already-generated parent/children.
        //-------------------------------------------------------------------------

        static GraniteFractureDebugCache fractureCache;

        if ( fractureCache.m_seed !=
                 seed ||
             !fractureCache.m_valid )
        {
            BuildGraniteFractureDebugCache(
                fractureCache,
                m_matterBodies[2],
                seed );
        }

        DrawGraniteFractureDebugCache(
            drawCtx,
            fractureCache );

        //-------------------------------------------------------------------------
        // P3C.10B actual RootedRockHead structural detachment sequence
        //
        // Descriptor generation + structural certification occurs only when
        // the deterministic seed changes. No per-frame geological surface
        // reevaluation is added.
        //-------------------------------------------------------------------------

        static GraniteDetachmentDebugCache detachmentCache;

        if ( detachmentCache.m_seed !=
                 seed ||
             !detachmentCache.m_valid )
        {
            BuildGraniteDetachmentDebugCache(
                detachmentCache,
                seed );
        }

        Render::RenderWorldSystem* pRenderWorldSystem =
            ctx.GetWorldSystem<Render::RenderWorldSystem>();

        if ( pRenderWorldSystem != m_pGraniteRenderWorldSystem )
        {
            if ( m_pGraniteRenderWorldSystem != nullptr &&
                 m_graniteRuntimeMeshID != 0 )
            {
                m_pGraniteRenderWorldSystem->UnregisterProceduralMesh
                (
                    m_graniteRuntimeMeshID
                );
            }

            if ( m_pGraniteRenderWorldSystem != nullptr &&
                 m_graniteToolStrikeRuntimeMeshID != 0 )
            {
                m_pGraniteRenderWorldSystem->UnregisterProceduralMesh
                (
                    m_graniteToolStrikeRuntimeMeshID
                );
            }

            m_pGraniteRenderWorldSystem = pRenderWorldSystem;
            m_graniteRuntimeMeshID = 0;
            m_graniteRuntimeMeshSeed = 0;
            m_graniteToolStrikeRuntimeMeshID = 0;
            m_graniteToolStrikeRuntimeRevision = 0;
        }

        if ( m_pGraniteRenderWorldSystem != nullptr &&
             ( m_graniteRuntimeMeshID == 0 ||
               m_graniteRuntimeMeshSeed != seed ) )
        {
            if ( m_graniteRuntimeMeshID != 0 )
            {
                m_pGraniteRenderWorldSystem->UnregisterProceduralMesh
                (
                    m_graniteRuntimeMeshID
                );

                m_graniteRuntimeMeshID = 0;
            }

            m_graniteRuntimeMeshID = RegisterGraniteRuntimeMaterialPreview
            (
                *m_pGraniteRenderWorldSystem,
                m_pSettings->GetGraniteMaterial(),
                detachmentCache
            );

            if ( m_graniteRuntimeMeshID != 0 )
            {
                m_graniteRuntimeMeshSeed = seed;
            }
        }

        DrawGraniteDetachmentDebugCache(
            drawCtx,
            detachmentCache );

        //-------------------------------------------------------------------------
        // P3C.11A-1 player-facing strike lab
        //-------------------------------------------------------------------------

        // Only the game/Play world owns this mutable lab. The editor world
        // also draws certificates each frame and must not reset the lab owner,
        // held-key latches, or accumulated strikes. There is one game world.
        if ( ctx.IsGameWorld() )
        {
            bool const excavationActive = ExcavationLab::Tick(this,ctx,drawCtx,
                m_pGraniteRenderWorldSystem,m_pSettings->GetGraniteMaterial());
            if ( s_graniteToolStrikeLab.m_pOwner != this ||
                 !s_graniteToolStrikeLab.m_initialized )
            {
                InitializeGraniteToolStrikeLab
                (
                    this,
                    seed,
                    s_graniteToolStrikeLab
                );
            }

            // The editor preview owns LMB focus and RMB debug-camera capture. Do
            // not observe either mouse button here: combining viewport capture
            // with a game-side click command can leave the editor in a confusing
            // cursor state. Aim with the ordinary RMB camera, release RMB, then
            // use the explicit F key to submit a strike along the view-center ray.
            Input::InputSystem const* pInputSystem =
                ctx.GetSystem<Input::InputSystem>();
            Viewport const* pMainViewport = ctx.GetMainViewport();

            // KeyboardMouseDevice does not maintain InputDevice::m_isConnected.
            // Like ToolsCameraComponent, read the owned device directly; the
            // connectivity helper otherwise disables this entire input path.
            if ( pInputSystem != nullptr &&
                 pMainViewport != nullptr &&
                 s_graniteToolStrikeLab.m_lastCommandFrameID !=
                     ctx.GetFrameID() )
            {
                Input::KeyboardMouseDevice const* pKeyboardMouse =
                    pInputSystem->GetKeyboardMouse();
                bool const strikeKeyHeld =
                    pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_F );
                bool const resetKeyHeld =
                    pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_R );
                bool const nextKeyHeld =
                    pKeyboardMouse->IsHeldDown( Input::InputID::Keyboard_N );

                bool const newStrikeKeyPress =
                    strikeKeyHeld &&
                    !s_graniteToolStrikeLab.m_strikeKeyHeldLastFrame;
                bool const newResetKeyPress =
                    resetKeyHeld &&
                    !s_graniteToolStrikeLab.m_resetKeyHeldLastFrame;
                bool const newNextKeyPress =
                    nextKeyHeld &&
                    !s_graniteToolStrikeLab.m_nextKeyHeldLastFrame;

                s_graniteToolStrikeLab.m_strikeKeyHeld = strikeKeyHeld;
                s_graniteToolStrikeLab.m_strikeKeyHeldLastFrame = strikeKeyHeld;
                s_graniteToolStrikeLab.m_resetKeyHeldLastFrame = resetKeyHeld;
                s_graniteToolStrikeLab.m_nextKeyHeldLastFrame = nextKeyHeld;

                if ( !excavationActive && !ExcavationLab::state.g && newNextKeyPress )
                {
                    ResetGraniteToolStrikeLab( ctx, true );
                }
                else if ( !excavationActive && !ExcavationLab::state.g && newResetKeyPress )
                {
                    ResetGraniteToolStrikeLab( ctx, false );
                }
                else if ( !excavationActive && !ExcavationLab::state.g && newStrikeKeyPress )
                {
                    TryGraniteToolStrike
                    (
                        ctx,
                        pMainViewport->GetViewPosition().ToFloat3(),
                        pMainViewport->GetViewForwardDirection().ToFloat3()
                    );
                }
            }

            if ( m_pGraniteRenderWorldSystem != nullptr &&
                 s_graniteToolStrikeLab.m_initialized &&
                 ( m_graniteToolStrikeRuntimeMeshID == 0 ||
                   m_graniteToolStrikeRuntimeRevision !=
                       s_graniteToolStrikeLab.m_revision ) )
            {
                if ( m_graniteToolStrikeRuntimeMeshID != 0 )
                {
                    m_pGraniteRenderWorldSystem->UnregisterProceduralMesh
                    (
                        m_graniteToolStrikeRuntimeMeshID
                    );
                }

                m_graniteToolStrikeRuntimeMeshID =
                    RegisterGraniteToolStrikeRuntimePreview
                    (
                        *m_pGraniteRenderWorldSystem,
                        m_pSettings->GetGraniteMaterial(),
                        s_graniteToolStrikeLab
                    );
                m_graniteToolStrikeRuntimeRevision =
                    s_graniteToolStrikeLab.m_revision;
            }

            if ( !excavationActive ) DrawGraniteToolStrikeLab
            (
                drawCtx,
                s_graniteToolStrikeLab
            );

            if ( !excavationActive && pMainViewport != nullptr )
            {
                Float2 const viewportSize = pMainViewport->GetDimensions();
                drawCtx.DrawText2D
                (
                    Float2( viewportSize.m_x * 0.5f, viewportSize.m_y * 0.5f ),
                    "[ + ]",
                    Colors::Yellow,
                    DebugFont::Normal,
                    DebugTextAlign::MiddleCenter
                );
            }
        }

        //-------------------------------------------------------------------------
        // Convert cached receipts to the existing presentation receipt type.
        // No world scan occurs here.
        //-------------------------------------------------------------------------

        ExposureReceipt graniteReceipt;

        if ( m_cachedGraniteReceipt.m_found )
        {
            graniteReceipt.m_found =
                true;

            graniteReceipt.m_x =
                m_cachedGraniteReceipt.m_x;

            graniteReceipt.m_y =
                m_cachedGraniteReceipt.m_y;

            graniteReceipt.m_surface.m_graniteStructuralRelief =
                m_cachedGraniteReceipt.m_graniteStructuralRelief;

            graniteReceipt.m_surface.m_graniteWeatheringRelief =
                m_cachedGraniteReceipt.m_graniteWeatheringRelief;

            graniteReceipt.m_surface.m_graniteJointInfluence =
                m_cachedGraniteReceipt.m_graniteJointInfluence;

            graniteReceipt.m_surface.m_soilGraniteInfillResponse =
                m_cachedGraniteReceipt.m_soilGraniteInfillResponse;

            graniteReceipt.m_surface.m_soilThickness =
                m_cachedGraniteReceipt.m_soilThickness;

            graniteReceipt.m_surface.m_signedSoilDepth =
                m_cachedGraniteReceipt.m_signedSoilDepth;
        }

        ExposureReceipt soilReceipt;

        if ( m_cachedSoilReceipt.m_found )
        {
            soilReceipt.m_found =
                true;

            soilReceipt.m_x =
                m_cachedSoilReceipt.m_x;

            soilReceipt.m_y =
                m_cachedSoilReceipt.m_y;

            soilReceipt.m_surface.m_graniteStructuralRelief =
                m_cachedSoilReceipt.m_graniteStructuralRelief;

            soilReceipt.m_surface.m_graniteWeatheringRelief =
                m_cachedSoilReceipt.m_graniteWeatheringRelief;

            soilReceipt.m_surface.m_graniteJointInfluence =
                m_cachedSoilReceipt.m_graniteJointInfluence;

            soilReceipt.m_surface.m_soilGraniteInfillResponse =
                m_cachedSoilReceipt.m_soilGraniteInfillResponse;

            soilReceipt.m_surface.m_soilThickness =
                m_cachedSoilReceipt.m_soilThickness;

            soilReceipt.m_surface.m_signedSoilDepth =
                m_cachedSoilReceipt.m_signedSoilDepth;
        }

        //-------------------------------------------------------------------------
        // Certification UI
        //-------------------------------------------------------------------------

        DrawDebugPanel(
            drawCtx,
            m_matterBodies,
            s_numMatterBodies,
            graniteMetrics,
            seed,
            seamPass,
            m_debugPreviewEvaluationCount,
            graniteReceipt,
            soilReceipt,
            surfaceStoneCache,
            fractureCache,
            detachmentCache );
    }

}

#endif
