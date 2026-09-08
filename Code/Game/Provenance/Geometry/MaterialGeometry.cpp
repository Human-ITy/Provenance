#include "MaterialGeometry.h"

#include <cmath>

namespace EE
{
    //-------------------------------------------------------------------------
    // Helpers
    //-------------------------------------------------------------------------

    static float CenterSignal(
        float value )
    {
        return value - 0.5f;
    }

    //-------------------------------------------------------------------------
    // Material definitions
    //-------------------------------------------------------------------------

    MaterialGeometryProfile GetMaterialGeometryProfile(
        ProvenanceMaterialID material )
    {
        switch ( material )
        {
            case ProvenanceMaterialID::Granite:
            {
                MaterialGeometryProfile profile;

                profile.m_material =
                    ProvenanceMaterialID::Granite;

                // Approximate intrinsic granite solid density.
                profile.m_intrinsicSolidDensityKgPerM3 =
                    2700.0f;

                profile.m_minimumMeaningfulFeatureScaleM =
                    0.10f;

                profile.m_surfaceSmoothAmplitude =
                    0.35f;

                profile.m_surfaceReliefAmplitude =
                    1.15f;

                profile.m_angularCharacter =
                    0.95f;

                profile.m_aggregateSpread =
                    0.85f;

                profile.m_cohesion =
                    0.95f;

                return profile;
            }

            case ProvenanceMaterialID::Soil:
            default:
            {
                MaterialGeometryProfile profile;

                profile.m_material =
                    ProvenanceMaterialID::Soil;

                // Particle / solid density.
                //
                // This is NOT bulk soil density.
                // Packing and porosity are handled separately.
                profile.m_intrinsicSolidDensityKgPerM3 =
                    2650.0f;

                profile.m_minimumMeaningfulFeatureScaleM =
                    0.05f;

                profile.m_surfaceSmoothAmplitude =
                    0.55f;

                profile.m_surfaceReliefAmplitude =
                    0.60f;

                profile.m_angularCharacter =
                    0.18f;

                profile.m_aggregateSpread =
                    1.25f;

                profile.m_cohesion =
                    0.45f;

                return profile;
            }
        }
    }

    //-------------------------------------------------------------------------
    // Packing / porosity
    //
    // This is deliberately separate from intrinsic solid density.
    //-------------------------------------------------------------------------

    float GetBodyStatePackingFraction(
        ProvenanceMaterialID material,
        ProvenanceBodyState  bodyState )
    {
        if ( material == ProvenanceMaterialID::Granite )
        {
            switch ( bodyState )
            {
                case ProvenanceBodyState::LooseAggregate:
                {
                    return 0.65f;
                }

                case ProvenanceBodyState::Compacted:
                {
                    return 0.95f;
                }

                case ProvenanceBodyState::Continuous:
                case ProvenanceBodyState::Fragment:
                case ProvenanceBodyState::Bonded:
                default:
                {
                    return 1.0f;
                }
            }
        }

        // Provisional mineral-soil packing values.
        switch ( bodyState )
        {
            case ProvenanceBodyState::LooseAggregate:
            {
                return 0.50f;
            }

            case ProvenanceBodyState::Compacted:
            {
                return 0.70f;
            }

            case ProvenanceBodyState::Fragment:
            {
                return 0.62f;
            }

            case ProvenanceBodyState::Bonded:
            {
                return 0.75f;
            }

            case ProvenanceBodyState::Continuous:
            default:
            {
                return 0.60f;
            }
        }
    }

    //-------------------------------------------------------------------------
    // Mass → physical volume
    //-------------------------------------------------------------------------

    MatterVolumeMetrics CalculateMatterVolume(
        ProvenanceMaterialID material,
        ProvenanceBodyState  bodyState,
        uint32_t             massGrams )
    {
        MaterialGeometryProfile const profile =
            GetMaterialGeometryProfile(
                material );

        MatterVolumeMetrics result;

        result.m_massKg =
            float( massGrams ) /
            1000.0f;

        result.m_solidDensityKgPerM3 =
            profile.m_intrinsicSolidDensityKgPerM3;

        if ( result.m_solidDensityKgPerM3 <= 0.0f )
        {
            result.m_solidDensityKgPerM3 =
                1.0f;
        }

        result.m_solidVolumeM3 =
            result.m_massKg /
            result.m_solidDensityKgPerM3;

        result.m_packingFraction =
            GetBodyStatePackingFraction(
                material,
                bodyState );

        if ( result.m_packingFraction < 0.01f )
        {
            result.m_packingFraction =
                0.01f;
        }

        result.m_envelopeVolumeM3 =
            result.m_solidVolumeM3 /
            result.m_packingFraction;

        if ( result.m_envelopeVolumeM3 > 0.0f )
        {
            result.m_characteristicLengthM =
                float(
                    std::cbrt(
                        double(
                            result.m_envelopeVolumeM3 ) ) );
        }

        return result;
    }

    //-------------------------------------------------------------------------
    // Unified material-owned geometry evaluator
    //-------------------------------------------------------------------------

    MaterialGeometryResult EvaluateMaterialGeometry(
        MaterialGeometryRequest const& request )
    {
        MaterialGeometryProfile const profile =
            GetMaterialGeometryProfile(
                request.m_material );

        MaterialGeometryResult result;

        result.m_cohesion =
            profile.m_cohesion;

        float const primary =
            CenterSignal(
                request.m_primarySignal );

        float const secondary =
            CenterSignal(
                request.m_secondarySignal );

        float const tertiary =
            CenterSignal(
                request.m_tertiarySignal );

        switch ( request.m_bodyState )
        {
                //-------------------------------------------------------------------------
                // Continuous geological / terrain body
                //-------------------------------------------------------------------------

            case ProvenanceBodyState::Continuous:
            {
                float const angularAmplitude =
                    profile.m_surfaceReliefAmplitude *
                    profile.m_angularCharacter;

                result.m_surfaceOffset =
                    primary *
                        profile.m_surfaceSmoothAmplitude +
                    secondary *
                        angularAmplitude +
                    tertiary *
                        angularAmplitude *
                        0.45f;

                result.m_angularness =
                    profile.m_angularCharacter;

                result.m_spread =
                    1.0f;

                result.m_heightScale =
                    1.0f;

                break;
            }

                //-------------------------------------------------------------------------
                // Fragment / clod
                //-------------------------------------------------------------------------

            case ProvenanceBodyState::Fragment:
            {
                result.m_angularness =
                    profile.m_angularCharacter;

                result.m_spread =
                    0.78f +
                    secondary *
                        0.14f;

                result.m_heightScale =
                    0.90f +
                    tertiary *
                        0.25f +
                    profile.m_cohesion *
                        0.10f;

                break;
            }

                //-------------------------------------------------------------------------
                // Loose aggregate
                //-------------------------------------------------------------------------

            case ProvenanceBodyState::LooseAggregate:
            {
                result.m_angularness =
                    profile.m_angularCharacter *
                    0.75f;

                result.m_spread =
                    profile.m_aggregateSpread;

                result.m_heightScale =
                    0.55f +
                    profile.m_cohesion *
                        0.12f;

                break;
            }

                //-------------------------------------------------------------------------
                // Compacted
                //-------------------------------------------------------------------------

            case ProvenanceBodyState::Compacted:
            {
                result.m_angularness =
                    profile.m_angularCharacter *
                    0.25f;

                result.m_spread =
                    profile.m_aggregateSpread *
                    1.08f;

                result.m_heightScale =
                    0.22f +
                    profile.m_cohesion *
                        0.12f;

                break;
            }

                //-------------------------------------------------------------------------
                // Bonded / structurally admitted
                //-------------------------------------------------------------------------

            case ProvenanceBodyState::Bonded:
            {
                result.m_angularness =
                    profile.m_angularCharacter *
                    0.55f;

                result.m_spread =
                    0.95f;

                result.m_heightScale =
                    1.05f;

                break;
            }

            default:
            {
                break;
            }
        }

        return result;
    }
}
