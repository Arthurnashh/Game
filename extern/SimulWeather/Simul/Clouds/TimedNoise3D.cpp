#include "precomp.h"

#include "Simul/Clouds/TimedNoise3D.h"
#include <algorithm>
#include <math.h>
#include "Simul/Math/Pi.h"
using namespace simul;
using namespace math;
using namespace clouds;


/*static float cos_interp(float a, float b,float x)
{
	float ft = x * 3.1415927f;
	float f = (1.f - cos(ft)) * .5f;
	return  a*(1.f-f) + b*f;
}*/

static float lerp(float a, float b,float x)
{
#if 0
	if(x<0.5f)
		return a;
	else
		return b;
#elif 0 //bubble_interp
	float r;
	if(b>a)
	{
		x=1.f-x;
		float i=sqrt(1.f-x*x);
		r=a*(1.f-i) + b*i;
	}
	else
	{
		float i=sqrt(1.f-x*x);
		r=b*(1.f-i) + a*i;
	}
	return r;
#elif 0
	float ft = x * 3.1415927f;
	float f = (1.f - cos(ft)) * .5f;
	return  a*(1.f-f) + b*f;
#else
	return  a*(1.f-x) + b*x;
#endif
}


/*
static float bubble_interp(float a, float b,float x)
{
	float r;
	if(b>a)
	{
		x=1.f-x;
		float i=sqrt(1.f-x*x);
		r=a*(1.f-i) + b*i;
	}
	else
	{
		float i=sqrt(1.f-x*x);
		r=b*(1.f-i) + a*i;
	}
	return r;
}*/

TimedNoise3D::TimedNoise3D(simul::graph::meta::MemoryInterface *mem)
	:Noise3D(mem)
	,initial_grid_height(0)
	,time(0.f)
{
	z_max=1e6f;
}

TimedNoise3D::~TimedNoise3D()
{
}

void TimedNoise3D::SetInitialGridHeight(float h)
{
	initial_grid_height=h;
}

float TimedNoise3D::cvalue_2d_at(int i,int j,float angle,int freq) const
{
	i=i%freq;
	j=j%freq;
	float lookup=noise_buffer[j*frequency+i];
	float val=.5f+0.5f*sin(2.f*pi*(lookup+angle));
	return val;
}
float TimedNoise3D::cvalue_at(int i,int j,int k,float angle,int kmax) const
{
	if(k<0||k>=kmax)
		return 0.f;
	i=i%frequency;
	j=j%frequency;
	k=k%frequency;
	float lookup=noise_buffer[(k*frequency+j)*frequency+i];
	float val=0.5f+0.5f*sin(2.f*pi*(lookup+angle));
	return val;
}

static float midpoint=0.5f;
float TimedNoise3D::cnoise2(float pos[2],float angle,int freq) const
{
	float x=pos[0]-midpoint;
	float y=pos[1]-midpoint;
	int i1=((int)x);
	int j1=((int)y);
	if(x<0)
		i1--;
	if(y<0)
		j1--;
	float sx=x-i1;
	float sy=y-j1;
	int i2=(i1+1);
	int j2=(j1+1);

	float u,v,a,b;

	u=cvalue_2d_at(i1,j1,angle,freq);		// left  back
	v=cvalue_2d_at(i2,j1,angle,freq);		// right  front
	a = lerp(u,v,sx);	//  back x=i1

	u=cvalue_2d_at(i1,j2,angle,freq);		// left  back
	v=cvalue_2d_at(i2,j2,angle,freq);		// right  front
	b = lerp(u,v,sx);	//  front x=i2

	return lerp(a,b,sy);	
}
float TimedNoise3D::cnoise3(float pos[3],float angle,int kmax) const
{
	float x=pos[0]-midpoint;
	float y=pos[1]-midpoint;
	float z=pos[2]-midpoint;
	int i1=((int)x);
	int j1=((int)y);
	int k1=((int)z);
	if(x<0)
		i1--;
	if(y<0)
		j1--;
	if(z<0)
		k1--;
	float sx=x-i1;
	float sy=y-j1;
	float sz=z-k1;
	int i2=(i1+1);
	int j2=(j1+1);
	int k2=(k1+1);

	float u,v,a,b,c,d;

	u=cvalue_at(i1,j1,k1,angle,kmax);		// left bottom back
	v=cvalue_at(i2,j1,k1,angle,kmax);		// right bottom front
	a = lerp(u,v,sx);	// bottom back x=i1

	u=cvalue_at(i1,j2,k1,angle,kmax);		// left bottom back
	v=cvalue_at(i2,j2,k1,angle,kmax);		// right bottom front
	b = lerp(u,v,sx);	// bottom front x=i2

	c = lerp(a,b,sy);	// bottom, z=k1

	u=cvalue_at(i1,j1,k2,angle,kmax);		// left top back
	v=cvalue_at(i2,j1,k2,angle,kmax);		// right top front
	a = lerp(u,v,sx);	// top back x=i1

	u=cvalue_at(i1,j2,k2,angle,kmax);		// left top back
	v=cvalue_at(i2,j2,k2,angle,kmax);		// right top front
	b = lerp(u,v,sx);	// top front y=j
	d = lerp(a,b,sy);	// top, z=k2

	return lerp(c,d,sz);
}
// Variant on Noise3D's Perlin function, but with limits to the value in the z direction.
float TimedNoise3D::PerlinNoise3D(float x,float y,float z) const
{
	int i;
	float result=0;
	float pos[3],effect=0.5f;
	static float ff=0.5f;
	static float midpoint1=0.f;
	static float midpoint2=0.f;
	static float n2=1.f;
	pos[0]=(1.f+x)*frequency;
	pos[1]=(1.f+y)*frequency*grid_y/(float)grid_x;
	pos[2]=(z)*frequency*grid_z/(float)grid_x;
	static float ss=1.f;
	static float uu=0.f;
	float pers_factor=ss+uu*pos[2];
	float pers=persistence*pers_factor;
	// this_grid_height is the height of the grid at this scale. we use
	// kmax to prevent the top row from having any non-zero value.
	float this_grid_height=initial_grid_height;
	float angle=time;
	static int octave_3d=1;
	
	for (i=0;i<numOctaves;i++)
	{
		int kmax=std::max((unsigned)1,(unsigned)(this_grid_height+ff));
		float pos2[]={pos[0]/2.f,pos[1]/2.f,pos[2]/2.f};
		float val=0;
		if(i<octave_3d)
			val=n2*cnoise2(pos2,angle,frequency/2);
		else
			val=cnoise3(pos,angle,kmax);
		if(filter)
			val=filter->Filter(val);
		result+=val*effect;
		effect*=pers;
		pos[0]*=2.f;			// double the position values
		pos[1]*=2.f;			// this is like making the noise grid 1/2 the size.
		pos[2]*=2.f;
		this_grid_height*=2;
		angle*=2.f;
	}
	result/=sum;
	return(result);
}

void TimedNoise3D::SetTime(float t)
{
	time=t;
}
