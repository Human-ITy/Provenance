#pragma once

#include "Game/_Module/API.h"
#include "Game/Provenance/Geometry/MaterialGeometry.h"
#include "Engine/Entity/EntityWorldSystem.h"

//
// Implementation is intentionally split across:
//   WorldSystem_Provenance.cpp            - runtime orchestration/package state
//   WorldSystem_Provenance_Workbench.cpp  - deterministic matter fixtures
//   WorldSystem_Provenance_Debug.cpp      - playable/workbench routing and cleanup
//   WorldSystem_Provenance_DebugWorkbench.cpp - historical visualization/certificates
//   GraniteOutcropLab.cpp                - current playable outcrop adapter
//   ProvenanceSurfaceEvaluation.cpp       - absolute-coordinate geological truth
//
// The reflected ProvenanceWorldSystem layout remains unchanged by this split.
//

namespace EE
{
    class ProvenanceWorldSettingsComponent;
    namespace Render { class RenderWorldSystem; }

    class EE_GAME_API ProvenanceWorldSystem final : public EntityWorldSystem
    {
        EE_ENTITY_WORLD_SYSTEM(
            ProvenanceWorldSystem,
            RequiresUpdate( UpdateStage::FrameEnd ) );

    public:

        //-------------------------------------------------------------------------
        // Shared terrain sample
        //
        // Geological truth remains separate:
        //
        //     Granite bedrock
        //     Soil cover
        //     resulting exposure
        //
        // No morphology blend weight represents material quantity.
        //-------------------------------------------------------------------------

        struct SurfaceSample
        {
            // Actual Granite boundary at this XY.
            float m_bedrockElevation = 0.0f;

            // Candidate Soil surface minus actual Granite boundary.
            //
            // Positive:
            //     Soil survives above Granite.
            //
            // Zero:
            //     geological exposure contact.
            //
            // Negative:
            //     Granite protrudes through the Soil candidate.
            float m_signedSoilDepth = 0.0f;

            // Actual physical Soil thickness.
            float m_soilThickness = 0.0f;

            // Final exposed terrain elevation.
            float m_elevation = 0.0f;

            // Material actually exposed at this sample.
            ProvenanceMaterialID m_material =
                ProvenanceMaterialID::Granite;
        };

        //-------------------------------------------------------------------------
        // Provenance contribution
        //-------------------------------------------------------------------------

        struct MatterContribution
        {
            uint32_t m_sourceBodyID = 0;

            uint32_t m_provenanceID = 0;

            uint32_t m_massGrams = 0;
        };

        //-------------------------------------------------------------------------
        // Matter Body
        //
        // Mass is authoritative.
        //
        // Geometry may not store arbitrary physical size.
        //-------------------------------------------------------------------------

        struct MatterBody
        {
            // Stable current body identity.
            uint32_t m_bodyID = 0;

            // Single-parent ancestry retained for later split lineage.
            uint32_t m_parentBodyID = 0;

            // Accounting/provenance identity.
            uint32_t m_provenanceID = 0;

            // Geological body/formation ancestry.
            uint32_t m_geologicalAncestryID = 0;

            // Structural revision.
            uint32_t m_structuralRevision = 1;

            // Matter identity.
            ProvenanceMaterialID m_material =
                ProvenanceMaterialID::Soil;

            // Structural/body state.
            ProvenanceBodyState m_bodyState =
                ProvenanceBodyState::Fragment;

            // Conserved matter quantity.
            uint32_t m_massGrams = 0;

            // World placement only.
            float m_centerX = 0.0f;

            float m_centerY = 0.0f;

            float m_baseZ = 0.0f;

            // Deterministic body-specific geometry salt.
            uint32_t m_geometrySalt = 0;

            //-------------------------------------------------------------------------
            // Multi-source provenance
            //-------------------------------------------------------------------------

            static constexpr int32_t s_maxContributors =
                4;

            MatterContribution m_contributors[s_maxContributors];

            int32_t m_numContributors = 0;
        };

#if EE_DEVELOPMENT_TOOLS
        //-------------------------------------------------------------------------
        // P3C.11A-1 playable Granite tool-strike lab
        //
        // Input and ray acquisition are engine-facing here. All structural
        // selection and state transition decisions are delegated to the POD
        // GraniteToolStrike contract in GraniteGeometry.
        //-------------------------------------------------------------------------

        bool TryGraniteToolStrike(
            EntityWorldUpdateContext const& ctx,
            Float3 const&                   rayOrigin,
            Float3 const&                   rayDirection );

        void ResetGraniteToolStrikeLab(
            EntityWorldUpdateContext const& ctx,
            bool                            advanceSeed );

        void ReleaseGraniteToolStrikeLabState();
#endif

    private:

        //-------------------------------------------------------------------------
        // World-system lifecycle
        //-------------------------------------------------------------------------

        virtual void ShutdownSystem() override;

        virtual void RegisterComponent(
            Entity*          pEntity,
            EntityComponent* pComponent ) override;

        virtual void UnregisterComponent(
            Entity*          pEntity,
            EntityComponent* pComponent ) override;

#if EE_DEVELOPMENT_TOOLS

        virtual void DebugDraw(
            EntityWorldUpdateContext const& ctx ) override;

        // Non-virtual development helpers; no reflected state/layout changes.
        void DebugDrawWorkbench(
            EntityWorldUpdateContext const& ctx,
            uint32_t                       seed );

        void ReleaseDebugWorkbenchState();

#endif

    private:

        //-------------------------------------------------------------------------
        // Continuous terrain packaging
        //
        // These remain the 1 m package/truth samples used for the hard
        // Patch-A / Patch-B seam certificate.
        //-------------------------------------------------------------------------

        static constexpr int32_t s_patchQuads =
            16;

        static constexpr int32_t s_patchSamples =
            s_patchQuads + 1;

        static constexpr int32_t s_samplesPerPatch =
            s_patchSamples *
            s_patchSamples;

        static constexpr float s_sampleSpacing =
            1.0f;

        // Patch A:
        //     X = -16 .. 0
        //
        // Patch B:
        //     X =   0 .. 16
        static constexpr int32_t s_patchAOriginX =
            -16;

        static constexpr int32_t s_patchBOriginX =
            0;

        static constexpr int32_t s_patchOriginY =
            -8;

        void GenerateSurfacePatch(
            int32_t        originX,
            int32_t        originY,
            uint32_t       seed,
            SurfaceSample* pSamples );

        void GenerateSurfacePatches(
            uint32_t seed );

        bool VerifySharedSeam() const;

        //-------------------------------------------------------------------------
        // Material Geometry certification workbench
        //-------------------------------------------------------------------------

        static constexpr int32_t s_numMatterBodies =
            6;

        void InitializeMatterBodies();

#if EE_DEVELOPMENT_TOOLS

    private:

        //-------------------------------------------------------------------------
        // Derived preview cache
        //
        // The geological/world truth is NOT reduced.
        //
        // Each unique 0.25 m preview sample is evaluated once per cache
        // generation and reused during steady DebugDraw.
        //-------------------------------------------------------------------------

        static constexpr int32_t s_debugPreviewSubdivisionsPerMeter =
            4;

        static constexpr int32_t s_debugPreviewQuadsX =
            s_patchQuads *
            2 *
            s_debugPreviewSubdivisionsPerMeter;

        static constexpr int32_t s_debugPreviewQuadsY =
            s_patchQuads *
            s_debugPreviewSubdivisionsPerMeter;

        static constexpr int32_t s_debugPreviewSamplesX =
            s_debugPreviewQuadsX + 1;

        static constexpr int32_t s_debugPreviewSamplesY =
            s_debugPreviewQuadsY + 1;

        static constexpr int32_t s_debugPreviewSampleCount =
            s_debugPreviewSamplesX *
            s_debugPreviewSamplesY;

        static constexpr float s_debugPreviewSpacing =
            s_sampleSpacing /
            float(
                s_debugPreviewSubdivisionsPerMeter );

        //-------------------------------------------------------------------------
        // Minimal derived render sample
        //-------------------------------------------------------------------------

        struct DebugSurfaceSample
        {
            float m_elevation = 0.0f;

            float m_signedSoilDepth = 0.0f;
        };

        //-------------------------------------------------------------------------
        // Cached causal receipt
        //-------------------------------------------------------------------------

        struct DebugExposureReceipt
        {
            bool m_found = false;

            float m_x = 0.0f;

            float m_y = 0.0f;

            float m_graniteStructuralRelief = 0.0f;

            float m_graniteWeatheringRelief = 0.0f;

            float m_graniteJointInfluence = 0.0f;

            float m_soilGraniteInfillResponse = 0.0f;

            float m_soilThickness = 0.0f;

            float m_signedSoilDepth = 0.0f;
        };

        //-------------------------------------------------------------------------
        // Cache generation
        //-------------------------------------------------------------------------

        void GenerateDebugPreviewCache(
            uint32_t seed );

#endif

    private:

        //-------------------------------------------------------------------------
        // World settings
        //-------------------------------------------------------------------------

        ProvenanceWorldSettingsComponent* m_pSettings =
            nullptr;

        //-------------------------------------------------------------------------
        // Terrain package state
        //-------------------------------------------------------------------------

        SurfaceSample m_patchA[s_samplesPerPatch];

        SurfaceSample m_patchB[s_samplesPerPatch];

        uint32_t m_generatedSeed =
            0;

        bool m_hasGeneratedSurface =
            false;

        //-------------------------------------------------------------------------
        // Matter-body certification state
        //-------------------------------------------------------------------------

        MatterBody m_matterBodies[s_numMatterBodies];

        bool m_haveMatterBodies =
            false;

        // Real forward-renderer preview of the same procedural Granite family
        // used by the P3C.10C-2 certificate. DebugDraw remains independent.
        Render::RenderWorldSystem* m_pGraniteRenderWorldSystem = nullptr;

        uint64_t m_graniteRuntimeMeshID = 0;

        uint32_t m_graniteRuntimeMeshSeed = 0;

#if EE_DEVELOPMENT_TOOLS
        uint64_t m_graniteToolStrikeRuntimeMeshID = 0;
        uint32_t m_graniteToolStrikeRuntimeRevision = 0;
#endif

#if EE_DEVELOPMENT_TOOLS

        //-------------------------------------------------------------------------
        // Derived debug-preview cache
        //-------------------------------------------------------------------------

        DebugSurfaceSample m_debugPreviewSamples[s_debugPreviewSampleCount];

        DebugExposureReceipt m_cachedGraniteReceipt;

        DebugExposureReceipt m_cachedSoilReceipt;

        uint32_t m_debugPreviewSeed =
            0;

        uint32_t m_debugPreviewEvaluationCount =
            0;

        bool m_hasGeneratedDebugPreview =
            false;

#endif
    };
}
