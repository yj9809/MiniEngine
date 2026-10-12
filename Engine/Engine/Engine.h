#pragma once

#include <memory>
#include <variant>
#include <optional>
#include <filesystem>

#include "Common/Common.h"
#include "Core/Input.h"
#include "EngineInitialization.h"
#include "Level/LevelTransition.h"

namespace Engine
{
	class Level;
	class Win32Window;
	class IRenderer;
	class ResourceManager;

	class Engine;
	using EngineCreateResult = std::variant<std::unique_ptr<Engine>, EngineInitError>;

	// 엔진의 진입점이자 최상위 관리자 클래스.
	// 게임 루프(Run), 설정 로드(LoadSettings), 레벨 전환(RequestLevelChange)을 담당한다.
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
		// Core Bootstrap과 Startup Level 활성화를 완료한 Engine을 반환한다.
		[[nodiscard]] static EngineCreateResult Create(EngineCreateInfo createInfo);

		~Engine() noexcept;

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;
		Engine(Engine&&) = delete;
		Engine& operator=(Engine&&) = delete;

		// 게임 루프를 시작한다. isQuit가 true가 될 때까지 블로킹된다.
		void Run();

		// isQuit 플래그를 세워 다음 루프 반복에서 Run()이 종료되도록 한다.
		static void QuitEngine();

		// 메인 스레드 전용.
		// Level을 즉시 활성화하지 않고 다음 프레임 경계까지 보관한다.
		[[nodiscard]] LevelTransitionRequestResult RequestLevelChange(std::unique_ptr<Level>&& level);

		// 지연 처리 과정에서 발생한 최근 오류를 꺼낸다.
		[[nodiscard]] std::optional<LevelTransitionError> ConsumeLevelTransitionError();

		inline Level* GetMainLevel() const { return mainLevel.get(); }

	private:
		// 객체의 메모리만 할당하고, 실제 초기화는 Initialize()에서 수행한다.
		Engine() noexcept = default;

		// 엔진 초기화. Create()에서 호출된다.
		[[nodiscard]] std::optional<EngineInitError> Initialize(EngineCreateInfo createInfo);

		// 전달된 경로의 설정 파일을 읽어 settings를 채운다.
		// 파일이 없으면 기본값으로 새로 생성한다.
		[[nodiscard]] std::optional<EngineInitError> LoadSettings(const std::filesystem::path& path);

		// 입력 갱신 및 현재 레벨의 Tick을 호출한다.
		void Tick(float deltaTime);

		// 현재 레벨의 Draw를 호출한다.
		void Draw();

		// 커서를 창 내부로 제한한다.
		void ClampCursor(RECT* windowRect);

		// 커서를 창의 중심으로 이동시킨다.
		void CenterCursor(RECT* windowRect);

		// 시작 Level과 런타임 Level이 공유하는 활성화 순서.
		std::optional<LevelTransitionError> TryActivateLevel(Level& level);

		// 프레임 경계에서 pendingLevel을 활성 Level로 전환한다.
		void ProcessPendingLevelChange();

		// EndLevel 예외와 관계없이 소유권을 확실히 정리한다.
		void ShutdownLevel(std::unique_ptr<Level>& level) noexcept;

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

		// 현재 활성 레벨. unique_ptr로 소유권 관리. RequestLevelChange()로 교체한다.
		std::unique_ptr<Level> mainLevel;

		// 다음 프레임 경계에서 활성화될 레벨. RequestLevelChange()로 교체된다.
		std::unique_ptr<Level> pendingLevel;

		std::optional<LevelTransitionError> levelTransitionError;

		bool isLevelTransitionInProgress = false;
	};
}


