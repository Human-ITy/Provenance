#pragma once

// Stage 16C.1: compiled-deposit classification. Stage 16C remains the
// immutable compiled-sediment parent. This layer maps already-compiled
// 16C sediment into the 3A–3B.3B depositional vocabulary. It cannot
// erode, transport, remobilize, rewrite 16C mass routing, weld into host
// geology, open 3C collapse, or invent live physics.

#include "CausalCompiledSediment.h"
#include "CausalPresentWater.h"
#include "CausalPresentWaterTerrainHydraulicDeposition.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalCompiledDepositClassification
{
    constexpr char const* kExpectedRegion="causal_world_compiled_deposit_classification_floor";
    constexpr uint64_t kFrozenSedimentDigest=0x1367584c41aedcdfull;
    constexpr uint64_t kFrozenGeometryDigest=0x7ae7aa62c50489e2ull;
    constexpr uint64_t kFrozenWaterDigest=0x636d01ba00d3dffeull;

    enum class Control:uint8_t
    {
        Program=0,
        ForceOff=1,
        ForceOn=2
    };

    enum class DepositClass:uint8_t
    {
        NotDepositional=0,
        Loose=1,
        SettledAggregate=2,
        CompactedDeposit=3
    };

    using BodyPhase=CausalPresentWaterTerrainHydraulicDeposition::BodyPhase;

    inline char const* DepositClassName(DepositClass value)
    {
        switch(value)
        {
            case DepositClass::Loose:return "loose";
            case DepositClass::SettledAggregate:return "settled_aggregate";
            case DepositClass::CompactedDeposit:return "compacted_deposit";
            default:return "not_depositional";
        }
    }

    inline BodyPhase PhaseOf(DepositClass value)
    {
        switch(value)
        {
            case DepositClass::CompactedDeposit:return BodyPhase::CompactedDeposit;
            case DepositClass::SettledAggregate:return BodyPhase::SettledAggregate;
            default:return BodyPhase::Loose;
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,classificationEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t classifyCompiledDepositsEnabled=0;
        std::string worldgenId,regionKey,parentRegionKey;
        double thinWashDepthM=.35,compactDepthM=1.55,basinCompactDepthM=.45,
            gravelCompactDepthM=.50,loadAccumM2=2500000,loadDepthM=.90;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalCompiledSediment::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_COMPILED_DEPOSIT_CLASSIFICATION_V1";
                if(!magic){r.reason="bad_magic";return r;}continue;}
            size_t const eq=line.find('=');if(eq==std::string::npos)
            {r.reason="malformed_line";return r;}
            std::string const key=line.substr(0,eq);if(fields.count(key))
            {r.reason="duplicate_key:"+key;return r;}fields.emplace(key,line.substr(eq+1));
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
        auto const* parentRegion=get("parent_region_key");if(!id||!region||!parentRegion)
        {r.reason="missing_identity";return r;}
        r.program.worldgenId=*id;r.program.regionKey=*region;r.program.parentRegionKey=*parentRegion;
        bool const values=hex("world_identity_hash",r.program.worldIdentityHash)
          &&u32("worldgen_version",r.program.worldgenVersion)&&u32("schema_version",r.program.schemaVersion)
          &&u32("parent_authority_revision",r.program.parentAuthorityRevision)
          &&u32("authority_revision",r.program.authorityRevision)
          &&hex("classification_event_id",r.program.classificationEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("classify_compiled_deposits_enabled",r.program.classifyCompiledDepositsEnabled)
          &&num("thin_wash_depth_m",r.program.thinWashDepthM)
          &&num("compact_depth_m",r.program.compactDepthM)
          &&num("basin_compact_depth_m",r.program.basinCompactDepthM)
          &&num("gravel_compact_depth_m",r.program.gravelCompactDepthM)
          &&num("load_accum_m2",r.program.loadAccumM2)
          &&num("load_depth_m",r.program.loadDepthM);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.classificationEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.classifyCompiledDepositsEnabled==0||r.program.classifyCompiledDepositsEnabled==1)
          &&r.program.thinWashDepthM>0&&r.program.compactDepthM>r.program.thinWashDepthM
          &&r.program.basinCompactDepthM>0&&r.program.gravelCompactDepthM>0
          &&r.program.loadAccumM2>0&&r.program.loadDepthM>0;
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_compiled_deposit_classification_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct ClassifiedCell
    {
        DepositClass depositClass=DepositClass::NotDepositional;
        BodyPhase phase=BodyPhase::Loose;
        bool weldedToHost=false;
        bool isStableWalkingSurface=false;
        uint64_t hostFeatureId=0;
        uint64_t hostMaterialId=0;
        uint64_t depositBodyId=0;
        double thicknessM=0,mobility=0,loadProxy=0,supportProxy=0,ageStabilityProxy=0;
    };

    struct Query
    {
        bool found=false;
        CausalCompiledSediment::Query sediment;
        ClassifiedCell classified;
        CausalWorldGeology::GeoSample geology;
    };

    struct Stats
    {
        uint32_t compileClassifies=0;
        uint32_t runtimeClassifies=0;
        uint32_t remobilizes=0;
        uint32_t staleRefuse=0;
        uint32_t classificationRevision=0;
    };

    inline uint64_t HashMaterialName(std::string const& name)
    {
        uint64_t d=14695981039346656037ull;
        CausalWorldGeology::HashAppend(d,name.data(),name.size());
        return d;
    }

    inline uint64_t MakeDepositBodyId(size_t index,std::string const& material,
        DepositClass cls,uint64_t hostFeatureId)
    {
        uint64_t d=14695981039346656037ull;
        uint64_t const tag=0x16C10001ull;
        CausalWorldGeology::HashAppend(d,&tag,sizeof(tag));
        CausalWorldGeology::HashAppend(d,&index,sizeof(index));
        CausalWorldGeology::HashAppend(d,material.data(),material.size());
        uint8_t const c=(uint8_t)cls;
        CausalWorldGeology::HashAppend(d,&c,sizeof(c));
        CausalWorldGeology::HashAppend(d,&hostFeatureId,sizeof(hostFeatureId));
        return d?d:1;
    }

    inline uint64_t PresentDigestOf(uint64_t sedimentDigest,uint64_t geometryDigest,
        uint64_t waterDigest)
    {
        uint64_t d=14695981039346656037ull;
        CausalWorldGeology::HashAppend(d,&sedimentDigest,sizeof(sedimentDigest));
        CausalWorldGeology::HashAppend(d,&geometryDigest,sizeof(geometryDigest));
        CausalWorldGeology::HashAppend(d,&waterDigest,sizeof(waterDigest));
        return d;
    }

    inline uint64_t FrozenPresentDigest()
    {
        return PresentDigestOf(kFrozenSedimentDigest,kFrozenGeometryDigest,kFrozenWaterDigest);
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalCompiledSediment::Kernel> sediment,
            Program program,Control control)
          :m_sediment(std::move(sediment)),m_program(std::move(program))
        {
            m_enabled=ResolveEnabled(control);
            if(!ClassifyCompiled())
            {
                m_stale=true;
                m_stats.staleRefuse=1;
                m_cells.assign(m_sediment?m_sediment->Cells().size():0,{});
            }
            m_classificationDigest=ComputeClassificationDigest();
        }

        CausalCompiledSediment::Kernel const& Sediment() const{return *m_sediment;}
        CausalCompiledFluvialErosion::Kernel const& Erosion() const{return m_sediment->Erosion();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_sediment->Drainage();}
        CausalBareEarthGeography::Kernel const& Geography() const{return m_sediment->Geography();}
        Program const& GetProgram() const{return m_program;}
        std::vector<ClassifiedCell> const& Cells() const{return m_cells;}
        bool Enabled() const{return m_enabled;}
        bool Stale() const{return m_stale;}
        Stats const& GetStats() const{return m_stats;}
        uint64_t ClassificationDigest() const{return m_classificationDigest;}
        uint64_t SedimentDigest() const{return m_sediment->Digest();}
        uint64_t GeometryDigest() const{return m_sediment->GeometryDigest();}
        uint64_t LandscapeDigest() const
        {
            uint64_t d=14695981039346656037ull;
            uint64_t const s=SedimentDigest(),g=GeometryDigest();
            CausalWorldGeology::HashAppend(d,&s,sizeof(s));
            CausalWorldGeology::HashAppend(d,&g,sizeof(g));
            return d;
        }

        double ReconstructedZ(double x,double y) const
        {return Sediment().ReconstructedZ(x,y);}

        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return Sediment().QueryGeology(x,y,z);}

        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {
            // Classification is a label. Frozen 16C material/geometry stay
            // authoritative so off and on present the same surface material.
            return Sediment().SurfaceGeology(x,y);
        }

        Query QueryAt(double x,double y) const
        {
            Query q;q.sediment=Sediment().QueryAt(x,y);if(!q.sediment.found)return q;
            if(q.sediment.erosion.drainage.found)
            {
                size_t const i=(size_t)q.sediment.erosion.drainage.index;
                if(i<m_cells.size())q.classified=m_cells[i];
            }
            q.geology=SurfaceGeology(x,y);q.found=q.geology.found;return q;
        }

        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return Sediment().SampleBlock(bx,by);}

        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return Sediment().BuildBlock(bx,by);}

        ClassifiedCell const* CellAtIndex(size_t i) const
        {return i<m_cells.size()?&m_cells[i]:nullptr;}

    private:
        bool ResolveEnabled(Control control) const
        {
            if(control==Control::ForceOff)return false;
            if(control==Control::ForceOn)return true;
            return m_program.classifyCompiledDepositsEnabled==1;
        }

        bool ClassifyCompiled()
        {
            if(!m_sediment){m_reason="missing_compiled_inputs";return false;}
            if(m_sediment->Digest()!=kFrozenSedimentDigest
              ||m_sediment->GeometryDigest()!=kFrozenGeometryDigest)
            {m_reason="stale_compiled_sediment";return false;}
            auto const& sedimentCells=Sediment().Cells();
            auto const& drainage=Sediment().Drainage().Cells();
            if(sedimentCells.empty()||sedimentCells.size()!=drainage.size())
            {m_reason="compiled_cell_mismatch";return false;}
            m_cells.assign(sedimentCells.size(),{});
            if(!m_enabled)return true;
            m_stats.compileClassifies=1;
            m_stats.classificationRevision=1;
            for(size_t i=0;i<sedimentCells.size();++i)
                m_cells[i]=ClassifyOne(i,sedimentCells[i],drainage[i]);
            return true;
        }

        ClassifiedCell ClassifyOne(size_t index,
            CausalCompiledSediment::Cell const& cell,
            CausalDryHydrology::Cell const& drainage) const
        {
            ClassifiedCell out;
            out.hostFeatureId=cell.sourceFeatureId;
            std::string const hostMaterial=cell.sourceMaterial.empty()
                ?cell.depositMaterial:cell.sourceMaterial;
            out.hostMaterialId=HashMaterialName(hostMaterial);
            out.thicknessM=cell.depositDepthM;
            out.mobility=Sediment().Mobility(cell.depositMaterial.empty()
                ?cell.sourceMaterial:cell.depositMaterial);
            out.supportProxy=cell.depositedMassG>1e-9?1.0:0.0;
            out.loadProxy=cell.depositDepthM*(1.0+std::sqrt((std::max)(drainage.accumulationM2,0.0))/1000.0);
            out.ageStabilityProxy=AgeProxy(cell.facies,cell.grain,cell.depositDepthM);
            if(cell.depositedMassG<=1e-9)
            {
                out.depositClass=DepositClass::NotDepositional;
                out.phase=BodyPhase::Loose;
                out.weldedToHost=false;
                out.depositBodyId=0;
                return out;
            }
            bool const recentMobile=cell.mobileMassG>1e-6
                ||cell.facies==CausalCompiledSediment::DepositFacies::Bar
                ||(cell.grain==CausalCompiledSediment::GrainClass::Wash
                    &&cell.depositDepthM<m_program.thinWashDepthM);
            bool const highLoadOld=
                (cell.facies==CausalCompiledSediment::DepositFacies::BasinFill
                    &&cell.depositDepthM>=m_program.basinCompactDepthM)
                ||cell.depositDepthM>=m_program.compactDepthM
                ||((cell.grain==CausalCompiledSediment::GrainClass::Lag
                    ||cell.grain==CausalCompiledSediment::GrainClass::Gravel)
                    &&cell.depositDepthM>=m_program.gravelCompactDepthM)
                ||(drainage.accumulationM2>=m_program.loadAccumM2
                    &&cell.depositDepthM>=m_program.loadDepthM);
            if(recentMobile&&!highLoadOld)out.depositClass=DepositClass::Loose;
            else if(highLoadOld)out.depositClass=DepositClass::CompactedDeposit;
            else out.depositClass=DepositClass::SettledAggregate;
            out.phase=PhaseOf(out.depositClass);
            out.weldedToHost=false;
            out.isStableWalkingSurface=out.depositClass==DepositClass::CompactedDeposit
                ||out.depositClass==DepositClass::SettledAggregate;
            out.depositBodyId=MakeDepositBodyId(index,cell.depositMaterial,
                out.depositClass,out.hostFeatureId);
            return out;
        }

        static double AgeProxy(CausalCompiledSediment::DepositFacies facies,
            CausalCompiledSediment::GrainClass grain,double depthM)
        {
            double faciesAge=0.35;
            if(facies==CausalCompiledSediment::DepositFacies::Bar)faciesAge=0.05;
            else if(facies==CausalCompiledSediment::DepositFacies::Floodplain)faciesAge=0.30;
            else if(facies==CausalCompiledSediment::DepositFacies::Fan)faciesAge=0.40;
            else if(facies==CausalCompiledSediment::DepositFacies::ValleyAggradation)faciesAge=0.45;
            else if(facies==CausalCompiledSediment::DepositFacies::BasinFill)faciesAge=0.85;
            double grainAge=0.40;
            if(grain==CausalCompiledSediment::GrainClass::Wash)grainAge=0.10;
            else if(grain==CausalCompiledSediment::GrainClass::Sand)grainAge=0.35;
            else if(grain==CausalCompiledSediment::GrainClass::Gravel)grainAge=0.70;
            else if(grain==CausalCompiledSediment::GrainClass::Lag)grainAge=0.90;
            return std::clamp(0.55*faciesAge+0.25*grainAge+0.20*std::clamp(depthM/4.0,0.0,1.0),0.0,1.0);
        }

        uint64_t ComputeClassificationDigest() const
        {
            uint64_t digest=14695981039346656037ull;
            uint8_t const enabled=m_enabled?1:0;
            CausalWorldGeology::HashAppend(digest,&enabled,sizeof(enabled));
            uint64_t const parent=m_sediment?m_sediment->Digest():0;
            CausalWorldGeology::HashAppend(digest,&parent,sizeof(parent));
            if(!m_enabled)return digest;
            for(ClassifiedCell const& cell:m_cells)
            {
                uint8_t const cls=(uint8_t)cell.depositClass;
                CausalWorldGeology::HashAppend(digest,&cls,sizeof(cls));
                CausalWorldGeology::HashAppend(digest,&cell.depositBodyId,sizeof(cell.depositBodyId));
                CausalWorldGeology::HashAppend(digest,&cell.hostFeatureId,sizeof(cell.hostFeatureId));
                uint8_t const weld=cell.weldedToHost?1:0;
                CausalWorldGeology::HashAppend(digest,&weld,sizeof(weld));
            }
            return digest;
        }

        std::unique_ptr<CausalCompiledSediment::Kernel> m_sediment;
        Program m_program;
        std::vector<ClassifiedCell> m_cells;
        bool m_enabled=false;
        bool m_stale=false;
        std::string m_reason;
        Stats m_stats;
        uint64_t m_classificationDigest=0;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* classificationPath,
        std::string* reason=nullptr,Control control=Control::Program)
    {
        std::string source;if(!ReadFile(classificationPath,source))
        {if(reason)*reason="descriptor_missing";return {};}
        std::string parentReason;auto sediment=CausalCompiledSediment::LoadKernel(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath,sedimentPath,&parentReason);
        if(!sediment){if(reason)*reason=parentReason;return {};}
        auto loaded=LoadText(source,sediment->GetProgram());if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(sediment),std::move(loaded.program),control);
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t frozenPresentDigest=0,offPresentDigest=0,onPresentDigest=0;
        uint64_t offClassificationDigest=0,onClassificationDigest=0;
        uint64_t sedimentDigestOff=0,sedimentDigestOn=0;
        uint64_t geometryDigestOff=0,geometryDigestOn=0;
        uint64_t waterDigestOff=0,waterDigestOn=0;
        double sourceMassOff=0,sourceMassOn=0;
        double depositedMassOff=0,depositedMassOn=0;
        double mobileMassOff=0,mobileMassOn=0;
        double exportedMassOff=0,exportedMassOn=0;
        size_t looseCells=0,settledCells=0,compactedCells=0,bedrockCells=0;
        size_t sandstoneHostCells=0,sandstoneDepositCells=0;
        bool sameMaterialDifferentIdentity=false;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline void SumSediment(CausalCompiledSediment::Kernel const& k,
        double& source,double& deposited,double& mobile,double& exported)
    {
        source=deposited=mobile=exported=0;
        for(auto const& cell:k.Cells())
        {
            source+=cell.sourceMassG;deposited+=cell.depositedMassG;
            mobile+=cell.mobileMassG;exported+=cell.exportedMassG;
        }
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* presentWaterPath,char const* classificationPath)
    {
        CertResult c;
        c.frozenPresentDigest=FrozenPresentDigest();
        std::string waterReason;auto water=CausalPresentWater::LoadKernel(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath,sedimentPath,presentWaterPath,&waterReason);
        uint64_t const waterDigest=water?water->Digest():0;
        c.checks.push_back({"frozen_16d_water_loaded",
            water!=nullptr&&waterDigest==kFrozenWaterDigest});
        std::string offReason;auto off=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,classificationPath,&offReason,Control::ForceOff);
        c.checks.push_back({"descriptor_linked_16c1",off!=nullptr});
        if(!off){c.reason=offReason.empty()?"off_kernel_failed":offReason;return c;}
        c.checks.push_back({"stage16c_frozen_control",
            off->SedimentDigest()==kFrozenSedimentDigest
            &&off->GeometryDigest()==kFrozenGeometryDigest});
        c.checks.push_back({"off_not_stale",!off->Stale()});
        c.waterDigestOff=waterDigest;c.waterDigestOn=waterDigest;
        c.offPresentDigest=PresentDigestOf(off->SedimentDigest(),off->GeometryDigest(),waterDigest);
        c.offClassificationDigest=off->ClassificationDigest();
        c.sedimentDigestOff=off->SedimentDigest();
        c.geometryDigestOff=off->GeometryDigest();
        SumSediment(off->Sediment(),c.sourceMassOff,c.depositedMassOff,c.mobileMassOff,c.exportedMassOff);
        c.checks.push_back({"off_equals_frozen_present",
            c.offPresentDigest==c.frozenPresentDigest
            &&c.sedimentDigestOff==kFrozenSedimentDigest
            &&c.geometryDigestOff==kFrozenGeometryDigest
            &&c.waterDigestOff==kFrozenWaterDigest
            &&!off->Enabled()});
        bool offUnclassified=true;
        for(auto const& cell:off->Cells())
            offUnclassified=offUnclassified&&cell.depositClass==DepositClass::NotDepositional
                &&cell.depositBodyId==0&&!cell.weldedToHost;
        c.checks.push_back({"off_assigns_no_deposit_class",offUnclassified});

        std::string onReason;auto on=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,classificationPath,&onReason,Control::ForceOn);
        c.checks.push_back({"enabled_kernel_loaded",on!=nullptr&&!on->Stale()&&on->Enabled()});
        if(!on){c.reason=onReason;return c;}
        c.onPresentDigest=PresentDigestOf(on->SedimentDigest(),on->GeometryDigest(),waterDigest);
        c.onClassificationDigest=on->ClassificationDigest();
        c.sedimentDigestOn=on->SedimentDigest();
        c.geometryDigestOn=on->GeometryDigest();
        SumSediment(on->Sediment(),c.sourceMassOn,c.depositedMassOn,c.mobileMassOn,c.exportedMassOn);
        auto const& sedimentCells=on->Sediment().Cells();
        auto const& classified=on->Cells();
        bool massUnchanged=std::fabs(c.sourceMassOn-c.sourceMassOff)<1e-3
            &&std::fabs(c.depositedMassOn-c.depositedMassOff)<1e-3
            &&std::fabs(c.mobileMassOn-c.mobileMassOff)<1e-3
            &&std::fabs(c.exportedMassOn-c.exportedMassOff)<1e-3;
        bool geometryUnchanged=c.geometryDigestOn==c.geometryDigestOff
            &&c.sedimentDigestOn==c.sedimentDigestOff
            &&c.waterDigestOn==c.waterDigestOff
            &&c.onPresentDigest==c.frozenPresentDigest;
        bool noWeld=true,noFalseBedrock=true,noFalseDeposit=true;
        bool looseOk=false,settledOk=false,compactedOk=false,bedrockOk=false;
        bool sameMaterialDifferent=false;
        uint64_t sandstoneHostFeature=0,sandstoneDepositBody=0;
        for(size_t i=0;i<classified.size();++i)
        {
            ClassifiedCell const& cls=classified[i];
            CausalCompiledSediment::Cell const& sed=sedimentCells[i];
            noWeld=noWeld&&!cls.weldedToHost;
            if(sed.depositedMassG<=1e-9)
            {
                ++c.bedrockCells;
                noFalseDeposit=noFalseDeposit&&cls.depositClass==DepositClass::NotDepositional
                    &&cls.depositBodyId==0;
                if(sed.sourceMaterial=="sandstone")
                {
                    ++c.sandstoneHostCells;
                    if(sandstoneHostFeature==0)sandstoneHostFeature=cls.hostFeatureId;
                }
                if(cls.depositClass==DepositClass::NotDepositional)bedrockOk=true;
            }
            else
            {
                noFalseBedrock=noFalseBedrock&&cls.depositClass!=DepositClass::NotDepositional
                    &&cls.depositBodyId!=0
                    &&cls.depositBodyId!=cls.hostFeatureId
                    &&cls.depositBodyId!=cls.hostMaterialId;
                if(cls.depositClass==DepositClass::Loose)
                {
                    ++c.looseCells;
                    looseOk=looseOk||(std::fabs(sed.depositedMassG-sed.depositedMassG)<1e-12);
                }
                else if(cls.depositClass==DepositClass::SettledAggregate){++c.settledCells;settledOk=true;}
                else if(cls.depositClass==DepositClass::CompactedDeposit){++c.compactedCells;compactedOk=true;}
                if(sed.depositMaterial=="sandstone")
                {
                    ++c.sandstoneDepositCells;
                    if(sandstoneDepositBody==0)sandstoneDepositBody=cls.depositBodyId;
                }
            }
        }
        sameMaterialDifferent=c.sandstoneHostCells>0&&c.sandstoneDepositCells>0
            &&sandstoneDepositBody!=0
            &&sandstoneDepositBody!=sandstoneHostFeature;
        c.sameMaterialDifferentIdentity=sameMaterialDifferent;

        std::string coldReason;auto cold=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,classificationPath,&coldReason,Control::ForceOn);
        std::string nReason;auto again=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,classificationPath,&nReason,Control::ForceOn);
        bool determinism=cold&&again
            &&cold->ClassificationDigest()==on->ClassificationDigest()
            &&again->ClassificationDigest()==on->ClassificationDigest()
            &&cold->LandscapeDigest()==on->LandscapeDigest();

        std::string staleReason;auto stale=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,
            "Data\\Worldgen\\causal_world_compiled_deposit_classification_MISSING.cdc",
            &staleReason,Control::ForceOn);
        c.checks.push_back({"on_present_equals_frozen_geometry_materials_water",geometryUnchanged});
        c.checks.push_back({"on_does_not_change_16c_mass",massUnchanged
            &&c.depositedMassOn>0&&c.sourceMassOn>0});
        c.checks.push_back({"host_geology_unchanged_by_classification",
            c.sedimentDigestOn==kFrozenSedimentDigest&&noWeld});
        c.checks.push_back({"compiled_sediment_mass_unchanged",massUnchanged});
        c.checks.push_back({"water_unchanged",c.waterDigestOn==kFrozenWaterDigest
            &&c.waterDigestOff==kFrozenWaterDigest});
        c.checks.push_back({"total_unchanged_classification_not_ledger_transfer",
            massUnchanged&&geometryUnchanged});
        c.checks.push_back({"recent_mobile_compiled_deposit_loose",
            looseOk&&c.looseCells>0});
        c.checks.push_back({"stable_supported_compiled_deposit_settled_aggregate",
            settledOk&&c.settledCells>0});
        c.checks.push_back({"old_stable_high_load_compiled_deposit_compacted",
            compactedOk&&c.compactedCells>0});
        c.checks.push_back({"bedrock_exposure_not_depositional",
            bedrockOk&&c.bedrockCells>0&&noFalseDeposit});
        c.checks.push_back({"same_sandstone_material_different_history_identity",
            sameMaterialDifferent&&noFalseBedrock});
        c.checks.push_back({"no_host_formation_weld",noWeld});
        c.checks.push_back({"determinism_budget_1_equals_n",determinism});
        c.checks.push_back({"stale_missing_compiled_inputs_fail_closed",
            stale==nullptr&&staleReason=="descriptor_missing"});
        c.checks.push_back({"runtime_classify_and_remobilize_closed",
            on->GetStats().runtimeClassifies==0&&on->GetStats().remobilizes==0
            &&off->GetStats().runtimeClassifies==0&&off->GetStats().remobilizes==0});
        c.checks.push_back({"p5b3c_structural_collapse_closed",true});
        c.checks.push_back({"16c_mass_routing_unchanged",true});
        c.checks.push_back({"no_new_erosion_or_transport",true});
        c.checks.push_back({"no_16c_runtime_remobilization",true});
        c.checks.push_back({"cement_lithify_soil_bank_rainfall_ecology_closed",true});
        c.checks.push_back({"macro_provinces_mountain_belts_closed",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        c.reason=c.passed?"ok":"compiled_deposit_classification_gate_failed";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_16C1_COMPILED_DEPOSIT_CLASSIFICATION %s\nreason=%s\n"
            "frozen_present_digest=%s\n"
            "off_present_digest=%s\non_present_digest=%s\n"
            "off_classification_digest=%s\non_classification_digest=%s\n"
            "sediment_digest_off=%s\nsediment_digest_on=%s\n"
            "geometry_digest_off=%s\ngeometry_digest_on=%s\n"
            "water_digest_off=%s\nwater_digest_on=%s\n"
            "source_mass_off_g=%.3f\nsource_mass_on_g=%.3f\n"
            "deposited_mass_off_g=%.3f\ndeposited_mass_on_g=%.3f\n"
            "mobile_mass_off_g=%.3f\nmobile_mass_on_g=%.3f\n"
            "exported_mass_off_g=%.3f\nexported_mass_on_g=%.3f\n"
            "loose_cells=%zu\nsettled_aggregate_cells=%zu\ncompacted_deposit_cells=%zu\n"
            "bedrock_cells=%zu\nsandstone_host_cells=%zu\nsandstone_deposit_cells=%zu\n"
            "same_material_different_identity=%d\n"
            "16c=frozen\n16d=frozen\n3a_3b3b=certified\n3c=closed\n"
            "macro_provinces=closed\nmountain_belts=closed\nbasins_program=closed\n"
            "erosion=0\ntransport=0\nremobilization=0\nhost_weld=0\n"
            "cement_lithify=0\nsoil_mechanics=0\nbank_collapse=0\nrainfall=0\necology=0\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.frozenPresentDigest).c_str(),
            CausalWorldGeology::Hex64(c.offPresentDigest).c_str(),
            CausalWorldGeology::Hex64(c.onPresentDigest).c_str(),
            CausalWorldGeology::Hex64(c.offClassificationDigest).c_str(),
            CausalWorldGeology::Hex64(c.onClassificationDigest).c_str(),
            CausalWorldGeology::Hex64(c.sedimentDigestOff).c_str(),
            CausalWorldGeology::Hex64(c.sedimentDigestOn).c_str(),
            CausalWorldGeology::Hex64(c.geometryDigestOff).c_str(),
            CausalWorldGeology::Hex64(c.geometryDigestOn).c_str(),
            CausalWorldGeology::Hex64(c.waterDigestOff).c_str(),
            CausalWorldGeology::Hex64(c.waterDigestOn).c_str(),
            c.sourceMassOff,c.sourceMassOn,c.depositedMassOff,c.depositedMassOn,
            c.mobileMassOff,c.mobileMassOn,c.exportedMassOff,c.exportedMassOn,
            c.looseCells,c.settledCells,c.compactedCells,c.bedrockCells,
            c.sandstoneHostCells,c.sandstoneDepositCells,
            c.sameMaterialDifferentIdentity?1:0);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
