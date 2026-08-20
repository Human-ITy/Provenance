#include "Ei0aHandshake.h"
#include "MacroPageAuthority.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
std::string Slurp(std::string const& path){std::ifstream in(path,std::ios::binary);std::ostringstream out;out<<in.rdbuf();return out.str();}
std::string PagePath(std::string const& root,int ri,int rj)
{return root+"/Data/Worldgen/MacroAuthority/page_"+std::to_string(ri)+"_"+std::to_string(rj)+".mcp";}
std::string EngineHello()
{
    return std::string("{\"id\":\"hello-1\",\"request_id\":\"hello-1\",\"protocol_id\":\"")+Ei0a::kProtocolId+
      "\",\"protocol_semver\":\""+Ei0a::kProtocolSemver+"\",\"schema_digest\":\""+Ei0a::kSchemaDigest+
      "\",\"descriptor_schema_digest\":\""+Ei0a::kDescriptorSchemaDigest+
      "\",\"engine_build\":\"ei0c-cert\",\"authority_mode\":\""+Ei0a::kAuthorityMode+
      "\",\"world_uuid\":\"11111111-1111-4111-8111-111111111111\",\"macro_genesis_digest\":\""+MacroPageAuthority::kGenesisDigest+
      "\",\"world_baseline_digest\":\""+std::string(64,'c')+
      "\",\"baseline_components\":[{\"role\":\"substrate\",\"semantic_identity_id\":\"substrate.test\",\"semantic_identity_version\":\"1\",\"semantic_digest\":\""+std::string(64,'d')+"\"}]"
      "\",\"generator_family\":\""+MacroPageAuthority::kGeneratorFamily+"\",\"generator_version\":\""+MacroPageAuthority::kGeneratorVersion+
      "\",\"material_registry_id\":\""+MacroPageAuthority::kMaterialRegistryId+"\",\"material_registry_digest\":\""+MacroPageAuthority::kMaterialRegistryDigest+
      "\",\"surface_grammar_id\":\""+Ms1::kSurfaceGrammarId+"\",\"surface_grammar_version\":\""+Ms1::kSurfaceGrammarVersion+
      "\",\"water_grammar_id\":\""+Ms1::kWaterGrammarId+"\",\"water_grammar_version\":\""+Ms1::kWaterGrammarVersion+
      "\",\"session_token\":\"session-ei0c\",\"lane_role\":\"control\",\"server_instance_id\":\"22222222-2222-4222-8222-222222222222\"}";
}
std::string SupersededIdentityPage(std::string const& text,bool toolchain)
{
    std::string magic;std::map<std::string,std::string> values;if(!MacroPageAuthority::Detail::Parse(text,magic,values))return {};
    values["generator_build_digest"]=toolchain?MacroPageAuthority::kToolchainSupersededGeneratorBuildDigest:MacroPageAuthority::kLegacyGeneratorBuildDigest;
    values["generator_config_digest"]=toolchain?MacroPageAuthority::kGeneratorConfigDigest:MacroPageAuthority::kLegacyGeneratorConfigDigest;
    values["material_registry_digest"]=toolchain?MacroPageAuthority::kMaterialRegistryDigest:MacroPageAuthority::kLegacyMaterialRegistryDigest;
    values["genesis_digest"]=toolchain?MacroPageAuthority::kToolchainSupersededGenesisDigest:MacroPageAuthority::kLegacyGenesisDigest;
    values["page_digest"]=MacroPageAuthority::Detail::Sha256(MacroPageAuthority::Detail::CanonicalPage(magic,values));
    std::string out=magic+"\n";for(auto const& value:values)out+=value.first+"="+value.second+"\n";return out;
}
}

int main(int argc,char** argv)
{
    if(argc!=2){std::cerr<<"usage: Ei0cMacroPageValidationCert client-root\n";return 2;}
    Ei0a::EngineHello hello;std::string error;if(!Ei0a::ParseAndValidateEngineHello(EngineHello(),"hello-1",hello,error)){std::cerr<<"hello: "<<error<<"\n";return 3;}
    MacroPageAuthority::Context context;context.genesisDigest=hello.macroGenesisDigest;context.generatorFamily=hello.generatorFamily;context.generatorVersion=hello.generatorVersion;context.materialRegistryId=hello.materialRegistryId;context.materialRegistryDigest=hello.materialRegistryDigest;context.surfaceGrammarId=hello.surfaceGrammarId;context.surfaceGrammarVersion=hello.surfaceGrammarVersion;context.waterGrammarId=hello.waterGrammarId;context.waterGrammarVersion=hello.waterGrammarVersion;context.descriptorSchemaDigest=hello.descriptorSchemaDigest;
    auto const start=std::chrono::steady_clock::now();size_t heights=0,surface=0,water=0;
    for(int rj=-2;rj<=2;++rj)for(int ri=-2;ri<=2;++ri){auto page=MacroPageAuthority::Load(PagePath(argv[1],ri,rj),ri,rj,context);if(!page.Authoritative()){std::cerr<<ri<<","<<rj<<": "<<MacroPageAuthority::FailureName(page.failure)<<" "<<page.detail<<"\n";return 4;}heights+=page.heights.size();surface+=page.surfaceCodes.size();water+=page.waterCodes.size();}
    auto valid=MacroPageAuthority::Load(PagePath(argv[1],0,0),0,0,context);auto wrongContext=context;wrongContext.genesisDigest=std::string(64,'f');auto corrupt=MacroPageAuthority::Load(PagePath(argv[1],0,0),0,0,wrongContext);if(corrupt.failure!=MacroPageAuthority::Failure::WrongGenesis)return 5;
    std::string legacy=Slurp(PagePath(argv[1],0,0));legacy.replace(0,std::string(MacroPageAuthority::kMagic).size(),MacroPageAuthority::kLegacyMagic);auto legacyAuthority=MacroPageAuthority::ValidateText(legacy,0,0,context);context.diagnosticLegacy=true;auto legacyDiagnostic=MacroPageAuthority::ValidateText(legacy,0,0,context);if(legacyAuthority.trust!=MacroPageAuthority::Trust::Quarantined||legacyDiagnostic.trust!=MacroPageAuthority::Trust::DiagnosticOnly||legacyDiagnostic.Authoritative())return 6;
    for(bool toolchain:{false,true}){context.diagnosticLegacy=false;std::string superseded=SupersededIdentityPage(Slurp(PagePath(argv[1],0,0)),toolchain);auto oldAuthority=MacroPageAuthority::ValidateText(superseded,0,0,context);context.diagnosticLegacy=true;auto oldDiagnostic=MacroPageAuthority::ValidateText(superseded,0,0,context);if(oldAuthority.trust!=MacroPageAuthority::Trust::Quarantined||oldDiagnostic.trust!=MacroPageAuthority::Trust::DiagnosticOnly||oldAuthority.failure!=MacroPageAuthority::Failure::WrongGenesis||oldDiagnostic.failure!=MacroPageAuthority::Failure::WrongGenesis||oldDiagnostic.Authoritative())return 9;}
    MacroPageAuthority::TrustedSlot slot;if(!slot.Publish(valid)||slot.Publish(corrupt)||!slot.hasPublished||slot.published.pageDigest!=valid.pageDigest)return 7;MacroPageAuthority::TrustedSlot empty;if(empty.Publish(corrupt)||!empty.Publish(valid)||!empty.published.Authoritative())return 8;
    double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"EI0.C C++ PAGE VALIDATION CERT: PASS\n"<<"pages=25 heights="<<heights<<" surface="<<surface<<" water="<<water<<"\n"<<"macro_genesis_digest="<<hello.macroGenesisDigest<<"\n"<<"validation_25_pages_ms="<<ms<<" per_page_ms="<<(ms/25.0)<<"\n";
    return 0;
}
