#pragma once

#include "Common/Common.h"
#include "Common/RTTI.h"
#include "Component/Component.h"

namespace Engine
{
    class LightingSystem;

    class ENGINE_API LightComponent : public Component
    {
        RTTI_DECLARATIONS(LightComponent, Component)

    protected:
        virtual void RegisterWithSystem() final override;

        virtual void UnregisterFromSystem() final override;

        virtual void RegisterToLightSystem(LightingSystem& system) = 0;

        virtual void UnregisterFromLightSystem(LightingSystem& system) = 0;

    private:
        LightingSystem* registeredSystem = nullptr;
    };
}