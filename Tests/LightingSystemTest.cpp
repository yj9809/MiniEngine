#include <gtest/gtest.h>

#include <memory>
#include <limits>
#include <utility>
#include <vector>

#include "Actor/Actor.h"
#include "Component/Light/DirectionalLightComponent.h"
#include "Component/Light/SkyLightComponent.h"
#include "Component/Transform/TransformComponent.h"
#include "Level/Level.h"
#include "Renderer/GPUData/LightingPacking.h"
#include "Renderer/RenderData/RenderFrameData.h"
#include "System/LightingSystem.h"

namespace
{
    using Engine::EnvironmentLightingRenderData;
    using Engine::LightingSystem;
    using Engine::SkyLightComponent;
    using Engine::Vector3;

    void ExpectColor(const Vector3& actual, const Vector3& expected)
    {
        EXPECT_FLOAT_EQ(actual.x, expected.x);
        EXPECT_FLOAT_EQ(actual.y, expected.y);
        EXPECT_FLOAT_EQ(actual.z, expected.z);
    }

    void ExpectContributions(const EnvironmentLightingRenderData& actual,
        const Vector3& sky, const Vector3& ground)
    {
        {
            SCOPED_TRACE("skyContribution");
            ExpectColor(actual.skyContribution, sky);
        }
        {
            SCOPED_TRACE("groundContribution");
            ExpectColor(actual.groundContribution, ground);
        }
    }

    void ConfigureLight(SkyLightComponent& light)
    {
        light.SetSkyColor(Vector3(0.25f, 0.5f, 2.0f));
        light.SetGroundColor(Vector3(3.0f, 0.75f, 0.125f));
        light.SetIntensity(2.0f);
    }

    class SkyActor final : public Engine::Actor
    {
    public:
        SkyActor() : light(AddComponent<SkyLightComponent>()) {}

        void DestroyForTest() { DispatchOnDestroy(); }

        SkyLightComponent* light;
    };

    class DirectionalLightQuaternionTest : public ::testing::Test
    {
    protected:
        Engine::Actor actor;
        Engine::DirectionalLightComponent* light = actor.AddComponent<Engine::DirectionalLightComponent>();
        LightingSystem system;

        void SetUp() override
        {
            light->SetColor(Vector3(0.25f, 0.5f, 2.0f));
            light->SetIntensity(3.0f);
            system.Register(light);
        }

        void TearDown() override { system.Unregister(light); }

        void ExpectDirection(const Vector3& expected)
        {
            const auto checkDirection = [&](const Vector3& actual)
            {
                EXPECT_NEAR(actual.x, expected.x, 1e-5f);
                EXPECT_NEAR(actual.y, expected.y, 1e-5f);
                EXPECT_NEAR(actual.z, expected.z, 1e-5f);
                EXPECT_NEAR(actual.Length(), 1.0f, 1e-6f);
            };
            checkDirection(light->GetDirection());
            const auto direct = light->BuildRenderData();
            checkDirection(direct.direction);
            Engine::RenderFrameData frame;
            system.BuildDirectionalLightRenderData(frame.directionalLights);
            ASSERT_EQ(frame.directionalLights.size(), 1u);
            checkDirection(frame.directionalLights[0].direction);
            ExpectColor(frame.directionalLights[0].color, Vector3(0.25f, 0.5f, 2.0f));
            EXPECT_FLOAT_EQ(frame.directionalLights[0].intensity, 3.0f);
            const auto packed = Engine::BuildLightingConstantBuffer(frame);
            ASSERT_EQ(packed.directionalLightCount, 1u);
            const auto& direction = packed.directionalLights[0].direction;
            checkDirection(Vector3(direction.x, direction.y, direction.z));
            EXPECT_FLOAT_EQ(direction.w, 0.0f);
        }
    };
}

TEST_F(DirectionalLightQuaternionTest, EulerAxesPolesAndRollUseLightTravelDirection)
{
    struct Sample { Vector3 euler, direction; };
    const Sample samples[] = {
        {Vector3(0, 0, 0), Vector3(1, 0, 0)},
        {Vector3(0, 90, 0), Vector3(0, 1, 0)},
        {Vector3(0, -90, 0), Vector3(0, -1, 0)},
        {Vector3(90, 0, 0), Vector3(0, 0, 1)},
        {Vector3(-90, 0, 0), Vector3(0, 0, -1)},
        {Vector3(0, 0, 90), Vector3(1, 0, 0)},
        {Vector3(20, 40, 0), Vector3(0.71984631f, 0.60402277f, 0.34202014f)},
        {Vector3(20, 40, 73), Vector3(0.71984631f, 0.60402277f, 0.34202014f)}
    };
    for (const auto& sample : samples)
    {
        SCOPED_TRACE(::testing::Message() << sample.euler.x << ',' << sample.euler.y << ',' << sample.euler.z);
        actor.SetRotation(sample.euler);
        ExpectDirection(sample.direction);
    }
}

TEST_F(DirectionalLightQuaternionTest, DirectQuaternionNormalizationAndEquivalentSignsReachPacking)
{
    for (const auto& rotation : {Engine::Quaternion(1, 1, 1, 1), Engine::Quaternion(-3, -3, -3, -3)})
    {
        ASSERT_TRUE(actor.GetRootComponent()->TrySetLocalRotation(rotation));
        ExpectDirection(Vector3::unitY);
    }
    ASSERT_TRUE(actor.GetRootComponent()->TrySetLocalRotation(Engine::Quaternion(0, 1, 0, 0)));
    ExpectDirection(Vector3(-1, 0, 0));
}

TEST_F(DirectionalLightQuaternionTest, LocalAndWorldIncrementsProduceDifferentCollectedDirections)
{
    auto* transform = actor.GetRootComponent();
    Engine::Quaternion pitch;
    ASSERT_TRUE(Engine::Quaternion::TryFromAxisAngle(Vector3::unitY, -PI * 0.5f, pitch));
    actor.SetRotation(Vector3(0, 90, 0));
    ASSERT_TRUE(transform->TryRotateLocal(pitch));
    ExpectDirection(Vector3::unitZ);
    actor.SetRotation(Vector3(0, 90, 0));
    ASSERT_TRUE(transform->TryRotateWorld(pitch));
    ExpectDirection(Vector3::unitY);
}

TEST_F(DirectionalLightQuaternionTest, PositionAndNonuniformNegativeOrZeroScaleDoNotAffectDirection)
{
    auto* transform = actor.GetRootComponent();
    ASSERT_TRUE(transform->TrySetLocalRotation(Engine::Quaternion(1, 1, 1, 1)));
    for (const auto& position : {Vector3::zero, Vector3(3, -2, 5), Vector3(33554432, 33554432, 33554432)})
    for (const auto& scale : {Vector3::one, Vector3(2, 3, 4), Vector3(-2, 3, -4), Vector3::zero})
    {
        transform->SetLocalPosition(position);
        transform->SetLocalScale(scale);
        transform->GetWorldMatrix();
        ExpectDirection(Vector3::unitY);
    }
}

TEST_F(DirectionalLightQuaternionTest, FailedSetAndCompositionPreserveCollectedDirection)
{
    auto* transform = actor.GetRootComponent();
    ASSERT_TRUE(transform->TrySetLocalRotation(Engine::Quaternion(1, 1, 1, 1)));
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const Engine::Quaternion invalid[] = {
        Engine::Quaternion(0, 0, 0, 0), Engine::Quaternion(0, 0, 0, 1e-6f),
        Engine::Quaternion(nan, 0, 0, 1), Engine::Quaternion(0, inf, 0, 1)
    };
    for (const auto& rotation : invalid)
    {
        ASSERT_FALSE(transform->TrySetLocalRotation(rotation));
        ExpectDirection(Vector3::unitY);
        ASSERT_FALSE(transform->TryRotateLocal(rotation));
        ExpectDirection(Vector3::unitY);
        ASSERT_FALSE(transform->TryRotateWorld(rotation));
        ExpectDirection(Vector3::unitY);
    }
    ASSERT_TRUE(transform->TrySetLocalRotation(Engine::Quaternion()));
    ExpectDirection(Vector3::unitX);
}

TEST(DirectionalLightQuaternionLifecycleTest, LevelRegistrationRotationAndRemovalRefreshCollection)
{
    class LightActor final : public Engine::Actor
    {
    public:
        LightActor() { AddComponent<Engine::DirectionalLightComponent>(); }
        void DestroyForTest() { DispatchOnDestroy(); }
    };
    Engine::Level level;
    auto* system = level.GetLightingSystem();
    auto ownedActor = std::make_unique<LightActor>();
    auto* observedActor = ownedActor.get();
    observedActor->SetRotation(Vector3(-90, 0, 0));
    level.AddNewActor(std::move(ownedActor));
    std::vector<Engine::DirectionalLightRenderData> data;
    system->BuildDirectionalLightRenderData(data);
    EXPECT_TRUE(data.empty());
    level.ProcessAddAndDestroyActor();
    system->BuildDirectionalLightRenderData(data);
    ASSERT_EQ(data.size(), 1u);
    EXPECT_NEAR(data[0].direction.z, -1.0f, 1e-6f);
    ASSERT_TRUE(observedActor->GetRootComponent()->TrySetLocalRotation(Engine::Quaternion(1, 1, 1, 1)));
    system->BuildDirectionalLightRenderData(data);
    ASSERT_EQ(data.size(), 1u);
    EXPECT_NEAR(data[0].direction.y, 1.0f, 1e-6f);
    EXPECT_NEAR(data[0].direction.z, 0.0f, 1e-6f);
    observedActor->DestroyForTest();
    level.ProcessAddAndDestroyActor();
    system->BuildDirectionalLightRenderData(data);
    EXPECT_TRUE(data.empty());
}

TEST(LightingSystemTest, NoSkyLightsProducesZeroContribution)
{
    const LightingSystem system;
    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3::zero, Vector3::zero);
}

TEST(LightingSystemTest, DefaultSkyLightProducesDefaultContribution)
{
    SkyLightComponent light;
    LightingSystem system;
    system.Register(&light);

    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3(0.1f, 0.1f, 0.1f), Vector3(0.1f, 0.1f, 0.1f));
}

TEST(LightingSystemTest, AppliesIntensityOnceWithoutClampingOrChangingSource)
{
    SkyLightComponent light;
    ConfigureLight(light);
    LightingSystem system;
    system.Register(&light);

    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3(0.5f, 1.0f, 4.0f), Vector3(6.0f, 1.5f, 0.25f));
    ExpectColor(light.GetSkyColor(), Vector3(0.25f, 0.5f, 2.0f));
    ExpectColor(light.GetGroundColor(), Vector3(3.0f, 0.75f, 0.125f));
    EXPECT_FLOAT_EQ(light.GetIntensity(), 2.0f);
}

TEST(LightingSystemTest, MultipleLightsSumEachHemisphereIndependently)
{
    SkyLightComponent first;
    ConfigureLight(first);
    SkyLightComponent second;
    second.SetSkyColor(Vector3(2.0f, 0.0f, 1.0f));
    second.SetGroundColor(Vector3(0.0f, 4.0f, 2.0f));
    second.SetIntensity(0.5f);
    LightingSystem system;
    system.Register(&first);
    system.Register(&second);

    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3(1.5f, 1.0f, 4.5f), Vector3(6.0f, 3.5f, 1.25f));
}

TEST(LightingSystemTest, RepeatedCollectionDoesNotAccumulatePreviousResults)
{
    SkyLightComponent light;
    ConfigureLight(light);
    LightingSystem system;
    system.Register(&light);

    for (int collection = 0; collection < 3; ++collection)
    {
        SCOPED_TRACE(collection);
        ExpectContributions(system.BuildEnvironmentLightingRenderData(),
            Vector3(0.5f, 1.0f, 4.0f), Vector3(6.0f, 1.5f, 0.25f));
    }
}

TEST(LightingSystemTest, ChangesAffectNextCollectionAndPreserveEarlierSnapshot)
{
    SkyLightComponent light;
    ConfigureLight(light);
    LightingSystem system;
    system.Register(&light);
    const auto before = system.BuildEnvironmentLightingRenderData();

    light.SetSkyColor(Vector3(2.0f, 4.0f, 6.0f));
    light.SetGroundColor(Vector3(8.0f, 2.0f, 0.0f));
    light.SetIntensity(0.5f);
    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3(1.0f, 2.0f, 3.0f), Vector3(4.0f, 1.0f, 0.0f));
    ExpectContributions(before,
        Vector3(0.5f, 1.0f, 4.0f), Vector3(6.0f, 1.5f, 0.25f));

    light.SetIntensity(0.0f);
    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3::zero, Vector3::zero);
}

TEST(LightingSystemTest, NullAndDuplicateRegistrationDoNotAddContribution)
{
    SkyLightComponent light;
    LightingSystem system;
    system.Register(static_cast<const SkyLightComponent*>(nullptr));
    system.Register(&light);
    system.Register(&light);

    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3(0.1f, 0.1f, 0.1f), Vector3(0.1f, 0.1f, 0.1f));
}

TEST(LightingSystemTest, UnregisterAndClearRemoveContributionsOnNextCollection)
{
    SkyLightComponent first;
    ConfigureLight(first);
    SkyLightComponent second;
    second.SetIntensity(0.5f);
    LightingSystem system;
    system.Register(&first);
    system.Register(&second);
    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3(1.0f, 1.5f, 4.5f), Vector3(6.5f, 2.0f, 0.75f));

    system.Unregister(&first);
    system.Unregister(&first);
    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3(0.5f, 0.5f, 0.5f), Vector3(0.5f, 0.5f, 0.5f));

    system.Clear();
    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3::zero, Vector3::zero);
    system.Register(&first);
    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3(0.5f, 1.0f, 4.0f), Vector3(6.0f, 1.5f, 0.25f));
}

TEST(LightingSystemTest, ActorRotationDoesNotChangeEnvironmentContribution)
{
    SkyActor actor;
    ConfigureLight(*actor.light);
    LightingSystem system;
    system.Register(actor.light);
    const auto before = system.BuildEnvironmentLightingRenderData();

    actor.SetRotation(Vector3(37.0f, 90.0f, 180.0f));
    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        before.skyContribution, before.groundContribution);
}

TEST(LightingSystemTest, EnvironmentCollectionAndRemovalPreserveDirectionalLight)
{
    Engine::Actor actor;
    auto* directional = actor.AddComponent<Engine::DirectionalLightComponent>();
    directional->SetColor(Vector3(0.25f, 0.5f, 1.0f));
    directional->SetIntensity(3.0f);
    SkyLightComponent sky;
    LightingSystem system;
    system.Register(directional);
    system.Register(&sky);
    std::vector<Engine::DirectionalLightRenderData> before;
    system.BuildDirectionalLightRenderData(before);
    ASSERT_EQ(before.size(), 1u);

    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3(0.1f, 0.1f, 0.1f), Vector3(0.1f, 0.1f, 0.1f));
    system.Unregister(&sky);
    ExpectContributions(system.BuildEnvironmentLightingRenderData(),
        Vector3::zero, Vector3::zero);

    std::vector<Engine::DirectionalLightRenderData> after;
    system.BuildDirectionalLightRenderData(after);
    ASSERT_EQ(after.size(), 1u);
    ExpectColor(after[0].direction, before[0].direction);
    ExpectColor(after[0].color, Vector3(0.25f, 0.5f, 1.0f));
    EXPECT_FLOAT_EQ(after[0].intensity, 3.0f);
}

TEST(LightingSystemTest, LevelLifecycleRegistersUpdatesAndRemovesSkyLight)
{
    Engine::Level level;
    auto actor = std::make_unique<SkyActor>();
    auto* observedActor = actor.get();
    auto* system = level.GetLightingSystem();
    level.AddNewActor(std::move(actor));
    ExpectContributions(system->BuildEnvironmentLightingRenderData(),
        Vector3::zero, Vector3::zero);

    level.ProcessAddAndDestroyActor();
    ExpectContributions(system->BuildEnvironmentLightingRenderData(),
        Vector3(0.1f, 0.1f, 0.1f), Vector3(0.1f, 0.1f, 0.1f));

    ConfigureLight(*observedActor->light);
    ExpectContributions(system->BuildEnvironmentLightingRenderData(),
        Vector3(0.5f, 1.0f, 4.0f), Vector3(6.0f, 1.5f, 0.25f));
    observedActor->DestroyForTest();
    level.ProcessAddAndDestroyActor();
    // The actor is now deleted; collection must not retain its component pointer.
    ExpectContributions(system->BuildEnvironmentLightingRenderData(),
        Vector3::zero, Vector3::zero);
}

TEST(LightingSystemTest, EndingLevelRemovesActiveAndPendingSkyLights)
{
    Engine::Level level;
    auto* system = level.GetLightingSystem();
    level.AddNewActor(std::make_unique<SkyActor>());
    level.ProcessAddAndDestroyActor();
    level.AddNewActor(std::make_unique<SkyActor>());
    ExpectContributions(system->BuildEnvironmentLightingRenderData(),
        Vector3(0.1f, 0.1f, 0.1f), Vector3(0.1f, 0.1f, 0.1f));

    level.EndLevel();
    ExpectContributions(system->BuildEnvironmentLightingRenderData(),
        Vector3::zero, Vector3::zero);
}
