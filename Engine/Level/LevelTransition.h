#pragma once

#include <string>

namespace Engine
{
    enum class LevelTransitionRequestResult
    {
        Accepted,
        NullLevel,
        Busy,
    };

    enum class LevelTransitionStage
    {
        AttachServices,
        BeginPlay,
        InitialActorActivation
    };

    struct LevelTransitionError
    {
        LevelTransitionStage stage;
        std::string errorMessage;
    };
}
