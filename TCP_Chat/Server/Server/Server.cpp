#pragma comment (lib, "Ws2_32.lib")

#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "resource.h"
#include <list>
#include "Chatmessage.h"

#define CMD_START_SERVER	1001
#define CMD_STOP_SERVER		1002
#define MAX_COUNT_MESSAGES  100

HINSTANCE hInst;
HWND grpEndpoint, grpLog, serverLog;
HWND btnStart, btnStop;
HWND editIP, editPort;
SOCKET listenSocket;
long long mId;
bool start = true;

LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);
DWORD	CALLBACK CreateUI(LPVOID);		// User Interface
DWORD	CALLBACK StartServer(LPVOID);
DWORD	CALLBACK StopServer(LPVOID);
char* SerializeMessages();


std::list<ChatMessage*> mes_buf;
std::list<char*> usersList;

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR cmdLine, _In_ int showMode)
{

	hInst = hInstance;

	const WCHAR WIN_CLASS_NAME[] = L"ServerWindow";
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
		mId = time(NULL);
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
	editIP = CreateWindowExW(0, L"Edit", L"127.0.0.1", WS_CHILD | WS_VISIBLE,
		45, 35, 110, 15, hWnd, 0, hInst, NULL);
	CreateWindowExW(0, L"Static", L"Port:", WS_CHILD | WS_VISIBLE,
		20, 50, 40, 15, hWnd, 0, hInst, NULL);
	editPort = CreateWindowExW(0, L"Edit", L"8888", WS_CHILD | WS_VISIBLE,
		55, 50, 45, 15, hWnd, 0, hInst, NULL);

	grpLog = CreateWindowExW(0, L"Button", L"Server log",
		BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
		170, 10, 300, 300, hWnd, 0, hInst, NULL);
	serverLog = CreateWindowExW(0, L"Listbox", L"", WS_CHILD | WS_VISIBLE | LBS_DISABLENOSCROLL | WS_VSCROLL | WS_HSCROLL,
		180, 30, 280, 280, hWnd, 0, hInst, NULL);

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
	mes_buf.clear();
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

		char* mts;
		if (strlen(data) == 0) {	// only \0 in request
			mts = SerializeMessages();
			send(acceptSocket, mts, strlen(mts) + 1, 0);
			delete[] mts;
		}
		else {
			// \b at first place - AUTH command
			if (data[0] == '\b') {
				data[0] = '+';
				char* username = new char[16];
				int len = strlen(data);
				bool usernameFree = true;
				for (size_t i = 1; i < len; i++)
				{
					username[i - 1] = data[i];
				}
				username[len - 1] = '\0';

				if (usersList.size() == 0) {
					usersList.push_back(username);
					SendMessageA(serverLog, LB_ADDSTRING, 0, (LPARAM)data);
					send(acceptSocket, "201", 4, 0);
				}
				else {
					for (auto it : usersList) {
						if (strcmp(username, it) == 0) {
							usernameFree = false;
						}
					}
					if (usernameFree) {
						SendMessageA(serverLog, LB_ADDSTRING, 0, (LPARAM)data);
						send(acceptSocket, "201", 4, 0);
						usersList.push_back(username);
					}
					else {
						SendMessageA(serverLog, LB_ADDSTRING, 0, (LPARAM)"Trying to use an existing username");
						send(acceptSocket, "401", 4, 0);
					}
				}
			}
			// \a at first place AUTH command (remove username)
			else if (data[0] == '\a') {
				data[0] = '-';
				char* username = new char[16];
				int len = strlen(data);
				for (size_t i = 1; i < len; i++)
				{
					username[i - 1] = data[i];
				}
				username[len - 1] = '\0';
				std::list<char*>::iterator it = usersList.begin();
				for (; it != usersList.end(); it++) {
					if (strcmp(username, *it) == 0) {
						break;
					}
				}
				usersList.erase(it);
				SendMessageA(serverLog, LB_ADDSTRING, 0, (LPARAM)data);
				send(acceptSocket, "200", 4, 0);
				delete[] username;
			}
			else if (data[0] == '\f') {
				char* id = new char[16];
				int len = strlen(data);
				for (size_t i = 1; i < len; i++)
				{
					id[i - 1] = data[i];
				}
				id[len - 1] = '\0';
				std::list<ChatMessage*>::iterator it = mes_buf.begin();
				for (; it != mes_buf.end(); it++) {
					if (atoll(id) == (*it)->getId()) {
						mes_buf.erase(it);
						break;
					}
				}
				delete[] id;
			}
			else {
				// extract message from data
				ChatMessage* message = new ChatMessage();
				if (message->parseString(data)) {
					bool isUsernameExist = false;
					std::list<char*>::iterator it = usersList.begin();
					for (; it != usersList.end(); it++) {
						if (strcmp(message->getNick(), *it) == 0) {
							isUsernameExist = true;
							break;
						}
					}
					if (isUsernameExist) {
						message->setId(mId++);
						mes_buf.push_back(message);
						if (mes_buf.size() > MAX_COUNT_MESSAGES) {
							delete mes_buf.front();
							mes_buf.pop_front();
						}
						SerializeMessages();
						mts = message->toString();
						SendMessageA(serverLog, LB_ADDSTRING, 0, (LPARAM)mts);

						mts = SerializeMessages();
						// send answer to client - write in socket
						send(acceptSocket, mts, strlen(mts) + 1, 0);
						//delete[] mts;
					}
					else {
						char buf[150];
						snprintf(buf, 150, "%s doesn't registered", message->getNick());
						SendMessageA(serverLog, LB_ADDSTRING, 0, (LPARAM)buf);
						send(acceptSocket, "401", 4, 0);
					}
				}
				else {
					delete message;
					SendMessageA(serverLog, LB_ADDSTRING, 0, (LPARAM)data);
					send(acceptSocket, "500", 4, 0);
				}
			}

		}

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

char* SerializeMessages() {
	// message->toString()  --> char*
	// collect them
	// calc size and build string
	// ==> StringBuilder
	char* ret;

	size_t n = mes_buf.size();
	if (n == 0) {
		ret = new char[1]{ '\0' };
		return ret;
	}

	char** strs = new char* [n];
	size_t total = 0, i = 0;

	for (auto it : mes_buf) {
		strs[i] = it->toString();
		total += strlen(strs[i]) + 1;
		++i;
	}
	ret = new char[total + 1];
	ret[0] = '\0';
	for (i = 0; i < n; i++)
	{
		strcat(ret, strs[i]);
		strcat(ret, "\r");
	}
	ret[total - 1] = '\0';

	return ret;
}