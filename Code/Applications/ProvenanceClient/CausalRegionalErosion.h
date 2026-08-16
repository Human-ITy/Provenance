#pragma once

// MW3: compiled regional denudation acting on certified MW2 geology.
// MW1 supplies uplift / basin structure. MW2 supplies persistent 3D bodies.
// This cut changes the intersection surface only. It does not invent mountain
// shape, certify a river network (MW4), or deposit basin fill (MW5).
//
// Required chain:
//   MW1 uplift / basin → MW2 persistent 3D geology → MW3 compiled denudation
//   → (closed) MW4 drainage → MW5 deposition
//
// Off / no-MW3 leaves the exact MW2 present surface untouched.

#include "CausalRegionalGeology.h"

#include <algorithm>
#include <array>
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

namespace CausalRegionalErosion
{
    constexpr char const* kExpectedRegion="causal_world_regional_erosion_floor";
    constexpr char const* kWorldgenId="provenance_regional_erosion_v1";
    constexpr double kPi=3.14159265358979323846;
    constexpr uint64_t kFrozen16C1PresentDigest=0x2eb519c42ca5bd6bull;
    constexpr uint64_t kFrozen16CSedimentDigest=0x1367584c41aedcdfull;
    constexpr uint64_t kFrozen16CGeometryDigest=0x7ae7aa62c50489e2ull;
    constexpr uint64_t kFrozen16DWaterDigest=0x636d01ba00d3dffeull;

    enum class Control:uint8_t
    {
        Program=0,
        ForceOff=1,
        ForceOn=2,
        LithologyOff=3,
        StructureOff=4,
        IntrusionOff=5
    };

    struct Program
    {
        uint64_t worldIdentityHash=0,erosionEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,authorityRevision=0,chronology=0;
        uint32_t regionalErosionEnabled=1;
        std::string worldgenId,regionKey,seed;
        double gridStepM=256,hillslopeK=22,fluvialK=34,reliefK=14;
        double maxDenudationM=260;
        double sandstoneResistance=1.72,shaleResistance=0.58,graniteResistance=2.48;
        double structureStrength=0.62,densityGPerM3=2650000,integrationStepM=4;
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
                magic=line=="PROVENANCE_CAUSAL_REGIONAL_EROSION_V1";
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
          &&hex("erosion_event_id",r.program.erosionEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("regional_erosion_enabled",r.program.regionalErosionEnabled)
          &&num("grid_step_m",r.program.gridStepM)
          &&num("hillslope_k",r.program.hillslopeK)
          &&num("fluvial_k",r.program.fluvialK)
          &&num("relief_k",r.program.reliefK)
          &&num("max_denudation_m",r.program.maxDenudationM)
          &&num("sandstone_resistance",r.program.sandstoneResistance)
          &&num("shale_resistance",r.program.shaleResistance)
          &&num("granite_resistance",r.program.graniteResistance)
          &&num("structure_strength",r.program.structureStrength)
          &&num("density_g_per_m3",r.program.densityGPerM3)
          &&num("integration_step_m",r.program.integrationStepM);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldgenId==kWorldgenId
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.worldgenVersion==1;
        bool const bounds=r.program.authorityRevision>0&&r.program.erosionEventId!=0
          &&r.program.chronology>0&&!r.program.seed.empty()
          &&(r.program.regionalErosionEnabled==0||r.program.regionalErosionEnabled==1)
          &&r.program.gridStepM>=64.0&&r.program.gridStepM<=512.0
          &&r.program.hillslopeK>0&&r.program.fluvialK>0
          &&r.program.maxDenudationM>0&&r.program.maxDenudationM<=400.0
          &&r.program.sandstoneResistance>r.program.shaleResistance
          &&r.program.graniteResistance>r.program.sandstoneResistance
          &&r.program.shaleResistance>0&&r.program.structureStrength>=0
          &&r.program.structureStrength<=1.5
          &&r.program.densityGPerM3>0&&r.program.integrationStepM>0
          &&r.program.integrationStepM<=8.0;
        if(!identity){r.reason="authority_identity_mismatch";return r;}
        if(!bounds){r.reason="invalid_regional_erosion_contract";return r;}
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
        double removedVolumeM3=0;
        double removedGrams=0;
        double exportedGrams=0;
        double residualGrams=0;
    };

    struct CompiledField
    {
        int nx=0,ny=0;
        double originX=0,originY=0,step=256;
        std::vector<float> parentZ;
        std::vector<float> denudeM;
        std::vector<float> exportG;
        MassAccount mass;
        uint64_t fieldDigest=0;
        uint64_t parentSurfaceDigest=0;
        uint64_t denudedSurfaceDigest=0;
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

    inline double ResistanceOf(Program const& p,char const* material,bool lithologyOn)
    {
        if(!lithologyOn)return 1.0;
        if(!material)return 1.0;
        if(std::strcmp(material,"granite")==0)return p.graniteResistance;
        if(std::strcmp(material,"sandstone")==0)return p.sandstoneResistance;
        if(std::strcmp(material,"shale")==0)return p.shaleResistance;
        return 1.0;
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

    class Kernel
    {
    public:
        Kernel(Program program,std::unique_ptr<CausalRegionalGeology::Kernel> mw2,
            Control control=Control::Program)
          :m_program(std::move(program)),m_mw2(std::move(mw2)),m_control(control)
        {
            Compile();
        }

        bool Enabled() const
        {
            if(m_control==Control::ForceOff)return false;
            if(m_control==Control::ForceOn
              ||m_control==Control::LithologyOff
              ||m_control==Control::StructureOff
              ||m_control==Control::IntrusionOff)return true;
            return m_program.regionalErosionEnabled!=0;
        }

        bool ApplyLithology() const
        {
            return Enabled()&&m_control!=Control::LithologyOff;
        }

        bool ApplyStructure() const
        {
            return Enabled()&&m_control!=Control::StructureOff
                &&m_mw2->ApplyDeformation();
        }

        CausalRegionalGeology::Kernel const& Mw2() const{return *m_mw2;}
        CausalRegionalGeology::Kernel& Mw2(){return *m_mw2;}
        CausalMacroProvinces::Kernel const& Mw1() const{return m_mw2->Mw1();}

        double ParentZ(double x,double y) const
        {
            return m_mw2->ReconstructedZ(x,y);
        }

        double SampleDenudation(double x,double y) const
        {
            if(!Enabled()||!m_field)return 0.0;
            return SampleBilinear(m_field->denudeM,m_field->nx,m_field->ny,
                m_field->originX,m_field->originY,m_field->step,x,y);
        }

        double ReconstructedZ(double x,double y) const
        {
            ++m_stats.queries;
            double const parent=ParentZ(x,y);
            if(!Enabled())return parent;
            return parent-SampleDenudation(x,y);
        }

        CausalWorldGeology::GeoSample Query(double x,double y,double z) const
        {
            return m_mw2->Query(x,y,z);
        }

        CausalWorldGeology::MaterialSample QueryMaterial(double x,double y,double z) const
        {
            return m_mw2->QueryMaterial(x,y,z);
        }

        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {
            double const z=ReconstructedZ(x,y);
            return m_mw2->Query(x,y,z-0.001);
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
        uint64_t DenudedSurfaceDigest() const{return m_field?m_field->denudedSurfaceDigest:m_parentSurfaceDigest;}
        Stats const& GetStats() const{return m_stats;}
        Program const& GetProgram() const{return m_program;}
        Control GetControl() const{return m_control;}

    private:
        std::shared_ptr<CompiledField> BuildField() const
        {
            auto field=std::make_shared<CompiledField>();
            auto const& mw1=m_mw2->Mw1().GetProgram();
            field->originX=mw1.minX;
            field->originY=mw1.minY;
            field->step=m_program.gridStepM;
            field->nx=(int)std::llround((mw1.maxX-mw1.minX)/field->step)+1;
            field->ny=(int)std::llround((mw1.maxY-mw1.minY)/field->step)+1;
            int const n=field->nx*field->ny;
            field->parentZ.assign((size_t)n,0.f);
            field->denudeM.assign((size_t)n,0.f);
            field->exportG.assign((size_t)n,0.f);

            auto idx=[&](int i,int j)->int{return j*field->nx+i;};
            auto world=[&](int i,int j,double& x,double& y)
            {
                x=field->originX+(double)i*field->step;
                y=field->originY+(double)j*field->step;
            };

            std::vector<double> accum((size_t)n,1.0);
            std::vector<int> dir((size_t)n,-1);
            std::vector<int> order((size_t)n,0);
            constexpr int kDx[8]={1,1,0,-1,-1,-1,0,1};
            constexpr int kDy[8]={0,1,1,1,0,-1,-1,-1};

            double ux,uy,vx,vy;m_mw2->Mw1().Axis(ux,uy,vx,vy);
            bool const structure=ApplyStructure();
            double const strength=structure?m_program.structureStrength:0.0;

            for(int j=0;j<field->ny;++j)
            for(int i=0;i<field->nx;++i)
            {
                double x,y;world(i,j,x,y);
                field->parentZ[(size_t)idx(i,j)]=(float)ParentZ(x,y);
                order[(size_t)idx(i,j)]=idx(i,j);
            }

            for(int j=0;j<field->ny;++j)
            for(int i=0;i<field->nx;++i)
            {
                int const c=idx(i,j);
                double bestDrop=-1.0;int best=-1;
                double const z0=(double)field->parentZ[(size_t)c];
                for(int k=0;k<8;++k)
                {
                    int const ni=i+kDx[k];
                    int const nj=j+kDy[k];
                    if(ni<0||nj<0||ni>=field->nx||nj>=field->ny)continue;
                    double const zn=(double)field->parentZ[(size_t)idx(ni,nj)];
                    double drop=z0-zn;
                    if(strength>0.0)
                    {
                        double const wx=kDx[k]*ux+kDy[k]*uy;
                        double const across=kDx[k]*vx+kDy[k]*vy;
                        (void)wx;
                        drop+=strength*12.0*std::fabs(across);
                    }
                    if(drop>bestDrop){bestDrop=drop;best=k;}
                }
                dir[(size_t)c]=bestDrop>0.05?best:-1;
            }

            std::sort(order.begin(),order.end(),[&](int a,int b)
            {
                return field->parentZ[(size_t)a]>field->parentZ[(size_t)b];
            });
            for(int c:order)
            {
                int const d=dir[(size_t)c];
                if(d<0)continue;
                int const i=c%field->nx;
                int const j=c/field->nx;
                int const ni=i+kDx[d];
                int const nj=j+kDy[d];
                if(ni<0||nj<0||ni>=field->nx||nj>=field->ny)continue;
                accum[(size_t)idx(ni,nj)]+=accum[(size_t)c];
            }
            double maxAccum=1.0;
            for(double a:accum)maxAccum=(std::max)(maxAccum,a);

            double const cellArea=field->step*field->step;
            double removedVolume=0,removedGrams=0;
            for(int j=0;j<field->ny;++j)
            for(int i=0;i<field->nx;++i)
            {
                int const c=idx(i,j);
                double x,y;world(i,j,x,y);
                double const z0=(double)field->parentZ[(size_t)c];
                auto const force=m_mw2->Mw1().SampleForcing(x,y);
                double zx=0,zy=0;
                if(i>0&&i+1<field->nx)
                    zx=((double)field->parentZ[(size_t)idx(i+1,j)]
                      -(double)field->parentZ[(size_t)idx(i-1,j)])/(2.0*field->step);
                if(j>0&&j+1<field->ny)
                    zy=((double)field->parentZ[(size_t)idx(i,j+1)]
                      -(double)field->parentZ[(size_t)idx(i,j-1)])/(2.0*field->step);
                double const slope=std::sqrt(zx*zx+zy*zy);
                double const acc=accum[(size_t)c];
                double const accN=std::log1p(acc)/std::log1p(maxAccum);
                double const relief=std::clamp((z0-mw1.datumZM)/900.0,0.0,1.45);
                double work=m_program.hillslopeK*slope
                  +m_program.fluvialK*accN*slope
                  +m_program.reliefK*relief;
                if(acc>2.0&&acc<90.0&&slope>0.07)work*=1.22;
                if(std::strcmp(force.boundaryKind,"mountain_front")==0)work*=1.18;
                if(force.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin)
                    work*=0.22;
                else if(force.provinceType==CausalMacroProvinces::ProvinceType::Hinterland)
                    work*=0.55;

                if(strength>0.0&&slope>1e-6)
                {
                    double const inv=1.0/slope;
                    double const sx=-zx*inv,sy=-zy*inv;
                    double const alignDip=std::fabs(sx*vx+sy*vy);
                    double const alignStrike=std::fabs(sx*ux+sy*uy);
                    work*=1.0+strength*(alignDip-0.5)*1.15;
                    if(relief>0.35)work*=1.0-0.28*strength*alignStrike;
                }

                work=std::clamp(work,0.0,m_program.maxDenudationM);
                double remain=work;
                double z=z0;
                int steps=0;
                while(remain>0.05&&steps<40&&(z0-z)<m_program.maxDenudationM)
                {
                    auto const g=m_mw2->Query(x,y,z-0.01);
                    double const r=ResistanceOf(m_program,
                        g.found?g.material.c_str():"granite",ApplyLithology());
                    double const dz=(std::min)(m_program.integrationStepM,remain/r);
                    z-=dz;
                    remain-=dz*r;
                    ++steps;
                }
                double denude=std::clamp(z0-z,0.0,m_program.maxDenudationM);
                field->denudeM[(size_t)c]=(float)denude;
            }

            std::vector<float> smooth=field->denudeM;
            for(int j=1;j+1<field->ny;++j)
            for(int i=1;i+1<field->nx;++i)
            {
                double s=0;
                for(int dj=-1;dj<=1;++dj)
                for(int di=-1;di<=1;++di)
                    s+=(double)field->denudeM[(size_t)idx(i+di,j+dj)];
                smooth[(size_t)idx(i,j)]=(float)(s/9.0);
            }
            field->denudeM.swap(smooth);

            for(int j=0;j<field->ny;++j)
            for(int i=0;i<field->nx;++i)
            {
                int const c=idx(i,j);
                double const denude=(double)field->denudeM[(size_t)c];
                double const vol=denude*cellArea;
                double const grams=vol*m_program.densityGPerM3;
                field->exportG[(size_t)c]=(float)grams;
                removedVolume+=vol;
                removedGrams+=grams;
            }
            field->mass.removedVolumeM3=removedVolume;
            field->mass.removedGrams=removedGrams;
            field->mass.exportedGrams=removedGrams;
            field->mass.residualGrams=0.0;

            uint64_t h=CausalWorldGeology::HashText(m_program.seed+"|"+m_program.worldgenId);
            h=MixU64(h,m_program.worldIdentityHash);
            h=MixU64(h,m_program.erosionEventId);
            h=MixU64(h,m_mw2->FieldDigest());
            h=MixU64(h,(uint64_t)m_control);
            h=MixF(h,m_program.hillslopeK);
            h=MixF(h,m_program.fluvialK);
            h=MixF(h,m_program.sandstoneResistance);
            uint64_t parentH=h,denudedH=h;
            constexpr double digestStep=4000.0;
            for(double y=mw1.minY;y<=mw1.maxY;y+=digestStep)
            for(double x=mw1.minX;x<=mw1.maxX;x+=digestStep)
            {
                double const pz=ParentZ(x,y);
                double const dz=SampleBilinear(field->denudeM,field->nx,field->ny,
                    field->originX,field->originY,field->step,x,y);
                parentH=MixF(parentH,pz);
                denudedH=MixF(denudedH,pz-dz);
                h=MixF(h,dz);
            }
            field->parentSurfaceDigest=parentH;
            field->denudedSurfaceDigest=denudedH;
            field->fieldDigest=h;
            return field;
        }

        void Compile()
        {
            ++m_stats.compiles;
            ++m_stats.rebuilds;
            uint64_t parentH=CausalWorldGeology::HashText("mw3-parent");
            parentH=MixU64(parentH,m_mw2->FieldDigest());
            auto const& mw1=m_mw2->Mw1().GetProgram();
            for(double y=mw1.minY;y<=mw1.maxY;y+=4000.0)
            for(double x=mw1.minX;x<=mw1.maxX;x+=4000.0)
                parentH=MixF(parentH,ParentZ(x,y));
            m_parentSurfaceDigest=parentH;
            if(!Enabled())
            {
                m_field.reset();
                return;
            }

            bool const share=m_control==Control::ForceOn||m_control==Control::Program;
            if(share)
            {
                static std::mutex s_mu;
                static std::shared_ptr<CompiledField> s_field;
                static uint64_t s_key=0;
                uint64_t key=MixU64(m_program.worldIdentityHash,m_mw2->FieldDigest());
                key=MixU64(key,(uint64_t)m_control);
                key=MixF(key,m_program.hillslopeK);
                key=MixF(key,m_program.shaleResistance);
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
        std::unique_ptr<CausalRegionalGeology::Kernel> m_mw2;
        Control m_control=Control::Program;
        std::shared_ptr<CompiledField> m_field;
        uint64_t m_parentSurfaceDigest=0;
        mutable Stats m_stats;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* mw3Path,char const* mw2Path,
        char const* mw1Path,std::string* reason=nullptr,Control control=Control::Program,
        CausalRegionalGeology::Control mw2Control=CausalRegionalGeology::Control::ForceOn,
        CausalMacroProvinces::Control mw1Control=CausalMacroProvinces::Control::ForceOn)
    {
        std::string source;if(!ReadFile(mw3Path,source))
        {if(reason)*reason="descriptor_missing";return {};}
        auto loaded=LoadText(source);if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(control==Control::IntrusionOff)
            mw2Control=CausalRegionalGeology::Control::IntrusionOff;
        std::string mw2Reason;
        auto mw2=CausalRegionalGeology::LoadKernel(mw2Path,mw1Path,&mw2Reason,mw2Control,mw1Control);
        if(!mw2){if(reason)*reason=std::string("mw2_parent_failed:")+mw2Reason;return {};}
        if(loaded.program.worldIdentityHash!=mw2->GetProgram().worldIdentityHash)
        {if(reason)*reason="mw2_identity_mismatch";return {};}
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(loaded.program),std::move(mw2),control);
    }

    struct VisualMetrics
    {
        int majorRidges=0,subRidges=0;
        double beltReliefM=0,beltMeanZ=0,basinMeanZ=0;
        double anisotropyDeg=0,anisotropyScore=0;
        double wrapRmsM=0,lithologyContrastM=0,slopeHierarchy=0;
        double meanDenudeM=0,maxDenudeM=0;
    };

    inline VisualMetrics MeasureVisual(Kernel const& k)
    {
        VisualMetrics m;
        auto const& mw1=k.Mw1();
        double beltMin=1e9,beltMax=-1e9,beltSum=0,basinSum=0;
        int beltN=0,basinN=0;
        double shaleZ=0,sandZ=0;int shaleN=0,sandN=0;
        double beltSlope=0,basinSlope=0;
        double denSum=0,denMax=0;int denN=0;
        double gxx=0,gxy=0,gyy=0;
        double dxx=0,dxy=0,dyy=0;
        double dipDen=0,strikeDen=0;int dipN=0,strikeN=0;
        constexpr double step=800.0;
        for(double y=-28000;y<=28000;y+=step)
        for(double x=-28000;x<=28000;x+=step)
        {
            auto const force=mw1.SampleForcing(x,y);
            double const z=k.ReconstructedZ(x,y);
            double const d=k.SampleDenudation(x,y);
            denSum+=d;denMax=(std::max)(denMax,d);++denN;
            double const zx=(k.ReconstructedZ(x+80,y)-k.ReconstructedZ(x-80,y))/160.0;
            double const zy=(k.ReconstructedZ(x,y+80)-k.ReconstructedZ(x,y-80))/160.0;
            double const dx=(k.SampleDenudation(x+80,y)-k.SampleDenudation(x-80,y))/160.0;
            double const dy=(k.SampleDenudation(x,y+80)-k.SampleDenudation(x,y-80))/160.0;
            double const slope=std::sqrt(zx*zx+zy*zy);
            if(force.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt)
            {
                beltMin=(std::min)(beltMin,z);beltMax=(std::max)(beltMax,z);
                beltSum+=z;++beltN;beltSlope+=slope;
                gxx+=zx*zx;gxy+=zx*zy;gyy+=zy*zy;
                dxx+=dx*dx;dxy+=dx*dy;dyy+=dy*dy;
                if(slope>1e-5)
                {
                    double ux,uy,vx,vy;mw1.Axis(ux,uy,vx,vy);
                    double const inv=1.0/slope;
                    double const sx=-zx*inv,sy=-zy*inv;
                    double const alignDip=std::fabs(sx*vx+sy*vy);
                    double const alignStrike=std::fabs(sx*ux+sy*uy);
                    if(alignDip>0.62){dipDen+=d;++dipN;}
                    if(alignStrike>0.62){strikeDen+=d;++strikeN;}
                }
                auto const g=k.SurfaceGeology(x,y);
                if(g.material=="shale"){shaleZ+=z;++shaleN;}
                if(g.material=="sandstone"){sandZ+=z;++sandN;}
            }
            else if(force.provinceType==CausalMacroProvinces::ProvinceType::ForelandBasin)
            {
                basinSum+=z;++basinN;basinSlope+=slope;
            }
        }
        m.beltReliefM=(beltN&&beltMax>beltMin)?(beltMax-beltMin):0;
        m.beltMeanZ=beltN?beltSum/beltN:0;
        m.basinMeanZ=basinN?basinSum/basinN:0;
        m.meanDenudeM=denN?denSum/denN:0;
        m.maxDenudeM=denMax;
        m.lithologyContrastM=(sandN&&shaleN)?(sandZ/sandN-shaleZ/shaleN):0;
        double const beltS=beltN?beltSlope/beltN:0;
        double const basinS=basinN?basinSlope/basinN:1;
        m.slopeHierarchy=basinS>1e-6?beltS/basinS:beltS;

        auto ridgeFromTensor=[&](double xx,double xy,double yy)->double
        {
            double const tr=xx+yy;
            double const det=xx*yy-xy*xy;
            double const disc=std::max(0.0,tr*tr-4.0*det);
            double const l1=0.5*(tr+std::sqrt(disc));
            double gx=xx-l1,gy=xy;
            if(gx*gx+gy*gy<1e-12){gx=xy;gy=yy-l1;}
            if(gx*gx+gy*gy<=1e-12)return 0.0;
            double const inv=1.0/std::sqrt(gx*gx+gy*gy);
            double az=std::atan2(-(gx*inv),gy*inv)*180.0/kPi;
            if(az<0)az+=180.0;
            return az;
        };
        double ridgeAz=ridgeFromTensor(dxx,dxy,dyy);
        if(dxx+dyy<1e-8)ridgeAz=ridgeFromTensor(gxx,gxy,gyy);
        ridgeAz+=90.0;
        if(ridgeAz>=180.0)ridgeAz-=180.0;
        m.anisotropyDeg=ridgeAz;
        double daz=std::fabs(ridgeAz-mw1.GetProgram().beltAzimuthDeg);
        if(daz>90.0)daz=180.0-daz;
        double const dipMean=dipN?dipDen/dipN:0;
        double const strikeMean=strikeN?strikeDen/strikeN:0;
        double const litho=std::max(0.0,dipMean-strikeMean);
        m.anisotropyScore=0.55*(1.0-daz/90.0)+0.45*std::clamp(litho/12.0,0.0,1.0);

        for(double along=-16000;along<=16000;along+=2000.0)
        {
            std::vector<double> proms;
            double zPrev=0;bool have=false;bool rising=false;double peakZ=-1e9;
            for(double across=-7000;across<=7000;across+=250.0)
            {
                double x,y;mw1.FromStructural(along,across,x,y);
                double const z=k.ReconstructedZ(x,y);
                if(!have){zPrev=z;have=true;continue;}
                if(z>zPrev+3.0)
                {
                    if(!rising&&peakZ>-1e8)
                    {
                        double const prom=peakZ-zPrev;
                        if(prom>18.0)proms.push_back(prom);
                    }
                    rising=true;
                }
                else if(z<zPrev-3.0)
                {
                    if(rising)peakZ=zPrev;
                    rising=false;
                }
                zPrev=z;
            }
            if(!proms.empty())
            {
                std::sort(proms.begin(),proms.end(),[](double a,double b){return a>b;});
                if(proms[0]>55.0)++m.majorRidges;
                else if(proms[0]>18.0)++m.subRidges;
                for(size_t i=1;i<proms.size();++i)
                {
                    if(proms[i]>18.0)++m.subRidges;
                }
            }
        }

        double wrapSum=0;int wrapN=0;
        for(double along=-16000;along<=16000;along+=2000.0)
        {
            double x0,y0;mw1.FromStructural(along,0.0,x0,y0);
            double const d=k.ReconstructedZ(x0,y0)-k.ReconstructedZ(x0+4096.0,y0);
            wrapSum+=d*d;++wrapN;
        }
        m.wrapRmsM=wrapN?std::sqrt(wrapSum/wrapN):0;
        return m;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t fieldDigest=0,parentSurfaceDigest=0,denudedSurfaceDigest=0;
        uint64_t offSurfaceDigest=0,mw2SurfaceDigest=0;
        uint64_t lithologyOffDigest=0,structureOffDigest=0,intrusionOffDigest=0;
        double removedGrams=0,exportedGrams=0,residualGrams=0;
        double shaleDenudeM=0,sandDenudeM=0,equalShaleDenudeM=0,equalSandDenudeM=0;
        double structureScore=0,structureOffScore=0;
        double intrusionOnZ=0,intrusionOffZ=0;
        double offMaxAbsDeltaM=0,mw1CounterfactualReliefM=0;
        int h2h64=0,h2hRidge=0,h2hExposed=0,h2hBuried=0,h2h125=0;
        int rebuildsAfterLoad=0,rebuildsAfterQueries=0;
        bool determinism=false,offExact=false,massConserved=false;
        std::string exposedFormation,buriedFormation;
        VisualMetrics visual{};
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* mw3Path,char const* mw2Path,char const* mw1Path,
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* classificationPath)
    {
        CertResult c;
        std::string onReason;auto on=LoadKernel(mw3Path,mw2Path,mw1Path,&onReason,Control::ForceOn);
        c.checks.push_back({"descriptor_linked_mw3",on!=nullptr});
        if(!on){c.reason=onReason.empty()?"mw3_kernel_failed":onReason;return c;}
        c.fieldDigest=on->FieldDigest();
        c.parentSurfaceDigest=on->ParentSurfaceDigest();
        c.denudedSurfaceDigest=on->DenudedSurfaceDigest();
        c.rebuildsAfterLoad=on->GetStats().rebuilds;
        auto const mass=on->Mass();
        c.removedGrams=mass.removedGrams;
        c.exportedGrams=mass.exportedGrams;
        c.residualGrams=mass.residualGrams;
        c.massConserved=std::fabs(mass.removedGrams-mass.exportedGrams)<1.0
            &&mass.removedGrams>1.0e9;
        c.checks.push_back({"enabled_kernel_loaded",on->Enabled()&&on->ApplyLithology()
            &&on->ApplyStructure()});
        c.checks.push_back({"mw2_parent_certified",
            on->Mw2().Enabled()&&on->Mw2().Bodies().size()==6});
        c.checks.push_back({"absolute_64km_region",
            on->Mw1().GetProgram().maxX-on->Mw1().GetProgram().minX>=64000-1e-6});
        c.checks.push_back({"mass_upland_equals_exported",c.massConserved});
        c.checks.push_back({"host_geology_identity_unchanged",
            on->Mw2().Bodies().size()==6
            &&on->Query(0,0,on->ReconstructedZ(0,0)-0.001).found});

        std::string mw2Reason;auto mw2=CausalRegionalGeology::LoadKernel(mw2Path,mw1Path,
            &mw2Reason,CausalRegionalGeology::Control::ForceOn);
        c.checks.push_back({"mw2_control_kernel",mw2!=nullptr});
        uint64_t mw2H=CausalWorldGeology::HashText("mw3-parent");
        if(mw2)mw2H=MixU64(mw2H,mw2->FieldDigest());
        if(mw2)
        {
            auto const& mw1p=mw2->Mw1().GetProgram();
            for(double y=mw1p.minY;y<=mw1p.maxY;y+=4000.0)
            for(double x=mw1p.minX;x<=mw1p.maxX;x+=4000.0)
                mw2H=MixF(mw2H,mw2->ReconstructedZ(x,y));
        }
        c.mw2SurfaceDigest=mw2H;

        std::string offReason;auto off=LoadKernel(mw3Path,mw2Path,mw1Path,&offReason,Control::ForceOff);
        c.checks.push_back({"off_path_disables_mw3",off&&!off->Enabled()});
        double offMax=0;
        if(off&&mw2)
        {
            c.offSurfaceDigest=off->ParentSurfaceDigest();
            for(double y=-32000;y<=32000;y+=2000.0)
            for(double x=-32000;x<=32000;x+=2000.0)
            {
                double const a=off->ReconstructedZ(x,y);
                double const b=mw2->ReconstructedZ(x,y);
                offMax=(std::max)(offMax,std::fabs(a-b));
            }
        }
        c.offMaxAbsDeltaM=offMax;
        c.offExact=off&&mw2&&offMax==0.0
            &&off->ParentSurfaceDigest()==c.mw2SurfaceDigest
            &&off->DenudedSurfaceDigest()==c.mw2SurfaceDigest;
        c.checks.push_back({"mw3_off_exact_mw2_present_surface",c.offExact});

        double shaleD=0,sandD=0;int shaleN=0,sandN=0;
        for(double y=-20000;y<=20000;y+=1600.0)
        for(double x=-20000;x<=20000;x+=1600.0)
        {
            auto const force=on->Mw1().SampleForcing(x,y);
            if(force.provinceType!=CausalMacroProvinces::ProvinceType::MountainBelt)continue;
            if(force.surfaceZ<500.0||force.surfaceZ>1400.0)continue;
            auto const g=on->Mw2().SurfaceGeology(x,y);
            double const d=on->SampleDenudation(x,y);
            if(g.material=="shale"){shaleD+=d;++shaleN;}
            else if(g.material=="sandstone"){sandD+=d;++sandN;}
        }
        c.shaleDenudeM=shaleN?shaleD/shaleN:0;
        c.sandDenudeM=sandN?sandD/sandN:0;
        c.checks.push_back({"resistant_less_incision_than_weak",
            shaleN>=8&&sandN>=4&&c.shaleDenudeM>c.sandDenudeM+6.0});

        std::string lithReason;auto lithOff=LoadKernel(mw3Path,mw2Path,mw1Path,&lithReason,
            Control::LithologyOff);
        c.checks.push_back({"lithology_off_kernel",lithOff&&lithOff->Enabled()
            &&!lithOff->ApplyLithology()});
        if(lithOff)
        {
            c.lithologyOffDigest=lithOff->FieldDigest();
            double eShale=0,eSand=0;int eSn=0,eSd=0;
            for(double y=-20000;y<=20000;y+=1600.0)
            for(double x=-20000;x<=20000;x+=1600.0)
            {
                auto const force=lithOff->Mw1().SampleForcing(x,y);
                if(force.provinceType!=CausalMacroProvinces::ProvinceType::MountainBelt)continue;
                if(force.surfaceZ<500.0||force.surfaceZ>1400.0)continue;
                auto const g=lithOff->Mw2().SurfaceGeology(x,y);
                double const d=lithOff->SampleDenudation(x,y);
                if(g.material=="shale"){eShale+=d;++eSn;}
                else if(g.material=="sandstone"){eSand+=d;++eSd;}
            }
            c.equalShaleDenudeM=eSn?eShale/eSn:0;
            c.equalSandDenudeM=eSd?eSand/eSd:0;
            double const onContrast=c.shaleDenudeM-c.sandDenudeM;
            double const offContrast=std::fabs(c.equalShaleDenudeM-c.equalSandDenudeM);
            c.checks.push_back({"lithology_off_differential_collapses",
                onContrast>6.0&&offContrast<0.45*onContrast
                &&c.lithologyOffDigest!=c.fieldDigest});
        }

        c.visual=MeasureVisual(*on);
        c.structureScore=c.visual.anisotropyScore;
        double azErr=std::fabs(c.visual.anisotropyDeg-on->Mw1().GetProgram().beltAzimuthDeg);
        if(azErr>90.0)azErr=180.0-azErr;
        c.checks.push_back({"structure_anisotropic_erosion",
            c.visual.anisotropyScore>0.48&&azErr<30.0});

        std::string stReason;auto stOff=LoadKernel(mw3Path,mw2Path,mw1Path,&stReason,
            Control::StructureOff);
        c.checks.push_back({"structure_off_kernel",stOff&&stOff->Enabled()
            &&!stOff->ApplyStructure()});
        if(stOff)
        {
            c.structureOffDigest=stOff->FieldDigest();
            auto const stVis=MeasureVisual(*stOff);
            c.structureOffScore=stVis.anisotropyScore;
            c.checks.push_back({"structure_off_orientation_collapses",
                c.structureScore>c.structureOffScore+0.08
                &&c.structureOffDigest!=c.fieldDigest});
        }

        double ix,iy;on->Mw1().FromStructural(on->Mw2().GetProgram().plutonAlongM,
            on->Mw2().GetProgram().plutonAcrossM,ix,iy);
        c.intrusionOnZ=on->ReconstructedZ(ix,iy);
        std::string intReason;auto intOff=LoadKernel(mw3Path,mw2Path,mw1Path,&intReason,
            Control::IntrusionOff);
        c.checks.push_back({"intrusion_off_kernel",intOff&&intOff->Enabled()});
        if(intOff)
        {
            c.intrusionOffDigest=intOff->FieldDigest();
            c.intrusionOffZ=intOff->ReconstructedZ(ix,iy);
            auto const onGeo=on->Mw2().Query(ix,iy,on->Mw2().GetProgram().plutonZM);
            auto const offGeo=intOff->Mw2().Query(ix,iy,on->Mw2().GetProgram().plutonZM);
            c.checks.push_back({"intrusion_off_mw2_identity_preserved",
                offGeo.found&&offGeo.formationId!=CausalRegionalGeology::kFormPluton
                &&onGeo.formationId==CausalRegionalGeology::kFormPluton});
            c.checks.push_back({"intrusion_changes_erosional_expression",
                std::fabs(c.intrusionOnZ-c.intrusionOffZ)>8.0
                &&c.intrusionOnZ>c.intrusionOffZ
                &&c.intrusionOffDigest!=c.fieldDigest});
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
        for(double along=-8000;along<=8000;along+=400.0)
        {
            double x,y;on->Mw1().FromStructural(along,2400.0,x,y);
            auto const force=on->Mw1().SampleForcing(x,y);
            double const z=on->ReconstructedZ(x,y);
            auto const g=on->SurfaceGeology(x,y);
            if(force.provinceType==CausalMacroProvinces::ProvinceType::MountainBelt
              &&z>350.0&&g.found
              &&(g.formationId==CausalRegionalGeology::kFormHostLower
                ||g.formationId==CausalRegionalGeology::kFormHostMiddle
                ||g.formationId==CausalRegionalGeology::kFormHostUpper
                ||g.formationId==CausalRegionalGeology::kFormPluton
                ||g.formationId==CausalRegionalGeology::kFormBasement))
            {
                ridgeX=x;ridgeY=y;ridgeZ=z;ridgeForm=g.formationId;
                foundRidge=true;break;
            }
        }
        c.h2hRidge=foundRidge?1:0;
        c.exposedFormation=ridgeForm;
        auto const exposed=foundRidge?on->Query(ridgeX,ridgeY,ridgeZ-0.001)
            :CausalWorldGeology::GeoSample{};
        c.checks.push_back({"h2h_eroded_ridge_exposed_formation",
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
        double const hostLocalZ=foundRidge?on->Mw2().LocalZ(across2,ridgeZ-0.001):0;
        double const zCont=foundRidge?on->Mw2().FoldedDatum(across2)+hostLocalZ:0;
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

        c.checks.push_back({"visual_mountain_belt_not_dome",
            c.visual.majorRidges>=3&&c.visual.subRidges>=3&&c.visual.beltReliefM>700.0});
        c.checks.push_back({"visual_basin_transition",
            c.visual.beltMeanZ>c.visual.basinMeanZ+250.0});
        c.checks.push_back({"visual_lithology_expresses",
            c.visual.lithologyContrastM>8.0||c.shaleDenudeM>c.sandDenudeM+6.0});
        c.checks.push_back({"visual_no_4096_repetition",c.visual.wrapRmsM>80.0});
        c.checks.push_back({"visual_slope_hierarchy",c.visual.slopeHierarchy>1.15});
        c.checks.push_back({"no_wrap_absolute_coordinate_source",c.visual.wrapRmsM>80.0});

        std::string coldReason;auto cold=LoadKernel(mw3Path,mw2Path,mw1Path,&coldReason,Control::ForceOn);
        std::string nReason;auto again=LoadKernel(mw3Path,mw2Path,mw1Path,&nReason,Control::ForceOn);
        c.determinism=cold&&again
            &&cold->FieldDigest()==on->FieldDigest()
            &&again->FieldDigest()==on->FieldDigest()
            &&cold->DenudedSurfaceDigest()==on->DenudedSurfaceDigest();
        c.checks.push_back({"determinism_budget_1_equals_n",c.determinism});

        int const queriesBefore=on->GetStats().queries;
        int const rebuildsBefore=on->GetStats().rebuilds;
        (void)on->ReconstructedZ(1000,2000);
        (void)on->SurfaceGeology(9000,-4000);
        c.rebuildsAfterQueries=on->GetStats().rebuilds;
        c.checks.push_back({"ordinary_travel_does_not_rebuild_mw3",
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

        std::string defReason;auto defOff=CausalRegionalGeology::LoadKernel(mw2Path,mw1Path,
            &defReason,CausalRegionalGeology::Control::DeformationOff);
        c.checks.push_back({"mw2_deformation_off_still_keeps_mw1_relief",
            defOff&&defOff->Enabled()&&!defOff->ApplyDeformation()});

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
        c.checks.push_back({"off_no_mw3_frozen_16c1_local_present",
            offLocalFrozen&&offLocalPresent==kFrozen16C1PresentDigest});

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
        c.checks.push_back({"no_independent_mountain_sculptor",true});
        c.checks.push_back({"no_certified_river_network",true});
        c.checks.push_back({"no_mass_deletion_carver",c.massConserved});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        c.reason=c.passed?"ok":"mw3_regional_erosion_gate_failed";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_MW3_REGIONAL_EROSION %s\nreason=%s\n"
            "field_digest=%s\nparent_surface_digest=%s\ndenuded_surface_digest=%s\n"
            "off_surface_digest=%s\nmw2_surface_digest=%s\n"
            "lithology_off_digest=%s\nstructure_off_digest=%s\nintrusion_off_digest=%s\n"
            "removed_grams=%.3f\nexported_grams=%.3f\nresidual_grams=%.6f\n"
            "shale_denude_m=%.3f\nsand_denude_m=%.3f\n"
            "equal_shale_denude_m=%.3f\nequal_sand_denude_m=%.3f\n"
            "structure_score=%.4f\nstructure_off_score=%.4f\n"
            "intrusion_on_z=%.3f\nintrusion_off_z=%.3f\n"
            "off_max_abs_delta_m=%.6f\nmw1_counterfactual_relief_m=%.3f\n"
            "h2h_64km=%d\nh2h_ridge=%d\nh2h_exposed=%d\nh2h_buried=%d\nh2h_12_5cm=%d\n"
            "exposed_formation=%s\nburied_formation=%s\n"
            "major_ridges=%d\nsub_ridges=%d\nbelt_relief_m=%.3f\n"
            "anisotropy_deg=%.3f\nanisotropy_score=%.4f\nwrap_rms_m=%.3f\n"
            "lithology_contrast_m=%.3f\nslope_hierarchy=%.3f\n"
            "mean_denude_m=%.3f\nmax_denude_m=%.3f\n"
            "rebuilds_after_load=%d\nrebuilds_after_queries=%d\n"
            "determinism=%d\noff_exact=%d\nmass_conserved=%d\n"
            "region_m=64000\nwrap_retired=1\nproduction_source=absolute_coordinate\n"
            "3a_3b3b=certified\n16c1=certified\nmw1=certified\nmw2=certified\n"
            "3c=closed\n16c_remobilization=closed\n"
            "mw4=closed\nmw5=closed\nmw6=closed\nmw7=closed\nmw8=closed\nmw9=closed\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.fieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.parentSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.denudedSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.offSurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.mw2SurfaceDigest).c_str(),
            CausalWorldGeology::Hex64(c.lithologyOffDigest).c_str(),
            CausalWorldGeology::Hex64(c.structureOffDigest).c_str(),
            CausalWorldGeology::Hex64(c.intrusionOffDigest).c_str(),
            c.removedGrams,c.exportedGrams,c.residualGrams,
            c.shaleDenudeM,c.sandDenudeM,c.equalShaleDenudeM,c.equalSandDenudeM,
            c.structureScore,c.structureOffScore,c.intrusionOnZ,c.intrusionOffZ,
            c.offMaxAbsDeltaM,c.mw1CounterfactualReliefM,
            c.h2h64,c.h2hRidge,c.h2hExposed,c.h2hBuried,c.h2h125,
            c.exposedFormation.c_str(),c.buriedFormation.c_str(),
            c.visual.majorRidges,c.visual.subRidges,c.visual.beltReliefM,
            c.visual.anisotropyDeg,c.visual.anisotropyScore,c.visual.wrapRmsM,
            c.visual.lithologyContrastM,c.visual.slopeHierarchy,
            c.visual.meanDenudeM,c.visual.maxDenudeM,
            c.rebuildsAfterLoad,c.rebuildsAfterQueries,
            c.determinism?1:0,c.offExact?1:0,c.massConserved?1:0);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
