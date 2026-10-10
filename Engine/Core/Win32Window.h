#pragma once

#include <Windows.h>
#include <string>

namespace Engine
{
	class Win32Window
	{
	public:
		Win32Window(uint32_t width, uint32_t height, const std::wstring& title);
		~Win32Window() noexcept;

		Win32Window(const Win32Window&) = delete;
		Win32Window& operator=(const Win32Window&) = delete;
		Win32Window(Win32Window&&) = delete;
		Win32Window& operator=(Win32Window&&) = delete;

		// Getter.
		RECT GetWindowRect() const;

		inline HWND GetHwnd() const { return hwnd; }

	private:
		bool Init();
		void Shutdown() noexcept;

		static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		
	private:
		std::wstring title;

		std::wstring className = L"MiniEngineWindow";

		uint32_t width = 0;
		uint32_t height = 0;

		HINSTANCE hInstance = nullptr;

		ATOM registeredClass = 0;
		HWND hwnd = nullptr;
	};
}

