#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Actor/Actor.h"
#include "Component/Component.h"

namespace
{
    class RecordingComponent final : public Engine::Component
    {
        RTTI_DECLARATIONS(RecordingComponent, Engine::Component)

    public:
        void Initialize() override
        {
            ++initializeCount;

            if (events != nullptr)
            {
                events->emplace_back("Component");
            }

            if (reenterDuringInitialize)
            {
                DispatchInitialize();
            }
        }

        void BeginPlay() override
        {
            ++beginPlayCount;

            if (events != nullptr)
            {
                events->emplace_back("Component.BeginPlay");
            }

            if (reenterDuringBeginPlay)
            {
                DispatchBeginPlay();
            }
        }

    protected:
        void OnRemove() override
        {
            ++removeCount;

            if (events != nullptr)
            {
                events->emplace_back("Component.OnRemove");
            }
        }

    public:
        std::vector<std::string>* events = nullptr;
        int initializeCount = 0;
        int beginPlayCount = 0;
        int removeCount = 0;
        bool reenterDuringInitialize = false;
        bool reenterDuringBeginPlay = false;
    };

    class RecordingActor final : public Engine::Actor
    {
        RTTI_DECLARATIONS(RecordingActor, Engine::Actor)

    public:
        RecordingActor()
        {
            component = AddComponent<RecordingComponent>();
            component->events = &events;
        }

        void Initialize() override
        {
            ++initializeCount;
            events.emplace_back("Actor");

            if (reenterDuringInitialize)
            {
                DispatchInitialize();
            }
        }

        void BeginPlay() override
        {
            ++beginPlayCount;
            events.emplace_back("Actor.BeginPlay");

            if (reenterDuringBeginPlay)
            {
                DispatchBeginPlay();
            }
        }

        void DestroyForTest()
        {
            DispatchOnDestroy();
        }

    protected:
        void OnDestroy() override
        {
            ++destroyCount;
            events.emplace_back("Actor.OnDestroy");
        }

    public:
        std::vector<std::string> events;
        RecordingComponent* component = nullptr;
        int initializeCount = 0;
        int beginPlayCount = 0;
        int destroyCount = 0;
        bool reenterDuringInitialize = false;
        bool reenterDuringBeginPlay = false;
    };
}

TEST(LifecycleTest, ActorHookRunsBeforeComponentHookWithoutParentCall)
{
    RecordingActor actor;

    actor.DispatchInitialize();

    EXPECT_EQ(actor.initializeCount, 1);
    EXPECT_EQ(actor.component->initializeCount, 1);
    EXPECT_EQ(actor.events, (std::vector<std::string>{ "Actor", "Component" }));
}

TEST(LifecycleTest, ActorDispatchInitializeRunsOnlyOnce)
{
    RecordingActor actor;

    actor.DispatchInitialize();
    actor.DispatchInitialize();

    EXPECT_EQ(actor.initializeCount, 1);
    EXPECT_EQ(actor.component->initializeCount, 1);
}

TEST(LifecycleTest, ActorInitializeReentryIsIgnored)
{
    RecordingActor actor;
    actor.reenterDuringInitialize = true;

    actor.DispatchInitialize();

    EXPECT_EQ(actor.initializeCount, 1);
    EXPECT_EQ(actor.component->initializeCount, 1);
}

TEST(LifecycleTest, ComponentInitializeReentryAndDuplicateDispatchAreIgnored)
{
    RecordingComponent component;
    component.reenterDuringInitialize = true;

    component.DispatchInitialize();
    component.DispatchInitialize();

    EXPECT_EQ(component.initializeCount, 1);
}

TEST(LifecycleTest, ActorBeginPlayHookRunsBeforeComponentHookWithoutParentCall)
{
    RecordingActor actor;
    actor.DispatchInitialize();
    actor.events.clear();

    actor.DispatchBeginPlay();

    EXPECT_EQ(actor.beginPlayCount, 1);
    EXPECT_EQ(actor.component->beginPlayCount, 1);
    EXPECT_EQ(actor.events, (std::vector<std::string>{ "Actor.BeginPlay", "Component.BeginPlay" }));
}

TEST(LifecycleTest, ActorDispatchBeginPlayRunsOnlyOnce)
{
    RecordingActor actor;
    actor.DispatchInitialize();

    actor.DispatchBeginPlay();
    actor.DispatchBeginPlay();

    EXPECT_EQ(actor.beginPlayCount, 1);
    EXPECT_EQ(actor.component->beginPlayCount, 1);
}

TEST(LifecycleTest, ActorBeginPlayReentryIsIgnored)
{
    RecordingActor actor;
    actor.DispatchInitialize();
    actor.reenterDuringBeginPlay = true;

    actor.DispatchBeginPlay();

    EXPECT_EQ(actor.beginPlayCount, 1);
    EXPECT_EQ(actor.component->beginPlayCount, 1);
}

TEST(LifecycleTest, ComponentBeginPlayReentryAndDuplicateDispatchAreIgnored)
{
    RecordingComponent component;
    component.DispatchInitialize();
    component.reenterDuringBeginPlay = true;

    component.DispatchBeginPlay();
    component.DispatchBeginPlay();

    EXPECT_EQ(component.beginPlayCount, 1);
}

TEST(LifecycleTest, ActorDispatchBeginPlayBeforeInitializeIsIgnored)
{
    RecordingActor actor;

    actor.DispatchBeginPlay();

    EXPECT_EQ(actor.beginPlayCount, 0);
    EXPECT_EQ(actor.component->beginPlayCount, 0);

    actor.DispatchInitialize();
    actor.DispatchBeginPlay();

    EXPECT_EQ(actor.beginPlayCount, 1);
    EXPECT_EQ(actor.component->beginPlayCount, 1);
}

TEST(LifecycleTest, ActorDestroyAfterBeginPlayRunsActorThenComponentOnce)
{
    RecordingActor actor;
    actor.DispatchInitialize();
    actor.DispatchBeginPlay();
    actor.events.clear();

    actor.DestroyForTest();
    actor.DestroyForTest();

    EXPECT_FALSE(actor.IsActive());
    EXPECT_TRUE(actor.IsPendingDestroy());
    EXPECT_EQ(actor.destroyCount, 1);
    EXPECT_EQ(actor.component->removeCount, 1);
    EXPECT_EQ(actor.events, (std::vector<std::string>{ "Actor.OnDestroy", "Component.OnRemove" }));
}

TEST(LifecycleTest, ActorDestroyBeforeBeginPlaySkipsActorHookButRemovesComponent)
{
    RecordingActor actor;

    actor.DestroyForTest();

    EXPECT_FALSE(actor.IsActive());
    EXPECT_TRUE(actor.IsPendingDestroy());
    EXPECT_EQ(actor.destroyCount, 0);
    EXPECT_EQ(actor.component->removeCount, 1);
    EXPECT_EQ(actor.events, (std::vector<std::string>{ "Component.OnRemove" }));
}
