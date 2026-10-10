#include <stdexcept>
#include <gtest/gtest.h>

#include "Engine/Engine.h"
#include "Engine/EngineInitialization.h"
#include "Level/Level.h"

class FailingStartupLevel final : public Engine::Level
{
public:
    void BeginPlay() override
    {
        // BeginPlay()에서 예외를 던져 엔진 초기화 실패를 시뮬레이션.
        throw std::runtime_error("Intentional startup failure");
    }
};

TEST(EngineInitializationTest, StartupLevelFailureReleasesResourcesForNextCreate)
{
    Engine::EngineCreateInfo failingInfo;
    failingInfo.startupLevel = std::make_unique<FailingStartupLevel>();

    auto failedResult = Engine::Engine::Create(std::move(failingInfo));

    const auto* error = std::get_if<Engine::EngineInitError>(&failedResult);

    ASSERT_NE(error, nullptr);
    EXPECT_EQ( error->engineInitialization, Engine::EngineInitialization::StartupLevel);

    Engine::EngineCreateInfo retryInfo;
    retryInfo.startupLevel = std::make_unique<Engine::Level>();

    auto retryResult = Engine::Engine::Create(std::move(retryInfo));

    EXPECT_TRUE( std::holds_alternative<std::unique_ptr<Engine::Engine>>(retryResult));
}
