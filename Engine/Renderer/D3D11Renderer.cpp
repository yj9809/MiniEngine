#include "D3D11Renderer.h"

#include <d3d11.h>
#include <wincodec.h>

#include "Common/Common.h"
#include "RenderLayer/OpaqueLayer.h"
#include "RenderLayer/RenderLayerType.h"
#include "RenderLayer/WireframeLayer.h"

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
		
		// Step.4 Depth Stencil View 생성.
		if (!InitDepthStencilView(width, height))
		{
			return false;
		}
		
		// Step.5 Viewport 설정.
		InitViewport(width, height);
		
		// Step.6 PassScheduler 설정.
		layerScheduler.RegisterPassScheduler(
			RenderLayerType::Opaque, std::make_unique<OpaqueLayer>(device.Get())
		);
		layerScheduler.RegisterPassScheduler(
			RenderLayerType::Wireframe, std::make_unique<WireframeLayer>(device.Get())
		);
		
		// WIC 쓰기 전 COM 초기화를 한 번 진행해줘야 한다.
		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		
		FAILCHECK(hr, L"Failed COM initialization", false)
		
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

		return true;
	}

	bool D3D11Renderer::InitDepthStencilView(int width, int height)
	{
		// Step.1 Texture2D 생성.
		// RTV와는 다르게 백 버퍼에서 받아오지 않고 새로 만들어야 한다.
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1; // 깊이 버퍼는 MipMap 불필요.
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_D32_FLOAT; // 32bit 부동소수점 깊이 버퍼.
		textureDesc.SampleDesc.Count = 1; // 스왑체인과 일치.
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL; // 깊이-스텐실 버퍼로 바인딩.
		
		HRESULT hr = device->CreateTexture2D(&textureDesc, nullptr, &depthStencilTexture);
		FAILCHECK(hr, L"Failed create depth stencil Texture", false)
		
		// Step.2 Depth Stencil View 생성.
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = textureDesc.Format; // Texture2D 포맷과 일치.
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; // Texture2D로 액세스.
		
		hr = device->CreateDepthStencilView(depthStencilTexture.Get(), &dsvDesc, &depthStencilView);
		FAILCHECK(hr, L"Failed create depth stencil view", false)
		
		// Step.3 Depth Stencil State 생성.
		D3D11_DEPTH_STENCIL_DESC dsDesc = {};
		dsDesc.DepthEnable = true; // 깊이 테스트 활성화.
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 깊이 버퍼에 쓰기 켜기.
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS; // 더 가까운 픽셀만 렌더링.
		dsDesc.StencilEnable = false; // 스텐실 테스트 비활성화.
		
		hr = device->CreateDepthStencilState(&dsDesc, &depthStencilState);
		FAILCHECK(hr, L"Failed create depth stencil state", false)
		
		// 깊이-스텐실 상태 설정.
		context->OMSetDepthStencilState(depthStencilState.Get(), 0); 
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

	void D3D11Renderer::ReleaseBuffer(BufferHandle buffer)
	{
		// ComPtr이기 때문에 Map에서 삭제만 해주면 된다.
		bufferMap.erase(buffer);
	}

	TextureHandle D3D11Renderer::CreateTexture(const wchar_t* fileName)
	{
		// 1. WIC Imaging Factory 생성.
		
		// WIC Factory 생성.
		ComPtr<IWICImagingFactory> wicFactory;
		HRESULT hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&wicFactory)
		);
		FAILCHECK(hr, L"Failed to create WIC Imaging Factory", NULL_TEXTURE)
		
		// 2. WIC Decoder 생성 및 이미지 파일 로드.
		ComPtr<IWICBitmapDecoder> wicDecoder;
		
		hr = wicFactory->CreateDecoderFromFilename(
			fileName,
			nullptr,
			GENERIC_READ,
			WICDecodeMetadataCacheOnDemand,
			&wicDecoder
		);
		FAILCHECK(hr, L"Failed to create WIC decoder", NULL_TEXTURE)
		
		// 3. 이미지의 프레임 가져오기.		
		ComPtr<IWICBitmapFrameDecode> wicFrame;
		
		hr = wicDecoder->GetFrame(0, &wicFrame);
		FAILCHECK(hr, L"Failed to get frame", NULL_TEXTURE)
		
		// 4. 이미지 사이즈 가져오기.
		UINT width = 0;
		UINT height = 0;
		
		hr = wicFrame->GetSize(&width, &height);
		FAILCHECK(hr, L"Failed to get frame size", NULL_TEXTURE)
		
		// 5. 이미지 변환기 생성.
		ComPtr<IWICFormatConverter> wicConverter;
		hr = wicFactory->CreateFormatConverter(&wicConverter);
		FAILCHECK(hr, L"Failed to create WIC format converter", NULL_TEXTURE)
		
		// 6. 이미지 포맷 변환 (DirectX에서 지원하는 포맷으로).
		// PNG/JPG는 색상표를 사용하지 않아 초기화 시 값을 넘겨주지 않아도 된다.
		// 팔레트 기반 변환이 아니기 때문에 디더링도 필요 없다.
		hr = wicConverter->Initialize(
			wicFrame.Get(),						// 원본.
			GUID_WICPixelFormat32bppRGBA,		// 변환할 포맷 (RGBA 8bit).
			WICBitmapDitherTypeNone,			// 디더링 없음.
			nullptr,							// 팔레트 (필요 없음).
			0.0f,								// 알파 임계값 (팔레트 변환 시에만 사용).
			WICBitmapPaletteTypeCustom			// 팔레트 유형 (필요 없음).
		);
		FAILCHECK(hr, L"Failed to initialize WIC format converter", NULL_TEXTURE)
		
		// 7. 변환된 이미지 데이터를 배열로 꺼내기.
		// 이미지 한 행(row)의 바이트 수 (RGBA 각 1바이트 씩 총 4바이트).
		UINT stride = width * 4;
		
		// 버퍼 크기는 이미지 전체 크기 (행 바이트 수 * 높이).
		UINT bufferSize = stride * height;
		
		// 버퍼 선언.
		std::vector<uint8_t> pixelBuffer(bufferSize);
		
		// 픽셀 버퍼에 가져오기.
		hr = wicConverter->CopyPixels(
			nullptr,				// 변환된 이미지 전체를 복사하기 위해 null로 설정.
			stride,					// 한 행의 바이트 수.
			bufferSize,				// 버퍼 크기.
			pixelBuffer.data()		// 변환된 이미지 데이터를 받을 버퍼.
		);
		FAILCHECK(hr, L"Failed to copy pixels from WIC converter", NULL_TEXTURE)
		
		// 8. Texture2D 생성.
		// 텍스처 생성용 설명서.
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_IMMUTABLE; // 변경 불가능한 텍스처 (생성 시 데이터 제공 필요).
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // 셰이더에서 읽을 수 있는 텍스처로 바인딩.
		
		// 업로드할 데이터 연결.
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = pixelBuffer.data(); // 변환된 이미지 데이터.
		initData.SysMemPitch = stride; // 한 행의 바이트 수.
		
		// 텍스처 생성.
		ComPtr<ID3D11Texture2D> texture;
		hr = device->CreateTexture2D(&textureDesc, &initData, &texture);
		FAILCHECK(hr, L"Failed to create texture", NULL_TEXTURE)
		
		// 9. Shader Resource View 생성.
		ComPtr<ID3D11ShaderResourceView> textureView;
		hr = device->CreateShaderResourceView(texture.Get(), nullptr, &textureView);
		FAILCHECK(hr, L"Failed to create shader resource view", NULL_TEXTURE)
		
		TextureHandle textureHandle = nextTextureHandle++;
		textureMap[textureHandle] = textureView;
		return textureHandle;
	}

	void D3D11Renderer::ReleaseTexture(TextureHandle texture)
	{
		textureMap.erase(texture);
	}

	void D3D11Renderer::BeginFrame(float r, float g, float b)
	{
		// FILP_DISCARD 방식은 Present() 후 렌더 타겟 바인딩이 해제되므로 매 프레임 재바인딩 필요.
		context->OMSetRenderTargets(
			1, 
			renderTargetView.GetAddressOf(), 
			depthStencilView.Get());

		// 매 프레임 시작 시 back buffer를 지정한 색으로 초기화.
		// 안 지우면 이전 프레임 내용이 남아있음.
		float color[4] = { r, g, b, 1.0f };
		context->ClearRenderTargetView(renderTargetView.Get(), color);
		context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
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
		textureMap.clear();
		depthStencilState.Reset();
		depthStencilView.Reset();
		depthStencilTexture.Reset();
		renderTargetView.Reset();
		swapChain.Reset();
		context.Reset();
		device.Reset();
		CoUninitialize();
	}
	
	void D3D11Renderer::Render()
	{			
		std::unordered_map<RenderLayerType, std::vector<RenderCommand>> passCommandMap;
		
		for (const RenderCommand& command : renderCommands)
		{
			// PassType별 Command 그룹화.
			passCommandMap[command.layerType].emplace_back(command);
		}
		
		for (auto& command : passCommandMap)
		{
			RenderLayer* pass = layerScheduler.GetPass(command.first);
			if (pass)
			{
				pass->Prepare(context.Get());
				pass->Execute(context.Get(), command.second, bufferMap, textureMap);
			}
		}
		
		renderCommands.clear();
	}

	void D3D11Renderer::Submit(const RenderCommand& command)
	{
		renderCommands.emplace_back(command);
	}
}
