#include "TransformComponent.h"


namespace Engine
{
    void TransformComponent::SetLocalPosition(const Vector3& position)
    {
        dirty = true;
        
        if (onTransformChanged)
        {
            onTransformChanged();
        }
        
        localPosition = position;
    }

    void TransformComponent::SetLocalRotationEulerDeg(const Vector3& rotation)
    {
        dirty = true;
        
        if (onTransformChanged)
        {
            onTransformChanged();
        }
        
        localRotationEulerDeg = rotation;
    }

    void TransformComponent::SetLocalScale(const Vector3& scale)
    {
        dirty = true;
        localScale = scale;
    }

    const Matrix4& TransformComponent::GetWorldMatrix() const
    {
        // dirty가 false면 캐시된 행렬 반환.
        if (!dirty)
        {
            return cachedWorldMatrix;
        }
        
        // Degree -> Radian 변환.
        Vector3 rotationRad = localRotationEulerDeg * (PI / 180.0f);
        // 새로운 World 행렬 대입.
        cachedWorldMatrix = Matrix4::Scale(localScale) *
                            Matrix4::Rotation(rotationRad) *
                            Matrix4::Translation(localPosition);
        
        dirty = false;
        
        return cachedWorldMatrix;
    }
}
