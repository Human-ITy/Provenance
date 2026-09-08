#pragma once

#include "Game/_Module/API.h"
#include "Game/Provenance/Geometry/MaterialGeometry.h"

namespace EE
{
    // Deterministic Granite generation identity. Any intentional change to
    // authoritative descriptor, geometry, fracture, separation, or spall
    // output must increment this value and publish a migrated fingerprint
    // corpus. Presentation-only changes do not affect it.
    static constexpr uint32_t s_graniteGeometryAlgorithmVersion = 1u;

    //-------------------------------------------------------------------------
    // Granite geometry scale
    //-------------------------------------------------------------------------

    enum class GraniteGeometryScale : uint8_t
    {
        Fragment = 0,
        Block,
        Outcrop
    };

    //-------------------------------------------------------------------------
    // Broad weathering condition
    //-------------------------------------------------------------------------

    enum class GraniteWeatheringState : uint8_t
    {
        FreshFracture = 0,
        WeatheredExposure
    };

    //-------------------------------------------------------------------------
    // Structural hierarchy
    //-------------------------------------------------------------------------

    enum class GraniteJointTier : uint8_t
    {
        Primary = 0,
        Secondary
    };

    //-------------------------------------------------------------------------
    // Visible surface history
    //-------------------------------------------------------------------------

    enum class GraniteSurfaceFaceClass : uint8_t
    {
        FreshFracture = 0,
        WeatheredExterior,
        Transitional
    };

    //-------------------------------------------------------------------------
    // Geometric origin of a generated face
    //-------------------------------------------------------------------------

    enum class GraniteFaceOrigin : uint8_t
    {
        Unknown = 0,
        SeedEnvelope,
        PrimaryJoint,
        SecondaryJoint,
        FreshBreak,
        InheritedExterior,

        // Presentation/physical edge-history bevel generated from the
        // relationship between two already-existing geological faces.
        // This is NOT a new fracture event.
        SurfaceTransition
    };

    //-------------------------------------------------------------------------
    // Edge-history treatment
    //
    // A flat face is valid Granite geometry. Step 6C operates on the seam
    // BETWEEN faces so we can preserve planar fracture surfaces while making
    // selected transitions read as chipped, worn, or weather-rounded rather
    // than mathematically razor sharp.
    //-------------------------------------------------------------------------

    enum class GraniteEdgeHistoryClass : uint8_t
    {
        None = 0,
        FreshSharp,
        StructuralCrisp,
        MixedHistory,
        WeatheredWear,
        SupportWear
    };

    //-------------------------------------------------------------------------
    // Boulder-scale Granite structural state
    //
    // These values describe the geological/structural meaning of a visible
    // surface-stone form. They are deliberately NOT render-prop categories.
    //
    // AttachedRockHead
    //     Still continuous bedrock. Positive relief is part of the parent
    //     Granite surface and must not become a separate MatterBody.
    //
    // PartiallyDetachedBlock
    //     Still connected to bedrock, but deterministic joints isolate part
    //     of its perimeter. Remaining structural connection is explicit.
    //
    // DetachedSurfaceStoneCandidate
    //     No longer contributes to parent-bedrock elevation. This is a
    //     deterministic descriptor for later MatterBody/world-state creation.
    //-------------------------------------------------------------------------

    enum class GraniteSurfaceStoneState : uint8_t
    {
        AttachedRockHead = 0,
        PartiallyDetachedBlock,
        DetachedSurfaceStoneCandidate
    };

    //-------------------------------------------------------------------------
    // Continuous formation-scale Granite forms
    //
    // These are NOT scaled-up B1/B3 meshes. They are deterministic structures
    // generated directly in absolute world space from the same Granite grammar:
    // weathered exterior, fracture ancestry, asymmetry and joint structure.
    //-------------------------------------------------------------------------

    enum class GraniteFormationFormType : uint8_t
    {
        BroadWeatheredBack = 0,
        RootedRockHead,
        SlabShoulder
    };

    //-------------------------------------------------------------------------
    // Attachment meaning for continuous formation forms
    //
    // CoherentBedrock
    //     Broad backs and slab/shoulder forms remain ordinary continuous
    //     formation-scale bedrock.
    //
    // RootedRockHead
    //     Boulder-like silhouette, but still continuously rooted into the
    //     formation. Boulder-like shape does NOT imply detached body state.
    //-------------------------------------------------------------------------

    enum class GraniteFormationAttachmentState : uint8_t
    {
        CoherentBedrock = 0,
        RootedRockHead
    };

    //-------------------------------------------------------------------------
    // Persistent Granite joint set
    //-------------------------------------------------------------------------

    struct GraniteJointSet
    {
        GraniteJointTier m_tier = GraniteJointTier::Primary;

        float m_orientationRadians = 0.0f;
        float m_spacingMeanM = 1.0f;
        float m_spacingVariation = 0.0f;
        float m_persistence = 1.0f;
        float m_apertureM = 0.0f;
        float m_recessDepthM = 0.05f;
        float m_expressionThreshold = 0.35f;
    };

    //-------------------------------------------------------------------------
    // Surface-history authority
    //-------------------------------------------------------------------------

    struct GraniteSurfaceHistoryProfile
    {
        float m_inheritedExteriorBias = 0.42f;

        float m_weatheredRoundingStrength = 0.38f;
        float m_weatheredEdgeSoftening = 0.34f;
        float m_weatheredPlanarityRetention = 0.55f;

        float m_freshBreakSharpness = 0.95f;
        float m_freshBreakPlanarity = 0.92f;
        float m_freshBreakEdgeWear = 0.04f;

        float m_historyTransitionWidth = 0.12f;
    };

    //-------------------------------------------------------------------------
    // Granite material-owned geometry authority
    //-------------------------------------------------------------------------

    struct GraniteGeometryProfile
    {
        float m_angularity = 0.95f;
        float m_facePlanarity = 0.90f;
        float m_edgeWear = 0.08f;
        float m_slabBias = 0.35f;
        float m_elongationBias = 0.25f;

        float m_jointSpacingMeanM = 5.40f;
        float m_jointSpacingVariation = 0.46f;
        float m_jointPersistence = 0.78f;
        float m_jointApertureM = 0.045f;
        float m_jointHierarchyContrast = 0.72f;
        float m_coherentMassBias = 0.72f;

        float m_exfoliationStrength = 0.40f;
        float m_domeRounding = 0.28f;
        float m_surfaceRoughness = 0.18f;

        float m_smallFeatureBurialDepthM = 0.10f;
        float m_mediumFeatureBurialDepthM = 0.30f;
        float m_largeFeatureInfluenceDepthM = 0.75f;

        GraniteSurfaceHistoryProfile m_surfaceHistory;
        GraniteJointSet              m_jointSets[3];
    };

    //-------------------------------------------------------------------------
    // Granite structural-field sample
    //-------------------------------------------------------------------------

    struct GraniteJointFieldSample
    {
        float m_jointDistanceM[3] = { 0.0f, 0.0f, 0.0f };
        float m_jointStrength[3] = { 0.0f, 0.0f, 0.0f };

        float m_primaryJointInfluence = 0.0f;
        float m_secondaryJointInfluence = 0.0f;
        float m_combinedJointInfluence = 0.0f;

        float m_largeScaleReliefM = 0.0f;
        float m_mediumScaleReliefM = 0.0f;
        float m_smallScaleReliefM = 0.0f;

        float m_primaryJointRecessM = 0.0f;
        float m_secondaryJointRecessM = 0.0f;

        float m_outcropReliefM = 0.0f;
        float m_weatheringReliefM = 0.0f;

        // P3C.8 Step 7C-1B boulder-scale receipts. These are decomposed so
        // WorldSystem can certify that visible large-form expression comes
        // from Granite grammar rather than a decorative mesh/spawn layer.
        float m_rockHeadReliefM = 0.0f;
        float m_partiallyDetachedReliefM = 0.0f;
        float m_surfaceStoneShoulderReliefM = 0.0f;

        // P3C.8 Step 7D continuous-formation inheritance receipts.
        // These are formation geometry, not separate surface bodies.
        float m_broadWeatheredBackReliefM = 0.0f;
        float m_rootedRockHeadReliefM = 0.0f;
        float m_slabShoulderReliefM = 0.0f;

        // Variable-depth / variable-aperture structural recesses.
        float m_primaryFormationJointRecessM = 0.0f;
        float m_secondaryFormationJointRecessM = 0.0f;

        uint32_t m_dominantFormationEventID = 0;
        uint32_t m_dominantFormationJointEventID = 0;
    };

    //-------------------------------------------------------------------------
    // Basic geometry types
    //-------------------------------------------------------------------------

    struct GranitePoint
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct GranitePlane
    {
        GranitePoint m_normal = { 0.0f, 0.0f, 1.0f };

        // dot( normal, point ) <= distance
        float m_distance = 0.0f;

        GraniteFaceOrigin       m_origin = GraniteFaceOrigin::FreshBreak;
        GraniteSurfaceFaceClass m_surfaceClass = GraniteSurfaceFaceClass::FreshFracture;
        float                   m_weatheredWeight = 0.0f;
        GraniteEdgeHistoryClass m_edgeHistoryClass = GraniteEdgeHistoryClass::None;
        int32_t                 m_jointSetIndex = -1;
    };

    //-------------------------------------------------------------------------
    // Geological polygon face
    //
    // This remains the parent/ancestral face even when presentation triangles
    // are locally refined into a weathered surface patch.
    //-------------------------------------------------------------------------

    struct GraniteFace
    {
        static constexpr int32_t s_maxVertices = 20;

        uint8_t m_vertexIndices[s_maxVertices] = { 0 };
        int32_t m_numVertices = 0;

        GranitePoint m_normal = { 0.0f, 0.0f, 1.0f };
        float        m_areaM2 = 0.0f;

        GraniteFaceOrigin       m_origin = GraniteFaceOrigin::Unknown;
        GraniteSurfaceFaceClass m_surfaceClass = GraniteSurfaceFaceClass::FreshFracture;
        float                   m_weatheredWeight = 0.0f;
        GraniteEdgeHistoryClass m_edgeHistoryClass = GraniteEdgeHistoryClass::None;
        int32_t                 m_jointSetIndex = -1;
    };

    //-------------------------------------------------------------------------
    // Generated body surface-history receipt
    //-------------------------------------------------------------------------

    struct GraniteBodySurfaceHistory
    {
        float m_weatheredExteriorCoverage = 0.0f;
        float m_freshFractureCoverage = 1.0f;
        float m_transitionalCoverage = 0.0f;

        float m_meanRounding = 0.0f;
        float m_meanSharpness = 1.0f;
        float m_meanPlanarity = 1.0f;

        bool m_hasInheritedExterior = false;

        //-------------------------------------------------------------------------
        // P3C.8 Step 6B measured local-surface geometry
        //-------------------------------------------------------------------------

        int32_t m_inheritedExteriorPatchCount = 0;
        int32_t m_historyAddedVertexCount = 0;
        int32_t m_historyAddedTriangleCount = 0;

        int32_t m_verticesBeforeHistory = 0;
        int32_t m_trianglesBeforeHistory = 0;

        float m_meanCrownM = 0.0f;
        float m_maxCrownM = 0.0f;

        float m_meanInheritedExteriorPlanarity = 1.0f;
        float m_meanFreshFracturePlanarity = 1.0f;
        float m_meanPrimaryJointPlanarity = 1.0f;

        float m_preHistoryMeshVolumeM3 = 0.0f;
        float m_postHistoryMeshVolumeM3 = 0.0f;

        //-------------------------------------------------------------------------
        // P3C.8 Step 6C measured edge-history geometry
        //-------------------------------------------------------------------------

        int32_t m_edgeCandidateCount = 0;
        int32_t m_edgeTransitionCount = 0;
        int32_t m_weatheredEdgeTransitionCount = 0;
        int32_t m_mixedHistoryEdgeTransitionCount = 0;
        int32_t m_supportEdgeTransitionCount = 0;
        int32_t m_freshEdgeTransitionCount = 0;

        float m_meanEdgeTransitionWidthM = 0.0f;
        float m_maxEdgeTransitionWidthM = 0.0f;
        float m_meanFreshEdgeWidthM = 0.0f;
        float m_meanWeatheredEdgeWidthM = 0.0f;

        bool m_flatFaceCentersPreserved = false;
        bool m_freshEdgesSharperThanWeathered = false;
        bool m_restingSupportUsesGeologicalFace = false;

        //-------------------------------------------------------------------------
        // Known-positive controls
        //-------------------------------------------------------------------------

        bool m_controlZeroExteriorProducesZeroCrown = false;
        bool m_controlExteriorCrownResponds = false;
        bool m_controlFreshBreakRejectsCrown = false;
    };

    //-------------------------------------------------------------------------
    // Closed Granite fracture polyhedron
    //-------------------------------------------------------------------------

    struct GraniteClosedGeometry
    {
        // Step 6C raises fixed capacity only. It does NOT change the frozen
        // fracture-polyhedron law. Extra capacity is reserved for bounded
        // inherited-exterior patches plus a small number of deterministic
        // edge-history transition planes.
        // P3C.10A-2:
        // Exact fracture partitioning clips the FINAL visible parent triangle
        // mesh rather than reconstructing children only from macro planes.
        // A cut may introduce one intersection vertex on many parent edges,
        // so the closed-child capacity is raised while remaining below the
        // uint8 index ceiling.
        static constexpr int32_t s_maxVertices = 192;
        static constexpr int32_t s_maxFaces = 48;
        static constexpr int32_t s_maxTriangles = 384;
        static constexpr int32_t s_maxTriangleIndices = s_maxTriangles * 3;
        static constexpr int32_t s_maxConstructionPlanes = 32;

        GranitePoint m_vertices[s_maxVertices];
        int32_t      m_numVertices = 0;

        // Construction-only topology: bit N means this vertex was produced by
        // a plane triple containing construction plane N. Merged vertices
        // union their masks so face membership is never re-guessed with a
        // wider distance tolerance.
        uint32_t m_vertexConstructionPlaneMask[s_maxVertices] = { 0 };

        GraniteFace m_faces[s_maxFaces];
        int32_t     m_numFaces = 0;

        uint8_t m_triangleIndices[s_maxTriangleIndices] = { 0 };
        int32_t m_numTriangles = 0;

        // Every refined child triangle still points back to its original
        // geological parent face.
        uint8_t                 m_triangleFaceIndex[s_maxTriangles] = { 0 };
        GraniteSurfaceFaceClass m_triangleSurfaceClass[s_maxTriangles];
        GraniteFaceOrigin       m_triangleFaceOrigin[s_maxTriangles];
        float                   m_triangleWeatheredWeight[s_maxTriangles] = { 0.0f };

        GranitePlane m_constructionPlanes[s_maxConstructionPlanes];
        int32_t      m_numConstructionPlanes = 0;

        int32_t m_majorFaceCount = 0;
        int32_t m_freshFractureFaceCount = 0;
        int32_t m_inheritedExteriorFaceCount = 0;
        int32_t m_primaryJointFaceCount = 0;
        int32_t m_secondaryJointFaceCount = 0;

        int32_t m_restingFaceIndex = -1;
        int32_t m_restingTriangleIndex = -1;

        GranitePoint m_restingFaceNormal = { 0.0f, 0.0f, -1.0f };
        float        m_restingFaceAreaM2 = 0.0f;

        float m_requiredSolidVolumeM3 = 0.0f;
        float m_requiredEnvelopeVolumeM3 = 0.0f;
        float m_measuredMeshVolumeM3 = 0.0f;
        float m_volumeResidualM3 = 0.0f;

        float m_extentXM = 0.0f;
        float m_extentYM = 0.0f;
        float m_extentZM = 0.0f;

        float m_effectiveAngularity = 0.0f;
        float m_effectiveEdgeWear = 0.0f;
        float m_slabCharacter = 0.0f;
        float m_elongationCharacter = 0.0f;

        GraniteBodySurfaceHistory m_surfaceHistory;

        // Diagnostic bias only; not a canonical mesh selector.
        int32_t m_topologyFamily = 0;

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
        // Standalone authority-stage diagnostics. These are deliberately not
        // part of the runtime carrier ABI or production generation cost.
        bool m_seedTriangleMeshClosed = false;
        bool m_debiasedTriangleMeshClosed = false;
        bool m_surfaceHistoryTriangleMeshClosed = false;
        bool m_structuredArticulationTriangleMeshClosed = false;
#endif
    };

    //-------------------------------------------------------------------------
    // Closed Granite request
    //-------------------------------------------------------------------------

    struct GraniteClosedGeometryRequest
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_geologicalAncestryID = 0;
        uint32_t m_bodyID = 0;
        uint32_t m_massGrams = 0;

        ProvenanceBodyState    m_bodyState = ProvenanceBodyState::Fragment;
        GraniteGeometryScale   m_scale = GraniteGeometryScale::Fragment;
        GraniteWeatheringState m_weathering = GraniteWeatheringState::FreshFracture;

        bool  m_canInheritExteriorSurface = false;
        float m_parentExteriorExposure = 0.0f;

        float m_worldX = 0.0f;
        float m_worldY = 0.0f;
        float m_worldZ = 0.0f;
    };

    //-------------------------------------------------------------------------
    // P3C.10A — Granite fracture transaction
    //
    // Previous Granite work generated fracture-shaped geometry and preserved
    // surface-history classes, but did not yet prove an actual matter
    // transaction:
    //
    //     one parent Granite body
    //         ->
    //     two or more independently valid child Granite bodies
    //
    // while conserving mass/solid volume and preserving old-vs-new surface
    // history.
    //
    // Governing laws:
    //
    //     parent mass
    //         =
    //     sum( child mass )
    //
    //     parent solid volume
    //         =
    //     sum( child solid volume )
    //
    //     inherited parent exterior
    //         remains inherited/weathered on whichever child receives it
    //
    //     newly created cut surfaces
    //         are FreshFracture
    //
    // This is a transaction contract, not a visual crack effect.
    //-------------------------------------------------------------------------

    enum class GraniteFractureChildRole : uint8_t
    {
        Primary = 0,
        Secondary,
        Chip
    };

    //-------------------------------------------------------------------------
    // World-space fracture plane
    //
    // Plane equation:
    //
    //     dot( normal, point - pointOnPlane ) = 0
    //
    // Positive/negative half-spaces determine child ownership.
    //-------------------------------------------------------------------------

    struct GraniteFracturePlane
    {
        GranitePoint m_normal = { 1.0f, 0.0f, 0.0f };

        GranitePoint m_pointOnPlane = { 0.0f, 0.0f, 0.0f };

        // Diagnostic relationship to the material grammar.
        GraniteFaceOrigin m_origin = GraniteFaceOrigin::FreshBreak;
        int32_t           m_jointSetIndex = -1;
    };

    //-------------------------------------------------------------------------
    // Transaction request
    //
    // The first certificate intentionally fractures one already-generated
    // closed body into two principal children. Optional chip generation is
    // bounded and deterministic.
    //-------------------------------------------------------------------------

    struct GraniteFractureRequest
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_eventID = 0;

        uint32_t m_parentBodyID = 0;
        uint32_t m_parentProvenanceID = 0;
        uint32_t m_geologicalAncestryID = 0;

        // Deterministic child identity is supplied by the caller/world ledger.
        uint32_t m_primaryChildBodyID = 0;
        uint32_t m_secondaryChildBodyID = 0;

        // First pass: two principal children. Later passes may enable a small
        // bounded chip budget without changing the transaction law.
        bool    m_allowChips = false;
        int32_t m_maxChipCount = 0;

        GraniteFracturePlane m_plane;

        // Small positive tolerance used when classifying vertices/polygons
        // lying numerically on the fracture plane.
        float m_planeToleranceM = 0.0005f;
    };

    //-------------------------------------------------------------------------
    // Child-body surface-history receipt
    //-------------------------------------------------------------------------

    struct GraniteFractureChildSurfaceReceipt
    {
        int32_t m_inheritedExteriorFaceCount = 0;
        int32_t m_freshFractureFaceCount = 0;
        int32_t m_transitionalFaceCount = 0;

        float m_inheritedExteriorAreaM2 = 0.0f;
        float m_newFractureAreaM2 = 0.0f;
        float m_transitionalAreaM2 = 0.0f;

        bool m_hasInheritedExterior = false;
        bool m_hasFreshFractureSurface = false;
    };

    //-------------------------------------------------------------------------
    // One resulting child body
    //-------------------------------------------------------------------------

    struct GraniteFractureChild
    {
        bool m_valid = false;

        GraniteFractureChildRole m_role =
            GraniteFractureChildRole::Primary;

        uint32_t m_bodyID = 0;
        uint32_t m_parentBodyID = 0;
        uint32_t m_provenanceID = 0;
        uint32_t m_geologicalAncestryID = 0;

        uint32_t m_massGrams = 0;

        float m_massKg = 0.0f;
        float m_requiredSolidVolumeM3 = 0.0f;
        float m_measuredMeshVolumeM3 = 0.0f;

        // Fraction of the original parent solid volume represented by this
        // child after clipping and closure.
        float m_parentVolumeFraction = 0.0f;

        GraniteClosedGeometry m_geometry;

        GraniteFractureChildSurfaceReceipt m_surfaceReceipt;
    };

    //-------------------------------------------------------------------------
    // P3C.10A-2 — geometric partition certificate
    //
    // P3C.10A already proves:
    //
    //     accounting union
    //     provenance ancestry
    //     fresh-vs-inherited surface history
    //
    // P3C.10A-2 additionally proves that the fracture is a true geometric
    // partition of the FINAL visible parent mesh:
    //
    //     parent geometry
    //         =
    //     primary child geometry
    //         UNION
    //     secondary child geometry
    //
    // with no finite overlap, no finite gap, and one coincident internal cut.
    //
    // The shared cut surface is an internal zero-thickness boundary and
    // therefore contributes no duplicate solid volume.
    //-------------------------------------------------------------------------

    struct GraniteFractureGeometricPartitionReceipt
    {
        // Exact parent visible mesh volume before fracture.
        float m_parentVisibleMeshVolumeM3 = 0.0f;

        // Child visible mesh volumes after clipping and cap closure.
        float m_primaryVisibleMeshVolumeM3 = 0.0f;
        float m_secondaryVisibleMeshVolumeM3 = 0.0f;
        float m_childVisibleMeshVolumeSumM3 = 0.0f;

        // child sum - parent visible mesh volume.
        float m_visibleMeshUnionResidualM3 = 0.0f;
        float m_visibleMeshUnionRelativeResidual = 0.0f;

        // Parent exterior area is partitioned between the two children.
        // Fresh cap area is intentionally excluded from this exterior test.
        float m_parentExteriorAreaM2 = 0.0f;
        float m_childInheritedExteriorAreaSumM2 = 0.0f;
        float m_exteriorAreaResidualM2 = 0.0f;
        float m_exteriorAreaRelativeResidual = 0.0f;

        // The two newly-created cap faces must represent the same geometric
        // section of the fracture plane.
        float m_primaryCutAreaM2 = 0.0f;
        float m_secondaryCutAreaM2 = 0.0f;
        float m_cutAreaResidualM2 = 0.0f;
        float m_cutAreaRelativeResidual = 0.0f;

        // Maximum absolute distance of any generated cut vertex from the
        // requested fracture plane.
        float m_maxCutPlaneDeviationM = 0.0f;

        // Half-space ownership proof:
        //
        // primary may occupy only one closed half-space;
        // secondary may occupy only the complementary half-space.
        //
        // Values measure the worst finite violation beyond the cut plane.
        float m_primaryHalfSpaceViolationM = 0.0f;
        float m_secondaryHalfSpaceViolationM = 0.0f;

        // Cut topology receipts.
        int32_t m_cutLoopCount = 0;
        int32_t m_cutVertexCount = 0;
        int32_t m_primaryInheritedTriangleCount = 0;
        int32_t m_secondaryInheritedTriangleCount = 0;
        int32_t m_primaryCapTriangleCount = 0;
        int32_t m_secondaryCapTriangleCount = 0;

        // Hard certificate gates.
        bool m_visibleMeshUnionPass = false;
        bool m_exteriorPartitionPass = false;
        bool m_sharedCutCoincidencePass = false;
        bool m_halfSpaceOwnershipPass = false;
        bool m_closedChildTopologyPass = false;

        bool m_pass = false;
    };

    //-------------------------------------------------------------------------
    // Whole fracture transaction certificate
    //-------------------------------------------------------------------------

    enum class GraniteFractureRejectionReason : uint8_t
    {
        None = 0,
        InvalidParentInput,
        ParentTriangleMeshNotClosed,
        ParentSeedTriangleMeshNotClosed,
        ParentDebiasedTriangleMeshNotClosed,
        ParentSurfaceHistoryTriangleMeshNotClosed,
        ParentStructuredArticulationTriangleMeshNotClosed,
        InvalidPlaneNormal,
        PlaneDoesNotSplitParent,
        ChildConstructionFailed,
        CutTopologyInsufficient,
        CutTopologyCapacityExceeded,
        CutTopologyOpen,
        CutTopologyBranch,
        CutTopologyTraversalFailed,
        CapLoopInvalid,
        PrimaryCapDegenerateTriangle,
        SecondaryCapDegenerateTriangle,
        BothCapsDegenerateTriangle,
        PrimaryCapTriangleRejected,
        SecondaryCapTriangleRejected,
        PrimaryCapVertexCapacityExceeded,
        SecondaryCapVertexCapacityExceeded,
        PrimaryCapTriangleCapacityExceeded,
        SecondaryCapTriangleCapacityExceeded,
        InsufficientChildMesh,
        NonPositiveChildVolume,
        CertificateGateFailed
    };

    //-------------------------------------------------------------------------

    struct GraniteFractureTransactionResult
    {
        static constexpr int32_t s_maxChildren = 6;

        bool m_valid = false;

        GraniteFractureRejectionReason m_rejectionReason =
            GraniteFractureRejectionReason::None;

        int32_t m_parentTopologyFamily = -1;

        uint32_t m_eventID = 0;
        uint32_t m_parentBodyID = 0;
        uint32_t m_parentProvenanceID = 0;
        uint32_t m_geologicalAncestryID = 0;

        GraniteFracturePlane m_plane;

        uint32_t m_parentMassGrams = 0;
        float    m_parentMassKg = 0.0f;

        float m_parentSolidVolumeM3 = 0.0f;
        float m_parentMeasuredMeshVolumeM3 = 0.0f;

        GraniteFractureChild m_children[s_maxChildren];
        int32_t              m_numChildren = 0;
        int32_t              m_numPrincipalChildren = 0;
        int32_t              m_numChipChildren = 0;

        uint64_t m_totalChildMassGrams = 0;
        float    m_totalChildSolidVolumeM3 = 0.0f;
        float    m_totalChildMeasuredMeshVolumeM3 = 0.0f;

        int64_t m_massResidualGrams = 0;
        float   m_solidVolumeResidualM3 = 0.0f;
        // Legacy diagnostic retained for the transaction panel.
        // In P3C.10A-2 this is:
        //
        //     child visible mesh sum - parent visible mesh volume
        //
        // rather than child mesh sum - abstract authoritative solid volume.
        float m_meshVolumeResidualM3 = 0.0f;

        float m_massRelativeResidual = 0.0f;
        float m_solidVolumeRelativeResidual = 0.0f;

        GraniteFractureGeometricPartitionReceipt m_geometricPartition;

        // Transaction/surface-history hard gates.
        bool m_massConservationPass = false;
        bool m_solidVolumeConservationPass = false;
        bool m_childGeometryPass = false;
        bool m_surfaceHistoryPass = false;

        // At least one child must preserve inherited exterior and both
        // principal children must expose newly-created FreshFracture area.
        bool m_inheritedExteriorPreserved = false;
        bool m_bothPrincipalChildrenHaveFreshBreak = false;

        // P3C.10A-2:
        // A fracture transaction is not fully certified merely because grams
        // add up. The two child meshes must also be an exact complementary
        // partition of the visible parent geometry.
        bool m_geometricPartitionPass = false;
    };

    //-------------------------------------------------------------------------
    // Boulder-scale Granite surface-stone descriptors
    //
    // Descriptor fields are absolute-world geological truth/query results.
    // Query bounds only select which already-deterministic events are returned;
    // they must never influence event identity, placement, or geometry.
    //-------------------------------------------------------------------------

    struct GraniteSurfaceStoneDescriptor
    {
        bool m_valid = false;

        uint32_t m_eventID = 0;
        uint32_t m_geologicalAncestryID = 0;
        uint32_t m_bodyGrammarSalt = 0;

        GraniteSurfaceStoneState m_state =
            GraniteSurfaceStoneState::AttachedRockHead;

        float m_centerWorldX = 0.0f;
        float m_centerWorldY = 0.0f;

        float m_orientationRadians = 0.0f;
        float m_majorRadiusM = 0.0f;
        float m_minorRadiusM = 0.0f;

        // Maximum coherent positive relief for attached/partially detached
        // forms. Detached candidates do not add this to parent bedrock.
        float m_maximumReliefM = 0.0f;

        // Positive means the body is visually/geometrically rooted below the
        // local exposed surface rather than placed as a prop on top.
        float m_embeddingDepthM = 0.0f;

        float m_weatheredExteriorWeight = 0.0f;
        float m_jointIsolation = 0.0f;
        float m_remainingConnection = 1.0f;

        // Signed local downslope preference used later for deterministic
        // detached-stone settling/placement. It is descriptive only here.
        float m_downslopeBias = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct GraniteSurfaceStoneFieldRequest
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_geologicalAncestryID = 0;

        float m_minWorldX = 0.0f;
        float m_minWorldY = 0.0f;
        float m_maxWorldX = 0.0f;
        float m_maxWorldY = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct GraniteSurfaceStoneField
    {
        static constexpr int32_t s_maxDescriptors = 8;

        GraniteSurfaceStoneDescriptor m_descriptors[s_maxDescriptors];
        int32_t                       m_numDescriptors = 0;

        uint32_t m_dominantEventID = 0;

        int32_t m_numAttachedRockHeads = 0;
        int32_t m_numPartiallyDetachedBlocks = 0;
        int32_t m_numDetachedCandidates = 0;
    };

    //-------------------------------------------------------------------------
    // Continuous formation-scale form descriptors
    //-------------------------------------------------------------------------

    struct GraniteFormationFormDescriptor
    {
        bool m_valid = false;

        uint32_t m_eventID = 0;
        uint32_t m_geologicalAncestryID = 0;
        uint32_t m_grammarSalt = 0;

        GraniteFormationFormType m_type =
            GraniteFormationFormType::BroadWeatheredBack;

        GraniteFormationAttachmentState m_attachment =
            GraniteFormationAttachmentState::CoherentBedrock;

        float m_centerWorldX = 0.0f;
        float m_centerWorldY = 0.0f;
        float m_orientationRadians = 0.0f;

        float m_majorRadiusM = 0.0f;
        float m_minorRadiusM = 0.0f;
        float m_maximumReliefM = 0.0f;

        // Width of the continuous return into surrounding bedrock. A rooted
        // head must reach zero relief through this zone rather than ending in
        // a circular prop-like skirt.
        float m_rootBlendRadiusM = 0.0f;

        // A broad back may preserve metres of coherent, relatively quiet
        // surface before a major structural event interrupts it.
        float m_quietCoreRadiusM = 0.0f;

        // Used primarily by slab/shoulder forms. This is a dominant large-
        // scale tendency; local surface articulation remains downstream.
        float m_dipRadians = 0.0f;
        float m_shoulderSharpness = 0.0f;

        float m_weatheredExteriorWeight = 0.0f;
        float m_fractureShoulderWeight = 0.0f;
        float m_asymmetry = 0.0f;

        // Dominant joint ancestry influencing the form, or -1 when the form
        // is governed by coherent-mass weathering rather than one joint set.
        int32_t m_primaryJointSetIndex = -1;
    };

    //-------------------------------------------------------------------------
    // Finite structural seam/joint descriptor
    //
    // A seam follows a deterministic curved path and carries varying depth
    // and aperture along that path. Cubic path and profile controls allow it
    // to open, deepen, narrow and close naturally rather than using one
    // uniform trench width/depth.
    //-------------------------------------------------------------------------

    struct GraniteFormationJointDescriptor
    {
        bool m_valid = false;

        uint32_t m_eventID = 0;
        uint32_t m_geologicalAncestryID = 0;

        GraniteJointTier m_tier = GraniteJointTier::Primary;
        int32_t          m_jointSetIndex = -1;

        // Cubic absolute-world centerline. This matches the existing curved
        // Granite groove evaluator and permits a seam to bend independently
        // near either end.
        float m_p0WorldX = 0.0f;
        float m_p0WorldY = 0.0f;
        float m_p1WorldX = 0.0f;
        float m_p1WorldY = 0.0f;
        float m_p2WorldX = 0.0f;
        float m_p2WorldY = 0.0f;
        float m_p3WorldX = 0.0f;
        float m_p3WorldY = 0.0f;

        float m_lengthM = 0.0f;

        // Depth and aperture are cubic envelopes along the same parameter t
        // as the centerline. They may open, deepen, narrow, and close in an
        // asymmetric way instead of using one constant trench section.
        float m_depthAtP0M = 0.0f;
        float m_depthAtP1M = 0.0f;
        float m_depthAtP2M = 0.0f;
        float m_depthAtP3M = 0.0f;

        float m_apertureAtP0M = 0.0f;
        float m_apertureAtP1M = 0.0f;
        float m_apertureAtP2M = 0.0f;
        float m_apertureAtP3M = 0.0f;

        // Optional raised shoulders bordering the recessed centerline. These
        // remain part of the same structural event and may differ by side.
        float m_leftShoulderLiftM = 0.0f;
        float m_rightShoulderLiftM = 0.0f;

        float m_persistence = 0.0f;
        float m_weathering = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct GraniteFormationFieldRequest
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_geologicalAncestryID = 0;

        float m_minWorldX = 0.0f;
        float m_minWorldY = 0.0f;
        float m_maxWorldX = 0.0f;
        float m_maxWorldY = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct GraniteFormationField
    {
        static constexpr int32_t s_maxForms = 12;
        static constexpr int32_t s_maxJoints = 16;

        GraniteFormationFormDescriptor m_forms[s_maxForms];
        int32_t                        m_numForms = 0;

        GraniteFormationJointDescriptor m_joints[s_maxJoints];
        int32_t                         m_numJoints = 0;

        int32_t m_numBroadWeatheredBacks = 0;
        int32_t m_numRootedRockHeads = 0;
        int32_t m_numSlabShoulders = 0;

        int32_t m_numPrimaryJoints = 0;
        int32_t m_numSecondaryJoints = 0;

        uint32_t m_dominantFormEventID = 0;
        uint32_t m_dominantJointEventID = 0;
    };

    //-------------------------------------------------------------------------
    // P3C.10B — Rooted Granite structural detachment
    //
    // P3C.10A / 10A-2 proved fracture of an already-isolated closed Granite
    // body. P3C.10B addresses a different question:
    //
    //     when does a boulder-like Granite head STOP being parent bedrock?
    //
    // A RootedRockHead may look detached while remaining one continuous
    // geological structure. Shape alone therefore cannot create a loose
    // MatterBody.
    //
    // P3C.10B models attachment as a finite set of explicit structural
    // bridges around the rooted return of a formation-scale rock head:
    //
    //     no bridge severed
    //         -> AttachedRockHead
    //
    //     one or more bridges severed,
    //     at least one bridge remains
    //         -> PartiallyDetachedBlock
    //
    //     every structural bridge severed
    //         -> DetachedSurfaceStoneCandidate
    //
    // IMPORTANT:
    //
    // DetachedSurfaceStoneCandidate is still NOT a free matter-body creation.
    // It only certifies that parent structural ownership has ended.
    //
    // A later matter-partition transaction must assign exact Granite mass and
    // volume before the detached candidate can become a ledger/world MatterBody.
    //-------------------------------------------------------------------------

    enum class GraniteStructuralBridgeAxis : uint8_t
    {
        MajorPositive = 0,
        MajorNegative,
        MinorPositive,
        MinorNegative
    };

    //-------------------------------------------------------------------------

    enum class GraniteDetachmentTransitionKind : uint8_t
    {
        None = 0,
        RootedToPartiallyDetached,
        PartiallyDetachedProgression,
        PartiallyDetachedToDetached
    };

    //-------------------------------------------------------------------------
    // One structural connection between a rooted head and parent formation.
    //
    // Bridge area is a geometry-derived structural ownership receipt. It is
    // NOT a material quantity and must never be converted directly into mass.
    //-------------------------------------------------------------------------

    struct GraniteStructuralBridgeDescriptor
    {
        bool m_valid = false;

        uint32_t m_bridgeID = 0;
        uint32_t m_parentFormEventID = 0;
        uint32_t m_geologicalAncestryID = 0;

        GraniteStructuralBridgeAxis m_axis =
            GraniteStructuralBridgeAxis::MajorPositive;

        // Approximate world-space root attachment location/orientation.
        float m_centerWorldX = 0.0f;
        float m_centerWorldY = 0.0f;
        float m_orientationRadians = 0.0f;

        // Structural bridge dimensions at the rooted return.
        float m_spanM = 0.0f;
        float m_rootDepthM = 0.0f;
        float m_attachmentAreaM2 = 0.0f;

        // Deterministic joint ancestry most capable of severing this bridge,
        // or -1 where no dominant joint set is assigned.
        int32_t m_dominantJointSetIndex = -1;

        // Transaction state.
        bool m_severed = false;

        uint32_t m_severingEventID = 0;
    };

    //-------------------------------------------------------------------------

    struct GraniteStructuralBridgeSet
    {
        static constexpr int32_t s_maxBridges = 4;

        GraniteStructuralBridgeDescriptor m_bridges[s_maxBridges];
        int32_t                           m_numBridges = 0;

        float m_initialAttachmentAreaM2 = 0.0f;
        float m_remainingAttachmentAreaM2 = 0.0f;

        float m_remainingConnectionFraction = 1.0f;
        float m_severedConnectionFraction = 0.0f;

        int32_t m_numIntactBridges = 0;
        int32_t m_numSeveredBridges = 0;
    };

    //-------------------------------------------------------------------------
    // Build request
    //
    // The source form must be:
    //
    //     GraniteFormationFormType::RootedRockHead
    //     GraniteFormationAttachmentState::RootedRockHead
    //
    // Bridge construction is deterministic from the form descriptor and seed.
    //-------------------------------------------------------------------------

    struct GraniteRootedBridgeSetRequest
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_structuralEventID = 0;

        GraniteFormationFormDescriptor m_rootedForm;
    };

    //-------------------------------------------------------------------------
    // Structural detachment event
    //
    // The event severs explicit bridge IDs through a bit mask.
    //
    // bit 0 -> bridge 0
    // bit 1 -> bridge 1
    // bit 2 -> bridge 2
    // bit 3 -> bridge 3
    //
    // This is intentionally discrete in P3C.10B. Later fracture/joint physics
    // may determine which bridge fails; they must feed the same transaction
    // contract instead of bypassing it with a visual state change.
    //-------------------------------------------------------------------------

    struct GraniteDetachmentEventRequest
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_eventID = 0;

        GraniteStructuralBridgeSet m_before;

        uint8_t m_severBridgeMask = 0;
    };

    //-------------------------------------------------------------------------
    // P3C.11A-1 — player/tool strike contract
    //
    // This is the engine-free authority boundary used by both the verifier and
    // the playable Granite strike lab. Input/raycast/animation may be engine
    // owned, but the decision about which structural bridge a strike reaches
    // is deterministic POD geometry here.
    //-------------------------------------------------------------------------

    static constexpr uint32_t s_graniteToolStrikeAlgorithmVersion = 1u;

    enum class GraniteToolStrikeRejectionReason : uint8_t
    {
        None = 0,
        InvalidRequest,
        InvalidContactFrame,
        UnsupportedMaterial,
        NoIntactStructuralBridge,
        NoBridgeInsideFootprint,
        DetachmentTransactionFailed
    };

    struct GraniteToolStrikeRequest
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_strikeEventID = 0;
        uint32_t m_priorStrikeCount = 0;

        GraniteFormationFormDescriptor m_rootedForm;
        GraniteStructuralBridgeSet      m_before;

        // Exact world-space contact supplied by the caller's raycast.
        float m_contactWorldX = 0.0f;
        float m_contactWorldY = 0.0f;
        float m_contactWorldZ = 0.0f;

        // Outward surface normal and incoming tool direction. The solver
        // reconstructs a stable orthonormal N/T/B contact frame.
        float m_surfaceNormalX = 0.0f;
        float m_surfaceNormalY = 0.0f;
        float m_surfaceNormalZ = 1.0f;

        float m_incomingDirectionX = 0.0f;
        float m_incomingDirectionY = 0.0f;
        float m_incomingDirectionZ = -1.0f;

        // Compact pick-scale influence controls. These define a candidate
        // removal/fracture influence region; they are not themselves removed
        // matter and cannot bypass the later partition transaction.
        float m_toolTipRadiusM = 0.045f;
        float m_maxPenetrationM = 0.075f;
        float m_normalizedStrikeEnergy = 0.72f;
        float m_toolHardness = 0.92f;

        GraniteWeatheringState m_materialState =
            GraniteWeatheringState::WeatheredExposure;
    };

    //-------------------------------------------------------------------------
    // P3C.10B structural ownership receipt
    //-------------------------------------------------------------------------

    struct GraniteDetachmentResult
    {
        bool m_valid = false;

        uint32_t m_eventID = 0;
        uint32_t m_parentFormEventID = 0;
        uint32_t m_geologicalAncestryID = 0;

        GraniteSurfaceStoneState m_stateBefore =
            GraniteSurfaceStoneState::AttachedRockHead;

        GraniteSurfaceStoneState m_stateAfter =
            GraniteSurfaceStoneState::AttachedRockHead;

        GraniteDetachmentTransitionKind m_transition =
            GraniteDetachmentTransitionKind::None;

        GraniteStructuralBridgeSet m_before;
        GraniteStructuralBridgeSet m_after;

        // Exact structural receipt for this event.
        float m_attachmentAreaRemovedM2 = 0.0f;

        uint8_t m_requestedSeverMask = 0;
        uint8_t m_effectiveSeverMask = 0;

        // Ownership doctrine:
        //
        // true:
        //     the rock head still belongs to parent bedrock elevation.
        //
        // false:
        //     all structural bridges have failed. The parent terrain must stop
        //     contributing this head as coherent bedrock.
        bool m_parentStructuralOwnershipRetained = true;

        // Becomes true only when every bridge is severed.
        //
        // This does NOT mean a new MatterBody has already been created.
        bool m_detachedCandidateReady = false;

        // Descriptor handed to the later matter-separation/settling layer once
        // structural ownership ends. It carries geometry/state ancestry only.
        GraniteSurfaceStoneDescriptor m_detachedCandidate;

        // Hard certificate gates.
        bool m_monotonicConnectionPass = false;
        bool m_stateTransitionPass = false;
        bool m_bridgeAccountingPass = false;
        bool m_parentOwnershipPass = false;

        bool m_pass = false;
    };

    //-------------------------------------------------------------------------
    // Full P3C.11A-1 strike receipt. Declared after the structural receipt it
    // embeds so the downstream ownership transaction remains explicit.
    //-------------------------------------------------------------------------

    struct GraniteToolStrikeResult
    {
        bool m_valid = false;
        bool m_accepted = false;

        uint32_t m_strikeEventID = 0;
        uint32_t m_priorStrikeCount = 0;

        GraniteToolStrikeRejectionReason m_rejectionReason =
            GraniteToolStrikeRejectionReason::None;

        // Stable normalized contact frame.
        float m_normalX = 0.0f;
        float m_normalY = 0.0f;
        float m_normalZ = 1.0f;
        float m_tangentX = 1.0f;
        float m_tangentY = 0.0f;
        float m_tangentZ = 0.0f;
        float m_bitangentX = 0.0f;
        float m_bitangentY = 1.0f;
        float m_bitangentZ = 0.0f;

        float m_footprintMajorRadiusM = 0.0f;
        float m_footprintMinorRadiusM = 0.0f;
        float m_influenceDepthM = 0.0f;

        int32_t m_selectedBridgeIndex = -1;
        uint8_t m_selectedBridgeMask = 0;
        float   m_selectedBridgeDistanceM = 0.0f;
        float   m_selectedBridgeNormalizedDistance = 0.0f;

        GraniteDetachmentResult m_detachment;

        bool m_requestPass = false;
        bool m_contactFramePass = false;
        bool m_materialResponsePass = false;
        bool m_footprintSelectionPass = false;
        bool m_structuralTransactionPass = false;
        bool m_pass = false;
    };

    //-------------------------------------------------------------------------
    // P3C.10B three-stage certification sequence
    //
    // Used by the workbench/debug certificate to prove:
    //
    //     Rooted
    //       ->
    //     PartiallyDetached
    //       ->
    //     DetachedCandidate
    //
    // from the SAME structural bridge set.
    //-------------------------------------------------------------------------

    struct GraniteDetachmentSequenceReceipt
    {
        bool m_valid = false;

        GraniteStructuralBridgeSet m_initialRooted;

        GraniteDetachmentResult m_partialEvent;
        GraniteDetachmentResult m_finalEvent;

        bool m_initialRootedPass = false;
        bool m_partialStillOwnedByParentPass = false;
        bool m_finalParentOwnershipReleasedPass = false;
        bool m_monotonicConnectionPass = false;

        bool m_pass = false;
    };

    //-------------------------------------------------------------------------
    // P3C.10B-2 — Rooted-head separation / matter partition
    //
    // P3C.10B proved structural ownership:
    //
    //     RootedRockHead
    //         ->
    //     PartiallyDetachedBlock
    //         ->
    //     DetachedSurfaceStoneCandidate
    //
    // P3C.10B-1 then exposed an important visual/physical distinction:
    //
    //     the detachable head
    //         !=
    //     the entire RootedRockHead + root-blend apron
    //
    // The parent Granite base/socket must remain in the world.
    //
    // P3C.10B-2 therefore partitions one finite LOCAL Granite parcel:
    //
    //     pre-detachment rooted parcel
    //         =
    //     detached head parcel
    //         UNION
    //     remaining parent socket parcel
    //
    // This does NOT attempt to assign a finite mass to the whole mountain.
    // Only the local Granite volume affected by the detachment event enters
    // this transaction.
    //
    // The detached child becomes explicit matter only after:
    //
    //     structural ownership released
    //     geometric partition certified
    //     local mass partition certified
    //
    //-------------------------------------------------------------------------

    struct GraniteRootedSeparationPolicy
    {
        // Sampling carrier for the local partition mesh.
        //
        // This is a geometry integration resolution, not visible voxel shape.
        float m_targetSampleSpacingM = 0.05f;

        // The separation surface is constrained below the visible head crown
        // but above the parent formation baseline. These values control the
        // socket/deep-root retention profile.
        float m_minSocketReliefFraction = 0.10f;
        float m_maxSocketReliefFraction = 0.42f;

        // Root-blend influence kept with the parent formation.
        float m_rootBlendRetention = 0.82f;

        // Bridge neighborhoods bias the socket deeper while a bridge was part
        // of the original rooted structure.
        float m_bridgeRetentionRadiusScale = 1.35f;
        float m_bridgeRetentionStrength = 0.28f;

        // Numerical certificate tolerances.
        float m_volumeRelativeTolerance = 0.0015f;
        float m_massRelativeTolerance = 0.0001f;
        float m_surfaceCoincidenceToleranceM = 0.0005f;
    };

    //-------------------------------------------------------------------------
    // FIX6 approved-shape authority
    //
    // These are product-shape bounds, not numerical conservation tolerances.
    // They are intentionally fixed beside the receipt they govern so a later
    // geometry cleanup cannot silently relax them through caller policy.
    // The corpus measurements supporting the apron/fin margins are recorded
    // beside the constants after Task 3 calibration.
    //-------------------------------------------------------------------------

    struct GraniteRootedApprovedShapeThresholds
    {
        // The detached underside and parent socket are emitted from the same
        // sampled mold. 0.5 mm is the already-approved interface tolerance and
        // is well above the 1 micrometre mesh vertex merge resolution while
        // remaining visually and mechanically negligible at boulder scale.
        static constexpr float s_maxPrincipalCastFitDeviationM = 0.0005f;

        // The approved 16-case FIX6 corpus retains 0.977699935..0.988216639
        // of root-blend-weighted apron mass. 0.95 leaves 2.77 percentage
        // points below the observed minimum for sampling drift, but still
        // rejects the old failure mode where the apron lifts with the body.
        static constexpr float s_minRetainedApronMassFraction = 0.95f;

        // Before the annotated seam trim, the independent boundary oracle
        // measured 0.138139546..0.216132537 of detached volume at the
        // positive-to-zero curtain boundary. The approved trim reduces that
        // range to exactly zero. 0.0001 permits only numerical drift and
        // deliberately rejects every recorded untrimmed case.
        static constexpr float s_maxUndersideFinVolumeFraction = 0.0001f;
    };

    //-------------------------------------------------------------------------
    // Dedicated local partition mesh
    //
    // Unlike GraniteClosedGeometry, this carrier is allowed to represent a
    // sampled terrain/socket parcel and therefore uses uint16 indices.
    //-------------------------------------------------------------------------

    struct GraniteRootedPartitionVertex
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct GraniteRootedPartitionMesh
    {
        static constexpr int32_t s_maxVertices = 2048;
        static constexpr int32_t s_maxTriangles = 4096;
        static constexpr int32_t s_maxTriangleIndices =
            s_maxTriangles *
            3;

        GraniteRootedPartitionVertex m_vertices[s_maxVertices];
        int32_t                      m_numVertices = 0;

        uint16_t m_triangleIndices[s_maxTriangleIndices] = { 0 };
        int32_t  m_numTriangles = 0;

        float m_measuredVolumeM3 = 0.0f;

        float m_extentXM = 0.0f;
        float m_extentYM = 0.0f;
        float m_extentZM = 0.0f;

        bool m_closed = false;
    };

    //-------------------------------------------------------------------------
    // One finite local rooted-head parcel
    //
    // topRelief:
    //     complete pre-detachment RootedRockHead relief contribution.
    //
    // socketRelief:
    //     Granite relief retained by the parent after detachment.
    //
    // detachedThickness:
    //
    //     max( topRelief - socketRelief, 0 )
    //
    //-------------------------------------------------------------------------

    struct GraniteRootedSeparationSample
    {
        float m_worldX = 0.0f;
        float m_worldY = 0.0f;

        float m_topReliefM = 0.0f;
        float m_socketReliefM = 0.0f;
        float m_detachedThicknessM = 0.0f;

        float m_rootBlendInfluence = 0.0f;
        float m_bridgeRetentionInfluence = 0.0f;

        bool m_insideDetachmentFootprint = false;
    };

    //-------------------------------------------------------------------------
    // Separation request
    //
    // The final detachment event must already certify:
    //
    //     stateAfter == DetachedSurfaceStoneCandidate
    //     parentStructuralOwnershipRetained == false
    //     detachedCandidateReady == true
    //
    // P3C.10B-2 will refuse to create explicit detached matter otherwise.
    //-------------------------------------------------------------------------

    struct GraniteRootedSeparationRequest
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_partitionEventID = 0;

        GraniteFormationFormDescriptor m_rootedForm;
        GraniteDetachmentResult        m_finalDetachment;

        GraniteRootedSeparationPolicy m_policy;

        // World Z of the parent Granite carrier beneath the LOCAL formation
        // contribution at the head center. The partition result is expressed
        // relative to this local carrier.
        float m_parentCarrierBaseZ = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Matter receipt for the finite local parcel.
    //
    // Integer grams are authoritative once the parcel is separated.
    //
    // The detached grams are assigned from the certified geometric fraction,
    // then the remaining parent parcel receives the exact integer remainder:
    //
    //     detached grams + parent remainder grams
    //         ==
    //     pre-detachment local parcel grams
    //
    // exactly.
    //-------------------------------------------------------------------------

    struct GraniteRootedMatterPartitionReceipt
    {
        float m_densityKgPerM3 = 0.0f;

        float m_preDetachParcelVolumeM3 = 0.0f;

        // Principal detached rock-head volume: this is the cast-fit body that
        // should seat back into the surviving parent socket.
        float m_detachedVolumeM3 = 0.0f;

        // Thin fracture fringe/spall volume intentionally excluded from the
        // principal detached body. It is NOT returned to the parent socket.
        // P3C.10C may later instantiate this reserved matter as explicit chips.
        float m_spallReserveVolumeM3 = 0.0f;

        float m_remainingParentVolumeM3 = 0.0f;

        float m_volumeResidualM3 = 0.0f;
        float m_volumeRelativeResidual = 0.0f;

        uint32_t m_preDetachParcelMassGrams = 0;

        // Exact integer matter assigned to the principal cast-fit head.
        // Principal cast-fit body only. Reserved spall grams are separate.
        uint32_t m_detachedMassGrams = 0;

        // Exact integer matter reserved for the thin fracture fringe/spalls.
        // This mass is conserved but is not yet committed as live chip bodies.
        uint32_t m_spallReserveMassGrams = 0;

        uint32_t m_remainingParentMassGrams = 0;

        int64_t m_massResidualGrams = 0;
        float   m_massRelativeResidual = 0.0f;

        bool m_volumeConservationPass = false;
        bool m_massConservationPass = false;
    };

    //-------------------------------------------------------------------------
    // Full rooted-head separation result
    //-------------------------------------------------------------------------

    struct GraniteRootedSeparationResult
    {
        bool m_valid = false;

        uint32_t m_partitionEventID = 0;
        uint32_t m_parentFormEventID = 0;
        uint32_t m_geologicalAncestryID = 0;

        GraniteFormationFormDescriptor m_rootedForm;
        GraniteSurfaceStoneDescriptor  m_sourceDetachedCandidate;

        GraniteRootedSeparationPolicy m_policy;

        // Sampled local partition domain.
        float m_minWorldX = 0.0f;
        float m_minWorldY = 0.0f;
        float m_maxWorldX = 0.0f;
        float m_maxWorldY = 0.0f;

        float m_sampleSpacingM = 0.0f;

        int32_t m_samplesX = 0;
        int32_t m_samplesY = 0;

        // Final explicit geometry pieces.
        //
        // detachedHeadMesh:
        //     the piece that may become an independent Granite MatterBody.
        //
        // remainingParentSocketMesh:
        //     the local Granite base/socket that stays with parent bedrock.
        GraniteRootedPartitionMesh m_detachedHeadMesh;
        GraniteRootedPartitionMesh m_remainingParentSocketMesh;

        GraniteRootedMatterPartitionReceipt m_matter;

        // Separation-interface receipts.
        float m_sharedInterfaceAreaM2 = 0.0f;
        float m_maxSharedInterfaceDeviationM = 0.0f;

        // Approved FIX6 shape receipts. These are independently measured from
        // the emitted mold/cast meshes and the sampled ownership field.
        uint32_t m_retainedApronMassGrams = 0;
        float    m_retainedApronMassFraction = 0.0f;

        float m_undersideFinVolumeM3 = 0.0f;
        float m_undersideFinVolumeFraction = 0.0f;

        // Detached explicit matter identity proposal.
        //
        // These IDs/mass are transaction outputs only. The caller/world ledger
        // still decides whether/when to commit them as a live MatterBody.
        uint32_t m_detachedBodyID = 0;
        uint32_t m_detachedParentBodyID = 0;
        uint32_t m_detachedProvenanceID = 0;
        uint32_t m_detachedMassGrams = 0;

        // Hard certificate gates.
        bool m_structuralReleasePass = false;

        // Three-way local partition:
        //
        //     original parcel
        //       =
        //     principal detached cast
        //       +
        //     parent socket
        //       +
        //     reserved fracture spalls
        bool m_geometricPartitionPass = false;

        // Principal detached underside and surviving socket use the same
        // mold/cast separation surface.
        bool m_sharedInterfacePass = false;

        bool m_detachedMeshClosedPass = false;
        bool m_parentSocketMeshClosedPass = false;

        // The principal detached body contains no sub-threshold fringe sheets.
        bool m_principalCastFitPass = false;

        // Cast fit, parent-owned apron and underside-fin bounds all describe
        // the visually approved FIX6 shape and must pass together.
        bool m_approvedShapePass = false;

        // Spall reserve is conserved explicitly rather than silently returned
        // to the parent apron or deleted.
        bool m_spallReservePass = false;

        bool m_matterPartitionPass = false;

        bool m_pass = false;
    };

    //-------------------------------------------------------------------------
    // Outcrop request/result
    //-------------------------------------------------------------------------

    struct GraniteOutcropRequest
    {
        uint32_t m_worldSeed = 0;
        uint32_t m_geologicalAncestryID = 0;

        GraniteWeatheringState m_weathering = GraniteWeatheringState::WeatheredExposure;

        float m_worldX = 0.0f;
        float m_worldY = 0.0f;
        float m_baseBedrockZ = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct GraniteOutcropResult
    {
        float m_baseBedrockZ = 0.0f;

        float m_largeScaleReliefM = 0.0f;
        float m_mediumScaleReliefM = 0.0f;
        float m_smallScaleReliefM = 0.0f;

        float m_primaryJointRecessM = 0.0f;
        float m_secondaryJointRecessM = 0.0f;

        float m_structuralReliefM = 0.0f;
        float m_weatheringReliefM = 0.0f;

        // Step 7D explicit continuous-formation components.
        float m_broadWeatheredBackReliefM = 0.0f;
        float m_rootedRockHeadReliefM = 0.0f;
        float m_slabShoulderReliefM = 0.0f;
        float m_primaryFormationJointRecessM = 0.0f;
        float m_secondaryFormationJointRecessM = 0.0f;

        float m_surfaceZ = 0.0f;

        GraniteJointFieldSample m_jointField;
    };

    //-------------------------------------------------------------------------
    // Granite geometry API
    //-------------------------------------------------------------------------

    EE_GAME_API GraniteGeometryProfile GetGraniteGeometryProfile();

    EE_GAME_API GraniteJointFieldSample EvaluateGraniteJointField(
        uint32_t               worldSeed,
        uint32_t               geologicalAncestryID,
        float                  worldX,
        float                  worldY,
        GraniteWeatheringState weathering );

    EE_GAME_API GraniteClosedGeometry GenerateGraniteClosedGeometry(
        GraniteClosedGeometryRequest const& request );

    // Returns deterministic boulder-scale structural descriptors selected by
    // absolute-world query bounds. The bounds do not participate in geology.
    EE_GAME_API GraniteSurfaceStoneField GenerateGraniteSurfaceStoneField(
        GraniteSurfaceStoneFieldRequest const& request );

    // Returns deterministic continuous-formation forms and finite variable-
    // width/depth joints selected by absolute-world query bounds. The query
    // bounds never participate in event generation.
    EE_GAME_API GraniteFormationField GenerateGraniteFormationField(
        GraniteFormationFieldRequest const& request );

    //-------------------------------------------------------------------------
    // P3C.10B-1 — isolated formation-form geometry witness
    //
    // Evaluates the SAME material-owned formation grammar used by the Granite
    // outcrop solver, but for one explicit descriptor only.
    //
    // This exists so structural-state diagnostics can render the actual
    // selected RootedRockHead instead of a symbolic bridge/ruler diagram.
    //
    // It is a presentation/evaluation query only:
    //
    //     descriptor + absolute XY -> positive Granite relief in metres
    //
    // It does not change geology, ownership, mass, fracture state, package
    // residency, or the formation descriptor.
    //-------------------------------------------------------------------------

    EE_GAME_API float EvaluateGraniteFormationFormRelief(
        GraniteFormationFormDescriptor const& descriptor,
        float                                 worldX,
        float                                 worldY );

    //-------------------------------------------------------------------------
    // P3C.10B structural-detachment API
    //-------------------------------------------------------------------------

    EE_GAME_API GraniteStructuralBridgeSet BuildGraniteRootedBridgeSet(
        GraniteRootedBridgeSetRequest const& request );

    EE_GAME_API GraniteDetachmentResult ApplyGraniteDetachmentEvent(
        GraniteDetachmentEventRequest const& request );

    EE_GAME_API GraniteToolStrikeResult ApplyGraniteToolStrike(
        GraniteToolStrikeRequest const& request );

    // Deterministic workbench certificate helper.
    //
    // The sequence intentionally uses two events:
    //
    //     event A severs a subset of bridges
    //         -> PartiallyDetachedBlock
    //
    //     event B severs the remaining bridges
    //         -> DetachedSurfaceStoneCandidate
    //
    // The same initial rooted form and bridge identities survive throughout.
    EE_GAME_API GraniteDetachmentSequenceReceipt
    CertifyGraniteRootedDetachmentSequence(
        GraniteRootedBridgeSetRequest const& request );

    //-------------------------------------------------------------------------
    // P3C.10B-2 rooted-head separation / finite matter partition
    //-------------------------------------------------------------------------

    EE_GAME_API GraniteRootedSeparationResult
    BuildGraniteRootedSeparationTransaction(
        GraniteRootedSeparationRequest const& request );

    EE_GAME_API GraniteOutcropResult EvaluateGraniteOutcropGeometry(
        GraniteOutcropRequest const& request );

    EE_GAME_API float MeasureGraniteClosedGeometryVolume(
        GraniteClosedGeometry const& geometry );

    //-------------------------------------------------------------------------
    // P3C.10A fracture transaction API
    //
    // The parent geometry/request are explicit so fracture consumes an
    // existing matter body rather than generating unrelated child props.
    //
    // P3C.10A / P3C.10A-2 certificate:
    //
    //     FINAL parent Granite triangle mesh
    //         + deterministic fracture plane
    //     ->
    //     two complementary closed child meshes
    //
    // with:
    //
    //     exact integer-gram partition
    //     authoritative solid-volume conservation
    //     exact visible-mesh geometric union
    //     coincident shared cut surface
    //     complementary half-space ownership
    //     inherited parent exterior
    //     FreshFracture cut faces
    //
    // Children are clipped from the final parent triangles. They are NOT
    // regenerated as unrelated rocks and are NOT globally rescaled after
    // fracture.
    //-------------------------------------------------------------------------

    EE_GAME_API GraniteFractureTransactionResult FractureGraniteClosedGeometry(
        GraniteClosedGeometry const&        parentGeometry,
        GraniteClosedGeometryRequest const& parentRequest,
        GraniteFractureRequest const&       fractureRequest );

    //-------------------------------------------------------------------------
    // P3C.10C-1 — Granite spall-resolution contract
    //
    // P3C.10B-2 FIX6 established three-way local accounting:
    //
    //     pre-detachment Granite parcel
    //         =
    //     remaining parent socket
    //         +
    //     principal detached rock
    //         +
    //     conserved spall reserve
    //
    // P3C.10C-1 defines how that reserve may become explicit secondary matter
    // WITHOUT confusing rendering particles with matter authority.
    //
    // Coarse spalls / chips / grit:
    //     may become explicit Granite MatterBodies.
    //
    // Fine reserve:
    //     remains authoritative mass accounting. A later dust particle system
    //     may visualize it, but particle count is never matter count.
    //
    // This header only defines the deterministic transaction contract.
    // Geometry/placement resolution is implemented separately.
    //-------------------------------------------------------------------------

    enum class GraniteSpallMatterClass : uint8_t
    {
        CoarseSpall = 0,
        Chip,
        Grit
    };

    //-------------------------------------------------------------------------

    struct GraniteSpallResolutionPolicy
    {
        // Hard storage bound for one deterministic secondary-fragment event.
        // The implementation may emit fewer pieces.
        int32_t m_maxExplicitPieces = 10;

        // Below this mass, a candidate is kept in the fine reserve instead of
        // becoming an explicit MatterBody candidate.
        uint32_t m_minExplicitPieceMassGrams = 60;

        // Classification thresholds. These affect representation only; mass
        // conservation is independent of class.
        uint32_t m_minChipMassGrams = 180;

        uint32_t m_minCoarseSpallMassGrams = 750;

        // Deterministic fraction of reserve intentionally kept as fine matter.
        // Actual value is selected inside [min,max] from world/event identity.
        float m_minFineReserveFraction = 0.08f;

        float m_maxFineReserveFraction = 0.20f;

        // Exact integer-mass receipt tolerance. Default is strict zero.
        int64_t m_massResidualToleranceGrams = 0;
    };

    //-------------------------------------------------------------------------

    struct GraniteSpallResolutionRequest
    {
        uint32_t m_worldSeed = 0;

        // P3C.10B-2 rooted-separation transaction identity.
        uint32_t m_sourcePartitionEventID = 0;

        // Principal detached body's proposed/live identity. Secondary fragments
        // descend from this body, not from an invented unrelated parent.
        uint32_t m_parentDetachedBodyID = 0;

        uint32_t m_parentProvenanceID = 0;

        uint32_t m_geologicalAncestryID = 0;

        // Exact authoritative reserve created by the preceding separation.
        uint32_t m_spallReserveMassGrams = 0;

        // Event-space anchor only. This does NOT prescribe physics.
        float m_originWorldX = 0.0f;

        float m_originWorldY = 0.0f;

        float m_originWorldZ = 0.0f;

        GraniteSpallResolutionPolicy m_policy;
    };

    //-------------------------------------------------------------------------

    struct GraniteSpallPieceDescriptor
    {
        bool m_valid = false;

        // Deterministic secondary body identity proposal.
        uint32_t m_bodyID = 0;

        // Lineage points back to the principal detached rock.
        uint32_t m_parentBodyID = 0;

        // Stable accounting/provenance identity for this child.
        uint32_t m_provenanceID = 0;

        uint32_t m_geologicalAncestryID = 0;

        GraniteSpallMatterClass m_class =
            GraniteSpallMatterClass::Grit;

        // Exact authoritative matter quantity.
        uint32_t m_massGrams = 0;

        // Deterministic event-local placement hint. These are not physics
        // integration results and may later be replaced by actual fracture
        // contact geometry.
        float m_localOffsetX = 0.0f;

        float m_localOffsetY = 0.0f;

        float m_localOffsetZ = 0.0f;

        // Unit-ish deterministic separation bias for later initial impulse or
        // debug visualization. It carries no mass.
        float m_biasX = 0.0f;

        float m_biasY = 0.0f;

        float m_biasZ = 1.0f;
    };

    //-------------------------------------------------------------------------

    struct GraniteSpallResolutionReceipt
    {
        uint32_t m_inputReserveMassGrams = 0;

        uint32_t m_explicitPieceMassGrams = 0;

        // Authoritative fine matter that may later drive dust presentation.
        uint32_t m_fineReserveMassGrams = 0;

        int64_t m_massResidualGrams = 0;

        int32_t m_numExplicitPieces = 0;

        int32_t m_numCoarseSpalls = 0;

        int32_t m_numChips = 0;

        int32_t m_numGritPieces = 0;

        bool m_uniqueBodyIDsPass = false;

        bool m_lineagePass = false;

        bool m_pieceMinimumMassPass = false;

        bool m_massConservationPass = false;

        bool m_pass = false;
    };

    //-------------------------------------------------------------------------

    struct GraniteSpallResolutionResult
    {
        static constexpr int32_t s_maxPieces = 16;

        GraniteSpallResolutionRequest m_request;

        GraniteSpallPieceDescriptor m_pieces[s_maxPieces];

        int32_t m_numPieces = 0;

        GraniteSpallResolutionReceipt m_receipt;

        bool m_valid = false;
    };

    //-------------------------------------------------------------------------
    // Deterministically resolve conserved Granite spall reserve into:
    //
    //     explicit coarse/medium/small secondary body candidates
    //         +
    //     fine authoritative reserve
    //
    // Exact integer grams must close:
    //
    //     input reserve
    //         =
    //     sum(explicit child grams)
    //         +
    //     fine reserve grams
    //
    // No rendered particle is created here.
    //-------------------------------------------------------------------------

    EE_GAME_API GraniteSpallResolutionResult ResolveGraniteSpallReserve(
        GraniteSpallResolutionRequest const& request );

}
