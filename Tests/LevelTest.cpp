#include <gtest/gtest.h>
#include "Level/Level.h"
#include "Actor/Actor.h"

using namespace Engine;

class TestActor final : public Engine::Actor
{
public:
    void DestroyForTest()
    {
        DispatchOnDestroy();
    }
};

// Test-only access to Level-owned actor containers.
class InspectableLevel final : public Engine::Level
{
public:
    int GetActorCount() const { return static_cast<int>(actors.size()); }
    int GetPendingActorCount() const { return static_cast<int>(actorsToAdd.size()); }

    // Non-owning access to actors managed by the Level.
    Actor* GetActor(int index) const { return actors[index].get(); }
    TestActor* GetTestActor(int index) const { return static_cast<TestActor*>(actors[index].get()); }
};


// Actor addition.

TEST(LevelTest, AddNewActor_PendingUntilProcessed)
{
    InspectableLevel level;

    // AddNewActor keeps the actor pending until the frame-boundary processing.
    level.AddNewActor(std::make_unique<TestActor>());

    EXPECT_EQ(level.GetActorCount(), 0);
    EXPECT_EQ(level.GetPendingActorCount(), 1);
}

TEST(LevelTest, ProcessAdd_MovesActorToActors)
{
    InspectableLevel level;

    level.AddNewActor(std::make_unique<TestActor>());
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 1);
    EXPECT_EQ(level.GetPendingActorCount(), 0);
}

TEST(LevelTest, AddMultipleActors)
{
    InspectableLevel level;

    level.AddNewActor(std::make_unique<TestActor>());
    level.AddNewActor(std::make_unique<TestActor>());
    level.AddNewActor(std::make_unique<TestActor>());
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 3);
}

// Actor removal.

TEST(LevelTest, DestroyActor_RemovedOnProcess)
{
    InspectableLevel level;

    level.AddNewActor(std::make_unique<TestActor>());
    level.ProcessAddAndDestroyActor();
    EXPECT_EQ(level.GetActorCount(), 1);

    // A dispatched actor is removed during frame-boundary processing.
    level.GetTestActor(0)->DestroyForTest();
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 0);
}

TEST(LevelTest, DestroyOneActor_OthersRemain)
{
    InspectableLevel level;

    level.AddNewActor(std::make_unique<TestActor>());
    level.AddNewActor(std::make_unique<TestActor>());
    level.ProcessAddAndDestroyActor();

    // Remove only the first actor.
    level.GetTestActor(0)->DestroyForTest();
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 1);
}

// Actor owner assignment.

TEST(LevelTest, AddNewActor_SetsOwner)
{
    InspectableLevel level;

    level.AddNewActor(std::make_unique<TestActor>());
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActor(0)->GetOwner(), &level);
}

// EndLevel.

TEST(LevelTest, EndLevel_ClearsAllActors)
{
    InspectableLevel level;

    level.AddNewActor(std::make_unique<TestActor>());
    level.AddNewActor(std::make_unique<TestActor>());
    level.ProcessAddAndDestroyActor();
    EXPECT_EQ(level.GetActorCount(), 2);

    level.EndLevel();

    EXPECT_EQ(level.GetActorCount(), 0);
    EXPECT_EQ(level.GetPendingActorCount(), 0);
}

TEST(LevelTest, EndLevel_ClearsPendingActors)
{
    InspectableLevel level;

    // EndLevel also destroys actors that are still pending.
    level.AddNewActor(std::make_unique<TestActor>());
    level.EndLevel();

    EXPECT_EQ(level.GetPendingActorCount(), 0);
}
// Bulk lifecycle coverage.

TEST(LevelTest, Performance_Add1000Actors)
{
    InspectableLevel level;

    for (int i = 0; i < 1000; i++)
        level.AddNewActor(std::make_unique<TestActor>());

    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 1000);
}

TEST(LevelTest, Performance_Add10000Actors)
{
    InspectableLevel level;

    for (int i = 0; i < 10000; i++)
        level.AddNewActor(std::make_unique<TestActor>());

    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 10000);
}

TEST(LevelTest, Performance_Destroy10000Actors)
{
    InspectableLevel level;

    for (int i = 0; i < 10000; i++)
        level.AddNewActor(std::make_unique<TestActor>());

    level.ProcessAddAndDestroyActor();

    // Dispatch removal for every actor.
    for (int i = 0; i < level.GetActorCount(); i++)
        level.GetTestActor(i)->DestroyForTest();

    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(level.GetActorCount(), 0);
}

TEST(LevelTest, Performance_AddAndDestroy_Repeated)
{
    InspectableLevel level;

    // Repeat add and removal processing.
    for (int round = 0; round < 100; round++)
    {
        for (int i = 0; i < 100; i++)
            level.AddNewActor(std::make_unique<TestActor>());

        level.ProcessAddAndDestroyActor();

        for (int i = 0; i < level.GetActorCount(); i++)
            level.GetTestActor(i)->DestroyForTest();

        level.ProcessAddAndDestroyActor();
    }

    EXPECT_EQ(level.GetActorCount(), 0);
}
