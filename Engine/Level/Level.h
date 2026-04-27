#pragma once

#include "Common/RTTI.h"

#include <vector>
#include <memory>

namespace Engine
{
    class Actor;
    class CameraComponent;

    // 게임 월드의 한 장면(스테이지, 씬)을 나타내는 클래스.
    // 액터 목록을 소유하고 생명주기(BeginPlay → Tick → Draw → EndLevel)를 관리한다.
    // 언리얼 엔진의 ULevel과 유사한 역할.
    class ENGINE_API Level : public RTTI
    {
        RTTI_DECLARATIONS(Level, RTTI)

    public:
        Level();
        virtual ~Level();

        // unique_ptr 멤버를 가지므로 복사는 의미가 없다. 명시적으로 삭제.
        Level(const Level&) = delete;
        Level& operator=(const Level&) = delete;

        virtual void BeginPlay();

        // isActive가 true인 액터만 Tick을 호출한다.
        virtual void Tick(float deltaTime);

        virtual void Draw();

        // 즉시 actors에 추가하지 않고 actorsToAdd 버퍼에 먼저 쌓는다.
        // Tick() 순회 도중 배열이 변경되면 이터레이터가 무효화되기 때문이다.
        // 실제 등록은 프레임 끝의 ProcessAddAndDestroyActor()에서 이루어진다.
        void AddNewActor(std::unique_ptr<Actor> actor);

        // 프레임 끝에 호출되어 대기 중인 추가/제거를 일괄 처리한다.
        void ProcessAddAndDestroyActor();

        // 레벨 전환 시 호출. 보유한 모든 액터를 즉시 해제한다.
        virtual void EndLevel();

        inline void SetMainCamera(CameraComponent* camera) { mainCamera = camera; }
        inline CameraComponent* GetMainCamera() const { return mainCamera; }

    protected:
        // 현재 레벨에서 활성 중인 액터 목록. Level이 unique_ptr로 소유권 관리.
        std::vector<std::unique_ptr<Actor>> actors;

        // 다음 프레임에 actors로 이동될 대기 버퍼.
        // Tick() 도중 추가 요청이 들어와도 안전하게 처리할 수 있다.
        std::vector<std::unique_ptr<Actor>> actorsToAdd;

        // 메인 카메라를 찾기 위한 관찰 포인터.
        CameraComponent* mainCamera = nullptr;
    };
}
