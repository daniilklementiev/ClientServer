#pragma comment (lib, "Ws2_32.lib")

#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <wchar.h>
#include <stdio.h>

#define CMD_START_SERVER	1001
#define CMD_STOP_SERVER		1002


HINSTANCE hInst;
HWND grpEndpoint, grpLog, serverLog;
HWND btnStart, btnStop;
HWND editIP, editPort;
SOCKET listenSocket;


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
		940, 300, 640, 480, 
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
		case CMD_START_SERVER: {
			SendMessageW(btnStart, WM_KILLFOCUS, 0, 0);
			CreateThread(NULL, 0, StartServer, &hWnd, 0, NULL); 
			break;
		}
		case CMD_STOP_SERVER: {
			SendMessageW(btnStop, WM_KILLFOCUS, 0, 0);
			StopServer(&hWnd); 
			break;
		}
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
	editIP = CreateWindowExW(0, L"Edit", L"91.228.59.1", WS_CHILD | WS_VISIBLE,
		45, 35, 110, 15, hWnd, 0, hInst, NULL);
	CreateWindowExW(0, L"Static", L"Port:", WS_CHILD | WS_VISIBLE,
		20, 50, 40, 15, hWnd, 0, hInst, NULL);
	editPort = CreateWindowExW(0, L"Edit", L"8888", WS_CHILD | WS_VISIBLE,
		55, 50, 45, 15, hWnd, 0, hInst, NULL);

	grpLog = CreateWindowExW(0, L"Button", L"Server log",
		BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
		170, 10, 300, 300, hWnd, 0, hInst, NULL);
	serverLog = CreateWindowExW(0, L"Listbox", L"", WS_CHILD | WS_VISIBLE | LBS_DISABLENOSCROLL | WS_VSCROLL | WS_HSCROLL, 180, 30, 280, 280, hWnd, 0, hInst, NULL);

	btnStart = CreateWindowExW(0, L"Button", L"Start server", WS_CHILD | WS_VISIBLE,
		30, 90, 100, 25, hWnd, (HMENU)CMD_START_SERVER, hInst, NULL);
	btnStop = CreateWindowExW(0, L"Button", L"Stop server", WS_CHILD | WS_VISIBLE,
		30, 120, 100, 25, hWnd, (HMENU)CMD_STOP_SERVER, hInst, NULL);
	EnableWindow(btnStop, FALSE);

	listenSocket = INVALID_SOCKET;

	return 0;
}

DWORD CALLBACK StartServer(LPVOID params) {
	HWND hWnd = *((HWND*)params);
	const size_t MAX_LEN = 100;
	WCHAR str[MAX_LEN];

	SYSTEMTIME  time;
	GetLocalTime(&time);

	WSADATA wsaData;
	int err;
	// WinSock API initializing (~ wsaData = new WSA(2.2) )
	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0)
	{
		_snwprintf_s(str, MAX_LEN, L"Startup failed, error %d", err);
		WSACleanup();
		SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -10;
	}

	// Socket prepairing
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		_snwprintf_s(str, MAX_LEN, L"Socket failed, error %d", WSAGetLastError());
		WSACleanup();
		SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -20;
	}
	// Socket configuration
	SOCKADDR_IN addr; // Config socket
	addr.sin_family = AF_INET; // 1. Network type (family)

	char ip[20];
	LRESULT ipLen = SendMessageA(editIP, WM_GETTEXT, 19, (LPARAM)ip);
	ip[ipLen] = '\0';
	inet_pton(AF_INET, ip, &addr.sin_addr); // 2. IP

	char port[8];
	LRESULT portLen = SendMessageA(editPort, WM_GETTEXT, 7, (LPARAM)port);
	port[portLen] = '\0';
	addr.sin_port = htons(atoi(port)); // 3. Port
	// --- end configuration of [addr] ---

	int lasetError = WSAGetLastError();

	// Socket binding - config applying to socket
	err = bind(listenSocket, (SOCKADDR*)&addr, sizeof(addr));
	if (err == SOCKET_ERROR)
	{
		_snwprintf_s(str, MAX_LEN, L"Socket error #%d", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		listenSocket = INVALID_SOCKET;
		SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -30;
	}
	if (lasetError == 10049)
	{
		_snwprintf_s(str, MAX_LEN, L"Wrong port or IP, error %d", lasetError);
		closesocket(listenSocket);
		WSACleanup();
		listenSocket = INVALID_SOCKET;
		SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -50;
	}

	// Start of listening - from this point Socket receives data from OS
	err = listen(listenSocket, SOMAXCONN);
	if (err == SOCKET_ERROR)
	{
		_snwprintf_s(str, MAX_LEN, L"Socket listen, error %d", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		listenSocket = INVALID_SOCKET;
		SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -40;
	}
	const size_t ipSize = strlen(ip) + 1;
	wchar_t* WIP = new wchar_t[ipSize];
	mbstowcs(WIP, ip, ipSize);

	const size_t portSize = strlen(port) + 1;
	wchar_t* WPORT = new wchar_t[portSize];
	mbstowcs(WPORT, port, portSize);

	// Log start message
	_snwprintf_s(str, MAX_LEN, L"Server start : %s:%s", WIP, WPORT);
	SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
	EnableWindow(btnStart, FALSE);
	EnableWindow(btnStop, TRUE);
	
	// Listening loop
	SOCKET acceptSocket; // second socket - for communication
	
	const size_t BUFF_LEN = 8;
	const size_t DATA_LEN = 2048;
	char buff[BUFF_LEN + 1]; // small buffer for transfered chunk (+ '\0')
	char data[DATA_LEN]; // bif buffer for all transfered chunks
	int receivedCnt; // chunk size


	while (true) {
		// wait for network activity
		acceptSocket = accept(listenSocket, NULL, NULL);
		if (acceptSocket == INVALID_SOCKET)
		{
			_snwprintf_s(str, MAX_LEN,
				L"Accept socket error %d", WSAGetLastError());
			closesocket(acceptSocket);
			SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
			return -70;
		}
		// from this point communication begins
		data[0] = '\0';
		
		do {
			receivedCnt = recv(acceptSocket, buff, BUFF_LEN, 0);
			if (receivedCnt == 0) { // 0 - connection closed by client
				closesocket(acceptSocket);
				SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)L"Connection closed");
				break;
			}
			if (receivedCnt < 0) { // receiving error
				_snwprintf_s(str, MAX_LEN, L"Communication socket error %d", WSAGetLastError());
				closesocket(acceptSocket);
				SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
				break;
			}
			buff[receivedCnt] = '\0';
			strcat_s(data, buff);	// data += chunk (buff)
		} while (strlen(buff) == BUFF_LEN); // '\0' - end of data 
		// data is sum of all chunks from socket
		

		_snprintf_s(data, DATA_LEN, DATA_LEN, "%s [%d:%d:%d.%d]", data, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
		SendMessageA(serverLog, LB_ADDSTRING, 0, (LPARAM)data);

		// send answer to client - write in socket
		// send(acceptSocket, "200", 4, 0);

		// closing socket
		shutdown(acceptSocket, SD_BOTH);
		closesocket(acceptSocket);

	}


	delete[] WPORT;
	delete[] WIP;
	return 0;
}

DWORD CALLBACK StopServer(LPVOID params) {
	HWND hWnd = *((HWND*)params);
	closesocket(listenSocket);
	WSACleanup();
	listenSocket = INVALID_SOCKET;

	SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)L"Server stop");
	EnableWindow(btnStart, TRUE);
	EnableWindow(btnStop, FALSE);
	return 0;
}