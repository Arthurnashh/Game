//-----------------------------------------------------------------------------
// File: enf_mathlib.cpp
//
// Desc: Mathematics library
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#ifdef __GCC__
	#include <stdlib.h>
#endif

#include <stdlib.h>
//#include "common/enf_memory.h"
#include "common/enf_mathlib.h"
#include "common/enf_mathlib2d.h"


namespace enf
{
uint _enf_holdrand;

/*
Catmull rom spline
q(t) = 0.5 *(    (2 * P1) + 
(-P0 + P2) * t +
(2*P0 - 5*P1 + 4*P2 - P3) * t2 +
(-P0 + 3*P1- 3*P2 + P3) * t3) 
*/
template<class T> T SawToothWave(T time, float freq, float amp)
{
	float t = time / freq;
	return enf::enf_limit((float)(t - floor(t)), 0.0f, 1.0f) * amp;
}

template<class T> T TriangleWave(T time, float freq, float amp)
{
	T t = time / freq;
	T a = enf::enf_limit((float)(t - floor(t)), 0.0f, 1.0f);
	if(a > 0.5f)
		a = 0.5f - (a - 0.5f);
	return a * amp * 2.0f;
}


//-----------------------------------------------------------------------
#if 0
Vector3 strtovec(const char *str) 
{ 
	Vector3 out(0.0, 0.0, 0.0); 

	//return zeroes if str is null 
	if (!str)  
		return out; 

	char tmp[50]; 
	int i = 0, j = 0; 

	//for each character 
	while (j < 3) 
	{
		if (!*str || *str == ',' || *str == ' ' || *str == '\t') 
		{ 
			//more spaces, commas,...
			if (i == 0 && *str)
			{
				str++;
				continue;
			}

			tmp[i] = '\0'; 
			out.v[j++] = atof(tmp); 

			if (!*str) 
				break; 

			i = 0; 
			str++; 
		}
		else 
		{ 
			tmp[i++] = *str; 
			str++; 
		} 
	}
	return out; 
} 

//-----------------------------------------------------------------------
ENF_ALIGN16(uint) fast_sqrt_table[0x10000];  // declare table of square roots 

typedef union FastSqrtUnion
{
  float f;
  unsigned int i;
} FastSqrtUnion;

void  build_sqrt_table()
{
  unsigned int i;
  FastSqrtUnion s;
  
  for (i = 0; i <= 0x7FFF; i++)
  {
    // Build a float with the bit pattern i as mantissa
    //  and an exponent of 0, stored as 127

    s.i = (i << 8) | (0x7F << 23);
    s.f = (float)sqrt(s.f);

    // Take the square root then strip the first 7 bits of
    //  the mantissa into the table
    fast_sqrt_table[i + 0x8000] = (s.i & 0x7FFFFF);

    // Repeat the process, this time with an exponent of 1, 
    //  stored as 128

    s.i = (i << 8) | (0x80 << 23);
    s.f = (float)sqrt(s.f);

    fast_sqrt_table[i] = (s.i & 0x7FFFFF);
  }
}

//-----------------------------------------------------------------------
void __mathInit()
{
	build_sqrt_table();
}
#endif
//-----------------------------------------------------------------------
Vector2::Vector2(const Vector3& vec): x(vec.X()), y(vec.Y())
{
}
//-----------------------------------------------------------------------
Vector2& Vector2::operator=(const Vector3& vec)
{
	x = vec.X();
	y = vec.Y();
	return *this;
}

//-----------------------------------------------------------------------
/*
Vector3::Vector3(const Vector2& v): x(v.x), y(v.y), z(0.0f)
{
}
*/
//-----------------------------------------------------------------------
AAB& AAB::InitFromMovingSphere(const Vector3& from, const Vector3& to, float radius)
{
	for(int i = 0; i < 3; i++)
	{
		if(to[i] > from[i])
		{
			Mins.v[i] = from[i] - radius;
			Maxs.v[i] = to[i] + radius;
		}
		else
		{
			Mins.v[i] = to[i] - radius;
			Maxs.v[i] = from[i] + radius;
		}
	}
	return *this;
}

//-----------------------------------------------------------------------
AAB& AAB::InitFromMovingBox(const Vector3& from, const Vector3& to, const AAB& box, float epsilon)
{
	for(int i = 0; i < 3; i++)
	{
		if(to[i] > from[i])
		{
			Mins.v[i] = from.v[i] + box.Mins.v[i] - epsilon;
			Maxs.v[i] = to.v[i] + box.Maxs.v[i] + epsilon;
		}
		else
		{
			Mins.v[i] = to.v[i] + box.Mins.v[i] - epsilon;
			Maxs.v[i] = from.v[i] + box.Maxs.v[i] + epsilon;
		}
	}

	return *this;
}

//-----------------------------------------------------------------------
AAB& AAB::AddPoints(const Vector3* points, uint numpoints, uint stride)
{
	Assert(numpoints != 0);

	//let the compiler to optimize memory accesses
	float minsx = enf_min(Mins.x, points->x);
	float minsy = enf_min(Mins.y, points->y);
	float minsz = enf_min(Mins.z, points->z);
	float maxsx = enf_max(Maxs.x, points->x);
	float maxsy = enf_max(Maxs.y, points->y);
	float maxsz = enf_max(Maxs.z, points->z);

	for(uint n = 1; n < numpoints; n++)
	{
		points = (Vector3*)((const char*)points + stride);
		minsx = enf_min(minsx, points->x);
		maxsx = enf_max(maxsx, points->x);
		minsy = enf_min(minsy, points->y);
		maxsy = enf_max(maxsy, points->y);
		minsz = enf_min(minsz, points->z);
		maxsz = enf_max(maxsz, points->z);
	}

	Mins.x = minsx;
	Mins.y = minsy;
	Mins.z = minsz;
	Maxs.x = maxsx;
	Maxs.y = maxsy;
	Maxs.z = maxsz;

	return *this;
}

//-----------------------------------------------------------------------
/*
AAB& AAB::AddOBB(const Matrix43& matrix, const AAB& bbox)
{ 
	Vector3 mins(matrix.m[3]), maxs(matrix.m[3]);

	float   a, b; 

	for (int j = 0;  j < 3; j++) 
	{
		for (int i = 0; i < 3; i++)
		{
			a = matrix.m[i][j] * bbox.Mins[i];
			b = matrix.m[i][j] * bbox.Maxs[i];

			if (a < b)
			{
				mins.v[j] += a;
				maxs.v[j] += b;
			}	else
			{
				mins.v[j] += b;
				maxs.v[j] += a;
			}
		}
	}

	AddMin(mins);
	AddMax(maxs);

	Assert(IsValid());
	return *this;
}

//-----------------------------------------------------------------------
AAB& AAB::InitFromOBB(const Matrix34& matrix, const AAB& box)
{
	Vector3 mins2, maxs2;
  	
	mins2 = maxs2 = matrix.GetTranslation();

	float   a, b; 

	for (int j = 0;  j < 3; j++) 
	{
		for (int i = 0; i < 3; i++)
		{
			a = matrix.m[j][i] * box.Mins[i];
			b = matrix.m[j][i] * box.Maxs[i];

			if (a < b)
			{
				mins2.v[j] += a;
				maxs2.v[j] += b;
			}
			else
			{
				mins2.v[j] += b;
				maxs2.v[j] += a;
			}
		}
	}

	Mins = mins2;
	Maxs = maxs2;

	Assert(IsValid());
	return *this;
}
*/
//-----------------------------------------------------------------------
AAB& AAB::InitFromOBB(const Matrix43& matrix, const AAB& box)
{
	float minsx, minsy, minsz;
	float maxsx, maxsy, maxsz;

	minsx = maxsx = matrix.Position().x;
	minsy = maxsy = matrix.Position().y;
	minsz = maxsz = matrix.Position().z;

	for (int i = 0; i < 3; i++)
	{
		float x = matrix[i][0];
		float a = x * box.Mins[i];
		float b = x * box.Maxs[i];
		minsx += (a < b) ? a : b;
		maxsx += (a < b) ? b : a;
	}
	for (int i = 0; i < 3; i++)
	{
		float y = matrix[i][1];
		float a = y * box.Mins[i];
		float b = y * box.Maxs[i];
		minsy += (a < b) ? a : b;
		maxsy += (a < b) ? b : a;
	}
	for (int i = 0; i < 3; i++)
	{
		float z = matrix[i][2];
		float a = z * box.Mins[i];
		float b = z * box.Maxs[i];
		minsz += (a < b) ? a : b;
		maxsz += (a < b) ? b : a;
	}

	Mins.x = minsx; Mins.y = minsy; Mins.z = minsz;
	Maxs.x = maxsx; Maxs.y = maxsy; Maxs.z = maxsz;

	Assert(IsValid());
	return *this;
}


//-----------------------------------------------------------------------------
Vector3 Quaternion::operator*( const Vector3& a ) const
{
	float xxzz = x * x - z * z;
	float wwyy = w * w - y * y;

	float xw2 = x * w * 2.0f;
	float xy2 = x * y * 2.0f;
	float xz2 = x * z * 2.0f;
	float yw2 = y * w * 2.0f;
	float yz2 = y * z * 2.0f;
	float zw2 = z * w * 2.0f;

	return Vector3((xxzz + wwyy) * a.x		+ (xy2 + zw2) * a.y		+ (xz2 - yw2) * a.z,
						(xy2 - zw2) * a.x			+ (xxzz - wwyy) * a.y		+ (yz2 + xw2) * a.z,
						(xz2 + yw2) * a.x			+ (yz2 - xw2) * a.y		+ (wwyy - xxzz) * a.z);
}

//-----------------------------------------------------------------------------
Vector3 Quaternion::ToMatrixRow(uint row) const
{
	Assert(row < 3);

	float fTX  = 2.0f * x;
	float fTY  = 2.0f * y;
	float fTZ  = 2.0f * z;

	Vector3 r;

	switch(row)
	{
	case 0:
		r.x  = 1.0f - ((fTY * y) + (fTZ * z));
		r.y  = (fTY * x) - (fTZ * w);
		r.z  = (fTZ * x) + (fTY * w);
		break;

	case 1:
		r.x  = (fTY * x) + (fTZ * w);
		r.y  = 1.0f - ((fTX * x) + (fTZ * z));
		r.z  = (fTZ * y) - (fTX * w);
		break;
		
	case 2:
		r.x  = (fTZ * x) - (fTY * w);
		r.y  = (fTZ * y) + (fTX * w);
		r.z = 1.0f - ( (fTX * x) + (fTY * y) );
		break;
	}
	return r;
}

//-----------------------------------------------------------------------------
float Quaternion::ToPitch() const
{
	Vector3 r;

	float fTX  = 2.0f * x;
	float fTY  = 2.0f * y;
	float fTZ  = 2.0f * z;
	float fTWY = fTY * w;
	float fTWZ = fTZ * w;
	float fTXY = fTY * x;
	float fTXZ = fTZ * x;
	float fTYY = fTY * y;
	float fTZZ = fTZ * z;

	r.x = 1.0f - ( fTYY + fTZZ );
	r.y = fTXY - fTWZ;
	r.z = fTXZ + fTWY;
	
	float ang = RAD2DEG(atan2(r.x, r.z));
	while(ang < 0)
		ang += 360.0f;

	while(ang >= 360.0f)
		ang -= 360.0f;

	return ang;
}

//-----------------------------------------------------------------------
Quaternion Quaternion::Lerp(const Quaternion& q, float lerp) const
{
	if ( lerp <= 0.0f )
	{
		return *this;
	}

	if ( lerp >= 1.0f )
	{
		return q;
	}

	Quaternion temp;

	float cosom = Dot(q);
	if ( cosom < 0.0f )
	{
		temp = -q;
		cosom = -cosom;
	}
	else
	{
		temp = q;
	}

#if 1 //PS3SDK
	if(cosom < 0.999f)
	{
		float angle = acosf( cosom );
		float recipSinAngle = ( 1.0f / sinf( angle ) );
		float scale0 = ( sinf( ( ( 1.0f - lerp ) * angle ) ) * recipSinAngle );
		float scale1 = ( sinf( ( lerp * angle ) ) * recipSinAngle );
		return ( *this * scale0) + ( temp * scale1 );
	}
	else
	{
		return ( *this * (1.0f - lerp)) + ( temp * lerp );
	}
#else
	if ( ( 1.0f - cosom ) > 1e-6f )
	{
#if 1
//Ogre
		float fSin = sqrtf(1 - sqr(cosom));
		float fAngle = atan2f(fSin, cosom);
		float fInvSin = 1.0f / fSin;
		float scale0 = sinf((1.0f - lerp) * fAngle) * fInvSin;
		float scale1 = sinf(lerp * fAngle) * fInvSin;
#else
// D3
		scale0 = 1.0f - sqr(cosom);
		float sinom = 1.0f / sqrtf( scale0 );
		float omega = atan2f( scale0 * sinom, cosom );
		float scale0 = sinf( ( 1.0f - lerp ) * omega ) * sinom;
		float scale1 = sinf( lerp * omega ) * sinom;
#endif
		return ( *this * scale0) + ( temp * scale1 );
	}
	else
	{
		return (( *this * (1.0f - lerp)) + ( temp * lerp )).Normalized();
	}
#endif
}

//-----------------------------------------------------------------------
Quaternion Quaternion::NLerp(const Quaternion& q, float lerp) const
{
	return (( (Vector4)(*this) * (1 - lerp)) + ( (Vector4)q * lerp )).Normalized();
}

//-----------------------------------------------------------------------
Quaternion Quaternion::Exponential() const
{
	Quaternion res;

	float A = sqrt(x*x + y*y + z*z);
	res.w = cos(A);

	float sinA = sin(A);
	float c = (abs(sinA) > 1e-6) ? sinA/A : 1.0f;

	res.x = c*x;
	res.y = c*y;
	res.z = c*z;

	return res;
}

//-----------------------------------------------------------------------
Quaternion Quaternion::Logarithm() const
{
	if (abs(w) >= 1.0f)
		return Quaternion(x, y, z, 0.0f);

	float A = cos(w);
	float sinA = sin(A);

	if (abs(sinA) < 1e-6f)
		return Quaternion(x, y, z, 0.0f);

	float c = A/sinA;
	return Quaternion(x*c, y*c, z*c, 0.0f);
}


//-----------------------------------------------------------------------
/*
Quaternion& Quaternion::operator=(const Matrix33& mat)
{
	float trace = mat._11 + mat._22 + mat._33;

	if ( trace > 0.0f )
	{
		float t = trace + 1.0f;
		float s = (1.0f / sqrtf(t)) * 0.5f;

		w = s * t;
		x = ( mat._23 - mat._32 ) * s;
		y = ( mat._31 - mat._13 ) * s;
		z = ( mat._12 - mat._21 ) * s;
	}
	else
	{
		const static int 	next[ 3 ] = { 1, 2, 0 };
		int i = 0;
		if ( mat._22 > mat._11 )
		{
			i = 1;
		}
		if ( mat._33 > mat.m[ i ][ i ] )
		{
			i = 2;
		}
		int j = next[ i ];
		int k = next[ j ];

		float t = ( mat.m[ i ][ i ] - ( mat.m[ j ][ j ] + mat.m[ k ][ k ] ) ) + 1.0f;
		float s = (1.0f / sqrtf(t)) * 0.5f;

		v[i] = s * t;
		w = ( mat.m[ j ][ k ] - mat.m[ k ][ j ] ) * s;
		v[j] = ( mat.m[ i ][ j ] + mat.m[ j ][ i ] ) * s;
		v[k] = ( mat.m[ i ][ k ] + mat.m[ k ][ i ] ) * s;
	}
	return *this; 
}

//-----------------------------------------------------------------------------
Matrix43 QKey::ToParent(const Matrix43& parent) const
{
#ifdef ENF_VMX_MATH
	Matrix43 res;
	XMMATRIX pmat = XMLoadFloat4x3((const XMFLOAT4X3*)&parent);
	XMMATRIX qmat = XMMatrixRotationQuaternion(XMLoadFloat4((const XMFLOAT4*)&m_qQuat));
	XMVECTOR trans = XMLoadFloat3((const XMFLOAT3*)&m_vTrans);
	trans.w = 1.0f;
	qmat.r[3] = trans;
	
	XMStoreFloat4x3((XMFLOAT4X3*)&res, XMMatrixMultiply(qmat, pmat));
	return res;
#else
	float qx = m_qQuat.x;
	float qy = m_qQuat.y;
	float qz = m_qQuat.z;
	float qw = m_qQuat.w;

	if(qx == 0.0f && qy == 0.0f && qz == 0.0f && qw == 1.0f)
	{
		Vector3 pos = parent.TransformCoord(m_vTrans);
		return Matrix43(parent._11, parent._12, parent._13,
							parent._21, parent._22, parent._23,
							parent._31, parent._32, parent._33,
							pos.x, pos.y, pos.z);
	}
	else
	{
		float	wx, wy, wz;
		float	xx, yy, yz;
		float	xy, xz, zz;
		float	x2, y2, z2;

		x2 = qx + qx;
		y2 = qy + qy;
		z2 = qz + qz;

		xx = qx * x2;
		xy = qx * y2;
		xz = qx * z2;

		yy = qy * y2;
		yz = qy * z2;
		zz = qz * z2;

		wx = qw * x2;
		wy = qw * y2;
		wz = qw * z2;

		return parent * Matrix43(1.0f - ( yy + zz ), xy + wz, xz - wy,
				xy - wz, 1.0f - ( xx + zz ), yz + wx,
				xz + wy, yz - wx, 1.0f - ( xx + yy ),
				m_vTrans.x, m_vTrans.y, m_vTrans.z);
	}
#endif
}

//-----------------------------------------------------------------------------
QKey QKey::Lerp(const QKey& q, float lerp) const
{
	if(lerp == 0 || (m_qQuat == q.m_qQuat && m_vTrans == q.m_vTrans))
		return *this;

	if(lerp == 1)
		return q;

	QKey qout;
	qout.m_qQuat = m_qQuat.Lerp(q.m_qQuat, lerp);
	qout.m_vTrans = m_vTrans.Lerp(q.m_vTrans, lerp);
	return qout;
}

//-----------------------------------------------------------------------------
QKey QKey::LerpRot(const QKey& q, float lerp) const
{
	if(lerp == 0 || m_qQuat == q.m_qQuat)
		return *this;

	if(lerp == 1)
		return q;

	QKey qout;
	qout.m_qQuat = m_qQuat.Lerp(q.m_qQuat, lerp);
	return qout;
}

//-----------------------------------------------------------------------
Vector3 Vector3::Perpendicular() const
{
	const Vector3 left(1.0f, 0.0f, 0.0f);

	float d = Dot(left);

	if(fabs(d) < 0.5f)
	{
		return (*this) ^ left;
	}

	const Vector3 forward(0.0f, 1.0f, 0.0f);

	d = Dot(forward);

	if(fabs(d) < 0.5f)
	{
		return (*this) ^ forward;
	}

	const Vector3 up(0.0f, 0.0f, 1.0f);

	return (*this) ^ up;
}

//-----------------------------------------------------------------------
Vector3 Vector3::RotatePointAroundVector(const Vector3& point, float degrees ) const
{
	float xx = x*point.x;
	float xy = x*point.y;
	float xz = x*point.z;
	float yx = y*point.x;
	float yy = y*point.y;
	float yz = y*point.z;
	float zx = z*point.x;
	float zy = z*point.y;
	float zz = z*point.z;
	float sa = sinf(DEG2RAD(degrees));
	float ca = cosf(DEG2RAD(degrees));

	return Vector3(
        x*(xx+yy+zz) + (point.x*(y*y+z*z)-x*(yy+zz))*ca+(-zy+yz)*sa,
		y*(xx+yy+zz) + (point.y*(x*x+z*z)-y*(xx+zz))*ca+(zx-xz)*sa,
		z*(xx+yy+zz) + (point.z*(x*x+y*y)-z*(xx+yy))*ca+(-yx+xy)*sa
		);
}
*/
//-----------------------------------------------------------------------
MPlane& MPlane::FromPoints(const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	Vector3 edge1 = p1 - p0;
	Vector3 edge2 = p2 - p0;
	Vector3 norm = edge2.CrossProduct(edge1).Normalized();
	x = norm.x;
	y = norm.y;
	z = norm.z;
	w = norm.DotProduct(p0);
	Type = 0;
	return *this;
}

//-----------------------------------------------------------------------
MPlane& MPlane::FromPointsNN(const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	Vector3 edge1 = p1 - p0;
	Vector3 edge2 = p2 - p0;
	Vector3 norm = edge2.CrossProduct(edge1);
	x = norm.x;
	y = norm.y;
	z = norm.z;
	w = norm.DotProduct(p0);
	Type = 0;
	return *this;
}

//-----------------------------------------------------------------------
Vector3 MPlane::Project(const Vector3& pt) const
{
	float d = -Distance(pt);

	return Vector3(x * d + pt.x, y * d + pt.y, z * d + pt.z);
}

#define COLINEAR_EPSILON 0.01

//-----------------------------------------------------------------------
bool Colinear(const Vector3& a, const Vector3& b, const Vector3& c)
{
	Vector3 v1 = (a - b).Normalized();
	Vector3 v2 = (b - c).Normalized();
   float f = v1.DotProduct(v2);

	return (f > (1.0 - COLINEAR_EPSILON) && f < (1.0 + COLINEAR_EPSILON));
}

#define FP_ABS_BITS(fp) (FP_BITS(fp)&0x7FFFFFFF)
#define FP_SIGN_BIT(fp) (FP_BITS(fp)&0x80000000)
#define FP_ONE_BITS 0x3F800000


// r = 1/p
#define FP_INV(r,p)                                                          \
{                                                                            \
    int _i = 2 * FP_ONE_BITS - *(int *)&(p);                                 \
    r = *(float *)&_i;                                                       \
    r = r * (2.0f - (p) * r);                                                \
}
/*
/////////////////////////////////////////////////
// The following comes from Vincent Van Eeckhout
// Thanks for sending us the code!
// It's the same thing in assembly but without this C-needed line:
//    r = *(float *)&_i;

float   two = 2.0f;

#define FP_INV2(r,p)                     \
{                                        \
    __asm { mov     eax,0x7F000000    }; \
    __asm { sub     eax,dword ptr [p] }; \
    __asm { mov     dword ptr [r],eax }; \
    __asm { fld     dword ptr [p]     }; \
    __asm { fmul    dword ptr [r]     }; \
    __asm { fsubr   [two]             }; \
    __asm { fmul    dword ptr [r]     }; \
    __asm { fstp    dword ptr [r]     }; \
}

/////////////////////////////////////////////////


#define FP_EXP(e,p)                                                          \
{                                                                            \
    int _i;                                                                  \
    e = -1.44269504f * (float)0x00800000 * (p);                              \
    _i = (int)e + 0x3F800000;                                                \
    e = *(float *)&_i;                                                       \
}

#define FP_NORM_TO_BYTE(i,p)                                                 \
{                                                                            \
    float _n = (p) + 1.0f;                                                   \
    i = *(int *)&_n;                                                         \
    if (i >= 0x40000000)     i = 0xFF;                                       \
    else if (i <=0x3F800000) i = 0;                                          \
    else i = ((i) >> 15) & 0xFF;                                             \
}

*/

ENF_INLINE unsigned long FP_NORM_TO_BYTE2(float p)                                                 
{                                                                            
  float fpTmp = p + 1.0f;                                                      
  return ((*(unsigned *)&fpTmp) >> 15) & 0xFF;  
}


ENF_INLINE unsigned long FP_NORM_TO_BYTE3(float p)     
{
  float ftmp = p + 12582912.0f;                                                      
  return ((*(unsigned long *)&ftmp) & 0xFF);
}



//-----------------------------------------------------------------------
// A triangle made up of three vertices

#define V_MAX 64       // Maximum number of vertices allowed (arbitrary)
#define BIG 1.0e30f      // A number bigger than we expect to find here
#define COUNTER_CLOCKWISE 0
#define CLOCKWISE 1

//-----------------------------------------------------------------------
int Orientation(int n, const Vector3 *v )
{
	float area = v[n-1][0] * v[0][1] - v[0][0] * v[n-1][1];

	// Compute the area (times 2) of the polygon
	for (int i = 0; i < n-1; i++)
		area += v[i][0] * v[i+1][1] - v[i+1][0] * v[i][1];

	if (area >= 0.0)
		return COUNTER_CLOCKWISE;
	else
		return CLOCKWISE;
}

//-----------------------------------------------------------------------
int Determinant( int p1, int p2, int p3, const Vector3 *v )
{
	float x1 = v[p1][0];
	float y1 = v[p1][1];
	float x2 = v[p2][0];
	float y2 = v[p2][1];
	float x3 = v[p3][0];
	float y3 = v[p3][1];
	float determ = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
	if (determ >= 0.0)
		return COUNTER_CLOCKWISE;
	else
		return CLOCKWISE;
}

//-----------------------------------------------------------------------
float Distance2(float x1, float y1, float x2, float y2)
{
	float xd = x1 - x2;
	float yd = y1 - y2;
	float dist2 = xd * xd + yd * yd;

	return dist2;
}

//-----------------------------------------------------------------------
int NoInterior( int p1, int p2, int p3, const Vector3 *v, const int *vp, int n, int poly_or )
{
	for (int i = 0; i < n; i++)
	{
		int p = vp[i];              // The point to test
		if ((p == p1) || (p == p2) || (p == p3))
			continue;           // Don't bother checking against yourself
		
		if ((Determinant( p2, p1, p, v ) == poly_or) ||
			(Determinant( p1, p3, p, v ) == poly_or) ||
			(Determinant( p3, p2, p, v ) == poly_or))
			continue;           // This point is outside
		else
			return 0;           // The point is inside
	}
	return 1;                   // No points inside this triangle
}

//-----------------------------------------------------------------------
int PolyToTriangles(int n, const Vector3 *v, Vector3 *pv )
{
	int outv = 0;
	int prev, cur, next;        /* Three points currently being considered */
	int vp[V_MAX];              /* Pointers to vertices still left */
	int count;                  /* How many vertices left */
	int min_vert;               /* Vertex with minimum distance */
	int i;                      /* Iterative counter */
	float dist;                 /* Distance across this one */
	float min_dist;             /* Minimum distance so far */
	int poly_orientation;       /* Polygon orientation */

	for (i = 0; i < n; i++)
		vp[i] = i;              // Put vertices in order to begin

	if(n > 3)
	{
		poly_orientation = Orientation( n, v );

	// Slice off clean triangles until nothing remains

		 count = n;
		 while (count > 3) {
			  min_dist = BIG;         /* A real big number */
			  min_vert = 0;           /* Just in case we don't find one... */
			  for (cur = 0; cur < count; cur++) {
					prev = cur - 1;
					next = cur + 1;
					if (cur == 0)       /* Wrap around on the ends */
						 prev = count - 1;
					else if (cur == count - 1)
						 next = 0;
					/* Pick out shortest distance that forms a good triangle */
					if (   (Determinant( vp[prev], vp[cur], vp[next], v ) == poly_orientation)
							  /* Same orientation as polygon */
						 && NoInterior( vp[prev], vp[cur], vp[next], v, vp, count, poly_orientation )
							  /* No points inside */
						 && ((dist = Distance2( v[vp[prev]][0], v[vp[prev]][1],
														v[vp[next]][0], v[vp[next]][1] )) < min_dist) )
							  /* Better than any so far */
					{
						 min_dist = dist;
						 min_vert = cur;
					}
			  } /* End of for each vertex (cur) */

			  /* The following error should "never happen". */
			  //Assert(min_dist != BIG);

			  prev = min_vert - 1;
			  next = min_vert + 1;
			  if (min_vert == 0)      /* Wrap around on the ends */
					prev = count - 1;
			  else if (min_vert == count - 1)
					next = 0;

	// Output this triangle
			pv[outv++] = v[vp[prev]];
			pv[outv++] = v[vp[min_vert]];
			pv[outv++] = v[vp[next]];

			count -= 1;
			for (i = min_vert; i < count; i++)
				vp[i] = vp[i+1];
		}
	}

	pv[outv++] = v[vp[0]];
	pv[outv++] = v[vp[1]];
	pv[outv++] = v[vp[2]];

	return outv;
}

//-----------------------------------------------------------------------
int PolyToTriangles(const Vector3 *points, uint n, const uint *v, uint *pv )
{
	int outv = 0;
	int prev, next;        /* Three points currently being considered */
	int vp[V_MAX];              /* Pointers to vertices still left */
	uint count;                  /* How many vertices left */
	int min_vert;               /* Vertex with minimum distance */
	uint i, cur;                      /* Iterative counter */
	float dist;                 /* Distance across this one */
	float min_dist;             /* Minimum distance so far */
	int poly_orientation;       /* Polygon orientation */

	for (i = 0; i < n; i++)
		vp[i] = v[i];              // Put vertices in order to begin

	if(n > 3)
	{
		 float area;

		 area = points[v[n-1]][0] * points[v[0]][1] - points[v[0]][0] * points[v[n-1]][1];

		 // Compute the area (times 2) of the polygon
		 for (i = 0; i < n - 1; i++)
			  area += points[v[i]][0] * points[v[i+1]][1] - points[v[i+1]][0] * points[v[i]][1];

		 if (area >= 0.0)
			  poly_orientation = COUNTER_CLOCKWISE;
		 else
			  poly_orientation = CLOCKWISE;

	// Slice off clean triangles until nothing remains

		 count = n;
		 while (count > 3) {
			  min_dist = BIG;         /* A real big number */
			  min_vert = 0;           /* Just in case we don't find one... */
			  for (cur = 0; cur < count; cur++) {
					prev = cur - 1;
					next = cur + 1;
					if (cur == 0)       /* Wrap around on the ends */
						 prev = count - 1;
					else if (cur == count - 1)
						 next = 0;
					/* Pick out shortest distance that forms a good triangle */
					if (   (Determinant( vp[prev], vp[cur], vp[next], points ) == poly_orientation)
							  /* Same orientation as polygon */
						 && NoInterior( vp[prev], vp[cur], vp[next], points, vp, count, poly_orientation )
							  /* No points inside */
						 && ((dist = Distance2( points[vp[prev]][0], points[vp[prev]][1],
														points[vp[next]][0], points[vp[next]][1] )) < min_dist) )
							  /* Better than any so far */
					{
						 min_dist = dist;
						 min_vert = cur;
					}
			  } /* End of for each vertex (cur) */

			  /* The following error should "never happen". */
				Assert (min_dist != BIG);

			  prev = min_vert - 1;
			  next = min_vert + 1;
			  if (min_vert == 0)      /* Wrap around on the ends */
					prev = count - 1;
			  else if (min_vert == count - 1)
					next = 0;

	// Output this triangle
			pv[outv++] = vp[prev];
			pv[outv++] = vp[min_vert];
			pv[outv++] = vp[next];

			count -= 1;
			for (i = min_vert; i < count; i++)
				vp[i] = vp[i+1];
		}
	}

	pv[outv++] = vp[0];
	pv[outv++] = vp[1];
	pv[outv++] = vp[2];

	return outv;
}

}