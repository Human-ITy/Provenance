#include "../../Code/Applications/ProvenanceClient/Ei3DetailedProjection.h"

#include <cstdio>
#include <fstream>
#include <iterator>

namespace
{
char const* Receipt( bool const value ) { return value ? "PASS" : "FAIL"; }
}

int main( int argc, char** argv )
{
    if ( argc != 2 ) { std::fprintf( stderr, "usage: cert fixture.json\n" ); return 2; }
    std::ifstream input( argv[1], std::ios::binary );
    std::string fixture( (std::istreambuf_iterator<char>( input )), {} );
    std::string baseResponse, delta, world, baseline;
    bool richLandforms = false;
    bool const fixtureContract = ( input.good() || input.eof() )
        && Ei3::Detail::ExtractObject( fixture, "base_response", baseResponse )
        && Ei3::Detail::ExtractObject( fixture, "delta", delta )
        && Ei3::Detail::ExtractString( fixture, "world_uuid", world )
        && Ei3::Detail::ExtractString( fixture, "world_baseline_digest", baseline );
    bool const fixtureDeclaresRich = Ei3::Detail::ExtractBool(
        fixture, "rich_landforms", richLandforms );

    std::vector<std::string> snapshots;
    bool const snapshotExtracted = fixtureContract
        && Ei3::ExtractSnapshots( baseResponse, snapshots ) && snapshots.size() == 1;
    Ei3::Snapshot parsed;
    std::string failure;
    bool const snapshotParsed = snapshotExtracted
        && Ei3::ParseSnapshot( snapshots[0], world, baseline, parsed, failure );
    bool const baseLattice = snapshotParsed && parsed.columns.size() == Ei3::kLatticeCount;
    bool const matterDetail = snapshotParsed
        && parsed.detailSurfaceHeightQ.size() == Ei3::kDetailLatticeCount;
    bool const surfaceContext = snapshotParsed
        && parsed.detailSurfaceSemantics.size() == Ei3::kDetailLatticeCount;
    bool const richDetail = !fixtureDeclaresRich || !richLandforms || (
        parsed.detailRefinedSurfaceHeightQ.size() == Ei3::kDetailLatticeCount
        && parsed.detailLandformStructures.size() == Ei3::kDetailLatticeCount
        && parsed.detailStructuralComplexity.size() == Ei3::kDetailLatticeCount );
    bool const columnContext = baseLattice && !parsed.columns.empty()
        && !parsed.columns[0].landformClass.empty()
        && parsed.columns[0].weathering >= 0.f
        && parsed.columns[0].weathering <= 1.f
        && parsed.columns[0].soilDepthM >= 0.f;
    bool const columnMix = columnContext && !parsed.columns[0].surfaceMaterialMix.empty();

    Ei3::Residency residency;
    bool snapshotAdmission = false, staticAncestry = false, surfaceMix = false;
    Ei3::MatterSurfaceSample ancestry;
    if ( snapshotExtracted )
    {
        auto const admitted = residency.AdmitSnapshot( snapshots[0], world, baseline, failure );
        float ground = 0.f;
        snapshotAdmission = admitted == Ei3::Residency::Admission::Published
            && residency.AdmitSnapshot( snapshots[0], world, baseline, failure )
                == Ei3::Residency::Admission::Idempotent
            && residency.GroundHeight( 160.f, 160.f, ground ) && std::isfinite( ground )
            && residency.AdmitSnapshot( snapshots[0],
                "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa", baseline, failure )
                == Ei3::Residency::Admission::Rejected;
        staticAncestry = residency.SampleMatterSurface( 160.f, 160.f, ancestry )
            && !ancestry.macroFeatureId.empty() && !ancestry.macroLandformId.empty()
            && !ancestry.parentRangeId.empty() && !ancestry.macroPeakId.empty()
            && !ancestry.lithologyClass.empty() && !ancestry.surfaceMaterialMix.empty();
        int ancestryParts = 0;
        for ( auto const& part : ancestry.surfaceMaterialMix ) { ancestryParts += part.parts; }
        surfaceMix = staticAncestry && ancestryParts == 65535;
    }
    bool const richAuthority = !fixtureDeclaresRich || !richLandforms || (
        staticAncestry && ancestry.richLandform && !ancestry.landformElementId.empty()
        && !ancestry.structureClass.empty() && !ancestry.supportStratumId.empty()
        && !ancestry.supportMaterialId.empty() && ancestry.structuralComplexity >= 0.f
        && ancestry.structuralComplexity <= 1.f );

    Ei3::Snapshot result;
    bool const deltaApplied = snapshotParsed
        && Ei3::ApplyDeltaJson( parsed, delta, world, baseline, result, failure )
        && result.chunkRevision == parsed.chunkRevision + 1;
    bool const deltaContiguity = deltaApplied && snapshotExtracted
        && residency.AdmitDelta( parsed.coord, delta, world, baseline, failure )
            == Ei3::Residency::Admission::Published
        && residency.AdmitDelta( parsed.coord, delta, world, baseline, failure )
            == Ei3::Residency::Admission::Rejected;

    bool predictiveCoverage = true;
    for ( float speed : { 24.f, 60.f, 120.f, 240.f } )
    {
        Ei3::PredictiveInput p;
        p.x = 160.f; p.y = 160.f; p.z = 120.f;
        p.velocityX = speed; p.cameraForwardX = 1.f; p.cameraForwardY = 0.f;
        auto const plan = Ei3::PlanResidency( p );
        int predictive = 0, furthest = Ei3::FloorChunk( p.x );
        for ( auto const& d : plan ) if ( d.priority == Ei3::Priority::P3 )
        { ++predictive; furthest = (std::max)( furthest, d.coord.x ); }
        predictiveCoverage &= predictive > 0 && furthest > Ei3::FloorChunk( p.x );
    }

    Ei3::PredictiveInput landing;
    landing.x = 160.f; landing.y = 160.f; landing.landingIntent = true;
    auto const landingPlan = Ei3::PlanResidency( landing );
    int p0 = 0, radial2 = 0, radial3 = 0, lastRing = -1;
    bool concentricFirst = true;
    for ( auto const& d : landingPlan )
    {
        if ( d.priority == Ei3::Priority::P0 ) { ++p0; }
        if ( d.radialRing == 2 ) { ++radial2; }
        if ( d.radialRing == 3 ) { ++radial3; }
        if ( d.radialRing >= 0 )
        {
            concentricFirst &= d.radialRing >= lastRing;
            lastRing = d.radialRing;
        }
        else { concentricFirst &= lastRing == 3; }
    }
    bool const landingP0 = p0 == 9;
    bool const isotropicRings = radial2 == 16 && radial3 == 24 && concentricFirst;

    Ei3::PredictiveInput turn = landing;
    turn.landingIntent = false; turn.velocityX = 120.f;
    turn.cameraForwardX = 0.f; turn.cameraForwardY = -1.f;
    auto const turnPlan = Ei3::PlanResidency( turn );
    bool hasVelocity = false, hasCamera = false;
    for ( auto const& d : turnPlan )
    {
        hasVelocity |= d.priority == Ei3::Priority::P3
            && d.coord.x > Ei3::FloorChunk( turn.x );
        hasCamera |= d.priority == Ei3::Priority::P4
            && d.coord.y < Ei3::FloorChunk( turn.y );
    }
    bool const courseChange = hasVelocity && hasCamera;

    bool const clientContract = fixtureContract && snapshotParsed && baseLattice
        && snapshotAdmission && deltaContiguity && staticAncestry && matterDetail
        && surfaceContext && columnContext && columnMix && surfaceMix
        && richDetail && richAuthority;
    bool const pass = clientContract && predictiveCoverage && landingP0
        && isotropicRings && courseChange;

    std::printf(
        "EI3_CLIENT_CONTRACT=%s\nSNAPSHOT_ADMISSION=%s\nDELTA_CONTIGUITY=%s\n"
        "STATIC_ANCESTRY=%s\nPREDICTIVE_24_60_120_240=%s\n"
        "MATTER_DETAIL_4M=%s\nSURFACE_CONTEXT_4M=%s\nSURFACE_MIX_4M=%s\n"
        "RICH_LANDFORM_AUTHORITY=%s\nLANDING_P0=%s\n"
        "ISOTROPIC_LOCAL_RINGS=%s\nCOURSE_CHANGE=%s\n",
        Receipt( clientContract ), Receipt( snapshotAdmission ), Receipt( deltaContiguity ),
        Receipt( staticAncestry ), Receipt( predictiveCoverage ), Receipt( matterDetail ),
        Receipt( surfaceContext && columnContext ), Receipt( surfaceMix && columnMix ),
        Receipt( richDetail && richAuthority ), Receipt( landingP0 ),
        Receipt( isotropicRings ), Receipt( courseChange ) );
    return pass ? 0 : 1;
}
