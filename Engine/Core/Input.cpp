#include "Input.h"

namespace Engine
{
	Input* Input::instance = nullptr;

	Input::Input()
	{
		instance = this;
	}

	Input::~Input()
	{
	}

	bool Input::GetKeyDown(int vkCode)
	{
		return keyInput[vkCode].isKeyDown;
	}

	bool Input::GetKeyUp(int vkCode)
	{
		return keyInput[vkCode].isKeyUp;
	}

	bool Input::GetKey(int vkCode)
	{
		return keyInput[vkCode].isKey;
	}

	void Input::ResetKeyState()
	{
		for (KeyState& keyState : keyInput)
		{
			keyState.isKeyUp = false;
			keyState.isKeyDown = false;
		}

		for (MouseState& mouseState : mouseInput)
		{
			mouseState.isButtonUp = false;
			mouseState.isButtonDown = false;
		}
	}

	void Input::ProcessInputMessage(int vkCode, bool isKeyUp, bool isKeyDown)
	{
		keyInput[vkCode].SetKeyUpDown(isKeyUp, isKeyDown);
	}

	void Input::SetMousePosition(const Vector2& position)
	{
		mousePosition.currentPosition = position;
	}

	void Input::SetPreviousMousePosition()
	{
		mousePosition.previousPosition = mousePosition.currentPosition;
	}

	void Input::SetMouseButtonState(int idx, bool down, bool up)
	{
		mouseInput[idx].SetButtonUpDown(down, up);
	}

	void Input::SetIsMouseClamped(bool isClamped)
	{
		isMouseClamped = isClamped;

		if (!isClamped)
		{
			ClipCursor(nullptr);
			ShowCursor(true);
		}
	}

	Vector2 Input::GetMousePosition() const
	{
		return mousePosition.currentPosition;
	}

	Vector2 Input::GetMouseDeltaPosition() const
	{
		return mousePosition.currentPosition - mousePosition.previousPosition;
	}

	bool Input::GetMouseButtonDown(int idx) const
	{
		return mouseInput[idx].isButtonDown;
	}

	bool Input::GetMouseButtonUp(int idx) const
	{
		return mouseInput[idx].isButtonUp;
	}

	bool Input::GetMouseButton(int idx) const
	{
		return mouseInput[idx].isButton;
	}
}
