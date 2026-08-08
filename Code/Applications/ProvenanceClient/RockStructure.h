// Conserved rock structure for foliated / jointed materials (mica schist law).
// Material index alone is identity + attrs. These fields make directional geology real:
//   1) foliation orientation — persistent layer direction
//   2) fracture state — which planes are cracked / opened
//   3) structural support — whether a slab can remain suspended
//
// Scale vs 6ft character (~1.83 m ≈ 15×12.5 cm voxels). 8ft wall ≈ 20 voxels.
//   < ~2 cm     : texture / mica flash / fine flakes (shader)
//   ~2–12.5 cm  : visible foliation relief + chipped edges
//   ≥ 12.5 cm   : conserved geometry, cavities, detachable slabs
#pragma once

#include "ProvenanceGeography.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace RockStruct
{
    constexpr float kCharHeightM = 1.8288f;
    constexpr float kVoxelEdgeM = 0.125f;
    constexpr float kFullVoxelMassKg_MicaSchist = 5.469f; // H2H freeze: 5469 g / voxel

    // Representation bands (meters)
    constexpr float kFineTextureMaxM = 0.02f;
    constexpr float kVisibleReliefMaxM = 0.125f;

    // Pick opening / plate sizes (mica schist)
    constexpr float kPickPunctureM = 0.08f;       // ~pick-head concentrated pit
    constexpr float kPickOpenAlongM = 0.18f;      // 10–25 cm class opening along seam
    constexpr float kPickPlateThickM = 0.04f;     // thin plate thickness
    constexpr float kPalmPlateAlongM = 0.22f;     // 15–40 cm palm plate
    constexpr float kPalmPlateAcrossM = 0.14f;
    constexpr float kStructuralSlabM = 0.55f;     // dangerous leg/torso class

    enum class FractureStage : uint8_t
    {
        Intact = 0,
        SurfaceFlakes,   // first square strike — small pit + flakes
        SeamOpened,      // crack traveling along foliation
        PlateReleased,   // primary conserved chunk separated
        Undermined,      // support low — collapse risk
    };

    // Persistent foliation at a world point (analytic until refined).
    struct Foliation
    {
        // Unit normal to the layer planes (points toward younger / free face when possible).
        float nx = 0.f, ny = 0.f, nz = 1.f;
        // Strike (horizontal along layers) and dip direction in XY for heightfield notches.
        float strikeX = 1.f, strikeY = 0.f;
        float dipDeg = 60.f; // 0 = beds flat, 90 = vertical sheets
    };

    struct FractureState
    {
        FractureStage stage = FractureStage::Intact;
        float crackAlongM = 0.f;   // opened seam length along strike
        float crackDepthM = 0.f;   // how far the pry recess goes
        float openThickM = 0.f;    // plate thickness liberated
        int strikeCount = 0;
    };

    struct StructuralSupport
    {
        // 1 = fully connected; 0 = free slab / imminent fall.
        float support = 1.f;
        bool suspended = false; // lip / slab still hanging
    };

    struct Sample
    {
        Foliation foliation;
        FractureState fracture;
        StructuralSupport support;
        ProvenanceGeo::RockBody rock = ProvenanceGeo::RockBody::DirtMantle;
        char const* cap = "dirt";
    };

    inline void Normalize3( float& x, float& y, float& z )
    {
        float const len = std::sqrt( x * x + y * y + z * z );
        if ( len < 1e-8f ) { x = 0.f; y = 0.f; z = 1.f; return; }
        x /= len; y /= len; z /= len;
    }

    // Deterministic persistent foliation from seed + absolute coords.
    // Shield / schist: steep to moderate dip; strike wanders slowly (not random per dig).
    inline Foliation FoliationAt( double x, double y )
    {
        ProvenanceGeo::EnsureReady();
        uint64_t const s = ProvenanceGeo::State().seed;
        Foliation f;

        // Slow strike field (~48–96 m) so neighboring walls share orientation.
        float const yawN = ProvenanceGeo::Fbm( x, y, s ^ 0xA11CEULL, 72.0, 3 );
        float const dipN = ProvenanceGeo::Fbm( x + 3.0, y - 5.0, s ^ 0xF0110ULL, 56.0, 2 );
        float const strikeAng = yawN * 6.2831853f;
        f.strikeX = std::cos( strikeAng );
        f.strikeY = std::sin( strikeAng );

        ProvenanceGeo::Province const prov = ProvenanceGeo::ProvinceAt( x, y );
        ProvenanceGeo::RockBody const rock = ProvenanceGeo::RockAt( x, y );
        float dip = 35.f + dipN * 50.f; // 35–85°
        if ( rock == ProvenanceGeo::RockBody::MicaSchist )
        {
            dip = 48.f + dipN * 38.f; // schist prefers steep sheets on walls
        }
        if ( prov == ProvenanceGeo::Province::Plateau ) { dip *= 0.65f; }
        if ( dip < 8.f ) { dip = 8.f; }
        if ( dip > 88.f ) { dip = 88.f; }
        f.dipDeg = dip;

        // Normal: rotate up toward dip direction (perp strike in XY).
        float const dipRad = dip * 0.017453292f;
        float const dx = -f.strikeY; // dip azimuth
        float const dy = f.strikeX;
        f.nx = dx * std::sin( dipRad );
        f.ny = dy * std::sin( dipRad );
        f.nz = std::cos( dipRad );
        Normalize3( f.nx, f.ny, f.nz );
        return f;
    }

    // How squarely the look hits the foliation plane (1 = face-on into plane, 0 = edge-on / peel).
    inline float StrikeFaceOn( Foliation const& f, float lookX, float lookY, float lookZ )
    {
        float const d = std::fabs( f.nx * lookX + f.ny * lookY + f.nz * lookZ );
        return d < 0.f ? 0.f : ( d > 1.f ? 1.f : d );
    }

    // Edge catch: look nearly parallel to planes → pry / peel between layers.
    inline bool CanPeelEdge( Foliation const& f, float lookX, float lookY, float lookZ )
    {
        return StrikeFaceOn( f, lookX, lookY, lookZ ) < 0.45f;
    }

    inline float PlateVolumeM3( float alongM, float acrossM, float thickM )
    {
        // Irregular plate ≈ flattened ellipsoid / prism proxy.
        return alongM * acrossM * thickM * 0.65f;
    }

    inline int PlateMassG( ProvenanceGeo::RockBody rock, float alongM, float acrossM, float thickM )
    {
        float const dens = ProvenanceGeo::Attrs( rock ).densityGPerL * 1000.f; // g/m³
        float const g = PlateVolumeM3( alongM, acrossM, thickM ) * dens;
        return (std::max)( 1, (int)std::lround( g ) );
    }

    // Carry gate: half-meter structural chunks are not normal handheld scoops.
    inline bool TooHeavyToCarryNormally( int massG )
    {
        return massG > 12000; // ~12 kg — beyond casual handheld plate
    }

    inline char const* StageName( FractureStage st )
    {
        switch ( st )
        {
        case FractureStage::SurfaceFlakes: return "surface_flakes";
        case FractureStage::SeamOpened: return "seam_opened";
        case FractureStage::PlateReleased: return "plate_released";
        case FractureStage::Undermined: return "undermined";
        default: return "intact";
        }
    }
}
