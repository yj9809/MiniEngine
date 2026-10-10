#include "WireframeLayer.h"

#include <d3dcompiler.h>
#include <stdexcept>

namespace Engine
{
    WireframeLayer::WireframeLayer(ID3D11Device* device)
    {
        if (!device)
        {
            throw std::invalid_argument("WireframeLayer requires a valid device.");
        }

        if (!InitShaders(device))
        {
            throw std::runtime_error("WireframeLayer shader initialization failed.");
        }

        if (!InitRasterizer(device))
        {
            throw std::runtime_error("WireframeLayer rasterizer initialization failed.");
        }

        if (!CreateConstantBuffer(device, sizeof(Matrix4), wvpConstantBuffer))
        {
            throw std::runtime_error("WireframeLayer constant buffer creation failed.");
        }
    }

    void WireframeLayer::Prepare(ID3D11DeviceContext* context, const RenderFrameData& frameData)
    {
        // 래스터라이저 바인딩.
        context->RSSetState(wireframeRasterizer.Get());
        
        // 셰이더에 상수 버퍼 등록.
        context->VSSetConstantBuffers(0, 1, wvpConstantBuffer.GetAddressOf());

        // InputLayout 등록.
        context->IASetInputLayout(inputLayout.Get());

        // 셰이더 바인딩: 이후 Draw 호출에서 이 셰이더로 처리.
        context->VSSetShader(vertexShader.Get(), nullptr, 0);
        context->PSSetShader(pixelShader.Get(), nullptr, 0);
    }

    void WireframeLayer::Draw(
        ID3D11DeviceContext* context, 
        const RenderCommand& command,
        const std::unordered_map<TextureHandle, ComPtr<ID3D11ShaderResourceView>>& textureMap)
    {
        context->DrawIndexed(command.indexCount, 0, 0);
    }

    void WireframeLayer::OnPostExecute(ID3D11DeviceContext* context)
    {
        // 래스터라이저 상태 초기화 (기본값으로 되돌리기).
        context->RSSetState(nullptr); 
    }

    bool WireframeLayer::InitRasterizer(ID3D11Device* device)
    {
        // 래스터라이저 생성을 위한 설명서.
        D3D11_RASTERIZER_DESC rsDesc = {};
        rsDesc.FillMode = D3D11_FILL_WIREFRAME; // 와이어프레임 모드 설정.
        rsDesc.CullMode = D3D11_CULL_BACK; // 뒷면.
        
        // 래스터라이저 생성.
        HRESULT hr = device->CreateRasterizerState(&rsDesc, &wireframeRasterizer);
        
        return SUCCEEDED(hr);
    }

    bool WireframeLayer::InitShaders(ID3D11Device* device)
    {
        // .cso: 빌드 시 컴파일한 셰이더 바이너리. 셰이더 객체 생성에 사용.
        ComPtr<ID3DBlob> vsBlod;
        ComPtr<ID3DBlob> psBlod;

        // 셰이더 .cso 파일 로드.
        HRESULT hr = D3DReadFileToBlob(L"Shader/WireframeVertexShader.cso", &vsBlod);
        if (FAILED(hr))
        {
            return false;
        }
        hr = D3DReadFileToBlob(L"Shader/WireframePixelShader.cso", &psBlod);
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
            }
        };
        // Input 레이아웃 추가시 NumElements 값도 추가해주어야 한다.
        hr = device->CreateInputLayout(
            layoutDesc,
            1,
            vsBlod->GetBufferPointer(),
            vsBlod->GetBufferSize(),
            &inputLayout
        );

        return SUCCEEDED(hr);
    }
}
