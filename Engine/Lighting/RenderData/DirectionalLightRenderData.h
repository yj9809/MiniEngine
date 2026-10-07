#pragma once

#include "Math/Vector3.h"

namespace Engine
{
    struct DirectionalLightRenderData
    {
        Vector3 direction{};
        Vector3 color = Vector3::one;
        float intensity = 1.0f;
    };
}