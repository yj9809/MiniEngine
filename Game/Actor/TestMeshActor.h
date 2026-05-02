#pragma once
#include "Actor/Actor.h"
#include "Component/Mesh/MeshRendererComponent.h"
#include "Engine/Engine.h"

class TestMeshActor : public Engine::Actor
{
    RTTI_DECLARATIONS(TestMeshActor, Actor)
    
public:
    void Initialize(Engine::IRenderer* renderer);
    virtual void BeginPlay() override;
    virtual void Tick(float deltaTime) override;
    virtual void Draw() override;
private:
    Engine::IRenderer* renderer = nullptr;
    
    Engine::MeshRendererComponent* meshRenderer = nullptr;
};
