#include "DirectX.h"
//��ѧ����
const double PI = 3.1415926535897932;
const double PI_under_180 = 180.0f / PI;
const double PI_over_180 = PI / 180.f;
//D3D�ӿڱ���
LPDIRECT3D9 d3d = NULL;
LPDIRECT3DDEVICE9 d3ddev = NULL;
LPDIRECT3DSURFACE9 backbuffer = NULL;
LPD3DXSPRITE spriteobj = NULL;
//Direct�������
LPDIRECTINPUT8 dinput = NULL;
LPDIRECTINPUTDEVICE8 dimouse = NULL;
LPPOINT lpPoint = NULL;
LPDIRECTINPUTDEVICE8 dikeyboard = NULL;
DIMOUSESTATE mouse_state;
char keys[256];
XINPUT_GAMEPAD controllers[4];
//��ѧ����

//�Ƕ�ת����
double toRadians(double degrees)
{
	return degrees * PI_over_180;
}
//����ת�Ƕ�
double toDegrees(double radians)
{
	return radians * PI_under_180;
}
//C������������������
POLYGON Polygon(int nvertices, ...)
{
	va_list args;
	va_start(args, nvertices);
	VEC *vertices = new VEC[nvertices];
	int i;
	for (i = 0; i < nvertices; i++) {
		vertices[i] = va_arg(args, VEC);
	}
	va_end(args);
	return POLYGON(nvertices, vertices);
}
//ʸ����ͶӰ����
float dot(VEC a, VEC b)
{
	return a.x*b.x + a.y*b.y;
}
//��ģ����
VEC normalize(VEC v)
{
	float mag = sqrt(v.x*v.x + v.y*v.y);
	VEC b = { v.x / mag, v.y / mag }; // ����b��ԭ�����Ϊ1 vector b is only of distance 1 from the origin
	return b;
}
//��������
VEC perp(VEC v)
{
	VEC b = { v.y, -v.x };
	return b;
}
//����ͶӰ
float* calProjection(POLYGON a, VEC axis)
{
	axis = normalize(axis);//������
	float min = dot(a.vertices[0], axis);
	float max = min; // min �� max ����ʼ���յ�
	for (int i = 0; i<a.n; i++) {
		float proj = dot(a.vertices[i], axis); // ��ֱ�����ҵ������ÿһ�����ͶӰ
		if (proj < min)
			min = proj;//Ѱ����Сֵ
		if (proj > max)
			max = proj;//Ѱ�����ֵ
	}
	float* arr = new float[2];
	arr[0] = min; arr[1] = max;//ͶӰ�߶�
	return arr;
}
//����������Ӻ���
int contains(float n, float* range)
{
	float a = range[0], b = range[1];//ȡ���߶ε����˵�
	if (b<a)
	{
		a = b;
		b = range[0];//aΪСֵ��bΪ��ֵ
	}
	if (n >= a && n <= b)//���n����a��b�����غ�
		return (n - a < b - n) ? (int)(n - a) + 1 : (int)(b - n) + 1;//���ؽ�С�ķֿ�����
	else
	{
		return 0;//û��ײ
	}
}
//�ж������߶��Ƿ��غ�
int overlap(float* a_, float* b_)
{
	int i = 0,step = 0;
	i = contains(a_[0], b_);//a����˵��Ƿ�����b��
	step = contains(a_[1], b_);//a���Ҷ˵��Ƿ�����b��
	if (i != 0 &&step != 0 && step < i||i == 0 && step != 0)//���step��С���߷�����ײ
		i = step;//ȡ��С�ķ������
	step = contains(b_[0], a_);//b����˵��Ƿ�����a��
	if (i != 0 && step != 0 && step < i || i == 0 && step != 0)
		i = step;//ȡ��С�ķ������
	step = contains(b_[1], a_);//b���Ҷ˵��Ƿ�����a��
	if (i != 0 && step != 0 && step < i || i == 0 && step != 0)
		i = step;//ȡ��С�ķ������
	return i;
}

//DirectX����

//D3D��ʼ��
bool Direct3DInit(HWND window, int width, int height, bool fullscreen)
{
	//D3D����
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d)
		return false;
	//D3D����
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = (!fullscreen);
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.BackBufferHeight = height;
	d3dpp.BackBufferWidth = width;
	d3dpp.hDeviceWindow = window;
	//D3D�豸
	(*d3d).CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev);
	if (!d3ddev)
		return false;
	//��ȡ������ָ��
	d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,&backbuffer);
	//��������ʵ��
	D3DXCreateSprite(d3ddev, &spriteobj);
	return true;
}
//D3D�ر�
void Direct3DShutdown()
{
	if (spriteobj)
		spriteobj->Release();
	if (d3ddev)
		d3ddev->Release();
	if (d3d)
		d3d->Release();
}
//װ��λͼ������
LPDIRECT3DSURFACE9 LoadSurface(string filename)
{
	LPDIRECT3DSURFACE9 image = NULL;
	//��ȡλͼ����
	D3DXIMAGE_INFO info;
	HRESULT result = D3DXGetImageInfoFromFile(filename.c_str(), &info);
	if (result != D3D_OK)
		return NULL;
	//��������
	result = d3ddev->CreateOffscreenPlainSurface(
		info.Width,		//���
		info.Height,	//�߶�
		D3DFMT_X8R8G8B8,//�����ʽ
		D3DPOOL_DEFAULT,//�洢��
		&image,			//ָ������ָ��
		NULL);			//��������ԶΪ�գ�
	if (result != D3D_OK)
		return NULL;
	//��λͼװ�ص�����
	result = D3DXLoadSurfaceFromFile(
		image,					//Ŀ�����
		NULL,					//Ŀ���ɫ��
		NULL,					//Ŀ�����
		filename.c_str(),		//Դ�ļ���
		NULL,					//Դ����
		D3DX_DEFAULT,			//ͼ����˿���
		D3DCOLOR_XRGB(0,0,0),	//����͸��
		NULL);					//Դͼ����Ϣ
	if (result != D3D_OK)
		return NULL;
	return image;
}
//���Ʊ���
void DrawSurface(LPDIRECT3DSURFACE9 dest, float x, float y, LPDIRECT3DSURFACE9 source)
{
	D3DSURFACE_DESC desc;
	source->GetDesc(&desc);
	RECT source_rect = { 0,0,(long)desc.Width,(long)desc.Height };
	RECT dest_rect = { (long)x,(long)y,(long)x + desc.Width,(long)y + desc.Height };
	d3ddev->StretchRect(source, &source_rect, dest, &dest_rect, D3DTEXF_NONE);
}
//��ͼ���ļ���ȡͼ���С
D3DXVECTOR2 GetBitmapSize(string filename)
{
	D3DXIMAGE_INFO info;
	HRESULT result = D3DXGetImageInfoFromFile(filename.c_str(), &info);
	if (result != S_OK)
	{
		MessageBox(NULL, "Error in load bitmap size", "GetBitmap Error", MB_OK | MB_ICONERROR);
		return  D3DXVECTOR2(0.0f, 0.0f);
	}
	return D3DXVECTOR2((float)info.Width, (float)info.Height);
}
//����ͼ���ļ�������
LPDIRECT3DTEXTURE9 LoadTexture(string filename, D3DCOLOR transcolor)
{
	LPDIRECT3DTEXTURE9 texture = NULL;
	D3DXIMAGE_INFO info;
	HRESULT result = D3DXGetImageInfoFromFile(filename.c_str(), &info);
	if (result != D3D_OK)
		return NULL;
	D3DXCreateTextureFromFileEx(
		d3ddev,
		filename.c_str(),
		info.Width,
		info.Height,
		1,
		D3DPOOL_DEFAULT,
		D3DFMT_UNKNOWN,
		D3DPOOL_DEFAULT,
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		transcolor,
		&info,
		NULL,
		&texture);
	if (result != D3D_OK)
		return NULL;
	return texture;
}
//Direct�����ʼ��
bool DirectInputInit(HWND hwnd)
{
	//��ʼ������ʵ��
	HRESULT result = DirectInput8Create(
		GetModuleHandle(NULL),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&dinput,
		NULL);
	//��ʼ������
	dinput->CreateDevice(GUID_SysKeyboard, &dikeyboard, NULL);
	dikeyboard->SetDataFormat(&c_dfDIKeyboard);
	dikeyboard->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	dikeyboard->Acquire();
	//��ʼ�����
	dinput->CreateDevice(GUID_SysMouse, &dimouse, NULL);
	dimouse->SetDataFormat(&c_dfDIMouse);
	dimouse->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	dimouse->Acquire();
	lpPoint = new POINT;
	GetCursorPos(lpPoint);
	ScreenToClient(hwnd, lpPoint);
	d3ddev->ShowCursor(false);
	return true;
}
//ˢ������״̬
void DirectInputUpdate()
{
	HRESULT result;
	//�������
	result = dimouse->GetDeviceState(sizeof(mouse_state), (LPVOID)&mouse_state);
	if (result != S_OK)
		dimouse->Acquire();
	//���¼���
	result = dikeyboard->GetDeviceState(sizeof(keys), (LPVOID)&keys);
	if (result != S_OK)
		dikeyboard->Acquire();
	//�����ֱ�
	for (int i = 0; i < 4; i++)
	{
		ZeroMemory(&controllers[i], sizeof(XINPUT_STATE));
		//��ȡ״̬
		XINPUT_STATE state;
		DWORD result = XInputGetState(i, &state);
		//��ȫ�ֿ�����������洢״̬
		if (result == 0)
			controllers[i] = state.Gamepad;
	}
}
//�ر�����
void DirectInputShutdown()
{
	if (dikeyboard)
	{
		dikeyboard->Unacquire();
		dikeyboard->Release();
		dikeyboard = NULL;
	}
	if (dimouse)
	{
		dimouse->Unacquire();
		dimouse->Release();
		dimouse = NULL;
	}
}
//��������
int KeyDown(int key)
{
	return (keys[key] & 0x80);
}
//��������
int MouseButton(int button)
{
	return mouse_state.rgbButtons[button] & 0x80;
}
//����ƶ�x���꣨��Ϊ����
int MouseX()
{
	return mouse_state.lX;
}
//����ƶ�y���꣨��Ϊ����
int MouseY()
{
	return mouse_state.lY;
}
int MouseWheel()
{
	return mouse_state.lZ;
}
int MousePositionX(HWND hwnd)
{
	GetCursorPos(lpPoint);
	ScreenToClient(hwnd, lpPoint);
	return lpPoint->x;
}
int MousePositionY(HWND hwnd)
{
	GetCursorPos(lpPoint);
	ScreenToClient(hwnd, lpPoint);
	return lpPoint->y;
}
//Xbox�ֱ���
void XinputVibrate(int contNum, int amount)
{
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = amount;
	vibration.wRightMotorSpeed = amount;
	XInputSetState(contNum, &vibration);
}
//��ʼ��Xbox�ֱ�
bool XinputControllFound()
{
	XINPUT_CAPABILITIES caps;
	ZeroMemory(&caps, sizeof(XINPUT_CAPABILITIES));
	XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &caps);
	return caps.Type != 0 ? false : true;
}

//������ع���

//��ͼ���ļ���ȡ�����С
SPRITE GetSpriteSize(string filename, SPRITE* sprite, int columns, int row)
{
	D3DXIMAGE_INFO info;
	HRESULT result = D3DXGetImageInfoFromFile(filename.c_str(), &info);
	if (result != S_OK)
	{
		MessageBox(NULL, "Error in load texture size", "GetBitmap Error", MB_OK | MB_ICONERROR);
		return  *sprite;
	}
	sprite->width = info.Width / columns, sprite->height = info.Height / row;
	sprite->columns = columns;
	return *sprite;
}
//���ƾ���֡
void SpriteDrawFrame(LPDIRECT3DTEXTURE9 texture, int destX, int destY, int frameNum, int frameW, int frameH, int columns)
{
	if (!texture)
		return;
	D3DXVECTOR3 position((float)destX, (float)destY, 0);
	D3DCOLOR white = D3DCOLOR_XRGB(255, 255, 255);
	RECT rect;
	rect.left = (frameNum % columns) * frameW;
	rect.top = (frameNum / columns) * frameH;
	rect.right = rect.left + frameW;
	rect.bottom = rect.top + frameH;
	spriteobj->Draw(texture, &rect, NULL, &position, white);
}
//�仯����֡
void SpriteAnimate(int & frame, int startFrame, int endFrame, int direction, int & startTime, int delay)
{
	if ((int)GetTickCount() > startTime + delay)
	{
		startTime = GetTickCount();
		frame += direction;
		if (frame > endFrame)
			frame = startFrame;
		if (frame < startFrame)
			frame = endFrame;
	}
}
//�任���ƾ���
void SpriteTransformDraw(LPDIRECT3DTEXTURE9 image, int x, int y, int width, int height, int frame, int columns, float rotation, float scaling, D3DCOLOR color)
{
	D3DXVECTOR2 scale(scaling, scaling);//xy��������
	D3DXVECTOR2 trans(x, y);//xyƫ��
	D3DXVECTOR2 center((float)(width*scaling) / 2, (float)(height*scaling) / 2);//��ת����
	D3DXMATRIX mat;//�任����
	D3DXMatrixTransformation2D(&mat, NULL, 0, &scale, &center, rotation, &trans);//�趨�任����
	spriteobj->SetTransform(&mat);
	int fx = (frame % columns)*width;
	int fy = (frame / columns)*height;
	RECT srcRect = { fx, fy, fx + width, fy + height };
	spriteobj->Draw(image, &srcRect, NULL, NULL, color);

	D3DXMatrixIdentity(&mat);//������λ����
	spriteobj->SetTransform(&mat);//�ָ���������ϵ
}
//��ȡ�߽��
POLYGON getCollisionBox(SPRITE sprite)
{
	float sx = (float)sprite.x, sy = (float)sprite.y, sw = (float)sprite.width / 2.0f * sprite.scaling, sh = (float)sprite.height / 2.0f*sprite.scaling;
	VEC center = { sx + sw,sy + sh };//���ĵ�
	float hypot = sqrt(sw*sw + sh*sh);//1/2б�ǳ���
	float theta = atan2(sw, sh);//�����Ϸ�˳ʱ��Ƕ�
								//������˳ʱ��4������ת������
	VEC *vec = new VEC[4];
	vec[0] = VEC(center.x + (sin(sprite.rotation - theta)*hypot), center.y - (cos(sprite.rotation - theta)*hypot));
	vec[1] = VEC(center.x + (sin(sprite.rotation + theta)*hypot), center.y - (cos(sprite.rotation + theta)*hypot));
	vec[2] = VEC(center.x + (sin(sprite.rotation - theta + PI)*hypot), center.y - (cos(sprite.rotation - theta + PI)*hypot));
	vec[3] = VEC(center.x + (sin(sprite.rotation + theta + PI)*hypot), center.y - (cos(sprite.rotation + theta + PI)*hypot));
	//���ƫ���С��ȡ��
	for (int check = 0; check < 4; check++)
	{
		if (vec[check].x - 0.01f < (int)vec[check].x)
			vec[check].x = (float)((int)vec[check].x);
		if (vec[check].x + 0.01f > (int)vec[check].x + 1)
			vec[check].x = (float)((int)vec[check].x + 1);
	}
	return POLYGON(4, vec);
}
//���ڷ������㷨��ײ����
int CollisionSAT(SPRITE sprite1, SPRITE sprite2)
{
	int distanceA = 0,distanceB = 0;
	POLYGON a = getCollisionBox(sprite1), b = getCollisionBox(sprite2);
	//�������㷨 Separating Axis Theorem
	for (int i = 0; i<a.n; i++)//��aÿһ���߼���
	{
		VEC axis = perp(a.edges[i].dir); //��ȡ�߷�������
		float *a_ = calProjection(a, axis), *b_ = calProjection(b, axis); // ��ȡa��b�ڷ����ϵ�ͶӰ
		distanceA = overlap(a_, b_);
		if (!distanceA)//���û�غ�
		{
			delete[] a_, delete[] b_;
			return 0; //���غϲ���ײ
		}
		else
		{
			delete[] a_, delete[] b_;
		}
	}
	for (int i = 0; i<b.n; i++) //��b��ÿ����Ҳ��һ��
	{
		VEC axis = b.edges[i].dir;
		axis = perp(axis);
		float *a_ = calProjection(a, axis), *b_ = calProjection(b, axis);
		distanceB = overlap(a_, b_);
		if (!distanceB)
		{
			delete[] a_, delete[] b_;
			return 0;
		}
		else
		{
			delete[] a_, delete[] b_;
		}
	}
	return distanceA < distanceB ? distanceA : distanceB;
}
//���ڱ߽����ײ����
int CollisionRect(SPRITE sprite1, SPRITE sprite2)
{
	RECT rect1;
	rect1.left = (long)sprite1.x;
	rect1.top = (long)sprite1.y;
	rect1.right = (long)sprite1.x + sprite1.width * sprite1.scaling;
	rect1.bottom = (long)sprite1.y + sprite1.height * sprite1.scaling;
	RECT rect2;
	rect2.left = (long)sprite2.x;
	rect2.top = (long)sprite2.y;
	rect2.right = (long)sprite2.x + sprite2.width * sprite2.scaling;
	rect2.bottom = (long)sprite2.y + sprite2.height * sprite2.scaling;
	RECT dest;
	return IntersectRect(&dest,&rect1,&rect2);
}
//���ھ�����ײ����
bool CollisionDistance(SPRITE sprite1, SPRITE sprite2)
{
	double radius1, radius2;
	radius1 = (sprite1.width > sprite1.height ? sprite1.width : sprite1.height) * sprite1.scaling / 2.0;
	radius2 = (sprite2.width > sprite2.height ? sprite2.width : sprite2.height) * sprite2.scaling / 2.0;
	double center1x = (double)sprite1.x + radius1,center1y = (double)sprite1.y + radius1,center2x = (double)sprite2.x + radius2,center2y = (double)sprite2.y + radius2;
	//D3DXVECTOR2 vector1(center1x, center1y), vector2(center2x, center2y);
	double deltaX = center1x - center2x, deltaY = center1y - center2y;
	return (sqrt((deltaX * deltaX) + (deltaY * deltaY)) < radius1 + radius2);
}

//������ع���

//�½��������
LPD3DXFONT MakeFont(string name, int size)
{
	LPD3DXFONT font;
	D3DXFONT_DESC desc = {
		size,					//�߶�
		0,0,					//��ȣ�����
		0,						//miplevels,0��D3DX_DEAFULT�ᴴ����1������ռ�����Ļ�ռ��ӳ����ͬ
		false,					//б��
		DEFAULT_CHARSET,		//�ַ���
		OUT_TT_PRECIS,			//�������
		CLIP_DEFAULT_PRECIS,	//�������
		DEFAULT_PITCH,			//��б������family
		'\0'					//������
	};
	strcpy_s(desc.FaceName, name.c_str());
	D3DXCreateFontIndirect(d3ddev, &desc, &font);
	return font;
}
//��ӡ�ı�
void FontPrint(LPD3DXFONT font, int x, int y, string text, D3DCOLOR color)
{
	//���ֱ߽�
	RECT rect = { x,y,0,0 };
	font->DrawText(NULL, text.c_str(), text.length(), &rect, DT_CALCRECT, color);
	font->DrawText(spriteobj, text.c_str(), text.length(), &rect, DT_LEFT, color);
}

//��Ƶ����

//��Ƶ����
CSoundManager *dsound = NULL;
//��ʼ����Ƶ
bool DirectSoundInit(HWND hwnd)
{
	//create DirectSound manager object
	dsound = new CSoundManager();

	//initialize DirectSound
	HRESULT result;
	result = dsound->Initialize(hwnd, DSSCL_PRIORITY);
	if (result != DS_OK) return false;

	//set the primary buffer format
	result = dsound->SetPrimaryBufferFormat(2, 22050, 16);
	if (result != DS_OK) return false;

	//return success
	return true;
}
//�ر���Ƶ
void DirectSoundShutdown()
{
	SafeDelete(dsound);
}
//װ����Ƶ
CSound * LoadSound(string filename)
{
	HRESULT result;
	//create local reference to wave data
	CSound *wave = NULL;
	//attempt to load the wave file
	char s[255];
	sprintf_s(s, "%s", filename.c_str());
	result = dsound->Create(&wave, s);
	if (result != DS_OK) 
		wave = NULL;
	//return the wave
	return wave;
}
//������Ƶ
void PlaySound(CSound * sound)
{
	sound->Play();
}
//ѭ����Ƶ
void LoopSound(CSound * sound)
{
	sound->Play(0, DSBPLAY_LOOPING);
}
//ֹͣ��Ƶ
void StopSound(CSound * sound)
{
	sound->Stop();
}

//��ȡ�ļ�
MAPDATA GetMap(string datapath)//�����ǲ�����Լ��ĺ���дע�ͣ�
{
	char workbench[256];
	string ThisNum = "";
	int size[2];
	int i = 0, numCounter = 0, columns = 0;
	ifstream ReadFile;
	ReadFile.open(datapath, ios::in);
	if (ReadFile.fail())
	{
		MAPDATA mapdata;
		mapdata.width = mapdata.height = -1;
		return mapdata;
	}
	ReadFile.getline(workbench, 256, '\n');
	while (workbench[i] != '\0')
	{
		if (workbench[i] >= '0'&& workbench[i] <= '9')
		{
			ThisNum += workbench[i];
			if (!(workbench[i + 1] >= '0'&& workbench[i + 1] <= '9'))
			{
				size[numCounter++] = stoi(ThisNum);
				ThisNum = "";
			}
		}
		if (numCounter> 1)
			break;
		i++;
	}
	MAPDATA mapdata(size[0], size[1]);
	numCounter = 0;
	while (!ReadFile.eof())
	{
		i = 0;
		ReadFile.getline(workbench, 256, '\n');

		numCounter = mapdata.width*columns;
		while (workbench[i] != '\0')
		{
			if (workbench[i] >= '0'&& workbench[i] <= '9')
			{
				ThisNum += workbench[i];
				if (!(workbench[i + 1] >= '0'&& workbench[i + 1] <= '9'))
				{
					mapdata.map[numCounter++] = stoi(ThisNum)-1;
					ThisNum = "";
				}
			}
			i++;
		}
		columns++;
	}
	return mapdata;
}

//3Dģ�͹���

//����ģ��
void DrawModel(MODEL * model)
{
	//any materials in this mesh?
	if (model->material_count == 0)
	{
		model->mesh->DrawSubset(0);
	}
	else {
		//draw each mesh subset
		for (DWORD i = 0; i < model->material_count; i++)
		{
			// Set the material and texture for this subset
			d3ddev->SetMaterial(&model->materials[i]);

			if (model->textures[i])
			{
				if (model->textures[i]->GetType() == D3DRTYPE_TEXTURE)
				{
					D3DSURFACE_DESC desc;
					model->textures[i]->GetLevelDesc(0, &desc);
					if (desc.Width > 0)
					{
						d3ddev->SetTexture(0, model->textures[i]);
					}
				}
			}

			// Draw the mesh subset
			model->mesh->DrawSubset(i);
		}
	}
}
//ɾ��ģ��
void DeleteModel(MODEL * model)
{
	//remove materials from memory
	if (model->materials != NULL)
		delete[] model->materials;

	//remove textures from memory
	if (model->textures != NULL)
	{
		for (DWORD i = 0; i < model->material_count; i++)
		{
			if (model->textures[i] != NULL)
				model->textures[i]->Release();
		}
		delete[] model->textures;
	}

	//remove mesh from memory
	if (model->mesh != NULL)
		model->mesh->Release();

	//remove model struct from memory
	if (model != NULL)
		free(model);
}
//װ��ģ��
MODEL * LoadModel(string filename)
{
	MODEL *model = (MODEL*)malloc(sizeof(MODEL));
	LPD3DXBUFFER matbuffer;
	HRESULT result;
	//load mesh from the specified file
	result = D3DXLoadMeshFromX(
		filename.c_str(),               //filename
		D3DXMESH_SYSTEMMEM,     //mesh options
		d3ddev,                 //Direct3D device
		NULL,                   //adjacency buffer
		&matbuffer,             //material buffer
		NULL,                   //special effects
		&model->material_count, //number of materials
		&model->mesh);          //resulting mesh
	if (result != D3D_OK)
	{
		MessageBox(0, "Error loading model file", APPTITLE.c_str(), 0);
		return NULL;
	}
	//extract material properties and texture names from material buffer
	LPD3DXMATERIAL d3dxMaterials = (LPD3DXMATERIAL)matbuffer->GetBufferPointer();
	model->materials = new D3DMATERIAL9[model->material_count];
	model->textures = new LPDIRECT3DTEXTURE9[model->material_count];
	//create the materials and textures
	for (DWORD i = 0; i<model->material_count; i++)
	{
		//grab the material
		model->materials[i] = d3dxMaterials[i].MatD3D;
		//set ambient color for material 
		model->materials[i].Ambient = model->materials[i].Diffuse;
		model->textures[i] = NULL;
		if (d3dxMaterials[i].pTextureFilename != NULL)
		{
			string filename = d3dxMaterials[i].pTextureFilename;
			if (FindFile(&filename))
			{
				result = D3DXCreateTextureFromFile(d3ddev, filename.c_str(), &model->textures[i]);
				if (result != D3D_OK)
				{
					MessageBox(NULL, "Could not find texture file", "Error", MB_OK);
					return false;
				}
			}
		}
	}
	//done using material buffer
	matbuffer->Release();
	return model;
}
//��һ���ĸ�������
bool DoesFileExist(const string & filename)
{
	return (_access(filename.c_str(), 0) != -1);
}
//�����ļ��Ƿ����
bool FindFile(string * filename)
{
	if (!filename) return false;

	//look for file using original filename and path
	if (DoesFileExist(*filename)) 
		return true;
	//since the file was not found, try removing the path
	string pathOnly;
	string filenameOnly;
	SplitPath(*filename, &pathOnly, &filenameOnly);

	//is file found in current folder, without the path?
	if (DoesFileExist(filenameOnly))
	{
		*filename = filenameOnly;
		return true;
	}
	//if not found
	return false;
}
//ȥ��ԭ·��
void SplitPath(const string & inputPath, string * pathOnly, string * filenameOnly)
{
	string fullPath(inputPath);
	replace(fullPath.begin(), fullPath.end(), '\\', '/');
	string::size_type lastSlashPos = fullPath.find_last_of('/');

	// check for there being no path element in the input
	if (lastSlashPos == string::npos)
	{
		*pathOnly = "";
		*filenameOnly = fullPath;
	}
	else {
		if (pathOnly) {
			*pathOnly = fullPath.substr(0, lastSlashPos);
		}
		if (filenameOnly)
		{
			*filenameOnly = fullPath.substr(
				lastSlashPos + 1,
				fullPath.size() - lastSlashPos - 1);
		}
	}
}
//���þ�ͷλ��
void SetCamera(float posx, float posy, float posz, float lookx, float looky, float lookz)
{
	float fov = D3DX_PI / 4.0;
	float aspectRatio = (float)SCREENW / (float)SCREENH;
	float nearRange = 1.0;
	float farRange = 2000.0;
	D3DXVECTOR3 updir = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	D3DXVECTOR3 position = D3DXVECTOR3(posx, posy, posz);
	D3DXVECTOR3 target = D3DXVECTOR3(lookx, looky, lookz);

	//set the perspective 
	D3DXMATRIX matProj;
	D3DXMatrixPerspectiveFovLH(&matProj, fov, aspectRatio, nearRange, farRange);
	d3ddev->SetTransform(D3DTS_PROJECTION, &matProj);

	//set up the camera view matrix
	D3DXMATRIX matView;
	D3DXMatrixLookAtLH(&matView, &position, &target, &updir);
	d3ddev->SetTransform(D3DTS_VIEW, &matView);
}

