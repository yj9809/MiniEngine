#include "Input.h"

#include <Windows.h>

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

	void Input::Update()
	{
		for (int i = 0; i < 256; i++)
		{
			keyInput[i].previous = keyInput[i].current;
			keyInput[i].current = (GetAsyncKeyState(i) & 0x8000) != 0;
		}
	}

	bool Input::GetKeyDown(int key) const
	{
		return keyInput[key].current && !keyInput[key].previous;
	}

	bool Input::GetKey(int key) const
	{
		return keyInput[key].current && keyInput[key].previous;
	}

	bool Input::GetKeyUp(int key) const
	{
		return !keyInput[key].current && keyInput[key].previous;
	}
}