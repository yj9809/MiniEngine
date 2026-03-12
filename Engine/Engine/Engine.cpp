#include "Engine.h"
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
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		// 프레임 계산용 변수.
		int64_t currentTime = 0;
		int64_t lastTime = 0;

		// 하드웨어 타이머로 시간 구하기.
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);

		// 엔진 시작 직전에는 두 시간 값을 동일하게 설정.
		currentTime = time.QuadPart;
		lastTime = time.QuadPart;

		settings.frameRate = settings.frameRate == 0.0f ? 120.0f : settings.frameRate;
		float oneFrameTime = 1.0f / settings.frameRate;

		// 엔진 루프 시작.
		while (!isQuit)
		{
			// 하드웨어 타이머로 시간 구하기.
			QueryPerformanceCounter(&time);
			currentTime = time.QuadPart;

			// 프레임 시간 계산 및 초 단위 변환.
			float deltaTime = static_cast<float>(currentTime - lastTime);
			deltaTime /= static_cast<float>(frequency.QuadPart);

			if (deltaTime >= oneFrameTime)
			{
				// 업데이트 및 그리기 함수 호출.
				BeginPlay();
				Tick(deltaTime);
				Draw();

				// 마지막 시간 업데이트.
				lastTime = currentTime;

				if (mainLevel)
				{
					mainLevel->ProcessAddAndDestroyActor();
				}
			}
		}
	}

	void Engine::QuitEngine()
	{
		isQuit = true;
	}

	void Engine::SetNewLevel(std::unique_ptr<Level> level)
	{

		mainLevel = std::move(level);
	}

	void Engine::Initialize()
	{
		LoadSettings();
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
			std::ifstream file(path);
			if (file.is_open())
			{
				std::string line;
				while (std::getline(file, line))
				{
					if (line.find("FrameRate:") == 0)
					{
						settings.frameRate = std::stoi(line.substr(10));
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

	void Engine::BeginPlay()
	{
		if (!mainLevel)
		{
			return;
		}

		mainLevel->BeginPlay();
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
	}
}
