#include "TestMeshActor.h"

#include <chrono>

#include "Component/Mesh/MeshRendererComponent.h"
#include "Component/Transform/TransformComponent.h"
#include "Core/Log.h"
#include "Core/Time.h"
#include "Level/Level.h"
#include "Renderer/Mesh.h"

void TestMeshActor::Initialize(Engine::IRenderer* renderer)
{
    this->renderer = renderer;
}

void TestMeshActor::BeginPlay()
{
    Actor::BeginPlay();
    
    meshRenderer = AddComponent<Engine::MeshRendererComponent>();
    meshRenderer->SetMesh(Engine::Mesh::LoadFromOBJ(renderer, "C:/Users/dkrkt/OneDrive/Desktop/MiniEngine/Engine/Asset/Cube.obj"));
    
    meshRenderer->Initialize(renderer);
    
    rootComponent->SetLocalScale({ 0.03f, 0.03f, 0.03f });
    rootComponent->SetLocalPosition({ 0.0f, -1.0f, 3.f });
}

void TestMeshActor::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);
    
    // Engine::Vector3 rotation = rootComponent->GetLocalRotationEulerDeg();
    // rotation.z += deltaTime * 180.0f; // 초당 45도 회전
    // rootComponent->SetLocalRotationEulerDeg(rotation);
    // Engine::Vector3 scale = rootComponent->GetLocalScale();
    // float scaleAmount = 0.5f * sinf(Engine::Time::GetTotalTime() * 2.0f) + 1.0f;
    // rootComponent->SetLocalScale({ scaleAmount, scaleAmount, 1.0f });
}

void TestMeshActor::Draw()
{
    Actor::Draw();
}
