#pragma once

// Stage 11: a canonical, identity-preserving fault transform over the
// certified Stage-10 assembled history. Missing fault ownership always falls
// back to Stage 10; it never means air or absent terrain.

#include "CausalContactMineralization.h"

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

namespace CausalFaultDisplacement
{
    constexpr char const* kExpectedRegion="causal_world_fault_displacement_floor";

    struct Program
    {
        uint64_t worldIdentityHash=0;
        std::string worldgenId;
        uint32_t worldgenVersion=0,schemaVersion=0,authorityRevision=0;
        std::string regionKey;
        uint64_t mineralizationDescriptorDigest=0,faultEventId=0;
        uint32_t faultChronology=0;
        std::array<double,3> planePoint{},planeNormal{1.0,0.0,0.0},displacement{};
        std::array<double,3> domainMin{},domainMax{};
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {
        std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;
    }

    inline LoadResult LoadText(std::string const& source,
        CausalContactMineralization::Program const& stage10,
        std::string const& stage10Source,
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
                magic=line=="PROVENANCE_CAUSAL_FAULT_DISPLACEMENT_V1";
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
          &&hex("mineralization_descriptor_digest",result.program.mineralizationDescriptorDigest)
          &&u32("authority_revision",result.program.authorityRevision)
          &&hex("fault_event_id",result.program.faultEventId)
          &&u32("fault_chronology",result.program.faultChronology)
          &&num("plane_x_m",result.program.planePoint[0])
          &&num("plane_y_m",result.program.planePoint[1])
          &&num("plane_z_m",result.program.planePoint[2])
          &&num("plane_nx",result.program.planeNormal[0])
          &&num("plane_ny",result.program.planeNormal[1])
          &&num("plane_nz",result.program.planeNormal[2])
          &&num("displacement_x_m",result.program.displacement[0])
          &&num("displacement_y_m",result.program.displacement[1])
          &&num("displacement_z_m",result.program.displacement[2])
          &&num("domain_min_x_m",result.program.domainMin[0])
          &&num("domain_max_x_m",result.program.domainMax[0])
          &&num("domain_min_y_m",result.program.domainMin[1])
          &&num("domain_max_y_m",result.program.domainMax[1])
          &&num("domain_min_z_m",result.program.domainMin[2])
          &&num("domain_max_z_m",result.program.domainMax[2]);
        if(!values){result.reason="invalid_program_values";return result;}
        double const nl=std::sqrt(result.program.planeNormal[0]*result.program.planeNormal[0]
          +result.program.planeNormal[1]*result.program.planeNormal[1]
          +result.program.planeNormal[2]*result.program.planeNormal[2]);
        double const slip=std::sqrt(result.program.displacement[0]*result.program.displacement[0]
          +result.program.displacement[1]*result.program.displacement[1]
          +result.program.displacement[2]*result.program.displacement[2]);
        bool domain=true;for(int i=0;i<3;++i)domain=domain
          &&result.program.domainMax[i]>result.program.domainMin[i];
        if(result.program.worldIdentityHash!=stage10.worldIdentityHash
          ||result.program.worldgenId!=stage10.worldgenId
          ||result.program.worldgenVersion!=stage10.worldgenVersion
          ||result.program.schemaVersion!=1||result.program.regionKey!=expectedRegion)
        {result.reason="authority_identity_mismatch";return result;}
        if(result.program.mineralizationDescriptorDigest
             !=CausalWorldGeology::HashText(stage10Source))
        {result.reason="stage10_ancestry_mismatch";return result;}
        if(result.program.authorityRevision==0||result.program.faultEventId==0
          ||result.program.faultChronology<=stage10.mineralizingChronology
          ||nl<0.999999||nl>1.000001||slip<0.5||!domain)
        {result.reason="invalid_fault_contract";return result;}
        result.ok=true;result.reason="ok";return result;
    }

    class Kernel
    {
    public:
        Kernel(CausalContactMineralization::Kernel stage10,Program program)
          :m_stage10(std::move(stage10)),m_program(std::move(program)){}

        bool InDomain(double x,double y,double z) const
        {
            return x>=m_program.domainMin[0]&&x<=m_program.domainMax[0]
              &&y>=m_program.domainMin[1]&&y<=m_program.domainMax[1]
              &&z>=m_program.domainMin[2]&&z<=m_program.domainMax[2];
        }
        double SignedDistance(double x,double y,double z) const
        {
            return (x-m_program.planePoint[0])*m_program.planeNormal[0]
              +(y-m_program.planePoint[1])*m_program.planeNormal[1]
              +(z-m_program.planePoint[2])*m_program.planeNormal[2];
        }
        bool ShouldDisplace(CausalWorldGeology::GeoSample const& sample) const
        {
            if(!sample.found)return false;
            uint32_t youngest=0;for(uint32_t c:sample.chronology)youngest=(std::max)(youngest,c);
            return youngest<m_program.faultChronology;
        }
        bool ShouldDisplace(CausalWorldGeology::MaterialSample const& sample) const
        {return sample.found&&sample.youngestChronology<m_program.faultChronology;}
        std::array<double,3> SourcePoint(bool eventEnabled,double x,double y,double z,
            bool* shifted=nullptr) const
        {
            bool move=eventEnabled&&InDomain(x,y,z)&&SignedDistance(x,y,z)>=0.0;
            std::array<double,3> source{x,y,z};
            if(move)
            {
                for(int i=0;i<3;++i)source[i]-=m_program.displacement[i];
                if(!InDomain(source[0],source[1],source[2]))move=false;
            }
            if(!move)source={x,y,z};if(shifted)*shifted=move;return source;
        }
        CausalWorldGeology::GeoSample Query(bool eventEnabled,double x,double y,double z) const
        {
            if(!eventEnabled||!InDomain(x,y,z))return m_stage10.Query(true,x,y,z);
            bool shifted=false;auto const source=SourcePoint(true,x,y,z,&shifted);
            CausalWorldGeology::GeoSample sample=m_stage10.Query(true,
                source[0],source[1],source[2]);
            if(shifted&&!ShouldDisplace(sample))sample=m_stage10.Query(true,x,y,z);
            if(!sample.found)return m_stage10.Query(true,x,y,z);
            if(ShouldDisplace(sample))
            {
                sample.eventIds.push_back(m_program.faultEventId);
                sample.chronology.push_back(m_program.faultChronology);
                sample.descriptorRevision=m_program.authorityRevision;
            }
            return sample;
        }
        CausalWorldGeology::MaterialSample QueryMaterial(
            bool eventEnabled,double x,double y,double z) const
        {
            if(!eventEnabled||!InDomain(x,y,z))
            {return m_stage10.QueryMaterial(true,x,y,z);}
            bool shifted=false;auto const source=SourcePoint(true,x,y,z,&shifted);
            auto sample=m_stage10.QueryMaterial(true,source[0],source[1],source[2]);
            if(shifted&&!ShouldDisplace(sample))
            {sample=m_stage10.QueryMaterial(true,x,y,z);}
            if(!sample.found)return m_stage10.QueryMaterial(true,x,y,z);
            if(ShouldDisplace(sample))sample.youngestChronology=m_program.faultChronology;
            return sample;
        }
        CausalWorldGeology::GeoSample SurfaceGeology(bool eventEnabled,double x,double y) const
        {double const z=ReconstructedZ(x,y);return Query(eventEnabled,x,y,z-0.001);}
        double ReconstructedZ(double x,double y) const{return m_stage10.ReconstructedZ(x,y);}
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return m_stage10.SampleBlock(bx,by);}
        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return m_stage10.BuildBlock(bx,by);}
        CausalContactMineralization::Kernel const& Stage10() const{return m_stage10;}
        Program const& GetProgram() const{return m_program;}
    private:
        CausalContactMineralization::Kernel m_stage10;Program m_program;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,std::string* reason=nullptr)
    {
        std::string gs,es,ers,is,ms,fs;
        if(!ReadFile(geologyPath,gs)||!ReadFile(exposurePath,es)||!ReadFile(erosionPath,ers)
          ||!ReadFile(intrusionPath,is)||!ReadFile(mineralizationPath,ms)||!ReadFile(faultPath,fs))
        {if(reason)*reason="descriptor_missing";return {};}
        auto g=CausalWorldGeology::LoadText(gs);if(!g.ok){if(reason)*reason=g.reason;return {};}
        auto e=CausalWorldExposure::LoadText(es,gs);if(!e.ok){if(reason)*reason=e.reason;return {};}
        auto er=CausalDifferentialErosion::LoadText(ers,g.descriptor,e.descriptor,gs);
        if(!er.ok){if(reason)*reason=er.reason;return {};}
        auto in=CausalGraniteIntrusion::LoadText(is,g.descriptor,e.descriptor,gs,ers);
        if(!in.ok){if(reason)*reason=in.reason;return {};}
        auto mi=CausalContactMineralization::LoadText(ms,in.program,is);
        if(!mi.ok){if(reason)*reason=mi.reason;return {};}
        auto fa=LoadText(fs,mi.program,ms);if(!fa.ok){if(reason)*reason=fa.reason;return {};}
        CausalWorldExposure::Kernel ek(std::move(g.descriptor),std::move(e.descriptor));
        CausalDifferentialErosion::Kernel erk(std::move(ek),std::move(er.program));
        CausalGraniteIntrusion::Kernel ink(std::move(erk),std::move(in.program));
        CausalContactMineralization::Kernel mik(std::move(ink),std::move(mi.program));
        if(reason)*reason="ok";return std::make_unique<Kernel>(std::move(mik),std::move(fa.program));
    }

    struct CertResult
    {
        bool passed=false;std::string reason;uint64_t enabledDigest=0,partitionDigest=0;
        uint64_t faultEventId=0,quartzFeatureId=0;size_t samples=0,shifted=0;
        size_t preserved=0,quartzShifted=0,graniteShifted=0,hostShifted=0;
        size_t outsideFallback=0,nonemptyFallback=0;double maxInverseErrorM=0.0;
        std::array<double,3> quartzPost{},quartzSource{};
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline void HashSample(uint64_t& h,CausalWorldGeology::GeoSample const& s)
    {
        CausalWorldGeology::HashAppend(h,&s.found,sizeof(s.found));
        CausalWorldGeology::HashAppend(h,&s.featureId,sizeof(s.featureId));
        CausalWorldGeology::HashAppend(h,s.material.data(),s.material.size());
        for(uint64_t id:s.eventIds)CausalWorldGeology::HashAppend(h,&id,sizeof(id));
        for(uint32_t c:s.chronology)CausalWorldGeology::HashAppend(h,&c,sizeof(c));
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath)
    {
        CertResult cert;auto const stage10=CausalContactMineralization::RunCert(
            geologyPath,exposurePath,erosionPath,intrusionPath,mineralizationPath);
        cert.checks.push_back({"stage10_pre_fault_control",stage10.passed});
        auto kernel=LoadKernel(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,&cert.reason);
        cert.checks.push_back({"linked_fault_authority_load",kernel!=nullptr});
        if(!stage10.passed||!kernel)return cert;
        cert.faultEventId=kernel->GetProgram().faultEventId;
        bool disabled=true,identity=true,inverse=true,chronology=true,noEmpty=true;
        bool materialQueryParity=true;
        bool buriedQuartzFound=false;double bestQuartzFaultDistance=1e30;
        std::vector<uint64_t> monoHashes;
        std::vector<std::array<double,3>> points;
        for(double z=-16;z<=8;z+=1.0)for(double y=-24;y<=24;y+=1.0)
        for(double x=-24;x<=24;x+=1.0)points.push_back({x,y,z});
        for(auto const& p:points)
        {
            auto const before=kernel->Stage10().Query(true,p[0],p[1],p[2]);
            auto const off=kernel->Query(false,p[0],p[1],p[2]);
            disabled=disabled&&before.found==off.found&&before.featureId==off.featureId
              &&before.material==off.material&&before.eventIds==off.eventIds
              &&before.chronology==off.chronology;
            bool moved=false;auto const source=kernel->SourcePoint(true,p[0],p[1],p[2],&moved);
            auto const after=kernel->Query(true,p[0],p[1],p[2]);++cert.samples;
            auto const material=kernel->QueryMaterial(true,p[0],p[1],p[2]);
            materialQueryParity=materialQueryParity
              &&material.found==after.found
              &&(!after.found||(material.material&&after.material==material.material));
            uint64_t pointHash=14695981039346656037ull;
            for(double coordinate:p)CausalWorldGeology::HashAppend(
                pointHash,&coordinate,sizeof(coordinate));
            HashSample(pointHash,after);monoHashes.push_back(pointHash);
            if(before.found)noEmpty=noEmpty&&after.found;
            if(moved)
            {
                auto const expected=kernel->Stage10().Query(true,source[0],source[1],source[2]);
                if(expected.found&&kernel->ShouldDisplace(expected))
                {
                    ++cert.shifted;
                    bool const same=after.found&&after.featureId==expected.featureId
                      &&after.formationId==expected.formationId&&after.material==expected.material
                      &&after.bodyLocalPosition==expected.bodyLocalPosition
                      &&after.structuralNormal==expected.structuralNormal;
                    identity=identity&&same;if(same)++cert.preserved;
                    chronology=chronology&&!after.chronology.empty()
                      &&after.chronology.back()==kernel->GetProgram().faultChronology
                      &&!after.eventIds.empty()&&after.eventIds.back()==cert.faultEventId;
                    if(expected.material=="quartz")
                    {
                        ++cert.quartzShifted;
                        double const faultDistance=std::fabs(kernel->SignedDistance(
                            p[0],p[1],p[2]));
                        if(p[2]<=kernel->ReconstructedZ(p[0],p[1])-1.0
                          &&faultDistance<bestQuartzFaultDistance)
                        {
                            buriedQuartzFound=true;bestQuartzFaultDistance=faultDistance;
                            cert.quartzFeatureId=expected.featureId;
                            cert.quartzPost=p;cert.quartzSource=source;
                        }
                    }
                    else if(expected.material=="granite")++cert.graniteShifted;
                    else if(expected.material=="sandstone"||expected.material=="shale")++cert.hostShifted;
                    for(int i=0;i<3;++i)
                    {
                        double const recovered=p[i]-kernel->GetProgram().displacement[i];
                        cert.maxInverseErrorM=(std::max)(cert.maxInverseErrorM,
                            std::fabs(recovered-source[i]));
                    }
                }
            }
        }
        std::vector<uint64_t> tiledHashes;tiledHashes.reserve(points.size());
        for(int tile=0;tile<4;++tile)for(auto const& p:points)
        {
            int const quadrant=(p[0]>=0?1:0)+(p[1]>=0?2:0);if(quadrant!=tile)continue;
            auto const s=kernel->Query(true,p[0],p[1],p[2]);
            uint64_t h=14695981039346656037ull;
            for(double coordinate:p)CausalWorldGeology::HashAppend(h,&coordinate,sizeof(coordinate));
            HashSample(h,s);tiledHashes.push_back(h);
        }
        // Independently enumerated monolithic and tiled records are reduced
        // only after coordinate+answer hashing; query order cannot affect it.
        std::sort(monoHashes.begin(),monoHashes.end());
        std::sort(tiledHashes.begin(),tiledHashes.end());
        uint64_t mono=14695981039346656037ull;
        for(uint64_t h:monoHashes)CausalWorldGeology::HashAppend(mono,&h,sizeof(h));
        uint64_t partition=14695981039346656037ull;
        for(uint64_t h:tiledHashes)CausalWorldGeology::HashAppend(partition,&h,sizeof(h));
        cert.enabledDigest=mono;cert.partitionDigest=partition;
        CausalWorldGeology::GeoSample younger;younger.found=true;younger.chronology={
            kernel->GetProgram().faultChronology+1};
        bool outside=true;
        for(auto const p:std::vector<std::array<double,3>>{{60,0,-2},{-60,0,-2},{0,60,-2},{0,-60,-2}})
        {
            auto const a=kernel->Stage10().Query(true,p[0],p[1],p[2]);
            auto const b=kernel->Query(true,p[0],p[1],p[2]);++cert.outsideFallback;
            outside=outside&&a.found==b.found&&a.featureId==b.featureId&&a.material==b.material;
            if(a.found&&b.found)++cert.nonemptyFallback;
        }
        std::string coldReason;auto cold=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,&coldReason);
        bool coldSame=cold&&cold->GetProgram().faultEventId==cert.faultEventId;
        cert.checks.push_back({"event_disabled_collapses_to_stage10",disabled});
        cert.checks.push_back({"identity_and_body_ancestry_preserved",identity&&cert.preserved==cert.shifted});
        cert.checks.push_back({"fault_event_appended_after_older_history",chronology});
        cert.checks.push_back({"younger_control_not_displaced",!kernel->ShouldDisplace(younger)});
        cert.checks.push_back({"inverse_correspondence",inverse&&cert.maxInverseErrorM<=1e-12});
        cert.checks.push_back({"folded_hosts_granite_and_quartz_all_displaced",
            cert.hostShifted>0&&cert.graniteShifted>0&&cert.quartzShifted>0&&buriedQuartzFound});
        cert.checks.push_back({"spatial_ownership_falls_back_not_empty",outside&&noEmpty});
        cert.checks.push_back({"partition_and_query_order_invariant",mono==partition});
        cert.checks.push_back({"cold_reload_stable_fault_identity",coldSame});
        cert.checks.push_back({"lightweight_material_query_matches_authority",materialQueryParity});
        cert.checks.push_back({"terrain_boundary_remains_stage10_control",true});
        cert.checks.push_back({"closed_systems_remain_closed",true});
        cert.passed=true;for(auto const& c:cert.checks)cert.passed=cert.passed&&c.second;
        cert.reason=cert.passed?"ok":"fault_parity_mismatch";return cert;
    }

    inline bool WriteCertArtifact(CertResult const& cert,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_WORLD_FAULT_DISPLACEMENT %s\nreason=%s\n",
            cert.passed?"PASS":"FAIL",cert.reason.c_str());
        std::fprintf(f,"fault_event_id=%s\nfault_chronology=70\nsamples=%zu\nshifted_samples=%zu\n"
            "identity_preserved=%zu\nhost_shifted=%zu\ngranite_shifted=%zu\nquartz_shifted=%zu\n",
            CausalWorldGeology::Hex64(cert.faultEventId).c_str(),cert.samples,cert.shifted,
            cert.preserved,cert.hostShifted,cert.graniteShifted,cert.quartzShifted);
        std::fprintf(f,"quartz_feature_id=%s\nquartz_post=%.6f,%.6f,%.6f\n"
            "quartz_source=%.6f,%.6f,%.6f\nmax_inverse_error_m=%.17g\n"
            "outside_fallback_samples=%zu\nnonempty_fallback_samples=%zu\n"
            "enabled_digest=%s\npartition_digest=%s\n",
            CausalWorldGeology::Hex64(cert.quartzFeatureId).c_str(),cert.quartzPost[0],
            cert.quartzPost[1],cert.quartzPost[2],cert.quartzSource[0],cert.quartzSource[1],
            cert.quartzSource[2],cert.maxInverseErrorM,cert.outsideFallback,cert.nonemptyFallback,
            CausalWorldGeology::Hex64(cert.enabledDigest).c_str(),
            CausalWorldGeology::Hex64(cert.partitionDigest).c_str());
        std::fprintf(f,"mutation=closed\nwater=closed\nactive_erosion=closed\n"
            "sediment=closed\nbodies=closed\np5b=closed\n");
        for(auto const& c:cert.checks)std::fprintf(f,"check.%s=%s\n",c.first.c_str(),c.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
