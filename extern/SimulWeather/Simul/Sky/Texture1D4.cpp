#include "precomp.h"

#include "./Texture1D4.h"
#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <cstdlib>
// Include string.h for memcpy:
#include <string.h>
using namespace simul::sky;

Texture1D4::Texture1D4()
{
	tex=0;
	size=0;
}
Texture1D4::~Texture1D4()
{
	delete [] tex;
}

const Texture1D4 &Texture1D4::operator=(const Texture1D4 &t)
{
	Setup(t.size,t.Wrap,t.Border,t.Edge);
	memcpy(tex,t.tex,sizeof(float4)*size);
	return *this;
}

#define value_at(i) tex[i]
#define edge_value_at(i) ((i<0)?Edge:(i>=size)?Edge:tex[i])
static int round_down(float f)
{
	return (f)>0?(int)(f):(int)((f-1.f));
}

/*static float4 lerp(float f,float4 l,float4 h)
{
	return l+((h-l)*f);
}*/

float4 Texture1D4::Get(float x)
{
	float4 u,v,a,b,c,d;
	int I1=round_down(x*size);

	float sx=(x*size-I1);
	int I2=I1+1;
	if(Wrap)
	{
		I1=I1%size;
		I2=I2%size;
		while(I1<0)
			I1+=size;
		while(I2<0)
			I2+=size;
	}
	else
	{
		if(Border)
		{
			if(I1<0||I2>=size)
			{
				u=edge_value_at(I1);		// left bottom back
				v=edge_value_at(I2);		// right bottom front
				a = lerp(sx,u, v);			// bottom back x=i1
		
				return a;
			}
		}
		else
		{
			I1=std::max(I1,0);
			I2=std::max(I2,0);
			I1=std::min(I1,size-1);
			I2=std::min(I2,size-1);
		}
	}

	u=value_at(I1);			// left bottom back
	v=value_at(I2);			// right bottom front
	a = lerp(sx,u, v);		// bottom back x=i1

	return a;
}

float4 *Texture1D4::Setup(int x,bool wrap,bool border,float4 edge)
{
	Edge=edge;
	Wrap=wrap;
	Border=border;
	if(x==size)
		return tex;
	size=x;
	delete [] tex;

	unsigned sz=sizeof(float4)*size;
	char *temp=new char[sz];

	tex=(float4*)temp;
	return tex;
}

simul::sky::float4 *Texture1D4::GetDataPointer()
{
	return tex;
}

void Texture1D4::Zero()
{
	memset(tex,0,sizeof(float4)*size);
}