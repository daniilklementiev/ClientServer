#pragma comment (lib, "Ws2_32.lib")

#define _CRT_SECURE_NO_WARNINGS

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <wchar.h>
#include <stdio.h>
#include "resource.h"
#include "../Server/Server/Chatmessage.h"

#define CMD_SEND_MESSAGE		1001
#define CMD_SET_NAME			1002
#define CMD_RESET_NAME			1003

HINSTANCE hInst;
HWND grpEndpoint, grpLog, chatLog;
HWND btnSend, btnName, btnReset;
HWND editIP, editPort, editName, editMessage;

char name[128];

LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);
DWORD	CALLBACK CreateUI(LPVOID); // User interface
DWORD	CALLBACK SendChatMessage(LPVOID); // User interface
DWORD	CALLBACK SetName(LPVOID);

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR cmdLine, _In_ int showMode)
{
	hInst = hInstance;

	const WCHAR WIN_CLASS_NAME[] = L"ClientWindow";
	WNDCLASS wc = {};

	wc.lpfnWndProc = WinProc;
	wc.hInstance = hInst;
	wc.lpszClassName = WIN_CLASS_NAME;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	ATOM mainWin = RegisterClass(&wc);
	if (mainWin == FALSE) {
		MessageBoxW(NULL, L"Register class error", L"Launch error client", MB_OK | MB_ICONSTOP);
		return -1;
	}

	HWND hwnd = CreateWindowExW(0, WIN_CLASS_NAME, L"TCP Chat - Client",
		WS_OVERLAPPEDWINDOW,
		300, 300, 640, 480,
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
		case CMD_SEND_MESSAGE: {
			SendMessageW(btnSend, WM_KILLFOCUS, 0, 0);
			CreateThread(NULL, 0, SendChatMessage, &hWnd, 0, NULL);
			break;
		}
		case CMD_SET_NAME: {
			int nameLen = SendMessageA(editName, WM_GETTEXT, 128, (LPARAM)name);
			name[nameLen] = '\0';
			SendMessageA(editName, WM_GETTEXT, 128, (LPARAM)name);
			EnableWindow(btnName, FALSE);
			EnableWindow(editName, FALSE);
			break;
		}
		case CMD_RESET_NAME: {
			EnableWindow(btnName, TRUE);
			EnableWindow(editName, TRUE);
			break;
		}
		}
		break;
	}
	case WM_DESTROY: PostQuitMessage(0); break;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hWnd, &ps);
		//FillRect(dc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_CTLCOLORSTATIC: {
		HDC dc = (HDC)wParam;
		HWND ctl = (HWND)lParam;
		if (ctl != grpEndpoint && ctl != grpLog) {
			SetBkMode(dc, TRANSPARENT);
			SetTextColor(dc, RGB(255, 0, 0));
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
		10, 10, 150, 250, hWnd, 0, hInst, NULL);
	CreateWindowExW(0, L"Static", L"IP:", WS_CHILD | WS_VISIBLE,
		20, 35, 40, 15, hWnd, 0, hInst, NULL);
	editIP = CreateWindowExW(0, L"Edit", L"127.0.0.1", WS_CHILD | WS_VISIBLE,
		45, 35, 110, 15, hWnd, 0, hInst, NULL);
	CreateWindowExW(0, L"Static", L"Port:", WS_CHILD | WS_VISIBLE,
		20, 50, 40, 15, hWnd, 0, hInst, NULL);
	editPort = CreateWindowExW(0, L"Edit", L"8888", WS_CHILD | WS_VISIBLE,
		55, 50, 45, 15, hWnd, 0, hInst, NULL);

	grpLog = CreateWindowExW(0, L"Button", L"Chat",
		BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
		170, 10, 300, 300, hWnd, 0, hInst, NULL);
	chatLog = CreateWindowExW(0, L"Listbox", L"", WS_CHILD | WS_VISIBLE | LBS_DISABLENOSCROLL | WS_VSCROLL | WS_HSCROLL,
		180, 30, 280, 280, hWnd, 0, hInst, NULL);

	editMessage = CreateWindowExW(0, L"Edit", L"Message", WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_BORDER,
		20, 80, 130, 50, hWnd, 0, hInst, 0);

	editName = CreateWindowExW(0, L"Edit", L"Name", WS_CHILD | WS_VISIBLE | WS_BORDER,
		20, 140, 130, 20, hWnd, 0, hInst, 0);

	btnSend = CreateWindowExW(0, L"Button", L"Send", WS_CHILD | WS_VISIBLE,
		30, 230, 100, 25, hWnd, (HMENU)CMD_SEND_MESSAGE, hInst, NULL);

	btnName = CreateWindowExW(0, L"Button", L"Set Name", WS_CHILD | WS_VISIBLE,
		30, 165, 100, 25, hWnd, (HMENU)CMD_SET_NAME, hInst, NULL);

	btnReset = CreateWindowExW(0, L"Button", L"Reset Name", WS_CHILD | WS_VISIBLE,
		30, 190, 100, 25, hWnd, (HMENU)CMD_RESET_NAME, hInst, NULL);
	return 0;
}

DWORD CALLBACK SendChatMessage(LPVOID params) {
	HWND hWnd = *((HWND*)params);

	SOCKET clientSocket;
	const size_t MAX_LEN = 100;
	WCHAR str[MAX_LEN];

	WSADATA wsaData;
	int err;

	// WinSock API initializing (~ wsaData = new WSA(2.2) )
	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0) {
		_snwprintf_s(str, MAX_LEN,
			L"Startup failed, error %d", err);
		SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -10;
	}

	// Socket preparing
	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) {
		_snwprintf_s(str, MAX_LEN,
			L"Socket failed, error %d", WSAGetLastError());
		WSACleanup();
		SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -20;
	}

	// -- Socket configuration --
	SOCKADDR_IN addr;   // Config structure

	addr.sin_family = AF_INET;  // 1. Network type (family)

	char ip[20];
	LRESULT ipLen = SendMessageA(editIP, WM_GETTEXT, 19, (LPARAM)ip);
	ip[ipLen] = '\0';
	inet_pton(AF_INET, ip, &addr.sin_addr);  // 2. IP

	char port[8];
	LRESULT portLen = SendMessageA(editPort, WM_GETTEXT, 7, (LPARAM)port);
	port[portLen] = '\0';
	addr.sin_port = htons(atoi(port));  // 3. Port
	// -- end configuration of [addr] --

	// Connect to endpoint
	err = connect(clientSocket, (SOCKADDR*)&addr, sizeof(addr));
	if (err == SOCKET_ERROR) {
		_snwprintf_s(str, MAX_LEN,
			L"Socket connect error %d", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		clientSocket = INVALID_SOCKET;
		SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -30;
	}

	const size_t MSG_LEN = 512;
	const size_t NIK_LEN = 16;

	char chatMsg[MSG_LEN];
	char chatNik[NIK_LEN];

	int nikLen = SendMessageA(editName, WM_GETTEXT,
		NIK_LEN - 1, (LPARAM)chatNik);
	chatNik[nikLen] = '\0';

	int msgLen = SendMessageA(editMessage, WM_GETTEXT,
		MSG_LEN - NIK_LEN - 1, (LPARAM)chatMsg);
	chatMsg[msgLen] = '\0';

	strcat(chatMsg, "\t");
	strcat(chatMsg, chatNik);

	int sent = send(clientSocket, chatMsg, msgLen + nikLen + 2, 0);
	if (sent == SOCKET_ERROR) {
		_snwprintf_s(str, MAX_LEN,
			L"Sending error %d", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		clientSocket = INVALID_SOCKET;
		SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -40;
	}

	// receive in the same buffer - chatMsg
	int receivedCnt = recv(clientSocket, chatMsg, MSG_LEN - 1, 0);
	if (receivedCnt > 0) {
		chatMsg[receivedCnt] = '\0';
		ChatMessage *message = new ChatMessage();
		message->parseStringDT(chatMsg);
		if(message->parseStringDT(chatMsg))
			SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)message->toClientString());
		else
			SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)L"Error");
		
	}
	SendMessageW(chatLog, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), NULL);

	shutdown(clientSocket, SD_BOTH);
	closesocket(clientSocket);
	WSACleanup();

	// SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)L"-End-");
	return 0;
}