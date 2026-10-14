#pragma once

#include "Common/Common.h"
#include "Common/RTTI.h"
#include "Component/Light/LightComponent.h"
#include "Lighting/Environment/SkyLight.h"

namespace Engine
{
    class ENGINE_API SkyLightComponent : public LightComponent
    {
        RTTI_DECLARATIONS(SkyLightComponent, LightComponent)

    public:
        void SetSkyColor(const Vector3& color);

        void SetGroundColor(const Vector3& color);

        void SetIntensity(float value);

        Vector3 GetSkyColor() const;

        Vector3 GetGroundColor() const;

        float GetIntensity() const;

    private:
        virtual void RegisterToLightSystem(LightingSystem& system) override;

        virtual void UnregisterFromLightSystem(LightingSystem& system) override;

    private:
        SkyLight light;
    };
}