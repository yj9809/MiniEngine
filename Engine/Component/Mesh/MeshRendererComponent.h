#pragma once

#include <memory>
#include <vector>

#include "Common/Common.h"
#include "Component/Component.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/RenderLayer/RenderLayerType.h"

namespace Engine
{
    class Mesh;
    class Texture;
    class IRenderer;
    class Material;
    
    class ENGINE_API MeshRendererComponent : public Component
    {
        RTTI_DECLARATIONS(MeshRendererComponent, Component)
        
    public:
        void SetMesh(std::shared_ptr<Mesh> newMesh);

        void SetMaterial(size_t slot, std::shared_ptr<Material> newMaterial);
        
        void SetLayerType(RenderLayerType newLayerType);
        
        void Initialize(IRenderer* renderer);
        
        virtual void Draw() override;
        
    private:
        std::shared_ptr<Mesh> mesh;
        
        std::vector<std::shared_ptr<Material>> materials;
        
        RenderLayerType layerType = RenderLayerType::Opaque;
        
        IRenderer* renderer = nullptr;
        
        BufferHandle vertexBuffer = NULL_BUFFER;
        
        BufferHandle indexBuffer = NULL_BUFFER;
        
        UINT indexCount = 0;
        
        UINT stride = 0;
        
        RenderCommand renderCommand;
    };
}
