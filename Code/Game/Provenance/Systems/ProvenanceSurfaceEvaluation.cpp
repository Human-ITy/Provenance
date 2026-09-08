#include "ProvenanceSurfaceEvaluation.h"

#include "Game/Provenance/Geometry/MaterialGeometry.h"
#include "Game/Provenance/Geometry/GraniteGeometry.h"

#include <cmath>

namespace EE
{
    //-------------------------------------------------------------------------
    // Geological ancestry
    //-------------------------------------------------------------------------

    static constexpr uint32_t s_graniteGeologicalAncestryID =
        7001u;

    //-------------------------------------------------------------------------
    // Deterministic spatial hash
    //
    // Broad landform + Soil use this path.
    //
    // Granite structural decisions belong to GraniteGeometry.
    //-------------------------------------------------------------------------

    static uint32_t HashProvenancePoint(
        uint32_t seed,
        int32_t  x,
        int32_t  y )
    {
        uint32_t h =
            seed;

        h ^=
            uint32_t( x ) *
            0x9E3779B9u;

        h ^=
            uint32_t( y ) *
            0x85EBCA6Bu;

        h ^=
            h >>
            16;

        h *=
            0x7FEB352Du;

        h ^=
            h >>
            15;

        h *=
            0x846CA68Bu;

        h ^=
            h >>
            16;

        return h;
    }

    //-------------------------------------------------------------------------

    static float HashToUnitFloat(
        uint32_t seed,
        int32_t  x,
        int32_t  y )
    {
        uint32_t const h =
            HashProvenancePoint(
                seed,
                x,
                y );

        uint32_t const value =
            h &
            0x00FFFFFFu;

        return float( value ) /
               float( 0x00FFFFFFu );
    }

    //-------------------------------------------------------------------------
    // Math helpers
    //-------------------------------------------------------------------------

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
    // Continuous absolute-coordinate value noise
    //-------------------------------------------------------------------------

    static float SampleValueNoise(
        uint32_t seed,
        float    worldX,
        float    worldY,
        float    scale,
        uint32_t salt )
    {
        EE_ASSERT(
            scale >
            0.0f );

        float const scaledX =
            worldX /
            scale;

        float const scaledY =
            worldY /
            scale;

        int32_t const gridX =
            int32_t(
                std::floor(
                    double(
                        scaledX ) ) );

        int32_t const gridY =
            int32_t(
                std::floor(
                    double(
                        scaledY ) ) );

        float const localX =
            scaledX -
            float(
                gridX );

        float const localY =
            scaledY -
            float(
                gridY );

        float const tx =
            SmoothStep01(
                localX );

        float const ty =
            SmoothStep01(
                localY );

        uint32_t const saltedSeed =
            seed ^
            salt;

        float const v00 =
            HashToUnitFloat(
                saltedSeed,
                gridX,
                gridY );

        float const v10 =
            HashToUnitFloat(
                saltedSeed,
                gridX + 1,
                gridY );

        float const v01 =
            HashToUnitFloat(
                saltedSeed,
                gridX,
                gridY + 1 );

        float const v11 =
            HashToUnitFloat(
                saltedSeed,
                gridX + 1,
                gridY + 1 );

        float const bottom =
            LerpFloat(
                v00,
                v10,
                tx );

        float const top =
            LerpFloat(
                v01,
                v11,
                tx );

        return LerpFloat(
            bottom,
            top,
            ty );
    }

    //-------------------------------------------------------------------------
    // Broad geological carrier
    //-------------------------------------------------------------------------

    static float SampleBroadLandform(
        uint32_t seed,
        float    worldX,
        float    worldY )
    {
        float const macro =
            SampleValueNoise(
                seed,
                worldX,
                worldY,
                16.0f,
                0xA341316Cu );

        float const meso =
            SampleValueNoise(
                seed,
                worldX,
                worldY,
                8.0f,
                0xC8013EA4u );

        return 5.0f +
               ( macro -
                 0.5f ) *
                   5.0f +
               ( meso -
                 0.5f ) *
                   2.0f;
    }

    //-------------------------------------------------------------------------
    // Granite broad-body offset
    //-------------------------------------------------------------------------

    static float SampleGraniteBroadBodyOffset(
        uint32_t seed,
        float    worldX,
        float    worldY )
    {
        float const smoothSignal =
            SampleValueNoise(
                seed,
                worldX,
                worldY,
                5.0f,
                0x7E95761Eu );

        float const structuralSignal =
            SampleValueNoise(
                seed,
                worldX,
                worldY,
                4.5f,
                0xD1B54A35u );

        float const broadRidgeNoise =
            SampleValueNoise(
                seed,
                worldX,
                worldY,
                9.0f,
                0x94D049BBu );

        float const ridgeSignal =
            1.0f -
            AbsFloat(
                broadRidgeNoise *
                    2.0f -
                1.0f );

        MaterialGeometryRequest request;

        request.m_material =
            ProvenanceMaterialID::Granite;

        request.m_bodyState =
            ProvenanceBodyState::Continuous;

        request.m_scale =
            1.0f;

        request.m_seed =
            seed;

        request.m_primarySignal =
            smoothSignal;

        request.m_secondarySignal =
            structuralSignal;

        request.m_tertiarySignal =
            ridgeSignal;

        MaterialGeometryResult const geometry =
            EvaluateMaterialGeometry(
                request );

        return geometry.m_surfaceOffset;
    }

    //-------------------------------------------------------------------------
    // Soil cover
    //-------------------------------------------------------------------------

    static float SampleSoilCoverBudget(
        uint32_t seed,
        float    worldX,
        float    worldY )
    {
        float const broadGradient =
            0.20f +
            worldX *
                0.055f -
            worldY *
                0.030f;

        float const broadVariation =
            SampleValueNoise(
                seed,
                worldX,
                worldY,
                7.0f,
                0x68E31DA4u );

        float const localVariation =
            SampleValueNoise(
                seed,
                worldX,
                worldY,
                2.25f,
                0xB8D2646Cu );

        return broadGradient +
               ( broadVariation -
                 0.5f ) *
                   0.32f +
               ( localVariation -
                 0.5f ) *
                   0.10f;
    }

    //-------------------------------------------------------------------------
    // Soil mantle response
    //-------------------------------------------------------------------------

    static float SampleSoilMantleResponse(
        uint32_t seed,
        float    worldX,
        float    worldY )
    {
        MaterialGeometryProfile const soil =
            GetMaterialGeometryProfile(
                ProvenanceMaterialID::Soil );

        float const primary =
            SampleValueNoise(
                seed,
                worldX,
                worldY,
                2.60f,
                0xAD90777Du );

        float const secondary =
            SampleValueNoise(
                seed,
                worldX,
                worldY,
                1.25f,
                0x9E3779B1u );

        float const tertiary =
            SampleValueNoise(
                seed,
                worldX,
                worldY,
                0.75f,
                0x243F6A88u );

        MaterialGeometryRequest request;

        request.m_material =
            ProvenanceMaterialID::Soil;

        request.m_bodyState =
            ProvenanceBodyState::Continuous;

        request.m_scale =
            soil.m_minimumMeaningfulFeatureScaleM;

        request.m_seed =
            seed;

        request.m_primarySignal =
            primary;

        request.m_secondarySignal =
            secondary;

        request.m_tertiarySignal =
            tertiary;

        MaterialGeometryResult const geometry =
            EvaluateMaterialGeometry(
                request );

        return geometry.m_surfaceOffset *
               0.10f;
    }

    //-------------------------------------------------------------------------
    // Evaluate absolute-coordinate terrain truth
    //
    // P3C.8 doctrine:
    //
    //     Granite remains fully present beneath Soil.
    //
    //     Soil thickness filters how strongly Granite STRUCTURE influences
    //     the Soil surface.
    //
    // Therefore:
    //
    //     thick Soil  -> broad Granite form only
    //     medium Soil -> some shoulders/ribs
    //     thin Soil   -> stronger joint-controlled expression
    //     zero Soil   -> full Granite surface
    //
    // This is attenuation of visible influence, not attenuation of Granite
    // matter or geology.
    //-------------------------------------------------------------------------

    EE_GAME_API EvaluatedSurfacePoint EvaluateSurfacePoint(
        uint32_t seed,
        float    worldX,
        float    worldY )
    {
        EvaluatedSurfacePoint result;

        result.m_broadLandform =
            SampleBroadLandform(
                seed,
                worldX,
                worldY );

        result.m_graniteBroadBodyOffset =
            SampleGraniteBroadBodyOffset(
                seed,
                worldX,
                worldY );

        result.m_bedrockBase =
            result.m_broadLandform +
            result.m_graniteBroadBodyOffset;

        GraniteOutcropRequest graniteRequest;

        graniteRequest.m_worldSeed =
            seed;

        graniteRequest.m_geologicalAncestryID =
            s_graniteGeologicalAncestryID;

        graniteRequest.m_weathering =
            GraniteWeatheringState::WeatheredExposure;

        graniteRequest.m_worldX =
            worldX;

        graniteRequest.m_worldY =
            worldY;

        graniteRequest.m_baseBedrockZ =
            result.m_bedrockBase;

        GraniteOutcropResult const granite =
            EvaluateGraniteOutcropGeometry(
                graniteRequest );

        result.m_graniteLargeScaleRelief =
            granite.m_largeScaleReliefM;

        result.m_graniteMediumScaleRelief =
            granite.m_mediumScaleReliefM;

        result.m_graniteSmallScaleRelief =
            granite.m_smallScaleReliefM;

        result.m_granitePrimaryJointRecess =
            granite.m_primaryJointRecessM;

        result.m_graniteSecondaryJointRecess =
            granite.m_secondaryJointRecessM;

        result.m_graniteStructuralRelief =
            granite.m_structuralReliefM;

        result.m_graniteWeatheringRelief =
            granite.m_weatheringReliefM;

        result.m_graniteJointField =
            granite.m_jointField;

        result.m_graniteJointInfluence =
            granite.m_jointField.m_combinedJointInfluence;

        // Actual Granite truth is always the complete Granite surface.
        result.m_bedrockElevation =
            granite.m_surfaceZ;

        //-------------------------------------------------------------------------
        // Independent Soil cover
        //-------------------------------------------------------------------------

        result.m_soilCoverBudget =
            SampleSoilCoverBudget(
                seed,
                worldX,
                worldY );

        result.m_soilMantleResponse =
            SampleSoilMantleResponse(
                seed,
                worldX,
                worldY );

        result.m_mantleActivation =
            SmoothStep01(
                (
                    result.m_soilCoverBudget +
                    0.10f ) /
                0.45f );

        GraniteGeometryProfile const graniteProfile =
            GetGraniteGeometryProfile();

        // Potential cover depth is a Soil-side quantity used only to decide
        // how much buried Granite structure may influence the Soil surface.
        float const potentialCoverDepth =
            MaxFloat(
                result.m_soilCoverBudget +
                    result.m_soilMantleResponse *
                        result.m_mantleActivation,
                0.0f );

        float const smallBurialDepth =
            MaxFloat(
                graniteProfile.m_smallFeatureBurialDepthM,
                0.01f );

        float const mediumBurialDepth =
            MaxFloat(
                graniteProfile.m_mediumFeatureBurialDepthM,
                smallBurialDepth +
                    0.01f );

        float const largeInfluenceDepth =
            MaxFloat(
                graniteProfile.m_largeFeatureInfluenceDepthM,
                mediumBurialDepth +
                    0.01f );

        // Small fracture detail disappears first.
        result.m_smallGraniteVisibilityUnderSoil =
            1.0f -
            SmoothStep01(
                potentialCoverDepth /
                smallBurialDepth );

        // Medium ribs/slabs survive somewhat deeper.
        result.m_mediumGraniteVisibilityUnderSoil =
            1.0f -
            SmoothStep01(
                potentialCoverDepth /
                mediumBurialDepth );

        // Broad coherent Granite can influence the Soil mantle much deeper,
        // but even that influence becomes subdued under substantial cover.
        result.m_largeGraniteVisibilityUnderSoil =
            0.22f +
            0.78f *
                ( 1.0f -
                  SmoothStep01(
                      potentialCoverDepth /
                      largeInfluenceDepth ) );

        float const primaryJointVisibility =
            1.0f -
            SmoothStep01(
                potentialCoverDepth /
                mediumBurialDepth );

        float const secondaryJointVisibility =
            result.m_smallGraniteVisibilityUnderSoil;

        // Weathered broad relief follows the large-scale carrier. It is an
        // exterior/broad-form modification, not a high-frequency crack mask.
        float const weatheringVisibility =
            result.m_largeGraniteVisibilityUnderSoil;

        result.m_filteredGraniteInfluenceUnderSoil =
            result.m_graniteLargeScaleRelief *
                result.m_largeGraniteVisibilityUnderSoil +
            result.m_graniteMediumScaleRelief *
                result.m_mediumGraniteVisibilityUnderSoil +
            result.m_graniteSmallScaleRelief *
                result.m_smallGraniteVisibilityUnderSoil +
            result.m_granitePrimaryJointRecess *
                primaryJointVisibility +
            result.m_graniteSecondaryJointRecess *
                secondaryJointVisibility +
            result.m_graniteWeatheringRelief *
                weatheringVisibility;

        //-------------------------------------------------------------------------
        // Soil settling / infill
        //
        // Joint recesses are negative Granite relief. Soil preferentially
        // occupies those lows, which prevents every buried crack from being
        // stamped visibly through the mantle.
        //-------------------------------------------------------------------------

        float const primaryRecessDepth =
            MaxFloat(
                -result.m_granitePrimaryJointRecess,
                0.0f );

        float const secondaryRecessDepth =
            MaxFloat(
                -result.m_graniteSecondaryJointRecess,
                0.0f );

        float const localStructuralLow =
            MaxFloat(
                -result.m_filteredGraniteInfluenceUnderSoil,
                0.0f );

        float const jointInfill =
            primaryRecessDepth *
                0.52f +
            secondaryRecessDepth *
                0.72f;

        float const recessInfill =
            localStructuralLow *
            0.18f;

        result.m_soilGraniteInfillResponse =
            ( jointInfill +
              recessInfill ) *
            result.m_mantleActivation;

        //-------------------------------------------------------------------------
        // Soil candidate surface
        //
        // Broad buried Granite can influence the mantle, but detail is filtered
        // according to cover depth. This is the anti-"granite pattern under a
        // blanket" rule.
        //-------------------------------------------------------------------------

        result.m_soilSurfaceElevation =
            result.m_bedrockBase +
            result.m_filteredGraniteInfluenceUnderSoil +
            result.m_soilCoverBudget +
            result.m_soilMantleResponse *
                result.m_mantleActivation +
            result.m_soilGraniteInfillResponse;

        //-------------------------------------------------------------------------
        // Exposure
        //-------------------------------------------------------------------------

        result.m_signedSoilDepth =
            result.m_soilSurfaceElevation -
            result.m_bedrockElevation;

        if ( result.m_signedSoilDepth >
             0.0f )
        {
            result.m_soilThickness =
                result.m_signedSoilDepth;

            result.m_material =
                ProvenanceMaterialID::Soil;

            result.m_elevation =
                result.m_soilSurfaceElevation;
        }
        else
        {
            result.m_soilThickness =
                0.0f;

            result.m_material =
                ProvenanceMaterialID::Granite;

            result.m_elevation =
                result.m_bedrockElevation;
        }

        return result;
    }

}
