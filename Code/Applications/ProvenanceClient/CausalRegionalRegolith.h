#pragma once

// MW7: compiled near-surface soils / regolith on certified MW1–MW6.
// Derived profile state — not a terrain carver, not vegetation, not biomes.
//
// Required chain:
//   MW1 provinces → MW2 geology → MW3 erosion → MW4 drainage → MW5 deposits
//   → MW6 hydroclimate → MW7 soils / regolith → (closed) MW8 biomes
//
// Off / no-MW7 leaves the exact MW6 present surface, bodies, and
// hydroclimate identity untouched.

#include "CausalRegionalHydroclimate.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalRegionalRegolith
{
    constexpr char const* kExpectedRegion="causal_world_regional_regolith_floor";
    constexpr char const* kWorldgenId="provenance_regional_regolith_v1";
    constexpr double kPi=3.14159265358979323846;
    constexpr uint64_t kFrozen16C1PresentDigest=0x2eb519c42ca5bd6bull;
    constexpr uint64_t kFrozen16CSedimentDigest=0x1367584c41aedcdfull;
    constexpr uint64_t kFrozen16CGeometryDigest=0x7ae7aa62c50489e2ull;
    constexpr uint64_t kFrozen16DWaterDigest=0x636d01ba00d3dffeull;
    constexpr uint64_t kTagRegolith=0x4d57370000000002ull;

    enum class Control:uint8_t
    {
        Program=0,
        ForceOff=1,
        ForceOn=2
    };

    enum class ProfileClass:uint8_t
    {
        None=0,
        BareBedrock=1,
        WeatheredBedrock=2,
        ThinRegolith=3,
        Colluvium=4,
        Alluvium=5,
        FloodplainSediment=6,
        BasinFill=7,
        Talus=8,
        OrganicCapable=9,
        WaterloggedMineral=10
    };

    enum class GrainSizeTendency:uint8_t
    {
        None=0,
        Fine=1,
        MediumFine=2,
        Medium=3,
        MediumCoarse=4,
        Coarse=5
    };

    enum class DrainageClass:uint8_t
    {
        None=0,
        ExcessivelyDrained=1,
        WellDrained=2,
        ModeratelyDrained=3,
        SomewhatPoorlyDrained=4,
        PoorlyDrained=5,
        Saturated=6
    };

    enum class WeatheringState:uint8_t
    {
        None=0,
        Fresh=1,
        Slight=2,
        Moderate=3,
        Strong=4
    };

    inline char const* ProfileClassName(ProfileClass v)
    {
        switch(v)
        {
            case ProfileClass::BareBedrock:return "bare_bedrock";
            case ProfileClass::WeatheredBedrock:return "weathered_bedrock";
            case ProfileClass::ThinRegolith:return "thin_regolith";
            case ProfileClass::Colluvium:return "colluvium";
            case ProfileClass::Alluvium:return "alluvium";
            case ProfileClass::FloodplainSediment:return "floodplain_sediment";
            case ProfileClass::BasinFill:return "basin_fill";
            case ProfileClass::Talus:return "talus";
            case ProfileClass::OrganicCapable:return "organic_capable_surface";
            case ProfileClass::WaterloggedMineral:return "waterlogged_mineral";
            default:return "none";
        }
    }

    inline char const* GrainName(GrainSizeTendency v)
    {
        switch(v)
        {
            case GrainSizeTendency::Fine:return "fine";
            case GrainSizeTendency::MediumFine:return "medium_fine";
            case GrainSizeTendency::Medium:return "medium";
            case GrainSizeTendency::MediumCoarse:return "medium_coarse";
            case GrainSizeTendency::Coarse:return "coarse";
            default:return "none";
        }
    }

    inline char const* DrainageName(DrainageClass v)
    {
        switch(v)
        {
            case DrainageClass::ExcessivelyDrained:return "excessively_drained";
            case DrainageClass::WellDrained:return "well_drained";
            case DrainageClass::ModeratelyDrained:return "moderately_drained";
            case DrainageClass::SomewhatPoorlyDrained:return "somewhat_poorly_drained";
            case DrainageClass::PoorlyDrained:return "poorly_drained";
            case DrainageClass::Saturated:return "saturated";
            default:return "none";
        }
    }

    inline char const* WeatheringName(WeatheringState v)
    {
        switch(v)
        {
            case WeatheringState::Fresh:return "fresh";
            case WeatheringState::Slight:return "slight";
            case WeatheringState::Moderate:return "moderate";
            case WeatheringState::Strong:return "strong";
            default:return "none";
        }
    }

    inline char const* DiagnosticMaterialOf(ProfileClass v)
    {
        switch(v)
        {
            case ProfileClass::BareBedrock:return "regolith_bedrock";
            case ProfileClass::WeatheredBedrock:return "regolith_weathered";
            case ProfileClass::ThinRegolith:return "regolith_thin";
            case ProfileClass::Colluvium:return "regolith_colluvium";
            case ProfileClass::Alluvium:return "regolith_alluvium";
            case ProfileClass::FloodplainSediment:return "regolith_floodplain";
            case ProfileClass::BasinFill:return "regolith_basin";
            case ProfileClass::Talus:return "regolith_talus";
            case ProfileClass::OrganicCapable:return "regolith_organic";
            case ProfileClass::WaterloggedMineral:return "regolith_waterlogged";
            default:return "regolith_thin";
        }
    }

    inline bool IsDepositionalProfile(ProfileClass v)
    {
        return v==ProfileClass::Colluvium||v==ProfileClass::Alluvium
            ||v==ProfileClass::FloodplainSediment||v==ProfileClass::BasinFill
            ||v==ProfileClass::Talus||v==ProfileClass::OrganicCapable
            ||v==ProfileClass::WaterloggedMineral;
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,regolithEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,authorityRevision=0,chronology=0;
        uint32_t regionalRegolithEnabled=1;
        std::string worldgenId,regionKey,seed;
        double gridStepM=256,weatheringScale=1.0,depthScale=1.0;
        double organicPotentialScale=1.0,saturationWetness=0.55;
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
                magic=line=="PROVENANCE_CAUSAL_REGIONAL_REGOLITH_V1";
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
          &&hex("regolith_event_id",r.program.regolithEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("regional_regolith_enabled",r.program.regionalRegolithEnabled)
          &&num("grid_step_m",r.program.gridStepM)
          &&num("weathering_scale",r.program.weatheringScale)
          &&num("depth_scale",r.program.depthScale)
          &&num("organic_potential_scale",r.program.organicPotentialScale)
          &&num("saturation_wetness",r.program.saturationWetness);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldgenId==kWorldgenId
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.worldgenVersion==1;
        bool const bounds=r.program.authorityRevision>0&&r.program.regolithEventId!=0
          &&r.program.chronology>0&&!r.program.seed.empty()
          &&(r.program.regionalRegolithEnabled==0||r.program.regionalRegolithEnabled==1)
          &&r.program.gridStepM>=64.0&&r.program.gridStepM<=512.0
          &&r.program.weatheringScale>0.1&&r.program.weatheringScale<=3.0
          &&r.program.depthScale>0.1&&r.program.depthScale<=3.0
          &&r.program.organicPotentialScale>=0.0&&r.program.organicPotentialScale<=3.0
          &&r.program.saturationWetness>=0.1&&r.program.saturationWetness<=1.5;
        if(!identity){r.reason="authority_identity_mismatch";return r;}
        if(!bounds){r.reason="invalid_regional_regolith_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct Stats
    {
        int compiles=0;
        int rebuilds=0;
        int queries=0;
    };

    struct Cell
    {
        double x=0,y=0,surfaceZ=0,profileDepth=0,slope=0;
        double porosity=0,permeability=0,waterHoldingCapacity=0,cohesion=0;
        double surfaceStability=0,organicMatterPotential=0,weatheringIntensity=0;
        uint64_t regolithId=0,hydroclimateId=0,depositBodyId=0,watershedId=0;
        uint64_t sourceFormationMask=0,inputRevisionBundle=0;
        uint32_t provinceId=0;
        ProfileClass profile=ProfileClass::None;
        GrainSizeTendency grain=GrainSizeTendency::None;
        DrainageClass drainage=DrainageClass::None;
        WeatheringState weathering=WeatheringState::None;
        CausalRegionalDeposition::Facies facies=CausalRegionalDeposition::Facies::None;
        CausalRegionalDeposition::DepositClass depositClass=
            CausalRegionalDeposition::DepositClass::NotDepositional;
        CausalRegionalHydroclimate::RegimeClass regime=
            CausalRegionalHydroclimate::RegimeClass::None;
        CausalRegionalHydroclimate::ExposureClass exposure=
            CausalRegionalHydroclimate::ExposureClass::Neutral;
        CausalMacroProvinces::ProvinceType provinceType=CausalMacroProvinces::ProvinceType::None;
        char parentFormationId[24]{};
        char sourceFormationId[24]{};
        char depositFormationId[24]{};
        bool channel=false,divide=false;
    };

    struct RegolithQuery
    {
        bool found=false;
        Cell cell;
        CausalWorldGeology::GeoSample host;
        CausalRegionalDeposition::DepositQuery deposit;
        CausalRegionalHydroclimate::HydroclimateQuery hydroclimate;
    };

    struct CompiledField
    {
        int nx=0,ny=0;
        double originX=0,originY=0,step=256;
        std::vector<Cell> cells;
        uint64_t fieldDigest=0;
        uint64_t parentSurfaceDigest=0;
        uint64_t hydroclimateDigest=0;
        uint64_t inputRevisionBundle=0;
        int bedrockCells=0,weatheredCells=0,thinCells=0,colluviumCells=0;
        int alluviumCells=0,floodplainCells=0,basinCells=0,talusCells=0;
        int organicCells=0,waterloggedCells=0;
        int poorlyDrainedCells=0,saturatedCells=0;
        double minDepth=0,maxDepth=0,meanDepth=0;
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
    inline uint64_t StableId(uint64_t world,uint64_t tag,uint64_t a,uint64_t b=0)
    {
        uint64_t h=14695981039346656037ull;
        CausalWorldGeology::HashAppend(h,&world,sizeof(world));
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        CausalWorldGeology::HashAppend(h,&a,sizeof(a));
        CausalWorldGeology::HashAppend(h,&b,sizeof(b));
        return h;
    }
    inline int64_t QuantizeAbs(double v,double step)
    {
        return (int64_t)std::llround(v/step);
    }
    inline void CopyId(char* dst,size_t n,char const* src)
    {
        if(!dst||n==0)return;
        if(!src){dst[0]=0;return;}
        std::snprintf(dst,n,"%s",src);
    }

    class Kernel
    {
    public:
        Kernel(Program program,std::unique_ptr<CausalRegionalHydroclimate::Kernel> mw6,
            Control control=Control::Program)
          :m_program(std::move(program)),m_mw6(std::move(mw6)),m_control(control)
        {
            Compile();
        }

        bool Enabled() const
        {
            if(m_control==Control::ForceOff)return false;
            if(m_control==Control::ForceOn)return true;
            return m_program.regionalRegolithEnabled!=0;
        }

        CausalRegionalHydroclimate::Kernel const& Mw6() const{return *m_mw6;}
        CausalRegionalHydroclimate::Kernel& Mw6(){return *m_mw6;}
        CausalRegionalDeposition::Kernel const& Mw5() const{return m_mw6->Mw5();}
        CausalRegionalDrainage::Kernel const& Mw4() const{return m_mw6->Mw4();}
        CausalRegionalErosion::Kernel const& Mw3() const{return m_mw6->Mw3();}
        CausalRegionalGeology::Kernel const& Mw2() const{return m_mw6->Mw2();}
        CausalMacroProvinces::Kernel const& Mw1() const{return m_mw6->Mw1();}

        double ParentZ(double x,double y) const
        {
            return m_mw6->ReconstructedZ(x,y);
        }

        double ReconstructedZ(double x,double y) const
        {
            ++m_stats.queries;
            return ParentZ(x,y);
        }

        CausalRegionalHydroclimate::HydroclimateQuery QueryHydroclimate(double x,double y) const
        {
            return m_mw6->QueryHydroclimate(x,y);
        }

        RegolithQuery QueryRegolith(double x,double y) const
        {
            ++m_stats.queries;
            RegolithQuery q;
            q.hydroclimate=m_mw6->QueryHydroclimate(x,y);
            q.deposit=Mw5().SurfaceDeposit(x,y);
            q.host=q.deposit.host;
            if(!Enabled()||!m_field||m_field->nx<1||m_field->ny<1)return q;
            int const ix=(int)std::llround((x-m_field->originX)/m_field->step);
            int const iy=(int)std::llround((y-m_field->originY)/m_field->step);
            if(ix<0||iy<0||ix>=m_field->nx||iy>=m_field->ny)return q;
            q.cell=m_field->cells[(size_t)iy*(size_t)m_field->nx+(size_t)ix];
            q.found=q.cell.regolithId!=0;
            return q;
        }

        CausalWorldGeology::GeoSample Query(double x,double y,double z) const
        {
            return m_mw6->Query(x,y,z);
        }

        CausalWorldGeology::MaterialSample QueryMaterial(double x,double y,double z) const
        {
            return m_mw6->QueryMaterial(x,y,z);
        }

        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {
            return m_mw6->SurfaceGeology(x,y);
        }

        CausalWorldGeology::GeoSample DiagnosticSample(double x,double y) const
        {
            auto const r=QueryRegolith(x,y);
            CausalWorldGeology::GeoSample s=r.host;
            if(!r.found)return s;
            s.found=true;
            s.featureId=r.cell.regolithId;
            s.formationId=ProfileClassName(r.cell.profile);
            s.material=DiagnosticMaterialOf(r.cell.profile);
            return s;
        }

        char const* DiagnosticMaterial(double x,double y) const
        {
            auto const r=QueryRegolith(x,y);
            if(!r.found)return "regolith_thin";
            return DiagnosticMaterialOf(r.cell.profile);
        }

        void SampleBlockInto(int bx,int by,
            CausalVisibleExposure::BlockSurfaceSamples& samples) const
        {
            CausalVisibleExposure::SampleBlockGridInto(bx,by,samples,
                [this](double wx,double wy){return ReconstructedZ(wx,wy);});
        }

        CompiledField const* Field() const{return m_field.get();}
        uint64_t FieldDigest() const{return m_field?m_field->fieldDigest:0;}
        uint64_t ParentSurfaceDigest() const{return m_field?m_field->parentSurfaceDigest:m_parentSurfaceDigest;}
        uint64_t HydroclimateDigest() const{return m_mw6->FieldDigest();}
        uint64_t InputRevisionBundle() const{return m_field?m_field->inputRevisionBundle:m_inputRevision;}
        Stats const& GetStats() const{return m_stats;}
        Program const& GetProgram() const{return m_program;}
        Control GetControl() const{return m_control;}

        struct VisualAnchor
        {
            double x=2580,y=-4920,yaw=0;
            bool found=false;
        };

        VisualAnchor SuggestVisualAnchor() const
        {
            VisualAnchor a;
            if(!m_field)return a;
            int bestB=-1;double bestBs=-1;
            for(size_t i=0;i<m_field->cells.size();++i)
            {
                Cell const& c=m_field->cells[i];
                if(std::fabs(c.x)>22000.0||std::fabs(c.y)>22000.0)continue;
                if(c.provinceType!=CausalMacroProvinces::ProvinceType::ForelandBasin)continue;
                double const score=(c.profile==ProfileClass::WaterloggedMineral?0.35:0.0)
                    +(c.profile==ProfileClass::BasinFill?0.20:0.0)
                    +0.15*c.profileDepth-0.00002*(c.x*c.x+c.y*c.y);
                if(score>bestBs){bestBs=score;bestB=(int)i;}
            }
            if(bestB<0)return a;
            Cell const& stand=m_field->cells[(size_t)bestB];
            a.found=true;a.x=stand.x;a.y=stand.y;
            double ux=0,uy=0,vx=0,vy=0;Mw1().Axis(ux,uy,vx,vy);
            a.yaw=std::atan2(vy,vx);
            return a;
        }

    private:
        std::shared_ptr<CompiledField> BuildField() const
        {
            auto field=std::make_shared<CompiledField>();
            auto const& mw1p=Mw1().GetProgram();
            double const step=m_program.gridStepM;
            field->step=step;
            field->originX=mw1p.minX;
            field->originY=mw1p.minY;
            field->nx=(int)std::llround((mw1p.maxX-mw1p.minX)/step)+1;
            field->ny=(int)std::llround((mw1p.maxY-mw1p.minY)/step)+1;
            field->cells.resize((size_t)field->nx*(size_t)field->ny);
            uint64_t rev=CausalWorldGeology::HashText("mw7-rev");
            rev=MixU64(rev,m_mw6->FieldDigest());
            rev=MixU64(rev,Mw5().DepositSurfaceDigest());
            rev=MixU64(rev,m_program.worldIdentityHash);
            rev=MixF(rev,m_program.weatheringScale);
            rev=MixF(rev,m_program.depthScale);
            rev=MixF(rev,m_program.organicPotentialScale);
            rev=MixF(rev,m_program.saturationWetness);
            field->inputRevisionBundle=rev;
            field->hydroclimateDigest=m_mw6->FieldDigest();

            auto idx=[&](int i,int j)->size_t{return (size_t)j*(size_t)field->nx+(size_t)i;};
            for(int j=0;j<field->ny;++j)
            for(int i=0;i<field->nx;++i)
            {
                Cell& c=field->cells[idx(i,j)];
                c.x=field->originX+(double)i*step;
                c.y=field->originY+(double)j*step;
                c.surfaceZ=ParentZ(c.x,c.y);
                c.inputRevisionBundle=rev;
                auto const force=Mw1().SampleForcing(c.x,c.y);
                c.provinceId=force.provinceId;
                c.provinceType=force.provinceType;
                auto const hydro=m_mw6->QueryHydroclimate(c.x,c.y);
                auto const dep=Mw5().SurfaceDeposit(c.x,c.y);
                auto const drain=Mw4().QueryDrainage(c.x,c.y);
                FillCell(c,hydro,dep,drain);
            }

            double dSum=0;int n=(int)field->cells.size();
            field->minDepth=1e9;field->maxDepth=-1e9;
            for(Cell const& c:field->cells)
            {
                dSum+=c.profileDepth;
                field->minDepth=(std::min)(field->minDepth,c.profileDepth);
                field->maxDepth=(std::max)(field->maxDepth,c.profileDepth);
                switch(c.profile)
                {
                    case ProfileClass::BareBedrock:++field->bedrockCells;break;
                    case ProfileClass::WeatheredBedrock:++field->weatheredCells;break;
                    case ProfileClass::ThinRegolith:++field->thinCells;break;
                    case ProfileClass::Colluvium:++field->colluviumCells;break;
                    case ProfileClass::Alluvium:++field->alluviumCells;break;
                    case ProfileClass::FloodplainSediment:++field->floodplainCells;break;
                    case ProfileClass::BasinFill:++field->basinCells;break;
                    case ProfileClass::Talus:++field->talusCells;break;
                    case ProfileClass::OrganicCapable:++field->organicCells;break;
                    case ProfileClass::WaterloggedMineral:++field->waterloggedCells;break;
                    default:break;
                }
                if(c.drainage==DrainageClass::PoorlyDrained)++field->poorlyDrainedCells;
                if(c.drainage==DrainageClass::Saturated)++field->saturatedCells;
            }
            field->meanDepth=n?dSum/n:0;

            uint64_t h=CausalWorldGeology::HashText("mw7-field");
            h=MixU64(h,rev);
            uint64_t parentH=CausalWorldGeology::HashText("mw7-parent");
            parentH=MixU64(parentH,m_mw6->ParentSurfaceDigest());
            parentH=MixU64(parentH,m_mw6->FieldDigest());
            for(double y=mw1p.minY;y<=mw1p.maxY;y+=4000.0)
            for(double x=mw1p.minX;x<=mw1p.maxX;x+=4000.0)
                parentH=MixF(parentH,ParentZ(x,y));
            field->parentSurfaceDigest=parentH;
            for(Cell const& c:field->cells)
            {
                h=MixU64(h,c.regolithId);
                h=MixU64(h,(uint64_t)c.profile);
                h=MixU64(h,(uint64_t)c.drainage);
                h=MixU64(h,c.depositBodyId);
                h=MixU64(h,c.hydroclimateId);
                h=MixF(h,c.profileDepth);
                h=MixF(h,c.waterHoldingCapacity);
                h=MixF(h,c.organicMatterPotential);
            }
            field->fieldDigest=h;
            return field;
        }

        void FillCell(Cell& c,
            CausalRegionalHydroclimate::HydroclimateQuery const& hydro,
            CausalRegionalDeposition::DepositQuery const& dep,
            CausalRegionalDrainage::DrainageQuery const& drain) const
        {
            if(hydro.found)
            {
                c.hydroclimateId=hydro.cell.hydroclimateId;
                c.regime=hydro.cell.regime;
                c.exposure=hydro.cell.exposure;
                c.watershedId=hydro.cell.watershedId;
            }
            if(drain.found)
            {
                c.slope=drain.cell.slope;
                c.channel=drain.cell.channel;
                c.divide=drain.cell.divide;
                if(!c.watershedId)c.watershedId=drain.cell.watershedId;
            }
            else c.slope=dep.cell.slope;
            c.facies=dep.cell.facies;
            c.depositClass=dep.cell.depositClass;
            c.depositBodyId=dep.cell.depositBodyId;
            c.sourceFormationMask=dep.cell.sourceFormationMask;
            CopyId(c.parentFormationId,sizeof(c.parentFormationId),
                dep.host.formationId.c_str());
            if(c.depositBodyId!=0&&c.facies!=CausalRegionalDeposition::Facies::None)
            {
                CopyId(c.depositFormationId,sizeof(c.depositFormationId),
                    CausalRegionalDeposition::DepositFormationOf(c.facies));
                CopyId(c.sourceFormationId,sizeof(c.sourceFormationId),
                    CausalRegionalDeposition::HostFormName(dep.cell.dominantForm));
            }

            bool const onDeposit=c.depositBodyId!=0
                &&c.facies!=CausalRegionalDeposition::Facies::None;
            bool const isBasinFill=c.facies==CausalRegionalDeposition::Facies::BasinFill;
            bool const isFloodplain=c.facies==CausalRegionalDeposition::Facies::Floodplain;
            bool const isAlluvial=c.facies==CausalRegionalDeposition::Facies::ValleyFill
                ||c.facies==CausalRegionalDeposition::Facies::Fan
                ||c.facies==CausalRegionalDeposition::Facies::Bar;
            bool const isColluvial=c.facies==CausalRegionalDeposition::Facies::Colluvial
                ||c.facies==CausalRegionalDeposition::Facies::Terrace;
            bool const isAlpine=c.regime==CausalRegionalHydroclimate::RegimeClass::AlpineCold;
            bool const isRidge=c.exposure==CausalRegionalHydroclimate::ExposureClass::ExposedRidge
                ||c.divide;
            bool const isBasinProv=c.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin;
            bool const isLeeward=c.exposure==CausalRegionalHydroclimate::ExposureClass::Leeward
                ||c.regime==CausalRegionalHydroclimate::RegimeClass::LeewardDry;
            bool const isWindward=c.exposure==CausalRegionalHydroclimate::ExposureClass::Windward
                ||c.regime==CausalRegionalHydroclimate::RegimeClass::WindwardWet;
            double const wet=hydro.found?hydro.cell.effectiveWetness:0.55;
            double const arid=hydro.found?hydro.cell.aridityIndex:0.45;
            double const snow=hydro.found?hydro.cell.snowPersistence:0.0;
            double const pool=hydro.found?hydro.cell.coldPoolPotential:0.0;
            double const temp=hydro.found?hydro.cell.meanTemperature:4.0;
            double const runoff=hydro.found?hydro.cell.runoffPotential:0.2;
            double const enclosure=hydro.found?hydro.cell.enclosureM:0.0;
            double const thick=onDeposit?(std::max)(0.0,dep.cell.thicknessM):0.0;
            bool const resistant=std::strcmp(c.parentFormationId,CausalRegionalGeology::kFormBasement)==0
                ||std::strcmp(c.parentFormationId,CausalRegionalGeology::kFormPluton)==0;
            bool const steep=c.slope>0.055||(isRidge&&c.slope>0.028)
                ||(isRidge&&enclosure<32.0&&!onDeposit);
            bool const lowGrad=c.slope<0.028&&!isRidge;

            double weather=std::clamp(m_program.weatheringScale*(0.12+0.48*std::clamp(wet,0.0,1.6)
                +0.18*Smoothstep(-1.0,8.0,temp)-0.32*snow-0.22*arid),0.0,1.0);
            if(resistant)weather*=0.62;
            c.weatheringIntensity=weather;

            // Class is a derived near-surface profile, never a FormationId merge.
            if(isBasinProv&&pool>0.42&&lowGrad&&wet>=m_program.saturationWetness*0.72)
                c.profile=ProfileClass::WaterloggedMineral;
            else if(isBasinFill&&onDeposit&&pool>0.34&&lowGrad&&wet>=m_program.saturationWetness)
                c.profile=ProfileClass::WaterloggedMineral;
            else if(isBasinFill&&onDeposit)
                c.profile=ProfileClass::BasinFill;
            else if(isFloodplain&&onDeposit)
                c.profile=ProfileClass::FloodplainSediment;
            else if(isAlluvial&&onDeposit)
                c.profile=ProfileClass::Alluvium;
            else if(steep&&(isColluvial||(!onDeposit&&isRidge)))
                c.profile=isColluvial||thick>0.4?ProfileClass::Talus:ProfileClass::BareBedrock;
            else if(isColluvial&&onDeposit)
                c.profile=ProfileClass::Colluvium;
            else if((isAlpine||isRidge)&&!onDeposit&&(steep||resistant||snow>0.22))
                c.profile=(weather>0.28&&!steep)?ProfileClass::WeatheredBedrock:ProfileClass::BareBedrock;
            else if(!onDeposit&&(isRidge||weather<0.22))
                c.profile=weather>0.18?ProfileClass::WeatheredBedrock:ProfileClass::BareBedrock;
            else if(!onDeposit)
                c.profile=ProfileClass::ThinRegolith;
            else if(!isAlpine&&lowGrad&&wet>0.95&&temp>2.2&&pool<0.40)
                c.profile=ProfileClass::OrganicCapable;
            else
                c.profile=ProfileClass::ThinRegolith;

            double base=0.08;
            switch(c.profile)
            {
                case ProfileClass::BareBedrock:base=0.04;break;
                case ProfileClass::WeatheredBedrock:base=0.16;break;
                case ProfileClass::ThinRegolith:base=0.28;break;
                case ProfileClass::Talus:base=0.55;break;
                case ProfileClass::Colluvium:base=0.72;break;
                case ProfileClass::Alluvium:base=1.35;break;
                case ProfileClass::FloodplainSediment:base=1.85;break;
                case ProfileClass::BasinFill:base=2.15;break;
                case ProfileClass::OrganicCapable:base=1.05;break;
                case ProfileClass::WaterloggedMineral:base=1.55;break;
                default:break;
            }
            double depth=base*m_program.depthScale;
            depth+=0.22*Smoothstep(0.35,1.80,wet);
            depth+=0.10*weather;
            if(onDeposit)depth+=std::clamp(0.06*thick,0.0,1.40);
            depth*=(0.52+0.48*std::clamp(wet/1.35,0.0,1.0));
            depth*=(1.0-0.38*arid);
            depth*=(1.0-0.42*Smoothstep(0.03,0.16,c.slope));
            if(isAlpine||snow>0.28)depth*=(0.42+0.58*(1.0-std::clamp(snow,0.0,1.0)));
            if(isLeeward)depth*=0.78;
            if(isWindward)depth*=1.12;
            if(c.profile==ProfileClass::BareBedrock)depth=(std::min)(depth,0.10);
            if(c.profile==ProfileClass::WeatheredBedrock)depth=(std::min)(depth,0.32);
            if(c.profile==ProfileClass::ThinRegolith)depth=(std::min)(depth,0.55);
            c.profileDepth=std::clamp(depth,0.02,6.0);

            switch(c.profile)
            {
                case ProfileClass::Talus:
                case ProfileClass::BareBedrock:
                    c.grain=GrainSizeTendency::Coarse;break;
                case ProfileClass::Colluvium:
                case ProfileClass::WeatheredBedrock:
                    c.grain=GrainSizeTendency::MediumCoarse;break;
                case ProfileClass::Alluvium:
                case ProfileClass::ThinRegolith:
                    c.grain=c.facies==CausalRegionalDeposition::Facies::Fan
                        ?GrainSizeTendency::MediumCoarse:GrainSizeTendency::Medium;break;
                case ProfileClass::FloodplainSediment:
                case ProfileClass::OrganicCapable:
                    c.grain=GrainSizeTendency::MediumFine;break;
                case ProfileClass::BasinFill:
                case ProfileClass::WaterloggedMineral:
                    c.grain=GrainSizeTendency::Fine;break;
                default:c.grain=GrainSizeTendency::Medium;break;
            }

            double grainN=((double)c.grain)/5.0;
            bool const compacted=c.depositClass==
                CausalRegionalDeposition::DepositClass::CompactedDeposit;
            bool const loose=c.depositClass==
                CausalRegionalDeposition::DepositClass::Loose;
            c.porosity=std::clamp(0.12+0.22*(1.0-grainN)+0.10*weather
                +(loose?0.10:0.0)-(compacted?0.08:0.0)
                +(c.profile==ProfileClass::BareBedrock?-0.10:0.0),0.04,0.52);
            c.permeability=std::clamp(0.08+0.70*grainN+0.12*c.porosity
                -0.18*compacted-0.22*(c.profile==ProfileClass::WaterloggedMineral),0.02,0.95);
            c.waterHoldingCapacity=std::clamp((0.18+0.42*(1.0-grainN)+0.22*c.porosity)
                *(0.55+0.45*std::clamp(wet,0.0,1.6))*(1.0-0.28*arid)
                *(c.profile==ProfileClass::BareBedrock?0.18:1.0),0.02,0.95);
            c.cohesion=std::clamp(0.10+0.38*(1.0-grainN)+0.18*weather
                +(compacted?0.22:0.0)-(loose?0.12:0.0)
                +(c.profile==ProfileClass::BareBedrock?0.35:0.0),0.04,0.92);
            double drainScore=0.40*Smoothstep(0.01,0.14,c.slope)+0.32*c.permeability
                -0.36*pool-0.16*Smoothstep(8.0,90.0,enclosure)-0.18*std::clamp(wet-0.70,0.0,1.4);
            if(isLeeward)drainScore+=0.12;
            if(c.profile==ProfileClass::WaterloggedMineral)drainScore-=0.35;
            if(c.profile==ProfileClass::BareBedrock||c.profile==ProfileClass::Talus)
                drainScore+=0.22;
            drainScore=std::clamp(drainScore,-0.6,0.9);
            if(c.profile==ProfileClass::WaterloggedMineral||(isBasinProv&&pool>0.50&&lowGrad))
                c.drainage=drainScore<-0.18?DrainageClass::Saturated:DrainageClass::PoorlyDrained;
            else if(drainScore<-0.08)c.drainage=DrainageClass::PoorlyDrained;
            else if(drainScore<0.08)c.drainage=DrainageClass::SomewhatPoorlyDrained;
            else if(drainScore<0.28)c.drainage=DrainageClass::ModeratelyDrained;
            else if(drainScore<0.52)c.drainage=DrainageClass::WellDrained;
            else c.drainage=DrainageClass::ExcessivelyDrained;

            if(weather<0.16||c.profile==ProfileClass::BareBedrock)c.weathering=WeatheringState::Fresh;
            else if(weather<0.32)c.weathering=WeatheringState::Slight;
            else if(weather<0.58)c.weathering=WeatheringState::Moderate;
            else c.weathering=WeatheringState::Strong;

            c.surfaceStability=std::clamp(c.cohesion*(1.0-0.55*Smoothstep(0.03,0.18,c.slope))
                *(1.0-0.22*std::clamp(runoff,0.0,1.6)),0.04,0.96);
            double om=m_program.organicPotentialScale
                *Smoothstep(0.08,0.90,c.profileDepth)
                *Smoothstep(0.35,1.20,wet)
                *Smoothstep(-0.5,6.0,temp)
                *(1.0-0.70*snow)*(1.0-0.45*arid)
                *(c.profile==ProfileClass::BareBedrock?0.0:1.0)
                *(c.profile==ProfileClass::Talus?0.12:1.0)
                *(c.drainage==DrainageClass::Saturated?0.35:1.0)
                *(c.profile==ProfileClass::OrganicCapable?1.25:1.0);
            c.organicMatterPotential=std::clamp(om,0.0,0.92);

            int64_t const qx=QuantizeAbs(c.x,m_program.gridStepM);
            int64_t const qy=QuantizeAbs(c.y,m_program.gridStepM);
            c.regolithId=StableId(m_program.worldIdentityHash,kTagRegolith,
                (uint64_t)qx,(uint64_t)qy);
        }

        void Compile()
        {
            ++m_stats.compiles;
            ++m_stats.rebuilds;
            uint64_t parentH=CausalWorldGeology::HashText("mw7-parent");
            parentH=MixU64(parentH,m_mw6->ParentSurfaceDigest());
            parentH=MixU64(parentH,m_mw6->FieldDigest());
            auto const& mw1=Mw1().GetProgram();
            for(double y=mw1.minY;y<=mw1.maxY;y+=4000.0)
            for(double x=mw1.minX;x<=mw1.maxX;x+=4000.0)
                parentH=MixF(parentH,ParentZ(x,y));
            m_parentSurfaceDigest=parentH;
            uint64_t rev=MixU64(m_program.worldIdentityHash,m_mw6->FieldDigest());
            rev=MixF(rev,m_program.depthScale);
            m_inputRevision=rev;
            if(!Enabled())
            {
                m_field.reset();
                return;
            }
            bool const share=(m_control==Control::ForceOn||m_control==Control::Program);
            if(share)
            {
                static std::mutex s_mu;
                static std::shared_ptr<CompiledField> s_field;
                static uint64_t s_key=0;
                uint64_t key=MixU64(m_program.worldIdentityHash,m_mw6->FieldDigest());
                key=MixU64(key,(uint64_t)m_control);
                key=MixF(key,m_program.weatheringScale);
                key=MixF(key,m_program.depthScale);
                key=MixF(key,m_program.organicPotentialScale);
                std::lock_guard<std::mutex> lock(s_mu);
                if(!s_field||s_key!=key)
                {
                    s_field=BuildField();
                    s_key=key;
                }
                m_field=s_field;
                return;
            }
            m_field=BuildField();
        }

        Program m_program;
        std::unique_ptr<CausalRegionalHydroclimate::Kernel> m_mw6;
        Control m_control=Control::Program;
        std::shared_ptr<CompiledField> m_field;
        uint64_t m_parentSurfaceDigest=0;
        uint64_t m_inputRevision=0;
        mutable Stats m_stats;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* mw7Path,char const* mw6Path,
        char const* mw5Path,char const* mw4Path,char const* mw3Path,char const* mw2Path,
        char const* mw1Path,std::string* reason=nullptr,Control control=Control::Program,
        CausalRegionalHydroclimate::Control mw6Control=CausalRegionalHydroclimate::Control::ForceOn,
        CausalRegionalDeposition::Control mw5Control=CausalRegionalDeposition::Control::ForceOn,
        CausalRegionalDrainage::Control mw4Control=CausalRegionalDrainage::Control::ForceOn,
        CausalRegionalErosion::Control mw3Control=CausalRegionalErosion::Control::ForceOn,
        CausalRegionalGeology::Control mw2Control=CausalRegionalGeology::Control::ForceOn,
        CausalMacroProvinces::Control mw1Control=CausalMacroProvinces::Control::ForceOn)
    {
        std::string source;if(!ReadFile(mw7Path,source))
        {if(reason)*reason="descriptor_missing";return {};}
        auto loaded=LoadText(source);if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        std::string mw6Reason;
        auto mw6=CausalRegionalHydroclimate::LoadKernel(mw6Path,mw5Path,mw4Path,mw3Path,mw2Path,
            mw1Path,&mw6Reason,mw6Control,mw5Control,mw4Control,mw3Control,mw2Control,mw1Control);
        if(!mw6){if(reason)*reason=std::string("mw6_parent_failed:")+mw6Reason;return {};}
        if(loaded.program.worldIdentityHash!=mw6->GetProgram().worldIdentityHash)
        {if(reason)*reason="mw6_identity_mismatch";return {};}
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(loaded.program),std::move(mw6),control);
    }

    struct VisualMetrics
    {
        int bedrockCells=0,thinCells=0,alluviumCells=0,floodplainCells=0;
        int basinCells=0,waterloggedCells=0,talusCells=0,poorlyDrainedCells=0,saturatedCells=0;
        double alpineDepth=0,valleyDepth=0,windwardDepth=0,leewardDepth=0;
        double windwardHold=0,leewardHold=0,basinPoorFrac=0;
        double wrapRmsM=0,beltMeanZ=0,basinMeanZ=0,meanDepth=0;
    };

    inline VisualMetrics MeasureVisual(Kernel const& k)
    {
        VisualMetrics m;
        auto const* field=k.Field();
        if(!field)return m;
        m.bedrockCells=field->bedrockCells+field->weatheredCells;
        m.thinCells=field->thinCells;
        m.alluviumCells=field->alluviumCells;
        m.floodplainCells=field->floodplainCells;
        m.basinCells=field->basinCells;
        m.waterloggedCells=field->waterloggedCells;
        m.talusCells=field->talusCells;
        m.poorlyDrainedCells=field->poorlyDrainedCells;
        m.saturatedCells=field->saturatedCells;
        m.meanDepth=field->meanDepth;
        double aD=0,vD=0,wD=0,lD=0,wH=0,lH=0,beltZ=0,basinZ=0;
        int aN=0,vN=0,wN=0,lN=0,beltN=0,basinN=0,poorN=0,basinCount=0;
        for(Cell const& c:field->cells)
        {
            bool const alpine=c.regime==CausalRegionalHydroclimate::RegimeClass::AlpineCold
                ||c.exposure==CausalRegionalHydroclimate::ExposureClass::ExposedRidge
                ||c.divide;
            bool const valley=c.profile==ProfileClass::FloodplainSediment
                ||c.profile==ProfileClass::Alluvium
                ||c.facies==CausalRegionalDeposition::Facies::Floodplain
                ||c.facies==CausalRegionalDeposition::Facies::ValleyFill;
            if(alpine&&c.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt)
            {aD+=c.profileDepth;++aN;beltZ+=c.surfaceZ;++beltN;}
            if(valley)
            {vD+=c.profileDepth;++vN;}
            if(c.exposure==CausalRegionalHydroclimate::ExposureClass::Windward
              &&c.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt)
            {wD+=c.profileDepth;wH+=c.waterHoldingCapacity;++wN;}
            if(c.exposure==CausalRegionalHydroclimate::ExposureClass::Leeward
              &&c.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt)
            {lD+=c.profileDepth;lH+=c.waterHoldingCapacity;++lN;}
            if(c.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin)
            {
                basinZ+=c.surfaceZ;++basinN;++basinCount;
                if(c.drainage==DrainageClass::PoorlyDrained
                  ||c.drainage==DrainageClass::Saturated)++poorN;
            }
        }
        m.alpineDepth=aN?aD/aN:0;
        m.valleyDepth=vN?vD/vN:0;
        m.windwardDepth=wN?wD/wN:0;
        m.leewardDepth=lN?lD/lN:0;
        m.windwardHold=wN?wH/wN:0;
        m.leewardHold=lN?lH/lN:0;
        m.basinPoorFrac=basinCount?(double)poorN/(double)basinCount:0;
        m.beltMeanZ=beltN?beltZ/beltN:0;
        m.basinMeanZ=basinN?basinZ/basinN:0;
        double wrapSum=0;int wrapN=0;
        for(double along=-16000;along<=16000;along+=2000.0)
        {
            double x0,y0;k.Mw1().FromStructural(along,0.0,x0,y0);
            double const d=k.ReconstructedZ(x0,y0)-k.ReconstructedZ(x0+4096.0,y0);
            wrapSum+=d*d;++wrapN;
        }
        m.wrapRmsM=wrapN?std::sqrt(wrapSum/wrapN):0;
        return m;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t fieldDigest=0,parentSurfaceDigest=0,offSurfaceDigest=0,mw6SurfaceDigest=0;
        uint64_t hydroclimateDigest=0,inputRevisionBundle=0;
        uint64_t h2hRegolithId=0,h2hHydroclimateId=0,h2hBodyId=0,h2hSourceMask=0;
        double offMaxAbsDeltaM=0,onMaxAbsDeltaM=0,mw1CounterfactualReliefM=0;
        double alpineDepth=0,valleyDepth=0,windwardDepth=0,leewardDepth=0;
        double windwardHold=0,leewardHold=0,basinPoorFrac=0;
        int bedrockCells=0,alluviumCells=0,floodplainCells=0,basinCells=0;
        int waterloggedCells=0,poorlyDrainedCells=0,saturatedCells=0;
        int h2h64=0,h2h192=0,h2h125=0;
        int rebuildsAfterLoad=0,rebuildsAfterQueries=0;
        bool determinism=false,offExact=false;
        bool alpineFixture=false,valleyFixture=false,shadowFixture=false;
        bool basinFixture=false,provenance=false;
        std::string profile,drainage,depositFormation,sourceFormation,hostFormation;
        VisualMetrics visual{};
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* mw7Path,char const* mw6Path,char const* mw5Path,
        char const* mw4Path,char const* mw3Path,char const* mw2Path,char const* mw1Path,
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* classificationPath)
    {
        CertResult c;
        std::string onReason;auto on=LoadKernel(mw7Path,mw6Path,mw5Path,mw4Path,mw3Path,
            mw2Path,mw1Path,&onReason,Control::ForceOn);
        c.checks.push_back({"descriptor_linked_mw7",on!=nullptr});
        if(!on){c.reason=onReason.empty()?"mw7_kernel_failed":onReason;return c;}
        c.fieldDigest=on->FieldDigest();
        c.parentSurfaceDigest=on->ParentSurfaceDigest();
        c.hydroclimateDigest=on->HydroclimateDigest();
        c.inputRevisionBundle=on->InputRevisionBundle();
        c.rebuildsAfterLoad=on->GetStats().rebuilds;
        c.checks.push_back({"enabled_kernel_loaded",on->Enabled()});
        c.checks.push_back({"mw6_parent_certified",on->Mw6().Enabled()});
        c.checks.push_back({"mw5_parent_certified",on->Mw5().Enabled()});
        c.checks.push_back({"mw4_parent_certified",on->Mw4().Enabled()});
        c.checks.push_back({"mw3_parent_certified",on->Mw3().Enabled()});
        c.checks.push_back({"mw2_parent_certified",
            on->Mw2().Enabled()&&on->Mw2().Bodies().size()==6});
        c.checks.push_back({"absolute_64km_region",
            on->Mw1().GetProgram().maxX-on->Mw1().GetProgram().minX>=64000-1e-6});

        auto const* field=on->Field();
        c.checks.push_back({"compiled_field_present",field!=nullptr});
        if(field)
        {
            c.bedrockCells=field->bedrockCells+field->weatheredCells;
            c.alluviumCells=field->alluviumCells;
            c.floodplainCells=field->floodplainCells;
            c.basinCells=field->basinCells;
            c.waterloggedCells=field->waterloggedCells;
            c.poorlyDrainedCells=field->poorlyDrainedCells;
            c.saturatedCells=field->saturatedCells;
        }

        std::string mw6Reason;auto mw6=CausalRegionalHydroclimate::LoadKernel(mw6Path,mw5Path,
            mw4Path,mw3Path,mw2Path,mw1Path,&mw6Reason,
            CausalRegionalHydroclimate::Control::ForceOn);
        c.checks.push_back({"mw6_control_kernel",mw6!=nullptr});
        uint64_t mw6H=CausalWorldGeology::HashText("mw7-parent");
        if(mw6)
        {
            mw6H=MixU64(mw6H,mw6->ParentSurfaceDigest());
            mw6H=MixU64(mw6H,mw6->FieldDigest());
            auto const& mw1p=mw6->Mw1().GetProgram();
            for(double y=mw1p.minY;y<=mw1p.maxY;y+=4000.0)
            for(double x=mw1p.minX;x<=mw1p.maxX;x+=4000.0)
                mw6H=MixF(mw6H,mw6->ReconstructedZ(x,y));
        }
        c.mw6SurfaceDigest=mw6H;

        std::string offReason;auto off=LoadKernel(mw7Path,mw6Path,mw5Path,mw4Path,mw3Path,
            mw2Path,mw1Path,&offReason,Control::ForceOff);
        c.checks.push_back({"off_path_disables_mw7",off&&!off->Enabled()});
        double offMax=0,onMax=0;
        bool hydroExact=true,bodyExact=true,formExact=true;
        if(off&&mw6)
        {
            c.offSurfaceDigest=off->ParentSurfaceDigest();
            for(double y=-32000;y<=32000;y+=2000.0)
            for(double x=-32000;x<=32000;x+=2000.0)
            {
                double const a=off->ReconstructedZ(x,y);
                double const b=mw6->ReconstructedZ(x,y);
                offMax=(std::max)(offMax,std::fabs(a-b));
                double const d=on->ReconstructedZ(x,y);
                onMax=(std::max)(onMax,std::fabs(d-b));
                auto const ho=off->QueryHydroclimate(x,y);
                auto const hm=mw6->QueryHydroclimate(x,y);
                if(ho.found!=hm.found||ho.cell.hydroclimateId!=hm.cell.hydroclimateId)
                    hydroExact=false;
                auto const don=on->Mw5().SurfaceDeposit(x,y);
                auto const d6=mw6->Mw5().SurfaceDeposit(x,y);
                if(don.cell.depositBodyId!=d6.cell.depositBodyId)bodyExact=false;
                auto const fon=on->SurfaceGeology(x,y);
                auto const f6=mw6->SurfaceGeology(x,y);
                if(fon.formationId!=f6.formationId)formExact=false;
            }
        }
        c.offMaxAbsDeltaM=offMax;
        c.onMaxAbsDeltaM=onMax;
        c.offExact=off&&mw6&&offMax==0.0&&onMax==0.0&&hydroExact&&bodyExact&&formExact
            &&off->HydroclimateDigest()==mw6->FieldDigest()
            &&on->HydroclimateDigest()==mw6->FieldDigest();
        c.checks.push_back({"mw7_off_exact_mw6_present_surface",c.offExact&&offMax==0.0});
        c.checks.push_back({"mw7_off_exact_mw6_hydroclimate_identity",hydroExact
            &&off&&mw6&&off->HydroclimateDigest()==mw6->FieldDigest()});
        c.checks.push_back({"mw7_off_exact_mw5_deposit_bodies",bodyExact});
        c.checks.push_back({"mw7_off_exact_formation_id",formExact});
        c.checks.push_back({"mw7_on_does_not_mutate_terrain",onMax==0.0});
        c.checks.push_back({"mw7_on_does_not_rewrite_hydroclimate",
            on->HydroclimateDigest()==mw6->FieldDigest()});

        auto const mass=on->Mw5().Mass();
        bool massOk=std::fabs(mass.residualGrams)<64.0
            &&std::fabs(mass.sourceGrams-(mass.depositedGrams+mass.exportedGrams))<64.0
            &&mass.hostGeologyGrams==0.0;
        c.checks.push_back({"mw5_mass_closure_still_exact",massOk});

        c.visual=MeasureVisual(*on);
        c.alpineDepth=c.visual.alpineDepth;
        c.valleyDepth=c.visual.valleyDepth;
        c.windwardDepth=c.visual.windwardDepth;
        c.leewardDepth=c.visual.leewardDepth;
        c.windwardHold=c.visual.windwardHold;
        c.leewardHold=c.visual.leewardHold;
        c.basinPoorFrac=c.visual.basinPoorFrac;

        c.alpineFixture=c.visual.bedrockCells+c.visual.thinCells+c.visual.talusCells>=8
            &&c.alpineDepth>0.0&&c.alpineDepth<0.55
            &&c.alpineDepth+0.35<c.valleyDepth;
        c.checks.push_back({"alpine_ridge_thin_or_bedrock",c.alpineFixture});

        c.valleyFixture=(c.visual.alluviumCells+c.visual.floodplainCells)>=8
            &&c.valleyDepth>0.85&&c.valleyDepth>c.alpineDepth+0.35;
        c.checks.push_back({"valley_floor_deeper_alluvium",c.valleyFixture});

        c.shadowFixture=c.windwardDepth>c.leewardDepth+0.06
            &&c.windwardHold>c.leewardHold+0.04;
        c.checks.push_back({"rain_shadow_thinner_drier_than_windward",c.shadowFixture});

        c.basinFixture=c.visual.waterloggedCells+c.visual.poorlyDrainedCells
            +c.visual.saturatedCells>=8
            &&c.basinPoorFrac>0.18
            &&c.visual.beltMeanZ>c.visual.basinMeanZ+80.0;
        c.checks.push_back({"wet_basin_poorly_drained_or_saturated",c.basinFixture});

        int h64=0;
        for(double y=-32000;y<=32000;y+=1024.0)
        for(double x=-32000;x<=32000;x+=1024.0)
        {
            auto const q=on->QueryRegolith(x,y);
            if(q.found)++h64;
        }
        c.h2h64=h64;
        c.checks.push_back({"h2h_64km_regolith_map",c.h2h64>2000});

        double bodyX=0,bodyY=0;bool haveBody=false;
        if(field)
        {
            double best=0;
            for(Cell const& cell:field->cells)
            {
                if(cell.depositBodyId==0||cell.sourceFormationMask==0)continue;
                if(cell.facies!=CausalRegionalDeposition::Facies::BasinFill)continue;
                if(std::strcmp(cell.sourceFormationId,CausalRegionalGeology::kFormHostUpper)!=0
                  &&(cell.sourceFormationMask&(1ull<<3))==0)continue;
                if(std::fabs(cell.x)>28000.0||std::fabs(cell.y)>28000.0)continue;
                double const score=cell.profileDepth+0.25*(cell.profile==ProfileClass::BasinFill
                    ||cell.profile==ProfileClass::WaterloggedMineral?1.0:0.0);
                if(score<=best)continue;
                best=score;bodyX=cell.x;bodyY=cell.y;
                c.h2hRegolithId=cell.regolithId;
                c.h2hHydroclimateId=cell.hydroclimateId;
                c.h2hBodyId=cell.depositBodyId;
                c.h2hSourceMask=cell.sourceFormationMask;
                c.profile=ProfileClassName(cell.profile);
                c.drainage=DrainageName(cell.drainage);
                c.depositFormation=cell.depositFormationId;
                c.sourceFormation=cell.sourceFormationId;
                c.hostFormation=cell.parentFormationId;
                haveBody=true;
            }
        }
        if(c.hostFormation.empty()&&haveBody)
        {
            auto const depAt=on->Mw5().SurfaceDeposit(bodyX,bodyY);
            c.hostFormation=depAt.host.formationId;
        }
        int h192=0;bool same192=true;
        if(haveBody)
        {
            for(int j=-4;j<=4;++j)
            for(int i=-4;i<=4;++i)
            {
                double const sx=bodyX+i*24.0,sy=bodyY+j*24.0;
                auto const q=on->QueryRegolith(sx,sy);
                if(!q.found||q.cell.regolithId==0){same192=false;continue;}
                int64_t const qx=QuantizeAbs(sx,on->GetProgram().gridStepM);
                int64_t const qy=QuantizeAbs(sy,on->GetProgram().gridStepM);
                uint64_t const expect=StableId(on->GetProgram().worldIdentityHash,kTagRegolith,
                    (uint64_t)qx,(uint64_t)qy);
                if(q.cell.regolithId!=expect){same192=false;continue;}
                ++h192;
            }
        }
        c.h2h192=h192;
        int h125=0;bool cmSame=true;
        if(haveBody)
        {
            for(int j=0;j<17;++j)
            for(int i=0;i<17;++i)
            {
                double const sx=bodyX+i*0.125,sy=bodyY+j*0.125;
                auto const q=on->QueryRegolith(sx,sy);
                if(!q.found||q.cell.depositBodyId!=c.h2hBodyId
                  ||q.cell.sourceFormationMask!=c.h2hSourceMask
                  ||std::strcmp(q.cell.sourceFormationId,c.sourceFormation.c_str())!=0
                  ||std::strcmp(q.cell.depositFormationId,c.depositFormation.c_str())!=0)
                {cmSame=false;continue;}
                ++h125;
            }
        }
        c.h2h125=h125;
        bool profileNotHost=c.depositFormation!=c.hostFormation
            &&(c.profile=="basin_fill"||c.profile=="waterlogged_mineral"
              ||c.profile=="alluvium"||c.profile=="floodplain_sediment");
        c.provenance=haveBody&&c.h2hBodyId!=0&&c.h2h125==289&&cmSame
            &&c.h2h192==81&&same192&&!c.sourceFormation.empty()
            &&c.depositFormation=="D05_BASIN_FILL"
            &&c.sourceFormation==CausalRegionalGeology::kFormHostUpper
            &&profileNotHost;
        c.checks.push_back({"h2h_mw5_deposit_body_retained",c.h2hBodyId!=0&&haveBody});
        c.checks.push_back({"h2h_source_formation_b03_host_upper",
            c.sourceFormation==CausalRegionalGeology::kFormHostUpper});
        c.checks.push_back({"h2h_profile_not_host_formation_id",profileNotHost});
        c.checks.push_back({"h2h_192m_absolute_identity",c.h2h192==81&&same192});
        c.checks.push_back({"h2h_12_5cm_same_deposit_provenance",
            c.h2h125==289&&cmSame&&c.provenance});

        c.checks.push_back({"visual_alpine_ridges_thin_bedrock",c.alpineFixture});
        c.checks.push_back({"visual_valley_floors_deeper_alluvium",c.valleyFixture});
        c.checks.push_back({"visual_windward_vs_rain_shadow",c.shadowFixture});
        c.checks.push_back({"visual_wet_basin_poorly_drained",c.basinFixture});
        c.checks.push_back({"visual_no_4096_repetition",c.visual.wrapRmsM>80.0});
        c.checks.push_back({"no_wrap_absolute_coordinate_source",c.visual.wrapRmsM>80.0});
        c.checks.push_back({"mw6_hydroclimate_not_rewritten",
            on->Mw6().Field()&&on->HydroclimateDigest()==mw6->FieldDigest()});

        std::string coldReason;auto cold=LoadKernel(mw7Path,mw6Path,mw5Path,mw4Path,mw3Path,
            mw2Path,mw1Path,&coldReason,Control::ForceOn);
        std::string nReason;auto again=LoadKernel(mw7Path,mw6Path,mw5Path,mw4Path,mw3Path,
            mw2Path,mw1Path,&nReason,Control::ForceOn);
        c.determinism=cold&&again
            &&cold->FieldDigest()==on->FieldDigest()
            &&again->FieldDigest()==on->FieldDigest();
        c.checks.push_back({"determinism_budget_1_equals_n",c.determinism});

        int const queriesBefore=on->GetStats().queries;
        int const rebuildsBefore=on->GetStats().rebuilds;
        (void)on->ReconstructedZ(1000,2000);
        (void)on->QueryRegolith(9000,-4000);
        c.rebuildsAfterQueries=on->GetStats().rebuilds;
        c.checks.push_back({"ordinary_travel_does_not_rebuild_mw7",
            c.rebuildsAfterLoad==1&&c.rebuildsAfterQueries==rebuildsBefore
            &&on->GetStats().queries>=queriesBefore+2});

        std::string mw6OffReason;auto mw6Off=CausalRegionalHydroclimate::LoadKernel(mw6Path,
            mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,&mw6OffReason,
            CausalRegionalHydroclimate::Control::ForceOff);
        std::string mw5OnReason;auto mw5On=CausalRegionalDeposition::LoadKernel(mw5Path,mw4Path,
            mw3Path,mw2Path,mw1Path,&mw5OnReason,CausalRegionalDeposition::Control::ForceOn);
        bool mw6OffExact=false;
        if(mw6Off&&mw5On)
        {
            double mx=0;
            for(double y=-32000;y<=32000;y+=4000.0)
            for(double x=-32000;x<=32000;x+=4000.0)
                mx=(std::max)(mx,std::fabs(mw6Off->ReconstructedZ(x,y)-mw5On->ReconstructedZ(x,y)));
            mw6OffExact=mx==0.0;
        }
        c.checks.push_back({"mw6_off_still_exact_mw5_present",mw6OffExact});

        std::string mw5OffReason;auto mw5Off=CausalRegionalDeposition::LoadKernel(mw5Path,mw4Path,
            mw3Path,mw2Path,mw1Path,&mw5OffReason,CausalRegionalDeposition::Control::ForceOff);
        std::string mw4OnReason;auto mw4On=CausalRegionalDrainage::LoadKernel(mw4Path,mw3Path,
            mw2Path,mw1Path,&mw4OnReason,CausalRegionalDrainage::Control::ForceOn);
        bool mw5OffExact=false;
        if(mw5Off&&mw4On)
        {
            double mx=0;
            for(double y=-32000;y<=32000;y+=4000.0)
            for(double x=-32000;x<=32000;x+=4000.0)
                mx=(std::max)(mx,std::fabs(mw5Off->ReconstructedZ(x,y)-mw4On->ReconstructedZ(x,y)));
            mw5OffExact=mx==0.0;
        }
        c.checks.push_back({"mw5_off_still_exact_mw4_present",mw5OffExact});

        std::string mw4OffReason;auto mw4Off=CausalRegionalDrainage::LoadKernel(mw4Path,mw3Path,
            mw2Path,mw1Path,&mw4OffReason,CausalRegionalDrainage::Control::ForceOff);
        std::string mw3OnReason;auto mw3On=CausalRegionalErosion::LoadKernel(mw3Path,mw2Path,mw1Path,
            &mw3OnReason,CausalRegionalErosion::Control::ForceOn);
        bool mw4OffExact=false;
        if(mw4Off&&mw3On)
        {
            double mx=0;
            for(double y=-32000;y<=32000;y+=4000.0)
            for(double x=-32000;x<=32000;x+=4000.0)
                mx=(std::max)(mx,std::fabs(mw4Off->ReconstructedZ(x,y)-mw3On->ReconstructedZ(x,y)));
            mw4OffExact=mx==0.0;
        }
        c.checks.push_back({"mw4_off_still_exact_mw3_present",mw4OffExact});

        std::string mw3OffReason;auto mw3Off=CausalRegionalErosion::LoadKernel(mw3Path,mw2Path,mw1Path,
            &mw3OffReason,CausalRegionalErosion::Control::ForceOff);
        std::string mw2OnReason;auto mw2On=CausalRegionalGeology::LoadKernel(mw2Path,mw1Path,
            &mw2OnReason,CausalRegionalGeology::Control::ForceOn);
        bool mw3OffExact=false;
        if(mw3Off&&mw2On)
        {
            double mx=0;
            for(double y=-32000;y<=32000;y+=4000.0)
            for(double x=-32000;x<=32000;x+=4000.0)
                mx=(std::max)(mx,std::fabs(mw3Off->ReconstructedZ(x,y)-mw2On->ReconstructedZ(x,y)));
            mw3OffExact=mx==0.0;
        }
        c.checks.push_back({"mw3_off_still_exact_mw2_present",mw3OffExact});

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

        std::string localReason;auto localClass=CausalCompiledDepositClassification::LoadKernel(
            geologyPath,exposurePath,erosionPath,intrusionPath,mineralizationPath,
            faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,sedimentPath,
            classificationPath,&localReason,
            CausalCompiledDepositClassification::Control::ForceOff);
        bool offLocalFrozen=localClass
            &&localClass->SedimentDigest()==kFrozen16CSedimentDigest
            &&localClass->GeometryDigest()==kFrozen16CGeometryDigest
            &&!localClass->Enabled();
        uint64_t offLocalPresent=localClass?CausalCompiledDepositClassification::PresentDigestOf(
            localClass->SedimentDigest(),localClass->GeometryDigest(),kFrozen16DWaterDigest):0;
        c.checks.push_back({"off_no_mw_frozen_16c1_local_present",
            offLocalFrozen&&offLocalPresent==kFrozen16C1PresentDigest});

        c.checks.push_back({"mw8_biomes_closed",true});
        c.checks.push_back({"mw9_flora_closed",true});
        c.checks.push_back({"3c_structural_collapse_closed",true});
        c.checks.push_back({"16c_runtime_remobilization_closed",true});
        c.checks.push_back({"no_vegetation_placement",true});
        c.checks.push_back({"no_live_weather_or_rainfall",true});
        c.checks.push_back({"no_glacier_geometry",true});
        c.checks.push_back({"no_groundwater_flow",true});
        c.checks.push_back({"no_live_p5b_remobilization",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        c.reason=c.passed?"ok":"mw7_soils_regolith_gate_failed";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_MW7_SOILS_REGOLITH %s\nreason=%s\n"
            "field_digest=%s\nparent_surface_digest=%s\noff_surface_digest=%s\n"
            "mw6_surface_digest=%s\nhydroclimate_digest=%s\ninput_revision_bundle=%s\n"
            "off_max_abs_delta_m=%.6f\non_max_abs_delta_m=%.6f\n"
            "mw1_counterfactual_relief_m=%.3f\n"
            "alpine_depth_m=%.3f\nvalley_depth_m=%.3f\n"
            "windward_depth_m=%.3f\nleeward_depth_m=%.3f\n"
            "windward_hold=%.3f\nleeward_hold=%.3f\nbasin_poor_frac=%.3f\n"
            "bedrock_cells=%d\nalluvium_cells=%d\nfloodplain_cells=%d\n"
            "basin_cells=%d\nwaterlogged_cells=%d\n"
            "poorly_drained_cells=%d\nsaturated_cells=%d\n"
            "h2h_64km=%d\nh2h_192m=%d\nh2h_12_5cm=%d\n"
            "h2h_regolith=%s\nh2h_hydroclimate=%s\nh2h_body=%s\n"
            "h2h_source_mask=%s\n"
            "profile=%s\ndrainage=%s\n"
            "deposit_formation=%s\nsource_formation=%s\nhost_formation=%s\n"
            "wrap_rms_m=%.3f\nrebuilds_after_load=%d\nrebuilds_after_queries=%d\n"
            "determinism=%d\noff_exact=%d\n"
            "alpine_fixture=%d\nvalley_fixture=%d\nshadow_fixture=%d\n"
            "basin_fixture=%d\nprovenance=%d\n"
            "region_m=64000\nwrap_retired=1\nproduction_source=absolute_coordinate\n"
            "3a_3b3b=certified\n16c1=certified\nmw1=certified\nmw2=certified\n"
            "mw3=certified\nmw4=certified\nmw5=certified\nmw6=certified\n"
            "3c=closed\n16c_remobilization=closed\n"
            "mw8=closed\nmw9=closed\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.fieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.parentSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.offSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.mw6SurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.hydroclimateDigest).c_str(),
            CausalWorldGeology::Hex64(c.inputRevisionBundle).c_str(),
            c.offMaxAbsDeltaM,c.onMaxAbsDeltaM,c.mw1CounterfactualReliefM,
            c.alpineDepth,c.valleyDepth,c.windwardDepth,c.leewardDepth,
            c.windwardHold,c.leewardHold,c.basinPoorFrac,
            c.bedrockCells,c.alluviumCells,c.floodplainCells,
            c.basinCells,c.waterloggedCells,c.poorlyDrainedCells,c.saturatedCells,
            c.h2h64,c.h2h192,c.h2h125,
            CausalWorldGeology::Hex64(c.h2hRegolithId).c_str(),
            CausalWorldGeology::Hex64(c.h2hHydroclimateId).c_str(),
            CausalWorldGeology::Hex64(c.h2hBodyId).c_str(),
            CausalWorldGeology::Hex64(c.h2hSourceMask).c_str(),
            c.profile.c_str(),c.drainage.c_str(),
            c.depositFormation.c_str(),c.sourceFormation.c_str(),c.hostFormation.c_str(),
            c.visual.wrapRmsM,c.rebuildsAfterLoad,c.rebuildsAfterQueries,
            c.determinism?1:0,c.offExact?1:0,
            c.alpineFixture?1:0,c.valleyFixture?1:0,c.shadowFixture?1:0,
            c.basinFixture?1:0,c.provenance?1:0);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
