#pragma once

#include <memory>

#include "Math/Vector4.h"

namespace Engine
{
    class Texture;

    class Material
    {
    public:
        void SetBaseColor(const Vector4& color) 
        {
            baseColor = color; 
        }
        const Vector4& GetBaseColor() const 
        {
            return baseColor; 
        }

        void SetMainTexture(std::shared_ptr<Texture> texture)
        {
            mainTexture = std::move(texture);
        }
        const std::shared_ptr<Texture>& GetMainTexture() const 
        {
            return mainTexture;
        }

    private:
        Vector4 baseColor = Vector4::one;
        std::shared_ptr<Texture> mainTexture;
    };
}