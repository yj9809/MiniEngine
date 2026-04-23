#include "RenderPass.h"

namespace Engine
{
    void RenderPass::Execute(ID3D11DeviceContext* context, const std::vector<RenderCommand>& commands,
        const std::unordered_map<BufferHandle, ComPtr<ID3D11Buffer>>& bufferMap)
    {
        for (const RenderCommand& command : commands)
        {
            UpdateConstantBuffer(context, command);
            BindBuffer(context, command, bufferMap);
            Draw(context, command);
        }
        OnPostExecute(context);
    }
    
    bool RenderPass::CreateConstantBuffer(ID3D11Device* device, UINT byteWidth, ComPtr<ID3D11Buffer>& buffer) const
    {
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = byteWidth;
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, &buffer);

        FAILCHECK(hr, L"Failed to create constant buffer", false)

        return true;
    }

    void RenderPass::UpdateConstantBuffer(ID3D11DeviceContext* context, const RenderCommand& command)
    {
        auto* cBuffer = GetConstantBuffer();
        
        // 커맨드마다 worldMatrix를 WVP 행렬로 변환하여 상수 버퍼에 업데이트.
        // WVPMatrix4에 담을 데이터를 얻기 위해 Map으로 버퍼 접근 권한 받아오기.
        D3D11_MAPPED_SUBRESOURCE mappedResource = {};
        HRESULT hr = context->Map(
            cBuffer,
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mappedResource
        );

        FAILCHECK(hr, L"Failed to map resource",)

        // 받아온 버퍼 데이터를 WorldViewProjection 행렬로 채우기.
        Matrix4 wvpMatrix = command.worldMatrix;
        memcpy(mappedResource.pData, &wvpMatrix, sizeof(Matrix4));
        context->Unmap(cBuffer, 0);
    }

    bool RenderPass::BindBuffer(ID3D11DeviceContext* context, const RenderCommand& command, const std::unordered_map<BufferHandle, ComPtr<ID3D11Buffer>>& bufferMap)
    {
        UINT offset = 0;
        
        // 버퍼 맵에서 각 버퍼 이터레이터 찾기.
        auto vbIt = bufferMap.find(command.vertexBuffer);
        auto ibIt = bufferMap.find(command.indexBuffer);

        // 버퍼 핸들이 유효한지 확인. 
        // 하나라도 유효하지 않으면 해당 명령은 건너뛰기.
        if (vbIt == bufferMap.end() || ibIt == bufferMap.end())
        {
            return false;
        }

        // 버퍼가 유효하면 GPU에 버퍼 바인딩, 프리미티브 토폴로지 설정.
        ID3D11Buffer* vb = vbIt->second.Get();
        context->IASetVertexBuffers(0, 1, &vb, &command.stride, &offset);
        context->IASetIndexBuffer(ibIt->second.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(command.topology);
    }    
}
