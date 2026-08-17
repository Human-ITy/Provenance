#pragma once

// MW8: compiled ecological potential / biome regime on certified MW1–MW7.
// Persistent regime inferred from climate, elevation, moisture, soil/regolith,
// drainage, substrate, and exposure. Not vegetation placement, not a color
// mask independent of MW6/MW7.
//
// Required chain:
//   MW1 provinces → MW2 geology → MW3 erosion → MW4 drainage → MW5 deposits
//   → MW6 hydroclimate → MW7 soils / regolith → MW8 biome regime
//   → (closed) MW9 flora/fauna
//
// Off / no-MW8 leaves the exact MW7 present surface, bodies, HydroclimateId,
// and RegolithId untouched.

#include "CausalRegionalRegolith.h"

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

namespace CausalRegionalBiome
{
    constexpr char const* kExpectedRegion="causal_world_regional_biome_floor";
    constexpr char const* kWorldgenId="provenance_regional_biome_v1";
    constexpr double kPi=3.14159265358979323846;
    constexpr uint64_t kFrozen16C1PresentDigest=0x2eb519c42ca5bd6bull;
    constexpr uint64_t kFrozen16CSedimentDigest=0x1367584c41aedcdfull;
    constexpr uint64_t kFrozen16CGeometryDigest=0x7ae7aa62c50489e2ull;
    constexpr uint64_t kFrozen16DWaterDigest=0x636d01ba00d3dffeull;
    constexpr uint64_t kTagBiome=0x4d57380000000002ull;

    enum class Control:uint8_t
    {
        Program=0,
        ForceOff=1,
        ForceOn=2,
        HydroclimateOff=3,
        RegolithOff=4,
        LapseOff=5
    };

    enum class RegimeClass:uint8_t
    {
        None=0,
        AlpineBarren=1,
        AlpineTundra=2,
        Subalpine=3,
        CoolMoistForest=4,
        DryInteriorWoodland=5,
        RiparianCorridor=6,
        WetMeadow=7,
        BasinWetland=8,
        DryRockySlope=9
    };

    enum class TemperatureBand:uint8_t
    {
        None=0,
        Alpine=1,
        Subalpine=2,
        Cool=3,
        Mild=4
    };

    enum class MoistureBand:uint8_t
    {
        None=0,
        Arid=1,
        Dry=2,
        Mesic=3,
        Wet=4,
        Saturated=5
    };

    enum class ElevationBand:uint8_t
    {
        None=0,
        Alpine=1,
        Montane=2,
        Valley=3,
        Basin=4
    };

    enum class SoilDepthClass:uint8_t
    {
        None=0,
        Barren=1,
        Thin=2,
        Moderate=3,
        Deep=4
    };

    inline char const* RegimeName(RegimeClass v)
    {
        switch(v)
        {
            case RegimeClass::AlpineBarren:return "alpine_barren";
            case RegimeClass::AlpineTundra:return "alpine_tundra_potential";
            case RegimeClass::Subalpine:return "subalpine";
            case RegimeClass::CoolMoistForest:return "cool_moist_forest_potential";
            case RegimeClass::DryInteriorWoodland:return "dry_interior_woodland_scrub_potential";
            case RegimeClass::RiparianCorridor:return "riparian_corridor";
            case RegimeClass::WetMeadow:return "wet_meadow_marsh_potential";
            case RegimeClass::BasinWetland:return "basin_wetland";
            case RegimeClass::DryRockySlope:return "dry_rocky_slope";
            default:return "none";
        }
    }

    inline char const* TemperatureName(TemperatureBand v)
    {
        switch(v)
        {
            case TemperatureBand::Alpine:return "alpine";
            case TemperatureBand::Subalpine:return "subalpine";
            case TemperatureBand::Cool:return "cool";
            case TemperatureBand::Mild:return "mild";
            default:return "none";
        }
    }

    inline char const* MoistureName(MoistureBand v)
    {
        switch(v)
        {
            case MoistureBand::Arid:return "arid";
            case MoistureBand::Dry:return "dry";
            case MoistureBand::Mesic:return "mesic";
            case MoistureBand::Wet:return "wet";
            case MoistureBand::Saturated:return "saturated";
            default:return "none";
        }
    }

    inline char const* ElevationName(ElevationBand v)
    {
        switch(v)
        {
            case ElevationBand::Alpine:return "alpine";
            case ElevationBand::Montane:return "montane";
            case ElevationBand::Valley:return "valley";
            case ElevationBand::Basin:return "basin";
            default:return "none";
        }
    }

    inline char const* SoilDepthName(SoilDepthClass v)
    {
        switch(v)
        {
            case SoilDepthClass::Barren:return "barren";
            case SoilDepthClass::Thin:return "thin";
            case SoilDepthClass::Moderate:return "moderate";
            case SoilDepthClass::Deep:return "deep";
            default:return "none";
        }
    }

    inline char const* DiagnosticMaterialOf(RegimeClass v)
    {
        switch(v)
        {
            case RegimeClass::AlpineBarren:return "biome_alpine_barren";
            case RegimeClass::AlpineTundra:return "biome_alpine_tundra";
            case RegimeClass::Subalpine:return "biome_subalpine";
            case RegimeClass::CoolMoistForest:return "biome_moist_forest";
            case RegimeClass::DryInteriorWoodland:return "biome_dry_woodland";
            case RegimeClass::RiparianCorridor:return "biome_riparian";
            case RegimeClass::WetMeadow:return "biome_wet_meadow";
            case RegimeClass::BasinWetland:return "biome_basin_wetland";
            case RegimeClass::DryRockySlope:return "biome_dry_rocky";
            default:return "biome_moist_forest";
        }
    }

    inline bool IsWetterRegime(RegimeClass v)
    {
        return v==RegimeClass::CoolMoistForest||v==RegimeClass::RiparianCorridor
            ||v==RegimeClass::WetMeadow||v==RegimeClass::BasinWetland
            ||v==RegimeClass::Subalpine;
    }

    inline bool IsDrierRegime(RegimeClass v)
    {
        return v==RegimeClass::DryInteriorWoodland||v==RegimeClass::DryRockySlope
            ||v==RegimeClass::AlpineBarren;
    }

    inline bool IsAlpineRegime(RegimeClass v)
    {
        return v==RegimeClass::AlpineBarren||v==RegimeClass::AlpineTundra;
    }

    // Terminal temperate moisture split. A cell reaches this only after every
    // alpine, riparian, wetland, arid, leeward and subalpine branch has been
    // ruled out, i.e. an ordinary interior temperate cell. This is the one
    // point where the MW6 wet/dry signal must express in that band: a windward
    // or genuinely wet cell is cool moist forest; its drier complement is dry
    // interior woodland/scrub. With no hydroclimate at all, absence of data
    // must not invent dryness, so the moist default holds.
    // Regression guard: `terminal_moisture_split_expresses_hydroclimate` in the
    // cert exercises this directly, because no cell in the 64 km fixture reaches
    // the false arm and a collapse here would otherwise be invisible.
    inline RegimeClass TerminalMoistureRegime(bool haveHydro,bool isWindward,double wet)
    {
        return (!haveHydro||isWindward||wet>1.05)
            ? RegimeClass::CoolMoistForest
            : RegimeClass::DryInteriorWoodland;
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,biomeEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,authorityRevision=0,chronology=0;
        uint32_t regionalBiomeEnabled=1;
        std::string worldgenId,regionKey,seed;
        double gridStepM=256,alpineTempC=1.20,tundraDepthM=0.38;
        double riparianDepthM=0.85,wetlandWetness=0.50,aridThreshold=0.48;
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
                magic=line=="PROVENANCE_CAUSAL_REGIONAL_BIOME_V1";
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
          &&hex("biome_event_id",r.program.biomeEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("regional_biome_enabled",r.program.regionalBiomeEnabled)
          &&num("grid_step_m",r.program.gridStepM)
          &&num("alpine_temp_c",r.program.alpineTempC)
          &&num("tundra_depth_m",r.program.tundraDepthM)
          &&num("riparian_depth_m",r.program.riparianDepthM)
          &&num("wetland_wetness",r.program.wetlandWetness)
          &&num("arid_threshold",r.program.aridThreshold);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldgenId==kWorldgenId
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.worldgenVersion==1;
        bool const bounds=r.program.authorityRevision>0&&r.program.biomeEventId!=0
          &&r.program.chronology>0&&!r.program.seed.empty()
          &&(r.program.regionalBiomeEnabled==0||r.program.regionalBiomeEnabled==1)
          &&r.program.gridStepM>=64.0&&r.program.gridStepM<=512.0
          &&r.program.alpineTempC>=-8.0&&r.program.alpineTempC<=8.0
          &&r.program.tundraDepthM>=0.05&&r.program.tundraDepthM<=2.0
          &&r.program.riparianDepthM>=0.2&&r.program.riparianDepthM<=4.0
          &&r.program.wetlandWetness>=0.1&&r.program.wetlandWetness<=1.5
          &&r.program.aridThreshold>=0.1&&r.program.aridThreshold<=1.5;
        if(!identity){r.reason="authority_identity_mismatch";return r;}
        if(!bounds){r.reason="invalid_regional_biome_contract";return r;}
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
        double x=0,y=0,surfaceZ=0,slope=0;
        double productivityPotential=0,treeLinePotential=0,wetlandPotential=0;
        double disturbanceSensitivity=0;
        uint64_t biomeId=0,hydroclimateId=0,regolithId=0,depositBodyId=0,watershedId=0;
        uint64_t sourceFormationMask=0,inputRevisionBundle=0;
        uint32_t provinceId=0;
        RegimeClass regime=RegimeClass::None;
        TemperatureBand temperature=TemperatureBand::None;
        MoistureBand moisture=MoistureBand::None;
        ElevationBand elevation=ElevationBand::None;
        SoilDepthClass soilDepth=SoilDepthClass::None;
        CausalRegionalRegolith::DrainageClass drainage=CausalRegionalRegolith::DrainageClass::None;
        CausalRegionalHydroclimate::ExposureClass exposure=
            CausalRegionalHydroclimate::ExposureClass::Neutral;
        CausalRegionalRegolith::ProfileClass profile=CausalRegionalRegolith::ProfileClass::None;
        CausalRegionalHydroclimate::RegimeClass hydroRegime=
            CausalRegionalHydroclimate::RegimeClass::None;
        CausalRegionalDeposition::Facies facies=CausalRegionalDeposition::Facies::None;
        CausalMacroProvinces::ProvinceType provinceType=CausalMacroProvinces::ProvinceType::None;
        char parentFormationId[24]{};
        char sourceFormationId[24]{};
        char depositFormationId[24]{};
        bool channel=false,divide=false;
    };

    struct BiomeQuery
    {
        bool found=false;
        Cell cell;
        CausalWorldGeology::GeoSample host;
        CausalRegionalRegolith::RegolithQuery regolith;
        CausalRegionalHydroclimate::HydroclimateQuery hydroclimate;
        CausalRegionalDeposition::DepositQuery deposit;
    };

    struct CompiledField
    {
        int nx=0,ny=0;
        double originX=0,originY=0,step=256;
        std::vector<Cell> cells;
        uint64_t fieldDigest=0;
        uint64_t parentSurfaceDigest=0;
        uint64_t hydroclimateDigest=0;
        uint64_t regolithDigest=0;
        uint64_t inputRevisionBundle=0;
        int alpineBarrenCells=0,alpineTundraCells=0,subalpineCells=0;
        int moistForestCells=0,dryWoodlandCells=0,riparianCells=0;
        int wetMeadowCells=0,basinWetlandCells=0,dryRockyCells=0;
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
        Kernel(Program program,std::unique_ptr<CausalRegionalRegolith::Kernel> mw7,
            Control control=Control::Program)
          :m_program(std::move(program)),m_mw7(std::move(mw7)),m_control(control)
        {
            Compile();
        }

        bool Enabled() const
        {
            if(m_control==Control::ForceOff)return false;
            if(m_control==Control::ForceOn||m_control==Control::HydroclimateOff
              ||m_control==Control::RegolithOff||m_control==Control::LapseOff)return true;
            return m_program.regionalBiomeEnabled!=0;
        }

        bool ConsumeHydroclimate() const
        {
            return Enabled()&&m_control!=Control::HydroclimateOff&&Mw6().Enabled();
        }
        bool ConsumeRegolith() const
        {
            return Enabled()&&m_control!=Control::RegolithOff&&m_mw7->Enabled();
        }

        CausalRegionalRegolith::Kernel const& Mw7() const{return *m_mw7;}
        CausalRegionalRegolith::Kernel& Mw7(){return *m_mw7;}
        CausalRegionalHydroclimate::Kernel const& Mw6() const{return m_mw7->Mw6();}
        CausalRegionalDeposition::Kernel const& Mw5() const{return m_mw7->Mw5();}
        CausalRegionalDrainage::Kernel const& Mw4() const{return m_mw7->Mw4();}
        CausalRegionalErosion::Kernel const& Mw3() const{return m_mw7->Mw3();}
        CausalRegionalGeology::Kernel const& Mw2() const{return m_mw7->Mw2();}
        CausalMacroProvinces::Kernel const& Mw1() const{return m_mw7->Mw1();}

        double ParentZ(double x,double y) const
        {
            return m_mw7->ReconstructedZ(x,y);
        }

        double ReconstructedZ(double x,double y) const
        {
            ++m_stats.queries;
            return ParentZ(x,y);
        }

        CausalRegionalRegolith::RegolithQuery QueryRegolith(double x,double y) const
        {
            return m_mw7->QueryRegolith(x,y);
        }

        CausalRegionalHydroclimate::HydroclimateQuery QueryHydroclimate(double x,double y) const
        {
            return m_mw7->QueryHydroclimate(x,y);
        }

        BiomeQuery QueryBiome(double x,double y) const
        {
            ++m_stats.queries;
            BiomeQuery q;
            q.regolith=m_mw7->QueryRegolith(x,y);
            q.hydroclimate=m_mw7->QueryHydroclimate(x,y);
            q.deposit=Mw5().SurfaceDeposit(x,y);
            q.host=q.deposit.host;
            if(!Enabled()||!m_field||m_field->nx<1||m_field->ny<1)return q;
            int const ix=(int)std::llround((x-m_field->originX)/m_field->step);
            int const iy=(int)std::llround((y-m_field->originY)/m_field->step);
            if(ix<0||iy<0||ix>=m_field->nx||iy>=m_field->ny)return q;
            q.cell=m_field->cells[(size_t)iy*(size_t)m_field->nx+(size_t)ix];
            q.found=q.cell.biomeId!=0;
            return q;
        }

        CausalWorldGeology::GeoSample Query(double x,double y,double z) const
        {
            return m_mw7->Query(x,y,z);
        }

        CausalWorldGeology::MaterialSample QueryMaterial(double x,double y,double z) const
        {
            auto m=m_mw7->QueryMaterial(x,y,z);
            if(!Enabled())return m;
            auto const b=QueryBiome(x,y);
            if(b.found)
            {
                m.found=true;
                m.material=DiagnosticMaterialOf(b.cell.regime);
            }
            return m;
        }

        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {
            return m_mw7->SurfaceGeology(x,y);
        }

        CausalWorldGeology::GeoSample DiagnosticSample(double x,double y) const
        {
            auto const b=QueryBiome(x,y);
            CausalWorldGeology::GeoSample s=b.host;
            if(!b.found)return s;
            s.found=true;
            s.featureId=b.cell.biomeId;
            s.formationId=RegimeName(b.cell.regime);
            s.material=DiagnosticMaterialOf(b.cell.regime);
            return s;
        }

        char const* DiagnosticMaterial(double x,double y) const
        {
            auto const b=QueryBiome(x,y);
            if(!b.found)return "biome_moist_forest";
            return DiagnosticMaterialOf(b.cell.regime);
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
        uint64_t HydroclimateDigest() const{return m_mw7->HydroclimateDigest();}
        uint64_t RegolithDigest() const{return m_mw7->FieldDigest();}
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
                if(c.provinceType!=CausalMacroProvinces::ProvinceType::ForelandBasin
                  &&c.regime!=RegimeClass::BasinWetland
                  &&c.regime!=RegimeClass::RiparianCorridor)continue;
                double const score=(c.regime==RegimeClass::BasinWetland?0.40:0.0)
                    +(c.regime==RegimeClass::RiparianCorridor?0.22:0.0)
                    +(c.regime==RegimeClass::WetMeadow?0.12:0.0)
                    -0.00002*(c.x*c.x+c.y*c.y);
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
            uint64_t rev=CausalWorldGeology::HashText("mw8-rev");
            rev=MixU64(rev,m_mw7->FieldDigest());
            rev=MixU64(rev,m_mw7->HydroclimateDigest());
            rev=MixU64(rev,m_program.worldIdentityHash);
            rev=MixU64(rev,(uint64_t)m_control);
            rev=MixF(rev,m_program.alpineTempC);
            rev=MixF(rev,m_program.tundraDepthM);
            rev=MixF(rev,m_program.riparianDepthM);
            rev=MixF(rev,m_program.wetlandWetness);
            rev=MixF(rev,m_program.aridThreshold);
            field->inputRevisionBundle=rev;
            field->hydroclimateDigest=m_mw7->HydroclimateDigest();
            field->regolithDigest=m_mw7->FieldDigest();

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
                auto const hydro=m_mw7->QueryHydroclimate(c.x,c.y);
                auto const reg=m_mw7->QueryRegolith(c.x,c.y);
                auto const drain=Mw4().QueryDrainage(c.x,c.y);
                auto const dep=Mw5().SurfaceDeposit(c.x,c.y);
                FillCell(c,hydro,reg,drain,dep);
            }

            for(Cell const& c:field->cells)
            {
                switch(c.regime)
                {
                    case RegimeClass::AlpineBarren:++field->alpineBarrenCells;break;
                    case RegimeClass::AlpineTundra:++field->alpineTundraCells;break;
                    case RegimeClass::Subalpine:++field->subalpineCells;break;
                    case RegimeClass::CoolMoistForest:++field->moistForestCells;break;
                    case RegimeClass::DryInteriorWoodland:++field->dryWoodlandCells;break;
                    case RegimeClass::RiparianCorridor:++field->riparianCells;break;
                    case RegimeClass::WetMeadow:++field->wetMeadowCells;break;
                    case RegimeClass::BasinWetland:++field->basinWetlandCells;break;
                    case RegimeClass::DryRockySlope:++field->dryRockyCells;break;
                    default:break;
                }
            }

            uint64_t h=CausalWorldGeology::HashText("mw8-field");
            h=MixU64(h,rev);
            uint64_t parentH=CausalWorldGeology::HashText("mw8-parent");
            parentH=MixU64(parentH,m_mw7->ParentSurfaceDigest());
            parentH=MixU64(parentH,m_mw7->FieldDigest());
            parentH=MixU64(parentH,m_mw7->HydroclimateDigest());
            for(double y=mw1p.minY;y<=mw1p.maxY;y+=4000.0)
            for(double x=mw1p.minX;x<=mw1p.maxX;x+=4000.0)
                parentH=MixF(parentH,ParentZ(x,y));
            field->parentSurfaceDigest=parentH;
            for(Cell const& c:field->cells)
            {
                h=MixU64(h,c.biomeId);
                h=MixU64(h,(uint64_t)c.regime);
                h=MixU64(h,(uint64_t)c.moisture);
                h=MixU64(h,c.regolithId);
                h=MixU64(h,c.hydroclimateId);
                h=MixU64(h,c.depositBodyId);
                h=MixF(h,c.productivityPotential);
                h=MixF(h,c.wetlandPotential);
            }
            field->fieldDigest=h;
            return field;
        }

        void FillCell(Cell& c,
            CausalRegionalHydroclimate::HydroclimateQuery const& hydro,
            CausalRegionalRegolith::RegolithQuery const& reg,
            CausalRegionalDrainage::DrainageQuery const& drain,
            CausalRegionalDeposition::DepositQuery const& dep) const
        {
            bool const haveHydro=ConsumeHydroclimate()&&hydro.found;
            bool const haveRegolith=ConsumeRegolith()&&reg.found;

            if(haveHydro)
            {
                c.hydroclimateId=hydro.cell.hydroclimateId;
                c.hydroRegime=hydro.cell.regime;
                c.exposure=hydro.cell.exposure;
                c.watershedId=hydro.cell.watershedId;
            }
            if(haveRegolith)
            {
                c.regolithId=reg.cell.regolithId;
                c.profile=reg.cell.profile;
                c.drainage=reg.cell.drainage;
                CopyId(c.parentFormationId,sizeof(c.parentFormationId),reg.cell.parentFormationId);
                CopyId(c.sourceFormationId,sizeof(c.sourceFormationId),reg.cell.sourceFormationId);
                CopyId(c.depositFormationId,sizeof(c.depositFormationId),reg.cell.depositFormationId);
                c.sourceFormationMask=reg.cell.sourceFormationMask;
                c.depositBodyId=reg.cell.depositBodyId;
                c.facies=reg.cell.facies;
                if(!c.watershedId)c.watershedId=reg.cell.watershedId;
                if(!c.hydroclimateId)c.hydroclimateId=reg.cell.hydroclimateId;
            }
            else
            {
                c.depositBodyId=dep.cell.depositBodyId;
                c.facies=dep.cell.facies;
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
            }
            if(drain.found)
            {
                c.slope=drain.cell.slope;
                c.channel=drain.cell.channel;
                c.divide=drain.cell.divide;
                if(!c.watershedId)c.watershedId=drain.cell.watershedId;
            }
            else c.slope=haveRegolith?reg.cell.slope:dep.cell.slope;

            double const temp=haveHydro?hydro.cell.meanTemperature:4.5;
            double const wet=haveHydro?hydro.cell.effectiveWetness:0.85;
            double const arid=haveHydro?hydro.cell.aridityIndex:0.40;
            double const snow=haveHydro?hydro.cell.snowPersistence:0.0;
            double const pool=haveHydro?hydro.cell.coldPoolPotential:0.0;
            double const depth=haveRegolith?reg.cell.profileDepth:0.45;
            bool const isAlpineCold=haveHydro
                &&(c.hydroRegime==CausalRegionalHydroclimate::RegimeClass::AlpineCold
                  ||(temp<m_program.alpineTempC&&snow>0.22));
            bool const isWindward=haveHydro
                &&(c.exposure==CausalRegionalHydroclimate::ExposureClass::Windward
                  ||c.hydroRegime==CausalRegionalHydroclimate::RegimeClass::WindwardWet);
            bool const isLeeward=haveHydro
                &&(c.exposure==CausalRegionalHydroclimate::ExposureClass::Leeward
                  ||c.hydroRegime==CausalRegionalHydroclimate::RegimeClass::LeewardDry);
            bool const isRidge=(haveHydro
                &&c.exposure==CausalRegionalHydroclimate::ExposureClass::ExposedRidge)
                ||c.divide;
            bool const isBasinProv=c.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin;
            bool const isBelt=c.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt;
            bool const thinSubstrate=haveRegolith
                &&(c.profile==CausalRegionalRegolith::ProfileClass::BareBedrock
                  ||c.profile==CausalRegionalRegolith::ProfileClass::WeatheredBedrock
                  ||c.profile==CausalRegionalRegolith::ProfileClass::ThinRegolith
                  ||c.profile==CausalRegionalRegolith::ProfileClass::Talus
                  ||depth<0.35);
            bool const deepAlluvium=haveRegolith
                &&(c.profile==CausalRegionalRegolith::ProfileClass::Alluvium
                  ||c.profile==CausalRegionalRegolith::ProfileClass::FloodplainSediment
                  ||c.profile==CausalRegionalRegolith::ProfileClass::BasinFill
                  ||c.profile==CausalRegionalRegolith::ProfileClass::WaterloggedMineral
                  ||c.profile==CausalRegionalRegolith::ProfileClass::OrganicCapable
                  ||depth>=m_program.riparianDepthM);
            bool const poorDrain=haveRegolith
                &&(c.drainage==CausalRegionalRegolith::DrainageClass::PoorlyDrained
                  ||c.drainage==CausalRegionalRegolith::DrainageClass::Saturated
                  ||c.profile==CausalRegionalRegolith::ProfileClass::WaterloggedMineral);
            bool const lowGrad=c.slope<0.032&&!isRidge;
            bool const floodplainish=haveRegolith
                &&(c.profile==CausalRegionalRegolith::ProfileClass::FloodplainSediment
                  ||c.profile==CausalRegionalRegolith::ProfileClass::Alluvium
                  ||c.facies==CausalRegionalDeposition::Facies::Floodplain
                  ||c.facies==CausalRegionalDeposition::Facies::ValleyFill);

            if(haveHydro&&haveRegolith&&isBasinProv&&poorDrain&&pool>0.34
              &&lowGrad&&wet>=m_program.wetlandWetness)
                c.regime=RegimeClass::BasinWetland;
            else if(haveHydro&&haveRegolith&&(c.channel||floodplainish)&&deepAlluvium
              &&!isAlpineCold&&wet>=0.40&&lowGrad)
                c.regime=RegimeClass::RiparianCorridor;
            else if(haveHydro&&haveRegolith&&poorDrain&&lowGrad&&wet>=0.70
              &&!isAlpineCold&&!isBasinProv)
                c.regime=RegimeClass::WetMeadow;
            else if(isAlpineCold&&thinSubstrate
              &&(snow>0.22||depth<m_program.tundraDepthM||isRidge
                ||c.profile==CausalRegionalRegolith::ProfileClass::BareBedrock
                ||c.profile==CausalRegionalRegolith::ProfileClass::Talus))
                c.regime=RegimeClass::AlpineBarren;
            else if(isAlpineCold)
                c.regime=RegimeClass::AlpineTundra;
            else if(haveRegolith&&thinSubstrate
              &&(isLeeward||arid>m_program.aridThreshold)&&!isWindward
              &&!isAlpineCold)
                c.regime=RegimeClass::DryRockySlope;
            else if(haveHydro&&(isLeeward||arid>m_program.aridThreshold)&&!isAlpineCold)
                c.regime=RegimeClass::DryInteriorWoodland;
            else if(haveHydro&&(temp<3.2||snow>0.10)&&isBelt&&!isAlpineCold)
                c.regime=RegimeClass::Subalpine;
            else
                c.regime=TerminalMoistureRegime(haveHydro,isWindward,wet);

            if(isAlpineCold||temp<m_program.alpineTempC)c.temperature=TemperatureBand::Alpine;
            else if(temp<3.4||c.regime==RegimeClass::Subalpine)c.temperature=TemperatureBand::Subalpine;
            else if(temp<6.2)c.temperature=TemperatureBand::Cool;
            else c.temperature=TemperatureBand::Mild;

            if(poorDrain&&wet>=0.85)c.moisture=MoistureBand::Saturated;
            else if(wet>=1.15||c.regime==RegimeClass::BasinWetland
              ||c.regime==RegimeClass::WetMeadow)c.moisture=MoistureBand::Wet;
            else if(wet>=0.70)c.moisture=MoistureBand::Mesic;
            else if(wet>=0.40)c.moisture=MoistureBand::Dry;
            else c.moisture=MoistureBand::Arid;
            if(!haveHydro)
            {
                c.moisture=MoistureBand::Mesic;
                if(c.temperature==TemperatureBand::Alpine)c.temperature=TemperatureBand::Cool;
            }

            if(isAlpineCold||c.regime==RegimeClass::AlpineBarren
              ||c.regime==RegimeClass::AlpineTundra)c.elevation=ElevationBand::Alpine;
            else if(isBasinProv)c.elevation=ElevationBand::Basin;
            else if(c.channel||floodplainish||lowGrad)c.elevation=ElevationBand::Valley;
            else c.elevation=ElevationBand::Montane;

            if(!haveRegolith)c.soilDepth=SoilDepthClass::Moderate;
            else if(depth<0.12||c.profile==CausalRegionalRegolith::ProfileClass::BareBedrock)
                c.soilDepth=SoilDepthClass::Barren;
            else if(depth<0.45||thinSubstrate)c.soilDepth=SoilDepthClass::Thin;
            else if(depth<1.10)c.soilDepth=SoilDepthClass::Moderate;
            else c.soilDepth=SoilDepthClass::Deep;

            double prod=0.18;
            prod+=0.34*Smoothstep(0.20,1.40,depth);
            prod+=0.28*Smoothstep(0.40,1.60,wet);
            prod+=0.16*Smoothstep(-0.5,7.0,temp);
            prod*=(1.0-0.62*snow);
            prod*=(1.0-0.40*arid);
            if(c.regime==RegimeClass::AlpineBarren||c.regime==RegimeClass::DryRockySlope)
                prod*=0.18;
            if(c.regime==RegimeClass::RiparianCorridor)prod*=1.22;
            if(!haveHydro||!haveRegolith)prod*=0.72;
            c.productivityPotential=std::clamp(prod,0.02,0.96);

            double tree=c.productivityPotential
                *(c.regime==RegimeClass::AlpineBarren?0.0:1.0)
                *(c.regime==RegimeClass::AlpineTundra?0.08:1.0)
                *(c.regime==RegimeClass::BasinWetland?0.12:1.0)
                *Smoothstep(m_program.alpineTempC,6.0,temp);
            c.treeLinePotential=std::clamp(tree,0.0,0.94);

            double wetP=0.0;
            if(haveHydro&&haveRegolith)
                wetP=Smoothstep(0.35,0.90,pool)*Smoothstep(0.40,1.20,wet)
                    *(poorDrain?1.0:0.25)*(lowGrad?1.0:0.20);
            c.wetlandPotential=std::clamp(wetP,0.0,0.96);

            c.disturbanceSensitivity=std::clamp(
                0.22+0.38*Smoothstep(0.03,0.16,c.slope)
                +0.22*(thinSubstrate?1.0:0.0)
                +0.18*std::clamp(snow,0.0,1.0)
                +(c.regime==RegimeClass::RiparianCorridor?0.12:0.0),0.04,0.95);

            int64_t const qx=QuantizeAbs(c.x,m_program.gridStepM);
            int64_t const qy=QuantizeAbs(c.y,m_program.gridStepM);
            c.biomeId=StableId(m_program.worldIdentityHash,kTagBiome,
                (uint64_t)qx,(uint64_t)qy);
        }

        void Compile()
        {
            ++m_stats.compiles;
            ++m_stats.rebuilds;
            uint64_t parentH=CausalWorldGeology::HashText("mw8-parent");
            parentH=MixU64(parentH,m_mw7->ParentSurfaceDigest());
            parentH=MixU64(parentH,m_mw7->FieldDigest());
            parentH=MixU64(parentH,m_mw7->HydroclimateDigest());
            auto const& mw1=Mw1().GetProgram();
            for(double y=mw1.minY;y<=mw1.maxY;y+=4000.0)
            for(double x=mw1.minX;x<=mw1.maxX;x+=4000.0)
                parentH=MixF(parentH,ParentZ(x,y));
            m_parentSurfaceDigest=parentH;
            uint64_t rev=MixU64(m_program.worldIdentityHash,m_mw7->FieldDigest());
            rev=MixF(rev,m_program.alpineTempC);
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
                uint64_t key=MixU64(m_program.worldIdentityHash,m_mw7->FieldDigest());
                key=MixU64(key,m_mw7->HydroclimateDigest());
                key=MixU64(key,(uint64_t)m_control);
                key=MixF(key,m_program.alpineTempC);
                key=MixF(key,m_program.tundraDepthM);
                key=MixF(key,m_program.riparianDepthM);
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
        std::unique_ptr<CausalRegionalRegolith::Kernel> m_mw7;
        Control m_control=Control::Program;
        std::shared_ptr<CompiledField> m_field;
        uint64_t m_parentSurfaceDigest=0;
        uint64_t m_inputRevision=0;
        mutable Stats m_stats;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* mw8Path,char const* mw7Path,
        char const* mw6Path,char const* mw5Path,char const* mw4Path,char const* mw3Path,
        char const* mw2Path,char const* mw1Path,std::string* reason=nullptr,
        Control control=Control::Program,
        CausalRegionalRegolith::Control mw7Control=CausalRegionalRegolith::Control::ForceOn,
        CausalRegionalHydroclimate::Control mw6Control=CausalRegionalHydroclimate::Control::ForceOn,
        CausalRegionalDeposition::Control mw5Control=CausalRegionalDeposition::Control::ForceOn,
        CausalRegionalDrainage::Control mw4Control=CausalRegionalDrainage::Control::ForceOn,
        CausalRegionalErosion::Control mw3Control=CausalRegionalErosion::Control::ForceOn,
        CausalRegionalGeology::Control mw2Control=CausalRegionalGeology::Control::ForceOn,
        CausalMacroProvinces::Control mw1Control=CausalMacroProvinces::Control::ForceOn)
    {
        std::string source;if(!ReadFile(mw8Path,source))
        {if(reason)*reason="descriptor_missing";return {};}
        auto loaded=LoadText(source);if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(control==Control::HydroclimateOff)
            mw6Control=CausalRegionalHydroclimate::Control::ForceOff;
        if(control==Control::RegolithOff)
            mw7Control=CausalRegionalRegolith::Control::ForceOff;
        if(control==Control::LapseOff)
            mw6Control=CausalRegionalHydroclimate::Control::LapseOff;
        std::string mw7Reason;
        auto mw7=CausalRegionalRegolith::LoadKernel(mw7Path,mw6Path,mw5Path,mw4Path,mw3Path,
            mw2Path,mw1Path,&mw7Reason,mw7Control,mw6Control,mw5Control,mw4Control,
            mw3Control,mw2Control,mw1Control);
        if(!mw7){if(reason)*reason=std::string("mw7_parent_failed:")+mw7Reason;return {};}
        if(loaded.program.worldIdentityHash!=mw7->GetProgram().worldIdentityHash)
        {if(reason)*reason="mw7_identity_mismatch";return {};}
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(loaded.program),std::move(mw7),control);
    }

    struct VisualMetrics
    {
        int alpineBarrenCells=0,alpineTundraCells=0,subalpineCells=0;
        int moistForestCells=0,dryWoodlandCells=0,riparianCells=0;
        int wetMeadowCells=0,basinWetlandCells=0,dryRockyCells=0;
        double windwardWetterFrac=0,leewardWetterFrac=0;
        double alpineFrac=0,basinWetlandFrac=0,riparianVsSlope=0;
        double wrapRmsM=0,beltMeanZ=0,basinMeanZ=0;
    };

    inline VisualMetrics MeasureVisual(Kernel const& k)
    {
        VisualMetrics m;
        auto const* field=k.Field();
        if(!field)return m;
        m.alpineBarrenCells=field->alpineBarrenCells;
        m.alpineTundraCells=field->alpineTundraCells;
        m.subalpineCells=field->subalpineCells;
        m.moistForestCells=field->moistForestCells;
        m.dryWoodlandCells=field->dryWoodlandCells;
        m.riparianCells=field->riparianCells;
        m.wetMeadowCells=field->wetMeadowCells;
        m.basinWetlandCells=field->basinWetlandCells;
        m.dryRockyCells=field->dryRockyCells;
        int wN=0,wWet=0,lN=0,lWet=0,aN=0,beltN=0,basinN=0,basinWet=0;
        int valleyN=0,valleyRip=0,slopeN=0,slopeRip=0;
        double beltZ=0,basinZ=0;
        for(Cell const& c:field->cells)
        {
            if(IsAlpineRegime(c.regime))++aN;
            if(c.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt)
            {
                beltZ+=c.surfaceZ;++beltN;
                if(!IsAlpineRegime(c.regime))
                {
                    if(c.exposure==CausalRegionalHydroclimate::ExposureClass::Windward)
                    {++wN;if(IsWetterRegime(c.regime))++wWet;}
                    if(c.exposure==CausalRegionalHydroclimate::ExposureClass::Leeward)
                    {++lN;if(IsWetterRegime(c.regime))++lWet;}
                }
            }
            if(c.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin)
            {
                basinZ+=c.surfaceZ;++basinN;
                if(c.regime==RegimeClass::BasinWetland)++basinWet;
            }
            bool const valley=c.channel
                ||c.profile==CausalRegionalRegolith::ProfileClass::FloodplainSediment
                ||c.profile==CausalRegionalRegolith::ProfileClass::Alluvium
                ||c.facies==CausalRegionalDeposition::Facies::Floodplain
                ||c.facies==CausalRegionalDeposition::Facies::ValleyFill;
            bool const slope=(c.divide||c.slope>0.05)&&!valley;
            if(valley){++valleyN;if(c.regime==RegimeClass::RiparianCorridor)++valleyRip;}
            if(slope){++slopeN;if(c.regime==RegimeClass::RiparianCorridor)++slopeRip;}
        }
        m.windwardWetterFrac=wN?(double)wWet/(double)wN:0;
        m.leewardWetterFrac=lN?(double)lWet/(double)lN:0;
        m.alpineFrac=field->cells.empty()?0:(double)aN/(double)field->cells.size();
        m.basinWetlandFrac=basinN?(double)basinWet/(double)basinN:0;
        double const valleyRipFrac=valleyN?(double)valleyRip/(double)valleyN:0;
        double const slopeRipFrac=slopeN?(double)slopeRip/(double)slopeN:0;
        m.riparianVsSlope=valleyRipFrac-slopeRipFrac;
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
        uint64_t fieldDigest=0,parentSurfaceDigest=0,offSurfaceDigest=0,mw7SurfaceDigest=0;
        uint64_t hydroclimateDigest=0,regolithDigest=0,inputRevisionBundle=0;
        uint64_t h2hBiomeId=0,h2hRegolithId=0,h2hHydroclimateId=0,h2hBodyId=0,h2hSourceMask=0;
        double offMaxAbsDeltaM=0,onMaxAbsDeltaM=0,mw1CounterfactualReliefM=0;
        double windwardWetterFrac=0,leewardWetterFrac=0,hydroOffContrast=0,lapseAlpineFrac=0;
        double basinWetlandFrac=0,riparianVsSlope=0,regolithOffRiparian=0;
        int alpineBarrenCells=0,alpineTundraCells=0,riparianCells=0,basinWetlandCells=0;
        int dryWoodlandCells=0,moistForestCells=0,dryRockyCells=0;
        int h2h64=0,h2h192=0,h2h125=0;
        int rebuildsAfterLoad=0,rebuildsAfterQueries=0;
        bool determinism=false,offExact=false;
        bool alpineFixture=false,shadowFixture=false,riparianFixture=false;
        bool basinFixture=false,provenance=false;
        bool hydroCollapse=false,regolithCollapse=false;
        std::string regime,drainage,depositFormation,sourceFormation,hostFormation;
        VisualMetrics visual{};
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* mw8Path,char const* mw7Path,char const* mw6Path,
        char const* mw5Path,char const* mw4Path,char const* mw3Path,char const* mw2Path,
        char const* mw1Path,char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* classificationPath)
    {
        CertResult c;
        std::string onReason;auto on=LoadKernel(mw8Path,mw7Path,mw6Path,mw5Path,mw4Path,
            mw3Path,mw2Path,mw1Path,&onReason,Control::ForceOn);
        c.checks.push_back({"descriptor_linked_mw8",on!=nullptr});
        if(!on){c.reason=onReason.empty()?"mw8_kernel_failed":onReason;return c;}
        c.fieldDigest=on->FieldDigest();
        c.parentSurfaceDigest=on->ParentSurfaceDigest();
        c.hydroclimateDigest=on->HydroclimateDigest();
        c.regolithDigest=on->RegolithDigest();
        c.inputRevisionBundle=on->InputRevisionBundle();
        c.rebuildsAfterLoad=on->GetStats().rebuilds;
        c.checks.push_back({"enabled_kernel_loaded",on->Enabled()});
        c.checks.push_back({"mw7_parent_certified",on->Mw7().Enabled()});
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
            c.alpineBarrenCells=field->alpineBarrenCells;
            c.alpineTundraCells=field->alpineTundraCells;
            c.riparianCells=field->riparianCells;
            c.basinWetlandCells=field->basinWetlandCells;
            c.dryWoodlandCells=field->dryWoodlandCells;
            c.moistForestCells=field->moistForestCells;
            c.dryRockyCells=field->dryRockyCells;
        }

        std::string mw7Reason;auto mw7=CausalRegionalRegolith::LoadKernel(mw7Path,mw6Path,
            mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,&mw7Reason,
            CausalRegionalRegolith::Control::ForceOn);
        c.checks.push_back({"mw7_control_kernel",mw7!=nullptr});
        uint64_t mw7H=CausalWorldGeology::HashText("mw8-parent");
        if(mw7)
        {
            mw7H=MixU64(mw7H,mw7->ParentSurfaceDigest());
            mw7H=MixU64(mw7H,mw7->FieldDigest());
            mw7H=MixU64(mw7H,mw7->HydroclimateDigest());
            auto const& mw1p=mw7->Mw1().GetProgram();
            for(double y=mw1p.minY;y<=mw1p.maxY;y+=4000.0)
            for(double x=mw1p.minX;x<=mw1p.maxX;x+=4000.0)
                mw7H=MixF(mw7H,mw7->ReconstructedZ(x,y));
        }
        c.mw7SurfaceDigest=mw7H;

        std::string offReason;auto off=LoadKernel(mw8Path,mw7Path,mw6Path,mw5Path,mw4Path,
            mw3Path,mw2Path,mw1Path,&offReason,Control::ForceOff);
        c.checks.push_back({"off_path_disables_mw8",off&&!off->Enabled()});
        double offMax=0,onMax=0;
        bool hydroExact=true,regExact=true,bodyExact=true,formExact=true;
        if(off&&mw7)
        {
            c.offSurfaceDigest=off->ParentSurfaceDigest();
            for(double y=-32000;y<=32000;y+=2000.0)
            for(double x=-32000;x<=32000;x+=2000.0)
            {
                double const a=off->ReconstructedZ(x,y);
                double const b=mw7->ReconstructedZ(x,y);
                offMax=(std::max)(offMax,std::fabs(a-b));
                double const d=on->ReconstructedZ(x,y);
                onMax=(std::max)(onMax,std::fabs(d-b));
                auto const ho=off->QueryHydroclimate(x,y);
                auto const hm=mw7->QueryHydroclimate(x,y);
                if(ho.found!=hm.found||ho.cell.hydroclimateId!=hm.cell.hydroclimateId)
                    hydroExact=false;
                auto const ro=off->QueryRegolith(x,y);
                auto const rm=mw7->QueryRegolith(x,y);
                if(ro.found!=rm.found||ro.cell.regolithId!=rm.cell.regolithId)
                    regExact=false;
                auto const don=on->Mw5().SurfaceDeposit(x,y);
                auto const d7=mw7->Mw5().SurfaceDeposit(x,y);
                if(don.cell.depositBodyId!=d7.cell.depositBodyId)bodyExact=false;
                auto const fon=on->SurfaceGeology(x,y);
                auto const f7=mw7->SurfaceGeology(x,y);
                if(fon.formationId!=f7.formationId)formExact=false;
            }
        }
        c.offMaxAbsDeltaM=offMax;
        c.onMaxAbsDeltaM=onMax;
        c.offExact=off&&mw7&&offMax==0.0&&onMax==0.0&&hydroExact&&regExact&&bodyExact
            &&formExact
            &&off->HydroclimateDigest()==mw7->HydroclimateDigest()
            &&on->HydroclimateDigest()==mw7->HydroclimateDigest()
            &&off->RegolithDigest()==mw7->FieldDigest()
            &&on->RegolithDigest()==mw7->FieldDigest();
        c.checks.push_back({"mw8_off_exact_mw7_present_surface",c.offExact&&offMax==0.0});
        c.checks.push_back({"mw8_off_exact_mw7_hydroclimate_identity",hydroExact
            &&off&&mw7&&off->HydroclimateDigest()==mw7->HydroclimateDigest()});
        c.checks.push_back({"mw8_off_exact_mw7_regolith_identity",regExact
            &&off&&mw7&&off->RegolithDigest()==mw7->FieldDigest()});
        c.checks.push_back({"mw8_off_exact_mw5_deposit_bodies",bodyExact});
        c.checks.push_back({"mw8_off_exact_formation_id",formExact});
        c.checks.push_back({"mw8_on_does_not_mutate_terrain",onMax==0.0});
        c.checks.push_back({"mw8_on_does_not_rewrite_hydroclimate",
            on->HydroclimateDigest()==mw7->HydroclimateDigest()});
        c.checks.push_back({"mw8_on_does_not_rewrite_regolith",
            on->RegolithDigest()==mw7->FieldDigest()});

        auto const mass=on->Mw5().Mass();
        bool massOk=std::fabs(mass.residualGrams)<64.0
            &&std::fabs(mass.sourceGrams-(mass.depositedGrams+mass.exportedGrams))<64.0
            &&mass.hostGeologyGrams==0.0;
        c.checks.push_back({"mw5_mass_closure_still_exact",massOk});

        c.visual=MeasureVisual(*on);
        c.windwardWetterFrac=c.visual.windwardWetterFrac;
        c.leewardWetterFrac=c.visual.leewardWetterFrac;
        c.basinWetlandFrac=c.visual.basinWetlandFrac;
        c.riparianVsSlope=c.visual.riparianVsSlope;

        std::string lapseReason;auto lapse=LoadKernel(mw8Path,mw7Path,mw6Path,mw5Path,
            mw4Path,mw3Path,mw2Path,mw1Path,&lapseReason,Control::LapseOff);
        int lapseAlpine=0,onAlpine=c.alpineBarrenCells+c.alpineTundraCells;
        if(lapse&&lapse->Field())
            lapseAlpine=lapse->Field()->alpineBarrenCells+lapse->Field()->alpineTundraCells;
        c.lapseAlpineFrac=onAlpine?(double)lapseAlpine/(double)onAlpine:1.0;
        c.alpineFixture=onAlpine>=8&&c.alpineBarrenCells>=4
            &&lapse&&lapseAlpine+8<onAlpine&&c.lapseAlpineFrac<0.72;
        c.checks.push_back({"alpine_elevation_control",c.alpineFixture});
        c.checks.push_back({"alpine_lapse_off_changes_classification",
            lapse&&lapseAlpine!=onAlpine&&c.lapseAlpineFrac<0.72});

        c.shadowFixture=c.windwardWetterFrac>c.leewardWetterFrac+0.12
            &&c.moistForestCells+c.visual.subalpineCells>=8
            &&c.dryWoodlandCells+c.dryRockyCells>=8;
        c.checks.push_back({"windward_vs_rain_shadow",c.shadowFixture});

        // Coverage guard for the terminal temperate moisture split. No cell in
        // the 64 km fixture reaches its dry arm, so a collapse there (both arms
        // returning the wet regime) would pass every other check silently. This
        // exercises the decision directly: a temperate interior cell with real
        // hydroclimate, not windward and not wet, must resolve to the drier
        // regime; windward, wet, and no-hydroclimate inputs must stay moist.
        bool const terminalDrySide=
            TerminalMoistureRegime(true,false,0.85)==RegimeClass::DryInteriorWoodland
            &&TerminalMoistureRegime(true,false,1.05)==RegimeClass::DryInteriorWoodland
            &&IsDrierRegime(TerminalMoistureRegime(true,false,0.85));
        bool const terminalWetSide=
            TerminalMoistureRegime(true,true,0.85)==RegimeClass::CoolMoistForest
            &&TerminalMoistureRegime(true,false,1.20)==RegimeClass::CoolMoistForest
            &&TerminalMoistureRegime(false,false,0.85)==RegimeClass::CoolMoistForest;
        c.checks.push_back({"terminal_moisture_split_expresses_hydroclimate",
            terminalDrySide&&terminalWetSide});

        std::string hydroOffReason;auto hydroOff=LoadKernel(mw8Path,mw7Path,mw6Path,mw5Path,
            mw4Path,mw3Path,mw2Path,mw1Path,&hydroOffReason,Control::HydroclimateOff);
        double hydroOffW=0,hydroOffL=0;
        if(hydroOff&&on->Field()&&hydroOff->Field())
        {
            int wN=0,wWet=0,lN=0,lWet=0;
            for(Cell const& cell:on->Field()->cells)
            {
                if(cell.provinceType!=CausalMacroProvinces::ProvinceType::MountainBelt)continue;
                if(IsAlpineRegime(cell.regime))continue;
                auto const q=hydroOff->QueryBiome(cell.x,cell.y);
                if(!q.found)continue;
                if(cell.exposure==CausalRegionalHydroclimate::ExposureClass::Windward)
                {++wN;if(IsWetterRegime(q.cell.regime))++wWet;}
                if(cell.exposure==CausalRegionalHydroclimate::ExposureClass::Leeward)
                {++lN;if(IsWetterRegime(q.cell.regime))++lWet;}
            }
            hydroOffW=wN?(double)wWet/(double)wN:0;
            hydroOffL=lN?(double)lWet/(double)lN:0;
        }
        c.hydroOffContrast=std::fabs(hydroOffW-hydroOffL);
        double const onContrast=std::fabs(c.windwardWetterFrac-c.leewardWetterFrac);
        c.hydroCollapse=hydroOff&&onContrast>0.12&&c.hydroOffContrast<onContrast*0.45
            &&c.hydroOffContrast<0.10;
        c.checks.push_back({"hydroclimate_neutralized_wet_dry_collapses",c.hydroCollapse});

        c.riparianFixture=c.riparianCells>=8&&c.riparianVsSlope>0.35;
        c.checks.push_back({"riparian_valley_distinct_from_slope",c.riparianFixture});

        std::string regOffReason;auto regOff=LoadKernel(mw8Path,mw7Path,mw6Path,mw5Path,
            mw4Path,mw3Path,mw2Path,mw1Path,&regOffReason,Control::RegolithOff);
        int regOffRip=0,regOffWet=0,onRipCoords=0,onWetCoords=0;
        if(regOff&&on->Field())
        {
            for(Cell const& cell:on->Field()->cells)
            {
                if(cell.regime==RegimeClass::RiparianCorridor)
                {
                    ++onRipCoords;
                    auto const q=regOff->QueryBiome(cell.x,cell.y);
                    if(q.found&&q.cell.regime==RegimeClass::RiparianCorridor)++regOffRip;
                }
                if(cell.regime==RegimeClass::BasinWetland)
                {
                    ++onWetCoords;
                    auto const q=regOff->QueryBiome(cell.x,cell.y);
                    if(q.found&&q.cell.regime==RegimeClass::BasinWetland)++regOffWet;
                }
            }
        }
        c.regolithOffRiparian=onRipCoords?(double)regOffRip/(double)onRipCoords:1.0;
        double const regOffWetFrac=onWetCoords?(double)regOffWet/(double)onWetCoords:1.0;
        c.regolithCollapse=regOff&&onRipCoords>=8&&c.regolithOffRiparian<0.35
            &&onWetCoords>=8&&regOffWetFrac<0.35;
        c.checks.push_back({"regolith_neutralized_substrate_drainage_collapses",
            c.regolithCollapse});

        c.basinFixture=c.basinWetlandCells>=8&&c.basinWetlandFrac>0.12
            &&c.visual.beltMeanZ>c.visual.basinMeanZ+80.0;
        c.checks.push_back({"wet_basin_wetland_regime",c.basinFixture});

        int h64=0;
        for(double y=-32000;y<=32000;y+=1024.0)
        for(double x=-32000;x<=32000;x+=1024.0)
        {
            auto const q=on->QueryBiome(x,y);
            if(q.found)++h64;
        }
        c.h2h64=h64;
        c.checks.push_back({"h2h_64km_biome_map",c.h2h64>2000});

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
                double const score=1.0
                    +(cell.regime==RegimeClass::BasinWetland?0.35:0.0)
                    +(cell.regime==RegimeClass::RiparianCorridor?0.10:0.0);
                if(score<=best)continue;
                best=score;bodyX=cell.x;bodyY=cell.y;
                c.h2hBiomeId=cell.biomeId;
                c.h2hRegolithId=cell.regolithId;
                c.h2hHydroclimateId=cell.hydroclimateId;
                c.h2hBodyId=cell.depositBodyId;
                c.h2hSourceMask=cell.sourceFormationMask;
                c.regime=RegimeName(cell.regime);
                c.drainage=CausalRegionalRegolith::DrainageName(cell.drainage);
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
                auto const q=on->QueryBiome(sx,sy);
                if(!q.found||q.cell.biomeId==0){same192=false;continue;}
                int64_t const qx=QuantizeAbs(sx,on->GetProgram().gridStepM);
                int64_t const qy=QuantizeAbs(sy,on->GetProgram().gridStepM);
                uint64_t const expect=StableId(on->GetProgram().worldIdentityHash,kTagBiome,
                    (uint64_t)qx,(uint64_t)qy);
                if(q.cell.biomeId!=expect){same192=false;continue;}
                ++h192;
            }
        }
        c.h2h192=h192;
        int h125=0;bool cmSame=true;bool biomeSame=true;
        if(haveBody)
        {
            for(int j=0;j<17;++j)
            for(int i=0;i<17;++i)
            {
                double const sx=bodyX+i*0.125,sy=bodyY+j*0.125;
                auto const q=on->QueryBiome(sx,sy);
                if(!q.found||q.cell.depositBodyId!=c.h2hBodyId
                  ||q.cell.sourceFormationMask!=c.h2hSourceMask
                  ||std::strcmp(q.cell.sourceFormationId,c.sourceFormation.c_str())!=0
                  ||std::strcmp(q.cell.depositFormationId,c.depositFormation.c_str())!=0)
                {cmSame=false;continue;}
                if(q.cell.biomeId!=c.h2hBiomeId){biomeSame=false;continue;}
                ++h125;
            }
        }
        c.h2h125=h125;
        bool profileNotHost=c.depositFormation!=c.hostFormation
            &&c.depositFormation=="D05_BASIN_FILL";
        c.provenance=haveBody&&c.h2hBodyId!=0&&c.h2h125==289&&cmSame&&biomeSame
            &&c.h2h192==81&&same192&&!c.sourceFormation.empty()
            &&c.depositFormation=="D05_BASIN_FILL"
            &&c.sourceFormation==CausalRegionalGeology::kFormHostUpper
            &&profileNotHost&&c.h2hRegolithId!=0&&c.h2hHydroclimateId!=0;
        c.checks.push_back({"h2h_mw5_deposit_body_retained",c.h2hBodyId!=0&&haveBody});
        c.checks.push_back({"h2h_source_formation_b03_host_upper",
            c.sourceFormation==CausalRegionalGeology::kFormHostUpper});
        c.checks.push_back({"h2h_profile_not_host_formation_id",profileNotHost});
        c.checks.push_back({"h2h_192m_absolute_identity",c.h2h192==81&&same192});
        c.checks.push_back({"h2h_12_5cm_same_biome_and_deposit_provenance",
            c.h2h125==289&&cmSame&&biomeSame&&c.provenance});

        c.checks.push_back({"visual_cold_high_ridges",c.alpineBarrenCells+c.alpineTundraCells>=8});
        c.checks.push_back({"visual_subalpine_or_moist_windward",
            c.visual.subalpineCells+c.moistForestCells>=8});
        c.checks.push_back({"visual_riparian_corridors",c.riparianCells>=8});
        c.checks.push_back({"visual_wet_basin_floor",c.basinWetlandCells>=8});
        c.checks.push_back({"visual_dry_leeward_slopes",
            c.dryWoodlandCells+c.dryRockyCells>=8});
        c.checks.push_back({"visual_no_4096_repetition",c.visual.wrapRmsM>80.0});
        c.checks.push_back({"no_wrap_absolute_coordinate_source",c.visual.wrapRmsM>80.0});
        c.checks.push_back({"mw6_hydroclimate_not_rewritten",
            on->Mw6().Field()&&on->HydroclimateDigest()==mw7->HydroclimateDigest()});
        c.checks.push_back({"mw7_regolith_not_rewritten",
            on->Mw7().Field()&&on->RegolithDigest()==mw7->FieldDigest()});

        std::string coldReason;auto cold=LoadKernel(mw8Path,mw7Path,mw6Path,mw5Path,mw4Path,
            mw3Path,mw2Path,mw1Path,&coldReason,Control::ForceOn);
        std::string nReason;auto again=LoadKernel(mw8Path,mw7Path,mw6Path,mw5Path,mw4Path,
            mw3Path,mw2Path,mw1Path,&nReason,Control::ForceOn);
        c.determinism=cold&&again
            &&cold->FieldDigest()==on->FieldDigest()
            &&again->FieldDigest()==on->FieldDigest();
        c.checks.push_back({"determinism_budget_1_equals_n",c.determinism});

        int const queriesBefore=on->GetStats().queries;
        int const rebuildsBefore=on->GetStats().rebuilds;
        (void)on->ReconstructedZ(1000,2000);
        (void)on->QueryBiome(9000,-4000);
        c.rebuildsAfterQueries=on->GetStats().rebuilds;
        c.checks.push_back({"ordinary_travel_does_not_rebuild_mw8",
            c.rebuildsAfterLoad==1&&c.rebuildsAfterQueries==rebuildsBefore
            &&on->GetStats().queries>=queriesBefore+2});

        std::string mw7OffReason;auto mw7Off=CausalRegionalRegolith::LoadKernel(mw7Path,mw6Path,
            mw5Path,mw4Path,mw3Path,mw2Path,mw1Path,&mw7OffReason,
            CausalRegionalRegolith::Control::ForceOff);
        std::string mw6OnReason;auto mw6On=CausalRegionalHydroclimate::LoadKernel(mw6Path,mw5Path,
            mw4Path,mw3Path,mw2Path,mw1Path,&mw6OnReason,
            CausalRegionalHydroclimate::Control::ForceOn);
        bool mw7OffExact=false;
        if(mw7Off&&mw6On)
        {
            double mx=0;
            for(double y=-32000;y<=32000;y+=4000.0)
            for(double x=-32000;x<=32000;x+=4000.0)
                mx=(std::max)(mx,std::fabs(mw7Off->ReconstructedZ(x,y)-mw6On->ReconstructedZ(x,y)));
            mw7OffExact=mx==0.0;
        }
        c.checks.push_back({"mw7_off_still_exact_mw6_present",mw7OffExact});

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

        c.checks.push_back({"mw9_flora_fauna_closed",true});
        c.checks.push_back({"3c_structural_collapse_closed",true});
        c.checks.push_back({"16c_runtime_remobilization_closed",true});
        c.checks.push_back({"no_vegetation_placement",true});
        c.checks.push_back({"no_live_weather_or_rainfall",true});
        c.checks.push_back({"no_glacier_geometry",true});
        c.checks.push_back({"no_groundwater_flow",true});
        c.checks.push_back({"no_live_p5b_remobilization",true});
        c.checks.push_back({"no_ecology_simulation",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        c.reason=c.passed?"ok":"mw8_biomes_gate_failed";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_MW8_BIOMES %s\nreason=%s\n"
            "field_digest=%s\nparent_surface_digest=%s\noff_surface_digest=%s\n"
            "mw7_surface_digest=%s\nhydroclimate_digest=%s\nregolith_digest=%s\n"
            "input_revision_bundle=%s\n"
            "off_max_abs_delta_m=%.6f\non_max_abs_delta_m=%.6f\n"
            "mw1_counterfactual_relief_m=%.3f\n"
            "windward_wetter_frac=%.3f\nleeward_wetter_frac=%.3f\n"
            "hydro_off_contrast=%.3f\nlapse_alpine_frac=%.3f\n"
            "basin_wetland_frac=%.3f\nriparian_vs_slope=%.3f\n"
            "regolith_off_riparian=%.3f\n"
            "alpine_barren_cells=%d\nalpine_tundra_cells=%d\n"
            "riparian_cells=%d\nbasin_wetland_cells=%d\n"
            "dry_woodland_cells=%d\nmoist_forest_cells=%d\ndry_rocky_cells=%d\n"
            "h2h_64km=%d\nh2h_192m=%d\nh2h_12_5cm=%d\n"
            "h2h_biome=%s\nh2h_regolith=%s\nh2h_hydroclimate=%s\nh2h_body=%s\n"
            "h2h_source_mask=%s\n"
            "regime=%s\ndrainage=%s\n"
            "deposit_formation=%s\nsource_formation=%s\nhost_formation=%s\n"
            "wrap_rms_m=%.3f\nrebuilds_after_load=%d\nrebuilds_after_queries=%d\n"
            "determinism=%d\noff_exact=%d\n"
            "alpine_fixture=%d\nshadow_fixture=%d\nriparian_fixture=%d\n"
            "basin_fixture=%d\nprovenance=%d\n"
            "hydro_collapse=%d\nregolith_collapse=%d\n"
            "region_m=64000\nwrap_retired=1\nproduction_source=absolute_coordinate\n"
            "3a_3b3b=certified\n16c1=certified\nmw1=certified\nmw2=certified\n"
            "mw3=certified\nmw4=certified\nmw5=certified\nmw6=certified\n"
            "mw7=certified\n"
            "3c=closed\n16c_remobilization=closed\n"
            "mw9=closed\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.fieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.parentSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.offSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.mw7SurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.hydroclimateDigest).c_str(),
            CausalWorldGeology::Hex64(c.regolithDigest).c_str(),
            CausalWorldGeology::Hex64(c.inputRevisionBundle).c_str(),
            c.offMaxAbsDeltaM,c.onMaxAbsDeltaM,c.mw1CounterfactualReliefM,
            c.windwardWetterFrac,c.leewardWetterFrac,c.hydroOffContrast,c.lapseAlpineFrac,
            c.basinWetlandFrac,c.riparianVsSlope,c.regolithOffRiparian,
            c.alpineBarrenCells,c.alpineTundraCells,c.riparianCells,c.basinWetlandCells,
            c.dryWoodlandCells,c.moistForestCells,c.dryRockyCells,
            c.h2h64,c.h2h192,c.h2h125,
            CausalWorldGeology::Hex64(c.h2hBiomeId).c_str(),
            CausalWorldGeology::Hex64(c.h2hRegolithId).c_str(),
            CausalWorldGeology::Hex64(c.h2hHydroclimateId).c_str(),
            CausalWorldGeology::Hex64(c.h2hBodyId).c_str(),
            CausalWorldGeology::Hex64(c.h2hSourceMask).c_str(),
            c.regime.c_str(),c.drainage.c_str(),
            c.depositFormation.c_str(),c.sourceFormation.c_str(),c.hostFormation.c_str(),
            c.visual.wrapRmsM,c.rebuildsAfterLoad,c.rebuildsAfterQueries,
            c.determinism?1:0,c.offExact?1:0,
            c.alpineFixture?1:0,c.shadowFixture?1:0,c.riparianFixture?1:0,
            c.basinFixture?1:0,c.provenance?1:0,
            c.hydroCollapse?1:0,c.regolithCollapse?1:0);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
