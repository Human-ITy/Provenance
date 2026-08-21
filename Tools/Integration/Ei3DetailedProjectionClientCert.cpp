#include "../../Code/Applications/ProvenanceClient/Ei3DetailedProjection.h"

#include <cstdio>
#include <fstream>
#include <iterator>

int main( int argc, char** argv )
{
    if ( argc != 2 ) { std::fprintf( stderr, "usage: cert fixture.json\n" ); return 2; }
    std::ifstream input( argv[1], std::ios::binary );
    std::string fixture( (std::istreambuf_iterator<char>( input )), {} );
    std::string baseResponse, delta, world, baseline;
    bool pass = input.good() || input.eof();
    pass = pass && Ei3::Detail::ExtractObject( fixture, "base_response", baseResponse );
    pass = pass && Ei3::Detail::ExtractObject( fixture, "delta", delta );
    pass = pass && Ei3::Detail::ExtractString( fixture, "world_uuid", world );
    pass = pass && Ei3::Detail::ExtractString( fixture, "world_baseline_digest", baseline );

    std::vector<std::string> snapshots;
    pass = pass && Ei3::ExtractSnapshots( baseResponse, snapshots ) && snapshots.size() == 1;
    Ei3::Snapshot parsed;
    std::string failure;
    pass = pass && Ei3::ParseSnapshot( snapshots[0], world, baseline, parsed, failure );
    pass = pass && parsed.columns.size() == Ei3::kLatticeCount;
    pass = pass && parsed.detailSurfaceHeightQ.size() == Ei3::kDetailLatticeCount;
    pass = pass && parsed.detailSurfaceSemantics.size() == Ei3::kDetailLatticeCount;
    pass = pass && !parsed.columns.empty()
        && !parsed.columns[0].landformClass.empty()
        && parsed.columns[0].weathering >= 0.f
        && parsed.columns[0].weathering <= 1.f
        && parsed.columns[0].soilDepthM >= 0.f
        && !parsed.columns[0].surfaceMaterialMix.empty();

    Ei3::Residency residency;
    auto admitted = residency.AdmitSnapshot( snapshots[0], world, baseline, failure );
    pass = pass && admitted == Ei3::Residency::Admission::Published;
    pass = pass && residency.AdmitSnapshot( snapshots[0], world, baseline, failure )
        == Ei3::Residency::Admission::Idempotent;
    float ground = 0.f;
    pass = pass && residency.GroundHeight( 160.f, 160.f, ground ) && std::isfinite( ground );
    Ei3::MatterSurfaceSample ancestry;
    pass = pass && residency.SampleMatterSurface( 160.f, 160.f, ancestry );
    pass = pass && !ancestry.macroFeatureId.empty()
        && !ancestry.macroLandformId.empty()
        && !ancestry.parentRangeId.empty()
        && !ancestry.macroPeakId.empty()
        && !ancestry.lithologyClass.empty()
        && !ancestry.surfaceMaterialMix.empty();
    int ancestryParts = 0;
    for ( auto const& part : ancestry.surfaceMaterialMix ) { ancestryParts += part.parts; }
    pass = pass && ancestryParts == 65535;
    pass = pass && residency.AdmitSnapshot( snapshots[0],
        "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa", baseline, failure )
        == Ei3::Residency::Admission::Rejected;

    Ei3::Snapshot result;
    pass = pass && Ei3::ApplyDeltaJson( parsed, delta, world, baseline, result, failure );
    pass = pass && result.chunkRevision == parsed.chunkRevision + 1;
    pass = pass && residency.AdmitDelta( parsed.coord, delta, world, baseline, failure )
        == Ei3::Residency::Admission::Published;
    pass = pass && residency.AdmitDelta( parsed.coord, delta, world, baseline, failure )
        == Ei3::Residency::Admission::Rejected;

    for ( float speed : { 24.f, 60.f, 120.f, 240.f } )
    {
        Ei3::PredictiveInput p;
        p.x = 160.f; p.y = 160.f; p.z = 120.f;
        p.velocityX = speed; p.cameraForwardX = 1.f; p.cameraForwardY = 0.f;
        auto const plan = Ei3::PlanResidency( p );
        int p1 = 0; int furthest = Ei3::FloorChunk( p.x );
        for ( auto const& d : plan ) if ( d.priority == Ei3::Priority::P1 )
        { ++p1; furthest = (std::max)( furthest, d.coord.x ); }
        pass = pass && p1 > 0 && furthest > Ei3::FloorChunk( p.x );
    }
    Ei3::PredictiveInput landing;
    landing.x = 160.f; landing.y = 160.f; landing.landingIntent = true;
    auto landingPlan = Ei3::PlanResidency( landing );
    int p0 = 0;
    for ( auto const& d : landingPlan ) { if ( d.priority == Ei3::Priority::P0 ) { ++p0; } }
    pass = pass && p0 == 9;
    Ei3::PredictiveInput turn = landing;
    turn.landingIntent = false; turn.velocityX = 120.f;
    turn.cameraForwardX = 0.f; turn.cameraForwardY = -1.f;
    auto turnPlan = Ei3::PlanResidency( turn );
    bool hasVelocity = false, hasCamera = false;
    for ( auto const& d : turnPlan )
    {
        hasVelocity |= d.priority == Ei3::Priority::P1 && d.coord.x > Ei3::FloorChunk( turn.x );
        hasCamera |= d.priority == Ei3::Priority::P2 && d.coord.y < Ei3::FloorChunk( turn.y );
    }
    pass = pass && hasVelocity && hasCamera;

    std::printf(
        "EI3_CLIENT_CONTRACT=%s\nSNAPSHOT_ADMISSION=%s\nDELTA_CONTIGUITY=%s\n"
        "STATIC_ANCESTRY=%s\nPREDICTIVE_24_60_120_240=%s\n"
        "MATTER_DETAIL_4M=%s\nSURFACE_CONTEXT_4M=%s\nSURFACE_MIX_4M=%s\n"
        "LANDING_P0=%s\nCOURSE_CHANGE=%s\n",
        pass ? "PASS" : "FAIL",
        pass ? "PASS" : "FAIL", pass ? "PASS" : "FAIL", pass ? "PASS" : "FAIL",
        pass ? "PASS" : "FAIL", pass ? "PASS" : "FAIL", pass ? "PASS" : "FAIL",
        pass ? "PASS" : "FAIL",
        pass ? "PASS" : "FAIL", pass ? "PASS" : "FAIL" );
    return pass ? 0 : 1;
}
