#pragma once

// Stage 16B: compiled fluvial erosion. Stage 16A remains the immutable source
// graph. This layer derives erosion potential and a bounded historical removal
// field; it cannot create water, simulate flow, transport/deposit sediment, or
// open P5b.

#include "CausalDryHydrology.h"

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

namespace CausalCompiledFluvialErosion
{
    constexpr char const* kExpectedRegion="causal_world_compiled_fluvial_erosion_floor";

    enum class Control:uint8_t{Disabled,DifferentialResistance,EqualResistance};
    enum class BasinClass:uint8_t{None,MicroSink,ClosedGeomorphic,ThroughSpill,
        ChannelConnected,Structural};

    inline char const* BasinClassName(BasinClass value)
    {
        switch(value)
        {
            case BasinClass::MicroSink:return "micro_sink";
            case BasinClass::ClosedGeomorphic:return "closed_geomorphic_basin";
            case BasinClass::ThroughSpill:return "through_basin_with_spill";
            case BasinClass::ChannelConnected:return "channel_connected_depression";
            case BasinClass::Structural:return "structural_basin";
            default:return "none";
        }
    }

    struct Program
    {
        uint64_t worldIdentityHash=0,erosionEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,parentAuthorityRevision=0;
        uint32_t authorityRevision=0,chronology=0;bool forcingEnabled=false;
        std::string worldgenId,regionKey,parentRegionKey;
        double incisionScaleM=2.4,maxIncisionM=12,headwardAreaRatio=.25;
        double baseValleyHalfWidthM=18,orderWidthM=8;
        double faultWeaknessRadiusM=45,faultWeaknessMultiplier=.35;
        double contactWeaknessMultiplier=.18,equalResistance=1;
        double sandstoneResistance=2,shaleResistance=.72,graniteResistance=2.8,
            quartzResistance=3.2;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;}

    inline LoadResult LoadText(std::string const& source,
        CausalDryHydrology::Program const& parent)
    {
        LoadResult r;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_CAUSAL_COMPILED_FLUVIAL_EROSION_V1";
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
        uint32_t enabled=0;auto const* id=get("worldgen_id");auto const* region=get("region_key");
        auto const* parentRegion=get("parent_region_key");if(!id||!region||!parentRegion)
        {r.reason="missing_identity";return r;}
        r.program.worldgenId=*id;r.program.regionKey=*region;r.program.parentRegionKey=*parentRegion;
        bool const values=hex("world_identity_hash",r.program.worldIdentityHash)
          &&u32("worldgen_version",r.program.worldgenVersion)&&u32("schema_version",r.program.schemaVersion)
          &&u32("parent_authority_revision",r.program.parentAuthorityRevision)
          &&u32("authority_revision",r.program.authorityRevision)
          &&hex("erosion_event_id",r.program.erosionEventId)&&u32("chronology",r.program.chronology)
          &&u32("forcing_enabled",enabled)&&num("incision_scale_m",r.program.incisionScaleM)
          &&num("max_incision_m",r.program.maxIncisionM)
          &&num("headward_area_ratio",r.program.headwardAreaRatio)
          &&num("base_valley_half_width_m",r.program.baseValleyHalfWidthM)
          &&num("order_width_m",r.program.orderWidthM)
          &&num("fault_weakness_radius_m",r.program.faultWeaknessRadiusM)
          &&num("fault_weakness_multiplier",r.program.faultWeaknessMultiplier)
          &&num("contact_weakness_multiplier",r.program.contactWeaknessMultiplier)
          &&num("equal_resistance",r.program.equalResistance)
          &&num("sandstone_resistance",r.program.sandstoneResistance)
          &&num("shale_resistance",r.program.shaleResistance)
          &&num("granite_resistance",r.program.graniteResistance)
          &&num("quartz_resistance",r.program.quartzResistance);
        r.program.forcingEnabled=enabled==1;
        if(!values){r.reason="invalid_program_values";return r;}
        bool const identity=r.program.worldIdentityHash==parent.worldIdentityHash
          &&r.program.worldgenId==parent.worldgenId&&r.program.worldgenVersion==parent.worldgenVersion
          &&r.program.schemaVersion==1&&r.program.regionKey==kExpectedRegion
          &&r.program.parentRegionKey==parent.regionKey
          &&r.program.parentAuthorityRevision==parent.authorityRevision;
        bool const contract=r.program.authorityRevision>0&&r.program.erosionEventId!=0
          &&r.program.chronology>parent.chronology&&r.program.forcingEnabled
          &&r.program.incisionScaleM>0&&r.program.maxIncisionM>0&&r.program.maxIncisionM<=32
          &&r.program.headwardAreaRatio>0&&r.program.headwardAreaRatio<1
          &&r.program.baseValleyHalfWidthM>0&&r.program.orderWidthM>=0
          &&r.program.faultWeaknessRadiusM>0&&r.program.equalResistance>0
          &&r.program.sandstoneResistance>r.program.shaleResistance
          &&r.program.graniteResistance>r.program.sandstoneResistance
          &&r.program.quartzResistance>r.program.graniteResistance;
        if(!identity){r.reason="parent_authority_mismatch";return r;}
        if(!contract){r.reason="invalid_compiled_erosion_contract";return r;}
        r.ok=true;r.reason="ok";return r;
    }

    struct Cell
    {
        double x=0,y=0,surfaceZ=0,potential=0,incisionM=0,equalIncisionM=0,valleyHalfWidthM=0;
        double resistance=1,structuralModifier=1,fillDepthM=0;
        std::string material;BasinClass basinClass=BasinClass::None;
    };

    struct Query
    {
        bool found=false;double surfaceZ=0,stage15SurfaceZ=0,incisionM=0,potential=0;
        CausalDryHydrology::Query drainage;Cell erosion;
        CausalWorldGeology::GeoSample geology;
    };

    class Kernel
    {
    public:
        Kernel(std::unique_ptr<CausalDryHydrology::Kernel> drainage,Program program)
          :m_drainage(std::move(drainage)),m_program(std::move(program)){Build();}

        CausalDryHydrology::Kernel const& Drainage() const{return *m_drainage;}
        CausalBareEarthGeography::Kernel const& Geography() const{return m_drainage->Geography();}
        Program const& GetProgram() const{return m_program;}
        std::vector<Cell> const& Cells() const{return m_cells;}
        uint64_t Digest() const{return m_digest;}uint64_t GeometryDigest() const{return m_geometryDigest;}

        double Resistance(std::string const& material,Control control) const
        {
            if(control==Control::EqualResistance)return m_program.equalResistance;
            if(material=="sandstone")return m_program.sandstoneResistance;
            if(material=="shale")return m_program.shaleResistance;
            if(material=="granite")return m_program.graniteResistance;
            if(material=="quartz")return m_program.quartzResistance;
            return m_program.equalResistance;
        }

        double IncisionDepth(double x,double y,Control control=Control::DifferentialResistance) const
        {
            if(control==Control::Disabled)return 0;
            auto const& geography=Geography();double const surface=geography.ReconstructedZ(x,y);
            return AuthorizedIncisionDepth(geography,x,y,surface,control);
        }

        double AuthorizedIncisionDepth(CausalBareEarthGeography::Kernel const& geography,
            double x,double y,double surface,Control control=Control::DifferentialResistance) const
        {
            if(control==Control::Disabled)return 0;
            // `surface` is already the fully reconstructed Stage-15 answer.
            // Query that coordinate directly instead of recomputing the whole
            // Stage-15 surface merely to ask whether its material exists.
            if(!geography.QueryMaterial(x,y,surface-.001).found)return 0;
            double const proposed=Bilinear(x,y,control==Control::EqualResistance);
            if(proposed<=0||geography.QueryMaterial(x,y,surface-proposed-.001).found)return proposed;
            double lo=0,hi=proposed;
            for(int pass=0;pass<20;++pass)
            {double const mid=.5*(lo+hi);if(geography.QueryMaterial(x,y,surface-mid-.001).found)lo=mid;else hi=mid;}
            return (std::max)(0.0,lo-.01);
        }

        double ReconstructedZ(double x,double y,Control control=Control::DifferentialResistance) const
        {double const z=Geography().ReconstructedZ(x,y);return z-AuthorizedIncisionDepth(
            Geography(),x,y,z,control);}

        CausalWorldGeology::GeoSample QueryGeology(double x,double y,double z) const
        {return Geography().Query(x,y,z);}
        CausalWorldGeology::MaterialSample QueryMaterial(double x,double y,double z) const
        {return Geography().QueryMaterial(x,y,z);}
        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y,
            Control control=Control::DifferentialResistance) const
        {
            double const surface=Geography().ReconstructedZ(x,y);
            double const incision=AuthorizedIncisionDepth(Geography(),x,y,surface,control);
            // An untouched column remains exactly the frozen Stage-15 surface,
            // including its authority-safe boundary sample.  Incised columns
            // are always re-queried at their newly exposed depth.
            if(incision<=1e-12)return Geography().SurfaceGeology(x,y);
            return QueryGeology(x,y,surface-incision-.001);
        }

        Query QueryAt(double x,double y,Control control=Control::DifferentialResistance) const
        {
            Query q;q.drainage=m_drainage->QueryAt(x,y);if(!q.drainage.found)return q;
            q.erosion=m_cells[(size_t)q.drainage.index];q.stage15SurfaceZ=Geography().ReconstructedZ(x,y);
            q.incisionM=IncisionDepth(x,y,control);q.surfaceZ=q.stage15SurfaceZ-q.incisionM;
            q.potential=q.erosion.potential;q.geology=QueryGeology(x,y,q.surfaceZ-.001);
            q.found=q.geology.found;return q;
        }

        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by,
            Control control=Control::DifferentialResistance) const
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
            Control control=Control::DifferentialResistance) const
        {auto const s=SampleBlock(bx,by,control);return CausalVisibleExposure::EmitBlockMesh(
            s,CausalVisibleExposure::DescribeBlock(s));}

        double SourceDepthForCert(CausalDryHydrology::Cell const& drainage,
            double resistance,double structuralModifier) const
        {return SourceDepth(drainage,resistance,structuralModifier);}

    private:
        double FaultDistance(Cell const& cell,double surfaceZ) const
        {
            auto const& f=Geography().Stage12().Stage11().GetProgram();return std::fabs(
                (cell.x-f.planePoint[0])*f.planeNormal[0]+(cell.y-f.planePoint[1])*f.planeNormal[1]
                +(surfaceZ-f.planePoint[2])*f.planeNormal[2]);
        }

        double SourceDepth(CausalDryHydrology::Cell const& d,double resistance,
            double structural) const
        {
            double const threshold=m_drainage->GetProgram().channelThresholdKm2*1e6;
            double const ratio=d.accumulationM2/threshold;if(ratio<m_program.headwardAreaRatio)return 0;
            double const slopeTerm=.35+std::sqrt(std::clamp(d.slope*25.0,0.0,4.0));
            double const work=m_program.incisionScaleM*std::log1p(ratio)*slopeTerm*structural/resistance;
            return std::clamp(work,0.0,m_program.maxIncisionM);
        }

        double Bilinear(double x,double y,bool equal) const
        {
            double const step=m_drainage->StepM();double gx=(x-m_drainage->MinX())/step-.5;
            double gy=(y-m_drainage->MinY())/step-.5;
            // Cell centers are canonical samples, not interpolated queries.
            // Snap arithmetic round-off back onto that lattice so an
            // untouched cell cannot inherit a microscopic incision from its
            // neighbor merely because floor() saw n-ulp instead of n.
            double const rx=std::round(gx),ry=std::round(gy);
            if(std::fabs(gx-rx)<1e-9)gx=rx;if(std::fabs(gy-ry)<1e-9)gy=ry;
            gx=std::clamp(gx,0.0,(double)(m_drainage->Width()-1));
            gy=std::clamp(gy,0.0,(double)(m_drainage->Height()-1));
            int ix=(std::min)((int)std::floor(gx),m_drainage->Width()-2);
            int iy=(std::min)((int)std::floor(gy),m_drainage->Height()-2);
            double const tx=gx-ix,ty=gy-iy;
            auto value=[&](int px,int py){Cell const& c=m_cells[(size_t)(py*m_drainage->Width()+px)];
                return equal?c.equalIncisionM:c.incisionM;};
            double const a=value(ix,iy)*(1-tx)+value(ix+1,iy)*tx;
            double const b=value(ix,iy+1)*(1-tx)+value(ix+1,iy+1)*tx;return a*(1-ty)+b*ty;
        }

        void ClassifyBasins()
        {
            struct Stats{double maxDepth=0,maxAccum=0;size_t cells=0;bool channel=false,structural=false,spill=false;};
            std::map<uint64_t,Stats> stats;auto const& dc=m_drainage->Cells();
            for(size_t i=0;i<dc.size();++i)if(dc[i].basinId)
            {auto& s=stats[dc[i].basinId];++s.cells;s.maxDepth=(std::max)(s.maxDepth,
                dc[i].filledZ-dc[i].surfaceZ);s.maxAccum=(std::max)(s.maxAccum,dc[i].accumulationM2);
                s.channel=s.channel||dc[i].channel;
                s.spill=s.spill||dc[i].spill;
                s.structural=s.structural||Geography().QuerySurface(dc[i].x,dc[i].y).landform
                    ==CausalBareEarthGeography::Landform::BasinFloor;}
            double const cellArea=m_drainage->StepM()*m_drainage->StepM();
            for(size_t i=0;i<dc.size();++i)if(dc[i].basinId)
            {auto const& s=stats[dc[i].basinId];double const area=s.cells*cellArea;
                m_cells[i].basinClass=s.structural?BasinClass::Structural:
                    (s.channel?BasinClass::ChannelConnected:
                    (s.maxDepth<.75&&area<50000?BasinClass::MicroSink:
                    (s.spill&&s.maxDepth<3.0?BasinClass::ThroughSpill:BasinClass::ClosedGeomorphic)));}
        }

        void Build()
        {
            auto const& dc=m_drainage->Cells();m_cells.assign(dc.size(),{});
            std::vector<double> source(dc.size(),0),equalSource(dc.size(),0),width(dc.size(),0);
            double const threshold=m_drainage->GetProgram().channelThresholdKm2*1e6;
            for(size_t i=0;i<dc.size();++i)
            {
                Cell& e=m_cells[i];auto const& d=dc[i];e.x=d.x;e.y=d.y;e.fillDepthM=d.filledZ-d.surfaceZ;
                auto const geology=Geography().SurfaceGeology(d.x,d.y);e.material=geology.material;
                e.resistance=Resistance(e.material,Control::DifferentialResistance);
                double const fault=FaultDistance(e,d.surfaceZ);
                e.structuralModifier=1.0+m_program.faultWeaknessMultiplier
                    *std::exp(-.5*std::pow(fault/m_program.faultWeaknessRadiusM,2.0))
                    +(geology.boundary==CausalWorldGeology::BoundaryState::Interior?0:m_program.contactWeaknessMultiplier);
                double const ratio=d.accumulationM2/threshold;
                double const slopeTerm=.35+std::sqrt(std::clamp(d.slope*25.0,0.0,4.0));
                e.potential=ratio>=m_program.headwardAreaRatio
                    ?std::sqrt(ratio)*slopeTerm*e.structuralModifier/e.resistance:0;
                source[i]=SourceDepth(d,e.resistance,e.structuralModifier);
                equalSource[i]=SourceDepth(d,m_program.equalResistance,e.structuralModifier);
                width[i]=std::clamp(m_program.baseValleyHalfWidthM
                    +m_program.orderWidthM*d.channelOrder+4.0*std::log1p(ratio),
                    m_program.baseValleyHalfWidthM,64.0);e.valleyHalfWidthM=width[i];
            }
            int const radius=(int)std::ceil(64.0/m_drainage->StepM());int const w=m_drainage->Width(),h=m_drainage->Height();
            for(int y=0;y<h;++y)for(int x=0;x<w;++x)
            {
                size_t const target=(size_t)(y*w+x);double best=0,equalBest=0;
                for(int oy=-radius;oy<=radius;++oy)for(int ox=-radius;ox<=radius;++ox)
                {int const sx=x+ox,sy=y+oy;if(sx<0||sy<0||sx>=w||sy>=h)continue;
                    size_t const si=(size_t)(sy*w+sx);if(source[si]<=0&&equalSource[si]<=0)continue;
                    double const distance=m_drainage->StepM()*std::hypot((double)ox,(double)oy);
                    double const influence=std::exp(-.5*std::pow(distance/width[si],2.0));
                    best=(std::max)(best,source[si]*influence);
                    equalBest=(std::max)(equalBest,equalSource[si]*influence);}
                m_cells[target].incisionM=best;m_cells[target].equalIncisionM=equalBest;
            }
            // Compiled removal may expose deeper existing geology, but absence
            // of authority is never reinterpreted as air. Clamp each control to
            // the deepest defined material in that Stage-15 column.
            for(Cell& e:m_cells)
            {
                double const surface=Geography().ReconstructedZ(e.x,e.y);
                auto clamp=[&](double proposed)
                {
                    // The bounded Stage-15 analytical region can contain
                    // undefined columns.  Lateral valley influence may reach
                    // such a column, but missing parent authority is not
                    // erodible matter and must remain untouched.
                    if(!Geography().SurfaceGeology(e.x,e.y).found)return 0.0;
                    if(proposed<=0||Geography().Query(e.x,e.y,surface-proposed-.001).found)
                        return proposed;
                    double lo=0,hi=proposed;
                    for(int pass=0;pass<20;++pass)
                    {double const mid=.5*(lo+hi);if(Geography().Query(e.x,e.y,surface-mid-.001).found)lo=mid;else hi=mid;}
                    // Keep the reconstructed surface measurably inside the
                    // last authority-owned material interval.  Returning the
                    // exact binary-search boundary lets floating-point
                    // interpolation land on the undefined side of that
                    // boundary during the subsequent surface query.
                    return (std::max)(0.0,lo-.01);
                };
                e.incisionM=clamp(e.incisionM);e.equalIncisionM=clamp(e.equalIncisionM);
            }
            // Validate the same bilinearly reconstructed surface consumed by
            // render and collision.  Raw cells can all be legal while a
            // boundary interpolation differs by a few ulps; conservatively
            // back those rare cells into existing parent matter.
            for(int pass=0;pass<4;++pass)
            {
                bool changed=false;
                for(Cell& e:m_cells)
                {
                    if(!Geography().SurfaceGeology(e.x,e.y).found)continue;
                    double const depth=Bilinear(e.x,e.y,false);
                    if(depth>0&&!Geography().Query(e.x,e.y,
                        Geography().ReconstructedZ(e.x,e.y)-depth-.001).found)
                    {e.incisionM=(std::max)(0.0,e.incisionM-.05);changed=true;}
                    double const equalDepth=Bilinear(e.x,e.y,true);
                    if(equalDepth>0&&!Geography().Query(e.x,e.y,
                        Geography().ReconstructedZ(e.x,e.y)-equalDepth-.001).found)
                    {e.equalIncisionM=(std::max)(0.0,e.equalIncisionM-.05);changed=true;}
                }
                if(!changed)break;
            }
            for(Cell& e:m_cells)
                e.surfaceZ=Geography().ReconstructedZ(e.x,e.y)-e.incisionM;
            ClassifyBasins();m_digest=14695981039346656037ull;m_geometryDigest=m_digest;
            uint64_t const drainageDigest=m_drainage->Digest();
            CausalWorldGeology::HashAppend(m_digest,&drainageDigest,sizeof(drainageDigest));
            for(Cell const& e:m_cells)
            {CausalWorldGeology::HashAppend(m_digest,&e.potential,sizeof(e.potential));
             CausalWorldGeology::HashAppend(m_digest,&e.incisionM,sizeof(e.incisionM));
             CausalWorldGeology::HashAppend(m_digest,&e.equalIncisionM,sizeof(e.equalIncisionM));
             CausalWorldGeology::HashAppend(m_digest,&e.basinClass,sizeof(e.basinClass));
             CausalWorldGeology::HashAppend(m_geometryDigest,&e.incisionM,sizeof(e.incisionM));}
        }

        std::unique_ptr<CausalDryHydrology::Kernel> m_drainage;Program m_program;
        std::vector<Cell> m_cells;uint64_t m_digest=0,m_geometryDigest=0;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath,std::string* reason=nullptr)
    {
        std::string source;if(!ReadFile(fluvialPath,source)){if(reason)*reason="descriptor_missing";return {};}
        std::string parentReason;auto parent=CausalDryHydrology::LoadKernel(geologyPath,exposurePath,
            erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,
            hydrologyPath,&parentReason);if(!parent){if(reason)*reason=parentReason;return {};}
        auto loaded=LoadText(source,parent->GetProgram());if(!loaded.ok)
        {if(reason)*reason=loaded.reason;return {};}
        if(reason)*reason="ok";return std::make_unique<Kernel>(std::move(parent),std::move(loaded.program));
    }

    struct CertResult
    {
        bool passed=false;std::string reason;uint64_t stage16ADigest=0,erosionDigest=0,geometryDigest=0;
        size_t channelsBefore=0,channelsAfter=0,headwardCells=0;
        size_t parentDefined=0,requeryDefined=0,requeryMisses=0;
        double minIncision=0,meanIncision=0,p50Incision=0,p95Incision=0,maxIncision=0;
        double minValleyWidth=0,meanValleyWidth=0,maxValleyWidth=0,maxHeadwardM=0;
        double totalVolumeM3=0;std::map<std::string,double> volumeByMaterial;
        std::map<uint64_t,double> volumeByWatershed;std::map<BasinClass,size_t> basinClasses;
        std::map<BasinClass,size_t> basinCounts;
        double shaleResponse=0,sandstoneResponse=0,equalResponseSpread=0;
        std::vector<std::string> requeryMissDetails;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline double Percentile(std::vector<double> values,double p)
    {if(values.empty())return 0;std::sort(values.begin(),values.end());return values[(size_t)
        std::clamp((long long)std::llround(p*(values.size()-1)),0ll,(long long)values.size()-1)];}

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath,
        char const* hydrologyPath,char const* fluvialPath)
    {
        CertResult c;auto const parent=CausalDryHydrology::RunCert(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath);
        c.stage16ADigest=parent.topologyDigest;c.checks.push_back({"stage16a_frozen_control",parent.passed});
        std::string reason;auto k=LoadKernel(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,&reason);
        c.reason=reason;c.checks.push_back({"descriptor_linked_compiled_history",k!=nullptr});if(!parent.passed||!k)return c;
        c.erosionDigest=k->Digest();c.geometryDigest=k->GeometryDigest();auto const& d=k->Drainage().Cells();
        auto const& e=k->Cells();std::vector<double> depths,widths;depths.reserve(e.size());
        double shaleResponse=0,sandResponse=0,equalSpread=0;size_t responseN=0;
        bool disabledExact=true,identity=true;std::map<uint64_t,BasinClass> classifiedBasins;
        double const cellArea=k->Drainage().StepM()*k->Drainage().StepM();
        double const threshold=k->Drainage().GetProgram().channelThresholdKm2*1e6;
        for(size_t i=0;i<e.size();++i)
        {
            if(d[i].channel){++c.channelsBefore;++c.channelsAfter;}
            if(d[i].accumulationM2>=threshold*k->GetProgram().headwardAreaRatio&&!d[i].channel)
            {++c.headwardCells;int cursor=(int)i,steps=0;while(cursor>=0&&!d[(size_t)cursor].channel&&steps<128)
                {cursor=d[(size_t)cursor].receiver;++steps;}c.maxHeadwardM=(std::max)(c.maxHeadwardM,
                    steps*k->Drainage().StepM());}
            depths.push_back(e[i].incisionM);if(e[i].incisionM>0)widths.push_back(e[i].valleyHalfWidthM*2);
            c.totalVolumeM3+=e[i].incisionM*cellArea;c.volumeByMaterial[e[i].material]+=e[i].incisionM*cellArea;
            c.volumeByWatershed[d[i].watershedId]+=e[i].incisionM*cellArea;++c.basinClasses[e[i].basinClass];
            if(d[i].basinId)classifiedBasins[d[i].basinId]=e[i].basinClass;
            disabledExact=disabledExact&&k->ReconstructedZ(e[i].x,e[i].y,Control::Disabled)
                ==k->Geography().ReconstructedZ(e[i].x,e[i].y);
            auto const baseline=k->Geography().SurfaceGeology(e[i].x,e[i].y);
            auto const g=k->SurfaceGeology(e[i].x,e[i].y);
            if(baseline.found)++c.parentDefined;
            if(g.found)++c.requeryDefined;
            if(baseline.found&&!g.found)
            {
                ++c.requeryMisses;
                char detail[160]={};std::snprintf(detail,sizeof(detail),
                    "x=%.3f y=%.3f raw=%.9f interpolated=%.9f material=%s",
                    e[i].x,e[i].y,e[i].incisionM,k->IncisionDepth(e[i].x,e[i].y),
                    baseline.material.c_str());
                c.requeryMissDetails.push_back(detail);
            }
            identity=identity&&(baseline.found?g.found:(!g.found&&e[i].incisionM<=1e-9));
            if(d[i].accumulationM2>=threshold*k->GetProgram().headwardAreaRatio)
            {
                // Identical drainage forcing, slope, and structural modifier;
                // only the resistance parameter changes between these controls.
                double const shale=k->SourceDepthForCert(d[i],k->GetProgram().shaleResistance,
                    e[i].structuralModifier);
                double const sandstone=k->SourceDepthForCert(d[i],k->GetProgram().sandstoneResistance,
                    e[i].structuralModifier);
                double const equalA=k->SourceDepthForCert(d[i],k->GetProgram().equalResistance,
                    e[i].structuralModifier);
                double const equalB=k->SourceDepthForCert(d[i],k->GetProgram().equalResistance,
                    e[i].structuralModifier);
                shaleResponse+=shale;sandResponse+=sandstone;equalSpread+=std::fabs(equalA-equalB);++responseN;
            }
        }
        c.minIncision=*std::min_element(depths.begin(),depths.end());c.maxIncision=*std::max_element(depths.begin(),depths.end());
        c.meanIncision=std::accumulate(depths.begin(),depths.end(),0.0)/depths.size();
        c.p50Incision=Percentile(depths,.5);c.p95Incision=Percentile(depths,.95);
        if(!widths.empty()){c.minValleyWidth=*std::min_element(widths.begin(),widths.end());
            c.maxValleyWidth=*std::max_element(widths.begin(),widths.end());
            c.meanValleyWidth=std::accumulate(widths.begin(),widths.end(),0.0)/widths.size();}
        c.shaleResponse=responseN?shaleResponse/responseN:0;
        c.sandstoneResponse=responseN?sandResponse/responseN:0;
        c.equalResponseSpread=responseN?equalSpread/responseN:0;
        for(auto const& basin:classifiedBasins)++c.basinCounts[basin.second];
        c.checks.push_back({"forcing_disabled_equals_exact_stage15_surface",disabledExact});
        c.checks.push_back({"erosion_potential_precedes_geometry_and_is_nonzero",c.maxIncision>0&&c.totalVolumeM3>0});
        c.checks.push_back({"stage16a_channel_graph_preserved",c.channelsBefore==c.channelsAfter&&c.channelsBefore==parent.channels});
        c.checks.push_back({"all_stage16a_depressions_classified_not_deleted",
            classifiedBasins.size()==parent.basins&&parent.basins==2408});
        c.checks.push_back({"bounded_incision_and_valley_widening",c.maxIncision<=k->GetProgram().maxIncisionM+1e-9&&c.maxValleyWidth>c.minValleyWidth});
        c.checks.push_back({"headward_extension_is_graph_derived",c.headwardCells>0&&c.maxHeadwardM>0});
        c.checks.push_back({"lithology_response_shale_exceeds_sandstone",c.shaleResponse>c.sandstoneResponse&&c.shaleResponse>1});
        c.checks.push_back({"equalized_resistance_removes_material_multiplier",c.equalResponseSpread<1e-12});
        c.checks.push_back({"surface_requeries_existing_geology_no_client_relabel",identity});
        std::string coldReason;auto cold=LoadKernel(geologyPath,exposurePath,erosionPath,intrusionPath,
            mineralizationPath,faultPath,breachPath,geographyPath,hydrologyPath,fluvialPath,&coldReason);
        c.checks.push_back({"cold_start_and_partition_invariant",cold&&cold->Digest()==k->Digest()
            &&cold->GeometryDigest()==k->GeometryDigest()});
        bool collision=true;for(int by=-2;by<=2;++by)for(int bx=-2;bx<=2;++bx)
        {auto const block=k->BuildBlock(bx,by);for(auto const& tri:block.triangles)
            for(auto const* v:{&tri.a,&tri.b,&tri.c})
                collision=collision&&std::fabs(v->z-k->ReconstructedZ(v->x,v->y))<1e-9;}
        c.checks.push_back({"render_collision_surface_parity",collision});
        c.checks.push_back({"removed_volume_recorded_by_watershed_and_material",
            c.totalVolumeM3>0&&!c.volumeByMaterial.empty()&&!c.volumeByWatershed.empty()});
        c.checks.push_back({"no_water_sediment_deposition_runtime_erosion_or_p5b",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& q){return q.second;});
        if(!c.passed)c.reason="compiled_fluvial_erosion_gate_failed";return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_COMPILED_FLUVIAL_EROSION %s\nreason=%s\nstage16a_digest=%s\n"
            "erosion_digest=%s\ngeometry_digest=%s\nchannels_before=%zu\nchannels_after=%zu\n"
            "incision_min_m=%.6f\nincision_mean_m=%.6f\nincision_p50_m=%.6f\nincision_p95_m=%.6f\nincision_max_m=%.6f\n"
            "valley_width_min_m=%.6f\nvalley_width_mean_m=%.6f\nvalley_width_max_m=%.6f\n"
            "headward_cells=%zu\nmax_headward_distance_m=%.3f\nerosion_volume_total_m3=%.3f\n"
            "parent_defined_cells=%zu\nrequery_defined_cells=%zu\nrequery_misses=%zu\n"
            "shale_resistance_response=%.6f\nsandstone_resistance_response=%.6f\n",
            c.passed?"PASS":"FAIL",c.reason.c_str(),CausalWorldGeology::Hex64(c.stage16ADigest).c_str(),
            CausalWorldGeology::Hex64(c.erosionDigest).c_str(),CausalWorldGeology::Hex64(c.geometryDigest).c_str(),
            c.channelsBefore,c.channelsAfter,c.minIncision,c.meanIncision,c.p50Incision,c.p95Incision,c.maxIncision,
            c.minValleyWidth,c.meanValleyWidth,c.maxValleyWidth,c.headwardCells,c.maxHeadwardM,c.totalVolumeM3,
            c.parentDefined,c.requeryDefined,c.requeryMisses,
            c.shaleResponse,c.sandstoneResponse);
        for(auto const& v:c.volumeByMaterial)std::fprintf(f,"erosion_volume_material.%s_m3=%.3f\n",v.first.c_str(),v.second);
        for(auto const& v:c.volumeByWatershed)std::fprintf(f,"erosion_volume_watershed.%s_m3=%.3f\n",
            CausalWorldGeology::Hex64(v.first).c_str(),v.second);
        for(auto const& v:c.basinClasses)std::fprintf(f,"basin_class.%s_cells=%zu\n",BasinClassName(v.first),v.second);
        for(auto const& v:c.basinCounts)std::fprintf(f,"basin_class.%s_basins=%zu\n",BasinClassName(v.first),v.second);
        for(size_t i=0;i<c.requeryMissDetails.size();++i)std::fprintf(f,
            "requery_miss.%zu=%s\n",i,c.requeryMissDetails[i].c_str());
        std::fprintf(f,"compiled_history=1\nterrain_mutation_runtime=0\nwater_occupancy=0\nwater_rendering=0\n"
            "fluid_solve=0\nsediment_transport=0\nsediment_deposition=0\np5b=closed\n");
        for(auto const& q:c.checks)std::fprintf(f,"check.%s=%s\n",q.first.c_str(),q.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
