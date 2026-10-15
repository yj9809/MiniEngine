#include "LightingSystem.h"

#include <algorithm>

#include "Component/Light/DirectionalLightComponent.h"
#include "Component/Light/SkyLightComponent.h"

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

    void LightingSystem::Register(const SkyLightComponent* lightComponent)
    {
        if(lightComponent == nullptr)
        {
            return;
        }

        const auto it = std::find(skyLights.begin(), skyLights.end(), lightComponent);
        if(it == skyLights.end())
        {
            skyLights.push_back(lightComponent);
        }
    }

    void LightingSystem::Unregister(const SkyLightComponent* lightComponent)
    {
        const auto it = std::find(skyLights.begin(), skyLights.end(), lightComponent);
        if(it != skyLights.end())
        {
            skyLights.erase(it);
        }
    }

    void LightingSystem::BuildDirectionalLightRenderData(std::vector<DirectionalLightRenderData>& outRenderData) const
    {
        outRenderData.clear();
        outRenderData.reserve(directionalLights.size());

        for (const auto* lightComponent : directionalLights)
        {
            if (lightComponent)
            {
                outRenderData.emplace_back(lightComponent->BuildRenderData());
            }
        }
    }

    EnvironmentLightingRenderData LightingSystem::BuildEnvironmentLightingRenderData() const
    {
        EnvironmentLightingRenderData result{};

        for (const auto* sky : skyLights)
        {
            const float intensity = sky->GetIntensity();

            result.skyContribution = result.skyContribution + sky->GetSkyColor() * intensity;
            result.groundContribution = result.groundContribution + sky->GetGroundColor() * intensity;
        }

        return result;
    }

    void LightingSystem::Clear()
    {
        directionalLights.clear();
        skyLights.clear();
    }
}
