#include "CameraComponent.h"

#include "Actor/Actor.h"
#include "Component/Transform/TransformComponent.h"

namespace Engine
{
    void CameraComponent::OnAdd()
    {
        Component::OnAdd();
        
        GetOwner().GetRootComponent()->onTransformChanged = [this]()
        {
            // 뷰 행렬 재계산을 위해 콜백 함수로 플래그 연결.
            dirtyView = true;
        };
    }

    Matrix4 CameraComponent::GetViewMatrix() const
    {
        if (dirtyView)
        {
            LookAt();
            dirtyView = false;
        }
        
        return cachedViewMatrix;
    }

    Matrix4 CameraComponent::GetProjectionMatrix() const
    {
        if (dirtyProjection)
        {
            Projection();
            dirtyProjection = false;
        }
        
        return cachedProjectionMatrix;
    }

    void CameraComponent::LookAt() const
    {
        Vector3 rotation = GetOwner().GetRotation();
        Vector3 forward;
        
        // Degree -> Radian 값으로 변환.
        // rotation.x = pitch.
        // rotation.y = yaw.
        float pitch = rotation.x * (PI / 180.f);
        float yaw = rotation.y * (PI / 180.f);
        
        forward.x = std::cos(pitch) * std::sin(yaw);
        forward.y = -std::sin(pitch);
        forward.z = std::cos(pitch) * std::cos(yaw);
        
        Vector3 cameraPosition = GetOwner().GetPosition();
        
        cachedViewMatrix = Matrix4::LookAt(
            cameraPosition, 
            cameraPosition + forward, 
            Vector3::up);
    }

    void CameraComponent::Projection() const
    {
        cachedProjectionMatrix = Matrix4::PerspectiveFOV(
            viewInfo.fov,
            viewInfo.aspect,
            viewInfo.nearZ,
            viewInfo.farZ
        );
    }
}
