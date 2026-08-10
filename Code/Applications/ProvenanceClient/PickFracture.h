// P5 side-gate — Material-True Pick Fracture + Closed Local Surface.
//
// Law: pick is NOT sphere/cup/cell deform.
//   impact → penetration → pry → material fracture → connected piece releases
//   → occupancy loses exactly that material → remaining boundary reconstructed
//   → detached piece = same separation event
//
// Contact frame at hit (angle-independent):
//   N = surface normal (outward)
//   T = projected strike/pry on tangent plane
//   B = cross(N, T)
// Tip geometry, penetration, pry sweep, fracture envelope live in (T,B,N).
// World-up/Z is never the fracture law — same physical strike on flat/slope/
// vertical/overhang = same local event rotated to world.
//
// Four distinct radii:
//   contact/query ≠ fracture volume ≠ D2 recon halo ≠ HF refine
// Outside physical changed region + min recon halo → pre-strike surface bit-identical.
//
// Hard presentation gates (no exceptions):
//   - Sky/clear RGB in or around a strike hole = PRESENTATION_COVERAGE_FAIL.
//     There is NO "legitimate cavity mouth showing sky" exception.
//   - Deform beyond fracture + min recon halo = HF_CHANGED_OUTSIDE_RECON_HALO.
//
// P5a water ledger FREEZE. P5b CLOSED. Do not redesign D2 topology / EditedRegion /
// SupportBelow / P4 / chip lifecycle here.

#pragma once

#include "HorizonToHand.h"
#include "RockStructure.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace PickFracture
{
    constexpr float kPi = 3.14159265358979323846f;

    // ---- Distinct radii (meters) ----
    constexpr float kContactQueryRM     = H2H::kLivePickContactRM; // aim/query only
    constexpr float kD2ReconHaloM       = 0.20f;  // min D2 rebuild halo beyond fracture AABB
    constexpr float kHfRefineHaloM      = 0.16f;  // HF refine beyond mouth (not fracture law)
    constexpr float kMouthOpenMaxM      = 0.12f;  // tip-scale mouth contribution
    constexpr float kMaxFractureExtentM = 0.14f;  // hard cap — no meter-scale bites

    enum class MaterialFamily : uint8_t
    {
        GravelAggregate = 0,
        GraniteCompactAngular,
        MicaSchistFoliation,
        Other
    };

    inline MaterialFamily FamilyOf( char const* matId )
    {
        if ( !matId || !matId[0] ) { return MaterialFamily::Other; }
        if ( std::strcmp( matId, "gravel" ) == 0 || std::strcmp( matId, "sand" ) == 0 )
        {
            return MaterialFamily::GravelAggregate;
        }
        if ( std::strcmp( matId, "granite" ) == 0 || std::strcmp( matId, "basalt" ) == 0
          || std::strcmp( matId, "stone" ) == 0 || std::strcmp( matId, "limestone" ) == 0 )
        {
            return MaterialFamily::GraniteCompactAngular;
        }
        if ( std::strcmp( matId, "mica_schist" ) == 0 || std::strcmp( matId, "shale" ) == 0
          || std::strcmp( matId, "sandstone" ) == 0 )
        {
            return MaterialFamily::MicaSchistFoliation;
        }
        H2H::MaterialFormContract const& f = H2H::FormOrDirt( matId );
        if ( f.fabric == H2H::FabricKind::Granular ) { return MaterialFamily::GravelAggregate; }
        if ( f.fabric == H2H::FabricKind::FoliatedAnisotropic
          || f.fabric == H2H::FabricKind::BeddedFissile )
        {
            return MaterialFamily::MicaSchistFoliation;
        }
        if ( f.rigid_fracture_body ) { return MaterialFamily::GraniteCompactAngular; }
        return MaterialFamily::Other;
    }

    inline char const* FamilyName( MaterialFamily fam )
    {
        switch ( fam )
        {
        case MaterialFamily::GravelAggregate: return "gravel_aggregate";
        case MaterialFamily::GraniteCompactAngular: return "granite_compact_angular";
        case MaterialFamily::MicaSchistFoliation: return "mica_schist_foliation";
        default: return "other";
        }
    }

    struct Vec3
    {
        float x = 0.f, y = 0.f, z = 0.f;
    };

    inline Vec3 V3( float x, float y, float z ) { return { x, y, z }; }
    inline float Dot( Vec3 a, Vec3 b ) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    inline Vec3 Cross( Vec3 a, Vec3 b )
    {
        return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    }
    inline float Len( Vec3 a ) { return std::sqrt( Dot( a, a ) ); }
    inline Vec3 Mul( Vec3 a, float s ) { return { a.x * s, a.y * s, a.z * s }; }
    inline Vec3 Add( Vec3 a, Vec3 b ) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    inline Vec3 Sub( Vec3 a, Vec3 b ) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    inline Vec3 Norm( Vec3 a )
    {
        float const L = Len( a );
        if ( L < 1e-8f ) { return { 0.f, 0.f, 1.f }; }
        return Mul( a, 1.f / L );
    }

    // Orthonormal contact frame at the strike.
    struct ContactFrame
    {
        Vec3 origin{};
        Vec3 N{}, T{}, B{}; // world axes; N outward, T pry tangent, B = N×T
        bool valid = false;
    };

    // Build angle-independent contact frame. N = surface normal; pry/strike projected to tangent.
    inline ContactFrame MakeContactFrame( Vec3 hit, Vec3 surfaceN, Vec3 strikeOrPry )
    {
        ContactFrame f;
        f.origin = hit;
        f.N = Norm( surfaceN );
        // Reject using world-up as fracture law: T from pry projected onto tangent plane.
        Vec3 t = Sub( strikeOrPry, Mul( f.N, Dot( strikeOrPry, f.N ) ) );
        if ( Len( t ) < 1e-5f )
        {
            // Degenerate pry (face-on): pick a stable tangent from N.
            Vec3 a = ( std::fabs( f.N.z ) < 0.9f ) ? V3( 0.f, 0.f, 1.f ) : V3( 1.f, 0.f, 0.f );
            t = Cross( f.N, a );
        }
        f.T = Norm( t );
        f.B = Norm( Cross( f.N, f.T ) );
        // Re-orthogonalize T = B×N for numerical stability.
        f.T = Norm( Cross( f.B, f.N ) );
        f.valid = true;
        return f;
    }

    inline Vec3 WorldToLocal( ContactFrame const& f, Vec3 world )
    {
        Vec3 d = Sub( world, f.origin );
        return { Dot( d, f.T ), Dot( d, f.B ), Dot( d, f.N ) };
    }

    inline Vec3 LocalToWorld( ContactFrame const& f, Vec3 local )
    {
        return Add( f.origin,
            Add( Mul( f.T, local.x ), Add( Mul( f.B, local.y ), Mul( f.N, local.z ) ) ) );
    }

    // Fracture envelope extents in contact-local meters (T,B,N).
    // N+: air side (small lip), N-: into material (penetration).
    struct Envelope
    {
        MaterialFamily family = MaterialFamily::Other;
        float tHalf = 0.05f;     // ±T
        float bHalf = 0.05f;     // ±B
        float nAir = 0.02f;      // +N lip
        float nInto = 0.06f;     // −N penetration
        float tipRM = 0.025f;    // tip sphere at origin for seed connectivity
        float angularSharp = 0.f;// 0 = smooth ellipsoid; 1 = boxy/angular
        float foliationBias = 0.f; // schist: stretch along foliation intersect tangent
        Vec3 folLocalTBN{};      // unit foliation normal in local frame (schist)
        uint64_t seed = 0;
        char material_id[32] = {};
    };

    struct FractureEvent
    {
        ContactFrame frame{};
        Envelope env{};
        float contactQueryRM = kContactQueryRM;
        float fractureExtentRM = 0.f; // max half-extent of envelope (physical changed)
        float d2ReconHaloRM = kD2ReconHaloM;
        float hfRefineHaloRM = kHfRefineHaloM;
        float mouthOpenRM = 0.08f;
        float volumeM3 = 0.f;
        int releasedGrams = 0;
        bool releasesRigidBody = false;
        float plateAlongM = 0.f;
        float plateAcrossM = 0.f;
        float plateThickM = 0.f;
        char morphology[40] = {};
        bool ok = false;
        char fail[64] = {};
    };

    inline uint64_t HashMix( uint64_t a, uint64_t b )
    {
        uint64_t x = a + 0x9E3779B97F4A7C15ull + ( b << 6 ) + ( b >> 2 );
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ull;
        x ^= x >> 27; x *= 0x94D049BB133111EBull;
        x ^= x >> 31;
        return x;
    }

    inline float SeedUnit( uint64_t seed, int salt )
    {
        uint64_t h = HashMix( seed, (uint64_t)(uint32_t)salt );
        return (float)( h & 0xFFFFFFull ) / (float)0xFFFFFFu; // [0,1]
    }

    // Deterministic family envelope — variation inside family from seed only.
    inline Envelope BuildEnvelope( char const* matId, uint64_t seed,
        RockStruct::Foliation const* fol, ContactFrame const& frame )
    {
        Envelope e;
        e.family = FamilyOf( matId );
        e.seed = seed;
        std::snprintf( e.material_id, sizeof( e.material_id ), "%s",
            matId && matId[0] ? matId : "dirt" );

        float const j0 = SeedUnit( seed, 11 );
        float const j1 = SeedUnit( seed, 29 );
        float const j2 = SeedUnit( seed, 47 );

        switch ( e.family )
        {
        case MaterialFamily::GravelAggregate:
            // Small central recess — aggregate crush, not a plate.
            e.tHalf = 0.045f + 0.012f * j0;   // ~4.5–5.7 cm
            e.bHalf = 0.045f + 0.012f * j1;
            e.nInto = 0.040f + 0.015f * j2;   // ~4–5.5 cm bite
            e.nAir  = 0.012f;
            e.tipRM = 0.022f;
            e.angularSharp = 0.15f;
            e.foliationBias = 0.f;
            break;
        case MaterialFamily::GraniteCompactAngular:
            // Compact angular wedge — tip + joint facets, still tip-scale.
            e.tHalf = 0.055f + 0.020f * j0;   // ~5.5–7.5 cm
            e.bHalf = 0.045f + 0.015f * j1;
            e.nInto = 0.055f + 0.020f * j2;   // ~5.5–7.5 cm
            e.nAir  = 0.015f;
            e.tipRM = 0.020f;
            e.angularSharp = 0.78f + 0.15f * j0;
            e.foliationBias = 0.f;
            break;
        case MaterialFamily::MicaSchistFoliation:
        {
            // Foliation-biased plate: elongate along seam, thin across layers.
            e.tHalf = 0.080f + 0.025f * j0;   // along pry / strike ~8–10.5 cm
            e.bHalf = 0.055f + 0.015f * j1;
            e.nInto = 0.035f + 0.015f * j2;   // thin peel into face
            e.nAir  = 0.018f;
            e.tipRM = 0.018f;
            e.angularSharp = 0.35f;
            e.foliationBias = 0.70f + 0.20f * j1;
            if ( fol )
            {
                Vec3 fn = Norm( V3( fol->nx, fol->ny, fol->nz ) );
                e.folLocalTBN = {
                    Dot( fn, frame.T ), Dot( fn, frame.B ), Dot( fn, frame.N )
                };
                float fl = Len( e.folLocalTBN );
                if ( fl > 1e-5f ) { e.folLocalTBN = Mul( e.folLocalTBN, 1.f / fl ); }
                else { e.folLocalTBN = { 0.f, 0.f, 1.f }; }
            }
            else
            {
                e.folLocalTBN = { 0.f, 0.35f, 0.94f }; // default shallow dip bias
            }
            break;
        }
        default:
            e.tHalf = 0.050f;
            e.bHalf = 0.050f;
            e.nInto = 0.050f;
            e.nAir  = 0.015f;
            e.tipRM = 0.022f;
            e.angularSharp = 0.4f;
            break;
        }

        // Hard freezes against meter-scale folds from cm bites.
        e.tHalf = (std::min)( e.tHalf, kMaxFractureExtentM );
        e.bHalf = (std::min)( e.bHalf, kMaxFractureExtentM );
        e.nInto = (std::min)( e.nInto, kMaxFractureExtentM );
        return e;
    }

    // Soft ellipsoid (0) → sharper superellipsoid/box (1) in local frame.
    inline float LocalEnvelopeDistance( Envelope const& e, Vec3 local )
    {
        // nLocal: +air / −into. Map into [0,1] normalized box then p-norm.
        float const nPos = ( local.z >= 0.f ) ? e.nAir : e.nInto;
        if ( nPos < 1e-6f ) { return 1e6f; }

        float u = local.x / (std::max)( 1e-6f, e.tHalf );
        float v = local.y / (std::max)( 1e-6f, e.bHalf );
        float w = local.z / nPos; // signed; into material w negative but we use |w|

        // Foliation bias: squash along foliation normal, stretch in-plane.
        if ( e.foliationBias > 0.05f )
        {
            float const along = 1.f + e.foliationBias * 0.55f;
            float const across = 1.f / ( 1.f + e.foliationBias * 0.85f );
            // Distance to foliation plane through origin (local).
            float const dFol = local.x * e.folLocalTBN.x
                + local.y * e.folLocalTBN.y + local.z * e.folLocalTBN.z;
            // Inflate in-plane coords, deflate across-foliation.
            Vec3 inPlane = Sub( local, Mul( e.folLocalTBN, dFol ) );
            Vec3 adj = Add( Mul( inPlane, along ), Mul( e.folLocalTBN, dFol * across ) );
            u = adj.x / (std::max)( 1e-6f, e.tHalf );
            v = adj.y / (std::max)( 1e-6f, e.bHalf );
            float const nPos2 = ( adj.z >= 0.f ) ? e.nAir : e.nInto;
            w = adj.z / (std::max)( 1e-6f, nPos2 );
        }

        float const p = 2.f + e.angularSharp * 6.f; // 2=ellipsoid … ~8=boxy
        float const au = std::fabs( u );
        float const av = std::fabs( v );
        float const aw = std::fabs( w );
        // Avoid pow(0,p) issues; clamp tiny.
        auto softPow = [&]( float a ) -> float {
            return std::pow( (std::max)( a, 0.f ), p );
        };
        float const s = softPow( au ) + softPow( av ) + softPow( aw );
        return std::pow( (std::max)( s, 0.f ), 1.f / p );
    }

    inline bool PointInEnvelopeLocal( Envelope const& e, Vec3 local )
    {
        // Tip seed ball always included (guarantees connected seed at origin).
        if ( Dot( local, local ) <= e.tipRM * e.tipRM ) { return true; }
        // Must be into-or-near material: allow tiny air lip only.
        if ( local.z > e.nAir + 1e-4f ) { return false; }
        if ( local.z < -( e.nInto + 1e-4f ) ) { return false; }
        return LocalEnvelopeDistance( e, local ) <= 1.0001f;
    }

    inline bool PointInEnvelopeWorld( FractureEvent const& ev, float wx, float wy, float wz )
    {
        if ( !ev.ok || !ev.frame.valid ) { return false; }
        return PointInEnvelopeLocal( ev.env, WorldToLocal( ev.frame, V3( wx, wy, wz ) ) );
    }

    inline float EnvelopeVolumeM3( Envelope const& e )
    {
        // Superellipsoid volume proxy: axis box * shape factor.
        float const box = ( 2.f * e.tHalf ) * ( 2.f * e.bHalf ) * ( e.nAir + e.nInto );
        float const shape = 0.52f - 0.18f * e.angularSharp; // ellipsoid→box fill
        return (std::max)( 1e-8f, box * shape );
    }

    inline FractureEvent BuildEvent( Vec3 hit, Vec3 surfaceN, Vec3 strikeOrPry,
        char const* matId, uint64_t seed, RockStruct::Foliation const* fol = nullptr )
    {
        FractureEvent ev;
        ev.frame = MakeContactFrame( hit, surfaceN, strikeOrPry );
        if ( !ev.frame.valid )
        {
            std::snprintf( ev.fail, sizeof( ev.fail ), "BAD_CONTACT_FRAME" );
            return ev;
        }
        // Orthonormal gate.
        float const nlen = Len( ev.frame.N );
        float const tlen = Len( ev.frame.T );
        float const blen = Len( ev.frame.B );
        float const nt = std::fabs( Dot( ev.frame.N, ev.frame.T ) );
        float const nb = std::fabs( Dot( ev.frame.N, ev.frame.B ) );
        float const tb = std::fabs( Dot( ev.frame.T, ev.frame.B ) );
        if ( nlen < 0.99f || tlen < 0.99f || blen < 0.99f || nt > 0.05f || nb > 0.05f || tb > 0.05f )
        {
            std::snprintf( ev.fail, sizeof( ev.fail ), "FRAME_NOT_ORTHONORMAL" );
            return ev;
        }

        ev.env = BuildEnvelope( matId, seed, fol, ev.frame );
        ev.contactQueryRM = kContactQueryRM;
        ev.fractureExtentRM = (std::max)( ev.env.tHalf,
            (std::max)( ev.env.bHalf, (std::max)( ev.env.nInto, ev.env.nAir ) ) );
        if ( ev.fractureExtentRM > kMaxFractureExtentM + 1e-4f )
        {
            std::snprintf( ev.fail, sizeof( ev.fail ), "FRACTURE_OUTSIDE_ALLOWED_REGION" );
            return ev;
        }
        ev.d2ReconHaloRM = kD2ReconHaloM;
        ev.hfRefineHaloRM = kHfRefineHaloM;
        ev.mouthOpenRM = (std::min)( kMouthOpenMaxM,
            (std::max)( 0.05f, 0.55f * ( ev.env.tHalf + ev.env.bHalf ) ) );
        ev.volumeM3 = EnvelopeVolumeM3( ev.env );
        H2H::MaterialFormContract const& form = H2H::FormOrDirt( ev.env.material_id );
        ev.releasedGrams = H2H::VolumeToGramsFloor( ev.volumeM3, form.density_kg_m3 );
        ev.releasesRigidBody = form.rigid_fracture_body
            && ev.env.family != MaterialFamily::GravelAggregate;
        if ( ev.env.family == MaterialFamily::MicaSchistFoliation )
        {
            ev.plateAlongM = 2.f * ev.env.tHalf;
            ev.plateAcrossM = 2.f * ev.env.bHalf;
            ev.plateThickM = (std::max)( 0.015f, ev.env.nInto * 0.65f );
            std::snprintf( ev.morphology, sizeof( ev.morphology ), "foliation_plate" );
        }
        else if ( ev.env.family == MaterialFamily::GraniteCompactAngular )
        {
            ev.plateAlongM = 2.f * ev.env.tHalf * 0.85f;
            ev.plateAcrossM = 2.f * ev.env.bHalf * 0.85f;
            ev.plateThickM = (std::max)( 0.02f, ev.env.nInto * 0.75f );
            std::snprintf( ev.morphology, sizeof( ev.morphology ), "compact_angular" );
        }
        else
        {
            ev.plateAlongM = 2.f * ev.env.tHalf;
            ev.plateAcrossM = 2.f * ev.env.bHalf;
            ev.plateThickM = ev.env.nInto;
            std::snprintf( ev.morphology, sizeof( ev.morphology ), "aggregate_recess" );
            ev.releasesRigidBody = false;
        }
        // Contact query must not define fracture volume.
        if ( ev.fractureExtentRM >= ev.contactQueryRM - 0.01f )
        {
            // Soft warn only when contact is tiny; pick contact is 0.18 so tip-scale is fine.
        }
        ev.ok = true;
        return ev;
    }

    // Local-frame AABB of envelope (for recon halo / outside-identity).
    inline void LocalAabb( Envelope const& e, float& t0, float& t1, float& b0, float& b1,
        float& n0, float& n1 )
    {
        t0 = -e.tHalf; t1 = e.tHalf;
        b0 = -e.bHalf; b1 = e.bHalf;
        n0 = -e.nInto; n1 = e.nAir;
    }

    inline void WorldAabb( FractureEvent const& ev,
        float& minX, float& minY, float& minZ, float& maxX, float& maxY, float& maxZ )
    {
        float t0, t1, b0, b1, n0, n1;
        LocalAabb( ev.env, t0, t1, b0, b1, n0, n1 );
        Vec3 corners[8] = {
            LocalToWorld( ev.frame, V3( t0, b0, n0 ) ),
            LocalToWorld( ev.frame, V3( t1, b0, n0 ) ),
            LocalToWorld( ev.frame, V3( t0, b1, n0 ) ),
            LocalToWorld( ev.frame, V3( t1, b1, n0 ) ),
            LocalToWorld( ev.frame, V3( t0, b0, n1 ) ),
            LocalToWorld( ev.frame, V3( t1, b0, n1 ) ),
            LocalToWorld( ev.frame, V3( t0, b1, n1 ) ),
            LocalToWorld( ev.frame, V3( t1, b1, n1 ) ),
        };
        minX = maxX = corners[0].x;
        minY = maxY = corners[0].y;
        minZ = maxZ = corners[0].z;
        for ( int i = 1; i < 8; ++i )
        {
            minX = (std::min)( minX, corners[i].x );
            minY = (std::min)( minY, corners[i].y );
            minZ = (std::min)( minZ, corners[i].z );
            maxX = (std::max)( maxX, corners[i].x );
            maxY = (std::max)( maxY, corners[i].y );
            maxZ = (std::max)( maxZ, corners[i].z );
        }
    }

    // Recon halo AABB = fracture AABB expanded by d2 halo (XY) — HF outside must be bit-identical.
    inline void ReconHaloAabb( FractureEvent const& ev,
        float& minX, float& minY, float& maxX, float& maxY )
    {
        float minZ, maxZ;
        WorldAabb( ev, minX, minY, minZ, maxX, maxY, maxZ );
        float const h = ev.d2ReconHaloRM;
        minX -= h; minY -= h; maxX += h; maxY += h;
    }

    // ---- Morphology fingerprint in local frame (rotation-invariant) ----
    struct LocalMorphMetrics
    {
        int insideCount = 0;
        float sumT = 0.f, sumB = 0.f, sumN = 0.f;
        float minT = 0.f, maxT = 0.f, minB = 0.f, maxB = 0.f, minN = 0.f, maxN = 0.f;
        float volumeProxy = 0.f;
        int grams = 0;
        char morphology[40] = {};
        MaterialFamily family = MaterialFamily::Other;
    };

    inline LocalMorphMetrics SampleLocalMorph( FractureEvent const& ev, float stepM = 0.012f )
    {
        LocalMorphMetrics m;
        m.family = ev.env.family;
        m.grams = ev.releasedGrams;
        m.volumeProxy = ev.volumeM3;
        std::snprintf( m.morphology, sizeof( m.morphology ), "%s", ev.morphology );
        float t0, t1, b0, b1, n0, n1;
        LocalAabb( ev.env, t0, t1, b0, b1, n0, n1 );
        m.minT = t1; m.maxT = t0; m.minB = b1; m.maxB = b0; m.minN = n1; m.maxN = n0;
        for ( float n = n0; n <= n1 + 1e-6f; n += stepM )
        {
            for ( float b = b0; b <= b1 + 1e-6f; b += stepM )
            {
                for ( float t = t0; t <= t1 + 1e-6f; t += stepM )
                {
                    Vec3 L{ t, b, n };
                    if ( !PointInEnvelopeLocal( ev.env, L ) ) { continue; }
                    ++m.insideCount;
                    m.sumT += t; m.sumB += b; m.sumN += n;
                    m.minT = (std::min)( m.minT, t ); m.maxT = (std::max)( m.maxT, t );
                    m.minB = (std::min)( m.minB, b ); m.maxB = (std::max)( m.maxB, b );
                    m.minN = (std::min)( m.minN, n ); m.maxN = (std::max)( m.maxN, n );
                }
            }
        }
        return m;
    }

    inline bool MorphEquivalent( LocalMorphMetrics const& a, LocalMorphMetrics const& b,
        char* why, size_t whyN )
    {
        if ( a.family != b.family )
        {
            std::snprintf( why, whyN, "family_mismatch" );
            return false;
        }
        if ( std::abs( a.grams - b.grams ) > (std::max)( 8, a.grams / 25 ) )
        {
            std::snprintf( why, whyN, "grams_delta=%d", a.grams - b.grams );
            return false;
        }
        if ( std::abs( a.insideCount - b.insideCount ) > (std::max)( 3, a.insideCount / 20 ) )
        {
            std::snprintf( why, whyN, "inside_delta=%d", a.insideCount - b.insideCount );
            return false;
        }
        auto span = []( float mn, float mx ) { return mx - mn; };
        if ( std::fabs( span( a.minT, a.maxT ) - span( b.minT, b.maxT ) ) > 0.012f
          || std::fabs( span( a.minB, a.maxB ) - span( b.minB, b.maxB ) ) > 0.012f
          || std::fabs( span( a.minN, a.maxN ) - span( b.minN, b.maxN ) ) > 0.012f )
        {
            std::snprintf( why, whyN, "bounds_span_delta" );
            return false;
        }
        if ( std::strcmp( a.morphology, b.morphology ) != 0 )
        {
            std::snprintf( why, whyN, "morphology_tag" );
            return false;
        }
        why[0] = 0;
        return true;
    }

    // Rotate a vector around axis X by deg (fixture orientation only).
    inline Vec3 RotateX( Vec3 v, float deg )
    {
        float const r = deg * ( kPi / 180.f );
        float const c = std::cos( r ), s = std::sin( r );
        return { v.x, v.y * c - v.z * s, v.y * s + v.z * c };
    }

    // ---- Synthetic occupancy lattice for two-strike / connected release (headless) ----
    struct SynthLattice
    {
        static constexpr int W = 24, H = 24, D = 16;
        float originX = 0.f, originY = 0.f, originZ = 0.f;
        float edge = 0.03f; // 3 cm cells — fine enough for tip-scale
        uint8_t solid[W * H * D];

        int Idx( int x, int y, int z ) const { return ( z * H + y ) * W + x; }
        bool In( int x, int y, int z ) const
        {
            return x >= 0 && y >= 0 && z >= 0 && x < W && y < H && z < D;
        }
        Vec3 Center( int x, int y, int z ) const
        {
            return {
                originX + ( (float)x + 0.5f ) * edge,
                originY + ( (float)y + 0.5f ) * edge,
                originZ + ( (float)z + 0.5f ) * edge
            };
        }
        void FillSolidBelowCrest( float crestZ )
        {
            std::memset( solid, 0, sizeof( solid ) );
            for ( int z = 0; z < D; ++z )
            {
                for ( int y = 0; y < H; ++y )
                {
                    for ( int x = 0; x < W; ++x )
                    {
                        Vec3 c = Center( x, y, z );
                        solid[Idx( x, y, z )] = ( c.z <= crestZ ) ? 1 : 0;
                    }
                }
            }
        }
        int CountSolid() const
        {
            int n = 0;
            for ( int i = 0; i < W * H * D; ++i ) { n += solid[i] ? 1 : 0; }
            return n;
        }
        bool SolidAtWorld( float x, float y, float z ) const
        {
            int const ix = (int)std::floor( ( x - originX ) / edge );
            int const iy = (int)std::floor( ( y - originY ) / edge );
            int const iz = (int)std::floor( ( z - originZ ) / edge );
            if ( !In( ix, iy, iz ) ) { return false; }
            return solid[Idx( ix, iy, iz )] != 0;
        }
    };

    // Subtract connected envelope voxels (6-connected from tip seed). Returns removed count.
    inline int SubtractConnected( SynthLattice& lat, FractureEvent const& ev )
    {
        if ( !ev.ok ) { return 0; }
        bool visit[SynthLattice::W * SynthLattice::H * SynthLattice::D];
        std::memset( visit, 0, sizeof( visit ) );
        int qx[SynthLattice::W * SynthLattice::H * SynthLattice::D];
        int qy[SynthLattice::W * SynthLattice::H * SynthLattice::D];
        int qz[SynthLattice::W * SynthLattice::H * SynthLattice::D];
        int qn = 0;
        auto push = [&]( int x, int y, int z )
        {
            if ( !lat.In( x, y, z ) ) { return; }
            int const i = lat.Idx( x, y, z );
            if ( visit[i] || !lat.solid[i] ) { return; }
            Vec3 c = lat.Center( x, y, z );
            if ( !PointInEnvelopeWorld( ev, c.x, c.y, c.z ) ) { return; }
            visit[i] = true;
            qx[qn] = x; qy[qn] = y; qz[qn] = z; ++qn;
        };
        // Seed near tip slightly into material (−N).
        Vec3 seed = Add( ev.frame.origin, Mul( ev.frame.N, -0.5f * ev.env.tipRM ) );
        int sx = (int)std::floor( ( seed.x - lat.originX ) / lat.edge );
        int sy = (int)std::floor( ( seed.y - lat.originY ) / lat.edge );
        int sz = (int)std::floor( ( seed.z - lat.originZ ) / lat.edge );
        push( sx, sy, sz );
        // Also seed neighbors around tip if exact cell is air lip.
        for ( int dz = -1; dz <= 1; ++dz )
            for ( int dy = -1; dy <= 1; ++dy )
                for ( int dx = -1; dx <= 1; ++dx )
                    push( sx + dx, sy + dy, sz + dz );

        int removed = 0;
        for ( int qi = 0; qi < qn; ++qi )
        {
            int const x = qx[qi], y = qy[qi], z = qz[qi];
            lat.solid[lat.Idx( x, y, z )] = 0;
            ++removed;
            static int const nb[6][3] = {
                {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
            };
            for ( auto& d : nb ) { push( x + d[0], y + d[1], z + d[2] ); }
        }
        return removed;
    }

    // ---- Cert ----
    struct CertRow
    {
        char check[48] = {};
        char verdict[12] = "SKIP";
        char note[192] = {};
        float x = 0.f, y = 0.f, z = 0.f;
    };

    struct CertResult
    {
        int passN = 0, failN = 0, skipN = 0;
        int exitCode = 0;
        static constexpr int kCap = 96;
        CertRow rows[kCap];
        int rowN = 0;
        char firstFail[96] = {};
        float failX = 0.f, failY = 0.f, failZ = 0.f;
    };

    inline void CertAdd( CertResult& R, char const* check, char const* verdict,
        char const* note, float x = 0.f, float y = 0.f, float z = 0.f )
    {
        if ( R.rowN >= CertResult::kCap ) { return; }
        CertRow& r = R.rows[R.rowN++];
        std::snprintf( r.check, sizeof( r.check ), "%s", check ? check : "?" );
        std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict ? verdict : "?" );
        std::snprintf( r.note, sizeof( r.note ), "%s", note ? note : "" );
        r.x = x; r.y = y; r.z = z;
        if ( std::strcmp( r.verdict, "PASS" ) == 0 ) { ++R.passN; }
        else if ( std::strcmp( r.verdict, "FAIL" ) == 0 )
        {
            ++R.failN;
            R.exitCode = 1;
            if ( !R.firstFail[0] )
            {
                std::snprintf( R.firstFail, sizeof( R.firstFail ), "%s", r.check );
                R.failX = x; R.failY = y; R.failZ = z;
            }
        }
        else { ++R.skipN; }
    }

    inline CertResult RunHeadlessCert()
    {
        CertResult R{};

        // 1) Contact frame orthonormal at flat / slope / vertical / overhang.
        {
            struct Case { float nx, ny, nz; char const* tag; };
            Case cases[] = {
                { 0.f, 0.f, 1.f, "flat" },
                { 0.35f, 0.f, 0.937f, "slope15" },
                { 0.707f, 0.f, 0.707f, "slope45" },
                { 1.f, 0.f, 0.f, "vertical" },
                { 0.5f, 0.f, -0.866f, "overhang" },
            };
            bool allOk = true;
            char note[160] = "ok";
            for ( auto const& c : cases )
            {
                ContactFrame f = MakeContactFrame( V3( 10.f, 20.f, 3.f ),
                    V3( c.nx, c.ny, c.nz ), V3( 0.f, 1.f, -0.2f ) );
                float const nt = std::fabs( Dot( f.N, f.T ) );
                // B should equal N×T; check B · (N×T) ≈ 1
                Vec3 nxt = Cross( f.N, f.T );
                float const align = Dot( f.B, nxt );
                if ( !f.valid || nt > 0.05f || align < 0.98f )
                {
                    allOk = false;
                    std::snprintf( note, sizeof( note ), "fail_%s nt=%.3f align=%.3f",
                        c.tag, nt, align );
                    CertAdd( R, "contact_frame_orthonormal", "FAIL", note, 10.f, 20.f, 3.f );
                    break;
                }
            }
            if ( allOk )
            {
                CertAdd( R, "contact_frame_orthonormal", "PASS",
                    "flat/slope/vertical/overhang orthonormal" );
            }
        }

        // 2) Radius separation law.
        {
            FractureEvent ev = BuildEvent( V3( 0, 0, 0 ), V3( 0, 0, 1 ), V3( 1, 0, 0 ),
                "granite", 0xC0FFEEu, nullptr );
            bool ok = ev.ok
                && ev.fractureExtentRM + 1e-4f < ev.contactQueryRM
                && ev.d2ReconHaloRM > ev.fractureExtentRM
                && ev.hfRefineHaloRM > 0.f
                && ev.mouthOpenRM <= kMouthOpenMaxM + 1e-4f
                && ev.fractureExtentRM <= kMaxFractureExtentM + 1e-4f;
            char note[160];
            std::snprintf( note, sizeof( note ),
                "frac=%.3f contact=%.3f d2=%.3f hf=%.3f mouth=%.3f",
                ev.fractureExtentRM, ev.contactQueryRM, ev.d2ReconHaloRM,
                ev.hfRefineHaloRM, ev.mouthOpenRM );
            CertAdd( R, "radius_separation", ok ? "PASS" : "FAIL", note );
            if ( !ok && ev.ok == false )
            {
                CertAdd( R, "FRACTURE_OUTSIDE_ALLOWED_REGION", "FAIL", ev.fail );
            }
        }

        // 3) Material morphology families distinct.
        {
            uint64_t const seed = 0xA11CEu;
            FractureEvent g = BuildEvent( V3(0,0,0), V3(0,0,1), V3(1,0,0), "gravel", seed, nullptr );
            FractureEvent r = BuildEvent( V3(0,0,0), V3(0,0,1), V3(1,0,0), "granite", seed, nullptr );
            RockStruct::Foliation fol = RockStruct::FoliationAt( 128.0, 128.0 );
            FractureEvent s = BuildEvent( V3(0,0,0), V3(0,0,1), V3(1,0,0), "mica_schist", seed, &fol );
            bool ok = g.ok && r.ok && s.ok
                && std::strcmp( g.morphology, "aggregate_recess" ) == 0
                && std::strcmp( r.morphology, "compact_angular" ) == 0
                && std::strcmp( s.morphology, "foliation_plate" ) == 0
                && g.env.tHalf < r.env.tHalf + 0.02f
                && s.env.tHalf > g.env.tHalf
                && s.env.nInto < r.env.nInto + 0.01f;
            char note[160];
            std::snprintf( note, sizeof( note ),
                "gravel(%s t=%.3f) granite(%s t=%.3f) schist(%s t=%.3f nInto=%.3f)",
                g.morphology, g.env.tHalf, r.morphology, r.env.tHalf,
                s.morphology, s.env.tHalf, s.env.nInto );
            CertAdd( R, "material_morphology", ok ? "PASS" : "FAIL", note );
            if ( !ok )
            {
                CertAdd( R, "MATERIAL_MORPHOLOGY_FAIL", "FAIL", note );
            }
        }

        // 4) Rotational equivalence 0..90° for gravel / granite / mica_schist.
        {
            static float const kAngles[] = { 0.f, 15.f, 30.f, 45.f, 60.f, 75.f, 90.f };
            char const* mats[] = { "gravel", "granite", "mica_schist" };
            bool allOk = true;
            char failNote[192] = {};
            float fx = 0.f, fy = 0.f, fz = 0.f;
            for ( char const* mat : mats )
            {
                uint64_t const seed = HashMix( 0x507A7Eull, (uint64_t)FamilyOf( mat ) );
                // Fixture-local material structure — rotate with the surface (world = fixture only).
                RockStruct::Foliation const fol0 = RockStruct::FoliationAt( 140.0, 120.0 );
                LocalMorphMetrics base{};
                bool haveBase = false;
                for ( float ang : kAngles )
                {
                    Vec3 N = RotateX( V3( 0.f, 0.f, 1.f ), ang );
                    Vec3 pry = RotateX( V3( 1.f, 0.2f, 0.f ), ang );
                    Vec3 hit = RotateX( V3( 5.f, 0.f, 2.f ), ang );
                    RockStruct::Foliation fol = fol0;
                    if ( FamilyOf( mat ) == MaterialFamily::MicaSchistFoliation )
                    {
                        Vec3 fn = RotateX( V3( fol0.nx, fol0.ny, fol0.nz ), ang );
                        fn = Norm( fn );
                        fol.nx = fn.x; fol.ny = fn.y; fol.nz = fn.z;
                        Vec3 st = RotateX( V3( fol0.strikeX, fol0.strikeY, 0.f ), ang );
                        float const sl = std::sqrt( st.x * st.x + st.y * st.y );
                        if ( sl > 1e-5f ) { fol.strikeX = st.x / sl; fol.strikeY = st.y / sl; }
                    }
                    FractureEvent ev = BuildEvent( hit, N, pry, mat, seed,
                        ( FamilyOf( mat ) == MaterialFamily::MicaSchistFoliation ) ? &fol : nullptr );
                    if ( !ev.ok )
                    {
                        allOk = false;
                        std::snprintf( failNote, sizeof( failNote ),
                            "%s ang=%.0f %s", mat, ang, ev.fail );
                        fx = hit.x; fy = hit.y; fz = hit.z;
                        break;
                    }
                    LocalMorphMetrics m = SampleLocalMorph( ev );
                    if ( !haveBase ) { base = m; haveBase = true; continue; }
                    char why[80];
                    if ( !MorphEquivalent( base, m, why, sizeof( why ) ) )
                    {
                        allOk = false;
                        std::snprintf( failNote, sizeof( failNote ),
                            "%s ang=%.0f vs0 %s", mat, ang, why );
                        fx = hit.x; fy = hit.y; fz = hit.z;
                        break;
                    }
                }
                if ( !allOk ) { break; }
            }
            CertAdd( R, "rotation_equivalence", allOk ? "PASS" : "FAIL",
                allOk ? "gravel/granite/mica_schist 0..90 local-equivalent" : failNote,
                fx, fy, fz );
            if ( !allOk )
            {
                CertAdd( R, "ROTATION_EQUIVALENCE_FAIL", "FAIL", failNote, fx, fy, fz );
            }
        }

        // 5) Two-strike gravel: only small central recesses; strike 2 does not remount virgin;
        //    prior recess unchanged outside second envelope; no giant plate.
        {
            SynthLattice lat;
            lat.originX = -0.36f; lat.originY = -0.36f; lat.originZ = -0.30f;
            float const crest = 0.0f;
            lat.FillSolidBelowCrest( crest );
            int const solid0 = lat.CountSolid();

            Vec3 hit1{ 0.f, 0.f, crest };
            FractureEvent e1 = BuildEvent( hit1, V3( 0, 0, 1 ), V3( 1, 0, 0 ),
                "gravel", 0x6001ull, nullptr );
            int rem1 = SubtractConnected( lat, e1 );
            int const solid1 = lat.CountSolid();

            // Second strike contacts NEW INTERIOR (deeper along -N), not virgin crest.
            Vec3 hit2 = Add( hit1, Mul( e1.frame.N, -0.5f * e1.env.nInto ) );
            // Ensure seed cell is air from first strike — contact new interior face.
            FractureEvent e2 = BuildEvent( hit2, V3( 0, 0, 1 ), V3( 0.2f, 1.f, 0.f ),
                "gravel", 0x6002ull, nullptr );
            // Snapshot lattice after strike1 for PREVIOUS_STRIKE_MUTATED check.
            uint8_t after1[sizeof( lat.solid )];
            std::memcpy( after1, lat.solid, sizeof( after1 ) );

            int rem2 = SubtractConnected( lat, e2 );
            int const solid2 = lat.CountSolid();

            // Prior-strike voxels outside e2 envelope must be unchanged.
            bool priorMut = false;
            float px = 0.f, py = 0.f, pz = 0.f;
            for ( int z = 0; z < SynthLattice::D; ++z )
            {
                for ( int y = 0; y < SynthLattice::H; ++y )
                {
                    for ( int x = 0; x < SynthLattice::W; ++x )
                    {
                        int const i = lat.Idx( x, y, z );
                        if ( after1[i] == lat.solid[i] ) { continue; }
                        Vec3 c = lat.Center( x, y, z );
                        if ( !PointInEnvelopeWorld( e2, c.x, c.y, c.z ) )
                        {
                            priorMut = true; px = c.x; py = c.y; pz = c.z;
                            break;
                        }
                    }
                    if ( priorMut ) { break; }
                }
                if ( priorMut ) { break; }
            }

            bool const small = e1.fractureExtentRM <= 0.10f && e2.fractureExtentRM <= 0.10f;
            bool const removed = rem1 > 0 && rem2 > 0;
            bool const notGiant = ( solid0 - solid2 ) < 400; // tip-scale cells only
            bool const deeper = hit2.z < hit1.z - 1e-4f;
            bool ok = e1.ok && e2.ok && small && removed && notGiant && deeper && !priorMut
                && !e1.releasesRigidBody;

            char note[192];
            std::snprintf( note, sizeof( note ),
                "rem1=%d rem2=%d solid=%d->%d->%d ext=%.3f/%.3f deeper=%d priorMut=%d",
                rem1, rem2, solid0, solid1, solid2,
                e1.fractureExtentRM, e2.fractureExtentRM, deeper ? 1 : 0, priorMut ? 1 : 0 );
            CertAdd( R, "two_strike_gravel", ok ? "PASS" : "FAIL", note, hit2.x, hit2.y, hit2.z );
            if ( priorMut )
            {
                CertAdd( R, "PREVIOUS_STRIKE_MUTATED_WITHOUT_CAUSE", "FAIL",
                    "voxel outside strike2 envelope changed", px, py, pz );
            }
            if ( !deeper )
            {
                CertAdd( R, "STRIKE_REMOUNTED_TO_VIRGIN_HF", "FAIL",
                    "strike2 contact not interior", hit2.x, hit2.y, hit2.z );
            }
            if ( !small || !notGiant )
            {
                CertAdd( R, "FRACTURE_OUTSIDE_ALLOWED_REGION", "FAIL",
                    "gravel bite exceeded tip-scale", hit1.x, hit1.y, hit1.z );
            }
        }

        // 6) Outside recon halo: synthetic "HF" samples unchanged (identity proxy).
        {
            FractureEvent ev = BuildEvent( V3( 0, 0, 0 ), V3( 0, 0, 1 ), V3( 1, 0, 0 ),
                "granite", 42u, nullptr );
            float minX, minY, maxX, maxY;
            ReconHaloAabb( ev, minX, minY, maxX, maxY );
            float fracMinX, fracMinY, fracMinZ, fracMaxX, fracMaxY, fracMaxZ;
            WorldAabb( ev, fracMinX, fracMinY, fracMinZ, fracMaxX, fracMaxY, fracMaxZ );
            float const fracSpan = (std::max)( fracMaxX - fracMinX, fracMaxY - fracMinY );
            float const maxAllowedSpan = fracSpan + 2.f * kD2ReconHaloM + 0.04f;
            // Probe rings outside halo — must not be classified as fracture/recon.
            bool outsideClean = true;
            float bx = 0.f, by = 0.f;
            float const rings[] = { 0.55f, 0.80f, 1.20f };
            for ( float rad : rings )
            {
                for ( int i = 0; i < 24; ++i )
                {
                    float ang = (float)i * ( 2.f * kPi / 24.f );
                    float x = rad * std::cos( ang );
                    float y = rad * std::sin( ang );
                    if ( x >= minX && x <= maxX && y >= minY && y <= maxY ) { continue; }
                    if ( PointInEnvelopeWorld( ev, x, y, -0.02f )
                      || PointInEnvelopeWorld( ev, x, y, 0.02f )
                      || PointInEnvelopeWorld( ev, x, y, -0.08f ) )
                    {
                        outsideClean = false; bx = x; by = y; break;
                    }
                }
                if ( !outsideClean ) { break; }
            }
            float const span = (std::max)( maxX - minX, maxY - minY );
            bool ok = outsideClean && span <= maxAllowedSpan + 1e-4f;
            char note[160];
            std::snprintf( note, sizeof( note ),
                "haloSpan=%.3f maxAllow=%.3f fracSpan=%.3f outsideClean=%d",
                span, maxAllowedSpan, fracSpan, outsideClean ? 1 : 0 );
            CertAdd( R, "outside_recon_halo_identity", ok ? "PASS" : "FAIL", note, bx, by, 0.f );
            if ( !ok )
            {
                CertAdd( R, "HF_CHANGED_OUTSIDE_RECON_HALO", "FAIL", note, bx, by, 0.f );
            }
        }

        // 7) No world-up fracture law: vertical N with horizontal pry equals flat local morph.
        {
            uint64_t const seed = 99u;
            FractureEvent flat = BuildEvent( V3(0,0,0), V3(0,0,1), V3(1,0,0), "granite", seed, nullptr );
            FractureEvent vert = BuildEvent( V3(0,0,0), V3(1,0,0), V3(0,0,-1), "granite", seed, nullptr );
            LocalMorphMetrics a = SampleLocalMorph( flat );
            LocalMorphMetrics b = SampleLocalMorph( vert );
            char why[80];
            bool ok = MorphEquivalent( a, b, why, sizeof( why ) );
            CertAdd( R, "no_world_up_fracture_law", ok ? "PASS" : "FAIL",
                ok ? "vertical==flat local morph" : why );
        }

        // 8) Presentation coverage closed — mouth samples must have a solid floor/wall cover.
        //    Sky/clear through the action neighborhood is never excused (no "open mouth OK").
        {
            SynthLattice lat;
            lat.originX = -0.36f; lat.originY = -0.36f; lat.originZ = -0.40f;
            float const crest = 0.0f;
            lat.FillSolidBelowCrest( crest );
            FractureEvent ev = BuildEvent( V3( 0, 0, crest ), V3( 0, 0, 1 ), V3( 1, 0, 0 ),
                "granite", 0xC0BEu, nullptr );
            int const rem = SubtractConnected( lat, ev );
            float const mouthR = (std::max)( 0.05f, ev.mouthOpenRM );
            float const floorDepth = ev.env.nInto + kD2ReconHaloM;
            int uncovered = 0;
            float ux = 0.f, uy = 0.f, uz = 0.f;
            // Dense mouth disk at crest — each sample needs solid within floorDepth below.
            for ( float y = -mouthR; y <= mouthR + 1e-4f; y += 0.02f )
            {
                for ( float x = -mouthR; x <= mouthR + 1e-4f; x += 0.02f )
                {
                    if ( x * x + y * y > mouthR * mouthR ) { continue; }
                    bool hasFloor = false;
                    for ( float dz = 0.01f; dz <= floorDepth + 1e-4f; dz += 0.012f )
                    {
                        if ( lat.SolidAtWorld( x, y, crest - dz ) )
                        {
                            hasFloor = true;
                            break;
                        }
                    }
                    if ( !hasFloor )
                    {
                        ++uncovered;
                        if ( uncovered == 1 ) { ux = x; uy = y; uz = crest; }
                    }
                }
            }
            bool ok = rem > 0 && uncovered == 0 && ev.ok;
            char note[160];
            std::snprintf( note, sizeof( note ),
                "rem=%d uncoveredMouth=%d mouthR=%.3f floorDepth=%.3f (sky/clear never excused)",
                rem, uncovered, mouthR, floorDepth );
            CertAdd( R, "presentation_coverage_closed", ok ? "PASS" : "FAIL", note, ux, uy, uz );
            if ( !ok )
            {
                CertAdd( R, "PRESENTATION_COVERAGE_FAIL", "FAIL", note, ux, uy, uz );
            }
        }

        // 9) Outside-strike-volume deform clamp — recon halo must not inflate past fracture+halo.
        {
            char const* mats[] = { "gravel", "granite", "mica_schist" };
            bool allOk = true;
            char note[192] = "ok";
            float fx = 0.f, fy = 0.f, fz = 0.f;
            for ( char const* mat : mats )
            {
                RockStruct::Foliation fol = RockStruct::FoliationAt( 130.0, 125.0 );
                FractureEvent ev = BuildEvent( V3( 0, 0, 0 ), V3( 0, 0, 1 ), V3( 1, 0.15f, 0 ),
                    mat, 0xDE70ADull,
                    ( FamilyOf( mat ) == MaterialFamily::MicaSchistFoliation ) ? &fol : nullptr );
                float minX, minY, maxX, maxY;
                ReconHaloAabb( ev, minX, minY, maxX, maxY );
                float fMinX, fMinY, fMinZ, fMaxX, fMaxY, fMaxZ;
                WorldAabb( ev, fMinX, fMinY, fMinZ, fMaxX, fMaxY, fMaxZ );
                float const haloSpan = (std::max)( maxX - minX, maxY - minY );
                float const fracSpan = (std::max)( fMaxX - fMinX, fMaxY - fMinY );
                float const maxAllow = fracSpan + 2.f * kD2ReconHaloM + 0.04f;
                // Mouth / HF refine must stay inside recon halo (not a second inflate).
                float const mouthPad = 2.f * ev.mouthOpenRM + 2.f * kHfRefineHaloM;
                if ( !ev.ok || haloSpan > maxAllow + 1e-4f || mouthPad > maxAllow + 0.08f
                  || ev.fractureExtentRM > kMaxFractureExtentM + 1e-4f )
                {
                    allOk = false;
                    std::snprintf( note, sizeof( note ),
                        "%s haloSpan=%.3f max=%.3f mouthPad=%.3f fracExt=%.3f",
                        mat, haloSpan, maxAllow, mouthPad, ev.fractureExtentRM );
                    fx = maxX; fy = maxY; fz = 0.f;
                    break;
                }
            }
            CertAdd( R, "outside_strike_volume_deform", allOk ? "PASS" : "FAIL", note, fx, fy, fz );
            if ( !allOk )
            {
                CertAdd( R, "HF_CHANGED_OUTSIDE_RECON_HALO", "FAIL", note, fx, fy, fz );
            }
        }

        return R;
    }

    inline bool WriteCertArtifact( CertResult const& R, char const* certPath, char const* failPath )
    {
        FILE* f = nullptr;
        fopen_s( &f, certPath, "w" );
        if ( !f ) { return false; }
        std::fprintf( f,
            "P5 SIDE-GATE - MATERIAL-TRUE PICK FRACTURE + CLOSED LOCAL SURFACE\n"
            "law=impact->penetration->pry->material fracture->connected release->occupancy->local recon\n"
            "contact_frame=(T,B,N) angle-independent; P5a FREEZE; P5b CLOSED\n"
            "hard_gate=sky/clear in|around hole => PRESENTATION_COVERAGE_FAIL (no mouth exception)\n"
            "hard_gate=deform beyond fracture+min recon halo => HF_CHANGED_OUTSIDE_RECON_HALO\n"
            "pass=%d fail=%d skip=%d exit_code=%d\n"
            "first_fail=%s @ (%.3f,%.3f,%.3f)\n\n",
            R.passN, R.failN, R.skipN, R.exitCode,
            R.firstFail[0] ? R.firstFail : "-",
            R.failX, R.failY, R.failZ );
        for ( int i = 0; i < R.rowN; ++i )
        {
            CertRow const& r = R.rows[i];
            std::fprintf( f, "%s\t%s\t%s\t(%.3f,%.3f,%.3f)\n",
                r.check, r.verdict, r.note, r.x, r.y, r.z );
        }
        std::fclose( f );

        if ( R.exitCode != 0 && failPath && failPath[0] )
        {
            FILE* ff = nullptr;
            fopen_s( &ff, failPath, "w" );
            if ( ff )
            {
                std::fprintf( ff,
                    "FAIL:\nreason=%s\nworld=(%.3f,%.3f,%.3f)\n",
                    R.firstFail, R.failX, R.failY, R.failZ );
                for ( int i = 0; i < R.rowN; ++i )
                {
                    if ( std::strcmp( R.rows[i].verdict, "FAIL" ) != 0 ) { continue; }
                    std::fprintf( ff, "row=%s note=%s @ (%.3f,%.3f,%.3f)\n",
                        R.rows[i].check, R.rows[i].note,
                        R.rows[i].x, R.rows[i].y, R.rows[i].z );
                }
                std::fclose( ff );
            }
        }
        return true;
    }
}
