#pragma once

#include <functional>

#include "Common/Common.h"
#include "Common/RTTI.h"
#include "Component/Component.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"

namespace Engine
{
    class ENGINE_API TransformComponent : public Component
    {
        RTTI_DECLARATIONS(TransformComponent, Component)

    public:
        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = delete;
        TransformComponent& operator=(const TransformComponent&) = delete;

        // Transform 변경 시 호출되는 콜백 함수.
        // Scale은 카메라에 영향을 주지 않으므로 콜백에서 제외한다.
        std::function<void()> onTransformChanged;

        void SetLocalPosition(const Vector3& position);

        /**
         * 로컬 자세를 Euler 각도로 설정합니다.
         * rotation.x=Pitch, y=Yaw, z=Roll이며 단위는 degree입니다.
         * 기존 자세에 더하는 값이 아니라 새 자세를 지정합니다.
         * Quaternion으로 변환·정규화해 저장하며 Euler 입력값은 보관하지 않습니다.
         * 변환 또는 정규화 실패 시 기존 자세를 유지하고 변경 알림을 호출하지 않습니다.
         */
        void SetLocalRotationEulerDeg(const Vector3& rotation);

        void SetLocalScale(const Vector3& scale);

        // Getter.
        inline Vector3 GetLocalPosition() const { return localPosition; }
        inline Vector3 GetLocalScale() const { return localScale; }
        inline Quaternion GetLocalRotation() const { return localRotation; }

        /**
         * 현재 로컬 Quaternion 자세를 Euler 각도로 표현해 반환합니다.
         * 반환값은 x=Pitch, y=Yaw, z=Roll이며 단위는 degree입니다.
         * Pitch는 [-90, 90], Yaw와 Roll은 [-180, 180) 범위입니다.
         * 극점 근처에서는 Roll=0인 대표 표현을 반환합니다.
         * 마지막 입력 각도, 누적 회전 횟수, 각도의 연속성을 보장하지 않습니다.
         * 저장된 자세와 변경 알림에는 영향을 주지 않습니다.
         * 내부 변환 실패 시 zero를 반환하며 저장된 자세는 유지합니다.
         */
        Vector3 GetLocalRotationEulerDeg() const;

        const Matrix4& GetWorldMatrix() const;
        Vector3 GetForward() const;

        /**
         * 현재 자세를 기준으로 물체의 오른쪽 방향을 반환합니다.
         * 로컬 +Y축을 Quaternion 기반 회전 행렬로 회전하고 정규화합니다.
         * 위치와 스케일의 영향을 받지 않으며, 자세를 변경하거나 알림을 호출하지 않습니다.
         * 현재 부모 Transform 계층이 없는 구조를 기준으로 합니다.
         * @return 월드 좌표로 표현한 물체의 오른쪽 단위 방향.
         */
        Vector3 GetRight() const;

        /**
         * 현재 자세를 기준으로 물체의 위쪽 방향을 반환합니다.
         * 로컬 +Z축을 Quaternion 기반 회전 행렬로 회전하고 정규화합니다.
         * Roll을 포함한 물체의 회전을 반영하므로 고정된 월드 Up과 다를 수 있습니다.
         * 위치와 스케일의 영향을 받지 않으며, 자세를 변경하거나 알림을 호출하지 않습니다.
         * 현재 부모 Transform 계층이 없는 구조를 기준으로 합니다.
         * @return 월드 좌표로 표현한 물체의 위쪽 단위 방향.
         */
        Vector3 GetUp() const;

        bool TrySetLocalRotation(const Quaternion& rotation);

        /**
         * 현재 물체 좌표계 기준으로 회전을 추가합니다.
         * q * delta 순서로 합성하고, 결과를 정규화해 저장합니다.
         * 위치와 스케일은 유지합니다.
         * 성공 시 변경 알림이 등록되어 있으면 한 번 호출합니다.
         * 실패 시 기존 자세를 유지하고 변경 알림을 호출하지 않습니다.
         * @param deltaRotation 물체의 로컬 좌표계에서 표현한 추가 회전.
         *          단위 Quaternion이 아니어도 TryNormalize의 기본 기준으로
         *          정규화할 수 있으면 허용합니다. 입력 원본은 변경하지 않습니다.
         * @return 입력 정규화와 합성 결과 저장에 성공하면 true, 둘 중 하나라도 실패하면 false.
         */
        bool TryRotateLocal(const Quaternion& deltaRotation);
        /**
         * 고정된 월드 축을 기준으로 회전을 추가합니다.
         * delta * q 순서로 합성하고, 결과를 정규화해 저장합니다.
         * 자세만 변경하며 위치와 스케일은 유지합니다.
         * 위치를 월드 원점 주위로 회전 시키는 공전 동작은 수행하지 않습니다.
         * 성공 시 변경 알림이 등록되어 있으면 한 번 호출합니다.
         * 실패 시 기존 자세를 유지하고 변경 알림을 호출하지 않습니다.
         * 현재 부모 Transform 계층을 지원하지 않는 상태를 기준으로 합니다.
         * @param deltaRotation 월드 좌표계에서 표현한 추가 회전.
         *         단위 Quaternion이 아니어도 TryNormalize의 기본 기준으로
         *         정규화할 수 있으면 허용합니다. 입력 원본은 변경하지 않습니다.
         * @return 입력 정규화와 합성 결과 저장에 성공하면 true, 둘 중 하나라도 실패하면 false.
         */
        bool TryRotateWorld(const Quaternion& deltaRotation);

    private:
        Matrix4 GetRotationMatrix() const;

    private:
        Vector3 localPosition = Vector3::zero;
        Vector3 localScale = Vector3::one;

        Quaternion localRotation = Quaternion::identity;

        // 재계산 필요 여부 플래그.
        mutable bool dirty = true;
        // 마지막으로 계산된 월드 행렬.
        mutable Matrix4 cachedWorldMatrix = Matrix4::identity;
        // 부모 TransformComponent 저장 변수.
        TransformComponent* parent = nullptr;
    };
}
