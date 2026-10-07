#include "LightingSystem.h"

#include <algorithm>

#include "Component/Light/DirectionalLightComponent.h"

namespace Engine
{
    void LightingSystem::Register(const DirectionalLightComponent* lightComponent)
    {
        if(lightComponent == nullptr)
        {
            return;
        }

        const auto it = std::find(directionalLights.begin(), directionalLights.end(), lightComponent);
        if(it == directionalLights.end())
        {
            directionalLights.push_back(lightComponent);
        }
    }

    void LightingSystem::Unregister(const DirectionalLightComponent* lightComponent)
    {
        const auto it = std::find(directionalLights.begin(), directionalLights.end(), lightComponent);
        if(it != directionalLights.end())
        {
            directionalLights.erase(it);
        }
    }

    void LightingSystem::Clear()
    {
        directionalLights.clear();
    }
}
