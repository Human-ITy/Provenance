#pragma once
#include "GraniteContactCast.h"

// Lab authoring only. Translate intact authoritative geometry, not a render proxy.
// No live strike transaction, soil receipt or recovered parcel is transformed.
namespace EE::GraniteLabPlacement
{
    namespace G = GraniteContactCast;
    inline bool Intact(G::Body const& body)
    {
        return !body.substrate.vertices.empty() && body.changed.empty() && body.damage.empty()
            && body.removedMg == 0 && body.removedM3 == 0 && body.nextChipID == 0;
    }
    inline bool Translate(G::Body& body, G::P delta)
    {
        if (!Intact(body) || !G::Finite(delta)) return false;
        for (double d : delta) if (std::abs(d) > 8) return false;
        if (G::Dot(delta, delta) == 0) return true;
        auto& base = body.substrate;
        for (auto& p : base.vertices) p = G::Add(p, delta);
        for (auto& piece : base.pieces)
        {
            piece.low = G::Add(piece.low, delta);
            piece.high = G::Add(piece.high, delta);
        }
        // Bin origin moves with its pieces: memberships and stable render-group
        // IDs do not change. Normals, connectivity, volume and integer mass stay.
        base.low = G::Add(base.low, delta); base.high = G::Add(base.high, delta);
        body.low = base.low; body.high = base.high;
        body.collisionQuery = G::UpdatedCollisionQuery(body, {});
        ++base.revision; ++body.revision;
        return true;
    }
}
