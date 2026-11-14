#ifndef SIMUL_MATH_FLOAT3_H
#define SIMUL_MATH_FLOAT3_H
#include "Simul/Math/Export.h"

namespace simul
{
	namespace math
	{
		SIMUL_MATH_EXPORT_STRUCT float3
		{
			float x,y,z;
			float3(){}
			static float dot(const float3 &a,const float3 &b)
			{
				return a.x*b.x+a.y*b.y+a.z*b.z;
			}
			static float length(const float3 &a);
			float3(const float3 &f)
			{
				x=f.x;
				y=f.y;
				z=f.z;
			}
			float3(const float* f)
			{
				x=f[0];
				y=f[1];
				z=f[2];
			}
			const float3 &operator=(const float* f)
			{
				x=f[0];
				y=f[1];
				z=f[2];
				return *this;
			}
			static float3 cross(const float3 &a,const float3 &b)
			{
				float3 r;
				r.x=a.y*b.z-a.z*b.y;
				r.y=a.z*b.x-a.x*b.z;
				r.z=a.x*b.y-a.y*b.x;
				return r;
			}
			operator const float*() const
			{
				return (const float*)this;
			}
			operator float*()
			{
				return (float*)this;
			}
			void operator=(const float3 &f)
			{
				x=f.x;
				y=f.y;
				z=f.z;
			}
			bool operator<(const float3 &f)
			{
				return(x<f.x&&y<f.y&&y<f.z);
			}
			bool operator>(const float3 &f)
			{
				return(x>f.x&&y>f.y&&z>f.z);
			}
			bool operator==(const float3 &f)
			{
				return(x==f.x&&y==f.y&&z==f.z);
			}
			float3(float X,float Y,float Z)
			{
				x=X;
				y=Y;
				z=Z;
			}
			float3 operator -() const
			{
				float3 r;
				r.x=-x;
				r.y=-y;
				r.z=-z;
				return r;
			}
			float3 operator -(const float3 &f)
			{
				float3 r;
				r.x=x-f.x;
				r.y=y-f.y;
				r.z=z-f.z;
				return r;
			}
			float3 operator +(const float3 &f)
			{
				float3 r;
				r.x=x+f.x;
				r.y=y+f.y;
				r.z=z+f.z;
				return r;
			}
			float3 &operator +=(const float3 &f)
			{
				x+=f.x;
				y+=f.y;
				z+=f.z;
				return *this;
			}
			float3 &operator -=(const float3 &f)
			{
				x-=f.x;
				y-=f.y;
				z-=f.z;
				return *this;
			}
			float3 &operator *=(const float3 &f)
			{
				x*=f.x;
				y*=f.y;
				z*=f.z;
				return *this;
			}
			float3 &operator /=(const float3 &f)
			{
				x/=f.x;
				y/=f.y;
				z/=f.z;
				return *this;
			}
			float3 &operator *=(float f)
			{
				x*=f;
				y*=f;
				z*=f;
				return *this;
			}
			float3 &operator /=(float f)
			{
				x/=f;
				y/=f;
				z/=f;
				return *this;
			}
			friend float3 operator *(float f,const float3 &c)
			{
				float3 r;
				r.x=c.x*f;
				r.y=c.y*f;
				r.z=c.z*f;
				return r;
			}
			friend float3 operator *(const float3 &c1,const float3 &c2)
			{
				float3 r;
				r.x=c1.x*c2.x;
				r.y=c1.y*c2.y;
				r.z=c1.z*c2.z;
				return r;
			}
			friend float3 operator /(const float3 &c1,const float3 &c2)
			{
				float3 r;
				r.x=c1.x/c2.x;
				r.y=c1.y/c2.y;
				r.z=c1.z/c2.z;
				return r;
			}
			friend float3 operator *(const float3 &c,float f)
			{
				float3 r;
				r.x=c.x*f;
				r.y=c.y*f;
				r.z=c.z*f;
				return r;
			}
			friend float3 operator /(const float3 &c,float f)
			{
				float3 r;
				r.x=c.x/f;
				r.y=c.y/f;
				r.z=c.z/f;
				return r;
			}
			friend float3 operator+(const float3 &c,const float3 &d)
			{
				float3 r;
				r.x=c.x+d.x;
				r.y=c.y+d.y;
				r.z=c.z+d.z;
				return r;
			}
			friend float3 operator-(const float3 &c,const float3 &d)
			{
				float3 r;
				r.x=c.x-d.x;
				r.y=c.y-d.y;
				r.z=c.z-d.z;
				return r;
			}
			friend float3 operator+(const float3 &c,float d)
			{
				float3 r;
				r.x=c.x+d;
				r.y=c.y+d;
				r.z=c.z+d;
				return r;
			}
			friend float3 operator-(const float3 &c,float d)
			{
				float3 r;
				r.x=c.x-d;
				r.y=c.y-d;
				r.z=c.z-d;
				return r;
			}
			friend float3 operator+(float d,const float3 &c)
			{
				float3 r;
				r.x=c.x+d;
				r.y=c.y+d;
				r.z=c.z+d;
				return r;
			}
			friend float3 operator-(float d,const float3 &c)
			{
				float3 r;
				r.x=d-c.x;
				r.y=d-c.y;
				r.z=d-c.z;
				return r;
			}
			void Set(float X,float Y,float Z)
			{
				x=X;
				y=Y;
				z=Z;
			}
			friend float3 lerp(float x,const float3 &a,const float3 &b)
			{
				float3 r=x*b;
				r+=(1.f-x)*a;
				return r;
			}
			static float3 Min(const float3 &a,const float3 &b)
			{
				float3 r;
				r.x=(a.x<b.x)?a.x:b.x;
				r.y=(a.y<b.y)?a.y:b.y;
				r.z=(a.z<b.z)?a.z:b.z;
				return r;
			}
			static float3 Max(const float3 &a,const float3 &b)
			{
				float3 r;
				r.x=(a.x>b.x)?a.x:b.x;
				r.y=(a.y>b.y)?a.y:b.y;
				r.z=(a.z>b.z)?a.z:b.z;
				return r;
			}
		};
		extern float3 Exp(const float3 &c);
		//extern float length(const float3 &f);
	}
}
#endif
