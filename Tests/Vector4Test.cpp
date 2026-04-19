#include <gtest/gtest.h>
#include "Math/Vector4.h"
#include "Math/Vector3.h"

#include <cmath>

using namespace Engine;

static constexpr float kEps = 1e-5f;

// -----------------------------------------------------------------------
// 생성자
// -----------------------------------------------------------------------
TEST(Vector4Test, ConstructFromXYZW)
{
    Vector4 v(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
    EXPECT_FLOAT_EQ(v.w, 4.0f);
}

TEST(Vector4Test, ConstructFromVector3)
{
    Vector3 v3(1.0f, 2.0f, 3.0f);
    Vector4 v4(v3, 1.0f);
    EXPECT_FLOAT_EQ(v4.x, 1.0f);
    EXPECT_FLOAT_EQ(v4.y, 2.0f);
    EXPECT_FLOAT_EQ(v4.z, 3.0f);
    EXPECT_FLOAT_EQ(v4.w, 1.0f);
}

// -----------------------------------------------------------------------
// 산술 연산
// -----------------------------------------------------------------------
TEST(Vector4Test, Addition)
{
    Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4 b(5.0f, 6.0f, 7.0f, 8.0f);
    Vector4 r = a + b;
    EXPECT_FLOAT_EQ(r.x, 6.0f);
    EXPECT_FLOAT_EQ(r.y, 8.0f);
    EXPECT_FLOAT_EQ(r.z, 10.0f);
    EXPECT_FLOAT_EQ(r.w, 12.0f);
}

TEST(Vector4Test, Subtraction)
{
    Vector4 a(5.0f, 6.0f, 7.0f, 8.0f);
    Vector4 b(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4 r = a - b;
    EXPECT_FLOAT_EQ(r.x, 4.0f);
    EXPECT_FLOAT_EQ(r.y, 4.0f);
    EXPECT_FLOAT_EQ(r.z, 4.0f);
    EXPECT_FLOAT_EQ(r.w, 4.0f);
}

TEST(Vector4Test, ScalarMultiply)
{
    Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4 r = a * 2.0f;
    EXPECT_FLOAT_EQ(r.x, 2.0f);
    EXPECT_FLOAT_EQ(r.y, 4.0f);
    EXPECT_FLOAT_EQ(r.z, 6.0f);
    EXPECT_FLOAT_EQ(r.w, 8.0f);
}

TEST(Vector4Test, ScalarDivide)
{
    Vector4 a(2.0f, 4.0f, 6.0f, 8.0f);
    Vector4 r = a / 2.0f;
    EXPECT_FLOAT_EQ(r.x, 1.0f);
    EXPECT_FLOAT_EQ(r.y, 2.0f);
    EXPECT_FLOAT_EQ(r.z, 3.0f);
    EXPECT_FLOAT_EQ(r.w, 4.0f);
}

// -----------------------------------------------------------------------
// 비교
// -----------------------------------------------------------------------
TEST(Vector4Test, Equality)
{
    EXPECT_EQ(Vector4(1.0f, 2.0f, 3.0f, 4.0f), Vector4(1.0f, 2.0f, 3.0f, 4.0f));
}

TEST(Vector4Test, Inequality)
{
    EXPECT_NE(Vector4(1.0f, 2.0f, 3.0f, 4.0f), Vector4(1.0f, 2.0f, 3.0f, 5.0f));
}

// -----------------------------------------------------------------------
// 내적
// -----------------------------------------------------------------------
TEST(Vector4Test, Dot_Perpendicular)
{
    Vector4 a(1.0f, 0.0f, 0.0f, 0.0f);
    Vector4 b(0.0f, 1.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(a.Dot(b), 0.0f);
}

TEST(Vector4Test, Dot_Value)
{
    Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4 b(5.0f, 6.0f, 7.0f, 8.0f);
    // 1*5 + 2*6 + 3*7 + 4*8 = 5+12+21+32 = 70
    EXPECT_FLOAT_EQ(a.Dot(b), 70.0f);
}

TEST(Vector4Test, Dot_Static)
{
    Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4 b(5.0f, 6.0f, 7.0f, 8.0f);
    EXPECT_FLOAT_EQ(Vector4::Dot(a, b), 70.0f);
}

// -----------------------------------------------------------------------
// 길이
// -----------------------------------------------------------------------
TEST(Vector4Test, LengthSquared)
{
    // 1^2 + 2^2 + 2^2 + 0^2 = 9
    Vector4 a(1.0f, 2.0f, 2.0f, 0.0f);
    EXPECT_FLOAT_EQ(a.LengthSquared(), 9.0f);
}

TEST(Vector4Test, Length)
{
    Vector4 a(1.0f, 2.0f, 2.0f, 0.0f);
    EXPECT_NEAR(a.Length(), 3.0f, kEps);
}

// -----------------------------------------------------------------------
// 정규화
// -----------------------------------------------------------------------
TEST(Vector4Test, Normalize_LengthIsOne)
{
    Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4 n = a.Normalize();
    EXPECT_NEAR(n.Length(), 1.0f, kEps);
}

TEST(Vector4Test, Normalize_Direction)
{
    Vector4 a(3.0f, 0.0f, 0.0f, 0.0f);
    Vector4 n = a.Normalize();
    EXPECT_NEAR(n.x, 1.0f, kEps);
    EXPECT_NEAR(n.y, 0.0f, kEps);
    EXPECT_NEAR(n.z, 0.0f, kEps);
    EXPECT_NEAR(n.w, 0.0f, kEps);
}

TEST(Vector4Test, Normalize_ZeroVector_ReturnsZero)
{
    Vector4 n = Vector4::zero.Normalize();
    EXPECT_FLOAT_EQ(n.x, 0.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);
    EXPECT_FLOAT_EQ(n.z, 0.0f);
    EXPECT_FLOAT_EQ(n.w, 0.0f);
}

// -----------------------------------------------------------------------
// ToVector3
// -----------------------------------------------------------------------
TEST(Vector4Test, ToVector3_DropsW)
{
    Vector4 v(1.0f, 2.0f, 3.0f, 99.0f);
    Vector3 r = v.ToVector3();
    EXPECT_FLOAT_EQ(r.x, 1.0f);
    EXPECT_FLOAT_EQ(r.y, 2.0f);
    EXPECT_FLOAT_EQ(r.z, 3.0f);
}

// -----------------------------------------------------------------------
// 정적 상수
// -----------------------------------------------------------------------
TEST(Vector4Test, Constants)
{
    EXPECT_EQ(Vector4::zero,  Vector4(0, 0, 0, 0));
    EXPECT_EQ(Vector4::one,   Vector4(1, 1, 1, 1));
    EXPECT_EQ(Vector4::unitX, Vector4(1, 0, 0, 0));
    EXPECT_EQ(Vector4::unitY, Vector4(0, 1, 0, 0));
    EXPECT_EQ(Vector4::unitZ, Vector4(0, 0, 1, 0));
}
