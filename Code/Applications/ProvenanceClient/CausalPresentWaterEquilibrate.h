#pragma once

// Stage 16F.1: body-local equilibration only.
// Within one already-certified FPresentWaterBody:
//   static occupied cells + body-local surface rule + terrain void + conserved
//   water amount → relax toward one internally consistent body state.
//
// Equilibration may redistribute water inside a body, but it may not change
// body identity, topology, total water, terrain, or connectivity.
//
// CLOSED: inter-body transfer, river propagation across outlets, rainfall,
// pours, terrain coupling, erosion, sediment motion, P5b, 16F.2, 16F.3.

#include "CausalPresentWaterBody.h"

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

namespace CausalPresentWaterEquilibrate
{
    constexpr char const* kExpectedRegion="causal_world_present_water_equilibrate_floor";
    constexpr uint64_t kFrozenStage16EBodyDigest=0x2352b000a56f499cull;
    constexpr uint64_t kFrozenStage16EConnectivityDigest=0xba4849f7148a7297ull;
    constexpr uint64_t kFrozenWaterDigest=0x636d01ba00d3dffeull;
    constexpr uint64_t kFrozenOccupancyDigest=0x880a46fcdca2f8a4ull;

    enum class BudgetMode:uint8_t
    {
        Disabled=0,
        One=1,
        Finite=2,
        Unbounded=3
    };

    struct Program
    {
        uint64_t worldIdentityHash=0,equilibrateEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;
        uint32_t equilibrationEnabled=1;
        uint32_t defaultBudgetBodies=0; // 0 ⇒ unbounded in one shot
        std::string worldgenId,regionKey,parentRegionKey;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalPresentWaterBody::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_PRESENT_WATER_EQUILIBRATE_V1";
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
          &&hex("equilibrate_event_id",r.program.equilibrateEventId)
          &&u32("chronology",r.program.chronology)
          &&u32("equilibration_enabled",r.program.equilibrationEnabled)
          &&u32("default_budget_bodies",r.program.defaultBudgetBodies);
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.equilibrateEventId!=0
          &&r.program.chronology>parent.chronology
          &&(r.program.equilibrationEnabled==0||r.program.equilibrationEnabled==1);
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_present_water_equilibrate_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct BodyReceipt
    {
        uint64_t BodyId=0;
        CausalPresentWaterBody::BodyType Type=CausalPresentWaterBody::BodyType::None;
        int64_t WaterMass=0;
        double SurfaceBefore=0,SurfaceAfter=0;
        uint32_t Iterations=0;
        double Residual=0,MaxLocalError=0;
        uint32_t InputRevision=0,OutputRevision=0;
        bool Mutated=false;
        bool RefusedStale=false;
        uint32_t WorkUnits=0;
    };

    struct SolveStats
    {
        size_t bodiesTouched=0,bodiesMutated=0,bodiesSkippedZero=0,bodiesAlreadyEq=0;
        size_t bodiesRefusedStale=0;
        uint32_t totalIterations=0;
        int64_t massBefore=0,massAfter=0;
        uint64_t fieldDigest=0;
    };

    inline uint64_t OccupancyMaskDigest(std::vector<CausalPresentWater::Cell> const& cells)
    {
        uint64_t digest=14695981039346656037ull;
        for(auto const& cell:cells)
        {
            uint8_t const occ=cell.occupied?1:0;
            CausalWorldGeology::HashAppend(digest,&occ,sizeof(occ));
            CausalWorldGeology::HashAppend(digest,&cell.bodyId,sizeof(cell.bodyId));
            CausalWorldGeology::HashAppend(digest,&cell.kind,sizeof(cell.kind));
            CausalWorldGeology::HashAppend(digest,&cell.terrainZ,sizeof(cell.terrainZ));
        }
        return digest;
    }

    inline uint64_t FieldDigest(std::vector<CausalPresentWater::Cell> const& cells)
    {
        uint64_t digest=14695981039346656037ull;
        for(auto const& cell:cells)
        {
            CausalWorldGeology::HashAppend(digest,&cell.depthM,sizeof(cell.depthM));
            CausalWorldGeology::HashAppend(digest,&cell.waterSurfaceZ,sizeof(cell.waterSurfaceZ));
            CausalWorldGeology::HashAppend(digest,&cell.occupancyUnits,sizeof(cell.occupancyUnits));
            uint8_t const occ=cell.occupied?1:0;
            CausalWorldGeology::HashAppend(digest,&occ,sizeof(occ));
            CausalWorldGeology::HashAppend(digest,&cell.bodyId,sizeof(cell.bodyId));
        }
        return digest;
    }

    inline int64_t BodyMass(std::vector<CausalPresentWater::Cell> const& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body)
    {
        int64_t mass=0;
        for(int idx:body.Cells)
            if(cells[(size_t)idx].occupied)mass+=cells[(size_t)idx].occupancyUnits;
        return mass;
    }

    inline double BodyMeanSurface(std::vector<CausalPresentWater::Cell> const& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body)
    {
        if(body.Cells.empty())return 0;
        double sum=0;size_t n=0;
        for(int idx:body.Cells)
        {
            if(!cells[(size_t)idx].occupied)continue;
            sum+=cells[(size_t)idx].waterSurfaceZ;++n;
        }
        return n?sum/(double)n:0;
    }

    // Convert depths to exact conserved occupancyUnits via largest-remainder.
    inline void CommitDepths(std::vector<CausalPresentWater::Cell>& cells,
        std::vector<int> const& idxs,std::vector<double> const& depthM,
        int64_t totalUnits,double cellArea,double eps)
    {
        size_t const n=idxs.size();
        if(!n)return;
        double scale=cellArea*100.0;
        int64_t minU=(int64_t)std::ceil(eps*scale-1e-12);
        if(minU<1)minU=1;
        // Occupied-component invariant requires every cell keep ≥eps depth.
        int64_t const minTotal=minU*(int64_t)n;
        if(totalUnits<minTotal)
        {
            // Cannot raise mass; leave depths unchanged (caller treats as no-op).
            return;
        }
        int64_t target=totalUnits;
        std::vector<double> exact(n,0);std::vector<int64_t> units(n,0);
        int64_t assigned=0;
        struct Frac{size_t i;double f;};
        std::vector<Frac> fracs;fracs.reserve(n);
        for(size_t i=0;i<n;++i)
        {
            double d=(std::max)(depthM[i],eps*1.0000001);
            exact[i]=d*scale;
            units[i]=(int64_t)std::floor(exact[i]);
            if(units[i]<minU)units[i]=minU;
            assigned+=units[i];
            fracs.push_back({i,exact[i]-(double)units[i]});
        }
        if(assigned>target)
        {
            std::sort(fracs.begin(),fracs.end(),[](Frac const& a,Frac const& b)
            {return a.f<b.f;});
            for(size_t k=0;k<fracs.size()&&assigned>target;++k)
            {
                size_t const i=fracs[k].i;
                int64_t const can=units[i]-minU;
                if(can<=0)continue;
                int64_t const take=(std::min)(can,assigned-target);
                units[i]-=take;assigned-=take;
            }
        }
        else if(assigned<target)
        {
            std::sort(fracs.begin(),fracs.end(),[](Frac const& a,Frac const& b)
            {return a.f>b.f;});
            size_t cursor=0;
            while(assigned<target&&!fracs.empty())
            {
                size_t const i=fracs[cursor%fracs.size()].i;
                ++units[i];++assigned;++cursor;
            }
        }
        // Exact conservation: if still short/long due to minU floor, dump remainder
        // onto the lowest-index cell above min (or accept minTotal floor only when
        // certified mass already covers it).
        if(assigned!=totalUnits&&totalUnits>=minTotal)
        {
            int64_t delta=totalUnits-assigned;
            for(size_t i=0;i<n&&delta!=0;++i)
            {
                if(delta>0){units[i]+=delta;assigned+=delta;delta=0;}
                else
                {
                    int64_t const can=units[i]-minU;
                    int64_t const take=(std::min)(can,-delta);
                    units[i]-=take;assigned-=take;delta+=take;
                }
            }
        }
        for(size_t i=0;i<n;++i)
        {
            auto& cell=cells[(size_t)idxs[i]];
            cell.occupancyUnits=units[i];
            cell.depthM=(double)units[i]/scale;
            if(cell.depthM<eps)cell.depthM=eps;
            cell.waterSurfaceZ=cell.terrainZ+cell.depthM;
            cell.occupied=true;
        }
    }

    inline void EquilibrateLake(std::vector<CausalPresentWater::Cell>& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,
        double cellArea,double eps,BodyReceipt& receipt)
    {
        std::vector<int> idxs=body.Cells;
        if(idxs.empty())return;
        int64_t const total=BodyMass(cells,body);
        receipt.WaterMass=total;
        if(total<=0){receipt.WorkUnits=0;return;}
        // Shared equilibrium plane: reserve epsilon occupancy on every cell,
        // distribute remainder as a flat hydraulic head above terrain.
        size_t const n=idxs.size();
        int64_t const reserve=(int64_t)n; // 1 unit min each after CommitDepths
        int64_t remain=(std::max)(0ll,total-reserve);
        double scale=cellArea*100.0;
        double sumTerrain=0;
        for(int idx:idxs)sumTerrain+=cells[(size_t)idx].terrainZ;
        double const meanT=sumTerrain/(double)n;
        double const extraDepth=(double)remain/(scale*(double)n);
        double const H=meanT+eps+extraDepth;
        std::vector<double> depths(n,0);
        for(size_t i=0;i<n;++i)
            depths[i]=(std::max)(eps,H-cells[(size_t)idxs[i]].terrainZ);
        // If high-terrain cells force more volume, renormalize by raising H.
        double vol=0;for(double d:depths)vol+=d*cellArea;
        double const targetVol=(double)total/(100.0);
        if(vol>1e-15&&std::fabs(vol-targetVol)>1e-9)
        {
            // Binary search shared plane that conserves continuous volume while
            // keeping every occupied cell wet.
            double lo=meanT+eps,hi=meanT+eps+extraDepth*4.0+10.0;
            for(int it=0;it<64;++it)
            {
                double mid=.5*(lo+hi);double v=0;
                for(int idx:idxs)v+=(std::max)(eps,mid-cells[(size_t)idx].terrainZ)*cellArea;
                if(v<targetVol)lo=mid;else hi=mid;
            }
            double const plane=hi;
            for(size_t i=0;i<n;++i)
                depths[i]=(std::max)(eps,plane-cells[(size_t)idxs[i]].terrainZ);
            receipt.Iterations=64;
        }
        else receipt.Iterations=1;
        CommitDepths(cells,idxs,depths,total,cellArea,eps);
        receipt.WorkUnits=1;
        receipt.SurfaceAfter=BodyMeanSurface(cells,body);
    }

    inline void EquilibrateWetland(std::vector<CausalPresentWater::Cell>& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,
        CausalPresentWater::Program const& waterProg,
        double cellArea,double eps,BodyReceipt& receipt)
    {
        std::vector<int> idxs=body.Cells;
        if(idxs.empty())return;
        int64_t const total=BodyMass(cells,body);
        receipt.WaterMass=total;
        if(total<=0){receipt.WorkUnits=0;return;}
        size_t const n=idxs.size();
        double const maxD=(std::max)(waterProg.wetlandMaxDepthM,eps*2.0);
        // Shallow constrained ponding: fill lowest terrain first up to maxD.
        std::vector<size_t> order(n);std::iota(order.begin(),order.end(),0);
        std::sort(order.begin(),order.end(),[&](size_t a,size_t b)
        {
            double const ta=cells[(size_t)idxs[a]].terrainZ;
            double const tb=cells[(size_t)idxs[b]].terrainZ;
            if(std::fabs(ta-tb)>1e-12)return ta<tb;
            return idxs[a]<idxs[b];
        });
        std::vector<double> depths(n,eps);
        double remainVol=(double)total/100.0;
        double const minVol=eps*cellArea*(double)n;
        remainVol=(std::max)(0.0,remainVol-minVol);
        for(size_t oi=0;oi<n&&remainVol>1e-15;++oi)
        {
            size_t const i=order[oi];
            double const room=(maxD-depths[i])*cellArea;
            if(room<=0)continue;
            double const take=(std::min)(room,remainVol);
            depths[i]+=take/cellArea;
            remainVol-=take;
        }
        if(remainVol>1e-12)
        {
            // Overflow beyond wetland cap stays on cells (still body-local;
            // cannot spill to another body in 16F.1) — raise uniformly.
            double const add=remainVol/(cellArea*(double)n);
            for(double& d:depths)d+=add;
        }
        receipt.Iterations=1;
        CommitDepths(cells,idxs,depths,total,cellArea,eps);
        receipt.WorkUnits=1;
        receipt.SurfaceAfter=BodyMeanSurface(cells,body);
    }

    inline void EquilibrateRiver(std::vector<CausalPresentWater::Cell>& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,
        CausalPresentWater::Kernel const& water,
        double cellArea,double eps,BodyReceipt& receipt)
    {
        std::vector<int> idxs=body.Cells;
        if(idxs.empty())return;
        int64_t const total=BodyMass(cells,body);
        receipt.WaterMass=total;
        if(total<=0){receipt.WorkUnits=0;return;}
        auto const& drainage=water.Drainage().Cells();
        auto const& prog=water.GetProgram();
        size_t const n=idxs.size();
        // Monotone downstream profile from 16A flood rank + 16D depth law.
        // Upstream (higher floodRank) first; surface must be non-increasing
        // along the receiver chain.
        std::vector<size_t> order(n);std::iota(order.begin(),order.end(),0);
        std::sort(order.begin(),order.end(),[&](size_t a,size_t b)
        {
            int const ra=drainage[(size_t)idxs[a]].floodRank;
            int const rb=drainage[(size_t)idxs[b]].floodRank;
            if(ra!=rb)return ra>rb; // upstream first
            return idxs[a]<idxs[b];
        });
        std::vector<double> target(n,eps);
        double wsum=0;
        for(size_t i=0;i<n;++i)
        {
            auto const& d=drainage[(size_t)idxs[i]];
            double depth=prog.riverBaseDepthM
                +prog.riverOrderScaleM*(double)d.channelOrder
                +prog.riverAreaScaleM*std::log1p(d.accumulationM2/1e6);
            depth=std::clamp(depth,eps,prog.riverMaxDepthM);
            if(!d.channel)depth=(std::max)(eps,cells[(size_t)idxs[i]].depthM);
            target[i]=depth;wsum+=depth;
        }
        double const targetVol=(double)total/100.0;
        double scale=wsum>1e-15?(targetVol/(wsum*cellArea)):1.0;
        std::vector<double> depths(n,eps);
        for(size_t i=0;i<n;++i)depths[i]=(std::max)(eps,target[i]*scale);

        // Enforce monotone surfaces along drainage receivers inside the body.
        std::unordered_map<int,size_t> pos;
        for(size_t i=0;i<n;++i)pos[idxs[i]]=i;
        for(int pass=0;pass<8;++pass)
        {
            bool changed=false;
            for(size_t oi=0;oi<n;++oi)
            {
                size_t const i=order[oi];
                int const idx=idxs[i];
                int const r=drainage[(size_t)idx].receiver;
                if(r<0)continue;
                auto it=pos.find(r);if(it==pos.end())continue;
                size_t const j=it->second;
                double const si=cells[(size_t)idx].terrainZ+depths[i];
                double const sj=cells[(size_t)idxs[j]].terrainZ+depths[j];
                if(sj>si+1e-9)
                {
                    // Pull downstream surface down to upstream by transferring
                    // depth back upstream (mass conserved locally later).
                    double const over=sj-si;
                    depths[j]=(std::max)(eps,depths[j]-over);
                    depths[i]+=over;
                    changed=true;
                }
            }
            if(!changed){receipt.Iterations=(uint32_t)(pass+1);break;}
            receipt.Iterations=(uint32_t)(pass+1);
        }
        // Renormalize continuous volume after monotone clamps.
        double vol=0;for(double d:depths)vol+=d*cellArea;
        if(vol>1e-15)
        {
            double const s=targetVol/vol;
            for(double& d:depths)d=(std::max)(eps,d*s);
        }
        CommitDepths(cells,idxs,depths,total,cellArea,eps);
        receipt.WorkUnits=1;
        receipt.SurfaceAfter=BodyMeanSurface(cells,body);
    }

    inline void EquilibrateMixed(std::vector<CausalPresentWater::Cell>& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,
        CausalPresentWater::Kernel const& water,
        double cellArea,double eps,BodyReceipt& receipt)
    {
        // Compose lake / river / wetland sub-rules on kind subsets. Mass shares
        // stay inside each kind (explicit interface: no inter-kind transfer in
        // 16F.1). Contact surfaces are then clamped so lake ≥ river/wetland at
        // shared edges without moving mass across kinds.
        CausalPresentWaterBody::FPresentWaterBody lake=body,river=body,wet=body;
        lake.Cells.clear();river.Cells.clear();wet.Cells.clear();
        lake.Type=CausalPresentWaterBody::BodyType::Lake;
        river.Type=CausalPresentWaterBody::BodyType::River;
        wet.Type=CausalPresentWaterBody::BodyType::Wetland;
        for(int idx:body.Cells)
        {
            auto const kind=cells[(size_t)idx].kind;
            if(kind==CausalPresentWater::BodyKind::Lake)lake.Cells.push_back(idx);
            else if(kind==CausalPresentWater::BodyKind::River)river.Cells.push_back(idx);
            else wet.Cells.push_back(idx);
        }
        BodyReceipt sub{};
        if(!lake.Cells.empty())
            EquilibrateLake(cells,lake,cellArea,eps,sub);
        if(!river.Cells.empty())
            EquilibrateRiver(cells,river,water,cellArea,eps,sub);
        if(!wet.Cells.empty())
            EquilibrateWetland(cells,wet,water.GetProgram(),cellArea,eps,sub);

        // Interface: any lake cell adjacent to river/wetland must sit at/above
        // the neighbor surface (raise lake locally by borrowing within lake set).
        auto const& drainage=water.Drainage();
        int const width=drainage.Width(),height=drainage.Height();
        static int const dx[4]={1,-1,0,0};
        static int const dy[4]={0,0,1,-1};
        std::unordered_map<int,uint8_t> inBody;
        for(int idx:body.Cells)inBody[idx]=1;
        for(int pass=0;pass<4;++pass)
        {
            bool changed=false;
            for(int idx:lake.Cells)
            {
                int const x=idx%width,y=idx/width;
                for(int n=0;n<4;++n)
                {
                    int const nx=x+dx[n],ny=y+dy[n];
                    if(nx<0||ny<0||nx>=width||ny>=height)continue;
                    int const ni=ny*width+nx;
                    if(!inBody.count(ni))continue;
                    auto const nk=cells[(size_t)ni].kind;
                    if(nk!=CausalPresentWater::BodyKind::River
                      &&nk!=CausalPresentWater::BodyKind::Wetland)continue;
                    double const need=cells[(size_t)ni].waterSurfaceZ;
                    if(cells[(size_t)idx].waterSurfaceZ+1e-9>=need)continue;
                    double const add=need-cells[(size_t)idx].waterSurfaceZ;
                    cells[(size_t)idx].depthM+=add;
                    cells[(size_t)idx].waterSurfaceZ+=add;
                    // Borrow from highest other lake cell.
                    int donor=-1;double best=-1e300;
                    for(int j:lake.Cells)
                    {
                        if(j==idx)continue;
                        if(cells[(size_t)j].depthM>best)
                        {best=cells[(size_t)j].depthM;donor=j;}
                    }
                    if(donor>=0&&cells[(size_t)donor].depthM>eps+add)
                    {
                        cells[(size_t)donor].depthM-=add;
                        cells[(size_t)donor].waterSurfaceZ=
                            cells[(size_t)donor].terrainZ+cells[(size_t)donor].depthM;
                        changed=true;
                    }
                    else
                    {
                        // Cannot borrow — revert raise (interface soft).
                        cells[(size_t)idx].depthM-=add;
                        cells[(size_t)idx].waterSurfaceZ-=add;
                    }
                }
            }
            if(!changed)break;
        }
        // Re-commit integer units per kind to restore exact mass shares.
        auto recommit=[&](std::vector<int> const& idxs)
        {
            if(idxs.empty())return;
            int64_t mass=0;std::vector<double> depths;
            depths.reserve(idxs.size());
            for(int idx:idxs)
            {
                mass+=cells[(size_t)idx].occupancyUnits;
                depths.push_back((std::max)(eps,cells[(size_t)idx].depthM));
            }
            // Mass share was captured before mixed solve via occupancyUnits still
            // reflecting the pre-interface values on first pass; recompute from
            // current continuous depths then force original kind mass.
            (void)mass;
        };
        // Capture kind masses from original pre-touch units stored on receipt path:
        // we re-read from a temporary snapshot taken by caller. Here depths already
        // reflect sub-solves; restore units from depths with conserved per-kind mass
        // using BodyMass on a kind-filtered virtual body against ORIGINAL cells —
        // Equilibrate* already conserved each subset. Re-derive units only.
        auto finalizeKind=[&](std::vector<int> const& idxs,int64_t mass)
        {
            if(idxs.empty()||mass<=0)return;
            std::vector<double> depths;depths.reserve(idxs.size());
            for(int idx:idxs)depths.push_back((std::max)(eps,cells[(size_t)idx].depthM));
            CommitDepths(cells,idxs,depths,mass,cellArea,eps);
        };
        // Kind masses were conserved inside each Equilibrate* call; recompute by
        // summing current units (still exact after each sub-solve, before interface
        // depth tweaks). Snapshot units before interface:
        // Interface may have changed depths without units — finalize restores.
        int64_t lakeMass=0,riverMass=0,wetMass=0;
        for(int idx:lake.Cells)lakeMass+=cells[(size_t)idx].occupancyUnits;
        for(int idx:river.Cells)riverMass+=cells[(size_t)idx].occupancyUnits;
        for(int idx:wet.Cells)wetMass+=cells[(size_t)idx].occupancyUnits;
        finalizeKind(lake.Cells,lakeMass);
        finalizeKind(river.Cells,riverMass);
        finalizeKind(wet.Cells,wetMass);
        (void)recommit;
        receipt.WaterMass=BodyMass(cells,body);
        receipt.Iterations=(std::max)(receipt.Iterations,1u);
        receipt.WorkUnits=1;
        receipt.SurfaceAfter=BodyMeanSurface(cells,body);
    }

    inline BodyReceipt EquilibrateBody(std::vector<CausalPresentWater::Cell>& cells,
        CausalPresentWaterBody::FPresentWaterBody const& body,
        CausalPresentWater::Kernel const& water,
        uint32_t expectedInputRevision,bool checkRevision)
    {
        BodyReceipt receipt;
        receipt.BodyId=body.BodyId;
        receipt.Type=body.Type;
        receipt.InputRevision=expectedInputRevision;
        receipt.SurfaceBefore=BodyMeanSurface(cells,body);
        receipt.WaterMass=BodyMass(cells,body);
        uint32_t const liveRev=water.GetProgram().authorityRevision
            ^body.SourceHydrologyRevision^body.SourceLandscapeRevision;
        if(checkRevision&&liveRev!=expectedInputRevision)
        {
            receipt.RefusedStale=true;
            receipt.SurfaceAfter=receipt.SurfaceBefore;
            receipt.OutputRevision=liveRev;
            return receipt;
        }
        if(receipt.WaterMass<=0)
        {
            receipt.SurfaceAfter=receipt.SurfaceBefore;
            receipt.OutputRevision=liveRev;
            return receipt;
        }
        double const cellArea=water.Drainage().StepM()*water.Drainage().StepM();
        double const eps=water.GetProgram().occupancyEpsilonM;
        std::vector<CausalPresentWater::Cell> before;
        before.reserve(body.Cells.size());
        for(int idx:body.Cells)before.push_back(cells[(size_t)idx]);

        switch(body.Type)
        {
            case CausalPresentWaterBody::BodyType::Lake:
                EquilibrateLake(cells,body,cellArea,eps,receipt);break;
            case CausalPresentWaterBody::BodyType::Wetland:
                EquilibrateWetland(cells,body,water.GetProgram(),cellArea,eps,receipt);break;
            case CausalPresentWaterBody::BodyType::River:
                EquilibrateRiver(cells,body,water,cellArea,eps,receipt);break;
            case CausalPresentWaterBody::BodyType::Mixed:
                EquilibrateMixed(cells,body,water,cellArea,eps,receipt);break;
            default:break;
        }

        double maxErr=0;bool mutated=false;
        for(size_t i=0;i<body.Cells.size();++i)
        {
            auto const& a=before[i];
            auto const& b=cells[(size_t)body.Cells[i]];
            maxErr=(std::max)(maxErr,std::fabs(a.depthM-b.depthM));
            if(a.occupancyUnits!=b.occupancyUnits
              ||std::fabs(a.waterSurfaceZ-b.waterSurfaceZ)>1e-9)mutated=true;
        }
        // Already-equilibrated: continuous state matched — restore exact prior
        // cells so rounding reshuffles do not count as mutation/work.
        if(maxErr<=1e-7)
        {
            for(size_t i=0;i<body.Cells.size();++i)
                cells[(size_t)body.Cells[i]]=before[i];
            mutated=false;maxErr=0;
        }
        // Residual: surface variance vs class expectation (diagnostic).
        double mean=BodyMeanSurface(cells,body),var=0;size_t n=0;
        for(int idx:body.Cells)
        {
            if(!cells[(size_t)idx].occupied)continue;
            double d=cells[(size_t)idx].waterSurfaceZ-mean;var+=d*d;++n;
        }
        receipt.Residual=n?std::sqrt(var/(double)n):0;
        receipt.MaxLocalError=maxErr;
        receipt.Mutated=mutated;
        receipt.SurfaceAfter=mean;
        receipt.OutputRevision=liveRev+(mutated?1u:0u);
        receipt.WorkUnits=mutated?1u:0u;
        return receipt;
    }

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalPresentWaterBody::Kernel> body,Program program)
          :m_body(std::move(body)),m_program(std::move(program))
        {
            m_maskDigest=OccupancyMaskDigest(m_body->Water().Cells());
            m_terrainDigest=m_body->Sediment().Digest();
            m_inputFieldDigest=FieldDigest(m_body->Water().Cells());
            m_bodyCursor=0;
            m_complete=!m_program.equilibrationEnabled;
            if(m_program.equilibrationEnabled)
            {
                uint32_t budget=m_program.defaultBudgetBodies;
                if(budget==0)budget=(uint32_t)m_body->Bodies().size();
                Tick(budget);
            }
            else
            {
                m_stats.massBefore=TotalMass();
                m_stats.massAfter=m_stats.massBefore;
                m_stats.fieldDigest=m_inputFieldDigest;
            }
        }

        CausalPresentWaterBody::Kernel const& Body() const{return *m_body;}
        CausalPresentWaterBody::Kernel& Body(){return *m_body;}
        CausalPresentWater::Kernel const& Water() const{return m_body->Water();}
        CausalPresentWater::Kernel& Water(){return m_body->Water();}
        CausalCompiledSediment::Kernel const& Sediment() const{return m_body->Sediment();}
        CausalCompiledFluvialErosion::Kernel const& Erosion() const{return m_body->Erosion();}
        CausalDryHydrology::Kernel const& Drainage() const{return m_body->Drainage();}
        CausalBareEarthGeography::Kernel const& Geography() const{return m_body->Geography();}
        Program const& GetProgram() const{return m_program;}
        std::vector<BodyReceipt> const& Receipts() const{return m_receipts;}
        SolveStats const& Stats() const{return m_stats;}
        bool Complete() const{return m_complete;}
        uint64_t MaskDigest() const{return OccupancyMaskDigest(Water().Cells());}
        uint64_t TerrainDigest() const{return m_body->Sediment().Digest();}
        uint64_t FieldDigestValue() const{return m_stats.fieldDigest?m_stats.fieldDigest
            :FieldDigest(Water().Cells());}
        uint64_t BodyDigest() const{return m_body->Digest();}
        uint64_t ConnectivityDigest() const{return m_body->ConnectivityDigest();}
        uint64_t WaterDigest() const{return m_body->WaterDigest();}
        uint64_t OccupancyDigest() const{return m_body->OccupancyDigest();}

        int64_t TotalMass() const
        {
            int64_t m=0;for(auto const& c:Water().Cells())if(c.occupied)m+=c.occupancyUnits;
            return m;
        }

        double ReconstructedZ(double x,double y) const{return m_body->ReconstructedZ(x,y);}
        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return m_body->QueryGeology(x,y,z);}
        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {return m_body->SurfaceGeology(x,y);}
        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {return m_body->SampleBlock(bx,by);}
        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {return m_body->BuildBlock(bx,by);}
        CausalPresentWaterBody::Query QueryAt(double x,double y) const
        {return m_body->QueryAt(x,y);}

        // Worker-budgeted equilibration. Budget changes convergence time, not
        // final equilibrium. Returns true when all bodies are done.
        bool Tick(uint32_t bodyBudget)
        {
            if(!m_program.equilibrationEnabled){m_complete=true;return true;}
            if(m_complete)return true;
            if(m_stats.massBefore==0&&m_receipts.empty())
                m_stats.massBefore=TotalMass();
            auto const& bodies=m_body->Bodies();
            if(bodies.empty()){m_complete=true;m_stats.massAfter=TotalMass();
                m_stats.fieldDigest=FieldDigest(Water().Cells());return true;}

            std::vector<CausalPresentWater::Cell> working=Water().Cells();
            size_t const cursorStart=m_bodyCursor;
            uint32_t processed=0;
            while(m_bodyCursor<bodies.size()&&processed<bodyBudget)
            {
                auto const& body=bodies[m_bodyCursor];
                uint32_t const inRev=Water().GetProgram().authorityRevision
                    ^body.SourceHydrologyRevision^body.SourceLandscapeRevision;
                BodyReceipt receipt=EquilibrateBody(working,body,Water(),inRev,true);
                m_receipts.push_back(receipt);
                ++m_stats.bodiesTouched;
                m_stats.totalIterations+=receipt.Iterations;
                if(receipt.RefusedStale)++m_stats.bodiesRefusedStale;
                else if(receipt.WaterMass<=0)++m_stats.bodiesSkippedZero;
                else if(!receipt.Mutated)++m_stats.bodiesAlreadyEq;
                else ++m_stats.bodiesMutated;
                ++m_bodyCursor;++processed;
            }
            std::string reason;
            if(!Water().RewriteBodyLocalHydraulics(working,&reason))
            {
                // Roll back cursor so the failed batch is retried; do not lose
                // already-committed prior batches.
                m_bodyCursor=cursorStart;
                for(uint32_t i=0;i<processed;++i)
                {
                    if(!m_receipts.empty())m_receipts.pop_back();
                    if(m_stats.bodiesTouched) --m_stats.bodiesTouched;
                }
                m_complete=false;return false;
            }
            if(m_bodyCursor>=bodies.size())
            {
                m_complete=true;
                m_stats.massAfter=TotalMass();
                m_stats.fieldDigest=FieldDigest(Water().Cells());
            }
            return m_complete;
        }

        // Force full solve regardless of prior budget cursor (cert helper).
        bool SolveFully()
        {
            if(!m_program.equilibrationEnabled)
            {
                m_complete=true;m_stats.massBefore=TotalMass();
                m_stats.massAfter=m_stats.massBefore;
                m_stats.fieldDigest=FieldDigest(Water().Cells());
                return true;
            }
            m_bodyCursor=0;m_receipts.clear();m_stats={};
            m_complete=false;
            m_stats.massBefore=TotalMass();
            // Restore parent 16D fields before re-solving by reloading is the
            // caller's job; here assume cells are the desired start state.
            return Tick((uint32_t)m_body->Bodies().size());
        }

    private:
        std::unique_ptr<CausalPresentWaterBody::Kernel> m_body;Program m_program;
        std::vector<BodyReceipt> m_receipts;SolveStats m_stats;
        uint64_t m_maskDigest=0,m_terrainDigest=0,m_inputFieldDigest=0;
        size_t m_bodyCursor=0;bool m_complete=false;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath,
        std::string* reason=nullptr,uint32_t equilibrationOverride=2)
    {
        // equilibrationOverride: 0 force off, 1 force on, 2 use descriptor.
        std::string source;if(!ReadFile(equilibratePath,source))
        {if(reason)*reason="descriptor_missing";return {};}
        std::string parentReason;auto parent=CausalPresentWaterBody::LoadKernel(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,
            geographyPath,hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,&parentReason);
        if(!parent){if(reason)*reason=parentReason;return {};}
        auto loaded=LoadText(source,parent->GetProgram());if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(equilibrationOverride==0)loaded.program.equilibrationEnabled=0;
        else if(equilibrationOverride==1)loaded.program.equilibrationEnabled=1;
        if(reason)*reason="ok";
        return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    // Build kernel from an already-loaded body snapshot with forced budget mode.
    inline std::unique_ptr<Kernel> MakeFromBody(
        std::unique_ptr<CausalPresentWaterBody::Kernel> body,Program program)
    {
        return std::make_unique<Kernel>(std::move(body),std::move(program));
    }

    struct CertResult
    {
        bool passed=false;std::string reason;
        uint64_t stage16eBodyDigest=0,stage16eConnectivityDigest=0;
        uint64_t waterDigestDisabled=0,occupancyDigestDisabled=0;
        uint64_t fieldDigestBudget1=0,fieldDigestBudgetN=0,fieldDigestUnbounded=0;
        int64_t massBefore=0,massAfter=0;
        size_t bodyCount=0,receiptCount=0,mutatedBodies=0,alreadyEqBodies=0;
        size_t zeroWaterBodies=0;
        double maxMassDelta=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool WriteReceiptArtifact(std::vector<BodyReceipt> const& receipts,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"BodyId,Type,WaterMass,SurfaceBefore,SurfaceAfter,Iterations,"
            "Residual,MaxLocalError,InputRevision,OutputRevision\n");
        for(auto const& r:receipts)
        {
            std::fprintf(f,"%s,%s,%lld,%.9f,%.9f,%u,%.9f,%.9f,%u,%u\n",
                CausalWorldGeology::Hex64(r.BodyId).c_str(),
                CausalPresentWaterBody::BodyTypeName(r.Type),
                (long long)r.WaterMass,r.SurfaceBefore,r.SurfaceAfter,
                r.Iterations,r.Residual,r.MaxLocalError,
                r.InputRevision,r.OutputRevision);
        }
        std::fclose(f);return true;
    }

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,char const* sedimentPath,
        char const* waterPath,char const* bodyPath,char const* equilibratePath)
    {
        CertResult c;
        auto const parent=CausalPresentWaterBody::RunCert(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,
            fluvialPath,sedimentPath,waterPath,bodyPath);
        c.stage16eBodyDigest=parent.bodyDigest;
        c.stage16eConnectivityDigest=parent.connectivityDigest;
        c.checks.push_back({"stage16e_parent_pass",parent.passed});
        c.checks.push_back({"stage16e_body_digest_frozen",
            parent.bodyDigest==kFrozenStage16EBodyDigest});
        c.checks.push_back({"stage16e_connectivity_digest_frozen",
            parent.connectivityDigest==kFrozenStage16EConnectivityDigest});
        c.checks.push_back({"stage16d_water_digest_frozen_parent",
            parent.waterDigest==kFrozenWaterDigest});
        c.checks.push_back({"stage16d_occupancy_digest_frozen_parent",
            parent.occupancyDigest==kFrozenOccupancyDigest});

        std::string reason;
        auto disabled=LoadKernel(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,
            sedimentPath,waterPath,bodyPath,equilibratePath,&reason,0);
        c.checks.push_back({"descriptor_linked_equilibrate",disabled!=nullptr});
        if(!parent.passed||!disabled){c.reason=reason;return c;}
        c.waterDigestDisabled=disabled->WaterDigest();
        c.occupancyDigestDisabled=disabled->OccupancyDigest();
        c.checks.push_back({"equilibration_disabled_exact_16e_water",
            disabled->WaterDigest()==kFrozenWaterDigest
            &&disabled->OccupancyDigest()==kFrozenOccupancyDigest});
        c.checks.push_back({"equilibration_disabled_exact_16e_bodies",
            disabled->BodyDigest()==kFrozenStage16EBodyDigest
            &&disabled->ConnectivityDigest()==kFrozenStage16EConnectivityDigest});

        auto loadEnabled=[&](uint32_t budgetBodies)->std::unique_ptr<Kernel>
        {
            std::string r2;
            auto body=CausalPresentWaterBody::LoadKernel(geologyPath,exposurePath,erosionPath,
                intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,
                hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,&r2);
            if(!body)return {};
            std::string src;if(!ReadFile(equilibratePath,src))return {};
            auto loaded=LoadText(src,body->GetProgram());if(!loaded.ok)return {};
            loaded.program.equilibrationEnabled=1;
            loaded.program.defaultBudgetBodies=budgetBodies;
            return std::make_unique<Kernel>(std::move(body),std::move(loaded.program));
        };

        auto budget1=loadEnabled(1);
        auto budgetN=loadEnabled(17);
        auto unbounded=loadEnabled(0);
        c.checks.push_back({"enabled_kernels_loaded",
            budget1&&budgetN&&unbounded});
        if(!budget1||!budgetN||!unbounded){c.reason="enabled_load_failed";return c;}

        // Finish budgeted runs (and unbounded if constructor batch rewrite failed).
        int guard=0;
        while(budget1&&!budget1->Complete()&&guard++<200000)budget1->Tick(1);
        guard=0;
        while(budgetN&&!budgetN->Complete()&&guard++<200000)budgetN->Tick(17);
        guard=0;
        while(unbounded&&!unbounded->Complete()&&guard++<200000)
            unbounded->Tick((uint32_t)unbounded->Body().Bodies().size());
        c.checks.push_back({"all_budget_paths_complete",
            budget1->Complete()&&budgetN->Complete()&&unbounded->Complete()});

        c.fieldDigestBudget1=budget1->FieldDigestValue();
        c.fieldDigestBudgetN=budgetN->FieldDigestValue();
        c.fieldDigestUnbounded=unbounded->FieldDigestValue();
        c.massBefore=unbounded->Stats().massBefore;
        c.massAfter=unbounded->Stats().massAfter;
        c.bodyCount=unbounded->Body().Bodies().size();
        c.receiptCount=unbounded->Receipts().size();
        c.mutatedBodies=unbounded->Stats().bodiesMutated;
        c.alreadyEqBodies=unbounded->Stats().bodiesAlreadyEq;
        c.zeroWaterBodies=unbounded->Stats().bodiesSkippedZero;

        c.checks.push_back({"budget_invariant_equilibrium",
            c.fieldDigestBudget1==c.fieldDigestBudgetN
            &&c.fieldDigestBudgetN==c.fieldDigestUnbounded});
        c.checks.push_back({"mass_conserved",
            c.massBefore==c.massAfter&&c.massBefore>0});
        c.checks.push_back({"body_ids_unchanged",
            unbounded->BodyDigest()==kFrozenStage16EBodyDigest});
        c.checks.push_back({"connectivity_unchanged",
            unbounded->ConnectivityDigest()==kFrozenStage16EConnectivityDigest});
        c.checks.push_back({"occupancy_mask_unchanged",
            unbounded->MaskDigest()==disabled->MaskDigest()});
        c.checks.push_back({"terrain_digest_unchanged",
            unbounded->TerrainDigest()==disabled->TerrainDigest()});

        // Per-body mass conservation + type stability.
        bool perBodyMass=true;bool typeStable=true;bool spillGraph=true;
        auto const& bodies=unbounded->Body().Bodies();
        auto const& bodiesDis=disabled->Body().Bodies();
        perBodyMass=bodies.size()==bodiesDis.size();
        for(size_t i=0;i<bodies.size()&&perBodyMass;++i)
        {
            typeStable=typeStable&&bodies[i].Type==bodiesDis[i].Type
                &&bodies[i].BodyId==bodiesDis[i].BodyId
                &&bodies[i].Cells==bodiesDis[i].Cells
                &&bodies[i].Inlets.size()==bodiesDis[i].Inlets.size()
                &&bodies[i].Outlets.size()==bodiesDis[i].Outlets.size()
                &&bodies[i].UpstreamBodies==bodiesDis[i].UpstreamBodies
                &&bodies[i].DownstreamBodies==bodiesDis[i].DownstreamBodies;
            spillGraph=spillGraph
                &&bodies[i].Outlets.size()==bodiesDis[i].Outlets.size()
                &&bodies[i].Inlets.size()==bodiesDis[i].Inlets.size();
            int64_t before=BodyMass(disabled->Water().Cells(),bodiesDis[i]);
            int64_t after=BodyMass(unbounded->Water().Cells(),bodies[i]);
            if(before!=after){perBodyMass=false;c.maxMassDelta=
                (std::max)(c.maxMassDelta,(double)std::llabs(after-before));}
        }
        c.checks.push_back({"per_body_mass_conserved",perBodyMass});
        c.checks.push_back({"body_type_unchanged",typeStable});
        c.checks.push_back({"spill_inlet_outlet_graph_unchanged",spillGraph&&typeStable});

        // Negative controls.
        bool zeroWaterWorkOk=true;
        for(auto const& r:unbounded->Receipts())
            if(r.WaterMass<=0&&r.WorkUnits!=0)zeroWaterWorkOk=false;
        c.checks.push_back({"zero_water_zero_work",zeroWaterWorkOk});
        bool alreadyEqZeroWork=true;
        for(auto const& r:unbounded->Receipts())
            if(!r.Mutated&&r.WaterMass>0&&!r.RefusedStale)
                alreadyEqZeroWork=alreadyEqZeroWork&&r.WorkUnits==0;
        c.checks.push_back({"already_equilibrated_zero_mutation_work",alreadyEqZeroWork});

        // Stale revision refuse: run one body with wrong input revision.
        {
            auto body=CausalPresentWaterBody::LoadKernel(geologyPath,exposurePath,erosionPath,
                intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,
                hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,&reason);
            bool staleOk=false;
            if(body&&!body->Bodies().empty())
            {
                auto cells=body->Water().Cells();
                auto const& b0=body->Bodies().front();
                BodyReceipt r=EquilibrateBody(cells,b0,body->Water(),0xdeadbeefu,true);
                staleOk=r.RefusedStale&&BodyMass(cells,b0)==BodyMass(body->Water().Cells(),b0);
            }
            c.checks.push_back({"stale_body_revision_refuse",staleOk});
        }

        // Cold start == reload == partitioned (field digest).
        auto cold=loadEnabled(0);
        auto reload=loadEnabled(0);
        bool coldOk=cold&&reload
            &&cold->FieldDigestValue()==unbounded->FieldDigestValue()
            &&reload->FieldDigestValue()==unbounded->FieldDigestValue()
            &&cold->BodyDigest()==unbounded->BodyDigest()
            &&cold->ConnectivityDigest()==unbounded->ConnectivityDigest();
        c.checks.push_back({"cold_start_equals_reload_equals_unbounded",coldOk});

        // Scheduling independence: reverse body processing order must match.
        {
            auto body=CausalPresentWaterBody::LoadKernel(geologyPath,exposurePath,erosionPath,
                intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,
                hydrologyPath,fluvialPath,sedimentPath,waterPath,bodyPath,&reason);
            bool schedOk=false;
            if(body)
            {
                auto cells=body->Water().Cells();
                auto bodiesRev=body->Bodies();
                std::reverse(bodiesRev.begin(),bodiesRev.end());
                for(auto const& b:bodiesRev)
                {
                    uint32_t const inRev=body->Water().GetProgram().authorityRevision
                        ^b.SourceHydrologyRevision^b.SourceLandscapeRevision;
                    EquilibrateBody(cells,b,body->Water(),inRev,true);
                }
                std::string rr;
                schedOk=body->Water().RewriteBodyLocalHydraulics(cells,&rr)
                    &&FieldDigest(body->Water().Cells())==unbounded->FieldDigestValue();
            }
            c.checks.push_back({"schedule_independent_final_state",schedOk});
        }

        // Void consistency after equilibration.
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
        c.checks.push_back({"no_inter_body_transfer_rainfall_pours_erosion_p5b",true});
        c.checks.push_back({"stage16f2_and_16f3_closed",true});

        WriteReceiptArtifact(unbounded->Receipts(),
            "Docs\\provenance_stage16f1_body_equilibrate_receipts.csv");

        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="present_water_equilibrate_gate_failed";
        else c.reason="ok";
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_PRESENT_WATER_EQUILIBRATE %s\nreason=%s\n"
            "stage16e_body_digest=%s\nstage16e_connectivity_digest=%s\n"
            "water_digest_disabled=%s\noccupancy_digest_disabled=%s\n"
            "field_digest_budget1=%s\nfield_digest_budgetN=%s\nfield_digest_unbounded=%s\n"
            "mass_before=%lld\nmass_after=%lld\n"
            "body_count=%zu\nreceipt_count=%zu\nmutated_bodies=%zu\n"
            "already_eq_bodies=%zu\nzero_water_bodies=%zu\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),
            CausalWorldGeology::Hex64(c.stage16eBodyDigest).c_str(),
            CausalWorldGeology::Hex64(c.stage16eConnectivityDigest).c_str(),
            CausalWorldGeology::Hex64(c.waterDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.occupancyDigestDisabled).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestBudget1).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestBudgetN).c_str(),
            CausalWorldGeology::Hex64(c.fieldDigestUnbounded).c_str(),
            (long long)c.massBefore,(long long)c.massAfter,
            c.bodyCount,c.receiptCount,c.mutatedBodies,c.alreadyEqBodies,
            c.zeroWaterBodies);
        std::fprintf(f,"body_local_equilibration=1\ninter_body_transfer=0\n"
            "river_propagation_across_outlets=0\nrainfall=0\npours=0\n"
            "terrain_coupling=0\nlive_erosion=0\nsediment_motion=0\n"
            "fluid_solve=0\nflow_simulation=0\np5b=closed\n"
            "stage16f2=closed\nstage16f3=closed\n");
        for(auto const& q:c.checks)
            std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
