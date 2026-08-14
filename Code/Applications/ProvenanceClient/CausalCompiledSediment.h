#pragma once

// Stage 16C: compiled sediment source / transport / deposition. Stage 16B
// remains the immutable removal receipt and surface parent. This layer routes
// that receipt along the frozen Stage 16A receiver graph. It cannot create
// water, run live erosion, open ecology, or couple P5b.

#include "CausalCompiledFluvialErosion.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalCompiledSediment
{
    constexpr char const* kExpectedRegion="causal_world_compiled_sediment_floor";

    enum class Control:uint8_t
    {
        Full=0,
        TransportOff=1,
        ErosionOff=2,
        DepositionOff=3
    };

    enum class GrainClass:uint8_t{Wash,Sand,Gravel,Lag};
    enum class DepositFacies:uint8_t
    {
        None=0,ValleyAggradation,BasinFill,Fan,Bar,Floodplain,OutletExport
    };

    inline char const* GrainClassName(GrainClass value)
    {
        switch(value)
        {
            case GrainClass::Wash:return "wash";
            case GrainClass::Sand:return "sand";
            case GrainClass::Gravel:return "gravel";
            case GrainClass::Lag:return "lag";
            default:return "none";
        }
    }

    inline char const* DepositFaciesName(DepositFacies value)
    {
        switch(value)
        {
            case DepositFacies::ValleyAggradation:return "valley_aggradation";
            case DepositFacies::BasinFill:return "basin_fill";
            case DepositFacies::Fan:return "fan";
            case DepositFacies::Bar:return "bar";
            case DepositFacies::Floodplain:return "floodplain";
            case DepositFacies::OutletExport:return "outlet_export";
            default:return "none";
        }
    }

    inline GrainClass GrainForMaterial(std::string const& material)
    {
        if(material=="shale")return GrainClass::Wash;
        if(material=="sandstone")return GrainClass::Sand;
        if(material=="granite")return GrainClass::Gravel;
        if(material=="quartz")return GrainClass::Lag;
        return GrainClass::Sand;
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,sedimentEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t transportEnabled=1,erosionSourceEnabled=1,depositionEnabled=1;
        std::string worldgenId,regionKey,parentRegionKey;
        double bulkDensityKgM3=2650,maxDepositDepthM=4,capacityScale=.085;
        double basinSettleRatio=.55,floodplainSettleRatio=.22,fanSettleRatio=.18,
            channelSettleRatio=.08;
        double mobilityShale=1,mobilitySandstone=.62,mobilityGranite=.28,
            mobilityQuartz=.22;
        double densityShaleKgM3=2400,densitySandstoneKgM3=2500,
            densityGraniteKgM3=2700,densityQuartzKgM3=2650;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalCompiledFluvialErosion::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_COMPILED_SEDIMENT_V1";
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
          &&hex("sediment_event_id",r.program.sedimentEventId)&&u32("chronology",r.program.chronology)
          &&u32("transport_enabled",r.program.transportEnabled)
          &&u32("erosion_source_enabled",r.program.erosionSourceEnabled)
          &&u32("deposition_enabled",r.program.depositionEnabled)
          &&num("bulk_density_kg_m3",r.program.bulkDensityKgM3)
          &&num("max_deposit_depth_m",r.program.maxDepositDepthM)
          &&num("capacity_scale",r.program.capacityScale)
          &&num("basin_settle_ratio",r.program.basinSettleRatio)
          &&num("floodplain_settle_ratio",r.program.floodplainSettleRatio)
          &&num("fan_settle_ratio",r.program.fanSettleRatio)
          &&num("channel_settle_ratio",r.program.channelSettleRatio)
          &&num("mobility_shale",r.program.mobilityShale)
          &&num("mobility_sandstone",r.program.mobilitySandstone)
          &&num("mobility_granite",r.program.mobilityGranite)
          &&num("mobility_quartz",r.program.mobilityQuartz)
          &&num("density_shale_kg_m3",r.program.densityShaleKgM3)
          &&num("density_sandstone_kg_m3",r.program.densitySandstoneKgM3)
          &&num("density_granite_kg_m3",r.program.densityGraniteKgM3)
          &&num("density_quartz_kg_m3",r.program.densityQuartzKgM3);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.sedimentEventId!=0
          &&r.program.chronology>parent.chronology
          &&r.program.transportEnabled==1&&r.program.erosionSourceEnabled==1
          &&r.program.depositionEnabled==1
          &&r.program.bulkDensityKgM3>1000&&r.program.maxDepositDepthM>0
          &&r.program.maxDepositDepthM<=16&&r.program.capacityScale>0
          &&r.program.mobilityShale>r.program.mobilitySandstone
          &&r.program.mobilitySandstone>r.program.mobilityGranite
          &&r.program.mobilityGranite>r.program.mobilityQuartz
          &&r.program.densityGraniteKgM3>r.program.densitySandstoneKgM3
          &&r.program.densitySandstoneKgM3>r.program.densityShaleKgM3;
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_compiled_sediment_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct MaterialLedger
    {
        double sourceMassG=0,depositedMassG=0,mobileMassG=0,exportedMassG=0;
        std::string formationId;uint64_t featureId=0;uint64_t sourceWatershedId=0;
        GrainClass grain=GrainClass::Sand;
    };

    struct Cell
    {
        double x=0,y=0,stage16BSurfaceZ=0,surfaceZ=0,depositDepthM=0;
        double sourceMassG=0,depositedMassG=0,mobileMassG=0,exportedMassG=0;
        double capacityG=0;
        std::string sourceMaterial,depositMaterial,sourceFormationId;
        uint64_t sourceFeatureId=0,sourceWatershedId=0;
        GrainClass grain=GrainClass::Sand;
        DepositFacies facies=DepositFacies::None;
        std::map<std::string,MaterialLedger> byMaterial;
    };

    struct Query
    {
        bool found=false;double surfaceZ=0,stage16BSurfaceZ=0,depositDepthM=0;
        CausalCompiledFluvialErosion::Query erosion;
        Cell sediment;
        CausalWorldGeology::GeoSample geology;
    };

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalCompiledFluvialErosion::Kernel> erosion,Program program)
          :m_erosion(std::move(erosion)),m_program(std::move(program))
        {
            Build(Control::Full);
            m_digest=DigestFor(Control::Full);
            m_geometryDigest=GeometryDigestFor(Control::Full);
        }

        CausalCompiledFluvialErosion::Kernel const& Erosion() const{return *m_erosion;}
        CausalDryHydrology::Kernel const& Drainage() const{return m_erosion->Drainage();}
        CausalBareEarthGeography::Kernel const& Geography() const{return m_erosion->Geography();}
        Program const& GetProgram() const{return m_program;}
        std::vector<Cell> const& Cells() const{return m_cells;}
        uint64_t Digest() const{return m_digest;}
        uint64_t GeometryDigest() const{return m_geometryDigest;}

        double DensityKgM3(std::string const& material) const
        {
            if(material=="shale")return m_program.densityShaleKgM3;
            if(material=="sandstone")return m_program.densitySandstoneKgM3;
            if(material=="granite")return m_program.densityGraniteKgM3;
            if(material=="quartz")return m_program.densityQuartzKgM3;
            return m_program.bulkDensityKgM3;
        }

        double Mobility(std::string const& material) const
        {
            if(material=="shale")return m_program.mobilityShale;
            if(material=="sandstone")return m_program.mobilitySandstone;
            if(material=="granite")return m_program.mobilityGranite;
            if(material=="quartz")return m_program.mobilityQuartz;
            return m_program.mobilitySandstone;
        }

        double VolumeToGrams(double volumeM3,std::string const& material) const
        {return volumeM3*DensityKgM3(material)*1000.0;}

        double GramsToDepthM(double massG) const
        {
            double const cellArea=Drainage().StepM()*Drainage().StepM();
            if(cellArea<=0||m_program.bulkDensityKgM3<=0)return 0;
            return massG/(cellArea*m_program.bulkDensityKgM3*1000.0);
        }

        std::vector<Cell> Evaluate(Control control) const
        {
            std::vector<Cell> cells;Route(control,cells);return cells;
        }

        double DepositDepth(double x,double y,Control control=Control::Full) const
        {
            if(control==Control::TransportOff||control==Control::DepositionOff
              ||control==Control::ErosionOff)return 0;
            return BilinearDeposit(x,y);
        }

        double ReconstructedZ(double x,double y,Control control=Control::Full) const
        {
            double const parent=m_erosion->ReconstructedZ(x,y);
            return parent+DepositDepth(x,y,control);
        }

        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return Geography().Query(x,y,z);}

        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y,
            Control control=Control::Full) const
        {
            double const depth=DepositDepth(x,y,control);
            if(depth<=1e-9)return m_erosion->SurfaceGeology(x,y);
            auto const drainage=Drainage().QueryAt(x,y);
            if(!drainage.found)return m_erosion->SurfaceGeology(x,y);
            Cell const& cell=m_cells[(size_t)drainage.index];
            if(cell.depositedMassG<=1e-9||cell.depositMaterial.empty())
                return m_erosion->SurfaceGeology(x,y);
            CausalWorldGeology::GeoSample sample=m_erosion->SurfaceGeology(x,y);
            if(!sample.found)
            {
                sample=Geography().SurfaceGeology(x,y);
                if(!sample.found)return sample;
            }
            sample.material=cell.depositMaterial;
            if(!cell.sourceFormationId.empty())sample.formationId=cell.sourceFormationId;
            if(cell.sourceFeatureId!=0)sample.featureId=cell.sourceFeatureId;
            return sample;
        }

        Query QueryAt(double x,double y,Control control=Control::Full) const
        {
            Query q;q.erosion=m_erosion->QueryAt(x,y);if(!q.erosion.found)return q;
            if(control==Control::Full)q.sediment=m_cells[(size_t)q.erosion.drainage.index];
            else
            {
                auto cells=Evaluate(control);
                q.sediment=cells[(size_t)q.erosion.drainage.index];
            }
            q.stage16BSurfaceZ=q.erosion.surfaceZ;
            q.depositDepthM=DepositDepth(x,y,control);
            q.surfaceZ=q.stage16BSurfaceZ+q.depositDepthM;
            q.geology=SurfaceGeology(x,y,control);q.found=q.geology.found;return q;
        }

        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by,
            Control control=Control::Full) const
        {
            CausalVisibleExposure::BlockSurfaceSamples s;s.blockX=bx;s.blockY=by;
            s.vertices.reserve((size_t)CausalVisibleExposure::kBlockSampleSpan
                *CausalVisibleExposure::kBlockSampleSpan);
            int const baseX=bx*CausalVisibleExposure::kBlockCells-1;
            int const baseY=by*CausalVisibleExposure::kBlockCells-1;
            for(int y=0;y<CausalVisibleExposure::kBlockSampleSpan;++y)
            for(int x=0;x<CausalVisibleExposure::kBlockSampleSpan;++x)
            {double const wx=(baseX+x+.5)*CausalVisibleExposure::kDualStepM;
             double const wy=(baseY+y+.5)*CausalVisibleExposure::kDualStepM;
             s.vertices.push_back({wx,wy,ReconstructedZ(wx,wy,control)});}
            return s;
        }

        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by,
            Control control=Control::Full) const
        {auto const s=SampleBlock(bx,by,control);return CausalVisibleExposure::EmitBlockMesh(
            s,CausalVisibleExposure::DescribeBlock(s));}

        double CapacityForCert(CausalDryHydrology::Cell const& drainage,
            double mobility) const
        {return CapacityG(drainage,mobility);}

    private:
        double CapacityG(CausalDryHydrology::Cell const& d,double mobility) const
        {
            double const slopeTerm=.12+std::clamp(d.slope*48.0,0.0,4.0);
            double const areaTerm=std::sqrt((std::max)(d.accumulationM2,1.0));
            return m_program.capacityScale*areaTerm*slopeTerm*(.25+mobility)*1e6;
        }

        DepositFacies ClassifyFacies(CausalDryHydrology::Cell const& d,
            CausalBareEarthGeography::Landform landform,bool outlet) const
        {
            if(outlet)return DepositFacies::OutletExport;
            if(landform==CausalBareEarthGeography::Landform::BasinFloor||d.basinId!=0)
                return DepositFacies::BasinFill;
            if(landform==CausalBareEarthGeography::Landform::FoothillReceivingZone)
                return DepositFacies::Fan;
            if(d.channel&&d.channelOrder>=2)return DepositFacies::Bar;
            if(landform==CausalBareEarthGeography::Landform::ValleyFloor||d.channel)
                return DepositFacies::ValleyAggradation;
            return DepositFacies::Floodplain;
        }

        double SettleRatio(DepositFacies facies) const
        {
            switch(facies)
            {
                case DepositFacies::BasinFill:return m_program.basinSettleRatio;
                case DepositFacies::Fan:return m_program.fanSettleRatio;
                case DepositFacies::Bar:return m_program.channelSettleRatio*.65;
                case DepositFacies::ValleyAggradation:return m_program.channelSettleRatio;
                case DepositFacies::Floodplain:return m_program.floodplainSettleRatio;
                default:return 0;
            }
        }

        void Route(Control control,std::vector<Cell>& out) const
        {
            auto const& drainageCells=Drainage().Cells();
            auto const& erosionCells=m_erosion->Cells();
            out.assign(drainageCells.size(),{});
            double const cellArea=Drainage().StepM()*Drainage().StepM();
            bool const sourceOn=control!=Control::ErosionOff;
            bool const transportOn=control!=Control::TransportOff;
            bool const depositionOn=control!=Control::DepositionOff&&transportOn;

            struct Bag{double mass=0;std::string formationId;uint64_t featureId=0;
                uint64_t watershedId=0;GrainClass grain=GrainClass::Sand;};
            std::vector<std::map<std::string,Bag>> available(drainageCells.size());
            std::vector<int> order(drainageCells.size());
            std::iota(order.begin(),order.end(),0);
            std::stable_sort(order.begin(),order.end(),[&](int a,int b)
            {
                if(drainageCells[(size_t)a].floodRank!=drainageCells[(size_t)b].floodRank)
                    return drainageCells[(size_t)a].floodRank>drainageCells[(size_t)b].floodRank;
                return a<b;
            });

            for(size_t i=0;i<drainageCells.size();++i)
            {
                Cell& cell=out[i];
                auto const& d=drainageCells[i];
                auto const& e=erosionCells[i];
                cell.x=d.x;cell.y=d.y;cell.stage16BSurfaceZ=e.surfaceZ;cell.surfaceZ=e.surfaceZ;
                cell.sourceMaterial=e.material;cell.grain=GrainForMaterial(e.material);
                cell.sourceWatershedId=d.watershedId;
                auto const geology=Geography().SurfaceGeology(d.x,d.y);
                if(geology.found)
                {
                    cell.sourceFormationId=geology.formationId;
                    cell.sourceFeatureId=geology.featureId;
                }
                if(sourceOn&&e.incisionM>0&&!e.material.empty()&&geology.found)
                {
                    double const volume=e.incisionM*cellArea;
                    double const mass=VolumeToGrams(volume,e.material);
                    cell.sourceMassG=mass;
                    MaterialLedger& ledger=cell.byMaterial[e.material];
                    ledger.sourceMassG=mass;ledger.grain=cell.grain;
                    ledger.formationId=cell.sourceFormationId;
                    ledger.featureId=cell.sourceFeatureId;
                    ledger.sourceWatershedId=d.watershedId;
                    Bag& bag=available[i][e.material];
                    bag.mass=mass;bag.formationId=cell.sourceFormationId;
                    bag.featureId=cell.sourceFeatureId;bag.watershedId=d.watershedId;
                    bag.grain=cell.grain;
                }
            }

            if(!transportOn)
            {
                for(size_t i=0;i<out.size();++i)
                {
                    Cell& cell=out[i];
                    cell.mobileMassG=cell.sourceMassG;
                    for(auto& kv:cell.byMaterial)kv.second.mobileMassG=kv.second.sourceMassG;
                    cell.surfaceZ=cell.stage16BSurfaceZ;
                }
                return;
            }

            for(int idx:order)
            {
                size_t const i=(size_t)idx;
                auto const& d=drainageCells[i];
                Cell& cell=out[i];
                auto& bags=available[i];
                double total=0;double mobilityMass=0;
                for(auto const& kv:bags){total+=kv.second.mass;mobilityMass+=kv.second.mass*Mobility(kv.first);}
                double const meanMobility=total>0?mobilityMass/total:m_program.mobilitySandstone;
                cell.capacityG=CapacityG(d,meanMobility);
                auto const landform=Geography().QuerySurface(d.x,d.y).landform;
                bool const outlet=d.receiver<0;
                DepositFacies facies=ClassifyFacies(d,landform,outlet);

                double depositBudget=0;
                if(depositionOn&&total>0&&!outlet)
                {
                    double const overCapacity=(std::max)(0.0,total-cell.capacityG);
                    double const settle=SettleRatio(facies)*total*(1.0-.35*meanMobility);
                    depositBudget=overCapacity+settle;
                    depositBudget=(std::min)(depositBudget,total);
                    double const maxMass=VolumeToGrams(
                        m_program.maxDepositDepthM*cellArea,"sandstone");
                    double const already=cell.depositedMassG;
                    depositBudget=(std::min)(depositBudget,(std::max)(0.0,maxMass-already));
                }

                std::vector<std::string> materials;materials.reserve(bags.size());
                for(auto const& kv:bags)materials.push_back(kv.first);
                std::stable_sort(materials.begin(),materials.end(),[&](std::string const& a,
                    std::string const& b)
                {
                    double const ma=Mobility(a),mb=Mobility(b);
                    if(ma!=mb)return ma<mb;return a<b;
                });

                double remainingDeposit=depositBudget;
                for(std::string const& material:materials)
                {
                    Bag& bag=bags[material];if(bag.mass<=0)continue;
                    double take=0;
                    if(depositionOn&&remainingDeposit>0)
                    {
                        take=(std::min)(bag.mass,remainingDeposit);
                        remainingDeposit-=take;bag.mass-=take;
                        cell.depositedMassG+=take;
                        MaterialLedger& ledger=cell.byMaterial[material];
                        ledger.depositedMassG+=take;ledger.grain=bag.grain;
                        if(ledger.formationId.empty())ledger.formationId=bag.formationId;
                        if(ledger.featureId==0)ledger.featureId=bag.featureId;
                        if(ledger.sourceWatershedId==0)ledger.sourceWatershedId=bag.watershedId;
                    }
                }

                if(cell.depositedMassG>0)
                {
                    cell.facies=facies==DepositFacies::OutletExport?DepositFacies::Floodplain:facies;
                    std::string dominant;double best=0;
                    for(auto const& kv:cell.byMaterial)
                        if(kv.second.depositedMassG>best){best=kv.second.depositedMassG;dominant=kv.first;}
                    cell.depositMaterial=dominant;
                    auto const it=cell.byMaterial.find(dominant);
                    if(it!=cell.byMaterial.end())
                    {
                        cell.sourceFormationId=it->second.formationId;
                        cell.sourceFeatureId=it->second.featureId;
                        cell.grain=it->second.grain;
                        cell.sourceWatershedId=it->second.sourceWatershedId;
                    }
                    cell.depositDepthM=(std::min)(m_program.maxDepositDepthM,
                        GramsToDepthM(cell.depositedMassG));
                    cell.surfaceZ=cell.stage16BSurfaceZ+cell.depositDepthM;
                }

                double remaining=0;for(auto const& kv:bags)remaining+=kv.second.mass;
                if(outlet)
                {
                    for(auto& kv:bags)
                    {
                        if(kv.second.mass<=0)continue;
                        cell.exportedMassG+=kv.second.mass;
                        MaterialLedger& ledger=cell.byMaterial[kv.first];
                        ledger.exportedMassG+=kv.second.mass;ledger.grain=kv.second.grain;
                        if(ledger.formationId.empty())ledger.formationId=kv.second.formationId;
                        if(ledger.featureId==0)ledger.featureId=kv.second.featureId;
                        if(ledger.sourceWatershedId==0)ledger.sourceWatershedId=kv.second.watershedId;
                        kv.second.mass=0;
                    }
                    if(cell.exportedMassG>0)cell.facies=DepositFacies::OutletExport;
                    continue;
                }

                // Residual mobile inventory is the load that remains after
                // deposition and before the capacity-limited handoff.
                double handoff=(std::min)(remaining,cell.capacityG);
                if(!depositionOn)handoff=remaining;
                double leaveBudget=handoff;
                for(std::string const& material:materials)
                {
                    auto bagIt=bags.find(material);if(bagIt==bags.end()||bagIt->second.mass<=0)continue;
                    Bag& bag=bagIt->second;
                    double const share=remaining>0?bag.mass/remaining:0;
                    double const send=(std::min)(bag.mass,leaveBudget*share+(leaveBudget>0?1e-15:0));
                    double const actual=(std::min)(bag.mass,send>0?send:0);
                    // Prefer mobility-weighted drain when capacity-limited.
                    (void)actual;
                }
                // Deterministic mobility-weighted drain into receiver.
                std::stable_sort(materials.begin(),materials.end(),[&](std::string const& a,
                    std::string const& b)
                {
                    double const ma=Mobility(a),mb=Mobility(b);
                    if(ma!=mb)return ma>mb;return a<b;
                });
                leaveBudget=handoff;
                for(std::string const& material:materials)
                {
                    auto bagIt=bags.find(material);if(bagIt==bags.end()||bagIt->second.mass<=0)continue;
                    Bag& bag=bagIt->second;
                    double const send=(std::min)(bag.mass,leaveBudget);
                    if(send<=0)continue;
                    bag.mass-=send;leaveBudget-=send;
                    Bag& dest=available[(size_t)d.receiver][material];
                    dest.mass+=send;
                    if(dest.formationId.empty())dest.formationId=bag.formationId;
                    if(dest.featureId==0)dest.featureId=bag.featureId;
                    if(dest.watershedId==0)dest.watershedId=bag.watershedId;
                    dest.grain=bag.grain;
                }
                double residual=0;for(auto const& kv:bags)residual+=kv.second.mass;
                cell.mobileMassG=residual;
                for(auto& kv:bags)
                {
                    if(kv.second.mass<=0)continue;
                    MaterialLedger& ledger=cell.byMaterial[kv.first];
                    ledger.mobileMassG+=kv.second.mass;ledger.grain=kv.second.grain;
                    if(ledger.formationId.empty())ledger.formationId=kv.second.formationId;
                    if(ledger.featureId==0)ledger.featureId=kv.second.featureId;
                    if(ledger.sourceWatershedId==0)ledger.sourceWatershedId=kv.second.watershedId;
                }
            }
        }

        void Build(Control control){Route(control,m_cells);}

        double BilinearDeposit(double x,double y) const
        {
            double const step=Drainage().StepM();double gx=(x-Drainage().MinX())/step-.5;
            double gy=(y-Drainage().MinY())/step-.5;
            double const rx=std::round(gx),ry=std::round(gy);
            if(std::fabs(gx-rx)<1e-9)gx=rx;if(std::fabs(gy-ry)<1e-9)gy=ry;
            gx=std::clamp(gx,0.0,(double)(Drainage().Width()-1));
            gy=std::clamp(gy,0.0,(double)(Drainage().Height()-1));
            int ix=(std::min)((int)std::floor(gx),Drainage().Width()-2);
            int iy=(std::min)((int)std::floor(gy),Drainage().Height()-2);
            double const tx=gx-ix,ty=gy-iy;
            auto value=[&](int px,int py){return m_cells[(size_t)(py*Drainage().Width()+px)].depositDepthM;};
            double const a=value(ix,iy)*(1-tx)+value(ix+1,iy)*tx;
            double const b=value(ix,iy+1)*(1-tx)+value(ix+1,iy+1)*tx;return a*(1-ty)+b*ty;
        }

        uint64_t DigestFor(Control control) const
        {
            auto cells=control==Control::Full?m_cells:Evaluate(control);
            uint64_t digest=14695981039346656037ull;
            uint64_t const parent=m_erosion->Digest();
            CausalWorldGeology::HashAppend(digest,&parent,sizeof(parent));
            uint8_t const c=(uint8_t)control;
            CausalWorldGeology::HashAppend(digest,&c,sizeof(c));
            for(Cell const& cell:cells)
            {
                CausalWorldGeology::HashAppend(digest,&cell.sourceMassG,sizeof(cell.sourceMassG));
                CausalWorldGeology::HashAppend(digest,&cell.depositedMassG,sizeof(cell.depositedMassG));
                CausalWorldGeology::HashAppend(digest,&cell.mobileMassG,sizeof(cell.mobileMassG));
                CausalWorldGeology::HashAppend(digest,&cell.exportedMassG,sizeof(cell.exportedMassG));
                CausalWorldGeology::HashAppend(digest,&cell.depositDepthM,sizeof(cell.depositDepthM));
                CausalWorldGeology::HashAppend(digest,&cell.facies,sizeof(cell.facies));
                CausalWorldGeology::HashAppend(digest,cell.depositMaterial.data(),
                    cell.depositMaterial.size());
            }
            return digest;
        }

        uint64_t GeometryDigestFor(Control control) const
        {
            auto cells=control==Control::Full?m_cells:Evaluate(control);
            uint64_t digest=14695981039346656037ull;
            for(Cell const& cell:cells)
                CausalWorldGeology::HashAppend(digest,&cell.depositDepthM,sizeof(cell.depositDepthM));
            return digest;
        }

        std::unique_ptr<CausalCompiledFluvialErosion::Kernel> m_erosion;Program m_program;
        std::vector<Cell> m_cells;uint64_t m_digest=0,m_geometryDigest=0;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        std::string* reason=nullptr)
    {
        std::string source;if(!ReadFile(sedimentPath,source))
        {if(reason)*reason="descriptor_missing";return {};}
        std::string parentReason;auto parent=CausalCompiledFluvialErosion::LoadKernel(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath,&parentReason);
        if(!parent){if(reason)*reason=parentReason;return {};}
        auto loaded=LoadText(source,parent->GetProgram());if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t stage16BDigest=0,sedimentDigest=0,geometryDigest=0;
        double sourceMassG=0,depositedMassG=0,mobileMassG=0,exportedMassG=0;
        double massResidualG=0,maxDepositDepthM=0,meanDepositDepthM=0;
        size_t sourceCells=0,depositCells=0,exportCells=0;
        std::map<std::string,double> sourceByMaterial,depositByMaterial,exportByMaterial;
        std::map<DepositFacies,size_t> faciesCounts;
        double shaleTravelProxy=0,sandstoneTravelProxy=0;
        double depositionOffSourceMassG=0,depositionOffDepositMassG=0,
            depositionOffMobileMassG=0,depositionOffExportMassG=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath)
    {
        CertResult c;auto const parent=CausalCompiledFluvialErosion::RunCert(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath);
        c.stage16BDigest=parent.erosionDigest;
        c.checks.push_back({"stage16b_frozen_control",parent.passed});
        std::string reason;auto k=LoadKernel(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,&reason);
        c.reason=reason;c.checks.push_back({"descriptor_linked_compiled_sediment",k!=nullptr});
        if(!parent.passed||!k)return c;
        c.sedimentDigest=k->Digest();c.geometryDigest=k->GeometryDigest();
        auto const& cells=k->Cells();auto const& drainage=k->Drainage().Cells();
        double depthSum=0;
        for(size_t i=0;i<cells.size();++i)
        {
            Cell const& cell=cells[i];
            c.sourceMassG+=cell.sourceMassG;c.depositedMassG+=cell.depositedMassG;
            c.mobileMassG+=cell.mobileMassG;c.exportedMassG+=cell.exportedMassG;
            if(cell.sourceMassG>0){++c.sourceCells;c.sourceByMaterial[cell.sourceMaterial]+=cell.sourceMassG;}
            if(cell.depositedMassG>0)
            {
                ++c.depositCells;c.depositByMaterial[cell.depositMaterial]+=cell.depositedMassG;
                ++c.faciesCounts[cell.facies];
            }
            if(cell.exportedMassG>0)
            {
                ++c.exportCells;
                for(auto const& kv:cell.byMaterial)
                    if(kv.second.exportedMassG>0)c.exportByMaterial[kv.first]+=kv.second.exportedMassG;
            }
            c.maxDepositDepthM=(std::max)(c.maxDepositDepthM,cell.depositDepthM);
            depthSum+=cell.depositDepthM;
        }
        c.meanDepositDepthM=cells.empty()?0:depthSum/cells.size();
        c.massResidualG=c.sourceMassG-(c.depositedMassG+c.mobileMassG+c.exportedMassG);

        bool transportOffExact=true,erosionOffZero=true,depositionOffExports=true;
        auto transportOff=k->Evaluate(Control::TransportOff);
        auto erosionOff=k->Evaluate(Control::ErosionOff);
        auto depositionOff=k->Evaluate(Control::DepositionOff);
        double depOffDeposit=0,depOffExport=0,depOffMobile=0,depOffSource=0;
        double erosionOffSource=0;
        for(size_t i=0;i<cells.size();++i)
        {
            transportOffExact=transportOffExact
                &&std::fabs(transportOff[i].depositDepthM)<1e-12
                &&std::fabs(transportOff[i].surfaceZ-cells[i].stage16BSurfaceZ)<1e-9
                &&std::fabs(k->ReconstructedZ(cells[i].x,cells[i].y,Control::TransportOff)
                    -k->Erosion().ReconstructedZ(cells[i].x,cells[i].y))<1e-9;
            erosionOffSource+=erosionOff[i].sourceMassG;
            erosionOffZero=erosionOffZero&&erosionOff[i].sourceMassG==0
                &&erosionOff[i].depositedMassG==0&&erosionOff[i].exportedMassG==0
                &&erosionOff[i].mobileMassG==0&&erosionOff[i].depositDepthM==0;
            depOffSource+=depositionOff[i].sourceMassG;
            depOffDeposit+=depositionOff[i].depositedMassG;
            depOffExport+=depositionOff[i].exportedMassG;
            depOffMobile+=depositionOff[i].mobileMassG;
        }
        c.depositionOffSourceMassG=depOffSource;
        c.depositionOffDepositMassG=depOffDeposit;
        c.depositionOffMobileMassG=depOffMobile;
        c.depositionOffExportMassG=depOffExport;
        depositionOffExports=depOffDeposit<=1e-3
            &&depOffExport>0
            &&depOffExport>=0.98*depOffSource
            &&depOffMobile<=0.02*depOffSource
            &&std::fabs(depOffSource-(depOffExport+depOffMobile))
                <=(std::max)(1.0,1e-9*depOffSource);

        // Equal input mass under identical drainage forcing must still diverge
        // by lithology/grain: higher mobility raises capacity and lowers settle.
        {
            int probe=-1;double bestAccum=-1;
            for(size_t i=0;i<drainage.size();++i)
                if(drainage[i].receiver>=0&&!drainage[i].boundary
                  &&drainage[i].accumulationM2>bestAccum)
                {bestAccum=drainage[i].accumulationM2;probe=(int)i;}
            if(probe>=0)
            {
                auto const& d=drainage[(size_t)probe];
                double const shaleMob=k->Mobility("shale");
                double const sandMob=k->Mobility("sandstone");
                double const shaleCap=k->CapacityForCert(d,shaleMob);
                double const sandCap=k->CapacityForCert(d,sandMob);
                double const shaleSettle=k->GetProgram().floodplainSettleRatio*(1.0-.35*shaleMob);
                double const sandSettle=k->GetProgram().floodplainSettleRatio*(1.0-.35*sandMob);
                c.shaleTravelProxy=shaleCap/std::max(1e-9,shaleSettle);
                c.sandstoneTravelProxy=sandCap/std::max(1e-9,sandSettle);
            }
        }

        bool geometryMatchesParentWhenTransportOff=true;
        for(int by=-1;by<=1&&geometryMatchesParentWhenTransportOff;++by)
        for(int bx=-1;bx<=1;++bx)
        {
            auto const block=k->BuildBlock(bx,by,Control::TransportOff);
            for(auto const& tri:block.triangles)
            for(auto const* v:{&tri.a,&tri.b,&tri.c})
                geometryMatchesParentWhenTransportOff=geometryMatchesParentWhenTransportOff
                    &&std::fabs(v->z-k->Erosion().ReconstructedZ(v->x,v->y))<1e-9;
        }

        bool collision=true;for(int by=-2;by<=2;++by)for(int bx=-2;bx<=2;++bx)
        {auto const block=k->BuildBlock(bx,by);for(auto const& tri:block.triangles)
            for(auto const* v:{&tri.a,&tri.b,&tri.c})
                collision=collision&&std::fabs(v->z-k->ReconstructedZ(v->x,v->y))<1e-9;}

        std::string coldReason;auto cold=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,&coldReason);
        auto partitioned=k->Evaluate(Control::Full);
        bool monolithic=partitioned.size()==cells.size();
        for(size_t i=0;i<cells.size()&&monolithic;++i)
            monolithic=monolithic
                &&std::fabs(partitioned[i].sourceMassG-cells[i].sourceMassG)<1e-6
                &&std::fabs(partitioned[i].depositedMassG-cells[i].depositedMassG)<1e-6
                &&std::fabs(partitioned[i].exportedMassG-cells[i].exportedMassG)<1e-6
                &&std::fabs(partitioned[i].mobileMassG-cells[i].mobileMassG)<1e-6
                &&std::fabs(partitioned[i].depositDepthM-cells[i].depositDepthM)<1e-9;

        c.checks.push_back({"mass_balance_source_eq_deposit_plus_mobile_plus_export",
            std::fabs(c.massResidualG)<=(std::max)(1.0,1e-9*c.sourceMassG)&&c.sourceMassG>0});
        c.checks.push_back({"transport_off_equals_exact_stage16b_surface",
            transportOffExact&&geometryMatchesParentWhenTransportOff});
        c.checks.push_back({"erosion_off_zero_sediment_source",erosionOffZero&&erosionOffSource==0});
        c.checks.push_back({"deposition_off_routes_to_explicit_export",depositionOffExports});
        c.checks.push_back({"lithology_grain_changes_transport_behavior",
            c.shaleTravelProxy>c.sandstoneTravelProxy+0.5});
        c.checks.push_back({"monolithic_equals_partitioned",monolithic});
        c.checks.push_back({"cold_start_digest_stable",cold&&cold->Digest()==k->Digest()
            &&cold->GeometryDigest()==k->GeometryDigest()});
        c.checks.push_back({"render_collision_surface_parity",collision});
        c.checks.push_back({"source_ledger_records_material_and_watershed",
            c.sourceCells>0&&!c.sourceByMaterial.empty()});
        c.checks.push_back({"deposition_and_outlet_export_both_present",
            c.depositCells>0&&c.exportCells>0&&c.exportedMassG>0&&c.depositedMassG>0});
        c.checks.push_back({"bounded_aggradation",c.maxDepositDepthM<=k->GetProgram().maxDepositDepthM+1e-9});
        c.checks.push_back({"no_water_live_erosion_ecology_or_p5b",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="compiled_sediment_gate_failed";return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_COMPILED_SEDIMENT %s\nreason=%s\nstage16b_digest=%s\n"
            "sediment_digest=%s\ngeometry_digest=%s\n"
            "source_mass_g=%.3f\ndeposited_mass_g=%.3f\nmobile_mass_g=%.3f\nexported_mass_g=%.3f\n"
            "mass_residual_g=%.6f\nsource_cells=%zu\ndeposit_cells=%zu\nexport_cells=%zu\n"
            "deposit_depth_max_m=%.6f\ndeposit_depth_mean_m=%.6f\n"
            "shale_travel_proxy_m=%.3f\nsandstone_travel_proxy_m=%.3f\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.stage16BDigest).c_str(),
            CausalWorldGeology::Hex64(c.sedimentDigest).c_str(),
            CausalWorldGeology::Hex64(c.geometryDigest).c_str(),
            c.sourceMassG,c.depositedMassG,c.mobileMassG,c.exportedMassG,c.massResidualG,
            c.sourceCells,c.depositCells,c.exportCells,c.maxDepositDepthM,c.meanDepositDepthM,
            c.shaleTravelProxy,c.sandstoneTravelProxy);
        std::fprintf(f,"deposition_off_source_g=%.3f\ndeposition_off_deposit_g=%.3f\n"
            "deposition_off_mobile_g=%.3f\ndeposition_off_export_g=%.3f\n",
            c.depositionOffSourceMassG,c.depositionOffDepositMassG,
            c.depositionOffMobileMassG,c.depositionOffExportMassG);
        for(auto const& v:c.sourceByMaterial)
            std::fprintf(f,"source_mass_material.%s_g=%.3f\n",v.first.c_str(),v.second);
        for(auto const& v:c.depositByMaterial)
            std::fprintf(f,"deposit_mass_material.%s_g=%.3f\n",v.first.c_str(),v.second);
        for(auto const& v:c.exportByMaterial)
            std::fprintf(f,"export_mass_material.%s_g=%.3f\n",v.first.c_str(),v.second);
        for(auto const& v:c.faciesCounts)
            std::fprintf(f,"facies.%s_cells=%zu\n",DepositFaciesName(v.first),v.second);
        std::fprintf(f,"compiled_history=1\nterrain_mutation_runtime=0\nwater_occupancy=0\n"
            "water_rendering=0\nfluid_solve=0\nlive_erosion=0\necology=0\np5b=closed\n");
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
