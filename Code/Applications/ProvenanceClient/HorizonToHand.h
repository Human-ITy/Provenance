// Esoterica Horizon-to-Hand v1 — continuous matter from geography to grip.
// Same posture as ProvenanceGeography: Esoterica owns engine+client for this lane.
// Law: generate once → preserve structure → tools modify stored structure →
//      derive separation → conserve parent + plate + fines → fall / grip / place.
//
// First proof: mica-schist wall + pick → persistent cracks → plate release →
//              cavity/plate/fines reconcile → gravity → grip at contact.
#pragma once

#include "ProvenanceGeography.h"
#include "RockStructure.h"
#include "VisualMaterial.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace H2H
{
    constexpr int kVersion = 2; // Material×Tool Index v1.2
    constexpr char const* kCapability = "horizon_to_hand_v1";
    constexpr char const* kIndexId = "material_tool_interaction_index_v1_2";

    // ---------- Scale (Index §1) — contact envelope ≠ transfer volume ----------
    constexpr float kVoxelEdgeM = 0.125f;
    constexpr float kVoxelVolumeM3 = kVoxelEdgeM * kVoxelEdgeM * kVoxelEdgeM; // 1.953125 L
    constexpr float kHandTransferVolumeM3 = kVoxelVolumeM3 / 8.f;            // 244.140625 mL
    // LIVE starter-tool contact/query radii (Index §2) — NOT removed-volume promises.
    constexpr float kLiveHandContactRM = 0.16f;
    constexpr float kLiveShovelContactRM = 0.28f;
    constexpr float kLivePickContactRM = 0.18f;
    // Mica-schist freeze (Index §3 / §5.12).
    constexpr int kMicaSchistVoxelG = 5469;
    constexpr float kMicaSchistDensityKgM3 = (float)kMicaSchistVoxelG / kVoxelVolumeM3; // ~2800

    enum class ForceClass : uint8_t { Hand = 0, Shovel, Pick, Axe, Bucket, Count };
    // Tool×material: Primary / Secondary / Collect-loose-only / Indirect / Invalid
    enum class ToolRole : uint8_t { Primary = 0, Secondary, CollectLoose, Indirect, Invalid };

    struct ToolMatterProfile
    {
        char const* id;
        ForceClass force;
        float contact_radius_m;          // LIVE query envelope
        float influence_depth_m;
        float transfer_volume_limit_m3;  // accepted volume budget
        int dig_tier;                    // LIVE hardness gate
        bool can_wedge;
        bool can_scoop;
        bool cuts_wood;
    };

    inline ToolMatterProfile const& ToolHand()
    {
        static ToolMatterProfile const t = {
            "hand", ForceClass::Hand, kLiveHandContactRM, 0.08f, kHandTransferVolumeM3,
            0, false, true, false
        };
        return t;
    }
    inline ToolMatterProfile const& ToolPick()
    {
        static ToolMatterProfile const t = {
            "pick", ForceClass::Pick, kLivePickContactRM, 0.14f, kHandTransferVolumeM3 * 1.5f,
            3, true, false, false
        };
        return t;
    }
    inline ToolMatterProfile const& ToolShovel()
    {
        static ToolMatterProfile const t = {
            "shovel", ForceClass::Shovel, kLiveShovelContactRM, 0.16f, kHandTransferVolumeM3 * 2.0f,
            0, false, true, false
        };
        return t;
    }
    inline ToolMatterProfile const& ToolAxe()
    {
        static ToolMatterProfile const t = {
            "axe", ForceClass::Axe, 0.12f, 0.10f, kHandTransferVolumeM3 * 0.5f,
            0, false, false, true
        };
        return t;
    }
    inline ToolMatterProfile const& ToolBucket()
    {
        static ToolMatterProfile const t = {
            "bucket", ForceClass::Bucket, 0.12f, 0.05f, kHandTransferVolumeM3 * 4.0f,
            0, false, false, false
        };
        return t;
    }

    inline ToolMatterProfile const& ToolById( char const* id )
    {
        if ( id && std::strcmp( id, "pick" ) == 0 ) { return ToolPick(); }
        if ( id && std::strcmp( id, "shovel" ) == 0 ) { return ToolShovel(); }
        if ( id && std::strcmp( id, "axe" ) == 0 ) { return ToolAxe(); }
        if ( id && std::strcmp( id, "bucket" ) == 0 ) { return ToolBucket(); }
        if ( id && ( std::strcmp( id, "torch" ) == 0 || id[0] == 0 ) ) { return ToolHand(); }
        return ToolHand();
    }

    // Volume → integer grams (floor, never mint). Index §1 exact conversion.
    inline int VolumeToGramsFloor( float volumeM3, float densityKgM3 )
    {
        if ( volumeM3 <= 0.f || densityKgM3 <= 0.f ) { return 0; }
        double const g = (double)volumeM3 * (double)densityKgM3 * 1000.0;
        return (int)std::floor( g + 1e-9 );
    }

    inline float GramsToVolumeM3( int grams, float densityKgM3 )
    {
        if ( grams <= 0 || densityKgM3 <= 1e-6f ) { return 0.f; }
        return (float)( (double)grams / ( (double)densityKgM3 * 1000.0 ) );
    }

    inline int TransferGramsCapped( float wantVolumeM3, float densityKgM3,
        float volumeLimitM3, int availableG )
    {
        float const vol = (std::min)( wantVolumeM3, volumeLimitM3 );
        int g = VolumeToGramsFloor( vol, densityKgM3 );
        if ( availableG >= 0 ) { g = (std::min)( g, availableG ); }
        return (std::max)( 0, g );
    }

    // ---------- MaterialFormContract + LIVE VOXEL_G table (Index §3 / §5) ----------
    enum class FabricKind : uint8_t
    {
        Cover = 0, Granular, CohesivePlastic, BeddedFissile, FoliatedAnisotropic, Massive,
        FibrousWood, Fluid, Air, Count
    };
    enum class LooseFamily : uint8_t
    {
        Aggregate = 0, Clod, ThinPlate, Block, Branch, FluidCup, None, Count
    };
    enum class SupportClass : uint8_t
    {
        None = 0, CohesiveCut, BedAttachment, FoliationAttachment, JointAttachment, HingeWood, Count
    };

    struct MaterialFormContract
    {
        int version = 2;
        char const* material_id;
        FabricKind fabric;
        int hardness;                // LIVE gate: 0 cover/fluid, 1 soil, 2 wood, 3 stone/ore
        float cohesion;              // 0..1
        float penetration_resist;
        float wedging_response;
        float crushing_response;
        float fines_fraction;
        float density_kg_m3;
        int voxel_g;                 // LIVE g per full 12.5 cm voxel
        LooseFamily loose_family;
        SupportClass support_class;
        bool aggregate_eligible;
        bool rigid_fracture_body;
        char const* preferred_fracture;
        char const* prime_form;      // Index §5.20 hand-scale form cue
    };

    inline MaterialFormContract const* FormTable( int& n )
    {
        // Density kg/m3 = voxel_g / VOXEL_VOLUME_M3 / 1000... wait voxel_g is grams, volume m3
        // density_kg_m3 = voxel_g / 1000 / kVoxelVolumeM3
        auto dens = []( int vg ) -> float {
            return (float)( (double)vg / 1000.0 / (double)kVoxelVolumeM3 );
        };
        static MaterialFormContract const k[] = {
            { 2, "grass", FabricKind::Cover, 0, 0.15f, 0.10f, 0.05f, 0.40f, 0.50f,
              dens( 1200 ), 1200, LooseFamily::Aggregate, SupportClass::None, true, false,
              "strip_tuft", "tuft / sod flap" },
            { 2, "dirt", FabricKind::CohesivePlastic, 1, 0.35f, 0.30f, 0.20f, 0.55f, 0.40f,
              dens( 2500 ), 2500, LooseFamily::Clod, SupportClass::CohesiveCut, true, false,
              "clod_shear", "root-bound clod 8–18 cm" },
            { 2, "loam", FabricKind::CohesivePlastic, 1, 0.40f, 0.28f, 0.22f, 0.50f, 0.35f,
              dens( 2500 ), 2500, LooseFamily::Clod, SupportClass::CohesiveCut, true, false,
              "clod_shear", "porous rooted clod" },
            { 2, "sand", FabricKind::Granular, 1, 0.08f, 0.15f, 0.05f, 0.90f, 0.95f,
              dens( 3125 ), 3125, LooseFamily::Aggregate, SupportClass::None, true, false,
              "crush_repose", "no stable rigid hand body" },
            { 2, "gravel", FabricKind::Granular, 1, 0.12f, 0.35f, 0.10f, 0.85f, 0.80f,
              dens( 3100 ), 3100, LooseFamily::Aggregate, SupportClass::None, true, false,
              "rubble", "cobble 8–20 cm" },
            { 2, "clay", FabricKind::CohesivePlastic, 1, 0.78f, 0.45f, 0.35f, 0.40f, 0.20f,
              dens( 3900 ), 3900, LooseFamily::Clod, SupportClass::CohesiveCut, true, true,
              "shear_peel", "deformable clod / peel plate" },
            { 2, "stone", FabricKind::Massive, 3, 0.90f, 0.88f, 0.30f, 0.50f, 0.18f,
              dens( 5300 ), 5300, LooseFamily::Block, SupportClass::JointAttachment, true, true,
              "joint_break", "angular chip / wedge 10–20 cm" },
            { 2, "sandstone", FabricKind::BeddedFissile, 3, 0.70f, 0.68f, 0.55f, 0.40f, 0.22f,
              dens( 4492 ), 4492, LooseFamily::ThinPlate, SupportClass::BedAttachment, true, true,
              "bedding_break", "tabular fragment 10–25 cm" },
            { 2, "shale", FabricKind::BeddedFissile, 3, 0.55f, 0.50f, 0.75f, 0.35f, 0.25f,
              dens( 4688 ), 4688, LooseFamily::ThinPlate, SupportClass::BedAttachment, true, true,
              "lamina_split", "thin fissile plate 8–20 cm" },
            { 2, "limestone", FabricKind::Massive, 3, 0.80f, 0.70f, 0.40f, 0.45f, 0.20f,
              dens( 5078 ), 5078, LooseFamily::Block, SupportClass::JointAttachment, true, true,
              "block_break", "joint-bounded chip 10–20 cm" },
            { 2, "granite", FabricKind::Massive, 3, 0.95f, 0.92f, 0.25f, 0.55f, 0.15f,
              dens( 5273 ), 5273, LooseFamily::Block, SupportClass::JointAttachment, true, true,
              "joint_break", "crystalline wedge 10–20 cm" },
            { 2, "mica_schist", FabricKind::FoliatedAnisotropic, 3, 0.78f, 0.72f, 0.92f, 0.30f, 0.18f,
              dens( kMicaSchistVoxelG ), kMicaSchistVoxelG, LooseFamily::ThinPlate,
              SupportClass::FoliationAttachment, true, true,
              "foliation_split", "irregular foliated plate 10–18×2–5 cm" },
            { 2, "basalt", FabricKind::Massive, 3, 0.92f, 0.90f, 0.28f, 0.50f, 0.15f,
              dens( 5859 ), 5859, LooseFamily::Block, SupportClass::JointAttachment, true, true,
              "block_break", "sharp dense wedge 10–20 cm" },
            { 2, "water", FabricKind::Fluid, 0, 0.f, 0.f, 0.f, 0.f, 0.f,
              dens( 1953 ), 1953, LooseFamily::FluidCup, SupportClass::None, false, false,
              "flow", "no stable loose object" },
            { 2, "wood", FabricKind::FibrousWood, 2, 0.60f, 0.40f, 0.20f, 0.30f, 0.25f,
              dens( 1800 ), 1800, LooseFamily::Branch, SupportClass::HingeWood, true, true,
              "grain_split", "stick / branch / log" },
        };
        n = (int)( sizeof( k ) / sizeof( k[0] ) );
        return k;
    }

    inline MaterialFormContract const& FormOrDirt( char const* id )
    {
        int n = 0;
        MaterialFormContract const* t = FormTable( n );
        if ( id && id[0] )
        {
            for ( int i = 0; i < n; ++i )
            {
                if ( std::strcmp( t[i].material_id, id ) == 0 ) { return t[i]; }
            }
            // Prefix aliases: clay_loam → clay, etc.
            for ( int i = 0; i < n; ++i )
            {
                size_t const len = std::strlen( t[i].material_id );
                if ( len >= 3 && std::strncmp( id, t[i].material_id, len ) == 0 ) { return t[i]; }
            }
        }
        return t[1]; // dirt
    }

    // Index §4 starter-tool compatibility for attached cutting/extraction.
    inline ToolRole RoleForAttached( ToolMatterProfile const& tool, MaterialFormContract const& mat )
    {
        if ( mat.fabric == FabricKind::Air ) { return ToolRole::Invalid; }
        if ( mat.fabric == FabricKind::Fluid )
        {
            if ( tool.force == ForceClass::Bucket ) { return ToolRole::Primary; }
            if ( tool.force == ForceClass::Shovel || tool.force == ForceClass::Pick )
            {
                return ToolRole::Indirect; // opens capacity only
            }
            return ToolRole::Invalid;
        }
        if ( mat.fabric == FabricKind::FibrousWood )
        {
            if ( tool.cuts_wood ) { return ToolRole::Primary; }
            if ( tool.force == ForceClass::Hand ) { return ToolRole::CollectLoose; }
            return ToolRole::Invalid;
        }
        if ( mat.fabric == FabricKind::Cover )
        {
            if ( tool.force == ForceClass::Shovel ) { return ToolRole::Primary; }
            if ( tool.force == ForceClass::Hand || tool.force == ForceClass::Pick )
            {
                return ToolRole::Secondary;
            }
            return ToolRole::Invalid;
        }
        // Soft soils hardness 1
        if ( mat.hardness <= 1 )
        {
            if ( tool.force == ForceClass::Shovel ) { return ToolRole::Primary; }
            if ( tool.force == ForceClass::Hand || tool.force == ForceClass::Pick )
            {
                return ToolRole::Secondary;
            }
            if ( tool.force == ForceClass::Bucket ) { return ToolRole::CollectLoose; }
            return ToolRole::Invalid;
        }
        // Wood hardness 2 already handled. Stone/ore hardness 3:
        if ( mat.hardness >= 3 )
        {
            if ( tool.force == ForceClass::Pick && tool.dig_tier >= 3 ) { return ToolRole::Primary; }
            if ( tool.force == ForceClass::Hand || tool.force == ForceClass::Shovel
              || tool.force == ForceClass::Bucket )
            {
                return ToolRole::CollectLoose; // chips only — never mine intact rock
            }
            return ToolRole::Invalid;
        }
        return ToolRole::Invalid;
    }

    inline bool CanExcavateAttached( ToolMatterProfile const& tool, MaterialFormContract const& mat )
    {
        ToolRole const r = RoleForAttached( tool, mat );
        return r == ToolRole::Primary || r == ToolRole::Secondary;
    }

    inline bool IsHardRockAttached( MaterialFormContract const& mat )
    {
        return mat.hardness >= 3 && mat.rigid_fracture_body;
    }

    // Client interaction view (Index §7 published view fields).
    struct MaterialInteractionView
    {
        char const* material_id;
        int form_version;
        float density_kg_m3;
        int hardness;
        float cohesion;
        FabricKind fabric;
        LooseFamily loose_family;
        SupportClass support_class;
        bool rigid_fracture_body;
        char const* visual_key;
        char const* prime_form;
    };

    inline MaterialInteractionView ViewOf( char const* id )
    {
        MaterialFormContract const& f = FormOrDirt( id );
        MaterialInteractionView v;
        v.material_id = f.material_id;
        v.form_version = f.version;
        v.density_kg_m3 = f.density_kg_m3;
        v.hardness = f.hardness;
        v.cohesion = f.cohesion;
        v.fabric = f.fabric;
        v.loose_family = f.loose_family;
        v.support_class = f.support_class;
        v.rigid_fracture_body = f.rigid_fracture_body;
        v.visual_key = f.material_id;
        v.prime_form = f.prime_form;
        return v;
    }

    // Soft scoop: accepted volume → integer grams by material density (not dirt constant).
    inline int ScoopAcceptedGrams( char const* matId, ToolMatterProfile const& tool )
    {
        MaterialFormContract const& f = FormOrDirt( matId );
        return TransferGramsCapped( tool.transfer_volume_limit_m3, f.density_kg_m3,
            tool.transfer_volume_limit_m3, -1 );
    }

    // ---------- Representation class ----------
    enum class RepClass : uint8_t
    {
        Structural = 0,
        Detaching,
        Loose,
        ExplicitBody,
        Aggregate,
        Count
    };

    // ---------- Fracture patch (persistent local cracks) ----------
    struct FracturePatch
    {
        uint64_t patch_id = 0;
        std::string material_id = "mica_schist";
        float ox = 0.f, oy = 0.f, oz = 0.f; // local origin (contact)
        float strikeX = 1.f, strikeY = 0.f;
        float dipDeg = 60.f;
        float nx = 0.f, ny = 0.f, nz = 1.f;
        float crackAlongM = 0.f;
        float crackDepthM = 0.f;
        float openThickM = 0.f;
        float attachment = 1.f; // 1 = fully attached … 0 = free
        int strikeCount = 0;
        int authority_revision = 0;
        bool candidateClosed = false;
        float plateAlongM = 0.f;
        float plateAcrossM = 0.f;
        float plateThickM = 0.f;
    };

    struct SeparationResult
    {
        uint64_t separation_id = 0;
        uint64_t patch_id = 0;
        uint64_t body_id = 0;
        bool detached = false;
        int parent_before_g = 0;
        int parent_after_g = 0;
        int plate_g = 0;
        int fines_g = 0;
        float alongM = 0.f, acrossM = 0.f, thickM = 0.f;
        float cx = 0.f, cy = 0.f, cz = 0.f;
        float strikeX = 1.f, strikeY = 0.f;
        float nx = 0.f, ny = 0.f, nz = 1.f;
        std::string material_id;
        char const* act = "none";
    };

    struct MatterBody
    {
        uint64_t body_id = 0;
        uint64_t separation_id = 0;
        uint64_t patch_id = 0;
        RepClass rep = RepClass::ExplicitBody;
        std::string material_id;
        int materials_g = 0;
        float alongM = 0.f, acrossM = 0.f, thickM = 0.f;
        float x = 0.f, y = 0.f, z = 0.f;
        float vx = 0.f, vy = 0.f, vz = 0.f;
        float strikeX = 1.f, strikeY = 0.f;
        float nx = 0.f, ny = 0.f, nz = 1.f;
        float yaw = 0.f;
        bool gripped = false;
        bool settled = false;
        int body_revision = 1;
        uint64_t form_seed = 0;
    };

    struct AggregatePatch
    {
        uint64_t aggregate_id = 0;
        float x = 0.f, y = 0.f, z = 0.f;
        int materials_g = 0;
        std::string material_id;
        uint64_t source_separation_id = 0;
        int revision = 1;
    };

    struct World
    {
        bool ready = false;
        int world_revision = 0;
        uint64_t next_id = 1;
        std::vector<FracturePatch> patches;
        std::vector<MatterBody> bodies;
        std::vector<AggregatePatch> aggregates;
        // Parent wall mass ledger per patch (reconcile accounting).
        std::unordered_map<uint64_t, int> parent_remaining_g;
    };

    inline World& State()
    {
        static World w;
        return w;
    }

    inline void EnsureReady()
    {
        World& w = State();
        if ( !w.ready )
        {
            w.ready = true;
            w.world_revision = 1;
            w.next_id = 1;
        }
    }

    inline uint64_t AllocId()
    {
        EnsureReady();
        return State().next_id++;
    }

    inline float PlateVolumeM3( float alongM, float acrossM, float thickM )
    {
        // Flattened irregular plate ≈ 0.65 of bounding prism.
        return alongM * acrossM * thickM * 0.65f;
    }

    inline int PlateGrams( MaterialFormContract const& form, float alongM, float acrossM, float thickM )
    {
        float const vol = PlateVolumeM3( alongM, acrossM, thickM );
        int g = VolumeToGramsFloor( vol, form.density_kg_m3 );
        return (std::max)( 1, g );
    }

    // Find or create fracture patch near contact (same seam merge).
    inline FracturePatch* FindOrCreatePatch( float x, float y, float z,
        RockStruct::Foliation const& fol, char const* matId )
    {
        EnsureReady();
        World& w = State();
        for ( FracturePatch& p : w.patches )
        {
            float const dx = p.ox - x, dy = p.oy - y;
            float const d2 = dx * dx + dy * dy;
            float const dot = std::fabs( p.strikeX * fol.strikeX + p.strikeY * fol.strikeY );
            if ( d2 < 0.35f * 0.35f && dot > 0.75f && p.material_id == matId )
            {
                return &p;
            }
        }
        FracturePatch p;
        p.patch_id = AllocId();
        p.material_id = matId ? matId : "mica_schist";
        p.ox = x; p.oy = y; p.oz = z;
        p.strikeX = fol.strikeX; p.strikeY = fol.strikeY;
        p.dipDeg = fol.dipDeg;
        p.nx = fol.nx; p.ny = fol.ny; p.nz = fol.nz;
        p.attachment = 1.f;
        p.authority_revision = 1;
        // Host wall budget for reconcile (~ structural slab mass).
        MaterialFormContract const& form = FormOrDirt( p.material_id.c_str() );
        int const hostG = PlateGrams( form, 0.80f, 0.50f, 0.25f );
        w.parent_remaining_g[p.patch_id] = hostG;
        w.patches.push_back( p );
        ++w.world_revision;
        return &w.patches.back();
    }

    // Deterministic mica-schist (and foliated) pick strike.
    // Rhythm: impact → crack → wedge → plate release (on final attachment loss).
    inline SeparationResult StrikePick( float hitX, float hitY, float hitZ,
        float lookX, float lookY, float lookZ, char const* matId )
    {
        EnsureReady();
        SeparationResult r;
        r.material_id = matId ? matId : "mica_schist";
        MaterialFormContract const& form = FormOrDirt( r.material_id.c_str() );
        RockStruct::Foliation const fol = RockStruct::FoliationAt( hitX, hitY );
        FracturePatch* patch = FindOrCreatePatch( hitX, hitY, hitZ, fol, form.material_id );
        r.patch_id = patch->patch_id;

        float const faceOn = RockStruct::StrikeFaceOn( fol, lookX, lookY, lookZ );
        bool const peel = RockStruct::CanPeelEdge( fol, lookX, lookY, lookZ );
        ToolMatterProfile const& tool = ToolPick();

        int& parentG = State().parent_remaining_g[patch->patch_id];
        r.parent_before_g = parentG;

        // Crush bounded fines at tip (always).
        float crushVol = (std::min)( tool.transfer_volume_limit_m3 * 0.15f,
            PlateVolumeM3( 0.04f, 0.04f, 0.02f ) );
        int fines = VolumeToGramsFloor( crushVol, form.density_kg_m3 );
        fines = (std::max)( 1, fines );

        patch->strikeCount += 1;
        r.act = "impact";

        // Crack elongation follows player swing in the face plane (look × fabric-N), blended with fabric.
        {
            float ax = lookY * fol.nz - lookZ * fol.ny;
            float ay = lookZ * fol.nx - lookX * fol.nz;
            float const axy = std::sqrt( ax * ax + ay * ay );
            if ( axy > 1e-4f )
            {
                float const inv = 1.f / axy;
                float const lx = ax * inv, ly = ay * inv;
                float const blend = 0.55f;
                float sx = blend * lx + ( 1.f - blend ) * fol.strikeX;
                float sy = blend * ly + ( 1.f - blend ) * fol.strikeY;
                float const sl = std::sqrt( sx * sx + sy * sy );
                if ( sl > 1e-5f )
                {
                    patch->strikeX = sx / sl;
                    patch->strikeY = sy / sl;
                }
            }
        }

        if ( !form.rigid_fracture_body )
        {
            // Sand path: no plate — all aggregate.
            fines = VolumeToGramsFloor(
                (std::min)( tool.transfer_volume_limit_m3, kHandTransferVolumeM3 ),
                form.density_kg_m3 );
            fines = (std::max)( 1, fines );
            parentG = (std::max)( 0, parentG - fines );
            r.fines_g = fines;
            r.parent_after_g = parentG;
            r.act = "granular_crush";
            AggregatePatch agg;
            agg.aggregate_id = AllocId();
            agg.x = hitX; agg.y = hitY; agg.z = hitZ;
            agg.materials_g = fines;
            agg.material_id = form.material_id;
            agg.source_separation_id = AllocId();
            State().aggregates.push_back( agg );
            ++State().world_revision;
            return r;
        }

        // Crack growth along fabric.
        float growth = 0.04f + form.wedging_response * 0.06f;
        if ( peel ) { growth *= 1.55f; r.act = "wedge_seam"; }
        else if ( faceOn > 0.70f ) { growth *= 0.45f; r.act = "face_flakes"; }
        else { growth *= 1.05f; r.act = "crack_seam"; }

        // Prefer foliation: cross-fabric strikes grow less.
        growth *= ( 0.35f + 0.65f * ( 1.f - faceOn * 0.85f ) );

        patch->crackAlongM = (std::min)( 0.28f, patch->crackAlongM + growth );
        patch->crackDepthM = (std::min)( 0.12f, patch->crackDepthM + growth * 0.55f );
        patch->openThickM = (std::min)( 0.06f, 0.02f + patch->crackAlongM * 0.12f );
        patch->attachment = (std::max)( 0.f, patch->attachment - ( peel ? 0.28f : 0.12f ) - growth );
        patch->authority_revision += 1;

        // Candidate plate dimensions from stored crack (geometry precedes classification).
        patch->plateAlongM = (std::max)( 0.08f, patch->crackAlongM * 1.1f );
        patch->plateAcrossM = (std::max)( 0.06f, patch->crackDepthM * 1.3f );
        patch->plateThickM = (std::max)( 0.02f, patch->openThickM );
        patch->candidateClosed = ( patch->crackAlongM > 0.10f && patch->crackDepthM > 0.05f );

        // Early strikes: fines only; plate stays attached.
        bool release = patch->candidateClosed && patch->attachment <= 0.08f && patch->strikeCount >= 2;

        if ( !release )
        {
            // Surface flakes / seam damage — fines only, no body.
            int flakeG = fines + VolumeToGramsFloor(
                PlateVolumeM3( patch->plateAlongM * 0.25f, patch->plateAcrossM * 0.2f, 0.008f ),
                form.density_kg_m3 );
            flakeG = (std::min)( flakeG, VolumeToGramsFloor( tool.transfer_volume_limit_m3, form.density_kg_m3 ) );
            flakeG = (std::max)( 1, flakeG );
            parentG = (std::max)( 0, parentG - flakeG );
            r.fines_g = flakeG;
            r.parent_after_g = parentG;
            r.alongM = patch->plateAlongM;
            r.acrossM = patch->plateAcrossM;
            r.thickM = patch->plateThickM;
            r.cx = hitX; r.cy = hitY; r.cz = hitZ;
            r.strikeX = fol.strikeX; r.strikeY = fol.strikeY;
            r.nx = fol.nx; r.ny = fol.ny; r.nz = fol.nz;
            AggregatePatch agg;
            agg.aggregate_id = AllocId();
            agg.x = hitX; agg.y = hitY; agg.z = hitZ;
            agg.materials_g = flakeG;
            agg.material_id = form.material_id;
            agg.source_separation_id = AllocId();
            State().aggregates.push_back( agg );
            ++State().world_revision;
            if ( patch->candidateClosed && patch->attachment > 0.08f )
            {
                r.act = "plate_held"; // geometry exists, still attached
            }
            return r;
        }

        // Final attachment loss → one separation event.
        r.separation_id = AllocId();
        r.detached = true;
        r.act = "slab_release";
        float along = patch->plateAlongM;
        float across = patch->plateAcrossM;
        float thick = patch->plateThickM;
        // Cap plate near hand-scale characteristic extent (10–20 cm), mass from geometry.
        along = (std::min)( along, 0.20f );
        across = (std::min)( across, 0.14f );
        thick = (std::min)( thick, 0.045f );

        int plateG = PlateGrams( form, along, across, thick );
        int finesShare = (int)std::lround( (double)plateG * (double)form.fines_fraction );
        finesShare = (std::max)( fines, finesShare );
        plateG = (std::max)( 1, plateG - finesShare );

        // Transfer volume cap on fines scooped to hand later; plate is conserved body.
        int totalDebit = plateG + finesShare;
        if ( totalDebit > parentG )
        {
            float scale = ( parentG > 0 ) ? ( (float)parentG / (float)totalDebit ) : 0.f;
            plateG = (std::max)( 1, (int)std::floor( plateG * scale ) );
            finesShare = (std::max)( 0, parentG - plateG );
            totalDebit = plateG + finesShare;
        }
        parentG = (std::max)( 0, parentG - totalDebit );

        r.plate_g = plateG;
        r.fines_g = finesShare;
        r.parent_after_g = parentG;
        r.alongM = along; r.acrossM = across; r.thickM = thick;
        r.cx = hitX + lookX * 0.04f;
        r.cy = hitY + lookY * 0.04f;
        r.cz = hitZ + lookZ * 0.02f;
        r.strikeX = fol.strikeX; r.strikeY = fol.strikeY;
        r.nx = fol.nx; r.ny = fol.ny; r.nz = fol.nz;

        MatterBody body;
        body.body_id = AllocId();
        body.separation_id = r.separation_id;
        body.patch_id = patch->patch_id;
        body.rep = RepClass::Loose;
        body.material_id = form.material_id;
        body.materials_g = plateG;
        body.alongM = along; body.acrossM = across; body.thickM = thick;
        body.x = r.cx; body.y = r.cy; body.z = r.cz;
        body.strikeX = fol.strikeX; body.strikeY = fol.strikeY;
        body.nx = fol.nx; body.ny = fol.ny; body.nz = fol.nz;
        // Tip outward / slide down steep foliation.
        body.vx = lookX * 0.35f;
        body.vy = lookY * 0.35f;
        body.vz = 0.15f;
        if ( fol.dipDeg > 50.f )
        {
            body.vx += fol.nx * 0.2f;
            body.vy += fol.ny * 0.2f;
            body.vz -= 0.4f;
        }
        body.form_seed = patch->patch_id ^ r.separation_id;
        r.body_id = body.body_id;
        State().bodies.push_back( body );

        if ( finesShare > 0 )
        {
            AggregatePatch agg;
            agg.aggregate_id = AllocId();
            agg.x = hitX; agg.y = hitY; agg.z = hitZ;
            agg.materials_g = finesShare;
            agg.material_id = form.material_id;
            agg.source_separation_id = r.separation_id;
            State().aggregates.push_back( agg );
        }

        // Reset attachment so further strikes open a new candidate on same seam.
        patch->attachment = 0.85f;
        patch->candidateClosed = false;
        patch->crackAlongM *= 0.35f;
        patch->strikeCount = 0;
        ++State().world_revision;
        return r;
    }

    // Integrate loose plate gravity (client prediction = authority for Esoterica lane).
    template <typename SampleGroundFn>
    inline void StepBodies( float dt, SampleGroundFn sampleGround )
    {
        EnsureReady();
        constexpr float gAcc = 18.f;
        for ( MatterBody& b : State().bodies )
        {
            if ( b.gripped || b.settled ) { continue; }
            if ( b.rep != RepClass::Loose && b.rep != RepClass::ExplicitBody ) { continue; }
            b.vz -= gAcc * dt;
            b.x += b.vx * dt;
            b.y += b.vy * dt;
            b.z += b.vz * dt;
            b.vx *= ( 1.f - 1.8f * dt );
            b.vy *= ( 1.f - 1.8f * dt );
            float ground = b.z;
            if ( sampleGround( b.x, b.y, ground ) )
            {
                float const rest = ground + b.thickM * 0.5f + 0.01f;
                if ( b.z <= rest )
                {
                    b.z = rest;
                    b.vz = 0.f;
                    b.vx *= 0.4f;
                    b.vy *= 0.4f;
                    if ( std::fabs( b.vx ) < 0.05f && std::fabs( b.vy ) < 0.05f )
                    {
                        b.settled = true;
                        b.vx = b.vy = 0.f;
                        b.rep = RepClass::ExplicitBody;
                    }
                }
            }
        }
    }

    // Grip ray vs plate AABB-ish (plate extents along strike / across / thick).
    inline MatterBody* RayHitBody( float ox, float oy, float oz,
        float dx, float dy, float dz, float maxT, float& outT,
        float& hx, float& hy, float& hz )
    {
        EnsureReady();
        MatterBody* best = nullptr;
        float bestT = maxT;
        for ( MatterBody& b : State().bodies )
        {
            if ( b.gripped ) { continue; }
            // Sphere proxy from half-extents.
            float const rad = 0.5f * std::sqrt(
                b.alongM * b.alongM + b.acrossM * b.acrossM + b.thickM * b.thickM );
            float const lx = ox - b.x, ly = oy - b.y, lz = oz - b.z;
            float const bb = lx * dx + ly * dy + lz * dz;
            float const c = lx * lx + ly * ly + lz * lz - rad * rad;
            float const disc = bb * bb - c;
            if ( disc < 0.f ) { continue; }
            float t = -bb - std::sqrt( disc );
            if ( t < 0.05f ) { t = -bb + std::sqrt( disc ); }
            if ( t < 0.05f || t >= bestT ) { continue; }
            bestT = t;
            best = &b;
            hx = ox + dx * t; hy = oy + dy * t; hz = oz + dz * t;
        }
        outT = bestT;
        return best;
    }

    inline bool ReconcileOk( SeparationResult const& r )
    {
        return ( r.parent_after_g + r.plate_g + r.fines_g ) == r.parent_before_g;
    }
}
