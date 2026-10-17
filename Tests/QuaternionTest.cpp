#include <gtest/gtest.h>
#include "Math/Quaternion.h"
#include "Math/Vector3.h"

#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using Engine::Quaternion;
using Engine::Vector3;

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

    void ExpectComponentsNear(const Quaternion& value, const Components& expected)
    {
        const Components actual = ReadComponents(value);
        for (size_t i = 0; i < actual.size(); ++i)
        {
            SCOPED_TRACE(i);
            EXPECT_NEAR(actual[i], expected[i], 1e-6f);
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

TEST(QuaternionTest, HamiltonProductIdentityWorksOnBothSides)
{
    const Quaternion value = MakeQuaternion(kInput);
    ExpectComponents(value * Quaternion::identity, kInput);
    ExpectComponents(Quaternion::identity * value, kInput);
}

TEST(QuaternionTest, HamiltonProductBasisFollowsCyclicOrderAndReverseSigns)
{
    const std::array<Components, 3> basis{{
        {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}
    }};
    for (size_t i = 0; i < basis.size(); ++i)
    {
        SCOPED_TRACE(i);
        const Quaternion first = MakeQuaternion(basis[i]);
        const Quaternion second = MakeQuaternion(basis[(i + 1) % 3]);
        const Components expected = basis[(i + 2) % 3];
        ExpectComponents(first * first, {0, 0, 0, -1});
        ExpectComponents(first * second, expected);
        ExpectComponents(second * first,
            {-expected[0], -expected[1], -expected[2], -expected[3]});
    }
}

TEST(QuaternionTest, HamiltonProductGeneralInputsPreserveOperandsAndMagnitude)
{
    const Quaternion left(1, 2, 3, 4);
    const Quaternion right(5, 6, 7, 8);
    // Hand-expanded products also detect accidental operand reversal or normalization.
    ExpectComponents(left * right, {24, 48, 48, -6});
    ExpectComponents(right * left, {32, 32, 56, -6});
    ExpectComponents(left, {1, 2, 3, 4});
    ExpectComponents(right, {5, 6, 7, 8});
}

TEST(QuaternionTest, HamiltonProductZeroAndScalarInputsRemainAlgebraic)
{
    const Quaternion value = MakeQuaternion(kInput);
    const Quaternion zero(0, 0, 0, 0);
    const Quaternion scalar(0, 0, 0, -2);
    ExpectComponents(value * zero, {0, 0, 0, 0});
    ExpectComponents(zero * value, {0, 0, 0, 0});
    ExpectComponents(value * scalar, {-2, 4, -4, -8});
    ExpectComponents(scalar * value, {-2, 4, -4, -8});
}

TEST(QuaternionTest, HamiltonProductSupportsSelfMultiplication)
{
    Quaternion value(1, 2, 3, 4);
    value = value * value;
    ExpectComponents(value, {8, 16, 24, 2});
}

TEST(QuaternionTest, HamiltonProductWithInverseIsIdentityOnBothSides)
{
    const Quaternion value = MakeQuaternion(kInput);
    Quaternion inverse;
    ASSERT_TRUE(value.TryInvert(inverse));
    for (const Quaternion& result : {value * inverse, inverse * value})
    {
        const Components actual = ReadComponents(result);
        for (size_t i = 0; i < actual.size(); ++i)
        {
            SCOPED_TRACE(i);
            EXPECT_NEAR(actual[i], kIdentity[i], 1e-6f);
        }
    }
}

TEST(QuaternionTest, AxisAngleZeroOnValidAxisReturnsIdentity)
{
    Quaternion result(9, 8, 7, 6);
    ASSERT_TRUE(Quaternion::TryFromAxisAngle(Vector3(2, -3, 6), 0.0f, result));
    ExpectComponents(result, kIdentity);
}

TEST(QuaternionTest, AxisAngleQuarterTurnUsesEachBasisAxisAndRadians)
{
    const std::array<Vector3, 3> axes{{Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)}};
    constexpr float sqrtHalf = 0.7071067811865475f;
    for (size_t i = 0; i < axes.size(); ++i)
    {
        SCOPED_TRACE(i);
        Quaternion result(9, 8, 7, 6);
        ASSERT_TRUE(Quaternion::TryFromAxisAngle(axes[i], PI * 0.5f, result));
        Components expected{0, 0, 0, sqrtHalf};
        expected[i] = sqrtHalf;
        ExpectComponentsNear(result, expected);
    }
}

TEST(QuaternionTest, AxisAngleNormalizesNonunitAxisAndPreservesInput)
{
    const Vector3 axis(2, -3, 6); // Length is exactly 7.
    Quaternion result(9, 8, 7, 6);
    ASSERT_TRUE(Quaternion::TryFromAxisAngle(axis, PI, result));
    ExpectComponentsNear(result, {2.0f / 7.0f, -3.0f / 7.0f, 6.0f / 7.0f, 0});
    EXPECT_FLOAT_EQ(axis.x, 2);
    EXPECT_FLOAT_EQ(axis.y, -3);
    EXPECT_FLOAT_EQ(axis.z, 6);
    Quaternion scaledResult;
    ASSERT_TRUE(Quaternion::TryFromAxisAngle(Vector3(20, -30, 60), PI, scaledResult));
    ExpectComponentsNear(scaledResult, ReadComponents(result));
}

TEST(QuaternionTest, AxisAngleNegativeAngleMatchesNegatedAxis)
{
    Quaternion negativeAngle;
    Quaternion negativeAxis;
    ASSERT_TRUE(Quaternion::TryFromAxisAngle(Vector3(0, 0, 1), -PI * 0.5f, negativeAngle));
    ASSERT_TRUE(Quaternion::TryFromAxisAngle(Vector3(0, 0, -1), PI * 0.5f, negativeAxis));
    ExpectComponentsNear(negativeAngle, {0, 0, -0.7071067811865475f, 0.7071067811865475f});
    ExpectComponentsNear(negativeAxis, ReadComponents(negativeAngle));
}

TEST(QuaternionTest, AxisAngleHalfAndFullTurnsKeepQuaternionSign)
{
    Quaternion halfTurn;
    Quaternion fullTurn;
    ASSERT_TRUE(Quaternion::TryFromAxisAngle(Vector3(0, 1, 0), PI, halfTurn));
    ASSERT_TRUE(Quaternion::TryFromAxisAngle(Vector3(0, 1, 0), 2.0f * PI, fullTurn));
    ExpectComponentsNear(halfTurn, {0, 1, 0, 0});
    // A full turn yields -Identity, which represents the same orientation as Identity.
    ExpectComponentsNear(fullTurn, {0, 0, 0, -1});
    ExpectComponentsNear(halfTurn * halfTurn, ReadComponents(fullTurn));
}

TEST(QuaternionTest, AxisAngleZeroAndTinyAxesFailEvenWithZeroAngle)
{
    for (const Vector3& axis : {Vector3(0, 0, 0), Vector3(0.00001f, 0, 0)})
    {
        for (const float radians : {0.0f, PI * 0.5f})
        {
            Quaternion result(9, 8, 7, 6);
            EXPECT_FALSE(Quaternion::TryFromAxisAngle(axis, radians, result));
            ExpectComponents(result, kIdentity);
        }
    }
}

TEST(QuaternionTest, AxisAngleUsesDefaultAxisLengthThreshold)
{
    const float boundary = std::sqrt(1e-8f);
    const float below = std::nextafter(boundary, 0.0f);
    const float above = std::nextafter(boundary, 1.0f);
    ASSERT_LT(static_cast<double>(below) * below, static_cast<double>(1e-8f));
    ASSERT_GT(static_cast<double>(above) * above, static_cast<double>(1e-8f));
    Quaternion result(9, 8, 7, 6);
    EXPECT_FALSE(Quaternion::TryFromAxisAngle(Vector3(below, 0, 0), PI, result));
    ExpectComponents(result, kIdentity);
    ASSERT_TRUE(Quaternion::TryFromAxisAngle(Vector3(above, 0, 0), PI, result));
    ExpectComponentsNear(result, {1, 0, 0, 0});
}

TEST(QuaternionTest, AxisAngleRejectsNonfiniteAnglesWithIdentity)
{
    const float infinity = std::numeric_limits<float>::infinity();
    for (const float radians : {std::numeric_limits<float>::quiet_NaN(), infinity, -infinity})
    {
        Quaternion result(9, 8, 7, 6);
        EXPECT_FALSE(Quaternion::TryFromAxisAngle(Vector3(0, 0, 1), radians, result));
        ExpectComponents(result, kIdentity);
    }
}

TEST(QuaternionTest, AxisAngleRejectsNonfiniteAxisComponentsEvenWithZeroAngle)
{
    const float infinity = std::numeric_limits<float>::infinity();
    for (size_t i = 0; i < 3; ++i)
    {
        SCOPED_TRACE(i);
        for (const float invalid : {std::numeric_limits<float>::quiet_NaN(), infinity, -infinity})
        {
            std::array<float, 3> values{1, 2, 3};
            values[i] = invalid;
            for (const float radians : {0.0f, PI * 0.5f})
            {
                Quaternion result(9, 8, 7, 6);
                EXPECT_FALSE(Quaternion::TryFromAxisAngle(Vector3(values[0], values[1], values[2]), radians, result));
                ExpectComponents(result, kIdentity);
            }
        }
    }
}

TEST(QuaternionTest, AxisAngleLargeFiniteAxisDoesNotOverflowNormalization)
{
    const float largest = (std::numeric_limits<float>::max)();
    Quaternion result;
    ASSERT_TRUE(Quaternion::TryFromAxisAngle(Vector3(largest, -largest, largest), PI, result));
    constexpr float inverseSqrtThree = 0.5773502691896258f;
    ExpectComponentsNear(result, {inverseSqrtThree, -inverseSqrtThree, inverseSqrtThree, 0});
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
