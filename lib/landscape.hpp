#ifdef _MSC_VER
#pragma once
#endif

#ifndef _LANDSCAPE_HPP
#define _LANDSCAPE_HPP

#include "object.hpp"
#include <El/ParamArchive/serializeClass.hpp>
#include "vehicle.hpp"
#include "weapons.hpp"
#include "location.hpp"
#include "scene.hpp"
#include "paramFileExt.hpp"

#include "scenery.hpp"

#include "system\enf_mutex.h"

#if _ARMA3_SIMUL_WEATHER
#include "SimulWeather/SimulWeatherBase.hpp"
#endif

#if _VBS3_LARGE_OBJECTS_WRP || _VBS3_LARGE_OBJECTS
#include <Es/Containers/RTree.hpp>
#endif

#define LandRange (GLandscape->GetLandRange())
#define LandRangeMask (GLandscape->GetLandRangeMask())
#define LandRangeLog (GLandscape->GetLandRangeLog())
#define InvLandRange (GLandscape->GetInvLandRange())

#define TerrainRange (GLandscape->GetTerrainRange())
#define TerrainRangeMask (GLandscape->GetTerrainRangeMask())
#define TerrainRangeLog (GLandscape->GetTerrainRangeLog())

#define LandGrid (GLandscape->GetLandGrid())
#define InvLandGrid (GLandscape->GetInvLandGrid())

#define TerrainGrid (GLandscape->GetTerrainGrid())
#define InvTerrainGrid (GLandscape->GetInvTerrainGrid())

#define LandSize (LandRange*LandGrid)
#define InvLandSize (InvLandRange*InvLandGrid)

// caution: many functions assume ObjRange==LandRange, ObjGrid==LandGrid

#define ObjRange (LandRange)
#define ObjGrid (LandGrid)
#define InvObjGrid (InvLandGrid)

/// land segment (mesh item) size
//const int LandSegmentSize = 8;
//const float InvLandSegmentSize=1.0/LandSegmentSize;


// trick: instead of four comparisons use logical operations
// works if LandRange is power of 2
// following lines are equivalent
//#define InRange(z,x) ( z>=0 && x>=0 && z<LandRange && x<LandRange )
#define InRange(z,x) ((((z)|(x))&~LandRangeMask)==0)

#define TerrainInRange(z,x) ((((z)|(x))&~TerrainRangeMask)==0)

#define ObjInRange(z,x) ((((z)|(x))&~(ObjRange-1))==0)


#define LANDDATA_SCALE_WRP (0.03f*1.5f)

#define LANDDATA_SCALE LANDDATA_SCALE_WRP

#define TACTICAL_VISIBILITY (Glob.config.tacticalZ)
#define RADAR_VISIBILITY (Glob.config.radarZ)

struct VisCheckContext;
struct CheckObjectCollisionContext;

class SerializeBinStream;

struct GroundLayerInfo;

class Landscape;
class LandSegment;
class WaterSegment;
class IOperCache;
class ILockCache;
class ISuppressCache;
class ObjectRectIndex;
class ObjectListCache;
class MapObjectRectIndex;
class MapObjectListCache;
class LandCache;
class Landscape;


/// compute extents in LandGrid coordinates for a sphere moving from oPos to nPos
void ObjRadiusRectangle(int &xMin, int &xMax, int &zMin, int &zMax, Vector3Par oPos, Vector3Par nPos, float radius);
/// compute extents in LandGrid coordinates for a sphere
void ObjRadiusRectangle(int &xMin, int &xMax, int &zMin, int &zMax, Vector3Par nPos, float radius);

/// compute extents in LandGrid coordinates for a rectangle
void ObjRectangle(int &xMin, int &xMax, int &zMin, int &zMax, Vector3Par minC, Vector3Par maxC, float radius);

/// compute extents in LangGrid coordinates for a GIVEN rectangle to use while drawing objects on map
void ObjMapRadiusRectangle(int &xMin, int &xMax, int &zMin, int &zMax);

#include <El/Rectangle/rectangle.hpp>

#if !_MSC_VER
  #ifndef __INTEL_COMPILER
  #pragma warning 549 10
  #endif
#endif

#include <Es/Memory/normalNew.hpp>


/// information about static object group, can be used for instancing + multi-object decisions
struct ObjectListGroup
{
  /// after the last object in the group
  /** note: each groups starts where the previous has ended */
  int _end;
  /// max. area for a given group
  /**
  Note: this is max. Area can differ slightly based on scale
  */
  float _maxArea;
};

TypeIsSimple(ObjectListGroup)

/// it has no sense to consider roadways of very small objects
/** done as optimization of small bullet impact "crater" handling */
static const float IgnoreSmallRoadways = 0.2f;

/// a list of objects in one landscape grid, used when the grid is non-empty
class ObjectListFull: public LinkBidirWithFrameStore, public RefArray<Object>
{
  friend class ObjectList;
  friend class Landscape;

  
  typedef RefArray<Object> base;

  //@{ bounding volume of static objects
  Vector3 _minMax[2];
  Vector3 _bCenter;
  float _bRadius;
  //@}
  /// how many non-static objects are included
  short _nNonStatic;
  /// how many object with roadways are included
  short _nRoadways;
  /// while list is used, it cannot be released
  int _used;
  /// max. est. area of each of the static objects - used for pixel coverage estimation
  float _maxStaticEstArea;
  /// how much memory is used by discardable objects (used to optimize GetMemoryControlled call)
  int _memoryUsedByDiscardable;

  /// some objects may be grouped
  AutoArray<ObjectListGroup> _groups;
  
private:
  int CountNonStatic() const;
  int CountRoadways() const;
  /// recalculate what is necessary, assume values were valid
  void AddStaticBoundingVolume(Object *obj);

public:
  ObjectListFull( int x, int z );
  ~ObjectListFull();

  __forceinline int GetNonStaticCount() const {return _nNonStatic;}
  __forceinline int GetRoadwayCount() const {return _nRoadways;}
  /// count streaming (primary) objects
  int CountPrimary() const;

  void ChangeNonStaticCount(int val);
  void ChangeRoadwayCount(int val);
  void StaticChanged(); // recalculate what is necessary
  void StaticAdded(Object *obj); // recalculate what is necessary
  void SetBSphere(Vector3Par center, float radius, const Vector3 *minmax);
  void SetBSphere(int x, int z);

  /// update group information (_groups, _startNoGroup)
  void UpdateGroups();

  //@{ access groups
  int NGroups() const {return _groups.Size();}
  const ObjectListGroup &Group(int i) const {return _groups[i];}
  //@}
  
  __forceinline Vector3Val GetBSphereCenter() const {return _bCenter;}
  __forceinline float GetBSphereRadius() const {return _bRadius;}
  __forceinline const Vector3 *GetMinMax() const {return _minMax;}
  __forceinline float GetMaxStaticArea() const {return _maxStaticEstArea;}

  // only add/remove methods
  int Add(Object *object, int x, int z, bool avoidRecalculation = false);
  void Delete(int index);
  using LinkBidirWithFrameStore::Delete;
  void Clear();
  
  /// get x,z coordinates based on data inspection
  void GetXZ(int &x, int &z);
  
  __forceinline int GetUsedCount() const {return _used;}
  //! check if list is currently used
  __forceinline bool IsUsed() const {return _used>0;}
  //! lock counting
  __forceinline int AddRef() {return ++_used;}
  //! lock counting
  int Release()
  {
    if (--_used == 0)
    {
      int x, z;
      DoRelease(x, z);
    }
    return _used;
  }
  //! lock counting, return coordinates
  int Release(int &x, int &z)
  {
    if (--_used == 0)
      DoRelease(x, z);
    return _used;
  }
  //! physical release
  void DoRelease(int &x, int &z);
  size_t GetMemoryControlled() const;

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

//! pointer to object list serving as object list
//! used in landscape object index - many ObjectLists are be empty

class ObjectList
{
  SRef<ObjectListFull> _list;

public:
  ObjectList() {}
  ObjectList(ObjectListFull *list) {_list = list;}

  //! check size, guaranteed to be non-empty
  __forceinline int SizeNotEmpty() const {return _list->Size();}
  //! check size, guaranteed to be non-empty
  int Size() const {return _list ? _list->Size() : 0;}
  //! get given object
  __forceinline Object *operator[](int i) const {return _list->Get(i);}
  //! recalculate what is necessary
  void Recalculate() const
  {
    if (_list)
      _list->StaticChanged();
  }
  bool IsRoadway() const
  {
    return _list && _list->_nRoadways != 0;
  }
  /// check if the list can be deleted
  bool CanBeDeleted() const
  {
    return _list->_used == 0 && _list->Size() == 0;
  }
  //! get underlying object list
  ObjectListFull *GetList() const {return _list;}
  //! get underlying object list
  ObjectListFull *operator->() const {return _list;}
  //! check if there is some list
  bool Null() const {return _list.IsNull();}

  __forceinline const Vector3 GetMin() const {return _list ? _list->GetMinMax()[0]:VZero;}
  __forceinline const Vector3 GetMax() const {return _list ? _list->GetMinMax()[1]:VZero;}
#if 0 //_DEBUG
    ~ObjectList()
    {
      Assert(!_list || !_list->IsUsed());
    }
#endif
};

TypeIsMovable(ObjectList)

//! used to keep ObjectList loaded while necessary
/**
This object should be used to get read-only access.
If list is empty by the time ObjectListUsed is created,
it is considered to be empty during its whole lifetime.
*/
class ObjectListUsed
{
  friend class Landscape;
  ObjectListFull *_list;
  bool _used;
  Landscape *_land; //!< pointer to list that should be kept loaded
  int _x; //!< coordinates of loaded list
  int _z; //!< coordinates of loaded list

  //! mark list as locked and autorelease when it is no longer needed
protected:
  ObjectListUsed(const ObjectList &list, bool cache, Landscape *land, int x, int z);

public:
  ~ObjectListUsed();

  ObjectListUsed(const ObjectListUsed &src);
  void operator =(const ObjectListUsed &src);

  __forceinline int Size() const {return _list ? _list->Size() : 0;}
  __forceinline int SizeNotEmpty() const {return _list->Size();}
  
  __forceinline Object *operator[](int index) const {return (*_list)[index];}
  __forceinline bool IsNull() const {return _list==NULL;}
  __forceinline ObjectListFull *operator->() const {return _list;}
  
  __forceinline int GetRoadwayCount() const {return _list ? _list->GetRoadwayCount() : 0;}
  __forceinline ObjectListFull *GetList() const {return _list;}
  //__forceinline const ObjectList &GetList() const {return *_list;}
  //__forceinline operator const ObjectList &() const {return *_list;}
};

#if _VBS3_LARGE_OBJECTS
class ObjectListUsedXXL
{
  const ObjectListUsed &_list;
  RefArray<Object>      _largeObj;
public:
  ObjectListUsedXXL(const ObjectListUsed &l, int x, int z);
  __forceinline int Size() const { return _list.Size()+_largeObj.Size(); }

  __forceinline Object *operator[](int index) const {return ( index<_list.Size() ) ? (Object *)_list[index] : (Object *)_largeObj[index-_list.Size()]; }
  __forceinline bool IsNull() const {return _list.IsNull() && !_largeObj.Size();}

  __forceinline int GetRoadwayCount() const {return _list.GetRoadwayCount();}
};
#else
typedef ObjectListUsed ObjectListUsedXXL;
#endif

#if 0 // FIXME: obsolete, remove completely?
//! used to keep ObjectList loaded while necessary
/*! note: rect used needs to store which fields are locked,
 as object lists may become empty or nonempty while used
*/
class ObjectListRectUsed
{
  // which landscape is this lock in
  const Landscape *_land;
  //!< coordinates of loaded landscape part
  int _xBeg,_zBeg,_xEnd,_zEnd;
  Array2D<bool> _used;
public:
  //! mark list as locked and autorelease when it is no longer needed
  ObjectListRectUsed(const Landscape *land, int xBeg,int zBeg,int xEnd,int zEnd, bool cache) {Fail("Not implemented");}
  ~ObjectListRectUsed() {Fail("Not implemented");}
  ObjectListRectUsed(const ObjectListRectUsed &src)  {Fail("Not implemented");}
  void operator=(const ObjectListRectUsed &src) {Fail("Not implemented");}
};
#endif

//////////////////////////////////////////////////////////////////////////
//
// Map Object List
//
#include "mapObject.hpp"

#include <Es/Memory/normalNew.hpp>
class MapObjectListFull: public LinkBidirWithFrameStore, public AutoArray< SRef<MapObject> >
{
protected:
  friend class MapObjectList;

  typedef AutoArray< SRef<MapObject> > base;
  int _used; // while list is used, it cannot be released

public:
  MapObjectListFull() :_used(0) {}
  ~MapObjectListFull();

  // only add/remove methods
  int Add(MapObject *object) {return base::Add(object);}
  using LinkBidirWithFrameStore::Delete;
  void Delete(int index) {base::Delete(index);}
  void Clear()
  {
    Assert(_used==0);
    base::Clear();
  }
  
  //! check if list is currently used
  bool IsUsed() const {return _used > 0;}
  //! check how many times is the list currently used
  int GetUseCount() const {return _used;}
  //! lock counting
  int AddRef()
  {
    Assert(Size() > 0);
    return ++_used;
  }
  //! lock counting
  int Release()
  {
    if (--_used==0)
      Clear();
    return _used;
  }
  size_t GetMemoryControlled() const;

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

//! pointer to map object list serving as map object list
//! used in landscape map object index - many MapObjectLists are be empty
class MapObjectList
{
protected:
  SRef<MapObjectListFull> _list;

public:
  //! check size, guaranteed to be non-empty
  int SizeNotEmpty() const {return _list->Size();}
  //! check size, guaranteed to be non-empty
  int Size() const {return _list ? _list->Size() : 0;}
  //! get given object
  MapObject *operator[](int i) const
  {
    Assert(_list);
    return _list->Get(i);
  }
  //! add object
  int Add(MapObject *object)
  {
    if (!_list)
      _list = new MapObjectListFull();
    return _list->Add(object);
  }
  //! delete given object
  void Delete(int index)
  {
    Assert(_list);
    _list->Delete(index);
    if (_list->Size() == 0)
      OnEmpty();
  }
  //! compact memory usage
  void Compact()
  {
    if (_list)
    {
      _list->Compact();
      if (_list->Size() == 0)
        OnEmpty();
    }
  }
  //! release all memory
  void Clear()
  {
    _list.Free();
  }
  //! say this list is needed
  int AddRef();

  //! list is empty and might be deleted
  void OnEmpty();

  //! get underlying object list
  MapObjectListFull *GetList() const {return _list;}
  //! get underlying object list
  MapObjectListFull *operator ->() const {return _list;}
  //! check if there is some list
  bool Null() const {return _list==NULL;}

#if _DEBUG
  ~MapObjectList() {Assert(!_list || !_list->IsUsed());}
#endif
};
TypeIsMovable(MapObjectList)

//! used to keep MapObjectList loaded while necessary
class MapObjectListUsed
{
  friend class Landscape;
protected:
  Landscape *_land; //!< pointer to list that should be kept loaded
  int _x; //!< coordinates of loaded list
  int _z; //!< coordinates of loaded list
  MapObjectListFull *_list;
  bool _used;

  //! mark list as locked and autorelease when it is no longer needed
  MapObjectListUsed(const MapObjectList &list, bool cache, Landscape *land, int x, int z);
public:
  ~MapObjectListUsed();

  MapObjectListUsed(const MapObjectListUsed &src);
  void operator =(const MapObjectListUsed &src);

  const MapObjectListFull *GetList() const {return _list;}
};

//! used to keep ObjectList loaded while necessary
/*!
functionality similar to ObjectListRectUsed, but there is no need to store _used,
as lists do not change and cannot become empty or non-empty during "used" lifetime.
*/
class MapObjectListRectUsed
{
  friend class Landscape;
protected:
  // which landscape is this lock in
  const Landscape *_land;
  //!< coordinates of loaded landscape part
  GridRectangle _rect;

  //! mark list as locked and autorelease when it is no longer needed
  MapObjectListRectUsed(const Landscape *land, int xBeg,int zBeg,int xEnd,int zEnd, bool cache);

public:
  //! default constructor - no lock
  MapObjectListRectUsed();
  //! mark list as locked and autorelease when it is no longer needed
  ~MapObjectListRectUsed();

  MapObjectListRectUsed(const MapObjectListRectUsed &src);
  void operator =(const MapObjectListRectUsed &src);

  //! check integrity, provided number of some other external references is known
  bool CheckRefCount(int addCount) const;
  //! check basic integrity
  bool CheckIntegrity() const {return CheckRefCount(0);}
};

/// Structure to store clutter geometry and properties
struct Clutter
{
  /// config entry name (always lowercase)
  RStringB _name;
  /// Object - one instance is enough, no need to create again and again
  Ref<Object> _obj;
  /// Geometry
  Ref<LODShape> _shape;
  //@{ Dynamic wind + flattening, based on scale of individual instances
  /// Determines the affection of the clutter to the wind (0 - stone, 1 - common grass)
  float _noWindAffectionScale;
  float _fullWindAffectionScale;
  /// Determines the affection of the clutter by flattening (by men or vehicles)
  float _noFlattenScale;
  float _fullFlattenScale;
  //@}
  /// minimal scale
  float _scaleMin;
  /// maximal scale
  float _scaleMax;
  /// small clutter can disappear a lot sooner than large one
  float _startDisappearDist,_endDisappearDist;
  /// alpha clutter should to be sorted back to front
  bool _isAlpha;
  /// should coloring based on surface be performed?
  bool _isColored;
  /// the surface color this clutter was prepared for
  Color _surfColor;
};

TypeIsMovableZeroed(Clutter)

const int NClouds = 4;

DEFINE_ENUM_BEG(GroundType)
    GroundSolid,GroundWater
DEFINE_ENUM_END(GroundType)

//! information about collision with ground
struct UndergroundInfo
{
  //! texture (determines surface)
  const Texture *texture;
  //! object we are in contact with (may be null - contact with terrain)
  const Object *obj;
  Vector3 pos; //!< world coordinate position of collision
  float under; //!< how much are we under the ground level
  float dX,dZ; //!< surface differential
  int vertex; //!< which vertex of checked object
  int level; //!< which level (landcontact or geometry)
  GroundType type; // type of collision (water, solid...)
};

TypeIsSimple(UndergroundInfo);


inline float ShortToHeight(short val) {return val * LANDDATA_SCALE;}
inline short HeightToShort(float val) {return toInt(val * (1.0f / LANDDATA_SCALE));}

/// collision with ground
class GroundCollisionBuffer: public AutoArray<UndergroundInfo, MemAllocDataStack<UndergroundInfo,512,AllocAlign16> >
{
};

struct VehicleCollision
{
  const EntityAI *who;
  Vector3 pos;
  /// time when the vehicle is closest to us
  float time;
  /// distance between the vehicle collision spheres at given time
  float distance;
  // what is the oriented "x separation" (distance of the object centers in the sense of the x direction) at given time
  float xSeparation;
};

TypeIsSimple(VehicleCollision)

/// vehicle - vehicle collision (used for collision prediction)
class VehicleCollisionBuffer: public AutoArray<VehicleCollision, MemAllocDataStack<VehicleCollision,128,AllocAlign16> >
{
public:
  int Find(const EntityAI *who) const
  {
    for (int i=0; i<Size(); i++)
    {
      if (Get(i).who==who)
        return i;
    }
    return -1;
  }
};

#include <Es/Memory/normalNew.hpp>
#include <El/Common/randomGen.hpp>

ObjectListCache *CreateListCache(Landscape *land);
MapObjectListCache *CreateMapListCache(Landscape *land);

class VisitorObjId;

struct EditorSelectionItem
{
  VisitorObjId id;
  mutable Vector3 pos;
};
TypeIsMovableZeroed(EditorSelectionItem)

class RoadNet;

class EntityAIFull;
class EntityAI;

struct QuadTreeExTraitsObjectList
{
  enum {LogSizeX = 0};
  enum {LogSizeY = 0};
  typedef ObjectList Type;
  static bool IsEqual(const ObjectList &a, const ObjectList &b) {return a.GetList() == b.GetList();}
  static const ObjectList &Get(const ObjectList &value, int x, int y)
  {
    // Assert(x >= 0 && x < (1 << LogSizeX));
    // Assert(y >= 0 && y < (1 << LogSizeY));
    Assert(x == 0);
    Assert(y == 0);
    return value;
  }
  static void Set(ObjectList &value, int x, int y, const ObjectList &item)
  {
    // Assert(x >= 0 && x < (1 << LogSizeX));
    // Assert(y >= 0 && y < (1 << LogSizeY));
    Assert(x == 0);
    Assert(y == 0);
    value = item;
  }
};

struct QuadTreeExTraitsMapObjectList
{
  enum {LogSizeX = 0};
  enum {LogSizeY = 0};
  typedef MapObjectList Type;
  static bool IsEqual(const MapObjectList &a, const MapObjectList &b) {return a.GetList() == b.GetList();}
  static const MapObjectList &Get(const MapObjectList &value, int x, int y)
  {
    // Assert(x >= 0 && x < (1 << LogSizeX));
    // Assert(y >= 0 && y < (1 << LogSizeY));
    Assert(x == 0);
    Assert(y == 0);
    return value;
  }
  static void Set(MapObjectList &value, int x, int y, const MapObjectList &item)
  {
    // Assert(x >= 0 && x < (1 << LogSizeX));
    // Assert(y >= 0 && y < (1 << LogSizeY));
    Assert(x == 0);
    Assert(y == 0);
    value = item;
  }
};

class Target;

/// minimal info needed to keep information about a static entity
struct StaticEntityLink: public OLinkL(Object)
{
  RefR<const EntityAIType,const RefCount> _type;
  Vector3 _pos;

  void SetLink(const OLinkL(Object) &obj) {OLinkL(Object)::operator = (obj);}
  
  mutable RefR<Target> _virtualTarget;
  
  Target *GetVirtualTarget() const;
  void ClearVirtualTarget() const;
};
TypeContainsOLink(StaticEntityLink)

template <>
struct FindArrayKeyTraits<StaticEntityLink>
{
  typedef ObjectId KeyType;
  static bool IsEqual(KeyType a, KeyType b) {return a==b;}
  static KeyType GetKey(const StaticEntityLink &a) {return a.GetId();}
};


/// memory-friendly list, listing only types and links
class StaticEntityList
{
  friend class World;

  FindArrayKey<StaticEntityLink> _list;

public:
  void Clear();

  /// add to the list
  void Insert(const ObjectId &id, const EntityAIType *type, Vector3Par pos);

  /// delete from the list
  void Remove(const ObjectId &id);

  void Realloc(int size) {_list.Realloc(size);}
  void Resize0() {_list.Resize(0);}
  void Compact() {_list.Compact();}
  
  int Size() const;

  const StaticEntityLink &GetInfo(int index) const {return _list[index];}
  OLinkL(Object) Get(int index) const;
  OLinkL(Object) Find(const ObjectId &id) const;
  const StaticEntityLink *FindInfo(const ObjectId &id) const;
  int FindIndex(const ObjectId &id) const {return _list.FindKey(id);}
};

/// functor useful for ForSome iterations
class DetectInRectangle
{
  int _xBeg,_zBeg;
  int _xEnd,_zEnd;
  
public:
  DetectInRectangle(int xBeg,int zBeg, int xEnd, int zEnd)
  {
    _xBeg = xBeg;
    _zBeg = zBeg;
    _xEnd = xEnd;
    _zEnd = zEnd;
  }
  bool operator()(int xBeg, int zBeg, int xSize, int zSize) const
  {
    int xEnd = xBeg + xSize;
    int zEnd = zBeg + zSize;
    return _xEnd > xBeg && xEnd > _xBeg && _zEnd > zBeg && zEnd > _zBeg;
  }
};

/// environmental sound type - used for _soundMap
enum EnvType
{
  EnvSea,
  EnvTrees,
  EnvMeadow,
  EnvHouses,
  NEnvType,
};

#if defined _MSC_VER && defined _M_PPC
// X360 bitfield layout is by default different (msb_to_lsb, common on PPC)
// we want the same as used on x86, i.e. lsb_to_msb
# pragma bitfield_order(push, lsb_to_msb)
#endif

/// environmental type stats for landscape field
union EnvTypeInfo
{
	unsigned packed;
	struct
	{
		unsigned sea:2;
		unsigned trees:2;
		unsigned meadow:2;
		unsigned houses:2;
	} u;
	
	explicit EnvTypeInfo(unsigned val):packed(val){}
	unsigned short GetPacked() const {return packed;}
	
	float GetEnvType(EnvType type) const
	{
	  switch (type)
	  {
	    case EnvSea: return u.sea*(1.0f/3);
	    case EnvTrees: return u.trees*(1.0f/3);
	    case EnvMeadow: return u.meadow*(1.0f/3);
	    case EnvHouses: return u.houses*(1.0f/3);
	    default: Fail("Bad EnvType");return 0;
	  }
	}
	void SetEnvType(EnvType type, float val)
	{
	  int iVal = toInt(val*3);
	  saturate(iVal,0,3);
	  switch (type)
	  {
	    case EnvSea: u.sea = iVal; break;
	    case EnvTrees: u.trees = iVal; break;
	    case EnvMeadow: u.meadow = iVal; break;
	    case EnvHouses: u.houses = iVal; break;
	    default: Fail("Bad EnvType");break;
	  }
	}
};

//! Terrain geometry types
enum GeometryType
{
  GTBasic = 0,      // Normal complex pixel shader
  GTSimple,         // Simple pixel shader (sat. map only)
  GTShore,          // Shore geometry for water
  GTShoreFoam,      // Shore geometry for foam
  GTShoreWet,       // Wet part of the shore geometry
  GTGrass,          // Grass layer rendered above the terrain
  GeometryTypeCount
};

// forward declaration for classes
struct ApplyWeatherMessage;


class PreloadDataState;
class CloudsCache;
class GroundClutterCache;
class LayerMaskCache;
class Weather;
struct DayLightingItem;

typedef bool CheckMapType(MapType type);

#ifndef DECL_ENUM_DANGER_CAUSE
#define DECL_ENUM_DANGER_CAUSE
DECL_ENUM(DangerCause)
#endif

const float WaterMapScale = 20;
/// water grid is independent on terrain resolution
const float WaterGrid = 50;
const float InvWaterGrid = 1.0f/WaterGrid;

/// unit used for _grassApprox
const float GrassApproxScale = 0.01f;

/// tide configuration
const float MaxTide=1.5f;
/// wave configuration
const float MaxWave=0.25;

/// terrain and scene database storage

#if _VBS3
//stores a heightnode to recover from terrain deformation
struct HeightNode{
  int _x, _y;
  int GetX() const {return _x;}
  int GetY() const {return _y;}
  float _oldHeight; //required to restore Element
  float _newHeight; //stored for later serialization

  int _index;
  bool operator == (const HeightNode &with) const
  {
    return _index==with._index;
  }
  HeightNode()
  {
    _index = -1;
  }
  HeightNode(int x, int y, float oldHeight, float newHeight)
    :_x(x), _y(y), _oldHeight(oldHeight), _newHeight(newHeight)
  {
  }
};
TypeIsMovable(HeightNode)
#endif

//#include <Es/Containers/bigArray.hpp>

class GroundClutterPosMap;
struct LODLimit;
struct FlexibleTerrainLoop;
struct DrawClutterHelperContext;
struct ClutterAccumulator;
struct ClutterStartIndices;
class Terrain;
class AmbientRoadNetwork;

struct BuildingsInfo
{
  /// never allow items smaller than a certain limit, they would cause OffTree to be too deep
  /** we perform lookups with 5 cm overlaps anyway - no need to be more precise */
  static float Eps() {return 0.025f;}

  // Required for TypeIsGeneric
  BuildingsInfo() {};

  OLinkL(Object) _object;
  OffTreeRegion<float> region;

  BuildingsInfo(float x1, float z1, float x2, float z2, OLinkL(Object) object)
    :region(
    floatMin(x1,x2),floatMin(z1,z2),
    floatMax(x1,x2),floatMax(z1,z2)
    )
  {
    region._endX = floatMax(region._endX,region._begX+Eps());
    region._endZ = floatMax(region._endZ,region._begZ+Eps());
    _object = object;
  }

  const OffTreeRegion<float> &GetRegion() const {return region;}

  bool IsIntersection(const OffTreeRegion<float> &q) const {return region.IsIntersection(q);}

  bool operator == (const BuildingsInfo &w){return _object==w._object;}
};
TypeContainsOLink(BuildingsInfo)

/// landscape object (contains terrain data, grass information, and all objects, both static and dynamic)
namespace enf
{
  class PhysicsWorld;
}
class Landscape: public SerializeClass
{
  friend class EditCursor;

  friend class CloudsCache;
  friend class GroundClutterCache;
  friend class GroundClutterInfo;
  
  //! dirty trick - hide GLandscape
  /**
    used to avoid accessing global GLandscape from class methods
    "this" should be used to access it
  */
  int GLandscape;
  
public: 
  /// default distance, where ground detail normal is starts to blend to satellite normal texture
  static const float DefaultSatelliteNormalBlendStart;
  /// default distance, where blending of detail normal ends (satellite normal is fully visible)
  static const float DefaultSatelliteNormalBlendEnd;

  /// if satellite normal should be shown
  static bool ShowSatelliteNormal;

  /// switching between implementations - debugging
  static bool _roadsIntoSegment;

  /// define sync/async mode for some operations
  enum UseObjectsMode
  {
    /// wait for the date to be ready
    UOWait,
    /// do not wait for the data, but refresh caches
    UONoWait,
    /// only return the data we have, do not touch any caches (MT safe mode)
    UONoLock
  };

protected:
  Scenery* _terrain;
  
  float _startDisappearDistGMin; // minimum from all _startDisappearDist
  float _endDisappearDistGMax; // maximum from all _endDisappearDist

  #define land_TerrainInRange(z,x) ((((z)|(x))&~(_terrain)->_terrainRangeMask)==0)
  #define land_InRange(z,x) ((((z)|(x))&~(_terrain)->_landRangeMask)==0)
  #define land_ObjInRange(z,x) ((((z)|(x))&~(_terrain)->_landRangeMask)==0)
  
  Engine *_engine;
  World *_world;
  enf::PhysicsWorld *_pPhysWorld;

  QuadTreeExRoot<byte, QuadTreeExTraitsBYTE<byte> > _soundMap;
#if USE_LANDSCAPE_LOCATIONS
  Locations _locations;
#else
  AutoArray<Vector3> _mountains;
#endif

  SRef<RoadNet> _roadNet;
#if _HELISIM_AMBIENT_TRAFFIC
  SRef<AmbientRoadNetwork> _ambientRoadNetwork;
#endif
  //!{ cached operational maps
  SRef<IOperCache> _operCache;
  SRef<ISuppressCache> _suppressCache;
  SRef<ILockCache> _lockCache;
  SRef<CloudsCache> _cloudsCache;
  RefR<LayerMaskCache> _layerMaskCache;
  RefR<GroundClutterCache> _groundClutterCache;
  //!}
  
  RefR<ObjectRectIndex> _objIndex;
  RefR<ObjectListCache> _listCache;
  RefR<MapObjectRectIndex> _mapObjIndex;
  RefR<MapObjectListCache> _mapListCache;
  RefR<PreloadDataState> _preloadState;
  /// cached synthesis based on _outsideData

	//precalculated value for CheckCloudOcclusion, must be mutable because they work like the "cache" in const functions
	mutable bool	_bRecalculateCloudsOcclusion;
	mutable float	_fCheckCloudOcclusionRes;

  /// true once all persistent objects were loaded
  /** used to detect objects for which persistence was missed during binarization */
  bool _persistentLoaded;

  /// current terrain file loaded
  RString _name;
  /// keep the stream open (optimization)
  mutable QIFStreamB _stream; 
  /// current terrain file version
  int _fileVersion;
  // is LZO compression used for compressed arrays?
  bool _lzoCompression;
  
  /// static entities, may be streamed out
  StaticEntityList _buildings;
  OffTree<BuildingsInfo> _buildingsOffTree;
  
public:
  _forceinline enf::PhysicsWorld * GetPhysicsWorld() { return _pPhysWorld;} 
#if _ENABLE_PHYSICS
  void SimulatePhysics(float timeslice);
#endif
	enf::CMutex m_Mutex;
	enf::CMutex& GetMutex()
	{
		return m_Mutex;
	}

  struct FeatureInfo
  {
    OLinkL(Object) _obj;
    /** note: approximate position could be derived from the _obj as well (based on real position or id) */
    Vector3 _pos;
    /** not sure if we need it, but it can be useful to decide if we really want the feature before link is loaded */
    RefR<const EntityType,const RefCount> _type;
  
    FeatureInfo(){}
    FeatureInfo(const OLinkL(Object) &obj, Vector3Par pos, const EntityType *type)
    : _obj(obj), _pos(pos), _type(type)
    {}
    
    //ObjectId GetKey() const {return _obj->GetId();}
    
    bool operator==(const FeatureInfo &f) {return _obj==f._obj;}
    
    ClassContainsOLink(FeatureInfo)
  };
  
  __forceinline float StartDisappearDistGlobMin() const { return _startDisappearDistGMin; }
  __forceinline float EndDisappearDistGMax() const { return _endDisappearDistGMax; }
  float EffDissapearDist() const;

protected:
  /// list of most important "buildings" - used for distant rendering
  FindArrayKey<FeatureInfo> _features;
  
  int _subdivHintsOffset;
  
  mutable TListBidir<ObjectIdAutoDestroy> _idList;

public:
	void RecalculateCloudOcclusion() {_bRecalculateCloudsOcclusion = true;}

protected:
  /// mapping material index 
  
  /// texture used for sea
  Ref<Texture> _seaTexture;
  /// texture used for middle detail;
  Ref<Texture> _midDetailTexture;
  /// material used for sea
  Ref<TexMaterial> _seaMaterial;
  /// material used for sea shore
  Ref<TexMaterial> _shoreMaterial;
  /// material used for foam at the sea shore
  Ref<TexMaterial> _shoreFoamMaterial;
  /// material used for wet parts at the sea shore
  Ref<TexMaterial> _shoreWetMaterial;

  /// surface material mapping
  /** indices given as LangGrid */
  QuadTreeExRoot<ObjectList, QuadTreeExTraitsObjectList> _objects;

#if _VBS3_LARGE_OBJECTS
public:
  typedef RTree<int, float, 2, float> RTreeStaticObject;
  typedef RTree<Object *, float, 2, float> RTreeObject;
  //! Calculates bounding rectangle of all large static objects in R-tree, r_min and r_max are returned values in land grids
  void SearchStaticRTree(const float a_min[2], const float a_max[2], int r_min[2], int r_max[2]);
  //! Calculates bounding rectangle of all large dynamic objects in R-tree, r_min and r_max are returned values in world coordinates
  void SearchDynamicRTree(const float a_min[2], const float a_max[2], float r_min[2], float r_max[2]);

  bool IsObjectLarge(Object* obj) const 
  {
    if (!obj->GetShape()) return false;
    const Shape *geom=obj->GetShape()->GeometryLevel();
    if( !geom ) return false;
    Vector3Val minv=geom->Min();
    Vector3Val maxv=geom->Max();
    float x = fabs((maxv-minv).X());
    float z = fabs((maxv-minv).Z());
    return ( floatMax(x,z) > _terrain->_landGrid );
  }

  RTreeStaticObject& LargeStaticObjects() {return _largeStaticObjects;}
  RTreeObject& LargeDynamicObjects() {return _largeDynamicObjects;}
  void ClearLargeStaticObjects()
  {
    _largeStaticObjects.RemoveAll();
    _largeStaticObjectInfos.Clear();
  }

private:
  void AddLargeStaticObjectsToRTree();
#if _ENABLE_CHEATS
public:
  // add the object into the Large Objects Maintenance structures (only for debugging purposes)
  bool AddLargeStaticObjectToRTree(Object *obj);
#endif
private:
  RTreeObject _largeDynamicObjects;
#endif

#if _VBS3_LARGE_OBJECTS_WRP || _VBS3_LARGE_OBJECTS
public:
  AutoArray<LargeObjectInfo> _largeStaticObjectInfos;
private:
  RTreeStaticObject _largeStaticObjects;
#endif

  QuadTreeExRoot<MapObjectList, QuadTreeExTraitsMapObjectList> _mapObjects;

  union RandomInfo
  {
    unsigned packed;
    struct
    {
      // random color. data
      unsigned int color:8;
      // precalculated u-v offset (-7..+7)
      int uOff:4;
      int vOff:4;
    } u;
    
    RandomInfo(){}
    explicit RandomInfo(unsigned val):packed(val){}
    unsigned short GetPacked() const {return packed;}

    ClassIsSimple(RandomInfo)
  };

  /*!
  \patch_internal 1.05 Date 7/17/2001 by Ondra.
  - Fixed: different instances of the game had different
  random generator used to generate bumps.
  This caused different simulation results on owner and remote computer.
  */
  RandomGenerator _randGen;

  struct YRange
  {
    float minY,maxY;
    ClassIsSimple(YRange);
  };
  
  /// pre-computed y-ranges for all inner segments
  Array2D<YRange> _yRange;
  
  // save the height map in a given file
  bool DebugSaveHeightMap(const char* filename) const;

  /// helpers for fast position -> primary texture mapping used in SurfaceY()
  // position -> layer index mapping (note that indices are not uniform and depend on corresponding material)
  Array2D<signed char> _primTexIndex;
  // initialization of _primTexIndex, either by FindPrimTexIndex() or just emptying
  void InitPrimaryTexture(bool compute = true);
  // get primary texture on given _terrainGrid coord (fast)
  Texture* PrimaryTexture(int x, int z) const;
  // find primary texture index (slow), just a helper for InitPrimaryTexture()
  int FindPrimTexIndex(int x, int z) const;
  // save the primary texture map as ASCII in a given file, either as raw indices or "unified: (i.e. re-mapped wrt. materials)
  bool DebugSavePrimaryTexture(const char* filename, bool unify = true) const;
  
  /// approximation of the grass around given point, including dynamic updates
  /**
  Used for distant rendering.
  */
  Array2D<unsigned char> _grassApprox;

  /// backup of _grassApprox - as loaded from the file
  Array2D<unsigned char> _grassApproxOrig;

  /// which segments can contain some water
  Array2D<signed char> _waterInSeg;

  //@{ access to data and textures
  float GetData(int x, int z) const {return _terrain->GetData(x, z);}//RawToHeight(_data(x,z));}

  int GetTex(int x, int z) const {return _terrain->_tex(x,z);}
  void SetTex(int x, int z, int data) {_terrain->_tex.Set(x, z, data);}
  //@}
  
  /// controls density of clutters (size of square where single clutter is present)
  float _clutterGrid;
  /// how far clutters are visible
  float _clutterDist;
  /// where ground detail texture is no longer visible
  float _noDetailDist;
  /// where ground detail texture is fully visible
  float _fullDetailDist;
  /// distance set via script
  float _noDetailDistWanted;
  /// distance set via script
  float _fullDetailDistWanted;
  /// distance, where ground detail normal is starts to blend to satellite normal texture
  float _satelliteNormalBlendStart;
  /// distance, where blending of detail normal ends (satellite normal is fully visible)
  float _satelliteNormalBlendEnd;
  /// distance set via script
  float _satelliteNormalBlendStartWanted;
  /// distance set via script
  float _satelliteNormalBlendEndWanted;

  /// individual clutter parameters
  AutoArray<Clutter> _clutter;
  /// if clutter is opaque, we can render it in opaque pass and cast shadows on it
  bool _someClutterAlpha;
  
  /// threshold for forest
  int _minTreesInForestSquare;
  /// threshold for rocks
  int _minRocksInRockSquare;
  
  /// object used for raindrops on the ground
  Ref<LODShape> _rainClutter;
  Ref<Object> _rainClutterObj;
  
  Ref<Object> _cloudObj[NClouds];
  Ref<Object> _skyObject;
  Ref<TexMaterial> _skyMaterial;
  Ref<TexMaterial> _horizonMaterial;
  Ref<Object> _horizontObject;
  Ref<Object> _sunObject;
  Ref<Object> _sunVisTestObject;
  Ref<Object> _moonObject;
  Ref<Object> _rainbowObject;
  Ref<Object> _starsObject;
  Ref<Object> _pointObject;
  RefR<Weather> _weather; // sky and clouds - textures and parameters
  Ref<Texture> _rainTexture;

  /// noise texture used for grass layer alpha disappearing
  Ref<Texture> _grassNoiseTexture;

  Time _weatherUpdate;


#if _VBS3_NO_SEABED
  Array2D<char> _segSeabed; // depth info for each segment: -1=unknown, 0=normal, 1=deep sea segment
  SRef<Shape> _seabedShape; // seabed object (just a tesselated plane)
  float _seaLevelOffset;
#endif

  float _seaLevel; // sea level with tide
  float _seaLevelWave; // sea level with wave effects
  float _seaWaveSpeed;
  
  VisitorObjId _objectId; //!< object ID of the last created object
  VisitorObjId _objectIdPrimary; //!< max object ID of primary (world file) objects

  // id to data conversion

  SRef<LandCache> _segCache;

  //AutoArray<WaterLevel> _waters; // reflection levels in current scene

  SurfaceInfo _waterSurface;
  
  bool _nets; // mark if _networks member is valid
  
#if _ARMA3_HDR_IMPROVEMENTS
  /// override valued for hdr coefs (if negative, coefs from light setup is used)
  float _desiredLuminanceCoefOverride;
  float _luminanceRectCoefOverride;
#endif

#if _ARMA3_HAZE_COMPUTATION
  /// represents how much fog we want to have at haze distance (=distance computed as farFog * hazeDistCoef) 0 = full fog, 1 = no fog
  /// negative value means that old computation of haze is used
  float _hazeFogCoef;
  /// represents coef for computation of haze distance (=distance computed as farFog * hazeDistCoef) 
  /// negative value means that old computation of haze is used
  float _hazeDistCoef;
#endif

#if _ARMA3_SIMUL_WEATHER
  bool _enableSimulWeather;
  Ref<SimulWeatherBase> _simulWeather;
  void CreateSimulWeather();
  /// draw simul weather clouds
  void DrawSimulWeatherClouds(Scene &scene);
#endif
	
	void DrawHeatHazeEffect();
  
#if _ARMA3_ENV_MAPS
  /// environmental texture
  Ref<Texture> _envTexture;
#endif
  
  /// color overrides for sky color and sky top color (for fog computation)
  // if R() < 0, then overrides are not valid
  Color _skyColorOverride;
  Color _skyTopColorOverride;
  /// dynamic object list loading / unloading
  bool LoadObjects(int x, int z, bool noWait) const;

  // friend access needed for iteration functor
  friend class LoadMapObjectsItem;
  friend class UnloadMapObjectsItem;
  friend class MapObjectListRectUsed;
  friend class MapObjectListUsed;

  //! dynamic map object list loading / unloading
  const MapObjectList &LoadMapObjects(int x, int z) const;
  void ReleaseMapObjects(int x, int z) const;

  //! faster way to check persistance without real object loading
  bool CheckPersistence(int x, int z) const;

  /// preload object grid request
  bool DoPreloadObjects(FileRequestPriority prior, int x, int z, int beg, int end) const;
  
  //@{ maintain Clutter mapping in SurfaceCharacter
  void ClearClutterMapping();
  void BuildClutterMapping();
  //@} 
  
  //@{ avoid Weather class being defined where constructor is implemented
  void CreateWeather();
  void LoadWeather(ParamEntryPar cls);
  LSError SerializeWeather(ParamArchive &ar);
  //@}

  /// extract grass mask texture from a primary texture
  Texture *GetGrassMask(const TexMaterial *tex, int layer) const;
  bool PrepareGrassTextures(TexMaterial *mat, const TexMaterial *src);

protected:
  void DoConstruct(Engine *engine, World *world);
  void Dim(int x,int z, int rx, int rz, float landGrid);
  bool TextureIsSimple(int txt) const {return true;}
  void InitObjectIndex();

public:
  Landscape(Engine *engine, World *world, bool nets=false); // default data
  virtual ~Landscape();

  // @{ Script satellite map blending pars
  __forceinline void SetSatelliteNormalBlendStart(float value) { _satelliteNormalBlendStart = value; }
  __forceinline void SetSatelliteNormalBlendEnd(float value) { _satelliteNormalBlendEnd = value; }
  __forceinline void SetNoDetailDist(float value) { _noDetailDist = value; }
  __forceinline void SetFullDetailDist(float value) { _fullDetailDist = value; }
  //@}

#if _ENABLE_PHYSICS
  void DestroyPhysics();
  void InitPhysics(World *world);
#endif

  int GetTerrainRange() const {return _terrain->_terrainRange;}
  int GetTerrainRangeMask() const {return _terrain->_terrainRangeMask;}
  int GetTerrainRangeLog() const {return _terrain->_terrainRangeLog;}

  void SetElevationOffset(float elev) { _terrain->_elevationOffset = elev; }
  float GetLandElevationOffset() const {return _terrain->GetLandElevationOffset();}
  int GetLandRange() const {return _terrain->_landRange;}
  int GetLandRangeMask() const {return _terrain->_landRangeMask;}
  int GetLandRangeLog() const {return _terrain->_landRangeLog;}
  float GetInvLandRange() const {return _terrain->_invLandRange;}
  
  int GetLandSegmentSize() const;
  int GetLandSegmentAlign() const;
  
  float GetInvLandSegmentSize() const;

  /// log2 of the subdivision - how much is _terrainGrig denser than _langGrid
  int GetSubdivLog() const {return _terrain->GetSubdivLog();}
  
  /// subdivision - how much is _terrainGrig denser than _langGrid
  int GetSubdiv() const {return _terrain->GetSubdiv();}

  float GetLandGrid() const {return _terrain->_landGrid;}
  float GetInvLandGrid() const {return _terrain->_invLandGrid;}

  float GetClutterGrid() const {return _clutterGrid;}
  int GetClutterPerLandGrid() const {return toInt(_terrain->_landGrid/_clutterGrid);}


  float GetTerrainGrid() const {return _terrain->_terrainGrid;}
  /**
  Note: precision error may be substantial for large terrain - https://projects.bistudio.com/dayz/Assertion_failed_'tMax_=-0.02f'.txt
  */
  float GetInvTerrainGrid() const {return _terrain->_invTerrainGrid;}

  int GetMinTreesInForestSquare() const {return _minTreesInForestSquare;}
  int GetMinRocksInRockSquare() const {return _minRocksInRockSquare;}

  int GetSatSegSize(int &zAlign) const;

  /// distance, where ground detail normal is starts to blend to satellite normal texture
  float GetSatelliteNormalBlendStart() const {return _satelliteNormalBlendStart;}
  /// distance, where blending of detail normal ends (satellite normal is fully visible)
  float GetSatelliteNormalBlendEnd() const {return _satelliteNormalBlendEnd;}

#if _VBS3_NO_SEABED
  bool IsSeabedSegment(int xBeg, int zBeg);
  void ResetSeabedInfo();
  void DrawSeabed(Scene &scene);
  void SetSeaLevelOffset(float offset)
  {
    _seaLevelOffset = offset; 
    ResetSeabedInfo();
  };
#endif

#if _ARMA3_SIMUL_WEATHER
  void EnableSimulWeather(bool enable)
  {
    if (IsDedicatedServer()) return;

    _enableSimulWeather = enable;
    if (_enableSimulWeather && !_simulWeather)
      CreateSimulWeather();
  }
  //!
  void LoadSimulWeather(ParamEntryPar entry, bool reInit = false);
  //!
  Ref<SimulWeatherBase> GetSimulWeather() {return _simulWeather;}
  //!
  bool IsSimulWeatherEnabled() const {return _simulWeather && _enableSimulWeather;}
  //! Update geometry helper layer
  bool SetSimulWeatherLayersCount(int val);
#endif

#if _ARMA3_HAZE_COMPUTATION
  float GetHazeFogCoef() const {return _hazeFogCoef;}
  void SetHazeFogCoef(float coef) {_hazeFogCoef = coef;}
  float GetHazeDistCoef() const {return _hazeDistCoef;}
  void SetHazeDistCoef(float coef) {_hazeDistCoef = coef;}
#endif

protected:
  void SetLandGrid(float grid);
  void FreeRectCaches();
  void CreateRectCaches();
  void FlushRectCaches();
  /// regular update - give opportunity for maintenance
  void UpdateCaches();
  
public:
  // data management
  /// set surface slot (bimpas) based on given name
  void SetTexture(int i, const char *name, int major);
  /// start preloading surface (bimpas) based on given name
  bool PreloadTextureInfo(const char *name);
  
  void FixDoubleIds();
  /// find a clutter by name
  int FindClutter(const RStringB &name) const;

  int LoadData(const char *name, ParamEntryPar cls, RString cachePath=RString());
  int SaveData(const char *name);

  LSError LoadData(QIStream &in, ParamEntryPar cls,RString cachePath=RString());
  void SaveData(QOStream &in) const;
  int GetFileVersion() const {return _fileVersion;}

  void UpdateRemote();
  void TranferMsg(ApplyWeatherMessage &msg);
  void ApplyWeatherMsg(ApplyWeatherMessage &msg);


  // load/save current status (no terrain/object data save here)
  LSError Serialize(ParamArchive &ar);

  void SerializeBin(SerializeBinStream &f, const ParamEntry *cls);
  void SaveMapObjects(SerializeBinStream *f, QuadTreeExRoot<int> &offsets, const FindArray<RStringB> &objectNames, Array2D<char> *persistent = NULL, int *index = NULL);


  void SerializeBinFP1(SerializeBinStream &f, const ParamEntry *cls);
  
  // perform terrain (_data) subdivision
protected:
  enum {SubdivEnabled=1,SubdivRandom=2};
  typedef unsigned char SubdivHints;

  //! terrain changed - flush segment cache
  void FlushSegCache();

  //! terrain changed - flush segment cache for one LandGrid rectangle
  void FlushSegCache(int x, int z);

public:
  bool LoadOptimized(QIStream &f, ParamEntryPar cls); // wrapper around SerializeBin - true if OK
  void SaveOptimized(QOStream &f);
  void SaveOptimized(const char *name);

  float GetLodBias() const {return _terrain->_lodBias;}

  //! save to optimized binary file in format for FP1
  void SaveOptimizedFP1(const char *name); // true if OK
  //! save to optimized binary stream in format for FP1
  void SaveOptimizedFP1(QOStream &f); // true if OK

  /// Used for diagnostics
  const QuadTreeExRootGeography &GetGeography() const {return _terrain->_geography;}

  //! reset geography information - next call to InitGeography will create it
  void ResetGeography();

  //! init geography information
  void InitGeography();
  void InitMapObjects();
  void InitMountains();
  void InitSoundMap();
  void InitDynSounds( ParamEntryPar entry );
  void InitRandomization();

  bool CheckMountain(int x, int z, float radius, float minDiff);

  int GetWaterSegmentSize() const;


 // initialize terrain min-max map (in terrain segments)
  void InitSegmentMinMax();
  /// init _waterInSeg - where is the water
  void InitWaterInSeg(bool empty);
  /// init _waterInSeg and _grassApprox
  void InitWaterAndGrassMap(bool empty);
  /// clear _grassApprox so that it is initialized later
  void ClearGrassMap();
  /// init _grassApprox
  void InitGrassMap();
  //! Check if grass map was initialized
  bool GrassMapPresent() const {return _grassApprox.RawSize() > 0;}
  /// check if we can render map as infinite
  bool HasTerrainOutside() const {return _terrain->_outsideData.NotNull();}
  /// set _grassApprox to empty
  void InitGrassMapEmpty();
  /// recalculate grass map to consider object dependent information as necessary
  void UpdateGrassMap();

  /// recalculate grass map to consider object dependent information as necessary, one grid only
  void UpdateGrassMap(int x, int z);

  /// recalculate grass map - initiating object known
  void UpdateGrassMap(Object *obj);

  /// cut grass in given terrain range
  bool CutGrass(int ztBeg, int ztEnd, int xtBeg, int xtEnd);
  /// calculate grass height for given terrain vertex
  float GetGrassHeight(float x, float z, StaticArrayAuto< InitPtr<Object> > &roadways) const;

  /// helper (preparation) for CheckRoadway
  void GetRoadList( int x, int z, StaticArrayAuto< InitPtr<Object> > &roadways );

  /// used for roadway checking (roadway suppressing grass)
  const SurfaceInfo *CheckRoadway(const StaticArrayAuto< InitPtr<Object> > &roadways, Vector3Par pos) const;

  const RString &GetName() const {return _name;}

  void SetSound(int x, int z, EnvTypeInfo value);
  EnvTypeInfo GetSound(int x, int z) const;
  void AddSoundRegion(int x, int z, int size, EnvType type, float value);

  /// interpolated result of envType density for given type
  float GetEnvTypeDensity(Vector3Par pos, EnvType type) const;
  
  bool VerifyStructure() const;

  /// Buldozer selection indication
  void SetSelection(const AutoArray<EditorSelectionItem> &sel);

  // object ID management
  //void RebuildIDCache();
  VisitorObjId NewObjectID()
  {
    _objectId.Increment();
    Assert(_objectId >= 0);
    return _objectId;
  }
  void SetLastObjectID(const VisitorObjId &id) {_objectId=id;}
  const VisitorObjId &GetLastObjectID() const {return _objectId;}
  void ResetObjectIDs(); // reset object ids - use with caution
  void ResetState(); // repair all objects, check there are no non-primaries
  void OnTimeSkipped(); //!< time skipped, react accordingly
  /// mission is starting, some cached may need to flushed
  void OnInitMission();

  /// called when terrain detail is changed
  bool OnTerrainGridChanged();

  /// handle when shading quality was changed
  void OnShadingQualityChanged();

  Scenery* GetTerrain(){return _terrain;}
  // data access
  GeographyInfo GetGeography(int x, int z) const{return _terrain->GetGeography(x, z);}
  float GetHeight(int z, int x) const {return _terrain->GetHeight(z, x);} 
  Texture *GetSeaTexture() const {return _seaTexture;}
  Texture *GetMidDetailTexture() const {return _midDetailTexture;}
  const ObjectList &GetObjects(int z, int x) const {return _objects(x,z);}

  struct FilterGeogrForest
  {
    bool operator()(GeographyInfo g) const {return g.u.forest != 0;}
  };

  template <class GeogrInfoFilter>
  float GeographicalDensity(const GeogrInfoFilter &f, Vector3Par pos)
  {
    float xGrid = pos.X() * GetInvLandGrid() - 0.5f;
    float zGrid = pos.Z() * GetInvLandGrid() - 0.5f;
    int x = toIntFloor(xGrid);
    int z = toIntFloor(zGrid);
    GeographyInfo g00 = GetGeography(x,z);
    GeographyInfo g01 = GetGeography(x,z+1);
    GeographyInfo g10 = GetGeography(x+1,z);
    GeographyInfo g11 = GetGeography(x+1,z+1);

    float xFrac = xGrid-x;
    float zFrac = zGrid-z;

    float v00 = ((1-xFrac)*(1-zFrac)); // 00
    float v10 = ((  xFrac)*(1-zFrac)); // 10
    float v01 = ((1-xFrac)*(  zFrac)); // 01
    float v11 = ((  xFrac)*(  zFrac)); // 11

    return v00*f(g00)+ v01*f(g01) + v10*f(g10)+ v11*f(g11);
  }

#if USE_LANDSCAPE_LOCATIONS
  const Locations &GetLocations() const {return _locations;}
  Locations &GetLocations() {return _locations;}

protected:
  void RebuildLocationDistances();

public:
#else
  const AutoArray<Vector3> &GetMountains() const {return _mountains;}
#endif

  const RoadNet *GetRoadNet() const {return _roadNet;}
#if _HELISIM_AMBIENT_TRAFFIC
  void GenerateAmbientTraffic(float t, float x0, float x1, float z0, float z1);
  void SetAmbientTrafficGap(float gap, float x0, float x1, float z0, float z1);
  void SetAmbientTrafficSpeed(float speed, float x0, float x1, float z0, float z1);
  void SetAmbientTrafficEnabled(bool enable);
  float GetAmbientTrafficAvgTrajectoriesPerRoad();
#endif

  void ReleaseAllVBuffers();

  void FlushAICache();
  void FlushCache();
  void InitCache(bool forceProcessing);
  void FillCache( const Frame &pos );
  /// build a material which can be used outside of the map
  void InitOutsideTextureInfo();
  
  void RegisterTexture(int id, const char *name, int major); // load a new texture
  void RegisterObjectType(const char *name); // add a shape into the bank

  //! check if all IDs are already released
  void FreeIds();

  //! free all caches that depend on terrain, objects or any other Landscape content
  void FreeCaches();

  void Init(ParamEntryPar cls); // empty landscape
  
  Ref<TexMaterial> LoadOutsideMaterial(ParamEntryPar outside, TexMaterial *basedOn, bool &terSynth);

  void InitClutter(ParamEntryPar cls);
  void InitSkyObjects(ParamEntryPar cls);
#if _VBS3_NO_SEABED
  void InitSeabed();
#endif
  void Quit(); // before quit
  
#if _VBS3
protected:
  //! change a single heightpoint, store the old value in quadtree
  void RT_ChangeHeightPoint( int x, int z, float y );

public:
  //! used to change the Height of the terrain during the game
  // call instead of HeightChange to make sure that changes get restored after the mission
  void RT_HeightChange( float x, float z, float y );
  //! change all height nodes in this area
  void RT_HeightChangeArea( float xs, float xe, float zs, float ze, float y, bool changeLower, bool changeHigher);
    //! reverts all dynamic Heightfield changes
  void RestoreOldHeightLevels();
#endif

  void HeightChange(int x, int z, float y);
  void TextureChange(int x, int z, int id);

  Object *ObjectCreate(OIDInt id, const char *shape, const Matrix4 &transform, bool avoidRecalculation=false, const char * forceName = NULL);
  Object *ObjectCreate(OIDInt id, const char *shape, ShapeParameters pars, const Matrix4 &transform, int x, int z, bool avoidRecalculation=false);

  //! destroys object.
  void ObjectDestroy(Object * obj);

  //! destroys object.
  void ObjectDestroy(const EditorSelectionItem &id);

  //! moves object.
  void ObjectMove(const EditorSelectionItem &id, const Matrix4 &transform);

  void ObjectTypeChange(const EditorSelectionItem &id, const char *shape);
    
public:
  float ClippedDataXZ(int x, int z, TerrainCache *cache=NULL) const {return _terrain->ClippedDataXZ(x, z, cache);}

  /// calculate a normal for given grid point
  Vector3 ClippedNormal(int x, int z) const;

  /// get texture on given grid (_landGrid coord)
	const Scenery::TextureInfo &ClippedTextureInfo(int z, int x) const {return _terrain->ClippedTextureInfo(z, x);}
  const Scenery::TextureInfo &OutsideTextureInfo() const {return _terrain->_outsideTextureInfo;}

  int ClampFlags(int txt) const {return NoClamp;}

  IOperCache *OperationalCache() const {return _operCache;}
  ILockCache *LockingCache() const {return _lockCache;}
  ISuppressCache *SuppressCache() const {return _suppressCache;}
  
  TListBidir<ObjectIdAutoDestroy> &GetIdList() const {return _idList;}
  
  /// generate a mesh (shape) for given rectangle (segment)
  Ref<LandSegment> GenerateSegment(const GridRectangle &rect, Ref<LandSegment> seg, int lod);

  void GetSegmentMinMaxBox(const GridRectangle &rect, Vector3 *minmax);
  
  /// generate a mesh (shape) for roads in the given rectangle (segment)
  void GenerateRoadSegment(const GridRectangle &rect, Ref<LandSegment> seg, int lod);

  /// generate cached water information for given water rectangle
  Ref<WaterSegment> GenerateWaterSegment(const GridRectangle &rect);

private:
  /// used in GenerateSegment
  void VertexLodHeights(float *res, int x, int z, int lod) const;
  
  void MapSeaUVLogicalLinear(float &u, float &v, float xx, float zz, float coastDirX, float coastDirZ) const;
  void MapSeaUVLogicalCircular(float &u, float &v, float xx, float zz) const;
  void MapSeaUVLogical(float &u, float &v, float xx, float zz, int waterSegmentSize) const;
  void MapSeaUV(float &u, float &v, float xx, float zz, int waterSegmentSize) const;

public:
  /// one universal water segment is all we need to have
  void GenerateWaterSegment(int lod);

  //! Prepare drawing part of the landscape
  void DrawRectPrepare(Scene::DrawContext &drawContext, Scene &scene, float deltaT, bool frameRepeated);
  //! Draw part of the landscape - object to appear in depth buffer
  void DrawRectShadowBuffer(Scene::DrawContext &drawContext, Scene &scene);
  //! Draw part of the landscape - object to appear in depth buffer
  void DrawRectDepthBuffer(Scene::DrawContext &drawContext, Scene &scene, bool frameRepeated);
  //! Draw part of the landscape (including all objects on it)
  void DrawRect(Scene::DrawContext &drawContext, const GridRectangle &groundPrimed, Scene &scene,
    const GridRectangle &rectG, const GridRectangle &rectW, float deltaT, bool frameRepeated
#if _HELISIM_RENDER_TO_TEXTURE
    , bool isR2T
#endif
    );
  // prime the depth buffer 
  void PrimeDepthBuffer(GridRectangle &groundPrimed, Scene &scene, Scene::DrawContext & drawContext, bool zOnly, bool frameRepeated
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif    
    , bool debugMe=false);
  /// use visibility testing to check how much of the sun is visible
  float TestSunVisibility(Scene &scene);
  /// draw sky layers (background, sun, moon, rainbow, not clouds)
  void DrawSky(Scene &scene);
  //! Return shape of the point object
  LODShape *GetPointShape() {return _pointObject->GetShape();}
  /// draw point at specified position
  void DrawPoint(int cb, const Vector3 &pos, float intensity, const Color &color
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
  /// draw horizon alpha effect
  void DrawHorizont(Scene &scene);
  /// draw rain effects on the ground
  void DrawRainClutter(Scene &scene);
  
  //!{ Ground clutter drawing
  //! Prepare drawing of ground clutter
  void PrepareGroundClutter(Scene &scene);
  
  /// interface used for GroundClutterTextureSource
  void AddOrRefreshGroundClutterPosMap(GroundClutterPosMap *posMap);
  //! Draw ground clutter (grass, stones)
  /*!
    \param limitDistance Distance the clutter will be drawn to. Value smaller than 0 indicates there is no limit
  */
  void DrawGroundClutter(Scene &scene, float limitDistance = FLT_MAX, bool debugMe=false);

  void DrawClutterHelper(int cb, const ClutterStartIndices &startIndices, ClutterAccumulator &clutters, const FlexibleTerrainLoop &loop,
    GroundClutterCache &cache, int x, int z, const DrawClutterHelperContext &ctx
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );

  void ConvertToVBufferClutterHelper(const DrawClutterHelperContext &ctx, int x, int z, GroundClutterCache &cache);
  //!}

  /// basic clutter distance (as given in the config)
  float GetClutterDist() const {return _clutterDist;}

  // clutter distance, taking gridSize into an account
  float EffectiveClutterDist(float gridSize) const
  {
    const float sqrt12p5 = 3.5355339059f;
    float clutterDistFactor = sqrt12p5 * InvSqrt(gridSize);
    return _clutterDist*clutterDistFactor;
  }

  /// access clutter types
  const Clutter &GetClutter(int type) const {return _clutter[type];}

  /// some objects when changed may need to force updating underlying clutter
  void ForceClutterUpdate(Object *obj);
  void FlushGroundClutterCache();

  /// preload ground clutter as necessary
  bool PreloadGroundClutter(Vector3Par pos, float radius, bool logging);
  /// check any occlusion caused by cloud layer
  float CheckCloudOcclusion(Vector3Par src, Vector3Par dir) const;
  /// draw dynamic cloud layer
  void DrawClouds(Scene &scene);
  /// draw whole landscape including objects
  void Draw(Scene::DrawContext &drawContext, const GridRectangle &groundPrimed, Scene &scene, float deltaT, bool frameRepeated
#if _HELISIM_RENDER_TO_TEXTURE
    , bool isR2T
#endif    
    );

  void DrawSkyBox(Scene &scene
#if _HELISIM_RENDER_TO_TEXTURE
    , bool isR2T
#endif
    );
  void DrawNull(Scene &scene);

  TerrainCache *UpdateTerrainCache(const GridRectangle &rect);

  bool PreloadWaterTextures(float dist2, float areaOverTex);
  /// helper for concurrent rendering in DrawWater
  void WaterDrawHelper(int cb, WaterSegment *seg, Scene &scene, Vector3Par offset, int x, int z, int lod, float dist, const LightList &lights);
  
  /// helper for concurrent rendering in DrawGround
  void LandDrawHelper(int cb, LandSegment *seg, int x, int z, const GridRectangle &groundPrimed, Scene &scene,
    bool depthOnly, bool zPrime, int lod, Vector3Par pos, float minZDist, float gridSize, float zd, const LightList &lights);

  class LandDrawTask; // allow access (could be friend instead?)
  class WaterDrawTask; // allow access (could be friend instead?)
  
  void DrawWater(const GridRectangle &bigRect, Scene &scene);
  void DrawShore(const GridRectangle &bigRect, Scene &scene, GeometryType gt);
  void DrawGround(const GridRectangle &bigRect, const GridRectangle &groundPrimed, Scene &scene, bool depthOnly, bool zPrime
#if _HELISIM_RENDER_TO_TEXTURE
    , bool isR2T = false
#endif
    );
  void PrepareGroundTextures(LandSegment *seg, float zMin, Vector3Par pos, int lod);
  void PrepareRoadTextures(LandSegment *seg, float zMin, Vector3Par pos, int lod);

  Texture *GetRainTexture() const {return _rainTexture;}
	Texture *GetGodraysTexture() const;
	Texture *GetNewRainTexture() const;

  void SurfacePlane( Plane &plane, float x, float z ) const;

  /// check if given point is in A or B part of the grid
  __forceinline float RectIsInAOrB(float x, float z) const {return x+z <= 1;}

  __forceinline bool GridInRange(int x, int z) const {return land_InRange(x,z);}
  __forceinline bool TerrainGridInRange(int x, int z) const {return land_TerrainInRange(x,z);}

  float ClippedGrass(int x, int z) const
  {
#if !_BULDOZER
    if (land_TerrainInRange(x, z))
      return _grassApprox(x, z) * GrassApproxScale;
#endif
    // note: we currently assume no grass outside of the world
    // this may be wrong
    // we probably should have some representation for this
    return 0;
  }
  /// get four data points from the grid, bigger size allowed
  void GetGrassRectY(int x, int z, int xStep, int zStep, float &y00, float &y01, float &y10, float &y11)  const
  {
    // temporary variables so that compiler can assume no aliasing when writing to them
    float t00,t01,t10,t11;
    if (land_TerrainInRange(z, x) && land_TerrainInRange(z+zStep, x+xStep))
    {
      t00 = _grassApprox(x, z) * GrassApproxScale;
      t10 = _grassApprox(x+xStep, z) * GrassApproxScale;
      t01 = _grassApprox(x, z+zStep) * GrassApproxScale;
      t11 = _grassApprox(x+xStep, z+zStep) * GrassApproxScale;
    }
    else
    {
      t00 = ClippedGrass(x, z);
      t10 = ClippedGrass(x+xStep, z);
      t01 = ClippedGrass(x, z+zStep);
      t11 = ClippedGrass(x+xStep, z+zStep);
    }
    y00 = t00;
    y01 = t01;
    y10 = t10;
    y11 = t11;
  }

  //! pseudo-bilinear interpolation, yXZ, xf and zf = <0,1>
  /**
  Select triangle and perform interpolation for given grid
  helper for SurfaceY calculation
  */
  static __forceinline float RectBilint(float y00, float y01, float y10, float y11, float x, float z)
  {
    if (x+z <= 1)
    { // triangle 00,01,10
      return y00+(y01-y00)*z+(y10-y00)*x;
    }
    else
    {
      // triangle 01,10,11
      return (y01+y10-y11)+(y11-y01)*x+(y11-y10)*z;
//      return y11-(y11-y01)*(1-x)-(y11-y10)*(1-z);
    }
  }
  /// RectBilint variant - point known to be on diagonal
  static __forceinline float RectBilintDiag(float y00, float y01, float y10, float y11, float x, float z)
  {
    // point on diagonal, ( x+z==1 )
    // any triangle will do
    // triangle 00,01,10
    return y00+(y01-y00)*z+(y10-y00)*x;
  }

#if _VBS3_LARGE_OBJECTS
  // when underground object is moved, all epe grids it can affect have to be regenerated
  void OnObjectMove(Object* obj);
#endif

  void DrawDiags();
  
  /// source y coordinate for given x,z - before subdivision
  float SurfaceYSource(float x, float z) const;
  /// y coordinate for given x,z
  float SurfaceY(float x, float z) const;
  float SurfaceY(Vector3Par pos) const {return SurfaceY(pos.X(), pos.Z());}
  // complex interface
  float SurfaceY(float x, float z, float *rdX, float *rdZ, Texture **texture=NULL) const;
  /// y coordinate for given x,z, simulating coarse lod tessellation
  float SurfaceYLod(float x, float z, int lod) const;

  float SurfaceYAboveWater(float x, float z, float *rdX=NULL, float *rdZ=NULL, Texture **texture=NULL) const;
  float SurfaceYAboveWaterNoWaves(float x, float z) const;

  // surface normal at the given point
  Vector3 SurfaceNormal(float x, float z) const;
  Vector3 SurfaceNormal(Vector3Par pos) const {return SurfaceNormal(pos.X(), pos.Z());}

  /// grass layer interpolation
  float GrassHeight(float x, float z) const;
 
  //! Default  filter used in RoadSurface* function
  class FilterNone : public IntersectionFilter
  {
  public:
    bool operator()(const Object *) const {return true;};    
  };

  //! Filter one object. Used in RoadSurface* function
  static bool FilterIgnoreOneHelper(const Object *veh, const Object *obj)
  {
    return !veh || veh != obj && obj->GetAttachedTo()!=veh && veh->GetAttachedTo()!=obj;
  }

  //! Filter one object. Used in RoadSurface* function
  class FilterIgnoreOne : public IntersectionFilter
  {
  protected:
    const Object * _obj;
  public:
    FilterIgnoreOne(const Object * obj) :_obj(obj) {}
    bool operator()(const Object * obj) const
    {
      return FilterIgnoreOneHelper(_obj,obj);
    }
  };

  class FilterIgnoreTwo : public IntersectionFilter
  {
  protected:
    const Object * _obj1;
    const Object * _obj2;
  public:
    FilterIgnoreTwo(const Object * obj1, const Object * obj2) :_obj1(obj1),_obj2(obj2) {}
    bool operator()(const Object * obj) const
    {
      return FilterIgnoreOneHelper(_obj1,obj) && FilterIgnoreOneHelper(_obj2,obj);
    }
  };

  //! Filter allows only primary objects. Used in RoadSurface* function
  class FilterPrimary : public IntersectionFilter
  {     
  public:    
    bool operator()(const Object * obj) const {return obj->GetType() == Primary;};
  };

  template <class Filter1, class Filter2>
  class FilterCombineT: public IntersectionFilter
  {
    const Filter1 &f1;
    const Filter2 &f2;

    public:
    FilterCombineT(const Filter1 &f1, const Filter2 &f2):f1(f1),f2(f2){}

    virtual bool operator()(const Object *o) const
    {
      // use non-virtual dispatch for performance reasons, assumes we know the exact type compile time
      return f1.Filter1::operator()(o) && f2.Filter2::operator()(o);
    };
  };

  template <class Filter1, class Filter2>
  static FilterCombineT<Filter1,Filter2> FilterCombine(const Filter1 &f1, const Filter2 &f2)
  {return FilterCombineT<Filter1,Filter2>(f1,f2);};

  //! find nearest surface under given point
  float RoadSurfaceY(Vector3Par pos, float *dX=NULL, float *dZ=NULL, Texture **texture=NULL,
    UseObjectsMode syncMode=UOWait, Object **obj=NULL, const Object *with=NULL, const Object *ignore=NULL) const;

  //! find topmost surface on given x,z coordinates
  float RoadSurfaceY(float xC, float zC, float *dX=NULL, float *dZ=NULL, Texture **texture=NULL,
    UseObjectsMode syncMode=UOWait, const Object *ignore=NULL) const;

  void ObjectRoadSurfaceY(const Object *with, Object *obj, Vector3Par pos, float landY, float sdX, float sdZ,
    Vector3 &ret, Object *&dXdZRelToObj, Object **retObj, float &maxY, Texture *&maxTexture, float &maxDX, float &maxDZ) const;
  
  float RoadSurfaceYAboveWater(Vector3Par pos, float *dX=NULL, float *dZ=NULL, Texture **texture=NULL,
    UseObjectsMode syncMode=UOWait, const Object *ignore=NULL) const;    
  float RoadSurfaceYAboveWater(float xC, float zC, float *dX=NULL, float *dZ=NULL, Texture **texture=NULL,
    UseObjectsMode syncMode=UOWait, const Object *ignore=NULL) const;

  //! find nearest surface under given point, but ignore land, test only objects and water
  float WaterSurfaceY(Vector3Par pos) const;

  Point3 PointOnSurface(float x, float y, float z) const;

  float AboveSurface(Vector3Val pos) const;
  float AboveSurfaceOrWater(Vector3Val pos) const;

  //@{ access to landscape specific random number generator
  float GetRandomValueForObject(Object *obj, int offset=0);
  float GetRandomValueForCoord(int x, int z, int offset=0);
  float GetRandomValue(int seed) const {return _randGen.RandomValue(seed);}
  RandomGenerator &GetRandGen() {return _randGen;}
  //@}

  float CalculateBump(float xC, float zC, float bumpy) const;
  float BumpySurfaceY(float x, float z, float &rdX, float &rdZ, Texture *&texture, float bumpy, float &bump) const;

  #if _ENABLE_REPORT && (_PROFILE || _DEBUG)
  struct ObjDebuggingContext
  {
    Matrix4 _toWorld;
    
    ObjDebuggingContext()
    {
      _toWorld = MIdentity;
    }
    ObjDebuggingContext Transform(const Object *obj) const
    {
      ObjDebuggingContext ret;
      ret._toWorld = _toWorld*obj->FutureVisualState().Transform();
      return ret;
    }
  };
  #else
  struct ObjDebuggingContext
  {
    ObjDebuggingContext Transform(const Object *obj) const {return ObjDebuggingContext();}
  };
  #endif
  
  float UnderRoadSurface(const Object *obj, AnimationContext &animContext,
    Vector3Par pos, Vector3Par center, float bumpy, const ObjDebuggingContext &ctx, float *dX=NULL, float *dZ=NULL,
    Texture **texture=NULL) const;

  void SetSkyTexture(Texture *textureA, Texture *textureRA, Texture *textureHA,
    Texture *textureB, Texture *textureRB, Texture *textureHB,
    float factor, ColorVal skyColor, ColorVal skyTopColor);

  void InitObjectVehicles();

  Object *AddObject(float x, float y, float z, float head, const char *name);
  Object *AddObject(Vector3Par pos, float head, LODShape *shape, void *user=NULL);

public:
  //! find a new ID for an object that will be added
  ObjectId NewObjectId(VisitorObjId visId, int x, int z) const;
  
  //! add an Object into given grid
  void AddObject(Object *obj, int x, int z, bool avoidRecalculation=false);
  //! add an Object, select grid
  void AddObject(Object *obj, int *xr=NULL, int *zr=NULL, bool avoidRecalculation=false);
  void Recalculate();
  void RemoveObject(Object *obj);
  void MoveObject(Object *obj, const Matrix4 &transform);
  void MoveObject(Object *obj, Vector3Par pos);

  /// identify a grid owning an object
  const ObjectList &GetList(Object *obj) const;

  //! moves object. If object IsObject and it is moved to different grid square object is replaced with one with correct ID
  void MoveObjectFar(Object *obj, Vector3Par pos);
  void MoveObjectFar(Object *obj, Matrix4Par pos);

  //! find object with given Visitor id, x,z is estimation where it should be searched for
  OLink(Object) GetObject(VisitorObjId id, int x, int z) const;
  //! find object with given Visitor id, pos is estimation where it should be searched for
  OLink(Object) GetObject(VisitorObjId id, Vector3Par pos) const;
  
  //! find object with given Visitor id, pos is estimation where it should be searched for
  ObjectId GetObjectId(VisitorObjId id, int x, int z) const;
  //! find object with given Visitor id, pos is estimation where it should be searched for
  ObjectId GetObjectId(VisitorObjId id, Vector3Par pos) const;

  //! find object based on object id
  OLink(Object) GetObject(const ObjectId &id) const;
  //! find object based on object id
  OLinkL(Object) GetObjectNoLock(const ObjectId &id) const;
  
  //! find object based on object id, and lock it
  Object *GetObjectLocked(const ObjectId &id) const;

  //! manipulate object list refcount - faster, use when list is already known
  void AddRefObjectList(ObjectListFull *list);
  //! manipulate object list refcount - faster
  void ReleaseObjectList(int x, int z, ObjectListFull *list)
  {
    /// list is verified as non-NULL on every place where ReleaseObjectList is called
    Assert(list);
    if (IsStreaming())
    {
      if (list->_used==1)
        ReleaseObjectListBody(x,z,list);
      else
        list->_used--;
    }
  }

private:  
  /// helper function for ReleaseObjectList
  void ReleaseObjectListBody(int x, int z, ObjectListFull *listFull);
  
public:
  //@{ manipulate object list refcount - caution: unsafe, not all lists can be addrefed
  void AddRefObjectList(int x, int z);
  void ReleaseObjectList(int x, int z);
  //@}
  
  //@{ safe access to object list locking/unlocking
  void AddRefObjects(int x, int z);
  void ReleaseObjects(int x, int z);
  //@}
  
  /// check if the landscape is streaming
  /*!
  Only streaming landscapes are currently supported on Xbox
  */
    bool SupportNonstreaming() const {return true;}
    bool IsStreaming() const {return _objIndex.NotNull();}

  /// check if there is some roadway in given rectangle
  __forceinline bool IsSomeRoadway(int x, int z) const
  {
    const ObjectListFull *list = _objects(x,z).GetList();
    if (!list)
    {
      GeographyInfo g(_terrain->_geography(x,z));
      return g.u.someRoadway;
    }
    else
      return list->GetRoadwayCount()>0;
  }
  
  enum PreloadResult
  {
    /// not preloaded - but request was made
    PreloadRequested,
    /// data are ready - trivial case
    PreloadFast,
    /// data are ready - complex case
    PreloadSlow
  };
  /// preload object grid if necessary
  PreloadResult PreloadObjects(FileRequestPriority prior, int x, int z) const;

  /// check if given object grid is ready. Defined only for grids where some object exists
  bool ObjectsReady(int x, int z) const;

  //! use objects and return lock object
  ObjectListUsed UseObjects(int x, int z, UseObjectsMode noWait=UOWait) const;

#if _VBS3_LARGE_OBJECTS
  //! use objects and return lock object
  ObjectListUsedXXL UseObjectsXXL(const ObjectListUsed &list, int x, int z) const;
#endif

  struct PreloadDataContext;
  
private:
  bool PreloadData(int x, int z, PreloadDataContext &ctx, bool grassLayer);
  
  void GetTerrainPreloadGrid(Vector3Par pos, float maxRange, GridRectangle &aroundRect);
  void PreloadTerrain(int x, int z, Vector3Par pos);
  
public:
  /// preload all objects in given grid
  bool PreloadData(bool grassLayer, Vector3Par pos, float maxPreloadDist, float maxTime, float degrade,
    Object *doNotPreload, bool alwaysRecreate, float autoTimeOut, bool logging,
    const Vector3 *dir, float fov);
  
  bool PreloadData(Vector3Par pos, float maxPreloadDist, float maxTime, float degrade,
    Object *doNotPreload, bool alwaysRecreate=false, float autoTimeOut=FLT_MAX, bool logging=false,
    const Vector3 *dir=NULL, float fov=-1);
  
  /// make sure sky texture is loaded
  bool PreloadSkyAndClouds() const;
  //! empty objeck list on given cell
  void DeleteObjectList(int x, int z);

#if _ARMA3_ENV_MAPS
  /// environmental texture
  Ref<Texture> GetEnvTexture() const {return _envTexture;}
#endif

  //! access to map objects - no loading
  const MapObjectList &GetMapObjects(int x, int z) const {return _mapObjects(x,z);}
  //! use map objects and return lock object
  MapObjectListUsed UseMapObjects(int x, int z) const;
  //! use map object rectangle and return lock object
  MapObjectListRectUsed UseMapObjects(int xBeg, int zBeg, int xEnd, int zEnd) const;
  //! use map object rectangle, change given lock object
  void UseMapObjects(MapObjectListRectUsed &lock, int xBeg, int zBeg, int xEnd, int zEnd) const;
  //! quick check if object list is really empty
  bool MapObjectsEmpty(int x, int z) const;

  /// preload map objects rectangle so that it is ready in the cache
  bool PreloadMapObjects(const MapObjectListRectUsed &lock, int xBeg, int zBeg, int xEnd, int zEnd) const;
  
  /// preload a single grid
  bool PreloadMapObjects(int x, int z) const;

  /// iterate though existing mapobject lists using a predicate
  template <class Detect, class Func>
  void ForSomeMapObjects(const Detect &det, const Func &f) const {_mapObjects.ForSomeItem(det, f);}

  /// iterate though all possible mapobject positions lists using a predicate
  template <class Detect, class Func>
  void ForSomeMapObjectSlots(const Detect &det, const Func &f) const {_mapObjects.ForSomePosition(det, f);}
  
  // control weather  
  float GetRainDensity() const;
  float GetOvercast() const;
  float GetNextRainChange() const;
  float GetRainForecast() const;
  void ResetWeather();
  void SetOvercast( float overcast, bool reset=false );
  void SetFog(float fog);
  void SetRain(float density, float time, bool force = false);
  void SetNextRainChange(float time);
  void SetWind( float wind );
  
  void SetLightOverride(const DayLightingItem &override);

  //! Wrapper for temperature retrieving
  void GetTemperature(float &air, float &black, float &white, float dayOffset = 0.0f) const;
  float DayFactor() const;

  void Simulate(float deltaT);
  /// sea level - ignore waves
  float GetSeaLevelNoWave() const {return _seaLevel;}
  /// sea level - global waves included
  float GetSeaLevel() const {return _seaLevelWave;}
  /// sea level - local waves included
  float GetSeaLevel(float x, float z) const;
  /// sea level - local waves included
  float GetSeaLevel(Vector3Par pos) const {return GetSeaLevel(pos.X(),pos.Z());}
  //@{ interface to Engine - see EnginePropeProperties, EngineSceneProperties
  void GetSeaWaveMatrices(Matrix4 &matrix0, Matrix4 &matrix1, Matrix4 &matrix2, Matrix4 &matrix3, const EngineShapeProperties &prop) const;
  void GetSeaWavePars(float &waveXScale, float &waveZScale, float &waveHeight) const;
  //@}
  /// max. possible sea level included waves
  float GetSeaLevelMax() const;
  /// wave height depending on the weather
  float GetWaveHeight() const;
  
  //@{ communicate LOD parameters with the vertex shader
  float GetLODBias() const;
  float GetMaxLod() const;
  //@}
  void SetSeaWaveSpeed(float seaWaveSpeed) {_seaWaveSpeed=seaWaveSpeed;}

  Vector3 GetWind() const;
  Vector3 GetWindSlow() const;

  Vector3 GetWindSpeed() const;
  Vector3 GetWindSpeedSlow() const;
  /// Set current direction of the wind
  void SetWindSpeed(Vector3 direction, bool force);

  const SurfaceInfo &GetWaterSurface() const {return _waterSurface;}

  // query weather
  float CloudsPosition() const;
  /// how much light can go through the sky layer
  float SkyThrough() const;
  /// how much light can go through the sky + cloud layer
  float CloudThrough() const;
 
  float GetLighting(Color &amb, Color &dif, Color &sun, float cosSun, float cloud) const;
  void GetSkyColors(Color &sky, Color &aroundSun, float cosSun) const;
#if _ARMA3_HDR_IMPROVEMENTS
  /// gets HDR coeficients
  void GetHDRCoefs(float cosSun, float cloud, float& desiredLuminanceCoef, float& luminanceRectCoef) const;
  /// sets override of desired luminance coef (if negative, coef from light config is used)
  void SetDesiredLuminanceCoefOverride(float coef) {_desiredLuminanceCoefOverride = coef;}
  float GetDesiredLuminanceCoefOverride() const {return _desiredLuminanceCoefOverride;}
  /// sets override of luminance rect coef (if negative, coef from light config is used)
  void SetLuminanceRectCoefOverride(float coef) {_luminanceRectCoefOverride = coef;}
  float GetLuminanceRectCoefOverride() const {return _luminanceRectCoefOverride;}
#endif

#if _ARMA3_NEW_LIGHTING_CFG
  /// debug switch on/off new lighting
  static bool DebugUseNewLighting;

  /// flag that tells us if we should use new land lighting values (this is true iff new land lighting is in cfg for world)
  bool UseNewLandLighting() const;
  /// get lighting values, returns if resul was successfully filled
  bool GetLightingNew(LandLighting::LightingResult& result, float height, float cosSun, float cloud) const;
  bool GetLightingNew(LandLighting::LightingResult& result, float height, float overcast, float cosSun, float cloud) const;
  /// set lighting override
  void SetLightOverrideNew(const LandLighting::Item &override);
  /// loads new lighting
  void DebugLoadLightingNew(ParamEntryPar cfg);
#endif


  float CloudsAlpha() const;
  float CloudsBrightness() const;
  float CloudsHeight() const;
  float CloudsSize() const;

  //@{ building list access    
  const StaticEntityLink &GetBuildingInfo(int i) const {return _buildings.GetInfo(i);}
  OLinkOL(Object) GetBuilding(int i) const {return _buildings.Get(i);}
  int NBuildings() const {return _buildings.Size();}
  const StaticEntityLink *FindBuildingInfo(const ObjectId &id) const;
  int FindBuildingIndex(const ObjectId &id) const {return _buildings.FindIndex(id);}
  //@}

  void AddBuildingOffTree(BuildingsInfo buildingsInfo) {_buildingsOffTree.Add(buildingsInfo);};
  void GetBuildings(OffTree<BuildingsInfo>::QueryResult &res, float x1, float z1, float x2, float z2)
  {
    _buildingsOffTree.Query(res, x1, z1, x2, z2);
  }

  //@{ feature list access
  const Array<FeatureInfo> &GetFeatures() const {return _features;}
  
  void BuildFeatureList();
  //@}
  
  //! find object nearest to given pos, apply given criteria
  OLink(Object) NearestObject(Vector3Par pos, float maxDist=0, ObjectType type=Any, Object *ignore=NULL); // default - no limit

  //! find object nearest to given pos, apply given criteria
  OLink(Object) NearestObjectAsync(bool &ready, bool async, Vector3Par pos, float maxDist=0, ObjectType type=Any, Object *ignore=NULL); // default - no limit
  
  //! find object nearest to given pos, apply user defined criteria
  OLink(Object) NearestObject(Vector3Par pos, float maxDist, bool (*isSuitable)(Object *obj, float dist2, void *context), void *context, bool dist2D=false);
  //! find object nearest to given pos, apply user defined criteria
  OLink(Object) NearestObjectAsync(bool &ready, bool async, Vector3Par pos, float maxDist, bool (*isSuitable)(Object *obj, float dist2, void *context), void *context, bool dist2D=false);

  float CheckVisibility(int x, int z, const VisCheckContext &context, ObjIntersect isect) const;

  bool CheckIntegrity() const;

  float VisibleStrategic(Vector3Par from, Vector3Par to) const;
  float Visible(VisualStateAge age, Vector3Par from, Vector3Par to, float toRadius, const Object *skip1, const Object *target, ObjIntersect isect=ObjIntersectView, bool cloudlets=true) const; // point visibility - used for flares etc.
  float Visible(const Object *sensor, const Object *object, float reserve=1, ObjIntersect isect=ObjIntersectView, bool cloudlets=true) const;
  float Visible(bool &headOnly, Vector3Par sensorPos, const Object *sensor, const Object *object, float reserve=1, ObjIntersect isect=ObjIntersectView, bool cloudlets=true) const;

  /// check how much is given line going under the terrain surface
  float IntersectUnder(Vector3Par from, Vector3Par dir, float maxDist, float bendUp, float bendUpOutside, TerrainCache *cache) const;

  //! check if point is inside some object, 
  void IsInside(StaticArrayAuto< OLink(Object) > &objects, Object *ignore, Vector3Par pos, ObjIntersect isect=ObjIntersectGeom);
  /// what distance is given line under the terrain (used for shadows)
  float CheckUnderLandDist(Vector3Par beg, Vector3Par dir, float tMin, float tMax, int x, int z, float &underT, TerrainCache *cache) const;

  struct CheckUnderLandContext;
  /// what angle is given line under the terrain (used for visibility testing)
  float CheckUnderLandAngle(float tMin, float tMax, int x, int z, CheckUnderLandContext &ctx) const;

  float ComputeUnder(
    float y00, float y01, float y10, float y11, int x, int z, float tMin, float tMax, Vector3Par beg, Vector3Par dir,
    float *travelUnder
  ) const;

  // old calling convention - do not use
  Vector3 IntersectWithGround(Vector3Par from, Vector3Par dir, float minDist=0, float maxDist=FLT_MAX /*virtually no limit*/) const;
  // old calling convention - do not use
  Vector3 IntersectWithGroundOrSea(Vector3Par from, Vector3Par dir, float minDist=0, float maxDist=FLT_MAX /*virtually no limit*/) const;

  // return distance where an intersection is found (or maxDist if no intersection is found)
  float IntersectWithGroundOrSea(Vector3 *ret, Vector3Par from, Vector3Par dir, float minDist=0, float maxDist=FLT_MAX /*virtually no limit*/) const;
  float IntersectWithGroundOrSea(Vector3 *ret, bool &sea, Vector3Par from, Vector3Par dir, float minDist=0, float maxDist=FLT_MAX /*virtually no limit*/) const;
  
  //MiVa: trying to move the intersection a little faster -> on XBOX is the speed up from 10 - 25%
  //the original version has name postfix Orig
  float IntersectWithGround(Vector3 *ret, Vector3Par from, Vector3Par dir, float minDist=0, float maxDist=FLT_MAX /*virtually no limit*/) const;
  //float IntersectWithGroundOrig(Vector3 *ret, Vector3Par from, Vector3Par dir, float minDist=0, float maxDist=FLT_MAX /*virtually no limit*/) const;

  bool CheckIntersection(Vector3Par beg, Vector3Par end, float cellx, float cellz, float &tRet, const float h00, const float h10, const float h01, const float h11) const;
  //bool CheckIntersectionOrig(Vector3Par beg, Vector3Par end, int x, int z, float &tRet) const;

  void CheckObjectCollision(int x, int z, CollisionBuffer &retVal, CheckObjectCollisionContext &context) const;

  //! Collision check with line specified by beg and end
  void ObjectCollisionLine(VisualStateAge age, CollisionBuffer &retVal, const IntersectionFilter& f, Vector3Par beg, Vector3Par end,
    float radius, ObjIntersect typePri=ObjIntersectFire, ObjIntersect typeSec=ObjIntersectNone, unsigned int collisionFlags = 0) const;
  
  /// find largest box around given point which is free of objects
  void CheckBoxFreeOfObjects(Vector3 *minmax, const Object *ignore, const FrameBase &pos, bool staticOnly, float allowedMinZ) const;
  
  void ObjectCollision(CollisionBuffer &retVal, const Object *with, const ObjectVisualState &withPos, bool onlyVehicles=false, bool bPerson=false) const;  

  // be ware normal is in context.with model coord
  void ObjectCollisionFirstTouch(float &doneFrac, Vector3 &normal, ObjectCollisionFirstTouchContext &context) const;

  void PredictCollision(VehicleCollisionBuffer &ret, const Entity *vehicle, float maxTime, float gap, float maxDistance) const;
  /// check collision of given model against ground/terrain
  int GroundCollision(GroundCollisionBuffer &retVal, Object *with, const ObjectVisualState &withPos, float above, float bumpy,
    bool enableLandcontact=true, bool soldier=false) const;  

  /// effects of explosion only - no actual damage
  void ExplosionDamageEffects(EntityAI *owner, Object *directHit, HitInfoPar hitInfo, int componentIndex, bool enemyDamage, float energyFactor, float explosionFactor);
  /// explosion does damage supports hierarchy of objects
  void ExplosionDamage(EntityAI *owner, Vector3Par originDir, Object *directHit, Vector3Par directHitModelPos, HitInfoPar hitInfo, Object *componentOwner=NULL, int componentIndex=-1, float energyFactor=1.0f, float explosionFactor=0.3f, float hitDepth = 0.f);
  /// some projectiles get stuck on the target entity
  void ApplyProjectileDecal(Object *directHit, int componentIndex, HitInfoPar hitInfo, float hitDepth, Vector3Par hitPosLcl, Vector3Par hitDirLcl, Vector3Par hitDirUpLcl);
  /// some projectiles get stuck on the target entity - applied to static objects
  void ApplyProjectileDecal(HitInfoPar hitInfo, float hitDepth);

  void Disclose(DangerCause cause, EntityAI *owner, Vector3Par pos, float maxDist, bool discloseSide, bool disclosePosition);
  void Stress(EntityAI *owner, Vector3Par pos, float hit);

  bool CheckObjectStructure() const;

  bool SaveXYZ(const char *name) const;
  bool SaveXYZ(QOFStream &f) const;

protected:
#if _VBS3
  //stores Real Time changes in the height field
  QuadTreeCont<HeightNode> _changedHeightField;
#endif
  
  void ClearGroundClutterCache();
  void CreateGroundClutterCache();
  void FlushGroundClutterSegCache();
  void InitGroundClutterSegCache();

  void ReplaceObjects(RString name);

  int CreateArea(int x, int z, const ObjectListUsed &src, MapObjectList &dst, CheckMapType &funcCount, CheckMapType &funcAddOutside, MapType areaType, float minObjPerSquare);

private:
  static void ObjectCollisionHelper(CollisionBuffer &retVal, const Object *obj, const ObjectVisualState &objhPos, ObjectCollisionContext &context);  
  static int ObjectCollisionFirstTouchHelper(float &doneFrac, Vector3 &normal, const Object *obj, const ObjectVisualState &objPos, ObjectCollisionFirstTouchContext &context);
  static void CheckObjectCollisionHelper(CollisionBuffer &retVal, const Object * obj, const ObjectVisualState& objPos, CheckObjectCollisionContext &context);
};

#if defined _MSC_VER && defined _M_PPC
// undo bitfield_order above
# pragma bitfield_order(pop)
#endif

#include <Es/Memory/debugNew.hpp>

extern Landscape *GLandscape;
#define GLOB_LAND (GLandscape)

void ClearShapes(); // flush all cached shapes, types ...

Object *NewObject(LODShape *shape, const CreateObjectId &id);

#endif
