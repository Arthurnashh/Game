#include "Simul/Terrain/HeightMapNode.h"
#include <math.h>
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Math/Noise2D.h"
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
HeightMapNode::HeightMapNode():Node()
{
	PageSize=513;
	TileSize=33;
	PageWorldX=50000.f;
	PageWorldZ=50000.f;
	MaxHeight=2000.f;
	MaxMipMapLevel=5;
	data=new float[PageSize*PageSize];
	Rebuild();
}

HeightMapNode::~HeightMapNode()
{
	delete [] data;
}

void HeightMapNode::Rebuild()
{
	PageSize=NearestPowerOfTwo(PageSize-1)+1;
	if(PageSize<5)
		PageSize=5;
	TileSize=NearestPowerOfTwo(TileSize-1)+1;
	if(TileSize>=PageSize)
		TileSize=NearestPowerOfTwo((PageSize-1)/2)+1;

	delete [] data;
	data=new float[PageSize*PageSize];
	float *pDest=data;

	Noise2D noise;
	static int octaves=5;
	static int freq=32;
	static float scale=250000.f;
	static float persistence=.25f;
	noise.SetNoiseFrequency(freq,1,octaves,persistence);

	class RidgeFilter:public NoiseFilter
	{
	public:
		float Filter(float val) const
		{
			val=1.f-2.f*fabs(.5f-val);
			return val;
		}
	};
	RidgeFilter rf;
	noise.SetFilter(&rf);
    for (int j = 0; j < PageSize; ++j)
    {
		float x=PageWorldX/scale*(float)j/(float)(PageSize-1);
        for (int i = 0; i < PageSize; ++i)
        {
			float y=PageWorldZ/scale*(float)i/(float)(PageSize-1);
			*pDest++ = noise.PerlinNoise2D(x,y);
		}
	}

	for(ClientUser<HeightMapClientInterface>::client_list::iterator i=ClientUser<HeightMapClientInterface>::clients.begin();
		i!=ClientUser<HeightMapClientInterface>::clients.end();i++)
	{
		i->first->Rebuild(this);
	}
}

void HeightMapNode::RecalcTiling()
{
	Rebuild();
}


void HeightMapNode::ChangedWorldSize()
{
	Rebuild();
}

const float *HeightMapNode::GetData() const
{
	return data;
}

static int num_steps=12;
//VisualDebugInterface for tools:
unsigned HeightMapNode::GetNumLines() const
{
	return num_steps*2;
}

void HeightMapNode::ToGrid(int &X,int &Y,float x,float y) const
{
	X=(int)((float)PageSize*x/PageWorldX);
	Y=(int)((float)PageSize*y/PageWorldZ);
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
	float cx=( (float)PageSize*x/PageWorldX);
	float cy=( (float)PageSize*y/PageWorldZ);
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

void HeightMapNode::GetLine(unsigned l,float* x1,float* x2) const
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
	int X,Y;
	float px,py;
	ToGrid(X,Y,px,py,x,y);
	float h=MaxHeight*data[PageSize*Y+X];
	return h;
}

float HeightMapNode::GetHeightAt(int X,int Y) const
{
	float h=MaxHeight*data[PageSize*Y+X];
	return h;
}

void HeightMapNode::SetHeightAt(int X,int Y,float h)
{
	data[PageSize*Y+X]=h/MaxHeight;

	for(ClientUser<HeightMapClientInterface>::client_list::iterator i=ClientUser<HeightMapClientInterface>::clients.begin();
		i!=ClientUser<HeightMapClientInterface>::clients.end();i++)
	{
		i->first->PartialRebuild(this,X,X,Y,Y);
	}
}