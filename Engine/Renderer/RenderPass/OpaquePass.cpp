#include "OpaquePass.h"

#include <d3dcompiler.h>
#include <cassert>

namespace Engine
{
    OpaquePass::OpaquePass(ID3D11Device* device)
    {
        assert(InitShaders(device) && "Failed to initialize shaders for OpaquePass");
        assert(
            CreateConstantBuffer(device, sizeof(Matrix4), wvpConstantBuffer)
            && "Failed to create constant buffer for OpaquePass"
        );
    }

    void OpaquePass::Prepare(ID3D11DeviceContext* context)
    {
        // 셰이더에 버퍼 등록.
        context->VSSetConstantBuffers(0, 1, wvpConstantBuffer.GetAddressOf());

        // InputLayout 등록.
        context->IASetInputLayout(inputLayout.Get());

        // 셰이더 바인딩: 이후 Draw 호출에서 이 셰이더로 처리.
        context->VSSetShader(vertexShader.Get(), nullptr, 0);
        context->PSSetShader(pixelShader.Get(), nullptr, 0);
    }

    void OpaquePass::Execute(
        ID3D11DeviceContext* context,
        const std::vector<RenderCommand>& commands,
        const std::unordered_map<BufferHandle, ComPtr<ID3D11Buffer>>& bufferMap)
    {
        UINT offset = 0;

        //RenderCommand 리스트를 순회하며 Draw를 실행.
        for (const RenderCommand& command : commands)
        {
            // 커맨드마다 worldMatrix를 WVP 행렬로 변환하여 상수 버퍼에 업데이트.
            // WVPMatrix4에 담을 데이터를 얻기 위해 Map으로 버퍼 접근 권한 받아오기.
            D3D11_MAPPED_SUBRESOURCE mappedResource = {};
            HRESULT hr = context->Map(
                wvpConstantBuffer.Get(),
                0,
                D3D11_MAP_WRITE_DISCARD,
                0,
                &mappedResource
            );

            FAILCHECK(hr, L"Failed to map resource",)

            // 받아온 버퍼 데이터를 WorldViewProjection 행렬로 채우기.
            Matrix4 wvpMatrix = command.worldMatrix;
            memcpy(mappedResource.pData, &wvpMatrix, sizeof(Matrix4));

            // 행렬 채운 후 버퍼 접근 권한 돌려주기.
            context->Unmap(wvpConstantBuffer.Get(), 0);

            // 버퍼 맵에서 각 버퍼 이터레이터 찾기.
            auto vbIt = bufferMap.find(command.vertexBuffer);
            auto ibIt = bufferMap.find(command.indexBuffer);

            // 버퍼 핸들이 유효한지 확인. 
            // 하나라도 유효하지 않으면 해당 명령은 건너뛰기.
            if (vbIt == bufferMap.end() || ibIt == bufferMap.end())
            {
                continue;
            }

            // 버퍼가 유효하면 GPU에 버퍼 바인딩, 프리미티브 토폴로지 설정, Draw 호출.
            ID3D11Buffer* vb = vbIt->second.Get();
            context->IASetVertexBuffers(0, 1, &vb, &command.stride, &offset);
            context->IASetIndexBuffer(ibIt->second.Get(), DXGI_FORMAT_R32_UINT, 0);
            context->IASetPrimitiveTopology(command.topology);

            context->DrawIndexed(command.indexCount, 0, 0);
        }
    }

    bool OpaquePass::CreateConstantBuffer(
        ID3D11Device* device, 
        UINT byteWidth, 
        ComPtr<ID3D11Buffer>& buffer) const
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

    bool OpaquePass::InitShaders(ID3D11Device* device)
    {
        // .cso: 빌드 시 컴파일한 셰이더 바이너리. 셰이더 객체 생성에 사용.
        ComPtr<ID3DBlob> vsBlod;
        ComPtr<ID3DBlob> psBlod;

        // 셰이더 .cso 파일 로드.
        HRESULT hr = D3DReadFileToBlob(L"Shader/VertexShader.cso", &vsBlod);
        FAILCHECK(hr, L"Failed to read VertexShader.cso", false)
        hr = D3DReadFileToBlob(L"Shader/PixelShader.cso", &psBlod);
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
                "COLOR",
                0,
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                0,
                D3D11_APPEND_ALIGNED_ELEMENT,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            }
        };
        // Input 레이아웃 추가시 NumElements 값도 추가해주어야 한다.
        hr = device->CreateInputLayout(
            layoutDesc,
            2,
            vsBlod->GetBufferPointer(),
            vsBlod->GetBufferSize(),
            &inputLayout
        );
        FAILCHECK(hr, L"Failed to create input layout", false)

        return true;
    }
}
