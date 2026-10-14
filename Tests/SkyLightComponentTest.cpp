#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <limits>
#include <string>

#include "Actor/Actor.h"
#include "Component/Light/SkyLightComponent.h"

namespace
{
    using Engine::SkyLightComponent;
    using Engine::Vector3;

    void ExpectColor(const Vector3& actual, const Vector3& expected)
    {
        EXPECT_FLOAT_EQ(actual.x, expected.x);
        EXPECT_FLOAT_EQ(actual.y, expected.y);
        EXPECT_FLOAT_EQ(actual.z, expected.z);
    }

    void SetBaseline(SkyLightComponent& light)
    {
        light.SetSkyColor(Vector3(0.25f, 0.5f, 2.0f));
        light.SetGroundColor(Vector3(3.0f, 0.75f, 0.125f));
        light.SetIntensity(1.5f);
    }

    void ExpectBaseline(const SkyLightComponent& light)
    {
        ExpectColor(light.GetSkyColor(), Vector3(0.25f, 0.5f, 2.0f));
        ExpectColor(light.GetGroundColor(), Vector3(3.0f, 0.75f, 0.125f));
        EXPECT_FLOAT_EQ(light.GetIntensity(), 1.5f);
    }

    std::array<float, 4> InvalidValues()
    {
        return {
            -1.0f,
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity()
        };
    }

    // Capture the stderr sink, not OutputDebugString. Always restore it,
    // including when a tested call throws. Do not nest captures.
    class ScopedStderrCapture final
    {
    public:
        ScopedStderrCapture()
        {
            testing::internal::CaptureStderr();
        }

        ~ScopedStderrCapture()
        {
            if (active)
            {
                testing::internal::GetCapturedStderr();
            }
        }

        ScopedStderrCapture(const ScopedStderrCapture&) = delete;
        ScopedStderrCapture& operator=(const ScopedStderrCapture&) = delete;

        std::string Finish()
        {
            std::string result = testing::internal::GetCapturedStderr();
            active = false;
            return result;
        }

    private:
        bool active = true;
    };

    class RemovalProbe final : public SkyLightComponent
    {
        RTTI_DECLARATIONS(RemovalProbe, SkyLightComponent)

    public:
        bool removalObserved = false;

    protected:
        void OnRemove() override
        {
            removalObserved = true;
            // Valid inputs must still be rejected while EndingPlay.
            SetSkyColor(Vector3(4.0f, 5.0f, 6.0f));
            SetGroundColor(Vector3(7.0f, 8.0f, 9.0f));
            SetIntensity(10.0f);
            ExpectBaseline(*this);
        }
    };
}

TEST(SkyLightComponentTest, DefaultsAreWhiteColorsAndPointOneIntensity)
{
    SkyLightComponent light;

    ExpectColor(light.GetSkyColor(), Vector3::one);
    ExpectColor(light.GetGroundColor(), Vector3::one);
    EXPECT_FLOAT_EQ(light.GetIntensity(), 0.1f);
}

TEST(SkyLightComponentTest, SettersAcceptZeroAndValuesAboveOneIndependently)
{
    SkyLightComponent light;

    for (float value : {0.0f, 2.0f})
    {
        SCOPED_TRACE(value);
        SetBaseline(light);
        const Vector3 color(value, value, value);

        light.SetSkyColor(color);
        ExpectColor(light.GetSkyColor(), color);
        ExpectColor(light.GetGroundColor(), Vector3(3.0f, 0.75f, 0.125f));
        EXPECT_FLOAT_EQ(light.GetIntensity(), 1.5f);

        light.SetGroundColor(color);
        ExpectColor(light.GetSkyColor(), color);
        ExpectColor(light.GetGroundColor(), color);
        EXPECT_FLOAT_EQ(light.GetIntensity(), 1.5f);

        light.SetIntensity(value);
        ExpectColor(light.GetSkyColor(), color);
        ExpectColor(light.GetGroundColor(), color);
        EXPECT_FLOAT_EQ(light.GetIntensity(), value);
    }
}

TEST(SkyLightComponentTest, InvalidColorInAnyChannelPreservesAllSettings)
{
    using Setter = void (SkyLightComponent::*)(const Vector3&);
    const std::array<Setter, 2> setters = {
        &SkyLightComponent::SetSkyColor,
        &SkyLightComponent::SetGroundColor
    };

    SkyLightComponent light;
    for (std::size_t setterIndex = 0; setterIndex < setters.size(); ++setterIndex)
    {
        SCOPED_TRACE(setterIndex);
        for (float invalid : InvalidValues())
        {
            SCOPED_TRACE(invalid);
            for (int channel = 0; channel < 3; ++channel)
            {
                SCOPED_TRACE(channel);
                SetBaseline(light);
                // Other channels differ from the baseline to catch partial writes.
                Vector3 color(4.0f, 5.0f, 6.0f);
                if (channel == 0) color.x = invalid;
                if (channel == 1) color.y = invalid;
                if (channel == 2) color.z = invalid;

                EXPECT_NO_THROW((light.*setters[setterIndex])(color));
                ExpectBaseline(light);
            }
        }
    }
}

TEST(SkyLightComponentTest, InvalidIntensityPreservesAllSettings)
{
    SkyLightComponent light;
    for (float invalid : InvalidValues())
    {
        SCOPED_TRACE(invalid);
        SetBaseline(light);

        EXPECT_NO_THROW(light.SetIntensity(invalid));
        ExpectBaseline(light);
    }
}

TEST(SkyLightComponentTest, ValidSettingsCanChangeAfterBeginPlay)
{
    // No Level/Renderer is required for setter tests; this does not test registration.
    Engine::Actor actor;
    auto* light = actor.AddComponent<SkyLightComponent>();
    actor.DispatchInitialize();
    actor.DispatchBeginPlay();

    SetBaseline(*light);
    ExpectBaseline(*light);
    actor.DispatchOnDestroy();
}

TEST(SkyLightComponentTest, SettersRejectChangesDuringRemoval)
{
    Engine::Actor actor;
    auto* light = actor.AddComponent<RemovalProbe>();
    SetBaseline(*light);
    actor.DispatchInitialize();
    actor.DispatchBeginPlay();

    EXPECT_NO_THROW(actor.DispatchOnDestroy());
    EXPECT_TRUE(light->removalObserved);
    // DispatchOnDestroy ends lifecycle but Actor still owns the live object.
    ExpectBaseline(*light);
}

TEST(SkyLightComponentTest, SettersRejectChangesAfterRemoval)
{
    Engine::Actor actor;
    auto* light = actor.AddComponent<SkyLightComponent>();
    SetBaseline(*light);
    actor.DispatchInitialize();
    actor.DispatchBeginPlay();
    actor.DispatchOnDestroy();

    EXPECT_NO_THROW(light->SetSkyColor(Vector3(4.0f, 5.0f, 6.0f)));
    EXPECT_NO_THROW(light->SetGroundColor(Vector3(7.0f, 8.0f, 9.0f)));
    EXPECT_NO_THROW(light->SetIntensity(10.0f));
    ExpectBaseline(*light);
}

TEST(SkyLightComponentTest, InvalidInputLogsApiAndReasonInEveryBuildConfiguration)
{
    SkyLightComponent light;
    ScopedStderrCapture capture;
    light.SetSkyColor(Vector3(-1.0f, 1.0f, 1.0f));
    light.SetGroundColor(Vector3(1.0f, -1.0f, 1.0f));
    light.SetIntensity(-1.0f);
    const std::string output = capture.Finish();

    EXPECT_NE(output.find("[ERROR] SkyLightComponent::SetSkyColor: RGB must be finite and non-negative."), std::string::npos);
    EXPECT_NE(output.find("[ERROR] SkyLightComponent::SetGroundColor: RGB must be finite and non-negative."), std::string::npos);
    EXPECT_NE(output.find("[ERROR] SkyLightComponent::SetIntensity: intensity must be finite and non-negative."), std::string::npos);
}

TEST(SkyLightComponentTest, EndedComponentLogsApiAndReasonInEveryBuildConfiguration)
{
    Engine::Actor actor;
    auto* light = actor.AddComponent<SkyLightComponent>();
    actor.DispatchOnDestroy();

    ScopedStderrCapture capture;
    light->SetSkyColor(Vector3::one);
    light->SetGroundColor(Vector3::one);
    light->SetIntensity(1.0f);
    const std::string output = capture.Finish();

    for (const char* api : {"SetSkyColor", "SetGroundColor", "SetIntensity"})
    {
        SCOPED_TRACE(api);
        const std::string expected = std::string("[ERROR] SkyLightComponent::")
            + api + ": component is ending or ended.";
        EXPECT_NE(output.find(expected), std::string::npos);
    }
}
