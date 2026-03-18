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
	}

	void Input::ProcessInputMessage(int vkCode, bool isKeyUp, bool isKeyDown)
	{
		keyInput[vkCode].SetKeyUpDown(isKeyUp, isKeyDown);
	}
}
