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
    class ENGINE_API RenderLayer
    {
    public:
        virtual ~RenderLayer() = default;

        // 해당 패스에서 사용할 Shader, Input Layout, 렌더 상태(블렌딩, 깊이 테스트 등)를 설정한다.
        virtual void Prepare(ID3D11DeviceContext* context) = 0;
        // Prepare 설정을 바탕으로 RenderCommand를 순회하며 드로우 콜을 실행.
        void Execute(
            ID3D11DeviceContext* context,
            const std::vector<RenderCommand>& commands,
            const std::unordered_map<BufferHandle, ComPtr<ID3D11Buffer>>& bufferMap);
        
    protected:
        // 상수 버퍼 생성.
        bool CreateConstantBuffer(ID3D11Device* device, UINT byteWidth, ComPtr<ID3D11Buffer>& buffer) const;
        
        // 파생 클래스에서 상수 버퍼를 가져오기 위해 Getter 구현.
        virtual ID3D11Buffer* GetConstantBuffer() = 0;
        
        // Execute 함수에서 드로우콜이 전부 끝난 후 렌더링 파이프라인 상태 복구 함수.
        virtual void OnPostExecute(ID3D11DeviceContext* context) {}
        
        // 파생 클래스에서 각자 드로우 구현을 하기 위한 순수 가상 함수.
        virtual void Draw(ID3D11DeviceContext* context, const RenderCommand& command) = 0;
        
    private:
        // 각 패스에 등록된 상수 버퍼로 행렬 업데이트.
        void UpdateConstantBuffer(ID3D11DeviceContext* context, const RenderCommand& command);
        
        // VB/IB/Topology 설정 함수.
        bool BindBuffer(ID3D11DeviceContext* context, const RenderCommand& command, const std::unordered_map<BufferHandle, ComPtr<ID3D11Buffer>>& bufferMap);
        
    protected:
        
    };
}
