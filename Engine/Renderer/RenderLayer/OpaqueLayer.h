#pragma once

#include <cstdint>

#include "LayerScheduler.h"
#include "Math/Vector4.h"

namespace Engine
{
    class OpaqueLayer : public RenderLayer
    {
    public:
        OpaqueLayer(ID3D11Device* device);

        virtual void Prepare(ID3D11DeviceContext* context, const RenderFrameData& frameData) override;

    protected:
        inline virtual ID3D11Buffer* GetConstantBuffer() override { return wvpConstantBuffer.Get(); }

        virtual void Draw(
            ID3D11DeviceContext* context,
            const RenderCommand& command,
            const std::unordered_map<TextureHandle, ComPtr<ID3D11ShaderResourceView>>& textureMap
        ) override;

    private:
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
        
        // 텍스터 샘플링 규칙 정의 객체.
        ComPtr<ID3D11SamplerState> samplerState;

        // 픽셸 세이더에 전달할 상수 버퍼.
        // 추후 Material 파라미터 확장을 쉽게 하기 위해 구조리로 정의.
        struct MaterialConstantBuffer
        {
            Vector4 baseColor;
        };
        ComPtr<ID3D11Buffer> materialConstantBuffer;

        // 정점 셰이더에 전달한 월드 행렬 상수 버퍼.
        ComPtr<ID3D11Buffer> worldConstantBuffer;

        // GPU 버퍼 용량은 최대 4개.
        // 첫 구현에서는 이 중 첫 번째 방향광만 사용.
        static constexpr uint32_t MaxDirectionalLights = 4;

        // 픽셀 셰이더에 전달할 조명 상수 버퍼.
        // 추후 조명 파라미터 확장을 쉽게 하기 위해 구조리로 정의.
        struct DirectionalLightGPUData
        {
            Vector4 direction;
            Vector4 colorAndIntensity; // RGB + Intensity
        };

        struct LightingConstantBuffer
        {
            DirectionalLightGPUData directionalLights[MaxDirectionalLights];
            Vector4 ambientColor;

            uint32_t directionalLightCount = 0;
            float padding[3]{};
        };

        ComPtr<ID3D11Buffer> lightingConstantBuffer;
    };
}
