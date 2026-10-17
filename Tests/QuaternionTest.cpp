#include <gtest/gtest.h>
#include "Math/Quaternion.h"
#include "Math/Matrix4.h"
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

    void ExpectUnitQuaternion(const Quaternion& value)
    {
        double lengthSquared = 0.0;
        for (float component : ReadComponents(value))
        {
            EXPECT_TRUE(std::isfinite(component));
            lengthSquared += static_cast<double>(component) * component;
        }
        EXPECT_NEAR(lengthSquared, 1.0, 1e-6);
    }

    // Algebraic reference for Euler conversion, independent of TryRotateVector.
    void ExpectRotatedVector(const Quaternion& rotation, const Vector3& input,
        const Vector3& expected)
    {
        const Quaternion vector(input.x, input.y, input.z, 0.0f);
        const Quaternion result = rotation * vector * rotation.Conjugate();
        EXPECT_NEAR(result.x, expected.x, 3e-6f);
        EXPECT_NEAR(result.y, expected.y, 3e-6f);
        EXPECT_NEAR(result.z, expected.z, 3e-6f);
        EXPECT_NEAR(result.w, 0.0f, 3e-6f);
    }

    void ExpectVectorNear(const Vector3& actual, const Vector3& expected)
    {
        EXPECT_NEAR(actual.x, expected.x, 1e-5f);
        EXPECT_NEAR(actual.y, expected.y, 1e-5f);
        EXPECT_NEAR(actual.z, expected.z, 1e-5f);
    }

    void ExpectSameUnitRotation(const Quaternion& actual, const Quaternion& expected)
    {
        ExpectUnitQuaternion(actual);
        const float dot = actual.x * expected.x + actual.y * expected.y +
            actual.z * expected.z + actual.w * expected.w;
        const float sign = dot < 0.0f ? -1.0f : 1.0f;
        ExpectComponentsNear(actual, {sign * expected.x, sign * expected.y,
            sign * expected.z, sign * expected.w});
    }

    using InterpolationFunction = bool (*)(const Quaternion&, const Quaternion&, float, Quaternion&);
    struct InterpolationCase
    {
        const char* name;
        InterpolationFunction interpolate;
    };
    class QuaternionInterpolationTest : public ::testing::TestWithParam<InterpolationCase> {};

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

TEST(QuaternionTest, EulerZeroReturnsIdentityForBothOverloads)
{
    Quaternion result(9, 8, 7, 6);
    ASSERT_TRUE(Quaternion::TryFromEuler(0.0f, 0.0f, 0.0f, result));
    ExpectComponents(result, kIdentity);
    result = Quaternion(9, 8, 7, 6);
    ASSERT_TRUE(Quaternion::TryFromEuler(Vector3::zero, result));
    ExpectComponents(result, kIdentity);
}

TEST(QuaternionTest, EulerSignedQuarterTurnsUseUnrealSignsAndRadians)
{
    constexpr float halfPi = 1.5707963267948966f;
    constexpr float sqrtHalf = 0.7071067811865475f;
    for (size_t axis = 0; axis < 3; ++axis)
    {
        SCOPED_TRACE(axis);
        for (float sign : {1.0f, -1.0f})
        {
            SCOPED_TRACE(sign);
            std::array<float, 3> angles{0, 0, 0};
            angles[axis] = sign * halfPi;
            Quaternion result(9, 8, 7, 6);
            ASSERT_TRUE(Quaternion::TryFromEuler(angles[0], angles[1], angles[2], result));
            Components expected{0, 0, 0, sqrtHalf};
            expected[axis] = (axis == 2 ? sign : -sign) * sqrtHalf;
            ExpectComponentsNear(result, expected);
            ExpectUnitQuaternion(result);
        }
    }
}

TEST(QuaternionTest, EulerMixedAnglesMatchIndependentBasis)
{
    constexpr float toRadians = 0.017453292519943295f;
    Quaternion result;
    ASSERT_TRUE(Quaternion::TryFromEuler(
        30.0f * toRadians, 20.0f * toRadians, 40.0f * toRadians, result));
    // Independent intrinsic yaw/pitch/roll basis for (Roll,Pitch,Yaw)=(30,20,40).
    // F=(cp*cy,cp*sy,sp), R=(sr*sp*cy-cr*sy,sr*sp*sy+cr*cy,-sr*cp),
    // U=(-cr*sp*cy-sr*sy,-cr*sp*sy+sr*cy,cr*cp).
    ExpectRotatedVector(result, Vector3::unitX, Vector3(0.71984631f, 0.60402277f, 0.34202014f));
    ExpectRotatedVector(result, Vector3::unitY, Vector3(-0.42566908f, 0.77333710f, -0.46984631f));
    ExpectRotatedVector(result, Vector3::unitZ, Vector3(-0.54829474f, 0.19262973f, 0.81379768f));
    ExpectUnitQuaternion(result);
}

TEST(QuaternionTest, EulerMatchesMatrixAcrossMixedAnglesAndPitchSingularities)
{
    constexpr float halfPi = 1.5707963267948966f;
    const std::array<Vector3, 4> vectors{
        Vector3::unitX, Vector3::unitY, Vector3::unitZ, Vector3(0.3f, -0.7f, 1.2f)};
    // 112 angle triples, including both signs, multiple turns, and Pitch +/-90 degrees.
    for (float roll : {0.0f, 0.37f, -1.2f, 6.0f * halfPi})
    {
        SCOPED_TRACE(roll);
        for (float pitch : {0.0f, 0.2f, -halfPi, halfPi, halfPi - 1e-4f, halfPi + 1e-4f, -2.3f})
        {
            SCOPED_TRACE(pitch);
            for (float yaw : {0.0f, -0.7f, 2.2f, 4.0f * halfPi})
            {
                SCOPED_TRACE(yaw);
                Quaternion result;
                ASSERT_TRUE(Quaternion::TryFromEuler(roll, pitch, yaw, result));
                ExpectUnitQuaternion(result);
                const Engine::Matrix4 matrix = Engine::Matrix4::Rotation(roll, pitch, yaw);
                for (size_t i = 0; i < vectors.size(); ++i)
                {
                    SCOPED_TRACE(i);
                    ExpectRotatedVector(result, vectors[i], matrix.TransformVector(vectors[i]));
                }
            }
        }
    }
}

TEST(QuaternionTest, EulerVectorPreservesInputAndUsesRollPitchYawOrder)
{
    const Vector3 input(0.31f, -0.47f, 0.83f);
    Quaternion result(9, 8, 7, 6);
    Quaternion scalarResult;
    ASSERT_TRUE(Quaternion::TryFromEuler(input, result));
    ASSERT_TRUE(Quaternion::TryFromEuler(input.x, input.y, input.z, scalarResult));
    ExpectComponentsNear(result, ReadComponents(scalarResult));
    ExpectUnitQuaternion(result);
    EXPECT_FLOAT_EQ(input.x, 0.31f);
    EXPECT_FLOAT_EQ(input.y, -0.47f);
    EXPECT_FLOAT_EQ(input.z, 0.83f);
}

TEST(QuaternionTest, EulerRejectsEveryNonfiniteAngleForBothOverloads)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    for (size_t axis = 0; axis < 3; ++axis)
    {
        SCOPED_TRACE(axis);
        for (float invalid : {nan, infinity, -infinity})
        {
            SCOPED_TRACE(invalid);
            std::array<float, 3> angles{0, 0, 0};
            angles[axis] = invalid;
            Quaternion result(9, 8, 7, 6);
            EXPECT_FALSE(Quaternion::TryFromEuler(angles[0], angles[1], angles[2], result));
            ExpectComponents(result, kIdentity);
            result = Quaternion(9, 8, 7, 6);
            EXPECT_FALSE(Quaternion::TryFromEuler(Vector3(angles[0], angles[1], angles[2]), result));
            ExpectComponents(result, kIdentity);
        }
    }
}

TEST(QuaternionTest, EulerFiniteExtremesStillProduceFiniteUnitResults)
{
    const float largest = (std::numeric_limits<float>::max)();
    const float smallest = (std::numeric_limits<float>::min)();
    const float denormal = std::numeric_limits<float>::denorm_min();
    const std::array<Vector3, 4> inputs{
        Vector3(largest, -largest, largest), Vector3(-largest, largest, -largest),
        Vector3(smallest, -smallest, smallest), Vector3(denormal, -denormal, denormal)};
    for (size_t i = 0; i < inputs.size(); ++i)
    {
        SCOPED_TRACE(i);
        Quaternion result;
        ASSERT_TRUE(Quaternion::TryFromEuler(inputs[i], result));
        ExpectUnitQuaternion(result);
    }
}

TEST(QuaternionTest, EulerFullTurnsPreserveOrientationWithoutRequiringSameQuaternionSign)
{
    constexpr float twoPi = 6.2831853071795865f;
    const Vector3 input(0.31f, -0.47f, 0.83f);
    const Engine::Matrix4 reference = Engine::Matrix4::Rotation(input);
    const std::array<Vector3, 3> basis{Vector3::unitX, Vector3::unitY, Vector3::unitZ};
    for (size_t axis = 0; axis < 3; ++axis)
    {
        SCOPED_TRACE(axis);
        for (float turn : {twoPi, -twoPi})
        {
            SCOPED_TRACE(turn);
            std::array<float, 3> angles{input.x, input.y, input.z};
            angles[axis] += turn;
            Quaternion result;
            ASSERT_TRUE(Quaternion::TryFromEuler(angles[0], angles[1], angles[2], result));
            for (const Vector3& v : basis)
                ExpectRotatedVector(result, v, reference.TransformVector(v));
        }
    }
}

TEST(QuaternionTest, RotateVectorIdentityAndZeroInputSucceed)
{
    const Vector3 input(2.0f, -3.0f, 4.0f);
    Vector3 result(9, 8, 7);
    ASSERT_TRUE(Quaternion::identity.TryRotateVector(input, result));
    ExpectVectorNear(result, input);
    const Quaternion rotation(1, -2, 3, 4);
    result = Vector3(9, 8, 7);
    ASSERT_TRUE(rotation.TryRotateVector(Vector3::zero, result));
    EXPECT_EQ(result, Vector3::zero);
}

TEST(QuaternionTest, RotateVectorSignedAxisQuarterTurnsPreserveLength)
{
    constexpr float sqrtHalf = 0.7071067811865475f;
    const std::array<Vector3, 3> inputs{Vector3(0, 2, 0), Vector3(0, 0, 2), Vector3(2, 0, 0)};
    const std::array<Vector3, 3> positiveResults{Vector3(0, 0, 2), Vector3(2, 0, 0), Vector3(0, 2, 0)};
    for (size_t axis = 0; axis < 3; ++axis)
    {
        SCOPED_TRACE(axis);
        for (float sign : {1.0f, -1.0f})
        {
            SCOPED_TRACE(sign);
            Components components{0, 0, 0, sqrtHalf};
            components[axis] = sign * sqrtHalf;
            Vector3 result;
            ASSERT_TRUE(MakeQuaternion(components).TryRotateVector(inputs[axis], result));
            ExpectVectorNear(result, positiveResults[axis] * sign);
            EXPECT_NEAR(result.LengthSquared(), 4.0f, 1e-5f);
        }
    }
}

TEST(QuaternionTest, RotateVectorNormalizesQuaternionAndPreservesBothInputs)
{
    const Vector3 input(2.0f, -3.0f, 4.0f);
    // Both signs and different magnitudes represent the same +90-degree Z rotation.
    for (float magnitude : {0.25f, 2.0f, -2.0f})
    {
        SCOPED_TRACE(magnitude);
        const Quaternion rotation(0, 0, magnitude, magnitude);
        const Components original = ReadComponents(rotation);
        Vector3 result(9, 8, 7);
        ASSERT_TRUE(rotation.TryRotateVector(input, result));
        ExpectVectorNear(result, Vector3(3, 2, 4));
        ExpectComponents(rotation, original);
        EXPECT_EQ(input, Vector3(2, -3, 4));
    }
}

TEST(QuaternionTest, RotateVectorMatchesMatricesAndInverseRoundTrips)
{
    const Vector3 input(2.0f, -3.0f, 4.0f);
    constexpr float halfPi = 1.5707963267948966f;
    for (float roll : {0.0f, 0.37f, -1.2f})
    {
        SCOPED_TRACE(roll);
        for (float pitch : {-halfPi, 0.23f, halfPi})
        {
            SCOPED_TRACE(pitch);
            for (float yaw : {0.0f, -0.7f, 2.2f})
            {
                SCOPED_TRACE(yaw);
                Quaternion rotation;
                ASSERT_TRUE(Quaternion::TryFromEuler(roll, pitch, yaw, rotation));
                Vector3 result;
                ASSERT_TRUE(rotation.TryRotateVector(input, result));
                ExpectVectorNear(result, Engine::Matrix4::Rotation(roll, pitch, yaw).TransformVector(input));
                EXPECT_NEAR(result.LengthSquared(), input.LengthSquared(), 5e-5f);
                Vector3 restored;
                ASSERT_TRUE(rotation.Conjugate().TryRotateVector(result, restored));
                ExpectVectorNear(restored, input);
            }
        }
    }
}

TEST(QuaternionTest, RotateVectorRejectsInvalidQuaternionsEvenForZeroVector)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    std::vector<Components> invalid{{0, 0, 0, 0}};
    for (size_t i = 0; i < 4; ++i)
    {
        for (float value : {nan, infinity, -infinity})
        {
            Components components = kIdentity;
            components[i] = value;
            invalid.push_back(components);
        }
    }
    for (size_t i = 0; i < invalid.size(); ++i)
    {
        SCOPED_TRACE(i);
        const Quaternion rotation = MakeQuaternion(invalid[i]);
        for (const Vector3& input : {Vector3::zero, Vector3(2, -3, 4)})
        {
            Vector3 result(9, 8, 7);
            EXPECT_FALSE(rotation.TryRotateVector(input, result));
            EXPECT_EQ(result, Vector3::zero);
            ExpectComponents(rotation, invalid[i]);
        }
    }
}

TEST(QuaternionTest, RotateVectorUsesDefaultQuaternionNormalizationThreshold)
{
    const float boundary = 1e-4f;
    const float below = std::nextafter(boundary, 0.0f);
    const float above = std::nextafter(boundary, 1.0f);
    ASSERT_LT(static_cast<double>(below) * below, static_cast<double>(1e-8f));
    ASSERT_GT(static_cast<double>(above) * above, static_cast<double>(1e-8f));
    const Vector3 input(2, -3, 4);
    Vector3 result(9, 8, 7);
    EXPECT_FALSE(Quaternion(0, 0, 0, below).TryRotateVector(input, result));
    EXPECT_EQ(result, Vector3::zero);
    ASSERT_TRUE(Quaternion(0, 0, 0, above).TryRotateVector(input, result));
    ExpectVectorNear(result, input);
}

TEST(QuaternionTest, RotateVectorRejectsEveryNonfiniteVectorComponent)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    const Quaternion rotation(0, 0, 2, 2);
    for (size_t i = 0; i < 3; ++i)
    {
        SCOPED_TRACE(i);
        for (float invalid : {nan, infinity, -infinity})
        {
            SCOPED_TRACE(invalid);
            std::array<float, 3> values{2, -3, 4};
            values[i] = invalid;
            const Vector3 input(values[0], values[1], values[2]);
            Vector3 result(9, 8, 7);
            EXPECT_FALSE(rotation.TryRotateVector(input, result));
            EXPECT_EQ(result, Vector3::zero);
            const std::array<float, 3> after{input.x, input.y, input.z};
            for (size_t component = 0; component < 3; ++component)
            {
                if (std::isnan(values[component])) EXPECT_TRUE(std::isnan(after[component]));
                else EXPECT_FLOAT_EQ(after[component], values[component]);
            }
            ExpectComponents(rotation, {0, 0, 2, 2});
        }
    }
}

TEST(QuaternionTest, RotateVectorSupportsInputOutputAliasingOnSuccessAndFailure)
{
    const Quaternion rotation(0, 0, 2, 2);
    Vector3 vector(2, 0, 0);
    ASSERT_TRUE(rotation.TryRotateVector(vector, vector));
    ExpectVectorNear(vector, Vector3(0, 2, 0));
    vector = Vector3(2, -3, 4);
    EXPECT_FALSE(Quaternion(0, 0, 0, 0).TryRotateVector(vector, vector));
    EXPECT_EQ(vector, Vector3::zero);
    vector = Vector3(std::numeric_limits<float>::quiet_NaN(), 2, 3);
    EXPECT_FALSE(rotation.TryRotateVector(vector, vector));
    EXPECT_EQ(vector, Vector3::zero);
}

TEST(QuaternionTest, RotateVectorHandlesLargeQuaternionAndRepresentableExtremeVectors)
{
    const float largest = (std::numeric_limits<float>::max)();
    Vector3 result;
    ASSERT_TRUE(Quaternion(0, 0, largest, largest).TryRotateVector(Vector3(2, 0, 0), result));
    ExpectVectorNear(result, Vector3(0, 2, 0));
    const Vector3 large(largest, -largest, largest);
    ASSERT_TRUE(Quaternion::identity.TryRotateVector(large, result));
    EXPECT_EQ(result, large);
    const float smallestNormal = (std::numeric_limits<float>::min)();
    const Vector3 tiny(smallestNormal, -smallestNormal, smallestNormal);
    ASSERT_TRUE(Quaternion::identity.TryRotateVector(tiny, result));
    EXPECT_EQ(result, tiny);
}

TEST(QuaternionTest, RotateVectorOverflowReturnsFalseAndZeroWithoutChangingInput)
{
    // +45 degrees about Z sends (max,max,0) to (0,sqrt(2)*max,0), outside float range.
    const Quaternion rotation(0, 0, 0.3826834323650898f, 0.9238795325112867f);
    const float largest = (std::numeric_limits<float>::max)();
    const Vector3 input(largest, largest, 0);
    Vector3 result(9, 8, 7);
    EXPECT_FALSE(rotation.TryRotateVector(input, result));
    EXPECT_EQ(result, Vector3::zero);
    EXPECT_EQ(input, Vector3(largest, largest, 0));
    Vector3 aliased = input;
    EXPECT_FALSE(rotation.TryRotateVector(aliased, aliased));
    EXPECT_EQ(aliased, Vector3::zero);
}

TEST_P(QuaternionInterpolationTest, EndpointsNormalizeInputsAndPreserveSources)
{
    const Quaternion from(0, 0, 0, 2);
    const Quaternion to(0, 0, -3, -3);
    constexpr float sqrtHalf = 0.7071067811865475f;
    Quaternion result(9, 8, 7, 6);
    ASSERT_TRUE(GetParam().interpolate(from, to, 0.0f, result));
    ExpectSameUnitRotation(result, Quaternion::identity);
    ASSERT_TRUE(GetParam().interpolate(from, to, 1.0f, result));
    ExpectSameUnitRotation(result, Quaternion(0, 0, sqrtHalf, sqrtHalf));
    ExpectComponents(from, {0, 0, 0, 2});
    ExpectComponents(to, {0, 0, -3, -3});
}

TEST_P(QuaternionInterpolationTest, IdenticalAndOppositeSignsKeepTheSameOrientation)
{
    const Quaternion from(0.5f, -0.5f, 0.5f, -0.5f);
    for (float sign : {1.0f, -1.0f})
    {
        SCOPED_TRACE(sign);
        const Quaternion to(sign * from.x, sign * from.y, sign * from.z, sign * from.w);
        for (float t : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
        {
            SCOPED_TRACE(t);
            Quaternion result;
            ASSERT_TRUE(GetParam().interpolate(from, to, t, result));
            ExpectSameUnitRotation(result, from);
        }
    }
}

TEST_P(QuaternionInterpolationTest, KnownMidpointsIncludeExactHalfTurnSeparation)
{
    constexpr float sqrtHalf = 0.7071067811865475f;
    constexpr float sqrtThreeOverTwo = 0.8660254037844386f;
    Quaternion result;
    // 0 -> 120 degrees about Z has a 60-degree midpoint.
    ASSERT_TRUE(GetParam().interpolate(Quaternion::identity,
        Quaternion(0, 0, sqrtThreeOverTwo, 0.5f), 0.5f, result));
    ExpectSameUnitRotation(result, Quaternion(0, 0, 0.5f, sqrtThreeOverTwo));
    // Dot is exactly zero: two equally short paths exist; this representation chooses +Z.
    ASSERT_TRUE(GetParam().interpolate(Quaternion::identity,
        Quaternion(0, 0, 1, 0), 0.5f, result));
    ExpectSameUnitRotation(result, Quaternion(0, 0, sqrtHalf, sqrtHalf));
}

TEST_P(QuaternionInterpolationTest, ShortestPathCrossesAngleWrapInsteadOfGoingThroughZero)
{
    // +170 -> -170 degrees about Z must pass through 180, not 0 degrees.
    const Quaternion from(0, 0, 0.9961946980917455f, 0.0871557427476582f);
    const Quaternion to(0, 0, -0.9961946980917455f, 0.0871557427476582f);
    Quaternion result;
    ASSERT_TRUE(GetParam().interpolate(from, to, 0.5f, result));
    ExpectSameUnitRotation(result, Quaternion(0, 0, 1, 0));
    ExpectRotatedVector(result, Vector3::unitX, Vector3(-1, 0, 0));
}

TEST_P(QuaternionInterpolationTest, EndpointSignChangesDoNotChangeTheChosenOrientation)
{
    const Quaternion from(0.5f, 0.5f, 0.5f, 0.5f);
    const Quaternion to(0, 0, 0.7071067811865475f, 0.7071067811865475f);
    for (float t : {0.2f, 0.8f})
    {
        SCOPED_TRACE(t);
        Quaternion reference;
        ASSERT_TRUE(GetParam().interpolate(from, to, t, reference));
        for (float fromSign : {1.0f, -1.0f})
        {
            for (float toSign : {1.0f, -1.0f})
            {
                SCOPED_TRACE(fromSign);
                SCOPED_TRACE(toSign);
                const Quaternion a(fromSign * from.x, fromSign * from.y, fromSign * from.z, fromSign * from.w);
                const Quaternion b(toSign * to.x, toSign * to.y, toSign * to.z, toSign * to.w);
                Quaternion result;
                ASSERT_TRUE(GetParam().interpolate(a, b, t, result));
                ExpectSameUnitRotation(result, reference);
            }
        }
    }
}

TEST_P(QuaternionInterpolationTest, InvalidTAlwaysReturnsFalseAndIdentity)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    for (float t : {-0.1f, std::nextafter(0.0f, -1.0f),
        std::nextafter(1.0f, 2.0f), 1.1f, nan, infinity, -infinity})
    {
        SCOPED_TRACE(t);
        Quaternion result(9, 8, 7, 6);
        EXPECT_FALSE(GetParam().interpolate(Quaternion::identity, Quaternion(0, 0, 1, 0), t, result));
        ExpectComponents(result, kIdentity);
    }
}

TEST_P(QuaternionInterpolationTest, BothEndpointsAreValidatedEvenAtZeroAndOne)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    std::vector<Components> invalid{{0, 0, 0, 0}, {0, 0, 0, 1e-5f}};
    for (size_t component = 0; component < 4; ++component)
    {
        for (float value : {nan, infinity, -infinity})
        {
            Components components = kIdentity;
            components[component] = value;
            invalid.push_back(components);
        }
    }
    for (size_t i = 0; i < invalid.size(); ++i)
    {
        SCOPED_TRACE(i);
        const Quaternion bad = MakeQuaternion(invalid[i]);
        for (float t : {0.0f, 0.5f, 1.0f})
        {
            SCOPED_TRACE(t);
            Quaternion result(9, 8, 7, 6);
            EXPECT_FALSE(GetParam().interpolate(bad, Quaternion::identity, t, result));
            ExpectComponents(result, kIdentity);
            result = Quaternion(9, 8, 7, 6);
            EXPECT_FALSE(GetParam().interpolate(Quaternion::identity, bad, t, result));
            ExpectComponents(result, kIdentity);
            ExpectComponents(bad, invalid[i]);
        }
    }
}

TEST_P(QuaternionInterpolationTest, NormalizationThresholdAndLargeFiniteEndpoints)
{
    const float below = std::nextafter(1e-4f, 0.0f);
    const float above = std::nextafter(1e-4f, 1.0f);
    Quaternion result(9, 8, 7, 6);
    EXPECT_FALSE(GetParam().interpolate(Quaternion(0, 0, 0, below), Quaternion::identity, 0.5f, result));
    ExpectComponents(result, kIdentity);
    ASSERT_TRUE(GetParam().interpolate(Quaternion(0, 0, 0, above), Quaternion::identity, 0.5f, result));
    ExpectSameUnitRotation(result, Quaternion::identity);
    const float largest = (std::numeric_limits<float>::max)();
    ASSERT_TRUE(GetParam().interpolate(Quaternion(0, 0, 0, largest),
        Quaternion(0, 0, largest, largest), 0.5f, result));
    ExpectSameUnitRotation(result, Quaternion(0, 0, 0.3826834323650898f, 0.9238795325112867f));
}

TEST_P(QuaternionInterpolationTest, OutputMayAliasEitherOrBothInputs)
{
    const Quaternion from(0.5f, 0.5f, 0.5f, 0.5f);
    const Quaternion to(0, 0, 0.7071067811865475f, 0.7071067811865475f);
    for (float t : {0.0f, 0.35f, 1.0f})
    {
        SCOPED_TRACE(t);
        Quaternion reference;
        ASSERT_TRUE(GetParam().interpolate(from, to, t, reference));
        Quaternion fromOutput = from;
        ASSERT_TRUE(GetParam().interpolate(fromOutput, to, t, fromOutput));
        ExpectSameUnitRotation(fromOutput, reference);
        Quaternion toOutput = to;
        ASSERT_TRUE(GetParam().interpolate(from, toOutput, t, toOutput));
        ExpectSameUnitRotation(toOutput, reference);
    }
    Quaternion both = from;
    ASSERT_TRUE(GetParam().interpolate(both, both, 0.5f, both));
    ExpectSameUnitRotation(both, from);
    Quaternion output = from;
    EXPECT_FALSE(GetParam().interpolate(output, to, -1.0f, output));
    ExpectComponents(output, kIdentity);
    output = Quaternion(0, 0, 0, 0);
    EXPECT_FALSE(GetParam().interpolate(from, output, 0.5f, output));
    ExpectComponents(output, kIdentity);
}

TEST_P(QuaternionInterpolationTest, NearIdenticalAnglesRemainFiniteAroundSlerpFallback)
{
    // Dots on both sides of the 0.9995 fallback threshold, plus an almost identical pair.
    for (float cosine : {0.9994f, 0.9995f, 0.9996f, std::nextafter(1.0f, 0.0f)})
    {
        SCOPED_TRACE(cosine);
        const double theta = std::acos(static_cast<double>(cosine));
        const Quaternion to(0, static_cast<float>(std::sin(theta)), 0, cosine);
        Quaternion result;
        ASSERT_TRUE(GetParam().interpolate(Quaternion::identity, to, 0.5f, result));
        // Both interpolations have the same angular midpoint.
        ExpectSameUnitRotation(result, Quaternion(0, static_cast<float>(std::sin(theta * 0.5)),
            0, static_cast<float>(std::cos(theta * 0.5))));
    }
}

TEST_P(QuaternionInterpolationTest, NearFallbackNonMidpointsStayWithinAngularErrorBudget)
{
    const double thresholdAngle = std::acos(0.9995);
    // Sample on either side of the fallback and at tiny separations.
    for (double halfAngle : {1e-7, 0.028, thresholdAngle - 1e-5, thresholdAngle + 1e-5, 0.035})
    {
        SCOPED_TRACE(halfAngle);
        const Quaternion to(0, 0, static_cast<float>(std::sin(halfAngle)),
            static_cast<float>(std::cos(halfAngle)));
        const double inputHalfAngle = std::atan2(static_cast<double>(to.z), static_cast<double>(to.w));
        for (float t : {0.125f, 0.25f, 0.75f, 0.875f})
        {
            SCOPED_TRACE(t);
            Quaternion result;
            ASSERT_TRUE(GetParam().interpolate(Quaternion::identity, to, t, result));
            ExpectUnitQuaternion(result);
            EXPECT_NEAR(result.x, 0.0f, 1e-7f);
            EXPECT_NEAR(result.y, 0.0f, 1e-7f);
            // atan2 avoids acos(dot)'s precision loss near identical rotations.
            const double actualAngle = 2.0 * std::atan2(static_cast<double>(result.z),
                static_cast<double>(result.w));
            const double constantSpeedAngle = 2.0 * t * inputHalfAngle;
            // For these small separations, even Nlerp's nonconstant-speed error is below
            // 2e-6 radians (~0.000115 degrees). This is a sampled bound, not a global guarantee.
            EXPECT_NEAR(actualAngle, constantSpeedAngle, 2e-6);
        }
    }
}

TEST_P(QuaternionInterpolationTest, HalfTurnBoundarySelectsTheShorterBranchOnEachSide)
{
    for (float scalar : {1e-3f, 1e-6f, -1e-6f, -1e-3f})
    {
        SCOPED_TRACE(scalar);
        const double w = scalar;
        const Quaternion to(0, 0, static_cast<float>(std::sqrt(1.0 - w * w)), scalar);
        Quaternion result;
        ASSERT_TRUE(GetParam().interpolate(Quaternion::identity, to, 0.5f, result));
        // Below 180 degrees use +Z; above 180 degrees use -Z. At exactly 180
        // either direction is equally short, so no continuity across that tie is required.
        const double sign = scalar < 0.0f ? -1.0 : 1.0;
        const Quaternion expected(0, 0,
            static_cast<float>(sign * std::sqrt((1.0 - std::abs(w)) * 0.5)),
            static_cast<float>(std::sqrt((1.0 + std::abs(w)) * 0.5)));
        ExpectSameUnitRotation(result, expected);
    }
}

INSTANTIATE_TEST_SUITE_P(
    Algorithms, QuaternionInterpolationTest,
    ::testing::Values(InterpolationCase{"Nlerp", &Quaternion::TryNlerp},
        InterpolationCase{"Slerp", &Quaternion::TrySlerp}),
    [](const ::testing::TestParamInfo<InterpolationCase>& info) { return std::string(info.param.name); });

TEST(QuaternionTest, SlerpAdvancesAtConstantAngularRateForFixedEndpoints)
{
    // 0 -> 120 degrees: the expected quaternion half-angle is t * 60 degrees.
    const Quaternion to(0, 0, 0.8660254037844386f, 0.5f);
    constexpr double halfAngle = 1.0471975511965977;
    for (float t : {0.0f, 0.125f, 0.25f, 0.5f, 0.75f, 0.875f, 1.0f})
    {
        SCOPED_TRACE(t);
        Quaternion result;
        ASSERT_TRUE(Quaternion::TrySlerp(Quaternion::identity, to, t, result));
        ExpectSameUnitRotation(result, Quaternion(0, 0,
            static_cast<float>(std::sin(t * halfAngle)), static_cast<float>(std::cos(t * halfAngle))));
    }
}

TEST(QuaternionTest, NlerpQuarterStepUsesNormalizedChordRatherThanConstantAngle)
{
    const Quaternion to(0, 0, 0.8660254037844386f, 0.5f);
    Quaternion result;
    ASSERT_TRUE(Quaternion::TryNlerp(Quaternion::identity, to, 0.25f, result));
    // Independently evaluated: normalized (0,0,sqrt(3)/8,7/8).
    ExpectSameUnitRotation(result, Quaternion(0, 0, 0.2401922307076307f, 0.9707253433941511f));
    // Slerp's quarter step would be 30 degrees, with z=sin(15 degrees).
    EXPECT_GT(std::abs(result.z - 0.2588190451025207f), 0.01f);
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
