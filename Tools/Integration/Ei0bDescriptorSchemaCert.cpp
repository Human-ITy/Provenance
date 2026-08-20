#include "WorldDescriptors.generated.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
bool StringField(std::string const& line,char const* key,std::string& out)
{
    std::string const needle=std::string("\"")+key+"\":\"";
    size_t const p=line.find(needle);if(p==std::string::npos)return false;
    size_t const first=p+needle.size(),last=line.find('"',first);
    if(last==std::string::npos)return false;out=line.substr(first,last-first);return true;
}
bool IntField(std::string const& line,char const* key,int& out)
{
    std::string const needle=std::string("\"")+key+"\":";
    size_t const p=line.find(needle);if(p==std::string::npos)return false;
    char* end=nullptr;long const value=std::strtol(line.c_str()+p+needle.size(),&end,10);
    if(end==line.c_str()+p+needle.size())return false;out=(int)value;return true;
}
bool BoolField(std::string const& line,char const* key,bool& out)
{
    std::string const needle=std::string("\"")+key+"\":";
    size_t const p=line.find(needle);if(p==std::string::npos)return false;
    size_t const first=p+needle.size();
    if(line.compare(first,4,"true")==0){out=true;return true;}
    if(line.compare(first,5,"false")==0){out=false;return true;}return false;
}
bool ArrayField(std::string const& line,char const* key,std::vector<double>& out)
{
    std::string const needle=std::string("\"")+key+"\":[";
    size_t const p=line.find(needle);if(p==std::string::npos)return false;
    char const* cur=line.c_str()+p+needle.size();
    while(*cur&&*cur!=']')
    {
        char* end=nullptr;double const value=std::strtod(cur,&end);if(end==cur)return false;
        out.push_back(value);cur=end;if(*cur==',')++cur;
    }
    return *cur==']';
}
Ms1::DescriptorDecodeError ErrorCode(std::string const& value)
{
    if(value=="unknown_encoding_version")return Ms1::DescriptorDecodeError::UnknownEncodingVersion;
    if(value=="reserved_bits")return Ms1::DescriptorDecodeError::ReservedBits;
    if(value=="reserved_enum")return Ms1::DescriptorDecodeError::ReservedEnum;
    if(value=="illegal_combination")return Ms1::DescriptorDecodeError::IllegalCombination;
    return Ms1::DescriptorDecodeError::None;
}
bool EqualCodes(std::vector<double> const& expected,std::vector<unsigned> const& actual)
{
    if(expected.size()!=actual.size())return false;
    for(size_t i=0;i<actual.size();++i)if((unsigned)expected[i]!=actual[i])return false;
    return true;
}
}

int main(int argc,char** argv)
{
    if(argc!=2){std::cerr<<"usage: Ei0bDescriptorSchemaCert vectors.jsonl\n";return 2;}
    std::ifstream in(argv[1]);if(!in){std::cerr<<"cannot read vectors\n";return 2;}
    size_t validCount=0,invalidCount=0,lineNo=0;std::string line;
    while(std::getline(in,line))
    {
        ++lineNo;std::string descriptor,name,hex,errorName;int version=0;bool valid=false;
        if(!StringField(line,"descriptor",descriptor)||!StringField(line,"name",name)
           ||!StringField(line,"packed_hex",hex)||!IntField(line,"version",version)
           ||!BoolField(line,"valid",valid)){std::cerr<<"malformed vector "<<lineNo<<"\n";return 3;}
        uint64_t const expected=std::strtoull(hex.c_str(),nullptr,16);
        Ms1::DescriptorDecodeError error=Ms1::DescriptorDecodeError::None;
        if(!valid)
        {
            StringField(line,"error",errorName);bool accepted=false;
            if(descriptor==Ms1::kSurfaceEncodingId){Ms1::SurfaceDescriptorWire out;accepted=Ms1::DecodeSurfaceDescriptor(version,expected,out,&error);}
            else {Ms1::WaterDescriptorWire out;accepted=Ms1::DecodeWaterDescriptor(version,expected,out,&error);}
            if(accepted||error!=ErrorCode(errorName)){std::cerr<<name<<": invalid vector accepted/wrong error\n";return 4;}
            ++invalidCount;continue;
        }

        std::vector<double> input,codes;if(!ArrayField(line,"input",input)||!ArrayField(line,"codes",codes))return 3;
        uint64_t packed=0;
        if(descriptor==Ms1::kSurfaceEncodingId)
        {
            if(input.size()!=10)return 3;Ms1::SurfaceDescriptorWire wire;
            wire.substrate=(uint8_t)input[0];wire.lith=(uint8_t)input[1];wire.family=(uint8_t)input[2];
            wire.wetness=input[3];wire.weathering=input[4];wire.soilDepth=input[5];wire.stability=input[6];
            wire.organic=input[7];wire.exposure=input[8];wire.roughness=input[9];
            if(!Ms1::EncodeSurfaceDescriptor(version,wire,packed,&error)||packed!=expected){std::cerr<<name<<": C++ surface encode drift\n";return 5;}
            Ms1::SurfaceDescriptorWire decoded;if(!Ms1::DecodeSurfaceDescriptor(version,packed,decoded,&error))return 5;
            if(!EqualCodes(codes,{decoded.substrate,decoded.lith,decoded.family,decoded.wetnessQ,decoded.weatheringQ,
                decoded.soilDepthQ,decoded.stabilityQ,decoded.organicQ,decoded.exposureQ,decoded.roughnessQ}))return 5;
            if(decoded.wetness!=codes[3]/15.0||decoded.weathering!=codes[4]/15.0
               ||decoded.soilDepth!=codes[5]/15.0||decoded.stability!=codes[6]/15.0
               ||decoded.organic!=codes[7]/15.0||decoded.exposure!=codes[8]/15.0
               ||decoded.roughness!=codes[9]/15.0)return 5;
        }
        else
        {
            if(input.size()!=16)return 3;Ms1::WaterDescriptorWire wire;
            wire.encodedPresence=(uint8_t)input[0];wire.body=(uint8_t)input[1];wire.flow=(uint8_t)input[2];wire.bottomFamily=(uint8_t)input[3];
            wire.depthM=input[4];wire.discharge=input[5];wire.seasonality=input[6];wire.clarity=input[7];wire.turbidity=input[8];
            wire.sediment=input[9];wire.mineral=input[10];wire.organic=input[11];wire.temperature=input[12];wire.waterfallPot=input[13];
            wire.mineralPot=input[14];wire.authority=(uint8_t)input[15];
            if(!Ms1::EncodeWaterDescriptor(version,wire,packed,&error)||packed!=expected){std::cerr<<name<<": C++ water encode drift\n";return 6;}
            Ms1::WaterDescriptorWire decoded;if(!Ms1::DecodeWaterDescriptor(version,packed,decoded,&error))return 6;
            if(!EqualCodes(codes,{decoded.encodedPresence,decoded.body,decoded.flow,decoded.bottomFamily,decoded.depthQ,
                decoded.dischargeQ,decoded.seasonalityQ,decoded.clarityQ,decoded.turbidityQ,decoded.sedimentQ,decoded.mineralQ,
                decoded.organicQ,decoded.temperatureQ,decoded.waterfallQ,decoded.mineralPotQ,decoded.authority}))return 6;
            if(decoded.depthM!=codes[4]/4.0||decoded.discharge!=codes[5]/15.0
               ||decoded.seasonality!=codes[6]/15.0||decoded.clarity!=codes[7]/15.0
               ||decoded.turbidity!=codes[8]/15.0||decoded.sediment!=codes[9]/15.0
               ||decoded.mineral!=codes[10]/15.0||decoded.organic!=codes[11]/15.0
               ||decoded.temperature!=codes[12]/15.0||decoded.waterfallPot!=codes[13]/3.0
               ||decoded.mineralPot!=codes[14]/3.0)return 6;
            if(decoded.authority==Ms1::WA_DEFER_TO_DETAILED&&(decoded.hasPresenceConclusion||decoded.presence!=Ms1::WP_UNSPECIFIED))return 6;
        }
        ++validCount;
    }
    {
        Ms1::WaterDescriptorWire reserved;reserved.authority=2;uint64_t packed=0;
        Ms1::DescriptorDecodeError error=Ms1::DescriptorDecodeError::None;
        if(Ms1::EncodeWaterDescriptor(Ms1::kWaterEncodingVersion,reserved,packed,&error)
           ||error!=Ms1::DescriptorDecodeError::ReservedEnum)
        {std::cerr<<"reserved authority input accepted\n";return 7;}
    }
    std::cout<<"EI0.B C++ SCHEMA CERT: PASS\n"
             <<"schema_digest="<<Ms1::kWorldDescriptorSchemaDigest<<"\n"
             <<"valid_vectors="<<validCount<<" invalid_vectors="<<invalidCount<<"\n";
    return 0;
}
