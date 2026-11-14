// homogenous vector and matrix arithmetics
// (C) 1997, SUMA
#include <El/elementpch.hpp>

// Intel compiler implementation of matrix operations
 
#if 0 // defined _PIII && defined _USE_INTEL_COMPILER

// use this to provide some working assembly code that can be hand tuned
// in MASM

#define BASE_CODE_FROM_INTEL_COMPILER 1
#if BASE_CODE_FROM_INTEL_COMPILER

#include <xmmintrin.h>

#if 0
void Vector3P3::DoSetFastTransform( const Matrix4P3 &a, Vector3P3Par o )
{
	// matrix stored in major-column format
	__m128 t1,t2,t3;

	t1 = _mm_set_ps1(o.X());
	t2 = _mm_set_ps1(o.Y());

	#define MQ(a) ( *(__m128 *)&(a)._e )
	t1 = _mm_mul_ps( t1, MQ(a._orientation._columns[0]) );
	t2 = _mm_mul_ps( t2, MQ(a._orientation._columns[1]) );

	t3 = _mm_set_ps1(o.Z());

	t1 = _mm_add_ps( t1, t2 );
	t3 = _mm_mul_ps( t3, MQ(a._orientation._columns[2]) );

	// sum a...
	t3 = _mm_add_ps( t3, MQ(a._position) );
	MQ(*this) = _mm_add_ps( t1, t3 );

}
#endif

extern "C" void TransformPoints
(
	Vector3P3 *dst, const Vector3P3 *src, int n,
	const Matrix4 &posView
);

//extern "C"
void TransformPoints
(
	Vector3P3 *dst, const Vector3P3 *src, int n,
	const Matrix4 &posView
)
{
	#define MQ(a) ( *(__m128 *)&((a)[0]) )
	//#define MQ(a) ( &((a)[0]) )
	Vector3 dx=posView.DirectionAside();
	Vector3 dy=posView.DirectionUp();
	Vector3 dz=posView.Direction();
	Vector3 p=posView.Position();
	__m128 m4=MQ(dx);
	__m128 m5=MQ(dy);
	__m128 m6=MQ(dz);
	__m128 m7=MQ(p);

	//while( (n-=2)>0 )	
	while( (n-=2)>=0 )
	{
		_mm_prefetch((char *)(src+2),_MM_HINT_NTA);
	
		{
			__m128 t0=_mm_set_ps1(src->X());
			__m128 t1=_mm_set_ps1(src->Y());
			__m128 t2=_mm_set_ps1(src->Z());

			t0=_mm_mul_ps(t0,m4);
			t1=_mm_mul_ps(t1,m5);
			t2=_mm_mul_ps(t2,m6);

			t2=_mm_add_ps(t2,m7);
			t0=_mm_add_ps(t0,t1);
			MQ(dst[0])=_mm_add_ps(t0,t2);
		}

		{
			__m128 t0=_mm_set_ps1(src[1].X());
			__m128 t1=_mm_set_ps1(src[1].Y());
			__m128 t2=_mm_set_ps1(src[1].Z());

			t0=_mm_mul_ps(t0,m4);
			t1=_mm_mul_ps(t1,m5);
			t2=_mm_mul_ps(t2,m6);

			t2=_mm_add_ps(t2,m7);
			t0=_mm_add_ps(t0,t1);
			MQ(dst[1])=_mm_add_ps(t0,t2);
		}
		dst+=2;
		src+=2;
	}
	if( n&1 )
	{
		__m128 t0=_mm_set_ps1(src->X());
		__m128 t1=_mm_set_ps1(src->Y());
		__m128 t2=_mm_set_ps1(src->Z());

		t0=_mm_mul_ps(t0,m4);
		t1=_mm_mul_ps(t1,m5);
		t2=_mm_mul_ps(t2,m6);

		t2=_mm_add_ps(t2,m7);
		t0=_mm_add_ps(t0,t1);
		MQ(dst[0])=_mm_add_ps(t0,t2);
	}
}

#endif

#endif
