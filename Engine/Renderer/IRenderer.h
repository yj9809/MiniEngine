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
	};
}
