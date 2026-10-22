#include <gtest/gtest.h>

#include "Actor/Actor.h"
#include "Component/Camera/CameraComponent.h"
#include "Component/Transform/TransformComponent.h"

#include <limits>

using namespace Engine;

namespace
{
    void ExpectVectorNear(const Vector3& actual, const Vector3& expected)
    {
        EXPECT_NEAR(actual.x, expected.x, 1e-5f);
        EXPECT_NEAR(actual.y, expected.y, 1e-5f);
        EXPECT_NEAR(actual.z, expected.z, 1e-5f);
    }

    void ExpectMatrixNear(const Matrix4& actual, const Matrix4& expected)
    {
        for (const Vector4& basis : {Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0),
            Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1)})
        {
            const Vector4 a = actual * basis;
            const Vector4 b = expected * basis;
            EXPECT_NEAR(a.x, b.x, 1e-5f);
            EXPECT_NEAR(a.y, b.y, 1e-5f);
            EXPECT_NEAR(a.z, b.z, 1e-5f);
            EXPECT_NEAR(a.w, b.w, 1e-5f);
        }
    }

    void ExpectCameraBasis(const Matrix4& view, const Vector3& position,
        const Vector3& forward, const Vector3& right, const Vector3& up)
    {
        ExpectVectorNear(view.TransformPosition(position), Vector3::zero);
        ExpectVectorNear(view.TransformVector(forward), Vector3::unitZ);
        ExpectVectorNear(view.TransformVector(right), Vector3::unitX);
        ExpectVectorNear(view.TransformVector(up), Vector3::unitY);
        ExpectVectorNear(view.TransformPosition(position + forward * 2.0f + right * 3.0f + up * 4.0f),
            Vector3(3, 4, 2));
    }

    class CameraQuaternionTest : public ::testing::Test
    {
    protected:
        Actor actor;
        CameraComponent* camera = actor.AddComponent<CameraComponent>();
        TransformComponent* transform = actor.GetRootComponent();
    };
}

TEST_F(CameraQuaternionTest, IdentityPoseUsesCameraAxisConvention)
{
    ExpectCameraBasis(camera->GetViewMatrix(), Vector3::zero,
        Vector3::unitX, Vector3::unitY, Vector3::unitZ);
}

TEST_F(CameraQuaternionTest, EulerRollAndPolesPreserveAllThreeAxes)
{
    const Vector3 position(3, -2, 5);
    transform->SetLocalPosition(position);
    for (float pitch : {-90.0f, -20.0f, 0.0f, 20.0f, 90.0f})
    for (float yaw : {-170.0f, 0.0f, 40.0f})
    for (float roll : {-90.0f, 0.0f, 30.0f, 90.0f})
    {
        SCOPED_TRACE(::testing::Message() << pitch << ',' << yaw << ',' << roll);
        transform->SetLocalRotationEulerDeg(Vector3(pitch, yaw, roll));
        const Quaternion rotation = transform->GetLocalRotation();
        Vector3 forward, right, up;
        // Use quaternion vector rotation as a reference independent of the
        // Transform matrix path used by the Camera implementation.
        ASSERT_TRUE(rotation.TryRotateVector(Vector3::unitX, forward));
        ASSERT_TRUE(rotation.TryRotateVector(Vector3::unitY, right));
        ASSERT_TRUE(rotation.TryRotateVector(Vector3::unitZ, up));
        ExpectCameraBasis(camera->GetViewMatrix(), position, forward, right, up);
    }
}

TEST_F(CameraQuaternionTest, DirectLocalWorldRotationAndPositionChangesRefreshCachedView)
{
    const Matrix4 initial = camera->GetViewMatrix();
    ASSERT_TRUE(transform->TrySetLocalRotation(Quaternion(0, 0, 1, 1)));
    ExpectCameraBasis(camera->GetViewMatrix(), Vector3::zero,
        Vector3::unitY, Vector3(-1, 0, 0), Vector3::unitZ);

    ASSERT_TRUE(transform->TryRotateLocal(Quaternion(0, -1, 0, 1)));
    ExpectCameraBasis(camera->GetViewMatrix(), Vector3::zero,
        Vector3::unitZ, Vector3(-1, 0, 0), Vector3(0, -1, 0));

    ASSERT_TRUE(transform->TryRotateWorld(Quaternion(0, 0, 1, 1)));
    ExpectCameraBasis(camera->GetViewMatrix(), Vector3::zero,
        Vector3::unitZ, Vector3(0, -1, 0), Vector3::unitX);

    transform->SetLocalPosition(Vector3(4, -3, 2));
    ExpectCameraBasis(camera->GetViewMatrix(), Vector3(4, -3, 2),
        Vector3::unitZ, Vector3(0, -1, 0), Vector3::unitX);
    ExpectCameraBasis(initial, Vector3::zero, Vector3::unitX, Vector3::unitY, Vector3::unitZ);
}

TEST_F(CameraQuaternionTest, IgnoresNonuniformNegativeAndZeroScale)
{
    transform->SetLocalPosition(Vector3(3, -2, 5));
    ASSERT_TRUE(transform->TrySetLocalRotation(Quaternion(1, 1, 1, 1)));
    for (const Vector3& scale : {Vector3(2, 3, 4), Vector3(-2, 3, -4), Vector3::zero})
    {
        transform->SetLocalScale(scale);
        ExpectCameraBasis(camera->GetViewMatrix(), Vector3(3, -2, 5),
            Vector3::unitY, Vector3::unitZ, Vector3::unitX);
        // Force another View build with this scale as well as reading the cache.
        transform->SetLocalPosition(Vector3(3, -2, 5));
        ExpectCameraBasis(camera->GetViewMatrix(), Vector3(3, -2, 5),
            Vector3::unitY, Vector3::unitZ, Vector3::unitX);
    }
}

TEST_F(CameraQuaternionTest, InitialFailureReturnsIdentityThenRecovers)
{
    transform->SetLocalPosition(Vector3(std::numeric_limits<float>::infinity(), 0, 0));
    ExpectMatrixNear(camera->GetViewMatrix(), Matrix4::identity);
    ExpectMatrixNear(camera->GetViewMatrix(), Matrix4::identity);
    transform->SetLocalPosition(Vector3(4, -3, 2));
    ExpectCameraBasis(camera->GetViewMatrix(), Vector3(4, -3, 2),
        Vector3::unitX, Vector3::unitY, Vector3::unitZ);
}

TEST_F(CameraQuaternionTest, InvalidPositionsPreserveLastSuccessfulViewAndRecover)
{
    transform->SetLocalPosition(Vector3(3, -2, 5));
    ASSERT_TRUE(transform->TrySetLocalRotation(Quaternion(1, 1, 1, 1)));
    const Matrix4 previous = camera->GetViewMatrix();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    for (const Vector3& invalid : {Vector3(nan, 0, 0), Vector3(0, infinity, 0),
        Vector3(0, 0, -infinity), Vector3(33554432, 33554432, 33554432)})
    {
        // The large finite position loses the unit target offset in float,
        // exercising a degenerate LookAt input as well as nonfinite inputs.
        transform->SetLocalPosition(invalid);
        ExpectMatrixNear(camera->GetViewMatrix(), previous);
        ExpectMatrixNear(camera->GetViewMatrix(), previous);
    }
    transform->SetLocalPosition(Vector3(-4, 1, 2));
    ExpectCameraBasis(camera->GetViewMatrix(), Vector3(-4, 1, 2),
        Vector3::unitY, Vector3::unitZ, Vector3::unitX);
}

TEST_F(CameraQuaternionTest, FailedUpdateRemainsPendingWithoutAnotherNotification)
{
    transform->SetLocalPosition(Vector3(3, -2, 5));
    const Matrix4 previous = camera->GetViewMatrix();
    transform->SetLocalPosition(Vector3(std::numeric_limits<float>::infinity(), 0, 0));
    ExpectMatrixNear(camera->GetViewMatrix(), previous);

    // Isolate the pending-retry contract: recovery must not rely on a new
    // callback setting dirtyView again. This is only test instrumentation.
    transform->onTransformChanged = {};
    transform->SetLocalPosition(Vector3(-4, 1, 2));
    ExpectCameraBasis(camera->GetViewMatrix(), Vector3(-4, 1, 2),
        Vector3::unitX, Vector3::unitY, Vector3::unitZ);
}
