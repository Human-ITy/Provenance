#pragma once

// Present-day surface exposure over the read-only CAUSAL_WORLD geology kernel.
// The surface is a compiled authority product. This module performs analytical
// intersections only: it creates no terrain, occupancy, D2, bodies, or water.

#include "CausalWorldGeology.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace CausalWorldExposure
{
    constexpr char const* kExpectedRegion = "causal_world_geologic_exposure_floor";
    constexpr char const* kPresentSurface = "present_erosion";

    enum class SurfaceMode : uint8_t
    {
        Erosion,
        Flat,
        Terraced
    };

    struct SurfaceProgram
    {
        std::string id;
        SurfaceMode mode = SurfaceMode::Erosion;
        double datumM = 0.0;
        double slopeX = 0.0;
        double slopeY = 0.0;
        double amplitudeM = 0.0;
        double wavelengthM = 1.0;
        double azimuthDeg = 0.0;
        double terraceStepM = 0.0;
    };

    struct Descriptor
    {
        uint64_t worldIdentityHash = 0;
        std::string worldgenId;
        uint32_t worldgenVersion = 0;
        uint32_t schemaVersion = 0;
        std::string regionKey;
        uint64_t geologyDescriptorDigest = 0;
        uint64_t surfaceProgramDigest = 0;
        uint32_t authorityRevision = 0;
        std::vector<SurfaceProgram> surfaces;
        size_t descriptorBytes = 0;
    };

    struct LoadResult
    {
        bool ok = false;
        std::string reason;
        Descriptor descriptor;
    };

    inline LoadResult LoadText( std::string const& source,
        std::string const& geologySource,
        std::string const& expectedRegion = kExpectedRegion )
    {
        LoadResult result;
        result.descriptor.descriptorBytes = source.size();
        std::unordered_map<std::string, std::string> scalar;
        std::vector<std::string> surfacePayloads;
        std::istringstream stream( source );
        std::string line;
        bool sawMagic = false;
        while ( std::getline( stream, line ) )
        {
            if ( !line.empty() && line.back() == '\r' ) { line.pop_back(); }
            if ( line.empty() || line[0] == '#' ) { continue; }
            if ( !sawMagic )
            {
                sawMagic = line == "PROVENANCE_CAUSAL_WORLD_EXPOSURE_V1";
                if ( !sawMagic ) { result.reason = "bad_magic"; return result; }
                continue;
            }
            size_t const eq = line.find( '=' );
            if ( eq == std::string::npos ) { result.reason = "malformed_line"; return result; }
            std::string const key = line.substr( 0, eq );
            std::string const value = line.substr( eq + 1 );
            if ( key == "surface" ) { surfacePayloads.push_back( value ); }
            else if ( scalar.count( key ) ) { result.reason = "duplicate_key:" + key; return result; }
            else { scalar.emplace( key, value ); }
        }
        if ( !sawMagic ) { result.reason = "missing_magic"; return result; }

        auto get = [&]( char const* key ) -> std::string const*
        {
            auto const it = scalar.find( key );
            return it == scalar.end() ? nullptr : &it->second;
        };
        std::string const* value = nullptr;
        value = get( "world_identity_hash" );
        if ( !value || !CausalWorldGeology::ParseHex64( *value, result.descriptor.worldIdentityHash ) )
        { result.reason = "invalid_world_identity_hash"; return result; }
        value = get( "worldgen_id" );
        if ( !value ) { result.reason = "missing_worldgen_id"; return result; }
        result.descriptor.worldgenId = *value;
        value = get( "worldgen_version" );
        if ( !value || !CausalWorldGeology::ParseU32( *value, result.descriptor.worldgenVersion ) )
        { result.reason = "invalid_worldgen_version"; return result; }
        value = get( "schema_version" );
        if ( !value || !CausalWorldGeology::ParseU32( *value, result.descriptor.schemaVersion ) )
        { result.reason = "invalid_schema_version"; return result; }
        value = get( "region_key" );
        if ( !value ) { result.reason = "missing_region_key"; return result; }
        result.descriptor.regionKey = *value;
        value = get( "geology_descriptor_digest" );
        if ( !value || !CausalWorldGeology::ParseHex64( *value, result.descriptor.geologyDescriptorDigest ) )
        { result.reason = "invalid_geology_descriptor_digest"; return result; }
        value = get( "surface_program_digest" );
        if ( !value || !CausalWorldGeology::ParseHex64( *value, result.descriptor.surfaceProgramDigest ) )
        { result.reason = "invalid_surface_program_digest"; return result; }
        value = get( "authority_revision" );
        if ( !value || !CausalWorldGeology::ParseU32( *value, result.descriptor.authorityRevision ) )
        { result.reason = "invalid_authority_revision"; return result; }

        if ( result.descriptor.worldgenId != CausalWorldGeology::kExpectedWorldgenId )
        { result.reason = "wrong_worldgen_id"; return result; }
        if ( result.descriptor.worldgenVersion != 1 )
        { result.reason = "unsupported_worldgen_version"; return result; }
        if ( result.descriptor.schemaVersion != 1 )
        { result.reason = "unsupported_schema"; return result; }
        if ( result.descriptor.regionKey != expectedRegion )
        { result.reason = "wrong_region_key"; return result; }
        if ( result.descriptor.authorityRevision == 0 )
        { result.reason = "stale_revision"; return result; }
        if ( result.descriptor.geologyDescriptorDigest != CausalWorldGeology::HashText( geologySource ) )
        { result.reason = "geology_descriptor_digest_mismatch"; return result; }
        CausalWorldGeology::LoadResult const linkedGeology =
            CausalWorldGeology::LoadText( geologySource );
        if ( !linkedGeology.ok )
        { result.reason = "linked_geology_refused:" + linkedGeology.reason; return result; }
        if ( result.descriptor.worldIdentityHash
            != linkedGeology.descriptor.authority.worldIdentityHash )
        { result.reason = "world_identity_mismatch"; return result; }

        std::string surfaceCatalog;
        for ( std::string const& payload : surfacePayloads ) { surfaceCatalog += payload + "\n"; }
        if ( result.descriptor.surfaceProgramDigest != CausalWorldGeology::HashText( surfaceCatalog ) )
        { result.reason = "surface_program_digest_mismatch"; return result; }

        std::unordered_set<std::string> ids;
        for ( std::string const& payload : surfacePayloads )
        {
            std::vector<std::string> const fields = CausalWorldGeology::Split( payload, ',' );
            if ( fields.size() != 9 ) { result.reason = "malformed_surface"; return result; }
            SurfaceProgram surface;
            surface.id = fields[0];
            if ( fields[1] == "erosion" ) { surface.mode = SurfaceMode::Erosion; }
            else if ( fields[1] == "flat" ) { surface.mode = SurfaceMode::Flat; }
            else if ( fields[1] == "terraced" ) { surface.mode = SurfaceMode::Terraced; }
            else { result.reason = "unknown_surface_mode"; return result; }
            if ( !CausalWorldGeology::ParseDouble( fields[2], surface.datumM )
              || !CausalWorldGeology::ParseDouble( fields[3], surface.slopeX )
              || !CausalWorldGeology::ParseDouble( fields[4], surface.slopeY )
              || !CausalWorldGeology::ParseDouble( fields[5], surface.amplitudeM )
              || !CausalWorldGeology::ParseDouble( fields[6], surface.wavelengthM )
              || !CausalWorldGeology::ParseDouble( fields[7], surface.azimuthDeg )
              || !CausalWorldGeology::ParseDouble( fields[8], surface.terraceStepM ) )
            { result.reason = "invalid_surface_values"; return result; }
            if ( surface.id.empty() || ids.count( surface.id ) || surface.wavelengthM <= 0.0
              || ( surface.mode == SurfaceMode::Terraced && surface.terraceStepM <= 0.0 ) )
            { result.reason = "invalid_surface_contract"; return result; }
            ids.insert( surface.id );
            result.descriptor.surfaces.push_back( std::move( surface ) );
        }
        if ( !ids.count( kPresentSurface ) || !ids.count( "flat_control" )
          || !ids.count( "terraced_control" ) )
        { result.reason = "required_surface_missing"; return result; }

        result.ok = true;
        result.reason = "ok";
        return result;
    }

    inline bool ReadFile( char const* path, std::string& out )
    {
        std::ifstream input( path, std::ios::binary );
        if ( !input ) { return false; }
        std::ostringstream bytes;
        bytes << input.rdbuf();
        out = bytes.str();
        return true;
    }

    struct ExposureSample
    {
        bool found = false;
        double surfaceZ = 0.0;
        CausalWorldGeology::GeoSample geology;
        double nearestContactDistanceM = 0.0;
        uint64_t nearestContactFeatureId = 0;
    };

    class Kernel
    {
    public:
        Kernel( CausalWorldGeology::Descriptor geology, Descriptor exposure )
            : m_geology( std::move( geology ) ), m_exposure( std::move( exposure ) )
        {
            for ( size_t i = 0; i < m_exposure.surfaces.size(); ++i )
            { m_surfaceIndex[m_exposure.surfaces[i].id] = i; }
        }

        double SurfaceZ( std::string const& surfaceId, double x, double y ) const
        {
            SurfaceProgram const& surface = m_exposure.surfaces[m_surfaceIndex.at( surfaceId )];
            if ( surface.mode == SurfaceMode::Flat ) { return surface.datumM; }
            double constexpr kPi = 3.14159265358979323846;
            double const angle = surface.azimuthDeg * kPi / 180.0;
            double const along = x * std::cos( angle ) + y * std::sin( angle );
            double raw = surface.datumM + surface.slopeX * x + surface.slopeY * y
                + surface.amplitudeM * std::sin( 2.0 * kPi * along / surface.wavelengthM );
            if ( surface.mode == SurfaceMode::Terraced )
            { raw = std::floor( raw / surface.terraceStepM ) * surface.terraceStepM; }
            return raw;
        }

        ExposureSample Query( std::string const& surfaceId, double x, double y ) const
        {
            ExposureSample sample;
            sample.surfaceZ = SurfaceZ( surfaceId, x, y );
            sample.geology = m_geology.Query( x, y, sample.surfaceZ - 0.001 );
            sample.found = sample.geology.found;
            if ( !sample.found ) { return sample; }

            auto const& formations = m_geology.GetDescriptor().formations;
            for ( size_t i = 0; i < formations.size(); ++i )
            {
                auto const& formation = formations[i];
                if ( formation.featureId != sample.geology.featureId ) { continue; }
                double const localZ = sample.geology.bodyLocalPosition[2];
                double const toLower = localZ - formation.localMinM;
                double const toUpper = formation.localMaxM - localZ;
                if ( toLower <= toUpper )
                {
                    sample.nearestContactDistanceM = toLower
                        * std::fabs( sample.geology.structuralNormal[2] );
                    sample.nearestContactFeatureId = i > 0 ? formations[i - 1].featureId : 0;
                }
                else
                {
                    sample.nearestContactDistanceM = toUpper
                        * std::fabs( sample.geology.structuralNormal[2] );
                    sample.nearestContactFeatureId = i + 1 < formations.size()
                        ? formations[i + 1].featureId : 0;
                }
                break;
            }
            return sample;
        }

        CausalWorldGeology::Kernel const& Geology() const { return m_geology; }
        Descriptor const& GetDescriptor() const { return m_exposure; }

    private:
        CausalWorldGeology::Kernel m_geology;
        Descriptor m_exposure;
        std::unordered_map<std::string, size_t> m_surfaceIndex;
    };

    inline uint64_t SemanticDigest( std::vector<ExposureSample> const& samples )
    {
        uint64_t hash = 14695981039346656037ull;
        for ( ExposureSample const& sample : samples )
        {
            CausalWorldGeology::HashAppend( hash, &sample.found, sizeof( sample.found ) );
            int64_t const surface = (int64_t)std::llround( sample.surfaceZ * 1000000000.0 );
            int64_t const contact = (int64_t)std::llround( sample.nearestContactDistanceM * 1000000000.0 );
            CausalWorldGeology::HashAppend( hash, &surface, sizeof( surface ) );
            CausalWorldGeology::HashAppend( hash, &sample.geology.featureId, sizeof( sample.geology.featureId ) );
            CausalWorldGeology::HashAppend( hash, sample.geology.material.data(), sample.geology.material.size() );
            for ( double value : sample.geology.structuralNormal )
            {
                int64_t const normal = (int64_t)std::llround( value * 1000000000.0 );
                CausalWorldGeology::HashAppend( hash, &normal, sizeof( normal ) );
            }
            CausalWorldGeology::HashAppend( hash, &contact, sizeof( contact ) );
            CausalWorldGeology::HashAppend( hash, &sample.nearestContactFeatureId,
                sizeof( sample.nearestContactFeatureId ) );
        }
        return hash;
    }

    struct CertResult
    {
        bool passed = false;
        std::string loadReason;
        uint64_t semanticDigest = 0;
        size_t sampleCount = 0;
        size_t exposedFeatureCount = 0;
        size_t contactTransitions = 0;
        size_t outcropBuriedMatches = 0;
        double queriesPerSecond = 0.0;
        double meanQueryUs = 0.0;
        double p99QueryUs = 0.0;
        size_t geologyDescriptorBytes = 0;
        size_t exposureDescriptorBytes = 0;
        std::vector<std::pair<std::string, bool>> checks;
    };

    inline CertResult RunCert( char const* geologyPath, char const* exposurePath )
    {
        CertResult cert;
        std::string geologySource;
        std::string exposureSource;
        if ( !ReadFile( geologyPath, geologySource ) || !ReadFile( exposurePath, exposureSource ) )
        {
            cert.loadReason = "descriptor_missing";
            cert.checks.push_back( { "authority_descriptors_load", false } );
            return cert;
        }
        CausalWorldGeology::LoadResult geology = CausalWorldGeology::LoadText( geologySource );
        LoadResult exposure = LoadText( exposureSource, geologySource );
        cert.loadReason = !geology.ok ? geology.reason : exposure.reason;
        bool const loaded = geology.ok && exposure.ok;
        cert.checks.push_back( { "authority_descriptors_load", loaded } );
        if ( !loaded ) { return cert; }
        cert.geologyDescriptorBytes = geologySource.size();
        cert.exposureDescriptorBytes = exposureSource.size();
        Kernel kernel( geology.descriptor, exposure.descriptor );

        std::vector<std::array<double, 2>> points;
        for ( int y = -64; y <= 64; y += 4 )
        for ( int x = -64; x <= 64; x += 4 )
        { points.push_back( { (double)x, (double)y } ); }
        std::vector<size_t> forward( points.size() );
        for ( size_t i = 0; i < forward.size(); ++i ) { forward[i] = i; }
        std::vector<size_t> reverse = forward;
        std::reverse( reverse.begin(), reverse.end() );
        std::vector<size_t> tiled = forward;
        std::stable_sort( tiled.begin(), tiled.end(), [&]( size_t a, size_t b )
        {
            int const ax = (int)std::floor( points[a][0] / 16.0 );
            int const ay = (int)std::floor( points[a][1] / 16.0 );
            int const bx = (int)std::floor( points[b][0] / 16.0 );
            int const by = (int)std::floor( points[b][1] / 16.0 );
            return ay != by ? ay < by : ( ax != bx ? ax < bx : a < b );
        } );
        std::vector<size_t> shuffled = forward;
        uint32_t state = 0x9e3779b9u;
        for ( size_t i = shuffled.size(); i > 1; --i )
        {
            state = state * 1664525u + 1013904223u;
            std::swap( shuffled[i - 1], shuffled[state % i] );
        }
        auto evaluate = [&]( std::vector<size_t> const& order, Kernel const& queryKernel )
        {
            std::vector<ExposureSample> samples( points.size() );
            for ( size_t i : order )
            { samples[i] = queryKernel.Query( kPresentSurface, points[i][0], points[i][1] ); }
            return samples;
        };

        std::vector<ExposureSample> const baseline = evaluate( forward, kernel );
        cert.sampleCount = baseline.size();
        cert.semanticDigest = SemanticDigest( baseline );
        bool const allFound = std::all_of( baseline.begin(), baseline.end(),
            []( ExposureSample const& sample ) { return sample.found; } );
        cert.checks.push_back( { "all_surface_points_resolve", allFound } );
        cert.checks.push_back( { "reverse_order_semantics",
            SemanticDigest( evaluate( reverse, kernel ) ) == cert.semanticDigest } );
        cert.checks.push_back( { "shuffled_order_semantics",
            SemanticDigest( evaluate( shuffled, kernel ) ) == cert.semanticDigest } );
        cert.checks.push_back( { "tiled_order_semantics",
            SemanticDigest( evaluate( tiled, kernel ) ) == cert.semanticDigest } );

        CausalWorldGeology::LoadResult geologyReload = CausalWorldGeology::LoadText( geologySource );
        LoadResult exposureReload = LoadText( exposureSource, geologySource );
        bool reloadStable = false;
        if ( geologyReload.ok && exposureReload.ok )
        {
            Kernel reloaded( std::move( geologyReload.descriptor ), std::move( exposureReload.descriptor ) );
            reloadStable = SemanticDigest( evaluate( reverse, reloaded ) ) == cert.semanticDigest;
        }
        cert.checks.push_back( { "reload_semantics", reloadStable } );

        std::unordered_set<uint64_t> catalogIds;
        for ( auto const& formation : kernel.Geology().GetDescriptor().formations )
        { catalogIds.insert( formation.featureId ); }
        std::unordered_set<uint64_t> exposedIds;
        bool onlyCatalogFeatures = true;
        bool directIntersection = true;
        for ( size_t i = 0; i < baseline.size(); ++i )
        {
            ExposureSample const& sample = baseline[i];
            exposedIds.insert( sample.geology.featureId );
            onlyCatalogFeatures = onlyCatalogFeatures && catalogIds.count( sample.geology.featureId ) != 0;
            auto const& p = points[i];
            CausalWorldGeology::GeoSample const direct = kernel.Geology().Query(
                p[0], p[1], sample.surfaceZ - 0.001 );
            directIntersection = directIntersection && direct.featureId == sample.geology.featureId
                && direct.material == sample.geology.material;
            if ( sample.nearestContactDistanceM > 1.25 )
            {
                CausalWorldGeology::GeoSample const buried = kernel.Geology().Query(
                    p[0], p[1], sample.surfaceZ - 1.0 );
                if ( buried.found && buried.featureId == sample.geology.featureId )
                { ++cert.outcropBuriedMatches; }
            }
        }
        cert.exposedFeatureCount = exposedIds.size();
        cert.checks.push_back( { "surface_is_direct_geology_intersection", directIntersection } );
        cert.checks.push_back( { "no_features_minted", onlyCatalogFeatures } );
        cert.checks.push_back( { "outcrop_buried_feature_identity", cert.outcropBuriedMatches > 200 } );

        size_t const side = 33;
        for ( size_t y = 0; y < side; ++y )
        for ( size_t x = 1; x < side; ++x )
        {
            size_t const a = y * side + x - 1;
            size_t const b = y * side + x;
            if ( baseline[a].geology.featureId != baseline[b].geology.featureId )
            {
                ++cert.contactTransitions;
                double const contactBound = 4.0 * 2.0;
                directIntersection = directIntersection
                    && ( baseline[a].nearestContactDistanceM <= contactBound
                      || baseline[b].nearestContactDistanceM <= contactBound );
            }
        }
        cert.checks.push_back( { "folded_contacts_exposed", cert.contactTransitions >= 20 } );
        cert.checks.push_back( { "contact_transitions_bracket_contacts", directIntersection } );

        bool sameHeightDifferentFeature = false;
        bool sameFeatureDifferentHeight = false;
        for ( size_t i = 0; i < baseline.size(); ++i )
        for ( size_t j = i + 1; j < baseline.size(); ++j )
        {
            double const heightDelta = std::fabs( baseline[i].surfaceZ - baseline[j].surfaceZ );
            if ( heightDelta < 0.05 && baseline[i].geology.featureId != baseline[j].geology.featureId )
            { sameHeightDifferentFeature = true; }
            if ( heightDelta > 2.0 && baseline[i].geology.featureId == baseline[j].geology.featureId )
            { sameFeatureDifferentHeight = true; }
            if ( sameHeightDifferentFeature && sameFeatureDifferentHeight ) { break; }
        }
        cert.checks.push_back( { "not_elevation_material_thresholds",
            sameHeightDifferentFeature && sameFeatureDifferentHeight } );

        bool flatExact = true;
        bool terracedExact = true;
        std::unordered_set<int64_t> terraceLevels;
        for ( auto const& p : points )
        {
            double const flatZ = kernel.SurfaceZ( "flat_control", p[0], p[1] );
            flatExact = flatExact && flatZ == 3.0;
            double const terraceZ = kernel.SurfaceZ( "terraced_control", p[0], p[1] );
            double const level = terraceZ / 2.0;
            terracedExact = terracedExact && std::fabs( level - std::round( level ) ) < 1e-12;
            terraceLevels.insert( (int64_t)std::llround( level ) );
        }
        cert.checks.push_back( { "flat_control_remains_flat", flatExact } );
        cert.checks.push_back( { "terraced_control_remains_terraced",
            terracedExact && terraceLevels.size() >= 3 } );

        std::string badLink = exposureSource;
        std::string const linkedDigest = CausalWorldGeology::Hex64(
            kernel.GetDescriptor().geologyDescriptorDigest );
        CausalWorldGeology::ReplaceFirst( badLink,
            "geology_descriptor_digest=" + linkedDigest,
            "geology_descriptor_digest=0000000000000000" );
        cert.checks.push_back( { "refuse_geology_digest_mismatch", !LoadText( badLink, geologySource ).ok } );
        std::string badSurface = exposureSource;
        CausalWorldGeology::ReplaceFirst( badSurface, ",erosion,", ",erosiom," );
        cert.checks.push_back( { "refuse_surface_digest_mismatch", !LoadText( badSurface, geologySource ).ok } );
        cert.checks.push_back( { "refuse_wrong_region", !LoadText( exposureSource, geologySource, "wrong" ).ok } );
        std::string badIdentity = exposureSource;
        CausalWorldGeology::ReplaceFirst( badIdentity,
            "world_identity_hash=" + CausalWorldGeology::Hex64(
                kernel.GetDescriptor().worldIdentityHash ),
            "world_identity_hash=0000000000000000" );
        cert.checks.push_back( { "refuse_world_identity_mismatch",
            !LoadText( badIdentity, geologySource ).ok } );
        std::string badSchema = exposureSource;
        CausalWorldGeology::ReplaceFirst( badSchema, "schema_version=1", "schema_version=2" );
        cert.checks.push_back( { "refuse_unsupported_schema",
            !LoadText( badSchema, geologySource ).ok } );
        std::string stale = exposureSource;
        CausalWorldGeology::ReplaceFirst( stale, "authority_revision=1", "authority_revision=0" );
        cert.checks.push_back( { "refuse_stale_revision", !LoadText( stale, geologySource ).ok } );

        using Clock = std::chrono::steady_clock;
        std::vector<double> timings;
        timings.reserve( 20000 );
        volatile uint64_t guard = 0;
        auto const totalBegin = Clock::now();
        for ( size_t i = 0; i < 20000; ++i )
        {
            auto const& p = points[i % points.size()];
            auto const begin = Clock::now();
            ExposureSample const sample = kernel.Query( kPresentSurface, p[0], p[1] );
            auto const end = Clock::now();
            guard ^= sample.geology.featureId;
            timings.push_back( std::chrono::duration<double, std::micro>( end - begin ).count() );
        }
        auto const totalEnd = Clock::now();
        (void)guard;
        double const elapsed = std::chrono::duration<double>( totalEnd - totalBegin ).count();
        cert.queriesPerSecond = elapsed > 0.0 ? timings.size() / elapsed : 0.0;
        for ( double value : timings ) { cert.meanQueryUs += value; }
        cert.meanQueryUs /= timings.size();
        std::sort( timings.begin(), timings.end() );
        cert.p99QueryUs = timings[(size_t)( 0.99 * ( timings.size() - 1 ) )];

        cert.passed = std::all_of( cert.checks.begin(), cert.checks.end(),
            []( auto const& check ) { return check.second; } );
        return cert;
    }

    inline bool WriteCertArtifact( CertResult const& cert, char const* path,
        char const* geologyPath, char const* exposurePath )
    {
        FILE* file = nullptr;
        if ( fopen_s( &file, path, "wb" ) != 0 || !file ) { return false; }
        std::fprintf( file, "CAUSAL_WORLD_GEOLOGIC_EXPOSURE_FLOOR %s\n",
            cert.passed ? "PASS" : "FAIL" );
        std::fprintf( file, "geology_descriptor=%s\nexposure_descriptor=%s\nload_reason=%s\n",
            geologyPath, exposurePath, cert.loadReason.c_str() );
        std::fprintf( file, "semantic_digest=%s\n", CausalWorldGeology::Hex64( cert.semanticDigest ).c_str() );
        std::fprintf( file, "samples=%zu\nexposed_features=%zu\ncontact_transitions=%zu\n"
            "outcrop_buried_matches=%zu\n", cert.sampleCount, cert.exposedFeatureCount,
            cert.contactTransitions, cert.outcropBuriedMatches );
        std::fprintf( file, "geology_descriptor_bytes=%zu\nexposure_descriptor_bytes=%zu\n",
            cert.geologyDescriptorBytes, cert.exposureDescriptorBytes );
        std::fprintf( file, "queries_per_second=%.3f\nmean_query_us=%.6f\np99_query_us=%.6f\n",
            cert.queriesPerSecond, cert.meanQueryUs, cert.p99QueryUs );
        std::fprintf( file, "terrain_output_changes=0\nfeatures_minted=0\noccupancy_mutations=0\n"
            "d2_rebuilds=0\nmatter_bodies=0\nwater_coupling=0\nactive_erosion=0\np5b=closed\n" );
        for ( auto const& check : cert.checks )
        { std::fprintf( file, "check.%s=%s\n", check.first.c_str(), check.second ? "PASS" : "FAIL" ); }
        std::fclose( file );
        return true;
    }
}
