#pragma once

#include "Common/Common.h"
#include "Common/RTTI.h"
#include "Component/Component.h"
#include "Math/Vector3.h"

namespace Engine
{
    class LightingSystem;

    class ENGINE_API LightComponent : public Component
    {
        RTTI_DECLARATIONS(LightComponent, Component)

    protected:
        // 설정 값 유효성 검사. 유효하지 않은 값이면 false를 반환한다.
        static bool IsValidLightValue(float value);
        static bool IsValidLightColor(const Vector3& color);

    private:
        virtual void RegisterWithSystem() final override;

        virtual void UnregisterFromSystem() final override;

        virtual void RegisterToLightSystem(LightingSystem& system) = 0;

        virtual void UnregisterFromLightSystem(LightingSystem& system) = 0;

    private:
        LightingSystem* registeredSystem = nullptr;
    };
}
