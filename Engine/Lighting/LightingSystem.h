#pragma once

#include <vector>

#include "Component/Light/DirectionalLightComponent.h"

namespace Engine
{
    class LightingSystem
    {
    public:
        void RegisterDirectionalLight(const DirectionalLightComponent* directionalLight);
        void UnregisterDirectionalLight(const DirectionalLightComponent* directionalLight);

    private:
        std::vector<const DirectionalLightComponent*> directionalLights;
    };
}