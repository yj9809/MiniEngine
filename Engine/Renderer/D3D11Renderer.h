#pragma once

#include "IRenderer.h"
#include "Common/Common.h"

#include <d3d11.h>
#include <dxgi1_2.h>
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
	};
}

