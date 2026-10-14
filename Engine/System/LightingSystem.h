#pragma once

#include <vector>

#include "Lighting/RenderData/DirectionalLightRenderData.h"

namespace Engine
{
    class DirectionalLightComponent;
    class SkyLightComponent;

    class LightingSystem
    {
    public:
        // DirectionalLightComponent를 등록/해제하는 메서드
        void Register(const DirectionalLightComponent* lightComponent);
        void Unregister(const DirectionalLightComponent* lightComponent);

        // SkyLightComponent를 등록/해제하는 메서드
        void Register(const SkyLightComponent* lightComponent);
        void Unregister(const SkyLightComponent* lightComponent);

        void BuildDirectionalLightRenderData(std::vector<DirectionalLightRenderData>& outRenderData) const;

        void Clear();
        
    private:
        std::vector<const DirectionalLightComponent*> directionalLights;
        std::vector<const SkyLightComponent*> skyLights;
    };
}