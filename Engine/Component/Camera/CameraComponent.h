#pragma once

#include "Common/Common.h"
#include "Component/Component.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"

namespace Engine
{
    struct ViewInfo
    {
        // 시야각.
        float fov = 60.0f;
        // 종횡비.
        float aspect = 16.0f / 9.0f;
        // 근접 클리핑 평면.
        // 해당 거리보다 가까운 물체는 그리지 않는다.
        float nearZ = 0.1f;
        // 원거리 클리핑 평면.
        // 해당 거리보다 먼 물체는 그리지 않는다.
        float farZ = 1000.0f;
    };
    
    class ENGINE_API CameraComponent : public Component
    {
        RTTI_DECLARATIONS(CameraComponent, Component)
        
    public:
        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = delete;
        CameraComponent& operator=(const CameraComponent&) = delete;
        
        virtual void OnAdd() override;
        
        // Setter.
        inline void SetViewInfo(const ViewInfo& viewInfo)
        {
            this->viewInfo = viewInfo;
            dirtyProjection = true;
        }
        inline void SetFov(float fov)
        {
            viewInfo.fov = fov;
            dirtyProjection = true;
        }
        inline void SetAspect(float aspect)
        {
            viewInfo.aspect = aspect;
            dirtyProjection = true;
        }
        inline void SetNearZ(float nearZ)
        {
            viewInfo.nearZ = nearZ;
            dirtyProjection = true;
        }
        inline void SetFarZ(float farZ)
        {
            viewInfo.farZ = farZ;
            dirtyProjection = true;
        }
        
        /**
         * 현재 카메라의 View 행렬을 반환합니다.
         * 갱신이 필요하면 재계산하며, 성공했을 때만 갱신 요청을 해제합니다.
         * 실패 시 기존 View를 반환하고 다음 조회에서 다시 시도합니다.
         * 아직 성공적으로 계산한 View가 없으면 초기값인 Identity를 반환합니다.
         * @return 마지막으로 성공적으로 계산한 View 행렬 또는 초기 Identity.
         */
        Matrix4 GetViewMatrix() const;
        Matrix4 GetProjectionMatrix() const;
        
    private:
        /**
         * 소유 Actor의 위치와 Quaternion 기반 방향으로 View 행렬 갱신을 시도합니다.
         * Transform의 Forward와 Up을 사용하므로 Roll을 포함한 자세를 반영합니다.
         * 위치와 회전만 사용하며 Transform의 스케일은 반영하지 않습니다.
         * 현재 부모 Transform 계층이 없는 구조를 기준으로 합니다.
         * 성공 시 계산한 행렬을 캐시에 저장하고, 실패 시 기존 캐시를 유지합니다.
         * dirtyView의 갱신은 호출부에서 처리합니다.
         * @return View 행렬 생성과 캐시 갱신에 성공하면 true, 실패하면 false.
         */
        bool TryUpdateViewMatrix() const;
        void Projection() const;
        
    private:
        ViewInfo viewInfo;
        
        mutable Matrix4 cachedViewMatrix = Matrix4::identity;
        mutable Matrix4 cachedProjectionMatrix = Matrix4::identity;
        
        mutable bool dirtyView = true;        
        mutable bool dirtyProjection = true;
    };
}
