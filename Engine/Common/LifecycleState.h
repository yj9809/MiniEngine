#pragma once
#include <cstdint>

namespace Engine
{
    enum class LifecycleState : uint8_t
    {
        Constructed,
        Initializing,
        InitializationFailed,
        Initialized,
        BeginningPlay,
        BeginPlayFailed,
        HasBegunPlay,
        EndingPlay,
        HasEndedPlay
    };
}
