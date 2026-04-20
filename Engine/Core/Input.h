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
        static bool GetKeyDown(int vkCode);
        static bool GetKeyUp(int vkCode);
        static bool GetKey(int vkCode);

        static void ResetKeyState();

        static void ProcessInputMessage(int vkCode, bool isKeyUp, bool isKeyDown);

        // Setter.
        static void SetMousePosition(const Vector2& position);
        static void SetPreviousMousePosition();

        static void SetMouseButtonState(int idx, bool down, bool up);

        static void SetIsMouseClamped(bool isClamped);

        // Getter.
        inline static Vector2 GetMousePosition()
        {
            return mousePosition.currentPosition;
        }

        inline static Vector2 GetMouseDeltaPosition()
        {
            return mousePosition.currentPosition - mousePosition.previousPosition;
        }

        inline static bool GetMouseButtonDown(int idx) { return mouseInput[idx].isButtonDown; }
        inline static bool GetMouseButtonUp(int idx) { return mouseInput[idx].isButtonUp; }
        inline static bool GetMouseButton(int idx) { return mouseInput[idx].isButton; }

        inline static bool GetIsMouseClamped() { return isMouseClamped; }

    private:
        // Windows 가상 키 코드는 0x00~0xFF(256개)이므로 배열 크기를 256으로 고정.
        static KeyState keyInput[256];

        static MouseState mouseInput[3];

        static MousePosition mousePosition;

        static bool isMouseClamped;
    };
}
