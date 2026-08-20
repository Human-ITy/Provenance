#pragma once

// EI0.A is contract identity only. This header has no terrain, rendering,
// mutation, residency, or transport dependencies.

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "WorldDescriptors.generated.h"

namespace Ei0a
{
    constexpr char const* kProtocolId = "fablescript.embodied-terrain";
    constexpr char const* kProtocolSemver = "1.0.0";
    constexpr char const* kSchemaDigest = Ms1::kWorldDescriptorSchemaDigest;
    constexpr char const* kAuthorityMode = "local_server_authoritative";
    constexpr char const* kClientBuild =
        "5073baa73905e731eb72c17ae026685b25a9e812+ei0d";

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
                                               std::string const& genesisDigest = {},
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
        if ( !genesisDigest.empty() )
        { result += ",\"genesis_digest\":\"" + genesisDigest + "\""; }
        result += "}";
        return result;
    }

    inline std::string BuildMacroWorldGenesisHelloParams(
        std::string const& requestId, char const* laneRole = "control",
        std::string const& sessionToken = {}, std::string const& serverInstanceId = {},
        std::string const& worldUuid = {}, std::string const& genesisDigest = {} )
    {
        return BuildClientHelloParams(
            requestId,laneRole,sessionToken,serverInstanceId,worldUuid,genesisDigest,
            "[\"macro_manifest\",\"macro_page_v2\"]" );
    }

    struct EngineHello
    {
        std::string requestId;
        std::string protocolId;
        std::string protocolSemver;
        std::string schemaDigest;
        std::string engineBuild;
        std::string authorityMode;
        std::string worldUuid;
        std::string genesisDigest;
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
         && ExtractString( json, "engine_build", hello.engineBuild )
         && ExtractString( json, "authority_mode", hello.authorityMode )
         && ExtractString( json, "world_uuid", hello.worldUuid )
         && ExtractString( json, "genesis_digest", hello.genesisDigest )
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
        // EI0.D fields are additive to the 1.0 EngineHello.  EI0.A tools may
        // still parse a transport-free fixture; EI0.D validates these fields.
        ExtractString( json, "profile_id", hello.transportProfileId );
        ExtractString( json, "framing", hello.transportFraming );
        if ( hello.protocolId != kProtocolId )
        { errorCode = "protocol_id_mismatch"; return false; }
        if ( hello.protocolSemver != kProtocolSemver )
        { errorCode = "protocol_semver_mismatch"; return false; }
        if ( hello.schemaDigest != kSchemaDigest )
        { errorCode = "schema_incompatibility"; return false; }
        if ( hello.authorityMode != kAuthorityMode )
        { errorCode = "authority_mode_mismatch"; return false; }
        if ( hello.laneRole != "control" && hello.laneRole != "bulk" )
        { errorCode = "invalid_lane_role"; return false; }
        if ( !IsCanonicalUuid( hello.worldUuid )
          || !IsCanonicalUuid( hello.serverInstanceId )
          || hello.sessionToken.empty()
          || !IsHexDigest( hello.genesisDigest )
          || !IsHexDigest( hello.materialRegistryDigest ) )
        { errorCode = "malformed_identity"; return false; }
        errorCode.clear();
        return true;
    }
}
