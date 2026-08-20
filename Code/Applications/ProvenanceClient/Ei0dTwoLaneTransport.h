#pragma once

// EI0.D transport primitives only.  No world, terrain, material, water,
// residency, rendering, or mutation law lives in this header.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "Ei0aHandshake.h"

namespace Ei0d
{
    constexpr char const* kTransportProfileId =
        "fablescript.localhost-two-lane-ndjson/1";
    constexpr char const* kFraming = "utf8-json-lines-lf/1";
    constexpr size_t kMaxRequestFrameBytes = 64u * 1024u;
    constexpr size_t kMaxControlResponseBytes = 1024u * 1024u;
    constexpr size_t kMaxBulkResponseBytes = 4u * 1024u * 1024u;
    constexpr size_t kMaxControlQueuedBytes = 2u * 1024u * 1024u;
    constexpr size_t kMaxBulkQueuedBytes = 8u * 1024u * 1024u;
    constexpr size_t kMaxBulkInflight = 8u;
    constexpr int kMaxBulkCells = 4096;
    constexpr int kMaxBulkExtent = 256;

    enum class ConnectionState
    {
        Disconnected,
        Connecting,
        WaitingForControlSession,
        HelloSent,
        Authenticated,
        Active,
        Draining,
        Closed,
        Failed,
    };

    inline char const* StateName( ConnectionState state )
    {
        switch ( state )
        {
            case ConnectionState::Disconnected: return "DISCONNECTED";
            case ConnectionState::Connecting: return "CONNECTING";
            case ConnectionState::WaitingForControlSession: return "WAITING_FOR_CONTROL_SESSION";
            case ConnectionState::HelloSent: return "HELLO_SENT";
            case ConnectionState::Authenticated: return "AUTHENTICATED";
            case ConnectionState::Active: return "ACTIVE";
            case ConnectionState::Draining: return "DRAINING";
            case ConnectionState::Closed: return "CLOSED";
            case ConnectionState::Failed: return "FAILED";
        }
        return "FAILED";
    }

    struct SessionBinding
    {
        std::string protocolId;
        std::string protocolSemver;
        std::string schemaDigest;
        std::string serverInstanceId;
        std::string sessionToken;
        std::string worldUuid;
        std::string macroGenesisDigest;
        std::string worldBaselineDigest;

        bool Complete() const
        {
            return !protocolId.empty() && !protocolSemver.empty()
                && !schemaDigest.empty() && !serverInstanceId.empty()
                && !sessionToken.empty() && !worldUuid.empty()
                && !macroGenesisDigest.empty() && !worldBaselineDigest.empty();
        }
    };

    inline SessionBinding BindingFromHello( Ei0a::EngineHello const& hello )
    {
        return { hello.protocolId, hello.protocolSemver, hello.schemaDigest,
            hello.serverInstanceId, hello.sessionToken, hello.worldUuid,
            hello.macroGenesisDigest, hello.worldBaselineDigest };
    }

    inline bool ValidateBulkBinding( SessionBinding const& control,
                                     Ei0a::EngineHello const& bulk,
                                     std::string& errorCode )
    {
        if ( !control.Complete() || bulk.laneRole != "bulk" )
        { errorCode = "malformed_session_binding"; return false; }
        if ( bulk.transportProfileId != kTransportProfileId
          || bulk.transportFraming != kFraming )
        { errorCode = "transport_profile_mismatch"; return false; }
        if ( bulk.protocolId != control.protocolId
          || bulk.protocolSemver != control.protocolSemver
          || bulk.schemaDigest != control.schemaDigest
          || bulk.serverInstanceId != control.serverInstanceId
          || bulk.sessionToken != control.sessionToken
          || bulk.worldUuid != control.worldUuid
          || bulk.macroGenesisDigest != control.macroGenesisDigest
          || bulk.worldBaselineDigest != control.worldBaselineDigest )
        { errorCode = "session_mismatch"; return false; }
        errorCode.clear();
        return true;
    }

    class JsonLineAccumulator
    {
    public:
        explicit JsonLineAccumulator( size_t maxFrame = kMaxRequestFrameBytes )
            : maxFrame_( maxFrame ) {}

        bool Feed( char const* data, size_t size, std::vector<std::string>& frames,
                   std::string& errorCode )
        {
            buffer_.append( data, size );
            for ( ;; )
            {
                size_t const newline = buffer_.find( '\n' );
                if ( newline == std::string::npos )
                {
                    if ( buffer_.size() > maxFrame_ )
                    { buffer_.clear(); errorCode = "frame_too_large"; return false; }
                    break;
                }
                std::string frame = buffer_.substr( 0, newline );
                buffer_.erase( 0, newline + 1 );
                if ( !frame.empty() && frame.back() == '\r' ) { frame.pop_back(); }
                if ( frame.size() > maxFrame_ )
                { errorCode = "frame_too_large"; return false; }
                if ( !frame.empty() ) { frames.push_back( std::move( frame ) ); }
            }
            errorCode.clear();
            return true;
        }

        bool Finish( std::string& errorCode )
        {
            bool onlyWhitespace = true;
            for ( char c : buffer_ )
            { if ( c != ' ' && c != '\t' && c != '\r' ) { onlyWhitespace = false; break; } }
            buffer_.clear();
            if ( !onlyWhitespace ) { errorCode = "truncated_frame"; return false; }
            errorCode.clear();
            return true;
        }

        size_t BufferedBytes() const { return buffer_.size(); }
        void Clear() { buffer_.clear(); }

    private:
        size_t maxFrame_;
        std::string buffer_;
    };

    class LaneFlow
    {
    public:
        LaneFlow( size_t maxInflight, size_t maxQueuedBytes )
            : maxInflight_( maxInflight ), maxQueuedBytes_( maxQueuedBytes ) {}

        bool ReserveRequest( size_t frameBytes, std::string& errorCode )
        {
            if ( inflight_ >= maxInflight_ )
            { errorCode = "too_many_inflight_requests"; return false; }
            if ( frameBytes > maxQueuedBytes_ || queuedBytes_ + frameBytes > maxQueuedBytes_ )
            { errorCode = "outbound_backpressure"; return false; }
            ++inflight_;
            queuedBytes_ += frameBytes;
            highWaterBytes_ = (std::max)( highWaterBytes_, queuedBytes_ );
            errorCode.clear();
            return true;
        }

        void Sent( size_t frameBytes )
        { queuedBytes_ = frameBytes > queuedBytes_ ? 0 : queuedBytes_ - frameBytes; }
        void Complete() { if ( inflight_ ) { --inflight_; } }
        size_t Inflight() const { return inflight_; }
        size_t QueuedBytes() const { return queuedBytes_; }
        size_t HighWaterBytes() const { return highWaterBytes_; }
        void Clear() { inflight_ = queuedBytes_ = highWaterBytes_ = 0; }

    private:
        size_t maxInflight_;
        size_t maxQueuedBytes_;
        size_t inflight_ = 0;
        size_t queuedBytes_ = 0;
        size_t highWaterBytes_ = 0;
    };

    template<class T>
    class AtomicPublicationSlot
    {
    public:
        void Publish( std::shared_ptr<T const> candidate )
        {
            std::lock_guard<std::mutex> lock( mutex_ );
            live_ = std::move( candidate );
            ++generation_;
        }

        std::shared_ptr<T const> Load() const
        {
            std::lock_guard<std::mutex> lock( mutex_ );
            return live_;
        }

        uint64_t Generation() const
        {
            std::lock_guard<std::mutex> lock( mutex_ );
            return generation_;
        }

    private:
        mutable std::mutex mutex_;
        std::shared_ptr<T const> live_;
        uint64_t generation_ = 0;
    };
}
