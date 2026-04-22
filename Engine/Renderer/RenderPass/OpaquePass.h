#pragma once

#include "PassScheduler.h"

namespace Engine
{
    class OpaquePass : public RenderPass
    {
    public:
        OpaquePass(ID3D11Device* device);

        virtual void Prepare(ID3D11DeviceContext* context) override;

        virtual void Execute(
            ID3D11DeviceContext* context,
            const std::vector<RenderCommand>& commands,
            const std::unordered_map<BufferHandle, ComPtr<ID3D11Buffer>>& bufferMap) override;

    private:
        // 상수 버퍼 생성.
        bool CreateConstantBuffer(ID3D11Device* device, UINT byteWidth, ComPtr<ID3D11Buffer>& buffer) const;

        // Shader 생성.
        bool InitShaders(ID3D11Device* device);

    private:
        // World View Projaction 행렬을 담는 상수 버퍼.
        ComPtr<ID3D11Buffer> wvpConstantBuffer;

        // vertex 버퍼 구조를 셰이더에 알려주는 객체.
        // 버퍼의 float3은 POSITION이다를 GPU에 정의.
        ComPtr<ID3D11InputLayout> inputLayout;

        // 셰이더 객체.
        // VS: 정점 셰이더, PS: 픽셀 셰이더.
        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
    };
}
