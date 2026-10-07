#pragma once

#include "Common/Common.h"
#include "Common/RTTI.h"
#include "Component/Light/LightComponent.h"
#include "Math/Vector3.h"
#include "Lighting/DirectionalLight.h"
#include "Lighting/RenderData/DirectionalLightRenderData.h"

namespace Engine
{
    class ENGINE_API DirectionalLightComponent : public LightComponent
    {
        RTTI_DECLARATIONS(DirectionalLightComponent, LightComponent)

    public:
        void SetColor(const Vector3& color);

        void SetIntensity(float value);

        Vector3 GetDirection() const;

        DirectionalLightRenderData BuildRenderData() const;

    private:
        virtual void RegisterToLightSystem(LightingSystem& system) override;

        virtual void UnregisterFromLightSystem(LightingSystem& system) override;

    private:
        DirectionalLight light;
    };
}