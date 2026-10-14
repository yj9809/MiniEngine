#pragma once

#include "Lighting/Light.h"
#include "Math/Vector3.h"

namespace Engine
{
    class ENGINE_API DirectLight : public Light
    {
    public:
        inline void SetColor(const Vector3& color)
        {
            this->color = color;
        }

        inline const Vector3& GetColor() const
        {
            return color;
        }

    private:
        Vector3 color = Vector3::one;

    };
}