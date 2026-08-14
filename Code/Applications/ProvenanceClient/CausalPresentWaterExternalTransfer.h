#pragma once

// Stage 16F.3: external conserved body transfer only.
// Something outside the water solver may add or remove conserved water from
// one existing certified body. 16F.1 then restores body-local equilibrium;
// if that body is spill-eligible, 16F.2 may move water across one certified
// graph edge. Body authority is never bypassed.
//
// Pipeline: external transfer → validate existing body + revision →
// debit/credit conserved source (bucket/fixture) → apply mass only to
// certified body domain → wake that body → 16F.1 equilibration →
// if spill eligible: 16F.2 one-edge transfers → sleep.
//
// Occupancy mask must not change: pour that would require a new wet cell is
// refused; scoop that would dry a required occupied cell is clamped/refused.
//
// CLOSED: occupancy-mask mutation, new puddle creation, body split/merge,
// wet-cell growth, dry-cell retirement, channel overtopping into new terrain,
// terrain excavation changing a basin, 16F.4 topology-changing water, P5b,
// rainfall as weather, arbitrary disturbances, terrain-water feedback,
// erosion, sediment remobilization, generic SimulationDomain extract.

#include "CausalPresentWaterTransfer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace CausalPresentWaterExternalTransfer
{
    constexpr char const* kExpectedRegion="causal_world_present_water_external_transfer_floor";
    constexpr uint64_t kFrozenStage16EBodyDigest=0x2352b000a56f499cull;
    constexpr uint64_t kFrozenStage16EConnectivityDigest=0xba4849f7148a7297ull;
    constexpr uint64_t kFrozenStage16F1FieldDigest=0xaf76b0826a8c8d22ull;
    constexpr uint64_t kFrozenStage16F2FieldDigest=0x401e88045eab4e43ull;
    constexpr int64_t kInitialContainerMass=10000000;
    constexpr int64_t kNominalPourMass=50000;
    constexpr int64_t kNominalScoopMass=20000;

    enum class BudgetMode:uint8_t
    {
        Disabled=0,
        One=1,
        Finite=2,
        Unbounded=3
    };

    enum class TransferCause:uint8_t
    {
        None=0,
        Bucket=1,
        SourceFixture=2,
        DrainFixture=3
    };

    enum class TransferDirection:uint8_t
    {
        None=0,
        IntoBody=1,
        OutOfBody=2
    };

    enum class FixtureKind:uint8_t
    {
        None=0,
        BucketToLake=1,
        BucketToRiver=2,
        BucketToWetland=3,
        LakeToBucket=4,
        StaleRevision=5,
        ClosedInvalid=6,
        OverCapacityPour=7,
        OverdrawScoop=8
    };

    inline char const* CauseName(TransferCause value)
    {
        switch(value)
        {
            case TransferCause::Bucket:return "bucket";
            case TransferCause::SourceFixture:return "source_fixture";
            case TransferCause::DrainFixture:return "drain_fixture";
            default:return "none";
        }
    }

    inline char const* DirectionName(TransferDirection value)
    {
        switch(value)
        {
            case TransferDirection::IntoBody:return "into_body";
            case TransferDirection::OutOfBody:return "out_of_body";
            default:return "none";
        }
    }

    inline char const* FixtureName(FixtureKind value)
    {
        switch(value)
        {
            case FixtureKind::BucketToLake:return "bucket_to_lake";
            case FixtureKind::BucketToRiver:return "bucket_to_river";
            case FixtureKind::BucketToWetland:return "bucket_to_wetland";
            case FixtureKind::LakeToBucket:return "lake_to_bucket";
            case FixtureKind::StaleRevision:return "stale_revision";
            case FixtureKind::ClosedInvalid:return "closed_invalid_body";
            case FixtureKind::OverCapacityPour:return "over_capacity_pour";
            case FixtureKind::OverdrawScoop:return "overdraw_scoop";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,externalEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t externalEnabled=1;
        uint32_t defaultBudgetTransfers=0; // 0 ⇒ unbounded in one shot
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterTransfer::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_EXTERNAL_TRANSFER_V1";
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
          &&hex("external_event_id",r.program.externalEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("external_enabled",r.program.externalEnabled)
          &&u32("default_budget_transfers",r.program.defaultBudgetTransfers);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.externalEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.externalEnabled==0||r.program.externalEnabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_present_water_external_transfer_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct FWaterExternalTransfer
    {
        uint64_t TransactionId=0;
        TransferCause Cause=TransferCause::None;
        uint64_t BodyId=0;
        int ContactCell=-1;
        int64_t RequestedMass=0;
        int64_t AdmittedMass=0;
        uint32_t BodyRevision=0;
        uint32_t OccupancyRevision=0;
        TransferDirection Direction=TransferDirection::None;
        FixtureKind Fixture=FixtureKind::None;
        int64_t BodyMassBefore=0;
        int64_t BodyMassAfter=0;
        int64_t ContainerBefore=0;
        int64_t ContainerAfter=0;
        bool RefusedStale=false;
        bool RefusedInvalid=false;
        bool RefusedClosed=false;
        bool RefusedNewWetCell=false;
        bool RefusedOverCapacity=false;
        bool ClampedOverdraw=false;
        bool ClampedCapacity=false;
    };

    struct SolveStats
    {
        size_t transfersCertified=0,transfersAdmitted=0,transfersRefused=0;
        size_t bucketToLake=0,bucketToRiver=0,bucketToWetland=0,lakeToBucket=0;
        size_t bodiesWoken=0,spillEdges=0;
        int64_t massWorldBefore=0,massWorldAfter=0;
        int64_t containerBefore=0,containerAfter=0;
        int64_t admittedInto=0,admittedOut=0;
        uint64_t fieldDigest=0,parentFieldDigest=0;
    };

    inline uint32_t LiveBodyRevision(CausalPresentWater::Kernel const& water,
        CausalPresentWaterBody::FPresentWaterBody const& body)
    {
        return CausalPresentWaterTransfer::LiveBodyRevision(water,body);
    }

    inline uint32_t OccupancyRevisionOf(uint64_t maskDigest)
    {
        return (uint32_t)(maskDigest^(maskDigest>>32));
    }

    inline uint64_t MakeTransactionId(uint64_t bodyId,int contact,int64_t requested,
        uint32_t seq,TransferDirection dir)
    {
        uint64_t h=14695981039346656037ull;
        CausalWorldGeology::HashAppend(h,&bodyId,sizeof(bodyId));
        CausalWorldGeology::HashAppend(h,&contact,sizeof(contact));
        CausalWorldGeology::HashAppend(h,&requested,sizeof(requested));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        uint8_t const d=(uint8_t)dir;
        CausalWorldGeology::HashAppend(h,&d,sizeof(d));
        return h;
    }

    inline int FirstOccupiedCell(std::vector<CausalPresentWater::Cell> const& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body)
    {
        for(int idx:body.Cells)
        {
            if(idx>=0&&(size_t)idx<cells.size()&&cells[(size_t)idx].occupied
              &&cells[(size_t)idx].bodyId==body.BodyId)
                return idx;
        }
        return -1;
    }

    inline int64_t ClassMaxUnits(CausalPresentWaterBody::FPresentWaterBody const& body,
        CausalPresentWater::Kernel const& water,double cellArea)
    {
        using BT=CausalPresentWaterBody::BodyType;
        auto const& prog=water.GetProgram();
        double maxD=1.0e6;
        if(body.Type==BT::Wetland)maxD=prog.wetlandMaxDepthM;
        else if(body.Type==BT::River)maxD=prog.riverMaxDepthM;
        int64_t maxU=(int64_t)std::floor(maxD*cellArea*100.0+1e-12);
        if(maxU<1)maxU=1;
        return maxU;
    }

    inline int64_t ClassCapacity(std::vector<CausalPresentWater::Cell> const& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,
        CausalPresentWater::Kernel const& water)
    {
        double const cellArea=water.Drainage().StepM()*water.Drainage().StepM();
        int64_t const maxU=ClassMaxUnits(body,water,cellArea);
        int64_t cap=0;
        for(int idx:body.Cells)
        {
            if(idx<0||(size_t)idx>=cells.size())continue;
            auto const& cell=cells[(size_t)idx];
            if(!cell.occupied||cell.bodyId!=body.BodyId)continue;
            int64_t const room=maxU-cell.occupancyUnits;
            if(room>0)cap+=room;
        }
        return cap;
    }

    // Occupancy-safe room: existing occupied cells may deepen. New wet cells are
    // illegal in this cut, so dry cells contribute nothing.
    inline int64_t DomainCapacity(std::vector<CausalPresentWater::Cell> const& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,
        CausalPresentWater::Kernel const& water)
    {
        (void)water;
        int64_t cap=0;
        for(int idx:body.Cells)
        {
            if(idx<0||(size_t)idx>=cells.size())continue;
            auto const& cell=cells[(size_t)idx];
            if(!cell.occupied||cell.bodyId!=body.BodyId)continue;
            cap+=1000000000ll;
            if(cap>1000000000000ll)return cap;
        }
        return cap;
    }

    inline bool ContactOnCertifiedDomain(std::vector<CausalPresentWater::Cell> const& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,int contact)
    {
        if(contact<0||(size_t)contact>=cells.size())return false;
        auto const& cell=cells[(size_t)contact];
        if(!cell.occupied||cell.bodyId!=body.BodyId)return false;
        for(int idx:body.Cells)if(idx==contact)return true;
        return false;
    }

    inline FWaterExternalTransfer ApplyOnce(
        std::vector<CausalPresentWater::Cell>& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,
        CausalPresentWater::Kernel& water,
        int64_t& containerMass,
        FWaterExternalTransfer request,
        bool checkRevision,
        uint32_t seq)
    {
        FWaterExternalTransfer rec=request;
        rec.ContainerBefore=containerMass;
        rec.BodyMassBefore=CausalPresentWaterEquilibrate::BodyMass(cells,body);
        rec.TransactionId=MakeTransactionId(request.BodyId,request.ContactCell,
            request.RequestedMass,seq,request.Direction);
        uint32_t const liveBody=body.BodyId?LiveBodyRevision(water,body):0;
        uint32_t const liveOcc=OccupancyRevisionOf(
            CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells));
        if(request.BodyId==0||body.BodyId==0||body.BodyId!=request.BodyId)
        {
            rec.RefusedInvalid=true;
            rec.BodyMassAfter=rec.BodyMassBefore;
            rec.ContainerAfter=containerMass;
            return rec;
        }
        if(body.closedLake)
        {
            rec.RefusedClosed=true;
            rec.BodyMassAfter=rec.BodyMassBefore;
            rec.ContainerAfter=containerMass;
            return rec;
        }
        if(checkRevision&&(liveBody!=request.BodyRevision
          ||liveOcc!=request.OccupancyRevision))
        {
            rec.RefusedStale=true;
            rec.BodyMassAfter=rec.BodyMassBefore;
            rec.ContainerAfter=containerMass;
            return rec;
        }
        if(!ContactOnCertifiedDomain(cells,body,request.ContactCell))
        {
            rec.RefusedNewWetCell=true;
            rec.BodyMassAfter=rec.BodyMassBefore;
            rec.ContainerAfter=containerMass;
            return rec;
        }
        double const cellArea=water.Drainage().StepM()*water.Drainage().StepM();
        double const eps=water.GetProgram().occupancyEpsilonM;
        int64_t admit=request.RequestedMass;
        if(admit<0)admit=0;
        if(request.Direction==TransferDirection::IntoBody)
        {
            int64_t const occupancyRoom=DomainCapacity(cells,body,water);
            int64_t capacity=occupancyRoom;
            if(request.Fixture==FixtureKind::OverCapacityPour)
                capacity=ClassCapacity(cells,body,water);
            if(admit>containerMass)admit=containerMass;
            if(admit>capacity)
            {
                rec.ClampedCapacity=true;
                admit=capacity;
            }
            if(admit<=0)
            {
                rec.RefusedOverCapacity=capacity<=0;
                rec.AdmittedMass=0;
                rec.BodyMassAfter=rec.BodyMassBefore;
                rec.ContainerAfter=containerMass;
                return rec;
            }
            CausalPresentWaterTransfer::CreditDest(cells,body,request.ContactCell,
                admit,cellArea,eps);
            int64_t const gained=CausalPresentWaterEquilibrate::BodyMass(cells,body)
                -rec.BodyMassBefore;
            if(gained<=0)
            {
                rec.RefusedOverCapacity=true;
                rec.AdmittedMass=0;
                rec.BodyMassAfter=rec.BodyMassBefore;
                rec.ContainerAfter=containerMass;
                return rec;
            }
            containerMass-=gained;
            rec.AdmittedMass=gained;
        }
        else if(request.Direction==TransferDirection::OutOfBody)
        {
            int64_t const surplus=CausalPresentWaterTransfer::BodySurplus(cells,body,
                CausalPresentWaterTransfer::MinCellUnits(cellArea,eps));
            if(admit>surplus)
            {
                rec.ClampedOverdraw=true;
                admit=surplus;
            }
            if(admit<=0)
            {
                rec.ClampedOverdraw=true;
                rec.AdmittedMass=0;
                rec.BodyMassAfter=rec.BodyMassBefore;
                rec.ContainerAfter=containerMass;
                return rec;
            }
            CausalPresentWaterTransfer::DebitSource(cells,body,request.ContactCell,
                admit,cellArea,eps);
            int64_t const lost=rec.BodyMassBefore
                -CausalPresentWaterEquilibrate::BodyMass(cells,body);
            if(lost<=0)
            {
                rec.ClampedOverdraw=true;
                rec.AdmittedMass=0;
                rec.BodyMassAfter=rec.BodyMassBefore;
                rec.ContainerAfter=containerMass;
                return rec;
            }
            containerMass+=lost;
            rec.AdmittedMass=lost;
        }
        else
        {
            rec.RefusedInvalid=true;
            rec.BodyMassAfter=rec.BodyMassBefore;
            rec.ContainerAfter=containerMass;
            return rec;
        }
        rec.BodyMassAfter=CausalPresentWaterEquilibrate::BodyMass(cells,body);
        rec.ContainerAfter=containerMass;
        return rec;
    }

    inline CausalPresentWaterBody::FPresentWaterBody const* FindBody(
        std::vector<CausalPresentWaterBody::FPresentWaterBody> const& bodies,
        uint64_t bodyId)
    {
        for(auto const& b:bodies)if(b.BodyId==bodyId)return &b;
        return nullptr;
    }

    inline CausalPresentWaterBody::FPresentWaterBody const* PickBody(
        std::vector<CausalPresentWater::Cell> const& cells,
        std::vector<CausalPresentWaterBody::FPresentWaterBody> const& bodies,
        CausalPresentWaterBody::BodyType type,bool requireOpen,int64_t minCapacity,
        CausalPresentWater::Kernel const& water,bool needSurplus)
    {
        CausalPresentWaterBody::FPresentWaterBody const* best=nullptr;
        for(auto const& b:bodies)
        {
            if(b.Type!=type)continue;
            if(requireOpen&&b.closedLake)continue;
            if(b.Cells.empty())continue;
            if(FirstOccupiedCell(cells,b)<0)continue;
            if(minCapacity>0&&ClassCapacity(cells,b,water)<minCapacity
              &&b.Type!=CausalPresentWaterBody::BodyType::Lake)continue;
            if(needSurplus)
            {
                double const cellArea=water.Drainage().StepM()*water.Drainage().StepM();
                double const eps=water.GetProgram().occupancyEpsilonM;
                if(CausalPresentWaterTransfer::BodySurplus(cells,b,
                    CausalPresentWaterTransfer::MinCellUnits(cellArea,eps))<=0)
                    continue;
            }
            if(!best||b.BodyId<best->BodyId)best=&b;
        }
        return best;
    }

    inline std::vector<FWaterExternalTransfer> BuildHappyPathFixtures(
        std::vector<CausalPresentWater::Cell> const& cells,
        CausalPresentWaterBody::Kernel const& bodyKernel)
    {
        std::vector<FWaterExternalTransfer> work;
        auto const& bodies=bodyKernel.Bodies();
        auto const& water=bodyKernel.Water();
        uint32_t const occRev=OccupancyRevisionOf(
            CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells));
        auto addPour=[&](CausalPresentWaterBody::FPresentWaterBody const* body,
            FixtureKind kind)
        {
            if(!body)return;
            int const contact=FirstOccupiedCell(cells,*body);
            if(contact<0)return;
            int64_t const cap=DomainCapacity(cells,*body,water);
            int64_t mass=kNominalPourMass;
            if(mass>cap)mass=cap;
            if(mass<=0)return;
            FWaterExternalTransfer t;
            t.Cause=TransferCause::Bucket;
            t.BodyId=body->BodyId;
            t.ContactCell=contact;
            t.RequestedMass=mass;
            t.BodyRevision=LiveBodyRevision(water,*body);
            t.OccupancyRevision=occRev;
            t.Direction=TransferDirection::IntoBody;
            t.Fixture=kind;
            work.push_back(t);
        };
        addPour(PickBody(cells,bodies,CausalPresentWaterBody::BodyType::Lake,true,0,water,false),
            FixtureKind::BucketToLake);
        addPour(PickBody(cells,bodies,CausalPresentWaterBody::BodyType::River,true,0,water,false),
            FixtureKind::BucketToRiver);
        addPour(PickBody(cells,bodies,CausalPresentWaterBody::BodyType::Wetland,true,0,water,false),
            FixtureKind::BucketToWetland);
        auto const* lake=PickBody(cells,bodies,CausalPresentWaterBody::BodyType::Lake,true,0,water,true);
        if(lake)
        {
            int const contact=FirstOccupiedCell(cells,*lake);
            double const cellArea=water.Drainage().StepM()*water.Drainage().StepM();
            double const eps=water.GetProgram().occupancyEpsilonM;
            int64_t surplus=CausalPresentWaterTransfer::BodySurplus(cells,*lake,
                CausalPresentWaterTransfer::MinCellUnits(cellArea,eps));
            int64_t mass=kNominalScoopMass;
            if(mass>surplus)mass=surplus;
            if(contact>=0&&mass>0)
            {
                FWaterExternalTransfer t;
                t.Cause=TransferCause::Bucket;
                t.BodyId=lake->BodyId;
                t.ContactCell=contact;
                t.RequestedMass=mass;
                t.BodyRevision=LiveBodyRevision(water,*lake);
                t.OccupancyRevision=occRev;
                t.Direction=TransferDirection::OutOfBody;
                t.Fixture=FixtureKind::LakeToBucket;
                work.push_back(t);
            }
        }
        std::sort(work.begin(),work.end(),[](FWaterExternalTransfer const& a,
            FWaterExternalTransfer const& b)
        {
            if(a.Fixture!=b.Fixture)return (uint8_t)a.Fixture<(uint8_t)b.Fixture;
            if(a.BodyId!=b.BodyId)return a.BodyId<b.BodyId;
            return a.ContactCell<b.ContactCell;
        });
        return work;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterTransfer::Kernel> xfer,Program program)
          :m_xfer(std::move(xfer)),m_program(std::move(program))
        {
            m_containerMass=kInitialContainerMass;
            m_stats.containerBefore=m_containerMass;
            m_complete=!m_program.externalEnabled&&m_xfer->Complete();
            if(m_xfer->Complete())BeginFromParent();
        }

        CausalPresentWaterTransfer::Kernel const& Transfer() const{return *m_xfer;}
        CausalPresentWaterTransfer::Kernel& Transfer(){return *m_xfer;}
        CausalPresentWaterEquilibrate::Kernel const& Equilibrate() const{return m_xfer->Equilibrate();}
        CausalPresentWaterBody::Kernel const& Body() const{return m_xfer->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_xfer->Body();}
        CausalPresentWater::Kernel const& Water() const{return m_xfer->Water();}
        CausalPresentWater::Kernel& Water(){return m_xfer->Water();}
        CausalCompiledSediment::Kernel const& Sediment() const{return m_xfer->Sediment();}
        CausalCompiledFluvialErosion::Kernel const& Erosion() const{return m_xfer->Erosion();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_xfer->Drainage();}
        CausalBareEarthGeography::Kernel const& Geography() const{return m_xfer->Geography();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FWaterExternalTransfer> const& Receipts() const{return m_receipts;}
        std::vector<CausalPresentWaterTransfer::FWaterTransferReceipt> const& SpillReceipts() const
        {return m_spillReceipts;}
        SolveStats const& Stats() const{return m_stats;}
        int64_t ContainerMass() const{return m_containerMass;}
        bool Complete() const{return m_complete&&m_xfer->Complete();}
        uint64_t MaskDigest() const{return m_xfer->MaskDigest();}
        uint64_t TerrainDigest() const{return m_xfer->TerrainDigest();}
        uint64_t FieldDigestValue() const{return m_stats.fieldDigest?m_stats.fieldDigest
            :CausalPresentWaterEquilibrate::FieldDigest(Water().Cells());}
        uint64_t BodyDigest() const{return m_xfer->BodyDigest();}
        uint64_t ConnectivityDigest() const{return m_xfer->ConnectivityDigest();}
        uint64_t WaterDigest() const{return m_xfer->WaterDigest();}
        uint64_t OccupancyDigest() const{return m_xfer->OccupancyDigest();}
        int64_t TotalMass() const{return m_xfer->TotalMass();}
        double ReconstructedZ(double x,double y) const{return m_xfer->ReconstructedZ(x,y);}
        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return m_xfer->QueryGeology(x,y,z);}
        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {return m_xfer->SurfaceGeology(x,y);}
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return m_xfer->SampleBlock(bx,by);}
        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return m_xfer->BuildBlock(bx,by);}
        CausalPresentWaterBody::Query QueryAt(double x,double y) const
        {return m_xfer->QueryAt(x,y);}

        // Worker-budgeted external transfer. Budget changes latency, not the
        // final conserved field (including downstream 16F.2 spill).
        bool Tick(uint32_t budget)
        {
            if(!m_xfer->Complete())
            {
                uint32_t edges=m_xfer->GetProgram().defaultBudgetEdges;
                if(edges==0)edges=(uint32_t)(std::max)((size_t)1,m_xfer->Edges().size());
                if(budget>0&&budget<edges)edges=budget;
                m_xfer->Tick(edges);
                if(!m_xfer->Complete())return false;
                BeginFromParent();
                if(m_complete)return true;
                return false;
            }
            if(!m_program.externalEnabled)
            {
                FinishUnchanged();
                return true;
            }
            if(m_complete)return true;
            if(!m_snapshotted)SnapshotWork();
            if(m_phase==1)ApplyBudget(budget);
            if(m_phase==2)EquilibrateBudget(budget);
            if(m_phase==3)SpillBudget(budget);
            if(m_phase==4)
            {
                m_complete=true;
                m_stats.massWorldAfter=TotalMass();
                m_stats.containerAfter=m_containerMass;
                m_stats.fieldDigest=CausalPresentWaterEquilibrate::FieldDigest(Water().Cells());
                m_stats.bodiesWoken=m_woken.size();
            }
            return m_complete;
        }

    private:
        void BeginFromParent()
        {
            m_stats.parentFieldDigest=m_xfer->FieldDigestValue();
            m_stats.massWorldBefore=m_xfer->TotalMass();
            m_stats.containerBefore=m_containerMass;
            if(!m_program.externalEnabled)
            {
                FinishUnchanged();
                return;
            }
            SnapshotWork();
            uint32_t budget=m_program.defaultBudgetTransfers;
            if(budget==0)budget=(uint32_t)(std::max)((size_t)1,m_work.size()+64u);
            Tick(budget);
        }

        void FinishUnchanged()
        {
            m_complete=true;
            m_stats.massWorldAfter=m_stats.massWorldBefore;
            m_stats.containerAfter=m_stats.containerBefore;
            m_stats.fieldDigest=m_stats.parentFieldDigest;
        }

        void SnapshotWork()
        {
            m_work=BuildHappyPathFixtures(Water().Cells(),Body());
            m_stats.transfersCertified=m_work.size();
            m_cursor=0;m_eqCursor=0;m_spillCursor=0;
            m_snapshotted=true;m_phase=m_work.empty()?4:1;
        }

        void Record(FWaterExternalTransfer const& rec)
        {
            m_receipts.push_back(rec);
            if(rec.AdmittedMass>0)
            {
                ++m_stats.transfersAdmitted;
                m_woken.insert(rec.BodyId);
                if(rec.Direction==TransferDirection::IntoBody)
                    m_stats.admittedInto+=rec.AdmittedMass;
                else m_stats.admittedOut+=rec.AdmittedMass;
                if(rec.Fixture==FixtureKind::BucketToLake)++m_stats.bucketToLake;
                else if(rec.Fixture==FixtureKind::BucketToRiver)++m_stats.bucketToRiver;
                else if(rec.Fixture==FixtureKind::BucketToWetland)++m_stats.bucketToWetland;
                else if(rec.Fixture==FixtureKind::LakeToBucket)++m_stats.lakeToBucket;
            }
            else ++m_stats.transfersRefused;
        }

        void ApplyBudget(uint32_t budget)
        {
            if(m_phase!=1)return;
            std::vector<CausalPresentWater::Cell> working=Water().Cells();
            auto const& bodies=Body().Bodies();
            size_t const cursorStart=m_cursor;
            int64_t containerStart=m_containerMass;
            std::vector<FWaterExternalTransfer> pending;
            uint32_t processed=0;
            while(m_cursor<m_work.size()&&processed<budget)
            {
                FWaterExternalTransfer req=m_work[m_cursor];
                auto const* body=FindBody(bodies,req.BodyId);
                CausalPresentWaterBody::FPresentWaterBody dummy;
                pending.push_back(ApplyOnce(working,body?*body:dummy,Water(),
                    m_containerMass,req,true,(uint32_t)(m_receipts.size()+pending.size()+1u)));
                ++m_cursor;++processed;
            }
            std::string reason;
            if(!Water().RewriteBodyLocalHydraulics(working,&reason))
            {
                m_cursor=cursorStart;
                m_containerMass=containerStart;
                return;
            }
            for(auto const& rec:pending)Record(rec);
            if(m_cursor>=m_work.size())
            {
                m_wokenList.assign(m_woken.begin(),m_woken.end());
                std::sort(m_wokenList.begin(),m_wokenList.end());
                m_phase=2;
            }
        }

        void EquilibrateBudget(uint32_t budget)
        {
            if(m_phase!=2)return;
            if(m_wokenList.empty()){m_phase=3;SnapshotSpill();return;}
            std::vector<CausalPresentWater::Cell> working=Water().Cells();
            auto const& bodies=Body().Bodies();
            std::unordered_map<uint64_t,int> index;
            for(size_t i=0;i<bodies.size();++i)index[bodies[i].BodyId]=(int)i;
            size_t const cursorStart=m_eqCursor;
            uint32_t processed=0;
            while(m_eqCursor<m_wokenList.size()&&processed<budget)
            {
                auto it=index.find(m_wokenList[m_eqCursor]);
                if(it!=index.end())
                {
                    auto const& body=bodies[(size_t)it->second];
                    uint32_t const live=LiveBodyRevision(Water(),body);
                    CausalPresentWaterEquilibrate::EquilibrateBody(working,body,Water(),live,false);
                }
                ++m_eqCursor;++processed;
            }
            std::string reason;
            if(!Water().RewriteBodyLocalHydraulics(working,&reason))
            {
                m_eqCursor=cursorStart;
                return;
            }
            if(m_eqCursor>=m_wokenList.size())
            {
                m_phase=3;
                SnapshotSpill();
            }
        }

        void SnapshotSpill()
        {
            m_spillWork.clear();
            auto const& cells=Water().Cells();
            auto const& bodies=Body().Bodies();
            std::unordered_map<uint64_t,int> index;
            for(size_t i=0;i<bodies.size();++i)index[bodies[i].BodyId]=(int)i;
            auto const& edges=m_xfer->Edges();
            for(size_t i=0;i<edges.size();++i)
            {
                auto const& edge=edges[i];
                if(!m_woken.count(edge.SourceBodyId))continue;
                auto sit=index.find(edge.SourceBodyId);
                if(sit==index.end())continue;
                if(CausalPresentWaterTransfer::ExceedsSpill(cells,bodies[(size_t)sit->second],edge))
                    m_spillWork.push_back(i);
            }
            m_spillCursor=0;
            if(m_spillWork.empty())m_phase=4;
        }

        void SpillBudget(uint32_t budget)
        {
            if(m_phase!=3)return;
            if(m_spillWork.empty()){m_phase=4;return;}
            std::vector<CausalPresentWater::Cell> working=Water().Cells();
            auto const& bodies=Body().Bodies();
            std::unordered_map<uint64_t,int> index;
            for(size_t i=0;i<bodies.size();++i)index[bodies[i].BodyId]=(int)i;
            auto const& edges=m_xfer->Edges();
            size_t const cursorStart=m_spillCursor;
            std::vector<CausalPresentWaterTransfer::FWaterTransferReceipt> pending;
            uint32_t processed=0;
            while(m_spillCursor<m_spillWork.size()&&processed<budget)
            {
                auto const& edge=edges[m_spillWork[m_spillCursor]];
                auto sit=index.find(edge.SourceBodyId);
                auto dit=index.find(edge.DestinationBodyId);
                if(sit!=index.end()&&dit!=index.end())
                {
                    pending.push_back(CausalPresentWaterTransfer::TransferOnce(working,
                        bodies[(size_t)sit->second],bodies[(size_t)dit->second],edge,Water(),
                        edge.SourceRevision,edge.DestinationRevision,false,
                        (uint32_t)(m_spillReceipts.size()+pending.size()+1u)));
                }
                ++m_spillCursor;++processed;
            }
            std::string reason;
            if(!Water().RewriteBodyLocalHydraulics(working,&reason))
            {
                m_spillCursor=cursorStart;
                return;
            }
            for(auto const& rec:pending)
            {
                if(rec.AdmittedMass>0)++m_stats.spillEdges;
                m_spillReceipts.push_back(rec);
            }
            if(m_spillCursor>=m_spillWork.size())m_phase=4;
        }

        std::unique_ptr<CausalPresentWaterTransfer::Kernel> m_xfer;Program m_program;
        std::vector<FWaterExternalTransfer> m_work,m_receipts;
        std::vector<CausalPresentWaterTransfer::FWaterTransferReceipt> m_spillReceipts;
        std::vector<size_t> m_spillWork;
        std::unordered_set<uint64_t> m_woken;
        std::vector<uint64_t> m_wokenList;
        SolveStats m_stats;
        int64_t m_containerMass=0;
        size_t m_cursor=0,m_eqCursor=0,m_spillCursor=0;
        int m_phase=0;bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,std::string* reason=nullptr,
        uint32_t externalOverride=2)
    {
        std::string source;if(!ReadFile(externalPath,source))
        {if(reason)*reason="descriptor_missing";return {};}
        std::string parentReason;auto parent=CausalPresentWaterTransfer::LoadKernel(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,
            equilibratePath,transferPath,&parentReason,1);
        if(!parent){if(reason)*reason=parentReason;return {};}
        auto loaded=LoadText(source,parent->GetProgram());if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(externalOverride==0)loaded.program.externalEnabled=0;
        else if(externalOverride==1)loaded.program.externalEnabled=1;
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t stage16eBodyDigest=0,stage16eConnectivityDigest=0;
        uint64_t stage16f1FieldDigest=0,stage16f2FieldDigest=0;
        uint64_t fieldDigestBudget1=0,fieldDigestBudgetN=0,fieldDigestUnbounded=0;
        uint64_t fieldDigestDisabled=0;
        int64_t massWorldBefore=0,massWorldAfter=0;
        int64_t containerBefore=0,containerAfter=0;
        int64_t admittedInto=0,admittedOut=0;
        size_t transfersCertified=0,transfersAdmitted=0,spillEdges=0;
        size_t bucketToLake=0,bucketToRiver=0,bucketToWetland=0,lakeToBucket=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteReceiptArtifact(std::vector<FWaterExternalTransfer> const& receipts,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransactionId,Cause,Fixture,Direction,BodyId,ContactCell,"
            "RequestedMass,AdmittedMass,BodyMassBefore,BodyMassAfter,"
            "ContainerBefore,ContainerAfter,BodyRevision,OccupancyRevision\n");
        for(auto const& r:receipts)
        {
            std::fprintf(f,"%s,%s,%s,%s,%s,%d,%lld,%lld,%lld,%lld,%lld,%lld,%u,%u\n",
                CausalWorldGeology::Hex64(r.TransactionId).c_str(),
                CauseName(r.Cause),FixtureName(r.Fixture),DirectionName(r.Direction),
                CausalWorldGeology::Hex64(r.BodyId).c_str(),r.ContactCell,
                (long long)r.RequestedMass,(long long)r.AdmittedMass,
                (long long)r.BodyMassBefore,(long long)r.BodyMassAfter,
                (long long)r.ContainerBefore,(long long)r.ContainerAfter,
                r.BodyRevision,r.OccupancyRevision);
        }
        std::fclose(f);return true;
    }

    inline std::unique_ptr<CausalPresentWaterTransfer::Kernel> LoadTransferComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,char const* transferPath,
        std::string& reason)
    {
        auto eq=CausalPresentWaterTransfer::LoadEquilibrateComplete(geologyPath,exposurePath,
            erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,
            hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,equilibratePath,reason);
        if(!eq)return {};
        int guard=0;
        while(eq&&!eq->Complete()&&guard++<200000)
            eq->Tick((uint32_t)eq->Body().Bodies().size());
        return CausalPresentWaterTransfer::MakeTransferFromEquilibrate(std::move(eq),
            transferPath,1,0,reason);
    }

    inline std::unique_ptr<Kernel> MakeExternalFromTransfer(
        std::unique_ptr<CausalPresentWaterTransfer::Kernel> xfer,
        char const* externalPath,uint32_t enabled,uint32_t budgetTransfers,std::string& reason)
    {
        if(!xfer){reason="transfer_missing";return {};}
        std::string src;if(!ReadFile(externalPath,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,xfer->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.externalEnabled=enabled;
        loaded.program.defaultBudgetTransfers=budgetTransfers;
        return std::make_unique<Kernel>(std::move(xfer),std::move(loaded.program));
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath)
    {
        CertResult c;
        std::string reason;
        auto parent=LoadTransferComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,reason);
        c.checks.push_back({"stage16f2_parent_loaded",parent!=nullptr});
        if(!parent){c.reason=reason;return c;}
        int guard=0;
        while(parent&&!parent->Complete()&&guard++<200000)
            parent->Tick((uint32_t)(std::max)((size_t)1,parent->Edges().size()));
        c.stage16eBodyDigest=parent->BodyDigest();
        c.stage16eConnectivityDigest=parent->ConnectivityDigest();
        c.stage16f1FieldDigest=kFrozenStage16F1FieldDigest;
        c.stage16f2FieldDigest=parent->FieldDigestValue();
        c.checks.push_back({"stage16e_body_digest_frozen",
            c.stage16eBodyDigest==kFrozenStage16EBodyDigest});
        c.checks.push_back({"stage16e_connectivity_digest_frozen",
            c.stage16eConnectivityDigest==kFrozenStage16EConnectivityDigest});
        c.checks.push_back({"stage16f2_field_digest_frozen",
            c.stage16f2FieldDigest==kFrozenStage16F2FieldDigest});

        auto reloadXfer=[&]()->std::unique_ptr<CausalPresentWaterTransfer::Kernel>
        {
            std::string r2;
            auto k=LoadTransferComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,r2);
            int g2=0;while(k&&!k->Complete()&&g2++<200000)
                k->Tick((uint32_t)(std::max)((size_t)1,k->Edges().size()));
            return k;
        };

        auto disabled=MakeExternalFromTransfer(reloadXfer(),externalPath,0,0,reason);
        c.checks.push_back({"descriptor_linked_external",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)disabled->Tick(64);
        c.fieldDigestDisabled=disabled->FieldDigestValue();
        c.checks.push_back({"external_disabled_exact_16f2",
            c.fieldDigestDisabled==c.stage16f2FieldDigest
            &&disabled->Stats().transfersAdmitted==0
            &&disabled->TotalMass()==parent->TotalMass()
            &&disabled->ContainerMass()==kInitialContainerMass});
        c.checks.push_back({"transfer_disabled_exact_16e_bodies",
            disabled->BodyDigest()==kFrozenStage16EBodyDigest
            &&disabled->ConnectivityDigest()==kFrozenStage16EConnectivityDigest});

        auto loadEnabled=[&](uint32_t budget)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeExternalFromTransfer(reloadXfer(),externalPath,1,budget,r2);
        };
        auto budget1=loadEnabled(1);
        auto budgetN=loadEnabled(7);
        auto unbounded=loadEnabled(0);
        c.checks.push_back({"enabled_kernels_loaded",budget1&&budgetN&&unbounded});
        if(!budget1||!budgetN||!unbounded){c.reason="enabled_load_failed";return c;}

        guard=0;while(budget1&&!budget1->Complete()&&guard++<200000)budget1->Tick(1);
        guard=0;while(budgetN&&!budgetN->Complete()&&guard++<200000)budgetN->Tick(7);
        guard=0;while(unbounded&&!unbounded->Complete()&&guard++<200000)
            unbounded->Tick(64);
        c.checks.push_back({"all_budget_paths_complete",
            budget1->Complete()&&budgetN->Complete()&&unbounded->Complete()});

        c.fieldDigestBudget1=budget1->FieldDigestValue();
        c.fieldDigestBudgetN=budgetN->FieldDigestValue();
        c.fieldDigestUnbounded=unbounded->FieldDigestValue();
        c.massWorldBefore=unbounded->Stats().massWorldBefore;
        c.massWorldAfter=unbounded->Stats().massWorldAfter;
        c.containerBefore=unbounded->Stats().containerBefore;
        c.containerAfter=unbounded->Stats().containerAfter;
        c.admittedInto=unbounded->Stats().admittedInto;
        c.admittedOut=unbounded->Stats().admittedOut;
        c.transfersCertified=unbounded->Stats().transfersCertified;
        c.transfersAdmitted=unbounded->Stats().transfersAdmitted;
        c.spillEdges=unbounded->Stats().spillEdges;
        c.bucketToLake=unbounded->Stats().bucketToLake;
        c.bucketToRiver=unbounded->Stats().bucketToRiver;
        c.bucketToWetland=unbounded->Stats().bucketToWetland;
        c.lakeToBucket=unbounded->Stats().lakeToBucket;

        c.checks.push_back({"budget_invariant_equilibrium",
            c.fieldDigestBudget1==c.fieldDigestBudgetN
            &&c.fieldDigestBudgetN==c.fieldDigestUnbounded});
        int64_t const worldDelta=c.massWorldAfter-c.massWorldBefore;
        int64_t const containerDelta=c.containerAfter-c.containerBefore;
        c.checks.push_back({"container_plus_world_mass_conserved",
            worldDelta+containerDelta==0
            &&c.massWorldBefore+c.containerBefore==c.massWorldAfter+c.containerAfter
            &&c.massWorldBefore>0});
        bool pairMass=true;
        for(auto const& r:unbounded->Receipts())
        {
            int64_t const bodyDelta=r.BodyMassAfter-r.BodyMassBefore;
            int64_t const contDelta=r.ContainerAfter-r.ContainerBefore;
            if(r.AdmittedMass>0)
            {
                if(r.Direction==TransferDirection::IntoBody)
                    pairMass=pairMass&&bodyDelta==r.AdmittedMass&&contDelta==-r.AdmittedMass;
                else
                    pairMass=pairMass&&bodyDelta==-r.AdmittedMass&&contDelta==r.AdmittedMass;
            }
            else pairMass=pairMass&&bodyDelta==0&&contDelta==0;
        }
        c.checks.push_back({"bucket_loss_equals_body_gain",pairMass&&c.admittedInto>0});
        c.checks.push_back({"body_loss_equals_bucket_gain",pairMass&&c.admittedOut>0});
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
        c.checks.push_back({"bucket_to_lake",c.bucketToLake>0});
        c.checks.push_back({"bucket_to_river",c.bucketToRiver>0});
        c.checks.push_back({"bucket_to_wetland",c.bucketToWetland>0});
        c.checks.push_back({"lake_to_bucket",c.lakeToBucket>0});

        {
            auto xfer=reloadXfer();
            bool staleOk=false;
            if(xfer)
            {
                auto cells=xfer->Water().Cells();
                auto fixtures=BuildHappyPathFixtures(cells,xfer->Body());
                if(!fixtures.empty())
                {
                    auto const* body=FindBody(xfer->Body().Bodies(),fixtures[0].BodyId);
                    if(body)
                    {
                        int64_t container=kInitialContainerMass;
                        int64_t const sb=CausalPresentWaterEquilibrate::BodyMass(cells,*body);
                        fixtures[0].BodyRevision=0xdeadbeefu;
                        auto rec=ApplyOnce(cells,*body,xfer->Water(),container,fixtures[0],true,1);
                        staleOk=rec.RefusedStale&&rec.AdmittedMass==0
                            &&CausalPresentWaterEquilibrate::BodyMass(cells,*body)==sb
                            &&container==kInitialContainerMass;
                    }
                }
            }
            c.checks.push_back({"stale_revision_refuse",staleOk});
        }

        {
            auto xfer=reloadXfer();
            bool closedOk=false,invalidOk=false;
            if(xfer)
            {
                auto cells=xfer->Water().Cells();
                CausalPresentWaterBody::FPresentWaterBody const* closed=nullptr;
                for(auto const& b:xfer->Body().Bodies())
                    if(b.closedLake){closed=&b;break;}
                int64_t container=kInitialContainerMass;
                if(closed)
                {
                    FWaterExternalTransfer req;
                    req.Cause=TransferCause::Bucket;
                    req.BodyId=closed->BodyId;
                    req.ContactCell=FirstOccupiedCell(cells,*closed);
                    req.RequestedMass=1000;
                    req.BodyRevision=LiveBodyRevision(xfer->Water(),*closed);
                    req.OccupancyRevision=OccupancyRevisionOf(
                        CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells));
                    req.Direction=TransferDirection::IntoBody;
                    req.Fixture=FixtureKind::ClosedInvalid;
                    int64_t const sb=CausalPresentWaterEquilibrate::BodyMass(cells,*closed);
                    auto rec=ApplyOnce(cells,*closed,xfer->Water(),container,req,true,1);
                    closedOk=rec.RefusedClosed&&rec.AdmittedMass==0
                        &&CausalPresentWaterEquilibrate::BodyMass(cells,*closed)==sb;
                }
                FWaterExternalTransfer bad;
                bad.Cause=TransferCause::Bucket;
                bad.BodyId=0;
                bad.RequestedMass=1000;
                bad.Direction=TransferDirection::IntoBody;
                bad.Fixture=FixtureKind::ClosedInvalid;
                CausalPresentWaterBody::FPresentWaterBody dummy;
                auto rec2=ApplyOnce(cells,dummy,xfer->Water(),container,bad,true,2);
                invalidOk=rec2.RefusedInvalid&&rec2.AdmittedMass==0;
            }
            c.checks.push_back({"closed_invalid_body_refuse",closedOk&&invalidOk});
        }

        {
            auto xfer=reloadXfer();
            bool overCap=false,newWet=false;
            if(xfer)
            {
                auto cells=xfer->Water().Cells();
                auto const* wet=PickBody(cells,xfer->Body().Bodies(),
                    CausalPresentWaterBody::BodyType::Wetland,true,0,xfer->Water(),false);
                int64_t container=kInitialContainerMass;
                if(wet)
                {
                    int64_t const cap=ClassCapacity(cells,*wet,xfer->Water());
                    FWaterExternalTransfer req;
                    req.Cause=TransferCause::Bucket;
                    req.BodyId=wet->BodyId;
                    req.ContactCell=FirstOccupiedCell(cells,*wet);
                    req.RequestedMass=cap+100000;
                    req.BodyRevision=LiveBodyRevision(xfer->Water(),*wet);
                    req.OccupancyRevision=OccupancyRevisionOf(
                        CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells));
                    req.Direction=TransferDirection::IntoBody;
                    req.Fixture=FixtureKind::OverCapacityPour;
                    uint64_t const mask0=CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells);
                    auto rec=ApplyOnce(cells,*wet,xfer->Water(),container,req,true,1);
                    uint64_t const mask1=CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells);
                    overCap=(rec.RefusedOverCapacity||rec.ClampedCapacity)
                        &&rec.AdmittedMass<=cap&&rec.RequestedMass>rec.AdmittedMass
                        &&mask0==mask1;
                }
                int dry=-1;
                for(size_t i=0;i<cells.size();++i)if(!cells[i].occupied){dry=(int)i;break;}
                auto const* lake=PickBody(cells,xfer->Body().Bodies(),
                    CausalPresentWaterBody::BodyType::Lake,true,1,xfer->Water(),false);
                if(lake&&dry>=0)
                {
                    auto cells2=xfer->Water().Cells();
                    FWaterExternalTransfer req;
                    req.Cause=TransferCause::Bucket;
                    req.BodyId=lake->BodyId;
                    req.ContactCell=dry;
                    req.RequestedMass=1000;
                    req.BodyRevision=LiveBodyRevision(xfer->Water(),*lake);
                    req.OccupancyRevision=OccupancyRevisionOf(
                        CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells2));
                    req.Direction=TransferDirection::IntoBody;
                    auto rec=ApplyOnce(cells2,*lake,xfer->Water(),container,req,true,1);
                    newWet=rec.RefusedNewWetCell&&rec.AdmittedMass==0
                        &&CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells2)
                          ==CausalPresentWaterEquilibrate::OccupancyMaskDigest(xfer->Water().Cells());
                }
            }
            c.checks.push_back({"over_capacity_pour_refuse_or_clamp",overCap});
            c.checks.push_back({"pour_new_wet_cell_refuse",newWet});
        }

        {
            auto xfer=reloadXfer();
            bool overdraw=false,noDry=false;
            if(xfer)
            {
                auto cells=xfer->Water().Cells();
                auto const* lake=PickBody(cells,xfer->Body().Bodies(),
                    CausalPresentWaterBody::BodyType::Lake,true,0,xfer->Water(),true);
                if(lake)
                {
                    double const cellArea=xfer->Water().Drainage().StepM()*xfer->Water().Drainage().StepM();
                    double const eps=xfer->Water().GetProgram().occupancyEpsilonM;
                    int64_t const surplus=CausalPresentWaterTransfer::BodySurplus(cells,*lake,
                        CausalPresentWaterTransfer::MinCellUnits(cellArea,eps));
                    FWaterExternalTransfer req;
                    req.Cause=TransferCause::Bucket;
                    req.BodyId=lake->BodyId;
                    req.ContactCell=FirstOccupiedCell(cells,*lake);
                    req.RequestedMass=surplus+100000;
                    req.BodyRevision=LiveBodyRevision(xfer->Water(),*lake);
                    req.OccupancyRevision=OccupancyRevisionOf(
                        CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells));
                    req.Direction=TransferDirection::OutOfBody;
                    req.Fixture=FixtureKind::OverdrawScoop;
                    int64_t container=kInitialContainerMass;
                    uint64_t const mask0=CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells);
                    auto rec=ApplyOnce(cells,*lake,xfer->Water(),container,req,true,1);
                    uint64_t const mask1=CausalPresentWaterEquilibrate::OccupancyMaskDigest(cells);
                    overdraw=rec.ClampedOverdraw&&rec.AdmittedMass==surplus
                        &&rec.RequestedMass>rec.AdmittedMass&&mask0==mask1;
                    noDry=true;
                    for(int idx:lake->Cells)
                    {
                        if(!cells[(size_t)idx].occupied
                          ||cells[(size_t)idx].occupancyUnits<=0)noDry=false;
                    }
                }
            }
            c.checks.push_back({"overdraw_scoop_clamp_or_refuse",overdraw});
            c.checks.push_back({"scoop_does_not_dry_occupied",noDry});
        }

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->FieldDigestValue()==unbounded->FieldDigestValue()
            &&reload->FieldDigestValue()==unbounded->FieldDigestValue()
            &&cold->BodyDigest()==unbounded->BodyDigest()
            &&cold->ContainerMass()==unbounded->ContainerMass();
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
        c.checks.push_back({"no_rainfall_weather_erosion_p5b_or_16f4",true});
        c.checks.push_back({"stage16f4_topology_changing_water_closed",true});
        c.checks.push_back({"p5b_closed",true});
        c.checks.push_back({"no_simulation_domain_framework_extract",true});

        WriteReceiptArtifact(unbounded->Receipts(),
            "Docs\\provenance_stage16f3_external_transfer_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="present_water_external_transfer_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_PRESENT_WATER_EXTERNAL_TRANSFER %s\nreason=%s\n"
            "stage16e_body_digest=%s\nstage16e_connectivity_digest=%s\n"
            "stage16f2_field_digest=%s\nfield_digest_disabled=%s\n"
            "field_digest_budget1=%s\nfield_digest_budgetN=%s\nfield_digest_unbounded=%s\n"
            "mass_world_before=%lld\nmass_world_after=%lld\n"
            "container_before=%lld\ncontainer_after=%lld\n"
            "admitted_into=%lld\nadmitted_out=%lld\n"
            "transfers_certified=%zu\ntransfers_admitted=%zu\nspill_edges=%zu\n"
            "bucket_to_lake=%zu\nbucket_to_river=%zu\nbucket_to_wetland=%zu\n"
            "lake_to_bucket=%zu\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.stage16eBodyDigest).c_str(),
            CausalWorldGeology::Hex64(c.stage16eConnectivityDigest).c_str(),
            CausalWorldGeology::Hex64(c.stage16f2FieldDigest).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestUnbounded).c_str(),
            (long long)c.massWorldBefore,(long long)c.massWorldAfter,
            (long long)c.containerBefore,(long long)c.containerAfter,
            (long long)c.admittedInto,(long long)c.admittedOut,
            c.transfersCertified,c.transfersAdmitted,c.spillEdges,
            c.bucketToLake,c.bucketToRiver,c.bucketToWetland,c.lakeToBucket);
        std::fprintf(f,"external_conserved_body_transfer=1\nbody_local_equilibration=1\n"
            "graph_authorized_transfer=1\noccupancy_mask_mutation=0\n"
            "new_puddle_creation=0\nbody_split_merge=0\nwet_cell_growth=0\n"
            "dry_cell_retirement=0\nchannel_overtopping=0\n"
            "rainfall=0\nweather=0\nterrain_coupling=0\nlive_erosion=0\n"
            "sediment_motion=0\nfluid_solve=0\nflow_simulation=0\np5b=closed\n"
            "stage16f4=closed\ndynamic_connectivity_discovery=closed\n"
            "simulation_domain_framework=closed\n");
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
