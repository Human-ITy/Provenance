#include "GraniteGeometry.h"
#include <cmath>

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
    // Authority verification must report named receipt failures and return a
    // process exit code. It must never block CI on an interactive breakpoint.
    #undef EE_ASSERT
    #define EE_ASSERT( cond ) do { } while ( 0 )
#endif

#include "GraniteMeshInternal.h"

namespace EE
{
    using namespace GraniteGeometryInternal;
    using namespace GraniteMeshInternal;

    //-------------------------------------------------------------------------
    // P3C.10A / P3C.10A-2 — Granite fracture transaction
    //
    // P3C.10A proved the matter transaction and old-vs-new surface history.
    //
    // P3C.10A-2 replaces the first-pass macro-plane child reconstruction with
    // an exact partition of the FINAL visible parent triangle mesh:
    //
    //     parent visible mesh
    //         =
    //     primary child mesh
    //         UNION
    //     secondary child mesh
    //
    // The fracture plane clips each final parent triangle. The inherited
    // portions retain the parent's triangle history metadata. One shared cut
    // topology is then capped in opposite winding for the two children.
    //
    // No child is globally rescaled after fracture.
    //-------------------------------------------------------------------------

    static constexpr uint8_t s_graniteFractureCapFaceIndex =
        0xFFu;

    //-------------------------------------------------------------------------

    struct GraniteFractureClipVertex
    {
        GranitePoint m_point;
        float        m_signedDistance = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct GraniteFractureCutSegment
    {
        GranitePoint m_a;
        GranitePoint m_b;
    };

    //-------------------------------------------------------------------------

    struct GraniteFractureCutTopology
    {
        static constexpr int32_t s_maxPoints =
            GraniteClosedGeometry::s_maxVertices;

        static constexpr int32_t s_maxSegments =
            GraniteClosedGeometry::s_maxTriangles;

        GranitePoint m_points[s_maxPoints];
        int32_t      m_numPoints = 0;

        uint16_t m_segmentPointA[s_maxSegments] = { 0 };
        uint16_t m_segmentPointB[s_maxSegments] = { 0 };
        int32_t  m_numSegments = 0;

        int16_t m_neighbor0[s_maxPoints];
        int16_t m_neighbor1[s_maxPoints];
        uint8_t m_degree[s_maxPoints] = { 0 };

        int32_t m_numLoops = 0;
        bool    m_manifold = false;
        bool    m_pointCapacityExceeded = false;
        bool    m_segmentCapacityExceeded = false;
    };

    //-------------------------------------------------------------------------

    static GranitePoint ConvertGraniteWorldPointToBodyLocal(
        GranitePoint const&                 worldPoint,
        GraniteClosedGeometryRequest const& parentRequest )
    {
        GranitePoint localPoint;

        localPoint.m_x =
            worldPoint.m_x -
            parentRequest.m_worldX;

        localPoint.m_y =
            worldPoint.m_y -
            parentRequest.m_worldY;

        localPoint.m_z =
            worldPoint.m_z -
            parentRequest.m_worldZ;

        return localPoint;
    }

    //-------------------------------------------------------------------------

    static float GraniteFractureSignedDistance(
        GranitePoint const& point,
        GranitePoint const& planePoint,
        GranitePoint const& unitNormal )
    {
        return GraniteDot(
            unitNormal,
            GraniteSubtract(
                point,
                planePoint ) );
    }

    //-------------------------------------------------------------------------

    static GranitePoint ProjectGranitePointOntoFracturePlane(
        GranitePoint const& point,
        GranitePoint const& planePoint,
        GranitePoint const& unitNormal )
    {
        float const signedDistance =
            GraniteFractureSignedDistance(
                point,
                planePoint,
                unitNormal );

        return GraniteSubtract(
            point,
            GraniteScale(
                unitNormal,
                signedDistance ) );
    }

    //-------------------------------------------------------------------------

    static GranitePoint IntersectGraniteEdgeWithFracturePlane(
        GranitePoint const& a,
        float               distanceA,
        GranitePoint const& b,
        float               distanceB,
        GranitePoint const& planePoint,
        GranitePoint const& unitNormal )
    {
        float const denominator =
            distanceA -
            distanceB;

        float t =
            0.5f;

        if ( GraniteAbs(
                 denominator ) >
             0.0000001f )
        {
            t =
                distanceA /
                denominator;
        }

        t =
            GraniteClamp01(
                t );

        GranitePoint point =
            GraniteAdd(
                a,
                GraniteScale(
                    GraniteSubtract(
                        b,
                        a ),
                    t ) );

        // Force the shared boundary to the exact requested plane. Both child
        // clippers therefore consume the same zero-thickness geometric cut.
        return ProjectGranitePointOntoFracturePlane(
            point,
            planePoint,
            unitNormal );
    }

    //-------------------------------------------------------------------------

    static bool GranitePointsNearlyEqual(
        GranitePoint const& a,
        GranitePoint const& b,
        float               toleranceM )
    {
        GranitePoint const delta =
            GraniteSubtract(
                a,
                b );

        return GraniteDot(
                   delta,
                   delta ) <=
               toleranceM *
                   toleranceM;
    }

    //-------------------------------------------------------------------------

    static int32_t FindOrAddGraniteFractureVertex(
        GraniteClosedGeometry& geometry,
        GranitePoint const&    point,
        float                  mergeToleranceM )
    {
        for ( int32_t i = 0;
              i <
              geometry.m_numVertices;
              ++i )
        {
            if ( GranitePointsNearlyEqual(
                     geometry.m_vertices[i],
                     point,
                     mergeToleranceM ) )
            {
                return i;
            }
        }

        EE_ASSERT(
            geometry.m_numVertices <
            GraniteClosedGeometry::s_maxVertices );

        if ( geometry.m_numVertices >=
             GraniteClosedGeometry::s_maxVertices )
        {
            return -1;
        }

        int32_t const index =
            geometry.m_numVertices++;

        geometry.m_vertices[index] =
            point;

        return index;
    }

    //-------------------------------------------------------------------------

    static bool AddGraniteFractureTriangle(
        GraniteClosedGeometry&  geometry,
        GranitePoint const&     a,
        GranitePoint const&     b,
        GranitePoint const&     c,
        uint8_t                 sourceFaceIndex,
        GraniteSurfaceFaceClass surfaceClass,
        GraniteFaceOrigin       origin,
        float                   weatheredWeight,
        float                   mergeToleranceM )
    {
        GranitePoint const edgeAB =
            GraniteSubtract(
                b,
                a );

        GranitePoint const edgeAC =
            GraniteSubtract(
                c,
                a );

        float const doubledArea =
            GraniteLength(
                GraniteCross(
                    edgeAB,
                    edgeAC ) );

        float const minimumDoubledArea =
            GraniteMax(
                0.000000000001f,
                mergeToleranceM *
                    mergeToleranceM *
                    0.25f );

        if ( doubledArea <= minimumDoubledArea )
        {
            return false;
        }

        int32_t const ia =
            FindOrAddGraniteFractureVertex(
                geometry,
                a,
                mergeToleranceM );

        int32_t const ib =
            FindOrAddGraniteFractureVertex(
                geometry,
                b,
                mergeToleranceM );

        int32_t const ic =
            FindOrAddGraniteFractureVertex(
                geometry,
                c,
                mergeToleranceM );

        if ( ia <
                 0 ||
             ib <
                 0 ||
             ic <
                 0 )
        {
            return false;
        }

        if ( ia ==
                 ib ||
             ib ==
                 ic ||
             ic ==
                 ia )
        {
            return false;
        }

        EE_ASSERT(
            ia <=
            255 );

        EE_ASSERT(
            ib <=
            255 );

        EE_ASSERT(
            ic <=
            255 );

        EE_ASSERT(
            geometry.m_numTriangles <
            GraniteClosedGeometry::s_maxTriangles );

        if ( geometry.m_numTriangles >=
             GraniteClosedGeometry::s_maxTriangles )
        {
            return false;
        }

        int32_t const triangleIndex =
            geometry.m_numTriangles++;

        int32_t const base =
            triangleIndex *
            3;

        geometry.m_triangleIndices[base + 0] =
            uint8_t(
                ia );

        geometry.m_triangleIndices[base + 1] =
            uint8_t(
                ib );

        geometry.m_triangleIndices[base + 2] =
            uint8_t(
                ic );

        geometry.m_triangleFaceIndex[triangleIndex] =
            sourceFaceIndex;

        geometry.m_triangleSurfaceClass[triangleIndex] =
            surfaceClass;

        geometry.m_triangleFaceOrigin[triangleIndex] =
            origin;

        geometry.m_triangleWeatheredWeight[triangleIndex] =
            weatheredWeight;

        return true;
    }

    //-------------------------------------------------------------------------

    static void CopyGraniteFractureBodyCharacteristics(
        GraniteClosedGeometry const& parentGeometry,
        GraniteClosedGeometry&       childGeometry )
    {
        childGeometry.m_effectiveAngularity =
            parentGeometry.m_effectiveAngularity;

        childGeometry.m_effectiveEdgeWear =
            parentGeometry.m_effectiveEdgeWear;

        childGeometry.m_slabCharacter =
            parentGeometry.m_slabCharacter;

        childGeometry.m_elongationCharacter =
            parentGeometry.m_elongationCharacter;

        childGeometry.m_topologyFamily =
            parentGeometry.m_topologyFamily;

        // Fracture children are explicit triangle meshes. Their final visible
        // geometry is no longer represented by the parent's half-space
        // construction-plane set.
        childGeometry.m_numConstructionPlanes =
            0;

        childGeometry.m_numFaces =
            0;

        childGeometry.m_restingFaceIndex =
            -1;

        childGeometry.m_restingTriangleIndex =
            -1;
    }

    //-------------------------------------------------------------------------
    // Clip one final parent triangle against one closed half-space.
    //
    // keepNegativeHalfSpace:
    //
    //     true  -> signed distance <= 0
    //     false -> signed distance >= 0
    //
    // Boundary points belong to both children as a zero-thickness interface.
    //-------------------------------------------------------------------------

    static int32_t ClipGraniteTriangleToFractureHalfSpace(
        GranitePoint const  inputPoints[3],
        float const         inputDistances[3],
        bool                keepNegativeHalfSpace,
        GranitePoint const& planePoint,
        GranitePoint const& unitNormal,
        float               planeToleranceM,
        GranitePoint        outputPoints[6] )
    {
        GraniteFractureClipVertex input[6];
        GraniteFractureClipVertex output[6];

        int32_t inputCount =
            3;

        for ( int32_t i = 0;
              i <
              3;
              ++i )
        {
            input[i].m_point =
                inputPoints[i];

            float distance =
                inputDistances[i];

            if ( GraniteAbs(
                     distance ) <=
                 planeToleranceM )
            {
                input[i].m_point =
                    ProjectGranitePointOntoFracturePlane(
                        inputPoints[i],
                        planePoint,
                        unitNormal );

                distance =
                    0.0f;
            }

            input[i].m_signedDistance =
                distance;
        }

        int32_t outputCount =
            0;

        for ( int32_t edgeIndex = 0;
              edgeIndex <
              inputCount;
              ++edgeIndex )
        {
            GraniteFractureClipVertex const& current =
                input[edgeIndex];

            GraniteFractureClipVertex const& next =
                input[(
                          edgeIndex +
                          1 ) %
                      inputCount];

            bool const currentInside =
                keepNegativeHalfSpace
                    ? current.m_signedDistance <=
                          0.0f
                    : current.m_signedDistance >=
                          0.0f;

            bool const nextInside =
                keepNegativeHalfSpace
                    ? next.m_signedDistance <=
                          0.0f
                    : next.m_signedDistance >=
                          0.0f;

            if ( currentInside &&
                 nextInside )
            {
                EE_ASSERT(
                    outputCount <
                    6 );

                output[outputCount++] =
                    next;
            }
            else if ( currentInside &&
                      !nextInside )
            {
                GraniteFractureClipVertex intersection;

                intersection.m_point =
                    IntersectGraniteEdgeWithFracturePlane(
                        current.m_point,
                        current.m_signedDistance,
                        next.m_point,
                        next.m_signedDistance,
                        planePoint,
                        unitNormal );

                intersection.m_signedDistance =
                    0.0f;

                EE_ASSERT(
                    outputCount <
                    6 );

                output[outputCount++] =
                    intersection;
            }
            else if ( !currentInside &&
                      nextInside )
            {
                GraniteFractureClipVertex intersection;

                intersection.m_point =
                    IntersectGraniteEdgeWithFracturePlane(
                        current.m_point,
                        current.m_signedDistance,
                        next.m_point,
                        next.m_signedDistance,
                        planePoint,
                        unitNormal );

                intersection.m_signedDistance =
                    0.0f;

                EE_ASSERT(
                    outputCount +
                        1 <
                    6 );

                output[outputCount++] =
                    intersection;

                output[outputCount++] =
                    next;
            }
        }

        // Remove any numerically duplicated consecutive points introduced by
        // a vertex that lies exactly on the cut plane.
        int32_t compactCount =
            0;

        float const mergeTolerance =
            GraniteMax(
                0.000001f,
                planeToleranceM *
                    0.25f );

        for ( int32_t i = 0;
              i <
              outputCount;
              ++i )
        {
            GranitePoint const point =
                output[i].m_point;

            if ( compactCount >
                     0 &&
                 GranitePointsNearlyEqual(
                     outputPoints[compactCount -
                                  1],
                     point,
                     mergeTolerance ) )
            {
                continue;
            }

            outputPoints[compactCount++] =
                point;
        }

        if ( compactCount >
                 1 &&
             GranitePointsNearlyEqual(
                 outputPoints[0],
                 outputPoints[compactCount -
                              1],
                 mergeTolerance ) )
        {
            --compactCount;
        }

        return compactCount;
    }

    //-------------------------------------------------------------------------

    static void TriangulateGraniteFractureClippedPolygon(
        GraniteClosedGeometry&  childGeometry,
        GranitePoint const*     pPolygon,
        int32_t                 polygonPointCount,
        uint8_t                 sourceFaceIndex,
        GraniteSurfaceFaceClass surfaceClass,
        GraniteFaceOrigin       origin,
        float                   weatheredWeight,
        float                   mergeToleranceM )
    {
        if ( polygonPointCount <
             3 )
        {
            return;
        }

        for ( int32_t i = 1;
              i <
              polygonPointCount -
                  1;
              ++i )
        {
            AddGraniteFractureTriangle(
                childGeometry,
                pPolygon[0],
                pPolygon[i],
                pPolygon[i +
                         1],
                sourceFaceIndex,
                surfaceClass,
                origin,
                weatheredWeight,
                mergeToleranceM );
        }
    }

    //-------------------------------------------------------------------------

    static int32_t CollectGraniteTriangleFracturePoints(
        GranitePoint const  inputPoints[3],
        float const         inputDistances[3],
        GranitePoint const& planePoint,
        GranitePoint const& unitNormal,
        float               planeToleranceM,
        GranitePoint        outputPoints[4] )
    {
        int32_t outputCount =
            0;

        float const mergeTolerance =
            GraniteMax(
                0.000001f,
                planeToleranceM *
                    0.25f );

        auto addUniquePoint =
            [&](
                GranitePoint const& point )
        {
            for ( int32_t existingIndex = 0;
                  existingIndex <
                  outputCount;
                  ++existingIndex )
            {
                if ( GranitePointsNearlyEqual(
                         outputPoints[existingIndex],
                         point,
                         mergeTolerance ) )
                {
                    return;
                }
            }

            if ( outputCount <
                 4 )
            {
                outputPoints[outputCount++] =
                    point;
            }
        };

        for ( int32_t i = 0;
              i <
              3;
              ++i )
        {
            if ( GraniteAbs(
                     inputDistances[i] ) <=
                 planeToleranceM )
            {
                addUniquePoint(
                    ProjectGranitePointOntoFracturePlane(
                        inputPoints[i],
                        planePoint,
                        unitNormal ) );
            }
        }

        for ( int32_t edgeIndex = 0;
              edgeIndex <
              3;
              ++edgeIndex )
        {
            int32_t const nextIndex =
                ( edgeIndex +
                  1 ) %
                3;

            float const distanceA =
                inputDistances[edgeIndex];

            float const distanceB =
                inputDistances[nextIndex];

            bool const crosses =
                ( distanceA <
                      -planeToleranceM &&
                  distanceB >
                      planeToleranceM ) ||
                ( distanceA >
                      planeToleranceM &&
                  distanceB <
                      -planeToleranceM );

            if ( crosses )
            {
                addUniquePoint(
                    IntersectGraniteEdgeWithFracturePlane(
                        inputPoints[edgeIndex],
                        distanceA,
                        inputPoints[nextIndex],
                        distanceB,
                        planePoint,
                        unitNormal ) );
            }
        }

        // Generic fracture events should intersect a triangle in a line
        // segment. If a degenerate coplanar case returns more than two points,
        // preserve the farthest pair as the section segment.
        if ( outputCount >
             2 )
        {
            float bestDistanceSquared =
                -1.0f;

            GranitePoint bestA =
                outputPoints[0];

            GranitePoint bestB =
                outputPoints[1];

            for ( int32_t i = 0;
                  i <
                  outputCount;
                  ++i )
            {
                for ( int32_t j = i + 1;
                      j <
                      outputCount;
                      ++j )
                {
                    GranitePoint const delta =
                        GraniteSubtract(
                            outputPoints[i],
                            outputPoints[j] );

                    float const distanceSquared =
                        GraniteDot(
                            delta,
                            delta );

                    if ( distanceSquared >
                         bestDistanceSquared )
                    {
                        bestDistanceSquared =
                            distanceSquared;

                        bestA =
                            outputPoints[i];

                        bestB =
                            outputPoints[j];
                    }
                }
            }

            outputPoints[0] =
                bestA;

            outputPoints[1] =
                bestB;

            outputCount =
                2;
        }

        return outputCount;
    }

    //-------------------------------------------------------------------------

    static int32_t FindOrAddGraniteCutTopologyPoint(
        GraniteFractureCutTopology& topology,
        GranitePoint const&         point,
        float                       mergeToleranceM )
    {
        for ( int32_t i = 0;
              i <
              topology.m_numPoints;
              ++i )
        {
            if ( GranitePointsNearlyEqual(
                     topology.m_points[i],
                     point,
                     mergeToleranceM ) )
            {
                return i;
            }
        }

        EE_ASSERT(
            topology.m_numPoints <
            GraniteFractureCutTopology::s_maxPoints );

        if ( topology.m_numPoints >=
             GraniteFractureCutTopology::s_maxPoints )
        {
            topology.m_pointCapacityExceeded =
                true;
            return -1;
        }

        int32_t const pointIndex =
            topology.m_numPoints++;

        topology.m_points[pointIndex] =
            point;

        return pointIndex;
    }

    //-------------------------------------------------------------------------

    static bool GraniteCutTopologyHasSegment(
        GraniteFractureCutTopology const& topology,
        int32_t                           pointA,
        int32_t                           pointB )
    {
        int32_t const low =
            pointA <
                    pointB
                ? pointA
                : pointB;

        int32_t const high =
            pointA <
                    pointB
                ? pointB
                : pointA;

        for ( int32_t i = 0;
              i <
              topology.m_numSegments;
              ++i )
        {
            int32_t const existingA =
                int32_t(
                    topology.m_segmentPointA[i] );

            int32_t const existingB =
                int32_t(
                    topology.m_segmentPointB[i] );

            int32_t const existingLow =
                existingA <
                        existingB
                    ? existingA
                    : existingB;

            int32_t const existingHigh =
                existingA <
                        existingB
                    ? existingB
                    : existingA;

            if ( existingLow ==
                     low &&
                 existingHigh ==
                     high )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    static void AddGraniteCutTopologySegment(
        GraniteFractureCutTopology& topology,
        GranitePoint const&         a,
        GranitePoint const&         b,
        float                       mergeToleranceM )
    {
        if ( GranitePointsNearlyEqual(
                 a,
                 b,
                 mergeToleranceM ) )
        {
            return;
        }

        int32_t const pointA =
            FindOrAddGraniteCutTopologyPoint(
                topology,
                a,
                mergeToleranceM );

        int32_t const pointB =
            FindOrAddGraniteCutTopologyPoint(
                topology,
                b,
                mergeToleranceM );

        if ( pointA <
                 0 ||
             pointB <
                 0 ||
             pointA ==
                 pointB )
        {
            return;
        }

        if ( GraniteCutTopologyHasSegment(
                 topology,
                 pointA,
                 pointB ) )
        {
            return;
        }

        EE_ASSERT(
            topology.m_numSegments <
            GraniteFractureCutTopology::s_maxSegments );

        if ( topology.m_numSegments >=
             GraniteFractureCutTopology::s_maxSegments )
        {
            topology.m_segmentCapacityExceeded =
                true;
            return;
        }

        topology.m_segmentPointA[topology.m_numSegments] =
            uint16_t(
                pointA );

        topology.m_segmentPointB[topology.m_numSegments] =
            uint16_t(
                pointB );

        ++topology.m_numSegments;
    }

    //-------------------------------------------------------------------------

    static bool AddGraniteCutNeighbor(
        GraniteFractureCutTopology& topology,
        int32_t                     pointIndex,
        int32_t                     neighborIndex )
    {
        EE_ASSERT(
            pointIndex >=
                0 &&
            pointIndex <
                topology.m_numPoints );

        if ( topology.m_degree[pointIndex] >=
             2 )
        {
            return false;
        }

        if ( topology.m_degree[pointIndex] >=
                 1 &&
             topology.m_neighbor0[pointIndex] ==
                 neighborIndex )
        {
            return true;
        }

        if ( topology.m_degree[pointIndex] ==
             0 )
        {
            topology.m_neighbor0[pointIndex] =
                int16_t(
                    neighborIndex );
        }
        else
        {
            topology.m_neighbor1[pointIndex] =
                int16_t(
                    neighborIndex );
        }

        ++topology.m_degree[pointIndex];

        return true;
    }

    //-------------------------------------------------------------------------

    static bool FinalizeGraniteCutTopology(
        GraniteFractureCutTopology&    topology,
        GraniteFractureRejectionReason& rejectionReason )
    {
        for ( int32_t i = 0;
              i <
              GraniteFractureCutTopology::s_maxPoints;
              ++i )
        {
            topology.m_neighbor0[i] =
                -1;

            topology.m_neighbor1[i] =
                -1;

            topology.m_degree[i] =
                0;
        }

        bool manifold =
            topology.m_numPoints >=
                3 &&
            topology.m_numSegments >=
                3;

        if ( topology.m_pointCapacityExceeded ||
             topology.m_segmentCapacityExceeded )
        {
            rejectionReason =
                GraniteFractureRejectionReason::CutTopologyCapacityExceeded;
            manifold =
                false;
        }

        if ( !manifold )
        {
            if ( rejectionReason ==
                 GraniteFractureRejectionReason::None )
            {
                rejectionReason =
                    GraniteFractureRejectionReason::CutTopologyInsufficient;
            }
        }

        for ( int32_t segmentIndex = 0;
              segmentIndex <
              topology.m_numSegments;
              ++segmentIndex )
        {
            int32_t const a =
                int32_t(
                    topology.m_segmentPointA[segmentIndex] );

            int32_t const b =
                int32_t(
                    topology.m_segmentPointB[segmentIndex] );

            bool const addedA =
                AddGraniteCutNeighbor(
                    topology,
                    a,
                    b );

            bool const addedB =
                AddGraniteCutNeighbor(
                    topology,
                    b,
                    a );

            if ( !addedA ||
                 !addedB )
            {
                rejectionReason =
                    GraniteFractureRejectionReason::CutTopologyBranch;
            }

            manifold =
                addedA &&
                addedB &&
                manifold;
        }

        for ( int32_t pointIndex = 0;
              pointIndex <
              topology.m_numPoints;
              ++pointIndex )
        {
            if ( topology.m_degree[pointIndex] !=
                 2 )
            {
                if ( rejectionReason !=
                     GraniteFractureRejectionReason::CutTopologyCapacityExceeded )
                {
                    rejectionReason =
                        topology.m_degree[pointIndex] <
                                2
                            ? GraniteFractureRejectionReason::CutTopologyOpen
                            : GraniteFractureRejectionReason::CutTopologyBranch;
                }

                manifold =
                    false;
            }
        }

        bool visited[GraniteFractureCutTopology::s_maxPoints] =
            {
                false };

        int32_t loopCount =
            0;

        if ( manifold )
        {
            for ( int32_t pointIndex = 0;
                  pointIndex <
                  topology.m_numPoints;
                  ++pointIndex )
            {
                if ( visited[pointIndex] )
                {
                    continue;
                }

                ++loopCount;

                int32_t current =
                    pointIndex;

                int32_t previous =
                    -1;

                int32_t guard =
                    0;

                do
                {
                    if ( current <
                             0 ||
                         current >=
                             topology.m_numPoints )
                    {
                        manifold =
                            false;

                        rejectionReason =
                            GraniteFractureRejectionReason::CutTopologyTraversalFailed;

                        break;
                    }

                    visited[current] =
                        true;

                    int32_t const n0 =
                        int32_t(
                            topology.m_neighbor0[current] );

                    int32_t const n1 =
                        int32_t(
                            topology.m_neighbor1[current] );

                    int32_t const next =
                        n0 !=
                                previous
                            ? n0
                            : n1;

                    previous =
                        current;

                    current =
                        next;

                    ++guard;

                    if ( guard >
                         topology.m_numPoints +
                             1 )
                    {
                        manifold =
                            false;

                        rejectionReason =
                            GraniteFractureRejectionReason::CutTopologyTraversalFailed;

                        break;
                    }
                } while ( current !=
                          pointIndex );

                if ( !manifold )
                {
                    break;
                }
            }
        }

        topology.m_numLoops =
            manifold
                ? loopCount
                : 0;

        topology.m_manifold =
            manifold &&
            loopCount >
                0;

        if ( manifold &&
             loopCount <=
                 0 )
        {
            rejectionReason =
                GraniteFractureRejectionReason::CutTopologyTraversalFailed;
        }

        return topology.m_manifold;
    }

    //-------------------------------------------------------------------------

    static GranitePoint CalculateGraniteLoopNormal(
        GraniteFractureCutTopology const& topology,
        int32_t const*                    pLoopIndices,
        int32_t                           loopCount,
        GranitePoint const&               centroid )
    {
        GranitePoint normal;

        for ( int32_t i = 0;
              i <
              loopCount;
              ++i )
        {
            int32_t const next =
                ( i +
                  1 ) %
                loopCount;

            GranitePoint const a =
                GraniteSubtract(
                    topology.m_points[pLoopIndices[i]],
                    centroid );

            GranitePoint const b =
                GraniteSubtract(
                    topology.m_points[pLoopIndices[next]],
                    centroid );

            normal =
                GraniteAdd(
                    normal,
                    GraniteCross(
                        a,
                        b ) );
        }

        return GraniteNormalize(
            normal );
    }

    //-------------------------------------------------------------------------

    static void ReverseGraniteLoopIndices(
        int32_t* pLoopIndices,
        int32_t  loopCount )
    {
        for ( int32_t i = 0;
              i <
              loopCount /
                  2;
              ++i )
        {
            int32_t const opposite =
                loopCount -
                1 -
                i;

            int32_t const temp =
                pLoopIndices[i];

            pLoopIndices[i] =
                pLoopIndices[opposite];

            pLoopIndices[opposite] =
                temp;
        }
    }

    //-------------------------------------------------------------------------

    static bool AddGraniteFractureCapLoop(
        GraniteClosedGeometry&            childGeometry,
        GraniteFractureCutTopology const& topology,
        int32_t*                          pLoopIndices,
        int32_t                           loopCount,
        GranitePoint const&               desiredOutwardNormal,
        float                             mergeToleranceM,
        int32_t&                          capTriangleCount,
        GraniteFractureRejectionReason    triangleRejectedReason,
        GraniteFractureRejectionReason    degenerateTriangleReason,
        GraniteFractureRejectionReason    vertexCapacityReason,
        GraniteFractureRejectionReason    triangleCapacityReason,
        GraniteFractureRejectionReason&   rejectionReason )
    {
        if ( loopCount <
             3 )
        {
            rejectionReason =
                GraniteFractureRejectionReason::CapLoopInvalid;
            return false;
        }

        GranitePoint centroid;

        for ( int32_t i = 0;
              i <
              loopCount;
              ++i )
        {
            centroid =
                GraniteAdd(
                    centroid,
                    topology.m_points[pLoopIndices[i]] );
        }

        centroid =
            GraniteScale(
                centroid,
                1.0f /
                    float(
                        loopCount ) );

        GranitePoint loopNormal =
            CalculateGraniteLoopNormal(
                topology,
                pLoopIndices,
                loopCount,
                centroid );

        if ( GraniteDot(
                 loopNormal,
                 desiredOutwardNormal ) <
             0.0f )
        {
            ReverseGraniteLoopIndices(
                pLoopIndices,
                loopCount );

            loopNormal =
                GraniteScale(
                    loopNormal,
                    -1.0f );
        }

        int32_t trianglesAdded =
            0;

        for ( int32_t i = 0;
              i <
              loopCount;
              ++i )
        {
            int32_t const next =
                ( i +
                  1 ) %
                loopCount;

            GranitePoint const edgeA =
                GraniteSubtract(
                    topology.m_points[pLoopIndices[i]],
                    centroid );

            GranitePoint const edgeB =
                GraniteSubtract(
                    topology.m_points[pLoopIndices[next]],
                    centroid );

            bool const degenerateTriangle =
                GraniteLength(
                    GraniteCross(
                        edgeA,
                        edgeB ) ) <=
                GraniteMax(
                    0.000000000001f,
                    mergeToleranceM *
                        mergeToleranceM *
                        0.25f );

            bool const triangleAdded =
                !degenerateTriangle &&
                AddGraniteFractureTriangle(
                    childGeometry,
                    centroid,
                    topology.m_points[pLoopIndices[i]],
                    topology.m_points[pLoopIndices[next]],
                    s_graniteFractureCapFaceIndex,
                    GraniteSurfaceFaceClass::FreshFracture,
                    GraniteFaceOrigin::FreshBreak,
                    0.0f,
                    mergeToleranceM );

            if ( triangleAdded )
            {
                ++trianglesAdded;
            }
            else
            {
                rejectionReason =
                    degenerateTriangle
                        ? degenerateTriangleReason
                        : (
                              childGeometry.m_numTriangles >=
                                      GraniteClosedGeometry::s_maxTriangles
                                  ? triangleCapacityReason
                                  : (
                                        childGeometry.m_numVertices >=
                                                GraniteClosedGeometry::s_maxVertices
                                            ? vertexCapacityReason
                                            : triangleRejectedReason ) );
            }
        }

        capTriangleCount +=
            trianglesAdded;

        return trianglesAdded ==
               loopCount;
    }

    //-------------------------------------------------------------------------

    static bool AddGraniteFractureCapsFromTopology(
        GraniteFractureCutTopology const& topology,
        GranitePoint const&               fractureNormal,
        GraniteClosedGeometry&            primaryGeometry,
        GraniteClosedGeometry&            secondaryGeometry,
        float                             mergeToleranceM,
        int32_t&                          primaryCapTriangleCount,
        int32_t&                          secondaryCapTriangleCount,
        GraniteFractureRejectionReason&   rejectionReason )
    {
        if ( !topology.m_manifold )
        {
            rejectionReason =
                GraniteFractureRejectionReason::CutTopologyTraversalFailed;
            return false;
        }

        bool visited[GraniteFractureCutTopology::s_maxPoints] =
            {
                false };

        int32_t loopsAdded =
            0;

        for ( int32_t startPoint = 0;
              startPoint <
              topology.m_numPoints;
              ++startPoint )
        {
            if ( visited[startPoint] )
            {
                continue;
            }

            int32_t loopIndices[GraniteFractureCutTopology::s_maxPoints];

            int32_t loopCount =
                0;

            int32_t current =
                startPoint;

            int32_t previous =
                -1;

            int32_t guard =
                0;

            do
            {
                if ( current <
                         0 ||
                     current >=
                         topology.m_numPoints ||
                     loopCount >=
                         GraniteFractureCutTopology::s_maxPoints )
                {
                    return false;
                }

                visited[current] =
                    true;

                loopIndices[loopCount++] =
                    current;

                int32_t const n0 =
                    int32_t(
                        topology.m_neighbor0[current] );

                int32_t const n1 =
                    int32_t(
                        topology.m_neighbor1[current] );

                int32_t const next =
                    n0 !=
                            previous
                        ? n0
                        : n1;

                previous =
                    current;

                current =
                    next;

                ++guard;

                if ( guard >
                     topology.m_numPoints +
                         1 )
                {
                    return false;
                }
            } while ( current !=
                      startPoint );

            if ( loopCount <
                 3 )
            {
                return false;
            }

            // Primary keeps the negative half-space, so +normal is its
            // outward cut normal. Secondary keeps the positive half-space and
            // receives the exact opposite winding.
            int32_t primaryLoop[GraniteFractureCutTopology::s_maxPoints];

            int32_t secondaryLoop[GraniteFractureCutTopology::s_maxPoints];

            for ( int32_t i = 0;
                  i <
                  loopCount;
                  ++i )
            {
                primaryLoop[i] =
                    loopIndices[i];

                secondaryLoop[i] =
                    loopIndices[i];
            }

            GraniteFractureRejectionReason primaryRejection =
                GraniteFractureRejectionReason::None;

            GraniteFractureRejectionReason secondaryRejection =
                GraniteFractureRejectionReason::None;

            bool const primaryAdded =
                AddGraniteFractureCapLoop(
                    primaryGeometry,
                    topology,
                    primaryLoop,
                    loopCount,
                    fractureNormal,
                    mergeToleranceM,
                    primaryCapTriangleCount,
                    GraniteFractureRejectionReason::PrimaryCapTriangleRejected,
                    GraniteFractureRejectionReason::PrimaryCapDegenerateTriangle,
                    GraniteFractureRejectionReason::PrimaryCapVertexCapacityExceeded,
                    GraniteFractureRejectionReason::PrimaryCapTriangleCapacityExceeded,
                    primaryRejection );

            bool const secondaryAdded =
                AddGraniteFractureCapLoop(
                    secondaryGeometry,
                    topology,
                    secondaryLoop,
                    loopCount,
                    GraniteScale(
                        fractureNormal,
                        -1.0f ),
                    mergeToleranceM,
                    secondaryCapTriangleCount,
                    GraniteFractureRejectionReason::SecondaryCapTriangleRejected,
                    GraniteFractureRejectionReason::SecondaryCapDegenerateTriangle,
                    GraniteFractureRejectionReason::SecondaryCapVertexCapacityExceeded,
                    GraniteFractureRejectionReason::SecondaryCapTriangleCapacityExceeded,
                    secondaryRejection );

            if ( !primaryAdded ||
                 !secondaryAdded )
            {
                if ( primaryRejection ==
                         GraniteFractureRejectionReason::PrimaryCapDegenerateTriangle &&
                     secondaryRejection ==
                         GraniteFractureRejectionReason::SecondaryCapDegenerateTriangle )
                {
                    rejectionReason =
                        GraniteFractureRejectionReason::BothCapsDegenerateTriangle;
                }
                else
                {
                    rejectionReason =
                        !primaryAdded
                            ? primaryRejection
                            : secondaryRejection;
                }

                return false;
            }

            ++loopsAdded;
        }

        return loopsAdded ==
               topology.m_numLoops;
    }

    //-------------------------------------------------------------------------

    static bool DoesGraniteFracturePlaneSplitParent(
        GraniteClosedGeometry const& parentGeometry,
        GranitePoint const&          localPlanePoint,
        GranitePoint const&          unitNormal,
        float                        toleranceM )
    {
        if ( parentGeometry.m_numVertices <=
             0 )
        {
            return false;
        }

        float minSignedDistance =
            1000000.0f;

        float maxSignedDistance =
            -1000000.0f;

        for ( int32_t i = 0;
              i <
              parentGeometry.m_numVertices;
              ++i )
        {
            float const signedDistance =
                GraniteFractureSignedDistance(
                    parentGeometry.m_vertices[i],
                    localPlanePoint,
                    unitNormal );

            minSignedDistance =
                GraniteMin(
                    minSignedDistance,
                    signedDistance );

            maxSignedDistance =
                GraniteMax(
                    maxSignedDistance,
                    signedDistance );
        }

        return minSignedDistance <
                 -toleranceM &&
               maxSignedDistance >
                   toleranceM;
    }

    //-------------------------------------------------------------------------
    // Exact final-visible-mesh split.
    //-------------------------------------------------------------------------

    static bool BuildGraniteExactFractureChildren(
        GraniteClosedGeometry const& parentGeometry,
        GranitePoint const&          localPlanePoint,
        GranitePoint const&          fractureNormal,
        float                        planeToleranceM,
        GraniteClosedGeometry&       primaryGeometry,
        GraniteClosedGeometry&       secondaryGeometry,
        GraniteFractureCutTopology&  cutTopology,
        GraniteFractureRejectionReason& rejectionReason,
        int32_t&                     primaryInheritedTriangleCount,
        int32_t&                     secondaryInheritedTriangleCount,
        int32_t&                     primaryCapTriangleCount,
        int32_t&                     secondaryCapTriangleCount )
    {
        CopyGraniteFractureBodyCharacteristics(
            parentGeometry,
            primaryGeometry );

        CopyGraniteFractureBodyCharacteristics(
            parentGeometry,
            secondaryGeometry );

        float const mergeToleranceM =
            GraniteMax(
                0.000001f,
                planeToleranceM *
                    0.25f );

        // The clipped child boundary, shared cut graph and cap must use one
        // vertex-identity rule. Different merge radii create T-junctions where
        // a graph node represents two still-distinct child boundary vertices.
        float const cutTopologyMergeToleranceM =
            mergeToleranceM;

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              parentGeometry.m_numTriangles;
              ++triangleIndex )
        {
            int32_t const base =
                triangleIndex *
                3;

            uint8_t const index0 =
                parentGeometry.m_triangleIndices[base + 0];

            uint8_t const index1 =
                parentGeometry.m_triangleIndices[base + 1];

            uint8_t const index2 =
                parentGeometry.m_triangleIndices[base + 2];

            GranitePoint inputPoints[3] =
                {
                    parentGeometry.m_vertices[index0],
                    parentGeometry.m_vertices[index1],
                    parentGeometry.m_vertices[index2] };

            float inputDistances[3] =
                {
                    GraniteFractureSignedDistance(
                        inputPoints[0],
                        localPlanePoint,
                        fractureNormal ),
                    GraniteFractureSignedDistance(
                        inputPoints[1],
                        localPlanePoint,
                        fractureNormal ),
                    GraniteFractureSignedDistance(
                        inputPoints[2],
                        localPlanePoint,
                        fractureNormal ) };

            GranitePoint negativePolygon[6];
            GranitePoint positivePolygon[6];

            int32_t const negativeCount =
                ClipGraniteTriangleToFractureHalfSpace(
                    inputPoints,
                    inputDistances,
                    true,
                    localPlanePoint,
                    fractureNormal,
                    planeToleranceM,
                    negativePolygon );

            int32_t const positiveCount =
                ClipGraniteTriangleToFractureHalfSpace(
                    inputPoints,
                    inputDistances,
                    false,
                    localPlanePoint,
                    fractureNormal,
                    planeToleranceM,
                    positivePolygon );

            uint8_t const sourceFaceIndex =
                parentGeometry.m_triangleFaceIndex[triangleIndex];

            GraniteSurfaceFaceClass const surfaceClass =
                parentGeometry.m_triangleSurfaceClass[triangleIndex];

            GraniteFaceOrigin const origin =
                parentGeometry.m_triangleFaceOrigin[triangleIndex];

            float const weatheredWeight =
                parentGeometry.m_triangleWeatheredWeight[triangleIndex];

            int32_t const primaryBefore =
                primaryGeometry.m_numTriangles;

            TriangulateGraniteFractureClippedPolygon(
                primaryGeometry,
                negativePolygon,
                negativeCount,
                sourceFaceIndex,
                surfaceClass,
                origin,
                weatheredWeight,
                mergeToleranceM );

            primaryInheritedTriangleCount +=
                primaryGeometry.m_numTriangles -
                primaryBefore;

            int32_t const secondaryBefore =
                secondaryGeometry.m_numTriangles;

            TriangulateGraniteFractureClippedPolygon(
                secondaryGeometry,
                positivePolygon,
                positiveCount,
                sourceFaceIndex,
                surfaceClass,
                origin,
                weatheredWeight,
                mergeToleranceM );

            secondaryInheritedTriangleCount +=
                secondaryGeometry.m_numTriangles -
                secondaryBefore;

            GranitePoint triangleCutPoints[4];

            int32_t const triangleCutPointCount =
                CollectGraniteTriangleFracturePoints(
                    inputPoints,
                    inputDistances,
                    localPlanePoint,
                    fractureNormal,
                    planeToleranceM,
                    triangleCutPoints );

            if ( triangleCutPointCount == 2 )
            {
                AddGraniteCutTopologySegment(
                    cutTopology,
                    triangleCutPoints[0],
                    triangleCutPoints[1],
                    cutTopologyMergeToleranceM );
            }

        }

        if ( !FinalizeGraniteCutTopology(
                 cutTopology,
                 rejectionReason ) )
        {
            return false;
        }

        if ( !AddGraniteFractureCapsFromTopology(
                 cutTopology,
                 fractureNormal,
                 primaryGeometry,
                 secondaryGeometry,
                 mergeToleranceM,
                 primaryCapTriangleCount,
                 secondaryCapTriangleCount,
                 rejectionReason ) )
        {
            return false;
        }

        if ( primaryGeometry.m_numVertices <
                 4 ||
             secondaryGeometry.m_numVertices <
                 4 ||
             primaryGeometry.m_numTriangles <
                 4 ||
             secondaryGeometry.m_numTriangles <
                 4 )
        {
            rejectionReason =
                GraniteFractureRejectionReason::InsufficientChildMesh;
            return false;
        }

        UpdateGraniteGeometryExtents(
            primaryGeometry );

        UpdateGraniteGeometryExtents(
            secondaryGeometry );

        primaryGeometry.m_measuredMeshVolumeM3 =
            MeasureGraniteClosedGeometryVolume(
                primaryGeometry );

        secondaryGeometry.m_measuredMeshVolumeM3 =
            MeasureGraniteClosedGeometryVolume(
                secondaryGeometry );

        bool const positiveVolume =
            primaryGeometry.m_measuredMeshVolumeM3 >
                0.000001f &&
            secondaryGeometry.m_measuredMeshVolumeM3 >
                0.000001f;

        if ( !positiveVolume )
        {
            rejectionReason =
                GraniteFractureRejectionReason::NonPositiveChildVolume;
        }

        return positiveVolume;
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteTriangleAreaByTriangleIndex(
        GraniteClosedGeometry const& geometry,
        int32_t                      triangleIndex )
    {
        int32_t const base =
            triangleIndex *
            3;

        return MeasureGraniteTriangleArea(
            geometry,
            geometry.m_triangleIndices[base + 0],
            geometry.m_triangleIndices[base + 1],
            geometry.m_triangleIndices[base + 2] );
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteVisibleExteriorArea(
        GraniteClosedGeometry const& geometry,
        bool                         includeFractureCap )
    {
        double area =
            0.0;

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              geometry.m_numTriangles;
              ++triangleIndex )
        {
            bool const isFractureCap =
                geometry.m_triangleFaceIndex[triangleIndex] ==
                s_graniteFractureCapFaceIndex;

            if ( !includeFractureCap &&
                 isFractureCap )
            {
                continue;
            }

            area +=
                double(
                    MeasureGraniteTriangleAreaByTriangleIndex(
                        geometry,
                        triangleIndex ) );
        }

        return float(
            area );
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteFractureCapArea(
        GraniteClosedGeometry const& geometry )
    {
        double area =
            0.0;

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              geometry.m_numTriangles;
              ++triangleIndex )
        {
            if ( geometry.m_triangleFaceIndex[triangleIndex] !=
                 s_graniteFractureCapFaceIndex )
            {
                continue;
            }

            area +=
                double(
                    MeasureGraniteTriangleAreaByTriangleIndex(
                        geometry,
                        triangleIndex ) );
        }

        return float(
            area );
    }

    //-------------------------------------------------------------------------

    static float MeasureMaximumGraniteFractureCapPlaneDeviation(
        GraniteClosedGeometry const& geometry,
        GranitePoint const&          localPlanePoint,
        GranitePoint const&          unitNormal )
    {
        float maximumDeviation =
            0.0f;

        bool usedVertex[GraniteClosedGeometry::s_maxVertices] =
            {
                false };

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              geometry.m_numTriangles;
              ++triangleIndex )
        {
            if ( geometry.m_triangleFaceIndex[triangleIndex] !=
                 s_graniteFractureCapFaceIndex )
            {
                continue;
            }

            int32_t const base =
                triangleIndex *
                3;

            for ( int32_t corner = 0;
                  corner <
                  3;
                  ++corner )
            {
                int32_t const vertexIndex =
                    int32_t(
                        geometry.m_triangleIndices[base +
                                                   corner] );

                if ( usedVertex[vertexIndex] )
                {
                    continue;
                }

                usedVertex[vertexIndex] =
                    true;

                maximumDeviation =
                    GraniteMax(
                        maximumDeviation,
                        GraniteAbs(
                            GraniteFractureSignedDistance(
                                geometry.m_vertices[vertexIndex],
                                localPlanePoint,
                                unitNormal ) ) );
            }
        }

        return maximumDeviation;
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteHalfSpaceViolation(
        GraniteClosedGeometry const& geometry,
        GranitePoint const&          localPlanePoint,
        GranitePoint const&          unitNormal,
        bool                         shouldRemainNegative )
    {
        float maximumViolation =
            0.0f;

        for ( int32_t vertexIndex = 0;
              vertexIndex <
              geometry.m_numVertices;
              ++vertexIndex )
        {
            float const signedDistance =
                GraniteFractureSignedDistance(
                    geometry.m_vertices[vertexIndex],
                    localPlanePoint,
                    unitNormal );

            float const violation =
                shouldRemainNegative
                    ? GraniteMax(
                          signedDistance,
                          0.0f )
                    : GraniteMax(
                          -signedDistance,
                          0.0f );

            maximumViolation =
                GraniteMax(
                    maximumViolation,
                    violation );
        }

        return maximumViolation;
    }

    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------

    static GraniteFractureChildSurfaceReceipt BuildGraniteFractureSurfaceReceipt(
        GraniteClosedGeometry const& geometry )
    {
        GraniteFractureChildSurfaceReceipt receipt;

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              geometry.m_numTriangles;
              ++triangleIndex )
        {
            float const area =
                MeasureGraniteTriangleAreaByTriangleIndex(
                    geometry,
                    triangleIndex );

            bool const isNewFractureCap =
                geometry.m_triangleFaceIndex[triangleIndex] ==
                s_graniteFractureCapFaceIndex;

            GraniteSurfaceFaceClass const surfaceClass =
                geometry.m_triangleSurfaceClass[triangleIndex];

            GraniteFaceOrigin const origin =
                geometry.m_triangleFaceOrigin[triangleIndex];

            if ( isNewFractureCap )
            {
                ++receipt.m_freshFractureFaceCount;

                receipt.m_newFractureAreaM2 +=
                    area;

                receipt.m_hasFreshFractureSurface =
                    true;

                continue;
            }

            switch ( surfaceClass )
            {
                case GraniteSurfaceFaceClass::WeatheredExterior:
                {
                    ++receipt.m_inheritedExteriorFaceCount;

                    receipt.m_inheritedExteriorAreaM2 +=
                        area;

                    receipt.m_hasInheritedExterior =
                        true;

                    break;
                }

                case GraniteSurfaceFaceClass::Transitional:
                {
                    ++receipt.m_transitionalFaceCount;

                    receipt.m_transitionalAreaM2 +=
                        area;

                    if ( origin ==
                             GraniteFaceOrigin::InheritedExterior ||
                         geometry.m_triangleWeatheredWeight[triangleIndex] >
                             0.05f )
                    {
                        receipt.m_hasInheritedExterior =
                            true;
                    }

                    break;
                }

                case GraniteSurfaceFaceClass::FreshFracture:
                default:
                {
                    // This may be a fresh surface inherited from an OLDER
                    // fracture event. It remains inherited exterior geometry
                    // for this transaction and must not be counted as the new
                    // P3C.10A-2 cut cap.
                    ++receipt.m_freshFractureFaceCount;
                    break;
                }
            }
        }

        return receipt;
    }

    //-------------------------------------------------------------------------

    static bool ParentGraniteHasInheritedExterior(
        GraniteClosedGeometry const& parentGeometry )
    {
        if ( parentGeometry.m_surfaceHistory.m_hasInheritedExterior )
        {
            return true;
        }

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              parentGeometry.m_numTriangles;
              ++triangleIndex )
        {
            if ( parentGeometry.m_triangleSurfaceClass[triangleIndex] ==
                     GraniteSurfaceFaceClass::WeatheredExterior ||
                 parentGeometry.m_triangleFaceOrigin[triangleIndex] ==
                     GraniteFaceOrigin::InheritedExterior ||
                 parentGeometry.m_triangleWeatheredWeight[triangleIndex] >
                     0.05f )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    static void FinalizeGraniteFractureChildSurfaceHistory(
        GraniteClosedGeometry& geometry )
    {
        double totalArea =
            0.0;

        double weatheredArea =
            0.0;

        double freshArea =
            0.0;

        double transitionArea =
            0.0;

        int32_t freshCount =
            0;

        int32_t inheritedCount =
            0;

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              geometry.m_numTriangles;
              ++triangleIndex )
        {
            float const area =
                MeasureGraniteTriangleAreaByTriangleIndex(
                    geometry,
                    triangleIndex );

            totalArea +=
                double(
                    area );

            GraniteSurfaceFaceClass const surfaceClass =
                geometry.m_triangleSurfaceClass[triangleIndex];

            if ( surfaceClass ==
                 GraniteSurfaceFaceClass::WeatheredExterior )
            {
                weatheredArea +=
                    double(
                        area );

                ++inheritedCount;
            }
            else if ( surfaceClass ==
                      GraniteSurfaceFaceClass::Transitional )
            {
                transitionArea +=
                    double(
                        area );
            }
            else
            {
                freshArea +=
                    double(
                        area );

                ++freshCount;
            }
        }

        if ( totalArea >
             0.0000001 )
        {
            geometry.m_surfaceHistory.m_weatheredExteriorCoverage =
                float(
                    weatheredArea /
                    totalArea );

            geometry.m_surfaceHistory.m_freshFractureCoverage =
                float(
                    freshArea /
                    totalArea );

            geometry.m_surfaceHistory.m_transitionalCoverage =
                float(
                    transitionArea /
                    totalArea );
        }

        geometry.m_surfaceHistory.m_hasInheritedExterior =
            weatheredArea >
                0.0000001 ||
            transitionArea >
                0.0000001;

        geometry.m_freshFractureFaceCount =
            freshCount;

        geometry.m_inheritedExteriorFaceCount =
            inheritedCount;

        // Geometry-derived summary only. Child fracture meshes no longer own
        // geological parent-face polygons, so major/joint face counters are
        // intentionally not reconstructed from presentation triangles.
        geometry.m_majorFaceCount =
            0;

        geometry.m_primaryJointFaceCount =
            0;

        geometry.m_secondaryJointFaceCount =
            0;
    }

    //-------------------------------------------------------------------------

    static void FinalizeGraniteFractureChildMatter(
        GraniteFractureChild& child,
        uint32_t              massGrams,
        float                 graniteDensityKgPerM3 )
    {
        child.m_massGrams =
            massGrams;

        child.m_massKg =
            float(
                massGrams ) /
            1000.0f;

        child.m_requiredSolidVolumeM3 =
            child.m_massKg /
            graniteDensityKgPerM3;

        child.m_geometry.m_requiredSolidVolumeM3 =
            child.m_requiredSolidVolumeM3;

        child.m_geometry.m_requiredEnvelopeVolumeM3 =
            child.m_requiredSolidVolumeM3;

        child.m_geometry.m_measuredMeshVolumeM3 =
            MeasureGraniteClosedGeometryVolume(
                child.m_geometry );

        child.m_geometry.m_volumeResidualM3 =
            child.m_geometry.m_measuredMeshVolumeM3 -
            child.m_requiredSolidVolumeM3;

        child.m_measuredMeshVolumeM3 =
            child.m_geometry.m_measuredMeshVolumeM3;

        FinalizeGraniteFractureChildSurfaceHistory(
            child.m_geometry );

        child.m_surfaceReceipt =
            BuildGraniteFractureSurfaceReceipt(
                child.m_geometry );

        float const tolerance =
            0.000001f +
            child.m_requiredSolidVolumeM3 *
                0.005f;

        child.m_valid =
            child.m_geometry.m_numVertices >=
                4 &&
            child.m_geometry.m_numTriangles >=
                4 &&
            child.m_measuredMeshVolumeM3 >
                0.000001f &&
            GraniteAbs(
                child.m_geometry.m_volumeResidualM3 ) <=
                tolerance &&
            child.m_surfaceReceipt.m_hasFreshFractureSurface &&
            IsGraniteTriangleMeshClosed(
                child.m_geometry );
    }

    //-------------------------------------------------------------------------

    static GraniteFractureGeometricPartitionReceipt
    BuildGraniteGeometricPartitionReceipt(
        GraniteClosedGeometry const&      parentGeometry,
        GraniteClosedGeometry const&      primaryGeometry,
        GraniteClosedGeometry const&      secondaryGeometry,
        GraniteFractureCutTopology const& cutTopology,
        GranitePoint const&               localPlanePoint,
        GranitePoint const&               fractureNormal,
        float                             planeToleranceM,
        int32_t                           primaryInheritedTriangleCount,
        int32_t                           secondaryInheritedTriangleCount,
        int32_t                           primaryCapTriangleCount,
        int32_t                           secondaryCapTriangleCount )
    {
        GraniteFractureGeometricPartitionReceipt receipt;

        receipt.m_parentVisibleMeshVolumeM3 =
            MeasureGraniteClosedGeometryVolume(
                parentGeometry );

        receipt.m_primaryVisibleMeshVolumeM3 =
            MeasureGraniteClosedGeometryVolume(
                primaryGeometry );

        receipt.m_secondaryVisibleMeshVolumeM3 =
            MeasureGraniteClosedGeometryVolume(
                secondaryGeometry );

        receipt.m_childVisibleMeshVolumeSumM3 =
            receipt.m_primaryVisibleMeshVolumeM3 +
            receipt.m_secondaryVisibleMeshVolumeM3;

        receipt.m_visibleMeshUnionResidualM3 =
            receipt.m_childVisibleMeshVolumeSumM3 -
            receipt.m_parentVisibleMeshVolumeM3;

        float const parentVolumeDenominator =
            GraniteMax(
                receipt.m_parentVisibleMeshVolumeM3,
                0.000001f );

        receipt.m_visibleMeshUnionRelativeResidual =
            GraniteAbs(
                receipt.m_visibleMeshUnionResidualM3 ) /
            parentVolumeDenominator;

        receipt.m_parentExteriorAreaM2 =
            MeasureGraniteVisibleExteriorArea(
                parentGeometry,
                true );

        receipt.m_childInheritedExteriorAreaSumM2 =
            MeasureGraniteVisibleExteriorArea(
                primaryGeometry,
                false ) +
            MeasureGraniteVisibleExteriorArea(
                secondaryGeometry,
                false );

        receipt.m_exteriorAreaResidualM2 =
            receipt.m_childInheritedExteriorAreaSumM2 -
            receipt.m_parentExteriorAreaM2;

        float const parentAreaDenominator =
            GraniteMax(
                receipt.m_parentExteriorAreaM2,
                0.000001f );

        receipt.m_exteriorAreaRelativeResidual =
            GraniteAbs(
                receipt.m_exteriorAreaResidualM2 ) /
            parentAreaDenominator;

        receipt.m_primaryCutAreaM2 =
            MeasureGraniteFractureCapArea(
                primaryGeometry );

        receipt.m_secondaryCutAreaM2 =
            MeasureGraniteFractureCapArea(
                secondaryGeometry );

        receipt.m_cutAreaResidualM2 =
            receipt.m_primaryCutAreaM2 -
            receipt.m_secondaryCutAreaM2;

        float const cutAreaDenominator =
            GraniteMax(
                GraniteMax(
                    receipt.m_primaryCutAreaM2,
                    receipt.m_secondaryCutAreaM2 ),
                0.000001f );

        receipt.m_cutAreaRelativeResidual =
            GraniteAbs(
                receipt.m_cutAreaResidualM2 ) /
            cutAreaDenominator;

        receipt.m_maxCutPlaneDeviationM =
            GraniteMax(
                MeasureMaximumGraniteFractureCapPlaneDeviation(
                    primaryGeometry,
                    localPlanePoint,
                    fractureNormal ),
                MeasureMaximumGraniteFractureCapPlaneDeviation(
                    secondaryGeometry,
                    localPlanePoint,
                    fractureNormal ) );

        receipt.m_primaryHalfSpaceViolationM =
            MeasureGraniteHalfSpaceViolation(
                primaryGeometry,
                localPlanePoint,
                fractureNormal,
                true );

        receipt.m_secondaryHalfSpaceViolationM =
            MeasureGraniteHalfSpaceViolation(
                secondaryGeometry,
                localPlanePoint,
                fractureNormal,
                false );

        receipt.m_cutLoopCount =
            cutTopology.m_numLoops;

        receipt.m_cutVertexCount =
            cutTopology.m_numPoints;

        receipt.m_primaryInheritedTriangleCount =
            primaryInheritedTriangleCount;

        receipt.m_secondaryInheritedTriangleCount =
            secondaryInheritedTriangleCount;

        receipt.m_primaryCapTriangleCount =
            primaryCapTriangleCount;

        receipt.m_secondaryCapTriangleCount =
            secondaryCapTriangleCount;

        float const volumeTolerance =
            GraniteMax(
                0.0000005f,
                receipt.m_parentVisibleMeshVolumeM3 *
                    0.0001f );

        float const exteriorAreaTolerance =
            GraniteMax(
                0.000005f,
                receipt.m_parentExteriorAreaM2 *
                    0.0002f );

        float const cutAreaTolerance =
            GraniteMax(
                0.000005f,
                cutAreaDenominator *
                    0.0002f );

        float const planeTolerance =
            GraniteMax(
                0.000002f,
                planeToleranceM *
                    0.10f );

        float const ownershipTolerance =
            GraniteMax(
                0.000005f,
                planeToleranceM *
                    1.10f );

        receipt.m_visibleMeshUnionPass =
            GraniteAbs(
                receipt.m_visibleMeshUnionResidualM3 ) <=
            volumeTolerance;

        receipt.m_exteriorPartitionPass =
            GraniteAbs(
                receipt.m_exteriorAreaResidualM2 ) <=
            exteriorAreaTolerance;

        receipt.m_sharedCutCoincidencePass =
            cutTopology.m_manifold &&
            receipt.m_cutLoopCount >
                0 &&
            receipt.m_primaryCapTriangleCount >
                0 &&
            receipt.m_secondaryCapTriangleCount >
                0 &&
            GraniteAbs(
                receipt.m_cutAreaResidualM2 ) <=
                cutAreaTolerance &&
            receipt.m_maxCutPlaneDeviationM <=
                planeTolerance;

        receipt.m_halfSpaceOwnershipPass =
            receipt.m_primaryHalfSpaceViolationM <=
                ownershipTolerance &&
            receipt.m_secondaryHalfSpaceViolationM <=
                ownershipTolerance;

        receipt.m_closedChildTopologyPass =
            IsGraniteTriangleMeshClosed(
                primaryGeometry ) &&
            IsGraniteTriangleMeshClosed(
                secondaryGeometry );

        receipt.m_pass =
            receipt.m_visibleMeshUnionPass &&
            receipt.m_exteriorPartitionPass &&
            receipt.m_sharedCutCoincidencePass &&
            receipt.m_halfSpaceOwnershipPass &&
            receipt.m_closedChildTopologyPass;

        return receipt;
    }

    //-------------------------------------------------------------------------
    // P3C.10A / P3C.10A-2 — actual Granite fracture transaction
    //-------------------------------------------------------------------------

    GraniteFractureTransactionResult FractureGraniteClosedGeometry(
        GraniteClosedGeometry const&        parentGeometry,
        GraniteClosedGeometryRequest const& parentRequest,
        GraniteFractureRequest const&       fractureRequest )
    {
        GraniteFractureTransactionResult result;

        result.m_parentTopologyFamily =
            parentGeometry.m_topologyFamily;

        result.m_eventID =
            fractureRequest.m_eventID;

        result.m_parentBodyID =
            fractureRequest.m_parentBodyID;

        result.m_parentProvenanceID =
            fractureRequest.m_parentProvenanceID;

        result.m_geologicalAncestryID =
            fractureRequest.m_geologicalAncestryID;

        result.m_plane =
            fractureRequest.m_plane;

        result.m_parentMassGrams =
            parentRequest.m_massGrams;

        result.m_parentMassKg =
            float(
                parentRequest.m_massGrams ) /
            1000.0f;

        result.m_parentMeasuredMeshVolumeM3 =
            MeasureGraniteClosedGeometryVolume(
                parentGeometry );

        MatterVolumeMetrics const parentMetrics =
            CalculateMatterVolume(
                ProvenanceMaterialID::Granite,
                parentRequest.m_bodyState,
                parentRequest.m_massGrams );

        result.m_parentSolidVolumeM3 =
            parentMetrics.m_solidVolumeM3;

        if ( parentRequest.m_massGrams <
                 2 ||
             parentGeometry.m_numVertices <
                 4 ||
             parentGeometry.m_numTriangles <
                 4 )
        {
            result.m_rejectionReason =
                GraniteFractureRejectionReason::InvalidParentInput;
            return result;
        }

        if ( !IsGraniteTriangleMeshClosed(
                 parentGeometry ) )
        {
#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
            if ( !parentGeometry.m_seedTriangleMeshClosed )
            {
                result.m_rejectionReason =
                    GraniteFractureRejectionReason::ParentSeedTriangleMeshNotClosed;
            }
            else if ( !parentGeometry.m_debiasedTriangleMeshClosed )
            {
                result.m_rejectionReason =
                    GraniteFractureRejectionReason::ParentDebiasedTriangleMeshNotClosed;
            }
            else if ( !parentGeometry.m_surfaceHistoryTriangleMeshClosed )
            {
                result.m_rejectionReason =
                    GraniteFractureRejectionReason::ParentSurfaceHistoryTriangleMeshNotClosed;
            }
            else if ( !parentGeometry.m_structuredArticulationTriangleMeshClosed )
            {
                result.m_rejectionReason =
                    GraniteFractureRejectionReason::ParentStructuredArticulationTriangleMeshNotClosed;
            }
            else
            {
                result.m_rejectionReason =
                    GraniteFractureRejectionReason::ParentTriangleMeshNotClosed;
            }
#else
            result.m_rejectionReason =
                GraniteFractureRejectionReason::ParentTriangleMeshNotClosed;
#endif
            return result;
        }

        GranitePoint const fractureNormal =
            GraniteNormalize(
                fractureRequest.m_plane.m_normal );

        if ( GraniteLength(
                 fractureNormal ) <
             0.999f )
        {
            result.m_rejectionReason =
                GraniteFractureRejectionReason::InvalidPlaneNormal;
            return result;
        }

        GranitePoint const localPlanePoint =
            ConvertGraniteWorldPointToBodyLocal(
                fractureRequest.m_plane.m_pointOnPlane,
                parentRequest );

        float const planeToleranceM =
            GraniteMax(
                fractureRequest.m_planeToleranceM,
                0.00001f );

        if ( !DoesGraniteFracturePlaneSplitParent(
                 parentGeometry,
                 localPlanePoint,
                 fractureNormal,
                 planeToleranceM ) )
        {
            result.m_rejectionReason =
                GraniteFractureRejectionReason::PlaneDoesNotSplitParent;
            return result;
        }

        GraniteClosedGeometry      primaryGeometry;
        GraniteClosedGeometry      secondaryGeometry;
        GraniteFractureCutTopology cutTopology;

        int32_t primaryInheritedTriangleCount =
            0;

        int32_t secondaryInheritedTriangleCount =
            0;

        int32_t primaryCapTriangleCount =
            0;

        int32_t secondaryCapTriangleCount =
            0;

        if ( !BuildGraniteExactFractureChildren(
                 parentGeometry,
                 localPlanePoint,
                 fractureNormal,
                 planeToleranceM,
                 primaryGeometry,
                 secondaryGeometry,
                 cutTopology,
                 result.m_rejectionReason,
                 primaryInheritedTriangleCount,
                 secondaryInheritedTriangleCount,
                 primaryCapTriangleCount,
                 secondaryCapTriangleCount ) )
        {
            if ( result.m_rejectionReason ==
                 GraniteFractureRejectionReason::None )
            {
                result.m_rejectionReason =
                    GraniteFractureRejectionReason::ChildConstructionFailed;
            }
            return result;
        }

        float const primaryVisibleVolume =
            MeasureGraniteClosedGeometryVolume(
                primaryGeometry );

        float const secondaryVisibleVolume =
            MeasureGraniteClosedGeometryVolume(
                secondaryGeometry );

        float const visibleVolumeSum =
            primaryVisibleVolume +
            secondaryVisibleVolume;

        if ( primaryVisibleVolume <=
                 0.000001f ||
             secondaryVisibleVolume <=
                 0.000001f ||
             visibleVolumeSum <=
                 0.000001f )
        {
            result.m_rejectionReason =
                GraniteFractureRejectionReason::NonPositiveChildVolume;
            return result;
        }

        float const primaryFraction =
            GraniteClamp01(
                primaryVisibleVolume /
                visibleVolumeSum );

        //-------------------------------------------------------------------------
        // Exact integer-gram partition.
        //
        // Geometry determines the mass fraction; integer matter authority then
        // assigns the primary rounded grams and gives the exact remainder to
        // the secondary child.
        //
        // Therefore:
        //
        //     primary grams + secondary grams == parent grams
        //
        // exactly.
        //-------------------------------------------------------------------------

        uint32_t primaryMassGrams =
            uint32_t(
                double(
                    parentRequest.m_massGrams ) *
                    double(
                        primaryFraction ) +
                0.5 );

        if ( primaryMassGrams <
             1 )
        {
            primaryMassGrams =
                1;
        }

        if ( primaryMassGrams >=
             parentRequest.m_massGrams )
        {
            primaryMassGrams =
                parentRequest.m_massGrams -
                1;
        }

        uint32_t const secondaryMassGrams =
            parentRequest.m_massGrams -
            primaryMassGrams;

        MaterialGeometryProfile const graniteMatterProfile =
            GetMaterialGeometryProfile(
                ProvenanceMaterialID::Granite );

        float const graniteDensityKgPerM3 =
            graniteMatterProfile.m_intrinsicSolidDensityKgPerM3;

        GraniteFractureChild& primary =
            result.m_children[result.m_numChildren++];

        primary.m_role =
            GraniteFractureChildRole::Primary;

        primary.m_bodyID =
            fractureRequest.m_primaryChildBodyID;

        primary.m_parentBodyID =
            fractureRequest.m_parentBodyID;

        primary.m_provenanceID =
            fractureRequest.m_parentProvenanceID;

        primary.m_geologicalAncestryID =
            fractureRequest.m_geologicalAncestryID;

        primary.m_parentVolumeFraction =
            primaryFraction;

        primary.m_geometry =
            primaryGeometry;

        FinalizeGraniteFractureChildMatter(
            primary,
            primaryMassGrams,
            graniteDensityKgPerM3 );

        GraniteFractureChild& secondary =
            result.m_children[result.m_numChildren++];

        secondary.m_role =
            GraniteFractureChildRole::Secondary;

        secondary.m_bodyID =
            fractureRequest.m_secondaryChildBodyID;

        secondary.m_parentBodyID =
            fractureRequest.m_parentBodyID;

        secondary.m_provenanceID =
            fractureRequest.m_parentProvenanceID;

        secondary.m_geologicalAncestryID =
            fractureRequest.m_geologicalAncestryID;

        secondary.m_parentVolumeFraction =
            1.0f -
            primaryFraction;

        secondary.m_geometry =
            secondaryGeometry;

        FinalizeGraniteFractureChildMatter(
            secondary,
            secondaryMassGrams,
            graniteDensityKgPerM3 );

        result.m_numPrincipalChildren =
            2;

        // P3C.10A-2 remains a two-principal-child proof. Chip generation is
        // deferred to P3C.10C so a failed chip path can never obscure the
        // geometric partition certificate.
        result.m_numChipChildren =
            0;

        result.m_totalChildMassGrams =
            uint64_t(
                primary.m_massGrams ) +
            uint64_t(
                secondary.m_massGrams );

        result.m_totalChildSolidVolumeM3 =
            primary.m_requiredSolidVolumeM3 +
            secondary.m_requiredSolidVolumeM3;

        result.m_totalChildMeasuredMeshVolumeM3 =
            primary.m_measuredMeshVolumeM3 +
            secondary.m_measuredMeshVolumeM3;

        result.m_massResidualGrams =
            int64_t(
                result.m_totalChildMassGrams ) -
            int64_t(
                result.m_parentMassGrams );

        result.m_solidVolumeResidualM3 =
            result.m_totalChildSolidVolumeM3 -
            result.m_parentSolidVolumeM3;

        // P3C.10A-2 geometric residual compares like with like:
        //
        //     child visible mesh sum - parent visible mesh
        //
        // The authoritative solid-volume receipt remains separate above.
        result.m_meshVolumeResidualM3 =
            result.m_totalChildMeasuredMeshVolumeM3 -
            result.m_parentMeasuredMeshVolumeM3;

        float const massDenominator =
            GraniteMax(
                result.m_parentMassKg,
                0.001f );

        result.m_massRelativeResidual =
            GraniteAbs(
                float(
                    result.m_massResidualGrams ) /
                1000.0f ) /
            massDenominator;

        float const volumeDenominator =
            GraniteMax(
                result.m_parentSolidVolumeM3,
                0.000001f );

        result.m_solidVolumeRelativeResidual =
            GraniteAbs(
                result.m_solidVolumeResidualM3 ) /
            volumeDenominator;

        result.m_massConservationPass =
            result.m_massResidualGrams ==
            0;

        float const solidVolumeTolerance =
            0.0000005f +
            result.m_parentSolidVolumeM3 *
                0.00005f;

        result.m_solidVolumeConservationPass =
            GraniteAbs(
                result.m_solidVolumeResidualM3 ) <=
            solidVolumeTolerance;

        result.m_geometricPartition =
            BuildGraniteGeometricPartitionReceipt(
                parentGeometry,
                primary.m_geometry,
                secondary.m_geometry,
                cutTopology,
                localPlanePoint,
                fractureNormal,
                planeToleranceM,
                primaryInheritedTriangleCount,
                secondaryInheritedTriangleCount,
                primaryCapTriangleCount,
                secondaryCapTriangleCount );

        result.m_geometricPartitionPass =
            result.m_geometricPartition.m_pass;

        result.m_childGeometryPass =
            primary.m_valid &&
            secondary.m_valid &&
            result.m_geometricPartition.m_closedChildTopologyPass;

        bool const parentHadInheritedExterior =
            ParentGraniteHasInheritedExterior(
                parentGeometry );

        result.m_inheritedExteriorPreserved =
            primary.m_surfaceReceipt.m_hasInheritedExterior ||
            secondary.m_surfaceReceipt.m_hasInheritedExterior;

        result.m_bothPrincipalChildrenHaveFreshBreak =
            primary.m_surfaceReceipt.m_hasFreshFractureSurface &&
            secondary.m_surfaceReceipt.m_hasFreshFractureSurface;

        result.m_surfaceHistoryPass =
            result.m_bothPrincipalChildrenHaveFreshBreak &&
            ( !parentHadInheritedExterior ||
              result.m_inheritedExteriorPreserved );

        result.m_valid =
            result.m_massConservationPass &&
            result.m_solidVolumeConservationPass &&
            result.m_childGeometryPass &&
            result.m_surfaceHistoryPass &&
            result.m_geometricPartitionPass;

        if ( !result.m_valid )
        {
            result.m_rejectionReason =
                GraniteFractureRejectionReason::CertificateGateFailed;
        }

        EE_ASSERT(
            result.m_massConservationPass );

        EE_ASSERT(
            result.m_solidVolumeConservationPass );

        EE_ASSERT(
            result.m_childGeometryPass );

        EE_ASSERT(
            result.m_surfaceHistoryPass );

        EE_ASSERT(
            result.m_geometricPartitionPass );

        return result;
    }
}
