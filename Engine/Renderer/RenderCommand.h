#pragma once

#include "Math/Matrix4.h"

#include <d3d11.h>
#include <cstdint>

namespace Engine
{
    // 정수 값을 버퍼 핸들로 사용하는 간단한 구조체.
    using BufferHandle = uint32_t;
    static constexpr BufferHandle NULL_BUFFER = 0;

    struct RenderCommand
    {
        BufferHandle vertexBuffer = NULL_BUFFER;
        BufferHandle indexBuffer = NULL_BUFFER;
        UINT indexCount = 0;
        UINT stride = 0;
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        Matrix4 worldMatrix = Matrix4::identity;
    };
}
