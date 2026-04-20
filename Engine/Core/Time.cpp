#include "Time.h"

#include <algorithm>

namespace Engine
{
    float Time::deltaTime = 0.0f;
    float Time::unscaledDeltaTime = 0.0f;
    float Time::totalTime = 0.0f;
    float Time::timeScale = 1.0f;
    
    float Time::maxDeltaTime = FLT_MAX;
    
    float Time::sampleFrame[10] = { 0.0f };
    int Time::sampleFrameCount = 0;
    int Time::sampleFrameIndex = 0;
    
    bool Time::isPaused = false;
    float Time::savedTimeScale = 1.0f;
    
    void Time::Update(float newDeltaTime)
    {
        newDeltaTime = std::clamp(newDeltaTime, 0.0f, maxDeltaTime);
        
        sampleFrame[sampleFrameIndex] = newDeltaTime;
        
        sampleFrameIndex = (sampleFrameIndex + 1) % 10;
        
        if (sampleFrameCount < 10)
        {
            sampleFrameCount++;
        }
        
        float totalSampleTime = 0.0f;
        for (int i = 0; i < sampleFrameCount; i++)
        {
            totalSampleTime += sampleFrame[i];
        }
        
        unscaledDeltaTime = totalSampleTime / sampleFrameCount;
        
        deltaTime = unscaledDeltaTime * timeScale;
        
        totalTime += newDeltaTime;
    }

    void Time::Reset()
    {
        deltaTime        = 0.0f;
        unscaledDeltaTime = 0.0f;
        totalTime        = 0.0f;
        timeScale        = 1.0f;
        maxDeltaTime     = FLT_MAX;
        sampleFrameCount = 0;
        sampleFrameIndex = 0;
        isPaused         = false;
        savedTimeScale   = 1.0f;
        memset(sampleFrame, 0, sizeof(sampleFrame));
    }

    void Time::Pause()
    {
        if (isPaused)
        {
            return;
        }
        
        isPaused = true;
        savedTimeScale = timeScale;
        timeScale = 0.0f;
    }

    void Time::Resume()
    {
        if (!isPaused)
        {
            return;
        }
        
        isPaused = false;
        timeScale = savedTimeScale;
    }
}
