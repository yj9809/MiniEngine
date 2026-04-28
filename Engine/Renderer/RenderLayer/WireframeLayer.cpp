#include "WireframeLayer.h"

#include <d3dcompiler.h>
#include <cassert>

namespace Engine
{
    WireframeLayer::WireframeLayer(ID3D11Device* device)
    {
        assert(InitShaders(device) && "Failed to initialize shaders for WireframePass");
        assert(InitRasterizer(device) && "Failed to initialize rasterizer for WireframePass");
        assert(
            CreateConstantBuffer(device, sizeof(Matrix4), wvpConstantBuffer)
            && "Failed to create constant buffer for WireframePass"
        );
    }

    void WireframeLayer::Prepare(ID3D11DeviceContext* context)
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

    void WireframeLayer::Draw(ID3D11DeviceContext* context, const RenderCommand& command)
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
        HRESULT hs = device->CreateRasterizerState(&rsDesc, &wireframeRasterizer);
        
        FAILCHECK(hs, L"Failed to create wireframe rasterizer", false)
        
        return true;
    }

    bool WireframeLayer::InitShaders(ID3D11Device* device)
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
