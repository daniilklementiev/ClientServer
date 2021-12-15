#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>

#define CMD_START_SERVER	1001
#define CMD_STOP_SERVER		1002


HINSTANCE hInst;
HWND grpEndpoint, grpLog, serverLog;
HWND btnStart, btnStop;

LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);
DWORD	CALLBACK CreateUI(LPVOID);		// User Interface
DWORD	CALLBACK StartServer(LPVOID);
DWORD	CALLBACK StopServer(LPVOID);

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR cmdLine, _In_ int showMode) 
{
	hInst = hInstance;

	const WCHAR WIN_CLASS_NAME[] = L"ServerWindow";
	WNDCLASS wc = {};

	wc.lpfnWndProc = WinProc;
	wc.hInstance = hInst;
	wc.lpszClassName = WIN_CLASS_NAME;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	ATOM mainWin = RegisterClass(&wc);
	if (mainWin == FALSE) {
		MessageBoxW(NULL, L"Register class error", L"Launch error client", MB_OK | MB_ICONSTOP );
		return -1;
	}

	HWND hwnd = CreateWindowExW(0, WIN_CLASS_NAME, L"TCP Chat - Server", 
		WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, 0, 640, 480, 
		NULL, NULL, hInst, NULL);
	if (hwnd == FALSE) {
		MessageBoxW(NULL, L"Creating window error", L"Launch error client", MB_OK | MB_ICONSTOP);
		return -2;
	}
	ShowWindow(hwnd, showMode);
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return 0;
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE: {
		CreateUI(&hWnd);
		break;
	}
	case WM_COMMAND: {
		int cmd = LOWORD(wParam);
		int ntf = HIWORD(wParam);
		switch (cmd) {
		case CMD_START_SERVER: StartServer(&hWnd); break;
		case CMD_STOP_SERVER: StopServer(&hWnd); break;
		}
		break;
	}
	case WM_DESTROY: PostQuitMessage(0); break;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hWnd, &ps);
		FillRect(dc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_CTLCOLORSTATIC: {
		HDC dc = (HDC)wParam;
		HWND ctl = (HWND)lParam;
		if (ctl != grpEndpoint && ctl != grpLog) {
			SetBkMode(dc, TRANSPARENT);
			SetTextColor(dc, RGB(20, 20, 200));
		}
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

DWORD CALLBACK CreateUI(LPVOID params) {
	HWND hWnd = *((HWND*)params);
	grpEndpoint = CreateWindowExW(0, L"Button", L"EndPoint",
		BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
		10, 10, 150, 70, hWnd, 0, hInst, NULL);
	CreateWindowExW(0, L"Static", L"IP:", WS_CHILD | WS_VISIBLE,
		20, 35, 40, 15, hWnd, 0, hInst, NULL);
	CreateWindowExW(0, L"Edit", L"127.0.0.1", WS_CHILD | WS_VISIBLE,
		45, 35, 110, 15, hWnd, 0, hInst, NULL);
	CreateWindowExW(0, L"Static", L"Port:", WS_CHILD | WS_VISIBLE,
		20, 50, 40, 15, hWnd, 0, hInst, NULL);
	CreateWindowExW(0, L"Edit", L"8888", WS_CHILD | WS_VISIBLE,
		55, 50, 45, 15, hWnd, 0, hInst, NULL);

	grpLog = CreateWindowExW(0, L"Button", L"Server log",
		BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
		170, 10, 300, 300, hWnd, 0, hInst, NULL);
	serverLog = CreateWindowExW(0, L"Listbox", L"", WS_CHILD | WS_VISIBLE, 180, 30, 280, 280, hWnd, 0, hInst, NULL);

	btnStart = CreateWindowExW(0, L"Button", L"Start server", WS_CHILD | WS_VISIBLE,
		30, 90, 100, 25, hWnd, (HMENU)CMD_START_SERVER, hInst, NULL);
	btnStop = CreateWindowExW(0, L"Button", L"Stop server", WS_CHILD | WS_VISIBLE,
		30, 120, 100, 25, hWnd, (HMENU)CMD_STOP_SERVER, hInst, NULL);
	EnableWindow(btnStop, FALSE);

	return 0;
}

DWORD CALLBACK StartServer(LPVOID params) {
	HWND hWnd = *((HWND*)params);
	SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)L"Server start");
	EnableWindow(btnStart, FALSE);
	EnableWindow(btnStop, TRUE);
	return 0;
}
DWORD CALLBACK StopServer(LPVOID params) {
	HWND hWnd = *((HWND*)params);
	SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)L"Server stop");
	EnableWindow(btnStart, TRUE);
	EnableWindow(btnStop, FALSE);
	return 0;
}