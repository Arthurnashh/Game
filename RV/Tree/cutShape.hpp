#ifdef _MSC_VER
#pragma once
#endif

#ifndef _cutShape_hpp_
#define _cutShape_hpp_


#include <Es/Containers/boolArray.hpp>

#include <El/Math/math3d.hpp>
#include "cutVertex.hpp"

#include <El/QStream/QStream.hpp>


#ifdef _MSC_VER
	#pragma warning( disable: 4355 )
#endif

//! information about single vertex relation to set (see Selection)

struct SelInfo
{
	VertexIndex index;
	byte weight;
	SelInfo(){}
	SelInfo( VertexIndex i, byte w ):index(i),weight(w){}
};

TypeIsSimple(SelInfo);

class SerializeBinStream;
class IAnimator;

//! generic fuzzy set of indices

class Selection
{
	protected:
	Buffer<VertexIndex> _sel; // short is enough to hold vertex index
	Buffer<byte> _weights; // byte is enough to hold weighting

	protected:
	void DoDestruct();
	void DoConstruct( const Selection &src );
	
	public:
	Selection(){}
	Selection( const SelInfo *sel, int nSel );
	Selection( const VertexIndex *sel, int nSel );
	void operator = ( const Selection &src ){DoDestruct();DoConstruct(src);}
	Selection( const Selection &src ){DoConstruct(src);}
	void Delete(){DoDestruct();}

	void CreateIntervals();

	int Size() const {return _sel.Length();}
	VertexIndex operator [] ( int i ) const {return _sel[i];}
	byte Weight( int i ) const
	{
		if (!_weights) return 255; // singular case
		return _weights[i];
	}
	bool IsSelected( int i ) const;
	bool IsSubset(const Selection &sel) const;
	int Find( int i ) const;

	void Add(int vi, byte weigth);
	void Add(int vi){Add(vi,255);}

	void SerializeBin(SerializeBinStream &f);
};

TypeIsMovable(Selection);

#include <Es/Containers/streamArray.hpp>

//! set of faces

class FaceSelection: public Buffer<Offset>
{
	Buffer<int> _sections; // which sections are included in this selection
	bool _needsSections; // some selections have to include sections

	public:
	FaceSelection();
	~FaceSelection();

	void RescanSections(Shape *shape, const char *debugName);

	bool GetNeedsSections() const {return _needsSections;}
	void SetNeedsSections(bool needsSections) {_needsSections=needsSections;}

	int NSections() const {return _sections.Size();}
	int GetSection(int i) const {return _sections[i];}
	void SerializeBin(SerializeBinStream &f);
};

//! named selection - fuzzy set of vertices and set of faces

class NamedSelection: public Selection
{
	friend class Shape;
	private:
	RStringB _name;
	Selection _faces;
	mutable FaceSelection _faceSel;
	//mutable Selection _fromFaces; // can be constructed from faces
	// note: section is face-based

	protected:
	void DoConstruct();
	void DoDestruct();
	void DoConstruct( const NamedSelection &src );

	public:
	NamedSelection
	(
		const char *name,
		const SelInfo *points, int nPoints,
		const VertexIndex *faces, int nFaces
	);

	NamedSelection(){DoConstruct();}
	void operator = ( const NamedSelection &src ){DoDestruct();DoConstruct(src);}
	NamedSelection( const NamedSelection &src ){DoConstruct(src);}
	const char *Name() const {return _name;}
	const RStringB &GetName() const {return _name;}
	Selection &Faces() {return _faces;}
	const Selection &Faces() const {return _faces;}

	//const Selection &FromFaces( Shape *shape ) const;
	void RescanSections(Shape *shape) {_faceSel.RescanSections(shape,Name());}

	bool GetNeedsSections() const {return _faceSel.GetNeedsSections();}
	void SetNeedsSections(bool needsSections) {_faceSel.SetNeedsSections(needsSections);}

	int NSections() const {return _faceSel.NSections();}
	int GetSection(int i) const {return _faceSel.GetSection(i);}

	const FaceSelection &FaceOffsets( Shape *shape ) const;
	bool FaceOffsetsReady() const; // calculated only when needed

	void SerializeBin(SerializeBinStream &f);
};

TypeIsMovable(NamedSelection);

//! named propetry - generic string value

class NamedProperty
{
	private:
	RStringB _name,_value;
	
	public:
	NamedProperty( const char *name="", const char *value="" );
	const RStringB &Name() const {return _name;}
	const RStringB &Value() const {return _value;}

	void SerializeBin(SerializeBinStream &f);
};

TypeIsMovableZeroed(NamedProperty);

//! one frame of keyframe animation

class AnimationPhase
{
	AutoArray<Vector3> _points;
	// note: only ClipUserMask part of Clipped is used
	float _time;
	
	public:
	AnimationPhase(){_time=0;}

	void SetTime( float time ){_time=time;}
	float Time() const {return _time;}

	Vector3Val operator [] ( int i ) const {return _points[i];}
	Vector3 &operator [] ( int i ) {return _points[i];}
	int Size() const {return _points.Size();}
	void Resize( int n ) {_points.Resize(n);}

	void SerializeBin(SerializeBinStream &f);
};

TypeIsMovable(AnimationPhase);

#include "cutClipShape.hpp"

#include <Es/Memory/normalNew.hpp>

struct ProxyObject: public RefCount
{
	// note: scale allowed in transform
	Ref<Object> obj;
	RStringB name; // original proxy name
	//Matrix4 transform,invTransform;
	Matrix4 invTransform;
	// color/state identical to parent object
	int id;
	int selection; // source selection index
	USE_FAST_ALLOCATOR

	void SerializeBin(SerializeBinStream &f);
};

struct MTextureMap;

//TypeIsMovableZeroed(ProxyObject)

class ParamEntry;

//! collection of all information connected to single level.
/*!
  Shape is mainly used to build LODShape LOD level set.
  It is based on VertexTable and besides of vertices contains also faces,
  named selections (NamedSelection), named properties (NamedProperty),
  keyframe animation (AnimationPhase) and proxy object (ProxyObject)
*/
class Shape: public VertexTable
{
	friend class Object;
	friend class LODShape;
	
 private:
	
  //! Stream of faces.
  /*!
    One face consists from several indices. Indices refer to vertices from vertex table.
  */
	FaceArray _face;
  //! Array of planes (Normal vector, distance) corresponds to array of faces.
	mutable AutoArray<Plane> _plane;

	RefArray<Texture> _textures; // all textures used in this object
	AutoArray<float> _areaOTex; // are associated with texture

	// TODO: avoid conversion
	AutoArray<VertexIndex> _pointToVertex; // objektiv point index to vertex index conversion
	AutoArray<VertexIndex> _vertexToPoint;

	mutable AutoArray<Offset> _faceIndexToOffset; // built when neccessary

	int _level; // which level of LODShape is this

	
	// named selections	- used for animations...
	AutoArray<NamedSelection> _sel;
	AutoArray<NamedProperty> _prop;
	AutoArray<AnimationPhase> _phase;
	PackedColor _colorTop;
	PackedColor _color;
	int _special;
	bool _faceNormalsValid;
	bool _loadWarning; // some warning during load - report filename

	// proxy objects
	RefArray<ProxyObject> _proxy;


	protected:
	// initializers and deinitializers
	// load shape with taggs, return resolution
	float LoadTagged
	(
		QIStream &f, bool reversed, int ver, bool geomteryOnly,
		AutoArray<float> &massArray, bool tagged
	);

	void Reverse();

	void SerializeBin(SerializeBinStream &f);

	void AddPhase( const AnimationPhase &phase );

	private:
	void operator = ( const Shape &src ); // assignment not defined

	public:
	// constructors and destructors
	Shape();
	Shape( const Shape &src, bool copyAnimations=true );

	//Shape( QIStream &f, bool reversed=false );
	virtual ~Shape();
	void Clear();
  //! Sorts faces according to textures.
  /*!
    This feature helps to minimize the number of state changes.
  */
	void Optimize(); // sort by texture/render state
	// be carefull: Optimize invalidates face offsets/indices
	void SortVertices();
	// be carefull: SortVertices invalidates vertex indices
	void FindSections(bool forceMaterial0=false); // find sections - take care of animated textures

	int NProxies() const {return _proxy.Size();}
	ProxyObject &Proxy(int i) {return *_proxy[i];}
	const ProxyObject &Proxy(int i) const {return *_proxy[i];}
	
	int NFaces() const {return _face.Size();}
	int FacesRawSize() const {return _face.RawSize();}

	// conversion to and from original shape (Objektiv) points
	int VertexToPoint(int i) const {return _vertexToPoint[i];}
	int PointToVertex(int i) const {return _pointToVertex[i];}
	int NPoints() const {return _pointToVertex.Size();}

	// conversion between face indices and offsets
	void BuildFaceIndexToOffset() const;
	Offset FaceIndexToOffset( int i ) const {return _faceIndexToOffset[i];}

	const ShapeSection &GetSection(int i) const {return _face._sections[i];}
	ShapeSection &GetSection(int i) {return _face._sections[i];}
	int NSections() const {return _face._sections.Size();}
	void AddSection(const ShapeSection &sec); // add section, respect selection boundaries
	void AddSection
	(
		NamedSelection *sel,
		const Offset *o, int n, const PolyProperties &prop
	); // add sections containing given faces
	
	const Poly &Face( Offset i ) const {return _face[i];}
	Poly &Face( Offset i ) {return _face[i];}

	const Plane &GetPlane( int i ) const {return _plane[i];}
	Plane &GetPlane( int i ) {return _plane[i];}
	void InitPlanes(); // not all shapes need planes
	
	const FaceArray &Faces() const {return _face;}
	FaceArray &Faces() {return _face;}

	Offset BeginFaces() const {return _face.Begin();}
	Offset EndFaces() const {return _face.End();}
	void NextFace( Offset &i ) const {_face.Next(i);}

	Offset FindFace( int i ) const {return _face.Find(i);}
	const Poly &FaceIndexed( int i ) const {return _face[_face.Find(i)];}
	Poly &FaceIndexed( int i ) {return _face[_face.Find(i)];}
	//Poly &AddFace() {return _face[_face.Add()];}
  //! Clears the face array represented by _face member.
	void ClearFaces() {_face.Clear();}
  //! Add one face to the face array.
	void AddFace( const Poly &face ) {_face.Add(face);}
  //! Reserves space for nFaces faces.
  /*!
    \param nFaces Number of faces to reserve space for.
  */
	void ReserveFaces( int nFaces ) {_face.Reserve(nFaces);}
	
	void SetPoints
	(
		const Vector3 *point, const ClipFlags *clip, int nPoints,
		const Vector3 *normal, int nNormals
		//const VertexMesh &mesh
	);
	bool VerifyStructure() const;
	void SetFaces(const FaceArray &src);

	void SetLevel(int level){_level=level;}
	int GetLevel() const {return _level;}

	void MakeCockpit();

	void OrSpecial( int special );
	void AndSpecial( int special );
	void SetSpecial( int special );
	int Special() const {return _special;}

	protected:
	// shape optimization
	//void RemoveFaces( int mustHave, int mustNotHave );
	//void CheckIsolatedPoints( AutoArray<int> &isolated );
	//void RemoveIsolatedPoints( const AutoArray<int> &include,  const AutoArray<int> &exclude );
	//void RemoveIsolatedPoints();

	//void CheckIsolatedNormals( AutoArray<int> &isolated );
	//void RemoveIsolatedNormals( const AutoArray<int> &include );
	//void RemoveIsolatedNormals();


	Shape *ExtractPath(); // optimize this shape as path lod

	//void OptimizeMesh();

	public:
	void MergeFast( const Shape *with, const Matrix4 &transform );
	void Merge( const Shape *with, const Matrix4 &transform );
  //! Reallocates the space of all members according to it's real size.
	void Compact();

	PackedColor GetColor() const {return _color;}
	PackedColor GetColorTop() const {return _colorTop;}
	void CalculateColor(); // calculate average of all texture colors
	void AutoClamp(); // calculate average of all texture colors

	void SurfaceSplit
	(
		const Landscape *land, const Matrix4 &toWorld, float y , float useOrigY
	);
	void Triangulate();

	void UntileTextures(const MTextureMap *mapping, int nMapping);
	void MergeTextures(bool untile);
	void DefineSections(const ParamEntry &cfg);

	void ConvertToVBuffer(VBType type);
	void ReleaseVBuffer();

	void RecalculateAreas();
	void RecalculateNormals( bool full );
	
	void InvalidateNormals();
	void RecalculateNormalsAsNeeded()
	{
		if( !_faceNormalsValid ) RecalculateNormals(true);
	}

	void DeleteFace( int i )
	{
		Offset offset=_face.Find(i);
		_face.Delete(offset);
	}
	//const VertexMesh &Mesh() const {return _mesh;}
	//VertexMesh &Mesh() {return _mesh;}

	// maintain named selections (points and faces)
	int PointIndex( const char *name ) const;
	int FindNamedSel( const char *name ) const;
	int FindNamedSel( const char *name, const char *altName ) const;
	void AddNamedSel( const NamedSelection &sel );
	NamedSelection &NamedSel( int i ) {return _sel[i];}
	const NamedSelection &NamedSel( int i ) const {return _sel[i];}
	int NNamedSel() const {return _sel.Size();}
	const V3 &NamedPosition( const char *name, const char *altName=NULL ) const;

	int NTextures() const {return _textures.Size();}
	Texture *GetTexture( int i ) const {return _textures[i];} // get texture
	int GetTextureIndex(Texture *tex) const {return _textures.Find(tex);}
	float GetAreaOTex( int i ) const {return _areaOTex[i];} // associated with texture

	Texture *FindTexture( const char *name ) const;

	void Draw
	(
		class IAnimator *matSource,
		const LightList &lights,
		ClipFlags clip, int spec,
		const Matrix4 &transform, const Matrix4 &invTransform
	);

	void PrepareTextures( float z2, int special ) const;

  //! Registers specified texture.
	void RegisterTexture( Texture *texture, float areaOTex );
	void RegisterTexture( Texture *texture, Texture *oldTexture );
	void RegisterTexture( Texture *texture, int selection );

	void SetProperty( const char *name, const char *value );
	int FindProperty( const char *name ) const;
	const NamedProperty &NamedProp( int i ) const {return _prop[i];}
	const RStringB &PropertyValue( const char *name ) const;

	private:
	void PreparePhase
	(
		const AnimationPhase *&prevPos, const AnimationPhase *&nextPos, float &interpol,
		float time, float baseTime
	) const;

	public:
	Vector3 PointPhase( int i, float time, float baseTime ) const; // interpolate point position
	void SetPhase( const Selection &anim, float time, float baseTime ); // interpolate between two nearest phases
	void SetPhase( float time, float baseTime ); // interpolate between two nearest phases

	bool IsAnimated() const {return _phase.Size()>1;}
	void SetPhaseIndex( int index );
	int NAnimationPhases() const {return _phase.Size();}

	int PrevAnimationPhase( float time ) const;
	int NextAnimationPhase( float time ) const;
	float AnimationPhaseTime( int index ) const {return _phase[index].Time();}
	const AnimationPhase &GetAnimationPhase( int i ) const {return _phase[i];}

	// destruction of objects
	void MakeDestroyed( float yOffset, Vector3Par bCenter, int seed, float coef=1 );
	void MakeTreeDestroyed( float yOffset, Vector3Par bCenter, int seed, float coef=1 );
	
	// some object have hand defined destructed shape
	// (stored in special shape)
	
	Vector3 CalculateCenter( const Selection &sel ) const; // calculate geometrical center
	Vector3 CalculateCenterOfMass( const Selection &sel ) const; // calculate center of mass

	//void Compact(); // compact all autogrowing arrays to fit exactly contained items
	USE_FAST_ALLOCATOR
};

#include "cutEdges.hpp"

class ConvexComponent: public NamedSelection,public RefCount
{ // used for collision testing
	Vector3 _minMax[2]; // bounding box of the selection - used to accelerate tests
	Vector3 _center;
	float _radius;
	Ref<Shape> _shape;
	AutoArray<int> _planes; // define polyedhron using half spaces
	//bool _filled; // no planes necessary - box is nearly completelly filled
	//bool _dirty;
	SRef<ComponentEdges> _edges;
	Ref<Texture> _texture; // by texture we distinguish some material properties

	public:
	ConvexComponent();
	~ConvexComponent();

	private:
	ConvexComponent( const ConvexComponent &src );
	void operator =( const ConvexComponent &src );

	public:
	void Init( Shape *shape, const char *name );
	Vector3Val Min() const {return _minMax[0];}
	Vector3Val Max() const {return _minMax[1];}
	const Vector3 *MinMax() const {return _minMax;}
	Vector3Val GetCenter() const {return _center;}
	float GetRadius() const {return _radius;}
	Texture *GetTexture() const {return _texture;}

	bool IsInside( Vector3Val point ) const;

	void Recalculate(); // recalc after animation - if dirty

	//bool IsFilled() const {return _filled;}
	//void SetDirty( bool dirty=true ) {_dirty=dirty;}
	//bool GetDirty() const {return _dirty;}

	int NPlanes() const {return _planes.Size();}
	const Plane &GetPlane( int i ) const;
	const Poly &GetFace( int i ) const;
	Shape *GetShape() const {return _shape;}

	ComponentEdges *GetEdges() const {return _edges;}
	void SetEdges( ComponentEdges *edges ) {_edges=edges;}

	USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

//TypeIsMovable(ConvexComponent);

#define MAX_LOD_LEVELS 32

#define LOD_INVISIBLE 127

#define REM_REVERSED 1
#define REM_NOSHADOW 2

//#include "SpecLods.hpp"

#define ALPHA_SPLIT 0
#define N_ALPHA 1
#define for_each_alpha

DECL_ENUM(MapType)

#include <Es/Memory/normalNew.hpp>

class ConvexComponents: public RefCount,public RefArray<ConvexComponent>
{
	mutable bool _valid;

	//private:
	//ConvexComponents( const ConvexComponents &src );
	//void operator =( const ConvexComponents &src );

	public:
	ConvexComponents();
	~ConvexComponents();

	bool RecalculateEdges( Shape *shape );
	void Recalculate( Shape *shape ) const;
	void Validate() {_valid=true;}
	void Invalidate() {_valid=false;}
	void RecalculateAsNeeded( Shape *shape ) const;

	USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

//! collections of shapes for all LOD levels and special levels
/*!
Each LODShape consist of several LOD levels. There are also several special-purpose levels:

Memory (different points used for animation or simulation, usually named)

Geometry (collision geometry)

Fire Geometry (geometry used for bullet/shooting collisions)

View Geometry (geometry used for intersecting line of sight)

Pilot View Geometry (occlusion geometry from pilot's point of view)

Gunner View Geometry (occlusion geometry from gunner's point of view)

Commander View Geometry (occlusion geometry from commander's point of view)

Cargo View Geometry (occlusion geometry from cargo space view)

Land Contact (points used to calculate collision with ground or roadways)

Roadway (roadways used for collisions with entities) 

Paths (patchs used for AI path-finding)

Hitpoints (points defining local dammage)

Each level has number 'resolution' which is describing it funtion - resolution in
case of graphical LOD levels, special (very large) value in case of special levels.
*/

//! tag that is saved with each Oxygen 2 Light Edition model

class LODShape: public RefCountWithLinks
{
	friend class Object;
	friend class LODShapeWithShadow;

	protected:
	// first list most accessed items

	// properties common to all LOD levels
	int _special; //!< some special flags must be applied to whole object (NoFarClip)

	float _boundingSphere; //!< radius of bouding sphere
	float _geometrySphere; //!< radius of bouding sphere around geometry level

	int _remarks; //!< some remarks - e.g. reversed	
	int _andHints,_orHints; //!< and/or of all LOD level hints

	Vector3 _aimingCenter; //!< point at which AI/missiles should aim
	PackedColor _color; //!< shape average color
	PackedColor _colorTop; //!< shape average color when viewed from above
	//! logarithm of average optical density of the shape
	//! used for volumetrical visibility attenuation
	float _viewDensity;

	//! all LOD and special levels
	Ref<Shape> _lods[MAX_LOD_LEVELS];

	//! minmax box of all levels
	Vector3 _minMax[2];
	// we aproximate it as (_min+_max)/2
	//! bounding sphere center of original shape
	/*!
	LODShape is usually adjusted so that bounding sphere is centered around (0,0,0).
	_boundingCenter is offset that is applied during adjusting.
	Therefore -_boundingCenter center is original (0,0,0) positioned in new coordinate system.
	*/
	Vector3 _boundingCenter;
	//! bounding sphere or geometry center
	Vector3 _geometryCenter;

	//! center of mass
	Vector3 _centerOfMass;
	//! matrix of inverse inertia tensor
	Matrix3 _invInertia;
	
	signed char _nLods; //!< number of all (LOD+special) levels 
	signed char _memory; //!< index of memory level
	signed char _geometry; //!< index of geometry level
	signed char _geometryFire; //!< index of fire geometry level
	signed char _geometryView; //!< index of view geometry level

	signed char _geometryViewPilot; //!< index of pilot view geometry level
	signed char _geometryViewGunner; //!< index of gunner view geometry level
	signed char _geometryViewCommander; //!< index of commander view geometry level
	signed char _geometryViewCargo; //!< index of cargo view geometry level

	signed char _landContact; //!< index of land contact level
	signed char _roadway; //!< index of roadway level
	signed char _paths; //!< index of paths level
	signed char _hitpoints; //!< index of hitpoins level

	bool _autoCenter; //!< allow BoundingCenter!=(0,0,0)	
	bool _lockAutoCenter; //!< disable any future changes
	bool _canOcclude; //!< this shape can be used for occlusion culling
	bool _canBeOccluded; //!< this shape should be tested against occlusions

	//! shape is animated - any cache may need to refreshed when drawing
	bool _allowAnimation;

	// often needed propetry values
	RStringB _propertyClass; //!< value of property "class"
	RStringB _propertyDammage; //!< value of property "dammage"

	SizedEnum<MapType,char> _mapType; //!< map symbol used to draw this shape 

	float _resolutions[MAX_LOD_LEVELS]; //!< resolutions of all levels
	//! array of mass assigned to all points of geometry level
	//! this is used to calculate angular inertia tensor (_invInertia)
	AutoArray<float> _massArray;

	//! list of convex components from geometry level
	mutable Ref<ConvexComponents> _geomComponents;
	//! list of convex components from view geometry level
	mutable Ref<ConvexComponents> _viewComponents;
	//! list of convex components from fire geometry level
	mutable Ref<ConvexComponents> _fireComponents;

	//! list of convex components from pilot view geometry level
	mutable Ref<ConvexComponents> _viewPilotComponents;
	//! list of convex components from gunner view geometry level
	mutable Ref<ConvexComponents> _viewGunnerComponents;
	//! list of convex components from commander view geometry level
	mutable Ref<ConvexComponents> _viewCommanderComponents;
	//! list of convex components from cargo view geometry level
	mutable Ref<ConvexComponents> _viewCargoComponents;


	//! initialize given component list from given level
	//! create component list if neccessary
	//! use #InitConvexComponent to do real work
	void InitCC( Ref<ConvexComponents> &cc, Shape *shape );
	
	//! source file name
	RStringB _name;
	// physical body constants
	float _mass; //!< total mass
	float _invMass; //!< total mass inverse

	float _armor; //!< armor (dammage resistance) value
	float _invArmor; //!< inverse armor value
	float _logArmor; //!< logarithm of armor value

	public:
	__forceinline PackedColor Color() const {return _color;} //!< access to #_color
	__forceinline PackedColor ColorTop() const {return _colorTop;} //!< access to #_

	__forceinline float Mass() const {return _mass;} //!< access to #_mass
	__forceinline float InvMass() const {return _invMass;} //!< access to #_invMass
	__forceinline float Armor() const {return _armor;} //!< access to #_armor
	__forceinline float InvArmor() const {return _invArmor;} //!< access to #_invArmor
	__forceinline float LogArmor() const {return _logArmor;} //!< access to #_logArmor

	__forceinline bool CanOcclude() const {return _canOcclude;} //!< access to #_canOcclude
	__forceinline bool CanBeOccluded() const {return _canBeOccluded;} //!< access to #_canBeOccluded

	__forceinline float ViewDensity() const {return _viewDensity;} //!< access to #_viewDensity
	Texture *FindTexture( const char *name ) const;

	void SetCanOcclude( bool val ) {_canOcclude=val;} //!< access to #_canOcclude
	void SetCanBeOccluded( bool val ) {_canBeOccluded=val;} //!< access to #_canBeOccluded

	Matrix3 InvInertia() const {return _invInertia;} //!< access to #_invInertia
	Vector3Val CenterOfMass() const {return _centerOfMass;} //!< access to #_centerOfMass

	//! access to #_geomComponents
	const ConvexComponents &GetGeomComponents() const {return *_geomComponents;}
	//! access to #_viewComponents
	const ConvexComponents &GetViewComponents() const {return *_viewComponents;}
	//! access to #_fireComponents
	const ConvexComponents &GetFireComponents() const {return *_fireComponents;}

	//! access to #_viewPilotComponents
	const ConvexComponents *GetViewPilotComponents() const {return _viewPilotComponents;}
	//! access to #_viewGunnerComponents
	const ConvexComponents *GetViewGunnerComponents() const {return _viewGunnerComponents;}
	//! access to #_viewCommanderComponents
	const ConvexComponents *GetViewCommanderComponents() const {return _viewCommanderComponents;}
	//! access to #_viewCargoComponents
	const ConvexComponents *GetViewCargoComponents() const {return _viewCargoComponents;}

	void FindHitComponents(FindArray<int> &hits, const char *name) const;

	void InvalidateConvexComponents(int level);
	void RecalculateConvexComponentsAsNeeded(int level);
	ConvexComponents *GetConvexComponents(int level) const;

	bool CheckLegalCreator() const;

	protected:
	void DoClear();
	void DoConstruct();
	void DoConstruct( const LODShape &src, bool copyAnimations );
	void DoDestruct();

	// normal p3d file handling
	//! load from named file
	void Load( const char *name, bool reversed );
	//! load from stream
	void Load( QIStream &f, bool reversed );

	//! prepare properties from config file
	void PrepareProperties(const ParamEntry &cfg);
	
	// fast file handling
	void Reverse();
	//! load from / save to optimized binary file
	//!\return true if loaded OK
	void SerializeBin(SerializeBinStream &f);
	
	//! initialize all convex componets (#_geomComponents etc.)
	void InitConvexComponents();
	//! initialize single convex componet set
	void InitConvexComponents( ConvexComponents &cc, Shape *geom );
		
	public:

	//! load from optimized binary file
	bool LoadOptimized(const char *name); // true if OK
	//! load from optimized binary stream
	bool LoadOptimized(QIStream &f); // true if OK

	//! save to optimized binary file
	void SaveOptimized(const char *name); // true if OK
	//! save to optimized binary stream
	void SaveOptimized(QOStream &f); // true if OK

	//! constructor - construct empty shape
	LODShape();
	//! copy shape (with or without keyframe animations)
	LODShape( const LODShape &src, bool copyAnimations=true );
	//! copy shape (with all keyframe animations)
	void operator = ( const LODShape &src );
	//! destructor
	~LODShape();

	//! construct by loading from stream
	LODShape( QIStream &f, bool reversed );
	//! construct by loading from file
	LODShape( const char *name, bool reversed );

	//! reload shape from file with name stored in #_name
	void Reload( QIStream &f, bool reversed );

	// access functions common to all LODs
	int Special() const {return _special;}
	void OrSpecial( int special );
	void AndSpecial( int special );
	void SetSpecial( int special );
	void RescanSpecial();

	int Remarks() const {return _remarks;}
	void SetRemarks( int remarks ) {_remarks=remarks;}

	void CalculateHints();
	void SetHints( ClipFlags orHints, ClipFlags andHints ){_orHints=orHints,_andHints=andHints;}
	ClipFlags GetOrHints() const {return _orHints;}
	ClipFlags GetAndHints() const {return _andHints;}

	void LockAutoCenter( bool autoCenter ) {_lockAutoCenter=autoCenter;}
	void SetAutoCenter( bool autoCenter ) {_autoCenter=autoCenter;}
	void AllowAnimation( bool allow=true ) {_allowAnimation=allow;} // non static usage detected
	bool GetAllowAnimation() const {return _allowAnimation;} // non static usage detected

	void OptimizeRendering();
	void CalculateBoundingSphere();
	void CalculateBoundingSphereRadius();
	void CalculateMinMax( bool recalcLevels=false );
	void DefineMinMax( int level );
	void CheckForcedProperties();
	void ScanProperties();
	void CalculateMass(); // always use best level

	void RecalculateGeomComponents() const {_geomComponents->Recalculate(GeometryLevel());}
	void InvalidateGeomComponents() {_geomComponents->Invalidate();}
	void RecalculateGeomComponentsAsNeeded() const {_geomComponents->RecalculateAsNeeded(GeometryLevel());}

	void InvalidateFireComponents() {_fireComponents->Invalidate();}
	void InvalidateViewComponents() {_viewComponents->Invalidate();}
	
	LODShape *MakeShadow();

	// properties
	Vector3Val Min() const {return _minMax[0];}
	Vector3Val Max() const {return _minMax[1];}
	const Vector3 *MinMax() const {return _minMax;}
	float BoundingSphere() const {return _boundingSphere;}
	Vector3Val BoundingCenter() const {return _boundingCenter;}
	float GeometrySphere() const {return _geometrySphere;}
	Vector3Val GeometryCenter() const {return _geometryCenter;}
	Vector3Val AimingCenter() const {return _aimingCenter;}
	const char *Name() const {return _name;}

	const RStringB &GetName() const {return _name;}

	const RStringB &PropertyValue( const char *name ) const;
	MapType GetMapType() const {return _mapType;}
	const RStringB &GetPropertyClass() const {return _propertyClass;}
	const RStringB &GetPropertyDammage() const {return _propertyDammage;}

	void InternalTransform( const Matrix4 &transform ); // transform all LODs
	void Translate( Vector3Par offset ); // transform all LODs

	void RegisterTexture( Texture *texture, const Animation &anim );

	// LOD maintenance
	Shape *LevelOpaque( int level ) const
	{
		Assert( level>=0 && level<NLevels() );
		return _lods[level];
	}
	Shape *Level( int level ) const
	{
		Assert( level>=0 && level<NLevels() );
		return _lods[level];
	}
	int NLevels() const {return _nLods;}
	float Resolution( int level ) const {return _resolutions[level];}
	void SetResolution( int level, float res ) {_resolutions[level]=res;}

	#define G__Level(level) ( level>=0 ? _lods[level].GetRef() : NULL )

	Shape *MemoryLevel() const {return G__Level(_memory);}
	Shape *GeometryLevel() const {return G__Level(_geometry);}
	Shape *FireGeometryLevel() const {return G__Level(_geometryFire);}
	Shape *ViewGeometryLevel() const {return G__Level(_geometryView);}

	Shape *ViewPilotGeometryLevel() const {return G__Level(_geometryViewPilot);}
	Shape *ViewGunnerGeometryLevel() const {return G__Level(_geometryViewGunner);}
	Shape *ViewCommanderGeometryLevel() const {return G__Level(_geometryViewCommander);}
	Shape *ViewCargoGeometryLevel() const {return G__Level(_geometryViewCargo);}

	Shape *LandContactLevel() const {return G__Level(_landContact);}
	Shape *RoadwayLevel() const {return G__Level(_roadway);}
	Shape *PathsLevel() const {return G__Level(_paths);}
	Shape *HitpointsLevel() const {return G__Level(_hitpoints);}

	int FindMemoryLevel() const {return _memory;}
	int FindGeometryLevel() const {return _geometry;}
	int FindFireGeometryLevel() const {return _geometryFire;}
	int FindViewGeometryLevel() const {return _geometryView;}

	int FindViewPilotGeometryLevel() const {return _geometryViewPilot;}
	int FindViewGunnerGeometryLevel() const {return _geometryViewGunner;}
	int FindViewCommanderGeometryLevel() const {return _geometryViewCommander;}
	int FindViewCargoGeometryLevel() const {return _geometryViewCargo;}

	int FindLandContactLevel() const {return _landContact;}
	int FindRoadwayLevel() const {return _roadway;}
	int FindPaths() const {return _paths;}
	int FindHitpoints() const {return _hitpoints;}

	const V3 &NamedPoint( int level, const char *name, const char *altName=NULL ) const;
	const V3 &MemoryPoint( const char *name, const char *altName=NULL ) const;
	bool MemoryPointExists( const char *name ) const;

	int FindLevel( float resolution, bool noDecal=false ) const;
	int FindSpecLevel( float resolution ) const;
	bool IsSpecLevel( int level, float resolution ) const;

	//! Check if point is inside the shape geometry
	bool IsInside( Vector3Par pos ) const;

	//int FindGeometryLevel() const;
	//int FindLandContactLevel() const;

	int FindSqrtLevel( float resolution2, bool noDecal=false ) const;
	int FindNearestWithoutProperty( int level, const char *property ) const;
	int FindNearestWithProperty( int level, const char *property ) const;
	void AddShape( Shape *shape, float resolution );
	void ChangeShape( int level, Shape *shape );

	void Optimize(); // sort by texture/render state

	void ScanShapes();
	void OptimizeShapes();

};

#include <Es/Memory/normalNew.hpp>

//! LOD shape able to calculate / store shadow shapes

class LODShapeWithShadow: public LODShape
{
	private:
	Ref<LODShape> _shadow;

	Ref<LODShape> _destroyed;
	Ref<LODShape> _destroyedShadow;
	//bool _isVehicle; // is there and associated vehicle type?

	public:
	LODShapeWithShadow();
	LODShapeWithShadow
	(
		const char *name, bool reversed=false
	);
	LODShapeWithShadow
	(
		QIStream &f, bool reversed=false
	);

	LODShape *Shadow() {return _shadow;}
	void CreateShadow();
	void ShadowChanged(){_shadow.Free();}

	LODShape *MakeDestroyed( float coef=1 ); // building/engine type
	LODShape *MakeTreeDestroyed( float coef=1 ); // tree type
	
	USE_FAST_ALLOCATOR;
};

#include <Es/Memory/debugNew.hpp>

#include <Es/Containers/bankArray.hpp>

//template BankTraits<LODShapeWithShadow>;

template<>
struct BankTraits<LODShapeWithShadow>
{
	// default name is character
	typedef const RStringB &NameType;
	// default name comparison
	static int CompareNames(NameType n1, NameType n2)
	{
		return n1!=n2;
	}

	typedef LinkArray<LODShapeWithShadow> ContainerType;
};

class ShapeBank: public BankArray<LODShapeWithShadow>
{
	public:
	LODShapeWithShadow *New
	(
		const char *name, bool reversed, bool shadow
	);
	void OptimizeAll();
	void OptimizeOneShape(LODShape *shape);
	void ReleaseAllVBuffers();
};

extern ShapeBank Shapes;


#endif
