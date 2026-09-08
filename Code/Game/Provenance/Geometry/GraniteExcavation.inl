// Compiled through GraniteGeometry.cpp in both runtime and standalone verifier.
#include "GraniteExcavation.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace EE::GraniteExcavation
{
    namespace
    {
        double Dot( Point const& a, Point const& b )
        { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }

        bool Finite( Point const& p )
        { return std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]); }

        Point Minimum( int id )
        { return { (id % Side)*CellM, ((id / Side) % Side)*CellM, (id / (Side*Side))*CellM }; }

        bool ValidStrike( Strike const& s )
        {
            double const length2 = Dot(s.direction, s.direction);
            return Finite(s.rayOrigin) && Finite(s.direction) &&
                std::isfinite(length2) && length2 > 1e-20 &&
                std::isfinite(s.reachM) && s.reachM > 0 && s.reachM <= 10 &&
                std::isfinite(s.radiusM) && s.radiusM > 0 && s.radiusM <= 0.1 &&
                std::isfinite(s.energyJ) && s.energyJ >= 0 && s.energyJ <= 1e9;
        }

        Point Unit( Point p )
        { double const length = std::sqrt(Dot(p,p)); for (double& v : p) v /= length; return p; }

        bool ValidState( Specimen const& s )
        {
            if (s.cells.size() != Side*Side*Side) return false;
            for (Cell const& c : s.cells)
                if (!std::isfinite(c.damageJ) || c.damageJ < 0 || (!c.occupied && c.damageJ != 0)) return false;
            return true;
        }

        Contact Trace( Specimen const& specimen, Point const& origin, Point const& direction, double reach )
        {
            Contact result;
            // Interior origins are not surface strikes; do not tunnel to the
            // next occupied cell after skipping the cell containing the origin.
            bool insideBounds = true;
            int originID = 0, stride = 1;
            for (int axis = 0; axis < 3; ++axis)
            {
                if (origin[axis] < 0 || origin[axis] >= Side*CellM) insideBounds = false;
                else originID += int(std::floor(origin[axis]/CellM))*stride;
                stride *= Side;
            }
            if (insideBounds && specimen.cells[originID].occupied) return result;
            double closest = reach;
            for (int id = 0; id < int(specimen.cells.size()); ++id)
            {
                if (!specimen.cells[id].occupied) continue;
                Point const low = Minimum(id);
                double enter = -std::numeric_limits<double>::infinity();
                double leave = std::numeric_limits<double>::infinity();
                Point normal = {};
                bool intersects = true;
                for (int axis = 0; axis < 3; ++axis)
                {
                    if (std::abs(direction[axis]) < 1e-15)
                    {
                        // Half-open slabs give boundary rays one deterministic cell owner.
                        if (origin[axis] < low[axis] || origin[axis] >= low[axis]+CellM)
                            intersects = false;
                        continue;
                    }
                    double a = (low[axis]-origin[axis])/direction[axis];
                    double b = (low[axis]+CellM-origin[axis])/direction[axis];
                    double sign = -1;
                    if (a > b) { std::swap(a,b); sign = 1; }
                    if (a > enter) { enter = a; normal = {}; normal[axis] = sign; }
                    leave = std::min(leave,b);
                }
                // Inside-origin and tangential contacts do not excavate hidden interior cells.
                if (!intersects || leave <= enter || enter < 0 || enter > closest) continue;
                if (result.hit && enter == closest) continue;
                closest = enter;
                result.hit = true;
                result.cell = id;
                result.normal = normal;
                result.distanceM = enter;
                for (int axis = 0; axis < 3; ++axis)
                    result.position[axis] = origin[axis] + enter*direction[axis];
            }
            return result;
        }
    }

    Contact Raycast( Specimen const& specimen, Strike const& strike )
    {
        if (!ValidStrike(strike) || !ValidState(specimen)) return {};
        return Trace(specimen, strike.rayOrigin, Unit(strike.direction), strike.reachM);
    }

    Receipt Apply( Specimen& specimen, Strike const& strike, MaterialResponse const& material )
    {
        Receipt receipt;
        if (!ValidStrike(strike) || !std::isfinite(material.removalWorkJPerM3) ||
            material.removalWorkJPerM3 <= 0 || material.removalWorkJPerM3 > 1e15) return receipt;
        receipt.requestValid = true;
        if (!ValidState(specimen)) { receipt.outcome = Outcome::InvalidState; return receipt; }
        Point const direction = Unit(strike.direction);
        receipt.contact = Trace(specimen, strike.rayOrigin, direction, strike.reachM);
        if (!receipt.contact.hit) { receipt.outcome = Outcome::Miss; return receipt; }
        receipt.transferredJ = strike.energyJ * std::max(0.0, -Dot(direction, receipt.contact.normal));
        if (strike.implementGrade < material.requiredGrade)
        {
            receipt.outcome = Outcome::InsufficientGrade;
            receipt.dissipatedJ = receipt.transferredJ;
            return receipt;
        }
        if (receipt.transferredJ == 0) { receipt.outcome = Outcome::ZeroTransfer; return receipt; }

        // Select a shallow, circular contact footprint from the PRE-strike surface.
        // Visibility excludes hidden cells. A strike cannot spend its energy again
        // on newly exposed layers: those are contacted by subsequent strikes.
        std::vector<int> selected;
        for (int id = 0; id < int(specimen.cells.size()); ++id)
        {
            if (!specimen.cells[id].occupied) continue;
            // The original surface ray already proved visibility at the actual
            // contact. A parallel CENTER ray may be occluded by a cavity lip;
            // it must not veto the matter that was physically struck.
            if (id == receipt.contact.cell) { selected.push_back(id); continue; }
            Point center = Minimum(id), delta;
            for (int axis = 0; axis < 3; ++axis)
            { center[axis] += CellM*0.5; delta[axis] = center[axis]-receipt.contact.position[axis]; }
            double const depth = Dot(delta,direction);
            double const lateral2 = std::max(0.0, Dot(delta,delta)-depth*depth);
            if (id != receipt.contact.cell &&
                (lateral2 > strike.radiusM*strike.radiusM || depth < -CellM || depth > CellM*1.75)) continue;
            Point probe;
            for (int axis = 0; axis < 3; ++axis) probe[axis] = center[axis]-direction[axis];
            if (Trace(specimen,probe,direction,1.1).cell == id) selected.push_back(id);
        }
        if (selected.empty())
        { receipt.outcome = Outcome::ZeroTransfer; receipt.dissipatedJ = receipt.transferredJ; return receipt; }
        double const share = receipt.transferredJ / double(selected.size());
        double const threshold = material.removalWorkJPerM3 * CellVolumeM3;
        receipt.affectedCells = uint32_t(selected.size());
        for (int id : selected)
        {
            Cell& cell = specimen.cells[id];
            double const previous = cell.damageJ;
            double const accumulated = previous + share;
            if (accumulated >= threshold)
            {
                receipt.chips.push_back({uint32_t(id), Minimum(id), CellMassMg});
                cell.occupied = false;
                cell.damageJ = 0;
                receipt.fractureWorkJ += threshold;
                receipt.dissipatedJ += accumulated-threshold;
            }
            else cell.damageJ = accumulated;
            receipt.damageDeltaJ += cell.damageJ-previous;
        }
        receipt.removedVolumeM3 = double(receipt.chips.size()) * CellVolumeM3;
        receipt.removedMassMg = uint64_t(receipt.chips.size()) * CellMassMg;
        receipt.outcome = receipt.chips.empty() ? Outcome::LocalDamage : Outcome::RemovedMatter;
        return receipt;
    }

    std::vector<Triangle> BuildBoundary( Specimen const& specimen )
    {
        std::vector<Triangle> result;
        if (!ValidState(specimen)) return result;
        for (int id = 0; id < int(specimen.cells.size()); ++id)
        {
            if (!specimen.cells[id].occupied) continue;
            int const coordinates[3] = {id % Side, (id/Side) % Side, id/(Side*Side)};
            int const strides[3] = {1, Side, Side*Side};
            for (int axis = 0; axis < 3; ++axis)
            for (int sign : {-1,1})
            {
                int const neighbor = coordinates[axis]+sign;
                if (neighbor >= 0 && neighbor < Side && specimen.cells[id+sign*strides[axis]].occupied) continue;
                Point a = Minimum(id);
                if (sign > 0) a[axis] = (coordinates[axis]+1)*CellM;
                int const u = (axis+1)%3, v = (axis+2)%3;
                Point b=a,c=a,d=a;
                b[u] = c[u] = (coordinates[u]+1)*CellM;
                c[v] = d[v] = (coordinates[v]+1)*CellM;
                if (sign > 0) { result.push_back({a,b,c}); result.push_back({a,c,d}); }
                else { result.push_back({a,c,b}); result.push_back({a,d,c}); }
            }
        }
        return result;
    }
}
