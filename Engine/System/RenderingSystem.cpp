#include "RenderingSystem.h"

#include <algorithm>

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

    void RenderingSystem::Render(const Matrix4& viewMatrix, const Matrix4& projectionMatrix)
    {
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
