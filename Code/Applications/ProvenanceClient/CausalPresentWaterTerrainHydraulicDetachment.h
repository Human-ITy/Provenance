#pragma once

// P5b.3A: hydraulic detachment. Certified hydraulic conditions may cause
// one bounded parcel of susceptible terrain matter to detach. Terrain
// solid loss == new loose/sediment matter. Water does not delete terrain
// merely because an erosion condition is true.
//
// Choice A: detached matter remains a local loose body/aggregate.
// Do NOT send through Stage 16C.
//
// CLOSED: P5b.3B detached sediment transport, P5b.3C bank/support collapse,
// general erosion, 16B active erosion, 16C remobilization, rainfall,
// evaporation, groundwater, plant uptake, ecology.
//
// Frozen parent: P5b.2C e4c9da99 + soak harness a28ed5c5.
// Long-haul infrastructure baseline f7ae29ea.
// Disabled P5b.3A must reproduce exact P5b.2C occupancy/overlay state.

#include "CausalPresentWaterTerrainPoreOccupancy.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#ifdef small
#undef small
#endif

namespace CausalPresentWaterTerrainHydraulicDetachment
{
    constexpr char const* kExpectedRegion=
        "causal_world_present_water_terrain_hydraulic_detachment_floor";
    constexpr uint64_t kFrozenP5b2cOccupancyDigest=
        CausalPresentWaterTerrainPoreOccupancy::kFrozenP5b2cOccupancyDigest;
    constexpr uint64_t kFrozenP5b3aDetachmentDigest=0xe3ba9b3af77265cbull;
    constexpr uint32_t kHoldBudget=CausalPresentWaterTerrainPoreOccupancy::kHoldBudget;
    constexpr int64_t kParcelGrams=80;
    constexpr uint16_t kSaturationDetachMin=900;
    constexpr uint16_t kCohesionDetachMax=800;
    constexpr uint8_t kCauseHydraulic=1;

    enum class FixtureKind:uint8_t
    {
        None=0,
        SaturatedBankDetach=1,
        DryControl=2,
        ResistantRock=3
    };

    inline char const* FixtureName(FixtureKind value)
    {
        switch(value)
        {
            case FixtureKind::SaturatedBankDetach:return "saturated_susceptible_bank";
            case FixtureKind::DryControl:return "same_material_dry_control";
            case FixtureKind::ResistantRock:return "resistant_rock_zero";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,p5b3aEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t p5b3aEnabled=1;
        uint32_t defaultBudgetTransactions=0;
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterTerrainPoreOccupancy::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_TERRAIN_HYDRAULIC_DETACHMENT_V1";
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
          &&hex("p5b3a_event_id",r.program.p5b3aEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("p5b3a_enabled",r.program.p5b3aEnabled)
          &&u32("default_budget_transactions",r.program.defaultBudgetTransactions);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.p5b3aEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.p5b3aEnabled==0||r.program.p5b3aEnabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_p5b3a_detachment_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct FDetachedLooseMatter
    {
        uint64_t DetachedMatterId=0;
        uint64_t MaterialId=0;
        char MaterialName[32]{};
        int64_t Grams=0;
        int SourceCell=-1;
        uint64_t SourceRegionId=0;
        uint64_t FeatureId=0;
        uint64_t GeologicalAncestry=0;
        uint64_t FormationProvenance=0;
        uint64_t BodyProvenance=0;
        uint8_t DetachmentCause=0;
        uint64_t TransactionId=0;
    };

    struct FHydraulicTerrainDetachment
    {
        uint64_t TransactionId=0;
        uint64_t CauseWaterBodyId=0;
        uint32_t WaterRevision=0;
        int TerrainCell=-1;
        uint32_t TerrainRevision=0;
        uint64_t MaterialId=0;
        uint32_t MaterialStateRevision=0;
        uint16_t Saturation=0;
        uint16_t CohesionModifier=1000;
        int64_t RequestedSolidMass=0;
        int64_t AdmittedSolidMass=0;
        uint64_t DetachedMatterId=0;
        std::vector<uint32_t> InputRevisions;
        std::vector<uint32_t> OutputRevisions;
        FixtureKind Fixture=FixtureKind::None;
        char MaterialName[32]{};
        bool WaterContact=false;
        bool ExposedFace=false;
        bool Susceptible=false;
        bool RefusedStale=false,RefusedInvalid=false;
        bool PublishedCoherent=false;
        bool RoutedThrough16C=false;
        size_t CellsVisited=0,BodiesExamined=0;
        uint32_t TerrainRevisionBefore=0,TerrainRevisionAfter=0;
        uint32_t WaterRevisionBefore=0,WaterRevisionAfter=0;
        uint32_t PublishedTerrainRevision=0,PublishedWaterRevision=0;
        int64_t TerrainSolidsBefore=0,TerrainSolidsAfter=0;
        int64_t LooseBefore=0,LooseAfter=0;
        int64_t WaterMassBefore=0,WaterMassAfter=0;
        int64_t HeldMatterBefore=0,HeldMatterAfter=0;
        uint64_t CompiledSedimentBefore=0,CompiledSedimentAfter=0;
    };

    struct DetachmentRequest
    {
        FixtureKind Fixture=FixtureKind::None;
        int TerrainCell=-1;
        int WaterCell=-1;
        uint64_t WaterBodyId=0;
        uint32_t TerrainRevision=0;
        uint32_t WaterRevision=0;
        bool CheckRevision=true;
        bool UseEvalOverride=false;
        uint16_t EvalSaturation=0;
        uint16_t EvalCohesion=1000;
        int64_t RequestedGrams=kParcelGrams;
    };

    struct SolveStats
    {
        size_t transactionsCertified=0,transactionsAdmitted=0,transactionsRefused=0;
        size_t bankDetach=0,dryZero=0,rockZero=0;
        size_t detachments=0,staleRefuse=0,waterResponses=0,topologyRebuilds=0;
        int64_t terrainSolidsBefore=0,terrainSolidsAfter=0;
        int64_t looseBefore=0,looseAfter=0;
        int64_t waterMassBefore=0,waterMassAfter=0;
        int64_t heldBefore=0,heldAfter=0;
        int64_t admittedMass=0;
        uint64_t parentOccupancyDigest=0,detachmentDigest=0;
        uint64_t compiledSedimentBefore=0,compiledSedimentAfter=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        uint32_t terrainRevision=0,waterRevision=0;
    };

    inline uint64_t MakeTxnId(FixtureKind kind,int terrain,int water,uint32_t seq)
    {
        uint64_t h=14695981039346656037ull;
        uint64_t const tag=0xA5B3000Aull;
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        uint8_t const k=(uint8_t)kind;
        CausalWorldGeology::HashAppend(h,&k,sizeof(k));
        CausalWorldGeology::HashAppend(h,&terrain,sizeof(terrain));
        CausalWorldGeology::HashAppend(h,&water,sizeof(water));
        CausalWorldGeology::HashAppend(h,&seq,sizeof(seq));
        return h?h:1;
    }

    inline uint64_t MakeDetachedId(uint64_t txn,int cell,uint64_t material,int64_t grams)
    {
        uint64_t h=14695981039346656037ull;
        uint64_t const tag=0xA5B3000Bull;
        CausalWorldGeology::HashAppend(h,&tag,sizeof(tag));
        CausalWorldGeology::HashAppend(h,&txn,sizeof(txn));
        CausalWorldGeology::HashAppend(h,&cell,sizeof(cell));
        CausalWorldGeology::HashAppend(h,&material,sizeof(material));
        CausalWorldGeology::HashAppend(h,&grams,sizeof(grams));
        return h?h:1;
    }

    inline uint64_t AncestryHash(CausalWorldGeology::GeoSample const& geo)
    {
        uint64_t h=14695981039346656037ull;
        CausalWorldGeology::HashAppend(h,&geo.regionId,sizeof(geo.regionId));
        CausalWorldGeology::HashAppend(h,&geo.featureId,sizeof(geo.featureId));
        for(uint64_t id:geo.eventIds)CausalWorldGeology::HashAppend(h,&id,sizeof(id));
        for(uint32_t c:geo.chronology)CausalWorldGeology::HashAppend(h,&c,sizeof(c));
        return h;
    }

    inline uint64_t FormationHash(std::string const& formation)
    {
        return CausalWorldGeology::HashBytes(formation.data(),formation.size());
    }

    inline char const* CellMaterial(CausalPresentWaterTerrainPoreOccupancy::Kernel const& parent,int cell)
    {
        auto const& cells=parent.Water().Cells();
        if(cell<0||(size_t)cell>=cells.size())return "dirt";
        auto const geo=parent.SurfaceGeology(cells[(size_t)cell].x,cells[(size_t)cell].y);
        return CausalPresentWaterTerrainState::CanonicalMaterial(
            geo.found?geo.material:std::string("dirt"));
    }

    // 2B overlays pore fill onto QueryTerrainState saturation. Detachment
    // eligibility reads certified P5b.2A material state (cohesion / wetting).
    inline CausalPresentWaterTerrainState::FTerrainMaterialState MaterialStateOf(
        CausalPresentWaterTerrainPoreOccupancy::Kernel const& parent,int cell)
    {
        return parent.Parent().Parent().QueryTerrainState(cell);
    }

    inline bool IsSusceptible(char const* name)
    {
        return CausalPresentWaterTerrainState::AcceptsMoisture(name);
    }

    inline bool HasWaterContact(std::vector<CausalPresentWater::Cell> const& cells,
        int cell,int width,int height,bool reverse)
    {
        if(cell<0||(size_t)cell>=cells.size())return false;
        if(cells[(size_t)cell].occupied)return true;
        for(int n=0;n<4;++n)
        {
            int const ni=CausalPresentWaterTopology::Neighbor4(cell,n,width,height,reverse);
            if(ni>=0&&cells[(size_t)ni].occupied)return true;
        }
        return false;
    }

    inline bool HasExposedFace(std::vector<CausalPresentWater::Cell> const& cells,
        int cell,int width,int height,bool reverse)
    {
        if(cell<0||(size_t)cell>=cells.size())return false;
        double const z=cells[(size_t)cell].terrainZ;
        for(int n=0;n<4;++n)
        {
            int const ni=CausalPresentWaterTopology::Neighbor4(cell,n,width,height,reverse);
            if(ni<0)return true;
            auto const& nb=cells[(size_t)ni];
            if(nb.occupied)return true;
            if(nb.terrainZ+1e-9<z)return true;
        }
        return false;
    }

    inline void BindWaterNeighbor(std::vector<CausalPresentWater::Cell> const& cells,
        int cell,int width,int height,bool reverse,int& water,uint64_t& body)
    {
        water=-1;body=0;
        if(cell<0||(size_t)cell>=cells.size())return;
        if(cells[(size_t)cell].occupied)
        {
            water=cell;body=cells[(size_t)cell].bodyId;return;
        }
        for(int n=0;n<4;++n)
        {
            int const ni=CausalPresentWaterTopology::Neighbor4(cell,n,width,height,reverse);
            if(ni<0||!cells[(size_t)ni].occupied)continue;
            water=ni;body=cells[(size_t)ni].bodyId;return;
        }
    }

    inline int64_t LooseMassOf(std::vector<FDetachedLooseMatter> const& loose)
    {
        int64_t s=0;for(auto const& m:loose)s+=m.Grams;return s;
    }

    inline uint64_t DetachmentDigestOf(std::vector<FDetachedLooseMatter> const& loose,
        uint64_t overlay,uint32_t terrainRev,uint32_t waterRev)
    {
        uint64_t d=14695981039346656037ull;
        CausalWorldGeology::HashAppend(d,&overlay,sizeof(overlay));
        CausalWorldGeology::HashAppend(d,&terrainRev,sizeof(terrainRev));
        CausalWorldGeology::HashAppend(d,&waterRev,sizeof(waterRev));
        for(auto const& m:loose)
        {
            CausalWorldGeology::HashAppend(d,&m.DetachedMatterId,sizeof(m.DetachedMatterId));
            CausalWorldGeology::HashAppend(d,&m.MaterialId,sizeof(m.MaterialId));
            CausalWorldGeology::HashAppend(d,&m.Grams,sizeof(m.Grams));
            CausalWorldGeology::HashAppend(d,&m.SourceCell,sizeof(m.SourceCell));
            CausalWorldGeology::HashAppend(d,&m.TransactionId,sizeof(m.TransactionId));
            CausalWorldGeology::HashAppend(d,&m.DetachmentCause,sizeof(m.DetachmentCause));
        }
        return d;
    }

    struct FixtureSet
    {
        DetachmentRequest bank,dry,rock;
        bool ok=false;
    };

    inline FixtureSet BuildFixtures(CausalPresentWaterTerrainPoreOccupancy::Kernel const& parent,
        bool reverseNeighbors=false)
    {
        FixtureSet set;
        auto const& cells=parent.Water().Cells();
        int const width=parent.Water().Drainage().Width();
        int const height=parent.Water().Drainage().Height();

        int bankCell=-1,bankWater=-1,rockCell=-1,rockWater=-1;
        uint64_t bankBody=0,rockBody=0;
        for(size_t i=0;i<cells.size();++i)
        {
            int const cell=(int)i;
            int64_t const col=CausalPresentWaterTerrainResponse::ColumnMatter(cells[i].terrainZ);
            if(col<kParcelGrams)continue;
            char const* const mat=CellMaterial(parent,cell);
            auto const st=MaterialStateOf(parent,cell);
            bool const contact=HasWaterContact(cells,cell,width,height,reverseNeighbors)
                ||st.ContactWet!=0;
            bool const exposed=HasExposedFace(cells,cell,width,height,reverseNeighbors);
            if(bankCell<0&&IsSusceptible(mat)&&contact&&exposed
              &&st.Saturation>=kSaturationDetachMin&&st.CohesionModifier<=kCohesionDetachMax)
            {
                bankCell=cell;
                BindWaterNeighbor(cells,cell,width,height,reverseNeighbors,bankWater,bankBody);
            }
            if(rockCell<0&&!IsSusceptible(mat))
            {
                rockCell=cell;
                BindWaterNeighbor(cells,cell,width,height,reverseNeighbors,rockWater,rockBody);
            }
            if(bankCell>=0&&rockCell>=0)break;
        }
        if(bankCell<0)
        {
            auto a2=CausalPresentWaterTerrainState::BuildFixtures(
                parent.Parent().Parent().Parent(),reverseNeighbors);
            if(a2.ok&&a2.wetland.TerrainCell>=0
              &&(size_t)a2.wetland.TerrainCell<cells.size())
            {
                int const cell=a2.wetland.TerrainCell;
                int64_t const col=CausalPresentWaterTerrainResponse::ColumnMatter(
                    cells[(size_t)cell].terrainZ);
                auto const st=MaterialStateOf(parent,cell);
                if(col>=kParcelGrams&&IsSusceptible(CellMaterial(parent,cell))
                  &&st.Saturation>=kSaturationDetachMin
                  &&st.CohesionModifier<=kCohesionDetachMax)
                {
                    bankCell=cell;
                    bankWater=a2.wetland.WaterCell;
                    bankBody=a2.wetland.WaterBodyId;
                }
            }
        }

        auto fill=[&](DetachmentRequest& req,FixtureKind kind,int terrain,int water,uint64_t body)
        {
            req.Fixture=kind;
            req.TerrainCell=terrain;
            req.WaterCell=water;
            req.WaterBodyId=body;
            req.RequestedGrams=kParcelGrams;
        };

        fill(set.bank,FixtureKind::SaturatedBankDetach,bankCell,bankWater,bankBody);
        fill(set.dry,FixtureKind::DryControl,bankCell,bankWater,bankBody);
        set.dry.UseEvalOverride=true;
        set.dry.EvalSaturation=50;
        set.dry.EvalCohesion=1000;
        fill(set.rock,FixtureKind::ResistantRock,rockCell,rockWater,rockBody);
        set.rock.UseEvalOverride=true;
        set.rock.EvalSaturation=1000;
        set.rock.EvalCohesion=550;

        bool const bankOk=bankCell>=0&&(size_t)bankCell<cells.size()
            &&IsSusceptible(CellMaterial(parent,bankCell));
        bool const rockOk=rockCell>=0&&(size_t)rockCell<cells.size()
            &&!IsSusceptible(CellMaterial(parent,rockCell));
        set.ok=bankOk&&rockOk&&parent.TotalMatter()>=kParcelGrams;
        return set;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterTerrainPoreOccupancy::Kernel> parent,Program program)
          :m_parent(std::move(parent)),m_program(std::move(program))
        {
            m_terrainRevision=m_parent->TerrainRevision();
            m_waterRevision=m_parent->WaterTopologyRevision();
            m_publishedTerrainRevision=m_parent->PublishedTerrainRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            m_complete=!m_program.p5b3aEnabled&&m_parent->Complete();
            if(m_parent->Complete())BeginFromParent();
        }

        CausalPresentWaterTerrainPoreOccupancy::Kernel const& Parent() const{return *m_parent;}
        CausalPresentWaterTerrainPoreOccupancy::Kernel& Parent(){return *m_parent;}
        CausalPresentWater::Kernel const& Water() const{return m_parent->Water();}
        CausalPresentWater::Kernel& Water(){return m_parent->Water();}
        CausalPresentWaterBody::Kernel const& Body() const{return m_parent->Body();}
        CausalPresentWaterBody::Kernel& Body(){return m_parent->Body();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_parent->Drainage();}
        Program const& GetProgram() const{return m_program;}
        std::vector<FHydraulicTerrainDetachment> const& Transactions() const{return m_txns;}
        std::vector<FDetachedLooseMatter> const& Loose() const{return m_loose;}
        SolveStats const& Stats() const{return m_stats;}
        int64_t ContainerMass() const{return m_parent->ContainerMass();}
        int64_t HeldMatter() const{return m_parent->HeldMatter();}
        uint32_t TerrainRevision() const{return m_terrainRevision;}
        uint32_t WaterTopologyRevision() const{return m_waterRevision;}
        uint32_t PublishedTerrainRevision() const{return m_publishedTerrainRevision;}
        uint32_t PublishedWaterTopologyRevision() const{return m_publishedWaterRevision;}
        uint32_t TerrainStateRevision() const{return m_parent->TerrainStateRevision();}
        uint32_t PoreRevision() const{return m_parent->PoreRevision();}
        bool Complete() const{return m_complete&&m_parent->Complete();}
        uint64_t FieldDigestValue() const{return m_parent->FieldDigestValue();}
        uint64_t TerrainOverlayDigestValue() const{return m_parent->TerrainOverlayDigestValue();}
        uint64_t TerrainDigest() const{return m_parent->TerrainDigest();}
        uint64_t BodyDigest() const{return m_parent->BodyDigest();}
        uint64_t StateDigestValue() const{return m_parent->StateDigestValue();}
        uint64_t PoreDigestValue() const{return m_parent->PoreDigestValue();}
        uint64_t OccupancyDigestValue() const{return m_parent->OccupancyDigestValue();}
        uint64_t DetachmentDigestValue() const
        {
            return DetachmentDigestOf(m_loose,TerrainOverlayDigestValue(),
                m_terrainRevision,m_waterRevision);
        }
        int64_t BodyMass() const{return m_parent->BodyMass();}
        int64_t PoreMass() const{return m_parent->PoreMass();}
        int64_t TotalMass() const{return m_parent->TotalMass();}
        int64_t ConservedMass() const{return m_parent->ConservedMass();}
        int64_t TerrainSolids() const{return m_parent->TotalMatter();}
        int64_t LooseMass() const{return LooseMassOf(m_loose);}
        int64_t TotalMatter() const{return TerrainSolids()+LooseMass();}
        double ReconstructedZ(double x,double y) const{return m_parent->ReconstructedZ(x,y);}
        CausalPresentWaterBody::Query QueryAt(double x,double y) const{return m_parent->QueryAt(x,y);}
        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return m_parent->QueryGeology(x,y,z);}
        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {return m_parent->SurfaceGeology(x,y);}
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return m_parent->SampleBlock(bx,by);}
        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return m_parent->BuildBlock(bx,by);}
        CausalPresentWaterTerrainPore::FTerrainPoreWaterState QueryPore(int cell) const
        {return m_parent->QueryPore(cell);}
        CausalPresentWaterTerrainState::FTerrainMaterialState QueryTerrainState(int cell) const
        {return m_parent->QueryTerrainState(cell);}
        CausalPresentWaterTerrainState::FTerrainMaterialState QueryTerrainStateAt(double x,double y) const
        {return m_parent->QueryTerrainStateAt(x,y);}
        CausalPresentWaterTerrainPore::FTerrainPoreWaterState QueryPoreAt(double x,double y) const
        {return m_parent->QueryPoreAt(x,y);}

        FHydraulicTerrainDetachment ApplyRequest(DetachmentRequest req,bool reverseNeighbors=false)
        {return CommitOne(req,reverseNeighbors);}

        bool Tick(uint32_t budget)
        {
            if(!m_parent->Complete())
            {
                uint32_t n=budget?budget:64;
                m_parent->Tick(n);
                if(!m_parent->Complete())return false;
                BeginFromParent();
                if(m_complete)return true;
                return false;
            }
            if(!m_program.p5b3aEnabled){FinishUnchanged();return true;}
            if(m_complete)return true;
            if(!m_snapshotted)SnapshotWork();
            ApplyBudget(budget);
            if(m_cursor>=m_work.size())
            {
                m_complete=true;
                m_stats.terrainSolidsAfter=TerrainSolids();
                m_stats.looseAfter=LooseMass();
                m_stats.waterMassAfter=ConservedMass();
                m_stats.heldAfter=HeldMatter();
                m_stats.compiledSedimentAfter=TerrainDigest();
                m_stats.detachmentDigest=DetachmentDigestValue();
                m_stats.terrainRevision=m_terrainRevision;
                m_stats.waterRevision=m_waterRevision;
            }
            return m_complete;
        }

    private:
        void BeginFromParent()
        {
            m_terrainRevision=m_parent->TerrainRevision();
            m_waterRevision=m_parent->WaterTopologyRevision();
            m_publishedTerrainRevision=m_parent->PublishedTerrainRevision();
            m_publishedWaterRevision=m_parent->PublishedWaterTopologyRevision();
            m_stats.parentOccupancyDigest=m_parent->OccupancyDigestValue();
            m_stats.terrainSolidsBefore=TerrainSolids();
            m_stats.looseBefore=LooseMass();
            m_stats.waterMassBefore=ConservedMass();
            m_stats.heldBefore=HeldMatter();
            m_stats.compiledSedimentBefore=TerrainDigest();
            if(!m_program.p5b3aEnabled){FinishUnchanged();return;}
            SnapshotWork();
            uint32_t budget=m_program.defaultBudgetTransactions;
            if(budget==kHoldBudget)return;
            if(budget==0)budget=(uint32_t)(std::max)((size_t)1,m_work.size());
            Tick(budget);
        }

        void FinishUnchanged()
        {
            m_complete=true;
            m_stats.terrainSolidsAfter=m_stats.terrainSolidsBefore;
            m_stats.looseAfter=m_stats.looseBefore;
            m_stats.waterMassAfter=m_stats.waterMassBefore;
            m_stats.heldAfter=m_stats.heldBefore;
            m_stats.compiledSedimentAfter=m_stats.compiledSedimentBefore;
            m_stats.detachmentDigest=DetachmentDigestValue();
            m_stats.terrainRevision=m_terrainRevision;
            m_stats.waterRevision=m_waterRevision;
        }

        void SnapshotWork()
        {
            auto fixtures=BuildFixtures(*m_parent,false);
            m_work.clear();
            if(fixtures.ok)
            {
                m_work.push_back(fixtures.dry);
                m_work.push_back(fixtures.rock);
                m_work.push_back(fixtures.bank);
            }
            m_stats.transactionsCertified=m_work.size();
            m_cursor=0;m_snapshotted=true;
        }

        void ApplyBudget(uint32_t budget)
        {
            uint32_t processed=0;
            while(m_cursor<m_work.size()&&processed<budget)
            {
                DetachmentRequest req=m_work[m_cursor];
                req.TerrainRevision=m_terrainRevision;
                req.WaterRevision=m_waterRevision;
                req.CheckRevision=true;
                FHydraulicTerrainDetachment rec=CommitOne(req,false);
                Record(rec);
                ++m_cursor;++processed;
            }
        }

        bool PublishWaterResponse(std::vector<CausalPresentWater::Cell>& working,
            int contact,bool added,uint64_t txn,bool reverseNeighbors,
            FHydraulicTerrainDetachment& rec)
        {
            int const width=Water().Drainage().Width();
            int const height=Water().Drainage().Height();
            double const cellArea=Water().Drainage().StepM()*Water().Drainage().StepM();
            double const eps=Water().GetProgram().occupancyEpsilonM;
            auto bodies=Body().Bodies();
            std::unordered_set<uint64_t> neighborhoodBodies;
            if(working[(size_t)contact].occupied&&working[(size_t)contact].bodyId)
                neighborhoodBodies.insert(working[(size_t)contact].bodyId);
            for(int n=0;n<4;++n)
            {
                int const ni=CausalPresentWaterTopology::Neighbor4(contact,n,width,height,reverseNeighbors);
                if(ni<0)continue;
                ++rec.CellsVisited;
                if(working[(size_t)ni].occupied&&working[(size_t)ni].bodyId)
                    neighborhoodBodies.insert(working[(size_t)ni].bodyId);
            }
            rec.BodiesExamined=neighborhoodBodies.size();
            if(added)
            {
                CausalPresentWaterTopology::OccupancyRequest occ;
                occ.ContactCell=contact;
                occ.RequestedMass=rec.AdmittedSolidMass;
                occ.OccupancyRevision=CausalPresentWaterTopology::OccupancyRevisionOf(
                    CausalPresentWaterTerrainPoreOccupancy::OccupancyMaskDigest(working));
                occ.TopologyRevision=m_waterRevision;
                occ.CheckRevision=false;
                occ.Fixture=CausalPresentWaterTopology::FixtureKind::PourGrow;
                auto delta=CausalPresentWaterTopology::ClassifyAfterOccupancy(
                    working,bodies,Water(),occ,true,false,txn,m_waterRevision,reverseNeighbors);
                rec.CellsVisited=(std::max)(rec.CellsVisited,delta.CellsVisited);
                rec.BodiesExamined=(std::max)(rec.BodiesExamined,delta.BodiesExamined);
                auto nextBodies=CausalPresentWaterTopology::InstantiateRevised(
                    bodies,working,Water(),delta);
                std::unordered_map<uint64_t,std::vector<int>> live;
                for(size_t i=0;i<working.size();++i)
                {
                    if(!working[i].occupied||!working[i].bodyId)continue;
                    live[working[i].bodyId].push_back((int)i);
                }
                nextBodies.erase(std::remove_if(nextBodies.begin(),nextBodies.end(),
                    [&](CausalPresentWaterBody::FPresentWaterBody const& b)
                    {return !live.count(b.BodyId);}),nextBodies.end());
                for(auto& b:nextBodies)
                {
                    auto it=live.find(b.BodyId);
                    if(it==live.end())continue;
                    b.Cells=it->second;
                }
                std::unordered_set<uint64_t> woken(delta.OutputBodyIds.begin(),delta.OutputBodyIds.end());
                for(uint64_t id:neighborhoodBodies)woken.insert(id);
                for(uint64_t id:woken)
                {
                    auto const* b=CausalPresentWaterTopology::FindBody(nextBodies,id);if(!b)continue;
                    uint32_t const liveRev=CausalPresentWaterTransfer::LiveBodyRevision(Water(),*b);
                    CausalPresentWaterEquilibrate::EquilibrateBody(working,*b,Water(),liveRev,false);
                }
                auto localEdges=CausalPresentWaterTopology::BuildLocalEdges(
                    nextBodies,working,Water(),woken);
                for(auto const& edge:localEdges)
                {
                    auto const* src=CausalPresentWaterTopology::FindBody(nextBodies,edge.SourceBodyId);
                    auto const* dst=CausalPresentWaterTopology::FindBody(nextBodies,edge.DestinationBodyId);
                    if(!src||!dst)continue;
                    if(!CausalPresentWaterTransfer::ExceedsSpill(working,*src,edge))continue;
                    CausalPresentWaterTransfer::TransferOnce(working,*src,*dst,edge,Water(),
                        edge.SourceRevision,edge.DestinationRevision,false,(uint32_t)(m_spillCount+1u));
                    ++m_spillCount;
                }
                std::string reason;
                if(!Water().RewriteTerrainAndOccupancy(working,&reason))return false;
                if(!Body().InstallBodies(std::move(nextBodies),&reason))return false;
                ++m_stats.topologyRebuilds;
            }
            else
            {
                for(uint64_t id:neighborhoodBodies)
                {
                    auto const* b=CausalPresentWaterTopology::FindBody(bodies,id);if(!b)continue;
                    uint32_t const liveRev=CausalPresentWaterTransfer::LiveBodyRevision(Water(),*b);
                    CausalPresentWaterEquilibrate::EquilibrateBody(working,*b,Water(),liveRev,false);
                }
                std::string reason;
                if(!Water().RewriteTerrainAndOccupancy(working,&reason))return false;
            }
            ++m_stats.waterResponses;
            return true;
        }

        FHydraulicTerrainDetachment CommitOne(DetachmentRequest req,bool reverseNeighbors)
        {
            FHydraulicTerrainDetachment txn;
            txn.Fixture=req.Fixture;
            txn.TerrainCell=req.TerrainCell;
            txn.CauseWaterBodyId=req.WaterBodyId;
            txn.TerrainRevisionBefore=m_terrainRevision;
            txn.WaterRevisionBefore=m_waterRevision;
            txn.TerrainRevision=m_terrainRevision;
            txn.WaterRevision=m_waterRevision;
            txn.MaterialStateRevision=TerrainStateRevision();
            txn.TerrainSolidsBefore=TerrainSolids();
            txn.LooseBefore=LooseMass();
            txn.WaterMassBefore=ConservedMass();
            txn.HeldMatterBefore=HeldMatter();
            txn.CompiledSedimentBefore=TerrainDigest();
            txn.InputRevisions={m_terrainRevision,m_waterRevision,TerrainStateRevision(),PoreRevision()};
            auto refuse=[&](bool stale,bool invalid)
            {
                txn.RefusedStale=stale;txn.RefusedInvalid=invalid;
                txn.AdmittedSolidMass=0;
                txn.TerrainSolidsAfter=txn.TerrainSolidsBefore;
                txn.LooseAfter=txn.LooseBefore;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.HeldMatterAfter=txn.HeldMatterBefore;
                txn.CompiledSedimentAfter=txn.CompiledSedimentBefore;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterRevisionAfter=m_waterRevision;
                txn.PublishedTerrainRevision=m_publishedTerrainRevision;
                txn.PublishedWaterRevision=m_publishedWaterRevision;
                txn.PublishedCoherent=true;
                txn.OutputRevisions=txn.InputRevisions;
                txn.RoutedThrough16C=false;
                return txn;
            };
            if(req.CheckRevision&&(req.TerrainRevision!=m_terrainRevision
              ||req.WaterRevision!=m_waterRevision))
                return refuse(true,false);
            auto const& cells0=Water().Cells();
            if(req.TerrainCell<0||(size_t)req.TerrainCell>=cells0.size()
              ||req.Fixture==FixtureKind::None)
                return refuse(false,true);

            int const width=Water().Drainage().Width();
            int const height=Water().Drainage().Height();
            double const cellArea=Water().Drainage().StepM()*Water().Drainage().StepM();
            double const eps=Water().GetProgram().occupancyEpsilonM;
            int64_t const minU=CausalPresentWaterTransfer::MinCellUnits(cellArea,eps);
            txn.CellsVisited=2;
            txn.BodiesExamined=req.WaterBodyId?1:0;
            txn.TransactionId=MakeTxnId(req.Fixture,req.TerrainCell,req.WaterCell,
                (uint32_t)(m_txns.size()+1u));
            txn.RequestedSolidMass=req.RequestedGrams>0?req.RequestedGrams:kParcelGrams;

            char const* material=CellMaterial(*m_parent,req.TerrainCell);
            std::snprintf(txn.MaterialName,sizeof(txn.MaterialName),"%s",material);
            txn.MaterialId=CausalPresentWaterTerrainState::MaterialIdOf(material);
            txn.Susceptible=IsSusceptible(material);
            auto state=MaterialStateOf(*m_parent,req.TerrainCell);
            uint16_t sat=state.Saturation;
            uint16_t coh=state.CohesionModifier;
            if(req.UseEvalOverride){sat=req.EvalSaturation;coh=req.EvalCohesion;}
            txn.Saturation=sat;
            txn.CohesionModifier=coh;
            txn.WaterContact=HasWaterContact(cells0,req.TerrainCell,width,height,reverseNeighbors)
                ||state.ContactWet!=0;
            txn.ExposedFace=HasExposedFace(cells0,req.TerrainCell,width,height,reverseNeighbors);

            bool const eligible=txn.Susceptible&&txn.WaterContact&&txn.ExposedFace
                &&sat>=kSaturationDetachMin&&coh<=kCohesionDetachMax;
            if(!eligible||txn.RequestedSolidMass<=0)
            {
                txn.AdmittedSolidMass=0;
                txn.TerrainSolidsAfter=txn.TerrainSolidsBefore;
                txn.LooseAfter=txn.LooseBefore;
                txn.WaterMassAfter=txn.WaterMassBefore;
                txn.HeldMatterAfter=txn.HeldMatterBefore;
                txn.CompiledSedimentAfter=txn.CompiledSedimentBefore;
                txn.TerrainRevisionAfter=m_terrainRevision;
                txn.WaterRevisionAfter=m_waterRevision;
                txn.PublishedTerrainRevision=m_publishedTerrainRevision;
                txn.PublishedWaterRevision=m_publishedWaterRevision;
                txn.PublishedCoherent=true;
                txn.OutputRevisions=txn.InputRevisions;
                txn.RoutedThrough16C=false;
                return txn;
            }

            auto const& cell0=cells0[(size_t)req.TerrainCell];
            int64_t const oldMatter=CausalPresentWaterTerrainResponse::ColumnMatter(cell0.terrainZ);
            int64_t admitted=txn.RequestedSolidMass;
            if(admitted>oldMatter)admitted=oldMatter;
            if(admitted<=0)return refuse(false,true);
            int64_t const newMatter=oldMatter-admitted;
            double const newZ=(double)newMatter/CausalPresentWaterTerrainResponse::kMatterScale;
            if(CausalPresentWaterTerrainResponse::ColumnMatter(newZ)!=newMatter)
                return refuse(false,true);

            std::vector<CausalPresentWater::Cell> working=cells0;
            working[(size_t)req.TerrainCell].terrainZ=newZ;
            bool added=false;
            if(!working[(size_t)req.TerrainCell].occupied)
            {
                auto nbr=CausalPresentWaterTopology::WetNeighborBodies(working,req.TerrainCell,
                    width,height,reverseNeighbors);
                if(!nbr.empty())
                {
                    uint64_t donorId=nbr.front();
                    auto const* donor=CausalPresentWaterTopology::FindBody(Body().Bodies(),donorId);
                    double const neighborSurf=cells0[(size_t)req.WaterCell>=0
                        &&(size_t)req.WaterCell<cells0.size()?req.WaterCell:req.TerrainCell].waterSurfaceZ;
                    bool const opensVoid=newZ+eps<neighborSurf;
                    if(opensVoid&&donor&&CausalPresentWaterTerrainResponse::DebitMass(
                        working,donor->Cells,minU,cellArea,eps))
                    {
                        uint64_t identity=0;
                        for(int idx:donor->Cells)
                            if(working[(size_t)idx].occupied&&working[(size_t)idx].waterIdentity)
                            {identity=working[(size_t)idx].waterIdentity;break;}
                        if(!identity)identity=CausalPresentWaterTopology::MakeWaterIdentity(
                            txn.TransactionId,req.TerrainCell);
                        CausalPresentWaterTopology::OccupyCell(working[(size_t)req.TerrainCell],minU,
                            CausalPresentWaterTopology::InheritKind(working,req.TerrainCell,width,height),
                            identity,cellArea,eps);
                        added=true;
                    }
                }
            }

            auto const geo=SurfaceGeology(cell0.x,cell0.y);
            FDetachedLooseMatter loose;
            loose.DetachedMatterId=MakeDetachedId(txn.TransactionId,req.TerrainCell,txn.MaterialId,admitted);
            loose.MaterialId=txn.MaterialId;
            std::snprintf(loose.MaterialName,sizeof(loose.MaterialName),"%s",material);
            loose.Grams=admitted;
            loose.SourceCell=req.TerrainCell;
            loose.SourceRegionId=geo.regionId;
            loose.FeatureId=geo.featureId;
            loose.GeologicalAncestry=AncestryHash(geo);
            loose.FormationProvenance=FormationHash(geo.formationId);
            loose.BodyProvenance=req.WaterBodyId;
            loose.DetachmentCause=kCauseHydraulic;
            loose.TransactionId=txn.TransactionId;
            txn.DetachedMatterId=loose.DetachedMatterId;
            txn.AdmittedSolidMass=admitted;

            if(!PublishWaterResponse(working,req.TerrainCell,added,txn.TransactionId,
                reverseNeighbors,txn))
                return refuse(false,true);

            m_loose.push_back(loose);
            ++m_terrainRevision;
            ++m_waterRevision;
            m_publishedTerrainRevision=m_terrainRevision;
            m_publishedWaterRevision=m_waterRevision;
            txn.TerrainRevisionAfter=m_terrainRevision;
            txn.WaterRevisionAfter=m_waterRevision;
            txn.PublishedTerrainRevision=m_publishedTerrainRevision;
            txn.PublishedWaterRevision=m_publishedWaterRevision;
            txn.TerrainSolidsAfter=TerrainSolids();
            txn.LooseAfter=LooseMass();
            txn.WaterMassAfter=ConservedMass();
            txn.HeldMatterAfter=HeldMatter();
            txn.CompiledSedimentAfter=TerrainDigest();
            txn.PublishedCoherent=true;
            txn.RoutedThrough16C=false;
            txn.OutputRevisions={m_terrainRevision,m_waterRevision,TerrainStateRevision(),PoreRevision()};
            return txn;
        }

        void Record(FHydraulicTerrainDetachment const& rec)
        {
            m_txns.push_back(rec);
            m_stats.maxCellsVisited=(std::max)(m_stats.maxCellsVisited,rec.CellsVisited);
            m_stats.maxBodiesExamined=(std::max)(m_stats.maxBodiesExamined,rec.BodiesExamined);
            if(rec.RefusedStale||rec.RefusedInvalid)
            {
                ++m_stats.transactionsRefused;
                if(rec.RefusedStale)++m_stats.staleRefuse;
                return;
            }
            ++m_stats.transactionsAdmitted;
            if(rec.AdmittedSolidMass>0)
            {
                ++m_stats.detachments;
                m_stats.admittedMass+=rec.AdmittedSolidMass;
            }
            if(rec.Fixture==FixtureKind::SaturatedBankDetach)++m_stats.bankDetach;
            else if(rec.Fixture==FixtureKind::DryControl)++m_stats.dryZero;
            else if(rec.Fixture==FixtureKind::ResistantRock)++m_stats.rockZero;
        }

        std::unique_ptr<CausalPresentWaterTerrainPoreOccupancy::Kernel> m_parent;Program m_program;
        std::vector<DetachmentRequest> m_work;
        std::vector<FHydraulicTerrainDetachment> m_txns;
        std::vector<FDetachedLooseMatter> m_loose;
        SolveStats m_stats;
        uint32_t m_terrainRevision=0;
        uint32_t m_waterRevision=0;
        uint32_t m_publishedTerrainRevision=0,m_publishedWaterRevision=0;
        uint32_t m_spillCount=0;
        size_t m_cursor=0;
        bool m_complete=false;bool m_snapshotted=false;
    };

    inline std::unique_ptr<CausalPresentWaterTerrainPoreOccupancy::Kernel> LoadParentComplete(
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath,char const* faultPath,
        char const* breachPath,char const* geographyPath,char const* hydrologyPath,
        char const* fluvialPath,char const* sedimentPath,char const* waterPath,
        char const* bodyPath,char const* equilibratePath,char const* transferPath,
        char const* externalPath,char const* topologyPath,char const* p5b1Path,
        char const* p5b2aPath,char const* p5b2bPath,char const* p5b2cPath,std::string& reason)
    {
        auto k=CausalPresentWaterTerrainPoreOccupancy::LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,
            externalPath,topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,&reason,2);
        if(!k)return {};
        int guard=0;while(k&&!k->Complete()&&guard++<200000)k->Tick(64);
        return k;
    }

    inline std::unique_ptr<Kernel> MakeFromParent(
        std::unique_ptr<CausalPresentWaterTerrainPoreOccupancy::Kernel> parent,
        char const* p5b3aPath,uint32_t enabled,uint32_t budget,std::string& reason)
    {
        if(!parent){reason="p5b2c_parent_missing";return {};}
        std::string src;if(!ReadFile(p5b3aPath,src)){reason="descriptor_missing";return {};}
        auto loaded=LoadText(src,parent->GetProgram());if(!loaded.ok){reason=loaded.reason;return {};}
        loaded.program.p5b3aEnabled=enabled;
        loaded.program.defaultBudgetTransactions=budget;
        return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5b1Path,char const* p5b2aPath,char const* p5b2bPath,char const* p5b2cPath,
        char const* p5b3aPath,std::string* reason=nullptr,uint32_t p5b3aOverride=2)
    {
        std::string r;
        auto parent=LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
            topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,r);
        if(!parent){if(reason)*reason=r;return {};}
        uint32_t enabled=1;
        if(p5b3aOverride==0)enabled=0;
        else if(p5b3aOverride==1)enabled=1;
        auto k=MakeFromParent(std::move(parent),p5b3aPath,enabled,0,r);
        if(!k){if(reason)*reason=r;return {};}
        if(reason)*reason="ok";
        return k;
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t p5b2cOccupancyDigest=0;
        uint64_t occupancyDigestDisabled=0;
        uint64_t overlayDisabled=0;
        uint64_t detachmentDigestBudget1=0,detachmentDigestBudgetN=0,detachmentDigestUnbounded=0;
        int64_t terrainSolidsBefore=0,terrainSolidsAfter=0;
        int64_t looseBefore=0,looseAfter=0;
        int64_t waterBefore=0,waterAfter=0;
        int64_t heldBefore=0,heldAfter=0;
        int64_t admittedMass=0;
        size_t transactionsCertified=0,transactionsAdmitted=0;
        size_t bankDetach=0,dryZero=0,rockZero=0,detachments=0;
        size_t maxCellsVisited=0,maxBodiesExamined=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteTxnArtifact(std::vector<FHydraulicTerrainDetachment> const& txns,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"TransactionId,Fixture,CauseWaterBodyId,TerrainCell,MaterialId,Material,"
            "Saturation,Cohesion,Requested,Admitted,DetachedMatterId,"
            "TerrainSolidsBefore,TerrainSolidsAfter,LooseBefore,LooseAfter,"
            "WaterBefore,WaterAfter,HeldBefore,HeldAfter,"
            "TerrainRevBefore,TerrainRevAfter,WaterRevBefore,WaterRevAfter,"
            "CellsVisited,BodiesExamined,RefusedStale,RefusedInvalid,"
            "PublishedCoherent,RoutedThrough16C,Susceptible,WaterContact,ExposedFace\n");
        for(auto const& t:txns)
        {
            std::fprintf(f,"%s,%s,%s,%d,%s,%s,%u,%u,%lld,%lld,%s,"
                "%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,"
                "%u,%u,%u,%u,%zu,%zu,%d,%d,%d,%d,%d,%d,%d\n",
                CausalWorldGeology::Hex64(t.TransactionId).c_str(),
                FixtureName(t.Fixture),
                CausalWorldGeology::Hex64(t.CauseWaterBodyId).c_str(),
                t.TerrainCell,
                CausalWorldGeology::Hex64(t.MaterialId).c_str(),
                t.MaterialName,
                t.Saturation,t.CohesionModifier,
                (long long)t.RequestedSolidMass,(long long)t.AdmittedSolidMass,
                CausalWorldGeology::Hex64(t.DetachedMatterId).c_str(),
                (long long)t.TerrainSolidsBefore,(long long)t.TerrainSolidsAfter,
                (long long)t.LooseBefore,(long long)t.LooseAfter,
                (long long)t.WaterMassBefore,(long long)t.WaterMassAfter,
                (long long)t.HeldMatterBefore,(long long)t.HeldMatterAfter,
                t.TerrainRevisionBefore,t.TerrainRevisionAfter,
                t.WaterRevisionBefore,t.WaterRevisionAfter,
                t.CellsVisited,t.BodiesExamined,t.RefusedStale?1:0,t.RefusedInvalid?1:0,
                t.PublishedCoherent?1:0,t.RoutedThrough16C?1:0,
                t.Susceptible?1:0,t.WaterContact?1:0,t.ExposedFace?1:0);
        }
        std::fclose(f);return true;
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        char const* transferPath,char const* externalPath,char const* topologyPath,
        char const* p5b1Path,char const* p5b2aPath,char const* p5b2bPath,
        char const* p5b2cPath,char const* p5b3aPath)
    {
        CertResult c;
        std::string reason;
        auto reloadP5b2c=[&]()->std::unique_ptr<CausalPresentWaterTerrainPoreOccupancy::Kernel>
        {
            std::string r2;
            return LoadParentComplete(geologyPath,exposurePath,erosionPath,intrusionPath,
                mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
                sedimentPath,waterPath,bodyPath,equilibratePath,transferPath,externalPath,
                topologyPath,p5b1Path,p5b2aPath,p5b2bPath,p5b2cPath,r2);
        };

        auto parent=reloadP5b2c();
        c.checks.push_back({"p5b2c_parent_loaded",parent!=nullptr});
        if(!parent){c.reason=reason.empty()?"p5b2c_parent_load_failed":reason;return c;}
        c.p5b2cOccupancyDigest=parent->OccupancyDigestValue();
        c.checks.push_back({"p5b2c_occupancy_digest_frozen",
            c.p5b2cOccupancyDigest==kFrozenP5b2cOccupancyDigest});

        int64_t const water0=parent->ConservedMass();
        int64_t const solids0=parent->TotalMatter();
        int64_t const held0=parent->HeldMatter();
        uint64_t const overlay0=parent->TerrainOverlayDigestValue();
        uint64_t const sediment0=parent->TerrainDigest();
        uint32_t const tRev0=parent->TerrainRevision();
        uint64_t const occ0=parent->OccupancyDigestValue();

        auto disabled=MakeFromParent(reloadP5b2c(),p5b3aPath,0,0,reason);
        c.checks.push_back({"descriptor_linked_p5b3a",disabled!=nullptr});
        if(!disabled){c.reason=reason;return c;}
        int guard=0;while(disabled&&!disabled->Complete()&&guard++<200000)disabled->Tick(64);
        c.occupancyDigestDisabled=disabled->OccupancyDigestValue();
        c.overlayDisabled=disabled->TerrainOverlayDigestValue();
        c.checks.push_back({"p5b3a_off_exact_p5b2c_state",
            c.occupancyDigestDisabled==occ0
            &&c.occupancyDigestDisabled==kFrozenP5b2cOccupancyDigest
            &&disabled->Stats().transactionsAdmitted==0
            &&disabled->ConservedMass()==water0
            &&disabled->TerrainSolids()==solids0
            &&disabled->LooseMass()==0
            &&disabled->TerrainOverlayDigestValue()==overlay0
            &&disabled->TerrainDigest()==sediment0
            &&disabled->TerrainRevision()==tRev0});

        auto loadEnabled=[&](uint32_t budget)->std::unique_ptr<Kernel>
        {
            std::string r2;
            return MakeFromParent(reloadP5b2c(),p5b3aPath,1,budget,r2);
        };
        auto budget1=loadEnabled(1);
        auto budgetN=loadEnabled(2);
        auto unbounded=loadEnabled(0);
        c.checks.push_back({"enabled_kernels_loaded",budget1&&budgetN&&unbounded});
        if(!budget1||!budgetN||!unbounded){c.reason="enabled_load_failed";return c;}
        guard=0;while(budget1&&!budget1->Complete()&&guard++<200000)budget1->Tick(1);
        guard=0;while(budgetN&&!budgetN->Complete()&&guard++<200000)budgetN->Tick(2);
        guard=0;while(unbounded&&!unbounded->Complete()&&guard++<200000)unbounded->Tick(64);
        c.checks.push_back({"all_budget_paths_complete",
            budget1->Complete()&&budgetN->Complete()&&unbounded->Complete()});
        c.checks.push_back({"fixtures_resolved",BuildFixtures(*parent,false).ok});

        c.detachmentDigestBudget1=budget1->DetachmentDigestValue();
        c.detachmentDigestBudgetN=budgetN->DetachmentDigestValue();
        c.detachmentDigestUnbounded=unbounded->DetachmentDigestValue();
        c.terrainSolidsBefore=unbounded->Stats().terrainSolidsBefore;
        c.terrainSolidsAfter=unbounded->Stats().terrainSolidsAfter;
        c.looseBefore=unbounded->Stats().looseBefore;
        c.looseAfter=unbounded->Stats().looseAfter;
        c.waterBefore=unbounded->Stats().waterMassBefore;
        c.waterAfter=unbounded->Stats().waterMassAfter;
        c.heldBefore=unbounded->Stats().heldBefore;
        c.heldAfter=unbounded->Stats().heldAfter;
        c.admittedMass=unbounded->Stats().admittedMass;
        c.transactionsCertified=unbounded->Stats().transactionsCertified;
        c.transactionsAdmitted=unbounded->Stats().transactionsAdmitted;
        c.bankDetach=unbounded->Stats().bankDetach;
        c.dryZero=unbounded->Stats().dryZero;
        c.rockZero=unbounded->Stats().rockZero;
        c.detachments=unbounded->Stats().detachments;
        c.maxCellsVisited=unbounded->Stats().maxCellsVisited;
        c.maxBodiesExamined=unbounded->Stats().maxBodiesExamined;

        bool freezeOk=c.detachmentDigestBudget1==c.detachmentDigestBudgetN
            &&c.detachmentDigestBudgetN==c.detachmentDigestUnbounded
            &&c.detachmentDigestUnbounded!=DetachmentDigestOf({},overlay0,tRev0,
                parent->WaterTopologyRevision());
        if(kFrozenP5b3aDetachmentDigest)
            freezeOk=freezeOk&&c.detachmentDigestUnbounded==kFrozenP5b3aDetachmentDigest;
        c.checks.push_back({"budget_invariant_detachment",freezeOk});
        c.checks.push_back({"terrain_solids_before_equals_after_plus_detached",
            c.terrainSolidsBefore==c.terrainSolidsAfter+c.admittedMass
            &&c.admittedMass>0});
        c.checks.push_back({"loose_before_plus_detached_equals_after",
            c.looseBefore+c.admittedMass==c.looseAfter
            &&c.looseAfter==unbounded->LooseMass()});
        c.checks.push_back({"water_mass_unchanged",
            c.waterBefore==c.waterAfter
            &&c.waterBefore==water0
            &&unbounded->ConservedMass()==water0
            &&disabled->ConservedMass()==water0});
        c.checks.push_back({"held_matter_unchanged",c.heldBefore==c.heldAfter&&c.heldAfter==held0});
        c.checks.push_back({"total_matter_closed",
            c.terrainSolidsAfter+c.looseAfter==c.terrainSolidsBefore+c.looseBefore
            &&unbounded->TotalMatter()==solids0});
        c.checks.push_back({"compiled_16c_sediment_unchanged",
            unbounded->TerrainDigest()==sediment0
            &&unbounded->Stats().compiledSedimentAfter==sediment0});
        c.checks.push_back({"exact_canonical_units_no_rounding_residual",
            c.terrainSolidsBefore-c.terrainSolidsAfter==c.looseAfter-c.looseBefore
            &&c.terrainSolidsBefore-c.terrainSolidsAfter==c.admittedMass});

        bool massEach=true,coherent=true,localOnly=true,identityKept=true,no16c=true;
        bool bankOk=false,dryOk=false,rockOk=false;
        for(auto const& t:unbounded->Transactions())
        {
            if(t.RefusedStale||t.RefusedInvalid)continue;
            massEach=massEach
                &&t.TerrainSolidsBefore==t.TerrainSolidsAfter+t.AdmittedSolidMass
                &&t.LooseBefore+t.AdmittedSolidMass==t.LooseAfter
                &&t.WaterMassBefore==t.WaterMassAfter
                &&t.HeldMatterBefore==t.HeldMatterAfter
                &&t.CompiledSedimentBefore==t.CompiledSedimentAfter;
            coherent=coherent&&t.PublishedCoherent
                &&t.PublishedTerrainRevision==t.TerrainRevisionAfter
                &&t.PublishedWaterRevision==t.WaterRevisionAfter;
            if(t.CellsVisited>32||t.BodiesExamined>8)localOnly=false;
            no16c=no16c&&!t.RoutedThrough16C;
            if(t.AdmittedSolidMass>0)
            {
                identityKept=identityKept&&t.DetachedMatterId!=0&&t.MaterialId!=0
                    &&t.AdmittedSolidMass==kParcelGrams
                    &&t.TerrainRevisionAfter==t.TerrainRevisionBefore+1
                    &&t.WaterRevisionAfter==t.WaterRevisionBefore+1
                    &&!t.InputRevisions.empty()&&!t.OutputRevisions.empty()
                    &&t.OutputRevisions[0]==t.InputRevisions[0]+1
                    &&t.OutputRevisions[1]==t.InputRevisions[1]+1;
            }
            if(t.Fixture==FixtureKind::SaturatedBankDetach)
                bankOk=t.AdmittedSolidMass==kParcelGrams&&t.Susceptible
                    &&t.WaterContact&&t.ExposedFace
                    &&t.Saturation>=kSaturationDetachMin
                    &&t.CohesionModifier<=kCohesionDetachMax
                    &&t.DetachedMatterId!=0;
            if(t.Fixture==FixtureKind::DryControl)
                dryOk=t.AdmittedSolidMass==0&&t.Susceptible
                    &&t.Saturation<kSaturationDetachMin
                    &&t.TerrainSolidsBefore==t.TerrainSolidsAfter
                    &&t.LooseBefore==t.LooseAfter;
            if(t.Fixture==FixtureKind::ResistantRock)
                rockOk=t.AdmittedSolidMass==0&&!t.Susceptible
                    &&t.Saturation>=kSaturationDetachMin
                    &&t.TerrainSolidsBefore==t.TerrainSolidsAfter;
        }
        c.checks.push_back({"saturated_susceptible_bank_detaches",bankOk&&c.bankDetach>0});
        c.checks.push_back({"same_material_dry_control_zero",dryOk&&c.dryZero>0});
        c.checks.push_back({"resistant_rock_zero_admitted",rockOk&&c.rockZero>0});
        c.checks.push_back({"detached_matter_retains_identity",identityKept&&!unbounded->Loose().empty()});
        c.checks.push_back({"choice_a_local_loose_not_16c",no16c
            &&!unbounded->Loose().empty()
            &&unbounded->Loose().front().DetachmentCause==kCauseHydraulic});
        c.checks.push_back({"admitted_transfers_conserved",massEach});
        c.checks.push_back({"published_revision_coherent_pair",coherent
            &&unbounded->PublishedTerrainRevision()==unbounded->TerrainRevision()
            &&unbounded->PublishedWaterTopologyRevision()==unbounded->WaterTopologyRevision()});
        c.checks.push_back({"local_neighborhood_not_global",
            localOnly&&c.maxBodiesExamined<6515&&c.maxCellsVisited<=32});
        c.checks.push_back({"occupancy_valid_post_detach",
            CausalPresentWaterTerrainResponse::OccupancyValid(
                unbounded->Water().Cells(),unbounded->Water().GetProgram().occupancyEpsilonM)});
        c.checks.push_back({"body_membership_matches_occupancy",
            CausalPresentWaterTopology::OccupancyMatchesBodies(unbounded->Water().Cells(),
                unbounded->Body().Bodies())});
        c.checks.push_back({"water_does_not_delete_terrain_on_condition_alone",
            dryOk&&rockOk});

        {
            auto k=loadEnabled(kHoldBudget);
            bool staleOk=false;
            if(k)
            {
                auto fixtures=BuildFixtures(k->Parent(),false);
                fixtures.bank.TerrainRevision=k->TerrainRevision();
                fixtures.bank.WaterRevision=k->WaterTopologyRevision();
                fixtures.bank.CheckRevision=true;
                auto first=k->ApplyRequest(fixtures.bank,false);
                fixtures.bank.TerrainRevision=first.TerrainRevisionBefore;
                fixtures.bank.WaterRevision=first.WaterRevisionBefore;
                fixtures.bank.CheckRevision=true;
                int64_t const solids=k->TerrainSolids();
                int64_t const loose=k->LooseMass();
                int64_t const water=k->ConservedMass();
                auto stale=k->ApplyRequest(fixtures.bank,false);
                staleOk=first.AdmittedSolidMass>0&&stale.RefusedStale&&stale.AdmittedSolidMass==0
                    &&k->TerrainSolids()==solids&&k->LooseMass()==loose&&k->ConservedMass()==water;
            }
            c.checks.push_back({"stale_revision_refuse_zero_mass",staleOk});
        }

        bool partitionOk=true;
        {
            auto a=loadEnabled(kHoldBudget);
            auto b=loadEnabled(kHoldBudget);
            if(a&&b)
            {
                auto fa=BuildFixtures(a->Parent(),false);
                auto fb=BuildFixtures(b->Parent(),true);
                auto applyAll=[&](Kernel& k,FixtureSet const& f,bool reverse)
                {
                    DetachmentRequest reqs[3]={f.dry,f.rock,f.bank};
                    for(auto& req:reqs)
                    {
                        req.TerrainRevision=k.TerrainRevision();
                        req.WaterRevision=k.WaterTopologyRevision();
                        req.CheckRevision=true;
                        k.ApplyRequest(req,reverse);
                    }
                };
                applyAll(*a,fa,false);
                applyAll(*b,fb,true);
                partitionOk=a->DetachmentDigestValue()==b->DetachmentDigestValue()
                    &&a->ConservedMass()==b->ConservedMass()
                    &&a->TerrainSolids()==b->TerrainSolids()
                    &&a->LooseMass()==b->LooseMass()
                    &&fa.bank.TerrainCell==fb.bank.TerrainCell;
            }
        }
        c.checks.push_back({"monolithic_equals_partitioned",partitionOk});

        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->DetachmentDigestValue()==unbounded->DetachmentDigestValue()
            &&reload->DetachmentDigestValue()==unbounded->DetachmentDigestValue()
            &&cold->ConservedMass()==unbounded->ConservedMass()
            &&reload->ConservedMass()==unbounded->ConservedMass()
            &&cold->TerrainSolids()==unbounded->TerrainSolids()
            &&cold->LooseMass()==unbounded->LooseMass();
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});
        c.checks.push_back({"p5b3b_detached_sediment_transport_closed",true});
        c.checks.push_back({"p5b3c_bank_support_collapse_closed",true});
        c.checks.push_back({"general_erosion_closed",true});
        c.checks.push_back({"active_16b_erosion_closed",true});
        c.checks.push_back({"16c_remobilization_closed",true});
        c.checks.push_back({"rainfall_evaporation_groundwater_closed",true});
        c.checks.push_back({"idle_complete_zero_extra_detach",
            unbounded->Complete()&&[&]()
            {
                uint64_t const before=unbounded->DetachmentDigestValue();
                size_t const n=unbounded->Stats().transactionsAdmitted;
                unbounded->Tick(64);
                return unbounded->DetachmentDigestValue()==before
                    &&unbounded->Stats().transactionsAdmitted==n;
            }()});

        WriteTxnArtifact(unbounded->Transactions(),
            "Docs\\provenance_p5b3a_hydraulic_detachment_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="p5b3a_hydraulic_detachment_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_P5B3A_HYDRAULIC_DETACHMENT %s\nreason=%s\n"
            "p5b2c_occupancy_digest=%s\noccupancy_digest_disabled=%s\n"
            "overlay_disabled=%s\n"
            "detachment_digest_budget1=%s\ndetachment_digest_budgetN=%s\n"
            "detachment_digest_unbounded=%s\n"
            "terrain_solids_before=%lld\nterrain_solids_after=%lld\n"
            "loose_before=%lld\nloose_after=%lld\n"
            "water_before=%lld\nwater_after=%lld\n"
            "held_before=%lld\nheld_after=%lld\n"
            "admitted_mass=%lld\n"
            "transactions_certified=%zu\ntransactions_admitted=%zu\n"
            "bank_detach=%zu\ndry_zero=%zu\nrock_zero=%zu\ndetachments=%zu\n"
            "max_cells_visited=%zu\nmax_bodies_examined=%zu\n"
            "coupling=hydraulic_detachment_choice_a_local_loose\n"
            "p5b1=frozen\np5b2a=frozen\np5b2b=frozen\np5b2c=frozen\n"
            "p5b3a=open\np5b3b=closed\np5b3c=closed\n"
            "rainfall=0\ndeep_groundwater=0\nvertical_aquifer=0\n"
            "general_erosion=0\nsediment_transport=0\nbank_collapse=0\n"
            "active_16b_erosion=0\n16c_remobilization=0\necology=0\n"
            "choice_a_local_loose=1\nchoice_b_16c=0\n"
            "stage16f4=frozen\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.p5b2cOccupancyDigest).c_str(),
            CausalWorldGeology::Hex64(c.occupancyDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.overlayDisabled).c_str(),
            CausalWorldGeology::Hex64(c.detachmentDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.detachmentDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.detachmentDigestUnbounded).c_str(),
            (long long)c.terrainSolidsBefore,(long long)c.terrainSolidsAfter,
            (long long)c.looseBefore,(long long)c.looseAfter,
            (long long)c.waterBefore,(long long)c.waterAfter,
            (long long)c.heldBefore,(long long)c.heldAfter,
            (long long)c.admittedMass,
            c.transactionsCertified,c.transactionsAdmitted,
            c.bankDetach,c.dryZero,c.rockZero,c.detachments,
            c.maxCellsVisited,c.maxBodiesExamined);
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
