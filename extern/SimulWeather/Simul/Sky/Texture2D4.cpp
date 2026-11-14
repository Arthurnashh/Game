#include "precomp.h"

#include "./Texture2D4.h"
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <string.h>
using namespace simul::sky;

Texture2D4::Texture2D4():
	texsize(0)
{
	tex=0;
	X=0;
	Y=0;
}

Texture2D4::~Texture2D4()
{
	delete [] tex;
}

const Texture2D4 &Texture2D4::operator=(const Texture2D4 &t)
{
	Setup(t.X,t.Y,t.Wrap,t.Border,t.Edge);
	memcpy(tex,t.tex,sizeof(float4)*texsize);
	return *this;
}

#define value_at(i,j) tex[j*X+i]
#define edge_value_at(i,j) ((i<0||j<0)?Edge:(i>=X||j>=Y)?Edge:tex[j*X+i])
static int round_down(float f)
{
	return (f)>0?(int)(f):(int)((f-1.f));
}

static inline void lerp(float4 &result,const float4 &l,const float4 &h,float f)
{
	result=h;
	result-=l;
	result*=f;
	result+=l;
}

float4 Texture2D4::Get(float x,float y)
{
	float4 u,v,a,b,c,d;
	int I1=round_down(x*X);
	int J1=round_down(y*Y);

	float sx=(x*X-I1);
	float sy=(y*Y-J1);
	int I2=I1+1;
	int J2=J1+1;
	if(Wrap)
	{
		I1=I1%X;
		J1=J1%Y;
		I2=I2%X;
		J2=J2%Y;
		while(I1<0)
			I1+=X;
		while(J1<0)
			J1+=Y;
		while(I2<0)
			I2+=X;
		while(J2<0)
			J2+=Y;
	}
	else
	{
		if(Border)
		{
			if(I1<0||J1<0||I2>=X||J2>=Y)
			{
				u=edge_value_at(I1,J1);		// left bottom back
				v=edge_value_at(I2,J1);		// right bottom front
				lerp(a,u,v,sx);			// bottom back x=i1

				u=edge_value_at(I1,J2);		// left bottom back
				v=edge_value_at(I2,J2);		// right bottom front
				lerp(b,u, v, sx);				// bottom front x=I2
							 
				lerp(c,a, b, sy);				// bottom, z=K1
		
				return c;
			}
		}
		else
		{
			I1=std::max(I1,0);
			I2=std::max(I2,0);
			J1=std::max(J1,0);
			J2=std::max(J2,0);
			I1=std::min(I1,X-1);
			I2=std::min(I2,X-1);
			J1=std::min(J1,Y-1);
			J2=std::min(J2,Y-1);
		}
	}

	u=value_at(I1,J1);		// left bottom back
	v=value_at(I2,J1);		// right bottom front
	lerp(a,u, v,sx);		// bottom back x=i1

	u=value_at(I1,J2);		// left bottom back
	v=value_at(I2,J2);		// right bottom front
	lerp(b,u, v, sx);		// bottom front x=I2
				 
	lerp(c,a, b, sy);		// bottom, z=K1

	return c;
}

float4 *Texture2D4::Setup(int x,int y,bool wrap,bool border,float4 edge)
{
	Edge=edge;
	Wrap=wrap;
	Border=border;
	if(x==X&&y==Y)
		return tex;
	X=x;
	Y=y;
	if(x*y<=texsize)
		return tex;
	texsize=x*y;
	delete [] tex;

	unsigned sz=sizeof(float4)*texsize;
	char *temp=new char[sz];

	tex=(float4*)temp;
	return tex;
}

simul::sky::float4 *Texture2D4::GetDataPointer()
{
	return tex;
}

void Texture2D4::Zero()
{
	memset(tex,0,sizeof(float4)*texsize);
}