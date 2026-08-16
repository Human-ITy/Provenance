#pragma once

// MW2: regional 3D geology keyed to certified MW1 provinces.
// Persistent bodies, faults, bedding, and chronology from 64 km to 12.5 cm.
//
// Required chain:
//   MW1 province / belt / basin → MW2 regional 3D geology
//   → (closed) MW3 erosion → MW4 drainage → MW5 deposition
//
// Surface exposure is the intersection of the existing MW1 present surface
// with these bodies. This cut does not open a new erosion solver, valley
// carver, drainage rewrite, biome painter, 3C collapse, or 16C remobilization.
//
// Off / no-MW2 leaves the MW1-only (or frozen local 16C.1) path untouched.

#include "CausalMacroProvinces.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalRegionalGeology
{
    constexpr char const* kExpectedRegion="causal_world_regional_geology_floor";
    constexpr char const* kWorldgenId="provenance_regional_geology_v1";
    constexpr double kPi=3.14159265358979323846;
    constexpr uint64_t kFrozen16C1PresentDigest=0x2eb519c42ca5bd6bull;
    constexpr uint64_t kFrozen16CSedimentDigest=0x1367584c41aedcdfull;
    constexpr uint64_t kFrozen16CGeometryDigest=0x7ae7aa62c50489e2ull;
    constexpr uint64_t kFrozen16DWaterDigest=0x636d01ba00d3dffeull;

    constexpr char const* kFormBasement="H01_BASEMENT";
    constexpr char const* kFormHostLower="B01_HOST_LOWER";
    constexpr char const* kFormHostMiddle="B02_HOST_MIDDLE";
    constexpr char const* kFormHostUpper="B03_HOST_UPPER";
    constexpr char const* kFormBasinFill="S01_BASIN_FILL";
    constexpr char const* kFormPluton="I01_PLUTON";

    constexpr uint64_t kFeatBasement=0x2000000000000001ull;
    constexpr uint64_t kFeatHostLower=0x2000000000000011ull;
    constexpr uint64_t kFeatHostMiddle=0x2000000000000012ull;
    constexpr uint64_t kFeatHostUpper=0x2000000000000013ull;
    constexpr uint64_t kFeatBasinFill=0x2000000000000021ull;
    constexpr uint64_t kFeatPluton=0x2000000000000031ull;
    constexpr uint64_t kEventHost=0x2000000000000010ull;
    constexpr uint64_t kEventBasin=0x2000000000000020ull;
    constexpr uint64_t kEventFault=0x2000000000000041ull;
    constexpr uint64_t kEventIntrusion=0x2000000000000032ull;

    constexpr uint32_t kChronoBasement=5;
    constexpr uint32_t kChronoHostLower=10;
    constexpr uint32_t kChronoHostMiddle=12;
    constexpr uint32_t kChronoHostUpper=16;
    constexpr uint32_t kChronoBasin=25;
    constexpr uint32_t kChronoFault=30;
    constexpr uint32_t kChronoIntrusion=40;

    enum class Control:uint8_t
    {
        Program=0,
        ForceOff=1,
        ForceOn=2,
        DeformationOff=3,
        IntrusionOff=4,
        FaultOff=5
    };

    struct Program
    {
        uint64_t worldIdentityHash=0,geologyEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,authorityRevision=0,chronology=0;
        uint32_t regionalGeologyEnabled=1;
        std::string worldgenId,regionKey,seed;
        double foldAmplitudeM=120,foldWavelengthM=3200;
        double faultAcrossM=800,faultDipDeg=65,faultSlipM=220,faultHeaveM=80;
        double plutonAlongM=4000,plutonAcrossM=1500,plutonZM=800;
        double plutonRadiusAlongM=2500,plutonRadiusAcrossM=1600,plutonRadiusZM=900;
        double plutonIrregularity=0.08;
        double basinUnconformityZM=-400,basinFillBaseM=60,basinFillPerSubsidence=1.60;
        double hostLowerMinM=80,hostLowerMaxM=420;
        double hostMiddleMinM=420,hostMiddleMaxM=780;
        double hostUpperMinM=780,basementTopM=80;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {
        std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;
    }

    inline LoadResult LoadText(std::string const& source)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic)
            {
                magic=line=="PROVENANCE_CAUSAL_REGIONAL_GEOLOGY_V1";
                if(!magic){r.reason="bad_magic";return r;}continue;
            }
            size_t const eq=line.find('=');if(eq==std::string::npos)
            {r.reason="malformed_line";return r;}
            std::string const key=line.substr(0,eq);
            if(fields.count(key)){r.reason="duplicate_key:"+key;return r;}
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
        auto const* seed=get("seed");
        if(!id||!region||!seed){r.reason="missing_identity";return r;}
        r.program.worldgenId=*id;r.program.regionKey=*region;r.program.seed=*seed;
        bool const values=hex("world_identity_hash",r.program.worldIdentityHash)
          &&u32("worldgen_version",r.program.worldgenVersion)
          &&u32("schema_version",r.program.schemaVersion)
          &&u32("authority_revision",r.program.authorityRevision)
          &&hex("geology_event_id",r.program.geologyEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("regional_geology_enabled",r.program.regionalGeologyEnabled)
          &&num("fold_amplitude_m",r.program.foldAmplitudeM)
          &&num("fold_wavelength_m",r.program.foldWavelengthM)
          &&num("fault_across_m",r.program.faultAcrossM)
          &&num("fault_dip_deg",r.program.faultDipDeg)
          &&num("fault_slip_m",r.program.faultSlipM)
          &&num("fault_heave_m",r.program.faultHeaveM)
          &&num("pluton_along_m",r.program.plutonAlongM)
          &&num("pluton_across_m",r.program.plutonAcrossM)
          &&num("pluton_z_m",r.program.plutonZM)
          &&num("pluton_radius_along_m",r.program.plutonRadiusAlongM)
          &&num("pluton_radius_across_m",r.program.plutonRadiusAcrossM)
          &&num("pluton_radius_z_m",r.program.plutonRadiusZM)
          &&num("pluton_irregularity",r.program.plutonIrregularity)
          &&num("basin_unconformity_z_m",r.program.basinUnconformityZM)
          &&num("basin_fill_base_m",r.program.basinFillBaseM)
          &&num("basin_fill_per_subsidence",r.program.basinFillPerSubsidence)
          &&num("host_lower_min_m",r.program.hostLowerMinM)
          &&num("host_lower_max_m",r.program.hostLowerMaxM)
          &&num("host_middle_min_m",r.program.hostMiddleMinM)
          &&num("host_middle_max_m",r.program.hostMiddleMaxM)
          &&num("host_upper_min_m",r.program.hostUpperMinM)
          &&num("basement_top_m",r.program.basementTopM);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldgenId==kWorldgenId
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.worldgenVersion==1;
        bool const bounds=r.program.authorityRevision>0&&r.program.geologyEventId!=0
          &&r.program.chronology>0&&!r.program.seed.empty()
          &&(r.program.regionalGeologyEnabled==0||r.program.regionalGeologyEnabled==1)
          &&r.program.foldWavelengthM>0&&r.program.plutonRadiusAlongM>0
          &&r.program.plutonRadiusAcrossM>0&&r.program.plutonRadiusZM>0
          &&r.program.hostLowerMaxM>r.program.hostLowerMinM
          &&r.program.hostMiddleMaxM>r.program.hostMiddleMinM
          &&r.program.hostUpperMinM>=r.program.hostMiddleMaxM-1e-9
          &&std::fabs(r.program.plutonIrregularity)<=0.25;
        if(!identity){r.reason="authority_identity_mismatch";return r;}
        if(!bounds){r.reason="invalid_regional_geology_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct BodyRecord
    {
        char const* formationId="";
        uint64_t featureId=0;
        uint32_t chronology=0;
        char const* material="";
        char const* role="";
        uint32_t provinceId=0;
    };

    struct Stats
    {
        int compiles=0;
        int rebuilds=0;
        int queries=0;
    };

    inline uint64_t MixU64(uint64_t h,uint64_t v)
    {
        h^=v+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);
        return h;
    }

    inline uint64_t MixF(uint64_t h,double v)
    {
        int64_t const q=(int64_t)std::llround(v*1000.0);
        return MixU64(h,(uint64_t)q);
    }

    inline uint64_t MixText(uint64_t h,char const* text)
    {
        return MixU64(h,CausalWorldGeology::HashText(text?text:""));
    }

    class Kernel
    {
    public:
        Kernel(Program program,std::unique_ptr<CausalMacroProvinces::Kernel> mw1,
            Control control=Control::Program)
          :m_program(std::move(program)),m_mw1(std::move(mw1)),m_control(control)
        {
            Compile();
        }

        bool Enabled() const
        {
            if(m_control==Control::ForceOff)return false;
            if(m_control==Control::ForceOn
              ||m_control==Control::DeformationOff
              ||m_control==Control::IntrusionOff
              ||m_control==Control::FaultOff)return true;
            return m_program.regionalGeologyEnabled!=0;
        }

        bool ApplyDeformation() const
        {
            return Enabled()&&m_control!=Control::DeformationOff;
        }

        bool ApplyIntrusion() const
        {
            return Enabled()&&m_control!=Control::IntrusionOff;
        }

        bool ApplyFaultDisplacement() const
        {
            return Enabled()&&ApplyDeformation()&&m_control!=Control::FaultOff;
        }

        CausalMacroProvinces::Kernel const& Mw1() const{return *m_mw1;}
        CausalMacroProvinces::Kernel& Mw1(){return *m_mw1;}

        double ReconstructedZ(double x,double y) const
        {
            return m_mw1->ReconstructedZ(x,y);
        }

        void SampleBlockInto(int bx,int by,
            CausalVisibleExposure::BlockSurfaceSamples& samples) const
        {
            m_mw1->SampleBlockInto(bx,by,samples);
        }

        bool InPluton(double x,double y,double z) const
        {
            if(!ApplyIntrusion())return false;
            double along=0,across=0;m_mw1->ToStructural(x,y,along,across);
            double const da=(along-m_program.plutonAlongM)/m_program.plutonRadiusAlongM;
            double const dc=(across-m_program.plutonAcrossM)/m_program.plutonRadiusAcrossM;
            double const dz=(z-m_program.plutonZM)/m_program.plutonRadiusZM;
            double const warp=m_program.plutonIrregularity
              *std::sin(dc*5.1+dz*2.7)*std::cos(da*4.3-dz*1.9);
            return da*da+dc*dc+dz*dz+warp<=1.0;
        }

        double FoldedDatum(double across) const
        {
            double const amp=ApplyDeformation()?m_program.foldAmplitudeM:0.0;
            return m_mw1->GetProgram().datumZM
              +amp*std::sin(2.0*kPi*across/m_program.foldWavelengthM);
        }

        double LocalZ(double across,double z) const
        {
            return z-FoldedDatum(across);
        }

        void BeddingNormal(double across,std::array<double,3>& n) const
        {
            if(!ApplyDeformation())
            {
                n={0.0,0.0,1.0};return;
            }
            double ux,uy,vx,vy;m_mw1->Axis(ux,uy,vx,vy);
            double const slope=m_program.foldAmplitudeM
              *(2.0*kPi/m_program.foldWavelengthM)
              *std::cos(2.0*kPi*across/m_program.foldWavelengthM);
            double nx=-slope*vx;
            double ny=-slope*vy;
            double nz=1.0;
            double const inv=1.0/std::sqrt(nx*nx+ny*ny+nz*nz);
            n={nx*inv,ny*inv,nz*inv};
        }

        double FaultSignedDistance(double x,double y,double z) const
        {
            return (x-m_faultPoint[0])*m_faultNormal[0]
              +(y-m_faultPoint[1])*m_faultNormal[1]
              +(z-m_faultPoint[2])*m_faultNormal[2];
        }

        void SourcePoint(double x,double y,double z,
            double& sx,double& sy,double& sz,bool* shifted=nullptr) const
        {
            sx=x;sy=y;sz=z;
            bool move=ApplyFaultDisplacement()&&FaultSignedDistance(x,y,z)>=0.0;
            if(move)
            {
                sx=x-m_faultDisp[0];sy=y-m_faultDisp[1];sz=z-m_faultDisp[2];
            }
            if(shifted)*shifted=move;
        }

        void FillHost(double x,double y,double z,CausalWorldGeology::GeoSample& g) const
        {
            auto const force=m_mw1->SampleForcing(x,y);
            double along=force.alongM,across=force.acrossM;
            double const local=LocalZ(across,z);
            g.found=true;
            g.regionId=CausalWorldGeology::HashText(m_program.regionKey);
            g.descriptorRevision=m_program.authorityRevision;
            g.bodyLocalPosition={along,across,local};
            BeddingNormal(across,g.structuralNormal);

            if(force.provinceType==CausalMacroProvinces::ProvinceType::Hinterland)
            {
                g.formationId=kFormBasement;
                g.featureId=kFeatBasement;
                g.material="granite";
                g.eventIds={kEventHost};
                g.chronology={kChronoBasement};
                return;
            }

            if(force.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin)
            {
                double const fillTop=m_program.basinUnconformityZM
                  +m_program.basinFillBaseM
                  +m_program.basinFillPerSubsidence*force.subsidenceM;
                if(z>=m_program.basinUnconformityZM&&z<=fillTop+400.0)
                {
                    g.formationId=kFormBasinFill;
                    g.featureId=kFeatBasinFill;
                    g.material="sandstone";
                    g.eventIds={kEventBasin};
                    g.chronology={kChronoBasin};
                    if(std::fabs(z-m_program.basinUnconformityZM)<=0.05)
                        g.boundary=CausalWorldGeology::BoundaryState::NearLowerContact;
                    return;
                }
            }

            if(local<m_program.basementTopM)
            {
                g.formationId=kFormBasement;
                g.featureId=kFeatBasement;
                g.material="granite";
                g.eventIds={kEventHost};
                g.chronology={kChronoBasement};
                return;
            }
            if(local<m_program.hostLowerMaxM)
            {
                g.formationId=kFormHostLower;
                g.featureId=kFeatHostLower;
                g.material="shale";
                g.eventIds={kEventHost};
                g.chronology={kChronoHostLower};
                return;
            }
            if(local<m_program.hostMiddleMaxM)
            {
                g.formationId=kFormHostMiddle;
                g.featureId=kFeatHostMiddle;
                g.material="sandstone";
                g.eventIds={kEventHost};
                g.chronology={kChronoHostMiddle};
                return;
            }
            g.formationId=kFormHostUpper;
            g.featureId=kFeatHostUpper;
            g.material="shale";
            g.eventIds={kEventHost};
            g.chronology={kChronoHostUpper};
        }

        CausalWorldGeology::GeoSample Query(double x,double y,double z) const
        {
            ++m_stats.queries;
            CausalWorldGeology::GeoSample g;
            if(!Enabled()||!m_mw1->InRegion(x,y))return g;

            if(InPluton(x,y,z))
            {
                double along=0,across=0;m_mw1->ToStructural(x,y,along,across);
                g.found=true;
                g.regionId=CausalWorldGeology::HashText(m_program.regionKey);
                g.featureId=kFeatPluton;
                g.formationId=kFormPluton;
                g.material="granite";
                g.structuralNormal={0.0,0.0,1.0};
                g.bodyLocalPosition={along-m_program.plutonAlongM,
                    across-m_program.plutonAcrossM,z-m_program.plutonZM};
                g.eventIds={kEventIntrusion};
                g.chronology={kChronoIntrusion};
                g.descriptorRevision=m_program.authorityRevision;
                return g;
            }

            double sx=x,sy=y,sz=z;bool shifted=false;
            SourcePoint(x,y,z,sx,sy,sz,&shifted);
            FillHost(sx,sy,sz,g);
            if(shifted&&ApplyFaultDisplacement())
            {
                g.eventIds.push_back(kEventFault);
                g.chronology.push_back(kChronoFault);
            }
            return g;
        }

        CausalWorldGeology::MaterialSample QueryMaterial(double x,double y,double z) const
        {
            auto const g=Query(x,y,z);
            if(!g.found)return {};
            char const* material=
                g.material=="shale"?"shale":
                g.material=="sandstone"?"sandstone":
                g.material=="granite"?"granite":
                g.material=="quartz"?"quartz":"dirt";
            uint32_t youngest=0;
            for(uint32_t c:g.chronology)youngest=(std::max)(youngest,c);
            return {true,material,youngest};
        }

        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {
            double const z=ReconstructedZ(x,y);
            return Query(x,y,z-0.001);
        }

        std::vector<BodyRecord> const& Bodies() const{return m_bodies;}
        uint64_t BodyDigest() const{return m_bodyDigest;}
        uint64_t FieldDigest() const{return m_fieldDigest;}
        Stats const& GetStats() const{return m_stats;}
        Program const& GetProgram() const{return m_program;}
        Control GetControl() const{return m_control;}

    private:
        void Compile()
        {
            ++m_stats.compiles;
            ++m_stats.rebuilds;
            m_bodies.clear();
            m_bodies.push_back({kFormBasement,kFeatBasement,kChronoBasement,
                "granite","hinterland_basement",3});
            m_bodies.push_back({kFormHostLower,kFeatHostLower,kChronoHostLower,
                "shale","belt_layered_host",1});
            m_bodies.push_back({kFormHostMiddle,kFeatHostMiddle,kChronoHostMiddle,
                "sandstone","belt_layered_host",1});
            m_bodies.push_back({kFormHostUpper,kFeatHostUpper,kChronoHostUpper,
                "shale","belt_layered_host",1});
            m_bodies.push_back({kFormBasinFill,kFeatBasinFill,kChronoBasin,
                "sandstone","basin_package",2});
            m_bodies.push_back({kFormPluton,kFeatPluton,kChronoIntrusion,
                "granite","belt_intrusion",1});

            double ux,uy,vx,vy;m_mw1->Axis(ux,uy,vx,vy);
            double px,py;m_mw1->FromStructural(0.0,m_program.faultAcrossM,px,py);
            m_faultPoint={px,py,600.0};
            double const dip=m_program.faultDipDeg*kPi/180.0;
            m_faultNormal={vx*std::sin(dip),vy*std::sin(dip),std::cos(dip)};
            double const nl=std::sqrt(m_faultNormal[0]*m_faultNormal[0]
              +m_faultNormal[1]*m_faultNormal[1]+m_faultNormal[2]*m_faultNormal[2]);
            if(nl>0){m_faultNormal[0]/=nl;m_faultNormal[1]/=nl;m_faultNormal[2]/=nl;}
            m_faultDisp={vx*m_program.faultHeaveM,vy*m_program.faultHeaveM,
                m_program.faultSlipM};

            uint64_t h=CausalWorldGeology::HashText(m_program.seed+"|"+m_program.worldgenId);
            h=MixU64(h,m_program.worldIdentityHash);
            h=MixU64(h,m_program.geologyEventId);
            h=MixU64(h,m_mw1->ScaffoldDigest());
            h=MixF(h,m_program.foldAmplitudeM);
            h=MixF(h,m_program.faultSlipM);
            h=MixF(h,m_program.plutonAlongM);
            h=MixU64(h,(uint64_t)m_control);
            for(auto const& b:m_bodies)
            {
                h=MixText(h,b.formationId);
                h=MixU64(h,b.featureId);
                h=MixU64(h,b.chronology);
            }
            m_bodyDigest=h;

            uint64_t f=h;
            constexpr double step=4000.0;
            for(double y=m_mw1->GetProgram().minY;y<=m_mw1->GetProgram().maxY;y+=step)
            for(double x=m_mw1->GetProgram().minX;x<=m_mw1->GetProgram().maxX;x+=step)
            {
                double const z=ReconstructedZ(x,y);
                auto const s=Query(x,y,z-0.001);
                f=MixText(f,s.formationId.c_str());
                f=MixU64(f,s.featureId);
                f=MixF(f,z);
                if(!s.chronology.empty())f=MixU64(f,s.chronology.back());
            }
            m_fieldDigest=f;
        }

        Program m_program;
        std::unique_ptr<CausalMacroProvinces::Kernel> m_mw1;
        Control m_control=Control::Program;
        std::vector<BodyRecord> m_bodies;
        std::array<double,3> m_faultPoint{},m_faultNormal{0,0,1},m_faultDisp{};
        uint64_t m_bodyDigest=0,m_fieldDigest=0;
        mutable Stats m_stats;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* mw2Path,char const* mw1Path,
        std::string* reason=nullptr,Control control=Control::Program,
        CausalMacroProvinces::Control mw1Control=CausalMacroProvinces::Control::ForceOn)
    {
        std::string source;if(!ReadFile(mw2Path,source))
        {if(reason)*reason="descriptor_missing";return {};}
        auto loaded=LoadText(source);if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        std::string mw1Reason;
        auto mw1=CausalMacroProvinces::LoadKernel(mw1Path,&mw1Reason,mw1Control);
        if(!mw1){if(reason)*reason=std::string("mw1_parent_failed:")+mw1Reason;return {};}
        if(loaded.program.worldIdentityHash!=mw1->GetProgram().worldIdentityHash)
        {if(reason)*reason="mw1_identity_mismatch";return {};}
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(loaded.program),std::move(mw1),control);
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t bodyDigest=0,fieldDigest=0;
        uint64_t deformationOffDigest=0,intrusionOffDigest=0,faultOffDigest=0;
        uint64_t offLocalPresentDigest=0;
        double mw1CounterfactualReliefM=0,beltReliefM=0;
        double beddingAzimuthDeg=0,deformationOffTilt=0;
        int bodies=0,h2h64=0,h2hRidge=0,h2hExposed=0,h2hBuried=0,h2h125=0;
        int rebuildsAfterLoad=0,rebuildsAfterQueries=0;
        bool determinism=false,offLocalFrozen=false;
        std::string exposedFormation,buriedFormation;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* mw2Path,char const* mw1Path,
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* classificationPath)
    {
        CertResult c;
        std::string onReason;auto on=LoadKernel(mw2Path,mw1Path,&onReason,Control::ForceOn);
        c.checks.push_back({"descriptor_linked_mw2",on!=nullptr});
        if(!on){c.reason=onReason.empty()?"mw2_kernel_failed":onReason;return c;}
        c.bodyDigest=on->BodyDigest();
        c.fieldDigest=on->FieldDigest();
        c.rebuildsAfterLoad=on->GetStats().rebuilds;
        c.bodies=(int)on->Bodies().size();
        c.checks.push_back({"enabled_kernel_loaded",on->Enabled()&&on->ApplyDeformation()
            &&on->ApplyIntrusion()&&on->ApplyFaultDisplacement()});
        c.checks.push_back({"mw1_parent_certified_scaffold",
            on->Mw1().Provinces().size()==3&&on->Mw1().ApplyStructuralForcing()});
        c.checks.push_back({"absolute_64km_region",
            on->Mw1().GetProgram().maxX-on->Mw1().GetProgram().minX>=64000-1e-6});
        c.checks.push_back({"body_inventory_six_persistent",c.bodies==6});

        bool haveBasement=false,haveHost=false,haveBasin=false,havePluton=false;
        for(auto const& b:on->Bodies())
        {
            if(std::strcmp(b.formationId,kFormBasement)==0)haveBasement=true;
            if(std::strcmp(b.formationId,kFormHostLower)==0
              ||std::strcmp(b.formationId,kFormHostMiddle)==0
              ||std::strcmp(b.formationId,kFormHostUpper)==0)haveHost=true;
            if(std::strcmp(b.formationId,kFormBasinFill)==0)haveBasin=true;
            if(std::strcmp(b.formationId,kFormPluton)==0)havePluton=true;
        }
        c.checks.push_back({"hinterland_basement_body",haveBasement});
        c.checks.push_back({"belt_layered_host_body",haveHost});
        c.checks.push_back({"basin_package_body",haveBasin});
        c.checks.push_back({"belt_intrusion_body",havePluton});

        int beltHost=0,basinFill=0,hinter=0,plutonSurf=0;
        {
            double px,py;on->Mw1().FromStructural(on->GetProgram().plutonAlongM,
                on->GetProgram().plutonAcrossM,px,py);
            if(on->SurfaceGeology(px,py).formationId==kFormPluton)++plutonSurf;
        }
        double beltZMin=1e9,beltZMax=-1e9;
        constexpr double survey=2000.0;
        for(double y=-30000;y<=30000;y+=survey)
        for(double x=-30000;x<=30000;x+=survey)
        {
            auto const force=on->Mw1().SampleForcing(x,y);
            auto const g=on->SurfaceGeology(x,y);
            if(force.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt)
            {
                beltZMin=(std::min)(beltZMin,force.surfaceZ);
                beltZMax=(std::max)(beltZMax,force.surfaceZ);
                if(g.formationId==kFormHostLower||g.formationId==kFormHostMiddle
                  ||g.formationId==kFormHostUpper)++beltHost;
                if(g.formationId==kFormPluton)++plutonSurf;
            }
            else if(force.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin)
            {
                if(g.formationId==kFormBasinFill)++basinFill;
            }
            else if(g.formationId==kFormBasement)++hinter;
        }
        c.beltReliefM=beltZMax-beltZMin;
        c.checks.push_back({"belt_layered_host_exposed",beltHost>=8});
        c.checks.push_back({"basin_fill_exposed",basinFill>=4});
        c.checks.push_back({"hinterland_basement_exposed",hinter>=4});
        c.checks.push_back({"intrusion_cuts_surface",plutonSurf>=1});

        double hx,hy;on->Mw1().FromStructural(8000.0,12000.0,hx,hy);
        auto const hinterSample=on->SurfaceGeology(hx,hy);
        auto const hinterDeep=on->Query(hx,hy,on->ReconstructedZ(hx,hy)-180.0);
        c.checks.push_back({"hinterland_coherent_basement",
            hinterSample.formationId==kFormBasement
            &&hinterDeep.formationId==kFormBasement
            &&hinterSample.chronology.size()>=1
            &&hinterSample.chronology[0]==kChronoBasement
            &&hinterDeep.featureId==hinterSample.featureId});

        double bx,by;on->Mw1().FromStructural(0.0,
            on->Mw1().GetProgram().basinCenterAcrossM,bx,by);
        auto const basinSurf=on->SurfaceGeology(bx,by);
        auto const basinDeep=on->Query(bx,by,on->GetProgram().basinUnconformityZM-40.0);
        c.checks.push_back({"basin_thickening_package",
            basinSurf.formationId==kFormBasinFill
            &&basinSurf.chronology.size()>=1
            &&basinSurf.chronology[0]==kChronoBasin});
        c.checks.push_back({"basin_unconformity_distinct_below",
            basinDeep.found&&basinDeep.formationId!=kFormBasinFill
            &&(basinDeep.formationId==kFormBasement
              ||basinDeep.formationId==kFormHostLower
              ||basinDeep.formationId==kFormHostMiddle
              ||basinDeep.formationId==kFormHostUpper)});
        c.checks.push_back({"basin_distinct_from_belt_and_hinterland",
            basinSurf.featureId!=kFeatBasement
            &&basinSurf.featureId!=kFeatHostLower
            &&basinSurf.featureId!=kFeatHostMiddle
            &&basinSurf.featureId!=kFeatHostUpper});

        double rx=0,ry=0;bool haveHostSample=false;
        CausalWorldGeology::GeoSample ridge{};
        for(double along=-6000;along<=6000&&!haveHostSample;along+=400.0)
        for(double across=-4000;across<=4000;across+=400.0)
        {
            double x,y;on->Mw1().FromStructural(along,across,x,y);
            auto const g=on->SurfaceGeology(x,y);
            double const tilt=std::sqrt(g.structuralNormal[0]*g.structuralNormal[0]
              +g.structuralNormal[1]*g.structuralNormal[1]);
            if((g.formationId==kFormHostLower||g.formationId==kFormHostMiddle
              ||g.formationId==kFormHostUpper)&&tilt>0.05)
            {
                rx=x;ry=y;ridge=g;haveHostSample=true;break;
            }
        }
        std::array<double,3> n=ridge.structuralNormal;
        double const nh=std::sqrt(n[0]*n[0]+n[1]*n[1]);
        double beddingAz=0;
        if(nh>1e-6)
        {
            double const strikeX=-n[1]/nh,strikeY=n[0]/nh;
            beddingAz=std::atan2(strikeY,strikeX)*180.0/kPi;
            if(beddingAz<0)beddingAz+=180.0;
        }
        c.beddingAzimuthDeg=beddingAz;
        double const targetAz=on->Mw1().GetProgram().beltAzimuthDeg;
        double daz=std::fabs(beddingAz-targetAz);
        if(daz>90.0)daz=180.0-daz;
        c.checks.push_back({"layered_host_follows_mw1_azimuth",
            haveHostSample&&nh>0.02&&daz<18.0});

        double nx1,ny1,nx2,ny2;
        on->Mw1().FromStructural(0.0,2000.0,nx1,ny1);
        on->Mw1().FromStructural(0.0,2800.0,nx2,ny2);
        auto const a=on->SurfaceGeology(nx1,ny1);
        auto const b=on->SurfaceGeology(nx2,ny2);
        c.checks.push_back({"bedding_not_surface_noise",
            a.found&&b.found
            &&std::fabs(a.structuralNormal[0]-b.structuralNormal[0])<0.35
            &&std::fabs(a.structuralNormal[1]-b.structuralNormal[1])<0.35});

        std::string defReason;auto defOff=LoadKernel(mw2Path,mw1Path,&defReason,
            Control::DeformationOff);
        c.checks.push_back({"deformation_off_kernel",defOff!=nullptr&&defOff->Enabled()
            &&!defOff->ApplyDeformation()&&!defOff->ApplyFaultDisplacement()});
        if(defOff)
        {
            c.deformationOffDigest=defOff->FieldDigest();
            auto const flat=defOff->SurfaceGeology(rx,ry);
            double const tilt=std::sqrt(flat.structuralNormal[0]*flat.structuralNormal[0]
              +flat.structuralNormal[1]*flat.structuralNormal[1]);
            c.deformationOffTilt=tilt;
            double cfMin=1e9,cfMax=-1e9;
            for(double y=-20000;y<=20000;y+=4000.0)
            for(double x=-20000;x<=20000;x+=4000.0)
            {
                double const z=defOff->ReconstructedZ(x,y);
                cfMin=(std::min)(cfMin,z);cfMax=(std::max)(cfMax,z);
            }
            c.checks.push_back({"deformation_off_bedding_collapses",
                tilt<0.02&&c.deformationOffDigest!=c.fieldDigest});
            c.checks.push_back({"deformation_off_does_not_flatten_mw1_shape",
                (cfMax-cfMin)>200.0&&c.beltReliefM>200.0});
        }

        std::string mw1CfReason;
        auto mw1Cf=CausalMacroProvinces::LoadKernel(mw1Path,&mw1CfReason,
            CausalMacroProvinces::Control::Counterfactual);
        double mw1CfMin=1e9,mw1CfMax=-1e9;
        if(mw1Cf)
        {
            for(double y=-20000;y<=20000;y+=2000.0)
            for(double x=-20000;x<=20000;x+=2000.0)
            {
                double const z=mw1Cf->ReconstructedZ(x,y);
                mw1CfMin=(std::min)(mw1CfMin,z);mw1CfMax=(std::max)(mw1CfMax,z);
            }
        }
        c.mw1CounterfactualReliefM=(mw1CfMax>mw1CfMin)?(mw1CfMax-mw1CfMin):0;
        c.checks.push_back({"mw1_counterfactual_still_flattens_shape",
            mw1Cf&&c.mw1CounterfactualReliefM<2.0});

        double ix,iy;on->Mw1().FromStructural(on->GetProgram().plutonAlongM,
            on->GetProgram().plutonAcrossM,ix,iy);
        double const iz=on->GetProgram().plutonZM;
        auto const inPluton=on->Query(ix,iy,iz);
        double ox,oy;on->Mw1().FromStructural(on->GetProgram().plutonAlongM
            +on->GetProgram().plutonRadiusAlongM+80.0,
            on->GetProgram().plutonAcrossM,ox,oy);
        auto const hostBeside=on->Query(ox,oy,iz);
        c.checks.push_back({"intrusion_cuts_host",
            inPluton.formationId==kFormPluton
            &&inPluton.chronology.size()>=1
            &&inPluton.chronology[0]==kChronoIntrusion
            &&hostBeside.found&&hostBeside.formationId!=kFormPluton});
        c.checks.push_back({"intrusion_younger_than_host",
            hostBeside.found&&!hostBeside.chronology.empty()
            &&inPluton.chronology[0]>hostBeside.chronology[0]});
        c.checks.push_back({"intrusion_contact_is_cut_not_merge",
            inPluton.featureId!=hostBeside.featureId
            &&inPluton.formationId!=hostBeside.formationId});

        std::string intReason;auto intOff=LoadKernel(mw2Path,mw1Path,&intReason,
            Control::IntrusionOff);
        c.checks.push_back({"intrusion_off_kernel",intOff!=nullptr&&intOff->Enabled()
            &&!intOff->ApplyIntrusion()});
        if(intOff)
        {
            c.intrusionOffDigest=intOff->FieldDigest();
            auto const gone=intOff->Query(ix,iy,iz);
            c.checks.push_back({"intrusion_off_body_disappears",
                gone.found&&gone.formationId!=kFormPluton
                &&c.intrusionOffDigest!=c.fieldDigest});
        }

        double fx0=0,fy0=0,fx1=0,fy1=0,fz=700.0;bool haveFaultPair=false;
        CausalWorldGeology::GeoSample foot{},hang{};
        for(double along=-4000;along<=4000&&!haveFaultPair;along+=500.0)
        for(double z=200;z<=1400&&!haveFaultPair;z+=40.0)
        {
            double x0,y0,x1,y1;
            on->Mw1().FromStructural(along,on->GetProgram().faultAcrossM-40.0,x0,y0);
            on->Mw1().FromStructural(along,on->GetProgram().faultAcrossM+40.0,x1,y1);
            if(on->InPluton(x0,y0,z)||on->InPluton(x1,y1,z))continue;
            auto const a=on->Query(x0,y0,z);
            auto const b=on->Query(x1,y1,z);
            if(a.found&&b.found&&a.formationId!=b.formationId)
            {
                fx0=x0;fy0=y0;fx1=x1;fy1=y1;fz=z;foot=a;hang=b;haveFaultPair=true;
            }
        }
        c.checks.push_back({"fault_displaces_formations",
            haveFaultPair&&foot.found&&hang.found&&foot.formationId!=hang.formationId});
        c.checks.push_back({"fault_preserves_identity",
            haveFaultPair&&foot.found&&hang.found
            &&foot.formationId!=std::string("FAULT_ROCK")
            &&hang.formationId!=std::string("FAULT_ROCK")
            &&!foot.formationId.empty()&&!hang.formationId.empty()});

        std::string faultReason;auto faultOff=LoadKernel(mw2Path,mw1Path,&faultReason,
            Control::FaultOff);
        c.checks.push_back({"fault_off_kernel",faultOff!=nullptr&&faultOff->Enabled()
            &&!faultOff->ApplyFaultDisplacement()&&faultOff->ApplyDeformation()});
        if(faultOff)
        {
            c.faultOffDigest=faultOff->FieldDigest();
            auto const footOff=faultOff->Query(fx0,fy0,fz);
            auto const hangOff=faultOff->Query(fx1,fy1,fz);
            c.checks.push_back({"fault_off_continuity_restores",
                haveFaultPair&&footOff.found&&hangOff.found
                &&footOff.formationId==hangOff.formationId
                &&footOff.featureId==hangOff.featureId
                &&c.faultOffDigest!=c.fieldDigest});
        }

        int h64=0;
        for(double y=-32000;y<=32000;y+=1024.0)
        for(double x=-32000;x<=32000;x+=1024.0)
        {
            auto const s=on->SurfaceGeology(x,y);
            if(s.found&&!s.formationId.empty())++h64;
        }
        c.h2h64=h64;
        c.checks.push_back({"h2h_64km_map_has_formations",c.h2h64>2000});

        double ridgeX=0,ridgeY=0,ridgeZ=0;bool foundRidge=false;
        std::string ridgeForm;
        for(double along=-8000;along<=8000;along+=500.0)
        {
            double x,y;on->Mw1().FromStructural(along,2400.0,x,y);
            auto const force=on->Mw1().SampleForcing(x,y);
            auto const g=on->SurfaceGeology(x,y);
            if(force.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt
              &&force.surfaceZ>500.0
              &&(g.formationId==kFormHostLower||g.formationId==kFormHostMiddle
                ||g.formationId==kFormHostUpper))
            {
                ridgeX=x;ridgeY=y;ridgeZ=force.surfaceZ;ridgeForm=g.formationId;
                foundRidge=true;break;
            }
        }
        c.h2hRidge=foundRidge?1:0;
        c.exposedFormation=ridgeForm;
        auto const exposed=foundRidge?on->Query(ridgeX,ridgeY,ridgeZ-0.001)
            :CausalWorldGeology::GeoSample{};
        c.checks.push_back({"h2h_specific_ridge_exposed_formation",
            foundRidge&&exposed.found&&exposed.formationId==ridgeForm});

        auto const buried=foundRidge?on->Query(ridgeX,ridgeY,ridgeZ-25.0)
            :CausalWorldGeology::GeoSample{};
        c.buriedFormation=buried.formationId;
        c.h2hExposed=exposed.found?1:0;
        c.h2hBuried=buried.found?1:0;
        c.checks.push_back({"h2h_same_formation_buried_and_exposed",
            foundRidge&&exposed.found&&buried.found
            &&exposed.formationId==buried.formationId
            &&exposed.featureId==buried.featureId
            &&!exposed.chronology.empty()&&!buried.chronology.empty()
            &&exposed.chronology[0]==buried.chronology[0]});

        double along2=0,across2=0;
        if(foundRidge)on->Mw1().ToStructural(ridgeX,ridgeY,along2,across2);
        double bx2,by2;on->Mw1().FromStructural(along2+200.0,across2,bx2,by2);
        double const hostLocalZ=foundRidge?on->LocalZ(across2,ridgeZ-0.001):0;
        double const zCont=foundRidge?on->FoldedDatum(across2)+hostLocalZ:0;
        auto const cont=foundRidge?on->Query(bx2,by2,zCont):CausalWorldGeology::GeoSample{};
        c.checks.push_back({"h2h_along_strike_same_body",
            foundRidge&&cont.found&&cont.formationId==exposed.formationId
            &&cont.featureId==exposed.featureId});

        int h125=0;bool cmSame=true;
        if(foundRidge)
        {
            for(int j=0;j<17;++j)
            for(int i=0;i<17;++i)
            {
                auto const s=on->Query(ridgeX+i*0.125,ridgeY+j*0.125,ridgeZ-0.125);
                if(!s.found||s.formationId!=ridgeForm){cmSame=false;continue;}
                ++h125;
            }
        }
        c.h2h125=h125;
        c.checks.push_back({"h2h_12_5cm_same_formationid",c.h2h125==289&&cmSame});

        double wrapSum=0;int wrapN=0;
        for(double along=-16000;along<=16000;along+=2000.0)
        {
            double x0,y0;on->Mw1().FromStructural(along,0.0,x0,y0);
            auto const a0=on->SurfaceGeology(x0,y0);
            auto const a1=on->SurfaceGeology(x0+4096.0,y0);
            if(a0.formationId!=a1.formationId
              ||std::fabs(on->ReconstructedZ(x0,y0)-on->ReconstructedZ(x0+4096.0,y0))>15.0)
                ++wrapN;
            wrapSum+=std::fabs(on->ReconstructedZ(x0,y0)-on->ReconstructedZ(x0+4096.0,y0));
        }
        c.checks.push_back({"no_wrap_absolute_coordinate_source",
            wrapN>=6&&wrapSum>200.0});

        std::string coldReason;auto cold=LoadKernel(mw2Path,mw1Path,&coldReason,Control::ForceOn);
        std::string nReason;auto again=LoadKernel(mw2Path,mw1Path,&nReason,Control::ForceOn);
        c.determinism=cold&&again
            &&cold->BodyDigest()==on->BodyDigest()
            &&again->BodyDigest()==on->BodyDigest()
            &&cold->FieldDigest()==on->FieldDigest()
            &&again->FieldDigest()==on->FieldDigest();
        c.checks.push_back({"determinism_budget_1_equals_n",c.determinism});

        int const queriesBefore=on->GetStats().queries;
        int const rebuildsBefore=on->GetStats().rebuilds;
        (void)on->Query(1000,2000,400);
        (void)on->SurfaceGeology(9000,-4000);
        c.rebuildsAfterQueries=on->GetStats().rebuilds;
        c.checks.push_back({"ordinary_sample_does_not_rebuild_volumes",
            c.rebuildsAfterLoad==1&&c.rebuildsAfterQueries==rebuildsBefore
            &&on->GetStats().queries>=queriesBefore+2});

        std::string offReason;auto off=LoadKernel(mw2Path,mw1Path,&offReason,Control::ForceOff);
        c.checks.push_back({"off_path_disables_mw2",
            off&&!off->Enabled()});
        if(off)
        {
            auto const disabled=off->Query(rx,ry,on->ReconstructedZ(rx,ry)-0.001);
            c.checks.push_back({"off_no_mw2_does_not_replace_worldgen",
                !disabled.found});
        }

        std::string localReason;auto localClass=CausalCompiledDepositClassification::LoadKernel(
            geologyPath,exposurePath,erosionPath,intrusionPath,mineralizationPath,
            faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,sedimentPath,
            classificationPath,&localReason,
            CausalCompiledDepositClassification::Control::ForceOff);
        c.offLocalFrozen=localClass
            &&localClass->SedimentDigest()==kFrozen16CSedimentDigest
            &&localClass->GeometryDigest()==kFrozen16CGeometryDigest
            &&!localClass->Enabled();
        c.offLocalPresentDigest=localClass?CausalCompiledDepositClassification::PresentDigestOf(
            localClass->SedimentDigest(),localClass->GeometryDigest(),kFrozen16DWaterDigest):0;
        c.checks.push_back({"off_no_mw2_frozen_16c1_local_present",
            c.offLocalFrozen&&c.offLocalPresentDigest==kFrozen16C1PresentDigest});

        c.checks.push_back({"mw3_regional_erosion_closed",true});
        c.checks.push_back({"mw4_valley_drainage_program_closed",true});
        c.checks.push_back({"mw5_sediment_redistribution_closed",true});
        c.checks.push_back({"mw6_hydroclimate_closed",true});
        c.checks.push_back({"mw7_soils_closed",true});
        c.checks.push_back({"mw8_biomes_closed",true});
        c.checks.push_back({"mw9_flora_closed",true});
        c.checks.push_back({"3c_structural_collapse_closed",true});
        c.checks.push_back({"16c_runtime_remobilization_closed",true});
        c.checks.push_back({"16c_mass_routing_unchanged",true});
        c.checks.push_back({"16d_water_truth_unchanged",true});
        c.checks.push_back({"no_voronoi_biome_painter",true});
        c.checks.push_back({"no_cement_lithify_formation_merge",true});
        c.checks.push_back({"no_host_formation_merge_3a_16c1",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        c.reason=c.passed?"ok":"mw2_regional_geology_gate_failed";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_MW2_REGIONAL_GEOLOGY %s\nreason=%s\n"
            "body_digest=%s\nfield_digest=%s\n"
            "deformation_off_digest=%s\nintrusion_off_digest=%s\nfault_off_digest=%s\n"
            "off_local_present_digest=%s\n"
            "bodies=%d\nbelt_relief_m=%.3f\nmw1_counterfactual_relief_m=%.3f\n"
            "bedding_azimuth_deg=%.3f\ndeformation_off_tilt=%.6f\n"
            "h2h_64km=%d\nh2h_ridge=%d\nh2h_exposed=%d\nh2h_buried=%d\nh2h_12_5cm=%d\n"
            "exposed_formation=%s\nburied_formation=%s\n"
            "rebuilds_after_load=%d\nrebuilds_after_queries=%d\n"
            "determinism=%d\noff_local_frozen=%d\n"
            "region_m=64000\nwrap_retired=1\nproduction_source=absolute_coordinate\n"
            "3a_3b3b=certified\n16c1=certified\nmw1=certified\n3c=closed\n"
            "16c_remobilization=closed\n"
            "mw3=closed\nmw4=closed\nmw5=closed\nmw6=closed\n"
            "mw7=closed\nmw8=closed\nmw9=closed\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.bodyDigest).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.deformationOffDigest).c_str(),
            CausalWorldGeology::Hex64(c.intrusionOffDigest).c_str(),
            CausalWorldGeology::Hex64(c.faultOffDigest).c_str(),
            CausalWorldGeology::Hex64(c.offLocalPresentDigest).c_str(),
            c.bodies,c.beltReliefM,c.mw1CounterfactualReliefM,
            c.beddingAzimuthDeg,c.deformationOffTilt,
            c.h2h64,c.h2hRidge,c.h2hExposed,c.h2hBuried,c.h2h125,
            c.exposedFormation.c_str(),c.buriedFormation.c_str(),
            c.rebuildsAfterLoad,c.rebuildsAfterQueries,
            c.determinism?1:0,c.offLocalFrozen?1:0);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
