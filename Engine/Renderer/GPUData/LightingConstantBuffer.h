#pragma once

#include <cstdint>

#include "Math/Vector4.h"

namespace Engine
{
    // GPU 버퍼에 저장할 수 있는 방향광의 최대 개수.
    // 용량 변경 시 HLSL 배열 크기도 함께 변경해야 한다.
    inline constexpr uint32_t MaxDirectionalLights = 4;

    struct DirectionalLightGPUData
    {
        Vector4 direction{};
        // RGB + Intensity.
        Vector4 colorAndIntensity{};
    };

    struct LightingConstantBuffer
    {
        DirectionalLightGPUData directionalLights[MaxDirectionalLights]{};

        Vector4 skyContribution{};
        Vector4 groundContribution{};

        uint32_t directionalLightCount = 0;
        float padding[3]{};
    };
}