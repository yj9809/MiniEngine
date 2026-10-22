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
        const float toRadians = PI / 180.0f;
        
        Quaternion converted;
        if (!Quaternion::TryFromEuler(
            rotation.z * toRadians, // Roll
            rotation.x * toRadians, // Pitch
            rotation.y * toRadians, // Yaw
            converted))
        {
            return;
        }
        
        TrySetLocalRotation(converted);
    }

    void TransformComponent::SetLocalScale(const Vector3& scale)
    {
        dirty = true;
        localScale = scale;
    }

    Vector3 TransformComponent::GetLocalRotationEulerDeg() const
    {
        Vector3 eulerRadians;
        if (!localRotation.TryToEuler(eulerRadians))
        {
            return Vector3::zero;
        }

        const float toDegrees = 180.0f / PI;

        return Vector3(
            eulerRadians.y * toDegrees, // Pitch
            eulerRadians.z * toDegrees, // Yaw
            eulerRadians.x * toDegrees  // Roll
        );
    }

    const Matrix4& TransformComponent::GetWorldMatrix() const
    {
        // dirty가 false면 캐시된 행렬 반환.
        if (!dirty)
        {
            return cachedWorldMatrix;
        }

        // 새로운 World 행렬 대입.
        const Matrix4 rotationMatrix = GetRotationMatrix();

        cachedWorldMatrix = Matrix4::Scale(localScale) *
            rotationMatrix *
            Matrix4::Translation(localPosition);

        dirty = false;

        return cachedWorldMatrix;
    }

    Vector3 TransformComponent::GetForward() const
    {
        Vector3 forward = GetRotationMatrix().TransformVector(Vector3::unitX);
        forward.Normalize();
        return forward;
    }

    Vector3 TransformComponent::GetRight() const
    {
        Vector3 right = GetRotationMatrix().TransformVector(Vector3::unitY);
        right.Normalize();
        return right;
    }

    Vector3 TransformComponent::GetUp() const
    {
        Vector3 up = GetRotationMatrix().TransformVector(Vector3::unitZ);
        up.Normalize();
        return up;
    }

    bool TransformComponent::TrySetLocalRotation(const Quaternion& rotation)
    {
        Quaternion normalizedRotation;

        if (!rotation.TryNormalize(normalizedRotation))
        {
            return false;
        }

        localRotation = normalizedRotation;
        dirty = true;

        if (onTransformChanged)
        {
            onTransformChanged();
        }

        return true;
    }

    bool TransformComponent::TryRotateLocal(const Quaternion& deltaRotation)
    {
        Quaternion normalizedDelta;
        if (!deltaRotation.TryNormalize(normalizedDelta))
        {
            return false;
        }

        const Quaternion combinedRotation = localRotation * normalizedDelta;

        return TrySetLocalRotation(combinedRotation);
    }

    bool TransformComponent::TryRotateWorld(const Quaternion& deltaRotation)
    {
        Quaternion normalizedDelta;
        if (!deltaRotation.TryNormalize(normalizedDelta))
        {
            return false;
        }

        const Quaternion combinedRotation = normalizedDelta * localRotation;

        return TrySetLocalRotation(combinedRotation);
    }

    Matrix4 TransformComponent::GetRotationMatrix() const
    {
        return Matrix4::Rotation(localRotation);
    }
}
