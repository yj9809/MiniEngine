#include "ResourceManager.h"

#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"

namespace Engine
{
    ResourceManager::ResourceManager(IRenderer& renderer)
        : renderer(renderer)
    {
    }

    std::shared_ptr<Mesh> ResourceManager::LoadMesh(const std::string& path)
    {
        const auto found = meshCache.find(path);
        if (found != meshCache.end())
        {
            return found->second;
        }
        
        auto mesh = Mesh::LoadFromOBJ(&renderer, path.c_str());
        if (!mesh)
        {
            return nullptr;
        }
        
        meshCache.emplace(path, mesh);
        return mesh;
    }

    std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::wstring& path)
    {
        const auto found = textureCache.find(path);
        if (found != textureCache.end())
        {
            return found->second;
        }

        auto texture = Texture::LoadFromFile(&renderer, path.c_str());
        if (!texture)
        {
            return nullptr;
        }

        textureCache.emplace(path, texture);
        return texture;
    }

    void ResourceManager::Clear()
    {
        meshCache.clear();
        textureCache.clear();
    }
}
