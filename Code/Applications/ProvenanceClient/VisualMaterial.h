// ProvenanceClient — renderer visual appearance contract (not full geology doctrine).
// Hand the client: body form, edges, fracture, merge, microrelief, wetness, texture.
// Storage lattice / material-index tiles must never define visible shape.
//
// Global shape doctrine (presentation):
//   primary body form | curvature profile | edge character |
//   fracture grammar  | surface character | pile/detached behavior
// Same voxel accounting underneath; each material keeps its own shape language.
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
            { "wood", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::IrregularBroken,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 96, 68, 42, 128, 92, 58 },
              "fibrous branch / stick body",
              "linear growth fragment; not rock fracture" },
            // Deposit / ore — CapColor for host-ish midtones; gallery paints mineral overlays.
            { "hematite", Family::Igneous, BodyForm::DenseFlow, EdgeBehavior::DenseBlocky,
              FractureCharacter::SharpBlocks, MergeBehavior::MassiveBody, Microrelief::HardVolcanic,
              SurfaceTexture::DenseStony, WetnessResponse::DeepSlick,
              { 88, 52, 44, 128, 72, 52 },
              "ore-host fragment with rusty mineralization",
              "dense host body; mineral seam, not a red gem" },
            { "azurite", Family::Sedimentary, BodyForm::MassiveBlock, EdgeBehavior::IrregularBroken,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::WeatheredPits,
              SurfaceTexture::CoarseRocky, WetnessResponse::PatchySparkle,
              { 48, 78, 118, 72, 112, 148 },
              "vein-backed copper mineral body",
              "blue/teal mineral on dark host, not generic blue rock" },
            { "gold", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::IrregularBroken,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 168, 142, 58, 210, 178, 72 },
              "soft-metal seam / bleb in host",
              "metallic vein fragment; never crystal points" },
            { "quartz", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 210, 208, 200, 236, 234, 228 },
              "crystal point / cluster morphology",
              "faceted growth from matrix base" },
            { "ruby", Family::Igneous, BodyForm::DenseFlow, EdgeBehavior::DenseBlocky,
              FractureCharacter::SharpBlocks, MergeBehavior::MassiveBody, Microrelief::HardVolcanic,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 168, 42, 48, 210, 68, 72 },
              "corundum in basalt matrix",
              "hard crystal inclusion; matrix relationship required" },
            { "amethyst", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::IrregularBroken,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 118, 72, 158, 158, 98, 198 },
              "geode rind + purple crystal interior",
              "hollow growth fragment; shell outside, points inside" },
            { "lapis", Family::Metamorphic, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::WeatheredPits,
              SurfaceTexture::ChalkyStone, WetnessResponse::DampStreak,
              { 42, 62, 132, 68, 92, 168 },
              "opaque massive pigment stone",
              "not translucent; host-backed pigment body" },
            { "emerald", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 48, 138, 78, 72, 178, 98 },
              "seam-backed green crystal body",
              "crystal protruding from host, not freestanding gem" },
            // Free-body aliases — same visual family, CapColor still works via exact id.
            { "hematite_free", Family::Igneous, BodyForm::DenseFlow, EdgeBehavior::DenseBlocky,
              FractureCharacter::SharpBlocks, MergeBehavior::MassiveBody, Microrelief::HardVolcanic,
              SurfaceTexture::DenseStony, WetnessResponse::DeepSlick,
              { 98, 48, 40, 138, 68, 48 },
              "free hematite nodule", "detached ore mass" },
            { "azurite_free", Family::Sedimentary, BodyForm::MassiveBlock, EdgeBehavior::IrregularBroken,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::WeatheredPits,
              SurfaceTexture::CoarseRocky, WetnessResponse::PatchySparkle,
              { 40, 88, 148, 68, 120, 168 },
              "free azurite mass", "detached copper mineral body" },
            { "gold_free", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::IrregularBroken,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::WeatheredPits,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 198, 168, 48, 228, 198, 72 },
              "free gold nugget", "lobate metallic body; not crystal" },
            { "quartz_free", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 210, 208, 200, 236, 234, 228 },
              "free quartz crystal", "prismatic cluster without required host" },
            { "ruby_free", Family::Igneous, BodyForm::DenseFlow, EdgeBehavior::DenseBlocky,
              FractureCharacter::SharpBlocks, MergeBehavior::MassiveBody, Microrelief::HardVolcanic,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 178, 38, 48, 220, 68, 72 },
              "free corundum", "stout prism / barrel crystal" },
            { "amethyst_free", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::IrregularBroken,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 118, 72, 158, 158, 98, 198 },
              "free amethyst cluster", "purple quartz points without geode rind" },
            { "lapis_free", Family::Metamorphic, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::WeatheredPits,
              SurfaceTexture::ChalkyStone, WetnessResponse::DampStreak,
              { 42, 62, 132, 68, 92, 168 },
              "free lapis chunk", "opaque massive pigment stone" },
            { "emerald_free", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 48, 138, 78, 72, 178, 98 },
              "free emerald crystal", "detached green crystal body" },
            // MW6 diagnostic climate visualization only — not biomes / soils / snowpacks.
            { "climate_cold", Family::Sedimentary, BodyForm::BeddedLedge, EdgeBehavior::SoftenedLedge,
              FractureCharacter::BlockyBedding, MergeBehavior::FormationContinuous, Microrelief::WeatheredPits,
              SurfaceTexture::ChalkyStone, WetnessResponse::DampStreak,
              { 186, 206, 228, 226, 236, 246 },
              "diagnostic alpine / cold temperature field",
              "MW6 climate authority; not a snowpack or glacier" },
            { "climate_wet", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::RoundedCrumbly,
              FractureCharacter::Clods, MergeBehavior::SeamlessSoil, Microrelief::CrumbPores,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 28, 118, 112, 52, 158, 142 },
              "diagnostic windward / wet moisture field",
              "MW6 climate authority; not a forest biome" },
            { "climate_dry", Family::Sedimentary, BodyForm::BeddedLedge, EdgeBehavior::SteppedLayered,
              FractureCharacter::BeddingGuided, MergeBehavior::FormationContinuous, Microrelief::GrainyFace,
              SurfaceTexture::WeatheredLayered, WetnessResponse::StreakBedding,
              { 196, 148, 72, 228, 178, 96 },
              "diagnostic leeward / rain-shadow moisture field",
              "MW6 climate authority; not a desert biome" },
            { "climate_pool", Family::Sedimentary, BodyForm::BeddedLedge, EdgeBehavior::SoftenedLedge,
              FractureCharacter::BlockyBedding, MergeBehavior::FormationContinuous, Microrelief::WeatheredPits,
              SurfaceTexture::ChalkyStone, WetnessResponse::DampStreak,
              { 72, 82, 148, 102, 112, 178 },
              "diagnostic basin / valley cold-pool field",
              "MW6 climate authority; not fog or vegetation" },
            { "climate_ridge", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 168, 168, 176, 198, 198, 206 },
              "diagnostic exposed-ridge field",
              "MW6 climate authority; not a lithology paint" },
            { "climate_mild", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::RoundedCrumbly,
              FractureCharacter::Clods, MergeBehavior::SeamlessSoil, Microrelief::CrumbPores,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 118, 132, 96, 148, 162, 118 },
              "diagnostic mild / lowland climate field",
              "MW6 climate authority; not a grassland biome" },
            // MW7 diagnostic profile visualization only — not biomes / vegetation.
            { "regolith_bedrock", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 168, 168, 176, 198, 198, 206 },
              "diagnostic bare bedrock exposure",
              "MW7 regolith authority; not a lithology paint" },
            { "regolith_weathered", Family::Sedimentary, BodyForm::BeddedLedge, EdgeBehavior::SteppedLayered,
              FractureCharacter::BeddingGuided, MergeBehavior::FormationContinuous, Microrelief::WeatheredPits,
              SurfaceTexture::ChalkyStone, WetnessResponse::DampStreak,
              { 168, 148, 118, 196, 176, 138 },
              "diagnostic weathered bedrock profile",
              "MW7 regolith authority; not vegetation" },
            { "regolith_thin", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::RoundedCrumbly,
              FractureCharacter::Clods, MergeBehavior::SeamlessSoil, Microrelief::CrumbPores,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 148, 128, 96, 172, 150, 112 },
              "diagnostic thin residual regolith",
              "MW7 regolith authority; not a dirt biome" },
            { "regolith_colluvium", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::RoundedCrumbly,
              FractureCharacter::Clods, MergeBehavior::SeamlessSoil, Microrelief::GrainyFace,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 128, 108, 78, 152, 128, 92 },
              "diagnostic colluvial slope debris",
              "MW7 regolith authority; not vegetation" },
            { "regolith_alluvium", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::RoundedCrumbly,
              FractureCharacter::Clods, MergeBehavior::SeamlessSoil, Microrelief::CrumbPores,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 118, 122, 78, 142, 146, 96 },
              "diagnostic alluvial profile",
              "MW7 regolith authority; not a floodplain biome" },
            { "regolith_floodplain", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::RoundedCrumbly,
              FractureCharacter::Clods, MergeBehavior::SeamlessSoil, Microrelief::CrumbPores,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 98, 112, 72, 122, 138, 90 },
              "diagnostic floodplain sediment profile",
              "MW7 regolith authority; not vegetation" },
            { "regolith_basin", Family::Sedimentary, BodyForm::BeddedLedge, EdgeBehavior::SoftenedLedge,
              FractureCharacter::BlockyBedding, MergeBehavior::FormationContinuous, Microrelief::WeatheredPits,
              SurfaceTexture::ChalkyStone, WetnessResponse::DampStreak,
              { 142, 118, 72, 168, 142, 92 },
              "diagnostic basin-fill profile",
              "MW7 regolith authority; not host FormationId" },
            { "regolith_talus", Family::Igneous, BodyForm::MassiveBlock, EdgeBehavior::BlockyJointed,
              FractureCharacter::AngularBlocks, MergeBehavior::MassiveBody, Microrelief::CrystallineRough,
              SurfaceTexture::HardCrystalline, WetnessResponse::CrackSpecular,
              { 138, 132, 124, 168, 160, 148 },
              "diagnostic talus / coarse slope debris",
              "MW7 regolith authority; not a rock biome" },
            { "regolith_organic", Family::Soil, BodyForm::SoftMantle, EdgeBehavior::RoundedCrumbly,
              FractureCharacter::Clods, MergeBehavior::SeamlessSoil, Microrelief::CrumbPores,
              SurfaceTexture::EarthySoft, WetnessResponse::DarkenTacky,
              { 72, 68, 52, 96, 88, 64 },
              "diagnostic organic-matter potential (capability only)",
              "MW7 regolith authority; not living plants" },
            { "regolith_waterlogged", Family::Sedimentary, BodyForm::BeddedLedge, EdgeBehavior::SoftenedLedge,
              FractureCharacter::BlockyBedding, MergeBehavior::FormationContinuous, Microrelief::WeatheredPits,
              SurfaceTexture::ChalkyStone, WetnessResponse::DampStreak,
              { 72, 86, 108, 96, 112, 138 },
              "diagnostic waterlogged mineral substrate",
              "MW7 regolith authority; not a wetland biome" },
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

    // Intrinsic reflectance response — not baked directional paint.
    // Relative + coherent for the first lit-material cut (gallery cert).
    inline float RoughnessOf( VisualMaterialDef const& d )
    {
        char const* id = d.id ? d.id : "dirt";
        if ( std::strstr( id, "gold" ) ) { return 0.22f; }
        if ( std::strcmp( id, "quartz" ) == 0 || std::strstr( id, "quartz_" ) ) { return 0.18f; }
        if ( std::strcmp( id, "ruby" ) == 0 || std::strstr( id, "ruby" )
          || std::strcmp( id, "emerald" ) == 0 || std::strstr( id, "emerald" )
          || std::strcmp( id, "amethyst" ) == 0 || std::strstr( id, "amethyst" ) )
        {
            return 0.16f;
        }
        if ( std::strcmp( id, "lapis" ) == 0 || std::strstr( id, "lapis" ) ) { return 0.55f; }
        if ( std::strcmp( id, "mica_schist" ) == 0 ) { return 0.42f; }
        switch ( d.surface_texture )
        {
        case SurfaceTexture::EarthySoft:
        case SurfaceTexture::FineGranular: return 0.92f;
        case SurfaceTexture::DenseSmeared: return 0.70f;
        case SurfaceTexture::CoarseRocky:
        case SurfaceTexture::WeatheredLayered: return 0.78f;
        case SurfaceTexture::FinePlaty: return 0.62f;
        case SurfaceTexture::ChalkyStone: return 0.72f;
        case SurfaceTexture::HardCrystalline: return 0.38f;
        case SurfaceTexture::DenseStony: return 0.55f;
        case SurfaceTexture::MicaceousDirectional: return 0.40f;
        default: return 0.75f;
        }
    }

    inline float MetallicOf( VisualMaterialDef const& d )
    {
        char const* id = d.id ? d.id : "";
        if ( std::strstr( id, "gold" ) ) { return 1.f; }
        return 0.f;
    }

    // Cheap face shade cue from edge/body — rocks read slightly cooler/harder; soils warmer.
    // Prefer world lighting for brightness; keep only mild intrinsic bias if needed.
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
