#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace Engine
{
    class Level;

    struct EngineCreateInfo
    {
        std::unique_ptr<Level> startupLevel;

        // 실행 위치에 숨겨진 의존성을 만들지 않고, 테스트에서도 별도의 설정 경로를 주입할 수 있게 한다.
        std::filesystem::path settingsPath = "Setting/Settings.txt";
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
