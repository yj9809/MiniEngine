#pragma once

#include "Common/Common.h"
#include "Renderer/GPUData/LightingConstantBuffer.h"

namespace Engine
{
    struct RenderFrameData;

    ENGINE_API LightingConstantBuffer BuildLightingConstantBuffer(const RenderFrameData& frameData);
}