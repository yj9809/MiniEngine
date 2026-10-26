#include <gtest/gtest.h>
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Component/Transform/TransformComponent.h"

#include <cmath>

using namespace Engine;

static constexpr float kEps = 1e-4f;

// -----------------------------------------------------------------------
// 헬퍼
// -----------------------------------------------------------------------
static void ExpectVec3Near(const Vector3& a, const Vector3& b, float eps = kEps)
{
    EXPECT_NEAR(a.x, b.x, eps);
    EXPECT_NEAR(a.y, b.y, eps);
    EXPECT_NEAR(a.z, b.z, eps);
}

static void ExpectVec4Near(const Vector4& a, const Vector4& b, float eps = kEps)
{
    EXPECT_NEAR(a.x, b.x, eps);
    EXPECT_NEAR(a.y, b.y, eps);
    EXPECT_NEAR(a.z, b.z, eps);
    EXPECT_NEAR(a.w, b.w, eps);
}

// -----------------------------------------------------------------------
// Identity / Zero
// -----------------------------------------------------------------------
TEST(Matrix4Test, IdentityTransformPosition)
{
    Vector3 p(1.0f, 2.0f, 3.0f);
    Vector3 result = Matrix4::identity.TransformPosition(p);
    ExpectVec3Near(result, p);
}

TEST(Matrix4Test, ZeroTimesIdentityIsZero)
{
    Matrix4 result = Matrix4::zero * Matrix4::identity;
    Vector3 p(1.0f, 2.0f, 3.0f);
    Vector3 v = result.TransformPosition(p);
    ExpectVec3Near(v, Vector3(0, 0, 0));
}

// -----------------------------------------------------------------------
// Translation
// -----------------------------------------------------------------------
TEST(Matrix4Test, TranslationMovesPosition)
{
    Matrix4 t = Matrix4::Translation(10.0f, 20.0f, 30.0f);
    Vector3 p(1.0f, 2.0f, 3.0f);
    Vector3 result = t.TransformPosition(p);
    ExpectVec3Near(result, Vector3(11.0f, 22.0f, 33.0f));
}

TEST(Matrix4Test, TranslationDoesNotMoveVector)
{
    // w=0인 방향 벡터는 이동 변환에 영향 받지 않아야 함
    Matrix4 t = Matrix4::Translation(10.0f, 20.0f, 30.0f);
    Vector3 dir(1.0f, 0.0f, 0.0f);
    Vector3 result = t.TransformVector(dir);
    ExpectVec3Near(result, dir);
}

TEST(Matrix4Test, TranslationChain)
{
    Matrix4 t1 = Matrix4::Translation(1.0f, 0.0f, 0.0f);
    Matrix4 t2 = Matrix4::Translation(0.0f, 2.0f, 0.0f);
    Matrix4 t  = t1 * t2;
    Vector3 p(0.0f, 0.0f, 0.0f);
    ExpectVec3Near(t.TransformPosition(p), Vector3(1.0f, 2.0f, 0.0f));
}

// -----------------------------------------------------------------------
// Scale
// -----------------------------------------------------------------------
TEST(Matrix4Test, ScaleUniform)
{
    Matrix4 s = Matrix4::Scale(3.0f);
    Vector3 p(1.0f, 2.0f, 3.0f);
    ExpectVec3Near(s.TransformPosition(p), Vector3(3.0f, 6.0f, 9.0f));
}

TEST(Matrix4Test, ScaleNonUniform)
{
    Matrix4 s = Matrix4::Scale(2.0f, 3.0f, 4.0f);
    Vector3 p(1.0f, 1.0f, 1.0f);
    ExpectVec3Near(s.TransformPosition(p), Vector3(2.0f, 3.0f, 4.0f));
}

// -----------------------------------------------------------------------
// Rotation — v*M, Z-up LH 기준
// -----------------------------------------------------------------------
// RotationX(90°): Y → +Z
TEST(Matrix4Test, RotationX_Y_to_PosZ)
{
    Matrix4 r = Matrix4::RotationX(3.141592f * 0.5f);
    Vector3 y(0.0f, 1.0f, 0.0f);
    ExpectVec3Near(r.TransformVector(y), Vector3(0.0f, 0.0f, 1.0f));
}

// RotationX(90°): Z → -Y
TEST(Matrix4Test, RotationX_Z_to_NegY)
{
    Matrix4 r = Matrix4::RotationX(3.141592f * 0.5f);
    Vector3 z(0.0f, 0.0f, 1.0f);
    ExpectVec3Near(r.TransformVector(z), Vector3(0.0f, -1.0f, 0.0f));
}

// RotationY(90°): X → -Z
TEST(Matrix4Test, RotationY_X_to_NegZ)
{
    Matrix4 r = Matrix4::RotationY(3.141592f * 0.5f);
    Vector3 x(1.0f, 0.0f, 0.0f);
    ExpectVec3Near(r.TransformVector(x), Vector3(0.0f, 0.0f, -1.0f));
}

// RotationY(90°): Z → +X
TEST(Matrix4Test, RotationY_Z_to_PosX)
{
    Matrix4 r = Matrix4::RotationY(3.141592f * 0.5f);
    Vector3 z(0.0f, 0.0f, 1.0f);
    ExpectVec3Near(r.TransformVector(z), Vector3(1.0f, 0.0f, 0.0f));
}

// RotationZ(90°): X → +Y
TEST(Matrix4Test, RotationZ_X_to_PosY)
{
    Matrix4 r = Matrix4::RotationZ(3.141592f * 0.5f);
    Vector3 x(1.0f, 0.0f, 0.0f);
    ExpectVec3Near(r.TransformVector(x), Vector3(0.0f, 1.0f, 0.0f));
}

// RotationZ(90°): Y → -X
TEST(Matrix4Test, RotationZ_Y_to_NegX)
{
    Matrix4 r = Matrix4::RotationZ(3.141592f * 0.5f);
    Vector3 y(0.0f, 1.0f, 0.0f);
    ExpectVec3Near(r.TransformVector(y), Vector3(-1.0f, 0.0f, 0.0f));
}

// 360도 회전 → 원위치
TEST(Matrix4Test, RotationX_360_IsIdentity)
{
    Matrix4 r = Matrix4::RotationX(3.141592f * 2.0f);
    Vector3 p(1.0f, 2.0f, 3.0f);
    ExpectVec3Near(r.TransformVector(p), p);
}

// -----------------------------------------------------------------------
// Euler conventions and Transform integration
// -----------------------------------------------------------------------
// Euler inputs use Unreal-style signs; raw RotationX/Y/Z retain axis-angle signs.
TEST(Matrix4Test, EulerZeroPreservesVector)
{
    const Vector3 v(2.0f, -3.0f, 4.0f);
    ExpectVec3Near(Matrix4::Rotation(0.0f, 0.0f, 0.0f).TransformVector(v), v);
}

TEST(Matrix4Test, EulerRollUsesUnrealDirection)
{
    constexpr float halfPi = 1.5707963267948966f;
    for (float sign : {1.0f, -1.0f})
    {
        SCOPED_TRACE(sign);
        const Matrix4 r = Matrix4::Rotation(sign * halfPi, 0.0f, 0.0f);
        ExpectVec3Near(r.TransformVector(Vector3::unitX), Vector3::unitX);
        ExpectVec3Near(r.TransformVector(Vector3::unitY), Vector3(0.0f, 0.0f, -sign));
        ExpectVec3Near(r.TransformVector(Vector3::unitZ), Vector3(0.0f, sign, 0.0f));
    }
}

TEST(Matrix4Test, EulerPitchUsesUnrealDirection)
{
    constexpr float halfPi = 1.5707963267948966f;
    for (float sign : {1.0f, -1.0f})
    {
        SCOPED_TRACE(sign);
        const Matrix4 r = Matrix4::Rotation(0.0f, sign * halfPi, 0.0f);
        ExpectVec3Near(r.TransformVector(Vector3::unitX), Vector3(0.0f, 0.0f, sign));
        ExpectVec3Near(r.TransformVector(Vector3::unitY), Vector3::unitY);
        ExpectVec3Near(r.TransformVector(Vector3::unitZ), Vector3(-sign, 0.0f, 0.0f));
    }
}

TEST(Matrix4Test, EulerYawUsesUnrealDirection)
{
    constexpr float halfPi = 1.5707963267948966f;
    for (float sign : {1.0f, -1.0f})
    {
        SCOPED_TRACE(sign);
        const Matrix4 r = Matrix4::Rotation(0.0f, 0.0f, sign * halfPi);
        ExpectVec3Near(r.TransformVector(Vector3::unitX), Vector3(0.0f, sign, 0.0f));
        ExpectVec3Near(r.TransformVector(Vector3::unitY), Vector3(-sign, 0.0f, 0.0f));
        ExpectVec3Near(r.TransformVector(Vector3::unitZ), Vector3::unitZ);
    }
}

// Independent basis reference for Roll=30, Pitch=20, Yaw=40 degrees.
// Derived from intrinsic yaw/pitch/roll axes, not Matrix4's rotation factories:
// F=(cp*cy,cp*sy,sp), R=(sr*sp*cy-cr*sy,sr*sp*sy+cr*cy,-sr*cp),
// U=(-cr*sp*cy-sr*sy,-cr*sp*sy+sr*cy,cr*cp).
static void ExpectMixedEulerBasis(const Matrix4& rotation)
{
    ExpectVec3Near(rotation.TransformVector(Vector3::unitX),
        Vector3(0.71984631f, 0.60402277f, 0.34202014f));
    ExpectVec3Near(rotation.TransformVector(Vector3::unitY),
        Vector3(-0.42566908f, 0.77333710f, -0.46984631f));
    ExpectVec3Near(rotation.TransformVector(Vector3::unitZ),
        Vector3(-0.54829474f, 0.19262973f, 0.81379768f));
}

TEST(Matrix4Test, EulerMixedAnglesMatchIntrinsicYawPitchRoll)
{
    constexpr float toRadians = 0.017453292519943295f;
    ExpectMixedEulerBasis(Matrix4::Rotation(
        30.0f * toRadians, 20.0f * toRadians, 40.0f * toRadians));
}

TEST(Matrix4Test, EulerVectorStoresRollPitchYawRadians)
{
    constexpr float toRadians = 0.017453292519943295f;
    ExpectMixedEulerBasis(Matrix4::Rotation(Vector3(
        30.0f * toRadians, 20.0f * toRadians, 40.0f * toRadians)));
}

TEST(Matrix4QuaternionTest, IdentityPreservesHomogeneousCoordinates)
{
    const Matrix4 rotation = Matrix4::Rotation(Quaternion::identity);
    for (const Vector4& input : {Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0),
        Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1), Vector4(2, -3, 4, 1)})
    {
        ExpectVec4Near(rotation * input, input, 1e-6f);
    }
}

TEST(Matrix4QuaternionTest, AxisQuarterTurnsFollowRowVectorConvention)
{
    const float half = std::sqrt(0.5f);
    for (float sign : {1.0f, -1.0f})
    {
        SCOPED_TRACE(sign);
        // Raw quaternion axes have the same signs as RotationX/Y/Z.
        const Matrix4 x = Matrix4::Rotation(Quaternion(sign * half, 0, 0, half));
        ExpectVec3Near(x.TransformVector(Vector3::unitX), Vector3::unitX, 1e-6f);
        ExpectVec3Near(x.TransformVector(Vector3::unitY), Vector3(0, 0, sign), 1e-6f);
        ExpectVec3Near(x.TransformVector(Vector3::unitZ), Vector3(0, -sign, 0), 1e-6f);

        const Matrix4 y = Matrix4::Rotation(Quaternion(0, sign * half, 0, half));
        ExpectVec3Near(y.TransformVector(Vector3::unitX), Vector3(0, 0, -sign), 1e-6f);
        ExpectVec3Near(y.TransformVector(Vector3::unitY), Vector3::unitY, 1e-6f);
        ExpectVec3Near(y.TransformVector(Vector3::unitZ), Vector3(sign, 0, 0), 1e-6f);

        const Matrix4 z = Matrix4::Rotation(Quaternion(0, 0, sign * half, half));
        ExpectVec3Near(z.TransformVector(Vector3::unitX), Vector3(0, sign, 0), 1e-6f);
        ExpectVec3Near(z.TransformVector(Vector3::unitY), Vector3(-sign, 0, 0), 1e-6f);
        ExpectVec3Near(z.TransformVector(Vector3::unitZ), Vector3::unitZ, 1e-6f);
    }
}

TEST(Matrix4QuaternionTest, MixedEulerMatchesIndependentBasis)
{
    constexpr float toRadians = 0.017453292519943295f;
    Quaternion input;
    ASSERT_TRUE(Quaternion::TryFromEuler(30 * toRadians, 20 * toRadians, 40 * toRadians, input));
    ExpectMixedEulerBasis(Matrix4::Rotation(input));
}

TEST(Matrix4QuaternionTest, MatchesQuaternionVectorRotationAcrossEulerSamples)
{
    constexpr float toRadians = 0.017453292519943295f;
    for (float roll : {-180.0f, -30.0f, 0.0f, 45.0f})
    for (float pitch : {-90.0f, -89.99f, -20.0f, 0.0f, 20.0f, 89.99f, 90.0f})
    for (float yaw : {-170.0f, 0.0f, 40.0f, 180.0f})
    {
        SCOPED_TRACE(::testing::Message() << roll << ',' << pitch << ',' << yaw);
        Quaternion input;
        ASSERT_TRUE(Quaternion::TryFromEuler(roll * toRadians, pitch * toRadians, yaw * toRadians, input));
        const Matrix4 rotation = Matrix4::Rotation(input);
        for (const Vector3& axis : {Vector3::unitX, Vector3::unitY, Vector3::unitZ, Vector3(2, -3, 4)})
        {
            Vector3 expected;
            ASSERT_TRUE(input.TryRotateVector(axis, expected));
            ExpectVec3Near(rotation.TransformVector(axis), expected, 3e-6f);
        }
    }
}

TEST(Matrix4QuaternionTest, OppositeQuaternionSignsProduceSameMatrix)
{
    for (const Quaternion& input : {Quaternion::identity, Quaternion(0.5f, 0.5f, 0.5f, 0.5f),
        Quaternion(0, 1, 0, 0)})
    {
        const Matrix4 positive = Matrix4::Rotation(input);
        const Matrix4 negative = Matrix4::Rotation(Quaternion(-input.x, -input.y, -input.z, -input.w));
        for (const Vector4& basis : {Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0),
            Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1)})
        {
            ExpectVec4Near(positive * basis, negative * basis, 1e-6f);
        }
    }
}

TEST(Matrix4QuaternionTest, PureRotationPreservesOriginAndHomogeneousW)
{
    // This unit quaternion maps X -> Y, Y -> Z, Z -> X.
    const Matrix4 rotation = Matrix4::Rotation(Quaternion(0.5f, 0.5f, 0.5f, 0.5f));
    ExpectVec4Near(rotation * Vector4(0, 0, 0, 1), Vector4(0, 0, 0, 1), 1e-6f);
    ExpectVec4Near(rotation * Vector4(2, -3, 4, 0), Vector4(4, 2, -3, 0), 1e-6f);
    ExpectVec4Near(rotation * Vector4(2, -3, 4, 1), Vector4(4, 2, -3, 1), 1e-6f);
}

TEST(TransformEulerTest, PitchYawRollDegreesPreserveWorldBasisAndForward)
{
    TransformComponent transform;
    ExpectVec3Near(transform.GetWorldMatrix().TransformPosition(Vector3::zero), Vector3::zero);

    // Transform storage is Pitch/Yaw/Roll degrees, unlike Matrix4's Roll/Pitch/Yaw radians.
    transform.SetLocalRotationEulerDeg(Vector3(20.0f, 40.0f, 30.0f));
    transform.SetLocalScale(Vector3(2.0f, 3.0f, 4.0f));
    transform.SetLocalPosition(Vector3(10.0f, -20.0f, 30.0f));
    const Matrix4& world = transform.GetWorldMatrix();

    ExpectVec3Near(world.TransformPosition(Vector3::zero), Vector3(10.0f, -20.0f, 30.0f));
    ExpectVec3Near(world.TransformPosition(Vector3::unitX), Vector3(11.43969262f, -18.79195446f, 30.68404028f));
    ExpectVec3Near(world.TransformPosition(Vector3::unitY), Vector3(8.72299276f, -17.67998870f, 28.59046107f));
    ExpectVec3Near(world.TransformPosition(Vector3::unitZ), Vector3(7.80682104f, -19.22948108f, 33.25519072f));
    ExpectVec3Near(transform.GetForward(), Vector3(0.71984631f, 0.60402277f, 0.34202014f));
}

// -----------------------------------------------------------------------
// Transpose
// -----------------------------------------------------------------------
// Four homogeneous basis vectors expose every matrix element via the public API.
static void ExpectMatrixNear(const Matrix4& actual, const Matrix4& expected)
{
    const Vector4 basis[] = {
        Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0),
        Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1)
    };
    for (const Vector4& v : basis)
    {
        ExpectVec4Near(actual * v, expected * v);
    }
}

TEST(Matrix4Test, TryInverseGeneralRestoresHomogeneousBasisAndPreservesSource)
{
    const Matrix4 inputs[] = {
        Matrix4::identity,
        Matrix4::Translation(10, -20, 30),
        Matrix4::Scale(2, -3, 4),
        Matrix4::Scale(2, 3, 4) * Matrix4::Rotation(0.3f, -0.6f, 0.8f) *
            Matrix4::Translation(10, -20, 30),
        Matrix4::PerspectiveFOV(70, 1.5f, 1, 100)
    };
    for (const Matrix4& input : inputs)
    {
        Matrix4 source = input;
        Matrix4 inverse = Matrix4::zero;
        ASSERT_TRUE(source.TryInverseGeneral(inverse));
        ExpectMatrixNear(source * inverse, Matrix4::identity);
        ExpectMatrixNear(inverse * source, Matrix4::identity);
        ExpectMatrixNear(source, input);
        ExpectMatrixNear(source.InverseGeneral(), inverse);
    }
}

TEST(Matrix4Test, TryInverseGeneralSingularInputReturnsFalseAndCopiesSource)
{
    const Matrix4 inputs[] = {
        Matrix4::zero,
        Matrix4::Scale(0, 2, 3) * Matrix4::Translation(5, 6, 7),
        Matrix4::Scale(2, 0, 3) * Matrix4::Translation(5, 6, 7),
        Matrix4::Scale(2, 3, 0) * Matrix4::Translation(5, 6, 7)
    };
    for (const Matrix4& input : inputs)
    {
        Matrix4 source = input;
        Matrix4 output = Matrix4::identity;
        EXPECT_FALSE(source.TryInverseGeneral(output));
        ExpectMatrixNear(output, input);
        ExpectMatrixNear(source, input);
        ExpectMatrixNear(source.InverseGeneral(), input);
    }
}

TEST(Matrix4Test, TryInverseGeneralSupportsOutputAliasingSource)
{
    const Matrix4 original = Matrix4::Scale(2, 3, 4) *
        Matrix4::Rotation(0.3f, -0.6f, 0.8f) * Matrix4::Translation(5, 6, 7);
    Matrix4 inPlace = original;
    ASSERT_TRUE(inPlace.TryInverseGeneral(inPlace));
    ExpectMatrixNear(original * inPlace, Matrix4::identity);
    ExpectMatrixNear(inPlace * original, Matrix4::identity);

    const Matrix4 singular = Matrix4::Scale(0, 2, 3) * Matrix4::Translation(5, 6, 7);
    inPlace = singular;
    EXPECT_FALSE(inPlace.TryInverseGeneral(inPlace));
    ExpectMatrixNear(inPlace, singular);
}

TEST(Matrix4Test, TransposeTwiceIsIdentity)
{
    Matrix4 t = Matrix4::Translation(1.0f, 2.0f, 3.0f);
    Matrix4 tt = t.Transpose().Transpose();
    Vector3 p(5.0f, 6.0f, 7.0f);
    ExpectVec3Near(t.TransformPosition(p), tt.TransformPosition(p));
}

// -----------------------------------------------------------------------
// 행렬 곱
// -----------------------------------------------------------------------
TEST(Matrix4Test, MultiplyByIdentity)
{
    Matrix4 t = Matrix4::Translation(1.0f, 2.0f, 3.0f);
    Matrix4 r = t * Matrix4::identity;
    Vector3 p(0.0f, 0.0f, 0.0f);
    ExpectVec3Near(r.TransformPosition(p), t.TransformPosition(p));
}

// Scale 뒤 Translation 적용 순서 검증 (v*M: 왼쪽이 먼저)
TEST(Matrix4Test, ScaleThenTranslate)
{
    Matrix4 s = Matrix4::Scale(2.0f);
    Matrix4 t = Matrix4::Translation(10.0f, 0.0f, 0.0f);
    // v * s * t : 먼저 scale, 그 다음 translate
    Matrix4 combined = s * t;
    Vector3 p(1.0f, 0.0f, 0.0f);
    // (1,0,0) * scale(2) = (2,0,0), * translate(10,0,0) = (12,0,0)
    ExpectVec3Near(combined.TransformPosition(p), Vector3(12.0f, 0.0f, 0.0f));
}

// -----------------------------------------------------------------------
// LookAt
// -----------------------------------------------------------------------
TEST(Matrix4Test, LookAt_ForwardBecomesZ)
{
    // 원점에서 +X를 바라볼 때, 월드 +X 방향이 카메라 +Z로 변환돼야 함
    Vector3 eye(0.0f, 0.0f, 0.0f);
    Vector3 at(1.0f, 0.0f, 0.0f);
    Vector3 up(0.0f, 0.0f, 1.0f);

    Matrix4 view = Matrix4::LookAt(eye, at, up);
    Vector3 worldForward(1.0f, 0.0f, 0.0f);
    Vector3 cameraSpace = view.TransformVector(worldForward);

    // forward가 카메라 +Z 방향으로 변환
    EXPECT_NEAR(cameraSpace.z, 1.0f, kEps);
}

TEST(Matrix4Test, LookAt_EyeTranslatesToOrigin)
{
    Vector3 eye(5.0f, 0.0f, 0.0f);
    Vector3 at(6.0f, 0.0f, 0.0f);
    Vector3 up(0.0f, 0.0f, 1.0f);

    Matrix4 view = Matrix4::LookAt(eye, at, up);
    // eye 위치 자체는 카메라 공간 원점으로
    Vector3 cameraEye = view.TransformPosition(eye);
    ExpectVec3Near(cameraEye, Vector3(0.0f, 0.0f, 0.0f));
}

// -----------------------------------------------------------------------
// PerspectiveFOV
// -----------------------------------------------------------------------
TEST(Matrix4Test, Perspective_NearMapsToZero)
{
    // nearZ 거리의 점은 NDC z = 0 이 돼야 함
    float fovY = 90.0f;
    float nearZ = 1.0f;
    float farZ  = 100.0f;

    Matrix4 proj = Matrix4::PerspectiveFOV(fovY, 1920.0f / 1080.0f, nearZ, farZ);

    // v*M: Vector4(0, 0, nearZ, 1) * proj
    Vector4 v(0.0f, 0.0f, nearZ, 1.0f);
    Vector4 clip = proj * v;
    float ndcZ = clip.z / clip.w;
    EXPECT_NEAR(ndcZ, 0.0f, kEps);
}

TEST(Matrix4Test, Perspective_FarMapsToOne)
{
    float fovY = 90.0f;
    float nearZ = 1.0f;
    float farZ  = 100.0f;

    Matrix4 proj = Matrix4::PerspectiveFOV(fovY, 1920.0f / 1080.0f, nearZ, farZ);

    Vector4 v(0.0f, 0.0f, farZ, 1.0f);
    Vector4 clip = proj * v;
    float ndcZ = clip.z / clip.w;
    EXPECT_NEAR(ndcZ, 1.0f, kEps);
}
