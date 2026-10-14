#include "SkyLightComponent.h"

#include "System/LightingSystem.h"
#include "Core/Log.h"

namespace Engine
{
    void SkyLightComponent::SetSkyColor(const Vector3& color)
    {
        if (IsEndingOrEnded())
        {
            Log::Error("SkyLightComponent::SetSkyColor: component is ending or ended.");
            return;
        }

        if (!IsValidLightColor(color))
        {
            Log::Error("SkyLightComponent::SetSkyColor: RGB must be finite and non-negative.");
            return;
        }

        light.SetSkyColor(color);
    }

    void SkyLightComponent::SetGroundColor(const Vector3& color)
    {
        if (IsEndingOrEnded())
        {
            Log::Error("SkyLightComponent::SetGroundColor: component is ending or ended.");
            return;
        }

        if (!IsValidLightColor(color))
        {
            Log::Error("SkyLightComponent::SetGroundColor: RGB must be finite and non-negative.");
            return;
        }

        light.SetGroundColor(color);
    }

    void SkyLightComponent::SetIntensity(float value)
    {
        if (IsEndingOrEnded())
        {
            Log::Error("SkyLightComponent::SetIntensity: component is ending or ended.");
            return;
        }

        if (!IsValidLightValue(value))
        {
            Log::Error("SkyLightComponent::SetIntensity: intensity must be finite and non-negative.");
            return;
        }

        light.SetIntensity(value);
    }

    Vector3 SkyLightComponent::GetSkyColor() const
    {
        return light.GetSkyColor();
    }

    Vector3 SkyLightComponent::GetGroundColor() const
    {
        return light.GetGroundColor();
    }

    float SkyLightComponent::GetIntensity() const
    {
        return light.GetIntensity();
    }

    void SkyLightComponent::RegisterToLightSystem(LightingSystem& system)
    {
        system.Register(this);
    }

    void SkyLightComponent::UnregisterFromLightSystem(LightingSystem& system)
    {
        system.Unregister(this);
    }
}