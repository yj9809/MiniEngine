#include "CameraActor.h"

#include <algorithm>
#include <cmath>

#include "Component/Transform/TransformComponent.h"
#include "Core/Input.h"
#include "Level/Level.h"
#include "Math/Quaternion.h"

CameraActor::CameraActor()
{
    mainCamera = AddComponent<Engine::CameraComponent>();
}

void CameraActor::BeginPlay()
{
    Actor::BeginPlay();
    GetOwner()->SetMainCamera(mainCamera);
}

void CameraActor::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

    // 현재 Quaternion 자세를 기준으로 이동 방향을 가져옵니다.
    const Engine::Vector3 forward = rootComponent->GetForward();
    const Engine::Vector3 right = rootComponent->GetRight();

    Engine::Vector3 position = rootComponent->GetLocalPosition();

    if (Engine::Input::GetKey('W'))
        position = position + forward * deltaTime;

    if (Engine::Input::GetKey('S'))
        position = position - forward * deltaTime;

    if (Engine::Input::GetKey('A'))
        position = position - right * deltaTime;

    if (Engine::Input::GetKey('D'))
        position = position + right * deltaTime;

    rootComponent->SetLocalPosition(position);

    if (Engine::Input::GetMouseButton(1))
    {
        const auto delta = Engine::Input::GetMouseDeltaPosition();
        constexpr float lookSensitivity = 0.1f; // 마우스 이동량을 회전량으로 변환하는 비율.

        TryApplyLookInputDeg(delta.x * lookSensitivity, delta.y * lookSensitivity);
    }
}

bool CameraActor::TryApplyLookInputDeg(float deltaYawDeg, float deltaPitchDeg)
{
    if (!std::isfinite(deltaYawDeg) || !std::isfinite(deltaPitchDeg))
    {
        return false;
    }

    if (deltaYawDeg == 0.0f && deltaPitchDeg == 0.0f)
    {
        return true;
    }

    // Transform 인터페이스: x=Pitch, y=Yaw, z=Roll. 단위는 degree.
    Engine::Vector3 rotation = rootComponent->GetLocalRotationEulerDeg();

    const float pitch = rotation.x + deltaPitchDeg;
    const float yaw = rotation.y + deltaYawDeg;

    if (!std::isfinite(pitch) || !std::isfinite(yaw))
    {
        return false;
    }

    rotation.x = std::clamp(pitch, -89.0f, 89.0f);
    rotation.y = std::remainder(yaw, 360.0f);
    if (rotation.y >= 180.0f)
    {
        rotation.y -= 360.0f;
    }

    const float toRadians = PI / 180.0f;

    Engine::Quaternion candidate;
    if (!Engine::Quaternion::TryFromEuler(
        rotation.z * toRadians, // roll
        rotation.x * toRadians, // pitch
        rotation.y * toRadians, // yaw
        candidate
    ))
    {
        return false;
    }

    return rootComponent->TrySetLocalRotation(candidate);
}
