#include "GraniteAuthorityVerifier.h"
#include "Game/Provenance/Geometry/GraniteExcavation.h"

#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <map>

namespace EE
{
    namespace
    {
        static constexpr uint32_t s_graniteAuthorityAncestryID = 7001u;
        static constexpr uint32_t s_graniteFingerprintSchemaVersion = 1u;

        struct GraniteCanonicalFingerprintBuilder
        {
            uint64_t m_low = 14695981039346656037ull;
            uint64_t m_high = 1099511628211ull ^ 0x9E3779B97F4A7C15ull;

            void AppendByte( uint8_t value )
            {
                m_low ^= uint64_t( value );
                m_low *= 1099511628211ull;

                m_high ^= uint64_t( value ) +
                    0x9E3779B97F4A7C15ull +
                    ( m_high << 6 ) +
                    ( m_high >> 2 );
                m_high *= 0xD6E8FEB86659FD93ull;
            }

            void AppendU16( uint16_t value )
            {
                AppendByte( uint8_t( value & 0xFFu ) );
                AppendByte( uint8_t( ( value >> 8 ) & 0xFFu ) );
            }

            void AppendU32( uint32_t value )
            {
                for ( int32_t byteIndex = 0; byteIndex < 4; ++byteIndex )
                {
                    AppendByte(
                        uint8_t(
                            ( value >> ( byteIndex * 8 ) ) & 0xFFu ) );
                }
            }

            void AppendU64( uint64_t value )
            {
                for ( int32_t byteIndex = 0; byteIndex < 8; ++byteIndex )
                {
                    AppendByte(
                        uint8_t(
                            ( value >> ( byteIndex * 8 ) ) & 0xFFull ) );
                }
            }

            void AppendI32( int32_t value )
            {
                AppendU32( uint32_t( value ) );
            }

            void AppendI64( int64_t value )
            {
                AppendU64( uint64_t( value ) );
            }

            void AppendBool( bool value )
            {
                AppendByte( value ? uint8_t( 1 ) : uint8_t( 0 ) );
            }

            void AppendFloat( float value )
            {
                uint32_t bits = 0;
                static_assert(
                    sizeof( bits ) == sizeof( value ),
                    "Granite authority fingerprints require binary32 float" );
                std::memcpy( &bits, &value, sizeof( bits ) );
                AppendU32( bits );
            }
        };

        struct GraniteFractureCorpusArtifacts
        {
            GraniteClosedGeometryRequest m_parentRequest;
            GraniteClosedGeometry m_parentGeometry;
            GraniteFractureRequest m_fractureRequest;
            GraniteFractureTransactionResult m_transaction;
        };

#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
        // Layout-drift tripwires, not serialization coverage proof. These make
        // most additions/reorders to canonicalized carriers a compile-time
        // event. Explicit field serialization plus reachability controls below
        // remain the authority for what is actually fingerprinted.
#define EE_GRANITE_LAYOUT_GUARD( type, sizeBytes, alignmentBytes, finalMember, finalOffset ) \
        static_assert( sizeof( type ) == sizeBytes, "Canonical Granite carrier size changed: " #type ); \
        static_assert( alignof( type ) == alignmentBytes, "Canonical Granite carrier alignment changed: " #type ); \
        static_assert( offsetof( type, finalMember ) == finalOffset, "Canonical Granite carrier terminal layout changed: " #type )

        EE_GRANITE_LAYOUT_GUARD( GranitePoint, 12, 4, m_z, 8 );
        EE_GRANITE_LAYOUT_GUARD( GranitePlane, 32, 4, m_jointSetIndex, 28 );
        EE_GRANITE_LAYOUT_GUARD( GraniteFace, 56, 4, m_jointSetIndex, 52 );
        EE_GRANITE_LAYOUT_GUARD( GraniteBodySurfaceHistory, 124, 4, m_controlFreshBreakRejectsCrown, 121 );
        EE_GRANITE_LAYOUT_GUARD( GraniteClosedGeometry, 10860, 4, m_structuredArticulationTriangleMeshClosed, 10859 );
        EE_GRANITE_LAYOUT_GUARD( GraniteClosedGeometryRequest, 36, 4, m_worldZ, 32 );
        EE_GRANITE_LAYOUT_GUARD( GraniteFracturePlane, 32, 4, m_jointSetIndex, 28 );
        EE_GRANITE_LAYOUT_GUARD( GraniteFractureRequest, 72, 4, m_planeToleranceM, 68 );
        EE_GRANITE_LAYOUT_GUARD( GraniteFractureChildSurfaceReceipt, 28, 4, m_hasFreshFractureSurface, 25 );
        EE_GRANITE_LAYOUT_GUARD( GraniteFractureChild, 10928, 4, m_surfaceReceipt, 10900 );
        EE_GRANITE_LAYOUT_GUARD( GraniteFractureGeometricPartitionReceipt, 100, 4, m_pass, 97 );
        EE_GRANITE_LAYOUT_GUARD( GraniteFractureTransactionResult, 65808, 8, m_geometricPartitionPass, 65802 );
        EE_GRANITE_LAYOUT_GUARD( GraniteFormationFormDescriptor, 76, 4, m_primaryJointSetIndex, 72 );
        EE_GRANITE_LAYOUT_GUARD( GraniteSurfaceStoneDescriptor, 64, 4, m_downslopeBias, 60 );
        EE_GRANITE_LAYOUT_GUARD( GraniteStructuralBridgeDescriptor, 56, 4, m_severingEventID, 52 );
        EE_GRANITE_LAYOUT_GUARD( GraniteStructuralBridgeSet, 252, 4, m_numSeveredBridges, 248 );
        EE_GRANITE_LAYOUT_GUARD( GraniteDetachmentResult, 604, 4, m_pass, 600 );
        EE_GRANITE_LAYOUT_GUARD( GraniteDetachmentSequenceReceipt, 1472, 4, m_pass, 1468 );
        EE_GRANITE_LAYOUT_GUARD( GraniteRootedSeparationPolicy, 36, 4, m_surfaceCoincidenceToleranceM, 32 );
        EE_GRANITE_LAYOUT_GUARD( GraniteRootedPartitionVertex, 12, 4, m_z, 8 );
        EE_GRANITE_LAYOUT_GUARD( GraniteRootedPartitionMesh, 49180, 4, m_closed, 49176 );
        EE_GRANITE_LAYOUT_GUARD( GraniteRootedMatterPartitionReceipt, 64, 8, m_massConservationPass, 61 );
        EE_GRANITE_LAYOUT_GUARD( GraniteRootedSeparationResult, 98704, 8, m_pass, 98697 );
        EE_GRANITE_LAYOUT_GUARD( GraniteSpallResolutionPolicy, 32, 8, m_massResidualToleranceGrams, 24 );
        EE_GRANITE_LAYOUT_GUARD( GraniteSpallResolutionRequest, 72, 8, m_policy, 40 );
        EE_GRANITE_LAYOUT_GUARD( GraniteSpallPieceDescriptor, 52, 4, m_biasZ, 48 );
        EE_GRANITE_LAYOUT_GUARD( GraniteSpallResolutionReceipt, 48, 8, m_pass, 44 );
        EE_GRANITE_LAYOUT_GUARD( GraniteSpallResolutionResult, 968, 8, m_valid, 960 );
        EE_GRANITE_LAYOUT_GUARD( GraniteTerminalConservationReceipt, 104, 8, m_pass, 103 );
        EE_GRANITE_LAYOUT_GUARD( GraniteAuthorityCaseFingerprint, 24, 8, m_high, 16 );

#undef EE_GRANITE_LAYOUT_GUARD
#endif

        static void FingerprintGranitePoint(
            GraniteCanonicalFingerprintBuilder& builder,
            GranitePoint const&                  point )
        {
            builder.AppendFloat( point.m_x );
            builder.AppendFloat( point.m_y );
            builder.AppendFloat( point.m_z );
        }

        static void FingerprintGranitePlane(
            GraniteCanonicalFingerprintBuilder& builder,
            GranitePlane const&                  plane )
        {
            FingerprintGranitePoint( builder, plane.m_normal );
            builder.AppendFloat( plane.m_distance );
            builder.AppendU32( uint32_t( plane.m_origin ) );
            builder.AppendU32( uint32_t( plane.m_surfaceClass ) );
            builder.AppendFloat( plane.m_weatheredWeight );
            builder.AppendU32( uint32_t( plane.m_edgeHistoryClass ) );
            builder.AppendI32( plane.m_jointSetIndex );
        }

        static void FingerprintGraniteFace(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteFace const&                   face )
        {
            builder.AppendI32( face.m_numVertices );
            for ( int32_t vertexIndex = 0;
                  vertexIndex < face.m_numVertices;
                  ++vertexIndex )
            {
                builder.AppendByte( face.m_vertexIndices[vertexIndex] );
            }

            FingerprintGranitePoint( builder, face.m_normal );
            builder.AppendFloat( face.m_areaM2 );
            builder.AppendU32( uint32_t( face.m_origin ) );
            builder.AppendU32( uint32_t( face.m_surfaceClass ) );
            builder.AppendFloat( face.m_weatheredWeight );
            builder.AppendU32( uint32_t( face.m_edgeHistoryClass ) );
            builder.AppendI32( face.m_jointSetIndex );
        }

        static void FingerprintGraniteSurfaceHistory(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteBodySurfaceHistory const&     history )
        {
            builder.AppendFloat( history.m_weatheredExteriorCoverage );
            builder.AppendFloat( history.m_freshFractureCoverage );
            builder.AppendFloat( history.m_transitionalCoverage );
            builder.AppendFloat( history.m_meanRounding );
            builder.AppendFloat( history.m_meanSharpness );
            builder.AppendFloat( history.m_meanPlanarity );
            builder.AppendBool( history.m_hasInheritedExterior );
            builder.AppendI32( history.m_inheritedExteriorPatchCount );
            builder.AppendI32( history.m_historyAddedVertexCount );
            builder.AppendI32( history.m_historyAddedTriangleCount );
            builder.AppendI32( history.m_verticesBeforeHistory );
            builder.AppendI32( history.m_trianglesBeforeHistory );
            builder.AppendFloat( history.m_meanCrownM );
            builder.AppendFloat( history.m_maxCrownM );
            builder.AppendFloat( history.m_meanInheritedExteriorPlanarity );
            builder.AppendFloat( history.m_meanFreshFracturePlanarity );
            builder.AppendFloat( history.m_meanPrimaryJointPlanarity );
            builder.AppendFloat( history.m_preHistoryMeshVolumeM3 );
            builder.AppendFloat( history.m_postHistoryMeshVolumeM3 );
            builder.AppendI32( history.m_edgeCandidateCount );
            builder.AppendI32( history.m_edgeTransitionCount );
            builder.AppendI32( history.m_weatheredEdgeTransitionCount );
            builder.AppendI32( history.m_mixedHistoryEdgeTransitionCount );
            builder.AppendI32( history.m_supportEdgeTransitionCount );
            builder.AppendI32( history.m_freshEdgeTransitionCount );
            builder.AppendFloat( history.m_meanEdgeTransitionWidthM );
            builder.AppendFloat( history.m_maxEdgeTransitionWidthM );
            builder.AppendFloat( history.m_meanFreshEdgeWidthM );
            builder.AppendFloat( history.m_meanWeatheredEdgeWidthM );
            builder.AppendBool( history.m_flatFaceCentersPreserved );
            builder.AppendBool( history.m_freshEdgesSharperThanWeathered );
            builder.AppendBool( history.m_restingSupportUsesGeologicalFace );
            builder.AppendBool( history.m_controlZeroExteriorProducesZeroCrown );
            builder.AppendBool( history.m_controlExteriorCrownResponds );
            builder.AppendBool( history.m_controlFreshBreakRejectsCrown );
        }

        static void FingerprintGraniteClosedGeometry(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteClosedGeometry const&         geometry )
        {
            builder.AppendI32( geometry.m_numVertices );
            for ( int32_t vertexIndex = 0;
                  vertexIndex < geometry.m_numVertices;
                  ++vertexIndex )
            {
                FingerprintGranitePoint(
                    builder,
                    geometry.m_vertices[vertexIndex] );
                builder.AppendU32(
                    geometry.m_vertexConstructionPlaneMask[vertexIndex] );
            }

            builder.AppendI32( geometry.m_numFaces );
            for ( int32_t faceIndex = 0;
                  faceIndex < geometry.m_numFaces;
                  ++faceIndex )
            {
                FingerprintGraniteFace(
                    builder,
                    geometry.m_faces[faceIndex] );
            }

            builder.AppendI32( geometry.m_numTriangles );
            for ( int32_t triangleIndex = 0;
                  triangleIndex < geometry.m_numTriangles;
                  ++triangleIndex )
            {
                int32_t const indexBase = triangleIndex * 3;
                builder.AppendByte(
                    geometry.m_triangleIndices[indexBase + 0] );
                builder.AppendByte(
                    geometry.m_triangleIndices[indexBase + 1] );
                builder.AppendByte(
                    geometry.m_triangleIndices[indexBase + 2] );
                builder.AppendByte(
                    geometry.m_triangleFaceIndex[triangleIndex] );
                builder.AppendU32(
                    uint32_t(
                        geometry.m_triangleSurfaceClass[triangleIndex] ) );
                builder.AppendU32(
                    uint32_t(
                        geometry.m_triangleFaceOrigin[triangleIndex] ) );
                builder.AppendFloat(
                    geometry.m_triangleWeatheredWeight[triangleIndex] );
            }

            builder.AppendI32( geometry.m_numConstructionPlanes );
            for ( int32_t planeIndex = 0;
                  planeIndex < geometry.m_numConstructionPlanes;
                  ++planeIndex )
            {
                FingerprintGranitePlane(
                    builder,
                    geometry.m_constructionPlanes[planeIndex] );
            }

            builder.AppendI32( geometry.m_majorFaceCount );
            builder.AppendI32( geometry.m_freshFractureFaceCount );
            builder.AppendI32( geometry.m_inheritedExteriorFaceCount );
            builder.AppendI32( geometry.m_primaryJointFaceCount );
            builder.AppendI32( geometry.m_secondaryJointFaceCount );
            builder.AppendI32( geometry.m_restingFaceIndex );
            builder.AppendI32( geometry.m_restingTriangleIndex );
            FingerprintGranitePoint( builder, geometry.m_restingFaceNormal );
            builder.AppendFloat( geometry.m_restingFaceAreaM2 );
            builder.AppendFloat( geometry.m_requiredSolidVolumeM3 );
            builder.AppendFloat( geometry.m_requiredEnvelopeVolumeM3 );
            builder.AppendFloat( geometry.m_measuredMeshVolumeM3 );
            builder.AppendFloat( geometry.m_volumeResidualM3 );
            builder.AppendFloat( geometry.m_extentXM );
            builder.AppendFloat( geometry.m_extentYM );
            builder.AppendFloat( geometry.m_extentZM );
            builder.AppendFloat( geometry.m_effectiveAngularity );
            builder.AppendFloat( geometry.m_effectiveEdgeWear );
            builder.AppendFloat( geometry.m_slabCharacter );
            builder.AppendFloat( geometry.m_elongationCharacter );
            FingerprintGraniteSurfaceHistory(
                builder,
                geometry.m_surfaceHistory );
            builder.AppendI32( geometry.m_topologyFamily );
#if defined( EE_PROVENANCE_STANDALONE_AUTHORITY )
            builder.AppendBool( geometry.m_seedTriangleMeshClosed );
            builder.AppendBool( geometry.m_debiasedTriangleMeshClosed );
            builder.AppendBool( geometry.m_surfaceHistoryTriangleMeshClosed );
            builder.AppendBool(
                geometry.m_structuredArticulationTriangleMeshClosed );
#endif
        }

        static void FingerprintGraniteClosedGeometryRequest(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteClosedGeometryRequest const&  request )
        {
            builder.AppendU32( request.m_worldSeed );
            builder.AppendU32( request.m_geologicalAncestryID );
            builder.AppendU32( request.m_bodyID );
            builder.AppendU32( request.m_massGrams );
            builder.AppendU32( uint32_t( request.m_bodyState ) );
            builder.AppendU32( uint32_t( request.m_scale ) );
            builder.AppendU32( uint32_t( request.m_weathering ) );
            builder.AppendBool( request.m_canInheritExteriorSurface );
            builder.AppendFloat( request.m_parentExteriorExposure );
            builder.AppendFloat( request.m_worldX );
            builder.AppendFloat( request.m_worldY );
            builder.AppendFloat( request.m_worldZ );
        }

        static void FingerprintGraniteFracturePlane(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteFracturePlane const&          plane )
        {
            FingerprintGranitePoint( builder, plane.m_normal );
            FingerprintGranitePoint( builder, plane.m_pointOnPlane );
            builder.AppendU32( uint32_t( plane.m_origin ) );
            builder.AppendI32( plane.m_jointSetIndex );
        }

        static void FingerprintGraniteFractureRequest(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteFractureRequest const&        request )
        {
            builder.AppendU32( request.m_worldSeed );
            builder.AppendU32( request.m_eventID );
            builder.AppendU32( request.m_parentBodyID );
            builder.AppendU32( request.m_parentProvenanceID );
            builder.AppendU32( request.m_geologicalAncestryID );
            builder.AppendU32( request.m_primaryChildBodyID );
            builder.AppendU32( request.m_secondaryChildBodyID );
            builder.AppendBool( request.m_allowChips );
            builder.AppendI32( request.m_maxChipCount );
            FingerprintGraniteFracturePlane( builder, request.m_plane );
            builder.AppendFloat( request.m_planeToleranceM );
        }

        static void FingerprintGraniteFractureChildSurfaceReceipt(
            GraniteCanonicalFingerprintBuilder&       builder,
            GraniteFractureChildSurfaceReceipt const& receipt )
        {
            builder.AppendI32( receipt.m_inheritedExteriorFaceCount );
            builder.AppendI32( receipt.m_freshFractureFaceCount );
            builder.AppendI32( receipt.m_transitionalFaceCount );
            builder.AppendFloat( receipt.m_inheritedExteriorAreaM2 );
            builder.AppendFloat( receipt.m_newFractureAreaM2 );
            builder.AppendFloat( receipt.m_transitionalAreaM2 );
            builder.AppendBool( receipt.m_hasInheritedExterior );
            builder.AppendBool( receipt.m_hasFreshFractureSurface );
        }

        static void FingerprintGraniteFractureChild(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteFractureChild const&           child )
        {
            builder.AppendBool( child.m_valid );
            builder.AppendU32( uint32_t( child.m_role ) );
            builder.AppendU32( child.m_bodyID );
            builder.AppendU32( child.m_parentBodyID );
            builder.AppendU32( child.m_provenanceID );
            builder.AppendU32( child.m_geologicalAncestryID );
            builder.AppendU32( child.m_massGrams );
            builder.AppendFloat( child.m_massKg );
            builder.AppendFloat( child.m_requiredSolidVolumeM3 );
            builder.AppendFloat( child.m_measuredMeshVolumeM3 );
            builder.AppendFloat( child.m_parentVolumeFraction );
            FingerprintGraniteClosedGeometry( builder, child.m_geometry );
            FingerprintGraniteFractureChildSurfaceReceipt(
                builder,
                child.m_surfaceReceipt );
        }

        static void FingerprintGraniteGeometricPartitionReceipt(
            GraniteCanonicalFingerprintBuilder&                builder,
            GraniteFractureGeometricPartitionReceipt const& receipt )
        {
            builder.AppendFloat( receipt.m_parentVisibleMeshVolumeM3 );
            builder.AppendFloat( receipt.m_primaryVisibleMeshVolumeM3 );
            builder.AppendFloat( receipt.m_secondaryVisibleMeshVolumeM3 );
            builder.AppendFloat( receipt.m_childVisibleMeshVolumeSumM3 );
            builder.AppendFloat( receipt.m_visibleMeshUnionResidualM3 );
            builder.AppendFloat( receipt.m_visibleMeshUnionRelativeResidual );
            builder.AppendFloat( receipt.m_parentExteriorAreaM2 );
            builder.AppendFloat(
                receipt.m_childInheritedExteriorAreaSumM2 );
            builder.AppendFloat( receipt.m_exteriorAreaResidualM2 );
            builder.AppendFloat( receipt.m_exteriorAreaRelativeResidual );
            builder.AppendFloat( receipt.m_primaryCutAreaM2 );
            builder.AppendFloat( receipt.m_secondaryCutAreaM2 );
            builder.AppendFloat( receipt.m_cutAreaResidualM2 );
            builder.AppendFloat( receipt.m_cutAreaRelativeResidual );
            builder.AppendFloat( receipt.m_maxCutPlaneDeviationM );
            builder.AppendFloat( receipt.m_primaryHalfSpaceViolationM );
            builder.AppendFloat( receipt.m_secondaryHalfSpaceViolationM );
            builder.AppendI32( receipt.m_cutLoopCount );
            builder.AppendI32( receipt.m_cutVertexCount );
            builder.AppendI32( receipt.m_primaryInheritedTriangleCount );
            builder.AppendI32( receipt.m_secondaryInheritedTriangleCount );
            builder.AppendI32( receipt.m_primaryCapTriangleCount );
            builder.AppendI32( receipt.m_secondaryCapTriangleCount );
            builder.AppendBool( receipt.m_visibleMeshUnionPass );
            builder.AppendBool( receipt.m_exteriorPartitionPass );
            builder.AppendBool( receipt.m_sharedCutCoincidencePass );
            builder.AppendBool( receipt.m_halfSpaceOwnershipPass );
            builder.AppendBool( receipt.m_closedChildTopologyPass );
            builder.AppendBool( receipt.m_pass );
        }

        static void FingerprintGraniteFractureTransaction(
            GraniteCanonicalFingerprintBuilder&     builder,
            GraniteFractureTransactionResult const& transaction )
        {
            builder.AppendBool( transaction.m_valid );
            builder.AppendU32( uint32_t( transaction.m_rejectionReason ) );
            builder.AppendI32( transaction.m_parentTopologyFamily );
            builder.AppendU32( transaction.m_eventID );
            builder.AppendU32( transaction.m_parentBodyID );
            builder.AppendU32( transaction.m_parentProvenanceID );
            builder.AppendU32( transaction.m_geologicalAncestryID );
            FingerprintGraniteFracturePlane( builder, transaction.m_plane );
            builder.AppendU32( transaction.m_parentMassGrams );
            builder.AppendFloat( transaction.m_parentMassKg );
            builder.AppendFloat( transaction.m_parentSolidVolumeM3 );
            builder.AppendFloat( transaction.m_parentMeasuredMeshVolumeM3 );
            builder.AppendI32( transaction.m_numChildren );
            for ( int32_t childIndex = 0;
                  childIndex < transaction.m_numChildren;
                  ++childIndex )
            {
                FingerprintGraniteFractureChild(
                    builder,
                    transaction.m_children[childIndex] );
            }
            builder.AppendI32( transaction.m_numPrincipalChildren );
            builder.AppendI32( transaction.m_numChipChildren );
            builder.AppendU64( transaction.m_totalChildMassGrams );
            builder.AppendFloat( transaction.m_totalChildSolidVolumeM3 );
            builder.AppendFloat(
                transaction.m_totalChildMeasuredMeshVolumeM3 );
            builder.AppendI64( transaction.m_massResidualGrams );
            builder.AppendFloat( transaction.m_solidVolumeResidualM3 );
            builder.AppendFloat( transaction.m_meshVolumeResidualM3 );
            builder.AppendFloat( transaction.m_massRelativeResidual );
            builder.AppendFloat( transaction.m_solidVolumeRelativeResidual );
            FingerprintGraniteGeometricPartitionReceipt(
                builder,
                transaction.m_geometricPartition );
            builder.AppendBool( transaction.m_massConservationPass );
            builder.AppendBool( transaction.m_solidVolumeConservationPass );
            builder.AppendBool( transaction.m_childGeometryPass );
            builder.AppendBool( transaction.m_surfaceHistoryPass );
            builder.AppendBool( transaction.m_inheritedExteriorPreserved );
            builder.AppendBool(
                transaction.m_bothPrincipalChildrenHaveFreshBreak );
            builder.AppendBool( transaction.m_geometricPartitionPass );
        }

        static void FingerprintGraniteFormationForm(
            GraniteCanonicalFingerprintBuilder&   builder,
            GraniteFormationFormDescriptor const& form )
        {
            builder.AppendBool( form.m_valid );
            builder.AppendU32( form.m_eventID );
            builder.AppendU32( form.m_geologicalAncestryID );
            builder.AppendU32( form.m_grammarSalt );
            builder.AppendU32( uint32_t( form.m_type ) );
            builder.AppendU32( uint32_t( form.m_attachment ) );
            builder.AppendFloat( form.m_centerWorldX );
            builder.AppendFloat( form.m_centerWorldY );
            builder.AppendFloat( form.m_orientationRadians );
            builder.AppendFloat( form.m_majorRadiusM );
            builder.AppendFloat( form.m_minorRadiusM );
            builder.AppendFloat( form.m_maximumReliefM );
            builder.AppendFloat( form.m_rootBlendRadiusM );
            builder.AppendFloat( form.m_quietCoreRadiusM );
            builder.AppendFloat( form.m_dipRadians );
            builder.AppendFloat( form.m_shoulderSharpness );
            builder.AppendFloat( form.m_weatheredExteriorWeight );
            builder.AppendFloat( form.m_fractureShoulderWeight );
            builder.AppendFloat( form.m_asymmetry );
            builder.AppendI32( form.m_primaryJointSetIndex );
        }

        static void FingerprintGraniteSurfaceStone(
            GraniteCanonicalFingerprintBuilder&  builder,
            GraniteSurfaceStoneDescriptor const& stone )
        {
            builder.AppendBool( stone.m_valid );
            builder.AppendU32( stone.m_eventID );
            builder.AppendU32( stone.m_geologicalAncestryID );
            builder.AppendU32( stone.m_bodyGrammarSalt );
            builder.AppendU32( uint32_t( stone.m_state ) );
            builder.AppendFloat( stone.m_centerWorldX );
            builder.AppendFloat( stone.m_centerWorldY );
            builder.AppendFloat( stone.m_orientationRadians );
            builder.AppendFloat( stone.m_majorRadiusM );
            builder.AppendFloat( stone.m_minorRadiusM );
            builder.AppendFloat( stone.m_maximumReliefM );
            builder.AppendFloat( stone.m_embeddingDepthM );
            builder.AppendFloat( stone.m_weatheredExteriorWeight );
            builder.AppendFloat( stone.m_jointIsolation );
            builder.AppendFloat( stone.m_remainingConnection );
            builder.AppendFloat( stone.m_downslopeBias );
        }

        static void FingerprintGraniteStructuralBridge(
            GraniteCanonicalFingerprintBuilder&      builder,
            GraniteStructuralBridgeDescriptor const& bridge )
        {
            builder.AppendBool( bridge.m_valid );
            builder.AppendU32( bridge.m_bridgeID );
            builder.AppendU32( bridge.m_parentFormEventID );
            builder.AppendU32( bridge.m_geologicalAncestryID );
            builder.AppendU32( uint32_t( bridge.m_axis ) );
            builder.AppendFloat( bridge.m_centerWorldX );
            builder.AppendFloat( bridge.m_centerWorldY );
            builder.AppendFloat( bridge.m_orientationRadians );
            builder.AppendFloat( bridge.m_spanM );
            builder.AppendFloat( bridge.m_rootDepthM );
            builder.AppendFloat( bridge.m_attachmentAreaM2 );
            builder.AppendI32( bridge.m_dominantJointSetIndex );
            builder.AppendBool( bridge.m_severed );
            builder.AppendU32( bridge.m_severingEventID );
        }

        static void FingerprintGraniteStructuralBridgeSet(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteStructuralBridgeSet const&    bridgeSet )
        {
            builder.AppendI32( bridgeSet.m_numBridges );
            for ( int32_t bridgeIndex = 0;
                  bridgeIndex < bridgeSet.m_numBridges;
                  ++bridgeIndex )
            {
                FingerprintGraniteStructuralBridge(
                    builder,
                    bridgeSet.m_bridges[bridgeIndex] );
            }
            builder.AppendFloat( bridgeSet.m_initialAttachmentAreaM2 );
            builder.AppendFloat( bridgeSet.m_remainingAttachmentAreaM2 );
            builder.AppendFloat( bridgeSet.m_remainingConnectionFraction );
            builder.AppendFloat( bridgeSet.m_severedConnectionFraction );
            builder.AppendI32( bridgeSet.m_numIntactBridges );
            builder.AppendI32( bridgeSet.m_numSeveredBridges );
        }

        static void FingerprintGraniteDetachmentResult(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteDetachmentResult const&       result )
        {
            builder.AppendBool( result.m_valid );
            builder.AppendU32( result.m_eventID );
            builder.AppendU32( result.m_parentFormEventID );
            builder.AppendU32( result.m_geologicalAncestryID );
            builder.AppendU32( uint32_t( result.m_stateBefore ) );
            builder.AppendU32( uint32_t( result.m_stateAfter ) );
            builder.AppendU32( uint32_t( result.m_transition ) );
            FingerprintGraniteStructuralBridgeSet( builder, result.m_before );
            FingerprintGraniteStructuralBridgeSet( builder, result.m_after );
            builder.AppendFloat( result.m_attachmentAreaRemovedM2 );
            builder.AppendByte( result.m_requestedSeverMask );
            builder.AppendByte( result.m_effectiveSeverMask );
            builder.AppendBool(
                result.m_parentStructuralOwnershipRetained );
            builder.AppendBool( result.m_detachedCandidateReady );
            FingerprintGraniteSurfaceStone(
                builder,
                result.m_detachedCandidate );
            builder.AppendBool( result.m_monotonicConnectionPass );
            builder.AppendBool( result.m_stateTransitionPass );
            builder.AppendBool( result.m_bridgeAccountingPass );
            builder.AppendBool( result.m_parentOwnershipPass );
            builder.AppendBool( result.m_pass );
        }

        static void FingerprintGraniteDetachmentSequence(
            GraniteCanonicalFingerprintBuilder&       builder,
            GraniteDetachmentSequenceReceipt const& sequence )
        {
            builder.AppendBool( sequence.m_valid );
            FingerprintGraniteStructuralBridgeSet(
                builder,
                sequence.m_initialRooted );
            FingerprintGraniteDetachmentResult(
                builder,
                sequence.m_partialEvent );
            FingerprintGraniteDetachmentResult(
                builder,
                sequence.m_finalEvent );
            builder.AppendBool( sequence.m_initialRootedPass );
            builder.AppendBool( sequence.m_partialStillOwnedByParentPass );
            builder.AppendBool(
                sequence.m_finalParentOwnershipReleasedPass );
            builder.AppendBool( sequence.m_monotonicConnectionPass );
            builder.AppendBool( sequence.m_pass );
        }

        static void FingerprintGraniteRootedSeparationPolicy(
            GraniteCanonicalFingerprintBuilder&  builder,
            GraniteRootedSeparationPolicy const& policy )
        {
            builder.AppendFloat( policy.m_targetSampleSpacingM );
            builder.AppendFloat( policy.m_minSocketReliefFraction );
            builder.AppendFloat( policy.m_maxSocketReliefFraction );
            builder.AppendFloat( policy.m_rootBlendRetention );
            builder.AppendFloat( policy.m_bridgeRetentionRadiusScale );
            builder.AppendFloat( policy.m_bridgeRetentionStrength );
            builder.AppendFloat( policy.m_volumeRelativeTolerance );
            builder.AppendFloat( policy.m_massRelativeTolerance );
            builder.AppendFloat( policy.m_surfaceCoincidenceToleranceM );
        }

        static void FingerprintGraniteRootedPartitionMesh(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteRootedPartitionMesh const&    mesh )
        {
            builder.AppendI32( mesh.m_numVertices );
            for ( int32_t vertexIndex = 0;
                  vertexIndex < mesh.m_numVertices;
                  ++vertexIndex )
            {
                GraniteRootedPartitionVertex const& vertex =
                    mesh.m_vertices[vertexIndex];
                builder.AppendFloat( vertex.m_x );
                builder.AppendFloat( vertex.m_y );
                builder.AppendFloat( vertex.m_z );
            }

            builder.AppendI32( mesh.m_numTriangles );
            int32_t const numIndices = mesh.m_numTriangles * 3;
            for ( int32_t index = 0; index < numIndices; ++index )
            {
                builder.AppendU16( mesh.m_triangleIndices[index] );
            }
            builder.AppendFloat( mesh.m_measuredVolumeM3 );
            builder.AppendFloat( mesh.m_extentXM );
            builder.AppendFloat( mesh.m_extentYM );
            builder.AppendFloat( mesh.m_extentZM );
            builder.AppendBool( mesh.m_closed );
        }

        static void FingerprintGraniteRootedMatter(
            GraniteCanonicalFingerprintBuilder&        builder,
            GraniteRootedMatterPartitionReceipt const& matter )
        {
            builder.AppendFloat( matter.m_densityKgPerM3 );
            builder.AppendFloat( matter.m_preDetachParcelVolumeM3 );
            builder.AppendFloat( matter.m_detachedVolumeM3 );
            builder.AppendFloat( matter.m_spallReserveVolumeM3 );
            builder.AppendFloat( matter.m_remainingParentVolumeM3 );
            builder.AppendFloat( matter.m_volumeResidualM3 );
            builder.AppendFloat( matter.m_volumeRelativeResidual );
            builder.AppendU32( matter.m_preDetachParcelMassGrams );
            builder.AppendU32( matter.m_detachedMassGrams );
            builder.AppendU32( matter.m_spallReserveMassGrams );
            builder.AppendU32( matter.m_remainingParentMassGrams );
            builder.AppendI64( matter.m_massResidualGrams );
            builder.AppendFloat( matter.m_massRelativeResidual );
            builder.AppendBool( matter.m_volumeConservationPass );
            builder.AppendBool( matter.m_massConservationPass );
        }

        static void FingerprintGraniteRootedSeparation(
            GraniteCanonicalFingerprintBuilder&   builder,
            GraniteRootedSeparationResult const& separation )
        {
            builder.AppendBool( separation.m_valid );
            builder.AppendU32( separation.m_partitionEventID );
            builder.AppendU32( separation.m_parentFormEventID );
            builder.AppendU32( separation.m_geologicalAncestryID );
            FingerprintGraniteFormationForm( builder, separation.m_rootedForm );
            FingerprintGraniteSurfaceStone(
                builder,
                separation.m_sourceDetachedCandidate );
            FingerprintGraniteRootedSeparationPolicy(
                builder,
                separation.m_policy );
            builder.AppendFloat( separation.m_minWorldX );
            builder.AppendFloat( separation.m_minWorldY );
            builder.AppendFloat( separation.m_maxWorldX );
            builder.AppendFloat( separation.m_maxWorldY );
            builder.AppendFloat( separation.m_sampleSpacingM );
            builder.AppendI32( separation.m_samplesX );
            builder.AppendI32( separation.m_samplesY );
            FingerprintGraniteRootedPartitionMesh(
                builder,
                separation.m_detachedHeadMesh );
            FingerprintGraniteRootedPartitionMesh(
                builder,
                separation.m_remainingParentSocketMesh );
            FingerprintGraniteRootedMatter( builder, separation.m_matter );
            builder.AppendFloat( separation.m_sharedInterfaceAreaM2 );
            builder.AppendFloat(
                separation.m_maxSharedInterfaceDeviationM );
            builder.AppendU32( separation.m_retainedApronMassGrams );
            builder.AppendFloat( separation.m_retainedApronMassFraction );
            builder.AppendFloat( separation.m_undersideFinVolumeM3 );
            builder.AppendFloat( separation.m_undersideFinVolumeFraction );
            builder.AppendU32( separation.m_detachedBodyID );
            builder.AppendU32( separation.m_detachedParentBodyID );
            builder.AppendU32( separation.m_detachedProvenanceID );
            builder.AppendU32( separation.m_detachedMassGrams );
            builder.AppendBool( separation.m_structuralReleasePass );
            builder.AppendBool( separation.m_geometricPartitionPass );
            builder.AppendBool( separation.m_sharedInterfacePass );
            builder.AppendBool( separation.m_detachedMeshClosedPass );
            builder.AppendBool( separation.m_parentSocketMeshClosedPass );
            builder.AppendBool( separation.m_principalCastFitPass );
            builder.AppendBool( separation.m_approvedShapePass );
            builder.AppendBool( separation.m_spallReservePass );
            builder.AppendBool( separation.m_matterPartitionPass );
            builder.AppendBool( separation.m_pass );
        }

        static void FingerprintGraniteSpallPolicy(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteSpallResolutionPolicy const& policy )
        {
            builder.AppendI32( policy.m_maxExplicitPieces );
            builder.AppendU32( policy.m_minExplicitPieceMassGrams );
            builder.AppendU32( policy.m_minChipMassGrams );
            builder.AppendU32( policy.m_minCoarseSpallMassGrams );
            builder.AppendFloat( policy.m_minFineReserveFraction );
            builder.AppendFloat( policy.m_maxFineReserveFraction );
            builder.AppendI64( policy.m_massResidualToleranceGrams );
        }

        static void FingerprintGraniteSpallRequest(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteSpallResolutionRequest const& request )
        {
            builder.AppendU32( request.m_worldSeed );
            builder.AppendU32( request.m_sourcePartitionEventID );
            builder.AppendU32( request.m_parentDetachedBodyID );
            builder.AppendU32( request.m_parentProvenanceID );
            builder.AppendU32( request.m_geologicalAncestryID );
            builder.AppendU32( request.m_spallReserveMassGrams );
            builder.AppendFloat( request.m_originWorldX );
            builder.AppendFloat( request.m_originWorldY );
            builder.AppendFloat( request.m_originWorldZ );
            FingerprintGraniteSpallPolicy( builder, request.m_policy );
        }

        static void FingerprintGraniteSpallPiece(
            GraniteCanonicalFingerprintBuilder& builder,
            GraniteSpallPieceDescriptor const&   piece )
        {
            builder.AppendBool( piece.m_valid );
            builder.AppendU32( piece.m_bodyID );
            builder.AppendU32( piece.m_parentBodyID );
            builder.AppendU32( piece.m_provenanceID );
            builder.AppendU32( piece.m_geologicalAncestryID );
            builder.AppendU32( uint32_t( piece.m_class ) );
            builder.AppendU32( piece.m_massGrams );
            builder.AppendFloat( piece.m_localOffsetX );
            builder.AppendFloat( piece.m_localOffsetY );
            builder.AppendFloat( piece.m_localOffsetZ );
            builder.AppendFloat( piece.m_biasX );
            builder.AppendFloat( piece.m_biasY );
            builder.AppendFloat( piece.m_biasZ );
        }

        static void FingerprintGraniteSpallReceipt(
            GraniteCanonicalFingerprintBuilder&  builder,
            GraniteSpallResolutionReceipt const& receipt )
        {
            builder.AppendU32( receipt.m_inputReserveMassGrams );
            builder.AppendU32( receipt.m_explicitPieceMassGrams );
            builder.AppendU32( receipt.m_fineReserveMassGrams );
            builder.AppendI64( receipt.m_massResidualGrams );
            builder.AppendI32( receipt.m_numExplicitPieces );
            builder.AppendI32( receipt.m_numCoarseSpalls );
            builder.AppendI32( receipt.m_numChips );
            builder.AppendI32( receipt.m_numGritPieces );
            builder.AppendBool( receipt.m_uniqueBodyIDsPass );
            builder.AppendBool( receipt.m_lineagePass );
            builder.AppendBool( receipt.m_pieceMinimumMassPass );
            builder.AppendBool( receipt.m_massConservationPass );
            builder.AppendBool( receipt.m_pass );
        }

        static void FingerprintGraniteSpallResolution(
            GraniteCanonicalFingerprintBuilder&  builder,
            GraniteSpallResolutionResult const& resolution )
        {
            FingerprintGraniteSpallRequest( builder, resolution.m_request );
            builder.AppendI32( resolution.m_numPieces );
            for ( int32_t pieceIndex = 0;
                  pieceIndex < resolution.m_numPieces;
                  ++pieceIndex )
            {
                FingerprintGraniteSpallPiece(
                    builder,
                    resolution.m_pieces[pieceIndex] );
            }
            FingerprintGraniteSpallReceipt( builder, resolution.m_receipt );
            builder.AppendBool( resolution.m_valid );
        }

        static void FingerprintGraniteTerminalReceipt(
            GraniteCanonicalFingerprintBuilder&          builder,
            GraniteTerminalConservationReceipt const& terminal )
        {
            builder.AppendU32( terminal.m_preDetachParcelMassGrams );
            builder.AppendU32( terminal.m_remainingParentMassGrams );
            builder.AppendU32( terminal.m_detachedMassGrams );
            builder.AppendU32( terminal.m_partitionReserveMassGrams );
            builder.AppendU32(
                terminal.m_resolutionRequestReserveMassGrams );
            builder.AppendU32(
                terminal.m_resolutionReceiptInputMassGrams );
            builder.AppendU64( terminal.m_actualExplicitPieceMassGrams );
            builder.AppendU32( terminal.m_reportedExplicitPieceMassGrams );
            builder.AppendU32( terminal.m_fineReserveMassGrams );
            builder.AppendI32( terminal.m_actualExplicitPieceCount );
            builder.AppendI32( terminal.m_reportedExplicitPieceCount );
            builder.AppendI64( terminal.m_partitionResidualGrams );
            builder.AppendI64( terminal.m_requestHandoffResidualGrams );
            builder.AppendI64( terminal.m_receiptInputResidualGrams );
            builder.AppendI64(
                terminal.m_explicitPieceReceiptResidualGrams );
            builder.AppendI64( terminal.m_resolutionResidualGrams );
            builder.AppendI64( terminal.m_terminalResidualGrams );
            builder.AppendBool( terminal.m_partitionPass );
            builder.AppendBool( terminal.m_requestHandoffPass );
            builder.AppendBool( terminal.m_receiptInputPass );
            builder.AppendBool( terminal.m_explicitPieceReceiptPass );
            builder.AppendBool( terminal.m_explicitPieceCountPass );
            builder.AppendBool( terminal.m_resolutionPass );
            builder.AppendBool( terminal.m_terminalPass );
            builder.AppendBool( terminal.m_pass );
        }

        static GraniteAuthorityCaseFingerprint BuildGraniteCaseFingerprint(
            uint32_t                                  worldSeed,
            uint32_t                                  topologyFamily,
            GraniteFractureCorpusArtifacts const&     fracture,
            GraniteDetachmentSequenceReceipt const& sequence,
            GraniteRootedSeparationResult const&     separation,
            GraniteSpallResolutionResult const&      spallResolution,
            GraniteTerminalConservationReceipt const& terminal )
        {
            GraniteCanonicalFingerprintBuilder builder;
            builder.AppendU32( s_graniteFingerprintSchemaVersion );
            builder.AppendU32( s_graniteGeometryAlgorithmVersion );
            builder.AppendU32( worldSeed );
            builder.AppendU32( topologyFamily );
            FingerprintGraniteClosedGeometryRequest(
                builder,
                fracture.m_parentRequest );
            FingerprintGraniteClosedGeometry(
                builder,
                fracture.m_parentGeometry );
            FingerprintGraniteFractureRequest(
                builder,
                fracture.m_fractureRequest );
            FingerprintGraniteFractureTransaction(
                builder,
                fracture.m_transaction );
            FingerprintGraniteDetachmentSequence( builder, sequence );
            FingerprintGraniteRootedSeparation( builder, separation );
            FingerprintGraniteSpallResolution( builder, spallResolution );
            FingerprintGraniteTerminalReceipt( builder, terminal );

            GraniteAuthorityCaseFingerprint fingerprint;
            fingerprint.m_worldSeed = worldSeed;
            fingerprint.m_topologyFamily = topologyFamily;
            fingerprint.m_low = builder.m_low;
            fingerprint.m_high = builder.m_high;
            return fingerprint;
        }

        static bool GraniteFingerprintsEqual(
            GraniteAuthorityCaseFingerprint const& lhs,
            GraniteAuthorityCaseFingerprint const& rhs )
        {
            return
                lhs.m_worldSeed == rhs.m_worldSeed &&
                lhs.m_topologyFamily == rhs.m_topologyFamily &&
                lhs.m_low == rhs.m_low &&
                lhs.m_high == rhs.m_high;
        }

        //-------------------------------------------------------------------------

        static void AppendFailure(
            GraniteAuthorityVerificationReport& report,
            uint32_t                             caseID,
            char const*                          receiptName,
            char const*                          gateName )
        {
            if ( report.m_numFailures >=
                 GraniteAuthorityVerificationReport::s_maxFailures )
            {
                report.m_failureCapacityExceeded = true;
                return;
            }

            GraniteAuthorityFailure& failure =
                report.m_failures[report.m_numFailures++];

            failure.m_caseID = caseID;
            failure.m_receiptName = receiptName;
            failure.m_gateName = gateName;
        }

        //-------------------------------------------------------------------------

        static void CheckGate(
            GraniteAuthorityVerificationReport& report,
            bool                                 gatePass,
            uint32_t                             caseID,
            char const*                          receiptName,
            char const*                          gateName )
        {
            ++report.m_numGateChecks;

            if ( !gatePass )
            {
                AppendFailure(
                    report,
                    caseID,
                    receiptName,
                    gateName );
            }
        }

        #include "GraniteExcavationCertificate.inl"

        static void CheckDeterminismGate(
            GraniteAuthorityVerificationReport& report,
            bool                                 gatePass,
            uint32_t                             caseID,
            char const*                          gateName )
        {
            ++report.m_numDeterminismChecks;
            CheckGate(
                report,
                gatePass,
                caseID,
                "GraniteAuthorityDeterminism",
                gateName );
        }

        static void RequireFingerprintNegativeControl(
            GraniteAuthorityVerificationReport&       report,
            GraniteAuthorityCaseFingerprint const& positive,
            uint32_t                                   caseID )
        {
            GraniteAuthorityCaseFingerprint mutated = positive;
            mutated.m_low ^= 1ull;
            ++report.m_numNegativeControls;

            if ( GraniteFingerprintsEqual( positive, mutated ) )
            {
                AppendFailure(
                    report,
                    caseID,
                    "GraniteAuthorityNegativeControl",
                    "FingerprintLowBitMutation" );
            }
        }

        static float MutateGraniteFloatOneBit( float value )
        {
            uint32_t bits = 0;
            std::memcpy( &bits, &value, sizeof( bits ) );
            bits ^= 1u;
            std::memcpy( &value, &bits, sizeof( value ) );
            return value;
        }

        static void RequireFingerprintFieldMutation(
            GraniteAuthorityVerificationReport&       report,
            GraniteAuthorityCaseFingerprint const& positive,
            GraniteAuthorityCaseFingerprint const& mutated,
            uint32_t                                   caseID,
            char const*                                gateName )
        {
            ++report.m_numNegativeControls;

            if ( GraniteFingerprintsEqual( positive, mutated ) )
            {
                AppendFailure(
                    report,
                    caseID,
                    "GraniteAuthorityNegativeControl",
                    gateName );
            }
        }

        static void RequireGraniteFloatReachabilityControls(
            GraniteAuthorityVerificationReport&       report,
            GraniteAuthorityCaseFingerprint const& positive,
            uint32_t                                   worldSeed,
            uint32_t                                   topologyFamily,
            GraniteFractureCorpusArtifacts const&     fracture,
            GraniteDetachmentSequenceReceipt const& sequence,
            GraniteRootedSeparationResult const&     separation,
            GraniteSpallResolutionResult const&      spallResolution,
            GraniteTerminalConservationReceipt const& terminal )
        {
            {
                GraniteFractureCorpusArtifacts mutated = fracture;
                if ( mutated.m_parentGeometry.m_numVertices <= 0 )
                {
                    AppendFailure(
                        report,
                        worldSeed,
                        "GraniteAuthorityNegativeControl",
                        "VertexFloatControlUnavailable" );
                }
                else
                {
                    mutated.m_parentGeometry.m_vertices[0].m_x =
                        MutateGraniteFloatOneBit(
                            mutated.m_parentGeometry.m_vertices[0].m_x );
                    RequireFingerprintFieldMutation(
                        report,
                        positive,
                        BuildGraniteCaseFingerprint(
                            worldSeed,
                            topologyFamily,
                            mutated,
                            sequence,
                            separation,
                            spallResolution,
                            terminal ),
                        worldSeed,
                        "VertexFloatReachability" );
                }
            }

            {
                GraniteFractureCorpusArtifacts mutated = fracture;
                mutated.m_transaction.m_geometricPartition.
                    m_primaryCutAreaM2 =
                    MutateGraniteFloatOneBit(
                        mutated.m_transaction.m_geometricPartition.
                            m_primaryCutAreaM2 );
                RequireFingerprintFieldMutation(
                    report,
                    positive,
                    BuildGraniteCaseFingerprint(
                        worldSeed,
                        topologyFamily,
                        mutated,
                        sequence,
                        separation,
                        spallResolution,
                        terminal ),
                    worldSeed,
                    "AreaFloatReachability" );
            }

            {
                GraniteRootedSeparationResult mutated = separation;
                mutated.m_matter.m_detachedVolumeM3 =
                    MutateGraniteFloatOneBit(
                        mutated.m_matter.m_detachedVolumeM3 );
                RequireFingerprintFieldMutation(
                    report,
                    positive,
                    BuildGraniteCaseFingerprint(
                        worldSeed,
                        topologyFamily,
                        fracture,
                        sequence,
                        mutated,
                        spallResolution,
                        terminal ),
                    worldSeed,
                    "VolumeFloatReachability" );
            }

            {
                GraniteRootedSeparationResult mutated = separation;
                mutated.m_maxSharedInterfaceDeviationM =
                    MutateGraniteFloatOneBit(
                        mutated.m_maxSharedInterfaceDeviationM );
                RequireFingerprintFieldMutation(
                    report,
                    positive,
                    BuildGraniteCaseFingerprint(
                        worldSeed,
                        topologyFamily,
                        fracture,
                        sequence,
                        mutated,
                        spallResolution,
                        terminal ),
                    worldSeed,
                    "InterfaceDeviationFloatReachability" );
            }

            {
                GraniteRootedSeparationResult mutated = separation;
                mutated.m_retainedApronMassFraction =
                    MutateGraniteFloatOneBit(
                        mutated.m_retainedApronMassFraction );
                RequireFingerprintFieldMutation(
                    report,
                    positive,
                    BuildGraniteCaseFingerprint(
                        worldSeed,
                        topologyFamily,
                        fracture,
                        sequence,
                        mutated,
                        spallResolution,
                        terminal ),
                    worldSeed,
                    "ApronRatioFloatReachability" );
            }

            {
                GraniteRootedSeparationResult mutated = separation;
                mutated.m_undersideFinVolumeFraction =
                    MutateGraniteFloatOneBit(
                        mutated.m_undersideFinVolumeFraction );
                RequireFingerprintFieldMutation(
                    report,
                    positive,
                    BuildGraniteCaseFingerprint(
                        worldSeed,
                        topologyFamily,
                        fracture,
                        sequence,
                        mutated,
                        spallResolution,
                        terminal ),
                    worldSeed,
                    "FinRatioFloatReachability" );
            }
        }

        //-------------------------------------------------------------------------

        static bool ContainsFailure(
            GraniteAuthorityVerificationReport const& report,
            char const*                                receiptName,
            char const*                                gateName )
        {
            for ( int32_t failureIndex = 0;
                  failureIndex < report.m_numFailures;
                  ++failureIndex )
            {
                GraniteAuthorityFailure const& failure =
                    report.m_failures[failureIndex];

                if ( failure.m_receiptName != nullptr &&
                     failure.m_gateName != nullptr &&
                     std::strcmp( failure.m_receiptName, receiptName ) == 0 &&
                     std::strcmp( failure.m_gateName, gateName ) == 0 )
                {
                    return true;
                }
            }

            return false;
        }

        //-------------------------------------------------------------------------

        static void RequireNegativeControl(
            GraniteAuthorityVerificationReport&       aggregateReport,
            GraniteAuthorityVerificationReport const& negativeReport,
            uint32_t                                   caseID,
            char const*                                receiptName,
            char const*                                gateName )
        {
            ++aggregateReport.m_numNegativeControls;

            if ( !ContainsFailure(
                     negativeReport,
                     receiptName,
                     gateName ) )
            {
                AppendFailure(
                    aggregateReport,
                    caseID,
                    "GraniteAuthorityNegativeControl",
                    gateName );
            }
        }

        //-------------------------------------------------------------------------

        static void RecordRootedShapeEnvelope(
            GraniteAuthorityVerificationReport&   report,
            GraniteRootedSeparationResult const& separation )
        {
            if ( !report.m_shapeEnvelopeInitialized )
            {
                report.m_maxPrincipalCastFitDeviationM =
                    separation.m_maxSharedInterfaceDeviationM;
                report.m_minRetainedApronMassGrams =
                    separation.m_retainedApronMassGrams;
                report.m_maxRetainedApronMassGrams =
                    separation.m_retainedApronMassGrams;
                report.m_minRetainedApronMassFraction =
                    separation.m_retainedApronMassFraction;
                report.m_maxRetainedApronMassFraction =
                    separation.m_retainedApronMassFraction;
                report.m_minUndersideFinVolumeFraction =
                    separation.m_undersideFinVolumeFraction;
                report.m_maxUndersideFinVolumeFraction =
                    separation.m_undersideFinVolumeFraction;
                report.m_shapeEnvelopeInitialized = true;
                return;
            }

            report.m_maxPrincipalCastFitDeviationM =
                separation.m_maxSharedInterfaceDeviationM >
                        report.m_maxPrincipalCastFitDeviationM
                    ? separation.m_maxSharedInterfaceDeviationM
                    : report.m_maxPrincipalCastFitDeviationM;

            report.m_minRetainedApronMassGrams =
                separation.m_retainedApronMassGrams <
                        report.m_minRetainedApronMassGrams
                    ? separation.m_retainedApronMassGrams
                    : report.m_minRetainedApronMassGrams;

            report.m_maxRetainedApronMassGrams =
                separation.m_retainedApronMassGrams >
                        report.m_maxRetainedApronMassGrams
                    ? separation.m_retainedApronMassGrams
                    : report.m_maxRetainedApronMassGrams;

            report.m_minRetainedApronMassFraction =
                separation.m_retainedApronMassFraction <
                        report.m_minRetainedApronMassFraction
                    ? separation.m_retainedApronMassFraction
                    : report.m_minRetainedApronMassFraction;

            report.m_maxRetainedApronMassFraction =
                separation.m_retainedApronMassFraction >
                        report.m_maxRetainedApronMassFraction
                    ? separation.m_retainedApronMassFraction
                    : report.m_maxRetainedApronMassFraction;

            report.m_minUndersideFinVolumeFraction =
                separation.m_undersideFinVolumeFraction <
                        report.m_minUndersideFinVolumeFraction
                    ? separation.m_undersideFinVolumeFraction
                    : report.m_minUndersideFinVolumeFraction;

            report.m_maxUndersideFinVolumeFraction =
                separation.m_undersideFinVolumeFraction >
                        report.m_maxUndersideFinVolumeFraction
                    ? separation.m_undersideFinVolumeFraction
                    : report.m_maxUndersideFinVolumeFraction;
        }

        //-------------------------------------------------------------------------

        static GranitePoint CalculateCentroid(
            GraniteClosedGeometry const& geometry )
        {
            GranitePoint centroid;

            if ( geometry.m_numVertices <= 0 )
            {
                return centroid;
            }

            for ( int32_t vertexIndex = 0;
                  vertexIndex < geometry.m_numVertices;
                  ++vertexIndex )
            {
                centroid.m_x += geometry.m_vertices[vertexIndex].m_x;
                centroid.m_y += geometry.m_vertices[vertexIndex].m_y;
                centroid.m_z += geometry.m_vertices[vertexIndex].m_z;
            }

            float const inverseVertexCount =
                1.0f / float( geometry.m_numVertices );

            centroid.m_x *= inverseVertexCount;
            centroid.m_y *= inverseVertexCount;
            centroid.m_z *= inverseVertexCount;

            return centroid;
        }

        //-------------------------------------------------------------------------

        static bool BuildFractureCorpusCase(
            uint32_t                         worldSeed,
            uint32_t                         topologyFamily,
            GraniteFractureCorpusArtifacts& outArtifacts )
        {
            GraniteClosedGeometryRequest& parentRequest =
                outArtifacts.m_parentRequest;

            parentRequest.m_worldSeed = worldSeed;
            parentRequest.m_geologicalAncestryID =
                s_graniteAuthorityAncestryID;
            parentRequest.m_bodyID = 1000u + topologyFamily;
            parentRequest.m_massGrams = 12000u + topologyFamily * 137u;
            parentRequest.m_bodyState = ProvenanceBodyState::Fragment;
            parentRequest.m_scale = GraniteGeometryScale::Block;
            parentRequest.m_weathering =
                GraniteWeatheringState::WeatheredExposure;
            parentRequest.m_canInheritExteriorSurface = true;
            parentRequest.m_parentExteriorExposure = 0.90f;
            parentRequest.m_worldX = float( topologyFamily ) * 1.75f;
            parentRequest.m_worldY = float( topologyFamily ) * -1.25f;
            parentRequest.m_worldZ = 0.0f;

            outArtifacts.m_parentGeometry =
                GenerateGraniteClosedGeometry( parentRequest );
            GraniteClosedGeometry const& parentGeometry =
                outArtifacts.m_parentGeometry;

            if ( parentGeometry.m_numVertices < 4 ||
                 parentGeometry.m_numFaces < 4 )
            {
                return false;
            }

            GranitePoint fractureNormal = { 0.67f, -0.22f, 0.71f };

            float const normalLength =
                float(
                    std::sqrt(
                        double(
                            fractureNormal.m_x * fractureNormal.m_x +
                            fractureNormal.m_y * fractureNormal.m_y +
                            fractureNormal.m_z * fractureNormal.m_z ) ) );

            if ( normalLength <= 0.000001f )
            {
                return false;
            }

            fractureNormal.m_x /= normalLength;
            fractureNormal.m_y /= normalLength;
            fractureNormal.m_z /= normalLength;

            GranitePoint planePoint = CalculateCentroid( parentGeometry );

            float const maximumExtentM =
                parentGeometry.m_extentXM > parentGeometry.m_extentYM
                    ? (
                          parentGeometry.m_extentXM > parentGeometry.m_extentZM
                              ? parentGeometry.m_extentXM
                              : parentGeometry.m_extentZM )
                    : (
                          parentGeometry.m_extentYM > parentGeometry.m_extentZM
                              ? parentGeometry.m_extentYM
                              : parentGeometry.m_extentZM );

            float const cutOffsetM = 0.035f * maximumExtentM;

            planePoint.m_x += fractureNormal.m_x * cutOffsetM;
            planePoint.m_y += fractureNormal.m_y * cutOffsetM;
            planePoint.m_z += fractureNormal.m_z * cutOffsetM;

            GraniteFractureRequest& fractureRequest =
                outArtifacts.m_fractureRequest;

            fractureRequest.m_worldSeed = worldSeed;
            fractureRequest.m_eventID = 0x10A00001u + topologyFamily;
            fractureRequest.m_parentBodyID = parentRequest.m_bodyID;
            fractureRequest.m_parentProvenanceID =
                2000u + topologyFamily;
            fractureRequest.m_geologicalAncestryID =
                parentRequest.m_geologicalAncestryID;
            fractureRequest.m_primaryChildBodyID = 3000u + topologyFamily * 2u;
            fractureRequest.m_secondaryChildBodyID =
                fractureRequest.m_primaryChildBodyID + 1u;
            fractureRequest.m_allowChips = false;
            fractureRequest.m_maxChipCount = 0;
            fractureRequest.m_plane.m_normal = fractureNormal;
            fractureRequest.m_plane.m_pointOnPlane.m_x =
                parentRequest.m_worldX + planePoint.m_x;
            fractureRequest.m_plane.m_pointOnPlane.m_y =
                parentRequest.m_worldY + planePoint.m_y;
            fractureRequest.m_plane.m_pointOnPlane.m_z =
                parentRequest.m_worldZ + planePoint.m_z;
            fractureRequest.m_plane.m_origin =
                GraniteFaceOrigin::FreshBreak;
            fractureRequest.m_plane.m_jointSetIndex = -1;
            fractureRequest.m_planeToleranceM = 0.00025f;

            outArtifacts.m_transaction =
                FractureGraniteClosedGeometry(
                    parentGeometry,
                    parentRequest,
                    fractureRequest );

            return outArtifacts.m_transaction.m_valid;
        }

        //-------------------------------------------------------------------------

        static char const* GetFractureRejectionGateName(
            GraniteFractureRejectionReason reason )
        {
            switch ( reason )
            {
                case GraniteFractureRejectionReason::InvalidParentInput:
                    return "InvalidParentInput";

                case GraniteFractureRejectionReason::ParentTriangleMeshNotClosed:
                    return "ParentTriangleMeshNotClosed";

                case GraniteFractureRejectionReason::ParentSeedTriangleMeshNotClosed:
                    return "ParentSeedTriangleMeshNotClosed";

                case GraniteFractureRejectionReason::ParentDebiasedTriangleMeshNotClosed:
                    return "ParentDebiasedTriangleMeshNotClosed";

                case GraniteFractureRejectionReason::ParentSurfaceHistoryTriangleMeshNotClosed:
                    return "ParentSurfaceHistoryTriangleMeshNotClosed";

                case GraniteFractureRejectionReason::ParentStructuredArticulationTriangleMeshNotClosed:
                    return "ParentStructuredArticulationTriangleMeshNotClosed";

                case GraniteFractureRejectionReason::InvalidPlaneNormal:
                    return "InvalidPlaneNormal";

                case GraniteFractureRejectionReason::PlaneDoesNotSplitParent:
                    return "PlaneDoesNotSplitParent";

                case GraniteFractureRejectionReason::ChildConstructionFailed:
                    return "ChildConstructionFailed";

                case GraniteFractureRejectionReason::CutTopologyInsufficient:
                    return "CutTopologyInsufficient";

                case GraniteFractureRejectionReason::CutTopologyCapacityExceeded:
                    return "CutTopologyCapacityExceeded";

                case GraniteFractureRejectionReason::CutTopologyOpen:
                    return "CutTopologyOpen";

                case GraniteFractureRejectionReason::CutTopologyBranch:
                    return "CutTopologyBranch";

                case GraniteFractureRejectionReason::CutTopologyTraversalFailed:
                    return "CutTopologyTraversalFailed";

                case GraniteFractureRejectionReason::CapLoopInvalid:
                    return "CapLoopInvalid";

                case GraniteFractureRejectionReason::PrimaryCapDegenerateTriangle:
                    return "PrimaryCapDegenerateTriangle";

                case GraniteFractureRejectionReason::SecondaryCapDegenerateTriangle:
                    return "SecondaryCapDegenerateTriangle";

                case GraniteFractureRejectionReason::BothCapsDegenerateTriangle:
                    return "BothCapsDegenerateTriangle";

                case GraniteFractureRejectionReason::PrimaryCapTriangleRejected:
                    return "PrimaryCapTriangleRejected";

                case GraniteFractureRejectionReason::SecondaryCapTriangleRejected:
                    return "SecondaryCapTriangleRejected";

                case GraniteFractureRejectionReason::PrimaryCapVertexCapacityExceeded:
                    return "PrimaryCapVertexCapacityExceeded";

                case GraniteFractureRejectionReason::SecondaryCapVertexCapacityExceeded:
                    return "SecondaryCapVertexCapacityExceeded";

                case GraniteFractureRejectionReason::PrimaryCapTriangleCapacityExceeded:
                    return "PrimaryCapTriangleCapacityExceeded";

                case GraniteFractureRejectionReason::SecondaryCapTriangleCapacityExceeded:
                    return "SecondaryCapTriangleCapacityExceeded";

                case GraniteFractureRejectionReason::InsufficientChildMesh:
                    return "InsufficientChildMesh";

                case GraniteFractureRejectionReason::NonPositiveChildVolume:
                    return "NonPositiveChildVolume";

                case GraniteFractureRejectionReason::CertificateGateFailed:
                    return "CertificateGateFailed";

                case GraniteFractureRejectionReason::None:
                default:
                    return "UnclassifiedFractureRejection";
            }
        }

        //-------------------------------------------------------------------------

        static char const* GetFractureCorpusReceiptName(
            int32_t topologyFamily )
        {
            switch ( topologyFamily )
            {
                case 0: return "GraniteAuthorityCorpus.TopologyFamily0";
                case 1: return "GraniteAuthorityCorpus.TopologyFamily1";
                case 2: return "GraniteAuthorityCorpus.TopologyFamily2";
                case 3: return "GraniteAuthorityCorpus.TopologyFamily3";
                default: return "GraniteAuthorityCorpus.UnknownTopologyFamily";
            }
        }

        //-------------------------------------------------------------------------

        static bool SelectNearestRootedForm(
            GraniteFormationField const&    field,
            GraniteFormationFormDescriptor& outRootedForm,
            float&                          inOutBestDistanceSquared )
        {
            bool found = false;

            for ( int32_t formIndex = 0;
                  formIndex < field.m_numForms;
                  ++formIndex )
            {
                GraniteFormationFormDescriptor const& form =
                    field.m_forms[formIndex];

                if ( !form.m_valid ||
                     form.m_type != GraniteFormationFormType::RootedRockHead ||
                     form.m_attachment !=
                         GraniteFormationAttachmentState::RootedRockHead )
                {
                    continue;
                }

                float const distanceSquared =
                    form.m_centerWorldX * form.m_centerWorldX +
                    form.m_centerWorldY * form.m_centerWorldY;

                if ( distanceSquared < inOutBestDistanceSquared )
                {
                    inOutBestDistanceSquared = distanceSquared;
                    outRootedForm = form;
                    found = true;
                }
            }

            return found;
        }

        //-------------------------------------------------------------------------

        static bool FindNearestRootedForm(
            uint32_t                        worldSeed,
            GraniteFormationFormDescriptor& outRootedForm )
        {
            float constexpr tileSizeM = 8.0f;
            float constexpr searchRadiiM[] = { 32.0f, 64.0f, 96.0f };

            float bestDistanceSquared =
                std::numeric_limits<float>::max();

            bool found = false;

            for ( float const searchRadiusM : searchRadiiM )
            {
                int32_t const minimumTile =
                    int32_t(
                        std::floor(
                            double( -searchRadiusM / tileSizeM ) ) );

                int32_t const maximumTile =
                    int32_t(
                        std::ceil(
                            double( searchRadiusM / tileSizeM ) ) ) - 1;

                for ( int32_t tileY = minimumTile;
                      tileY <= maximumTile;
                      ++tileY )
                {
                    for ( int32_t tileX = minimumTile;
                          tileX <= maximumTile;
                          ++tileX )
                    {
                        float const minimumWorldX =
                            float( tileX ) * tileSizeM;
                        float const minimumWorldY =
                            float( tileY ) * tileSizeM;
                        float const maximumWorldX = minimumWorldX + tileSizeM;
                        float const maximumWorldY = minimumWorldY + tileSizeM;

                        float const closestX =
                            minimumWorldX > 0.0f
                                ? minimumWorldX
                                : ( maximumWorldX < 0.0f ? maximumWorldX : 0.0f );
                        float const closestY =
                            minimumWorldY > 0.0f
                                ? minimumWorldY
                                : ( maximumWorldY < 0.0f ? maximumWorldY : 0.0f );

                        if ( closestX * closestX + closestY * closestY >
                             searchRadiusM * searchRadiusM )
                        {
                            continue;
                        }

                        GraniteFormationFieldRequest request;

                        request.m_worldSeed = worldSeed;
                        request.m_geologicalAncestryID =
                            s_graniteAuthorityAncestryID;
                        request.m_minWorldX = minimumWorldX;
                        request.m_minWorldY = minimumWorldY;
                        request.m_maxWorldX = maximumWorldX;
                        request.m_maxWorldY = maximumWorldY;

                        GraniteFormationField const field =
                            GenerateGraniteFormationField( request );

                        found =
                            SelectNearestRootedForm(
                                field,
                                outRootedForm,
                                bestDistanceSquared ) ||
                            found;
                    }
                }

                if ( found )
                {
                    return true;
                }
            }

            return false;
        }

        //-------------------------------------------------------------------------

        static bool BuildRootedCorpusCase(
            uint32_t                            worldSeed,
            GraniteDetachmentSequenceReceipt&  outSequence,
            GraniteRootedSeparationResult&      outSeparation,
            GraniteSpallResolutionResult&       outSpallResolution )
        {
            GraniteFormationFormDescriptor rootedForm;

            if ( !FindNearestRootedForm( worldSeed, rootedForm ) )
            {
                return false;
            }

            GraniteRootedBridgeSetRequest bridgeRequest;

            bridgeRequest.m_worldSeed = worldSeed;
            bridgeRequest.m_structuralEventID =
                rootedForm.m_eventID ^ 0x10B51001u;

            if ( bridgeRequest.m_structuralEventID == 0 )
            {
                bridgeRequest.m_structuralEventID = 0x10B51001u;
            }

            bridgeRequest.m_rootedForm = rootedForm;

            outSequence =
                CertifyGraniteRootedDetachmentSequence( bridgeRequest );

            if ( !outSequence.m_pass )
            {
                return false;
            }

            GraniteRootedSeparationRequest separationRequest;

            separationRequest.m_worldSeed = worldSeed;
            separationRequest.m_partitionEventID =
                rootedForm.m_eventID ^ 0x10B200A1u;

            if ( separationRequest.m_partitionEventID == 0 )
            {
                separationRequest.m_partitionEventID = 0x10B200A1u;
            }

            separationRequest.m_rootedForm = rootedForm;
            separationRequest.m_finalDetachment = outSequence.m_finalEvent;
            separationRequest.m_parentCarrierBaseZ = 0.0f;

            outSeparation =
                BuildGraniteRootedSeparationTransaction(
                    separationRequest );

            if ( !outSeparation.m_pass )
            {
                return false;
            }

            GraniteSpallResolutionRequest spallRequest;

            spallRequest.m_worldSeed = worldSeed;
            spallRequest.m_sourcePartitionEventID =
                outSeparation.m_partitionEventID;
            spallRequest.m_parentDetachedBodyID =
                outSeparation.m_detachedBodyID;
            spallRequest.m_parentProvenanceID =
                outSeparation.m_detachedProvenanceID;
            spallRequest.m_geologicalAncestryID =
                outSeparation.m_geologicalAncestryID;
            spallRequest.m_spallReserveMassGrams =
                outSeparation.m_matter.m_spallReserveMassGrams;
            spallRequest.m_originWorldX = rootedForm.m_centerWorldX;
            spallRequest.m_originWorldY = rootedForm.m_centerWorldY;
            spallRequest.m_originWorldZ = rootedForm.m_maximumReliefM;
            spallRequest.m_policy.m_massResidualToleranceGrams = 0;

            outSpallResolution = ResolveGraniteSpallReserve( spallRequest );

            return outSpallResolution.m_valid;
        }

        //-------------------------------------------------------------------------

        static void RunFractureNegativeControls(
            GraniteFractureTransactionResult const& positive,
            GraniteAuthorityVerificationReport&     aggregate,
            uint32_t                                 caseID )
        {
            {
                GraniteFractureGeometricPartitionReceipt negative =
                    positive.m_geometricPartition;
                negative.m_visibleMeshUnionPass = false;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteFractureGeometricPartitionReceipt",
                    "m_visibleMeshUnionPass" );
            }

            {
                GraniteFractureTransactionResult negative = positive;
                negative.m_massConservationPass = false;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteFractureTransactionResult",
                    "m_massConservationPass" );
            }
        }

        //-------------------------------------------------------------------------

        static void RunRootedNegativeControls(
            GraniteDetachmentSequenceReceipt const& sequence,
            GraniteRootedSeparationResult const&     separation,
            GraniteSpallResolutionResult const&      spallResolution,
            GraniteAuthorityVerificationReport&      aggregate,
            uint32_t                                  caseID )
        {
            {
                GraniteDetachmentResult negative = sequence.m_finalEvent;
                negative.m_parentOwnershipPass = false;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteDetachmentResult",
                    "m_parentOwnershipPass" );
            }

            {
                GraniteDetachmentSequenceReceipt negative = sequence;
                negative.m_monotonicConnectionPass = false;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteDetachmentSequenceReceipt",
                    "m_monotonicConnectionPass" );
            }

            {
                GraniteRootedMatterPartitionReceipt negative =
                    separation.m_matter;
                negative.m_massConservationPass = false;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteRootedMatterPartitionReceipt",
                    "m_massConservationPass" );
            }

            {
                GraniteRootedSeparationResult negative = separation;
                negative.m_principalCastFitPass = false;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteRootedSeparationResult",
                    "m_principalCastFitPass" );
            }

            {
                // The cast-fit distance gate is independent of the aggregate
                // pass flag and must catch a mesh/interface displacement.
                GraniteRootedSeparationResult negative = separation;
                negative.m_maxSharedInterfaceDeviationM =
                    GraniteRootedApprovedShapeThresholds::
                        s_maxPrincipalCastFitDeviationM +
                    0.001f;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteRootedSeparationResult",
                    "m_maxSharedInterfaceDeviationM" );
            }

            {
                // Synthetic rejected outcome: apron matter leaves with the
                // detached body instead of remaining parent-owned.
                GraniteRootedSeparationResult negative = separation;
                negative.m_retainedApronMassGrams = 0;
                negative.m_retainedApronMassFraction = 0.0f;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteRootedSeparationResult",
                    "m_retainedApronMassFraction" );
            }

            {
                // Synthetic rejected outcome: a downward underside curtain
                // consumes more than the approved share of the principal body.
                GraniteRootedSeparationResult negative = separation;
                negative.m_undersideFinVolumeFraction =
                    GraniteRootedApprovedShapeThresholds::
                        s_maxUndersideFinVolumeFraction +
                    0.01f;
                negative.m_undersideFinVolumeM3 =
                    negative.m_matter.m_detachedVolumeM3 *
                    negative.m_undersideFinVolumeFraction;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteRootedSeparationResult",
                    "m_undersideFinVolumeFraction" );
            }

            {
                GraniteSpallResolutionReceipt negative =
                    spallResolution.m_receipt;
                negative.m_lineagePass = false;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteSpallResolutionReceipt",
                    "m_lineagePass" );
            }

            {
                GraniteSpallResolutionResult negative = spallResolution;

                if ( negative.m_numPieces > 0 )
                {
                    negative.m_pieces[0].m_valid = false;
                }
                else if ( negative.m_receipt.m_fineReserveMassGrams > 0 )
                {
                    --negative.m_receipt.m_fineReserveMassGrams;
                }

                GraniteTerminalConservationReceipt const terminal =
                    BuildGraniteTerminalConservationReceipt(
                        separation,
                        negative );

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( terminal, report, caseID );

                char const* expectedGate =
                    spallResolution.m_numPieces > 0
                        ? "m_explicitPieceReceiptPass"
                        : "m_resolutionPass";

                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteTerminalConservationReceipt",
                    expectedGate );
            }

            {
                // The partition reserve is an intermediate handoff, not a
                // terminal output. Reintroducing it into the terminal sum must
                // produce a named failure even when every stage receipt is
                // internally self-consistent.
                GraniteTerminalConservationReceipt negative =
                    BuildGraniteTerminalConservationReceipt(
                        separation,
                        spallResolution );

                negative.m_terminalResidualGrams +=
                    int64_t( negative.m_partitionReserveMassGrams );
                negative.m_terminalPass =
                    negative.m_terminalResidualGrams == 0;
                negative.m_pass =
                    negative.m_pass &&
                    negative.m_terminalPass;

                GraniteAuthorityVerificationReport report;
                ValidateGraniteAuthority( negative, report, caseID );
                RequireNegativeControl(
                    aggregate,
                    report,
                    caseID,
                    "GraniteTerminalConservationReceipt",
                    "m_terminalPass" );
            }
        }

        //-------------------------------------------------------------------------
        // P3C.11A-1 additive strike-input corpus
        //-------------------------------------------------------------------------

        static void RunGraniteToolStrikeContractCase
        (
            uint32_t                                 worldSeed,
            GraniteDetachmentSequenceReceipt const& baselineSequence,
            GraniteRootedSeparationResult const&    baselineSeparation,
            GraniteAuthorityVerificationReport&     report
        )
        {
            int32_t const checksBefore = report.m_numGateChecks;
            ++report.m_numToolStrikeCases;

            GraniteStructuralBridgeSet current =
                baselineSequence.m_initialRooted;
            GraniteToolStrikeResult finalStrike;

            // A contact far outside every bridge must be a named refusal and
            // may not manufacture a detachment receipt or state mutation.
            {
                GraniteToolStrikeRequest missRequest;
                missRequest.m_worldSeed = worldSeed;
                missRequest.m_strikeEventID =
                    baselineSeparation.m_rootedForm.m_eventID ^ 0x11A1F001u;
                if ( missRequest.m_strikeEventID == 0 )
                {
                    missRequest.m_strikeEventID = 0x11A1F001u;
                }
                missRequest.m_rootedForm = baselineSeparation.m_rootedForm;
                missRequest.m_before = current;
                missRequest.m_contactWorldX =
                    baselineSeparation.m_rootedForm.m_centerWorldX + 20.0f;
                missRequest.m_contactWorldY =
                    baselineSeparation.m_rootedForm.m_centerWorldY + 20.0f;
                missRequest.m_contactWorldZ =
                    baselineSeparation.m_rootedForm.m_maximumReliefM;
                missRequest.m_surfaceNormalZ = 1.0f;
                missRequest.m_incomingDirectionX = 0.5f;
                missRequest.m_incomingDirectionZ = -0.8660254f;

                GraniteToolStrikeResult const miss =
                    ApplyGraniteToolStrike( missRequest );

                ++report.m_numNegativeControls;
                CheckGate
                (
                    report,
                    !miss.m_pass &&
                    !miss.m_accepted &&
                    miss.m_rejectionReason ==
                        GraniteToolStrikeRejectionReason::NoBridgeInsideFootprint &&
                    miss.m_detachment.m_effectiveSeverMask == 0,
                    worldSeed,
                    "GraniteToolStrikeResult",
                    "MissLeavesStructuralStateUntouched"
                );
            }

            int32_t const numInitialBridges = current.m_numBridges;

            for ( int32_t strikeOrdinal = 0;
                  strikeOrdinal < numInitialBridges;
                  ++strikeOrdinal )
            {
                int32_t selectedBridgeIndex = -1;

                for ( int32_t bridgeIndex = 0;
                      bridgeIndex < current.m_numBridges;
                      ++bridgeIndex )
                {
                    if ( current.m_bridges[bridgeIndex].m_valid &&
                         !current.m_bridges[bridgeIndex].m_severed )
                    {
                        selectedBridgeIndex = bridgeIndex;
                        break;
                    }
                }

                CheckGate
                (
                    report,
                    selectedBridgeIndex >= 0,
                    worldSeed,
                    "GraniteToolStrikeCorpus",
                    "IntactBridgeAvailable"
                );

                if ( selectedBridgeIndex < 0 )
                {
                    break;
                }

                GraniteStructuralBridgeDescriptor const& bridge =
                    current.m_bridges[selectedBridgeIndex];
                GraniteToolStrikeRequest request;
                request.m_worldSeed = worldSeed;
                request.m_strikeEventID =
                    baselineSeparation.m_rootedForm.m_eventID ^
                    0x11A10000u ^
                    ( 0x9E3779B9u * uint32_t( strikeOrdinal + 1 ) );
                if ( request.m_strikeEventID == 0 )
                {
                    request.m_strikeEventID =
                        0x11A10001u + uint32_t( strikeOrdinal );
                }
                request.m_priorStrikeCount = uint32_t( strikeOrdinal );
                request.m_rootedForm = baselineSeparation.m_rootedForm;
                request.m_before = current;
                request.m_contactWorldX = bridge.m_centerWorldX;
                request.m_contactWorldY = bridge.m_centerWorldY;
                request.m_contactWorldZ =
                    baselineSeparation.m_rootedForm.m_maximumReliefM * 0.25f;
                request.m_surfaceNormalZ = 1.0f;
                request.m_incomingDirectionX = 0.5f;
                request.m_incomingDirectionZ = -0.8660254f;

                GraniteToolStrikeResult const strike =
                    ApplyGraniteToolStrike( request );
                GraniteToolStrikeResult const repeatStrike =
                    ApplyGraniteToolStrike( request );

                CheckGate( report, strike.m_requestPass, worldSeed, "GraniteToolStrikeResult", "m_requestPass" );
                CheckGate( report, strike.m_contactFramePass, worldSeed, "GraniteToolStrikeResult", "m_contactFramePass" );
                CheckGate( report, strike.m_materialResponsePass, worldSeed, "GraniteToolStrikeResult", "m_materialResponsePass" );
                CheckGate( report, strike.m_footprintSelectionPass, worldSeed, "GraniteToolStrikeResult", "m_footprintSelectionPass" );
                CheckGate( report, strike.m_structuralTransactionPass, worldSeed, "GraniteToolStrikeResult", "m_structuralTransactionPass" );
                CheckGate( report, strike.m_pass, worldSeed, "GraniteToolStrikeResult", "m_pass" );
                CheckGate
                (
                    report,
                    strike.m_selectedBridgeIndex == selectedBridgeIndex &&
                    strike.m_selectedBridgeMask ==
                        uint8_t( 1u << selectedBridgeIndex ),
                    worldSeed,
                    "GraniteToolStrikeResult",
                    "ExactBridgeSelection"
                );
                CheckGate
                (
                    report,
                    strike.m_detachment.m_after.m_numIntactBridges ==
                        current.m_numIntactBridges - 1,
                    worldSeed,
                    "GraniteToolStrikeResult",
                    "ExactlyOneBridgeSevered"
                );
                CheckGate
                (
                    report,
                    repeatStrike.m_pass &&
                    repeatStrike.m_selectedBridgeIndex ==
                        strike.m_selectedBridgeIndex &&
                    repeatStrike.m_selectedBridgeDistanceM ==
                        strike.m_selectedBridgeDistanceM &&
                    repeatStrike.m_detachment.m_after.m_remainingConnectionFraction ==
                        strike.m_detachment.m_after.m_remainingConnectionFraction,
                    worldSeed,
                    "GraniteToolStrikeResult",
                    "RepeatStrikeBitExact"
                );

                if ( !strike.m_pass )
                {
                    break;
                }

                current = strike.m_detachment.m_after;
                finalStrike = strike;
            }

            CheckGate
            (
                report,
                finalStrike.m_pass &&
                finalStrike.m_detachment.m_detachedCandidateReady &&
                !finalStrike.m_detachment.m_parentStructuralOwnershipRetained &&
                current.m_numIntactBridges == 0,
                worldSeed,
                "GraniteToolStrikeResult",
                "FinalStrikeReleasesParentOwnership"
            );

            if ( finalStrike.m_pass &&
                 finalStrike.m_detachment.m_detachedCandidateReady )
            {
                GraniteRootedSeparationRequest separationRequest;
                separationRequest.m_worldSeed = worldSeed;
                separationRequest.m_partitionEventID =
                    finalStrike.m_strikeEventID ^ 0x11A120A1u;
                if ( separationRequest.m_partitionEventID == 0 )
                {
                    separationRequest.m_partitionEventID = 0x11A120A1u;
                }
                separationRequest.m_rootedForm =
                    baselineSeparation.m_rootedForm;
                separationRequest.m_finalDetachment =
                    finalStrike.m_detachment;
                separationRequest.m_parentCarrierBaseZ = 0.0f;

                GraniteRootedSeparationResult const separation =
                    BuildGraniteRootedSeparationTransaction
                    (
                        separationRequest
                    );
                ValidateGraniteAuthority( separation.m_matter, report, worldSeed );
                ValidateGraniteAuthority( separation, report, worldSeed );

                GraniteSpallResolutionRequest spallRequest;
                spallRequest.m_worldSeed = worldSeed;
                spallRequest.m_sourcePartitionEventID =
                    separation.m_partitionEventID;
                spallRequest.m_parentDetachedBodyID =
                    separation.m_detachedBodyID;
                spallRequest.m_parentProvenanceID =
                    separation.m_detachedProvenanceID;
                spallRequest.m_geologicalAncestryID =
                    separation.m_geologicalAncestryID;
                spallRequest.m_spallReserveMassGrams =
                    separation.m_matter.m_spallReserveMassGrams;
                spallRequest.m_originWorldX =
                    separation.m_rootedForm.m_centerWorldX;
                spallRequest.m_originWorldY =
                    separation.m_rootedForm.m_centerWorldY;
                spallRequest.m_originWorldZ =
                    separation.m_rootedForm.m_maximumReliefM;
                spallRequest.m_policy.m_massResidualToleranceGrams = 0;

                GraniteSpallResolutionResult const spall =
                    ResolveGraniteSpallReserve( spallRequest );
                ValidateGraniteAuthority( spall.m_receipt, report, worldSeed );

                GraniteTerminalConservationReceipt const terminal =
                    BuildGraniteTerminalConservationReceipt
                    (
                        separation,
                        spall
                    );
                ValidateGraniteAuthority( terminal, report, worldSeed );
            }

            report.m_numToolStrikeChecks +=
                report.m_numGateChecks - checksBefore;
        }
    }

    //-------------------------------------------------------------------------

    GraniteTerminalConservationReceipt
    BuildGraniteTerminalConservationReceipt(
        GraniteRootedSeparationResult const& separation,
        GraniteSpallResolutionResult const&  spallResolution )
    {
        GraniteTerminalConservationReceipt receipt;

        GraniteRootedMatterPartitionReceipt const& partition =
            separation.m_matter;
        GraniteSpallResolutionReceipt const& resolution =
            spallResolution.m_receipt;

        receipt.m_preDetachParcelMassGrams =
            partition.m_preDetachParcelMassGrams;
        receipt.m_remainingParentMassGrams =
            partition.m_remainingParentMassGrams;
        receipt.m_detachedMassGrams =
            partition.m_detachedMassGrams;
        receipt.m_partitionReserveMassGrams =
            partition.m_spallReserveMassGrams;
        receipt.m_resolutionRequestReserveMassGrams =
            spallResolution.m_request.m_spallReserveMassGrams;
        receipt.m_resolutionReceiptInputMassGrams =
            resolution.m_inputReserveMassGrams;
        receipt.m_reportedExplicitPieceMassGrams =
            resolution.m_explicitPieceMassGrams;
        receipt.m_fineReserveMassGrams =
            resolution.m_fineReserveMassGrams;
        receipt.m_reportedExplicitPieceCount =
            resolution.m_numExplicitPieces;

        int32_t const boundedPieceCount =
            spallResolution.m_numPieces < 0
                ? 0
                : (
                      spallResolution.m_numPieces >
                              GraniteSpallResolutionResult::s_maxPieces
                          ? GraniteSpallResolutionResult::s_maxPieces
                          : spallResolution.m_numPieces );

        for ( int32_t pieceIndex = 0;
              pieceIndex < boundedPieceCount;
              ++pieceIndex )
        {
            GraniteSpallPieceDescriptor const& piece =
                spallResolution.m_pieces[pieceIndex];

            if ( !piece.m_valid )
            {
                continue;
            }

            receipt.m_actualExplicitPieceMassGrams += piece.m_massGrams;
            ++receipt.m_actualExplicitPieceCount;
        }

        receipt.m_partitionResidualGrams =
            int64_t( receipt.m_remainingParentMassGrams ) +
            int64_t( receipt.m_detachedMassGrams ) +
            int64_t( receipt.m_partitionReserveMassGrams ) -
            int64_t( receipt.m_preDetachParcelMassGrams );

        receipt.m_requestHandoffResidualGrams =
            int64_t( receipt.m_resolutionRequestReserveMassGrams ) -
            int64_t( receipt.m_partitionReserveMassGrams );

        receipt.m_receiptInputResidualGrams =
            int64_t( receipt.m_resolutionReceiptInputMassGrams ) -
            int64_t( receipt.m_resolutionRequestReserveMassGrams );

        receipt.m_explicitPieceReceiptResidualGrams =
            int64_t( receipt.m_actualExplicitPieceMassGrams ) -
            int64_t( receipt.m_reportedExplicitPieceMassGrams );

        receipt.m_resolutionResidualGrams =
            int64_t( receipt.m_actualExplicitPieceMassGrams ) +
            int64_t( receipt.m_fineReserveMassGrams ) -
            int64_t( receipt.m_resolutionReceiptInputMassGrams );

        receipt.m_terminalResidualGrams =
            int64_t( receipt.m_remainingParentMassGrams ) +
            int64_t( receipt.m_detachedMassGrams ) +
            int64_t( receipt.m_actualExplicitPieceMassGrams ) +
            int64_t( receipt.m_fineReserveMassGrams ) -
            int64_t( receipt.m_preDetachParcelMassGrams );

        receipt.m_partitionPass =
            receipt.m_partitionResidualGrams == 0;
        receipt.m_requestHandoffPass =
            receipt.m_requestHandoffResidualGrams == 0;
        receipt.m_receiptInputPass =
            receipt.m_receiptInputResidualGrams == 0;
        receipt.m_explicitPieceReceiptPass =
            receipt.m_explicitPieceReceiptResidualGrams == 0;
        receipt.m_explicitPieceCountPass =
            receipt.m_actualExplicitPieceCount ==
            receipt.m_reportedExplicitPieceCount;
        receipt.m_resolutionPass =
            receipt.m_resolutionResidualGrams == 0;
        receipt.m_terminalPass =
            receipt.m_terminalResidualGrams == 0;

        receipt.m_pass =
            receipt.m_partitionPass &&
            receipt.m_requestHandoffPass &&
            receipt.m_receiptInputPass &&
            receipt.m_explicitPieceReceiptPass &&
            receipt.m_explicitPieceCountPass &&
            receipt.m_resolutionPass &&
            receipt.m_terminalPass;

        return receipt;
    }

    //-------------------------------------------------------------------------

    void ValidateGraniteAuthority(
        GraniteFractureGeometricPartitionReceipt const& receipt,
        GraniteAuthorityVerificationReport&             report,
        uint32_t                                         caseID )
    {
        char const* const receiptName =
            "GraniteFractureGeometricPartitionReceipt";

        CheckGate( report, receipt.m_visibleMeshUnionPass, caseID, receiptName, "m_visibleMeshUnionPass" );
        CheckGate( report, receipt.m_exteriorPartitionPass, caseID, receiptName, "m_exteriorPartitionPass" );
        CheckGate( report, receipt.m_sharedCutCoincidencePass, caseID, receiptName, "m_sharedCutCoincidencePass" );
        CheckGate( report, receipt.m_halfSpaceOwnershipPass, caseID, receiptName, "m_halfSpaceOwnershipPass" );
        CheckGate( report, receipt.m_closedChildTopologyPass, caseID, receiptName, "m_closedChildTopologyPass" );
    }

    //-------------------------------------------------------------------------

    void ValidateGraniteAuthority(
        GraniteFractureTransactionResult const& receipt,
        GraniteAuthorityVerificationReport&     report,
        uint32_t                                 caseID )
    {
        char const* const receiptName =
            "GraniteFractureTransactionResult";

        CheckGate( report, receipt.m_massConservationPass, caseID, receiptName, "m_massConservationPass" );
        CheckGate( report, receipt.m_solidVolumeConservationPass, caseID, receiptName, "m_solidVolumeConservationPass" );
        CheckGate( report, receipt.m_childGeometryPass, caseID, receiptName, "m_childGeometryPass" );
        CheckGate( report, receipt.m_surfaceHistoryPass, caseID, receiptName, "m_surfaceHistoryPass" );
        CheckGate( report, receipt.m_geometricPartitionPass, caseID, receiptName, "m_geometricPartitionPass" );
    }

    //-------------------------------------------------------------------------

    void ValidateGraniteAuthority(
        GraniteDetachmentResult const&       receipt,
        GraniteAuthorityVerificationReport& report,
        uint32_t                             caseID )
    {
        char const* const receiptName = "GraniteDetachmentResult";

        CheckGate( report, receipt.m_monotonicConnectionPass, caseID, receiptName, "m_monotonicConnectionPass" );
        CheckGate( report, receipt.m_stateTransitionPass, caseID, receiptName, "m_stateTransitionPass" );
        CheckGate( report, receipt.m_bridgeAccountingPass, caseID, receiptName, "m_bridgeAccountingPass" );
        CheckGate( report, receipt.m_parentOwnershipPass, caseID, receiptName, "m_parentOwnershipPass" );
    }

    //-------------------------------------------------------------------------

    void ValidateGraniteAuthority(
        GraniteDetachmentSequenceReceipt const& receipt,
        GraniteAuthorityVerificationReport&     report,
        uint32_t                                 caseID )
    {
        char const* const receiptName =
            "GraniteDetachmentSequenceReceipt";

        CheckGate( report, receipt.m_initialRootedPass, caseID, receiptName, "m_initialRootedPass" );
        CheckGate( report, receipt.m_partialStillOwnedByParentPass, caseID, receiptName, "m_partialStillOwnedByParentPass" );
        CheckGate( report, receipt.m_finalParentOwnershipReleasedPass, caseID, receiptName, "m_finalParentOwnershipReleasedPass" );
        CheckGate( report, receipt.m_monotonicConnectionPass, caseID, receiptName, "m_monotonicConnectionPass" );
    }

    //-------------------------------------------------------------------------

    void ValidateGraniteAuthority(
        GraniteRootedMatterPartitionReceipt const& receipt,
        GraniteAuthorityVerificationReport&        report,
        uint32_t                                    caseID )
    {
        char const* const receiptName =
            "GraniteRootedMatterPartitionReceipt";

        CheckGate( report, receipt.m_massConservationPass, caseID, receiptName, "m_massConservationPass" );
        CheckGate( report, receipt.m_volumeConservationPass, caseID, receiptName, "m_volumeConservationPass" );
    }

    //-------------------------------------------------------------------------

    void ValidateGraniteAuthority(
        GraniteRootedSeparationResult const& receipt,
        GraniteAuthorityVerificationReport& report,
        uint32_t                             caseID )
    {
        char const* const receiptName =
            "GraniteRootedSeparationResult";

        CheckGate( report, receipt.m_structuralReleasePass, caseID, receiptName, "m_structuralReleasePass" );
        CheckGate( report, receipt.m_geometricPartitionPass, caseID, receiptName, "m_geometricPartitionPass" );
        CheckGate( report, receipt.m_sharedInterfacePass, caseID, receiptName, "m_sharedInterfacePass" );
        CheckGate( report, receipt.m_detachedMeshClosedPass, caseID, receiptName, "m_detachedMeshClosedPass" );
        CheckGate( report, receipt.m_parentSocketMeshClosedPass, caseID, receiptName, "m_parentSocketMeshClosedPass" );
        CheckGate( report, receipt.m_principalCastFitPass, caseID, receiptName, "m_principalCastFitPass" );
        CheckGate(
            report,
            receipt.m_maxSharedInterfaceDeviationM <=
                GraniteRootedApprovedShapeThresholds::
                    s_maxPrincipalCastFitDeviationM,
            caseID,
            receiptName,
            "m_maxSharedInterfaceDeviationM" );
        CheckGate(
            report,
            receipt.m_retainedApronMassFraction >=
                GraniteRootedApprovedShapeThresholds::
                    s_minRetainedApronMassFraction,
            caseID,
            receiptName,
            "m_retainedApronMassFraction" );
        CheckGate(
            report,
            receipt.m_undersideFinVolumeFraction <=
                GraniteRootedApprovedShapeThresholds::
                    s_maxUndersideFinVolumeFraction,
            caseID,
            receiptName,
            "m_undersideFinVolumeFraction" );
        CheckGate( report, receipt.m_approvedShapePass, caseID, receiptName, "m_approvedShapePass" );
        CheckGate( report, receipt.m_spallReservePass, caseID, receiptName, "m_spallReservePass" );
        CheckGate( report, receipt.m_matterPartitionPass, caseID, receiptName, "m_matterPartitionPass" );
    }

    //-------------------------------------------------------------------------

    void ValidateGraniteAuthority(
        GraniteSpallResolutionReceipt const& receipt,
        GraniteAuthorityVerificationReport& report,
        uint32_t                             caseID )
    {
        char const* const receiptName =
            "GraniteSpallResolutionReceipt";

        CheckGate( report, receipt.m_uniqueBodyIDsPass, caseID, receiptName, "m_uniqueBodyIDsPass" );
        CheckGate( report, receipt.m_lineagePass, caseID, receiptName, "m_lineagePass" );
        CheckGate( report, receipt.m_pieceMinimumMassPass, caseID, receiptName, "m_pieceMinimumMassPass" );
        CheckGate( report, receipt.m_massConservationPass, caseID, receiptName, "m_massConservationPass" );
    }

    //-------------------------------------------------------------------------

    void ValidateGraniteAuthority(
        GraniteTerminalConservationReceipt const& receipt,
        GraniteAuthorityVerificationReport&       report,
        uint32_t                                   caseID )
    {
        char const* const receiptName =
            "GraniteTerminalConservationReceipt";

        CheckGate( report, receipt.m_partitionPass, caseID, receiptName, "m_partitionPass" );
        CheckGate( report, receipt.m_requestHandoffPass, caseID, receiptName, "m_requestHandoffPass" );
        CheckGate( report, receipt.m_receiptInputPass, caseID, receiptName, "m_receiptInputPass" );
        CheckGate( report, receipt.m_explicitPieceReceiptPass, caseID, receiptName, "m_explicitPieceReceiptPass" );
        CheckGate( report, receipt.m_explicitPieceCountPass, caseID, receiptName, "m_explicitPieceCountPass" );
        CheckGate( report, receipt.m_resolutionPass, caseID, receiptName, "m_resolutionPass" );
        CheckGate( report, receipt.m_terminalPass, caseID, receiptName, "m_terminalPass" );
    }

    //-------------------------------------------------------------------------

    GraniteAuthorityVerificationReport
    RunGraniteAuthorityVerificationCorpus()
    {
        GraniteAuthorityVerificationReport report;

        struct FractureCorpusCase
        {
            uint32_t m_worldSeed = 0;
            uint32_t m_topologyFamily = 0;
        };

        static constexpr FractureCorpusCase corpusCases[] =
        {
            { 8675309u, 0u },
            { 8675309u, 1u },
            { 8675309u, 2u },
            { 8675309u, 3u },
            { 0x00C0FFEEu, 0u },
            { 0x00C0FFEEu, 1u },
            { 0x00C0FFEEu, 2u },
            { 0x00C0FFEEu, 3u },
            { 0x5EED1234u, 0u },
            { 0x5EED1234u, 1u },
            { 0x5EED1234u, 2u },
            { 0x5EED1234u, 3u },
            { 0x5EED1235u, 0u },
            { 0x5EED1235u, 1u },
            { 0x5EED1235u, 2u },
            { 0x5EED1235u, 3u }
        };

        // Task 4 approved exact-output corpus. Values are 128-bit canonical
        // fingerprints recorded only after Debug and /O2 /fp:precise Release
        // produced identical results. The algorithm version is part of every
        // fingerprint, so an intentional generator migration must increment
        // that version and publish a complete replacement table.
        static constexpr GraniteAuthorityCaseFingerprint
            approvedFingerprints[] =
        {
            { 8675309u, 0u, 0x9A73BDE8886EEB6Aull, 0x7D9F4B3F1FE57943ull },
            { 8675309u, 1u, 0x0C22CC7596E24617ull, 0xF657D4C2DF97F64Aull },
            { 8675309u, 2u, 0xA950DCC3E439AF32ull, 0xCB7C361DE467220Cull },
            { 8675309u, 3u, 0x5E27A2B899A203DFull, 0xE230CB62F831C854ull },
            { 0x00C0FFEEu, 0u, 0x474437D25A61826Bull, 0x8B0CE684387E9769ull },
            { 0x00C0FFEEu, 1u, 0x1A873EAFDD37CB66ull, 0x9F535DF6BE0D45DDull },
            { 0x00C0FFEEu, 2u, 0x0BF7CFEC5412430Cull, 0xE51FA3578C96EE87ull },
            { 0x00C0FFEEu, 3u, 0x462BB1218974DDC0ull, 0x021F3E25520C280Dull },
            { 0x5EED1234u, 0u, 0xA2BAEB1889748A65ull, 0xE4879FEFF8241BE6ull },
            { 0x5EED1234u, 1u, 0xBB31C19BA101B13Bull, 0x5E680A3C76477B14ull },
            { 0x5EED1234u, 2u, 0x8346914C6EBF55D3ull, 0xA3D6C61086AF794Cull },
            { 0x5EED1234u, 3u, 0x474122E3AF90DF6Full, 0xE49D1B95489ED5BBull },
            { 0x5EED1235u, 0u, 0x42B12C9C480DED01ull, 0x72D37C1DA8E6DC0Eull },
            { 0x5EED1235u, 1u, 0x48FB08FDF350C39Aull, 0x95271FC95F74F7BFull },
            { 0x5EED1235u, 2u, 0xBEC3D168A0CA5B52ull, 0x59F9913D18295C7Full },
            { 0x5EED1235u, 3u, 0x2EE98703B0C8254Full, 0x94CE8193BCA7B952ull }
        };

        static_assert(
            sizeof( corpusCases ) / sizeof( corpusCases[0] ) ==
                sizeof( approvedFingerprints ) /
                    sizeof( approvedFingerprints[0] ),
            "Every Granite corpus case requires an approved fingerprint" );

        bool fractureNegativeControlsRun = false;
        bool rootedNegativeControlsRun = false;
        bool fingerprintNegativeControlRun = false;
        bool floatReachabilityControlsRun = false;

        for ( uint32_t caseOrdinal = 0;
              caseOrdinal < uint32_t( sizeof( corpusCases ) / sizeof( corpusCases[0] ) );
              ++caseOrdinal )
        {
            FractureCorpusCase const& corpusCase =
                corpusCases[caseOrdinal];
            uint32_t const worldSeed = corpusCase.m_worldSeed;
            ++report.m_numCorpusCases;

            GraniteFractureCorpusArtifacts fractureArtifacts;
            GraniteFractureTransactionResult& fracture =
                fractureArtifacts.m_transaction;

            if ( !BuildFractureCorpusCase(
                     worldSeed,
                     corpusCase.m_topologyFamily,
                     fractureArtifacts ) )
            {
                AppendFailure(
                    report,
                    worldSeed,
                    GetFractureCorpusReceiptName(
                        fracture.m_parentTopologyFamily ),
                    GetFractureRejectionGateName(
                        fracture.m_rejectionReason ) );

                if ( fracture.m_rejectionReason ==
                     GraniteFractureRejectionReason::CertificateGateFailed )
                {
                    ValidateGraniteAuthority(
                        fracture.m_geometricPartition,
                        report,
                        worldSeed );
                    ValidateGraniteAuthority(
                        fracture,
                        report,
                        worldSeed );
                }
            }
            else
            {
                ValidateGraniteAuthority(
                    fracture.m_geometricPartition,
                    report,
                    worldSeed );
                ValidateGraniteAuthority(
                    fracture,
                    report,
                    worldSeed );

                if ( !fractureNegativeControlsRun )
                {
                    RunFractureNegativeControls(
                        fracture,
                        report,
                        worldSeed );
                    fractureNegativeControlsRun = true;
                }
            }

            GraniteDetachmentSequenceReceipt sequence;
            GraniteRootedSeparationResult separation;
            GraniteSpallResolutionResult spallResolution;

            if ( !BuildRootedCorpusCase(
                     worldSeed,
                     sequence,
                     separation,
                     spallResolution ) )
            {
                AppendFailure(
                    report,
                    worldSeed,
                    "GraniteAuthorityCorpus",
                    "BuildRootedCorpusCase" );
                continue;
            }

            ValidateGraniteAuthority(
                sequence.m_partialEvent,
                report,
                worldSeed );
            ValidateGraniteAuthority(
                sequence.m_finalEvent,
                report,
                worldSeed );
            ValidateGraniteAuthority(
                sequence,
                report,
                worldSeed );
            ValidateGraniteAuthority(
                separation.m_matter,
                report,
                worldSeed );
            ValidateGraniteAuthority(
                separation,
                report,
                worldSeed );

            RecordRootedShapeEnvelope(
                report,
                separation );

            ValidateGraniteAuthority(
                spallResolution.m_receipt,
                report,
                worldSeed );

            GraniteTerminalConservationReceipt const terminal =
                BuildGraniteTerminalConservationReceipt(
                    separation,
                    spallResolution );

            ValidateGraniteAuthority(
                terminal,
                report,
                worldSeed );

            // One additive strike path per qualified seed. Topology families
            // remain exclusive to the frozen fracture corpus above.
            if ( corpusCase.m_topologyFamily == 0u )
            {
                RunGraniteToolStrikeContractCase
                (
                    worldSeed,
                    sequence,
                    separation,
                    report
                );
            }

            GraniteAuthorityCaseFingerprint const fingerprint =
                BuildGraniteCaseFingerprint(
                    worldSeed,
                    corpusCase.m_topologyFamily,
                    fractureArtifacts,
                    sequence,
                    separation,
                    spallResolution,
                    terminal );

            CheckDeterminismGate(
                report,
                GraniteFingerprintsEqual(
                    fingerprint,
                    approvedFingerprints[caseOrdinal] ),
                worldSeed,
                "ApprovedBitPatternMatch" );

            if ( report.m_numFingerprints <
                 GraniteAuthorityVerificationReport::s_maxFingerprints )
            {
                report.m_fingerprints[report.m_numFingerprints++] =
                    fingerprint;
            }
            else
            {
                AppendFailure(
                    report,
                    worldSeed,
                    "GraniteAuthorityDeterminism",
                    "FingerprintCapacityExceeded" );
            }

            GraniteFractureCorpusArtifacts repeatFractureArtifacts;
            GraniteDetachmentSequenceReceipt repeatSequence;
            GraniteRootedSeparationResult repeatSeparation;
            GraniteSpallResolutionResult repeatSpallResolution;

            bool const repeatFractureBuilt =
                BuildFractureCorpusCase(
                    worldSeed,
                    corpusCase.m_topologyFamily,
                    repeatFractureArtifacts );
            bool const repeatRootedBuilt =
                BuildRootedCorpusCase(
                    worldSeed,
                    repeatSequence,
                    repeatSeparation,
                    repeatSpallResolution );

            CheckDeterminismGate(
                report,
                repeatFractureBuilt && repeatRootedBuilt,
                worldSeed,
                "RepeatCorpusCaseBuilt" );

            if ( repeatFractureBuilt && repeatRootedBuilt )
            {
                GraniteTerminalConservationReceipt const repeatTerminal =
                    BuildGraniteTerminalConservationReceipt(
                        repeatSeparation,
                        repeatSpallResolution );

                GraniteAuthorityCaseFingerprint const repeatFingerprint =
                    BuildGraniteCaseFingerprint(
                        worldSeed,
                        corpusCase.m_topologyFamily,
                        repeatFractureArtifacts,
                        repeatSequence,
                        repeatSeparation,
                        repeatSpallResolution,
                        repeatTerminal );

                CheckDeterminismGate(
                    report,
                    GraniteFingerprintsEqual(
                        fingerprint,
                        repeatFingerprint ),
                    worldSeed,
                    "RepeatBitPatternMatch" );
            }

            if ( !fingerprintNegativeControlRun )
            {
                RequireFingerprintNegativeControl(
                    report,
                    fingerprint,
                    worldSeed );
                fingerprintNegativeControlRun = true;
            }

            if ( !floatReachabilityControlsRun )
            {
                RequireGraniteFloatReachabilityControls(
                    report,
                    fingerprint,
                    worldSeed,
                    corpusCase.m_topologyFamily,
                    fractureArtifacts,
                    sequence,
                    separation,
                    spallResolution,
                    terminal );
                floatReachabilityControlsRun = true;
            }

            if ( !rootedNegativeControlsRun )
            {
                RunRootedNegativeControls(
                    sequence,
                    separation,
                    spallResolution,
                    report,
                    worldSeed );
                rootedNegativeControlsRun = true;
            }
        }

        if ( !fractureNegativeControlsRun )
        {
            AppendFailure(
                report,
                0,
                "GraniteAuthorityNegativeControl",
                "FractureControlsNotRun" );
        }

        if ( !rootedNegativeControlsRun )
        {
            AppendFailure(
                report,
                0,
                "GraniteAuthorityNegativeControl",
                "RootedControlsNotRun" );
        }

        if ( !fingerprintNegativeControlRun )
        {
            AppendFailure(
                report,
                0,
                "GraniteAuthorityNegativeControl",
                "FingerprintControlNotRun" );
        }

        if ( !floatReachabilityControlsRun )
        {
            AppendFailure(
                report,
                0,
                "GraniteAuthorityNegativeControl",
                "FloatReachabilityControlsNotRun" );
        }

        RunGraniteExcavationCertificate(report);

        report.m_pass =
            report.m_numFailures == 0 &&
            !report.m_failureCapacityExceeded;

        return report;
    }
}
