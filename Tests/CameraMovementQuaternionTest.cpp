#include <gtest/gtest.h>
#include <cmath>
#include <limits>

#include "../Game/Actor/CameraActor.h"
#include "Component/Transform/TransformComponent.h"
#include "Core/Input.h"

using namespace Engine;

namespace
{
    void ExpectVectorNear(const Vector3& actual, const Vector3& expected)
    {
        EXPECT_NEAR(actual.x, expected.x, 1e-5f);
        EXPECT_NEAR(actual.y, expected.y, 1e-5f);
        EXPECT_NEAR(actual.z, expected.z, 1e-5f);
    }

    void ExpectRotationEqual(const Quaternion& actual, const Quaternion& expected)
    {
        EXPECT_FLOAT_EQ(actual.x, expected.x);
        EXPECT_FLOAT_EQ(actual.y, expected.y);
        EXPECT_FLOAT_EQ(actual.z, expected.z);
        EXPECT_FLOAT_EQ(actual.w, expected.w);
    }

    void ClearInput()
    {
        for (int key : {'W', 'S', 'A', 'D'})
            Input::ProcessInputMessage(key, true, false);
        for (int button = 0; button < 3; ++button)
            Input::SetMouseButtonState(button, false, true);
        Input::SetMousePosition(Vector2(0, 0));
        Input::SetPreviousMousePosition();
        Input::ResetKeyState();
    }

    class CameraMovementQuaternionTest : public ::testing::Test
    {
    protected:
        void SetUp() override { ClearInput(); }
        void TearDown() override { ClearInput(); }
    };

    void SetLookDelta(float x, float y)
    {
        Input::SetMousePosition(Vector2(0, 0));
        Input::SetPreviousMousePosition();
        Input::SetMousePosition(Vector2(x, y));
        Input::SetMouseButtonState(1, true, false);
    }

    // Independent trigonometric basis; do not use the production Euler converter
    // to compute the expected orientation.
    void ExpectLookPose(CameraActor& actor, double pitchDeg, double yawDeg,
        double rollDeg, float tolerance = 2e-5f)
    {
        constexpr double radians = 3.14159265358979323846 / 180.0;
        const double cp = std::cos(pitchDeg * radians), sp = std::sin(pitchDeg * radians);
        const double cy = std::cos(yawDeg * radians), sy = std::sin(yawDeg * radians);
        const double cr = std::cos(rollDeg * radians), sr = std::sin(rollDeg * radians);
        const Vector3 expected[] = {
            Vector3(float(cp * cy), float(cp * sy), float(sp)),
            Vector3(float(-sy * cr + cy * sp * sr), float(cy * cr + sy * sp * sr), float(-cp * sr)),
            Vector3(float(-cy * sp * cr - sy * sr), float(-sy * sp * cr + cy * sr), float(cp * cr))
        };
        auto* transform = actor.GetRootComponent();
        const Vector3 actual[] = {transform->GetForward(), transform->GetRight(), transform->GetUp()};
        const Vector3 viewAxes[] = {Vector3::unitZ, Vector3::unitX, Vector3::unitY};
        const Matrix4 view = actor.GetComponent<CameraComponent>()->GetViewMatrix();
        for (int axis = 0; axis < 3; ++axis)
        {
            SCOPED_TRACE(axis);
            EXPECT_NEAR(actual[axis].x, expected[axis].x, tolerance);
            EXPECT_NEAR(actual[axis].y, expected[axis].y, tolerance);
            EXPECT_NEAR(actual[axis].z, expected[axis].z, tolerance);
            const Vector3 mapped = view.TransformVector(expected[axis]);
            EXPECT_NEAR(mapped.x, viewAxes[axis].x, tolerance);
            EXPECT_NEAR(mapped.y, viewAxes[axis].y, tolerance);
            EXPECT_NEAR(mapped.z, viewAxes[axis].z, tolerance);
        }
        ExpectVectorNear(view.TransformPosition(transform->GetLocalPosition()), Vector3::zero);
        const auto q = transform->GetLocalRotation();
        EXPECT_NEAR(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w, 1.0f, 1e-6f);
    }
}

TEST_F(CameraMovementQuaternionTest, WasdFollowsObjectAxesAndRefreshesView)
{
    struct Pose
    {
        Vector3 euler;
        Vector3 forward;
        Vector3 right;
    };
    const Pose poses[] = {
        {Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0)},
        {Vector3(0, 90, 0), Vector3(0, 1, 0), Vector3(-1, 0, 0)},
        {Vector3(0, -90, 0), Vector3(0, -1, 0), Vector3(1, 0, 0)},
        {Vector3(90, 0, 0), Vector3(0, 0, 1), Vector3(0, 1, 0)},
        {Vector3(-90, 0, 0), Vector3(0, 0, -1), Vector3(0, 1, 0)},
        {Vector3(0, 0, 90), Vector3(1, 0, 0), Vector3(0, 0, -1)},
        {Vector3(0, 0, -90), Vector3(1, 0, 0), Vector3(0, 0, 1)},
        {Vector3(20, 40, 30), Vector3(0.71984631f, 0.60402277f, 0.34202014f),
            Vector3(-0.42566908f, 0.77333710f, -0.46984631f)}
    };
    for (const Pose& pose : poses)
    for (int key : {'W', 'S', 'A', 'D'})
    {
        SCOPED_TRACE(::testing::Message() << pose.euler.x << ',' << pose.euler.y
            << ',' << pose.euler.z << "; key=" << key);
        ClearInput();
        CameraActor actor;
        auto* transform = actor.GetRootComponent();
        auto* camera = actor.GetComponent<CameraComponent>();
        ASSERT_NE(camera, nullptr);
        const Vector3 start(3, -2, 5);
        transform->SetLocalPosition(start);
        transform->SetLocalRotationEulerDeg(pose.euler);
        const Quaternion previous = transform->GetLocalRotation();
        const Matrix4 oldView = camera->GetViewMatrix();
        const Vector3 direction = (key == 'W' || key == 'S') ? pose.forward : pose.right;
        const float sign = (key == 'S' || key == 'A') ? -1.0f : 1.0f;
        const Vector3 expected = start + direction * (sign * 0.25f);
        Input::ProcessInputMessage(key, false, true);

        actor.Tick(0.25f);

        ExpectVectorNear(transform->GetLocalPosition(), expected);
        ExpectRotationEqual(transform->GetLocalRotation(), previous);
        const Matrix4 view = camera->GetViewMatrix();
        ExpectVectorNear(view.TransformPosition(expected), Vector3::zero);
        ExpectVectorNear(view.TransformVector(pose.forward), Vector3::unitZ);
        ExpectVectorNear(view.TransformVector(pose.right), Vector3::unitX);
        ExpectVectorNear(oldView.TransformPosition(start), Vector3::zero);
    }
}

TEST_F(CameraMovementQuaternionTest, DirectQuaternionAndScaleDoNotChangeMovementSpeed)
{
    for (const Vector3& scale : {Vector3::one, Vector3(2, 3, 4),
        Vector3(-2, 3, -4), Vector3::zero})
    for (int key : {'W', 'S', 'A', 'D'})
    {
        SCOPED_TRACE(::testing::Message() << scale.x << ',' << scale.y << ',' << scale.z << "; key=" << key);
        ClearInput();
        CameraActor actor;
        auto* transform = actor.GetRootComponent();
        transform->SetLocalScale(scale);
        ASSERT_TRUE(transform->TrySetLocalRotation(Quaternion(1, 1, 1, 1)));
        Input::ProcessInputMessage(key, false, true);

        actor.Tick(0.5f);

        const Vector3 direction = (key == 'W' || key == 'S') ? Vector3::unitY : Vector3::unitZ;
        const float sign = (key == 'S' || key == 'A') ? -1.0f : 1.0f;
        ExpectVectorNear(transform->GetLocalPosition(), direction * (sign * 0.5f));
        EXPECT_NEAR(transform->GetLocalPosition().Length(), 0.5f, 1e-6f);
        ExpectRotationEqual(transform->GetLocalRotation(), Quaternion(0.5f, 0.5f, 0.5f, 0.5f));
    }
}

TEST_F(CameraMovementQuaternionTest, DeltaTimeScalesDistanceAcrossFrames)
{
    CameraActor wholeFrame;
    CameraActor splitFrames;
    for (CameraActor* actor : {&wholeFrame, &splitFrames})
    {
        actor->SetPosition(Vector3(3, -2, 5));
        ASSERT_TRUE(actor->GetRootComponent()->TrySetLocalRotation(Quaternion(1, 1, 1, 1)));
    }
    Input::ProcessInputMessage('D', false, true);
    wholeFrame.Tick(0.0f);
    ExpectVectorNear(wholeFrame.GetPosition(), Vector3(3, -2, 5));
    wholeFrame.Tick(0.5f);
    for (int frame = 0; frame < 4; ++frame)
        splitFrames.Tick(0.125f);
    ExpectVectorNear(wholeFrame.GetPosition(), Vector3(3, -2, 5.5f));
    ExpectVectorNear(splitFrames.GetPosition(), wholeFrame.GetPosition());
}

TEST_F(CameraMovementQuaternionTest, NoKeysAndOpposingKeysPreservePositionAndRotation)
{
    CameraActor actor;
    actor.SetPosition(Vector3(3, -2, 5));
    actor.SetRotation(Vector3(20, 40, 30));
    const Quaternion previous = actor.GetRootComponent()->GetLocalRotation();
    actor.Tick(0.25f);
    ExpectVectorNear(actor.GetPosition(), Vector3(3, -2, 5));
    for (int key : {'W', 'S', 'A', 'D'})
        Input::ProcessInputMessage(key, false, true);
    actor.Tick(0.25f);
    ExpectVectorNear(actor.GetPosition(), Vector3(3, -2, 5));
    ExpectRotationEqual(actor.GetRootComponent()->GetLocalRotation(), previous);
}

TEST_F(CameraMovementQuaternionTest, LookRequiresButtonAndZeroDeltaPreservesPolePose)
{
    CameraActor actor;
    actor.SetRotation(Vector3(90, 25, 40));
    const auto previous = actor.GetRootComponent()->GetLocalRotation();
    Input::SetMousePosition(Vector2(50, 70));
    actor.Tick(0.25f);
    ExpectRotationEqual(actor.GetRootComponent()->GetLocalRotation(), previous);
    SetLookDelta(0, 0);
    actor.Tick(0.25f);
    ExpectRotationEqual(actor.GetRootComponent()->GetLocalRotation(), previous);
}

TEST_F(CameraMovementQuaternionTest, LookChangesEulerPitchAndYawWithRollAndRefreshesView)
{
    for (float roll : {0.0f, 30.0f, 90.0f, -90.0f})
    {
        SCOPED_TRACE(roll);
        CameraActor actor;
        actor.SetPosition(Vector3(3, -2, 5));
        actor.SetRotation(Vector3(20, 40, roll));
        auto* transform = actor.GetRootComponent();
        transform->SetLocalScale(Vector3(-2, 0, 3));
        actor.GetComponent<CameraComponent>()->GetViewMatrix();
        SetLookDelta(100, 50);
        actor.Tick(0.25f);
        ExpectLookPose(actor, 25, 50, roll);
        ExpectVectorNear(transform->GetLocalPosition(), Vector3(3, -2, 5));
        ExpectVectorNear(transform->GetLocalScale(), Vector3(-2, 0, 3));
    }
}

TEST_F(CameraMovementQuaternionTest, LookClampsBothPitchLimitsAndCanImmediatelyTurnBack)
{
    for (float sign : {-1.0f, 1.0f})
    {
        SCOPED_TRACE(sign);
        CameraActor actor;
        actor.SetRotation(Vector3(sign * 88, 35, 30));
        SetLookDelta(0, sign * 200);
        for (int frame = 0; frame < 4; ++frame)
        {
            actor.Tick(0.016f);
            ExpectLookPose(actor, sign * 89, 35, 30, 5e-5f);
        }
        SetLookDelta(0, -sign * 100);
        actor.Tick(0.016f);
        ExpectLookPose(actor, sign * 79, 35, 30, 5e-5f);
    }
}

TEST_F(CameraMovementQuaternionTest, LookWrapsYawAcrossBoundariesAndMultipleTurns)
{
    struct Sample { float initial, mouse, expected; };
    const Sample samples[] = {{179, 20, -179}, {-179, -20, 179},
        {0, 1800, -180}, {0, -1800, -180}, {0, 10810, 1}, {0, -10810, -1}};
    for (const auto& sample : samples)
    {
        SCOPED_TRACE(sample.mouse);
        CameraActor actor;
        actor.SetRotation(Vector3(20, sample.initial, 40));
        SetLookDelta(sample.mouse, 0);
        actor.Tick(0.016f);
        ExpectLookPose(actor, 20, sample.expected, 40);
    }
}

TEST_F(CameraMovementQuaternionTest, LookUsesDisplacementWithoutDeltaTimeScaling)
{
    for (float deltaTime : {0.0f, 0.016f, 0.5f})
    {
        SCOPED_TRACE(deltaTime);
        CameraActor actor;
        SetLookDelta(200, 100);
        actor.Tick(deltaTime);
        ExpectLookPose(actor, 10, 20, 0);
    }
}

TEST_F(CameraMovementQuaternionTest, LookStartsFromExternallyChangedQuaternionAndClampsOnlyOnInput)
{
    CameraActor actor;
    SetLookDelta(100, 100);
    actor.Tick(0.016f);
    Quaternion external;
    ASSERT_TRUE(Quaternion::TryFromEuler(30 * PI / 180, 20 * PI / 180, 40 * PI / 180, external));
    ASSERT_TRUE(actor.GetRootComponent()->TrySetLocalRotation(external));
    SetLookDelta(-50, 50);
    actor.Tick(0.016f);
    ExpectLookPose(actor, 25, 35, 30);
    for (float sign : {-1.0f, 1.0f})
    {
        actor.SetRotation(Vector3(sign * 90, 0, 0));
        const auto previous = actor.GetRootComponent()->GetLocalRotation();
        SetLookDelta(0, 0);
        actor.Tick(0.016f);
        ExpectRotationEqual(actor.GetRootComponent()->GetLocalRotation(), previous);
        SetLookDelta(10, 0);
        actor.Tick(0.016f);
        ExpectLookPose(actor, sign * 89, 1, 0);
    }
}

TEST_F(CameraMovementQuaternionTest, InvalidMouseInputPreservesPoseAndRecovers)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const Vector2 invalid[] = {Vector2(nan, 10), Vector2(10, nan),
        Vector2(inf, 10), Vector2(10, inf), Vector2(-inf, 10), Vector2(10, -inf)};
    CameraActor actor;
    actor.SetRotation(Vector3(20, 40, 30));
    const auto previous = actor.GetRootComponent()->GetLocalRotation();
    for (const auto& mouse : invalid)
    {
        SetLookDelta(mouse.x, mouse.y);
        actor.Tick(0.016f);
        ExpectRotationEqual(actor.GetRootComponent()->GetLocalRotation(), previous);
        ExpectLookPose(actor, 20, 40, 30);
    }
    const float largest = (std::numeric_limits<float>::max)();
    Input::SetMousePosition(Vector2(-largest, 0));
    Input::SetPreviousMousePosition();
    Input::SetMousePosition(Vector2(largest, 0));
    actor.Tick(0.016f);
    ExpectRotationEqual(actor.GetRootComponent()->GetLocalRotation(), previous);
    SetLookDelta(10, 10);
    actor.Tick(0.016f);
    ExpectLookPose(actor, 21, 41, 30);
}

TEST_F(CameraMovementQuaternionTest, RepeatedLookRetainsRollAndNormalizedPose)
{
    CameraActor actor;
    actor.SetRotation(Vector3(0, 0, 30));
    SetLookDelta(10, 0.5f);
    for (int frame = 0; frame < 720; ++frame)
        actor.Tick(0.016f);
    ExpectLookPose(actor, 36, 0, 30, 2e-4f);
}

TEST_F(CameraMovementQuaternionTest, MovementUsesPoseBeforeMouseRotationInSameTick)
{
    CameraActor actor;
    Input::ProcessInputMessage('W', false, true);
    SetLookDelta(900, 0);
    actor.Tick(0.5f);
    ExpectVectorNear(actor.GetPosition(), Vector3(0.5f, 0, 0));
    ExpectLookPose(actor, 0, 90, 0);
    SetLookDelta(0, 0);
    actor.Tick(0.5f);
    ExpectVectorNear(actor.GetPosition(), Vector3(0.5f, 0.5f, 0));
    ExpectLookPose(actor, 0, 90, 0);
}
