#pragma once

// Read-only CAUSAL_WORLD geology kernel floor.
//
// The descriptor is compiled by the Python/Fablescript authority lane. This
// module only validates, indexes, and evaluates it. It deliberately has no
// terrain, occupancy, D2, body, water, or presentation dependencies.

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalWorldGeology
{
    constexpr uint32_t kSupportedSchemaVersion = 1;
    constexpr char const* kExpectedWorldgenId = "provenance_causal_world";
    constexpr char const* kFloorRegionKey = "causal_world_geology_kernel_floor";

    inline uint64_t HashBytes( char const* data, size_t size )
    {
        uint64_t h = 14695981039346656037ull;
        for ( size_t i = 0; i < size; ++i )
        {
            h ^= (uint8_t)data[i];
            h *= 1099511628211ull;
        }
        return h;
    }

    inline uint64_t HashText( std::string const& text )
    {
        return HashBytes( text.data(), text.size() );
    }

    inline std::string Hex64( uint64_t value )
    {
        char out[24] = {};
        std::snprintf( out, sizeof( out ), "%016llx", (unsigned long long)value );
        return out;
    }

    inline bool ParseHex64( std::string const& text, uint64_t& out )
    {
        if ( text.empty() ) { return false; }
        char* end = nullptr;
        unsigned long long const value = std::strtoull( text.c_str(), &end, 16 );
        if ( !end || *end != '\0' ) { return false; }
        out = (uint64_t)value;
        return true;
    }

    inline bool ParseU32( std::string const& text, uint32_t& out )
    {
        if ( text.empty() ) { return false; }
        char* end = nullptr;
        unsigned long const value = std::strtoul( text.c_str(), &end, 10 );
        if ( !end || *end != '\0' || value > std::numeric_limits<uint32_t>::max() ) { return false; }
        out = (uint32_t)value;
        return true;
    }

    inline bool ParseDouble( std::string const& text, double& out )
    {
        if ( text.empty() ) { return false; }
        char* end = nullptr;
        double const value = std::strtod( text.c_str(), &end );
        if ( !end || *end != '\0' || !std::isfinite( value ) ) { return false; }
        out = value;
        return true;
    }

    inline std::vector<std::string> Split( std::string const& text, char separator )
    {
        std::vector<std::string> fields;
        size_t begin = 0;
        for ( ;; )
        {
            size_t const end = text.find( separator, begin );
            fields.push_back( text.substr( begin,
                end == std::string::npos ? std::string::npos : end - begin ) );
            if ( end == std::string::npos ) { break; }
            begin = end + 1;
        }
        return fields;
    }

    struct AuthorityEnvelope
    {
        std::string worldIdentity;
        uint64_t worldIdentityHash = 0;
        std::string worldgenId;
        uint32_t worldgenVersion = 0;
        uint32_t schemaVersion = 0;
        std::string regionKey;
        uint64_t eventProgramDigest = 0;
        uint64_t bodyCatalogDigest = 0;
        uint32_t authorityRevision = 0;
    };

    struct Event
    {
        std::string id;
        uint64_t stableId = 0;
        uint32_t chronology = 0;
        std::string kind;
    };

    struct Formation
    {
        std::string id;
        uint64_t featureId = 0;
        std::string material;
        double localMinM = 0.0;
        double localMaxM = 0.0;
        std::vector<std::string> eventIds;
        uint32_t youngestChronology = 0;
    };

    struct Descriptor
    {
        AuthorityEnvelope authority;
        double foldAmplitudeM = 0.0;
        double foldWavelengthM = 0.0;
        double foldAxisDeg = 0.0;
        double datumM = 0.0;
        std::vector<Event> events;
        std::vector<Formation> formations;
        size_t descriptorBytes = 0;
    };

    enum class BoundaryState : uint8_t
    {
        Interior,
        NearLowerContact,
        NearUpperContact
    };

    struct GeoSample
    {
        bool found = false;
        uint64_t regionId = 0;
        uint64_t featureId = 0;
        std::string formationId;
        std::string material;
        std::array<double, 3> structuralNormal = { 0.0, 0.0, 1.0 };
        std::array<double, 3> bodyLocalPosition = { 0.0, 0.0, 0.0 };
        std::vector<uint64_t> eventIds;
        std::vector<uint32_t> chronology;
        BoundaryState boundary = BoundaryState::Interior;
        uint32_t descriptorRevision = 0;
    };

    // Lightweight read used when a consumer needs only presentation material
    // and fault-age admission. The pointer refers to immutable descriptor data
    // or a static material literal owned by a later causal stage.
    struct MaterialSample
    {
        bool found = false;
        char const* material = nullptr;
        uint32_t youngestChronology = 0;
    };

    struct LoadResult
    {
        bool ok = false;
        std::string reason;
        Descriptor descriptor;
    };

    inline LoadResult LoadText( std::string const& source,
        std::string const& expectedRegion = kFloorRegionKey )
    {
        LoadResult result;
        result.descriptor.descriptorBytes = source.size();
        std::unordered_map<std::string, std::string> scalar;
        std::vector<std::string> eventPayloads;
        std::vector<std::string> formationPayloads;

        std::istringstream stream( source );
        std::string line;
        bool sawMagic = false;
        while ( std::getline( stream, line ) )
        {
            if ( !line.empty() && line.back() == '\r' ) { line.pop_back(); }
            if ( line.empty() || line[0] == '#' ) { continue; }
            if ( !sawMagic )
            {
                sawMagic = line == "PROVENANCE_CAUSAL_WORLD_DESCRIPTOR_V1";
                if ( !sawMagic ) { result.reason = "bad_magic"; return result; }
                continue;
            }
            size_t const eq = line.find( '=' );
            if ( eq == std::string::npos ) { result.reason = "malformed_line"; return result; }
            std::string const key = line.substr( 0, eq );
            std::string const value = line.substr( eq + 1 );
            if ( key == "event" ) { eventPayloads.push_back( value ); }
            else if ( key == "formation" ) { formationPayloads.push_back( value ); }
            else if ( scalar.find( key ) != scalar.end() )
            {
                result.reason = "duplicate_key:" + key;
                return result;
            }
            else { scalar.emplace( key, value ); }
        }
        if ( !sawMagic ) { result.reason = "missing_magic"; return result; }

        auto require = [&]( char const* key ) -> std::string const*
        {
            auto const it = scalar.find( key );
            return it == scalar.end() ? nullptr : &it->second;
        };
        auto missing = [&]( char const* key )
        {
            result.reason = std::string( "missing_key:" ) + key;
        };

        std::string const* value = nullptr;
        value = require( "world_identity" ); if ( !value ) { missing( "world_identity" ); return result; }
        result.descriptor.authority.worldIdentity = *value;
        value = require( "world_identity_hash" );
        if ( !value || !ParseHex64( *value, result.descriptor.authority.worldIdentityHash ) )
        { result.reason = "invalid_world_identity_hash"; return result; }
        value = require( "worldgen_id" ); if ( !value ) { missing( "worldgen_id" ); return result; }
        result.descriptor.authority.worldgenId = *value;
        value = require( "worldgen_version" );
        if ( !value || !ParseU32( *value, result.descriptor.authority.worldgenVersion ) )
        { result.reason = "invalid_worldgen_version"; return result; }
        value = require( "schema_version" );
        if ( !value || !ParseU32( *value, result.descriptor.authority.schemaVersion ) )
        { result.reason = "invalid_schema_version"; return result; }
        value = require( "region_key" ); if ( !value ) { missing( "region_key" ); return result; }
        result.descriptor.authority.regionKey = *value;
        value = require( "event_program_digest" );
        if ( !value || !ParseHex64( *value, result.descriptor.authority.eventProgramDigest ) )
        { result.reason = "invalid_event_program_digest"; return result; }
        value = require( "body_catalog_digest" );
        if ( !value || !ParseHex64( *value, result.descriptor.authority.bodyCatalogDigest ) )
        { result.reason = "invalid_body_catalog_digest"; return result; }
        value = require( "authority_revision" );
        if ( !value || !ParseU32( *value, result.descriptor.authority.authorityRevision ) )
        { result.reason = "invalid_authority_revision"; return result; }
        value = require( "fold_amplitude_m" );
        if ( !value || !ParseDouble( *value, result.descriptor.foldAmplitudeM ) )
        { result.reason = "invalid_fold_amplitude"; return result; }
        value = require( "fold_wavelength_m" );
        if ( !value || !ParseDouble( *value, result.descriptor.foldWavelengthM ) )
        { result.reason = "invalid_fold_wavelength"; return result; }
        value = require( "fold_axis_deg" );
        if ( !value || !ParseDouble( *value, result.descriptor.foldAxisDeg ) )
        { result.reason = "invalid_fold_axis"; return result; }
        value = require( "datum_m" );
        if ( !value || !ParseDouble( *value, result.descriptor.datumM ) )
        { result.reason = "invalid_datum"; return result; }

        if ( result.descriptor.authority.worldIdentityHash
            != HashText( result.descriptor.authority.worldIdentity ) )
        { result.reason = "world_identity_digest_mismatch"; return result; }
        if ( result.descriptor.authority.worldgenId != kExpectedWorldgenId )
        { result.reason = "wrong_worldgen_id"; return result; }
        if ( result.descriptor.authority.worldgenVersion != 1 )
        { result.reason = "unsupported_worldgen_version"; return result; }
        if ( result.descriptor.authority.schemaVersion != kSupportedSchemaVersion )
        { result.reason = "unsupported_schema"; return result; }
        if ( result.descriptor.authority.regionKey != expectedRegion )
        { result.reason = "wrong_region_key"; return result; }
        if ( result.descriptor.authority.authorityRevision == 0 )
        { result.reason = "stale_revision"; return result; }
        if ( result.descriptor.foldWavelengthM <= 0.0 )
        { result.reason = "nonpositive_fold_wavelength"; return result; }

        std::string eventCatalog;
        for ( std::string const& payload : eventPayloads ) { eventCatalog += payload + "\n"; }
        std::string formationCatalog;
        for ( std::string const& payload : formationPayloads ) { formationCatalog += payload + "\n"; }
        if ( HashText( eventCatalog ) != result.descriptor.authority.eventProgramDigest )
        { result.reason = "event_program_digest_mismatch"; return result; }
        if ( HashText( formationCatalog ) != result.descriptor.authority.bodyCatalogDigest )
        { result.reason = "body_catalog_digest_mismatch"; return result; }

        std::unordered_map<std::string, uint32_t> eventChronology;
        for ( std::string const& payload : eventPayloads )
        {
            std::vector<std::string> const fields = Split( payload, ',' );
            if ( fields.size() != 4 ) { result.reason = "malformed_event"; return result; }
            Event event;
            event.id = fields[0];
            if ( !ParseHex64( fields[1], event.stableId ) || !ParseU32( fields[2], event.chronology ) )
            { result.reason = "invalid_event"; return result; }
            event.kind = fields[3];
            if ( event.id.empty() || event.kind.empty() || eventChronology.count( event.id ) )
            { result.reason = "duplicate_or_empty_event"; return result; }
            eventChronology[event.id] = event.chronology;
            result.descriptor.events.push_back( std::move( event ) );
        }

        std::unordered_map<uint64_t, bool> featureIds;
        for ( std::string const& payload : formationPayloads )
        {
            std::vector<std::string> const fields = Split( payload, ',' );
            if ( fields.size() != 6 ) { result.reason = "malformed_formation"; return result; }
            Formation formation;
            formation.id = fields[0];
            if ( !ParseHex64( fields[1], formation.featureId )
              || !ParseDouble( fields[3], formation.localMinM )
              || !ParseDouble( fields[4], formation.localMaxM ) )
            { result.reason = "invalid_formation"; return result; }
            formation.material = fields[2];
            formation.eventIds = Split( fields[5], '|' );
            if ( formation.id.empty() || formation.material.empty()
              || formation.localMinM >= formation.localMaxM
              || featureIds.count( formation.featureId ) )
            { result.reason = "invalid_formation_bounds_or_id"; return result; }
            featureIds[formation.featureId] = true;
            uint32_t priorChronology = 0;
            for ( std::string const& eventId : formation.eventIds )
            {
                auto const eventIt = eventChronology.find( eventId );
                if ( eventIt == eventChronology.end() || eventIt->second < priorChronology )
                { result.reason = "invalid_event_ancestry"; return result; }
                priorChronology = eventIt->second;
            }
            formation.youngestChronology = priorChronology;
            result.descriptor.formations.push_back( std::move( formation ) );
        }
        if ( result.descriptor.events.empty() || result.descriptor.formations.size() < 2 )
        { result.reason = "catalog_too_small"; return result; }

        std::sort( result.descriptor.formations.begin(), result.descriptor.formations.end(),
            []( Formation const& a, Formation const& b ) { return a.localMinM < b.localMinM; } );
        for ( size_t i = 1; i < result.descriptor.formations.size(); ++i )
        {
            if ( result.descriptor.formations[i].localMinM
                < result.descriptor.formations[i - 1].localMaxM )
            { result.reason = "overlapping_formations"; return result; }
        }

        result.ok = true;
        result.reason = "ok";
        return result;
    }

    inline LoadResult LoadFile( char const* path,
        std::string const& expectedRegion = kFloorRegionKey )
    {
        std::ifstream input( path, std::ios::binary );
        if ( !input ) { LoadResult out; out.reason = "descriptor_missing"; return out; }
        std::ostringstream bytes;
        bytes << input.rdbuf();
        return LoadText( bytes.str(), expectedRegion );
    }

    class Kernel
    {
    public:
        struct ColumnContext
        {
            double axis = 0.0;
            double along = 0.0;
            double across = 0.0;
            double wave = 0.0;
            double foldedDatum = 0.0;
        };

        explicit Kernel( Descriptor descriptor ) : m_descriptor( std::move( descriptor ) )
        {
            for ( size_t i = 0; i < m_descriptor.events.size(); ++i )
            { m_events[m_descriptor.events[i].id] = i; }
        }

        ColumnContext PrepareColumn( double x, double y ) const
        {
            ColumnContext column;
            double constexpr kPi = 3.14159265358979323846;
            column.axis = m_descriptor.foldAxisDeg * kPi / 180.0;
            column.along = x * std::cos( column.axis ) + y * std::sin( column.axis );
            column.across = -x * std::sin( column.axis ) + y * std::cos( column.axis );
            column.wave = 2.0 * kPi * column.along / m_descriptor.foldWavelengthM;
            column.foldedDatum = m_descriptor.datumM
                + m_descriptor.foldAmplitudeM * std::sin( column.wave );
            return column;
        }

        Formation const* FormationAt( ColumnContext const& column, double z ) const
        {
            double const localZ = z - column.foldedDatum;
            for ( Formation const& formation : m_descriptor.formations )
            {
                if ( localZ >= formation.localMinM && localZ < formation.localMaxM )
                { return &formation; }
            }
            return nullptr;
        }

        MaterialSample QueryMaterial( double x, double y, double z ) const
        {
            ColumnContext const column = PrepareColumn( x, y );
            Formation const* const formation = FormationAt( column, z );
            if ( !formation ) { return {}; }
            return { true, formation->material.c_str(), formation->youngestChronology };
        }

        GeoSample Query( double x, double y, double z ) const
        {
            GeoSample sample;
            double constexpr kPi = 3.14159265358979323846;
            ColumnContext const column = PrepareColumn( x, y );
            double const localZ = z - column.foldedDatum;
            Formation const* found = FormationAt( column, z );
            if ( !found ) { return sample; }

            double const slope = m_descriptor.foldAmplitudeM
                * ( 2.0 * kPi / m_descriptor.foldWavelengthM ) * std::cos( column.wave );
            double nx = -slope * std::cos( column.axis );
            double ny = -slope * std::sin( column.axis );
            double nz = 1.0;
            double const invLength = 1.0 / std::sqrt( nx * nx + ny * ny + nz * nz );
            nx *= invLength; ny *= invLength; nz *= invLength;

            sample.found = true;
            sample.regionId = HashText( m_descriptor.authority.regionKey );
            sample.featureId = found->featureId;
            sample.formationId = found->id;
            sample.material = found->material;
            sample.structuralNormal = { nx, ny, nz };
            sample.bodyLocalPosition = { column.along, column.across, localZ };
            sample.descriptorRevision = m_descriptor.authority.authorityRevision;
            double constexpr kBoundaryEpsilonM = 0.05;
            if ( localZ - found->localMinM <= kBoundaryEpsilonM )
            { sample.boundary = BoundaryState::NearLowerContact; }
            else if ( found->localMaxM - localZ <= kBoundaryEpsilonM )
            { sample.boundary = BoundaryState::NearUpperContact; }
            for ( std::string const& eventId : found->eventIds )
            {
                Event const& event = m_descriptor.events[m_events.at( eventId )];
                sample.eventIds.push_back( event.stableId );
                sample.chronology.push_back( event.chronology );
            }
            return sample;
        }

        Descriptor const& GetDescriptor() const { return m_descriptor; }

    private:
        Descriptor m_descriptor;
        std::unordered_map<std::string, size_t> m_events;
    };

    inline void HashAppend( uint64_t& hash, void const* data, size_t size )
    {
        uint8_t const* bytes = (uint8_t const*)data;
        for ( size_t i = 0; i < size; ++i )
        {
            hash ^= bytes[i];
            hash *= 1099511628211ull;
        }
    }

    inline uint64_t SemanticDigest( std::vector<GeoSample> const& samples )
    {
        uint64_t hash = 14695981039346656037ull;
        for ( GeoSample const& sample : samples )
        {
            HashAppend( hash, &sample.found, sizeof( sample.found ) );
            HashAppend( hash, &sample.regionId, sizeof( sample.regionId ) );
            HashAppend( hash, &sample.featureId, sizeof( sample.featureId ) );
            HashAppend( hash, sample.formationId.data(), sample.formationId.size() );
            HashAppend( hash, sample.material.data(), sample.material.size() );
            for ( double value : sample.structuralNormal )
            {
                int64_t const quantized = (int64_t)std::llround( value * 1000000000.0 );
                HashAppend( hash, &quantized, sizeof( quantized ) );
            }
            for ( uint64_t eventId : sample.eventIds ) { HashAppend( hash, &eventId, sizeof( eventId ) ); }
            for ( uint32_t chronology : sample.chronology ) { HashAppend( hash, &chronology, sizeof( chronology ) ); }
            HashAppend( hash, &sample.descriptorRevision, sizeof( sample.descriptorRevision ) );
        }
        return hash;
    }

    struct CertResult
    {
        bool passed = false;
        std::string loadReason;
        uint64_t semanticDigest = 0;
        size_t descriptorBytes = 0;
        size_t descriptorMemoryBytes = 0;
        size_t indexMemoryBytes = 0;
        size_t featureCount = 0;
        size_t eventCount = 0;
        size_t queryCount = 0;
        double queriesPerSecond = 0.0;
        double meanQueryUs = 0.0;
        double p99QueryUs = 0.0;
        double coldQueryUs = 0.0;
        double warmQueryUs = 0.0;
        std::vector<std::pair<std::string, bool>> checks;
    };

    inline bool ReplaceFirst( std::string& text, std::string const& from, std::string const& to )
    {
        size_t const at = text.find( from );
        if ( at == std::string::npos ) { return false; }
        text.replace( at, from.size(), to );
        return true;
    }

    inline CertResult RunCert( char const* descriptorPath )
    {
        CertResult cert;
        std::ifstream input( descriptorPath, std::ios::binary );
        if ( !input )
        {
            cert.loadReason = "descriptor_missing";
            cert.checks.push_back( { "authority_descriptor_load", false } );
            return cert;
        }
        std::ostringstream bytes;
        bytes << input.rdbuf();
        std::string const source = bytes.str();
        LoadResult loaded = LoadText( source );
        cert.loadReason = loaded.reason;
        cert.checks.push_back( { "authority_descriptor_load", loaded.ok } );
        if ( !loaded.ok ) { return cert; }

        Kernel kernel( loaded.descriptor );
        Descriptor const& descriptor = kernel.GetDescriptor();
        cert.descriptorBytes = descriptor.descriptorBytes;
        cert.featureCount = descriptor.formations.size();
        cert.eventCount = descriptor.events.size();
        cert.descriptorMemoryBytes = sizeof( Descriptor )
            + descriptor.events.size() * sizeof( Event )
            + descriptor.formations.size() * sizeof( Formation );
        cert.indexMemoryBytes = descriptor.events.size()
            * ( sizeof( std::string ) + sizeof( Event const* ) );

        std::vector<std::array<double, 3>> points;
        double constexpr kPi = 3.14159265358979323846;
        double const axis = descriptor.foldAxisDeg * kPi / 180.0;
        for ( int i = -48; i <= 48; ++i )
        {
            double const along = i * 2.0;
            double const x = along * std::cos( axis );
            double const y = along * std::sin( axis );
            double const foldedDatum = descriptor.datumM + descriptor.foldAmplitudeM
                * std::sin( 2.0 * kPi * along / descriptor.foldWavelengthM );
            double const localZ = descriptor.formations[( size_t )( ( i + 48 )
                % (int)descriptor.formations.size() )].localMinM + 1.0;
            points.push_back( { x, y, foldedDatum + localZ } );
        }

        auto evaluate = [&]( std::vector<size_t> const& order, Kernel const& queryKernel )
        {
            std::vector<GeoSample> samples( points.size() );
            for ( size_t index : order )
            {
                auto const& p = points[index];
                samples[index] = queryKernel.Query( p[0], p[1], p[2] );
            }
            return samples;
        };
        std::vector<size_t> forward( points.size() );
        for ( size_t i = 0; i < forward.size(); ++i ) { forward[i] = i; }
        std::vector<size_t> reverse = forward;
        std::reverse( reverse.begin(), reverse.end() );
        std::vector<size_t> checker;
        for ( size_t i = 0; i < forward.size(); i += 2 ) { checker.push_back( i ); }
        for ( size_t i = 1; i < forward.size(); i += 2 ) { checker.push_back( i ); }
        std::vector<size_t> shuffled = forward;
        uint32_t randomState = 0x6d2b79f5u;
        for ( size_t i = shuffled.size(); i > 1; --i )
        {
            randomState = randomState * 1664525u + 1013904223u;
            std::swap( shuffled[i - 1], shuffled[randomState % i] );
        }
        // Discover the same points in artificial presentation tiles. Geology
        // semantics must not depend on the tiling chosen by a future renderer.
        std::vector<size_t> tiled = forward;
        std::stable_sort( tiled.begin(), tiled.end(), [&]( size_t a, size_t b )
        {
            int const tileA = (int)std::floor( points[a][0] / 17.0 );
            int const tileB = (int)std::floor( points[b][0] / 17.0 );
            if ( tileA != tileB ) { return tileA < tileB; }
            return ( a & 1u ) < ( b & 1u );
        } );

        std::vector<GeoSample> const baseline = evaluate( forward, kernel );
        cert.semanticDigest = SemanticDigest( baseline );
        bool allFound = std::all_of( baseline.begin(), baseline.end(),
            []( GeoSample const& sample ) { return sample.found; } );
        cert.checks.push_back( { "all_points_resolve", allFound } );
        cert.checks.push_back( { "reverse_order_semantics",
            SemanticDigest( evaluate( reverse, kernel ) ) == cert.semanticDigest } );
        cert.checks.push_back( { "shuffled_order_semantics",
            SemanticDigest( evaluate( shuffled, kernel ) ) == cert.semanticDigest } );
        cert.checks.push_back( { "checkerboard_order_semantics",
            SemanticDigest( evaluate( checker, kernel ) ) == cert.semanticDigest } );
        cert.checks.push_back( { "tiled_order_semantics",
            SemanticDigest( evaluate( tiled, kernel ) ) == cert.semanticDigest } );

        LoadResult reloaded = LoadText( source );
        bool reloadStable = false;
        if ( reloaded.ok )
        {
            Kernel reloadedKernel( std::move( reloaded.descriptor ) );
            reloadStable = SemanticDigest( evaluate( reverse, reloadedKernel ) ) == cert.semanticDigest;
        }
        cert.checks.push_back( { "evict_reload_semantics", reloadStable } );

        bool sameFeatureDifferentFrame = false;
        for ( size_t i = 0; i < baseline.size(); ++i )
        for ( size_t j = i + 1; j < baseline.size(); ++j )
        {
            double const dot = baseline[i].structuralNormal[0] * baseline[j].structuralNormal[0]
                + baseline[i].structuralNormal[1] * baseline[j].structuralNormal[1]
                + baseline[i].structuralNormal[2] * baseline[j].structuralNormal[2];
            if ( baseline[i].featureId == baseline[j].featureId && dot < 0.995 )
            { sameFeatureDifferentFrame = true; break; }
        }
        cert.checks.push_back( { "same_feature_different_local_frame", sameFeatureDifferentFrame } );

        bool ancestryChronological = true;
        for ( GeoSample const& sample : baseline )
        for ( size_t i = 1; i < sample.chronology.size(); ++i )
        { ancestryChronological = ancestryChronological && sample.chronology[i - 1] <= sample.chronology[i]; }
        cert.checks.push_back( { "ancestry_chronology", ancestryChronological } );

        cert.checks.push_back( { "refuse_wrong_region",
            !LoadText( source, "wrong_region" ).ok } );
        cert.checks.push_back( { "refuse_missing_descriptor",
            !LoadFile( "Data\\Worldgen\\__missing_causal_world_descriptor__.cwg" ).ok } );
        std::string badSchema = source;
        ReplaceFirst( badSchema, "schema_version=1", "schema_version=2" );
        cert.checks.push_back( { "refuse_unsupported_schema", !LoadText( badSchema ).ok } );
        std::string badWorldgenVersion = source;
        ReplaceFirst( badWorldgenVersion, "worldgen_version=1", "worldgen_version=2" );
        cert.checks.push_back( { "refuse_unsupported_worldgen_version",
            !LoadText( badWorldgenVersion ).ok } );
        std::string badDigest = source;
        ReplaceFirst( badDigest, ",sandstone,", ",sandstond," );
        cert.checks.push_back( { "refuse_catalog_digest_mismatch", !LoadText( badDigest ).ok } );
        std::string stale = source;
        ReplaceFirst( stale, "authority_revision=1", "authority_revision=0" );
        cert.checks.push_back( { "refuse_stale_revision", !LoadText( stale ).ok } );

        using Clock = std::chrono::steady_clock;
        std::vector<double> queryTimes;
        queryTimes.reserve( 20000 );
        volatile uint64_t queryGuard = 0;
        auto const performanceBegin = Clock::now();
        for ( size_t i = 0; i < 20000; ++i )
        {
            auto const& p = points[i % points.size()];
            auto const begin = Clock::now();
            GeoSample const sample = kernel.Query( p[0], p[1], p[2] );
            auto const end = Clock::now();
            queryGuard = queryGuard ^ sample.featureId;
            queryTimes.push_back( std::chrono::duration<double, std::micro>( end - begin ).count() );
        }
        auto const performanceEnd = Clock::now();
        (void)queryGuard;
        cert.queryCount = queryTimes.size();
        double const elapsedSeconds = std::chrono::duration<double>(
            performanceEnd - performanceBegin ).count();
        cert.queriesPerSecond = elapsedSeconds > 0.0 ? cert.queryCount / elapsedSeconds : 0.0;
        for ( double time : queryTimes ) { cert.meanQueryUs += time; }
        cert.meanQueryUs /= (double)queryTimes.size();
        cert.coldQueryUs = queryTimes.front();
        cert.warmQueryUs = queryTimes.back();
        std::sort( queryTimes.begin(), queryTimes.end() );
        cert.p99QueryUs = queryTimes[(size_t)( 0.99 * ( queryTimes.size() - 1 ) )];

        cert.passed = std::all_of( cert.checks.begin(), cert.checks.end(),
            []( auto const& check ) { return check.second; } );
        return cert;
    }

    inline bool WriteCertArtifact( CertResult const& cert, char const* path,
        char const* descriptorPath )
    {
        FILE* file = nullptr;
        if ( fopen_s( &file, path, "wb" ) != 0 || !file ) { return false; }
        std::fprintf( file, "CAUSAL_WORLD_GEOLOGY_KERNEL_FLOOR %s\n",
            cert.passed ? "PASS" : "FAIL" );
        std::fprintf( file, "descriptor=%s\nload_reason=%s\n", descriptorPath, cert.loadReason.c_str() );
        std::fprintf( file, "semantic_digest=%s\n", Hex64( cert.semanticDigest ).c_str() );
        std::fprintf( file, "descriptor_bytes=%zu\ndescriptor_memory_bytes=%zu\nindex_memory_bytes=%zu\n",
            cert.descriptorBytes, cert.descriptorMemoryBytes, cert.indexMemoryBytes );
        std::fprintf( file, "features=%zu\nevents=%zu\nfeatures_materialized=0\n",
            cert.featureCount, cert.eventCount );
        std::fprintf( file, "queries=%zu\nqueries_per_second=%.3f\nmean_query_us=%.6f\np99_query_us=%.6f\n",
            cert.queryCount, cert.queriesPerSecond, cert.meanQueryUs, cert.p99QueryUs );
        std::fprintf( file, "cold_query_us=%.6f\nwarm_query_us=%.6f\n",
            cert.coldQueryUs, cert.warmQueryUs );
        std::fprintf( file, "terrain_output_changes=0\noccupancy_mutations=0\nd2_rebuilds=0\n"
            "matter_bodies=0\nwater_coupling=0\np5b=closed\n" );
        for ( auto const& check : cert.checks )
        { std::fprintf( file, "check.%s=%s\n", check.first.c_str(), check.second ? "PASS" : "FAIL" ); }
        std::fclose( file );
        return true;
    }
}
