#include "DirectX.h"

using namespace std;

bool gameover = false;

LRESULT WINAPI WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		gameover = true;
		PostQuitMessage(0);
		return 0;
		break;
	default:
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);//大小是结构大小
	wc.style = CS_HREDRAW | CS_VREDRAW;//改变宽度/高度时重绘
	wc.lpfnWndProc = (WNDPROC)WinProc;//和win消息值相同时调用回调函数
	wc.cbClsExtra = 0;//增加内存
	wc.cbWndExtra = 0;//同上
	wc.hInstance = hInstance;//实例选择
	wc.hIcon = NULL;//图标
	wc.hIconSm = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);//游标
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//背景颜色
	wc.lpszMenuName = NULL;//程序菜单
	wc.lpszClassName = "MainWindowCLass";//给予窗口类名，用于消息处理
	RegisterClassEx(&wc);
	HWND window = CreateWindow("MainWindowClass", APPTITLE.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, SCREENW, SCREENH, (HWND)NULL, (HMENU)NULL, hInstance, (LPVOID)NULL);//无标题栏、菜单
	if (window == 0)
		return 0;
	ShowWindow(window, nCmdShow);
	UpdateWindow(window);
	if (!GameInit(window))
		return 0;
	MSG message;
	while (!gameover)
	{
		if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		GameRun(window);
	}
	GameEnd();
	return message.wParam;
}