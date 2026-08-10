#pragma once

#include "PickFracture.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Tool-scale local boolean reconstruction for pick strikes.
//
// The coarse occupancy lattice remains the matter/receipt ledger.  This mesh is the
// presentation boundary of (virgin solid - fracture envelopes), sampled at a scale
// appropriate to the tool.  The patch includes unchanged virgin surface around the
// cut so the renderer can atomically replace a bounded HF rectangle without a plug.
namespace FractureSurface
{
    struct Vec3
    {
        float x = 0.f, y = 0.f, z = 0.f;
    };

    struct Tri
    {
        Vec3 a{}, b{}, c{};
    };

    struct Region
    {
        float minX = 0.f, minY = 0.f, minZ = 0.f;
        float maxX = 0.f, maxY = 0.f, maxZ = 0.f;
    };

    struct Patch
    {
        float minX = 0.f, minY = 0.f, maxX = 0.f, maxY = 0.f;
        float minZ = 0.f, maxZ = 0.f;
        float spacing = 0.015625f; // 1.5625 cm; <= smallest pick extent / 3
        std::vector<PickFracture::FractureEvent> events;
        std::vector<Region> eventBounds;
        float eventBucketM = 0.125f;
        std::unordered_map<uint64_t, std::vector<int>> eventBuckets;
        // Sparse reconstruction ownership.  Connected events share a boolean field,
        // but untouched space inside their aggregate bounding box is never subdivided.
        std::vector<Region> regions;
        // XY ownership mask used only to retire the virgin heightfield. This is a
        // deduplicated tile union; one rectangle per strike made a body-scale dig
        // submit thousands of redundant stencil primitives every frame.
        std::vector<Region> footprintRegions;
        std::vector<Tri> tris;
        int nx = 0, ny = 0, nz = 0;
        int activeCells = 0;
    };

    inline Vec3 Add( Vec3 a, Vec3 b ) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    inline Vec3 Sub( Vec3 a, Vec3 b ) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    inline Vec3 Mul( Vec3 a, float s ) { return { a.x * s, a.y * s, a.z * s }; }
    inline float Dot( Vec3 a, Vec3 b ) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    inline Vec3 Cross( Vec3 a, Vec3 b )
    {
        return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    }

    inline float CharacteristicM( PickFracture::FractureEvent const& ev )
    {
        return (std::max)( 0.01f, (std::min)( ev.env.tHalf,
            (std::min)( ev.env.bHalf, (std::min)( ev.env.nInto, (std::max)( ev.env.nAir, 0.01f ) ) ) ) );
    }

    // Positive = material, negative = air.  Intersection implements:
    //     material = below virgin HF AND outside every removed fracture envelope.
    inline float SolidField( std::vector<PickFracture::FractureEvent> const& events,
        std::function<float( float, float )> const& groundZ,
        float x, float y, float z )
    {
        float phi = groundZ( x, y ) - z;
        PickFracture::Vec3 const p = PickFracture::V3( x, y, z );
        for ( PickFracture::FractureEvent const& ev : events )
        {
            if ( !ev.ok || !ev.frame.valid ) { continue; }
            PickFracture::Vec3 const q = PickFracture::WorldToLocal( ev.frame, p );
            float const outside = ( PickFracture::LocalEnvelopeDistance( ev.env, q ) - 1.f )
                * CharacteristicM( ev );
            phi = (std::min)( phi, outside );
        }
        return phi;
    }

    inline uint64_t BucketKey( int x, int y, int z )
    {
        constexpr uint64_t mask = ( 1ull << 21 ) - 1ull;
        return ( (uint64_t)x & mask ) << 42
            | ( (uint64_t)y & mask ) << 21
            | ( (uint64_t)z & mask );
    }

    inline float SolidField( Patch const& patch,
        std::function<float( float, float )> const& groundZ,
        float x, float y, float z )
    {
        float phi = groundZ( x, y ) - z;
        int const bx = (int)std::floor( x / patch.eventBucketM );
        int const by = (int)std::floor( y / patch.eventBucketM );
        int const bz = (int)std::floor( z / patch.eventBucketM );
        auto const found = patch.eventBuckets.find( BucketKey( bx, by, bz ) );
        if ( found == patch.eventBuckets.end() ) { return phi; }
        PickFracture::Vec3 const p = PickFracture::V3( x, y, z );
        for ( int const index : found->second )
        {
            PickFracture::FractureEvent const& ev = patch.events[(size_t)index];
            PickFracture::Vec3 const q = PickFracture::WorldToLocal( ev.frame, p );
            float const outside = ( PickFracture::LocalEnvelopeDistance( ev.env, q ) - 1.f )
                * CharacteristicM( ev );
            phi = (std::min)( phi, outside );
        }
        return phi;
    }

    inline Vec3 Interp( Vec3 a, Vec3 b, float fa, float fb )
    {
        float t = 0.5f;
        float const den = fa - fb;
        if ( std::fabs( den ) > 1e-8f ) { t = fa / den; }
        t = std::clamp( t, 0.f, 1.f );
        return Add( a, Mul( Sub( b, a ), t ) );
    }

    inline Vec3 Gradient( std::vector<PickFracture::FractureEvent> const& events,
        std::function<float( float, float )> const& groundZ,
        Vec3 p, float eps )
    {
        return {
            SolidField( events, groundZ, p.x + eps, p.y, p.z )
                - SolidField( events, groundZ, p.x - eps, p.y, p.z ),
            SolidField( events, groundZ, p.x, p.y + eps, p.z )
                - SolidField( events, groundZ, p.x, p.y - eps, p.z ),
            SolidField( events, groundZ, p.x, p.y, p.z + eps )
                - SolidField( events, groundZ, p.x, p.y, p.z - eps )
        };
    }

    inline Vec3 Gradient( Patch const& patch,
        std::function<float( float, float )> const& groundZ,
        Vec3 p, float eps )
    {
        return {
            SolidField( patch, groundZ, p.x + eps, p.y, p.z )
                - SolidField( patch, groundZ, p.x - eps, p.y, p.z ),
            SolidField( patch, groundZ, p.x, p.y + eps, p.z )
                - SolidField( patch, groundZ, p.x, p.y - eps, p.z ),
            SolidField( patch, groundZ, p.x, p.y, p.z + eps )
                - SolidField( patch, groundZ, p.x, p.y, p.z - eps )
        };
    }

    inline void EmitOriented( Patch& patch,
        std::function<float( float, float )> const& groundZ,
        Vec3 a, Vec3 b, Vec3 c, Vec3 outwardHint )
    {
        Vec3 const n = Cross( Sub( b, a ), Sub( c, a ) );
        if ( Dot( n, n ) < 1e-14f ) { return; }
        // PolygoniseTet already classifies its vertices. Air centroid minus solid
        // centroid is a stable outward direction and avoids six field lookups for
        // every emitted triangle.
        if ( Dot( n, outwardHint ) < 0.f ) { std::swap( b, c ); }
        // The air/solid centroid hint is exact for the tetrahedron's linear field,
        // but min(body,envelope) has a nonlinear crease at the cavity mouth. Verify
        // the final winding against occupancy truth so a crease sliver cannot report
        // an inward normal. Two bucketed field samples are much cheaper than a full
        // gradient and make the orientation causal against the represented matter.
        Vec3 const nn0 = Cross( Sub( b, a ), Sub( c, a ) );
        float const nl = std::sqrt( Dot( nn0, nn0 ) );
        if ( nl > 1e-8f )
        {
            Vec3 const nn = Mul( nn0, 1.f / nl );
            Vec3 const center = Mul( Add( a, Add( b, c ) ), 1.f / 3.f );
            // Probe at the same 0.3-cell scale used by collision/presentation
            // continuity; a sub-millimetre sample can choose the opposite branch
            // of the nonlinear min() crease and invert an otherwise valid face.
            float const probe = patch.spacing * 0.30f;
            Vec3 const plus = Add( center, Mul( nn, probe ) );
            Vec3 const minus = Add( center, Mul( nn, -probe ) );
            float const fp = SolidField( patch, groundZ, plus.x, plus.y, plus.z );
            float const fm = SolidField( patch, groundZ, minus.x, minus.y, minus.z );
            if ( fp > fm ) { std::swap( b, c ); }
        }
        patch.tris.push_back( { a, b, c } );
    }

    inline void PolygoniseTet( Patch& patch,
        std::function<float( float, float )> const& groundZ,
        Vec3 const p[4], float const f[4] )
    {
        int solid[4], air[4], ns = 0, na = 0;
        for ( int i = 0; i < 4; ++i )
        {
            if ( f[i] >= 0.f ) { solid[ns++] = i; }
            else { air[na++] = i; }
        }
        if ( ns == 0 || ns == 4 ) { return; }
        Vec3 solidMean{}, airMean{};
        for ( int i=0;i<ns;++i ) { solidMean=Add(solidMean,p[solid[i]]); }
        for ( int i=0;i<na;++i ) { airMean=Add(airMean,p[air[i]]); }
        solidMean=Mul(solidMean,1.f/(float)ns);
        airMean=Mul(airMean,1.f/(float)na);
        Vec3 const outwardHint=Sub(airMean,solidMean);
        if ( ns == 1 || ns == 3 )
        {
            bool const oneSolid = ns == 1;
            int const lone = oneSolid ? solid[0] : air[0];
            int const* other = oneSolid ? air : solid;
            Vec3 q[3];
            for ( int i = 0; i < 3; ++i )
            {
                q[i] = Interp( p[lone], p[other[i]], f[lone], f[other[i]] );
            }
            EmitOriented( patch, groundZ, q[0], q[1], q[2], outwardHint );
            return;
        }

        // Two solid / two air: the cut is a quad.  Use a stable diagonal.
        Vec3 q0 = Interp( p[solid[0]], p[air[0]], f[solid[0]], f[air[0]] );
        Vec3 q1 = Interp( p[solid[0]], p[air[1]], f[solid[0]], f[air[1]] );
        Vec3 q2 = Interp( p[solid[1]], p[air[0]], f[solid[1]], f[air[0]] );
        Vec3 q3 = Interp( p[solid[1]], p[air[1]], f[solid[1]], f[air[1]] );
        EmitOriented( patch, groundZ, q0, q1, q2, outwardHint );
        EmitOriented( patch, groundZ, q1, q3, q2, outwardHint );
    }

    inline float SnapDown( float v, float step ) { return std::floor( v / step ) * step; }
    inline float SnapUp( float v, float step ) { return std::ceil( v / step ) * step; }

    inline Patch BuildPatch( std::vector<PickFracture::FractureEvent> const& events,
        std::function<float( float, float )> const& groundZ,
        float requestedSpacing = 0.015625f )
    {
        Patch patch;
        patch.events = events;
        patch.spacing = std::clamp( requestedSpacing, 0.0078125f, 0.025f );
        if ( events.empty() ) { return patch; }

        float minX = 1e9f, minY = 1e9f, minZ = 1e9f;
        float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;
        std::vector<Region> footprintSources;
        // Half a cell plus lattice snapping supplies the outside sample needed by
        // rotated envelopes without restoring the old two-cell collar. Reconstruction
        // can extend by at most 1.5 cells, independent of tool shape or strike history.
        float const collar = patch.spacing * 0.50f;
        for ( PickFracture::FractureEvent const& ev : events )
        {
            float a, b, c, d, e, f;
            PickFracture::WorldAabb( ev, a, b, c, d, e, f );
            patch.eventBounds.push_back( { a,b,c,d,e,f } );
            Region region;
            region.minX = SnapDown( a - collar, patch.spacing );
            region.minY = SnapDown( b - collar, patch.spacing );
            region.maxX = SnapUp( d + collar, patch.spacing );
            region.maxY = SnapUp( e + collar, patch.spacing );
            float localGroundMin = 1e9f, localGroundMax = -1e9f;
            for ( int j = 0; j <= 4; ++j )
            for ( int i = 0; i <= 4; ++i )
            {
                float const x = region.minX + ( region.maxX - region.minX ) * (float)i / 4.f;
                float const y = region.minY + ( region.maxY - region.minY ) * (float)j / 4.f;
                float const z = groundZ( x, y );
                localGroundMin = (std::min)( localGroundMin, z );
                localGroundMax = (std::max)( localGroundMax, z );
            }
            // Reconstruction follows the physical 3-D envelope, never a column from
            // a buried strike back to the heightfield roof. That column was invisible
            // subdivision creep and dominated long tunnel cost.
            bool const touchesGround = f + collar >= localGroundMin
                && c - collar <= localGroundMax;
            // Do not stretch a rotated/sloped strike's reconstruction box to the
            // extrema of the surrounding height field.  The event's world AABB
            // already contains every point of the physical carve; one outside
            // sample plus lattice snapping is sufficient to close its boundary.
            // Pulling localGroundMin/Max into Z created a tall rectangular column
            // (7.9 cm at 45 degrees) outside a 4--7 cm pick wound.
            region.minZ = SnapDown( c - collar, patch.spacing );
            region.maxZ = SnapUp( f + collar, patch.spacing );
            patch.regions.push_back( region );
            // Retire HF only where this event can actually meet the virgin skin.
            // Fully buried tunnel strikes remain closed cavity geometry and do not
            // punch a sky window through the roof above them.
            if ( touchesGround )
            {
                footprintSources.push_back( region );
            }
            minX = (std::min)( minX, region.minX ); minY = (std::min)( minY, region.minY );
            minZ = (std::min)( minZ, region.minZ ); maxX = (std::max)( maxX, region.maxX );
            maxY = (std::max)( maxY, region.maxY ); maxZ = (std::max)( maxZ, region.maxZ );
        }

        patch.minX = minX; patch.minY = minY; patch.minZ = minZ;
        patch.maxX = maxX; patch.maxY = maxY; patch.maxZ = maxZ;

        // Collapse overlapping event projections into a stable, bounded HF handoff
        // mask. Geometry still owns the exact 3-D sparse regions above; these tiles
        // prevent render cost from scaling with repeated strikes in the same doorway.
        {
            float const tile = (std::max)( 0.0625f, patch.spacing * 4.f );
            std::unordered_set<uint64_t> owned;
            for ( Region const& r : footprintSources )
            {
                int const x0 = (int)std::floor( r.minX / tile );
                int const y0 = (int)std::floor( r.minY / tile );
                int const x1 = (int)std::ceil( r.maxX / tile );
                int const y1 = (int)std::ceil( r.maxY / tile );
                for ( int y = y0; y < y1; ++y )
                for ( int x = x0; x < x1; ++x )
                {
                    uint64_t const key = ( (uint64_t)(uint32_t)x << 32 )
                        | (uint64_t)(uint32_t)y;
                    if ( !owned.insert( key ).second ) { continue; }
                    patch.footprintRegions.push_back( {
                        (float)x * tile, (float)y * tile, 0.f,
                        (float)( x + 1 ) * tile, (float)( y + 1 ) * tile, 0.f } );
                }
            }
        }

        patch.nx = (std::max)( 1, (int)std::ceil( ( patch.maxX - patch.minX ) / patch.spacing ) );
        patch.ny = (std::max)( 1, (int)std::ceil( ( patch.maxY - patch.minY ) / patch.spacing ) );
        patch.nz = (std::max)( 1, (int)std::ceil( ( patch.maxZ - patch.minZ ) / patch.spacing ) );
        float const dx = ( patch.maxX - patch.minX ) / (float)patch.nx;
        float const dy = ( patch.maxY - patch.minY ) / (float)patch.ny;
        float const dz = ( patch.maxZ - patch.minZ ) / (float)patch.nz;

        for ( size_t eventIndex = 0; eventIndex < patch.eventBounds.size(); ++eventIndex )
        {
            Region const& b = patch.eventBounds[eventIndex];
            // Buckets accelerate exact point queries; event AABBs already bound the
            // envelope. The former whole-cell guard tripled candidate events in a
            // dense excavation and was only needed by the retired finite-difference
            // winding pass.
            float const guard = 1e-4f;
            int const bx0 = (int)std::floor( ( b.minX - guard ) / patch.eventBucketM );
            int const by0 = (int)std::floor( ( b.minY - guard ) / patch.eventBucketM );
            int const bz0 = (int)std::floor( ( b.minZ - guard ) / patch.eventBucketM );
            int const bx1 = (int)std::floor( ( b.maxX + guard ) / patch.eventBucketM );
            int const by1 = (int)std::floor( ( b.maxY + guard ) / patch.eventBucketM );
            int const bz1 = (int)std::floor( ( b.maxZ + guard ) / patch.eventBucketM );
            for ( int bz = bz0; bz <= bz1; ++bz )
            for ( int by = by0; by <= by1; ++by )
            for ( int bx = bx0; bx <= bx1; ++bx )
            {
                patch.eventBuckets[BucketKey( bx, by, bz )].push_back( (int)eventIndex );
            }
        }

        static int const tet[6][4] = {
            { 0, 5, 1, 6 }, { 0, 1, 2, 6 }, { 0, 2, 3, 6 },
            { 0, 3, 7, 6 }, { 0, 7, 4, 6 }, { 0, 4, 5, 6 }
        };
        static int const ox[8] = { 0, 1, 1, 0, 0, 1, 1, 0 };
        static int const oy[8] = { 0, 0, 1, 1, 0, 0, 1, 1 };
        static int const oz[8] = { 0, 0, 0, 0, 1, 1, 1, 1 };

        size_t estimatedCells = 0;
        for ( Region const& r : patch.regions )
        {
            estimatedCells += (size_t)( ( r.maxX - r.minX ) / patch.spacing + 1.f )
                * (size_t)( ( r.maxY - r.minY ) / patch.spacing + 1.f )
                * (size_t)( ( r.maxZ - r.minZ ) / patch.spacing + 1.f );
        }
        patch.tris.reserve( estimatedCells * 8u );
        std::unordered_set<size_t> visited;
        visited.reserve( estimatedCells );
        for ( Region const& r : patch.regions )
        {
            int const i0 = std::clamp( (int)std::floor( ( r.minX - patch.minX ) / dx ), 0, patch.nx - 1 );
            int const i1 = std::clamp( (int)std::ceil( ( r.maxX - patch.minX ) / dx ), 1, patch.nx );
            int const j0 = std::clamp( (int)std::floor( ( r.minY - patch.minY ) / dy ), 0, patch.ny - 1 );
            int const j1 = std::clamp( (int)std::ceil( ( r.maxY - patch.minY ) / dy ), 1, patch.ny );
            int const k0 = std::clamp( (int)std::floor( ( r.minZ - patch.minZ ) / dz ), 0, patch.nz - 1 );
            int const k1 = std::clamp( (int)std::ceil( ( r.maxZ - patch.minZ ) / dz ), 1, patch.nz );
            for ( int k = k0; k < k1; ++k )
            for ( int j = j0; j < j1; ++j )
            for ( int i = i0; i < i1; ++i )
            {
            size_t const cellKey = ( (size_t)k * (size_t)patch.ny + (size_t)j )
                * (size_t)patch.nx + (size_t)i;
            if ( !visited.insert( cellKey ).second ) { continue; }
            ++patch.activeCells;
            Vec3 cp[8];
            float cf[8];
            for ( int q = 0; q < 8; ++q )
            {
                cp[q] = {
                    patch.minX + ( (float)i + (float)ox[q] ) * dx,
                    patch.minY + ( (float)j + (float)oy[q] ) * dy,
                    patch.minZ + ( (float)k + (float)oz[q] ) * dz
                };
                cf[q] = SolidField( patch, groundZ, cp[q].x, cp[q].y, cp[q].z );
                // A native face can pass exactly through a lattice vertex.  Leaving
                // that sample at mathematical zero makes adjacent tetrahedra emit
                // zero-area slivers on different sides of the shared vertex; the
                // slivers are then discarded and leave sub-millimetre open edges.
                // Resolve the ambiguous near-zero band consistently toward occupied
                // matter.  At the current finest spacing this guard is 2.0 mm, below
                // the 4--12 mm HF lip collar.  It can only make the opening smaller,
                // never extend the carve beyond its event-owned envelope.
                float const zeroTie = 0.003f;
                if ( std::fabs( cf[q] ) < zeroTie ) { cf[q] = zeroTie; }
            }
            for ( int t = 0; t < 6; ++t )
            {
                Vec3 tp[4]; float tf[4];
                for ( int q = 0; q < 4; ++q ) { tp[q] = cp[tet[t][q]]; tf[q] = cf[tet[t][q]]; }
                PolygoniseTet( patch, groundZ, tp, tf );
            }
            }
        }
        return patch;
    }

    inline bool EventBoundsOverlap( PickFracture::FractureEvent const& a,
        PickFracture::FractureEvent const& b, float collar = 0.002f )
    {
        float a0,a1,a2,a3,a4,a5, b0,b1,b2,b3,b4,b5;
        PickFracture::WorldAabb( a, a0,a1,a2,a3,a4,a5 );
        PickFracture::WorldAabb( b, b0,b1,b2,b3,b4,b5 );
        return a3 + collar >= b0 && b3 + collar >= a0
            && a4 + collar >= b1 && b4 + collar >= a1
            && a5 + collar >= b2 && b5 + collar >= a2;
    }

    inline std::vector<Patch> BuildConnectedPatches(
        std::vector<PickFracture::FractureEvent> const& events,
        std::function<float( float, float )> const& groundZ,
        float spacing = 0.015625f )
    {
        std::vector<Patch> out;
        std::vector<uint8_t> used( events.size(), 0 );
        for ( size_t seed = 0; seed < events.size(); ++seed )
        {
            if ( used[seed] ) { continue; }
            std::vector<size_t> queue{ seed };
            std::vector<PickFracture::FractureEvent> group;
            used[seed] = 1;
            for ( size_t qi = 0; qi < queue.size(); ++qi )
            {
                size_t const i = queue[qi];
                group.push_back( events[i] );
                for ( size_t j = 0; j < events.size(); ++j )
                {
                    if ( used[j] || !EventBoundsOverlap( events[i], events[j] ) ) { continue; }
                    used[j] = 1;
                    queue.push_back( j );
                }
            }
            out.push_back( BuildPatch( group, groundZ, spacing ) );
        }
        return out;
    }
}
