#pragma once

// EI0.A is contract identity only. This header has no terrain, rendering,
// mutation, residency, or transport dependencies.

#include <cstdlib>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "WorldDescriptors.generated.h"

namespace Ei0a
{
    constexpr char const* kProtocolId = "fablescript.embodied-terrain";
    constexpr char const* kProtocolSemver = "1.1.0";
    constexpr char const* kSchemaDigest =
        "1219f83ca0241bfb807ee6a58678e84a1a7c90aa29d3ba03beef04d34e2b31fd";
    constexpr char const* kDescriptorSchemaDigest = Ms1::kWorldDescriptorSchemaDigest;
    constexpr char const* kAuthorityMode = "local_server_authoritative";
    constexpr char const* kClientBuild =
        "7fa70056006e6c8cb0d35e9e838da31bfcb8c0d7+ei3";

    struct PendingRequest
    {
        int kind = 0;
        int bx = 0;
        int by = 0;
    };

    enum class Correlation
    {
        Matched,
        MissingId,
        UnknownId,
        DuplicateCompletedId,
    };

    inline bool ExtractString( std::string const& json, char const* key, std::string& out )
    {
        std::string needle = std::string( "\"" ) + key + "\":\"";
        size_t p = json.find( needle );
        if ( p == std::string::npos )
        {
            needle = std::string( "\"" ) + key + "\": \"";
            p = json.find( needle );
            if ( p == std::string::npos ) { return false; }
        }
        p += needle.size();
        size_t const e = json.find( '"', p );
        if ( e == std::string::npos ) { return false; }
        out.assign( json, p, e-p );
        return true;
    }

    inline bool ExtractEnvelopeId( std::string const& json, std::string& out )
    {
        return ExtractString( json, "id", out ) && !out.empty();
    }

    inline bool ExtractJsonArray( std::string const& json, char const* key,
                                  std::string& out )
    {
        std::string const needle = std::string( "\"" ) + key + "\":";
        size_t p = json.find( needle );
        if ( p == std::string::npos ) { return false; }
        p += needle.size();
        while ( p < json.size() && ( json[p] == ' ' || json[p] == '\t' ) ) { ++p; }
        if ( p == json.size() || json[p] != '[' ) { return false; }
        size_t const begin = p;
        int depth = 0; bool quoted = false; bool escaped = false;
        for ( ; p < json.size(); ++p )
        {
            char const c = json[p];
            if ( quoted )
            {
                if ( escaped ) { escaped = false; }
                else if ( c == '\\' ) { escaped = true; }
                else if ( c == '"' ) { quoted = false; }
                continue;
            }
            if ( c == '"' ) { quoted = true; continue; }
            if ( c == '[' ) { ++depth; }
            else if ( c == ']' && --depth == 0 )
            { out.assign( json, begin, p - begin + 1 ); return true; }
        }
        return false;
    }

    class RequestTracker
    {
    public:
        bool Register( std::string const& requestId, PendingRequest request )
        {
            if ( requestId.empty() || active_.count( requestId ) || completed_.count( requestId ) )
            { return false; }
            active_.emplace( requestId, request );
            return true;
        }

        Correlation Complete( std::string const& response, PendingRequest& request,
                              std::string& responseId )
        {
            if ( !ExtractEnvelopeId( response, responseId ) )
            { return Correlation::MissingId; }
            auto const found = active_.find( responseId );
            if ( found == active_.end() )
            {
                return completed_.count( responseId )
                    ? Correlation::DuplicateCompletedId : Correlation::UnknownId;
            }
            request = found->second;
            active_.erase( found );
            completed_.insert( responseId );
            return Correlation::Matched;
        }

        void Clear()
        {
            active_.clear();
            completed_.clear();
        }

        size_t ActiveCount() const { return active_.size(); }
        size_t CompletedCount() const { return completed_.size(); }
        void Cancel( std::string const& requestId ) { active_.erase( requestId ); }

    private:
        std::unordered_map<std::string, PendingRequest> active_;
        std::unordered_set<std::string> completed_;
    };

    inline std::string BuildClientHelloParams( std::string const& requestId,
                                               char const* laneRole = "control",
                                               std::string const& sessionToken = {},
                                               std::string const& serverInstanceId = {},
                                               std::string const& worldUuid = {},
                                               std::string const& macroGenesisDigest = {},
                                               std::string const& worldBaselineDigest = {},
                                               char const* projectionModesJson =
                                                   "[\"surface_field\",\"world_snapshot\",\"cell_surface\"]" )
    {
        std::string result =
            "{\"request_id\":\"" + requestId
            + "\",\"protocol_id\":\"" + kProtocolId
            + "\",\"protocol_semver\":\"" + kProtocolSemver
            + "\",\"supported_schema_digest\":\"" + kSchemaDigest
            + "\",\"client_build\":\"" + kClientBuild
            + "\",\"requested_authority_mode\":\"" + kAuthorityMode
            + "\",\"requested_projection_modes\":" + projectionModesJson
            + ",\"lane_role\":\"" + laneRole + "\"";
        if ( !sessionToken.empty() )
        { result += ",\"session_token\":\"" + sessionToken + "\""; }
        if ( !serverInstanceId.empty() )
        { result += ",\"server_instance_id\":\"" + serverInstanceId + "\""; }
        if ( !worldUuid.empty() )
        { result += ",\"world_uuid\":\"" + worldUuid + "\""; }
        if ( !macroGenesisDigest.empty() )
        { result += ",\"macro_genesis_digest\":\"" + macroGenesisDigest + "\""; }
        if ( !worldBaselineDigest.empty() )
        { result += ",\"world_baseline_digest\":\"" + worldBaselineDigest + "\""; }
        result += "}";
        return result;
    }

    inline std::string BuildMacroWorldGenesisHelloParams(
        std::string const& requestId, char const* laneRole = "control",
        std::string const& sessionToken = {}, std::string const& serverInstanceId = {},
        std::string const& worldUuid = {}, std::string const& macroGenesisDigest = {},
        std::string const& worldBaselineDigest = {} )
    {
        return BuildClientHelloParams(
            requestId,laneRole,sessionToken,serverInstanceId,worldUuid,
            macroGenesisDigest,worldBaselineDigest,
            "[\"macro_manifest\",\"macro_page_v2\"]" );
    }

    inline std::string BuildDetailedWorldHelloParams(
        std::string const& requestId, char const* laneRole = "control",
        std::string const& sessionToken = {}, std::string const& serverInstanceId = {},
        std::string const& worldUuid = {}, std::string const& macroGenesisDigest = {},
        std::string const& worldBaselineDigest = {} )
    {
        return BuildClientHelloParams(
            requestId, laneRole, sessionToken, serverInstanceId, worldUuid,
            macroGenesisDigest, worldBaselineDigest,
            "[\"macro_manifest\",\"macro_page_v2\",\"detailed_chunk_v1\",\"predictive_residency_v1\"]" );
    }

    struct BaselineComponent
    {
        std::string role;
        std::string semanticIdentityId;
        std::string semanticIdentityVersion;
        std::string semanticDigest;
    };

    struct EngineHello
    {
        std::string requestId;
        std::string protocolId;
        std::string protocolSemver;
        std::string schemaDigest;
        std::string descriptorSchemaDigest;
        std::string engineBuild;
        std::string authorityMode;
        std::string worldUuid;
        std::string macroGenesisDigest;
        std::string worldBaselineDigest;
        std::vector<BaselineComponent> baselineComponents;
        std::string generatorFamily;
        std::string generatorVersion;
        std::string materialRegistryId;
        std::string materialRegistryDigest;
        std::string surfaceGrammarId;
        std::string surfaceGrammarVersion;
        std::string waterGrammarId;
        std::string waterGrammarVersion;
        std::string sessionToken;
        std::string laneRole;
        std::string serverInstanceId;
        std::string transportProfileId;
        std::string transportFraming;
    };

    inline bool IsHexDigest( std::string const& value )
    {
        if ( value.size() != 64 ) { return false; }
        for ( char c : value )
        {
            if ( !( c >= '0' && c <= '9' ) && !( c >= 'a' && c <= 'f' ) )
            { return false; }
        }
        return true;
    }

    inline bool IsCanonicalUuid( std::string const& value )
    {
        if ( value.size() != 36 || value[8] != '-' || value[13] != '-'
          || value[18] != '-' || value[23] != '-' )
        { return false; }
        for ( size_t i = 0; i < value.size(); ++i )
        {
            if ( i == 8 || i == 13 || i == 18 || i == 23 ) { continue; }
            char const c = value[i];
            if ( !( c >= '0' && c <= '9' ) && !( c >= 'a' && c <= 'f' ) )
            { return false; }
        }
        return true;
    }

    inline bool ParseBaselineComponents( std::string const& array,
                                         std::vector<BaselineComponent>& out )
    {
        out.clear();
        if ( array.size() < 3 || array.front() != '[' || array.back() != ']' )
        { return false; }
        size_t p = 1;
        std::unordered_set<std::string> roles;
        while ( p + 1 < array.size() )
        {
            while ( p + 1 < array.size()
                 && ( array[p] == ' ' || array[p] == '\t' || array[p] == ',' ) ) { ++p; }
            if ( p + 1 == array.size() ) { break; }
            if ( array[p] != '{' ) { return false; }
            size_t const begin = p; int depth = 0; bool quoted = false; bool escaped = false;
            for ( ; p < array.size(); ++p )
            {
                char const c = array[p];
                if ( quoted )
                {
                    if ( escaped ) { escaped = false; }
                    else if ( c == '\\' ) { escaped = true; }
                    else if ( c == '"' ) { quoted = false; }
                    continue;
                }
                if ( c == '"' ) { quoted = true; continue; }
                if ( c == '{' ) { ++depth; }
                else if ( c == '}' && --depth == 0 ) { ++p; break; }
            }
            if ( depth != 0 ) { return false; }
            std::string const object = array.substr( begin, p - begin );
            BaselineComponent component;
            if ( !ExtractString( object, "role", component.role )
              || !ExtractString( object, "semantic_identity_id", component.semanticIdentityId )
              || !ExtractString( object, "semantic_identity_version", component.semanticIdentityVersion )
              || !ExtractString( object, "semantic_digest", component.semanticDigest )
              || !IsHexDigest( component.semanticDigest ) )
            { return false; }
            if ( !roles.insert( component.role ).second ) { return false; }
            if ( !out.empty()
              && std::tie( component.role, component.semanticIdentityId,
                           component.semanticIdentityVersion, component.semanticDigest )
               < std::tie( out.back().role, out.back().semanticIdentityId,
                           out.back().semanticIdentityVersion, out.back().semanticDigest ) )
            { return false; }
            out.push_back( component );
        }
        return !out.empty();
    }

    inline bool ParseAndValidateEngineHello( std::string const& json,
                                             std::string const& expectedRequestId,
                                             EngineHello& hello,
                                             std::string& errorCode )
    {
        std::string envelopeId;
        if ( !ExtractEnvelopeId( json, envelopeId ) || envelopeId != expectedRequestId )
        { errorCode = "request_id_mismatch"; return false; }
        if ( !ExtractString( json, "request_id", hello.requestId )
          || hello.requestId != expectedRequestId )
        { errorCode = "request_id_mismatch"; return false; }

        bool const complete =
            ExtractString( json, "protocol_id", hello.protocolId )
         && ExtractString( json, "protocol_semver", hello.protocolSemver )
         && ExtractString( json, "schema_digest", hello.schemaDigest )
         && ExtractString( json, "descriptor_schema_digest", hello.descriptorSchemaDigest )
         && ExtractString( json, "engine_build", hello.engineBuild )
         && ExtractString( json, "authority_mode", hello.authorityMode )
         && ExtractString( json, "world_uuid", hello.worldUuid )
         && ExtractString( json, "macro_genesis_digest", hello.macroGenesisDigest )
         && ExtractString( json, "world_baseline_digest", hello.worldBaselineDigest )
         && ExtractString( json, "generator_family", hello.generatorFamily )
         && ExtractString( json, "generator_version", hello.generatorVersion )
         && ExtractString( json, "material_registry_id", hello.materialRegistryId )
         && ExtractString( json, "material_registry_digest", hello.materialRegistryDigest )
         && ExtractString( json, "surface_grammar_id", hello.surfaceGrammarId )
         && ExtractString( json, "surface_grammar_version", hello.surfaceGrammarVersion )
         && ExtractString( json, "water_grammar_id", hello.waterGrammarId )
         && ExtractString( json, "water_grammar_version", hello.waterGrammarVersion )
         && ExtractString( json, "session_token", hello.sessionToken )
         && ExtractString( json, "lane_role", hello.laneRole )
         && ExtractString( json, "server_instance_id", hello.serverInstanceId );
        if ( !complete ) { errorCode = "malformed_identity"; return false; }
        std::string baselineComponents;
        if ( !ExtractJsonArray( json, "baseline_components", baselineComponents )
          || !ParseBaselineComponents( baselineComponents, hello.baselineComponents ) )
        { errorCode = "malformed_identity"; return false; }
        // EI0.D fields are additive to EngineHello. EI0.A tools may
        // still parse a transport-free fixture; EI0.D validates these fields.
        ExtractString( json, "profile_id", hello.transportProfileId );
        ExtractString( json, "framing", hello.transportFraming );
        if ( hello.protocolId != kProtocolId )
        { errorCode = "protocol_id_mismatch"; return false; }
        if ( hello.protocolSemver != kProtocolSemver )
        { errorCode = "protocol_semver_mismatch"; return false; }
        if ( hello.schemaDigest != kSchemaDigest )
        { errorCode = "schema_incompatibility"; return false; }
        if ( hello.descriptorSchemaDigest != kDescriptorSchemaDigest )
        { errorCode = "schema_incompatibility"; return false; }
        if ( hello.authorityMode != kAuthorityMode )
        { errorCode = "authority_mode_mismatch"; return false; }
        if ( hello.laneRole != "control" && hello.laneRole != "bulk" )
        { errorCode = "invalid_lane_role"; return false; }
        if ( !IsCanonicalUuid( hello.worldUuid )
          || !IsCanonicalUuid( hello.serverInstanceId )
          || hello.sessionToken.empty()
          || !IsHexDigest( hello.macroGenesisDigest )
          || !IsHexDigest( hello.worldBaselineDigest )
          || !IsHexDigest( hello.materialRegistryDigest ) )
        { errorCode = "malformed_identity"; return false; }
        errorCode.clear();
        return true;
    }
}
