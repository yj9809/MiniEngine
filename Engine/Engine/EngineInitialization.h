#pragma once

#include <memory>
#include <string>

namespace Engine
{
    class Level;

    struct EngineCreateInfo
    {
        std::unique_ptr<Level> startupLevel;
    };

    // Engine::Create()에서 수행하는 Core Bootstrap 단계를 나타낸다.
    enum class EngineInitialization
    {
        Settings,
        Window,
        Renderer,
        Resource,
        StartupLevel
    };

    struct EngineInitError
    {
        EngineInitialization engineInitialization;
        std::string errorMessage;
    };
}
