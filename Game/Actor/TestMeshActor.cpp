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

    // 1/2/3 키로 각 축 회전을 토글 (한 번 누르면 시작, 다시 누르면 정지).
    if (Engine::Input::GetKeyDown('1')) rotateX = !rotateX;
    if (Engine::Input::GetKeyDown('2')) rotateY = !rotateY;
    if (Engine::Input::GetKeyDown('3')) rotateZ = !rotateZ;

    // 켜진 축만 매 프레임 각도를 누적.
    Engine::Vector3 rotation = rootComponent->GetLocalRotationEulerDeg();
    if (rotateX) rotation.x += rotationSpeedDeg * deltaTime;
    if (rotateY) rotation.y += rotationSpeedDeg * deltaTime;
    if (rotateZ) rotation.z += rotationSpeedDeg * deltaTime;
    rootComponent->SetLocalRotationEulerDeg(rotation);
}
