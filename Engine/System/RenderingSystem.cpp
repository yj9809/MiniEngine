#include "RenderingSystem.h"

#include <algorithm>

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
}
