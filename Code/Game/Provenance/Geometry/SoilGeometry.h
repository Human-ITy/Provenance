#pragma once

#include "Game/_Module/API.h"
#include "Game/Provenance/Geometry/MaterialGeometry.h"

#include <cstdint>

namespace EE
{
    //-------------------------------------------------------------------------
    // P3C.9 — Soil material authority
    //
    // A: matter authority
    // B: conserved continuous-mantle redistribution over substrate geometry
    // C: conserved breach / residual patch survival
    // D: generic substrate certification
    //
    // Governing law:
    //
    //     Soil mass
    //         / intrinsic particle density
    //     = Soil solid volume
    //
    //     Soil solid volume
    //         / packing fraction
    //     = Soil bulk mantle volume
    //
    // P3C.9C may reduce occupied AREA, but it may not reduce authoritative
    // Soil quantity. Soil evacuated from a failed surface cell must be
    // redistributed into surviving cells unless an explicit export/removal
    // process exists. P3C.9C contains no such export process.
    //
    // Therefore:
    //
    //     initial Soil bulk volume
    //         =
    //     final surviving-patch bulk volume
    //
    // and:
    //
    //     initial Soil mass
    //         =
    //     reconstructed final Soil mass
    //
    // Coverage fraction and breach fraction are DERIVED RECEIPTS only.
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // Physical state
    //-------------------------------------------------------------------------

    enum class SoilPhysicalState : uint8_t
    {
        Loose = 0,
        Settled,
        Compacted
    };

    //-------------------------------------------------------------------------
    // Distribution state
    //-------------------------------------------------------------------------

    enum class SoilDistributionState : uint8_t
    {
        ContinuousMantle = 0,
        ResidualThinCover,
        PocketedInfill
    };

    //-------------------------------------------------------------------------
    // Evaluation domain
    //-------------------------------------------------------------------------

    struct SoilMantleDomain
    {
        float m_minX = 0.0f;
        float m_maxX = 1.0f;

        float m_minY = 0.0f;
        float m_maxY = 1.0f;
    };

    //-------------------------------------------------------------------------
    // Authoritative Soil body
    //-------------------------------------------------------------------------

    struct SoilMantleBody
    {
        uint32_t m_bodyID = 0;
        uint32_t m_provenanceID = 0;
        uint32_t m_seed = 0;

        uint64_t m_massGrams = 0;

        SoilPhysicalState m_physicalState =
            SoilPhysicalState::Settled;

        SoilDistributionState m_distributionState =
            SoilDistributionState::ContinuousMantle;
    };

    //-------------------------------------------------------------------------
    // P3C.9A uniform sample
    //-------------------------------------------------------------------------

    struct SoilMantleSample
    {
        bool m_occupied = false;

        float m_worldX = 0.0f;
        float m_worldY = 0.0f;

        float m_thicknessZM = 0.0f;
    };

    //-------------------------------------------------------------------------
    // P3C.9A authority / conservation receipt
    //-------------------------------------------------------------------------

    struct SoilMantleAuthorityResult
    {
        SoilMantleBody   m_body;
        SoilMantleDomain m_domain;

        float m_massKg = 0.0f;
        float m_particleDensityKgPerM3 = 0.0f;
        float m_solidVolumeM3 = 0.0f;

        float m_packingFraction = 1.0f;
        float m_bulkVolumeM3 = 0.0f;

        float m_footprintAreaM2 = 0.0f;
        float m_meanThicknessZM = 0.0f;

        float m_integratedBulkVolumeM3 = 0.0f;
        float m_reconstructedMassKg = 0.0f;

        float m_bulkVolumeResidualM3 = 0.0f;
        float m_massResidualKg = 0.0f;

        float m_bulkVolumeRelativeResidual = 0.0f;
        float m_massRelativeResidual = 0.0f;

        bool m_conservationPass = false;
    };

    //-------------------------------------------------------------------------
    // P3C.9B/C grid carrier
    //-------------------------------------------------------------------------

    struct SoilMantleDistributionGrid
    {
        float m_minWorldX = 0.0f;
        float m_minWorldY = 0.0f;

        int32_t m_numCellsX = 0;
        int32_t m_numCellsY = 0;

        float m_cellSizeX = 1.0f;
        float m_cellSizeY = 1.0f;
    };

    //-------------------------------------------------------------------------
    // Generic substrate / retention sample
    //
    // SoilGeometry owns no Granite-specific dependency here.
    //-------------------------------------------------------------------------

    struct SoilRetentionSample
    {
        ProvenanceMaterialID m_substrateMaterial =
            ProvenanceMaterialID::Granite;

        float m_worldX = 0.0f;
        float m_worldY = 0.0f;
        float m_substrateSurfaceZM = 0.0f;

        float m_slope = 0.0f;
        float m_convexHighInfluence = 0.0f;
        float m_concavityInfluence = 0.0f;

        // Physical recess accommodation in metres.
        float m_recessDepthM = 0.0f;

        float m_obstacleBankingInfluence = 0.0f;
        float m_protrusionInfluence = 0.0f;
    };

    //-------------------------------------------------------------------------
    // P3C.9B per-cell redistributed continuous Soil
    //-------------------------------------------------------------------------

    struct SoilRedistributionCell
    {
        bool m_occupied = false;

        float m_worldX = 0.0f;
        float m_worldY = 0.0f;

        float m_retentionWeight = 1.0f;
        float m_thicknessZM = 0.0f;

        float m_projectedAreaM2 = 0.0f;
        float m_bulkVolumeM3 = 0.0f;

        ProvenanceMaterialID m_substrateMaterial =
            ProvenanceMaterialID::Granite;
    };

    //-------------------------------------------------------------------------
    // P3C.9B receipt
    //-------------------------------------------------------------------------

    struct SoilMantleRedistributionResult
    {
        bool m_valid = false;
        bool m_conservationPass = false;
        bool m_continuousCoverPass = false;

        int32_t m_numCells = 0;

        float m_cellAreaM2 = 0.0f;
        float m_gridAreaM2 = 0.0f;

        float m_authoritativeBulkVolumeM3 = 0.0f;
        float m_integratedBulkVolumeM3 = 0.0f;
        float m_bulkVolumeResidualM3 = 0.0f;
        float m_bulkVolumeRelativeResidual = 0.0f;

        float m_authoritativeMassKg = 0.0f;
        float m_reconstructedMassKg = 0.0f;
        float m_massResidualKg = 0.0f;
        float m_massRelativeResidual = 0.0f;

        float m_uniformMeanThicknessZM = 0.0f;
        float m_redistributedMeanThicknessZM = 0.0f;
        float m_minThicknessZM = 0.0f;
        float m_maxThicknessZM = 0.0f;

        float m_meanRawRetentionWeight = 0.0f;
        float m_normalizationScale = 1.0f;

        int32_t m_numConvexHighCells = 0;
        int32_t m_numRetentionLowCells = 0;

        float m_meanConvexHighThicknessZM = 0.0f;
        float m_meanRetentionLowThicknessZM = 0.0f;
    };

    //-------------------------------------------------------------------------
    // P3C.9C — local cover-stability state
    //-------------------------------------------------------------------------

    enum class SoilCoverCellState : uint8_t
    {
        // Soil remains part of the surviving residual mantle.
        OccupiedResidual = 0,

        // Soil was evacuated because local cover could not remain stable.
        BreachedToSubstrate,

        // Occupied cell strongly favored by recess / concavity / banking.
        RetainedPocket
    };

    //-------------------------------------------------------------------------
    // P3C.9C physical breach policy
    //
    // These are stability parameters, not requested visual coverage values.
    //
    // The solver never asks for "42% coverage". It asks whether a local Soil
    // thickness is physically stable under the supplied state/geometry, then
    // derives final occupied area from those results.
    //-------------------------------------------------------------------------

    struct SoilBreachPolicy
    {
        // Base minimum stable world-Z thickness before local geometry/state
        // modifies it.
        float m_looseMinimumStableThicknessM = 0.030f;
        float m_settledMinimumStableThicknessM = 0.020f;
        float m_compactedMinimumStableThicknessM = 0.012f;

        // Convex highs, slope, and protrusions raise the local minimum stable
        // thickness. Retention geometry lowers it.
        float m_slopeInstabilityM = 0.020f;
        float m_convexInstabilityM = 0.030f;
        float m_protrusionInstabilityM = 0.026f;

        float m_concavityStabilityM = 0.018f;
        float m_recessStabilityPerMeter = 0.090f;
        float m_bankingStabilityM = 0.018f;

        // Clamp prevents a retention feature from declaring an arbitrarily
        // thin film stable. Trace ribbons/pockets remain finite matter.
        float m_minimumRetainedPocketThicknessM = 0.004f;

        // Upper bound on evacuation iterations. This is a safety limit only;
        // it does not prescribe breach fraction.
        int32_t m_maxIterations = 16;
    };

    //-------------------------------------------------------------------------
    // P3C.9C per-cell patch result
    //-------------------------------------------------------------------------

    struct SoilPatchCell
    {
        SoilCoverCellState m_state =
            SoilCoverCellState::OccupiedResidual;

        bool m_occupied = false;

        float m_worldX = 0.0f;
        float m_worldY = 0.0f;

        float m_projectedAreaM2 = 0.0f;

        // Final conserved Soil after evacuation/redistribution.
        float m_thicknessZM = 0.0f;
        float m_bulkVolumeM3 = 0.0f;

        // Geometry/state-derived local stability requirement.
        float m_minimumStableThicknessM = 0.0f;

        // Positive = locally above stability requirement.
        // Negative = unstable before evacuation.
        float m_stabilityMarginM = 0.0f;

        // Reused from the continuous-retention solve; remains a preference,
        // never a quantity authority.
        float m_retentionWeight = 1.0f;

        ProvenanceMaterialID m_substrateMaterial =
            ProvenanceMaterialID::Granite;

        // 0 for breached cells. Occupied connected components are assigned
        // deterministic positive IDs after the solve.
        int32_t m_patchID = 0;
    };

    //-------------------------------------------------------------------------
    // P3C.9C final receipt
    //-------------------------------------------------------------------------

    struct SoilMantleBreachResult
    {
        bool m_valid = false;
        bool m_conservationPass = false;
        bool m_patchTopologyPass = false;

        int32_t m_numCells = 0;
        int32_t m_numOccupiedCells = 0;
        int32_t m_numBreachedCells = 0;
        int32_t m_numPocketCells = 0;
        int32_t m_numPatches = 0;

        float m_cellAreaM2 = 0.0f;
        float m_totalAreaM2 = 0.0f;
        float m_occupiedAreaM2 = 0.0f;
        float m_breachedAreaM2 = 0.0f;

        // DERIVED spatial receipts.
        float m_coverageFraction = 0.0f;
        float m_breachFraction = 0.0f;

        float m_authoritativeBulkVolumeM3 = 0.0f;
        float m_finalBulkVolumeM3 = 0.0f;
        float m_bulkVolumeResidualM3 = 0.0f;
        float m_bulkVolumeRelativeResidual = 0.0f;

        float m_authoritativeMassKg = 0.0f;
        float m_reconstructedMassKg = 0.0f;
        float m_massResidualKg = 0.0f;
        float m_massRelativeResidual = 0.0f;

        // Soil volume evacuated from cells that ultimately breached.
        // This volume must reappear in surviving occupied cells.
        float m_evacuatedBulkVolumeM3 = 0.0f;

        float m_minOccupiedThicknessZM = 0.0f;
        float m_maxOccupiedThicknessZM = 0.0f;
        float m_meanOccupiedThicknessZM = 0.0f;

        float m_meanBreachedPreEvacuationThicknessZM = 0.0f;
        float m_meanPocketThicknessZM = 0.0f;

        int32_t m_iterationsUsed = 0;
    };

    //-------------------------------------------------------------------------
    // P3C.9C-4 — Visible Conserved Soil Geometry
    //
    // P3C.9A-C proved quantity, redistribution, and occupancy topology.
    // P3C.9C-4 turns that certified matter into an explicit visible Soil
    // geometry field without allowing presentation to become quantity
    // authority.
    //
    // Accounting hierarchy:
    //
    //     2 m SoilPatchCell
    //         = conserved parent bulk-volume owner
    //
    //     finer SoilGeometrySubcell
    //         = deterministic spatial allocation of that parent volume
    //
    //     visible contour/mesh
    //         = presentation of explicit occupied sub-regions
    //
    // The required conservation law is:
    //
    //     sum( subcell bulk volume )
    //         =
    //     parent SoilPatchCell bulk volume
    //
    // for every occupied parent cell independently.
    //-------------------------------------------------------------------------

    enum class SoilVisibleGeometryState : uint8_t
    {
        // Broad continuous or residual drape.
        Mantle = 0,

        // Thin surviving patch with explicit exposed-substrate neighbors.
        ResidualPatch,

        // Strong retention geometry such as a joint, recess, or protected base.
        PocketedInfill,

        // Real volume-backed aggregate removed from the local mantle budget.
        Clod
    };

    //-------------------------------------------------------------------------
    // Fine geometry grid
    //
    // This describes one deterministic subcell lattice over the accounting
    // domain. For the current 2 m → 0.25 m certificate:
    //
    //     8 x 8 fine cells per parent accounting cell.
    //
    // The structure does not require those exact values.
    //-------------------------------------------------------------------------

    struct SoilGeometryGrid
    {
        float m_minWorldX = 0.0f;
        float m_minWorldY = 0.0f;

        int32_t m_numCellsX = 0;
        int32_t m_numCellsY = 0;

        float m_cellSizeX = 0.25f;
        float m_cellSizeY = 0.25f;

        // Number of fine cells spanning one parent accounting cell.
        int32_t m_subcellsPerParentX = 1;
        int32_t m_subcellsPerParentY = 1;
    };

    //-------------------------------------------------------------------------
    // State-specific visible-geometry policy
    //
    // These parameters govern permitted shape response, not conserved
    // quantity. The solver must still integrate exactly to the parent volume.
    //-------------------------------------------------------------------------

    struct SoilVisibleGeometryPolicy
    {
        // Maximum redistribution contrast inside one occupied parent cell.
        float m_looseInternalMobility = 0.90f;
        float m_settledInternalMobility = 0.64f;
        float m_compactedInternalMobility = 0.34f;

        // Stable visible slope tendencies. These are dimensionless dz/dxy
        // limits for this height-surface prototype, not repose angles stored
        // as render normals.
        float m_looseMaximumSurfaceSlope = 0.70f;
        float m_settledMaximumSurfaceSlope = 0.92f;
        float m_compactedMaximumSurfaceSlope = 1.20f;

        // Thin residual edges taper toward the substrate rather than ending
        // as vertical curtains.
        float m_residualEdgeTaperWidthM = 0.30f;

        // Strong pocket geometry may keep a locally thicker center while
        // remaining continuous with its retained footprint.
        float m_pocketCenterBias = 0.34f;

        // Optional real clod budget. The percentage is taken from a parent
        // cell's own Soil bulk volume before mantle allocation.
        //
        // P3C.9C-4A/B may leave this at zero. P3C.9C-4C can enable it.
        float m_maxClodBulkVolumeFraction = 0.0f;

        // Numerical guard: an occupied fine cell must own finite thickness.
        float m_minimumVisibleThicknessM = 0.001f;
    };

    //-------------------------------------------------------------------------
    // Per-fine-cell explicit Soil geometry
    //-------------------------------------------------------------------------

    struct SoilGeometrySubcell
    {
        bool m_occupied = false;

        SoilVisibleGeometryState m_geometryState =
            SoilVisibleGeometryState::Mantle;

        float m_worldX = 0.0f;
        float m_worldY = 0.0f;

        float m_projectedAreaM2 = 0.0f;

        // Index of the conserved parent SoilPatchCell.
        int32_t m_parentCellIndex = -1;

        // Must preserve the parent P3C.9C patch identity.
        int32_t m_parentPatchID = 0;

        // Generic substrate identity/material.
        ProvenanceMaterialID m_substrateMaterial =
            ProvenanceMaterialID::Granite;

        float m_substrateSurfaceZM = 0.0f;

        // Relative local allocation priority derived from retention geometry.
        // This is never matter quantity on its own.
        float m_allocationWeight = 0.0f;

        // Exact Soil matter assigned to this visible fine cell.
        float m_bulkVolumeM3 = 0.0f;
        float m_thicknessZM = 0.0f;

        // Explicit visible top surface.
        float m_soilSurfaceZM = 0.0f;

        // 0..1 geometric taper near a residual/pocket edge.
        // Taper modifies distribution only through the volume-conserving solve.
        float m_edgeTaper = 1.0f;

        // Used by contour extraction. Positive means occupied Soil geometry;
        // zero/negative means explicit substrate region.
        float m_contactField = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Optional real clod descriptor
    //
    // A clod is not a free decorative bump. Its bulk volume is reserved from
    // its parent Soil patch before mantle allocation and must be included in
    // the same parent conservation receipt.
    //-------------------------------------------------------------------------

    struct SoilClodDescriptor
    {
        bool m_valid = false;

        uint32_t m_eventID = 0;

        int32_t m_parentCellIndex = -1;
        int32_t m_parentPatchID = 0;

        float m_centerWorldX = 0.0f;
        float m_centerWorldY = 0.0f;
        float m_baseWorldZ = 0.0f;

        float m_bulkVolumeM3 = 0.0f;

        float m_radiusM = 0.0f;
        float m_heightM = 0.0f;

        SoilPhysicalState m_physicalState =
            SoilPhysicalState::Settled;
    };

    //-------------------------------------------------------------------------
    // Parent-cell fine-geometry conservation receipt
    //-------------------------------------------------------------------------

    struct SoilParentGeometryReceipt
    {
        bool m_valid = false;
        bool m_conservationPass = false;
        bool m_topologyPass = false;

        int32_t m_parentCellIndex = -1;
        int32_t m_parentPatchID = 0;

        int32_t m_numSubcells = 0;
        int32_t m_numOccupiedSubcells = 0;

        float m_parentBulkVolumeM3 = 0.0f;

        float m_allocatedMantleBulkVolumeM3 = 0.0f;
        float m_allocatedClodBulkVolumeM3 = 0.0f;
        float m_totalAllocatedBulkVolumeM3 = 0.0f;

        float m_bulkVolumeResidualM3 = 0.0f;

        float m_meanOccupiedThicknessZM = 0.0f;
        float m_minOccupiedThicknessZM = 0.0f;
        float m_maxOccupiedThicknessZM = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Whole visible-geometry result
    //-------------------------------------------------------------------------

    struct SoilVisibleGeometryResult
    {
        bool m_valid = false;
        bool m_conservationPass = false;
        bool m_parentTopologyPass = false;

        int32_t m_numParentCells = 0;
        int32_t m_numOccupiedParentCells = 0;

        int32_t m_numFineCells = 0;
        int32_t m_numOccupiedFineCells = 0;

        int32_t m_numClods = 0;

        float m_authoritativeBulkVolumeM3 = 0.0f;
        float m_allocatedMantleBulkVolumeM3 = 0.0f;
        float m_allocatedClodBulkVolumeM3 = 0.0f;
        float m_totalAllocatedBulkVolumeM3 = 0.0f;
        float m_bulkVolumeResidualM3 = 0.0f;
        float m_bulkVolumeRelativeResidual = 0.0f;

        // Derived visual/topological receipts only.
        float m_fineCoverageFraction = 0.0f;

        int32_t m_numResidualPatchSubcells = 0;
        int32_t m_numPocketSubcells = 0;

        float m_meanResidualPatchThicknessZM = 0.0f;
        float m_meanPocketThicknessZM = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Explicit Soil/substrate contact segment
    //
    // These segments are derived from the signed contact field on the fine
    // Soil geometry lattice. They describe real material partition boundaries,
    // not blend weights.
    //-------------------------------------------------------------------------

    struct SoilContactSegment
    {
        bool m_valid = false;

        int32_t m_patchID = 0;

        float m_startWorldX = 0.0f;
        float m_startWorldY = 0.0f;
        float m_startWorldZ = 0.0f;

        float m_endWorldX = 0.0f;
        float m_endWorldY = 0.0f;
        float m_endWorldZ = 0.0f;
    };

    //-------------------------------------------------------------------------
    // P3C.9C-4F — Soil State Geometry Grammar
    //
    // P3C.9A-C certified Soil quantity and distribution.
    // P3C.9C-4A-E certified visible mantle/contact geometry.
    //
    // This pass gives hand-scale / body-scale Soil its own material-owned
    // geometry grammar so B4/B5/B6 stop being generic debug primitives.
    //
    // Governing rule:
    //
    //     same Soil material
    //     + same mass authority
    //     + different body/physical state
    //     =
    //     different permitted geometry
    //
    // This is the small-scale reference language that later terrain clods,
    // piles, disturbed Soil, and compacted deposits may reuse.
    //
    // IMPORTANT:
    //
    // The generator consumes the existing MaterialGeometry matter law:
    //
    //     mass / particle density = solid volume
    //     solid volume / state packing = bulk envelope volume
    //
    // It does NOT introduce a second Soil density or packing authority.
    //-------------------------------------------------------------------------

    enum class SoilBodyGeometryKind : uint8_t
    {
        CohesiveClod = 0,
        LoosePile,
        CompactedMass
    };

    //-------------------------------------------------------------------------
    // Geological / physical surface regions on a generated Soil body
    //-------------------------------------------------------------------------

    enum class SoilBodySurfaceRegion : uint8_t
    {
        Crown = 0,
        Shoulder,
        Side,
        BasalContact,
        AggregateLobe
    };

    //-------------------------------------------------------------------------
    // Local Soil body point
    //-------------------------------------------------------------------------

    struct SoilPoint
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;
    };

    //-------------------------------------------------------------------------
    // State-owned shape profile
    //
    // These values describe geometry tendencies only. They do not own mass,
    // density, packing, or bulk volume.
    //-------------------------------------------------------------------------

    struct SoilBodyGeometryProfile
    {
        // Horizontal footprint shape.
        float m_radialIrregularity = 0.0f;
        float m_elongation = 1.0f;
        float m_asymmetry = 0.0f;

        // Vertical shape.
        float m_crownBias = 0.0f;
        float m_crownOffset = 0.0f;
        float m_shoulderSoftness = 0.0f;
        float m_contactFlattening = 0.0f;

        // Surface / aggregate expression.
        float m_aggregateLobeBias = 0.0f;
        float m_edgeCrumble = 0.0f;
        float m_surfaceUndulation = 0.0f;

        // Maximum intended visible dz/dxy tendency for the body-scale mesh.
        // This is a geometry constraint, not a packing or quantity value.
        float m_maxStableSurfaceSlope = 1.0f;
    };

    //-------------------------------------------------------------------------
    // Generated Soil body request
    //
    // bodyState is the existing MaterialGeometry packing authority:
    //
    //     B4 Soil Clod       -> Fragment
    //     B5 Soil Loose      -> LooseAggregate
    //     B6 Soil Compacted  -> Compacted
    //
    // kind selects only the Soil geometry grammar.
    //-------------------------------------------------------------------------

    struct SoilBodyGeometryRequest
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_bodyID = 0;
        uint32_t m_provenanceID = 0;
        uint32_t m_geometrySalt = 0;

        uint32_t m_massGrams = 0;

        ProvenanceBodyState m_bodyState =
            ProvenanceBodyState::Fragment;

        SoilBodyGeometryKind m_kind =
            SoilBodyGeometryKind::CohesiveClod;

        float m_worldX = 0.0f;
        float m_worldY = 0.0f;
        float m_worldZ = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Generated Soil body geometry
    //
    // Fixed-capacity deterministic mesh. No heap allocation is required.
    //
    // The mesh volume represents BULK/envelope volume, because visible Soil
    // contains both solid particles and pore space according to packing.
    // Solid volume remains separately certified from mass/density.
    //-------------------------------------------------------------------------

    struct SoilBodyGeometry
    {
        static constexpr int32_t s_maxVertices = 64;
        static constexpr int32_t s_maxTriangles = 128;
        static constexpr int32_t s_maxTriangleIndices =
            s_maxTriangles * 3;

        SoilPoint m_vertices[s_maxVertices];
        int32_t   m_numVertices = 0;

        uint8_t m_triangleIndices[s_maxTriangleIndices] = { 0 };
        int32_t m_numTriangles = 0;

        SoilBodySurfaceRegion m_triangleSurfaceRegion[s_maxTriangles];

        // Matter authority / conservation receipts.
        float m_massKg = 0.0f;
        float m_particleDensityKgPerM3 = 0.0f;
        float m_packingFraction = 1.0f;

        float m_requiredSolidVolumeM3 = 0.0f;
        float m_requiredBulkVolumeM3 = 0.0f;

        float m_measuredMeshVolumeM3 = 0.0f;
        float m_bulkVolumeResidualM3 = 0.0f;
        float m_bulkVolumeRelativeResidual = 0.0f;

        bool m_conservationPass = false;

        // Body-scale geometry receipts.
        float m_extentXM = 0.0f;
        float m_extentYM = 0.0f;
        float m_extentZM = 0.0f;

        float m_contactAreaM2 = 0.0f;
        float m_meanCrownHeightM = 0.0f;
        float m_maxCrownHeightM = 0.0f;

        float m_effectiveSurfaceSlope = 0.0f;

        int32_t m_crownTriangleCount = 0;
        int32_t m_shoulderTriangleCount = 0;
        int32_t m_sideTriangleCount = 0;
        int32_t m_contactTriangleCount = 0;
        int32_t m_aggregateLobeCount = 0;

        SoilBodyGeometryKind m_kind =
            SoilBodyGeometryKind::CohesiveClod;
    };

    //-------------------------------------------------------------------------
    // P3C.9A API
    //-------------------------------------------------------------------------

    EE_GAME_API float GetSoilPhysicalStatePackingFraction(
        SoilPhysicalState state );

    EE_GAME_API float GetSoilPhysicalStateRedistributionMobility(
        SoilPhysicalState state );

    EE_GAME_API float CalculateSoilMantleFootprintArea(
        SoilMantleDomain const& domain );

    EE_GAME_API SoilMantleAuthorityResult CalculateSoilMantleAuthority(
        SoilMantleBody const&   body,
        SoilMantleDomain const& domain );

    EE_GAME_API SoilMantleSample EvaluateSoilMantleAuthoritySample(
        SoilMantleAuthorityResult const& authority,
        float                            worldX,
        float                            worldY );

    EE_GAME_API float ReconstructSoilBulkVolumeFromUniformThickness(
        SoilMantleDomain const& domain,
        float                   thicknessZM );

    EE_GAME_API float ReconstructSoilMassKg(
        float bulkVolumeM3,
        float packingFraction,
        float particleDensityKgPerM3 );

    //-------------------------------------------------------------------------
    // P3C.9B API
    //-------------------------------------------------------------------------

    EE_GAME_API float CalculateSoilRetentionWeight(
        SoilRetentionSample const& sample,
        SoilPhysicalState          physicalState );

    EE_GAME_API SoilMantleRedistributionResult RedistributeContinuousSoilMantle(
        SoilMantleAuthorityResult const&  authority,
        SoilMantleDistributionGrid const& grid,
        SoilRetentionSample const*        pRetentionSamples,
        int32_t                           numRetentionSamples,
        SoilRedistributionCell*           pOutCells,
        int32_t                           numOutputCells );

    //-------------------------------------------------------------------------
    // P3C.9C API
    //-------------------------------------------------------------------------

    // Computes the local minimum stable cover thickness from physical Soil
    // state + generic substrate geometry.
    EE_GAME_API float CalculateSoilMinimumStableThickness(
        SoilRetentionSample const& sample,
        SoilPhysicalState          physicalState,
        SoilBreachPolicy const&    policy );

    // Conserved breach / residual patch solve.
    //
    // Input:
    //     authoritative matter result from P3C.9A
    //     continuous redistributed field from P3C.9B
    //     the same generic retention samples used by P3C.9B
    //
    // Output:
    //     occupied residual cells
    //     breached-to-substrate cells
    //     retention-pocket cells
    //     deterministic connected patch IDs
    //
    // The solver may reduce occupied area, but must conserve total Soil bulk
    // volume and reconstructed mass exactly within the declared tolerance.
    EE_GAME_API SoilMantleBreachResult ResolveConservedSoilBreach(
        SoilMantleAuthorityResult const&  authority,
        SoilMantleDistributionGrid const& grid,
        SoilRetentionSample const*        pRetentionSamples,
        int32_t                           numRetentionSamples,
        SoilRedistributionCell const*     pRedistributedCells,
        int32_t                           numRedistributedCells,
        SoilBreachPolicy const&           policy,
        SoilPatchCell*                    pOutPatchCells,
        int32_t                           numOutputCells );

    //-------------------------------------------------------------------------
    // P3C.9C-4 API — visible conserved Soil geometry
    //-------------------------------------------------------------------------

    // Fine-cell allocation priority. Generic substrate geometry only.
    EE_GAME_API float CalculateSoilVisibleAllocationWeight(
        SoilRetentionSample const&       sample,
        SoilPhysicalState                physicalState,
        SoilCoverCellState               parentCoverState,
        SoilVisibleGeometryPolicy const& policy );

    // Converts certified parent SoilPatchCells into a finer explicit Soil
    // geometry field while conserving every occupied parent's bulk volume.
    //
    // pFineRetentionSamples and pOutSubcells both contain exactly:
    //
    //     geometryGrid.m_numCellsX * geometryGrid.m_numCellsY
    //
    // fine cells.
    //
    // pParentReceipts contains exactly numParentPatchCells entries so each
    // accounting cell receives an independent conservation certificate.
    //
    // pOutClods may be null when policy.m_maxClodBulkVolumeFraction == 0.
    EE_GAME_API SoilVisibleGeometryResult BuildVisibleConservedSoilGeometry(
        SoilMantleAuthorityResult const&  authority,
        SoilMantleDistributionGrid const& parentGrid,
        SoilPatchCell const*              pParentPatchCells,
        int32_t                           numParentPatchCells,
        SoilGeometryGrid const&           geometryGrid,
        SoilRetentionSample const*        pFineRetentionSamples,
        int32_t                           numFineRetentionSamples,
        SoilVisibleGeometryPolicy const&  policy,
        SoilGeometrySubcell*              pOutSubcells,
        int32_t                           numOutputSubcells,
        SoilParentGeometryReceipt*        pParentReceipts,
        int32_t                           numParentReceipts,
        SoilClodDescriptor*               pOutClods,
        int32_t                           maxOutputClods );

    // Extracts explicit Soil/substrate contact segments from the fine geometry
    // field. The contact is a spatial partition boundary; it does not create
    // or destroy Soil quantity.
    EE_GAME_API int32_t ExtractSoilContactSegments(
        SoilGeometryGrid const&    geometryGrid,
        SoilGeometrySubcell const* pSubcells,
        int32_t                    numSubcells,
        SoilContactSegment*        pOutSegments,
        int32_t                    maxOutputSegments );

    //-------------------------------------------------------------------------
    // P3C.9C-4F API — Soil state geometry grammar
    //-------------------------------------------------------------------------

    EE_GAME_API SoilBodyGeometryProfile GetSoilBodyGeometryProfile(
        SoilBodyGeometryKind kind );

    // Generates a deterministic, volume-backed Soil body mesh.
    //
    // The generator must preserve:
    //
    //     mass
    //     solid volume
    //     packing-derived bulk volume
    //
    // while giving CohesiveClod / LoosePile / CompactedMass visibly different
    // geometry.
    EE_GAME_API SoilBodyGeometry GenerateSoilBodyGeometry(
        SoilBodyGeometryRequest const& request );
}
