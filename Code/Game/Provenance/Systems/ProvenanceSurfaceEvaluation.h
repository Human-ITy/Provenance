#pragma once

#include "Game/_Module/API.h"
#include "Game/Provenance/Geometry/MaterialGeometry.h"
#include "Game/Provenance/Geometry/GraniteGeometry.h"

namespace EE
{
    // Absolute-XY surface law identity. Intentional changes to evaluated
    // terrain truth require a version bump and an attributed corpus re-bank.
    // This is separate from Granite fracture/detachment algorithm identity.
    static constexpr uint32_t s_provenanceSurfaceAlgorithmVersion = 1u;

    //-------------------------------------------------------------------------
    // Absolute-coordinate geological evaluation receipt
    //
    // This module owns the deterministic evaluation path that turns:
    //
    //     world seed + absolute XY
    //
    // into the geological/material truth needed by the world-system package
    // cache and the development preview cache.
    //
    // Package/chunk identity is intentionally absent from this API.
    //-------------------------------------------------------------------------

    struct EvaluatedSurfacePoint
    {
        float m_broadLandform =
            0.0f;

        float m_graniteBroadBodyOffset =
            0.0f;

        float m_bedrockBase =
            0.0f;

        // GraniteGeometry-owned hierarchy.
        float m_graniteLargeScaleRelief =
            0.0f;

        float m_graniteMediumScaleRelief =
            0.0f;

        float m_graniteSmallScaleRelief =
            0.0f;

        float m_granitePrimaryJointRecess =
            0.0f;

        float m_graniteSecondaryJointRecess =
            0.0f;

        float m_graniteStructuralRelief =
            0.0f;

        float m_graniteWeatheringRelief =
            0.0f;

        float m_graniteJointInfluence =
            0.0f;

        GraniteJointFieldSample m_graniteJointField;

        // Actual Granite boundary. This is never attenuated by Soil.
        float m_bedrockElevation =
            0.0f;

        // Soil authority.
        float m_soilCoverBudget =
            0.0f;

        float m_soilMantleResponse =
            0.0f;

        float m_soilGraniteInfillResponse =
            0.0f;

        float m_mantleActivation =
            0.0f;

        // P3C.8 burial-filtering receipts.
        float m_largeGraniteVisibilityUnderSoil =
            0.0f;

        float m_mediumGraniteVisibilityUnderSoil =
            0.0f;

        float m_smallGraniteVisibilityUnderSoil =
            0.0f;

        float m_filteredGraniteInfluenceUnderSoil =
            0.0f;

        float m_soilSurfaceElevation =
            0.0f;

        float m_signedSoilDepth =
            0.0f;

        float m_soilThickness =
            0.0f;

        float m_elevation =
            0.0f;

        ProvenanceMaterialID m_material =
            ProvenanceMaterialID::Granite;
    };

    //-------------------------------------------------------------------------
    // Evaluate absolute-coordinate terrain truth.
    //
    // Same seed + same world XY must always produce the same result regardless
    // of package, chunk, residency, debug-preview, or caller.
    //-------------------------------------------------------------------------

    EE_GAME_API EvaluatedSurfacePoint EvaluateSurfacePoint(
        uint32_t seed,
        float    worldX,
        float    worldY );
}
