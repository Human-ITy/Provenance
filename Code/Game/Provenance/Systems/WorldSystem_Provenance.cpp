#include "WorldSystem_Provenance.h"
#include "ProvenanceSurfaceEvaluation.h"

#include "Game/Provenance/Components/Component_ProvenanceWorldSettings.h"

#include "Engine/Entity/Entity.h"
#include "Engine/Render/Systems/WorldSystem_Render.h"

namespace EE
{
    //-------------------------------------------------------------------------
    // Package generation
    //-------------------------------------------------------------------------

    void ProvenanceWorldSystem::GenerateSurfacePatch(
        int32_t        originX,
        int32_t        originY,
        uint32_t       seed,
        SurfaceSample* pSamples )
    {
        EE_ASSERT(
            pSamples !=
            nullptr );

        for ( int32_t y = 0;
              y <
              s_patchSamples;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  s_patchSamples;
                  ++x )
            {
                float const worldX =
                    float(
                        originX +
                        x ) *
                    s_sampleSpacing;

                float const worldY =
                    float(
                        originY +
                        y ) *
                    s_sampleSpacing;

                int32_t const idx =
                    y *
                        s_patchSamples +
                    x;

                EvaluatedSurfacePoint const evaluated =
                    EvaluateSurfacePoint(
                        seed,
                        worldX,
                        worldY );

                SurfaceSample& sample =
                    pSamples[idx];

                sample.m_bedrockElevation =
                    evaluated.m_bedrockElevation;

                sample.m_signedSoilDepth =
                    evaluated.m_signedSoilDepth;

                sample.m_soilThickness =
                    evaluated.m_soilThickness;

                sample.m_elevation =
                    evaluated.m_elevation;

                sample.m_material =
                    evaluated.m_material;
            }
        }
    }

    //-------------------------------------------------------------------------

    void ProvenanceWorldSystem::GenerateSurfacePatches(
        uint32_t seed )
    {
        GenerateSurfacePatch(
            s_patchAOriginX,
            s_patchOriginY,
            seed,
            m_patchA );

        GenerateSurfacePatch(
            s_patchBOriginX,
            s_patchOriginY,
            seed,
            m_patchB );

        EE_ASSERT(
            VerifySharedSeam() );

        m_generatedSeed =
            seed;

        m_hasGeneratedSurface =
            true;

#if EE_DEVELOPMENT_TOOLS
        // Any regenerated package truth invalidates the derived preview cache.
        m_hasGeneratedDebugPreview =
            false;
#endif
    }

    //-------------------------------------------------------------------------
    // Hard package seam certification
    //-------------------------------------------------------------------------

    bool ProvenanceWorldSystem::VerifySharedSeam() const
    {
        for ( int32_t y = 0;
              y <
              s_patchSamples;
              ++y )
        {
            int32_t const idxA =
                y *
                    s_patchSamples +
                s_patchQuads;

            int32_t const idxB =
                y *
                s_patchSamples;

            SurfaceSample const& a =
                m_patchA[idxA];

            SurfaceSample const& b =
                m_patchB[idxB];

            if ( a.m_bedrockElevation !=
                 b.m_bedrockElevation )
            {
                return false;
            }

            if ( a.m_signedSoilDepth !=
                 b.m_signedSoilDepth )
            {
                return false;
            }

            if ( a.m_soilThickness !=
                 b.m_soilThickness )
            {
                return false;
            }

            if ( a.m_elevation !=
                 b.m_elevation )
            {
                return false;
            }

            if ( a.m_material !=
                 b.m_material )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------
    // Lifecycle
    //-------------------------------------------------------------------------

    void ProvenanceWorldSystem::ShutdownSystem()
    {
        if ( m_pGraniteRenderWorldSystem != nullptr &&
             m_graniteRuntimeMeshID != 0 )
        {
            m_pGraniteRenderWorldSystem->UnregisterProceduralMesh
            (
                m_graniteRuntimeMeshID
            );
        }

#if EE_DEVELOPMENT_TOOLS
        if ( m_pGraniteRenderWorldSystem != nullptr &&
             m_graniteToolStrikeRuntimeMeshID != 0 )
        {
            m_pGraniteRenderWorldSystem->UnregisterProceduralMesh
            (
                m_graniteToolStrikeRuntimeMeshID
            );
        }
#endif

        m_graniteRuntimeMeshID = 0;
        m_graniteRuntimeMeshSeed = 0;
        m_pGraniteRenderWorldSystem = nullptr;

#if EE_DEVELOPMENT_TOOLS
        m_graniteToolStrikeRuntimeMeshID = 0;
        m_graniteToolStrikeRuntimeRevision = 0;
        ReleaseGraniteToolStrikeLabState();
#endif

        EE_ASSERT(
            m_pSettings ==
            nullptr );
    }

    //-------------------------------------------------------------------------

    void ProvenanceWorldSystem::RegisterComponent(
        Entity*          pEntity,
        EntityComponent* pComponent )
    {
        if ( auto pSettings =
                 TryCast<
                     ProvenanceWorldSettingsComponent>(
                     pComponent ) )
        {
            EE_ASSERT(
                m_pSettings ==
                nullptr );

            m_pSettings =
                pSettings;
        }
    }

    //-------------------------------------------------------------------------

    void ProvenanceWorldSystem::UnregisterComponent(
        Entity*          pEntity,
        EntityComponent* pComponent )
    {
        if ( pComponent ==
             m_pSettings )
        {
            if ( m_pGraniteRenderWorldSystem != nullptr &&
                 m_graniteRuntimeMeshID != 0 )
            {
                m_pGraniteRenderWorldSystem->UnregisterProceduralMesh
                (
                    m_graniteRuntimeMeshID
                );
            }

            m_graniteRuntimeMeshID = 0;
            m_graniteRuntimeMeshSeed = 0;

#if EE_DEVELOPMENT_TOOLS
            if ( m_pGraniteRenderWorldSystem != nullptr &&
                 m_graniteToolStrikeRuntimeMeshID != 0 )
            {
                m_pGraniteRenderWorldSystem->UnregisterProceduralMesh
                (
                    m_graniteToolStrikeRuntimeMeshID
                );
            }

            m_graniteToolStrikeRuntimeMeshID = 0;
            m_graniteToolStrikeRuntimeRevision = 0;
            ReleaseGraniteToolStrikeLabState();
#endif
            m_pGraniteRenderWorldSystem = nullptr;
            m_pSettings =
                nullptr;
        }
    }

}
