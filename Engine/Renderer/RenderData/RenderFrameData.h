#pragma once

#include <vector>

#include "Lighting/RenderData/DirectionalLightRenderData.h"
#include "Lighting/RenderData/EnvironmentLightingRenderData.h"

namespace Engine
{
    struct RenderFrameData
    {
        std::vector<DirectionalLightRenderData> directionalLights;
        EnvironmentLightingRenderData environmentLighting{};
    };
}
