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
        if (dirtyView && TryUpdateViewMatrix())
        {
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

    Vector3 CameraComponent::GetViewPosition() const
    {
        GetViewMatrix();
        return cachedViewPosition;
    }

    bool CameraComponent::TryUpdateViewMatrix() const
    {
        const TransformComponent* transform = GetOwner().GetRootComponent();
        
        const Vector3 position = transform->GetLocalPosition();
        const Vector3 forward = transform->GetForward();
        const Vector3 up = transform->GetUp();
        
        Matrix4 candidateView;
        if (!Matrix4::TryLookAt(position, position + forward, up, candidateView))
        {
            return false;
        }
        
        cachedViewMatrix = candidateView;
        cachedViewPosition = position;
        return true;
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
