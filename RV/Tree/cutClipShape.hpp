#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CLIPSHAPE_HPP
#define _CLIPSHAPE_HPP

#include "cutTypes.hpp"
#include "cutPoly.hpp"
#include <Es/Containers/staticArray.hpp>

#include "cutMaterial.hpp"
// binary copy will do

#include <Es/Containers/streamArray.hpp>

class SerializeBinStream;

//! info about shape section properties (texture and material)

struct ShapeSectionInfo
{
	PolyProperties properties; //!< face properties
	int material; //!< special material index
	Ref<TexMaterial> surfMat; //!< generic surface material

	bool operator == (const ShapeSectionInfo &sec) const
	{
		return
		(
			properties.GetTexture()==sec.properties.GetTexture() &&
			properties.Special()==sec.properties.Special() &&
			surfMat==sec.surfMat &&
			material==sec.material
		);
	}
	bool operator != (const ShapeSectionInfo &sec) const
	{
		return
		(
			properties.GetTexture()!=sec.properties.GetTexture() ||
			properties.Special()!=sec.properties.Special() ||
			surfMat!=sec.surfMat ||
			material!=sec.material
		);
	}
};

TypeIsMovableZeroed(ShapeSectionInfo)

struct ShapeSection: public ShapeSectionInfo
{
	Offset beg,end; // beg,end in PolyPlainArray of Shape

	void SerializeBin(SerializeBinStream &f, Shape *shape);
	void PrepareTL
	(
		const TLMaterial &mat, const LightList &lights, int spec
	) const;
};

TypeIsMovableZeroed(ShapeSection);


class LightList;

class IAnimator;

class FaceArray: public StreamArray<Poly,StaticArray<char> >
{
	typedef StreamArray<Poly,StaticArray<char> > base;

	friend class Shape; // shape needs access to sections

	StaticArray<ShapeSection> _sections; // faces divided by sections

	public:
	FaceArray() {};
	~FaceArray() {};

	FaceArray( int size, bool dynamic );
	void ReserveFaces( int size, bool dynamic );
	void SetSections( const ShapeSection *sec, int nSec);
	
	void Draw
	(
		const IAnimator *matSource,
		const LightList &lights,
		const Shape &mesh, ClipFlags clip, int spec,
		const Matrix4 &transform, const Matrix4 &invTransform
	) const;
	void Draw
	(
		const IAnimator *matSource,
		TLVertexTable &tlTable,
		const LightList &lights,
		const Shape &mesh, ClipFlags clip, int spec,
		const Matrix4 &invTransform
	) const;

	void Clip
	(
		const FaceArray &faces, TLVertexTable &tlMesh,
		const Camera &camera, ClipFlags clipFlags, bool doCull=true
	);
	void SurfaceSplit
	(
		const FaceArray &faces, TLVertexTable &tlMesh,
		Scene &scene, ClipFlags clipFlags, float y
	);
	
	Poly *AddClipped
	(
		const Poly &face, TLVertexTable &tlMesh, Scene &scene,
		ClipFlags clipFlags
	);
	Poly *AddNoClip
	(
		const Poly &face, TLVertexTable &tlMesh, Scene &scene
	);

	bool VerifyStructure() const;
};

#endif
