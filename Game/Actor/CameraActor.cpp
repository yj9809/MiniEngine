#include "CameraActor.h"

#include "Component/Transform/TransformComponent.h"
#include "Core/Input.h"
#include "Level/Level.h"

void CameraActor::BeginPlay()
{
    Actor::BeginPlay();
    
    mainCamera = AddComponent<Engine::CameraComponent>();
    GetOwner()->SetMainCamera(mainCamera);
    
}

void CameraActor::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

    Engine::Vector3 rotation = rootComponent->GetLocalRotationEulerDeg();
    float pitch = rotation.x * (PI / 180.f);
    float yaw   = rotation.y * (PI / 180.f);

    Engine::Vector3 forward;
    forward.x = cos(pitch) * sin(yaw);
    forward.y = -sin(pitch);
    forward.z = cos(pitch) * cos(yaw);
    forward = forward.Normalize();

    Engine::Vector3 right = Engine::Vector3::up.Cross(forward).Normalize();

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
        auto delta = Engine::Input::GetMouseDeltaPosition();
        rotation.x += delta.y * 0.1f;
        rotation.y += delta.x * 0.1f;
        rootComponent->SetLocalRotationEulerDeg(rotation);
    }
}
