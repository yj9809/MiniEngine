#pragma once

#include "Actor/Actor.h"
#include "Component/Mesh/MeshRendererComponent.h"

class TestMeshActor : public Engine::Actor
{
    RTTI_DECLARATIONS(TestMeshActor, Actor)
    
public:
    TestMeshActor();

    virtual void Initialize() override;
    virtual void BeginPlay() override;
    virtual void Tick(float deltaTime) override;

private:
    Engine::MeshRendererComponent* meshRenderer = nullptr;

    bool rotatePitch = false;
    bool rotateYaw = false;
    bool rotateRoll = false;

    // 초당 회전 각도(도).
    float rotationSpeedDeg = 90.0f;
};