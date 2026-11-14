#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SHAPE_FACE_ARRAY_HPP
#define _SHAPE_FACE_ARRAY_HPP

#include "poly.hpp"
#include <Es/Containers/streamArray.hpp>
class SerializeBinStream;

//! info about shape section properties (texture and material)

typedef PolyProperties ShapeSectionInfo;

class MaterialList;

//! Section of the shape
struct ShapeSection: public ShapeSectionInfo
{
  //! Offset to the first polygon in the polygon array
	Offset beg;
  //! Offset to the last polygon in the polygon array
  Offset end;
  //! Index of the bone with minimal index referenced by this section (or by some other within the same set)
  int _minBoneIndex;
  //! Size of the interval of bones this section belongs into
  int _bonesCount;
  //! Constructor
  ShapeSection()
  {
    _minBoneIndex = 0;
    _bonesCount = 0;
  }
  //! Serialization
	void SerializeBin(SerializeBinStream &f, Shape *shape, const MaterialList &materials);
  //! Sets information associated with the section
	void SetInfo(const PolyProperties &info)
	{
		PolyProperties::operator =(info);
	}
};
TypeIsMovableZeroed(ShapeSection);

/// data used for face streaming
class FaceData: public RefCount
{
  friend class FaceArray;
  friend class Shape;
  
  /// canonical form: all polygons triangular and guaranteed to be not degenerate
  bool _canonical;
  

  StreamArray<Poly,StaticArray<char> > _faces;

  public:  
  __forceinline int Size() const {return _faces.Size();}
	__forceinline Offset Begin() const {return _faces.Begin();}
	__forceinline Offset End() const {return _faces.End();}
  __forceinline const Poly &operator [] ( Offset pos ) const {return _faces[pos];}
	__forceinline void Next( Offset &index ) const {_faces.Next(index);}
	__forceinline void NextCanonical( Offset &index ) const {Assert(_canonical);OffsetAdvance(index,PolyVerticesSize(3));}
  __forceinline bool GetCanonical() const {return _canonical;}
	FaceData(){_canonical = false;};
	~FaceData() {};
};

class AnimationContext;

//! array of all faces that create given shape
class FaceArray
{
  Ref<FaceData> _data;

	friend class Shape; // shape needs access to sections
  friend class AnimationContext;

	StaticArray<ShapeSection> _sections; // faces divided by sections


  public:
  //@{ export StreamArray interface, locking may be checked/done here
  __forceinline const Poly &operator [] ( Offset pos ) const {return _data->_faces[pos];}
  __forceinline Poly &operator [] ( Offset pos ) {return _data->_faces[pos];}
	__forceinline void RawRealloc( int n ) {if (!_data) _data = new FaceData;_data->_faces.RawRealloc(n);}
	__forceinline Offset Begin() const {return Offset(0);}
	__forceinline Offset End() const {return _data ? _data->_faces.End() : Offset(0);}
	__forceinline void Next( Offset &index ) const {_data->_faces.Next(index);}
	__forceinline void NextCanonical( Offset &index ) const {Assert(GetCanonical());OffsetAdvance(index,PolyVerticesSize(3));}
	__forceinline void Merge( const char *data, int size, int n ){_data->_faces.Merge(data,size,n);}
  __forceinline void Merge(const FaceArray &src){_data->_faces.Merge(src._data->_faces);}
	__forceinline int RawSize() const {return _data ? _data->_faces.RawSize() : 0;}
  __forceinline void ReserveRaw( int size ) {if (!_data) _data = new FaceData;_data->_faces.ReserveRaw(size);}
	__forceinline Offset Add(const Poly &src) {if (!_data) _data = new FaceData;return _data->_faces.Add(src);} // add single
	__forceinline void Reserve( int n ){if (!_data) _data = new FaceData;_data->_faces.Reserve(n);}
	__forceinline void Compact() {if (_data) _data->_faces.Compact();}
  __forceinline int Size() const {return _data ? _data->_faces.Size() : 0;}
	__forceinline Offset Find(int index) const {return _data->_faces.Find(index);}
	__forceinline void Delete( Offset pos ){_data->_faces.Delete(pos);}
	__forceinline void Clear() {if (_data) _data->_faces.Clear();_sections.Clear();}
  __forceinline bool GetCanonical() const {return _data->_canonical;}
  __forceinline FaceData *GetData() const {return _data;}
  void SetCanonical(bool val);
	
	//@}

public:
	FaceArray() {};
	~FaceArray() {};
	FaceArray(const FaceArray &src);
	void operator = (const FaceArray &src);

	FaceArray( int size, bool dynamic );
	void ReserveFaces( int size, bool dynamic );
	void SetSections( const ShapeSection *sec, int nSec);


	void Draw	(
		ColorVal animColor,
		const LightList &lights,
    AnimationContext *animContext,
		const Shape &mesh, ClipFlags clip, int spec,
		const Matrix4 &transform, const Matrix4 &invTransform
	) const;
	void Draw	(
		ColorVal animColor,
		TLVertexTable &tlTable,
		const LightList &lights,
    AnimationContext *animContext,
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
	
	//! get section count
	__forceinline int NSections() const {return _sections.Size();}
	//! get specific section
	__forceinline const ShapeSection &GetSection(int i) const {return _sections[i];}

	Poly *AddClipped
	(
		const Poly &face, TLVertexTable &tlMesh, Scene &scene,
		ClipFlags clipFlags
	);
	Poly *AddNoClip
	(
		const Poly &face, TLVertexTable &tlMesh, Scene &scene
	);
	//! create a new section from the faces that are not in any section yet
	void CloseSection(const ShapeSectionInfo &prop);
	//! try to append faces that are not in any section yet to the last section
	//! if they cannot be appended, create a new section
  //! if forceNewSection is true, the section will be created in any case
	void ExtendSection(const ShapeSectionInfo &prop, bool forceNewSection = false);

	//! sort sections, includes face reordering
	void SortSections(int (*Compare)( const ShapeSection **p0, const ShapeSection **p1 ));

	/// concat sections with identical attributes
	void ConcatSections(int start);
	
  size_t GetMemoryAllocated() const
  {
    return (_data ? _data->_faces.GetMemoryAllocated() : 0)+_sections.GetMemoryAllocated();
  }
	bool VerifyStructure() const;
  double GetMemoryUsed() const;
};

#endif
