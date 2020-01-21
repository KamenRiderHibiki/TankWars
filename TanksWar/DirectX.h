#pragma once
#ifndef _DIRECTX_H
#define _DIRECTX_H

#define WIN32_EXTRA_LEAN
#define DIRECTINPUT_VERSION 0x0800
#define SafeRelease(p) if((p)) (p)->Release();//��������
#define SafeDelete(p) if(p) delete p;
//����ͷ�ļ�
#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <XInput.h>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <fstream>
#include "DirectSound.h"
#include <io.h>
#include <algorithm>

using namespace std;

//�����
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"xinput.lib")
#pragma comment(lib,"dsound.lib")
#pragma comment(lib,"dxerr.lib")

//����ֵ
extern const string APPTITLE;
extern const int SCREENW;
extern const int SCREENH;
extern bool gameover;
extern const double PI, PI_under_180, PI_over_180;

//Direct3d�ӿڱ���
extern LPDIRECT3D9 d3d;
extern LPDIRECT3DDEVICE9 d3ddev;
extern LPDIRECT3DSURFACE9 backbuffer;
extern LPD3DXSPRITE spriteobj;

//Direct3d���ܺ���
bool Direct3DInit(HWND window,int width,int height,bool fullscreen);
void Direct3DShutdown();
LPDIRECT3DSURFACE9 LoadSurface(string filename);
void DrawSurface(LPDIRECT3DSURFACE9 dest, float x, float y, LPDIRECT3DSURFACE9 source);
D3DXVECTOR2 GetBitmapSize(string filename);
LPDIRECT3DTEXTURE9 LoadTexture(string filename, D3DCOLOR transcolor = D3DCOLOR_XRGB(0, 0, 0));

//Direct3d����������豸��״̬
extern LPDIRECTINPUT8 dinput;
extern LPDIRECTINPUTDEVICE8 dimouse;
extern LPPOINT lpPoint;
extern LPDIRECTINPUTDEVICE8 dikeyboard;
extern DIMOUSESTATE mouse_state;
extern XINPUT_GAMEPAD controllers[4];

//Direct���빦��
bool DirectInputInit(HWND hwnd);
void DirectInputUpdate();
void DirectInputShutdown();
int KeyDown(int);
int MouseButton(int);
int MouseX();
int MouseY();
int MouseWheel();
int MousePositionX(HWND hwnd);
int MousePositionY(HWND hwnd);
void XinputVibrate(int contNum = 0, int amount = 65535);
bool XinputControllFound();

//��Ϸ����
bool GameInit(HWND window);
void GameRun(HWND window);
void GameEnd();

//�ṹ��

//����
struct SPRITE
{
	float x, y;//����
	int frame, columns;//֡��ţ�����
	int width, height;//��ȣ��߶�
	float scaling, rotation;//���ű�������ת����
	int startframe, endframe;//��ʼ������֡���
	int starttime, delay;//��ʼʱ�䣬֡���ӳ�
	int direction;//����֡�ı仯����
	float velx, vely;//�ٶ�
	D3DCOLOR color;
	SPRITE()
	{
		x = y = 0;
		frame = 0, columns = 1;
		width = height = 0;
		scaling = 1.0f, rotation = 0.0f;
		startframe = endframe = 0;
		direction = 1;
		starttime = delay = 0;
		velx = vely = 0.0f;
		color = D3DCOLOR_XRGB(255, 255, 255);
	}
};
//��ȡ��ͼ
struct MAPDATA
{
	int width, height;
	vector <int>map;
	MAPDATA(int x, int y)
	{
		width = x, height = y;
		map.resize(width*height);
		for (int i = 0; i < width*height; i++)
			map.push_back(0);
	}
	MAPDATA()
	{
		width = height = 0;
		map.push_back(0);
	}
	~MAPDATA()
	{
		vector<int>(map).swap(map);
	}
};
//3Dģ��
struct MODEL
{
	LPD3DXMESH mesh;
	D3DMATERIAL9* materials;
	LPDIRECT3DTEXTURE9* textures;
	DWORD material_count;
	D3DXVECTOR3 translate;
	D3DXVECTOR3 rotate;
	D3DXVECTOR3 scale;

	MODEL()
	{
		material_count = 0;
		mesh = NULL;
		materials = NULL;
		textures = NULL;
		translate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		rotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		scale = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
	}
};
//��ѧ���������
struct VEC
{
	float x, y;
	VEC()
	{
		x = y = 0.0f;
	}
	VEC(float ix, float iy)
	{
		x = ix, y = iy;
	}
};
//�߶�
struct SEGMENT
{
	VEC p0, p1, dir;
	SEGMENT(VEC p00, VEC p01)
	{
		dir = VEC(p01.x - p00.x, p01.y - p00.y);//��00��01������
		p0 = p00, p1 = p01;
	}
	SEGMENT()
	{
		p0 = p1 = dir = VEC();
	}
};
//�����
struct POLYGON
{
	int n;//����
	VEC *vertices = nullptr;//��
	SEGMENT *edges = nullptr;//��
	POLYGON(int numVertices, VEC *inVertices)
	{
		n = numVertices;
		vertices = inVertices;
		edges = new SEGMENT[numVertices];
		for (int i = 0; i < numVertices - 1; i++)
			edges[i] = SEGMENT(vertices[i], vertices[i + 1]);//���׵�β�����������
		edges[numVertices - 1] = SEGMENT(vertices[numVertices - 1], vertices[0]); // ���һ��������β��֮��
	}
	//��������
	~POLYGON()
	{
		if (vertices)
			delete[] vertices;
		if (edges)
			delete[] edges;
	}
	//POLYGON(int numVertices);
	//��ƺ���
	POLYGON(const POLYGON & py)
	{
		n = py.n;
		vertices = new VEC[n];
		edges = new SEGMENT[n];
		for (int i = 0; i < n; i++)
		{
			vertices[i] = py.vertices[i];
			edges[i] = py.edges[i];
		}
	}
	//���ظ�ֵ����
	POLYGON operator=(const POLYGON &py)
	{
		if (this == &py)
			return *this;
		delete[] vertices;//ɾ���ɵ���Ϣ
		delete[] edges;
		n = py.n;
		vertices = new VEC[n];
		edges = new SEGMENT[n];
		for (int i = 0; i < n; i++)
		{
			vertices[i] = py.vertices[i];
			edges[i] = py.edges[i];
		}
		return *this;
	}
};

//������ع���
SPRITE GetSpriteSize(string filename, SPRITE* sprite, int columns, int row);
void SpriteDrawFrame(LPDIRECT3DTEXTURE9 texture = NULL, int destX = 0, int destY = 0, int frameNum = 1, int frameW = 64, int frameH = 64, int columns = 1);
void SpriteAnimate(int &frame, int startFrame, int endFrame, int direction, int &startTime, int delay);
void SpriteTransformDraw(LPDIRECT3DTEXTURE9 image, int x, int y, int width, int height, int frame = 0, int columns = 1, float rotation = 0.0f, float scaling = 1.0f, D3DCOLOR color = D3DCOLOR_XRGB(255, 255, 255));
int CollisionSAT(SPRITE sprite1, SPRITE sprite2);
int CollisionRect(SPRITE sprite1, SPRITE sprite2);
bool CollisionDistance(SPRITE sprite1, SPRITE sprite2);

//���幦��
LPD3DXFONT MakeFont(string name, int size);
void FontPrint(LPD3DXFONT font, int x, int y, string text, D3DCOLOR color = D3DCOLOR_XRGB(255, 255, 255));

//��Ƶ����
extern CSoundManager *dsound;//��Ƶ����

bool DirectSoundInit(HWND hwnd);
void DirectSoundShutdown();
CSound *LoadSound(string filename);
void PlaySound(CSound *sound);
void LoopSound(CSound *sound);
void StopSound(CSound *sound);

//��ȡ�ļ�
MAPDATA GetMap(string datapath);//��ȡ�ֿ��ͼ

//3Dģ�͹���
void DrawModel(MODEL *model);
void DeleteModel(MODEL *model);
MODEL *LoadModel(string filename);
bool FindFile(string *filename);
void SplitPath(const string& inputPath, string* pathOnly, string* filenameOnly);
void SetCamera(float posx, float posy, float posz, float lookx = 0.0f, float looky = 0.0f, float lookz = 0.0f);

//��ѧ����
double toRadians(double degrees);
double toDegrees(double radians);

#endif // !_DIRECTX_H