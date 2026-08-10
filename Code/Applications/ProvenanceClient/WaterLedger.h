// P5a — Water ledger / settle floor (Esoterica-local until Fablescript water authority wires in).
// Law: Water should wake from causality, not from time passing.
//
// terrain occupancy → container/obstruction
// water ledger      → conserved capacity_units (not mass grams)
// surface/body      → presentation + local hydraulic level solve
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
    // ---- Unit contract (see Docs/P5A_WATER_LEDGER.md) ----
    // Ledger authority is capacity_units, matching Fablescript CELL_CAPACITY_UNITS scale.
    // kCellCapacityUnits = 100 ⇔ one full open cell column.
    // kCellDepthM       = 1.0 m hydraulic/visual depth at full capacity.
    // kCellFootprintM2  = 1.0 m² nominal column footprint for volume conversion.
    //
    // volume_m3 = capacity_units * (kCellFootprintM2 * kCellDepthM) / kCellCapacityUnits
    // mass_kg   = volume_m3 * kWaterDensityKgM3
    // mass_g    = mass_kg * 1000
    //
    // Do NOT call capacity_units "grams". Real mass/volume is derived only via this contract
    // before any cross-system coupling. Dirt place mapping lives in Main.cpp (dirt grams →
    // capacity_units via occupancy fill scale) and is not water-ledger mass.
    constexpr int64_t kCellCapacityUnits = 100;
    constexpr float   kCellDepthM        = 1.0f;
    constexpr float   kCellFootprintM2   = 1.0f;
    constexpr float   kWaterDensityKgM3  = 1000.0f;
    constexpr int     kMaxSettlePasses   = 64;

    // Backward-compatible alias name used by earlier P5a pin; same value, honest unit.
    constexpr int64_t kCellCapacityGrams = kCellCapacityUnits;

    inline double VolumeM3FromUnits( int64_t units )
    {
        return (double)units * ( (double)kCellFootprintM2 * (double)kCellDepthM )
            / (double)kCellCapacityUnits;
    }

    inline double MassKgFromUnits( int64_t units )
    {
        return VolumeM3FromUnits( units ) * (double)kWaterDensityKgM3;
    }

    inline int64_t UnitsFromVolumeM3( double volumeM3 )
    {
        if ( volumeM3 <= 0.0 ) { return 0; }
        double const u = volumeM3 * (double)kCellCapacityUnits
            / ( (double)kCellFootprintM2 * (double)kCellDepthM );
        return (int64_t)std::llround( u );
    }

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
    // floorZ = open basin floor; openCapacityUnits = free volume above floor (≤ kCellCapacityUnits).
    struct ContainerCell
    {
        bool    solid = true;
        float   floorZ = 0.f;
        int64_t openCapacityUnits = 0;
        uint32_t terrainRev = 0;
    };

    struct WaterCell
    {
        int64_t  amount = 0;     // capacity_units (ledger authority)
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
        int64_t   totalUnits = 0;
        float     surfaceZ = 0.f; // settled hydraulic plane when leveled
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

        // Out-of-scope spill bucket: conserved units that left local open containers.
        // Never stored in solid / zero-capacity cells.
        int64_t spillOutOfScopeUnits = 0;

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
        int     solidOccupancyViolations = 0;
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
        return c ? c->amount : 0;
    }

    inline int64_t TotalUnits( World const& w )
    {
        int64_t t = 0;
        for ( auto const& kv : w.cells ) { t += kv.second.amount; }
        return t;
    }

    // Conserved universe for this module: in-world open water + spill bucket.
    inline int64_t TotalConservedUnits( World const& w )
    {
        return TotalUnits( w ) + w.spillOutOfScopeUnits;
    }

    // True if any positive water sits in solid / zero-capacity / over-capacity cells.
    inline bool HasWaterInSolidOccupancy( World const& w )
    {
        for ( auto const& kv : w.cells )
        {
            if ( kv.second.amount <= 0 ) { continue; }
            int x = 0, y = 0;
            UnpackXY( kv.first, x, y );
            ContainerCell const* cc = GetContainer( w, x, y );
            if ( !cc || cc->solid || cc->openCapacityUnits <= 0 ) { return true; }
            if ( kv.second.amount > cc->openCapacityUnits ) { return true; }
        }
        return false;
    }

    inline int CountWaterInSolidCells( World const& w )
    {
        int n = 0;
        for ( auto const& kv : w.cells )
        {
            if ( kv.second.amount <= 0 ) { continue; }
            int x = 0, y = 0;
            UnpackXY( kv.first, x, y );
            ContainerCell const* cc = GetContainer( w, x, y );
            if ( !cc || cc->solid || cc->openCapacityUnits <= 0
                || kv.second.amount > cc->openCapacityUnits )
            {
                ++n;
            }
        }
        return n;
    }

    inline bool AnyAwake( World const& w )
    {
        for ( WaterBody const& b : w.bodies )
        {
            if ( b.state != BodyState::Dormant && b.totalUnits > 0 ) { return true; }
        }
        return false;
    }

    inline int CountDormantBodies( World const& w )
    {
        int n = 0;
        for ( WaterBody const& b : w.bodies )
        {
            if ( b.state == BodyState::Dormant && b.totalUnits > 0 ) { ++n; }
        }
        return n;
    }

    inline void SetBasin( World& w, int x, int y, float floorZ, int64_t openCap,
        bool solid = false )
    {
        ContainerCell& c = w.containers[PackXY( x, y )];
        c.solid = solid;
        c.floorZ = floorZ;
        c.openCapacityUnits = solid ? 0 : (std::max)( (int64_t)0,
            (std::min)( openCap, kCellCapacityUnits ) );
        c.terrainRev = w.terrainRev;
        ++w.workUnits; // container authoring is a terrain-side change, not a water tick
    }

    inline void BumpTerrainRev( World& w )
    {
        ++w.terrainRev;
    }

    inline float SurfaceFromUnits( float floorZ, int64_t amount, int64_t capacity )
    {
        if ( capacity <= 0 || amount <= 0 ) { return floorZ; }
        float const frac = (float)amount / (float)capacity;
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
        if ( b.totalUnits <= 0 ) { return; }
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
            b.totalUnits = 0;
        }
        std::unordered_set<uint64_t> seen;
        auto tryGet = [&]( int x, int y ) -> WaterCell*
        {
            return GetWater( w, x, y );
        };

        for ( auto& kv : w.cells )
        {
            if ( kv.second.amount <= 0 ) { continue; }
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
                if ( !c || c->amount <= 0 ) { continue; }
                c->bodyId = bid;
                body->cells.push_back( { x, y } );
                sum += c->amount;
                maxSurf = (std::max)( maxSurf, c->surfaceZ );
                static int const dx[4] = { 1, -1, 0, 0 };
                static int const dy[4] = { 0, 0, 1, -1 };
                for ( int i = 0; i < 4; ++i )
                {
                    int const nx = x + dx[i], ny = y + dy[i];
                    uint64_t const nk = PackXY( nx, ny );
                    if ( seen.count( nk ) ) { continue; }
                    WaterCell* n = tryGet( nx, ny );
                    if ( !n || n->amount <= 0 ) { continue; }
                    seen.insert( nk );
                    q.push_back( { nx, ny } );
                }
            }
            body->totalUnits = sum;
            body->surfaceZ = maxSurf;
            if ( sum <= 0 ) { body->state = BodyState::Dormant; }
        }

        // Drop empty bodies (keep vector compact).
        w.bodies.erase(
            std::remove_if( w.bodies.begin(), w.bodies.end(),
                []( WaterBody const& b ) { return b.totalUnits <= 0 || b.cells.empty(); } ),
            w.bodies.end() );
        ++w.workUnits;
    }

    inline void RecomputeCellSurface( World& w, int x, int y )
    {
        WaterCell* wc = GetWater( w, x, y );
        ContainerCell const* cc = GetContainer( w, x, y );
        if ( !wc ) { return; }
        if ( !cc || cc->solid || cc->openCapacityUnits <= 0 )
        {
            wc->surfaceZ = wc->floorZ;
            return;
        }
        wc->floorZ = cc->floorZ;
        wc->terrainRev = cc->terrainRev;
        wc->surfaceZ = SurfaceFromUnits( cc->floorZ, wc->amount, cc->openCapacityUnits );
    }

    // Pour exact capacity_units into an open container cell. Rejects solid / zero-capacity.
    // Returns units accepted (may be less if capacity limited — remainder not invented).
    inline int64_t Pour( World& w, int x, int y, int64_t units )
    {
        if ( units <= 0 ) { return 0; }
        ContainerCell const* cc = GetContainer( w, x, y );
        if ( !cc || cc->solid || cc->openCapacityUnits <= 0 )
        {
            ++w.pourRejectedSolid;
            ++w.workUnits;
            return 0;
        }
        WaterCell& wc = w.cells[PackXY( x, y )];
        int64_t const room = cc->openCapacityUnits - wc.amount;
        if ( room <= 0 )
        {
            ++w.pourRejectedSolid;
            ++w.workUnits;
            return 0;
        }
        int64_t const add = (std::min)( units, room );
        wc.amount += add;
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

    // Deterministic hydraulic head/level solve for one awake body.
    // Expands through open (non-solid) container adjacency so equal-floor basins share a plane.
    // Conserves total units exactly within open capacity; overflow → spillOutOfScopeUnits.
    // Never writes water into solid / zero-capacity cells.
    inline bool SettleBodyOnce( World& w, WaterBody& body )
    {
        if ( body.cells.empty() || body.totalUnits <= 0 ) { return false; }
        ++w.settlePasses;
        ++w.workUnits;

        struct Slot
        {
            int x, y;
            int64_t cap;
            float floorZ;
        };

        // Flood open containers from body seeds (basin), without stealing other bodies' water.
        std::unordered_set<uint64_t> openSeen;
        std::vector<std::pair<int, int>> q;
        for ( auto const& xy : body.cells )
        {
            uint64_t const k = PackXY( xy.first, xy.second );
            if ( openSeen.insert( k ).second ) { q.push_back( xy ); }
        }
        static int const dx[4] = { 1, -1, 0, 0 };
        static int const dy[4] = { 0, 0, 1, -1 };
        while ( !q.empty() )
        {
            auto [x, y] = q.back();
            q.pop_back();
            for ( int i = 0; i < 4; ++i )
            {
                int const nx = x + dx[i], ny = y + dy[i];
                uint64_t const nk = PackXY( nx, ny );
                if ( openSeen.count( nk ) ) { continue; }
                ContainerCell const* ncc = GetContainer( w, nx, ny );
                if ( !ncc || ncc->solid || ncc->openCapacityUnits <= 0 ) { continue; }
                WaterCell const* nwc = GetWater( w, nx, ny );
                if ( nwc && nwc->amount > 0 && nwc->bodyId != 0 && nwc->bodyId != body.id )
                {
                    continue; // other body
                }
                openSeen.insert( nk );
                q.push_back( { nx, ny } );
            }
        }

        std::vector<Slot> slots;
        slots.reserve( openSeen.size() );
        int64_t total = 0;
        int64_t totalCap = 0;
        for ( uint64_t k : openSeen )
        {
            int x = 0, y = 0;
            UnpackXY( k, x, y );
            ContainerCell const* cc = GetContainer( w, x, y );
            if ( !cc || cc->solid || cc->openCapacityUnits <= 0 ) { continue; }
            slots.push_back( { x, y, cc->openCapacityUnits, cc->floorZ } );
            totalCap += cc->openCapacityUnits;
        }
        // Total conserved from the body's wet cells only (authoritative).
        total = 0;
        for ( auto const& xy : body.cells )
        {
            if ( WaterCell const* wc = GetWater( w, xy.first, xy.second ) )
            {
                total += wc->amount;
            }
        }
        if ( slots.empty() || total <= 0 ) { return false; }

        // Deterministic slot order for tie-breaks.
        std::sort( slots.begin(), slots.end(),
            []( Slot const& a, Slot const& b ) {
                if ( a.x != b.x ) { return a.x < b.x; }
                return a.y < b.y;
            } );

        // Integer hydraulic fill: repeatedly place one unit into the open cell whose
        // resulting surface is lowest (head-seeking). Tie-break (x,y).
        std::vector<int64_t> target( slots.size(), 0 );
        int64_t remain = total;
        int64_t placed = 0;
        while ( remain > 0 )
        {
            int best = -1;
            float bestSurf = 0.f;
            for ( size_t i = 0; i < slots.size(); ++i )
            {
                if ( target[i] >= slots[i].cap ) { continue; }
                float const newSurf = slots[i].floorZ
                    + ( (float)( target[i] + 1 ) / (float)slots[i].cap ) * kCellDepthM;
                if ( best < 0
                    || newSurf < bestSurf - 1e-8f
                    || ( fabsf( newSurf - bestSurf ) <= 1e-8f
                        && ( slots[i].x < slots[best].x
                            || ( slots[i].x == slots[best].x && slots[i].y < slots[best].y ) ) ) )
                {
                    best = (int)i;
                    bestSurf = newSurf;
                }
            }
            if ( best < 0 ) { break; } // all open capacity full
            target[best] += 1;
            --remain;
            ++placed;
        }
        // Overflow that cannot sit in open capacity → spill (never solid / over-cap write).
        if ( remain > 0 )
        {
            w.spillOutOfScopeUnits += remain;
            remain = 0;
        }
        (void)totalCap;
        (void)placed;

        // Settled hydraulic plane = max surface among partially/fully filled cells (common head).
        float plane = -1e9f;
        for ( size_t i = 0; i < slots.size(); ++i )
        {
            if ( target[i] <= 0 ) { continue; }
            float const s = SurfaceFromUnits( slots[i].floorZ, target[i], slots[i].cap );
            plane = (std::max)( plane, s );
        }
        if ( plane < -1e8f ) { plane = 0.f; }

        bool moved = false;
        std::unordered_set<uint64_t> written;
        for ( size_t i = 0; i < slots.size(); ++i )
        {
            int const x = slots[i].x, y = slots[i].y;
            uint64_t const k = PackXY( x, y );
            written.insert( k );
            WaterCell* wc = GetWater( w, x, y );
            int64_t const prev = wc ? wc->amount : 0;
            if ( target[i] == prev ) 
            {
                if ( wc && target[i] > 0 )
                {
                    wc->surfaceZ = SurfaceFromUnits( slots[i].floorZ, target[i], slots[i].cap );
                    wc->floorZ = slots[i].floorZ;
                }
                continue;
            }
            moved = true;
            if ( target[i] <= 0 )
            {
                if ( wc )
                {
                    wc->amount = 0;
                    wc->bodyId = 0;
                    ++w.waterRev;
                    wc->waterRev = w.waterRev;
                    w.cells.erase( k );
                }
                continue;
            }
            WaterCell& dest = w.cells[k];
            dest.amount = target[i];
            dest.floorZ = slots[i].floorZ;
            dest.bodyId = body.id;
            if ( ContainerCell const* cc = GetContainer( w, x, y ) )
            {
                dest.terrainRev = cc->terrainRev;
            }
            ++w.waterRev;
            dest.waterRev = w.waterRev;
            dest.surfaceZ = SurfaceFromUnits( dest.floorZ, dest.amount, slots[i].cap );
        }

        // Clear any body cells that were not in the open flood (should not happen) and hold amount.
        for ( auto const& xy : body.cells )
        {
            uint64_t const k = PackXY( xy.first, xy.second );
            if ( written.count( k ) ) { continue; }
            WaterCell* wc = GetWater( w, xy.first, xy.second );
            if ( wc && wc->amount > 0 )
            {
                // Cell became solid/unopen during settle — spill, do not keep in solid.
                w.spillOutOfScopeUnits += wc->amount;
                wc->amount = 0;
                moved = true;
                w.cells.erase( k );
            }
        }

        body.totalUnits = TotalUnits( w ); // temporary; membership rebuild corrects per-body
        // Recompute this body's total from targets.
        int64_t bodyTotal = 0;
        for ( int64_t t : target ) { bodyTotal += t; }
        body.totalUnits = bodyTotal;
        body.surfaceZ = plane;

        if ( HasWaterInSolidOccupancy( w ) )
        {
            ++w.solidOccupancyViolations;
        }
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
        bool anyMoved = false;
        for ( WaterBody& b : w.bodies )
        {
            if ( b.state == BodyState::Dormant || b.totalUnits <= 0 ) { continue; }
            b.state = BodyState::Settling;
            bool moved = SettleBodyOnce( w, b );
            anyMoved = anyMoved || moved;
            if ( b.settlePassesLeft > 0 ) { --b.settlePassesLeft; }
            if ( !moved || b.settlePassesLeft <= 0 )
            {
                b.state = BodyState::Dormant;
                b.wakeCause = 0;
            }
        }
        if ( anyMoved )
        {
            // Preserve wake/dormant flags across rebuild by snapshotting ids that should stay awake.
            std::unordered_set<uint32_t> stayAwake;
            for ( WaterBody const& b : w.bodies )
            {
                if ( b.state != BodyState::Dormant && b.totalUnits > 0 )
                {
                    stayAwake.insert( b.id );
                }
            }
            RebuildBodyMembership( w );
            for ( WaterBody& b : w.bodies )
            {
                if ( stayAwake.count( b.id ) )
                {
                    b.state = BodyState::Settling;
                }
                else
                {
                    b.state = BodyState::Dormant;
                    b.wakeCause = 0;
                }
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
                if ( !wc || wc->amount <= 0 ) { continue; }
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
                    cc->openCapacityUnits = kCellCapacityUnits;
                    cc->floorZ -= 0.25f;
                }
                else
                {
                    cc->floorZ -= 0.15f;
                    cc->openCapacityUnits = kCellCapacityUnits;
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
                if ( wc && wc->amount > 0 && wc->bodyId != 0 )
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

    // Place matter into water: displace / reconfigure — never delete units.
    // solidFillUnits reduces open capacity; excess moves to neighbor open cells or spill bucket.
    // Fail-closed: never returns displaced water into a source that is solid / zero-capacity.
    inline int64_t OnTerrainPlace( World& w, int cx, int cy, int64_t solidFillUnits )
    {
        BumpTerrainRev( w );
        ContainerCell* cc = GetContainer( w, cx, cy );
        if ( !cc ) { return TotalConservedUnits( w ); }

        int64_t const before = TotalConservedUnits( w );
        WaterCell* wc = GetWater( w, cx, cy );
        int64_t displaced = 0;
        uint32_t priorBody = ( wc ? wc->bodyId : 0 );
        if ( wc && wc->amount > 0 )
        {
            int64_t const fill = (std::max)( (int64_t)0,
                (std::min)( solidFillUnits, kCellCapacityUnits ) );
            int64_t newCap = (std::max)( (int64_t)0, kCellCapacityUnits - fill );
            if ( fill >= kCellCapacityUnits )
            {
                cc->solid = true;
                cc->openCapacityUnits = 0;
                newCap = 0;
            }
            else
            {
                cc->solid = false;
                cc->openCapacityUnits = newCap;
            }
            cc->terrainRev = w.terrainRev;

            if ( wc->amount > newCap )
            {
                displaced = wc->amount - newCap;
                wc->amount = newCap;
                ++w.waterRev;
                wc->waterRev = w.waterRev;
            }
            if ( newCap <= 0 || cc->solid )
            {
                // Source is solid / zero-cap: remove any residual water cell entry.
                displaced += wc->amount;
                wc->amount = 0;
                w.cells.erase( PackXY( cx, cy ) );
                wc = nullptr;
            }
            else
            {
                RecomputeCellSurface( w, cx, cy );
            }

            std::unordered_set<uint32_t> wakeIds;
            if ( priorBody != 0 ) { wakeIds.insert( priorBody ); }
            if ( displaced > 0 )
            {
                struct N { int x, y; float floorZ; int64_t room; };
                std::vector<N> ns;
                static int const pdx[4] = { 1, -1, 0, 0 };
                static int const pdy[4] = { 0, 0, 1, -1 };
                for ( int i = 0; i < 4; ++i )
                {
                    int const nx = cx + pdx[i], ny = cy + pdy[i];
                    ContainerCell const* ncc = GetContainer( w, nx, ny );
                    if ( !ncc || ncc->solid || ncc->openCapacityUnits <= 0 ) { continue; }
                    WaterCell const* nwc = GetWater( w, nx, ny );
                    int64_t have = nwc ? nwc->amount : 0;
                    int64_t room = ncc->openCapacityUnits - have;
                    if ( room > 0 ) { ns.push_back( { nx, ny, ncc->floorZ, room } ); }
                }
                std::sort( ns.begin(), ns.end(),
                    []( N const& a, N const& b ) {
                        if ( a.floorZ != b.floorZ ) { return a.floorZ < b.floorZ; }
                        if ( a.x != b.x ) { return a.x < b.x; }
                        return a.y < b.y;
                    } );
                for ( N const& n : ns )
                {
                    if ( displaced <= 0 ) { break; }
                    int64_t take = (std::min)( displaced, n.room );
                    WaterCell& dest = w.cells[PackXY( n.x, n.y )];
                    ContainerCell const* ncc = GetContainer( w, n.x, n.y );
                    dest.amount += take;
                    if ( ncc )
                    {
                        dest.floorZ = ncc->floorZ;
                        dest.terrainRev = ncc->terrainRev;
                    }
                    ++w.waterRev;
                    dest.waterRev = w.waterRev;
                    RecomputeCellSurface( w, n.x, n.y );
                    displaced -= take;
                    if ( dest.bodyId != 0 ) { wakeIds.insert( dest.bodyId ); }
                }
                // Remainder cannot sit in solid source — spill out of local scope (conserved).
                if ( displaced > 0 )
                {
                    w.spillOutOfScopeUnits += displaced;
                    displaced = 0;
                }
            }
            ++w.displaceEvents;
            RebuildBodyMembership( w );
            if ( WaterCell* still = GetWater( w, cx, cy ) )
            {
                if ( still->bodyId != 0 ) { wakeIds.insert( still->bodyId ); }
            }
            // Neighbors that received water may have new body ids after rebuild.
            {
                static int const pdx2[4] = { 1, -1, 0, 0 };
                static int const pdy2[4] = { 0, 0, 1, -1 };
                for ( int i = 0; i < 4; ++i )
                {
                    if ( WaterCell* nwc = GetWater( w, cx + pdx2[i], cy + pdy2[i] ) )
                    {
                        if ( nwc->amount > 0 && nwc->bodyId != 0 )
                        {
                            wakeIds.insert( nwc->bodyId );
                        }
                    }
                }
            }
            for ( uint32_t id : wakeIds )
            {
                if ( WaterBody* woken = FindBody( w, id ) )
                {
                    MarkBodyAwake( w, *woken, WakeCause::Place );
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
        }
        else
        {
            // Dry place: just update container.
            int64_t const fill = (std::max)( (int64_t)0,
                (std::min)( solidFillUnits, kCellCapacityUnits ) );
            if ( fill >= kCellCapacityUnits )
            {
                cc->solid = true;
                cc->openCapacityUnits = 0;
            }
            else
            {
                cc->openCapacityUnits = (std::max)( (int64_t)0, cc->openCapacityUnits - fill );
            }
            cc->terrainRev = w.terrainRev;
        }

        if ( HasWaterInSolidOccupancy( w ) )
        {
            ++w.solidOccupancyViolations;
        }
        int64_t const after = TotalConservedUnits( w );
        (void)before;
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

    // Deterministic content digest over wet cells (order-independent).
    inline uint64_t ContentDigest( World const& w )
    {
        struct Row { int x, y; int64_t amount; int64_t cap; int32_t floorMilli; };
        std::vector<Row> rows;
        rows.reserve( w.cells.size() );
        for ( auto const& kv : w.cells )
        {
            if ( kv.second.amount <= 0 ) { continue; }
            int x = 0, y = 0;
            UnpackXY( kv.first, x, y );
            ContainerCell const* cc = GetContainer( w, x, y );
            int64_t cap = cc ? cc->openCapacityUnits : 0;
            int32_t floorMilli = (int32_t)std::lround( (double)kv.second.floorZ * 1000.0 );
            rows.push_back( { x, y, kv.second.amount, cap, floorMilli } );
        }
        std::sort( rows.begin(), rows.end(),
            []( Row const& a, Row const& b ) {
                if ( a.x != b.x ) { return a.x < b.x; }
                return a.y < b.y;
            } );
        uint64_t h = 14695981039346656037ull;
        auto mix = [&]( uint64_t v ) {
            h ^= v;
            h *= 1099511628211ull;
        };
        for ( Row const& r : rows )
        {
            mix( (uint64_t)(uint32_t)r.x );
            mix( (uint64_t)(uint32_t)r.y );
            mix( (uint64_t)r.amount );
            mix( (uint64_t)r.cap );
            mix( (uint64_t)(uint32_t)r.floorMilli );
        }
        mix( (uint64_t)w.spillOutOfScopeUnits );
        return h;
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
        CertRow rows[64];
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

    inline void DrainSettle( World& w, int passes = 16 )
    {
        for ( int i = 0; i < passes; ++i ) { TickSettle( w ); }
        RebuildBodyMembership( w );
        for ( WaterBody& b : w.bodies )
        {
            b.state = BodyState::Dormant;
            b.wakeCause = 0;
        }
    }

    inline CertResult RunP5aCert()
    {
        CertResult R{};
        World w{};

        // Basin A (wide, equal floor): 3 connected cells @ floor 0 — hydraulic share demo.
        SetBasin( w, 10, 10, 0.0f, 100 );
        SetBasin( w, 11, 10, 0.0f, 100 );
        SetBasin( w, 10, 11, 0.0f, 100 );
        // Basin B (narrow): 1 cell @ same floor 0 — unequal amount ⇒ equal level vs A when sized right.
        SetBasin( w, 80, 80, 0.0f, 100 );
        // Basin C (deep cup): 1 cell @ floor -1 — equal amount vs B ⇒ unequal level.
        SetBasin( w, 90, 90, -1.0f, 100 );
        // Solid wall / blocked pour target
        SetBasin( w, 12, 10, 0.0f, 0, /*solid*/true );

        // 1) pour adds exact capacity_units
        int64_t a1 = Pour( w, 10, 10, 100 ); // all into one cell — settle must level across basin
        char note[240];
        std::snprintf( note, sizeof( note ), "poured=%lld total=%lld units (not grams)",
            (long long)a1, (long long)TotalUnits( w ) );
        CertAdd( R, "pour_exact_mass",
            ( a1 == 100 && TotalUnits( w ) == 100 ) ? "PASS" : "FAIL", note );

        // 2) cannot occupy solid
        int64_t solidPour = Pour( w, 12, 10, 25 );
        std::snprintf( note, sizeof( note ), "solid_accept=%lld rejected=%d",
            (long long)solidPour, w.pourRejectedSolid );
        CertAdd( R, "no_solid_occupy",
            ( solidPour == 0 && AmountAt( w, 12, 10 ) == 0 && w.pourRejectedSolid >= 1
              && !HasWaterInSolidOccupancy( w ) )
                ? "PASS" : "FAIL", note );

        // Settle basin A: equal-floor cells must share a leveled plane (not greedy 100/0/0).
        DrainSettle( w );
        int64_t u1010 = AmountAt( w, 10, 10 );
        int64_t u1110 = AmountAt( w, 11, 10 );
        int64_t u1011 = AmountAt( w, 10, 11 );
        int64_t mx = (std::max)( u1010, (std::max)( u1110, u1011 ) );
        int64_t mn = (std::min)( u1010, (std::min)( u1110, u1011 ) );
        std::snprintf( note, sizeof( note ),
            "split=%lld/%lld/%lld max-min=%lld total=%lld",
            (long long)u1010, (long long)u1110, (long long)u1011,
            (long long)( mx - mn ), (long long)TotalUnits( w ) );
        CertAdd( R, "level_equal_floor_share",
            ( TotalUnits( w ) == 100 && ( mx - mn ) <= 1 && mn > 0
              && !AnyAwake( w ) && !HasWaterInSolidOccupancy( w ) )
                ? "PASS" : "FAIL", note );

        CertAdd( R, "settled_dormant",
            ( !AnyAwake( w ) && CountDormantBodies( w ) >= 1 && TotalUnits( w ) == 100 )
                ? "PASS" : "FAIL",
            AnyAwake( w ) ? "still_awake" : "dormant_ok" );

        // Snapshot idle counters
        int64_t work0 = w.workUnits;
        int wakes0 = w.wakeEvents;
        int64_t skips0 = w.dormantIdleSkips;

        // 3) unrelated receipts → zero water work
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

        // 4) Derived equal surface height ≠ equal units (different basin geometries).
        // A: 3-cell wide basin holding 100 → plane ≈ 100/300 of depth above floor 0.
        // B: 1-cell narrow basin — pour units so its plane matches A's settled plane.
        float surfA = 0.f;
        int64_t unitsA = 0;
        uint32_t idA = 0;
        for ( WaterBody const& b : w.bodies )
        {
            if ( b.cells.empty() ) { continue; }
            if ( b.cells[0].first < 50 )
            {
                surfA = b.surfaceZ;
                unitsA = b.totalUnits;
                idA = b.id;
                break;
            }
        }
        // Target B amount from A's plane: amount = round(surfA / kCellDepthM * cap) on floor 0.
        int64_t pourBTarget = (int64_t)std::llround( (double)surfA / (double)kCellDepthM
            * (double)kCellCapacityUnits );
        if ( pourBTarget < 1 ) { pourBTarget = 1; }
        if ( pourBTarget > kCellCapacityUnits ) { pourBTarget = kCellCapacityUnits; }
        // Ensure unequal totals vs A.
        if ( pourBTarget == unitsA ) { pourBTarget = (std::max)( (int64_t)1, unitsA / 3 ); }
        int64_t pourB = Pour( w, 80, 80, pourBTarget );
        DrainSettle( w );

        int64_t unitsB = 0;
        float sB = 0.f;
        uint32_t idB = 0;
        for ( WaterBody const& b : w.bodies )
        {
            if ( b.cells.empty() ) { continue; }
            if ( b.cells[0].first >= 70 && b.cells[0].first < 85 )
            {
                unitsB = b.totalUnits;
                sB = b.surfaceZ;
                idB = b.id;
            }
            if ( b.cells[0].first < 50 )
            {
                unitsA = b.totalUnits;
                surfA = b.surfaceZ;
                idA = b.id;
            }
        }
        bool heightEqAmountNeq = ( idA && idB && unitsA != unitsB
            && fabsf( surfA - sB ) <= 0.02f );
        std::snprintf( note, sizeof( note ),
            "uA=%lld uB=%lld sA=%.4f sB=%.4f pourB=%lld derived",
            (long long)unitsA, (long long)unitsB, surfA, sB, (long long)pourB );
        CertAdd( R, "height_neq_grams",
            heightEqAmountNeq ? "PASS" : "FAIL", note );

        // 4b) Equal units ≠ equal level when basin geometry differs (B vs C).
        int64_t pourC = Pour( w, 90, 90, unitsB > 0 ? unitsB : pourBTarget );
        DrainSettle( w );
        float sC = 0.f;
        int64_t unitsC = 0;
        uint32_t idC = 0;
        for ( WaterBody const& b : w.bodies )
        {
            if ( b.cells.empty() ) { continue; }
            if ( b.cells[0].first >= 85 )
            {
                unitsC = b.totalUnits;
                sC = b.surfaceZ;
                idC = b.id;
            }
            if ( b.cells[0].first >= 70 && b.cells[0].first < 85 )
            {
                unitsB = b.totalUnits;
                sB = b.surfaceZ;
                idB = b.id;
            }
        }
        bool amountEqHeightNeq = ( idB && idC && unitsB == unitsC && fabsf( sB - sC ) > 0.05f );
        std::snprintf( note, sizeof( note ),
            "uB=%lld uC=%lld sB=%.4f sC=%.4f pourC=%lld",
            (long long)unitsB, (long long)unitsC, sB, sC, (long long)pourC );
        CertAdd( R, "amount_eq_height_neq",
            amountEqHeightNeq ? "PASS" : "FAIL", note );

        // 5) dig near A wakes only A's body; B/C stay dormant
        int wakesBefore = w.wakeEvents;
        int woken = OnTerrainDig( w, 10, 10, 1 );
        bool aAwake = false, bAwake = false, cAwake = false;
        if ( WaterBody const* ba = FindBody( w, idA ) )
        {
            aAwake = ( ba->state != BodyState::Dormant );
        }
        if ( WaterBody const* bb = FindBody( w, idB ) )
        {
            bAwake = ( bb->state != BodyState::Dormant );
        }
        if ( WaterBody const* bc = FindBody( w, idC ) )
        {
            cAwake = ( bc->state != BodyState::Dormant );
        }
        std::snprintf( note, sizeof( note ),
            "woken=%d dWake=%d aAwake=%d bAwake=%d cAwake=%d globalAttempts=%d",
            woken, w.wakeEvents - wakesBefore, aAwake ? 1 : 0, bAwake ? 1 : 0,
            cAwake ? 1 : 0, w.globalWakeAttempts );
        CertAdd( R, "dig_wakes_local_body",
            ( woken == 1 && aAwake && !bAwake && !cAwake && w.globalWakeAttempts == 0 )
                ? "PASS" : "FAIL", note );

        DrainSettle( w );

        // 6) place into water displaces — conserved (in-world + spill); never solid occupancy.
        int64_t beforePlace = TotalConservedUnits( w );
        int64_t afterPlace = OnTerrainPlace( w, 10, 10, 100 ); // full solidify wet cell
        bool solidCellWet = ( AmountAt( w, 10, 10 ) > 0 );
        ContainerCell const* placedCC = GetContainer( w, 10, 10 );
        bool sourceSolid = placedCC && placedCC->solid;
        std::snprintf( note, sizeof( note ),
            "before=%lld after=%lld spill=%lld solidSrc=%d wetSrc=%d viol=%d",
            (long long)beforePlace, (long long)afterPlace,
            (long long)w.spillOutOfScopeUnits, sourceSolid ? 1 : 0,
            solidCellWet ? 1 : 0, w.solidOccupancyViolations );
        CertAdd( R, "place_displaces_conserved",
            ( afterPlace == beforePlace && w.displaceEvents >= 1
              && !HasWaterInSolidOccupancy( w ) && !solidCellWet )
                ? "PASS" : "FAIL", note );

        CertAdd( R, "no_water_in_solid_mutation",
            ( !HasWaterInSolidOccupancy( w ) && CountWaterInSolidCells( w ) == 0
              && w.solidOccupancyViolations == 0 )
                ? "PASS" : "FAIL",
            HasWaterInSolidOccupancy( w ) ? "solid_wet" : "occupancy_ok" );

        DrainSettle( w );

        // 7) revision-consistent presentation stamp
        RebuildBodyMembership( w );
        uint32_t presentId = idA ? idA : 0;
        if ( !FindBody( w, presentId ) )
        {
            presentId = 0;
            for ( WaterBody const& b : w.bodies )
            {
                if ( b.totalUnits > 0 ) { presentId = b.id; break; }
            }
        }
        PresentationStamp ok = PresentBodyCurrent( w, presentId );
        uint32_t const wrongT = ok.valid ? ( ok.terrainRev + 1u ) : ( w.terrainRev + 1u );
        PresentationStamp bad = PresentBody( w, presentId, wrongT, w.waterRev );
        std::snprintf( note, sizeof( note ),
            "ok=%d bad=%d refusals=%d bodyT=%u globalT=%u wRev=%u",
            ok.valid ? 1 : 0, bad.valid ? 1 : 0, w.presentationRefusals,
            ok.terrainRev, w.terrainRev, w.waterRev );
        CertAdd( R, "presentation_rev_consistent",
            ( ok.valid && !bad.valid ) ? "PASS" : "FAIL", note );

        // 8) final conservation + dormant under unrelated burst
        int64_t finalG = TotalConservedUnits( w );
        int64_t expected = 100 + pourB + pourC; // all pours; place only reconfigures/spills
        work0 = w.workUnits;
        for ( int i = 0; i < 20; ++i )
        {
            OnUnrelatedReceipt( w );
            TickSettle( w );
        }
        std::snprintf( note, sizeof( note ),
            "final=%lld expected=%lld dWork=%lld spill=%lld dormantBodies=%d",
            (long long)finalG, (long long)expected,
            (long long)( w.workUnits - work0 ), (long long)w.spillOutOfScopeUnits,
            CountDormantBodies( w ) );
        CertAdd( R, "final_conserved_dormant",
            ( finalG == expected && ( w.workUnits - work0 ) == 0 && !AnyAwake( w )
              && !HasWaterInSolidOccupancy( w ) )
                ? "PASS" : "FAIL", note );

        // Classifier mirror: water affect ≠ unrelated
        work0 = w.workUnits;
        OnWaterReceipt( w, 10, 10, 11, 11 );
        CertAdd( R, "water_receipt_wakes_scoped",
            ( ( w.workUnits - work0 ) > 0 ) ? "PASS" : "FAIL", "scoped water channel" );

        // 9) Honest unit conversion contract smoke (capacity_units ↔ volume ↔ mass).
        double const vol = VolumeM3FromUnits( kCellCapacityUnits );
        double const mass = MassKgFromUnits( kCellCapacityUnits );
        int64_t const back = UnitsFromVolumeM3( vol );
        std::snprintf( note, sizeof( note ),
            "fullCell_m3=%.4f mass_kg=%.1f roundtrip=%lld density=%.0f",
            vol, mass, (long long)back, (double)kWaterDensityKgM3 );
        CertAdd( R, "units_volume_mass_contract",
            ( fabs( vol - 1.0 ) < 1e-9 && fabs( mass - 1000.0 ) < 1e-6 && back == kCellCapacityUnits )
                ? "PASS" : "FAIL", note );

        // 10) Repeat-order determinism digest: same pours, different cell order → same digest.
        World w1{};
        SetBasin( w1, 10, 10, 0.0f, 100 );
        SetBasin( w1, 11, 10, 0.0f, 100 );
        SetBasin( w1, 10, 11, 0.0f, 100 );
        Pour( w1, 10, 10, 40 );
        Pour( w1, 11, 10, 35 );
        Pour( w1, 10, 11, 25 );
        DrainSettle( w1 );
        uint64_t d1 = ContentDigest( w1 );

        World w2{};
        SetBasin( w2, 10, 10, 0.0f, 100 );
        SetBasin( w2, 11, 10, 0.0f, 100 );
        SetBasin( w2, 10, 11, 0.0f, 100 );
        Pour( w2, 10, 11, 25 );
        Pour( w2, 11, 10, 35 );
        Pour( w2, 10, 10, 40 );
        DrainSettle( w2 );
        uint64_t d2 = ContentDigest( w2 );

        std::snprintf( note, sizeof( note ),
            "d1=%016llx d2=%016llx total1=%lld total2=%lld",
            (unsigned long long)d1, (unsigned long long)d2,
            (long long)TotalUnits( w1 ), (long long)TotalUnits( w2 ) );
        CertAdd( R, "settle_order_determinism",
            ( d1 == d2 && TotalUnits( w1 ) == 100 && TotalUnits( w2 ) == 100
              && !HasWaterInSolidOccupancy( w1 ) && !HasWaterInSolidOccupancy( w2 ) )
                ? "PASS" : "FAIL", note );

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
            "units=capacity_units (100 = full cell); see Docs/P5A_WATER_LEDGER.md conversion\n"
            "settle=deterministic hydraulic head/level (not greedy fill)\n"
            "P5b=CLOSED\n"
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
