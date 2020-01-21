#include "DirectX.h"
#include <memory>//引入智能指针

int (WINAPIV * __vsnprintf)(char *, size_t, const char*, va_list) = _vsnprintf;//解决LNK2019错误

																			   //常量
const string APPTITLE = "Tanks War";
const int SCREENW = 1024;//屏幕宽度
const int SCREENH = 768;//屏幕高度
const int TankNum = 10;//坦克数量限制
const int ShellNum = 300;//炮弹数量限制
const int TILEWIDTH = 64;//要显示的位块宽度
const int TILEHEIGHT = 64;//位块高度

						  //debug字体
LPD3DXFONT fontTest36 = NULL;
//生成字体
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

//第二种顶点，第一种是需要光照和投影变换的删了
static const unsigned short TEXTURE_FVF = D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_DIFFUSE;// 灵活顶点格式
struct VERTEX2	//定义自己的顶点结构体
{

	float x, y, z, rhw;	// 纹理坐标位置
	unsigned long color;// 颜色
	float u, v;// 纹理坐标内的位置
			   //设置顶点坐标
	VERTEX2(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f, float _u = 0.0f, float _v = 0.0f, float _rhw = 1.0f)
	{
		x = _x, y = _y, z = _z, u = _u, v = _v, rhw = _rhw;
		color = D3DCOLOR_ARGB(255, 255, 255, 255);
	}
};

//四边形
struct QUAD2
{
	VERTEX2 vertices[4];//顶点成员数组
	LPDIRECT3DTEXTURE9 texture;//纹理指针
	LPDIRECT3DVERTEXBUFFER9 buffer;//渲染缓冲区指针
								   //深复制函数
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
	//重载赋值函数
	QUAD2 operator=(const QUAD2 &quad)
	{
		for (int i = 0; i < 4; i++)
			vertices[i] = quad.vertices[i];
		texture = quad.texture;//先不释放，万一有别的四方形用呢
		buffer->Release();//
		d3ddev->CreateVertexBuffer(
			4 * sizeof(VERTEX2),
			0,
			TEXTURE_FVF, D3DPOOL_DEFAULT,
			&buffer,
			NULL);
	}
	//有参构造函数
	QUAD2(char *textureFilename, double width = 0, double height = 0, int screenW = 0, int screenH = 0)
	{
		//装载纹理
		D3DXCreateTextureFromFile(d3ddev, textureFilename, &texture);
		//默认大小
		if (width == 0 && height == 0)
		{
			D3DXVECTOR2 inf = GetBitmapSize(textureFilename);
			width = inf.x, height = inf.y;
		}
		//创建顶点缓冲区
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
//闪烁四边形
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
		if (now - timetick > 2)//过两个时间跳
		{
			timetick = now;
			int live = now - startkick;//播放总时间
			live = live / 1024;//换算成时间段
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
				gamestate = menu;//状态转换
				break;
			}
			if (alpha > 25000)//alpha下溢到无穷大
				alpha = 0;
			if (alpha > 255)
				alpha = 255;
			for (int i = 0; i < 4; i++)
				vertices[i].color = D3DCOLOR_ARGB(alpha, 255, 255, 255);//设置顶点颜色
		}
	}
};
//坦克结构
struct TANK :SPRITE
{
	int rotateX = 39, rotateY = 32;//底盘与炮塔的连接点
	int turretX = 30, turretY = 46;//炮塔的旋转中心
	int turretWidth = 50, turretHeight = 76;//炮塔宽度高度
	float acceleration = 0.0f, turretRotation = 0.0f, fx, fy;//加速度，炮塔方向，浮点x位置，浮点y位置
	float frontacc = 3.0f, reverseacc = 1.5f, accpers = 0.3f;//前向最大速度，倒车最大速度，加速效率
	float rOffsetX = 0, rOffsetY = 0;//开炮炮弹的位置补偿
	int lastshoot = 0, HP = 0, MaxHP = 10;//最后开炮时间，血量
	bool alive;//是否存活
			   //shared_ptr<LPDIRECT3DTEXTURE9>imgchassis;
	TANK();
	void TankTransformDraw(LPDIRECT3DTEXTURE9 image, int frame, int columns, float scaling, D3DCOLOR color);
	bool FireShell(int type);
};
//炮弹结构
struct SHELL :SPRITE
{
	int shellWidth = 250, shellHeight = 232;//图像在源图上的偏移
	float acceleration = 10.0f, fx = 0.0f, fy = 0.0f;//速度，浮点显示坐标
	bool alive = false;//是否存活
	TANK *father;//发射者
	int type;//型号
	SHELL();
	SHELL(float ix, float iy, float d, TANK *f, int shelltype = 1);
};

//数据数值及向量
vector<SPRITE>expo;//爆炸对象存储向量
TANK tanks[TankNum];//坦克存储数组
SHELL shells[ShellNum];//炮弹存储数组
SPRITE ingameQuit;//游戏状态中退出图标
SPRITE opinion1, opinion2;
int ScrollX = 0, ScrollY = 0;//卷轴地图位置坐标
int SpeedX, SpeedY, GAMEWORLDWIDTH, GAMEWORLDHEIGHT;//卷轴横向、纵向滚动速度，地图宽度、高度
unsigned long createtime;//玩家座驾产生时间
unsigned long addtime;//上一个AI坦克产生时间
bool flag_control = false;
int enemyfireinterval = 1000;

//图像资源
LPDIRECT3DTEXTURE9 imgexplosion = NULL;//爆炸图片
D3DXVECTOR2 inforexplo;//爆炸图像大小信息
LPDIRECT3DTEXTURE9 imgsource = NULL;//坦克及炮弹图片
LPDIRECT3DSURFACE9 gameworld = NULL;//游戏地图
LPDIRECT3DTEXTURE9 imgmenuBG = NULL;//菜单
LPDIRECT3DTEXTURE9 imgmenuST = NULL;//开始键
LPDIRECT3DTEXTURE9 imgmenuCG = NULL;//设置键
LPDIRECT3DTEXTURE9 imgmenuQT = NULL;//退出键
LPDIRECT3DTEXTURE9 imgreturn = NULL;//返回菜单键
LPDIRECT3DTEXTURE9 imgdead = NULL;//死亡画面
LPDIRECT3DTEXTURE9 imgconfig = NULL;//设置画面
LPDIRECT3DTEXTURE9 imgopinion = NULL;//选择精灵
SHINYQUAD *sq = NULL;//开场动画

					 //绘制四边形
void DrawQuad2(QUAD2 *quad)
{
	//使用四边形顶点填充顶点缓冲区
	void *temp = NULL;
	quad->buffer->Lock(0, sizeof(quad->vertices), (void**)&temp, 0);//锁定后填充
	memcpy(temp, quad->vertices, sizeof(quad->vertices));//复制到目标区域
	quad->buffer->Unlock();//解锁

						   //绘制两个三角形组成的四边形
	d3ddev->SetTexture(0, quad->texture);//指定纹理
	d3ddev->SetStreamSource(0, quad->buffer, 0, sizeof(VERTEX2));//指定流源

																 // 设置纹理和顶点混合
	d3ddev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);//第一项混合参数来自纹理
	d3ddev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);//第二项混合参数来自顶点颜色
	d3ddev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);//调制方式：相乘

																	 // 设置Alpha混合
	d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);//启用透明混合
	d3ddev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);//设定源混合因子
	d3ddev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);//设定目标混合因子
	d3ddev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);//绘制连续三角形，数量为2
	d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);//关闭透明混合
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
	//for (auto iter : expo)不能提取iter？
	if (expo.size() > 0)
	{
		for (auto iter = expo.begin(); iter != expo.end(); iter++)
		{
			ExplosionAnimate(iter->frame, iter->startframe, iter->endframe, 1, iter->starttime, 10);//将每个爆炸动画向后一帧
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
				SpriteDrawFrame(imgexplosion, i->x - ScrollX, i->y - ScrollY, i->frame, i->width, i->height, i->columns);//绘制爆炸动画
				i++;
			}
		}

	}
}

//坦克
int newTank(int ix, int iy, bool player = false, int num = 0, int iwidth = 78, int iheight = 78)
{
	int n = 0;
	if (!player)
	{
		if (num != 0)//不是玩家又指定了序号
			n = num;
		else//不是玩家，序号为0，必是没手动指定序号
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
	tanks[n].lastshoot = (int)GetTickCount();//生成时不要立即开炮
	if (player)
	{
		createtime = GetTickCount();//记录生成时间
	}
	else
	{
		addtime = GetTickCount();//记录生成时间
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
	//绘制底盘
	D3DXVECTOR2 scale(scaling, scaling);//缩放向量
	D3DXVECTOR2 trans(this->x - ScrollX, this->y - ScrollY);//绘制位置
	D3DXVECTOR2 center((float)(this->width*scaling) / 2, (float)(this->height*scaling) / 2);//旋转中心
	D3DXMATRIX mat;
	D3DXMatrixTransformation2D(&mat, NULL, 0, &scale, &center, (float)toRadians(direction), &trans);//变换矩阵
	spriteobj->SetTransform(&mat);
	int cx = (frame % this->columns)*this->width;//源位置
	int cy = (frame / this->columns)*this->height;
	RECT srcRect = { cx, cy, cx + width, cy + height };
	spriteobj->Draw(image, &srcRect, NULL, NULL, color);
	//绘制炮塔
	rotation = toRadians(direction);//方向
	rOffsetX = sin(toRadians(direction)) * 7, rOffsetY = cos(toRadians(direction)) * 7;//炮塔旋转点相对底座中心的差值
	D3DXVECTOR2 ttrans(fx + (rotateX + rOffsetX - 30)*scaling - ScrollX, fy + (rotateY - rOffsetY + 7 - 46)*scaling - ScrollY);
	D3DXVECTOR2 tcenter((float)turretX*scaling, (float)turretY*scaling);
	D3DXMATRIX tmat;
	D3DXMatrixTransformation2D(&tmat, NULL, 0, &scale, &tcenter, turretRotation, &ttrans);
	spriteobj->SetTransform(&tmat);
	int tx = (frame % this->columns)*turretWidth;
	int ty = (frame / this->columns)*turretHeight + 156;//资源图片上炮塔偏移量
	RECT turRect = { tx, ty, tx + turretWidth, ty + turretHeight };
	spriteobj->Draw(image, &turRect, NULL, NULL, color);
	D3DXMatrixIdentity(&mat);//恢复单位矩阵
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
	//循环更新坦克成员
	for (int i = 0; i < 10; i++)
	{
		if (tanks[i].alive)
		{
			//计算位置
			tanks[i].fx += sin((double)toRadians(tanks[i].direction))*tanks[i].acceleration;
			tanks[i].fy -= cos((double)toRadians(tanks[i].direction))*tanks[i].acceleration;
			//边界检查
			if (tanks[i].fx < 0.0f)
				tanks[i].fx = 0.0f;
			if (tanks[i].fy < 0.0f)
				tanks[i].fy = 0.0f;
			if (tanks[i].fx >(float)GAMEWORLDWIDTH - tanks[i].width)
				tanks[i].fx = (float)GAMEWORLDWIDTH - tanks[i].width;
			if (tanks[i].fy >(float)GAMEWORLDHEIGHT - tanks[i].height)
				tanks[i].fy = (float)GAMEWORLDHEIGHT - tanks[i].height;
			//碰撞检测
			for (int j = i + 1; j < 10; j++)
			{
				float mx = tanks[i].fx - tanks[j].fx, my = tanks[i].fy - tanks[j].fy;
				double hypot = sqrt(mx*mx + my*my);
				if (hypot < 111)//距离小于一定值再碰撞检测
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
			//摩擦力减速,设摩擦力为0.1f
			if (tanks[i].acceleration > -0.1f&&tanks[i].acceleration < 0.1f)//如果速度低于摩擦力减速则停下
				tanks[i].acceleration = 0.0f;
			if (tanks[i].acceleration>0.1f)//正向前进给负向摩擦力
				tanks[i].acceleration -= 0.1f;
			if (tanks[i].acceleration<-0.1f)//倒车给正向摩擦力
				tanks[i].acceleration += 0.1f;
			//数值处理
			if (tanks[i].direction < 0)
				tanks[i].direction += 360;
			if (tanks[i].direction > 360)
				tanks[i].direction -= 360;
			//计算屏幕显示位置
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
			tank = TANK();//调用无惨（确信）构造函数，坦克消失
		else//玩家
		{
			gamestate = dead;
			if (GetTickCount()>createtime)
				createtime = GetTickCount() + 10000;//10秒以后活
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

//炮弹
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
				if (GetDistance(tanks[t], shells[n]) < 128)//一定距离以上不检测碰撞
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
	int fx, fy;//图像资源上图案偏移量
	if (shell.type == 2)
		fx = shell.shellWidth + shell.width;
	else
		fx = shell.shellWidth;
	fy = shell.shellHeight;
	RECT srcRect = { fx, fy, fx + shell.width, fy + shell.height };
	spriteobj->Draw(image, &srcRect, NULL, NULL, color);

	D3DXMatrixIdentity(&mat);//创建单位矩阵
	spriteobj->SetTransform(&mat);//恢复正常坐标系
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

//地图功能

//绘制图块    已知bug：地图块大小不能变，因为StretchRect不能缩放非缓冲区表面
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
//创建地图
void BulidGameWorld(MAPDATA mapdata, string originimage)
{
	HRESULT result;//结果
	int x, y;//位图块在源图上的位置
	float widthScaling, heightScaling;
	LPDIRECT3DSURFACE9 tiles;
	D3DXVECTOR2 imageinfo;//源图信息
						  //装载位图块图像
	tiles = LoadSurface(originimage);
	imageinfo = GetBitmapSize(originimage);
	widthScaling = TILEWIDTH / (int)(imageinfo.x / 10), heightScaling = TILEHEIGHT / (int)(imageinfo.y / 4);//显示和源图的缩放比例
																											//10和4是源图上横向和纵向图片的数量
	GAMEWORLDWIDTH = TILEWIDTH * mapdata.width, GAMEWORLDHEIGHT = TILEHEIGHT * mapdata.height;
	//创建滚动地图
	result = d3ddev->CreateOffscreenPlainSurface(
		GAMEWORLDWIDTH,//表面宽度（块宽*块数）
		GAMEWORLDHEIGHT,//表面高度
		D3DFMT_X8R8G8B8,
		D3DPOOL_DEFAULT,
		&gameworld,  //指向表面的指针
		NULL);
	if (result != D3D_OK)
	{
		MessageBox(NULL, "Error creating working surface!", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	//用位图块填充地图图像
	for (y = 0; y < mapdata.height; y++)
		for (x = 0; x < mapdata.width; x++)
			DrawTile(tiles, mapdata.map[y * mapdata.width + x], (int)(imageinfo.x / 10), (int)(imageinfo.y / 4), 10, gameworld, x * TILEWIDTH, y * TILEHEIGHT, widthScaling, heightScaling);
	//释放源图
	tiles->Release();
}
//滚动卷轴
void ScrollScreen(SPRITE player)
{
	//更新横向卷轴位置和速度
	ScrollX = player.x + player.width*0.5*player.scaling - SCREENW*0.5;
	if (ScrollX < 0)
	{
		ScrollX = 0;
	}
	else if (ScrollX > GAMEWORLDWIDTH - SCREENW)
		ScrollX = GAMEWORLDWIDTH - SCREENW;
	//更新纵向卷轴位置和速度
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
//绘制主菜单
void DrawMenu()
{
	LPDIRECT3DSURFACE9 sur = NULL;
	imgmenuBG->GetSurfaceLevel(0, &sur);
	d3ddev->StretchRect(sur, NULL, backbuffer, NULL, D3DTEXF_LINEAR);
	SpriteTransformDraw(imgmenuST, 28, 360, 367, 88);
	SpriteTransformDraw(imgmenuCG, 28, 484, 367, 88);
	SpriteTransformDraw(imgmenuQT, 28, 612, 367, 88);
}
//坦克AI函数
bool TanksAI(TANK tanks[])
{
	for (int i = 1; i < 10; i++)//tanks[0]是玩家座驾不能控制
	{
		if (tanks[i].alive)//对活着的坦克
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
		else//死了的空位置
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
//检查菜单按键情况
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
//获取退出键
SPRITE getQuitButton(string originimage)
{
	SPRITE sprite = GetSpriteSize(originimage, &sprite, 1, 1);
	sprite.scaling = 0.75;
	sprite.x = 800;//请与ClickQuitButton函数中的数值联动
	sprite.y = -sprite.height;//(int)(-sprite.height*sprite.scaling);
							  //实际显示大小为96*96
	return sprite;
}

//移动退出键
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
//点击返回菜单按键
bool ClickQuitButton(HWND hwnd)
{
	int x = MousePositionX(hwnd), y = MousePositionY(hwnd);
	if (ClickArea(x, y, RECT{ 800,0,896,96 }))
		return true;
	return false;
}
//游戏初始化
bool GameInit(HWND window)
{
	//加载Direct3D组件
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
	//debug字体
	fontTest36 = MakeFont("Arial", 36);
	fontGaramond50 = MakeFont("Garamond", 50);
	//加载坦克图像
	imgsource = LoadTexture("tanksource.png", D3DCOLOR_XRGB(255, 0, 255));
	//imgsource = LoadTexture("tanktest.png",D3DCOLOR_XRGB(255,0,255));
	if (!imgsource)
	{
		MessageBox(window, "Error load tank image", "Error!", MB_OK | MB_ICONERROR);
		return false;
	}
	//加载爆炸图像
	imgexplosion = LoadTexture("explode.png", D3DCOLOR_XRGB(255, 0, 255));
	inforexplo = GetBitmapSize("explode.png");
	if (!imgexplosion)
	{
		MessageBox(window, "Error load image", "Error!", MB_OK | MB_ICONERROR);
		return false;
	}
	//加载菜单图像
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

	//设定Direct3D流为灵活顶点格式
	d3ddev->SetFVF(TEXTURE_FVF);
	sq = new SHINYQUAD("directx-logo.jpg", 672, 371, SCREENW, SCREENH);//装载开头闪现图像
	expo.reserve(10);//爆炸向量设定大小
	d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);//设定缓冲表面
	BulidGameWorld(GetMap("file.txt"), "maptilesuit.png");//创建地图
	srand((int)time(0));//设定随机种子
	return true;
}
//游戏主循环
void GameRun(HWND window)
{
	if (!d3ddev)//没创建d3d设备返回
		return;
	DirectInputUpdate();
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET || D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	//逻辑处理
	string tex = "";//准备接收debug信息
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
			case 0://什么都没点到
				break;
			case 1://开始键
				gamestate = ingame;//转换状态
				newTank(100, 100, true);//生成玩家坦克
				ingameQuit.y = -ingameQuit.height;//收好退出按钮
				for (int i = 1; i < 10; i++)
				{
					tanks[i] = TANK();//初始化坦克序列
				}
				break;
			case 2://设置键
				gamestate = config;//状态转换
				break;
			case 3://退出键
				gameover = true;
				break;
			default:
				break;
			}
		}
		break;
	case ingame:
		//更新炮弹、坦克
		UpdateShells(shells);
		UpdateTanks(tanks);
		//输入响应
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
				if (tanks[0].acceleration <= tanks[0].frontacc)//速度没到最大
					tanks[0].acceleration += tanks[0].accpers;//加速
				if (tanks[0].acceleration > tanks[0].frontacc)//速度到达极限
					tanks[0].acceleration = tanks[0].frontacc;//保持
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
			//改变方向
			int i = tanks[0].direction - inputdirection;
			if (inputdirection != -1)
			{

				if (i != 0)//方向不同
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
			//加速
			if (i >= 0 && i <= 10)
			{
				if (tanks[0].acceleration <= tanks[0].frontacc)//速度没到最大
					tanks[0].acceleration += tanks[0].accpers;//加速
				if (tanks[0].acceleration > tanks[0].frontacc)//速度到达极限
					tanks[0].acceleration = tanks[0].frontacc;//保持
			}
		}
		if (KeyDown(DIK_Q))
		{
			tanks[0].turretRotation -= 0.05f;//旋转炮塔
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
			gamestate = pause;//状态转换
			Sleep(200);//好好睡，不要按一下键接收无数次按键动作
		}
		if (ingameQuit.y > -ingameQuit.height)//活动状态则移动按钮
			MoveQuitButton();
		//计算炮口朝向
		if (tanks[0].alive)
		{
			GetCursorPos(lpPoint);
			ScreenToClient(window, lpPoint);
			tanks[0].turretRotation = atan2(lpPoint->y + ScrollY - (tanks[0].y + tanks[0].rotateY), lpPoint->x + ScrollX - (tanks[0].x + tanks[0].rotateX)) + PI / 2;//arctan=0时向右，但是tR=0为向上，差了Π/2

		}
		//调用坦克AI函数
		TanksAI(tanks);
		break;
	case pause:
		if (MouseButton(0))
			if (ClickQuitButton(window))
				gamestate = menu;
		if (KeyDown(DIK_P) || KeyDown(DIK_ESCAPE))
		{
			gamestate = ingame;
			Sleep(200);//（+1）
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
			gamestate = menu;//状态转换
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

	//绘制处理
	if (d3ddev->BeginScene())
	{
		//不用精灵设备的绘制
		switch (gamestate)
		{
		case init:
			d3ddev->SetFVF(TEXTURE_FVF);
			DrawQuad2(sq);//绘制四边形
			break;
		case menu:
			break;
		case ingame:
		case pause:
		case dead:
			ScrollScreen(tanks[0]);//更新卷轴
			break;
		default:
			break;
		}
		//需要精灵设备绘制的部分
		spriteobj->Begin(D3DXSPRITE_ALPHABLEND);
		//开始绘制
		switch (gamestate)
		{
		case init:
			break;
		case menu:
			DrawMenu();//绘制菜单
			break;
		case ingame:
			DrawShells(shells);//绘制炮弹
			DrawTanks(tanks);//绘制坦克
			updateExplosion(expo);//虽然名字是更新但是包含绘制功能，所以放在此处
			if (ingameQuit.y > -ingameQuit.height)//按钮没收回去接着收
				SpriteTransformDraw(imgreturn, ingameQuit.x, ingameQuit.y, ingameQuit.width, ingameQuit.height, 0, 1, 0.0f, 0.75f);
			FontPrint(fontGaramond50, 200, 0, to_string(tanks[0].HP), D3DCOLOR_XRGB(255, 255, 255));
			break;
		case pause:
			DrawShells(shells);//暂停也要绘制
			DrawTanks(tanks);
			if (ingameQuit.y > -ingameQuit.height)
				SpriteTransformDraw(imgreturn, ingameQuit.x, ingameQuit.y, ingameQuit.width, ingameQuit.height, 0, 1, 0.0f, 0.75f);
			FontPrint(fontGaramond50, 200, 0, to_string(tanks[0].HP), D3DCOLOR_XRGB(255, 255, 255));
			break;
		case dead:
		{
			DrawShells(shells);//死了也要绘制
			DrawTanks(tanks);
			updateExplosion(expo);//死了爆炸也走，暂停不走
			if (ingameQuit.y > -ingameQuit.height)
				SpriteTransformDraw(imgreturn, ingameQuit.x, ingameQuit.y, ingameQuit.width, ingameQuit.height, 0, 1, 0.0f, 0.75f);
			SpriteTransformDraw(imgdead, 0, 161, 1024, 346);//绘制死亡画面（梗来源：黑暗之魂）
			string deathnote = "还要" + to_string((createtime - GetTickCount()) / 1000) + "秒复活";
			FontPrint(fontTest36, 400, 507, deathnote, D3DCOLOR_XRGB(200, 0, 0));//绘制提示信息
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
		//结束绘制
		spriteobj->End();
		d3ddev->EndScene();
		d3ddev->Present(NULL, NULL, NULL, NULL);
	}
}
//游戏结束
void GameEnd()
{
	SafeRelease(fontTest36);
	SafeRelease(imgexplosion);
	vector<SPRITE>(expo).swap(expo);
	SafeRelease(imgsource);
	SafeRelease(gameworld);
	//释放Direct组件
	DirectInputShutdown();
	Direct3DShutdown();
}