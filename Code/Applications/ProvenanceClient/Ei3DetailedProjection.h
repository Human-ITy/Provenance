#pragma once

// EI3 client-side projection admission and predictive residency.  This owns no
// world generation or mutation law: every live detail chunk is admitted only
// from an engine-bound, checksum-valid snapshot/delta.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MacroPageAuthority.h"

namespace Ei3
{
    constexpr char const* kProjectionSchemaId = "fablescript.detailed-chunk-projection";
    constexpr int kProjectionSchemaVersion = 1;
    constexpr char const* kProjectionProfile = "surface-strata-8m-v1";
    constexpr int kChunkEdgeM = 64;
    constexpr int kLatticeSide = 9;
    constexpr int kLatticeCount = kLatticeSide * kLatticeSide;
    constexpr float kVoxelEdgeM = 0.125f;
    constexpr float kFillDenominator = 255.f;
    constexpr size_t kMaxResidentChunks = 256;

    struct ChunkCoord
    {
        int x = 0, y = 0;
        bool operator==( ChunkCoord const& rhs ) const { return x == rhs.x && y == rhs.y; }
    };

    struct ChunkCoordHash
    {
        size_t operator()( ChunkCoord const& c ) const
        {
            uint64_t const ux = (uint32_t)c.x, uy = (uint32_t)c.y;
            uint64_t v = ( ux << 32 ) ^ uy;
            v ^= v >> 33; v *= 0xff51afd7ed558ccdULL;
            v ^= v >> 33; v *= 0xc4ceb9fe1a85ec53ULL;
            return (size_t)( v ^ ( v >> 33 ) );
        }
    };

    inline int FloorChunk( float metres )
    { return (int)std::floor( (double)metres / (double)kChunkEdgeM ); }

    struct SurfaceColumn
    {
        int ix = 0, iy = 0;
        int surfaceHeightQ = 0;
        int surfaceVoxelZ = 0;
        int surfaceFill = 0;
        std::string dominantSurfaceFamily;
        std::string dominantMaterialId;
        std::string formationId;
        std::string macroFeatureId;
        std::string macroLandformId;
        std::string parentRangeId;
        std::string macroPeakId;
        std::string lithologyClass;
        std::string substrateClass;
        std::string canonicalJson;
    };

    struct MatterSurfaceSample
    {
        float z = 0.f;
        std::string dominantMaterialId;
        std::string dominantSurfaceFamily;
        std::string formationId;
        std::string macroFeatureId;
        std::string macroLandformId;
        std::string parentRangeId;
        std::string macroPeakId;
        std::string lithologyClass;
        std::string substrateClass;
        ChunkCoord chunkCoord;
        int64_t chunkRevision = -1;
    };

    struct Snapshot
    {
        std::string worldUuid;
        std::string macroGenesisDigest;
        std::string worldBaselineDigest;
        std::string chunkId;
        ChunkCoord coord;
        int64_t worldRevision = -1;
        int64_t chunkRevision = -1;
        std::string baselineChunkDigest;
        std::string checksum;
        std::vector<SurfaceColumn> columns;
        std::string canonicalJson;
    };

    namespace Detail
    {
        inline bool ExtractString( std::string const& json, char const* key, std::string& out )
        {
            std::string const needle = std::string( "\"" ) + key + "\":\"";
            size_t p = json.find( needle );
            if ( p == std::string::npos ) { return false; }
            p += needle.size();
            size_t const e = json.find( '"', p );
            if ( e == std::string::npos ) { return false; }
            out.assign( json, p, e - p );
            return true;
        }

        inline bool IsHex64( std::string const& value )
        {
            if ( value.size() != 64 ) { return false; }
            for ( char c : value )
                if ( !( c >= '0' && c <= '9' ) && !( c >= 'a' && c <= 'f' ) ) { return false; }
            return true;
        }

        inline bool ExtractInt64( std::string const& json, char const* key, int64_t& out )
        {
            std::string const needle = std::string( "\"" ) + key + "\":";
            size_t p = json.find( needle );
            if ( p == std::string::npos ) { return false; }
            p += needle.size();
            bool negative = p < json.size() && json[p] == '-';
            if ( negative ) { ++p; }
            if ( p == json.size() || json[p] < '0' || json[p] > '9' ) { return false; }
            int64_t value = 0;
            while ( p < json.size() && json[p] >= '0' && json[p] <= '9' )
            { value = value * 10 + ( json[p++] - '0' ); }
            out = negative ? -value : value;
            return true;
        }

        inline bool ExtractInt( std::string const& json, char const* key, int& out )
        {
            int64_t value = 0;
            if ( !ExtractInt64( json, key, value )
              || value < (std::numeric_limits<int>::min)()
              || value > (std::numeric_limits<int>::max)() ) { return false; }
            out = (int)value;
            return true;
        }

        inline bool ExtractArray( std::string const& json, char const* key, std::string& out )
        {
            std::string const needle = std::string( "\"" ) + key + "\":[";
            size_t p = json.find( needle );
            if ( p == std::string::npos ) { return false; }
            p += needle.size() - 1;
            size_t const begin = p;
            int depth = 0; bool quoted = false, escaped = false;
            for ( ; p < json.size(); ++p )
            {
                char const c = json[p];
                if ( quoted )
                {
                    if ( escaped ) { escaped = false; }
                    else if ( c == '\\' ) { escaped = true; }
                    else if ( c == '"' ) { quoted = false; }
                    continue;
                }
                if ( c == '"' ) { quoted = true; continue; }
                if ( c == '[' ) { ++depth; }
                else if ( c == ']' && --depth == 0 )
                { out.assign( json, begin, p - begin + 1 ); return true; }
            }
            return false;
        }

        inline bool ExtractObject( std::string const& json, char const* key, std::string& out )
        {
            std::string const needle = std::string( "\"" ) + key + "\":{";
            size_t p = json.find( needle );
            if ( p == std::string::npos ) { return false; }
            p += needle.size() - 1;
            size_t const begin = p;
            int depth = 0; bool quoted = false, escaped = false;
            for ( ; p < json.size(); ++p )
            {
                char const c = json[p];
                if ( quoted )
                {
                    if ( escaped ) { escaped = false; }
                    else if ( c == '\\' ) { escaped = true; }
                    else if ( c == '"' ) { quoted = false; }
                    continue;
                }
                if ( c == '"' ) { quoted = true; continue; }
                if ( c == '{' ) { ++depth; }
                else if ( c == '}' && --depth == 0 )
                { out.assign( json, begin, p - begin + 1 ); return true; }
            }
            return false;
        }

        inline bool SplitObjects( std::string const& array, std::vector<std::string>& out )
        {
            out.clear();
            if ( array.size() < 2 || array.front() != '[' || array.back() != ']' ) { return false; }
            size_t p = 1;
            while ( p + 1 < array.size() )
            {
                if ( array[p] == ',' ) { ++p; continue; }
                if ( array[p] != '{' ) { return false; }
                size_t const begin = p; int depth = 0; bool quoted = false, escaped = false;
                for ( ; p < array.size(); ++p )
                {
                    char const c = array[p];
                    if ( quoted )
                    {
                        if ( escaped ) { escaped = false; }
                        else if ( c == '\\' ) { escaped = true; }
                        else if ( c == '"' ) { quoted = false; }
                        continue;
                    }
                    if ( c == '"' ) { quoted = true; continue; }
                    if ( c == '{' ) { ++depth; }
                    else if ( c == '}' && --depth == 0 )
                    { out.emplace_back( array.substr( begin, ++p - begin ) ); break; }
                }
                if ( depth != 0 ) { return false; }
            }
            return true;
        }

        inline bool ExtractPair( std::string const& json, char const* key, ChunkCoord& out )
        {
            std::string array;
            if ( !ExtractArray( json, key, array ) ) { return false; }
            return std::sscanf( array.c_str(), "[%d,%d]", &out.x, &out.y ) == 2;
        }

        inline bool CanonicalWithoutChecksum( std::string canonical, std::string const& checksum,
                                              std::string& out )
        {
            std::string const field = "\"checksum\":\"" + checksum + "\",";
            size_t const p = canonical.find( field );
            if ( p == std::string::npos ) { return false; }
            canonical.erase( p, field.size() );
            out = std::move( canonical );
            return true;
        }

        inline bool ReplaceNumberField( std::string& json, char const* key,
                                        int64_t oldValue, int64_t newValue )
        {
            std::string const oldText = std::string( "\"" ) + key + "\":"
                + std::to_string( oldValue );
            size_t const p = json.find( oldText );
            if ( p == std::string::npos ) { return false; }
            json.replace( p, oldText.size(), std::string( "\"" ) + key + "\":"
                + std::to_string( newValue ) );
            return true;
        }
    }

    inline bool ParseSnapshot( std::string const& canonicalJson,
                               std::string const& expectedWorldUuid,
                               std::string const& expectedBaselineDigest,
                               Snapshot& out, std::string& failure )
    {
        std::string schema, profile, columnsArray, canonicalNoChecksum;
        int schemaVersion = 0;
        Snapshot candidate;
        bool const fields =
            Detail::ExtractString( canonicalJson, "projection_schema_id", schema )
         && Detail::ExtractInt( canonicalJson, "projection_schema_version", schemaVersion )
         && Detail::ExtractString( canonicalJson, "projection_profile", profile )
         && Detail::ExtractString( canonicalJson, "world_uuid", candidate.worldUuid )
         && Detail::ExtractString( canonicalJson, "macro_genesis_digest", candidate.macroGenesisDigest )
         && Detail::ExtractString( canonicalJson, "world_baseline_digest", candidate.worldBaselineDigest )
         && Detail::ExtractString( canonicalJson, "chunk_id", candidate.chunkId )
         && Detail::ExtractPair( canonicalJson, "chunk_coord", candidate.coord )
         && Detail::ExtractInt64( canonicalJson, "world_revision", candidate.worldRevision )
         && Detail::ExtractInt64( canonicalJson, "chunk_revision", candidate.chunkRevision )
         && Detail::ExtractString( canonicalJson, "baseline_chunk_digest", candidate.baselineChunkDigest )
         && Detail::ExtractString( canonicalJson, "checksum", candidate.checksum )
         && Detail::ExtractArray( canonicalJson, "columns", columnsArray );
        if ( !fields ) { failure = "malformed_detailed_snapshot"; return false; }
        if ( schema != kProjectionSchemaId || schemaVersion != kProjectionSchemaVersion
          || profile != kProjectionProfile )
        { failure = "detailed_projection_schema_mismatch"; return false; }
        if ( candidate.worldUuid != expectedWorldUuid
          || candidate.worldBaselineDigest != expectedBaselineDigest )
        { failure = "detailed_authority_binding_mismatch"; return false; }
        if ( !Detail::IsHex64( candidate.macroGenesisDigest )
          || !Detail::IsHex64( candidate.worldBaselineDigest )
          || !Detail::IsHex64( candidate.baselineChunkDigest )
          || !Detail::IsHex64( candidate.checksum )
          || candidate.chunkId.empty() || candidate.worldRevision < 0 || candidate.chunkRevision < 0 )
        { failure = "malformed_detailed_identity"; return false; }
        if ( !Detail::CanonicalWithoutChecksum( canonicalJson, candidate.checksum, canonicalNoChecksum )
          || MacroPageAuthority::Detail::Sha256( canonicalNoChecksum ) != candidate.checksum )
        { failure = "detailed_snapshot_checksum_mismatch"; return false; }
        std::vector<std::string> columnObjects;
        if ( !Detail::SplitObjects( columnsArray, columnObjects )
          || columnObjects.size() != kLatticeCount )
        { failure = "incomplete_detailed_surface_lattice"; return false; }
        for ( std::string const& object : columnObjects )
        {
            SurfaceColumn column;
            ChunkCoord voxel;
            if ( !Detail::ExtractPair( object, "voxel_xy", voxel )
              || !Detail::ExtractInt( object, "surface_height_q", column.surfaceHeightQ )
              || !Detail::ExtractInt( object, "surface_voxel_z", column.surfaceVoxelZ )
              || !Detail::ExtractInt( object, "surface_fill", column.surfaceFill )
              || !Detail::ExtractString( object, "dominant_surface_family",
                                         column.dominantSurfaceFamily )
              || !Detail::ExtractString( object, "dominant_material_id",
                                         column.dominantMaterialId )
              || !Detail::ExtractString( object, "formation_id", column.formationId )
              || !Detail::ExtractString( object, "macro_feature_id", column.macroFeatureId )
              || !Detail::ExtractString( object, "macro_landform_id", column.macroLandformId )
              || !Detail::ExtractString( object, "parent_range_id", column.parentRangeId )
              || !Detail::ExtractString( object, "macro_peak_id", column.macroPeakId )
              || !Detail::ExtractString( object, "lithology_class", column.lithologyClass )
              || !Detail::ExtractString( object, "substrate_class", column.substrateClass )
              || column.surfaceFill < 0 || column.surfaceFill > 255
              || column.macroFeatureId.empty() )
            { failure = "malformed_detailed_surface_column"; return false; }
            column.ix = voxel.x; column.iy = voxel.y; column.canonicalJson = object;
            candidate.columns.push_back( std::move( column ) );
        }
        candidate.canonicalJson = canonicalJson;
        out = std::move( candidate );
        failure.clear();
        return true;
    }

    inline bool ExtractSnapshots( std::string const& response,
                                  std::vector<std::string>& snapshots )
    {
        std::string array;
        return Detail::ExtractArray( response, "snapshots", array )
            && Detail::SplitObjects( array, snapshots );
    }

    inline bool ApplyDeltaJson( Snapshot const& base, std::string const& deltaJson,
                                std::string const& expectedWorldUuid,
                                std::string const& expectedBaselineDigest,
                                Snapshot& out, std::string& failure )
    {
        std::string schema, worldUuid, baseline, chunkId, baseChecksum, resultChecksum;
        std::string dirtyBounds, changesArray;
        int schemaVersion = 0;
        int64_t baseRevision = -1, newRevision = -1, worldRevision = -1;
        bool const fields = Detail::ExtractString( deltaJson, "projection_schema_id", schema )
            && Detail::ExtractInt( deltaJson, "projection_schema_version", schemaVersion )
            && Detail::ExtractString( deltaJson, "world_uuid", worldUuid )
            && Detail::ExtractString( deltaJson, "world_baseline_digest", baseline )
            && Detail::ExtractString( deltaJson, "chunk_id", chunkId )
            && Detail::ExtractInt64( deltaJson, "base_chunk_revision", baseRevision )
            && Detail::ExtractInt64( deltaJson, "new_chunk_revision", newRevision )
            && Detail::ExtractInt64( deltaJson, "world_revision", worldRevision )
            && Detail::ExtractString( deltaJson, "base_checksum", baseChecksum )
            && Detail::ExtractString( deltaJson, "result_checksum", resultChecksum )
            && Detail::ExtractArray( deltaJson, "dirty_bounds_m", dirtyBounds )
            && Detail::ExtractArray( deltaJson, "changes", changesArray );
        if ( !fields ) { failure = "malformed_detailed_delta"; return false; }
        if ( schema != kProjectionSchemaId || schemaVersion != kProjectionSchemaVersion )
        { failure = "detailed_delta_schema_mismatch"; return false; }
        if ( worldUuid != expectedWorldUuid || baseline != expectedBaselineDigest
          || worldUuid != base.worldUuid || baseline != base.worldBaselineDigest
          || chunkId != base.chunkId )
        { failure = "detailed_delta_authority_binding_mismatch"; return false; }
        if ( baseRevision != base.chunkRevision || baseChecksum != base.checksum
          || newRevision != baseRevision + 1 || worldRevision < base.worldRevision )
        { failure = "detailed_delta_revision_mismatch"; return false; }
        if ( !Detail::IsHex64( baseChecksum ) || !Detail::IsHex64( resultChecksum ) )
        { failure = "malformed_detailed_delta_checksum"; return false; }

        std::vector<std::string> changes;
        if ( !Detail::SplitObjects( changesArray, changes ) )
        { failure = "malformed_detailed_delta_changes"; return false; }
        std::vector<std::string> columns;
        columns.reserve( base.columns.size() );
        for ( SurfaceColumn const& column : base.columns ) { columns.push_back( column.canonicalJson ); }
        for ( std::string const& change : changes )
        {
            std::string kind, column;
            int sampleIndex = -1;
            if ( !Detail::ExtractString( change, "kind", kind ) || kind != "replace_column"
              || !Detail::ExtractInt( change, "sample_index", sampleIndex )
              || sampleIndex < 0 || sampleIndex >= (int)columns.size()
              || !Detail::ExtractObject( change, "column", column ) )
            { failure = "unsupported_detailed_delta_change"; return false; }
            columns[(size_t)sampleIndex] = std::move( column );
        }
        std::string oldColumns;
        if ( !Detail::ExtractArray( base.canonicalJson, "columns", oldColumns ) )
        { failure = "malformed_detailed_delta_base"; return false; }
        std::string newColumns = "[";
        for ( size_t i = 0; i < columns.size(); ++i )
        { if ( i ) { newColumns += ','; } newColumns += columns[i]; }
        newColumns += ']';
        std::string candidate = base.canonicalJson;
        size_t const columnsAt = candidate.find( oldColumns );
        if ( columnsAt == std::string::npos )
        { failure = "malformed_detailed_delta_base"; return false; }
        candidate.replace( columnsAt, oldColumns.size(), newColumns );
        if ( !Detail::ReplaceNumberField( candidate, "chunk_revision", baseRevision, newRevision )
          || !Detail::ReplaceNumberField( candidate, "world_revision", base.worldRevision, worldRevision ) )
        { failure = "malformed_detailed_delta_base"; return false; }
        std::string const oldChecksumField = "\"checksum\":\"" + base.checksum + "\"";
        size_t const checksumAt = candidate.find( oldChecksumField );
        if ( checksumAt == std::string::npos )
        { failure = "malformed_detailed_delta_base"; return false; }
        candidate.replace( checksumAt, oldChecksumField.size(),
                           "\"checksum\":\"" + resultChecksum + "\"" );
        if ( !ParseSnapshot( candidate, expectedWorldUuid, expectedBaselineDigest, out, failure )
          || out.checksum != resultChecksum )
        { if ( failure.empty() ) { failure = "detailed_delta_result_checksum_mismatch"; } return false; }
        return true;
    }

    enum class Priority : int { P0 = 0, P1 = 1, P2 = 2, P3 = 3, P4 = 4, P5 = 5 };

    struct DesiredChunk
    {
        ChunkCoord coord;
        Priority priority = Priority::P5;
        float etaSeconds = std::numeric_limits<float>::infinity();
        float score = 0.f;
    };

    struct PredictiveInput
    {
        float x = 0.f, y = 0.f, z = 0.f;
        float velocityX = 0.f, velocityY = 0.f;
        float cameraForwardX = 0.f, cameraForwardY = 1.f;
        bool walkMode = false;
        bool landingIntent = false;
    };

    struct SyncClaim
    {
        ChunkCoord coord;
        int64_t chunkRevision = 0;
        std::string checksum;
    };

    inline std::vector<DesiredChunk> PlanResidency( PredictiveInput const& input )
    {
        std::unordered_map<ChunkCoord, DesiredChunk, ChunkCoordHash> best;
        ChunkCoord const center{ FloorChunk( input.x ), FloorChunk( input.y ) };
        auto admit = [&]( int x, int y, Priority priority, float eta, float score )
        {
            ChunkCoord const key{ x, y };
            DesiredChunk value{ key, priority, eta, score };
            auto found = best.find( key );
            if ( found == best.end() || (int)priority < (int)found->second.priority
              || ( priority == found->second.priority && score > found->second.score ) )
            { best[key] = value; }
        };

        // P0: current support and landing collar. It can never be displaced by
        // visual-interest work.
        int const supportRadius = ( input.walkMode || input.landingIntent ) ? 1 : 0;
        for ( int oy = -supportRadius; oy <= supportRadius; ++oy )
            for ( int ox = -supportRadius; ox <= supportRadius; ++ox )
                admit( center.x + ox, center.y + oy, Priority::P0, 0.f,
                       10000.f - (float)( std::abs( ox ) + std::abs( oy ) ) );

        float const speed = std::sqrt( input.velocityX * input.velocityX
                                    + input.velocityY * input.velocityY );
        float dx = speed > 0.5f ? input.velocityX / speed : input.cameraForwardX;
        float dy = speed > 0.5f ? input.velocityY / speed : input.cameraForwardY;
        float dlen = std::sqrt( dx * dx + dy * dy );
        if ( dlen < 1e-5f ) { dx = 0.f; dy = 1.f; dlen = 1.f; }
        dx /= dlen; dy /= dlen;
        float const leadM = (std::max)( 192.f, speed * 3.f );
        int const leadChunks = (std::min)( 16, (int)std::ceil( leadM / kChunkEdgeM ) );
        float const lateralX = -dy, lateralY = dx;
        for ( int step = 1; step <= leadChunks; ++step )
        {
            float const px = input.x + dx * ( step * (float)kChunkEdgeM );
            float const py = input.y + dy * ( step * (float)kChunkEdgeM );
            for ( int side = -1; side <= 1; ++side )
            {
                ChunkCoord const c{ FloorChunk( px + lateralX * side * kChunkEdgeM ),
                                    FloorChunk( py + lateralY * side * kChunkEdgeM ) };
                float const eta = speed > .5f ? step * kChunkEdgeM / speed
                                               : std::numeric_limits<float>::infinity();
                admit( c.x, c.y, Priority::P1, eta, 8000.f - step * 10.f - std::abs( side ) );
            }
        }

        // P2 camera/landmark pursuit, including course changes before velocity
        // has caught up. Altitude expands this cheap directional preview.
        float const altitudeLead = (std::min)( 1024.f, 256.f + (std::max)( 0.f, input.z ) * .5f );
        int const sightSteps = (std::max)( 4, (int)std::ceil( altitudeLead / kChunkEdgeM ) );
        for ( int step = 1; step <= sightSteps; ++step )
        {
            ChunkCoord const c{ FloorChunk( input.x + input.cameraForwardX * step * kChunkEdgeM ),
                                FloorChunk( input.y + input.cameraForwardY * step * kChunkEdgeM ) };
            admit( c.x, c.y, Priority::P2, std::numeric_limits<float>::infinity(),
                   6000.f - step * 10.f );
        }

        // P3 local continuity ring; P4/P5 are trailing/speculative and are the
        // first work dropped under pressure.
        for ( int r = 1; r <= 3; ++r )
            for ( int oy = -r; oy <= r; ++oy )
                for ( int ox = -r; ox <= r; ++ox )
                    if ( (std::max)( std::abs( ox ), std::abs( oy ) ) == r )
                        admit( center.x + ox, center.y + oy,
                               r <= 2 ? Priority::P3 : Priority::P5,
                               std::numeric_limits<float>::infinity(), 4000.f - r * 100.f );
        if ( speed > .5f )
        {
            for ( int step = 1; step <= 3; ++step )
                admit( FloorChunk( input.x - dx * step * kChunkEdgeM ),
                       FloorChunk( input.y - dy * step * kChunkEdgeM ), Priority::P4,
                       std::numeric_limits<float>::infinity(), 2000.f - step );
        }
        std::vector<DesiredChunk> result;
        result.reserve( best.size() );
        for ( auto const& value : best ) { result.push_back( value.second ); }
        std::sort( result.begin(), result.end(), []( DesiredChunk const& a, DesiredChunk const& b )
        {
            if ( a.priority != b.priority ) { return (int)a.priority < (int)b.priority; }
            if ( a.etaSeconds != b.etaSeconds ) { return a.etaSeconds < b.etaSeconds; }
            if ( a.score != b.score ) { return a.score > b.score; }
            if ( a.coord.y != b.coord.y ) { return a.coord.y < b.coord.y; }
            return a.coord.x < b.coord.x;
        } );
        return result;
    }

    inline std::string BuildChunksParams( std::vector<ChunkCoord> const& chunks )
    {
        std::string value = "{\"chunks\":[";
        for ( size_t i = 0; i < chunks.size(); ++i )
        {
            if ( i ) { value += ','; }
            value += "[" + std::to_string( chunks[i].x ) + ","
                + std::to_string( chunks[i].y ) + "]";
        }
        value += "]}";
        return value;
    }

    inline std::string BuildSyncParams( std::vector<SyncClaim> const& claims )
    {
        std::string value = "{\"chunks\":[";
        for ( size_t i = 0; i < claims.size(); ++i )
        {
            if ( i ) { value += ','; }
            value += "{\"checksum\":\"" + claims[i].checksum
                + "\",\"chunk_coord\":[" + std::to_string( claims[i].coord.x )
                + "," + std::to_string( claims[i].coord.y )
                + "],\"chunk_revision\":" + std::to_string( claims[i].chunkRevision ) + "}";
        }
        value += "]}";
        return value;
    }

    class Residency
    {
    public:
        enum class Admission { Published, Idempotent, Rejected };

        Admission AdmitSnapshot( std::string const& canonicalJson,
                                 std::string const& worldUuid,
                                 std::string const& baselineDigest,
                                 std::string& failure )
        {
            Snapshot candidate;
            if ( !ParseSnapshot( canonicalJson, worldUuid, baselineDigest, candidate, failure ) )
            { ++rejected_; return Admission::Rejected; }
            auto immutable = std::make_shared<Snapshot const>( std::move( candidate ) );
            std::lock_guard<std::mutex> lock( mutex_ );
            auto const found = live_.find( immutable->coord );
            if ( found != live_.end() )
            {
                Snapshot const& current = *found->second.value;
                if ( immutable->chunkRevision < current.chunkRevision )
                { failure = "stale_chunk_revision"; ++rejected_; return Admission::Rejected; }
                if ( immutable->chunkRevision == current.chunkRevision )
                {
                    if ( immutable->checksum == current.checksum )
                    { found->second.lastUse = ++clock_; return Admission::Idempotent; }
                    failure = "same_revision_different_checksum";
                    ++rejected_; return Admission::Rejected;
                }
                if ( immutable->chunkRevision != current.chunkRevision + 1 )
                { failure = "noncontiguous_chunk_revision"; ++rejected_; return Admission::Rejected; }
            }
            live_[immutable->coord] = { immutable, ++clock_ };
            ++published_;
            failure.clear();
            return Admission::Published;
        }

        Admission AdmitDelta( ChunkCoord coord, std::string const& deltaJson,
                              std::string const& worldUuid,
                              std::string const& baselineDigest,
                              std::string& failure )
        {
            std::shared_ptr<Snapshot const> base;
            {
                std::lock_guard<std::mutex> lock( mutex_ );
                auto const found = live_.find( coord );
                if ( found == live_.end() )
                { failure = "detailed_delta_base_missing"; ++rejected_; return Admission::Rejected; }
                base = found->second.value;
            }
            Snapshot candidate;
            if ( !ApplyDeltaJson( *base, deltaJson, worldUuid, baselineDigest, candidate, failure ) )
            { ++rejected_; return Admission::Rejected; }
            return AdmitSnapshot( candidate.canonicalJson, worldUuid, baselineDigest, failure );
        }

        bool Has( ChunkCoord coord ) const
        {
            std::lock_guard<std::mutex> lock( mutex_ );
            return live_.find( coord ) != live_.end();
        }

        bool GroundHeight( float x, float y, float& z ) const
        {
            MatterSurfaceSample sample;
            if ( !SampleMatterSurface( x, y, sample ) ) { return false; }
            z = sample.z;
            return true;
        }

        bool SampleMatterSurface( float x, float y, MatterSurfaceSample& out ) const
        {
            ChunkCoord const coord{ FloorChunk( x ), FloorChunk( y ) };
            std::shared_ptr<Snapshot const> snapshot;
            {
                std::lock_guard<std::mutex> lock( mutex_ );
                auto found = live_.find( coord );
                if ( found == live_.end() ) { return false; }
                found->second.lastUse = ++clock_;
                snapshot = found->second.value;
            }
            float const localX = x - coord.x * (float)kChunkEdgeM;
            float const localY = y - coord.y * (float)kChunkEdgeM;
            float const gx = (std::max)( 0.f, (std::min)( 7.999f, localX / 8.f ) );
            float const gy = (std::max)( 0.f, (std::min)( 7.999f, localY / 8.f ) );
            int const ix = (std::min)( 7, (int)std::floor( gx ) );
            int const iy = (std::min)( 7, (int)std::floor( gy ) );
            float const tx = gx - ix, ty = gy - iy;
            auto height = [&]( int sx, int sy )
            { return snapshot->columns[(size_t)sy * kLatticeSide + sx].surfaceHeightQ
                    / kFillDenominator * kVoxelEdgeM; };
            float const a = height( ix, iy ), b = height( ix + 1, iy );
            float const c = height( ix, iy + 1 ), d = height( ix + 1, iy + 1 );
            out.z = a * ( 1 - tx ) * ( 1 - ty ) + b * tx * ( 1 - ty )
              + c * ( 1 - tx ) * ty + d * tx * ty;
            int const nearestX = tx < .5f ? ix : ix + 1;
            int const nearestY = ty < .5f ? iy : iy + 1;
            SurfaceColumn const& source = snapshot->columns[
                (size_t)nearestY * kLatticeSide + nearestX];
            out.dominantMaterialId = source.dominantMaterialId;
            out.dominantSurfaceFamily = source.dominantSurfaceFamily;
            out.formationId = source.formationId;
            out.macroFeatureId = source.macroFeatureId;
            out.macroLandformId = source.macroLandformId;
            out.parentRangeId = source.parentRangeId;
            out.macroPeakId = source.macroPeakId;
            out.lithologyClass = source.lithologyClass;
            out.substrateClass = source.substrateClass;
            out.chunkCoord = coord;
            out.chunkRevision = snapshot->chunkRevision;
            return std::isfinite( out.z ) && !out.dominantMaterialId.empty();
        }

        void Prune( ChunkCoord center, int protectRadius, size_t limit = kMaxResidentChunks )
        {
            std::lock_guard<std::mutex> lock( mutex_ );
            while ( live_.size() > limit )
            {
                auto victim = live_.end();
                for ( auto it = live_.begin(); it != live_.end(); ++it )
                {
                    int const distance = (std::max)( std::abs( it->first.x - center.x ),
                                                    std::abs( it->first.y - center.y ) );
                    if ( distance <= protectRadius ) { continue; }
                    if ( victim == live_.end() || it->second.lastUse < victim->second.lastUse )
                    { victim = it; }
                }
                if ( victim == live_.end() ) { break; }
                live_.erase( victim ); ++evicted_;
            }
        }

        size_t Size() const { std::lock_guard<std::mutex> lock( mutex_ ); return live_.size(); }

        std::vector<std::shared_ptr<Snapshot const>> SnapshotsNear(
            ChunkCoord center, int radius ) const
        {
            std::vector<std::shared_ptr<Snapshot const>> result;
            std::lock_guard<std::mutex> lock( mutex_ );
            result.reserve( live_.size() );
            for ( auto const& entry : live_ )
            {
                int const distance = (std::max)( std::abs( entry.first.x - center.x ),
                                                std::abs( entry.first.y - center.y ) );
                if ( distance <= radius ) { result.push_back( entry.second.value ); }
            }
            return result;
        }

        std::vector<SyncClaim> Claims() const
        {
            std::lock_guard<std::mutex> lock( mutex_ );
            std::vector<SyncClaim> result;
            result.reserve( live_.size() );
            for ( auto const& value : live_ )
                result.push_back( { value.first, value.second.value->chunkRevision,
                                    value.second.value->checksum } );
            std::sort( result.begin(), result.end(), []( SyncClaim const& a, SyncClaim const& b )
            { return a.coord.y != b.coord.y ? a.coord.y < b.coord.y : a.coord.x < b.coord.x; } );
            return result;
        }
        void Clear()
        {
            std::lock_guard<std::mutex> lock( mutex_ );
            live_.clear(); ++clock_;
        }
        uint64_t Published() const { return published_; }
        uint64_t Rejected() const { return rejected_; }
        uint64_t Evicted() const { return evicted_; }

    private:
        struct Entry { std::shared_ptr<Snapshot const> value; mutable uint64_t lastUse = 0; };
        mutable std::mutex mutex_;
        mutable uint64_t clock_ = 0;
        std::unordered_map<ChunkCoord, Entry, ChunkCoordHash> live_;
        uint64_t published_ = 0, rejected_ = 0, evicted_ = 0;
    };
}
