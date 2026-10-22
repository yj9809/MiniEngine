#pragma once

#include "Actor/Actor.h"
#include "Component/Camera/CameraComponent.h"

class CameraActor : public Engine::Actor
{
    RTTI_DECLARATIONS(CameraActor, Actor)
    
public:
    CameraActor();
    
    virtual void BeginPlay() override;
    virtual void Tick(float deltaTime) override;

private:
    /**
     * 현재 자세의 Euler 각도에 시선 회전 입력을 적용합니다.
     * Pitch는 [-89, 89]도로 제한하고 Yaw는 [-180, 180)도로 정리합니다.
     * 조회된 Roll 각도는 유지하며 결과는 Quaternion으로 저장합니다.
     * 입력이 모두 0이면 자세를 변경하지 않습니다.
     * 실패하면 기존 자세를 유지합니다.
     * @param deltaYawDeg Yaw 변화량(도). 유한한 값이어야 합니다.
     * @param deltaPitchDeg Pitch 변화량(도). 유한한 값이어야 합니다.
     * @return 입력 처리에 성공하면 true, 실패하면 false.
     */
    bool TryApplyLookInputDeg(float deltaYawDeg, float deltaPitchDeg);

private:
    Engine::CameraComponent* mainCamera;
};
