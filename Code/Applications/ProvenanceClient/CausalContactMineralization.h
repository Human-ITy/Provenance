#pragma once

// Stage 10: a deposit system caused by the Stage-9 granite contact.
// Mineralization is admitted only in valid host rock, inside a bounded contact
// band, and through a deterministic structural-permeability field.

#include "CausalGraniteIntrusion.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalContactMineralization
{
    constexpr char const* kExpectedRegion = "causal_world_contact_mineralization_floor";

    struct Program
    {
        uint64_t worldIdentityHash = 0;
        std::string worldgenId;
        uint32_t worldgenVersion = 0;
        uint32_t schemaVersion = 0;
        std::string regionKey;
        uint64_t intrusionDescriptorDigest = 0;
        uint32_t authorityRevision = 0;
        uint64_t depositSystemId = 0;
        uint64_t featureId = 0;
        uint64_t parentIntrusionFeatureId = 0;
        uint64_t parentIntrusionEventId = 0;
        uint64_t mineralizingEventId = 0;
        uint32_t mineralizingChronology = 0;
        double contactMinM = 0.0;
        double contactMaxM = 0.0;
        double permeabilityThreshold = 0.0;
        double veinThreshold = 0.0;
        std::array<double, 3> fluidDirection = { 0.0, 0.0, 1.0 };
    };

    struct LoadResult { bool ok = false; std::string reason; Program program; };

    inline bool ReadFile( char const* path, std::string& out )
    {
        std::ifstream input( path, std::ios::binary );
        if ( !input ) { return false; }
        std::ostringstream bytes; bytes << input.rdbuf(); out = bytes.str(); return true;
    }

    inline LoadResult LoadText( std::string const& source,
        CausalGraniteIntrusion::Program const& intrusion,
        std::string const& intrusionSource,
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
                magic = line == "PROVENANCE_CAUSAL_CONTACT_MINERALIZATION_V1";
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
        auto hex = [&]( char const* key, uint64_t& destination )
        { auto field = get( key ); return field && CausalWorldGeology::ParseHex64( *field, destination ); };
        auto u32 = [&]( char const* key, uint32_t& destination )
        { auto field = get( key ); return field && CausalWorldGeology::ParseU32( *field, destination ); };
        auto number = [&]( char const* key, double& destination )
        { auto field = get( key ); return field && CausalWorldGeology::ParseDouble( *field, destination ); };
        std::string const* value = get( "worldgen_id" );
        if ( !value ) { result.reason = "missing_worldgen"; return result; }
        result.program.worldgenId = *value;
        value = get( "region_key" );
        if ( !value ) { result.reason = "missing_region"; return result; }
        result.program.regionKey = *value;
        if ( !hex( "world_identity_hash", result.program.worldIdentityHash )
          || !u32( "worldgen_version", result.program.worldgenVersion )
          || !u32( "schema_version", result.program.schemaVersion )
          || !hex( "intrusion_descriptor_digest", result.program.intrusionDescriptorDigest )
          || !u32( "authority_revision", result.program.authorityRevision )
          || !hex( "deposit_system_id", result.program.depositSystemId )
          || !hex( "feature_id", result.program.featureId )
          || !hex( "parent_intrusion_feature_id", result.program.parentIntrusionFeatureId )
          || !hex( "parent_intrusion_event_id", result.program.parentIntrusionEventId )
          || !hex( "mineralizing_event_id", result.program.mineralizingEventId )
          || !u32( "mineralizing_chronology", result.program.mineralizingChronology )
          || !number( "contact_min_m", result.program.contactMinM )
          || !number( "contact_max_m", result.program.contactMaxM )
          || !number( "permeability_threshold", result.program.permeabilityThreshold )
          || !number( "vein_threshold", result.program.veinThreshold )
          || !number( "fluid_nx", result.program.fluidDirection[0] )
          || !number( "fluid_ny", result.program.fluidDirection[1] )
          || !number( "fluid_nz", result.program.fluidDirection[2] ) )
        { result.reason = "invalid_program_values"; return result; }
        double const nl = std::sqrt(
            result.program.fluidDirection[0] * result.program.fluidDirection[0]
          + result.program.fluidDirection[1] * result.program.fluidDirection[1]
          + result.program.fluidDirection[2] * result.program.fluidDirection[2] );
        if ( result.program.worldIdentityHash != intrusion.worldIdentityHash
          || result.program.worldgenId != intrusion.worldgenId
          || result.program.worldgenVersion != intrusion.worldgenVersion
          || result.program.schemaVersion != 1 || result.program.regionKey != expectedRegion )
        { result.reason = "authority_identity_mismatch"; return result; }
        if ( result.program.intrusionDescriptorDigest != CausalWorldGeology::HashText( intrusionSource )
          || result.program.parentIntrusionFeatureId != intrusion.featureId
          || result.program.parentIntrusionEventId != intrusion.intrusionEventId )
        { result.reason = "intrusion_ancestry_mismatch"; return result; }
        if ( result.program.authorityRevision == 0 || result.program.depositSystemId == 0
          || result.program.featureId == 0 || result.program.mineralizingEventId == 0
          || result.program.featureId == intrusion.featureId
          || result.program.mineralizingChronology <= intrusion.coolingChronology
          || result.program.contactMinM < 0.0
          || result.program.contactMaxM <= result.program.contactMinM
          || result.program.contactMaxM > 8.0
          || result.program.permeabilityThreshold <= 0.0
          || result.program.permeabilityThreshold >= 1.0
          || result.program.veinThreshold <= 0.0 || result.program.veinThreshold >= 1.0
          || nl < 0.99 || nl > 1.01 )
        { result.reason = "invalid_mineralization_contract"; return result; }
        result.ok = true; result.reason = "ok"; return result;
    }

    class Kernel
    {
    public:
        Kernel( CausalGraniteIntrusion::Kernel intrusion, Program program )
            : m_intrusion( std::move( intrusion ) ), m_program( std::move( program ) ) {}

        double ContactDistanceM( double x, double y, double z ) const
        {
            double const f = (std::max)( 0.0, m_intrusion.BodyField( x, y, z ) );
            double const scale = (std::min)( m_intrusion.GetProgram().radiusX,
                (std::min)( m_intrusion.GetProgram().radiusY,
                    m_intrusion.GetProgram().radiusZ ) );
            return std::fabs( std::sqrt( f ) - 1.0 ) * scale;
        }

        double Permeability( CausalWorldGeology::GeoSample const& host,
            double x, double y, double z ) const
        {
            double const structural = std::fabs(
                host.structuralNormal[0] * m_program.fluidDirection[0]
              + host.structuralNormal[1] * m_program.fluidDirection[1]
              + host.structuralNormal[2] * m_program.fluidDirection[2] );
            double const fracture = 0.5 + 0.5 * std::sin(
                x * 0.47 + y * 0.71 + z * 0.29 + std::sin( y * 0.19 ) );
            return 0.38 * structural + 0.62 * fracture;
        }

        bool Admitted( bool eventEnabled, double permeabilityThreshold,
            double x, double y, double z ) const
        {
            if ( !eventEnabled || m_intrusion.Occupies( x, y, z ) ) { return false; }
            CausalWorldGeology::GeoSample const host = m_intrusion.Query( false, x, y, z );
            if ( !host.found || ( host.material != "sandstone" && host.material != "shale" ) )
            { return false; }
            double const contact = ContactDistanceM( x, y, z );
            if ( contact < m_program.contactMinM || contact > m_program.contactMaxM )
            { return false; }
            if ( Permeability( host, x, y, z ) < permeabilityThreshold ) { return false; }
            double const vein = std::fabs( std::sin(
                x * 0.83 - y * 0.37 + z * 0.41 + host.featureId % 17 ) );
            return vein <= m_program.veinThreshold;
        }

        bool AdmittedMaterial( bool eventEnabled, double permeabilityThreshold,
            double x, double y, double z ) const
        {
            if ( !eventEnabled || m_intrusion.Occupies( x, y, z ) ) { return false; }
            CausalWorldGeology::MaterialSample const hostMaterial =
                m_intrusion.QueryMaterial( false, x, y, z );
            if ( !hostMaterial.found || ( std::strcmp( hostMaterial.material, "sandstone" ) != 0
              && std::strcmp( hostMaterial.material, "shale" ) != 0 ) ) { return false; }
            double const contact = ContactDistanceM( x, y, z );
            if ( contact < m_program.contactMinM || contact > m_program.contactMaxM )
            { return false; }
            // Structural permeability and the vein phase require the complete
            // host sample only after the cheap material/contact gates admit it.
            CausalWorldGeology::GeoSample const host = m_intrusion.Query( false, x, y, z );
            if ( !host.found || Permeability( host, x, y, z ) < permeabilityThreshold )
            { return false; }
            double const vein = std::fabs( std::sin(
                x * 0.83 - y * 0.37 + z * 0.41 + host.featureId % 17 ) );
            return vein <= m_program.veinThreshold;
        }

        CausalWorldGeology::GeoSample Query( bool eventEnabled, double x, double y, double z,
            double permeabilityThreshold = -1.0 ) const
        {
            CausalWorldGeology::GeoSample const assembled = m_intrusion.Query( true, x, y, z );
            double const threshold = permeabilityThreshold >= 0.0
                ? permeabilityThreshold : m_program.permeabilityThreshold;
            if ( !Admitted( eventEnabled, threshold, x, y, z ) ) { return assembled; }
            CausalWorldGeology::GeoSample deposit;
            deposit.found = true;
            deposit.regionId = CausalWorldGeology::HashText( m_program.regionKey );
            deposit.featureId = m_program.featureId;
            deposit.formationId = "D01_CONTACT_QUARTZ";
            deposit.material = "quartz";
            deposit.structuralNormal = m_program.fluidDirection;
            deposit.bodyLocalPosition = { x - m_intrusion.GetProgram().centerX,
                y - m_intrusion.GetProgram().centerY, z - m_intrusion.GetProgram().centerZ };
            deposit.eventIds = { m_program.parentIntrusionEventId,
                m_program.mineralizingEventId };
            deposit.chronology = { m_intrusion.GetProgram().intrusionChronology,
                m_program.mineralizingChronology };
            deposit.boundary = CausalWorldGeology::BoundaryState::Interior;
            deposit.descriptorRevision = m_program.authorityRevision;
            return deposit;
        }

        CausalWorldGeology::MaterialSample QueryMaterial( bool eventEnabled,
            double x, double y, double z, double permeabilityThreshold = -1.0 ) const
        {
            CausalWorldGeology::MaterialSample const assembled =
                m_intrusion.QueryMaterial( true, x, y, z );
            double const threshold = permeabilityThreshold >= 0.0
                ? permeabilityThreshold : m_program.permeabilityThreshold;
            if ( !AdmittedMaterial( eventEnabled, threshold, x, y, z ) )
            { return assembled; }
            return { true, "quartz", m_program.mineralizingChronology };
        }

        CausalWorldGeology::GeoSample SurfaceGeology( bool eventEnabled,
            double x, double y, double permeabilityThreshold = -1.0 ) const
        {
            double const z = ReconstructedZ( x, y );
            return Query( eventEnabled, x, y, z - 0.001, permeabilityThreshold );
        }

        double ReconstructedZ( double x, double y ) const
        { return m_intrusion.ReconstructedZ( x, y ); }
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock( int bx, int by ) const
        { return m_intrusion.SampleBlock( bx, by ); }
        CausalVisibleExposure::BlockMesh BuildBlock( int bx, int by ) const
        { return m_intrusion.BuildBlock( bx, by ); }
        CausalGraniteIntrusion::Kernel const& Intrusion() const { return m_intrusion; }
        Program const& GetProgram() const { return m_program; }

    private:
        CausalGraniteIntrusion::Kernel m_intrusion;
        Program m_program;
    };

    struct CertResult
    {
        bool passed = false;
        std::string loadReason;
        uint64_t semanticDigest = 0;
        size_t surfaceSamples = 0;
        size_t depositSurfaceSamples = 0;
        size_t disabledDepositSamples = 0;
        size_t stricterDepositSamples = 0;
        std::vector<std::pair<std::string, bool>> checks;
    };

    inline CertResult RunCert( char const* geologyPath, char const* exposurePath,
        char const* erosionPath, char const* intrusionPath, char const* mineralizationPath )
    {
        CertResult cert;
        auto const stage9 = CausalGraniteIntrusion::RunCert(
            geologyPath, exposurePath, erosionPath, intrusionPath );
        cert.checks.push_back( { "stage9_granite_intrusion_certificate", stage9.passed } );
        if ( !stage9.passed ) { cert.loadReason = stage9.loadReason; return cert; }

        std::string gs, es, ers, is, ms;
        if ( !ReadFile( geologyPath, gs ) || !ReadFile( exposurePath, es )
          || !ReadFile( erosionPath, ers ) || !ReadFile( intrusionPath, is )
          || !ReadFile( mineralizationPath, ms ) )
        { cert.loadReason = "descriptor_missing"; return cert; }
        auto geology = CausalWorldGeology::LoadText( gs );
        auto exposure = CausalWorldExposure::LoadText( es, gs );
        CausalDifferentialErosion::LoadResult erosion;
        CausalGraniteIntrusion::LoadResult intrusion;
        LoadResult mineralization;
        if ( geology.ok && exposure.ok )
        { erosion = CausalDifferentialErosion::LoadText(
            ers, geology.descriptor, exposure.descriptor, gs ); }
        if ( geology.ok && exposure.ok && erosion.ok )
        { intrusion = CausalGraniteIntrusion::LoadText(
            is, geology.descriptor, exposure.descriptor, gs, ers ); }
        if ( intrusion.ok ) { mineralization = LoadText( ms, intrusion.program, is ); }
        bool const loaded = geology.ok && exposure.ok && erosion.ok
            && intrusion.ok && mineralization.ok;
        cert.loadReason = loaded ? "ok" : ( !geology.ok ? geology.reason
            : ( !exposure.ok ? exposure.reason : ( !erosion.ok ? erosion.reason
            : ( !intrusion.ok ? intrusion.reason : mineralization.reason ) ) ) );
        cert.checks.push_back( { "linked_mineralization_authority_load", loaded } );
        if ( !loaded ) { return cert; }

        auto makeKernel = [&]()
        {
            CausalWorldExposure::Kernel exposureKernel( geology.descriptor, exposure.descriptor );
            CausalDifferentialErosion::Kernel erosionKernel(
                std::move( exposureKernel ), erosion.program );
            CausalGraniteIntrusion::Kernel intrusionKernel(
                std::move( erosionKernel ), intrusion.program );
            return Kernel( std::move( intrusionKernel ), mineralization.program );
        };
        Kernel kernel = makeKernel();

        bool validAdmission = true, unchangedUnderlying = true;
        bool directSurfaceQuery = true, chronology = true;
        uint64_t hash = 14695981039346656037ull;
        std::vector<std::pair<std::array<int, 2>, uint64_t>> canonical;
        for ( int y = -32; y <= 32; ++y )
        for ( int x = -32; x <= 32; ++x )
        {
            double const wx = x + 0.5, wy = y + 0.5;
            double const z = kernel.ReconstructedZ( wx, wy );
            auto const disabled = kernel.Query( false, wx, wy, z - 0.001 );
            auto const enabled = kernel.Query( true, wx, wy, z - 0.001 );
            auto const direct = kernel.SurfaceGeology( true, wx, wy );
            auto const stricter = kernel.SurfaceGeology( true, wx, wy, 0.86 );
            ++cert.surfaceSamples;
            if ( disabled.featureId == mineralization.program.featureId )
            { ++cert.disabledDepositSamples; }
            if ( stricter.featureId == mineralization.program.featureId )
            { ++cert.stricterDepositSamples; }
            unchangedUnderlying = unchangedUnderlying
                && disabled.featureId == kernel.Intrusion().SurfaceGeology( true, wx, wy ).featureId;
            directSurfaceQuery = directSurfaceQuery && direct.featureId == enabled.featureId;
            if ( enabled.featureId == mineralization.program.featureId )
            {
                ++cert.depositSurfaceSamples;
                auto const host = kernel.Intrusion().Query( false, wx, wy, z - 0.001 );
                double const contact = kernel.ContactDistanceM( wx, wy, z - 0.001 );
                validAdmission = validAdmission && !kernel.Intrusion().Occupies( wx, wy, z - 0.001 )
                    && ( host.material == "sandstone" || host.material == "shale" )
                    && contact >= mineralization.program.contactMinM
                    && contact <= mineralization.program.contactMaxM
                    && kernel.Permeability( host, wx, wy, z - 0.001 )
                        >= mineralization.program.permeabilityThreshold;
                chronology = chronology && enabled.eventIds.size() == 2
                    && enabled.eventIds[0] == intrusion.program.intrusionEventId
                    && enabled.eventIds[1] == mineralization.program.mineralizingEventId
                    && enabled.chronology.size() == 2
                    && enabled.chronology[1] > intrusion.program.coolingChronology;
            }
            canonical.push_back( { { x, y }, enabled.featureId } );
            CausalWorldGeology::HashAppend( hash, &enabled.featureId, sizeof( enabled.featureId ) );
        }
        cert.semanticDigest = hash;

        std::vector<std::pair<std::array<int, 2>, uint64_t>> reversed;
        for ( int y = 32; y >= -32; --y ) for ( int x = 32; x >= -32; --x )
        { reversed.push_back( { { x, y }, kernel.SurfaceGeology( true, x + 0.5, y + 0.5 ).featureId } ); }
        auto byPoint = []( auto const& a, auto const& b ) { return a.first < b.first; };
        std::sort( canonical.begin(), canonical.end(), byPoint );
        std::sort( reversed.begin(), reversed.end(), byPoint );
        Kernel reloaded = makeKernel();
        bool reloadStable = true;
        for ( int y = -32; y <= 32 && reloadStable; y += 4 )
        for ( int x = -32; x <= 32; x += 4 )
        { reloadStable = reloadStable && kernel.SurfaceGeology( true, x + 0.5, y + 0.5 ).featureId
            == reloaded.SurfaceGeology( true, x + 0.5, y + 0.5 ).featureId; }

        auto const stage10Block = kernel.BuildBlock( 0, 0 );
        auto const stage9Block = kernel.Intrusion().BuildBlock( 0, 0 );
        cert.checks.push_back( { "event_disabled_produces_zero_deposits",
            cert.disabledDepositSamples == 0 } );
        cert.checks.push_back( { "host_and_contact_valid_admission",
            validAdmission && cert.depositSurfaceSamples >= 12 } );
        cert.checks.push_back( { "permeability_control_moves_or_removes_deposits",
            cert.stricterDepositSamples < cert.depositSurfaceSamples } );
        cert.checks.push_back( { "stable_deposit_identity_and_chronology", chronology } );
        cert.checks.push_back( { "underlying_stage9_geology_unchanged", unchangedUnderlying } );
        cert.checks.push_back( { "surface_requeries_assembled_world", directSurfaceQuery } );
        cert.checks.push_back( { "partition_and_query_order_invariant", canonical == reversed } );
        cert.checks.push_back( { "reload_deterministic_identity", reloadStable } );
        cert.checks.push_back( { "stage9_geometry_preserved",
            CausalVisibleExposure::GeometryDigest( stage10Block.triangles )
                == CausalVisibleExposure::GeometryDigest( stage9Block.triangles ) } );
        cert.checks.push_back( { "bounded_64m_surface", cert.surfaceSamples == 4225 } );
        cert.passed = std::all_of( cert.checks.begin(), cert.checks.end(),
            []( auto const& check ) { return check.second; } );
        return cert;
    }

    inline bool WriteCertArtifact( CertResult const& cert, char const* path )
    {
        FILE* file = nullptr;
        if ( fopen_s( &file, path, "wb" ) != 0 || !file ) { return false; }
        std::fprintf( file, "CAUSAL_WORLD_CONTACT_MINERALIZATION %s\nload_reason=%s\n"
            "semantic_digest=%s\nsurface_samples=%zu\ndeposit_surface_samples=%zu\n"
            "disabled_deposit_samples=%zu\nstricter_permeability_deposit_samples=%zu\n",
            cert.passed ? "PASS" : "FAIL", cert.loadReason.c_str(),
            CausalWorldGeology::Hex64( cert.semanticDigest ).c_str(), cert.surfaceSamples,
            cert.depositSurfaceSamples, cert.disabledDepositSamples,
            cert.stricterDepositSamples );
        std::fprintf( file, "fault=0\nsurface_breach_proof=0\noccupancy=0\nd2=0\n"
            "active_erosion=0\nwater_coupling=0\nsediment=0\np5b=closed\n" );
        for ( auto const& check : cert.checks )
        { std::fprintf( file, "check.%s=%s\n", check.first.c_str(), check.second ? "PASS" : "FAIL" ); }
        std::fclose( file ); return true;
    }
}
