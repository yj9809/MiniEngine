#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "Actor/Actor.h"
#include "Component/Light/DirectionalLightComponent.h"
#include "Component/Light/SkyLightComponent.h"
#include "Level/Level.h"
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
