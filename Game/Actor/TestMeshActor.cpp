#include "TestMeshActor.h"

#include <chrono>

#include "Component/Mesh/MeshRendererComponent.h"
#include "Component/Transform/TransformComponent.h"
#include "Core/Log.h"
#include "Core/Time.h"
#include "Level/Level.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"

void TestMeshActor::Initialize(Engine::IRenderer* renderer)
{
    this->renderer = renderer;
}

void TestMeshActor::BeginPlay()
{
    Actor::BeginPlay();
    
    meshRenderer = AddComponent<Engine::MeshRendererComponent>();
    meshRenderer->SetMesh(Engine::Mesh::LoadFromOBJ(renderer, "C:/Users/dkrkt/OneDrive/Desktop/MiniEngine/Engine/Asset/world.obj"));
    meshRenderer->SetTexture(Engine::Texture::LoadFromFile(renderer, L"D:/모델링/world/world_giant.jpg"));
    meshRenderer->Initialize(renderer);
    
    rootComponent->SetLocalPosition({ 10.0f, 0.0f, 0.0f });
    
    // --- RotationX 부호 검증 로그 ---
    const float rad90 = 3.14159265f * 0.5f;   // 90도 = π/2
    Engine::Matrix4 rx = Engine::Matrix4::RotationX(rad90);
    Engine::Vector3 v = rx.TransformVector(Engine::Vector3(0.0f, 1.0f, 0.0f));
    Engine::Log::Info("RotationX(90) * (0,1,0) = ({}, {}, {})", v.x, v.y, v.z);
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

void TestMeshActor::Draw()
{
    Actor::Draw();
}
