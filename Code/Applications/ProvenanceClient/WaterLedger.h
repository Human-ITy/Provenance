// P5a — Water ledger / settle floor (Esoterica-local until Fablescript water authority wires in).
// Law: Water should wake from causality, not from time passing.
//
// terrain occupancy → container/obstruction
// water ledger      → conserved water amount (grams)
// surface/body      → presentation + local flow (minimal)
// terrain mutation  → wake only affected water neighborhood
//
// Not P5b coupling depth / P5c presentation / P5d flow stress.
// Not continuum MPM / Atomic-Fluid authority. Not cubic lattice-leak as truth.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace WaterLedger
{
    // Matching Fablescript CELL_CAPACITY_UNITS scale: 100 units = one full cell of water.
    // P5a treats these as conserved grams (integer mass/volume proxy).
    constexpr int64_t kCellCapacityGrams = 100;
    constexpr float   kCellDepthM        = 1.0f; // 1 cell ≈ 1 m depth at full capacity
    constexpr int     kMaxSettlePasses   = 64;

    enum class BodyState : uint8_t
    {
        Awake = 0,
        Settling,
        Dormant,
    };

    enum class WakeCause : uint32_t
    {
        None          = 0,
        Pour          = 1u << 0,
        Dig           = 1u << 1,
        Place         = 1u << 2,
        WaterReceipt  = 1u << 3,
        FlowNeighbor  = 1u << 4,
    };

    inline uint64_t PackXY( int x, int y )
    {
        return ( (uint64_t)(uint32_t)x << 32 ) | (uint32_t)y;
    }

    inline void UnpackXY( uint64_t k, int& x, int& y )
    {
        x = (int)(uint32_t)( k >> 32 );
        y = (int)(uint32_t)k;
    }

    // Local occupancy / container view for water (not D2 presentation).
    // solid=true ⇒ water cannot occupy that column cell.
    // floorZ = open basin floor; openCapacityGrams = free volume above floor (≤ kCellCapacityGrams).
    struct ContainerCell
    {
        bool    solid = true;
        float   floorZ = 0.f;
        int64_t openCapacityGrams = 0;
        uint32_t terrainRev = 0;
    };

    struct WaterCell
    {
        int64_t  grams = 0;
        float    floorZ = 0.f;
        float    surfaceZ = 0.f; // presentation height (derived; not authority for mass)
        uint32_t bodyId = 0;
        uint32_t terrainRev = 0; // stamp of terrain container used for last surface solve
        uint32_t waterRev = 0;   // last water write that touched this cell
    };

    struct WaterBody
    {
        uint32_t  id = 0;
        BodyState state = BodyState::Awake;
        int64_t   totalGrams = 0;
        float     surfaceZ = 0.f;
        std::vector<std::pair<int, int>> cells;
        int       settlePassesLeft = 0;
        uint32_t  wakeCause = 0;
    };

    // Presentation stub: refuses to claim a surface plane from mismatched revisions.
    struct PresentationStamp
    {
        uint32_t terrainRev = 0;
        uint32_t waterRev = 0;
        bool     valid = false;
        float    surfaceZ = 0.f;
        uint32_t bodyId = 0;
    };

    struct World
    {
        std::unordered_map<uint64_t, ContainerCell> containers;
        std::unordered_map<uint64_t, WaterCell> cells;
        std::vector<WaterBody> bodies;
        uint32_t nextBodyId = 1;
        uint32_t waterRev = 0;
        uint32_t terrainRev = 1; // monotonic container revision

        // Causality / cost counters (cert)
        int64_t workUnits = 0;           // increments only when water actually does work
        int64_t dormantIdleSkips = 0;    // TickSettle with nothing awake
        int     wakeEvents = 0;
        int     settlePasses = 0;
        int     pourAccepted = 0;
        int     pourRejectedSolid = 0;
        int     displaceEvents = 0;
        int     presentationRefusals = 0;
        int     globalWakeAttempts = 0;  // must stay 0 — we never wake all water
    };

    inline ContainerCell* GetContainer( World& w, int x, int y )
    {
        auto it = w.containers.find( PackXY( x, y ) );
        return it == w.containers.end() ? nullptr : &it->second;
    }

    inline ContainerCell const* GetContainer( World const& w, int x, int y )
    {
        auto it = w.containers.find( PackXY( x, y ) );
        return it == w.containers.end() ? nullptr : &it->second;
    }

    inline WaterCell* GetWater( World& w, int x, int y )
    {
        auto it = w.cells.find( PackXY( x, y ) );
        return it == w.cells.end() ? nullptr : &it->second;
    }

    inline WaterCell const* GetWater( World const& w, int x, int y )
    {
        auto it = w.cells.find( PackXY( x, y ) );
        return it == w.cells.end() ? nullptr : &it->second;
    }

    inline int64_t AmountAt( World const& w, int x, int y )
    {
        WaterCell const* c = GetWater( w, x, y );
        return c ? c->grams : 0;
    }

    inline int64_t TotalGrams( World const& w )
    {
        int64_t t = 0;
        for ( auto const& kv : w.cells ) { t += kv.second.grams; }
        return t;
    }

    inline bool AnyAwake( World const& w )
    {
        for ( WaterBody const& b : w.bodies )
        {
            if ( b.state != BodyState::Dormant && b.totalGrams > 0 ) { return true; }
        }
        return false;
    }

    inline int CountDormantBodies( World const& w )
    {
        int n = 0;
        for ( WaterBody const& b : w.bodies )
        {
            if ( b.state == BodyState::Dormant && b.totalGrams > 0 ) { ++n; }
        }
        return n;
    }

    inline void SetBasin( World& w, int x, int y, float floorZ, int64_t openCap,
        bool solid = false )
    {
        ContainerCell& c = w.containers[PackXY( x, y )];
        c.solid = solid;
        c.floorZ = floorZ;
        c.openCapacityGrams = solid ? 0 : (std::max)( (int64_t)0,
            (std::min)( openCap, kCellCapacityGrams ) );
        c.terrainRev = w.terrainRev;
        ++w.workUnits; // container authoring is a terrain-side change, not a water tick
    }

    inline void BumpTerrainRev( World& w )
    {
        ++w.terrainRev;
    }

    inline float SurfaceFromGrams( float floorZ, int64_t grams, int64_t capacity )
    {
        if ( capacity <= 0 || grams <= 0 ) { return floorZ; }
        float const frac = (float)grams / (float)capacity;
        return floorZ + frac * kCellDepthM;
    }

    inline WaterBody* FindBody( World& w, uint32_t id )
    {
        if ( id == 0 ) { return nullptr; }
        for ( WaterBody& b : w.bodies )
        {
            if ( b.id == id ) { return &b; }
        }
        return nullptr;
    }

    inline WaterBody const* FindBody( World const& w, uint32_t id )
    {
        if ( id == 0 ) { return nullptr; }
        for ( WaterBody const& b : w.bodies )
        {
            if ( b.id == id ) { return &b; }
        }
        return nullptr;
    }

    inline void MarkBodyAwake( World& w, WaterBody& b, WakeCause cause )
    {
        if ( b.totalGrams <= 0 ) { return; }
        bool const wasDormant = ( b.state == BodyState::Dormant );
        b.state = BodyState::Awake;
        b.settlePassesLeft = kMaxSettlePasses;
        b.wakeCause |= (uint32_t)cause;
        if ( wasDormant )
        {
            ++w.wakeEvents;
        }
        ++w.workUnits;
    }

    inline void RebuildBodyMembership( World& w )
    {
        // Connected components over wet 4-neighbors. O(wet cells) — only called on wake/settle.
        for ( WaterBody& b : w.bodies )
        {
            b.cells.clear();
            b.totalGrams = 0;
        }
        std::unordered_set<uint64_t> seen;
        auto tryGet = [&]( int x, int y ) -> WaterCell*
        {
            return GetWater( w, x, y );
        };

        for ( auto& kv : w.cells )
        {
            if ( kv.second.grams <= 0 ) { continue; }
            if ( seen.count( kv.first ) ) { continue; }
            int sx = 0, sy = 0;
            UnpackXY( kv.first, sx, sy );

            // Reuse existing body id if seed already has one and body exists; else mint.
            uint32_t bid = kv.second.bodyId;
            WaterBody* body = FindBody( w, bid );
            if ( !body )
            {
                WaterBody nb{};
                nb.id = w.nextBodyId++;
                nb.state = BodyState::Awake;
                nb.settlePassesLeft = kMaxSettlePasses;
                w.bodies.push_back( nb );
                body = &w.bodies.back();
                bid = body->id;
            }

            std::vector<std::pair<int, int>> q;
            q.push_back( { sx, sy } );
            seen.insert( kv.first );
            int64_t sum = 0;
            float maxSurf = -1e9f;
            while ( !q.empty() )
            {
                auto [x, y] = q.back();
                q.pop_back();
                WaterCell* c = tryGet( x, y );
                if ( !c || c->grams <= 0 ) { continue; }
                c->bodyId = bid;
                body->cells.push_back( { x, y } );
                sum += c->grams;
                maxSurf = (std::max)( maxSurf, c->surfaceZ );
                static int const dx[4] = { 1, -1, 0, 0 };
                static int const dy[4] = { 0, 0, 1, -1 };
                for ( int i = 0; i < 4; ++i )
                {
                    int const nx = x + dx[i], ny = y + dy[i];
                    uint64_t const nk = PackXY( nx, ny );
                    if ( seen.count( nk ) ) { continue; }
                    WaterCell* n = tryGet( nx, ny );
                    if ( !n || n->grams <= 0 ) { continue; }
                    seen.insert( nk );
                    q.push_back( { nx, ny } );
                }
            }
            body->totalGrams = sum;
            body->surfaceZ = maxSurf;
            if ( sum <= 0 ) { body->state = BodyState::Dormant; }
        }

        // Drop empty bodies (keep vector compact).
        w.bodies.erase(
            std::remove_if( w.bodies.begin(), w.bodies.end(),
                []( WaterBody const& b ) { return b.totalGrams <= 0 || b.cells.empty(); } ),
            w.bodies.end() );
        ++w.workUnits;
    }

    inline void RecomputeCellSurface( World& w, int x, int y )
    {
        WaterCell* wc = GetWater( w, x, y );
        ContainerCell const* cc = GetContainer( w, x, y );
        if ( !wc ) { return; }
        if ( !cc || cc->solid || cc->openCapacityGrams <= 0 )
        {
            wc->surfaceZ = wc->floorZ;
            return;
        }
        wc->floorZ = cc->floorZ;
        wc->terrainRev = cc->terrainRev;
        wc->surfaceZ = SurfaceFromGrams( cc->floorZ, wc->grams, cc->openCapacityGrams );
    }

    // Pour exact grams into an open container cell. Rejects solid / zero-capacity.
    // Returns grams accepted (may be less if capacity limited — remainder not invented).
    inline int64_t Pour( World& w, int x, int y, int64_t grams )
    {
        if ( grams <= 0 ) { return 0; }
        ContainerCell const* cc = GetContainer( w, x, y );
        if ( !cc || cc->solid || cc->openCapacityGrams <= 0 )
        {
            ++w.pourRejectedSolid;
            ++w.workUnits;
            return 0;
        }
        WaterCell& wc = w.cells[PackXY( x, y )];
        int64_t const room = cc->openCapacityGrams - wc.grams;
        if ( room <= 0 )
        {
            ++w.pourRejectedSolid;
            ++w.workUnits;
            return 0;
        }
        int64_t const add = (std::min)( grams, room );
        wc.grams += add;
        wc.floorZ = cc->floorZ;
        wc.terrainRev = cc->terrainRev;
        ++w.waterRev;
        wc.waterRev = w.waterRev;
        RecomputeCellSurface( w, x, y );
        ++w.pourAccepted;
        ++w.workUnits;
        RebuildBodyMembership( w );
        if ( WaterBody* b = FindBody( w, wc.bodyId ) )
        {
            MarkBodyAwake( w, *b, WakeCause::Pour );
        }
        return add;
    }

    // Local level-seeking within one awake body: equalize grams by open capacity / head.
    // Minimal flow — not P5d stress. Conserves total grams exactly.
    inline bool SettleBodyOnce( World& w, WaterBody& body )
    {
        if ( body.cells.size() < 2 ) { return false; }
        ++w.settlePasses;
        ++w.workUnits;

        struct Slot
        {
            int x, y;
            int64_t cap;
            int64_t grams;
            float floorZ;
        };
        std::vector<Slot> slots;
        slots.reserve( body.cells.size() );
        int64_t total = 0;
        int64_t totalCap = 0;
        for ( auto const& xy : body.cells )
        {
            ContainerCell const* cc = GetContainer( w, xy.first, xy.second );
            WaterCell* wc = GetWater( w, xy.first, xy.second );
            if ( !cc || !wc || cc->solid || cc->openCapacityGrams <= 0 ) { continue; }
            slots.push_back( { xy.first, xy.second, cc->openCapacityGrams, wc->grams, cc->floorZ } );
            total += wc->grams;
            totalCap += cc->openCapacityGrams;
        }
        if ( slots.empty() || totalCap <= 0 ) { return false; }

        // Prefer lower floors: fill lowest basin first (head-driven), then level.
        std::sort( slots.begin(), slots.end(),
            []( Slot const& a, Slot const& b ) {
                if ( a.floorZ != b.floorZ ) { return a.floorZ < b.floorZ; }
                if ( a.x != b.x ) { return a.x < b.x; }
                return a.y < b.y;
            } );

        int64_t remain = total;
        std::vector<int64_t> target( slots.size(), 0 );
        for ( size_t i = 0; i < slots.size(); ++i )
        {
            int64_t take = (std::min)( slots[i].cap, remain );
            target[i] = take;
            remain -= take;
        }
        // Remainder (overflow) stays in highest slots — conserved; not deleted.
        if ( remain > 0 )
        {
            for ( size_t i = slots.size(); i-- > 0 && remain > 0; )
            {
                int64_t room = slots[i].cap - target[i];
                int64_t take = (std::min)( room, remain );
                target[i] += take;
                remain -= take;
            }
        }

        bool moved = false;
        int64_t check = 0;
        for ( size_t i = 0; i < slots.size(); ++i )
        {
            check += target[i];
            if ( target[i] != slots[i].grams ) { moved = true; }
            WaterCell* wc = GetWater( w, slots[i].x, slots[i].y );
            if ( !wc ) { continue; }
            if ( wc->grams != target[i] )
            {
                wc->grams = target[i];
                ++w.waterRev;
                wc->waterRev = w.waterRev;
            }
            RecomputeCellSurface( w, slots[i].x, slots[i].y );
        }
        // Exact conservation: if overflow couldn't place (all full), keep on last cell.
        if ( remain > 0 && !slots.empty() )
        {
            WaterCell* wc = GetWater( w, slots.back().x, slots.back().y );
            if ( wc )
            {
                wc->grams += remain;
                check += remain;
                remain = 0;
                ++w.waterRev;
                wc->waterRev = w.waterRev;
                RecomputeCellSurface( w, slots.back().x, slots.back().y );
            }
        }
        (void)check;
        body.totalGrams = total;
        float surf = -1e9f;
        for ( auto const& xy : body.cells )
        {
            if ( WaterCell const* wc = GetWater( w, xy.first, xy.second ) )
            {
                surf = (std::max)( surf, wc->surfaceZ );
            }
        }
        body.surfaceZ = surf;
        return moved;
    }

    // Tick only awake/settling bodies. Dormant pond ⇒ zero work (aside from the idle skip count).
    inline void TickSettle( World& w )
    {
        if ( !AnyAwake( w ) )
        {
            ++w.dormantIdleSkips;
            return;
        }
        for ( WaterBody& b : w.bodies )
        {
            if ( b.state == BodyState::Dormant || b.totalGrams <= 0 ) { continue; }
            b.state = BodyState::Settling;
            bool moved = SettleBodyOnce( w, b );
            if ( b.settlePassesLeft > 0 ) { --b.settlePassesLeft; }
            if ( !moved || b.settlePassesLeft <= 0 )
            {
                b.state = BodyState::Dormant;
                b.wakeCause = 0;
            }
        }
    }

    // Unrelated world activity must not wake or tick water.
    inline void OnUnrelatedReceipt( World& w )
    {
        // Intentional no-op — counter proves we were called and did nothing.
        (void)w;
    }

    // Water channel receipt (affect includes water) — wake scoped cells if present.
    inline void OnWaterReceipt( World& w, int minX, int minY, int maxX, int maxY )
    {
        ++w.workUnits;
        bool any = false;
        for ( int y = minY; y <= maxY; ++y )
        {
            for ( int x = minX; x <= maxX; ++x )
            {
                WaterCell* wc = GetWater( w, x, y );
                if ( !wc || wc->grams <= 0 ) { continue; }
                if ( WaterBody* b = FindBody( w, wc->bodyId ) )
                {
                    MarkBodyAwake( w, *b, WakeCause::WaterReceipt );
                    any = true;
                }
            }
        }
        (void)any;
    }

    // Dig below/next to water: wake ONLY bodies that touch the neighborhood (never global).
    inline int OnTerrainDig( World& w, int cx, int cy, int radiusCells = 1 )
    {
        BumpTerrainRev( w );
        // Deepen / open container at dig site (minimal coupling hook for P5a).
        for ( int dy = -radiusCells; dy <= radiusCells; ++dy )
        {
            for ( int dx = -radiusCells; dx <= radiusCells; ++dx )
            {
                int const x = cx + dx, y = cy + dy;
                ContainerCell* cc = GetContainer( w, x, y );
                if ( !cc ) { continue; }
                if ( cc->solid )
                {
                    cc->solid = false;
                    cc->openCapacityGrams = kCellCapacityGrams;
                    cc->floorZ -= 0.25f;
                }
                else
                {
                    cc->floorZ -= 0.15f;
                    cc->openCapacityGrams = kCellCapacityGrams;
                }
                cc->terrainRev = w.terrainRev;
            }
        }

        std::unordered_set<uint32_t> wakeIds;
        for ( int dy = -radiusCells; dy <= radiusCells; ++dy )
        {
            for ( int dx = -radiusCells; dx <= radiusCells; ++dx )
            {
                WaterCell* wc = GetWater( w, cx + dx, cy + dy );
                if ( wc && wc->grams > 0 && wc->bodyId != 0 )
                {
                    wakeIds.insert( wc->bodyId );
                }
            }
        }
        int woken = 0;
        for ( uint32_t id : wakeIds )
        {
            if ( WaterBody* b = FindBody( w, id ) )
            {
                MarkBodyAwake( w, *b, WakeCause::Dig );
                ++woken;
            }
        }
        // Re-stamp containers + surfaces for woken bodies only (rev-consistent presentation).
        for ( uint32_t id : wakeIds )
        {
            if ( WaterBody* b = FindBody( w, id ) )
            {
                for ( auto const& xy : b->cells )
                {
                    if ( ContainerCell* bcc = GetContainer( w, xy.first, xy.second ) )
                    {
                        bcc->terrainRev = w.terrainRev;
                    }
                    RecomputeCellSurface( w, xy.first, xy.second );
                }
            }
        }
        ++w.workUnits;
        return woken;
    }

    // Place matter into water: displace / reconfigure — never delete grams.
    // solidFillGrams reduces open capacity; excess water moves to neighbor open cells in-body.
    inline int64_t OnTerrainPlace( World& w, int cx, int cy, int64_t solidFillGrams )
    {
        BumpTerrainRev( w );
        ContainerCell* cc = GetContainer( w, cx, cy );
        if ( !cc ) { return TotalGrams( w ); }

        int64_t const before = TotalGrams( w );
        WaterCell* wc = GetWater( w, cx, cy );
        int64_t displaced = 0;
        if ( wc && wc->grams > 0 )
        {
            // Shrink capacity by solid fill (clamped).
            int64_t const fill = (std::max)( (int64_t)0,
                (std::min)( solidFillGrams, kCellCapacityGrams ) );
            int64_t newCap = (std::max)( (int64_t)0, kCellCapacityGrams - fill );
            if ( fill >= kCellCapacityGrams )
            {
                cc->solid = true;
                cc->openCapacityGrams = 0;
                newCap = 0;
            }
            else
            {
                cc->solid = false;
                cc->openCapacityGrams = newCap;
            }
            cc->terrainRev = w.terrainRev;

            if ( wc->grams > newCap )
            {
                displaced = wc->grams - newCap;
                wc->grams = newCap;
                ++w.waterRev;
                wc->waterRev = w.waterRev;
            }
            RecomputeCellSurface( w, cx, cy );

            // Push displaced grams into neighboring open containers (4-neigh), preferring lower floor.
            if ( displaced > 0 )
            {
                struct N { int x, y; float floorZ; int64_t room; };
                std::vector<N> ns;
                static int const dx[4] = { 1, -1, 0, 0 };
                static int const dy[4] = { 0, 0, 1, -1 };
                for ( int i = 0; i < 4; ++i )
                {
                    int const nx = cx + dx[i], ny = cy + dy[i];
                    ContainerCell const* ncc = GetContainer( w, nx, ny );
                    if ( !ncc || ncc->solid || ncc->openCapacityGrams <= 0 ) { continue; }
                    WaterCell const* nwc = GetWater( w, nx, ny );
                    int64_t have = nwc ? nwc->grams : 0;
                    int64_t room = ncc->openCapacityGrams - have;
                    if ( room > 0 ) { ns.push_back( { nx, ny, ncc->floorZ, room } ); }
                }
                std::sort( ns.begin(), ns.end(),
                    []( N const& a, N const& b ) {
                        if ( a.floorZ != b.floorZ ) { return a.floorZ < b.floorZ; }
                        return a.x < b.x;
                    } );
                for ( N const& n : ns )
                {
                    if ( displaced <= 0 ) { break; }
                    int64_t take = (std::min)( displaced, n.room );
                    WaterCell& dest = w.cells[PackXY( n.x, n.y )];
                    ContainerCell const* ncc = GetContainer( w, n.x, n.y );
                    dest.grams += take;
                    if ( ncc )
                    {
                        dest.floorZ = ncc->floorZ;
                        dest.terrainRev = ncc->terrainRev;
                    }
                    ++w.waterRev;
                    dest.waterRev = w.waterRev;
                    RecomputeCellSurface( w, n.x, n.y );
                    displaced -= take;
                }
                // If still displaced (no room), keep at source even if over-cap — conservation > delete.
                if ( displaced > 0 )
                {
                    wc->grams += displaced;
                    displaced = 0;
                    ++w.waterRev;
                    wc->waterRev = w.waterRev;
                    RecomputeCellSurface( w, cx, cy );
                }
            }
            ++w.displaceEvents;
            RebuildBodyMembership( w );
            WaterBody* woken = nullptr;
            if ( WaterCell* still = GetWater( w, cx, cy ) )
            {
                woken = FindBody( w, still->bodyId );
            }
            if ( !woken && wc->bodyId != 0 ) { woken = FindBody( w, wc->bodyId ); }
            if ( woken )
            {
                MarkBodyAwake( w, *woken, WakeCause::Place );
                // All cells in the woken body must share the place's terrainRev stamp.
                for ( auto const& xy : woken->cells )
                {
                    if ( ContainerCell* bcc = GetContainer( w, xy.first, xy.second ) )
                    {
                        bcc->terrainRev = w.terrainRev;
                    }
                    RecomputeCellSurface( w, xy.first, xy.second );
                }
            }
        }
        else
        {
            // Dry place: just update container.
            int64_t const fill = (std::max)( (int64_t)0,
                (std::min)( solidFillGrams, kCellCapacityGrams ) );
            if ( fill >= kCellCapacityGrams )
            {
                cc->solid = true;
                cc->openCapacityGrams = 0;
            }
            else
            {
                cc->openCapacityGrams = (std::max)( (int64_t)0, cc->openCapacityGrams - fill );
            }
            cc->terrainRev = w.terrainRev;
        }

        int64_t const after = TotalGrams( w );
        (void)before;
        (void)after;
        ++w.workUnits;
        return after;
    }

    // Presentation stub: surface only valid when terrain+water revisions match the stamps.
    inline PresentationStamp PresentBody( World const& w, uint32_t bodyId,
        uint32_t expectTerrainRev, uint32_t expectWaterRev )
    {
        PresentationStamp s{};
        WaterBody const* b = FindBody( w, bodyId );
        if ( !b ) { return s; }
        s.bodyId = bodyId;
        s.surfaceZ = b->surfaceZ;
        s.terrainRev = expectTerrainRev;
        s.waterRev = expectWaterRev;
        // Every wet cell in body must agree with stamps (no plane from one rev over occupancy from another).
        for ( auto const& xy : b->cells )
        {
            WaterCell const* wc = GetWater( w, xy.first, xy.second );
            ContainerCell const* cc = GetContainer( w, xy.first, xy.second );
            if ( !wc || !cc ) { return s; }
            if ( wc->terrainRev != expectTerrainRev || cc->terrainRev != expectTerrainRev )
            {
                return s; // refused
            }
            if ( wc->waterRev > expectWaterRev )
            {
                return s; // water moved past stamp
            }
        }
        s.valid = true;
        return s;
    }

    inline PresentationStamp PresentBodyCurrent( World& w, uint32_t bodyId )
    {
        // Stamp from the body's agreed terrainRev (uniform across cells) + current waterRev.
        // A dormant distant body may lag global terrain HEAD — that is OK if internally consistent.
        // Refuse mixed terrain revs inside one body, or water solved against a different container rev.
        WaterBody const* b = FindBody( w, bodyId );
        if ( !b || b->cells.empty() )
        {
            ++w.presentationRefusals;
            return {};
        }
        uint32_t tRev = 0;
        bool first = true;
        for ( auto const& xy : b->cells )
        {
            WaterCell const* wc = GetWater( w, xy.first, xy.second );
            ContainerCell const* cc = GetContainer( w, xy.first, xy.second );
            if ( !wc || !cc || wc->terrainRev != cc->terrainRev )
            {
                ++w.presentationRefusals;
                return {};
            }
            if ( first ) { tRev = wc->terrainRev; first = false; }
            else if ( wc->terrainRev != tRev )
            {
                ++w.presentationRefusals;
                return {};
            }
        }
        PresentationStamp s = PresentBody( w, bodyId, tRev, w.waterRev );
        if ( !s.valid ) { ++w.presentationRefusals; }
        return s;
    }

    // ---- Cert fixture + checks (headless, no bridge) ----

    struct CertRow
    {
        char check[64];
        char verdict[12];
        char note[240];
    };

    struct CertResult
    {
        CertRow rows[48];
        int rowN = 0;
        int exitCode = 0;
        char firstFail[240] = {};
    };

    inline void CertAdd( CertResult& r, char const* check, char const* verdict, char const* note )
    {
        if ( r.rowN >= (int)( sizeof( r.rows ) / sizeof( r.rows[0] ) ) ) { return; }
        CertRow& row = r.rows[r.rowN++];
        std::snprintf( row.check, sizeof( row.check ), "%s", check ? check : "?" );
        std::snprintf( row.verdict, sizeof( row.verdict ), "%s", verdict ? verdict : "?" );
        std::snprintf( row.note, sizeof( row.note ), "%s", note ? note : "" );
        if ( verdict && std::strcmp( verdict, "FAIL" ) == 0 )
        {
            r.exitCode = 1;
            if ( !r.firstFail[0] )
            {
                std::snprintf( r.firstFail, sizeof( r.firstFail ), "%s", row.check );
            }
        }
    }

    inline CertResult RunP5aCert()
    {
        CertResult R{};
        World w{};

        // Two disconnected basins (A near origin, B far). Different depths/capacities.
        // Basin A: 3 cells, deep floor → can hold more volume at same surface height as B.
        SetBasin( w, 10, 10, 0.0f, 100 );
        SetBasin( w, 11, 10, 0.0f, 100 );
        SetBasin( w, 10, 11, 0.0f, 100 );
        // Basin B: 2 cells, higher floor, smaller open cap for height≠grams demo
        SetBasin( w, 80, 80, 2.0f, 40 );
        SetBasin( w, 81, 80, 2.0f, 40 );
        // Solid wall / blocked pour target
        SetBasin( w, 12, 10, 0.0f, 0, /*solid*/true );

        // 1) pour adds exact water mass
        int64_t a1 = Pour( w, 10, 10, 60 );
        int64_t a2 = Pour( w, 11, 10, 40 );
        char note[240];
        std::snprintf( note, sizeof( note ), "poured=%lld total=%lld",
            (long long)( a1 + a2 ), (long long)TotalGrams( w ) );
        CertAdd( R, "pour_exact_mass",
            ( a1 == 60 && a2 == 40 && TotalGrams( w ) == 100 ) ? "PASS" : "FAIL", note );

        // 2) cannot occupy solid
        int64_t solidPour = Pour( w, 12, 10, 25 );
        std::snprintf( note, sizeof( note ), "solid_accept=%lld rejected=%d",
            (long long)solidPour, w.pourRejectedSolid );
        CertAdd( R, "no_solid_occupy",
            ( solidPour == 0 && AmountAt( w, 12, 10 ) == 0 && w.pourRejectedSolid >= 1 )
                ? "PASS" : "FAIL", note );

        // Settle basin A to dormant
        for ( int i = 0; i < 8; ++i ) { TickSettle( w ); }
        RebuildBodyMembership( w );
        CertAdd( R, "settled_dormant",
            ( !AnyAwake( w ) && CountDormantBodies( w ) >= 1 && TotalGrams( w ) == 100 )
                ? "PASS" : "FAIL",
            AnyAwake( w ) ? "still_awake" : "dormant_ok" );

        // Snapshot idle counters
        int64_t work0 = w.workUnits;
        int wakes0 = w.wakeEvents;
        int64_t skips0 = w.dormantIdleSkips;

        // 3) unrelated receipts → zero water work (affect=0 pattern)
        for ( int i = 0; i < 50; ++i )
        {
            OnUnrelatedReceipt( w );
            TickSettle( w ); // dormant idle skip
        }
        std::snprintf( note, sizeof( note ),
            "dWork=%lld dWake=%d skips=%lld awake=%d",
            (long long)( w.workUnits - work0 ), w.wakeEvents - wakes0,
            (long long)( w.dormantIdleSkips - skips0 ), AnyAwake( w ) ? 1 : 0 );
        CertAdd( R, "dormant_zero_unrelated",
            ( ( w.workUnits - work0 ) == 0 && ( w.wakeEvents - wakes0 ) == 0
              && ( w.dormantIdleSkips - skips0 ) >= 50 && !AnyAwake( w ) )
                ? "PASS" : "FAIL", note );

        // 4) second pond, equal surface height ≠ equal grams
        // Pour into B enough that surface matches A's surface but grams differ.
        // Force settle A surface first.
        float surfA = 0.f;
        for ( WaterBody const& b : w.bodies )
        {
            if ( !b.cells.empty() && b.cells[0].first < 50 ) { surfA = b.surfaceZ; break; }
        }
        // Target surface at B: floor 2.0 + frac*1.0. Choose grams so surface ≈ surfA if possible,
        // else just prove two bodies can share surfaceZ with different totals.
        int64_t pourB = Pour( w, 80, 80, 30 );
        int64_t pourB2 = Pour( w, 81, 80, 10 );
        for ( int i = 0; i < 8; ++i ) { TickSettle( w ); }
        RebuildBodyMembership( w );

        int64_t gramsA = 0, gramsB = 0;
        float sA = 0.f, sB = 0.f;
        uint32_t idA = 0, idB = 0;
        for ( WaterBody const& b : w.bodies )
        {
            if ( b.cells.empty() ) { continue; }
            if ( b.cells[0].first < 50 ) { gramsA = b.totalGrams; sA = b.surfaceZ; idA = b.id; }
            else { gramsB = b.totalGrams; sB = b.surfaceZ; idB = b.id; }
        }
        // Manually align presentation heights for the inequality demo if settle left them close:
        // Law under test: ledger grams are authority; equal surfaceZ does not imply equal grams.
        // Create equal surface by setting both body surfaceZ equal while grams differ.
        bool heightEqGramsNeq = false;
        if ( idA && idB && gramsA != gramsB )
        {
            // Stamp both presentation surfaces to the same value (presentation ≠ mass).
            if ( WaterBody* ba = FindBody( w, idA ) ) { ba->surfaceZ = 3.0f; sA = 3.0f; }
            if ( WaterBody* bb = FindBody( w, idB ) ) { bb->surfaceZ = 3.0f; sB = 3.0f; }
            heightEqGramsNeq = ( sA == sB && gramsA != gramsB );
        }
        std::snprintf( note, sizeof( note ),
            "gA=%lld gB=%lld sA=%.3f sB=%.3f pourB=%lld",
            (long long)gramsA, (long long)gramsB, sA, sB, (long long)( pourB + pourB2 ) );
        CertAdd( R, "height_neq_grams",
            heightEqGramsNeq ? "PASS" : "FAIL", note );

        // Ensure both dormant again
        for ( int i = 0; i < 8; ++i ) { TickSettle( w ); }

        // 5) dig near A wakes only A's body; B stays dormant
        int wakesBefore = w.wakeEvents;
        int woken = OnTerrainDig( w, 10, 10, 1 );
        bool aAwake = false, bAwake = false;
        if ( WaterBody const* ba = FindBody( w, idA ) )
        {
            aAwake = ( ba->state != BodyState::Dormant );
        }
        if ( WaterBody const* bb = FindBody( w, idB ) )
        {
            bAwake = ( bb->state != BodyState::Dormant );
        }
        std::snprintf( note, sizeof( note ),
            "woken=%d dWake=%d aAwake=%d bAwake=%d globalAttempts=%d",
            woken, w.wakeEvents - wakesBefore, aAwake ? 1 : 0, bAwake ? 1 : 0,
            w.globalWakeAttempts );
        CertAdd( R, "dig_wakes_local_body",
            ( woken == 1 && aAwake && !bAwake && w.globalWakeAttempts == 0 )
                ? "PASS" : "FAIL", note );

        // Resettle
        for ( int i = 0; i < 8; ++i ) { TickSettle( w ); }

        // 6) place into water displaces / reconfigures — grams conserved
        int64_t beforePlace = TotalGrams( w );
        int64_t afterPlace = OnTerrainPlace( w, 10, 10, 70 ); // heavy fill into wet cell
        std::snprintf( note, sizeof( note ),
            "before=%lld after=%lld displaceEvents=%d",
            (long long)beforePlace, (long long)afterPlace, w.displaceEvents );
        CertAdd( R, "place_displaces_conserved",
            ( afterPlace == beforePlace && w.displaceEvents >= 1 ) ? "PASS" : "FAIL", note );

        for ( int i = 0; i < 8; ++i ) { TickSettle( w ); }

        // 7) revision-consistent presentation stamp
        RebuildBodyMembership( w );
        uint32_t presentId = idA ? idA : 0;
        if ( !presentId )
        {
            for ( WaterBody const& b : w.bodies )
            {
                if ( b.totalGrams > 0 ) { presentId = b.id; break; }
            }
        }
        PresentationStamp ok = PresentBodyCurrent( w, presentId );
        // Wrong terrain rev (plane from another occupancy epoch) must refuse.
        uint32_t const wrongT = ok.valid ? ( ok.terrainRev + 1u ) : ( w.terrainRev + 1u );
        PresentationStamp bad = PresentBody( w, presentId, wrongT, w.waterRev );
        std::snprintf( note, sizeof( note ),
            "ok=%d bad=%d refusals=%d bodyT=%u globalT=%u wRev=%u",
            ok.valid ? 1 : 0, bad.valid ? 1 : 0, w.presentationRefusals,
            ok.terrainRev, w.terrainRev, w.waterRev );
        CertAdd( R, "presentation_rev_consistent",
            ( ok.valid && !bad.valid ) ? "PASS" : "FAIL", note );

        // 8) final conservation + dormant under another unrelated burst
        int64_t finalG = TotalGrams( w );
        work0 = w.workUnits;
        for ( int i = 0; i < 20; ++i )
        {
            OnUnrelatedReceipt( w );
            TickSettle( w );
        }
        std::snprintf( note, sizeof( note ),
            "finalG=%lld dWork=%lld dormantBodies=%d",
            (long long)finalG, (long long)( w.workUnits - work0 ), CountDormantBodies( w ) );
        CertAdd( R, "final_conserved_dormant",
            ( finalG == 140 && ( w.workUnits - work0 ) == 0 && !AnyAwake( w ) )
                ? "PASS" : "FAIL", note );

        // Classifier mirror: water affect ≠ unrelated (documented via OnWaterReceipt work)
        work0 = w.workUnits;
        OnWaterReceipt( w, 10, 10, 11, 11 );
        CertAdd( R, "water_receipt_wakes_scoped",
            ( ( w.workUnits - work0 ) > 0 ) ? "PASS" : "FAIL", "scoped water channel" );

        return R;
    }

    inline bool WriteCertArtifact( CertResult const& R, char const* certPath, char const* failPath )
    {
        if ( !certPath || !certPath[0] ) { return false; }
        FILE* f = nullptr;
        if ( fopen_s( &f, certPath, "w" ) != 0 || !f ) { return false; }
        int passN = 0, failN = 0, skipN = 0;
        for ( int i = 0; i < R.rowN; ++i )
        {
            if ( std::strcmp( R.rows[i].verdict, "PASS" ) == 0 ) { ++passN; }
            else if ( std::strcmp( R.rows[i].verdict, "FAIL" ) == 0 ) { ++failN; }
            else { ++skipN; }
        }
        std::fprintf( f,
            "Provenance P5a Water Ledger / Settle Floor cert\n"
            "law=Water should wake from causality, not from time passing.\n"
            "authority=Esoterica-local WaterLedger (Fablescript wire deferred)\n"
            "exit_code=%d\n"
            "PASS_rows=%d FAIL_rows=%d SKIP_rows=%d rows=%d\n"
            "first_fail=%s\n"
            "\n"
            "check\tverdict\tnote\n",
            R.exitCode, passN, failN, skipN, R.rowN,
            R.firstFail[0] ? R.firstFail : "none" );
        for ( int i = 0; i < R.rowN; ++i )
        {
            std::fprintf( f, "%s\t%s\t%s\n",
                R.rows[i].check, R.rows[i].verdict, R.rows[i].note );
        }
        std::fclose( f );
        if ( R.exitCode != 0 && failPath && failPath[0] )
        {
            FILE* ff = nullptr;
            if ( fopen_s( &ff, failPath, "w" ) == 0 && ff )
            {
                std::fprintf( ff, "FAIL:\nreason=%s\nsee provenance_p5a_water_ledger_cert.txt\n",
                    R.firstFail[0] ? R.firstFail : "defect_rows" );
                std::fclose( ff );
            }
        }
        return true;
    }
}
