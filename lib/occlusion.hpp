#ifdef _MSC_VER
#pragma once
#endif

#ifndef  _OCCLUSION_HPP
#define  _OCCLUSION_HPP

#if _MSC_FULL_VER>=12008804
#define _COMPILER_CAN_MMX 1
#endif

#ifdef __ICL
#define _COMPILER_CAN_MMX 1
#endif

#define SIMD 0
#define SIMD2 0

#if SIMD || SIMD2
	// unsigned 8b SSE occlusions
	typedef unsigned char OccZType;
#else
	// signed 8b MMX occlusions
	typedef signed char OccZType;
#endif

#include "fixed.hpp"

#include "Shape/Shape.hpp"

#include <El/DebugWindow/debugWin.hpp>

class OcclusionPoly
{
	int _n;
	Vector3 _v[MaxPoly];

	public:
	OcclusionPoly(){_n=0;}
	void Copy( const OcclusionPoly &src );

	OcclusionPoly( const OcclusionPoly &src ){Copy(src);}
	void operator =( const OcclusionPoly &src ){Copy(src);}

	int N() const {return _n;}
	Vector3Val operator [] ( int index ) const {return _v[index];}
	Vector3Val Get( int index ) const {return _v[index];}

	void Add( Vector3Par v )
	{
		if (_n<MaxPoly)
		{
			_v[_n++]=v;
		}
		else
		{
			LogF("Too complex occlusion");
		}
	}
	void Clear() {_n=0;}

	// clipping with single plane
	bool Clip( OcclusionPoly &res, Vector3Par normal, Coord d ) const;

	// clipping with all planes
	void Clip( float cNear, float cFar, ClipFlags clip );

	void Perspective();

	void Transform(const Matrix4 &trans);

	void SumXYVolume(float &volume, Vector3 &cov) const;
	void SumCrossProducts(Vector3 &sum) const;
	void SumPositions(Vector3 &sum,int &count) const;
};

class Occlusion
{
	#if _ENABLE_CHEATS
	mutable Link<DebugMemWindow> _debugWin;
	#endif

	SRefArray<OccZType> _data;
	int _w,_h;
	float _w2,_h2; // precalculate w/2, h/2

	// note - ordered by column - accessed and rendered by column
	OccZType Get( int x, int y ) const {return _data[x*_h+y];}
	OccZType &Set( int x, int y ) {return _data[x*_h+y];}
	
	__forceinline OccZType &operator () ( int x, int y ) {return Set(x,y);}
	__forceinline OccZType operator () ( int x, int y ) const {return Get(x,y);}

	public:
	Occlusion( int w, int h );
	~Occlusion();

	bool TestRect( const OccZType *col, int w, int h, OccZType maxz ) const;
	bool TestProjectedBBox( const Vector3 &min, const Vector3 &max ) const;
	// project, clip and test bbox
	bool TestBBox
	(
		const Matrix4 &trans, const Vector3 *minMax, ClipFlags clip
	) const;

	void DebugPoly( const OcclusionPoly &poly, DebugPixel color ) const;

	// horizontal fill
	void RenderSpan( OccZType *tgt, int yBeg, int width, OccZType z );

	// render projected triangle
	void RenderTri
	(
		const Vector3 *vip, const Vector3 *vlp, const Vector3 *vrp
	);

	// predefined callbacks
	void RenderProjectedPoly( const OcclusionPoly &poly );
	void RenderPoly( const OcclusionPoly &poly, ClipFlags clip );
	bool TestPoint( Vector3Par pos ) const;
	bool TestPointWSpace( Vector3Par pos ) const;
	float TestSphereWSpace(Vector3Par pos, float radius) const;
	void RenderShape
	(
		Matrix4 &trans, Shape *shape, const ConvexComponents &components,
		ClipFlags orClip
	);

	void Clear();

	void OutputDebug();
};

#endif
