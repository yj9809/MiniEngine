#pragma once

#include <Windows.h>
#include <string>

namespace Engine
{
	class Win32Window
	{
	public:
		Win32Window(uint32_t width, uint32_t height, const std::wstring& title);
		~Win32Window();

		bool Init();

		static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		// Getter.
		RECT GetWindowRect() const;
	private:
		std::wstring title;

		std::wstring className = L"MiniEngineWindow";

		uint32_t width = 0;
		uint32_t height = 0;

		HWND hwnd = nullptr;

		HINSTANCE hInstance = nullptr;
	};
}

