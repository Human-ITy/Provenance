#include "SoilGeometry.h"

#include <cmath>

namespace EE
{
    //-------------------------------------------------------------------------
    // P3C.9A / P3C.9B — Soil Material Authority
    //-------------------------------------------------------------------------

    static constexpr float s_soilLoosePackingFraction =
        0.50f;

    static constexpr float s_soilSettledPackingFraction =
        0.60f;

    static constexpr float s_soilCompactedPackingFraction =
        0.70f;

    static constexpr float s_soilLooseRedistributionMobility =
        0.88f;

    static constexpr float s_soilSettledRedistributionMobility =
        0.62f;

    static constexpr float s_soilCompactedRedistributionMobility =
        0.36f;

    static constexpr float s_soilMinimumPositiveValue =
        1.0e-8f;

    static constexpr float s_soilMinimumContinuousRetentionWeight =
        0.12f;

    static constexpr float s_soilVolumeAbsoluteToleranceM3 =
        1.0e-7f;

    static constexpr float s_soilMassAbsoluteToleranceKg =
        1.0e-4f;

    static constexpr float s_soilRelativeTolerance =
        1.0e-5f;

    //-------------------------------------------------------------------------

    static float SoilAbs(
        float value )
    {
        return value < 0.0f
                 ? -value
                 : value;
    }

    //-------------------------------------------------------------------------

    static float SoilMin(
        float a,
        float b )
    {
        return a < b
                 ? a
                 : b;
    }

    //-------------------------------------------------------------------------

    static float SoilMax(
        float a,
        float b )
    {
        return a > b
                 ? a
                 : b;
    }

    //-------------------------------------------------------------------------

    static float SoilClamp01(
        float value )
    {
        if ( value < 0.0f )
        {
            return 0.0f;
        }

        if ( value > 1.0f )
        {
            return 1.0f;
        }

        return value;
    }

    //-------------------------------------------------------------------------

    static bool IsInsideSoilMantleDomain(
        SoilMantleDomain const& domain,
        float                   worldX,
        float                   worldY )
    {
        return worldX >=
                   domain.m_minX &&
               worldX <=
                   domain.m_maxX &&
               worldY >=
                   domain.m_minY &&
               worldY <=
                   domain.m_maxY;
    }

    //-------------------------------------------------------------------------
    // Physical-state packing authority
    //-------------------------------------------------------------------------

    float GetSoilPhysicalStatePackingFraction(
        SoilPhysicalState state )
    {
        switch ( state )
        {
            case SoilPhysicalState::Loose:
            {
                return s_soilLoosePackingFraction;
            }

            case SoilPhysicalState::Settled:
            {
                return s_soilSettledPackingFraction;
            }

            case SoilPhysicalState::Compacted:
            {
                return s_soilCompactedPackingFraction;
            }

            default:
            {
                EE_ASSERT( false );
                return s_soilSettledPackingFraction;
            }
        }
    }

    //-------------------------------------------------------------------------
    // Physical-state redistribution mobility
    //
    // Packing and mobility are related but distinct:
    //
    // packing
    //     controls bulk volume for a fixed mass
    //
    // mobility
    //     controls how strongly that already-authoritative bulk volume may
    //     respond spatially to substrate geometry.
    //-------------------------------------------------------------------------

    float GetSoilPhysicalStateRedistributionMobility(
        SoilPhysicalState state )
    {
        switch ( state )
        {
            case SoilPhysicalState::Loose:
            {
                return s_soilLooseRedistributionMobility;
            }

            case SoilPhysicalState::Settled:
            {
                return s_soilSettledRedistributionMobility;
            }

            case SoilPhysicalState::Compacted:
            {
                return s_soilCompactedRedistributionMobility;
            }

            default:
            {
                EE_ASSERT( false );
                return s_soilSettledRedistributionMobility;
            }
        }
    }

    //-------------------------------------------------------------------------
    // Projected XY footprint
    //-------------------------------------------------------------------------

    float CalculateSoilMantleFootprintArea(
        SoilMantleDomain const& domain )
    {
        float const width =
            domain.m_maxX -
            domain.m_minX;

        float const depth =
            domain.m_maxY -
            domain.m_minY;

        if ( width <=
                 0.0f ||
             depth <=
                 0.0f )
        {
            return 0.0f;
        }

        return width *
               depth;
    }

    //-------------------------------------------------------------------------
    // Independent reconstruction helpers
    //-------------------------------------------------------------------------

    float ReconstructSoilBulkVolumeFromUniformThickness(
        SoilMantleDomain const& domain,
        float                   thicknessZM )
    {
        if ( thicknessZM <=
             0.0f )
        {
            return 0.0f;
        }

        return CalculateSoilMantleFootprintArea(
                   domain ) *
               thicknessZM;
    }

    //-------------------------------------------------------------------------

    float ReconstructSoilMassKg(
        float bulkVolumeM3,
        float packingFraction,
        float particleDensityKgPerM3 )
    {
        if ( bulkVolumeM3 <=
                 0.0f ||
             packingFraction <=
                 0.0f ||
             particleDensityKgPerM3 <=
                 0.0f )
        {
            return 0.0f;
        }

        // Bulk volume contains Soil solids + pore space:
        //
        //     solid volume
        //         =
        //     bulk volume * packing
        //
        //     mass
        //         =
        //     solid volume * particle density
        return bulkVolumeM3 *
               packingFraction *
               particleDensityKgPerM3;
    }

    //-------------------------------------------------------------------------
    // Main P3C.9A authority solve
    //-------------------------------------------------------------------------

    SoilMantleAuthorityResult CalculateSoilMantleAuthority(
        SoilMantleBody const&   body,
        SoilMantleDomain const& domain )
    {
        SoilMantleAuthorityResult result;

        result.m_body =
            body;

        result.m_domain =
            domain;

        result.m_massKg =
            float(
                double(
                    body.m_massGrams ) /
                1000.0 );

        MaterialGeometryProfile const soilProfile =
            GetMaterialGeometryProfile(
                ProvenanceMaterialID::Soil );

        result.m_particleDensityKgPerM3 =
            soilProfile.m_intrinsicSolidDensityKgPerM3;

        result.m_packingFraction =
            GetSoilPhysicalStatePackingFraction(
                body.m_physicalState );

        result.m_footprintAreaM2 =
            CalculateSoilMantleFootprintArea(
                domain );

        EE_ASSERT(
            result.m_particleDensityKgPerM3 >
            0.0f );

        EE_ASSERT(
            result.m_packingFraction >
                0.0f &&
            result.m_packingFraction <=
                1.0f );

        EE_ASSERT(
            result.m_footprintAreaM2 >
            0.0f );

        if ( result.m_particleDensityKgPerM3 <=
                 0.0f ||
             result.m_packingFraction <=
                 0.0f ||
             result.m_packingFraction >
                 1.0f ||
             result.m_footprintAreaM2 <=
                 0.0f )
        {
            return result;
        }

        result.m_solidVolumeM3 =
            result.m_massKg /
            result.m_particleDensityKgPerM3;

        result.m_bulkVolumeM3 =
            result.m_solidVolumeM3 /
            result.m_packingFraction;

        // P3C.9A/B only solve a continuous mantle. Later distribution states
        // require a spatial occupancy topology rather than a uniform authority
        // thickness.
        if ( body.m_distributionState !=
             SoilDistributionState::ContinuousMantle )
        {
            return result;
        }

        result.m_meanThicknessZM =
            result.m_bulkVolumeM3 /
            result.m_footprintAreaM2;

        result.m_integratedBulkVolumeM3 =
            ReconstructSoilBulkVolumeFromUniformThickness(
                domain,
                result.m_meanThicknessZM );

        result.m_reconstructedMassKg =
            ReconstructSoilMassKg(
                result.m_integratedBulkVolumeM3,
                result.m_packingFraction,
                result.m_particleDensityKgPerM3 );

        result.m_bulkVolumeResidualM3 =
            result.m_integratedBulkVolumeM3 -
            result.m_bulkVolumeM3;

        result.m_massResidualKg =
            result.m_reconstructedMassKg -
            result.m_massKg;

        float const bulkVolumeDenominator =
            SoilMax(
                SoilAbs(
                    result.m_bulkVolumeM3 ),
                s_soilMinimumPositiveValue );

        float const massDenominator =
            SoilMax(
                SoilAbs(
                    result.m_massKg ),
                s_soilMinimumPositiveValue );

        result.m_bulkVolumeRelativeResidual =
            SoilAbs(
                result.m_bulkVolumeResidualM3 ) /
            bulkVolumeDenominator;

        result.m_massRelativeResidual =
            SoilAbs(
                result.m_massResidualKg ) /
            massDenominator;

        float const permittedVolumeResidual =
            SoilMax(
                s_soilVolumeAbsoluteToleranceM3,
                SoilAbs(
                    result.m_bulkVolumeM3 ) *
                    s_soilRelativeTolerance );

        float const permittedMassResidual =
            SoilMax(
                s_soilMassAbsoluteToleranceKg,
                SoilAbs(
                    result.m_massKg ) *
                    s_soilRelativeTolerance );

        result.m_conservationPass =
            SoilAbs(
                result.m_bulkVolumeResidualM3 ) <=
                permittedVolumeResidual &&
            SoilAbs(
                result.m_massResidualKg ) <=
                permittedMassResidual;

        EE_ASSERT(
            result.m_conservationPass );

        return result;
    }

    //-------------------------------------------------------------------------
    // Uniform P3C.9A field sample
    //-------------------------------------------------------------------------

    SoilMantleSample EvaluateSoilMantleAuthoritySample(
        SoilMantleAuthorityResult const& authority,
        float                            worldX,
        float                            worldY )
    {
        SoilMantleSample sample;

        sample.m_worldX =
            worldX;

        sample.m_worldY =
            worldY;

        if ( !authority.m_conservationPass ||
             authority.m_body.m_distributionState !=
                 SoilDistributionState::ContinuousMantle ||
             !IsInsideSoilMantleDomain(
                 authority.m_domain,
                 worldX,
                 worldY ) ||
             authority.m_meanThicknessZM <=
                 0.0f )
        {
            return sample;
        }

        sample.m_occupied =
            true;

        sample.m_thicknessZM =
            authority.m_meanThicknessZM;

        return sample;
    }

    //-------------------------------------------------------------------------
    // P3C.9B — geometry-dependent retention
    //
    // IMPORTANT:
    //
    // This is a RELATIVE SPATIAL PREFERENCE only.
    //
    // It does not create Soil, destroy Soil, or directly set thickness.
    // RedistributeContinuousSoilMantle() normalizes the complete field back to
    // the authoritative bulk volume.
    //-------------------------------------------------------------------------

    float CalculateSoilRetentionWeight(
        SoilRetentionSample const& sample,
        SoilPhysicalState          physicalState )
    {
        float const mobility =
            GetSoilPhysicalStateRedistributionMobility(
                physicalState );

        float const slope =
            SoilClamp01(
                sample.m_slope );

        float const convexHigh =
            SoilClamp01(
                sample.m_convexHighInfluence );

        float const concavity =
            SoilClamp01(
                sample.m_concavityInfluence );

        float const banking =
            SoilClamp01(
                sample.m_obstacleBankingInfluence );

        float const protrusion =
            SoilClamp01(
                sample.m_protrusionInfluence );

        // Physical recess depth is converted to a bounded retention response.
        // Roughly:
        //
        //     0 cm     -> 0
        //     10 cm    -> 0.25
        //     20 cm    -> 0.50
        //     40+ cm   -> 1
        //
        // The depth itself remains the caller's physical geometry receipt.
        float const recessInfluence =
            SoilClamp01(
                SoilMax(
                    sample.m_recessDepthM,
                    0.0f ) /
                0.40f );

        // Positive storage terms.
        float const retentionGain =
            concavity * 0.64f +
            recessInfluence * 0.78f +
            banking * 0.52f;

        // Shedding terms.
        float const retentionLoss =
            slope * 0.34f +
            convexHigh * 0.68f +
            protrusion * 0.58f;

        float weight =
            1.0f +
            mobility *
                ( retentionGain -
                  retentionLoss );

        // P3C.9B still requires continuous occupancy. Even an exposed convex
        // high receives some positive Soil thickness. Actual zero-occupancy
        // breach belongs to P3C.9C.
        weight =
            SoilMax(
                weight,
                s_soilMinimumContinuousRetentionWeight );

        // Prevent one extreme recess from monopolizing the entire mantle in
        // this first continuous-redistribution pass.
        weight =
            SoilMin(
                weight,
                2.75f );

        return weight;
    }

    //-------------------------------------------------------------------------
    // P3C.9B — conserved continuous redistribution
    //-------------------------------------------------------------------------

    SoilMantleRedistributionResult RedistributeContinuousSoilMantle(
        SoilMantleAuthorityResult const&  authority,
        SoilMantleDistributionGrid const& grid,
        SoilRetentionSample const*        pRetentionSamples,
        int32_t                           numRetentionSamples,
        SoilRedistributionCell*           pOutCells,
        int32_t                           numOutputCells )
    {
        SoilMantleRedistributionResult result;

        int32_t const numCells =
            grid.m_numCellsX *
            grid.m_numCellsY;

        result.m_numCells =
            numCells;

        result.m_cellAreaM2 =
            grid.m_cellSizeX *
            grid.m_cellSizeY;

        result.m_gridAreaM2 =
            result.m_cellAreaM2 *
            float(
                numCells );

        result.m_authoritativeBulkVolumeM3 =
            authority.m_bulkVolumeM3;

        result.m_authoritativeMassKg =
            authority.m_massKg;

        result.m_uniformMeanThicknessZM =
            result.m_gridAreaM2 >
                    0.0f
                ? authority.m_bulkVolumeM3 /
                      result.m_gridAreaM2
                : 0.0f;

        if ( !authority.m_conservationPass ||
             authority.m_body.m_distributionState !=
                 SoilDistributionState::ContinuousMantle ||
             grid.m_numCellsX <=
                 0 ||
             grid.m_numCellsY <=
                 0 ||
             grid.m_cellSizeX <=
                 0.0f ||
             grid.m_cellSizeY <=
                 0.0f ||
             pRetentionSamples ==
                 nullptr ||
             pOutCells ==
                 nullptr ||
             numRetentionSamples !=
                 numCells ||
             numOutputCells !=
                 numCells ||
             numCells <=
                 0 ||
             result.m_cellAreaM2 <=
                 0.0f )
        {
            return result;
        }

        EE_ASSERT(
            numRetentionSamples ==
            numCells );

        EE_ASSERT(
            numOutputCells ==
            numCells );

        //-------------------------------------------------------------------------
        // Pass 1:
        //
        // Evaluate relative retention only.
        //-------------------------------------------------------------------------

        double sumRawWeights =
            0.0;

        for ( int32_t i = 0;
              i < numCells;
              ++i )
        {
            SoilRetentionSample const& retention =
                pRetentionSamples[i];

            SoilRedistributionCell& cell =
                pOutCells[i];

            cell =
                SoilRedistributionCell();

            cell.m_worldX =
                retention.m_worldX;

            cell.m_worldY =
                retention.m_worldY;

            cell.m_projectedAreaM2 =
                result.m_cellAreaM2;

            cell.m_substrateMaterial =
                retention.m_substrateMaterial;

            cell.m_retentionWeight =
                CalculateSoilRetentionWeight(
                    retention,
                    authority.m_body.m_physicalState );

            sumRawWeights +=
                double(
                    cell.m_retentionWeight );
        }

        if ( sumRawWeights <=
             0.0 )
        {
            return result;
        }

        result.m_meanRawRetentionWeight =
            float(
                sumRawWeights /
                double(
                    numCells ) );

        //-------------------------------------------------------------------------
        // Normalize the entire relative field directly against authoritative
        // bulk volume:
        //
        //     thickness_i
        //         =
        //     rawWeight_i
        //       * authoritativeBulkVolume
        //       / ( cellArea * sum(rawWeights) )
        //
        // Therefore:
        //
        //     sum( thickness_i * cellArea )
        //         =
        //     authoritativeBulkVolume
        //
        // by construction, independent of the retention pattern.
        //-------------------------------------------------------------------------

        double const thicknessPerWeight =
            double(
                authority.m_bulkVolumeM3 ) /
            ( double(
                  result.m_cellAreaM2 ) *
              sumRawWeights );

        result.m_normalizationScale =
            authority.m_meanThicknessZM >
                    0.0f
                ? float(
                      thicknessPerWeight /
                      double(
                          authority.m_meanThicknessZM ) )
                : 0.0f;

        double integratedBulkVolume =
            0.0;

        double convexThicknessSum =
            0.0;

        double retentionThicknessSum =
            0.0;

        result.m_minThicknessZM =
            0.0f;

        result.m_maxThicknessZM =
            0.0f;

        bool firstThickness =
            true;

        bool everyCellOccupied =
            true;

        //-------------------------------------------------------------------------
        // Pass 2:
        //
        // Convert normalized preference into actual Soil thickness/volume.
        //-------------------------------------------------------------------------

        for ( int32_t i = 0;
              i < numCells;
              ++i )
        {
            SoilRetentionSample const& retention =
                pRetentionSamples[i];

            SoilRedistributionCell& cell =
                pOutCells[i];

            cell.m_thicknessZM =
                float(
                    double(
                        cell.m_retentionWeight ) *
                    thicknessPerWeight );

            cell.m_bulkVolumeM3 =
                cell.m_thicknessZM *
                cell.m_projectedAreaM2;

            cell.m_occupied =
                cell.m_thicknessZM >
                0.0f;

            if ( !cell.m_occupied )
            {
                everyCellOccupied =
                    false;
            }

            integratedBulkVolume +=
                double(
                    cell.m_bulkVolumeM3 );

            if ( firstThickness )
            {
                result.m_minThicknessZM =
                    cell.m_thicknessZM;

                result.m_maxThicknessZM =
                    cell.m_thicknessZM;

                firstThickness =
                    false;
            }
            else
            {
                result.m_minThicknessZM =
                    SoilMin(
                        result.m_minThicknessZM,
                        cell.m_thicknessZM );

                result.m_maxThicknessZM =
                    SoilMax(
                        result.m_maxThicknessZM,
                        cell.m_thicknessZM );
            }

            bool const convexHighCell =
                SoilClamp01(
                    retention.m_convexHighInfluence ) >=
                    0.55f ||
                SoilClamp01(
                    retention.m_protrusionInfluence ) >=
                    0.55f;

            bool const retentionLowCell =
                SoilClamp01(
                    retention.m_concavityInfluence ) >=
                    0.45f ||
                retention.m_recessDepthM >=
                    0.08f ||
                SoilClamp01(
                    retention.m_obstacleBankingInfluence ) >=
                    0.50f;

            if ( convexHighCell )
            {
                ++result.m_numConvexHighCells;

                convexThicknessSum +=
                    double(
                        cell.m_thicknessZM );
            }

            if ( retentionLowCell )
            {
                ++result.m_numRetentionLowCells;

                retentionThicknessSum +=
                    double(
                        cell.m_thicknessZM );
            }
        }

        result.m_integratedBulkVolumeM3 =
            float(
                integratedBulkVolume );

        result.m_redistributedMeanThicknessZM =
            result.m_gridAreaM2 >
                    0.0f
                ? result.m_integratedBulkVolumeM3 /
                      result.m_gridAreaM2
                : 0.0f;

        result.m_reconstructedMassKg =
            ReconstructSoilMassKg(
                result.m_integratedBulkVolumeM3,
                authority.m_packingFraction,
                authority.m_particleDensityKgPerM3 );

        result.m_bulkVolumeResidualM3 =
            result.m_integratedBulkVolumeM3 -
            result.m_authoritativeBulkVolumeM3;

        result.m_massResidualKg =
            result.m_reconstructedMassKg -
            result.m_authoritativeMassKg;

        float const bulkVolumeDenominator =
            SoilMax(
                SoilAbs(
                    result.m_authoritativeBulkVolumeM3 ),
                s_soilMinimumPositiveValue );

        float const massDenominator =
            SoilMax(
                SoilAbs(
                    result.m_authoritativeMassKg ),
                s_soilMinimumPositiveValue );

        result.m_bulkVolumeRelativeResidual =
            SoilAbs(
                result.m_bulkVolumeResidualM3 ) /
            bulkVolumeDenominator;

        result.m_massRelativeResidual =
            SoilAbs(
                result.m_massResidualKg ) /
            massDenominator;

        if ( result.m_numConvexHighCells >
             0 )
        {
            result.m_meanConvexHighThicknessZM =
                float(
                    convexThicknessSum /
                    double(
                        result.m_numConvexHighCells ) );
        }

        if ( result.m_numRetentionLowCells >
             0 )
        {
            result.m_meanRetentionLowThicknessZM =
                float(
                    retentionThicknessSum /
                    double(
                        result.m_numRetentionLowCells ) );
        }

        float const permittedVolumeResidual =
            SoilMax(
                s_soilVolumeAbsoluteToleranceM3,
                SoilAbs(
                    result.m_authoritativeBulkVolumeM3 ) *
                    s_soilRelativeTolerance );

        float const permittedMassResidual =
            SoilMax(
                s_soilMassAbsoluteToleranceKg,
                SoilAbs(
                    result.m_authoritativeMassKg ) *
                    s_soilRelativeTolerance );

        result.m_conservationPass =
            SoilAbs(
                result.m_bulkVolumeResidualM3 ) <=
                permittedVolumeResidual &&
            SoilAbs(
                result.m_massResidualKg ) <=
                permittedMassResidual;

        result.m_continuousCoverPass =
            everyCellOccupied &&
            result.m_minThicknessZM >
                0.0f;

        result.m_valid =
            result.m_conservationPass &&
            result.m_continuousCoverPass;

        EE_ASSERT(
            result.m_conservationPass );

        EE_ASSERT(
            result.m_continuousCoverPass );

        return result;
    }

    //-------------------------------------------------------------------------
    // P3C.9C — local minimum stable thickness
    //-------------------------------------------------------------------------

    float CalculateSoilMinimumStableThickness(
        SoilRetentionSample const& sample,
        SoilPhysicalState          physicalState,
        SoilBreachPolicy const&    policy )
    {
        float baseMinimum = policy.m_settledMinimumStableThicknessM;

        switch ( physicalState )
        {
            case SoilPhysicalState::Loose:
            {
                baseMinimum = policy.m_looseMinimumStableThicknessM;
                break;
            }

            case SoilPhysicalState::Settled:
            {
                baseMinimum = policy.m_settledMinimumStableThicknessM;
                break;
            }

            case SoilPhysicalState::Compacted:
            {
                baseMinimum = policy.m_compactedMinimumStableThicknessM;
                break;
            }

            default:
            {
                EE_ASSERT( false );
                break;
            }
        }

        float const slope =
            SoilClamp01(
                sample.m_slope );

        float const convexHigh =
            SoilClamp01(
                sample.m_convexHighInfluence );

        float const protrusion =
            SoilClamp01(
                sample.m_protrusionInfluence );

        float const concavity =
            SoilClamp01(
                sample.m_concavityInfluence );

        float const banking =
            SoilClamp01(
                sample.m_obstacleBankingInfluence );

        float const recessDepthM =
            SoilMax(
                sample.m_recessDepthM,
                0.0f );

        float minimumStableThicknessM =
            baseMinimum +
            slope *
                policy.m_slopeInstabilityM +
            convexHigh *
                policy.m_convexInstabilityM +
            protrusion *
                policy.m_protrusionInstabilityM -
            concavity *
                policy.m_concavityStabilityM -
            recessDepthM *
                policy.m_recessStabilityPerMeter -
            banking *
                policy.m_bankingStabilityM;

        return SoilMax(
            minimumStableThicknessM,
            policy.m_minimumRetainedPocketThicknessM );
    }

    //-------------------------------------------------------------------------
    // P3C.9C — retained-pocket classification
    //-------------------------------------------------------------------------

    static bool IsSoilRetentionPocket(
        SoilRetentionSample const& sample )
    {
        return SoilClamp01(
                   sample.m_concavityInfluence ) >=
                   0.52f ||
               sample.m_recessDepthM >=
                   0.065f ||
               SoilClamp01(
                   sample.m_obstacleBankingInfluence ) >=
                   0.52f;
    }

    //-------------------------------------------------------------------------
    // P3C.9C — deterministic connected-component labeling
    //
    // Four-neighbor connectivity is intentional for the certification grid.
    // Diagonal contact alone does not create a physical Soil bridge.
    //-------------------------------------------------------------------------

    static int32_t LabelSoilPatchComponents(
        SoilMantleDistributionGrid const& grid,
        SoilPatchCell*                    pCells,
        int32_t                           numCells )
    {
        EE_ASSERT(
            pCells !=
            nullptr );

        for ( int32_t i = 0;
              i < numCells;
              ++i )
        {
            pCells[i].m_patchID =
                0;
        }

        int32_t patchCount =
            0;

        for ( int32_t seedIndex = 0;
              seedIndex < numCells;
              ++seedIndex )
        {
            if ( !pCells[seedIndex].m_occupied ||
                 pCells[seedIndex].m_patchID !=
                     0 )
            {
                continue;
            }

            ++patchCount;

            pCells[seedIndex].m_patchID =
                patchCount;

            bool changed =
                true;

            // Deterministic flood-fill without dynamic allocation.
            // Repeated scans are acceptable here because this runs only on
            // geological rebuild/certification, never per frame.
            for ( int32_t propagation = 0;
                  propagation < numCells &&
                  changed;
                  ++propagation )
            {
                changed =
                    false;

                for ( int32_t y = 0;
                      y < grid.m_numCellsY;
                      ++y )
                {
                    for ( int32_t x = 0;
                          x < grid.m_numCellsX;
                          ++x )
                    {
                        int32_t const index =
                            y *
                                grid.m_numCellsX +
                            x;

                        SoilPatchCell& cell =
                            pCells[index];

                        if ( !cell.m_occupied ||
                             cell.m_patchID !=
                                 0 )
                        {
                            continue;
                        }

                        bool joinsPatch =
                            false;

                        if ( x > 0 )
                        {
                            int32_t const neighbor =
                                index -
                                1;

                            joinsPatch =
                                joinsPatch ||
                                pCells[neighbor].m_patchID ==
                                    patchCount;
                        }

                        if ( x + 1 <
                             grid.m_numCellsX )
                        {
                            int32_t const neighbor =
                                index +
                                1;

                            joinsPatch =
                                joinsPatch ||
                                pCells[neighbor].m_patchID ==
                                    patchCount;
                        }

                        if ( y > 0 )
                        {
                            int32_t const neighbor =
                                index -
                                grid.m_numCellsX;

                            joinsPatch =
                                joinsPatch ||
                                pCells[neighbor].m_patchID ==
                                    patchCount;
                        }

                        if ( y + 1 <
                             grid.m_numCellsY )
                        {
                            int32_t const neighbor =
                                index +
                                grid.m_numCellsX;

                            joinsPatch =
                                joinsPatch ||
                                pCells[neighbor].m_patchID ==
                                    patchCount;
                        }

                        if ( joinsPatch )
                        {
                            cell.m_patchID =
                                patchCount;

                            changed =
                                true;
                        }
                    }
                }
            }
        }

        return patchCount;
    }

    //-------------------------------------------------------------------------
    // P3C.9C — conserved breach / residual patch solver
    //-------------------------------------------------------------------------

    SoilMantleBreachResult ResolveConservedSoilBreach(
        SoilMantleAuthorityResult const&  authority,
        SoilMantleDistributionGrid const& grid,
        SoilRetentionSample const*        pRetentionSamples,
        int32_t                           numRetentionSamples,
        SoilRedistributionCell const*     pRedistributedCells,
        int32_t                           numRedistributedCells,
        SoilBreachPolicy const&           policy,
        SoilPatchCell*                    pOutPatchCells,
        int32_t                           numOutputCells )
    {
        SoilMantleBreachResult result;

        int32_t const numCells =
            grid.m_numCellsX *
            grid.m_numCellsY;

        result.m_numCells =
            numCells;

        result.m_cellAreaM2 =
            grid.m_cellSizeX *
            grid.m_cellSizeY;

        result.m_totalAreaM2 =
            result.m_cellAreaM2 *
            float(
                numCells );

        result.m_authoritativeBulkVolumeM3 =
            authority.m_bulkVolumeM3;

        result.m_authoritativeMassKg =
            authority.m_massKg;

        if ( !authority.m_conservationPass ||
             grid.m_numCellsX <=
                 0 ||
             grid.m_numCellsY <=
                 0 ||
             grid.m_cellSizeX <=
                 0.0f ||
             grid.m_cellSizeY <=
                 0.0f ||
             pRetentionSamples ==
                 nullptr ||
             pRedistributedCells ==
                 nullptr ||
             pOutPatchCells ==
                 nullptr ||
             numRetentionSamples !=
                 numCells ||
             numRedistributedCells !=
                 numCells ||
             numOutputCells !=
                 numCells ||
             numCells <=
                 0 ||
             result.m_cellAreaM2 <=
                 0.0f ||
             policy.m_maxIterations <=
                 0 )
        {
            return result;
        }

        //-------------------------------------------------------------------------
        // Initialize from the already-conserved P3C.9B continuous field.
        //-------------------------------------------------------------------------

        int32_t occupiedCount =
            0;

        for ( int32_t i = 0;
              i < numCells;
              ++i )
        {
            SoilRetentionSample const& retention =
                pRetentionSamples[i];

            SoilRedistributionCell const& redistributed =
                pRedistributedCells[i];

            SoilPatchCell& cell =
                pOutPatchCells[i];

            cell =
                SoilPatchCell();

            cell.m_worldX =
                redistributed.m_worldX;

            cell.m_worldY =
                redistributed.m_worldY;

            cell.m_projectedAreaM2 =
                result.m_cellAreaM2;

            cell.m_retentionWeight =
                SoilMax(
                    redistributed.m_retentionWeight,
                    s_soilMinimumPositiveValue );

            cell.m_substrateMaterial =
                retention.m_substrateMaterial;

            cell.m_minimumStableThicknessM =
                CalculateSoilMinimumStableThickness(
                    retention,
                    authority.m_body.m_physicalState,
                    policy );

            cell.m_thicknessZM =
                SoilMax(
                    redistributed.m_thicknessZM,
                    0.0f );

            cell.m_bulkVolumeM3 =
                cell.m_thicknessZM *
                result.m_cellAreaM2;

            cell.m_occupied =
                cell.m_thicknessZM >
                0.0f;

            cell.m_state =
                IsSoilRetentionPocket(
                    retention )
                    ? SoilCoverCellState::RetainedPocket
                    : SoilCoverCellState::OccupiedResidual;

            cell.m_stabilityMarginM =
                cell.m_thicknessZM -
                cell.m_minimumStableThicknessM;

            if ( cell.m_occupied )
            {
                ++occupiedCount;
            }
        }

        //-------------------------------------------------------------------------
        // Evacuate the least stable cell one at a time.
        //
        // After each evacuation, its Soil bulk volume is redistributed over
        // all surviving cells in proportion to their retention weights.
        //
        // This ordering matters:
        //
        //     weak cells can fail
        //         ->
        //     survivors become thicker
        //         ->
        //     some marginal cells may become stable
        //
        // Therefore coverage emerges from the matter solve instead of from an
        // authored target fraction.
        //-------------------------------------------------------------------------

        double evacuatedBulkVolume =
            0.0;

        double breachedPreEvacuationThicknessSum =
            0.0;

        int32_t breachedCount =
            0;

        int32_t iterationsUsed =
            0;

        for ( int32_t iteration = 0;
              iteration < policy.m_maxIterations;
              ++iteration )
        {
            int32_t mostUnstableIndex =
                -1;

            float mostUnstableMargin =
                0.0f;

            for ( int32_t i = 0;
                  i < numCells;
                  ++i )
            {
                SoilPatchCell& cell =
                    pOutPatchCells[i];

                if ( !cell.m_occupied )
                {
                    continue;
                }

                cell.m_stabilityMarginM =
                    cell.m_thicknessZM -
                    cell.m_minimumStableThicknessM;

                if ( cell.m_stabilityMarginM <
                     mostUnstableMargin )
                {
                    mostUnstableMargin =
                        cell.m_stabilityMarginM;

                    mostUnstableIndex =
                        i;
                }
            }

            if ( mostUnstableIndex <
                 0 )
            {
                break;
            }

            // Keep at least one cell available to receive conserved Soil.
            if ( occupiedCount <=
                 1 )
            {
                break;
            }

            SoilPatchCell& failed =
                pOutPatchCells[mostUnstableIndex];

            double const removedVolume =
                double(
                    failed.m_bulkVolumeM3 );

            evacuatedBulkVolume +=
                removedVolume;

            breachedPreEvacuationThicknessSum +=
                double(
                    failed.m_thicknessZM );

            ++breachedCount;

            failed.m_state =
                SoilCoverCellState::BreachedToSubstrate;

            failed.m_occupied =
                false;

            failed.m_thicknessZM =
                0.0f;

            failed.m_bulkVolumeM3 =
                0.0f;

            failed.m_stabilityMarginM =
                -failed.m_minimumStableThicknessM;

            failed.m_patchID =
                0;

            --occupiedCount;

            double survivingWeightSum =
                0.0;

            int32_t correctionIndex =
                -1;

            float strongestRetention =
                -1.0f;

            for ( int32_t i = 0;
                  i < numCells;
                  ++i )
            {
                SoilPatchCell const& cell =
                    pOutPatchCells[i];

                if ( !cell.m_occupied )
                {
                    continue;
                }

                survivingWeightSum +=
                    double(
                        cell.m_retentionWeight );

                if ( cell.m_retentionWeight >
                     strongestRetention )
                {
                    strongestRetention =
                        cell.m_retentionWeight;

                    correctionIndex =
                        i;
                }
            }

            if ( survivingWeightSum <=
                     0.0 ||
                 correctionIndex <
                     0 )
            {
                break;
            }

            double distributedVolume =
                0.0;

            for ( int32_t i = 0;
                  i < numCells;
                  ++i )
            {
                SoilPatchCell& cell =
                    pOutPatchCells[i];

                if ( !cell.m_occupied ||
                     i ==
                         correctionIndex )
                {
                    continue;
                }

                double const share =
                    removedVolume *
                    ( double(
                          cell.m_retentionWeight ) /
                      survivingWeightSum );

                cell.m_bulkVolumeM3 +=
                    float(
                        share );

                cell.m_thicknessZM =
                    cell.m_bulkVolumeM3 /
                    result.m_cellAreaM2;

                distributedVolume +=
                    share;
            }

            SoilPatchCell& correctionCell =
                pOutPatchCells[correctionIndex];

            double const correctionShare =
                removedVolume -
                distributedVolume;

            correctionCell.m_bulkVolumeM3 +=
                float(
                    correctionShare );

            correctionCell.m_thicknessZM =
                correctionCell.m_bulkVolumeM3 /
                result.m_cellAreaM2;

            iterationsUsed =
                iteration +
                1;
        }

        result.m_iterationsUsed =
            iterationsUsed;

        result.m_evacuatedBulkVolumeM3 =
            float(
                evacuatedBulkVolume );

        //-------------------------------------------------------------------------
        // Final conservation correction
        //
        // Per-cell storage is float. After several proportional transfers the
        // tiny roundoff residual is returned to the strongest surviving cell.
        // This is numerical closure only; it is not a physical source/sink.
        //-------------------------------------------------------------------------

        double preCorrectionVolume =
            0.0;

        int32_t strongestOccupiedIndex =
            -1;

        float strongestOccupiedRetention =
            -1.0f;

        for ( int32_t i = 0;
              i < numCells;
              ++i )
        {
            SoilPatchCell const& cell =
                pOutPatchCells[i];

            if ( cell.m_occupied )
            {
                preCorrectionVolume +=
                    double(
                        cell.m_bulkVolumeM3 );

                if ( cell.m_retentionWeight >
                     strongestOccupiedRetention )
                {
                    strongestOccupiedRetention =
                        cell.m_retentionWeight;

                    strongestOccupiedIndex =
                        i;
                }
            }
        }

        if ( strongestOccupiedIndex >=
             0 )
        {
            double const volumeCorrection =
                double(
                    authority.m_bulkVolumeM3 ) -
                preCorrectionVolume;

            SoilPatchCell& correctionCell =
                pOutPatchCells[strongestOccupiedIndex];

            correctionCell.m_bulkVolumeM3 =
                SoilMax(
                    correctionCell.m_bulkVolumeM3 +
                        float(
                            volumeCorrection ),
                    0.0f );

            correctionCell.m_thicknessZM =
                correctionCell.m_bulkVolumeM3 /
                result.m_cellAreaM2;
        }

        //-------------------------------------------------------------------------
        // Final classification and receipts
        //-------------------------------------------------------------------------

        double finalBulkVolume =
            0.0;

        double occupiedThicknessSum =
            0.0;

        double pocketThicknessSum =
            0.0;

        bool firstOccupiedThickness =
            true;

        bool allOccupiedCellsStable =
            true;

        for ( int32_t i = 0;
              i < numCells;
              ++i )
        {
            SoilRetentionSample const& retention =
                pRetentionSamples[i];

            SoilPatchCell& cell =
                pOutPatchCells[i];

            if ( !cell.m_occupied )
            {
                cell.m_state =
                    SoilCoverCellState::BreachedToSubstrate;

                cell.m_patchID =
                    0;

                continue;
            }

            cell.m_stabilityMarginM =
                cell.m_thicknessZM -
                cell.m_minimumStableThicknessM;

            if ( cell.m_stabilityMarginM <
                 -1.0e-6f )
            {
                allOccupiedCellsStable =
                    false;
            }

            bool const retainedPocket =
                IsSoilRetentionPocket(
                    retention );

            cell.m_state =
                retainedPocket
                    ? SoilCoverCellState::RetainedPocket
                    : SoilCoverCellState::OccupiedResidual;

            if ( retainedPocket )
            {
                ++result.m_numPocketCells;

                pocketThicknessSum +=
                    double(
                        cell.m_thicknessZM );
            }

            ++result.m_numOccupiedCells;

            finalBulkVolume +=
                double(
                    cell.m_bulkVolumeM3 );

            occupiedThicknessSum +=
                double(
                    cell.m_thicknessZM );

            if ( firstOccupiedThickness )
            {
                result.m_minOccupiedThicknessZM =
                    cell.m_thicknessZM;

                result.m_maxOccupiedThicknessZM =
                    cell.m_thicknessZM;

                firstOccupiedThickness =
                    false;
            }
            else
            {
                result.m_minOccupiedThicknessZM =
                    SoilMin(
                        result.m_minOccupiedThicknessZM,
                        cell.m_thicknessZM );

                result.m_maxOccupiedThicknessZM =
                    SoilMax(
                        result.m_maxOccupiedThicknessZM,
                        cell.m_thicknessZM );
            }
        }

        result.m_numBreachedCells =
            numCells -
            result.m_numOccupiedCells;

        result.m_numPatches =
            LabelSoilPatchComponents(
                grid,
                pOutPatchCells,
                numCells );

        result.m_occupiedAreaM2 =
            float(
                result.m_numOccupiedCells ) *
            result.m_cellAreaM2;

        result.m_breachedAreaM2 =
            float(
                result.m_numBreachedCells ) *
            result.m_cellAreaM2;

        if ( result.m_totalAreaM2 >
             0.0f )
        {
            result.m_coverageFraction =
                result.m_occupiedAreaM2 /
                result.m_totalAreaM2;

            result.m_breachFraction =
                result.m_breachedAreaM2 /
                result.m_totalAreaM2;
        }

        result.m_finalBulkVolumeM3 =
            float(
                finalBulkVolume );

        result.m_reconstructedMassKg =
            ReconstructSoilMassKg(
                result.m_finalBulkVolumeM3,
                authority.m_packingFraction,
                authority.m_particleDensityKgPerM3 );

        result.m_bulkVolumeResidualM3 =
            result.m_finalBulkVolumeM3 -
            result.m_authoritativeBulkVolumeM3;

        result.m_massResidualKg =
            result.m_reconstructedMassKg -
            result.m_authoritativeMassKg;

        float const bulkVolumeDenominator =
            SoilMax(
                SoilAbs(
                    result.m_authoritativeBulkVolumeM3 ),
                s_soilMinimumPositiveValue );

        float const massDenominator =
            SoilMax(
                SoilAbs(
                    result.m_authoritativeMassKg ),
                s_soilMinimumPositiveValue );

        result.m_bulkVolumeRelativeResidual =
            SoilAbs(
                result.m_bulkVolumeResidualM3 ) /
            bulkVolumeDenominator;

        result.m_massRelativeResidual =
            SoilAbs(
                result.m_massResidualKg ) /
            massDenominator;

        if ( result.m_numOccupiedCells >
             0 )
        {
            result.m_meanOccupiedThicknessZM =
                float(
                    occupiedThicknessSum /
                    double(
                        result.m_numOccupiedCells ) );
        }

        if ( breachedCount >
             0 )
        {
            result.m_meanBreachedPreEvacuationThicknessZM =
                float(
                    breachedPreEvacuationThicknessSum /
                    double(
                        breachedCount ) );
        }

        if ( result.m_numPocketCells >
             0 )
        {
            result.m_meanPocketThicknessZM =
                float(
                    pocketThicknessSum /
                    double(
                        result.m_numPocketCells ) );
        }

        float const permittedVolumeResidual =
            SoilMax(
                s_soilVolumeAbsoluteToleranceM3,
                SoilAbs(
                    result.m_authoritativeBulkVolumeM3 ) *
                    s_soilRelativeTolerance );

        float const permittedMassResidual =
            SoilMax(
                s_soilMassAbsoluteToleranceKg,
                SoilAbs(
                    result.m_authoritativeMassKg ) *
                    s_soilRelativeTolerance );

        result.m_conservationPass =
            SoilAbs(
                result.m_bulkVolumeResidualM3 ) <=
                permittedVolumeResidual &&
            SoilAbs(
                result.m_massResidualKg ) <=
                permittedMassResidual;

        bool topologyAssignmentsValid =
            true;

        for ( int32_t i = 0;
              i < numCells;
              ++i )
        {
            SoilPatchCell const& cell =
                pOutPatchCells[i];

            if ( cell.m_occupied )
            {
                if ( cell.m_patchID <=
                     0 )
                {
                    topologyAssignmentsValid =
                        false;

                    break;
                }
            }
            else if ( cell.m_patchID !=
                      0 )
            {
                topologyAssignmentsValid =
                    false;

                break;
            }
        }

        float const partitionResidual =
            SoilAbs(
                result.m_coverageFraction +
                result.m_breachFraction -
                1.0f );

        result.m_patchTopologyPass =
            topologyAssignmentsValid &&
            allOccupiedCellsStable &&
            result.m_numOccupiedCells >
                0 &&
            result.m_numPatches >
                0 &&
            partitionResidual <=
                1.0e-5f;

        result.m_valid =
            result.m_conservationPass &&
            result.m_patchTopologyPass;

        EE_ASSERT(
            result.m_conservationPass );

        EE_ASSERT(
            result.m_patchTopologyPass );

        return result;
    }

    //-------------------------------------------------------------------------
    // P3C.9C-4 — visible Soil geometry helpers
    //-------------------------------------------------------------------------

    static float GetSoilVisibleInternalMobility(
        SoilPhysicalState                physicalState,
        SoilVisibleGeometryPolicy const& policy )
    {
        switch ( physicalState )
        {
            case SoilPhysicalState::Loose:
            {
                return SoilClamp01(
                    policy.m_looseInternalMobility );
            }

            case SoilPhysicalState::Settled:
            {
                return SoilClamp01(
                    policy.m_settledInternalMobility );
            }

            case SoilPhysicalState::Compacted:
            {
                return SoilClamp01(
                    policy.m_compactedInternalMobility );
            }

            default:
            {
                EE_ASSERT( false );
                return SoilClamp01(
                    policy.m_settledInternalMobility );
            }
        }
    }

    //-------------------------------------------------------------------------

    static float GetSoilVisibleMaximumSlope(
        SoilPhysicalState                physicalState,
        SoilVisibleGeometryPolicy const& policy )
    {
        switch ( physicalState )
        {
            case SoilPhysicalState::Loose:
            {
                return SoilMax(
                    policy.m_looseMaximumSurfaceSlope,
                    0.0f );
            }

            case SoilPhysicalState::Settled:
            {
                return SoilMax(
                    policy.m_settledMaximumSurfaceSlope,
                    0.0f );
            }

            case SoilPhysicalState::Compacted:
            {
                return SoilMax(
                    policy.m_compactedMaximumSurfaceSlope,
                    0.0f );
            }

            default:
            {
                EE_ASSERT( false );
                return SoilMax(
                    policy.m_settledMaximumSurfaceSlope,
                    0.0f );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Fine visible allocation priority
    //
    // This remains a preference only. Parent-cell bulk volume is applied
    // later and independently normalized back to exact conservation.
    //-------------------------------------------------------------------------

    float CalculateSoilVisibleAllocationWeight(
        SoilRetentionSample const&       sample,
        SoilPhysicalState                physicalState,
        SoilCoverCellState               parentCoverState,
        SoilVisibleGeometryPolicy const& policy )
    {
        float const mobility =
            GetSoilVisibleInternalMobility(
                physicalState,
                policy );

        float const slope =
            SoilClamp01(
                sample.m_slope );

        float const convexHigh =
            SoilClamp01(
                sample.m_convexHighInfluence );

        float const concavity =
            SoilClamp01(
                sample.m_concavityInfluence );

        float const banking =
            SoilClamp01(
                sample.m_obstacleBankingInfluence );

        float const protrusion =
            SoilClamp01(
                sample.m_protrusionInfluence );

        float const recessInfluence =
            SoilClamp01(
                SoilMax(
                    sample.m_recessDepthM,
                    0.0f ) /
                0.40f );

        float pocketBias =
            0.0f;

        if ( parentCoverState ==
             SoilCoverCellState::RetainedPocket )
        {
            pocketBias =
                policy.m_pocketCenterBias *
                ( concavity * 0.42f +
                  recessInfluence * 0.38f +
                  banking * 0.20f );
        }

        float const positiveRetention =
            concavity * 0.62f +
            recessInfluence * 0.82f +
            banking * 0.56f +
            pocketBias;

        float const shedding =
            slope * 0.30f +
            convexHigh * 0.70f +
            protrusion * 0.62f;

        float weight =
            1.0f +
            mobility *
                ( positiveRetention -
                  shedding );

        // A fine cell may later taper to zero at a certified patch edge, but
        // the geometry response itself remains finite and positive.
        return SoilMax(
            weight,
            0.04f );
    }

    //-------------------------------------------------------------------------
    // Parent/fine-grid index helpers
    //-------------------------------------------------------------------------

    static int32_t SoilParentIndexFromFineCell(
        SoilMantleDistributionGrid const& parentGrid,
        SoilGeometryGrid const&           geometryGrid,
        int32_t                           fineX,
        int32_t                           fineY )
    {
        int32_t const parentX =
            fineX /
            geometryGrid.m_subcellsPerParentX;

        int32_t const parentY =
            fineY /
            geometryGrid.m_subcellsPerParentY;

        if ( parentX < 0 ||
             parentX >=
                 parentGrid.m_numCellsX ||
             parentY < 0 ||
             parentY >=
                 parentGrid.m_numCellsY )
        {
            return -1;
        }

        return parentY *
                   parentGrid.m_numCellsX +
               parentX;
    }

    //-------------------------------------------------------------------------

    static bool SoilParentNeighborIsBreached(
        SoilMantleDistributionGrid const& parentGrid,
        SoilPatchCell const*              pParentPatchCells,
        int32_t                           parentX,
        int32_t                           parentY )
    {
        if ( parentX < 0 ||
             parentX >=
                 parentGrid.m_numCellsX ||
             parentY < 0 ||
             parentY >=
                 parentGrid.m_numCellsY )
        {
            return true;
        }

        int32_t const index =
            parentY *
                parentGrid.m_numCellsX +
            parentX;

        return !pParentPatchCells[index].m_occupied;
    }

    //-------------------------------------------------------------------------
    // Edge taper factor
    //
    // Only certified occupied parents neighboring a certified breached parent
    // receive this taper. Shared edges between occupied parents of the same
    // patch remain open so parent connectivity is not destroyed.
    //
    // Retention geometry perturbs the effective distance slightly: a strong
    // recess or protected base can extend Soil closer to the breach edge,
    // while a weak convex edge retreats farther. This avoids a visible square
    // outline without introducing arbitrary screen-space noise.
    //-------------------------------------------------------------------------

    static float CalculateSoilFineEdgeTaper(
        SoilMantleDistributionGrid const& parentGrid,
        SoilGeometryGrid const&           geometryGrid,
        SoilPatchCell const*              pParentPatchCells,
        int32_t                           parentX,
        int32_t                           parentY,
        int32_t                           localFineX,
        int32_t                           localFineY,
        float                             allocationWeight,
        SoilVisibleGeometryPolicy const&  policy )
    {
        float const taperWidth =
            SoilMax(
                policy.m_residualEdgeTaperWidthM,
                SoilMin(
                    geometryGrid.m_cellSizeX,
                    geometryGrid.m_cellSizeY ) );

        bool const breachLeft =
            SoilParentNeighborIsBreached(
                parentGrid,
                pParentPatchCells,
                parentX - 1,
                parentY );

        bool const breachRight =
            SoilParentNeighborIsBreached(
                parentGrid,
                pParentPatchCells,
                parentX + 1,
                parentY );

        bool const breachBottom =
            SoilParentNeighborIsBreached(
                parentGrid,
                pParentPatchCells,
                parentX,
                parentY - 1 );

        bool const breachTop =
            SoilParentNeighborIsBreached(
                parentGrid,
                pParentPatchCells,
                parentX,
                parentY + 1 );

        float taper =
            1.0f;

        float const retentionShiftX =
            ( allocationWeight -
              1.0f ) *
            geometryGrid.m_cellSizeX *
            0.42f;

        float const retentionShiftY =
            ( allocationWeight -
              1.0f ) *
            geometryGrid.m_cellSizeY *
            0.42f;

        if ( breachLeft )
        {
            float const distanceM =
                ( float(
                      localFineX ) +
                  0.5f ) *
                    geometryGrid.m_cellSizeX -
                0.5f *
                    geometryGrid.m_cellSizeX +
                retentionShiftX;

            taper =
                SoilMin(
                    taper,
                    SoilClamp01(
                        distanceM /
                        taperWidth ) );
        }

        if ( breachRight )
        {
            float const cellsFromEdge =
                float(
                    geometryGrid.m_subcellsPerParentX -
                    1 -
                    localFineX );

            float const distanceM =
                ( cellsFromEdge +
                  0.5f ) *
                    geometryGrid.m_cellSizeX -
                0.5f *
                    geometryGrid.m_cellSizeX +
                retentionShiftX;

            taper =
                SoilMin(
                    taper,
                    SoilClamp01(
                        distanceM /
                        taperWidth ) );
        }

        if ( breachBottom )
        {
            float const distanceM =
                ( float(
                      localFineY ) +
                  0.5f ) *
                    geometryGrid.m_cellSizeY -
                0.5f *
                    geometryGrid.m_cellSizeY +
                retentionShiftY;

            taper =
                SoilMin(
                    taper,
                    SoilClamp01(
                        distanceM /
                        taperWidth ) );
        }

        if ( breachTop )
        {
            float const cellsFromEdge =
                float(
                    geometryGrid.m_subcellsPerParentY -
                    1 -
                    localFineY );

            float const distanceM =
                ( cellsFromEdge +
                  0.5f ) *
                    geometryGrid.m_cellSizeY -
                0.5f *
                    geometryGrid.m_cellSizeY +
                retentionShiftY;

            taper =
                SoilMin(
                    taper,
                    SoilClamp01(
                        distanceM /
                        taperWidth ) );
        }

        return SoilClamp01(
            taper );
    }

    //-------------------------------------------------------------------------
    // Fine-cell slope relaxation inside one parent
    //
    // Transfers thickness only between already-occupied fine cells, preserving
    // that parent's Soil bulk volume exactly apart from float roundoff.
    //-------------------------------------------------------------------------

    static void RelaxSoilFineSurfaceSlopeWithinParent(
        SoilGeometryGrid const&          geometryGrid,
        int32_t                          parentX,
        int32_t                          parentY,
        SoilPhysicalState                physicalState,
        SoilVisibleGeometryPolicy const& policy,
        SoilGeometrySubcell*             pSubcells )
    {
        float const maxSlope =
            GetSoilVisibleMaximumSlope(
                physicalState,
                policy );

        if ( maxSlope <=
             0.0f )
        {
            return;
        }

        int32_t const startX =
            parentX *
            geometryGrid.m_subcellsPerParentX;

        int32_t const startY =
            parentY *
            geometryGrid.m_subcellsPerParentY;

        for ( int32_t relaxationPass = 0;
              relaxationPass < 4;
              ++relaxationPass )
        {
            for ( int32_t localY = 0;
                  localY <
                  geometryGrid.m_subcellsPerParentY;
                  ++localY )
            {
                for ( int32_t localX = 0;
                      localX <
                      geometryGrid.m_subcellsPerParentX;
                      ++localX )
                {
                    int32_t const fineX =
                        startX +
                        localX;

                    int32_t const fineY =
                        startY +
                        localY;

                    int32_t const index =
                        fineY *
                            geometryGrid.m_numCellsX +
                        fineX;

                    SoilGeometrySubcell& a =
                        pSubcells[index];

                    if ( !a.m_occupied )
                    {
                        continue;
                    }

                    if ( localX + 1 <
                         geometryGrid.m_subcellsPerParentX )
                    {
                        int32_t const neighborIndex =
                            index +
                            1;

                        SoilGeometrySubcell& b =
                            pSubcells[neighborIndex];

                        if ( b.m_occupied )
                        {
                            float const allowedDifference =
                                maxSlope *
                                geometryGrid.m_cellSizeX;

                            float const difference =
                                a.m_thicknessZM -
                                b.m_thicknessZM;

                            if ( SoilAbs(
                                     difference ) >
                                 allowedDifference )
                            {
                                float const transferThickness =
                                    ( SoilAbs(
                                          difference ) -
                                      allowedDifference ) *
                                    0.5f;

                                float const transferVolume =
                                    transferThickness *
                                    a.m_projectedAreaM2;

                                if ( difference >
                                     0.0f )
                                {
                                    a.m_bulkVolumeM3 -=
                                        transferVolume;

                                    b.m_bulkVolumeM3 +=
                                        transferVolume;
                                }
                                else
                                {
                                    b.m_bulkVolumeM3 -=
                                        transferVolume;

                                    a.m_bulkVolumeM3 +=
                                        transferVolume;
                                }

                                a.m_thicknessZM =
                                    a.m_bulkVolumeM3 /
                                    a.m_projectedAreaM2;

                                b.m_thicknessZM =
                                    b.m_bulkVolumeM3 /
                                    b.m_projectedAreaM2;
                            }
                        }
                    }

                    if ( localY + 1 <
                         geometryGrid.m_subcellsPerParentY )
                    {
                        int32_t const neighborIndex =
                            index +
                            geometryGrid.m_numCellsX;

                        SoilGeometrySubcell& b =
                            pSubcells[neighborIndex];

                        if ( b.m_occupied )
                        {
                            float const allowedDifference =
                                maxSlope *
                                geometryGrid.m_cellSizeY;

                            float const difference =
                                a.m_thicknessZM -
                                b.m_thicknessZM;

                            if ( SoilAbs(
                                     difference ) >
                                 allowedDifference )
                            {
                                float const transferThickness =
                                    ( SoilAbs(
                                          difference ) -
                                      allowedDifference ) *
                                    0.5f;

                                float const transferVolume =
                                    transferThickness *
                                    a.m_projectedAreaM2;

                                if ( difference >
                                     0.0f )
                                {
                                    a.m_bulkVolumeM3 -=
                                        transferVolume;

                                    b.m_bulkVolumeM3 +=
                                        transferVolume;
                                }
                                else
                                {
                                    b.m_bulkVolumeM3 -=
                                        transferVolume;

                                    a.m_bulkVolumeM3 +=
                                        transferVolume;
                                }

                                a.m_thicknessZM =
                                    a.m_bulkVolumeM3 /
                                    a.m_projectedAreaM2;

                                b.m_thicknessZM =
                                    b.m_bulkVolumeM3 /
                                    b.m_projectedAreaM2;
                            }
                        }
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    // Visible conserved Soil geometry
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // P3C.9C-4B — accounting-grid invisibility helpers
    //-------------------------------------------------------------------------

    static bool SoilParentNeighborSharesPatch(
        SoilMantleDistributionGrid const& parentGrid,
        SoilPatchCell const*              pParentPatchCells,
        int32_t                           parentX,
        int32_t                           parentY,
        int32_t                           patchID )
    {
        if ( parentX < 0 ||
             parentX >=
                 parentGrid.m_numCellsX ||
             parentY < 0 ||
             parentY >=
                 parentGrid.m_numCellsY )
        {
            return false;
        }

        int32_t const index =
            parentY *
                parentGrid.m_numCellsX +
            parentX;

        SoilPatchCell const& neighbor =
            pParentPatchCells[index];

        return neighbor.m_occupied &&
               neighbor.m_patchID ==
                   patchID;
    }

    //-------------------------------------------------------------------------

    static bool SoilParentNeighborIsExplicitBreach(
        SoilMantleDistributionGrid const& parentGrid,
        SoilPatchCell const*              pParentPatchCells,
        int32_t                           parentX,
        int32_t                           parentY )
    {
        // The certification-domain edge is NOT a Soil/geology boundary.
        if ( parentX < 0 ||
             parentX >=
                 parentGrid.m_numCellsX ||
             parentY < 0 ||
             parentY >=
                 parentGrid.m_numCellsY )
        {
            return false;
        }

        int32_t const index =
            parentY *
                parentGrid.m_numCellsX +
            parentX;

        return !pParentPatchCells[index].m_occupied;
    }

    //-------------------------------------------------------------------------

    static float CalculateSoilFineBreachEdgeExposure(
        SoilMantleDistributionGrid const& parentGrid,
        SoilGeometryGrid const&           geometryGrid,
        SoilPatchCell const*              pParentPatchCells,
        int32_t                           parentX,
        int32_t                           parentY,
        int32_t                           localFineX,
        int32_t                           localFineY,
        SoilVisibleGeometryPolicy const&  policy )
    {
        float const taperWidthM =
            SoilMax(
                policy.m_residualEdgeTaperWidthM,
                0.50f );

        float exposure =
            0.0f;

        auto accumulateExposure =
            [&]( bool  breached,
                 float distanceM )
        {
            if ( !breached )
            {
                return;
            }

            float const localExposure =
                1.0f -
                SoilClamp01(
                    distanceM /
                    taperWidthM );

            exposure =
                SoilMax(
                    exposure,
                    localExposure );
        };

        float const distanceLeftM =
            ( float(
                  localFineX ) +
              0.5f ) *
            geometryGrid.m_cellSizeX;

        float const distanceRightM =
            ( float(
                  geometryGrid.m_subcellsPerParentX -
                  localFineX ) -
              0.5f ) *
            geometryGrid.m_cellSizeX;

        float const distanceBottomM =
            ( float(
                  localFineY ) +
              0.5f ) *
            geometryGrid.m_cellSizeY;

        float const distanceTopM =
            ( float(
                  geometryGrid.m_subcellsPerParentY -
                  localFineY ) -
              0.5f ) *
            geometryGrid.m_cellSizeY;

        accumulateExposure(
            SoilParentNeighborIsExplicitBreach(
                parentGrid,
                pParentPatchCells,
                parentX - 1,
                parentY ),
            distanceLeftM );

        accumulateExposure(
            SoilParentNeighborIsExplicitBreach(
                parentGrid,
                pParentPatchCells,
                parentX + 1,
                parentY ),
            distanceRightM );

        accumulateExposure(
            SoilParentNeighborIsExplicitBreach(
                parentGrid,
                pParentPatchCells,
                parentX,
                parentY - 1 ),
            distanceBottomM );

        accumulateExposure(
            SoilParentNeighborIsExplicitBreach(
                parentGrid,
                pParentPatchCells,
                parentX,
                parentY + 1 ),
            distanceTopM );

        return SoilClamp01(
            exposure );
    }

    //-------------------------------------------------------------------------

    static float CalculateSoilFinePatchBridgeBonus(
        SoilMantleDistributionGrid const& parentGrid,
        SoilGeometryGrid const&           geometryGrid,
        SoilPatchCell const*              pParentPatchCells,
        int32_t                           parentX,
        int32_t                           parentY,
        int32_t                           parentPatchID,
        int32_t                           localFineX,
        int32_t                           localFineY )
    {
        float bonus =
            0.0f;

        float const centerX =
            ( float(
                  geometryGrid.m_subcellsPerParentX ) -
              1.0f ) *
            0.5f;

        float const centerY =
            ( float(
                  geometryGrid.m_subcellsPerParentY ) -
              1.0f ) *
            0.5f;

        float const safeHalfX =
            SoilMax(
                centerX,
                1.0f );

        float const safeHalfY =
            SoilMax(
                centerY,
                1.0f );

        float const verticalBridgeProfile =
            1.0f -
            SoilClamp01(
                SoilAbs(
                    float(
                        localFineY ) -
                    centerY ) /
                safeHalfY );

        float const horizontalBridgeProfile =
            1.0f -
            SoilClamp01(
                SoilAbs(
                    float(
                        localFineX ) -
                    centerX ) /
                safeHalfX );

        if ( localFineX <= 1 &&
             SoilParentNeighborSharesPatch(
                 parentGrid,
                 pParentPatchCells,
                 parentX - 1,
                 parentY,
                 parentPatchID ) )
        {
            bonus +=
                0.52f *
                verticalBridgeProfile;
        }

        if ( localFineX >=
                 geometryGrid.m_subcellsPerParentX -
                     2 &&
             SoilParentNeighborSharesPatch(
                 parentGrid,
                 pParentPatchCells,
                 parentX + 1,
                 parentY,
                 parentPatchID ) )
        {
            bonus +=
                0.52f *
                verticalBridgeProfile;
        }

        if ( localFineY <= 1 &&
             SoilParentNeighborSharesPatch(
                 parentGrid,
                 pParentPatchCells,
                 parentX,
                 parentY - 1,
                 parentPatchID ) )
        {
            bonus +=
                0.52f *
                horizontalBridgeProfile;
        }

        if ( localFineY >=
                 geometryGrid.m_subcellsPerParentY -
                     2 &&
             SoilParentNeighborSharesPatch(
                 parentGrid,
                 pParentPatchCells,
                 parentX,
                 parentY + 1,
                 parentPatchID ) )
        {
            bonus +=
                0.52f *
                horizontalBridgeProfile;
        }

        return bonus;
    }

    //-------------------------------------------------------------------------

    static float CalculateSoilFineMinimumStableThickness(
        SoilRetentionSample const& retention,
        SoilPhysicalState          physicalState,
        SoilCoverCellState         parentState,
        float                      breachEdgeExposure )
    {
        SoilBreachPolicy finePolicy;

        float minimumThickness =
            CalculateSoilMinimumStableThickness(
                retention,
                physicalState,
                finePolicy );

        // Exposed residual edges have less lateral support than the middle of
        // the same parent accounting cell. This is a physical edge-stability
        // term, not a visual erosion mask.
        minimumThickness +=
            breachEdgeExposure *
            0.018f;

        if ( parentState ==
             SoilCoverCellState::RetainedPocket )
        {
            // Recess/banking geometry already made the parent a retained
            // pocket. Permit thinner local fingers to survive inside it.
            minimumThickness *=
                0.82f;
        }

        return SoilMax(
            minimumThickness,
            0.0035f );
    }

    //-------------------------------------------------------------------------
    // P3C.9C-4B — visible conserved Soil geometry
    //
    // Important correction from 9C-4A:
    //
    // Parent 2 m cells now own VOLUME only. Fine 0.25 m cells independently
    // solve local cover stability. A surviving parent may therefore contain
    // explicit substrate windows and a frayed/lobed Soil footprint while its
    // total fine-cell Soil volume still closes exactly to that parent.
    //-------------------------------------------------------------------------

    SoilVisibleGeometryResult BuildVisibleConservedSoilGeometry(
        SoilMantleAuthorityResult const&  authority,
        SoilMantleDistributionGrid const& parentGrid,
        SoilPatchCell const*              pParentPatchCells,
        int32_t                           numParentPatchCells,
        SoilGeometryGrid const&           geometryGrid,
        SoilRetentionSample const*        pFineRetentionSamples,
        int32_t                           numFineRetentionSamples,
        SoilVisibleGeometryPolicy const&  policy,
        SoilGeometrySubcell*              pOutSubcells,
        int32_t                           numOutputSubcells,
        SoilParentGeometryReceipt*        pParentReceipts,
        int32_t                           numParentReceipts,
        SoilClodDescriptor*               pOutClods,
        int32_t                           maxOutputClods )
    {
        SoilVisibleGeometryResult result;

        int32_t const numParentCells =
            parentGrid.m_numCellsX *
            parentGrid.m_numCellsY;

        int32_t const numFineCells =
            geometryGrid.m_numCellsX *
            geometryGrid.m_numCellsY;

        int32_t const fineCellsPerParent =
            geometryGrid.m_subcellsPerParentX *
            geometryGrid.m_subcellsPerParentY;

        result.m_numParentCells =
            numParentCells;

        result.m_numFineCells =
            numFineCells;

        if ( !authority.m_conservationPass ||
             pParentPatchCells ==
                 nullptr ||
             pFineRetentionSamples ==
                 nullptr ||
             pOutSubcells ==
                 nullptr ||
             pParentReceipts ==
                 nullptr ||
             numParentCells <=
                 0 ||
             numFineCells <=
                 0 ||
             fineCellsPerParent <=
                 0 ||
             numParentPatchCells !=
                 numParentCells ||
             numParentReceipts !=
                 numParentCells ||
             numFineRetentionSamples !=
                 numFineCells ||
             numOutputSubcells !=
                 numFineCells ||
             parentGrid.m_numCellsX <=
                 0 ||
             parentGrid.m_numCellsY <=
                 0 ||
             geometryGrid.m_numCellsX <=
                 0 ||
             geometryGrid.m_numCellsY <=
                 0 ||
             geometryGrid.m_cellSizeX <=
                 0.0f ||
             geometryGrid.m_cellSizeY <=
                 0.0f ||
             geometryGrid.m_subcellsPerParentX <=
                 0 ||
             geometryGrid.m_subcellsPerParentY <=
                 0 ||
             geometryGrid.m_numCellsX !=
                 parentGrid.m_numCellsX *
                     geometryGrid.m_subcellsPerParentX ||
             geometryGrid.m_numCellsY !=
                 parentGrid.m_numCellsY *
                     geometryGrid.m_subcellsPerParentY )
        {
            return result;
        }

        float const fineCellAreaM2 =
            geometryGrid.m_cellSizeX *
            geometryGrid.m_cellSizeY;

        if ( fineCellAreaM2 <=
             0.0f )
        {
            return result;
        }

        if ( policy.m_maxClodBulkVolumeFraction >
                 0.0f &&
             ( pOutClods ==
                   nullptr ||
               maxOutputClods <=
                   0 ) )
        {
            return result;
        }

        for ( int32_t i = 0;
              i < numFineCells;
              ++i )
        {
            pOutSubcells[i] =
                SoilGeometrySubcell();
        }

        for ( int32_t i = 0;
              i < numParentCells;
              ++i )
        {
            pParentReceipts[i] =
                SoilParentGeometryReceipt();

            pParentReceipts[i].m_parentCellIndex =
                i;

            pParentReceipts[i].m_parentPatchID =
                pParentPatchCells[i].m_patchID;

            pParentReceipts[i].m_parentBulkVolumeM3 =
                pParentPatchCells[i].m_bulkVolumeM3;
        }

        if ( pOutClods !=
             nullptr )
        {
            for ( int32_t i = 0;
                  i < maxOutputClods;
                  ++i )
            {
                pOutClods[i] =
                    SoilClodDescriptor();
            }
        }

        double totalParentBulkVolume =
            0.0;

        //-------------------------------------------------------------------------
        // Solve every parent independently for exact volume, while fine-cell
        // stability/bridge terms remove the visible parent-grid silhouette.
        //-------------------------------------------------------------------------

        for ( int32_t parentIndex = 0;
              parentIndex < numParentCells;
              ++parentIndex )
        {
            SoilPatchCell const& parent =
                pParentPatchCells[parentIndex];

            SoilParentGeometryReceipt& receipt =
                pParentReceipts[parentIndex];

            receipt.m_numSubcells =
                fineCellsPerParent;

            int32_t const parentX =
                parentIndex %
                parentGrid.m_numCellsX;

            int32_t const parentY =
                parentIndex /
                parentGrid.m_numCellsX;

            int32_t const startFineX =
                parentX *
                geometryGrid.m_subcellsPerParentX;

            int32_t const startFineY =
                parentY *
                geometryGrid.m_subcellsPerParentY;

            // Populate generic substrate metadata for every fine cell first.
            for ( int32_t localY = 0;
                  localY <
                  geometryGrid.m_subcellsPerParentY;
                  ++localY )
            {
                for ( int32_t localX = 0;
                      localX <
                      geometryGrid.m_subcellsPerParentX;
                      ++localX )
                {
                    int32_t const fineX =
                        startFineX +
                        localX;

                    int32_t const fineY =
                        startFineY +
                        localY;

                    int32_t const fineIndex =
                        fineY *
                            geometryGrid.m_numCellsX +
                        fineX;

                    SoilRetentionSample const& retention =
                        pFineRetentionSamples[fineIndex];

                    SoilGeometrySubcell& subcell =
                        pOutSubcells[fineIndex];

                    subcell.m_worldX =
                        retention.m_worldX;

                    subcell.m_worldY =
                        retention.m_worldY;

                    subcell.m_projectedAreaM2 =
                        fineCellAreaM2;

                    subcell.m_parentCellIndex =
                        parentIndex;

                    subcell.m_parentPatchID =
                        parent.m_occupied
                            ? parent.m_patchID
                            : 0;

                    subcell.m_substrateMaterial =
                        retention.m_substrateMaterial;

                    subcell.m_substrateSurfaceZM =
                        retention.m_substrateSurfaceZM;

                    subcell.m_soilSurfaceZM =
                        retention.m_substrateSurfaceZM;

                    subcell.m_contactField =
                        -SoilMax(
                            policy.m_minimumVisibleThicknessM,
                            0.001f );
                }
            }

            if ( !parent.m_occupied ||
                 parent.m_bulkVolumeM3 <=
                     0.0f )
            {
                receipt.m_valid =
                    true;

                receipt.m_conservationPass =
                    SoilAbs(
                        parent.m_bulkVolumeM3 ) <=
                    s_soilVolumeAbsoluteToleranceM3;

                receipt.m_topologyPass =
                    true;

                continue;
            }

            ++result.m_numOccupiedParentCells;

            totalParentBulkVolume +=
                double(
                    parent.m_bulkVolumeM3 );

            //-------------------------------------------------------------------------
            // Optional real clod reservation remains volume-backed.
            //-------------------------------------------------------------------------
            float clodBulkVolumeM3 =
                0.0f;

            if ( policy.m_maxClodBulkVolumeFraction >
                     0.0f &&
                 pOutClods !=
                     nullptr &&
                 result.m_numClods <
                     maxOutputClods &&
                 authority.m_body.m_physicalState ==
                     SoilPhysicalState::Loose &&
                 parent.m_state ==
                     SoilCoverCellState::OccupiedResidual )
            {
                uint32_t const mixed =
                    uint32_t(
                        parentIndex +
                        1 ) *
                        1664525u ^
                    authority.m_body.m_seed *
                        1013904223u;

                float const deterministicSignal =
                    float(
                        mixed &
                        0x0000FFFFu ) /
                    float(
                        0x0000FFFFu );

                float const clodFraction =
                    SoilClamp01(
                        policy.m_maxClodBulkVolumeFraction ) *
                    ( 0.28f +
                      deterministicSignal *
                          0.42f );

                clodBulkVolumeM3 =
                    parent.m_bulkVolumeM3 *
                    clodFraction;

                SoilClodDescriptor& clod =
                    pOutClods[result.m_numClods];

                clod.m_valid =
                    clodBulkVolumeM3 >
                    0.0f;

                clod.m_eventID =
                    mixed ^
                    0x9E3779B9u;

                clod.m_parentCellIndex =
                    parentIndex;

                clod.m_parentPatchID =
                    parent.m_patchID;

                clod.m_bulkVolumeM3 =
                    clodBulkVolumeM3;

                clod.m_physicalState =
                    authority.m_body.m_physicalState;

                clod.m_radiusM =
                    SoilMax(
                        geometryGrid.m_cellSizeX,
                        geometryGrid.m_cellSizeY ) *
                    ( 0.45f +
                      deterministicSignal *
                          0.35f );

                float const footprintArea =
                    SoilMax(
                        clod.m_radiusM *
                            clod.m_radiusM *
                            2.0f,
                        s_soilMinimumPositiveValue );

                clod.m_heightM =
                    clodBulkVolumeM3 /
                    footprintArea;

                ++result.m_numClods;
            }

            float const mantleBulkVolumeM3 =
                SoilMax(
                    parent.m_bulkVolumeM3 -
                        clodBulkVolumeM3,
                    0.0f );

            //-------------------------------------------------------------------------
            // Initialize all fine cells as candidates.
            //
            // Neighboring parents in the SAME certified patch receive a bridge
            // preference across their accounting edge. Neighbors that breached
            // receive an exposed-edge stability penalty. Neither term depends
            // on package/chunk boundaries.
            //-------------------------------------------------------------------------
            int32_t activeCount =
                0;

            for ( int32_t localY = 0;
                  localY <
                  geometryGrid.m_subcellsPerParentY;
                  ++localY )
            {
                for ( int32_t localX = 0;
                      localX <
                      geometryGrid.m_subcellsPerParentX;
                      ++localX )
                {
                    int32_t const fineX =
                        startFineX +
                        localX;

                    int32_t const fineY =
                        startFineY +
                        localY;

                    int32_t const fineIndex =
                        fineY *
                            geometryGrid.m_numCellsX +
                        fineX;

                    SoilRetentionSample const& retention =
                        pFineRetentionSamples[fineIndex];

                    SoilGeometrySubcell& subcell =
                        pOutSubcells[fineIndex];

                    float const rawWeight =
                        CalculateSoilVisibleAllocationWeight(
                            retention,
                            authority.m_body.m_physicalState,
                            parent.m_state,
                            policy );

                    float const bridgeBonus =
                        CalculateSoilFinePatchBridgeBonus(
                            parentGrid,
                            geometryGrid,
                            pParentPatchCells,
                            parentX,
                            parentY,
                            parent.m_patchID,
                            localX,
                            localY );

                    subcell.m_allocationWeight =
                        SoilMax(
                            rawWeight +
                                bridgeBonus,
                            0.04f );

                    subcell.m_occupied =
                        true;

                    ++activeCount;
                }
            }

            //-------------------------------------------------------------------------
            // Fine-scale conserved stability solve.
            //
            // Start from all candidate subcells. Repeatedly remove only the
            // least-stable fine cell, then reallocate the SAME parent volume
            // across the remaining cells. This mirrors the certified P3C.9C
            // breach law at finer spatial resolution.
            //-------------------------------------------------------------------------
            for ( int32_t iteration = 0;
                  iteration <
                  fineCellsPerParent -
                      1;
                  ++iteration )
            {
                double weightSum =
                    0.0;

                for ( int32_t localY = 0;
                      localY <
                      geometryGrid.m_subcellsPerParentY;
                      ++localY )
                {
                    for ( int32_t localX = 0;
                          localX <
                          geometryGrid.m_subcellsPerParentX;
                          ++localX )
                    {
                        int32_t const fineX =
                            startFineX +
                            localX;

                        int32_t const fineY =
                            startFineY +
                            localY;

                        int32_t const fineIndex =
                            fineY *
                                geometryGrid.m_numCellsX +
                            fineX;

                        SoilGeometrySubcell const& subcell =
                            pOutSubcells[fineIndex];

                        if ( subcell.m_occupied )
                        {
                            weightSum +=
                                double(
                                    subcell.m_allocationWeight );
                        }
                    }
                }

                if ( weightSum <=
                         0.0 ||
                     activeCount <=
                         1 )
                {
                    break;
                }

                int32_t weakestFineIndex =
                    -1;

                float weakestMarginM =
                    0.0f;

                for ( int32_t localY = 0;
                      localY <
                      geometryGrid.m_subcellsPerParentY;
                      ++localY )
                {
                    for ( int32_t localX = 0;
                          localX <
                          geometryGrid.m_subcellsPerParentX;
                          ++localX )
                    {
                        int32_t const fineX =
                            startFineX +
                            localX;

                        int32_t const fineY =
                            startFineY +
                            localY;

                        int32_t const fineIndex =
                            fineY *
                                geometryGrid.m_numCellsX +
                            fineX;

                        SoilGeometrySubcell& subcell =
                            pOutSubcells[fineIndex];

                        if ( !subcell.m_occupied )
                        {
                            continue;
                        }

                        SoilRetentionSample const& retention =
                            pFineRetentionSamples[fineIndex];

                        float const provisionalThicknessM =
                            float(
                                double(
                                    mantleBulkVolumeM3 ) *
                                ( double(
                                      subcell.m_allocationWeight ) /
                                  weightSum ) /
                                double(
                                    fineCellAreaM2 ) );

                        float const edgeExposure =
                            CalculateSoilFineBreachEdgeExposure(
                                parentGrid,
                                geometryGrid,
                                pParentPatchCells,
                                parentX,
                                parentY,
                                localX,
                                localY,
                                policy );

                        float const minimumStableThicknessM =
                            CalculateSoilFineMinimumStableThickness(
                                retention,
                                authority.m_body.m_physicalState,
                                parent.m_state,
                                edgeExposure );

                        float const marginM =
                            provisionalThicknessM -
                            minimumStableThicknessM;

                        if ( marginM <
                             weakestMarginM )
                        {
                            weakestMarginM =
                                marginM;

                            weakestFineIndex =
                                fineIndex;
                        }
                    }
                }

                if ( weakestFineIndex <
                     0 )
                {
                    break;
                }

                pOutSubcells[weakestFineIndex].m_occupied =
                    false;

                --activeCount;
            }

            if ( activeCount <=
                 0 )
            {
                return result;
            }

            //-------------------------------------------------------------------------
            // Exact volume allocation over surviving fine topology.
            //-------------------------------------------------------------------------
            double finalWeightSum =
                0.0;

            for ( int32_t localY = 0;
                  localY <
                  geometryGrid.m_subcellsPerParentY;
                  ++localY )
            {
                for ( int32_t localX = 0;
                      localX <
                      geometryGrid.m_subcellsPerParentX;
                      ++localX )
                {
                    int32_t const fineX =
                        startFineX +
                        localX;

                    int32_t const fineY =
                        startFineY +
                        localY;

                    int32_t const fineIndex =
                        fineY *
                            geometryGrid.m_numCellsX +
                        fineX;

                    SoilGeometrySubcell const& subcell =
                        pOutSubcells[fineIndex];

                    if ( subcell.m_occupied )
                    {
                        finalWeightSum +=
                            double(
                                subcell.m_allocationWeight );
                    }
                }
            }

            if ( finalWeightSum <=
                 0.0 )
            {
                return result;
            }

            double allocatedMantleVolume =
                0.0;

            int32_t strongestFineIndex =
                -1;

            float strongestWeight =
                -1.0f;

            for ( int32_t localY = 0;
                  localY <
                  geometryGrid.m_subcellsPerParentY;
                  ++localY )
            {
                for ( int32_t localX = 0;
                      localX <
                      geometryGrid.m_subcellsPerParentX;
                      ++localX )
                {
                    int32_t const fineX =
                        startFineX +
                        localX;

                    int32_t const fineY =
                        startFineY +
                        localY;

                    int32_t const fineIndex =
                        fineY *
                            geometryGrid.m_numCellsX +
                        fineX;

                    SoilGeometrySubcell& subcell =
                        pOutSubcells[fineIndex];

                    if ( !subcell.m_occupied )
                    {
                        subcell.m_bulkVolumeM3 =
                            0.0f;

                        subcell.m_thicknessZM =
                            0.0f;

                        subcell.m_soilSurfaceZM =
                            subcell.m_substrateSurfaceZM;

                        subcell.m_contactField =
                            -SoilMax(
                                policy.m_minimumVisibleThicknessM,
                                0.001f );

                        continue;
                    }

                    double const volumeShare =
                        double(
                            mantleBulkVolumeM3 ) *
                        ( double(
                              subcell.m_allocationWeight ) /
                          finalWeightSum );

                    subcell.m_bulkVolumeM3 =
                        float(
                            volumeShare );

                    subcell.m_thicknessZM =
                        subcell.m_bulkVolumeM3 /
                        fineCellAreaM2;

                    subcell.m_soilSurfaceZM =
                        subcell.m_substrateSurfaceZM +
                        subcell.m_thicknessZM;

                    subcell.m_geometryState =
                        parent.m_state ==
                                SoilCoverCellState::RetainedPocket
                            ? SoilVisibleGeometryState::PocketedInfill
                            : SoilVisibleGeometryState::ResidualPatch;

                    // Positive contact magnitude is actual Soil thickness.
                    // This deliberately removes the old edge-taper scaling
                    // that pulled the zero contour toward the occupied vertex
                    // and visually created steep/curtain-like terminations.
                    subcell.m_contactField =
                        SoilMax(
                            subcell.m_thicknessZM,
                            policy.m_minimumVisibleThicknessM );

                    allocatedMantleVolume +=
                        double(
                            subcell.m_bulkVolumeM3 );

                    if ( subcell.m_allocationWeight >
                         strongestWeight )
                    {
                        strongestWeight =
                            subcell.m_allocationWeight;

                        strongestFineIndex =
                            fineIndex;
                    }
                }
            }

            if ( strongestFineIndex <
                 0 )
            {
                return result;
            }

            double const correction =
                double(
                    mantleBulkVolumeM3 ) -
                allocatedMantleVolume;

            SoilGeometrySubcell& correctionCell =
                pOutSubcells[strongestFineIndex];

            correctionCell.m_bulkVolumeM3 =
                SoilMax(
                    correctionCell.m_bulkVolumeM3 +
                        float(
                            correction ),
                    0.0f );

            correctionCell.m_thicknessZM =
                correctionCell.m_bulkVolumeM3 /
                fineCellAreaM2;

            correctionCell.m_soilSurfaceZM =
                correctionCell.m_substrateSurfaceZM +
                correctionCell.m_thicknessZM;

            correctionCell.m_contactField =
                SoilMax(
                    correctionCell.m_thicknessZM,
                    policy.m_minimumVisibleThicknessM );

            // Slope relaxation transfers volume only among this parent's
            // already-surviving fine cells.
            RelaxSoilFineSurfaceSlopeWithinParent(
                geometryGrid,
                parentX,
                parentY,
                authority.m_body.m_physicalState,
                policy,
                pOutSubcells );

            //-------------------------------------------------------------------------
            // Parent receipt.
            //-------------------------------------------------------------------------
            double parentMantleVolume =
                0.0;

            double occupiedThicknessSum =
                0.0;

            bool firstOccupied =
                true;

            receipt.m_numOccupiedSubcells =
                0;

            for ( int32_t localY = 0;
                  localY <
                  geometryGrid.m_subcellsPerParentY;
                  ++localY )
            {
                for ( int32_t localX = 0;
                      localX <
                      geometryGrid.m_subcellsPerParentX;
                      ++localX )
                {
                    int32_t const fineX =
                        startFineX +
                        localX;

                    int32_t const fineY =
                        startFineY +
                        localY;

                    int32_t const fineIndex =
                        fineY *
                            geometryGrid.m_numCellsX +
                        fineX;

                    SoilGeometrySubcell& subcell =
                        pOutSubcells[fineIndex];

                    if ( !subcell.m_occupied )
                    {
                        continue;
                    }

                    subcell.m_soilSurfaceZM =
                        subcell.m_substrateSurfaceZM +
                        subcell.m_thicknessZM;

                    subcell.m_contactField =
                        SoilMax(
                            subcell.m_thicknessZM,
                            policy.m_minimumVisibleThicknessM );

                    parentMantleVolume +=
                        double(
                            subcell.m_bulkVolumeM3 );

                    occupiedThicknessSum +=
                        double(
                            subcell.m_thicknessZM );

                    ++receipt.m_numOccupiedSubcells;

                    if ( firstOccupied )
                    {
                        receipt.m_minOccupiedThicknessZM =
                            subcell.m_thicknessZM;

                        receipt.m_maxOccupiedThicknessZM =
                            subcell.m_thicknessZM;

                        firstOccupied =
                            false;
                    }
                    else
                    {
                        receipt.m_minOccupiedThicknessZM =
                            SoilMin(
                                receipt.m_minOccupiedThicknessZM,
                                subcell.m_thicknessZM );

                        receipt.m_maxOccupiedThicknessZM =
                            SoilMax(
                                receipt.m_maxOccupiedThicknessZM,
                                subcell.m_thicknessZM );
                    }

                    ++result.m_numOccupiedFineCells;

                    if ( subcell.m_geometryState ==
                         SoilVisibleGeometryState::ResidualPatch )
                    {
                        ++result.m_numResidualPatchSubcells;

                        result.m_meanResidualPatchThicknessZM +=
                            subcell.m_thicknessZM;
                    }
                    else if ( subcell.m_geometryState ==
                              SoilVisibleGeometryState::PocketedInfill )
                    {
                        ++result.m_numPocketSubcells;

                        result.m_meanPocketThicknessZM +=
                            subcell.m_thicknessZM;
                    }
                }
            }

            receipt.m_allocatedMantleBulkVolumeM3 =
                float(
                    parentMantleVolume );

            receipt.m_allocatedClodBulkVolumeM3 =
                clodBulkVolumeM3;

            receipt.m_totalAllocatedBulkVolumeM3 =
                receipt.m_allocatedMantleBulkVolumeM3 +
                receipt.m_allocatedClodBulkVolumeM3;

            receipt.m_bulkVolumeResidualM3 =
                receipt.m_totalAllocatedBulkVolumeM3 -
                receipt.m_parentBulkVolumeM3;

            if ( receipt.m_numOccupiedSubcells >
                 0 )
            {
                receipt.m_meanOccupiedThicknessZM =
                    float(
                        occupiedThicknessSum /
                        double(
                            receipt.m_numOccupiedSubcells ) );
            }

            float const parentTolerance =
                SoilMax(
                    s_soilVolumeAbsoluteToleranceM3,
                    SoilAbs(
                        receipt.m_parentBulkVolumeM3 ) *
                        s_soilRelativeTolerance );

            receipt.m_conservationPass =
                SoilAbs(
                    receipt.m_bulkVolumeResidualM3 ) <=
                parentTolerance;

            receipt.m_topologyPass =
                receipt.m_numOccupiedSubcells >
                    0 &&
                parent.m_patchID >
                    0;

            receipt.m_valid =
                receipt.m_conservationPass &&
                receipt.m_topologyPass;

            if ( !receipt.m_valid )
            {
                return result;
            }

            result.m_allocatedMantleBulkVolumeM3 +=
                receipt.m_allocatedMantleBulkVolumeM3;

            result.m_allocatedClodBulkVolumeM3 +=
                receipt.m_allocatedClodBulkVolumeM3;
        }

        //-------------------------------------------------------------------------
        // P3C.9C-4C — signed contact distance + tapered contact wedge
        //
        // 9C-4B proved fine occupancy can break the 2 m accounting silhouette,
        // but the visible contact still used:
        //
        //     occupied  contact = +Soil thickness
        //     breached  contact = -~1 mm
        //
        // That put the zero crossing almost on top of the breached sample and
        // made a 4–7 cm Soil mantle collapse over only a tiny horizontal
        // distance, visually producing steep polygon curtains.
        //
        // The contact field below is now a signed GEOMETRIC distance to the
        // nearest opposite occupancy state. It is presentation geometry only;
        // it does not author Soil quantity.
        //
        // Soil top Z remains:
        //
        //     substrate Z + conserved Soil thickness
        //
        // so clipping/interpolation creates a real sloping wedge toward the
        // substrate at the explicit material contact.
        //-------------------------------------------------------------------------

        float const taperWidthM =
            SoilMax(
                policy.m_residualEdgeTaperWidthM,
                0.75f );

        int32_t const searchRadiusCells =
            4;

        for ( int32_t fineY = 0;
              fineY <
              geometryGrid.m_numCellsY;
              ++fineY )
        {
            for ( int32_t fineX = 0;
                  fineX <
                  geometryGrid.m_numCellsX;
                  ++fineX )
            {
                int32_t const fineIndex =
                    fineY *
                        geometryGrid.m_numCellsX +
                    fineX;

                SoilGeometrySubcell& subcell =
                    pOutSubcells[fineIndex];

                bool const occupied =
                    subcell.m_occupied;

                float nearestOppositeDistanceM =
                    taperWidthM;

                bool foundOpposite =
                    false;

                for ( int32_t offsetY = -searchRadiusCells;
                      offsetY <=
                      searchRadiusCells;
                      ++offsetY )
                {
                    for ( int32_t offsetX = -searchRadiusCells;
                          offsetX <=
                          searchRadiusCells;
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
                            fineX +
                            offsetX;

                        int32_t const neighborY =
                            fineY +
                            offsetY;

                        if ( neighborX < 0 ||
                             neighborX >=
                                 geometryGrid.m_numCellsX ||
                             neighborY < 0 ||
                             neighborY >=
                                 geometryGrid.m_numCellsY )
                        {
                            // The certification-domain boundary is packaging,
                            // not geology. Do not manufacture a Soil edge here.
                            continue;
                        }

                        int32_t const neighborIndex =
                            neighborY *
                                geometryGrid.m_numCellsX +
                            neighborX;

                        bool const neighborOccupied =
                            pOutSubcells[neighborIndex].m_occupied;

                        if ( neighborOccupied ==
                             occupied )
                        {
                            continue;
                        }

                        float const dxM =
                            float(
                                offsetX ) *
                            geometryGrid.m_cellSizeX;

                        float const dyM =
                            float(
                                offsetY ) *
                            geometryGrid.m_cellSizeY;

                        float const distanceSquared =
                            dxM *
                                dxM +
                            dyM *
                                dyM;

                        float const distanceM =
                            distanceSquared >
                                    0.0f
                                ? float(
                                      std::sqrt(
                                          double(
                                              distanceSquared ) ) )
                                : 0.0f;

                        nearestOppositeDistanceM =
                            SoilMin(
                                nearestOppositeDistanceM,
                                distanceM );

                        foundOpposite =
                            true;
                    }
                }

                float const signedDistanceM =
                    foundOpposite
                        ? SoilMax(
                              nearestOppositeDistanceM,
                              0.5f *
                                  SoilMin(
                                      geometryGrid.m_cellSizeX,
                                      geometryGrid.m_cellSizeY ) )
                        : taperWidthM;

                if ( occupied )
                {
                    subcell.m_edgeTaper =
                        SoilClamp01(
                            signedDistanceM /
                            taperWidthM );

                    // Positive signed distance, not Soil thickness.
                    // This places the zero contour between opposite samples
                    // instead of almost on the breached sample.
                    subcell.m_contactField =
                        signedDistanceM;
                }
                else
                {
                    subcell.m_edgeTaper =
                        0.0f;

                    subcell.m_contactField =
                        -signedDistanceM;

                    subcell.m_soilSurfaceZM =
                        subcell.m_substrateSurfaceZM;
                }
            }
        }

        result.m_authoritativeBulkVolumeM3 =
            float(
                totalParentBulkVolume );

        result.m_totalAllocatedBulkVolumeM3 =
            result.m_allocatedMantleBulkVolumeM3 +
            result.m_allocatedClodBulkVolumeM3;

        result.m_bulkVolumeResidualM3 =
            result.m_totalAllocatedBulkVolumeM3 -
            result.m_authoritativeBulkVolumeM3;

        float const denominator =
            SoilMax(
                SoilAbs(
                    result.m_authoritativeBulkVolumeM3 ),
                s_soilMinimumPositiveValue );

        result.m_bulkVolumeRelativeResidual =
            SoilAbs(
                result.m_bulkVolumeResidualM3 ) /
            denominator;

        if ( result.m_numFineCells >
             0 )
        {
            result.m_fineCoverageFraction =
                float(
                    result.m_numOccupiedFineCells ) /
                float(
                    result.m_numFineCells );
        }

        if ( result.m_numResidualPatchSubcells >
             0 )
        {
            result.m_meanResidualPatchThicknessZM /=
                float(
                    result.m_numResidualPatchSubcells );
        }

        if ( result.m_numPocketSubcells >
             0 )
        {
            result.m_meanPocketThicknessZM /=
                float(
                    result.m_numPocketSubcells );
        }

        bool allParentReceiptsPass =
            true;

        for ( int32_t parentIndex = 0;
              parentIndex < numParentCells;
              ++parentIndex )
        {
            SoilPatchCell const& parent =
                pParentPatchCells[parentIndex];

            SoilParentGeometryReceipt const& receipt =
                pParentReceipts[parentIndex];

            if ( parent.m_occupied )
            {
                if ( !receipt.m_valid ||
                     !receipt.m_conservationPass ||
                     !receipt.m_topologyPass )
                {
                    allParentReceiptsPass =
                        false;

                    break;
                }
            }
            else if ( receipt.m_numOccupiedSubcells !=
                      0 )
            {
                allParentReceiptsPass =
                    false;

                break;
            }
        }

        float const globalTolerance =
            SoilMax(
                s_soilVolumeAbsoluteToleranceM3,
                SoilAbs(
                    result.m_authoritativeBulkVolumeM3 ) *
                    s_soilRelativeTolerance );

        result.m_conservationPass =
            SoilAbs(
                result.m_bulkVolumeResidualM3 ) <=
            globalTolerance;

        result.m_parentTopologyPass =
            allParentReceiptsPass;

        result.m_valid =
            result.m_conservationPass &&
            result.m_parentTopologyPass;

        EE_ASSERT(
            result.m_conservationPass );

        EE_ASSERT(
            result.m_parentTopologyPass );

        return result;
    }

    //-------------------------------------------------------------------------
    // Contact-contour interpolation helper
    //-------------------------------------------------------------------------

    static void InterpolateSoilContactPoint(
        SoilGeometrySubcell const& a,
        SoilGeometrySubcell const& b,
        float&                     outX,
        float&                     outY,
        float&                     outZ )
    {
        float const denominator =
            a.m_contactField -
            b.m_contactField;

        float t =
            0.5f;

        if ( SoilAbs(
                 denominator ) >
             s_soilMinimumPositiveValue )
        {
            t =
                SoilClamp01(
                    a.m_contactField /
                    denominator );
        }

        outX =
            a.m_worldX +
            ( b.m_worldX -
              a.m_worldX ) *
                t;

        outY =
            a.m_worldY +
            ( b.m_worldY -
              a.m_worldY ) *
                t;

        // Material contact lives at the substrate/Soil bottom interface.
        outZ =
            a.m_substrateSurfaceZM +
            ( b.m_substrateSurfaceZM -
              a.m_substrateSurfaceZM ) *
                t;
    }

    //-------------------------------------------------------------------------
    // Explicit Soil/substrate contact extraction
    //
    // Fine-cell centers act as signed samples. The zero contour is extracted
    // through each fine-grid quad, so presentation does not expose the parent
    // 2 m accounting-cell rectangles.
    //-------------------------------------------------------------------------

    int32_t ExtractSoilContactSegments(
        SoilGeometryGrid const&    geometryGrid,
        SoilGeometrySubcell const* pSubcells,
        int32_t                    numSubcells,
        SoilContactSegment*        pOutSegments,
        int32_t                    maxOutputSegments )
    {
        if ( pSubcells ==
                 nullptr ||
             pOutSegments ==
                 nullptr ||
             geometryGrid.m_numCellsX <=
                 1 ||
             geometryGrid.m_numCellsY <=
                 1 ||
             numSubcells !=
                 geometryGrid.m_numCellsX *
                     geometryGrid.m_numCellsY ||
             maxOutputSegments <=
                 0 )
        {
            return 0;
        }

        for ( int32_t i = 0;
              i < maxOutputSegments;
              ++i )
        {
            pOutSegments[i] =
                SoilContactSegment();
        }

        int32_t numSegments =
            0;

        for ( int32_t y = 0;
              y <
              geometryGrid.m_numCellsY -
                  1;
              ++y )
        {
            for ( int32_t x = 0;
                  x <
                  geometryGrid.m_numCellsX -
                      1;
                  ++x )
            {
                int32_t const i00 =
                    y *
                        geometryGrid.m_numCellsX +
                    x;

                int32_t const i10 =
                    i00 +
                    1;

                int32_t const i01 =
                    i00 +
                    geometryGrid.m_numCellsX;

                int32_t const i11 =
                    i01 +
                    1;

                SoilGeometrySubcell const& s00 =
                    pSubcells[i00];

                SoilGeometrySubcell const& s10 =
                    pSubcells[i10];

                SoilGeometrySubcell const& s11 =
                    pSubcells[i11];

                SoilGeometrySubcell const& s01 =
                    pSubcells[i01];

                float   intersectionX[4];
                float   intersectionY[4];
                float   intersectionZ[4];
                int32_t intersectionEdge[4];

                int32_t numIntersections =
                    0;

                auto addEdgeIntersection =
                    [&]( SoilGeometrySubcell const& a,
                         SoilGeometrySubcell const& b,
                         int32_t                    edgeIndex )
                {
                    bool const aPositive =
                        a.m_contactField >
                        0.0f;

                    bool const bPositive =
                        b.m_contactField >
                        0.0f;

                    if ( aPositive ==
                         bPositive )
                    {
                        return;
                    }

                    if ( numIntersections >=
                         4 )
                    {
                        return;
                    }

                    InterpolateSoilContactPoint(
                        a,
                        b,
                        intersectionX[numIntersections],
                        intersectionY[numIntersections],
                        intersectionZ[numIntersections] );

                    intersectionEdge[numIntersections] =
                        edgeIndex;

                    ++numIntersections;
                };

                // Bottom, right, top, left.
                addEdgeIntersection(
                    s00,
                    s10,
                    0 );

                addEdgeIntersection(
                    s10,
                    s11,
                    1 );

                addEdgeIntersection(
                    s11,
                    s01,
                    2 );

                addEdgeIntersection(
                    s01,
                    s00,
                    3 );

                if ( numIntersections <
                     2 )
                {
                    continue;
                }

                int32_t patchID =
                    0;

                if ( s00.m_occupied )
                {
                    patchID =
                        s00.m_parentPatchID;
                }
                else if ( s10.m_occupied )
                {
                    patchID =
                        s10.m_parentPatchID;
                }
                else if ( s11.m_occupied )
                {
                    patchID =
                        s11.m_parentPatchID;
                }
                else if ( s01.m_occupied )
                {
                    patchID =
                        s01.m_parentPatchID;
                }

                auto emitSegment =
                    [&]( int32_t a,
                         int32_t b )
                {
                    if ( numSegments >=
                         maxOutputSegments )
                    {
                        return;
                    }

                    SoilContactSegment& segment =
                        pOutSegments[numSegments];

                    segment.m_valid =
                        true;

                    segment.m_patchID =
                        patchID;

                    segment.m_startWorldX =
                        intersectionX[a];

                    segment.m_startWorldY =
                        intersectionY[a];

                    segment.m_startWorldZ =
                        intersectionZ[a];

                    segment.m_endWorldX =
                        intersectionX[b];

                    segment.m_endWorldY =
                        intersectionY[b];

                    segment.m_endWorldZ =
                        intersectionZ[b];

                    ++numSegments;
                };

                if ( numIntersections ==
                     2 )
                {
                    emitSegment(
                        0,
                        1 );
                }
                else if ( numIntersections ==
                          4 )
                {
                    // Marching-squares saddle. Resolve deterministically from
                    // the cell-center signed field rather than from package
                    // order or render state.
                    float const centerField =
                        ( s00.m_contactField +
                          s10.m_contactField +
                          s11.m_contactField +
                          s01.m_contactField ) *
                        0.25f;

                    // Find intersection array indices for the four edge IDs.
                    int32_t edgeToIntersection[4] =
                        {
                            -1,
                            -1,
                            -1,
                            -1 };

                    for ( int32_t i = 0;
                          i < 4;
                          ++i )
                    {
                        edgeToIntersection[intersectionEdge[i]] =
                            i;
                    }

                    if ( centerField >
                         0.0f )
                    {
                        emitSegment(
                            edgeToIntersection[0],
                            edgeToIntersection[3] );

                        emitSegment(
                            edgeToIntersection[1],
                            edgeToIntersection[2] );
                    }
                    else
                    {
                        emitSegment(
                            edgeToIntersection[0],
                            edgeToIntersection[1] );

                        emitSegment(
                            edgeToIntersection[2],
                            edgeToIntersection[3] );
                    }
                }

                if ( numSegments >=
                     maxOutputSegments )
                {
                    return numSegments;
                }
            }
        }

        return numSegments;
    }

    //-------------------------------------------------------------------------
    // P3C.9C-4F — Soil State Geometry Grammar
    //-------------------------------------------------------------------------

    static uint32_t MixSoilBodySeed(
        uint32_t a,
        uint32_t b )
    {
        uint32_t value =
            a ^
            ( b +
              0x9E3779B9u +
              ( a << 6 ) +
              ( a >> 2 ) );

        value ^=
            value >> 16;

        value *=
            0x7FEB352Du;

        value ^=
            value >> 15;

        value *=
            0x846CA68Bu;

        value ^=
            value >> 16;

        return value;
    }

    //-------------------------------------------------------------------------

    static float SoilBodySignal(
        uint32_t seed,
        int32_t  channel )
    {
        uint32_t mixed =
            MixSoilBodySeed(
                seed,
                uint32_t(
                    channel +
                    1 ) *
                    0x9E3779B9u );

        return float(
                   mixed &
                   0x00FFFFFFu ) /
               float(
                   0x00FFFFFFu );
    }

    //-------------------------------------------------------------------------

    static uint32_t BuildSoilBodyGeometrySeed(
        SoilBodyGeometryRequest const& request )
    {
        uint32_t seed =
            MixSoilBodySeed(
                request.m_worldSeed,
                request.m_bodyID );

        seed =
            MixSoilBodySeed(
                seed,
                request.m_provenanceID );

        seed =
            MixSoilBodySeed(
                seed,
                request.m_geometrySalt );

        seed =
            MixSoilBodySeed(
                seed,
                uint32_t(
                    request.m_kind ) );

        return seed;
    }

    //-------------------------------------------------------------------------

    SoilBodyGeometryProfile GetSoilBodyGeometryProfile(
        SoilBodyGeometryKind kind )
    {
        SoilBodyGeometryProfile profile;

        switch ( kind )
        {
            case SoilBodyGeometryKind::CohesiveClod:
            {
                // Irregular cohesive lump:
                // broad low crown, crumbling shoulders, flattened contact.
                profile.m_radialIrregularity = 0.24f;
                profile.m_elongation = 1.10f;
                profile.m_asymmetry = 0.24f;

                profile.m_crownBias = 0.72f;
                profile.m_crownOffset = 0.22f;
                profile.m_shoulderSoftness = 0.62f;
                profile.m_contactFlattening = 0.66f;

                profile.m_aggregateLobeBias = 0.32f;
                profile.m_edgeCrumble = 0.24f;
                profile.m_surfaceUndulation = 0.12f;

                profile.m_maxStableSurfaceSlope = 0.92f;
                break;
            }

            case SoilBodyGeometryKind::LoosePile:
            {
                // Broad repose-controlled mound:
                // widest footprint, low crown, visible asymmetric slump.
                profile.m_radialIrregularity = 0.18f;
                profile.m_elongation = 1.18f;
                profile.m_asymmetry = 0.30f;

                profile.m_crownBias = 0.48f;
                profile.m_crownOffset = 0.32f;
                profile.m_shoulderSoftness = 0.86f;
                profile.m_contactFlattening = 0.34f;

                profile.m_aggregateLobeBias = 0.24f;
                profile.m_edgeCrumble = 0.18f;
                profile.m_surfaceUndulation = 0.10f;

                profile.m_maxStableSurfaceSlope = 0.70f;
                break;
            }

            case SoilBodyGeometryKind::CompactedMass:
            {
                // Dense compacted body:
                // flatter upper surface, smaller bulk envelope, stable edge.
                profile.m_radialIrregularity = 0.07f;
                profile.m_elongation = 1.12f;
                profile.m_asymmetry = 0.10f;

                profile.m_crownBias = 0.24f;
                profile.m_crownOffset = 0.08f;
                profile.m_shoulderSoftness = 0.92f;
                profile.m_contactFlattening = 0.86f;

                profile.m_aggregateLobeBias = 0.02f;
                profile.m_edgeCrumble = 0.04f;
                profile.m_surfaceUndulation = 0.025f;

                profile.m_maxStableSurfaceSlope = 1.15f;
                break;
            }

            default:
            {
                EE_ASSERT( false );
                break;
            }
        }

        return profile;
    }

    //-------------------------------------------------------------------------
    // Soil body mesh helpers
    //-------------------------------------------------------------------------

    static void AddSoilBodyVertex(
        SoilBodyGeometry& geometry,
        float             x,
        float             y,
        float             z )
    {
        EE_ASSERT(
            geometry.m_numVertices <
            SoilBodyGeometry::s_maxVertices );

        if ( geometry.m_numVertices >=
             SoilBodyGeometry::s_maxVertices )
        {
            return;
        }

        SoilPoint& vertex =
            geometry.m_vertices[geometry.m_numVertices++];

        vertex.m_x =
            x;

        vertex.m_y =
            y;

        vertex.m_z =
            z;
    }

    //-------------------------------------------------------------------------

    static void AddSoilBodyTriangle(
        SoilBodyGeometry&     geometry,
        int32_t               a,
        int32_t               b,
        int32_t               c,
        SoilBodySurfaceRegion region )
    {
        EE_ASSERT(
            geometry.m_numTriangles <
            SoilBodyGeometry::s_maxTriangles );

        EE_ASSERT(
            a >= 0 &&
            a < geometry.m_numVertices );

        EE_ASSERT(
            b >= 0 &&
            b < geometry.m_numVertices );

        EE_ASSERT(
            c >= 0 &&
            c < geometry.m_numVertices );

        if ( geometry.m_numTriangles >=
             SoilBodyGeometry::s_maxTriangles )
        {
            return;
        }

        int32_t const indexBase =
            geometry.m_numTriangles *
            3;

        geometry.m_triangleIndices[indexBase + 0] =
            uint8_t(
                a );

        geometry.m_triangleIndices[indexBase + 1] =
            uint8_t(
                b );

        geometry.m_triangleIndices[indexBase + 2] =
            uint8_t(
                c );

        geometry.m_triangleSurfaceRegion[geometry.m_numTriangles] =
            region;

        ++geometry.m_numTriangles;

        switch ( region )
        {
            case SoilBodySurfaceRegion::Crown:
            {
                ++geometry.m_crownTriangleCount;
                break;
            }

            case SoilBodySurfaceRegion::Shoulder:
            {
                ++geometry.m_shoulderTriangleCount;
                break;
            }

            case SoilBodySurfaceRegion::Side:
            {
                ++geometry.m_sideTriangleCount;
                break;
            }

            case SoilBodySurfaceRegion::BasalContact:
            {
                ++geometry.m_contactTriangleCount;
                break;
            }

            case SoilBodySurfaceRegion::AggregateLobe:
            {
                ++geometry.m_crownTriangleCount;
                break;
            }

            default:
            {
                EE_ASSERT( false );
                break;
            }
        }
    }

    //-------------------------------------------------------------------------

    static float CalculateSoilTriangleSignedVolume(
        SoilPoint const& a,
        SoilPoint const& b,
        SoilPoint const& c )
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

    static float CalculateSoilBodyMeshVolume(
        SoilBodyGeometry const& geometry )
    {
        double signedVolume =
            0.0;

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

            signedVolume +=
                double(
                    CalculateSoilTriangleSignedVolume(
                        geometry.m_vertices[ia],
                        geometry.m_vertices[ib],
                        geometry.m_vertices[ic] ) );
        }

        return float(
            signedVolume < 0.0
                ? -signedVolume
                : signedVolume );
    }

    //-------------------------------------------------------------------------

    static void ScaleSoilBodyGeometry(
        SoilBodyGeometry& geometry,
        float             uniformScale )
    {
        for ( int32_t i = 0;
              i < geometry.m_numVertices;
              ++i )
        {
            geometry.m_vertices[i].m_x *=
                uniformScale;

            geometry.m_vertices[i].m_y *=
                uniformScale;

            geometry.m_vertices[i].m_z *=
                uniformScale;
        }
    }

    //-------------------------------------------------------------------------

    static void TranslateSoilBodyGeometry(
        SoilBodyGeometry& geometry,
        float             x,
        float             y,
        float             z )
    {
        for ( int32_t i = 0;
              i < geometry.m_numVertices;
              ++i )
        {
            geometry.m_vertices[i].m_x +=
                x;

            geometry.m_vertices[i].m_y +=
                y;

            geometry.m_vertices[i].m_z +=
                z;
        }
    }

    //-------------------------------------------------------------------------

    static float SoilBodyRingSignal(
        uint32_t geometrySeed,
        int32_t  ringIndex,
        int32_t  sectorIndex,
        int32_t  channelBase )
    {
        return SoilBodySignal(
            geometrySeed,
            channelBase +
                ringIndex *
                    37 +
                sectorIndex *
                    3 );
    }

    //-------------------------------------------------------------------------
    // Three-ring Soil host mesh
    //
    // Rings:
    //
    //     contact ring
    //     shoulder ring
    //     crown ring
    //     broad crown center
    //
    // The top center is intentionally only slightly above the crown ring for
    // CohesiveClod and CompactedMass. This avoids the old radial pyramid apex.
    //-------------------------------------------------------------------------

    static void BuildSoilBodyHostMesh(
        SoilBodyGeometry&              geometry,
        SoilBodyGeometryRequest const& request,
        SoilBodyGeometryProfile const& profile,
        uint32_t                       geometrySeed )
    {
        int32_t constexpr sectorCount =
            12;

        float const twoPi =
            6.28318530718f;

        float const phase =
            SoilBodySignal(
                geometrySeed,
                11 ) *
            twoPi;

        float const orientation =
            SoilBodySignal(
                geometrySeed,
                12 ) *
            twoPi;

        float const cosine =
            float(
                std::cos(
                    double(
                        orientation ) ) );

        float const sine =
            float(
                std::sin(
                    double(
                        orientation ) ) );

        float const elongation =
            SoilMax(
                profile.m_elongation,
                0.2f );

        float const axisX =
            elongation;

        float const axisY =
            1.0f /
            SoilMax(
                elongation *
                    0.92f,
                0.2f );

        float const offsetAngle =
            SoilBodySignal(
                geometrySeed,
                13 ) *
            twoPi;

        float const offsetMagnitude =
            profile.m_crownOffset *
            0.34f;

        float const crownOffsetX =
            float(
                std::cos(
                    double(
                        offsetAngle ) ) ) *
            offsetMagnitude;

        float const crownOffsetY =
            float(
                std::sin(
                    double(
                        offsetAngle ) ) ) *
            offsetMagnitude;

        float contactRadius =
            1.0f;

        float shoulderRadius =
            0.94f;

        float crownRadius =
            0.58f;

        float shoulderZ =
            0.34f;

        float crownZ =
            0.72f;

        float centerZ =
            0.78f;

        switch ( request.m_kind )
        {
            case SoilBodyGeometryKind::CohesiveClod:
            {
                contactRadius = 0.92f;
                shoulderRadius = 1.00f;
                crownRadius = 0.58f;

                shoulderZ = 0.36f;
                crownZ = 0.70f;
                centerZ = 0.76f;
                break;
            }

            case SoilBodyGeometryKind::LoosePile:
            {
                contactRadius = 1.22f;
                shoulderRadius = 0.86f;
                crownRadius = 0.42f;

                shoulderZ = 0.24f;
                crownZ = 0.48f;
                centerZ = 0.56f;
                break;
            }

            case SoilBodyGeometryKind::CompactedMass:
            {
                contactRadius = 1.06f;
                shoulderRadius = 1.00f;
                crownRadius = 0.78f;

                shoulderZ = 0.18f;
                crownZ = 0.30f;
                centerZ = 0.315f;
                break;
            }

            default:
            {
                EE_ASSERT( false );
                break;
            }
        }

        //-------------------------------------------------------------------------
        // Contact ring
        //-------------------------------------------------------------------------

        int32_t const contactStart =
            geometry.m_numVertices;

        for ( int32_t sector = 0;
              sector < sectorCount;
              ++sector )
        {
            float const angle =
                phase +
                twoPi *
                    float(
                        sector ) /
                    float(
                        sectorCount );

            float const irregular =
                ( SoilBodyRingSignal(
                      geometrySeed,
                      0,
                      sector,
                      31 ) -
                  0.5f ) *
                2.0f *
                profile.m_radialIrregularity *
                0.52f;

            float const localRadius =
                contactRadius *
                ( 1.0f +
                  irregular );

            float localX =
                float(
                    std::cos(
                        double(
                            angle ) ) ) *
                localRadius *
                axisX;

            float localY =
                float(
                    std::sin(
                        double(
                            angle ) ) ) *
                localRadius *
                axisY;

            float const rotatedX =
                localX *
                    cosine -
                localY *
                    sine;

            float const rotatedY =
                localX *
                    sine +
                localY *
                    cosine;

            // Basal contact stays deliberately flat. Cohesive clods may have
            // small edge crumbs above this plane, but the support footprint
            // itself must seat cleanly.
            AddSoilBodyVertex(
                geometry,
                rotatedX,
                rotatedY,
                0.0f );
        }

        //-------------------------------------------------------------------------
        // Shoulder ring
        //-------------------------------------------------------------------------

        int32_t const shoulderStart =
            geometry.m_numVertices;

        for ( int32_t sector = 0;
              sector < sectorCount;
              ++sector )
        {
            float const angle =
                phase +
                twoPi *
                    float(
                        sector ) /
                    float(
                        sectorCount );

            float const primaryIrregular =
                ( SoilBodyRingSignal(
                      geometrySeed,
                      1,
                      sector,
                      61 ) -
                  0.5f ) *
                2.0f *
                profile.m_radialIrregularity;

            float const asymmetryWave =
                float(
                    std::sin(
                        double(
                            angle +
                            offsetAngle ) ) ) *
                profile.m_asymmetry *
                0.16f;

            float const lobeWave =
                ( 0.55f *
                      float(
                          std::sin(
                              double(
                                  angle *
                                      2.0f +
                                  phase *
                                      0.37f ) ) ) +
                  0.45f *
                      float(
                          std::cos(
                              double(
                                  angle *
                                      3.0f -
                                  phase *
                                      0.23f ) ) ) ) *
                profile.m_aggregateLobeBias *
                0.16f;

            float const localRadius =
                shoulderRadius *
                ( 1.0f +
                  primaryIrregular +
                  asymmetryWave +
                  lobeWave );

            float localX =
                float(
                    std::cos(
                        double(
                            angle ) ) ) *
                localRadius *
                axisX;

            float localY =
                float(
                    std::sin(
                        double(
                            angle ) ) ) *
                localRadius *
                axisY;

            float const rotatedX =
                localX *
                    cosine -
                localY *
                    sine;

            float const rotatedY =
                localX *
                    sine +
                localY *
                    cosine;

            float const zUndulation =
                ( SoilBodyRingSignal(
                      geometrySeed,
                      1,
                      sector,
                      67 ) -
                  0.5f ) *
                2.0f *
                profile.m_surfaceUndulation *
                0.22f;

            AddSoilBodyVertex(
                geometry,
                rotatedX,
                rotatedY,
                shoulderZ +
                    zUndulation );
        }

        //-------------------------------------------------------------------------
        // Crown ring
        //-------------------------------------------------------------------------

        int32_t const crownStart =
            geometry.m_numVertices;

        for ( int32_t sector = 0;
              sector < sectorCount;
              ++sector )
        {
            float const angle =
                phase +
                twoPi *
                    float(
                        sector ) /
                    float(
                        sectorCount );

            float const crownIrregular =
                ( SoilBodyRingSignal(
                      geometrySeed,
                      2,
                      sector,
                      91 ) -
                  0.5f ) *
                2.0f *
                profile.m_radialIrregularity *
                0.72f;

            float const lobeWave =
                float(
                    std::sin(
                        double(
                            angle *
                                2.0f +
                            phase *
                                0.41f ) ) ) *
                profile.m_aggregateLobeBias *
                0.12f;

            float const localRadius =
                crownRadius *
                ( 1.0f +
                  crownIrregular +
                  lobeWave );

            float localX =
                crownOffsetX +
                float(
                    std::cos(
                        double(
                            angle ) ) ) *
                    localRadius *
                    axisX;

            float localY =
                crownOffsetY +
                float(
                    std::sin(
                        double(
                            angle ) ) ) *
                    localRadius *
                    axisY;

            float const rotatedX =
                localX *
                    cosine -
                localY *
                    sine;

            float const rotatedY =
                localX *
                    sine +
                localY *
                    cosine;

            float const crownUndulation =
                ( SoilBodyRingSignal(
                      geometrySeed,
                      2,
                      sector,
                      97 ) -
                  0.5f ) *
                2.0f *
                profile.m_surfaceUndulation *
                0.24f;

            AddSoilBodyVertex(
                geometry,
                rotatedX,
                rotatedY,
                crownZ +
                    crownUndulation );
        }

        //-------------------------------------------------------------------------
        // Broad crown center
        //-------------------------------------------------------------------------

        int32_t const crownCenter =
            geometry.m_numVertices;

        float const centerJitterX =
            ( SoilBodySignal(
                  geometrySeed,
                  121 ) -
              0.5f ) *
            profile.m_asymmetry *
            0.22f;

        float const centerJitterY =
            ( SoilBodySignal(
                  geometrySeed,
                  122 ) -
              0.5f ) *
            profile.m_asymmetry *
            0.22f;

        AddSoilBodyVertex(
            geometry,
            crownOffsetX +
                centerJitterX,
            crownOffsetY +
                centerJitterY,
            centerZ +
                ( SoilBodySignal(
                      geometrySeed,
                      123 ) -
                  0.5f ) *
                    profile.m_surfaceUndulation *
                    0.16f );

        //-------------------------------------------------------------------------
        // Basal center
        //-------------------------------------------------------------------------

        int32_t const basalCenter =
            geometry.m_numVertices;

        AddSoilBodyVertex(
            geometry,
            0.0f,
            0.0f,
            0.0f );

        //-------------------------------------------------------------------------
        // Triangulate closed host
        //
        // Bottom fan is wound downward; side/crown surfaces outward/upward.
        //-------------------------------------------------------------------------

        for ( int32_t sector = 0;
              sector < sectorCount;
              ++sector )
        {
            int32_t const next =
                ( sector +
                  1 ) %
                sectorCount;

            AddSoilBodyTriangle(
                geometry,
                basalCenter,
                contactStart +
                    next,
                contactStart +
                    sector,
                SoilBodySurfaceRegion::BasalContact );

            AddSoilBodyTriangle(
                geometry,
                contactStart +
                    sector,
                contactStart +
                    next,
                shoulderStart +
                    next,
                SoilBodySurfaceRegion::Side );

            AddSoilBodyTriangle(
                geometry,
                contactStart +
                    sector,
                shoulderStart +
                    next,
                shoulderStart +
                    sector,
                SoilBodySurfaceRegion::Side );

            AddSoilBodyTriangle(
                geometry,
                shoulderStart +
                    sector,
                shoulderStart +
                    next,
                crownStart +
                    next,
                SoilBodySurfaceRegion::Shoulder );

            AddSoilBodyTriangle(
                geometry,
                shoulderStart +
                    sector,
                crownStart +
                    next,
                crownStart +
                    sector,
                SoilBodySurfaceRegion::Shoulder );

            SoilBodySurfaceRegion crownRegion =
                SoilBodySurfaceRegion::Crown;

            if ( request.m_kind !=
                     SoilBodyGeometryKind::CompactedMass &&
                 SoilBodyRingSignal(
                     geometrySeed,
                     3,
                     sector,
                     141 ) <
                     profile.m_aggregateLobeBias )
            {
                crownRegion =
                    SoilBodySurfaceRegion::AggregateLobe;

                ++geometry.m_aggregateLobeCount;
            }

            AddSoilBodyTriangle(
                geometry,
                crownStart +
                    sector,
                crownStart +
                    next,
                crownCenter,
                crownRegion );
        }
    }

    //-------------------------------------------------------------------------

    static void FinalizeSoilBodyReceipts(
        SoilBodyGeometry& geometry,
        float             localBaseWorldZ )
    {
        if ( geometry.m_numVertices <=
             0 )
        {
            return;
        }

        float minX =
            geometry.m_vertices[0].m_x;

        float maxX =
            minX;

        float minY =
            geometry.m_vertices[0].m_y;

        float maxY =
            minY;

        float minZ =
            geometry.m_vertices[0].m_z;

        float maxZ =
            minZ;

        double crownHeightSum =
            0.0;

        int32_t crownVertexCount =
            0;

        for ( int32_t i = 0;
              i < geometry.m_numVertices;
              ++i )
        {
            SoilPoint const& vertex =
                geometry.m_vertices[i];

            minX =
                SoilMin(
                    minX,
                    vertex.m_x );

            maxX =
                SoilMax(
                    maxX,
                    vertex.m_x );

            minY =
                SoilMin(
                    minY,
                    vertex.m_y );

            maxY =
                SoilMax(
                    maxY,
                    vertex.m_y );

            minZ =
                SoilMin(
                    minZ,
                    vertex.m_z );

            maxZ =
                SoilMax(
                    maxZ,
                    vertex.m_z );

            float const relativeZ =
                vertex.m_z -
                localBaseWorldZ;

            if ( relativeZ >
                 0.20f *
                     SoilMax(
                         maxZ -
                             minZ,
                         0.001f ) )
            {
                crownHeightSum +=
                    double(
                        relativeZ );

                ++crownVertexCount;
            }
        }

        geometry.m_extentXM =
            maxX -
            minX;

        geometry.m_extentYM =
            maxY -
            minY;

        geometry.m_extentZM =
            maxZ -
            minZ;

        geometry.m_maxCrownHeightM =
            geometry.m_extentZM;

        if ( crownVertexCount >
             0 )
        {
            geometry.m_meanCrownHeightM =
                float(
                    crownHeightSum /
                    double(
                        crownVertexCount ) );
        }

        float const horizontalHalfSpan =
            0.5f *
            SoilMax(
                SoilMax(
                    geometry.m_extentXM,
                    geometry.m_extentYM ),
                0.001f );

        geometry.m_effectiveSurfaceSlope =
            geometry.m_extentZM /
            horizontalHalfSpan;
    }

    //-------------------------------------------------------------------------

    static float CalculateSoilBodyContactArea(
        SoilBodyGeometry const& geometry )
    {
        double area =
            0.0;

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              geometry.m_numTriangles;
              ++triangleIndex )
        {
            if ( geometry.m_triangleSurfaceRegion[triangleIndex] !=
                 SoilBodySurfaceRegion::BasalContact )
            {
                continue;
            }

            int32_t const indexBase =
                triangleIndex *
                3;

            SoilPoint const& a =
                geometry.m_vertices[int32_t(
                    geometry.m_triangleIndices[indexBase + 0] )];

            SoilPoint const& b =
                geometry.m_vertices[int32_t(
                    geometry.m_triangleIndices[indexBase + 1] )];

            SoilPoint const& c =
                geometry.m_vertices[int32_t(
                    geometry.m_triangleIndices[indexBase + 2] )];

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

            float const magnitude =
                float(
                    std::sqrt(
                        double(
                            crossX *
                                crossX +
                            crossY *
                                crossY +
                            crossZ *
                                crossZ ) ) );

            area +=
                0.5 *
                double(
                    magnitude );
        }

        return float(
            area );
    }

    //-------------------------------------------------------------------------
    // Generate volume-backed Soil body
    //-------------------------------------------------------------------------

    SoilBodyGeometry GenerateSoilBodyGeometry(
        SoilBodyGeometryRequest const& request )
    {
        EE_ASSERT(
            request.m_massGrams >
            0 );

        SoilBodyGeometry geometry;

        geometry.m_kind =
            request.m_kind;

        MatterVolumeMetrics const volumeMetrics =
            CalculateMatterVolume(
                ProvenanceMaterialID::Soil,
                request.m_bodyState,
                request.m_massGrams );

        geometry.m_massKg =
            float(
                request.m_massGrams ) /
            1000.0f;

        geometry.m_particleDensityKgPerM3 =
            volumeMetrics.m_solidDensityKgPerM3;

        geometry.m_packingFraction =
            volumeMetrics.m_packingFraction;

        geometry.m_requiredSolidVolumeM3 =
            volumeMetrics.m_solidVolumeM3;

        geometry.m_requiredBulkVolumeM3 =
            volumeMetrics.m_envelopeVolumeM3;

        EE_ASSERT(
            geometry.m_requiredBulkVolumeM3 >
            0.0f );

        SoilBodyGeometryProfile const profile =
            GetSoilBodyGeometryProfile(
                request.m_kind );

        uint32_t const geometrySeed =
            BuildSoilBodyGeometrySeed(
                request );

        BuildSoilBodyHostMesh(
            geometry,
            request,
            profile,
            geometrySeed );

        float initialVolume =
            CalculateSoilBodyMeshVolume(
                geometry );

        EE_ASSERT(
            initialVolume >
            0.0f );

        if ( initialVolume <=
             0.0f )
        {
            return geometry;
        }

        float firstScale =
            float(
                std::cbrt(
                    double(
                        geometry.m_requiredBulkVolumeM3 /
                        initialVolume ) ) );

        ScaleSoilBodyGeometry(
            geometry,
            firstScale );

        // Second exact-volume normalization after float scaling.
        float measuredVolume =
            CalculateSoilBodyMeshVolume(
                geometry );

        if ( measuredVolume >
             0.0f )
        {
            float correctionScale =
                float(
                    std::cbrt(
                        double(
                            geometry.m_requiredBulkVolumeM3 /
                            measuredVolume ) ) );

            ScaleSoilBodyGeometry(
                geometry,
                correctionScale );
        }

        geometry.m_measuredMeshVolumeM3 =
            CalculateSoilBodyMeshVolume(
                geometry );

        geometry.m_bulkVolumeResidualM3 =
            geometry.m_measuredMeshVolumeM3 -
            geometry.m_requiredBulkVolumeM3;

        float const denominator =
            SoilMax(
                SoilAbs(
                    geometry.m_requiredBulkVolumeM3 ),
                s_soilMinimumPositiveValue );

        geometry.m_bulkVolumeRelativeResidual =
            SoilAbs(
                geometry.m_bulkVolumeResidualM3 ) /
            denominator;

        float const permittedResidual =
            SoilMax(
                s_soilVolumeAbsoluteToleranceM3,
                SoilAbs(
                    geometry.m_requiredBulkVolumeM3 ) *
                    5.0e-5f );

        geometry.m_conservationPass =
            SoilAbs(
                geometry.m_bulkVolumeResidualM3 ) <=
            permittedResidual;

        EE_ASSERT(
            geometry.m_conservationPass );

        geometry.m_contactAreaM2 =
            CalculateSoilBodyContactArea(
                geometry );

        // Translate only after volume/contact calculations to keep numerical
        // precision independent of world position.
        TranslateSoilBodyGeometry(
            geometry,
            request.m_worldX,
            request.m_worldY,
            request.m_worldZ );

        FinalizeSoilBodyReceipts(
            geometry,
            request.m_worldZ );

        return geometry;
    }

}
