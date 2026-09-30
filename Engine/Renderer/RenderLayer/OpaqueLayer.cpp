#include "OpaqueLayer.h"

#include <d3dcompiler.h>
#include <cassert>
#include <cstring>

namespace Engine
{
    OpaqueLayer::OpaqueLayer(ID3D11Device* device)
    {
        bool isShaders = InitShaders(device);
        assert(isShaders && "Failed to initialize shaders for OpaquePass");
        bool isConstantBuffer = CreateConstantBuffer(device, sizeof(Matrix4), wvpConstantBuffer);
        assert(isConstantBuffer && "Failed to create constant buffer for OpaquePass");

        // 픽셀 셰이더에 전달할 상수 버퍼 생성.
        bool isMaterialBuffer = CreateConstantBuffer(device, sizeof(MaterialConstantBuffer), materialConstantBuffer);
        assert(isMaterialBuffer && "Failed to create material constant buffer for OpaquePass");

        // 정점 셰이더에 전달할 월드 행렬 상수 버퍼 생성.
        bool isWorldBuffer = CreateConstantBuffer(device, sizeof(Matrix4), worldConstantBuffer);
        assert(isWorldBuffer && "Failed to create world constant buffer for OpaquePass");

        // 픽셀 셰이더에 전달할 조명 상수 버퍼 생성.
        bool isLightingBuffer = CreateConstantBuffer(device, sizeof(LightingConstantBuffer), lightingConstantBuffer);
        assert(isLightingBuffer && "Failed to create lighting constant buffer for OpaquePass");

        // 샘플링 규칙 생성용 설명서.
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // 선형 필터링.
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; // U 좌표 범위 밖의 텍스처 샘플링 시 반복.
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; // V 좌표 범위 밖의 텍스처 샘플링 시 반복.
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; // W 좌표 범위 밖의 텍스처 샘플링 시 반복.
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS; // 비교 함수는 사용하지 않으므로 항상 통과하도록 설정.
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX; // 최대 LOD 설정.
        
        bool isSamplerState = SUCCEEDED(device->CreateSamplerState(&samplerDesc, &samplerState));
        assert(isSamplerState && "Failed to create sampler state for OpaquePass");
    }

    void OpaqueLayer::Prepare(ID3D11DeviceContext* context)
    {
        // 정점 셰이더에 버퍼 등록.
        context->VSSetConstantBuffers(0, 1, wvpConstantBuffer.GetAddressOf());
        // 픽셀 셰이더에 버퍼 등록.
        context->PSSetConstantBuffers(0, 1, materialConstantBuffer.GetAddressOf());
        // 정점 셰이더에 월드 행렬 상수 버퍼 등록.
        context->VSSetConstantBuffers(1, 1, worldConstantBuffer.GetAddressOf());
        
        // 광원은 하나의 방향광만 존재한다고 가정.
        #pragma region Lighting 상수 버퍼 업데이트.
        LightingConstantBuffer lightingBuffer{};
        // 조명 방향은 위에서 아래로 향하는 방향으로 설정.
        lightingBuffer.lightDirection = Vector4(0.0f, -1.0f, 0.0f, 0.0f);
        
        D3D11_MAPPED_SUBRESOURCE mappedResource{};
        HRESULT hr = context->Map(lightingConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        FAILCHECK(hr, L"Failed to map lighting constant buffer", )

        memcpy(mappedResource.pData, &lightingBuffer, sizeof(LightingConstantBuffer));

        context->Unmap(lightingConstantBuffer.Get(), 0);
        #pragma endregion
        // 픽셀 셰이더에 조명 상수 버퍼 등록.
        context->PSSetConstantBuffers(1, 1, lightingConstantBuffer.GetAddressOf());

        // InputLayout 등록.
        context->IASetInputLayout(inputLayout.Get());

        // 셰이더 바인딩: 이후 Draw 호출에서 이 셰이더로 처리.
        context->VSSetShader(vertexShader.Get(), nullptr, 0);
        context->PSSetShader(pixelShader.Get(), nullptr, 0);
        
        // 샘플링 규칙 등록.
        context->PSSetSamplers(0, 1, samplerState.GetAddressOf());
    }

    void OpaqueLayer::Draw(
        ID3D11DeviceContext* context, 
        const RenderCommand& command,
        const std::unordered_map<TextureHandle, ComPtr<ID3D11ShaderResourceView>>& textureMap)
    {
        #pragma region Material 상수 버퍼 업데이트.
        MaterialConstantBuffer materialBuffer{};
        materialBuffer.baseColor = command.baseColor;

        D3D11_MAPPED_SUBRESOURCE mappedResource{};

        HRESULT hr = context->Map(materialConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        FAILCHECK(hr, L"Failed to map material constant buffer", )

        memcpy(mappedResource.pData, &materialBuffer, sizeof(MaterialConstantBuffer));

        context->Unmap(materialConstantBuffer.Get(), 0);
        #pragma endregion

        #pragma region World 상수 버퍼 업데이트.
        hr = context->Map(worldConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        FAILCHECK(hr, L"Failed to map world constant buffer", )

        memcpy(mappedResource.pData, &command.worldMatrix, sizeof(Matrix4));

        context->Unmap(worldConstantBuffer.Get(), 0);
        #pragma endregion

        // Texture 바인딩.
        ID3D11ShaderResourceView* textureView = nullptr;

        auto it = textureMap.find(command.texture);
        if (it != textureMap.end())
        {
            textureView = it->second.Get();
        }

        context->PSSetShaderResources(0, 1, &textureView);
        context->DrawIndexed(command.indexCount, 0, 0);
    }

    bool OpaqueLayer::InitShaders(ID3D11Device* device)
    {
        // .cso: 빌드 시 컴파일한 셰이더 바이너리. 셰이더 객체 생성에 사용.
        ComPtr<ID3DBlob> vsBlod;
        ComPtr<ID3DBlob> psBlod;

        // 셰이더 .cso 파일 로드.
        HRESULT hr = D3DReadFileToBlob(L"Shader/MeshVertexShader.cso", &vsBlod);
        FAILCHECK(hr, L"Failed to read VertexShader.cso", false)
        hr = D3DReadFileToBlob(L"Shader/MeshPixelShader.cso", &psBlod);
        FAILCHECK(hr, L"Failed to read PixelShader.cso", false)

        // 셰이더 객체 생성.
        hr = device->CreateVertexShader(
            vsBlod->GetBufferPointer(),
            vsBlod->GetBufferSize(),
            nullptr,
            &vertexShader
        );
        FAILCHECK(hr, L"Failed to create vertex shader", false)
        hr = device->CreatePixelShader(
            psBlod->GetBufferPointer(),
            psBlod->GetBufferSize(),
            nullptr,
            &pixelShader
        );
        FAILCHECK(hr, L"Failed to create pixel shader", false)

        // Input Layout 생성.
        D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
        {
            {
                "POSITION",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                0,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            },
            {
                "NORMAL",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                D3D11_APPEND_ALIGNED_ELEMENT,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            },
            {
                "TEXCOORD",
                0,
                DXGI_FORMAT_R32G32_FLOAT,
                0,
                D3D11_APPEND_ALIGNED_ELEMENT,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            }
        };
        // Input 레이아웃 추가시 NumElements 값도 추가해주어야 한다.
        hr = device->CreateInputLayout(
            layoutDesc,
            3,
            vsBlod->GetBufferPointer(),
            vsBlod->GetBufferSize(),
            &inputLayout
        );
        FAILCHECK(hr, L"Failed to create input layout", false)

        return true;
    }
}
