#include "../../Code/Applications/ProvenanceClient/Ei0aHandshake.h"

#include <iostream>

namespace
{
    int failures = 0;

    void Check( bool condition, char const* name )
    {
        std::cout << ( condition ? "PASS " : "FAIL " ) << name << "\n";
        if ( !condition ) { ++failures; }
    }

    std::string EngineHelloJson( char const* envelopeId = "17",
                                 char const* bodyId = "17",
                                 char const* protocolId = Ei0a::kProtocolId )
    {
        return std::string( "{\"id\":\"" ) + envelopeId
            + "\",\"ok\":true,\"result\":{\"request_id\":\"" + bodyId
            + "\",\"protocol_id\":\"" + protocolId
            + "\",\"protocol_semver\":\"" + Ei0a::kProtocolSemver
            + "\",\"schema_digest\":\"" + Ei0a::kSchemaDigest
            + "\",\"engine_build\":\"engine-test\""
              ",\"authority_mode\":\"local_server_authoritative\""
              ",\"world_uuid\":\"3f25975e-4a31-445c-939f-6a1a4f4f6722\""
              ",\"genesis_digest\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\""
              ",\"generator_family\":\"fablescript.test\""
              ",\"generator_version\":\"1\""
              ",\"material_registry_id\":\"materials.test\""
              ",\"material_registry_digest\":\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\""
              ",\"surface_grammar_id\":\"surface.test\""
              ",\"surface_grammar_version\":\"1\""
              ",\"water_grammar_id\":\"water.test\""
              ",\"water_grammar_version\":\"1\""
              ",\"session_token\":\"session-test\""
              ",\"lane_role\":\"control\""
              ",\"session_binding\":{\"server_instance_id\":\"10ad6c72-a2b1-4d68-a58d-d9b846eeff71\"}}}";
    }
}

int main()
{
    std::string const hello = Ei0a::BuildClientHelloParams( "17" );
    Check( hello.find( "\"protocol_id\":\"fablescript.embodied-terrain\"" )
        != std::string::npos, "canonical ClientHello protocol identity" );
    Check( hello.find( "\"requested_projection_modes\"" ) != std::string::npos,
        "canonical ClientHello projection capabilities" );

    Ei0a::RequestTracker tracker;
    Check( tracker.Register( "1", { 10, 4, 5 } ), "register request 1" );
    Check( tracker.Register( "2", { 20, 8, 9 } ), "register request 2" );
    Ei0a::PendingRequest request;
    std::string responseId;
    Check( tracker.Complete( "{\"id\":\"2\"}", request, responseId )
        == Ei0a::Correlation::Matched && request.kind == 20
        && request.bx == 8 && request.by == 9,
        "out-of-order response dispatches exact request" );
    Check( tracker.Complete( "{\"id\":\"999\"}", request, responseId )
        == Ei0a::Correlation::UnknownId,
        "unknown response id rejected" );
    Check( tracker.Complete( "{\"id\":\"1\"}", request, responseId )
        == Ei0a::Correlation::Matched && request.kind == 10,
        "pending request survives wrong response id" );
    Check( tracker.Complete( "{\"id\":\"1\"}", request, responseId )
        == Ei0a::Correlation::DuplicateCompletedId,
        "duplicate completed response rejected" );

    Ei0a::EngineHello parsed;
    std::string error;
    Check( Ei0a::ParseAndValidateEngineHello(
        EngineHelloJson(), "17", parsed, error ),
        "canonical EngineHello parser accepts matching authority" );
    Check( !Ei0a::ParseAndValidateEngineHello(
        EngineHelloJson( "wrong", "17" ), "17", parsed, error )
        && error == "request_id_mismatch",
        "wrong EngineHello envelope id rejected" );
    Check( !Ei0a::ParseAndValidateEngineHello(
        EngineHelloJson( "17", "17", "legacy.v1" ), "17", parsed, error )
        && error == "protocol_id_mismatch",
        "historical v1 is not implicitly accepted" );

    std::cout << ( failures ? "EI0.A CLIENT CONTRACT FAIL\n"
                            : "EI0.A CLIENT CONTRACT PASS\n" );
    return failures ? 1 : 0;
}
