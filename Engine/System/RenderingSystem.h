#pragma once

#include <vector>

#include "Common/Common.h"
#include "Math/Matrix4.h"

namespace Engine
{
    class IRenderer;
    class MeshRendererComponent;
    
    class ENGINE_API RenderingSystem
    {
    public:
        explicit RenderingSystem(IRenderer& renderer);
        
        RenderingSystem(const RenderingSystem&) = delete;
        RenderingSystem& operator=(const RenderingSystem&) = delete;
        
        void Register(MeshRendererComponent* component);
        void Unregister(MeshRendererComponent* component);
        void Clear();

        void Render(const Matrix4& viewMatrix, const Matrix4& projectionMatrix);
        
    private:
        IRenderer& renderer;
        std::vector<MeshRendererComponent*> meshRenderers;
    };
}
