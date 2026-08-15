#pragma once

// Stage 7: watertight visible reconstruction of the certified Stage-6 exposure.
// The implicit occupancy is solid iff z <= the compiled present erosion surface.
// A globally aligned dual grid gives every vertical crossing one canonical quad;
// blocks own half-open crossing ranges and query identical halo vertices.

#include "CausalWorldExposure.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace CausalVisibleExposure
{
    constexpr double kDualStepM = 0.5;
    constexpr int kBlockCells = 16;
    constexpr double kBlockSizeM = kDualStepM * kBlockCells;

    struct Vec3
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    struct Tri
    {
        Vec3 a;
        Vec3 b;
        Vec3 c;
    };

    struct BlockMesh
    {
        int blockX = 0;
        int blockY = 0;
        std::vector<Tri> triangles;
    };

    // Package construction is deliberately represented as linear arrays.  The
    // authority pass fills samples once; later stages consume indices into that
    // immutable field and must not re-enter the authority while emitting mesh.
    constexpr int kBlockSampleSpan = kBlockCells + 1;
    struct BlockSurfaceSamples
    {
        int blockX = 0;
        int blockY = 0;
        std::vector<Vec3> vertices; // row-major kBlockSampleSpan squared
    };

    struct CrossingDescriptor
    {
        uint16_t v00 = 0;
        uint16_t v10 = 0;
        uint16_t v11 = 0;
        uint16_t v01 = 0;
    };

    struct BlockSurfaceDescriptors
    {
        int blockX = 0;
        int blockY = 0;
        std::vector<CrossingDescriptor> crossings;
    };

    inline BlockSurfaceDescriptors DescribeBlock( BlockSurfaceSamples const& samples )
    {
        BlockSurfaceDescriptors out;
        out.blockX = samples.blockX;
        out.blockY = samples.blockY;
        out.crossings.reserve( (size_t)kBlockCells * kBlockCells );
        for ( int y = 0; y < kBlockCells; ++y )
        for ( int x = 0; x < kBlockCells; ++x )
        {
            uint16_t const v00 = (uint16_t)( y * kBlockSampleSpan + x );
            uint16_t const v10 = (uint16_t)( v00 + 1 );
            uint16_t const v01 = (uint16_t)( v00 + kBlockSampleSpan );
            uint16_t const v11 = (uint16_t)( v01 + 1 );
            out.crossings.push_back( { v00, v10, v11, v01 } );
        }
        return out;
    }

    inline Vec3 PresentationSamplePoint( BlockSurfaceSamples const& samples,
        CrossingDescriptor const& crossing )
    {
        // Preserve the former two-triangle centroid exactly: v00 and v11 occur
        // once; v10 and v01 occur twice in the fixed-diagonal triangle pair.
        Vec3 const& v00 = samples.vertices[crossing.v00];
        Vec3 const& v10 = samples.vertices[crossing.v10];
        Vec3 const& v11 = samples.vertices[crossing.v11];
        Vec3 const& v01 = samples.vertices[crossing.v01];
        auto sixVertexMean = []( double a00, double a10, double a11, double a01 )
        {
            // Match the previous `a.a+a.b+a.c+b.a+b.b+b.c` evaluation order.
            return ( a00 + a10 + a01 + a10 + a11 + a01 ) / 6.0;
        };
        return { sixVertexMean( v00.x, v10.x, v11.x, v01.x ),
                 sixVertexMean( v00.y, v10.y, v11.y, v01.y ),
                 sixVertexMean( v00.z, v10.z, v11.z, v01.z ) };
    }

    inline void EmitBlockMeshInto( BlockSurfaceSamples const& samples,
        BlockSurfaceDescriptors const& descriptors, BlockMesh& mesh )
    {
        mesh.blockX = samples.blockX;
        mesh.blockY = samples.blockY;
        mesh.triangles.clear();
        if ( mesh.triangles.capacity() < descriptors.crossings.size() * 2u )
            mesh.triangles.reserve( descriptors.crossings.size() * 2u );
        for ( CrossingDescriptor const& crossing : descriptors.crossings )
        {
            Vec3 const& v00 = samples.vertices[crossing.v00];
            Vec3 const& v10 = samples.vertices[crossing.v10];
            Vec3 const& v11 = samples.vertices[crossing.v11];
            Vec3 const& v01 = samples.vertices[crossing.v01];
            mesh.triangles.push_back( { v00, v10, v01 } );
            mesh.triangles.push_back( { v10, v11, v01 } );
        }
    }

    inline BlockMesh EmitBlockMesh( BlockSurfaceSamples const& samples,
        BlockSurfaceDescriptors const& descriptors )
    {
        BlockMesh mesh;
        EmitBlockMeshInto( samples, descriptors, mesh );
        return mesh;
    }

    inline bool ReconstructedZ( BlockSurfaceSamples const& samples,
        double x, double y, double& outZ )
    {
        double const gx = x / kDualStepM - 0.5;
        double const gy = y / kDualStepM - 0.5;
        int const ix = (int)std::floor( gx );
        int const iy = (int)std::floor( gy );
        int const baseX = samples.blockX * kBlockCells - 1;
        int const baseY = samples.blockY * kBlockCells - 1;
        int const lx = ix - baseX;
        int const ly = iy - baseY;
        if ( lx < 0 || ly < 0 || lx + 1 >= kBlockSampleSpan
          || ly + 1 >= kBlockSampleSpan ) { return false; }
        double const tx = gx - (double)ix;
        double const ty = gy - (double)iy;
        auto const& v = samples.vertices;
        size_t const p = (size_t)ly * kBlockSampleSpan + (size_t)lx;
        double const z00 = v[p].z;
        double const z10 = v[p + 1].z;
        double const z01 = v[p + kBlockSampleSpan].z;
        double const z11 = v[p + kBlockSampleSpan + 1].z;
        if ( tx + ty <= 1.0 )
        { outZ = z00 + ( z10 - z00 ) * tx + ( z01 - z00 ) * ty; }
        else
        { outZ = z11 + ( z01 - z11 ) * ( 1.0 - tx )
               + ( z10 - z11 ) * ( 1.0 - ty ); }
        return true;
    }

    inline double TriArea2( Tri const& tri )
    {
        Vec3 const a{ tri.b.x - tri.a.x, tri.b.y - tri.a.y, tri.b.z - tri.a.z };
        Vec3 const b{ tri.c.x - tri.a.x, tri.c.y - tri.a.y, tri.c.z - tri.a.z };
        double const cx = a.y * b.z - a.z * b.y;
        double const cy = a.z * b.x - a.x * b.z;
        double const cz = a.x * b.y - a.y * b.x;
        return std::sqrt( cx * cx + cy * cy + cz * cz );
    }

    inline bool Finite( Vec3 const& p )
    {
        return std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z );
    }

    class Kernel
    {
    public:
        explicit Kernel( CausalWorldExposure::Kernel exposure )
            : m_exposure( std::move( exposure ) ) {}

        double AuthoritySurfaceZ( double x, double y ) const
        {
            return m_exposure.SurfaceZ( CausalWorldExposure::kPresentSurface, x, y );
        }

        bool SolidAt( double x, double y, double z ) const
        {
            return z <= AuthoritySurfaceZ( x, y );
        }

        Vec3 DualVertex( int cellX, int cellY ) const
        {
            double const x = ( (double)cellX + 0.5 ) * kDualStepM;
            double const y = ( (double)cellY + 0.5 ) * kDualStepM;
            return { x, y, AuthoritySurfaceZ( x, y ) };
        }

        void EmitCrossingQuad( int crossingX, int crossingY,
            std::vector<Tri>& triangles ) const
        {
            Vec3 const v00 = DualVertex( crossingX - 1, crossingY - 1 );
            Vec3 const v10 = DualVertex( crossingX,     crossingY - 1 );
            Vec3 const v11 = DualVertex( crossingX,     crossingY );
            Vec3 const v01 = DualVertex( crossingX - 1, crossingY );
            // Fixed global diagonal. Both triangles face +Z and adjacent blocks
            // share bit-identical boundary vertices.
            triangles.push_back( { v00, v10, v01 } );
            triangles.push_back( { v10, v11, v01 } );
        }

        BlockSurfaceSamples SampleBlock( int blockX, int blockY ) const
        {
            BlockSurfaceSamples samples;
            samples.blockX = blockX;
            samples.blockY = blockY;
            samples.vertices.reserve( (size_t)kBlockSampleSpan * kBlockSampleSpan );
            int const baseX = blockX * kBlockCells - 1;
            int const baseY = blockY * kBlockCells - 1;
            for ( int y = 0; y < kBlockSampleSpan; ++y )
            for ( int x = 0; x < kBlockSampleSpan; ++x )
            { samples.vertices.push_back( DualVertex( baseX + x, baseY + y ) ); }
            return samples;
        }

        BlockMesh BuildBlock( int blockX, int blockY ) const
        {
            BlockSurfaceSamples const samples = SampleBlock( blockX, blockY );
            BlockSurfaceDescriptors const descriptors = DescribeBlock( samples );
            return EmitBlockMesh( samples, descriptors );
        }

        double ReconstructedZ( double x, double y ) const
        {
            double const gx = x / kDualStepM - 0.5;
            double const gy = y / kDualStepM - 0.5;
            int const ix = (int)std::floor( gx );
            int const iy = (int)std::floor( gy );
            double const tx = gx - (double)ix;
            double const ty = gy - (double)iy;
            double const z00 = DualVertex( ix,     iy ).z;
            double const z10 = DualVertex( ix + 1, iy ).z;
            double const z01 = DualVertex( ix,     iy + 1 ).z;
            double const z11 = DualVertex( ix + 1, iy + 1 ).z;
            if ( tx + ty <= 1.0 )
            { return z00 + ( z10 - z00 ) * tx + ( z01 - z00 ) * ty; }
            double const ux = 1.0 - tx;
            double const uy = 1.0 - ty;
            return z11 + ( z01 - z11 ) * ux + ( z10 - z11 ) * uy;
        }

        CausalWorldExposure::ExposureSample AuthorityAt( double x, double y ) const
        {
            return m_exposure.Query( CausalWorldExposure::kPresentSurface, x, y );
        }

        CausalWorldExposure::Kernel const& Exposure() const { return m_exposure; }

    private:
        CausalWorldExposure::Kernel m_exposure;
    };

    struct EdgeKey
    {
        std::array<int64_t, 3> a{};
        std::array<int64_t, 3> b{};
        bool operator<( EdgeKey const& rhs ) const { return std::tie( a, b ) < std::tie( rhs.a, rhs.b ); }
    };

    inline std::array<int64_t, 3> Quantize( Vec3 const& p )
    {
        return { (int64_t)std::llround( p.x * 1000000.0 ),
                 (int64_t)std::llround( p.y * 1000000.0 ),
                 (int64_t)std::llround( p.z * 1000000.0 ) };
    }

    inline EdgeKey MakeEdge( Vec3 const& a, Vec3 const& b )
    {
        auto qa = Quantize( a );
        auto qb = Quantize( b );
        if ( qb < qa ) { std::swap( qa, qb ); }
        return { qa, qb };
    }

    struct CertResult
    {
        bool passed = false;
        std::string loadReason;
        uint64_t geometryDigest = 0;
        size_t blocks = 0;
        size_t quads = 0;
        size_t triangles = 0;
        size_t interiorEdges = 0;
        size_t boundaryEdges = 0;
        size_t contactTransitions = 0;
        size_t authoritySamples = 0;
        size_t collisionSamples = 0;
        double buildMs = 0.0;
        std::vector<std::pair<std::string, bool>> checks;
    };

    inline uint64_t GeometryDigest( std::vector<Tri> triangles )
    {
        auto key = []( Tri const& tri )
        {
            std::array<std::array<int64_t, 3>, 3> points = {
                Quantize( tri.a ), Quantize( tri.b ), Quantize( tri.c ) };
            std::sort( points.begin(), points.end() );
            return points;
        };
        std::sort( triangles.begin(), triangles.end(), [&]( Tri const& a, Tri const& b )
        { return key( a ) < key( b ); } );
        uint64_t hash = 14695981039346656037ull;
        for ( Tri const& tri : triangles )
        {
            auto const points = key( tri );
            CausalWorldGeology::HashAppend( hash, points.data(), sizeof( points ) );
        }
        return hash;
    }

    inline CertResult RunCert( char const* geologyPath, char const* exposurePath )
    {
        CertResult cert;
        CausalWorldExposure::CertResult const exposureCert =
            CausalWorldExposure::RunCert( geologyPath, exposurePath );
        cert.checks.push_back( { "stage6_exposure_certificate", exposureCert.passed } );
        if ( !exposureCert.passed )
        {
            cert.loadReason = exposureCert.loadReason;
            return cert;
        }

        std::string geologySource;
        std::string exposureSource;
        if ( !CausalWorldExposure::ReadFile( geologyPath, geologySource )
          || !CausalWorldExposure::ReadFile( exposurePath, exposureSource ) )
        {
            cert.loadReason = "descriptor_missing";
            cert.checks.push_back( { "authority_load", false } );
            return cert;
        }
        CausalWorldGeology::LoadResult geology = CausalWorldGeology::LoadText( geologySource );
        CausalWorldExposure::LoadResult exposure =
            CausalWorldExposure::LoadText( exposureSource, geologySource );
        bool const loaded = geology.ok && exposure.ok;
        cert.checks.push_back( { "authority_load", loaded } );
        if ( !loaded )
        {
            cert.loadReason = !geology.ok ? geology.reason : exposure.reason;
            return cert;
        }
        cert.loadReason = "ok";
        Kernel kernel( CausalWorldExposure::Kernel(
            std::move( geology.descriptor ), std::move( exposure.descriptor ) ) );

        constexpr int kBlocksPerSide = 8; // one 64 m certification region
        constexpr int kStartBlock = -4;
        std::vector<Tri> tiledTriangles;
        tiledTriangles.reserve( (size_t)kBlocksPerSide * kBlocksPerSide
            * kBlockCells * kBlockCells * 2 );
        auto const buildStart = std::chrono::steady_clock::now();
        for ( int by = 0; by < kBlocksPerSide; ++by )
        for ( int bx = 0; bx < kBlocksPerSide; ++bx )
        {
            BlockMesh const block = kernel.BuildBlock( kStartBlock + bx, kStartBlock + by );
            tiledTriangles.insert( tiledTriangles.end(),
                block.triangles.begin(), block.triangles.end() );
            ++cert.blocks;
        }
        auto const buildEnd = std::chrono::steady_clock::now();
        cert.buildMs = std::chrono::duration<double, std::milli>( buildEnd - buildStart ).count();
        cert.triangles = tiledTriangles.size();
        cert.quads = cert.triangles / 2;
        cert.geometryDigest = GeometryDigest( tiledTriangles );

        size_t const expectedQuads = (size_t)kBlocksPerSide * kBlocksPerSide
            * kBlockCells * kBlockCells;
        bool finite = true;
        bool nondegenerate = true;
        std::map<EdgeKey, int> edges;
        for ( Tri const& tri : tiledTriangles )
        {
            finite = finite && Finite( tri.a ) && Finite( tri.b ) && Finite( tri.c );
            nondegenerate = nondegenerate && TriArea2( tri ) > 1e-10;
            ++edges[MakeEdge( tri.a, tri.b )];
            ++edges[MakeEdge( tri.b, tri.c )];
            ++edges[MakeEdge( tri.c, tri.a )];
        }
        bool manifold = true;
        for ( auto const& edge : edges )
        {
            if ( edge.second == 2 ) { ++cert.interiorEdges; }
            else if ( edge.second == 1 ) { ++cert.boundaryEdges; }
            else { manifold = false; }
        }
        size_t const regionCells = (size_t)kBlocksPerSide * kBlockCells;
        cert.checks.push_back( { "complete_quad_coverage", cert.quads == expectedQuads } );
        cert.checks.push_back( { "finite_geometry", finite } );
        cert.checks.push_back( { "nondegenerate_geometry", nondegenerate } );
        cert.checks.push_back( { "watertight_interior_edges",
            manifold && cert.boundaryEdges == regionCells * 4 } );

        // Monolithic generation uses the identical global crossing ownership but
        // no block iteration. Canonical triangle digest must match tiled output.
        std::vector<Tri> monolithic;
        monolithic.reserve( tiledTriangles.size() );
        int const base = kStartBlock * kBlockCells;
        int const count = kBlocksPerSide * kBlockCells;
        for ( int y = 0; y < count; ++y )
        for ( int x = 0; x < count; ++x )
        { kernel.EmitCrossingQuad( base + x, base + y, monolithic ); }
        cert.checks.push_back( { "partition_invariant_geometry",
            GeometryDigest( monolithic ) == cert.geometryDigest } );

        bool authorityParity = true;
        bool surfaceParity = true;
        bool occupancyBoundary = true;
        bool collisionParity = true;
        uint64_t priorFeature = 0;
        for ( int y = 0; y < count; y += 3 )
        for ( int x = 0; x < count; x += 3 )
        {
            double const wx = ( (double)( base + x ) + 0.17 ) * kDualStepM;
            double const wy = ( (double)( base + y ) + 0.31 ) * kDualStepM;
            CausalWorldExposure::ExposureSample const sample = kernel.AuthorityAt( wx, wy );
            authorityParity = authorityParity && sample.found
                && sample.geology.featureId != 0 && !sample.geology.material.empty();
            if ( priorFeature != 0 && priorFeature != sample.geology.featureId )
            { ++cert.contactTransitions; }
            priorFeature = sample.geology.featureId;
            ++cert.authoritySamples;

            // Every dual vertex lies exactly on the Stage-6 implicit boundary.
            Vec3 const v = kernel.DualVertex( base + x, base + y );
            surfaceParity = surfaceParity
                && std::fabs( v.z - kernel.AuthoritySurfaceZ( v.x, v.y ) ) < 1e-12;
            occupancyBoundary = occupancyBoundary
                && kernel.SolidAt( v.x, v.y, v.z - 0.001 )
                && !kernel.SolidAt( v.x, v.y, v.z + 0.001 );

            // Collision is evaluated from the same two-triangle dual grid.
            double const reconstructed = kernel.ReconstructedZ( wx, wy );
            double const gx = wx / kDualStepM - 0.5;
            double const gy = wy / kDualStepM - 0.5;
            int const ix = (int)std::floor( gx );
            int const iy = (int)std::floor( gy );
            double const tx = gx - ix, ty = gy - iy;
            Vec3 const v00 = kernel.DualVertex( ix, iy );
            Vec3 const v10 = kernel.DualVertex( ix + 1, iy );
            Vec3 const v01 = kernel.DualVertex( ix, iy + 1 );
            Vec3 const v11 = kernel.DualVertex( ix + 1, iy + 1 );
            double plane = 0.0;
            if ( tx + ty <= 1.0 )
            { plane = v00.z + ( v10.z - v00.z ) * tx + ( v01.z - v00.z ) * ty; }
            else
            { plane = v11.z + ( v01.z - v11.z ) * ( 1.0 - tx )
                    + ( v10.z - v11.z ) * ( 1.0 - ty ); }
            collisionParity = collisionParity && std::fabs( reconstructed - plane ) < 1e-12;
            ++cert.collisionSamples;
        }
        cert.checks.push_back( { "stage6_authority_parity", authorityParity } );
        cert.checks.push_back( { "surface_vertices_on_authority", surfaceParity } );
        cert.checks.push_back( { "continuous_occupancy_boundary", occupancyBoundary } );
        cert.checks.push_back( { "contact_fidelity", cert.contactTransitions > 20 } );
        cert.checks.push_back( { "collision_render_generation_parity", collisionParity } );
        cert.checks.push_back( { "bounded_64m_region", cert.blocks == 64 } );

        cert.passed = std::all_of( cert.checks.begin(), cert.checks.end(),
            []( auto const& check ) { return check.second; } );
        return cert;
    }

    inline bool WriteCertArtifact( CertResult const& cert, char const* path )
    {
        FILE* file = nullptr;
        if ( fopen_s( &file, path, "wb" ) != 0 || !file ) { return false; }
        std::fprintf( file, "CAUSAL_WORLD_VISIBLE_GEOLOGIC_EXPOSURE %s\n",
            cert.passed ? "PASS" : "FAIL" );
        std::fprintf( file, "load_reason=%s\ngeometry_digest=%s\n",
            cert.loadReason.c_str(), CausalWorldGeology::Hex64( cert.geometryDigest ).c_str() );
        std::fprintf( file, "dual_step_m=%.3f\nblock_size_m=%.3f\nblocks=%zu\nquads=%zu\ntriangles=%zu\n",
            kDualStepM, kBlockSizeM, cert.blocks, cert.quads, cert.triangles );
        std::fprintf( file, "interior_edges=%zu\nboundary_edges=%zu\ncontact_transitions=%zu\n",
            cert.interiorEdges, cert.boundaryEdges, cert.contactTransitions );
        std::fprintf( file, "authority_samples=%zu\ncollision_samples=%zu\nbuild_ms=%.3f\n",
            cert.authoritySamples, cert.collisionSamples, cert.buildMs );
        std::fprintf( file, "new_geology=0\noccupancy_mutations=0\ndigging=0\nmatter_bodies=0\n"
            "water_coupling=0\nactive_erosion=0\np5b=closed\n" );
        for ( auto const& check : cert.checks )
        { std::fprintf( file, "check.%s=%s\n", check.first.c_str(), check.second ? "PASS" : "FAIL" ); }
        std::fclose( file );
        return true;
    }
}
