#include <gtest/gtest.h>

#include <cstddef>
#include <type_traits>

#include "Renderer/GPUData/LightingPacking.h"
#include "Renderer/RenderData/RenderFrameData.h"

namespace
{
    using namespace Engine;

    RenderFrameData MakeFrame(uint32_t count)
    {
        RenderFrameData frame;
        for (uint32_t i = 0; i < count; ++i)
        {
            const float value = static_cast<float>(i + 1);
            DirectionalLightRenderData light;
            light.direction = Vector3(value, -2.0f * value, 3.0f * value);
            light.color = Vector3(0.25f * value, 2.0f * value, 0.5f * value);
            light.intensity = 1.5f * value;
            frame.directionalLights.push_back(light);
        }
        return frame;
    }

    void ExpectVector(const Vector4& actual, float x, float y, float z, float w)
    {
        EXPECT_FLOAT_EQ(actual.x, x);
        EXPECT_FLOAT_EQ(actual.y, y);
        EXPECT_FLOAT_EQ(actual.z, z);
        EXPECT_FLOAT_EQ(actual.w, w);
    }

    void ExpectLight(const DirectionalLightGPUData& actual,
        const DirectionalLightRenderData& source)
    {
        ExpectVector(actual.direction, source.direction.x, source.direction.y, source.direction.z, 0.0f);
        ExpectVector(actual.colorAndIntensity, source.color.x, source.color.y, source.color.z, source.intensity);
    }

    void ExpectUnusedSlotsZero(const LightingConstantBuffer& buffer, uint32_t firstUnused)
    {
        for (uint32_t i = firstUnused; i < MaxDirectionalLights; ++i)
        {
            SCOPED_TRACE(i);
            ExpectVector(buffer.directionalLights[i].direction, 0, 0, 0, 0);
            ExpectVector(buffer.directionalLights[i].colorAndIntensity, 0, 0, 0, 0);
        }
        for (float value : buffer.padding)
        {
            EXPECT_FLOAT_EQ(value, 0.0f);
        }
    }
}

TEST(LightingPackingTest, EmptyFrameHasZeroCountAndZeroUnusedSlots)
{
    const auto result = BuildLightingConstantBuffer(RenderFrameData{});
    EXPECT_EQ(result.directionalLightCount, 0u);
    ExpectUnusedSlotsZero(result, 0);
    ExpectVector(result.skyContribution, 0, 0, 0, 0);
    ExpectVector(result.groundContribution, 0, 0, 0, 0);
}

TEST(LightingPackingTest, OneLightPreservesDirectionRgbAndSeparateIntensity)
{
    const auto frame = MakeFrame(1);
    const auto result = BuildLightingConstantBuffer(frame);
    ASSERT_EQ(result.directionalLightCount, 1u);
    // Non-unit direction and RGB > 1 must be copied, not normalized or clamped.
    ExpectVector(result.directionalLights[0].direction, 1.0f, -2.0f, 3.0f, 0.0f);
    ExpectVector(result.directionalLights[0].colorAndIntensity, 0.25f, 2.0f, 0.5f, 1.5f);
    ExpectUnusedSlotsZero(result, 1);
}

TEST(LightingPackingTest, CapacityLightsAreAllPackedInCollectionOrder)
{
    const auto frame = MakeFrame(MaxDirectionalLights);
    const auto result = BuildLightingConstantBuffer(frame);
    ASSERT_EQ(result.directionalLightCount, MaxDirectionalLights);
    for (uint32_t i = 0; i < MaxDirectionalLights; ++i)
    {
        SCOPED_TRACE(i);
        ExpectLight(result.directionalLights[i], frame.directionalLights[i]);
    }
    ExpectUnusedSlotsZero(result, MaxDirectionalLights);
}

TEST(LightingPackingTest, OverCapacityKeepsOnlyFirstCapacityLights)
{
    const auto frame = MakeFrame(MaxDirectionalLights + 1);
    const auto result = BuildLightingConstantBuffer(frame);
    ASSERT_EQ(result.directionalLightCount, MaxDirectionalLights);
    for (uint32_t i = 0; i < MaxDirectionalLights; ++i)
    {
        SCOPED_TRACE(i);
        ExpectLight(result.directionalLights[i], frame.directionalLights[i]);
    }
    ExpectUnusedSlotsZero(result, MaxDirectionalLights);
}

TEST(LightingPackingTest, SmallerAndEmptyFramesDoNotRetainPreviousLights)
{
    auto frame = MakeFrame(MaxDirectionalLights);
    const auto full = BuildLightingConstantBuffer(frame);
    ASSERT_EQ(full.directionalLightCount, MaxDirectionalLights);

    frame.directionalLights.resize(1);
    frame.directionalLights[0].intensity = 0.0f;
    const auto smaller = BuildLightingConstantBuffer(frame);
    ASSERT_EQ(smaller.directionalLightCount, 1u);
    ExpectLight(smaller.directionalLights[0], frame.directionalLights[0]);
    ExpectUnusedSlotsZero(smaller, 1);

    frame.directionalLights.clear();
    const auto empty = BuildLightingConstantBuffer(frame);
    EXPECT_EQ(empty.directionalLightCount, 0u);
    ExpectUnusedSlotsZero(empty, 0);
    // Previously returned values remain independent of later input and results.
    EXPECT_FLOAT_EQ(full.directionalLights[0].colorAndIntensity.w, 1.5f);
}

TEST(LightingPackingTest, PackingDoesNotMutateInputOrRetainItsReferences)
{
    auto frame = MakeFrame(MaxDirectionalLights + 1);
    const auto before = frame;
    const auto packed = BuildLightingConstantBuffer(frame);
    ASSERT_EQ(frame.directionalLights.size(), before.directionalLights.size());
    for (size_t i = 0; i < frame.directionalLights.size(); ++i)
    {
        SCOPED_TRACE(i);
        const auto& actual = frame.directionalLights[i];
        const auto& expected = before.directionalLights[i];
        EXPECT_EQ(actual.direction, expected.direction);
        EXPECT_EQ(actual.color, expected.color);
        EXPECT_FLOAT_EQ(actual.intensity, expected.intensity);
    }
    frame.directionalLights[0].color = Vector3::zero;
    frame.directionalLights[0].intensity = 0.0f;
    ExpectLight(packed.directionalLights[0], before.directionalLights[0]);
}

TEST(LightingPackingTest, CpuLayoutMatchesCurrentLightingBufferSlots)
{
    static_assert(std::is_standard_layout_v<LightingConstantBuffer>);
    static_assert(std::is_trivially_copyable_v<LightingConstantBuffer>);
    EXPECT_EQ(sizeof(Vector4), 16u);
    EXPECT_EQ(sizeof(DirectionalLightGPUData), 32u);
    EXPECT_EQ(offsetof(DirectionalLightGPUData, colorAndIntensity), 16u);
    EXPECT_EQ(offsetof(LightingConstantBuffer, directionalLights), 0u);
    EXPECT_EQ(offsetof(LightingConstantBuffer, skyContribution), 32u * MaxDirectionalLights);
    EXPECT_EQ(offsetof(LightingConstantBuffer, groundContribution), 32u * MaxDirectionalLights + 16u);
    EXPECT_EQ(offsetof(LightingConstantBuffer, directionalLightCount), 32u * MaxDirectionalLights + 32u);
    EXPECT_EQ(offsetof(LightingConstantBuffer, padding), 32u * MaxDirectionalLights + 36u);
    EXPECT_EQ(sizeof(LightingConstantBuffer), 32u * MaxDirectionalLights + 48u);
    EXPECT_EQ(sizeof(LightingConstantBuffer) % 16, 0u);
}

TEST(LightingPackingTest, EnvironmentPreservesIndependentRgbAndSetsWToZero)
{
    RenderFrameData frame;
    frame.environmentLighting.skyContribution = Vector3(0.25f, 2.0f, 8.0f);
    frame.environmentLighting.groundContribution = Vector3(4.0f, 0.0f, 0.125f);

    const auto packed = BuildLightingConstantBuffer(frame);

    ExpectVector(packed.skyContribution, 0.25f, 2.0f, 8.0f, 0.0f);
    ExpectVector(packed.groundContribution, 4.0f, 0.0f, 0.125f, 0.0f);
    EXPECT_EQ(packed.directionalLightCount, 0u);
    ExpectUnusedSlotsZero(packed, 0);
    EXPECT_EQ(frame.environmentLighting.skyContribution, Vector3(0.25f, 2.0f, 8.0f));
    EXPECT_EQ(frame.environmentLighting.groundContribution, Vector3(4.0f, 0.0f, 0.125f));
}

TEST(LightingPackingTest, EnvironmentUpdatesAndClearsWithoutChangingEarlierResult)
{
    RenderFrameData frame;
    frame.environmentLighting.skyContribution = Vector3(1.0f, 2.0f, 3.0f);
    frame.environmentLighting.groundContribution = Vector3(4.0f, 5.0f, 6.0f);
    const auto first = BuildLightingConstantBuffer(frame);

    frame.environmentLighting.skyContribution = Vector3(7.0f, 8.0f, 9.0f);
    frame.environmentLighting.groundContribution = Vector3(0.5f, 0.25f, 0.125f);
    for (int repeat = 0; repeat < 2; ++repeat)
    {
        const auto updated = BuildLightingConstantBuffer(frame);
        ExpectVector(updated.skyContribution, 7.0f, 8.0f, 9.0f, 0.0f);
        ExpectVector(updated.groundContribution, 0.5f, 0.25f, 0.125f, 0.0f);
    }

    frame.environmentLighting = {};
    const auto cleared = BuildLightingConstantBuffer(frame);
    ExpectVector(cleared.skyContribution, 0, 0, 0, 0);
    ExpectVector(cleared.groundContribution, 0, 0, 0, 0);
    ExpectVector(first.skyContribution, 1.0f, 2.0f, 3.0f, 0.0f);
    ExpectVector(first.groundContribution, 4.0f, 5.0f, 6.0f, 0.0f);
}

TEST(LightingPackingTest, EnvironmentCoexistsWithCappedDirectionalLights)
{
    auto frame = MakeFrame(MaxDirectionalLights + 1);
    frame.environmentLighting.skyContribution = Vector3(3.0f, 0.5f, 1.0f);
    frame.environmentLighting.groundContribution = Vector3(0.0f, 2.0f, 0.25f);
    const auto packed = BuildLightingConstantBuffer(frame);

    ASSERT_EQ(packed.directionalLightCount, MaxDirectionalLights);
    for (uint32_t i = 0; i < MaxDirectionalLights; ++i)
    {
        SCOPED_TRACE(i);
        ExpectLight(packed.directionalLights[i], frame.directionalLights[i]);
    }
    ExpectVector(packed.skyContribution, 3.0f, 0.5f, 1.0f, 0.0f);
    ExpectVector(packed.groundContribution, 0.0f, 2.0f, 0.25f, 0.0f);
    ExpectUnusedSlotsZero(packed, MaxDirectionalLights);
}
