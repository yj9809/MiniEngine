#pragma once

#include "Common/Common.h"
#include "Math/Vector2.h"

#include <Windows.h>

namespace Engine
{
	// 키보드 입력을 프레임 단위로 추적하는 싱글턴 클래스.
	// 매 프레임 Update()를 호출하면 전 프레임 상태(previous)와 현재 상태(current)를 비교해
	// Pressed / Held / Released를 구분할 수 있다.
	// 싱글턴은 Input에만 허용된다(CLAUDE.md 설계 원칙 참고).
	class ENGINE_API Input
	{
	private:
		// 키 하나의 현재/이전 프레임 눌림 상태를 저장한다.
		// 두 값의 조합으로 Pressed(↑→↓), Held(↓→↓), Released(↓→↑)를 판별한다.
		struct KeyState
		{
			bool isKeyDown = false;
			bool isKeyUp = false;
			bool isKey = false;

			void SetKeyUpDown(bool isKeyUp, bool isKeyDown)
			{
				this->isKeyUp = isKeyUp;
				this->isKeyDown = isKeyDown;

				this->isKey = isKeyDown && !isKeyUp;
			}
		};

		struct MouseState
		{
			bool isButtonDown = false;
			bool isButtonUp = false;
			bool isButton = false;

			void SetButtonUpDown(bool isButtonDown, bool isButtonUp)
			{
				this->isButtonDown = isButtonDown;
				this->isButtonUp = isButtonUp;

				this->isButton = isButtonDown && !isButtonUp;
			}
		};

		struct MousePosition
		{
			Vector2 currentPosition;
			Vector2 previousPosition;
		};

	public:
		Input();
		~Input();

		bool GetKeyDown(int vkCode);
		bool GetKeyUp(int vkCode);
		bool GetKey(int vkCode);

		void ResetKeyState();

		void ProcessInputMessage(int vkCode, bool isKeyUp, bool isKeyDown);

		// Setter.
		void SetMousePosition(const Vector2& position);
		void SetPreviousMousePosition();
		
		void SetMouseButtonState(int idx, bool down, bool up);

		void SetIsMouseClamped(bool isClamped);

		// Getter.
		Vector2 GetMousePosition() const;
		Vector2 GetMouseDeltaPosition() const;

		bool GetMouseButtonDown(int idx) const;
		bool GetMouseButtonUp(int idx) const;
		bool GetMouseButton(int idx) const;

		inline bool GetIsMouseClamped() const { return isMouseClamped; }

		static inline Input& Get() { return *instance; }

	private:
		// Windows 가상 키 코드는 0x00~0xFF(256개)이므로 배열 크기를 256으로 고정.
		KeyState keyInput[256] = { KeyState{false, false} };

		MouseState mouseInput[3] = { MouseState{false, false} };

		MousePosition mousePosition;

		bool isMouseClamped = false;

		static Input* instance;
	};
}
