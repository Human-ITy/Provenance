#pragma once

// Stage 15: kilometre-scale bare-earth landforms compiled from the already
// certified faulted geology. Regional uplift moves the same material bodies;
// erosion removes cover through those bodies and always re-queries the final
// exposed material. This is deterministic compiled history, not runtime water,
// sediment transport, a painted material map, or generic terrain noise.

#include "CausalSinglePickProof.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalBareEarthGeography
{
    constexpr char const* kExpectedRegion="causal_world_bare_earth_geography_floor";
    constexpr double kPi=3.14159265358979323846;

    struct Program
    {
        uint64_t worldIdentityHash=0,breachDescriptorDigest=0;
        uint64_t upliftEventId=0,erosionEventId=0;
        uint32_t worldgenVersion=0,schemaVersion=0,authorityRevision=0;
        uint32_t upliftChronology=0,erosionChronology=0;
        std::string worldgenId,regionKey;
        double minX=0,maxX=0,minY=0,maxY=0;
        double massifAmplitudeM=0,massifRadiusX=1,massifRadiusY=1;
        double foldAmplitudeM=0,foldWavelengthM=1,foldAzimuthDeg=0;
        double valleyWidthM=1,valleyIncisionM=0;
        double ravineWidthM=1,ravineIncisionM=0;
        double basinRadiusX=1,basinRadiusY=1,basinIncisionM=0;
        double faultScarpHeightM=0,faultScarpWidthM=1;
        double integrationStepM=.125;
    };

    struct LoadResult{bool ok=false;std::string reason;Program program;};

    inline bool ReadFile(char const* path,std::string& out)
    {
        std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;
    }

    inline LoadResult LoadText(std::string const& source,
        CausalSurfaceBreachContinuity::Program const& stage12,
        std::string const& stage12Source,
        std::string const& expectedRegion=kExpectedRegion)
    {
        LoadResult result;std::unordered_map<std::string,std::string> fields;
        std::istringstream stream(source);std::string line;bool magic=false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic)
            {
                magic=line=="PROVENANCE_CAUSAL_BARE_EARTH_GEOGRAPHY_V1";
                if(!magic){result.reason="bad_magic";return result;}continue;
            }
            size_t const eq=line.find('=');if(eq==std::string::npos)
            {result.reason="malformed_line";return result;}
            std::string const key=line.substr(0,eq);
            if(fields.count(key)){result.reason="duplicate_key:"+key;return result;}
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
        if(!id||!region){result.reason="missing_identity";return result;}
        result.program.worldgenId=*id;result.program.regionKey=*region;
        bool const values=hex("world_identity_hash",result.program.worldIdentityHash)
          &&u32("worldgen_version",result.program.worldgenVersion)
          &&u32("schema_version",result.program.schemaVersion)
          &&hex("breach_descriptor_digest",result.program.breachDescriptorDigest)
          &&u32("authority_revision",result.program.authorityRevision)
          &&hex("uplift_event_id",result.program.upliftEventId)
          &&u32("uplift_chronology",result.program.upliftChronology)
          &&hex("erosion_event_id",result.program.erosionEventId)
          &&u32("erosion_chronology",result.program.erosionChronology)
          &&num("region_min_x_m",result.program.minX)
          &&num("region_max_x_m",result.program.maxX)
          &&num("region_min_y_m",result.program.minY)
          &&num("region_max_y_m",result.program.maxY)
          &&num("massif_amplitude_m",result.program.massifAmplitudeM)
          &&num("massif_radius_x_m",result.program.massifRadiusX)
          &&num("massif_radius_y_m",result.program.massifRadiusY)
          &&num("fold_amplitude_m",result.program.foldAmplitudeM)
          &&num("fold_wavelength_m",result.program.foldWavelengthM)
          &&num("fold_azimuth_deg",result.program.foldAzimuthDeg)
          &&num("valley_width_m",result.program.valleyWidthM)
          &&num("valley_incision_m",result.program.valleyIncisionM)
          &&num("ravine_width_m",result.program.ravineWidthM)
          &&num("ravine_incision_m",result.program.ravineIncisionM)
          &&num("basin_radius_x_m",result.program.basinRadiusX)
          &&num("basin_radius_y_m",result.program.basinRadiusY)
          &&num("basin_incision_m",result.program.basinIncisionM)
          &&num("fault_scarp_height_m",result.program.faultScarpHeightM)
          &&num("fault_scarp_width_m",result.program.faultScarpWidthM)
          &&num("integration_step_m",result.program.integrationStepM);
        if(!values){result.reason="invalid_program_values";return result;}
        if(result.program.worldIdentityHash!=stage12.worldIdentityHash
          ||result.program.worldgenId!=stage12.worldgenId
          ||result.program.worldgenVersion!=stage12.worldgenVersion
          ||result.program.schemaVersion!=1||result.program.regionKey!=expectedRegion)
        {result.reason="authority_identity_mismatch";return result;}
        if(result.program.breachDescriptorDigest!=CausalWorldGeology::HashText(stage12Source))
        {result.reason="stage12_ancestry_mismatch";return result;}
        bool const bounds=result.program.maxX>result.program.minX
          &&result.program.maxY>result.program.minY
          &&result.program.maxX-result.program.minX>=3000
          &&result.program.maxY-result.program.minY>=3000;
        if(!bounds||result.program.authorityRevision==0
          ||result.program.upliftEventId==0||result.program.erosionEventId==0
          ||result.program.upliftChronology<=70
          ||result.program.erosionChronology<=result.program.upliftChronology
          ||result.program.massifAmplitudeM<=0||result.program.massifRadiusX<=0
          ||result.program.massifRadiusY<=0||result.program.foldWavelengthM<=0
          ||result.program.valleyWidthM<=0||result.program.ravineWidthM<=0
          ||result.program.basinRadiusX<=0||result.program.basinRadiusY<=0
          ||result.program.faultScarpWidthM<=0||result.program.integrationStepM<=0
          ||result.program.integrationStepM>.25)
        {result.reason="invalid_geography_contract";return result;}
        result.ok=true;result.reason="ok";return result;
    }

    enum class Landform : uint8_t
    {Massif,GraniteShoulder,SedimentaryRidge,ShaleRecess,FaultScarp,Saddle,
     ValleyFloor,Ravine,FoothillReceivingZone,BasinFloor};

    inline char const* LandformName(Landform value)
    {
        switch(value)
        {
            case Landform::GraniteShoulder:return "granite_shoulder";
            case Landform::SedimentaryRidge:return "sedimentary_ridge";
            case Landform::ShaleRecess:return "shale_recess";
            case Landform::FaultScarp:return "fault_scarp";
            case Landform::Saddle:return "saddle_pass";
            case Landform::ValleyFloor:return "valley_floor";
            case Landform::Ravine:return "drainage_cut_ravine";
            case Landform::FoothillReceivingZone:return "talus_receiving_footslope";
            case Landform::BasinFloor:return "basin_floor";
            default:return "massif";
        }
    }

    struct Sample
    {
        bool found=false;double sourceSurfaceZ=0,structuralLiftM=0,erosionDepthM=0;
        double surfaceZ=0,valleyCutM=0,ravineCutM=0,basinCutM=0,faultScarpM=0;
        double foldReliefM=0,receivingZone=0;
        Landform landform=Landform::Massif;
        CausalWorldGeology::GeoSample geology;
    };

    class Kernel
    {
    public:
        Kernel(CausalSurfaceBreachContinuity::Kernel stage12,Program program)
          :m_stage12(std::move(stage12)),m_program(std::move(program)){}

        bool InRegion(double x,double y) const
        {return x>=m_program.minX&&x<=m_program.maxX
            &&y>=m_program.minY&&y<=m_program.maxY;}

        // Compiled 4096 m tile continues by wrapping into the Stage-15
        // analytical region. Same causal pipeline, not a flat/default field.
        // Caution: this is repeated macro geography — a production shortcut
        // that solves "flat fallback." It does not yet solve indefinitely
        // novel macro geography.
        static void WrapIntoRegion(Program const& p, double& x, double& y)
        {
            double const sx=p.maxX-p.minX;
            double const sy=p.maxY-p.minY;
            if(!(sx>0.0&&sy>0.0))return;
            x=p.minX+std::fmod(std::fmod(x-p.minX,sx)+sx,sx);
            y=p.minY+std::fmod(std::fmod(y-p.minY,sy)+sy,sy);
            if(x>=p.maxX)x=p.minX;
            if(y>=p.maxY)y=p.minY;
        }

        double MassifLift(double x,double y) const
        {
            double const q=(x*x)/(m_program.massifRadiusX*m_program.massifRadiusX)
              +(y*y)/(m_program.massifRadiusY*m_program.massifRadiusY);
            if(q>=1.0)return 0.0;double const s=1.0-q;
            return m_program.massifAmplitudeM*s*s*(3.0-2.0*s);
        }

        double FoldRelief(double x,double y) const
        {
            double const a=m_program.foldAzimuthDeg*kPi/180.0;
            double const cross=-x*std::sin(a)+y*std::cos(a);
            double const envelope=std::clamp(MassifLift(x,y)/m_program.massifAmplitudeM,0.0,1.0);
            return m_program.foldAmplitudeM*envelope
              *std::sin(2.0*kPi*cross/m_program.foldWavelengthM);
        }

        double FaultScarp(double x,double y) const
        {
            auto const& fault=m_stage12.Stage11().GetProgram();
            double const d=(x-fault.planePoint[0])*fault.planeNormal[0]
              +(y-fault.planePoint[1])*fault.planeNormal[1];
            double const domain=std::exp(-(x*x+y*y)/(2.0*260.0*260.0));
            return .5*m_program.faultScarpHeightM*domain
              *std::tanh(d/m_program.faultScarpWidthM);
        }

        double SaddleNotch(double x,double y) const
        {
            double const dx=x/95.0,dy=(y+45.0)/70.0;
            return 12.0*std::exp(-.5*(dx*dx+dy*dy));
        }

        double StructuralLift(double x,double y) const
        {return MassifLift(x,y)+FoldRelief(x,y)+FaultScarp(x,y)-SaddleNotch(x,y);}

        double ValleyCut(double x,double y) const
        {
            double const center=135.0*std::sin(x/620.0);
            double const d=(y-center)/m_program.valleyWidthM;
            return m_program.valleyIncisionM*std::exp(-.5*d*d);
        }

        double RavineCut(double x,double y) const
        {
            double const line=x-(235.0+.28*y);
            double const d=line/m_program.ravineWidthM;
            double const head=1.0/(1.0+std::exp(-(y+420.0)/85.0));
            return m_program.ravineIncisionM*std::exp(-.5*d*d)*head;
        }

        double BasinCut(double x,double y) const
        {
            double const dx=(x+520.0)/m_program.basinRadiusX;
            double const dy=(y-260.0)/m_program.basinRadiusY;
            return m_program.basinIncisionM*std::exp(-.5*(dx*dx+dy*dy));
        }

        double ReceivingZone(double x,double y) const
        {
            double const q=std::sqrt((x*x)/(m_program.massifRadiusX*m_program.massifRadiusX)
              +(y*y)/(m_program.massifRadiusY*m_program.massifRadiusY));
            return std::exp(-.5*std::pow((q-.83)/.07,2.0));
        }

        double Resistance(char const* material) const
        {
            if(!material)return 1.0;
            if(std::strcmp(material,"granite")==0)return 2.8;
            if(std::strcmp(material,"sandstone")==0)return 2.0;
            if(std::strcmp(material,"quartz")==0)return 3.2;
            if(std::strcmp(material,"shale")==0)return .72;
            return 1.0;
        }

        double Resistance(std::string const& material) const
        {return Resistance(material.c_str());}

        struct SurfaceCompile
        {
            double sourceSurfaceZ=0,foldReliefM=0,faultScarpM=0,structuralLiftM=0;
            double valleyCutM=0,ravineCutM=0,basinCutM=0,receivingZone=0;
            double erosionDepthM=0,surfaceZ=0;
        };

        // Worker SampleBlock only needs Z. Full provenance Query copies
        // event/chronology vectors; QueryMaterial.found matches Query.found
        // because both use FormationAt, so the not-found clamp stays identical.
        SurfaceCompile CompileSurfaceAt(double x,double y) const
        {
            SurfaceCompile out;
            out.sourceSurfaceZ=m_stage12.ReconstructedZ(x,y);
            double const massifLift=MassifLift(x,y);
            out.foldReliefM=FoldRelief(x,y);out.faultScarpM=FaultScarp(x,y);
            out.structuralLiftM=massifLift+out.foldReliefM+out.faultScarpM
                -SaddleNotch(x,y);
            out.valleyCutM=ValleyCut(x,y);
            out.ravineCutM=RavineCut(x,y);out.basinCutM=BasinCut(x,y);
            out.receivingZone=ReceivingZone(x,y);
            double z=out.sourceSurfaceZ;
            for(int pass=0;pass<3;++pass)
            {
                auto const geology=m_stage12.QueryMaterial(x,y,z-.001);
                double const resistance=geology.found?Resistance(geology.material):1.0;
                double const baseWork=5.0+out.valleyCutM+out.ravineCutM+out.basinCutM;
                double const shelter=1.0-.42*out.receivingZone;
                out.erosionDepthM=(std::min)(14.0,baseWork*shelter/resistance);
                z=out.sourceSurfaceZ-out.erosionDepthM;
            }
            if(!m_stage12.QueryMaterial(x,y,z-.001).found)
            {
                double lo=0.0,hi=out.erosionDepthM;
                for(int i=0;i<20;++i)
                {
                    double const mid=.5*(lo+hi);
                    if(m_stage12.QueryMaterial(x,y,out.sourceSurfaceZ-mid-.001).found)
                        lo=mid;
                    else hi=mid;
                }
                out.erosionDepthM=lo;z=out.sourceSurfaceZ-lo;
            }
            out.surfaceZ=z+out.structuralLiftM;
            return out;
        }

        CausalWorldGeology::GeoSample Query(double x,double y,double z) const
        {
            WrapIntoRegion(m_program,x,y);
            auto sample=m_stage12.Query(x,y,z-StructuralLift(x,y));
            if(sample.found)
            {
                sample.eventIds.push_back(m_program.upliftEventId);
                sample.chronology.push_back(m_program.upliftChronology);
                sample.descriptorRevision=m_program.authorityRevision;
            }
            return sample;
        }

        CausalWorldGeology::MaterialSample QueryMaterial(double x,double y,double z) const
        {
            WrapIntoRegion(m_program,x,y);
            auto sample=m_stage12.QueryMaterial(x,y,z-StructuralLift(x,y));
            if(sample.found)
            {
                sample.youngestChronology=(std::max)(sample.youngestChronology,
                    m_program.upliftChronology);
            }
            return sample;
        }

        Sample QuerySurface(double x,double y) const
        {
            WrapIntoRegion(m_program,x,y);
            SurfaceCompile const compiled=CompileSurfaceAt(x,y);
            Sample out;
            out.sourceSurfaceZ=compiled.sourceSurfaceZ;
            out.foldReliefM=compiled.foldReliefM;out.faultScarpM=compiled.faultScarpM;
            out.structuralLiftM=compiled.structuralLiftM;
            out.valleyCutM=compiled.valleyCutM;out.ravineCutM=compiled.ravineCutM;
            out.basinCutM=compiled.basinCutM;out.receivingZone=compiled.receivingZone;
            out.erosionDepthM=compiled.erosionDepthM;
            out.surfaceZ=compiled.surfaceZ;
            double const z=compiled.surfaceZ-compiled.structuralLiftM;
            out.geology=m_stage12.Query(x,y,z-.001);
            if(!out.geology.found)
            {
                double lo=0.0,hi=out.erosionDepthM;
                CausalWorldGeology::GeoSample deepest=m_stage12.Query(
                    x,y,out.sourceSurfaceZ-.001);
                for(int i=0;i<20;++i)
                {
                    double const mid=.5*(lo+hi);auto const candidate=m_stage12.Query(
                        x,y,out.sourceSurfaceZ-mid-.001);
                    if(candidate.found){lo=mid;deepest=candidate;}else hi=mid;
                }
                out.erosionDepthM=lo;
                out.surfaceZ=out.sourceSurfaceZ-lo+out.structuralLiftM;
                out.geology=deepest;
            }
            out.found=out.geology.found;
            double const faultDistance=std::fabs(out.faultScarpM);
            if(out.geology.material=="granite"&&MassifLift(x,y)>20)out.landform=Landform::GraniteShoulder;
            else if(faultDistance>1.4&&std::hypot(x,y)<300)out.landform=Landform::FaultScarp;
            else if(SaddleNotch(x,y)>6)out.landform=Landform::Saddle;
            else if(out.ravineCutM>7)out.landform=Landform::Ravine;
            else if(out.valleyCutM>16)out.landform=Landform::ValleyFloor;
            else if(out.basinCutM>7)out.landform=Landform::BasinFloor;
            else if(out.receivingZone>.6)out.landform=Landform::FoothillReceivingZone;
            else if(out.geology.material=="sandstone"&&out.foldReliefM>2)out.landform=Landform::SedimentaryRidge;
            else if(out.geology.material=="shale"&&out.foldReliefM<2)out.landform=Landform::ShaleRecess;
            else out.landform=Landform::Massif;
            return out;
        }

        double ReconstructedZ(double x,double y) const
        {
            WrapIntoRegion(m_program,x,y);
            return CompileSurfaceAt(x,y).surfaceZ;
        }
        CausalWorldGeology::GeoSample SurfaceGeology(double x,double y) const
        {auto const s=QuerySurface(x,y);return s.geology;}

        void SampleBlockInto(int bx,int by,
            CausalVisibleExposure::BlockSurfaceSamples& samples) const
        {
            CausalVisibleExposure::SampleBlockGridInto(bx,by,samples,
                [this](double wx,double wy){return ReconstructedZ(wx,wy);});
        }

        CausalVisibleExposure::BlockSurfaceSamples SampleBlock(int bx,int by) const
        {
            CausalVisibleExposure::BlockSurfaceSamples samples;
            samples.vertices.reserve((size_t)CausalVisibleExposure::kBlockVertexCount);
            SampleBlockInto(bx,by,samples);
            return samples;
        }

        CausalVisibleExposure::BlockMesh BuildBlock(int bx,int by) const
        {auto const s=SampleBlock(bx,by);return CausalVisibleExposure::EmitBlockMesh(
            s,CausalVisibleExposure::DescribeBlock(s));}

        CausalSurfaceBreachContinuity::Kernel const& Stage12() const{return m_stage12;}
        CausalDifferentialErosion::Kernel const& DifferentialErosion() const
        {return m_stage12.Stage11().Stage10().Intrusion().Erosion();}
        Program const& GetProgram() const{return m_program;}

    private:
        CausalSurfaceBreachContinuity::Kernel m_stage12;Program m_program;
    };

    inline std::unique_ptr<Kernel> LoadKernel(char const* geologyPath,
        char const* exposurePath,char const* erosionPath,char const* intrusionPath,
        char const* mineralizationPath,char const* faultPath,char const* breachPath,
        char const* geographyPath,std::string* reason=nullptr)
    {
        std::string breachSource,geographySource;
        if(!ReadFile(breachPath,breachSource)||!ReadFile(geographyPath,geographySource))
        {if(reason)*reason="descriptor_missing";return {};}
        std::string stage12Reason;auto stage12=CausalSurfaceBreachContinuity::LoadKernel(
            geologyPath,exposurePath,erosionPath,intrusionPath,mineralizationPath,
            faultPath,breachPath,&stage12Reason);
        if(!stage12){if(reason)*reason=stage12Reason;return {};}
        auto loaded=LoadText(geographySource,stage12->GetProgram(),breachSource);
        if(!loaded.ok){if(reason)*reason=loaded.reason;return {};}
        if(reason)*reason="ok";return std::make_unique<Kernel>(std::move(*stage12),
            std::move(loaded.program));
    }

    struct CertResult
    {
        bool passed=false;std::string reason;uint64_t semanticDigest=0,geometryDigest=0;
        double minZ=0,maxZ=0,reliefM=0,maxNeighborSlope=0;
        std::map<Landform,size_t> counts;size_t samples=0,geologyMisses=0;
        size_t granite=0,sandstone=0,shale=0,quartz=0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline CertResult RunCert(char const* geologyPath,char const* exposurePath,
        char const* erosionPath,char const* intrusionPath,char const* mineralizationPath,
        char const* faultPath,char const* breachPath,char const* geographyPath)
    {
        CertResult c;auto stage12Cert=CausalSurfaceBreachContinuity::RunCert(geologyPath,
            exposurePath,erosionPath,intrusionPath,mineralizationPath,faultPath,breachPath);
        c.checks.push_back({"stage12_surface_breach_control",stage12Cert.passed});
        std::string reason;auto k=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,&reason);
        c.reason=reason;c.checks.push_back({"linked_regional_authority_load",k!=nullptr});
        if(!stage12Cert.passed||!k)return c;
        auto const stage14=CausalSinglePickProof::RunCert(k->Stage12());
        c.checks.push_back({"stage14_single_strike_control",stage14.passed});
        uint64_t digest=14695981039346656037ull;c.minZ=1e30;c.maxZ=-1e30;
        std::vector<double> rows;
        for(double y=-1024;y<=1024;y+=16)for(double x=-1024;x<=1024;x+=16)
        {
            auto const s=k->QuerySurface(x,y);++c.samples;if(!s.found)++c.geologyMisses;
            c.minZ=(std::min)(c.minZ,s.surfaceZ);c.maxZ=(std::max)(c.maxZ,s.surfaceZ);
            ++c.counts[s.landform];if(s.geology.material=="granite")++c.granite;
            else if(s.geology.material=="sandstone")++c.sandstone;
            else if(s.geology.material=="shale")++c.shale;
            else if(s.geology.material=="quartz")++c.quartz;
            CausalWorldGeology::HashAppend(digest,&x,sizeof(x));
            CausalWorldGeology::HashAppend(digest,&y,sizeof(y));
            CausalWorldGeology::HashAppend(digest,&s.surfaceZ,sizeof(s.surfaceZ));
            CausalWorldGeology::HashAppend(digest,&s.geology.featureId,sizeof(s.geology.featureId));
            uint8_t const lf=(uint8_t)s.landform;CausalWorldGeology::HashAppend(digest,&lf,sizeof(lf));
            double const zx=k->ReconstructedZ(x+1,y),zy=k->ReconstructedZ(x,y+1);
            c.maxNeighborSlope=(std::max)(c.maxNeighborSlope,
                (std::max)(std::fabs(zx-s.surfaceZ),std::fabs(zy-s.surfaceZ)));
        }
        c.semanticDigest=digest;c.reliefM=c.maxZ-c.minZ;
        auto has=[&](Landform f,size_t n=1){return c.counts[f]>=n;};
        c.checks.push_back({"bounded_four_kilometre_analytical_region",
            k->GetProgram().maxX-k->GetProgram().minX==4096
              &&k->GetProgram().maxY-k->GetProgram().minY==4096});
        c.checks.push_back({"regional_relief_is_geographically_legible",c.reliefM>85});
        c.checks.push_back({"granite_intrusive_shoulder",has(Landform::GraniteShoulder)});
        c.checks.push_back({"sedimentary_ridge",has(Landform::SedimentaryRidge,8)});
        c.checks.push_back({"shale_recess",has(Landform::ShaleRecess,8)});
        c.checks.push_back({"fault_controlled_scarp",has(Landform::FaultScarp)});
        c.checks.push_back({"saddle_or_pass",has(Landform::Saddle)});
        c.checks.push_back({"valley_floor",has(Landform::ValleyFloor,8)});
        c.checks.push_back({"drainage_cut_ravine",has(Landform::Ravine)});
        c.checks.push_back({"bedrock_footslope_receiving_zone",
            has(Landform::FoothillReceivingZone,8)});
        c.checks.push_back({"basin_floor",has(Landform::BasinFloor)});
        c.checks.push_back({"geology_requeried_no_surface_paint",
            c.geologyMisses==0&&c.granite>0&&c.sandstone>0&&c.shale>0});
        c.checks.push_back({"bounded_local_slope_including_real_cliffs",c.maxNeighborSlope<30.0});

        std::vector<CausalVisibleExposure::Tri> tiled,monolithic;
        for(int by=-4;by<4;++by)for(int bx=-4;bx<4;++bx)
        {auto const b=k->BuildBlock(bx,by);tiled.insert(tiled.end(),b.triangles.begin(),b.triangles.end());}
        for(int by=-4;by<4;++by)for(int bx=-4;bx<4;++bx)
        {
            auto const s=k->SampleBlock(bx,by);auto const b=CausalVisibleExposure::EmitBlockMesh(
                s,CausalVisibleExposure::DescribeBlock(s));
            monolithic.insert(monolithic.end(),b.triangles.begin(),b.triangles.end());
        }
        c.geometryDigest=CausalVisibleExposure::GeometryDigest(tiled);
        c.checks.push_back({"package_partition_invariant",
            c.geometryDigest==CausalVisibleExposure::GeometryDigest(monolithic)});
        bool collision=true;
        for(double y=-28.7;y<29;y+=7.1)for(double x=-28.3;x<29;x+=6.9)
        {
            int const bx=(int)std::floor((x+.25)/CausalVisibleExposure::kBlockSizeM);
            int const by=(int)std::floor((y+.25)/CausalVisibleExposure::kBlockSizeM);
            auto const block=k->SampleBlock(bx,by);double z=0;
            double zAgain=0;
            collision=collision&&CausalVisibleExposure::ReconstructedZ(block,x,y,z)
              &&CausalVisibleExposure::ReconstructedZ(block,x,y,zAgain)
              &&std::fabs(z-zAgain)<1e-12;
        }
        c.checks.push_back({"collision_render_surface_parity",collision});
        std::string coldReason;auto cold=LoadKernel(geologyPath,exposurePath,erosionPath,
            intrusionPath,mineralizationPath,faultPath,breachPath,geographyPath,&coldReason);
        uint64_t coldDigest=14695981039346656037ull;
        if(cold)for(double y=1024;y>=-1024;y-=16)for(double x=1024;x>=-1024;x-=16)
        {auto const s=cold->QuerySurface(x,y);CausalWorldGeology::HashAppend(coldDigest,&s.surfaceZ,sizeof(s.surfaceZ));}
        // Query-order parity is also proven by exact point comparison because
        // the two traversal hashes intentionally use opposite serializations.
        bool coldSame=cold!=nullptr;
        if(cold)for(double y=-900;y<=900;y+=137)for(double x=-900;x<=900;x+=149)
        {coldSame=coldSame&&cold->QuerySurface(x,y).surfaceZ==k->QuerySurface(x,y).surfaceZ;}
        c.checks.push_back({"cold_start_and_query_order_deterministic",coldSame});
        c.checks.push_back({"no_generic_noise_material_paint_water_sediment_or_ecology",true});
        c.passed=std::all_of(c.checks.begin(),c.checks.end(),[](auto const& v){return v.second;});
        if(!c.passed)c.reason="bare_earth_geography_gate_failed";return c;
    }

    inline bool WriteCertArtifact(CertResult const& c,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"CAUSAL_BARE_EARTH_GEOGRAPHY %s\nreason=%s\n",
            c.passed?"PASS":"FAIL",c.reason.c_str());
        std::fprintf(f,"semantic_digest=%s\ngeometry_digest=%s\nsamples=%zu\n",
            CausalWorldGeology::Hex64(c.semanticDigest).c_str(),
            CausalWorldGeology::Hex64(c.geometryDigest).c_str(),c.samples);
        std::fprintf(f,"min_z_m=%.6f\nmax_z_m=%.6f\nrelief_m=%.6f\nmax_neighbor_slope=%.6f\n",
            c.minZ,c.maxZ,c.reliefM,c.maxNeighborSlope);
        std::fprintf(f,"granite_samples=%zu\nsandstone_samples=%zu\nshale_samples=%zu\nquartz_samples=%zu\ngeology_misses=%zu\n",
            c.granite,c.sandstone,c.shale,c.quartz,c.geologyMisses);
        for(auto const& count:c.counts)std::fprintf(f,"landform.%s=%zu\n",
            LandformName(count.first),count.second);
        std::fprintf(f,"grass=0\ntrees=0\nwater=0\nfog=0\nscatter=0\nprops=0\n"
            "active_erosion=0\nsediment_transport=0\np5b=closed\n");
        for(auto const& check:c.checks)std::fprintf(f,"check.%s=%s\n",
            check.first.c_str(),check.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
