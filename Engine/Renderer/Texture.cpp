#include "Texture.h"

#include "IRenderer.h"

namespace Engine
{
    Texture::~Texture()
    {
        if (textureHandle)
        {
            renderer->ReleaseTexture(textureHandle);
        }
    }

    std::shared_ptr<Texture> Texture::LoadFromFile(IRenderer* renderer, const wchar_t* fileName)
    {
        
    }
}
