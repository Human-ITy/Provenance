// Esoterica-native deterministic planet geography (seed → causes → relief → surface cap).
// Analytic until resident. Absolute coordinates. Not a Fablescript/Unreal port.
// Chain: WORLD SEED → province → geology → relief → surface material (cap).
//
// Geo fixtures (D2 harness):
//   RANGE   — default play/dev. Local mountain-flank transect near spawn (see kRangeOrigin*).
//   TORTURE — prior extreme FBM cliffs/diagonals/voids for edge-case dig/D2 stress.
//   BASELINE— constant-cost flat floor for isolated worldgen streaming/performance measurement.
// Select: --geo-fixture=range|torture|baseline or F8 in-client. Virgin load stays HF-only (no D2).
#pragma once

#include "VisualMaterial.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

namespace ProvenanceGeo
{
    constexpr int kGeneratorVersion = 1;
    constexpr char const* kGeneratorId = "esoterica_geography_v1";

    // F1 representative harness origin (matches client default feet ~128,128).
    // Walk +X through A→G; corridor is local in Y — not a whole mountain in a tiny pad.
    constexpr double kRangeOriginX = 128.0;
    constexpr double kRangeOriginY = 128.0;

    enum class GeoFixture : uint8_t
    {
        Range = 0,   // PROVENANCE GEOLOGY RANGE (representative)
        Torture = 1, // D2 TORTURE TERRAIN (extreme prior path)
        Baseline = 2 // WORLDGEN BASELINE (flat, constant-cost traversal floor)
    };

    inline GeoFixture& FixtureMut()
    {
        static GeoFixture f = GeoFixture::Range;
        return f;
    }

    inline GeoFixture Fixture() { return FixtureMut(); }

    inline void SetFixture( GeoFixture f ) { FixtureMut() = f; }

    inline char const* FixtureName( GeoFixture f )
    {
        switch ( f )
        {
        case GeoFixture::Range: return "range";
        case GeoFixture::Torture: return "torture";
        case GeoFixture::Baseline: return "baseline";
        default: return "range";
        }
    }

    inline char const* FixtureLabel( GeoFixture f )
    {
        switch ( f )
        {
        case GeoFixture::Range: return "PROVENANCE GEOLOGY RANGE";
        case GeoFixture::Torture: return "D2 TORTURE TERRAIN";
        case GeoFixture::Baseline: return "WORLDGEN BASELINE FLOOR";
        default: return "PROVENANCE GEOLOGY RANGE";
        }
    }

    inline bool AsciiEqI( char const* a, char const* b )
    {
        if ( !a || !b ) { return false; }
        while ( *a && *b )
        {
            unsigned char ca = (unsigned char)*a++;
            unsigned char cb = (unsigned char)*b++;
            if ( ca >= 'A' && ca <= 'Z' ) { ca = (unsigned char)( ca - 'A' + 'a' ); }
            if ( cb >= 'A' && cb <= 'Z' ) { cb = (unsigned char)( cb - 'A' + 'a' ); }
            if ( ca != cb ) { return false; }
        }
        return *a == *b;
    }

    // Accepts "range" / "geology" / "f1" and "torture" / "extreme" / "f2".
    inline bool SetFixtureFromString( char const* s )
    {
        if ( !s || !s[0] ) { return false; }
        if ( AsciiEqI( s, "range" ) || AsciiEqI( s, "geology" )
          || AsciiEqI( s, "geo-range" ) || AsciiEqI( s, "f1" ) )
        {
            SetFixture( GeoFixture::Range );
            return true;
        }
        if ( AsciiEqI( s, "torture" ) || AsciiEqI( s, "extreme" ) || AsciiEqI( s, "f2" ) )
        {
            SetFixture( GeoFixture::Torture );
            return true;
        }
        if ( AsciiEqI( s, "baseline" ) || AsciiEqI( s, "stream-floor" )
          || AsciiEqI( s, "worldgen-baseline" ) || AsciiEqI( s, "f0" ) )
        {
            SetFixture( GeoFixture::Baseline );
            return true;
        }
        return false;
    }

    inline GeoFixture CycleFixture()
    {
        GeoFixture const n = ( Fixture() == GeoFixture::Range )
            ? GeoFixture::Torture : GeoFixture::Range;
        SetFixture( n );
        return n;
    }

    enum class Province : uint8_t
    {
        Shield = 0,       // ancient crystalline
        MountainBelt,     // uplifted resistant
        SedimentaryBasin, // soft fill
        VolcanicProvince,
        Plateau,
        CoastalShelf,
        InteriorPlain,
        Count
    };

    enum class RockBody : uint8_t
    {
        DirtMantle = 0,
        Loam,
        Sand,
        Gravel,
        Clay,
        Sandstone,
        Shale,
        Limestone,
        Granite,
        Basalt,
        MicaSchist,
        Count
    };

    struct MaterialAttrs
    {
        char const* id;
        float densityGPerL;     // approx bulk for handful credit
        float cohesion;         // 0 loose … 1 holds cuts
        float erosionResist;    // drives relief survival
        float infiltrate;       // hydrology hint (analytic)
        RockBody body;
    };

    inline MaterialAttrs const& Attrs( RockBody b )
    {
        static MaterialAttrs const kTab[(int)RockBody::Count] = {
            { "dirt",       1250.f, 0.35f, 0.25f, 0.45f, RockBody::DirtMantle },
            { "loam",       1200.f, 0.40f, 0.28f, 0.50f, RockBody::Loam },
            { "sand",       1600.f, 0.08f, 0.18f, 0.85f, RockBody::Sand },
            { "gravel",     1700.f, 0.12f, 0.35f, 0.90f, RockBody::Gravel },
            { "clay",       1400.f, 0.75f, 0.30f, 0.12f, RockBody::Clay },
            { "sandstone",  2200.f, 0.70f, 0.72f, 0.35f, RockBody::Sandstone },
            { "shale",      2400.f, 0.55f, 0.40f, 0.15f, RockBody::Shale },
            { "limestone",  2500.f, 0.80f, 0.68f, 0.40f, RockBody::Limestone },
            { "granite",    2700.f, 0.95f, 0.95f, 0.05f, RockBody::Granite },
            { "basalt",     2900.f, 0.92f, 0.90f, 0.08f, RockBody::Basalt },
            { "mica_schist",2800.f, 0.78f, 0.75f, 0.20f, RockBody::MicaSchist }, // H2H: 5469 g/voxel
        };
        return kTab[(int)b];
    }

    inline char const* CapId( RockBody b ) { return Attrs( b ).id; }

    struct WorldSeed
    {
        uint64_t seed = 0xC0FFEEULL ^ 0x50524F56ULL; // "PROV"
        int generatorVersion = kGeneratorVersion;
        char const* generatorId = kGeneratorId;
        bool ready = false;
    };

    inline uint64_t HashMix( uint64_t x )
    {
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return x;
    }

    inline uint64_t Hash2( int64_t ix, int64_t iy, uint64_t salt )
    {
        uint64_t h = salt;
        h = HashMix( h ^ (uint64_t)(uint32_t)ix );
        h = HashMix( h ^ (uint64_t)(uint32_t)iy * 0x9e3779b97f4a7c15ULL );
        return h;
    }

    inline float Hash01( int64_t ix, int64_t iy, uint64_t salt )
    {
        return (float)( Hash2( ix, iy, salt ) & 0xFFFFFFULL ) / (float)0x1000000;
    }

    inline float Smoothstep( float t )
    {
        t = t < 0.f ? 0.f : ( t > 1.f ? 1.f : t );
        return t * t * ( 3.f - 2.f * t );
    }

    inline float Lerp( float a, float b, float t ) { return a + ( b - a ) * t; }

    // Value noise on absolute meter lattice — seam-free across any request region.
    inline float ValueNoise2( double x, double y, uint64_t salt, double period )
    {
        double const sx = x / period;
        double const sy = y / period;
        int64_t const x0 = (int64_t)std::floor( sx );
        int64_t const y0 = (int64_t)std::floor( sy );
        float const tx = Smoothstep( (float)( sx - (double)x0 ) );
        float const ty = Smoothstep( (float)( sy - (double)y0 ) );
        float const n00 = Hash01( x0, y0, salt );
        float const n10 = Hash01( x0 + 1, y0, salt );
        float const n01 = Hash01( x0, y0 + 1, salt );
        float const n11 = Hash01( x0 + 1, y0 + 1, salt );
        float const a = Lerp( n00, n10, tx );
        float const b = Lerp( n01, n11, tx );
        return Lerp( a, b, ty );
    }

    inline float Fbm( double x, double y, uint64_t salt, double basePeriod, int octaves )
    {
        float sum = 0.f, amp = 0.5f, norm = 0.f;
        double p = basePeriod;
        for ( int i = 0; i < octaves; ++i )
        {
            sum += amp * ValueNoise2( x, y, salt + (uint64_t)i * 0x85ebca77c2b2ae63ULL, p );
            norm += amp;
            amp *= 0.5f;
            p *= 0.5;
        }
        return ( norm > 1e-6f ) ? ( sum / norm ) : 0.5f;
    }

    inline WorldSeed& State()
    {
        static WorldSeed s;
        return s;
    }

    inline void SetSeedU64( uint64_t seed )
    {
        WorldSeed& s = State();
        s.seed = seed ? seed : 0xC0FFEEULL;
        s.generatorVersion = kGeneratorVersion;
        s.generatorId = kGeneratorId;
        s.ready = true;
    }

    inline void SetSeedFromIdentity( std::string const& worldIdentityHash,
        std::string const& generatorId, int generatorVersion )
    {
        uint64_t h = 0x50524F56454EULL; // PROVEN
        for ( unsigned char c : worldIdentityHash ) { h = HashMix( h ^ c ); }
        for ( unsigned char c : generatorId ) { h = HashMix( h ^ (uint64_t)( c + 17 ) ); }
        h = HashMix( h ^ (uint64_t)(uint32_t)generatorVersion );
        if ( !worldIdentityHash.empty() || !generatorId.empty() )
        {
            SetSeedU64( h );
        }
        else if ( !State().ready )
        {
            SetSeedU64( 0xE50A7E81CAULL ); // default Esoterica planet
        }
        (void)generatorId;
    }

    inline void EnsureReady()
    {
        if ( !State().ready ) { SetSeedU64( 0xE50A7E81CAULL ); }
    }

    // Macro bands (m): province structure — not mere elevation noise.
    inline float ProvinceField( double x, double y )
    {
        EnsureReady();
        uint64_t const s = State().seed;
        float p = 0.f;
        p += 0.45f * Fbm( x, y, s ^ 0x1111ULL, 384.0, 3 );
        p += 0.30f * Fbm( x, y, s ^ 0x2222ULL, 192.0, 3 );
        p += 0.25f * Fbm( x, y, s ^ 0x3333ULL, 144.0, 2 );
        return p; // ~0..1
    }

    inline Province ClassifyProvince( float field )
    {
        if ( field < 0.18f ) { return Province::CoastalShelf; }
        if ( field < 0.30f ) { return Province::SedimentaryBasin; }
        if ( field < 0.42f ) { return Province::InteriorPlain; }
        if ( field < 0.55f ) { return Province::Plateau; }
        if ( field < 0.68f ) { return Province::Shield; }
        if ( field < 0.82f ) { return Province::MountainBelt; }
        return Province::VolcanicProvince;
    }

    // Forward (RangeProvinceAt defined with F1 block below).
    inline Province RangeProvinceAt( double x, double y );

    inline Province ProvinceAt( double x, double y )
    {
        if ( Fixture() == GeoFixture::Range ) { return RangeProvinceAt( x, y ); }
        return ClassifyProvince( ProvinceField( x, y ) );
    }

    inline char const* ProvinceName( Province p )
    {
        switch ( p )
        {
        case Province::CoastalShelf: return "coastal_shelf";
        case Province::SedimentaryBasin: return "sedimentary_basin";
        case Province::InteriorPlain: return "interior_plain";
        case Province::Plateau: return "plateau";
        case Province::Shield: return "shield";
        case Province::MountainBelt: return "mountain_belt";
        case Province::VolcanicProvince: return "volcanic";
        default: return "unknown";
        }
    }

    inline char const* RockName( RockBody b )
    {
        return CapId( b );
    }

    // ---- F2 D2 TORTURE TERRAIN (prior extreme path; unchanged math) ----

    inline RockBody TortureRockAt( double x, double y )
    {
        EnsureReady();
        uint64_t const s = State().seed;
        Province const prov = ClassifyProvince( ProvinceField( x, y ) );
        float const meso = Fbm( x, y, s ^ 0x4444ULL, 34.0, 3 );
        float const ridge = Fbm( x, y, s ^ 0x5555ULL, 15.0, 2 );
        float const hill = Fbm( x, y, s ^ 0x6666ULL, 7.0, 2 );
        float const pick = 0.5f * meso + 0.3f * ridge + 0.2f * hill;

        switch ( prov )
        {
        case Province::Shield:
            return ( pick > 0.55f ) ? RockBody::Granite : RockBody::MicaSchist;
        case Province::MountainBelt:
            if ( pick > 0.65f ) { return RockBody::Granite; }
            if ( pick > 0.40f ) { return RockBody::Sandstone; }
            return RockBody::Shale;
        case Province::SedimentaryBasin:
            if ( pick < 0.30f ) { return RockBody::Clay; }
            if ( pick < 0.55f ) { return RockBody::Sand; }
            return RockBody::Shale;
        case Province::VolcanicProvince:
            return ( pick > 0.45f ) ? RockBody::Basalt : RockBody::Gravel;
        case Province::Plateau:
            return ( pick > 0.50f ) ? RockBody::Limestone : RockBody::Sandstone;
        case Province::CoastalShelf:
            return ( pick < 0.40f ) ? RockBody::Sand : RockBody::Gravel;
        case Province::InteriorPlain:
        default:
            if ( pick < 0.35f ) { return RockBody::Loam; }
            if ( pick < 0.60f ) { return RockBody::DirtMantle; }
            if ( pick < 0.78f ) { return RockBody::Clay; }
            return RockBody::Gravel;
        }
    }

    // Geology creates relief: resistant bodies high, weak bodies low (+ drainage carve cue).
    // This is the brutal prior field — keep for F2 torture only.
    inline float TortureReliefM( double x, double y )
    {
        EnsureReady();
        uint64_t const s = State().seed;
        float const provF = ProvinceField( x, y );
        RockBody const rock = TortureRockAt( x, y );
        float const resist = Attrs( rock ).erosionResist;

        // Macro uplift / basin from province field
        float z = ( provF - 0.45f ) * 28.f;

        // Resistant bodies survive as ridges; soft bodies become valleys
        float const ridgePulse = Fbm( x, y, s ^ 0x7777ULL, 34.0, 3 );
        z += ( resist - 0.5f ) * 14.f * ( 0.55f + 0.45f * ridgePulse );

        // Drainage accumulation proxy — shallow valley incision (terrain feature before water)
        float const drain = Fbm( x + 17.0, y - 9.0, s ^ 0x8888ULL, 48.0, 3 );
        float const channel = Smoothstep( 1.f - std::fabs( drain - 0.5f ) * 2.f );
        z -= channel * ( 1.2f + ( 1.f - resist ) * 2.5f );

        // Fine hill detail (not gravel-scale noise deciding continents)
        z += ( Fbm( x, y, s ^ 0x9999ULL, 7.0, 2 ) - 0.5f ) * 1.8f;

        return z;
    }

    // ---- F1 PROVENANCE GEOLOGY RANGE (representative local flank transect) ----
    // u = x-128 along walk; v = y-128 across. Player-scale horizontal run vs rise.
    // A flat → B grassy slope → C mound → D drainage → E rocky hill → F diagonal bed → G local cliff
    // (+ I scree apron, J limestone shoulder when cheap).

    inline float RangeCorridorW( double v )
    {
        float const av = (float)std::fabs( v );
        // Full strength |v|<14 m; fade by ~22 m — local corridor, not arena-wide cliff wall.
        if ( av <= 14.f ) { return 1.f; }
        if ( av >= 22.f ) { return 0.f; }
        return 1.f - Smoothstep( ( av - 14.f ) / 8.f );
    }

    inline float RangeBand( float u, float a, float b, float edge )
    {
        float const e = ( edge > 0.05f ) ? edge : 0.05f;
        return Smoothstep( ( u - a ) / e ) * ( 1.f - Smoothstep( ( u - ( b - e ) ) / e ) );
    }

    inline float RangeReliefM( double x, double y )
    {
        EnsureReady();
        uint64_t const s = State().seed;
        float const u = (float)( x - kRangeOriginX );
        float const v = (float)( y - kRangeOriginY );
        float const w = RangeCorridorW( v );

        // Gentle mountain-flank continuum (≈2°), mild detail — never whole-mountain-in-pad.
        float z = u * 0.035f;
        z += ( Fbm( x, y, s ^ 0xA101ULL, 56.0, 3 ) - 0.5f ) * 0.55f;
        z += ( Fbm( x, y, s ^ 0xA102ULL, 14.0, 2 ) - 0.5f ) * 0.16f;

        float authored = 0.f;

        // A — flat depositional pad near spawn: cancel flank slope locally.
        float const aFlat = RangeBand( u, -10.f, 12.f, 4.f );
        authored += aFlat * ( -u * 0.035f );

        // B — shallow grassy slope (extra gentle rise after pad).
        float const bSlope = RangeBand( u, 10.f, 28.f, 3.f );
        authored += bSlope * ( ( u - 10.f ) * 0.055f );

        // C — rounded earth mound.
        {
            float const cu = ( u - 36.f ) / 7.5f;
            float const cv = v / 9.f;
            float const mound = std::exp( -0.5f * ( cu * cu + cv * cv ) );
            authored += mound * 1.35f * ( 0.4f + 0.6f * w );
        }

        // D — eroded drainage cut crossing the corridor.
        {
            float const along = u - 51.f;
            float const channel = ( 1.f - Smoothstep( std::fabs( along ) / 3.2f ) )
                * ( 0.55f + 0.45f * w );
            authored -= channel * 1.15f;
        }

        // E — rocky hillside (steeper but walkable rise).
        {
            float const t = Smoothstep( ( u - 58.f ) / 18.f ) - Smoothstep( ( u - 78.f ) / 4.f );
            authored += t * 3.4f * ( 0.35f + 0.65f * w );
        }

        // F — exposed diagonal bedrock shoulder (strike tilts with v).
        {
            float const plane = u + 0.55f * v - 80.f;
            float const face = Smoothstep( plane / 7.f ) - Smoothstep( ( plane - 9.f ) / 3.f );
            authored += face * 2.3f * ( 0.25f + 0.75f * w );
        }

        // G — true near-vertical local cliff (≈4.2 m over ≈1.3 m run), not arena-wide.
        {
            float const face = Smoothstep( ( u - 93.4f ) / 1.3f );
            authored += face * 4.2f * ( 0.2f + 0.8f * w );
        }

        // I — scree / talus apron at cliff base (cheap broken rock pile).
        {
            float const apron = RangeBand( u, 88.f, 93.6f, 2.f )
                * ( 1.f - Smoothstep( std::fabs( v ) / 10.f ) );
            authored += apron * 0.55f;
        }

        // J — small mineralized limestone shoulder above cliff.
        {
            float const ju = ( u - 102.f ) / 4.f;
            float const jv = ( v + 3.f ) / 3.5f;
            float const knob = std::exp( -0.5f * ( ju * ju + jv * jv ) );
            authored += knob * 0.85f;
        }

        z += authored * ( 0.30f + 0.70f * w );
        return z;
    }

    inline RockBody RangeRockAt( double x, double y )
    {
        EnsureReady();
        uint64_t const s = State().seed;
        float const u = (float)( x - kRangeOriginX );
        float const v = (float)( y - kRangeOriginY );
        float const w = RangeCorridorW( v );
        float const n = Fbm( x, y, s ^ 0xB201ULL, 9.0, 2 );

        if ( w < 0.2f )
        {
            // Soft flank outside the walk corridor.
            if ( n < 0.45f ) { return RockBody::Loam; }
            if ( n < 0.75f ) { return RockBody::DirtMantle; }
            return RockBody::Gravel;
        }

        // A/B gentle floor — soil mantle (cap may paint grass).
        if ( u < 28.f ) { return ( n < 0.55f ) ? RockBody::Loam : RockBody::DirtMantle; }
        // C mound — dirt/loam.
        if ( u < 44.f ) { return RockBody::Loam; }
        // D drainage — clay thalweg, gravel banks.
        if ( u < 58.f )
        {
            float const along = std::fabs( u - 51.f );
            return ( along < 2.2f ) ? RockBody::Clay : RockBody::Gravel;
        }
        // E rocky hillside — thinner soil → gravel / sandstone.
        if ( u < 78.f ) { return ( n > 0.5f ) ? RockBody::Sandstone : RockBody::Gravel; }
        // F diagonal bedrock — limestone / sandstone exposure.
        if ( u < 92.f ) { return ( n > 0.42f ) ? RockBody::Limestone : RockBody::Sandstone; }
        // G cliff face — hard rock.
        if ( u < 100.f ) { return ( n > 0.55f ) ? RockBody::Granite : RockBody::MicaSchist; }
        // J shoulder — mineralized limestone outcrop.
        if ( u < 108.f && std::fabs( v + 3.f ) < 5.f ) { return RockBody::Limestone; }
        return ( n > 0.5f ) ? RockBody::Sandstone : RockBody::Gravel;
    }

    inline Province RangeProvinceAt( double x, double y )
    {
        float const u = (float)( x - kRangeOriginX );
        float const w = RangeCorridorW( y - kRangeOriginY );
        if ( w > 0.25f && u >= 78.f ) { return Province::MountainBelt; }
        if ( w > 0.25f && u >= 58.f ) { return Province::Plateau; }
        if ( w > 0.25f && u >= 44.f && u < 58.f ) { return Province::SedimentaryBasin; }
        return Province::InteriorPlain;
    }

    inline char const* RangeCapAt( double x, double y, RockBody rock )
    {
        float const u = (float)( x - kRangeOriginX );
        float const v = (float)( y - kRangeOriginY );
        float const w = RangeCorridorW( v );
        // Grassy cover on gentle A/B floor; scree reads gravel at cliff base.
        if ( w > 0.3f && u < 28.f
          && ( rock == RockBody::Loam || rock == RockBody::DirtMantle ) )
        {
            return "grass";
        }
        if ( w > 0.35f && u >= 88.f && u < 93.8f ) { return "gravel"; }
        return CapId( rock );
    }

    // Dominant rock body from province + meso structure (34/15/7 m tectonic cues).
    inline RockBody RockAt( double x, double y )
    {
        if ( Fixture() == GeoFixture::Range ) { return RangeRockAt( x, y ); }
        return TortureRockAt( x, y );
    }

    // Geology → relief meters. Fixture selects representative flank vs torture FBM.
    inline float AnalyticReliefM( double x, double y )
    {
        if ( Fixture() == GeoFixture::Range ) { return RangeReliefM( x, y ); }
        return TortureReliefM( x, y );
    }

    // Map analytic meters → grade using the same relief/datum law the client uses for Z.
    inline float GradeFromReliefM( float reliefM, float gradeDatum, float reliefVoxels, float voxelEdgeM )
    {
        float const scale = (std::max)( 0.05f, reliefVoxels * voxelEdgeM );
        float grade = gradeDatum + reliefM / scale;
        if ( grade < 0.05f ) { grade = 0.05f; }
        if ( grade > 0.98f ) { grade = 0.98f; }
        return grade;
    }

    struct SurfaceSample
    {
        float grade = 0.85f;
        float reliefM = 0.f;
        RockBody rock = RockBody::DirtMantle;
        Province province = Province::InteriorPlain;
        char const* cap = "dirt";
    };

    inline SurfaceSample SampleSurface( double x, double y,
        float gradeDatum, float reliefVoxels, float voxelEdgeM )
    {
        SurfaceSample o;
        if ( Fixture() == GeoFixture::Baseline )
        {
            // Deliberately constant-cost certification floor: no FBM, geology, morphology,
            // water, or authored relief.  Keep the current HF/stream packaging around it.
            (void)x; (void)y; (void)reliefVoxels; (void)voxelEdgeM;
            o.province = Province::InteriorPlain;
            o.rock = RockBody::DirtMantle;
            o.cap = "dirt";
            o.reliefM = 0.f;
            o.grade = gradeDatum;
            return o;
        }
        o.province = ProvinceAt( x, y );
        o.rock = RockAt( x, y );
        o.cap = ( Fixture() == GeoFixture::Range )
            ? RangeCapAt( x, y, o.rock )
            : CapId( o.rock );
        o.reliefM = AnalyticReliefM( x, y );
        o.grade = GradeFromReliefM( o.reliefM, gradeDatum, reliefVoxels, voxelEdgeM );
        return o;
    }

    // Stratum at depth below crest (meters) — excavation discovery chain (analytic).
    inline RockBody StratumAt( double x, double y, float depthBelowCrestM )
    {
        RockBody surface = RockAt( x, y );
        if ( depthBelowCrestM < 0.15f ) { return surface; }
        if ( depthBelowCrestM < 0.45f )
        {
            // soil profile transition
            if ( surface == RockBody::Granite || surface == RockBody::Basalt
              || surface == RockBody::Sandstone || surface == RockBody::Limestone
              || surface == RockBody::Shale || surface == RockBody::MicaSchist )
            {
                return surface;
            }
            return RockBody::Clay;
        }
        if ( depthBelowCrestM < 1.2f )
        {
            Province const p = ProvinceAt( x, y );
            if ( p == Province::SedimentaryBasin ) { return RockBody::Shale; }
            if ( p == Province::CoastalShelf ) { return RockBody::Sand; }
            if ( p == Province::VolcanicProvince ) { return RockBody::Basalt; }
            if ( p == Province::MountainBelt || p == Province::Shield ) { return surface; }
            return RockBody::Sandstone;
        }
        // Deep: basement / host
        Province const p = ProvinceAt( x, y );
        if ( p == Province::VolcanicProvince ) { return RockBody::Basalt; }
        if ( p == Province::Shield || p == Province::MountainBelt ) { return RockBody::Granite; }
        if ( p == Province::Plateau ) { return RockBody::Limestone; }
        return RockBody::Shale;
    }
}
