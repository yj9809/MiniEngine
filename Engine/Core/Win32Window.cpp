#include "Win32Window.h"

#include "Input.h"

#include <windowsx.h>

namespace Engine
{
	Win32Window::Win32Window(uint32_t width, uint32_t height, const std::wstring& title)
		: width(width), height(height), title(title)
	{
		hInstance = GetModuleHandle(nullptr);
		Init();
	}

	Win32Window::~Win32Window()
	{
	}

	bool Win32Window::Init()
	{
		WNDCLASSEX wc = {};

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.lpfnWndProc = WndProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = className.c_str();

		if (!RegisterClassEx(&wc))
		{
			MessageBox(nullptr, L"Failed to register window class!", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		RECT rect = {};
		rect.left = 0;
		rect.top = 0;
		rect.right = width;
		rect.bottom = height;
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

		uint32_t windowWidth = rect.right - rect.left;
		uint32_t windowHeight = rect.bottom - rect.top;

		hwnd = CreateWindowEx(
			0,
			className.c_str(),
			title.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowWidth,
			windowHeight,
			nullptr,
			nullptr,
			hInstance,
			nullptr
		);

		if (!hwnd)
		{
			MessageBox(nullptr, L"Failed to create window!", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		ShowWindow(hwnd, SW_SHOW);
		UpdateWindow(hwnd);

		return true;
	}

	LRESULT Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		{
			bool isKeyUp = (lParam & ((int64_t)1 << 30)) != 0;
			bool isKeyDown = (lParam & ((int64_t)1 << 31)) == 0;

			if (isKeyDown != isKeyUp)
			{
				Input::Get().ProcessInputMessage(static_cast<int>(wParam), isKeyUp, isKeyDown);
			}
		}
		return 0;

		case WM_MOUSEMOVE:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			Input::Get().SetMousePosition(Vector2(static_cast<float>(x), static_cast<float>(y)));
		}
		return 0;

		case WM_LBUTTONDOWN:
		{
			Input::Get().SetMouseButtonState(0, true, false);
		}
		return 0;

		case WM_LBUTTONUP:
		{
			Input::Get().SetMouseButtonState(0, false, true);
		}
		return 0;

		case WM_RBUTTONDOWN:
		{
			Input::Get().SetMouseButtonState(1, true, false);
		}
		return 0;

		case WM_RBUTTONUP:
		{
			Input::Get().SetMouseButtonState(1, false, true);
		}
		return 0;

		case WM_MBUTTONDOWN:
		{
			Input::Get().SetMouseButtonState(2, true, false);
		}
		return 0;

		case WM_MBUTTONUP:
		{
			Input::Get().SetMouseButtonState(2, false, true);
		}
		return 0;

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}

	RECT Win32Window::GetWindowRect() const
	{
		RECT r = {};
		if (GetClientRect(hwnd, &r))
		{
			POINT topLeft = { r.left, r.top };
			POINT bottomRight = { r.right, r.bottom };
			ClientToScreen(hwnd, &topLeft);
			ClientToScreen(hwnd, &bottomRight);
			r.left = topLeft.x;
			r.top = topLeft.y;
			r.right = bottomRight.x;
			r.bottom = bottomRight.y;
			return r;
		}

		return r;
	}
}