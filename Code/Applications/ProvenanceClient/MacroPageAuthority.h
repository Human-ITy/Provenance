#pragma once

#include "WorldDescriptors.generated.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace MacroPageAuthority
{
constexpr char const* kMagic="PROVENANCE_MACRO_AUTHORITY_PAGE_V2";
constexpr char const* kLegacyMagic="PROVENANCE_MACRO_AUTHORITY_PAGE_V1";
constexpr int kPageSchemaVersion=2;
constexpr char const* kGenesisDigest="8394bfefb6955cfffec1c927721d2e6da1b4a24c5525dce4cd238640c2ecd801";
constexpr char const* kGeneratorFamily="provenance.macro-authority";
constexpr char const* kGeneratorVersion="5";
constexpr char const* kGeneratorBuildDigest="5d7c1636de50e62dabbae43233e7fc4abdb6d16bc06e79874a92f0010f697d08";
constexpr char const* kGeneratorConfigDigest="bd88947d297f3c231f94e5213ec7da67d03853a6882f56bcfb184f33b285ab1a";
constexpr char const* kMaterialRegistryId="provenance.material-registry.ms1a-v1";
constexpr char const* kMaterialRegistryDigest="f095c59862b3856a3c71761ea0e8d9b56e24a08f4530a37e589713ba8ed3eaaf";
constexpr char const* kLegacyGenesisDigest="17405cbecb97d55aee9e85408e8735c7f055f6ad87369dfa94bee4762c67e994";
constexpr char const* kLegacyGeneratorBuildDigest="afa225c1bada4642a3ba6dd79948870e396293a5dd8c5595b1e8c10257a8b72a";
constexpr char const* kLegacyGeneratorConfigDigest="f5ad774512951d272fe507700dd4702ac3094f253c75596936a92e1d852cf8a0";
constexpr char const* kLegacyMaterialRegistryDigest="e33bf4530caf681caed2d8a0bebe5cd886fd38d4954a480fa4d458149f825c76";
constexpr char const* kRegionKey="causal_world_macro_provinces_floor";
constexpr char const* kPageDigestAlgorithm="sha256-canonical-records-v1";
constexpr char const* kSourceDigestAlgorithm="fnv1a64-height-centimeters-le6-v1";
constexpr char const* kSurfaceDigestAlgorithm="fnv1a64-packed-le5-v1";
constexpr char const* kWaterDigestAlgorithm="fnv1a64-packed-le8-v1";
constexpr uint64_t kFnvOffset=14695981039346656037ull,kFnvPrime=1099511628211ull;

enum class Trust:uint8_t { Unloaded,Validating,ValidAuthority,Quarantined,DiagnosticOnly };
enum class Failure:uint8_t {
    None,IoError,LegacyPage,MalformedHeader,UnsupportedPageSchema,WrongGenesis,
    WrongGenerator,WrongMaterialRegistry,WrongSchemaDigest,WrongPageCoord,InvalidRange,
    TruncatedPayload,UnsupportedSurfaceEncoding,UnsupportedWaterEncoding,
    SourceDigestMismatch,SurfaceDigestMismatch,WaterDigestMismatch,PageDigestMismatch,
    InvalidEnum,LineageMismatch
};

inline char const* FailureName(Failure value)
{
    switch(value){
    case Failure::None:return "NONE";case Failure::IoError:return "IO_ERROR";
    case Failure::LegacyPage:return "LEGACY_PAGE";case Failure::MalformedHeader:return "MALFORMED_HEADER";
    case Failure::UnsupportedPageSchema:return "UNSUPPORTED_PAGE_SCHEMA";case Failure::WrongGenesis:return "WRONG_GENESIS";
    case Failure::WrongGenerator:return "WRONG_GENERATOR";case Failure::WrongMaterialRegistry:return "WRONG_MATERIAL_REGISTRY";
    case Failure::WrongSchemaDigest:return "WRONG_SCHEMA_DIGEST";case Failure::WrongPageCoord:return "WRONG_PAGE_COORD";
    case Failure::InvalidRange:return "INVALID_RANGE";case Failure::TruncatedPayload:return "TRUNCATED_PAYLOAD";
    case Failure::UnsupportedSurfaceEncoding:return "UNSUPPORTED_SURFACE_ENCODING";
    case Failure::UnsupportedWaterEncoding:return "UNSUPPORTED_WATER_ENCODING";
    case Failure::SourceDigestMismatch:return "SOURCE_DIGEST_MISMATCH";
    case Failure::SurfaceDigestMismatch:return "SURFACE_DIGEST_MISMATCH";
    case Failure::WaterDigestMismatch:return "WATER_DIGEST_MISMATCH";
    case Failure::PageDigestMismatch:return "PAGE_DIGEST_MISMATCH";
    case Failure::InvalidEnum:return "INVALID_ENUM";case Failure::LineageMismatch:return "LINEAGE_MISMATCH";}
    return "UNKNOWN";
}

struct Context
{
    std::string genesisDigest=kGenesisDigest;
    std::string generatorFamily=kGeneratorFamily,generatorVersion=kGeneratorVersion;
    std::string materialRegistryId=kMaterialRegistryId,materialRegistryDigest=kMaterialRegistryDigest;
    std::string surfaceGrammarId=Ms1::kSurfaceGrammarId,surfaceGrammarVersion=Ms1::kSurfaceGrammarVersion;
    std::string waterGrammarId=Ms1::kWaterGrammarId,waterGrammarVersion=Ms1::kWaterGrammarVersion;
    std::string descriptorSchemaDigest=Ms1::kWorldDescriptorSchemaDigest;
    bool diagnosticLegacy=false;
};

struct Page
{
    Trust trust=Trust::Unloaded;Failure failure=Failure::None;std::string detail;
    int ri=0,rj=0,n=0,surfaceN=0,waterN=0;double minX=0,minY=0,step=0,surfaceStep=0,waterStep=0;
    uint64_t regionId=0,sourceDigest=0,surfaceDigest=0,waterDigest=0;
    uint32_t surfaceVersion=0,waterVersion=0;
    std::string genesisDigest,pageDigest;
    std::vector<float> heights;std::vector<uint64_t> surfaceCodes,waterCodes;
    bool Authoritative()const{return trust==Trust::ValidAuthority;}
};

struct TrustedSlot
{
    Page published,lastRejected;bool hasPublished=false,hasRejected=false;
    bool Publish(Page const& candidate){if(!candidate.Authoritative()){lastRejected=candidate;hasRejected=true;return false;}
        published=candidate;hasPublished=true;hasRejected=false;return true;}
};

namespace Detail
{
inline uint32_t Ror(uint32_t x,unsigned n){return (x>>n)|(x<<(32-n));}
inline std::string Sha256(std::string const& input)
{
    static uint32_t const k[64]={
      0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
      0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
      0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
      0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
      0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
      0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
      0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
      0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u};
    std::vector<uint8_t> bytes(input.begin(),input.end());uint64_t bits=(uint64_t)bytes.size()*8;
    bytes.push_back(0x80);while(bytes.size()%64!=56)bytes.push_back(0);
    for(int i=7;i>=0;--i)bytes.push_back((uint8_t)(bits>>(i*8)));
    uint32_t h[8]={0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    for(size_t off=0;off<bytes.size();off+=64){uint32_t w[64];for(int i=0;i<16;++i)w[i]=((uint32_t)bytes[off+i*4]<<24)|((uint32_t)bytes[off+i*4+1]<<16)|((uint32_t)bytes[off+i*4+2]<<8)|bytes[off+i*4+3];
      for(int i=16;i<64;++i){uint32_t s0=Ror(w[i-15],7)^Ror(w[i-15],18)^(w[i-15]>>3),s1=Ror(w[i-2],17)^Ror(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}
      uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
      for(int i=0;i<64;++i){uint32_t s1=Ror(e,6)^Ror(e,11)^Ror(e,25),ch=(e&f)^((~e)&g),t1=hh+s1+ch+k[i]+w[i],s0=Ror(a,2)^Ror(a,13)^Ror(a,22),maj=(a&b)^(a&c)^(b&c),t2=s0+maj;hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}
      h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;}
    std::ostringstream out;out<<std::hex<<std::setfill('0');for(uint32_t v:h)out<<std::setw(8)<<v;return out.str();
}
inline uint64_t FnvBytes(uint64_t h,uint64_t v,int count){for(int i=0;i<count;++i){h^=(v>>(i*8))&0xff;h*=kFnvPrime;}return h;}
inline uint64_t FnvText(std::string const& s){uint64_t h=kFnvOffset;for(unsigned char c:s){h^=c;h*=kFnvPrime;}return h;}
inline std::string Hex64(uint64_t v){std::ostringstream s;s<<std::hex<<std::setfill('0')<<std::setw(16)<<v;return s.str();}
inline bool Int(std::string const& s,int& out){if(s.empty())return false;char* e=nullptr;long v=std::strtol(s.c_str(),&e,10);if(!e||*e||v<std::numeric_limits<int>::min()||v>std::numeric_limits<int>::max())return false;out=(int)v;return true;}
inline bool Double(std::string const& s,double& out){if(s.empty())return false;char* e=nullptr;out=std::strtod(s.c_str(),&e);return e&&!*e&&std::isfinite(out);}
inline bool Hex(std::string const& s,uint64_t& out){if(s.empty()||s.size()>16)return false;char* e=nullptr;out=std::strtoull(s.c_str(),&e,16);return e&&!*e;}
inline bool Cents(std::string const& s,int64_t& out)
{double value=0;if(!Double(s,value))return false;double const scaled=value*100.0;if(scaled<(double)std::numeric_limits<int64_t>::min()||scaled>(double)std::numeric_limits<int64_t>::max())return false;out=(int64_t)std::llround(scaled);return true;}
inline bool Parse(std::string const& text,std::string& magic,std::map<std::string,std::string>& v)
{std::istringstream in(text);std::string line;while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()||line[0]=='#')continue;if(magic.empty()){magic=line;continue;}size_t eq=line.find('=');if(eq==std::string::npos||eq==0||v.count(line.substr(0,eq)))return false;v[line.substr(0,eq)]=line.substr(eq+1);}return !magic.empty();}
inline std::string CanonicalPage(std::string const& magic,std::map<std::string,std::string> const& v)
{std::string out=magic+"\n";for(auto const& kv:v)if(kv.first!="page_digest")out+=kv.first+"="+kv.second+"\n";return out;}
inline std::string JsonEscape(std::string const& s){std::string o;for(char c:s){if(c=='\\'||c=='\"')o+='\\';o+=c;}return o;}
inline std::string CanonicalGenesis(std::map<std::string,std::string> const& v)
{static char const* names[]={"identity_schema_version","seed","coordinate_frame_id","generator_family","generator_version","generator_build_digest","generator_config_digest","material_registry_id","material_registry_digest","surface_grammar_id","surface_grammar_version","water_grammar_id","water_grammar_version"};std::string o="{";for(size_t i=0;i<13;++i){if(i)o+=',';o+='\"';o+=names[i];o+="\":\""+JsonEscape(v.at(names[i]))+"\"";}return o+"}";}
inline std::string RegionId(std::string const& seed,int ri,int rj)
{return Hex64(FnvText(std::string(kRegionKey)+":macro:"+seed+":("+std::to_string(ri)+","+std::to_string(rj)+")"));}
inline int SuperCell(int r){return (int)std::floor(((double)r*64000.0+192000.0)/384000.0);}
inline std::string SuperId(std::string const& seed,int si,int sj)
{return Hex64(FnvText(std::string(kRegionKey)+":macro:"+seed+":supertile:("+std::to_string(si)+","+std::to_string(sj)+")"));}
inline std::vector<std::string> Tokens(std::string const& s){std::istringstream in(s);std::vector<std::string> v;std::string x;while(in>>x)v.push_back(x);return v;}
inline Page Fail(Page page,Failure f,std::string const& d,Trust t=Trust::Quarantined){page.trust=t;page.failure=f;page.detail=d;return page;}
}

inline Page ValidateText(std::string const& text,int requestedRi,int requestedRj,Context const& ctx=Context())
{
    Page p;p.trust=Trust::Validating;p.ri=requestedRi;p.rj=requestedRj;std::string magic;std::map<std::string,std::string> v;
    if(!Detail::Parse(text,magic,v))return Detail::Fail(p,Failure::MalformedHeader,"invalid records");
    if(magic==kLegacyMagic)return Detail::Fail(p,Failure::LegacyPage,"legacy page is non-authoritative",ctx.diagnosticLegacy?Trust::DiagnosticOnly:Trust::Quarantined);
    if(magic!=kMagic)return Detail::Fail(p,Failure::MalformedHeader,"wrong page family");
    static char const* required[]={"macro_page_schema_version","identity_schema_version","seed","coordinate_frame_id","generator_family","generator_version","generator_build_digest","generator_config_digest","material_registry_id","material_registry_digest","surface_grammar_id","surface_grammar_version","water_grammar_id","water_grammar_version","genesis_digest","descriptor_schema_digest","region_key","region_id","region_cell","region_min_x_m","region_min_y_m","region_size_m","sample_step_m","grid_n","height_count","source_digest_algorithm","source_digest","height_grid_row_major","surface_descriptor_version","surface_step_m","surface_grid_n","surface_count","surface_digest_algorithm","surface_digest","surface_grid_row_major","water_descriptor_version","water_step_m","water_grid_n","water_count","water_digest_algorithm","water_digest","water_grid_row_major","parent_supertile_cell","parent_supertile_id","payload_encoding","page_digest_algorithm","page_digest","no_wrap_into_region","cheap_source","world_seed","world_identity_hash","neighbor_n","neighbor_s","neighbor_e","neighbor_w","neighbor_ne","neighbor_nw","neighbor_se","neighbor_sw"};
    if(v.size()!=sizeof(required)/sizeof(required[0]))return Detail::Fail(p,Failure::MalformedHeader,"missing or unknown field");for(char const* name:required)if(!v.count(name))return Detail::Fail(p,Failure::MalformedHeader,std::string("missing ")+name);
    int schema=0;if(!Detail::Int(v["macro_page_schema_version"],schema)||schema!=kPageSchemaVersion)return Detail::Fail(p,Failure::UnsupportedPageSchema,"unsupported page schema");
    bool const legacyIdentity=v["generator_build_digest"]==kLegacyGeneratorBuildDigest&&v["generator_config_digest"]==kLegacyGeneratorConfigDigest&&v["material_registry_digest"]==kLegacyMaterialRegistryDigest&&v["genesis_digest"]==kLegacyGenesisDigest;
    if(v["generator_family"]!=ctx.generatorFamily||v["generator_version"]!=ctx.generatorVersion||(!legacyIdentity&&(v["generator_build_digest"]!=kGeneratorBuildDigest||v["generator_config_digest"]!=kGeneratorConfigDigest)))return Detail::Fail(p,Failure::WrongGenerator,"generator identity mismatch");
    if(v["material_registry_id"]!=ctx.materialRegistryId||(!legacyIdentity&&v["material_registry_digest"]!=ctx.materialRegistryDigest))return Detail::Fail(p,Failure::WrongMaterialRegistry,"material registry mismatch");
    if(v["surface_grammar_id"]!=ctx.surfaceGrammarId||v["surface_grammar_version"]!=ctx.surfaceGrammarVersion||v["water_grammar_id"]!=ctx.waterGrammarId||v["water_grammar_version"]!=ctx.waterGrammarVersion||v["descriptor_schema_digest"]!=ctx.descriptorSchemaDigest)return Detail::Fail(p,Failure::WrongSchemaDigest,"descriptor contract mismatch");
    std::string computedGenesis=Detail::Sha256(Detail::CanonicalGenesis(v));if(v["genesis_digest"]!=computedGenesis||(!legacyIdentity&&v["genesis_digest"]!=ctx.genesisDigest))return Detail::Fail(p,Failure::WrongGenesis,"genesis mismatch");p.genesisDigest=v["genesis_digest"];
    size_t comma=v["region_cell"].find(',');int ri=0,rj=0;if(comma==std::string::npos||!Detail::Int(v["region_cell"].substr(0,comma),ri)||!Detail::Int(v["region_cell"].substr(comma+1),rj)||ri!=requestedRi||rj!=requestedRj)return Detail::Fail(p,Failure::WrongPageCoord,"stored coordinate mismatch");
    if(!Detail::Double(v["region_min_x_m"],p.minX)||!Detail::Double(v["region_min_y_m"],p.minY)||p.minX!=(double)ri*64000.0-32000.0||p.minY!=(double)rj*64000.0-32000.0)return Detail::Fail(p,Failure::WrongPageCoord,"absolute bounds mismatch");
    double size=0;if(!Detail::Double(v["region_size_m"],size)||size!=64000.0||!Detail::Double(v["sample_step_m"],p.step)||!Detail::Int(v["grid_n"],p.n)||!Detail::Double(v["surface_step_m"],p.surfaceStep)||!Detail::Int(v["surface_grid_n"],p.surfaceN)||!Detail::Double(v["water_step_m"],p.waterStep)||!Detail::Int(v["water_grid_n"],p.waterN)||p.step<=0||p.surfaceStep<=0||p.waterStep<=0||p.n<2||p.surfaceN<2||p.waterN<2||p.step*(p.n-1)!=64000.0||p.surfaceStep*(p.surfaceN-1)!=64000.0||p.waterStep*(p.waterN-1)!=64000.0)return Detail::Fail(p,Failure::InvalidRange,"invalid grids");
    int hc=0,sc=0,wc=0;if(!Detail::Int(v["height_count"],hc)||!Detail::Int(v["surface_count"],sc)||!Detail::Int(v["water_count"],wc))return Detail::Fail(p,Failure::MalformedHeader,"invalid counts");auto ht=Detail::Tokens(v["height_grid_row_major"]),st=Detail::Tokens(v["surface_grid_row_major"]),wt=Detail::Tokens(v["water_grid_row_major"]);if(hc!=p.n*p.n||sc!=p.surfaceN*p.surfaceN||wc!=p.waterN*p.waterN||(int)ht.size()!=hc||(int)st.size()!=sc||(int)wt.size()!=wc)return Detail::Fail(p,Failure::TruncatedPayload,"payload count mismatch");
    p.heights.reserve(ht.size());uint64_t hd=kFnvOffset;for(auto const& token:ht){double z=0;int64_t q=0;if(!Detail::Double(token,z)||!Detail::Cents(token,q))return Detail::Fail(p,Failure::InvalidRange,"invalid height");p.heights.push_back((float)z);hd=Detail::FnvBytes(hd,(uint64_t)q,6);}
    int sv=0,wv=0;if(!Detail::Int(v["surface_descriptor_version"],sv)||sv!=(int)Ms1::kSurfaceEncodingVersion)return Detail::Fail(p,Failure::UnsupportedSurfaceEncoding,"unsupported SurfaceState encoding");if(!Detail::Int(v["water_descriptor_version"],wv)||wv!=(int)Ms1::kWaterEncodingVersion)return Detail::Fail(p,Failure::UnsupportedWaterEncoding,"unsupported WaterState encoding");p.surfaceVersion=(uint32_t)sv;p.waterVersion=(uint32_t)wv;
    p.surfaceCodes.reserve(st.size());uint64_t sd=kFnvOffset;for(auto const& token:st){uint64_t code=0;if(!Detail::Hex(token,code))return Detail::Fail(p,Failure::MalformedHeader,"malformed SurfaceState token");Ms1::SurfaceDescriptorWire decoded;Ms1::DescriptorDecodeError error;if(!Ms1::DecodeSurfaceDescriptor(p.surfaceVersion,code,decoded,&error))return Detail::Fail(p,Failure::InvalidEnum,"invalid SurfaceState descriptor");p.surfaceCodes.push_back(code);sd=Detail::FnvBytes(sd,code,5);}
    p.waterCodes.reserve(wt.size());uint64_t wd=kFnvOffset;for(auto const& token:wt){uint64_t code=0;if(!Detail::Hex(token,code))return Detail::Fail(p,Failure::MalformedHeader,"malformed WaterState token");Ms1::WaterDescriptorWire decoded;Ms1::DescriptorDecodeError error;if(!Ms1::DecodeWaterDescriptor(p.waterVersion,code,decoded,&error))return Detail::Fail(p,Failure::InvalidEnum,"invalid WaterState descriptor");p.waterCodes.push_back(code);wd=Detail::FnvBytes(wd,code,8);}
    if(v["source_digest_algorithm"]!=kSourceDigestAlgorithm||!Detail::Hex(v["source_digest"],p.sourceDigest)||p.sourceDigest!=hd)return Detail::Fail(p,Failure::SourceDigestMismatch,"terrain payload digest mismatch");
    if(v["surface_digest_algorithm"]!=kSurfaceDigestAlgorithm||!Detail::Hex(v["surface_digest"],p.surfaceDigest)||p.surfaceDigest!=sd)return Detail::Fail(p,Failure::SurfaceDigestMismatch,"surface payload digest mismatch");
    if(v["water_digest_algorithm"]!=kWaterDigestAlgorithm||!Detail::Hex(v["water_digest"],p.waterDigest)||p.waterDigest!=wd)return Detail::Fail(p,Failure::WaterDigestMismatch,"water payload digest mismatch");
    if(v["region_key"]!=kRegionKey||v["region_id"]!=Detail::RegionId(v["seed"],ri,rj)||!Detail::Hex(v["region_id"],p.regionId))return Detail::Fail(p,Failure::LineageMismatch,"region lineage mismatch");
    struct Dir{char const* name;int x,y;};static Dir const dirs[]={{"n",0,1},{"s",0,-1},{"e",1,0},{"w",-1,0},{"ne",1,1},{"nw",-1,1},{"se",1,-1},{"sw",-1,-1}};for(auto const& d:dirs)if(v[std::string("neighbor_")+d.name]!=Detail::RegionId(v["seed"],ri+d.x,rj+d.y))return Detail::Fail(p,Failure::LineageMismatch,"neighbor lineage mismatch");
    int si=Detail::SuperCell(ri),sj=Detail::SuperCell(rj);if(v["parent_supertile_cell"]!=std::to_string(si)+","+std::to_string(sj)||v["parent_supertile_id"]!=Detail::SuperId(v["seed"],si,sj))return Detail::Fail(p,Failure::LineageMismatch,"parent supertile lineage mismatch");
    if(v["world_seed"]!=v["seed"]||v["payload_encoding"]!="ascii-row-major-v1"||v["no_wrap_into_region"]!="1"||v["cheap_source"]!="macro_analytic_no_reconstructedz")return Detail::Fail(p,Failure::MalformedHeader,"invalid page declaration");
    p.pageDigest=Detail::Sha256(Detail::CanonicalPage(magic,v));if(v["page_digest_algorithm"]!=kPageDigestAlgorithm||v["page_digest"]!=p.pageDigest)return Detail::Fail(p,Failure::PageDigestMismatch,"whole-page digest mismatch");
    if(legacyIdentity){p.trust=ctx.diagnosticLegacy?Trust::DiagnosticOnly:Trust::Quarantined;p.failure=Failure::WrongGenesis;p.detail="superseded opaque EI0.C generator identity";return p;}
    p.trust=Trust::ValidAuthority;p.failure=Failure::None;p.detail.clear();return p;
}

inline Page Load(std::string const& path,int ri,int rj,Context const& ctx=Context())
{std::ifstream in(path,std::ios::binary);Page p;p.ri=ri;p.rj=rj;if(!in)return Detail::Fail(p,Failure::IoError,"open failed");std::ostringstream text;text<<in.rdbuf();return ValidateText(text.str(),ri,rj,ctx);}
}
