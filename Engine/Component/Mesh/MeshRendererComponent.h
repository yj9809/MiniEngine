#pragma once

#include <memory>

#include "Common/Common.h"
#include "Component/Component.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/RenderLayer/RenderLayerType.h"

namespace Engine
{
    class Mesh;
    class IRenderer;
    
    class ENGINE_API MeshRendererComponent : public Component
    {
        RTTI_DECLARATIONS(MeshRendererComponent, Component)
        
    public:
        void SetMesh(std::shared_ptr<Mesh> newMesh);
        
        void SetLayerType(RenderLayerType newLayerType);
        
        void Initialize(IRenderer* renderer);
        
        virtual void Draw() override;
        
    private:
        std::shared_ptr<Mesh> mesh;
        
        RenderLayerType layerType = RenderLayerType::Opaque;
        
        IRenderer* renderer = nullptr;
        
        BufferHandle vertexBuffer = NULL_BUFFER;
        
        BufferHandle indexBuffer = NULL_BUFFER;
        
        UINT indexCount = 0;
        
        UINT stride = 0;
        
        RenderCommand renderCommand;
    };
}
