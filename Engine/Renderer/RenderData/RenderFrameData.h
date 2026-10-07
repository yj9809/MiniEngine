#pragma once

#include <vector>

#include "Lighting/RenderData/DirectionalLightRenderData.h"

namespace Engine
{
    struct RenderFrameData
    {
        std::vector<DirectionalLightRenderData> directionalLights;
    };
}
