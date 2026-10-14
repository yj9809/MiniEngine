#pragma once

#include "Lighting/Light.h"
#include "Math/Vector3.h"

namespace Engine
{
    class ENGINE_API SkyLight : public Light
    {
    public:
        SkyLight()
        {
            SetIntensity(0.1f);
        }

        inline void SetSkyColor(const Vector3& color)
        {
            skyColor = color;
        }

        inline void SetGroundColor(const Vector3& color)
        {
            groundColor = color;
        }

        inline const Vector3& GetSkyColor() const
        {
            return skyColor;
        }

        inline const Vector3& GetGroundColor() const
        {
            return groundColor;
        }

    private:
        Vector3 skyColor = Vector3::one;
        Vector3 groundColor = Vector3::one;
    };
}