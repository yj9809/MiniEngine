#include "Engine.h"

#include <filesystem>
#include <fstream>
#include <utility>
#include <cmath>
#include <exception>
#include <Windows.h>

#include "Core/Input.h"
#include "Core/Win32Window.h"
#include "Core/Time.h"
#include "Level/Level.h"
#include "Renderer/D3D11Renderer.h"
#include "Resource/ResourceManager.h"
#include "Common/ScopedFlag.h"

namespace fs = std::filesystem;

namespace Engine
{
    EngineCreateResult Engine::Create(EngineCreateInfo createInfo)
    {
        if (!createInfo.startupLevel)
        {
            return EngineInitError
            {
                EngineInitialization::StartupLevel,
                "Startup level must not be null."
            };
        }

        // make_unique는 private 생성자에 접근할 수 없으므로,
        // 여기서는 직접 new로 생성 후 unique_ptr에 담는다.
        auto engine = std::unique_ptr<Engine>(new Engine());

        if (auto error = engine->Initialize(std::move(createInfo)); error.has_value())
        {
            return std::move(*error);
        }

        return std::move(engine);
    }

    Engine::~Engine() noexcept
    {
        // Reject new level requests from cleanup callbacks during engine shutdown.
        isLevelTransitionInProgress = true;
        ShutdownLevel(pendingLevel);
        ShutdownLevel(mainLevel);
        resourceManager.reset();
        renderer.reset();
        window.reset();
    }

    void Engine::Run()
    {
        // QueryPerformanceFrequency: 초당 카운터 틱 수. CPU마다 다르므로 런타임에 조회한다.
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        // 프레임 계산용 변수.
        int64_t currentTime = 0;
        int64_t lastTime = 0;

        // 루프 진입 전 초기 시간을 동일하게 설정해 첫 deltaTime이 0이 되도록 한다.
        LARGE_INTEGER time;
        QueryPerformanceCounter(&time);

        // 엔진 시작 직전에는 두 시간 값을 동일하게 설정.
        currentTime = time.QuadPart;
        lastTime = time.QuadPart;

        // frameRate가 0이면 기본값 120으로 보정.
        settings.frameRate = settings.frameRate == 0.0f ? 120.0f : settings.frameRate;
        float oneFrameTime = 1.0f / settings.frameRate;

        // Time에 하드 클램프 값 설정.
        Time::SetMaxDeltaTime(oneFrameTime * 10);

        // 고정 프레임레이트 루프.
        // deltaTime이 목표 프레임 시간(oneFrameTime)을 넘었을 때만 Tick/Draw를 실행한다.
        // busy-wait 방식이므로 CPU를 점유하지만, Sleep보다 정밀한 타이밍을 보장한다.
        while (!isQuit)
        {
            // 하드웨어 타이머로 시간 구하기.
            QueryPerformanceCounter(&time);
            currentTime = time.QuadPart;

            // (currentTime - lastTime) 틱을 frequency로 나누면 초 단위 경과 시간이 된다.
            float deltaTime = static_cast<float>(currentTime - lastTime);
            deltaTime /= static_cast<float>(frequency.QuadPart);

            MSG msg = {};
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    isQuit = true;
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (deltaTime >= oneFrameTime)
            {
                // 시간 업데이트.
                Time::Update(deltaTime);

                // 업데이트 및 그리기 함수 호출.
                // TimeScale이 적용된 시간으로 Tick 업데이트.
                Tick(Time::GetDeltaTime());
                Draw();

                // Tick/Draw 완료 후 액터 추가·제거를 일괄 처리한다.
                // 순회 중 배열을 수정하지 않기 위해 프레임 끝으로 미룬다.
                if (mainLevel)
                {
                    mainLevel->ProcessAddAndDestroyActor();
                }

                if (Input::GetIsMouseClamped())
                {
                    RECT windowRect = window->GetWindowRect();
                    ClampCursor(&windowRect);
                    CenterCursor(&windowRect);
                }

                Input::SetPreviousMousePosition();
                Input::ResetKeyState();

                // 현재 프레임 작업을 마친 뒤 레벨 전환 작업을 진행한다.
                if(!isQuit)
                {
                    ProcessPendingLevelChange();
                }

                // 마지막 시간 업데이트.
                lastTime = currentTime;
            }
        }
    }

    void Engine::QuitEngine()
    {
        isQuit = true;
    }

    LevelTransitionRequestResult Engine::RequestLevelChange(std::unique_ptr<Level>&& level)
    {
        if (!level)
        {
            return LevelTransitionRequestResult::NullLevel;
        }

        if (pendingLevel || isLevelTransitionInProgress)
        {
            return LevelTransitionRequestResult::Busy;
        }

        // 새 요청의 결과를 기록하기 전에 이전 오류를 제거한다.
        levelTransitionError.reset();

        // 접수가 확정된 경우에만 소유권을 가져온다.
        pendingLevel = std::move(level);

        return LevelTransitionRequestResult::Accepted;
    }

    std::optional<LevelTransitionError> Engine::ConsumeLevelTransitionError()
    {
        // 이전 오류를 소모한다.
        auto error = std::move(levelTransitionError);
        levelTransitionError.reset();

        return error;
    }

    std::optional<EngineInitError> Engine::Initialize(EngineCreateInfo createInfo)
    {
        try
        {
            if (auto error = LoadSettings(createInfo.settingsPath); error.has_value())
            {
                return error;
            }
        }
        catch (const std::exception& e)
        {
            return EngineInitError
            {
                EngineInitialization::Settings,
                std::string("Failed to load settings: ") + e.what()
            };
        }

        try
        {
            // 출력 창 생성 및 초기화.
            window = std::make_unique<Win32Window>(settings.width, settings.height, L"Eden Engine");
        }
        catch (const std::exception& e)
        {
            return EngineInitError
            {
                EngineInitialization::Window,
                std::string("Failed to create window: ") + e.what()
            };
        }

        if (window->GetHwnd() == nullptr)
        {
            return EngineInitError
            {
                EngineInitialization::Window,
                "Failed to create Win32 window."
            };
        }

        try
        {
            // 렌더러 생성 및 초기화.
            // 출력 창이 먼저 만들어진 뒤에 HWND를 넘거야 하기 때문에 반드시 출력 창 생성 후 호출.
            auto initializedRenderer = std::make_unique<D3D11Renderer>();

            auto error = initializedRenderer->GPUInit(window->GetHwnd(), settings.width, settings.height);

            if (error.has_value())
            {
                return EngineInitError
                {
                    EngineInitialization::Renderer,
                    error->errorMessage
                };
            }

            // 초기화에 성공한 렌더러만 Engine에 연결.
            renderer = std::move(initializedRenderer);
        }
        catch (const std::exception& e)
        {
            return EngineInitError
            {
                EngineInitialization::Renderer,
                std::string("Failed to initialize renderer: ") + e.what()
            };
        }

        try
        {
            resourceManager = std::make_unique<ResourceManager>(*renderer);
        }
        catch (const std::exception& e)
        {
            return EngineInitError
            {
                EngineInitialization::Resource,
                std::string("Failed to initialize resource manager: ") + e.what()
            };
        }

        try
        {
            // 먼저 Engine이 소유권을 가진다.
            // 이후 단계에서 실패해도 Engine 소멸자가 Level을 정리할 수 있다.
            mainLevel = std::move(createInfo.startupLevel);

            if (auto error = TryActivateLevel(*mainLevel))
            {
                return EngineInitError
                {
                    EngineInitialization::StartupLevel,
                    std::string("Failed to Initialize startup level: ") + error->errorMessage
                };
            }
        }
        catch (const std::exception& e)
        {
            return EngineInitError
            {
                EngineInitialization::StartupLevel,
                std::string("Failed to initialize startup level: ") + e.what()
            };
        }

        return std::nullopt;
    }

    std::optional<EngineInitError> Engine::LoadSettings(const std::filesystem::path& path)
    {
        auto makeError = [](std::string message) -> std::optional<EngineInitError>
        {
            return EngineInitError
            {
                EngineInitialization::Settings,
                std::move(message)
            };
        };

        if (path.empty())
        {
            return makeError("Settings path must not be empty.");
        }

        Settings loadedSettings = settings;
        const fs::path directory = path.parent_path();

        if (!directory.empty())
        {
            std::error_code errorCode;
            fs::create_directories(directory, errorCode);

            if (errorCode)
            {
                return makeError("Failed to create settings directory: " + errorCode.message());
            }
        }

        std::error_code errorCode;
        const bool fileExists = fs::exists(path, errorCode);

        if (errorCode)
        {
            return makeError("Failed to inspect settings path: " + errorCode.message());
        }

        if (!fileExists)
        {
            std::ofstream file(path, std::ios::trunc);

            if (!file.is_open())
            {
                return makeError("Failed to create settings file: " + path.string());
            }

            file << "FrameRate: " << loadedSettings.frameRate << "\n";
            file << "Width: " << loadedSettings.width << "\n";
            file << "Height: " << loadedSettings.height << "\n";
            file.flush();

            if (!file)
            {
                return makeError("Failed to write default settings to file: " + path.string());
            }
        }
        else
        {
            std::ifstream file(path);

            if (!file.is_open())
            {
                return makeError("Failed to open settings file: " + path.string());
            }

            std::string line;

            while (std::getline(file, line))
            {
                if (line.find("FrameRate:") == 0)
                {
                    loadedSettings.frameRate =
                        std::stof(line.substr(10));
                }
                else if (line.find("Width:") == 0)
                {
                    loadedSettings.width =
                        std::stoi(line.substr(6));
                }
                else if (line.find("Height:") == 0)
                {
                    loadedSettings.height =
                        std::stoi(line.substr(7));
                }
            }

            if (file.bad())
            {
                return makeError("Failed while reading settings file: " + path.string());
            }
        }

        if (!std::isfinite(loadedSettings.frameRate) ||
            loadedSettings.frameRate <= 0.0f ||
            loadedSettings.width <= 0 ||
            loadedSettings.height <= 0)
        {
            return makeError(
                "Settings values must be finite and greater than zero.");
        }

        settings = loadedSettings;
        return std::nullopt;
    }

    void Engine::Tick(float deltaTime)
    {
        if (!mainLevel)
        {
            return;
        }

        mainLevel->Tick(deltaTime);
    }

    void Engine::Draw()
    {
        if (!renderer)
        {
            return;
        }

        // 출력 창 배경색 지정.
        renderer->BeginFrame(0.06f, 0.06f, 0.06f);

        if (mainLevel)
        {
            mainLevel->Draw();
        }

        // 오브젝트 렌더링.
        renderer->Render();

        renderer->EndFrame();
    }

    void Engine::ClampCursor(RECT* windowRect)
    {
        ClipCursor(windowRect);

        ShowCursor(windowRect == nullptr);
    }

    void Engine::CenterCursor(RECT* windowRect)
    {
        if (windowRect)
        {
            Vector2 centerPosition;
            centerPosition.x = (windowRect->left + windowRect->right) / 2.0f;
            centerPosition.y = (windowRect->top + windowRect->bottom) / 2.0f;

            SetCursorPos(static_cast<int>(centerPosition.x), static_cast<int>(centerPosition.y));

            Input::SetMousePosition(centerPosition);
        }
    }

    std::optional<LevelTransitionError> Engine::TryActivateLevel(Level& level)
    {
        const auto tryStage = [](LevelTransitionStage stage, auto&& operation)
            -> std::optional<LevelTransitionError>
        {
            try
            {
                operation();
                return std::nullopt;
            }
            catch (const std::exception& e)
            {
                return LevelTransitionError
                {
                    stage,
                    e.what()
                };
            }
            catch (...)
            {
                return LevelTransitionError
                {
                    stage,
                    "Unknown exception occurred."
                };
            }
        };

        if (auto error = tryStage(LevelTransitionStage::AttachServices, [&]()
        {
            level.AttachServices(*renderer, *resourceManager);
        }))
        {
            return error;
        }

        if (auto error = tryStage(LevelTransitionStage::BeginPlay, [&]()
        {
            level.BeginPlay();
        }))
        {
            return error;
        }

        if (auto error = tryStage(LevelTransitionStage::InitialActorActivation, [&]()
        {
            level.ProcessAddAndDestroyActor();
        }))
        {
            return error;
        }

        return std::nullopt;
    }

    void Engine::ProcessPendingLevelChange()
    {
        if(!pendingLevel || isLevelTransitionInProgress)
        {
            return;
        }

        ScopedFlag guard(isLevelTransitionInProgress);

        // 기존 mainLevel은 유지한 상태에서 후보를 활성화한다.
        if (auto error = TryActivateLevel(*pendingLevel))
        {
            levelTransitionError = std::move(*error);

            // 부분적으로 활성화된 pendingLevel을 정리한다.
            ShutdownLevel(pendingLevel);
            return;
        }

        // 후보 활성화에 성공했으므로 기존 mainLevel을 종료한다.
        ShutdownLevel(mainLevel);

        // mainLevel을 새로 활성화된 pendingLevel로 교체한다.
        mainLevel = std::move(pendingLevel);
    }

    void Engine::ShutdownLevel(std::unique_ptr<Level>& level) noexcept
    {
        if (!level)
        {
            return;
        }

        // Keep requests blocked through EndLevel and object destruction.
        const bool wasTransitionInProgress =
            std::exchange(isLevelTransitionInProgress, true);

        try
        {
            level->EndLevel();
        }
        catch (const std::exception& e)
        {
            OutputDebugStringA("Exception in EndLevel(): ");
            OutputDebugStringA(e.what());
            OutputDebugStringA("\n");
        }
        catch (...)
        {
            OutputDebugStringA("Unknown exception in EndLevel()\n");
        }

        level.reset();
        isLevelTransitionInProgress = wasTransitionInProgress;
    }
}
