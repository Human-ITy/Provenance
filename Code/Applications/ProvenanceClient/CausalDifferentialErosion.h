#pragma once

// Stage 8: compiled erosion work spent through the existing Stage-5 geology.
// The two controls differ only in material resistance. Every moved surface is
// re-queried against geology; this module never carries a cached material label.

#include "CausalVisibleExposure.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace CausalDifferentialErosion
{
    constexpr char const* kExpectedRegion = "causal_world_differential_erosion_floor";

    enum class Control : uint8_t { EqualResistance, DifferentialResistance };

    struct Program
    {
        uint64_t worldIdentityHash = 0;
        std::string worldgenId;
        uint32_t worldgenVersion = 0;
        uint32_t schemaVersion = 0;
        std::string regionKey;
        uint64_t geologyDescriptorDigest = 0;
        uint64_t surfaceProgramDigest = 0;
        uint32_t authorityRevision = 0;
        double equalResistance = 1.0;
        double sandstoneResistance = 1.0;
        double shaleResistance = 1.0;
        double workDatumM = 0.0;
        double workAmplitudeM = 0.0;
        double workWavelengthM = 1.0;
        double workAzimuthDeg = 0.0;
        double integrationStepM = 0.125;
        size_t descriptorBytes = 0;
    };

    struct LoadResult
    {
        bool ok = false;
        std::string reason;
        Program program;
    };

    inline bool ReadFile( char const* path, std::string& out )
    {
        std::ifstream input( path, std::ios::binary );
        if ( !input ) { return false; }
        std::ostringstream bytes;
        bytes << input.rdbuf();
        out = bytes.str();
        return true;
    }

    inline LoadResult LoadText( std::string const& source,
        CausalWorldGeology::Descriptor const& geology,
        CausalWorldExposure::Descriptor const& exposure,
        std::string const& geologySource,
        std::string const& expectedRegion = kExpectedRegion )
    {
        LoadResult result;
        result.program.descriptorBytes = source.size();
        std::unordered_map<std::string, std::string> fields;
        std::istringstream stream( source );
        std::string line;
        bool magic = false;
        while ( std::getline( stream, line ) )
        {
            if ( !line.empty() && line.back() == '\r' ) { line.pop_back(); }
            if ( line.empty() || line[0] == '#' ) { continue; }
            if ( !magic )
            {
                magic = line == "PROVENANCE_CAUSAL_DIFFERENTIAL_EROSION_V1";
                if ( !magic ) { result.reason = "bad_magic"; return result; }
                continue;
            }
            size_t const eq = line.find( '=' );
            if ( eq == std::string::npos ) { result.reason = "malformed_line"; return result; }
            std::string const key = line.substr( 0, eq );
            if ( fields.count( key ) ) { result.reason = "duplicate_key:" + key; return result; }
            fields.emplace( key, line.substr( eq + 1 ) );
        }
        if ( !magic ) { result.reason = "missing_magic"; return result; }
        auto get = [&]( char const* key ) -> std::string const*
        {
            auto const it = fields.find( key );
            return it == fields.end() ? nullptr : &it->second;
        };
        std::string const* value = get( "world_identity_hash" );
        if ( !value || !CausalWorldGeology::ParseHex64( *value, result.program.worldIdentityHash ) )
        { result.reason = "invalid_world_identity_hash"; return result; }
        value = get( "worldgen_id" );
        if ( !value ) { result.reason = "missing_worldgen_id"; return result; }
        result.program.worldgenId = *value;
        value = get( "worldgen_version" );
        if ( !value || !CausalWorldGeology::ParseU32( *value, result.program.worldgenVersion ) )
        { result.reason = "invalid_worldgen_version"; return result; }
        value = get( "schema_version" );
        if ( !value || !CausalWorldGeology::ParseU32( *value, result.program.schemaVersion ) )
        { result.reason = "invalid_schema_version"; return result; }
        value = get( "region_key" );
        if ( !value ) { result.reason = "missing_region_key"; return result; }
        result.program.regionKey = *value;
        value = get( "geology_descriptor_digest" );
        if ( !value || !CausalWorldGeology::ParseHex64( *value, result.program.geologyDescriptorDigest ) )
        { result.reason = "invalid_geology_digest"; return result; }
        value = get( "surface_program_digest" );
        if ( !value || !CausalWorldGeology::ParseHex64( *value, result.program.surfaceProgramDigest ) )
        { result.reason = "invalid_surface_digest"; return result; }
        value = get( "authority_revision" );
        if ( !value || !CausalWorldGeology::ParseU32( *value, result.program.authorityRevision ) )
        { result.reason = "invalid_authority_revision"; return result; }
        auto number = [&]( char const* key, double& destination )
        {
            std::string const* field = get( key );
            return field && CausalWorldGeology::ParseDouble( *field, destination );
        };
        if ( !number( "equal_resistance", result.program.equalResistance )
          || !number( "sandstone_resistance", result.program.sandstoneResistance )
          || !number( "shale_resistance", result.program.shaleResistance )
          || !number( "work_datum_m", result.program.workDatumM )
          || !number( "work_amplitude_m", result.program.workAmplitudeM )
          || !number( "work_wavelength_m", result.program.workWavelengthM )
          || !number( "work_azimuth_deg", result.program.workAzimuthDeg )
          || !number( "integration_step_m", result.program.integrationStepM ) )
        { result.reason = "invalid_program_values"; return result; }

        if ( result.program.worldIdentityHash != geology.authority.worldIdentityHash
          || result.program.worldIdentityHash != exposure.worldIdentityHash )
        { result.reason = "world_identity_mismatch"; return result; }
        if ( result.program.worldgenId != CausalWorldGeology::kExpectedWorldgenId
          || result.program.worldgenVersion != 1 )
        { result.reason = "unsupported_worldgen"; return result; }
        if ( result.program.schemaVersion != 1 )
        { result.reason = "unsupported_schema"; return result; }
        if ( result.program.regionKey != expectedRegion )
        { result.reason = "wrong_region_key"; return result; }
        if ( result.program.authorityRevision == 0 )
        { result.reason = "stale_revision"; return result; }
        if ( result.program.geologyDescriptorDigest != CausalWorldGeology::HashText( geologySource )
          || result.program.surfaceProgramDigest != exposure.surfaceProgramDigest )
        { result.reason = "authority_link_mismatch"; return result; }
        if ( result.program.equalResistance <= 0.0
          || result.program.sandstoneResistance <= result.program.shaleResistance
          || result.program.shaleResistance <= 0.0
          || result.program.workDatumM <= std::fabs( result.program.workAmplitudeM )
          || result.program.workWavelengthM <= 0.0
          || result.program.integrationStepM <= 0.0
          || result.program.integrationStepM > 0.25 )
        { result.reason = "invalid_causal_contract"; return result; }
        result.ok = true;
        result.reason = "ok";
        return result;
    }

    // Derived dual-surface acceleration. Not a lifetime map of every dual
    // vertex ever queried. Stage-15 WrapIntoRegion happens before DualVertex,
    // so keys are wrapped analytical cells (4096 m tile). Z is a pure function
    // of those wrapped coords plus immutable gen params, so sharing across
    // tiled world instances is legitimate. Residency is the active 192 m live
    // envelope + apron + lookahead, measured in that same wrapped cell space
    // with toroidal distance so a seam-straddling window stays one neighborhood.
    // Absolute world location is the published player origin (wrapped for
    // lookup); travel distance must not grow this cache.
    inline std::atomic<int> g_dualCacheResidencyCellX{ 0 };
    inline std::atomic<int> g_dualCacheResidencyCellY{ 0 };
    inline std::atomic<uint32_t> g_dualCacheResidencySeq{ 0 };

    inline void PublishDualCacheResidency( int cellX, int cellY )
    {
        g_dualCacheResidencyCellX.store( cellX, std::memory_order_relaxed );
        g_dualCacheResidencyCellY.store( cellY, std::memory_order_relaxed );
        g_dualCacheResidencySeq.fetch_add( 1, std::memory_order_release );
    }

    inline bool ReadDualCacheResidency( int& cellX, int& cellY )
    {
        if ( g_dualCacheResidencySeq.load( std::memory_order_acquire ) == 0 )
        { return false; }
        cellX = g_dualCacheResidencyCellX.load( std::memory_order_relaxed );
        cellY = g_dualCacheResidencyCellY.load( std::memory_order_relaxed );
        return true;
    }

    struct DualCacheSnapshot
    {
        size_t entries = 0;
        size_t buckets = 0;
        size_t estimatedBytes = 0;
        uint64_t insertions = 0;
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t evictions = 0;
        uint64_t generationResets = 0;
        size_t highWaterEntries = 0;
    };

    inline int DualCacheFloorDiv( int a, int b )
    {
        int q = a / b;
        int r = a % b;
        if ( r != 0 && ( ( a < 0 ) != ( b < 0 ) ) ) { --q; }
        return q;
    }

    inline int DualCacheTorusDelta( int a, int b, int period )
    {
        int d = a - b;
        int const half = period / 2;
        if ( d > half ) { d -= period; }
        if ( d < -half ) { d += period; }
        if ( d > half ) { d -= period; }
        if ( d < -half ) { d += period; }
        return d;
    }

    class DualVertexCache
    {
    public:
        // 192 m live + 8 m apron + 16 m lookahead + 32 m snap + 8 m dual skirt.
        static constexpr int kRadiusCells = 512;
        static constexpr int kTileCells = 64;
        static constexpr int kSnapCells = 32;
        static constexpr int kPeriodCells = 8192; // 4096 m / 0.5 m
        static constexpr size_t kNodeBytes = sizeof( uint32_t ) + sizeof( double ) + 32u;

        DualVertexCache() = default;
        DualVertexCache( DualVertexCache const& ) = delete;
        DualVertexCache& operator=( DualVertexCache const& ) = delete;
        DualVertexCache( DualVertexCache&& o ) noexcept { MoveFrom( std::move( o ) ); }
        DualVertexCache& operator=( DualVertexCache&& o ) noexcept
        {
            if ( this != &o ) { MoveFrom( std::move( o ) ); }
            return *this;
        }

        bool Find( int cellX, int cellY, double& z )
        {
            NoteResidency( cellX, cellY );
            auto const tile = m_tiles.find( TileKey( cellX, cellY ) );
            if ( tile == m_tiles.end() )
            {
                m_misses.fetch_add( 1, std::memory_order_relaxed );
                return false;
            }
            auto const found = tile->second.find( LocalKey( cellX, cellY ) );
            if ( found == tile->second.end() )
            {
                m_misses.fetch_add( 1, std::memory_order_relaxed );
                return false;
            }
            z = found->second;
            m_hits.fetch_add( 1, std::memory_order_relaxed );
            return true;
        }

        void Insert( int cellX, int cellY, double z )
        {
            NoteResidency( cellX, cellY );
            uint64_t const tk = TileKey( cellX, cellY );
            auto& tile = m_tiles[tk];
            auto const placed = tile.emplace( LocalKey( cellX, cellY ), z );
            if ( !placed.second ) { return; }
            m_insertions.fetch_add( 1, std::memory_order_relaxed );
            size_t const n = m_entries.fetch_add( 1, std::memory_order_relaxed ) + 1;
            size_t hw = m_highWater.load( std::memory_order_relaxed );
            while ( n > hw && !m_highWater.compare_exchange_weak(
                hw, n, std::memory_order_relaxed ) ) {}
            RefreshEstimate();
        }

        void ForceCold()
        {
            m_tiles.clear();
            m_entries.store( 0, std::memory_order_relaxed );
            m_hasOrigin = false;
            m_generationResets.fetch_add( 1, std::memory_order_relaxed );
            RefreshEstimate();
        }

        DualCacheSnapshot Snapshot() const
        {
            DualCacheSnapshot s;
            s.entries = m_entries.load( std::memory_order_relaxed );
            s.buckets = m_buckets.load( std::memory_order_relaxed );
            s.estimatedBytes = m_estimatedBytes.load( std::memory_order_relaxed );
            s.insertions = m_insertions.load( std::memory_order_relaxed );
            s.hits = m_hits.load( std::memory_order_relaxed );
            s.misses = m_misses.load( std::memory_order_relaxed );
            s.evictions = m_evictions.load( std::memory_order_relaxed );
            s.generationResets = m_generationResets.load( std::memory_order_relaxed );
            s.highWaterEntries = m_highWater.load( std::memory_order_relaxed );
            return s;
        }

        size_t size() const { return m_entries.load( std::memory_order_relaxed ); }

    private:
        using TileMap = std::unordered_map<uint32_t, double>;
        std::unordered_map<uint64_t, TileMap> m_tiles;
        std::atomic<size_t> m_entries{ 0 };
        std::atomic<size_t> m_buckets{ 0 };
        std::atomic<size_t> m_estimatedBytes{ 0 };
        std::atomic<uint64_t> m_insertions{ 0 };
        std::atomic<uint64_t> m_hits{ 0 };
        std::atomic<uint64_t> m_misses{ 0 };
        std::atomic<uint64_t> m_evictions{ 0 };
        std::atomic<uint64_t> m_generationResets{ 0 };
        std::atomic<size_t> m_highWater{ 0 };
        int m_originX = 0;
        int m_originY = 0;
        bool m_hasOrigin = false;

        static int Snap( int cell )
        {
            return DualCacheFloorDiv( cell, kSnapCells ) * kSnapCells;
        }

        static uint64_t TileKey( int cellX, int cellY )
        {
            int const tx = DualCacheFloorDiv( cellX, kTileCells );
            int const ty = DualCacheFloorDiv( cellY, kTileCells );
            return ( (uint64_t)(uint32_t)tx << 32 ) ^ (uint32_t)ty;
        }

        static uint32_t LocalKey( int cellX, int cellY )
        {
            int const lx = cellX - DualCacheFloorDiv( cellX, kTileCells ) * kTileCells;
            int const ly = cellY - DualCacheFloorDiv( cellY, kTileCells ) * kTileCells;
            return (uint32_t)lx | ( (uint32_t)ly << 16 );
        }

        static void DecodeTile( uint64_t key, int& tx, int& ty )
        {
            tx = (int)(int32_t)(uint32_t)( key >> 32 );
            ty = (int)(int32_t)(uint32_t)key;
        }

        void NoteResidency( int cellX, int cellY )
        {
            int ox = cellX, oy = cellY;
            if ( !ReadDualCacheResidency( ox, oy ) )
            {
                ox = cellX;
                oy = cellY;
            }
            EnsureResidency( Snap( ox ), Snap( oy ) );
        }

        void EnsureResidency( int originX, int originY )
        {
            if ( m_hasOrigin && originX == m_originX && originY == m_originY )
            { return; }
            bool const first = !m_hasOrigin;
            m_originX = originX;
            m_originY = originY;
            m_hasOrigin = true;
            if ( first ) { return; }
            EvictOutside();
        }

        void EvictOutside()
        {
            size_t dropped = 0;
            for ( auto it = m_tiles.begin(); it != m_tiles.end(); )
            {
                int tx = 0, ty = 0;
                DecodeTile( it->first, tx, ty );
                int const cx = tx * kTileCells + kTileCells / 2;
                int const cy = ty * kTileCells + kTileCells / 2;
                int const dx = DualCacheTorusDelta( cx, m_originX, kPeriodCells );
                int const dy = DualCacheTorusDelta( cy, m_originY, kPeriodCells );
                int const chebyshev = (std::max)( std::abs( dx ), std::abs( dy ) );
                if ( chebyshev > kRadiusCells )
                {
                    dropped += it->second.size();
                    it = m_tiles.erase( it );
                }
                else { ++it; }
            }
            if ( dropped == 0 ) { RefreshEstimate(); return; }
            m_evictions.fetch_add( dropped, std::memory_order_relaxed );
            size_t const have = m_entries.load( std::memory_order_relaxed );
            m_entries.store( have > dropped ? have - dropped : 0,
                std::memory_order_relaxed );
            if ( m_tiles.empty() )
            { m_generationResets.fetch_add( 1, std::memory_order_relaxed ); }
            RefreshEstimate();
        }

        void RefreshEstimate()
        {
            size_t const e = m_entries.load( std::memory_order_relaxed );
            size_t const t = m_tiles.size();
            size_t const buckets = m_tiles.bucket_count() + e + t;
            m_buckets.store( buckets, std::memory_order_relaxed );
            m_estimatedBytes.store(
                e * kNodeBytes + t * 80u + buckets * sizeof( void* ),
                std::memory_order_relaxed );
        }

        void MoveFrom( DualVertexCache&& o ) noexcept
        {
            m_tiles = std::move( o.m_tiles );
            m_entries.store( o.m_entries.exchange( 0, std::memory_order_relaxed ),
                std::memory_order_relaxed );
            m_buckets.store( o.m_buckets.exchange( 0, std::memory_order_relaxed ),
                std::memory_order_relaxed );
            m_estimatedBytes.store( o.m_estimatedBytes.exchange( 0,
                std::memory_order_relaxed ), std::memory_order_relaxed );
            m_insertions.store( o.m_insertions.exchange( 0, std::memory_order_relaxed ),
                std::memory_order_relaxed );
            m_hits.store( o.m_hits.exchange( 0, std::memory_order_relaxed ),
                std::memory_order_relaxed );
            m_misses.store( o.m_misses.exchange( 0, std::memory_order_relaxed ),
                std::memory_order_relaxed );
            m_evictions.store( o.m_evictions.exchange( 0, std::memory_order_relaxed ),
                std::memory_order_relaxed );
            m_generationResets.store( o.m_generationResets.exchange( 0,
                std::memory_order_relaxed ), std::memory_order_relaxed );
            m_highWater.store( o.m_highWater.exchange( 0, std::memory_order_relaxed ),
                std::memory_order_relaxed );
            m_originX = o.m_originX;
            m_originY = o.m_originY;
            m_hasOrigin = o.m_hasOrigin;
            o.m_tiles.clear();
            o.m_hasOrigin = false;
        }
    };

    struct ReliefSample
    {
        bool found = false;
        double baseSurfaceZ = 0.0;
        double surfaceZ = 0.0;
        double erosionWork = 0.0;
        CausalWorldGeology::GeoSample geology;
    };

    class Kernel
    {
    public:
        Kernel( CausalWorldExposure::Kernel exposure, Program program )
            : m_exposure( std::move( exposure ) ), m_program( std::move( program ) ) {}

        double RegionalWork( double x, double y ) const
        {
            constexpr double kPi = 3.14159265358979323846;
            double const a = m_program.workAzimuthDeg * kPi / 180.0;
            double const along = x * std::cos( a ) + y * std::sin( a );
            double const cross = -x * std::sin( a ) + y * std::cos( a );
            return m_program.workDatumM
                + m_program.workAmplitudeM * std::sin( 2.0 * kPi * along / m_program.workWavelengthM )
                + 0.35 * m_program.workAmplitudeM
                    * std::sin( 2.0 * kPi * cross / ( m_program.workWavelengthM * 1.7 ) );
        }

        double Resistance( Control control, std::string const& material ) const
        {
            if ( control == Control::EqualResistance ) { return m_program.equalResistance; }
            if ( material == "sandstone" ) { return m_program.sandstoneResistance; }
            if ( material == "shale" ) { return m_program.shaleResistance; }
            return m_program.equalResistance;
        }

        double SurfaceZ( Control control, double x, double y ) const
        {
            ++m_surfaceCompilations;
            return CompileSurfaceZ( control, x, y );
        }

        // Pure authoritative compilation used by package workers.  The
        // interactive query path keeps its diagnostic counter and vertex cache,
        // but a background package build must not mutate either one while the
        // frame thread is resolving collision or inspector samples.
        double CompileSurfaceZ( Control control, double x, double y ) const
        {
            double z = m_exposure.SurfaceZ( CausalWorldExposure::kPresentSurface, x, y );
            double remaining = RegionalWork( x, y );
            auto const column = m_exposure.Geology().PrepareColumn( x, y );
            for ( int i = 0; i < 1024 && remaining > 1e-12; ++i )
            {
                CausalWorldGeology::Formation const* const body =
                    m_exposure.Geology().FormationAt( column, z - 0.001 );
                if ( !body ) { break; }
                double const resistance = Resistance( control, body->material );
                double const dz = (std::min)( m_program.integrationStepM, remaining / resistance );
                z -= dz;
                remaining -= dz * resistance;
            }
            return z;
        }

        ReliefSample Query( Control control, double x, double y ) const
        {
            ReliefSample sample;
            sample.baseSurfaceZ = m_exposure.SurfaceZ(
                CausalWorldExposure::kPresentSurface, x, y );
            sample.erosionWork = RegionalWork( x, y );
            sample.surfaceZ = SurfaceZ( control, x, y );
            sample.geology = m_exposure.Geology().Query( x, y, sample.surfaceZ - 0.001 );
            sample.found = sample.geology.found;
            return sample;
        }

        CausalVisibleExposure::Vec3 DualVertex( Control control, int cellX, int cellY ) const
        {
            DualVertexCache& cache = control == Control::EqualResistance
                ? m_equalDualCache : m_differentialDualCache;
            double const x = ( (double)cellX + 0.5 ) * CausalVisibleExposure::kDualStepM;
            double const y = ( (double)cellY + 0.5 ) * CausalVisibleExposure::kDualStepM;
            double cachedZ = 0.0;
            if ( cache.Find( cellX, cellY, cachedZ ) ) { return { x, y, cachedZ }; }
            double const z = SurfaceZ( control, x, y );
            cache.Insert( cellX, cellY, z );
            return { x, y, z };
        }

        void EmitCrossingQuad( Control control, int crossingX, int crossingY,
            std::vector<CausalVisibleExposure::Tri>& triangles ) const
        {
            auto const v00 = DualVertex( control, crossingX - 1, crossingY - 1 );
            auto const v10 = DualVertex( control, crossingX, crossingY - 1 );
            auto const v11 = DualVertex( control, crossingX, crossingY );
            auto const v01 = DualVertex( control, crossingX - 1, crossingY );
            triangles.push_back( { v00, v10, v01 } );
            triangles.push_back( { v10, v11, v01 } );
        }

        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(
            Control control, int bx, int by ) const
        {
            CausalVisibleExposure::BlockSurfaceSamples samples;
            samples.blockX = bx;
            samples.blockY = by;
            samples.vertices.reserve( (size_t)CausalVisibleExposure::kBlockSampleSpan
                * CausalVisibleExposure::kBlockSampleSpan );
            int const baseX = bx * CausalVisibleExposure::kBlockCells - 1;
            int const baseY = by * CausalVisibleExposure::kBlockCells - 1;
            for ( int y = 0; y < CausalVisibleExposure::kBlockSampleSpan; ++y )
            for ( int x = 0; x < CausalVisibleExposure::kBlockSampleSpan; ++x )
            {
                int const cellX = baseX + x, cellY = baseY + y;
                double const wx = ( (double)cellX + 0.5 )
                    * CausalVisibleExposure::kDualStepM;
                double const wy = ( (double)cellY + 0.5 )
                    * CausalVisibleExposure::kDualStepM;
                samples.vertices.push_back(
                    { wx, wy, CompileSurfaceZ( control, wx, wy ) } );
            }
            return samples;
        }

        CausalVisibleExposure::BlockMesh BuildBlock( Control control, int bx, int by ) const
        {
            auto const samples = SampleBlock( control, bx, by );
            auto const descriptors = CausalVisibleExposure::DescribeBlock( samples );
            return CausalVisibleExposure::EmitBlockMesh( samples, descriptors );
        }

        double ReconstructedZ( Control control, double x, double y ) const
        {
            double const gx = x / CausalVisibleExposure::kDualStepM - 0.5;
            double const gy = y / CausalVisibleExposure::kDualStepM - 0.5;
            int const ix = (int)std::floor( gx ), iy = (int)std::floor( gy );
            double const tx = gx - ix, ty = gy - iy;
            double const z00 = DualVertex( control, ix, iy ).z;
            double const z10 = DualVertex( control, ix + 1, iy ).z;
            double const z01 = DualVertex( control, ix, iy + 1 ).z;
            double const z11 = DualVertex( control, ix + 1, iy + 1 ).z;
            if ( tx + ty <= 1.0 )
            { return z00 + ( z10 - z00 ) * tx + ( z01 - z00 ) * ty; }
            return z11 + ( z01 - z11 ) * ( 1.0 - tx )
                + ( z10 - z11 ) * ( 1.0 - ty );
        }

        CausalWorldGeology::GeoSample GeologyAt( double x, double y, double z ) const
        { return m_exposure.Geology().Query( x, y, z - 0.001 ); }
        CausalWorldExposure::Kernel const& Exposure() const { return m_exposure; }
        Program const& GetProgram() const { return m_program; }
        uint64_t SurfaceCompilationCount() const { return m_surfaceCompilations; }
        size_t CachedVertexCount( Control control ) const
        {
            return control == Control::EqualResistance
                ? m_equalDualCache.size() : m_differentialDualCache.size();
        }
        DualCacheSnapshot DualCacheStats( Control control ) const
        {
            return control == Control::EqualResistance
                ? m_equalDualCache.Snapshot() : m_differentialDualCache.Snapshot();
        }
        DualCacheSnapshot DualCacheStatsTotal() const
        {
            DualCacheSnapshot a = m_differentialDualCache.Snapshot();
            DualCacheSnapshot const b = m_equalDualCache.Snapshot();
            a.entries += b.entries;
            a.buckets += b.buckets;
            a.estimatedBytes += b.estimatedBytes;
            a.insertions += b.insertions;
            a.hits += b.hits;
            a.misses += b.misses;
            a.evictions += b.evictions;
            a.generationResets += b.generationResets;
            if ( b.highWaterEntries > a.highWaterEntries )
            { a.highWaterEntries = b.highWaterEntries; }
            return a;
        }
        void ForceColdDualCaches() const
        {
            m_equalDualCache.ForceCold();
            m_differentialDualCache.ForceCold();
        }

    private:
        CausalWorldExposure::Kernel m_exposure;
        Program m_program;
        mutable DualVertexCache m_equalDualCache;
        mutable DualVertexCache m_differentialDualCache;
        mutable uint64_t m_surfaceCompilations = 0;
    };

    struct CertResult
    {
        bool passed = false;
        std::string loadReason;
        uint64_t geometryDigest = 0;
        size_t samples = 0;
        double sandstoneMeanReliefM = 0.0;
        double shaleMeanReliefM = 0.0;
        double sandstoneMeanRetainedM = 0.0;
        double sandstoneMeanRecessedM = 0.0;
        double shaleMeanRetainedM = 0.0;
        double shaleMeanRecessedM = 0.0;
        double contrastSeparationM = 0.0;
        double equalizedContrastM = 0.0;
        double sandstonePositiveFraction = 0.0;
        double shaleRecessedFraction = 0.0;
        double longestCoherentRidgeM = 0.0;
        double equalSandstoneExposureFraction = 0.0;
        double differentialSandstoneExposureFraction = 0.0;
        std::vector<std::pair<std::string, bool>> checks;
    };

    inline CertResult RunCert( char const* geologyPath, char const* exposurePath,
        char const* erosionPath )
    {
        CertResult cert;
        CausalVisibleExposure::CertResult const stage7 =
            CausalVisibleExposure::RunCert( geologyPath, exposurePath );
        cert.checks.push_back( { "stage7_visible_exposure_certificate", stage7.passed } );
        if ( !stage7.passed ) { cert.loadReason = stage7.loadReason; return cert; }

        std::string geologySource, exposureSource, erosionSource;
        if ( !ReadFile( geologyPath, geologySource ) || !ReadFile( exposurePath, exposureSource )
          || !ReadFile( erosionPath, erosionSource ) )
        { cert.loadReason = "descriptor_missing"; return cert; }
        auto geology = CausalWorldGeology::LoadText( geologySource );
        auto exposure = CausalWorldExposure::LoadText( exposureSource, geologySource );
        bool loaded = geology.ok && exposure.ok;
        LoadResult erosion;
        if ( loaded )
        { erosion = LoadText( erosionSource, geology.descriptor, exposure.descriptor, geologySource ); }
        loaded = loaded && erosion.ok;
        cert.loadReason = loaded ? "ok" : ( !geology.ok ? geology.reason
            : ( !exposure.ok ? exposure.reason : erosion.reason ) );
        cert.checks.push_back( { "linked_authority_load", loaded } );
        if ( !loaded ) { return cert; }

        Kernel kernel( CausalWorldExposure::Kernel(
            geology.descriptor, exposure.descriptor ), erosion.program );

        std::string badLink = erosionSource;
        CausalWorldGeology::ReplaceFirst( badLink,
            "geology_descriptor_digest=" + CausalWorldGeology::Hex64(
                erosion.program.geologyDescriptorDigest ),
            "geology_descriptor_digest=0000000000000000" );
        cert.checks.push_back( { "refuse_geology_link_mismatch",
            !LoadText( badLink, geology.descriptor, exposure.descriptor, geologySource ).ok } );
        std::string badSurface = erosionSource;
        CausalWorldGeology::ReplaceFirst( badSurface,
            "surface_program_digest=" + CausalWorldGeology::Hex64(
                erosion.program.surfaceProgramDigest ),
            "surface_program_digest=0000000000000000" );
        cert.checks.push_back( { "refuse_surface_link_mismatch",
            !LoadText( badSurface, geology.descriptor, exposure.descriptor, geologySource ).ok } );
        std::string equalizedDifferential = erosionSource;
        CausalWorldGeology::ReplaceFirst( equalizedDifferential,
            "sandstone_resistance=2.0", "sandstone_resistance=0.65" );
        cert.checks.push_back( { "refuse_non_differential_program",
            !LoadText( equalizedDifferential, geology.descriptor,
                exposure.descriptor, geologySource ).ok } );
        cert.checks.push_back( { "refuse_wrong_region",
            !LoadText( erosionSource, geology.descriptor, exposure.descriptor,
                geologySource, "wrong" ).ok } );
        constexpr int kSide = 64;
        constexpr int kStart = -32;
        int sandstone = 0, shale = 0, sandstonePositive = 0, shaleRecessed = 0;
        int equalSandstone = 0, differentialSandstone = 0;
        double equalSandstoneResidual = 0.0, equalShaleResidual = 0.0;
        int equalSandstoneResidualCount = 0, equalShaleResidualCount = 0;
        bool equalControlExact = true, directRequery = true, catalogOnly = true;
        std::unordered_set<uint64_t> catalog;
        for ( auto const& formation : kernel.Exposure().Geology().GetDescriptor().formations )
        { catalog.insert( formation.featureId ); }
        std::vector<uint8_t> retained( kSide * kSide, 0 );
        for ( int iy = 0; iy < kSide; ++iy )
        for ( int ix = 0; ix < kSide; ++ix )
        {
            double const x = kStart + ix + 0.5, y = kStart + iy + 0.5;
            ReliefSample const equal = kernel.Query( Control::EqualResistance, x, y );
            ReliefSample const differential = kernel.Query( Control::DifferentialResistance, x, y );
            ++cert.samples;
            equalControlExact = equalControlExact
                && std::fabs( equal.surfaceZ
                    - ( equal.baseSurfaceZ - equal.erosionWork / erosion.program.equalResistance ) ) < 1e-9;
            double const equalResidual = equal.surfaceZ
                - ( equal.baseSurfaceZ - equal.erosionWork / erosion.program.equalResistance );
            auto const direct = kernel.GeologyAt( x, y, differential.surfaceZ );
            directRequery = directRequery && differential.found
                && direct.featureId == differential.geology.featureId
                && direct.material == differential.geology.material;
            catalogOnly = catalogOnly && catalog.count( differential.geology.featureId ) != 0;
            if ( equal.geology.material == "sandstone" )
            {
                ++equalSandstone;
                equalSandstoneResidual += equalResidual;
                ++equalSandstoneResidualCount;
            }
            else if ( equal.geology.material == "shale" )
            {
                equalShaleResidual += equalResidual;
                ++equalShaleResidualCount;
            }
            if ( differential.geology.material == "sandstone" ) { ++differentialSandstone; }
            double const relief = differential.surfaceZ - equal.surfaceZ;
            if ( differential.geology.material == "sandstone" )
            {
                cert.sandstoneMeanReliefM += relief;
                cert.sandstoneMeanRetainedM += (std::max)( 0.0, relief );
                cert.sandstoneMeanRecessedM += (std::max)( 0.0, -relief );
                ++sandstone;
                if ( relief > 0.25 )
                { ++sandstonePositive; retained[iy * kSide + ix] = 1; }
            }
            else if ( differential.geology.material == "shale" )
            {
                cert.shaleMeanReliefM += relief;
                cert.shaleMeanRetainedM += (std::max)( 0.0, relief );
                cert.shaleMeanRecessedM += (std::max)( 0.0, -relief );
                ++shale;
                if ( relief < -0.25 ) { ++shaleRecessed; }
            }
        }
        if ( sandstone )
        {
            cert.sandstoneMeanReliefM /= sandstone;
            cert.sandstoneMeanRetainedM /= sandstone;
            cert.sandstoneMeanRecessedM /= sandstone;
            cert.sandstonePositiveFraction = (double)sandstonePositive / sandstone;
        }
        if ( shale )
        {
            cert.shaleMeanReliefM /= shale;
            cert.shaleMeanRetainedM /= shale;
            cert.shaleMeanRecessedM /= shale;
            cert.shaleRecessedFraction = (double)shaleRecessed / shale;
        }
        cert.contrastSeparationM = cert.sandstoneMeanReliefM - cert.shaleMeanReliefM;
        double const equalSandstoneMean = equalSandstoneResidualCount
            ? equalSandstoneResidual / equalSandstoneResidualCount : 0.0;
        double const equalShaleMean = equalShaleResidualCount
            ? equalShaleResidual / equalShaleResidualCount : 0.0;
        cert.equalizedContrastM = equalSandstoneMean - equalShaleMean;
        cert.equalSandstoneExposureFraction = (double)equalSandstone / cert.samples;
        cert.differentialSandstoneExposureFraction = (double)differentialSandstone / cert.samples;

        int longest = 0;
        for ( int y = 0; y < kSide; ++y )
        {
            int run = 0;
            for ( int x = 0; x < kSide; ++x )
            { run = retained[y * kSide + x] ? run + 1 : 0; longest = (std::max)( longest, run ); }
        }
        for ( int x = 0; x < kSide; ++x )
        {
            int run = 0;
            for ( int y = 0; y < kSide; ++y )
            { run = retained[y * kSide + x] ? run + 1 : 0; longest = (std::max)( longest, run ); }
        }
        cert.longestCoherentRidgeM = (double)longest;

        cert.checks.push_back( { "equal_control_exact_common_forcing", equalControlExact } );
        cert.checks.push_back( { "surface_material_is_final_geology_requery", directRequery } );
        cert.checks.push_back( { "no_features_minted_or_relabelled", catalogOnly } );
        cert.checks.push_back( { "differential_relief_separation", cert.contrastSeparationM > 0.75 } );
        cert.checks.push_back( { "equalized_resistance_removes_landform",
            std::fabs( cert.equalizedContrastM ) < 1e-12 } );
        cert.checks.push_back( { "sandstone_positive_area_coherent",
            cert.sandstonePositiveFraction > 0.55 && cert.longestCoherentRidgeM >= 6.0 } );
        cert.checks.push_back( { "shale_recessed_area", cert.shaleRecessedFraction > 0.45 } );

        // The differential surface retains Stage-7 topology and must be invariant
        // to package iteration: one 64 m patch, tiled versus monolithic.
        std::vector<CausalVisibleExposure::Tri> tiled;
        constexpr int kBlocks = 8, kStartBlock = -4;
        for ( int by = 0; by < kBlocks; ++by )
        for ( int bx = 0; bx < kBlocks; ++bx )
        {
            auto const block = kernel.BuildBlock( Control::DifferentialResistance,
                kStartBlock + bx, kStartBlock + by );
            tiled.insert( tiled.end(), block.triangles.begin(), block.triangles.end() );
        }
        std::vector<CausalVisibleExposure::Tri> monolithic;
        int const base = kStartBlock * CausalVisibleExposure::kBlockCells;
        int const count = kBlocks * CausalVisibleExposure::kBlockCells;
        for ( int y = 0; y < count; ++y )
        for ( int x = 0; x < count; ++x )
        { kernel.EmitCrossingQuad( Control::DifferentialResistance, base + x, base + y, monolithic ); }
        cert.geometryDigest = CausalVisibleExposure::GeometryDigest( tiled );
        cert.checks.push_back( { "partition_invariant_geometry",
            CausalVisibleExposure::GeometryDigest( monolithic ) == cert.geometryDigest } );
        std::map<CausalVisibleExposure::EdgeKey, int> edges;
        for ( auto const& tri : tiled )
        {
            ++edges[CausalVisibleExposure::MakeEdge( tri.a, tri.b )];
            ++edges[CausalVisibleExposure::MakeEdge( tri.b, tri.c )];
            ++edges[CausalVisibleExposure::MakeEdge( tri.c, tri.a )];
        }
        size_t boundary = 0; bool manifold = true;
        for ( auto const& edge : edges )
        { if ( edge.second == 1 ) { ++boundary; } else if ( edge.second != 2 ) { manifold = false; } }
        cert.checks.push_back( { "watertight_stage7_topology",
            manifold && boundary == (size_t)count * 4 } );

        bool collisionParity = true;
        for ( int y = -28; y <= 28; y += 7 )
        for ( int x = -28; x <= 28; x += 7 )
        {
            double const wx = x + 0.17, wy = y + 0.31;
            double const z = kernel.ReconstructedZ(
                Control::DifferentialResistance, wx, wy );
            double const gx = wx / CausalVisibleExposure::kDualStepM - 0.5;
            double const gy = wy / CausalVisibleExposure::kDualStepM - 0.5;
            int const ix = (int)std::floor( gx ), iy = (int)std::floor( gy );
            double const tx = gx - ix, ty = gy - iy;
            auto const v00 = kernel.DualVertex( Control::DifferentialResistance, ix, iy );
            auto const v10 = kernel.DualVertex( Control::DifferentialResistance, ix + 1, iy );
            auto const v01 = kernel.DualVertex( Control::DifferentialResistance, ix, iy + 1 );
            auto const v11 = kernel.DualVertex( Control::DifferentialResistance, ix + 1, iy + 1 );
            double const plane = tx + ty <= 1.0
                ? v00.z + ( v10.z - v00.z ) * tx + ( v01.z - v00.z ) * ty
                : v11.z + ( v01.z - v11.z ) * ( 1.0 - tx )
                    + ( v10.z - v11.z ) * ( 1.0 - ty );
            collisionParity = collisionParity && std::isfinite( z )
                && std::fabs( z - plane ) < 1e-12;
        }
        cert.checks.push_back( { "collision_render_generation_parity", collisionParity } );
        cert.checks.push_back( { "bounded_64m_region", tiled.size() == 32768 } );
        cert.checks.push_back( { "geological_ancestry_descriptor_unchanged",
            erosion.program.geologyDescriptorDigest == CausalWorldGeology::HashText( geologySource ) } );

        // Derived acceleration, not world truth: a cache hit, a forced cold
        // recompute, and CompileSurfaceZ must agree exactly.
        struct DualProbe { int x, y; double z = 0.0; };
        std::vector<DualProbe> probes;
        for ( int y = -40; y <= 40; y += 5 )
        for ( int x = -40; x <= 40; x += 5 )
        { probes.push_back( { x, y, 0.0 } ); }
        bool cacheMatchesCompile = true;
        for ( auto& probe : probes )
        {
            double const wx = ( (double)probe.x + 0.5 ) * CausalVisibleExposure::kDualStepM;
            double const wy = ( (double)probe.y + 0.5 ) * CausalVisibleExposure::kDualStepM;
            probe.z = kernel.DualVertex( Control::DifferentialResistance, probe.x, probe.y ).z;
            cacheMatchesCompile = cacheMatchesCompile
                && std::fabs( probe.z - kernel.CompileSurfaceZ(
                    Control::DifferentialResistance, wx, wy ) ) < 1e-12;
        }
        kernel.ForceColdDualCaches();
        bool coldMatchesWarm = kernel.CachedVertexCount( Control::DifferentialResistance ) == 0;
        for ( auto const& probe : probes )
        {
            double const z = kernel.DualVertex(
                Control::DifferentialResistance, probe.x, probe.y ).z;
            coldMatchesWarm = coldMatchesWarm && std::fabs( z - probe.z ) < 1e-12;
        }
        int ox = 0, oy = 0;
        ReadDualCacheResidency( ox, oy );
        PublishDualCacheResidency( ox + DualVertexCache::kPeriodCells / 2,
            oy + DualVertexCache::kPeriodCells / 2 );
        kernel.DualVertex( Control::DifferentialResistance, ox + DualVertexCache::kPeriodCells / 2,
            oy + DualVertexCache::kPeriodCells / 2 );
        PublishDualCacheResidency( probes.front().x, probes.front().y );
        bool evictRecompute = true;
        for ( auto const& probe : probes )
        {
            double const z = kernel.DualVertex(
                Control::DifferentialResistance, probe.x, probe.y ).z;
            evictRecompute = evictRecompute && std::fabs( z - probe.z ) < 1e-12;
        }
        if ( g_dualCacheResidencySeq.load( std::memory_order_relaxed ) )
        { PublishDualCacheResidency( ox, oy ); }
        cert.checks.push_back( { "dual_cache_matches_compile_surface", cacheMatchesCompile } );
        cert.checks.push_back( { "dual_cache_enabled_equals_forced_cold", coldMatchesWarm } );
        cert.checks.push_back( { "dual_cache_evict_recomputes_identically", evictRecompute } );

        cert.passed = std::all_of( cert.checks.begin(), cert.checks.end(),
            []( auto const& check ) { return check.second; } );
        return cert;
    }

    inline bool WriteCertArtifact( CertResult const& cert, char const* path )
    {
        FILE* file = nullptr;
        if ( fopen_s( &file, path, "wb" ) != 0 || !file ) { return false; }
        std::fprintf( file, "CAUSAL_WORLD_DIFFERENTIAL_EROSION %s\n",
            cert.passed ? "PASS" : "FAIL" );
        std::fprintf( file, "load_reason=%s\ngeometry_digest=%s\nsamples=%zu\n",
            cert.loadReason.c_str(), CausalWorldGeology::Hex64( cert.geometryDigest ).c_str(), cert.samples );
        std::fprintf( file, "sandstone_mean_relief_m=%.6f\nshale_mean_relief_m=%.6f\n"
            "sandstone_mean_retained_m=%.6f\nsandstone_mean_recessed_m=%.6f\n"
            "shale_mean_retained_m=%.6f\nshale_mean_recessed_m=%.6f\n"
            "contrast_separation_m=%.6f\nequalized_contrast_m=%.6f\n",
            cert.sandstoneMeanReliefM, cert.shaleMeanReliefM,
            cert.sandstoneMeanRetainedM, cert.sandstoneMeanRecessedM,
            cert.shaleMeanRetainedM, cert.shaleMeanRecessedM,
            cert.contrastSeparationM, cert.equalizedContrastM );
        std::fprintf( file, "sandstone_positive_area_fraction=%.6f\n"
            "shale_recessed_area_fraction=%.6f\nlongest_coherent_ridge_m=%.3f\n",
            cert.sandstonePositiveFraction, cert.shaleRecessedFraction,
            cert.longestCoherentRidgeM );
        std::fprintf( file, "equal_sandstone_exposure_fraction=%.6f\n"
            "differential_sandstone_exposure_fraction=%.6f\n",
            cert.equalSandstoneExposureFraction, cert.differentialSandstoneExposureFraction );
        std::fprintf( file, "new_geology=0\nactive_erosion=0\nwater_coupling=0\n"
            "sediment_transport=0\nintrusion=0\nfault=0\nmineralization=0\np5b=closed\n" );
        for ( auto const& check : cert.checks )
        { std::fprintf( file, "check.%s=%s\n", check.first.c_str(), check.second ? "PASS" : "FAIL" ); }
        std::fclose( file );
        return true;
    }
}
