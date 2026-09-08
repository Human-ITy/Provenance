#pragma once

#include "GraniteGeometryInternal.h"

// Private mesh helpers shared by solid construction and exact fracture.
namespace EE::GraniteMeshInternal
{
    using namespace GraniteGeometryInternal;

    static float MeasureGraniteTriangleArea(
        GraniteClosedGeometry const& geometry,
        uint8_t                      i0,
        uint8_t                      i1,
        uint8_t                      i2,
        GranitePoint*                pNormal = nullptr )
    {
        EE_ASSERT( i0 < geometry.m_numVertices );
        EE_ASSERT( i1 < geometry.m_numVertices );
        EE_ASSERT( i2 < geometry.m_numVertices );

        GranitePoint const& a = geometry.m_vertices[i0];
        GranitePoint const& b = geometry.m_vertices[i1];
        GranitePoint const& c = geometry.m_vertices[i2];

        GranitePoint const edge1 = GraniteSubtract( b, a );
        GranitePoint const edge2 = GraniteSubtract( c, a );
        GranitePoint const cross = GraniteCross( edge1, edge2 );
        float const        crossLength = GraniteLength( cross );

        if ( pNormal != nullptr )
        {
            *pNormal = crossLength > 0.000001f
                         ? GraniteScale( cross, 1.0f / crossLength )
                         : GranitePoint();
        }

        return crossLength * 0.5f;
    }

    static void UpdateGraniteGeometryExtents( GraniteClosedGeometry& geometry )
    {
        EE_ASSERT( geometry.m_numVertices > 0 );

        float minX = geometry.m_vertices[0].m_x;
        float maxX = minX;
        float minY = geometry.m_vertices[0].m_y;
        float maxY = minY;
        float minZ = geometry.m_vertices[0].m_z;
        float maxZ = minZ;

        for ( int32_t i = 1; i < geometry.m_numVertices; ++i )
        {
            GranitePoint const& v = geometry.m_vertices[i];

            minX = GraniteMin( minX, v.m_x );
            maxX = GraniteMax( maxX, v.m_x );
            minY = GraniteMin( minY, v.m_y );
            maxY = GraniteMax( maxY, v.m_y );
            minZ = GraniteMin( minZ, v.m_z );
            maxZ = GraniteMax( maxZ, v.m_z );
        }

        geometry.m_extentXM = maxX - minX;
        geometry.m_extentYM = maxY - minY;
        geometry.m_extentZM = maxZ - minZ;
    }

    struct GraniteFractureEdgeUse
    {
        uint8_t m_low = 0;
        uint8_t m_high = 0;
        uint8_t m_count = 0;
    };

    static bool IsGraniteTriangleMeshClosed(
        GraniteClosedGeometry const& geometry )
    {
        static constexpr int32_t s_maxEdges =
            GraniteClosedGeometry::s_maxTriangles *
            3;

        GraniteFractureEdgeUse edges[s_maxEdges];

        int32_t numEdges =
            0;

        for ( int32_t triangleIndex = 0;
              triangleIndex <
              geometry.m_numTriangles;
              ++triangleIndex )
        {
            int32_t const base =
                triangleIndex *
                3;

            uint8_t const triangleVertices[3] =
                {
                    geometry.m_triangleIndices[base + 0],
                    geometry.m_triangleIndices[base + 1],
                    geometry.m_triangleIndices[base + 2] };

            for ( int32_t edgeIndex = 0;
                  edgeIndex <
                  3;
                  ++edgeIndex )
            {
                uint8_t const a =
                    triangleVertices[edgeIndex];

                uint8_t const b =
                    triangleVertices[(
                                         edgeIndex +
                                         1 ) %
                                     3];

                uint8_t const low =
                    a <
                            b
                        ? a
                        : b;

                uint8_t const high =
                    a <
                            b
                        ? b
                        : a;

                int32_t foundEdge =
                    -1;

                for ( int32_t existingIndex = 0;
                      existingIndex <
                      numEdges;
                      ++existingIndex )
                {
                    if ( edges[existingIndex].m_low ==
                             low &&
                         edges[existingIndex].m_high ==
                             high )
                    {
                        foundEdge =
                            existingIndex;

                        break;
                    }
                }

                if ( foundEdge <
                     0 )
                {
                    EE_ASSERT(
                        numEdges <
                        s_maxEdges );

                    if ( numEdges >=
                         s_maxEdges )
                    {
                        return false;
                    }

                    foundEdge =
                        numEdges++;

                    edges[foundEdge].m_low =
                        low;

                    edges[foundEdge].m_high =
                        high;

                    edges[foundEdge].m_count =
                        0;
                }

                if ( edges[foundEdge].m_count >=
                     2 )
                {
                    return false;
                }

                ++edges[foundEdge].m_count;
            }
        }

        if ( numEdges <
             6 )
        {
            return false;
        }

        for ( int32_t edgeIndex = 0;
              edgeIndex <
              numEdges;
              ++edgeIndex )
        {
            if ( edges[edgeIndex].m_count !=
                 2 )
            {
                return false;
            }
        }

        return true;
    }
}
