#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <d3d11.h>
#include <d3d11sdklayers.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <wrl/client.h>

#include "Renderer/RenderLayer/OpaqueLayer.h"
#include "Actor/Actor.h"
#include "Component/Light/DirectionalLightComponent.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

namespace
{
    using namespace Engine;
    using Microsoft::WRL::ComPtr;

    std::filesystem::path ExecutableDirectory()
    {
        wchar_t path[32768]{};
        const DWORD length = GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
        if (length == 0 || length >= std::size(path))
            throw std::runtime_error("Cannot locate test executable");
        return std::filesystem::path(path).parent_path();
    }

    RenderFrameData MakeGpuFrame(uint32_t count)
    {
        RenderFrameData frame;
        frame.environmentLighting.skyContribution = Vector3(0.25f, 2.0f, 8.0f);
        frame.environmentLighting.groundContribution = Vector3(4.0f, 0.0f, 0.125f);
        for (uint32_t i = 0; i < count; ++i)
        {
            const float value = static_cast<float>(i + 1);
            DirectionalLightRenderData light;
            light.direction = Vector3(value, -2 * value, 3 * value);
            light.color = Vector3(0.25f * value, 2 * value, 0.5f * value);
            light.intensity = 1.5f * value;
            frame.directionalLights.push_back(light);
        }
        return frame;
    }

    void ExpectVector(const Vector4& actual, const Vector4& expected)
    {
        EXPECT_FLOAT_EQ(actual.x, expected.x);
        EXPECT_FLOAT_EQ(actual.y, expected.y);
        EXPECT_FLOAT_EQ(actual.z, expected.z);
        EXPECT_FLOAT_EQ(actual.w, expected.w);
    }

    uint32_t ExpectedCount(const RenderFrameData& frame)
    {
        return frame.directionalLights.size() > MaxDirectionalLights
            ? MaxDirectionalLights : static_cast<uint32_t>(frame.directionalLights.size());
    }

    // Expected values come from input semantics, not the production packing function.
    std::vector<Vector4> ExpectedPixels(const RenderFrameData& frame)
    {
        const uint32_t count = ExpectedCount(frame);
        std::vector<Vector4> values = {
            Vector4(frame.environmentLighting.skyContribution, 0),
            Vector4(frame.environmentLighting.groundContribution, 0),
            Vector4(static_cast<float>(count), 0, 0, 0)
        };
        for (uint32_t i = 0; i < MaxDirectionalLights; ++i)
        {
            values.push_back(i < count ? Vector4(frame.directionalLights[i].direction, 0) : Vector4{});
            values.push_back(i < count ? Vector4(frame.directionalLights[i].color,
                frame.directionalLights[i].intensity) : Vector4{});
        }
        return values;
    }

    class LightingGpuTest : public ::testing::TestWithParam<D3D_DRIVER_TYPE>
    {
    protected:
        void SetUp() override
        {
            originalDirectory = std::filesystem::current_path();
            std::filesystem::current_path(ExecutableDirectory());
            const D3D_FEATURE_LEVEL requested = D3D_FEATURE_LEVEL_11_0;
            D3D_FEATURE_LEVEL actual{};
            HRESULT hr = D3D11CreateDevice(nullptr, GetParam(), nullptr, D3D11_CREATE_DEVICE_DEBUG,
                &requested, 1, D3D11_SDK_VERSION, &device, &actual, &context);
            if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING)
            {
                device.Reset();
                context.Reset();
                hr = D3D11CreateDevice(nullptr, GetParam(), nullptr, 0,
                    &requested, 1, D3D11_SDK_VERSION, &device, &actual, &context);
            }
            ASSERT_TRUE(SUCCEEDED(hr)) << "D3D11CreateDevice HRESULT=" << std::hex << hr;
            ASSERT_EQ(actual, D3D_FEATURE_LEVEL_11_0);
            RecordProperty("debug_layer", SUCCEEDED(device.As(&infoQueue)) ? "enabled" : "unavailable");
            ComPtr<IDXGIDevice> dxgiDevice;
            ComPtr<IDXGIAdapter> adapter;
            ASSERT_TRUE(SUCCEEDED(device.As(&dxgiDevice)));
            ASSERT_TRUE(SUCCEEDED(dxgiDevice->GetAdapter(&adapter)));
            DXGI_ADAPTER_DESC description{};
            ASSERT_TRUE(SUCCEEDED(adapter->GetDesc(&description)));
            const std::wstring adapterName(description.Description);
            RecordProperty("adapter", std::string(adapterName.begin(), adapterName.end()));
            ASSERT_NO_THROW(layer = std::make_unique<OpaqueLayer>(device.Get()));
        }

        void TearDown() override
        {
            layer.reset();
            if (context) context->ClearState();
            if (infoQueue)
            {
                for (UINT64 i = 0; i < infoQueue->GetNumStoredMessages(); ++i)
                {
                    SIZE_T size = 0;
                    if (FAILED(infoQueue->GetMessage(i, nullptr, &size))) continue;
                    std::vector<unsigned char> storage(size);
                    auto* message = reinterpret_cast<D3D11_MESSAGE*>(storage.data());
                    if (SUCCEEDED(infoQueue->GetMessage(i, message, &size)) &&
                        message->Severity <= D3D11_MESSAGE_SEVERITY_WARNING)
                    {
                        ADD_FAILURE() << message->pDescription;
                    }
                }
            }
            if (!originalDirectory.empty()) std::filesystem::current_path(originalDirectory);
        }

        void ReadBoundLighting(LightingConstantBuffer& result)
        {
            ComPtr<ID3D11Buffer> bound;
            context->PSGetConstantBuffers(1, 1, &bound);
            ASSERT_NE(bound.Get(), nullptr);
            D3D11_BUFFER_DESC description{};
            bound->GetDesc(&description);
            ASSERT_EQ(description.ByteWidth, sizeof(LightingConstantBuffer));
            EXPECT_EQ(description.BindFlags, D3D11_BIND_CONSTANT_BUFFER);
            EXPECT_EQ(description.Usage, D3D11_USAGE_DYNAMIC);

            description.Usage = D3D11_USAGE_STAGING;
            description.BindFlags = 0;
            description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            ComPtr<ID3D11Buffer> staging;
            ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&description, nullptr, &staging)));
            context->CopyResource(staging.Get(), bound.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            ASSERT_TRUE(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
            std::memcpy(&result, mapped.pData, sizeof(result));
            context->Unmap(staging.Get(), 0);
        }

        void ExpectUploaded(const RenderFrameData& frame)
        {
            layer->Prepare(context.Get(), frame);
            LightingConstantBuffer uploaded{};
            ASSERT_NO_FATAL_FAILURE(ReadBoundLighting(uploaded));
            const auto expected = ExpectedPixels(frame);
            ExpectVector(uploaded.skyContribution, expected[0]);
            ExpectVector(uploaded.groundContribution, expected[1]);
            EXPECT_EQ(uploaded.directionalLightCount, ExpectedCount(frame));
            for (uint32_t i = 0; i < MaxDirectionalLights; ++i)
            {
                SCOPED_TRACE(i);
                ExpectVector(uploaded.directionalLights[i].direction, expected[3 + 2 * i]);
                ExpectVector(uploaded.directionalLights[i].colorAndIntensity, expected[4 + 2 * i]);
            }
            for (float value : uploaded.padding) EXPECT_FLOAT_EQ(value, 0.0f);
        }

        std::filesystem::path originalDirectory;
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        ComPtr<ID3D11InfoQueue> infoQueue;
        std::unique_ptr<OpaqueLayer> layer;
    };

    TEST_P(LightingGpuTest, PrepareUploadsAndBindsCurrentValuesAcrossFrames)
    {
        for (uint32_t count : {0u, 1u, MaxDirectionalLights, MaxDirectionalLights + 1, 1u})
        {
            SCOPED_TRACE(count);
            ASSERT_NO_FATAL_FAILURE(ExpectUploaded(MakeGpuFrame(count)));
        }
        ASSERT_NO_FATAL_FAILURE(ExpectUploaded(RenderFrameData{}));
    }

    TEST_P(LightingGpuTest, PixelShaderReadsActualBoundBuffer)
    {
        // Probe only transports b1 values to float pixels. It does not implement hemisphere lighting.
        const char* shader = R"(
struct Light { float4 direction; float4 colorAndIntensity; };
cbuffer LightingConstantBuffer : register(b1) {
    Light lights[CAPACITY];
    float4 sky;
    float4 ground;
    uint count;
    float3 padding;
};
float4 VS(uint id : SV_VertexID) : SV_POSITION {
    float2 p = id == 0 ? float2(-1,-1) : (id == 1 ? float2(-1,3) : float2(3,-1));
    return float4(p, 0, 1);
}
float4 PS(float4 position : SV_POSITION) : SV_TARGET {
    uint pixel = (uint)position.x;
    if (pixel == 0) return sky;
    if (pixel == 1) return ground;
    if (pixel == 2) return float4(count, padding);
    uint index = (pixel - 3) / 2;
    return ((pixel - 3) % 2) == 0 ? lights[index].direction : lights[index].colorAndIntensity;
})";
        const std::string capacity = std::to_string(MaxDirectionalLights);
        const D3D_SHADER_MACRO macros[] = {{"CAPACITY", capacity.c_str()}, {nullptr, nullptr}};
        ComPtr<ID3DBlob> vsCode, psCode, errors;
        HRESULT hr = D3DCompile(shader, std::strlen(shader), "LightingGpuProbe", macros, nullptr,
            "VS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &vsCode, &errors);
        ASSERT_TRUE(SUCCEEDED(hr)) << (errors ? static_cast<const char*>(errors->GetBufferPointer()) : "VS compile failed");
        errors.Reset();
        hr = D3DCompile(shader, std::strlen(shader), "LightingGpuProbe", macros, nullptr,
            "PS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &psCode, &errors);
        ASSERT_TRUE(SUCCEEDED(hr)) << (errors ? static_cast<const char*>(errors->GetBufferPointer()) : "PS compile failed");
        ComPtr<ID3D11VertexShader> vs;
        ComPtr<ID3D11PixelShader> ps;
        ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(vsCode->GetBufferPointer(), vsCode->GetBufferSize(), nullptr, &vs)));
        ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(psCode->GetBufferPointer(), psCode->GetBufferSize(), nullptr, &ps)));

        const UINT width = 3 + 2 * MaxDirectionalLights;
        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = width;
        textureDesc.Height = textureDesc.MipLevels = textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
        ComPtr<ID3D11Texture2D> target, staging;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&textureDesc, nullptr, &target)));
        ComPtr<ID3D11RenderTargetView> rtv;
        ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(target.Get(), nullptr, &rtv)));
        textureDesc.Usage = D3D11_USAGE_STAGING;
        textureDesc.BindFlags = 0;
        textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&textureDesc, nullptr, &staging)));
        D3D11_RASTERIZER_DESC rasterDesc{};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable = TRUE;
        ComPtr<ID3D11RasterizerState> raster;
        ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rasterDesc, &raster)));
        const D3D11_VIEWPORT viewport{0, 0, static_cast<float>(width), 1, 0, 1};

        const std::array<RenderFrameData, 3> frames = {
            MakeGpuFrame(MaxDirectionalLights + 1), MakeGpuFrame(1), RenderFrameData{}
        };
        for (const auto& frame : frames)
        {
            SCOPED_TRACE(frame.directionalLights.size());
            layer->Prepare(context.Get(), frame);
            // Keep b1 from the real Prepare call; only replace shaders for observation.
            context->VSSetShader(vs.Get(), nullptr, 0);
            context->PSSetShader(ps.Get(), nullptr, 0);
            context->IASetInputLayout(nullptr);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context->RSSetState(raster.Get());
            context->RSSetViewports(1, &viewport);
            context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
            const float sentinel[4] = {-99, -99, -99, -99};
            context->ClearRenderTargetView(rtv.Get(), sentinel);
            context->Draw(3, 0);
            context->OMSetRenderTargets(0, nullptr, nullptr);
            context->CopyResource(staging.Get(), target.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            ASSERT_TRUE(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
            std::vector<Vector4> pixels(width);
            std::memcpy(pixels.data(), mapped.pData, width * sizeof(Vector4));
            context->Unmap(staging.Get(), 0);
            const auto expected = ExpectedPixels(frame);
            for (UINT pixel = 0; pixel < width; ++pixel)
            {
                SCOPED_TRACE(pixel);
                ExpectVector(pixels[pixel], expected[pixel]);
            }
        }
    }

    TEST_P(LightingGpuTest, ProductionPixelShaderBlendsHemisphereAndAppliesAlbedoOnce)
    {
        // Supply controlled world normals; keep the production PS and Prepare's lighting binding.
        const char* source = R"(
cbuffer TestNormal : register(b0) { float4 normal; };
struct Output { float4 pos : SV_POSITION; float3 normal : NORMAL; float2 uv : TEXCOORD; float3 worldPosition : TEXCOORD1; };
Output VS(uint id : SV_VertexID) {
    Output result;
    float2 p = id == 0 ? float2(-1,-1) : (id == 1 ? float2(-1,3) : float2(3,-1));
    result.pos = float4(p, 0, 1);
    result.normal = normal.xyz;
    result.uv = float2(0.5, 0.5);
    result.worldPosition = float3(p, 0);
    return result;
})";
        ComPtr<ID3DBlob> code, errors;
        const HRESULT hr = D3DCompile(source, std::strlen(source), "HemisphereTestInput", nullptr,
            nullptr, "VS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &code, &errors);
        ASSERT_TRUE(SUCCEEDED(hr)) << (errors ? static_cast<const char*>(errors->GetBufferPointer()) : "VS compile failed");
        ComPtr<ID3D11VertexShader> vs;
        ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &vs)));

        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = sizeof(Vector4);
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        ComPtr<ID3D11Buffer> normalBuffer, materialBuffer;
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufferDesc, nullptr, &normalBuffer)));
        const Vector4 white(1, 1, 1, 1);
        D3D11_SUBRESOURCE_DATA whiteData{};
        whiteData.pSysMem = &white;
        whiteData.SysMemPitch = sizeof(white);
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufferDesc, &whiteData, &materialBuffer)));

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = textureDesc.Height = textureDesc.MipLevels = textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        ComPtr<ID3D11Texture2D> texture, target, staging;
        ComPtr<ID3D11ShaderResourceView> textureView;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&textureDesc, &whiteData, &texture)));
        ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(texture.Get(), nullptr, &textureView)));
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&textureDesc, nullptr, &target)));
        ComPtr<ID3D11RenderTargetView> rtv;
        ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(target.Get(), nullptr, &rtv)));
        textureDesc.Usage = D3D11_USAGE_STAGING;
        textureDesc.BindFlags = 0;
        textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&textureDesc, nullptr, &staging)));

        D3D11_RASTERIZER_DESC rasterDesc{};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable = TRUE;
        ComPtr<ID3D11RasterizerState> raster;
        ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rasterDesc, &raster)));
        const D3D11_VIEWPORT viewport{0, 0, 1, 1, 0, 1};

        // Explicit expected colors detect swapped endpoints, wrong up axis, scalar truncation,
        // missing normalization, stale frame data, and accidental clamping of HDR contributions.
        const Vector3 sky(0.2f, 0.6f, 2.0f), ground(1.0f, 0.2f, 0.4f);
        struct Case
        {
            const char* name;
            Vector4 normal;
            Vector3 sky, ground, expected;
            Vector4 baseColor{1, 1, 1, 1};
            Vector4 texel{1, 1, 1, 1};
            Vector3 directionalColor{};
            float directionalIntensity = 0;
            float expectedAlpha = 1;
            Vector3 lightEuler{-90, 0, 0};
        };
        const Case cases[] = {
            {"up", {0,0,1,0}, sky, ground, sky},
            {"down", {0,0,-1,0}, sky, ground, ground},
            {"right", {0,1,0,0}, sky, ground, {0.6f,0.4f,1.2f}},
            {"left", {0,-1,0,0}, sky, ground, {0.6f,0.4f,1.2f}},
            {"forward", {1,0,0,0}, sky, ground, {0.6f,0.4f,1.2f}},
            {"back", {-1,0,0,0}, sky, ground, {0.6f,0.4f,1.2f}},
            {"sloped", {0.6f,0,0.8f,0}, sky, ground, {0.28f,0.56f,1.84f}},
            {"nonunit sloped", {3,0,4,0}, sky, ground, {0.28f,0.56f,1.84f}},
            {"equal up", {0,0,1,0}, sky, sky, sky},
            {"equal side", {1,0,0,0}, sky, sky, sky},
            {"equal down", {0,0,-1,0}, sky, sky, sky},
            {"zero", {0,0,1,0}, {}, {}, {}},
            // These nonwhite inputs distinguish one albedo multiplication from two.
            {"colored material ambient", {0,0,1,0}, {0.4f,0.4f,0.4f}, {}, {0.2f,0.1f,0.3f},
                {0.5f,0.25f,0.75f,0.6f}, {1,1,1,1}, {}, 0, 0.6f},
            {"colored texture and material ambient", {0,0,1,0}, {0.4f,0.6f,0.8f}, {}, {0.1f,0.12f,0.16f},
                {0.5f,0.25f,0.8f,0.6f}, {0.5f,0.8f,0.25f,0.5f}, {}, 0, 0.3f},
            {"colored direct only", {0,0,1,0}, {}, {}, {0.3f,0.08f,0.16f},
                {0.5f,0.25f,0.8f,0.6f}, {0.5f,0.8f,0.25f,0.5f}, {0.6f,0.2f,0.4f}, 2, 0.3f},
            {"colored ambient plus direct", {0,0,1,0}, {0.4f,0.6f,0.8f}, {}, {0.4f,0.2f,0.32f},
                {0.5f,0.25f,0.8f,0.6f}, {0.5f,0.8f,0.25f,0.5f}, {0.6f,0.2f,0.4f}, 2, 0.3f},
            {"black material", {0,0,1,0}, sky, ground, {},
                {0,0,0,0.6f}, {0.5f,0.8f,0.25f,0.5f}, {0.6f,0.2f,0.4f}, 2, 0.3f},
            {"colored unlit", {0,0,1,0}, {}, {}, {},
                {0.5f,0.25f,0.8f,0.6f}, {0.5f,0.8f,0.25f,0.5f}, {}, 0, 0.3f},
            {"quaternion yaw lights negative Y normal", {0,-1,0,0}, {}, {}, {1,1,1},
                {1,1,1,1}, {1,1,1,1}, {1,1,1}, 1, 1, {0,90,0}},
            {"quaternion yaw rejects back face", {0,1,0,0}, {}, {}, {},
                {1,1,1,1}, {1,1,1,1}, {1,1,1}, 1, 1, {0,90,0}},
            {"quaternion positive pitch lights downward normal", {0,0,-1,0}, {}, {}, {1,1,1},
                {1,1,1,1}, {1,1,1,1}, {1,1,1}, 1, 1, {90,0,0}},
            {"quaternion roll preserves forward direction", {-1,0,0,0}, {}, {}, {1,1,1},
                {1,1,1,1}, {1,1,1,1}, {1,1,1}, 1, 1, {0,0,90}}
        };
        for (const auto& test : cases)
        {
            SCOPED_TRACE(test.name);
            RenderFrameData frame;
            frame.environmentLighting.skyContribution = test.sky;
            frame.environmentLighting.groundContribution = test.ground;
            if (test.directionalIntensity > 0)
            {
                // Feed the production shader from an actual Quaternion-backed light.
                Actor lightActor;
                auto* light = lightActor.AddComponent<DirectionalLightComponent>();
                lightActor.SetRotation(test.lightEuler);
                light->SetColor(test.directionalColor);
                light->SetIntensity(test.directionalIntensity);
                frame.directionalLights.push_back(light->BuildRenderData());
            }
            layer->Prepare(context.Get(), frame);
            context->VSSetShader(vs.Get(), nullptr, 0);
            context->UpdateSubresource(normalBuffer.Get(), 0, nullptr, &test.normal, 0, 0);
            context->UpdateSubresource(materialBuffer.Get(), 0, nullptr, &test.baseColor, 0, 0);
            context->UpdateSubresource(texture.Get(), 0, nullptr, &test.texel, sizeof(Vector4), 0);
            context->VSSetConstantBuffers(0, 1, normalBuffer.GetAddressOf());
            context->PSSetConstantBuffers(0, 1, materialBuffer.GetAddressOf());
            context->PSSetShaderResources(0, 1, textureView.GetAddressOf());
            context->IASetInputLayout(nullptr);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context->RSSetState(raster.Get());
            context->RSSetViewports(1, &viewport);
            context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
            const float sentinel[4] = {-99, -99, -99, -99};
            context->ClearRenderTargetView(rtv.Get(), sentinel);
            context->Draw(3, 0);
            context->OMSetRenderTargets(0, nullptr, nullptr);
            context->CopyResource(staging.Get(), target.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            ASSERT_TRUE(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
            Vector4 actual;
            std::memcpy(&actual, mapped.pData, sizeof(actual));
            context->Unmap(staging.Get(), 0);
            EXPECT_NEAR(actual.x, test.expected.x, 0.00001f);
            EXPECT_NEAR(actual.y, test.expected.y, 0.00001f);
            EXPECT_NEAR(actual.z, test.expected.z, 0.00001f);
            EXPECT_NEAR(actual.w, test.expectedAlpha, 0.00001f);
        }
    }

    TEST_P(LightingGpuTest, ProductionShadersReceiveObjectAndCameraDataAndSkipSingularDraws)
    {
        // Observe the real VS output; Execute still runs the production upload and Draw paths.
        const char* probe = R"(
struct Input { float4 pos : SV_POSITION; float3 normal : NORMAL; float2 uv : TEXCOORD; };
float4 PS(Input input) : SV_TARGET { return float4(input.normal, 1); }
)";
        ComPtr<ID3DBlob> code, errors;
        ASSERT_TRUE(SUCCEEDED(D3DCompile(probe, std::strlen(probe), "NormalProbe", nullptr,
            nullptr, "PS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &code, &errors)))
            << (errors ? static_cast<const char*>(errors->GetBufferPointer()) : "compile failed");
        ComPtr<ID3D11PixelShader> probePS;
        ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(),
            nullptr, &probePS)));

        // Reuse the production PS declarations so semantic/register mismatches fail here.
        // Its normal entry point does not consume positions yet and may optimize them away.
        const auto sourcePath = ExecutableDirectory() / "../../../Engine/Shader/MeshPixelShader.hlsl";
        std::ifstream sourceFile(sourcePath);
        ASSERT_TRUE(sourceFile.is_open());
        std::string positionSource((std::istreambuf_iterator<char>(sourceFile)),
            std::istreambuf_iterator<char>());
        positionSource += R"(
float4 WorldProbe(VSOutput input) : SV_TARGET { return float4(input.worldPosition, 1); }
float4 CameraProbe(VSOutput input) : SV_TARGET { return gCameraPosition; }
)";
        ComPtr<ID3D11PixelShader> positionShaders[2];
        const char* entries[] = {"WorldProbe", "CameraProbe"};
        for (int i = 0; i < 2; ++i)
        {
            ComPtr<ID3DBlob> positionCode, positionErrors;
            ASSERT_TRUE(SUCCEEDED(D3DCompile(positionSource.data(), positionSource.size(),
                "ProductionPositionProbe", nullptr, nullptr, entries[i], "ps_5_0",
                D3DCOMPILE_ENABLE_STRICTNESS, 0, &positionCode, &positionErrors)))
                << (positionErrors ? static_cast<const char*>(positionErrors->GetBufferPointer()) : "compile failed");
            ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(positionCode->GetBufferPointer(),
                positionCode->GetBufferSize(), nullptr, &positionShaders[i])));
        }

        struct Vertex { float position[3]; float normal[3]; float uv[2]; };
        const Vertex vertices[] = {
            {{-1,-1,0.5f}, {1,1,1}, {0,0}},
            {{-1, 3,0.5f}, {1,1,1}, {0,0}},
            {{ 3,-1,0.5f}, {1,1,1}, {0,0}}
        };
        const uint32_t indices[] = {0,1,2};
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = sizeof(vertices);
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = vertices;
        ComPtr<ID3D11Buffer> vb, ib;
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufferDesc, &data, &vb)));
        bufferDesc.ByteWidth = sizeof(indices);
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        data.pSysMem = indices;
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufferDesc, &data, &ib)));
        const std::unordered_map<BufferHandle, ComPtr<ID3D11Buffer>> buffers = {{1, vb}, {2, ib}};
        const std::unordered_map<TextureHandle, ComPtr<ID3D11ShaderResourceView>> textures;

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = textureDesc.Height = textureDesc.MipLevels = textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
        ComPtr<ID3D11Texture2D> target, staging;
        ComPtr<ID3D11RenderTargetView> rtv;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&textureDesc, nullptr, &target)));
        ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(target.Get(), nullptr, &rtv)));
        textureDesc.Usage = D3D11_USAGE_STAGING;
        textureDesc.BindFlags = 0;
        textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&textureDesc, nullptr, &staging)));
        D3D11_RASTERIZER_DESC rasterDesc{};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable = TRUE;
        ComPtr<ID3D11RasterizerState> raster;
        ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rasterDesc, &raster)));
        const D3D11_VIEWPORT viewport{0,0,1,1,0,1};
        D3D11_QUERY_DESC queryDesc{D3D11_QUERY_PIPELINE_STATISTICS, 0};
        ComPtr<ID3D11Query> query;
        ASSERT_TRUE(SUCCEEDED(device->CreateQuery(&queryDesc, &query)));

        const Matrix4 scale = Matrix4::Scale(2,3,4);
        const Matrix4 undoScale = Matrix4::Scale(0.5f,1.0f/3,0.25f);
        const Matrix4 rotation = Matrix4::RotationZ(1.57079632679f);
        const Matrix4 undoRotation = Matrix4::RotationZ(-1.57079632679f);
        struct Case { const char* name; Matrix4 world; Matrix4 view; Vector3 normal; bool draws; Vector3 worldPosition; };
        // View cancels the known transform to keep coverage fixed. Expected normals are analytic.
        const Case cases[] = {
            {"identity", Matrix4::identity, Matrix4::identity, {1,1,1}, true, {0,0,0.5f}},
            {"uniform", Matrix4::Scale(3), Matrix4::Scale(1.0f/3), {1,1,1}, true, {0,0,1.5f}},
            {"nonuniform", scale, undoScale, {0.5f,1.0f/3,0.25f}, true, {0,0,2}},
            {"rotated", scale * rotation, undoRotation * undoScale, {-1.0f/3,0.5f,0.25f}, true, {0,0,2}},
            {"translated", scale * rotation * Matrix4::Translation(5,6,7),
                Matrix4::Translation(-5,-6,-7) * undoRotation * undoScale,
                {-1.0f/3,0.5f,0.25f}, true, {5,6,9}},
            {"mirrored", Matrix4::Scale(-2,3,4), Matrix4::Scale(-0.5f,1.0f/3,0.25f),
                {-0.5f,1.0f/3,0.25f}, true, {0,0,2}},
            {"zero X", Matrix4::Scale(0,2,3), Matrix4::identity, {}, false},
            {"zero Y", Matrix4::Scale(2,0,3), Matrix4::identity, {}, false},
            {"zero Z", Matrix4::Scale(2,3,0), Matrix4::identity, {}, false},
            {"valid after failures", Matrix4::identity, Matrix4::identity, {1,1,1}, true, {0,0,0.5f}}
        };
        for (const Case& test : cases)
        {
            SCOPED_TRACE(test.name);
            RenderFrameData positionFrame;
            positionFrame.cameraPosition = test.worldPosition + Vector3(11, -13, 17);
            layer->Prepare(context.Get(), positionFrame);
            context->PSSetShader(probePS.Get(), nullptr, 0);
            context->RSSetState(raster.Get());
            context->RSSetViewports(1, &viewport);
            context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
            const float sentinel[4] = {-99,-99,-99,-99};
            context->ClearRenderTargetView(rtv.Get(), sentinel);
            RenderCommand command;
            command.vertexBuffer = 1;
            command.indexBuffer = 2;
            command.indexCount = 3;
            command.stride = sizeof(Vertex);
            command.worldMatrix = test.world;
            command.viewMatrix = test.view;
            context->Begin(query.Get());
            layer->Execute(context.Get(), {command}, buffers, textures);
            context->End(query.Get());
            context->OMSetRenderTargets(0, nullptr, nullptr);
            context->CopyResource(staging.Get(), target.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            ASSERT_TRUE(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
            Vector4 actual;
            std::memcpy(&actual, mapped.pData, sizeof(actual));
            context->Unmap(staging.Get(), 0);
            D3D11_QUERY_DATA_PIPELINE_STATISTICS stats{};
            HRESULT queryResult = S_FALSE;
            const ULONGLONG deadline = GetTickCount64() + 5000;
            do {
                queryResult = context->GetData(query.Get(), &stats, sizeof(stats), 0);
                if (queryResult == S_FALSE) Sleep(1);
            } while (queryResult == S_FALSE && GetTickCount64() < deadline);
            ASSERT_EQ(queryResult, S_OK);
            if (!test.draws)
            {
                EXPECT_EQ(stats.VSInvocations, 0u);
                ExpectVector(actual, Vector4(-99,-99,-99,-99));
                continue;
            }
            EXPECT_GT(stats.VSInvocations, 0u);
            const float length = std::sqrt(test.normal.x * test.normal.x +
                test.normal.y * test.normal.y + test.normal.z * test.normal.z);
            EXPECT_NEAR(actual.x, test.normal.x / length, 0.00001f);
            EXPECT_NEAR(actual.y, test.normal.y / length, 0.00001f);
            EXPECT_NEAR(actual.z, test.normal.z / length, 0.00001f);
            EXPECT_FLOAT_EQ(actual.w, 1);

            const Vector4 expectedPositions[] = {
                Vector4(test.worldPosition, 1),
                Vector4(positionFrame.cameraPosition, 1)
            };
            for (int i = 0; i < 2; ++i)
            {
                SCOPED_TRACE(entries[i]);
                context->PSSetShader(positionShaders[i].Get(), nullptr, 0);
                context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
                context->ClearRenderTargetView(rtv.Get(), sentinel);
                layer->Execute(context.Get(), {command}, buffers, textures);
                context->OMSetRenderTargets(0, nullptr, nullptr);
                context->CopyResource(staging.Get(), target.Get());
                ASSERT_TRUE(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
                std::memcpy(&actual, mapped.pData, sizeof(actual));
                context->Unmap(staging.Get(), 0);
                EXPECT_NEAR(actual.x, expectedPositions[i].x, 0.00001f);
                EXPECT_NEAR(actual.y, expectedPositions[i].y, 0.00001f);
                EXPECT_NEAR(actual.z, expectedPositions[i].z, 0.00001f);
                EXPECT_FLOAT_EQ(actual.w, expectedPositions[i].w);
            }
        }
    }

    INSTANTIATE_TEST_SUITE_P(D3D11, LightingGpuTest,
        ::testing::Values(D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP),
        [](const ::testing::TestParamInfo<D3D_DRIVER_TYPE>& info) {
            return info.param == D3D_DRIVER_TYPE_HARDWARE ? "Hardware" : "Warp";
        });
}

TEST(LightingShaderLayoutTest, CompiledProductionShaderMatchesCpuLayoutAndBinding)
{
    ComPtr<ID3DBlob> bytecode;
    const auto path = ExecutableDirectory() / "Shader" / "MeshPixelShader.cso";
    ASSERT_TRUE(SUCCEEDED(D3DReadFileToBlob(path.c_str(), &bytecode)));
    ComPtr<ID3D11ShaderReflection> reflection;
    ASSERT_TRUE(SUCCEEDED(D3DReflect(bytecode->GetBufferPointer(), bytecode->GetBufferSize(),
        IID_ID3D11ShaderReflection, reinterpret_cast<void**>(reflection.GetAddressOf()))));
    auto* buffer = reflection->GetConstantBufferByName("LightingConstantBuffer");
    D3D11_SHADER_BUFFER_DESC bufferDesc{};
    ASSERT_TRUE(SUCCEEDED(buffer->GetDesc(&bufferDesc)));
    EXPECT_EQ(bufferDesc.Size, sizeof(Engine::LightingConstantBuffer));
    const std::pair<const char*, UINT> fields[] = {
        {"gDirectionalLights", offsetof(Engine::LightingConstantBuffer, directionalLights)},
        {"gSkyContribution", offsetof(Engine::LightingConstantBuffer, skyContribution)},
        {"gGroundContribution", offsetof(Engine::LightingConstantBuffer, groundContribution)},
        {"gDirectionalLightCount", offsetof(Engine::LightingConstantBuffer, directionalLightCount)},
        {"gLightingPadding", offsetof(Engine::LightingConstantBuffer, padding)}
    };
    for (const auto& [name, offset] : fields)
    {
        SCOPED_TRACE(name);
        D3D11_SHADER_VARIABLE_DESC variable{};
        ASSERT_TRUE(SUCCEEDED(buffer->GetVariableByName(name)->GetDesc(&variable)));
        EXPECT_EQ(variable.StartOffset, offset);
    }
    D3D11_SHADER_TYPE_DESC lights{};
    ASSERT_TRUE(SUCCEEDED(buffer->GetVariableByName("gDirectionalLights")->GetType()->GetDesc(&lights)));
    EXPECT_EQ(lights.Elements, Engine::MaxDirectionalLights);
    D3D11_SHADER_INPUT_BIND_DESC binding{};
    ASSERT_TRUE(SUCCEEDED(reflection->GetResourceBindingDescByName("LightingConstantBuffer", &binding)));
    EXPECT_EQ(binding.Type, D3D_SIT_CBUFFER);
    EXPECT_EQ(binding.BindPoint, 1u);
}

TEST(LightingShaderLayoutTest, ObjectMatricesMatchCpuSizeRowMajorLayoutAndBinding)
{
    ComPtr<ID3DBlob> bytecode;
    const auto path = ExecutableDirectory() / "Shader" / "MeshVertexShader.cso";
    ASSERT_TRUE(SUCCEEDED(D3DReadFileToBlob(path.c_str(), &bytecode)));
    ComPtr<ID3D11ShaderReflection> reflection;
    ASSERT_TRUE(SUCCEEDED(D3DReflect(bytecode->GetBufferPointer(), bytecode->GetBufferSize(),
        IID_ID3D11ShaderReflection, reinterpret_cast<void**>(reflection.GetAddressOf()))));
    auto* buffer = reflection->GetConstantBufferByName("ObjectConstant");
    D3D11_SHADER_BUFFER_DESC bufferDesc{};
    ASSERT_TRUE(SUCCEEDED(buffer->GetDesc(&bufferDesc)));
    EXPECT_EQ(bufferDesc.Size, 2 * sizeof(Engine::Matrix4));
    const std::pair<const char*, UINT> matrices[] = {{"world", 0}, {"normal", 64}};
    for (const auto& [name, offset] : matrices)
    {
        auto* variable = buffer->GetVariableByName(name);
        D3D11_SHADER_VARIABLE_DESC variableDesc{};
        ASSERT_TRUE(SUCCEEDED(variable->GetDesc(&variableDesc)));
        EXPECT_EQ(variableDesc.StartOffset, offset);
        EXPECT_EQ(variableDesc.Size, sizeof(Engine::Matrix4));
        D3D11_SHADER_TYPE_DESC type{};
        ASSERT_TRUE(SUCCEEDED(variable->GetType()->GetDesc(&type)));
        EXPECT_EQ(type.Class, D3D_SVC_MATRIX_ROWS);
        EXPECT_EQ(type.Rows, 4u);
        EXPECT_EQ(type.Columns, 4u);
    }
    D3D11_SHADER_INPUT_BIND_DESC binding{};
    ASSERT_TRUE(SUCCEEDED(reflection->GetResourceBindingDescByName("ObjectConstant", &binding)));
    EXPECT_EQ(binding.Type, D3D_SIT_CBUFFER);
    EXPECT_EQ(binding.BindPoint, 1u);
}
