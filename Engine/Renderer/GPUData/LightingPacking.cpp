#include "LightingPacking.h"

#include "Renderer/RenderData/RenderFrameData.h"

namespace Engine
{
    LightingConstantBuffer BuildLightingConstantBuffer(const RenderFrameData& frameData)
    {
        LightingConstantBuffer result{};

        const EnvironmentLightingRenderData& environment = frameData.environmentLighting;

        result.skyContribution = Vector4(environment.skyContribution, 0.0f);
        result.groundContribution = Vector4(environment.groundContribution, 0.0f);

        for(uint32_t i = 0; i < frameData.directionalLights.size() && i < MaxDirectionalLights; ++i)
        {
            // 입력: LightingSystem에서 수집해 프레임 데이터에 담아둔 방향광.
            const DirectionalLightRenderData& light = frameData.directionalLights[i];

            // 출력: GPU용 배치로 채울 방향광.
            DirectionalLightGPUData& gpuLight = result.directionalLights[i];

            gpuLight.direction = Vector4(light.direction, 0.0f);
            gpuLight.colorAndIntensity = Vector4(light.color, light.intensity);

            result.directionalLightCount++;
        }

        return result;
    }
}