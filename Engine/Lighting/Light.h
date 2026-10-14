#pragma once

#include "Math/Vector3.h"

namespace Engine
{
    class ENGINE_API Light
    {
    public:
        inline  void SetIntensity(float value)
        {
            intensity = value;
        }

        inline float GetIntensity() const
        {
            return intensity;
        }

    private:
        // 빛의 밝기.
        float intensity = 1.0f;
    };
}