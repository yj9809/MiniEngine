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
        
        // Getter.
        Matrix4 GetViewMatrix() const;
        Matrix4 GetProjectionMatrix() const;
        
    private:
        void LookAt() const;
        void Projection() const;
        
    private:
        ViewInfo viewInfo;
        
        mutable Matrix4 cachedViewMatrix = Matrix4::identity;
        mutable Matrix4 cachedProjectionMatrix = Matrix4::identity;
        
        mutable bool dirtyView = true;        
        mutable bool dirtyProjection = true;
    };
}
