#include "DirectionalLightComponent.h"

#include "Actor/Actor.h"
#include "Component/Transform/TransformComponent.h"
#include "System/LightingSystem.h"

namespace Engine
{
    void DirectionalLightComponent::RegisterToLightSystem(LightingSystem& system)
    {
        system.Register(this);
    }

    void DirectionalLightComponent::UnregisterFromLightSystem(LightingSystem& system)
    {
        system.Unregister(this);
    }

    void DirectionalLightComponent::SetColor(const Vector3& color)
    {
        light.SetColor(color);
    }

    void DirectionalLightComponent::SetIntensity(float value)
    {
        light.SetIntensity(value);
    }

    Vector3 DirectionalLightComponent::GetDirection() const
    {
        return GetOwner().GetRootComponent()->GetForward();
    }

    DirectionalLightRenderData DirectionalLightComponent::BuildRenderData() const
    {
        DirectionalLightRenderData renderData;
        renderData.direction = GetDirection();
        renderData.color = light.GetColor();
        renderData.intensity = light.GetIntensity();

        return renderData;
    }
}