#include <gtest/gtest.h>
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

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
// RotationX(90°): Y → -Z
TEST(Matrix4Test, RotationX_Y_to_NegZ)
{
    Matrix4 r = Matrix4::RotationX(3.141592f * 0.5f);
    Vector3 y(0.0f, 1.0f, 0.0f);
    ExpectVec3Near(r.TransformVector(y), Vector3(0.0f, 0.0f, -1.0f));
}

// RotationX(90°): Z → +Y
TEST(Matrix4Test, RotationX_Z_to_PosY)
{
    Matrix4 r = Matrix4::RotationX(3.141592f * 0.5f);
    Vector3 z(0.0f, 0.0f, 1.0f);
    ExpectVec3Near(r.TransformVector(z), Vector3(0.0f, 1.0f, 0.0f));
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
// Transpose
// -----------------------------------------------------------------------
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

    Matrix4 proj = Matrix4::PerspectiveFOV(fovY, 1920.0f, 1080.0f, nearZ, farZ);

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

    Matrix4 proj = Matrix4::PerspectiveFOV(fovY, 1920.0f, 1080.0f, nearZ, farZ);

    Vector4 v(0.0f, 0.0f, farZ, 1.0f);
    Vector4 clip = proj * v;
    float ndcZ = clip.z / clip.w;
    EXPECT_NEAR(ndcZ, 1.0f, kEps);
}
