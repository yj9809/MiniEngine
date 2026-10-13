#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

#include "Actor/Actor.h"
#include "Component/Camera/CameraComponent.h"
#include "Level/Level.h"

namespace
{
    struct ActorProbe
    {
        int initialized = 0;
        int begun = 0;
        int ticked = 0;
        int ended = 0;
        int deleted = 0;
    };

    class CallbackActor final : public Engine::Actor
    {
    public:
        explicit CallbackActor(ActorProbe& probe) : probe(probe) {}
        ~CallbackActor() override
        {
            ++probe.deleted;
            if (onDelete) onDelete();
        }
        void Initialize() override
        {
            ++probe.initialized;
            if (onInitialize) onInitialize();
        }
        void BeginPlay() override
        {
            ++probe.begun;
            if (onBegin) onBegin();
        }
        void Tick(float) override
        {
            ++probe.ticked;
            if (onTick) onTick();
        }
        std::function<void()> onInitialize, onBegin, onTick, onEnd, onDelete;

    protected:
        void OnDestroy() override
        {
            ++probe.ended;
            if (onEnd) onEnd();
        }

    private:
        ActorProbe& probe;
    };

    class BoundaryLevel final : public Engine::Level
    {
    public:
        size_t ActiveCount() const { return actors.size(); }
        size_t PendingCount() const { return actorsToAdd.size(); }
    };

    // false: Initialize callback, true: BeginPlay callback.
    class LevelProcessingBoundaryTest : public ::testing::TestWithParam<bool>
    {
    protected:
        void SetCallback(CallbackActor& actor, std::function<void()> callback)
        {
            if (GetParam()) actor.onBegin = std::move(callback);
            else actor.onInitialize = std::move(callback);
        }
    };
}

TEST_P(LevelProcessingBoundaryTest, RejectsAdditionAndDestroysTransferredActor)
{
    ActorProbe parentProbe, rejectedProbe;
    BoundaryLevel level;
    auto parent = std::make_unique<CallbackActor>(parentProbe);
    bool called = false;
    SetCallback(*parent, [&]()
    {
        called = true;
        auto rejected = std::make_unique<CallbackActor>(rejectedProbe);
        EXPECT_THROW(level.AddNewActor(std::move(rejected)), std::logic_error);
        EXPECT_EQ(rejected, nullptr);
        EXPECT_EQ(rejectedProbe.deleted, 1);
        EXPECT_EQ(level.PendingCount(), 1u);
    });
    level.AddNewActor(std::move(parent));
    ASSERT_NO_THROW(level.ProcessAddAndDestroyActor());
    EXPECT_TRUE(called);
    EXPECT_EQ(level.ActiveCount(), 1u);
    EXPECT_EQ(level.PendingCount(), 0u);
    EXPECT_EQ(rejectedProbe.initialized, 0);
    EXPECT_EQ(rejectedProbe.begun, 0);
    level.EndLevel();
}

TEST_P(LevelProcessingBoundaryTest, RejectsReentrantProcessing)
{
    ActorProbe probe;
    BoundaryLevel level;
    auto actor = std::make_unique<CallbackActor>(probe);
    bool called = false;
    SetCallback(*actor, [&]()
    {
        called = true;
        EXPECT_THROW(level.ProcessAddAndDestroyActor(), std::logic_error);
    });
    level.AddNewActor(std::move(actor));
    ASSERT_NO_THROW(level.ProcessAddAndDestroyActor());
    EXPECT_TRUE(called);
    EXPECT_EQ(probe.initialized, 1);
    EXPECT_EQ(probe.begun, 1);
    EXPECT_EQ(level.ActiveCount(), 1u);
    // A completed batch must release the processing guard.
    EXPECT_NO_THROW(level.ProcessAddAndDestroyActor());
    level.EndLevel();
}

TEST_P(LevelProcessingBoundaryTest, RejectsEndLevelWithoutEndingOuterProcessing)
{
    ActorProbe probe, laterProbe;
    BoundaryLevel level;
    auto actor = std::make_unique<CallbackActor>(probe);
    bool called = false;
    SetCallback(*actor, [&]()
    {
        called = true;
        EXPECT_THROW(level.EndLevel(), std::logic_error);
        EXPECT_EQ(probe.deleted, 0);
    });
    level.AddNewActor(std::move(actor));
    ASSERT_NO_THROW(level.ProcessAddAndDestroyActor());
    EXPECT_TRUE(called);
    EXPECT_EQ(probe.begun, 1);
    // Rejected EndLevel must not set hasEnded; successful processing releases the guard.
    ASSERT_NO_THROW(level.AddNewActor(std::make_unique<CallbackActor>(laterProbe)));
    ASSERT_NO_THROW(level.ProcessAddAndDestroyActor());
    EXPECT_EQ(laterProbe.begun, 1);
    EXPECT_NO_THROW(level.EndLevel());
    EXPECT_EQ(probe.ended, 1);
    EXPECT_EQ(laterProbe.deleted, 1);
}

TEST_P(LevelProcessingBoundaryTest, EscapingExceptionReleasesGuardForCleanup)
{
    ActorProbe firstProbe, failedProbe, pendingProbe;
    BoundaryLevel level;
    level.AddNewActor(std::make_unique<CallbackActor>(firstProbe));
    auto failed = std::make_unique<CallbackActor>(failedProbe);
    SetCallback(*failed, []() { throw std::runtime_error("activation failed"); });
    level.AddNewActor(std::move(failed));
    level.AddNewActor(std::make_unique<CallbackActor>(pendingProbe));

    EXPECT_THROW(level.ProcessAddAndDestroyActor(), std::runtime_error);
    EXPECT_EQ(level.ActiveCount(), 1u);
    EXPECT_EQ(pendingProbe.initialized, 0);
    // A partially moved batch is not retried. EndLevel must handle its null entries.
    EXPECT_NO_THROW(level.EndLevel());
    EXPECT_EQ(firstProbe.ended, 1);
    EXPECT_EQ(firstProbe.deleted, 1);
    EXPECT_EQ(failedProbe.deleted, 1);
    EXPECT_EQ(pendingProbe.deleted, 1);
    EXPECT_EQ(level.ActiveCount(), 0u);
    EXPECT_EQ(level.PendingCount(), 0u);
}

INSTANTIATE_TEST_SUITE_P(
    LifecycleCallbacks, LevelProcessingBoundaryTest, ::testing::Bool(),
    [](const ::testing::TestParamInfo<bool>& info)
    { return info.param ? "BeginPlay" : "Initialize"; });

TEST(LevelBoundaryTest, RejectsNullActorWithoutChangingQueues)
{
    BoundaryLevel level;
    EXPECT_THROW(level.AddNewActor(nullptr), std::invalid_argument);
    EXPECT_EQ(level.ActiveCount(), 0u);
    EXPECT_EQ(level.PendingCount(), 0u);
}

TEST(LevelBoundaryTest, RejectsAdditionAfterEndAndProcessingRemainsNoOp)
{
    ActorProbe probe;
    BoundaryLevel level;
    level.EndLevel();
    EXPECT_THROW(level.AddNewActor(std::make_unique<CallbackActor>(probe)), std::logic_error);
    EXPECT_EQ(probe.deleted, 1);
    EXPECT_NO_THROW(level.ProcessAddAndDestroyActor());
    EXPECT_EQ(level.ActiveCount(), 0u);
    EXPECT_EQ(level.PendingCount(), 0u);
}

TEST(LevelBoundaryTest, RejectsAdditionDuringEndCallback)
{
    ActorProbe probe, rejectedProbe;
    BoundaryLevel level;
    auto actor = std::make_unique<CallbackActor>(probe);
    actor->onEnd = [&]()
    {
        EXPECT_THROW(level.AddNewActor(std::make_unique<CallbackActor>(rejectedProbe)),
            std::logic_error);
        EXPECT_NO_THROW(level.EndLevel());
        EXPECT_NO_THROW(level.ProcessAddAndDestroyActor());
    };
    level.AddNewActor(std::move(actor));
    level.ProcessAddAndDestroyActor();
    EXPECT_NO_THROW(level.EndLevel());
    EXPECT_EQ(probe.ended, 1);
    EXPECT_EQ(probe.deleted, 1);
    EXPECT_EQ(rejectedProbe.deleted, 1);
    EXPECT_EQ(level.PendingCount(), 0u);
}

TEST(LevelBoundaryTest, TickAdditionIsDeferredUntilFrameBoundary)
{
    ActorProbe parentProbe, childProbe;
    BoundaryLevel level;
    auto parent = std::make_unique<CallbackActor>(parentProbe);
    bool added = false;
    parent->onTick = [&]()
    {
        if (!added)
        {
            level.AddNewActor(std::make_unique<CallbackActor>(childProbe));
            added = true;
        }
    };
    level.AddNewActor(std::move(parent));
    level.ProcessAddAndDestroyActor();
    ASSERT_NO_THROW(level.Tick(0.016f));
    EXPECT_EQ(level.ActiveCount(), 1u);
    EXPECT_EQ(level.PendingCount(), 1u);
    EXPECT_EQ(childProbe.initialized, 0);
    EXPECT_EQ(childProbe.ticked, 0);
    level.ProcessAddAndDestroyActor();
    EXPECT_EQ(childProbe.initialized, 1);
    EXPECT_EQ(childProbe.begun, 1);
    EXPECT_EQ(childProbe.ticked, 0);
    level.Tick(0.016f);
    EXPECT_EQ(childProbe.ticked, 1);
    level.EndLevel();
}

namespace
{
    // false: pending removal, true: active removal.
    class LevelCameraBoundaryTest : public ::testing::TestWithParam<bool> {};
}

TEST_P(LevelCameraBoundaryTest, ClearsMainCameraBeforeOwnerMemoryIsReleased)
{
    ActorProbe probe;
    BoundaryLevel level;
    auto actor = std::make_unique<CallbackActor>(probe);
    auto* actorPtr = actor.get();
    auto* camera = actor->AddComponent<Engine::CameraComponent>();
    actor->onDelete = [&]() { EXPECT_EQ(level.GetMainCamera(), nullptr); };
    level.SetMainCamera(camera);
    level.AddNewActor(std::move(actor));
    if (GetParam()) level.ProcessAddAndDestroyActor();
    actorPtr->DispatchOnDestroy();
    level.ProcessAddAndDestroyActor();
    EXPECT_EQ(level.GetMainCamera(), nullptr);
    EXPECT_EQ(probe.deleted, 1);
    EXPECT_EQ(probe.initialized, GetParam() ? 1 : 0);
    EXPECT_EQ(probe.begun, GetParam() ? 1 : 0);
    EXPECT_EQ(level.ActiveCount(), 0u);
    EXPECT_EQ(level.PendingCount(), 0u);
    level.EndLevel();
}

TEST_P(LevelCameraBoundaryTest, RemovingOtherCameraActorPreservesMainCamera)
{
    ActorProbe mainProbe, otherProbe;
    BoundaryLevel level;
    auto mainActor = std::make_unique<CallbackActor>(mainProbe);
    auto* mainCamera = mainActor->AddComponent<Engine::CameraComponent>();
    level.SetMainCamera(mainCamera);
    level.AddNewActor(std::move(mainActor));
    level.ProcessAddAndDestroyActor();

    auto other = std::make_unique<CallbackActor>(otherProbe);
    other->AddComponent<Engine::CameraComponent>();
    auto* otherPtr = other.get();
    level.AddNewActor(std::move(other));
    if (GetParam()) level.ProcessAddAndDestroyActor();
    otherPtr->DispatchOnDestroy();
    level.ProcessAddAndDestroyActor();
    EXPECT_EQ(level.GetMainCamera(), mainCamera);
    EXPECT_EQ(otherProbe.deleted, 1);
    EXPECT_EQ(mainProbe.deleted, 0);
    EXPECT_EQ(level.ActiveCount(), 1u);
    level.EndLevel();
}

INSTANTIATE_TEST_SUITE_P(
    ActorQueues, LevelCameraBoundaryTest, ::testing::Bool(),
    [](const ::testing::TestParamInfo<bool>& info)
    { return info.param ? "Active" : "Pending"; });
