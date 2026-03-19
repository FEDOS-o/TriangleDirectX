#pragma once

#include "Game.h"

class DisplayWin32 {
private:
	LONG ClientHeight, ClientWidth;

	WNDCLASSEX Wc;

public:
	HWND Window;

public:
	DisplayWin32(LONG clientWidth, LONG clientHeight, HINSTANCE instance, LPCWSTR applicationName) :
		ClientHeight(clientHeight),
		ClientWidth(clientWidth) {
		Wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		Wc.lpfnWndProc = WndProc;
		Wc.cbClsExtra = 0;
		Wc.cbWndExtra = 0;
		Wc.hInstance = instance;
		Wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
		Wc.hIconSm = Wc.hIcon;
		Wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		Wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
		Wc.lpszMenuName = nullptr;
		Wc.lpszClassName = applicationName;
		Wc.cbSize = sizeof(WNDCLASSEX);

		// Register the window class.
		RegisterClassEx(&Wc);


		RECT windowRect = { 0, 0, static_cast<LONG>(ClientWidth), static_cast<LONG>(ClientHeight) };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		auto dwStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_THICKFRAME;

		auto posX = (GetSystemMetrics(SM_CXSCREEN) - ClientWidth) / 2;
		auto posY = (GetSystemMetrics(SM_CYSCREEN) - ClientHeight) / 2;

		Window = CreateWindowEx(
			WS_EX_APPWINDOW,
			applicationName,
			applicationName,
			dwStyle,
			posX, posY,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr,
			nullptr,
			instance,
			nullptr);

		ShowWindow(Window, SW_SHOW);
		SetForegroundWindow(Window);
		SetFocus(Window);

		ShowCursor(true);
	}

	void createMessageBox(LPCWSTR text, LPCWSTR caption, UINT type) {
		MessageBox(Window, text, caption, type);
	}

	void setWindowText(LPCWSTR text) {
		SetWindowText(Window, text);
	}

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
	{
		switch (umessage)
		{
		case WM_KEYDOWN:
		{
			// If a key is pressed send it to the input object so it can record that state.
			std::cout << "Key: " << static_cast<unsigned int>(wparam) << std::endl;
			return 0;
		}
		default:
		{
			return DefWindowProc(hwnd, umessage, wparam, lparam);
		}
		}
	}
};
