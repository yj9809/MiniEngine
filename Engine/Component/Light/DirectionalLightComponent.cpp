#include "DirectionalLightComponent.h"

namespace Engine
{
    void DirectionalLightComponent::SetColor(const Vector3& color)
    {
        light.SetColor(color);
    }

    void DirectionalLightComponent::SetIntensity(float value)
    {
        light.SetIntensity(value);
    }
}