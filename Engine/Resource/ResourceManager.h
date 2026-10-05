#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Common/Common.h"

namespace Engine
{
    class IRenderer;
    class Mesh;
    class Texture;
    
    class ENGINE_API ResourceManager
    {
    public:
        explicit ResourceManager(IRenderer& renderer);
        
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;
        
        std::shared_ptr<Mesh> LoadMesh(const std::string& path);
        std::shared_ptr<Texture> LoadTexture(const std::wstring& path);
        
        void Clear();
        
    private:
        IRenderer& renderer;
        
        std::unordered_map<std::string, std::shared_ptr<Mesh>> meshCache;
        std::unordered_map<std::wstring, std::shared_ptr<Texture>> textureCache;
    };
}
