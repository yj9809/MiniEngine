#include <gtest/gtest.h>
#include "Math/Quaternion.h"

#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using Engine::Quaternion;

namespace
{
    using Components = std::array<float, 4>;
    constexpr Components kIdentity{0.0f, 0.0f, 0.0f, 1.0f};
    constexpr Components kInput{1.0f, -2.0f, 2.0f, 4.0f};
    constexpr Components kNormalized{0.2f, -0.4f, 0.4f, 0.8f};
    constexpr Components kInverse{-0.04f, 0.08f, -0.08f, 0.16f};

    Components ReadComponents(const Quaternion& value)
    {
        return {value.x, value.y, value.z, value.w};
    }

    Quaternion MakeQuaternion(const Components& values)
    {
        return Quaternion(values[0], values[1], values[2], values[3]);
    }

    void ExpectComponents(const Quaternion& value, const Components& expected)
    {
        const Components actual = ReadComponents(value);
        for (size_t i = 0; i < actual.size(); ++i)
        {
            SCOPED_TRACE(i);
            if (std::isnan(expected[i]))
                EXPECT_TRUE(std::isnan(actual[i]));
            else
                EXPECT_FLOAT_EQ(actual[i], expected[i]);
        }
    }

    struct InvalidInput
    {
        std::string name;
        Components values;
        float threshold = 1e-8f;
    };

    std::vector<InvalidInput> InvalidInputs()
    {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float infinity = std::numeric_limits<float>::infinity();
        std::vector<InvalidInput> cases{
            {"Zero", {0, 0, 0, 0}},
            {"BelowDefaultThreshold", {0.00001f, 0, 0, 0}},
            {"AtThreshold", {0.5f, 0, 0, 0}, 0.25f},
            {"BelowThreshold", {std::nextafter(0.5f, 0.0f), 0, 0, 0}, 0.25f},
            {"ZeroWithZeroThreshold", {0, 0, 0, 0}, 0.0f},
            {"NegativeThreshold", kInput, -1.0f},
            {"NaNThreshold", kInput, nan},
            {"PositiveInfinityThreshold", kInput, infinity},
            {"NegativeInfinityThreshold", kInput, -infinity}
        };
        for (size_t i = 0; i < 4; ++i)
        {
            Components values = kInput;
            values[i] = nan;
            cases.push_back({"NaNComponent" + std::to_string(i), values});
            values[i] = infinity;
            cases.push_back({"PositiveInfinityComponent" + std::to_string(i), values});
            values[i] = -infinity;
            cases.push_back({"NegativeInfinityComponent" + std::to_string(i), values});
        }
        return cases;
    }

    class QuaternionFailureTest : public ::testing::TestWithParam<InvalidInput> {};
}

TEST(QuaternionTest, DefaultAndIdentityHaveNoRotationComponents)
{
    ExpectComponents(Quaternion(), kIdentity);
    ExpectComponents(Quaternion::identity, kIdentity);
}

TEST(QuaternionTest, ComponentConstructorPreservesValuesWithoutNormalizing)
{
    ExpectComponents(MakeQuaternion(kInput), kInput);
}

TEST(QuaternionTest, NormalizeUsesAllFourComponents)
{
    Quaternion value = MakeQuaternion(kInput);
    ASSERT_TRUE(value.Normalize());
    ExpectComponents(value, kNormalized);
}

TEST(QuaternionTest, TryNormalizePreservesSourceAndOverwritesOutput)
{
    const Quaternion source = MakeQuaternion(kInput);
    Quaternion result(9, 8, 7, 6);
    ASSERT_TRUE(source.TryNormalize(result));
    ExpectComponents(result, kNormalized);
    ExpectComponents(source, kInput);
}

TEST(QuaternionTest, NormalizingIdentityIsStable)
{
    Quaternion value = Quaternion::identity;
    ASSERT_TRUE(value.Normalize());
    ASSERT_TRUE(value.Normalize());
    ExpectComponents(value, kIdentity);
}

TEST(QuaternionTest, NormalizeLargeFiniteInputDoesNotOverflowLengthSquared)
{
    const float largest = (std::numeric_limits<float>::max)();
    Quaternion value(largest, -largest, largest, -largest);
    ASSERT_TRUE(value.Normalize());
    ExpectComponents(value, {0.5f, -0.5f, 0.5f, -0.5f});
}

TEST(QuaternionTest, ZeroThresholdAllowsTinyNonzeroNormalization)
{
    const float smallestNormal = (std::numeric_limits<float>::min)();
    Quaternion value(smallestNormal, 0, 0, 0);
    ASSERT_TRUE(value.Normalize(0.0f));
    ExpectComponents(value, {1, 0, 0, 0});
}

TEST(QuaternionTest, ImmediatelyAboveThresholdSucceeds)
{
    const float x = std::nextafter(0.5f, 1.0f);
    const Quaternion source(x, 0, 0, 0);
    Quaternion result;
    ASSERT_TRUE(source.TryNormalize(result, 0.25f));
    ExpectComponents(result, {1, 0, 0, 0});
    ASSERT_TRUE(source.TryInvert(result, 0.25f));
    ExpectComponents(result, {-1.0f / x, 0, 0, 0});
}

TEST(QuaternionTest, ConjugateFlipsOnlyVectorPartAndPreservesSource)
{
    const Quaternion source = MakeQuaternion(kInput);
    ExpectComponents(source.Conjugate(), {-1, 2, -2, 4});
    ExpectComponents(source, kInput);
}

TEST(QuaternionTest, ConjugateInPlaceChangesSelfAndTwiceRestoresOriginal)
{
    Quaternion value = MakeQuaternion(kInput);
    value.ConjugateInPlace();
    ExpectComponents(value, {-1, 2, -2, 4});
    value.ConjugateInPlace();
    ExpectComponents(value, kInput);
}

TEST(QuaternionTest, ConjugatePreservesIdentity)
{
    ExpectComponents(Quaternion::identity.Conjugate(), kIdentity);
}

TEST(QuaternionTest, InvertNonunitQuaternionDividesBySquaredLength)
{
    Quaternion value = MakeQuaternion(kInput);
    ASSERT_TRUE(value.Invert());
    ExpectComponents(value, kInverse);
}

TEST(QuaternionTest, TryInvertPreservesSourceAndOverwritesOutput)
{
    const Quaternion source = MakeQuaternion(kInput);
    Quaternion result(9, 8, 7, 6);
    ASSERT_TRUE(source.TryInvert(result));
    ExpectComponents(result, kInverse);
    ExpectComponents(source, kInput);
}

TEST(QuaternionTest, ScalarOnlyInverseIsReciprocal)
{
    Quaternion value(0, 0, 0, 2);
    ASSERT_TRUE(value.Invert());
    ExpectComponents(value, {0, 0, 0, 0.5f});
}

TEST(QuaternionTest, UnitQuaternionInverseEqualsConjugate)
{
    const Quaternion source(0.5f, -0.5f, 0.5f, -0.5f);
    Quaternion result;
    ASSERT_TRUE(source.TryInvert(result));
    ExpectComponents(result, {-0.5f, 0.5f, -0.5f, -0.5f});
    ExpectComponents(result, ReadComponents(source.Conjugate()));
}

TEST(QuaternionTest, InvertingIdentityIsStable)
{
    Quaternion value = Quaternion::identity;
    ASSERT_TRUE(value.Invert());
    ExpectComponents(value, kIdentity);
}

TEST(QuaternionTest, InvertingTwiceRestoresNonunitInput)
{
    Quaternion value = MakeQuaternion(kInput);
    ASSERT_TRUE(value.Invert());
    ASSERT_TRUE(value.Invert());
    ExpectComponents(value, kInput);
}

TEST(QuaternionTest, InverseLargeFiniteInputRetainsSmallNonzeroResult)
{
    const float largest = (std::numeric_limits<float>::max)();
    Quaternion value(largest, 0, 0, 0);
    ASSERT_TRUE(value.Invert());
    const Components actual = ReadComponents(value);
    EXPECT_LT(actual[0], 0.0f);
    EXPECT_NEAR(static_cast<double>(actual[0]), -1.0 / largest, 1e-44);
    EXPECT_FLOAT_EQ(actual[1], 0);
    EXPECT_FLOAT_EQ(actual[2], 0);
    EXPECT_FLOAT_EQ(actual[3], 0);
}

TEST(QuaternionTest, ZeroThresholdAllowsRepresentableLargeInverse)
{
    const float smallestNormal = (std::numeric_limits<float>::min)();
    Quaternion value(smallestNormal, 0, 0, 0);
    ASSERT_TRUE(value.Invert(0.0f));
    ExpectComponents(value, {-1.0f / smallestNormal, 0, 0, 0});
}

TEST(QuaternionTest, InverseOutsideFloatRangeFailsWithIdentity)
{
    // This input has a finite double inverse but cannot store it in float.
    const float tiny = std::numeric_limits<float>::denorm_min();
    const Quaternion source(0, 0, 0, tiny);
    Quaternion result(9, 8, 7, 6);
    EXPECT_FALSE(source.TryInvert(result, 0.0f));
    ExpectComponents(result, kIdentity);
    ExpectComponents(source, {0, 0, 0, tiny});
    Quaternion value = source;
    EXPECT_FALSE(value.Invert(0.0f));
    ExpectComponents(value, kIdentity);
}

TEST(QuaternionTest, TryOperationsSupportUsingSelfAsOutput)
{
    Quaternion normalized = MakeQuaternion(kInput);
    ASSERT_TRUE(normalized.TryNormalize(normalized));
    ExpectComponents(normalized, kNormalized);
    Quaternion inverted = MakeQuaternion(kInput);
    ASSERT_TRUE(inverted.TryInvert(inverted));
    ExpectComponents(inverted, kInverse);
    Quaternion invalid(0, 0, 0, 0);
    EXPECT_FALSE(invalid.TryNormalize(invalid));
    ExpectComponents(invalid, kIdentity);
    invalid = Quaternion(0, 0, 0, 0);
    EXPECT_FALSE(invalid.TryInvert(invalid));
    ExpectComponents(invalid, kIdentity);
}

TEST_P(QuaternionFailureTest, NormalizeFormsReturnFalseAndIdentity)
{
    const InvalidInput& input = GetParam();
    const Quaternion source = MakeQuaternion(input.values);
    Quaternion result(9, 8, 7, 6);
    EXPECT_FALSE(source.TryNormalize(result, input.threshold));
    ExpectComponents(result, kIdentity);
    ExpectComponents(source, input.values);
    Quaternion value = source;
    EXPECT_FALSE(value.Normalize(input.threshold));
    ExpectComponents(value, kIdentity);
}

TEST_P(QuaternionFailureTest, InverseFormsReturnFalseAndIdentity)
{
    const InvalidInput& input = GetParam();
    const Quaternion source = MakeQuaternion(input.values);
    Quaternion result(9, 8, 7, 6);
    EXPECT_FALSE(source.TryInvert(result, input.threshold));
    ExpectComponents(result, kIdentity);
    ExpectComponents(source, input.values);
    Quaternion value = source;
    EXPECT_FALSE(value.Invert(input.threshold));
    ExpectComponents(value, kIdentity);
}

INSTANTIATE_TEST_SUITE_P(
    InvalidInputs, QuaternionFailureTest, ::testing::ValuesIn(InvalidInputs()),
    [](const ::testing::TestParamInfo<InvalidInput>& info) { return info.param.name; });
