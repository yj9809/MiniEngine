#include "LightComponent.h"

#include "System/LightingSystem.h"
#include "Level/Level.h"
#include "Actor/Actor.h"

namespace Engine
{
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