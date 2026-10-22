#pragma once

#include "Actor/Actor.h"
#include "Component/Mesh/MeshRendererComponent.h"
#include "Math/Vector3.h"

class TestMeshActor : public Engine::Actor
{
    RTTI_DECLARATIONS(TestMeshActor, Actor)

    enum class RotationMode
    {
        EulerAccumulation,
        QuaternionIncrement
    };
    
public:
    TestMeshActor();

    virtual void Initialize() override;
    virtual void BeginPlay() override;
    virtual void Tick(float deltaTime) override;

private:
    /**
     * 회전 비교 데모를 지정한 초기 자세로 되돌립니다.
     * Transform의 Quaternion과 Euler 누적값을 같은 자세로 맞추고,
     * Pitch/Yaw/Roll 회전 토글을 모두 끕니다.
     * 현재 회전 모드, 위치, 스케일은 유지합니다.
     * 실패하면 기존 자세와 데모 상태를 유지합니다.
     *
     * @param initialEulerDeg 초기 자세. x=Pitch, y=Yaw, z=Roll, 단위는 도.
     * @return 초기화에 성공하면 true, 실패하면 false.
     */
    bool TryResetRotationDemo(const Engine::Vector3& initialEulerDeg);

private:
    Engine::MeshRendererComponent* meshRenderer = nullptr;

    bool rotatePitch = false;
    bool rotateYaw = false;
    bool rotateRoll = false;

    // 초당 회전 각도(도).
    float rotationSpeedDeg = 90.0f;

    // 처음에는 기존 방식과 가까운 Euler 누적 모드로 시작.
    RotationMode rotationMode = RotationMode::EulerAccumulation;

    // Euler 비교 모드의 입력 상태입니다.
    // x=Pitch, y=Yaw, z=Roll이며 단위는 Degree입니다.
    // 매 프레임 Quaternion에서 역변환하지 않고 이 값에 누적합니다.
    Engine::Vector3 accumulatedEulerDeg = Engine::Vector3::zero;
};