// PopupMenu.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.

//фон в PopupMenu

#include <Windows.h>

LRESULT CALLBACK WindowProcXLL(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) {
	case WM_COMMAND: {

		// ! создаем PopupMenu и закрашиваем фон (но не закрашивается левая область под иконку) !
		HMENU m_hMenu = CreatePopupMenu();
		HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		MENUINFO mi = {0};
		mi.cbSize = sizeof(MENUINFO);
		mi.fMask = MIM_BACKGROUND;
		mi.hbrBack = hBrush;
		SetMenuInfo(m_hMenu, &mi);

		InsertMenuW(m_hMenu, 0, MF_BYCOMMAND | MF_STRING | MF_ENABLED, 1, L"✚ Добавить столбец");

		POINT pointCursor = { 0 };
		GetCursorPos(&pointCursor);
		TrackPopupMenuEx(m_hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pointCursor.x, pointCursor.y, hWnd, NULL);

		DestroyMenu(m_hMenu);

		return 0;
	}
	break;
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
	break;
	}
	return DefWindowProc(hWnd, Message, wParam, lParam);

}

int main()
{
	HWND hMainWnd = NULL;
	HINSTANCE hInstance = NULL;

	const wchar_t* szClassName = L"ClassWindowsXLL";
	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProcXLL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (!RegisterClassExW(&wc)) { 
		return -1; 
	}

	hMainWnd = CreateWindowExW(0, szClassName, L"ClassWindowsXLL", WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW & (~WS_MINIMIZEBOX), 100, 100, 400, 200, NULL, NULL, hInstance, NULL);
	CreateWindowExW(0, L"Button", L"CreatePopupMenu", WS_CHILD | WS_VISIBLE, 50, 50, 150, 50, hMainWnd, HMENU(333), hInstance, NULL);

	ShowWindow(hMainWnd, SW_NORMAL);
	UpdateWindow(hMainWnd);

	MSG msg;
	BOOL bRet = false;
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (bRet == -1) {
			break;
		}
		else {
			if (!IsWindow(hMainWnd) || !IsDialogMessageW(hMainWnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
	}
	UnregisterClassW(szClassName, hInstance);

	return 0;
}