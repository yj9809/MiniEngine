#pragma once

#include "Common/Common.h"

namespace Engine
{
	class ENGINE_API Input
	{
	private:
		struct KeyState
		{
			bool current;

			bool previous;
		};

	public:
		Input();
		~Input();

		void Update();

		bool GetKeyDown(int key) const;
		bool GetKey(int key) const;
		bool GetKeyUp(int key) const;


		static inline Input& Get() { return *instance; }

	private:
		KeyState keyInput[256] = { KeyState{false, false} };

		static Input* instance;
	};
}
