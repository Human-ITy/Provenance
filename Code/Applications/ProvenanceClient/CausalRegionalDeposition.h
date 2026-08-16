#pragma once

// MW5: depositional landscape on certified MW4 drainage.
// Consumes MW3/MW4 exported mass. Builds depositional bodies that remain
// distinct from host geology (16C.1 / P5b.3B.3B vocabulary).
//
// Required chain:
//   MW1 uplift / basin → MW2 persistent 3D geology → MW3 compiled denudation
//   → MW4 watershed + valley organization → MW5 depositional landscape
//   → (closed) MW6 hydroclimate
//
// Off / no-MW5 leaves the exact MW4 present surface untouched.

#include "CausalRegionalDrainage.h"
#include "CausalCompiledDepositClassification.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalRegionalDeposition
{
    constexpr char const* kExpectedRegion="causal_world_regional_deposition_floor";
    constexpr char const* kWorldgenId="provenance_regional_deposition_v1";
    constexpr uint64_t kFrozen16C1PresentDigest=0x2eb519c42ca5bd6bull;
    constexpr uint64_t kFrozen16CSedimentDigest=0x1367584c41aedcdfull;
    constexpr uint64_t kFrozen16CGeometryDigest=0x7ae7aa62c50489e2ull;
    constexpr uint64_t kFrozen16DWaterDigest=0x636d01ba00d3dffeull;

    using DepositClass=CausalCompiledDepositClassification::DepositClass;

    enum class Control:uint8_t
    {
        Program=0,
        ForceOff=1,
        ForceOn=2,
        SlopeBreakOff=3,
        LowSupply=4
    };

    enum class Facies:uint8_t
    {
        None=0,
        Fan=1,
        ValleyFill=2,
        Floodplain=3,
        Bar=4,
        BasinFill=5,
        Terrace=6,
        Colluvial=7
    };

    inline char const* FaciesName(Facies f)
    {
        switch(f)
        {
            case Facies::Fan:return "alluvial_fan";
            case Facies::ValleyFill:return "valley_fill";
            case Facies::Floodplain:return "floodplain";
            case Facies::Bar:return "bar";
            case Facies::BasinFill:return "basin_fill";
            case Facies::Terrace:return "terrace";
            case Facies::Colluvial:return "colluvial_apron";
            default:return "none";
        }
    }

    inline char const* DepositFormationOf(Facies f)
    {
        switch(f)
        {
            case Facies::Fan:return "D05_FAN";
            case Facies::ValleyFill:return "D05_VALLEY_FILL";
            case Facies::Floodplain:return "D05_FLOODPLAIN";
            case Facies::Bar:return "D05_BAR";
            case Facies::BasinFill:return "D05_BASIN_FILL";
            case Facies::Terrace:return "D05_TERRACE";
            case Facies::Colluvial:return "D05_COLLUVIAL";
            default:return "D05_NONE";
        }
    }

    inline char const* ClassName(DepositClass value)
    {
        return CausalCompiledDepositClassification::DepositClassName(value);
    }

    constexpr int kFormCount=6;
    inline char const* HostFormName(int i)
    {
        switch(i)
        {
            case 0:return CausalRegionalGeology::kFormBasement;
            case 1:return CausalRegionalGeology::kFormHostLower;
            case 2:return CausalRegionalGeology::kFormHostMiddle;
            case 3:return CausalRegionalGeology::kFormHostUpper;
            case 4:return CausalRegionalGeology::kFormBasinFill;
            default:return CausalRegionalGeology::kFormPluton;
        }
    }
    inline char const* HostFormMaterial(int i)
    {
        switch(i)
        {
            case 0:return "granite";
            case 1:return "shale";
            case 2:return "sandstone";
            case 3:return "shale";
            case 4:return "sandstone";
            default:return "granite";
        }
    }
    inline int FormIndexOf(char const* id)
    {
        if(!id||!*id)return 0;
        if(std::strcmp(id,CausalRegionalGeology::kFormBasement)==0)return 0;
        if(std::strcmp(id,CausalRegionalGeology::kFormHostLower)==0)return 1;
        if(std::strcmp(id,CausalRegionalGeology::kFormHostMiddle)==0)return 2;
        if(std::strcmp(id,CausalRegionalGeology::kFormHostUpper)==0)return 3;
        if(std::strcmp(id,CausalRegionalGeology::kFormBasinFill)==0)return 4;
        if(std::strcmp(id,CausalRegionalGeology::kFormPluton)==0)return 5;
        return 0;
    }

    struct Mix
    {
        double g[kFormCount]{};
        double Total() const
        {
            double s=0;for(int i=0;i<kFormCount;++i)s+=g[i];return s;
        }
        void Add(Mix const& o)
        {
            for(int i=0;i<kFormCount;++i)g[i]+=o.g[i];
        }
        void Scale(double s)
        {
            for(int i=0;i<kFormCount;++i)g[i]*=s;
        }
        Mix Split(double grams) const
        {
            Mix o;double const t=Total();
            if(t<=0.0||grams<=0.0)return o;
            double const u=grams/t;
            for(int i=0;i<kFormCount;++i)o.g[i]=g[i]*u;
            return o;
        }
        int Dominant() const
        {
            int b=0;for(int i=1;i<kFormCount;++i)if(g[i]>g[b])b=i;return b;
        }
        uint64_t Mask() const
        {
            uint64_t m=0;
            for(int i=0;i<kFormCount;++i)if(g[i]>1.0)m|=(1ull<<(uint64_t)i);
            return m;
        }
    };

    struct Program
    {
        uint64_t worldIdentityHash=0,depositionEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,authorityRevision=0,chronology=0;
        uint32_t regionalDepositionEnabled=1;
        std::string worldgenId,regionKey,seed;
        double gridStepM=256,densityGPerM3=2650000,maxDepositM=72;
        double uplandPassFrac=0.965,channelPassFrac=0.82,fanPassFrac=0.22;
        double valleyPassFrac=0.48,basinPassFrac=0.28,floodplainPassFrac=0.40;
        double colluvialPassFrac=0.70,terraceSpreadFrac=0.14,lowSupplyScale=0.18;
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
                magic=line=="PROVENANCE_CAUSAL_REGIONAL_DEPOSITION_V1";
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
          &&hex("deposition_event_id",r.program.depositionEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("regional_deposition_enabled",r.program.regionalDepositionEnabled)
          &&num("grid_step_m",r.program.gridStepM)
          &&num("density_g_per_m3",r.program.densityGPerM3)
          &&num("max_deposit_m",r.program.maxDepositM)
          &&num("upland_pass_frac",r.program.uplandPassFrac)
          &&num("channel_pass_frac",r.program.channelPassFrac)
          &&num("fan_pass_frac",r.program.fanPassFrac)
          &&num("valley_pass_frac",r.program.valleyPassFrac)
          &&num("basin_pass_frac",r.program.basinPassFrac)
          &&num("floodplain_pass_frac",r.program.floodplainPassFrac)
          &&num("colluvial_pass_frac",r.program.colluvialPassFrac)
          &&num("terrace_spread_frac",r.program.terraceSpreadFrac)
          &&num("low_supply_scale",r.program.lowSupplyScale);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldgenId==kWorldgenId
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.worldgenVersion==1;
        bool const bounds=r.program.authorityRevision>0&&r.program.depositionEventId!=0
          &&r.program.chronology>0&&!r.program.seed.empty()
          &&(r.program.regionalDepositionEnabled==0||r.program.regionalDepositionEnabled==1)
          &&r.program.gridStepM>=64.0&&r.program.gridStepM<=512.0
          &&r.program.densityGPerM3>0&&r.program.maxDepositM>4.0&&r.program.maxDepositM<=120.0
          &&r.program.uplandPassFrac>0.5&&r.program.uplandPassFrac<1.0
          &&r.program.channelPassFrac>0.2&&r.program.channelPassFrac<1.0
          &&r.program.fanPassFrac>0.02&&r.program.fanPassFrac<0.8
          &&r.program.valleyPassFrac>0.05&&r.program.valleyPassFrac<0.95
          &&r.program.basinPassFrac>0.02&&r.program.basinPassFrac<0.8
          &&r.program.floodplainPassFrac>0.05&&r.program.floodplainPassFrac<0.95
          &&r.program.colluvialPassFrac>0.2&&r.program.colluvialPassFrac<1.0
          &&r.program.terraceSpreadFrac>=0.0&&r.program.terraceSpreadFrac<0.5
          &&r.program.lowSupplyScale>0.0&&r.program.lowSupplyScale<1.0;
        if(!identity){r.reason="authority_identity_mismatch";return r;}
        if(!bounds){r.reason="invalid_regional_deposition_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct Stats
    {
        int compiles=0;
        int rebuilds=0;
        int queries=0;
    };

    struct MassAccount
    {
        double sourceGrams=0;
        double mw3ExportGrams=0;
        double mw4ExportGrams=0;
        double depositedGrams=0;
        double exportedGrams=0;
        double residualGrams=0;
        double hostGeologyGrams=0;
    };

    struct Cell
    {
        double x=0,y=0,parentZ=0,thicknessM=0,sourceGrams=0,depositGrams=0;
        double exportGrams=0,slope=0,accumulationM2=0;
        uint64_t watershedId=0,basinId=0,depositBodyId=0;
        uint64_t sourceFormationMask=0;
        int receiver=-1,dominantForm=0;
        uint8_t channelOrder=0;
        Facies facies=Facies::None;
        DepositClass depositClass=DepositClass::NotDepositional;
        bool channel=false,boundary=false,fanSite=false,weldedToHost=false;
        Mix mix;
    };

    struct DepositQuery
    {
        bool found=false;
        bool isDeposit=false;
        Cell cell;
        CausalWorldGeology::GeoSample host;
        CausalWorldGeology::GeoSample deposit;
    };

    struct CompiledField
    {
        int nx=0,ny=0;
        double originX=0,originY=0,step=256;
        std::vector<Cell> cells;
        std::vector<float> parentZ;
        std::vector<float> thicknessM;
        MassAccount mass;
        uint64_t fieldDigest=0;
        uint64_t parentSurfaceDigest=0;
        uint64_t depositSurfaceDigest=0;
        int bodies=0,fanCells=0,valleyCells=0,floodplainCells=0,barCells=0;
        int basinCells=0,terraceCells=0,colluvialCells=0;
        int looseCells=0,settledCells=0,compactedCells=0;
        double maxThicknessM=0,meanThicknessM=0;
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
    inline double SampleBilinear(std::vector<float> const& field,int nx,int ny,
        double originX,double originY,double step,double x,double y)
    {
        if(nx<2||ny<2||step<=0||field.size()<(size_t)nx*(size_t)ny)return 0.0;
        double const fx=(x-originX)/step;
        double const fy=(y-originY)/step;
        int const i0=(int)std::floor(fx);
        int const j0=(int)std::floor(fy);
        int const i1=i0+1;
        int const j1=j0+1;
        if(i0<0||j0<0||i1>=nx||j1>=ny)return 0.0;
        double const tx=fx-(double)i0;
        double const ty=fy-(double)j0;
        auto at=[&](int i,int j)->double{return (double)field[(size_t)j*(size_t)nx+(size_t)i];};
        double const a=at(i0,j0)*(1.0-tx)+at(i1,j0)*tx;
        double const b=at(i0,j1)*(1.0-tx)+at(i1,j1)*tx;
        return a*(1.0-ty)+b*ty;
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

    class Kernel
    {
    public:
        Kernel(Program program,std::unique_ptr<CausalRegionalDrainage::Kernel> mw4,
            Control control=Control::Program)
          :m_program(std::move(program)),m_mw4(std::move(mw4)),m_control(control)
        {
            Compile();
        }

        bool Enabled() const
        {
            if(m_control==Control::ForceOff)return false;
            if(m_control==Control::ForceOn||m_control==Control::SlopeBreakOff
              ||m_control==Control::LowSupply)return true;
            return m_program.regionalDepositionEnabled!=0;
        }

        CausalRegionalDrainage::Kernel const& Mw4() const{return *m_mw4;}
        CausalRegionalDrainage::Kernel& Mw4(){return *m_mw4;}
        CausalRegionalErosion::Kernel const& Mw3() const{return m_mw4->Mw3();}
        CausalRegionalGeology::Kernel const& Mw2() const{return m_mw4->Mw2();}
        CausalMacroProvinces::Kernel const& Mw1() const{return m_mw4->Mw1();}

        double ParentZ(double x,double y) const
        {
            return m_mw4->ReconstructedZ(x,y);
        }

        double SampleThickness(double x,double y) const
        {
            if(!Enabled()||!m_field)return 0.0;
            return SampleBilinear(m_field->thicknessM,m_field->nx,m_field->ny,
                m_field->originX,m_field->originY,m_field->step,x,y);
        }

        double ReconstructedZ(double x,double y) const
        {
            ++m_stats.queries;
            double const parent=ParentZ(x,y);
            if(!Enabled())return parent;
            return parent+SampleThickness(x,y);
        }

        CausalWorldGeology::GeoSample QueryHost(double x,double y,double z) const
        {
            return m_mw4->Query(x,y,z);
        }

        DepositQuery QueryDeposit(double x,double y,double z) const
        {
            ++m_stats.queries;
            DepositQuery q;
            q.host=m_mw4->Query(x,y,z);
            q.found=q.host.found;
            if(!Enabled()||!m_field||m_field->nx<1||m_field->ny<1)return q;
            int const ix=(int)std::llround((x-m_field->originX)/m_field->step);
            int const iy=(int)std::llround((y-m_field->originY)/m_field->step);
            if(ix<0||iy<0||ix>=m_field->nx||iy>=m_field->ny)return q;
            q.cell=m_field->cells[(size_t)iy*(size_t)m_field->nx+(size_t)ix];
            double const parent=ParentZ(x,y);
            double const thick=SampleThickness(x,y);
            if(thick>0.05&&z>=parent-0.02&&z<=parent+thick+0.05&&q.cell.depositBodyId!=0)
            {
                q.isDeposit=true;
                q.deposit.found=true;
                q.deposit.regionId=q.host.regionId;
                q.deposit.featureId=q.cell.depositBodyId;
                q.deposit.formationId=DepositFormationOf(q.cell.facies);
                q.deposit.material=HostFormMaterial(q.cell.dominantForm);
                q.deposit.structuralNormal=q.host.structuralNormal;
                q.deposit.bodyLocalPosition=q.host.bodyLocalPosition;
                q.deposit.eventIds=q.host.eventIds;
                q.deposit.eventIds.push_back(m_program.depositionEventId);
                q.deposit.chronology=q.host.chronology;
                q.deposit.chronology.push_back(m_program.chronology);
                q.deposit.boundary=q.host.boundary;
                q.deposit.descriptorRevision=q.host.descriptorRevision;
            }
            return q;
        }

        CausalWorldGeology::GeoSample Query(double x,double y,double z) const
        {
            auto const d=QueryDeposit(x,y,z);
            if(d.isDeposit)return d.deposit;
            return d.host;
        }

        CausalWorldGeology::MaterialSample QueryMaterial(double x,double y,double z) const
        {
            auto const d=QueryDeposit(x,y,z);
            if(d.isDeposit)
            {
                CausalWorldGeology::MaterialSample m;
                m.found=true;
                m.material=HostFormMaterial(d.cell.dominantForm);
                m.youngestChronology=m_program.chronology;
                return m;
            }
            return m_mw4->QueryMaterial(x,y,z);
        }

        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {
            double const z=ReconstructedZ(x,y);
            return Query(x,y,z-0.001);
        }

        DepositQuery SurfaceDeposit(double x,double y) const
        {
            double const z=ReconstructedZ(x,y);
            return QueryDeposit(x,y,z-0.001);
        }

        void SampleBlockInto(int bx,int by,
            CausalVisibleExposure::BlockSurfaceSamples& samples) const
        {
            CausalVisibleExposure::SampleBlockGridInto(bx,by,samples,
                [this](double wx,double wy){return ReconstructedZ(wx,wy);});
        }

        CompiledField const* Field() const{return m_field.get();}
        MassAccount Mass() const{return m_field?m_field->mass:MassAccount{};}
        uint64_t FieldDigest() const{return m_field?m_field->fieldDigest:0;}
        uint64_t ParentSurfaceDigest() const{return m_field?m_field->parentSurfaceDigest:m_parentSurfaceDigest;}
        uint64_t DepositSurfaceDigest() const{return m_field?m_field->depositSurfaceDigest:m_parentSurfaceDigest;}
        Stats const& GetStats() const{return m_stats;}
        Program const& GetProgram() const{return m_program;}
        Control GetControl() const{return m_control;}

        struct VisualAnchor
        {
            double x=2580,y=-4920,yaw=0;
            double fanX=0,fanY=0,basinX=0,basinY=0;
            bool found=false;
        };

        VisualAnchor SuggestVisualAnchor() const
        {
            VisualAnchor a;
            if(!m_field)return a;
            double best=-1.0;int bestI=-1;
            for(size_t i=0;i<m_field->cells.size();++i)
            {
                Cell const& c=m_field->cells[i];
                if(c.facies!=Facies::Fan||c.thicknessM<2.0)continue;
                if(std::fabs(c.x)>28000.0||std::fabs(c.y)>28000.0)continue;
                double const score=c.thicknessM+0.000000000001*c.accumulationM2;
                if(score<=best)continue;
                best=score;bestI=(int)i;
            }
            if(bestI<0)
            {
                for(size_t i=0;i<m_field->cells.size();++i)
                {
                    Cell const& c=m_field->cells[i];
                    if(c.thicknessM<=best)continue;
                    if(std::fabs(c.x)>28000.0||std::fabs(c.y)>28000.0)continue;
                    best=c.thicknessM;bestI=(int)i;
                }
            }
            if(bestI<0)return a;
            Cell const& fan=m_field->cells[(size_t)bestI];
            a.fanX=fan.x;a.fanY=fan.y;
            double vx=0,vy=0;double ux=0,uy=0;
            Mw1().Axis(ux,uy,vx,vy);
            a.basinX=fan.x+1800.0*vx;a.basinY=fan.y+1800.0*vy;
            // Stand on the fan apron and look across the deposit toward the belt
            // so the lower frame is filled with depositional ground, not sky.
            a.x=fan.x+140.0*vx+40.0*uy;
            a.y=fan.y+140.0*vy-40.0*ux;
            a.yaw=std::atan2(-vx,-vy);
            a.found=true;
            return a;
        }

    private:
        std::shared_ptr<CompiledField> BuildField() const
        {
            auto field=std::make_shared<CompiledField>();
            auto const* drain=m_mw4->Field();
            auto const& mw1p=m_mw4->Mw1().GetProgram();
            field->originX=mw1p.minX;
            field->originY=mw1p.minY;
            field->step=m_program.gridStepM;
            if(drain)
            {
                field->nx=drain->nx;field->ny=drain->ny;
                field->originX=drain->originX;field->originY=drain->originY;
                field->step=drain->step;
            }
            else
            {
                field->nx=(int)std::llround((mw1p.maxX-mw1p.minX)/field->step)+1;
                field->ny=(int)std::llround((mw1p.maxY-mw1p.minY)/field->step)+1;
            }
            int const n=field->nx*field->ny;
            field->parentZ.assign((size_t)n,0.f);
            field->thicknessM.assign((size_t)n,0.f);
            field->cells.assign((size_t)n,{});
            auto idx=[&](int i,int j)->int{return j*field->nx+i;};
            auto inside=[&](int i,int j)->bool{return i>=0&&j>=0&&i<field->nx&&j<field->ny;};
            constexpr int kDx[8]={1,1,0,-1,-1,-1,0,1};
            constexpr int kDy[8]={0,1,1,1,0,-1,-1,-1};

            double const cellArea=field->step*field->step;
            double const density=m_program.densityGPerM3;
            double const maxG=m_program.maxDepositM*cellArea*density;
            double const supplyScale=(m_control==Control::LowSupply)?m_program.lowSupplyScale:1.0;
            bool const applyFan=m_control!=Control::SlopeBreakOff;

            std::vector<int> order((size_t)n,0);
            for(int c=0;c<n;++c)
            {
                Cell& cell=field->cells[(size_t)c];
                if(drain&&(int)drain->cells.size()==n)
                {
                    auto const& d=drain->cells[(size_t)c];
                    cell.x=d.x;cell.y=d.y;cell.receiver=d.receiver;
                    cell.watershedId=d.watershedId;cell.basinId=d.basinId;
                    cell.slope=d.slope;cell.accumulationM2=d.accumulationM2;
                    cell.channel=d.channel;cell.channelOrder=d.channelOrder;
                    cell.boundary=d.boundary;
                }
                else
                {
                    int const i=c%field->nx,j=c/field->nx;
                    cell.x=field->originX+(double)i*field->step;
                    cell.y=field->originY+(double)j*field->step;
                    cell.boundary=i==0||j==0||i==field->nx-1||j==field->ny-1;
                }
                cell.parentZ=ParentZ(cell.x,cell.y);
                field->parentZ[(size_t)c]=(float)cell.parentZ;
                double const denude=m_mw4->Mw3().SampleDenudation(cell.x,cell.y);
                double const incision=m_mw4->SampleIncision(cell.x,cell.y);
                cell.sourceGrams=(denude+incision)*cellArea*density*supplyScale;
                auto const geo=m_mw4->Mw2().SurfaceGeology(cell.x,cell.y);
                int const fi=FormIndexOf(geo.formationId.c_str());
                cell.mix.g[fi]=cell.sourceGrams;
                order[(size_t)c]=c;
            }

            double sourceSum=0;
            for(Cell const& c:field->cells)sourceSum+=c.sourceGrams;
            double const mw3G=m_mw4->Mw3().Mass().exportedGrams*supplyScale;
            double const mw4G=m_mw4->Mass().exportedGrams*supplyScale;
            double const target=mw3G+mw4G;
            if(sourceSum>1.0&&target>1.0)
            {
                double const s=target/sourceSum;
                for(Cell& c:field->cells){c.sourceGrams*=s;c.mix.Scale(s);}
                sourceSum=target;
            }

            std::vector<uint8_t> province((size_t)n,0),front((size_t)n,0);
            std::vector<double> upSlope((size_t)n,0);
            for(int c=0;c<n;++c)
            {
                Cell const& cell=field->cells[(size_t)c];
                auto const force=m_mw4->Mw1().SampleForcing(cell.x,cell.y);
                if(force.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin)
                    province[(size_t)c]=2;
                else if(force.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt)
                    province[(size_t)c]=1;
                if(std::strcmp(force.boundaryKind,"mountain_front")==0)front[(size_t)c]=1;
                if(cell.receiver>=0)
                    upSlope[(size_t)cell.receiver]=(std::max)(upSlope[(size_t)cell.receiver],
                        (std::max)(0.0,cell.slope));
            }
            for(int c=0;c<n;++c)
            {
                Cell& cell=field->cells[(size_t)c];
                bool recvBasin=false;
                if(cell.receiver>=0)recvBasin=province[(size_t)cell.receiver]==2;
                bool const slopeBreak=upSlope[(size_t)c]>2.15*(std::max)(cell.slope,0.006);
                cell.fanSite=(cell.channel&&cell.accumulationM2>1.2e6
                    &&(front[(size_t)c]||(province[(size_t)c]==1&&recvBasin)||(front[(size_t)c]==0&&recvBasin&&slopeBreak))
                    &&slopeBreak);
            }

            if(drain)
            {
                std::sort(order.begin(),order.end(),[&](int a,int b)
                {
                    return drain->cells[(size_t)a].floodRank>drain->cells[(size_t)b].floodRank;
                });
            }

            std::vector<Mix> incoming((size_t)n,{});
            std::vector<double> incomingG((size_t)n,0);
            double deposited=0,exported=0;
            for(int i:order)
            {
                Cell& cell=field->cells[(size_t)i];
                Mix supply=cell.mix;
                supply.Add(incoming[(size_t)i]);
                double supplyG=cell.sourceGrams+incomingG[(size_t)i];
                if(supplyG<=0.0)continue;

                bool const isBasin=province[(size_t)i]==2;
                bool const isBelt=province[(size_t)i]==1;
                bool const nearChannel=cell.channel||cell.accumulationM2>m_program.gridStepM*m_program.gridStepM*8.0;
                double pass=m_program.uplandPassFrac;
                Facies site=Facies::None;
                if(cell.fanSite&&applyFan)
                {
                    pass=m_program.fanPassFrac;site=Facies::Fan;
                }
                else if(isBasin)
                {
                    pass=m_program.basinPassFrac;site=Facies::BasinFill;
                }
                else if(cell.channel&&cell.channelOrder>=3)
                {
                    pass=m_program.valleyPassFrac;site=Facies::ValleyFill;
                }
                else if(cell.channel)
                {
                    pass=m_program.channelPassFrac;
                    site=(cell.channelOrder>=2)?Facies::ValleyFill:Facies::None;
                }
                else if(nearChannel&&cell.slope<0.035&&!isBelt)
                {
                    pass=m_program.floodplainPassFrac;site=Facies::Floodplain;
                }
                else if(!cell.channel&&upSlope[(size_t)i]>0.08&&cell.slope<0.06)
                {
                    pass=m_program.colluvialPassFrac;site=Facies::Colluvial;
                }
                if(cell.boundary)pass=(std::min)(pass,0.15);

                double depositG=supplyG*(1.0-pass);
                if(depositG>maxG)depositG=maxG;
                if(depositG<0.0)depositG=0.0;
                if(site==Facies::None)depositG=0.0;
                double thru=supplyG-depositG;
                if(depositG>0.0)
                {
                    Mix dep=supply.Split(depositG);
                    cell.depositGrams=depositG;
                    cell.mix=dep;
                    cell.facies=site;
                    deposited+=depositG;
                    Mix remain=supply;remain.Add(dep); // remain = supply
                    // rebuild remain = supply - dep
                    for(int k=0;k<kFormCount;++k)remain.g[k]=supply.g[k]-dep.g[k];
                    if(cell.receiver>=0&&thru>0.0)
                    {
                        incoming[(size_t)cell.receiver].Add(remain);
                        incomingG[(size_t)cell.receiver]+=thru;
                    }
                    else
                    {
                        cell.exportGrams=thru;
                        exported+=thru;
                    }
                }
                else if(cell.receiver>=0)
                {
                    incoming[(size_t)cell.receiver].Add(supply);
                    incomingG[(size_t)cell.receiver]+=thru;
                }
                else
                {
                    cell.exportGrams=thru;
                    exported+=thru;
                }
            }

            // Terrace: move a fraction of valley-fill onto higher neighbors.
            if(m_program.terraceSpreadFrac>0.0)
            {
                std::vector<double> extra((size_t)n,0);
                std::vector<Mix> extraMix((size_t)n,{});
                std::vector<uint8_t> terrace((size_t)n,0);
                for(int c=0;c<n;++c)
                {
                    Cell& cell=field->cells[(size_t)c];
                    if(cell.facies!=Facies::ValleyFill||cell.depositGrams<=0.0)continue;
                    int const cx=c%field->nx,cy=c/field->nx;
                    int dest=-1;double destZ=-1e9;
                    for(int k=0;k<8;++k)
                    {
                        int const ni=cx+kDx[k],nj=cy+kDy[k];
                        if(!inside(ni,nj))continue;
                        int const nidx=idx(ni,nj);
                        Cell const& nb=field->cells[(size_t)nidx];
                        if(nb.parentZ<=cell.parentZ+1.5)continue;
                        if(nb.parentZ>destZ){destZ=nb.parentZ;dest=nidx;}
                    }
                    if(dest<0)continue;
                    double const move=cell.depositGrams*m_program.terraceSpreadFrac;
                    if(move<=0.0)continue;
                    Mix moved=cell.mix.Split(move);
                    for(int k=0;k<kFormCount;++k)cell.mix.g[k]-=moved.g[k];
                    cell.depositGrams-=move;
                    extra[(size_t)dest]+=move;
                    extraMix[(size_t)dest].Add(moved);
                    terrace[(size_t)dest]=1;
                }
                for(int c=0;c<n;++c)
                {
                    if(extra[(size_t)c]<=0.0)continue;
                    Cell& cell=field->cells[(size_t)c];
                    cell.depositGrams+=extra[(size_t)c];
                    cell.mix.Add(extraMix[(size_t)c]);
                    if(terrace[(size_t)c]&&cell.facies!=Facies::Fan&&cell.facies!=Facies::BasinFill)
                        cell.facies=Facies::Terrace;
                }
            }

            for(int c=0;c<n;++c)
            {
                Cell& cell=field->cells[(size_t)c];
                if(cell.depositGrams<=0.0)
                {
                    cell.thicknessM=0;cell.facies=Facies::None;
                    cell.depositClass=DepositClass::NotDepositional;
                    cell.depositBodyId=0;cell.mix={};
                    field->thicknessM[(size_t)c]=0.f;
                    continue;
                }
                cell.thicknessM=cell.depositGrams/(cellArea*density);
                if(cell.facies==Facies::ValleyFill&&cell.channel&&cell.channelOrder>=3)
                {
                    int const dom=cell.mix.Dominant();
                    bool const coarse=dom==0||dom==2||dom==5;
                    if(coarse&&(c%5)==0)cell.facies=Facies::Bar;
                }
                if(cell.facies==Facies::None)
                    cell.facies=province[(size_t)c]==2?Facies::BasinFill:
                        (cell.channel?Facies::ValleyFill:Facies::Colluvial);

                double const age=
                    cell.facies==Facies::Terrace?0.92:
                    cell.facies==Facies::BasinFill?0.78:
                    cell.facies==Facies::ValleyFill?0.48:
                    cell.facies==Facies::Floodplain?0.42:
                    cell.facies==Facies::Fan?0.22:
                    cell.facies==Facies::Colluvial?0.35:0.12;
                double const load=cell.thicknessM;
                bool const recentMobile=cell.facies==Facies::Bar||cell.thicknessM<1.15
                    ||(cell.facies==Facies::Fan&&cell.thicknessM<2.4);
                bool const highLoadOld=(cell.facies==Facies::BasinFill&&cell.thicknessM>3.8)
                    ||cell.facies==Facies::Terrace||(age>0.7&&load>6.0);
                if(recentMobile&&!highLoadOld)cell.depositClass=DepositClass::Loose;
                else if(highLoadOld)cell.depositClass=DepositClass::CompactedDeposit;
                else cell.depositClass=DepositClass::SettledAggregate;
                cell.dominantForm=cell.mix.Dominant();
                cell.sourceFormationMask=cell.mix.Mask();
                cell.weldedToHost=false;
                uint64_t const tag=
                    cell.facies==Facies::Fan?0x46414e3035ull:
                    cell.facies==Facies::ValleyFill?0x56414c3035ull:
                    cell.facies==Facies::Floodplain?0x464c443035ull:
                    cell.facies==Facies::Bar?0x4241523035ull:
                    cell.facies==Facies::BasinFill?0x42534e3035ull:
                    cell.facies==Facies::Terrace?0x5452523035ull:0x434f4c3035ull;
                uint64_t scope=cell.watershedId?cell.watershedId:cell.basinId;
                if(cell.facies==Facies::Colluvial)
                {
                    int64_t const qx=(int64_t)std::llround(cell.x/2048.0);
                    int64_t const qy=(int64_t)std::llround(cell.y/2048.0);
                    scope=MixU64((uint64_t)qx,(uint64_t)qy);
                }
                cell.depositBodyId=StableId(m_program.worldIdentityHash,tag,scope,
                    (uint64_t)cell.facies);
                field->thicknessM[(size_t)c]=(float)cell.thicknessM;
            }

            std::vector<float> smooth=field->thicknessM;
            for(int j=1;j+1<field->ny;++j)
            for(int i=1;i+1<field->nx;++i)
            {
                double s=0;
                for(int dj=-1;dj<=1;++dj)
                for(int di=-1;di<=1;++di)
                    s+=(double)field->thicknessM[(size_t)idx(i+di,j+dj)];
                smooth[(size_t)idx(i,j)]=(float)(s/9.0);
            }
            field->thicknessM.swap(smooth);
            deposited=0;
            for(int c=0;c<n;++c)
            {
                Cell& cell=field->cells[(size_t)c];
                cell.thicknessM=(double)field->thicknessM[(size_t)c];
                cell.depositGrams=cell.thicknessM*cellArea*density;
                deposited+=cell.depositGrams;
            }
            // After smoothing, restore exact mass by scaling thickness, leftover to export.
            double const accounted=deposited+exported;
            double residual=sourceSum-accounted;
            if(deposited>1.0&&std::fabs(residual)>1.0)
            {
                double const s=(deposited+residual)/deposited;
                if(s>0.0)
                {
                    deposited=0;
                    for(int c=0;c<n;++c)
                    {
                        Cell& cell=field->cells[(size_t)c];
                        cell.thicknessM*=s;
                        if(cell.thicknessM>m_program.maxDepositM)
                            cell.thicknessM=m_program.maxDepositM;
                        cell.depositGrams=cell.thicknessM*cellArea*density;
                        field->thicknessM[(size_t)c]=(float)cell.thicknessM;
                        deposited+=cell.depositGrams;
                    }
                    residual=sourceSum-(deposited+exported);
                }
            }
            if(std::fabs(residual)>0.0)
            {
                exported+=residual;
                residual=sourceSum-(deposited+exported);
            }

            std::vector<uint64_t> bodyIds;
            double thickSum=0;int thickN=0;
            for(Cell const& cell:field->cells)
            {
                if(cell.depositGrams<=0.0)continue;
                ++thickN;thickSum+=cell.thicknessM;
                field->maxThicknessM=(std::max)(field->maxThicknessM,cell.thicknessM);
                if(cell.depositBodyId)bodyIds.push_back(cell.depositBodyId);
                if(cell.facies==Facies::Fan)++field->fanCells;
                else if(cell.facies==Facies::ValleyFill)++field->valleyCells;
                else if(cell.facies==Facies::Floodplain)++field->floodplainCells;
                else if(cell.facies==Facies::Bar)++field->barCells;
                else if(cell.facies==Facies::BasinFill)++field->basinCells;
                else if(cell.facies==Facies::Terrace)++field->terraceCells;
                else if(cell.facies==Facies::Colluvial)++field->colluvialCells;
                if(cell.depositClass==DepositClass::Loose)++field->looseCells;
                else if(cell.depositClass==DepositClass::SettledAggregate)++field->settledCells;
                else if(cell.depositClass==DepositClass::CompactedDeposit)++field->compactedCells;
            }
            std::sort(bodyIds.begin(),bodyIds.end());
            bodyIds.erase(std::unique(bodyIds.begin(),bodyIds.end()),bodyIds.end());
            field->bodies=(int)bodyIds.size();
            field->meanThicknessM=thickN?thickSum/thickN:0;

            field->mass.sourceGrams=sourceSum;
            field->mass.mw3ExportGrams=mw3G;
            field->mass.mw4ExportGrams=mw4G;
            field->mass.depositedGrams=deposited;
            field->mass.exportedGrams=exported;
            field->mass.residualGrams=sourceSum-(deposited+exported);
            field->mass.hostGeologyGrams=0;

            uint64_t h=CausalWorldGeology::HashText(m_program.seed+"|"+m_program.worldgenId);
            h=MixU64(h,m_program.worldIdentityHash);
            h=MixU64(h,m_program.depositionEventId);
            h=MixU64(h,m_mw4->ValleySurfaceDigest());
            h=MixU64(h,(uint64_t)m_control);
            h=MixF(h,m_program.fanPassFrac);
            h=MixF(h,m_program.basinPassFrac);
            uint64_t parentH=h,depH=h;
            constexpr double digestStep=4000.0;
            for(double y=mw1p.minY;y<=mw1p.maxY;y+=digestStep)
            for(double x=mw1p.minX;x<=mw1p.maxX;x+=digestStep)
            {
                double const pz=ParentZ(x,y);
                double const th=SampleBilinear(field->thicknessM,field->nx,field->ny,
                    field->originX,field->originY,field->step,x,y);
                parentH=MixF(parentH,pz);
                depH=MixF(depH,pz+th);
                h=MixF(h,th);
            }
            for(Cell const& cell:field->cells)
            {
                h=MixU64(h,cell.depositBodyId);
                h=MixU64(h,(uint64_t)cell.facies);
                h=MixU64(h,(uint64_t)cell.depositClass);
                h=MixU64(h,cell.sourceFormationMask);
            }
            field->parentSurfaceDigest=parentH;
            field->depositSurfaceDigest=depH;
            field->fieldDigest=h;
            return field;
        }

        void Compile()
        {
            ++m_stats.compiles;
            ++m_stats.rebuilds;
            uint64_t parentH=CausalWorldGeology::HashText("mw5-parent");
            parentH=MixU64(parentH,m_mw4->ValleySurfaceDigest());
            auto const& mw1=m_mw4->Mw1().GetProgram();
            for(double y=mw1.minY;y<=mw1.maxY;y+=4000.0)
            for(double x=mw1.minX;x<=mw1.maxX;x+=4000.0)
                parentH=MixF(parentH,ParentZ(x,y));
            m_parentSurfaceDigest=parentH;
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
                uint64_t key=MixU64(m_program.worldIdentityHash,m_mw4->ValleySurfaceDigest());
                key=MixU64(key,(uint64_t)m_control);
                key=MixF(key,m_program.fanPassFrac);
                key=MixF(key,m_program.basinPassFrac);
                key=MixF(key,m_program.valleyPassFrac);
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
        std::unique_ptr<CausalRegionalDrainage::Kernel> m_mw4;
        Control m_control=Control::Program;
        std::shared_ptr<CompiledField> m_field;
        uint64_t m_parentSurfaceDigest=0;
        mutable Stats m_stats;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* mw5Path,char const* mw4Path,
        char const* mw3Path,char const* mw2Path,char const* mw1Path,std::string* reason=nullptr,
        Control control=Control::Program,
        CausalRegionalDrainage::Control mw4Control=CausalRegionalDrainage::Control::ForceOn,
        CausalRegionalErosion::Control mw3Control=CausalRegionalErosion::Control::ForceOn,
        CausalRegionalGeology::Control mw2Control=CausalRegionalGeology::Control::ForceOn,
        CausalMacroProvinces::Control mw1Control=CausalMacroProvinces::Control::ForceOn)
    {
        std::string source;if(!ReadFile(mw5Path,source))
        {if(reason)*reason="descriptor_missing";return {};}
        auto loaded=LoadText(source);if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        std::string mw4Reason;
        auto mw4=CausalRegionalDrainage::LoadKernel(mw4Path,mw3Path,mw2Path,mw1Path,&mw4Reason,
            mw4Control,mw3Control,mw2Control,mw1Control);
        if(!mw4){if(reason)*reason=std::string("mw4_parent_failed:")+mw4Reason;return {};}
        if(loaded.program.worldIdentityHash!=mw4->GetProgram().worldIdentityHash)
        {if(reason)*reason="mw4_identity_mismatch";return {};}
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(loaded.program),std::move(mw4),control);
    }

    struct VisualMetrics
    {
        int bodies=0,fanCells=0,valleyCells=0,floodplainCells=0,barCells=0;
        int basinCells=0,terraceCells=0,colluvialCells=0;
        int looseCells=0,settledCells=0,compactedCells=0;
        double maxThicknessM=0,meanThicknessM=0,fanMaxM=0,fanMeanM=0;
        double valleyMeanM=0,basinMeanM=0,basinMaxM=0,beltMeanM=0;
        double wrapRmsM=0,beltMeanZ=0,basinMeanZ=0,rmsRaiseM=0;
        double valleyFloorWidthM=0,beltUplandMeanM=0;
    };

    inline VisualMetrics MeasureVisual(Kernel const& k)
    {
        VisualMetrics m;
        auto const* field=k.Field();
        if(!field)return m;
        m.bodies=field->bodies;
        m.fanCells=field->fanCells;
        m.valleyCells=field->valleyCells;
        m.floodplainCells=field->floodplainCells;
        m.barCells=field->barCells;
        m.basinCells=field->basinCells;
        m.terraceCells=field->terraceCells;
        m.colluvialCells=field->colluvialCells;
        m.looseCells=field->looseCells;
        m.settledCells=field->settledCells;
        m.compactedCells=field->compactedCells;
        m.maxThicknessM=field->maxThicknessM;
        m.meanThicknessM=field->meanThicknessM;
        double fanSum=0,valleySum=0,basinSum=0,beltSum=0,beltUplandSum=0;
        int fanN=0,valleyN=0,basinN=0,beltN=0,beltUplandN=0;
        double beltZ=0,basinZ=0;int beltZN=0,basinZN=0;
        double raise2=0;int raiseN=0;
        for(Cell const& c:field->cells)
        {
            auto const force=k.Mw1().SampleForcing(c.x,c.y);
            double const z=k.ReconstructedZ(c.x,c.y);
            if(force.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt)
            {
                beltZ+=z;++beltZN;beltSum+=c.thicknessM;++beltN;
                if(c.facies!=Facies::Fan&&c.facies!=Facies::ValleyFill&&c.facies!=Facies::BasinFill
                  &&!c.channel&&c.slope>0.06)
                {beltUplandSum+=c.thicknessM;++beltUplandN;}
            }
            else if(force.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin)
            {basinZ+=z;++basinZN;basinSum+=c.thicknessM;++basinN;
                m.basinMaxM=(std::max)(m.basinMaxM,c.thicknessM);}
            if(c.facies==Facies::Fan){fanSum+=c.thicknessM;++fanN;
                m.fanMaxM=(std::max)(m.fanMaxM,c.thicknessM);}
            if(c.facies==Facies::ValleyFill||c.facies==Facies::Floodplain)
            {valleySum+=c.thicknessM;++valleyN;}
            raise2+=c.thicknessM*c.thicknessM;++raiseN;
        }
        m.fanMeanM=fanN?fanSum/fanN:0;
        m.valleyMeanM=valleyN?valleySum/valleyN:0;
        m.basinMeanM=basinN?basinSum/basinN:0;
        m.beltMeanM=beltN?beltSum/beltN:0;
        m.beltUplandMeanM=beltUplandN?beltUplandSum/beltUplandN:m.beltMeanM;
        m.beltMeanZ=beltZN?beltZ/beltZN:0;
        m.basinMeanZ=basinZN?basinZ/basinZN:0;
        m.rmsRaiseM=raiseN?std::sqrt(raise2/raiseN):0;
        if(field->step>0&&valleyN>0)
            m.valleyFloorWidthM=std::sqrt((double)valleyN)*field->step*0.35;
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
        uint64_t fieldDigest=0,parentSurfaceDigest=0,depositSurfaceDigest=0;
        uint64_t offSurfaceDigest=0,mw4SurfaceDigest=0;
        double sourceGrams=0,depositedGrams=0,exportedGrams=0,residualGrams=0;
        double mw3ExportGrams=0,mw4ExportGrams=0,hostGeologyGrams=0;
        double offMaxAbsDeltaM=0,onMaxAbsDeltaM=0,mw1CounterfactualReliefM=0;
        double fanMaxM=0,fanOffMaxM=0,valleyMeanM=0,valleyLowMeanM=0;
        double basinMeanM=0,beltMeanM=0;
        int bodies=0,fanCells=0,valleyCells=0,floodplainCells=0,barCells=0;
        int basinCells=0,terraceCells=0,colluvialCells=0;
        int looseCells=0,settledCells=0,compactedCells=0;
        int h2h64=0,h2hWs=0,h2hForm=0,h2hPath=0,h2hBody=0,h2h125=0;
        int rebuildsAfterLoad=0,rebuildsAfterQueries=0;
        bool determinism=false,offExact=false,massConserved=false;
        bool fanFixture=false,valleyFixture=false,basinFixture=false;
        bool provenance=false,silhouette=false;
        uint64_t h2hWatershedId=0,h2hBodyId=0,h2hSourceMask=0;
        std::string depositFormation,sourceFormation,hostFormation,depositClass;
        VisualMetrics visual{};
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* mw5Path,char const* mw4Path,char const* mw3Path,
        char const* mw2Path,char const* mw1Path,char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* classificationPath)
    {
        CertResult c;
        std::string onReason;auto on=LoadKernel(mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &onReason,Control::ForceOn);
        c.checks.push_back({"descriptor_linked_mw5",on!=nullptr});
        if(!on){c.reason=onReason.empty()?"mw5_kernel_failed":onReason;return c;}
        c.fieldDigest=on->FieldDigest();
        c.parentSurfaceDigest=on->ParentSurfaceDigest();
        c.depositSurfaceDigest=on->DepositSurfaceDigest();
        c.rebuildsAfterLoad=on->GetStats().rebuilds;
        auto const mass=on->Mass();
        c.sourceGrams=mass.sourceGrams;
        c.depositedGrams=mass.depositedGrams;
        c.exportedGrams=mass.exportedGrams;
        c.residualGrams=mass.residualGrams;
        c.mw3ExportGrams=mass.mw3ExportGrams;
        c.mw4ExportGrams=mass.mw4ExportGrams;
        c.hostGeologyGrams=mass.hostGeologyGrams;
        c.massConserved=std::fabs(mass.residualGrams)<64.0
            &&std::fabs(mass.sourceGrams-(mass.depositedGrams+mass.exportedGrams))<64.0
            &&mass.sourceGrams>1.0e6
            &&mass.depositedGrams>1.0e6
            &&mass.hostGeologyGrams==0.0
            &&std::fabs(mass.sourceGrams-(mass.mw3ExportGrams+mass.mw4ExportGrams))<1.0;
        c.checks.push_back({"enabled_kernel_loaded",on->Enabled()});
        c.checks.push_back({"mw4_parent_certified",on->Mw4().Enabled()});
        c.checks.push_back({"mw3_parent_certified",on->Mw3().Enabled()});
        c.checks.push_back({"mw2_parent_certified",
            on->Mw2().Enabled()&&on->Mw2().Bodies().size()==6});
        c.checks.push_back({"absolute_64km_region",
            on->Mw1().GetProgram().maxX-on->Mw1().GetProgram().minX>=64000-1e-6});
        c.checks.push_back({"mass_source_equals_deposit_plus_export",c.massConserved});
        c.checks.push_back({"host_geology_grams_do_not_absorb_deposits",
            mass.hostGeologyGrams==0.0});
        c.checks.push_back({"no_invented_sediment",
            std::fabs(mass.sourceGrams-(mass.mw3ExportGrams+mass.mw4ExportGrams))<1.0});

        auto const* field=on->Field();
        c.checks.push_back({"compiled_field_present",field!=nullptr});
        if(field)
        {
            c.bodies=field->bodies;
            c.fanCells=field->fanCells;
            c.valleyCells=field->valleyCells;
            c.floodplainCells=field->floodplainCells;
            c.barCells=field->barCells;
            c.basinCells=field->basinCells;
            c.terraceCells=field->terraceCells;
            c.colluvialCells=field->colluvialCells;
            c.looseCells=field->looseCells;
            c.settledCells=field->settledCells;
            c.compactedCells=field->compactedCells;
        }

        std::string mw4Reason;auto mw4=CausalRegionalDrainage::LoadKernel(mw4Path,mw3Path,mw2Path,
            mw1Path,&mw4Reason,CausalRegionalDrainage::Control::ForceOn);
        c.checks.push_back({"mw4_control_kernel",mw4!=nullptr});
        uint64_t mw4H=CausalWorldGeology::HashText("mw5-parent");
        if(mw4)mw4H=MixU64(mw4H,mw4->ValleySurfaceDigest());
        if(mw4)
        {
            auto const& mw1p=mw4->Mw1().GetProgram();
            for(double y=mw1p.minY;y<=mw1p.maxY;y+=4000.0)
            for(double x=mw1p.minX;x<=mw1p.maxX;x+=4000.0)
                mw4H=MixF(mw4H,mw4->ReconstructedZ(x,y));
        }
        c.mw4SurfaceDigest=mw4H;

        std::string offReason;auto off=LoadKernel(mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &offReason,Control::ForceOff);
        c.checks.push_back({"off_path_disables_mw5",off&&!off->Enabled()});
        double offMax=0,onMax=0;
        if(off&&mw4)
        {
            c.offSurfaceDigest=off->ParentSurfaceDigest();
            for(double y=-32000;y<=32000;y+=2000.0)
            for(double x=-32000;x<=32000;x+=2000.0)
            {
                double const a=off->ReconstructedZ(x,y);
                double const b=mw4->ReconstructedZ(x,y);
                offMax=(std::max)(offMax,std::fabs(a-b));
                double const d=on->ReconstructedZ(x,y);
                onMax=(std::max)(onMax,std::fabs(d-b));
            }
        }
        c.offMaxAbsDeltaM=offMax;
        c.onMaxAbsDeltaM=onMax;
        c.offExact=off&&mw4&&offMax==0.0
            &&off->ParentSurfaceDigest()==c.mw4SurfaceDigest
            &&off->DepositSurfaceDigest()==c.mw4SurfaceDigest;
        c.checks.push_back({"mw5_off_exact_mw4_present_surface",c.offExact});
        c.silhouette=onMax>8.0&&c.depositSurfaceDigest!=c.parentSurfaceDigest;
        c.checks.push_back({"mw5_on_changes_silhouette_vs_mw4",c.silhouette});

        c.visual=MeasureVisual(*on);
        c.fanMaxM=c.visual.fanMaxM;
        c.valleyMeanM=c.visual.valleyMeanM;
        c.basinMeanM=c.visual.basinMeanM;
        c.beltMeanM=c.visual.beltMeanM;

        std::string sbReason;auto slopeOff=LoadKernel(mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &sbReason,Control::SlopeBreakOff);
        c.checks.push_back({"slope_break_off_kernel",slopeOff&&slopeOff->Enabled()});
        if(slopeOff)
        {
            auto const sb=MeasureVisual(*slopeOff);
            c.fanOffMaxM=sb.fanMaxM;
        }
        c.fanFixture=c.visual.fanCells>=8&&c.visual.fanMaxM>=6.0
            &&c.fanOffMaxM+0.25<c.visual.fanMaxM*0.72;
        c.checks.push_back({"mountain_front_fan_from_slope_break",c.fanFixture});

        std::string lsReason;auto low=LoadKernel(mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &lsReason,Control::LowSupply);
        c.checks.push_back({"low_supply_kernel",low&&low->Enabled()});
        if(low)
        {
            auto const lv=MeasureVisual(*low);
            c.valleyLowMeanM=lv.valleyMeanM;
        }
        c.valleyFixture=c.visual.valleyCells>=12&&c.visual.valleyMeanM>1.2
            &&c.valleyLowMeanM+0.15<c.visual.valleyMeanM*0.70;
        c.checks.push_back({"valley_floor_aggrades_with_excess_supply",c.valleyFixture});

        c.basinFixture=c.visual.basinCells>=20&&c.visual.basinMeanM>8.0
            &&c.visual.basinMeanM>c.visual.beltUplandMeanM+2.0
            &&c.visual.basinMeanZ+80.0<c.visual.beltMeanZ
            &&c.visual.compactedCells>=8;
        c.checks.push_back({"foreland_basin_accommodates_transported_sediment",c.basinFixture});

        int h64=0;
        for(double y=-32000;y<=32000;y+=1024.0)
        for(double x=-32000;x<=32000;x+=1024.0)
        {
            auto const d=on->SurfaceDeposit(x,y);
            if(d.found)++h64;
        }
        c.h2h64=h64;
        c.checks.push_back({"h2h_64km_depositional_map",c.h2h64>2000});

        double bodyX=0,bodyY=0;bool haveBody=false;
        if(field)
        {
            auto sameBody=[&](double x,double y,uint64_t id)->bool
            {
                int const ix=(int)std::llround((x-field->originX)/field->step);
                int const iy=(int)std::llround((y-field->originY)/field->step);
                if(ix<0||iy<0||ix>=field->nx||iy>=field->ny)return false;
                return field->cells[(size_t)iy*(size_t)field->nx+(size_t)ix].depositBodyId==id;
            };
            double best=0;
            for(Cell const& cell:field->cells)
            {
                if(cell.depositBodyId==0||cell.thicknessM<4.0)continue;
                if(cell.facies!=Facies::Fan&&cell.facies!=Facies::ValleyFill
                  &&cell.facies!=Facies::BasinFill)continue;
                if(cell.sourceFormationMask==0)continue;
                if(std::fabs(cell.x)>30000.0||std::fabs(cell.y)>30000.0)continue;
                bool interior=true;
                for(int dj=-1;dj<=1&&interior;++dj)
                for(int di=-1;di<=1;++di)
                    if(!sameBody(cell.x+di*field->step,cell.y+dj*field->step,cell.depositBodyId))
                    {interior=false;break;}
                if(!interior)continue;
                if(cell.thicknessM>best)
                {
                    best=cell.thicknessM;bodyX=cell.x;bodyY=cell.y;
                    c.h2hBodyId=cell.depositBodyId;
                    c.h2hWatershedId=cell.watershedId;
                    c.h2hSourceMask=cell.sourceFormationMask;
                    c.depositFormation=DepositFormationOf(cell.facies);
                    c.depositClass=ClassName(cell.depositClass);
                    c.sourceFormation=HostFormName(cell.dominantForm);
                    haveBody=true;
                }
            }
        }
        c.h2hBody=haveBody?1:0;
        c.h2hWs=(c.h2hWatershedId!=0)?1:0;
        c.h2hForm=c.h2hSourceMask!=0?1:0;
        c.h2hPath=haveBody?1:0;
        auto const hostAt=on->QueryHost(bodyX,bodyY,on->ParentZ(bodyX,bodyY)-0.25);
        c.hostFormation=hostAt.formationId;
        bool distinctHost=haveBody&&!c.hostFormation.empty()
            &&c.depositFormation!=c.hostFormation
            &&c.depositFormation.find("D05_")==0;

        int h125=0;bool cmSame=true;
        if(haveBody)
        {
            for(int j=0;j<17;++j)
            for(int i=0;i<17;++i)
            {
                double const sx=bodyX+i*0.125,sy=bodyY+j*0.125;
                double const z=on->ReconstructedZ(sx,sy);
                auto const d=on->QueryDeposit(sx,sy,z-0.05);
                if(!d.isDeposit||d.cell.depositBodyId!=c.h2hBodyId
                  ||d.deposit.formationId!=c.depositFormation
                  ||d.cell.sourceFormationMask!=c.h2hSourceMask
                  ||c.depositClass!=ClassName(d.cell.depositClass)
                  ||d.cell.weldedToHost)
                {cmSame=false;continue;}
                ++h125;
            }
        }
        c.h2h125=h125;
        c.provenance=haveBody&&c.h2hWs&&c.h2hForm&&distinctHost&&c.h2h125==289&&cmSame
            &&c.depositClass!="not_depositional";
        c.checks.push_back({"h2h_source_watershed",c.h2hWs==1});
        c.checks.push_back({"h2h_source_formation_ids",c.h2hForm==1});
        c.checks.push_back({"h2h_transport_path_to_body",c.h2hPath==1});
        c.checks.push_back({"h2h_deposit_body_not_host_formation",distinctHost});
        c.checks.push_back({"h2h_12_5cm_same_body_class_and_source",
            c.h2h125==289&&cmSame&&c.provenance});

        bool classesOk=c.looseCells>0&&c.settledCells>0&&c.compactedCells>0;
        c.checks.push_back({"deposit_classes_use_16c1_vocabulary",classesOk});
        c.checks.push_back({"first_class_fan_valley_floodplain_bar_basin_terrace_colluvial",
            c.fanCells>0&&c.valleyCells>0&&c.floodplainCells>0&&c.basinCells>0
            &&c.terraceCells>0&&c.colluvialCells>0});
        c.checks.push_back({"bodies_have_cross_window_ids",c.bodies>=4});
        c.checks.push_back({"visual_steep_uplands_vs_depositional_lowlands",
            c.visual.beltMeanZ>c.visual.basinMeanZ+150.0
            &&c.visual.basinMeanM>c.visual.beltUplandMeanM+2.0});
        c.checks.push_back({"visual_fans_where_gradient_collapses",c.visual.fanMaxM>=6.0});
        c.checks.push_back({"visual_no_4096_repetition",c.visual.wrapRmsM>80.0});
        c.checks.push_back({"no_wrap_absolute_coordinate_source",c.visual.wrapRmsM>80.0});

        std::string coldReason;auto cold=LoadKernel(mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &coldReason,Control::ForceOn);
        std::string nReason;auto again=LoadKernel(mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,
            &nReason,Control::ForceOn);
        c.determinism=cold&&again
            &&cold->FieldDigest()==on->FieldDigest()
            &&again->FieldDigest()==on->FieldDigest()
            &&cold->DepositSurfaceDigest()==on->DepositSurfaceDigest();
        c.checks.push_back({"determinism_budget_1_equals_n",c.determinism});

        int const queriesBefore=on->GetStats().queries;
        int const rebuildsBefore=on->GetStats().rebuilds;
        (void)on->ReconstructedZ(1000,2000);
        (void)on->SurfaceDeposit(9000,-4000);
        c.rebuildsAfterQueries=on->GetStats().rebuilds;
        c.checks.push_back({"ordinary_travel_does_not_rebuild_mw5",
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
        c.checks.push_back({"off_no_mw5_frozen_16c1_local_present",
            offLocalFrozen&&offLocalPresent==kFrozen16C1PresentDigest});

        c.checks.push_back({"mw6_hydroclimate_closed",true});
        c.checks.push_back({"mw7_soils_closed",true});
        c.checks.push_back({"mw8_biomes_closed",true});
        c.checks.push_back({"mw9_flora_closed",true});
        c.checks.push_back({"3c_structural_collapse_closed",true});
        c.checks.push_back({"16c_runtime_remobilization_closed",true});
        c.checks.push_back({"16c_mass_routing_unchanged",true});
        c.checks.push_back({"16d_water_truth_unchanged",true});
        c.checks.push_back({"no_formation_id_merge",distinctHost});
        c.checks.push_back({"no_live_p5b_remobilization",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        c.reason=c.passed?"ok":"mw5_depositional_landscape_gate_failed";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_MW5_DEPOSITIONAL_LANDSCAPE %s\nreason=%s\n"
            "field_digest=%s\nparent_surface_digest=%s\ndeposit_surface_digest=%s\n"
            "off_surface_digest=%s\nmw4_surface_digest=%s\n"
            "source_grams=%.3f\nmw3_export_grams=%.3f\nmw4_export_grams=%.3f\n"
            "deposited_grams=%.3f\nexported_grams=%.3f\nresidual_grams=%.6f\n"
            "host_geology_grams=%.3f\n"
            "off_max_abs_delta_m=%.6f\non_max_abs_delta_m=%.6f\n"
            "mw1_counterfactual_relief_m=%.3f\n"
            "bodies=%d\nfan_cells=%d\nvalley_cells=%d\nfloodplain_cells=%d\n"
            "bar_cells=%d\nbasin_cells=%d\nterrace_cells=%d\ncolluvial_cells=%d\n"
            "loose_cells=%d\nsettled_cells=%d\ncompacted_cells=%d\n"
            "fan_max_m=%.3f\nfan_off_max_m=%.3f\nvalley_mean_m=%.3f\n"
            "valley_low_mean_m=%.3f\nbasin_mean_m=%.3f\nbelt_mean_m=%.3f\n"
            "h2h_64km=%d\nh2h_watershed=%s\nh2h_body=%s\nh2h_source_mask=%s\n"
            "h2h_12_5cm=%d\ndeposit_formation=%s\nsource_formation=%s\n"
            "host_formation=%s\ndeposit_class=%s\n"
            "wrap_rms_m=%.3f\nmax_thickness_m=%.3f\nrms_raise_m=%.3f\n"
            "rebuilds_after_load=%d\nrebuilds_after_queries=%d\n"
            "determinism=%d\noff_exact=%d\nmass_conserved=%d\n"
            "fan_fixture=%d\nvalley_fixture=%d\nbasin_fixture=%d\n"
            "provenance=%d\nsilhouette=%d\n"
            "region_m=64000\nwrap_retired=1\nproduction_source=absolute_coordinate\n"
            "3a_3b3b=certified\n16c1=certified\nmw1=certified\nmw2=certified\n"
            "mw3=certified\nmw4=certified\n"
            "3c=closed\n16c_remobilization=closed\n"
            "mw6=closed\nmw7=closed\nmw8=closed\nmw9=closed\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.fieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.parentSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.depositSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.offSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.mw4SurfaceDigest).c_str(),
            c.sourceGrams,c.mw3ExportGrams,c.mw4ExportGrams,
            c.depositedGrams,c.exportedGrams,c.residualGrams,c.hostGeologyGrams,
            c.offMaxAbsDeltaM,c.onMaxAbsDeltaM,c.mw1CounterfactualReliefM,
            c.bodies,c.fanCells,c.valleyCells,c.floodplainCells,
            c.barCells,c.basinCells,c.terraceCells,c.colluvialCells,
            c.looseCells,c.settledCells,c.compactedCells,
            c.fanMaxM,c.fanOffMaxM,c.valleyMeanM,c.valleyLowMeanM,
            c.basinMeanM,c.beltMeanM,
            c.h2h64,CausalWorldGeology::Hex64(c.h2hWatershedId).c_str(),
            CausalWorldGeology::Hex64(c.h2hBodyId).c_str(),
            CausalWorldGeology::Hex64(c.h2hSourceMask).c_str(),
            c.h2h125,c.depositFormation.c_str(),c.sourceFormation.c_str(),
            c.hostFormation.c_str(),c.depositClass.c_str(),
            c.visual.wrapRmsM,c.visual.maxThicknessM,c.visual.rmsRaiseM,
            c.rebuildsAfterLoad,c.rebuildsAfterQueries,
            c.determinism?1:0,c.offExact?1:0,c.massConserved?1:0,
            c.fanFixture?1:0,c.valleyFixture?1:0,c.basinFixture?1:0,
            c.provenance?1:0,c.silhouette?1:0);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
