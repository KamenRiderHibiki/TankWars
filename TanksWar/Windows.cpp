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
	wc.cbSize = sizeof(WNDCLASSEX);//��С�ǽṹ��С
	wc.style = CS_HREDRAW | CS_VREDRAW;//�ı���/�߶�ʱ�ػ�
	wc.lpfnWndProc = (WNDPROC)WinProc;//��win��Ϣֵ��ͬʱ���ûص�����
	wc.cbClsExtra = 0;//�����ڴ�
	wc.cbWndExtra = 0;//ͬ��
	wc.hInstance = hInstance;//ʵ��ѡ��
	wc.hIcon = NULL;//ͼ��
	wc.hIconSm = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);//�α�
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//������ɫ
	wc.lpszMenuName = NULL;//����˵�
	wc.lpszClassName = "MainWindowCLass";//���贰��������������Ϣ����
	RegisterClassEx(&wc);
	HWND window = CreateWindow("MainWindowClass", APPTITLE.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, SCREENW, SCREENH, (HWND)NULL, (HMENU)NULL, hInstance, (LPVOID)NULL);//�ޱ��������˵�
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