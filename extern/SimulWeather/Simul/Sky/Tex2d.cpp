#include "precomp.h"

#include "./Tex2d.h"
#include <algorithm>
#include <math.h>
using namespace simul::sky;



Texture2D::Texture2D():
	texsize(0)
{
	tex=0;
	X=0;
	Y=0;
}

Texture2D::~Texture2D()
{
	delete [] tex;
}
#define value_at(i,j) tex[j*X+i]

#define edge_value_at(i,j) ((i<0||j<0)?Border?GetBorder(i,j):Edge:(i>=X||j>=Y)?Border?GetBorder(i,j):Edge:tex[j*X+i])
static int round_down(float f)
{
	return (f)>0?(int)(f):(int)((f-1.f));
}
//((f>0)?(int)(f):((int)((f)-.5f)))
static float lerp(float l,float h,float f)
{
	return l+((h-l)*f);
}

float Texture2D::GetBorder(int i,int j)
{
	if(i<0)
		i=0;
	if(j<0)
		j=0;
	if(i>=X)
		i=X-1;
	if(j>=Y)
		j=Y-1;
	return value_at(i,j);
}

float Texture2D::Get(float x,float y)
{
	float u,v,a,b,c;
	int I1=round_down(x*X);
	int J1=round_down(y*Y);
	
	int I2=I1+1;
	int J2=J1+1;
	

	float sx=(x*X-I1);
	float sy=(y*Y-J1);
	
//	bool edgeI=false,edgeJ=false;//,edgeK=false;
	if(Wrap)
	{
		I1=I1%X;
		J1=J1%Y;
		
		I2=I2%X;
		J2=J2%Y;
	}
	else
	{
		if(I1<0||J1<0||I2>=X||J2>=Y)
		{
			u=edge_value_at(I1,J1);		// left bottom back
			v=edge_value_at(I2,J1);		// right bottom front
			a = lerp(u, v,sx);			// bottom back x=i1

			u=edge_value_at(I1,J2);		// left bottom back
			v=edge_value_at(I2,J2);		// right bottom front
			b = lerp(u, v, sx);				// bottom front x=I2
						 
			c = lerp(a, b, sy);				// bottom, z=K1
	
			return c;
		}
	}

	u=value_at(I1,J1);		// left bottom back
	v=value_at(I2,J1);		// right bottom front
	a = lerp(u, v,sx);			// bottom back x=i1

	u=value_at(I1,J2);		// left bottom back
	v=value_at(I2,J2);		// right bottom front
	b = lerp(u, v, sx);			// bottom front x=I2
				 
	c = lerp(a, b, sy);			// bottom, z=K1

	return c;
}
float *Texture2D::Setup(int x,int y,bool wrap,float edge)
{
	Border=(edge==-1.f);
	if(edge<0)
		edge=0;
	Edge=edge;
	Wrap=wrap;
	X=x;
	Y=y;
	if(x*y<=texsize)
		return tex;
	texsize=x*y;
	delete [] tex;
	tex=new float[x*y];
	return tex;
}
