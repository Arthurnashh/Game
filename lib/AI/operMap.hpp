#ifdef _MSC_VER
#pragma once
#endif

#ifndef _OPER_MAP_HPP
#define _OPER_MAP_HPP

///////////////////////////////////////////////////////////////////////////////
// class OperMap

#include "../pathSteer.hpp"
#include "../object.hpp"
#include "../roads.hpp"
#include "supressId.hpp"

#ifndef _MSC_VER
  #include "../vehicleAI.hpp"
#endif
#include <Es/Containers/quadtree.hpp>
#include <Es/Containers/maps.hpp>

#define Directions                20
#define direction_delta           directions20

extern int directions20[20][3];

#ifndef DECL_ENUM_COMBAT_MODE
#define DECL_ENUM_COMBAT_MODE
DECL_ENUM(CombatMode)
#endif

/// one item stored in the planned path
struct OperInfo
{
  float _cost;

  /// what field type we are
  SizedEnum<ASONodeType> _type;
  SizedEnum<Clearance> _clearance;

  OperMapIndex _x, _z;
  
  /// house information
  OLink(Object) _house;
  int _housePos;

  /// road information
  RoadNode _road;


  OperInfo() : _x(-1), _z(-1), _road(NULL, -1) {_type = NTGrid; _cost = 0; _housePos = -1; _clearance = ClearanceNormal;}
  
  Vector3 GetPathPos(float combatHeight, bool soldier, float vehWidth) const;
};
TypeContainsOLink(OperInfo)

#ifndef DECL_ENUM_OPER_ITEM_TYPE
#define DECL_ENUM_OPER_ITEM_TYPE
DECL_ENUM(OperItemType)
#endif
DEFINE_ENUM_BEG(OperItemType)
  OITNormal,
  OITAvoidBush,
  OITAvoidTree,
  OITAvoid,
  OITWater,
  OITSpaceRoad,
  OITRoad,
  OITSpaceBush,
  OITSpaceHardBush,
  OITSpaceTree,
  OITSpace,
  OITRoadForced,
  //OITWater,
  NOperItemType
DEFINE_ENUM_END(OperItemType)


/// sometimes it might be useful to know which object are rendered
#define STORE_OBJ_ID 0

/// one item used for operative path finding
struct OperItem
{
  /// packed content, parts are accessible through functions
  /** used functions and not bitfields, as bitfields result in 32b representation */
  unsigned short _content;
/*
  /// type as viewed by vehicles (more space around obstacles, high vertical clearance)
  unsigned _type:7;
  /// type as viewed by soldiers (little space around obstacles, low vertical clearance)
  unsigned _typeSoldier:7;
  /// the house exit is present
  unsigned _houseExitPresent:1;
*/
  static const int typeBits = 6;
  static const int presentBits = 1;
  static const int clearanceBits = 2;

  static const int typeOffset = 0;
  static const int typeSoldierOffset = typeOffset + typeBits;
  static const int presentOffset = typeSoldierOffset + typeBits;
  static const int clearanceOffset = presentOffset + presentBits;

  static const int typeMask = ((1 << typeBits) - 1) << typeOffset;
  static const int typeSoldierMask = ((1 << typeBits) - 1) << typeSoldierOffset;
  static const int presentMask = ((1 << presentBits) - 1) << presentOffset;
  static const int clearanceMask = ((1 << clearanceBits) - 1) << clearanceOffset;

#if STORE_OBJ_ID
  /// id of the object for the _typeSoldier field as listed in the OperFieldFull
  short _objSoldier;
#endif

  OperItemType GetType() const {return (OperItemType)(int)((_content & typeMask) >> typeOffset);}
  OperItemType GetTypeSoldier() const {return (OperItemType)(int)((_content & typeSoldierMask) >> typeSoldierOffset);}
  bool IsHouseExitPresent() const {return (_content & presentMask) != 0;}
  Clearance GetClearance() const {return Clearance((_content & clearanceMask) >> clearanceOffset);}

  void SetType(OperItemType type) {_content &= ~typeMask; _content |= (type << typeOffset);}
  void SetTypeSoldier(OperItemType type) {_content &= ~typeSoldierMask; _content |= (type << typeSoldierOffset);}
  void SetClearance(Clearance clearance) {_content &= ~clearanceMask; _content |= (clearance << clearanceOffset);}
  void SetHouseExitPresent(bool set) {if (set) _content |= presentMask; else _content &= ~presentMask;}
};

///////////////////////////////////////////////////////////////////////////////
// class OperField

#define MASK_AVOID_OBJECTS    1
#define MASK_AVOID_VEHICLES   2
#define MASK_PREFER_ROADS     4
/// if the field does not exist, do not create a new one
#define MASK_NO_CREATE        8

#define MASK_USE_BUFFER       16

/// never update the position of the field in LRU lists - used for debugging
#define MASK_NO_UPDATE        32

/// simplified field processing is possible when we do not need the field for pathing
#define MASK_FOR_PATHING      64

/// info about a single building exit
struct OperHouseExit
{
  /// index of position in the model
  int _pos;
  /// grid coordinate of exit position - 0..LandRange * OperItemRange 
  int _x;
  /// grid coordinate of exit position - 0..LandRange * OperItemRange 
  int _z;
};
TypeIsSimple(OperHouseExit)

/// info about the connection from a house to other one
struct OperHouseConnection
{
  /// index of position in the current house
  int _posFrom;
  /// index of the connected house
  int _houseTo;
  /// index of position in the connected house
  int _posTo;
};
TypeIsSimple(OperHouseConnection)

/// info about a (passable) building in the OperField
struct OperHouse
{
  /// building object 
  OLink(Object) _house;
  /// list of available exits
  AutoArray<OperHouseExit> _exits;
  /// list of connections to other houses
  AutoArray<OperHouseConnection> _connections;
};
TypeContainsOLink(OperHouse)

/// traits for key comparison and searching
struct OperHouseTraits
{
  typedef const Object *KeyType;
  /// check if two keys are equal
  static bool IsEqual(KeyType a, KeyType b) {return a == b;}
  /// get a key from an item
  static KeyType GetKey(const OperHouse &a) {return a._house;}
};

/// info about the road node in the OperMap
struct OperRoadNode
{
  /// the exact position
  Vector3 _pos;
  /// grid coordinate of road position - 0..LandRange * OperItemRange 
  OperMapIndex _x;
  /// grid coordinate of road position - 0..LandRange * OperItemRange 
  OperMapIndex _z;
};
TypeIsSimple(OperRoadNode);

/// info about road segment in the OperMap
struct OperRoad
{
  /// road information
  const RoadLink *_road;
  /// move directly to / from the landscape is impossible
  bool _bridge;
  /// list of all nodes on this segment
  AutoArray<OperRoadNode, MemAllocLocal<OperRoadNode, 2> > _nodes; // in most cases segment contains 2 nodes
};
TypeIsGeneric(OperRoad)

/// traits for key comparison and searching
struct OperRoadTraits
{
  typedef const RoadLink *KeyType;
  /// check if two keys are equal
  static bool IsEqual(KeyType a, KeyType b) {return a == b;}
  /// get a key from an item
  static KeyType GetKey(const OperRoad &a) {return a._road;}
};

/// interface for one field of operative map (provides access to costs of the individual grid locations)
class OperField : public RefCount,public LinkBidirWithFrameStore
{
friend class OperCache;
friend class OperMap;
friend class DebugWindowOperMap;
friend class ASOIterator;

protected:
  /// houses in the field
  AutoArray<OperHouse> _houses;

  LandIndex _x; //!< square grid coordinate - 0..LandRange
  LandIndex _z; //!< square grid coordinate - 0..LandRange

  Time _lastUsed;

public:
  OperField(LandIndex x, LandIndex z) : _x(x), _z(z) {}

  bool IsField(LandIndex x, LandIndex z) const {return x == _x && z == _z;}
  LandIndex X() const {return _x;}
  LandIndex Z() const {return _z;}
  virtual OperItem GetItem(OperFieldIndex x, OperFieldIndex z) const = 0;
  /// list cover candidates for a given location
  virtual const OperCover *GetCover(OperFieldIndex xs, OperFieldIndex zs, int &nCover) const = 0;
  /// list cover candidates for whole field
  virtual const OperCover *GetCover(int &nCover) const = 0;
  
  /// compute memory requirements
  virtual size_t GetMemoryControlled() const = 0;

  /// make sure the field can be used for path finding
  virtual void PrepareForPathing() = 0;
  
  /// check if PrepareForPathing was already done
  virtual bool ReadyForPathing() const = 0;
  
  /// return time (in seconds) when the field was last used
  float GetAge() const;
};

class AIBrain;

/// interface for one field of operative map (provides access to costs of the individual grid locations)
class SuppressField : public RefCount,public TLinkBidirRef
{
friend class SuppressCache;
friend class OperMap;
friend class ASOIterator;

protected:
  LandIndex _x; //!< square grid coordinate - 0..LandRange
  LandIndex _z; //!< square grid coordinate - 0..LandRange
  /// list of the suppression sources in this field

  Time _lastUsed;

public:
  SuppressField(LandIndex x, LandIndex z) : _x(x), _z(z) {}

  bool IsField(LandIndex x, LandIndex z) {return x == _x && z == _z;}
  LandIndex X() const {return _x;}
  LandIndex Z() const {return _z;}
  
  /// notification about a destroyed target
  virtual void RemoveSource(EntityAI *source) = 0;

  /// access cost map
  virtual float GetCost(int x, int z, const SuppressTargetList &list) const = 0;
};


#include "El/FreeOnDemand/memFreeReq.hpp"

/// interface to operative path finding map manager
class IOperCache
{
public:
  virtual OperField* GetOperField(LandIndex x, LandIndex z, int mask) = 0;
  virtual void RemoveField(OperField*fld) = 0;
  virtual void RemoveField(LandIndex x, LandIndex z) = 0;
  virtual void Update() = 0;

  virtual ~IOperCache() {}

  virtual int NFields() const = 0;
  /// direct access to the cache (diagnostics)
  virtual const OperField *CheckField(LandIndex x, LandIndex z) const = 0;
};

IOperCache *CreateOperCache(Landscape *land);

/// interface to suppressive fire map manager
class ISuppressCache
{
public:
  virtual SuppressField* GetField(LandIndex x, LandIndex z, int mask) = 0;
  virtual void RemoveField(SuppressField*fld) = 0;
  virtual void RemoveField(LandIndex x, LandIndex z) = 0;
  virtual void Update() = 0;
  
  /// compute suppression level at given spot
  virtual float GetSuppressionLevel(Vector3Par pos, float radius, AIBrain *unit) = 0;
  
  /// rasterize a bullet line
  virtual void Trace(EntityAI *source, float ammoHit, float distanceLeft, Vector3Par beg, Vector3Par end) = 0;

  virtual ~ISuppressCache() {}

  virtual int NFields() const = 0;
};

ISuppressCache *CreateSuppressCache(Landscape *land);



//! callback function type - see also FindFreePositionCallback
typedef bool FindNearestEmptyCallback(Vector3Par pos, void *context);

class IAStarOperative;

/// current requirements for a cover
struct CoverDisposition
{
  //@{ position against which we are covering
  Vector3 _hideFromL;
  Vector3 _hideFromR;
  //@}
  /// position to which we want to be closer than we are now
  PlanPosition _movingTo;
  /// max. distance of the cover from current position (when negative, no cover is ever tested)
  float _maxCost;
  
  /// max. delay caused by moving to the cover
  /**
  This is a difference between cost of path beg..cover..end to beg..end
  */
  float _maxDelay;
};

/// item of the local cache of fields currently used by the OperMap
struct OperMapField
{
  RefD<OperField> _operField;
  Ref<SuppressField> _suppressField;
  Ref<LockField> _lockField;
  /// EntityAIType::GetBaseCost * EntityAIType::GetFieldCost ... cover also conditions not included in operative path planning - roads, objects etc.
  float _heurCost;
  /// EntityAIType::GetBaseCost ... cost common for both strategic and operative path planning - gradient, water, forest etc.
  float _baseCost;
  /// same as _baseCost, but not including the gradient penalty
  float _baseCostWithoutGradient;

  OperMapField(OperField *field) : _operField(field) {}
  bool operator ==(const OperMapField &with) const {return _operField == with._operField;}
  bool operator !=(const OperMapField &with) const {return _operField != with._operField;}
};
TypeIsMovableZeroed(OperMapField);

unsigned int GetKeyOperMapField(const OperMapField &field);

/// enable to check what delay is between path finding started and finished
#define DIAG_DELAY 0

/// allow searching of the operative map through a job
#define USE_OPERPATH_THREAD 1

#if USE_OPERPATH_THREAD
#include <El/Multicore/jobs.hpp>
#endif

/// rasterized map for A* searching
class OperMap : private NoCopy
#if USE_OPERPATH_THREAD
  , public Job
#endif
{
friend class AIBrain;
friend class Path;
friend class DebugWindowOperMap;
friend class ASOCostFunction;
friend class ASOHeuristicFunction;
friend class ASOIterator;
protected:
  /// rasterized fields
  ImplicitMap<unsigned int, OperMapField, GetKeyOperMapField, false> _fields;

  // AutoArray<OperMapField, MemAllocLocal<OperMapField, 256> > _fields;
  /// list of entry points into buildings
  /** this is gathered from all OperFields so that ASOIterator can access it */
  FindArrayKey<OperHouse, OperHouseTraits, MemAllocLocal<OperHouse, 256> > _houses;

  /// list of all road segments (and nodes) in the map
  FindArrayKey<OperRoad, OperRoadTraits, MemAllocLocal<OperRoad, 256> > _roads;

  /// snapshot of target list needed for suppress cost calculation
  SuppressTargetList _suppressTargets; 

  /// path not found to the destination, the node closest to it was selected instead
  bool _alternateGoal;
  /// destination is on bridge - do not check accessibility in map
  bool _endOnRoadOnly;
  /// even when only alternate goal was found, do not set the final target to it - better path can be found later
  bool _replanLater;

#if USE_OPERPATH_THREAD
  //@{ _field access failure info
  bool _failure;
  LandIndex _failureX, _failureZ;
  //@}
#endif

  /// how many iterations were already used (may be reset when 2nd pass is started)
  int _iterOp;

  /// locker of the vehicle searching the path
  TaskOwnerRef<AILocker> _locker;
  /// vehicle type for which we are performing the search
  TaskOwnerRef<EntityAIType> _vehType;
  /// what combat mode is the search in?
  CombatMode _mode;
  /// is the search done for a soldier?
  bool _isSoldier;

  /// create _houses array (needed only for pathfinding, not for FindNearestEmpty etc.)
  bool _connectHouses;

#if _ENABLE_REPORT
  /// needed for some diagnostics
  bool _isCameraVehicle;
  /// needed for some diagnostics
  RString _debugName;
  
  public:
  
#endif

#if _ENABLE_AI
  SRef<IAStarOperative> _algorithm;
  /// when searching 
  bool _pass2;
  
  float _heuristicLimit;

  /// cost of the shortest path found
  float _pathCost;
#endif

public:
  //@{ exported path
  AutoArray<OperInfo, MemAllocLocal<OperInfo,256> > _path;
  /// which cover the exported path leads to
  CoverInfo _pathToCover;
  /// if no cover was found, we may want to be like in cover even when in the open
  bool _pathToOpenCover;
  //@}

#if DIAG_DELAY
  /// diagnostics - time when the path searching started
  DWORD _findStarted;
#endif

  explicit OperMap(bool connectHouses = false);
  ~OperMap();

  void ClearMap();
  void CreateMap(
    EntityAI *veh, LandIndex xMin, LandIndex zMin, LandIndex xMax, LandIndex zMax,
    int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER
  );
  void CreateMap(
    const EntityAIType *vehType, bool isSoldier, CombatMode mode, LandIndex xMin, LandIndex zMin, LandIndex xMax, LandIndex zMax,
    int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER
  );

  CoverInfo FindNearestCover(Vector3Par pos,  EntityAIFull *veh, LandIndex xMin, LandIndex zMin, LandIndex xMax, LandIndex zMax) const;

#if _ENABLE_AI
  //! Initialize path searching
  /*!
    \return true if simple path found
  */
  bool InitFindPath(AIBrain *unit, const OperInfo &infoFrom, const OperInfo &infoTo, int dir, bool locks, const CoverDisposition &dispo);

  /// common code for InitFindPath / InitFindCover
  float PrepareInitFind(int iteration, bool endOnRoadOnly, float maxCost);
  //! Process maxIters iterations of path searching
  /*!
    \return number of iterations really processed
  */
  bool ProcessFindPath(AIBrain *unit);
  //! Export path from current searching
  /*!
    \return true if path found
  */
  bool ExportPath(AIBrain *unit);
  //! Searching was completed
  bool IsDone() const;
  //! Path was found
  bool IsFound() const;

  /// get cost of the best path to the destination, or if not found, the closest alternate end
  float GetBestPathCost() const;

/// check if search is in progress
  bool IsInProgress() const {return _algorithm.NotNull();}

#endif
  /// preload the given area to be sure further calls will not wait for data
  static bool OperFieldRangeReady(LandIndex xMin, LandIndex xMax, LandIndex zMin, LandIndex zMax);
  /// preload data needed to FindEmptyArea call
  static bool FindEmptyAreaReady(Vector3 &center, float radius, float maxDistance);

  bool IsSimplePath(const AIBrain *unit, OperMapIndex xs, OperMapIndex zs, OperMapIndex xe, OperMapIndex ze, float &simpleCost, float &simpleHeurCost);
#if _ENABLE_AI
  /// convert path from OperMap to AIBrain
  void ResultPath(AIBrain *unit, bool prepend, Vector3Par prependPos);
#endif
  bool IsAlternateGoal() const {return _alternateGoal;}
  bool CanReplanLater() const {return _replanLater;}

  bool IsDestinationFree(AIBrain *unit);

  bool IsDestinationNotLocked(AIBrain *unit);

  /// used when path is forced
  void SetAlternateGoal(bool set) {_alternateGoal = set;}

  /// render 3D diagnostics for the search
  void Path3DDiags(AIBrain *unit) const;
  
  /// get list of all cover candidates present in the given OperItemGrid
  const OperCover *GetCover(OperMapIndex xs, OperMapIndex zs, int &nCover) const;

  /// return the interface to the given house
  const IPaths *GetHouse(int houseIndex) const;

  /// find the house in the list
  int GetHouseIndex(Object *house) const;

  /// return the interface to the given road segment
  const OperRoad &GetRoad(int roadIndex) const
  {
    return _roads[roadIndex];
  }

  /// fast check if house exit is present on this position
  bool IsHouseExit(OperMapIndex x, OperMapIndex z); // cannot be const, new field can be created

  /// find connections from house to other house
  void ConnectHouses(int index1, int index2);

#if USE_OPERPATH_THREAD
  /// the main Job procedure
  virtual bool Process();
#endif

  // implementation
protected:
  /// search for the field in the internal list
  // !!! pointer into AutoArray - can be used only temporarily !!!
  const OperMapField *GetField(LandIndex x, LandIndex z) const;
  FieldCost GetFieldCost(OperMapIndex x, OperMapIndex z, bool locks, const EntityAIType *type, CombatMode mode, bool soldier, bool suppress, bool includeGradient = true);
  bool GetFieldLocked(OperMapIndex x, OperMapIndex z, const EntityAIType *type, CombatMode mode, bool soldier);
  Clearance GetFieldClearance(OperMapIndex x, OperMapIndex z, const EntityAIType *type, CombatMode mode, bool soldier);
  
  OperItem CheckOperItem(OperMapIndex x, OperMapIndex z) const;
  /// find nearest empty field in this map
  bool FindNearestEmpty(
    OperMapIndex &x, OperMapIndex &z, float xf, float zf,
    OperMapIndex xbase, OperMapIndex zbase, OperMapIndex xsize, OperMapIndex zsize, bool locks,
    const EntityAIType *vehType, bool soldier, bool insideBuilding, // allow positions in buildings other than exits
    FindNearestEmptyCallback *isFree, void *context
  );

  /// find an empty field with a specified region on this map
  bool FindEmptyInRect(
    OperMapIndex &x, OperMapIndex &z, const FrameBase &pos, const Vector3 *minmax, Vector3Par nearTo,
    bool locks, const EntityAIType *vehType, bool soldier, bool insideBuilding,
    FindNearestEmptyCallback *isFree, void *context
  );

  /// search for the free area nearest to the given position
  bool FindEmptyArea(Vector3 &center, float radius, float maxDistance, const EntityAIType *vehType);
  /// helper function for FindEmptyArea - check if given area is free
  bool IsAreaFree(OperMapIndex x, OperMapIndex z, float radiusO);

  /// get current number of nodes visited
  /** used for diagnostics purposes */
  RString GetDiagText(bool cover) const;

  /// effective pathfinding procedure, used both from ProcessFindPath (singlecore) or ThreadProcedure (multicore)
  bool Update();

  /// check if field is locked (by other unit than myself)
  bool IsLocked(OperMapIndex x, OperMapIndex z, const OperMapField *field, OperFieldIndex xx, OperFieldIndex zz, bool soldier) const;
  /// check if building node is locked (by other unit than myself)
  bool IsHouseLocked(int houseIndex, int pt) const;
  /// check if road segment is locked (by other unit than myself)
  bool IsRoadLocked(const RoadLink *road, bool soldier) const;

public:  
  void LogMap(LandIndex xMin, LandIndex xMax, LandIndex zMin, LandIndex zMax, 
    OperMapIndex xs, OperMapIndex xe, OperMapIndex zs, OperMapIndex ze, bool path);
  bool IsIntersection(
    OperMapIndex xs, OperMapIndex zs, OperMapIndex xe, OperMapIndex ze,
    float &cost, float &costPerItem, const EntityAIType *vehType, CombatMode mode, bool soldier
  );
  /// find first intersection with inaccessible field
  float FirstIntersection(
    float xsf, float zsf, float xef, float zef,
    const EntityAIType *vehType, CombatMode mode, bool soldier
  );
  // !!! reference into AutoArray - can be used only temporarily !!!
  const OperMapField &CreateField(LandIndex x, LandIndex z, int mask, const EntityAIType *vehType, CombatMode mode, bool soldier);

#if USE_OPERPATH_THREAD
  /// suspend the task until field x, z is rasterized
  void AskForField(LandIndex x, LandIndex z);
#endif

  /// get node position for diagnostic purposes
  Vector3 GetNodePos(const void *node) const;
  
  //! Init all internal structures
  void Clear(int xRange=0, int zRange=0);

  #if _ENABLE_REPORT
  const RString &GetDebugName() const {return _debugName;}
  #endif
};

#endif
