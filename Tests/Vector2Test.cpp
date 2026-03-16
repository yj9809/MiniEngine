#include <gtest/gtest.h>
#include "Math/Vector2.h"

using namespace Engine;

TEST(Vector2Test, Addition)
{
    Vector2 a(1.0f, 2.0f);
    Vector2 b(3.0f, 4.0f);
    Vector2 result = a + b;

    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
}

TEST(Vector2Test, Equality)
{
    Vector2 a(1.0f, 2.0f);
    Vector2 b(1.0f, 2.0f);
    EXPECT_EQ(a, b);
}
