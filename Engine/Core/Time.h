#pragma once

#include "Common/Common.h"

namespace Engine
{
    class ENGINE_API Time
    {
    public:
        static void Update(float newDeltaTime);
        
        static void Pause();
        static void Resume();

        // Getter.
        // TimeScale이 적용된 deltaTime 반환 함수.
        inline static float GetDeltaTime() { return deltaTime; }
        // TimeScale이 적용되지 않은 deltaTime 반환 함수.
        inline static float GetUnscaledDeltaTime() { return unscaledDeltaTime; }
        // 실행부터 호출 시점까지 누적된 시간 반환 함수.
        inline static float GetTotalTime() { return totalTime; }
        // TimeScale 반환 함수.
        inline static float GetTimeScale() { return timeScale; }
        
        // Setter.
        // TimeScale 설정 함수.
        inline static void SetTimeScale(float newTimeScale) { timeScale = newTimeScale; }
        // 하드 클램프 값 설정 함수.
        inline static void SetMaxDeltaTime(float oneFrame) { maxDeltaTime = oneFrame; }

    private:
        static float deltaTime;
        static float unscaledDeltaTime;
        static float totalTime;
        static float timeScale;
        
        // 하드 클램프 값.
        static float maxDeltaTime;
        
        // Frame 평균을 위한 delta Time 컨테이너.
        static float sampleFrame[10];
        // 현재 샘플 프레임 인덱스.
        static int sampleFrameIndex;
        // 실제 추가된 샘플 프레임 개수 (최대 10).
        static int sampleFrameCount;
        
        // 일시정지를 위한 플래그.
        static bool isPaused;
        // 일시정지 시점의 TimeScale 저장 변수.
        static float savedTimeScale;
    };
}
