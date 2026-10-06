#include "TestMeshActor.h"

#include <memory>

#include "Component/Mesh/MeshRendererComponent.h"
#include "Component/Transform/TransformComponent.h"
#include "Core/Input.h"
#include "Resource/ResourceManager.h"
#include "Level/Level.h"
#include "Renderer/Material.h"
#include "Math/Vector3.h"

TestMeshActor::TestMeshActor()
{
    meshRenderer = AddComponent<Engine::MeshRendererComponent>();
}

void TestMeshActor::Initialize()
{
    Actor::Initialize();

    Engine::Level* ownerLevel = GetOwner();

    if(!ownerLevel)
    {
        return;
    }

    Engine::ResourceManager& resourceManager = ownerLevel->GetResourceManager();

    meshRenderer->SetMesh(resourceManager.LoadMesh("Asset/world.obj"));

    auto material = std::make_shared<Engine::Material>();
    material->SetMainTexture(resourceManager.LoadTexture(L"Asset/world_giant.jpg"));

    material->SetBaseColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // RGBA
    meshRenderer->SetMaterial(0, material);
}

void TestMeshActor::BeginPlay()
{
    Actor::BeginPlay();
    
    rootComponent->SetLocalPosition({ 2.0f, 0.0f, 0.0f });
}

void TestMeshActor::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

    Engine::Vector3 rotation = rootComponent->GetLocalRotationEulerDeg();

    // 1/2/3 키로 Pitch/Yaw/Roll 회전을 토글.
    if (Engine::Input::GetKeyDown('1')) rotatePitch = !rotatePitch;
    if (Engine::Input::GetKeyDown('2')) rotateYaw = !rotateYaw;
    if (Engine::Input::GetKeyDown('3')) rotateRoll = !rotateRoll;

    if (rotatePitch) rotation.x += rotationSpeedDeg * deltaTime;
    if (rotateYaw) rotation.y += rotationSpeedDeg * deltaTime;
    if (rotateRoll) rotation.z += rotationSpeedDeg * deltaTime;
    rootComponent->SetLocalRotationEulerDeg(rotation);
}
