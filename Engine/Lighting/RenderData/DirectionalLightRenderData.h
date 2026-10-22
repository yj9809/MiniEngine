#pragma once

#include "Math/Vector3.h"

namespace Engine
{
    struct DirectionalLightRenderData
    {
        // 월드 공간에서 빛이 진행하는 방향.
        // 셰이더는 반대 방향을 표면에서 광원으로 향하는 벡터로 사용합니다.
        Vector3 direction{};
        Vector3 color = Vector3::one;
        float intensity = 1.0f;
    };
}