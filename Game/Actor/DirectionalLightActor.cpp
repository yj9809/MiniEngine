#include "DirectionalLightActor.h"

#include "Component/Transform/TransformComponent.h"

DirectionalLightActor::DirectionalLightActor()
{
    directionalLight = AddComponent<Engine::DirectionalLightComponent>();
}

void DirectionalLightActor::Initialize()
{
    directionalLight->SetColor({ 1.0f, 1.0f, 1.0f });
    directionalLight->SetIntensity(1.0f);
}

void DirectionalLightActor::Tick(float deltaTime)
{
    Engine::Vector3 rotation = rootComponent->GetLocalRotationEulerDeg();

    rotation.x += 10.0f * deltaTime;
    rootComponent->SetLocalRotationEulerDeg(rotation);
}