#include "precomp.h"

#include "Simul/Sky/Float4.h"
#include <math.h>
#include <algorithm>
using namespace simul::sky;

namespace simul
{
	namespace sky
	{
		float4 Pow(const float4 &c,float p)
		{
			float4 r;
			r.x=pow((float)c.x,(float)p);
			r.y=pow((float)c.y,(float)p);
			r.z=pow((float)c.z,(float)p);
			r.w=pow((float)c.w,(float)p);
			return r;
		}
		float4 float4::Min(const float4 &c1,const float4 &c2)
		{
			float4 r;
			r.x=std::min(c1.x,c2.x);
			r.y=std::min(c1.y,c2.y);
			r.z=std::min(c1.z,c2.z);
			r.w=std::min(c1.w,c2.w);
			return r;
		}
		float4 float4::Max(const float4 &c1,const float4 &c2)
		{
			float4 r;
			r.x=std::max(c1.x,c2.x);
			r.y=std::max(c1.y,c2.y);
			r.z=std::max(c1.z,c2.z);
			r.w=std::max(c1.w,c2.w);
			return r;
		}
		float4 Exp(const float4 &c)
		{
			float4 r;
			r.x=exp(c.x);
			r.y=exp(c.y);
			r.z=exp(c.z);
			r.w=exp(c.w);
			return r;
		}
		float4 Ln(const float4 &c)
		{
			float4 r;
			r.x=log(c.x);
			r.y=log(c.y);
			r.z=log(c.z);
			r.w=log(c.w);
			return r;
		}
		float4 lerp(float x,const float4 &a,const float4 &b)
		{
			float4 r=x*b;
			r+=(1.f-x)*a;
			return r;
		}

		float length(const float4 &u)
		{
			float size=u.x*u.x+u.y*u.y+u.z*u.z;
			return sqrt(size);
		}
		float largest(const float4 &u)
		{
			float m=std::max(u.x,u.y);
			m=std::max(m,u.z);
			m=std::max(m,u.w);
			return m;
		}

		void zero(float4 &u)
		{
			u.x=0;
			u.y=0;
			u.z=0;
			u.w=0;
		}

		void unit(float4 &u)
		{
			float size=u.x*u.x+u.y*u.y+u.z*u.z;
			size=sqrt(size);
			u.x/=size;
			u.y/=size;
			u.z/=size;
		}
		float4 saturate(const float4 &l)
		{
			float4 r=l;
			if(r.x>1.f)
				r.x=1.f;
			if(r.y>1.f)
				r.y=1.f;
			if(r.z>1.f)
				r.z=1.f;
			if(r.w>1.f)
				r.w=1.f;
			return r;
		}
	}
}

void cross(float4 &c,const float4 &a,const float4 &b)
{
	c.x=a.y*b.z-a.z*b.y;
	c.y=a.z*b.x-a.x*b.z;
	c.z=a.x*b.y-a.y*b.x;
}

float clamp(float x,float l,float h)
{
	if(x<l)
		return l;
	if(x>h)
		return h;
	return x;
}

void zero(float4 &u)
{
	u.x=0;
	u.y=0;
	u.z=0;
	u.w=0;
}

void unit(float4 &u)
{
	float size=u.x*u.x+u.y*u.y+u.z*u.z;
	size=sqrt(size);
	u.x/=size;
	u.y/=size;
	u.z/=size;
}

void add(float4 &c,const float4 &a,const float4 &b)
{
	c.x=a.x+b.x;
	c.y=a.y+b.y;
	c.z=a.z+b.z;
}
