#include <gtest/gtest.h>

#include "Actor/Actor.h"
#include "Component/Transform/TransformComponent.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"

#include <cmath>
#include <limits>

using namespace Engine;

namespace
{
    constexpr float kDegreesToRadians = PI / 180.0f;

    ::testing::AssertionResult HasCanonicalRange(const Vector3& angles)
    {
        if (std::isfinite(angles.x) && std::isfinite(angles.y) && std::isfinite(angles.z) &&
            angles.x >= -PI && angles.x < PI && angles.y >= -PI * 0.5f &&
            angles.y <= PI * 0.5f && angles.z >= -PI && angles.z < PI)
            return ::testing::AssertionSuccess();
        return ::testing::AssertionFailure() << "Roll/Pitch/Yaw outside canonical range: "
            << angles.x << ',' << angles.y << ',' << angles.z;
    }

    ::testing::AssertionResult SamePose(const Quaternion& a, const Quaternion& b, float tolerance = 4e-6f)
    {
        for (const Vector3& basis : {Vector3::unitX, Vector3::unitY, Vector3::unitZ})
        {
            Vector3 first, second;
            if (!a.TryRotateVector(basis, first) || !b.TryRotateVector(basis, second))
                return ::testing::AssertionFailure() << "Vector rotation failed";
            if (std::abs(first.x - second.x) > tolerance ||
                std::abs(first.y - second.y) > tolerance || std::abs(first.z - second.z) > tolerance)
                return ::testing::AssertionFailure() << "Basis mismatch: (" << first.x << ',' << first.y << ','
                    << first.z << ") versus (" << second.x << ',' << second.y << ',' << second.z << ')';
        }
        return ::testing::AssertionSuccess();
    }

    void ExpectAnglesNear(const Vector3& actual, const Vector3& expected, float tolerance = 1e-6f)
    {
        EXPECT_NEAR(actual.x, expected.x, tolerance);
        EXPECT_NEAR(actual.y, expected.y, tolerance);
        EXPECT_NEAR(actual.z, expected.z, tolerance);
    }
}

TEST(QuaternionEulerOutputTest, IdentityOverwritesOutputWithZero)
{
    Vector3 output(3, 4, 5);
    ASSERT_TRUE(Quaternion::identity.TryToEuler(output));
    ExpectAnglesNear(output, Vector3::zero);
}

TEST(QuaternionEulerOutputTest, PitchSignMatchesUnrealEulerConvention)
{
    for (float sign : {1.0f, -1.0f})
    {
        SCOPED_TRACE(sign);
        const float halfPitch = sign * 30.0f * kDegreesToRadians * 0.5f;
        const Quaternion input(0, -std::sin(halfPitch), 0, std::cos(halfPitch));
        Vector3 output;
        ASSERT_TRUE(input.TryToEuler(output));
        ExpectAnglesNear(output, Vector3(0, sign * 30.0f * kDegreesToRadians, 0));
    }
}

TEST(QuaternionEulerOutputTest, MixedAnglesReturnRollPitchYawRadians)
{
    const Vector3 expected(30 * kDegreesToRadians, 20 * kDegreesToRadians, 40 * kDegreesToRadians);
    Quaternion input;
    ASSERT_TRUE(Quaternion::TryFromEuler(expected, input));
    Vector3 output;
    ASSERT_TRUE(input.TryToEuler(output));
    ExpectAnglesNear(output, expected);
}

TEST(QuaternionEulerOutputTest, HalfTurnsUseNegativePiForBothBoundarySigns)
{
    for (const Quaternion& input : {Quaternion(1, 0, 0, 0), Quaternion(-1, 0, 0, 0),
        Quaternion(0, 0, 1, 0), Quaternion(0, 0, -1, 0)})
    {
        SCOPED_TRACE(::testing::Message() << input.x << ',' << input.z);
        Vector3 output;
        ASSERT_TRUE(input.TryToEuler(output));
        EXPECT_TRUE(HasCanonicalRange(output));
        if (input.x != 0)
            EXPECT_FLOAT_EQ(output.x, -PI);
        else
            EXPECT_FLOAT_EQ(output.z, -PI);
    }
}

TEST(QuaternionEulerOutputTest, ExactPolesSetRollZeroAndPreserveCoupledYaw)
{
    for (float sign : {1.0f, -1.0f})
    {
        SCOPED_TRACE(sign);
        Quaternion input;
        ASSERT_TRUE(Quaternion::TryFromEuler(30 * kDegreesToRadians,
            sign * 90 * kDegreesToRadians, 40 * kDegreesToRadians, input));
        Vector3 output;
        ASSERT_TRUE(input.TryToEuler(output));
        EXPECT_FLOAT_EQ(output.x, 0);
        EXPECT_NEAR(output.y, sign * PI * 0.5f, 1e-6f);
        EXPECT_NEAR(output.z, (sign > 0 ? 10 : 70) * kDegreesToRadians, 1e-6f);
        Quaternion rebuilt;
        ASSERT_TRUE(Quaternion::TryFromEuler(output, rebuilt));
        EXPECT_TRUE(SamePose(input, rebuilt));
    }
}

TEST(QuaternionEulerOutputTest, BothSidesOfPoleThresholdRespectRollPolicyAndPoseTolerance)
{
    for (float sign : {1.0f, -1.0f})
    for (float offset : {0.25e-6f, 4e-6f})
    {
        SCOPED_TRACE(::testing::Message() << sign << ',' << offset);
        Quaternion input;
        ASSERT_TRUE(Quaternion::TryFromEuler(0.3f, sign * (PI * 0.5f - offset), 0.7f, input));
        Vector3 forward;
        ASSERT_TRUE(input.TryRotateVector(Vector3::unitX, forward));
        const double horizontal = std::hypot(static_cast<double>(forward.x), static_cast<double>(forward.y));
        if (offset < 1e-6f)
            ASSERT_LT(horizontal, 0.75e-6);
        else
            ASSERT_GT(horizontal, 2e-6);

        Vector3 output;
        ASSERT_TRUE(input.TryToEuler(output));
        EXPECT_TRUE(HasCanonicalRange(output));
        if (offset < 1e-6f)
            EXPECT_FLOAT_EQ(output.x, 0);
        else
            EXPECT_GT(std::abs(output.x), 0.1f);
        Quaternion rebuilt;
        ASSERT_TRUE(Quaternion::TryFromEuler(output, rebuilt));
        EXPECT_TRUE(SamePose(input, rebuilt));
    }
}

TEST(QuaternionEulerOutputTest, RoundTripsRepresentativeAnglesIncludingBeyondPitchRange)
{
    for (float roll : {-180.0f, -30.0f, 0.0f, 45.0f})
    for (float pitch : {-110.0f, -90.0f, -20.0f, 0.0f, 20.0f, 90.0f, 110.0f})
    for (float yaw : {-180.0f, 0.0f, 40.0f, 450.0f})
    {
        SCOPED_TRACE(::testing::Message() << roll << ',' << pitch << ',' << yaw);
        Quaternion input;
        ASSERT_TRUE(Quaternion::TryFromEuler(roll * kDegreesToRadians,
            pitch * kDegreesToRadians, yaw * kDegreesToRadians, input));
        Vector3 output;
        ASSERT_TRUE(input.TryToEuler(output));
        ASSERT_TRUE(HasCanonicalRange(output));
        Quaternion rebuilt;
        ASSERT_TRUE(Quaternion::TryFromEuler(output, rebuilt));
        ASSERT_TRUE(SamePose(input, rebuilt));
    }
}

TEST(QuaternionEulerOutputTest, OppositeSignsGiveSameCanonicalAngles)
{
    for (const Quaternion& input : {Quaternion::identity, Quaternion(0.5f, 0.5f, 0.5f, 0.5f),
        Quaternion(1, 0, 0, 0), Quaternion(0, 0, 1, 0)})
    {
        const Quaternion opposite(-input.x, -input.y, -input.z, -input.w);
        Vector3 first, second;
        ASSERT_TRUE(input.TryToEuler(first));
        ASSERT_TRUE(opposite.TryToEuler(second));
        EXPECT_TRUE(HasCanonicalRange(first));
        EXPECT_TRUE(HasCanonicalRange(second));
        ExpectAnglesNear(first, second);
    }
}

TEST(QuaternionEulerOutputTest, NonUnitInputPreservesSourceAndRepresentsSamePose)
{
    const float largest = (std::numeric_limits<float>::max)();
    for (const Quaternion& input : {Quaternion(1, -2, 3, 4), Quaternion(largest, largest, largest, largest)})
    {
        const Quaternion before = input;
        Vector3 output;
        ASSERT_TRUE(input.TryToEuler(output));
        EXPECT_FLOAT_EQ(input.x, before.x);
        EXPECT_FLOAT_EQ(input.y, before.y);
        EXPECT_FLOAT_EQ(input.z, before.z);
        EXPECT_FLOAT_EQ(input.w, before.w);
        Quaternion rebuilt;
        ASSERT_TRUE(Quaternion::TryFromEuler(output, rebuilt));
        EXPECT_TRUE(SamePose(input, rebuilt));
    }
}

TEST(QuaternionEulerOutputTest, InvalidInputsFailAndOverwriteOutputWithZero)
{
    for (const Quaternion& input : {Quaternion(0, 0, 0, 0), Quaternion(0, 0, 0, 1e-5f),
        Quaternion(std::numeric_limits<float>::quiet_NaN(), 0, 0, 1),
        Quaternion(0, std::numeric_limits<float>::infinity(), 0, 1),
        Quaternion(0, 0, -std::numeric_limits<float>::infinity(), 1),
        Quaternion(0, 0, 0, std::numeric_limits<float>::quiet_NaN())})
    {
        Vector3 output(3, 4, 5);
        EXPECT_FALSE(input.TryToEuler(output));
        ExpectAnglesNear(output, Vector3::zero);
    }
}

TEST(TransformQuaternionEulerGetterTest, ReportsCurrentQuaternionAsPitchYawRollDegrees)
{
    TransformComponent transform;
    Quaternion input;
    ASSERT_TRUE(Quaternion::TryFromEuler(30 * kDegreesToRadians, 20 * kDegreesToRadians,
        40 * kDegreesToRadians, input));
    ASSERT_TRUE(transform.TrySetLocalRotation(input));
    ExpectAnglesNear(transform.GetLocalRotationEulerDeg(), Vector3(20, 40, 30), 1e-4f);
}

TEST(TransformQuaternionEulerGetterTest, DegreesStayInCanonicalRangeAtHalfTurns)
{
    for (const Quaternion& input : {Quaternion(1, 0, 0, 0), Quaternion(-1, 0, 0, 0),
        Quaternion(0, 0, 1, 0), Quaternion(0, 0, -1, 0)})
    {
        TransformComponent transform;
        ASSERT_TRUE(transform.TrySetLocalRotation(input));
        const Vector3 output = transform.GetLocalRotationEulerDeg();
        EXPECT_GE(output.x, -90);
        EXPECT_LE(output.x, 90);
        EXPECT_GE(output.y, -180);
        EXPECT_LT(output.y, 180);
        EXPECT_GE(output.z, -180);
        EXPECT_LT(output.z, 180);
        if (input.x != 0)
            EXPECT_NEAR(output.z, -180, 1e-4f);
        else
            EXPECT_NEAR(output.y, -180, 1e-4f);
    }
}

TEST(TransformQuaternionEulerGetterTest, DegreeConversionPreservesRangesNearFloatBoundaries)
{
    const float belowPi = std::nextafter(PI, 0.0f);
    const float belowHalfPi = std::nextafter(PI * 0.5f, 0.0f);
    for (float roll : {-PI, -belowPi, belowPi, PI})
    for (float pitch : {-PI * 0.5f, -belowHalfPi, 0.0f, belowHalfPi, PI * 0.5f})
    for (float yaw : {-PI, -belowPi, belowPi, PI})
    {
        SCOPED_TRACE(::testing::Message() << roll << ',' << pitch << ',' << yaw);
        Quaternion input;
        ASSERT_TRUE(Quaternion::TryFromEuler(roll, pitch, yaw, input));
        TransformComponent transform;
        ASSERT_TRUE(transform.TrySetLocalRotation(input));
        const Vector3 output = transform.GetLocalRotationEulerDeg();
        ASSERT_GE(output.x, -90);
        ASSERT_LE(output.x, 90);
        ASSERT_GE(output.y, -180);
        ASSERT_LT(output.y, 180);
        ASSERT_GE(output.z, -180);
        ASSERT_LT(output.z, 180);
        Quaternion rebuilt;
        ASSERT_TRUE(Quaternion::TryFromEuler(output.z * kDegreesToRadians,
            output.x * kDegreesToRadians, output.y * kDegreesToRadians, rebuilt));
        ASSERT_TRUE(SamePose(input, rebuilt));
    }
}

TEST(TransformQuaternionEulerGetterTest, ReadingDoesNotChangePoseWorldOrNotify)
{
    TransformComponent transform;
    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(1, 2, 3, 4)));
    const Quaternion before = transform.GetLocalRotation();
    const Matrix4 world = transform.GetWorldMatrix();
    int notifications = 0;
    transform.onTransformChanged = [&]() { ++notifications; };
    transform.GetLocalRotationEulerDeg();
    transform.GetLocalRotationEulerDeg();
    EXPECT_EQ(notifications, 0);
    const Quaternion after = transform.GetLocalRotation();
    EXPECT_FLOAT_EQ(after.x, before.x);
    EXPECT_FLOAT_EQ(after.y, before.y);
    EXPECT_FLOAT_EQ(after.z, before.z);
    EXPECT_FLOAT_EQ(after.w, before.w);
    ExpectAnglesNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX),
        world.TransformVector(Vector3::unitX));
}

TEST(TransformQuaternionEulerGetterTest, ActorGetterUsesCurrentQuaternionPose)
{
    Actor actor;
    actor.SetRotation(Vector3(20, 40, 30));
    ExpectAnglesNear(actor.GetRotation(), Vector3(20, 40, 30), 1e-4f);
}
