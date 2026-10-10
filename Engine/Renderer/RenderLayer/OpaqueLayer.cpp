#include "OpaqueLayer.h"

#include <d3dcompiler.h>
#include <stdexcept>

namespace Engine
{
    OpaqueLayer::OpaqueLayer(ID3D11Device* device)
    {
        if (!device)
        {
            throw std::invalid_argument("Device pointer is null in OpaqueLayer constructor");
        }

        if (!InitShaders(device))
        {
            throw std::runtime_error("OpaqueLayer shader initialization failed");
        }

        if (!CreateConstantBuffer(device, sizeof(Matrix4), wvpConstantBuffer))
        {
            throw std::runtime_error("OpaqueLayer constant buffer creation failed");
        }

        // 픽셀 셰이더에 전달할 상수 버퍼 생성.
        if (!CreateConstantBuffer(device, sizeof(MaterialConstantBuffer), materialConstantBuffer))
        {
            throw std::runtime_error("OpaqueLayer material constant buffer creation failed");
        }

        // 정점 셰이더에 전달할 월드 행렬 상수 버퍼 생성.
        if (!CreateConstantBuffer(device, sizeof(Matrix4), worldConstantBuffer))
        {
            throw std::runtime_error("OpaqueLayer world constant buffer creation failed");
        }

        // 픽셀 셰이더에 전달할 조명 상수 버퍼 생성.
        if (!CreateConstantBuffer(device, sizeof(LightingConstantBuffer), lightingConstantBuffer))
        {
            throw std::runtime_error("OpaqueLayer lighting constant buffer creation failed");
        }

        // 샘플링 규칙 생성용 설명서.
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // 선형 필터링.
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; // U 좌표 범위 밖의 텍스처 샘플링 시 반복.
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; // V 좌표 범위 밖의 텍스처 샘플링 시 반복.
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; // W 좌표 범위 밖의 텍스처 샘플링 시 반복.
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS; // 비교 함수는 사용하지 않으므로 항상 통과하도록 설정.
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX; // 최대 LOD 설정.

        if (FAILED(device->CreateSamplerState(&samplerDesc, &samplerState)))
        {
            throw std::runtime_error("Failed to create sampler state for OpaqueLayer");
        }
    }

    void OpaqueLayer::Prepare(ID3D11DeviceContext* context, const RenderFrameData& frameData)
    {
        // 정점 셰이더에 버퍼 등록.
        context->VSSetConstantBuffers(0, 1, wvpConstantBuffer.GetAddressOf());
        // 픽셀 셰이더에 버퍼 등록.
        context->PSSetConstantBuffers(0, 1, materialConstantBuffer.GetAddressOf());
        // 정점 셰이더에 월드 행렬 상수 버퍼 등록.
        context->VSSetConstantBuffers(1, 1, worldConstantBuffer.GetAddressOf());

        #pragma region Lighting 상수 버퍼 업데이트.
        LightingConstantBuffer lightingBuffer{};
        lightingBuffer.ambientColor = Vector4(0.1f, 0.1f, 0.1f, 1.0f); // 어두운 환경 조명.

        // 첫 구현에서는 등록된 방향광 중 첫 번째 광원만 GPU에 전달한다.
        if (!frameData.directionalLights.empty())
        {
            const DirectionalLightRenderData& light = frameData.directionalLights.front();
            DirectionalLightGPUData& gpuLight = lightingBuffer.directionalLights[0];

            gpuLight.direction = Vector4(light.direction, 0.0f);
            gpuLight.colorAndIntensity = Vector4(light.color, light.intensity);
            lightingBuffer.directionalLightCount = 1;
        }

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
        if (FAILED(hr))
        {
            return false;
        }

        hr = D3DReadFileToBlob(L"Shader/MeshPixelShader.cso", &psBlod);
        if (FAILED(hr))
        {
            return false;
        }

        // 셰이더 객체 생성.
        hr = device->CreateVertexShader(
            vsBlod->GetBufferPointer(),
            vsBlod->GetBufferSize(),
            nullptr,
            &vertexShader
        );
        if (FAILED(hr))
        {
            return false;
        }

        hr = device->CreatePixelShader(
            psBlod->GetBufferPointer(),
            psBlod->GetBufferSize(),
            nullptr,
            &pixelShader
        );
        if (FAILED(hr))
        {
            return false;
        }

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

        return SUCCEEDED(hr);
    }
}
