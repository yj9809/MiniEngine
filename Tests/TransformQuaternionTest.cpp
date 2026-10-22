#include <gtest/gtest.h>

#include "Component/Transform/TransformComponent.h"

#include <limits>

using namespace Engine;

namespace
{
    void ExpectRotationEqual(const Quaternion& actual, const Quaternion& expected)
    {
        EXPECT_FLOAT_EQ(actual.x, expected.x);
        EXPECT_FLOAT_EQ(actual.y, expected.y);
        EXPECT_FLOAT_EQ(actual.z, expected.z);
        EXPECT_FLOAT_EQ(actual.w, expected.w);
    }

    void ExpectRotatedAxis(const Quaternion& rotation, const Vector3& axis, const Vector3& expected)
    {
        Vector3 actual;
        ASSERT_TRUE(rotation.TryRotateVector(axis, actual));
        EXPECT_NEAR(actual.x, expected.x, 1e-6f);
        EXPECT_NEAR(actual.y, expected.y, 1e-6f);
        EXPECT_NEAR(actual.z, expected.z, 1e-6f);
    }

    void ExpectVectorNear(const Vector3& actual, const Vector3& expected)
    {
        EXPECT_NEAR(actual.x, expected.x, 1e-5f);
        EXPECT_NEAR(actual.y, expected.y, 1e-5f);
        EXPECT_NEAR(actual.z, expected.z, 1e-5f);
    }
}

TEST(TransformQuaternionDirectionTest, IdentityAndSignedQuarterTurnsUseObjectAxes)
{
    TransformComponent transform;
    ExpectVectorNear(transform.GetForward(), Vector3::unitX);
    ExpectVectorNear(transform.GetRight(), Vector3::unitY);
    ExpectVectorNear(transform.GetUp(), Vector3::unitZ);

    for (float sign : {-1.0f, 1.0f})
    {
        SCOPED_TRACE(sign);
        transform.SetLocalRotationEulerDeg(Vector3(0, sign * 90, 0));
        ExpectVectorNear(transform.GetForward(), Vector3(0, sign, 0));
        ExpectVectorNear(transform.GetRight(), Vector3(-sign, 0, 0));
        ExpectVectorNear(transform.GetUp(), Vector3::unitZ);

        transform.SetLocalRotationEulerDeg(Vector3(sign * 90, 0, 0));
        ExpectVectorNear(transform.GetForward(), Vector3(0, 0, sign));
        ExpectVectorNear(transform.GetRight(), Vector3::unitY);
        ExpectVectorNear(transform.GetUp(), Vector3(-sign, 0, 0));

        transform.SetLocalRotationEulerDeg(Vector3(0, 0, sign * 90));
        ExpectVectorNear(transform.GetForward(), Vector3::unitX);
        ExpectVectorNear(transform.GetRight(), Vector3(0, 0, -sign));
        ExpectVectorNear(transform.GetUp(), Vector3(0, sign, 0));
    }
}

TEST(TransformQuaternionDirectionTest, MixedEulerMatchesIndependentBasis)
{
    TransformComponent transform;
    transform.SetLocalRotationEulerDeg(Vector3(20, 40, 30));
    ExpectVectorNear(transform.GetForward(), Vector3(0.71984631f, 0.60402277f, 0.34202014f));
    ExpectVectorNear(transform.GetRight(), Vector3(-0.42566908f, 0.77333710f, -0.46984631f));
    ExpectVectorNear(transform.GetUp(), Vector3(-0.54829474f, 0.19262973f, 0.81379768f));
}

TEST(TransformQuaternionDirectionTest, SampledPosesMatchQuaternionRotationAndRemainOrthonormal)
{
    for (float roll : {-170.0f, 0.0f, 30.0f, 90.0f})
    for (float pitch : {-100.0f, -90.0f, -20.0f, 0.0f, 90.0f, 100.0f})
    for (float yaw : {-180.0f, 0.0f, 40.0f, 90.0f})
    {
        SCOPED_TRACE(::testing::Message() << roll << ',' << pitch << ',' << yaw);
        Quaternion rotation;
        ASSERT_TRUE(Quaternion::TryFromEuler(roll * PI / 180, pitch * PI / 180, yaw * PI / 180, rotation));
        Vector3 expectedForward, expectedRight, expectedUp;
        ASSERT_TRUE(rotation.TryRotateVector(Vector3::unitX, expectedForward));
        ASSERT_TRUE(rotation.TryRotateVector(Vector3::unitY, expectedRight));
        ASSERT_TRUE(rotation.TryRotateVector(Vector3::unitZ, expectedUp));

        // Both quaternion signs must describe the same complete object basis.
        for (float sign : {-1.0f, 1.0f})
        {
            SCOPED_TRACE(sign);
            TransformComponent transform;
            ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(
                sign * rotation.x, sign * rotation.y, sign * rotation.z, sign * rotation.w)));
            const Vector3 forward = transform.GetForward();
            const Vector3 right = transform.GetRight();
            const Vector3 up = transform.GetUp();
            ExpectVectorNear(forward, expectedForward);
            ExpectVectorNear(right, expectedRight);
            ExpectVectorNear(up, expectedUp);
            EXPECT_NEAR(forward.LengthSquared(), 1.0f, 1e-6f);
            EXPECT_NEAR(right.LengthSquared(), 1.0f, 1e-6f);
            EXPECT_NEAR(up.LengthSquared(), 1.0f, 1e-6f);
            EXPECT_NEAR(forward.Dot(right), 0.0f, 1e-6f);
            EXPECT_NEAR(right.Dot(up), 0.0f, 1e-6f);
            EXPECT_NEAR(up.Dot(forward), 0.0f, 1e-6f);
            ExpectVectorNear(forward.Cross(right), up);
        }
    }
}

TEST(TransformQuaternionDirectionTest, QueriesIgnorePositionAndScaleWithoutChangingPoseOrWorldUpdates)
{
    for (bool cacheLatestPose : {false, true})
    for (const Vector3& position : {Vector3::zero, Vector3(10, -20, 30)})
    for (const Vector3& scale : {Vector3::one, Vector3(2, 3, 4),
        Vector3(-2, 3, -4), Vector3(0, 3, 0), Vector3::zero})
    {
        SCOPED_TRACE(::testing::Message() << cacheLatestPose << "; scale="
            << scale.x << ',' << scale.y << ',' << scale.z);
        TransformComponent transform;
        transform.SetLocalPosition(position);
        transform.SetLocalScale(scale);
        const Matrix4 oldWorld = transform.GetWorldMatrix();
        // A 120-degree rotation about (1,1,1) cycles X -> Y -> Z -> X.
        ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(1, 1, 1, 1)));
        const Quaternion previous = transform.GetLocalRotation();
        if (cacheLatestPose)
        {
            ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3(0, scale.x, 0));
        }
        int notifications = 0;
        transform.onTransformChanged = [&]() { ++notifications; };
        const TransformComponent& view = transform;
        for (int read = 0; read < 3; ++read)
        {
            ExpectVectorNear(view.GetForward(), Vector3::unitY);
            ExpectVectorNear(view.GetRight(), Vector3::unitZ);
            ExpectVectorNear(view.GetUp(), Vector3::unitX);
        }
        EXPECT_EQ(notifications, 0);
        ExpectRotationEqual(transform.GetLocalRotation(), previous);
        ExpectVectorNear(transform.GetLocalPosition(), position);
        ExpectVectorNear(transform.GetLocalScale(), scale);
        const Matrix4 world = transform.GetWorldMatrix();
        ExpectVectorNear(world.TransformPosition(Vector3::zero), position);
        ExpectVectorNear(world.TransformVector(Vector3::unitX), Vector3(0, scale.x, 0));
        ExpectVectorNear(world.TransformVector(Vector3::unitY), Vector3(0, 0, scale.y));
        ExpectVectorNear(world.TransformVector(Vector3::unitZ), Vector3(scale.z, 0, 0));
        ExpectVectorNear(oldWorld.TransformVector(Vector3::unitX), Vector3(scale.x, 0, 0));
    }
}

TEST(TransformQuaternionCompositionTest, NoncommutingRotationsPreservePositionScaleAndNotifyWithFreshPose)
{
    for (bool worldSpace : {false, true})
    for (float yawSign : {-1.0f, 1.0f})
    for (float pitchSign : {-1.0f, 1.0f})
    {
        SCOPED_TRACE(::testing::Message() << worldSpace << ',' << yawSign << ',' << pitchSign);
        TransformComponent transform;
        const Vector3 position(10, -20, 30);
        const Vector3 scale(2, 3, 4);
        transform.SetLocalPosition(position);
        transform.SetLocalScale(scale);
        ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(0, 0, yawSign, 1)));
        const Matrix4 previousWorld = transform.GetWorldMatrix();

        // Signed 90-degree yaw followed by signed 90-degree Euler pitch.
        // Explicit bases distinguish local-axis rotation from fixed-world-axis rotation.
        const Vector3 forward = worldSpace ? Vector3(0, yawSign, 0) : Vector3(0, 0, pitchSign);
        const Vector3 right = worldSpace ? Vector3(0, 0, -yawSign * pitchSign) : Vector3(-yawSign, 0, 0);
        const Vector3 up = worldSpace ? Vector3(-pitchSign, 0, 0) : Vector3(0, -pitchSign * yawSign, 0);
        const Quaternion delta(0, -pitchSign, 0, 1);
        int notifications = 0;
        transform.onTransformChanged = [&]()
        {
            ++notifications;
            const Quaternion rotation = transform.GetLocalRotation();
            EXPECT_NEAR(rotation.x * rotation.x + rotation.y * rotation.y +
                rotation.z * rotation.z + rotation.w * rotation.w, 1.0f, 1e-6f);
            ExpectRotatedAxis(rotation, Vector3::unitX, forward);
            ExpectRotatedAxis(rotation, Vector3::unitY, right);
            ExpectRotatedAxis(rotation, Vector3::unitZ, up);
            ExpectVectorNear(transform.GetLocalPosition(), position);
            ExpectVectorNear(transform.GetLocalScale(), scale);
            const Matrix4 world = transform.GetWorldMatrix();
            ExpectVectorNear(world.TransformPosition(Vector3::zero), position);
            ExpectVectorNear(world.TransformVector(Vector3::unitX), forward * scale.x);
            ExpectVectorNear(world.TransformVector(Vector3::unitY), right * scale.y);
            ExpectVectorNear(world.TransformVector(Vector3::unitZ), up * scale.z);
            ExpectVectorNear(transform.GetForward(), forward);
            ExpectVectorNear(transform.GetRight(), right);
            ExpectVectorNear(transform.GetUp(), up);
        };

        ASSERT_TRUE(worldSpace ? transform.TryRotateWorld(delta) : transform.TryRotateLocal(delta));
        EXPECT_EQ(notifications, 1);
        ExpectRotationEqual(delta, Quaternion(0, -pitchSign, 0, 1));
        ExpectVectorNear(previousWorld.TransformVector(Vector3::unitX), Vector3(0, 2 * yawSign, 0));
        ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), forward * scale.x);
    }
}

TEST(TransformQuaternionCompositionTest, NormalizesHugeDeltaBeforeMultiplicationWithoutCallback)
{
    for (bool worldSpace : {false, true})
    {
        SCOPED_TRACE(worldSpace);
        TransformComponent transform;
        ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(-1, -1, -1, 1)));
        const float largest = (std::numeric_limits<float>::max)();
        const Quaternion delta(largest, largest, largest, largest);
        // The inverse increment returns to identity. Multiplying before normalizing
        // would overflow the scalar component of the intermediate product.
        ASSERT_TRUE(worldSpace ? transform.TryRotateWorld(delta) : transform.TryRotateLocal(delta));
        ExpectRotationEqual(delta, Quaternion(largest, largest, largest, largest));
        ExpectRotationEqual(transform.GetLocalRotation(), Quaternion::identity);
        ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3::unitX);
        ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitY), Vector3::unitY);
        ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitZ), Vector3::unitZ);
    }
}

TEST(TransformQuaternionCompositionTest, IdentityIncrementSucceedsAndNotifiesOnce)
{
    for (bool worldSpace : {false, true})
    {
        SCOPED_TRACE(worldSpace);
        TransformComponent transform;
        ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(1, 1, 1, 1)));
        const Quaternion previous = transform.GetLocalRotation();
        int notifications = 0;
        transform.onTransformChanged = [&]() { ++notifications; };
        ASSERT_TRUE(worldSpace ? transform.TryRotateWorld(Quaternion::identity) :
            transform.TryRotateLocal(Quaternion::identity));
        EXPECT_EQ(notifications, 1);
        ExpectRotationEqual(transform.GetLocalRotation(), previous);
    }
}

TEST(TransformQuaternionIntegrationTest, DirectInputUpdatesCachedWorldWithScaleAndTranslation)
{
    TransformComponent transform;
    transform.SetLocalPosition(Vector3(10, -20, 30));
    transform.SetLocalScale(Vector3(2, 3, 4));
    ExpectVectorNear(transform.GetWorldMatrix().TransformPosition(Vector3::unitX), Vector3(12, -20, 30));

    // Normalized (1,1,1,1) maps X -> Y, Y -> Z, Z -> X.
    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(1, 1, 1, 1)));
    const Matrix4 world = transform.GetWorldMatrix();
    ExpectVectorNear(world.TransformPosition(Vector3::zero), Vector3(10, -20, 30));
    ExpectVectorNear(world.TransformPosition(Vector3::unitX), Vector3(10, -18, 30));
    ExpectVectorNear(world.TransformPosition(Vector3::unitY), Vector3(10, -20, 33));
    ExpectVectorNear(world.TransformPosition(Vector3::unitZ), Vector3(14, -20, 30));
    ExpectVectorNear(transform.GetForward(), Vector3::unitY);
}

TEST(TransformQuaternionIntegrationTest, AlternatingInputPathsRefreshAlreadyReadWorld)
{
    TransformComponent transform;
    ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3::unitX);

    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(0, 0, 1, 0)));
    ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3(-1, 0, 0));
    ExpectVectorNear(transform.GetForward(), Vector3(-1, 0, 0));

    transform.SetLocalRotationEulerDeg(Vector3(90, 0, 0));
    ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3::unitZ);
    ExpectVectorNear(transform.GetForward(), Vector3::unitZ);

    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion::identity));
    ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3::unitX);
    ExpectVectorNear(transform.GetForward(), Vector3::unitX);
}

TEST(TransformQuaternionIntegrationTest, CallbackCanReadFreshWorldAndForward)
{
    TransformComponent transform;
    const Matrix4 oldWorld = transform.GetWorldMatrix();
    int notifications = 0;
    transform.onTransformChanged = [&]()
    {
        ++notifications;
        ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3::unitY);
        ExpectVectorNear(transform.GetForward(), Vector3::unitY);
    };

    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(0, 0, 1, 1)));
    EXPECT_EQ(notifications, 1);
    ExpectVectorNear(oldWorld.TransformVector(Vector3::unitX), Vector3::unitX);
    ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3::unitY);
}

TEST(TransformQuaternionIntegrationTest, RejectedInputPreservesCachedAndPendingWorldUpdates)
{
    for (bool cacheLatestPose : {false, true})
    for (const Quaternion& invalid : {Quaternion(0, 0, 0, 0),
        Quaternion(0, 0, std::numeric_limits<float>::quiet_NaN(), 1),
        Quaternion(0, 0, 0, std::numeric_limits<float>::infinity())})
    {
        SCOPED_TRACE(cacheLatestPose);
        TransformComponent transform;
        const Matrix4 oldWorld = transform.GetWorldMatrix();
        ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(0, 0, 1, 1)));
        if (cacheLatestPose)
        {
            ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3::unitY);
        }
        int notifications = 0;
        transform.onTransformChanged = [&]() { ++notifications; };

        EXPECT_FALSE(transform.TrySetLocalRotation(invalid));
        transform.SetLocalRotationEulerDeg(Vector3(0, std::numeric_limits<float>::infinity(), 0));
        EXPECT_EQ(notifications, 0);
        ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3::unitY);
        ExpectVectorNear(transform.GetForward(), Vector3::unitY);
        ExpectVectorNear(oldWorld.TransformVector(Vector3::unitX), Vector3::unitX);
    }
}

TEST(TransformQuaternionEulerInputTest, MapsPitchYawRollDegreesWithBothSigns)
{
    TransformComponent transform;
    for (float sign : {1.0f, -1.0f})
    {
        SCOPED_TRACE(sign);
        transform.SetLocalRotationEulerDeg(Vector3(sign * 90.0f, 0, 0));
        Quaternion rotation = transform.GetLocalRotation();
        ExpectRotatedAxis(rotation, Vector3::unitX, Vector3(0, 0, sign));
        ExpectRotatedAxis(rotation, Vector3::unitY, Vector3::unitY);
        ExpectRotatedAxis(rotation, Vector3::unitZ, Vector3(-sign, 0, 0));

        transform.SetLocalRotationEulerDeg(Vector3(0, sign * 90.0f, 0));
        rotation = transform.GetLocalRotation();
        ExpectRotatedAxis(rotation, Vector3::unitX, Vector3(0, sign, 0));
        ExpectRotatedAxis(rotation, Vector3::unitY, Vector3(-sign, 0, 0));
        ExpectRotatedAxis(rotation, Vector3::unitZ, Vector3::unitZ);

        transform.SetLocalRotationEulerDeg(Vector3(0, 0, sign * 90.0f));
        rotation = transform.GetLocalRotation();
        ExpectRotatedAxis(rotation, Vector3::unitX, Vector3::unitX);
        ExpectRotatedAxis(rotation, Vector3::unitY, Vector3(0, 0, -sign));
        ExpectRotatedAxis(rotation, Vector3::unitZ, Vector3(0, sign, 0));
    }
}

TEST(TransformQuaternionEulerInputTest, MixedAnglesNotifyOnceAfterPoseIsStored)
{
    TransformComponent transform;
    int notifications = 0;
    transform.onTransformChanged = [&]()
    {
        ++notifications;
        const Quaternion rotation = transform.GetLocalRotation();
        // Independent yaw/pitch/roll basis for Pitch=20, Yaw=40, Roll=30 degrees.
        ExpectRotatedAxis(rotation, Vector3::unitX, Vector3(0.71984631f, 0.60402277f, 0.34202014f));
        ExpectRotatedAxis(rotation, Vector3::unitY, Vector3(-0.42566908f, 0.77333710f, -0.46984631f));
        ExpectRotatedAxis(rotation, Vector3::unitZ, Vector3(-0.54829474f, 0.19262973f, 0.81379768f));
        EXPECT_NEAR(rotation.x * rotation.x + rotation.y * rotation.y +
            rotation.z * rotation.z + rotation.w * rotation.w, 1.0f, 1e-6f);
    };
    transform.SetLocalRotationEulerDeg(Vector3(20, 40, 30));
    EXPECT_EQ(notifications, 1);
}

TEST(TransformQuaternionEulerInputTest, InvalidAnglesPreserveQuaternionPoseWithoutNotification)
{
    TransformComponent transform;
    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(0, 0, 3, 4)));
    const Quaternion previous = transform.GetLocalRotation();
    int notifications = 0;
    transform.onTransformChanged = [&]() { ++notifications; };

    for (float invalid : {std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()})
    {
        for (const Vector3& input : {Vector3(invalid, 40, 30),
            Vector3(20, invalid, 30), Vector3(20, 40, invalid)})
        {
            SCOPED_TRACE(::testing::Message() << input.x << ',' << input.y << ',' << input.z);
            transform.SetLocalRotationEulerDeg(input);
            ExpectRotationEqual(transform.GetLocalRotation(), previous);
            EXPECT_EQ(notifications, 0);
        }
    }
}

TEST(TransformQuaternionEulerInputTest, ZeroEulerReplacesExistingQuaternionPose)
{
    TransformComponent transform;
    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(0, 0, 3, 4)));
    transform.SetLocalRotationEulerDeg(Vector3::zero);
    ExpectRotationEqual(transform.GetLocalRotation(), Quaternion(0, 0, 0, 1));
}

TEST(TransformQuaternionTest, DefaultsToIdentity)
{
    TransformComponent transform;
    ExpectRotationEqual(transform.GetLocalRotation(), Quaternion(0, 0, 0, 1));
}

TEST(TransformQuaternionTest, GetterReturnsIndependentValue)
{
    TransformComponent transform;
    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(0, 0, 3, 4)));
    Quaternion copy = transform.GetLocalRotation();
    copy.z = 0;
    copy.w = 0;
    ExpectRotationEqual(transform.GetLocalRotation(), Quaternion(0, 0, 0.6f, 0.8f));
}

TEST(TransformQuaternionTest, NormalizesWithoutCallbackAndPreservesInput)
{
    TransformComponent transform;
    const Quaternion input(0, 0, 3, 4);
    bool accepted = false;
    ASSERT_NO_THROW(accepted = transform.TrySetLocalRotation(input));
    ASSERT_TRUE(accepted);
    ExpectRotationEqual(transform.GetLocalRotation(), Quaternion(0, 0, 0.6f, 0.8f));
    ExpectRotationEqual(input, Quaternion(0, 0, 3, 4));
}

TEST(TransformQuaternionTest, CallbackObservesCommittedNormalizedRotationOnce)
{
    TransformComponent transform;
    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(0, 0, 3, 4)));
    int notifications = 0;
    transform.onTransformChanged = [&]()
    {
        ++notifications;
        ExpectRotationEqual(transform.GetLocalRotation(), Quaternion(0.6f, 0.8f, 0, 0));
    };

    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(3, 4, 0, 0)));
    EXPECT_EQ(notifications, 1);
}

TEST(TransformQuaternionTest, AcceptsLargeFiniteInputWithoutOverflow)
{
    TransformComponent transform;
    const float largest = (std::numeric_limits<float>::max)();
    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(largest, largest, largest, largest)));
    ExpectRotationEqual(transform.GetLocalRotation(), Quaternion(0.5f, 0.5f, 0.5f, 0.5f));
}

class TransformQuaternionInvalidInputTest : public ::testing::TestWithParam<Quaternion>
{
};

TEST_P(TransformQuaternionInvalidInputTest, PreservesPreviousPoseWithoutNotification)
{
    TransformComponent transform;
    ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(0, 0, 3, 4)));
    const Quaternion previous = transform.GetLocalRotation();
    int notifications = 0;
    transform.onTransformChanged = [&]() { ++notifications; };

    EXPECT_FALSE(transform.TrySetLocalRotation(GetParam()));
    ExpectRotationEqual(transform.GetLocalRotation(), previous);
    EXPECT_EQ(notifications, 0);
}

TEST_P(TransformQuaternionInvalidInputTest, CompositionPreservesPoseAndCachedOrPendingWorldWithoutNotification)
{
    for (bool worldSpace : {false, true})
    for (bool cacheLatestPose : {false, true})
    {
        SCOPED_TRACE(::testing::Message() << worldSpace << ',' << cacheLatestPose);
        TransformComponent transform;
        transform.SetLocalPosition(Vector3(10, -20, 30));
        transform.SetLocalScale(Vector3(2, 3, 4));
        const Matrix4 oldWorld = transform.GetWorldMatrix();
        ASSERT_TRUE(transform.TrySetLocalRotation(Quaternion(0, 0, 1, 1)));
        const Quaternion previous = transform.GetLocalRotation();
        if (cacheLatestPose)
        {
            ExpectVectorNear(transform.GetWorldMatrix().TransformVector(Vector3::unitX), Vector3(0, 2, 0));
        }
        int notifications = 0;
        transform.onTransformChanged = [&]() { ++notifications; };

        EXPECT_FALSE(worldSpace ? transform.TryRotateWorld(GetParam()) : transform.TryRotateLocal(GetParam()));
        EXPECT_EQ(notifications, 0);
        ExpectRotationEqual(transform.GetLocalRotation(), previous);
        ExpectVectorNear(transform.GetLocalPosition(), Vector3(10, -20, 30));
        ExpectVectorNear(transform.GetLocalScale(), Vector3(2, 3, 4));
        ExpectVectorNear(transform.GetForward(), Vector3::unitY);
        ExpectVectorNear(transform.GetRight(), Vector3(-1, 0, 0));
        ExpectVectorNear(transform.GetUp(), Vector3::unitZ);
        const Matrix4 world = transform.GetWorldMatrix();
        ExpectVectorNear(world.TransformPosition(Vector3::zero), Vector3(10, -20, 30));
        ExpectVectorNear(world.TransformVector(Vector3::unitX), Vector3(0, 2, 0));
        ExpectVectorNear(world.TransformVector(Vector3::unitY), Vector3(-3, 0, 0));
        ExpectVectorNear(world.TransformVector(Vector3::unitZ), Vector3(0, 0, 4));
        ExpectVectorNear(oldWorld.TransformVector(Vector3::unitX), Vector3(2, 0, 0));
    }
}

INSTANTIATE_TEST_SUITE_P(
    RejectedRotations,
    TransformQuaternionInvalidInputTest,
    ::testing::Values(
        Quaternion(0, 0, 0, 0),
        Quaternion(0, 0, 0, 1e-5f),
        Quaternion(std::numeric_limits<float>::quiet_NaN(), 0, 0, 1),
        Quaternion(0, std::numeric_limits<float>::quiet_NaN(), 0, 1),
        Quaternion(0, 0, std::numeric_limits<float>::quiet_NaN(), 1),
        Quaternion(0, 0, 0, std::numeric_limits<float>::quiet_NaN()),
        Quaternion(std::numeric_limits<float>::infinity(), 0, 0, 1),
        Quaternion(0, std::numeric_limits<float>::infinity(), 0, 1),
        Quaternion(0, 0, std::numeric_limits<float>::infinity(), 1),
        Quaternion(0, 0, 0, std::numeric_limits<float>::infinity()),
        Quaternion(-std::numeric_limits<float>::infinity(), 0, 0, 1),
        Quaternion(0, -std::numeric_limits<float>::infinity(), 0, 1),
        Quaternion(0, 0, -std::numeric_limits<float>::infinity(), 1),
        Quaternion(0, 0, 0, -std::numeric_limits<float>::infinity())));
