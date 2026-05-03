#pragma once

#include <memory>

#include "RenderCommand.h"
#include "Common/Common.h"

namespace Engine
{
    class IRenderer;
    
    class ENGINE_API Texture
    {
    public:
        ~Texture();
        
        static std::shared_ptr<Texture> LoadFromFile(IRenderer* renderer, const wchar_t* fileName);
        
         // Getter.
        inline TextureHandle GetTextureHandle() const { return textureHandle; }
        
    private:
        IRenderer* renderer = nullptr;
        TextureHandle textureHandle = NULL_TEXTURE;
    };
}
