#pragma once

#include <memory>
#include <variant>
#include <optional>

#include "Common/Common.h"
#include "Core/Input.h"
#include "EngineInitialization.h"

namespace Engine
{
	class Level;
	class Win32Window;
	class IRenderer;
	class ResourceManager;
	
	class Engine;
	using EngineCreateResult = std::variant<std::unique_ptr<Engine>, EngineInitError>;

	// 엔진의 진입점이자 최상위 관리자 클래스.
	// 게임 루프(Run), 설정 로드(LoadSettings), 레벨 전환(SetNewLevel)을 담당한다.
	class ENGINE_API Engine final
	{
		// Settings.txt에서 읽어오는 런타임 설정값.
		struct Settings
		{
			// 목표 프레임레이트. Run()에서 고정 프레임 루프의 기준이 된다.
			float frameRate = 120;

			// 화면 너비.
			int width = 1280;

			// 화면 높이.
			int height = 720;
		};

	public:
		// 완전히 초기화된 Engine 또는 초기화 실패 정보를 반환한다.
		[[nodiscard]] static EngineCreateResult Create();
		
		~Engine() noexcept;
		
		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;
		Engine(Engine&&) = delete;
		Engine& operator=(Engine&&) = delete;

		// 게임 루프를 시작한다. isQuit가 true가 될 때까지 블로킹된다.
		void Run();

		// isQuit 플래그를 세워 다음 루프 반복에서 Run()이 종료되도록 한다.
		static void QuitEngine();

		// 현재 레벨을 종료하고 새 레벨로 교체한다.
		// 이전 레벨의 EndLevel()을 호출해 정리한 뒤, 새 레벨의 BeginPlay()를 호출한다.
		void SetNewLevel(std::unique_ptr<Level> level);
		
		inline Level* GetMainLevel() const { return mainLevel.get(); }

	private:
		// 객체의 메모리만 할당하고, 실제 초기화는 Initialize()에서 수행한다.
		Engine() noexcept = default;
		
		// 엔진 초기화. Create()에서 호출된다.
		[[nodiscard]] std::optional<EngineInitError> Initialize();

		// Setting/Settings.txt를 읽어 settings를 채운다.
		// 파일이 없으면 기본값으로 새로 생성한다.
		void LoadSettings();

		// 입력 갱신 및 현재 레벨의 Tick을 호출한다.
		void Tick(float deltaTime);

		// 현재 레벨의 Draw를 호출한다.
		void Draw();
		
		// 커서를 창 내부로 제한한다.
		void ClampCursor(RECT* windowRect);

		// 커서를 창의 중심으로 이동시킨다.
		void CenterCursor(RECT* windowRect);

	private:
		// true가 되면 Run()의 루프가 종료된다. static으로 선언해 어디서든 QuitEngine()으로 설정 가능.
		inline static bool isQuit = false;

		// 출력 창.
		std::unique_ptr<Win32Window> window;
		
		// 렌더러.
		std::unique_ptr<IRenderer> renderer;

		// 게임 전역 리소스 캐시.
		// Renderer보다 먼저 파괴되어야 한다.
		std::unique_ptr<ResourceManager> resourceManager;

		// Settings.txt에서 로드된 설정값.
		Settings settings;

		// 현재 활성 레벨. unique_ptr로 소유권 관리. SetNewLevel()로 교체한다.
		std::unique_ptr<Level> mainLevel;
	};
}


