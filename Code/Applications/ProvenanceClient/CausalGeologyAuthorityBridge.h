#pragma once

// FableScript -> Esoterica geology authority bridge.
//
// FableScript owns the history descriptor and the independently evaluated
// hostile-query oracle.  Esoterica loads those products and proves that its
// Stage-10 reference evaluator returns the same geological answer.

#include "CausalContactMineralization.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CausalGeologyAuthorityBridge
{
    struct OracleRow
    {
        double x=0.0,y=0.0,z=0.0;
        bool found=false,intrusionInside=false,depositAdmitted=false;
        std::string material,body;
        uint64_t featureId=0,depositSystemId=0,depositBodyId=0;
        std::array<double,3> normal={0.0,0.0,0.0};
        double contactDistanceM=0.0,permeability=0.0;
        std::vector<uint32_t> chronology;
        std::vector<uint64_t> eventIds;
    };

    struct CertResult
    {
        bool passed=false;
        std::string loadReason;
        size_t queryCount=0,depositQueries=0,intrusionQueries=0;
        uint64_t bridgeDigest=0,oracleDigest=0,semanticDigest=0;
        double maxNormalError=0.0,maxContactErrorM=0.0,maxPermeabilityError=0.0;
        std::vector<std::pair<std::string,bool>> checks;
    };

    inline bool ReadFile(char const* path,std::string& out)
    {
        std::ifstream input(path,std::ios::binary);if(!input)return false;
        std::ostringstream bytes;bytes<<input.rdbuf();out=bytes.str();return true;
    }

    inline std::unordered_map<std::string,std::string> ParseBridgeScalars(
        std::string const& text,size_t& eventCount,size_t& featureCount,
        size_t& systemCount,size_t& bodyCount)
    {
        std::unordered_map<std::string,std::string> out;
        std::istringstream stream(text);std::string line;bool magic=false;
        eventCount=featureCount=systemCount=bodyCount=0;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            if(!magic){magic=line=="PROVENANCE_GEOLOGY_AUTHORITY_BRIDGE_V1";continue;}
            size_t const eq=line.find('=');if(eq==std::string::npos)continue;
            std::string const key=line.substr(0,eq),value=line.substr(eq+1);
            if(key=="event")++eventCount;
            else if(key=="feature")++featureCount;
            else if(key=="deposit_system")++systemCount;
            else if(key=="deposit_body")++bodyCount;
            else out[key]=value;
        }
        if(!magic)out.clear();return out;
    }

    inline bool ParseBool(std::string const& s,bool& out)
    {if(s=="0"){out=false;return true;}if(s=="1"){out=true;return true;}return false;}

    inline bool ParseOracle(std::string const& text,std::vector<OracleRow>& rows,
        uint64_t& declaredDigest)
    {
        size_t const firstNl=text.find('\n');
        if(firstNl==std::string::npos||text.rfind("# oracle_digest=",0)!=0)return false;
        if(!CausalWorldGeology::ParseHex64(text.substr(16,firstNl-16),declaredDigest))return false;
        std::string const payload=text.substr(firstNl+1);
        if(CausalWorldGeology::HashText(payload)!=declaredDigest)return false;
        std::istringstream stream(payload);std::string line;
        if(!std::getline(stream,line)||line.rfind("x\ty\tz\tfound",0)!=0)return false;
        while(std::getline(stream,line))
        {
            if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;
            auto const f=CausalWorldGeology::Split(line,'\t');if(f.size()!=18)return false;
            OracleRow r;
            if(!CausalWorldGeology::ParseDouble(f[0],r.x)
              ||!CausalWorldGeology::ParseDouble(f[1],r.y)
              ||!CausalWorldGeology::ParseDouble(f[2],r.z)
              ||!ParseBool(f[3],r.found)
              ||!CausalWorldGeology::ParseHex64(f[5],r.featureId)
              ||!CausalWorldGeology::ParseHex64(f[7],r.depositSystemId)
              ||!CausalWorldGeology::ParseHex64(f[8],r.depositBodyId)
              ||!CausalWorldGeology::ParseDouble(f[9],r.normal[0])
              ||!CausalWorldGeology::ParseDouble(f[10],r.normal[1])
              ||!CausalWorldGeology::ParseDouble(f[11],r.normal[2])
              ||!ParseBool(f[12],r.intrusionInside)||!ParseBool(f[13],r.depositAdmitted)
              ||!CausalWorldGeology::ParseDouble(f[14],r.contactDistanceM)
              ||!CausalWorldGeology::ParseDouble(f[15],r.permeability))return false;
            r.material=f[4];r.body=f[6];
            if(f[16]!="-")for(auto const& v:CausalWorldGeology::Split(f[16],'|'))
            {uint32_t n=0;if(!CausalWorldGeology::ParseU32(v,n))return false;r.chronology.push_back(n);}
            if(f[17]!="-")for(auto const& v:CausalWorldGeology::Split(f[17],'|'))
            {uint64_t n=0;if(!CausalWorldGeology::ParseHex64(v,n))return false;r.eventIds.push_back(n);}
            rows.push_back(std::move(r));
        }
        return !rows.empty();
    }

    inline CertResult RunCert(char const* bridgePath,char const* oraclePath,
        char const* geologyPath,char const* exposurePath,char const* erosionPath,
        char const* intrusionPath,char const* mineralizationPath)
    {
        CertResult cert;std::string bridge,oracle,gs,es,ers,is,ms;
        bool const files=ReadFile(bridgePath,bridge)&&ReadFile(oraclePath,oracle)
          &&ReadFile(geologyPath,gs)&&ReadFile(exposurePath,es)&&ReadFile(erosionPath,ers)
          &&ReadFile(intrusionPath,is)&&ReadFile(mineralizationPath,ms);
        cert.checks.push_back({"all_authority_products_present",files});
        if(!files){cert.loadReason="authority_product_missing";return cert;}
        cert.bridgeDigest=CausalWorldGeology::HashText(bridge);

        size_t events=0,features=0,systems=0,bodies=0;
        auto const scalar=ParseBridgeScalars(bridge,events,features,systems,bodies);
        bool envelope=!scalar.empty()&&scalar.count("authority_owner")
          &&scalar.at("authority_owner")=="FableScript"&&scalar.count("evaluator_role")
          &&scalar.at("evaluator_role")=="Esoterica_reference_client";
        cert.checks.push_back({"fablescript_owns_authority",envelope});
        bool vocabulary=events>=7&&features>=8&&systems==1&&bodies==1;
        cert.checks.push_back({"stage5_to_10_vocabulary_complete",vocabulary});
        auto digestMatches=[&](char const* key,std::string const& source)
        {auto it=scalar.find(key);uint64_t value=0;return it!=scalar.end()
            &&CausalWorldGeology::ParseHex64(it->second,value)
            &&value==CausalWorldGeology::HashText(source);};
        bool links=digestMatches("geology_descriptor_digest",gs)
          &&digestMatches("exposure_descriptor_digest",es)
          &&digestMatches("erosion_descriptor_digest",ers)
          &&digestMatches("intrusion_descriptor_digest",is)
          &&digestMatches("mineralization_descriptor_digest",ms);
        cert.checks.push_back({"source_descriptor_digest_links",links});
        size_t const digestLine=bridge.rfind("bridge_payload_digest=");uint64_t payloadDigest=0;
        bool payload=digestLine!=std::string::npos
          &&CausalWorldGeology::ParseHex64(bridge.substr(digestLine+22,16),payloadDigest)
          &&payloadDigest==CausalWorldGeology::HashText(bridge.substr(0,digestLine));
        cert.checks.push_back({"bridge_payload_digest",payload});

        std::vector<OracleRow> rows;
        bool const oracleLoaded=ParseOracle(oracle,rows,cert.oracleDigest);
        cert.checks.push_back({"fablescript_reference_oracle_load",oracleLoaded});

        auto geology=CausalWorldGeology::LoadText(gs);
        auto exposure=geology.ok?CausalWorldExposure::LoadText(es,gs):CausalWorldExposure::LoadResult{};
        CausalDifferentialErosion::LoadResult erosion;
        CausalGraniteIntrusion::LoadResult intrusion;
        CausalContactMineralization::LoadResult mineralization;
        if(geology.ok&&exposure.ok)erosion=CausalDifferentialErosion::LoadText(
            ers,geology.descriptor,exposure.descriptor,gs);
        if(erosion.ok)intrusion=CausalGraniteIntrusion::LoadText(
            is,geology.descriptor,exposure.descriptor,gs,ers);
        if(intrusion.ok)mineralization=CausalContactMineralization::LoadText(ms,intrusion.program,is);
        bool const linked=geology.ok&&exposure.ok&&erosion.ok&&intrusion.ok&&mineralization.ok;
        cert.checks.push_back({"esoterica_linked_evaluator_load",linked});
        if(!oracleLoaded||!linked){cert.loadReason="linked_evaluator_or_oracle_refused";return cert;}

        CausalContactMineralization::Kernel kernel(
            CausalGraniteIntrusion::Kernel(
                CausalDifferentialErosion::Kernel(
                    CausalWorldExposure::Kernel(geology.descriptor,exposure.descriptor),erosion.program),
                intrusion.program),mineralization.program);
        bool material=true,identity=true,body=true,structure=true,inside=true,contact=true,
            chronology=true,order=true;
        uint64_t semantic=14695981039346656037ull;
        for(OracleRow const& expected:rows)
        {
            auto const actual=kernel.Query(true,expected.x,expected.y,expected.z);
            auto const host=kernel.Intrusion().Query(false,expected.x,expected.y,expected.z);
            double const actualContact=kernel.ContactDistanceM(expected.x,expected.y,expected.z);
            double const actualPerm=host.found?kernel.Permeability(host,expected.x,expected.y,expected.z):0.0;
            bool const actualInside=kernel.Intrusion().Occupies(expected.x,expected.y,expected.z);
            bool const actualDeposit=actual.featureId==mineralization.program.featureId;
            material=material&&actual.found==expected.found
                &&(!expected.found||actual.material==expected.material);
            identity=identity&&actual.featureId==expected.featureId
                &&(actualDeposit?mineralization.program.depositSystemId:0)==expected.depositSystemId
                &&(actualDeposit?actual.featureId:0)==expected.depositBodyId;
            body=body&&(!expected.found||actual.formationId==expected.body);
            if(expected.found)for(int i=0;i<3;++i)cert.maxNormalError=(std::max)(cert.maxNormalError,
                std::fabs(actual.structuralNormal[i]-expected.normal[i]));
            structure=structure&&cert.maxNormalError<=1e-12;
            inside=inside&&actualInside==expected.intrusionInside&&actualDeposit==expected.depositAdmitted;
            cert.maxContactErrorM=(std::max)(cert.maxContactErrorM,
                std::fabs(actualContact-expected.contactDistanceM));
            cert.maxPermeabilityError=(std::max)(cert.maxPermeabilityError,
                std::fabs(actualPerm-expected.permeability));
            contact=contact&&cert.maxContactErrorM<=1e-11&&cert.maxPermeabilityError<=1e-11;
            chronology=chronology&&actual.chronology==expected.chronology&&actual.eventIds==expected.eventIds;
            if(actualDeposit)++cert.depositQueries;if(actualInside)++cert.intrusionQueries;
            CausalWorldGeology::HashAppend(semantic,&actual.featureId,sizeof(actual.featureId));
            CausalWorldGeology::HashAppend(semantic,actual.material.data(),actual.material.size());
        }
        cert.queryCount=rows.size();cert.semanticDigest=semantic;
        cert.checks.push_back({"material_parity",material});
        cert.checks.push_back({"feature_deposit_identity_parity",identity});
        cert.checks.push_back({"formation_body_parity",body});
        cert.checks.push_back({"structural_frame_parity",structure});
        cert.checks.push_back({"inside_outside_admission_parity",inside});
        cert.checks.push_back({"contact_permeability_parity",contact});
        cert.checks.push_back({"chronology_event_parity",chronology});
        // Re-evaluate in reverse to prove the client has no query-order state.
        for(auto it=rows.rbegin();it!=rows.rend();++it)
        {auto const a=kernel.Query(true,it->x,it->y,it->z);order=order&&a.featureId==it->featureId;}
        cert.checks.push_back({"hostile_reverse_order_parity",order});
        cert.checks.push_back({"oracle_contains_intrusion_and_deposit",cert.intrusionQueries>0&&cert.depositQueries>0});
        cert.passed=true;for(auto const& c:cert.checks)cert.passed=cert.passed&&c.second;
        cert.loadReason=cert.passed?"ok":"parity_mismatch";return cert;
    }

    inline bool WriteCertArtifact(CertResult const& cert,char const* path)
    {
        FILE* f=nullptr;if(fopen_s(&f,path,"wb")!=0||!f)return false;
        std::fprintf(f,"FABLESCRIPT_ESOTERICA_GEOLOGY_AUTHORITY_PARITY %s\n",cert.passed?"PASS":"FAIL");
        std::fprintf(f,"load_reason=%s\nqueries=%zu\nintrusion_queries=%zu\ndeposit_queries=%zu\n",
            cert.loadReason.c_str(),cert.queryCount,cert.intrusionQueries,cert.depositQueries);
        std::fprintf(f,"bridge_digest=%s\noracle_digest=%s\nsemantic_digest=%s\n",
            CausalWorldGeology::Hex64(cert.bridgeDigest).c_str(),
            CausalWorldGeology::Hex64(cert.oracleDigest).c_str(),
            CausalWorldGeology::Hex64(cert.semanticDigest).c_str());
        std::fprintf(f,"max_normal_error=%.17g\nmax_contact_error_m=%.17g\nmax_permeability_error=%.17g\n",
            cert.maxNormalError,cert.maxContactErrorM,cert.maxPermeabilityError);
        std::fprintf(f,"authority_rule=FableScript determines what geological history exists; Esoterica evaluates locally and may not originate a competing answer.\n");
        std::fprintf(f,"occupancy=deferred_to_cut_B\nd2=deferred_to_cut_C\nfault=closed\np5b=closed\n");
        for(auto const& c:cert.checks)std::fprintf(f,"check.%s=%s\n",c.first.c_str(),c.second?"PASS":"FAIL");
        std::fclose(f);return true;
    }
}
