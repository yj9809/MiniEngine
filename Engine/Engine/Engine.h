#pragma once

#include "Common/Common.h"
#include "Core/Input.h"

#include <memory>


namespace Engine
{
	class Level;

	class ENGINE_API Engine
	{
		struct Settings
		{
			// 프레임.
			float frameRate = 120;

			// 화면 너비.
			int width = 1280;

			// 화면 높이.
			int height = 720;
		};

	public:
		Engine();
		~Engine();

		// 엔진 루프.
		void Run();

		// 엔진 종료.
		static void QuitEngine();

		void SetNewLevel(std::unique_ptr<Level> level);

	protected:
		// 엔진 초기화.
		void Initialize();

		// 엔진 세팅.
		void LoadSettings();

		// 업데이트 함수.
		void Tick(float deltaTime);

		// 그리기 함수.
		void Draw();

	protected:
		// 엔진 종료 플래그.
		inline static bool isQuit = false;

		// 엔진 설정.
		Settings settings;

		Input input;

		// 메인 레벨.
		std::unique_ptr<Level> mainLevel;
	};
}

