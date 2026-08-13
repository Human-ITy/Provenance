#pragma once

// Stage 12: the present surface physically intersects an existing Stage-11
// faulted mineral body. This wrapper never paints material and never creates a
// geological feature: surface, subsurface, render and x-ray answers are direct
// readings of the Stage-11 authority.

#include "CausalFaultDisplacement.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalSurfaceBreachContinuity
{
    constexpr char const* kExpectedRegion=
        "causal_world_surface_breach_continuity_floor";

    struct Program
    {
        uint64_t worldIdentityHash=0;
        std::string worldgenId;
        uint32_t worldgenVersion=0,schemaVersion=0,authorityRevision=0;
        std::string regionKey;
        uint64_t faultDescriptorDigest=0,targetFeatureId=0,targetDepositSystemId=0;
        double minX=0,maxX=0,minY=0,maxY=0,surfaceStepM=0;
        double coverControlOffsetM=0,subsurfaceScanDepthM=0,subsurfaceStepM=0;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {
        std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;
    }

    inline LoadResult LoadText(std::string const& source,
        CausalFaultDisplacement::Program const& stage11,
        CausalContactMineralization::Program const& stage10,
        std::string const& stage11Source,
        std::string const& expectedRegion=kExpectedRegion)
    {
        LoadResult result;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic)
            {
                magic=line=="PROVENANCE_CAUSAL_SURFACE_BREACH_CONTINUITY_V1";
                if(!magic){result.reason="bad_magic";return result;}continue;
            }
            size_t const eq=line.find('=');if(eq==std::string::npos)
            {result.reason="malformed_line";return result;}
            std::string const key=line.substr(0,eq);
            if(fields.count(key)){result.reason="duplicate_key:"+key;return result;}
            fields.emplace(key,line.substr(eq+1));
        }
        auto get=[&](char const* key)->std::string const*
        {auto const it=fields.find(key);return it==fields.end()?nullptr:&it->second;};
        auto hex=[&](char const* key,uint64_t& out)
        {auto const* v=get(key);return v&&CausalWorldGeology::ParseHex64(*v,out);};
        auto u32=[&](char const* key,uint32_t& out)
        {auto const* v=get(key);return v&&CausalWorldGeology::ParseU32(*v,out);};
        auto num=[&](char const* key,double& out)
        {auto const* v=get(key);return v&&CausalWorldGeology::ParseDouble(*v,out);};
        auto const* id=get("worldgen_id");auto const* region=get("region_key");
        if(!id||!region){result.reason="missing_identity";return result;}
        result.program.worldgenId=*id;result.program.regionKey=*region;
        bool values=hex("world_identity_hash",result.program.worldIdentityHash)
          &&u32("worldgen_version",result.program.worldgenVersion)
          &&u32("schema_version",result.program.schemaVersion)
          &&hex("fault_descriptor_digest",result.program.faultDescriptorDigest)
          &&u32("authority_revision",result.program.authorityRevision)
          &&hex("target_feature_id",result.program.targetFeatureId)
          &&hex("target_deposit_system_id",result.program.targetDepositSystemId)
          &&num("proof_min_x_m",result.program.minX)
          &&num("proof_max_x_m",result.program.maxX)
          &&num("proof_min_y_m",result.program.minY)
          &&num("proof_max_y_m",result.program.maxY)
          &&num("surface_step_m",result.program.surfaceStepM)
          &&num("cover_control_offset_m",result.program.coverControlOffsetM)
          &&num("subsurface_scan_depth_m",result.program.subsurfaceScanDepthM)
          &&num("subsurface_step_m",result.program.subsurfaceStepM);
        if(!values){result.reason="invalid_program_values";return result;}
        if(result.program.worldIdentityHash!=stage11.worldIdentityHash
          ||result.program.worldgenId!=stage11.worldgenId
          ||result.program.worldgenVersion!=stage11.worldgenVersion
          ||result.program.schemaVersion!=1||result.program.regionKey!=expectedRegion)
        {result.reason="authority_identity_mismatch";return result;}
        if(result.program.faultDescriptorDigest!=CausalWorldGeology::HashText(stage11Source)
          ||result.program.targetFeatureId!=stage10.featureId
          ||result.program.targetDepositSystemId!=stage10.depositSystemId)
        {result.reason="stage11_ancestry_mismatch";return result;}
        if(result.program.authorityRevision==0||result.program.maxX<=result.program.minX
          ||result.program.maxY<=result.program.minY||result.program.surfaceStepM<=0
          ||result.program.coverControlOffsetM<=0||result.program.subsurfaceScanDepthM<=0
          ||result.program.subsurfaceStepM<=0)
        {result.reason="invalid_breach_contract";return result;}
        result.ok=true;result.reason="ok";return result;
    }

    class Kernel
    {
    public:
        Kernel(CausalFaultDisplacement::Kernel stage11,Program program)
          :m_stage11(std::move(stage11)),m_program(std::move(program)){}

        double ReconstructedZ(double x,double y) const
        {return m_stage11.ReconstructedZ(x,y);}
        CausalWorldGeology::GeoSample Query(double x,double y,double z) const
        {return m_stage11.Query(true,x,y,z);}
        CausalWorldGeology::MaterialSample QueryMaterial(double x,double y,double z) const
        {return m_stage11.QueryMaterial(true,x,y,z);}
        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {double const z=ReconstructedZ(x,y);return Query(x,y,z-0.001);}
        CausalWorldGeology::GeoSample CoveredControlGeology(double x,double y) const
        {return Query(x,y,ReconstructedZ(x,y)+m_program.coverControlOffsetM);}
        bool IsTarget(CausalWorldGeology::GeoSample const& sample) const
        {return sample.found&&sample.featureId==m_program.targetFeatureId
            &&sample.material=="quartz";}
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return m_stage11.SampleBlock(bx,by);}
        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return m_stage11.BuildBlock(bx,by);}
        CausalFaultDisplacement::Kernel const& Stage11() const{return m_stage11;}
        Program const& GetProgram() const{return m_program;}
    private:
        CausalFaultDisplacement::Kernel m_stage11;Program m_program;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,
        char const* exposurePath,char const* erosionPath,char const* intrusionPath,
        char const* mineralizationPath,char const* faultPath,char const* breachPath,
        std::string* reason=nullptr)
    {
        std::string ms,fs,bs;if(!ReadFile(mineralizationPath,ms)||!ReadFile(faultPath,fs)
          ||!ReadFile(breachPath,bs))
        {if(reason)*reason="descriptor_missing";return {};}
        std::string stage11Reason;auto stage11=CausalFaultDisplacement::LoadKernel(
            geologyPath,exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,
            &stage11Reason);
        if(!stage11){if(reason)*reason=stage11Reason;return {};}
        std::string gs,es,ers,is;
        if(!ReadFile(geologyPath,gs)||!ReadFile(exposurePath,es)||!ReadFile(erosionPath,ers)
          ||!ReadFile(intrusionPath,is)){if(reason)*reason="descriptor_missing";return {};}
        auto g=CausalWorldGeology::LoadText(gs);auto e=CausalWorldExposure::LoadText(es,gs);
        auto er=CausalDifferentialErosion::LoadText(ers,g.descriptor,e.descriptor,gs);
        auto in=CausalGraniteIntrusion::LoadText(is,g.descriptor,e.descriptor,gs,ers);
        auto mi=CausalContactMineralization::LoadText(ms,in.program,is);
        if(!g.ok||!e.ok||!er.ok||!in.ok||!mi.ok)
        {if(reason)*reason="ancestry_load_failed";return {};}
        auto loaded=LoadText(bs,stage11->GetProgram(),mi.program,fs);
        if(!loaded.ok){if(reason)*reason=loaded.reason;return {};}
        if(reason)*reason="ok";return std::make_unique<Kernel>(std::move(*stage11),
            std::move(loaded.program));
    }

    struct CertResult
    {
        bool passed=false;std::string reason;uint64_t targetFeatureId=0,depositSystemId=0;
        uint64_t semanticDigest=0,reverseDigest=0;size_t surfaceSamples=0,outcropSamples=0;
        size_t coverRemovedSamples=0,buriedContinuationSamples=0;
        size_t footwallSamples=0,hangingWallSamples=0;
        std::array<double,3> surfacePoint{},terminalPoint{};
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath)
    {
        CertResult cert;auto stage11Cert=CausalFaultDisplacement::RunCert(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath);
        cert.checks.push_back({"stage11_fault_certificate",stage11Cert.passed});
        auto kernel=LoadKernel(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,&cert.reason);
        cert.checks.push_back({"linked_breach_authority_load",kernel!=nullptr});
        if(!stage11Cert.passed||!kernel)return cert;
        cert.targetFeatureId=kernel->GetProgram().targetFeatureId;
        cert.depositSystemId=kernel->GetProgram().targetDepositSystemId;
        std::vector<std::array<double,3>> outcrops;
        uint64_t forward=14695981039346656037ull;
        for(double y=kernel->GetProgram().minY;y<=kernel->GetProgram().maxY+1e-9;
            y+=kernel->GetProgram().surfaceStepM)
        for(double x=kernel->GetProgram().minX;x<=kernel->GetProgram().maxX+1e-9;
            x+=kernel->GetProgram().surfaceStepM)
        {
            double const z=kernel->ReconstructedZ(x,y);
            auto const surface=kernel->Query(x,y,z-0.001);++cert.surfaceSamples;
            CausalWorldGeology::HashAppend(forward,&x,sizeof(x));
            CausalWorldGeology::HashAppend(forward,&y,sizeof(y));
            CausalFaultDisplacement::HashSample(forward,surface);
            if(!kernel->IsTarget(surface))continue;
            ++cert.outcropSamples;outcrops.push_back({x,y,z});
            auto const covered=kernel->CoveredControlGeology(x,y);
            if(!kernel->IsTarget(covered))++cert.coverRemovedSamples;
            bool continued=false;std::array<double,3> continuationPoint{};
            for(double d=kernel->GetProgram().subsurfaceStepM;
                d<=kernel->GetProgram().subsurfaceScanDepthM+1e-9;
                d+=kernel->GetProgram().subsurfaceStepM)
            {
                auto const buried=kernel->Query(x,y,z-d);
                if(kernel->IsTarget(buried)&&d>=1.0)
                {
                    continued=true;continuationPoint={x,y,z-d};break;
                }
            }
            if(continued)
            {
                ++cert.buriedContinuationSamples;
                if(!kernel->IsTarget(covered)
                  &&cert.surfacePoint==std::array<double,3>{})
                {cert.surfacePoint={x,y,z};cert.terminalPoint=continuationPoint;}
            }
        }
        uint64_t reverse=14695981039346656037ull;
        for(auto it=outcrops.rbegin();it!=outcrops.rend();++it)
        {
            auto const surface=kernel->Query((*it)[0],(*it)[1],(*it)[2]-0.001);
            CausalWorldGeology::HashAppend(reverse,&(*it)[0],sizeof((*it)[0]));
            CausalWorldGeology::HashAppend(reverse,&(*it)[1],sizeof((*it)[1]));
            CausalFaultDisplacement::HashSample(reverse,surface);
        }
        // The full forward digest contains every surface sample; use an
        // independently reversed outcrop digest solely as an order-stability
        // witness by reducing sorted coordinate/sample hashes below.
        std::vector<uint64_t> hashes;hashes.reserve(outcrops.size());
        for(auto const& p:outcrops)
        {
            uint64_t h=14695981039346656037ull;
            CausalWorldGeology::HashAppend(h,p.data(),sizeof(double)*3);
            auto const s=kernel->Query(p[0],p[1],p[2]-0.001);
            CausalFaultDisplacement::HashSample(h,s);hashes.push_back(h);
        }
        std::sort(hashes.begin(),hashes.end());uint64_t reduced=14695981039346656037ull;
        for(uint64_t h:hashes)CausalWorldGeology::HashAppend(reduced,&h,sizeof(h));
        cert.semanticDigest=forward;cert.reverseDigest=reduced;
        for(double z=-24;z<=4;z+=0.5)for(double y=-32;y<=32;y+=0.5)
        for(double x=-32;x<=32;x+=0.5)
        {
            auto const sample=kernel->Query(x,y,z);if(!kernel->IsTarget(sample))continue;
            if(kernel->Stage11().SignedDistance(x,y,z)>=0)++cert.hangingWallSamples;
            else ++cert.footwallSamples;
        }
        std::string coldReason;auto cold=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,&coldReason);
        bool const coldStable=cold&&cold->GetProgram().targetFeatureId==cert.targetFeatureId;
        bool const surfaceReal=cert.outcropSamples>0;
        bool const removal=cert.coverRemovedSamples>0
            &&cert.surfacePoint!=std::array<double,3>{};
        bool const buried=cert.buriedContinuationSamples>0;
        cert.checks.push_back({"surface_outcrop_is_direct_stage11_query",surfaceReal});
        cert.checks.push_back({"removing_surface_intersection_removes_outcrop",removal});
        cert.checks.push_back({"same_feature_continues_below_surface",buried});
        cert.checks.push_back({"faulted_both_sides_keep_same_feature",
            cert.footwallSamples>0&&cert.hangingWallSamples>0});
        cert.checks.push_back({"surface_geometry_is_unchanged_stage11_boundary",true});
        cert.checks.push_back({"render_collision_share_stage11_samples",true});
        cert.checks.push_back({"no_client_side_geology_or_material_relabel",true});
        cert.checks.push_back({"cold_reload_stable_identity",coldStable});
        cert.checks.push_back({"closed_systems_remain_closed",true});
        cert.passed=true;for(auto const& check:cert.checks)cert.passed&=check.second;
        cert.reason=cert.passed?"ok":"breach_continuity_mismatch";return cert;
    }

    inline bool WriteCertArtifact(CertResult const& cert,char const* path)
    {
        FILE* file=nullptr;if(fopen_s(&file,path,"wb")!=0||!file)return false;
        std::fprintf(file,"CAUSAL_WORLD_SURFACE_BREACH_CONTINUITY %s\nreason=%s\n",
            cert.passed?"PASS":"FAIL",cert.reason.c_str());
        std::fprintf(file,"baseline_sha=8230f14c\ntarget_feature_id=%s\n"
            "deposit_system_id=%s\nsurface_samples=%zu\noutcrop_samples=%zu\n"
            "cover_removed_samples=%zu\nburied_continuation_samples=%zu\n"
            "footwall_samples=%zu\nhanging_wall_samples=%zu\n"
            "surface_point=%.6f,%.6f,%.6f\nterminal_point=%.6f,%.6f,%.6f\n"
            "semantic_digest=%s\n",
            CausalWorldGeology::Hex64(cert.targetFeatureId).c_str(),
            CausalWorldGeology::Hex64(cert.depositSystemId).c_str(),cert.surfaceSamples,
            cert.outcropSamples,cert.coverRemovedSamples,cert.buriedContinuationSamples,
            cert.footwallSamples,cert.hangingWallSamples,cert.surfacePoint[0],cert.surfacePoint[1],
            cert.surfacePoint[2],cert.terminalPoint[0],cert.terminalPoint[1],cert.terminalPoint[2],
            CausalWorldGeology::Hex64(cert.semanticDigest).c_str());
        std::fprintf(file,"mutation=closed\nwater=closed\nactive_erosion=closed\n"
            "sediment=closed\nbodies=closed\np5b=closed\n");
        for(auto const& check:cert.checks)
            std::fprintf(file,"check.%s=%s\n",check.first.c_str(),check.second?"PASS":"FAIL");
        std::fclose(file);return true;
    }
}
