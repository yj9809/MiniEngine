#include "DirectionalLightComponent.h"

#include "Actor/Actor.h"
#include "Level/Level.h"
#include "System/LightingSystem.h"

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

    void DirectionalLightComponent::BeginPlay()
    {
        Component::BeginPlay();

        // LightingSystem에 중복 등록 방지.
        if (registeredSystem != nullptr)
        {
            return;
        }

        Level* ownerLevel = GetOwner().GetOwner();

        if (!ownerLevel)
        {
            return;
        }

        LightingSystem* lightingSystem = ownerLevel->GetLightingSystem();
        lightingSystem->RegisterDirectionalLight(this);
        registeredSystem = lightingSystem;
    }

    void DirectionalLightComponent::OnRemove()
    {
        if (registeredSystem != nullptr)
        {
            registeredSystem->UnregisterDirectionalLight(this);
            registeredSystem = nullptr;
        }
        Component::OnRemove();
    }
}