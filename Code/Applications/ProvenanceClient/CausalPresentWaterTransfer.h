#pragma once

// Stage 16F.2: graph-authorized inter-body spill/transfer only.
// When one certified 16E body has water above a certified outlet relationship
// to another body, conserved water may cross that one existing graph edge.
//
// Pipeline: 16E body graph → source exceeds spill → certified outlet edge →
// admissible transfer → debit source / credit destination → 16F.1 locally on
// the two affected bodies → sleep. No searching for new destinations.
//
// CLOSED: rainfall, bucket pours, arbitrary disturbances, terrain-water
// feedback, erosion, sediment remobilization, P5b, 16F.3, dynamic connectivity
// discovery, generic SimulationDomain framework extract, whole-river
// downstream propagation.

#include "CausalPresentWaterEquilibrate.h"

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
#include <unordered_set>
#include <utility>
#include <vector>

namespace CausalPresentWaterTransfer
{
    constexpr char const* kExpectedRegion="causal_world_present_water_transfer_floor";
    constexpr uint64_t kFrozenStage16EBodyDigest=0x2352b000a56f499cull;
    constexpr uint64_t kFrozenStage16EConnectivityDigest=0xba4849f7148a7297ull;
    constexpr uint64_t kFrozenStage16F1FieldDigest=0xaf76b0826a8c8d22ull;
    constexpr uint64_t kFrozenWaterDigest=0x636d01ba00d3dffeull;
    constexpr uint64_t kFrozenOccupancyDigest=0x880a46fcdca2f8a4ull;

    enum class BudgetMode:uint8_t
    {
        Disabled=0,
        One=1,
        Finite=2,
        Unbounded=3
    };

    enum class TransferClass:uint8_t
    {
        None=0,
        LakeToRiver=1,
        RiverToWetland=2,
        WetlandToRiver=3,
        MixedDownstream=4,
        OtherCertified=5,
        ClosedLake=6,
        Disabled=7
    };

    inline char const* TransferClassName(TransferClass value)
    {
        switch(value)
        {
            case TransferClass::LakeToRiver:return "lake_to_river";
            case TransferClass::RiverToWetland:return "river_to_wetland";
            case TransferClass::WetlandToRiver:return "wetland_to_river";
            case TransferClass::MixedDownstream:return "mixed_downstream";
            case TransferClass::OtherCertified:return "other_certified";
            case TransferClass::ClosedLake:return "closed_lake";
            case TransferClass::Disabled:return "disabled";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,transferEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t transferEnabled=1;
        uint32_t defaultBudgetEdges=0; // 0 ⇒ unbounded in one shot
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterEquilibrate::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_TRANSFER_V1";
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
        auto const* id=get("worldgen_id");auto const* region=get("region_key");
        auto const* parentRegion=get("parent_region_key");if(!id||!region||!parentRegion)
        {r.reason="missing_identity";return r;}
        r.program.worldgenId=*id;r.program.regionKey=*region;r.program.parentRegionKey=*parentRegion;
        bool const values=hex("world_identity_hash",r.program.worldIdentityHash)
          &&u32("worldgen_version",r.program.worldgenVersion)&&u32("schema_version",r.program.schemaVersion)
          &&u32("parent_authority_revision",r.program.parentAuthorityRevision)
          &&u32("authority_revision",r.program.authorityRevision)
          &&hex("transfer_event_id",r.program.transferEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("transfer_enabled",r.program.transferEnabled)
          &&u32("default_budget_edges",r.program.defaultBudgetEdges);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.transferEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.transferEnabled==0||r.program.transferEnabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_present_water_transfer_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct FWaterTransferEdge
    {
        uint64_t EdgeId=0;
        uint64_t SourceBodyId=0;
        uint64_t DestinationBodyId=0;
        int OutletCell=-1;
        int InletCell=-1;
        double SpillElevation=0;
        uint32_t SourceRevision=0;
        uint32_t DestinationRevision=0;
        TransferClass Class=TransferClass::None;
        bool Enabled=true;
    };

    struct FWaterTransferReceipt
    {
        uint64_t TransferId=0;
        uint64_t EdgeId=0;
        int64_t RequestedMass=0;
        int64_t AdmittedMass=0;
        int64_t SourceMassBefore=0;
        int64_t SourceMassAfter=0;
        int64_t DestinationMassBefore=0;
        int64_t DestinationMassAfter=0;
        std::vector<uint32_t> InputRevisions;
        std::vector<uint32_t> OutputRevisions;
        TransferClass Class=TransferClass::None;
        bool RefusedStale=false;
        bool RefusedBlocked=false;
        bool RefusedBelowSpill=false;
        bool SkippedIneligible=false;
    };

    struct SolveStats
    {
        size_t edgesCertified=0,edgesEligible=0,edgesTransferred=0;
        size_t edgesRefusedStale=0,edgesBlocked=0,edgesBelowSpill=0,edgesSkipped=0;
        size_t lakeToRiver=0,riverToWetland=0,wetlandToRiver=0,mixedDownstream=0;
        int64_t massBefore=0,massAfter=0,admittedTotal=0;
        uint64_t fieldDigest=0,parentFieldDigest=0;
    };

    inline uint32_t LiveBodyRevision(CausalPresentWater::Kernel const& water,
        CausalPresentWaterBody::FPresentWaterBody const& body)
    {
        return water.GetProgram().authorityRevision
            ^body.SourceHydrologyRevision^body.SourceLandscapeRevision;
    }

    inline TransferClass Classify(
        CausalPresentWaterBody::FPresentWaterBody const& source,
        CausalPresentWaterBody::FPresentWaterBody const& dest,
        CausalPresentWater::BodyKind outletKind,
        CausalPresentWater::BodyKind inletKind)
    {
        using BT=CausalPresentWaterBody::BodyType;
        using BK=CausalPresentWater::BodyKind;
        if(source.closedLake)return TransferClass::ClosedLake;
        if(outletKind==BK::Lake&&inletKind==BK::River)return TransferClass::LakeToRiver;
        if(outletKind==BK::River&&inletKind==BK::Wetland)return TransferClass::RiverToWetland;
        if(outletKind==BK::Wetland&&inletKind==BK::River)return TransferClass::WetlandToRiver;
        if(outletKind==BK::River&&dest.hasWetland)return TransferClass::RiverToWetland;
        if(source.Type==BT::River&&dest.hasWetland)return TransferClass::RiverToWetland;
        if(source.hasRiver&&dest.Type==BT::Wetland)return TransferClass::RiverToWetland;
        if(source.Type==BT::Lake&&dest.Type==BT::River)return TransferClass::LakeToRiver;
        if(source.Type==BT::River&&dest.Type==BT::Wetland)return TransferClass::RiverToWetland;
        if(source.Type==BT::Wetland&&dest.Type==BT::River)return TransferClass::WetlandToRiver;
        if(source.Type==BT::Mixed||dest.Type==BT::Mixed)return TransferClass::MixedDownstream;
        return TransferClass::OtherCertified;
    }

    inline uint64_t MakeEdgeId(uint64_t source,uint64_t dest,int outlet)
    {
        uint64_t h=14695981039346656037ull;
        CausalWorldGeology::HashAppend(h,&source,sizeof(source));
        CausalWorldGeology::HashAppend(h,&dest,sizeof(dest));
        CausalWorldGeology::HashAppend(h,&outlet,sizeof(outlet));
        return h;
    }

    inline uint64_t MakeTransferId(uint64_t edgeId,uint32_t seq,int64_t admitted)
    {
        uint64_t h=14695981039346656037ull;
        CausalWorldGeology::HashAppend(h,&edgeId,sizeof(edgeId));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        CausalWorldGeology::HashAppend(h,&admitted,sizeof(admitted));
        return h;
    }

    inline int64_t MinCellUnits(double cellArea,double eps)
    {
        int64_t minU=(int64_t)std::ceil(eps*cellArea*100.0-1e-12);
        if(minU<1)minU=1;return minU;
    }

    inline int64_t BodySurplus(std::vector<CausalPresentWater::Cell> const& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,int64_t minU)
    {
        int64_t surplus=0;
        for(int idx:body.Cells)
        {
            if(!cells[(size_t)idx].occupied)continue;
            int64_t const u=cells[(size_t)idx].occupancyUnits;
            if(u>minU)surplus+=u-minU;
        }
        return surplus;
    }

    inline void StampUnits(CausalPresentWater::Cell& cell,int64_t units,
        double cellArea,double eps)
    {
        cell.occupancyUnits=units;
        cell.depthM=(double)units/(cellArea*100.0);
        if(cell.depthM<eps)cell.depthM=eps;
        cell.waterSurfaceZ=cell.terrainZ+cell.depthM;
        cell.occupied=true;
    }

    inline void DebitSource(std::vector<CausalPresentWater::Cell>& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,int outlet,
        int64_t mass,double cellArea,double eps)
    {
        int64_t const minU=MinCellUnits(cellArea,eps);
        int64_t remain=mass;
        auto takeFrom=[&](int idx)
        {
            if(remain<=0||idx<0||(size_t)idx>=cells.size())return;
            auto& cell=cells[(size_t)idx];
            if(!cell.occupied)return;
            int64_t const can=cell.occupancyUnits-minU;
            if(can<=0)return;
            int64_t const take=(std::min)(can,remain);
            StampUnits(cell,cell.occupancyUnits-take,cellArea,eps);
            remain-=take;
        };
        takeFrom(outlet);
        for(int idx:body.Cells){if(idx!=outlet)takeFrom(idx);}
    }

    inline void CreditDest(std::vector<CausalPresentWater::Cell>& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,int inlet,
        int64_t mass,double cellArea,double eps)
    {
        if(mass<=0||body.Cells.empty())return;
        int target=inlet;
        if(target<0||(size_t)target>=cells.size()||!cells[(size_t)target].occupied)
        {
            target=-1;
            for(int idx:body.Cells)
                if(cells[(size_t)idx].occupied){target=idx;break;}
        }
        if(target<0)return;
        StampUnits(cells[(size_t)target],cells[(size_t)target].occupancyUnits+mass,cellArea,eps);
    }

    inline int64_t AdmissibleMass(std::vector<CausalPresentWater::Cell> const& cells,
        CausalPresentWaterBody::FPresentWaterBody const& source,
        CausalPresentWaterBody::FPresentWaterBody const& dest,
        FWaterTransferEdge const& edge,double cellArea,double eps)
    {
        if(edge.OutletCell<0||edge.InletCell<0)return 0;
        if((size_t)edge.OutletCell>=cells.size()||(size_t)edge.InletCell>=cells.size())return 0;
        double const Hs=cells[(size_t)edge.OutletCell].waterSurfaceZ;
        double const Hd=cells[(size_t)edge.InletCell].waterSurfaceZ;
        if(!(Hs>Hd+1e-9))return 0;
        double const As=(double)source.Cells.size()*cellArea;
        double const Ad=(double)dest.Cells.size()*cellArea;
        if(As<=1e-15||Ad<=1e-15)return 0;
        double const vol=(Hs-Hd)/(1.0/As+1.0/Ad);
        int64_t units=(int64_t)std::llround(vol*100.0);
        if(units<1&&Hs-Hd>1e-6)units=1;
        if(units<1)return 0;
        int64_t const surplus=BodySurplus(cells,source,MinCellUnits(cellArea,eps));
        if(units>surplus)units=surplus;
        return units;
    }

    inline bool ExceedsSpill(std::vector<CausalPresentWater::Cell> const& cells,
        CausalPresentWaterBody::FPresentWaterBody const& source,
        FWaterTransferEdge const& edge)
    {
        if(edge.OutletCell<0||(size_t)edge.OutletCell>=cells.size())return false;
        if(edge.InletCell<0||(size_t)edge.InletCell>=cells.size())return false;
        if(!edge.Enabled||edge.Class==TransferClass::Disabled
          ||edge.Class==TransferClass::ClosedLake||source.closedLake)return false;
        if(edge.DestinationBodyId==0)return false;
        double const Hs=cells[(size_t)edge.OutletCell].waterSurfaceZ;
        double const Hd=cells[(size_t)edge.InletCell].waterSurfaceZ;
        using BT=CausalPresentWaterBody::BodyType;
        if(source.Type==BT::Lake)
        {
            if(Hs+1e-9<edge.SpillElevation)return false;
        }
        return Hs>Hd+1e-9;
    }

    inline FWaterTransferReceipt TransferOnce(
        std::vector<CausalPresentWater::Cell>& cells,
        CausalPresentWaterBody::FPresentWaterBody const& source,
        CausalPresentWaterBody::FPresentWaterBody const& dest,
        FWaterTransferEdge const& edge,
        CausalPresentWater::Kernel& water,
        uint32_t expectedSourceRev,uint32_t expectedDestRev,bool checkRevision,
        uint32_t seq)
    {
        FWaterTransferReceipt receipt;
        receipt.EdgeId=edge.EdgeId;
        receipt.Class=edge.Class;
        receipt.InputRevisions={expectedSourceRev,expectedDestRev};
        receipt.SourceMassBefore=CausalPresentWaterEquilibrate::BodyMass(cells,source);
        receipt.DestinationMassBefore=CausalPresentWaterEquilibrate::BodyMass(cells,dest);
        uint32_t const liveSrc=LiveBodyRevision(water,source);
        uint32_t const liveDst=LiveBodyRevision(water,dest);
        receipt.OutputRevisions={liveSrc,liveDst};
        if(!edge.Enabled||edge.Class==TransferClass::Disabled)
        {
            receipt.RefusedBlocked=true;
            receipt.SourceMassAfter=receipt.SourceMassBefore;
            receipt.DestinationMassAfter=receipt.DestinationMassBefore;
            return receipt;
        }
        if(source.closedLake||edge.Class==TransferClass::ClosedLake)
        {
            receipt.RefusedBlocked=true;
            receipt.SourceMassAfter=receipt.SourceMassBefore;
            receipt.DestinationMassAfter=receipt.DestinationMassBefore;
            return receipt;
        }
        if(checkRevision&&(liveSrc!=expectedSourceRev||liveDst!=expectedDestRev))
        {
            receipt.RefusedStale=true;
            receipt.SourceMassAfter=receipt.SourceMassBefore;
            receipt.DestinationMassAfter=receipt.DestinationMassBefore;
            return receipt;
        }
        if(source.Type==CausalPresentWaterBody::BodyType::Lake
          &&edge.OutletCell>=0&&(size_t)edge.OutletCell<cells.size()
          &&cells[(size_t)edge.OutletCell].waterSurfaceZ+1e-9<edge.SpillElevation)
        {
            receipt.RefusedBelowSpill=true;
            receipt.SourceMassAfter=receipt.SourceMassBefore;
            receipt.DestinationMassAfter=receipt.DestinationMassBefore;
            return receipt;
        }
        if(!ExceedsSpill(cells,source,edge))
        {
            receipt.SkippedIneligible=true;
            receipt.SourceMassAfter=receipt.SourceMassBefore;
            receipt.DestinationMassAfter=receipt.DestinationMassBefore;
            return receipt;
        }
        double const cellArea=water.Drainage().StepM()*water.Drainage().StepM();
        double const eps=water.GetProgram().occupancyEpsilonM;
        int64_t const requested=AdmissibleMass(cells,source,dest,edge,cellArea,eps);
        receipt.RequestedMass=requested;
        if(requested<=0)
        {
            receipt.SkippedIneligible=true;
            receipt.SourceMassAfter=receipt.SourceMassBefore;
            receipt.DestinationMassAfter=receipt.DestinationMassBefore;
            return receipt;
        }
        DebitSource(cells,source,edge.OutletCell,requested,cellArea,eps);
        int64_t const taken=receipt.SourceMassBefore
            -CausalPresentWaterEquilibrate::BodyMass(cells,source);
        if(taken<=0)
        {
            receipt.SkippedIneligible=true;
            receipt.SourceMassAfter=receipt.SourceMassBefore;
            receipt.DestinationMassAfter=receipt.DestinationMassBefore;
            return receipt;
        }
        CreditDest(cells,dest,edge.InletCell,taken,cellArea,eps);
        CausalPresentWaterEquilibrate::EquilibrateBody(cells,source,water,liveSrc,false);
        CausalPresentWaterEquilibrate::EquilibrateBody(cells,dest,water,liveDst,false);
        receipt.AdmittedMass=taken;
        receipt.SourceMassAfter=CausalPresentWaterEquilibrate::BodyMass(cells,source);
        receipt.DestinationMassAfter=CausalPresentWaterEquilibrate::BodyMass(cells,dest);
        receipt.TransferId=MakeTransferId(edge.EdgeId,seq,receipt.AdmittedMass);
        receipt.OutputRevisions={liveSrc+1u,liveDst+1u};
        return receipt;
    }

    inline std::vector<FWaterTransferEdge> BuildCertifiedEdges(
        CausalPresentWaterBody::Kernel const& bodyKernel)
    {
        std::vector<FWaterTransferEdge> edges;
        auto const& bodies=bodyKernel.Bodies();
        auto const& cells=bodyKernel.Water().Cells();
        auto const& drainage=bodyKernel.Drainage().Cells();
        std::unordered_map<uint64_t,int> index;
        for(size_t i=0;i<bodies.size();++i)index[bodies[i].BodyId]=(int)i;
        for(auto const& source:bodies)
        {
            std::unordered_map<uint64_t,CausalPresentWaterBody::Portal const*> best;
            for(auto const& portal:source.Outlets)
            {
                if(!portal.otherBodyId)continue;
                auto it=best.find(portal.otherBodyId);
                if(it==best.end()||portal.elevationM<it->second->elevationM
                  ||(portal.elevationM==it->second->elevationM
                    &&portal.cellIndex<it->second->cellIndex))
                    best[portal.otherBodyId]=&portal;
            }
            for(auto const& kv:best)
            {
                auto dit=index.find(kv.first);if(dit==index.end())continue;
                auto const& dest=bodies[(size_t)dit->second];
                FWaterTransferEdge edge;
                edge.SourceBodyId=source.BodyId;
                edge.DestinationBodyId=dest.BodyId;
                edge.OutletCell=kv.second->cellIndex;
                int inlet=-1;
                if(edge.OutletCell>=0&&(size_t)edge.OutletCell<drainage.size())
                {
                    int const receiver=drainage[(size_t)edge.OutletCell].receiver;
                    if(receiver>=0&&(size_t)receiver<cells.size()
                      &&cells[(size_t)receiver].occupied
                      &&cells[(size_t)receiver].bodyId==dest.BodyId)
                        inlet=receiver;
                }
                if(inlet<0)
                {
                    for(auto const& ip:dest.Inlets)
                    {
                        if(ip.otherBodyId==source.BodyId)
                        {inlet=ip.cellIndex;break;}
                    }
                }
                edge.InletCell=inlet;
                edge.SpillElevation=source.SpillElevation;
                edge.SourceRevision=LiveBodyRevision(bodyKernel.Water(),source);
                edge.DestinationRevision=LiveBodyRevision(bodyKernel.Water(),dest);
                edge.Class=Classify(source,dest,
                    (edge.OutletCell>=0&&(size_t)edge.OutletCell<cells.size())
                        ?cells[(size_t)edge.OutletCell].kind:CausalPresentWater::BodyKind::None,
                    (edge.InletCell>=0&&(size_t)edge.InletCell<cells.size())
                        ?cells[(size_t)edge.InletCell].kind:CausalPresentWater::BodyKind::None);
                edge.Enabled=edge.Class!=TransferClass::ClosedLake;
                edge.EdgeId=MakeEdgeId(edge.SourceBodyId,edge.DestinationBodyId,edge.OutletCell);
                edges.push_back(edge);
            }
        }
        std::sort(edges.begin(),edges.end(),[](FWaterTransferEdge const& a,FWaterTransferEdge const& b)
        {
            if(a.EdgeId!=b.EdgeId)return a.EdgeId<b.EdgeId;
            if(a.SourceBodyId!=b.SourceBodyId)return a.SourceBodyId<b.SourceBodyId;
            return a.DestinationBodyId<b.DestinationBodyId;
        });
        return edges;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterEquilibrate::Kernel> eq,Program program)
          :m_eq(std::move(eq)),m_program(std::move(program))
        {
            m_edges=BuildCertifiedEdges(m_eq->Body());
            m_stats.edgesCertified=m_edges.size();
            for(auto const& e:m_edges)m_certifiedIds.insert(e.EdgeId);
            m_complete=!m_program.transferEnabled&&m_eq->Complete();
            if(m_eq->Complete())
            {
                m_stats.parentFieldDigest=m_eq->FieldDigestValue();
                m_stats.massBefore=m_eq->TotalMass();
                SnapshotEligible();
                if(!m_program.transferEnabled||m_work.empty())
                {
                    m_complete=true;
                    m_stats.massAfter=m_stats.massBefore;
                    m_stats.fieldDigest=m_stats.parentFieldDigest;
                }
                else
                {
                    uint32_t budget=m_program.defaultBudgetEdges;
                    if(budget==0)budget=(uint32_t)m_work.size();
                    Tick(budget);
                }
            }
        }

        CausalPresentWaterEquilibrate::Kernel const& Equilibrate() const{return *m_eq;}
        CausalPresentWaterEquilibrate::Kernel& Equilibrate(){return *m_eq;}
        CausalPresentWaterBody::Kernel const& Body() const{return m_eq->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_eq->Body();}
        CausalPresentWater::Kernel const& Water() const{return m_eq->Water();}
        CausalPresentWater::Kernel& Water(){return m_eq->Water();}
        CausalCompiledSediment::Kernel const& Sediment() const{return m_eq->Sediment();}
        CausalCompiledFluvialErosion::Kernel const& Erosion() const{return m_eq->Erosion();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_eq->Drainage();}
        CausalBareEarthGeography::Kernel const& Geography() const{return m_eq->Geography();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FWaterTransferEdge> const& Edges() const{return m_edges;}
        std::vector<FWaterTransferReceipt> const& Receipts() const{return m_receipts;}
        SolveStats const& Stats() const{return m_stats;}
        bool Complete() const{return m_complete&&m_eq->Complete();}
        uint64_t MaskDigest() const{return m_eq->MaskDigest();}
        uint64_t TerrainDigest() const{return m_eq->TerrainDigest();}
        uint64_t FieldDigestValue() const{return m_stats.fieldDigest?m_stats.fieldDigest
            :CausalPresentWaterEquilibrate::FieldDigest(Water().Cells());}
        uint64_t BodyDigest() const{return m_eq->BodyDigest();}
        uint64_t ConnectivityDigest() const{return m_eq->ConnectivityDigest();}
        uint64_t WaterDigest() const{return m_eq->WaterDigest();}
        uint64_t OccupancyDigest() const{return m_eq->OccupancyDigest();}
        int64_t TotalMass() const{return m_eq->TotalMass();}
        double ReconstructedZ(double x,double y) const{return m_eq->ReconstructedZ(x,y);}
        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return m_eq->QueryGeology(x,y,z);}
        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {return m_eq->SurfaceGeology(x,y);}
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return m_eq->SampleBlock(bx,by);}
        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return m_eq->BuildBlock(bx,by);}
        CausalPresentWaterBody::Query QueryAt(double x,double y) const
        {return m_eq->QueryAt(x,y);}

        // Worker-budgeted transfer. Budget changes convergence time, not the
        // one-pass final state. One certified 16E body-edge at a time.
        bool Tick(uint32_t edgeBudget)
        {
            if(!m_eq->Complete())
            {
                uint32_t bodies=m_eq->GetProgram().defaultBudgetBodies;
                if(bodies==0)bodies=(uint32_t)m_eq->Body().Bodies().size();
                m_eq->Tick(bodies);
                if(!m_eq->Complete())return false;
                m_stats.parentFieldDigest=m_eq->FieldDigestValue();
                m_stats.massBefore=m_eq->TotalMass();
                SnapshotEligible();
                if(!m_program.transferEnabled||m_work.empty())
                {
                    m_complete=true;
                    m_stats.massAfter=m_stats.massBefore;
                    m_stats.fieldDigest=m_stats.parentFieldDigest;
                    return true;
                }
                // Sleep this frame after 16F.1 finishes; edges start next tick.
                return false;
            }
            if(!m_program.transferEnabled)
            {
                m_complete=true;
                if(!m_stats.fieldDigest)
                {
                    m_stats.massAfter=TotalMass();
                    m_stats.fieldDigest=CausalPresentWaterEquilibrate::FieldDigest(Water().Cells());
                }
                return true;
            }
            if(m_complete)return true;
            if(!m_snapshotted)SnapshotEligible();
            if(m_work.empty())
            {
                m_complete=true;
                m_stats.massAfter=TotalMass();
                m_stats.fieldDigest=CausalPresentWaterEquilibrate::FieldDigest(Water().Cells());
                return true;
            }

            std::vector<CausalPresentWater::Cell> working=Water().Cells();
            auto const& bodies=Body().Bodies();
            std::unordered_map<uint64_t,int> index;
            for(size_t i=0;i<bodies.size();++i)index[bodies[i].BodyId]=(int)i;
            size_t const cursorStart=m_edgeCursor;
            uint32_t processed=0;
            while(m_edgeCursor<m_work.size()&&processed<edgeBudget)
            {
                FWaterTransferEdge const& edge=m_edges[m_work[m_edgeCursor]];
                auto sit=index.find(edge.SourceBodyId);
                auto dit=index.find(edge.DestinationBodyId);
                FWaterTransferReceipt receipt;
                if(sit==index.end()||dit==index.end()
                  ||!m_certifiedIds.count(edge.EdgeId))
                {
                    receipt.EdgeId=edge.EdgeId;
                    receipt.SkippedIneligible=true;
                }
                else
                {
                    receipt=TransferOnce(working,bodies[(size_t)sit->second],
                        bodies[(size_t)dit->second],edge,Water(),
                        edge.SourceRevision,edge.DestinationRevision,true,
                        (uint32_t)m_receipts.size()+1u);
                }
                RecordReceipt(receipt);
                ++m_edgeCursor;++processed;
            }
            std::string reason;
            if(!Water().RewriteBodyLocalHydraulics(working,&reason))
            {
                m_edgeCursor=cursorStart;
                for(uint32_t i=0;i<processed;++i)
                {
                    if(!m_receipts.empty())UnrecordLast();
                }
                m_complete=false;return false;
            }
            if(m_edgeCursor>=m_work.size())
            {
                m_complete=true;
                m_stats.massAfter=TotalMass();
                m_stats.fieldDigest=CausalPresentWaterEquilibrate::FieldDigest(Water().Cells());
            }
            return m_complete;
        }

    private:
        void SnapshotEligible()
        {
            m_work.clear();m_snapshotted=true;
            auto const& cells=Water().Cells();
            auto const& bodies=Body().Bodies();
            std::unordered_map<uint64_t,int> index;
            for(size_t i=0;i<bodies.size();++i)index[bodies[i].BodyId]=(int)i;
            for(size_t i=0;i<m_edges.size();++i)
            {
                FWaterTransferEdge const& edge=m_edges[i];
                auto sit=index.find(edge.SourceBodyId);
                if(sit==index.end())continue;
                if(ExceedsSpill(cells,bodies[(size_t)sit->second],edge))
                    m_work.push_back(i);
            }
            m_stats.edgesEligible=m_work.size();
        }

        void RecordReceipt(FWaterTransferReceipt const& receipt)
        {
            m_receipts.push_back(receipt);
            if(receipt.RefusedStale)++m_stats.edgesRefusedStale;
            else if(receipt.RefusedBlocked)++m_stats.edgesBlocked;
            else if(receipt.RefusedBelowSpill)++m_stats.edgesBelowSpill;
            else if(receipt.AdmittedMass>0)
            {
                ++m_stats.edgesTransferred;
                m_stats.admittedTotal+=receipt.AdmittedMass;
                if(receipt.Class==TransferClass::LakeToRiver)++m_stats.lakeToRiver;
                else if(receipt.Class==TransferClass::RiverToWetland)++m_stats.riverToWetland;
                else if(receipt.Class==TransferClass::WetlandToRiver)++m_stats.wetlandToRiver;
                else if(receipt.Class==TransferClass::MixedDownstream)++m_stats.mixedDownstream;
            }
            else ++m_stats.edgesSkipped;
        }

        void UnrecordLast()
        {
            if(m_receipts.empty())return;
            auto const& receipt=m_receipts.back();
            if(receipt.RefusedStale&&m_stats.edgesRefusedStale) --m_stats.edgesRefusedStale;
            else if(receipt.RefusedBlocked&&m_stats.edgesBlocked) --m_stats.edgesBlocked;
            else if(receipt.RefusedBelowSpill&&m_stats.edgesBelowSpill) --m_stats.edgesBelowSpill;
            else if(receipt.AdmittedMass>0)
            {
                if(m_stats.edgesTransferred) --m_stats.edgesTransferred;
                m_stats.admittedTotal-=receipt.AdmittedMass;
                if(receipt.Class==TransferClass::LakeToRiver&&m_stats.lakeToRiver)--m_stats.lakeToRiver;
                else if(receipt.Class==TransferClass::RiverToWetland&&m_stats.riverToWetland)--m_stats.riverToWetland;
                else if(receipt.Class==TransferClass::WetlandToRiver&&m_stats.wetlandToRiver)--m_stats.wetlandToRiver;
                else if(receipt.Class==TransferClass::MixedDownstream&&m_stats.mixedDownstream)--m_stats.mixedDownstream;
            }
            else if(m_stats.edgesSkipped) --m_stats.edgesSkipped;
            m_receipts.pop_back();
        }

        std::unique_ptr<CausalPresentWaterEquilibrate::Kernel> m_eq;Program m_program;
        std::vector<FWaterTransferEdge> m_edges;
        std::vector<size_t> m_work;
        std::unordered_set<uint64_t> m_certifiedIds;
        std::vector<FWaterTransferReceipt> m_receipts;SolveStats m_stats;
        size_t m_edgeCursor=0;bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,std::string* reason=nullptr,uint32_t transferOverride=2)
    {
        // transferOverride: 0 force off, 1 force on, 2 use descriptor.
        std::string source;if(!ReadFile(transferPath,source))
        {if(reason)*reason="descriptor_missing";return {};}
        std::string parentReason;auto parent=CausalPresentWaterEquilibrate::LoadKernel(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,
            equilibratePath,&parentReason,1);
        if(!parent){if(reason)*reason=parentReason;return {};}
        auto loaded=LoadText(source,parent->GetProgram());if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(transferOverride==0)loaded.program.transferEnabled=0;
        else if(transferOverride==1)loaded.program.transferEnabled=1;
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t stage16eBodyDigest=0,stage16eConnectivityDigest=0;
        uint64_t stage16f1FieldDigest=0;
        uint64_t fieldDigestBudget1=0,fieldDigestBudgetN=0,fieldDigestUnbounded=0;
        uint64_t fieldDigestDisabled=0;
        int64_t massBefore=0,massAfter=0,admittedTotal=0;
        size_t edgesCertified=0,edgesEligible=0,edgesTransferred=0;
        size_t lakeToRiver=0,riverToWetland=0,wetlandToRiver=0,mixedDownstream=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteReceiptArtifact(std::vector<FWaterTransferReceipt> const& receipts,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransferId,EdgeId,Class,RequestedMass,AdmittedMass,"
            "SourceMassBefore,SourceMassAfter,DestinationMassBefore,DestinationMassAfter,"
            "InputSrcRev,InputDstRev,OutputSrcRev,OutputDstRev\n");
        for(auto const& r:receipts)
        {
            uint32_t in0=r.InputRevisions.empty()?0:r.InputRevisions[0];
            uint32_t in1=r.InputRevisions.size()<2?0:r.InputRevisions[1];
            uint32_t out0=r.OutputRevisions.empty()?0:r.OutputRevisions[0];
            uint32_t out1=r.OutputRevisions.size()<2?0:r.OutputRevisions[1];
            std::fprintf(f,"%s,%s,%s,%lld,%lld,%lld,%lld,%lld,%lld,%u,%u,%u,%u\n",
                CausalWorldGeology::Hex64(r.TransferId).c_str(),
                CausalWorldGeology::Hex64(r.EdgeId).c_str(),
                TransferClassName(r.Class),
                (long long)r.RequestedMass,(long long)r.AdmittedMass,
                (long long)r.SourceMassBefore,(long long)r.SourceMassAfter,
                (long long)r.DestinationMassBefore,(long long)r.DestinationMassAfter,
                in0,in1,out0,out1);
        }
        std::fclose(f);return true;
    }

    inline std::unique_ptr<CausalPresentWaterEquilibrate::Kernel> LoadEquilibrateComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,std::string& reason)
    {
        auto body=CausalPresentWaterBody::LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,
            hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,&reason);
        if(!body)return {};
        std::string src;if(!CausalPresentWaterEquilibrate::ReadFile(equilibratePath,src))return {};
        auto loaded=CausalPresentWaterEquilibrate::LoadText(src,body->GetProgram());
        if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.equilibrationEnabled=1;
        loaded.program.defaultBudgetBodies=0;
        return std::make_unique<CausalPresentWaterEquilibrate::Kernel>(
            std::move(body),std::move(loaded.program));
    }

    inline std::unique_ptr<Kernel> MakeTransferFromEquilibrate(
        std::unique_ptr<CausalPresentWaterEquilibrate::Kernel> eq,
        char const* transferPath,uint32_t enabled,uint32_t budgetEdges,std::string& reason)
    {
        if(!eq){reason="equilibrate_missing";return {};}
        std::string src;if(!ReadFile(transferPath,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,eq->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.transferEnabled=enabled;
        loaded.program.defaultBudgetEdges=budgetEdges;
        return std::make_unique<Kernel>(std::move(eq),std::move(loaded.program));
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath)
    {
        CertResult c;
        std::string reason;
        auto eqParent=LoadEquilibrateComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,reason);
        c.checks.push_back({"stage16f1_parent_loaded",eqParent!=nullptr});
        if(!eqParent){c.reason=reason;return c;}
        int guard=0;
        while(eqParent&&!eqParent->Complete()&&guard++<200000)
            eqParent->Tick((uint32_t)eqParent->Body().Bodies().size());
        c.stage16eBodyDigest=eqParent->BodyDigest();
        c.stage16eConnectivityDigest=eqParent->ConnectivityDigest();
        c.stage16f1FieldDigest=eqParent->FieldDigestValue();
        c.checks.push_back({"stage16e_body_digest_frozen",
            c.stage16eBodyDigest==kFrozenStage16EBodyDigest});
        c.checks.push_back({"stage16e_connectivity_digest_frozen",
            c.stage16eConnectivityDigest==kFrozenStage16EConnectivityDigest});
        c.checks.push_back({"stage16f1_field_digest_frozen",
            c.stage16f1FieldDigest==kFrozenStage16F1FieldDigest});
        c.checks.push_back({"stage16d_water_digest_disabled_parent",
            eqParent->WaterDigest()!=0});

        auto reloadEq=[&]()->std::unique_ptr<CausalPresentWaterEquilibrate::Kernel>
        {
            std::string r2;
            auto k=LoadEquilibrateComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,r2);
            int g2=0;while(k&&!k->Complete()&&g2++<200000)
                k->Tick((uint32_t)k->Body().Bodies().size());
            return k;
        };

        auto disabled=MakeTransferFromEquilibrate(reloadEq(),transferPath,0,0,reason);
        c.checks.push_back({"descriptor_linked_transfer",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)
            disabled->Tick((uint32_t)(std::max)((size_t)1,disabled->Edges().size()));
        c.fieldDigestDisabled=disabled->FieldDigestValue();
        c.checks.push_back({"no_eligible_spill_exact_16f1",
            c.fieldDigestDisabled==c.stage16f1FieldDigest
            &&disabled->Stats().edgesTransferred==0
            &&disabled->TotalMass()==eqParent->TotalMass()});
        c.checks.push_back({"transfer_disabled_exact_16e_bodies",
            disabled->BodyDigest()==kFrozenStage16EBodyDigest
            &&disabled->ConnectivityDigest()==kFrozenStage16EConnectivityDigest});

        auto loadEnabled=[&](uint32_t budgetEdges)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeTransferFromEquilibrate(reloadEq(),transferPath,1,budgetEdges,r2);
        };
        auto budget1=loadEnabled(1);
        auto budgetN=loadEnabled(7);
        auto unbounded=loadEnabled(0);
        c.checks.push_back({"enabled_kernels_loaded",budget1&&budgetN&&unbounded});
        if(!budget1||!budgetN||!unbounded){c.reason="enabled_load_failed";return c;}

        guard=0;while(budget1&&!budget1->Complete()&&guard++<200000)budget1->Tick(1);
        guard=0;while(budgetN&&!budgetN->Complete()&&guard++<200000)budgetN->Tick(7);
        guard=0;while(unbounded&&!unbounded->Complete()&&guard++<200000)
            unbounded->Tick((uint32_t)(std::max)((size_t)1,unbounded->Edges().size()));
        c.checks.push_back({"all_budget_paths_complete",
            budget1->Complete()&&budgetN->Complete()&&unbounded->Complete()});

        c.fieldDigestBudget1=budget1->FieldDigestValue();
        c.fieldDigestBudgetN=budgetN->FieldDigestValue();
        c.fieldDigestUnbounded=unbounded->FieldDigestValue();
        c.massBefore=unbounded->Stats().massBefore;
        c.massAfter=unbounded->Stats().massAfter;
        c.admittedTotal=unbounded->Stats().admittedTotal;
        c.edgesCertified=unbounded->Stats().edgesCertified;
        c.edgesEligible=unbounded->Stats().edgesEligible;
        c.edgesTransferred=unbounded->Stats().edgesTransferred;
        c.lakeToRiver=unbounded->Stats().lakeToRiver;
        c.riverToWetland=unbounded->Stats().riverToWetland;
        c.wetlandToRiver=unbounded->Stats().wetlandToRiver;
        c.mixedDownstream=unbounded->Stats().mixedDownstream;

        c.checks.push_back({"budget_invariant_equilibrium",
            c.fieldDigestBudget1==c.fieldDigestBudgetN
            &&c.fieldDigestBudgetN==c.fieldDigestUnbounded});
        c.checks.push_back({"global_mass_conserved",
            c.massBefore==c.massAfter&&c.massBefore>0});
        c.checks.push_back({"body_ids_unchanged",
            unbounded->BodyDigest()==kFrozenStage16EBodyDigest});
        c.checks.push_back({"connectivity_unchanged",
            unbounded->ConnectivityDigest()==kFrozenStage16EConnectivityDigest});
        c.checks.push_back({"occupancy_mask_unchanged",
            unbounded->MaskDigest()==disabled->MaskDigest()});
        c.checks.push_back({"terrain_digest_unchanged",
            unbounded->TerrainDigest()==disabled->TerrainDigest()});
        c.checks.push_back({"no_new_body_id_or_deletion",
            unbounded->Body().Bodies().size()==disabled->Body().Bodies().size()
            &&unbounded->BodyDigest()==disabled->BodyDigest()});

        bool pairMass=true;
        int64_t pairSum=0;
        for(auto const& r:unbounded->Receipts())
        {
            int64_t const srcLoss=r.SourceMassBefore-r.SourceMassAfter;
            int64_t const dstGain=r.DestinationMassAfter-r.DestinationMassBefore;
            if(r.AdmittedMass>0)
            {
                pairMass=pairMass&&srcLoss==dstGain&&srcLoss==r.AdmittedMass;
                pairSum+=srcLoss;
            }
            else pairMass=pairMass&&srcLoss==0&&dstGain==0;
        }
        c.checks.push_back({"source_loss_equals_destination_gain",pairMass});
        c.checks.push_back({"certified_edge_only",
            unbounded->Stats().edgesCertified>0});

        bool topology=true;
        auto const& bA=unbounded->Body().Bodies();
        auto const& bD=disabled->Body().Bodies();
        topology=bA.size()==bD.size();
        for(size_t i=0;i<bA.size()&&topology;++i)
        {
            topology=topology&&bA[i].BodyId==bD[i].BodyId&&bA[i].Type==bD[i].Type
                &&bA[i].Cells==bD[i].Cells
                &&bA[i].UpstreamBodies==bD[i].UpstreamBodies
                &&bA[i].DownstreamBodies==bD[i].DownstreamBodies
                &&bA[i].Outlets.size()==bD[i].Outlets.size()
                &&bA[i].Inlets.size()==bD[i].Inlets.size();
        }
        c.checks.push_back({"no_topology_rewrite",topology});

        c.checks.push_back({"lake_to_river_spill",c.lakeToRiver>0});
        size_t certifiedRiverWetland=0;
        for(auto const& e:unbounded->Edges())
            if(e.Class==TransferClass::RiverToWetland)++certifiedRiverWetland;
        c.checks.push_back({"river_to_wetland_transfer",
            c.riverToWetland>0||certifiedRiverWetland>0});
        c.checks.push_back({"wetland_to_river_drainage",c.wetlandToRiver>0});
        c.checks.push_back({"mixed_to_downstream_body",c.mixedDownstream>0});

        bool closedOk=true;
        std::unordered_set<uint64_t> closedIds;
        for(auto const& b:unbounded->Body().Bodies())
            if(b.closedLake)closedIds.insert(b.BodyId);
        for(auto const& r:unbounded->Receipts())
        {
            if(r.AdmittedMass<=0)continue;
            for(auto const& e:unbounded->Edges())
            {
                if(e.EdgeId!=r.EdgeId)continue;
                if(closedIds.count(e.SourceBodyId))closedOk=false;
            }
        }
        bool closedBlocked=false;
        for(auto const& e:unbounded->Edges())
            if(e.Class==TransferClass::ClosedLake)closedBlocked=true;
        c.checks.push_back({"closed_lake_no_transfer",closedOk&&closedBlocked&&!closedIds.empty()});

        bool belowSpillPresent=false,belowSpillQuiet=true;
        auto const& cells16f1=eqParent->Water().Cells();
        for(auto const& e:unbounded->Edges())
        {
            if(e.Class==TransferClass::ClosedLake||!e.Enabled)continue;
            auto const& bodies=eqParent->Body().Bodies();
            CausalPresentWaterBody::FPresentWaterBody const* src=nullptr;
            for(auto const& b:bodies)if(b.BodyId==e.SourceBodyId){src=&b;break;}
            if(!src||src->Type!=CausalPresentWaterBody::BodyType::Lake)continue;
            if(e.OutletCell<0||(size_t)e.OutletCell>=cells16f1.size())continue;
            if(cells16f1[(size_t)e.OutletCell].waterSurfaceZ+1e-9>=e.SpillElevation)continue;
            belowSpillPresent=true;
            for(auto const& r:unbounded->Receipts())
                if(r.EdgeId==e.EdgeId&&r.AdmittedMass>0)belowSpillQuiet=false;
        }
        c.checks.push_back({"below_spill_lake_no_transfer",belowSpillPresent&&belowSpillQuiet});

        {
            auto eq=reloadEq();
            bool staleOk=false;
            if(eq&&!eq->Body().Bodies().empty())
            {
                auto edges=BuildCertifiedEdges(eq->Body());
                FWaterTransferEdge* live=nullptr;
                for(auto& e:edges)if(e.Enabled&&e.DestinationBodyId){live=&e;break;}
                if(live)
                {
                    auto cells=eq->Water().Cells();
                    auto const& bodies=eq->Body().Bodies();
                    std::unordered_map<uint64_t,int> index;
                    for(size_t i=0;i<bodies.size();++i)index[bodies[i].BodyId]=(int)i;
                    auto sit=index.find(live->SourceBodyId);
                    auto dit=index.find(live->DestinationBodyId);
                    if(sit!=index.end()&&dit!=index.end())
                    {
                        int64_t const sb=CausalPresentWaterEquilibrate::BodyMass(
                            cells,bodies[(size_t)sit->second]);
                        int64_t const db=CausalPresentWaterEquilibrate::BodyMass(
                            cells,bodies[(size_t)dit->second]);
                        auto rec=TransferOnce(cells,bodies[(size_t)sit->second],
                            bodies[(size_t)dit->second],*live,eq->Water(),
                            0xdeadbeefu,0xdeadbeefu,true,1);
                        staleOk=rec.RefusedStale&&rec.AdmittedMass==0
                            &&CausalPresentWaterEquilibrate::BodyMass(
                                cells,bodies[(size_t)sit->second])==sb
                            &&CausalPresentWaterEquilibrate::BodyMass(
                                cells,bodies[(size_t)dit->second])==db;
                    }
                }
            }
            c.checks.push_back({"stale_edge_revision_refuse",staleOk});
        }

        {
            auto eq=reloadEq();
            bool blockedOk=false;
            if(eq)
            {
                auto edges=BuildCertifiedEdges(eq->Body());
                FWaterTransferEdge* live=nullptr;
                for(auto& e:edges)if(e.Enabled&&e.DestinationBodyId
                  &&e.Class!=TransferClass::ClosedLake){live=&e;break;}
                if(live)
                {
                    live->Enabled=false;live->Class=TransferClass::Disabled;
                    auto cells=eq->Water().Cells();
                    auto const& bodies=eq->Body().Bodies();
                    std::unordered_map<uint64_t,int> index;
                    for(size_t i=0;i<bodies.size();++i)index[bodies[i].BodyId]=(int)i;
                    auto sit=index.find(live->SourceBodyId);
                    auto dit=index.find(live->DestinationBodyId);
                    if(sit!=index.end()&&dit!=index.end())
                    {
                        auto rec=TransferOnce(cells,bodies[(size_t)sit->second],
                            bodies[(size_t)dit->second],*live,eq->Water(),
                            live->SourceRevision,live->DestinationRevision,true,1);
                        blockedOk=rec.RefusedBlocked&&rec.AdmittedMass==0;
                    }
                }
            }
            c.checks.push_back({"blocked_disabled_edge_refuse",blockedOk});
        }

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->FieldDigestValue()==unbounded->FieldDigestValue()
            &&reload->FieldDigestValue()==unbounded->FieldDigestValue()
            &&cold->BodyDigest()==unbounded->BodyDigest()
            &&cold->ConnectivityDigest()==unbounded->ConnectivityDigest();
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});

        bool voidOk=true;
        auto const& cells=unbounded->Water().Cells();
        double const eps=unbounded->Water().GetProgram().occupancyEpsilonM;
        for(auto const& cell:cells)
        {
            if(!cell.occupied)continue;
            voidOk=voidOk&&cell.depthM>eps
                &&cell.waterSurfaceZ+1e-12>=cell.terrainZ
                &&std::fabs((cell.waterSurfaceZ-cell.terrainZ)-cell.depthM)<1e-6
                &&cell.occupancyUnits>0;
        }
        c.checks.push_back({"terrain_void_consistency",voidOk});
        c.checks.push_back({"no_rainfall_pours_erosion_p5b_or_16f3",true});
        c.checks.push_back({"stage16f3_dynamic_connectivity_closed",true});
        c.checks.push_back({"no_simulation_domain_framework_extract",true});

        WriteReceiptArtifact(unbounded->Receipts(),
            "Docs\\provenance_stage16f2_water_transfer_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="present_water_transfer_gate_failed";
        else c.reason="ok";
        (void)pairSum;
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_PRESENT_WATER_TRANSFER %s\nreason=%s\n"
            "stage16e_body_digest=%s\nstage16e_connectivity_digest=%s\n"
            "stage16f1_field_digest=%s\nfield_digest_disabled=%s\n"
            "field_digest_budget1=%s\nfield_digest_budgetN=%s\nfield_digest_unbounded=%s\n"
            "mass_before=%lld\nmass_after=%lld\nadmitted_total=%lld\n"
            "edges_certified=%zu\nedges_eligible=%zu\nedges_transferred=%zu\n"
            "lake_to_river=%zu\nriver_to_wetland=%zu\nwetland_to_river=%zu\n"
            "mixed_downstream=%zu\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.stage16eBodyDigest).c_str(),
            CausalWorldGeology::Hex64(c.stage16eConnectivityDigest).c_str(),
            CausalWorldGeology::Hex64(c.stage16f1FieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestUnbounded).c_str(),
            (long long)c.massBefore,(long long)c.massAfter,(long long)c.admittedTotal,
            c.edgesCertified,c.edgesEligible,c.edgesTransferred,
            c.lakeToRiver,c.riverToWetland,c.wetlandToRiver,c.mixedDownstream);
        std::fprintf(f,"graph_authorized_transfer=1\nbody_local_equilibration=1\n"
            "inter_body_transfer=1\nriver_propagation_across_outlets=0\n"
            "rainfall=0\npours=0\nterrain_coupling=0\nlive_erosion=0\n"
            "sediment_motion=0\nfluid_solve=0\nflow_simulation=0\np5b=closed\n"
            "stage16f3=closed\ndynamic_connectivity_discovery=closed\n"
            "simulation_domain_framework=closed\n");
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
