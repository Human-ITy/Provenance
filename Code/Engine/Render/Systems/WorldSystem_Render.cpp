#include "WorldSystem_Render.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Render/Components/Component_EnvironmentMaps.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/RenderGeometryBuilder.h"
#include "Engine/Render/RenderMaterial.h"
#include "Engine/Render/RenderViewport.h"
#include "Base/Types/Arrays.h"
#include "Base/Profiling.h"
#include "Base/Render/RHI.h"
#include "Base/Threading/TaskSystem.h"

#include "Engine/Render/Shaders/MeshInstance.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    #if EE_DEVELOPMENT_TOOLS
    void RenderWorldSystem::UpdateViewportPickingData( RenderViewport* pViewport ) const
    {
        PickingData& pickingData = pViewport->GetPickingData();
        pickingData.clear();

        if ( !pViewport->IsPickingEnabled() )
        {
            return;
        }

        auto ResolvePickingData = [this] ( DeviceAppendBuffer<PickingResult> const& buffer, PickingData& pickingData )
        {
            auto TryResolvePickingID = [this] ( PickingResult const& pr )
            {
                if ( pr.m_hitTestID != PickingID::InvalidID )
                {
                    return PickingID( pr.m_hitTestID, PickingID::InvalidID, pr.m_sortPriority, pr.m_intersectionDistance );
                }

                for ( StaticMeshComponent const* pComponent : m_staticMeshComponents )
                {
                    if ( ( pComponent->m_meshInstanceProxy.m_instanceHandle.m_offset <= pr.m_instanceID ) && ( pr.m_instanceID < ( pComponent->m_meshInstanceProxy.m_instanceHandle.m_offset + pComponent->m_meshInstanceProxy.m_instanceHandle.m_size ) ) )
                    {
                        return PickingID( pComponent->GetEntityID().m_value, pComponent->GetID().m_value, pr.m_sortPriority, pr.m_intersectionDistance );
                    }
                }

                for ( SkeletalMeshComponent const* pComponent : m_skeletalMeshComponents )
                {
                    if ( ( pComponent->m_meshInstanceProxy.m_instanceHandle.m_offset <= pr.m_instanceID ) && ( pr.m_instanceID < ( pComponent->m_meshInstanceProxy.m_instanceHandle.m_offset + pComponent->m_meshInstanceProxy.m_instanceHandle.m_size ) ) )
                    {
                        return PickingID( pComponent->GetEntityID().m_value, pComponent->GetID().m_value, pr.m_sortPriority, pr.m_intersectionDistance );
                    }
                }

                return PickingID();
            };

            //-----------------------------------------------------------------------------------------------

            for ( PickingResult const& result : buffer.m_bufferData )
            {
                PickingID pickingID = TryResolvePickingID( result );
                if ( pickingID.IsSet() )
                {
                    pickingData.push_back( pickingID );
                }
            }
        };

        uint32_t frameIndex = m_pRenderSystem->GetFrameIndex();

        ResolvePickingData( pViewport->m_debugDrawPickingResultsBuffer, pickingData );
        ResolvePickingData( pViewport->m_instancePickingResultsBuffer, pickingData );

        pickingData.DeduplicateAndSort();
    }
    #endif

    void RenderWorldSystem::InitializeSystem( SystemRegistry const& systemRegistry )
    {
        m_pTaskSystem = systemRegistry.GetSystem<TaskSystem>();
        m_pRenderSystem = systemRegistry.GetSystem<RenderSystem>();

        m_deviceRenderWorld.Initialize( m_pTaskSystem, m_pRenderSystem );

        m_materialShaderClusterCapacity.Initialize();

        // TODO: Need to make it a resource instead of allocating it here
        static constexpr uint32_t g_RadianceResolution = 128;
        static constexpr uint32_t g_IrradianceResolution = 32;

        RHI::TextureParameters renderTargetParameters = {};
        renderTargetParameters.m_width = g_RadianceResolution;
        renderTargetParameters.m_height = g_RadianceResolution;
        renderTargetParameters.m_arrayLayers = 6;
        renderTargetParameters.m_mipLevels = RHI::ComputeTextureMipLevels( g_RadianceResolution, g_RadianceResolution, 1 );
        renderTargetParameters.m_format = RHI::DataFormat::RGBA16_SFloat;
        renderTargetParameters.m_descriptorTypes = TBitFlags<RHI::DescriptorTypeFlags>( RHI::DescriptorTypeFlags::TextureCube,
                                                                                        RHI::DescriptorTypeFlags::RenderTarget );
        renderTargetParameters.m_clearValue = { { 0.0F, 0.0F, 0.0F, 1.0F } };
        renderTargetParameters.m_debugName = "GlobalEnvironmentMap Radiance Target";

        m_pRadianceTexture = RHI::CreateTexture( m_pRenderSystem->GetContextRHI(), renderTargetParameters );

        renderTargetParameters.m_width = g_IrradianceResolution;
        renderTargetParameters.m_height = g_IrradianceResolution;
        renderTargetParameters.m_format = RHI::DataFormat::RGBA32_SFloat;
        renderTargetParameters.m_mipLevels = 1;
        renderTargetParameters.m_debugName = "GlobalEnvironmentMap Irradiance Target";

        m_pIrradianceTexture = RHI::CreateTexture( m_pRenderSystem->GetContextRHI(), renderTargetParameters );
    }

    ProceduralMeshID RenderWorldSystem::RegisterProceduralMesh
    (
        TArrayView<ProceduralMeshVertex const> vertices,
        TArrayView<uint32_t const>             indices,
        Material const*                        pMaterial,
        Transform const&                       worldTransform,
        TBitFlags<ViewLayer>                   viewLayers,
        bool                                  fastBuild,
        float                                 vertexDisplacementRadius
    )
    {
        if ( vertices.empty() ||
             indices.empty() ||
             ( indices.size() % 3 ) != 0 )
        {
            return 0;
        }

        if ( pMaterial == nullptr )
        {
            pMaterial = m_pRenderSystem->GetPlaceholderMaterial();
        }

        EE_ASSERT( pMaterial != nullptr && pMaterial->IsValid() );

        GeometryBuilder geometryBuilder;
        geometryBuilder.SetNumTextureCoordinateAttributes( 2 );
        geometryBuilder.SetNumColorAttributes( 1 );
        geometryBuilder.InitializeVertexFormat();
        geometryBuilder.SetIndices( indices, false );
        geometryBuilder.SetNumVertices( vertices.size() );

        for ( size_t vertexIndex = 0;
              vertexIndex < vertices.size();
              ++vertexIndex )
        {
            ProceduralMeshVertex const& vertex = vertices[vertexIndex];
            geometryBuilder.SetPositionAttribute
            (
                vertexIndex,
                { vertex.m_position, vertex.m_normal }
            );
            geometryBuilder.SetTextureCoordinateAttribute( vertexIndex, 0, vertex.m_uv0 );
            geometryBuilder.SetTextureCoordinateAttribute( vertexIndex, 1, vertex.m_uv1 );
            geometryBuilder.SetColorAttribute( vertexIndex, 0, Color( vertex.m_color ) );
        }

        // Runtime geometry arrives as triangle lists and commonly repeats the
        // same position/normal at every adjacent face (fracture boundaries and
        // crossed foliage cards are the two largest producers). Deduplicate
        // exact vertex attributes before cache/fetch optimization. This does
        // not simplify or move geometry, but substantially lowers meshlet
        // vertex pressure and near-camera cluster work.
        geometryBuilder.Optimize( TBitFlags<GeometryOptimizeFlags>(
            GeometryOptimizeFlags::VertexRemap,
            GeometryOptimizeFlags::VertexCache,
            GeometryOptimizeFlags::VertexFetch ) );

        ProceduralMeshInstance instance;
        instance.m_ID = m_nextProceduralMeshID++;
        instance.m_pMaterial = pMaterial;
        instance.m_viewLayers = viewLayers;
        instance.m_worldTransform = worldTransform;
        instance.m_geometry.SetVertexStride( sizeof( StaticMeshVertex ) );
        AABB bounds = geometryBuilder.ComputeAABB();
        float const displacement = Math::Max( 0.0f, vertexDisplacementRadius );
        bounds.Expand( Vector( displacement ) );
        instance.m_geometry.SetBounds( OBB( bounds ) );
        geometryBuilder.BuildAndAppendGeometry( instance.m_geometry, fastBuild );
        // Both instance and meshlet culling must contain animated vertices.
        auto* clusters = reinterpret_cast<MeshCluster*>( instance.m_geometry.GetClusters().data() );
        for ( uint32_t i = 0; i < instance.m_geometry.GetNumClusters(); ++i )
        {
            clusters[i].m_boundingSphereRadius += displacement;
        }

        ProceduralMeshID const result = instance.m_ID;
        m_proceduralMeshInstances.emplace_back( eastl::move( instance ) );
        return result;
    }

    bool RenderWorldSystem::UpdateProceduralMeshTransform( ProceduralMeshID meshID, Transform const& worldTransform )
    {
        for ( auto& instance : m_proceduralMeshInstances )
        {
            if ( instance.m_ID != meshID ) continue;
            instance.m_worldTransform = worldTransform;
            instance.m_transformUpdatePending = true;
            return true;
        }
        return false;
    }

    Material const* RenderWorldSystem::CreateSurfaceCoverMaterial( Material const* pSource )
    {
        if ( m_pSurfaceCoverMaterial ) return m_pSurfaceCoverMaterial;
        if ( !pSource || !pSource->m_shaderParametersInstance.IsValid() ) return nullptr;
        auto const& source = pSource->m_shaderParametersInstance;
        if ( !source.FindParameter( StringID( "m_coverTexture" ) ).IsValid() ) return nullptr;
        m_pSurfaceCoverMaterial = EE::New<Material>();
        m_pSurfaceCoverMaterial->m_shaderIndex = pSource->m_shaderIndex;
        m_surfaceCoverSourceParameters.assign( source.m_parametersMemory.begin(), source.m_parametersMemory.end() );
        return m_pSurfaceCoverMaterial;
    }

    void RenderWorldSystem::SetSurfaceCoverData( uint32_t width, uint32_t height, TArrayView<Float4 const> pixels )
    {
        if ( !m_pSurfaceCoverMaterial || width == 0 || height == 0 || width > 2048 || height > 2048 || pixels.size() != size_t(width) * height ) return;
        if ( m_pSurfaceCoverTexture && (width!=m_surfaceCoverWidth || height!=m_surfaceCoverHeight) ) return;
        m_surfaceCoverWidth = width; m_surfaceCoverHeight = height;
        m_surfaceCoverPixels.assign( pixels.begin(), pixels.end() );
        m_surfaceCoverDirty = true;
    }

    void RenderWorldSystem::SetSurfaceCoverWindPhase( float phase )
    {
        m_surfaceCoverWindPhase = phase;
        m_surfaceCoverWindDirty = true;
    }

    void RenderWorldSystem::UpdateSurfaceCoverWindResources()
    {
        if ( !m_pSurfaceCoverMaterial || !m_surfaceCoverWindDirty ) return;
        auto& parameters = m_pSurfaceCoverMaterial->m_shaderParametersInstance;
        if ( !parameters.IsValid() ) return;
        auto parameter = parameters.FindParameter( StringID( "m_windPhase" ) );
        if ( !parameter.IsValid() ) return;
        parameters.SetScalar( parameter, m_surfaceCoverWindPhase );
        m_pRenderSystem->QueueShaderParametersUpdate( parameters );
        m_surfaceCoverWindDirty = false;
    }

    void RenderWorldSystem::UpdateSurfaceCoverResources()
    {
        if ( !m_pSurfaceCoverMaterial || !m_surfaceCoverDirty ) return;
        auto& parameters = m_pSurfaceCoverMaterial->m_shaderParametersInstance;
        if ( !parameters.IsValid() )
        {
            parameters = m_pRenderSystem->CreateShaderParameters( m_pSurfaceCoverMaterial->m_shaderIndex );
            EE_ASSERT( parameters.m_parametersMemory.size() == m_surfaceCoverSourceParameters.size() );
            std::memcpy( parameters.m_parametersMemory.data(), m_surfaceCoverSourceParameters.data(), m_surfaceCoverSourceParameters.size() );
            m_surfaceCoverSourceParameters.clear();
        }
        auto copy = [this]( uint8_t* dst, size_t, uint32_t rowStride, uint32_t row )
        { std::memcpy( dst, m_surfaceCoverPixels.data() + size_t(row)*m_surfaceCoverWidth, Math::Min( size_t(rowStride), size_t(m_surfaceCoverWidth)*sizeof(Float4) ) ); };
        if ( !m_pSurfaceCoverTexture )
        {
            RHI::TextureParameters texture;
            texture.m_width = m_surfaceCoverWidth; texture.m_height = m_surfaceCoverHeight;
            texture.m_format = RHI::DataFormat::RGBA32_SFloat;
            texture.m_initialState = RHI::TextureState::Common;
            texture.m_debugName = "Provenance grass cover";
            m_pSurfaceCoverTexture = m_pRenderSystem->QueueTextureCreate( copy, texture );
            parameters.SetTexture( parameters.FindParameter( StringID("m_coverTexture") ), RHI::GetTextureHandle( m_pSurfaceCoverTexture, RHI::DescriptorTypeFlags::Texture, 0 ) );
            parameters.SetScalar( parameters.FindParameter( StringID("m_coverEnabled") ), 1.0f );
            m_pRenderSystem->QueueShaderParametersUpdate( parameters );
        }
        else
        {
            RHI::TextureCopyRegion region;region.m_width=m_surfaceCoverWidth;region.m_height=m_surfaceCoverHeight;
            m_pRenderSystem->QueueTextureUpdate( copy, m_pSurfaceCoverTexture, region, 1, 1, RHI::TextureState::Common );
        }
        m_surfaceCoverDirty=false;
    }

    void RenderWorldSystem::ReleaseSurfaceCoverMaterial()
    {
        if ( m_pSurfaceCoverTexture ) m_pRenderSystem->QueueResourceDelete( eastl::move(m_pSurfaceCoverTexture) );
        m_pSurfaceCoverTexture=nullptr;
        if ( m_pSurfaceCoverMaterial )
        {
            if ( m_pSurfaceCoverMaterial->m_shaderParametersInstance.IsValid() ) m_pRenderSystem->QueueResourceDelete( eastl::move(m_pSurfaceCoverMaterial->m_shaderParametersInstance) );
            EE::Delete( m_pSurfaceCoverMaterial );
            m_pSurfaceCoverMaterial=nullptr;
        }
        m_surfaceCoverPixels.clear();m_surfaceCoverSourceParameters.clear();m_surfaceCoverDirty=false;
        m_surfaceCoverWindPhase=0.0f;m_surfaceCoverWindDirty=false;
    }

    void RenderWorldSystem::CreateProceduralMeshDeviceResources
    (
        ProceduralMeshInstance& instance
    )
    {
        EE_ASSERT( instance.m_deviceCreatePending );

        auto CopyClusterVertices = [&geometry = instance.m_geometry]
        (
            uint8_t* pDstMemory_WriteCombined,
            size_t   dstSize
        )
        {
            Memory::CopyToWriteCombined
            (
                pDstMemory_WriteCombined,
                geometry.GetClusterVertices().data(),
                dstSize
            );
        };

        RHI::BufferParameters vertexBufferParameters = {};
        vertexBufferParameters.m_bufferSize =
            instance.m_geometry.GetNumClusterVertices() *
            instance.m_geometry.GetClusterVertexStride();
        vertexBufferParameters.m_descriptorTypes.SetMultipleFlags
        (
            RHI::DescriptorTypeFlags::Buffer,
            RHI::DescriptorTypeFlags::Raw
        );
        vertexBufferParameters.m_debugName = "Procedural Mesh Vertices";

        instance.m_pClusterVertexBuffer = m_pRenderSystem->QueueBufferCreate
        (
            CopyClusterVertices,
            vertexBufferParameters
        );

        auto CopyClusterTriangles = [&geometry = instance.m_geometry]
        (
            uint8_t* pDstMemory_WriteCombined,
            size_t   dstSize
        )
        {
            Memory::CopyToWriteCombined
            (
                pDstMemory_WriteCombined,
                geometry.GetClusterTriangles().data(),
                dstSize
            );
        };

        RHI::BufferParameters triangleBufferParameters = {};
        triangleBufferParameters.m_bufferSize =
            instance.m_geometry.GetNumClusterTriangles() *
            sizeof( uint32_t );
        triangleBufferParameters.m_bufferStride = sizeof( uint32_t );
        triangleBufferParameters.m_debugName = "Procedural Mesh Triangles";

        instance.m_pClusterTriangleBuffer = m_pRenderSystem->QueueBufferCreate
        (
            CopyClusterTriangles,
            triangleBufferParameters
        );

        MeshUpdate meshUpdate = m_pRenderSystem->CreateMesh
        (
            1,
            instance.m_geometry.GetNumClusters()
        );

        instance.m_meshHandle = meshUpdate.m_meshHandle;
        instance.m_clustersHandle = meshUpdate.m_clustersHandle;

        meshUpdate.m_deviceMeshes[0].m_clusterVertexBuffer = RHI::GetBufferHandle
        (
            instance.m_pClusterVertexBuffer,
            RHI::DescriptorTypeFlags::Buffer
        );

        meshUpdate.m_deviceMeshes[0].m_clusterTriangleBuffer = RHI::GetBufferHandle
        (
            instance.m_pClusterTriangleBuffer,
            RHI::DescriptorTypeFlags::Buffer
        );

        meshUpdate.m_deviceMeshes[0].m_numBones = 0;

        m_pRenderSystem->WriteCommonMeshData
        (
            meshUpdate,
            0,
            0,
            instance.m_geometry
        );

        m_pRenderSystem->QueueMeshUpdate
        (
            meshUpdate.m_meshHandle,
            meshUpdate.m_clustersHandle
        );

        AddProceduralMeshClusters( instance );

        instance.m_meshInstanceRootProxy =
            m_deviceRenderWorld.AllocateMeshInstanceRoot( 2 );

        instance.m_meshInstanceProxy =
            m_deviceRenderWorld.AllocateMeshInstance( 1 );

        Matrix43 const localTransform;
        instance.m_meshInstanceProxy.WriteLocalTransforms
        (
            TArrayView<Matrix43 const>( &localTransform, 1 )
        );

        instance.m_deviceCreatePending = false;
        instance.m_rootUploadPending = true;
    }

    void RenderWorldSystem::UploadProceduralMeshRootAndInitialize
    (
        ProceduralMeshInstance& instance
    )
    {
        EE_ASSERT( instance.m_rootUploadPending );

        TArray<uint32_t, 32> rootUpload = {};
        ShaderTypes::MeshInstanceRoot root = {};
        root.m_renderViewLayerFlags = instance.m_viewLayers;
        root.m_numLODs = 1;
        root.m_boneOffset = ~0U;
        root.m_firstInstance = uint32_t
        (
            instance.m_meshInstanceProxy.m_instanceHandle.m_offset
        );
        root.m_numInstances = 1;

        Matrix const rootMatrix = instance.m_worldTransform.ToMatrix();
        rootMatrix.GetRow( 0 ).StoreFloat3( root.m_rootTransform + 0 );
        rootMatrix.GetRow( 1 ).StoreFloat3( root.m_rootTransform + 3 );
        rootMatrix.GetRow( 2 ).StoreFloat3( root.m_rootTransform + 6 );
        rootMatrix.GetRow( 3 ).StoreFloat3( root.m_rootTransform + 9 );

        std::memcpy( rootUpload.data(), &root, sizeof( root ) );

        auto CopyRootData = [rootUpload]
        (
            uint8_t* pDstMemory_WriteCombined,
            size_t   dstSize
        )
        {
            EE_ASSERT( dstSize == rootUpload.size() * sizeof( uint32_t ) );
            // Lambda captures (including copies made by QueueBufferUpdate) do
            // not inherit a variable's alignment. Stage the tiny root payload
            // in aligned storage inside the callback, at the point of use.
            alignas( 32 ) uint32_t alignedRootUpload[32];
            std::memcpy( alignedRootUpload, rootUpload.data(), sizeof( alignedRootUpload ) );
            Memory::CopyToWriteCombined
            (
                pDstMemory_WriteCombined,
                alignedRootUpload,
                dstSize
            );
        };

        m_pRenderSystem->QueueBufferUpdate
        (
            CopyRootData,
            m_deviceRenderWorld.GetMeshInstanceRootBuffer(),
            instance.m_meshInstanceRootProxy.m_instanceHandle.m_offset *
                sizeof( ShaderTypes::MeshInstanceRoot ),
            instance.m_meshInstanceRootProxy.m_instanceHandle.m_size *
                sizeof( ShaderTypes::MeshInstanceRoot )
        );

        m_deviceRenderWorld.QueueProceduralMeshInstanceInitialize
        (
            uint32_t( instance.m_meshInstanceProxy.m_instanceHandle.m_offset ),
            uint32_t( instance.m_meshInstanceRootProxy.m_instanceHandle.m_offset ),
            instance.m_meshHandle,
            instance.m_geometry,
            instance.m_pClusterVertexBuffer,
            instance.m_pClusterTriangleBuffer,
            instance.m_pMaterial
        );

        instance.m_rootUploadPending = false;
    }

    void RenderWorldSystem::UnregisterProceduralMesh( ProceduralMeshID meshID )
    {
        if ( meshID == 0 )
        {
            return;
        }

        for ( size_t instanceIndex = 0;
              instanceIndex < m_proceduralMeshInstances.size();
              ++instanceIndex )
        {
            ProceduralMeshInstance& instance =
                m_proceduralMeshInstances[instanceIndex];

            if ( instance.m_ID != meshID )
            {
                continue;
            }

            if ( !instance.m_deviceCreatePending )
            {
                RemoveProceduralMeshClusters( instance );

                m_deviceRenderWorld.DeallocateMeshInstance
                (
                    eastl::move( instance.m_meshInstanceProxy )
                );

                m_deviceRenderWorld.DeallocateMeshInstanceRoot
                (
                    eastl::move( instance.m_meshInstanceRootProxy )
                );

                m_pRenderSystem->QueueResourceDelete
                (
                    eastl::move( instance.m_pClusterVertexBuffer ),
                    eastl::move( instance.m_pClusterTriangleBuffer ),
                    TPair
                    {
                        eastl::move( instance.m_meshHandle ),
                        eastl::move( instance.m_clustersHandle )
                    }
                );
            }

            m_proceduralMeshInstances.erase
            (
                m_proceduralMeshInstances.begin() + instanceIndex
            );
            return;
        }
    }

    void RenderWorldSystem::AddProceduralMeshClusters
    (
        ProceduralMeshInstance const& instance
    )
    {
        int32_t const shaderIndex = instance.m_pMaterial->GetShaderIndex();
        EE_ASSERT( shaderIndex != -1 );

        uint32_t const clusterCount = instance.m_geometry.GetNumClusters();
        m_materialShaderClusterCapacity.AddGlobalClusters( clusterCount );

        ForEachViewLayer
        (
            [this, &instance, shaderIndex, clusterCount]
            ( ViewLayer viewLayer )
            {
                if ( instance.m_viewLayers.IsFlagSet( viewLayer ) )
                {
                    m_materialShaderClusterCapacity.AddViewLayerClusters
                    (
                        uint32_t( viewLayer ),
                        shaderIndex,
                        clusterCount
                    );
                }
            }
        );
    }

    void RenderWorldSystem::RemoveProceduralMeshClusters
    (
        ProceduralMeshInstance const& instance
    )
    {
        int32_t const shaderIndex = instance.m_pMaterial->GetShaderIndex();
        EE_ASSERT( shaderIndex != -1 );

        uint32_t const clusterCount = instance.m_geometry.GetNumClusters();
        m_materialShaderClusterCapacity.RemoveGlobalClusters( clusterCount );

        ForEachViewLayer
        (
            [this, &instance, shaderIndex, clusterCount]
            ( ViewLayer viewLayer )
            {
                if ( instance.m_viewLayers.IsFlagSet( viewLayer ) )
                {
                    m_materialShaderClusterCapacity.RemoveViewLayerClusters
                    (
                        uint32_t( viewLayer ),
                        shaderIndex,
                        clusterCount
                    );
                }
            }
        );
    }

    void RenderWorldSystem::ShutdownSystem()
    {
        EE_ASSERT( m_proceduralMeshInstances.empty() );
        ReleaseSurfaceCoverMaterial();
        m_pRenderSystem->WaitAllQueuesIdle();

        m_materialShaderClusterCapacity.Shutdown();

        EE_ASSERT( m_numShadowCastingDirectionalLights == 0 );

        m_deviceRenderWorld.Shutdown( m_pRenderSystem );

        RHI::DestroyTexture( m_pRenderSystem->GetContextRHI(), eastl::move( m_pRadianceTexture ) );
        RHI::DestroyTexture( m_pRenderSystem->GetContextRHI(), eastl::move( m_pIrradianceTexture ) );

        m_pRenderSystem = nullptr;
        m_pTaskSystem = nullptr;
    }

    void RenderWorldSystem::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        // Meshes
        //-------------------------------------------------------------------------

        if ( StaticMeshComponent* pStaticMeshComponent = TryCast<StaticMeshComponent>( pComponent ) )
        {
            if ( pStaticMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( !pStaticMeshComponent->m_meshInstanceProxy.m_instanceHandle.IsValid() );

                Mesh const* pStaticMesh = pStaticMeshComponent->GetMesh();

                AddMeshClusters( pStaticMesh, pStaticMeshComponent->m_materialOverrides, pStaticMeshComponent->m_viewLayers );

                uint32_t instanceDataSizeInBytes = pStaticMeshComponent->ComputeInstanceDataSizeInBytes();

                pStaticMeshComponent->m_meshInstanceRootProxy = m_deviceRenderWorld.AllocateMeshInstanceRoot( ( instanceDataSizeInBytes + 63 ) / 64 );
                pStaticMeshComponent->m_meshInstanceProxy = m_deviceRenderWorld.AllocateMeshInstance( pStaticMesh->GetNumSubmeshes() );

                pStaticMeshComponent->QueueInitializeMeshInstance( &m_deviceRenderWorld );

                if ( pStaticMeshComponent->m_viewLayers.IsFlagSet( ViewLayer::GlobalEnvironmentMap ) )
                {
                    m_needUpdateGlobalEnvironmentMap = true;
                }

                m_staticMeshComponents.Add( pStaticMeshComponent );
                m_staticMeshComponentInstanceUpdateQueue.Bind( pStaticMeshComponent, pStaticMeshComponent->GetInstanceDataUpdateSignal() );

                if ( instanceDataSizeInBytes )
                {
                    pStaticMeshComponent->GetInstanceDataUpdateSignal()->Send( pStaticMeshComponent );
                }
            }
        }
        else if ( SkeletalMeshComponent* pSkeletalMeshComponent = TryCast<SkeletalMeshComponent>( pComponent ) )
        {
            if ( pSkeletalMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( !pSkeletalMeshComponent->m_meshInstanceProxy.m_instanceHandle.IsValid() );
                EE_ASSERT( !pSkeletalMeshComponent->m_skinningProxy.IsValid() );

                SkeletalMesh const* pSkeletalMesh = pSkeletalMeshComponent->GetMesh();

                AddMeshClusters( pSkeletalMesh, pSkeletalMeshComponent->m_materialOverrides, pSkeletalMeshComponent->m_viewLayers );

                uint32_t instanceDataSizeInBytes = pSkeletalMeshComponent->ComputeInstanceDataSizeInBytes();

                pSkeletalMeshComponent->m_skinningProxy = m_deviceRenderWorld.AllocateSkinningInstance( pSkeletalMesh->GetNumBones() );

                pSkeletalMeshComponent->m_meshInstanceRootProxy = m_deviceRenderWorld.AllocateMeshInstanceRoot( ( instanceDataSizeInBytes + 63 ) / 64 );
                pSkeletalMeshComponent->m_meshInstanceProxy = m_deviceRenderWorld.AllocateMeshInstance( pSkeletalMesh->GetNumSubmeshes() );

                pSkeletalMeshComponent->QueueInitializeMeshInstance( &m_deviceRenderWorld );
                pSkeletalMeshComponent->UpdateSkinningProxy();

                if ( pSkeletalMeshComponent->m_viewLayers.IsFlagSet( ViewLayer::GlobalEnvironmentMap ) )
                {
                    m_needUpdateGlobalEnvironmentMap = true;
                }

                m_skeletalMeshComponents.Add( pSkeletalMeshComponent );
                m_skeletalMeshComponentInstanceUpdateQueue.Bind( pSkeletalMeshComponent, pSkeletalMeshComponent->GetInstanceDataUpdateSignal() );

                if ( instanceDataSizeInBytes )
                {
                    pSkeletalMeshComponent->GetInstanceDataUpdateSignal()->Send( pSkeletalMeshComponent );
                }
            }
        }

        // Lights
        //-------------------------------------------------------------------------

        else if ( auto pLightComponent = TryCast<LightComponent>( pComponent ) )
        {
            if ( auto pDirectionalLightComponent = TryCast<DirectionalLightComponent>( pComponent ) )
            {
                if ( pDirectionalLightComponent->GetShadowed() )
                {
                    pDirectionalLightComponent->m_cascadedShadowIndex = uint16_t( m_numShadowCastingDirectionalLights );
                    m_numShadowCastingDirectionalLights++;
                }

                pDirectionalLightComponent->m_lightInstanceProxy = m_deviceRenderWorld.AllocateDirectionalLight();
                pDirectionalLightComponent->OnWorldTransformUpdated();

                m_directionalLightComponents.Add( pDirectionalLightComponent );
            }
            else if ( auto pPointLightComponent = TryCast<PointLightComponent>( pComponent ) )
            {
                pPointLightComponent->m_lightInstanceProxy = m_deviceRenderWorld.AllocatePointLight();
                pPointLightComponent->OnWorldTransformUpdated();

                m_pointLightComponents.Add( pPointLightComponent );
            }
            else if ( auto pSpotLightComponent = TryCast<SpotLightComponent>( pComponent ) )
            {
                pSpotLightComponent->m_lightInstanceProxy = m_deviceRenderWorld.AllocateSpotLight();
                pSpotLightComponent->OnWorldTransformUpdated();

                m_spotLightComponents.Add( pSpotLightComponent );
            }
        }

        // Environment Maps
        //-------------------------------------------------------------------------

        else if ( auto pLocalEnvMapComponent = TryCast<LocalEnvironmentMapComponent>( pComponent ) )
        {
        }
    }

    void RenderWorldSystem::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        // Meshes
        //-------------------------------------------------------------------------

        if ( StaticMeshComponent* pStaticMeshComponent = TryCast<StaticMeshComponent>( pComponent ) )
        {
            if ( pStaticMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( pStaticMeshComponent->m_meshInstanceProxy.m_instanceHandle.IsValid() );

                RemoveMeshClusters( pStaticMeshComponent->GetMesh(), pStaticMeshComponent->m_materialOverrides, pStaticMeshComponent->m_viewLayers );

                m_staticMeshComponentInstanceUpdateQueue.Unbind( pStaticMeshComponent, pStaticMeshComponent->GetInstanceDataUpdateSignal() );

                m_staticMeshComponents.Remove( pStaticMeshComponent->GetID() );
                m_deviceRenderWorld.DeallocateMeshInstance( eastl::move( pStaticMeshComponent->m_meshInstanceProxy ) );
                m_deviceRenderWorld.DeallocateMeshInstanceRoot( eastl::move( pStaticMeshComponent->m_meshInstanceRootProxy ) );

                if ( pStaticMeshComponent->m_viewLayers.IsFlagSet( ViewLayer::GlobalEnvironmentMap ) )
                {
                    m_needUpdateGlobalEnvironmentMap = true;
                }
            }
        }
        else if ( SkeletalMeshComponent* pSkeletalMeshComponent = TryCast<SkeletalMeshComponent>( pComponent ) )
        {
            if ( pSkeletalMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( pSkeletalMeshComponent->m_meshInstanceProxy.IsValid() );
                EE_ASSERT( pSkeletalMeshComponent->m_skinningProxy.IsValid() );

                RemoveMeshClusters( pSkeletalMeshComponent->GetMesh(), pSkeletalMeshComponent->m_materialOverrides, pSkeletalMeshComponent->m_viewLayers );

                m_skeletalMeshComponentInstanceUpdateQueue.Unbind( pSkeletalMeshComponent, pSkeletalMeshComponent->GetInstanceDataUpdateSignal() );

                m_skeletalMeshComponents.Remove( pSkeletalMeshComponent->GetID() );
                m_deviceRenderWorld.DeallocateSkinningInstance( eastl::move( pSkeletalMeshComponent->m_skinningProxy ) );
                m_deviceRenderWorld.DeallocateMeshInstance( eastl::move( pSkeletalMeshComponent->m_meshInstanceProxy ) );
                m_deviceRenderWorld.DeallocateMeshInstanceRoot( eastl::move( pSkeletalMeshComponent->m_meshInstanceRootProxy ) );

                if ( pSkeletalMeshComponent->m_viewLayers.IsFlagSet( ViewLayer::GlobalEnvironmentMap ) )
                {
                    m_needUpdateGlobalEnvironmentMap = true;
                }
            }
        }

        // Lights
        //-------------------------------------------------------------------------

        else if ( auto pLightComponent = TryCast<LightComponent>( pComponent ) )
        {
            if ( auto pDirectionalLightComponent = TryCast<DirectionalLightComponent>( pComponent ) )
            {
                if ( pDirectionalLightComponent->GetShadowed() )
                {
                    m_numShadowCastingDirectionalLights--;
                }

                m_deviceRenderWorld.DeallocateDirectionalLight( eastl::move( pDirectionalLightComponent->m_lightInstanceProxy ) );

                m_directionalLightComponents.Remove( pDirectionalLightComponent->GetID() );
            }
            else if ( auto pPointLightComponent = TryCast<PointLightComponent>( pComponent ) )
            {
                m_deviceRenderWorld.DeallocatePointLight( eastl::move( pPointLightComponent->m_lightInstanceProxy ) );

                m_pointLightComponents.Remove( pPointLightComponent->GetID() );
            }
            else if ( auto pSpotLightComponent = TryCast<SpotLightComponent>( pComponent ) )
            {
                m_deviceRenderWorld.DeallocateSpotLight( eastl::move( pSpotLightComponent->m_lightInstanceProxy ) );

                m_spotLightComponents.Remove( pSpotLightComponent->GetID() );
            }
        }

        // Environment Maps
        //-------------------------------------------------------------------------

        else if ( auto pLocalEnvMapComponent = TryCast<LocalEnvironmentMapComponent>( pComponent ) )
        {
            // Do nothing
        }
    }

    void RenderWorldSystem::AddMeshClusters( Mesh const* pMeshResource, TArrayView<TResourcePtr<Material> const> materialOverrides, TBitFlags<ViewLayer> viewLayers )
    {
        int32_t const numSubmeshes = pMeshResource->GetNumSubmeshes();
        for ( int32_t submeshIdx = 0; submeshIdx < numSubmeshes; ++submeshIdx )
        {
            uint32_t const geometryIdx = pMeshResource->GetSubmeshGeometryIndex( submeshIdx );

            // Resolve material override
            Material const* pMaterial = pMeshResource->GetMaterial( submeshIdx );
            if ( submeshIdx < materialOverrides.size() )
            {
                TResourcePtr<Material> const& overrideMaterial = materialOverrides[submeshIdx];
                if ( overrideMaterial.IsLoaded() )
                {
                    EE_ASSERT( overrideMaterial->IsValid() );
                    pMaterial = overrideMaterial.GetPtr();
                }
            }

            if ( pMaterial == nullptr )
            {
                pMaterial = m_pRenderSystem->GetPlaceholderMaterial();
            }

            EE_ASSERT( pMaterial != nullptr );

            int32_t shaderIndex = pMaterial->GetShaderIndex();
            EE_ASSERT( shaderIndex != -1 );

            Geometry const& geometry = pMeshResource->GetGeometry()[geometryIdx];
            uint32_t const requiredClusterCapacity = geometry.GetNumClusters();

            m_materialShaderClusterCapacity.AddGlobalClusters( requiredClusterCapacity );
            ForEachViewLayer( [this, viewLayers, &shaderIndex, requiredClusterCapacity] ( ViewLayer viewLayer )
            {
                if ( viewLayers.IsFlagSet( viewLayer ) )
                {
                    m_materialShaderClusterCapacity.AddViewLayerClusters( uint32_t( viewLayer ), shaderIndex, requiredClusterCapacity );
                }
            } );
        }
    }

    void RenderWorldSystem::RemoveMeshClusters( Mesh const* pMeshResource, TArrayView<TResourcePtr<Material> const> materialOverrides, TBitFlags<ViewLayer> viewLayers )
    {
        int32_t const numSubmeshes = pMeshResource->GetNumSubmeshes();
        for ( int32_t submeshIdx = 0; submeshIdx < pMeshResource->GetNumSubmeshes(); ++submeshIdx )
        {
            uint32_t const geometryIdx = pMeshResource->GetSubmeshGeometryIndex( submeshIdx );

            // Resolve material override
            Material const* pMaterial = pMeshResource->GetMaterial( submeshIdx );
            if ( submeshIdx < materialOverrides.size() )
            {
                TResourcePtr<Material> const& overrideMaterial = materialOverrides[submeshIdx];
                if ( overrideMaterial.IsLoaded() )
                {
                    EE_ASSERT( overrideMaterial->IsValid() );
                    pMaterial = overrideMaterial.GetPtr();
                }
            }

            if ( pMaterial == nullptr )
            {
                pMaterial = m_pRenderSystem->GetPlaceholderMaterial();
            }

            EE_ASSERT( pMaterial != nullptr );

            int32_t shaderIndex = pMaterial->GetShaderIndex();
            EE_ASSERT( shaderIndex != -1 );

            Geometry const& geometry = pMeshResource->GetGeometry()[geometryIdx];
            uint32_t const requiredClusterCapacity = geometry.GetNumClusters();

            m_materialShaderClusterCapacity.RemoveGlobalClusters( requiredClusterCapacity );
            ForEachViewLayer( [this, viewLayers, &shaderIndex, requiredClusterCapacity] ( ViewLayer viewLayer )
            {
                if ( viewLayers.IsFlagSet( viewLayer ) )
                {
                    m_materialShaderClusterCapacity.RemoveViewLayerClusters( uint32_t( viewLayer ), shaderIndex, requiredClusterCapacity );
                }
            } );
        }
    }

    void RenderWorldSystem::UpdateDeviceResources()
    {
        EE_PROFILE_FUNCTION_RENDER();
        UpdateSurfaceCoverResources();
        UpdateSurfaceCoverWindResources();

        // Procedural requests can arrive from world/debug updates, outside the
        // render system's resource-update stage. Defer all GPU allocation to
        // this stage, before DeviceRenderWorld sizes its instance buffers.
        for ( ProceduralMeshInstance& instance : m_proceduralMeshInstances )
        {
            if ( instance.m_deviceCreatePending )
            {
                CreateProceduralMeshDeviceResources( instance );
            }
            // Submit rigid transforms before DeviceRenderWorld snapshots the
            // update-command counts and schedules their uploads.
            if ( !instance.m_rootUploadPending && instance.m_transformUpdatePending )
            {
                instance.m_meshInstanceRootProxy.WriteRootTransform( instance.m_worldTransform, Float3::One );
                instance.m_transformUpdatePending = false;
            }
        }

        m_deviceRenderWorld.UpdateDeviceResources_BeforeInstanceInitialize( m_pRenderSystem );

        for ( ProceduralMeshInstance& instance : m_proceduralMeshInstances )
        {
            if ( instance.m_rootUploadPending )
            {
                UploadProceduralMeshRootAndInitialize( instance );
                instance.m_transformUpdatePending = false;
            }
        }

        // InstanceUpdate StaticMesh
        //---------------------------------------------------------------------------------------------------
        {
            TEntityMessageQueue<StaticMeshComponent>::Message staticMeshComponentMessage = {};
            while ( m_staticMeshComponentInstanceUpdateQueue.Dequeue( staticMeshComponentMessage ) )
            {
                StaticMeshComponent* pStaticMeshComponent = staticMeshComponentMessage.m_pComponent;
                EE_ASSERT( pStaticMeshComponent );

                auto CopyBufferMemory = [pStaticMeshComponent] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
                {
                    uint32_t instanceDataSizeInBytes = pStaticMeshComponent->ComputeInstanceDataSizeInBytes();
                    EE_ASSERT( pStaticMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_size == ( ( instanceDataSizeInBytes + 63 ) / 64 ) );
                    EE_ASSERT( ( ( instanceDataSizeInBytes + 63 ) / 64 ) * 64 == dstSize );

                    pStaticMeshComponent->WriteInstanceData( { reinterpret_cast<uint32_t*>( pDstMemory_WriteCombined ), instanceDataSizeInBytes / sizeof( uint32_t ) } );
                };

                m_pRenderSystem->QueueBufferUpdate
                (
                    CopyBufferMemory,
                    m_deviceRenderWorld.GetMeshInstanceRootBuffer(),
                    pStaticMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_offset * sizeof( ShaderTypes::MeshInstanceRoot ),
                    pStaticMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_size * sizeof( ShaderTypes::MeshInstanceRoot )
                );
                pStaticMeshComponent->QueueInitializeMeshInstance( &m_deviceRenderWorld );
            }

            m_staticMeshComponentInstanceUpdateQueue.ClearIgnoredComponents();
        }

        // InstanceUpdate SkeletalMesh
        //---------------------------------------------------------------------------------------------------
        {
            TEntityMessageQueue<SkeletalMeshComponent>::Message skeletalMeshComponentMessage = {};
            while ( m_skeletalMeshComponentInstanceUpdateQueue.Dequeue( skeletalMeshComponentMessage ) )
            {
                SkeletalMeshComponent* pSkeletalMeshComponent = skeletalMeshComponentMessage.m_pComponent;
                EE_ASSERT( pSkeletalMeshComponent );

                auto CopyBufferMemory = [pSkeletalMeshComponent] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
                {
                    uint32_t instanceDataSizeInBytes = pSkeletalMeshComponent->ComputeInstanceDataSizeInBytes();
                    EE_ASSERT( pSkeletalMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_size == ( ( instanceDataSizeInBytes + 63 ) / 64 ) );
                    EE_ASSERT( ( ( instanceDataSizeInBytes + 63 ) / 64 ) * 64 == dstSize );

                    pSkeletalMeshComponent->WriteInstanceData( { reinterpret_cast<uint32_t*>( pDstMemory_WriteCombined ), instanceDataSizeInBytes / sizeof( uint32_t ) } );
                };

                m_pRenderSystem->QueueBufferUpdate
                (
                    CopyBufferMemory,
                    m_deviceRenderWorld.GetMeshInstanceRootBuffer(),
                    pSkeletalMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_offset * sizeof( ShaderTypes::MeshInstanceRoot ),
                    pSkeletalMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_size * sizeof( ShaderTypes::MeshInstanceRoot )
                );
                pSkeletalMeshComponent->QueueInitializeMeshInstance( &m_deviceRenderWorld );
            }

            m_skeletalMeshComponentInstanceUpdateQueue.ClearIgnoredComponents();
        }

        m_deviceRenderWorld.UpdateDeviceResources_AfterInstanceInitialize( m_pRenderSystem );
    }

    RHI::TextureHandle RenderWorldSystem::GetRadianceTextureHandle() const
    {
        return RHI::GetTextureHandle( m_pRadianceTexture, RHI::DescriptorTypeFlags::TextureCube, 0 );
    }

    float RenderWorldSystem::GetRadianceTextureMipLevels() const
    {
        return float( m_pRadianceTexture->m_mipLevels );
    }

    RHI::TextureHandle RenderWorldSystem::GetIrradianceTextureHandle() const
    {
        return RHI::GetTextureHandle( m_pIrradianceTexture, RHI::DescriptorTypeFlags::TextureCube, 0 );
    }
}
