//-----------------------------------------------------------------------------
// File: enf_simdmathlib.h
//
// Desc: Mathematics library
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SIMDMATHLIB_H
#define ENF_SIMDMATHLIB_H

#include "enf_config.h"
#include <float.h>
#include <math.h>
#include "common/enf_types.h"
#include "common/enf_mathlib.h"

#ifdef ENF_XENON
	#include <VectorIntrinsics.h>
	#include <PPCIntrinsics.h>
#elif defined(ENF_PS3)
		#include <vec_types.h>
	#ifdef __PPU__
		#include <altivec.h>
	#else
		#error not implemented
		#include <vmx2spu.h>
	#endif
#elif defined(ENF_SSE)
	#include <emmintrin.h>
#endif

namespace enf
{
/** @defgroup Math3D 3D Mathematic library for Altivec/SPU/SSE
@ingroup Common
@{*/

#if defined(ENF_XENON) || defined(ENF_PS3) || defined(ENF_SSE)
	#define ENF_SIMDMATH
#endif

#define ALIGNED16_PTR(ptr) ((ptr) && ((intptr_t)(ptr) & 0xf) == 0)
#define ALIGNED4_PTR(ptr) ((ptr) && ((intptr_t)(ptr) & 0x3) == 0)

class SimdMatrix34;

typedef ENF_ALIGN16(struct) SimdVectorF32
{
	union
	{
		float f[4];
#ifdef ENF_XENON
		__vector4 v;
#elif defined(ENF_PS3)
		vec_float4 v;
#elif defined(ENF_SSE)
		__m128 v;
#endif
	};
} SimdVectorF32;

typedef ENF_ALIGN16(struct) SimdVectorI32
{
	union
	{
		int i[4];
#ifdef ENF_XENON
		__vector4 v;
#elif defined(ENF_PS3)
		vec_int4 v;
		vec_float4 vf;
#elif defined(ENF_SSE)
		__m128 v;
#endif
	};
} SimdVectorI32;


//-----------------------------------------------------------------------------
#ifdef ENF_XENON
__declspec(passinreg) ENF_ALIGN16(class) SimdScalar: public __vector4
{

#elif defined(ENF_PS3)
ENF_ALIGN16(class) SimdScalar
{
protected:
	vec_float4 vec;

#elif defined(ENF_SSE)
static const SimdVectorF32 g_SimdOne = { 1.0f, 1.0f, 1.0f, 1.0f};
ENF_ALIGN16(class) SimdScalar
{
protected:
	__m128 vec;
#endif

public:
	SimdScalar() {}

#ifdef ENF_XENON
	SimdScalar(__vector4 v): __vector4(v) {}
	SimdScalar(float v): __vector4(__lvlx(&v, 0))
	{
	}
	operator __vector4() const { return *this; }
#elif defined(ENF_PS3)
	SimdScalar(vec_float4 v): vec(v) {}
	SimdScalar(float v): vec(vec_lvlx(0, &v))
	{
	}
	operator vec_float4() const { return vec; }
#elif defined(ENF_SSE)
	SimdScalar(__m128 v): vec(v) {}
	SimdScalar(float v)
	{
		vec = _mm_set_ss(v);
	}
	operator __m128() const { return vec; }
#endif

	ENF_INLINE static SimdScalar Zero()
	{
#ifdef ENF_XENON
		return __vzero();
#elif defined(ENF_PS3)
		return vec_float4(0.0f);
#elif defined(ENF_SSE)
		return SimdScalar(_mm_setzero_ps());
#endif
	}
	ENF_INLINE static SimdScalar One()
	{
#ifdef ENF_XENON
		return __vupkd3d(__vzero(), VPACK_D3DCOLOR);
#elif defined(ENF_PS3)
		return vec_float4(1.0f);
#elif defined(ENF_SSE)
		return g_SimdOne.v;
#endif
	}

	ENF_INLINE static SimdScalar LoadFloat(const float* ptr)
	{
		enf_assert(ALIGNED4_PTR(ptr));
#ifdef ENF_XENON
		return __lvlx(ptr, 0);
#elif defined(ENF_PS3)
		return vec_lvlx(0, ptr);
#elif defined(ENF_SSE)
		return _mm_load_ss(ptr);
#endif
	}
	void StoreFloat(float* ptr) const
	{
		enf_assert(ALIGNED16_PTR(ptr));
#ifdef ENF_XENON
		__stvx(*this, ptr, 0);
#elif defined(ENF_PS3)
		vec_stvlx(vec, 0, ptr);
#elif defined(ENF_SSE)
		_mm_store_ss(ptr, vec);
#endif
	}
	SimdScalar operator+(const SimdScalar& v) const
	{
#ifdef ENF_XENON
		return __vaddfp(*this, v);
#elif defined(ENF_PS3)
		return vec_add(vec, v);
#elif defined(ENF_SSE)
		return _mm_add_ss(vec, v);
#endif
	}
	SimdScalar operator-(const SimdScalar& v) const
	{
#ifdef ENF_XENON
		return __vsubfp(*this, v);
#elif defined(ENF_PS3)
		return vec_sub(vec, v);
#elif defined(ENF_SSE)
		return _mm_sub_ss(vec, v);
#endif
	}
	SimdScalar operator-() const
	{
#ifdef ENF_XENON
		return __vsubfp(__vzero(), *this);
#elif defined(ENF_PS3)
		return (vec_float4)vec_xor( (vec_uint4)vec, vec_splats(0x80000000) );
#elif defined(ENF_SSE)
		return _mm_sub_ss(_mm_setzero_ps(), vec);
#endif
	}
	SimdScalar operator*(const SimdScalar& v) const
	{
#ifdef ENF_XENON
		return __vmulfp(*this, v);
#elif defined(ENF_PS3)
		return vec_madd(vec, v, Zero());
#elif defined(ENF_SSE)
		return _mm_mul_ss(vec, v);
#endif
	}
	SimdScalar operator/(const SimdScalar& v) const
	{
#ifdef ENF_XENON
		__vector4 reciprocal = __vrefp(v);
		__vector4 s1 = __vupkd3d(__vzero(), VPACK_NORMSHORT2);
		__vector4 s0 = __vspltw(s1, 3);
		s0 = __vnmsubfp(v, reciprocal, s0);
		s0 = __vmaddcfp(reciprocal, s0, reciprocal);
		s1 = __vspltw(s1, 3);
		s1 = __vnmsubfp(v, s0, s1);
		__vector4 refine = __vcmpeqfp(s0, s0);
		s0 = __vmaddfp(s0, s1, s0);
		refine = __vsel(reciprocal, s0, refine);
		return __vmulfp(*this, refine);
#elif defined(ENF_PS3)
		// Reciprocal estimate and 1 Newton-Raphson iteration.
		vec_uint4 mask = (vec_uint4)(vec_int4)(-1);
		vec_float4 vnzero = (vec_float4)vec_vslw(mask, mask);
		vec_float4 y0, y0numer;

		y0 = vec_re(v);
		y0numer = vec_madd(vec, y0, vnzero);
		y0 = vec_madd(vec_nmsub(v, y0, (vec_float4)(1.0f) ), y0numer, y0numer);

		return vec_sel(y0numer, y0, vec_cmpeq(y0, y0));
#elif defined(ENF_SSE)
		return _mm_div_ss(vec, v);
#endif
	}
	SimdScalar DivideEst(const SimdScalar& v) const
	{
#ifdef ENF_XENON
		return __vmulfp(*this, __vrefp(v));
#elif defined(ENF_PS3)
		vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
		return vec_madd( vec, vec_re( v ), vzero);
#elif defined(ENF_SSE)
		__m128 tmp = _mm_rcp_ss(v);
		return _mm_mul_ss(vec, tmp);
#endif
	}
	int IsTrue() const
	{
#ifdef ENF_XENON
		return x != 0.0f;
#elif defined(ENF_PS3)
		float el = vec_extract(vec, 0);
		return el != 0.0f;
#elif defined(ENF_SSE)
		__m128 v = _mm_and_ps(vec, _mm_set_ss(1.0f));
		return _mm_comineq_ss(v, _mm_setzero_ps());
#endif
	}
	int IsFalse() const
	{
#ifdef ENF_XENON
		return x == 0.0f;
#elif defined(ENF_PS3)
		float el = vec_extract(vec, 0);
		return el == 0.0f;
#elif defined(ENF_SSE)
		__m128 v = _mm_and_ps(vec, _mm_set_ss(1.0f));
		return _mm_comieq_ss(v, _mm_setzero_ps());
#endif
	}
	int operator==(const float f) const
	{
#ifdef ENF_XENON
		return x == f;
#elif defined(ENF_PS3)
		float el = vec_extract(vec, 0);
		return el == f;
#elif defined(ENF_SSE)
		__m128 v = _mm_set_ss(f);
		return _mm_comieq_ss(vec, v);
#endif
	}
	int operator!=(const float f) const
	{
#ifdef ENF_XENON
		return x != f;
#elif defined(ENF_PS3)
		float el = vec_extract(vec, 0);
		return el != f;
#elif defined(ENF_SSE)
		__m128 v = _mm_set_ss(f);
		return _mm_comineq_ss(vec, v);
#endif
	}
	int operator>(const float f) const
	{
#ifdef ENF_XENON
		return x > f;
#elif defined(ENF_PS3)
		float el = vec_extract(vec, 0);
		return el > f;
#elif defined(ENF_SSE)
		__m128 v = _mm_set_ss(f);
		return _mm_comigt_ss(vec, v);
#endif
	}
	int operator<(const float f) const
	{
#ifdef ENF_XENON
		return x < f;
#elif defined(ENF_PS3)
		float el = vec_extract(vec, 0);
		return el < f;
#elif defined(ENF_SSE)
		__m128 v = _mm_set_ss(f);
		return _mm_comilt_ss(vec, v);
#endif
	}
};

//-----------------------------------------------------------------------------
#ifdef ENF_XENON
__declspec(passinreg) ENF_ALIGN16(class) SimdVector: public __vector4
{
#elif defined(ENF_PS3)
ENF_ALIGN16(class) SimdVector
{
protected:
	vec_float4 vec;
#elif defined(ENF_SSE)
ENF_ALIGN16(class) SimdVector
{
protected:
	__m128 vec;
	ENF_INLINE SimdVector WrapPi() const;

#endif
public:
	SimdVector() {}
	
#ifdef ENF_XENON
	SimdVector(__vector4 v): __vector4(v) {}

	SimdVector(float v): __vector4(__vspltw(__lvlx(&v, 0), 0))
	{
	}
	SimdVector(int v): __vector4(__vspltw(__lvlx(&v, 0), 0))
	{
	}
	SimdVector(const SimdScalar& v): __vector4(__vspltw(v, 0))
	{
	}
#elif defined(ENF_PS3)
	SimdVector(vec_float4 v): vec(v) {}

	SimdVector(float v): vec(vec_vspltw(vec_lvlx(0, &v), 0))
	{
	}
	operator vec_float4() const { return vec; }
	SimdVector(const SimdScalar& v)
	{
		vec = vec_vspltw(v, 0);
	}
#elif defined(ENF_SSE)
	SimdVector(__m128 v): vec(v) {}

	SimdVector(float v)
	{
		vec = _mm_set_ps1(v);
	}
	operator __m128() const { return vec; }
	SimdVector(int v)
	{
		__m128i V = _mm_set_epi32( v, v, v, v );
		vec = reinterpret_cast<__m128 *>(&V)[0];
	}
	SimdVector(const SimdScalar& v)
	{
		vec = _mm_shuffle_ps( v, v, _MM_SHUFFLE(0, 0, 0, 0) );
	}
#endif

	SimdVector(float x, float y, float z, float w)
	{
#ifdef ENF_SSE
		vec = _mm_set_ps( w, z, y, x );
#else
		SimdVectorF32 temp = {x, y, z, w};
		*this = temp.v;
#endif
	}

	SimdVector(int x, int y, int z, int w)
	{
#ifdef ENF_XENON
		SimdVectorI32 temp = {x, y, z, w};
		*this = temp.v;
#elif defined(ENF_PS3)
		SimdVectorI32 temp = {x, y, z, w};
		*this = (vec_float4)temp.v;
#elif defined(ENF_SSE)
		__m128i V = _mm_set_epi32( w, z, y, x );
		vec = reinterpret_cast<__m128 *>(&V)[0];
#endif
	}

	ENF_INLINE SimdVector RotateLeft(const int shift) const
	{
		enf_assert(shift >= 1 && shift <= 3);
		SimdVector ret;

		switch(shift)
		{
	#if defined(ENF_SSE)
		case 1:
			ret = _mm_shuffle_ps(*this, *this, _MM_SHUFFLE(0, 3, 2, 1));
			break;
		case 2:
			ret = _mm_shuffle_ps(*this, *this, _MM_SHUFFLE(1, 0, 3, 2));
			break;
		case 3:
			ret = _mm_shuffle_ps(*this, *this, _MM_SHUFFLE(2, 1, 0, 3));
			break;

	#elif defined(ENF_XENON)
		case 1:
			ret = __vrlimi(ret, *this, 0xf, 1);
			break;
		case 2:
			ret = __vrlimi(ret, *this, 0xf, 2);
			break;
		case 3:
			ret = __vrlimi(ret, *this, 0xf, 3);
			break;
	#elif defined(ENF_PS3)
		case 1:
			ret = vec_sld(*this, *this, 4);
			break;
		case 2:
			ret = vec_sld(*this, *this, 8);
			break;
		case 3:
			ret = vec_sld(*this, *this, 12);
			break;
	#endif
		}

		return ret;
	}
	ENF_INLINE SimdVector MergeHigh(const SimdVector& v2) const
	{
	#if defined(ENF_SSE)
		return _mm_unpacklo_ps(*this, v2);
	#elif defined(ENF_XENON)
		return __vmrghw(*this, v2);
	#elif defined(ENF_PS3)
		return vec_vmrghw(*this, v2);
	#endif
	}

	ENF_INLINE SimdVector MergeLow(const SimdVector& v2) const
	{
	#if defined(ENF_SSE)
		return _mm_unpackhi_ps(*this, v2);
	#elif defined(ENF_XENON)
		return __vmrglw(*this, v2);
	#elif defined(ENF_PS3)
		return vec_vmrglw(*this, v2);
	#endif
	}


	ENF_INLINE static SimdVector Zero()
	{
#ifdef ENF_XENON
		return __vzero();
#elif defined(ENF_PS3)
		return vec_float4(0.0f);
#elif defined(ENF_SSE)
		return SimdVector(_mm_setzero_ps());
#endif
	}
	ENF_INLINE static SimdVector One()
	{
#ifdef ENF_XENON
		return __vupkd3d(__vzero(), VPACK_D3DCOLOR);
#elif defined(ENF_PS3)
		return vec_float4(1.0f);
#elif defined(ENF_SSE)
		return g_SimdOne.v;
#endif
	}
	ENF_INLINE static SimdVector LoadUShort4(const ushort* ptr)
	{
		//enf_assert(ALIGNED4_PTR(ptr));
#ifdef ENF_XENON
		__vector4 VR = __lvrx(ptr, 8);
		__vector4 VL = __lvlx(ptr, 0);
		VR = __vsldoi(VR, VR, 2 << 2);
		__vector4 V = __vor(VL, VR);
		V = __vmrghh(__vzero(), V);
		return __vcfux(V, 0);
#elif defined(ENF_PS3)
		enf_assert(ptr);
		vec_ushort8 VR = vec_lvrx(8, ptr);
		vec_ushort8 VL = vec_lvlx(0, ptr);
		VR = vec_vsldoi(VR, VR, 2 << 2);
		vec_ushort8 V = vec_vor(VL, VR);
		V = vec_vmrghh(vec_ushort8(0), V);
		return vec_vcfux((vec_uint4)V, 0);
#elif defined(ENF_SSE)
		return _mm_loadu_ps((float*)ptr);
#endif
	}

	ENF_INLINE static SimdVector LoadFloat2A16(const float* ptr)
	{
		enf_assert(ALIGNED16_PTR(ptr));
#ifdef ENF_XENON
		return __lvx(ptr, 0);
#elif defined(ENF_PS3)
		return vec_lvx(0, ptr); 
#elif defined(ENF_SSE)
		//TODO: it loads 4 floats!
		return _mm_load_ps(ptr);
#endif
	}
	ENF_INLINE static SimdVector LoadFloat3A16(const float* ptr)
	{
		enf_assert(ALIGNED16_PTR(ptr));
#ifdef ENF_XENON
		__vector4 VR = __lvrx(ptr, 16);
		__vector4 VL = __lvlx(ptr, 0);
		return __vor(VL, VR);
#elif defined(ENF_PS3)
		return vec_lvx(0, ptr);
#elif defined(ENF_SSE)
		return _mm_load_ps(ptr);
#endif
	}
	ENF_INLINE static SimdVector LoadFloat3(const float* ptr)
	{
		enf_assert(ALIGNED4_PTR(ptr));
#ifdef ENF_XENON
		__vector4 VR = __lvrx(ptr, 12);
		__vector4 VL = __lvlx(ptr, 0);
		VR = __vsldoi(VR, VR, 1 << 2);
		__vector4 V = __vor(VL, VR);
		return V;
#elif defined(ENF_PS3)
		vec_float4 VR = vec_lvrx(12, ptr);
		vec_float4 VL = vec_lvlx(0, ptr);
		VR = vec_vsldoi(VR, VR, 1 << 2);
		vec_float4 V = vec_vor(VL, VR);
		return V;
#elif defined(ENF_SSE)
		return _mm_loadu_ps(ptr);
#endif
	}
	ENF_INLINE static SimdVector LoadFloat4A16(const float* ptr)
	{
		enf_assert(ALIGNED16_PTR(ptr));
#ifdef ENF_XENON
		return __lvx(ptr, 0);
#elif defined(ENF_PS3)
		return vec_lvx(0, ptr);
#elif defined(ENF_SSE)
		return _mm_load_ps(ptr);
#endif
	}
	ENF_INLINE static SimdVector LoadFloat4(const float* ptr)
	{
		enf_assert(ALIGNED4_PTR(ptr));
#ifdef ENF_XENON
		SimdVector a = __lvlx(ptr, 0);		
		return __vor(__lvrx(ptr, 16), a);
#elif defined(ENF_PS3)
		SimdVector a = vec_lvlx(0, ptr);		
		return vec_vor(vec_lvrx(16, ptr), a);
#elif defined(ENF_SSE)
		return _mm_loadu_ps(ptr);
#endif
	}
	ENF_INLINE static SimdVector LoadFloat4A16Offset(const float* ptr, const int off)
	{
		enf_assert(ALIGNED16_PTR(ptr));
#ifdef ENF_XENON
		return __lvx(ptr, off);
#elif defined(ENF_PS3)
		return vec_lvx(off, ptr);
#elif defined(ENF_SSE)
		return _mm_load_ps(ptr + (off >> 2));
#endif
	}
	ENF_INLINE static SimdVector LoadInt4A16(const int* ptr)
	{
		enf_assert(ALIGNED16_PTR(ptr));
#ifdef ENF_XENON
		return __lvx(ptr, 0);
#elif defined(ENF_PS3)
		enf_assert(ALIGNED16_PTR(ptr));
		vec_int4 ivec = vec_lvx(0, ptr);
		return vec_ctf(ivec, 0);
#elif defined(ENF_SSE)
		return _mm_load_ps((float*)ptr);
#endif
	}
	ENF_INLINE static SimdVector LoadFloat(const float* ptr)
	{
		enf_assert(ALIGNED4_PTR(ptr));
#ifdef ENF_XENON
		return __vspltw(__lvlx(ptr, 0), 0);
#elif defined(ENF_PS3)
		return vec_vspltw(vec_lvlx(0, ptr), 0);
#elif defined(ENF_SSE)
		return _mm_load_ps1(ptr);
#endif
	}
	ENF_INLINE static SimdVector LoadFloat2(const float* ptr)
	{
		enf_assert(ALIGNED4_PTR(ptr));
#ifdef ENF_XENON
		__vector4 VR = __lvrx(ptr, 8);
		__vector4 VL = __lvlx(ptr, 0);
		VR = __vsldoi(VR, VR, 2 << 2);
		return __vor(VL, VR);
#elif defined(ENF_PS3)
		vec_float4 VR = vec_lvrx(8, ptr);
		vec_float4 VL = vec_lvlx(0, ptr);
		VR = vec_vsldoi(VR, VR, 2 << 2);
		return vec_vor(VL, VR);
#elif defined(ENF_SSE)
		//TODO: reads 4 floats!
		return _mm_loadu_ps((float*)ptr);
#endif
	}

	void StoreFloat4A16(float* ptr) const
	{
		enf_assert(ALIGNED16_PTR(ptr));
#ifdef ENF_XENON
		__stvx(*this, ptr, 0);
#elif defined(ENF_PS3)
		vec_stvx(vec, 0, ptr);
#elif defined(ENF_SSE)
		_mm_store_ps(ptr, vec);
#endif
	}	
	void StreamFloat4A16(float* ptr) const
	{
		enf_assert(ALIGNED16_PTR(ptr));
#ifdef ENF_XENON
		//TODO: we have stream store?
		//__stvx(*this, ptr, 0);
		__vector4 S;
		__vector4 Permute;

		enf_assert(((uint)ptr & 3) == 0);

		Permute = __lvsr(ptr, 0);                 // Compute the permute control vector for shift right
		S = __vperm(*this, *this, Permute);                        // Right rotate the vector

		__stvewx(S, ptr, 0);
		__stvewx(S, ptr, 4);
		__stvewx(S, ptr, 8);
		__stvewx(S, ptr, 12);
#elif defined(ENF_PS3)
		//vec_stvx(vec, 0, ptr);
		vec_float4 S;
		vec_uchar16 Permute;

		enf_assert(((uint)ptr & 3) == 0);

		Permute = vec_lvsr(0, ptr);                 // Compute the permute control vector for shift right
		S = vec_perm(vec, vec, Permute);                        // Right rotate the vector

		vec_stvewx(S, 0, ptr);
		vec_stvewx(S, 4, ptr);
		vec_stvewx(S, 8, ptr);
		vec_stvewx(S, 12, ptr);
#elif defined(ENF_SSE)
		_mm_stream_ps(ptr, vec);
#endif
	}	
	void StoreFloat4A16Offset(float* ptr, const int offset) const
	{
		enf_assert(ALIGNED16_PTR(ptr));
#ifdef ENF_XENON
		__stvx(*this, ptr, offset);
#elif defined(ENF_PS3)
		vec_stvx(vec, offset, ptr);
#elif defined(ENF_SSE)
		_mm_store_ps(ptr + (offset >> 2), vec);
#endif
	}	
	void StreamFloat4A16Offset(float* ptr, const int offset) const
	{
		enf_assert(ALIGNED16_PTR(ptr));
#ifdef ENF_XENON
		//TODO: we have stream store?
		//__stvx(*this, ptr, offset);
		__vector4 S;
		__vector4 Permute;

		enf_assert(((uint)ptr & 3) == 0);

		Permute = __lvsr(ptr, offset);                 // Compute the permute control vector for shift right
		S = __vperm(*this, *this, Permute);                        // Right rotate the vector

		__stvewx(S, ptr, offset);
		__stvewx(S, ptr, offset + 4);
		__stvewx(S, ptr, offset + 8);
		__stvewx(S, ptr, offset + 12);
#elif defined(ENF_PS3)
		//vec_stvx(vec, offset, ptr);
		vec_float4 S;
		vec_uchar16 Permute;

		enf_assert(((uint)ptr & 3) == 0);

		Permute = vec_lvsr(offset, ptr);                 // Compute the permute control vector for shift right
		S = vec_perm(vec, vec, Permute);                        // Right rotate the vector

		vec_stvewx(S, offset, ptr);
		vec_stvewx(S, offset + 4, ptr);
		vec_stvewx(S, offset + 8, ptr);
		vec_stvewx(S, offset + 12, ptr);
#elif defined(ENF_SSE)
		_mm_stream_ps(ptr + (offset >> 2), vec);
#endif
	}	
	SimdScalar X() const
	{
#ifdef ENF_XENON
		return __vspltw(*this, 0);
#elif defined(ENF_PS3)
		return vec_vspltw(vec, 0);
#elif defined(ENF_SSE)
		return _mm_shuffle_ps( vec, vec, _MM_SHUFFLE(0, 0, 0, 0) );
#endif
	}
	SimdScalar Y() const
	{
#ifdef ENF_XENON
		return __vspltw(*this, 1);
#elif defined(ENF_PS3)
		return vec_vspltw(vec, 1);
#elif defined(ENF_SSE)
		return _mm_shuffle_ps( vec, vec, _MM_SHUFFLE(1, 1, 1, 1) );
#endif
	}
	SimdScalar Z() const
	{
#ifdef ENF_XENON
		return __vspltw(*this, 2);
#elif defined(ENF_PS3)
		return vec_vspltw(vec, 2);
#elif defined(ENF_SSE)
		return _mm_shuffle_ps( vec, vec, _MM_SHUFFLE(2, 2, 2, 2) );
#endif
	}
	SimdScalar W() const
	{
#ifdef ENF_XENON
		return __vspltw(*this, 3);
#elif defined(ENF_PS3)
		return vec_vspltw(vec, 3);
#elif defined(ENF_SSE)
		return _mm_shuffle_ps( vec, vec, _MM_SHUFFLE(3, 3, 3, 3) );
#endif
	}

	SimdVector SplatX() const
	{
#ifdef ENF_XENON
		return __vspltw(*this, 0);
#elif defined(ENF_PS3)
		return vec_vspltw(vec, 0);
#elif defined(ENF_SSE)
		return _mm_shuffle_ps( vec, vec, _MM_SHUFFLE(0, 0, 0, 0) );
#endif
	}
	SimdVector SplatY() const
	{
#ifdef ENF_XENON
		return __vspltw(*this, 1);
#elif defined(ENF_PS3)
		return vec_vspltw(vec, 1);
#elif defined(ENF_SSE)
		return _mm_shuffle_ps( vec, vec, _MM_SHUFFLE(1, 1, 1, 1) );
#endif
	}
	SimdVector SplatZ() const
	{
#ifdef ENF_XENON
		return __vspltw(*this, 2);
#elif defined(ENF_PS3)
		return vec_vspltw(vec, 2);
#elif defined(ENF_SSE)
		return _mm_shuffle_ps( vec, vec, _MM_SHUFFLE(2, 2, 2, 2) );
#endif
	}
	SimdVector SplatW() const
	{
#ifdef ENF_XENON
		return __vspltw(*this, 3);
#elif defined(ENF_PS3)
		return vec_vspltw(vec, 3);
#elif defined(ENF_SSE)
		return _mm_shuffle_ps( vec, vec, _MM_SHUFFLE(3, 3, 3, 3) );
#endif
	}

	SimdVector operator+(const SimdVector& v) const
	{
#ifdef ENF_XENON
		return __vaddfp(*this, v);
#elif defined(ENF_PS3)
		return vec_add(vec, v);
#elif defined(ENF_SSE)
		return _mm_add_ps(vec, v);
#endif
	}
	SimdVector operator-(const SimdVector& v) const
	{
#ifdef ENF_XENON
		return __vsubfp(*this, v);
#elif defined(ENF_PS3)
		return vec_sub(vec, v);
#elif defined(ENF_SSE)
		return _mm_sub_ps(vec, v);
#endif
	}
	SimdVector operator-() const
	{
#ifdef ENF_XENON
		return __vsubfp(__vzero(), *this);
#elif defined(ENF_PS3)
		return (vec_float4)vec_xor( (vec_uint4)vec, vec_splats(0x80000000) );
#elif defined(ENF_SSE)
		return _mm_sub_ps(_mm_setzero_ps(), vec);
#endif
	}
	SimdVector operator*(const SimdVector& v) const
	{
#ifdef ENF_XENON
		return __vmulfp(*this, v);
#elif defined(ENF_PS3)
		return vec_madd(vec, v, Zero());
#elif defined(ENF_SSE)
		return _mm_mul_ps(vec, v);
#endif
	}
	SimdVector operator/(const SimdVector& v) const
	{
#ifdef ENF_XENON
		__vector4 reciprocal = __vrefp(v);
		__vector4 s1 = __vupkd3d(__vzero(), VPACK_NORMSHORT2);
		__vector4 s0 = __vspltw(s1, 3);
		s0 = __vnmsubfp(v, reciprocal, s0);
		s0 = __vmaddcfp(reciprocal, s0, reciprocal);
		s1 = __vspltw(s1, 3);
		s1 = __vnmsubfp(v, s0, s1);
		__vector4 refine = __vcmpeqfp(s0, s0);
		s0 = __vmaddfp(s0, s1, s0);
		refine = __vsel(reciprocal, s0, refine);
		return __vmulfp(*this, refine);
#elif defined(ENF_PS3)
		// Reciprocal estimate and 1 Newton-Raphson iteration.
		vec_uint4 mask = (vec_uint4)(vec_int4)(-1);
		vec_float4 vnzero = (vec_float4)vec_vslw(mask, mask);
		vec_float4 y0, y0numer;

		y0 = vec_re(v);
		y0numer = vec_madd(vec, y0, vnzero);
		y0 = vec_madd(vec_nmsub(v, y0, (vec_float4)(1.0f) ), y0numer, y0numer);

		return vec_sel(y0numer, y0, vec_cmpeq(y0, y0));
#elif defined(ENF_SSE)
		return _mm_div_ps(vec, v);
#endif
	}
	SimdVector DivideEst(const SimdVector& v) const
	{
#ifdef ENF_XENON
		return __vmulfp(*this, __vrefp(v));
#elif defined(ENF_PS3)
		vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
		return vec_madd( vec, vec_re( v ), vzero);
#elif defined(ENF_SSE)
		__m128 tmp = _mm_rcp_ps(v);
		return _mm_mul_ps(vec, tmp);
#endif
	}
	SimdScalar Dot3(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vmsum3fp(*this, other);
#elif defined(ENF_PS3)
		vec_float4 result;
		result = vec_madd( vec, other, (vec_float4)(0.0f) );
		result = vec_madd( vec_sld( vec, vec, 4 ), vec_sld( other, other, 4 ), result );
		return vec_madd( vec_sld( vec, vec, 8 ), vec_sld( other, other, 8 ), result );
#elif defined(ENF_SSE)
		__m128 rdot = _mm_mul_ps(vec, other);
		__m128 rdot2 = _mm_movehl_ps(rdot, rdot);
		rdot2 = _mm_add_ps(rdot2, rdot);
		rdot = _mm_shuffle_ps(rdot, rdot , _MM_SHUFFLE(0, 0, 0, 1));
		return _mm_add_ps(rdot, rdot2);
#endif
	}

	SimdScalar SquaredLength3() const
	{
		return Dot3(*this);
	}

	SimdScalar Length3() const
	{
#ifdef ENF_XENON
		__vector4 H = __vspltisw(1);
		__vector4 D = __vmsum3fp(*this, *this);
		H = __vcfsx(H, 1);
		__vector4 Rsq = __vrsqrtefp(D);
		__vector4 RT = __vmulfp(D, H);
		__vector4 Rcp = __vmulfp(Rsq, Rsq);
		H = __vnmsubfp(RT, Rcp, H);
		Rsq = __vmaddfp(Rsq, H, Rsq);
		__vector4 Result = __vcmpeqfp(D, __vzero());
		__vector4 Length = __vmulfp(D, Rsq);
		return __vsel(Length, D, Result);
#elif defined(ENF_PS3)
		vec_float4 dotres;
		dotres = vec_madd( vec, vec, (vec_float4)(0.0f) );
		dotres = vec_madd( vec_sld( vec, vec, 4 ), vec_sld( vec, vec, 4 ), dotres );
		dotres = vec_madd( vec_sld( vec, vec, 8 ), vec_sld( vec, vec, 8 ), dotres );
		return sqrtf4(dotres);
#elif defined(ENF_SSE)
		__m128 dot = Dot3(*this);
		return _mm_sqrt_ss(dot);
#endif
	}

	SimdScalar QLength3() const
	{
#ifdef ENF_XENON
		return Length3();
#elif defined(ENF_PS3)
		vec_float4 dotres;
		dotres = vec_madd( vec, vec, (vec_float4)(0.0f) );
		dotres = vec_madd( vec_sld( vec, vec, 4 ), vec_sld( vec, vec, 4 ), dotres );
		dotres = vec_madd( vec_sld( vec, vec, 8 ), vec_sld( vec, vec, 8 ), dotres );
		return sqrtf4fast(dotres);
#elif defined(ENF_SSE)
		__m128 dot = Dot3(*this);
		return _mm_sqrt_ss(dot);
#endif
	}

	SimdScalar QLength3Inv() const
	{
#ifdef ENF_XENON
		__vector4 dot = __vmsum3fp(*this, *this);
		return __vrsqrtefp(dot);
#elif defined(ENF_PS3)
		vec_float4 dotres;
		dotres = vec_madd( vec, vec, (vec_float4)(0.0f) );
		dotres = vec_madd( vec_sld( vec, vec, 4 ), vec_sld( vec, vec, 4 ), dotres );
		dotres = vec_madd( vec_sld( vec, vec, 8 ), vec_sld( vec, vec, 8 ), dotres );
		return vec_rsqrte(dotres);
#elif defined(ENF_SSE)
		__m128 dot = Dot3(*this);
		return _mm_rsqrt_ss(dot);
#endif
	}

	SimdVector QNormalize3() const
	{
		SimdVector inv = QLength3Inv();
		return *this*inv.SplatX();
	}

	SimdVector MultiplyAdd(const SimdVector& mul, const SimdVector& add) const
	{
#ifdef ENF_XENON
		return __vmaddfp(*this, mul, add);
#elif defined(ENF_PS3)
		return vec_madd(vec, mul, add);
#elif defined(ENF_SSE)
		__m128 vResult = _mm_mul_ps( vec, mul );
		return _mm_add_ps(vResult, add );
#endif
	}

	SimdVector Lerp(const SimdVector& other, const SimdVector& lerp) const
	{
		SimdVector invLerp = One() - lerp;
		return MultiplyAdd(invLerp, other * lerp);
	}

	SimdVector Min(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vminfp(*this, other);
#elif defined(ENF_PS3)
		return vec_min(vec, other);
#elif defined(ENF_SSE)
		return _mm_min_ps(vec, other);
#endif
	}
	SimdVector Max(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vmaxfp(*this, other);
#elif defined(ENF_PS3)
		return vec_max(vec, other);
#elif defined(ENF_SSE)
		return _mm_max_ps(vec, other);
#endif
	}

	SimdVector Select(const SimdVector& other, const SimdVector& mask) const
	{
#ifdef ENF_XENON
		return __vsel(*this, other, mask);
#elif defined(ENF_PS3)
		vec_bint4 bitMask = vec_cmpeq(mask, vec_float4(0.0f));
		return vec_sel(vec, other, bitMask);
#elif defined(ENF_SSE)
		__m128 vTemp1 = _mm_andnot_ps(mask, vec);
		__m128 vTemp2 = _mm_and_ps(other, mask);
		return _mm_or_ps(vTemp1, vTemp2);
#endif
	}

	SimdVector Sqrt() const
	{
#ifdef ENF_XENON
		__vector4 vResult = __vrsqrtefp(*this);

		// Now refine the estimate with Newton’s method
		__vector4 vHalf = {0.5f, 0.5f, 0.5f, 0.5f};
		__vector4 vHalfV = __vmulfp( *this, vHalf );
		__vector4 vRecip = __vmulfp( vResult, vResult );
		__vector4 vScale = __vnmsubfp( vHalfV, vRecip, vHalf );
		vResult = __vmaddfp( vResult, vScale, vResult );

		//vResult is x^(-0.5) -> x*x^(-0.5) = x^(1 - 0.5) = x^(0.5)
		vResult = __vmulfp( vResult, *this);

		return vResult;
#elif defined(ENF_PS3)
		vec_float4 vResult = vec_rsqrte(vec);

		// Now refine the estimate with Newton’s method
		vec_float4 vHalf = {0.5f, 0.5f, 0.5f, 0.5f};
		vec_float4 vZero = Zero();
		vec_float4 vHalfV = vec_madd(vec, vHalf, vZero);
		vec_float4 vRecip = vec_madd(vResult, vResult, vZero);
		vec_float4 vScale = vec_nmsub( vHalfV, vRecip, vHalf );
		vResult = vec_madd( vResult, vScale, vResult );

		//vResult is x^(-0.5) -> x*x^(-0.5) = x^(1 - 0.5) = x^(0.5)
		vResult = vec_madd(vResult, vec, vZero);

		return vResult;
#elif defined(ENF_SSE)
		return _mm_sqrt_ps(vec);
#endif
	}

	SimdVector operator<(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vcmpgtfp(other, *this);
#elif defined(ENF_PS3)
		return (vec_float4)vec_cmpgt(other.vec, vec);
#elif defined(ENF_SSE)
		return _mm_cmplt_ps(vec, other);
#endif
	}
	SimdVector operator>(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vcmpgtfp(*this, other);
#elif defined(ENF_PS3)
		return (vec_float4)vec_cmpgt(vec, other.vec);
#elif defined(ENF_SSE)
		return _mm_cmpgt_ps(vec, other);		
#endif
	}
	SimdVector operator<=(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vcmpgefp(other, *this);
#elif defined(ENF_PS3)
		return (vec_float4)vec_cmpge(other.vec, vec);
#elif defined(ENF_SSE)
		return _mm_cmple_ps(vec, other);		
#endif
	}
	SimdVector operator>=(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vcmpgefp(*this, other);
#elif defined(ENF_PS3)
		return (vec_float4)vec_cmpge(vec, other.vec);
#elif defined(ENF_SSE)
		return _mm_cmpge_ps(vec, other);		
#endif
	}
	SimdVector operator==(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vcmpeqfp(*this, other);
#elif defined(ENF_PS3)
		return (vec_float4)vec_cmpeq(vec, other.vec);
#elif defined(ENF_SSE)
		return _mm_cmpeq_ps(vec, other);		
#endif
	}
	SimdVector operator!=(const SimdVector& other) const
	{
#ifdef ENF_XENON
		SimdVector eq = __vcmpeqfp(*this, other);
		return __vnor(eq, Zero());
#elif defined(ENF_PS3)
		vec_bint4 eq = vec_cmpeq(vec, other.vec);
		return (vec_float4)vec_nor(eq, vec_bint4(0));
#elif defined(ENF_SSE)
		return _mm_cmpneq_ps(vec, other);		
#endif
	}
	SimdScalar HasEQ(const float f) const
	{
#ifdef ENF_XENON
		SimdVector tmp = f;
		SimdVector eq = __vcmpeqfp(*this, tmp);		
		SimdVector req = __vrlimi(req, eq, 0xf, 2);

		eq = __vor(req, eq);					
		req = __vrlimi(req, eq, 0xf, 1);
		eq = __vor(req, eq);	

		return SimdVector(eq.x);
#elif defined(ENF_PS3)
		vec_float4 tmp = vec_float4(f);
		vec_float4 eq = (vec_float4)vec_cmpeq(vec, tmp);		
		vec_float4 req = vec_sld( eq, eq, 8);

		eq = vec_or(req, eq);					
		req = vec_sld( eq, eq, 4);
		eq = vec_or(req, eq);	

		return vec_float4(vec_extract(eq, 0));
#elif defined(ENF_SSE)
		__m128 v = _mm_set_ps1(f);
		__m128 eq = _mm_cmpeq_ps(vec, v);		
		__m128 req = _mm_movelh_ps(eq, eq);	
		eq = _mm_or_ps(req, eq);					
		req = _mm_shuffle_ps(eq, eq , _MM_SHUFFLE(1, 1, 1, 1));
		eq = _mm_or_ps(req, eq);	

		req = _mm_shuffle_ps(eq, eq , _MM_SHUFFLE(0, 0, 0, 0));
		return req;
#endif
	}
	SimdScalar HasNEQ(const float f) const
	{
#ifdef ENF_XENON
		SimdVector tmp = f;
		SimdVector toInv = __vcmpeqfp(*this, tmp);
		SimdVector neq = __vnor(toInv, Zero());

		SimdVector rneq = __vrlimi(rneq, neq, 0xf, 2);
		neq = __vor(rneq, neq);

		rneq = __vrlimi(rneq, neq, 0xf, 1);
		neq = __vor(rneq, neq);

		return SimdVector(neq.x);
#elif defined(ENF_PS3)
		vec_float4 tmp = vec_float4(f);
		vec_bint4 toInv = vec_cmpeq(vec, tmp);
		vec_float4 eq = (vec_float4)vec_nor(toInv, vec_bint4(0));

		vec_float4 req = vec_sld( eq, eq, 8);

		eq = vec_or(req, eq);					
		req = vec_sld( eq, eq, 4);
		eq = vec_or(req, eq);	

		return vec_float4(vec_extract(eq, 0));
#elif defined(ENF_SSE)
		__m128 v = _mm_set_ps1(f);
		__m128 neq = _mm_cmpneq_ps(vec, v);

		__m128 rneq = _mm_movelh_ps(neq, neq);	
		neq = _mm_or_ps(rneq, neq);				
		rneq = _mm_shuffle_ps(neq, neq , _MM_SHUFFLE(1, 1, 1, 1));
		neq = _mm_or_ps(rneq, neq);

		rneq = _mm_shuffle_ps(neq, neq , _MM_SHUFFLE(0, 0, 0, 0));
		return rneq;
#endif
	}
	SimdScalar HasGT(const float f) const 
	{
#ifdef ENF_XENON
		SimdVector tmp = f;
		SimdVector g = __vcmpgtfp(*this, tmp);		
		SimdVector rg = __vrlimi(rg, g, 0xf, 2);
		g = __vor(rg, g);	

		rg = __vrlimi(rg, g, 0xf, 1);
		g = __vor(rg, g);

		return SimdVector(g.x);
#elif defined(ENF_PS3)
		vec_float4 tmp = vec_float4(f);
		vec_float4 eq = (vec_float4)vec_cmpgt(vec, tmp);		
		vec_float4 req = vec_sld( eq, eq, 8);

		eq = vec_or(req, eq);					
		req = vec_sld( eq, eq, 4);
		eq = vec_or(req, eq);	

		return vec_float4(vec_extract(eq, 0));
#elif defined(ENF_SSE)
		__m128 v = _mm_set_ps1(f);
		__m128 g = _mm_cmpgt_ps(vec, v);		
		__m128 rg = _mm_movelh_ps(g, g);	
		g = _mm_or_ps(rg, g);					
		rg = _mm_shuffle_ps(g, g , _MM_SHUFFLE(1, 1, 1, 1));
		g = _mm_or_ps(rg, g);

		rg = _mm_shuffle_ps(g, g , _MM_SHUFFLE(0, 0, 0, 0));
		return g;
#endif
	}
	SimdScalar HasLT(const float f) const 
	{
#ifdef ENF_XENON
		SimdVector tmp = f;
		SimdVector g = __vcmpgtfp(tmp, *this);		
		SimdVector rg = __vrlimi(rg, g, 0xf, 2);
		g = __vor(rg, g);	

		rg = __vrlimi(rg, g, 0xf, 1);
		g = __vor(rg, g);

		return SimdVector(g.x);
#elif defined(ENF_PS3)
		vec_float4 tmp = vec_float4(f);
		vec_float4 eq = (vec_float4)vec_cmpgt(tmp, vec);		
		vec_float4 req = vec_sld( eq, eq, 8);


		eq = vec_or(req, eq);					
		req = vec_sld( eq, eq, 4);
		eq = vec_or(req, eq);	

		return vec_float4(vec_extract(eq, 0));
#elif defined(ENF_SSE)
		__m128 v = _mm_set_ps1(f);
		__m128 g = _mm_cmplt_ps(vec, v);		
		__m128 rg = _mm_movelh_ps(g, g);	
		g = _mm_or_ps(rg, g);					
		rg = _mm_shuffle_ps(g, g , _MM_SHUFFLE(1, 1, 1, 1));
		g = _mm_or_ps(rg, g);

		rg = _mm_shuffle_ps(g, g , _MM_SHUFFLE(0, 0, 0, 0));
		return rg;
#endif
	}
	SimdScalar HasGE(const float f) const 
	{
#ifdef ENF_XENON
		SimdVector tmp = f;
		SimdVector g = __vcmpgefp(*this, tmp);		
		SimdVector rg = __vrlimi(rg, g, 0xf, 2);
		g = __vor(rg, g);	

		rg = __vrlimi(rg, g, 0xf, 1);
		g = __vor(rg, g);

		return SimdVector(g.x);
#elif defined(ENF_PS3)
		vec_float4 tmp = vec_float4(f);
		vec_float4 eq = (vec_float4)vec_cmpge(vec, tmp);		
		vec_float4 req = vec_sld( eq, eq, 8);


		eq = vec_or(req, eq);					
		req = vec_sld( eq, eq, 4);
		eq = vec_or(req, eq);	

		return vec_float4(vec_extract(eq, 0));
#elif defined(ENF_SSE)
		__m128 v = _mm_set_ps1(f);
		__m128 g = _mm_cmpge_ps(vec, v);		
		__m128 rg = _mm_movelh_ps(g, g);	
		g = _mm_or_ps(rg, g);					
		rg = _mm_shuffle_ps(g, g , _MM_SHUFFLE(1, 1, 1, 1));
		g = _mm_or_ps(rg, g);

		rg = _mm_shuffle_ps(g, g , _MM_SHUFFLE(0, 0, 0, 0));
		return rg;
#endif
	}
	SimdScalar HasLE(const float f) const 
	{
#ifdef ENF_XENON
		SimdVector tmp = f;
		SimdVector g = __vcmpgefp(tmp, *this);		
		SimdVector rg = __vrlimi(rg, g, 0xf, 2);
		g = __vor(rg, g);	

		rg = __vrlimi(rg, g, 0xf, 1);
		g = __vor(rg, g);

		return SimdVector(g.x);
#elif defined(ENF_PS3)
		vec_float4 tmp = vec_float4(f);
		vec_float4 eq = (vec_float4)vec_cmpge(tmp, vec);		
		vec_float4 req = vec_sld( eq, eq, 8);

		eq = vec_or(req, eq);					
		req = vec_sld( eq, eq, 4);
		eq = vec_or(req, eq);	

		return vec_float4(vec_extract(eq, 0));
#elif defined(ENF_SSE)
		__m128 v = _mm_set_ps1(f);
		__m128 g = _mm_cmple_ps(vec, v);		
		__m128 rg = _mm_movelh_ps(g, g);	
		g = _mm_or_ps(rg, g);					
		rg = _mm_shuffle_ps(g, g , _MM_SHUFFLE(1, 1, 1, 1));
		g = _mm_or_ps(rg, g);

		rg = _mm_shuffle_ps(g, g , _MM_SHUFFLE(0, 0, 0, 0));
		return rg;
#endif
	}
	SimdScalar Min() const
	{
#ifdef ENF_XENON
		//min(min(ac), min(bd))
		__vector4 toCompare = __vrlimi(toCompare, *this, 0xf, 2);
		__vector4 res = __vminfp(*this, toCompare);

		toCompare = __vrlimi(toCompare, res, 0xf, 1);
		toCompare = __vminfp(res, toCompare);

		return __vspltw(toCompare, 0);
#elif defined(ENF_PS3)
		//min(min(ac), min(bd))
		vec_float4 toCompare = vec_sld( vec, vec, 8);
		vec_float4 res = vec_min(vec, toCompare);

		toCompare = vec_sld( res, res, 4);
		toCompare = vec_min(res, toCompare);

		res = vec_float4(vec_extract(toCompare, 0));
		return res;
#elif defined(ENF_SSE)
		//min(min(ac), min(bd))
		//res = min(abcd, cdab) - swap halfvec
		__m128 toCompare = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 0, 3, 2));
		__m128 res = _mm_min_ps(vec, toCompare);
		//res = min(res, res(badc)) - shift
		toCompare = _mm_shuffle_ps(res, res, _MM_SHUFFLE(2, 3, 0, 1));
		return _mm_min_ps(res, toCompare);
#endif
	}
	SimdScalar Max() const
	{
#ifdef ENF_XENON
		//max(max(ac), max(bd))
		__vector4 toCompare = __vrlimi(toCompare, *this, 0xf, 2);
		__vector4 res = __vmaxfp(*this, toCompare);

		toCompare = __vrlimi(toCompare, res, 0xf, 1);
		toCompare = __vmaxfp(res, toCompare);

		return __vspltw(toCompare, 0);
#elif defined(ENF_PS3)
		//max(max(ac), max(bd))
		vec_float4 toCompare = vec_sld(vec, vec, 8);
		vec_float4 res = vec_max(vec, toCompare);

		toCompare = vec_sld(res, res, 4);
		toCompare = vec_max(res, toCompare);

		res = vec_float4(vec_extract(toCompare, 0));
		return res;
#elif defined(ENF_SSE)
		//min(min(ac), min(bd))
		//res = min(abcd, cdab) - swap halfvec
		__m128 toCompare = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 0, 3, 2));
		__m128 res = _mm_max_ps(vec, toCompare);
		toCompare = _mm_shuffle_ps(res, res, _MM_SHUFFLE(2, 3, 0, 1));
		return _mm_max_ps(res, toCompare);
#endif
	}
	SimdScalar HasTrue() const
	{
#ifdef ENF_XENON
		//invert and find zero
		SimdVector inv = __vnor(*this, Zero());
		SimdVector neq = __vcmpeqfp(inv, Zero());		
		SimdVector rneq = __vrlimi(rneq, neq, 0xf, 2);
		neq = __vor(rneq, neq);			

		rneq = __vrlimi(rneq, neq, 0xf, 1);
		neq = __vor(rneq, neq);

		return neq.x;
#elif defined(ENF_PS3)
		//vec_bint4 toInv = vec_cmpeq(vec, Zero());
		//vec_float4 eq = (vec_float4)vec_nor(toInv, vec_bint4(0));
		vec_float4 inv = vec_nor(vec, Zero());
		vec_float4 eq = (vec_float4)vec_cmpeq(inv, Zero());	

		vec_float4 req = vec_sld(eq, eq, 8);

		eq = vec_or(req, eq);					
		req = vec_sld(eq, eq, 4);
		eq = vec_or(req, eq);	

		return vec_float4(vec_extract(eq, 0));
#elif defined(ENF_SSE)
		__m128 rneq = _mm_movelh_ps(vec, vec);	
		__m128 neq = _mm_or_ps(rneq, vec);				
		rneq = _mm_shuffle_ps(neq, neq , _MM_SHUFFLE(1, 1, 1, 1));
		neq = _mm_or_ps(rneq, neq);

		rneq = _mm_shuffle_ps(neq, neq , _MM_SHUFFLE(0, 0, 0, 0));
		return rneq;
#endif
	}
	SimdScalar HasFalse() const
	{
#ifdef ENF_XENON
		SimdVector eq = __vcmpeqfp(*this, Zero());	
		SimdVector req = __vrlimi(req, eq, 0xf, 2);
		eq = __vor(req, eq);		

		req = __vrlimi(req, eq, 0xf, 1);
		eq = __vor(req, eq);	

		return eq.x;
#elif defined(ENF_PS3)
		vec_float4 eq = (vec_float4)vec_cmpeq(vec, Zero());		
		vec_float4 req = vec_sld(eq, eq, 8);

		eq = vec_or(req, eq);					
		req = vec_sld(eq, eq, 4);
		eq = vec_or(req, eq);	

		req = vec_float4(vec_extract(eq, 0));
		return req;
#elif defined(ENF_SSE)
		__m128 v = _mm_setzero_ps();
		__m128 eq = _mm_cmpeq_ps(vec, v);		
		__m128 req = _mm_movelh_ps(eq, eq);	
		eq = _mm_or_ps(req, eq);					
		req = _mm_shuffle_ps(eq, eq , _MM_SHUFFLE(1, 1, 1, 1));
		eq = _mm_or_ps(req, eq);	

		req = _mm_shuffle_ps(eq, eq , _MM_SHUFFLE(0, 0, 0, 0));
		return req;
#endif
	}
	SimdVector Inv() const
	{
#ifdef ENF_XENON
		return __vrefp(*this);
#elif defined(ENF_PS3)
		return vec_re(vec);
#elif defined(ENF_SSE)
		return _mm_rcp_ps(vec);
#endif
	}
	SimdVector operator&(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vand(*this, other);
#elif defined(ENF_PS3)
		return vec_and(vec, other);
#elif defined(ENF_SSE)
		return _mm_and_ps(vec, other);
#endif
	}
	SimdVector operator|(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vor(*this, other);
#elif defined(ENF_PS3)
		return vec_or(vec, other);
#elif defined(ENF_SSE)
		return _mm_or_ps(vec, other);
#endif
	}
	SimdVector AndNot(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vandc(*this, other);
#elif defined(ENF_PS3)
		return vec_andc(vec, other);
#elif defined(ENF_SSE)
		return _mm_andnot_ps(other, vec);
#endif
	}
	SimdVector Xor(const SimdVector& other) const
	{
#ifdef ENF_XENON
		return __vxor(*this, other);
#elif defined(ENF_PS3)
		return vec_xor(vec, other);
#elif defined(ENF_SSE)
		return _mm_xor_ps(other, vec);
#endif
	}

#ifdef ENF_XENON
	ENF_INLINE void SinCosf(SimdVector& outSin, SimdVector& outCos) const;
	ENF_INLINE SimdVector Cosf() const;
	ENF_INLINE SimdVector Sinf() const;
#elif defined(ENF_PS3)
	void SinCosf(SimdVector& sinRes, SimdVector& cosRes) const
	{
		sincosf4(vec, &sinRes.vec, &cosRes.vec);
	}
	SimdVector Cosf() const
	{
		return cosf4(vec);
	}
	SimdVector Sinf() const
	{
		return sinf4(vec);
	}
#elif defined(ENF_SSE)
	ENF_INLINE void SinCosf(SimdVector& outSin, SimdVector& outCos) const;
	ENF_INLINE SimdVector Cosf() const;
	ENF_INLINE SimdVector Sinf() const;
#endif

#ifdef ENF_XENON
	//static const SimdVectorI32 vPerm  = { 0x0c080f06, 0x0d0b0103, 0x0e050700, 0x020a0409 };
	static ENF_INLINE SimdVector Rand( SimdVector& seed, SimdVector& perm )
	{
		const SimdVectorI32 vAdder = { 0x592fa04b, 0x9e03756c, 0xf883c2da, 0x14e136b7 };
		const SimdVectorI32 vSwiz0 = { 0x10000a06, 0x10010b0e, 0x10020c08, 0x10030d0f };

		// grab a byte from the middle of the last dword of the seed
		SimdVector temp = __vspltb( seed, 0xd );
		// generate our 4 floats
		SimdVector m = __vperm( seed, SimdVector::One(), vSwiz0.v );
		// scramble the seed
		seed   = __vperm( seed, seed, perm );
		// change the swizzle-pattern based on temp
		perm   = __vxor( perm, temp );
		// 1.0f is 0x3f800000. the vperm above gets the 0x3f right, but the next-highest bit also needs to be set.
		m = __vor( m, SimdVector::One() );
		// add a somewhat random (but unchanging) value to the seed
		seed   = __vadduwm( seed, vAdder.v );

		return m;
	}
#endif
};

//-----------------------------------------------------------------------------
ENF_ALIGN16(class) SimdMatrix43
{
public:
	SimdVector vec[4];

public:
	SimdMatrix43(){};
	ENF_INLINE static SimdMatrix43 LoadFloat4x3(const float* ptr)
	{
		SimdMatrix43 m;
#ifdef ENF_XENON
		m.vec[0] = SimdVector::LoadFloat4(ptr + 0);
		SimdVector V1 = SimdVector::LoadFloat4(ptr + 4);
		SimdVector V2 = SimdVector::LoadFloat4(ptr + 8);

		m.vec[3] = __vupkd3d(m.vec[3], VPACK_NORMSHORT2);

		m.vec[1] = __vsldoi(m.vec[0], V1, 3 << 2);
		m.vec[2] = __vsldoi(V1, V2, 2 << 2);

		m.vec[0] = __vrlimi(m.vec[0], m.vec[3], 0x1, 3);
		m.vec[1] = __vrlimi(m.vec[1], m.vec[3], 0x1, 3);
		m.vec[2] = __vrlimi(m.vec[2], m.vec[3], 0x1, 3);
		m.vec[3] = __vrlimi(m.vec[3], V2, 0xE, 1);
#else
		SimdVector mul(1.f, 1.f, 1.f, 0.f);

		m.vec[0] = mul * SimdVector::LoadFloat3(ptr);
		m.vec[1] = mul * SimdVector::LoadFloat3(&ptr[3]);
		m.vec[2] = mul * SimdVector::LoadFloat3(&ptr[6]);
		m.vec[3] = mul * SimdVector::LoadFloat3(&ptr[9]);

		mul = SimdVector::One() - mul;	//0 0 0 1

		m.vec[3] = m.vec[3] + mul;
#endif
		return m;
	}
	ENF_INLINE static SimdMatrix43 LoadFloat4x3A16(const float* ptr)
	{
		SimdMatrix43 m;
#ifdef ENF_XENON
		m.vec[0] = SimdVector::LoadFloat4A16Offset(ptr, 0);
		SimdVector V1 = SimdVector::LoadFloat4A16Offset(ptr, 16);
		SimdVector V2 = SimdVector::LoadFloat4A16Offset(ptr, 32);

		m.vec[3] = __vupkd3d(m.vec[3], VPACK_NORMSHORT2);

		m.vec[1] = __vsldoi(m.vec[0], V1, 3 << 2);
		m.vec[2] = __vsldoi(V1, V2, 2 << 2);

		m.vec[0] = __vrlimi(m.vec[0], m.vec[3], 0x1, 3);
		m.vec[1] = __vrlimi(m.vec[1], m.vec[3], 0x1, 3);
		m.vec[2] = __vrlimi(m.vec[2], m.vec[3], 0x1, 3);
		m.vec[3] = __vrlimi(m.vec[3], V2, 0xE, 1);
#else
		static const SimdVectorF32 imul = {1.f, 1.f, 1.f, 0.f};
		SimdVector mul(imul.v);

		m.vec[0] = SimdVector::LoadFloat4A16(ptr) * mul;
		m.vec[1] = SimdVector::LoadFloat3(&ptr[3]) * mul;
		m.vec[2] = SimdVector::LoadFloat3(&ptr[6]) * mul;
		m.vec[3] = SimdVector::LoadFloat3(&ptr[9]) * mul;

		mul = SimdVector::One() - mul;	//0 0 0 1

		m.vec[3] = m.vec[3] + mul;
#endif

		return m;
	}
	ENF_INLINE static SimdMatrix43 Load(const Matrix43& mat)
	{
		return LoadFloat4x3((const float*)&mat);
	}
	ENF_INLINE static SimdMatrix43 LoadA16(const Matrix43& mat)
	{
		return LoadFloat4x3A16((const float*)&mat);
	}
	ENF_INLINE static SimdMatrix43 LoadQuaternion(const SimdVector& q, const SimdVector& t);
	ENF_INLINE SimdMatrix34 operator*(const SimdMatrix34& b) const;
	ENF_INLINE SimdMatrix34 Transpose() const;
};

//-----------------------------------------------------------------------------
ENF_ALIGN16(class) SimdMatrix34
{
public:
	SimdVector vec[3];

	ENF_INLINE static SimdMatrix34 LoadFloat4x3(const SimdVector& a, const SimdVector& b, const SimdVector& c)
	{
		SimdMatrix34 m;
		/*
		from:				to:
		1 2  3  4		1 4 7 10
		5 6  7  8		2 5 8 11
		9 10 11 12	3 6 9 12		
		*/
#if defined(ENF_XENON) || defined(ENF_PS3)
		SimdVector shiftB = b.RotateLeft(2); //7, 8, 5, 6 
		SimdVector shiftA = a.RotateLeft(2); //3, 4, 1, 2

		SimdVector x = a.MergeHigh(shiftB);		//1, 7, 2, 8
		SimdVector y = shiftB.MergeLow(c);		//5, 11, 6, 12
		SimdVector z = shiftA.MergeHigh(c);		//3, 9, 4, 10

		shiftB = y.RotateLeft(2);	//6, 12, 5, 11
		shiftA = z.RotateLeft(2);	//4, 10, 3, 9

		m.vec[0] = x.MergeHigh(shiftA);			//1 4 7 10
		m.vec[1] = x.MergeLow(shiftB);			//3 6 9 12
		m.vec[2] = z.MergeHigh(shiftB);			//2 5 8 11
#elif defined(ENF_SSE)
		//b(0,1,2,3)a(0,1,2,3)
		SimdVector ab = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 0, 3, 0));		//1, 4, 5, 8 (tedy 8 5 4 1)
		SimdVector bc = _mm_shuffle_ps(b, c, _MM_SHUFFLE(3, 1, 1, 2));		//7, 6, 10, 12	(tedy 12, 10, 6, 7)	
		SimdVector ac = _mm_shuffle_ps(a, c, _MM_SHUFFLE(2, 0, 2, 1));		//2, 3, 9, 11 (tedy 11, 9, 3, 2)

		m.vec[0] = _mm_shuffle_ps(ab, bc, _MM_SHUFFLE(2, 0, 1, 0));			//1 4 7 10 (tedy 10, 7, 4, 1)

		SimdVector b2 = _mm_shuffle_ps(bc, ac, _MM_SHUFFLE(2, 1, 3, 1));	//6, 12, 3, 9 (tedy 9, 3, 12, 6)
		m.vec[2] = _mm_shuffle_ps(b2, b2, _MM_SHUFFLE(1, 3, 0, 2));			//3 6 9 12 (tedy 12, 9, 6, 3)

		SimdVector c2 = _mm_shuffle_ps(ab, ac, _MM_SHUFFLE(3, 0, 3, 2));	//5, 8, 2, 11 (tedy 11, 2, 8, 5)
		m.vec[1] = _mm_shuffle_ps(c2, c2, _MM_SHUFFLE(3, 1, 0, 2));			//2 5 8 11 (tedy 11, 8, 5, 2)
#endif
		return m;
	}

	ENF_INLINE static SimdMatrix34 LoadFloat4x4(const SimdVector& a, const SimdVector& b, const SimdVector& c, const SimdVector& d)
	{
		SimdVector acxy	= a.MergeHigh(c);	//ax, cx, ay, cy
		SimdVector acz		= a.MergeLow(c);	//az, cz, x, x
		SimdVector bdxy	= b.MergeHigh(d);	//bx, dx, by, dy
		SimdVector bdz		= b.MergeLow(d);	//bz, dz, x, x

		SimdMatrix34 mat;
		mat.vec[0] = acxy.MergeHigh(bdxy);	//ax, bx, cx, dx
		mat.vec[1] = acxy.MergeLow(bdxy);	//ay, by, cy, dy
		mat.vec[2] = acz.MergeHigh(bdz);		//az, bz, cz, dz
		return mat;
	}

	SimdMatrix34(){};

	ENF_INLINE static SimdMatrix34 LoadFloat4x3A16(const float* ptr)
	{
		SimdVector a = SimdVector::LoadFloat4A16Offset(ptr, 0);	//1, 2, 3, 4
		SimdVector b = SimdVector::LoadFloat4A16Offset(ptr, 16);	//5, 6, 7, 8
		SimdVector c = SimdVector::LoadFloat4A16Offset(ptr, 32);	//9, 10, 11, 12
		return LoadFloat4x3(a, b, c);
	}

   ENF_INLINE static SimdMatrix34 LoadFloat4x3(const float* ptr)
	{
		SimdVector a = SimdVector::LoadFloat4(ptr);		//1, 2, 3, 4
		SimdVector b = SimdVector::LoadFloat4(&ptr[4]);	//5, 6, 7, 8
		SimdVector c = SimdVector::LoadFloat4(&ptr[8]);	//9, 10, 11, 12
		return LoadFloat4x3(a, b, c);
	}

	SimdMatrix34(const float* ptr)
	{
		vec[0] = SimdVector::LoadFloat4A16Offset(ptr, 0);
		vec[1] = SimdVector::LoadFloat4A16Offset(ptr, 16);
		vec[2] = SimdVector::LoadFloat4A16Offset(ptr, 32);
	}
	
	ENF_INLINE static SimdMatrix34 Load(const Matrix43& mat)
	{
		return LoadFloat4x3((const float*)&mat);
	}
	ENF_INLINE static SimdMatrix34 LoadA16(const Matrix43& mat)
	{
		return LoadFloat4x3A16((const float*)&mat);
	}

	SimdMatrix34(const SimdVector& x, const SimdVector& y, const SimdVector& z)
	{
		vec[0] = x;
		vec[1] = y;
		vec[2] = z;
	}
	SimdMatrix34(const SimdVector& row, const float* col)
	{
		vec[0] = row * SimdVector(col[0]);
		vec[1] = row * SimdVector(col[1]);
		vec[2] = row * SimdVector(col[2]);
	}
	SimdMatrix34(const SimdVector& a, const SimdVector& b, const SimdVector& c, const SimdVector& d)
	{
		*this = LoadFloat4x4(a, b, c, d);
	}
	SimdMatrix34(const float* fa, const float* fb, const float* fc, const float* fd)
	{
		SimdVector a = SimdVector::LoadFloat3A16(fa);
		SimdVector b = SimdVector::LoadFloat3A16(fb);
		SimdVector c = SimdVector::LoadFloat3A16(fc);
		SimdVector d = SimdVector::LoadFloat3A16(fd);
		*this = LoadFloat4x4(a, b, c, d);
	}
	
	SimdMatrix34 RowSub(const SimdVector& v) const
	{
		SimdMatrix34 out;
		out.vec[0] = vec[0] - v;
		out.vec[1] = vec[1] - v;
		out.vec[2] = vec[2] - v;

		return out;
	}
	SimdMatrix34 ColSub(const SimdVector& v) const
	{
		SimdMatrix34 out;
		out.vec[0] = vec[0] - v.SplatX();
		out.vec[1] = vec[1] - v.SplatY();
		out.vec[2] = vec[2] - v.SplatZ();

		return out;
	}
	SimdVector RowSum() const
	{
		return vec[0] + vec[1] + vec[2];
	}
	//SimdMatrix34 RowPow() const - Removed! The same is mat.ElemMul(mat)
	SimdVector Dot(const float* v) const
	{
		return vec[0]*SimdVector(v[0]) + vec[1]*SimdVector(v[1]) + vec[2]*SimdVector(v[2]);
	}
	SimdMatrix34 operator*(const SimdVector& v) const
	{
		return SimdMatrix34(vec[0]*v, vec[1]*v, vec[2]*v);
	}
	SimdMatrix34 operator+(const SimdMatrix34& m) const
	{
		return SimdMatrix34(vec[0]+m.vec[0], vec[1]+m.vec[1], vec[2]+m.vec[2]);
	}
	SimdMatrix34 operator-(const SimdMatrix34& m) const
	{
		return SimdMatrix34(vec[0]-m.vec[0], vec[1]-m.vec[1], vec[2]-m.vec[2]);
	}
	SimdMatrix34 operator&(const SimdVector& v) const
	{
		SimdMatrix34 out;
		out.vec[0] = vec[0] & v;
		out.vec[1] = vec[1] & v;
		out.vec[2] = vec[2] & v;

		return out;
	}
	SimdVector SqrLen() const
	{
		return (vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
	}
	SimdVector Len() const
	{
		return SqrLen().Sqrt();
	}
	SimdMatrix34 Min(const SimdScalar& bound) const
	{
		SimdMatrix34 out;
		SimdVector v = bound;

		out.vec[0] = vec[0].Min(v);
		out.vec[1] = vec[1].Min(v);
		out.vec[2] = vec[2].Min(v);

		return out;
	}
	SimdMatrix34 Max(const SimdScalar& bound) const
	{
		SimdMatrix34 out;
		SimdVector v = bound;

		out.vec[0] = vec[0].Max(v);
		out.vec[1] = vec[1].Max(v);
		out.vec[2] = vec[2].Max(v);

		return out;
	}
	SimdVector Min() const
	{
		/*
		0: ax bx cx dx
		1: ay by cy dy
		2: az bz cz dz
		*/
#ifdef ENF_XENON
		__vector4 ab = __vmrghw(vec[0], vec[2]);	//ax, az, bx, bz
		__vector4 cd = __vmrglw(vec[0], vec[2]);	//cx, cz, dx, dz
		__vector4 min2a = __vminfp(ab, cd);			//min(ax,cx), min(az,cz), min(bx, dx), min(bz, dz)

		__vector4 min2b = __vrlimi(min2b, min2a, 0xf, 2); //min(bx, dx), min(bz, dz), x, x
		__vector4 XZres = __vminfp(min2a, min2b);	//min(ax, bx, cx, dx), min(az, bz, cz, dz)

		ab = __vmrghw(vec[1], vec[0]);				//ay, ax, by, bx
		cd = __vmrglw(vec[1], vec[0]);				//cy, cx, dy, dx
		min2a = __vminfp(ab, cd);						//min(ay, cy), min(ax, cx), min(by, dy), min(bx, dx)
		min2b = __vrlimi(min2b, min2a, 0xf, 2);	//min(by, dy), min(bx, dx), x, x
		__vector4 Yres = __vminfp(min2a, min2b);	//min(ay, by, cy, dy), min(ax, bx, cx, dx)

		return __vmrghw(XZres, Yres);					//min(ax, bx, cx, dx), min(ay, by, cy, dy), min(az, bz, cz, dz), dummy:min(ax, bx, cx, dx)
#elif defined(ENF_PS3)
		vec_float4 ab = vec_vmrghw(vec[0], vec[2]);	//ax, az, bx, bz
		vec_float4 cd = vec_vmrglw(vec[0], vec[2]);	//cx, cz, dx, dz
		vec_float4 min2a = vec_vminfp(ab, cd);			//min(ax,cx), min(az,cz), min(bx, dx), min(bz, dz)

		vec_float4 min2b = vec_sld(min2a, min2a, 8);	//min(bx, dx), min(bz, dz), x, x
		vec_float4 XZres = vec_vminfp(min2a, min2b);	//min(ax, bx, cx, dx), min(az, bz, cz, dz)

		ab = vec_vmrghw(vec[1], vec[0]);				//ay, ax, by, bx
		cd = vec_vmrglw(vec[1], vec[0]);				//cy, cx, dy, dx
		min2a = vec_vminfp(ab, cd);						//min(ay, cy), min(ax, cx), min(by, dy), min(bx, dx)
		min2b = vec_sld(min2a, min2a, 8);					//min(by, dy), min(bx, dx), x, x
		vec_float4 Yres = vec_vminfp(min2a, min2b);	//min(ay, by, cy, dy), min(ax, bx, cx, dx)

		return vec_vmrghw(XZres, Yres);					//min(ax, bx, cx, dx), min(ay, by, cy, dy), min(az, bz, cz, dz), dummy:min(ax, bx, cx, dx)
#elif defined(ENF_SSE)
		SimdScalar xmin = vec[0].Min();
		SimdScalar ymin = vec[1].Min();
		SimdScalar zmin = vec[2].Min();

		__m128 tmp = _mm_shuffle_ps(xmin, ymin, _MM_SHUFFLE(0, 0, 0, 0)); //x, x, y, y
		return _mm_shuffle_ps(tmp, zmin, _MM_SHUFFLE(2, 0, 2, 0));			//?, z, x, y
#endif
	}
	SimdVector Max() const
	{
#ifdef ENF_XENON
		__vector4 ab = __vmrghw(vec[0], vec[2]);	//ax, az, bx, bz
		__vector4 cd = __vmrglw(vec[0], vec[2]);	//cx, cz, dx, dz
		__vector4 max2a = __vmaxfp(ab, cd);			//max(ax,cx), max(az,cz), max(bx, dx), max(bz, dz)

		__vector4 max2b = __vrlimi(max2b, max2a, 0xf, 2);	//max(bx, dx), max(bz, dz), x, x
		__vector4 XZres = __vmaxfp(max2a, max2b);	//max(ax, bx, cx, dx), max(az, bz, cz, dz)

		ab = __vmrghw(vec[1], vec[0]);				//ay, ax, by, bx
		cd = __vmrglw(vec[1], vec[0]);				//cy, cx, dy, dx
		max2a = __vmaxfp(ab, cd);						//max(ay, cy), max(ax, cx), max(by, dy), max(bx, dx)
		max2b = __vrlimi(max2b, max2a, 0xf, 2);	//max(by, dy), max(bx, dx), x, x
		__vector4 Yres = __vmaxfp(max2a, max2b);	//max(ay, by, cy, dy), max(ax, bx, cx, dx)

		return __vmrghw(XZres, Yres);					//max(ax, bx, cx, dx), max(ay, by, cy, dy), max(az, bz, cz, dz), dummy:max(ax, bx, cx, dx)
#elif defined(ENF_PS3)
		vec_float4 ab = vec_vmrghw(vec[0], vec[2]);	//ax, az, bx, bz
		vec_float4 cd = vec_vmrglw(vec[0], vec[2]);	//cx, cz, dx, dz
		vec_float4 max2a = vec_vmaxfp(ab, cd);			//min(ax,cx), min(az,cz), min(bx, dx), min(bz, dz)

		vec_float4 max2b = vec_sld(max2a, max2a, 8);	//min(bx, dx), min(bz, dz), x, x
		vec_float4 XZres = vec_vmaxfp(max2a, max2b);	//min(ax, bx, cx, dx), min(az, bz, cz, dz)

		ab = vec_vmrghw(vec[1], vec[0]);				//ay, ax, by, bx
		cd = vec_vmrglw(vec[1], vec[0]);				//cy, cx, dy, dx
		max2a = vec_vmaxfp(ab, cd);						//min(ay, cy), min(ax, cx), min(by, dy), min(bx, dx)
		max2b = vec_sld(max2a, max2a, 8);					//min(by, dy), min(bx, dx), x, x
		vec_float4 Yres = vec_vmaxfp(max2a, max2b);	//min(ay, by, cy, dy), min(ax, bx, cx, dx)

		return vec_vmrghw(XZres, Yres);					//min(ax, bx, cx, dx), min(ay, by, cy, dy), min(az, bz, cz, dz), dummy:min(ax, bx, cx, dx)
#elif defined(ENF_SSE)
		SimdScalar xmax = vec[0].Max();
		SimdScalar ymax = vec[1].Max();
		SimdScalar zmax = vec[2].Max();

		__m128 tmp = _mm_shuffle_ps(xmax, ymax, _MM_SHUFFLE(0, 0, 0, 0)); //x, x, y, y
		return _mm_shuffle_ps(tmp, zmax, _MM_SHUFFLE(2, 0, 2, 0));			//x, y, z 
#endif
	}
	SimdMatrix34 ElemMul(const SimdMatrix34& mx) const
	{
		SimdMatrix34 out;
		out.vec[0] = vec[0] * mx.vec[0];
		out.vec[1] = vec[1] * mx.vec[1];
		out.vec[2] = vec[2] * mx.vec[2];

		return out;
	}
	SimdMatrix34 Select(const SimdMatrix34& mx, const SimdVector& mask)
	{
		SimdMatrix34 out;
		out.vec[0] = vec[0].Select(mx.vec[0], mask);
		out.vec[1] = vec[1].Select(mx.vec[1], mask);
		out.vec[2] = vec[2].Select(mx.vec[2], mask);

		return out;
	}

	SimdVector TransformVector(const SimdVector& pos) const
	{
		SimdVector row0 = pos * vec[0];
		SimdVector row1 = pos * vec[1];
		SimdVector row2 = pos * vec[2];
		SimdVector row3 = SimdVector::Zero();

		SimdVector tmp0 = row0.MergeHigh(row2);
		SimdVector tmp1 = row1.MergeHigh(row3);
		SimdVector tmp2 = row0.MergeLow(row2);
		SimdVector tmp3 = row1.MergeLow(row3);

		row0 = tmp0.MergeHigh(tmp1);
		row1 = tmp0.MergeLow(tmp1);
		row2 = tmp2.MergeHigh(tmp3);
		return row0 + row1 + row2;
	}

	SimdVector TransformCoord(const SimdVector& pos) const
	{
		SimdVector row0 = pos * vec[0];
		SimdVector row1 = pos * vec[1];
		SimdVector row2 = pos * vec[2];
		SimdVector row3 = SimdVector::Zero();

		SimdVector tmp0 = row0.MergeHigh(row2);
		SimdVector tmp1 = row1.MergeHigh(row3);
		SimdVector tmp2 = row0.MergeLow(row2);
		SimdVector tmp3 = row1.MergeLow(row3);

		row0 = tmp0.MergeHigh(tmp1);
		row1 = tmp0.MergeLow(tmp1);
		row2 = tmp2.MergeHigh(tmp3);
		row3 = tmp2.MergeLow(tmp3);
		return row0 + row1 + row2 + row3;
	}

	void StoreFloat3x4A16(float* ptr) const
	{
		//by rows
		vec[0].StoreFloat4A16Offset(ptr, 0);
		vec[1].StoreFloat4A16Offset(ptr, 16);
		vec[2].StoreFloat4A16Offset(ptr, 32);
	}

	void StoreFloat4x3A16(float *ptr) const
	{
		//by cols
		/*
		from:				to:
		ax bx cx dx		ax ay az bx
		ay by cy dy		by bz cx cy
		az bz cz dz		cz dx dy dz
		*/
#ifdef ENF_XENON
		//0-3(0x10203),		4-7(0x4050607),		8-11(0x8090a0b),		12-15(0xc0d0e0f)
		//16-19(0x10111213), 20-23(0x14151617),	24-27(0x18191a1b),	28-31(0x1c1d1e1f)
		static const SimdVectorI32 maskA = {0x10203, 0x4050607, 0x10111213, 0x8090a0b};	//0142
		static const SimdVectorI32 maskB = {0x8090a0b, 0xc0d0e0f, 0x10111213, 0x14151617};	//2345
		static const SimdVectorI32 maskC = {0x4050607, 0x1c1d1e1f, 0x8090a0b, 0xc0d0e0f};	//1723

		SimdVector a	= __vmrghw(vec[0], vec[1]);		//ax, ay, bx, by
		a					= __vperm(a, vec[2], maskA.v);

		SimdVector tmp = __vmrghw(vec[1], vec[2]);		//ay, az, by, bz
		SimdVector b	= __vmrglw(vec[0], vec[1]);		//cx, cy, dx, dy
		b					= __vperm(tmp, b, maskB.v);

		SimdVector c	= __vmrglw(vec[1], vec[2]);		//cy, cz, dy, dz
		c					= __vperm(c, vec[0], maskC.v);
#elif defined(ENF_PS3)
		//0-3(0x10203),		4-7(0x4050607),		8-11(0x8090a0b),		12-15(0xc0d0e0f)
		//16-19(0x10111213), 20-23(0x14151617),	24-27(0x18191a1b),	28-31(0x1c1d1e1f)
		static const SimdVectorI32 maskA = {0x10203, 0x4050607, 0x10111213, 0x8090a0b};	//0142
		static const SimdVectorI32 maskB = {0x8090a0b, 0xc0d0e0f, 0x10111213, 0x14151617};	//2345
		static const SimdVectorI32 maskC = {0x4050607, 0x1c1d1e1f, 0x8090a0b, 0xc0d0e0f};	//1723

		SimdVector a	= vec_vmrghw(vec[0], vec[1]);		//ax, ay, bx, by
		a					= vec_perm(a, vec[2], (vec_uchar16)maskA.v);

		SimdVector tmp = vec_vmrghw(vec[1], vec[2]);		//ay, az, by, bz
		SimdVector b	= vec_vmrglw(vec[0], vec[1]);		//cx, cy, dx, dy
		b					= vec_perm(tmp, b, (vec_uchar16)maskB.v);

		SimdVector c	= vec_vmrglw(vec[1], vec[2]);		//cy, cz, dy, dz
		c					= vec_perm(c, vec[0], (vec_uchar16)maskC.v);

#elif defined(ENF_SSE)
		__m128 bcRest	= _mm_shuffle_ps(vec[1], vec[2], _MM_SHUFFLE(1, 0, 3, 0));	//ay, dy, az, bz
		__m128 aTmp		= _mm_shuffle_ps(vec[0], bcRest, _MM_SHUFFLE(2, 0, 1, 0));	//ax, bx, ay, az
		bcRest			= _mm_shuffle_ps(bcRest, vec[0], _MM_SHUFFLE(3, 2, 3, 1));	//dy, bz, cx, dx

		SimdVector b	= _mm_shuffle_ps(vec[1], bcRest, _MM_SHUFFLE(2, 1, 2, 1));	//by, cy, bz, cx
		SimdVector c	= _mm_shuffle_ps(vec[2], bcRest, _MM_SHUFFLE(0, 3, 3, 2)); //cz, dz, dx, dy

		SimdVector a	= _mm_shuffle_ps(aTmp, aTmp, _MM_SHUFFLE(1, 3, 2, 0));	//ax, ay, az, bx
		b	= _mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 3, 2, 0));			//by, bz, cx, cy
		c	= _mm_shuffle_ps(c, c, _MM_SHUFFLE(1, 3, 2, 0));			//cz, dx, dy, dz

#endif
		a.StoreFloat4A16Offset(ptr, 0);
		b.StoreFloat4A16Offset(ptr, 16);
		c.StoreFloat4A16Offset(ptr, 32);

	}
	void StoreA16(Matrix43& mat) const
	{
		StoreFloat4x3A16((float*)&mat);
	}
	void StoreA16(Matrix34& mat) const
	{
		StoreFloat3x4A16((float*)&mat);
	}
	void StoreFloat4x4A16(float* ptrA, float* ptrB, float* ptrC, float* ptrD) const
	{
		//by cols
		/*
		from:				to:
		ax bx cx dx		ax ay az x
		ay by cy dy		bx by bz x
		az bz cz dz		cx cy cz x
							dx dy dz x
		*/
#if defined(ENF_XENON) || defined(ENF_PS3)
		SimdVector abXZ = vec[0].MergeHigh(vec[2]);		//ax, az, bx, bz
		SimdVector cdXZ = vec[0].MergeLow(vec[2]);		//cx, cz, dx, dz

		SimdVector a = abXZ.MergeHigh(vec[1]);		//ax, ay, az, by
		SimdVector b = vec[1].RotateLeft(3);				//x, ay, by, cy
		b = abXZ.MergeLow(b);								//bx, by, bz, cy

		SimdVector dTmp = vec[1].RotateLeft(1);			//by, cy, dy, x
		SimdVector d = cdXZ.MergeLow(dTmp);				//dx, dy, dz, x

		SimdVector c = dTmp.RotateLeft(1);			//cy, dy, x
		c = cdXZ.MergeHigh(c);									//cx, cy, cz, dy
#elif defined(ENF_SSE)
		__m128 abXY	= _mm_shuffle_ps(vec[0], vec[1], _MM_SHUFFLE(1, 0, 1, 0));	//ax, bx, ay, by
		__m128 cdXY	= _mm_shuffle_ps(vec[0], vec[1], _MM_SHUFFLE(3, 2, 3, 2));	//cx, dx, cy, dy

		SimdVector a	= _mm_shuffle_ps(abXY, vec[2], _MM_SHUFFLE(0, 0, 2, 0));	//ax, ay, az, az
		SimdVector b	= _mm_shuffle_ps(abXY, vec[2], _MM_SHUFFLE(1, 1, 3, 1));	//bx, by, bz, bz
		SimdVector c	= _mm_shuffle_ps(cdXY, vec[2], _MM_SHUFFLE(2, 2, 2, 0));	//cx, cy, cz, cz
		SimdVector d	= _mm_shuffle_ps(cdXY, vec[2], _MM_SHUFFLE(3, 3, 3, 1));	//dx, dy, dz, dz

#endif
		//safe from last to first -> keep data valid in case of dummy filling nonexisting item by pointer to first one
		d.StoreFloat4A16(ptrD);
		c.StoreFloat4A16(ptrC);
		b.StoreFloat4A16(ptrB);
		a.StoreFloat4A16(ptrA);
	}

	ENF_INLINE SimdMatrix34 operator*(const SimdMatrix43& mat) const;
	ENF_INLINE SimdMatrix43 Transpose() const;
};




//@}
}

#endif //ENF_SIMDMATHLIB_H