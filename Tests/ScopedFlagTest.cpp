#include <gtest/gtest.h>

#include <stdexcept>
#include <type_traits>

#include "Common/ScopedFlag.h"

static_assert(!std::is_copy_constructible_v<Engine::ScopedFlag>);
static_assert(!std::is_copy_assignable_v<Engine::ScopedFlag>);
static_assert(!std::is_move_constructible_v<Engine::ScopedFlag>);
static_assert(!std::is_move_assignable_v<Engine::ScopedFlag>);

TEST(ScopedFlagTest, SetsTrueAndRestoresFalse)
{
    bool flag = false;
    {
        Engine::ScopedFlag guard(flag);
        EXPECT_TRUE(flag);
    }
    EXPECT_FALSE(flag);
}

TEST(ScopedFlagTest, RestoresPreviousTrueValue)
{
    bool flag = true;
    {
        Engine::ScopedFlag guard(flag);
        EXPECT_TRUE(flag);
    }
    EXPECT_TRUE(flag);
}

TEST(ScopedFlagTest, NestedScopesPreserveOuterGuard)
{
    bool flag = false;
    {
        Engine::ScopedFlag outer(flag);
        {
            Engine::ScopedFlag inner(flag);
            EXPECT_TRUE(flag);
        }
        EXPECT_TRUE(flag);
    }
    EXPECT_FALSE(flag);
}

TEST(ScopedFlagTest, RestoresPreviousValueWhenExceptionEscapes)
{
    for (const bool previous : { false, true })
    {
        SCOPED_TRACE(previous);
        bool flag = previous;
        const auto throwInsideScope = [&]()
        {
            Engine::ScopedFlag guard(flag);
            EXPECT_TRUE(flag);
            throw std::runtime_error("test failure");
        };
        EXPECT_THROW(throwInsideScope(), std::runtime_error);
        EXPECT_EQ(flag, previous);
    }
}
