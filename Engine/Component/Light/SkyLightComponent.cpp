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

        light.SetSkyColor(color);
    }

    void SkyLightComponent::SetGroundColor(const Vector3& color)
    {
        light.SetGroundColor(color);
    }

    void SkyLightComponent::SetIntensity(float value)
    {
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