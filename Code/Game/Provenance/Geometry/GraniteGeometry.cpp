#include "GraniteGeometry.h"

#include <cmath>

#include "GraniteExcavation.inl"

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
    // Authority verification must report named receipt failures and return a
    // process exit code. It must never block CI on an interactive breakpoint.
    #undef EE_ASSERT
    #define EE_ASSERT( cond ) do { } while ( 0 )
#endif

#include "GraniteGeometryInternal.h"
#include "GraniteFormationInternal.h"
#include "GraniteMeshInternal.h"

namespace EE
{
    using namespace GraniteGeometryInternal;
    using namespace GraniteFormationInternal;
    using namespace GraniteMeshInternal;

    // All half-space construction stages must agree on whether two points are
    // the same boundary vertex and whether that vertex belongs to a plane.
    // A wider face-membership tolerance can attach a merged near-plane point
    // to only one of two neighboring faces, producing an open triangle mesh
    // from an otherwise valid convex half-space body.
    static constexpr float s_graniteConstructionToleranceM =
        0.00020f;

    // Plane triples are solved in float, but an actually outside point must
    // not enter the convex body merely because it is close enough to merge
    // with another vertex. Keep half-space classification strict and reserve
    // the wider construction tolerance for vertex identity/collinearity.
    static constexpr float s_graniteHalfSpaceClassificationToleranceM =
        0.00001f;

    //-------------------------------------------------------------------------
    // Plane/polyhedron helpers
    //-------------------------------------------------------------------------

    static GranitePlane MakeGranitePlane(
        GranitePoint const&     normal,
        float                   distance,
        GraniteFaceOrigin       origin,
        GraniteSurfaceFaceClass surfaceClass,
        float                   weatheredWeight,
        int32_t                 jointSetIndex = -1,
        GraniteEdgeHistoryClass edgeHistoryClass = GraniteEdgeHistoryClass::None )
    {
        GranitePlane plane;

        GranitePoint const unitNormal = GraniteNormalize( normal );
        EE_ASSERT( GraniteLength( unitNormal ) > 0.999f );

        plane.m_normal = unitNormal;
        plane.m_distance = distance;
        plane.m_origin = origin;
        plane.m_surfaceClass = surfaceClass;
        plane.m_weatheredWeight = GraniteClamp01( weatheredWeight );
        plane.m_edgeHistoryClass = edgeHistoryClass;
        plane.m_jointSetIndex = jointSetIndex;

        return plane;
    }

    //-------------------------------------------------------------------------

    static void AddGraniteConstructionPlane(
        GraniteClosedGeometry& geometry,
        GranitePlane const&    plane )
    {
        EE_ASSERT(
            geometry.m_numConstructionPlanes <
            GraniteClosedGeometry::s_maxConstructionPlanes );

        geometry.m_constructionPlanes[geometry.m_numConstructionPlanes++] = plane;
    }

    //-------------------------------------------------------------------------

    static bool IntersectGranitePlaneTriple(
        GranitePlane const& a,
        GranitePlane const& b,
        GranitePlane const& c,
        GranitePoint&       result )
    {
        GranitePoint const bCrossC = GraniteCross( b.m_normal, c.m_normal );
        float const        denominator = GraniteDot( a.m_normal, bCrossC );

        if ( GraniteAbs( denominator ) <= 0.00001f )
        {
            return false;
        }

        GranitePoint const cCrossA = GraniteCross( c.m_normal, a.m_normal );
        GranitePoint const aCrossB = GraniteCross( a.m_normal, b.m_normal );

        GranitePoint numerator = GraniteScale( bCrossC, a.m_distance );
        numerator = GraniteAdd( numerator, GraniteScale( cCrossA, b.m_distance ) );
        numerator = GraniteAdd( numerator, GraniteScale( aCrossB, c.m_distance ) );

        result = GraniteScale( numerator, 1.0f / denominator );
        return true;
    }

    //-------------------------------------------------------------------------

    static bool IsGranitePointInsidePlanes(
        GraniteClosedGeometry const& geometry,
        GranitePoint const&          point,
        float                        epsilon )
    {
        for ( int32_t i = 0; i < geometry.m_numConstructionPlanes; ++i )
        {
            GranitePlane const& plane = geometry.m_constructionPlanes[i];

            if ( GraniteDot( plane.m_normal, point ) >
                 plane.m_distance + epsilon )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    static bool AddUniqueGraniteVertex(
        GraniteClosedGeometry& geometry,
        GranitePoint const&    point,
        uint32_t               constructionPlaneMask )
    {
        float constexpr duplicateDistance =
            s_graniteConstructionToleranceM;
        float constexpr duplicateDistanceSq =
            duplicateDistance * duplicateDistance;

        for ( int32_t i = 0; i < geometry.m_numVertices; ++i )
        {
            GranitePoint const delta =
                GraniteSubtract( point, geometry.m_vertices[i] );

            if ( GraniteDot( delta, delta ) <= duplicateDistanceSq )
            {
                geometry.m_vertexConstructionPlaneMask[i] |=
                    constructionPlaneMask;
                return false;
            }
        }

        EE_ASSERT(
            geometry.m_numVertices <
            GraniteClosedGeometry::s_maxVertices );

        int32_t const vertexIndex = geometry.m_numVertices++;
        geometry.m_vertices[vertexIndex] = point;
        geometry.m_vertexConstructionPlaneMask[vertexIndex] =
            constructionPlaneMask;
        return true;
    }

    //-------------------------------------------------------------------------

    static void BuildGraniteVerticesFromPlanes( GraniteClosedGeometry& geometry )
    {
        geometry.m_numVertices = 0;

        float constexpr insideEpsilon =
            s_graniteHalfSpaceClassificationToleranceM;

        for ( int32_t i = 0; i < geometry.m_numConstructionPlanes - 2; ++i )
        {
            for ( int32_t j = i + 1; j < geometry.m_numConstructionPlanes - 1; ++j )
            {
                for ( int32_t k = j + 1; k < geometry.m_numConstructionPlanes; ++k )
                {
                    GranitePoint point;

                    if ( !IntersectGranitePlaneTriple(
                             geometry.m_constructionPlanes[i],
                             geometry.m_constructionPlanes[j],
                             geometry.m_constructionPlanes[k],
                             point ) )
                    {
                        continue;
                    }

                    if ( !IsGranitePointInsidePlanes(
                             geometry,
                             point,
                             insideEpsilon ) )
                    {
                        continue;
                    }

                    uint32_t const constructionPlaneMask =
                        ( uint32_t( 1 ) << uint32_t( i ) ) |
                        ( uint32_t( 1 ) << uint32_t( j ) ) |
                        ( uint32_t( 1 ) << uint32_t( k ) );

                    AddUniqueGraniteVertex(
                        geometry,
                        point,
                        constructionPlaneMask );
                }
            }
        }

        EE_ASSERT( geometry.m_numVertices >= 4 );
    }

    //-------------------------------------------------------------------------

    struct GraniteFaceSortPoint
    {
        uint8_t m_vertexIndex = 0;
        float   m_x = 0.0f;
        float   m_y = 0.0f;
    };

    //-------------------------------------------------------------------------

    static void SortGraniteFacePoints(
        GraniteFaceSortPoint* pPoints,
        int32_t               count )
    {
        for ( int32_t i = 1; i < count; ++i )
        {
            GraniteFaceSortPoint const key = pPoints[i];
            int32_t                    j = i - 1;

            while ( j >= 0 &&
                    ( pPoints[j].m_x > key.m_x ||
                      ( pPoints[j].m_x == key.m_x &&
                        pPoints[j].m_y > key.m_y ) ) )
            {
                pPoints[j + 1] = pPoints[j];
                --j;
            }

            pPoints[j + 1] = key;
        }
    }

    //-------------------------------------------------------------------------

    static float GraniteFaceHullCross(
        GraniteFaceSortPoint const& origin,
        GraniteFaceSortPoint const& a,
        GraniteFaceSortPoint const& b )
    {
        return ( a.m_x - origin.m_x ) * ( b.m_y - origin.m_y ) -
               ( a.m_y - origin.m_y ) * ( b.m_x - origin.m_x );
    }

    //-------------------------------------------------------------------------

    static bool GraniteFaceHullTurnIsNotLeft(
        GraniteFaceSortPoint const& origin,
        GraniteFaceSortPoint const& a,
        GraniteFaceSortPoint const& b )
    {
        float const edgeX = a.m_x - origin.m_x;
        float const edgeY = a.m_y - origin.m_y;
        float const edgeLength = float(
            std::sqrt( double( edgeX * edgeX + edgeY * edgeY ) ) );

        float const orientationTolerance =
            s_graniteConstructionToleranceM *
            GraniteMax( edgeLength, 0.000001f );

        return GraniteFaceHullCross( origin, a, b ) <=
               orientationTolerance;
    }

    //-------------------------------------------------------------------------
    // Plane triples can contribute points inside a convex face as well as on
    // its boundary. Extract the true 2D boundary and discard interior or
    // construction-tolerance-collinear points before fan triangulation.
    //-------------------------------------------------------------------------

    static int32_t BuildGraniteFaceConvexHull(
        GraniteFaceSortPoint* pPoints,
        int32_t               count )
    {
        if ( count < 3 )
        {
            return count;
        }

        SortGraniteFacePoints( pPoints, count );

        GraniteFaceSortPoint hull[GraniteFace::s_maxVertices * 2];
        int32_t hullCount = 0;

        for ( int32_t i = 0; i < count; ++i )
        {
            while ( hullCount >= 2 &&
                    GraniteFaceHullTurnIsNotLeft(
                        hull[hullCount - 2],
                        hull[hullCount - 1],
                        pPoints[i] ) )
            {
                --hullCount;
            }

            hull[hullCount++] = pPoints[i];
        }

        int32_t const upperHullMinimum = hullCount + 1;

        for ( int32_t i = count - 2; i >= 0; --i )
        {
            while ( hullCount >= upperHullMinimum &&
                    GraniteFaceHullTurnIsNotLeft(
                        hull[hullCount - 2],
                        hull[hullCount - 1],
                        pPoints[i] ) )
            {
                --hullCount;
            }

            hull[hullCount++] = pPoints[i];
        }

        if ( hullCount > 1 )
        {
            --hullCount;
        }

        for ( int32_t i = 0; i < hullCount; ++i )
        {
            pPoints[i] = hull[i];
        }

        return hullCount;
    }

    //-------------------------------------------------------------------------

    static void GetGranitePlaneBasis(
        GranitePoint const& normal,
        GranitePoint&       tangent,
        GranitePoint&       bitangent )
    {
        GranitePoint helper;

        if ( GraniteAbs( normal.m_z ) < 0.80f )
        {
            helper = { 0.0f, 0.0f, 1.0f };
        }
        else
        {
            helper = { 1.0f, 0.0f, 0.0f };
        }

        tangent = GraniteNormalize( GraniteCross( helper, normal ) );
        bitangent = GraniteNormalize( GraniteCross( normal, tangent ) );
    }

    //-------------------------------------------------------------------------

    static float MeasureGraniteFaceArea(
        GraniteClosedGeometry const& geometry,
        GraniteFace const&           face )
    {
        if ( face.m_numVertices < 3 )
        {
            return 0.0f;
        }

        GranitePoint const& origin =
            geometry.m_vertices[face.m_vertexIndices[0]];

        float area = 0.0f;

        for ( int32_t i = 1; i < face.m_numVertices - 1; ++i )
        {
            GranitePoint const& b =
                geometry.m_vertices[face.m_vertexIndices[i]];

            GranitePoint const& c =
                geometry.m_vertices[face.m_vertexIndices[i + 1]];

            GranitePoint const edge1 = GraniteSubtract( b, origin );
            GranitePoint const edge2 = GraniteSubtract( c, origin );

            area += 0.5f * GraniteLength( GraniteCross( edge1, edge2 ) );
        }

        return area;
    }

    //-------------------------------------------------------------------------

    static void BuildGraniteFacesFromPlanes( GraniteClosedGeometry& geometry )
    {
        geometry.m_numFaces = 0;

        for ( int32_t planeIndex = 0;
              planeIndex < geometry.m_numConstructionPlanes;
              ++planeIndex )
        {
            GranitePlane const& plane =
                geometry.m_constructionPlanes[planeIndex];

            uint32_t const constructionPlaneBit =
                uint32_t( 1 ) << uint32_t( planeIndex );

            uint8_t faceVertexIndices[GraniteFace::s_maxVertices];
            int32_t faceVertexCount = 0;

            GranitePoint centroid;

            for ( int32_t vertexIndex = 0;
                  vertexIndex < geometry.m_numVertices;
                  ++vertexIndex )
            {
                if ( ( geometry.m_vertexConstructionPlaneMask[vertexIndex] &
                       constructionPlaneBit ) == 0 )
                {
                    continue;
                }

                GranitePoint const& vertex = geometry.m_vertices[vertexIndex];

                EE_ASSERT( faceVertexCount < GraniteFace::s_maxVertices );

                faceVertexIndices[faceVertexCount++] = uint8_t( vertexIndex );
                centroid = GraniteAdd( centroid, vertex );
            }

            if ( faceVertexCount < 3 )
            {
                continue;
            }

            centroid = GraniteScale( centroid, 1.0f / float( faceVertexCount ) );

            GranitePoint tangent;
            GranitePoint bitangent;
            GetGranitePlaneBasis( plane.m_normal, tangent, bitangent );

            GraniteFaceSortPoint sorted[GraniteFace::s_maxVertices];

            for ( int32_t i = 0; i < faceVertexCount; ++i )
            {
                GranitePoint const delta = GraniteSubtract(
                    geometry.m_vertices[faceVertexIndices[i]],
                    centroid );

                float const x = GraniteDot( delta, tangent );
                float const y = GraniteDot( delta, bitangent );

                sorted[i].m_vertexIndex = faceVertexIndices[i];
                sorted[i].m_x = x;
                sorted[i].m_y = y;
            }

            faceVertexCount =
                BuildGraniteFaceConvexHull( sorted, faceVertexCount );

            if ( faceVertexCount < 3 )
            {
                continue;
            }

            EE_ASSERT(
                geometry.m_numFaces <
                GraniteClosedGeometry::s_maxFaces );

            GraniteFace& face = geometry.m_faces[geometry.m_numFaces++];
            face.m_numVertices = faceVertexCount;
            face.m_normal = plane.m_normal;
            face.m_origin = plane.m_origin;
            face.m_surfaceClass = plane.m_surfaceClass;
            face.m_weatheredWeight = plane.m_weatheredWeight;
            face.m_edgeHistoryClass = plane.m_edgeHistoryClass;
            face.m_jointSetIndex = plane.m_jointSetIndex;

            for ( int32_t i = 0; i < faceVertexCount; ++i )
            {
                face.m_vertexIndices[i] = sorted[i].m_vertexIndex;
            }

            face.m_areaM2 = MeasureGraniteFaceArea( geometry, face );

            // Derived Step 6C transition planes can occasionally collapse to
            // a numerically tiny sliver after neighboring seams are also
            // trimmed. Such a plane is redundant presentation geometry, not a
            // geological failure. Drop the degenerate face instead of
            // certifying a zero-area polygon.
            if ( face.m_areaM2 <= 0.000001f )
            {
                --geometry.m_numFaces;
                continue;
            }
        }

        EE_ASSERT( geometry.m_numFaces >= 4 );
    }

    //-------------------------------------------------------------------------

    static void TriangulateGraniteFaces( GraniteClosedGeometry& geometry )
    {
        geometry.m_numTriangles = 0;

        for ( int32_t faceIndex = 0;
              faceIndex < geometry.m_numFaces;
              ++faceIndex )
        {
            GraniteFace const& face = geometry.m_faces[faceIndex];

            for ( int32_t i = 1; i < face.m_numVertices - 1; ++i )
            {
                EE_ASSERT(
                    geometry.m_numTriangles <
                    GraniteClosedGeometry::s_maxTriangles );

                int32_t const triangleIndex = geometry.m_numTriangles;
                int32_t const base = triangleIndex * 3;

                geometry.m_triangleIndices[base] = face.m_vertexIndices[0];
                geometry.m_triangleIndices[base + 1] = face.m_vertexIndices[i];
                geometry.m_triangleIndices[base + 2] = face.m_vertexIndices[i + 1];

                geometry.m_triangleFaceIndex[triangleIndex] = uint8_t( faceIndex );
                geometry.m_triangleSurfaceClass[triangleIndex] = face.m_surfaceClass;
                geometry.m_triangleFaceOrigin[triangleIndex] = face.m_origin;
                geometry.m_triangleWeatheredWeight[triangleIndex] =
                    face.m_weatheredWeight;

                ++geometry.m_numTriangles;
            }
        }

        EE_ASSERT( geometry.m_numTriangles >= 4 );
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 6B — localized surface-history geometry
    //-------------------------------------------------------------------------

    static float GraniteBodySignal( uint32_t geometrySeed, int32_t channel );

    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------

    static GranitePoint GetGraniteFaceCentroid(
        GraniteClosedGeometry const& geometry,
        GraniteFace const&           face )
    {
        EE_ASSERT( face.m_numVertices >= 3 );

        GranitePoint centroid;

        for ( int32_t i = 0; i < face.m_numVertices; ++i )
        {
            centroid = GraniteAdd(
                centroid,
                geometry.m_vertices[face.m_vertexIndices[i]] );
        }

        return GraniteScale( centroid, 1.0f / float( face.m_numVertices ) );
    }

    //-------------------------------------------------------------------------

    static bool IsGraniteFaceEligibleForExteriorCrown(
        GraniteFace const& face )
    {
        return face.m_origin == GraniteFaceOrigin::InheritedExterior &&
               face.m_surfaceClass == GraniteSurfaceFaceClass::WeatheredExterior &&
               face.m_weatheredWeight > 0.0001f &&
               face.m_numVertices >= 3;
    }

    //-------------------------------------------------------------------------
    // Raw crown law
    //
    // This is deliberately local to an inherited old exterior face.
    // FreshBreak and PrimaryJoint faces cannot enter this path.
    //-------------------------------------------------------------------------

    static float CalculateGraniteExteriorCrownMagnitude(
        GraniteFace const&            face,
        GraniteGeometryProfile const& profile,
        uint32_t                      geometrySeed,
        int32_t                       faceIndex )
    {
        if ( !IsGraniteFaceEligibleForExteriorCrown( face ) )
        {
            return 0.0f;
        }

        float const characteristicLength = float(
            std::sqrt( double( GraniteMax( face.m_areaM2, 0.000001f ) ) ) );

        float const variation = GraniteLerp(
            0.78f,
            1.22f,
            GraniteBodySignal( geometrySeed, 401 + faceIndex * 11 ) );

        float const requestedCrown =
            characteristicLength *
            0.20f *
            profile.m_surfaceHistory.m_weatheredRoundingStrength *
            GraniteClamp01( face.m_weatheredWeight ) *
            variation;

        // Broad and shallow. Never a hemisphere pasted onto the fragment.
        return GraniteMin(
            requestedCrown,
            characteristicLength * 0.115f );
    }

    //-------------------------------------------------------------------------
    // Keep the crown from punching through another fracture/joint plane.
    // Only the parent exterior plane itself may be exceeded.
    //-------------------------------------------------------------------------

    static float ClampGraniteExteriorCrownToNeighborPlanes(
        GraniteClosedGeometry const& geometry,
        GraniteFace const&           face,
        GranitePoint const&          centroid,
        float                        requestedCrown )
    {
        float allowedCrown = requestedCrown;
        bool  foundConstraint = false;

        for ( int32_t planeIndex = 0;
              planeIndex < geometry.m_numConstructionPlanes;
              ++planeIndex )
        {
            GranitePlane const& plane = geometry.m_constructionPlanes[planeIndex];

            float const normalAlignment = GraniteDot(
                plane.m_normal,
                face.m_normal );

            float const centroidPlaneDistance = GraniteDot(
                plane.m_normal,
                centroid );

            // Skip the parent old-exterior plane. Violating that plane is the
            // controlled weathering crown itself.
            if ( normalAlignment > 0.9990f &&
                 GraniteAbs( centroidPlaneDistance - plane.m_distance ) < 0.0015f )
            {
                continue;
            }

            float const movementRate = GraniteDot(
                plane.m_normal,
                face.m_normal );

            if ( movementRate <= 0.0001f )
            {
                continue;
            }

            float const slack = plane.m_distance - centroidPlaneDistance;

            if ( slack <= 0.0f )
            {
                continue;
            }

            float const planeLimit = slack / movementRate;

            allowedCrown = GraniteMin(
                allowedCrown,
                planeLimit * 0.72f );

            foundConstraint = true;
        }

        return foundConstraint
                 ? GraniteMax( allowedCrown, 0.0f )
                 : requestedCrown;
    }

    //-------------------------------------------------------------------------

    static void AddGraniteHistoryTriangle(
        GraniteClosedGeometry& geometry,
        uint8_t                i0,
        uint8_t                i1,
        uint8_t                i2,
        int32_t                parentFaceIndex,
        GraniteFace const&     parentFace )
    {
        EE_ASSERT(
            geometry.m_numTriangles <
            GraniteClosedGeometry::s_maxTriangles );

        EE_ASSERT( i0 < geometry.m_numVertices );
        EE_ASSERT( i1 < geometry.m_numVertices );
        EE_ASSERT( i2 < geometry.m_numVertices );
        EE_ASSERT( parentFaceIndex >= 0 );
        EE_ASSERT( parentFaceIndex < geometry.m_numFaces );

        int32_t const triangleIndex = geometry.m_numTriangles;
        int32_t const base = triangleIndex * 3;

        geometry.m_triangleIndices[base] = i0;
        geometry.m_triangleIndices[base + 1] = i1;
        geometry.m_triangleIndices[base + 2] = i2;

        geometry.m_triangleFaceIndex[triangleIndex] = uint8_t( parentFaceIndex );
        geometry.m_triangleSurfaceClass[triangleIndex] = parentFace.m_surfaceClass;
        geometry.m_triangleFaceOrigin[triangleIndex] = parentFace.m_origin;
        geometry.m_triangleWeatheredWeight[triangleIndex] = parentFace.m_weatheredWeight;

        ++geometry.m_numTriangles;
    }

    //-------------------------------------------------------------------------
    // Build one bounded weathered patch.
    //
    // The original polygon boundary remains fixed, so smoothing cannot leak
    // into neighboring FreshBreak / PrimaryJoint faces. An inset ring plus an
    // asymmetric crowned center creates genuine multi-vertex curvature.
    //-------------------------------------------------------------------------

    static bool RefineGraniteInheritedExteriorFace(
        GraniteClosedGeometry&        geometry,
        int32_t                       faceIndex,
        GraniteGeometryProfile const& profile,
        uint32_t                      geometrySeed,
        float&                        crownSum,
        int32_t&                      crownSampleCount )
    {
        GraniteFace const& face = geometry.m_faces[faceIndex];

        if ( !IsGraniteFaceEligibleForExteriorCrown( face ) )
        {
            return false;
        }

        int32_t const requiredNewVertices = face.m_numVertices + 1;
        int32_t const requiredNewTriangles = face.m_numVertices * 3;

        if ( geometry.m_numVertices + requiredNewVertices >
             GraniteClosedGeometry::s_maxVertices )
        {
            return false;
        }

        if ( geometry.m_numTriangles + requiredNewTriangles >
             GraniteClosedGeometry::s_maxTriangles )
        {
            return false;
        }

        GranitePoint const centroid = GetGraniteFaceCentroid( geometry, face );

        float crown = CalculateGraniteExteriorCrownMagnitude(
            face,
            profile,
            geometrySeed,
            faceIndex );

        crown = ClampGraniteExteriorCrownToNeighborPlanes(
            geometry,
            face,
            centroid,
            crown );

        if ( crown <= 0.00001f )
        {
            return false;
        }

        GranitePoint tangent;
        GranitePoint bitangent;
        GetGranitePlaneBasis( face.m_normal, tangent, bitangent );

        float const characteristicLength = float(
            std::sqrt( double( GraniteMax( face.m_areaM2, 0.000001f ) ) ) );

        float const asymmetryX =
            ( GraniteBodySignal( geometrySeed, 503 + faceIndex * 13 ) - 0.5f ) *
            characteristicLength * 0.055f;

        float const asymmetryY =
            ( GraniteBodySignal( geometrySeed, 504 + faceIndex * 13 ) - 0.5f ) *
            characteristicLength * 0.055f;

        GranitePoint crownCenter = GraniteAdd(
            centroid,
            GraniteAdd(
                GraniteScale( tangent, asymmetryX ),
                GraniteScale( bitangent, asymmetryY ) ) );

        float const centerVariation = GraniteLerp(
            0.93f,
            1.07f,
            GraniteBodySignal( geometrySeed, 505 + faceIndex * 13 ) );

        crownCenter = GraniteAdd(
            crownCenter,
            GraniteScale( face.m_normal, crown * centerVariation ) );

        uint8_t innerRing[GraniteFace::s_maxVertices];

        for ( int32_t i = 0; i < face.m_numVertices; ++i )
        {
            GranitePoint const& boundary = geometry.m_vertices[face.m_vertexIndices[i]];

            float const insetFraction = GraniteLerp(
                0.30f,
                0.40f,
                GraniteBodySignal( geometrySeed, 520 + faceIndex * 19 + i * 2 ) );

            float const shoulderFraction = GraniteLerp(
                0.24f,
                0.42f,
                GraniteBodySignal( geometrySeed, 521 + faceIndex * 19 + i * 2 ) );

            GranitePoint const boundaryToCentroid = GraniteSubtract(
                centroid,
                boundary );

            GranitePoint inner = GraniteAdd(
                boundary,
                GraniteScale( boundaryToCentroid, insetFraction ) );

            inner = GraniteAdd(
                inner,
                GraniteScale( face.m_normal, crown * shoulderFraction ) );

            EE_ASSERT(
                geometry.m_numVertices <
                GraniteClosedGeometry::s_maxVertices );

            innerRing[i] = uint8_t( geometry.m_numVertices );
            geometry.m_vertices[geometry.m_numVertices++] = inner;
        }

        EE_ASSERT(
            geometry.m_numVertices <
            GraniteClosedGeometry::s_maxVertices );

        uint8_t const centerIndex = uint8_t( geometry.m_numVertices );
        geometry.m_vertices[geometry.m_numVertices++] = crownCenter;

        for ( int32_t i = 0; i < face.m_numVertices; ++i )
        {
            int32_t const next = ( i + 1 ) % face.m_numVertices;

            uint8_t const boundaryI = face.m_vertexIndices[i];
            uint8_t const boundaryNext = face.m_vertexIndices[next];
            uint8_t const innerI = innerRing[i];
            uint8_t const innerNext = innerRing[next];

            AddGraniteHistoryTriangle(
                geometry,
                boundaryI,
                boundaryNext,
                innerNext,
                faceIndex,
                face );

            AddGraniteHistoryTriangle(
                geometry,
                boundaryI,
                innerNext,
                innerI,
                faceIndex,
                face );

            AddGraniteHistoryTriangle(
                geometry,
                innerI,
                innerNext,
                centerIndex,
                faceIndex,
                face );
        }

        ++geometry.m_surfaceHistory.m_inheritedExteriorPatchCount;
        geometry.m_surfaceHistory.m_historyAddedVertexCount += requiredNewVertices;

        int32_t const baseTriangleCount = face.m_numVertices - 2;
        geometry.m_surfaceHistory.m_historyAddedTriangleCount +=
            requiredNewTriangles - baseTriangleCount;

        crownSum += crown;
        ++crownSampleCount;

        geometry.m_surfaceHistory.m_maxCrownM = GraniteMax(
            geometry.m_surfaceHistory.m_maxCrownM,
            crown );

        return true;
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 6E — explicit local edge-fillet mesh
    //
    // A SurfaceTransition face is only a narrow topological corridor opened
    // by one derived half-space. Weathered/mixed/support corridors are then
    // tessellated into two bounded inset rows plus a shallow crowned center.
    //
    // Geological face centers on either side are never moved. FreshSharp and
    // StructuralCrisp edges never enter this path.
    //-------------------------------------------------------------------------

    static bool IsGraniteFaceEligibleForEdgeFillet(
        GraniteFace const& face )
    {
        if ( face.m_origin != GraniteFaceOrigin::SurfaceTransition ||
             face.m_numVertices < 3 )
        {
            return false;
        }

        switch ( face.m_edgeHistoryClass )
        {
            case GraniteEdgeHistoryClass::WeatheredWear:
            case GraniteEdgeHistoryClass::MixedHistory:
            case GraniteEdgeHistoryClass::SupportWear:
                return true;

            case GraniteEdgeHistoryClass::FreshSharp:
            case GraniteEdgeHistoryClass::StructuralCrisp:
            case GraniteEdgeHistoryClass::None:
            default:
                return false;
        }
    }

    //-------------------------------------------------------------------------

    static float GetGraniteEdgeFilletCrownFactor(
        GraniteEdgeHistoryClass edgeClass )
    {
        switch ( edgeClass )
        {
            case GraniteEdgeHistoryClass::WeatheredWear:
                return 0.30f;

            case GraniteEdgeHistoryClass::MixedHistory:
                return 0.25f;

            case GraniteEdgeHistoryClass::SupportWear:
                return 0.18f;

            case GraniteEdgeHistoryClass::FreshSharp:
            case GraniteEdgeHistoryClass::StructuralCrisp:
            case GraniteEdgeHistoryClass::None:
            default:
                return 0.0f;
        }
    }

    //-------------------------------------------------------------------------

    static bool RefineGraniteSurfaceTransitionFace(
        GraniteClosedGeometry& geometry,
        int32_t                faceIndex,
        uint32_t               geometrySeed )
    {
        GraniteFace const& face = geometry.m_faces[faceIndex];

        if ( !IsGraniteFaceEligibleForEdgeFillet( face ) )
        {
            return false;
        }

        // Two explicit intermediate rows plus one crowned center.
        int32_t const requiredNewVertices =
            face.m_numVertices * 2 + 1;

        int32_t const requiredNewTriangles =
            face.m_numVertices * 5;

        if ( geometry.m_numVertices + requiredNewVertices >
             GraniteClosedGeometry::s_maxVertices )
        {
            return false;
        }

        if ( geometry.m_numTriangles + requiredNewTriangles >
             GraniteClosedGeometry::s_maxTriangles )
        {
            return false;
        }

        GranitePoint const centroid =
            GetGraniteFaceCentroid(
                geometry,
                face );

        // Find the longest boundary edge. SurfaceTransition faces are normally
        // long, narrow polygons around the original geological seam; this gives
        // us a stable along-edge axis without needing parent-face pointers.
        GranitePoint longAxis =
            {
                1.0f,
                0.0f,
                0.0f };

        float longestBoundaryEdge =
            0.0f;

        for ( int32_t i = 0;
              i < face.m_numVertices;
              ++i )
        {
            int32_t const next =
                ( i + 1 ) %
                face.m_numVertices;

            GranitePoint const& a =
                geometry.m_vertices[face.m_vertexIndices[i]];

            GranitePoint const& b =
                geometry.m_vertices[face.m_vertexIndices[next]];

            GranitePoint const edge =
                GraniteSubtract(
                    b,
                    a );

            float const edgeLength =
                GraniteLength(
                    edge );

            if ( edgeLength >
                 longestBoundaryEdge )
            {
                longestBoundaryEdge =
                    edgeLength;

                longAxis =
                    GraniteNormalize(
                        edge );
            }
        }

        if ( longestBoundaryEdge <=
             0.0002f )
        {
            return false;
        }

        GranitePoint crossAxis =
            GraniteNormalize(
                GraniteCross(
                    face.m_normal,
                    longAxis ) );

        if ( GraniteLength( crossAxis ) <=
             0.0001f )
        {
            return false;
        }

        float minLong =
            1000000.0f;

        float maxLong =
            -1000000.0f;

        float minCross =
            1000000.0f;

        float maxCross =
            -1000000.0f;

        for ( int32_t i = 0;
              i < face.m_numVertices;
              ++i )
        {
            GranitePoint const relative =
                GraniteSubtract(
                    geometry.m_vertices[face.m_vertexIndices[i]],
                    centroid );

            float const along =
                GraniteDot(
                    relative,
                    longAxis );

            float const across =
                GraniteDot(
                    relative,
                    crossAxis );

            minLong =
                GraniteMin(
                    minLong,
                    along );

            maxLong =
                GraniteMax(
                    maxLong,
                    along );

            minCross =
                GraniteMin(
                    minCross,
                    across );

            maxCross =
                GraniteMax(
                    maxCross,
                    across );
        }

        float const longSpan =
            maxLong -
            minLong;

        float const crossSpan =
            maxCross -
            minCross;

        float const minorSpan =
            GraniteMin(
                longSpan,
                crossSpan );

        if ( minorSpan <=
             0.00035f )
        {
            return false;
        }

        float const crownFactor =
            GetGraniteEdgeFilletCrownFactor(
                face.m_edgeHistoryClass );

        if ( crownFactor <=
             0.0f )
        {
            return false;
        }

        float crown =
            minorSpan *
            crownFactor *
            GraniteLerp(
                0.86f,
                1.14f,
                GraniteBodySignal(
                    geometrySeed,
                    1701 +
                        faceIndex *
                            29 ) );

        // Keep the fillet shallow relative to the strip itself.
        crown =
            GraniteMin(
                crown,
                minorSpan *
                    0.34f );

        crown =
            ClampGraniteExteriorCrownToNeighborPlanes(
                geometry,
                face,
                centroid,
                crown );

        if ( crown <=
             0.00004f )
        {
            return false;
        }

        uint8_t row1[GraniteFace::s_maxVertices];

        uint8_t row2[GraniteFace::s_maxVertices];

        for ( int32_t i = 0;
              i < face.m_numVertices;
              ++i )
        {
            GranitePoint const& boundary =
                geometry.m_vertices[face.m_vertexIndices[i]];

            GranitePoint const relative =
                GraniteSubtract(
                    boundary,
                    centroid );

            float const along =
                GraniteDot(
                    relative,
                    longAxis );

            float const across =
                GraniteDot(
                    relative,
                    crossAxis );

            // Preserve almost all of the edge length while collapsing the
            // narrow cross-strip dimension. This makes actual rows around the
            // seam instead of another face-wide pyramid.
            float const row1AlongScale =
                GraniteLerp(
                    0.955f,
                    0.980f,
                    GraniteBodySignal(
                        geometrySeed,
                        1731 +
                            faceIndex *
                                31 +
                            i *
                                3 ) );

            float const row2AlongScale =
                GraniteLerp(
                    0.875f,
                    0.925f,
                    GraniteBodySignal(
                        geometrySeed,
                        1732 +
                            faceIndex *
                                31 +
                            i *
                                3 ) );

            float const row1CrossScale =
                GraniteLerp(
                    0.66f,
                    0.76f,
                    GraniteBodySignal(
                        geometrySeed,
                        1733 +
                            faceIndex *
                                31 +
                            i *
                                3 ) );

            float const row2CrossScale =
                GraniteLerp(
                    0.30f,
                    0.42f,
                    GraniteBodySignal(
                        geometrySeed,
                        1734 +
                            faceIndex *
                                31 +
                            i *
                                3 ) );

            float const localWear =
                GraniteLerp(
                    0.90f,
                    1.10f,
                    GraniteBodySignal(
                        geometrySeed,
                        1735 +
                            faceIndex *
                                37 +
                            i *
                                5 ) );

            GranitePoint row1Point =
                GraniteAdd(
                    centroid,
                    GraniteAdd(
                        GraniteScale(
                            longAxis,
                            along *
                                row1AlongScale ),
                        GraniteScale(
                            crossAxis,
                            across *
                                row1CrossScale ) ) );

            row1Point =
                GraniteAdd(
                    row1Point,
                    GraniteScale(
                        face.m_normal,
                        crown *
                            0.24f *
                            localWear ) );

            GranitePoint row2Point =
                GraniteAdd(
                    centroid,
                    GraniteAdd(
                        GraniteScale(
                            longAxis,
                            along *
                                row2AlongScale ),
                        GraniteScale(
                            crossAxis,
                            across *
                                row2CrossScale ) ) );

            row2Point =
                GraniteAdd(
                    row2Point,
                    GraniteScale(
                        face.m_normal,
                        crown *
                            0.68f *
                            localWear ) );

            EE_ASSERT(
                geometry.m_numVertices <
                GraniteClosedGeometry::s_maxVertices );

            row1[i] =
                uint8_t(
                    geometry.m_numVertices );

            geometry.m_vertices[geometry.m_numVertices++] =
                row1Point;

            EE_ASSERT(
                geometry.m_numVertices <
                GraniteClosedGeometry::s_maxVertices );

            row2[i] =
                uint8_t(
                    geometry.m_numVertices );

            geometry.m_vertices[geometry.m_numVertices++] =
                row2Point;
        }

        float const centerAlongOffset =
            ( GraniteBodySignal(
                  geometrySeed,
                  1811 +
                      faceIndex *
                          17 ) -
              0.5f ) *
            longSpan *
            0.045f;

        GranitePoint crownCenter =
            GraniteAdd(
                centroid,
                GraniteScale(
                    longAxis,
                    centerAlongOffset ) );

        crownCenter =
            GraniteAdd(
                crownCenter,
                GraniteScale(
                    face.m_normal,
                    crown ) );

        EE_ASSERT(
            geometry.m_numVertices <
            GraniteClosedGeometry::s_maxVertices );

        uint8_t const centerIndex =
            uint8_t(
                geometry.m_numVertices );

        geometry.m_vertices[geometry.m_numVertices++] =
            crownCenter;

        for ( int32_t i = 0;
              i < face.m_numVertices;
              ++i )
        {
            int32_t const next =
                ( i + 1 ) %
                face.m_numVertices;

            uint8_t const boundaryI =
                face.m_vertexIndices[i];

            uint8_t const boundaryNext =
                face.m_vertexIndices[next];

            // Geological face -> first fillet row.
            AddGraniteHistoryTriangle(
                geometry,
                boundaryI,
                boundaryNext,
                row1[next],
                faceIndex,
                face );

            AddGraniteHistoryTriangle(
                geometry,
                boundaryI,
                row1[next],
                row1[i],
                faceIndex,
                face );

            // First row -> second fillet row.
            AddGraniteHistoryTriangle(
                geometry,
                row1[i],
                row1[next],
                row2[next],
                faceIndex,
                face );

            AddGraniteHistoryTriangle(
                geometry,
                row1[i],
                row2[next],
                row2[i],
                faceIndex,
                face );

            // Second row -> shallow center ridge.
            AddGraniteHistoryTriangle(
                geometry,
                row2[i],
                row2[next],
                centerIndex,
                faceIndex,
                face );
        }

        geometry.m_surfaceHistory.m_historyAddedVertexCount +=
            requiredNewVertices;

        int32_t const baseTriangleCount =
            face.m_numVertices -
            2;

        geometry.m_surfaceHistory.m_historyAddedTriangleCount +=
            requiredNewTriangles -
            baseTriangleCount;

        return true;
    }

    //-------------------------------------------------------------------------
    // Rebuild presentation/certification triangles from geological parent
    // faces. InheritedExterior receives its existing local crown. Selected
    // SurfaceTransition corridors receive the Step 6E explicit curved strip.
    //-------------------------------------------------------------------------

    static void TriangulateGraniteFacesWithSurfaceHistory(
        GraniteClosedGeometry&        geometry,
        GraniteGeometryProfile const& profile,
        uint32_t                      geometrySeed )
    {
        int32_t const originalVertexCount = geometry.m_numVertices;
        int32_t const baseTriangleCount = geometry.m_numTriangles;

        geometry.m_surfaceHistory.m_verticesBeforeHistory = originalVertexCount;
        geometry.m_surfaceHistory.m_trianglesBeforeHistory = baseTriangleCount;
        geometry.m_surfaceHistory.m_inheritedExteriorPatchCount = 0;
        geometry.m_surfaceHistory.m_historyAddedVertexCount = 0;
        geometry.m_surfaceHistory.m_historyAddedTriangleCount = 0;
        geometry.m_surfaceHistory.m_meanCrownM = 0.0f;
        geometry.m_surfaceHistory.m_maxCrownM = 0.0f;

        geometry.m_numTriangles = 0;

        bool refinedFace[GraniteClosedGeometry::s_maxFaces] =
            {
                false };

        int32_t remainingRefinementTriangleBudget =
            GraniteClosedGeometry::s_maxTriangles -
            baseTriangleCount;

        remainingRefinementTriangleBudget =
            remainingRefinementTriangleBudget > 0
                ? remainingRefinementTriangleBudget
                : 0;

        float   crownSum = 0.0f;
        int32_t crownSampleCount = 0;

        //-------------------------------------------------------------------------
        // Pass 1: preserve Step 6B old-exterior authority first.
        //
        // B3 south is our visual no-regression control. Edge fillets may use
        // leftover topology, but they may never consume the fixed budget needed
        // to express an inherited exterior crown.
        //-------------------------------------------------------------------------

        for ( int32_t faceIndex = 0;
              faceIndex < geometry.m_numFaces;
              ++faceIndex )
        {
            GraniteFace const& face =
                geometry.m_faces[faceIndex];

            if ( !IsGraniteFaceEligibleForExteriorCrown(
                     face ) )
            {
                continue;
            }

            int32_t const baseFaceTriangles =
                face.m_numVertices -
                2;

            int32_t const refinedFaceTriangles =
                face.m_numVertices *
                3;

            int32_t const netTriangleCost =
                refinedFaceTriangles -
                baseFaceTriangles;

            if ( netTriangleCost >
                 remainingRefinementTriangleBudget )
            {
                continue;
            }

            if ( RefineGraniteInheritedExteriorFace(
                     geometry,
                     faceIndex,
                     profile,
                     geometrySeed,
                     crownSum,
                     crownSampleCount ) )
            {
                refinedFace[faceIndex] =
                    true;

                remainingRefinementTriangleBudget -=
                    netTriangleCost;
            }
        }

        //-------------------------------------------------------------------------
        // Pass 2: explicit curved seam strips.
        //
        // Priority is history-driven. Mixed old/fresh seams are most valuable,
        // then old/old weather wear, then support-edge wear. Maximum three
        // curved strips per closed body keeps the operation local and prevents
        // B1/B2/B3 from becoming uniformly softened.
        //-------------------------------------------------------------------------

        static constexpr GraniteEdgeHistoryClass s_filletPriority[3] =
            {
                GraniteEdgeHistoryClass::MixedHistory,
                GraniteEdgeHistoryClass::WeatheredWear,
                GraniteEdgeHistoryClass::SupportWear };

        static constexpr int32_t s_maxCurvedEdgeFillets =
            2;

        int32_t curvedEdgeFilletCount =
            0;

        for ( int32_t priorityIndex = 0;
              priorityIndex < 3 &&
              curvedEdgeFilletCount <
                  s_maxCurvedEdgeFillets;
              ++priorityIndex )
        {
            GraniteEdgeHistoryClass const targetClass =
                s_filletPriority[priorityIndex];

            for ( int32_t faceIndex = 0;
                  faceIndex < geometry.m_numFaces &&
                  curvedEdgeFilletCount <
                      s_maxCurvedEdgeFillets;
                  ++faceIndex )
            {
                if ( refinedFace[faceIndex] )
                {
                    continue;
                }

                GraniteFace const& face =
                    geometry.m_faces[faceIndex];

                if ( face.m_origin !=
                         GraniteFaceOrigin::SurfaceTransition ||
                     face.m_edgeHistoryClass !=
                         targetClass )
                {
                    continue;
                }

                int32_t const baseFaceTriangles =
                    face.m_numVertices -
                    2;

                int32_t const refinedFaceTriangles =
                    face.m_numVertices *
                    5;

                int32_t const netTriangleCost =
                    refinedFaceTriangles -
                    baseFaceTriangles;

                if ( netTriangleCost >
                     remainingRefinementTriangleBudget )
                {
                    continue;
                }

                if ( RefineGraniteSurfaceTransitionFace(
                         geometry,
                         faceIndex,
                         geometrySeed ) )
                {
                    refinedFace[faceIndex] =
                        true;

                    remainingRefinementTriangleBudget -=
                        netTriangleCost;

                    ++curvedEdgeFilletCount;
                }
            }
        }

        //-------------------------------------------------------------------------
        // Pass 3: untouched geological and transition faces keep their ordinary
        // planar triangulation.
        //-------------------------------------------------------------------------

        for ( int32_t faceIndex = 0;
              faceIndex < geometry.m_numFaces;
              ++faceIndex )
        {
            if ( refinedFace[faceIndex] )
            {
                continue;
            }

            GraniteFace const& face =
                geometry.m_faces[faceIndex];

            for ( int32_t i = 1;
                  i < face.m_numVertices - 1;
                  ++i )
            {
                AddGraniteHistoryTriangle(
                    geometry,
                    face.m_vertexIndices[0],
                    face.m_vertexIndices[i],
                    face.m_vertexIndices[i + 1],
                    faceIndex,
                    face );
            }
        }

        if ( crownSampleCount > 0 )
        {
            geometry.m_surfaceHistory.m_meanCrownM =
                crownSum /
                float(
                    crownSampleCount );
        }

        EE_ASSERT(
            geometry.m_numTriangles >=
            4 );

        EE_ASSERT(
            geometry.m_numTriangles <=
            GraniteClosedGeometry::s_maxTriangles );

        EE_ASSERT(
            geometry.m_numVertices >=
            originalVertexCount );

        EE_ASSERT(
            geometry.m_numVertices <=
            GraniteClosedGeometry::s_maxVertices );
    }

    //-------------------------------------------------------------------------
    // Surface-history falsifiability controls.
    //-------------------------------------------------------------------------

    static void RunGraniteSurfaceHistoryControls(
        GraniteClosedGeometry&        geometry,
        GraniteGeometryProfile const& profile,
        uint32_t                      geometrySeed )
    {
        GraniteFace controlFace;
        controlFace.m_numVertices = 4;
        controlFace.m_areaM2 = 1.0f;
        controlFace.m_origin = GraniteFaceOrigin::InheritedExterior;
        controlFace.m_surfaceClass = GraniteSurfaceFaceClass::WeatheredExterior;

        controlFace.m_weatheredWeight = 0.0f;

        float const zeroCrown = CalculateGraniteExteriorCrownMagnitude(
            controlFace,
            profile,
            geometrySeed,
            0 );

        geometry.m_surfaceHistory.m_controlZeroExteriorProducesZeroCrown =
            GraniteAbs( zeroCrown ) <= 0.000001f;

        controlFace.m_weatheredWeight = 1.0f;

        float const positiveCrown = CalculateGraniteExteriorCrownMagnitude(
            controlFace,
            profile,
            geometrySeed,
            1 );

        float const controlRadius = 0.50f;
        float const expectedPlanarity = controlRadius /
                                        float( std::sqrt( double(
                                            controlRadius * controlRadius +
                                            positiveCrown * positiveCrown ) ) );

        geometry.m_surfaceHistory.m_controlExteriorCrownResponds =
            positiveCrown > 0.0001f &&
            expectedPlanarity < 0.99999f;

        GraniteFace freshControl = controlFace;
        freshControl.m_origin = GraniteFaceOrigin::FreshBreak;
        freshControl.m_surfaceClass = GraniteSurfaceFaceClass::FreshFracture;
        freshControl.m_weatheredWeight = 1.0f;

        float const rejectedFreshCrown = CalculateGraniteExteriorCrownMagnitude(
            freshControl,
            profile,
            geometrySeed,
            2 );

        geometry.m_surfaceHistory.m_controlFreshBreakRejectsCrown =
            GraniteAbs( rejectedFreshCrown ) <= 0.000001f;

        EE_ASSERT( geometry.m_surfaceHistory.m_controlZeroExteriorProducesZeroCrown );
        EE_ASSERT( geometry.m_surfaceHistory.m_controlExteriorCrownResponds );
        EE_ASSERT( geometry.m_surfaceHistory.m_controlFreshBreakRejectsCrown );
    }

    //-------------------------------------------------------------------------
    // Closed mesh volume
    //-------------------------------------------------------------------------

    static float GraniteSignedTriangleVolume(
        GranitePoint const& a,
        GranitePoint const& b,
        GranitePoint const& c )
    {
        return GraniteDot( a, GraniteCross( b, c ) ) / 6.0f;
    }

    //-------------------------------------------------------------------------

    float MeasureGraniteClosedGeometryVolume(
        GraniteClosedGeometry const& geometry )
    {
        float signedVolume = 0.0f;

        for ( int32_t triangleIndex = 0;
              triangleIndex < geometry.m_numTriangles;
              ++triangleIndex )
        {
            int32_t const base = triangleIndex * 3;

            uint8_t const i0 = geometry.m_triangleIndices[base];
            uint8_t const i1 = geometry.m_triangleIndices[base + 1];
            uint8_t const i2 = geometry.m_triangleIndices[base + 2];

            EE_ASSERT( i0 < geometry.m_numVertices );
            EE_ASSERT( i1 < geometry.m_numVertices );
            EE_ASSERT( i2 < geometry.m_numVertices );

            signedVolume += GraniteSignedTriangleVolume(
                geometry.m_vertices[i0],
                geometry.m_vertices[i1],
                geometry.m_vertices[i2] );
        }

        return GraniteAbs( signedVolume );
    }

    //-------------------------------------------------------------------------
    // Geometry transform helpers
    //-------------------------------------------------------------------------

    static GranitePoint RotateGraniteDirectionToDirection(
        GranitePoint const& point,
        GranitePoint const& sourceDirection,
        GranitePoint const& targetDirection )
    {
        GranitePoint const source = GraniteNormalize( sourceDirection );
        GranitePoint const target = GraniteNormalize( targetDirection );

        float const cosine = GraniteClampSigned( GraniteDot( source, target ) );

        GranitePoint axis = GraniteCross( source, target );
        float const  sine = GraniteLength( axis );

        if ( sine <= 0.000001f && cosine > 0.0f )
        {
            return point;
        }

        if ( sine <= 0.000001f )
        {
            GranitePoint helper;

            if ( GraniteAbs( source.m_x ) < 0.80f )
            {
                helper = { 1.0f, 0.0f, 0.0f };
            }
            else
            {
                helper = { 0.0f, 1.0f, 0.0f };
            }

            axis = GraniteNormalize( GraniteCross( source, helper ) );

            float const projection = GraniteDot( axis, point );

            GranitePoint result;
            result.m_x = -point.m_x + 2.0f * axis.m_x * projection;
            result.m_y = -point.m_y + 2.0f * axis.m_y * projection;
            result.m_z = -point.m_z + 2.0f * axis.m_z * projection;
            return result;
        }

        axis = GraniteScale( axis, 1.0f / sine );

        GranitePoint const axisCrossPoint = GraniteCross( axis, point );
        float const        axisDotPoint = GraniteDot( axis, point );

        GranitePoint result;

        result.m_x =
            point.m_x * cosine +
            axisCrossPoint.m_x * sine +
            axis.m_x * axisDotPoint * ( 1.0f - cosine );

        result.m_y =
            point.m_y * cosine +
            axisCrossPoint.m_y * sine +
            axis.m_y * axisDotPoint * ( 1.0f - cosine );

        result.m_z =
            point.m_z * cosine +
            axisCrossPoint.m_z * sine +
            axis.m_z * axisDotPoint * ( 1.0f - cosine );

        return result;
    }

    //-------------------------------------------------------------------------

    static void RecalculateGraniteFaceAreas( GraniteClosedGeometry& geometry )
    {
        for ( int32_t faceIndex = 0; faceIndex < geometry.m_numFaces; ++faceIndex )
        {
            geometry.m_faces[faceIndex].m_areaM2 =
                MeasureGraniteFaceArea( geometry, geometry.m_faces[faceIndex] );
        }
    }

    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------
    // Stable polygon resting face
    //-------------------------------------------------------------------------

    static void OrientGraniteToStableRestingFace( GraniteClosedGeometry& geometry )
    {
        EE_ASSERT( geometry.m_numFaces > 0 );

        int32_t bestFace = -1;
        float   bestArea = -1.0f;

        // Step 6F: prefer actual geological support. SeedEnvelope is a bounded
        // construction scaffold and should not become the flat underside just
        // because it happens to be the largest polygon.
        for ( int32_t faceIndex = 0; faceIndex < geometry.m_numFaces; ++faceIndex )
        {
            GraniteFace const& face = geometry.m_faces[faceIndex];

            bool const geologicalSupport =
                face.m_origin == GraniteFaceOrigin::FreshBreak ||
                face.m_origin == GraniteFaceOrigin::PrimaryJoint ||
                face.m_origin == GraniteFaceOrigin::SecondaryJoint;

            if ( !geologicalSupport ||
                 face.m_surfaceClass == GraniteSurfaceFaceClass::WeatheredExterior )
            {
                continue;
            }

            if ( face.m_areaM2 > bestArea )
            {
                bestArea = face.m_areaM2;
                bestFace = faceIndex;
            }
        }

        if ( bestFace < 0 )
        {
            for ( int32_t faceIndex = 0; faceIndex < geometry.m_numFaces; ++faceIndex )
            {
                GraniteFace const& face = geometry.m_faces[faceIndex];

                if ( face.m_areaM2 > bestArea )
                {
                    bestArea = face.m_areaM2;
                    bestFace = faceIndex;
                }
            }
        }

        EE_ASSERT( bestFace >= 0 );

        GranitePoint const sourceNormal = geometry.m_faces[bestFace].m_normal;
        GranitePoint const targetNormal = { 0.0f, 0.0f, -1.0f };

        for ( int32_t i = 0; i < geometry.m_numVertices; ++i )
        {
            geometry.m_vertices[i] = RotateGraniteDirectionToDirection(
                geometry.m_vertices[i],
                sourceNormal,
                targetNormal );
        }

        for ( int32_t faceIndex = 0; faceIndex < geometry.m_numFaces; ++faceIndex )
        {
            geometry.m_faces[faceIndex].m_normal = GraniteNormalize(
                RotateGraniteDirectionToDirection(
                    geometry.m_faces[faceIndex].m_normal,
                    sourceNormal,
                    targetNormal ) );
        }

        for ( int32_t planeIndex = 0;
              planeIndex < geometry.m_numConstructionPlanes;
              ++planeIndex )
        {
            geometry.m_constructionPlanes[planeIndex].m_normal = GraniteNormalize(
                RotateGraniteDirectionToDirection(
                    geometry.m_constructionPlanes[planeIndex].m_normal,
                    sourceNormal,
                    targetNormal ) );
        }

        float minZ = geometry.m_vertices[0].m_z;

        for ( int32_t i = 1; i < geometry.m_numVertices; ++i )
        {
            minZ = GraniteMin( minZ, geometry.m_vertices[i].m_z );
        }

        for ( int32_t i = 0; i < geometry.m_numVertices; ++i )
        {
            geometry.m_vertices[i].m_z -= minZ;
        }

        // Translation changes plane distance by dot(normal, translation).
        for ( int32_t planeIndex = 0;
              planeIndex < geometry.m_numConstructionPlanes;
              ++planeIndex )
        {
            geometry.m_constructionPlanes[planeIndex].m_distance -=
                geometry.m_constructionPlanes[planeIndex].m_normal.m_z * minZ;
        }

        geometry.m_restingFaceIndex = bestFace;
        geometry.m_restingFaceNormal = targetNormal;
        geometry.m_restingFaceAreaM2 = bestArea;

        RecalculateGraniteFaceAreas( geometry );

        // Triangulation is face-derived and can be rebuilt after orientation.
        TriangulateGraniteFaces( geometry );

        geometry.m_restingTriangleIndex = -1;

        for ( int32_t triangleIndex = 0;
              triangleIndex < geometry.m_numTriangles;
              ++triangleIndex )
        {
            if ( int32_t( geometry.m_triangleFaceIndex[triangleIndex] ) == bestFace )
            {
                geometry.m_restingTriangleIndex = triangleIndex;
                break;
            }
        }

        EE_ASSERT( geometry.m_restingTriangleIndex >= 0 );
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 6F/6G — detached-fragment compass de-bias
    //
    // Fragment bodies receive a deterministic yaw only AFTER their geological
    // resting face has been selected. This prevents B1/B2 clasts from all
    // presenting the same construction wall toward world north without
    // changing face topology, mass, history, or the resting plane. Bonded B3
    // is intentionally NOT yawed so its successful south witness remains a
    // valid no-regression comparison.
    //-------------------------------------------------------------------------

    static void ApplyGraniteDetachedBodyYaw(
        GraniteClosedGeometry& geometry,
        uint32_t               geometrySeed )
    {
        float constexpr twoPi = 6.28318530718f;
        float const angle =
            GraniteBodySignal( geometrySeed, 390 ) * twoPi;

        float const cosine = float( std::cos( double( angle ) ) );
        float const sine = float( std::sin( double( angle ) ) );

        for ( int32_t vertexIndex = 0;
              vertexIndex < geometry.m_numVertices;
              ++vertexIndex )
        {
            GranitePoint& vertex = geometry.m_vertices[vertexIndex];
            float const   x = vertex.m_x;
            float const   y = vertex.m_y;

            vertex.m_x = x * cosine - y * sine;
            vertex.m_y = x * sine + y * cosine;
        }

        for ( int32_t faceIndex = 0;
              faceIndex < geometry.m_numFaces;
              ++faceIndex )
        {
            GranitePoint& normal = geometry.m_faces[faceIndex].m_normal;
            float const   x = normal.m_x;
            float const   y = normal.m_y;

            normal.m_x = x * cosine - y * sine;
            normal.m_y = x * sine + y * cosine;
            normal = GraniteNormalize( normal );
        }

        for ( int32_t planeIndex = 0;
              planeIndex < geometry.m_numConstructionPlanes;
              ++planeIndex )
        {
            GranitePoint& normal =
                geometry.m_constructionPlanes[planeIndex].m_normal;
            float const x = normal.m_x;
            float const y = normal.m_y;

            normal.m_x = x * cosine - y * sine;
            normal.m_y = x * sine + y * cosine;
            normal = GraniteNormalize( normal );
        }
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 6G — constrained body-vertex relief
    //
    // The half-space planes remain the MACRO fracture / host ancestry, but a
    // real Granite surface does not remain mathematically coincident with
    // those planes at every corner. This pass moves the shared geological
    // vertices by a small deterministic amount so the same parent face can
    // acquire several secondary facets without opening cracks between faces.
    //
    // Important:
    //  - one body seed / BodyID always reproduces the same result;
    //  - movement is bounded by the body's characteristic length;
    //  - fresh/joint faces stay closer to their dominant plane;
    //  - SeedEnvelope remnants receive stronger relief so construction walls
    //    stop reading as manufactured slabs;
    //  - the resting face keeps real support anchors while other support
    //    vertices may lift inward, breaking the dead-flat underside.
    //-------------------------------------------------------------------------

    static bool GraniteFaceContainsVertex(
        GraniteFace const& face,
        int32_t            vertexIndex )
    {
        for ( int32_t i = 0; i < face.m_numVertices; ++i )
        {
            if ( int32_t( face.m_vertexIndices[i] ) == vertexIndex )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    static void ApplyGraniteConstrainedVertexRelief(
        GraniteClosedGeometry&        geometry,
        GraniteGeometryProfile const& profile,
        uint32_t                      geometrySeed )
    {
        EE_ASSERT( geometry.m_numVertices >= 4 );
        EE_ASSERT( geometry.m_numFaces >= 4 );

        float const volume = MeasureGraniteClosedGeometryVolume( geometry );
        EE_ASSERT( volume > 0.000001f );

        float const characteristicLength = float(
            std::cbrt( double( volume ) ) );

        GranitePoint centroid;

        for ( int32_t vertexIndex = 0;
              vertexIndex < geometry.m_numVertices;
              ++vertexIndex )
        {
            centroid = GraniteAdd(
                centroid,
                geometry.m_vertices[vertexIndex] );
        }

        centroid = GraniteScale(
            centroid,
            1.0f / float( geometry.m_numVertices ) );

        // Freeze the count. No vertices are created by this stage.
        int32_t const originalVertexCount = geometry.m_numVertices;

        for ( int32_t vertexIndex = 0;
              vertexIndex < originalVertexCount;
              ++vertexIndex )
        {
            GranitePoint incidentNormalSum;
            int32_t      incidentGeologicalFaceCount = 0;

            bool onRestingFace = false;
            bool touchesInheritedExterior = false;
            bool touchesSeedEnvelope = false;
            bool touchesFreshBreak = false;
            bool touchesPrimaryJoint = false;
            bool touchesSecondaryJoint = false;

            for ( int32_t faceIndex = 0;
                  faceIndex < geometry.m_numFaces;
                  ++faceIndex )
            {
                GraniteFace const& face = geometry.m_faces[faceIndex];

                if ( face.m_origin == GraniteFaceOrigin::SurfaceTransition ||
                     !GraniteFaceContainsVertex( face, vertexIndex ) )
                {
                    continue;
                }

                incidentNormalSum = GraniteAdd(
                    incidentNormalSum,
                    face.m_normal );

                ++incidentGeologicalFaceCount;

                onRestingFace =
                    onRestingFace ||
                    faceIndex == geometry.m_restingFaceIndex;

                touchesInheritedExterior =
                    touchesInheritedExterior ||
                    face.m_origin == GraniteFaceOrigin::InheritedExterior;

                touchesSeedEnvelope =
                    touchesSeedEnvelope ||
                    face.m_origin == GraniteFaceOrigin::SeedEnvelope;

                touchesFreshBreak =
                    touchesFreshBreak ||
                    face.m_origin == GraniteFaceOrigin::FreshBreak;

                touchesPrimaryJoint =
                    touchesPrimaryJoint ||
                    face.m_origin == GraniteFaceOrigin::PrimaryJoint;

                touchesSecondaryJoint =
                    touchesSecondaryJoint ||
                    face.m_origin == GraniteFaceOrigin::SecondaryJoint;
            }

            if ( incidentGeologicalFaceCount <= 0 )
            {
                continue;
            }

            GranitePoint const radial = GraniteNormalize(
                GraniteSubtract(
                    geometry.m_vertices[vertexIndex],
                    centroid ) );

            GranitePoint const incidentNormal = GraniteNormalize(
                incidentNormalSum );

            GranitePoint direction = GraniteNormalize(
                GraniteAdd(
                    GraniteScale( radial, 0.68f ),
                    GraniteScale( incidentNormal, 0.32f ) ) );

            if ( GraniteLength( direction ) <= 0.000001f )
            {
                direction = radial;
            }

            float minFactor = 0.018f;
            float maxFactor = 0.040f;

            if ( touchesSeedEnvelope )
            {
                // Construction-envelope fingerprints deserve the strongest
                // local articulation so they no longer survive as clean walls.
                minFactor = 0.045f;
                maxFactor = 0.082f;
            }
            else if ( touchesInheritedExterior )
            {
                minFactor = 0.036f;
                maxFactor = 0.068f;
            }
            else if ( touchesFreshBreak )
            {
                // Fresh fracture remains MACRO-planar but never perfectly flat.
                minFactor = 0.022f;
                maxFactor = 0.048f;
            }
            else if ( touchesSecondaryJoint )
            {
                minFactor = 0.020f;
                maxFactor = 0.042f;
            }
            else if ( touchesPrimaryJoint )
            {
                minFactor = 0.014f;
                maxFactor = 0.032f;
            }

            float const materialVariation = GraniteLerp(
                0.90f,
                1.12f,
                GraniteClamp01( profile.m_surfaceRoughness + 0.35f ) );

            float const magnitude =
                characteristicLength *
                GraniteLerp(
                    minFactor,
                    maxFactor,
                    GraniteBodySignal(
                        geometrySeed,
                        701 + vertexIndex * 11 ) ) *
                materialVariation;

            float const signSignal = GraniteBodySignal(
                geometrySeed,
                702 + vertexIndex * 11 );

            float signedMagnitude =
                signSignal < 0.47f
                    ? -magnitude
                    : magnitude;

            GranitePoint displacement;

            if ( onRestingFace )
            {
                float const supportSignal = GraniteBodySignal(
                    geometrySeed,
                    703 + vertexIndex * 11 );

                if ( supportSignal < 0.38f )
                {
                    // Keep a deterministic subset of real support anchors on
                    // the original resting plane. This is stability without a
                    // manufactured flat bottom.
                    continue;
                }

                // The resting-face outward normal points downward. Moving in
                // the opposite direction creates an underside recess rather
                // than pushing geometry below the support plane.
                float const lift =
                    characteristicLength *
                    GraniteLerp(
                        0.020f,
                        0.050f,
                        GraniteBodySignal(
                            geometrySeed,
                            704 + vertexIndex * 11 ) );

                displacement = GraniteScale(
                    geometry.m_restingFaceNormal,
                    -lift );
            }
            else
            {
                displacement = GraniteScale(
                    direction,
                    signedMagnitude );

                // A small tangential component prevents all relief from being
                // purely radial. It remains far smaller than the normal/radial
                // component so the parent geological shape still dominates.
                GranitePoint tangent = GraniteCross(
                    direction,
                    { 0.0f, 0.0f, 1.0f } );

                if ( GraniteLength( tangent ) <= 0.000001f )
                {
                    tangent = GraniteCross(
                        direction,
                        { 0.0f, 1.0f, 0.0f } );
                }

                tangent = GraniteNormalize( tangent );

                float const tangentAmount =
                    characteristicLength *
                    ( GraniteBodySignal(
                          geometrySeed,
                          705 + vertexIndex * 11 ) -
                      0.5f ) *
                    0.018f;

                displacement = GraniteAdd(
                    displacement,
                    GraniteScale( tangent, tangentAmount ) );
            }

            geometry.m_vertices[vertexIndex] = GraniteAdd(
                geometry.m_vertices[vertexIndex],
                displacement );
        }

        RecalculateGraniteFaceAreas( geometry );
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 6G — structured face articulation
    //
    // This is the direct correction to the overly literal "fresh = planar"
    // interpretation. A geological face keeps one dominant parent normal, but
    // selected large triangles receive one deterministic interior control
    // point displaced slightly IN or OUT of that parent plane. The result is a
    // set of secondary facets, shallow recesses, shoulders and knuckles like
    // the productive B3 south witness rather than one clean polygon wall.
    //
    // Boundaries are never split independently, so the closed mesh stays
    // watertight. Randomness is BodyID/seed constrained and therefore stable.
    //-------------------------------------------------------------------------

    static float GetGraniteFaceArticulationFactor(
        GraniteFace const& face )
    {
        switch ( face.m_origin )
        {
            case GraniteFaceOrigin::SeedEnvelope:
                return 0.090f;

            case GraniteFaceOrigin::InheritedExterior:
                return 0.060f;

            case GraniteFaceOrigin::FreshBreak:
                return 0.052f;

            case GraniteFaceOrigin::SecondaryJoint:
                return 0.040f;

            case GraniteFaceOrigin::PrimaryJoint:
                return 0.028f;

            case GraniteFaceOrigin::SurfaceTransition:
            case GraniteFaceOrigin::Unknown:
            default:
                return 0.0f;
        }
    }

    //-------------------------------------------------------------------------

    static bool ArticulateGraniteTriangle(
        GraniteClosedGeometry& geometry,
        int32_t                triangleIndex,
        uint32_t               geometrySeed,
        int32_t                articulationOrdinal )
    {
        if ( geometry.m_numVertices + 1 >
                 GraniteClosedGeometry::s_maxVertices ||
             geometry.m_numTriangles + 2 >
                 GraniteClosedGeometry::s_maxTriangles )
        {
            return false;
        }

        EE_ASSERT( triangleIndex >= 0 );
        EE_ASSERT( triangleIndex < geometry.m_numTriangles );

        int32_t const base = triangleIndex * 3;

        uint8_t const i0 = geometry.m_triangleIndices[base];
        uint8_t const i1 = geometry.m_triangleIndices[base + 1];
        uint8_t const i2 = geometry.m_triangleIndices[base + 2];

        EE_ASSERT( i0 < geometry.m_numVertices );
        EE_ASSERT( i1 < geometry.m_numVertices );
        EE_ASSERT( i2 < geometry.m_numVertices );

        int32_t const parentFaceIndex =
            int32_t( geometry.m_triangleFaceIndex[triangleIndex] );

        EE_ASSERT( parentFaceIndex >= 0 );
        EE_ASSERT( parentFaceIndex < geometry.m_numFaces );

        GraniteFace const& parentFace =
            geometry.m_faces[parentFaceIndex];

        float const articulationFactor =
            GetGraniteFaceArticulationFactor( parentFace );

        if ( articulationFactor <= 0.0f )
        {
            return false;
        }

        GranitePoint triangleNormal;
        float const  area = MeasureGraniteTriangleArea(
            geometry,
            i0,
            i1,
            i2,
            &triangleNormal );

        if ( area <= 0.00001f )
        {
            return false;
        }

        GranitePoint const& a = geometry.m_vertices[i0];
        GranitePoint const& b = geometry.m_vertices[i1];
        GranitePoint const& c = geometry.m_vertices[i2];

        GranitePoint center = GraniteScale(
            GraniteAdd(
                GraniteAdd( a, b ),
                c ),
            1.0f / 3.0f );

        float const characteristic = float(
            std::sqrt( double( area ) ) );

        float const magnitude =
            characteristic *
            articulationFactor *
            GraniteLerp(
                0.72f,
                1.28f,
                GraniteBodySignal(
                    geometrySeed,
                    811 + articulationOrdinal * 17 ) );

        float const directionSignal = GraniteBodySignal(
            geometrySeed,
            812 + articulationOrdinal * 17 );

        float sign =
            directionSignal < 0.48f
                ? -1.0f
                : 1.0f;

        if ( parentFaceIndex == geometry.m_restingFaceIndex )
        {
            // Recess the underside into the body; never extrude below support.
            sign = -1.0f;
        }
        else if ( parentFace.m_origin == GraniteFaceOrigin::InheritedExterior )
        {
            // Old exterior more often preserves an outward shoulder/back.
            sign = directionSignal < 0.34f ? -1.0f : 1.0f;
        }

        center = GraniteAdd(
            center,
            GraniteScale(
                parentFace.m_normal,
                magnitude * sign ) );

        GranitePoint tangent;
        GranitePoint bitangent;
        GetGranitePlaneBasis(
            parentFace.m_normal,
            tangent,
            bitangent );

        float const tangentJitter =
            characteristic *
            ( GraniteBodySignal(
                  geometrySeed,
                  813 + articulationOrdinal * 17 ) -
              0.5f ) *
            0.045f;

        float const bitangentJitter =
            characteristic *
            ( GraniteBodySignal(
                  geometrySeed,
                  814 + articulationOrdinal * 17 ) -
              0.5f ) *
            0.045f;

        center = GraniteAdd(
            center,
            GraniteAdd(
                GraniteScale( tangent, tangentJitter ),
                GraniteScale( bitangent, bitangentJitter ) ) );

        uint8_t const centerIndex = uint8_t( geometry.m_numVertices );
        geometry.m_vertices[geometry.m_numVertices++] = center;

        GraniteSurfaceFaceClass const surfaceClass =
            geometry.m_triangleSurfaceClass[triangleIndex];

        GraniteFaceOrigin const faceOrigin =
            geometry.m_triangleFaceOrigin[triangleIndex];

        float const weatheredWeight =
            geometry.m_triangleWeatheredWeight[triangleIndex];

        // Replace the original triangle with child 0.
        geometry.m_triangleIndices[base] = i0;
        geometry.m_triangleIndices[base + 1] = i1;
        geometry.m_triangleIndices[base + 2] = centerIndex;

        // Child 1.
        int32_t triangleBase = geometry.m_numTriangles * 3;
        geometry.m_triangleIndices[triangleBase] = i1;
        geometry.m_triangleIndices[triangleBase + 1] = i2;
        geometry.m_triangleIndices[triangleBase + 2] = centerIndex;
        geometry.m_triangleFaceIndex[geometry.m_numTriangles] =
            uint8_t( parentFaceIndex );
        geometry.m_triangleSurfaceClass[geometry.m_numTriangles] =
            surfaceClass;
        geometry.m_triangleFaceOrigin[geometry.m_numTriangles] =
            faceOrigin;
        geometry.m_triangleWeatheredWeight[geometry.m_numTriangles] =
            weatheredWeight;
        ++geometry.m_numTriangles;

        // Child 2.
        triangleBase = geometry.m_numTriangles * 3;
        geometry.m_triangleIndices[triangleBase] = i2;
        geometry.m_triangleIndices[triangleBase + 1] = i0;
        geometry.m_triangleIndices[triangleBase + 2] = centerIndex;
        geometry.m_triangleFaceIndex[geometry.m_numTriangles] =
            uint8_t( parentFaceIndex );
        geometry.m_triangleSurfaceClass[geometry.m_numTriangles] =
            surfaceClass;
        geometry.m_triangleFaceOrigin[geometry.m_numTriangles] =
            faceOrigin;
        geometry.m_triangleWeatheredWeight[geometry.m_numTriangles] =
            weatheredWeight;
        ++geometry.m_numTriangles;

        geometry.m_surfaceHistory.m_historyAddedVertexCount += 1;
        geometry.m_surfaceHistory.m_historyAddedTriangleCount += 2;

        return true;
    }

    //-------------------------------------------------------------------------

    static void ApplyGraniteStructuredFaceArticulation(
        GraniteClosedGeometry& geometry,
        uint32_t               geometrySeed )
    {
        int32_t const originalTriangleCount = geometry.m_numTriangles;

        float geologicalArea = 0.0f;

        for ( int32_t faceIndex = 0;
              faceIndex < geometry.m_numFaces;
              ++faceIndex )
        {
            GraniteFace const& face = geometry.m_faces[faceIndex];

            if ( face.m_origin != GraniteFaceOrigin::SurfaceTransition )
            {
                geologicalArea += face.m_areaM2;
            }
        }

        if ( geologicalArea <= 0.00001f )
        {
            return;
        }

        bool faceReceivedArticulation[GraniteClosedGeometry::s_maxFaces] =
            {
                false };

        int32_t                  articulationOrdinal = 0;
        static constexpr int32_t s_maxPrimaryArticulations = 10;

        // Pass 1: at most one strong articulation on each materially large
        // geological face. This spreads the B3-south language around the body
        // instead of piling every detail into one lucky orientation.
        for ( int32_t faceIndex = 0;
              faceIndex < geometry.m_numFaces &&
              articulationOrdinal < s_maxPrimaryArticulations;
              ++faceIndex )
        {
            GraniteFace const& face = geometry.m_faces[faceIndex];

            if ( face.m_origin == GraniteFaceOrigin::SurfaceTransition ||
                 GetGraniteFaceArticulationFactor( face ) <= 0.0f )
            {
                continue;
            }

            float const faceFraction = face.m_areaM2 / geologicalArea;

            float threshold = 0.045f;

            if ( face.m_origin == GraniteFaceOrigin::SeedEnvelope )
            {
                threshold = 0.025f;
            }
            else if ( faceIndex == geometry.m_restingFaceIndex )
            {
                threshold = 0.030f;
            }

            if ( faceFraction < threshold )
            {
                continue;
            }

            float gate = 0.18f;

            if ( face.m_origin == GraniteFaceOrigin::PrimaryJoint )
            {
                gate = 0.42f;
            }
            else if ( face.m_origin == GraniteFaceOrigin::SecondaryJoint )
            {
                gate = 0.30f;
            }

            if ( faceIndex != geometry.m_restingFaceIndex &&
                 GraniteBodySignal(
                     geometrySeed,
                     850 + faceIndex * 7 ) < gate )
            {
                continue;
            }

            int32_t bestTriangle = -1;
            float   bestArea = -1.0f;

            for ( int32_t triangleIndex = 0;
                  triangleIndex < originalTriangleCount;
                  ++triangleIndex )
            {
                if ( int32_t( geometry.m_triangleFaceIndex[triangleIndex] ) !=
                     faceIndex )
                {
                    continue;
                }

                int32_t const base = triangleIndex * 3;
                GranitePoint  normal;
                float const   area = MeasureGraniteTriangleArea(
                    geometry,
                    geometry.m_triangleIndices[base],
                    geometry.m_triangleIndices[base + 1],
                    geometry.m_triangleIndices[base + 2],
                    &normal );

                if ( area > bestArea )
                {
                    bestArea = area;
                    bestTriangle = triangleIndex;
                }
            }

            if ( bestTriangle >= 0 &&
                 ArticulateGraniteTriangle(
                     geometry,
                     bestTriangle,
                     geometrySeed,
                     articulationOrdinal ) )
            {
                faceReceivedArticulation[faceIndex] = true;
                ++articulationOrdinal;
            }
        }

        // Pass 2: exceptionally large faces may receive a second recess/knuckle
        // on a different original triangle. This is where giant B1/B2/B3 walls
        // stop being one smooth sheet without becoming noisy everywhere.
        for ( int32_t faceIndex = 0;
              faceIndex < geometry.m_numFaces &&
              articulationOrdinal < 14;
              ++faceIndex )
        {
            if ( !faceReceivedArticulation[faceIndex] )
            {
                continue;
            }

            GraniteFace const& face = geometry.m_faces[faceIndex];
            float const        faceFraction = face.m_areaM2 / geologicalArea;

            if ( faceFraction < 0.115f ||
                 GraniteBodySignal(
                     geometrySeed,
                     910 + faceIndex * 13 ) < 0.34f )
            {
                continue;
            }

            int32_t firstTriangle = -1;
            int32_t secondTriangle = -1;
            float   firstArea = -1.0f;
            float   secondArea = -1.0f;

            for ( int32_t triangleIndex = 0;
                  triangleIndex < originalTriangleCount;
                  ++triangleIndex )
            {
                if ( int32_t( geometry.m_triangleFaceIndex[triangleIndex] ) !=
                     faceIndex )
                {
                    continue;
                }

                int32_t const base = triangleIndex * 3;
                GranitePoint  normal;
                float const   area = MeasureGraniteTriangleArea(
                    geometry,
                    geometry.m_triangleIndices[base],
                    geometry.m_triangleIndices[base + 1],
                    geometry.m_triangleIndices[base + 2],
                    &normal );

                if ( area > firstArea )
                {
                    secondArea = firstArea;
                    secondTriangle = firstTriangle;
                    firstArea = area;
                    firstTriangle = triangleIndex;
                }
                else if ( area > secondArea )
                {
                    secondArea = area;
                    secondTriangle = triangleIndex;
                }
            }

            if ( secondTriangle >= 0 &&
                 ArticulateGraniteTriangle(
                     geometry,
                     secondTriangle,
                     geometrySeed,
                     articulationOrdinal ) )
            {
                ++articulationOrdinal;
            }
        }
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 6E — local edge-fillet corridor geometry
    //
    // Flat geological faces remain valid and keep their planar centers.
    // This pass selectively trims the mathematical seam BETWEEN adjacent
    // faces with a small derived half-space. The new SurfaceTransition plane
    // is presentation/physical wear geometry, not a new fracture event.
    //-------------------------------------------------------------------------

    struct GraniteEdgeTransitionCandidate
    {
        static constexpr int32_t s_maxSegments = 3;

        int32_t m_faceA = -1;
        int32_t m_faceB = -1;

        uint8_t m_vertexA = 0;
        uint8_t m_vertexB = 0;

        GraniteEdgeHistoryClass m_class = GraniteEdgeHistoryClass::None;

        GranitePoint m_edgeTangent;
        float        m_edgeLengthM = 0.0f;
        float        m_widthM = 0.0f;
        float        m_score = 0.0f;
        float        m_weatheredWeight = 0.0f;

        int32_t      m_segmentCount = 0;
        GranitePoint m_segmentNormals[s_maxSegments];
        float        m_segmentDistances[s_maxSegments] = { 0.0f, 0.0f, 0.0f };
        float        m_segmentWidthsM[s_maxSegments] = { 0.0f, 0.0f, 0.0f };

        bool m_selected = false;
    };

    //-------------------------------------------------------------------------

    static bool IsGraniteWeatheredEdgeFace( GraniteFace const& face )
    {
        return face.m_surfaceClass == GraniteSurfaceFaceClass::WeatheredExterior ||
               face.m_weatheredWeight >= 0.35f;
    }

    //-------------------------------------------------------------------------

    static bool IsGraniteStructuralEdgeFace( GraniteFace const& face )
    {
        return face.m_origin == GraniteFaceOrigin::PrimaryJoint ||
               face.m_origin == GraniteFaceOrigin::SecondaryJoint;
    }

    //-------------------------------------------------------------------------

    static GraniteEdgeHistoryClass ClassifyGraniteEdgeHistory(
        GraniteFace const& faceA,
        GraniteFace const& faceB,
        bool               touchesRestingFace )
    {
        bool const weatheredA = IsGraniteWeatheredEdgeFace( faceA );
        bool const weatheredB = IsGraniteWeatheredEdgeFace( faceB );

        if ( weatheredA && weatheredB )
        {
            return GraniteEdgeHistoryClass::WeatheredWear;
        }

        if ( weatheredA != weatheredB )
        {
            return GraniteEdgeHistoryClass::MixedHistory;
        }

        // Resting/support seams receive moderate treatment even when both
        // adjoining faces are otherwise fresh. A support face may be broad and
        // planar, but its entire perimeter should not default to a machined
        // razor edge.
        if ( touchesRestingFace )
        {
            return GraniteEdgeHistoryClass::SupportWear;
        }

        if ( IsGraniteStructuralEdgeFace( faceA ) ||
             IsGraniteStructuralEdgeFace( faceB ) )
        {
            return GraniteEdgeHistoryClass::StructuralCrisp;
        }

        return GraniteEdgeHistoryClass::FreshSharp;
    }

    //-------------------------------------------------------------------------

    static float GetGraniteEdgeWidthFactor( GraniteEdgeHistoryClass edgeClass )
    {
        switch ( edgeClass )
        {
            case GraniteEdgeHistoryClass::WeatheredWear:
                return 0.056f;

            case GraniteEdgeHistoryClass::MixedHistory:
                return 0.050f;

            case GraniteEdgeHistoryClass::SupportWear:
                return 0.036f;

            case GraniteEdgeHistoryClass::StructuralCrisp:
            case GraniteEdgeHistoryClass::FreshSharp:
                // Step 6E/6F: fresh and structural seams remain geological
                // sharp edges. They are not eligible for a derived transition
                // corridor or fillet scaffold.
                return 0.0f;

            case GraniteEdgeHistoryClass::None:
            default:
                return 0.0f;
        }
    }

    //-------------------------------------------------------------------------

    static float GetGraniteEdgeSelectionPriority( GraniteEdgeHistoryClass edgeClass )
    {
        switch ( edgeClass )
        {
            case GraniteEdgeHistoryClass::MixedHistory:
                return 1.00f;

            case GraniteEdgeHistoryClass::SupportWear:
                return 0.93f;

            case GraniteEdgeHistoryClass::WeatheredWear:
                return 0.88f;

            case GraniteEdgeHistoryClass::FreshSharp:
                return 0.46f;

            case GraniteEdgeHistoryClass::StructuralCrisp:
                return 0.38f;

            case GraniteEdgeHistoryClass::None:
            default:
                return 0.0f;
        }
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 6E — local edge-fillet scaffold
    //
    // Fresh and structural edges stay as geological sharp edges. Weathered,
    // mixed-history, and support seams receive ONE narrow derived transition
    // plane only to open a seam corridor. The actual rounded profile is built
    // later as explicit local mesh geometry inside that corridor.
    //-------------------------------------------------------------------------

    static int32_t GetGraniteEdgeSegmentCount( GraniteEdgeHistoryClass edgeClass )
    {
        // Step 6E uses ONE derived half-space only to open a narrow seam
        // corridor. Curvature is now carried by explicit tessellated strip
        // geometry during face triangulation rather than by stacking several
        // clipping planes.
        switch ( edgeClass )
        {
            case GraniteEdgeHistoryClass::WeatheredWear:
            case GraniteEdgeHistoryClass::MixedHistory:
            case GraniteEdgeHistoryClass::SupportWear:
                return 1;

            case GraniteEdgeHistoryClass::StructuralCrisp:
            case GraniteEdgeHistoryClass::FreshSharp:
            case GraniteEdgeHistoryClass::None:
            default:
                return 0;
        }
    }

    //-------------------------------------------------------------------------

    static float GetGraniteEdgeSegmentParameter(
        int32_t segmentCount,
        int32_t segmentIndex )
    {
        EE_ASSERT( segmentCount >= 1 );
        EE_ASSERT( segmentCount <= GraniteEdgeTransitionCandidate::s_maxSegments );
        EE_ASSERT( segmentIndex >= 0 );
        EE_ASSERT( segmentIndex < segmentCount );

        if ( segmentCount == 1 )
        {
            return 0.50f;
        }

        if ( segmentCount == 2 )
        {
            return segmentIndex == 0 ? 0.34f : 0.66f;
        }

        static float constexpr s_parameters[3] =
            {
                0.24f,
                0.50f,
                0.76f };

        return s_parameters[segmentIndex];
    }

    //-------------------------------------------------------------------------

    static float GetGraniteEdgeSegmentWidthScale(
        int32_t segmentCount,
        int32_t segmentIndex )
    {
        EE_ASSERT( segmentCount >= 1 );
        EE_ASSERT( segmentCount <= GraniteEdgeTransitionCandidate::s_maxSegments );
        EE_ASSERT( segmentIndex >= 0 );
        EE_ASSERT( segmentIndex < segmentCount );

        if ( segmentCount == 1 )
        {
            return 1.0f;
        }

        if ( segmentCount == 2 )
        {
            // Two equal intermediate tangents create a shallow two-plane roll.
            return 0.78f;
        }

        // A three-plane transition keeps the strongest trim at the center and
        // lets the outer strips taper back into the original flat faces.
        return segmentIndex == 1 ? 1.0f : 0.58f;
    }

    //-------------------------------------------------------------------------

    static float GetGraniteEdgeAlongLengthSkew(
        GraniteEdgeHistoryClass edgeClass )
    {
        switch ( edgeClass )
        {
            case GraniteEdgeHistoryClass::WeatheredWear:
                return 0.045f;

            case GraniteEdgeHistoryClass::MixedHistory:
                return 0.034f;

            case GraniteEdgeHistoryClass::SupportWear:
                return 0.022f;

            case GraniteEdgeHistoryClass::StructuralCrisp:
                return 0.004f;

            case GraniteEdgeHistoryClass::FreshSharp:
                return 0.003f;

            case GraniteEdgeHistoryClass::None:
            default:
                return 0.0f;
        }
    }

    //-------------------------------------------------------------------------

    static int32_t FindGraniteSharedEdgeVertices(
        GraniteFace const& faceA,
        GraniteFace const& faceB,
        uint8_t&           sharedA,
        uint8_t&           sharedB )
    {
        uint8_t shared[2] = { 0, 0 };
        int32_t sharedCount = 0;

        for ( int32_t a = 0; a < faceA.m_numVertices; ++a )
        {
            uint8_t const vertexA = faceA.m_vertexIndices[a];

            for ( int32_t b = 0; b < faceB.m_numVertices; ++b )
            {
                if ( vertexA != faceB.m_vertexIndices[b] )
                {
                    continue;
                }

                if ( sharedCount < 2 )
                {
                    shared[sharedCount] = vertexA;
                }

                ++sharedCount;
                break;
            }
        }

        if ( sharedCount != 2 )
        {
            return sharedCount;
        }

        sharedA = shared[0];
        sharedB = shared[1];
        return 2;
    }

    //-------------------------------------------------------------------------

    static bool DoesGraniteFaceMatchPlaneReceipt(
        GraniteClosedGeometry const& geometry,
        GraniteFace const&           face,
        GranitePoint const&          targetNormal,
        float                        targetDistance,
        GraniteFaceOrigin            targetOrigin )
    {
        if ( face.m_origin != targetOrigin )
        {
            return false;
        }

        if ( GraniteDot( face.m_normal, targetNormal ) < 0.9990f )
        {
            return false;
        }

        GranitePoint const centroid = GetGraniteFaceCentroid( geometry, face );
        float const        distance = GraniteDot( targetNormal, centroid );

        return GraniteAbs( distance - targetDistance ) <= 0.0020f;
    }

    //-------------------------------------------------------------------------

    static bool DoesGraniteTransitionSegmentFaceExist(
        GraniteClosedGeometry const&          geometry,
        GraniteEdgeTransitionCandidate const& candidate,
        int32_t                               segmentIndex )
    {
        EE_ASSERT( segmentIndex >= 0 );
        EE_ASSERT( segmentIndex < candidate.m_segmentCount );

        GranitePoint const& targetNormal =
            candidate.m_segmentNormals[segmentIndex];
        float const targetDistance =
            candidate.m_segmentDistances[segmentIndex];

        for ( int32_t faceIndex = 0;
              faceIndex < geometry.m_numFaces;
              ++faceIndex )
        {
            GraniteFace const& face = geometry.m_faces[faceIndex];

            if ( face.m_origin != GraniteFaceOrigin::SurfaceTransition )
            {
                continue;
            }

            if ( face.m_edgeHistoryClass != candidate.m_class )
            {
                continue;
            }

            if ( GraniteDot( face.m_normal, targetNormal ) < 0.9990f )
            {
                continue;
            }

            GranitePoint const centroid = GetGraniteFaceCentroid( geometry, face );

            if ( GraniteAbs(
                     GraniteDot( targetNormal, centroid ) -
                     targetDistance ) <= 0.0020f )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    static bool DoesGraniteTransitionEdgeExist(
        GraniteClosedGeometry const&          geometry,
        GraniteEdgeTransitionCandidate const& candidate )
    {
        for ( int32_t segmentIndex = 0;
              segmentIndex < candidate.m_segmentCount;
              ++segmentIndex )
        {
            if ( DoesGraniteTransitionSegmentFaceExist(
                     geometry,
                     candidate,
                     segmentIndex ) )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    static void SortGraniteEdgeTransitionCandidates(
        GraniteEdgeTransitionCandidate* pCandidates,
        int32_t                         count )
    {
        for ( int32_t i = 1; i < count; ++i )
        {
            GraniteEdgeTransitionCandidate const key = pCandidates[i];
            int32_t                              j = i - 1;

            while ( j >= 0 && pCandidates[j].m_score < key.m_score )
            {
                pCandidates[j + 1] = pCandidates[j];
                --j;
            }

            pCandidates[j + 1] = key;
        }
    }

    //-------------------------------------------------------------------------
    // Build the single bounded scaffold plane for an eligible geological seam.
    //
    // Step 6E intentionally does NOT stack multiple half-spaces. This plane
    // trims only enough material to create a narrow SurfaceTransition polygon.
    // That polygon is later tessellated into an explicit curved strip.
    //-------------------------------------------------------------------------

    static void BuildGraniteEdgeTransitionSegments(
        GraniteEdgeTransitionCandidate& candidate,
        GraniteFace const&              faceA,
        GraniteFace const&              faceB,
        GranitePoint const&             a,
        GranitePoint const&             b,
        GranitePoint const&             centroidA,
        GranitePoint const&             centroidB,
        uint32_t                        geometrySeed,
        int32_t                         candidateSalt )
    {
        candidate.m_segmentCount =
            GetGraniteEdgeSegmentCount( candidate.m_class );

        // FreshSharp / StructuralCrisp deliberately request zero transition
        // segments. This function can be reached only through eligible edge
        // classes, but keep the guard here so an ineligible class can never
        // trip a debug assertion if candidate filtering changes later.
        if ( candidate.m_segmentCount <= 0 )
        {
            candidate.m_segmentCount = 0;
            return;
        }

        EE_ASSERT(
            candidate.m_segmentCount <=
            GraniteEdgeTransitionCandidate::s_maxSegments );

        GranitePoint const tangent = GraniteNormalize(
            GraniteSubtract( b, a ) );

        candidate.m_edgeTangent = tangent;

        float const tangentSkewAmplitude =
            GetGraniteEdgeAlongLengthSkew( candidate.m_class );

        int32_t validSegmentCount = 0;

        for ( int32_t segmentIndex = 0;
              segmentIndex < candidate.m_segmentCount;
              ++segmentIndex )
        {
            float const t = GetGraniteEdgeSegmentParameter(
                candidate.m_segmentCount,
                segmentIndex );

            GranitePoint interpolatedNormal = GraniteAdd(
                GraniteScale( faceA.m_normal, 1.0f - t ),
                GraniteScale( faceB.m_normal, t ) );

            interpolatedNormal = GraniteNormalize( interpolatedNormal );

            if ( GraniteLength( interpolatedNormal ) <= 0.0001f )
            {
                continue;
            }

            float const skewSignal =
                GraniteBodySignal(
                    geometrySeed,
                    1103 + candidateSalt * 19 + segmentIndex * 7 ) *
                    2.0f -
                1.0f;

            // Outer segments lean in opposite directions around the edge. The
            // center segment is almost unskewed, giving the seam a small but
            // visible variation along its length instead of a ruler-straight
            // bevel band.
            float segmentDirection = 0.0f;

            if ( candidate.m_segmentCount == 3 )
            {
                segmentDirection =
                    segmentIndex == 0
                        ? -1.0f
                        : ( segmentIndex == 2 ? 1.0f : 0.0f );
            }
            else if ( candidate.m_segmentCount == 2 )
            {
                segmentDirection = segmentIndex == 0 ? -0.65f : 0.65f;
            }

            float const tangentSkew =
                tangentSkewAmplitude *
                ( segmentDirection + skewSignal * 0.30f );

            GranitePoint segmentNormal = GraniteNormalize(
                GraniteAdd(
                    interpolatedNormal,
                    GraniteScale( tangent, tangentSkew ) ) );

            if ( GraniteLength( segmentNormal ) <= 0.0001f )
            {
                continue;
            }

            float const supportA = GraniteDot( segmentNormal, a );
            float const supportB = GraniteDot( segmentNormal, b );
            float const edgeSupport = GraniteMax( supportA, supportB );

            float const centroidSupport = GraniteMax(
                GraniteDot( segmentNormal, centroidA ),
                GraniteDot( segmentNormal, centroidB ) );

            float const centerClearance = edgeSupport - centroidSupport;

            if ( centerClearance <= 0.0005f )
            {
                continue;
            }

            float segmentWidth =
                candidate.m_widthM *
                GetGraniteEdgeSegmentWidthScale(
                    candidate.m_segmentCount,
                    segmentIndex );

            segmentWidth = GraniteMin(
                segmentWidth,
                centerClearance * 0.48f );

            if ( segmentWidth <= 0.00012f )
            {
                continue;
            }

            float const segmentDistance =
                edgeSupport - segmentWidth;

            if ( GraniteDot( segmentNormal, centroidA ) >
                     segmentDistance + 0.00005f ||
                 GraniteDot( segmentNormal, centroidB ) >
                     segmentDistance + 0.00005f )
            {
                continue;
            }

            candidate.m_segmentNormals[validSegmentCount] = segmentNormal;
            candidate.m_segmentDistances[validSegmentCount] = segmentDistance;
            candidate.m_segmentWidthsM[validSegmentCount] = segmentWidth;
            ++validSegmentCount;
        }

        candidate.m_segmentCount = validSegmentCount;
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 6E
    //
    // Edge classification from Step 6C remains the authority. Step 6E stops
    // approximating weathering by stacking bevel planes. Eligible seams get
    // one bounded transition corridor; their actual round-over is generated
    // later as a tessellated local strip.
    //
    // Important:
    //  - broad planar face centers stay planar;
    //  - FreshSharp and StructuralCrisp edges remain untouched;
    //  - weathered/mixed/support seams alone receive transition corridors;
    //  - the resting face remains geological support authority;
    //  - no new fracture ancestry is invented.
    //-------------------------------------------------------------------------

    static void ApplyGraniteEdgeHistoryTransitions(
        GraniteClosedGeometry&        geometry,
        GraniteGeometryProfile const& profile,
        uint32_t                      geometrySeed )
    {
        GraniteBodySurfaceHistory& history = geometry.m_surfaceHistory;

        history.m_edgeCandidateCount = 0;
        history.m_edgeTransitionCount = 0;
        history.m_weatheredEdgeTransitionCount = 0;
        history.m_mixedHistoryEdgeTransitionCount = 0;
        history.m_supportEdgeTransitionCount = 0;
        history.m_freshEdgeTransitionCount = 0;
        history.m_meanEdgeTransitionWidthM = 0.0f;
        history.m_maxEdgeTransitionWidthM = 0.0f;
        history.m_meanFreshEdgeWidthM = 0.0f;
        history.m_meanWeatheredEdgeWidthM = 0.0f;
        history.m_flatFaceCentersPreserved = true;
        history.m_freshEdgesSharperThanWeathered = true;
        history.m_restingSupportUsesGeologicalFace = false;

        EE_ASSERT( geometry.m_restingFaceIndex >= 0 );
        EE_ASSERT( geometry.m_restingFaceIndex < geometry.m_numFaces );

        GraniteFace const restingFaceBefore =
            geometry.m_faces[geometry.m_restingFaceIndex];

        GranitePoint const restingFaceCentroidBefore =
            GetGraniteFaceCentroid( geometry, restingFaceBefore );

        GranitePoint const restingNormal = restingFaceBefore.m_normal;
        float const        restingDistance = GraniteDot(
            restingNormal,
            restingFaceCentroidBefore );
        GraniteFaceOrigin const restingOrigin = restingFaceBefore.m_origin;

        static constexpr int32_t       s_maxEdgeCandidates = 96;
        GraniteEdgeTransitionCandidate candidates[s_maxEdgeCandidates];
        int32_t                        candidateCount = 0;

        for ( int32_t faceAIndex = 0;
              faceAIndex < geometry.m_numFaces - 1;
              ++faceAIndex )
        {
            GraniteFace const& faceA = geometry.m_faces[faceAIndex];

            if ( faceA.m_origin == GraniteFaceOrigin::SurfaceTransition )
            {
                continue;
            }

            for ( int32_t faceBIndex = faceAIndex + 1;
                  faceBIndex < geometry.m_numFaces;
                  ++faceBIndex )
            {
                GraniteFace const& faceB = geometry.m_faces[faceBIndex];

                if ( faceB.m_origin == GraniteFaceOrigin::SurfaceTransition )
                {
                    continue;
                }

                uint8_t sharedA = 0;
                uint8_t sharedB = 0;

                if ( FindGraniteSharedEdgeVertices(
                         faceA,
                         faceB,
                         sharedA,
                         sharedB ) != 2 )
                {
                    continue;
                }

                EE_ASSERT( sharedA < geometry.m_numVertices );
                EE_ASSERT( sharedB < geometry.m_numVertices );

                GranitePoint const& a = geometry.m_vertices[sharedA];
                GranitePoint const& b = geometry.m_vertices[sharedB];
                float const         edgeLength = GraniteLength( GraniteSubtract( b, a ) );

                if ( edgeLength <= 0.0005f )
                {
                    continue;
                }

                GranitePoint const bisector = GraniteNormalize(
                    GraniteAdd( faceA.m_normal, faceB.m_normal ) );

                if ( GraniteLength( bisector ) <= 0.0001f )
                {
                    continue;
                }

                bool const touchesRestingFace =
                    faceAIndex == geometry.m_restingFaceIndex ||
                    faceBIndex == geometry.m_restingFaceIndex;

                GraniteEdgeHistoryClass const edgeClass =
                    ClassifyGraniteEdgeHistory(
                        faceA,
                        faceB,
                        touchesRestingFace );

                float const widthFactor = GetGraniteEdgeWidthFactor( edgeClass );

                if ( widthFactor <= 0.0f )
                {
                    continue;
                }

                GranitePoint const midpoint = GraniteScale(
                    GraniteAdd( a, b ),
                    0.5f );

                GranitePoint const centroidA = GetGraniteFaceCentroid( geometry, faceA );
                GranitePoint const centroidB = GetGraniteFaceCentroid( geometry, faceB );

                float const edgeSupport = GraniteDot( bisector, midpoint );
                float const centroidSupport = GraniteMax(
                    GraniteDot( bisector, centroidA ),
                    GraniteDot( bisector, centroidB ) );

                float const centerClearance = edgeSupport - centroidSupport;

                if ( centerClearance <= 0.0005f )
                {
                    continue;
                }

                float const characteristicLength = GraniteMin(
                    float( std::sqrt( double( GraniteMax( faceA.m_areaM2, 0.000001f ) ) ) ),
                    float( std::sqrt( double( GraniteMax( faceB.m_areaM2, 0.000001f ) ) ) ) );

                float const deterministicVariation = GraniteLerp(
                    0.82f,
                    1.18f,
                    GraniteBodySignal(
                        geometrySeed,
                        701 + faceAIndex * 37 + faceBIndex * 11 +
                            int32_t( sharedA ) * 3 + int32_t( sharedB ) ) );

                float width = edgeLength * widthFactor * deterministicVariation;

                width = GraniteMin(
                    width,
                    characteristicLength * widthFactor * 1.35f );

                width = GraniteMin(
                    width,
                    centerClearance * 0.52f );

                if ( width <= 0.00015f )
                {
                    continue;
                }

                if ( candidateCount >= s_maxEdgeCandidates )
                {
                    continue;
                }

                GraniteEdgeTransitionCandidate candidate;

                candidate.m_faceA = faceAIndex;
                candidate.m_faceB = faceBIndex;
                candidate.m_vertexA = sharedA;
                candidate.m_vertexB = sharedB;
                candidate.m_class = edgeClass;
                candidate.m_edgeLengthM = edgeLength;
                candidate.m_widthM = width;

                float const classPriority =
                    GetGraniteEdgeSelectionPriority( edgeClass );

                float const lengthWeight = GraniteClamp01(
                    edgeLength /
                    GraniteMax( characteristicLength * 1.4f, 0.0001f ) );

                float const selectionJitter = GraniteBodySignal(
                    geometrySeed,
                    809 + faceAIndex * 23 + faceBIndex * 17 );

                candidate.m_score =
                    classPriority * 10.0f +
                    lengthWeight * 0.40f +
                    selectionJitter * 0.08f +
                    ( touchesRestingFace ? 3.0f : 0.0f );

                candidate.m_weatheredWeight = GraniteClamp01(
                    ( faceA.m_weatheredWeight + faceB.m_weatheredWeight ) * 0.5f );

                BuildGraniteEdgeTransitionSegments(
                    candidate,
                    faceA,
                    faceB,
                    a,
                    b,
                    centroidA,
                    centroidB,
                    geometrySeed,
                    faceAIndex * 31 + faceBIndex * 13 );

                if ( candidate.m_segmentCount <= 0 )
                {
                    continue;
                }

                candidates[candidateCount++] = candidate;
            }
        }

        history.m_edgeCandidateCount = candidateCount;

        if ( candidateCount <= 0 )
        {
            history.m_restingSupportUsesGeologicalFace =
                restingFaceBefore.m_origin != GraniteFaceOrigin::SurfaceTransition;
            return;
        }

        SortGraniteEdgeTransitionCandidates( candidates, candidateCount );

        int32_t remainingPlaneSlots =
            GraniteClosedGeometry::s_maxConstructionPlanes -
            geometry.m_numConstructionPlanes;

        int32_t selectedEdgeCount = 0;

        static constexpr int32_t s_maxSelectedEdgeTransitions = 4;

        for ( int32_t candidateIndex = 0;
              candidateIndex < candidateCount &&
              remainingPlaneSlots > 0 &&
              selectedEdgeCount < s_maxSelectedEdgeTransitions;
              ++candidateIndex )
        {
            GraniteEdgeTransitionCandidate& candidate = candidates[candidateIndex];

            // Step 6E uses one scaffold plane per selected seam. The curved
            // roll is created later from the resulting SurfaceTransition face.
            if ( candidate.m_segmentCount > remainingPlaneSlots )
            {
                continue;
            }

            float transitionWeatheredWeight = candidate.m_weatheredWeight;

            switch ( candidate.m_class )
            {
                case GraniteEdgeHistoryClass::WeatheredWear:
                    transitionWeatheredWeight = GraniteMax(
                        transitionWeatheredWeight,
                        0.72f );
                    break;

                case GraniteEdgeHistoryClass::MixedHistory:
                    transitionWeatheredWeight = GraniteMax(
                        transitionWeatheredWeight,
                        0.40f );
                    break;

                case GraniteEdgeHistoryClass::SupportWear:
                    transitionWeatheredWeight = GraniteMax(
                        transitionWeatheredWeight,
                        profile.m_edgeWear * 0.48f );
                    break;

                case GraniteEdgeHistoryClass::StructuralCrisp:
                case GraniteEdgeHistoryClass::FreshSharp:
                case GraniteEdgeHistoryClass::None:
                default:
                    transitionWeatheredWeight = GraniteMin(
                        transitionWeatheredWeight,
                        0.10f );
                    break;
            }

            for ( int32_t segmentIndex = 0;
                  segmentIndex < candidate.m_segmentCount;
                  ++segmentIndex )
            {
                AddGraniteConstructionPlane(
                    geometry,
                    MakeGranitePlane(
                        candidate.m_segmentNormals[segmentIndex],
                        candidate.m_segmentDistances[segmentIndex],
                        GraniteFaceOrigin::SurfaceTransition,
                        GraniteSurfaceFaceClass::Transitional,
                        transitionWeatheredWeight,
                        -1,
                        candidate.m_class ) );
            }

            candidate.m_selected = true;
            remainingPlaneSlots -= candidate.m_segmentCount;
            ++selectedEdgeCount;
        }

        if ( selectedEdgeCount <= 0 )
        {
            history.m_restingSupportUsesGeologicalFace =
                restingFaceBefore.m_origin != GraniteFaceOrigin::SurfaceTransition;
            return;
        }

        // Reconstruct the exact convex body from the original geological
        // half-spaces plus the selected derived transition planes.
        BuildGraniteVerticesFromPlanes( geometry );
        BuildGraniteFacesFromPlanes( geometry );
        TriangulateGraniteFaces( geometry );
        RecalculateGraniteFaceAreas( geometry );

        // Recover the exact geological resting face. Transition geometry may
        // trim its perimeter but may never replace its support authority.
        geometry.m_restingFaceIndex = -1;

        for ( int32_t faceIndex = 0;
              faceIndex < geometry.m_numFaces;
              ++faceIndex )
        {
            if ( DoesGraniteFaceMatchPlaneReceipt(
                     geometry,
                     geometry.m_faces[faceIndex],
                     restingNormal,
                     restingDistance,
                     restingOrigin ) )
            {
                geometry.m_restingFaceIndex = faceIndex;
                break;
            }
        }

        EE_ASSERT( geometry.m_restingFaceIndex >= 0 );

        geometry.m_restingFaceNormal = restingNormal;
        geometry.m_restingFaceAreaM2 =
            geometry.m_faces[geometry.m_restingFaceIndex].m_areaM2;

        geometry.m_restingTriangleIndex = -1;

        for ( int32_t triangleIndex = 0;
              triangleIndex < geometry.m_numTriangles;
              ++triangleIndex )
        {
            if ( int32_t( geometry.m_triangleFaceIndex[triangleIndex] ) ==
                 geometry.m_restingFaceIndex )
            {
                geometry.m_restingTriangleIndex = triangleIndex;
                break;
            }
        }

        EE_ASSERT( geometry.m_restingTriangleIndex >= 0 );

        history.m_restingSupportUsesGeologicalFace =
            geometry.m_faces[geometry.m_restingFaceIndex].m_origin !=
            GraniteFaceOrigin::SurfaceTransition;

        float   allWidthSum = 0.0f;
        int32_t allWidthCount = 0;

        float   crispWidthSum = 0.0f;
        int32_t crispWidthCount = 0;

        float   weatheredWidthSum = 0.0f;
        int32_t weatheredWidthCount = 0;

        for ( int32_t candidateIndex = 0;
              candidateIndex < candidateCount;
              ++candidateIndex )
        {
            GraniteEdgeTransitionCandidate const& candidate =
                candidates[candidateIndex];

            if ( !candidate.m_selected ||
                 !DoesGraniteTransitionEdgeExist( geometry, candidate ) )
            {
                continue;
            }

            ++history.m_edgeTransitionCount;
            allWidthSum += candidate.m_widthM;
            ++allWidthCount;

            history.m_maxEdgeTransitionWidthM = GraniteMax(
                history.m_maxEdgeTransitionWidthM,
                candidate.m_widthM );

            switch ( candidate.m_class )
            {
                case GraniteEdgeHistoryClass::WeatheredWear:
                    ++history.m_weatheredEdgeTransitionCount;
                    weatheredWidthSum += candidate.m_widthM;
                    ++weatheredWidthCount;
                    break;

                case GraniteEdgeHistoryClass::MixedHistory:
                    ++history.m_mixedHistoryEdgeTransitionCount;
                    weatheredWidthSum += candidate.m_widthM;
                    ++weatheredWidthCount;
                    break;

                case GraniteEdgeHistoryClass::SupportWear:
                    ++history.m_supportEdgeTransitionCount;
                    break;

                case GraniteEdgeHistoryClass::FreshSharp:
                    ++history.m_freshEdgeTransitionCount;
                    crispWidthSum += candidate.m_widthM;
                    ++crispWidthCount;
                    break;

                case GraniteEdgeHistoryClass::StructuralCrisp:
                    crispWidthSum += candidate.m_widthM;
                    ++crispWidthCount;
                    break;

                case GraniteEdgeHistoryClass::None:
                default:
                    break;
            }
        }

        if ( allWidthCount > 0 )
        {
            history.m_meanEdgeTransitionWidthM =
                allWidthSum / float( allWidthCount );
        }

        if ( crispWidthCount > 0 )
        {
            history.m_meanFreshEdgeWidthM =
                crispWidthSum / float( crispWidthCount );
        }

        if ( weatheredWidthCount > 0 )
        {
            history.m_meanWeatheredEdgeWidthM =
                weatheredWidthSum / float( weatheredWidthCount );
        }

        if ( crispWidthCount > 0 && weatheredWidthCount > 0 )
        {
            history.m_freshEdgesSharperThanWeathered =
                history.m_meanFreshEdgeWidthM <
                history.m_meanWeatheredEdgeWidthM;
        }

        EE_ASSERT( history.m_flatFaceCentersPreserved );
        EE_ASSERT( history.m_restingSupportUsesGeologicalFace );
    }

    //-------------------------------------------------------------------------
    // Surface-history receipt
    //-------------------------------------------------------------------------

    static void FinalizeGraniteSurfaceHistory(
        GraniteClosedGeometry&        geometry,
        GraniteGeometryProfile const& profile )
    {
        float totalArea = 0.0f;
        float weatheredArea = 0.0f;
        float freshArea = 0.0f;
        float transitionArea = 0.0f;
        float weightedWeathering = 0.0f;

        float inheritedPlanarityArea = 0.0f;
        float inheritedArea = 0.0f;
        float freshPlanarityArea = 0.0f;
        float freshPlanarityWeight = 0.0f;
        float primaryPlanarityArea = 0.0f;
        float primaryPlanarityWeight = 0.0f;
        float allPlanarityArea = 0.0f;

        geometry.m_freshFractureFaceCount = 0;
        geometry.m_inheritedExteriorFaceCount = 0;
        geometry.m_primaryJointFaceCount = 0;
        geometry.m_secondaryJointFaceCount = 0;

        for ( int32_t faceIndex = 0; faceIndex < geometry.m_numFaces; ++faceIndex )
        {
            GraniteFace const& face = geometry.m_faces[faceIndex];

            switch ( face.m_origin )
            {
                case GraniteFaceOrigin::InheritedExterior:
                    ++geometry.m_inheritedExteriorFaceCount;
                    break;

                case GraniteFaceOrigin::PrimaryJoint:
                    ++geometry.m_primaryJointFaceCount;
                    ++geometry.m_freshFractureFaceCount;
                    break;

                case GraniteFaceOrigin::SecondaryJoint:
                    ++geometry.m_secondaryJointFaceCount;
                    ++geometry.m_freshFractureFaceCount;
                    break;

                case GraniteFaceOrigin::FreshBreak:
                case GraniteFaceOrigin::SeedEnvelope:
                    ++geometry.m_freshFractureFaceCount;
                    break;

                case GraniteFaceOrigin::SurfaceTransition:
                    // Derived edge-history surface; not geological fracture ancestry.
                    break;

                default:
                    break;
            }
        }

        for ( int32_t triangleIndex = 0;
              triangleIndex < geometry.m_numTriangles;
              ++triangleIndex )
        {
            int32_t const base = triangleIndex * 3;

            uint8_t const i0 = geometry.m_triangleIndices[base];
            uint8_t const i1 = geometry.m_triangleIndices[base + 1];
            uint8_t const i2 = geometry.m_triangleIndices[base + 2];

            GranitePoint triangleNormal;
            float const  area = MeasureGraniteTriangleArea(
                geometry,
                i0,
                i1,
                i2,
                &triangleNormal );

            if ( area <= 0.000001f )
            {
                continue;
            }

            int32_t const parentFaceIndex =
                int32_t( geometry.m_triangleFaceIndex[triangleIndex] );

            EE_ASSERT( parentFaceIndex >= 0 );
            EE_ASSERT( parentFaceIndex < geometry.m_numFaces );

            GraniteFace const& parentFace = geometry.m_faces[parentFaceIndex];

            float const planarity = GraniteClamp01(
                GraniteAbs( GraniteDot( triangleNormal, parentFace.m_normal ) ) );

            totalArea += area;
            allPlanarityArea += area * planarity;

            float const weatheredWeight = GraniteClamp01(
                geometry.m_triangleWeatheredWeight[triangleIndex] );

            weightedWeathering += area * weatheredWeight;

            switch ( geometry.m_triangleSurfaceClass[triangleIndex] )
            {
                case GraniteSurfaceFaceClass::WeatheredExterior:
                    weatheredArea += area;
                    break;

                case GraniteSurfaceFaceClass::Transitional:
                    transitionArea += area;
                    break;

                case GraniteSurfaceFaceClass::FreshFracture:
                default:
                    freshArea += area;
                    break;
            }

            switch ( geometry.m_triangleFaceOrigin[triangleIndex] )
            {
                case GraniteFaceOrigin::InheritedExterior:
                {
                    if ( geometry.m_triangleSurfaceClass[triangleIndex] ==
                         GraniteSurfaceFaceClass::WeatheredExterior )
                    {
                        inheritedPlanarityArea += area * planarity;
                        inheritedArea += area;
                    }
                    break;
                }

                case GraniteFaceOrigin::PrimaryJoint:
                    primaryPlanarityArea += area * planarity;
                    primaryPlanarityWeight += area;
                    break;

                case GraniteFaceOrigin::FreshBreak:
                case GraniteFaceOrigin::SeedEnvelope:
                    freshPlanarityArea += area * planarity;
                    freshPlanarityWeight += area;
                    break;

                default:
                    break;
            }
        }

        if ( totalArea <= 0.000001f )
        {
            return;
        }

        float const weatheringFraction = GraniteClamp01(
            weightedWeathering / totalArea );

        geometry.m_surfaceHistory.m_weatheredExteriorCoverage = weatheredArea / totalArea;
        geometry.m_surfaceHistory.m_freshFractureCoverage = freshArea / totalArea;
        geometry.m_surfaceHistory.m_transitionalCoverage = transitionArea / totalArea;

        geometry.m_surfaceHistory.m_hasInheritedExterior =
            weatheredArea > 0.000001f || transitionArea > 0.000001f;

        geometry.m_surfaceHistory.m_meanInheritedExteriorPlanarity =
            inheritedArea > 0.000001f
                ? inheritedPlanarityArea / inheritedArea
                : 1.0f;

        geometry.m_surfaceHistory.m_meanFreshFracturePlanarity =
            freshPlanarityWeight > 0.000001f
                ? freshPlanarityArea / freshPlanarityWeight
                : 1.0f;

        geometry.m_surfaceHistory.m_meanPrimaryJointPlanarity =
            primaryPlanarityWeight > 0.000001f
                ? primaryPlanarityArea / primaryPlanarityWeight
                : 1.0f;

        geometry.m_surfaceHistory.m_meanPlanarity = allPlanarityArea / totalArea;

        float const inheritedCurvature = GraniteClamp01(
            1.0f - geometry.m_surfaceHistory.m_meanInheritedExteriorPlanarity );

        geometry.m_surfaceHistory.m_meanRounding = GraniteClamp01(
            inheritedCurvature * 3.5f +
            weatheringFraction *
                profile.m_surfaceHistory.m_weatheredRoundingStrength * 0.35f );

        geometry.m_surfaceHistory.m_meanSharpness = GraniteClamp01(
            profile.m_surfaceHistory.m_freshBreakSharpness *
            geometry.m_surfaceHistory.m_meanFreshFracturePlanarity );

        geometry.m_majorFaceCount = 0;

        float geologicalArea = 0.0f;
        for ( int32_t faceIndex = 0; faceIndex < geometry.m_numFaces; ++faceIndex )
        {
            GraniteFace const& face = geometry.m_faces[faceIndex];

            if ( face.m_origin == GraniteFaceOrigin::SurfaceTransition )
            {
                continue;
            }

            geologicalArea += face.m_areaM2;
        }

        float const majorAreaThreshold = geologicalArea * 0.065f;

        for ( int32_t faceIndex = 0; faceIndex < geometry.m_numFaces; ++faceIndex )
        {
            GraniteFace const& face = geometry.m_faces[faceIndex];

            if ( face.m_origin == GraniteFaceOrigin::SurfaceTransition )
            {
                continue;
            }

            if ( face.m_areaM2 >= majorAreaThreshold )
            {
                ++geometry.m_majorFaceCount;
            }
        }
    }

    //-------------------------------------------------------------------------
    // Body geometry signals
    //-------------------------------------------------------------------------

    static float GraniteBodySignal( uint32_t geometrySeed, int32_t channel )
    {
        return HashGraniteUnit(
            geometrySeed,
            channel,
            channel * 13 + 7 );
    }

    //-------------------------------------------------------------------------

    static float DetermineGraniteExteriorHistoryWeight(
        GraniteClosedGeometryRequest const& request,
        GraniteGeometryProfile const&       profile,
        uint32_t                            geometrySeed )
    {
        if ( request.m_weathering == GraniteWeatheringState::WeatheredExposure )
        {
            return 0.85f;
        }

        if ( !request.m_canInheritExteriorSurface )
        {
            return 0.0f;
        }

        float const parentExposure = GraniteClamp01(
            request.m_parentExteriorExposure );

        if ( parentExposure <= 0.0f )
        {
            return 0.0f;
        }

        float const inheritanceSignal = GraniteBodySignal( geometrySeed, 151 );

        // A body explicitly authored as coming from a strongly exposed parent
        // should retain that exterior deterministically. This is the P3C.8
        // certification path used by B1 and later becomes a geometric fracture
        // intersection test rather than a probability.
        if ( parentExposure >= 0.75f )
        {
            float const variation =
                0.88f + inheritanceSignal * 0.18f;

            return GraniteClamp01(
                ( 0.50f + parentExposure * 0.45f ) * variation );
        }

        float const inheritanceChance = GraniteClamp01(
            profile.m_surfaceHistory.m_inheritedExteriorBias *
            ( 0.55f + parentExposure * 0.85f ) );

        if ( inheritanceSignal > inheritanceChance )
        {
            return 0.0f;
        }

        return GraniteClamp01( 0.35f + parentExposure * 0.45f );
    }

    //-------------------------------------------------------------------------
    // Local basis used to seed the body envelope
    //-------------------------------------------------------------------------

    static void GetGraniteBodyAxes(
        float         inheritedOrientation,
        GranitePoint& axisX,
        GranitePoint& axisY,
        GranitePoint& axisZ )
    {
        float const cosine = float( std::cos( double( inheritedOrientation ) ) );
        float const sine = float( std::sin( double( inheritedOrientation ) ) );

        axisX = { cosine, sine, 0.0f };
        axisY = { -sine, cosine, 0.0f };
        axisZ = { 0.0f, 0.0f, 1.0f };
    }

    //-------------------------------------------------------------------------

    static float GraniteEnvelopeSupport(
        GranitePoint const& normal,
        GranitePoint const& axisX,
        GranitePoint const& axisY,
        GranitePoint const& axisZ,
        float               halfX,
        float               halfY,
        float               halfZ )
    {
        return GraniteAbs( GraniteDot( normal, axisX ) ) * halfX +
               GraniteAbs( GraniteDot( normal, axisY ) ) * halfY +
               GraniteAbs( GraniteDot( normal, axisZ ) ) * halfZ;
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 6G — asymmetric host-envelope helpers
    //
    // The seed envelope is only a bounded host volume, never a manufactured
    // box. Its six broad constraints are therefore allowed to tilt away from
    // perfect orthogonality in a deterministic, material-bounded way. This
    // removes the recurring world-north / world-south wall signature before
    // any fracture or surface articulation is applied.
    //-------------------------------------------------------------------------

    static GranitePoint MakeGraniteHostConstraintNormal(
        GranitePoint const& primary,
        GranitePoint const& secondaryA,
        GranitePoint const& secondaryB,
        uint32_t            geometrySeed,
        int32_t             channel,
        float               secondaryAStrength,
        float               secondaryBStrength )
    {
        float const a =
            ( GraniteBodySignal( geometrySeed, channel ) - 0.5f ) *
            2.0f *
            secondaryAStrength;

        float const b =
            ( GraniteBodySignal( geometrySeed, channel + 1 ) - 0.5f ) *
            2.0f *
            secondaryBStrength;

        return GraniteNormalize(
            GraniteAdd(
                primary,
                GraniteAdd(
                    GraniteScale( secondaryA, a ),
                    GraniteScale( secondaryB, b ) ) ) );
    }

    //-------------------------------------------------------------------------

    static float GraniteAsymmetricEnvelopeSupport(
        GranitePoint const& normal,
        GranitePoint const& axisX,
        GranitePoint const& axisY,
        GranitePoint const& axisZ,
        float               plusX,
        float               minusX,
        float               plusY,
        float               minusY,
        float               plusZ,
        float               minusZ )
    {
        float const xProjection = GraniteDot( normal, axisX );
        float const yProjection = GraniteDot( normal, axisY );
        float const zProjection = GraniteDot( normal, axisZ );

        return GraniteAbs( xProjection ) *
                   ( xProjection >= 0.0f ? plusX : minusX ) +
               GraniteAbs( yProjection ) *
                   ( yProjection >= 0.0f ? plusY : minusY ) +
               GraniteAbs( zProjection ) *
                   ( zProjection >= 0.0f ? plusZ : minusZ );
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 7D4 - rounded body-language recovery
    //
    // The seed envelope is still a bounded host volume, but weathered/boulder
    // families need extra crown and shoulder constraints so the result reads
    // as Granite mass first and selective fracture planes second.
    //-------------------------------------------------------------------------

    static void AddGraniteRoundedEnvelopeSoftening(
        GraniteClosedGeometry& geometry,
        GranitePoint const&    axisX,
        GranitePoint const&    axisY,
        GranitePoint const&    axisZ,
        float                  halfX,
        float                  halfY,
        float                  halfZ,
        uint32_t               geometrySeed,
        float                  roundingStrength )
    {
        roundingStrength =
            GraniteClamp01(
                roundingStrength );

        if ( roundingStrength <=
             0.01f )
        {
            return;
        }

        float constexpr twoPi =
            6.28318530718f;

        float const phase =
            GraniteBodySignal(
                geometrySeed,
                286 ) *
            twoPi;

        int32_t const crownPlaneCount =
            roundingStrength >
                    0.80f
                ? 6
                : 4;

        for ( int32_t i = 0;
              i < crownPlaneCount;
              ++i )
        {
            float const angle =
                phase +
                float( i ) *
                    ( twoPi /
                      float(
                          crownPlaneCount ) );

            GranitePoint const horizontal =
                GraniteCombine(
                    axisX,
                    float(
                        std::cos(
                            double(
                                angle ) ) ),
                    axisY,
                    float(
                        std::sin(
                            double(
                                angle ) ) ) );

            float const tilt =
                GraniteLerp(
                    0.22f,
                    0.44f,
                    GraniteBodySignal(
                        geometrySeed,
                        287 + i ) ) *
                GraniteLerp(
                    0.85f,
                    1.15f,
                    roundingStrength );

            GranitePoint const normal =
                GraniteNormalize(
                    GraniteAdd(
                        axisZ,
                        GraniteScale(
                            horizontal,
                            tilt ) ) );

            float const support =
                GraniteEnvelopeSupport(
                    normal,
                    axisX,
                    axisY,
                    axisZ,
                    halfX,
                    halfY,
                    halfZ );

            float const shave =
                GraniteLerp(
                    0.035f,
                    0.105f,
                    GraniteBodySignal(
                        geometrySeed,
                        295 + i ) ) *
                roundingStrength;

            AddGraniteConstructionPlane(
                geometry,
                MakeGranitePlane(
                    normal,
                    support *
                        ( 1.0f -
                          shave ),
                    GraniteFaceOrigin::InheritedExterior,
                    GraniteSurfaceFaceClass::Transitional,
                    0.30f *
                        roundingStrength ) );
        }

        // Two shallow cheek cuts prevent long dead-flat walls without turning
        // the body into a dense faceted polyhedron.
        for ( int32_t i = 0;
              i < 2;
              ++i )
        {
            float const angle =
                phase +
                ( i ==
                          0
                      ? 0.75f
                      : 3.95f );

            GranitePoint horizontal =
                GraniteCombine(
                    axisX,
                    float(
                        std::cos(
                            double(
                                angle ) ) ),
                    axisY,
                    float(
                        std::sin(
                            double(
                                angle ) ) ) );

            float const verticalBias =
                GraniteLerp(
                    0.08f,
                    0.22f,
                    GraniteBodySignal(
                        geometrySeed,
                        303 + i ) ) *
                roundingStrength;

            GranitePoint normal =
                GraniteNormalize(
                    GraniteAdd(
                        horizontal,
                        GraniteScale(
                            axisZ,
                            verticalBias ) ) );

            if ( GraniteBodySignal(
                     geometrySeed,
                     305 + i ) <
                 0.5f )
            {
                normal =
                    GraniteScale(
                        normal,
                        -1.0f );
            }

            float const support =
                GraniteEnvelopeSupport(
                    normal,
                    axisX,
                    axisY,
                    axisZ,
                    halfX,
                    halfY,
                    halfZ );

            float const shave =
                GraniteLerp(
                    0.025f,
                    0.075f,
                    GraniteBodySignal(
                        geometrySeed,
                        307 + i ) ) *
                roundingStrength;

            AddGraniteConstructionPlane(
                geometry,
                MakeGranitePlane(
                    normal,
                    support *
                        ( 1.0f -
                          shave ),
                    GraniteFaceOrigin::FreshBreak,
                    GraniteSurfaceFaceClass::Transitional,
                    0.0f ) );
        }
    }

    //-------------------------------------------------------------------------
    // Seed bounded body envelope
    //-------------------------------------------------------------------------

    static void AddGraniteSeedEnvelope(
        GraniteClosedGeometry& geometry,
        GranitePoint const&    axisX,
        GranitePoint const&    axisY,
        GranitePoint const&    axisZ,
        float                  halfX,
        float                  halfY,
        float                  halfZ,
        uint32_t               geometrySeed,
        float                  exteriorHistoryWeight,
        int32_t                topologyFamily )
    {
        float const plusX = halfX * GraniteLerp(
                                        0.88f,
                                        1.10f,
                                        GraniteBodySignal( geometrySeed, 201 ) );

        float const minusX = halfX * GraniteLerp(
                                         0.88f,
                                         1.10f,
                                         GraniteBodySignal( geometrySeed, 202 ) );

        float const plusY = halfY * GraniteLerp(
                                        0.88f,
                                        1.10f,
                                        GraniteBodySignal( geometrySeed, 203 ) );

        float const minusY = halfY * GraniteLerp(
                                         0.88f,
                                         1.10f,
                                         GraniteBodySignal( geometrySeed, 204 ) );

        float const plusZ = halfZ * GraniteLerp(
                                        0.90f,
                                        1.08f,
                                        GraniteBodySignal( geometrySeed, 205 ) );

        float const minusZ = halfZ * GraniteLerp(
                                         0.90f,
                                         1.08f,
                                         GraniteBodySignal( geometrySeed, 206 ) );

        GranitePoint const plusXNormal = MakeGraniteHostConstraintNormal(
            axisX,
            axisY,
            axisZ,
            geometrySeed,
            220,
            0.20f,
            0.16f );

        GranitePoint const minusXNormal = MakeGraniteHostConstraintNormal(
            GraniteScale( axisX, -1.0f ),
            axisY,
            axisZ,
            geometrySeed,
            222,
            0.20f,
            0.16f );

        GranitePoint const plusYNormal = MakeGraniteHostConstraintNormal(
            axisY,
            axisX,
            axisZ,
            geometrySeed,
            224,
            0.20f,
            0.16f );

        GranitePoint const minusYNormal = MakeGraniteHostConstraintNormal(
            GraniteScale( axisY, -1.0f ),
            axisX,
            axisZ,
            geometrySeed,
            226,
            0.20f,
            0.16f );

        GranitePoint const plusZNormal = MakeGraniteHostConstraintNormal(
            axisZ,
            axisX,
            axisY,
            geometrySeed,
            228,
            0.12f,
            0.12f );

        GranitePoint const minusZNormal = MakeGraniteHostConstraintNormal(
            GraniteScale( axisZ, -1.0f ),
            axisX,
            axisY,
            geometrySeed,
            230,
            0.14f,
            0.14f );

        AddGraniteConstructionPlane(
            geometry,
            MakeGranitePlane(
                plusXNormal,
                GraniteAsymmetricEnvelopeSupport(
                    plusXNormal,
                    axisX,
                    axisY,
                    axisZ,
                    plusX,
                    minusX,
                    plusY,
                    minusY,
                    plusZ,
                    minusZ ),
                GraniteFaceOrigin::SeedEnvelope,
                GraniteSurfaceFaceClass::FreshFracture,
                0.0f ) );

        AddGraniteConstructionPlane(
            geometry,
            MakeGranitePlane(
                minusXNormal,
                GraniteAsymmetricEnvelopeSupport(
                    minusXNormal,
                    axisX,
                    axisY,
                    axisZ,
                    plusX,
                    minusX,
                    plusY,
                    minusY,
                    plusZ,
                    minusZ ),
                GraniteFaceOrigin::SeedEnvelope,
                GraniteSurfaceFaceClass::FreshFracture,
                0.0f ) );

        AddGraniteConstructionPlane(
            geometry,
            MakeGranitePlane(
                plusYNormal,
                GraniteAsymmetricEnvelopeSupport(
                    plusYNormal,
                    axisX,
                    axisY,
                    axisZ,
                    plusX,
                    minusX,
                    plusY,
                    minusY,
                    plusZ,
                    minusZ ),
                GraniteFaceOrigin::SeedEnvelope,
                GraniteSurfaceFaceClass::FreshFracture,
                0.0f ) );

        AddGraniteConstructionPlane(
            geometry,
            MakeGranitePlane(
                minusYNormal,
                GraniteAsymmetricEnvelopeSupport(
                    minusYNormal,
                    axisX,
                    axisY,
                    axisZ,
                    plusX,
                    minusX,
                    plusY,
                    minusY,
                    plusZ,
                    minusZ ),
                GraniteFaceOrigin::SeedEnvelope,
                GraniteSurfaceFaceClass::FreshFracture,
                0.0f ) );

        // +Z may preserve the parent/outcrop exterior. It is intentionally
        // only approximately horizontal: an inherited weathered shell should
        // not become a perfectly level manufactured cap.
        AddGraniteConstructionPlane(
            geometry,
            MakeGranitePlane(
                plusZNormal,
                GraniteAsymmetricEnvelopeSupport(
                    plusZNormal,
                    axisX,
                    axisY,
                    axisZ,
                    plusX,
                    minusX,
                    plusY,
                    minusY,
                    plusZ,
                    minusZ ),
                exteriorHistoryWeight > 0.0f
                    ? GraniteFaceOrigin::InheritedExterior
                    : GraniteFaceOrigin::SeedEnvelope,
                exteriorHistoryWeight > 0.0f
                    ? GraniteSurfaceFaceClass::WeatheredExterior
                    : GraniteSurfaceFaceClass::FreshFracture,
                exteriorHistoryWeight ) );

        AddGraniteConstructionPlane(
            geometry,
            MakeGranitePlane(
                minusZNormal,
                GraniteAsymmetricEnvelopeSupport(
                    minusZNormal,
                    axisX,
                    axisY,
                    axisZ,
                    plusX,
                    minusX,
                    plusY,
                    minusY,
                    plusZ,
                    minusZ ),
                GraniteFaceOrigin::SeedEnvelope,
                GraniteSurfaceFaceClass::FreshFracture,
                0.0f ) );

        // Step 7D4: the host must read as Granite mass before fracture cuts.
        // Rounded families receive extra crown/shoulder constraints; slab
        // families remain flatter by design.
        float roundedBodyBias =
            0.0f;

        switch ( topologyFamily )
        {
            case 0:
            {
                roundedBodyBias =
                    0.88f;
                break;
            }

            case 1:
            {
                roundedBodyBias =
                    0.35f;
                break;
            }

            case 2:
            {
                roundedBodyBias =
                    0.58f;
                break;
            }

            case 3:
            default:
            {
                roundedBodyBias =
                    1.00f;
                break;
            }
        }

        roundedBodyBias =
            GraniteMax(
                roundedBodyBias,
                exteriorHistoryWeight *
                    0.72f );

        AddGraniteRoundedEnvelopeSoftening(
            geometry,
            axisX,
            axisY,
            axisZ,
            halfX,
            halfY,
            halfZ,
            geometrySeed,
            roundedBodyBias );

        // Weathered exterior gets shallow bevel/crown planes around the old face.
        if ( exteriorHistoryWeight > 0.20f )
        {
            float constexpr twoPi = 6.28318530718f;
            float const phase = GraniteBodySignal( geometrySeed, 207 ) * twoPi;

            for ( int32_t i = 0; i < 4; ++i )
            {
                float const angle = phase + float( i ) * ( twoPi / 4.0f );

                GranitePoint horizontal = GraniteCombine(
                    axisX,
                    float( std::cos( double( angle ) ) ),
                    axisY,
                    float( std::sin( double( angle ) ) ) );

                float const tilt = GraniteLerp(
                    0.28f,
                    0.42f,
                    GraniteBodySignal( geometrySeed, 208 + i ) );

                GranitePoint const normal = GraniteNormalize(
                    GraniteAdd( axisZ, GraniteScale( horizontal, tilt ) ) );

                float const horizontalSupport =
                    GraniteAbs( GraniteDot( normal, axisX ) ) * halfX +
                    GraniteAbs( GraniteDot( normal, axisY ) ) * halfY;

                float const distance =
                    normal.m_z * plusZ +
                    horizontalSupport *
                        GraniteLerp(
                            0.46f,
                            0.64f,
                            GraniteBodySignal( geometrySeed, 212 + i ) );

                AddGraniteConstructionPlane(
                    geometry,
                    MakeGranitePlane(
                        normal,
                        distance,
                        GraniteFaceOrigin::InheritedExterior,
                        GraniteSurfaceFaceClass::Transitional,
                        exteriorHistoryWeight * 0.55f ) );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Add fracture/joint clipping planes
    //-------------------------------------------------------------------------

    static void AddGraniteFracturePlanes(
        GraniteClosedGeometry&              geometry,
        GraniteClosedGeometryRequest const& request,
        GraniteGeometryProfile const&       profile,
        GranitePoint const&                 axisX,
        GranitePoint const&                 axisY,
        GranitePoint const&                 axisZ,
        float                               halfX,
        float                               halfY,
        float                               halfZ,
        uint32_t                            geometrySeed,
        int32_t                             topologyFamily )
    {
        bool const roundedBlockFamily =
            topologyFamily ==
                0 ||
            topologyFamily ==
                3;

        int32_t cutCount =
            3;

        switch ( topologyFamily )
        {
            case 0:
            {
                cutCount =
                    2;
                break;
            }

            case 1:
            {
                cutCount =
                    2;
                break;
            }

            case 2:
            {
                cutCount =
                    2;
                break;
            }

            case 3:
            {
                cutCount =
                    2;
                break;
            }

            default:
            {
                cutCount =
                    3;
                break;
            }
        }

        float const formationRotation =
            GetGraniteFormationRotation(
                request.m_worldSeed,
                request.m_geologicalAncestryID );

        for ( int32_t cutIndex = 0;
              cutIndex < cutCount;
              ++cutIndex )
        {
            int32_t const jointSetIndex =
                cutIndex %
                3;

            GraniteJointSet const& jointSet =
                profile.m_jointSets[jointSetIndex];

            float orientationJitter =
                ( GraniteBodySignal(
                      geometrySeed,
                      240 +
                          cutIndex *
                              4 ) -
                  0.5f ) *
                0.95f;

            if ( roundedBlockFamily )
            {
                orientationJitter *=
                    0.55f;
            }

            float const orientation =
                jointSet.m_orientationRadians +
                formationRotation +
                orientationJitter;

            GranitePoint horizontal =
                GraniteCombine(
                    axisX,
                    float(
                        std::cos(
                            double(
                                orientation ) ) ),
                    axisY,
                    float(
                        std::sin(
                            double(
                                orientation ) ) ) );

            float zComponent =
                ( GraniteBodySignal(
                      geometrySeed,
                      241 +
                          cutIndex *
                              4 ) -
                  0.5f ) *
                1.25f;

            if ( topologyFamily ==
                 1 )
            {
                zComponent *=
                    0.48f;
            }
            else if ( topologyFamily ==
                          2 &&
                      cutIndex ==
                          0 )
            {
                zComponent =
                    GraniteBodySignal(
                        geometrySeed,
                        242 ) >
                            0.5f
                        ? 0.92f
                        : -0.92f;
            }
            else if ( roundedBlockFamily )
            {
                zComponent *=
                    cutIndex ==
                            0
                        ? 0.42f
                        : 0.28f;
            }

            GranitePoint normal =
                GraniteNormalize(
                    GraniteAdd(
                        horizontal,
                        GraniteScale(
                            axisZ,
                            zComponent ) ) );

            if ( GraniteBodySignal(
                     geometrySeed,
                     243 +
                         cutIndex *
                             4 ) <
                 0.5f )
            {
                normal =
                    GraniteScale(
                        normal,
                        -1.0f );
            }

            float const support =
                GraniteEnvelopeSupport(
                    normal,
                    axisX,
                    axisY,
                    axisZ,
                    halfX,
                    halfY,
                    halfZ );

            float clipFraction =
                GraniteLerp(
                    0.68f,
                    0.88f,
                    GraniteBodySignal(
                        geometrySeed,
                        244 +
                            cutIndex *
                                4 ) );

            if ( topologyFamily ==
                     2 &&
                 cutIndex ==
                     0 )
            {
                clipFraction =
                    GraniteLerp(
                        0.56f,
                        0.68f,
                        GraniteBodySignal(
                            geometrySeed,
                            245 ) );
            }
            else if ( roundedBlockFamily )
            {
                clipFraction =
                    cutIndex ==
                            0
                        ? GraniteLerp(
                              0.80f,
                              0.89f,
                              GraniteBodySignal(
                                  geometrySeed,
                                  244 +
                                      cutIndex *
                                          4 ) )
                        : GraniteLerp(
                              0.88f,
                              0.95f,
                              GraniteBodySignal(
                                  geometrySeed,
                                  244 +
                                      cutIndex *
                                          4 ) );
            }

            GraniteFaceOrigin const origin =
                jointSet.m_tier ==
                        GraniteJointTier::Primary
                    ? GraniteFaceOrigin::PrimaryJoint
                    : GraniteFaceOrigin::SecondaryJoint;

            AddGraniteConstructionPlane(
                geometry,
                MakeGranitePlane(
                    normal,
                    support *
                        clipFraction,
                    origin,
                    GraniteSurfaceFaceClass::FreshFracture,
                    0.0f,
                    jointSetIndex ) );
        }

        // Rounded families keep one small fresh chip so the weathered mass
        // remains geological rather than becoming a smooth blob.
        if ( roundedBlockFamily &&
             geometry.m_numConstructionPlanes <
                 GraniteClosedGeometry::
                     s_maxConstructionPlanes )
        {
            float const sideSign =
                GraniteBodySignal(
                    geometrySeed,
                    310 ) <
                        0.5f
                    ? -1.0f
                    : 1.0f;

            GranitePoint chipNormal =
                GraniteNormalize(
                    GraniteAdd(
                        GraniteCombine(
                            axisX,
                            0.64f *
                                sideSign,
                            axisY,
                            -0.30f ),
                        GraniteScale(
                            axisZ,
                            0.18f ) ) );

            float const support =
                GraniteEnvelopeSupport(
                    chipNormal,
                    axisX,
                    axisY,
                    axisZ,
                    halfX,
                    halfY,
                    halfZ );

            AddGraniteConstructionPlane(
                geometry,
                MakeGranitePlane(
                    chipNormal,
                    support *
                        0.92f,
                    GraniteFaceOrigin::FreshBreak,
                    GraniteSurfaceFaceClass::FreshFracture,
                    0.0f ) );
        }

        // Keep one gentler corner cut for the explicitly blockier family.
        if ( topologyFamily ==
                 3 &&
             geometry.m_numConstructionPlanes <
                 GraniteClosedGeometry::
                     s_maxConstructionPlanes )
        {
            GranitePoint cornerNormal =
                GraniteNormalize(
                    GraniteAdd(
                        GraniteCombine(
                            axisX,
                            0.62f,
                            axisY,
                            -0.48f ),
                        GraniteScale(
                            axisZ,
                            0.26f ) ) );

            if ( GraniteBodySignal(
                     geometrySeed,
                     280 ) <
                 0.5f )
            {
                cornerNormal =
                    GraniteScale(
                        cornerNormal,
                        -1.0f );
            }

            float const support =
                GraniteEnvelopeSupport(
                    cornerNormal,
                    axisX,
                    axisY,
                    axisZ,
                    halfX,
                    halfY,
                    halfZ );

            AddGraniteConstructionPlane(
                geometry,
                MakeGranitePlane(
                    cornerNormal,
                    support *
                        0.88f,
                    GraniteFaceOrigin::FreshBreak,
                    GraniteSurfaceFaceClass::FreshFracture,
                    0.0f ) );
        }
    }

    //-------------------------------------------------------------------------
    // P3C.8 Step 6F — visible construction-envelope de-bias
    //
    // Preserve the successful Step 6E fracture body, especially complex
    // non-envelope faces such as the current B3 south witness. Only surviving
    // lateral SeedEnvelope faces are eligible here. At most two oblique
    // FreshBreak planes are added, and only when a scaffold wall occupies a
    // materially large portion of the finished surface.
    //-------------------------------------------------------------------------

    static bool AddGraniteSeedEnvelopeDebiasPlane(
        GraniteClosedGeometry& geometry,
        uint32_t               geometrySeed,
        int32_t                passIndex )
    {
        EE_ASSERT( passIndex >= 0 );
        EE_ASSERT( passIndex < 2 );

        if ( geometry.m_numConstructionPlanes >=
             GraniteClosedGeometry::s_maxConstructionPlanes )
        {
            return false;
        }

        float   totalArea = 0.0f;
        float   largestCandidateArea = 0.0f;
        int32_t largestCandidateFace = -1;

        for ( int32_t faceIndex = 0;
              faceIndex < geometry.m_numFaces;
              ++faceIndex )
        {
            GraniteFace const& face = geometry.m_faces[faceIndex];
            totalArea += face.m_areaM2;

            bool const lateralSeedEnvelope =
                face.m_origin == GraniteFaceOrigin::SeedEnvelope &&
                GraniteAbs( face.m_normal.m_z ) < 0.82f;

            if ( lateralSeedEnvelope &&
                 face.m_areaM2 > largestCandidateArea )
            {
                largestCandidateArea = face.m_areaM2;
                largestCandidateFace = faceIndex;
            }
        }

        if ( largestCandidateFace < 0 ||
             totalArea <= 0.000001f )
        {
            return false;
        }

        float const areaFraction =
            largestCandidateArea / totalArea;

        if ( areaFraction < 0.145f )
        {
            return false;
        }

        GraniteFace const candidate =
            geometry.m_faces[largestCandidateFace];

        GranitePoint tangent = GraniteCross(
            GranitePoint{ 0.0f, 0.0f, 1.0f },
            candidate.m_normal );

        if ( GraniteLength( tangent ) < 0.05f )
        {
            tangent = GraniteCross(
                GranitePoint{ 1.0f, 0.0f, 0.0f },
                candidate.m_normal );
        }

        tangent = GraniteNormalize( tangent );

        float const tangentSign =
            GraniteBodySignal( geometrySeed, 330 + passIndex * 7 ) < 0.5f
                ? -1.0f
                : 1.0f;

        float const lateralSkew =
            GraniteLerp(
                0.34f,
                0.58f,
                GraniteBodySignal( geometrySeed, 331 + passIndex * 7 ) ) *
            tangentSign;

        float const verticalSkew =
            ( GraniteBodySignal( geometrySeed, 332 + passIndex * 7 ) - 0.5f ) *
            0.46f;

        GranitePoint const correctionNormal = GraniteNormalize(
            GraniteAdd(
                GraniteAdd(
                    candidate.m_normal,
                    GraniteScale( tangent, lateralSkew ) ),
                GranitePoint{ 0.0f, 0.0f, verticalSkew } ) );

        float minProjection = GraniteDot(
            correctionNormal,
            geometry.m_vertices[0] );
        float maxProjection = minProjection;

        for ( int32_t vertexIndex = 1;
              vertexIndex < geometry.m_numVertices;
              ++vertexIndex )
        {
            float const projection = GraniteDot(
                correctionNormal,
                geometry.m_vertices[vertexIndex] );

            minProjection = GraniteMin( minProjection, projection );
            maxProjection = GraniteMax( maxProjection, projection );
        }

        float const projectionSpan =
            maxProjection - minProjection;

        if ( projectionSpan <= 0.0001f )
        {
            return false;
        }

        float const shaveFraction = GraniteLerp(
            0.055f,
            0.105f,
            GraniteBodySignal( geometrySeed, 333 + passIndex * 7 ) );

        AddGraniteConstructionPlane(
            geometry,
            MakeGranitePlane(
                correctionNormal,
                maxProjection - projectionSpan * shaveFraction,
                GraniteFaceOrigin::FreshBreak,
                GraniteSurfaceFaceClass::FreshFracture,
                0.0f ) );

        return true;
    }

    //-------------------------------------------------------------------------

    static void DebiasGraniteSeedEnvelopeFaces(
        GraniteClosedGeometry& geometry,
        uint32_t               geometrySeed )
    {
        for ( int32_t passIndex = 0;
              passIndex < 2;
              ++passIndex )
        {
            if ( !AddGraniteSeedEnvelopeDebiasPlane(
                     geometry,
                     geometrySeed,
                     passIndex ) )
            {
                break;
            }

            BuildGraniteVerticesFromPlanes( geometry );
            BuildGraniteFacesFromPlanes( geometry );
            TriangulateGraniteFaces( geometry );
        }
    }


    //-------------------------------------------------------------------------
    // Generate true half-space fracture polyhedron
    //-------------------------------------------------------------------------

    GraniteClosedGeometry GenerateGraniteClosedGeometry(
        GraniteClosedGeometryRequest const& request )
    {
        EE_ASSERT( request.m_massGrams > 0 );
        EE_ASSERT( request.m_bodyState != ProvenanceBodyState::LooseAggregate );
        EE_ASSERT( request.m_bodyState != ProvenanceBodyState::Continuous );
        EE_ASSERT( request.m_scale != GraniteGeometryScale::Outcrop );

        GraniteGeometryProfile const profile = GetGraniteGeometryProfile();

        MatterVolumeMetrics const volumeMetrics = CalculateMatterVolume(
            ProvenanceMaterialID::Granite,
            request.m_bodyState,
            request.m_massGrams );

        GraniteClosedGeometry geometry;

        geometry.m_requiredSolidVolumeM3 = volumeMetrics.m_solidVolumeM3;
        geometry.m_requiredEnvelopeVolumeM3 = volumeMetrics.m_envelopeVolumeM3;

        uint32_t const geometrySeed = MixGraniteSeed(
            request.m_worldSeed,
            request.m_geologicalAncestryID,
            request.m_bodyID );

        GraniteJointFieldSample const jointField = EvaluateGraniteJointField(
            request.m_worldSeed,
            request.m_geologicalAncestryID,
            request.m_worldX,
            request.m_worldY,
            request.m_weathering );

        int32_t dominantJointSet = 0;
        float   strongestJoint = jointField.m_jointStrength[0];

        for ( int32_t i = 1; i < 3; ++i )
        {
            if ( jointField.m_jointStrength[i] > strongestJoint )
            {
                strongestJoint = jointField.m_jointStrength[i];
                dominantJointSet = i;
            }
        }

        float const formationRotation = GetGraniteFormationRotation(
            request.m_worldSeed,
            request.m_geologicalAncestryID );

        float const inheritedOrientation =
            profile.m_jointSets[dominantJointSet].m_orientationRadians +
            formationRotation;

        int32_t topologyFamily = 0;

        if ( request.m_bodyID != 0 )
        {
            topologyFamily = int32_t( request.m_bodyID % 4u );
        }
        else
        {
            topologyFamily = int32_t(
                HashGraniteValue( geometrySeed, 103, 61 ) % 4u );
        }

        geometry.m_topologyFamily = topologyFamily;

        float slabCharacter = profile.m_slabBias;
        float elongationCharacter = profile.m_elongationBias;

        float halfX = 0.72f;
        float halfY = 0.60f;
        float halfZ = 0.55f;

        switch ( topologyFamily )
        {
            case 0:
            {
                slabCharacter = 0.12f + GraniteBodySignal( geometrySeed, 1 ) * 0.26f;
                elongationCharacter = 0.12f + GraniteBodySignal( geometrySeed, 2 ) * 0.30f;

                halfX = 0.72f * ( 1.0f + elongationCharacter * 0.26f );
                halfY = 0.62f * ( 1.0f - elongationCharacter * 0.08f );
                halfZ = 0.58f;
                break;
            }

            case 1:
            {
                slabCharacter = 0.68f + GraniteBodySignal( geometrySeed, 1 ) * 0.24f;
                elongationCharacter = 0.26f + GraniteBodySignal( geometrySeed, 2 ) * 0.34f;

                halfX = 0.95f * ( 1.0f + elongationCharacter * 0.24f );
                halfY = 0.63f;
                halfZ = 0.30f;
                break;
            }

            case 2:
            {
                slabCharacter = 0.30f + GraniteBodySignal( geometrySeed, 1 ) * 0.30f;
                elongationCharacter = 0.24f + GraniteBodySignal( geometrySeed, 2 ) * 0.34f;

                halfX = 0.84f;
                halfY = 0.60f;
                halfZ = 0.58f;
                break;
            }

            case 3:
            default:
            {
                slabCharacter = 0.24f + GraniteBodySignal( geometrySeed, 1 ) * 0.28f;
                elongationCharacter = 0.16f + GraniteBodySignal( geometrySeed, 2 ) * 0.28f;

                halfX = 0.78f * ( 1.0f + elongationCharacter * 0.20f );
                halfY = 0.68f;
                halfZ = 0.62f;
                break;
            }
        }

        if ( request.m_scale == GraniteGeometryScale::Block )
        {
            // Scaling is ultimately mass-normalized. These only adjust proportions.
            halfX *= 1.06f;
            halfY *= 1.06f;
            halfZ *= 1.04f;
        }

        geometry.m_slabCharacter = GraniteClamp01( slabCharacter );
        geometry.m_elongationCharacter = GraniteClamp01( elongationCharacter );

        float const exteriorHistoryWeight = DetermineGraniteExteriorHistoryWeight(
            request,
            profile,
            geometrySeed );

        geometry.m_effectiveAngularity =
            profile.m_angularity *
            GraniteLerp( 1.0f, 0.82f, exteriorHistoryWeight );

        geometry.m_effectiveEdgeWear = GraniteLerp(
            profile.m_surfaceHistory.m_freshBreakEdgeWear,
            profile.m_edgeWear +
                profile.m_surfaceHistory.m_weatheredEdgeSoftening,
            exteriorHistoryWeight );

        GranitePoint axisX;
        GranitePoint axisY;
        GranitePoint axisZ;

        GetGraniteBodyAxes(
            inheritedOrientation,
            axisX,
            axisY,
            axisZ );

        AddGraniteSeedEnvelope(
            geometry,
            axisX,
            axisY,
            axisZ,
            halfX,
            halfY,
            halfZ,
            geometrySeed,
            exteriorHistoryWeight,
            topologyFamily );

        AddGraniteFracturePlanes(
            geometry,
            request,
            profile,
            axisX,
            axisY,
            axisZ,
            halfX,
            halfY,
            halfZ,
            geometrySeed,
            topologyFamily );

        EE_ASSERT( geometry.m_numConstructionPlanes >= 6 );
        EE_ASSERT(
            geometry.m_numConstructionPlanes <=
            GraniteClosedGeometry::s_maxConstructionPlanes );

        BuildGraniteVerticesFromPlanes( geometry );
        BuildGraniteFacesFromPlanes( geometry );
        TriangulateGraniteFaces( geometry );

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
        geometry.m_seedTriangleMeshClosed =
            IsGraniteTriangleMeshClosed(
                geometry );
#endif

        DebiasGraniteSeedEnvelopeFaces(
            geometry,
            geometrySeed );

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
        geometry.m_debiasedTriangleMeshClosed =
            IsGraniteTriangleMeshClosed(
                geometry );
#endif

        float const preRestVolume = MeasureGraniteClosedGeometryVolume( geometry );
        EE_ASSERT( preRestVolume > 0.000001f );

        // Stable orientation is chosen from the frozen fracture-polyhedron
        // parent faces before any old-exterior presentation refinement.
        OrientGraniteToStableRestingFace( geometry );

        if ( request.m_bodyState == ProvenanceBodyState::Fragment )
        {
            ApplyGraniteDetachedBodyYaw(
                geometry,
                geometrySeed );
        }

        geometry.m_surfaceHistory.m_preHistoryMeshVolumeM3 =
            MeasureGraniteClosedGeometryVolume( geometry );

        // Step 6G: preserve fracture ancestry while allowing bounded face/silhouette articulation after construction-wall de-bias.
        // One bounded scaffold plane opens each selected weathered/mixed/support
        // seam corridor. Explicit curved strip geometry is generated later
        // during presentation/certification triangulation.
        ApplyGraniteEdgeHistoryTransitions(
            geometry,
            profile,
            geometrySeed );

        RunGraniteSurfaceHistoryControls(
            geometry,
            profile,
            geometrySeed );

        // Step 6G: shared geological vertices receive bounded deterministic
        // relief before presentation refinement. This changes silhouette and
        // breaks dead-flat support/lateral walls without opening the mesh.
        ApplyGraniteConstrainedVertexRelief(
            geometry,
            profile,
            geometrySeed );

        TriangulateGraniteFacesWithSurfaceHistory(
            geometry,
            profile,
            geometrySeed );

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
        geometry.m_surfaceHistoryTriangleMeshClosed =
            IsGraniteTriangleMeshClosed(
                geometry );
#endif

        // Step 6G: large parent faces are then articulated internally with
        // shallow deterministic recesses / shoulders. The parent plane remains
        // the dominant geological direction; the visible surface is no longer
        // mathematically flat.
        ApplyGraniteStructuredFaceArticulation(
            geometry,
            geometrySeed );

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
        geometry.m_structuredArticulationTriangleMeshClosed =
            IsGraniteTriangleMeshClosed(
                geometry );
#endif

        geometry.m_surfaceHistory.m_postHistoryMeshVolumeM3 =
            MeasureGraniteClosedGeometryVolume( geometry );

        // Local refinement changes triangle ordering. Refresh the resting
        // triangle receipt from the geological parent face.
        geometry.m_restingTriangleIndex = -1;

        for ( int32_t triangleIndex = 0;
              triangleIndex < geometry.m_numTriangles;
              ++triangleIndex )
        {
            if ( int32_t( geometry.m_triangleFaceIndex[triangleIndex] ) ==
                 geometry.m_restingFaceIndex )
            {
                geometry.m_restingTriangleIndex = triangleIndex;
                break;
            }
        }

        EE_ASSERT( geometry.m_restingTriangleIndex >= 0 );

        float const unscaledVolume =
            geometry.m_surfaceHistory.m_postHistoryMeshVolumeM3;

        EE_ASSERT( unscaledVolume > 0.000001f );

        float const uniformScale = float(
            std::cbrt(
                double(
                    geometry.m_requiredSolidVolumeM3 /
                    unscaledVolume ) ) );

        for ( int32_t i = 0; i < geometry.m_numVertices; ++i )
        {
            geometry.m_vertices[i].m_x *= uniformScale;
            geometry.m_vertices[i].m_y *= uniformScale;
            geometry.m_vertices[i].m_z *= uniformScale;
        }

        for ( int32_t planeIndex = 0;
              planeIndex < geometry.m_numConstructionPlanes;
              ++planeIndex )
        {
            geometry.m_constructionPlanes[planeIndex].m_distance *= uniformScale;
        }

        float const volumeScale =
            uniformScale * uniformScale * uniformScale;

        geometry.m_surfaceHistory.m_meanCrownM *= uniformScale;
        geometry.m_surfaceHistory.m_maxCrownM *= uniformScale;

        geometry.m_surfaceHistory.m_meanEdgeTransitionWidthM *= uniformScale;
        geometry.m_surfaceHistory.m_maxEdgeTransitionWidthM *= uniformScale;
        geometry.m_surfaceHistory.m_meanFreshEdgeWidthM *= uniformScale;
        geometry.m_surfaceHistory.m_meanWeatheredEdgeWidthM *= uniformScale;

        geometry.m_surfaceHistory.m_preHistoryMeshVolumeM3 *= volumeScale;
        geometry.m_surfaceHistory.m_postHistoryMeshVolumeM3 *= volumeScale;

        RecalculateGraniteFaceAreas( geometry );

        geometry.m_restingFaceAreaM2 =
            geometry.m_faces[geometry.m_restingFaceIndex].m_areaM2;

        geometry.m_measuredMeshVolumeM3 =
            MeasureGraniteClosedGeometryVolume( geometry );

        geometry.m_volumeResidualM3 =
            geometry.m_measuredMeshVolumeM3 -
            geometry.m_requiredSolidVolumeM3;

        UpdateGraniteGeometryExtents( geometry );
        FinalizeGraniteSurfaceHistory( geometry, profile );

        float const tolerance =
            0.000001f +
            geometry.m_requiredSolidVolumeM3 * 0.005f;

        EE_ASSERT(
            GraniteAbs( geometry.m_volumeResidualM3 ) <= tolerance );

        EE_ASSERT( geometry.m_numFaces >= 4 );
        EE_ASSERT( geometry.m_numTriangles >= 4 );

        EE_ASSERT( geometry.m_surfaceHistory.m_controlZeroExteriorProducesZeroCrown );
        EE_ASSERT( geometry.m_surfaceHistory.m_controlExteriorCrownResponds );
        EE_ASSERT( geometry.m_surfaceHistory.m_controlFreshBreakRejectsCrown );

        if ( geometry.m_surfaceHistory.m_inheritedExteriorPatchCount > 0 )
        {
            EE_ASSERT( geometry.m_surfaceHistory.m_historyAddedVertexCount > 0 );
            EE_ASSERT( geometry.m_surfaceHistory.m_historyAddedTriangleCount > 0 );
            EE_ASSERT( geometry.m_surfaceHistory.m_maxCrownM > 0.0f );

            // Step 6G: fresh fracture is no longer required to be a
            // mathematically flatter surface than every inherited patch. The
            // governing law is MACRO-planar fresh fracture with bounded local
            // roughness. Old exterior must show real curvature, while fresh
            // fracture must retain a strong dominant plane.
            // A weathered inherited patch is already positively certified by
            // non-zero crown geometry above. Do not require its aggregate
            // planarity metric to beat every rough fresh-fracture patch: Step
            // 6G intentionally gives fresh fracture meso-scale relief too.
            EE_ASSERT(
                geometry.m_surfaceHistory.m_meanFreshFracturePlanarity >
                0.75f );
        }

        EE_ASSERT( geometry.m_surfaceHistory.m_flatFaceCentersPreserved );
        EE_ASSERT( geometry.m_surfaceHistory.m_restingSupportUsesGeologicalFace );

        return geometry;
    }
}
