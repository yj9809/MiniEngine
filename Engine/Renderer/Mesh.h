#pragma once

#include "RenderCommand.h"

#include <memory>

namespace Engine
{
    class IRenderer;
    
    class ENGINE_API Mesh
    {
    public:
        ~Mesh();
        
        static std::shared_ptr<Mesh> LoadFromOBJ(IRenderer* renderer, const char* fileName);
        
        // Getter.
        inline BufferHandle GetVertexBuffer() const { return vertexBuffer; }
        inline BufferHandle GetIndexBuffer() const { return indexBuffer; }
        inline UINT GetIndexCount() const { return indexCount; }
        inline UINT GetStride() const { return stride; }
        
    private:
        // 버퍼 생성/반환을 위한 렌더러.
        IRenderer* renderer;
        
        BufferHandle vertexBuffer = NULL_BUFFER;
        BufferHandle indexBuffer = NULL_BUFFER;
        
        UINT indexCount = 0;
        UINT stride = 0;
    };
}
