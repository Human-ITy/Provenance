#pragma once

// Stage 9: a younger persistent granite body cuts the existing folded host.
// The pluton is a 3D authority body, never an elevation/material paint rule.

#include "CausalDifferentialErosion.h"

#include <algorithm>
#include <array>
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

namespace CausalGraniteIntrusion
{
    constexpr char const* kExpectedRegion = "causal_world_granite_intrusion_floor";

    struct Program
    {
        uint64_t worldIdentityHash = 0;
        std::string worldgenId;
        uint32_t worldgenVersion = 0;
        uint32_t schemaVersion = 0;
        std::string regionKey;
        uint64_t geologyDescriptorDigest = 0;
        uint64_t erosionDescriptorDigest = 0;
        uint32_t authorityRevision = 0;
        uint64_t featureId = 0;
        uint64_t intrusionEventId = 0;
        uint64_t coolingEventId = 0;
        uint32_t intrusionChronology = 0;
        uint32_t coolingChronology = 0;
        double centerX = 0.0, centerY = 0.0, centerZ = 0.0;
        double radiusX = 1.0, radiusY = 1.0, radiusZ = 1.0;
        double irregularity = 0.0;
        std::array<double, 3> jointNormal = { 0.0, 0.0, 1.0 };
    };

    struct LoadResult { bool ok = false; std::string reason; Program program; };

    inline bool ReadFile( char const* path, std::string& out )
    {
        std::ifstream input( path, std::ios::binary );
        if ( !input ) { return false; }
        std::ostringstream bytes; bytes << input.rdbuf(); out = bytes.str(); return true;
    }

    inline LoadResult LoadText( std::string const& source,
        CausalWorldGeology::Descriptor const& geology,
        CausalWorldExposure::Descriptor const& exposure,
        std::string const& geologySource,
        std::string const& erosionSource,
        std::string const& expectedRegion = kExpectedRegion )
    {
        LoadResult result;
        std::unordered_map<std::string, std::string> fields;
        std::istringstream stream( source );
        std::string line; bool magic = false;
        while ( std::getline( stream, line ) )
        {
            if ( !line.empty() && line.back() == '\r' ) { line.pop_back(); }
            if ( line.empty() || line[0] == '#' ) { continue; }
            if ( !magic )
            {
                magic = line == "PROVENANCE_CAUSAL_GRANITE_INTRUSION_V1";
                if ( !magic ) { result.reason = "bad_magic"; return result; }
                continue;
            }
            size_t const eq = line.find( '=' );
            if ( eq == std::string::npos ) { result.reason = "malformed_line"; return result; }
            std::string const key = line.substr( 0, eq );
            if ( fields.count( key ) ) { result.reason = "duplicate_key:" + key; return result; }
            fields.emplace( key, line.substr( eq + 1 ) );
        }
        auto get = [&]( char const* key ) -> std::string const*
        { auto it = fields.find( key ); return it == fields.end() ? nullptr : &it->second; };
        std::string const* value = get( "world_identity_hash" );
        if ( !value || !CausalWorldGeology::ParseHex64( *value, result.program.worldIdentityHash ) )
        { result.reason = "invalid_world_identity"; return result; }
        value = get( "worldgen_id" ); if ( !value ) { result.reason = "missing_worldgen"; return result; }
        result.program.worldgenId = *value;
        value = get( "worldgen_version" );
        if ( !value || !CausalWorldGeology::ParseU32( *value, result.program.worldgenVersion ) )
        { result.reason = "invalid_worldgen_version"; return result; }
        value = get( "schema_version" );
        if ( !value || !CausalWorldGeology::ParseU32( *value, result.program.schemaVersion ) )
        { result.reason = "invalid_schema"; return result; }
        value = get( "region_key" ); if ( !value ) { result.reason = "missing_region"; return result; }
        result.program.regionKey = *value;
        auto hex = [&]( char const* key, uint64_t& destination )
        { auto field = get( key ); return field && CausalWorldGeology::ParseHex64( *field, destination ); };
        auto u32 = [&]( char const* key, uint32_t& destination )
        { auto field = get( key ); return field && CausalWorldGeology::ParseU32( *field, destination ); };
        auto number = [&]( char const* key, double& destination )
        { auto field = get( key ); return field && CausalWorldGeology::ParseDouble( *field, destination ); };
        if ( !hex( "geology_descriptor_digest", result.program.geologyDescriptorDigest )
          || !hex( "erosion_descriptor_digest", result.program.erosionDescriptorDigest )
          || !u32( "authority_revision", result.program.authorityRevision )
          || !hex( "feature_id", result.program.featureId )
          || !hex( "intrusion_event_id", result.program.intrusionEventId )
          || !hex( "cooling_event_id", result.program.coolingEventId )
          || !u32( "intrusion_chronology", result.program.intrusionChronology )
          || !u32( "cooling_chronology", result.program.coolingChronology )
          || !number( "center_x_m", result.program.centerX )
          || !number( "center_y_m", result.program.centerY )
          || !number( "center_z_m", result.program.centerZ )
          || !number( "radius_x_m", result.program.radiusX )
          || !number( "radius_y_m", result.program.radiusY )
          || !number( "radius_z_m", result.program.radiusZ )
          || !number( "irregularity", result.program.irregularity )
          || !number( "joint_nx", result.program.jointNormal[0] )
          || !number( "joint_ny", result.program.jointNormal[1] )
          || !number( "joint_nz", result.program.jointNormal[2] ) )
        { result.reason = "invalid_program_values"; return result; }
        if ( result.program.worldIdentityHash != geology.authority.worldIdentityHash
          || result.program.worldIdentityHash != exposure.worldIdentityHash )
        { result.reason = "world_identity_mismatch"; return result; }
        if ( result.program.worldgenId != CausalWorldGeology::kExpectedWorldgenId
          || result.program.worldgenVersion != 1 || result.program.schemaVersion != 1 )
        { result.reason = "unsupported_authority"; return result; }
        if ( result.program.regionKey != expectedRegion )
        { result.reason = "wrong_region"; return result; }
        if ( result.program.geologyDescriptorDigest != CausalWorldGeology::HashText( geologySource )
          || result.program.erosionDescriptorDigest != CausalWorldGeology::HashText( erosionSource ) )
        { result.reason = "authority_link_mismatch"; return result; }
        if ( result.program.authorityRevision == 0 || result.program.featureId == 0
          || result.program.intrusionEventId == 0 || result.program.coolingEventId == 0
          || result.program.intrusionChronology <= 30
          || result.program.coolingChronology <= result.program.intrusionChronology
          || result.program.radiusX <= 0.0 || result.program.radiusY <= 0.0
          || result.program.radiusZ <= 0.0 || std::fabs( result.program.irregularity ) > 0.25 )
        { result.reason = "invalid_intrusion_contract"; return result; }
        double const nl = std::sqrt( result.program.jointNormal[0] * result.program.jointNormal[0]
            + result.program.jointNormal[1] * result.program.jointNormal[1]
            + result.program.jointNormal[2] * result.program.jointNormal[2] );
        if ( nl < 0.99 || nl > 1.01 ) { result.reason = "invalid_joint_frame"; return result; }
        for ( auto const& formation : geology.formations )
        { if ( formation.featureId == result.program.featureId ) { result.reason = "feature_id_collision"; return result; } }
        result.ok = true; result.reason = "ok"; return result;
    }

    class Kernel
    {
    public:
        Kernel( CausalDifferentialErosion::Kernel erosion, Program program )
            : m_erosion( std::move( erosion ) ), m_program( std::move( program ) ) {}

        double BodyField( double x, double y, double z ) const
        {
            double const dx = ( x - m_program.centerX ) / m_program.radiusX;
            double const dy = ( y - m_program.centerY ) / m_program.radiusY;
            double const dz = ( z - m_program.centerZ ) / m_program.radiusZ;
            double const warp = m_program.irregularity
                * std::sin( dy * 5.1 + dz * 2.7 ) * std::cos( dx * 4.3 - dz * 1.9 );
            return dx * dx + dy * dy + dz * dz + warp;
        }

        bool Occupies( double x, double y, double z ) const { return BodyField( x, y, z ) <= 1.0; }

        CausalWorldGeology::GeoSample Query( bool enabled, double x, double y, double z ) const
        {
            CausalWorldGeology::GeoSample const host =
                m_erosion.Exposure().Geology().Query( x, y, z );
            if ( !enabled || !Occupies( x, y, z ) ) { return host; }
            CausalWorldGeology::GeoSample granite;
            granite.found = true;
            granite.regionId = CausalWorldGeology::HashText( m_program.regionKey );
            granite.featureId = m_program.featureId;
            granite.formationId = "G01_PLUTON";
            granite.material = "granite";
            granite.structuralNormal = m_program.jointNormal;
            granite.bodyLocalPosition = { x - m_program.centerX,
                y - m_program.centerY, z - m_program.centerZ };
            granite.eventIds = { m_program.intrusionEventId, m_program.coolingEventId };
            granite.chronology = { m_program.intrusionChronology, m_program.coolingChronology };
            granite.boundary = CausalWorldGeology::BoundaryState::Interior;
            granite.descriptorRevision = m_program.authorityRevision;
            return granite;
        }

        CausalWorldGeology::MaterialSample QueryMaterial(
            bool enabled, double x, double y, double z ) const
        {
            if ( enabled && Occupies( x, y, z ) )
            { return { true, "granite", m_program.coolingChronology }; }
            return m_erosion.Exposure().Geology().QueryMaterial( x, y, z );
        }

        CausalWorldGeology::GeoSample SurfaceGeology( bool enabled, double x, double y ) const
        {
            double const z = ReconstructedZ( x, y );
            return Query( enabled, x, y, z - 0.001 );
        }

        double ReconstructedZ( double x, double y ) const
        { return m_erosion.ReconstructedZ( CausalDifferentialErosion::Control::DifferentialResistance, x, y ); }
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock( int bx, int by ) const
        { return m_erosion.SampleBlock(
            CausalDifferentialErosion::Control::DifferentialResistance, bx, by ); }
        CausalVisibleExposure::BlockMesh BuildBlock( int bx, int by ) const
        { return m_erosion.BuildBlock( CausalDifferentialErosion::Control::DifferentialResistance, bx, by ); }
        CausalDifferentialErosion::Kernel const& Erosion() const { return m_erosion; }
        Program const& GetProgram() const { return m_program; }

    private:
        CausalDifferentialErosion::Kernel m_erosion;
        Program m_program;
    };

    struct CertResult
    {
        bool passed = false;
        std::string loadReason;
        uint64_t semanticDigest = 0;
        size_t surfaceSamples = 0;
        size_t graniteSurfaceSamples = 0;
        size_t truncatedHostSamples = 0;
        size_t graniteContinuitySamples = 0;
        double graniteMinSurfaceZ = 0.0;
        double graniteMaxSurfaceZ = 0.0;
        std::vector<std::pair<std::string, bool>> checks;
    };

    inline CertResult RunCert( char const* geologyPath, char const* exposurePath,
        char const* erosionPath, char const* intrusionPath )
    {
        CertResult cert;
        auto const stage8 = CausalDifferentialErosion::RunCert(
            geologyPath, exposurePath, erosionPath );
        cert.checks.push_back( { "stage8_differential_erosion_certificate", stage8.passed } );
        if ( !stage8.passed ) { cert.loadReason = stage8.loadReason; return cert; }
        std::string geologySource, exposureSource, erosionSource, intrusionSource;
        if ( !ReadFile( geologyPath, geologySource ) || !ReadFile( exposurePath, exposureSource )
          || !ReadFile( erosionPath, erosionSource ) || !ReadFile( intrusionPath, intrusionSource ) )
        { cert.loadReason = "descriptor_missing"; return cert; }
        auto geology = CausalWorldGeology::LoadText( geologySource );
        auto exposure = CausalWorldExposure::LoadText( exposureSource, geologySource );
        CausalDifferentialErosion::LoadResult erosion;
        if ( geology.ok && exposure.ok )
        { erosion = CausalDifferentialErosion::LoadText( erosionSource,
            geology.descriptor, exposure.descriptor, geologySource ); }
        LoadResult intrusion;
        if ( geology.ok && exposure.ok && erosion.ok )
        { intrusion = LoadText( intrusionSource, geology.descriptor,
            exposure.descriptor, geologySource, erosionSource ); }
        bool const loaded = geology.ok && exposure.ok && erosion.ok && intrusion.ok;
        cert.loadReason = loaded ? "ok" : ( !geology.ok ? geology.reason
            : ( !exposure.ok ? exposure.reason : ( !erosion.ok ? erosion.reason : intrusion.reason ) ) );
        cert.checks.push_back( { "linked_intrusion_authority_load", loaded } );
        if ( !loaded ) { return cert; }
        Kernel kernel( CausalDifferentialErosion::Kernel(
            CausalWorldExposure::Kernel( geology.descriptor, exposure.descriptor ),
            erosion.program ), intrusion.program );

        bool disabledClean = true, onlyInside = true, noBlend = true;
        bool chronology = true, jointFrame = true, directSurfaceQuery = true;
        bool hostIdsPreserved = true;
        std::unordered_set<uint64_t> hostIds;
        for ( auto const& formation : geology.descriptor.formations )
        { hostIds.insert( formation.featureId ); }
        uint64_t hash = 14695981039346656037ull;
        cert.graniteMinSurfaceZ = 1e9; cert.graniteMaxSurfaceZ = -1e9;
        std::vector<std::array<double, 3>> granitePoints;
        for ( int y = -32; y <= 32; ++y )
        for ( int x = -32; x <= 32; ++x )
        {
            double const wx = x + 0.5, wy = y + 0.5;
            double const z = kernel.ReconstructedZ( wx, wy );
            auto const disabled = kernel.Query( false, wx, wy, z - 0.001 );
            auto const enabled = kernel.Query( true, wx, wy, z - 0.001 );
            auto const direct = kernel.SurfaceGeology( true, wx, wy );
            ++cert.surfaceSamples;
            disabledClean = disabledClean && disabled.material != "granite"
                && hostIds.count( disabled.featureId ) != 0;
            directSurfaceQuery = directSurfaceQuery && direct.featureId == enabled.featureId
                && direct.material == enabled.material;
            onlyInside = onlyInside && ( enabled.featureId != intrusion.program.featureId
                || kernel.Occupies( wx, wy, z - 0.001 ) );
            noBlend = noBlend && ( enabled.material == "granite"
                || enabled.material == "sandstone" || enabled.material == "shale" );
            if ( enabled.featureId == intrusion.program.featureId )
            {
                ++cert.graniteSurfaceSamples;
                granitePoints.push_back( { wx, wy,z } );
                cert.graniteMinSurfaceZ = (std::min)( cert.graniteMinSurfaceZ, z );
                cert.graniteMaxSurfaceZ = (std::max)( cert.graniteMaxSurfaceZ, z );
                if ( disabled.featureId != enabled.featureId ) { ++cert.truncatedHostSamples; }
                chronology = chronology && enabled.chronology.size() == 2
                    && enabled.chronology[0] > 30 && enabled.chronology[1] > enabled.chronology[0];
                double dot = enabled.structuralNormal[0] * disabled.structuralNormal[0]
                    + enabled.structuralNormal[1] * disabled.structuralNormal[1]
                    + enabled.structuralNormal[2] * disabled.structuralNormal[2];
                jointFrame = jointFrame && std::fabs( dot ) < 0.995;
            }
            else
            { hostIdsPreserved = hostIdsPreserved && hostIds.count( enabled.featureId ) != 0; }
            CausalWorldGeology::HashAppend( hash, &enabled.featureId, sizeof( enabled.featureId ) );
        }
        cert.semanticDigest = hash;
        for ( auto const& point : granitePoints )
        {
            auto const down = kernel.Query( true, point[0], point[1], point[2] - 5.0 );
            double const towardCenterX = point[0] + ( intrusion.program.centerX - point[0] ) * 0.25;
            double const towardCenterY = point[1] + ( intrusion.program.centerY - point[1] ) * 0.25;
            auto const lateral = kernel.Query( true, towardCenterX, towardCenterY, point[2] - 1.0 );
            if ( down.featureId == intrusion.program.featureId
              && lateral.featureId == intrusion.program.featureId )
            { ++cert.graniteContinuitySamples; }
        }

        cert.checks.push_back( { "disabled_control_contains_no_granite", disabledClean } );
        cert.checks.push_back( { "younger_granite_feature_is_new",
            hostIds.count( intrusion.program.featureId ) == 0 && intrusion.program.featureId != 0 } );
        cert.checks.push_back( { "intrusion_truncates_older_host",
            cert.truncatedHostSamples > 250 } );
        cert.checks.push_back( { "granite_only_where_body_occupies", onlyInside } );
        cert.checks.push_back( { "no_host_granite_blending", noBlend } );
        cert.checks.push_back( { "chronology_resolves_overlap", chronology } );
        cert.checks.push_back( { "granite_joint_frame_not_bedding", jointFrame } );
        cert.checks.push_back( { "surface_requeries_resulting_world", directSurfaceQuery } );
        cert.checks.push_back( { "sedimentary_feature_ids_preserved", hostIdsPreserved } );
        cert.checks.push_back( { "three_dimensional_feature_continuity",
            cert.graniteContinuitySamples > cert.graniteSurfaceSamples / 2 } );
        cert.checks.push_back( { "not_an_elevation_rule",
            cert.graniteMaxSurfaceZ - cert.graniteMinSurfaceZ > 2.0 } );

        // Tiled, reversed, and monolithic query order must not change identity.
        std::vector<std::pair<std::array<int, 2>, uint64_t>> reversed;
        for ( int y = 32; y >= -32; --y )
        for ( int x = 32; x >= -32; --x )
        {
            double const wx = x + 0.5, wy = y + 0.5;
            auto const sample = kernel.SurfaceGeology( true, wx, wy );
            reversed.push_back( { { x, y }, sample.featureId } );
        }
        std::vector<std::pair<std::array<int, 2>, uint64_t>> canonical;
        for ( int y = -32; y <= 32; ++y ) for ( int x = -32; x <= 32; ++x )
        { canonical.push_back( { { x, y }, kernel.SurfaceGeology( true, x + 0.5, y + 0.5 ).featureId } ); }
        auto byPoint = []( auto const& a, auto const& b ) { return a.first < b.first; };
        std::sort( canonical.begin(), canonical.end(), byPoint );
        std::sort( reversed.begin(), reversed.end(), byPoint );
        cert.checks.push_back( { "partition_and_order_invariant_identity", reversed == canonical } );
        auto const stage9Block = kernel.BuildBlock( 0, 0 );
        auto const stage8Block = kernel.Erosion().BuildBlock(
            CausalDifferentialErosion::Control::DifferentialResistance, 0, 0 );
        cert.checks.push_back( { "stage8_geometry_preserved",
            CausalVisibleExposure::GeometryDigest( stage9Block.triangles )
                == CausalVisibleExposure::GeometryDigest( stage8Block.triangles ) } );
        cert.checks.push_back( { "bounded_64m_surface", cert.surfaceSamples == 4225 } );
        cert.passed = std::all_of( cert.checks.begin(), cert.checks.end(),
            []( auto const& check ) { return check.second; } );
        return cert;
    }

    inline bool WriteCertArtifact( CertResult const& cert, char const* path )
    {
        FILE* file = nullptr;
        if ( fopen_s( &file, path, "wb" ) != 0 || !file ) { return false; }
        std::fprintf( file, "CAUSAL_WORLD_GRANITE_INTRUSION %s\nload_reason=%s\n"
            "semantic_digest=%s\nsurface_samples=%zu\ngranite_surface_samples=%zu\n"
            "truncated_host_samples=%zu\ngranite_continuity_samples=%zu\n"
            "granite_surface_z_min=%.6f\ngranite_surface_z_max=%.6f\n",
            cert.passed ? "PASS" : "FAIL", cert.loadReason.c_str(),
            CausalWorldGeology::Hex64( cert.semanticDigest ).c_str(), cert.surfaceSamples,
            cert.graniteSurfaceSamples, cert.truncatedHostSamples,
            cert.graniteContinuitySamples, cert.graniteMinSurfaceZ, cert.graniteMaxSurfaceZ );
        std::fprintf( file, "new_sedimentary_features=0\nactive_erosion=0\nwater_coupling=0\n"
            "mineralization=0\nfault=0\np5b=closed\n" );
        for ( auto const& check : cert.checks )
        { std::fprintf( file, "check.%s=%s\n", check.first.c_str(), check.second ? "PASS" : "FAIL" ); }
        std::fclose( file ); return true;
    }
}
