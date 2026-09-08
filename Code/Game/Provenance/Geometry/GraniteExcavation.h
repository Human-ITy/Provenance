#pragma once

#include "Game/_Module/API.h"
#include <array>
#include <cstdint>
#include <vector>

namespace EE::GraniteExcavation
{
    // Additive local-excavation law; does not version or alter the bridge fixture.
    // V1: bounded coherent 32 cm cube, 1 cm cells, circular flat implement.
    // Cell-center footprint sampling is intentional; boundary uncertainty is
    // bounded by a cell diagonal. No crack propagation or support solve yet.
    // V2 (2026-09-03): preserve the directly contacted cell when its center
    // visibility ray is blocked by a cavity lip. Neighbor occlusion unchanged.
    inline constexpr uint32_t AlgorithmVersion = 2;
    inline constexpr int Side = 32;
    inline constexpr double CellM = 0.01;
    inline constexpr double CellVolumeM3 = 0.000001;
    inline constexpr double DensityKgM3 = 2700.0;
    inline constexpr uint32_t CellMassMg = 2700; // Exact sub-gram accounting.
    using Point = std::array<double, 3>;

    struct Cell
    {
        bool occupied = true;
        double damageJ = 0.0;
    };

    struct Specimen
    {
        // Heap-backed: never put a large fixed mesh/damage field on the stack.
        std::vector<Cell> cells = std::vector<Cell>( Side * Side * Side );
    };

    struct MaterialResponse
    {
        uint32_t requiredGrade = 2;
        double removalWorkJPerM3 = 1000000.0; // Certificate calibration, not measured rock data.
    };

    struct Strike
    {
        Point rayOrigin = { 0.055, 0.055, 0.6 };
        Point direction = { 0.0, 0.0, -1.0 };
        double reachM = 1.0;
        double radiusM = 0.004;
        double energyJ = 0.4;
        uint32_t implementGrade = 2;
    };

    struct Contact
    {
        bool hit = false;
        int cell = -1;
        Point position = {};
        Point normal = {};
        double distanceM = 0.0;
    };

    enum class Outcome : uint8_t
    {
        InvalidRequest, InvalidState, Miss, InsufficientGrade,
        ZeroTransfer, LocalDamage, RemovedMatter
    };

    struct Chip
    {
        // Original cell identity and cube geometry, not a trusted aggregate.
        uint32_t sourceCell = 0;
        Point minimum = {};
        uint32_t massMg = CellMassMg;
    };

    struct Receipt
    {
        bool requestValid = false;
        Contact contact;
        Outcome outcome = Outcome::InvalidRequest;
        uint32_t affectedCells = 0;
        double transferredJ = 0.0;
        double damageDeltaJ = 0.0; // May be negative when previously damaged matter leaves.
        double fractureWorkJ = 0.0;
        double dissipatedJ = 0.0;
        double removedVolumeM3 = 0.0;
        uint64_t removedMassMg = 0;
        std::vector<Chip> chips;
        // No attachment mutation in this certificate. Support integration is a later stage.
    };

    struct Triangle { Point a, b, c; };

    EE_GAME_API Contact Raycast( Specimen const&, Strike const& );
    EE_GAME_API Receipt Apply( Specimen&, Strike const&, MaterialResponse const& = {} );
    EE_GAME_API std::vector<Triangle> BuildBoundary( Specimen const& );
}
