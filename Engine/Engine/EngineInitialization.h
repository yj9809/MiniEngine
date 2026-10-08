#pragma once

#include <string>

namespace Engine
{
    enum class EngineInitialization
    {
        Settings,
        Window,
        Renderer,
        Resource
    };

    struct EngineInitError
    {
        EngineInitialization engineInitialization;
        std::string errorMessage;
    };
}
