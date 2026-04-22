#pragma once

#include "Common/Common.h"
#include "Renderer/RenderCommand.h"

#include <d3d11.h>
#include <vector>
#include <unordered_map>
#include <wrl/client.h>

using namespace Microsoft::WRL;

namespace Engine
{
    // Render pass base class.
    class ENGINE_API RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        // 해당 패스에서 사용할 Shader, Input Layout, 렌더 상태(블렌딩, 깊이 테스트 등)를 설정한다.
        virtual void Prepare(ID3D11DeviceContext* context) = 0;
        // Prepare 설정을 바탕으로 RenderCommand를 순회하며 드로우 콜을 실행.
        virtual void Execute(
            ID3D11DeviceContext* context,
            const std::vector<RenderCommand>& commands,
            const std::unordered_map<BufferHandle, ComPtr<ID3D11Buffer>>& bufferMap) = 0;
    };
}
