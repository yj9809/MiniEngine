#include "DirectionalLightComponent.h"

#include "Actor/Actor.h"
#include "Level/Level.h"
#include "System/LightingSystem.h"

namespace Engine
{
    void DirectionalLightComponent::BeginPlay()
    {
        Component::BeginPlay();

        // LightingSystem에 DirectionalLightComponent 등록.
        auto* lightingSystem = GetOwner().GetOwner()->GetLightingSystem();
        lightingSystem->RegisterDirectionalLight(this);
    }

    void DirectionalLightComponent::OnRemove()
    {
        Component::OnRemove();

        // LightingSystem에서 DirectionalLightComponent 제거.
        auto* lightingSystem = GetOwner().GetOwner()->GetLightingSystem();
        lightingSystem->UnregisterDirectionalLight(this);
    }

    void DirectionalLightComponent::SetColor(const Vector3& color)
    {
        light.SetColor(color);
    }

    void DirectionalLightComponent::SetIntensity(float value)
    {
        light.SetIntensity(value);
    }
}