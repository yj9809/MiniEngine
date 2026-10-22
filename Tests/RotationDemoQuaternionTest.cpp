#include <gtest/gtest.h>
#include <initializer_list>
#include <limits>

#include "../Game/Actor/TestMeshActor.h"
#include "Component/Transform/TransformComponent.h"
#include "Core/Input.h"

namespace
{
    using namespace Engine;

    void ClearDemoInput()
    {
        for (int key : {'0', '1', '2', '3', '4', '9'})
            Input::ProcessInputMessage(key, true, false);
        Input::ResetKeyState();
    }

    void ExpectVectorNear(const Vector3& actual, const Vector3& expected)
    {
        EXPECT_NEAR(actual.x, expected.x, 1e-5f);
        EXPECT_NEAR(actual.y, expected.y, 1e-5f);
        EXPECT_NEAR(actual.z, expected.z, 1e-5f);
    }

    void ExpectSamePose(const Matrix4& actual, const Matrix4& expected)
    {
        for (const auto& axis : {Vector3::unitX, Vector3::unitY, Vector3::unitZ})
            ExpectVectorNear(actual.TransformVector(axis), expected.TransformVector(axis));
    }

    void PressDemoKeys(TestMeshActor& actor, std::initializer_list<int> keys, float dt = 0.0f)
    {
        ClearDemoInput();
        for (int key : keys)
            Input::ProcessInputMessage(key, false, true);
        actor.Tick(dt);
        ClearDemoInput();
    }

    void ExpectBasis(TestMeshActor& actor, const Vector3& forward, const Vector3& right, const Vector3& up)
    {
        const auto* transform = actor.GetRootComponent();
        ExpectVectorNear(transform->GetForward(), forward);
        ExpectVectorNear(transform->GetRight(), right);
        ExpectVectorNear(transform->GetUp(), up);
    }

    void ExpectOrthonormal(TestMeshActor& actor)
    {
        const auto* transform = actor.GetRootComponent();
        const auto q = transform->GetLocalRotation();
        EXPECT_NEAR(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w, 1.0f, 1e-6f);
        const Vector3 f = transform->GetForward(), r = transform->GetRight(), u = transform->GetUp();
        EXPECT_NEAR(f.Length(), 1.0f, 1e-6f);
        EXPECT_NEAR(r.Length(), 1.0f, 1e-6f);
        EXPECT_NEAR(u.Length(), 1.0f, 1e-6f);
        EXPECT_NEAR(f.Dot(r), 0.0f, 1e-6f);
        EXPECT_NEAR(r.Dot(u), 0.0f, 1e-6f);
        EXPECT_NEAR(u.Dot(f), 0.0f, 1e-6f);
        ExpectVectorNear(f.Cross(r), u);
    }

    class RotationDemoQuaternionTest : public ::testing::Test
    {
    protected:
        void SetUp() override { ClearDemoInput(); }
        void TearDown() override { ClearDemoInput(); }
    };
}

TEST_F(RotationDemoQuaternionTest, StartupDispatchPreservesInitialOrientationAndScale)
{
    for (const auto& pose : {Vector3::zero, Vector3(90, 0, 0), Vector3(-90, 35, 25),
        Vector3(20, 40, 30), Vector3(100, 200, -220)})
    {
        SCOPED_TRACE(::testing::Message() << pose.x << ',' << pose.y << ',' << pose.z);
        TestMeshActor actor;
        actor.SetPosition(Vector3(7, 8, 9));
        actor.SetScale(Vector3(-2, 0, 3));
        actor.SetRotation(pose);
        auto* transform = actor.GetRootComponent();
        const Matrix4 before = Matrix4::Rotation(transform->GetLocalRotation());
        // No Level: production Initialize takes its no-resource path, while
        // DispatchBeginPlay still calls the real demo reset implementation.
        actor.DispatchInitialize();
        actor.DispatchBeginPlay();
        ExpectSamePose(Matrix4::Rotation(transform->GetLocalRotation()), before);
        ExpectVectorNear(actor.GetPosition(), Vector3(2, 0, 0));
        ExpectVectorNear(actor.GetScale(), Vector3(-2, 0, 3));
        actor.Tick(0.25f);
        ExpectSamePose(Matrix4::Rotation(transform->GetLocalRotation()), before);
        actor.DispatchOnDestroy();
    }
}

TEST_F(RotationDemoQuaternionTest, StartupAcceptsDirectQuaternionAndBothEquivalentSigns)
{
    for (const auto& input : {Quaternion(1, 1, 1, 1), Quaternion(-3, -3, -3, -3),
        Quaternion(0, 1, 0, 1), Quaternion(0, -1, 0, 1)})
    {
        TestMeshActor actor;
        auto* transform = actor.GetRootComponent();
        ASSERT_TRUE(transform->TrySetLocalRotation(input));
        const Matrix4 before = Matrix4::Rotation(transform->GetLocalRotation());
        actor.DispatchInitialize();
        actor.DispatchBeginPlay();
        ExpectSamePose(Matrix4::Rotation(transform->GetLocalRotation()), before);
        const auto q = transform->GetLocalRotation();
        EXPECT_NEAR(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w, 1.0f, 1e-6f);
        actor.DispatchOnDestroy();
    }
}

TEST_F(RotationDemoQuaternionTest, BeginPlayHookResetClearsPreviouslyEnabledRotationToggles)
{
    for (int key : {'1', '2', '3'})
    {
        SCOPED_TRACE(key);
        ClearDemoInput();
        TestMeshActor actor;
        Input::ProcessInputMessage(key, false, true);
        actor.Tick(0.25f);
        ClearDemoInput();
        // Isolated public-hook test: deliberately seed toggle state before
        // BeginPlay. This is not a simulation of normal engine lifecycle order.
        actor.SetRotation(Vector3(20, 40, 30));
        const Matrix4 before = Matrix4::Rotation(actor.GetRootComponent()->GetLocalRotation());
        actor.BeginPlay();
        actor.Tick(0.25f);
        ExpectSamePose(Matrix4::Rotation(actor.GetRootComponent()->GetLocalRotation()), before);
    }
}

TEST_F(RotationDemoQuaternionTest, EulerPoleYawAndRollShareOppositeAxisAndEqualInputsCancel)
{
    TestMeshActor yaw, roll, combined;
    for (auto* actor : {&yaw, &roll, &combined})
        PressDemoKeys(*actor, {'9'});
    PressDemoKeys(yaw, {'2'}, 1.0f);
    ExpectBasis(yaw, Vector3::unitZ, Vector3(-1, 0, 0), Vector3(0, -1, 0));
    PressDemoKeys(roll, {'3'}, 1.0f);
    ExpectBasis(roll, Vector3::unitZ, Vector3::unitX, Vector3::unitY);
    PressDemoKeys(combined, {'2', '3'});
    for (int frame = 0; frame < 600; ++frame)
    {
        combined.Tick(1.0f / 120.0f);
        ExpectBasis(combined, Vector3::unitZ, Vector3::unitY, Vector3(-1, 0, 0));
    }
}

TEST_F(RotationDemoQuaternionTest, QuaternionPoleLocalYawAndRollRemainIndependent)
{
    TestMeshActor yaw, roll, combined;
    for (auto* actor : {&yaw, &roll, &combined})
        PressDemoKeys(*actor, {'4'});
    PressDemoKeys(yaw, {'2'}, 1.0f);
    ExpectBasis(yaw, Vector3::unitY, Vector3(0, 0, -1), Vector3(-1, 0, 0));
    PressDemoKeys(roll, {'3'}, 1.0f);
    ExpectBasis(roll, Vector3::unitZ, Vector3::unitX, Vector3::unitY);
    PressDemoKeys(combined, {'2', '3'});
    for (int frame = 0; frame < 120; ++frame)
        combined.Tick(1.0f / 120.0f);
    EXPECT_LT(combined.GetRootComponent()->GetForward().Dot(Vector3::unitZ), 0.9f);
    ExpectOrthonormal(combined);
}

TEST_F(RotationDemoQuaternionTest, BothModesCrossPolesAndCompleteThreeSingleAxisTurns)
{
    for (bool quaternionMode : {false, true})
    for (int key : {'1', '2', '3'})
    {
        SCOPED_TRACE(::testing::Message() << quaternionMode << ',' << key);
        TestMeshActor actor;
        if (quaternionMode)
            PressDemoKeys(actor, {'4'});
        PressDemoKeys(actor, {'0'});
        PressDemoKeys(actor, {key});
        for (int frame = 1; frame <= 720; ++frame)
        {
            actor.Tick(1.0f / 60.0f);
            if (frame == 60)
            {
                if (key == '1') ExpectBasis(actor, Vector3::unitZ, Vector3::unitY, Vector3(-1, 0, 0));
                if (key == '2') ExpectBasis(actor, Vector3::unitY, Vector3(-1, 0, 0), Vector3::unitZ);
                if (key == '3') ExpectBasis(actor, Vector3::unitX, Vector3(0, 0, -1), Vector3::unitY);
            }
        }
        const auto* transform = actor.GetRootComponent();
        const Vector3 actual[] = {transform->GetForward(), transform->GetRight(), transform->GetUp()};
        const Vector3 expected[] = {Vector3::unitX, Vector3::unitY, Vector3::unitZ};
        for (int axis = 0; axis < 3; ++axis)
        {
            EXPECT_NEAR(actual[axis].x, expected[axis].x, 1e-4f);
            EXPECT_NEAR(actual[axis].y, expected[axis].y, 1e-4f);
            EXPECT_NEAR(actual[axis].z, expected[axis].z, 1e-4f);
        }
        ExpectOrthonormal(actor);
    }
}

TEST_F(RotationDemoQuaternionTest, TenThousandCombinedStepsPreserveNormalizedBasisPositionAndScale)
{
    TestMeshActor actor;
    PressDemoKeys(actor, {'4'});
    PressDemoKeys(actor, {'0'});
    actor.SetPosition(Vector3(3, -2, 5));
    actor.SetScale(Vector3(-2, 0, 3));
    PressDemoKeys(actor, {'2', '3'});
    for (int frame = 0; frame < 10000; ++frame)
    {
        actor.Tick(1.0f / 120.0f);
        if (frame % 100 == 0)
            ExpectOrthonormal(actor);
    }
    ExpectOrthonormal(actor);
    ExpectVectorNear(actor.GetPosition(), Vector3(3, -2, 5));
    ExpectVectorNear(actor.GetScale(), Vector3(-2, 0, 3));
}

TEST_F(RotationDemoQuaternionTest, ResetKeysClearTogglesKeepModeAndSkipRotationOnResetFrame)
{
    TestMeshActor actor;
    actor.SetPosition(Vector3(3, -2, 5));
    actor.SetScale(Vector3(2, 3, 4));
    PressDemoKeys(actor, {'1', '2', '3'}, 0.25f);
    PressDemoKeys(actor, {'4'}, 1.0f);
    actor.Tick(1.0f);
    ExpectBasis(actor, Vector3::unitZ, Vector3::unitY, Vector3(-1, 0, 0));
    PressDemoKeys(actor, {'2'}, 1.0f);
    ExpectBasis(actor, Vector3::unitY, Vector3(0, 0, -1), Vector3(-1, 0, 0));
    PressDemoKeys(actor, {'0'}, 1.0f);
    actor.Tick(1.0f);
    ExpectBasis(actor, Vector3::unitX, Vector3::unitY, Vector3::unitZ);
    PressDemoKeys(actor, {'9'}, 1.0f);
    PressDemoKeys(actor, {'2'}, 1.0f);
    ExpectBasis(actor, Vector3::unitY, Vector3(0, 0, -1), Vector3(-1, 0, 0));
    PressDemoKeys(actor, {'4'}, 1.0f);
    PressDemoKeys(actor, {'2', '3'}, 1.0f);
    ExpectBasis(actor, Vector3::unitZ, Vector3::unitY, Vector3(-1, 0, 0));
    ExpectVectorNear(actor.GetPosition(), Vector3(3, -2, 5));
    ExpectVectorNear(actor.GetScale(), Vector3(2, 3, 4));
}

TEST_F(RotationDemoQuaternionTest, InvalidFrameTimesDoNotAdvancePoseOrEulerAccumulation)
{
    for (bool quaternionMode : {false, true})
    {
        TestMeshActor actor;
        if (quaternionMode) PressDemoKeys(actor, {'4'});
        PressDemoKeys(actor, {'0'});
        PressDemoKeys(actor, {'1', '2'});
        const auto before = actor.GetRootComponent()->GetLocalRotation();
        for (float dt : {0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::infinity(), (std::numeric_limits<float>::max)()})
        {
            actor.Tick(dt);
            const auto after = actor.GetRootComponent()->GetLocalRotation();
            EXPECT_FLOAT_EQ(after.x, before.x);
            EXPECT_FLOAT_EQ(after.y, before.y);
            EXPECT_FLOAT_EQ(after.z, before.z);
            EXPECT_FLOAT_EQ(after.w, before.w);
        }
        actor.Tick(0.5f);
        constexpr float s = 0.70710678118f;
        ExpectBasis(actor, Vector3(0.5f, 0.5f, s), Vector3(-s, s, 0), Vector3(-0.5f, -0.5f, s));
    }
}

TEST_F(RotationDemoQuaternionTest, IdleFramesAndTurningOffLastAxisDoNotWriteRotation)
{
    TestMeshActor actor;
    PressDemoKeys(actor, {'9'});
    int notifications = 0;
    actor.GetRootComponent()->onTransformChanged = [&]() { ++notifications; };
    actor.Tick(0.5f);
    EXPECT_EQ(notifications, 0);
    PressDemoKeys(actor, {'2'}, 0.5f);
    EXPECT_EQ(notifications, 1);
    const auto before = Matrix4::Rotation(actor.GetRootComponent()->GetLocalRotation());
    PressDemoKeys(actor, {'2'}, 0.5f);
    actor.Tick(0.5f);
    EXPECT_EQ(notifications, 1);
    ExpectSamePose(Matrix4::Rotation(actor.GetRootComponent()->GetLocalRotation()), before);
}
