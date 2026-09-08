#pragma once

#include "Game/_Module/API.h"
#include "Base/Memory/Memory.h"

namespace EE
{
    //-------------------------------------------------------------------------
    // Material identity
    //-------------------------------------------------------------------------

    enum class ProvenanceMaterialID : uint8_t
    {
        Soil = 0,
        Granite
    };

    //-------------------------------------------------------------------------
    // Current geometric / structural body state
    //-------------------------------------------------------------------------

    enum class ProvenanceBodyState : uint8_t
    {
        Continuous = 0,
        Fragment,
        LooseAggregate,
        Compacted,
        Bonded
    };

    //-------------------------------------------------------------------------
    // Material-owned geometry grammar
    //
    // Density describes actual solid matter.
    // Packing belongs to body state and is evaluated separately.
    //-------------------------------------------------------------------------

    struct MaterialGeometryProfile
    {
        ProvenanceMaterialID m_material =
            ProvenanceMaterialID::Soil;

        // kg of actual solid matter per cubic metre of solids.
        float m_intrinsicSolidDensityKgPerM3 = 1000.0f;

        // Smallest scale at which this material needs distinct
        // gameplay-visible geometry.
        float m_minimumMeaningfulFeatureScaleM = 0.05f;

        // Continuous-body geometry.
        float m_surfaceSmoothAmplitude = 0.0f;
        float m_surfaceReliefAmplitude = 0.0f;

        // Cross-scale characteristic.
        //
        // 0 = smoother / rounder
        // 1 = strongly angular / faceted
        float m_angularCharacter = 0.0f;

        // Aggregate behavior.
        float m_aggregateSpread = 1.0f;

        // Ability to retain coherent form.
        float m_cohesion = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Unified geometry request
    //-------------------------------------------------------------------------

    struct MaterialGeometryRequest
    {
        ProvenanceMaterialID m_material =
            ProvenanceMaterialID::Soil;

        ProvenanceBodyState m_bodyState =
            ProvenanceBodyState::Continuous;

        // Physical scale in metres.
        float m_scale = 1.0f;

        uint32_t m_seed = 0;

        // Deterministic normalized signals supplied by world/body generation.
        float m_primarySignal = 0.5f;
        float m_secondarySignal = 0.5f;
        float m_tertiarySignal = 0.5f;
    };

    //-------------------------------------------------------------------------
    // Geometry interpretation
    //-------------------------------------------------------------------------

    struct MaterialGeometryResult
    {
        // Used by continuous terrain.
        float m_surfaceOffset = 0.0f;

        // Used by fragments and coherent detached bodies.
        float m_angularness = 0.0f;

        // Horizontal/body spread tendency.
        float m_spread = 1.0f;

        // Vertical geometric character.
        float m_heightScale = 1.0f;

        float m_cohesion = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Physical volume metrics
    //
    // Mass is authoritative.
    //
    // solidVolume:
    //     actual physical volume of the matter itself
    //
    // envelopeVolume:
    //     occupied bulk envelope including void space caused by packing
    //-------------------------------------------------------------------------

    struct MatterVolumeMetrics
    {
        float m_massKg = 0.0f;

        float m_solidDensityKgPerM3 = 0.0f;

        float m_solidVolumeM3 = 0.0f;

        float m_packingFraction = 1.0f;

        float m_envelopeVolumeM3 = 0.0f;

        // Cube-root reference dimension used by geometry generation.
        float m_characteristicLengthM = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Material API
    //-------------------------------------------------------------------------

    EE_GAME_API MaterialGeometryProfile GetMaterialGeometryProfile(
        ProvenanceMaterialID material );

    EE_GAME_API float GetBodyStatePackingFraction(
        ProvenanceMaterialID material,
        ProvenanceBodyState  bodyState );

    EE_GAME_API MatterVolumeMetrics CalculateMatterVolume(
        ProvenanceMaterialID material,
        ProvenanceBodyState  bodyState,
        uint32_t             massGrams );

    EE_GAME_API MaterialGeometryResult EvaluateMaterialGeometry(
        MaterialGeometryRequest const& request );
}
