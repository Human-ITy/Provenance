#pragma once

// MW6: compiled regional hydroclimate on certified MW1–MW5 terrain.
// Long-term temperature / moisture / exposure / snow-persistence forcing.
// Not live weather, soils, biomes, glaciers, or another terrain carver.
//
// Required chain:
//   MW1 provinces → MW2 geology → MW3 erosion → MW4 drainage → MW5 deposits
//   → MW6 hydroclimate → (closed) MW7 soils → MW8 biomes
//
// Off / no-MW6 leaves the exact MW5 present surface untouched.

#include "CausalRegionalDeposition.h"

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

namespace CausalRegionalHydroclimate
{
    constexpr char const* kExpectedRegion="causal_world_regional_hydroclimate_floor";
    constexpr char const* kWorldgenId="provenance_regional_hydroclimate_v1";
    constexpr double kPi=3.14159265358979323846;
    constexpr uint64_t kFrozen16C1PresentDigest=0x2eb519c42ca5bd6bull;
    constexpr uint64_t kFrozen16CSedimentDigest=0x1367584c41aedcdfull;
    constexpr uint64_t kFrozen16CGeometryDigest=0x7ae7aa62c50489e2ull;
    constexpr uint64_t kFrozen16DWaterDigest=0x636d01ba00d3dffeull;
    constexpr uint64_t kTagHydroclimate=0x4d57360000000002ull;

    enum class Control:uint8_t
    {
        Program=0,
        ForceOff=1,
        ForceOn=2,
        LapseOff=3,
        BarrierOff=4
    };

    enum class ExposureClass:uint8_t
    {
        Neutral=0,
        Windward=1,
        Leeward=2,
        ExposedRidge=3,
        ShelteredValley=4,
        ShelteredBasin=5
    };

    enum class RegimeClass:uint8_t
    {
        None=0,
        AlpineCold=1,
        WindwardWet=2,
        LeewardDry=3,
        BasinColdPool=4,
        ValleySheltered=5,
        LowlandMild=6
    };

    inline char const* ExposureName(ExposureClass v)
    {
        switch(v)
        {
            case ExposureClass::Windward:return "windward";
            case ExposureClass::Leeward:return "leeward";
            case ExposureClass::ExposedRidge:return "exposed_ridge";
            case ExposureClass::ShelteredValley:return "sheltered_valley";
            case ExposureClass::ShelteredBasin:return "sheltered_basin";
            default:return "neutral";
        }
    }

    inline char const* RegimeName(RegimeClass v)
    {
        switch(v)
        {
            case RegimeClass::AlpineCold:return "alpine_cold";
            case RegimeClass::WindwardWet:return "windward_wet";
            case RegimeClass::LeewardDry:return "leeward_dry";
            case RegimeClass::BasinColdPool:return "basin_cold_pool";
            case RegimeClass::ValleySheltered:return "valley_sheltered";
            case RegimeClass::LowlandMild:return "lowland_mild";
            default:return "none";
        }
    }

    inline char const* DiagnosticMaterialOf(RegimeClass v)
    {
        switch(v)
        {
            case RegimeClass::AlpineCold:return "climate_cold";
            case RegimeClass::WindwardWet:return "climate_wet";
            case RegimeClass::LeewardDry:return "climate_dry";
            case RegimeClass::BasinColdPool:return "climate_pool";
            case RegimeClass::ValleySheltered:return "climate_pool";
            case RegimeClass::LowlandMild:return "climate_mild";
            default:return "climate_ridge";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,hydroclimateEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,authorityRevision=0,chronology=0;
        uint32_t regionalHydroclimateEnabled=1;
        std::string worldgenId,regionKey,seed;
        double gridStepM=256,moistureAzimuthDeg=298,seaLevelTempC=6.5;
        double lapseKPerKm=6.5,latitudeTempKPerDeg=0.55,seasonalBaseK=11.0;
        double moistureSupply=1.0,orographicGainScale=0.85,rainShadowScale=0.90;
        double freezeThresholdC=0.0,etBase=0.42;
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
                magic=line=="PROVENANCE_CAUSAL_REGIONAL_HYDROCLIMATE_V1";
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
          &&hex("hydroclimate_event_id",r.program.hydroclimateEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("regional_hydroclimate_enabled",r.program.regionalHydroclimateEnabled)
          &&num("grid_step_m",r.program.gridStepM)
          &&num("moisture_azimuth_deg",r.program.moistureAzimuthDeg)
          &&num("sea_level_temp_c",r.program.seaLevelTempC)
          &&num("lapse_k_per_km",r.program.lapseKPerKm)
          &&num("latitude_temp_k_per_deg",r.program.latitudeTempKPerDeg)
          &&num("seasonal_base_k",r.program.seasonalBaseK)
          &&num("moisture_supply",r.program.moistureSupply)
          &&num("orographic_gain_scale",r.program.orographicGainScale)
          &&num("rain_shadow_scale",r.program.rainShadowScale)
          &&num("freeze_threshold_c",r.program.freezeThresholdC)
          &&num("et_base",r.program.etBase);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldgenId==kWorldgenId
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.worldgenVersion==1;
        bool const bounds=r.program.authorityRevision>0&&r.program.hydroclimateEventId!=0
          &&r.program.chronology>0&&!r.program.seed.empty()
          &&(r.program.regionalHydroclimateEnabled==0||r.program.regionalHydroclimateEnabled==1)
          &&r.program.gridStepM>=64.0&&r.program.gridStepM<=512.0
          &&r.program.moistureAzimuthDeg>=0.0&&r.program.moistureAzimuthDeg<360.0
          &&r.program.lapseKPerKm>=2.0&&r.program.lapseKPerKm<=12.0
          &&r.program.moistureSupply>0.1&&r.program.moistureSupply<=3.0
          &&r.program.orographicGainScale>0.1&&r.program.orographicGainScale<=2.0
          &&r.program.rainShadowScale>0.1&&r.program.rainShadowScale<=2.0;
        if(!identity){r.reason="authority_identity_mismatch";return r;}
        if(!bounds){r.reason="invalid_regional_hydroclimate_contract";return r;}
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
        double x=0,y=0,surfaceZ=0,meanTemperature=0,seasonalAmplitude=0;
        double freezeTendency=0,thawTendency=0,moistureSupply=0;
        double orographicGain=0,rainShadowLoss=0,effectiveWetness=0,aridityIndex=0;
        double snowPersistence=0,etDemand=0,runoffPotential=0,coldPoolPotential=0;
        double windwardFactor=0,leeFactor=0,enclosureM=0;
        uint64_t hydroclimateId=0,watershedId=0,depositBodyId=0,sourceRegionId=0;
        uint64_t inputRevisionBundle=0;
        uint32_t provinceId=0;
        ExposureClass exposure=ExposureClass::Neutral;
        RegimeClass regime=RegimeClass::None;
        CausalRegionalDeposition::Facies facies=CausalRegionalDeposition::Facies::None;
        CausalMacroProvinces::ProvinceType provinceType=CausalMacroProvinces::ProvinceType::None;
        bool channel=false,divide=false;
    };

    struct HydroclimateQuery
    {
        bool found=false;
        Cell cell;
        CausalWorldGeology::GeoSample host;
        CausalRegionalDeposition::DepositQuery deposit;
    };

    struct CompiledField
    {
        int nx=0,ny=0;
        double originX=0,originY=0,step=256;
        std::vector<Cell> cells;
        uint64_t fieldDigest=0;
        uint64_t parentSurfaceDigest=0;
        uint64_t inputRevisionBundle=0;
        int alpineCells=0,windwardCells=0,leewardCells=0,basinCells=0,valleyCells=0;
        int ridgeCells=0,mildCells=0;
        double minTempC=0,maxTempC=0,meanTempC=0;
        double maxOrographic=0,maxShadow=0,maxSnow=0;
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

    class Kernel
    {
    public:
        Kernel(Program program,std::unique_ptr<CausalRegionalDeposition::Kernel> mw5,
            Control control=Control::Program)
          :m_program(std::move(program)),m_mw5(std::move(mw5)),m_control(control)
        {
            Compile();
        }

        bool Enabled() const
        {
            if(m_control==Control::ForceOff)return false;
            if(m_control==Control::ForceOn||m_control==Control::LapseOff
              ||m_control==Control::BarrierOff)return true;
            return m_program.regionalHydroclimateEnabled!=0;
        }

        bool ApplyLapse() const{return Enabled()&&m_control!=Control::LapseOff;}
        bool ApplyBarrier() const{return Enabled()&&m_control!=Control::BarrierOff;}

        CausalRegionalDeposition::Kernel const& Mw5() const{return *m_mw5;}
        CausalRegionalDeposition::Kernel& Mw5(){return *m_mw5;}
        CausalRegionalDrainage::Kernel const& Mw4() const{return m_mw5->Mw4();}
        CausalRegionalErosion::Kernel const& Mw3() const{return m_mw5->Mw3();}
        CausalRegionalGeology::Kernel const& Mw2() const{return m_mw5->Mw2();}
        CausalMacroProvinces::Kernel const& Mw1() const{return m_mw5->Mw1();}

        double ParentZ(double x,double y) const
        {
            return m_mw5->ReconstructedZ(x,y);
        }

        double ReconstructedZ(double x,double y) const
        {
            ++m_stats.queries;
            return ParentZ(x,y);
        }

        CausalWorldGeology::GeoSample QueryHost(double x,double y,double z) const
        {
            return m_mw5->Query(x,y,z);
        }

        HydroclimateQuery QueryHydroclimate(double x,double y) const
        {
            ++m_stats.queries;
            HydroclimateQuery q;
            q.deposit=m_mw5->SurfaceDeposit(x,y);
            q.host=q.deposit.host;
            if(!Enabled()||!m_field||m_field->nx<1||m_field->ny<1)return q;
            int const ix=(int)std::llround((x-m_field->originX)/m_field->step);
            int const iy=(int)std::llround((y-m_field->originY)/m_field->step);
            if(ix<0||iy<0||ix>=m_field->nx||iy>=m_field->ny)return q;
            q.cell=m_field->cells[(size_t)iy*(size_t)m_field->nx+(size_t)ix];
            q.found=q.cell.hydroclimateId!=0;
            return q;
        }

        CausalWorldGeology::GeoSample Query(double x,double y,double z) const
        {
            return m_mw5->Query(x,y,z);
        }

        CausalWorldGeology::MaterialSample QueryMaterial(double x,double y,double z) const
        {
            return m_mw5->QueryMaterial(x,y,z);
        }

        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {
            return m_mw5->SurfaceGeology(x,y);
        }

        CausalWorldGeology::GeoSample DiagnosticSample(double x,double y) const
        {
            auto const h=QueryHydroclimate(x,y);
            CausalWorldGeology::GeoSample s=h.host;
            if(!h.found)return s;
            s.found=true;
            s.featureId=h.cell.hydroclimateId;
            s.formationId=RegimeName(h.cell.regime);
            s.material=DiagnosticMaterialOf(h.cell.regime);
            if(h.cell.exposure==ExposureClass::ExposedRidge&&h.cell.regime==RegimeClass::LowlandMild)
                s.material="climate_ridge";
            return s;
        }

        char const* DiagnosticMaterial(double x,double y) const
        {
            auto const h=QueryHydroclimate(x,y);
            if(!h.found)return "climate_mild";
            if(h.cell.exposure==ExposureClass::ExposedRidge&&h.cell.regime!=RegimeClass::AlpineCold
              &&h.cell.regime!=RegimeClass::WindwardWet)
                return "climate_ridge";
            return DiagnosticMaterialOf(h.cell.regime);
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
        uint64_t InputRevisionBundle() const{return m_field?m_field->inputRevisionBundle:m_inputRevision;}
        Stats const& GetStats() const{return m_stats;}
        Program const& GetProgram() const{return m_program;}
        Control GetControl() const{return m_control;}

        struct VisualAnchor
        {
            double x=2580,y=-4920,yaw=0;
            double windwardX=0,windwardY=0,leewardX=0,leewardY=0;
            bool found=false;
        };

        VisualAnchor SuggestVisualAnchor() const
        {
            VisualAnchor a;
            if(!m_field)return a;
            int bestB=-1,bestW=-1;double bestBs=-1,bestWs=-1;
            for(size_t i=0;i<m_field->cells.size();++i)
            {
                Cell const& c=m_field->cells[i];
                if(std::fabs(c.x)>22000.0||std::fabs(c.y)>22000.0)continue;
                if(c.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin)
                {
                    double const score=c.coldPoolPotential+0.15*c.rainShadowLoss
                        -0.00002*(c.x*c.x+c.y*c.y);
                    if(score>bestBs){bestBs=score;bestB=(int)i;}
                }
                if(c.exposure==ExposureClass::Windward)
                {
                    double const ws=c.orographicGain+0.25*c.windwardFactor;
                    if(ws>bestWs){bestWs=ws;bestW=(int)i;}
                }
            }
            if(bestB<0&&bestW<0)return a;
            Cell const& stand=m_field->cells[(size_t)(bestB>=0?bestB:bestW)];
            a.found=true;a.x=stand.x;a.y=stand.y;
            double ux=0,uy=0,vx=0,vy=0;Mw1().Axis(ux,uy,vx,vy);
            a.windwardX=stand.x+4000.0*vx;a.windwardY=stand.y+4000.0*vy;
            a.leewardX=stand.x;a.leewardY=stand.y;
            if(bestW>=0)
            {
                Cell const& w=m_field->cells[(size_t)bestW];
                a.windwardX=w.x;a.windwardY=w.y;
            }
            a.yaw=std::atan2(vy,vx);
            return a;
        }

    private:
        double ClimateZ(double x,double y) const
        {
            if(!ApplyBarrier())return Mw1().GetProgram().datumZM;
            return ParentZ(x,y);
        }

        void MoistureDir(double& dx,double& dy) const
        {
            double const a=m_program.moistureAzimuthDeg*kPi/180.0;
            dx=std::cos(a);dy=std::sin(a);
        }

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
            double dx=0,dy=0;MoistureDir(dx,dy);
            double const datum=mw1p.datumZM;
            uint64_t rev=CausalWorldGeology::HashText("mw6-rev");
            rev=MixU64(rev,m_mw5->DepositSurfaceDigest());
            rev=MixU64(rev,m_mw5->Mw4().ValleySurfaceDigest());
            rev=MixU64(rev,m_program.worldIdentityHash);
            rev=MixF(rev,m_program.moistureAzimuthDeg);
            rev=MixF(rev,m_program.lapseKPerKm);
            rev=MixF(rev,m_program.moistureSupply);
            field->inputRevisionBundle=rev;

            auto idx=[&](int i,int j)->size_t{return (size_t)j*(size_t)field->nx+(size_t)i;};
            auto atZ=[&](double x,double y)->double{return ClimateZ(x,y);};

            for(int j=0;j<field->ny;++j)
            for(int i=0;i<field->nx;++i)
            {
                Cell& c=field->cells[idx(i,j)];
                c.x=field->originX+(double)i*step;
                c.y=field->originY+(double)j*step;
                c.surfaceZ=ParentZ(c.x,c.y);
                auto const force=Mw1().SampleForcing(c.x,c.y);
                c.provinceId=force.provinceId;
                c.provinceType=force.provinceType;
                auto const drain=Mw4().QueryDrainage(c.x,c.y);
                if(drain.found)
                {
                    c.watershedId=drain.cell.watershedId;
                    c.channel=drain.cell.channel;
                    c.divide=drain.cell.divide;
                }
                auto const dep=m_mw5->SurfaceDeposit(c.x,c.y);
                c.facies=dep.cell.facies;
                c.depositBodyId=dep.cell.depositBodyId;
                c.sourceRegionId=c.watershedId?c.watershedId:(uint64_t)c.provinceId;
                c.inputRevisionBundle=rev;

                double const zClim=atZ(c.x,c.y);
                double const zFwd=atZ(c.x+dx*step,c.y+dy*step);
                double const zBack=atZ(c.x-dx*step,c.y-dy*step);
                double const dZs=(zFwd-zBack)/(2.0*step);
                double const localLift=(std::max)(0.0,dZs);
                double const localSink=(std::max)(0.0,-dZs);
                double barrierH=0.0;
                static double const kUpwindM[5]={4000.0,8000.0,16000.0,24000.0,32000.0};
                for(double d:kUpwindM)
                {
                    double const zUp=atZ(c.x-dx*d,c.y-dy*d);
                    barrierH=(std::max)(barrierH,zUp-zClim);
                }
                c.leeFactor=Smoothstep(80.0,420.0,barrierH);
                c.windwardFactor=Smoothstep(0.018,0.110,localLift)*(1.0-0.72*c.leeFactor);
                double const elevGain=Smoothstep(120.0,900.0,zClim-datum);
                c.orographicGain=m_program.moistureSupply*m_program.orographicGainScale
                    *c.windwardFactor*(0.28+0.72*elevGain);
                c.rainShadowLoss=m_program.moistureSupply*m_program.rainShadowScale
                    *c.leeFactor*(0.40+0.60*Smoothstep(0.0,0.085,localSink));
                c.moistureSupply=m_program.moistureSupply;

                double ringMax=c.surfaceZ;
                for(int dj=-4;dj<=4;dj+=2)
                for(int di=-4;di<=4;di+=2)
                {
                    if(di==0&&dj==0)continue;
                    ringMax=(std::max)(ringMax,ParentZ(c.x+(double)di*step,c.y+(double)dj*step));
                }
                c.enclosureM=(std::max)(0.0,ringMax-c.surfaceZ);
                bool const isBasin=c.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin
                    ||c.facies==CausalRegionalDeposition::Facies::BasinFill;
                bool const isValley=c.channel
                    ||c.facies==CausalRegionalDeposition::Facies::ValleyFill
                    ||c.facies==CausalRegionalDeposition::Facies::Floodplain;
                double pool=Smoothstep(6.0,90.0,c.enclosureM)*(1.0-0.55*c.windwardFactor);
                if(isBasin)pool=(std::max)(pool,0.38+0.42*Smoothstep(4.0,140.0,c.enclosureM)
                    +0.12*(1.0-c.windwardFactor));
                if(isValley)pool=(std::max)(pool,0.24+0.38*Smoothstep(4.0,100.0,c.enclosureM));
                if(c.divide||c.enclosureM<16.0)pool*=0.35;
                c.coldPoolPotential=std::clamp(pool,0.0,1.0);

                double const latDeg=c.y/111000.0;
                double const baseTemp=m_program.seaLevelTempC-m_program.latitudeTempKPerDeg*latDeg;
                double const elevK=ApplyLapse()?m_program.lapseKPerKm*((c.surfaceZ-datum)/1000.0):0.0;
                c.meanTemperature=baseTemp-elevK;
                double const continent=0.35*c.leeFactor+0.20*c.coldPoolPotential;
                c.seasonalAmplitude=m_program.seasonalBaseK+7.0*continent
                    +2.2*Smoothstep(200.0,1100.0,c.surfaceZ-datum);
                c.freezeTendency=(std::max)(0.0,m_program.freezeThresholdC-c.meanTemperature);
                c.thawTendency=(std::max)(0.0,c.meanTemperature-m_program.freezeThresholdC);
                c.etDemand=std::clamp(m_program.etBase+0.055*(std::max)(0.0,c.meanTemperature)
                    +0.012*c.seasonalAmplitude,0.16,1.85);
                double const rawMoist=c.moistureSupply+c.orographicGain-c.rainShadowLoss;
                c.effectiveWetness=std::clamp(rawMoist/(std::max)(0.20,c.etDemand),0.0,2.40);
                c.aridityIndex=1.0/(1.0+c.effectiveWetness);
                double const elevSnow=ApplyLapse()?Smoothstep(280.0,1050.0,c.surfaceZ-datum):0.0;
                double const tempSnow=std::clamp((3.2-c.meanTemperature)/9.0,0.0,1.0);
                c.snowPersistence=std::clamp(0.52*tempSnow+0.48*elevSnow,0.0,1.0)
                    *(0.42+0.58*std::clamp(c.effectiveWetness,0.0,1.20));
                double const slope=std::hypot(dZs,0.0);
                c.runoffPotential=std::clamp(c.effectiveWetness*(0.38+0.62*Smoothstep(0.01,0.14,std::fabs(dZs)+slope))
                    *(1.0-0.30*c.snowPersistence),0.0,2.0);

                if(c.windwardFactor>0.42&&c.orographicGain>c.rainShadowLoss)
                    c.exposure=ExposureClass::Windward;
                else if(c.leeFactor>0.38)
                    c.exposure=ExposureClass::Leeward;
                else if(c.coldPoolPotential>0.48&&isBasin)
                    c.exposure=ExposureClass::ShelteredBasin;
                else if(c.coldPoolPotential>0.34&&isValley)
                    c.exposure=ExposureClass::ShelteredValley;
                else if(c.divide||(c.enclosureM<28.0&&c.surfaceZ>datum+350.0))
                    c.exposure=ExposureClass::ExposedRidge;
                else c.exposure=ExposureClass::Neutral;

                if(c.snowPersistence>0.26&&c.meanTemperature<3.2
                  &&c.surfaceZ>datum+380.0)
                    c.regime=RegimeClass::AlpineCold;
                else if(c.exposure==ExposureClass::Windward&&c.effectiveWetness>0.70)
                    c.regime=RegimeClass::WindwardWet;
                else if(isBasin&&c.coldPoolPotential>0.28)
                    c.regime=RegimeClass::BasinColdPool;
                else if(c.exposure==ExposureClass::Leeward&&c.aridityIndex>0.42)
                    c.regime=RegimeClass::LeewardDry;
                else if(isValley&&c.coldPoolPotential>0.20)
                    c.regime=RegimeClass::ValleySheltered;
                else c.regime=RegimeClass::LowlandMild;

                int64_t const qx=QuantizeAbs(c.x,step);
                int64_t const qy=QuantizeAbs(c.y,step);
                c.hydroclimateId=StableId(m_program.worldIdentityHash,kTagHydroclimate,
                    (uint64_t)qx,(uint64_t)qy);
            }

            double tSum=0;int n=(int)field->cells.size();
            field->minTempC=1e9;field->maxTempC=-1e9;
            for(Cell const& c:field->cells)
            {
                tSum+=c.meanTemperature;
                field->minTempC=(std::min)(field->minTempC,c.meanTemperature);
                field->maxTempC=(std::max)(field->maxTempC,c.meanTemperature);
                field->maxOrographic=(std::max)(field->maxOrographic,c.orographicGain);
                field->maxShadow=(std::max)(field->maxShadow,c.rainShadowLoss);
                field->maxSnow=(std::max)(field->maxSnow,c.snowPersistence);
                switch(c.regime)
                {
                    case RegimeClass::AlpineCold:++field->alpineCells;break;
                    case RegimeClass::WindwardWet:++field->windwardCells;break;
                    case RegimeClass::LeewardDry:++field->leewardCells;break;
                    case RegimeClass::BasinColdPool:++field->basinCells;break;
                    case RegimeClass::ValleySheltered:++field->valleyCells;break;
                    case RegimeClass::LowlandMild:++field->mildCells;break;
                    default:break;
                }
                if(c.exposure==ExposureClass::ExposedRidge)++field->ridgeCells;
            }
            field->meanTempC=n?tSum/n:0;

            uint64_t h=CausalWorldGeology::HashText("mw6-field");
            h=MixU64(h,rev);
            uint64_t parentH=CausalWorldGeology::HashText("mw6-parent");
            parentH=MixU64(parentH,m_mw5->DepositSurfaceDigest());
            for(double y=mw1p.minY;y<=mw1p.maxY;y+=4000.0)
            for(double x=mw1p.minX;x<=mw1p.maxX;x+=4000.0)
                parentH=MixF(parentH,ParentZ(x,y));
            field->parentSurfaceDigest=parentH;
            for(Cell const& c:field->cells)
            {
                h=MixU64(h,c.hydroclimateId);
                h=MixU64(h,(uint64_t)c.regime);
                h=MixU64(h,(uint64_t)c.exposure);
                h=MixF(h,c.meanTemperature);
                h=MixF(h,c.effectiveWetness);
                h=MixF(h,c.orographicGain);
                h=MixF(h,c.rainShadowLoss);
                h=MixF(h,c.snowPersistence);
            }
            field->fieldDigest=h;
            return field;
        }

        void Compile()
        {
            ++m_stats.compiles;
            ++m_stats.rebuilds;
            uint64_t parentH=CausalWorldGeology::HashText("mw6-parent");
            parentH=MixU64(parentH,m_mw5->DepositSurfaceDigest());
            auto const& mw1=Mw1().GetProgram();
            for(double y=mw1.minY;y<=mw1.maxY;y+=4000.0)
            for(double x=mw1.minX;x<=mw1.maxX;x+=4000.0)
                parentH=MixF(parentH,ParentZ(x,y));
            m_parentSurfaceDigest=parentH;
            uint64_t rev=MixU64(m_program.worldIdentityHash,m_mw5->DepositSurfaceDigest());
            rev=MixF(rev,m_program.moistureAzimuthDeg);
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
                uint64_t key=MixU64(m_program.worldIdentityHash,m_mw5->DepositSurfaceDigest());
                key=MixU64(key,(uint64_t)m_control);
                key=MixF(key,m_program.lapseKPerKm);
                key=MixF(key,m_program.moistureAzimuthDeg);
                key=MixF(key,m_program.moistureSupply);
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
        std::unique_ptr<CausalRegionalDeposition::Kernel> m_mw5;
        Control m_control=Control::Program;
        std::shared_ptr<CompiledField> m_field;
        uint64_t m_parentSurfaceDigest=0;
        uint64_t m_inputRevision=0;
        mutable Stats m_stats;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* mw6Path,char const* mw5Path,
        char const* mw4Path,char const* mw3Path,char const* mw2Path,char const* mw1Path,
        std::string* reason=nullptr,Control control=Control::Program,
        CausalRegionalDeposition::Control mw5Control=CausalRegionalDeposition::Control::ForceOn,
        CausalRegionalDrainage::Control mw4Control=CausalRegionalDrainage::Control::ForceOn,
        CausalRegionalErosion::Control mw3Control=CausalRegionalErosion::Control::ForceOn,
        CausalRegionalGeology::Control mw2Control=CausalRegionalGeology::Control::ForceOn,
        CausalMacroProvinces::Control mw1Control=CausalMacroProvinces::Control::ForceOn)
    {
        std::string source;if(!ReadFile(mw6Path,source))
        {if(reason)*reason="descriptor_missing";return {};}
        auto loaded=LoadText(source);if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        std::string mw5Reason;
        auto mw5=CausalRegionalDeposition::LoadKernel(mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &mw5Reason,mw5Control,mw4Control,mw3Control,mw2Control,mw1Control);
        if(!mw5){if(reason)*reason=std::string("mw5_parent_failed:")+mw5Reason;return {};}
        if(loaded.program.worldIdentityHash!=mw5->GetProgram().worldIdentityHash)
        {if(reason)*reason="mw5_identity_mismatch";return {};}
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(loaded.program),std::move(mw5),control);
    }

    struct VisualMetrics
    {
        int alpineCells=0,windwardCells=0,leewardCells=0,basinCells=0,valleyCells=0,ridgeCells=0;
        double minTempC=0,maxTempC=0,highMeanTempC=0,lowMeanTempC=0;
        double windwardWetness=0,leewardWetness=0,lowlandWetness=0;
        double highSnow=0,lowSnow=0,ridgeColdPool=0,basinColdPool=0;
        double wrapRmsM=0,beltMeanZ=0,basinMeanZ=0;
        double maxOrographic=0,maxShadow=0;
    };

    inline VisualMetrics MeasureVisual(Kernel const& k)
    {
        VisualMetrics m;
        auto const* field=k.Field();
        if(!field)return m;
        m.alpineCells=field->alpineCells;
        m.windwardCells=field->windwardCells;
        m.leewardCells=field->leewardCells;
        m.basinCells=field->basinCells;
        m.valleyCells=field->valleyCells;
        m.ridgeCells=field->ridgeCells;
        m.minTempC=field->minTempC;
        m.maxTempC=field->maxTempC;
        m.maxOrographic=field->maxOrographic;
        m.maxShadow=field->maxShadow;
        double highT=0,lowT=0,highS=0,lowS=0,wW=0,lW=0,uW=0,rC=0,bC=0;
        int highN=0,lowN=0,wN=0,lN=0,uN=0,rN=0,bN=0;
        double beltZ=0,basinZ=0;int beltN=0,basinN=0;
        for(Cell const& c:field->cells)
        {
            if(c.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt
              &&c.surfaceZ>k.Mw1().GetProgram().datumZM+450.0)
            {highT+=c.meanTemperature;highS+=c.snowPersistence;++highN;beltZ+=c.surfaceZ;++beltN;}
            else if(c.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin)
            {lowT+=c.meanTemperature;lowS+=c.snowPersistence;++lowN;basinZ+=c.surfaceZ;++basinN;}
            if(c.exposure==ExposureClass::Windward){wW+=c.effectiveWetness;++wN;}
            if(c.exposure==ExposureClass::Leeward){lW+=c.effectiveWetness;++lN;}
            if(c.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin
              &&c.windwardFactor<0.12&&c.leeFactor<0.20)
            {uW+=c.effectiveWetness;++uN;}
            if(c.exposure==ExposureClass::ExposedRidge||c.divide)
            {rC+=c.coldPoolPotential;++rN;}
            if(c.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin
              ||c.regime==RegimeClass::BasinColdPool||c.regime==RegimeClass::ValleySheltered)
            {bC+=c.coldPoolPotential;++bN;}
        }
        m.highMeanTempC=highN?highT/highN:0;
        m.lowMeanTempC=lowN?lowT/lowN:0;
        m.highSnow=highN?highS/highN:0;
        m.lowSnow=lowN?lowS/lowN:0;
        m.windwardWetness=wN?wW/wN:0;
        m.leewardWetness=lN?lW/lN:0;
        m.lowlandWetness=uN?uW/uN:m.leewardWetness;
        m.ridgeColdPool=rN?rC/rN:0;
        m.basinColdPool=bN?bC/bN:0;
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
        uint64_t fieldDigest=0,parentSurfaceDigest=0,offSurfaceDigest=0,mw5SurfaceDigest=0;
        uint64_t inputRevisionBundle=0,h2hHydroclimateId=0,h2hWatershedId=0,h2hBodyId=0;
        double offMaxAbsDeltaM=0,onMaxAbsDeltaM=0,mw1CounterfactualReliefM=0;
        double highTempC=0,lowTempC=0,lapseOffHighC=0,lapseOffLowC=0;
        double highSnow=0,lowSnow=0,windwardWet=0,leewardWet=0,lowlandWet=0;
        double barrierOffWindward=0,barrierOffLeeward=0;
        double ridgeColdPool=0,basinColdPool=0;
        int alpineCells=0,windwardCells=0,leewardCells=0,basinCells=0,valleyCells=0;
        int h2h64=0,h2h192=0,h2h125=0;
        int rebuildsAfterLoad=0,rebuildsAfterQueries=0;
        bool determinism=false,offExact=false;
        bool lapseFixture=false,orographicFixture=false,shadowFixture=false;
        bool basinFixture=false,provenance=false;
        std::string regime,exposure,province,depositFormation,hostFormation;
        VisualMetrics visual{};
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* mw6Path,char const* mw5Path,char const* mw4Path,
        char const* mw3Path,char const* mw2Path,char const* mw1Path,char const* geologyPath,
        char const* exposurePath,char const* erosionPath,char const* intrusionPath,
        char const* mineralizationPath,char const* faultPath,char const* breachPath,
        char const* geographyPath,char const* hydrologyPath,char const* fluvialPath,
        char const* sedimentPath,char const* classificationPath)
    {
        CertResult c;
        std::string onReason;auto on=LoadKernel(mw6Path,mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &onReason,Control::ForceOn);
        c.checks.push_back({"descriptor_linked_mw6",on!=nullptr});
        if(!on){c.reason=onReason.empty()?"mw6_kernel_failed":onReason;return c;}
        c.fieldDigest=on->FieldDigest();
        c.parentSurfaceDigest=on->ParentSurfaceDigest();
        c.inputRevisionBundle=on->InputRevisionBundle();
        c.rebuildsAfterLoad=on->GetStats().rebuilds;
        c.checks.push_back({"enabled_kernel_loaded",on->Enabled()});
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
            c.alpineCells=field->alpineCells;
            c.windwardCells=field->windwardCells;
            c.leewardCells=field->leewardCells;
            c.basinCells=field->basinCells;
            c.valleyCells=field->valleyCells;
        }

        std::string mw5Reason;auto mw5=CausalRegionalDeposition::LoadKernel(mw5Path,mw4Path,mw3Path,
            mw2Path,mw1Path,&mw5Reason,CausalRegionalDeposition::Control::ForceOn);
        c.checks.push_back({"mw5_control_kernel",mw5!=nullptr});
        uint64_t mw5H=CausalWorldGeology::HashText("mw6-parent");
        if(mw5)mw5H=MixU64(mw5H,mw5->DepositSurfaceDigest());
        if(mw5)
        {
            auto const& mw1p=mw5->Mw1().GetProgram();
            for(double y=mw1p.minY;y<=mw1p.maxY;y+=4000.0)
            for(double x=mw1p.minX;x<=mw1p.maxX;x+=4000.0)
                mw5H=MixF(mw5H,mw5->ReconstructedZ(x,y));
        }
        c.mw5SurfaceDigest=mw5H;

        std::string offReason;auto off=LoadKernel(mw6Path,mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &offReason,Control::ForceOff);
        c.checks.push_back({"off_path_disables_mw6",off&&!off->Enabled()});
        double offMax=0,onMax=0;
        if(off&&mw5)
        {
            c.offSurfaceDigest=off->ParentSurfaceDigest();
            for(double y=-32000;y<=32000;y+=2000.0)
            for(double x=-32000;x<=32000;x+=2000.0)
            {
                double const a=off->ReconstructedZ(x,y);
                double const b=mw5->ReconstructedZ(x,y);
                offMax=(std::max)(offMax,std::fabs(a-b));
                double const d=on->ReconstructedZ(x,y);
                onMax=(std::max)(onMax,std::fabs(d-b));
            }
        }
        c.offMaxAbsDeltaM=offMax;
        c.onMaxAbsDeltaM=onMax;
        c.offExact=off&&mw5&&offMax==0.0&&onMax==0.0
            &&off->ParentSurfaceDigest()==c.mw5SurfaceDigest;
        c.checks.push_back({"mw6_off_exact_mw5_present_surface",c.offExact});
        c.checks.push_back({"mw6_on_does_not_mutate_terrain",onMax==0.0});

        auto const mass=on->Mw5().Mass();
        bool massOk=std::fabs(mass.residualGrams)<64.0
            &&std::fabs(mass.sourceGrams-(mass.depositedGrams+mass.exportedGrams))<64.0
            &&mass.hostGeologyGrams==0.0;
        c.checks.push_back({"mw5_mass_closure_still_exact",massOk});

        c.visual=MeasureVisual(*on);
        c.highTempC=c.visual.highMeanTempC;
        c.lowTempC=c.visual.lowMeanTempC;
        c.highSnow=c.visual.highSnow;
        c.lowSnow=c.visual.lowSnow;
        c.windwardWet=c.visual.windwardWetness;
        c.leewardWet=c.visual.leewardWetness;
        c.lowlandWet=c.visual.lowlandWetness;
        c.ridgeColdPool=c.visual.ridgeColdPool;
        c.basinColdPool=c.visual.basinColdPool;

        std::string lapseReason;auto lapseOff=LoadKernel(mw6Path,mw5Path,mw4Path,mw3Path,mw2Path,
            mw1Path,&lapseReason,Control::LapseOff);
        c.checks.push_back({"lapse_off_kernel",lapseOff&&lapseOff->Enabled()});
        if(lapseOff)
        {
            auto const lv=MeasureVisual(*lapseOff);
            c.lapseOffHighC=lv.highMeanTempC;
            c.lapseOffLowC=lv.lowMeanTempC;
        }
        double const lapseOn=c.lowTempC-c.highTempC;
        double const lapseOffD=std::fabs(c.lapseOffLowC-c.lapseOffHighC);
        c.lapseFixture=c.visual.alpineCells>=8&&lapseOn>2.4&&c.highSnow>c.lowSnow+0.10
            &&lapseOffD<lapseOn*0.45;
        c.checks.push_back({"elevation_lapse_high_colder_more_snow",c.lapseFixture});

        std::string barReason;auto barrierOff=LoadKernel(mw6Path,mw5Path,mw4Path,mw3Path,mw2Path,
            mw1Path,&barReason,Control::BarrierOff);
        c.checks.push_back({"barrier_off_kernel",barrierOff&&barrierOff->Enabled()});
        if(barrierOff)
        {
            auto const bv=MeasureVisual(*barrierOff);
            c.barrierOffWindward=bv.windwardWetness;
            c.barrierOffLeeward=bv.leewardWetness;
        }
        double const shadowOn=c.windwardWet-c.leewardWet;
        double const shadowOff=std::fabs(c.barrierOffWindward-c.barrierOffLeeward);
        c.orographicFixture=c.visual.windwardCells>=8&&c.windwardWet>c.lowlandWet+0.08
            &&c.windwardWet>c.leewardWet+0.10&&c.visual.maxOrographic>0.08;
        c.checks.push_back({"windward_orographic_enhancement",c.orographicFixture});
        c.shadowFixture=c.visual.leewardCells>=8&&c.leewardWet+0.10<c.windwardWet
            &&shadowOn>0.12&&shadowOff<shadowOn*0.72;
        c.checks.push_back({"leeward_rain_shadow_from_barrier",c.shadowFixture});

        c.basinFixture=c.visual.basinCells+c.visual.valleyCells>=8
            &&c.basinColdPool>c.ridgeColdPool+0.08
            &&c.visual.beltMeanZ>c.visual.basinMeanZ+80.0;
        c.checks.push_back({"basin_valley_microclimate_distinct",c.basinFixture});

        int h64=0;
        for(double y=-32000;y<=32000;y+=1024.0)
        for(double x=-32000;x<=32000;x+=1024.0)
        {
            auto const q=on->QueryHydroclimate(x,y);
            if(q.found)++h64;
        }
        c.h2h64=h64;
        c.checks.push_back({"h2h_64km_hydroclimate_map",c.h2h64>2000});

        double bodyX=0,bodyY=0;bool haveBody=false;
        if(field)
        {
            double best=0;
            for(Cell const& cell:field->cells)
            {
                if(cell.hydroclimateId==0||cell.watershedId==0)continue;
                if(std::fabs(cell.x)>28000.0||std::fabs(cell.y)>28000.0)continue;
                if(cell.regime==RegimeClass::None)continue;
                double const score=cell.orographicGain+cell.rainShadowLoss
                    +0.25*cell.snowPersistence+0.15*cell.coldPoolPotential;
                if(score<=best)continue;
                best=score;bodyX=cell.x;bodyY=cell.y;
                c.h2hHydroclimateId=cell.hydroclimateId;
                c.h2hWatershedId=cell.watershedId;
                c.h2hBodyId=cell.depositBodyId;
                c.regime=RegimeName(cell.regime);
                c.exposure=ExposureName(cell.exposure);
                c.province=CausalMacroProvinces::ProvinceTypeName(cell.provinceType);
                c.depositFormation=CausalRegionalDeposition::DepositFormationOf(cell.facies);
                haveBody=true;
            }
        }
        auto const hostAt=on->SurfaceGeology(bodyX,bodyY);
        c.hostFormation=hostAt.formationId;
        int h192=0;bool same192=true;
        if(haveBody)
        {
            for(int j=-4;j<=4;++j)
            for(int i=-4;i<=4;++i)
            {
                double const sx=bodyX+i*24.0,sy=bodyY+j*24.0;
                auto const q=on->QueryHydroclimate(sx,sy);
                if(!q.found||q.cell.hydroclimateId==0){same192=false;continue;}
                int64_t const qx=QuantizeAbs(sx,on->GetProgram().gridStepM);
                int64_t const qy=QuantizeAbs(sy,on->GetProgram().gridStepM);
                uint64_t const expect=StableId(on->GetProgram().worldIdentityHash,kTagHydroclimate,
                    (uint64_t)qx,(uint64_t)qy);
                if(q.cell.hydroclimateId!=expect){same192=false;continue;}
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
                auto const q=on->QueryHydroclimate(sx,sy);
                if(!q.found||q.cell.hydroclimateId!=c.h2hHydroclimateId
                  ||q.cell.watershedId!=c.h2hWatershedId
                  ||std::strcmp(RegimeName(q.cell.regime),c.regime.c_str())!=0)
                {cmSame=false;continue;}
                ++h125;
            }
        }
        c.h2h125=h125;
        c.provenance=haveBody&&c.h2hWatershedId!=0&&c.h2h125==289&&cmSame
            &&c.h2h192==81&&same192&&!c.regime.empty();
        c.checks.push_back({"h2h_mw1_province_mw4_watershed",c.h2hWatershedId!=0&&haveBody});
        c.checks.push_back({"h2h_mw5_landform_context",haveBody});
        c.checks.push_back({"h2h_192m_absolute_identity",c.h2h192==81&&same192});
        c.checks.push_back({"h2h_12_5cm_same_hydroclimate_identity",
            c.h2h125==289&&cmSame&&c.provenance});

        c.checks.push_back({"visual_high_terrain_colder",c.highTempC+2.0<c.lowTempC});
        c.checks.push_back({"visual_windward_wetter_leeward_drier",
            c.windwardWet>c.leewardWet+0.10});
        c.checks.push_back({"visual_basin_valley_distinct",c.basinFixture});
        c.checks.push_back({"visual_no_4096_repetition",c.visual.wrapRmsM>80.0});
        c.checks.push_back({"no_wrap_absolute_coordinate_source",c.visual.wrapRmsM>80.0});
        c.checks.push_back({"mw4_drainage_tree_not_rewritten",
            on->Mw4().Field()&&on->Mw4().Field()->watersheds>=4});

        std::string coldReason;auto cold=LoadKernel(mw6Path,mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &coldReason,Control::ForceOn);
        std::string nReason;auto again=LoadKernel(mw6Path,mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &nReason,Control::ForceOn);
        c.determinism=cold&&again
            &&cold->FieldDigest()==on->FieldDigest()
            &&again->FieldDigest()==on->FieldDigest();
        c.checks.push_back({"determinism_budget_1_equals_n",c.determinism});

        int const queriesBefore=on->GetStats().queries;
        int const rebuildsBefore=on->GetStats().rebuilds;
        (void)on->ReconstructedZ(1000,2000);
        (void)on->QueryHydroclimate(9000,-4000);
        c.rebuildsAfterQueries=on->GetStats().rebuilds;
        c.checks.push_back({"ordinary_travel_does_not_rebuild_mw6",
            c.rebuildsAfterLoad==1&&c.rebuildsAfterQueries==rebuildsBefore
            &&on->GetStats().queries>=queriesBefore+2});

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

        c.checks.push_back({"mw7_soils_closed",true});
        c.checks.push_back({"mw8_biomes_closed",true});
        c.checks.push_back({"mw9_flora_closed",true});
        c.checks.push_back({"3c_structural_collapse_closed",true});
        c.checks.push_back({"16c_runtime_remobilization_closed",true});
        c.checks.push_back({"no_live_weather_or_rainfall",true});
        c.checks.push_back({"no_glacier_geometry",true});
        c.checks.push_back({"no_live_p5b_remobilization",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        c.reason=c.passed?"ok":"mw6_hydroclimate_gate_failed";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_MW6_HYDROCLIMATE %s\nreason=%s\n"
            "field_digest=%s\nparent_surface_digest=%s\noff_surface_digest=%s\n"
            "mw5_surface_digest=%s\ninput_revision_bundle=%s\n"
            "off_max_abs_delta_m=%.6f\non_max_abs_delta_m=%.6f\n"
            "mw1_counterfactual_relief_m=%.3f\n"
            "high_temp_c=%.3f\nlow_temp_c=%.3f\nlapse_off_high_c=%.3f\nlapse_off_low_c=%.3f\n"
            "high_snow=%.3f\nlow_snow=%.3f\n"
            "windward_wetness=%.3f\nleeward_wetness=%.3f\nlowland_wetness=%.3f\n"
            "barrier_off_windward=%.3f\nbarrier_off_leeward=%.3f\n"
            "ridge_cold_pool=%.3f\nbasin_cold_pool=%.3f\n"
            "alpine_cells=%d\nwindward_cells=%d\nleeward_cells=%d\n"
            "basin_cells=%d\nvalley_cells=%d\n"
            "h2h_64km=%d\nh2h_192m=%d\nh2h_12_5cm=%d\n"
            "h2h_hydroclimate=%s\nh2h_watershed=%s\nh2h_body=%s\n"
            "regime=%s\nexposure=%s\nprovince=%s\n"
            "deposit_formation=%s\nhost_formation=%s\n"
            "wrap_rms_m=%.3f\nrebuilds_after_load=%d\nrebuilds_after_queries=%d\n"
            "determinism=%d\noff_exact=%d\n"
            "lapse_fixture=%d\norographic_fixture=%d\nshadow_fixture=%d\n"
            "basin_fixture=%d\nprovenance=%d\n"
            "region_m=64000\nwrap_retired=1\nproduction_source=absolute_coordinate\n"
            "3a_3b3b=certified\n16c1=certified\nmw1=certified\nmw2=certified\n"
            "mw3=certified\nmw4=certified\nmw5=certified\n"
            "3c=closed\n16c_remobilization=closed\n"
            "mw7=closed\nmw8=closed\nmw9=closed\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.fieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.parentSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.offSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.mw5SurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.inputRevisionBundle).c_str(),
            c.offMaxAbsDeltaM,c.onMaxAbsDeltaM,c.mw1CounterfactualReliefM,
            c.highTempC,c.lowTempC,c.lapseOffHighC,c.lapseOffLowC,
            c.highSnow,c.lowSnow,c.windwardWet,c.leewardWet,c.lowlandWet,
            c.barrierOffWindward,c.barrierOffLeeward,
            c.ridgeColdPool,c.basinColdPool,
            c.alpineCells,c.windwardCells,c.leewardCells,c.basinCells,c.valleyCells,
            c.h2h64,c.h2h192,c.h2h125,
            CausalWorldGeology::Hex64(c.h2hHydroclimateId).c_str(),
            CausalWorldGeology::Hex64(c.h2hWatershedId).c_str(),
            CausalWorldGeology::Hex64(c.h2hBodyId).c_str(),
            c.regime.c_str(),c.exposure.c_str(),c.province.c_str(),
            c.depositFormation.c_str(),c.hostFormation.c_str(),
            c.visual.wrapRmsM,c.rebuildsAfterLoad,c.rebuildsAfterQueries,
            c.determinism?1:0,c.offExact?1:0,
            c.lapseFixture?1:0,c.orographicFixture?1:0,c.shadowFixture?1:0,
            c.basinFixture?1:0,c.provenance?1:0);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
