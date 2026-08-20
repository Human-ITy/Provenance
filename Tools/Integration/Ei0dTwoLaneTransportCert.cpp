#include "Ei0dTwoLaneTransport.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
std::string Hello(char const* id,char const* lane,char const* token,
                  char const* world="11111111-1111-4111-8111-111111111111")
{
    return std::string("{\"id\":\"")+id+"\",\"request_id\":\""+id+
      "\",\"protocol_id\":\""+Ei0a::kProtocolId+"\",\"protocol_semver\":\""+
      Ei0a::kProtocolSemver+"\",\"schema_digest\":\""+Ei0a::kSchemaDigest+
      "\",\"descriptor_schema_digest\":\""+Ei0a::kDescriptorSchemaDigest+
      "\",\"engine_build\":\"ei0d-cert\",\"authority_mode\":\""+Ei0a::kAuthorityMode+
      "\",\"world_uuid\":\""+world+"\",\"macro_genesis_digest\":\""+std::string(64,'a')+
      "\",\"world_baseline_digest\":\""+std::string(64,'c')+
      "\",\"baseline_components\":[{\"role\":\"substrate\",\"semantic_identity_id\":\"substrate.test\",\"semantic_identity_version\":\"1\",\"semantic_digest\":\""+std::string(64,'d')+"\"}]"
      "\",\"generator_family\":\"provmapfps\",\"generator_version\":\"5\""
      " ,\"material_registry_id\":\"provenance-material-registry\",\"material_registry_digest\":\""+
      std::string(64,'b')+"\",\"surface_grammar_id\":\"ms1.surface-state\",\"surface_grammar_version\":\"1\""
      " ,\"water_grammar_id\":\"wd1.water-state\",\"water_grammar_version\":\"1\""
      " ,\"session_token\":\""+token+"\",\"lane_role\":\""+lane+
      "\",\"server_instance_id\":\"22222222-2222-4222-8222-222222222222\""
      " ,\"transport_profile\":{\"profile_id\":\""+Ei0d::kTransportProfileId+
      "\",\"framing\":\""+Ei0d::kFraming+"\"}}";
}

int Fail(char const* message){std::cerr<<"EI0.D CERT FAIL: "<<message<<"\n";return 1;}
}

int main()
{
    std::string error;Ei0a::EngineHello control,bulk;
    if(!Ei0a::ParseAndValidateEngineHello(Hello("c-1","control","session"),"c-1",control,error))return Fail("control hello");
    if(!Ei0a::ParseAndValidateEngineHello(Hello("b-1","bulk","session"),"b-1",bulk,error))return Fail("bulk hello");
    Ei0d::SessionBinding const binding=Ei0d::BindingFromHello(control);
    if(!Ei0d::ValidateBulkBinding(binding,bulk,error))return Fail("same-session binding");
    Ei0a::EngineHello wrongBaseline=bulk;wrongBaseline.worldBaselineDigest=std::string(64,'e');
    if(Ei0d::ValidateBulkBinding(binding,wrongBaseline,error)||error!="session_mismatch")return Fail("wrong baseline admitted");
    Ei0a::EngineHello wrong=bulk;wrong.worldUuid="33333333-3333-4333-8333-333333333333";
    if(Ei0d::ValidateBulkBinding(binding,wrong,error)||error!="session_mismatch")return Fail("wrong world admitted");
    wrong=bulk;wrong.transportProfileId="legacy-v1";
    if(Ei0d::ValidateBulkBinding(binding,wrong,error)||error!="transport_profile_mismatch")return Fail("wrong profile admitted");

    Ei0d::JsonLineAccumulator framer(64);std::vector<std::string> frames;
    if(!framer.Feed("{\"id\":1",7,frames,error)||!frames.empty())return Fail("fragment A");
    std::string const tail="}\n{\"id\":2}\n";
    if(!framer.Feed(tail.data(),tail.size(),frames,error)||frames.size()!=2)return Fail("fragment/multiple frames");
    Ei0d::JsonLineAccumulator oversized(4);frames.clear();
    if(oversized.Feed("12345",5,frames,error)||error!="frame_too_large")return Fail("oversized frame");
    Ei0d::JsonLineAccumulator trailing;frames.clear();
    trailing.Feed("{\"id\":1}",8,frames,error);
    if(trailing.Finish(error)||error!="truncated_frame")return Fail("trailing partial frame");

    Ei0a::RequestTracker controlRequests,bulkRequests;Ei0a::PendingRequest pending;
    if(!controlRequests.Register("same",{1,0,0})||!bulkRequests.Register("same",{2,0,0}))return Fail("independent ids");
    std::string responseId;
    if(bulkRequests.Complete("{\"id\":\"same\"}",pending,responseId)!=Ei0a::Correlation::Matched||pending.kind!=2)return Fail("bulk correlation");
    controlRequests.Register("slow",{3,0,0});controlRequests.Register("fast",{4,0,0});
    if(controlRequests.Complete("{\"id\":\"fast\"}",pending,responseId)!=Ei0a::Correlation::Matched||pending.kind!=4)return Fail("out of order fast");
    if(controlRequests.Complete("{\"id\":\"slow\"}",pending,responseId)!=Ei0a::Correlation::Matched||pending.kind!=3)return Fail("out of order slow");

    Ei0d::LaneFlow flow(2,16);
    if(!flow.ReserveRequest(8,error)||!flow.ReserveRequest(8,error))return Fail("flow reserve");
    if(flow.ReserveRequest(1,error)||error!="too_many_inflight_requests")return Fail("inflight cap");
    if(flow.HighWaterBytes()!=16)return Fail("queue high-water");
    flow.Sent(8);flow.Complete();
    if(flow.ReserveRequest(9,error)||error!="outbound_backpressure")return Fail("byte backpressure");

    Ei0d::AtomicPublicationSlot<std::string> publication;
    auto candidate=std::make_shared<std::string const>("validated-offside");
    if(publication.Load())return Fail("premature publication");
    publication.Publish(candidate);
    if(!publication.Load()||*publication.Load()!="validated-offside"||publication.Generation()!=1)return Fail("atomic publication");

    std::cout<<"EI0.D C++ TWO-LANE CLIENT CERT: PASS\n"
             <<"profile="<<Ei0d::kTransportProfileId<<" framing="<<Ei0d::kFraming<<"\n"
             <<"max_bulk_inflight="<<Ei0d::kMaxBulkInflight
             <<" max_bulk_cells="<<Ei0d::kMaxBulkCells
             <<" max_bulk_response_bytes="<<Ei0d::kMaxBulkResponseBytes<<"\n";
    return 0;
}

