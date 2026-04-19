#include <gtest/gtest.h>
#include "Math/Vector3.h"

#include <cmath>

using namespace Engine;

static constexpr float kEps = 1e-5f;

// -----------------------------------------------------------------------
// 산술 연산
// -----------------------------------------------------------------------
TEST(Vector3Test, Addition)
{
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    Vector3 r = a + b;
    EXPECT_FLOAT_EQ(r.x, 5.0f);
    EXPECT_FLOAT_EQ(r.y, 7.0f);
    EXPECT_FLOAT_EQ(r.z, 9.0f);
}

TEST(Vector3Test, Subtraction)
{
    Vector3 a(5.0f, 7.0f, 9.0f);
    Vector3 b(1.0f, 2.0f, 3.0f);
    Vector3 r = a - b;
    EXPECT_FLOAT_EQ(r.x, 4.0f);
    EXPECT_FLOAT_EQ(r.y, 5.0f);
    EXPECT_FLOAT_EQ(r.z, 6.0f);
}

TEST(Vector3Test, ScalarMultiply)
{
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 r = a * 3.0f;
    EXPECT_FLOAT_EQ(r.x, 3.0f);
    EXPECT_FLOAT_EQ(r.y, 6.0f);
    EXPECT_FLOAT_EQ(r.z, 9.0f);
}

TEST(Vector3Test, ScalarDivide)
{
    Vector3 a(3.0f, 6.0f, 9.0f);
    Vector3 r = a / 3.0f;
    EXPECT_FLOAT_EQ(r.x, 1.0f);
    EXPECT_FLOAT_EQ(r.y, 2.0f);
    EXPECT_FLOAT_EQ(r.z, 3.0f);
}

// -----------------------------------------------------------------------
// 비교
// -----------------------------------------------------------------------
TEST(Vector3Test, Equality)
{
    EXPECT_EQ(Vector3(1.0f, 2.0f, 3.0f), Vector3(1.0f, 2.0f, 3.0f));
}

TEST(Vector3Test, Inequality)
{
    EXPECT_NE(Vector3(1.0f, 2.0f, 3.0f), Vector3(1.0f, 2.0f, 4.0f));
}

// -----------------------------------------------------------------------
// 내적
// -----------------------------------------------------------------------
TEST(Vector3Test, Dot_Member)
{
    Vector3 a(1.0f, 0.0f, 0.0f);
    Vector3 b(0.0f, 1.0f, 0.0f);
    EXPECT_FLOAT_EQ(a.Dot(b), 0.0f);
}

TEST(Vector3Test, Dot_Parallel)
{
    Vector3 a(1.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(a.Dot(a), 1.0f);
}

TEST(Vector3Test, Dot_Static)
{
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    // 1*4 + 2*5 + 3*6 = 32
    EXPECT_FLOAT_EQ(Vector3::Dot(a, b), 32.0f);
}

// -----------------------------------------------------------------------
// 외적
// -----------------------------------------------------------------------
TEST(Vector3Test, Cross_Member_XY)
{
    // X x Y = Z
    Vector3 r = Vector3::unitX.Cross(Vector3::unitY);
    EXPECT_NEAR(r.x, 0.0f, kEps);
    EXPECT_NEAR(r.y, 0.0f, kEps);
    EXPECT_NEAR(r.z, 1.0f, kEps);
}

TEST(Vector3Test, Cross_Member_YX)
{
    // Y x X = -Z
    Vector3 r = Vector3::unitY.Cross(Vector3::unitX);
    EXPECT_NEAR(r.x,  0.0f, kEps);
    EXPECT_NEAR(r.y,  0.0f, kEps);
    EXPECT_NEAR(r.z, -1.0f, kEps);
}

TEST(Vector3Test, Cross_Static)
{
    Vector3 r = Vector3::Cross(Vector3::unitX, Vector3::unitY);
    EXPECT_NEAR(r.z, 1.0f, kEps);
}

TEST(Vector3Test, Cross_Parallel_IsZero)
{
    // 평행 벡터의 외적 = 영벡터
    Vector3 r = Vector3::unitX.Cross(Vector3::unitX);
    EXPECT_NEAR(r.x, 0.0f, kEps);
    EXPECT_NEAR(r.y, 0.0f, kEps);
    EXPECT_NEAR(r.z, 0.0f, kEps);
}

// -----------------------------------------------------------------------
// 길이
// -----------------------------------------------------------------------
TEST(Vector3Test, LengthSquared)
{
    Vector3 a(1.0f, 2.0f, 2.0f);
    EXPECT_FLOAT_EQ(a.LengthSquared(), 9.0f);
}

TEST(Vector3Test, Length)
{
    Vector3 a(1.0f, 2.0f, 2.0f);
    EXPECT_NEAR(a.Length(), 3.0f, kEps);
}

// -----------------------------------------------------------------------
// 정규화
// -----------------------------------------------------------------------
TEST(Vector3Test, Normalize_LengthIsOne)
{
    Vector3 a(3.0f, 1.0f, 2.0f);
    Vector3 n = a.Normalize();
    EXPECT_NEAR(n.Length(), 1.0f, kEps);
}

TEST(Vector3Test, Normalize_Direction)
{
    Vector3 a(2.0f, 0.0f, 0.0f);
    Vector3 n = a.Normalize();
    EXPECT_NEAR(n.x, 1.0f, kEps);
    EXPECT_NEAR(n.y, 0.0f, kEps);
    EXPECT_NEAR(n.z, 0.0f, kEps);
}

TEST(Vector3Test, Normalize_ZeroVector_ReturnsZero)
{
    Vector3 n = Vector3::zero.Normalize();
    EXPECT_FLOAT_EQ(n.x, 0.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);
    EXPECT_FLOAT_EQ(n.z, 0.0f);
}

// -----------------------------------------------------------------------
// 정적 상수
// -----------------------------------------------------------------------
TEST(Vector3Test, Constants)
{
    EXPECT_EQ(Vector3::zero,  Vector3(0, 0, 0));
    EXPECT_EQ(Vector3::one,   Vector3(1, 1, 1));
    EXPECT_EQ(Vector3::unitX, Vector3(1, 0, 0));
    EXPECT_EQ(Vector3::unitY, Vector3(0, 1, 0));
    EXPECT_EQ(Vector3::unitZ, Vector3(0, 0, 1));
}
