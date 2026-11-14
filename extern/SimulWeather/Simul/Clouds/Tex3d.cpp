#include "precomp.h"

#include "Simul/Clouds/Tex3d.h"
#include <algorithm>
#include <math.h>




Texture3D::Texture3D():
	texsize(0)
{
	tex=0;
	X=0;
	Y=0;
	Z=0;
}

Texture3D::~Texture3D()
{
	delete [] tex;
}

#define value_at(i,j,k) tex[k*X*Y+j*X+i]

#define edge_value_at(i,j,k) ((i<0||j<0||k<0)?Border?GetBorder(i,j,k):Edge:(i>=X||j>=Y||k>=Z)?Border?GetBorder(i,j,k):Edge:tex[k*X*Y+j*X+i])

static int round_down(float f)
{
	return (f)>0?(int)(f):(int)((f-1.f));
}

static float lerp(float l,float h,float f)
{
	return l+((h-l)*f);
}

static inline int wrap(int value,int step)
{
	if(value<0)
	{
		value*=-1;
		int remainder=value%step;
		return (step-remainder)%step;
	}
	else
	{
		return value%step;
	}
}

float Texture3D::GetBorder(int i,int j,int k)
{
	if(i<0)
		i=0;
	if(j<0)
		j=0;
	if(k<0)
		k=0;
	if(i>=X)
		i=X-1;
	if(j>=Y)
		j=Y-1;
	if(k>=Z)
		k=Z-1;
	return value_at(i,j,k);
}
float Texture3D::Get(float x,float y,float z)
{
	float u,v,a,b,c,d;
	int I1=round_down(x*X);
	int J1=round_down(y*Y);
	int K1=round_down(z*Z);
	int I2=I1+1;
	int J2=J1+1;
	int K2=K1+1;

	float sx=(x*X-I1);
	float sy=(y*Y-J1);
	float sz=(z*Z-K1);

	if(WrapX)
	{
		I1=wrap(I1,X);
		I2=wrap(I2,X);
	}
	else
	{
		if(I1<0)
			I1=0;
		if(I2<0)
			I2=0;
		if(I1>=X)
			I1=X-1;
		if(I2>=X)
			I2=X-1;
	}
	if(WrapY)
	{
		J1=wrap(J1,Y);
		J2=wrap(J2,Y);
	}
	else
	{
		if(J1<0)
			J1=0;
		if(J2<0)
			J2=0;
		if(J1>=Y)
			J1=Y-1;
		if(J2>=Y)
			J2=Y-1;
	}
	if(WrapZ)
	{
		K1=wrap(K1,Z);
		K2=wrap(K2,Z);
	}
	else
	{
		if(K1<0)
			K1=0;
		if(K2<0)
			K2=0;
		if(K1>=Z)
			K1=Z-1;
		if(K2>=Z)
			K2=Z-1;
	}

	u=value_at(I1,J1,K1);		// left bottom back
	v=value_at(I2,J1,K1);		// right bottom front
	a = lerp(u, v,sx);			// bottom back x=i1

	u=value_at(I1,J2,K1);		// left bottom back
	v=value_at(I2,J2,K1);		// right bottom front
	b = lerp(u, v, sx);			// bottom front x=I2
				 
	c = lerp(a, b, sy);			// bottom, z=K1

	u=value_at(I1,J1,K2);		// left top back
	v=value_at(I2,J1,K2);		// right top front
	a = lerp(u, v, sx);			// top back x=I1

	u=value_at(I1,J2,K2);		// left top back
	v=value_at(I2,J2,K2);		// right top front
	b = lerp(u, v, sx);			// top front y=j
				 
	d = lerp(a, b, sy);			// top, z=K2

	return lerp(c,d,sz);
}

float *Texture3D::Setup(int x,int y,int z,bool wrapx,bool wrapy,bool wrapz,float edge)
{
	Border=(edge==-1.f);
	if(edge<0)
		edge=0;
	Edge=edge;
	WrapX=wrapx;
	WrapY=wrapy;
	WrapZ=wrapz;
	X=x;
	Y=y;
	Z=z;
	if(x*y*z<=texsize)
		return tex;
	delete [] tex;
	tex=new float[x*y*z];
	texsize=X*Y*Z;
	return tex;
}
