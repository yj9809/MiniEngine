#pragma once

#include "RenderCommand.h"

#include <Windows.h>

namespace Engine
{
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		// GPU 초기화. 성공하면 true 반환.
		virtual bool GPUInit(HWND hwnd, int width, int height) = 0;

		// 렌더링 요청 함수.
		virtual void Submit(const RenderCommand& command) = 0;

		// 프레임 시작 -> back buffer 그리기.
		virtual void BeginFrame(float r, float g, float b) = 0;

		// 프레임 종료 -> back buffer를 front buffer와 교체.
		virtual void EndFrame() = 0;

		// GPU 해제.
		virtual void GPUShutdown() = 0;

		// 렌더링 명령 실행 함수.
		virtual void Render() = 0;
		
		// 버퍼 생성 함수.
		virtual BufferHandle CreateVertexBuffer(const void* vertexData, UINT vertexDataSize) = 0;
		virtual BufferHandle CreateIndexBuffer(const void* indexData, UINT indexDataSize) = 0;
		
		// 버퍼 반환 함수.
		virtual void ReleaseBuffer(BufferHandle buffer) = 0;
		
		// 텍스처 생성 함수.
		// 경로를 wchar_t로 받는 이유는 WIC API가 와이드 문자열을 요구하기 때문.
		// WIC = Windows Imaging Component.
		// 윈도우가 기본으로 제공하는 이미지 디코딩 라이브러리.
		virtual TextureHandle CreateTexture(const wchar_t* fileName) = 0;
		
		// 텍스처 반환 함수.
		virtual void ReleaseTexture(TextureHandle texture) = 0;
	};
}
