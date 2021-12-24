#pragma comment (lib, "Ws2_32.lib")

#define _CRT_SECURE_NO_WARNINGS

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <wchar.h>
#include <stdio.h>
#include <list>
#include "resource.h"
#include "../Server/Server/Chatmessage.h"

#define CMD_SEND_MESSAGE		1001
#define CMD_SET_NAME			1002
#define CMD_RESET_NAME			1003
#define CMD_BUTTON_AUTH			1004
#define CMD_BUTTON_DISC			1005
#define SYNC_TIMER_MESSAGE	    2001

const size_t MSG_LEN = 4096;
const size_t NIK_LEN = 16;

HINSTANCE hInst;
HWND grpEndpoint, grpLog, chatLog;
HWND btnSend, btnName, btnReset, btnAuth, btnDscn;
HWND editIP, editPort, editName, editMessage;
HANDLE sendLock = NULL;


char chatMsg[MSG_LEN];
char chatNik[NIK_LEN];


std::list<ChatMessage>* msg = new std::list<ChatMessage>;
std::list<ChatMessage>* names = new std::list<ChatMessage>;
char name[128];
bool isConnected = false;

LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);
DWORD	CALLBACK CreateUI(LPVOID);			// User interface
DWORD	CALLBACK SyncChatMessage(LPVOID);	//
DWORD	CALLBACK SendChatMessage(LPVOID);	// 
DWORD	CALLBACK SendToServer(LPVOID);		// 
DWORD	CALLBACK SetName(LPVOID);
DWORD	CALLBACK JoinServerClick(LPVOID);
DWORD CALLBACK LeaveFromServer(LPVOID);
bool			 DeserializeMessage(char*);


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
		sendLock = CreateMutex(NULL, FALSE, NULL);
		if (sendLock == NULL)
		{
			MessageBoxW(NULL, L"Mutex not created", L"APP STOPPED", MB_ICONWARNING | MB_OK);
			PostQuitMessage(0);
			return 0;
		}
		break;
	}
	case WM_COMMAND: {
		int cmd = LOWORD(wParam);
		int ntf = HIWORD(wParam);
		switch (cmd) {
		case CMD_SEND_MESSAGE: {
			SendMessageW(btnSend, WM_KILLFOCUS, 0, 0);
			SendMessageW(chatLog, LB_RESETCONTENT, 0, 0);
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
		case CMD_BUTTON_AUTH: {
			if (isConnected == false) CreateThread(NULL, 0, JoinServerClick, &hWnd, 0, NULL); 
			
			else CreateThread(NULL, 0, LeaveFromServer, &hWnd, 0, NULL); 
			break;
		}
		
		}
		break;
	}
	case WM_TIMER: {
		if (wParam == SYNC_TIMER_MESSAGE) {
			CreateThread(NULL, 0, SyncChatMessage, &hWnd, 0, NULL);
		}
		break;
	}
	case WM_DESTROY: {
		KillTimer(hWnd, SYNC_TIMER_MESSAGE);
		CloseHandle(sendLock);
		PostQuitMessage(0);
		break;
	}
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
		10, 10, 150, 300, hWnd, 0, hInst, NULL);
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
		20, 110, 120, 50, hWnd, 0, hInst, 0);

	editName = CreateWindowExW(0, L"Edit", L"Login", WS_CHILD | WS_VISIBLE | WS_BORDER,
		20, 170, 130, 20, hWnd, 0, hInst, 0);

	btnSend = CreateWindowExW(0, L"Button", L"Send", WS_CHILD | WS_VISIBLE,
		30, 270, 100, 25, hWnd, (HMENU)CMD_SEND_MESSAGE, hInst, NULL);

	btnName = CreateWindowExW(0, L"Button", L"Set Name", WS_CHILD | WS_VISIBLE,
		30, 230, 100, 25, hWnd, (HMENU)CMD_SET_NAME, hInst, NULL);

	btnReset = CreateWindowExW(0, L"Button", L"Reset Name", WS_CHILD | WS_VISIBLE,
		30, 200, 100, 25, hWnd, (HMENU)CMD_RESET_NAME, hInst, NULL);

	btnAuth = CreateWindowExW(0, L"Button", L"->", WS_CHILD | WS_VISIBLE,
		45, 75, 75, 23, hWnd, (HMENU)CMD_BUTTON_AUTH, hInst, NULL);
	SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)L"Click \"Auth\" to get started");
	return 0;
}

DWORD CALLBACK SendChatMessage(LPVOID params) {
	HWND hWnd = *((HWND*)params);
	WaitForSingleObject(sendLock, INFINITE);
	int nikLen = SendMessageA(editName, WM_GETTEXT,
		NIK_LEN - 1, (LPARAM)chatNik);
	chatNik[nikLen] = '\0';

	int msgLen = SendMessageA(editMessage, WM_GETTEXT,
		MSG_LEN - NIK_LEN - 1, (LPARAM)chatMsg);
	chatMsg[msgLen] = '\0';

	strcat(chatMsg, "\t");
	strcat(chatMsg, chatNik);

	if (SendToServer(chatMsg) > 0)
		DeserializeMessage(chatMsg);
	else {
		SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)"Send error");
	}
	ReleaseMutex(sendLock);
	return 0;
}

DWORD	CALLBACK SyncChatMessage(LPVOID params) {
	HWND hWnd = *((HWND*)params);
	WaitForSingleObject(sendLock, INFINITE);
	chatMsg[0] = '\0';
	if (SendToServer(chatMsg) > 0)
		DeserializeMessage(chatMsg);
	else {
		SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)"Sync error");
	}
	ReleaseMutex(sendLock);
	return 0;
}

bool DeserializeMessage(char* str) {
	WaitForSingleObject(sendLock, INFINITE);
	if (str == NULL) {
		return false;
	}
	size_t len = 0;
	char* start = str;
	std::list<ChatMessage>* newMsg = new std::list<ChatMessage>;
	bool isParsing = (str[len] != '\0');
	msg->clear();
	SendMessageA(chatLog, LB_RESETCONTENT, 0, 0);
	while (isParsing) {
		if (str[len] == '\r' || str[len] == '\0') {
			if (str[len] == '\0') isParsing = false;
			str[len] = '\0';
			ChatMessage m;
			if (m.parseStringDT(start)) {

				msg->push_back(m);
				SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)m.toClientString());
			}
			else SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)"Message parse error");
			start = str + len + 1;
		}
		len += 1;
	}
	ChatMessage n;
	n.parseStringDT(start);
	newMsg->push_back(n);
	bool msgflag = false;

	for (auto it1 = newMsg->begin(); it1!= newMsg->end(); it1++) {
		for (auto it2 = msg->begin(); it2 != msg->end(); it2++) {
			if (it2->getId() == it1->getId()) msgflag = true;
		}
		if (!msgflag) SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)it1->toClientString());
		msgflag = false;
	}
	delete msg;
	msg = newMsg;
	
	ReleaseMutex(sendLock);
	SendMessageW(chatLog, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), NULL);
	return true;
}

/*bool DeserializeMessage(char* str) {
   if (str == NULL) {
	   return false;
   }
   size_t len = 0, r = 0;
   char* start = str;
   msg.clear();
   SendMessageA(chatLog, LB_RESETCONTENT, 0, 0);

   while (str[len] != '\0') {
	   if (str[len] == '\r') {
		   r += 1;
		   str[len] = '\0';
		   ChatMessage m;
		   if (m.parseStringDT(start)) {
			   msg.push_back(m);
			   SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)m.toClientString());
		   }
		   else SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)"Message parse error");
		   start = str + len + 1;
	   }
	   len += 1;
   }
   if (len > r) {
	   ChatMessage m;
	   if (m.parseStringDT(start)) {
		   msg.push_back(m);
		   SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)m.toClientString());
	   }
	   else SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)"Message parse error");
   }
   SendMessageW(chatLog, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), NULL);
   return true;
}
*/

DWORD CALLBACK SendToServer(LPVOID params) {

	char* data = (char*)params;
	if (data == NULL) return -1;
	SOCKET clientSocket;
	const size_t MAX_LEN = 100;
	WCHAR str[MAX_LEN];

	WSADATA wsaData;
	int err;
	// look mutex
	WaitForSingleObject(sendLock, INFINITE);
	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0) {
		_snwprintf_s(str, MAX_LEN, L"Startup failed, error %d", err);
		SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -10;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) {
		_snwprintf_s(str, MAX_LEN, L"Socket failed, error %d", WSAGetLastError());
		WSACleanup();
		SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -20;
	}

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;

	char ip[20];
	LRESULT ipLen = SendMessageA(editIP, WM_GETTEXT, 19, (LPARAM)ip);
	ip[ipLen] = '\0';
	inet_pton(AF_INET, ip, &addr.sin_addr);

	char port[8];
	LRESULT portLen = SendMessageA(editPort, WM_GETTEXT, 7, (LPARAM)port);
	port[portLen] = '\0';
	addr.sin_port = htons(atoi(port));

	err = connect(clientSocket, (SOCKADDR*)&addr, sizeof(addr));
	int error = WSAGetLastError();
	if (err == SOCKET_ERROR) {
		if (error == 10061) {
			return -80;
		}
		else {
			_snwprintf_s(str, MAX_LEN, L"Socket connect error %d", WSAGetLastError());
			closesocket(clientSocket);
			WSACleanup();
			clientSocket = INVALID_SOCKET;
			SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)str);
			return -30;
		}
	}

	int sent = send(clientSocket, data, strlen(data) + 1, 0);
	if (sent == SOCKET_ERROR) {
		_snwprintf_s(str, MAX_LEN, L"Sending error %d", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		clientSocket = INVALID_SOCKET;
		SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -40;
	}

	int receivedCnt = recv(clientSocket, chatMsg, MSG_LEN - 1, 0);
	if (receivedCnt > 0) {
		chatMsg[receivedCnt] = '\0';
	}
	else {
		chatMsg[0] = '\0';

	}
	shutdown(clientSocket, SD_BOTH);
	closesocket(clientSocket);
	WSACleanup();
	// Unlock mutex
	ReleaseMutex(sendLock);
	return receivedCnt;
}

DWORD CALLBACK JoinServerClick(LPVOID params) {
	HWND hWnd = *((HWND*)params);
	isConnected = true;
	chatNik[0] = '\b';
	int nickLen = SendMessageA(editName, WM_GETTEXT, NIK_LEN - 1, (LPARAM)(chatNik + 1));
	chatNik[nickLen + 1] = '\0';
	int res = (int)SendToServer(chatNik);
	
	if (res < 0) {  // error
		SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)"Join error");
	}
	else {
		// server response (201/401) stored in chatMsg
		if (chatMsg[0] == '2') { // Accepted
			SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)L"Accepted");
			SetTimer(hWnd, SYNC_TIMER_MESSAGE, 1000, NULL);
			EnableWindow(btnSend, TRUE);
			EnableWindow(btnName, FALSE);
			EnableWindow(editName, FALSE);
			EnableWindow(btnReset, FALSE);
			SendMessageW(btnAuth, WM_SETTEXT, 0, (LPARAM)L"<-");
		}
		else {	// Restricted
			SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)L"Restricted");
		}
	}
	return 0;
}

DWORD CALLBACK LeaveFromServer(LPVOID params) {
	isConnected = false;
	HWND hWnd = *((HWND*)params);
	chatNik[0] = '\a';
	int nickLen = SendMessageA(editName, WM_GETTEXT, NIK_LEN - 1, (LPARAM)(chatNik + 1));
	chatNik[nickLen + 1] = '\0';
	int res = (int)SendToServer(chatNik);
	if (res < 0) {  // error
		SendMessageA(chatLog, LB_ADDSTRING, 0, (LPARAM)"Join error");
	}
	else {
		// server response (201/401) stored in chatMsg
		if (chatMsg[0] == '2') { // Accepted
			SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)L"Disconnected");
			KillTimer(hWnd, SYNC_TIMER_MESSAGE);
			EnableWindow(btnSend, FALSE);
			EnableWindow(editName, TRUE);
			EnableWindow(btnName, TRUE);
			EnableWindow(btnReset, TRUE);
			SendMessageW(btnAuth, WM_SETTEXT, 0, (LPARAM)L"->");
		}
		else {	// Restricted
			SendMessageW(chatLog, LB_ADDSTRING, 0, (LPARAM)L"Server error");
		}
	}
	return 0;
}