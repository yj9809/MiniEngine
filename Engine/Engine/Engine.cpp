#include "Engine.h"

#include "Core/Input.h"
#include "Core/Win32Window.h"

#include "Level/Level.h"

#include <fstream>
#include <filesystem>
#include <Windows.h>

namespace fs = std::filesystem;

namespace Engine
{
	Engine::Engine()
	{
		Initialize();
	}

	Engine::~Engine()
	{

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
				// 업데이트 및 그리기 함수 호출.
				Tick(deltaTime);
				Draw();

				// Tick/Draw 완료 후 액터 추가·제거를 일괄 처리한다.
				// 순회 중 배열을 수정하지 않기 위해 프레임 끝으로 미룬다.
				if (mainLevel)
				{
					mainLevel->ProcessAddAndDestroyActor();
				}

				input.ResetKeyState();

				// 마지막 시간 업데이트.
				lastTime = currentTime;
			}
		}
	}

	void Engine::QuitEngine()
	{
		isQuit = true;
	}

	void Engine::SetNewLevel(std::unique_ptr<Level> level)
	{
		// 기존 레벨이 있으면 먼저 정리한다(액터 전부 해제).
		if (mainLevel)
		{
			mainLevel->EndLevel();
		}

		mainLevel = std::move(level);
		mainLevel->BeginPlay();
	}

	void Engine::Initialize()
	{
		input = Input::Get();
		LoadSettings();

		window = std::make_unique<Win32Window>(settings.width, settings.height, L"Mini Engine");
	}

	void Engine::LoadSettings()
	{
		fs::path path = "Setting/Settings.txt";
		fs::path dir = path.parent_path();

		// 폴더가 없으면 생성.
		if (!dir.empty() && !fs::exists(dir))
		{
			fs::create_directory(path.parent_path());
		}

		if (!fs::exists(path))
		{
			// 파일이 없으면 현재 기본값으로 새로 만든다.
			std::ofstream file(path);
			if (file.is_open())
			{
				file << "FrameRate: " << settings.frameRate << "\n";
				file << "Width: " << settings.width << "\n";
				file << "Height: " << settings.height << "\n";
				file.close();
			}

		}
		else
		{
			// 한 줄씩 읽어 "키: 값" 형식으로 파싱한다.
			// substr의 오프셋은 각 키 문자열 길이 + 공백 1칸이다("FrameRate: " = 11자 → substr(11)).
			std::ifstream file(path);
			if (file.is_open())
			{
				std::string line;
				while (std::getline(file, line))
				{
					if (line.find("FrameRate:") == 0)
					{
						settings.frameRate = static_cast<float>(std::stoi(line.substr(10)));
					}
					else if (line.find("Width:") == 0)
					{
						settings.width = std::stoi(line.substr(6));
					}
					else if (line.find("Height:") == 0)
					{
						settings.height = std::stoi(line.substr(7));
					}
				}
				file.close();
			}
		}
	}

	void Engine::Tick(float deltaTime)
	{
		if (!mainLevel)
		{
			return;
		}

		input.Update();
		mainLevel->Tick(deltaTime);
	}

	void Engine::Draw()
	{
	}
}
