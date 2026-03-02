#pragma once

#include <Windows.h>

namespace Engine
{
	class ScreenBuffer
	{
	public:
		ScreenBuffer();
		~ScreenBuffer();

		void Clear();

		void Draw(CHAR_INFO)
	};
}

