#include "DirectX.h"
#include <memory>//��������ָ��

int (WINAPIV * __vsnprintf)(char *, size_t, const char*, va_list) = _vsnprintf;//���LNK2019����

																			   //����
const string APPTITLE = "Tanks War";
const int SCREENW = 1024;//��Ļ���
const int SCREENH = 768;//��Ļ�߶�
const int TankNum = 10;//̹����������
const int ShellNum = 300;//�ڵ���������
const int TILEWIDTH = 64;//Ҫ��ʾ��λ����
const int TILEHEIGHT = 64;//λ��߶�

						  //debug����
LPD3DXFONT fontTest36 = NULL;
//��������
LPD3DXFONT fontGaramond50 = NULL;

enum game_state
{
	init = 0,
	menu = 1,
	ingame = 2,
	pause = 3,
	dead = 4,
	config = 5
};
game_state gamestate = init;

//�ڶ��ֶ��㣬��һ������Ҫ���պ�ͶӰ�任��ɾ��
static const unsigned short TEXTURE_FVF = D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_DIFFUSE;// �����ʽ
struct VERTEX2	//�����Լ��Ķ���ṹ��
{

	float x, y, z, rhw;	// ��������λ��
	unsigned long color;// ��ɫ
	float u, v;// ���������ڵ�λ��
			   //���ö�������
	VERTEX2(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f, float _u = 0.0f, float _v = 0.0f, float _rhw = 1.0f)
	{
		x = _x, y = _y, z = _z, u = _u, v = _v, rhw = _rhw;
		color = D3DCOLOR_ARGB(255, 255, 255, 255);
	}
};

//�ı���
struct QUAD2
{
	VERTEX2 vertices[4];//�����Ա����
	LPDIRECT3DTEXTURE9 texture;//����ָ��
	LPDIRECT3DVERTEXBUFFER9 buffer;//��Ⱦ������ָ��
								   //��ƺ���
	QUAD2(const QUAD2 &quad)
	{
		for (int i = 0; i < 4; i++)
			vertices[i] = quad.vertices[i];
		texture = quad.texture;
		d3ddev->CreateVertexBuffer(
			4 * sizeof(VERTEX2),
			0,
			TEXTURE_FVF, D3DPOOL_DEFAULT,
			&buffer,
			NULL);
	}
	//���ظ�ֵ����
	QUAD2 operator=(const QUAD2 &quad)
	{
		for (int i = 0; i < 4; i++)
			vertices[i] = quad.vertices[i];
		texture = quad.texture;//�Ȳ��ͷţ���һ�б���ķ�������
		buffer->Release();//
		d3ddev->CreateVertexBuffer(
			4 * sizeof(VERTEX2),
			0,
			TEXTURE_FVF, D3DPOOL_DEFAULT,
			&buffer,
			NULL);
	}
	//�вι��캯��
	QUAD2(char *textureFilename, double width = 0, double height = 0, int screenW = 0, int screenH = 0)
	{
		//װ������
		D3DXCreateTextureFromFile(d3ddev, textureFilename, &texture);
		//Ĭ�ϴ�С
		if (width == 0 && height == 0)
		{
			D3DXVECTOR2 inf = GetBitmapSize(textureFilename);
			width = inf.x, height = inf.y;
		}
		//�������㻺����
		d3ddev->CreateVertexBuffer(
			4 * sizeof(VERTEX2),
			0,
			TEXTURE_FVF, D3DPOOL_DEFAULT,
			&buffer,
			NULL);
		double left = (screenW == 0) ? 0 : 0.5*((width < screenW) ? screenW - width : 0);
		double right = (left == (double)0) ? width : left + width;
		double up = (screenH == 0) ? 0 : 0.5*((height < screenH) ? screenH - height : 0);
		double down = (up == (double)0) ? height : up + height;
		//create the four corners of this dual triangle strip
		//each vertex is X,Y,Z and the texture coordinates U,V
		vertices[0] = VERTEX2(left, up, 0.0f, 0.0f, 0.0f);
		vertices[1] = VERTEX2(right, up, 0.0f, 1.0f, 0.0f);
		vertices[2] = VERTEX2(left, down, 0.0f, 0.0f, 1.0f);
		vertices[3] = VERTEX2(right, down, 0.0f, 1.0f, 1.0f);
	}
};
//��˸�ı���
struct SHINYQUAD :QUAD2
{
	int timetick, startkick;
	unsigned int alpha;
	SHINYQUAD(char *textureFilename, double width, double height, int screenW, int screenH) :QUAD2(textureFilename, width, height, screenW, screenH)
	{
		timetick = startkick = (int)GetTickCount();
		alpha = 1;
		for (int i = 0; i < 4; i++)
			vertices[i].color = D3DCOLOR_ARGB(alpha, 255, 255, 255);
	}
	void animate()
	{
		int now = (int)GetTickCount();
		if (now - timetick > 2)//������ʱ����
		{
			timetick = now;
			int live = now - startkick;//������ʱ��
			live = live / 1024;//�����ʱ���
			switch (live)
			{
			case 0:
				alpha = alpha < 255 ? alpha + 4 : 255;
				break;
			case 1:
				alpha = 255;
				break;
			case 2:
				alpha = alpha > 0 ? alpha - 4 : 0;
				break;
			default:
				alpha = 0;
				gamestate = menu;//״̬ת��
				break;
			}
			if (alpha > 25000)//alpha���絽�����
				alpha = 0;
			if (alpha > 255)
				alpha = 255;
			for (int i = 0; i < 4; i++)
				vertices[i].color = D3DCOLOR_ARGB(alpha, 255, 255, 255);//���ö�����ɫ
		}
	}
};
//̹�˽ṹ
struct TANK :SPRITE
{
	int rotateX = 39, rotateY = 32;//���������������ӵ�
	int turretX = 30, turretY = 46;//��������ת����
	int turretWidth = 50, turretHeight = 76;//������ȸ߶�
	float acceleration = 0.0f, turretRotation = 0.0f, fx, fy;//���ٶȣ��������򣬸���xλ�ã�����yλ��
	float frontacc = 3.0f, reverseacc = 1.5f, accpers = 0.3f;//ǰ������ٶȣ���������ٶȣ�����Ч��
	float rOffsetX = 0, rOffsetY = 0;//�����ڵ���λ�ò���
	int lastshoot = 0, HP = 0, MaxHP = 10;//�����ʱ�䣬Ѫ��
	bool alive;//�Ƿ���
			   //shared_ptr<LPDIRECT3DTEXTURE9>imgchassis;
	TANK();
	void TankTransformDraw(LPDIRECT3DTEXTURE9 image, int frame, int columns, float scaling, D3DCOLOR color);
	bool FireShell(int type);
};
//�ڵ��ṹ
struct SHELL :SPRITE
{
	int shellWidth = 250, shellHeight = 232;//ͼ����Դͼ�ϵ�ƫ��
	float acceleration = 10.0f, fx = 0.0f, fy = 0.0f;//�ٶȣ�������ʾ����
	bool alive = false;//�Ƿ���
	TANK *father;//������
	int type;//�ͺ�
	SHELL();
	SHELL(float ix, float iy, float d, TANK *f, int shelltype = 1);
};

//������ֵ������
vector<SPRITE>expo;//��ը����洢����
TANK tanks[TankNum];//̹�˴洢����
SHELL shells[ShellNum];//�ڵ��洢����
SPRITE ingameQuit;//��Ϸ״̬���˳�ͼ��
SPRITE opinion1, opinion2;
int ScrollX = 0, ScrollY = 0;//�����ͼλ������
int SpeedX, SpeedY, GAMEWORLDWIDTH, GAMEWORLDHEIGHT;//���������������ٶȣ���ͼ��ȡ��߶�
unsigned long createtime;//������ݲ���ʱ��
unsigned long addtime;//��һ��AI̹�˲���ʱ��
bool flag_control = false;
int enemyfireinterval = 1000;

//ͼ����Դ
LPDIRECT3DTEXTURE9 imgexplosion = NULL;//��ըͼƬ
D3DXVECTOR2 inforexplo;//��ըͼ���С��Ϣ
LPDIRECT3DTEXTURE9 imgsource = NULL;//̹�˼��ڵ�ͼƬ
LPDIRECT3DSURFACE9 gameworld = NULL;//��Ϸ��ͼ
LPDIRECT3DTEXTURE9 imgmenuBG = NULL;//�˵�
LPDIRECT3DTEXTURE9 imgmenuST = NULL;//��ʼ��
LPDIRECT3DTEXTURE9 imgmenuCG = NULL;//���ü�
LPDIRECT3DTEXTURE9 imgmenuQT = NULL;//�˳���
LPDIRECT3DTEXTURE9 imgreturn = NULL;//���ز˵���
LPDIRECT3DTEXTURE9 imgdead = NULL;//��������
LPDIRECT3DTEXTURE9 imgconfig = NULL;//���û���
LPDIRECT3DTEXTURE9 imgopinion = NULL;//ѡ����
SHINYQUAD *sq = NULL;//��������

					 //�����ı���
void DrawQuad2(QUAD2 *quad)
{
	//ʹ���ı��ζ�����䶥�㻺����
	void *temp = NULL;
	quad->buffer->Lock(0, sizeof(quad->vertices), (void**)&temp, 0);//���������
	memcpy(temp, quad->vertices, sizeof(quad->vertices));//���Ƶ�Ŀ������
	quad->buffer->Unlock();//����

						   //����������������ɵ��ı���
	d3ddev->SetTexture(0, quad->texture);//ָ������
	d3ddev->SetStreamSource(0, quad->buffer, 0, sizeof(VERTEX2));//ָ����Դ

																 // ��������Ͷ�����
	d3ddev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);//��һ���ϲ�����������
	d3ddev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);//�ڶ����ϲ������Զ�����ɫ
	d3ddev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);//���Ʒ�ʽ�����

																	 // ����Alpha���
	d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);//����͸�����
	d3ddev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);//�趨Դ�������
	d3ddev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);//�趨Ŀ��������
	d3ddev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);//�������������Σ�����Ϊ2
	d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);//�ر�͸�����
}

bool addExplosion(vector<SPRITE>& expo, int x, int y)
{
	SPRITE ex;
	ex.width = inforexplo.x / 8, ex.height = inforexplo.y / 6;
	ex.x = x - ex.width / 2, ex.y = y - ex.height / 2;
	ex.columns = 8;
	ex.startframe = ex.frame = 0, ex.starttime = GetTickCount(), ex.endframe = 47;
	expo.push_back(ex);
	return true;
}

void ExplosionAnimate(int & frame, int startFrame, int endFrame, int direction, int & startTime, int delay)
{
	if ((int)GetTickCount() > startTime + delay)
	{
		startTime = GetTickCount();
		frame += direction;
		if (frame < startFrame)
			frame = startFrame;
	}
}

void updateExplosion(vector<SPRITE> &expo)
{
	//vector<SPRITE>::iterator iter = expo.begin();
	//for (auto iter : expo)������ȡiter��
	if (expo.size() > 0)
	{
		for (auto iter = expo.begin(); iter != expo.end(); iter++)
		{
			ExplosionAnimate(iter->frame, iter->startframe, iter->endframe, 1, iter->starttime, 10);//��ÿ����ը�������һ֡
		}
		for (auto i = expo.begin(); i != expo.end();)
		{
			if (i->frame > i->endframe)
			{
				i = expo.erase(i);
				if (i == expo.end())
					break;
			}
			else
			{
				SpriteDrawFrame(imgexplosion, i->x - ScrollX, i->y - ScrollY, i->frame, i->width, i->height, i->columns);//���Ʊ�ը����
				i++;
			}
		}

	}
}

//̹��
int newTank(int ix, int iy, bool player = false, int num = 0, int iwidth = 78, int iheight = 78)
{
	int n = 0;
	if (!player)
	{
		if (num != 0)//���������ָ�������
			n = num;
		else//������ң����Ϊ0������û�ֶ�ָ�����
		{
			for (int i = 1; i < 10; i++)
			{
				if (!tanks[i].alive)
				{
					n = i;
					break;
				}
			}
		}
		if (n == 0)
			return -1;
	}
	tanks[n].x = ix, tanks[n].y = iy;
	tanks[n].fx = (float)tanks[n].x, tanks[n].fy = (float)tanks[n].y;
	tanks[n].width = iwidth, tanks[n].height = iheight;
	tanks[n].frame = 0, tanks[n].columns = 6;
	tanks[n].startframe = 0, tanks[n].endframe = 10;
	tanks[n].direction = -1, tanks[n].HP = 10;
	tanks[n].alive = true;
	tanks[n].lastshoot = (int)GetTickCount();//����ʱ��Ҫ��������
	if (player)
	{
		createtime = GetTickCount();//��¼����ʱ��
	}
	else
	{
		addtime = GetTickCount();//��¼����ʱ��
	}
	return n;
}

TANK::TANK()
{
	alive = false;
	fx = fy = 0.0f, x = y = 0;
	HP = 0;
	//direction = -1;
}

void TANK::TankTransformDraw(LPDIRECT3DTEXTURE9 image, int frame, int columns, float scaling, D3DCOLOR color)
{
	//���Ƶ���
	D3DXVECTOR2 scale(scaling, scaling);//��������
	D3DXVECTOR2 trans(this->x - ScrollX, this->y - ScrollY);//����λ��
	D3DXVECTOR2 center((float)(this->width*scaling) / 2, (float)(this->height*scaling) / 2);//��ת����
	D3DXMATRIX mat;
	D3DXMatrixTransformation2D(&mat, NULL, 0, &scale, &center, (float)toRadians(direction), &trans);//�任����
	spriteobj->SetTransform(&mat);
	int cx = (frame % this->columns)*this->width;//Դλ��
	int cy = (frame / this->columns)*this->height;
	RECT srcRect = { cx, cy, cx + width, cy + height };
	spriteobj->Draw(image, &srcRect, NULL, NULL, color);
	//��������
	rotation = toRadians(direction);//����
	rOffsetX = sin(toRadians(direction)) * 7, rOffsetY = cos(toRadians(direction)) * 7;//������ת����Ե������ĵĲ�ֵ
	D3DXVECTOR2 ttrans(fx + (rotateX + rOffsetX - 30)*scaling - ScrollX, fy + (rotateY - rOffsetY + 7 - 46)*scaling - ScrollY);
	D3DXVECTOR2 tcenter((float)turretX*scaling, (float)turretY*scaling);
	D3DXMATRIX tmat;
	D3DXMatrixTransformation2D(&tmat, NULL, 0, &scale, &tcenter, turretRotation, &ttrans);
	spriteobj->SetTransform(&tmat);
	int tx = (frame % this->columns)*turretWidth;
	int ty = (frame / this->columns)*turretHeight + 156;//��ԴͼƬ������ƫ����
	RECT turRect = { tx, ty, tx + turretWidth, ty + turretHeight };
	spriteobj->Draw(image, &turRect, NULL, NULL, color);
	D3DXMatrixIdentity(&mat);//�ָ���λ����
	spriteobj->SetTransform(&mat);
}

bool DrawTanks(TANK tanks[])
{
	for (int n = 0; n < TankNum; n++)
	{
		if (tanks[n].alive)
		{
			tanks[n].TankTransformDraw(imgsource, tanks[n].frame, 6, 1.0f, D3DCOLOR_XRGB(255, 255, 255));
		}
	}
	return true;
}

double GetDistance(SPRITE& s1, SPRITE& s2)
{
	float mx = s1.x - s2.x, my = s1.y - s2.y;
	double hydra = sqrt(mx*mx + my*my);
	return hydra;
}

void UpdateTanks(TANK tanks[])
{
	//ѭ������̹�˳�Ա
	for (int i = 0; i < 10; i++)
	{
		if (tanks[i].alive)
		{
			//����λ��
			tanks[i].fx += sin((double)toRadians(tanks[i].direction))*tanks[i].acceleration;
			tanks[i].fy -= cos((double)toRadians(tanks[i].direction))*tanks[i].acceleration;
			//�߽���
			if (tanks[i].fx < 0.0f)
				tanks[i].fx = 0.0f;
			if (tanks[i].fy < 0.0f)
				tanks[i].fy = 0.0f;
			if (tanks[i].fx >(float)GAMEWORLDWIDTH - tanks[i].width)
				tanks[i].fx = (float)GAMEWORLDWIDTH - tanks[i].width;
			if (tanks[i].fy >(float)GAMEWORLDHEIGHT - tanks[i].height)
				tanks[i].fy = (float)GAMEWORLDHEIGHT - tanks[i].height;
			//��ײ���
			for (int j = i + 1; j < 10; j++)
			{
				float mx = tanks[i].fx - tanks[j].fx, my = tanks[i].fy - tanks[j].fy;
				double hypot = sqrt(mx*mx + my*my);
				if (hypot < 111)//����С��һ��ֵ����ײ���
				{
					int leave = CollisionSAT(tanks[i], tanks[j]);
					if (leave)
					{
						float mx = tanks[i].fx - tanks[j].fx, my = tanks[i].fy - tanks[j].fy;
						double hypot = sqrt(mx*mx + my*my);
						tanks[i].fx += mx / hypot*0.5*leave, tanks[i].fy += my / hypot*0.5*leave;
						tanks[j].fx -= mx / hypot*0.5*leave, tanks[j].fy -= my / hypot*0.5*leave;
						tanks[i].acceleration *= 0.5, tanks[j].acceleration *= 0.5;
					}
				}
			}
			//Ħ��������,��Ħ����Ϊ0.1f
			if (tanks[i].acceleration > -0.1f&&tanks[i].acceleration < 0.1f)//����ٶȵ���Ħ����������ͣ��
				tanks[i].acceleration = 0.0f;
			if (tanks[i].acceleration>0.1f)//����ǰ��������Ħ����
				tanks[i].acceleration -= 0.1f;
			if (tanks[i].acceleration<-0.1f)//����������Ħ����
				tanks[i].acceleration += 0.1f;
			//��ֵ����
			if (tanks[i].direction < 0)
				tanks[i].direction += 360;
			if (tanks[i].direction > 360)
				tanks[i].direction -= 360;
			//������Ļ��ʾλ��
			tanks[i].x = (int)tanks[i].fx, tanks[i].y = (int)tanks[i].fy;
		}
	}
}

void AnimateTank(TANK& tank, int type)
{
	if (type == 2)
		tank.HP -= 2;
	else
	{
		tank.HP--;
	}
	if (tank.HP <= 0)
	{
		//tank.alive = false;
		addExplosion(expo, tank.x + tank.width / 2, tank.y + tank.height / 2);
		if (&tanks[0] != &tank)
			tank = TANK();//�����޲ң�ȷ�ţ����캯����̹����ʧ
		else//���
		{
			gamestate = dead;
			if (GetTickCount()>createtime)
				createtime = GetTickCount() + 10000;//10���Ժ��
			tanks[0].alive = false;
		}
	}
	tank.frame = tank.endframe - (int)((float)tank.HP / (float)tank.MaxHP*tank.endframe - tank.startframe);
	if (tank.frame < tank.startframe)
		tank.frame = tank.startframe;
	if (tank.frame > tank.endframe)
		tank.frame = tank.endframe;
}

bool TANK::FireShell(int type)
{
	bool shell = false;
	if ((int)timeGetTime() < lastshoot + 500 || alive == false)
		return shell;
	for (int n = 0; n<ShellNum; n++)
	{
		if (!(shells[n].alive))
		{
			shells[n] = SHELL((int)(fx + rotateX + rOffsetX + sin(turretRotation - atan2(3, 30))*30.1496), (int)fy + rotateY - rOffsetY + 7 - (cos(turretRotation - atan2(3, 30)))*30.1496, turretRotation, this, type);
			lastshoot = timeGetTime();
			shell = true;
			break;
		}
	}
	return shell;
}

//�ڵ�
SHELL::SHELL()
{
	x = y = rotation = 0;
	fx = fy = 0.0f;
	alive = false;
	father = NULL;
	type = 1;
}

SHELL::SHELL(float ix, float iy, float d, TANK *f, int shelltype)
{
	fx = ix, fy = iy, rotation = d;
	x = (int)ix, y = (int)iy;
	width = 12, height = 18;
	direction = toDegrees(d);
	alive = true;
	father = f;
	type = shelltype;
}

void UpdateShells(SHELL shells[])
{
	for (int n = 0; n < ShellNum; n++)
	{
		if (shells[n].alive)
		{
			shells[n].fx += sin((double)shells[n].rotation)*shells[n].acceleration;
			shells[n].fy -= cos((double)shells[n].rotation)*shells[n].acceleration;
			if (shells[n].fx < 0.0f || shells[n].fy < 0.0f || shells[n].fx >(float)GAMEWORLDWIDTH - shells[n].width || shells[n].fy >(float)GAMEWORLDHEIGHT - shells[n].height)
				shells[n] = SHELL();
			for (int t = 0; t < TankNum; t++)
			{
				if (GetDistance(tanks[t], shells[n]) < 128)//һ���������ϲ������ײ
				{
					if (CollisionSAT(tanks[t], shells[n]) && shells[n].father != &tanks[t])
					{
						AnimateTank(tanks[t], shells[n].type);
						shells[n] = SHELL();
					}
				}
			}
			shells[n].x = (int)shells[n].fx, shells[n].y = (int)shells[n].fy;
		}
	}

}

void ShellTransformDraw(LPDIRECT3DTEXTURE9 image, SHELL shell, float scaling, D3DCOLOR color)
{
	D3DXVECTOR2 scale(scaling, scaling);
	D3DXVECTOR2 trans(shell.x - (shell.width*scaling) / 2 - ScrollX, shell.y - (shell.height*scaling) / 2 - ScrollY);
	D3DXVECTOR2 center((float)(shell.width*scaling) / 2, (float)(shell.height*scaling) / 2);
	D3DXMATRIX mat;
	D3DXMatrixTransformation2D(&mat, NULL, 0, &scale, &center, shell.rotation, &trans);
	spriteobj->SetTransform(&mat);
	int fx, fy;//ͼ����Դ��ͼ��ƫ����
	if (shell.type == 2)
		fx = shell.shellWidth + shell.width;
	else
		fx = shell.shellWidth;
	fy = shell.shellHeight;
	RECT srcRect = { fx, fy, fx + shell.width, fy + shell.height };
	spriteobj->Draw(image, &srcRect, NULL, NULL, color);

	D3DXMatrixIdentity(&mat);//������λ����
	spriteobj->SetTransform(&mat);//�ָ���������ϵ
}

bool DrawShells(SHELL shells[])
{
	for (int n = 0; n < ShellNum; n++)
	{
		if (shells[n].alive)
		{
			ShellTransformDraw(imgsource, shells[n], 1.0f, D3DCOLOR_XRGB(255, 255, 255));
		}
	}
	return true;
}

//��ͼ����

//����ͼ��    ��֪bug����ͼ���С���ܱ䣬��ΪStretchRect�������ŷǻ���������
void DrawTile(LPDIRECT3DSURFACE9 source, int tilenum, int width, int height, int columns, LPDIRECT3DSURFACE9 &dest, int destX, int destY, float widthScaling, float heightScaling)
{
	RECT r1;
	r1.left = (tilenum%columns)*width;
	r1.top = (tilenum / columns)*height;
	r1.right = r1.left + width;
	r1.bottom = r1.top + height;
	RECT r2 = { destX,destY,destX + width*widthScaling,destY + height*heightScaling };
	d3ddev->StretchRect(source, &r1, dest, &r2, D3DTEXF_LINEAR);
}
//������ͼ
void BulidGameWorld(MAPDATA mapdata, string originimage)
{
	HRESULT result;//���
	int x, y;//λͼ����Դͼ�ϵ�λ��
	float widthScaling, heightScaling;
	LPDIRECT3DSURFACE9 tiles;
	D3DXVECTOR2 imageinfo;//Դͼ��Ϣ
						  //װ��λͼ��ͼ��
	tiles = LoadSurface(originimage);
	imageinfo = GetBitmapSize(originimage);
	widthScaling = TILEWIDTH / (int)(imageinfo.x / 10), heightScaling = TILEHEIGHT / (int)(imageinfo.y / 4);//��ʾ��Դͼ�����ű���
																											//10��4��Դͼ�Ϻ��������ͼƬ������
	GAMEWORLDWIDTH = TILEWIDTH * mapdata.width, GAMEWORLDHEIGHT = TILEHEIGHT * mapdata.height;
	//����������ͼ
	result = d3ddev->CreateOffscreenPlainSurface(
		GAMEWORLDWIDTH,//�����ȣ����*������
		GAMEWORLDHEIGHT,//����߶�
		D3DFMT_X8R8G8B8,
		D3DPOOL_DEFAULT,
		&gameworld,  //ָ������ָ��
		NULL);
	if (result != D3D_OK)
	{
		MessageBox(NULL, "Error creating working surface!", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	//��λͼ������ͼͼ��
	for (y = 0; y < mapdata.height; y++)
		for (x = 0; x < mapdata.width; x++)
			DrawTile(tiles, mapdata.map[y * mapdata.width + x], (int)(imageinfo.x / 10), (int)(imageinfo.y / 4), 10, gameworld, x * TILEWIDTH, y * TILEHEIGHT, widthScaling, heightScaling);
	//�ͷ�Դͼ
	tiles->Release();
}
//��������
void ScrollScreen(SPRITE player)
{
	//���º������λ�ú��ٶ�
	ScrollX = player.x + player.width*0.5*player.scaling - SCREENW*0.5;
	if (ScrollX < 0)
	{
		ScrollX = 0;
	}
	else if (ScrollX > GAMEWORLDWIDTH - SCREENW)
		ScrollX = GAMEWORLDWIDTH - SCREENW;
	//�����������λ�ú��ٶ�
	ScrollY = player.y + player.height*0.5*player.scaling - SCREENH*0.5;
	if (ScrollY < 0)
	{
		ScrollY = 0;
	}
	else if (ScrollY > GAMEWORLDHEIGHT - SCREENH)
	{
		ScrollY = GAMEWORLDHEIGHT - SCREENH;
	}
	RECT r1 = { ScrollX,ScrollY,ScrollX + SCREENW - 1,ScrollY + SCREENH - 1 };
	RECT r2 = { 0,0,SCREENW - 1,SCREENH - 1 };
	d3ddev->StretchRect(gameworld, &r1, backbuffer, &r2, D3DTEXF_NONE);
}
//�������˵�
void DrawMenu()
{
	LPDIRECT3DSURFACE9 sur = NULL;
	imgmenuBG->GetSurfaceLevel(0, &sur);
	d3ddev->StretchRect(sur, NULL, backbuffer, NULL, D3DTEXF_LINEAR);
	SpriteTransformDraw(imgmenuST, 28, 360, 367, 88);
	SpriteTransformDraw(imgmenuCG, 28, 484, 367, 88);
	SpriteTransformDraw(imgmenuQT, 28, 612, 367, 88);
}
//̹��AI����
bool TanksAI(TANK tanks[])
{
	for (int i = 1; i < 10; i++)//tanks[0]��������ݲ��ܿ���
	{
		if (tanks[i].alive)//�Ի��ŵ�̹��
		{
			int target = -1;
			double distance = 10000;
			for (int j = 0; j < 10; j++)
			{
				if (i == j || tanks[j].alive == false)
					continue;
				double d = GetDistance(tanks[i], tanks[j]);
				if (d < distance)
				{
					target = j;
					distance = d;
				}
			}
			if (target != -1)
			{
				tanks[i].turretRotation = atan2(tanks[target].y - (tanks[i].y + tanks[i].rotateY), tanks[target].x - (tanks[i].x + tanks[i].rotateX)) + PI / 2;
				tanks[i].direction += toDegrees(tanks[i].turretRotation) < tanks[i].direction ? -1 : 1;
				if (abs(tanks[i].direction - toDegrees(tanks[i].turretRotation))<10 && distance>350)
				{
					if (tanks[i].acceleration <= tanks[i].frontacc)
						tanks[i].acceleration += tanks[i].accpers;
				}
				if (distance < 350)
				{
					tanks[i].acceleration = 0;
				}
			}
			else
			{
				tanks[i].turretRotation = toRadians(tanks[i].direction);
			}
			if (GetTickCount() - tanks[i].lastshoot > enemyfireinterval)
			{
				tanks[i].FireShell(1);
			}
		}
		else//���˵Ŀ�λ��
		{
			if (GetTickCount() - addtime > 5000)
			{
				int px, py;
				bool run = true;
				while (run)
				{
					px = rand() % GAMEWORLDWIDTH;
					py = rand() % GAMEWORLDHEIGHT;
					SPRITE sp;
					sp.x = px, sp.y = py, sp.width = sp.height = 78;
					for (int i = 0; i < 10; i++)
					{
						if (tanks[i].alive == false)
							continue;
						if (!CollisionRect(sp, tanks[i]))
							run = false;
					}
				}
				newTank(px, py);
			}
		}
	}
	return true;
}

bool ClickArea(int x, int y, RECT rect)
{
	if (x > rect.left&&x<rect.right&&y>rect.top&&y < rect.bottom)
		return true;
	return false;
}
//���˵��������
int ClickMenuButton(HWND hwnd)
{
	int x = MousePositionX(hwnd), y = MousePositionY(hwnd);
	if (ClickArea(x, y, RECT{ 28,336,395,424 }))
		return 1;
	if (ClickArea(x, y, RECT{ 28,460,395,548 }))
		return 2;
	if (ClickArea(x, y, RECT{ 28,559,395,676 }))
		return 3;
	/*
	if (x > 28 && x < 395)
	{
	if (y > 360 && y < 448)
	return 1;
	if (y > 484 && y < 572)
	return 2;
	if (y > 612 && y < 700)
	return 3;
	}
	*/
	return 0;
}

void DrawGui()
{

}

void DrawConfig()
{
	switch (flag_control)
	{
	case false:
		opinion1.x = 539, opinion1.y = 306;
		break;
	case true:
		opinion1.x = 763, opinion1.y = 306;
		break;
	default:
		break;
	}
	switch (enemyfireinterval)
	{
	case 500:
		opinion2.x = 799, opinion2.y = 451;
		break;
	case 1000:
		opinion2.x = 668, opinion2.y = 451;
		break;
	case 1500:
		opinion2.x = 535, opinion2.y = 451;
		break;
	default:
		break;
	}
	SpriteTransformDraw(imgconfig, 0, 0, 1024, 768);
	SpriteTransformDraw(imgopinion, opinion1.x, opinion1.y, opinion1.width, opinion1.height);
	SpriteTransformDraw(imgopinion, opinion2.x, opinion2.y, opinion2.width, opinion2.height);
}
//��ȡ�˳���
SPRITE getQuitButton(string originimage)
{
	SPRITE sprite = GetSpriteSize(originimage, &sprite, 1, 1);
	sprite.scaling = 0.75;
	sprite.x = 800;//����ClickQuitButton�����е���ֵ����
	sprite.y = -sprite.height;//(int)(-sprite.height*sprite.scaling);
							  //ʵ����ʾ��СΪ96*96
	return sprite;
}

//�ƶ��˳���
void MoveQuitButton()
{
	switch (gamestate)
	{
	case pause:
		if (ingameQuit.y < 0)
			ingameQuit.y += 1;
		if (ingameQuit.y > 0)
			ingameQuit.y = 0;
		break;
	default:
		if (ingameQuit.y > -ingameQuit.height)
			ingameQuit.y -= 1;
		if (ingameQuit.y <= -ingameQuit.height)
			ingameQuit.y = -ingameQuit.height;
		break;
	}
}
//������ز˵�����
bool ClickQuitButton(HWND hwnd)
{
	int x = MousePositionX(hwnd), y = MousePositionY(hwnd);
	if (ClickArea(x, y, RECT{ 800,0,896,96 }))
		return true;
	return false;
}
//��Ϸ��ʼ��
bool GameInit(HWND window)
{
	//����Direct3D���
	if (!Direct3DInit(window, SCREENW, SCREENH, false))
	{
		MessageBox(window, "Error initializing DirectX3D", "Error!", MB_OK | MB_ICONERROR);
		return false;
	}
	if (!DirectInputInit(window))
	{
		MessageBox(window, "Error initializing DirectInput", "Error!", MB_OK | MB_ICONERROR);
		return false;
	}
	//debug����
	fontTest36 = MakeFont("Arial", 36);
	fontGaramond50 = MakeFont("Garamond", 50);
	//����̹��ͼ��
	imgsource = LoadTexture("tanksource.png", D3DCOLOR_XRGB(255, 0, 255));
	//imgsource = LoadTexture("tanktest.png",D3DCOLOR_XRGB(255,0,255));
	if (!imgsource)
	{
		MessageBox(window, "Error load tank image", "Error!", MB_OK | MB_ICONERROR);
		return false;
	}
	//���ر�ըͼ��
	imgexplosion = LoadTexture("explode.png", D3DCOLOR_XRGB(255, 0, 255));
	inforexplo = GetBitmapSize("explode.png");
	if (!imgexplosion)
	{
		MessageBox(window, "Error load image", "Error!", MB_OK | MB_ICONERROR);
		return false;
	}
	//���ز˵�ͼ��
	imgmenuBG = LoadTexture("MenuDemo.jpg");
	imgmenuST = LoadTexture("MenuStart.png", D3DCOLOR_XRGB(255, 0, 255));
	imgmenuCG = LoadTexture("MenuConfig.png", D3DCOLOR_XRGB(255, 0, 255));
	imgmenuQT = LoadTexture("MenuQuit.png", D3DCOLOR_XRGB(255, 0, 255));
	imgreturn = LoadTexture("IngameQuit.png", D3DCOLOR_XRGB(255, 0, 255));
	ingameQuit = getQuitButton("IngameQuit.png");
	imgdead = LoadTexture("dead.png", D3DCOLOR_XRGB(255, 0, 255));
	imgconfig = LoadTexture("config.png", D3DCOLOR_XRGB(255, 0, 255));
	imgopinion = LoadTexture("switchopinion.jpg");
	opinion1 = opinion2 = GetSpriteSize("switchopinion.jpg", &opinion1, 1, 1);

	//�趨Direct3D��Ϊ�����ʽ
	d3ddev->SetFVF(TEXTURE_FVF);
	sq = new SHINYQUAD("directx-logo.jpg", 672, 371, SCREENW, SCREENH);//װ�ؿ�ͷ����ͼ��
	expo.reserve(10);//��ը�����趨��С
	d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);//�趨�������
	BulidGameWorld(GetMap("file.txt"), "maptilesuit.png");//������ͼ
	srand((int)time(0));//�趨�������
	return true;
}
//��Ϸ��ѭ��
void GameRun(HWND window)
{
	if (!d3ddev)//û����d3d�豸����
		return;
	DirectInputUpdate();
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET || D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	//�߼�����
	string tex = "";//׼������debug��Ϣ
	switch (gamestate)
	{
	case init:
		if (sq != NULL)
		{
			sq->animate();
		}
		break;
	case menu:
		if (MouseButton(0))
		{
			switch (ClickMenuButton(window))
			{
			case 0://ʲô��û�㵽
				break;
			case 1://��ʼ��
				gamestate = ingame;//ת��״̬
				newTank(100, 100, true);//�������̹��
				ingameQuit.y = -ingameQuit.height;//�պ��˳���ť
				for (int i = 1; i < 10; i++)
				{
					tanks[i] = TANK();//��ʼ��̹������
				}
				break;
			case 2://���ü�
				gamestate = config;//״̬ת��
				break;
			case 3://�˳���
				gameover = true;
				break;
			default:
				break;
			}
		}
		break;
	case ingame:
		//�����ڵ���̹��
		UpdateShells(shells);
		UpdateTanks(tanks);
		//������Ӧ
		if (MouseButton(0))
		{
			tanks[0].FireShell(1);
		}
		if (MouseButton(1))
		{
			tanks[0].FireShell(2);
		}
		if (MouseButton(2))
		{
			for (int var = 0; var < 10; var++)
			{
				tanks[var].HP = 0;
			}
		}
		if (MouseWheel() != 0)
		{

		}
		if (!flag_control)
		{
			if (KeyDown(DIK_UP) || KeyDown(DIK_W))
			{
				if (tanks[0].acceleration <= tanks[0].frontacc)//�ٶ�û�����
					tanks[0].acceleration += tanks[0].accpers;//����
				if (tanks[0].acceleration > tanks[0].frontacc)//�ٶȵ��Ｋ��
					tanks[0].acceleration = tanks[0].frontacc;//����
			}
			if (KeyDown(DIK_DOWN) || KeyDown(DIK_S))
			{
				if (tanks[0].acceleration >= -tanks[0].reverseacc)
					tanks[0].acceleration -= tanks[0].accpers;
				if (tanks[0].acceleration < -tanks[0].reverseacc)
					tanks[0].acceleration = -tanks[0].reverseacc;
			}
			if (KeyDown(DIK_LEFT) || KeyDown(DIK_A))
			{
				tanks[0].direction -= (tanks[0].acceleration < 0 ? (-1) : 1);
			}

			if (KeyDown(DIK_RIGHT) || KeyDown(DIK_D))
			{
				tanks[0].direction += (tanks[0].acceleration < 0 ? (-1) : 1);
			}
		}
		else
		{
			int inputdirection = -1;
			if (KeyDown(DIK_UP) || KeyDown(DIK_W))
			{
				inputdirection = 0;
			}
			if (KeyDown(DIK_DOWN) || KeyDown(DIK_S))
			{
				if (inputdirection != 0)
				{
					inputdirection = 180;
				}
				else
				{
					inputdirection = -1;
				}
			}
			if (KeyDown(DIK_LEFT) || KeyDown(DIK_A))
			{
				switch (inputdirection)
				{
				case -1:
					inputdirection = 270;
					break;
				case 0:
					inputdirection = 315;
					break;
				case 180:
					inputdirection = 225;
					break;
				default:
					break;
				}
			}

			if (KeyDown(DIK_RIGHT) || KeyDown(DIK_D))
			{
				switch (inputdirection)
				{
				case -1:
					inputdirection = 90;
					break;
				case 0:
					inputdirection = 45;
					break;
				case 180:
					inputdirection = 135;
					break;
				case 270:
					inputdirection = -1;
					break;
				default:
					break;
				}
			}
			//�ı䷽��
			int i = tanks[0].direction - inputdirection;
			if (inputdirection != -1)
			{

				if (i != 0)//����ͬ
				{
					if (i < 0)
						i += 360;
					if (i <= 180)
					{
						tanks[0].direction--;
					}
					else
					{
						tanks[0].direction++;
					}
				}
			}
			//����
			if (i >= 0 && i <= 10)
			{
				if (tanks[0].acceleration <= tanks[0].frontacc)//�ٶ�û�����
					tanks[0].acceleration += tanks[0].accpers;//����
				if (tanks[0].acceleration > tanks[0].frontacc)//�ٶȵ��Ｋ��
					tanks[0].acceleration = tanks[0].frontacc;//����
			}
		}
		if (KeyDown(DIK_Q))
		{
			tanks[0].turretRotation -= 0.05f;//��ת����
			if (tanks[0].turretRotation > 6.28f)
				tanks[0].turretRotation -= 6.28f;
			if (tanks[0].turretRotation < 0)
				tanks[0].turretRotation += 6.28f;
		}
		if (KeyDown(DIK_E))
		{
			tanks[0].turretRotation += 0.05f;
			if (tanks[0].turretRotation > 6.28f)
				tanks[0].turretRotation -= 6.28f;
			if (tanks[0].turretRotation < 0)
				tanks[0].turretRotation += 6.28f;
		}
		if (KeyDown(DIK_SPACE))
		{
			newTank(MousePositionX(window) + ScrollX, MousePositionY(window) + ScrollY);
		}
		if (KeyDown(DIK_P) || KeyDown(DIK_ESCAPE))
		{
			gamestate = pause;//״̬ת��
			Sleep(200);//�ú�˯����Ҫ��һ�¼����������ΰ�������
		}
		if (ingameQuit.y > -ingameQuit.height)//�״̬���ƶ���ť
			MoveQuitButton();
		//�����ڿڳ���
		if (tanks[0].alive)
		{
			GetCursorPos(lpPoint);
			ScreenToClient(window, lpPoint);
			tanks[0].turretRotation = atan2(lpPoint->y + ScrollY - (tanks[0].y + tanks[0].rotateY), lpPoint->x + ScrollX - (tanks[0].x + tanks[0].rotateX)) + PI / 2;//arctan=0ʱ���ң�����tR=0Ϊ���ϣ����˦�/2

		}
		//����̹��AI����
		TanksAI(tanks);
		break;
	case pause:
		if (MouseButton(0))
			if (ClickQuitButton(window))
				gamestate = menu;
		if (KeyDown(DIK_P) || KeyDown(DIK_ESCAPE))
		{
			gamestate = ingame;
			Sleep(200);//��+1��
		}
		MoveQuitButton();
		break;
	case dead:
		UpdateShells(shells);
		UpdateTanks(tanks);
		TanksAI(tanks);
		if (GetTickCount() > createtime)
		{
			gamestate = ingame;
			newTank(100, 100, true);
		}
		break;
	case config:
		if (MouseButton(0))
		{
			int x = MousePositionX(window), y = MousePositionY(window);
			if (ClickArea(x, y, RECT{ 530,290,586,346 }))
			{
				flag_control = 0;
			}
			if (ClickArea(x, y, RECT{ 750,290,806,346 }))
			{
				flag_control = 1;
			}
			if (ClickArea(x, y, RECT{ 525,428,581,484 }))
			{
				enemyfireinterval = 1500;
			}
			if (ClickArea(x, y, RECT{ 656,428,712,484 }))
			{
				enemyfireinterval = 1000;
			}
			if (ClickArea(x, y, RECT{ 786,428,842,484 }))
			{
				enemyfireinterval = 500;
			}
			if (ClickArea(x, y, RECT{ 675,575,915,682 }))
			{
				gamestate = menu;
			}
		}
		if (KeyDown(DIK_ESCAPE))
		{
			gamestate = menu;//״̬ת��
		}
		break;
	default:
		break;
	}
	/*
	if (tex == "")
	{
	switch (gamestate)
	{
	case init:
	tex = "Init";
	break;
	case menu:
	tex = "Menu";
	break;
	case ingame:
	tex = "In Game";
	break;
	case pause:
	tex = "PAUSE";
	break;
	case dead:
	tex = "DEAD";
	break;
	case config:
	tex = "Config";
	break;
	default:
	break;
	}
	tex = "Mouse Position = "+to_string(MousePositionX(window)) + "," + to_string(MousePositionY(window))+","+to_string(flag_control) + "," + to_string(enemyfireinterval);
	wtex = to_string(flag_control) + "," + to_string(enemyfireinterval);
	}
	*/

	//���ƴ���
	if (d3ddev->BeginScene())
	{
		//���þ����豸�Ļ���
		switch (gamestate)
		{
		case init:
			d3ddev->SetFVF(TEXTURE_FVF);
			DrawQuad2(sq);//�����ı���
			break;
		case menu:
			break;
		case ingame:
		case pause:
		case dead:
			ScrollScreen(tanks[0]);//���¾���
			break;
		default:
			break;
		}
		//��Ҫ�����豸���ƵĲ���
		spriteobj->Begin(D3DXSPRITE_ALPHABLEND);
		//��ʼ����
		switch (gamestate)
		{
		case init:
			break;
		case menu:
			DrawMenu();//���Ʋ˵�
			break;
		case ingame:
			DrawShells(shells);//�����ڵ�
			DrawTanks(tanks);//����̹��
			updateExplosion(expo);//��Ȼ�����Ǹ��µ��ǰ������ƹ��ܣ����Է��ڴ˴�
			if (ingameQuit.y > -ingameQuit.height)//��ťû�ջ�ȥ������
				SpriteTransformDraw(imgreturn, ingameQuit.x, ingameQuit.y, ingameQuit.width, ingameQuit.height, 0, 1, 0.0f, 0.75f);
			FontPrint(fontGaramond50, 200, 0, to_string(tanks[0].HP), D3DCOLOR_XRGB(255, 255, 255));
			break;
		case pause:
			DrawShells(shells);//��ͣҲҪ����
			DrawTanks(tanks);
			if (ingameQuit.y > -ingameQuit.height)
				SpriteTransformDraw(imgreturn, ingameQuit.x, ingameQuit.y, ingameQuit.width, ingameQuit.height, 0, 1, 0.0f, 0.75f);
			FontPrint(fontGaramond50, 200, 0, to_string(tanks[0].HP), D3DCOLOR_XRGB(255, 255, 255));
			break;
		case dead:
		{
			DrawShells(shells);//����ҲҪ����
			DrawTanks(tanks);
			updateExplosion(expo);//���˱�ըҲ�ߣ���ͣ����
			if (ingameQuit.y > -ingameQuit.height)
				SpriteTransformDraw(imgreturn, ingameQuit.x, ingameQuit.y, ingameQuit.width, ingameQuit.height, 0, 1, 0.0f, 0.75f);
			SpriteTransformDraw(imgdead, 0, 161, 1024, 346);//�����������棨����Դ���ڰ�֮�꣩
			string deathnote = "��Ҫ" + to_string((createtime - GetTickCount()) / 1000) + "�븴��";
			FontPrint(fontTest36, 400, 507, deathnote, D3DCOLOR_XRGB(200, 0, 0));//������ʾ��Ϣ
			break;
		}
		case config:
			DrawMenu();
			DrawConfig();
			break;
		default:
			break;
		}
		//FontPrint(fontTest36, 400, 0, tex, D3DCOLOR_XRGB(114, 51, 41));
		//��������
		spriteobj->End();
		d3ddev->EndScene();
		d3ddev->Present(NULL, NULL, NULL, NULL);
	}
}
//��Ϸ����
void GameEnd()
{
	SafeRelease(fontTest36);
	SafeRelease(imgexplosion);
	vector<SPRITE>(expo).swap(expo);
	SafeRelease(imgsource);
	SafeRelease(gameworld);
	//�ͷ�Direct���
	DirectInputShutdown();
	Direct3DShutdown();
}