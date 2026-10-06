#pragma once
#include <cstdint>

namespace Engine
{
    enum class LifecycleState : uint8_t
    {
        Constructed,
        Initializing,
        Initialized,
        BeginningPlay,
        HasBegunPlay,
        EndingPlay,
        HasEndedPlay
    };
}
