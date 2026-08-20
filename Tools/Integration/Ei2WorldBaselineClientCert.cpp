#include "../../Code/Applications/ProvenanceClient/Ei0dTwoLaneTransport.h"

#include <iostream>
#include <string>

namespace
{
int failures = 0;
void Check(bool value,char const* label)
{
    std::cout<<(value?"PASS ":"FAIL ")<<label<<"\n";
    if(!value)++failures;
}

std::string Hello(char const* id,char const* lane)
{
    return std::string("{\"id\":\"")+id+"\",\"request_id\":\""+id+
      "\",\"protocol_id\":\""+Ei0a::kProtocolId+"\",\"protocol_semver\":\""+
      Ei0a::kProtocolSemver+"\",\"schema_digest\":\""+Ei0a::kSchemaDigest+
      "\",\"descriptor_schema_digest\":\""+Ei0a::kDescriptorSchemaDigest+
      "\",\"engine_build\":\"engine-ei2\",\"authority_mode\":\""+Ei0a::kAuthorityMode+
      "\",\"world_uuid\":\"11111111-1111-4111-8111-111111111111\""
      ",\"macro_genesis_digest\":\""+std::string(64,'a')+
      "\",\"world_baseline_digest\":\""+std::string(64,'b')+
      "\",\"baseline_components\":[{\"role\":\"substrate\",\"semantic_identity_id\":\"fablescript.substrate\",\"semantic_identity_version\":\"ei2.1\",\"semantic_digest\":\""+std::string(64,'c')+"\"}]"
      ",\"generator_family\":\"provmapfps\",\"generator_version\":\"5\""
      ",\"material_registry_id\":\"provenance-material-registry\",\"material_registry_digest\":\""+std::string(64,'d')+
      "\",\"surface_grammar_id\":\"ms1.surface-state\",\"surface_grammar_version\":\"1\""
      ",\"water_grammar_id\":\"wd1.water-state\",\"water_grammar_version\":\"1\""
      ",\"session_token\":\"same-token\",\"lane_role\":\""+lane+
      "\",\"session_binding\":{\"server_instance_id\":\"22222222-2222-4222-8222-222222222222\"}"
      ",\"transport_profile\":{\"profile_id\":\""+Ei0d::kTransportProfileId+
      "\",\"framing\":\""+Ei0d::kFraming+"\"}}";
}
}

int main()
{
    std::string const macro(64,'a'),baseline(64,'b');
    std::string const bulkHello=Ei0a::BuildClientHelloParams(
        "bulk-1","bulk","same-token","22222222-2222-4222-8222-222222222222",
        "11111111-1111-4111-8111-111111111111",macro,baseline);
    Check(bulkHello.find("\"macro_genesis_digest\":\""+macro+"\"")!=std::string::npos,
          "bulk claim carries macro identity");
    Check(bulkHello.find("\"world_baseline_digest\":\""+baseline+"\"")!=std::string::npos,
          "bulk claim carries physical baseline identity");
    Check(bulkHello.find("\"genesis_digest\"")==std::string::npos,
          "canonical handshake has no ambiguous genesis field");

    std::string error;Ei0a::EngineHello control,bulk;
    Check(Ei0a::ParseAndValidateEngineHello(Hello("control-1","control"),"control-1",control,error),
          "layered EngineHello accepted");
    Check(control.macroGenesisDigest==macro&&control.worldBaselineDigest==baseline
          &&control.baselineComponents.size()==1
          &&control.baselineComponents[0].role=="substrate",
          "all layered identities preserved");
    Check(Ei0a::ParseAndValidateEngineHello(Hello("bulk-1","bulk"),"bulk-1",bulk,error),
          "bulk EngineHello accepted");
    auto binding=Ei0d::BindingFromHello(control);
    Check(Ei0d::ValidateBulkBinding(binding,bulk,error),"two lanes prove same baseline");
    bulk.worldBaselineDigest=std::string(64,'e');
    Check(!Ei0d::ValidateBulkBinding(binding,bulk,error)&&error=="session_mismatch",
          "different baseline cannot attach");

    std::string legacy=Hello("old","control");
    std::string const marker="\"world_baseline_digest\":\""+baseline+"\",";
    legacy.erase(legacy.find(marker),marker.size());
    Ei0a::EngineHello old;
    Check(!Ei0a::ParseAndValidateEngineHello(legacy,"old",old,error)
          &&error=="malformed_identity","old single-layer hello fails closed");

    std::cout<<(failures?"EI2 CLIENT IDENTITY CERT: FAIL\n":"EI2 CLIENT IDENTITY CERT: PASS\n");
    return failures?1:0;
}
