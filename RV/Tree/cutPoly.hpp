#ifdef _MSC_VER
#pragma once
#endif

#ifndef _cutPoly_hpp_
#define _cutPoly_hpp_

// Horizont - basic 2D/3D raster operations
// (C) 1997, SUMA

#include "cutTypes.hpp"
#include "cutVertex.hpp"
#include "cutPlane.hpp"
#include <El/Math/math3d.hpp>
#include "cutTextbank.hpp"

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
//const int MaxPoly=32;
// when building occlusions there may be some very large polygons
const int MaxPoly=192;

class PolyProperties
{
	protected:
	Texture *_texture; // TODO: remove _texture member
	int _special; // some special properties

	// properties
	public:
	#if _DEBUG
		PolyProperties();
	#endif

	void Init();
	void Copy( const PolyProperties &src )
	{
		_texture=src._texture;
		_special=src._special; // some special properties
	}

	RString GetDebugText() const;
	int Special() const {return _special;}
	void SetSpecial( int special ) {_special=special;}
	void OrSpecial( int special ) {_special|=special;}
	void AndSpecial( int special ) {_special&=special;}

	void SetTexture( Texture *texture ) {_texture=texture;}
	Texture *GetTexture() const {return _texture;}
	void AnimateTexture( float deltaT );

	static void Prepare(Texture *texture, int special);
	static void PrepareTL(Texture *texture, int special);

	void PrepareTL() const;
};

#define PolyVerticesSize(n) sizeof(VertexIndex)+sizeof(VertexIndex)*(n)

// all geometry operations are done on vertices only
class PolyVertices
{
	protected:
	// typical poly - 3 or 4 vertices uses 8 or 10 B memory
	// poly properties use always 8 B memory
	// it is usefull to reduce this

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
	// returns true is some clipping occured (result is then is res)
	// if false, result is original polygon
	bool Clip( TLVertexTable &mesh, PolyVertices &res, Vector3Par normal, Coord d, ClipFlags test, const Camera &camera );
	bool Clip( TLVertexTable &mesh, PolyVertices &res, Vector3Par normal, Coord d );

	void Clip( TLVertexTable &mesh, const Camera &camera, ClipFlags clipFlags ); // clip to all planes
	void CheckClip( TLVertexTable &mesh, const Camera &camera, ClipFlags clipFlags );

	// splitting does two complementary clips a time
	// split in transformed space
	void Split( TLVertexTable &mesh, PolyVertices &clip, PolyVertices &rest, Vector3Par normal, Coord d );
	// split in model space
	void Split
	(
		const Matrix4 &toWorld,
		Shape &mesh, PolyVertices &clip, PolyVertices &rest,
		Vector3Par normal, Coord d
	);

	// split in texture space (by uv line)
	void SplitUV
	(
		Shape &mesh, PolyVertices &clip, PolyVertices &rest,
		float a, float b, float c
	);

	// drawing and projecting	
	//void Project( TLVertexMesh &mesh, const Matrix4 &projection ); // project all points

	// various geometry checks
	bool InsideFromX
	(
		const VertexTable &mesh, Vector3Par pos
	) const;
	//Vector3 Normal( const VertexMesh &mesh ) const;

	float GetArea( const VertexTable &mesh ) const;
	float GetAreaTop( const VertexTable &mesh ) const;

	// vertex to face material conversion
	int GetMaterial(const VertexTable &mesh) const;
	Vector3 GetNormal( const VertexTable &mesh ) const; // model space normal
	Vector3 GetViewNormal( const TLVertexTable &mesh ) const; // view space normal
	bool BackfaceCull( const TLVertexTable &mesh ) const;

	//void AutoClamp( const Shape *shape );
	void CalculateNormal( Plane &dst, const VertexTable &mesh );
	void CalculateD( Plane &dst, const VertexTable &mesh );

	bool Inside( const VertexTable &mesh, const Plane &plane, Vector3Par pos ) const;
	bool InsideFromTop
	(
		const VertexTable &mesh, const Plane &plane, Vector3Par pos,
		float *y=NULL, float *dX=NULL, float *dZ=NULL
	) const;
	float DistanceFromTop
	(
		const VertexTable &mesh, Vector3Par pos
	) const;
	float SquareDistance
	(
		const VertexTable &mesh, const Plane &plane, Vector3Par pos, Vector3 *normal=NULL
	) const;

	void Reflect( TLVertexTable &mesh ); // reverse if not decal
};

class Poly: public PolyProperties,public PolyVertices
{
	public:
	Poly(){}
	void Init(){PolyProperties::Init(),PolyVertices::Init();}
	void CopyProperties( const PolyProperties &src ) {PolyProperties::Copy(src);}

	void operator = ( const Poly &src )
	{
		memcpy(this,&src,PolyVerticesSize(src._n)+sizeof(PolyProperties));
	}
	Poly( const Poly &src )
	{
		memcpy(this,&src,PolyVerticesSize(src._n)+sizeof(PolyProperties));
	}

	static int TypicalItemSize() {return sizeof(PolyProperties)+PolyVerticesSize(4);}
	int ItemSize() const {return sizeof(PolyProperties)+PolyVerticesSize(N());}
	void Duplicate( Poly &dst ) const {memcpy(&dst,this,ItemSize());}

	float CalculateArea( const VertexTable &mesh ); // real space area
	float CalculateUVArea( const VertexTable &mesh ); // texture space area

};

// polyplain is used when many polygons share the same properties
class PolyPlain: public PolyVertices
{
	public:
	PolyPlain(){}
	void Init(){PolyVertices::Init();}

	void operator = ( const PolyPlain &src )
	{
		memcpy(this,&src,PolyVerticesSize(src._n));
	}
	PolyPlain( const PolyPlain &src )
	{
		memcpy(this,&src,PolyVerticesSize(src._n));
	}

	static int TypicalItemSize() {return PolyVerticesSize(4);}
	int ItemSize() const {return PolyVerticesSize(N());}
	void Duplicate( Poly &dst ) const {memcpy(&dst,this,ItemSize());}
};

TypeIsSimple(Poly);
TypeIsSimple(PolyPlain);

#include <Es/Containers/streamArray.hpp>
#include <Es/Containers/staticArray.hpp>

typedef StreamArray<PolyPlain,StaticArray<char> > PolyPlainArray;

inline void PolyVertices::Reverse()
{
	// revert geometry
	if( _n==3 )
	{
		swap(_vertex[0],_vertex[1]);
	}
	else if( _n==4 )
	{
		swap(_vertex[0],_vertex[1]);
		swap(_vertex[2],_vertex[3]);
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



