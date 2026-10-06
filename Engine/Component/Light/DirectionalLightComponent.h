#pragma once

#include "Common/Common.h"
#include "Common/RTTI.h"
#include "Component/Component.h"
#include "Math/Vector3.h"
#include "Lighting/DirectionalLight.h"

namespace Engine
{
    class LightingSystem;

    class ENGINE_API DirectionalLightComponent : public Component
    {
        RTTI_DECLARATIONS(DirectionalLightComponent, Component)

    public:
        void SetColor(const Vector3& color);

        void SetIntensity(float value);

    protected:
        virtual void BeginPlay() override;

        virtual void OnRemove() override;

    private:
        DirectionalLight light;

        LightingSystem* registeredSystem = nullptr;
    };
}