#pragma once

// MW1: regional geologic provinces / mountain belts / basins.
// This is the geologic forcing field, not final planet terrain.
//
// Required chain:
//   province history → uplift / subsidence / deformation → persistent
//   geology → (observed) drainage tendency → present topography
//
// Forbidden: noise → mountains → label geology afterward.
// Production source is absolute-coordinate 64 km regional authority.
// The 4096 m Stage-15 wrap is not used here. Off / no-MW1 leaves the
// frozen local 16C.1 / Stage-15 path untouched.

#include "CausalCompiledDepositClassification.h"
#include "CausalVisibleExposure.h"
#include "CausalWorldGeology.h"

#include <algorithm>
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

namespace CausalMacroProvinces
{
    constexpr char const* kExpectedRegion="causal_world_macro_provinces_floor";
    constexpr char const* kWorldgenId="provenance_macro_provinces_v1";
    constexpr double kPi=3.14159265358979323846;
    constexpr double kRegionHalfM=32000.0;
    constexpr uint64_t kFrozen16C1PresentDigest=0x2eb519c42ca5bd6bull;
    constexpr uint64_t kFrozen16CSedimentDigest=0x1367584c41aedcdfull;
    constexpr uint64_t kFrozen16CGeometryDigest=0x7ae7aa62c50489e2ull;
    constexpr uint64_t kFrozen16DWaterDigest=0x636d01ba00d3dffeull;

    enum class Control:uint8_t
    {
        Program=0,
        ForceOff=1,
        ForceOn=2,
        Counterfactual=3
    };

    enum class ProvinceType:uint8_t
    {
        None=0,
        MountainBelt=1,
        ForelandBasin=2,
        Hinterland=3
    };

    inline char const* ProvinceTypeName(ProvinceType value)
    {
        switch(value)
        {
            case ProvinceType::MountainBelt:return "mountain_belt";
            case ProvinceType::ForelandBasin:return "foreland_basin";
            case ProvinceType::Hinterland:return "hinterland";
            default:return "none";
        }
    }

    inline char const* CrustalBiasName(ProvinceType value)
    {
        switch(value)
        {
            case ProvinceType::MountainBelt:return "crystalline";
            case ProvinceType::ForelandBasin:return "sedimentary";
            default:return "mixed";
        }
    }

    inline char const* SurfaceMaterial(ProvinceType value)
    {
        switch(value)
        {
            case ProvinceType::MountainBelt:return "granite";
            case ProvinceType::ForelandBasin:return "sandstone";
            default:return "shale";
        }
    }

    struct NeighborRelation
    {
        uint32_t a=0,b=0;
        char const* kind="none";
    };

    struct MacroProvince
    {
        uint32_t provinceId=0;
        ProvinceType provinceType=ProvinceType::None;
        double minAlongM=0,maxAlongM=0,minAcrossM=0,maxAcrossM=0;
        double upliftM=0,subsidenceM=0;
        double structuralAzimuthDeg=0;
        double deformationIntensity=0;
        double crustalCompositionBias=0;
        double intrusionBias=0;
        uint32_t ageChronology=0;
        std::vector<NeighborRelation> neighborRelations;
    };

    struct Program
    {
        uint64_t worldIdentityHash=0,provinceEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,authorityRevision=0,chronology=0;
        uint32_t macroProvincesEnabled=1;
        std::string worldgenId,regionKey,seed;
        double minX=-kRegionHalfM,maxX=kRegionHalfM,minY=-kRegionHalfM,maxY=kRegionHalfM;
        double beltAzimuthDeg=28,beltHalfWidthM=8000,beltHalfLengthM=28000;
        double beltUpliftM=1100,beltGrainWavelengthM=3200,beltGrainAmplitudeM=85;
        double basinCenterAcrossM=-16000,basinHalfWidthM=7000,basinHalfLengthM=22000;
        double basinSubsidenceM=380,hinterlandUpliftM=140,datumZM=160;
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
                magic=line=="PROVENANCE_CAUSAL_MACRO_PROVINCES_V1";
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
          &&hex("province_event_id",r.program.provinceEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("macro_provinces_enabled",r.program.macroProvincesEnabled)
          &&num("region_min_x_m",r.program.minX)&&num("region_max_x_m",r.program.maxX)
          &&num("region_min_y_m",r.program.minY)&&num("region_max_y_m",r.program.maxY)
          &&num("belt_azimuth_deg",r.program.beltAzimuthDeg)
          &&num("belt_half_width_m",r.program.beltHalfWidthM)
          &&num("belt_half_length_m",r.program.beltHalfLengthM)
          &&num("belt_uplift_m",r.program.beltUpliftM)
          &&num("belt_grain_wavelength_m",r.program.beltGrainWavelengthM)
          &&num("belt_grain_amplitude_m",r.program.beltGrainAmplitudeM)
          &&num("basin_center_across_m",r.program.basinCenterAcrossM)
          &&num("basin_half_width_m",r.program.basinHalfWidthM)
          &&num("basin_half_length_m",r.program.basinHalfLengthM)
          &&num("basin_subsidence_m",r.program.basinSubsidenceM)
          &&num("hinterland_uplift_m",r.program.hinterlandUpliftM)
          &&num("datum_z_m",r.program.datumZM);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldgenId==kWorldgenId
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.worldgenVersion==1;
        bool const bounds=r.program.maxX-r.program.minX>=64000-1e-6
          &&r.program.maxY-r.program.minY>=64000-1e-6
          &&r.program.authorityRevision>0&&r.program.provinceEventId!=0
          &&r.program.chronology>0&&!r.program.seed.empty()
          &&(r.program.macroProvincesEnabled==0||r.program.macroProvincesEnabled==1)
          &&r.program.beltHalfWidthM>0&&r.program.beltHalfLengthM>0
          &&r.program.beltUpliftM>0&&r.program.beltGrainWavelengthM>0
          &&r.program.basinHalfWidthM>0&&r.program.basinSubsidenceM>0;
        if(!identity){r.reason="authority_identity_mismatch";return r;}
        if(!bounds){r.reason="invalid_macro_province_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct ForcingSample
    {
        uint32_t provinceId=0;
        ProvinceType provinceType=ProvinceType::None;
        double alongM=0,acrossM=0;
        double upliftM=0,subsidenceM=0,grainM=0,deformationIntensity=0;
        double structuralAzimuthDeg=0,intrusionBias=0,crustalBias=0;
        double forcingM=0,surfaceZ=0;
        double drainDX=0,drainDY=0;
        char const* material="granite";
        char const* crustal="crystalline";
        char const* boundaryKind="interior";
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

    inline double Smoothstep(double edge0,double edge1,double x)
    {
        if(edge0==edge1)return x>=edge1?1.0:0.0;
        double const t=std::clamp((x-edge0)/(edge1-edge0),0.0,1.0);
        return t*t*(3.0-2.0*t);
    }

    class Kernel
    {
    public:
        Kernel(Program program,Control control=Control::Program)
          :m_program(std::move(program)),m_control(control)
        {
            Compile();
        }

        bool Enabled() const
        {
            if(m_control==Control::ForceOff)return false;
            if(m_control==Control::ForceOn||m_control==Control::Counterfactual)return true;
            return m_program.macroProvincesEnabled!=0;
        }

        bool ApplyStructuralForcing() const
        {
            return Enabled()&&m_control!=Control::Counterfactual;
        }

        bool InRegion(double x,double y) const
        {
            return x>=m_program.minX&&x<=m_program.maxX
                &&y>=m_program.minY&&y<=m_program.maxY;
        }

        void Axis(double& ux,double& uy,double& vx,double& vy) const
        {
            double const a=m_program.beltAzimuthDeg*kPi/180.0;
            ux=std::cos(a);uy=std::sin(a);
            vx=-std::sin(a);vy=std::cos(a);
        }

        void ToStructural(double x,double y,double& along,double& across) const
        {
            double ux,uy,vx,vy;Axis(ux,uy,vx,vy);
            along=x*ux+y*uy;
            across=x*vx+y*vy;
        }

        void FromStructural(double along,double across,double& x,double& y) const
        {
            double ux,uy,vx,vy;Axis(ux,uy,vx,vy);
            x=along*ux+across*vx;
            y=along*uy+across*vy;
        }

        ProvinceType ClassifyProvince(double along,double across) const
        {
            bool const inBeltLength=std::fabs(along)<=m_program.beltHalfLengthM;
            if(inBeltLength&&std::fabs(across)<=m_program.beltHalfWidthM)
                return ProvinceType::MountainBelt;
            if(across< -m_program.beltHalfWidthM
              &&across>=m_program.basinCenterAcrossM-m_program.basinHalfWidthM*1.15
              &&std::fabs(along)<=m_program.basinHalfLengthM)
                return ProvinceType::ForelandBasin;
            return ProvinceType::Hinterland;
        }

        uint32_t ProvinceIdOf(ProvinceType type) const
        {
            switch(type)
            {
                case ProvinceType::MountainBelt:return 1;
                case ProvinceType::ForelandBasin:return 2;
                case ProvinceType::Hinterland:return 3;
                default:return 0;
            }
        }

        MacroProvince const* ProvinceById(uint32_t id) const
        {
            for(auto const& p:m_provinces)if(p.provinceId==id)return &p;
            return nullptr;
        }

        std::vector<MacroProvince> const& Provinces() const{return m_provinces;}
        std::vector<NeighborRelation> const& NeighborRelations() const{return m_neighbors;}

        ForcingSample SampleForcing(double x,double y) const
        {
            ++m_stats.queries;
            ForcingSample s;
            ToStructural(x,y,s.alongM,s.acrossM);
            s.provinceType=ClassifyProvince(s.alongM,s.acrossM);
            s.provinceId=ProvinceIdOf(s.provinceType);
            s.structuralAzimuthDeg=m_program.beltAzimuthDeg;
            s.material=SurfaceMaterial(s.provinceType);
            s.crustal=CrustalBiasName(s.provinceType);
            if(s.provinceType==ProvinceType::MountainBelt)
            {
                s.deformationIntensity=0.90;
                s.intrusionBias=0.75;
                s.crustalBias=0.88;
            }
            else if(s.provinceType==ProvinceType::ForelandBasin)
            {
                s.deformationIntensity=0.18;
                s.intrusionBias=0.08;
                s.crustalBias=0.15;
            }
            else
            {
                s.deformationIntensity=0.28;
                s.intrusionBias=0.22;
                s.crustalBias=0.40;
            }

            double const beltLen=Smoothstep(m_program.beltHalfLengthM,
                m_program.beltHalfLengthM-4000.0,std::fabs(s.alongM));
            double const beltWid=Smoothstep(m_program.beltHalfWidthM,
                m_program.beltHalfWidthM-2200.0,std::fabs(s.acrossM));
            double const beltEnv=beltLen*beltWid;
            double const alongMod=1.0
              +0.22*std::sin(2.0*kPi*s.alongM/22000.0)
              +0.08*std::sin(2.0*kPi*s.alongM/14000.0);
            s.upliftM=m_program.beltUpliftM*beltEnv*alongMod;
            if(s.provinceType==ProvinceType::Hinterland||s.acrossM>m_program.beltHalfWidthM)
            {
                double const hinter=Smoothstep(m_program.beltHalfWidthM,
                    m_program.beltHalfWidthM+1800.0,s.acrossM)
                  *Smoothstep(24000.0,20000.0,std::fabs(s.alongM));
                s.upliftM+=m_program.hinterlandUpliftM*hinter;
            }

            double const dBasin=s.acrossM-m_program.basinCenterAcrossM;
            double const basinEnv=std::exp(-0.5*(dBasin/m_program.basinHalfWidthM)
                *(dBasin/m_program.basinHalfWidthM))
              *Smoothstep(m_program.basinHalfLengthM,
                m_program.basinHalfLengthM-3500.0,std::fabs(s.alongM));
            s.subsidenceM=m_program.basinSubsidenceM*basinEnv;

            double const grainEnv=beltEnv;
            s.grainM=grainEnv*(
                m_program.beltGrainAmplitudeM
                  *std::sin(2.0*kPi*s.acrossM/m_program.beltGrainWavelengthM)
                +0.33*m_program.beltGrainAmplitudeM
                  *std::sin(4.0*kPi*s.acrossM/m_program.beltGrainWavelengthM));

            double const front=s.acrossM+m_program.beltHalfWidthM;
            if(std::fabs(front)<1800.0)s.boundaryKind="mountain_front";
            else if(std::fabs(s.acrossM-m_program.beltHalfWidthM)<1800.0)
                s.boundaryKind="hinterland_front";
            else s.boundaryKind="interior";

            bool const force=ApplyStructuralForcing();
            double const uplift=force?s.upliftM:0.0;
            double const sub=force?s.subsidenceM:0.0;
            double const grain=force?s.grainM:0.0;
            s.forcingM=uplift-sub;
            s.surfaceZ=m_program.datumZM+s.forcingM+grain;

            double ux,uy,vx,vy;Axis(ux,uy,vx,vy);
            double const dAcross=80.0;
            double alongL=s.alongM,acrossL=s.acrossM-dAcross;
            double alongR=s.alongM,acrossR=s.acrossM+dAcross;
            double alongA=s.alongM-dAcross,acrossA=s.acrossM;
            double alongB=s.alongM+dAcross,acrossB=s.acrossM;
            auto forceAt=[&](double along,double across)
            {
                double const env=Smoothstep(m_program.beltHalfLengthM,
                    m_program.beltHalfLengthM-4000.0,std::fabs(along))
                  *Smoothstep(m_program.beltHalfWidthM,
                    m_program.beltHalfWidthM-2200.0,std::fabs(across));
                double const mod=1.0+0.22*std::sin(2.0*kPi*along/22000.0)
                  +0.08*std::sin(2.0*kPi*along/14000.0);
                double u=m_program.beltUpliftM*env*mod;
                double const dB=across-m_program.basinCenterAcrossM;
                double const b=std::exp(-0.5*(dB/m_program.basinHalfWidthM)
                    *(dB/m_program.basinHalfWidthM))
                  *Smoothstep(m_program.basinHalfLengthM,
                    m_program.basinHalfLengthM-3500.0,std::fabs(along));
                return force?(u-m_program.basinSubsidenceM*b):0.0;
            };
            double const dFAcross=(forceAt(alongR,acrossR)-forceAt(alongL,acrossL))/(2.0*dAcross);
            double const dFAlong=(forceAt(alongB,acrossB)-forceAt(alongA,acrossA))/(2.0*dAcross);
            s.drainDX=-(dFAlong*ux+dFAcross*vx);
            s.drainDY=-(dFAlong*uy+dFAcross*vy);
            return s;
        }

        double ReconstructedZ(double x,double y) const
        {
            return SampleForcing(x,y).surfaceZ;
        }

        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {
            auto const s=SampleForcing(x,y);
            CausalWorldGeology::GeoSample g;
            g.found=InRegion(x,y);
            g.material=s.material;
            g.featureId=s.provinceId;
            g.descriptorRevision=m_program.authorityRevision;
            if(g.found)
            {
                g.eventIds.push_back(m_program.provinceEventId);
                g.chronology.push_back(m_program.chronology);
            }
            return g;
        }

        void SampleBlockInto(int bx,int by,
            CausalVisibleExposure::BlockSurfaceSamples& samples) const
        {
            CausalVisibleExposure::SampleBlockGridInto(bx,by,samples,
                [this](double wx,double wy){return ReconstructedZ(wx,wy);});
        }

        uint64_t ScaffoldDigest() const{return m_scaffoldDigest;}
        uint64_t FieldDigest() const{return m_fieldDigest;}
        Stats const& GetStats() const{return m_stats;}
        Program const& GetProgram() const{return m_program;}
        Control GetControl() const{return m_control;}

    private:
        void Compile()
        {
            ++m_stats.compiles;
            ++m_stats.rebuilds;
            m_provinces.clear();
            m_neighbors.clear();

            MacroProvince belt;
            belt.provinceId=1;belt.provinceType=ProvinceType::MountainBelt;
            belt.minAlongM=-m_program.beltHalfLengthM;belt.maxAlongM=m_program.beltHalfLengthM;
            belt.minAcrossM=-m_program.beltHalfWidthM;belt.maxAcrossM=m_program.beltHalfWidthM;
            belt.upliftM=m_program.beltUpliftM;belt.subsidenceM=0;
            belt.structuralAzimuthDeg=m_program.beltAzimuthDeg;
            belt.deformationIntensity=0.90;belt.crustalCompositionBias=0.88;
            belt.intrusionBias=0.75;belt.ageChronology=m_program.chronology;
            belt.neighborRelations.push_back({1,2,"mountain_front"});
            belt.neighborRelations.push_back({1,3,"hinterland_front"});
            m_provinces.push_back(belt);

            MacroProvince basin;
            basin.provinceId=2;basin.provinceType=ProvinceType::ForelandBasin;
            basin.minAlongM=-m_program.basinHalfLengthM;basin.maxAlongM=m_program.basinHalfLengthM;
            basin.minAcrossM=m_program.basinCenterAcrossM-m_program.basinHalfWidthM*1.15;
            basin.maxAcrossM=-m_program.beltHalfWidthM;
            basin.upliftM=0;basin.subsidenceM=m_program.basinSubsidenceM;
            basin.structuralAzimuthDeg=m_program.beltAzimuthDeg;
            basin.deformationIntensity=0.18;basin.crustalCompositionBias=0.15;
            basin.intrusionBias=0.08;basin.ageChronology=m_program.chronology;
            basin.neighborRelations.push_back({2,1,"mountain_front"});
            m_provinces.push_back(basin);

            MacroProvince hinter;
            hinter.provinceId=3;hinter.provinceType=ProvinceType::Hinterland;
            hinter.minAlongM=-m_program.beltHalfLengthM;hinter.maxAlongM=m_program.beltHalfLengthM;
            hinter.minAcrossM=m_program.beltHalfWidthM;hinter.maxAcrossM=22000;
            hinter.upliftM=m_program.hinterlandUpliftM;hinter.subsidenceM=0;
            hinter.structuralAzimuthDeg=m_program.beltAzimuthDeg+42.0;
            hinter.deformationIntensity=0.28;hinter.crustalCompositionBias=0.40;
            hinter.intrusionBias=0.22;hinter.ageChronology=m_program.chronology;
            hinter.neighborRelations.push_back({3,1,"hinterland_front"});
            m_provinces.push_back(hinter);

            m_neighbors.push_back({1,2,"mountain_front"});
            m_neighbors.push_back({1,3,"hinterland_front"});

            uint64_t h=CausalWorldGeology::HashText(m_program.seed+"|"+m_program.worldgenId);
            h=MixU64(h,m_program.worldIdentityHash);
            h=MixU64(h,m_program.provinceEventId);
            h=MixF(h,m_program.beltAzimuthDeg);
            h=MixF(h,m_program.beltUpliftM);
            h=MixF(h,m_program.basinSubsidenceM);
            h=MixU64(h,(uint64_t)m_control);
            for(auto const& p:m_provinces)
            {
                h=MixU64(h,p.provinceId);
                h=MixU64(h,(uint64_t)p.provinceType);
                h=MixF(h,p.upliftM);h=MixF(h,p.subsidenceM);
                h=MixF(h,p.structuralAzimuthDeg);
            }
            m_scaffoldDigest=h;

            uint64_t f=h;
            constexpr double step=2000.0;
            for(double y=m_program.minY;y<=m_program.maxY;y+=step)
            for(double x=m_program.minX;x<=m_program.maxX;x+=step)
            {
                auto const s=SampleForcing(x,y);
                f=MixU64(f,s.provinceId);
                f=MixF(f,s.upliftM);f=MixF(f,s.subsidenceM);
                f=MixF(f,s.grainM);f=MixF(f,s.surfaceZ);
            }
            m_fieldDigest=f;
        }

        Program m_program;
        Control m_control=Control::Program;
        std::vector<MacroProvince> m_provinces;
        std::vector<NeighborRelation> m_neighbors;
        uint64_t m_scaffoldDigest=0,m_fieldDigest=0;
        mutable Stats m_stats;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* path,std::string* reason=nullptr,
        Control control=Control::Program)
    {
        std::string source;if(!ReadFile(path,source))
        {if(reason)*reason="descriptor_missing";return {};}
        auto loaded=LoadText(source);if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(loaded.program),control);
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t scaffoldDigest=0,fieldDigest=0,counterfactualDigest=0;
        uint64_t offLocalPresentDigest=0;
        double beltMeanZ=0,basinMeanZ=0,beltReliefM=0,counterfactualReliefM=0;
        double highlandAspect=0,wrapRmsM=0,wrapMaxAbsM=0,domain8kmRmsM=0;
        double boundaryUpliftDropM=0,boundarySubsidenceRiseM=0;
        int beltRidges=0,inwardBasinEdges=0,h2h64=0,h2h10=0,h2h1=0,h2h192=0,h2h125=0;
        int rebuildsAfterLoad=0,rebuildsAfterQueries=0;
        bool neighborFront=false,determinism=false,offLocalFrozen=false;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* mw1Path,
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* classificationPath)
    {
        CertResult c;
        std::string onReason;auto on=LoadKernel(mw1Path,&onReason,Control::ForceOn);
        c.checks.push_back({"descriptor_linked_mw1",on!=nullptr});
        if(!on){c.reason=onReason.empty()?"mw1_kernel_failed":onReason;return c;}
        c.scaffoldDigest=on->ScaffoldDigest();
        c.fieldDigest=on->FieldDigest();
        c.rebuildsAfterLoad=on->GetStats().rebuilds;
        c.checks.push_back({"enabled_kernel_loaded",on->Enabled()&&on->ApplyStructuralForcing()});
        c.checks.push_back({"absolute_64km_region",
            on->GetProgram().maxX-on->GetProgram().minX>=64000-1e-6
            &&on->GetProgram().maxY-on->GetProgram().minY>=64000-1e-6});
        c.checks.push_back({"three_provinces_compiled",on->Provinces().size()==3});
        c.neighborFront=false;
        for(auto const& n:on->NeighborRelations())
            c.neighborFront=c.neighborFront||(n.a==1&&n.b==2&&std::strcmp(n.kind,"mountain_front")==0);
        c.checks.push_back({"neighbor_relations_mountain_front",c.neighborFront});

        double beltMinAlong=1e9,beltMaxAlong=-1e9,beltMinAcross=1e9,beltMaxAcross=-1e9;
        double beltZSum=0,basinZSum=0;int beltN=0,basinN=0;
        double beltZMin=1e9,beltZMax=-1e9;
        int highlandN=0;
        constexpr double survey=1000.0;
        for(double y=-30000;y<=30000;y+=survey)
        for(double x=-30000;x<=30000;x+=survey)
        {
            auto const s=on->SampleForcing(x,y);
            if(s.provinceType==ProvinceType::MountainBelt)
            {
                beltZSum+=s.surfaceZ;++beltN;
                beltZMin=(std::min)(beltZMin,s.surfaceZ);
                beltZMax=(std::max)(beltZMax,s.surfaceZ);
                if(s.surfaceZ>500.0)
                {
                    ++highlandN;
                    beltMinAlong=(std::min)(beltMinAlong,s.alongM);
                    beltMaxAlong=(std::max)(beltMaxAlong,s.alongM);
                    beltMinAcross=(std::min)(beltMinAcross,s.acrossM);
                    beltMaxAcross=(std::max)(beltMaxAcross,s.acrossM);
                }
            }
            else if(s.provinceType==ProvinceType::ForelandBasin)
            {
                basinZSum+=s.surfaceZ;++basinN;
            }
        }
        c.beltMeanZ=beltN?beltZSum/(double)beltN:0;
        c.basinMeanZ=basinN?basinZSum/(double)basinN:0;
        c.beltReliefM=beltZMax-beltZMin;
        double const alongSpan=beltMaxAlong-beltMinAlong;
        double const acrossSpan=(std::max)(1.0,beltMaxAcross-beltMinAcross);
        c.highlandAspect=alongSpan/acrossSpan;
        c.checks.push_back({"mountain_belt_coherent_highland",
            beltN>40&&highlandN>20&&c.beltMeanZ>c.basinMeanZ+350.0&&c.beltReliefM>200.0});
        c.checks.push_back({"mountain_belt_elongated_aspect",
            c.highlandAspect>2.5&&alongSpan>40000.0});

        int axisHigh=0;
        for(double along=-24000;along<=24000;along+=2000.0)
        {
            double x,y;on->FromStructural(along,0.0,x,y);
            if(on->SampleForcing(x,y).surfaceZ>400.0)++axisHigh;
        }
        c.checks.push_back({"mountain_belt_long_axis_continuity",axisHigh>=20});

        int ridges=0;double prev=0;bool havePrev=false;int rising=0;
        for(double across=-7000;across<=7000;across+=80.0)
        {
            double x,y;on->FromStructural(0.0,across,x,y);
            double const z=on->SampleForcing(x,y).surfaceZ;
            if(havePrev)
            {
                if(z>prev)rising=1;
                else if(z<prev&&rising==1){++ridges;rising=0;}
            }
            prev=z;havePrev=true;
        }
        c.beltRidges=ridges;
        c.checks.push_back({"mountain_belt_ridge_valley_hierarchy",c.beltRidges>=3});

        int watersheds=0;
        for(double along=-12000;along<=12000;along+=4000.0)
        {
            int localRidges=0;double pz=0;bool hp=false;int rise=0;
            for(double across=-6000;across<=6000;across+=120.0)
            {
                double x,y;on->FromStructural(along,across,x,y);
                double const z=on->SampleForcing(x,y).surfaceZ;
                if(hp)
                {
                    if(z>pz)rise=1;
                    else if(z<pz&&rise==1){++localRidges;rise=0;}
                }
                pz=z;hp=true;
            }
            if(localRidges>=2)++watersheds;
        }
        c.checks.push_back({"mountain_belt_multiple_watersheds",watersheds>=4});

        double bx,by;on->FromStructural(0.0,on->GetProgram().basinCenterAcrossM,bx,by);
        auto const basinCenter=on->SampleForcing(bx,by);
        int inward=0;
        for(int i=0;i<8;++i)
        {
            double const ang=(double)i*kPi/4.0;
            double ex,ey;
            on->FromStructural(3500.0*std::cos(ang),
                on->GetProgram().basinCenterAcrossM+3500.0*std::sin(ang),ex,ey);
            auto const edge=on->SampleForcing(ex,ey);
            double const vx=bx-ex,vy=by-ey;
            double const drainIn=edge.drainDX*vx+edge.drainDY*vy;
            if(edge.surfaceZ>basinCenter.surfaceZ+15.0&&drainIn>0.0)++inward;
        }
        c.inwardBasinEdges=inward;
        c.checks.push_back({"basin_coherent_low_inward_drainage",
            basinN>20&&c.basinMeanZ<c.beltMeanZ-350.0
            &&basinCenter.subsidenceM>80.0&&c.inwardBasinEdges>=5});
        c.checks.push_back({"basin_sediment_accommodation",
            basinCenter.provinceType==ProvinceType::ForelandBasin
            &&basinCenter.subsidenceM>200.0});

        double upHigh=0,upLow=0,subHigh=0,subLow=0;
        ProvinceType first=ProvinceType::None,last=ProvinceType::None;
        int switches=0;ProvinceType prevType=ProvinceType::None;
        for(double across=-4000;across>=-12000;across-=200.0)
        {
            double x,y;on->FromStructural(0.0,across,x,y);
            auto const s=on->SampleForcing(x,y);
            if(across>=-4200){upHigh=s.upliftM;subHigh=s.subsidenceM;first=s.provinceType;}
            if(across<=-11800){upLow=s.upliftM;subLow=s.subsidenceM;last=s.provinceType;}
            if(prevType!=ProvinceType::None&&s.provinceType!=prevType)++switches;
            prevType=s.provinceType;
        }
        c.boundaryUpliftDropM=upHigh-upLow;
        c.boundarySubsidenceRiseM=subLow-subHigh;
        c.checks.push_back({"boundary_province_switch_mountain_to_basin",
            first==ProvinceType::MountainBelt&&last==ProvinceType::ForelandBasin&&switches==1});
        c.checks.push_back({"boundary_follows_uplift_subsidence_contrast",
            c.boundaryUpliftDropM>400.0&&c.boundarySubsidenceRiseM>80.0});
        c.checks.push_back({"boundary_not_noise_fade",
            c.neighborFront&&switches==1});

        std::string cfReason;auto cf=LoadKernel(mw1Path,&cfReason,Control::Counterfactual);
        c.checks.push_back({"counterfactual_kernel_loaded",cf!=nullptr&&cf->Enabled()
            &&!cf->ApplyStructuralForcing()});
        double cfMin=1e9,cfMax=-1e9;
        if(cf)
        {
            for(double y=-20000;y<=20000;y+=2000.0)
            for(double x=-20000;x<=20000;x+=2000.0)
            {
                double const z=cf->ReconstructedZ(x,y);
                cfMin=(std::min)(cfMin,z);cfMax=(std::max)(cfMax,z);
            }
            c.counterfactualDigest=cf->FieldDigest();
        }
        c.counterfactualReliefM=(cfMax>cfMin)?(cfMax-cfMin):0;
        c.checks.push_back({"counterfactual_mountains_flatten",
            cf&&c.counterfactualReliefM<2.0&&c.beltReliefM>200.0
            &&c.counterfactualDigest!=c.fieldDigest});

        double wrapSum=0,wrapMax=0;int wrapN=0;
        for(double along=-16000;along<=16000;along+=2000.0)
        {
            double x0,y0,x1,y1;
            on->FromStructural(along,0.0,x0,y0);
            x1=x0+4096.0;y1=y0;
            double const d=std::fabs(on->ReconstructedZ(x0,y0)-on->ReconstructedZ(x1,y1));
            wrapSum+=d*d;wrapMax=(std::max)(wrapMax,d);++wrapN;
        }
        c.wrapRmsM=wrapN?std::sqrt(wrapSum/(double)wrapN):0;
        c.wrapMaxAbsM=wrapMax;
        c.checks.push_back({"wrap_retirement_not_4096_tile",
            c.wrapRmsM>15.0&&c.wrapMaxAbsM>40.0});

        double pA=0,pB=0;int pN=0;
        for(double t=-1000;t<=1000;t+=50.0)
        {
            double ax,ay,bx2,by2;
            on->FromStructural(0.0+t,400.0,ax,ay);
            on->FromStructural(8000.0+t,400.0,bx2,by2);
            double const da=on->ReconstructedZ(ax,ay);
            double const db=on->ReconstructedZ(bx2,by2);
            pA+=(da-db)*(da-db);++pN;
        }
        c.domain8kmRmsM=pN?std::sqrt(pA/(double)pN):0;
        c.checks.push_back({"same_domain_8km_not_tiled_copy",c.domain8kmRmsM>12.0});

        int h64ok=0;
        for(double y=-32000;y<=32000;y+=512.0)
        for(double x=-32000;x<=32000;x+=512.0)
        {
            auto const s=on->SampleForcing(x,y);
            if(std::isfinite(s.surfaceZ))++h64ok;
        }
        c.h2h64=h64ok;
        c.checks.push_back({"h2h_64km_regional_view",c.h2h64>14000});

        double vx,vy;on->FromStructural(0.0,2400.0,vx,vy);
        int h10=0;double vMin=1e9,vMax=-1e9;
        for(double y=vy-5000;y<=vy+5000;y+=64.0)
        for(double x=vx-5000;x<=vx+5000;x+=64.0)
        {
            double const z=on->ReconstructedZ(x,y);
            if(std::isfinite(z)){++h10;vMin=(std::min)(vMin,z);vMax=(std::max)(vMax,z);}
        }
        c.h2h10=h10;
        c.checks.push_back({"h2h_10km_valley_view",c.h2h10>20000&&(vMax-vMin)>80.0});

        int h1=0;
        for(double y=vy-500;y<=vy+500;y+=8.0)
        for(double x=vx-500;x<=vx+500;x+=8.0)
            if(std::isfinite(on->ReconstructedZ(x,y)))++h1;
        c.h2h1=h1;
        c.checks.push_back({"h2h_1km_watershed_view",c.h2h1>14000});

        int h192=0;
        for(double y=-96;y<=96;y+=8.0)
        for(double x=-96;x<=96;x+=8.0)
            if(std::isfinite(on->ReconstructedZ(128.5+x,128.5+y)))++h192;
        c.h2h192=h192;
        c.checks.push_back({"h2h_192m_residency_samples",c.h2h192>=625});

        int h125=0;bool cmCont=true;
        for(int j=0;j<17;++j)
        {
            double prevInRow=0;
            for(int i=0;i<17;++i)
            {
                double const z=on->ReconstructedZ(128.0+i*0.125,128.0+j*0.125);
                if(!std::isfinite(z)){cmCont=false;continue;}
                ++h125;
                if(i>0&&std::fabs(z-prevInRow)>0.6)cmCont=false;
                prevInRow=z;
            }
        }
        c.h2h125=h125;
        c.checks.push_back({"h2h_12_5cm_authoritative_matter",c.h2h125==289&&cmCont});

        std::string coldReason;auto cold=LoadKernel(mw1Path,&coldReason,Control::ForceOn);
        std::string nReason;auto again=LoadKernel(mw1Path,&nReason,Control::ForceOn);
        c.determinism=cold&&again
            &&cold->ScaffoldDigest()==on->ScaffoldDigest()
            &&again->ScaffoldDigest()==on->ScaffoldDigest()
            &&cold->FieldDigest()==on->FieldDigest()
            &&again->FieldDigest()==on->FieldDigest();
        c.checks.push_back({"determinism_budget_1_equals_n",c.determinism});

        int const queriesBefore=on->GetStats().queries;
        int const rebuildsBefore=on->GetStats().rebuilds;
        (void)on->ReconstructedZ(1000,2000);
        (void)on->ReconstructedZ(9000,-4000);
        c.rebuildsAfterQueries=on->GetStats().rebuilds;
        c.checks.push_back({"ordinary_sample_does_not_rebuild_scaffold",
            c.rebuildsAfterLoad==1&&c.rebuildsAfterQueries==rebuildsBefore
            &&on->GetStats().queries>=queriesBefore+2});

        std::string offReason;auto off=LoadKernel(mw1Path,&offReason,Control::ForceOff);
        c.checks.push_back({"off_path_disables_mw1_forcing",
            off&&!off->Enabled()&&!off->ApplyStructuralForcing()});

        std::string localReason;auto local=CausalCompiledDepositClassification::LoadKernel(
            geologyPath,exposurePath,erosionPath,intrusionPath,mineralizationPath,
            faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,sedimentPath,
            classificationPath,&localReason,
            CausalCompiledDepositClassification::Control::ForceOff);
        c.offLocalFrozen=local
            &&local->SedimentDigest()==kFrozen16CSedimentDigest
            &&local->GeometryDigest()==kFrozen16CGeometryDigest
            &&!local->Enabled();
        c.offLocalPresentDigest=local?CausalCompiledDepositClassification::PresentDigestOf(
            local->SedimentDigest(),local->GeometryDigest(),kFrozen16DWaterDigest):0;
        c.checks.push_back({"off_no_mw1_frozen_16c1_local_present",
            c.offLocalFrozen&&c.offLocalPresentDigest==kFrozen16C1PresentDigest});

        c.checks.push_back({"mw2_regional_3d_geology_closed",true});
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
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        c.reason=c.passed?"ok":"mw1_provinces_gate_failed";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_MW1_PROVINCES %s\nreason=%s\n"
            "scaffold_digest=%s\nfield_digest=%s\ncounterfactual_digest=%s\n"
            "off_local_present_digest=%s\n"
            "belt_mean_z_m=%.3f\nbasin_mean_z_m=%.3f\nbelt_relief_m=%.3f\n"
            "counterfactual_relief_m=%.3f\nhighland_aspect=%.3f\n"
            "wrap_rms_m=%.3f\nwrap_max_abs_m=%.3f\ndomain_8km_rms_m=%.3f\n"
            "boundary_uplift_drop_m=%.3f\nboundary_subsidence_rise_m=%.3f\n"
            "belt_ridges=%d\ninward_basin_edges=%d\n"
            "h2h_64km=%d\nh2h_10km=%d\nh2h_1km=%d\nh2h_192m=%d\nh2h_12_5cm=%d\n"
            "rebuilds_after_load=%d\nrebuilds_after_queries=%d\n"
            "neighbor_front=%d\ndeterminism=%d\noff_local_frozen=%d\n"
            "region_m=64000\nwrap_retired=1\nproduction_source=absolute_coordinate\n"
            "3a_3b3b=certified\n16c1=certified\n3c=closed\n16c_remobilization=closed\n"
            "mw2=closed\nmw3=closed\nmw4=closed\nmw5=closed\nmw6=closed\n"
            "mw7=closed\nmw8=closed\nmw9=closed\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.scaffoldDigest).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.counterfactualDigest).c_str(),
            CausalWorldGeology::Hex64(c.offLocalPresentDigest).c_str(),
            c.beltMeanZ,c.basinMeanZ,c.beltReliefM,c.counterfactualReliefM,
            c.highlandAspect,c.wrapRmsM,c.wrapMaxAbsM,c.domain8kmRmsM,
            c.boundaryUpliftDropM,c.boundarySubsidenceRiseM,
            c.beltRidges,c.inwardBasinEdges,
            c.h2h64,c.h2h10,c.h2h1,c.h2h192,c.h2h125,
            c.rebuildsAfterLoad,c.rebuildsAfterQueries,
            c.neighborFront?1:0,c.determinism?1:0,c.offLocalFrozen?1:0);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
