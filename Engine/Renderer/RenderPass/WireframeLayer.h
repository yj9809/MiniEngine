#pragma once

#include "RenderLayer.h"

namespace Engine
{
    class WireframeLayer : public RenderLayer
    {
    public:
        WireframeLayer(ID3D11Device* device);

        virtual void Prepare(ID3D11DeviceContext* context) override;
        
    protected:
        inline virtual ID3D11Buffer* GetConstantBuffer() override { return wvpConstantBuffer.Get(); }
        
        virtual void Draw(ID3D11DeviceContext* context, const RenderCommand& command) override;
        
        virtual void OnPostExecute(ID3D11DeviceContext* context) override;
        
    private:
        // 래스터라이저 생성.
        bool InitRasterizer(ID3D11Device* device);
        
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
        
        // 와이어프레임 레스터라이저 객체.
        ComPtr<ID3D11RasterizerState> wireframeRasterizer;
    };
}
