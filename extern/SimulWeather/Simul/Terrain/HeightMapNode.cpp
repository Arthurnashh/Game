#include "Simul/Terrain/HeightMapNode.h"
#include <math.h>
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Math/Noise2D.h"
#include "Simul/Math/Float2.h"
#include "Simul/Math/Pi.h"
using namespace simul::terrain;
using namespace simul::graph::meta;
using namespace simul::math;

static int NearestPowerOfTwo(int unum)
{
	float num=(float)unum;
	if(fabs(num)<1e-8f)
		num=1.f;
	float Exp=log(num);
	Exp/=log(2.f);
	return (int)1<<(int)Exp;
}
static float lerp(float x,float a,float b)
{
	return b*x+a*(1.f-x);
}
HeightMapNode::HeightMapNode()
	:Group()
	,need_rebuild(true)
{
	FractalOctaves=7;
	FractalFrequency=256;
	Persistence=.45f;
	FractalScale=250000.f;
	BaseAltitude=0.f;
	FlattenBelow=0.f;
	PageSize=513;
	TileSize=65;
	PageWorldX=60000.f;
	PageWorldZ=60000.f;
	MaxHeight=8000.f;
	NumMipMapLevels=1;
	data=new float[PageSize*PageSize];
}

HeightMapNode::~HeightMapNode()
{
	delete [] data;
}

void HeightMapNode::Rebuild() const
{

	delete [] data;
	data=new float[PageSize*PageSize];
	float *pDest=data;

	Noise2D noise;
	noise.Setup(FractalFrequency,7,FractalOctaves,Persistence);

	class RidgeFilter:public NoiseFilter
	{
	public:
		float Filter(float val) const
		{
			val=1.f-2.f*fabs(.5f-val);
			val-=0.5f;
			if(val<0)
				val*=0.1f;//.01f*sin(val*pi);
			val=(val+0.1f)/0.6f;
			return val;
		}
	};
	RidgeFilter rf;
//	noise.SetFilter(&rf);

	float minf=1e23f;
	float maxf=-1e23f;
    for (int j = 0; j < PageSize; ++j)
    {
		float x=PageWorldX/FractalScale*(float)j/(float)(PageSize-1);
        for (int i = 0; i < PageSize; ++i)
        {
			float y=PageWorldZ/FractalScale*(float)i/(float)(PageSize-1);
			float z=noise.PerlinNoise2D(x,y);
			minf=std::min(z,minf);
			maxf=std::max(z,maxf);
			*pDest++ = z;
		}
	}
/*
	for(ClientUser<HeightMapClientInterface>::client_list::iterator i=ClientUser<HeightMapClientInterface>::clients.begin();
		i!=ClientUser<HeightMapClientInterface>::clients.end();i++)
	{
		i->first->Rebuild(this);
	}*/
	need_rebuild=false;
}


void HeightMapNode::NeedRebuild()
{
	need_rebuild=true;
}

void HeightMapNode::RecalcTiling()
{
	PageSize=NearestPowerOfTwo(PageSize-1)+1;
	if(PageSize<5)
		PageSize=5;
	TileSize=NearestPowerOfTwo(TileSize-1)+1;
	if(TileSize>=PageSize)
		TileSize=NearestPowerOfTwo((PageSize-1)/2)+1;
	NeedRebuild();
}


void HeightMapNode::ChangedWorldSize()
{
	NeedRebuild();
}

const float *HeightMapNode::GetData() const
{
	if(need_rebuild)
		Rebuild();
	return data;
}

static int num_steps=0;
//VisualDebugInterface for tools:
unsigned HeightMapNode::GetNumLines() const
{
	return num_steps*2;
}

void HeightMapNode::ToGrid(int &X,int &Y,float x,float y) const
{
	X=(int)((float)PageSize*(x/PageWorldX+0.5f));
	Y=(int)((float)PageSize*(y/PageWorldZ+0.5f));
	if(X<0)
		X=0;
	if(X>PageSize-1)
		X=PageSize-1;
	if(Y<0)
		Y=0;
	if(Y>PageSize-1)
		Y=PageSize-1;
}

void HeightMapNode::ToGrid(int &X,int &Y,float &px,float &py,float x,float y) const
{
	float cx=( (float)PageSize*(x/PageWorldX+0.5f));
	float cy=( (float)PageSize*(y/PageWorldZ+0.5f));
	X=(int)cx;
	Y=(int)cy;
	if(X<0)
		X=0;
	if(X>PageSize-1)
		X=PageSize-1;
	if(Y<0)
		Y=0;
	if(Y>PageSize-1)
		Y=PageSize-1;
	px=cx-X;
	py=cy-Y;
}

void HeightMapNode::GetLine(unsigned l,float* x1,float* x2,int *) const
{
	float rad=500.f;
	if((int)l>=num_steps)
	{
		l-=num_steps;
		rad=100.f;
	}
	float angle1=2.f*3.141f*(l/(float)num_steps);
	float angle2=2.f*3.141f*((l+1.f)/(float)num_steps);
	
	x1[0]=rad*cos(angle1);
	x1[1]=rad*sin(angle1);

	x2[0]=rad*cos(angle2);
	x2[1]=rad*sin(angle2);

	int X,Y;
	ToGrid(X,Y,x1[0],x1[1]);
	x1[2]=MaxHeight*data[PageSize*Y+X]+100;

	ToGrid(X,Y,x2[0],x2[1]);
	x2[2]=MaxHeight*data[PageSize*Y+X]+100;
}

void HeightMapNode::Smooth(float strength)
{
	float retain=1.f/(1.f+strength);
	float add=0.25f*(1.f-retain);
	float *new_data=new float[PageSize*PageSize];
    for (int j = 0; j < PageSize; ++j)
    {
        for (int i = 0; i < PageSize; ++i)
        {
			if(j==0||i==0||j==PageSize-1||i==PageSize-1)
			{
				new_data[PageSize*i+j]= data[PageSize*i+j];
			}
			else
			{
				float east=data[PageSize*i+j-1];
				float west=data[PageSize*i+j+1];
				float north=data[PageSize*(i+1)+j];
				float south=data[PageSize*(i-1)+j];
				new_data[PageSize*i+j]= data[PageSize*i+j]*retain+add*(east+west+north+south);
			}
		}
	}
	delete [] data;
	data=new_data;
}

bool HeightMapNode::IntersectLineSegment(const float start[],const float end[],float intersection[])
{
	Vector3 Start(start);
	Vector3 End(end);
	Vector3 Diff=End-Start;
	if(Diff.x==0&&Diff.y==0)
	{
		int X,Y;
		ToGrid(X,Y,Start.x,Start.y);
		intersection[0]=Start.x;
		intersection[1]=Start.y;
		intersection[2]=MaxHeight*data[PageSize*Y+X];
		return true;
	}
	Vector3 Pos=Start;
	Vector3 Intercept;
	if(Start.z>MaxHeight)
	{
		// start from the top of the volume
		LineInterceptPosition(Intercept,Vector3(0,0,MaxHeight),Vector3(0,0,1.f),Start,Diff);
		Pos=Intercept;
	}
	Vector3 Step=Diff;
	Step.Normalize();
	Step*=PageWorldX/(float)PageSize;
	for(int i=0;i<2*PageSize;i++)
	{
		float h=GetHeightAt(Pos.x,Pos.y);
		if(h>Pos.z)
			break;
		if(Pos.x<0||Pos.y<0)
			break;
		if(Pos.x>PageWorldX||Pos.y>PageWorldZ)
			break;
		Intercept=Pos;
		Pos+=Step;
	}
	intersection[0]=Intercept.x;
	intersection[1]=Intercept.y;
	intersection[2]=Intercept.z;
	return true;
}

float HeightMapNode::GetHeightAt(float x,float y) const
{
	if(need_rebuild)
		Rebuild();
	int X,Y;
	float px,py;
	ToGrid(X,Y,px,py,x,y);
	int X2=X+1;
	int Y2=Y+1;
	if(X2>=PageSize)
		X2-=PageSize;
	if(Y2>=PageSize)
		Y2-=PageSize;
	float H11=data[PageSize*Y+X];
	float H12=data[PageSize*Y+X2];
	float H21=data[PageSize*Y2+X];
	float H22=data[PageSize*Y2+X2];

	float h1=lerp(px,H11,H12);
	float h2=lerp(px,H21,H22);

	float h=lerp(py,h1,h2)*MaxHeight;

	float z=h+BaseAltitude;
	if(z<FlattenBelow)
		z=FlattenBelow;
	return z;
}

float HeightMapNode::GetHeightAt(int X,int Y) const
{
	if(need_rebuild)
		Rebuild();
	float h=MaxHeight*data[PageSize*Y+X];
	float z=h+BaseAltitude;
	if(z<FlattenBelow)
		z=FlattenBelow;
	return z;
}

const float *HeightMapNode::GetNormalAt(float x,float y) const
{
	if(need_rebuild)
		Rebuild();
	x/=PageWorldX;
	y/=PageWorldX;
	x+=0.5f;
	y+=0.5f;
	x*=(float)(PageSize-1);
	y*=(float)(PageSize-1);
	return GetNormalAt((int)x,(int)y);
}

const float *HeightMapNode::GetNormalAt(int x,int y) const
{
	if(need_rebuild)
		Rebuild();
	Vector3 here, left, down;
	float X=(float)x;
	float Y=(float)y;
	if(X<.5f)
		X+=.5f;
	if(Y<.5f)
		Y+=.5f;
	if(X>PageSize-.5f)
		X-=.5f;
	if(Y>PageSize-.5f)
		Y-=.5f;
	here.x = X + .5f;
	left.x = X - .5f;
	down.x = X + .5f;
	
	here.y = Y + .5f;
	left.y = Y + .5f;
	down.y = Y - .5f;
	
	here/=(float)PageSize;
	left/=(float)PageSize;
	down/=(float)PageSize;
	
	here-=(float)0.5f;
	left-=(float)0.5f;
	down-=(float)0.5f;
	
	here*=PageWorldX;
	left*=PageWorldX;
	down*=PageWorldX;
	here.z = GetHeightAt( here.x, here.y );
	left.z = GetHeightAt( left.x, left.y );
	down.z = GetHeightAt( down.x, down.y );

	left -=here;

	down -=here;

	left.Unit();
	down.Unit();

	static Vector3 result;
	CrossProduct(result,left,down);
	result.Unit();
	return result.FloatPointer(0);
}

void HeightMapNode::SetHeightAt(int X,int Y,float h)
{
	data[PageSize*Y+X]=(h-BaseAltitude)/MaxHeight;

	for(ClientUser<HeightMapClientInterface>::client_list::iterator i=ClientUser<HeightMapClientInterface>::clients.begin();
		i!=ClientUser<HeightMapClientInterface>::clients.end();i++)
	{
		i->first->PartialRebuild(this,X,X,Y,Y);
	}
}

void HeightMapNode::GetBlockCorners(int i,int j,float *x1,float *x2) const
{
	i*=TileSize-1;
	j*=TileSize-1;
	int i1=i+TileSize;
	int j1=j+TileSize;

	x1[0]=(i /(float)(PageSize-1)-0.5f)*PageWorldX;
	x1[1]=(j /(float)(PageSize-1)-0.5f)*PageWorldZ;
	x2[0]=(i1/(float)(PageSize-1)-0.5f)*PageWorldX;
	x2[1]=(j1/(float)(PageSize-1)-0.5f)*PageWorldZ;
}

void HeightMapNode::GetVertexPos(int i,int j,float *x) const
{
	x[0]=(i /(float)(PageSize-1)-0.5f)*PageWorldX;
	x[1]=(j /(float)(PageSize-1)-0.5f)*PageWorldZ;
}