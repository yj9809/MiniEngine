#include "RenderingSystem.h"

#include <algorithm>

#include "System/LightingSystem.h"
#include "Component/Mesh/MeshRendererComponent.h"
#include "Renderer/IRenderer.h"
#include "Renderer/RenderCommand.h"

namespace Engine
{
    RenderingSystem::RenderingSystem(IRenderer& renderer)
        : renderer(renderer)
    {
    }

    void RenderingSystem::Register(MeshRendererComponent* component)
    {
        if (!component)
        {
            return;
        }
        
        const auto found = std::find(
            meshRenderers.begin(),
            meshRenderers.end(),
            component
        );
        
        if (found == meshRenderers.end())
        {
            meshRenderers.push_back(component);
        }
    }

    void RenderingSystem::Unregister(MeshRendererComponent* component)
    {
        const auto found = std::find(
            meshRenderers.begin(),
            meshRenderers.end(),
            component
        );

        if (found != meshRenderers.end())
        {
            meshRenderers.erase(found);
        }
    }

    void RenderingSystem::Clear()
    {
        meshRenderers.clear();
    }

    void RenderingSystem::Render(const Matrix4& viewMatrix, const Matrix4& projectionMatrix, const LightingSystem* lightingSystem)
    {
        directionalLights.clear();
        EnvironmentLightingRenderData environmentLighting = {};

        // LightingSystem에서 DirectionalLightRenderData를 가져와서 RenderingSystem에 전달.
        // 환경광이 추가됨에 따라 EnvironmentLightingRenderData도 가져와서 전달.
        if(lightingSystem != nullptr)
        {
            lightingSystem->BuildDirectionalLightRenderData(directionalLights);
            environmentLighting = lightingSystem->BuildEnvironmentLightingRenderData();
        }

        renderer.SubmitDirectionalLights(directionalLights);
        renderer.SubmitEnvironmentLighting(environmentLighting);

        for(MeshRendererComponent* component : meshRenderers)
        {
            if(!component)
            {
                continue;
            }
            
            RenderCommand command;

            if(component->BuildRenderCommand(viewMatrix, projectionMatrix, command))
            {
                renderer.Submit(command);
            }
        }
    }
}
