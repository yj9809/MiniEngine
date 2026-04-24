#pragma once

#include "Common/Common.h"
#include "Common/RTTI.h"
#include "Component/Component.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"

namespace Engine
{
    class ENGINE_API TransformComponent : public Component
    {
        RTTI_DECLARATIONS(TransformComponent, Component)
        
    public:
        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = delete;
        TransformComponent& operator=(const TransformComponent&) = delete;
        
        // Setter.
        void SetLocalPosition(const Vector3& position);
        void SetLocalRotationEulerDeg(const Vector3& rotation);
        void SetLocalScale(const Vector3& scale);
        
        // Getter.
        inline Vector3 GetLocalPosition() const { return localPosition; }
        inline Vector3 GetLocalRotationEulerDeg() const { return localRotationEulerDeg; }
        inline Vector3 GetLocalScale() const { return localScale; }
        const Matrix4& GetWorldMatrix() const;
    private:
        Vector3 localPosition = Vector3::zero;
        // 회전 값은 Degree로 저장.
        // Todo: 추후 Quaternion으로 변경 고려.
        Vector3 localRotationEulerDeg = Vector3::zero;
        Vector3 localScale = Vector3::one;
        
        // 재계산 필요 여부 플래그.
        mutable bool dirty = true;
        // 마지막으로 계산된 월드 행렬.
        mutable Matrix4 cachedWorldMatrix = Matrix4::identity;
        // 부모 TransformComponent 저장 변수.
        TransformComponent* parent = nullptr;
    };
}

