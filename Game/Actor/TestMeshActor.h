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

    // 회전 테스트용 토글 (1/2/3 키로 각 축 on/off).
    bool rotateX = false; // 1번 → X축(roll)
    bool rotateY = false; // 2번 → Y축(pitch)
    bool rotateZ = false; // 3번 → Z축(yaw)

    // 초당 회전 각도(도).
    float rotationSpeedDeg = 90.0f;
};