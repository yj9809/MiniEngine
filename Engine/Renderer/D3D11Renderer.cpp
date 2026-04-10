#include "D3D11Renderer.h"

#include <d3d11.h>

#include "Common/Common.h"

namespace Engine
{
	bool D3D11Renderer::GPUInit(HWND hwnd, int width, int height)
	{
		UINT debugFlag = 0;

#ifdef _DEBUG
		debugFlag = D3D11_CREATE_DEVICE_DEBUG;
#endif

		// Step.1 Device + Device Context 생성.
		HRESULT hr = D3D11CreateDevice(
			nullptr,					// 어떤 GPU를 쓸지.
			D3D_DRIVER_TYPE_HARDWARE,	// 하드웨어 GPU를 사용 (Direct3D 애플리케이션에서 사용해야하는 기본 드라이버).
			nullptr,					// 드라이버 타입이 hardware일 경우 nullptr
			debugFlag,					// 디버그 플래그 (Debug 빌드 시 디버그 계층을 지원하는 디바이스 생성).
			nullptr,					// 지원할 DX 버전 목록 (nullptr일 경우 자동).
			0,							// 버전 목록 개수.
			D3D11_SDK_VERSION,			// SDK 버전 (항상 이 값을 사용).
			&device,						// device 반환.
			nullptr,						// D3D_FEATURE_LEVEL 값을 반환 (필요 없으면 null).
			&context						// DeviceContext 반환.
		);

		FAILCHECK(hr, L"Failed create D3D11 Device");

		// Step.2 DXGI Factory 꺼내기.
		// Device에서 IDXGIDevice 꺼내기.
		// 같은 GPU를 공유하는 것을 보장하기 위해 새로 만들지 않고 꺼내옴.
		ComPtr<IDXGIDevice> dxgiDevice;
		hr = device->QueryInterface(__uuidof(IDXGIDevice), &dxgiDevice);
		FAILCHECK(hr, L"Failed dxgi device");

		// IDXGIDevice ->IDXGIAdapter (실제 GPU 어댑터).
		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		FAILCHECK(hr, L"Failed get dxgi adapter");

		// DX11.1부터 CreateSwapChain 방식을 사용하지 않는 관계로 CreateSwapChainForHwnd를 사용할 예정.
		// IDXGFactory도 2버전으로 사용.
		// 
		// IDXGIAdapter -> IDXGIFactory (Swapchain을 얻어오기 위한 Factory).
		// ComPtr<IDXGIFactory> dxgiFactory;
		// hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), &dxgiFactory);
		// FAILCHECK(hr, L"Failed get dxgi factory");

		// Step.3 SwapChain 생성.
		//DXGI_SWAP_CHAIN_DESC scDesc = {};
		//scDesc.BufferCount = 1;									// back buffer 개수.
		//scDesc.BufferDesc.Width = width;							// 가로 사이즈.
		//scDesc.BufferDesc.Height = height;						// 세로 사이즈.
		//scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// 픽셀 포맷: RGBA 각각 8bit.
		//scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;		// Render Target으로 사용.
		//scDesc.OutputWindow = hwnd;								// 출력 창 설정.
		//scDesc.SampleDesc.Count = 1;								// 멀티 샘플링 비활성화.
		//scDesc.Windowed = TRUE;									// 창 모드.

		//hr = dxgiFactory->CreateSwapChain(device.Get(), &scDesc, &swapChain);

		ComPtr<IDXGIFactory2> dxgiFactpory;
		hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactpory);
		FAILCHECK(hr, L"Failed get dxgi factory");

		// Step.3 SwapChain 생성.
		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		scDesc.Width = width;									// 가로 사이즈.
		scDesc.Height = height;									// 세로 사이즈.
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// 포맷 설정: RGBA 8bit.
		scDesc.BufferCount = 2;									// front buffer까지 포함한 개수.
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// Render Target으로 사용.
		// Flip 방식으로 스왑 후 back buffer 삭제 
		// 포인터를 사용한 뒤집기 방식이라 비용이 적음.
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.SampleDesc.Count = 1;									// 멀티 샘플링 비활성화.

		hr = dxgiFactpory->CreateSwapChainForHwnd(
			device.Get(),	//device.
			hwnd,			// 출력 창.
			&scDesc,
			nullptr,		// 창 모드 설정 (전체화면의 경우 DXGI_SWAP_CHAIN_FULLSCREEN_DESC 설정해야함).
			nullptr,		// 출력 모니터 지정 (nullptr일 경우 기본).
			&swapChain		// IDXGISwapChain1 반환.
		);
		FAILCHECK(hr, L"Failed create swap chain");

		// Step.4 Render Target View 생성.
		// SwapChain의 back buffer 텍스처를 꺼내오기.
		// 0 = 첫 번째 back buffer.
		ComPtr<ID3D11Texture2D> backBuffer;
		hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
		FAILCHECK(hr, L"Failed get back buffer");

		// back buffer 텍스처를 Render Target View로 감싸기.
		// Device한테 Render Target View 알림.
		hr = device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView);
		FAILCHECK(hr, L"Failed create render target view");

		// Render Target View를 Output Merger 단계(그래픽스 파이프라인의 마지막 단계)에 바인딩.
		// 이후 모든 Draw 명령이 이 back buffer에 그려짐.
		context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), nullptr);

		// Step.5 ViewPort 설정.
		// GPU에 픽셀 좌표를 해당 범위에 매핑하도록 알림.
		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = static_cast<float>(width);
		vp.Height = static_cast<float>(height);
		// Depth는 0~1 사이 범위.
		vp.MinDepth = 0.0f; // 깊이 범위 최솟값 (항상 0).
		vp.MaxDepth = 1.0f; // 깊이 범위 최댓값 (항상 1).
		// 그래픽스 파이프라인 Rasterizer 단계를 진행할 ViewPort 설정.
		context->RSSetViewports(1, &vp);

		return true;
	}

	void D3D11Renderer::BeginFrame(float r, float g, float b)
	{
		// 매 프레임 시작 시 back buffer를 지정한 색으로 초기화.
		// 안 지우면 이전 프레임 내용이 남아있음.
		float color[4] = { r, g, b, 1.0f };
		context->ClearRenderTargetView(renderTargetView.Get(), color);
	}

	void D3D11Renderer::EndFrame()
	{
		// back buffer -> front buffer 교체 (모니터 출력).
		// 1 = VSync 활성화 (모니터 주사율 동기화).
		// 0 = VSync 비활성화.
		swapChain->Present(1, 0);
	}

	void D3D11Renderer::GPUShutdown()
	{
		// 생성 역순으로 해제.
		// ComPtr이라 스마트 포인터로 자동 해제가 되지만 명시적 해제가 필요할 경우 사용.
		renderTargetView.Reset();
		swapChain.Reset();
		context.Reset();
		device.Reset();
	}
}
