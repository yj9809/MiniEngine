#pragma once

#include "Common/Common.h"
#include "Common/RTTI.h"
#include "Component/Light/LightComponent.h"
#include "Math/Vector3.h"
#include "Lighting/Direct/DirectionalLight.h"
#include "Lighting/RenderData/DirectionalLightRenderData.h"

namespace Engine
{
    class ENGINE_API DirectionalLightComponent : public LightComponent
    {
        RTTI_DECLARATIONS(DirectionalLightComponent, LightComponent)

    public:
        void SetColor(const Vector3& color);

        void SetIntensity(float value);

        /**
         * 소유 Actor의 Quaternion 자세에서 빛의 진행 방향을 구합니다.
         * Actor의 Forward(+X)를 사용하며 위치와 스케일의 영향을 받지 않습니다.
         * 현재 부모 Transform 합성이 없는 구조에서 월드 공간 방향을 반환합니다.
         * @return 빛이 진행하는 방향의 단위 벡터.
         */
        Vector3 GetDirection() const;

        DirectionalLightRenderData BuildRenderData() const;

    private:
        virtual void RegisterToLightSystem(LightingSystem& system) override;

        virtual void UnregisterFromLightSystem(LightingSystem& system) override;

    private:
        DirectionalLight light;
    };
}