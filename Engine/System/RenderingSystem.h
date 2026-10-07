#pragma once

#include <vector>

#include "Common/Common.h"
#include "Math/Matrix4.h"
#include "Lighting/RenderData/DirectionalLightRenderData.h"

namespace Engine
{
    class IRenderer;
    class LightingSystem;
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

        void Render(const Matrix4& viewMatrix, const Matrix4& projectionMatrix, const LightingSystem* lightingSystem);
        
    private:
        IRenderer& renderer;

        std::vector<MeshRendererComponent*> meshRenderers;

        std::vector<DirectionalLightRenderData> directionalLights;
    };
}
