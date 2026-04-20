#include "D3D11Renderer.h"

#include <d3d11.h>

#include "Common/Common.h"
#include "Math/Matrix4.h"

namespace Engine
{
	bool D3D11Renderer::GPUInit(HWND hwnd, int width, int height)
	{		
		// Step.1 Device + DeviceContext 생성.
		if (!InitDevice())
		{
			return false;
		}
		
		// Step.2 SwapChain 생성.
		if (!InitSwapChain(hwnd, width, height))
		{
			return false;
		}
		
		// Step.3 RenderTargetView 생성.
		if (!InitRenderTargetView())
		{
			return false;
		}
		
		// Step.4 Viewport 설정.
		InitViewport(width, height);

		// Step.5 상수 버퍼 생성.
		if (!CreateConstantBuffer(sizeof(Matrix4), wvpConstantBuffer))
		{
			return false;
		}
		
		// Step.6 Shader 생성.
		if (!InitShaders())
		{
			return false;
		}
		
		return true;
	}

	bool D3D11Renderer::InitDevice()
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

		FAILCHECK(hr, L"Failed create D3D11 Device", false)

		return true;
	}

	bool D3D11Renderer::InitSwapChain(HWND hwnd, int width, int height)
	{
		// Step.2 DXGI Factory 꺼내기.
		// Device에서 IDXGIDevice 꺼내기.
		// 같은 GPU를 공유하는 것을 보장하기 위해 새로 만들지 않고 꺼내옴.
		ComPtr<IDXGIDevice> dxgiDevice;
		HRESULT hr = device->QueryInterface(__uuidof(IDXGIDevice), &dxgiDevice);
		FAILCHECK(hr, L"Failed dxgi device", false);

		// IDXGIDevice ->IDXGIAdapter (실제 GPU 어댑터).
		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		FAILCHECK(hr, L"Failed get dxgi adapter", false)

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
		FAILCHECK(hr, L"Failed get dxgi factory", false)

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
		FAILCHECK(hr, L"Failed create swap chain", false)
		
		return true;
	}

	bool D3D11Renderer::InitRenderTargetView()
	{
		// Step.4 Render Target View 생성.
		// SwapChain의 back buffer 텍스처를 꺼내오기.
		// 0 = 첫 번째 back buffer.
		ComPtr<ID3D11Texture2D> backBuffer;
		HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
		FAILCHECK(hr, L"Failed get back buffer", false)

		// back buffer 텍스처를 Render Target View로 감싸기.
		// Device한테 Render Target View 알림.
		hr = device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView);
		FAILCHECK(hr, L"Failed create render target view", false)

		// Render Target View를 Output Merger 단계(그래픽스 파이프라인의 마지막 단계)에 바인딩.
		// 이후 모든 Draw 명령이 이 back buffer에 그려짐.
		context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), nullptr);
		
		return true;
	}

	void D3D11Renderer::InitViewport(int width, int height) const
	{
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
	}

	bool D3D11Renderer::CreateConstantBuffer(UINT byteWidth, ComPtr<ID3D11Buffer>& buffer) const
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

	bool D3D11Renderer::InitShaders()
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
		hr = device->CreateVertexShader(vsBlod->GetBufferPointer(), vsBlod->GetBufferSize(), nullptr, &vertexShader);
		FAILCHECK(hr, L"Failed to create vertex shader", false)
		hr = device->CreatePixelShader(psBlod->GetBufferPointer(), psBlod->GetBufferSize(), nullptr, &pixelShader);
		FAILCHECK(hr, L"Failed to create pixel shader", false)

		// Input Layout 생성.
		D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
		{
			{"POSITION", 
				0, 
				DXGI_FORMAT_R32G32B32_FLOAT, 
				0, 
				0, 
				D3D11_INPUT_PER_VERTEX_DATA, 
				0
			},
			{ "COLOR",
				0,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				0,
				D3D11_APPEND_ALIGNED_ELEMENT,
				D3D11_INPUT_PER_VERTEX_DATA,
				0
			}
		};
		// Input 레이아웃 추가시 NumElements 값도 추가해주어야 한다.
		hr = device->CreateInputLayout(layoutDesc, 2, vsBlod->GetBufferPointer(), vsBlod->GetBufferSize(), &inputLayout);
		FAILCHECK(hr, L"Failed to create input layout", false)

		return true;
	}

	BufferHandle D3D11Renderer::CreateVertexBuffer(const void* vertexData, UINT vertexDataSize)
	{
		// 정점 버퍼 생성을 위한 설명서.
		D3D11_BUFFER_DESC vbDesc = {};
		vbDesc.ByteWidth = vertexDataSize;
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		
		// 정점 버퍼에 들어갈 데이터 설명서.
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = vertexData;
		
		ComPtr<ID3D11Buffer> vertexBuffer;
		HRESULT hr = device->CreateBuffer(&vbDesc, &initData, &vertexBuffer);
		FAILCHECK(hr, L"Failed to create vertex buffer", NULL_BUFFER)
		
		BufferHandle count = nextBufferHandle++;
		bufferMap.emplace(count, vertexBuffer);
		return count;
	}

	BufferHandle D3D11Renderer::CreateIndexBuffer(const void* indexData, UINT indexDataSize)
	{
		// 인덱스 버퍼 생성을 위한 설명서.
		D3D11_BUFFER_DESC ibDesc = {};
		ibDesc.ByteWidth = indexDataSize;
		ibDesc.Usage = D3D11_USAGE_DEFAULT;
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		
		// 인덱스 버퍼에 들어갈 데이터 설명서.
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = indexData;
		
		ComPtr<ID3D11Buffer> indexBuffer;
		HRESULT hr = device->CreateBuffer(&ibDesc, &initData, &indexBuffer);
		FAILCHECK(hr, L"Failed to create index buffer", NULL_BUFFER)
		
		BufferHandle count = nextBufferHandle++;
		bufferMap.emplace(count, indexBuffer);
		return count;
	}

	void D3D11Renderer::BeginFrame(float r, float g, float b)
	{
		// FILP_DISCARD 방식은 Present() 후 렌더 타겟 바인딩이 해제되므로 매 프레임 재바인딩 필요.
		context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), nullptr);

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
		bufferMap.clear();
		wvpConstantBuffer.Reset();
		renderTargetView.Reset();
		swapChain.Reset();
		context.Reset();
		device.Reset();
	}
	
	void D3D11Renderer::Render()
	{
		// 셰이더에 버퍼 등록.
		context->VSSetConstantBuffers(0, 1, wvpConstantBuffer.GetAddressOf());
		
		// InputLayout 등록.
		context->IASetInputLayout(inputLayout.Get());

		// 셰이더 바인딩: 이후 Draw 호출에서 이 셰이더로 처리.
		context->VSSetShader(vertexShader.Get(), nullptr, 0);
		context->PSSetShader(pixelShader.Get(), nullptr, 0);
				
		UINT offset = 0;
		
		// RenderCommand 리스트를 순회하며 Draw를 실행.
		for (const RenderCommand& command : renderCommands)
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
		
			FAILCHECK(hr, L"Failed to map resource", )
		
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
		
		renderCommands.clear();
	}

	void D3D11Renderer::Submit(const RenderCommand& command)
	{
		renderCommands.emplace_back(command);
	}
}
