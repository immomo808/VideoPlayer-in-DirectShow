// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#define ID_FILE_EXIT 9001
#define ID_FILE_OPEN 9002
#define ID_FUNC_PLAY 10001
#define ID_FUNC_HALF 10002
#define ID_FUNC_TWICE 10003
#define ID_FUNC_SMALL 10004
#define ID_FUNC_LARGE 10005

#include "resource.h"
#include <new>
#include <windows.h>
#include <dshow.h>
#include "playback.h"
#include <string.h>
#include <limits.h>
//#include <atlstr.h>
//#include <atlstr.h>


#pragma comment(lib, "strmiids")

DShowPlayer *g_pPlayer = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CALLBACK OnGraphEvent(HWND hwnd, long eventCode, LONG_PTR param1, LONG_PTR param2);
void OnChar(HWND hwnd, wchar_t c);
void OnFileOpen(HWND hwnd);
void OnPaint(HWND hwnd);
void OnSize(HWND hwnd);
void NotifyError(HWND hwnd, PCWSTR pszMessage);
void SizeChange(HWND, int);



int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE)))
	{
		NotifyError(NULL, L"CoInitializeEx failed.");
		return 0;
	}

	// Register the window class.
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"DirectShow Playback",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		NotifyError(NULL, L"CreateWindowEx failed.");
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);

	// Run the message loop.

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CHAR:
		OnChar(hwnd, (wchar_t)wParam);
		return 0;

	case WM_CREATE:
		g_pPlayer = new (std::nothrow) DShowPlayer(hwnd);
		if (g_pPlayer == NULL)
		{
			return -1;
		}
		HMENU hMenu, hSubMenu;


		hMenu = CreateMenu();

		hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, ID_FILE_OPEN, L"O&pen");
		AppendMenu(hSubMenu, MF_STRING, ID_FILE_EXIT, L"E&xit");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, L"&File");

		hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, ID_FUNC_PLAY, L"&Play/Pause");
		AppendMenu(hSubMenu, MF_STRING, ID_FUNC_HALF, L"Size &Half");
		AppendMenu(hSubMenu, MF_STRING, ID_FUNC_TWICE, L"Size &Twice");
		AppendMenu(hSubMenu, MF_STRING, ID_FUNC_SMALL, L"Size &Smaller");
		AppendMenu(hSubMenu, MF_STRING, ID_FUNC_LARGE, L"Size &Larger");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, L"Function");

		SetMenu(hwnd, hMenu);

		return 0;

	case WM_DESTROY:
		delete g_pPlayer;
		PostQuitMessage(0);
		return 0;

	case WM_DISPLAYCHANGE:
		g_pPlayer->DisplayModeChanged();
		break;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		OnPaint(hwnd);
		return 0;

	case WM_SIZE:
		OnSize(hwnd);
		return 0;

	case WM_GRAPH_EVENT:
		g_pPlayer->HandleGraphEvent(OnGraphEvent);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILE_OPEN:
			OnFileOpen(hwnd);
			break;
		case ID_FILE_EXIT:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case ID_FUNC_PLAY:
			if (g_pPlayer->State() == STATE_RUNNING)
			{
				g_pPlayer->Pause();
			}
			else
			{
				g_pPlayer->Play();
				
			}
			break;
		case ID_FUNC_HALF:
			SizeChange(hwnd,1);
			break;
		case ID_FUNC_TWICE:
			SizeChange(hwnd, 2);
			break;
		case ID_FUNC_SMALL:
			SizeChange(hwnd, 3);
			break;
		case ID_FUNC_LARGE:
			SizeChange(hwnd, 4);
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc;

	hdc = BeginPaint(hwnd, &ps);

	if (g_pPlayer->State() != STATE_NO_GRAPH && g_pPlayer->HasVideo())
	{
		// The player has video, so ask the player to repaint. 
		g_pPlayer->Repaint(hdc);
	}
	else
	{
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
	}

	EndPaint(hwnd, &ps);
}

void OnSize(HWND hwnd)
{
	if (g_pPlayer)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		g_pPlayer->UpdateVideoWindow(&rc);
	}
}

void SizeChange(HWND hwnd, int flag)
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	RECT tmp;
	GetWindowRect(hwnd, &tmp);
	float right=(float)rc.right, bottom=(float)rc.bottom;
	float ratio = right / bottom;
	switch (flag)
	{
	case 1:
		right = right / 2;
		bottom = bottom / 2;
		break;
	case 2:
		right = right * 2;
		bottom = bottom * 2;
		break;
	case 3:
		right *= 0.9;
		bottom *= 0.9 ;
		break;
	case 4:
		right *= 1.2;
		bottom *= 1.2 ;
		break;
	}
	SetWindowPos(hwnd, HWND_TOPMOST, tmp.left, tmp.top, right, bottom, SWP_SHOWWINDOW);
}

void CALLBACK OnGraphEvent(HWND hwnd, long evCode, LONG_PTR param1, LONG_PTR param2)
{
	switch (evCode)
	{
	case EC_COMPLETE:
	case EC_USERABORT:
		g_pPlayer->Stop();
		break;

	case EC_ERRORABORT:
		NotifyError(hwnd, L"Playback error.");
		g_pPlayer->Stop();
		break;
	}
}

void OnChar(HWND hwnd, wchar_t c)
{
	switch (c)
	{
	case L'o':
	case L'O':
		OnFileOpen(hwnd);
		break;

	case L' ':
		if (g_pPlayer->State() == STATE_RUNNING)
		{
			g_pPlayer->Pause();
		}
		else
		{
			g_pPlayer->Play();
		}
		break;
	case L'+':
	case L'=':
		SizeChange(hwnd, 4);
		break;
	case L'-':
	case L'_':
		SizeChange(hwnd, 3);
		break;
	case L'2':
		SizeChange(hwnd, 2);
		break;
	case L'1':
		SizeChange(hwnd, 1);
		break;
	}
}

void OnFileOpen(HWND hwnd)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	WCHAR szFileName[MAX_PATH];
	szFileName[0] = L'\0';

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = L"All (*.*)/0*.*/0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST;

	HRESULT hr;

	if (GetOpenFileName(&ofn))
	{
		hr = g_pPlayer->OpenFile(szFileName);

		InvalidateRect(hwnd, NULL, FALSE);

		if (SUCCEEDED(hr))
		{
			// If this file has a video stream, notify the video renderer 
			// about the size of the destination rectangle.
			RECT rc;
			GetClientRect(hwnd, &rc);
			RECT tmp;
			GetWindowRect(hwnd, &tmp);
			SetWindowPos(hwnd, HWND_TOPMOST, tmp.left, tmp.top, rc.right , rc.bottom , SWP_SHOWWINDOW);
			OnSize(hwnd);
		}
		else
		{
			NotifyError(hwnd, TEXT("Cannot open this file."));
		}
	}
}

void NotifyError(HWND hwnd, PCWSTR pszMessage)
{
	MessageBox(hwnd, pszMessage, TEXT("Error"), MB_OK | MB_ICONERROR);

}


