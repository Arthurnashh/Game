#ifndef SIMUL_SKY_float2_H
#define SIMUL_SKY_float2_H
#include "Simul/Math/Export.h"

namespace simul
{
	namespace math
	{
		SIMUL_MATH_EXPORT_STRUCT float2
		{
			float x,y;
			float2(){}
			static float dot(const float2 &a,const float2 &b)
			{
				return a.x*b.x+a.y*b.y;
			}
			static float length(const float2 &a);
			float2(const float2 &f)
			{
				x=f.x;
				y=f.y;
			}
			float2(const float* f)
			{
				x=f[0];
				y=f[1];
			}
			const float2 &operator=(const float* f)
			{
				x=f[0];
				y=f[1];
				return *this;
			}
			static float cross(const float2 &a,const float2 &b)
			{
				return a.x*b.y-a.y*b.x;
			}
			operator const float*() const
			{
				return (const float*)this;
			}
			operator float*()
			{
				return (float*)this;
			}
			void operator=(const float2 &f)
			{
				x=f.x;
				y=f.y;
			}
			bool operator<(const float2 &f)
			{
				return(x<f.x&&y<f.y);
			}
			bool operator>(const float2 &f)
			{
				return(x>f.x&&y>f.y);
			}
			bool operator==(const float2 &f)
			{
				return(x==f.x&&y==f.y);
			}
			float2(float X,float Y)
			{
				x=X;
				y=Y;
			}
			float2 operator -() const
			{
				float2 r;
				r.x=-x;
				r.y=-y;
				return r;
			}
			float2 operator -(const float2 &f)
			{
				float2 r;
				r.x=x-f.x;
				r.y=y-f.y;
				return r;
			}
			float2 operator +(const float2 &f)
			{
				float2 r;
				r.x=x+f.x;
				r.y=y+f.y;
				return r;
			}
			float2 &operator +=(const float2 &f)
			{
				x+=f.x;
				y+=f.y;
				return *this;
			}
			float2 &operator -=(const float2 &f)
			{
				x-=f.x;
				y-=f.y;
				return *this;
			}
			float2 &operator *=(const float2 &f)
			{
				x*=f.x;
				y*=f.y;
				return *this;
			}
			float2 &operator /=(const float2 &f)
			{
				x/=f.x;
				y/=f.y;
				return *this;
			}
			float2 &operator *=(float f)
			{
				x*=f;
				y*=f;
				return *this;
			}
			float2 &operator /=(float f)
			{
				x/=f;
				y/=f;
				return *this;
			}
			friend float2 operator *(float f,const float2 &c)
			{
				float2 r;
				r.x=c.x*f;
				r.y=c.y*f;
				return r;
			}
			friend float2 operator *(const float2 &c1,const float2 &c2)
			{
				float2 r;
				r.x=c1.x*c2.x;
				r.y=c1.y*c2.y;
				return r;
			}
			friend float2 operator /(const float2 &c1,const float2 &c2)
			{
				float2 r;
				r.x=c1.x/c2.x;
				r.y=c1.y/c2.y;
				return r;
			}
			friend float2 operator *(const float2 &c,float f)
			{
				float2 r;
				r.x=c.x*f;
				r.y=c.y*f;
				return r;
			}
			friend float2 operator /(const float2 &c,float f)
			{
				float2 r;
				r.x=c.x/f;
				r.y=c.y/f;
				return r;
			}
			friend float2 operator+(const float2 &c,const float2 &d)
			{
				float2 r;
				r.x=c.x+d.x;
				r.y=c.y+d.y;
				return r;
			}
			friend float2 operator-(const float2 &c,const float2 &d)
			{
				float2 r;
				r.x=c.x-d.x;
				r.y=c.y-d.y;
				return r;
			}
			friend float2 operator+(const float2 &c,float d)
			{
				float2 r;
				r.x=c.x+d;
				r.y=c.y+d;
				return r;
			}
			friend float2 operator-(const float2 &c,float d)
			{
				float2 r;
				r.x=c.x-d;
				r.y=c.y-d;
				return r;
			}
			friend float2 operator+(float d,const float2 &c)
			{
				float2 r;
				r.x=c.x+d;
				r.y=c.y+d;
				return r;
			}
			friend float2 operator-(float d,const float2 &c)
			{
				float2 r;
				r.x=d-c.x;
				r.y=d-c.y;
				return r;
			}
			void Set(float X,float Y)
			{
				x=X;
				y=Y;
			}
			friend float2 lerp(float x,const float2 &a,const float2 &b)
			{
				float2 r=x*b;
				r+=(1.f-x)*a;
				return r;
			}
			static float2 Min(const float2 &a,const float2 &b)
			{
				float2 r;
				r.x=(a.x<b.x)?a.x:b.x;
				r.y=(a.y<b.y)?a.y:b.y;
				return r;
			}
			static float2 Max(const float2 &a,const float2 &b)
			{
				float2 r;
				r.x=(a.x>b.x)?a.x:b.x;
				r.y=(a.y>b.y)?a.y:b.y;
				return r;
			}
		};
		extern float2 Exp(const float2 &c);
		//extern float length(const float2 &f);
	}
}
#endif
