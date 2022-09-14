//====================================================================================================
// Filename:	Window.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#include "Precompiled.h"
#include "Window.h"

using namespace NFGE;
using namespace NFGE::Core;

LRESULT CALLBACK WinProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	}
	return DefWindowProcA(handle, message, wParam, lParam);
}

void NFGE::Core::Window::Initialize(HINSTANCE instance, LPCSTR appName, uint32_t width, uint32_t height, bool maximize, int icon)
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window 
	// to achieve 100% scaling while still allowing non-client window content to 
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	mInstance = instance;
	mAppName = appName;

	// Every WindowsWindow requires at least one window object. Three thinge are involved:
	// 1)	Register a window class (where we can specify the style of window we want)
	// 2)	Create a window instance (spawbubg the actual window)
	// 3)	Retrieve and dispatch messages for this window

	// Register class
	WNDCLASSEXA wcex;
	wcex.cbSize = sizeof(WNDCLASSEXA);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = &WinProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = instance;
	//wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wcex.hIcon = LoadIcon(instance, MAKEINTRESOURCE(icon));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = appName;
	//wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	wcex.hIconSm = LoadIcon(instance, MAKEINTRESOURCE(icon));

	RegisterClassExA(&wcex);

	// Compute the correct window dimension
	RECT rc = { 0, 0, (LONG)width, (LONG)height };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	// Compute the window position so its centered on screen
	const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	const int winWidth = rc.right - rc.left;
	const int winHeight = rc.bottom - rc.top;
	const int left = (screenWidth - winWidth) / 2;
	const int top = (screenHeight - winHeight) / 2;

	// Create window
	mWindow = CreateWindowA
	(
		appName, appName,
		WS_OVERLAPPEDWINDOW,
		left, top,
		winWidth, winHeight,
		nullptr, nullptr,
		instance, nullptr
	);

	ShowWindow(mWindow, maximize ? SW_MAXIMIZE : SW_SHOWNORMAL);

	SetCursorPos(screenWidth >> 1, screenHeight >> 1);

	GetWindowRect(mWindow, &mWindowRect);
}

void NFGE::Core::Window::Terminate()
{
	DestroyWindow(mWindow);

	UnregisterClassA(mAppName.c_str(), mInstance);

	mWindow = nullptr;
	mInstance = nullptr;
}

bool NFGE::Core::Window::ProcessMessage()
{
	MSG msg = { 0 };
	bool quit = false;
	while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg); // call the function pointer

		if (WM_QUIT == msg.message)
		{
			quit = true;
		}
	}

	return quit;
}
