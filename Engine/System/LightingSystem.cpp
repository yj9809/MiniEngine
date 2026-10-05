#include "LightingSystem.h"

#include <algorithm>

namespace Engine
{
    void LightingSystem::RegisterDirectionalLight(const DirectionalLightComponent* directionalLight)
    {
        if(std::find(directionalLights.begin(), directionalLights.end(), directionalLight) == directionalLights.end())
        {
            directionalLights.push_back(directionalLight);
        }
    }

    void LightingSystem::UnregisterDirectionalLight(const DirectionalLightComponent* directionalLight)
    {
        auto it = std::find(directionalLights.begin(), directionalLights.end(), directionalLight);
        
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
