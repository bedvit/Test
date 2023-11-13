// Test.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <Windows.h>
#include <unordered_map>
#include <stdexcept>//исключения
#include <functional>

class UserFormXLL
{
public:
	UserFormXLL(std::function<int(HWND, UINT, WPARAM, LPARAM)>);
};
//std::unordered_map<HWND, int (*)(HWND, UINT, WPARAM, LPARAM)> mapFF;
std::unordered_map<HWND, std::function<int(HWND, UINT, WPARAM, LPARAM)>> mapFF;


LRESULT CALLBACK WindowProcXLL(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message){
	case WM_COMMAND:{
		try {
			//int (*funcPtr)(HWND, UINT, WPARAM, LPARAM) = mapFF[hWnd];
			int result = mapFF[hWnd](hWnd, Message, wParam, lParam);
			return 0;
		}
		catch (...) {
			return 0;
		}
	}
	break;	
	case WM_DESTROY:{
		PostQuitMessage(0);
		mapFF.erase(hWnd);
		return 0;
	}
	break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
	}
	return 0;
}
UserFormXLL::UserFormXLL(std::function<int(HWND, UINT, WPARAM, LPARAM)> funcPtr)
{
	HWND hMainWnd = 0;
	HINSTANCE hInstance = NULL;

	int nCmdShow = 1;
	const wchar_t* szClassName = L"ClassWindowsXLL";
	WNDCLASSEX wc;
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProcXLL;//!!!
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = HBRUSH(COLOR_WINDOW + 1);// (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (!RegisterClassExW(&wc)) { throw std::runtime_error("UserFormXLL->Cannot register class"); MessageBox(NULL, L"Cannot register class", L"Error", MB_OK | MB_TOPMOST); return; }


	hMainWnd = CreateWindowExW(0, szClassName, L"ClassWindowsXLL",WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW & (~WS_MINIMIZEBOX), 100, 100, 500, 500, NULL, NULL, hInstance, NULL);//eWindowEx(WS_EX_APPWINDOW, szClassName, lpWindowName, WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW | WS_VSCROLL| WS_THICKFRAME, (GetSystemMetrics(SM_CXSCREEN) >> 1) - (fWidth >> 1), (GetSystemMetrics(SM_CYSCREEN) >> 1) - (fHeight >> 1), fWidth, fHeight, HwndExcel(), NULL, hInstance, NULL);//HwndExcel()
	mapFF.insert({ hMainWnd, funcPtr });

	CreateWindowExW(0, L"Button", L"Button", WS_CLIPSIBLINGS | WS_CHILD | WS_VISIBLE | BS_FLAT | BS_PUSHBUTTON | BS_MULTILINE, 1, 1, 200, 200, hMainWnd, HMENU(333), hInstance, NULL);

	ShowWindow(hMainWnd, nCmdShow);
	UpdateWindow(hMainWnd);

	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (bRet == -1) {
			// handle the error and possibly exit
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	UnregisterClassW(szClassName, hInstance);
	return;
}

int main() {

	const wchar_t* hello = L"Hello, World!";//как пробросить локальную переменную в лямбду

	UserFormXLL f(
		[hello](HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
		{
			switch (Message) {
				case WM_COMMAND: {
					switch (LOWORD(wParam)) {
						case 333: {
							//const wchar_t* hello = L"Hello, World!";
							MessageBoxW(hWnd, hello, L"ClassWindowsXLL", MB_ICONINFORMATION);
							return 0;
						}
				
					}
				}
			}
			return 0;
		});
	return 0;
}











