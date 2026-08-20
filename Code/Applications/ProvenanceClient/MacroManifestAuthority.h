#pragma once

#include "MacroPageAuthority.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace MacroManifestAuthority
{
constexpr char const* kMagic="FABLESCRIPT_MACRO_MANIFEST_V1";
constexpr char const* kFamily="fablescript.worldgen.macro-manifest";
constexpr char const* kProducer="FABLESCRIPT_WORLDGENESIS";
constexpr char const* kContentKeyAlgorithm="sha256-page-v2-bytes-v1";

struct PageBinding
{
    int ri=0,rj=0;std::string pageDigest,sourceDigest,surfaceDigest,waterDigest;
    std::string cacheKey,cachePath;uint64_t payloadSize=0;
};

struct Manifest
{
    bool valid=false;std::string failure,genesisDigest,manifestDigest;
    std::map<std::pair<int,int>,PageBinding> pages;
};

namespace Detail
{
inline bool Int(std::string const& value,int& out)
{char* end=nullptr;long v=std::strtol(value.c_str(),&end,10);if(!end||*end)return false;out=(int)v;return true;}
inline bool U64(std::string const& value,uint64_t& out)
{char* end=nullptr;unsigned long long v=std::strtoull(value.c_str(),&end,10);if(!end||*end)return false;out=(uint64_t)v;return true;}
inline std::vector<std::string> Split(std::string const& value,char delimiter)
{std::vector<std::string> out;size_t begin=0;for(;;){size_t end=value.find(delimiter,begin);out.push_back(value.substr(begin,end-begin));if(end==std::string::npos)break;begin=end+1;}return out;}
inline bool Read(std::string const& path,std::string& bytes)
{std::ifstream in(path,std::ios::binary);if(!in)return false;std::ostringstream stream;stream<<in.rdbuf();bytes=stream.str();return true;}
inline Manifest Fail(std::string const& value){Manifest out;out.failure=value;return out;}
}

inline Manifest Load(std::string const& path,MacroPageAuthority::Context const& context)
{
    std::string text;if(!Detail::Read(path,text))return Detail::Fail("manifest_open_failed");
    std::istringstream input(text);std::string line;if(!std::getline(input,line))
        return Detail::Fail("manifest_family_mismatch");
    if(!line.empty()&&line.back()=='\r')line.pop_back();if(line!=kMagic)
        return Detail::Fail("manifest_family_mismatch");
    std::map<std::string,std::string> fields;std::vector<std::string> pageLines;
    Manifest result;
    while(std::getline(input,line))
    {
        if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;
        size_t eq=line.find('=');if(eq==std::string::npos||eq==0)return Detail::Fail("manifest_malformed_record");
        std::string key=line.substr(0,eq),value=line.substr(eq+1);
        if(key=="page")
        {
            auto parts=Detail::Split(value,',');if(parts.size()!=9)return Detail::Fail("manifest_malformed_page");
            PageBinding b;if(!Detail::Int(parts[0],b.ri)||!Detail::Int(parts[1],b.rj)||!Detail::U64(parts[8],b.payloadSize))
                return Detail::Fail("manifest_malformed_page");
            b.pageDigest=parts[2];b.sourceDigest=parts[3];b.surfaceDigest=parts[4];b.waterDigest=parts[5];
            b.cacheKey=parts[6];b.cachePath=parts[7];
            if(b.cacheKey.rfind("sha256:",0)!=0||b.cachePath.empty()||b.cachePath.find("..")!=std::string::npos)
                return Detail::Fail("manifest_malformed_cache_binding");
            if(!result.pages.emplace(std::make_pair(b.ri,b.rj),b).second)
                return Detail::Fail("manifest_duplicate_page");
            pageLines.push_back(line);continue;
        }
        if(!fields.emplace(key,value).second)return Detail::Fail("manifest_duplicate_field");
    }
    static char const* required[]={"content_key_algorithm","descriptor_schema_digest","generator_build_digest",
        "generator_config_digest","generator_family","generator_version","genesis_digest","macro_page_schema_version",
        "manifest_digest","manifest_family","manifest_schema_version","material_registry_digest","material_registry_id",
        "producer_authority","surface_state_encoding_version","water_state_encoding_version"};
    if(fields.size()!=sizeof(required)/sizeof(required[0]))return Detail::Fail("manifest_fields_mismatch");
    for(char const* name:required)if(!fields.count(name))return Detail::Fail("manifest_fields_mismatch");
    if(fields["manifest_schema_version"]!="1"||fields["manifest_family"]!=kFamily||fields["producer_authority"]!=kProducer||
       fields["content_key_algorithm"]!=kContentKeyAlgorithm)return Detail::Fail("manifest_contract_mismatch");
    if(fields["genesis_digest"]!=context.genesisDigest||fields["generator_family"]!=context.generatorFamily||
       fields["generator_version"]!=context.generatorVersion||fields["generator_build_digest"]!=MacroPageAuthority::kGeneratorBuildDigest||
       fields["generator_config_digest"]!=MacroPageAuthority::kGeneratorConfigDigest||
       fields["material_registry_id"]!=context.materialRegistryId||fields["material_registry_digest"]!=context.materialRegistryDigest)
        return Detail::Fail("manifest_identity_mismatch");
    if(fields["macro_page_schema_version"]!="2"||fields["surface_state_encoding_version"]!=std::to_string(Ms1::kSurfaceEncodingVersion)||
       fields["water_state_encoding_version"]!=std::to_string(Ms1::kWaterEncodingVersion)||
       fields["descriptor_schema_digest"]!=context.descriptorSchemaDigest)return Detail::Fail("manifest_schema_mismatch");
    std::string canonical=std::string(kMagic)+"\n";
    for(auto const& field:fields)if(field.first!="manifest_digest")canonical+=field.first+"="+field.second+"\n";
    std::sort(pageLines.begin(),pageLines.end(),[](std::string const& a,std::string const& b){
        auto pa=Detail::Split(a.substr(5),','),pb=Detail::Split(b.substr(5),',');int ari=0,arj=0,bri=0,brj=0;
        Detail::Int(pa[0],ari);Detail::Int(pa[1],arj);Detail::Int(pb[0],bri);Detail::Int(pb[1],brj);
        return ari==bri?arj<brj:ari<bri;});
    for(auto const& page:pageLines)canonical+=page+"\n";
    if(MacroPageAuthority::Detail::Sha256(canonical)!=fields["manifest_digest"])
        return Detail::Fail("manifest_digest_mismatch");
    result.valid=true;result.genesisDigest=fields["genesis_digest"];result.manifestDigest=fields["manifest_digest"];
    return result;
}

inline bool ValidatePageBinding(Manifest const& manifest,PageBinding const& binding,
                                std::string const& path,MacroPageAuthority::Page const& page,
                                std::string& failure)
{
    if(!manifest.valid||!page.Authoritative()){failure="non_authoritative_input";return false;}
    std::string bytes;if(!Detail::Read(path,bytes)){failure="page_open_failed";return false;}
    if(bytes.size()!=binding.payloadSize){failure="page_payload_size_mismatch";return false;}
    if("sha256:"+MacroPageAuthority::Detail::Sha256(bytes)!=binding.cacheKey){failure="page_cache_key_mismatch";return false;}
    if(page.pageDigest!=binding.pageDigest||MacroPageAuthority::Detail::Hex64(page.sourceDigest)!=binding.sourceDigest||
       MacroPageAuthority::Detail::Hex64(page.surfaceDigest)!=binding.surfaceDigest||
       MacroPageAuthority::Detail::Hex64(page.waterDigest)!=binding.waterDigest)
    {failure="page_manifest_digest_mismatch";return false;}
    failure.clear();return true;
}
}
