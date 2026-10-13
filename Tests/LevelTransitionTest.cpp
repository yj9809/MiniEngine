#include <gtest/gtest.h>

#include <cstdlib>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>

#include "Engine/Engine.h"
#include "Level/Level.h"
#include "Actor/Actor.h"

namespace
{
using Runtime = Engine::Engine;
using Result = Engine::LevelTransitionRequestResult;

struct Probe
{
    int ticks = 0;
    int draws = 0;
    int initialized = 0;
    int begun = 0;
    int ended = 0;
    int destroyed = 0;
};

class TransitionActor final : public Engine::Actor
{
public:
    explicit TransitionActor(Probe& probe) : probe(probe) {}
    std::function<void()> initialize;
    std::function<void()> begin;
    std::function<void()> end;

    void Initialize() override
    {
        ++probe.initialized;
        if (initialize) initialize();
    }
    void BeginPlay() override
    {
        ++probe.begun;
        if (begin) begin();
    }
protected:
    void OnDestroy() override
    {
        ++probe.ended;
        if (end) end();
    }
private:
    Probe& probe;
};

class TransitionLevel final : public Engine::Level
{
public:
    explicit TransitionLevel(Probe& probe) : probe(probe) {}
    ~TransitionLevel() override { ++probe.destroyed; }
    std::function<void()> begin;
    std::function<void()> tick;
    void BeginPlay() override { if (begin) begin(); }
    void Tick(float) override
    {
        ++probe.ticks;
        if (tick) tick();
        // Bound the loop even if a transition is accidentally disconnected.
        if (probe.ticks >= 4) Runtime::QuitEngine();
    }
    void Draw() override { ++probe.draws; }
private:
    Probe& probe;
};

enum class Scenario { Success, LevelFailure, InitializeFailure, BeginFailure, CleanupFailure, QuitPending };

void RunTransitionScenario(Scenario scenario)
{
    Probe oldProbe, nextProbe;
    Runtime* runtime = nullptr;
    auto oldLevel = std::make_unique<TransitionLevel>(oldProbe);
    auto* old = oldLevel.get();
    auto oldActor = std::make_unique<TransitionActor>(oldProbe);
    oldActor->end = [&]
    {
        std::unique_ptr<Engine::Level> rejected = std::make_unique<Engine::Level>();
        EXPECT_EQ(runtime->RequestLevelChange(std::move(rejected)), Result::Busy);
        EXPECT_NE(rejected, nullptr);
        if (scenario == Scenario::CleanupFailure)
            throw std::runtime_error("Intentional old level cleanup failure");
    };
    old->AddNewActor(std::move(oldActor));

    auto nextLevel = std::make_unique<TransitionLevel>(nextProbe);
    auto* next = nextLevel.get();
    next->begin = [&]
    {
        EXPECT_EQ(runtime->GetMainLevel(), old);
        EXPECT_EQ(oldProbe.ended, 0);
        EXPECT_EQ(oldProbe.draws, 1);
        std::unique_ptr<Engine::Level> rejected = std::make_unique<Engine::Level>();
        EXPECT_EQ(runtime->RequestLevelChange(std::move(rejected)), Result::Busy);
        EXPECT_NE(rejected, nullptr);
        if (scenario == Scenario::LevelFailure)
            throw std::runtime_error("Intentional candidate BeginPlay failure");
    };
    auto nextActor = std::make_unique<TransitionActor>(nextProbe);
    nextActor->initialize = [&]
    {
        if (scenario == Scenario::InitializeFailure)
            throw std::runtime_error("Intentional actor Initialize failure");
    };
    nextActor->begin = [&]
    {
        if (scenario == Scenario::BeginFailure)
            throw std::runtime_error("Intentional actor BeginPlay failure");
    };
    next->AddNewActor(std::move(nextActor));
    next->tick = [&]
    {
        EXPECT_EQ(runtime->GetMainLevel(), next);
        EXPECT_EQ(oldProbe.destroyed, 1);
        EXPECT_EQ(oldProbe.ended, 1);
        EXPECT_EQ(nextProbe.initialized, 1);
        EXPECT_EQ(nextProbe.begun, 1);
        Runtime::QuitEngine();
    };

    std::unique_ptr<Engine::Level> pending = std::move(nextLevel);
    const bool fails = scenario == Scenario::LevelFailure ||
        scenario == Scenario::InitializeFailure || scenario == Scenario::BeginFailure;
    old->tick = [&]
    {
        if (oldProbe.ticks == 1)
        {
            std::unique_ptr<Engine::Level> empty;
            EXPECT_EQ(runtime->RequestLevelChange(std::move(empty)), Result::NullLevel);
            EXPECT_EQ(runtime->RequestLevelChange(std::move(pending)), Result::Accepted);
            EXPECT_EQ(pending, nullptr);
            EXPECT_EQ(runtime->GetMainLevel(), old);
            EXPECT_EQ(nextProbe.initialized, 0);
            std::unique_ptr<Engine::Level> rejected = std::make_unique<Engine::Level>();
            EXPECT_EQ(runtime->RequestLevelChange(std::move(rejected)), Result::Busy);
            EXPECT_NE(rejected, nullptr);
            if (scenario == Scenario::QuitPending) Runtime::QuitEngine();
        }
        else if (fails)
        {
            EXPECT_EQ(runtime->GetMainLevel(), old);
            EXPECT_EQ(nextProbe.destroyed, 1);
            EXPECT_EQ(oldProbe.ended, 0);
            auto error = runtime->ConsumeLevelTransitionError();
            ASSERT_TRUE(error.has_value());
            EXPECT_EQ(error->stage, scenario == Scenario::LevelFailure
                ? Engine::LevelTransitionStage::BeginPlay
                : Engine::LevelTransitionStage::InitialActorActivation);
            EXPECT_FALSE(error->errorMessage.empty());
            EXPECT_FALSE(runtime->ConsumeLevelTransitionError().has_value());
            // Failure must also release the transition guard for the next request.
            std::unique_ptr<Engine::Level> retry = std::make_unique<Engine::Level>();
            EXPECT_EQ(runtime->RequestLevelChange(std::move(retry)), Result::Accepted);
            Runtime::QuitEngine();
        }
    };

    Engine::EngineCreateInfo info;
    info.startupLevel = std::move(oldLevel);
    auto result = Runtime::Create(std::move(info));
    if (auto* error = std::get_if<Engine::EngineInitError>(&result))
    {
        FAIL() << error->errorMessage;
    }
    auto engine = std::move(std::get<std::unique_ptr<Runtime>>(result));
    runtime = engine.get();
    engine->Run();

    if (fails)
    {
        EXPECT_EQ(oldProbe.ticks, 2);
        EXPECT_EQ(nextProbe.ticks, 0);
    }
    else if (scenario == Scenario::QuitPending)
    {
        EXPECT_EQ(runtime->GetMainLevel(), old);
        EXPECT_EQ(nextProbe.initialized, 0);
        EXPECT_EQ(nextProbe.ticks, 0);
    }
    else
    {
        EXPECT_EQ(oldProbe.ticks, 1);
        EXPECT_EQ(oldProbe.draws, 1);
        EXPECT_EQ(nextProbe.ticks, 1);
        EXPECT_FALSE(runtime->ConsumeLevelTransitionError().has_value());
    }
    engine.reset();
    EXPECT_EQ(oldProbe.ended, 1);
    EXPECT_EQ(oldProbe.destroyed, 1);
    EXPECT_EQ(nextProbe.destroyed, 1);
}
}

// Each child starts with a fresh Engine::isQuit and Win32 message queue.
#define TRANSITION_TEST(Name, ScenarioValue) \
    TEST(LevelTransitionDeathTest, Name) \
    { \
        EXPECT_EXIT({ \
            RunTransitionScenario(Scenario::ScenarioValue); \
            std::exit(::testing::Test::HasFailure() ? 1 : 0); \
        }, ::testing::ExitedWithCode(0), ""); \
    }

TRANSITION_TEST(SuccessAtFrameBoundaryAndBusyDuringCallbacks, Success)
TRANSITION_TEST(LevelBeginFailurePreservesOldLevelAndAllowsRetry, LevelFailure)
TRANSITION_TEST(ActorInitializeFailurePreservesOldLevel, InitializeFailure)
TRANSITION_TEST(ActorBeginFailurePreservesOldLevel, BeginFailure)
TRANSITION_TEST(OldCleanupExceptionStillCommitsNewLevel, CleanupFailure)
TRANSITION_TEST(QuitSkipsPendingActivationAndShutdownReleasesIt, QuitPending)

#undef TRANSITION_TEST
