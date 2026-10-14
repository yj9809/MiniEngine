#include "LightComponent.h"

#include <cmath>

#include "System/LightingSystem.h"
#include "Level/Level.h"
#include "Actor/Actor.h"

namespace Engine
{
    bool LightComponent::IsValidLightValue(float value)
    {
        return std::isfinite(value) && value >= 0.0f;
    }

    bool LightComponent::IsValidLightColor(const Vector3& color)
    {
        return IsValidLightValue(color.x) && IsValidLightValue(color.y) && IsValidLightValue(color.z);
    }

    void LightComponent::RegisterWithSystem()
    {
        if(registeredSystem != nullptr)
        {
            return;
        }

        Level* ownerLevel = GetOwner().GetOwner();

        if(!ownerLevel)
        {
            return;
        }

        LightingSystem* lightingSystem = ownerLevel->GetLightingSystem();

        if(!lightingSystem)
        {
            return;
        }

        RegisterToLightSystem(*lightingSystem);
        registeredSystem = lightingSystem;
    }

    void LightComponent::UnregisterFromSystem()
    {
        if(registeredSystem == nullptr)
        {
            return;
        }

        UnregisterFromLightSystem(*registeredSystem);
        registeredSystem = nullptr;
    }
}