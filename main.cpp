#include <windows.h>
#include <commctrl.h>
#include "CmdLine/cmdline.h"
#include "defs.h"
#include "common.h"
#include <algorithm>
#include "QrDrawer/QrDrawer.h"
#pragma warning(disable: 4244)

LRESULT CALLBACK MainWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT OnCreateMain(HWND hDlg, WPARAM wParam, LPVOID param);
LRESULT OnCloseMain(HWND hDlg);
LRESULT OnWindowRedraw(HWND hWnd);
LRESULT OnKey(HWND hWnd, DWORD vKeyCode);


INT WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE, LPTSTR ptCmdLine, int nCmdShow)
{
	int moduleSize = DEFAULT_MODULE_SIZE;
	MODULE_SHAPE moduleShape = MODULE_SHAPE::DEFAULT_MODULE_SHAPE;
	std::tstring strToCode = _T("");
#pragma region cmd line options
	CmdLine cmline;
	cmline.AddOption(CMD_SHAPE_OPTION, true, _T("module shape (square or round)"), {_T("square"), _T("round")});
	cmline.AddOption(CMD_SIZE_OPTION, true, _T("size of module"));
	cmline.AddOption(CMD_STRING_OPTION, true, _T("string to code"));
	if(cmline.SetCmd(ptCmdLine))
	{
		moduleSize = cmline.IsSet(CMD_SIZE_OPTION) ? cmline.GetInt(CMD_SIZE_OPTION) : DEFAULT_MODULE_SIZE;
		if (cmline.IsSet(CMD_SHAPE_OPTION))
		{
			std::tstring shapeStr = cmline.GetString(CMD_SHAPE_OPTION);
			if (shapeStr._Equal(_T("square")))
				moduleShape = MODULE_SHAPE::SQUARE;
			else
				if (shapeStr._Equal(_T("round")))
					moduleShape = MODULE_SHAPE::ROUND;
		}
		strToCode = cmline.GetString(CMD_STRING_OPTION);
	}
	else
	{
		strToCode = ptCmdLine;
	}
	if(strToCode.length() == 0)
	{
		return 1;
	}
#pragma endregion
#pragma region drawer initialization
	QrDrawer drawer;
	drawer.CreateQrCode(strToCode);
	drawer.SetFgColor(DEFAULT_FOREGROUND_COLOR);
	drawer.SetBgColor(DEFAULT_BACKGROUND_COLOR);
	drawer.SetModuleSize(abs(moduleSize));
	drawer.SetShape(moduleShape);
#pragma endregion
#pragma region creating window
	WNDCLASS wnd_class;
	ZeroMemory(&wnd_class, sizeof(wnd_class));
	wnd_class.style = 0;
	wnd_class.lpfnWndProc = MainWndProc;
	wnd_class.hInstance = hInst;
	//wnd_class.hbrBackground = HBRUSH(COLOR_WINDOW + 1);
	wnd_class.lpszClassName = MAIN_WINDOW_CNAME;
	RegisterClass(&wnd_class);
	SIZE screenSize = { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	SIZE windowSize = { drawer.GetWindowSize(),  drawer.GetWindowSize() };
	HWND mainWindow = CreateWindow(	MAIN_WINDOW_CNAME, _T("QrCoder"), WS_OVERLAPPED | WS_VISIBLE, (screenSize.cx - windowSize.cx)/2, (screenSize.cy - windowSize.cy)/2,
									windowSize.cx, windowSize.cy, NULL, NULL, NULL, &drawer);
#pragma endregion

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.lParam;
};

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return OnCreateMain(hWnd, wParam, ((CREATESTRUCT*)lParam)->lpCreateParams);
	case WM_DESTROY:
		return OnCloseMain(hWnd);
	case WM_PAINT:
		return OnWindowRedraw(hWnd);
	case WM_KEYUP:
		if(OnKey(hWnd, wParam) == 0)
			return 0;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
LRESULT OnCreateMain(HWND hWnd, WPARAM wParam, LPVOID param)
{
	QrDrawer* drawer = (QrDrawer*)param;
	SetProp(hWnd, WINDOW_PROPERTY_QRDRAWER, (HANDLE)drawer);
	return 0;
}
LRESULT OnWindowRedraw(HWND hWnd)
{
	QrDrawer* drawer = (QrDrawer*)GetProp(hWnd, WINDOW_PROPERTY_QRDRAWER);
	drawer->RedrawQrCode(hWnd);
	return 0;
}
LRESULT OnCloseMain(HWND hWnd)
{
	PostQuitMessage(0);
	return 0;
}

LRESULT OnKey(HWND hWnd, DWORD vKeyCode)
{
	switch(vKeyCode)
	{
	case VK_ESCAPE:
		DestroyWindow(hWnd);
		return 0;
		break;
	}
	return 1;
}