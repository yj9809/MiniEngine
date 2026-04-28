#pragma once

#include "IRenderer.h"
#include "Common/Common.h"
#include "RenderLayer/LayerScheduler.h"

#include <d3d11.h>
#include <dxgi1_2.h>

#include <vector>
#include <unordered_map>
#include <wrl/client.h>

using namespace Microsoft::WRL;

// 기본적인 DirectX11 초기화 순서.
// 1. Device와 SwapChain 생성.
// 2. backbuffer에 실제 렌더링할 렌더 타겟 뷰 생성.
// 3. 뷰포트 생성.
// 4. 매 프레임마다 위에서 생성한 렌더 타겟 뷰에 게임 화면을 렌더링.
// 5. SwapChain을 이용하여 디바이스에 화면을 그린다.
// 제일 중요한 작업은 Render Target View를 생성하는 작업.

namespace Engine
{
	class ENGINE_API D3D11Renderer : public IRenderer
	{
	public:
		D3D11Renderer() = default;
		~D3D11Renderer() = default;

		virtual bool GPUInit(HWND hwnd, int width, int height) override;
		
		virtual void BeginFrame(float r, float g, float b) override;

		virtual void EndFrame() override;

		virtual void GPUShutdown() override;

		virtual void Render() override;
			
		virtual void Submit(const RenderCommand& command) override;
		
		// 외부에서 정점/인덱스 버퍼를 생성하기 위한 함수.
		virtual BufferHandle CreateVertexBuffer(const void* vertexData, UINT vertexDataSize) override;
		virtual BufferHandle CreateIndexBuffer(const void* indexData, UINT indexDataSize) override;
		
		// 버퍼를 반환하기 위한 함수.
		virtual void ReleaseBuffer(BufferHandle buffer) override;

	private:
		// Step.1 Device + DeviceContext 생성.
		bool InitDevice();
		
		// Step.2 SwapChain 생성.
		bool InitSwapChain(HWND hwnd, int width, int height);
		
		// Step.3 Render Target View 생성.
		bool InitRenderTargetView();
		
		// Step.4 Depth Stencil View 생성.
		bool InitDepthStencilView(int width, int height);
		
		// Step.5 Viewport 설정.
		void InitViewport(int width, int height) const;
		
	private:
		// GPU와 연결되는 기본 디바이스 객체.
		ComPtr<ID3D11Device> device;

		// GPU 파이프라인에 명령을 내리는 객체.
		// Device로 만든 자원을 세팅하고 Draw 명령을 실행.
		ComPtr<ID3D11DeviceContext> context;

		// backbuffer와 frontbuffer를 스왑하여 화면에 렌더링하는 역할을 하는 객체.
		ComPtr<IDXGISwapChain1> swapChain;

		// Render Target View 생성.
		ComPtr<ID3D11RenderTargetView> renderTargetView;
		
		// Rendering 목록을 담당하는 컨테이너.
		std::vector<RenderCommand> renderCommands;
		
		// 버퍼를 담아놓는 컨테이너.
		std::unordered_map<BufferHandle, ComPtr<ID3D11Buffer>> bufferMap;
		
		// 버퍼 핸들 생성용 카운터.
		BufferHandle nextBufferHandle = 1;
		
		// Render Layer 스케줄러.
		LayerScheduler layerScheduler;
		
		// Depth Stencil View를 구현하기 위한 멤버 변수.
		ComPtr<ID3D11Texture2D> depthStencilTexture;
		ComPtr<ID3D11DepthStencilView> depthStencilView;
		ComPtr<ID3D11DepthStencilState> depthStencilState;
	};
}

