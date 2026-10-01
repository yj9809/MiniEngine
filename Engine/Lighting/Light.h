#pragma once

#include "Math/Vector3.h"

namespace Engine
{
    class ENGINE_API Light
    {
    public:
        void SetColor(const Vector3& color)
        {
            this->color = color;
        }

        void SetIntensity(float value)
        {
            intensity = value;
        }

    private:
        // 빛의 색상.
        Vector3 color = Vector3::one;

        // 빛의 밝기.
        float intensity = 1.0f;
    };
}