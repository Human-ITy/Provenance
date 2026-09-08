#pragma once

#include "Engine/Entity/EntityWorldSystem.h"
#include "Engine/Entity/EntityWorldSystemSignal.h"

#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/RenderGeometry.h"
#include "Engine/Render/RenderMaterialShaderClusterCapacity.h"
#include "Engine/Viewport/ViewportPicking.h"

#include "Base/Resource/ResourcePtr.h"
#include "Base/Render/RHI.h"
#include "Base/Math/Transform.h"
#include "Base/Systems.h"
#include "Base/Types/IDVector.h"

//-----------------------------------------------------------------------------------------------------------

namespace EE::Render
{
    class StaticMeshComponent;
    class SkeletalMeshComponent;
    class PCGComponent;
    class DirectionalLightComponent;
    class PointLightComponent;
    class SpotLightComponent;
    class GlobalEnvironmentMapComponent;
    class LocalEnvironmentMapComponent;
    class Mesh;
    class Material;
    class RenderSystem;
    class RenderViewport;

    struct ProceduralMeshVertex final
    {
        Float3 m_position = Float3::Zero;
        Float3 m_normal = Float3::UnitZ;
        Float2 m_uv0 = Float2::Zero;
        // Optional material payloads; defaults preserve existing procedural meshes.
        Float2 m_uv1 = Float2::Zero;
        uint32_t m_color = 0;
    };

    using ProceduralMeshID = uint64_t;

    //-------------------------------------------------------------------------------------------------------

    class EE_ENGINE_API RenderWorldSystem final : public EntityWorldSystem
    {
        friend class ForwardShadingRenderer;
        friend class RenderDebugView;

    public:

        EE_ENTITY_WORLD_SYSTEM( RenderWorldSystem );

        void UpdateViewportPickingData( RenderViewport* pViewport ) const;

        // Registers immutable runtime-generated triangles with the ordinary
        // clustered/PBR renderer. This is intentionally separate from
        // DebugDraw so certification rendering can remain unchanged.
        ProceduralMeshID RegisterProceduralMesh(
            TArrayView<ProceduralMeshVertex const> vertices,
            TArrayView<uint32_t const>             indices,
            Material const*                        pMaterial,
            Transform const&                       worldTransform,
            TBitFlags<ViewLayer>                   viewLayers,
            bool                                  fastBuild = false,
            float                                 vertexDisplacementRadius = 0.0f );

        void UnregisterProceduralMesh( ProceduralMeshID meshID );

        // Rigid motion does not change immutable mesh geometry or GPU buffers.
        bool UpdateProceduralMeshTransform( ProceduralMeshID meshID, Transform const& worldTransform );

        // World-owned clone: never modify a shared material resource's values.
        Material const* CreateSurfaceCoverMaterial( Material const* pSource );
        void SetSurfaceCoverData( uint32_t width, uint32_t height, TArrayView<Float4 const> pixels );
        // Presentation only; does not rebuild cover textures or mesh geometry.
        void SetSurfaceCoverWindPhase( float phase );
        void ReleaseSurfaceCoverMaterial(); // Unregister its meshes first.

    private:

        // Entity System
        //---------------------------------------------------------------------------------------------------

        virtual void InitializeSystem( SystemRegistry const& systemRegistry ) override final;
        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;

        //-------------------------------------------------------------------------

        void AddMeshClusters( Mesh const* pMeshResource, TArrayView<TResourcePtr<Material> const> materialOverrides, TBitFlags<ViewLayer> viewLayers );
        void RemoveMeshClusters( Mesh const* pMeshResource, TArrayView<TResourcePtr<Material> const> materialOverrides, TBitFlags<ViewLayer> viewLayers );

        //-------------------------------------------------------------------------

        void UpdateDeviceResources();
        void UpdateSurfaceCoverResources();
        void UpdateSurfaceCoverWindResources();

        Material* m_pSurfaceCoverMaterial = nullptr;
        RHI::Texture* m_pSurfaceCoverTexture = nullptr;
        TVector<uint8_t> m_surfaceCoverSourceParameters;
        TVector<Float4> m_surfaceCoverPixels;
        uint32_t m_surfaceCoverWidth = 0, m_surfaceCoverHeight = 0;
        bool m_surfaceCoverDirty = false;
        float m_surfaceCoverWindPhase = 0.0f;
        bool m_surfaceCoverWindDirty = false;

        RHI::TextureHandle GetRadianceTextureHandle() const;
        float GetRadianceTextureMipLevels() const;
        RHI::TextureHandle GetIrradianceTextureHandle() const;

    private:

        struct ProceduralMeshInstance final
        {
            ProceduralMeshID m_ID = 0;
            Geometry m_geometry;
            RHI::Buffer* m_pClusterVertexBuffer = nullptr;
            RHI::Buffer* m_pClusterTriangleBuffer = nullptr;
            MeshHandle m_meshHandle = {};
            ClustersHandle m_clustersHandle = {};
            MeshInstanceProxy m_meshInstanceRootProxy = {};
            MeshInstanceProxy m_meshInstanceProxy = {};
            Material const* m_pMaterial = nullptr;
            TBitFlags<ViewLayer> m_viewLayers;
            Transform m_worldTransform = Transform::Identity;
            bool m_deviceCreatePending = true;
            bool m_rootUploadPending = false;
            bool m_transformUpdatePending = false;
        };

        void AddProceduralMeshClusters( ProceduralMeshInstance const& instance );
        void RemoveProceduralMeshClusters( ProceduralMeshInstance const& instance );
        void CreateProceduralMeshDeviceResources( ProceduralMeshInstance& instance );
        void UploadProceduralMeshRootAndInitialize( ProceduralMeshInstance& instance );

    private:

        //---------------------------------------------------------------------------------------------------

        TaskSystem*                                                         m_pTaskSystem = nullptr;
        RenderSystem*                                                       m_pRenderSystem = nullptr;

        DeviceRenderWorld                                                   m_deviceRenderWorld;

        TIDVector<ComponentID, StaticMeshComponent const*>                  m_staticMeshComponents;
        TIDVector<ComponentID, SkeletalMeshComponent const*>                m_skeletalMeshComponents;
        TIDVector<ComponentID, PCGComponent const*>                         m_pcgComponents;
        TIDVector<ComponentID, DirectionalLightComponent const*>            m_directionalLightComponents;
        TIDVector<ComponentID, PointLightComponent const*>                  m_pointLightComponents;
        TIDVector<ComponentID, SpotLightComponent const*>                   m_spotLightComponents;

        TEntityMessageQueue<StaticMeshComponent>                            m_staticMeshComponentInstanceUpdateQueue;
        TEntityMessageQueue<SkeletalMeshComponent>                          m_skeletalMeshComponentInstanceUpdateQueue;

        TVector<ProceduralMeshInstance>                                     m_proceduralMeshInstances;
        ProceduralMeshID                                                    m_nextProceduralMeshID = 1;

        uint32_t                                                            m_numShadowCastingDirectionalLights = 0;

        MaterialShaderClusterCapacity                                       m_materialShaderClusterCapacity;

        bool                                                                m_needUpdateGlobalEnvironmentMap = true;
        RHI::Texture*                                                       m_pRadianceTexture = nullptr;
        RHI::Texture*                                                       m_pIrradianceTexture = nullptr;
    };
}
