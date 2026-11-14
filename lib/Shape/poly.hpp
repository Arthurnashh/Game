#ifdef _MSC_VER
#pragma once
#endif

#ifndef _POLY_HPP
#define _POLY_HPP

// Horizont - basic 2D/3D raster operations
// (C) 1997, SUMA

#include "../types.hpp"
#include "vertex.hpp"
#include "plane.hpp"
#include "../engine.hpp"
#include <El/Math/math3d.hpp>
#include "../textbank.hpp"

#if _ENABLE_PHYSICS
	#include "Physics/enf_worldphysics.h"
#endif

// polygon stores all 2D information need for rendering
// six clipping planes can transform 4-gon into 10-gon
// additional splitting (surface-fitting) may add more points

// polygon can have max. 4 vertices
// 5 planes can be applied when doing surface split
// 6 planes can be applied when clipping to view frustum
// 1 plane can be applied when clipping to user plane
// this can result is 16 vertices
// polys are stored compressed anyway (StreamArray)
// we want to have some space reserved for sure
const int MaxPoly=32;
// when building occlusions there may be some very large polygons
//const int MaxPoly=192;

class TexMaterial;

//! definition about face properties (texture, material ...)
class PolyProperties
{
protected:
	RefR<Texture> _texture; //!< primary texture
	RefR<TexMaterial> _surfMat; //!< config defined material
	int _special; //!< some special properties
	//! real area divided by u-v area (used for mipmap selection)
  AreaOverTex _areaOverTex;

	// properties
public:
	void Init();

	RString GetDebugText() const;
	__forceinline int Special() const {return _special;}
	__forceinline void SetSpecial( int special ) {_special=special;}
	__forceinline void OrSpecial( int special ) {_special|=special;}
	__forceinline void AndSpecial( int special ) {_special&=special;}

	__forceinline float GetAreaOverTex(int uvStage) const {return _areaOverTex.Get(uvStage);}
	__forceinline void SetAreaOverTex(int uvStage, float areaOverTex) {_areaOverTex.Set(uvStage, areaOverTex);}
  __forceinline const AreaOverTex &GetAreaOverTex() const {return _areaOverTex;}

	TexMaterial *GetMaterialExt() const;
	void SetMaterialExt(TexMaterial *m);

	__forceinline void SetTexture( Texture *texture ) {_texture=texture;}
	__forceinline Texture *GetTexture() const {return _texture;}
	/// potentially unsafe - obtain back pointer to the texture Ref
	__forceinline const RefR<Texture> *GetTextureBackPointer() const {return &_texture;}

	const SurfaceInfo *GetSurfaceInfo() const;

#if _ENABLE_PHYSICS
  enf::PhysicsMaterial *GetPhysMaterial() const;
#endif

	void AnimateTexture(float phase);
	/// animate texture - will modify underlying material
	void AnimateStageTexture(int stage, float phase);

	static void Prepare(int cb, Texture *texture, const TexMaterial *mat, int special);
	//static void PrepareTL(Texture *texture, const TexMaterial *mat, int special);

  /// make sure texture mipmap levels are loaded as necessary for all stages
  MipInfo LoadTextureMipmaps(int spec) const;
  /// call LoadTextureMipmaps and prepare material for the engine
	void PrepareTL(
		int cb, const TLMaterial &mat, int spec, const EngineShapeProperties &prop, const DrawParameters &dp=DrawParameters::_default, bool UVchanged=false, float offsetU = 0.0f, float offsetV = 0.0f
	) const;

	bool operator == (const PolyProperties &sec) const;
	bool operator != (const PolyProperties &sec) const;
};

TypeIsMovableZeroed(PolyProperties)

#define PolyVerticesSize(n) (sizeof(VertexIndex)+sizeof(VertexIndex)*(n))

/// represent polygon vertices
/** all geometry operations are done on vertices only */
class PolyVertices
{
	protected:
	// typical poly - 3 or 4 vertices uses 8 or 10 B memory
	// poly properties use always 8 B memory
	// it is useful to reduce this

	VertexIndex _n; // point count
	VertexIndex _vertex[MaxPoly]; // transformed and lit vertices - ready for clipping

	public:
	// basic management functions
	void Clear(){_n=0;}
	void Init(){_n=0;}
	//void Add( int point ){_vertex[_n++]=point;}
	void Set( int i, VertexIndex point ) {_vertex[i]=point;}
	void SetN( VertexIndex i ) {_n=i;}
	__forceinline VertexIndex N() const {return _n;}
	__forceinline VertexIndex GetVertex( int i ) const {return _vertex[i];}
	__forceinline const VertexIndex *GetVertexList() const {return _vertex;}

	void operator = ( const PolyVertices &src )
	{
		memcpy(this,&src,PolyVerticesSize(src._n));
	}
	PolyVertices(){}
	PolyVertices( const PolyVertices &src )
	{
		memcpy(this,&src,PolyVerticesSize(src._n));
	}

	void Reverse();
	// advanced geometry functions

	void FitToLandscape( TLVertexTable &mesh, Scene &scene, float y );
	void FitToLandscape( const Matrix4 &toWorld, VertexTable &mesh, const Landscape &land, float y );

	// clipping
	// returns true is some clipping occurred (result is then is res)
	// if false, result is original polygon
	bool Clip( TLVertexTable &mesh, PolyVertices &res, Vector3Par normal, Coord d, ClipFlags test, const Camera &camera );
	bool Clip( TLVertexTable &mesh, PolyVertices &res, Vector3Par normal, Coord d );

	void Clip( TLVertexTable &mesh, const Camera &camera, ClipFlags clipFlags ); // clip to all planes
	void CheckClip( TLVertexTable &mesh, const Camera &camera, ClipFlags clipFlags );

	// splitting does two complementary clips a time
	// split in transformed space
	void Split( TLVertexTable &mesh, PolyVertices &clip, PolyVertices &rest, Vector3Par normal, Coord d );
	// split in model space
	void Split(const Matrix4 &toWorld, Shape &mesh, PolyVertices &clip, PolyVertices &rest, Vector3Par normal, Coord d, bool quickAdd=false);

	// various geometry checks
	bool InsideFromX(const VertexTable &mesh, Vector3Par pos) const;

	void GetMinMax(Vector3 &minV, Vector3 &maxV, const VertexTable &mesh ) const;
	float GetArea( const VertexTable &mesh ) const;
	float GetAreaTop( const VertexTable &mesh ) const;
	Vector3 GetAverage( const VertexTable &mesh ) const;

	Vector3 GetNormal(const Array<Vector3> &pos) const; // model space normal
	void CalculateNormal(Plane &dst, const Array<Vector3> &pos) const;
	void CalculateD(Plane &dst, const Array<Vector3> &pos) const;

	bool Inside(Vector3Par dir, const Array<Vector3> &posArray, const Plane &plane, Vector3Par pos, float *under=NULL) const;
	bool InsideFromTop(
		const Array<Vector3> &posArray, const Plane &plane, Vector3Par pos,
		float *y=NULL, float *dX=NULL, float *dZ=NULL
	) const;
#if _VBS3 //required for new roads
  bool InsideFromBottom
	(
		const Array<Vector3> &posArray, const Plane &plane, Vector3Par pos,
		float *y=NULL, float *dX=NULL, float *dZ=NULL
	) const;
#endif
	float DistanceFromTop(const VertexTable &mesh, Vector3Par pos) const;
	float SquareDistance(const VertexTable &mesh, const Plane &plane, Vector3Par pos, Vector3 *normal=NULL) const;
};

/// One face - stores face vertices
class Poly: public PolyVertices
{
	public:
	Poly(){}
	void Init(){PolyVertices::Init();}

	void operator = ( const Poly &src )
	{
		memcpy(this,&src,PolyVerticesSize(src._n));
	}
	Poly( const Poly &src )
	{
		memcpy(this,&src,PolyVerticesSize(src._n));
	}

	static int TypicalItemSize() {return PolyVerticesSize(4);}
	int ItemSize() const {return PolyVerticesSize(N());}
	void Duplicate( Poly &dst ) const
	{
	  #if _MSC_VER  
	    {
	      COMPILETIME_COMPARE(sizeof(_n),2);
	    }
	    {
	      COMPILETIME_COMPARE(sizeof(VertexIndex),2);
	    }
	    Assert(N()+1==ItemSize()/2);
	    __movsw(reinterpret_cast<unsigned short *>(&dst),reinterpret_cast<const unsigned short *>(this),N()+1);
	  #else
  	  memcpy(&dst,this,ItemSize());
	  #endif
	}
	void DuplicateVertices(VertexIndex *dst) const
	{
	  #if _MSC_VER  
	    COMPILETIME_COMPARE(sizeof(VertexIndex),2);
	    __movsw(reinterpret_cast<unsigned short *>(dst),reinterpret_cast<const unsigned short *>(_vertex),N());
	  #else
  	  memcpy(dst,_vertex,N()*sizeof(VertexIndex));
	  #endif
	}

	float CalculateUDensity(const VertexTable &mesh, int uvStage) const; ///< texture space density
	float CalculateVDensity(const VertexTable &mesh, int uvStage) const; ///< texture space density
	float CalculateUVArea(const VertexTable &mesh, int uvStage) const; ///< texture space density

};

inline void PolyVertices::Reverse()
{
	// revert geometry
	if( _n==3 )
	{
		Swap(_vertex[0],_vertex[1]);
	}
	else if( _n==4 )
	{
		Swap(_vertex[0],_vertex[1]);
		Swap(_vertex[2],_vertex[3]);
	}
	else
	{
		int i;
		int temp[MaxPoly];
		for( i=0; i<_n; i++ ) temp[i]=_vertex[i];
		for( i=0; i<_n; i++ ) _vertex[i]=temp[_n-1-i];
	}
}

#endif



