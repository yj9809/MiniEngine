#pragma once

#include "Math/Matrix4.h"
#include "RenderPass/RenderPassType.h"

#include <d3d11.h>
#include <cstdint>

namespace Engine
{
    // 정수 값을 버퍼 핸들로 사용하는 간단한 구조체.
    using BufferHandle = uint32_t;
    static constexpr BufferHandle NULL_BUFFER = 0;

    struct RenderCommand
    {
        // 정점 버퍼 핸들.
        BufferHandle vertexBuffer = NULL_BUFFER;
        // 인덱스 버퍼 핸들.
        BufferHandle indexBuffer = NULL_BUFFER;
        // 인덱스 버퍼 크기 설정용 변수.
        UINT indexCount = 0;
        // 정점 하나의 크기 설정용 변수.
        UINT stride = 0;
        // 그리기를 할 때 사용할 기본 프리미티브 토폴로지. 기본값은 삼각형 리스트.
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        // 월드 좌표.
        Matrix4 worldMatrix = Matrix4::identity;
        // RenderPass 타입.
        // 해당 타입에 따라 그려질 패스가 달라진다. 기본값은 Opaque.
        RenderPassType passType = RenderPassType::Opaque;
    };
}
