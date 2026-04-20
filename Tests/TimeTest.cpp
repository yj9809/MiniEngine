#include <gtest/gtest.h>
#include "Core/Time.h"

using namespace Engine;

static constexpr float kEps = 1e-5f;

class TimeTest : public ::testing::Test
{
protected:
    void SetUp() override { Time::Reset(); }
};

// -----------------------------------------------------------------------
// Update — 기본 동작
// -----------------------------------------------------------------------
TEST_F(TimeTest, Update_SingleFrame_DeltaEqualsInput)
{
    Time::Update(0.016f);
    EXPECT_NEAR(Time::GetDeltaTime(), 0.016f, kEps);
}

TEST_F(TimeTest, Update_SingleFrame_UnscaledEqualsInput)
{
    Time::Update(0.016f);
    EXPECT_NEAR(Time::GetUnscaledDeltaTime(), 0.016f, kEps);
}

// -----------------------------------------------------------------------
// Update — TimeScale
// -----------------------------------------------------------------------
TEST_F(TimeTest, Update_TimeScaleApplied)
{
    Time::SetTimeScale(2.0f);
    Time::Update(0.016f);
    EXPECT_NEAR(Time::GetDeltaTime(), 0.032f, kEps);
    EXPECT_NEAR(Time::GetUnscaledDeltaTime(), 0.016f, kEps);
}

TEST_F(TimeTest, Update_TimeScaleZero_DeltaIsZero)
{
    Time::SetTimeScale(0.0f);
    Time::Update(0.016f);
    EXPECT_NEAR(Time::GetDeltaTime(), 0.0f, kEps);
}

// -----------------------------------------------------------------------
// Update — 10프레임 이동 평균 스무딩
// -----------------------------------------------------------------------
TEST_F(TimeTest, Update_Smoothing_FullBuffer_SameValues)
{
    for (int i = 0; i < 10; i++)
        Time::Update(0.016f);
    EXPECT_NEAR(Time::GetUnscaledDeltaTime(), 0.016f, kEps);
}

TEST_F(TimeTest, Update_Smoothing_SpikeDampened)
{
    for (int i = 0; i < 9; i++)
        Time::Update(0.016f);

    Time::Update(1.0f);

    // 버퍼 평균: (9 * 0.016 + 1.0) / 10 = 0.1144
    const float expected = (9 * 0.016f + 1.0f) / 10.0f;
    EXPECT_NEAR(Time::GetUnscaledDeltaTime(), expected, kEps);
    EXPECT_LT(Time::GetUnscaledDeltaTime(), 1.0f);
}

TEST_F(TimeTest, Update_Smoothing_PartialBuffer)
{
    // 버퍼가 채워지기 전에는 실제 샘플 수로 나눠야 한다
    Time::Update(0.010f);
    Time::Update(0.020f);
    // average = (0.010 + 0.020) / 2
    EXPECT_NEAR(Time::GetUnscaledDeltaTime(), 0.015f, kEps);
}

// -----------------------------------------------------------------------
// Update — MaxDeltaTime 클램프
// -----------------------------------------------------------------------
TEST_F(TimeTest, Update_MaxDeltaTimeClamp_DeltaClamped)
{
    Time::SetMaxDeltaTime(0.033f);
    Time::Update(1.0f);
    EXPECT_NEAR(Time::GetUnscaledDeltaTime(), 0.033f, kEps);
}

TEST_F(TimeTest, Update_MaxDeltaTimeClamp_TotalTimeClamped)
{
    Time::SetMaxDeltaTime(0.033f);
    Time::Update(1.0f);
    EXPECT_NEAR(Time::GetTotalTime(), 0.033f, kEps);
}

// -----------------------------------------------------------------------
// Update — TotalTime 누적
// -----------------------------------------------------------------------
TEST_F(TimeTest, Update_TotalTimeAccumulates)
{
    Time::Update(0.1f);
    Time::Update(0.2f);
    Time::Update(0.3f);
    EXPECT_NEAR(Time::GetTotalTime(), 0.6f, kEps);
}

TEST_F(TimeTest, Update_TotalTime_UsesRawDelta_NotAverage)
{
    // TotalTime은 스무딩 평균이 아닌 클램프된 raw delta를 합산한다
    Time::Update(0.1f);
    Time::Update(0.3f);
    EXPECT_NEAR(Time::GetTotalTime(), 0.4f, kEps);
}

// -----------------------------------------------------------------------
// Pause / Resume
// -----------------------------------------------------------------------
TEST_F(TimeTest, Pause_DeltaTimeIsZero)
{
    Time::Update(0.016f);
    Time::Pause();
    Time::Update(0.016f);
    EXPECT_NEAR(Time::GetDeltaTime(), 0.0f, kEps);
}

TEST_F(TimeTest, Pause_UnscaledDeltaNotAffected)
{
    Time::Update(0.016f);
    Time::Pause();
    Time::Update(0.016f);
    // unscaledDeltaTime은 timeScale과 무관하게 계산된다
    EXPECT_GT(Time::GetUnscaledDeltaTime(), 0.0f);
}

TEST_F(TimeTest, Pause_DoublePause_SavedTimeScalePreserved)
{
    Time::SetTimeScale(3.0f);
    Time::Pause();
    Time::Pause();  // 두 번째 Pause는 savedTimeScale을 덮어쓰면 안 된다
    Time::Resume();
    EXPECT_NEAR(Time::GetTimeScale(), 3.0f, kEps);
}

TEST_F(TimeTest, Resume_RestoresTimeScale)
{
    Time::SetTimeScale(2.0f);
    Time::Pause();
    Time::Resume();
    EXPECT_NEAR(Time::GetTimeScale(), 2.0f, kEps);
}

TEST_F(TimeTest, Resume_WhenNotPaused_NoEffect)
{
    Time::SetTimeScale(1.5f);
    Time::Resume();  // 일시정지 상태가 아닐 때 호출해도 TimeScale 변화 없어야 한다
    EXPECT_NEAR(Time::GetTimeScale(), 1.5f, kEps);
}

TEST_F(TimeTest, PauseResume_DeltaTimeRestored)
{
    Time::Update(0.016f);

    Time::Pause();
    Time::Update(0.016f);
    EXPECT_NEAR(Time::GetDeltaTime(), 0.0f, kEps);

    Time::Resume();
    Time::Update(0.016f);
    EXPECT_GT(Time::GetDeltaTime(), 0.0f);
}
