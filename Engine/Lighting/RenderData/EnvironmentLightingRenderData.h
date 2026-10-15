#pragma once

#include "Math/Vector3.h"

namespace Engine
{
    struct EnvironmentLightingRenderData
    {
        Vector3 skyContribution{};
        Vector3 groundContribution{};
    };
}