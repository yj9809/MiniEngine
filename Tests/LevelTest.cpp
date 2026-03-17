#include <gtest/gtest.h>
#include "Level/Level.h"
#include "Actor/Actor.h"

using namespace Engine;

// protected 멤버에 접근하기 위한 테스트용 서브클래스
class TestLevel : public Level
{
public:
    int GetActorCount() const { return static_cast<int>(actors.size()); }
    int GetPendingActorCount() const { return static_cast<int>(actorsToAdd.size()); }

    // 특정 Actor의 raw pointer를 가져옴 (소유권 없는 참조)
    Actor* GetActor(int index) const { return actors[index].get(); }
};


// --- 액터 추가 테스트 ---

TEST(LevelTest, AddNewActor_PendingUntilProcessed)
{
    TestLevel level;

    // AddNewActor 직후에는 actorsToAdd에만 들어있어야 함
    level.AddNewActor(std::make_unique<Actor>());

    EXPECT_EQ(level.GetActorCount(), 0);
    EXPECT_EQ(level.GetPendingActorCount(), 1);
}

TEST(LevelTest, ProcessAdd_MovesActorToActors)
{
    TestLevel level;

    level.AddNewActor(std::make_unique<Actor>());
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 1);
    EXPECT_EQ(level.GetPendingActorCount(), 0);
}

TEST(LevelTest, AddMultipleActors)
{
    TestLevel level;

    level.AddNewActor(std::make_unique<Actor>());
    level.AddNewActor(std::make_unique<Actor>());
    level.AddNewActor(std::make_unique<Actor>());
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 3);
}

// --- 액터 제거 테스트 ---

TEST(LevelTest, DestroyActor_RemovedOnProcess)
{
    TestLevel level;

    level.AddNewActor(std::make_unique<Actor>());
    level.ProcessAddAndDestroyActor();
    EXPECT_EQ(level.GetActorCount(), 1);

    // OnDestroy 호출 후 Process하면 제거되어야 함
    level.GetActor(0)->OnDestroy();
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 0);
}

TEST(LevelTest, DestroyOneActor_OthersRemain)
{
    TestLevel level;

    level.AddNewActor(std::make_unique<Actor>());
    level.AddNewActor(std::make_unique<Actor>());
    level.ProcessAddAndDestroyActor();

    // 첫 번째 액터만 제거
    level.GetActor(0)->OnDestroy();
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 1);
}

// --- Actor의 owner 설정 테스트 ---

TEST(LevelTest, AddNewActor_SetsOwner)
{
    TestLevel level;

    level.AddNewActor(std::make_unique<Actor>());
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActor(0)->GetOwner(), &level);
}

// --- EndLevel 테스트 ---

TEST(LevelTest, EndLevel_ClearsAllActors)
{
    TestLevel level;

    level.AddNewActor(std::make_unique<Actor>());
    level.AddNewActor(std::make_unique<Actor>());
    level.ProcessAddAndDestroyActor();
    EXPECT_EQ(level.GetActorCount(), 2);

    level.EndLevel();

    EXPECT_EQ(level.GetActorCount(), 0);
    EXPECT_EQ(level.GetPendingActorCount(), 0);
}

TEST(LevelTest, EndLevel_ClearsPendingActors)
{
    TestLevel level;

    // Process 전에 EndLevel 호출
    level.AddNewActor(std::make_unique<Actor>());
    level.EndLevel();

    EXPECT_EQ(level.GetPendingActorCount(), 0);
}
// --- 성능 테스트 ---

TEST(LevelTest, Performance_Add1000Actors)
{
    TestLevel level;

    for (int i = 0; i < 1000; i++)
        level.AddNewActor(std::make_unique<Actor>());

    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 1000);
}

TEST(LevelTest, Performance_Add10000Actors)
{
    TestLevel level;

    for (int i = 0; i < 10000; i++)
        level.AddNewActor(std::make_unique<Actor>());

    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 10000);
}

TEST(LevelTest, Performance_Destroy10000Actors)
{
    TestLevel level;

    for (int i = 0; i < 10000; i++)
        level.AddNewActor(std::make_unique<Actor>());

    level.ProcessAddAndDestroyActor();

    // 전부 제거 요청
    for (int i = 0; i < level.GetActorCount(); i++)
        level.GetActor(i)->OnDestroy();

    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 0);
}

TEST(LevelTest, Performance_AddAndDestroy_Repeated)
{
    TestLevel level;

    // 추가 → 제거를 100회 반복
    for (int round = 0; round < 100; round++)
    {
        for (int i = 0; i < 100; i++)
            level.AddNewActor(std::make_unique<Actor>());

        level.ProcessAddAndDestroyActor();

        for (int i = 0; i < level.GetActorCount(); i++)
            level.GetActor(i)->OnDestroy();

        level.ProcessAddAndDestroyActor();
    }

    EXPECT_EQ(level.GetActorCount(), 0);
}
