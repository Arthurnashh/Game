#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ROADS_HPP
#define _ROADS_HPP

#include "types.hpp"
#include "object.hpp"
#include "objLink.hpp"
#include "landscape.hpp"
#include "AI/supressId.hpp"

class SerializeBinStream;
class SupressIdList;

#include <Es/Memory/normalNew.hpp>

class RoadLink;

/// node of RoadNet for A*
class RoadNode
{
protected:
  /// road segment this node is on
  const RoadLink *_road;
  /// index of node in the _road
  int _index;

public:
  RoadNode(const RoadLink *road, int index) : _road(road), _index(index) {}

  const RoadLink *GetRoad() const {return _road;}
  int GetNodeIndex() const {return _index;}

  bool IsValid() const {return _road != NULL;}

  /// return the position of the node for soldier or vehicle of given width
  Vector3 Position(bool soldier, float vehWidth) const;
  /// return the direction of road in the node for a vehicle
  Vector3 Direction() const;
  /// return the number of nodes we are connected to
  int NConnections(bool soldier) const;
  /// return the node we are connected to
  RoadNode GetConnection(bool soldier, int i) const;
#if _ENABLE_CHEATS
  RString GetDebugName() const {return Format("0x%x:%d", _road, _index);}
#endif
};

class RoadLink
{
  public:
  enum {NCon=4};
  /// Type of the road connection
  #ifndef DECL_ENUM_CONNECTION_TYPE
  #define DECL_ENUM_CONNECTION_TYPE
    DECL_ENUM(ConnectionType)
  #endif
  DEFINE_ENUM_BEG(ConnectionType)
    CTNone = -1,
    CTBegin,
    CTEnd,
    CTLeft,
    CTRight
  DEFINE_ENUM_END(ConnectionType)
  /// Type of the road link
  enum RoadType
  {
    RTError, // invalid road segment
    RTDisconnected, // segment not connected to road net
    RTBegin, // begin of the road
    RTEnd, // end of the road
    RTI, // simple road segment
    RTTR, // T shape crossing (right)
    RTTL, // T shape crossing (left)
    RTX // X shape crossing
  };

  private:

  Vector3 _pos[NCon];
  RoadLink *_con[NCon];
  SizedEnum<ConnectionType, char> _type[NCon];
  signed char _nCon;

  /// locks applicable to vehicles only (caused by soldiers)
  short int _locksVehicle;
  short int _locksVehicleWaiting;
  /// locks applicable to soldiers and vehicles (caused by vehicles)
  short int _locksSoldier;
  /// locks applicable to soldiers and vehicles (caused by vehicles which may start moving again soon)
  short int _locksSoldierWaiting;
  OLinkL(Object) _object;
  Ref<LODShape> _lshape;
  Matrix4 _toWorld;
  /// list of units which are suppressing given part of the road (if any)
  SRef<SupressIdList> _suppressed;

  public:
  RoadLink(); // used when serializing
  RoadLink(Object *object, Vector3 *pos, ConnectionType *type, int c);
  ~RoadLink(){}

  __forceinline int NConnections() const {return _nCon;}
  __forceinline const Vector3 *PosConnections() const {return _pos;}
  __forceinline RoadLink * const *Connections() const {return _con;}
  __forceinline void SetConnection( int i, RoadLink *con )
  {
    Assert(i<NCon);
    _con[i]=con;
  }
  __forceinline void SetPosConnection(int i, Vector3Par pos)
  {
    Assert(i<NCon);
    _pos[i]=pos;
  }
  OLinkLPtr(Object) GetObject() const {return _object.GetLock();}
  LODShape *GetLODShape() const { return _lshape; }
  const Matrix4 &Transform() const { return _toWorld; }
  /// position of the road element for A*
  Vector3 AIPosition() const;

  /// check if this link is a part of the bridge
  bool IsBridge() const;
  
  /// the shape of the road
  RoadType GetRoadType() const;

  /// number of nodes for A* RoadLink represent
  int NNodes(bool soldier) const;
  /// create the node for A*
  RoadNode GetNode(int index) const {return RoadNode(this, index);}

  /// find the node which is best matching to the given parameters
  RoadNode FindBestNode(bool soldier, float vehWidth, Vector3Par position, Vector3Par direction) const;

  void AddConnection( Vector3Par pos, RoadLink *con );
  Vector3 GetCenter() const;

  int IsLocked(bool soldier) const;
  int IsLockedByWaiting(bool soldier) const;
  void Lock(bool soldier, bool waiting);
  void Unlock(bool soldier, bool waiting);

  /// quick reject impossible inside candidates
  bool MayBeInside(Vector3Val pos, float size) const;
  /// check if IsInside has all input data ready
  bool IsInsideReady() const;
  /// check if point is inside of a road object
  bool IsInside(Vector3Val pos, float sizeXZ, float sizeY) const;
  void SerializeBin(SerializeBinStream &f, Landscape *land);
  
  /// maintain a suppression list - add new source + bullet
  void TraceSuppression(
    EntityAI *source, Vector3Par srcPos, Vector3Par srcDir,
    float ammoHit, float distanceLeft, Vector3Par tBeg, Vector3Par tEnd
  );
  
  /// check suppression
  float GetSuppressionCost(const SuppressTargetList &list) const
  {
    if (!_suppressed) return 0;
    return _suppressed->GetTotalCost(list);
  }
  
  USE_FAST_ALLOCATOR

// implementation of RoadNode functions
friend class RoadNode;
  private:
    /// return the 2D position of the node for soldier or vehicle of given width
    Vector3 GetNodePositionXZ(int index, bool soldier, float vehWidth) const;
    /// return the 3D position of the node for soldier or vehicle of given width
    Vector3 GetNodePosition(int index, bool soldier, float vehWidth) const;
    /// return the number of nodes we are connected to
    int NNodeConnections(int index, bool soldier) const;
    /// return the node we are connected to
    RoadNode GetNodeConnection(int index, bool soldier, int i) const;
    /// found to which node we will come from the given road
    RoadNode GetEnterNode(const RoadLink *from, bool soldier) const;
    /// translate semantic index of bridge (connection #) to node index
    int GetBridgeIndex(int conIndex) const;

    /// direction of path from this node, for vehicles only
    Vector3 GetNodeDirection(int index) const;
};

inline Vector3 RoadNode::Position(bool soldier, float vehWidth) const {return _road->GetNodePosition(_index, soldier, vehWidth);}
inline Vector3 RoadNode::Direction() const {return _road->GetNodeDirection(_index);}
inline int RoadNode::NConnections(bool soldier) const {return _road->NNodeConnections(_index, soldier);}
inline RoadNode RoadNode::GetConnection(bool soldier, int i) const {return _road->GetNodeConnection(_index, soldier, i);}

class RoadListFull: public AutoArray< SRef<RoadLink> >
{
  public:
  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

// assume many RoadLists will be empty
class RoadList
{
  SRef<RoadListFull> _list;

  public:

  RoadList() {}
  RoadList(RoadListFull *list) {_list = list;}
  int Size() const {return ( _list ? _list->Size() : 0 );}
  RoadListFull *GetList() const {return _list;}
  RoadLink *operator [] ( int i ) const {Assert(_list);return (*_list)[i];}
  void Replace( int index, RoadLink *link ) const {Assert(_list);_list->Set(index)=link;}
  bool Null() const {return _list == NULL;}
};

TypeIsMovable(RoadList)

// #include <Es/Containers/array2D.hpp>
#include <Es/Containers/quadtreeEx.hpp>

struct QuadTreeExTraitsRoadList
{
  enum {LogSizeX = 0};
  enum {LogSizeY = 0};
  typedef RoadList Type;
  static bool IsEqual(const RoadList &a, const RoadList &b)
  {
    return a.Null() && b.Null();
  }
  static const RoadList &Get(const RoadList &value, int x, int y)
  {
    // Assert(x >= 0 && x < (1 << LogSizeX));
    // Assert(y >= 0 && y < (1 << LogSizeY));
    Assert(x == 0);
    Assert(y == 0);
    return value;
  }
  static void Set(RoadList &value, int x, int y, const RoadList &item)
  {
    // Assert(x >= 0 && x < (1 << LogSizeX));
    // Assert(y >= 0 && y < (1 << LogSizeY));
    Assert(x == 0);
    Assert(y == 0);
    value = item;
  }
};

class RoadNet: public RefCount
{
  public:
  // road information kept separate from objects (in object-like manner)
  // TODO: consider using some pointer from object to _road

  typedef QuadTreeExRoot<RoadList, QuadTreeExTraitsRoadList> QuadTreeExRootRoadList;
  
  private:
  QuadTreeExRootRoadList _roads;

  // RoadList &SelectRoadList( float x, float z );
  void AddRoad(float x, float z, RoadLink *elem);

  public:
  RoadNet();
  ~RoadNet();

  void Scan( Landscape *land ); // scan landscape
  void Connect(bool pass1Only); // connect as necessary
  void Optimize(); // merge small straight elements
  void Compact(); // optimize memory picture
  void Clear();

  void SerializeBin(SerializeBinStream &f, Landscape *land);

  void Build( Landscape *land ); // all steps together

  const RoadLink *IsOnRoad(Vector3Par pos, float sizeXZ, float sizeY) const;
  bool IsOnRoadReady(Vector3Par pos) const;
  Vector3 GetNearestRoadPoint(Vector3Par from, Vector3Par pos, float minDist, float vehSize, bool soldier, float vehWidth) const;
  const RoadList &GetRoadList( int xx, int zz ) const; // used for diags

/*
  bool IsLocked(Vector3Par pos, float size, bool soldier) const;
*/
};

inline const RoadList &RoadNet::GetRoadList( int xx, int zz ) const
{
  if( !InRange(xx,zz) )
  {
    // find nearest in-range square and use it
    if( xx<0 ) xx=0;else if( xx>LandRange-1 ) xx=LandRange-1;
    if( zz<0 ) zz=0;else if( zz>LandRange-1 ) zz=LandRange-1;
    Assert( InRange(xx,zz) );
  }
  return _roads(xx,zz);
}

#include "Es/Containers/offTree.hpp"
struct RoadVertex : public OffTreeRegion<float>
{
  static float epsilon;
  VertexIndex vertex;  //OffTree is build for one segment shape (index into its vertexTable)

  RoadVertex() {}
  RoadVertex(float x, float z, VertexIndex ix) : OffTreeRegion<float>(x-epsilon, z-epsilon, x+epsilon, z+epsilon) {vertex=ix;}
  const OffTreeRegion<float> &GetRegion() const { return *this; }
  bool operator==(const RoadVertex& obj) const
  {
    return (_begX==obj._begX && _begZ==obj._begZ);
  }
};
TypeIsSimple(RoadVertex)

const float RoadPrecTex=0.01f;
const float RoadPrecP=0.1f;

template <int StorageSize, int MinGrw=4>
class MemAllocSimpleRoad : public MemAllocSimple<MemAllocSimpleStorage<RoadVertex, StorageSize> >
{
public:
  ///
  /**
  @return Minimal recommended size in items for optimal usage of this allocator
  */
  static inline int MinGrow() {return MinGrw;}
};

/// traits for OffTree property implementation
template <int StorageSize>
struct OffTreeRoadTraits: public OffTreeDefTraits<RoadVertex, MemAllocSimpleRoad<StorageSize> >
{
  typedef AutoArray<RoadVertex, MemAllocSimpleRoad<StorageSize> > Container;
  MemAllocSimpleRoad<StorageSize> alloc;
  MemAllocSimpleStorage<RoadVertex, StorageSize> storage;
  
  void SetStorage(Container &container) 
  {
    container.SetStorage(alloc);
  }
  OffTreeRoadTraits() 
  {
    alloc.SetStorage(storage);
  }
};

//typedef OffTree<RoadVertex> OffTreeRoad; //mCnt==10800
//typedef OffTree<RoadVertex, OffTreeRoadTraits<16384> > OffTreeRoad; //mCnt==3212
  typedef OffTree<RoadVertex, OffTreeRoadTraits<4096>, MemAllocSimpleDirect<int,5000> > OffTreeRoad; //mCnt==2900
extern void MergeRoads(Shape *roads, const Shape *with, const LODShape *lShape, const Matrix4 &transform, OffTreeRoad *vertexTree=NULL, bool quickMerge=false, bool callCondense=true);

#if _HELISIM_AMBIENT_TRAFFIC

class AmbientCar: public ObjectPlain
{
public:
  typedef ObjectPlain base;
  AmbientCar(LODShape *shape, const CreateObjectId &id):base(shape, id) {}
  // we want cars to be visible in the distance
  virtual float MinPixelArea(float minPixelAuto) const { return floatMin(1.0,minPixelAuto*0.5f); }
};

// One polyline of ambient roads.
// The smallest piece that can have its speed and density changed.
class AmbientRoad: public RefCount
{
  friend class AmbientRoadNetwork;
  friend struct OffTreeRegionTraits< Ref<AmbientRoad> >;

  Vector3 _points[2];            /// segment coords
  float _len;                    /// segment length
  OffTreeRegion<float> _region;  /// bounding box for OffTree

  AutoArray<float> _phases;      /// trajectory phases (4 lowest bits encode position in _neighbors)
  unsigned16 _prevMask, _nextMask;
  AutoArray<AmbientRoad*> _prevR;
  AutoArray<AmbientRoad*> _nextR;

  bool _sticky;

  /// Hashes for car jittering / shape randomization. They return a number in [0,1).
  float CarHash(int code) { unsigned k = code*0xDEADBEEF; k ^= k>>9; k += k<<15; return k * (1.f/4294967296.f); }
  float CarHash2(int code) { unsigned k = code*0xCAFEBABE; k ^= k>>7; k += k<<19; return k * (1.f/4294967296.f); }

public:
  float _speed;  // speed in m/s
  float _gap;    // average gap between cars in m

  AmbientRoad() {}

  void Seal()  // compute remaining auxiliary variables: _region, _len
  {
    _len = _points[1].DistanceXZ(_points[0]);

    const float eps = 1;
    float minX = floatMin(_points[0].X(), _points[1].X()) - eps, maxX = floatMax(_points[0].X(), _points[1].X()) + eps;
    float minZ = floatMin(_points[0].Z(), _points[1].Z()) - eps, maxZ = floatMax(_points[0].Z(), _points[1].Z()) + eps;
    _region = OffTreeRegion<float>(minX, minZ, maxX, maxZ);
  }
  ~AmbientRoad() { _phases.Clear(); _prevR.Clear(); _nextR.Clear(); }

  struct AmbientCarParams
  {
    Vector3 position;
    Vector3 direction;
    float shapeHash;
    bool sticky;
  };

  /// draw cars in time t
  void GenerateTraffic(float t, AutoArray<AmbientCarParams>& resultArray);

  /// return size of everything
  int StorageSize(void) const { return sizeof(AmbientRoad) + _phases.StorageSize() + _prevR.StorageSize() + _nextR.StorageSize(); }
};

TypeIsGeneric(AmbientRoad);
TypeIsSimpleZeroed(AmbientRoad::AmbientCarParams);


/// Traits for putting polylines into the OffTree.
template <>
struct OffTreeRegionTraits< Ref<AmbientRoad> >
{
  typedef OffTreeRegion<float> Region;
  static Region GetRegion(AmbientRoad const* p) { return p->_region; }
  static bool IsIntersection(AmbientRoad const* p, Region const& r) { return p->_region.IsIntersection(r); }
  static bool IsEqual(AmbientRoad const* p1, AmbientRoad const* p2) { return p1==p2; }
};


class AmbientRoadNetwork: public RefCount
{
public:
  struct AmbientCarType {
    Ref<LODShape> shape;
    float landContactY;
    float landContactZFront;
    float landContactZBack;
    AutoArray<Vector3> lightsFront;
    AutoArray<Vector3> lightsBack;

    // save reallocations
    int carPoolLast;
    RefArray<Object> carPool;

    // save reallocations
    int lightPoolLast;
    RefArray<LightPointEntity> lightPool;
  };

private:
  OffTree< Ref<AmbientRoad> > _tree;
  AutoArray<AmbientCarType> _carTypes;
  bool _initialized;
  bool _on;
  float _avgTrajectoriesPerRoad;  // TODO: use it in computing AmbientRoad::GenerateTraffic::gap

  // save allocation
  AutoArray<AmbientRoad::AmbientCarParams> _carParams;

public:
  static const unsigned minimumGap = 2;  // meters

  AmbientRoadNetwork() { _initialized = false; _on = true; _avgTrajectoriesPerRoad = 15*2; }
  ~AmbientRoadNetwork() { Clear(); }

  /// Return the old state.
  bool SetEnabled(bool on) { bool old = _on; _on = on; return old; }
  bool IsEnabled() { return _on; }
  float GetAvgTrajectoriesPerRoad() { return _avgTrajectoriesPerRoad; }

  int Load(ParamEntryPar par);

  void Clear() { 
    if (_initialized) {
      _tree.Clear();
      _carTypes.Clear();
      _initialized = false;
    }
  }

  /// Generate traffic for the whole rectangle.
  void GenerateTraffic(float t, float x0, float x1, float z0, float z1);
  /// Set inter-car gap for the whole rectangle. Setting it to less than minimumGap turns traffic off.
  void SetGap(float gap, float x0, float x1, float z0, float z1);
  /// Set traffic speed for the whole rectangle.
  void SetSpeed(float speedKmph, float x0, float x1, float z0, float z1);
};

TypeIsGeneric(AmbientRoadNetwork::AmbientCarType);

#endif


#endif
