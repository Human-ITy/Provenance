// ProvenanceClient — renderer visual appearance contract (not full geology doctrine).
// Hand the client: body form, edges, fracture, merge, microrelief, wetness, texture.
// Storage lattice / material-index tiles must never define visible shape.
#pragma once

#include <cstdint>
#include <cstring>

namespace VisualMat
{
    enum class Family : uint8_t
    {
        Soil = 0,
        Aggregate,
        Sedimentary,
        Igneous,
        Metamorphic,
        Count
    };

    enum class BodyForm : uint8_t
    {
        SoftMantle = 0,   // dirt/loam continuous cover
        GranularDrift,    // sand
        AggregatePile,    // gravel
        CohesiveMass,     // clay
        BeddedLedge,      // sandstone / limestone
        ThinBedRock,      // shale
        MassiveBlock,     // granite
        DenseFlow,        // basalt
        FoliatedMass,     // mica schist
        Count
    };

    enum class EdgeBehavior : uint8_t
    {
        RoundedCrumbly = 0,
        SlumpRound,
        IrregularBroken,
        SteepSmooth,      // clay holds cuts
        SteppedLayered,
        FlakyPlate,
        SoftenedLedge,
        BlockyJointed,
        DenseBlocky,
        PlanarDirectional,
        Count
    };

    enum class FractureCharacter : uint8_t
    {
        Clods = 0,
        LooseCollapse,
        CoarseRubble,
        ShearSmear,
        BeddingGuided,
        SplitLayers,
        BlockyBedding,
        AngularBlocks,
        SharpBlocks,
        FoliationSplit,
        Count
    };

    enum class MergeBehavior : uint8_t
    {
        SeamlessSoil = 0,
        SmoothDeposit,
        AggregatePile,
        FuseSame,
        FormationContinuous,
        AnisotropicBeds,
        MassiveBody,
        DirectionalContinuous,
        Count
    };

    enum class Microrelief : uint8_t
    {
        CrumbPores = 0,
        FineRipple,
        ChunkyPebble,
        SmoothCrackNet,
        GrainyFace,
        StackedLaminae,
        WeatheredPits,
        CrystallineRough,
        HardVolcanic,
        SheetFlake,
        Count
    };

    enum class SurfaceTexture : uint8_t
    {
        EarthySoft = 0,
        FineGranular,
        CoarseRocky,
        DenseSmeared,
        WeatheredLayered,
        FinePlaty,
        ChalkyStone,
        HardCrystalline,
        DenseStony,
        MicaceousDirectional,
        Count
    };

    enum class WetnessResponse : uint8_t
    {
        DarkenTacky = 0,
        DarkenCompact,
        PatchySparkle,
        GlossyUnified,
        StreakBedding,
        DarkenPlanes,
        DampStreak,
        CrackSpecular,
        DeepSlick,
        PlaneSpecular,
        Count
    };

    struct ColorRange
    {
        uint8_t r0 = 100, g0 = 100, b0 = 80;
        uint8_t r1 = 120, g1 = 110, b1 = 90;
    };

    // Client-facing visual handoff — enough for continuous-body terrain language.
    struct VisualMaterialDef
    {
        char const* id;
        Family family;
        BodyForm body_form;
        EdgeBehavior edge_behavior;
        FractureCharacter fracture_character;
        MergeBehavior merge_behavior;
        Microrelief microrelief;
        SurfaceTexture surface_texture;
        WetnessResponse wetness_response;
        ColorRange color_range;
        char const* scale_cues;       // human-readable scale implication
        char const* contact_behavior; // meets water/air/slopes/neighbors
    };

    // Canonical table — dirt/loam, sand, gravel, clay, sandstone, shale,
    // limestone, granite, basalt, mica schist (+ a few bridge aliases).
    inline VisualMaterialDef const* Table( int& outCount )
    {
        static VisualMaterialDef const kDefs[] = {
            { "dirt", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::RoundedCrumbly,
              FractureCharacter::Clods, MergeBehavior::SeamlessSoil, Microrelief::CrumbPores,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 110, 88, 58, 140, 110, 72 },
              "fine-to-medium crumb structure",
              "blends with soil; soft slopes; no smooth clay walls" },
            { "loam", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::RoundedCrumbly,
              FractureCharacter::Clods, MergeBehavior::SeamlessSoil, Microrelief::CrumbPores,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 96, 78, 52, 128, 102, 68 },
              "fine-to-medium crumb structure",
              "organic mantle; continuous with neighboring soil" },
            { "sand", Family::Aggregate, BodyForm::GranularDrift, EdgeBehavior::SlumpRound,
              FractureCharacter::LooseCollapse, MergeBehavior::SmoothDeposit, Microrelief::FineRipple,
              SurfaceTexture::FineGranular, WetnessResponse::DarkenCompact,
              { 178, 160, 108, 200, 182, 128 },
              "small-scale ripples; no chunky breakup",
              "slumps at repose; never holds vertical cut walls" },
            { "gravel", Family::Aggregate, BodyForm::AggregatePile, EdgeBehavior::IrregularBroken,
              FractureCharacter::CoarseRubble, MergeBehavior::AggregatePile, Microrelief::ChunkyPebble,
              SurfaceTexture::CoarseRocky, WetnessResponse::PatchySparkle,
              { 118, 114, 106, 148, 142, 132 },
              "obvious coarse aggregate / clast-scale breakup",
              "piles in pockets; irregular edges; not clay-smooth" },
            { "clay", Family::Soil, BodyForm::CohesiveMass, EdgeBehavior::SteepSmooth,
              FractureCharacter::ShearSmear, MergeBehavior::FuseSame, Microrelief::SmoothCrackNet,
              SurfaceTexture::DenseSmeared, WetnessResponse::GlossyUnified,
              { 142, 88, 62, 168, 108, 78 },
              "low-grain cohesive appearance",
              "holds steeper cuts; fuses with same clay; slick when wet" },
            { "sandstone", Family::Sedimentary, BodyForm::BeddedLedge, EdgeBehavior::SteppedLayered,
              FractureCharacter::BeddingGuided, MergeBehavior::FormationContinuous, Microrelief::GrainyFace,
              SurfaceTexture::WeatheredLayered, WetnessResponse::StreakBedding,
              { 168, 132, 96, 196, 158, 118 },
              "visible strata and ledges",
              "stepped bedding faces; continuous within formation" },
            { "shale", Family::Sedimentary, BodyForm::ThinBedRock, EdgeBehavior::FlakyPlate,
              FractureCharacter::SplitLayers, MergeBehavior::AnisotropicBeds, Microrelief::StackedLaminae,
              SurfaceTexture::FinePlaty, WetnessResponse::DarkenPlanes,
              { 92, 90, 84, 120, 116, 108 },
              "thin layering visible",
              "splits along beds; directional flake edges" },
            { "limestone", Family::Sedimentary, BodyForm::BeddedLedge, EdgeBehavior::SoftenedLedge,
              FractureCharacter::BlockyBedding, MergeBehavior::FormationContinuous, Microrelief::WeatheredPits,
              SurfaceTexture::ChalkyStone, WetnessResponse::DampStreak,
              { 176, 172, 158, 210, 206, 190 },
              "layered stone; broad coherent masses",
              "weathered ledges; coherent stone mass" },
            { "granite", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 148, 140, 132, 178, 170, 160 },
              "large-scale blocky structure",
              "jointed hard edges; massive bodies" },
            { "basalt", Family::Igneous, BodyForm::DenseFlow, EdgeBehavior::DenseBlocky,
              FractureCharacter::SharpBlocks, MergeBehavior::MassiveBody, Microrelief::HardVolcanic,
              SurfaceTexture::DenseStony, WetnessResponse::DeepSlick,
              { 52, 54, 58, 78, 80, 86 },
              "heavy dense rock; dark coherent forms",
              "dense volcanic skin; sharp block breaks" },
            { "mica_schist", Family::Metamorphic, BodyForm::FoliatedMass, EdgeBehavior::PlanarDirectional,
              FractureCharacter::FoliationSplit, MergeBehavior::DirectionalContinuous, Microrelief::SheetFlake,
              SurfaceTexture::MicaceousDirectional, WetnessResponse::PlaneSpecular,
              { 110, 108, 96, 142, 138, 118 },
              "visible foliation and directional splitting",
              "planar flake contacts; directional merge" },
            // Bridge / cover aliases map into the same visual language
            { "rock", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 120, 118, 112, 150, 146, 138 },
              "blocky rock mass",
              "generic hard stone fallback" },
            { "stone", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 120, 118, 112, 150, 146, 138 },
              "blocky rock mass",
              "generic hard stone fallback" },
            { "mud", Family::Soil, BodyForm::CohesiveMass, EdgeBehavior::SteepSmooth,
              FractureCharacter::ShearSmear, MergeBehavior::FuseSame, Microrelief::SmoothCrackNet,
              SurfaceTexture::DenseSmeared, WetnessResponse::GlossyUnified,
              { 86, 68, 48, 112, 88, 62 },
              "cohesive wet soil",
              "smears; holds soft cuts when damp" },
            { "grass", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::RoundedCrumbly,
              FractureCharacter::Clods, MergeBehavior::SeamlessSoil, Microrelief::CrumbPores,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 78, 118, 58, 108, 148, 78 },
              "vegetated soil mantle",
              "cover over dirt/loam body" },
        };
        outCount = (int)( sizeof( kDefs ) / sizeof( kDefs[0] ) );
        return kDefs;
    }

    inline VisualMaterialDef const* Find( char const* id )
    {
        if ( !id || !id[0] ) { return nullptr; }
        int n = 0;
        VisualMaterialDef const* t = Table( n );
        for ( int i = 0; i < n; ++i )
        {
            if ( std::strcmp( t[i].id, id ) == 0 ) { return &t[i]; }
        }
        // prefix aliases: "clay_loam" → clay, etc.
        for ( int i = 0; i < n; ++i )
        {
            size_t const len = std::strlen( t[i].id );
            if ( len >= 3 && std::strncmp( id, t[i].id, len ) == 0 ) { return &t[i]; }
        }
        return nullptr;
    }

    inline VisualMaterialDef const& OrDirt( char const* id )
    {
        if ( VisualMaterialDef const* d = Find( id ) ) { return *d; }
        int n = 0;
        return Table( n )[0]; // dirt
    }

    // Palette from visual def (mid of color_range). Optional edge darkening for rock bodies.
    inline void CapColor( char const* cap, uint8_t& r, uint8_t& g, uint8_t& b )
    {
        VisualMaterialDef const& d = OrDirt( cap && cap[0] ? cap : "dirt" );
        r = (uint8_t)( ( (int)d.color_range.r0 + (int)d.color_range.r1 ) / 2 );
        g = (uint8_t)( ( (int)d.color_range.g0 + (int)d.color_range.g1 ) / 2 );
        b = (uint8_t)( ( (int)d.color_range.b0 + (int)d.color_range.b1 ) / 2 );
    }

    // Cheap face shade cue from edge/body — rocks read slightly cooler/harder; soils warmer.
    inline float EdgeShadeBias( VisualMaterialDef const& d )
    {
        switch ( d.edge_behavior )
        {
        case EdgeBehavior::BlockyJointed:
        case EdgeBehavior::DenseBlocky:
        case EdgeBehavior::SteppedLayered:
            return 0.92f;
        case EdgeBehavior::SlumpRound:
        case EdgeBehavior::RoundedCrumbly:
            return 1.04f;
        case EdgeBehavior::SteepSmooth:
            return 0.98f;
        default:
            return 1.0f;
        }
    }

    // Microrelief amplitude hint in meters (presentation grain — not authority).
    inline float MicroreliefAmpM( VisualMaterialDef const& d )
    {
        switch ( d.microrelief )
        {
        case Microrelief::ChunkyPebble: return 0.018f;
        case Microrelief::CrystallineRough: return 0.012f;
        case Microrelief::StackedLaminae: return 0.008f;
        case Microrelief::FineRipple: return 0.004f;
        case Microrelief::CrumbPores: return 0.006f;
        case Microrelief::WeatheredPits: return 0.010f;
        default: return 0.003f;
        }
    }
}
