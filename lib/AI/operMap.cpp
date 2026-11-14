// implementation of OperMap 

#include "../wpch.hpp"

#include "ai.hpp"
#include "aiDefs.hpp"
#include "operMap.hpp"
#include "../landscape.hpp"
#include <El/DebugWindow/debugWin.hpp>
#include "../house.hpp"

#include <El/Common/perfProf.hpp>
#include <El/Common/perfLog.hpp>
#include <Es/Algorithms/qsort.hpp>
#include "../diagModes.hpp"
#include "../objectClasses.hpp" //required for road width - walk / drive on the correct side of the road
#include "../global.hpp"
#include "../roads.hpp"

#if _VBS3_LARGE_OBJECTS
#define MAKE_OBJECT_LIST_XXL(LIST,land,IX,IZ) \
  const ObjectListUsed& LIST##_use = land->UseObjects(IX,IZ); \
  const ObjectListUsedXXL& LIST = land->UseObjectsXXL(LIST##_use,IX,IZ);
#else
#define MAKE_OBJECT_LIST_XXL(LIST,land,IX,IZ) \
  const ObjectListUsed& LIST = land->UseObjects(IX,IZ);
#endif

#if 0 //_PROFILE
#pragma optimize( "", off )
#endif

#include <Es/Algorithms/aStar.hpp>

#include <Es/Memory/normalNew.hpp>

#include <utility>

#define DIAG 0
#define DIAG_PATH   0

class OperFieldSimple : public OperField
{
protected:
  OperItem _item;

public:
  OperFieldSimple(LandIndex x, LandIndex z, OperItem item)
  : OperField(x, z)
  {
    _item = item;
  }

  virtual OperItem GetItem(OperFieldIndex x, OperFieldIndex z) const {return _item;}
  virtual const OperCover *GetCover(OperFieldIndex xs, OperFieldIndex zs, int &nCover) const {nCover=0;return NULL;}
  virtual const OperCover *GetCover(int &nCover) const {nCover=0;return NULL;}
  virtual size_t GetMemoryControlled() const {return sizeof(*this);}
  virtual void PrepareForPathing() {}
  virtual bool ReadyForPathing() const {return true;} // always ready for pathing
  
  USE_FAST_ALLOCATOR
};

struct CoverField
{
  int beg,end;

  CoverField():beg(0),end(0){}
  CoverField(int b, int e):beg(b),end(e){}

  bool operator == (const CoverField &with) const {return beg==with.beg && end==with.end;}
  bool IsEmpty() const {return beg>=end;}
};

TypeIsMovableZeroed(CoverField)

/// rasterization of all objects participating in given field
/**
TODO: separate rasterization for soldiers and vehicles
*/
class OperFieldFull : public OperField
{
protected:
  /// list
  OperItem _items[OperItemRange][OperItemRange];

  /// mark all preparations needed for path finding are done
  bool _pathingReady;
  /// cover candidates in the field
  AutoArray<OperCover> _cover;

  /// map x/z to interval in the _cover array
  QuadTree<CoverField> _coverIndex;

#if STORE_OBJ_ID
  OLinkOLArray(Object) _objs;
  int AddObj(Object *obj);
#endif

  /// compute a CoverField for given coordinates
  CoverField GetCoverField(OperFieldIndex x, OperFieldIndex z) const;

public:
  OperFieldFull(LandIndex x, LandIndex z, int mask)
  : OperField(x, z)
  {
    CreateField(mask);
  }
  virtual OperItem GetItem(OperFieldIndex x, OperFieldIndex z) const {return _items[z][x];}
  virtual const OperCover *GetCover(OperFieldIndex x, OperFieldIndex z, int &nCover) const;
  virtual const OperCover *GetCover(int &nCover) const;
  virtual void PrepareForPathing();
  virtual bool ReadyForPathing() const {return _pathingReady;}

  bool IsSimple(OperItem &item) const;

protected:
  void CreateField(int mask);

  void ProcessCoverCorner(CoverType ct00, Vector3 &pt00, float surf1, Vector3 out00, int score);
  void ProcessCoverEdge(Vector3Par pt00, Vector3Par pt01, float surf1, float surf2, Vector3Par out00_01, int score);
  // add houses accessible from this field
  void AddHouses();

  void Rasterize(
    Object *obj, const Vector3 *minMax, const Vector3 *obbRect,
    OperItemType type, float xResize, float zResize, bool soldier,
    bool checkHeight, float minSurf, Clearance clearance
  );


  /// add all cover candidates
  void AddCover();
  /// remove cover positions which do not provide any cover
  void RemoveFalseCover();
  /// perform processing of the cover list into usable form
  void CloseCoverList();

  virtual size_t GetMemoryControlled() const
  {
    // we control some more memory, but the main storage is in the members, esp. in _items
    return sizeof(*this);
  }
  //USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

//static int ITERS_PER_FRAME = 40;
//static int ItersPerFrame = 120;
static int ItersPerFrame = 250;

/// Context passed to A* path finding iterator
struct ASOContext
{
  OperMap *_map;
  bool _useRoads;
  bool _isSoldier;
};

#if DIAG_A_STAR >= 2 || DIAG_PATH
static char *ASONodeTypeName(ASONodeType type)
{
  switch (type)
  {
  case NTRoad:
    return "road";
  case NTBridge:
    return "bridge";
  case NTHouse:
    return "house";
  case NTGrid:
    return "terrain";
  case NTError:
    return "error";
  default:
    return "";
  }
}
#endif

enum HouseFieldHelper {HouseField};
enum RoadFieldHelper {RoadField};
enum GridFieldHelper {GridField};
enum ErrorFieldHelper {ErrorField};

/// node for operative A* path finding
struct ASOField
{
private:
  /// node identification
  union
  {
    /// used as a key for hash table and to compare
    int key;
    /// coordinates of field for NTGrid
    struct
    {
      short int x;
      short int z;
    } coord;
    /// description of house position for NTHouse
    struct
    {
      /// index of house in OperMap::_houses
      short int houseIndex;
      /// index of position in the house
      short int posIndex;
    } housePos;
    /// RoadNet node
    struct
    {
      /// index of road segment in OperMap::_roads
      short int roadIndex;
      /// index of node in the road
      short int nodeIndex;
    } roadPos;
  } u;
public:
  /// arrival direction 0..255 (128=180degree)
  BYTE dir;
  /// unpaid direction change - 0..255 (128=180degree)
  signed char debet;
  /// what field type we are
  SizedEnum<ASONodeType> type;

  /// time needed to reach destination
  /**
  somewhat similar to node _g, but this time it is the real time, not cost.
  AI sometimes needs to prefer other path than the fastest one, because the fastest one is not safe.
  */
  float _t;
  
  ASOField(enum GridFieldHelper, OperMapIndex x, OperMapIndex z, int d)
  {
    u.coord.x = x; u.coord.z = z;
    dir = d; debet = 0; type = NTGrid;
    _t = FLT_SNAN;
  }
  ASOField(enum HouseFieldHelper, int houseIndex, int posIndex)
  {
    u.housePos.houseIndex = houseIndex; u.housePos.posIndex = posIndex;
    dir = 0; debet = 0; type = NTHouse;
    _t = FLT_SNAN;
  }
  ASOField(enum RoadFieldHelper, ASONodeType t, int roadIndex, int nodeIndex, int d)
  {
    Assert(t == NTRoad || t == NTBridge);
    u.roadPos.roadIndex = roadIndex; u.roadPos.nodeIndex = nodeIndex;
    dir = d; debet = 0; type = t;
    _t = FLT_SNAN;
  }
  ASOField(enum ErrorFieldHelper)
  {
    u.key = 0;
    dir = 0; debet = 0; type = NTError;
    _t = FLT_SNAN;
  };

  bool operator == (const ASOField &with) const
  {
    return with.type == type && with.u.key == u.key;
  }
  struct Key
  {
    unsigned type;
    unsigned key;
    
    Key(SizedEnum<ASONodeType> t, int k): type(t.GetEnumValue()),key(k){}
    bool operator == (const Key &with) const {return with.type == type && with.key == key;}
    unsigned operator % (unsigned mod) const
    {
      return (unsigned)(((((unsigned long long)type)<<32)|key)%mod);
    
    }
  };
  Key GetKey() const {return Key(type,u.key);}

  OperMapIndex GetX() const
  {
    if (type == NTGrid)
    {
      return OperMapIndex(u.coord.x);
    }
    Fail("Unexpected field type");
    return OperMapIndex(-1);
  }
  OperMapIndex GetZ() const
  {
    if (type == NTGrid)
    {
      return OperMapIndex(u.coord.z);
    }
    Fail("Unexpected field type");
    return OperMapIndex(-1);
  }

  int GetHouseIndex() const {return type == NTHouse ? u.housePos.houseIndex : -1;}
  int GetPosIndex() const {return type == NTHouse ? u.housePos.posIndex : -1;}

  int GetRoadIndex() const {return type == NTRoad || type == NTBridge ? u.roadPos.roadIndex : -1;}
  int GetNodeIndex() const {return type == NTRoad || type == NTBridge ? u.roadPos.nodeIndex : -1;}

#if DIAG_A_STAR >= 2
  RString GetDebugName() const
  {
    return Format("[%d, %d] %s", u.coord.x, u.coord.z, const_cast(FindEnumName(type)));
  }
#endif

  float Distance(const ASOField &from, const void *context) const;

  void GetXZ(const OperMap &map, OperMapIndex &x, OperMapIndex &z) const;
};

void ASOField::GetXZ(const OperMap &map, OperMapIndex &x, OperMapIndex &z) const
{
  switch (type.GetEnumValue())
  {
  case NTHouse:
    {
      const IPaths *paths = map.GetHouse(GetHouseIndex());
      if (paths)
      {
        Vector3 pos = paths->GetPosition(GetPosIndex());
        x = OperMapIndex(toIntFloor(pos.X() * InvOperItemGrid));
        z = OperMapIndex(toIntFloor(pos.Z() * InvOperItemGrid));
      }
      else
      {
        // avoid the crash
        x = OperMapIndex(0); z = OperMapIndex(0);
      }
    }
    break;
  case NTGrid:
    x = GetX();
    z = GetZ();
    break;
  case NTRoad:
  case NTBridge:
    {
      const OperRoad &info = map.GetRoad(GetRoadIndex());
      const OperRoadNode &node = info._nodes[GetNodeIndex()];
      x = node._x;
      z = node._z;
    }
    break;
  default:
    Fail("Unexpected field type");
    x = OperMapIndex(-1);
    z = OperMapIndex(-1);
    break;
  }
}

/// HOTFIX: handle at least some MT collisions for diagnostics
static inline bool DetectFreedMemory(int value)
{
  #if _ENABLE_CHEATS
  return value==0 || value==0x7f817f81 || value==0x7f8f7f8f; // MEM_FREE_VAL_32
  #else
  return false;
  #endif
}
static inline bool DetectFreedMemory(const void *ptr)
{
  return DetectFreedMemory(intptr_t(ptr));
}



float ASOField::Distance(const ASOField &from, const void *context) const
{
  if (DetectFreedMemory(this)) return FLT_MAX; // probably inside ASOClosedList::resize

  OperMapIndex xs, zs, xe, ze;

  const OperMap *map = reinterpret_cast<const OperMap *>(context);
  from.GetXZ(*map,xs,zs);
  GetXZ(*map,xe,ze);

  return Square(xs - xe) + Square(zs - ze);
}

typedef AStarNode<ASOField> ASONode;
TypeIsSimple(ASONode *)
DEFINE_FAST_ALLOCATOR(ASONode)

#define DIAG_ROAD_ITERATOR  0

class ASOCostFunction
{
protected:
  OperMap *_map;
  bool _locks;

public:
  ASOCostFunction(OperMap *map, bool locks)
  {
    _map = map;
    _locks = locks;
  }
  float operator () (const ASOField &field1, ASOField &field2) const;

protected:

  FieldCost GetFieldCost(OperMapIndex x, OperMapIndex z) const;
};

float ASOCostFunction::operator () (const ASOField &field1, ASOField &field2) const
{
  TaskPtr<EntityAIType> vehType = _map->_vehType;
  if (!vehType) return SET_UNACCESSIBLE;

  if (field1.type == NTError || field2.type == NTError) return SET_UNACCESSIBLE;

  // different calculation inside houses
  if (field2.type == NTHouse)
  {
    // first check if the node is not locked
    if (_map->IsHouseLocked(field2.GetHouseIndex(), field2.GetPosIndex())) return SET_UNACCESSIBLE;

    // get the exact position of the target node
    const IPaths *paths2 = _map->GetHouse(field2.GetHouseIndex());
    if (!paths2) return SET_UNACCESSIBLE; // if house is not found, do not try to access it

    Vector3 posT = paths2->GetPosition(field2.GetPosIndex());
    
    // get the exact position of the source node
    Vector3 posF;
    if (field1.type == NTHouse)
    {
      // inside house
      const IPaths *paths1 = _map->GetHouse(field1.GetHouseIndex());
      if (!paths1) return SET_UNACCESSIBLE; // if house is not found, do not try to access it
      posF = paths1->GetPosition(field1.GetPosIndex());
    }
    else
    {
      // entering house
      Assert(field1.type == NTGrid); // cannot enter from road
      posF[0] = field1.GetX() * OperItemGrid + 0.5f * OperItemGrid;
      posF[1] = posT.Y(); // avoid RoadSurfaceY, assume no distance in Y axis
      posF[2] = field1.GetZ() * OperItemGrid + 0.5f * OperItemGrid;
    }
    float cost = paths2->GetBType()->GetCoefInside() * vehType->GetMinCost() * posF.Distance(posT);
    field2._t = field1._t + cost;
    return cost;
  }

  FieldCost result;

  // different calculation on road net
  bool onRoad1 = field1.type == NTRoad || field1.type == NTBridge;
  bool onRoad2 = field2.type == NTRoad || field2.type == NTBridge;
  if (onRoad1 || onRoad2)
  {
    // coef based on the vehicle and combat mode
    float baseCost = OperItemGrid * vehType->GetMinCost();
    OperMapIndex x, z;
    field2.GetXZ(*_map, x, z);
    LAND_INDICES(x, z, xt0, zt0);
    const OperMapField *field = _map->GetField(xt0, zt0);
    // make sure the field is created - we will need it to expand neighbors
    if (!field)
    {
      // field not found - we have to wait for it or create it
#if USE_OPERPATH_THREAD
      // in the multicore environment, only signalize failure and end
      if (_map->IsJobRunning())
      {
        _map->AskForField(xt0, zt0);
        return 0;
      }
#endif
      int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER | MASK_FOR_PATHING;
      field = &_map->CreateField(xt0, zt0, mask, vehType, _map->_mode, _map->_isSoldier);
    }
    saturateMin(baseCost, field->_baseCostWithoutGradient);

    // calculate the real coordinates of the path segment
    Vector3 pos1;
    if (onRoad1)
    {
      const OperRoad &info = _map->_roads[field1.GetRoadIndex()];
      const OperRoadNode &node = info._nodes[field1.GetNodeIndex()];
      if (info._road->IsLocked(_map->_isSoldier))
        return SET_UNACCESSIBLE;
      pos1 = node._pos;
//       float surfY = GLandscape->SurfaceYAboveWaterNoWaves(pos1[0], pos1[2]);
//       Assert(fabs(pos1.Y()-surfY)<0.02f);
#if DIAG_ROAD_ITERATOR
      LogF("From road %d:%d", field1.GetRoadIndex(), field1.GetNodeIndex());
#endif
    }
    else
    {
      pos1[0] = (field1.GetX() + 0.5f) * OperItemGrid;
      pos1[2] = (field1.GetZ() + 0.5f) * OperItemGrid;
      pos1[1] = GLandscape->SurfaceYAboveWaterNoWaves(pos1[0], pos1[2]);
    }
    Vector3 pos2;
    float suppressionCost = 0;
    if (onRoad2)
    {
      const OperRoad &info = _map->_roads[field2.GetRoadIndex()];
      const OperRoadNode &node = info._nodes[field2.GetNodeIndex()];
      pos2 = node._pos;
      suppressionCost = info._road->GetSuppressionCost(_map->_suppressTargets);
      if (info._road->IsLocked(_map->_isSoldier))
        return SET_UNACCESSIBLE;
#if DIAG_ROAD_ITERATOR
      LogF("To road %d:%d", field2.GetRoadIndex(), field2.GetNodeIndex());
#endif
    }
    else
    {
      pos2[0] = (field2.GetX() + 0.5f) * OperItemGrid;
      pos2[2] = (field2.GetZ() + 0.5f) * OperItemGrid;
      pos2[1] = GLandscape->SurfaceYAboveWaterNoWaves(pos2[0], pos2[2]);
    }
    // distance and gradient
    float distXZ2 = pos1.DistanceXZ2(pos2);
    float invDistXZ = distXZ2 > 0 ? InvSqrt(distXZ2) : 0;
    float distY = pos2.Y() - pos1.Y();
    float gradient = distY * invDistXZ; // distY / distXZ

    FieldCost roadCost = vehType->GetRoadCost(_map->_mode);
    // distXZ2 * invDistXZ - distance in meters
    // (distXZ2 * invDistXZ) * InvOperItemGrid - distance in OperItemGrid
    // baseCost - cost per OperItemGrid
    result = roadCost * (distXZ2 * invDistXZ) * InvOperItemGrid * baseCost * vehType->GetGradientPenalty(gradient);
    result.cost += suppressionCost;
#if DIAG_ROAD_ITERATOR
    LogF(" - cost %.2f", result.cost);
#endif
    // continue to turning penalty
  }
  else
  {
    // grid coordinates of starting field
    OperMapIndex xt = field2.GetX();
    OperMapIndex zt = field2.GetZ();

    if (field1.type == NTHouse)
    {
      // leaving house
      const IPaths *paths = _map->GetHouse(field1.GetHouseIndex());
      if (!paths) return SET_UNACCESSIBLE; // if house is not found, do not try to use it
      Vector3 posF = paths->GetPosition(field1.GetPosIndex());
      Vector3 posT;
      posT[0] = xt * OperItemGrid + 0.5f * OperItemGrid;
      posT[1] = posF.Y(); // avoid RoadSurfaceY, assume no distance in Y axis
      posT[2] = zt * OperItemGrid + 0.5f * OperItemGrid;
      FieldCost costPerMeter = GetFieldCost(xt, zt) * InvOperItemGrid;
      FieldCost cost = costPerMeter * posT.Distance(posF);
      field2._t = field1._t + cost.time;
      return cost.cost;
    }

    // movement in terrain
    OperMapIndex xf = field1.GetX();
    OperMapIndex zf = field1.GetZ();

    const float H_SQRT5 = 2.2360679775;
    const float H_SQRT5_4 = 0.25 * H_SQRT5;
    const float H_SQRT2_2 = 0.5 * H_SQRT2;

    result = GetFieldCost(xf, zf) + GetFieldCost(xt, zt);

    int dx = xt - xf;
    int dz = zt - zf;
    switch (dx)
    {
    case -2:
      switch (dz)
      {
      case -1:
        result += GetFieldCost(OperMapIndex(xf - 1), OperMapIndex(zf - 1));
        result += GetFieldCost(OperMapIndex(xf - 1), zf);
        result *= H_SQRT5_4;
        break;
      case 0:
        result *= 0.5;
        result += GetFieldCost(OperMapIndex(xf - 1), zf);
        break;
      case 1:
        result += GetFieldCost(OperMapIndex(xf - 1), zf);
        result += GetFieldCost(OperMapIndex(xf - 1), OperMapIndex(zf + 1));
        result *= H_SQRT5_4;
        break;
      case -2:
      case 2:
        Fail("Unused");
        break;
      }
      break;
    case -1:
      switch (dz)
      {
      case -2:
        result += GetFieldCost(OperMapIndex(xf - 1), OperMapIndex(zf - 1));
        result += GetFieldCost(xf, OperMapIndex(zf - 1));
        result *= H_SQRT5_4;
        break;
      case -1:
        result *= H_SQRT2_2;
        break;
      case 0:
        result *= 0.5;
        break;
      case 1:
        result *= H_SQRT2_2;
        break;
      case 2:
        result += GetFieldCost(OperMapIndex(xf - 1), OperMapIndex(zf + 1));
        result += GetFieldCost(xf, OperMapIndex(zf + 1));
        result *= H_SQRT5_4;
        break;
      }
      break;
    case 0:
      switch (dz)
      {
      case -2:
        result *= 0.5;
        result += GetFieldCost(xf, OperMapIndex(zf - 1));
        break;
      case -1:
        result *= 0.5;
        break;
      case 1:
        result *= 0.5;
        break;
      case 2:
        result *= 0.5;
        result += GetFieldCost(xf, OperMapIndex(zf + 1));
        break;
      case 0:
        Fail("Unused");
        break;
      }
      break;
    case 1:
      switch (dz)
      {
      case -2:
        result += GetFieldCost(xf, OperMapIndex(zf - 1));
        result += GetFieldCost(OperMapIndex(xf + 1), OperMapIndex(zf - 1));
        result *= H_SQRT5_4;
        break;
      case -1:
        result *= H_SQRT2_2;
        break;
      case 0:
        result *= 0.5;
        break;
      case 1:
        result *= H_SQRT2_2;
        break;
      case 2:
        result += GetFieldCost(xf, OperMapIndex(zf + 1));
        result += GetFieldCost(OperMapIndex(xf + 1), OperMapIndex(zf + 1));
        result *= H_SQRT5_4;
        break;
      }
      break;
    case 2:
      switch (dz)
      {
      case -1:
        result += GetFieldCost(OperMapIndex(xf + 1), OperMapIndex(zf - 1));
        result += GetFieldCost(OperMapIndex(xf + 1), zf);
        result *= H_SQRT5_4;
        break;
      case 0:
        result *= 0.5;
        result += GetFieldCost(OperMapIndex(xf + 1), zf);
        break;
      case 1:
        result += GetFieldCost(OperMapIndex(xf + 1), zf);
        result += GetFieldCost(OperMapIndex(xf + 1), OperMapIndex(zf + 1));
        result *= H_SQRT5_4;
        break;
      case -2:
      case 2:
        Fail("Unused");
        break;
      }
      break;
    }

    // include gradient
    // TODO: calculation can be optimized

    // calculate the real coordinates of the path segment
    Vector3 pos1;
    pos1[0] = (xf + 0.5f) * OperItemGrid;
    pos1[2] = (zf + 0.5f) * OperItemGrid;
    pos1[1] = GLandscape->SurfaceYAboveWaterNoWaves(pos1[0], pos1[2]);
    Vector3 pos2;
    pos2[0] = (xt + 0.5f) * OperItemGrid;
    pos2[2] = (zt + 0.5f) * OperItemGrid;
    pos2[1] = GLandscape->SurfaceYAboveWaterNoWaves(pos2[0], pos2[2]);
    // distance and gradient
    //float distXZ2 = pos1.DistanceXZ2(pos2);
    //float invDistXZ = distXZ2 > 0 ? InvSqrt(distXZ2) : 0;
    //float distY = pos2.Y() - pos1.Y();
    //float gradient = distY * invDistXZ; // distY / distXZ

    //result *= vehType->GetGradientPenalty(gradient);
  }

  if (field1.type != NTHouse)
  {
    // direction penalty
    int dirD = AI::DirectionAngleDifference(field2.dir - field1.dir);

    // we want to pretend the direction change may be smooth
    // we need to call even with dirD = 0, as debet may be necessary to pay
    int debet = field1.debet;
    Assert(field2.debet == 0);
    if (dirD!=0 || debet!=0)
    {
      // normalize the angle by the distance
      //result += (1.0f / coef) * _vehicle->GetType()->GetCostTurn(dirD,debet,false);
      float turnCost = vehType->GetCostTurn(dirD, debet, false);
      if (field1.type == NTRoad && field2.type == NTRoad)
      {
        // avoid using of terrain on crossing - prefer movement on the road even when turning quite a lot 
        turnCost *= 0.25;
      }
      result.cost += turnCost;
      // stored debet should never be too high
      Assert(abs(debet)<127);
      field2.debet = debet;
    }
  }

  field2._t = field1._t + result.time;
  return result.cost;
}

FieldCost ASOCostFunction::GetFieldCost(OperMapIndex x, OperMapIndex z) const
{
  TaskPtr<EntityAIType> vehType = _map->_vehType;
  if (!vehType) return FieldCost(SET_UNACCESSIBLE,SET_UNACCESSIBLE);
  
  return _map->GetFieldCost(x, z, _locks, vehType, _map->_mode, _map->_isSoldier, true, true);
}

class ASOReadyFunction
{
protected:
  OperMap *_map;

public:
  ASOReadyFunction(OperMap *map) : _map(map) {}
  bool operator () (const ASOField &field) const;
};

bool ASOReadyFunction::operator () (const ASOField &field) const
{
  if (field.type != NTGrid) return true;

#if USE_OPERPATH_THREAD
  // for multicore, solve as a part of failure
  if (_map->IsJobRunning()) return true;
#endif

  OperMapIndex xf(field.GetX());
  OperMapIndex zf(field.GetZ());

  // check if the field and +-2 neighbors are ready
  OperMapIndex xMin(xf - 2), xMax(xf + 2);
  OperMapIndex zMin(zf - 2), zMax(zf + 2);
  // convert to LandGrid 
  LAND_INDICES(xMin, zMin, xLMin, zLMin);
  // note: rasterizing field may require 1 more neighbor
  xLMin--; zLMin--;
  LAND_INDICES(xMax, zMax, xLMax, zLMax);
  // note: rasterizing field may require 1 more neighbor
  xLMax++; zLMax++;
  return OperMap::OperFieldRangeReady(xLMin, xLMax, zLMin, zLMax);
}

class ASOHeuristicFunction
{
protected:
  OperMap *_map;
  float _coef;

public:
  ASOHeuristicFunction(OperMap *map, float coef) {_map = map; _coef = coef;}
  float operator () (const ASOField &field1, const ASOField &field2) const
  {
    if (field1 == field2) return 0;

    TaskPtr<EntityAIType> vehType = _map->_vehType;
    if (!vehType) return 0;

    OperMapIndex xs, zs, xe, ze;
    field1.GetXZ(*_map, xs, zs);
    field2.GetXZ(*_map, xe, ze);

    int dx = abs(xs - xe);
    int dz = abs(zs - ze);
/*
    float minD = (dx + dz - fabs(dx - dz)) * 0.5;
    float maxD = (dx + dz + fabs(dx - dz)) * 0.5;
    return _coef * ((maxD - minD) + H_SQRT2 * minD);
*/
    // optimization
    float dif = fabs(dx - dz);
    float result = _coef * (dif + (0.5f * H_SQRT2) * (dx + dz - dif));

    // direction penalty
    if (field1.type != NTHouse)
    {
      int dirD = AI::CalcDirection(Vector3(xe - xs, 0, ze - zs)) - field1.dir;
      dirD = AI::DirectionAngleDifference(dirD);
      if (dirD != 0)
      {
        int debet = 0;
        result += vehType->GetCostTurn(1,debet,true) * abs(dirD);
      }
    }

    return result;
  }
};

class ASOInterruptFunction
{
protected:
  OperMap *_map;

  int _iter;
  int _maxIters;

public:
  ASOInterruptFunction(OperMap *map, int maxIters) {_map = map; _maxIters = maxIters;}
  void Start() {_iter = 0;}
  void Iteration() {_iter++;}
  bool operator () () 
  {
#if USE_OPERPATH_THREAD
    if (_map->WantToBeSuspended() || _map->WantToBeCanceled()) return true;
#endif
    return _iter >= _maxIters;
  }
};

/// iterate through all item neighbor
class ASOIterator
{
protected:
  //@{
  /// iteration status
  ASONodeType _type; // what connection are we checking - NTRoad, NTHouse, NTGrid + NTError used when finished
  int _indexFrom1; // semantics differs by the _type and _field.type - described inside operator ++ ()
  int _indexFrom2; // DTTO
  int _indexTo1; // DTTO
  int _indexTo2; // DTTO
  //@}

  /// field searching from
  ASOField _field;
  /// other iterator parameters
  ASOContext _context;

public:
  ASOIterator();
  ASOIterator(const ASOField &field, void *context);
  operator bool () const {return _type != NTError;}
  void operator ++ ();
  operator ASOField ();
};

ASOIterator::ASOIterator()
: _field(GridField, OperMapIndex(-1), OperMapIndex(-1), 0)
{
  _type = NTRoad;
  _indexFrom1 = 0;
  _indexFrom2 = -1;
  _indexTo1 = 0;
  _indexTo2 = -1;
}

ASOIterator::ASOIterator(const ASOField &field, void *context)
: _field(field)
{
  // set iterator prior any node
  _type = NTRoad;
  _indexFrom1 = 0;
  _indexFrom2 = -1;
  _indexTo1 = 0;
  _indexTo2 = -1;
  // searching context
  _context = *reinterpret_cast<ASOContext *>(context);

#if DIAG_ROAD_ITERATOR
  if (_field.type == NTRoad)
    LogF("Road %d:%d", _field.GetRoadIndex(), _field.GetNodeIndex());
  else if (_field.type == NTBridge)
    LogF("Bridge %d:%d", _field.GetRoadIndex(), _field.GetNodeIndex());
#endif

  // find the first valid node
  operator ++();
}

void ASOIterator::operator ++ ()
{
  switch (_field.type.GetEnumValue())
  {
  case NTHouse:
    // inside house, iterate only house paths and exits
    switch (_type)
    {
    case NTRoad:
      // cannot go from house directly to the road
      _type = NTHouse;
      _indexFrom1 = 0;
      _indexFrom2 = -1;
      // continue immediately
    case NTHouse:
      // movement inside house
      if (_context._isSoldier && _context._map)
      {
        // semantics:
        //  _indexFrom1 ... phase of searching - 0 ... connections to other buildings, 1 ... connections inside building
        //  _indexFrom2 ... index of connection
        //  _indexTo1 ... house we are moving to
        //  _indexTo2 ... position (of house _indexTo1) we are moving to

        // the node we are moving from
        int houseFrom = _field.GetHouseIndex();
        const OperHouse &house = _context._map->_houses[houseFrom];
        int posFrom = _field.GetPosIndex();

        // 1. connections to other houses
        if (_indexFrom1 == 0) // phase 1
        {
          while (++_indexFrom2 < house._connections.Size())
          {
            const OperHouseConnection &conn = house._connections[_indexFrom2];
            if (conn._posFrom == posFrom)
            {
              // set the target to speed up operator ASOField ()
              _indexTo1 = conn._houseTo;
              _indexTo2 = conn._posTo;
              return;
            }
          }
          // continue to the next phase
          _indexFrom1 = 1;
          _indexFrom2 = -1;
        }

        // 2. connections inside house
        Assert(_indexFrom1 == 1); // phase 2
        const IPaths *paths = house._house ? house._house->GetIPaths() : NULL;
        if (paths)
        {
          // the connections from the current node
          const FindArray<int> &connections = paths->GetConnections(posFrom);
          if (++_indexFrom2 < connections.Size()) // "while (condition) {return;}" is equal to "if (condition) {return;}"
          {
            // set the target to speed up operator ASOField ()
            _indexTo1 = houseFrom; // houseTo == houseFrom
            _indexTo2 = connections[_indexFrom2];
            return;
          }
        }

        // 3. check if the current node is exit
        bool exit = false;
        for (int i=0; i<house._exits.Size(); i++)
        {
          if (house._exits[i]._pos == _field.GetPosIndex())
          {
            exit = true; break;
          }
        }
        if (exit)
        {
          // if so, go to the grid field the exit is in
          _type = NTGrid;
          _indexTo1 = -1;
        }
        else
        {
          // if not, we are done
          _type = NTError;
        }
        return;
      }
      else
      {
        // do not continue - the current context is invalid
        Fail("Unexpected context inside house");
        _type = NTError;
        return; // done
      }
    case NTGrid:
      // no more grid fields, finish
      _type = NTError;
      return; // done
    default:
      Fail("Unexpected itarator state");
      _type = NTError;
      return;
    }
  case NTGrid:
    switch (_type)
    {
    case NTRoad:
      // try to enter the road net
      if (_context._useRoads && _context._map)
      {
        // semantics:
        //  _indexTo1 ... road we are moving to
        //  _indexTo2 ... node of road we are moving to

        for (; _indexTo1<_context._map->_roads.Size(); _indexTo1++)
        {
          const OperRoad &road = _context._map->_roads[_indexTo1];
          if (road._bridge) continue; // cannot enter from landscape to the bridge
          while (++_indexTo2 < road._nodes.Size())
          {
            const OperRoadNode &node = road._nodes[_indexTo2];
            if (node._x == _field.GetX() && node._z == _field.GetZ())
              return; // found road node in the grid we are in
          }
          _indexTo2 = -1;
        }
      }
      
      // continue with the houses
      _type = NTHouse;
      _indexTo1 = 0;
      _indexFrom1 = 0;
      _indexFrom2 = -1;
    case NTHouse:
      // try to enter the house with doors
      if (_context._isSoldier && _context._map)
      {
        // semantics:
        //  _indexFrom1 ... index of exit (of house _indexTo1)
        //  _indexFrom2 ... index of connection
        //  _indexTo1 ... house we are moving to
        //  _indexTo2 ... position (of house _indexTo1) we are moving to

        // check if some exit is present
        if (_context._map->IsHouseExit(_field.GetX(), _field.GetZ()))
        {
          // for each valid house
          while (_indexTo1 < _context._map->_houses.Size())
          {
            const OperHouse &house = _context._map->_houses[_indexTo1];
            const IPaths *paths = house._house ? house._house->GetIPaths() : NULL;
            if (paths)
            {
              // for each exit on the correct position
              while (_indexFrom1 < house._exits.Size())
              {
                const OperHouseExit &pos = house._exits[_indexFrom1];
                if (pos._x == _field.GetX() && pos._z == _field.GetZ())
                {
                  // the connections from this exit
                  const FindArray<int> &connections = paths->GetConnections(pos._pos);
                  if (++_indexFrom2 < connections.Size()) // "while (condition) {return;}" is equal to "if (condition) {return;}"
                  {
                    _indexTo2 = connections[_indexFrom2];
                    return;
                  }
                }
                // next exitIndex
                _indexFrom1++;
                _indexFrom2 = -1;
              }
            }
            // next houseIndex
            _indexTo1++;
            _indexFrom1 = 0;
          }
        }
      }

      // continue through landscape
      _type = NTGrid;
      _indexTo1 = -1;
    case NTGrid:
      _indexTo1++;
      // 8 directions
      if (_indexTo1 >= 8) _type = NTError;
      return; // done
    default:
      Fail("Unexpected iterator state");
      _type = NTError;
      return;
    }
  case NTRoad:
  case NTBridge:
    switch (_type)
    {
    case NTRoad:
      {
        const OperRoad &road = _context._map->_roads[_field.GetRoadIndex()];

        // move using the road net
        if (_context._useRoads && _context._map)
        {
          // semantics:
          //  _indexFrom2 ... index of connection we are testing
          //  _indexTo1 ... road we are moving to
          //  _indexTo2 ... node of road we are moving to

          RoadNode node = road._road->GetNode(_field.GetNodeIndex());

#if DIAG_ROAD_ITERATOR
          LogF(" - %d connections", node.NConnections(_context._isSoldier));
#endif

          // go through connections
          while (++_indexFrom2 < node.NConnections(_context._isSoldier))
          {
            RoadNode con = node.GetConnection(_context._isSoldier, _indexFrom2);
            if (con.IsValid())
            {
              _indexTo1 = _context._map->_roads.FindKey(con.GetRoad());
              if (_indexTo1 >= 0)
              {
                _indexTo2 = con.GetNodeIndex();
#if DIAG_ROAD_ITERATOR
                LogF(" - connection %d to %d:%d", _indexFrom2, _indexTo1, _indexTo2);
#endif
                return;
              }
              Fail("Road not found");
            }
#if DIAG_ROAD_ITERATOR
            else
              LogF(" - connection %d invalid", _indexFrom2);
#endif
          }
        }

        if (road._bridge)
        {
          // cannot exit the road net
          _type = NTError;
        }
        else
        {
          // houses and roads are not directly connected
          // exit the road net
          _type = NTGrid;
          _indexTo1 = -1;
        }
        return;
      }
    case NTGrid:
      // no more grid fields, finish
      _type = NTError;
      return; // done
    default:
      Fail("Unexpected itarator state");
      _type = NTError;
      return;
    }
  default:
    Fail("Unexpected field type");
    // continue;
  case NTError:
    _type = NTError; // do not expand
    return; // done
  }
}

ASOIterator::operator ASOField ()
{
  switch (_type)
  {
  case NTRoad:
    {
      // semantics:
      //  _indexTo1 ... road we are moving to
      //  _indexTo2 ... node of road we are moving to
      const OperRoad &road = _context._map->_roads[_indexTo1];
      const OperRoadNode &node = road._nodes[_indexTo2];
      Vector3 posTo = node._pos;

      bool bridge = road._road->IsBridge();
      // calculate direction based on real fields positions
      Vector3 posFrom;
      int dir;
      if (_field.type == NTBridge || _field.type == NTRoad)
      {
        const OperRoad &road = _context._map->_roads[_field.GetRoadIndex()];
        const OperRoadNode &node = road._nodes[_field.GetNodeIndex()];
        posFrom = node._pos;
        dir = AI::CalcDirection(posTo - posFrom);
      }
      else
      {
        Assert(_field.type == NTGrid); // no direct connection from house to road
        posFrom[0] = (_field.GetX() + 0.5f) * OperItemGrid;
        posFrom[2] = (_field.GetZ() + 0.5f) * OperItemGrid;
        posFrom[1] = posTo[1]; // we want to calculate direction only
        dir = _field.dir;
      }

      return ASOField(RoadField, bridge ? NTBridge : NTRoad, _indexTo1, _indexTo2, dir);
    }
  case NTHouse:
    {
      // semantics:
      //  _indexTo1 ... house we are moving to
      //  _indexTo2 ... position (of house _indexTo1) we are moving to
      return ASOField(HouseField, _indexTo1, _indexTo2);
    }
  case NTGrid:
    Assert(_context._map);
    if (_field.type == NTHouse)
    {
      // leaving house
      DoAssert(_indexTo1 == -1);
      const IPaths *paths = _context._map->GetHouse(_field.GetHouseIndex());
      if (!paths) return ASOField(ErrorField);
      Vector3 pos = paths->GetPosition(_field.GetPosIndex());
      OperMapIndex newX(toIntFloor(pos.X() * InvOperItemGrid));
      OperMapIndex newZ(toIntFloor(pos.Z() * InvOperItemGrid));
      return ASOField(GridField, newX, newZ, 0);
    }
    else if (_field.type == NTRoad || _field.type == NTBridge)
    {
      // leaving road net
      const OperRoad &road = _context._map->_roads[_field.GetRoadIndex()];
      const OperRoadNode &node = road._nodes[_field.GetNodeIndex()];
      // keep the original direction - we have no choice to leave the road differently
      return ASOField(GridField, node._x, node._z, _field.dir);
    }
    else
    {
      OperMapIndex newX(_field.GetX() + direction_delta[2 * _indexTo1][0]);
      OperMapIndex newZ(_field.GetZ() + direction_delta[2 * _indexTo1][1]);
      return ASOField(GridField, newX, newZ, direction_delta[2 * _indexTo1][2]);
    }
  default:
    Fail("Unexpected field type");
    return ASOField(ErrorField);
  }
}

struct ASOOpenListTraits
{
  static bool IsLess(const ASONode *a, const ASONode *b) {return a->_f < b->_f;}
  static bool IsLessOrEqual(const ASONode *a, const ASONode *b){return a->_f <= b->_f;}
};
class ASOOpenList : public HeapArray<ASONode *, MemAllocDSafe, ASOOpenListTraits>
{
typedef HeapArray<ASONode *, MemAllocDSafe, ASOOpenListTraits> base;
public:
  void UpdateUp(ASONode *node) {base::HeapUpdateUp(node);}
  void Add(ASONode *node) {base::HeapInsert(node);}
  bool RemoveFirst(ASONode *&node) {return base::HeapRemoveFirst(node);}
  const ASONode *GetFirst() const {return Size() > 0 ? Get(0) : NULL;}
};

#ifdef MFC_NEW
# define ALLOC_NODES_IN_BLOCKS 0 // NewPage / DeletePage not usable as expected
#else
# define ALLOC_NODES_IN_BLOCKS 1
#endif

#define USE_CLOSED_HASHING 1

#if ALLOC_NODES_IN_BLOCKS

struct ASONodeRef
{
  ASONode *_node;

  ASONodeRef() : _node(NULL) {}
  ASONodeRef(ASONode *node) : _node(node) {}
  operator const ASONode *() const {return _node;}
  operator ASONode *() {return _node;}
  ASONode *operator ->() const {return _node;}
  ASOField::Key GetKey() const {return _node->_field.GetKey();}
};
TypeIsSimpleZeroed(ASONodeRef)

#else

struct ASONodeRefTraits
{
  static void Delete(ASONode *ptr)
  {
    #if USE_CLOSED_HASHING
      // zombie not allocated, we cannot free it
      if (ptr==(ASONode *)1) return;
    #endif
#if USE_OPERPATH_THREAD
    // remove from the safe heap
    ptr->~ASONode();
    mtDelete(ptr);
#else
    delete ptr;
#endif
  }
};

class ASONodeRef : public SRef<ASONode, ASONodeRefTraits>
{
  typedef SRef<ASONode, ASONodeRefTraits> base;

public:
  ASONodeRef() {}
  ASONodeRef(ASONode *node) : base(node) {}
  ASOField::Key GetKey() const {return (*this)->_field.GetKey();}
};
TypeIsMovableZeroed(ASONodeRef)

#endif

#if USE_CLOSED_HASHING
# include <Es/Containers/maps.hpp>
  ASOField::Key GetASONodeRefKey(const ASONodeRef &nodeRef)
  {
    return nodeRef.GetKey();
  }

  typedef ImplicitMap<ASOField::Key, ASONodeRef, GetASONodeRefKey, false, MemAllocDSafe> ASOCLMap;

  template <> ASONodeRef ImplicitMapTraits<ASONodeRef>::zombie = (ASONode *)1;
  template <> ASONodeRef ImplicitMapTraits<ASONodeRef>::null = (ASONode *)NULL;

#else
  struct ASOClosedListTraits: public DefMapClassTraits<ASONodeRef>
  {
    //! key type
    typedef int KeyType;
    //! calculate hash value
    static unsigned int CalculateHashValue(KeyType key)
    {
      return (unsigned int)key;
    }

    //! compare keys, return negative when k1<k2, positive when k1>k2, zero when equal
    static int CmpKey(KeyType k1, KeyType k2)
    {
      return k1 - k2;
    }
    static KeyType GetKey(const ASONodeRef &item) {return item.GetKey();}
  };

  typedef MapStringToClass<ASONodeRef, AutoArray<ASONodeRef, MemAllocDSafe>, ASOClosedListTraits, MemAllocDSafe> ASOCLMap;
#endif

#include <Es/Memory/normalNew.hpp>
class ASOClosedList : public ASOCLMap
{
typedef ASOCLMap base;

#if ALLOC_NODES_IN_BLOCKS
  // Own storage for ASONode structures:
  // - allocate memory from the main heap in blocks
  // - free allocated memory at once in the destructor
  // - does not support freeing of single items
  // - allocation need not to be thread safe (except allocation from the main heap) - only one tread at a time is accessing it

  /// blocks of memory allocated from the main heap
  AutoArray<void *, MemAllocDSafe> _blocks;
  /// size of the single item
  static const int _itemSize = sizeof(ASONode);
  /// number of occupied items in the last block
  int _itemsInLastBlock;

public:
  ASOClosedList()
  {
    // force the first allocation create a block
    _itemsInLastBlock = INT_MAX / _itemSize - 1;
  }
  ~ASOClosedList()
  {
    int blocks = _blocks.Size();
    if (blocks > 0)
    {
      int blockSize = GetPageRecommendedSize();
      int maxItems = blockSize / _itemSize;
      // call destructor of all items
      for (int i=0; i<blocks-1; i++)
      {
        // full blocks
        ASONode *ptr = (ASONode *)_blocks[i];
        for (int j=0; j<maxItems; j++)
        {
          ptr->~ASONode();
          ptr++;
        }
      }
      {
        // the current block
        ASONode *ptr = (ASONode *)_blocks[blocks - 1];
        for (int j=0; j<_itemsInLastBlock; j++)
        {
          ptr->~ASONode();
          ptr++;
        }
      }
      // free the blocks
      for (int i=0; i<blocks; i++)
        GMTMemFunctions->DeletePage(_blocks[i], blockSize);
    }
  }
#endif

public:
  ASONode *AddNew(const ASOField &field, ASONode *parent, float g, float f)
  {
#if ALLOC_NODES_IN_BLOCKS
    int blockSize = GetPageRecommendedSize();
    Assert(blockSize > _itemSize);
    // check if item fits in the current block
    if ((_itemsInLastBlock + 1) * _itemSize > blockSize)
    {
      // allocate the new block
      _blocks.Add(GMTMemFunctions->NewPage(blockSize, blockSize));
      _itemsInLastBlock = 0;
    }
    // find the value in the current block
    void *currentBlock = _blocks[_blocks.Size() - 1];
    ASONode *node = ((ASONode *)currentBlock + _itemsInLastBlock);
    ::new(node) ASONode(field, parent, g, f); // explicit call of constructor
    // update the number of allocated items
    _itemsInLastBlock++;
#elif USE_OPERPATH_THREAD
    // allocate on the safe heap
    ASONode *node = ::new(mtNew(sizeof(ASONode))) ASONode(field, parent, g, f);
    DoAssert(node);
#else
    ASONode *node = new ASONode(field, parent, g, f);
#endif
#if USE_CLOSED_HASHING
    base::put(node);
#else
    base::Add(node);
#endif
    return node;
  }

#if USE_CLOSED_HASHING
  // MapStringToClass-like access to ImplicitMap
  int NItems() const {return base::card();}
  ASONodeRef operator [] (const ASOField::Key &key) const
  {
    ASONodeRef result;
    if (unconst_cast(this)->base::get(key, result)) return result;
    return NULL;
  }

  class Iterator
  {
  protected:
    IteratorState _iterator;
    ASONodeRef _nodeRef;
    const ASOCLMap &_map;

  public:
    Iterator(const ASOCLMap &map) : _map(map)
    {
      _map.getFirst(_iterator, _nodeRef);
    }
    operator bool () const {return _nodeRef != NULL;}
    void operator ++ ()
    {
      _map.getNext(_iterator, _nodeRef);
    }
    const ASONodeRef &operator * ()
    {
      return _nodeRef;
    }
  };
#endif
};
#include <Es/Memory/debugNew.hpp>

class ASOParams
{
public:
  static float LimitCost() {return GET_UNACCESSIBLE;}
  static float BestNodeCoef() {return 0.5;}
};


#define COVER_REJECT_ENUM(type,prefix,XX) \
	XX(type, prefix, Bad) \
	XX(type, prefix, Good) \
	XX(type, prefix, Slow) \
	XX(type, prefix, SlowP1) \
	XX(type, prefix, Busy) \
	XX(type, prefix, Out) \
	XX(type, prefix, NotF) \
	XX(type, prefix, Far) \

#ifndef DECL_ENUM_COVER_REJECT_STATUS
#define DECL_ENUM_COVER_REJECT_STATUS
DECL_ENUM(CoverRejectStatus)
#endif

DECLARE_DEFINE_ENUM(CoverRejectStatus,CR,COVER_REJECT_ENUM)

/// diagnostics information about a particular cover
struct CoverDiagInfo
{
  CoverRejectStatus status;
  float extValue;
  float extValue2;
  const AStarNode<ASOField> *node;
  
  CoverDiagInfo(CoverRejectStatus s, float v=0, const AStarNode<ASOField> *n=NULL){status=s;extValue=v;node=n;extValue2=0;}
};

TypeIsSimple(CoverDiagInfo)

/// interface for operative searching
class IAStarOperative
{
  public:

  virtual bool IsDone() const = 0;
  virtual bool IsFound() const = 0;
  virtual const AStarNode<ASOField> *GetLastNode() const = 0;
  virtual const AStarNode<ASOField> *GetBestNode(const void *context) const = 0;
  virtual int GetNodeCount() const = 0;

  virtual int Process(void *context=NULL, bool diags=false) = 0;

  /// handle search abortion (too much iterations)
  virtual bool AbortSearch(OperMap *map) {return true;}
  
  //! Access to closed list used for diagnostics
  virtual const ASOClosedList &GetClosedList() const = 0;
  //! Access to open list used for diagnostics
  virtual const ASOOpenList &GetOpenList() const = 0;
  //! Access to destination for consistency checks
  virtual const ASOField &GetDestination() const = 0;
  //! Access to destination for diagnostics
  virtual const ASOField &GetStart() const = 0;

  /// access to the best cover found
  virtual const AStarNode<ASOField> *GetBestCover(const OperMap &map, CoverInfo &cover, AIBrain *unit, AutoArray<CoverDiagInfo> *diagInfo=NULL) const {return NULL;}

  virtual RString GetDiagText(bool cover) const = 0;
  
  //@{ access to the cover candidates found
  virtual const OperCover &GetCoverFound(int i) const
  {
    // return bad value - should not be called at all, because of GetCoverFoundCount returning 0
    return *(OperCover *)NULL;
  }
  virtual int GetCoverFoundCount() const {return 0;}
  /// used for debugging cover search
  virtual IAStarOperative *Get2ndPass() const {return NULL;}
  //@}

  virtual ~IAStarOperative() {}

  virtual bool IsTargetReached(const ASONode *node, const void *context) const = 0;
};

struct ASOCostFunctions: public AStarCostFunctionsDef<ASOField>
{
  typedef ASOCostFunction CostFunction;
  typedef ASOHeuristicFunction HeuristicFunction;
};

static float NearHeurFactor = 0.5f;

struct ASOCoverCostFunctions: public AStarCostFunctionsDef<ASOField>
{
  typedef ASOCostFunction CostFunction;
  typedef ASOHeuristicFunction HeuristicFunction;

  /// how should be heuristics combined with cost to create a total cost
  struct AddHeuristics
  {
    OperMap *_map;
    /// we want to perform a search similar to breadth first near the starting point
    float _breadthUntilCost;
    float _distToDest;
    int _targetX;
    int _targetZ;
    
    AddHeuristics(OperMap *map, float breadthUntilCost, float distToDest, int xt, int zt)
    :_map(map),_breadthUntilCost(breadthUntilCost),_distToDest(distToDest),_targetX(xt),_targetZ(zt){}
    
    /// by default add them
    float operator () (const ASOField &curr, const ASOField &next, float g, float h) const
    {
      if (g<_breadthUntilCost)
      {
        // exploring the area around the starting point
        // we want the heuristic to be limited, but still partially used still
        // this does not have any sense for fields which would be rejected anyway
        OperMapIndex x, z;
        curr.GetXZ(*_map, x, z);

        float dist2 = Square(x-_targetX)+Square(z-_targetZ);
        if (dist2<Square(_distToDest))
        {
          return g+h*NearHeurFactor;
        }
        
      }
      return g+h;
    }
  };
};

typedef AStarCostFunctionsWrapped<ASOCostFunctions> ASOCostFunctionsWrapped;
typedef AStarCostFunctionsWrapped<ASOCoverCostFunctions> ASOCoverCostFunctionsWrapped;

typedef AStar<
  ASOField,
  ASOCostFunctions, ASOReadyFunction, ASOInterruptFunction, AStarEndDefault<ASOField>,
  ASOIterator,
  ASOClosedList, ASOOpenList,
  ASOParams, IAStarOperative
> AStarOperativeBase;

/// A* used for operative path finding
class AStarOperative : public AStarOperativeBase
{
  typedef AStarOperativeBase base;

public:
  AStarOperative(
    const ASOField &start, const ASOField &destination,
    const ASOCostFunctionsWrapped &costFcs,
    const ASOReadyFunction &ready, const ASOInterruptFunction &interrupt,
    const AStarEndDefault<ASOField> &end
  ) : base(start, destination, costFcs, ready, interrupt,end) {}
};

/// candidate for a cover, as found by A*
struct OperCoverAStarCandidate: public OperCover
{
  /** as long as the corresponding OperMap is not cleared, the node is known to exist */
  AStarNode<ASOField> *_node; 
  
  OperCoverAStarCandidate(){}
  OperCoverAStarCandidate(const OperCover &cover, AStarNode<ASOField> *node):OperCover(cover),_node(node){}
};

TypeIsMovable(OperCoverAStarCandidate)


typedef AutoArray<OperCoverAStarCandidate, MemAllocDSafe> CandidatesList;


/// A* used for cover findind in operative map
/**
Shares a lot of common functionality with AStarOperative. Major difference is the termination condition.
*/
class AStarCoverContext: public IAStarOperative
{
protected:
  /// helper results data
  mutable CandidatesList _coverCandidates;

  /// node matching the destination
  mutable AStarNode<ASOField> * _destNode;
  
  AStarCoverContext():_destNode(NULL){}
};

static float CornerSuitable(float engageHeadingL, float engageHeadingR, float tgtHeading)
{
  float diffL = AngleDifference(engageHeadingL,tgtHeading);
  float diffR = AngleDifference(engageHeadingR,tgtHeading);
  // when target is in the engaging range, return full suitability
  if (diffL<0 && diffR>0) return 1;
  // when out of the range, return how much out
  return InterpolativC(floatMin(fabs(diffL),fabs(diffR)),0,H_PI*0.125f,1,0);
}


/// check how much is given cover suitable against known threat direction
/**
@return
 0..fullyCovered how much cover does the cover provide (0.5 = full cover, 0 = no cover)
 fullyCovered..1 what firing opportunity does the cover provide (1 = very good, 0 = none)

*/

float CoverSuitableAgainst(const OperCover &cover, Vector3Par hideLPos, Vector3Par hideRPos)
{
  #if DIAG_FALSE_COVER
    if (cover._valid!=OperCover::Valid) return 0;
  #endif
  const float fullyCovered = 0.8f;
  Vector3 hideL = hideLPos-cover._pos;
  Vector3 hideR = hideRPos-cover._pos;

  float inCoverHeading = cover._heading+H_PI;

  float headingL = atan2(hideL.X(),hideL.Z());
  float headingR = atan2(hideR.X(),hideR.Z());
  // create a "canonical" form, so that we can perform comparisons
  if (headingR<headingL) headingR += H_PI*2;
  
  float coverL,coverR;
  // this will be different for corner / edge / wall covers
  float engagePossible = 0;
  if (cover._type.GetEnumValue()<CoverEdgeMiddle)
  {
    // corner cover - covering against -direction +- pi/4
    // when cover is used properly, it can cover even more - we may step a bit behind the corner
    // full covering angle obtained this way can be something between +- pi/4 and +- pi/2
    coverL = inCoverHeading-H_PI*0.4f;
    coverR = inCoverHeading+H_PI*0.4f;
    // check how well does the cover allow us to engage leftmost or rightmost target
    // engaging into the cover is not possible
    float engageL = inCoverHeading-H_PI*0.25f;
    float engageR = inCoverHeading+H_PI*0.25f;
    
    engagePossible = floatMax(CornerSuitable(coverL,engageL,headingL),CornerSuitable(engageR,coverR,headingR));
  }
  else
  {
    if (cover._type.GetEnumValue()<CoverWall)
    {
      // engaging over edge is always possible, but not that safe as with corner covers
      engagePossible = 0.7f;
    }
    // edge / wall cover - covering against -direction +- pi/2
    // however, not full pi/2, as unless the wall is very long, we need to keep some distance from the wall
    coverL = inCoverHeading-H_PI*0.45f;
    coverR = inCoverHeading+H_PI*0.45f;
  }

  Vector3 diagPos = cover._pos + Matrix3(MRotationY,-cover._heading).Direction()*0.5f*(1+engagePossible);
  
  // with multicore we are unable to show diags from jobs - show them in singlecore only
  extern int GetCPUCount();
  if (CHECK_DIAG(DECostMap) && GetCPUCount()==1)
  {
    Vector3 coverLDir = Matrix3(MRotationY,-coverL).Direction();
    Vector3 coverRDir = Matrix3(MRotationY,-coverR).Direction();

    LODShape *forceArrow=GScene->ForceArrow();
    {
      Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);
      float size=0.02f;
      arrow->SetPosition(diagPos);
      arrow->SetDirectionAndUp(coverLDir,VUp);
      arrow->SetPosition(arrow->FutureVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size));
      arrow->SetScale(size);
      arrow->SetConstantColor(Color(0,0.5,0));
      GScene->DrawObject(arrow);
    }
    {
      Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);
      float size=0.02f;
      arrow->SetPosition(diagPos);
      arrow->SetDirectionAndUp(coverRDir,VUp);
      arrow->SetPosition(arrow->FutureVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size));
      arrow->SetScale(size);
      arrow->SetConstantColor(Color(0,0.5,0));
      GScene->DrawObject(arrow);
    }
    {
      Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);
      float size=0.02f;
      arrow->SetPosition(diagPos);
      arrow->SetDirectionAndUp(hideL,VUp);
      arrow->SetPosition(arrow->FutureVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size));
      arrow->SetScale(size);
      arrow->SetConstantColor(Color(0.5,0,0));
      GScene->DrawObject(arrow);
    }
    {
      Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);
      float size=0.02f;
      arrow->SetPosition(diagPos);
      arrow->SetDirectionAndUp(hideR,VUp);
      arrow->SetPosition(arrow->FutureVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size));
      arrow->SetScale(size);
      arrow->SetConstantColor(Color(0.5,0,0));
      GScene->DrawObject(arrow);
    }
    
  }
  // try to match cover heading with the enemy interval, check if there is any overlap
  // if not, we may need to adjust cover angle about one circle plus or minus
  // we are interested in the best result of the three
  float adjust = -H_PI*2;
  float maxOverlap = 0;
  // try adjust values -2pi, 0 ,+2pi
  for (int i=0; i<3; i++, adjust+=H_PI*2)
  {
    float coverLAdjusted = coverL+adjust;
    float coverRAdjusted = coverR+adjust;
    
    // if the whole interval headingL..headingR is covered, we are done
    // this is important for a special case of headingL = headingR
    if (coverLAdjusted<headingL && coverRAdjusted>headingR)
    {
      if (CHECK_DIAG(DECostMap) && GetCPUCount()==1)
      {
        GScene->DrawDiagText(Format("Overlap 1+, %.2f",engagePossible),diagPos,4);
      }
      // full cover - we want to consider if engaging is possible or not
      return fullyCovered+engagePossible*(1-fullyCovered);
    }
    // check how much the two intervals overlap
    float overlapL = floatMax(coverLAdjusted,headingL);
    float overlapR = floatMin(coverRAdjusted,headingR);
    float overlapSize = overlapR-overlapL;
    if (maxOverlap<overlapSize)
    {
      maxOverlap = overlapSize;
    }
  }
  if (maxOverlap<=0)
  {
    // TODO: when out of interval, check how much out we are
    if (CHECK_DIAG(DECostMap) && GetCPUCount()==1)
    {
      GScene->DrawDiagText(Format("Overlap 0-"),diagPos,4);
    }
    return 0;
  }
  // when overlap is big enough, report full success
  if (maxOverlap>=headingR-headingL)
  {
    LogF("Late full cover detection");
    return engagePossible;
  }
  // scale the result into 0..1 range
  float ret = maxOverlap/(headingR-headingL);
  if (CHECK_DIAG(DECostMap) && GetCPUCount()==1)
  {
    GScene->DrawDiagText(Format("Overlap %.2f",ret),diagPos,4);
  }
  return fullyCovered*ret;
}



/// check when a search for cover candidates may terminate
/**
Each node is submitted to AStarCoverEnd once.
The function gathers cover candidates and as soon as it thinks it has enough of them, it indicates search termination.

Another condition is when sufficiently enough distance around destination is covered

*/
struct AStarCoverEnd
{
  /// is this the 2nd pass?
  /**
  For 2nd pass some conditions are handled differently. The candidate list is shared from the 1st pass.
  */
  bool _is2ndPass;
  /// back-pointer to the parent
  OperMap *_map;
  /// list of found candidates
  CandidatesList &_candidates;
  /// search destination
  ASOField _destination;
  /// was search destination already reached?
  AStarNode<ASOField> *&_destNode;
  /// max. allowed distance (Euclidean, in x/z grid unit) of the cover to destination
  float _distToDest;
  
  //@{ expected enemy position
  Vector3 _hideFromL;
  Vector3 _hideFromR;
  //@}
  /// cost to terminate the search
  float _maxCost;
  float _maxDelay;
  //@{ coordinates of the target
  int _tgtX,_tgtZ;
  PlanPosition _tgtPos;
  //@}
  
  AStarCoverEnd(
    OperMap *map, CandidatesList &candidates, AStarNode<ASOField> *&destNode,
    const ASOField &destination, float distToDest, Vector3Par hideFromL, Vector3Par hideFromR, int tgtX, int tgtZ,
    const PlanPosition &tgtPos, float maxCost, float maxDelay, bool is2ndPass
  )
  :_map(map),_candidates(candidates),_destination(destination),
  _destNode(destNode),_distToDest(distToDest),_hideFromL(hideFromL),_hideFromR(hideFromR),
  _maxCost(maxCost),_maxDelay(maxDelay),
  _tgtX(tgtX),_tgtZ(tgtZ),_tgtPos(tgtPos),_is2ndPass(is2ndPass)
  {
  }
  
  bool operator () (AStarNode<ASOField> *best);
};

bool AStarCoverEnd::operator () (AStarNode<ASOField> *best)
{
  // ignore cover which is too costly to be accessed
  float detour = _destNode ? best->_f-_destNode->_g : 0;
  if (detour<_maxDelay || _tgtPos._cover)
  {
    OperMapIndex x(-1), z(-1);
    best->_field.GetXZ(*_map, x, z);
    float operItemGrid = OperItemGrid;
    // check if the field is bringing us towards the target - if not, ignore it
    float distBest2 = Square(x - _tgtX) + Square(z - _tgtZ);
    if (distBest2 <= Square(_distToDest))
    {
      // check for cover candidates
      int nCover=0;
      const OperCover *cover = _map->GetCover(x, z, nCover);
      for (int i=0; i<nCover; i++)
      {
        const OperCover &ci = cover[i];
        // check if cover is able to protect us against the target - if not, ignore it
        // check if cover provides any cover in the direction we need
        #if DIAG_FALSE_COVER
          if (ci._valid!=OperCover::Valid)
            continue;
        #endif
        if (!_tgtPos._cover)
        {
        float suitable = CoverSuitableAgainst(ci,_hideFromL,_hideFromR);
        if (suitable<=0) continue;
        }

        // verify distance again - this time for the cover itself
        float distCover2 = Square(ci._pos.X()-_tgtX*operItemGrid)+Square(ci._pos.Z()-_tgtZ*operItemGrid);
        if (distCover2<=Square(_distToDest*operItemGrid))
        {
          // cover suitable, we can add it
          // verify real cover position
          if (_tgtPos._cover)
          {
            Matrix4 coverPos;
            coverPos.SetPosition(ci._pos);
            coverPos.SetOrientation(Matrix3(MRotationY,H_PI-ci._heading));
            Vector3 coverTgtPos = coverPos.FastTransform(Vector3(0,0,-1));
            if (coverTgtPos.DistanceXZ2(_tgtPos)<Square(0.1f))
            {
              _candidates.Add(OperCoverAStarCandidate(ci,best));
              _destNode = best;
              return true;
        }
      }
          _candidates.Add(OperCoverAStarCandidate(ci,best));
    }
  }
  }
  }
  if (best->_field==_destination) _destNode = best;
  // when search in the 2nd pass, we cannot stop based on candidates count,
  // as this would leave to many candidates unverified
  // on the other hand, 2nd pass search should be limited by _maxCost quite soon
  if (_tgtPos._cover)
  {
    // search until the cover desired is really found
    return _destNode && _candidates.Size()>4;
  }
  return _destNode && (!_is2ndPass && _candidates.Size()>4 || best->_g>=_maxCost);
}

struct AStarCoverRefTraits;

class AStarCover: public AStar<
  ASOField,
  ASOCoverCostFunctions, ASOReadyFunction, ASOInterruptFunction, AStarCoverEnd,
  ASOIterator, ASOClosedList, ASOOpenList,
  ASOParams, AStarCoverContext
>
{
  typedef AStar<
    ASOField,
    ASOCoverCostFunctions, ASOReadyFunction, ASOInterruptFunction, AStarCoverEnd,
    ASOIterator, ASOClosedList, ASOOpenList,
    ASOParams, AStarCoverContext
  > base;

  /// we need to know when 2nd pass is done or skipped, so that we can return if we are done
  bool _2ndPassDone; 
  /// we need to remember start so that we can reverse the search
  ASOField _start;
  /// 2nd pass is used for cover verification
  SRef<AStarCover, AStarCoverRefTraits> _2ndPass;
  /// what cost from the start was selected as a starting point for the 2nd pass
  float _2ndPassCost;
  
public:
  /// constructor used for the 1st pass
  AStarCover(
    const ASOField &start, const ASOField &destination,
    const ASOCoverCostFunctionsWrapped &costFcs, 
    const ASOReadyFunction &ready, const ASOInterruptFunction &interrupt,
    OperMap *map, float distToDest, Vector3Par hideFromL, Vector3Par hideFromR, int tgtX, int tgtZ, const PlanPosition &tgtPos,
    float maxCost, float maxDelay
  ) : base(
    start, destination, costFcs, ready, interrupt,
    AStarCoverEnd(
      map,AStarCoverContext::_coverCandidates,AStarCoverContext::_destNode,
      destination,distToDest,hideFromL,hideFromR,tgtX,tgtZ,tgtPos,maxCost,maxDelay,false
    )
  ),_2ndPassDone(false),_start(start),_2ndPassCost(0) {}

  /// constructor used for the 2nd pass
  AStarCover(
    const ASOField &start, const ASOField &destination,
    const ASOCoverCostFunctionsWrapped &costFcs, 
    const ASOReadyFunction &ready, const ASOInterruptFunction &interrupt,
    OperMap *map, const AStarCover &pass1
  ) : base(
    start, destination, costFcs, ready, interrupt,
    AStarCoverEnd(
      map,AStarCoverContext::_coverCandidates,AStarCoverContext::_destNode,
      destination,pass1._endFunction._distToDest,
      pass1._endFunction._hideFromL,pass1._endFunction._hideFromR,
      pass1._endFunction._tgtX,pass1._endFunction._tgtZ,pass1._endFunction._tgtPos,pass1.ComputeMaxF(),pass1._endFunction._maxDelay,true
    )
  ),_2ndPassDone(false),_start(start),_2ndPassCost(0) {}

  //@{ implement IAStarOperative
  virtual const AStarNode<ASOField> *GetBestCover(const OperMap &map, CoverInfo &cover, AIBrain *unit, AutoArray<CoverDiagInfo> *diagInfo=NULL) const;
  virtual const AStarNode<ASOField> *GetLastNode() const
  {
    // we need to override default implementation, because sometimes we need to return other node than the last one
    return AStarCoverContext::_destNode;
  }
  virtual bool IsDone() const {return base::IsDone() && _2ndPassDone;}
  virtual bool IsFound() const {return AStarCoverContext::_destNode != NULL;}

  virtual bool AbortSearch(OperMap *map);
  virtual int Process(void *context=NULL, bool diags=false);

  virtual RString GetDiagText(bool cover) const;

  float ComputeMaxF() const
  {
    // 20 % of the shortest path cost is there to handle better paths going though high cost only
    return _2ndPassCost*1.2f + _endFunction._maxDelay;
  }
  virtual const OperCover &GetCoverFound(int i) const {return AStarCoverContext::_coverCandidates[i];}
  virtual int GetCoverFoundCount() const {return AStarCoverContext::_coverCandidates.Size();}
  
  virtual IAStarOperative *Get2ndPass() const {return _2ndPass;}
  //@}
};

struct AStarCoverRefTraits
{
  static void Delete(AStarCover *ptr)
  {
#if USE_OPERPATH_THREAD
    // remove from the safe heap
    ptr->~AStarCover();
    mtDelete(ptr);
#else
    delete ptr;
#endif
  }
};

#include <Es/Memory/normalNew.hpp>
bool AStarCover::AbortSearch(OperMap *map)
{
  // in any case mark this search as done
  _done = true;
  // if 2nd pass is already completed or marked as skipped, we may not start it, and we need to abort
  if (_2ndPassDone)
  {
    return true;
  }
  // if 2nd pass is already started, we may not start it again, we need to abort it
  if (_2ndPass)
  {
    _2ndPassDone = true;
    return true;
  }

  // reversed test is best run from the destination
  const AStarNode<ASOField> *last = _destNode;
  // sometimes the destination may be not found, in such case we want to run the test from the "alternate end"
  // (point nearest to the destination)
  if (!last) last = GetBestNode(map);
  
  // even when there are no candidates, we still want to run the 2nd pass, because it allows us to select "open cover"
  if (last && !_endFunction._tgtPos._cover)
  {
    // we need to perform the 2nd pass to verify the cost from the target to the cover
    // this allows us to eliminate covers too far from the best path
    // we want to export the path just for case reversed test will eliminate every cover
    // run reversed test, verifying the cover
    // the test may be run from a point on the shortest path where we know the furthest cover may lie
    // traverse the path as long as the cost of the parent is still over the limit
    const AStarNode<ASOField> *safe = last;
    int sinceSafe = 0;
    while (last->_parent && last->_parent->_g>_endFunction._maxCost)
    {
      const AStarNode<ASOField> *parent = last->_parent;
      float cost = last->_g-parent->_g;
      float time = last->_field._t-parent->_field._t;
      last = parent;
      
      float danger = time>0 ? cost/time : cost*1e6f;
      static float maxAllowedDanger = 2.0f;
      static int maxGoodToLastChain = 10;
      // remember safe fields - low cost per time
      if (danger<maxAllowedDanger)
        safe = last, sinceSafe = 0;
      else if (sinceSafe<maxGoodToLastChain) // avoid the chain being too long
        sinceSafe++;
      else
        safe = safe->_parent;
        
    }
    #if 0 // _ENABLE_REPORT
      if (last!=safe)
      {
        LogF("%s: Last g:%g,t:%g, safe g:%g,t:%g, since %d",cc_cast(map->GetDebugName()),last->_g,last->_field._t,safe->_g,safe->_field._t,sinceSafe);
      }
    #endif
    last = safe;
    _2ndPassCost = last->_g;
    // now last is the point we want our reversed search to start
    ASOField revBeg = last->_field;

    // when performing reversed search, the only thing we need to is to reach the covers we are interested in
    // once all covers are reached, we can terminate
    // we could provide a specialized termination + heuristics for this
#if USE_OPERPATH_THREAD
    // allocate on the safe heap
    _2ndPass = ::new(mtNew(sizeof(AStarCover))) AStarCover(
      revBeg,_start,*this,_readyFunction,_interruptFunction,map,*this
    );
#else
    _2ndPass = new AStarCover(
      revBeg,_start,*this,_readyFunction,_interruptFunction,map,
      *this
    );
#endif
    // we need to prevent creating 2nd pass from 2nd pass
    _2ndPass->_2ndPassDone = true;
    
    // report search has to continue - 2nd pass started
    return false;
  }
  else
  {
    // no candidates, no need to perform any 2nd pass
    _2ndPassDone = true;
    return true;
  }
}
#include <Es/Memory/debugNew.hpp>

RString AStarCover::GetDiagText(bool cover) const
{
  if (cover && _2ndPass)
  {
    return _2ndPass->GetDiagText(cover);
  }
  float maxF = ComputeMaxF();

  const AStarNode<ASOField> *best = GetLastNode();
  if (best)
  {
    return base::GetDiagText(cover)+Format(", f=%g, maxF=%g, maxCost=%g",best->_f,maxF,_endFunction._maxCost);
  }
  return base::GetDiagText(cover);
}

int AStarCover::Process(void *context, bool diags)
{
  if (_2ndPass)
  {
    int ret = _2ndPass->Process(context,diags);
    if (_2ndPass->IsDone())
    {
      _2ndPassDone = true;
    }
    return ret;
  }
  int ret = base::Process(context,diags);
  if (base::IsDone())
  {
    // abort search will start 2nd pass as needed
    ASOContext *ctx = (ASOContext *)context;
    AbortSearch(ctx->_map);
  }
  return ret;
}

/// cover candidate used for sorting
struct OperCoverCandidate: public OperCoverAStarCandidate
{
  /// real cost to the destination when going through this cover
  float _gSum;

  #if _ENABLE_CHEATS
  /// source index in the _coverCandidates
  int _i;
  float _cost;
  float _addCost;
  #endif
};

TypeIsMovable(OperCoverCandidate)

/// cover candidate evaluation - leader
struct CmpCoverCandidate
{
  float _preferredG;
  Vector3 _hideFromL;
  Vector3 _hideFromR;

  CmpCoverCandidate(float preferredG, Vector3Par hideFromL, Vector3Par hideFromR)
  :_preferredG(preferredG),_hideFromL(hideFromL),_hideFromR(hideFromR){}

  float CoverCandidateCost(const OperCoverCandidate *a) const
  {
    // we somewhat prefer higher corner covers, and we prefer right corners before left ones
    // low corner is considered to be very bad
    // wall cover is the last option
    static const float typeCost[] = {
      0,
      0, 0.1, 2.2, // RightCornerHigh, RightCornerMiddle, RightCornerLow
      0.5, 0.6, 2.7, // LeftCornerHigh, LeftCornerMiddle, LeftCornerLow
      0.45, 1.6, // EdgeMiddle, EdgeLow
      2.5 // Wall
    }; // see CoverType

    float suitable = CoverSuitableAgainst(*a,_hideFromL,_hideFromR);

    COMPILETIME_COMPARE(lenof(typeCost),NCoverType);
    float cost = typeCost[a->_type.GetEnumValue()];
    static float weightGSum = 1.0f;
    static float weightG = 1.0f;
    static float weightCost = 1.0f;
    static float scoreCost = 1.0f;
    static float unsuitableCost = 5.0f;
    // we get some penalty for high score (high score means small or penetrable)
    return (
      cost*weightCost + a->_gSum*weightGSum + fabs(a->_node->_g-_preferredG)*weightG+
      a->GetScore() + unsuitableCost*(1-suitable)
    );
  }

  int operator () (const OperCoverCandidate *a, const OperCoverCandidate *b) const
  {
    // we prefer the one with both low _g and _f
    float costA = CoverCandidateCost(a);
    float costB = CoverCandidateCost(b);
    return sign(costA-costB);
  }
};

static float FormationCoefOpenCover = 0.1f;
static float FormationCoefCover = 0.1f;

static float FormationCoefFarOpenCover = 2.0f;
static float FormationCoefFarCover = 2.0f;

static float FormationMaxDistOpenCover = 20.0f;
static float FormationMaxDistCover = 50.0f;

/// cover candidate evaluation - subordinate
struct CmpCoverCandidateInFormation: public CmpCoverCandidate
{
  typedef CmpCoverCandidate base;

  Vector3 _formPos;
  bool _strictFormation;

  CmpCoverCandidateInFormation(
    float preferredG, Vector3Par hideFromL, Vector3Par hideFromR, Vector3Par formPos, bool strictFormation
  )
  :base(preferredG,hideFromL,hideFromR),_formPos(formPos),_strictFormation(strictFormation){}

  float AdditionalCost(const OperCoverCandidate *a) const
  {
    float dist = a->_pos.Distance(_formPos);
    return (
      dist * FormationCoefCover+
      +(_strictFormation ? floatMax(dist-FormationMaxDistCover,0) * FormationCoefFarCover : 0)
    );
  }

  int operator () (const OperCoverCandidate *a, const OperCoverCandidate *b) const
  {
    // we prefer the one with both low _g and _f
    float costA = CoverCandidateCost(a)+AdditionalCost(a);
    float costB = CoverCandidateCost(b)+AdditionalCost(b);
    return sign(costA-costB);
  }
};

/// cover candidate evaluation - formation leader prefering a destination
struct CmpCoverCandidateToDestination: public CmpCoverCandidate
{
  typedef CmpCoverCandidate base;

  Vector3 _tgtPos;

  CmpCoverCandidateToDestination(
    float preferredG, Vector3Par hideFromL, Vector3Par hideFromR, Vector3Par tgtPos
  )
  :base(preferredG,hideFromL,hideFromR),_tgtPos(tgtPos){}

  float AdjustedCost(const OperCoverCandidate *a) const
  {
    float cost = CoverCandidateCost(a);
    bool preferDestination = false;
    float dist2 = a->_pos.DistanceXZ2(_tgtPos);
    float distCost = dist2*0.2f; // strongly prefer nearest cover
    if (dist2<Square(2.0f))
    {
      Matrix4 coverPos;
      coverPos.SetPosition(a->_pos);
      coverPos.SetOrientation(Matrix3(MRotationY,H_PI-a->_heading));
      Vector3 coverTgtPos = coverPos.FastTransform(Vector3(0,0,-1));
      if (coverTgtPos.DistanceXZ2(_tgtPos)<Square(0.1f))
      {
        preferDestination = true;
  }
    }

    return (preferDestination ? 0.01f : 1.0f)*cost+100*(1-preferDestination)+distCost;
  }

  int operator () (const OperCoverCandidate *a, const OperCoverCandidate *b) const
  {
    // we prefer the one with both low _g and _f
    float costA = AdjustedCost(a);
    float costB = AdjustedCost(b);
    return sign(costA-costB);
  }
};


float Dist2(int x, int z, int tgtX, int tgtZ) 
{
  return Square(x - tgtX) + Square(z - tgtZ);
}

const AStarNode<ASOField> *AStarCover::GetBestCover(const OperMap &map, CoverInfo &cover, AIBrain *unit, AutoArray<CoverDiagInfo> *diagInfo) const
{
  // check which cover of those we have found is the best
  // we need to use only those which are really usable
  // we will sort them by _g and _f
  // from the sorted ones, we select 1st which will pass
  EntityAIFull *vehicle = unit->GetVehicle();
  if (!vehicle) return NULL;

  #if 1 // real cover selection
  Vector3 hideFromL,hideFromR;
  unit->GetHideFrom(hideFromL,hideFromR);

  float maxF = ComputeMaxF(); // TODO: pass _maxDelay
  AutoArray<OperCoverCandidate, MemAllocLocal<OperCoverCandidate,64,AllocAlign16> > candidates;
  if (_2ndPass)
  {
    // when 2nd pass information is available, we can verify the real cost to the target
    // for each 1st pass candidate we need to find a corresponding 2nd pass one
    #if 1 // verify - this does not seem to have any significant effect
    static bool pruneLongDelay = false;
    if (pruneLongDelay)
    {
    // first we can prune candidates for which even heuristics tells us the delay will be longer than acceptable
    // this should somewhat reduce the lists (esp. the 2nd pass one, which contains a lot of covers far from the starting point)
    int s,d;
    for (s=0,d=0; s<_2ndPass->AStarCoverContext::_coverCandidates.Size(); s++)
    {
      const OperCoverAStarCandidate &cj = _2ndPass->AStarCoverContext::_coverCandidates[s];
      if (cj._node->_f<=maxF)
      {
        if (s!=d)
        {
          _2ndPass->AStarCoverContext::_coverCandidates[d] = cj;
        }
        d++;
      }
    }
    _2ndPass->AStarCoverContext::_coverCandidates.Resize(d);
    for (s=0,d=0; s<AStarCoverContext::_coverCandidates.Size(); s++)
    {
      const OperCoverAStarCandidate &cj = AStarCoverContext::_coverCandidates[s];
      if (cj._node->_f<=maxF)
      {
        if (s!=d)
        {
          AStarCoverContext::_coverCandidates[d] = cj;
        }
        d++;
      }
    }
    AStarCoverContext::_coverCandidates.Resize(d);
    }
    #endif
    
    #if _ENABLE_CHEATS
    if (diagInfo)
    {
      diagInfo->Resize(AStarCoverContext::_coverCandidates.Size());
      for (int d=0; d<diagInfo->Size(); d++) (*diagInfo)[d] = CRBad;
    }
    #endif
    
    for (int i=0; i<AStarCoverContext::_coverCandidates.Size(); i++)
    {
      OperCoverAStarCandidate &ci = AStarCoverContext::_coverCandidates[i];
      // unless 2nd pass will confirm the real cost, we do not want to use it
      float gSum = FLT_MAX;
      for (int j=0; j<_2ndPass->AStarCoverContext::_coverCandidates.Size(); j++)
      {
        const OperCoverAStarCandidate &cj = _2ndPass->AStarCoverContext::_coverCandidates[j];
        if (!(ci==cj)) continue;
        gSum = ci._node->_g + cj._node->_g;
        break;
      }
      // if total cost is too high, ignore the cover
      // this means 2nd pass did not confirm, or confirmed, and the cover is too far from the best path
      if (gSum<maxF)
      {
        OperCoverCandidate &c = candidates.Append();
        c.OperCoverAStarCandidate::operator =(ci);
        c._gSum = gSum;
        #if _ENABLE_CHEATS
          if (diagInfo)
          {
            c._i = i;
            (*diagInfo)[i] = CoverDiagInfo(CRGood,0,ci._node);
          }
        #endif
      }
      else
      {
        #if _ENABLE_CHEATS
        if (diagInfo)
        {
          (*diagInfo)[i] = CoverDiagInfo(CRSlow,gSum,ci._node);
        }
        #endif
      }
      
    }
  }
  else
  {
    #if _ENABLE_CHEATS
    if (diagInfo)
    {
      diagInfo->Resize(AStarCoverContext::_coverCandidates.Size());
      for (int d=0; d<diagInfo->Size(); d++) (*diagInfo)[d] = CRBad;
    }
    #endif
    // no 2nd pass - use what we have from the 1st pass, and use heuristics only
    for (int i=0; i<AStarCoverContext::_coverCandidates.Size(); i++)
    {
      const OperCoverAStarCandidate &ci = AStarCoverContext::_coverCandidates[i];
      float f = ci._node->_f;
      if (f<maxF || _endFunction._tgtPos._cover)
      {
        OperCoverCandidate &c = candidates.Append();
        c.OperCoverAStarCandidate::operator =(ci);
        c._gSum = f;
        #if _ENABLE_CHEATS
          if (diagInfo)
          {
            c._i = i;
            (*diagInfo)[i] = CoverDiagInfo(CRGood,0,ci._node);
          }
        #endif
      }
      else
      {
        #if _ENABLE_CHEATS
        if (diagInfo)
        (*diagInfo)[i] = CoverDiagInfo(CRSlowP1,f,ci._node);
        #endif
      }
      
    }
  }

  // we need to detect corner sides for corner covers
  // we should detect angle suitability as well
  for (int i=0; i<candidates.Size(); i++)
  {
    OperCoverCandidate &ci = candidates[i];
    // corner covers are always rasterized as Left corner
    if (ci._type.GetEnumValue()<CoverEdgeMiddle)
    {
      // detect if this is left or right corner
      // this is done based on cover angle
      // check sign of cross product with a direction to target
      // check if we should use it as a left or right cover
      // TODO: use all corner covers as bidirectional, always computing both left and right areas
      Vector3 hideL = hideFromL-ci._pos;
      Vector3 hideR = hideFromR-ci._pos;
      float headingL = atan2(hideL.X(),hideL.Z());
      float headingR = atan2(hideR.X(),hideR.Z());
      
      if (CHECK_DIAG(DECombat) || CHECK_DIAG(DECostMap))
      {
        bool one = hideL.CosAngle(hideR)>0.999f;
        static int handleL;
        GDiagsShown.ShowArrow(handleL,i,100,Format(one ? "LR %d" : "L %d",i),ci._pos,hideL.Normalized(),0.9,Color(0,1,0));
        if (!one)
        {
          static int handleR;
          GDiagsShown.ShowArrow(handleR,i,100,Format("R %d",i),ci._pos,hideR.Normalized(),0.9,Color(1,1,0));
        }
      }
      
      // create a "canonical" form, so that we can perform comparisons
      if (headingR<headingL) headingR += H_PI*2;
      float heading = (headingL+headingR)*0.5f;
      
      // now we have two headings, and we want to compare them for left/right 
      // TODO: perform comparison directly with ci._heading, headingL and headingR
      float side = AngleDifference(heading,ci._heading+H_PI);
      // heading>coverOutHeading means enemies are rigth of the cover
      // we want to use it as a right one is such case
      if (side>0)
      {
        switch (ci._type.GetEnumValue())
        {
          case CoverLeftCornerHigh: ci._type = CoverRightCornerHigh; break;
          case CoverLeftCornerMiddle: ci._type = CoverRightCornerMiddle; break;
          case CoverLeftCornerLow: ci._type = CoverRightCornerLow; break;
        }
      }
    }
  }
  // if unit is not a formation leader, we somewhat prefer candidates
  // which are close to where unit is supposed to be located in the formation
  float preferredG = _endFunction._maxCost*0.5;
  if (_endFunction._tgtPos._cover)
  {
    // select the cover matching the given order
    CmpCoverCandidateToDestination cmp(preferredG,hideFromL,hideFromR,_endFunction._tgtPos);
    QSort(candidates,cmp);
    #if _ENABLE_CHEATS
      for (int i=0; i<candidates.Size(); i++)
      {
        candidates[i]._cost = cmp.AdjustedCost(&candidates[i]);
        candidates[i]._addCost = cmp.CoverCandidateCost(&candidates[i]);
      }
      if (diagInfo) for (int i=0; i<candidates.Size(); i++)
      {
        OperCoverCandidate &ci = candidates[i];
        // leave as accepted, only fill order info
        (*diagInfo)[ci._i].extValue = i;
        (*diagInfo)[ci._i].extValue2 = cmp.AdjustedCost(&ci);
      }
    #endif
  }
  else
  {
  // for the formation leader the candidate selection is simpler
  CmpCoverCandidate cmp(preferredG,hideFromL,hideFromR);
  QSort(candidates,cmp);
  #if _ENABLE_CHEATS
    for (int i=0; i<candidates.Size(); i++)
    {
      candidates[i]._cost = cmp.CoverCandidateCost(&candidates[i]);
      candidates[i]._addCost = 0;
    }
    if (diagInfo) for (int i=0; i<candidates.Size(); i++)
    {
      OperCoverCandidate &ci = candidates[i];
      // leave as accepted, only fill order info
      (*diagInfo)[ci._i].extValue = i;
      (*diagInfo)[ci._i].extValue2 = cmp.CoverCandidateCost(&ci);
    }
  #endif
  }

  for (int i=0; i<candidates.Size(); i++)
  {
    const OperCoverAStarCandidate &ci = candidates[i];
    Vector3 entryPos;
    const ASOField &field = ci._node->_field;
    if (field.type == NTHouse)
    {
      // leaving house
      const IPaths *paths = unit->GetOperMap().GetHouse(field.GetHouseIndex());
      if (!paths) continue;
      entryPos = paths->GetPosition(field.GetPosIndex());
    }
    else if (field.type == NTRoad || field.type == NTBridge)
    {
      const OperRoad &info = unit->GetOperMap().GetRoad(field.GetRoadIndex());
      const OperRoadNode &node = info._nodes[field.GetNodeIndex()];
      entryPos = node._pos;
    }
    else
    {
      // several candidates are inserted for the same cover, each with a different entry
      // A* has selected the most suitable entry
      OperMapIndex x = field.GetX();
      OperMapIndex z = field.GetZ();

      entryPos[0] = (x + 0.5f) * OperItemGrid;
      entryPos[1] = 0;
      entryPos[2] = (z + 0.5f) * OperItemGrid;
    }

    // find exact location in the cover to move to
    CoverVars coverVars;
    // with left cover we are interested about the leftmost enemy, with right cover about the rightmost
    // with edge/wall covers we care about an average direction instead
    Vector3 tgtPos;
    switch (ci._type.GetEnumValue())
    {
    case CoverRightCornerHigh: case CoverRightCornerMiddle: case CoverRightCornerLow: 
      tgtPos = hideFromR;
      break;
    case CoverLeftCornerHigh: case CoverLeftCornerMiddle: case CoverLeftCornerLow:
      tgtPos = hideFromL;
      break;
    case CoverEdgeLow: case CoverEdgeMiddle: case CoverWall:
      {
        Vector3 hideL = hideFromL-ci._pos;
        Vector3 hideR = hideFromR-ci._pos;
        float headingL = atan2(hideL.X(),hideL.Z());
        float headingR = atan2(hideR.X(),hideR.Z());
        
        // create a "canonical" form, so that we can perform comparisons
        if (headingR<headingL) headingR += H_PI*2;
        float heading = (headingL+headingR)*0.5f;
        tgtPos = Matrix3(MRotationY,-heading).Direction()*100+ci._pos;
      }
      break;
    }
    if (vehicle->FindCover(coverVars,ci,tgtPos))
    {
      // we need to verify that the entry found by A* found is a point within the cover limits
      Matrix4 toCover(MInverseRotation,coverVars._coverPos);
      Vector3 entryLocal = toCover.FastTransform(entryPos);

      if (
        entryLocal.X()>=-coverVars._areaFreeLeft && entryLocal.X()<coverVars._areaFreeRight &&
        entryLocal.Z()>=-coverVars._areaFreeBack && entryLocal.Z()<0
      )
      {
        cover = ci;
        cover._payload = new CoverVars(coverVars);
        cover._payload->_entry = entryPos;
        cover._payload->_entry[1] = coverVars._coverPos.Position().Y();
        
        if (CHECK_DIAG(DECombat) || CHECK_DIAG(DECostMap))
        {
          static int handle;
          GDiagsShown.ShowArrow(handle,i,100,Format("Entry %d",i),cover._payload->_entry,VUp,1.22f,Color(0.5,0.5,0.5));
        }
        
        return ci._node;
      }
      else
      {
        if (CHECK_DIAG(DECombat) || CHECK_DIAG(DECostMap))
        {
          struct ShowDiagCover: public IShowDiagDelegate
          {
            CoverVars cover;   
            ShowDiagCover(const CoverVars &cover):cover(cover){}
            virtual void Draw() override
            {
              Vector3 minmax[2];
              minmax[0] = Vector3(-cover._areaFreeLeft,0.0f,-cover._areaFreeBack);
              minmax[1] = Vector3(+cover._areaFreeRight,2.0f,0);

              LODShape *shape = GScene->Preloaded(SphereModel);

              Color color(0.5,0.5,0.5);
              for( int lr=0; lr<2; lr++ )
              for( int ud=0; ud<2; ud++ )
              for( int fb=0; fb<2; fb++ )
              {
                Vector3 wCorner=Vector3(minmax[lr][0],minmax[ud][1],minmax[fb][2]);
                Ref<Object> obj=new ObjectColored(shape,VISITOR_NO_ID);
                obj->SetPosition(cover._coverPos.FastTransform(wCorner));
                obj->SetScale( 0.1 );
                obj->SetConstantColor(color);
                GScene->ObjectForDrawing(obj);
              }
            }
            virtual Vector3 Pos() const override {return cover._pos;}
          };
          static int handleBox;
          GDiagsShown.Show(handleBox,i,100,new ShowDiagCover(coverVars));
          static int handle;
          GDiagsShown.ShowArrow(handle,i,100,Format("No entry %d for %d",i,(int)coverVars._type),GLandscape->PointOnSurface(entryPos.X(),0,entryPos.Z()),VUp,1.1f,Color(0.25,0.25,0.5));
        }
        #if _ENABLE_CHEATS
        if (diagInfo)
        {
          (*diagInfo)[candidates[i]._i] = CoverDiagInfo(CROut,i,candidates[i]._node);
        }
        #endif
      }
    }
    else
    {
      #if _ENABLE_CHEATS
      if (diagInfo)
      {
        (*diagInfo)[candidates[i]._i] = CoverDiagInfo(CRNotF,i,candidates[i]._node);
      }
      #endif
    }
  }
  #endif

  const ASONode *altEnd = NULL;
  #if 1 // open cover selection
  // scan intersection of pass1 and pass2 closed lists
  {
    IteratorState it;
    ASONodeRef node;
    if (_2ndPass && _2ndPass->_closed.getFirst(it,node))
    {
      float bestCost = FLT_MAX;
      do 
      {
        // check if the node can be found in the 1st pass
        if (const ASONodeRef *node1stPassRef = _closed.get(GetASONodeRefKey(node)))
        {
          const ASONode *node1stPass = *node1stPassRef;
          // if there is no parent, it must be the 1st node - we do not want it
          if (node1stPass->_parent)
          {
            // we have both nodes
            // we can perform any evaluation based on both of them
            float nodeRouteCost = node->_g+node1stPass->_g;
            float nodeDetour = nodeRouteCost-_2ndPassCost;
            float nodeFromStartCost = node1stPass->_g;
            float nodeToEndCost = node->_g;
            float nodeFieldCost = node1stPass->_g-node1stPass->_parent->_g;
            
            // if the node is not leading closer to the finish point or detour is too high, ignore the node
            if (nodeToEndCost>_2ndPassCost || nodeDetour>_endFunction._maxDelay) continue;
            
            // similar to AStarCoverEnd::operator ()
            OperMapIndex x(-1), z(-1);
            node->_field.GetXZ(map, x, z);

            // check if the field is bringing us towards the target - if not, ignore it
//             float distBest2 = Dist2(x,z,_endFunction._tgtX,_endFunction._tgtZ);
//             if (distBest2 <= Square(_endFunction._distToDest))
            {
//               Vector3 tgtPos(_endFunction._tgtX*OperItemGrid,0,_endFunction._tgtZ*OperItemGrid);
              Vector3 nodePos(x*OperItemGrid,0,z*OperItemGrid);
              // similar to CoverCandidateCost
              float nodeTotalCost = (
                // we want: acceptable detour
                nodeDetour*1.0f
                // good distance from the starting point for the formation leader
                +fabs(nodeFromStartCost-unit->GetPreferredCostToCover())*1.0f
                // low cost for the node itself - very important
                +nodeFieldCost*10.0f
              );
              
              // if the cover is too hard to reach from the starting point, give it a huge penalty
              if (nodeFromStartCost>_endFunction._maxCost) nodeTotalCost += 50.0f;
              // when too near the starting point, it is even worse, unless this is needed to keep in formation
              if (nodeFromStartCost<_endFunction._maxCost*0.2f) nodeTotalCost += 200.0f;

              #if 1
              if (CHECK_DIAG(DEFSM) && unit->GetVehicle()==GWorld->CameraOn())
              {
                static int handle;
                static int unique; // make unique so that each arrow stays displayed until timeout
                static float CostScale = 0.0211f;
                float scale = floatMinMax(nodeTotalCost*CostScale,0.1f,1.0f);
                Vector3 nodePosY = map.GetNodePos(node);
                GDiagsShown.ShowArrow(
                  handle,unique++,10,Format("Cost %.1f, start %.1f",nodeTotalCost,nodeFromStartCost),
                  nodePosY,VUp,scale,Color(1,0,0)
                );
              }
              #endif

              if (bestCost>nodeTotalCost)
              {
                #if _DEBUG
                  Vector3 v1Pos = map.GetNodePos(node);
                  Vector3 v2Pos = map.GetNodePos(node1stPass);
                #endif
                bestCost = nodeTotalCost;
                altEnd = node1stPass;
              }
            }

          }
        }
        
      } while (_2ndPass->_closed.getNext(it,node));
    }
  }
  #endif
  
  if (!altEnd)
  {
    const AStarNode<ASOField> *last = NULL;
    // simulate what ExportPath would do
    Assert(IsDone());
    if (IsFound())
    {
      // path found
      last = GetLastNode();
    }
    else
    {
      // path not found - try to select alternate goal
      last = GetBestNode(&map);
      if (last && !last->_parent) last = false; // handle singular path (last is start node)
    }
    // when covering is enabled and no cover was found, stop in the open field
    // this means drop the end of the path, to keep only its initial part which is interesting for us
    float limitT = unit->GetPreferredCostToCover();
    for (const ASONode *cur=last; cur != NULL; cur=cur->_parent)
    {
      if (cur->_field._t<limitT)
      {
        altEnd = cur;
        break;
      }
    }
  }
  
  // when covering is enabled, always mark the path as leading into a cover
  // this will prevent re-planning it, and it will make sure AI is reporting reaching it
  // mark the path as leading into an open cover
  cover = CoverInfo();
  
  if (CHECK_DIAG(DEPath))
  {
    static int handle;
    static int unique; // make unique so that each arrow stays displayed until timeout
    Vector3 pos = DetectFreedMemory(altEnd) ? unit->Position(unit->GetFutureVisualState()) : map.GetNodePos(altEnd);
    Color color = Color(1,0,0);
    GDiagsShown.ShowArrow(handle,unique++,10,unit->GetDebugName()+" AltEnd",pos,VUp,3,color);
  }
  
  return altEnd;
}

// optimized
static inline float OperHeuristic( float dx, float dz )
{
  dx=fabs(dx);
  dz=fabs(dz);
  float sum=dx+dz;
  float adif=fabs(dx-dz);
  float minD2=(sum-adif);
  float maxD2=(sum+adif);
  return (maxD2 - minD2)*0.5 + ( H_SQRT2 *0.5 )* minD2;
}

int AI::CalcDirection(Vector3Par direction)
{
  // TODO: avoid atan2
  // 16 directions (+-8)

  float angle = atan2(direction.X(), direction.Z()) + H_PI;
  // angle is from 0 to 2*pi

  angle *= 128 / H_PI;
  //int dir = toInt(angle) & (Directions - 1);
  int dir = toInt(angle);
  // we want result in range 0..255, mod 256
  return dir&255;
}

int AI::DirectionAngleDifference(int dirD)
{
  // we expect result -128..+127
  return ((dirD+128)&255)-128;
}


#if _ENABLE_CHEATS

/*!
\patch_internal 5089 Date 11/22/2006 by Jirka
- Improved: support for scrollbars in operative map debug window
*/

#include <El/DebugWindow/debugWinImpl.hpp>

// class DebugWindowOperMap
class DebugWindowOperMap : public DebugWindow
{
  typedef DebugWindow base;
  
protected:
  OLinkPerm<AIBrain> _unit;
  int _xs, _zs, _xe, _ze;
  bool _unitAssigned;
  float _heurCost;
  float _scale;
  /// used to construct the title
  RString _title;
  
  /// diagnose suppression fire as well?
  bool _suppression;
  
  /// diagnose cover search?
  bool _cover;
  
  void SetTitle();
  
  int GetItemSize()
  {
    return toInt(OperItemGrid*3*_scale);
  }

public:
  DebugWindowOperMap(bool cover);
  void Attach(AIBrain *unit, int xs, int zs, int xe, int ze, float heurCost, bool cover, RString title=RString());
  void Detach(AIBrain *unit, bool cover);
  void AssignUnit(AIBrain *unit) {_unit = unit; _unitAssigned = true;}
  void UnassignUnit() {_unitAssigned = false;}
  bool IsAttached() const {return _unit != NULL;}

  void Update();
  void OnPaint( const OnPaintContext &dc);

  void PaintAlg(
    const OperMap &map, IAStarOperative *alg, bool extInfo, int xMin, int xMax, int zMin, int zMax, int xScroll, int yScroll, int itemSize,
    HDC dc, HPEN openPen, HPEN closedPen, HPEN pathPen, int xoff, int zoff, bool is2ndPass
  );
  void OnScroll(int bar, int request, int pos);
  void OnKeyDown(unsigned short key, unsigned short repCnt, unsigned short flags);
};

DebugWindowOperMap::DebugWindowOperMap(bool cover)
: DebugWindow(cover ? "Debug - Oper. cover Map" : "Debug - Operative Map", true, true)
{
  _unit = NULL;
  _unitAssigned = false;
  _heurCost = 1;
  _scale = 1;
  _suppression = true;
  _cover = cover;
}

#include "../global.hpp"

void DebugWindowOperMap::SetTitle()
{
  if (_unit)
  {
    EntityAI *veh = _unit->GetVehicle();
    const OperMap &map = _unit->GetOperMap();
    RString winTitle = Format(
      "%s map for %s %s %s: %s, G.time: %.2f, iter %d, h: %.2f, %s",
      _cover ? "Oper. cover" : "Operative",
      cc_cast(_title),
      cc_cast(veh->GetType()->GetDisplayName()),
      cc_cast(_unit->GetDebugName()),
      map.IsDone() ? (map.IsFound() ? "found" : "not found") : "searching",
      Glob.time-Time(0), _unit->GetIter(),
      _heurCost * veh->GetType()->GetMaxSpeedMs(),
      cc_cast(map.GetDiagText(_cover))
    );
    HWND hwnd = GetWindowHandle(this);
    SetWindowText(hwnd, winTitle);
  }
}

void DebugWindowOperMap::Attach(AIBrain *unit, int xs, int zs, int xe, int ze, float heurCost, bool cover, RString title)
{
  if (_unitAssigned)
  {
    if (_unit != unit) return;
  }
  else
    _unit = unit;
  _title = title;
  _xs = xs;
  _zs = zs;
  _xe = xe;
  _ze = ze;
  _heurCost = heurCost;
  if (unit)
  {
    SetTitle();
  }
}

void DebugWindowOperMap::Update()
{
  // make sure rounding error do not cummulatem - when near to integer value, use the integer value
  int intScale = toInt(_scale);
  if (fabs(_scale-intScale)<0.01f)
  {
    _scale = intScale;
  }
  base::Update();
  SetTitle();
}

void DebugWindowOperMap::Detach(AIBrain *unit, bool cover)
{
  if (_unitAssigned && _unit != unit) return;

  char buffer[1024];
  HWND hwnd = GetWindowHandle(this);
  GetWindowText(hwnd, buffer, sizeof(buffer));
  RString text = RString(buffer) + RString("[Finished]");
  SetWindowText(hwnd,text);
}

#define XXL(x) ((x - OperItemRange * xMin) * itemSize - xScroll)
#define ZZB(z) ((OperItemRange * (zMax + 1) - z) * itemSize - yScroll)

#define XX(x) ((x - OperItemRange * xMin) * itemSize + itemSize / 2 - xScroll)
#define ZZ(z) ((OperItemRange * (zMax + 1) - z) * itemSize - itemSize / 2 - yScroll)

void DebugWindowOperMap::OnKeyDown(unsigned short key, unsigned short repCnt, unsigned short flags)
{
  switch (key)
  {
  case VK_ADD:
    _scale *= H_SQRT2;
    Update();
    break;
  case VK_SUBTRACT:
    _scale *= (1.0f/H_SQRT2);
    Update();
    break;
  case 'S':
    _suppression = !_suppression;
    Update();
  case 'C':
    _cover = !_cover;
    Update();
    break;
  }
}

void DebugWindowOperMap::OnPaint(const OnPaintContext &pc)
{
  int itemSize = GetItemSize();
  HDC dc = pc.dc;
  HWND hwnd = GetWindowHandle(this);
  RECT clientRect;
  GetClientRect(hwnd, &clientRect);

  SCROLLINFO info;
  info.cbSize = sizeof(info); 

  bool valid = false;
  if (_unit)
  {
    const OperMap &map = _unit->GetOperMap();
    valid = map._fields.card() > 0;
  }

  if (!valid)
  {
    HBRUSH hbrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    FillRect(dc, &clientRect, hbrush);

    info.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
    info.nMin   = 0; 
    info.nMax   = 0; 
    info.nPage  = OperItemRange; 
    info.nPos   = 0; 
    SetScrollInfo(hwnd, SB_HORZ, &info, TRUE); 

    info.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
    info.nMin   = 0; 
    info.nMax   = 0; 
    info.nPage  = OperItemRange; 
    info.nPos   = 0; 
    SetScrollInfo(hwnd, SB_VERT, &info, TRUE); 

    return;
  }

  const OperMap &map = _unit->GetOperMap();
  SuppressTargetList suppressTargets;
  suppressTargets.CollectTargets(_unit);

  int xMin = INT_MAX;
  int xMax = INT_MIN;
  int zMin = INT_MAX;
  int zMax = INT_MIN;

  IteratorState it;
  const OperMapField *fld = map._fields.getFirst(it);
  while (fld)
  {
    saturateMin(xMin, fld->_operField->X());
    saturateMax(xMax, fld->_operField->X());
    saturateMin(zMin, fld->_operField->Z());
    saturateMax(zMax, fld->_operField->Z());
    
    fld = map._fields.getNext(it);
  }
  
  const int fieldSize = OperItemRange * itemSize;

  int xCount = xMax - xMin + 1;
  int zCount = zMax - zMin + 1;

  int w = fieldSize * xCount;
  int h = fieldSize * zCount;

  // read the current position
  info.fMask = SIF_POS;
  int xScroll = 0;
  if (GetScrollInfo(hwnd, SB_HORZ, &info) != 0) xScroll = info.nPos;
  int yScroll = 0;
  if (GetScrollInfo(hwnd, SB_VERT, &info) != 0) yScroll = info.nPos;

  // set scrollbar range
  saturateMin(xScroll, w); 
  info.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
  info.nMin   = 0; 
  info.nMax   = w; 
  info.nPage  = clientRect.right; 
  info.nPos   = xScroll; 
  SetScrollInfo(hwnd, SB_HORZ, &info, TRUE); 

  saturateMin(yScroll, h); 
  info.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
  info.nMin   = 0; 
  info.nMax   = h; 
  info.nPage  = clientRect.bottom; 
  info.nPos   = yScroll; 
  SetScrollInfo(hwnd, SB_VERT, &info, TRUE);

  if (clientRect.bottom > h - yScroll)
  {
    RECT rect;
    rect.left = 0;
    rect.right = clientRect.right;
    rect.top = h - yScroll;
    rect.bottom = clientRect.bottom;
    HBRUSH hbrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    FillRect(dc, &rect, hbrush);
    clientRect.bottom = h - yScroll;
  }
  if (clientRect.right > w - xScroll)
  {
    RECT rect;
    rect.left = w - xScroll;
    rect.right = clientRect.right;
    rect.top = 0;
    rect.bottom = clientRect.bottom;
    HBRUSH hbrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    FillRect(dc, &rect, hbrush);
    clientRect.right = w - xScroll;
  }

  EntityAI *veh = _unit->GetVehicle();
  const EntityAIType *vehType = veh->GetType();
  CombatMode mode = _unit->GetCombatMode();
  bool soldier = _unit->IsFreeSoldier();

  float unaccLimit = GET_UNACCESSIBLE;
  float invHeurCost = 1.0 / _heurCost;

  AutoArray<bool, MemAllocLocal<bool,256> > used;
  used.Resize(zCount * xCount);
  for (int i=0; i<used.Size(); i++) used[i] = false;

  int fontSize = itemSize/5;
  if (fontSize<8) fontSize = 8;
  HANDLE textFont = CreateFont(
   8,                        // nHeight
   0,                         // nWidth
   0,                         // nEscapement
   0,                         // nOrientation
   FW_NORMAL,                 // nWeight
   FALSE,                     // bItalic
   FALSE,                     // bUnderline
   0,                         // cStrikeOut
   ANSI_CHARSET,              // nCharSet
   OUT_DEFAULT_PRECIS,        // nOutPrecision
   CLIP_DEFAULT_PRECIS,       // nClipPrecision
   DEFAULT_QUALITY,           // nQuality
   DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
   NULL /*"Arial"*/);                 // lpszFacename

  HANDLE oldFont = SelectObject(dc,textFont);
  SetBkMode(dc,TRANSPARENT);
  // field costs and locks
  HPEN lockPen = CreatePen(PS_SOLID, 0, RGB(255, 0, 255));
  HGDIOBJ oldPen = SelectObject(dc, lockPen);

  fld = map._fields.getFirst(it);
  while (fld)
  {
    const SuppressField *supp = fld->_suppressField;
    
    used[(fld->_operField->Z() - zMin) * xCount + (fld->_operField->X() - xMin)] = true;
    float baseCost = fld->_baseCost;

    RECT rect;
    float left = fieldSize * (fld->_operField->X() - xMin) - xScroll; 
    rect.bottom = fieldSize * (zMax + 1 - fld->_operField->Z()) - yScroll;
    for (OperFieldIndex z(0); z<OperItemRange; z++)
    {
      rect.top = rect.bottom - itemSize;
      rect.left = left;
      for (OperFieldIndex x(0); x<OperItemRange; x++)
      {
        OperItem item = fld->_operField->GetItem(x, z);
        rect.right = rect.left + itemSize;
        OperItemType type = soldier ?
          item.GetTypeSoldier() :
          item.GetType();
        Clearance clearance = item.GetClearance();
        float cost = vehType->GetTypeCost(type, mode,clearance).cost * baseCost;

        if (supp && _unit && _suppression)
        {
          cost += supp->GetCost(x,z,suppressTargets);
        }
        
        COLORREF color;
#if 0
        if (item._houseExitPresent)
        {
          color = RGB(0, 0, 0);
        }
        else
#endif
        if (cost >= unaccLimit)
          color = RGB(255, 0, 0);
        else
        {
          if (cost <= _heurCost)
          {
            int green = toLargeInt(cost * invHeurCost * 256.0f);
            saturate(green, 0, 255);
            color = RGB(0, (BYTE)green, 0);
          }
          else if (cost <= 3.0f * _heurCost)
          {
            int red = toLargeInt((cost - _heurCost) * invHeurCost * 128.0f);
            saturate(red, 0, 255);
            color = RGB(red, 255, 0);
          }
          else
          {
            int nogreen = toLargeInt((cost - 3.0 * _heurCost) * invHeurCost * 32.0f);
            saturate(nogreen, 0, 191);
            color = RGB(255, (BYTE)(255 - nogreen), 0);
          }
        }
        HBRUSH hbrush = CreateSolidBrush(color);
        FillRect(dc, &rect, hbrush);
        DeleteObject(hbrush);
        
        OperMapIndex totalX((fld->_operField->X() << OperItemRangeLog) + x);
        OperMapIndex totalZ((fld->_operField->Z() << OperItemRangeLog) + z);
        if (map.IsLocked(totalX, totalZ, fld, x, z, soldier))
        {
          MoveToEx(dc, rect.left, rect.top, NULL);
          LineTo(dc, rect.right, rect.bottom);
          MoveToEx(dc, rect.left, rect.bottom, NULL);
          LineTo(dc, rect.right + 1, rect.top);
        }
        

        rect.left = rect.right;
      }
      rect.bottom = rect.top;
    }
  
    fld = map._fields.getNext(it);
  }
  SelectObject(dc, oldPen);
  DeleteObject(lockPen);

  int i = 0;
  for (int z=0; z<zCount; z++)
    for (int x=0; x<xCount; x++)
      if (!used[i++])
      {
        RECT rect;
        rect.left = fieldSize * x - xScroll;
        rect.right = rect.left + fieldSize;
        rect.bottom = fieldSize * (zCount - z) - yScroll;
        rect.top = rect.bottom - fieldSize;
        HBRUSH hbrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
        FillRect(dc, &rect, hbrush);
      }

  // houses, exits
  HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
  oldPen = SelectObject(dc, GetStockObject(WHITE_PEN));
  for (int i=0; i<map._houses.Size(); i++)
  {
    const OperHouse &house = map._houses[i];
    for (int j=0; j<house._exits.Size(); j++)
    {
      const OperHouseExit &pos = house._exits[j];
      int left = XXL(pos._x);
      int bottom = ZZB(pos._z);
      int right = left + itemSize;
      int top = bottom - itemSize;
      Ellipse(dc, left, top, right, bottom);
    }
  }
  SelectObject(dc, oldPen);
  SelectObject(dc, oldBrush);

  float vehWidth = 0;
  if (!soldier)
  {
    LODShape *shape = vehType->GetShape();
    if (shape) vehWidth = shape->Max().X() - shape->Min().X();
  }

  // roads
  HPEN roadPen = CreatePen(PS_SOLID, 0, RGB(255, 255, 255));
  HPEN roadOnlyPen = CreatePen(PS_SOLID, 0, RGB(0, 255, 255));
  HPEN roadLockedPen = CreatePen(PS_SOLID, 0, RGB(255, 128, 128));
  oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
  for (int i=0; i<map._roads.Size(); i++)
  {
    const OperRoad &road1 = map._roads[i];
    for (int j=0; j<road1._nodes.Size(); j++)
    {
      const OperRoadNode &info1 = road1._nodes[j];
      int x1 = toIntFloor(XXL(info1._pos.X() * InvOperItemGrid));
      int z1 = toIntFloor(ZZB(info1._pos.Z() * InvOperItemGrid));
      RoadNode node1(road1._road, j);
      for (int k=0; k<node1.NConnections(soldier); k++)
      {
        RoadNode node2 = node1.GetConnection(soldier, k);
        if (node2.IsValid())
        {
          if (map.IsRoadLocked(node1.GetRoad(), soldier) || map.IsRoadLocked(node2.GetRoad(), soldier))
            SelectObject(dc, roadLockedPen);
          else if (node1.GetRoad()->IsBridge() || node2.GetRoad()->IsBridge())
            SelectObject(dc, roadOnlyPen);
          else
            SelectObject(dc, roadPen);
          Vector3 pos2 = node2.Position(soldier, vehWidth);
          int x2 = toIntFloor(XXL(pos2.X() * InvOperItemGrid));
          int z2 = toIntFloor(ZZB(pos2.Z() * InvOperItemGrid));
          MoveToEx(dc, x1, z1, NULL);
          LineTo(dc, x2, z2);
        }
      }
    }
  }
  SelectObject(dc, oldPen);
  DeleteObject(roadPen);
  DeleteObject(roadOnlyPen);
  DeleteObject(roadLockedPen);

  {
    // start & end point
    oldPen = SelectObject(dc, GetStockObject(WHITE_PEN));
    HBRUSH sbrush = CreateSolidBrush(RGB(255, 0, 255));
    HBRUSH ebrush = CreateSolidBrush(RGB(0, 255, 255));
    oldBrush = SelectObject(dc, sbrush);
    {
      int left = XXL(_xs);
      int bottom = ZZB(_zs);
      int right = left + itemSize;
      int top = bottom - itemSize;
      Ellipse(dc, left + 1, top + 1, right - 1, bottom - 1);
    }
    SelectObject(dc, ebrush);
    {
      int left = XXL(_xe);
      int bottom = ZZB(_ze);
      int right = left + itemSize;
      int top = bottom - itemSize;
      Ellipse(dc, left + 1, top + 1, right - 1, bottom - 1);
    }
    SelectObject(dc, oldBrush);
    DeleteObject(ebrush);
    DeleteObject(sbrush);
    SelectObject(dc, oldPen);
  
  }
  
  {
    if (map._algorithm)
    {
      IAStarOperative *pass2 = map._algorithm->Get2ndPass();
      if (_cover && pass2)
      {
        oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
        HPEN openPen = CreatePen(PS_SOLID, 0, RGB(100, 100, 255));
        HPEN closedPen = CreatePen(PS_SOLID, 0, RGB(0, 0, 40));
        HPEN pathPen = CreatePen(PS_SOLID, 0, RGB(30, 30, 200));
        PaintAlg(
          map, pass2, _cover, xMin, zMin, xMax, zMax, xScroll, yScroll, itemSize, dc,
          openPen, closedPen, pathPen, 2, 1, true
        );
        SelectObject(dc, oldPen);
        DeleteObject(closedPen);
        DeleteObject(openPen);
        DeleteObject(pathPen);
      }

      // we can ask for the best cover only in the 1st pass
      if (!_cover)
      {
        CoverInfo bestCoverInfo;
        const AStarNode<ASOField> *bestCover = map._algorithm->GetBestCover(map,bestCoverInfo,_unit,NULL);
        if (bestCover)
        {
          // start & end point
          DWORD color = bestCoverInfo ? RGB(128, 255, 0) : RGB(128, 128, 0); // yellow indicates open cover
          HBRUSH bbrush = CreateSolidBrush(color);
          HGDIOBJ oldPen = SelectObject(dc, GetStockObject(WHITE_PEN));
          HGDIOBJ oldBrush = SelectObject(dc, bbrush);
          
          {
            Vector3 pos = map.GetNodePos(bestCover)-Vector3(OperItemGrid*0.5f,0,OperItemGrid*0.5f);
            int left = XXL(pos.X()*InvOperItemGrid)+itemSize/8;
            int bottom = ZZB(pos.Z()*InvOperItemGrid)-itemSize/8;
            int right = left + itemSize - itemSize/4;
            int top = bottom - itemSize + itemSize/4;
            Ellipse(dc, left + 1, top + 1, right - 1, bottom - 1);
          }
          DeleteObject(bbrush);
      
          SelectObject(dc, oldBrush);
          SelectObject(dc, oldPen);
        }
      }

      {
        oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
        HPEN openPen = CreatePen(PS_SOLID, 0, RGB(191, 191, 191));
        HPEN closedPen = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
        HPEN pathPen = CreatePen(PS_SOLID, 0, RGB(64, 64, 128));
        PaintAlg(
          map, map._algorithm, !_cover, xMin, zMin, xMax, zMax, xScroll, yScroll, itemSize, dc,
          openPen, closedPen, pathPen, 0, 0, false
        );
        SelectObject(dc, oldPen);
        DeleteObject(closedPen);
        DeleteObject(openPen);
        DeleteObject(pathPen);
      }
    }

    
  }
    

  SelectObject(dc,oldFont);
  DeleteObject(textFont);
}

struct CoverTextCount
{
  int x,z;
  int textCount;
  
  CoverTextCount(){}
  CoverTextCount(int xx, int zz, int c=0){x=xx;z=zz;textCount=c;}
  ClassIsSimple(CoverTextCount)
};

template <>
struct MapClassTraits<CoverTextCount>: public DefMapClassTraits<CoverTextCount>
{
  typedef CoverTextCount KeyType;
  
  static unsigned int CalculateHashValue(const KeyType &key)
  {
    return key.x+key.z*33;
  }

  /// compare keys, return negative when k1<k2, positive when k1>k2, zero when equal
  static int CmpKey(KeyType k1, KeyType k2)
  {
    if (k1.x!=k2.x) return 1;
    if (k1.z!=k2.z) return 1;
    return 0;
  }

  /// get a key for given a item
  static KeyType GetKey(const CoverTextCount &item) {return item;}
  
};

void DebugWindowOperMap::PaintAlg(
  const OperMap &map, IAStarOperative *alg, bool extInfo, int xMin, int xMax, int zMin, int zMax, int xScroll, int yScroll, int itemSize,
  HDC dc, HPEN openPen, HPEN closedPen, HPEN pathPen, int xoff, int zoff, bool is2ndPass
)
{
  // closed / opened nodes
  const ASOClosedList &list = alg->GetClosedList();
  for (ASOClosedList::Iterator iterator(list); iterator; ++iterator)
  {
    const ASONode *node = *iterator;

    // HOTFIX: handle at least some MT collisions
    if (DetectFreedMemory(node)) break; // probably inside ASOClosedList::resize

    if (!node->_parent) continue;

    bool open = node->_open < 0;

    int x1, z1, x2, z2;
    if (node->_field.type == NTHouse)
    {
      // leaving house
      const IPaths *paths = map.GetHouse(node->_field.GetHouseIndex());
      if (!paths) continue;
      Vector3 pos = paths->GetPosition(node->_field.GetPosIndex());
      x1 = toIntFloor(XXL(pos.X() * InvOperItemGrid));
      z1 = toIntFloor(ZZB(pos.Z() * InvOperItemGrid));
    }
    else if (node->_field.type == NTRoad || node->_field.type == NTBridge)
    {
      const OperRoad &road = map._roads[node->_field.GetRoadIndex()];
      const OperRoadNode &info = road._nodes[node->_field.GetNodeIndex()];
      x1 = toIntFloor(XXL(info._pos.X() * InvOperItemGrid));
      z1 = toIntFloor(ZZB(info._pos.Z() * InvOperItemGrid));
    }
    else
    {
      x1 = toIntFloor(XX(node->_field.GetX()));
      z1 = toIntFloor(ZZ(node->_field.GetZ()));
    }
    if (node->_parent->_field.type == NTHouse)
    {
      // leaving house
      const IPaths *paths = map.GetHouse(node->_parent->_field.GetHouseIndex());
      if (!paths) continue;
      Vector3 pos = paths->GetPosition(node->_parent->_field.GetPosIndex());
      x2 = toIntFloor(XXL(pos.X() * InvOperItemGrid));
      z2 = toIntFloor(ZZB(pos.Z() * InvOperItemGrid));
    }
    else if (node->_parent->_field.type == NTRoad || node->_parent->_field.type == NTBridge)
    {
      const OperRoad &road = map._roads[node->_parent->_field.GetRoadIndex()];
      const OperRoadNode &info = road._nodes[node->_parent->_field.GetNodeIndex()];
      x2 = toIntFloor(XXL(info._pos.X() * InvOperItemGrid));
      z2 = toIntFloor(ZZB(info._pos.Z() * InvOperItemGrid));
    }
    else
    {
      x2 = toIntFloor(XX(node->_parent->_field.GetX()));
      z2 = toIntFloor(ZZ(node->_parent->_field.GetZ()));
    }

    SelectObject(dc, open ? openPen : closedPen);
    MoveToEx(dc, x1+xoff, z1+zoff, NULL);
    LineTo(dc, x2+xoff, z2+zoff);
    
    if (extInfo && itemSize>=30)
    {
      { // draw a number showing the path cost incl. heuristics
        BString<16> num;
        sprintf(num,"%.2f",node->_f);
        TextOut(dc,x1,z1,num,strlen(num));
      }
      { // draw a number showing the clean path cost
        BString<16> num;
        sprintf(num,"%.2f",node->_g);
        TextOut(dc,x1,z1+8,num,strlen(num));
      }
    }

  }

  // path
  const ASONode *last = NULL;
  if (map.IsDone() && map.IsFound())
  {
    // path found
    last = alg->GetLastNode();
  }
  else
  {
    // path not found - try to select alternate goal
    last = alg->GetBestNode(&map);
  }

  if (last && last->_parent)
  {
    SelectObject(dc, pathPen);
    int x1, z1;
    if (last->_field.type == NTHouse)
    {
      // leaving house
      const IPaths *paths = map.GetHouse(last->_field.GetHouseIndex());
      if (paths)
      {
        Vector3 pos = paths->GetPosition(last->_field.GetPosIndex());
        x1 = toIntFloor(XXL(pos.X() * InvOperItemGrid));
        z1 = toIntFloor(ZZB(pos.Z() * InvOperItemGrid));
      }
      else
      {
        x1 = 0; z1 = 0;
      }
    }
    else if (last->_field.type == NTRoad || last->_field.type == NTBridge)
    {
      const OperRoad &road = map._roads[last->_field.GetRoadIndex()];
      const OperRoadNode &info = road._nodes[last->_field.GetNodeIndex()];
      x1 = toIntFloor(XXL(info._pos.X() * InvOperItemGrid));
      z1 = toIntFloor(ZZB(info._pos.Z() * InvOperItemGrid));
    }
    else
    {
      x1 = toIntFloor(XX(last->_field.GetX()));
      z1 = toIntFloor(ZZ(last->_field.GetZ()));
    }
    while (last->_parent)
    {
      last = last->_parent;
      int x2, z2;
      if (last->_field.type == NTHouse)
      {
        // leaving house
        const IPaths *paths = map.GetHouse(last->_field.GetHouseIndex());
        if (paths)
        {
          Vector3 pos = paths->GetPosition(last->_field.GetPosIndex());
          x2 = toIntFloor(XXL(pos.X() * InvOperItemGrid));
          z2 = toIntFloor(ZZB(pos.Z() * InvOperItemGrid));
        }
        else
        {
          x2 = 0; z2 = 0;
        }
      }
      else if (last->_field.type == NTRoad || last->_field.type == NTBridge)
      {
        const OperRoad &road = map._roads[last->_field.GetRoadIndex()];
        const OperRoadNode &info = road._nodes[last->_field.GetNodeIndex()];
        x2 = toIntFloor(XXL(info._pos.X() * InvOperItemGrid));
        z2 = toIntFloor(ZZB(info._pos.Z() * InvOperItemGrid));
      }
      else
      {
        x2 = toIntFloor(XX(last->_field.GetX()));
        z2 = toIntFloor(ZZ(last->_field.GetZ()));
      }
      MoveToEx(dc, x1+xoff, z1+zoff, NULL);
      LineTo(dc, x2+xoff, z2+zoff);
      x1 = x2; z1 = z2;
    }
  }

  if (extInfo)
  {
    // cover points
    // different candidates may be present in each pass
    
    //HPEN pen = CreatePen(PS_SOLID,0, RGB(0,255,255));
    int nCover = alg->GetCoverFoundCount();
    AutoArray<CoverDiagInfo> coverDiagInfo;
    
    if (!is2ndPass)
    {
      CoverInfo bestCoverInfo;
      alg->GetBestCover(map,bestCoverInfo,_unit,&coverDiagInfo);
    }
        
    
    MapStringToClass<CoverTextCount, AutoArray<CoverTextCount> > textCounts;
    

    HGDIOBJ pen = GetStockObject(BLACK_PEN);
    HGDIOBJ pen2 = CreatePen(PS_DOT, 0, RGB(0, 0, 255));
    HGDIOBJ pen3 = CreatePen(PS_DOT, 0, RGB(255, 0, 255));
    HBRUSH brush = CreateSolidBrush(RGB(255, 0, 255));
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    for (int i=0; i<nCover; i++)
    {
      const OperCover &ci = alg->GetCoverFound(i);
      
      int xx = toIntFloor(ci._pos.X()*InvOperItemGrid);
      int zz = toIntFloor(ci._pos.Z()*InvOperItemGrid);
      
      
      CoverTextCount *count = &textCounts.Set(CoverTextCount(xx,zz));
      if (textCounts.IsNull(*count))
      {
        textCounts.Add(CoverTextCount(xx,zz,1));
        count = &textCounts.Set(CoverTextCount(xx,zz));
      }
      

      int left = XXL(xx);
      int bottom = ZZB(zz);
      int right = left + itemSize;
      int top = bottom - itemSize;
      Ellipse(dc, left + 1 + xoff, top + 1 + zoff, right - 1 + xoff, bottom - 1 + zoff);
      if (extInfo && itemSize>1)
      {
        // draw a short line pointing outwards from the cover
        Vector3 cOut = ci._pos + Matrix3(MRotationY,-ci._heading).Direction()*OperItemGrid*0.667f;
        float cOutX0 = ci._pos.X()*InvOperItemGrid;
        float cOutZ0 = ci._pos.Z()*InvOperItemGrid;
        float cOutX1 = cOut.X()*InvOperItemGrid;
        float cOutZ1 = cOut.Z()*InvOperItemGrid;
        int xc = toInt(XXL(cOutX0));
        int zc = toInt(ZZB(cOutZ0));
        int xo = toInt(XXL(cOutX1));
        int zo = toInt(ZZB(cOutZ1));
        MoveToEx(dc, xc+xoff, zc+zoff, NULL);
        LineTo(dc, xo+xoff, zo+zoff);
        // draw a number showing the rejection status
        if (coverDiagInfo.Size()>0 && itemSize>10)
        {
          const AStarNode<ASOField> *cNode = coverDiagInfo[i].node;
          if (!DetectFreedMemory(cNode))
          {
            // connect a line into the corresponding node
            Vector3 nodePos = map.GetNodePos(cNode);
            int xn = toIntFloor(XX((nodePos.X()-OperItemGrid*0.5f)*InvOperItemGrid));
            int zn = toIntFloor(ZZ((nodePos.Z()-OperItemGrid*0.5f)*InvOperItemGrid));
            OperMapIndex fx,fz;
            cNode->_field.GetXZ(map,fx,fz);
            int xnn = toIntFloor(XX(fx));
            int znn = toIntFloor(ZZ(fz));
            
            SelectObject(dc, pen3);
            MoveToEx(dc, xc+xoff, zc+zoff, NULL);
            LineTo(dc, xnn+xoff, znn+zoff);
          
            SelectObject(dc, pen2);
            MoveToEx(dc, xn+xoff, zn+zoff, NULL);
            LineTo(dc, xnn+xoff, znn+zoff);
            SelectObject(dc, pen);
          
            BString<16> num;
            if (coverDiagInfo[i].extValue>=1e8)
            {
              sprintf(num,"%s:MAX",cc_cast(FindEnumName(coverDiagInfo[i].status)));
            }
            else
            {
              sprintf(num,"%s:%.2f",cc_cast(FindEnumName(coverDiagInfo[i].status)),coverDiagInfo[i].extValue);
            }
            TextOut(dc,right,top+8*count->textCount++,num,strlen(num));
            
            if (itemSize>=30 && coverDiagInfo[i].extValue2>0)
            {
              sprintf(num,"  %.2f",coverDiagInfo[i].extValue2);
              TextOut(dc,right,top+8*count->textCount++,num,strlen(num));
            }
          }
        }
      }
      
    }
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
    DeleteObject(pen2);
    DeleteObject(pen3);
    DeleteObject(brush);
  
  }


  if (extInfo)
  {
    // start & end point
    HANDLE oldPen = SelectObject(dc, GetStockObject(WHITE_PEN));
    HBRUSH sbrush = CreateSolidBrush(RGB(255, 0, 255));
    HBRUSH ebrush = CreateSolidBrush(RGB(0, 255, 255));
    HANDLE oldBrush = SelectObject(dc, sbrush);
    {
      OperMapIndex x, z;
      alg->GetStart().GetXZ(map, x, z);
      int left = XXL(x) + itemSize/4;
      int bottom = ZZB(z) - itemSize/4;
      int right = left + itemSize/2;
      int top = bottom - itemSize/2;
      Ellipse(dc, left + 1, top + 1, right - 1, bottom - 1);
    }
    SelectObject(dc, ebrush);
    {
      OperMapIndex x, z;
      alg->GetDestination().GetXZ(map, x, z);
      int left = XXL(x) + itemSize/4;
      int bottom = ZZB(z) - itemSize/4;
      int right = left + itemSize/2;
      int top = bottom - itemSize/2;
      Ellipse(dc, left + 1, top + 1, right - 1, bottom - 1);
    }
    SelectObject(dc, oldBrush);
    DeleteObject(ebrush);
    DeleteObject(sbrush);
    SelectObject(dc, oldPen);
  
  }
}

void DebugWindowOperMap::OnScroll(int bar, int request, int pos)
{
  int itemSize = GetItemSize();

  HWND hwnd = GetWindowHandle(this);

  SCROLLINFO info;
  info.cbSize = sizeof(info); 
  info.fMask = SIF_POS;
  GetScrollInfo(hwnd, bar, &info);

  int oldPos = info.nPos;
  int newPos = oldPos;
  switch (request) 
  { 
  case SB_PAGEUP: 
    newPos -= OperItemRange * itemSize; 
    break; 
  case SB_PAGEDOWN: 
    newPos += OperItemRange * itemSize; 
    break; 
  case SB_LINEUP: 
    newPos -= itemSize; 
    break; 
  case SB_LINEDOWN: 
    newPos += itemSize; 
    break; 
  case SB_THUMBPOSITION: 
    newPos = pos; 
    break; 
  } 

  info.fMask = SIF_RANGE;
  GetScrollInfo(hwnd, bar, &info);
  saturate(newPos, info.nMin, info.nMax);

  // set the position
  info.fMask = SIF_POS;
  info.nPos = newPos;
  SetScrollInfo(hwnd, bar, &info, TRUE);

  // update the position
  GetScrollInfo(hwnd, bar, &info);
  if (info.nPos != oldPos) Update();
}

static Link<DebugWindowOperMap> GOperMapDebug;
static bool DebugAutoHide;

void ProcessMessagesNoWait();

void WaitForClose(DebugWindow *win)
{
  Link<DebugWindow> link = win;
  while (link)
  {
    ProcessMessagesNoWait();
    Sleep(100);
  }
}

void DebugOperMap()
{
  if (!GOperMapDebug)
  {
    GOperMapDebug = new DebugWindowOperMap(false);
    DebugAutoHide = false;
  }
  GOperMapDebug->UnassignUnit();
}

void DebugOperMapCover()
{
  if (!GOperMapDebug)
  {
    GOperMapDebug = new DebugWindowOperMap(true);
    DebugAutoHide = false;
  }
  GOperMapDebug->UnassignUnit();
}

void DebugOperMap(AIBrain *unit)
{
  if (!GOperMapDebug)
  {
    GOperMapDebug = new DebugWindowOperMap(false);
    DebugAutoHide = false;
  }
  GOperMapDebug->AssignUnit(unit);
}

static bool showTrouble=false;
void DebugOperMapTrouble()
{
  showTrouble = !showTrouble;
  GlobalShowMessage(500,"Show path troubles %s",showTrouble ? "On" : "Off");
}

#endif

unsigned int GetKeyOperMapField(const OperMapField &field)
{
  return field._operField->X() | (field._operField->Z() << 16);
}

template <> OperMapField ImplicitMapTraits<OperMapField>::null(NULL);
template <> OperMapField ImplicitMapTraits<OperMapField>::zombie((OperField *)1);

Vector3 OperMap::GetNodePos(const void *vNode) const
{
  float above = 0.3f;
  const ASONode *node = (const ASONode *)vNode;
  switch (node->_field.type.GetEnumValue())
  {
  case NTHouse:
    {
      const IPaths *paths = GetHouse(node->_field.GetHouseIndex());
      Assert(paths);
      if (!paths)
      {
        Fail("House not found");
        return VZero;
      }
      return paths->GetPosition(node->_field.GetPosIndex());
    }
  case NTRoad:
  case NTBridge:
    {
      const OperRoad &road = _roads[node->_field.GetRoadIndex()];
      const OperRoadNode &info = road._nodes[node->_field.GetNodeIndex()];
      return info._pos+Vector3(0,above,0);
    }
  default: // case NTGrid
    {
      float operItemGrid = OperItemGrid;
      float x = node->_field.GetX()*operItemGrid+operItemGrid*0.5f;
      float z = node->_field.GetZ()*operItemGrid+operItemGrid*0.5f;
      float y = GLandscape->SurfaceY(x,z)+above;
      return Vector3(x,y,z);
    }
  }
}

void OperMap::Path3DDiags(AIBrain *unit) const
{
#if _ENABLE_CHEATS
  // display only finished results, so that we do not have to care about MT safety
  if (_algorithm && _algorithm->IsDone())
  {
    static const PackedColor colorClosed(Color(0, 0, 0.7f, 1));
    static const PackedColor colorOpen(Color(0, 0, 0.35f, 1));
    static const PackedColor colorPath(Color(0.5, 0.5, 0, 1));
    // closed / opened nodes
    const ASOClosedList &list = _algorithm->GetClosedList();
    for (ASOClosedList::Iterator iterator(list); iterator; ++iterator)
    {
      const ASONode *node = *iterator;
      if (!node->_parent) continue;

      bool open = node->_open < 0;

      Vector3 end = GetNodePos(node);
      RString details;
      if (node->_field.type==NTGrid)
      {
        OperMapIndex x = node->_field.GetX(), z = node->_field.GetZ();
        OperItem item = CheckOperItem(x,z);
        Clearance clearance = item.GetClearance();
        if (clearance<ClearanceNormal)
        {
          OperItemType type = unit->IsSoldier() ? item.GetTypeSoldier() : item.GetType();
          details = Format("%s, %d, %d,%d",cc_cast(FindEnumName(clearance)),type,x,z);
        }
      }

      GScene->DrawDiagText(Format("f=%.1f, g=%.1f %s",node->_f,node->_g,cc_cast(details)),end+VUp*0.2,25,0.71,HWhite,20.0f);
      Vector3 beg = GetNodePos(node->_parent);

      GEngine->DrawVerticalArrow(beg,end,open ? colorOpen : colorClosed,3, 0, 0.7f);

      Vector3 dirN = Matrix3(MRotationY,node->_field.dir*-H_PI/128-H_PI).Direction();
      
      GEngine->DrawVerticalArrow(end,end+dirN*0.30,PackedBlack,3, 0, 0.2f);
    }

    // path
    const ASONode *last = NULL;
    if (IsDone() && IsFound())
    {
      // path found
      last = _algorithm->GetLastNode();
    }
    else
    {
      // path not found - try to select alternate goal
      last = _algorithm->GetBestNode(this);
    }

    if (last && last->_parent)
    {
      Vector3 pos = GetNodePos(last);
      while (last->_parent)
      {
        last = last->_parent;
        Vector3 next = GetNodePos(last);
        GEngine->DrawVerticalArrow(next,pos,colorPath, 3, 0, 0.5f);
        pos = next;
      }
    }
  }

  // we might display start & end point as well
#endif
}


// class OperMap
OperMap::OperMap(bool connectHouses)
: _isSoldier(false), _vehType(NULL), _mode(CMSafe), _connectHouses(connectHouses)
#if USE_OPERPATH_THREAD
  ,_failureX(-1), _failureZ(-1)
#endif
{
  _alternateGoal = false;
  _endOnRoadOnly = false;
  _replanLater = false;
#if USE_OPERPATH_THREAD
  _failure = false;
#endif
#if _ENABLE_REPORT
  _isCameraVehicle = false;
#endif
}

OperMap::~OperMap()
{
  Clear();
}

bool OperMap::IsHouseExit(OperMapIndex x, OperMapIndex z)
{
  if (x < 0 || x >= OperItemRange * LandRange || z < 0 || z >= OperItemRange * LandRange)
  {
    // out of map
    return false;
  }

  // search for field in the map
  SPLIT_INDICES(x, z, x0, z0, xx, zz);

  const OperMapField *field = GetField(x0, z0);

  if (!field)
  {
    // field not found - we have to wait for it or create it
#if USE_OPERPATH_THREAD
    // in the multicore environment, only signalize failure and end
    if (IsJobRunning())
    {
      AskForField(x0, z0);
      return false;
    }
#endif
    int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER | MASK_FOR_PATHING;
    field = &CreateField(x0, z0, mask, _vehType, _mode, _isSoldier);
  }

  return field->_operField->GetItem(xx, zz).IsHouseExitPresent();
}

static inline void AppendConnections(OperHouse &info1, OperHouse &info2, int index1, int i, int index2, int j)
{
  // found, connect them
  OperHouseConnection &conn1 = info1._connections.Append();
  conn1._posFrom = i;
  conn1._houseTo = index2;
  conn1._posTo = j;

  OperHouseConnection &conn2 = info2._connections.Append();
  conn2._posFrom = j;
  conn2._houseTo = index1;
  conn2._posTo = i;
}

void OperMap::ConnectHouses(int index1, int index2)
{
  OperHouse *info1 = &_houses[index1];
  OperHouse *info2 = &_houses[index2];

  // how near positions to connect
  const float connLimit = OperItemGrid;

  Object *obj1 = info1->_house;
  Object *obj2 = info2->_house;
  if (!obj1 || !obj2) return;

  // check the distance of objects (avoid searching for connections where none can exist)
  float dist2 = obj1->FutureVisualState().Position().Distance2(obj2->FutureVisualState().Position());
  float limit = obj1->GetShape()->BoundingSphere() + obj2->GetShape()->BoundingSphere() + connLimit;
  if (dist2 > Square(limit)) return;

  const IPaths *house1 = obj1->GetIPaths();
  const IPaths *house2 = obj2->GetIPaths();
  if (!house1 || !house2) return;
  // empty path should be detected earlier, but it cost litter here and makes the function safer
  if (house1->NPoints()<=0 || house2->NPoints()<=0)
    return;
  // test each pair of points
  
  // check if house1 or house2 has some significant animation
  // unless both are animated, rearrange so that inner loop is on non-animated one
  if (!obj2->IsAnimationIdentity(obj2->GetShape()->FindPaths()))
  {
    Swap(info1,info2);
    Swap(house1,house2);
    Swap(index1,index2);
  }

  if (
    obj1->GetShape()==obj2->GetShape() && dist2<Square(0.5f)
    && obj1->FutureVisualState().Transform().Distance2(obj2->FutureVisualState().Transform())<Square(0.5f)
  )
  { // hot-fix a data bug: two identical buildings on the same position
    // most often happens by a wrong ruin spawning

  }
  else if (obj2->IsAnimationIdentity(obj2->GetShape()->FindPaths()))
  {
    #if _DEBUG || _PROFILE
      if (!obj1->IsAnimationIdentity(obj1->GetShape()->FindPaths()))
      {
        __asm nop;
      }
    #endif
    // optimized solution, inner loop working in house2 model space
    Matrix4 toHouse2 = obj2->GetFrameBase().GetInvTransform();
    AutoArray<Vector3,MemAllocLocal<Vector3,256,AllocAlign16> > house2Static;
    house2Static.Resize(house2->NPoints());
    for (int j=0; j<house2->NPoints(); j++)
    {
      house2Static[j] = house2->GetModelStaticPosition(j);
    }

    for (int i=0; i<house1->NPoints(); i++)
    {
      Vector3 pos1 = toHouse2*house1->GetPosition(i);

      for (int j=0; j<house2->NPoints(); j++)
      {
        Vector3 pos2 = house2Static[j];
        if (pos1.Distance2(pos2) <= Square(connLimit))
          AppendConnections(*info1,*info2,index1,i,index2,j);
      }
    }
  }
  else
  {
    // generic solution, both models animated: test each pair of points in world space
    // to prevent animation being run multiple times, animate the inner model only once
    AutoArray<Vector3,MemAllocLocal<Vector3,256,AllocAlign16> > house2Anim;
    house2Anim.Resize(house2->NPoints());
    for (int j=0; j<house2->NPoints(); j++)
    {
      house2Anim[j] = house2->GetPosition(j);
    }

    for (int i=0; i<house1->NPoints(); i++)
    {
      Vector3 pos1 = house1->GetPosition(i);
      for (int j=0; j<house2Anim.Size(); j++)
      {
        Vector3 pos2 = house2Anim[j];
        if (pos1.Distance2(pos2) <= Square(connLimit))
        {
          AppendConnections(*info1,*info2,index1,i,index2,j);
      }
    }
  }
}
}

const OperMapField *OperMap::GetField(LandIndex x, LandIndex z) const
{
  unsigned int key = x | (z << 16);
  return _fields.get(key);
/*
  for (int i=0; i<_fields.Size(); i++)
  {
    const OperMapField &field = _fields[i];
    if (field._operField->IsField(x, z)) return &field;
  }
  return NULL;
*/
}

bool OperMap::IsLocked(OperMapIndex x, OperMapIndex z, const OperMapField *field, OperFieldIndex xx, OperFieldIndex zz, bool soldier) const
{
  if (!field->_lockField) return false;
  int locks = field->_lockField->IsLocked(xx, zz, soldier);
  if (locks == 0) return false; // nobody locked it

  // check if locked only by myself
  if (!_locker) return true;
  if ((locks & LockMaskAll) > 0x10 || (locks & LockMaskVeh) > 1) return true; // more than 1 unit is locking
  
  int myLocks = _locker->IsLocking(x, z, soldier);
  return myLocks != locks;
}

bool OperMap::IsHouseLocked(int houseIndex, int pt) const
{
  Object *house = _houses[houseIndex]._house;
  if (!house) return true; // if house is not found, do not try to access it

  const IPaths *paths = house->GetIPaths();

  // in houses, not all points are locking, only the positions
  int pos = -1;
  for (int i=0; i<paths->NPos(); i++)
  {
    if (paths->GetPos(i) == pt)
    {
      pos = i;
      break;
    }
  }
  if (pos < 0) return false; // not a position

  int locks = paths->IsLocked(pos);
  if (locks == 0) return false; // nobody locked it

  // check if locked only by myself
  if (!_locker) return true;
  if (locks > 1) return true; // more than 1 unit is locking

  int myLocks = _locker->IsLocking(house, pos);
  return myLocks != locks;
}

bool OperMap::IsRoadLocked(const RoadLink *road, bool soldier) const
{
  int locks = road->IsLocked(soldier);
  if (locks == 0) return false; // nobody locked it

  // check if locked only by myself
  if (!_locker) return true;
  if ((locks & LockMaskAll) > 0x10 || (locks & LockMaskVeh) > 1) return true; // more than 1 unit is locking

  int myLocks = _locker->IsLocking(road, soldier);
  return myLocks != locks;
}

#define FC(x,z) GetFieldCost(x, z, locks, vehType, mode, soldier, false)

bool OperMap::GetFieldLocked(OperMapIndex x, OperMapIndex z, const EntityAIType *type, CombatMode mode, bool soldier)
{
SPLIT_INDICES(x, z, x0, z0, xx, zz);

  const OperMapField *field = GetField(x0, z0);
  if (!field)
  {
    // field not found - we have to wait for it or create it
#if USE_OPERPATH_THREAD
    // in the multicore environment, only signalize failure and end
    if (IsJobRunning())
    {
      AskForField(x0, z0);
      return false;
    }
#endif
    int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER | MASK_FOR_PATHING;
    field = &CreateField(x0, z0, mask, type, mode, soldier);
  }

  if (IsLocked(x, z, field, xx, zz, soldier))
  {
    return true;
  }

  return false;
}

OperItem OperMap::CheckOperItem(OperMapIndex x, OperMapIndex z) const
{

  SPLIT_INDICES(x, z, x0, z0, xx, zz);

  const OperMapField *field = GetField(x0, z0);
  if (!field)
  {
    OperItem item = {};
    return item;
  }

  return field->_operField->GetItem(xx, zz);
}

Clearance OperMap::GetFieldClearance(OperMapIndex x, OperMapIndex z, const EntityAIType *type, CombatMode mode, bool soldier)
{
  if (!soldier) return ClearanceNormal;

  SPLIT_INDICES(x, z, x0, z0, xx, zz);

  const OperMapField *field = GetField(x0, z0);
  if (!field)
  {
    // field not found - we have to wait for it or create it
#if USE_OPERPATH_THREAD
    // in the multicore environment, only signalize failure and end
    if (IsJobRunning())
    {
      AskForField(x0, z0);
      return ClearanceNormal;
    }
#endif
    int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER | MASK_FOR_PATHING;
    field = &CreateField(x0, z0, mask, type, mode, soldier);
  }

  return field->_operField->GetItem(xx, zz).GetClearance();
}


FieldCost OperMap::GetFieldCost(OperMapIndex x, OperMapIndex z, bool locks, const EntityAIType *type, CombatMode mode, bool soldier, bool suppress, bool includeGradient)
{
  SPLIT_INDICES(x, z, x0, z0, xx, zz);

  const OperMapField *field = GetField(x0, z0);
  if (!field)
  {
    // field not found - we have to wait for it or create it
#if USE_OPERPATH_THREAD
    // in the multicore environment, only signalize failure and end
    if (IsJobRunning())
    {
      AskForField(x0, z0);
      return FieldCost(0, 0);
    }
#endif
    int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER | MASK_FOR_PATHING;
    field = &CreateField(x0, z0, mask, type, mode, soldier);
  }

  if (locks && IsLocked(x, z, field, xx, zz, soldier))
  {
    return FieldCost(SET_UNACCESSIBLE, SET_UNACCESSIBLE);
  }

  // no need to call UpdateField (already processed in OperMap::ProcessFindPath)
  OperItem item = field->_operField->GetItem(xx, zz);
  OperItemType otype = soldier ? item.GetTypeSoldier() : item.GetType();
  Clearance clearance = item.GetClearance();
  // avoid returning infinity
  float baseCost = includeGradient ? field->_baseCost : field->_baseCostWithoutGradient;
  FieldCost cost = type->GetTypeCost(otype,mode,clearance) * baseCost;

  // handle suppress cost
  if (suppress && field->_suppressField) cost.cost += field->_suppressField->GetCost(xx, zz, _suppressTargets);

  if (cost.cost>SET_UNACCESSIBLE) cost.cost = SET_UNACCESSIBLE;
  if (cost.time>SET_UNACCESSIBLE) cost.time = SET_UNACCESSIBLE;
  return cost;
}

/**
@param xRange advisory - what map size is expected to be created (used to pre-size internal structures)
@param zRange same as xRange
*/
void OperMap::Clear(int xRange, int zRange)
{
#if USE_OPERPATH_THREAD
  GJobManager.CancelJob(this);
  _failure = false;
#endif

  {
#if 0
    extern int AllowBPMemLock;
    extern void StartBPScope();
    struct ScopeBP: private NoCopy
    {
      ScopeBP(){StartBPScope();AllowBPMemLock++;}
      ~ScopeBP(){AllowBPMemLock--;}
    } allowBPInScope;
#endif

    PROFILE_SCOPE_EX(pthMD, aiPath);
    _algorithm = NULL;
  }

  // _fields.Clear();
  _fields.reset();
  _fields.rebuild(xRange*zRange);
  _houses.Clear();
  _suppressTargets.Clear(); 
  _roads.Clear();
  _locker = NULL;
}

void OperMap::ClearMap()
{
#if _ENABLE_CHEATS
  // do not destroy map if debugging info can be shown
  if (!GOperMapDebug)
#endif
  if (!CHECK_DIAG(DEPathFind))
    Clear();
}

#if USE_OPERPATH_THREAD
void OperMap::AskForField(LandIndex x, LandIndex z)
{
  if (_jobCommand < JCSuspend)
  {
    _failureX = x;
    _failureZ = z;
    _failure = true;
    GJobManager.SuspendJob(this);
  }
}
#endif

const OperMapField &OperMap::CreateField(LandIndex x, LandIndex z, int mask, const EntityAIType *vehType, CombatMode mode, bool soldier)
{
  // lookup map
/*
  int n = _fields.Size();
  if (n>50*50)
  {
    LogF("Very large OperMap");
  }
*/
  Assert(!GetField(x, z));

  OperField *fld = NULL;
  // outside the map no objects are present - we can use the OperFieldSimple
  if (!InRange(z, x))
  {
    OperItem item;
    item._content = 0;
    item.SetType(OITNormal);
    item.SetTypeSoldier(OITNormal);
    item.SetHouseExitPresent(false);
    item.SetClearance(ClearanceNormal);
#if STORE_OBJ_ID
    item._objSoldier = -1;
#endif
    const float maxWater = -0.5f + GLandscape->GetSeaLevel();
    // deep water should be marked as OITWater
    if (mask & MASK_USE_BUFFER)
    {
      // check only center of the square
      float xt = (x + 0.5) * LandGrid;
      float zt = (z + 0.5) * LandGrid;
      float yt = GLandscape->SurfaceY(xt, zt);
      if (yt < maxWater)
      {
        item.SetTypeSoldier(OITWater);
        item.SetType(OITWater);
      }
    }
    fld = new OperFieldSimple(x, z, item);
  }
  else if (mask & MASK_USE_BUFFER)
  {
    #if _ENABLE_AI
    // using buffering implies standard flags are used
    Assert((mask&(MASK_AVOID_OBJECTS | MASK_PREFER_ROADS))==(MASK_AVOID_OBJECTS | MASK_PREFER_ROADS));
    fld = GLOB_LAND->OperationalCache()->GetOperField(x, z, mask&~MASK_FOR_PATHING);
    // when needed for pathing, make it ready
    if (mask&MASK_FOR_PATHING)
    {
      fld->PrepareForPathing();
    }
    #else
    Fail("OperField Cache used.");
    #endif
  }
  else
  {
    fld = new OperFieldFull(x, z, mask);
    // pathing is expected to be always used on cached fields only
    Assert((mask&MASK_FOR_PATHING)==0);
  }

  if (_connectHouses)
  {
    PROFILE_SCOPE_DETAIL_EX(pthCH, aiPath);
    Assert(fld->ReadyForPathing());
    for (int i=0; i<fld->_houses.Size(); i++)
    {
      int index = _houses.AddUnique(fld->_houses[i]);
      if (index >= 0)
      {
        // added, connect with other houses
        for (int i=0; i<index; i++) ConnectHouses(i, index);
      }
    }
  }

  if (mask & MASK_FOR_PATHING)
  {
    PROFILE_SCOPE_DETAIL_EX(pthCR, aiPath);
    float vehWidth = 0;
    if (!soldier)
    {
      LODShape *shape = vehType->GetShape();
      if (shape) vehWidth = shape->Max().X() - shape->Min().X();
    }

    // add road nodes which can be in this field
    // check also neighbor fields to find all nodes
    for (LandIndex zz(z-1); zz<=z+1; zz++)
      for (LandIndex xx(x-1); xx<=x+1; xx++)
      {
        if (!InRange(xx, zz)) continue;
        const RoadList &roadList = GLandscape->GetRoadNet()->GetRoadList(xx, zz);
        for (int i=0; i<roadList.Size(); i++)
        {
          const RoadLink *road = roadList[i];
          if (_roads.FindKey(road) >= 0) continue; // already present

          int nNodes = road->NNodes(soldier);

          OperRoad &info = _roads.Append();
          info._road = road;
          info._bridge = road->IsBridge();
          // for each road segment, add all nodes
          info._nodes.Realloc(nNodes);
          info._nodes.Resize(nNodes);
          for (int j=0; j<nNodes; j++)
          {
            RoadNode node = road->GetNode(j);
            // fill all needed data
            OperRoadNode &infoNode = info._nodes[j];
            infoNode._pos = node.Position(soldier, vehWidth);
            infoNode._x = OperMapIndex(toIntFloor(infoNode._pos.X() * InvOperItemGrid));
            infoNode._z = OperMapIndex(toIntFloor(infoNode._pos.Z() * InvOperItemGrid));
          }
        }
      }
  }

//  OperMapField &field = _fields.Append();
  OperMapField field(fld);
//  field._operField = fld;
  field._suppressField = GLandscape->SuppressCache()->GetField(x, z, MASK_NO_CREATE);
  field._lockField = GLandscape->LockingCache()->GetField(x, z, false);

  if (vehType)
  {
    GeographyInfo geogr = GLandscape->GetGeography(x, z);
    field._baseCost = OperItemGrid * vehType->GetBaseCost(geogr, true, true);
    field._baseCostWithoutGradient = OperItemGrid * vehType->GetBaseCost(geogr, true, false);
    field._heurCost = field._baseCost * vehType->GetFieldCost(geogr, mode);
  }
  else
  {
    field._baseCost = 0;
    field._baseCostWithoutGradient = 0;
    field._heurCost = 0;
  }
  return *_fields.putAndGet(field);
// return field;
}

#include "../scene.hpp"

bool OperMap::FindNearestEmpty(
  OperMapIndex &x, OperMapIndex &z,  float xf, float zf,
  OperMapIndex xbase, OperMapIndex zbase, OperMapIndex xsize, OperMapIndex zsize,
  bool locks, const EntityAIType *vehType, bool soldier, bool insideBuilding,
  FindNearestEmptyCallback *isFree, void *context
)
{
  // x, z, xbase, zbase are in OperItemGrid units
  // xf,zf have the same meaning as x,z, but are with sub-square precision
  int xrest = x - xbase;
  int zrest = z - zbase;
  int xt, zt;
  int t;
  bool inner;
  int nMax = xrest;
  if (zrest > nMax) nMax = zrest;
  t = xsize - 1 - xrest;
  if (t > nMax) nMax = t;
  t = zsize - 1 - zrest;
  if (t > nMax) nMax = t;

  Assert( nMax<160 );
  if( nMax>=160 ) return false;

  // unaccessible does not depend on combat mode - we are not interested in exact cost
  CombatMode mode = CMSafe;
  const float operItemGrid = OperItemGrid;

  // for soldier in the building, check position inside building
  float nearestDist2=1e10;
  #if 1
  if (soldier && insideBuilding)
  {
    float posX = xf*operItemGrid;
    float posZ = zf*operItemGrid;
    Vector3 pos = GLandscape->PointOnSurface(posX,0,posZ);
    // convert to LandGrid units
    LAND_INDICES(x, z, xLand, zLand);
    for (LandIndex xx(xLand-1); xx<=xLand+1; xx++)
    for (LandIndex zz(zLand-1); zz<=zLand+1; zz++)
    {
      if (!InRange(xx, zz)) continue;
      MAKE_OBJECT_LIST_XXL(list, GLandscape, xx, zz)
      for (int o=0; o<list.Size(); o++)
      {
        const Object *obj = list[o];
        const IPaths *paths = obj->GetIPaths();
        if (!paths) continue;
        Vector3 retPos;
        if (paths->FindNearestPoint(pos, retPos) < 0) continue;
        float dist2 = retPos.Distance2(pos);
        if (nearestDist2>dist2)
        {
          nearestDist2 = dist2;
          x = OperMapIndex(toIntFloor(retPos.X() * InvOperItemGrid));
          z = OperMapIndex(toIntFloor(retPos.Z() * InvOperItemGrid));
        }
      }
    }
    // limit nMax with position already found in house
    if (nearestDist2<1e9)
    {
      float maxDist = sqrt(nearestDist2)*H_SQRT2;
      int maxDistGrid = toIntCeil(maxDist*InvOperItemGrid);
      saturateMin(nMax,maxDistGrid);

#if _ENABLE_CHEATS
      if (CHECK_DIAG(DECostMap))
      {
        float xPos = operItemGrid*x;
        float zPos = operItemGrid*z;
        float yPos = GLandscape->SurfaceYAboveWater(xPos,zPos);
        Vector3 dPos(xPos,yPos,zPos);

        Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),VISITOR_NO_ID);
        obj->SetPosition(dPos);
        obj->SetScale(0.5);
        obj->SetConstantColor(PackedColor(Color(1,1,1)));
        GScene->ShowObject(obj);
      }
#endif
    }
  }
  #endif

  for (int n=1; n<nMax; n++)
  {
    #define IS_FREE(xg, zg) \
      isFree( Vector3( (xg+0.5f)*operItemGrid, 0, (zg+0.5f)*operItemGrid ),context )

    #define CHECK_EMPTY \
      float dist2 = Square(xbase+xt-xf)+Square(zbase+zt-zf); \
      if (dist2<nearestDist2) \
      { \
        nearestDist2 = dist2; \
        x = OperMapIndex(xbase + xt); \
        z = OperMapIndex(zbase + zt); \
        saturateMin(nMax,i+1); \
      }
      // if something found, limit number of iterations

    for (int i=0; i<=n; i++)
    {
      inner = 0 < i && i < n;
      // left edge
      xt = xrest - n;
      if (xt >= 0)
      {
        // up
        zt = zrest - i;
        if
        (
          zt < zsize && FC(OperMapIndex(xbase + xt), OperMapIndex(zbase + zt)).cost < GET_UNACCESSIBLE &&
          IS_FREE(xbase + xt, zbase + zt) ||
          inner && (zt = zrest + i) >= 0 &&
          FC(OperMapIndex(xbase + xt), OperMapIndex(zbase + zt)).cost < GET_UNACCESSIBLE &&
          IS_FREE(xbase + xt, zbase + zt)
        )
        {
          CHECK_EMPTY
        }
      }
      // top edge
      zt = zrest - n;
      if (zt >= 0)
      {
        // right
        xt = xrest + i;
        if
        (
          xt < xsize && FC(OperMapIndex(xbase + xt), OperMapIndex(zbase + zt)).cost < GET_UNACCESSIBLE &&
          IS_FREE(xbase + xt, zbase + zt) ||
          inner && (xt = xrest - i) >= 0 &&
          FC(OperMapIndex(xbase + xt), OperMapIndex(zbase + zt)).cost < GET_UNACCESSIBLE &&
          IS_FREE(xbase + xt, zbase + zt)
        )
        {
          CHECK_EMPTY
        }
      }
      // right edge
      xt = xrest + n;
      if (xt < xsize)
      {
        // down
        zt = zrest + i;
        if
        (
          zt >= 0 && FC(OperMapIndex(xbase + xt), OperMapIndex(zbase + zt)).cost < GET_UNACCESSIBLE &&
          IS_FREE(xbase + xt, zbase + zt) ||
          inner && (zt = zrest - i) < zsize &&
          FC(OperMapIndex(xbase + xt), OperMapIndex(zbase + zt)).cost < GET_UNACCESSIBLE &&
          IS_FREE(xbase + xt, zbase + zt)
        )
        {
          CHECK_EMPTY
        }
      }
      // bottom edge
      zt = zrest + n;
      if (zt < zsize)
      {
        // left
        xt = xrest - i;
        if
        (
          xt >=0  && FC(OperMapIndex(xbase + xt), OperMapIndex(zbase + zt)).cost < GET_UNACCESSIBLE &&
          IS_FREE(xbase + xt, zbase + zt) ||
          inner && (xt = xrest + i) < xsize &&
          FC(OperMapIndex(xbase + xt), OperMapIndex(zbase + zt)).cost < GET_UNACCESSIBLE &&
          IS_FREE(xbase + xt, zbase + zt)
        ) 
        {
          CHECK_EMPTY
        }
      }
    }   
  }
  return nearestDist2<1e9;
}

bool OperMap::FindEmptyInRect(
  OperMapIndex &xBest, OperMapIndex &zBest, const FrameBase &pos, const Vector3 *minmax, Vector3Par nearTo,
  bool locks, const EntityAIType *vehType, bool soldier, bool insideBuilding,
  FindNearestEmptyCallback *isFree, void *context
)
{
  // x, z, xbase, zbase are in OperItemGrid units

  // unaccessible does not depend on combat mode - we are not interested in exact cost
  CombatMode mode = CMSafe;
  const float operItemGrid = OperItemGrid;

  // compute world-space min-max to know what range to traverse
  Vector3 minRect(+FLT_MAX,+FLT_MAX,+FLT_MAX),maxRect(-FLT_MAX,-FLT_MAX,-FLT_MAX);
  for (int x=0; x<2; x++) for (int z=0; z<2; z++)
  {
    Vector3 corner(minmax[x].X(),minmax[0].Y(),minmax[z].Z());
    Vector3 worldCorner = pos.FastTransform(corner);
    SaturateMin(minRect,worldCorner);
    SaturateMax(maxRect,worldCorner);
  }
  // now compute necessary OperItem grid range
  OperMapIndex xBeg(toIntFloor(minRect.X() * InvOperItemGrid)), xEnd(toIntFloor(maxRect.X() * InvOperItemGrid) + 1);
  OperMapIndex zBeg(toIntFloor(minRect.Z() * InvOperItemGrid)), zEnd(toIntFloor(maxRect.Z() * InvOperItemGrid) + 1);

  Matrix4 toPos = pos.GetInvTransform();
  // TODO: for soldier check also position inside building (paths)

  float nearestDist2=FLT_MAX;
  for (OperMapIndex z(zBeg); z<zEnd; z++) for (OperMapIndex x(xBeg); x<xEnd; x++)
  {
    // check if we are (at least partially) withing the required min-max box
    // i.e. check if interesection of OperItemGrid:x+0.5,z+0.5 is inside of the minmax box
    Vector3 xzCenter((x+0.5f)*operItemGrid, 0, (z+0.5f)*operItemGrid );
    Vector3 zxCenterPos = toPos.FastTransform(xzCenter);

    // we want the point to be well inside
    const float keepBorder = operItemGrid*0.5f;
    if (
      zxCenterPos.X()>=minmax[0].X()+keepBorder && zxCenterPos.X()<=minmax[1].X()-keepBorder &&
      zxCenterPos.Z()>=minmax[0].Z()+keepBorder && zxCenterPos.Z()<=minmax[1].Z()-keepBorder
    )
    {
      // check if the field is free
      if (FC(x,z).cost < GET_UNACCESSIBLE && isFree(xzCenter,context))
      {
        float dist2 = Square(x*operItemGrid-nearTo.X())+Square(z*operItemGrid-nearTo.Z());
        if (dist2<nearestDist2)
        {
          nearestDist2 = dist2;
          xBest = x, zBest = z;
        }
      }
    }
  }
  return nearestDist2<FLT_MAX;
}

bool OperMap::IsAreaFree(OperMapIndex x, OperMapIndex z, float radiusO)
{
  float radius2 = Square(radiusO);
  int radiusI = toIntFloor(radiusO);

#define CHECK_COST(x, z) \
  if (GetFieldCost(OperMapIndex(x), OperMapIndex(z), true, _vehType, _mode, _isSoldier, false).cost >= GET_UNACCESSIBLE) return false;

  // heuristic - try to quickly reject
  if (radiusI > 1)
  {
    // center
    CHECK_COST(x, z);
    // sides
    CHECK_COST(x - radiusI, z);
    CHECK_COST(x, z - radiusI);
    CHECK_COST(x + radiusI, z);
    CHECK_COST(x, z + radiusI);
  }

  // check all fields
  for (int zz=z-radiusI; zz<=z+radiusI; zz++)
  {
    float zz2 = Square(zz - z);
    for (int xx=x-radiusI; xx<=x+radiusI; xx++)
    {
      float xx2 = Square(xx - x);
      if (zz2 + xx2 > radius2) continue;
      CHECK_COST(xx, zz);
    }
  }

  return true;
}

bool OperMap::FindEmptyAreaReady(Vector3 &center, float radius, float maxDistance)
{
  // find the bounds
  float xMin = center.X() - maxDistance - radius;
  float xMax = center.X() + maxDistance + radius;
  float zMin = center.Z() - maxDistance - radius;
  float zMax = center.Z() + maxDistance + radius;
  // convert the coordinates
  OperMapIndex xMinO(toIntFloor(xMin * InvOperItemGrid));
  OperMapIndex xMaxO(toIntFloor(xMax * InvOperItemGrid));
  OperMapIndex zMinO(toIntFloor(zMin * InvOperItemGrid));
  OperMapIndex zMaxO(toIntFloor(zMax * InvOperItemGrid));
  LAND_INDICES(xMinO, zMinO, xMinL, zMinL);
  LAND_INDICES(xMaxO, zMaxO, xMaxL, zMaxL);

  return OperFieldRangeReady(xMinL, xMaxL, zMinL, zMaxL);
}

bool OperMap::FindEmptyArea(Vector3 &center, float radius, float maxDistance, const EntityAIType *vehType)
{
  PROFILE_SCOPE_EX(feaAl, aiPath);
  _vehType = vehType;
  _mode = CMSafe;
  _isSoldier = vehType->IsKindOf(GWorld->Preloaded(VTypeMan));
  int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER;

  // find the bounds
  float xMin = center.X() - maxDistance - radius;
  float xMax = center.X() + maxDistance + radius;
  float zMin = center.Z() - maxDistance - radius;
  float zMax = center.Z() + maxDistance + radius;
  // convert the coordinates
  OperMapIndex xMinO(toIntFloor(xMin * InvOperItemGrid));
  OperMapIndex xMaxO(toIntFloor(xMax * InvOperItemGrid));
  OperMapIndex zMinO(toIntFloor(zMin * InvOperItemGrid));
  OperMapIndex zMaxO(toIntFloor(zMax * InvOperItemGrid));
  LAND_INDICES(xMinO, zMinO, xMinL, zMinL);
  LAND_INDICES(xMaxO, zMaxO, xMaxL, zMaxL);

  // create the map
  {
    PROFILE_SCOPE_EX(feaMa, aiPath);
    for (LandIndex i(xMinL); i<=xMaxL; i++)
      for (LandIndex j(zMinL); j<=zMaxL; j++)
      {
        CreateField(i, j, mask, vehType, _mode, _isSoldier);
      }
  }

  PROFILE_SCOPE_EX(feaCh, aiPath);

  // check the center
  OperMapIndex xCenter(toIntFloor(center.X() * InvOperItemGrid));
  OperMapIndex zCenter(toIntFloor(center.Z() * InvOperItemGrid));
  float radiusO = radius * InvOperItemGrid;
  if (IsAreaFree(xCenter, zCenter, radiusO))
  {
    center = Vector3((xCenter + 0.5f) * OperItemGrid, 0, (zCenter + 0.5f) * OperItemGrid);
    return true;
  }

  // traverse the circle, check for the empty area
  bool found = false;
  Vector3 bestPos;
  float maxDist2 = Square(maxDistance);
  for (int n=1; ;n++)
  {
    // items in the square in distance n from the center
    if (Square((n - 1) * OperItemGrid) >= maxDist2) break; // no better position can be found
    // check the borders of the square
    for (int i=0; i<=n; i++)
    {

#define CHECK_CANDIDATE \
  dist2 = pos.DistanceXZ2(center); \
  if (dist2 < maxDist2) \
  { \
    if (IsAreaFree(x, z, radiusO)) \
    { \
      found = true; \
      bestPos = pos; \
      maxDist2 = dist2; \
    } \
  }

      bool inner = 0 < i && i < n;
      Vector3 pos = VZero;
      float dist2;

      // left edge, up
      OperMapIndex x = OperMapIndex(xCenter - n);
      pos[0] = (x + 0.5) * OperItemGrid;
      OperMapIndex z = OperMapIndex(zCenter - i);
      pos[2] = (z + 0.5) * OperItemGrid;
      CHECK_CANDIDATE;
      if (inner)
      {
        // left edge, down
        z = OperMapIndex(zCenter + i);
        pos[2] = (z + 0.5) * OperItemGrid;
        CHECK_CANDIDATE;
      }

      // top edge, right
      z = OperMapIndex(zCenter - n);
      pos[2] = (z + 0.5) * OperItemGrid;
      x = OperMapIndex(xCenter + i);
      pos[0] = (x + 0.5) * OperItemGrid;
      CHECK_CANDIDATE;
      if (inner)
      {
        // top edge, left
        x = OperMapIndex(xCenter - i);
        pos[0] = (x + 0.5) * OperItemGrid;
        CHECK_CANDIDATE;
      }

      // right edge, down
      x = OperMapIndex(xCenter + n);
      pos[0] = (x + 0.5) * OperItemGrid;
      z = OperMapIndex(zCenter + i);
      pos[2] = (z + 0.5) * OperItemGrid;
      CHECK_CANDIDATE;
      if (inner)
      {
        // right edge, up
        z = OperMapIndex(zCenter - i);
        pos[2] = (z + 0.5) * OperItemGrid;
        CHECK_CANDIDATE;
      }

      // bottom edge, left
      z = OperMapIndex(zCenter + n);
      pos[2] = (z + 0.5) * OperItemGrid;
      x = OperMapIndex(xCenter - i);
      pos[0] = (x + 0.5) * OperItemGrid;
      CHECK_CANDIDATE;
      if (inner)
      {
        // bottom edge, right
        x = OperMapIndex(xCenter + i);
        pos[0] = (x + 0.5) * OperItemGrid;
        CHECK_CANDIDATE;
      }
    }   
  }

  // return the result
  if (!found) return false;
  center = bestPos;
  return true;
}

bool OperMap::IsDestinationNotLocked(AIBrain *unit)
{
  if (!_algorithm) return true;
  if (_endOnRoadOnly) return true;

  const ASOField &field = _algorithm->GetDestination();
  if (field.type == NTHouse) return true; // locking of house positions is not supported 

  EntityAIFull *vehicle = unit->GetVehicle();
  CombatMode mode = unit ? unit->GetCombatMode() : CMSafe;
  bool soldier = unit->GetVehicleIn() == NULL;

  if (field.type == NTRoad || field.type == NTBridge)
  {
    const OperRoad &road = _roads[field.GetRoadIndex()];
    return !road._road->IsLocked(soldier);
  }

  return !GetFieldLocked(field.GetX(), field.GetZ(), vehicle->GetType(), mode, soldier);
}

bool OperMap::IsDestinationFree(AIBrain *unit)
{
  if (!_algorithm) return true;
  if (_endOnRoadOnly) return true;

  const ASOField &field = _algorithm->GetDestination();
  if (field.type == NTHouse) return true; // locking of house positions is not supported 

  EntityAIFull *vehicle = unit->GetVehicle();
  CombatMode mode = unit ? unit->GetCombatMode() : CMSafe;
  bool soldier = unit->GetVehicleIn() == NULL;

  if (field.type == NTRoad || field.type == NTBridge)
  {
    const OperRoad &road = _roads[field.GetRoadIndex()];
    return !road._road->IsLocked(soldier);
  }

  return GetFieldCost(field.GetX(), field.GetZ(), true, vehicle->GetType(), mode, soldier, false).cost < GET_UNACCESSIBLE;
}

void OperMap::LogMap(LandIndex xMin, LandIndex xMax, LandIndex zMin, LandIndex zMax, 
  OperMapIndex xs, OperMapIndex xe, OperMapIndex zs, OperMapIndex ze, bool locks)
{
#if _ENABLE_CHEATS
  int xsize = (xMax - xMin + 1) * OperItemRange;
  int zsize = (zMax - zMin + 1) * OperItemRange;

  LogF("Fields: %d..%d, %d..%d", xMax, xMin, zMin, zMax);

  Temp< Temp<char> > map(zsize);
  for (int i=0; i<zsize; i++)
    map[i].Realloc(xsize * 2 + 1),memset(map[i],' ',xsize*2);

  for (OperMapIndex i(zMin * OperItemRange); i<(zMax+1)*OperItemRange; i++)
  {
    for (OperMapIndex j(xMin*OperItemRange); j<(xMax+1)*OperItemRange; j++)
    {
      SPLIT_INDICES(j, i, x0, z0, xx, zz);
      const OperMapField *fld = GetField(x0, z0);
      Assert(fld);
      OperItem item = fld->_operField->GetItem(xx, zz);

      char ch = '?';
      if (locks && IsLocked(j, i, fld, xx, zz, false)) ch = 'L';
      else if (!fld)
        ch = ' ';
      else
      {
        switch (item.GetTypeSoldier())
        {
        case OITNormal:
          ch = '.';
          break;
        case OITAvoidBush:
          ch = 'b';
          break;
        case OITAvoidTree:
          ch = 't';
          break;
        case OITAvoid:
          ch = 'x';
          break;
        case OITWater:
          ch = 'W';
          break;
        case OITSpaceRoad:
          ch = 'R';
          break;
        case OITRoad:
          ch = 'r';
          break;
        case OITSpaceBush:
          ch = 'B';
          break;
        case OITSpaceHardBush:
          ch = 'H';
          break;
        case OITSpaceTree:
          ch = 'T';
          break;
        case OITSpace:
          ch = 'X';
          break;
        case OITRoadForced:
          ch = 'F';
          break;
        }
      }
      map[i - zMin * OperItemRange][(xsize - 1 - (j - xMin * OperItemRange))*2] = ch;
    }
    map[i - zMin * OperItemRange][xsize*2] = 0;
  }
  for (int i=0; i<_path.Size(); i++)
  {
    int iz = _path[i]._z - zMin * OperItemRange;
    if (iz < 0 || iz >= zsize)
      continue;
    int ix = xsize - 1 - (_path[i]._x - xMin * OperItemRange);
    if (ix < 0 || ix >= xsize)
      continue;
    map[iz][2 * ix + 1] = '+';
  }
  map[zs - zMin * OperItemRange][(xsize - 1 - (xs - xMin * OperItemRange))*2 + 1] = 'S';
  map[ze - zMin * OperItemRange][(xsize - 1 - (xe - xMin * OperItemRange))*2 + 1] = 'E';
  LogF("Begin loop ... %d, begin at %d, %d", zsize, xsize - 1 - (xs - xMin * OperItemRange), zs - zMin * OperItemRange);
  for (int i=0; i<zsize; i++)
    LogF("%02d:%s", i, map[i]);
  LogF("End loop ...");
  Sleep(50);
#endif
}

static inline float FloatSign(float x)
{
  if (x>=0) return 1;
  return -1;
}

/*! modified version of OperMap::IsIntersection
\param [out] cost total cost of direct path
\param [out] costPerItem cost per single item (rectangle of size OperItemGrid)
\return distance to the nearest unaccessible field, negative if none
*/

float OperMap::FirstIntersection(
  float xsf, float zsf, float xef, float zef,
  const EntityAIType *vehType, CombatMode mode, bool soldier
)
{
  #if 1
  static bool doItNew = true;
  if (doItNew)
  {
  
  // based on Landscape::IntersectWithGroundOrig
  
  // when normalizing direction, we need to change maxDist
  // max
  float size2 = Square(xef-xsf)+Square(zef-zsf);
  if (size2<=0)
  {
    // ill defined - only possible result
    return 0;
  }
  float invSize = InvSqrt(size2);
  float size = invSize * size2;
  
  float dx = xef-xsf;
  float dz = zef-zsf;
  
  float dxNorm = dx*invSize;
  float dzNorm = dz*invSize;
  
  float posX = xsf;
  float posZ = zsf;

  float deltaX = dxNorm;
  float deltaZ = dzNorm;

  int xInt = toIntFloor(posX);
  int zInt = toIntFloor(posZ);

  float invDeltaX = fabs(deltaX)<1e-10 ? FloatSign(deltaX)*1e10 : 1/deltaX;
  float invDeltaZ = fabs(deltaZ)<1e-10 ? FloatSign(deltaZ)*1e10 : 1/deltaZ;
  int ddx = deltaX>=0 ? 1 : -1;
  int ddz = deltaZ>=0 ? 1 : -1;
  float dnx = deltaX>=0 ? 1 : 0;
  float dnz = deltaZ>=0 ? 1 : 0;

  // check cost of the field we are already in
  float cost = GetFieldCost(OperMapIndex(xInt), OperMapIndex(zInt), true, vehType, mode, soldier, false).cost;
  if (cost>=GET_UNACCESSIBLE)
  {
    // if already too high, return immediate collision
    return 0;
  }
    
  // maintain beg, end on current square
  float tRest = size;
  float tDone = 0;
  // max. iterations helps to keep number of iterations with a finite limits
  int maxIter = toIntCeil(size*H_SQRT2)+2;
  while (tRest>0)
  {
    if (--maxIter<0)
    {
      LogF(
        "OperMap::FirstIntersection: Max iterations failed (size %.1f, iters %d, max %d)",
        size,toIntCeil(size*H_SQRT2),toIntCeil(size*H_SQRT2)-maxIter
      );
      break;
    }
    //float begX = posX;
    //float begZ = posZ;

    //advance to next relevant neighbor
    //int xio = xInt, zio = zInt;
    float tx = (xInt+dnx-posX) * invDeltaX;
    float tz = (zInt+dnz-posZ) * invDeltaZ;
    //Assert( tx>=-0.01 );
    //Assert( tz>=-0.01 );
    //float tDoneBeg = tDone;
    if (tx<=tz)
    {
      saturateMin(tx,tRest);
      xInt += ddx;
      tRest -= tx;
      tDone += tx;
      posX += dxNorm*tx;
      posZ += dzNorm*tx;
    }
    else
    {
      saturateMin(tz,tRest);
      zInt += ddz;
      tRest -= tz;
      tDone += tz;
      posX += dxNorm*tz;
      posZ += dzNorm*tz;
    }

    // check cost of the field we are going into
    float cost = GetFieldCost(OperMapIndex(xInt), OperMapIndex(zInt), true, vehType, mode, soldier, false).cost;
    if (cost>=GET_UNACCESSIBLE)
    {
      return tDone;
    }
  }
  return size;
  }
  #endif
  
  OperMapIndex xs(toIntFloor(xsf));
  OperMapIndex zs(toIntFloor(zsf));
  OperMapIndex xe(toIntFloor(xef));
  OperMapIndex ze(toIntFloor(zef));
  int deltaX = xe - xs;
  int deltaZ = ze - zs;
  int incx = deltaX > 0 ? 1 : -1;
  int incz = deltaZ > 0 ? 1 : -1;
  float dx = deltaX;
  float dz = deltaZ;

//LogF("Check intersection %d, %d -> %d, %d", xs, zs, xe, ze);  
  if (fabs(dx) < fabs(dz))
  {
    float invabsdz = 1.0 / fabs(dz);
    dx *= invabsdz;
    float invdx = dx!=0 ? 1.0 / dx : 1e10;
    //float coef = sqrt(1 + Square(dx));
    float x = xs + 0.5;
    for (OperMapIndex z(zs); z!=ze+incz; z+=incz)
    {
      x += 0.5 * dx;
      if (toIntFloor(x) != xs)
      {
        Assert(toIntFloor(x) == xs + incx);
        float cost1 = GetFieldCost(xs, z, true, vehType, mode, soldier, false).cost;
        float cost2 = GetFieldCost(OperMapIndex(xs + incx), z, true, vehType, mode, soldier, false).cost;
        float a;
        if (dx > 0)
          a = (x - (xs + 1)) * invdx;
        else
          a = (x - xs) * invdx; 
        if
        (
          a < 0.99f && cost1 >= GET_UNACCESSIBLE ||
          a > 0.01f && cost2 >= GET_UNACCESSIBLE
        )
        {
          return sqrt(Square(z-zs)+Square(x-xs));
        }
        xs += incx;
      }
      else
      {
        float cost1 = GetFieldCost(xs, z, true, vehType, mode, soldier, false).cost;
        if (cost1 >= GET_UNACCESSIBLE)
        {
          return sqrt(Square(z-zs)+Square(x-xs));
        }
      }
      x += 0.5 * dx;
    }
  }
  else
  {
    float invabsdx = 1.0 / fabs(dx);
    dz *= invabsdx;
    float invdz = dz!=0 ? 1.0 / dz : 1e10;
    //float coef = sqrt(1 + Square(dz));
    float z = zs + 0.5;
    for (OperMapIndex x(xs); x!=xe+incx; x+=incx)
    {
      z += 0.5 * dz;
      if (toIntFloor(z) != zs)
      {
//        Assert(toIntFloor(z) == zs + incz);
        float cost1 = GetFieldCost(x, zs, true, vehType, mode, soldier, false).cost;
        float cost2 = GetFieldCost(x, OperMapIndex(zs + incz), true, vehType, mode, soldier, false).cost;
        float a;
        if (dz > 0)
          a = (z - (zs + 1)) * invdz;
        else
          a = (z - zs) * invdz; 
        if
        (
          a < 0.99f && cost1 >= GET_UNACCESSIBLE ||
          a > 0.01f && cost2 >= GET_UNACCESSIBLE
        )
        {
          return sqrt(Square(z-zs)+Square(x-xs));
        }
        zs += incz;
      }
      else
      {
        float cost1 = GetFieldCost(x, zs, true, vehType, mode, soldier, false).cost;
        if (cost1 >= GET_UNACCESSIBLE)
        {
          return sqrt(Square(z-zs)+Square(x-xs));
        }
      }
      z += 0.5 * dz;
    }
  }
  return FLT_MAX;
}

/*! modified version of OperMap::IsIntersection
\param [out] cost total cost of direct path
\param [out] costPerItem cost per single item (rectangle of size OperItemGrid)
\return true if some unaccessible field is found
*/

bool OperMap::IsIntersection(
  OperMapIndex xs, OperMapIndex zs, OperMapIndex xe, OperMapIndex ze,
  float &cost, float &costPerItem, const EntityAIType *vehType, CombatMode mode, bool soldier
)
{
  if (xs==xe && zs==ze)
  {
    cost = 0;
    costPerItem = 1;
    return false;
  }

  int deltaX = xe - xs;
  int deltaZ = ze - zs;
  int incx = deltaX > 0 ? 1 : -1;
  int incz = deltaZ > 0 ? 1 : -1;
  float dx = deltaX;
  float dz = deltaZ;

  cost = 0;

  //LogF("Check intersection %d, %d -> %d, %d", xs, zs, xe, ze);  
  if (fabs(dx) < fabs(dz))
  {
    float invabsdz = 1.0f / fabs(dz);
    dx *= invabsdz;
    float invdx = 1e10f;
    if (dx!=0) invdx = 1.0f / dx;
    float coef = sqrt(1 + Square(dx));
    float x = xs + 0.5f;
    for (OperMapIndex z(zs); z!=ze+incz; z+=incz)
    {
      x += 0.5f * dx;
      if (toIntFloor(x) != xs)
      {
        Assert(toIntFloor(x) == xs + incx);
        float cost1 = GetFieldCost(xs, z, true, vehType, mode, soldier, false).cost;
        float cost2 = GetFieldCost(OperMapIndex(xs + incx), z, true, vehType, mode, soldier, false).cost;
        if (soldier)
        {
          if (CheckOperItem(xs,z).GetClearance()<ClearanceNormal) return true;
          if (CheckOperItem(OperMapIndex(xs + incx),z).GetClearance()<ClearanceNormal) return true;
        }
        float a;
        if (dx > 0)
          a = (x - (xs + 1)) * invdx;
        else
          a = (x - xs) * invdx; 
        if (a < 0.99f && cost1 >= GET_UNACCESSIBLE)
          return true;
        if (a > 0.01f && cost2 >= GET_UNACCESSIBLE)
          return true;
        if (z!=zs) cost += a * cost2 + (1.0f - a) * cost1;
        xs += incx;
      }
      else
      {
        float cost1 = GetFieldCost(xs, z, true, vehType, mode, soldier, false).cost;
        if (cost1 >= GET_UNACCESSIBLE)
          return true;
        if (CheckOperItem(xs,z).GetClearance()<ClearanceNormal) return true;
        if (z!=zs) cost += cost1;
      }
      x += 0.5f * dx;
    }
    costPerItem = cost * invabsdz;
    cost *= coef;
  }
  else
  {
    float invabsdx = 1.0f / fabs(dx);
    dz *= invabsdx;
    float invdz = 1e10f;
    if (dz!=0) invdz = 1.0f / dz;
    float coef = sqrt(1 + Square(dz));
    float z = zs + 0.5f;
    for (OperMapIndex x(xs); x!=xe+incx; x+=incx)
    {
      z += 0.5f * dz;
      if (toIntFloor(z) != zs)
      {
        //        Assert(toIntFloor(z) == zs + incz);
        float cost1 = GetFieldCost(x, zs, true, vehType, mode, soldier, false).cost;
        float cost2 = GetFieldCost(x, OperMapIndex(zs + incz), true, vehType, mode, soldier, false).cost;
        if (soldier)
        {
          if (CheckOperItem(x,zs).GetClearance()<ClearanceNormal) return true;
          if (CheckOperItem(x, OperMapIndex(zs + incz)).GetClearance()<ClearanceNormal) return true;
        }
        float a;
        if (dz > 0)
          a = (z - (zs + 1)) * invdz;
        else
          a = (z - zs) * invdz; 
        if (a < 0.99f && cost1 >= GET_UNACCESSIBLE)
          return true;
        if (a > 0.01f && cost2 >= GET_UNACCESSIBLE)
          return true;
        if (x!=xs) cost += a * cost2 + (1.0f - a) * cost1;
        zs += incz;
      }
      else
      {
        float cost1 = GetFieldCost(x, zs, true, vehType, mode, soldier, false).cost;
        if (CheckOperItem(x,zs).GetClearance()<ClearanceNormal) return true;
        if (cost1 >= GET_UNACCESSIBLE)
          return true;
        if (x!=xs) cost += cost1;
      }
      z += 0.5f * dz;
    }
    costPerItem = cost * invabsdx;
    cost *= coef;
  }
  return false;
}

void OperMap::CreateMap(EntityAI *veh, LandIndex xMin, LandIndex zMin, LandIndex xMax, LandIndex zMax, int mask)
{

  // Create map
  const EntityAIType *vehType = veh->GetType();
  AIBrain *unit = veh->PilotUnit();
  CombatMode mode = unit ? unit->GetCombatMode() : CMSafe;
  bool isSoldier = unit ? unit->IsSoldier() : false;
  CreateMap(vehType,isSoldier,mode,xMin,zMin,xMax,zMax,mask);
}  
  
void OperMap::CreateMap(
  const EntityAIType *vehType, bool isSoldier, CombatMode mode, LandIndex xMin, LandIndex zMin, LandIndex xMax, LandIndex zMax, int mask
)
{
  PROFILE_SCOPE_EX(pthMa, aiPath);
  // destroy current map
  Clear(xMax+1-xMin,zMax+1-zMin);
  _vehType = vehType;
  _mode = mode;
  _isSoldier = isSoldier;
  
  for (LandIndex i(xMin); i<=xMax; i++)
    for (LandIndex j(zMin); j<=zMax; j++)
    {
      CreateField(i, j, mask, vehType, mode, _isSoldier);
    }
}

bool OperMap::OperFieldRangeReady(LandIndex xMin, LandIndex xMax, LandIndex zMin, LandIndex zMax)
{
  // CreateField may use one more on each side because of RoadSurfaceY
  xMin--,xMax++;
  zMin--,zMax++;
  return GWorld->CheckObjectsReady(xMin,xMax,zMin,zMax,false);
}

bool OperMap::IsSimplePath(const AIBrain *unit, OperMapIndex xs, OperMapIndex zs, OperMapIndex xe, OperMapIndex ze, float &simpleCost, float &simpleHeurCost)
{
  return !IsIntersection(xs, zs, xe, ze, simpleCost, simpleHeurCost, unit->GetVehicle()->GetType(), unit->GetCombatMode(), unit->IsSoldier());
}

/*
bool operator <(OperItem &a, OperItem &b)
{
  return a._cost + a._heur < b._cost + b._heur;
}

bool operator <=(OperItem &a, OperItem &b)
{
  return a._cost + a._heur <= b._cost + b._heur;
}

TypeIsSimple(OperItem *);
typedef HeapArray<OperItem *,MemAllocSS> OperOpenList;
*/

#if _DEBUG
  const int UpdateSleep=1000;
  const int FinishSleep=1000;
#else
  const int UpdateSleep=0; // time to sleep after single interation
  const int FinishSleep=0; // time to sleep after finishing search
#endif

#if _ENABLE_AI

/**
@param maxCost allows to provide adhoc information used for last attempts when first attempts fail
*/

float OperMap::PrepareInitFind(int iteration, bool endOnRoadOnly, float maxCost)
{
  _alternateGoal = endOnRoadOnly;
  _endOnRoadOnly = endOnRoadOnly;
  _replanLater = false;

  // scan for min,max,avg cost and do some arithmetics with results
  float minCost = GET_UNACCESSIBLE, avgCost = 0;
  int cntCost = 0;

  IteratorState it;
  const OperMapField *fld = _fields.getFirst(it);
  while (fld)
  {
    // fields should be valid here (called directly after CreateMap)
    float cost = fld->_heurCost;
    if (cost < GET_UNACCESSIBLE)
    {
      saturateMax(maxCost, cost);
      saturateMin(minCost, cost);
      avgCost += cost;
      cntCost++;
    }

    fld = _fields.getNext(it);
  }

  // maybe we can use some roads
  if (_roads.Size() > 0)
  {
    float coef = _vehType->GetRoadCost(_mode).cost;
    float baseCost = OperItemGrid * _vehType->GetMinCost();
    saturateMin(minCost, coef * baseCost);
  }

/*
  int n = _fields.Size();
  for (int i=0; i<n; i++)
  {
    const OperMapField &fld = _fields[i];
    // fields should be valid here (called directly after CreateMap)
    float cost = fld._heurCost;
    if (cost < GET_UNACCESSIBLE)
    {
      saturateMax(maxCost, cost);
      saturateMin(minCost, cost);
      avgCost += cost;
      cntCost++;

      // if there are roads in this field, we can use them
      if (fld._operField->_roads.Size() > 0)
      {
        float coef = _vehType->GetRoadCost(_mode).cost;
        float baseCost = floatMin(cost, OperItemGrid * _vehType->GetMinCost());
        saturateMin(minCost, coef * baseCost);
      }
    }
  }
*/
  if (cntCost>0)
  {
    avgCost /= cntCost;
  }

  float heurCost = 1;
  // TODO: tune coefs
  // because of roads, we need to decrease a heuristic
  switch (iteration)
  {
  case 0:
    // heurCost = avgCost * 0.8 + minCost * 0.2;
    heurCost = avgCost * 0.2 + minCost * 0.8;
    break;
  case 1:
    heurCost = maxCost * 0.4 + avgCost * 0.6;
    break;
  case 2:
  default:
    heurCost = maxCost;
    break;
  }
  return heurCost;
}

/*!
\patch 5128 Date 2/8/2007 by Jirka
- Fixed: Moving of cars in convoy - better following of road
*/

bool OperMap::InitFindPath(AIBrain *unit, const OperInfo &infoFrom, const OperInfo &infoTo, int dir, bool locks, const CoverDisposition &dispo)
{
  #if 0 // _PROFILE
  if (unit->GetUnit() && unit->GetUnit()->ID()==2)
  {
    LogF(
      "%s: search to %.1f,%.1f (dist %.1f)",cc_cast(unit->GetDebugName()),
      infoTo._x*OperItemGrid,infoTo._z*OperItemGrid,
      sqrt(Square(infoTo._x-infoFrom._x)+Square(infoTo._z-infoFrom._z))*OperItemGrid
    );
  }
  #endif
  PROFILE_SCOPE_EX(fpAll, aiPath);

  // make sure we reset iteration count limit before starting the first pass
  _iterOp = 0;
  // Clear() was called in createMap
  
  OperMapIndex xs(infoFrom._x);
  OperMapIndex zs(infoFrom._z);
  OperMapIndex xe(infoTo._x);
  OperMapIndex ze(infoTo._z);

// LogF("Searching from: %d, %d to %d, %d", xs, zs, xe, ze);

  EntityAI *veh = unit->GetVehicle();
  DoAssert(veh->GetType() == _vehType);
  DoAssert(unit->IsSoldier() == _isSoldier);
  DoAssert(unit->GetCombatMode() == _mode);

  // we know we want to visit both starting and final fields, therefore we want to have both of them included in stats
  float costS = 0,costE = 0;
  if (infoFrom._type==NTGrid) costS = GetFieldCost(xs, zs, true, _vehType, _mode, _isSoldier, false).cost;
  if (infoTo._type==NTGrid) costE = GetFieldCost(xe, ze, true, _vehType, _mode, _isSoldier, false).cost;
  // if cost of starting or final field is UNACCESSIBLE, something went wrong
  // note: when planning into cover, infoTo may be a nonsense, and may have cost unaccessible
  float maxCost = costE<GET_UNACCESSIBLE ? floatMax(costS,costE) : costS;
  DoAssert(maxCost<GET_UNACCESSIBLE);
  if (maxCost>= GET_UNACCESSIBLE) maxCost = 0;
  
  float heurCost = PrepareInitFind(unit->GetIter(), infoTo._type == NTBridge, maxCost);

  bool searchForCover = ( dispo._maxCost>0 || dispo._movingTo._cover ) && infoFrom._type != NTBridge && infoTo._type != NTBridge;
  // diagnostics initialization
#if _ENABLE_CHEATS
  if (GOperMapDebug)
  {
    GOperMapDebug->Attach(unit, xs, zs, xe, ze, heurCost, false, "Path");
  }
  #if 0
  if (unit->GetUnit() && unit->GetUnit()->IsSubgroupLeader())
  {
    LogF("%s: Planning in mode %s, cover %d", cc_cast(unit->GetDebugName()), cc_cast(FindEnumName(_mode)),searchForCover);
    if (_mode==CMCombat && !searchForCover)
    {
      #ifndef _DEBUG
        DebugOperMap(unit);
#endif
      __asm nop;
    }
    __asm nop;
  }
  #endif
#endif



  // when searching for a cover, we can never use simple pathing
  if (!searchForCover)
  {
//  Check if simplified methods can be used
  // for some vehicles - if changing direction a lot, always use proper path finding
  // direction to target ...

  int dirD = AI::CalcDirection(Vector3(xe - xs, 0, ze - zs)) - dir;
  dirD = AI::DirectionAngleDifference(dirD);

    float simpleCost, simpleHeurCost;
    if (
      infoFrom._type == NTGrid && infoTo._type == NTGrid &&
      !IsIntersection(xs, zs, xe, ze, simpleCost, simpleHeurCost, _vehType, _mode, _isSoldier))
    {
      // include turning in checking result acceptability
      int debet = 0;
      float costTurn = _vehType->GetCostTurn(dirD,debet,true);
      float costMinTurn = _vehType->GetCostTurn(1,debet,true)*abs(dirD);
  #if LOG_MAPS
        LogF
        (
          "Unit %s: Direct path %s, heur %.3f, found %.3f",
          (const char *)unit->GetDebugName(),
          simpleHeurCost <= 1.01f * heurCost ? "found" : "not found",
          heurCost,simpleHeurCost
        );
  #endif
      // check if we match heuristic estimation
      if (simpleHeurCost+costTurn <= 1.01f * (heurCost+costMinTurn))
      {
        _path.Resize(2);
        _path[0]._x = xs;
        _path[0]._z = zs;
        _path[0]._house = NULL;
        _path[0]._housePos = -1;
        _path[0]._road = RoadNode(NULL, -1);
        _path[0]._cost = 0;
        _path[0]._type = NTGrid;
        _path[1]._x = xe;
        _path[1]._z = ze;
        _path[1]._house = NULL;
        _path[1]._housePos = -1;
        _path[1]._road = RoadNode(NULL, -1);
        _path[1]._cost = simpleCost;
        _path[1]._type = NTGrid;
        // mark the path as not leading into any cover
        _pathToOpenCover = false;
        _pathToCover = CoverInfo();

  #if _ENABLE_CHEATS
        if (GOperMapDebug && GOperMapDebug->IsAttached())
        {
          GOperMapDebug->Update();
          if (FinishSleep > 0) Sleep(FinishSleep);
          GOperMapDebug->Detach(unit,false);
          if (DebugAutoHide) WaitForClose(GOperMapDebug);
        }
  #endif
        return true;
      }
      else
      {
        // TODO: verify difference between using and not using following statement
        //heurCost = simpleHeurCost; // use simple heur cost 
      }
    }
  }

  LAND_INDICES(xs, zs, xsLand, zsLand);
  const OperMapField *test = GetField(xsLand, zsLand);
  if (!test)
  {
    // cannot plan operative path outside map
    RptF(
      "Out of path-planning region for %s at %.1f,%.1f, node type",
      cc_cast(unit->GetDebugName()),xs*OperItemGrid,zs*OperItemGrid,cc_cast(FindEnumName(infoFrom._type.GetEnumValue()))
    );
    
    // error recovery - try to use simple path
    return true;
  }

  // find the indices of houses
  int houseFrom = -1;
  ASONodeType typeFrom = infoFrom._type;
  if (typeFrom == NTHouse)
  {
    houseFrom = GetHouseIndex(infoFrom._house);
    if (houseFrom < 0)
    {
      RptF("House %s (from) not found in the operative map", cc_cast(infoFrom._house ? infoFrom._house->GetDebugName() : "<null>"));
      // Recovery (avoid the crash)
      typeFrom = NTGrid;
    }
  }
  int houseTo = -1;
  ASONodeType typeTo = infoTo._type;
  if (typeTo == NTHouse)
  {
    houseTo = GetHouseIndex(infoTo._house);
    if (houseTo < 0)
    {
      RptF("House %s (to) not found in the operative map", cc_cast(infoTo._house ? infoTo._house->GetDebugName() : "<null>"));
      // Recovery (avoid the crash)
      typeTo = NTGrid;
    }
  }

  // create a snapshot of unit's target list usable for suppress cost calculations
  _suppressTargets.CollectTargets(unit);

  // store locker to decide if locked field is locked by searching vehicle
  _locker = veh->GetLocker();

#if _ENABLE_REPORT
  _isCameraVehicle = veh == GWorld->CameraOn();
  _debugName = unit->GetDebugName();
#endif

  // create A* algorithm
  ASOField start(ErrorField);
  switch (typeFrom)
  {
  case NTHouse:
    start = ASOField(HouseField, houseFrom, infoFrom._housePos);
    break;
  case NTRoad:
  case NTBridge:
    {
      int roadIndex = _roads.FindKey(infoFrom._road.GetRoad());
      DoAssert(roadIndex >= 0);
      start = ASOField(RoadField, typeFrom, roadIndex, infoFrom._road.GetNodeIndex(), dir);
      break;
    }
  default:
    Fail("Unexpected node type");
  case NTGrid:
    start = ASOField(GridField, xs, zs, dir);
    break;
  }
  ASOField end(ErrorField);
  switch (typeTo)
  {
  case NTHouse:
    end = ASOField(HouseField, houseTo, infoTo._housePos);
    break;
  case NTRoad:
  case NTBridge:
    {
      int roadIndex = _roads.FindKey(infoTo._road.GetRoad());
      DoAssert(roadIndex >= 0);
      end = ASOField(RoadField, typeTo, roadIndex, infoTo._road.GetNodeIndex(), dir);
      break;
    }
  default:
    Fail("Unexpected node type");
  case NTGrid:
    end = ASOField(GridField, xe, ze, dir);
    break;
  }

  // init "time" information for the search
  start._t = 0;

  // Clear was called at least during CreateMap
  DoAssert(_algorithm == NULL);

  ASOHeuristicFunction heuristicFunction(this, heurCost);
  if (searchForCover)
  {
    // note: for the formation leader target is the same as command destination
    // when we are the formation leader, we only want to accept cover closer to the command destination
    // when subordinate, we want to move forward in the direction of the formation
    
    int xt = toIntFloor(dispo._movingTo.X()*InvOperItemGrid);
    int zt = toIntFloor(dispo._movingTo.Z()*InvOperItemGrid);
    #if _DEBUG
      Vector3 sPos(xs*OperItemGrid,0,zs*OperItemGrid);
      Vector3 sDif((xt-xs)*OperItemGrid,0,(zt-zs)*OperItemGrid);
    #endif
    // we want to be at least 3m closer
    float distToTarget = sqrt(Square(xt-xs)+Square(zt-zs))-3.0f*InvOperItemGrid;
    // we must have at least some chance of finding a cover
    if (distToTarget>OperItemGrid*2.0f || dispo._movingTo._cover && distToTarget>0)
    {
      // TODO: consider: separate AStar implementation when seeking a path into a given cover
      float distToDest = dispo._movingTo._cover ? 3.0f : distToTarget;
      ASOCoverCostFunctionsWrapped costFcs(
        ASOCostFunction(this, locks),
        heuristicFunction,
        ASOCoverCostFunctions::AddHeuristics(this, dispo._maxCost,distToDest,xt,zt)
      );
      if (CHECK_DIAG(DEFSM))
      {
        static int handle;
        static int unique; // make unique so that each arrow stays displayed until timeout
        GDiagsShown.ShowArrow(handle,unique++,20,Format("%s: Moving to",cc_cast(unit->GetDebugName())),dispo._movingTo,VUp,3,Color(1,0,1));
      }
      if (CHECK_DIAG(DEFSM))
      {
        Vector3 ePos = infoTo.GetPathPos(0,true,0);
        static int handle;
        static int unique; // make unique so that each arrow stays displayed until timeout
        GDiagsShown.ShowArrow(handle,unique++,20,Format("%s: Path target",cc_cast(unit->GetDebugName())),ePos,VUp,3,Color(1,0.5,1));
      }
      _algorithm = new AStarCover(
        start, end,
        costFcs,
        ASOReadyFunction(this), ASOInterruptFunction(this, ItersPerFrame),
        this,distToDest,dispo._hideFromL,dispo._hideFromR,xt,zt,dispo._movingTo,dispo._maxCost,dispo._maxDelay
      );
    }
  }
  if (_algorithm.IsNull())
  { // no cover searched - use a simple termination
    ASOCostFunctionsWrapped costFcs(
      ASOCostFunction(this, locks),
      heuristicFunction
    );
    _algorithm = new AStarOperative(
      start, end,
      costFcs,
      ASOReadyFunction(this), ASOInterruptFunction(this, ItersPerFrame),
      AStarEndDefault<ASOField>(end)
    );
  }

  _heuristicLimit = heuristicFunction(start, end);

#if USE_OPERPATH_THREAD
  // TODO: finish time based on the real priority
  if (GJobManager.IsUsable()) GJobManager.CreateJob(this, 5000);
#endif

#if DIAG_DELAY
  _findStarted = GlobalTickCount();
#endif

#if 0
  bool diags = GWorld->CameraOn() == veh;
  if (diags)
  {
    LogF("%.2f %s starts searching of operative path", Glob.time.toFloat(), cc_cast(unit->GetVehicle()->GetDebugName()));
    LogF("  minCost = %.2f", minCost);
    LogF("  avgCost = %.2f", avgCost);
    LogF("  maxCost = %.2f", maxCost);
    LogF("  heurCost = %.2f (iteration %d)", heurCost, unit->GetIter());
  }
#endif

  return false;
}

CoverInfo OperMap::FindNearestCover(Vector3Par pos, EntityAIFull *veh, LandIndex xMin, LandIndex zMin, LandIndex xMax, LandIndex zMax) const
{
  float coverDist2 = Square(5);
  CoverInfo bestCover;
  for (LandIndex x(xMin); x<=xMax; x++) for (LandIndex z(zMin); z<=zMax; z++)
  {
    // for each field find the best cover
    const OperMapField *mapField = GetField(x,z);
    if (mapField && mapField->_operField)
    {
      OperField *field = mapField->_operField;
      if (field)
      {
        field->PrepareForPathing();
        int nCover;
        const OperCover *cover = field->GetCover(nCover);
        for (int i=0; i<nCover; i++)
        {
          #if DIAG_FALSE_COVER // for diags add cover even when we know it is false
          if (cover[i]._valid!=OperCover::Valid) continue;
          #endif
          float dist2 = cover[i]._pos.Distance2(pos);
          if (coverDist2>dist2)
          {
            coverDist2 = dist2;
            // TODO: use some simplified version of veh->FindCover
            bestCover._payload = new CoverVars;
            bestCover._payload->_pos = cover[i]._pos;
            bestCover._payload->_entry = cover[i]._pos;
            bestCover._payload->_type = cover[i]._type;
            bestCover._payload->_areaFreeBack = 1.0f;       
            bestCover._payload->_areaFreeLeft = 0.5f;
            bestCover._payload->_areaFreeRight = 0.5f;
            
            Vector3 landPos = cover[i]._pos;
            landPos[1] = GLandscape->RoadSurfaceY(cover[i]._pos);
            bestCover._payload->_coverPos.SetPosition(landPos);
            bestCover._payload->_coverPos.SetOrientation(Matrix3(MRotationY,H_PI-cover[i]._heading));
            
          }
        }
      }
      
    }
  }
  return bestCover;
}

#if USE_OPERPATH_THREAD

bool OperMap::Process()
{
  // until there is no condition to finish, continue with Updates
  while (!Update())
  {
    if (WantToBeCanceled() || WantToBeSuspended()) return false;
  }
  return true; // searching finished
}

#endif

/// max. number of iterations for OperItemGrid 2.5
const float MaxItersFor2p5 = 4000.0f;
const float MaxItersFor1p0 = MaxItersFor2p5 * (2.5f * 2.5f);

bool OperMap::Update()
{
  PROFILE_SCOPE_EX(opMUp, aiPath);

  if (!_algorithm)
  {
    Fail("Cannot search path - no algorithm");
    return true;
  }
  DoAssert(!IsDone());

  ASOContext context;
  context._map = this;
  context._useRoads = true;
  context._isSoldier = _isSoldier;
#if DIAG_A_STAR && _ENABLE_REPORT
  bool diags = _isCameraVehicle;
#else
  bool diags = false;
#endif
  int ret = _algorithm->Process(&context, diags);
  _iterOp += ret;

  // if done, report success
  if (IsDone()) return true;

  // check if iteration count spent in both passes
  if (_iterOp >= MaxItersFor1p0 * Square(InvOperItemGrid))
  {
    // not done, aborting search, but we may need to start 2nd pass
    if (_algorithm->AbortSearch(this))
    {
      // search aborted, report failure
      return true;
    }
    // search not aborted (2nd pass started), report search should continue
    // reset iteration count for the 2nd pass
    _iterOp = 0;
    return false;
  }

  return false;
}

/**
@param respectCombatModeCost sometimes we need to increase base cost to indicate movement in the open is more dangerous/slower.
 This is a hack needed because otherwise the paths found are too fast when searching for a cover.
*/

bool OperMap::ProcessFindPath(AIBrain *unit)
{
  PROFILE_SCOPE_EX(fpAll, aiPath);

#if _ENABLE_CHEATS
  if (GOperMapDebug && GOperMapDebug->IsAttached())
  {
    GOperMapDebug->Update();
    if (UpdateSleep > 0) Sleep(UpdateSleep);
  }
#endif

#if USE_OPERPATH_THREAD
  // check state
  switch (GetJobState())
  {
  case JSNone:
    // singlecore solution - no job is running
    break;
  case JSWaiting:
  case JSRunning:
    return false; // in progress
  case JSDone:
    return true;
  case JSSuspended:
    // LogF("*** %s - resuming, %d iterations done", cc_cast(unit->GetDebugName()), _iterOp);
    if (_failure)
    {
      // first, check if objects are loaded
      if (!OperFieldRangeReady(LandIndex(_failureX - 1), LandIndex(_failureX + 1), LandIndex(_failureZ - 1), LandIndex(_failureZ + 1)))
        return false;
      // handle failure
      int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER | MASK_FOR_PATHING;
      CreateField(_failureX, _failureZ, mask, _vehType, _mode, _isSoldier);
      _failure = false;
      // LogF(" - failure handled on %d, %d", _failureX, _failureZ);
    }
    // resume
    GJobManager.ResumeJob(this);
    return false;
  case JSCanceled:
  default:
    Fail("Unexpected thread state");
    return true;
  }
#endif
  return Update();
}


#if 0 // _DEBUG || _PROFILE
# define PATH_STATS 1
#endif

#if PATH_STATS

class PathStats
{
  static const int NUnits = 32;
  float distanceSum[NUnits];
  float travelSum[NUnits];
  float costSum[NUnits];
  int samples[NUnits];
  
  public:
  PathStats() {Reset();}
  void Reset();
  
  void Add(int unit, float distance, float travel, float cost)
  {
    if (unit>NUnits || unit<=0) return;
    distanceSum[unit-1] += distance;
    travelSum[unit-1] += travel;
    costSum[unit-1] += cost;
    samples[unit-1] += 1;
  }
  
  void Report(int unit)
  {
    if (unit>NUnits || unit<=0) return;
    int hops = samples[unit-1];
    if (hops<=0) return;
    LogF(
      "#$$ %d stats: avg distance %.2f, avg travel %.2f, avg cost %.2f, hops %d",
      unit, distanceSum[unit-1]/hops, travelSum[unit-1]/hops, costSum[unit-1]/hops, hops
    );  
  }
};

static PathStats GPathStats;

void PathStats::Reset()
{
  for (int i=0; i<NUnits; i++) distanceSum[i] = 0, travelSum[i] = 0, costSum[i] = 0, samples[i] = 0;
}
#endif

/*!
\patch 5090 Date 11/27/2006 by Jirka
- Improved: AI pathfinding failing less often now
*/

bool OperMap::ExportPath(AIBrain *unit)
{
  if (!_algorithm) return false;

  PROFILE_SCOPE_EX(fpAll, aiPath);

#if _ENABLE_CHEATS
  if (GOperMapDebug && GOperMapDebug->IsAttached())
  {
    GOperMapDebug->Update();
    if (FinishSleep > 0) Sleep(FinishSleep);
    GOperMapDebug->Detach(unit,false);
    if (DebugAutoHide) WaitForClose(GOperMapDebug);
  }
#endif

  _pathToCover = CoverInfo();
  _pathToOpenCover = false;
  const ASONode *last = NULL;
  bool coverFound = false;
  //if (unit->IsCoveringEnabled()) // check if covering is still enabled. If not, do not use cover, even if it was found
    last = _algorithm->GetBestCover(*this,_pathToCover,unit);
  // even when last is returned, we need to check what kind of cover it is
  if (last)
  {
    if (_pathToCover)
    {
      coverFound = true;
      // when command destination not reached, re-planning will be needed
      if (!_algorithm->IsTargetReached(last,this))
      {
      _alternateGoal = true;
      _replanLater = true;
    }
  }
  }
  else if (_algorithm->IsDone() && _algorithm->IsFound())
  {
    // path found
    last = _algorithm->GetLastNode();
    if (!last) return false;
    if (!last->_parent) return false; // singular path (last is start node)
  }
  else
  {
    // path not found - try to select alternate goal
    last = _algorithm->GetBestNode(this);
    if (!last) return false;
    if (!last->_parent) return false; // singular path (last is start node)

#if 0
    if (last->_f - last->_g >= _heuristicLimit) return false;
    _alternateGoal = true;
#else
    const ASOField &end = _algorithm->GetDestination();

    // find the start node
    const ASONode *start = last;
    while (start->_parent) start = start->_parent;

    // calculate how far from the destination are start and last points
    OperMapIndex xs, zs, xe, ze, xl, zl;
    start->_field.GetXZ(*this, xs, zs);
    last->_field.GetXZ(*this, xl, zl);
    end.GetXZ(*this, xe, ze);

    float distStart = OperItemGrid * sqrt(Square(xe - xs) + Square(ze - zs));
    float distLast = OperItemGrid * sqrt(Square(xe - xl) + Square(ze - zl));

    LogF(
      "distStart %.2f, distLast %.2f, diff %.2f, limit %.2f",
      distStart, distLast, distStart - distLast, 2.0 * unit->GetVehicle()->GetPrecision()
    );

    // check if I will move enough
    if (distStart - distLast >= 1.5f * unit->GetVehicle()->GetPrecision())
    {
      _alternateGoal = true;
      _replanLater = true;
    }
    else
    {
      // not worth to use the path
      return false;
    }
#endif
  }

  if (unit->IsCoveringEnabled() && !coverFound)
  {
    // GetBestCover have already performed a "best cover" search - use it
    if (last!=_algorithm->GetLastNode())
    {
      // mark the path as a partial one
      _alternateGoal = true;
      _replanLater = true;
    }
    // when covering is enabled, always mark the path as leading into a cover
    // this will prevent re-planning it, and it will make sure AI is reporting reaching it
    // mark the path as leading into an open cover
    _pathToOpenCover = true;
  }

  // export path
  int depth = 0;
  for (const ASONode *cur=last; cur!=NULL; cur=cur->_parent) depth++;
  _path.Resize(depth);
  int i = depth - 1;
  for (const ASONode *cur=last; cur != NULL; cur=cur->_parent)
  {
    OperInfo &info = _path[i--];
    //info._cost = cur->_g;
    info._cost = cur->_field._t;
    info._x = OperMapIndex(-1);
    info._z = OperMapIndex(-1);
    info._house = NULL;
    info._housePos = -1;
    info._road = RoadNode(NULL, -1);
    ASONodeType type = cur->_field.type;
    info._type = type;
    info._clearance = ClearanceNormal;
    // additional parameters specific for the type
    switch (type)
    {
    case NTRoad:
    case NTBridge:
      {
        const OperRoad &road = _roads[cur->_field.GetRoadIndex()];
        info._road = RoadNode(road._road, cur->_field.GetNodeIndex());
      }
      break;
    case NTHouse:
      {
        const OperHouse &house = _houses[cur->_field.GetHouseIndex()];
        info._house = house._house;
        info._housePos = cur->_field.GetPosIndex();
      }
      break;
    default:
      Assert(type == NTGrid);
      info._x = cur->_field.GetX();
      info._z = cur->_field.GetZ();
      info._clearance = GetFieldClearance(info._x,info._z,_vehType,_mode,_isSoldier);
      break;
    }
  }
  
  #if PATH_STATS
  float directDist = 0; // direct distance from start to end
  float travelDist = 0; // path length
  if (_path.Size()>0)
  {
    Vector3 beg = _path[0].GetPathPos(0,true,0);
    Vector3 end = _path.Last().GetPathPos(0,true,0);
    directDist = beg.Distance(end);
    Vector3 prev = beg;
    for (int i=1; i<_path.Size(); i++)
    {
      Vector3 cur = _path[i].GetPathPos(0,true,0);
      travelDist += prev.Distance(cur);
      prev = cur;
    }
    if (unit->GetUnit())
    {
      LogF(
        "#$$ Path for %d, distance %.2f, travel %.2f, cost %.2f, cover %s",
        unit->GetUnit()->ID(), directDist, travelDist, _path.Last()._cost, coverFound ? "real" : "open"
      );
      GPathStats.Add(unit->GetUnit()->ID(),directDist, travelDist, _path.Last()._cost);
      GPathStats.Report(unit->GetUnit()->ID());
  }
  }
  
  #endif
  
  return true;
}

float OperMap::GetBestPathCost() const
{
  const ASONode *last;
  if (_algorithm->IsDone() && _algorithm->IsFound())
  {
    // path found
    last = _algorithm->GetLastNode();
    // heuristics should be 0 here
  }
  else
  {
    // path not found - use the closest node we have
    last = _algorithm->GetBestNode(this);
  }
  return last->_f;
}

bool OperMap::IsDone() const
{
  return _algorithm ? _algorithm->IsDone() : false;
}

bool OperMap::IsFound() const
{
  return _algorithm ? _algorithm->IsFound() : false;
}

RString OperMap::GetDiagText(bool cover) const
{
  return _algorithm ? _algorithm->GetDiagText(cover) : RString();
}

const OperCover *OperMap::GetCover(OperMapIndex x, OperMapIndex z, int &nCover) const
{
  nCover = 0;

  SPLIT_INDICES(x, z, xLand, zLand, xField, zField);

  const OperMapField *field = GetField(xLand, zLand);
  if (!field) return NULL;

  return field->_operField->GetCover(xField,zField,nCover);
}

const IPaths *OperMap::GetHouse(int houseIndex) const
{
  Object *obj = _houses[houseIndex]._house;
  return obj ? obj->GetIPaths() : NULL;
}

int OperMap::GetHouseIndex(Object *house) const
{
  for (int h=0; h<_houses.Size(); h++)
  {
    const OperHouse &item = _houses[h];
    if (item._house == house) return h;
  }
  Fail("House not found in the list");
  return -1;
}

#endif //_ENABLE_AI


#if _ENABLE_AI

Vector3 OperInfo::GetPathPos(float combatHeight, bool soldier, float vehWidth) const
{
  Vector3 pos;
  if (_house)
  {
    const IPaths *paths = _house->GetIPaths();
    if (!paths)
    {
      Fail("Wrong building");
      return VZero;
    }
    pos = paths->GetPosition(_housePos);
    pos[1] += combatHeight;
  }
  else if (_road.IsValid())
  {
    pos = _road.Position(soldier, vehWidth);
    pos[1] += combatHeight;
  }
  else
  {
    // not house, not road - must be a grid (terrain)
    float operItemGrid = OperItemGrid;
    pos[0] = _x * operItemGrid + 0.5f * operItemGrid;
    pos[2] = _z * operItemGrid + 0.5f * operItemGrid;
    pos[1] = GLandscape->SurfaceYAboveWaterNoWaves(pos[0], pos[2]) + combatHeight;
  }
  return pos;
}

/*!
\patch 5143 Date 3/21/2007 by Ondra
- Fixed: slightly improved AI path finding in a non-uniform environment.
*/
void OperMap::ResultPath(AIBrain *unit, bool prepend, Vector3Par prependPos)
{
#if DIAG_PATH || DIAG
// Original path
LogF("Raw path for %s, length = %d", (const char *)unit->GetDebugName(), _path.Size());
for (int i=0; i<_path.Size(); i++)
{
  OperInfo &info = _path[i];
  if (info._house)
    LogF(" %d: house 0x%x %d, cost %.2f", i, (Object *)info._house, info._housePos, info._cost);
  else if (info._road.IsValid())
    LogF(" %d: road 0x%x %d, cost %.2f", i, info._road.GetRoad(), info._road.GetNodeIndex(), info._cost);
  else
    LogF(" %d: %d, %d, cost %.2f", i, info._x, info._z, info._cost);
}
#endif

  Path &path = unit->GetPath();

  path.Clear();
  int n = _path.Size();
  Assert(n >= 1);
  if (n < 1) return;

  // if the path leads into a cover, mark it as such
  path.SetCover(_pathToCover,_pathToOpenCover);
  // make sure vehicle is marked as not there yet
  EntityAI *veh = unit->GetVehicle();
  veh->FormationChanged();

  float combatHeight = veh->GetCombatHeight();
  bool soldier = unit->IsFreeSoldier();
  float vehWidth = 0;
  if (!soldier)
  {
    LODShape *shape = veh->GetShape();
    if (shape) vehWidth = shape->Max().X() - shape->Min().X();
  }

  int lastIndex = 0;
  // how much cost was inserted by prepending or inserting before the current node
  float costInserted = 0;
  Vector3 lastPos = _path[0].GetPathPos(combatHeight, soldier, vehWidth);
  
  // we need n>=2 to be able to compute first segment cost, path with n<2 is invalid anyway
  if (prepend && n>=2)
  {
    // prepending - used when starting point cost was too high (unaccessible)
    float distInserted = prependPos.Distance(lastPos);
    // avoid inserting segment which is not substantial enough
    if (distInserted>OperItemGrid*0.25f)
    {
      // compute speed on the first segment of the planned path
      Vector3 pos = _path[1].GetPathPos(combatHeight, soldier, vehWidth);
      float dist2 = pos.Distance2(lastPos);
      float costPerM = dist2>0 ? (_path[1]._cost - _path[0]._cost)*InvSqrt(dist2) : 1.0f;
      costInserted = costPerM*distInserted;
      int index = path.Add();
      OperInfoResult &info = path[index];
      info._pos = prependPos;
      info._cost = 0; 
      info._type = NTGrid;
    }

  }
  float lastCost = costInserted;

  {
    // add start point
    int index = path.Add();
    OperInfoResult &info = path[index];
    info._pos = lastPos;
    info._cost = lastCost; 
    info._type = _path[0]._type;
    if (info._type == NTHouse)
    {
      info._house = _path[0]._house;
      info._index = _path[0]._housePos;
      const IPaths *paths = info._house ? info._house->GetIPaths() : NULL;
      if (paths) info._action = paths->GetPathAction(info._index);
    }
    else if (info._type == NTRoad || info._type == NTBridge)
    {
      info._road = _path[0]._road.GetRoad();
    }
#if 0 && _VBS2
    else if(!_path[0]._road) //check again, if there isn't a road for the first element!
    {
      float size = veh->GetShape() ? veh->GetShape()->GeometrySphere() : 0;
      _path[0]._road = unconst_cast(GLandscape->GetRoadNet()->IsOnRoad(lastPos, size));
      if(_path[0]._road)
        info._type = NTRoad;
    }
#endif
    if (n < 2)
    {
      RptF
      (
        "Error %s: Invalid path - length 1",
        (const char *)unit->GetDebugName()
      );
      return;
    }
  }

  while (lastIndex < n - 1)
  {
    int i = lastIndex + 1;

    int dx = _path[i]._x-_path[i-1]._x;
    int dz = _path[i]._z-_path[i-1]._z;
    // we want to merge only parts of a path which have a similar cost
    float dCost = _path[i]._cost-_path[i-1]._cost;
    while
    (
      i < n - 1 &&
      _path[i]._house == NULL &&      // using building
      _path[i+1]._house == NULL &&
      //_path[i]._clearance == _path[i+1]._clearance && 
      _path[i]._clearance == ClearanceNormal && 
      _path[i]._road.GetRoad() == NULL &&       // using road
      _path[i+1]._road.GetRoad() == NULL &&
      // check direction change
      _path[i+1]._x-_path[i]._x == dx && _path[i+1]._z-_path[i]._z == dz &&
      fabs((_path[i+1]._cost-_path[i]._cost) - dCost)<1e-3f
    )
    {
      i++;
      // check direction to last point
    }
#if DIAG
LogF("    Index down to %d", i);
#endif

#if 0 && _VBS2
    if(!_path[i]._road) //check again, if there isn't a road for the first element!
    {
      float size = veh->GetShape() ? veh->GetShape()->GeometrySphere() : 0;
      _path[i]._road = unconst_cast(GLandscape->GetRoadNet()->IsOnRoad(lastPos, size));
    }
#endif

    lastIndex = i;
    int index = path.Add();
#if DIAG
LogF("  Added point %d", index);
#endif
    OperInfoResult &info = path[index];
    info._pos = _path[i].GetPathPos(combatHeight, soldier, vehWidth);
    info._cost = lastCost = _path[i]._cost + costInserted;
    info._type = _path[i]._type;
    info._clearance = _path[i]._clearance;
    if (info._type == NTHouse)
    {
      info._house = _path[i]._house;
      info._index = _path[i]._housePos;
      const IPaths *paths = info._house ? info._house->GetIPaths() : NULL;
      if (paths) info._action = paths->GetPathAction(info._index);
    }
    else if (info._type == NTRoad || info._type == NTBridge)
    {
      info._road = _path[i]._road.GetRoad();
    }

    lastPos = info._pos;
  }

#if DIAG
LogF("");
#endif
//    float minCostPerM = 1/unit->GetVehicle()->GetType()->GetMaxSpeedMs();

  // TODO: optimizing pass - remove unnecessary turns

  // final pass - limit cost to reasonable value
  if (path.Size()>1)
  {

    float minCostPerM = veh->GetType()->GetMinCost();
    float maxCostPerM = veh->GetType()->GetMinCost() * 20;
    saturateMin(maxCostPerM, 3.6);
    // note: saturateMax was above - but that would select the hight cost of the two
    // we need to move it at higher of speeds maxspeed/20 or  1/3.6 m/s

    Vector3 lastPos = path[0]._pos;
    float lastSrcCost = path[0]._cost;
    float lastDstCost = 0;
    for (int i=1; i<path.Size(); i++)
    {
      Vector3Val pos = path[i]._pos;
      float srcCost = path[i]._cost;
      float dist = pos.Distance(lastPos);

      float diffCost = srcCost-lastSrcCost;

      float minCost = minCostPerM*dist;
      float maxCost = maxCostPerM*dist;

      if (diffCost<minCost) diffCost = minCost;
      else if (diffCost>maxCost)
      {
        if (diffCost>1e5)
        {
          LogF("%s: Cost too high",(const char *)unit->GetDebugName());
        }
        diffCost = maxCost;
      }
      
      lastDstCost = lastDstCost + diffCost;
      path[i]._cost = lastDstCost;
      // 
      lastPos = pos;
      lastSrcCost = srcCost;
      //LogF("  update from %.2f to %.2f",lastSrcCost,lastDstCost);
    }
  }

#if DIAG_PATH
  // Result path
  LogF("Result path for %s", (const char *)unit->GetDebugName());
  for (int i=0; i<path.Size(); i++)
  {
    LogF(
      " %d: %.0f, %.0f, cost %.2f, type: %s",
      i,
      path[i]._pos.X() / OperItemGrid,
      path[i]._pos.Z() / OperItemGrid,
      path[i]._cost,
      ASONodeTypeName(path[i]._type));
  }
#endif  
}

float OperField::GetAge() const
{
  return Glob.time - _lastUsed;
}

///////////////////////////////////////////////////////////////////////////////
// class OperField, OperFieldSimple, OperFieldFull

DEFINE_FAST_ALLOCATOR(OperFieldSimple)
//DEFINE_FAST_ALLOCATOR(OperFieldFull)

#if STORE_OBJ_ID
int OperFieldFull::AddObj(Object *obj)
{
  Assert(!obj->GetObjectId().IsNull());
  int index = _objs.FindKey(obj->GetObjectId());
  if (index>=0) return 0;
  if (_objs.Size()>=SHRT_MAX)
  {
    RptF("Too many objects in one grid in %d,%d",_x,_z);
    return -1;
  }
  return _objs.Add(obj);
}
#endif

/// return which of the two is more important
/**
This function was created to give roads less important than AvoidXXX areas without having to reorder the enum
Reordering enum is difficult, because it is used for array indexing
*/
static inline bool MoreImportant(OperItemType t1, OperItemType t2)
{
  static const signed char importance[]={
    0, // OITNormal,
    2, // OITAvoidBush,
    3, // OITAvoidTree,
    4, // OITAvoid,
    5, // OITWater,
    6, // OITSpaceRoad,
    1, // OITRoad - least important, 
    7, // OITSpaceBush,
    8, // OITSpaceHardBush,
    9, // OITSpaceTree,
    10, // OITSpace,
    11, // OITRoadForced,
  };
  COMPILETIME_COMPARE(lenof(importance),NOperItemType);
  return importance[t1]>importance[t2];
}

/*!
\patch 5088 Date 11/15/2006 by Jirka
- Fixed: AI sometimes planned path through walls
*/

void OperFieldFull::Rasterize(
  Object *obj, const Vector3 *minMax, const Vector3 *obbRect,
  OperItemType type, float xResize, float zResize, bool soldier,
  bool checkHeight, float minSurf, Clearance clearance
)
{
  // checkHeight - partial rasterization, check for each field if GLandscape->SurfaceY >= minSurf

  PROFILE_SCOPE_EX(pthMR, aiPath);

  // bottom face of the minMax block
  Point3 rect[4];

  if (!obbRect)
  {
    rect[0] = minMax[0];
    rect[0][0] -= xResize;
    rect[0][2] -= zResize;
    rect[2] = minMax[1];
    rect[2][0] += xResize;
    rect[2][2] += zResize;
    rect[1].Init();
    rect[1][0] = rect[0][0];
    rect[1][1] = rect[0][1];
    rect[1][2] = rect[2][2];
    rect[3].Init();
    rect[3][0] = rect[2][0];
    rect[3][1] = rect[0][1];
    rect[3][2] = rect[0][2];
  }
  else
  {    
    rect[0] = obbRect[0];
    rect[1] = obbRect[1];
    rect[2] = obbRect[2];
    rect[3] = obbRect[3];

    rect[2][1] = minMax[1][1];
  }

  // convert to the world space
  for (int j=0; j<4; j++)
  {
    rect[j] = obj->FutureVisualState().PositionModelToWorld(rect[j]);
    rect[j][0]*=InvOperItemGrid;
    rect[j][2]*=InvOperItemGrid;
  }

  // find hull in the world space, find the vertex with the minimal z
  float xMin = rect[0][0];
  float xMax = rect[0][0];
  int jzMin = 0;
  float zMin = rect[0][2];
  float zMax = rect[0][2];
  for ( int j=1; j<4; j++)
  {
    if (rect[j][0] < xMin)
      xMin = rect[j][0];
    else if (rect[j][0] > xMax)
      xMax = rect[j][0];
    if (rect[j][2] < zMin)
    {
      zMin = rect[j][2];
      jzMin = j;
    }
    else if (rect[j][2] > zMax)
      zMax = rect[j][2];
  }
  // check if the hull falls into rasterized field
  if (xMin >= (_x + 1) * OperItemRange)
    return;
  if (xMax < _x * OperItemRange)
    return;
  if (zMin >= (_z + 1) * OperItemRange)
    return;
  if (zMax < _z * OperItemRange)
    return;

  int baseX = _x * OperItemRange;
  int baseZ = _z * OperItemRange;

  MapCoord zzMin = toIntFloor(zMin);
  MapCoord zzMax = toIntFloor(zMax);
  
  // "left" and "right" edges
  int jLCur, jRCur, jLNext, jRNext;
  jLCur = jRCur = jzMin;
  jLNext = (jLCur - 1) & 3;
  jRNext = (jRCur + 1) & 3;

  // current x position on the "left" and "right" edge
  float xL = rect[jLCur][0];
  float xR = rect[jRCur][0];

  bool bEnd = false;

  // remaining z to the end of the row
  float dzL = zzMin + 1 - zMin;
  float dzR = dzL;

  // calculate the grade of the "left" and "right" edge
  float dxL, dxR;
  {
    float denomL = (rect[jLNext][2] - rect[jLCur][2]);
    float denomR = (rect[jRNext][2] - rect[jRCur][2]);
    float invDL = denomL!=0 ? 1/denomL : 1e10;
    float invDR = denomR!=0 ? 1/denomR : 1e10;
    dxL = (rect[jLNext][0] - rect[jLCur][0]) * invDL;
    dxR = (rect[jRNext][0] - rect[jRCur][0]) * invDR;
  }

#if STORE_OBJ_ID
  int setObjId = -1;
  // we need to remember object for some field types
  if (soldier)
  {
    switch (type)
    {
      case OITSpaceBush:
      case OITSpaceHardBush:
      case OITSpaceTree:
      case OITSpace:
        setObjId = AddObj(obj);
        break;
    }
  }
#endif

  for (int j=zzMin; j<=zzMax; j++)
  {
    // min and max value of x on the top of the row
    MapCoord xxMin, xxMax;
    if (xL < xR)
    {
      xxMin = toIntFloor(xL);
      xxMax = toIntFloor(xR);
    }
    else
    {
      xxMax = toIntFloor(xL);
      xxMin = toIntFloor(xR);
    }

    // "left" edge processing - go through all vertices in the current row
    float zzL = toIntFloor(rect[jLNext][2]); // in which row is the next vertex
    while (!bEnd && zzL == j)
    {
      // move to the next edge
      jLCur--;
      jLCur &= 3;
      jLNext--;
      jLNext &= 3;
      // update the xL and dzL
      xL = rect[jLCur][0];
      dzL = j + 1 - rect[jLCur][2]; // how much of z remains to the bottom of the row

      // extend the min .. max x area if needed
      MapCoord xx = toIntFloor(xL);
      if (xx < xxMin)
        xxMin = xx;
      else if (xx > xxMax)
        xxMax = xx;

      // update the cycle conditions
      bEnd = jLNext == jRCur; // met the "right" edge
      zzL = toIntFloor(rect[jLNext][2]); // row of the next vertex 
    }
    // handle remain to the bottom of the row
    if (!bEnd)
    {
      // new grade of the "left" edge
      float denomL = (rect[jLNext][2] - rect[jLCur][2]);
      float invDL = denomL!=0 ? 1/denomL : 1e10;
      dxL = (rect[jLNext][0] - rect[jLCur][0]) * invDL;

      // update the xL to the bottom of the row
      xL += dxL * dzL;

      // extend the min .. max x area if needed
      MapCoord xx = toIntFloor(xL);
      if (xx < xxMin)
        xxMin = xx;
      else if (xx > xxMax)
        xxMax = xx;
    }

    // "right" edge processing - go through all vertices in the current row
    float zzR = toIntFloor(rect[jRNext][2]);
    while (!bEnd && zzR == j)
    {
      // move to the next edge
      jRCur++;
      jRCur &= 3;
      jRNext++;
      jRNext &= 3;
      // update the xR and dzR
      xR = rect[jRCur][0];
      dzR = j + 1 - rect[jRCur][2]; // how much of z remains to the bottom of the row

      // extend the min .. max x area if needed
      MapCoord xx = toIntFloor(xR);
      if (xx < xxMin)
        xxMin = xx;
      else if (xx > xxMax)
        xxMax = xx;

      // update the cycle conditions
      bEnd = jRNext == jLCur; // met the "left" edge
      zzR = toIntFloor(rect[jRNext][2]); // row of the next vertex
    }
    // handle remain to the bottom of the row
    if (!bEnd)
    {
      // new grade of the "left" edge
      float denomR = (rect[jRNext][2] - rect[jRCur][2]);
      float invDR = denomR!=0 ? 1/denomR : 1e10;
      dxR = (rect[jRNext][0] - rect[jRCur][0]) * invDR;

      // update the xR to the bottom of the row
      xR += dxR * dzR;

      // extend the min .. max x area if needed
      MapCoord xx = toIntFloor(xR);
      if (xx < xxMin)
        xxMin = xx;
      else if (xx > xxMax)
        xxMax = xx;
    }

    // prepare dzL, dzR for the next row
    dzL = dzR = 1.0f;

    // fill the area inside the field occupied by the face
    int z0 = j - baseZ;
    if (z0 < 0 || z0 >= OperItemRange)
      continue;

    int x0 = xxMin - baseX;
    int x1 = xxMax - baseX;
    saturateMax( x0, 0 );
    saturateMin( x1, OperItemRange-1 );

    OperItem *itemsZ0=_items[z0];

//Log("Row %d (%d), columns %d - %d (%d - %d)", j, z0, xxMin, xxMax, x0, x1);

    // world coordinate of the field center
    float zCenter = (j + 0.5) * OperItemGrid;

    if (soldier)
      for (int k=x0; k<=x1; k++)
      {
        if (checkHeight)
        {
          // world coordinates of the field center
          float xCenter = (baseX + k + 0.5) * OperItemGrid;
          if (GLandscape->SurfaceY(xCenter, zCenter) < minSurf) continue;
        }
        if (itemsZ0[k].GetClearance()>clearance) itemsZ0[k].SetClearance(clearance);
        if (MoreImportant(type, itemsZ0[k].GetTypeSoldier()))
        {
          itemsZ0[k].SetTypeSoldier(type);
#if STORE_OBJ_ID
          if (setObjId>=0)
          {
            itemsZ0[k]._objSoldier = setObjId;
          }
#endif
        }
      }
    else
      for (int k=x0; k<=x1; k++)
      {
        if (checkHeight)
        {
          // world coordinates of the field center
          float xCenter = (baseX + k + 0.5) * OperItemGrid;
          if (GLandscape->SurfaceY(xCenter, zCenter) < minSurf) continue;
        }
        if (itemsZ0[k].GetClearance()>clearance) itemsZ0[k].SetClearance(clearance);
        if (MoreImportant(type, itemsZ0[k].GetType()))
        {
          itemsZ0[k].SetType(type);
        }
      }
  }
}

/// check if point is inside of given LandGrid field
/**
TODO: include 1 OperItemGrid border on all sides
*/
static bool InsideOperField(Vector3Par pt, int x, int z, int &xx, int &zz)
{
  int xog = toIntFloor(pt.X()*InvOperItemGrid);
  int zog = toIntFloor(pt.Z()*InvOperItemGrid);
  xx = xog-x*OperItemRange;
  zz = zog-z*OperItemRange;
  #if 0 //_ENABLE_CHEATS
    float xg = xog*InvOperItemRange;
    float zg = zog*InvOperItemRange;
    (void)xg,(void)zg;
  #endif
  return xx>=0 && xx<OperItemRange && zz>=0 && zz<OperItemRange;
}

/// check if given vector is mostly vertical
static inline bool IsVertical(Vector3Par up)
{
  // max. allowed angle is 10 degree
  const float cos10deg = 0.98480775301220805937f;
  return up.Y()>cos10deg;
}

extern const float CoverHeightCorner[];
extern const float CoverHeightEdge[];

static inline CoverType AdjustCoverHeight(float &ptY, float surfY)
{
  static const CoverType typeBasedOnHeight[3]={CoverLeftCornerLow,CoverLeftCornerMiddle,CoverLeftCornerHigh};
  // find highest lower
  // list is ordered lowest to highest, we need to check highest first
  for (int i=3; --i>=0; )
  {
    if (ptY>surfY+CoverHeightCorner[i])
    {
      ptY = surfY+CoverHeightCorner[i];
      return typeBasedOnHeight[i];
    }
  }
  // no cover possible - do not touch input Y, can be used for diagnostics
  return CoverNone;
}

static inline CoverType AdjustCoverHeightEdge(float &ptY, float surfY)
{
  if (ptY>surfY+CoverHeightEdge[1]+0.20f)
  {
    // "wall" style cover (no good fire angle, but often better than nothing)
    ptY = surfY+CoverHeightEdge[1]+0.20f;
    return CoverWall;
  }
  static const CoverType typeBasedOnHeight[2]={CoverEdgeLow,CoverEdgeMiddle};
  // find highest lower
  // list is ordered lowest to highest, we need to check highest first
  for (int i=2; --i>=0; )
  {
    if (ptY>surfY+CoverHeightEdge[i]-0.15f)
    {
      if (ptY>surfY+CoverHeightEdge[i]+0.25f)
      {
        return CoverNone;
      }
      ptY = surfY+CoverHeightEdge[i];
      return typeBasedOnHeight[i];
    }
  }
  // no cover possible - do not touch input Y, can be used for diagnostics
  return CoverNone;
}

// reserve at balk
const float OBJ_SPACE         = 1.0f;
const float OBJ_AVOID         = 8.0f;
//const float ROAD_SPACE      =   3.0f;
const float ROAD_SPACE        = 0.0f;
const float OBJ_HEIGHT        = 2.5f;

const float OBJ_SPACE_SOLDIER = 0.2f;
const float OBJ_AVOID_SOLDIER = 1.0f;
const float ROAD_SPACE_SOLDIER  = 0.0f;
const float OBJ_HEIGHT_SOLDIER  = 0.5f;
const float OBJ_HEIGHT_SOLDIER_STAND = 1.7f;

const float OBJ_BELOW_LIMIT = 0.5f;

#define RASTERIZE_BRIDGES 0

#define ALLOW_OBB 1

/*!
\patch 1.53 Date 4/26/2002 by Ondra
- New: Bridge supported as part of road network.
\patch 5117 Date 1/12/2007 by Jirka
- Fixed: AI soldiers sometimes went through handrail of bridge
\patch 5143 Date 3/21/2007 by Ondra
- Fixed: Tides are now considered for AI path finding.
*/
void OperFieldFull::CreateField(int mask)
{
  _pathingReady = false;
  
  PROFILE_SCOPE_EX(pthMF, aiPath);

  OperItem *item;
  int ix, iz, i;
  const float maxWater = -0.5f + GLandscape->GetSeaLevel();
  for (iz=0; iz<OperItemRange; iz++) for (ix=0; ix<OperItemRange; ix++)
  {
    item = &_items[iz][ix];
    item->_content=0;
    item->SetType(OITNormal);
    item->SetTypeSoldier(OITNormal);
    item->SetHouseExitPresent(false);
    item->SetClearance(ClearanceNormal);
#if STORE_OBJ_ID
    item->_objSoldier = -1;
#endif
    // deep water should be marked as OITSpace for soldiers
    // get coordinates in terrain
    if (mask&MASK_USE_BUFFER)
    {
      // sample center of the grid squares
      float xt = _x * LandGrid + ix*OperItemGrid + OperItemGrid*0.5f;
      float zt = _z * LandGrid + iz*OperItemGrid + OperItemGrid*0.5f;
      // note: all samples are in the same grid
      // SurfaceY may be optimized for constant grid
      float yt = GLandscape->SurfaceY(xt,zt);
      if (yt<maxWater)
      {
        item->SetTypeSoldier(OITWater);
        item->SetType(OITWater);
      }

    }

  }

  Object *obj;
  // avoid
  for (ix=_x-1; ix<=_x+1;ix++)
    for (iz=_z-1; iz<=_z+1;iz++) 
      if (InRange(ix, iz))
      {
        MAKE_OBJECT_LIST_XXL(list, GLandscape, ix, iz)
        for (i=0;i<list.Size();i++)
        {
          obj = list[i];

          if (obj == NULL) continue;
          LODShape *shape=obj->GetShape();
          if (!shape) continue;
          //if (obj->IsPassable()) continue;
          if (!obj->HasGeometry()) continue;
          if (obj->Static())
          {
            if ((mask & MASK_AVOID_OBJECTS)== 0) continue;
          }
          else if (obj->GetType() == TypeVehicle )
          {
            if( (mask & MASK_AVOID_VEHICLES) == 0) continue;
          }
          //if (obj->GetType() == Network) continue;
#if _ARMA3_TERRAIN_DECALS
          if (obj->GetType() == LandDecal) continue;
#endif     
        
          // test bounding sphere
          Object::ProtectedVisualState<const ObjectVisualState> pvs = obj->FutureVisualStateScope();

          Vector3Val center = pvs->Position();
          float radius = shape->BoundingSphere() * pvs->Scale();
          if (center.X() + radius <= _x * LandGrid) continue;
          if (center.X() - radius >= (_x + 1) * LandGrid) continue;
          if (center.Z() + radius <= _z * LandGrid) continue;
          if (center.Z() - radius >= (_z + 1) * LandGrid) continue;

          
          #if 0 //_ENABLE_REPORT
            if (strstr(shape->GetName(),"sloup_vn_drat"))
            {
              __asm nop;
            }
          #endif
          OperItemType type = OITAvoid;
          if (obj->IsPassable() ) type = OITAvoidBush;
          else if (obj->GetDestructType() == DestructTree || obj->GetDestructType() == DestructTent) type = OITAvoidTree;
          // render all convex components
          /* do not animate, the map is static anyway
          int geometry = shape->FindGeometryLevel();
          if (geometry >= 0) obj->Animate(geometry, false, obj);
          */
          const ConvexComponents &ccs=shape->GetGeomComponents();
          for( int c=0; c<ccs.Size(); c++ )
          {
            const ConvexComponent *cc=ccs.Get(c);
            Vector3 center = 0.5 * (cc->Min() + cc->Max());
            // bottom center point of the object
            Vector3 aveDown(center.X(),cc->Min().Y(),center.Z());
            Vector3Val ave=obj->FutureVisualState().PositionModelToWorld(aveDown); // ave.Y is given as the minimal y of the object

            // FIX: for large components, partial rasterization is needed
            // to avoid rasterizing where component is high above terrain


            Vector3 pt00 = obj->FutureVisualState().PositionModelToWorld(Vector3(cc->Min().X(), center.Y(), cc->Min().Z()));
            Vector3 pt01 = obj->FutureVisualState().PositionModelToWorld(Vector3(cc->Min().X(), center.Y(), cc->Max().Z()));
            Vector3 pt10 = obj->FutureVisualState().PositionModelToWorld(Vector3(cc->Max().X(), center.Y(), cc->Min().Z()));
            Vector3 pt11 = obj->FutureVisualState().PositionModelToWorld(Vector3(cc->Max().X(), center.Y(), cc->Max().Z()));

            // landscape height in the corners of component
            float surf1 = GLandscape->SurfaceY(pt00.X(), pt00.Z());
            float surf2 = GLandscape->SurfaceY(pt01.X(), pt01.Z());
            float surf3 = GLandscape->SurfaceY(pt10.X(), pt10.Z());
            float surf4 = GLandscape->SurfaceY(pt11.X(), pt11.Z());
 
            Vector3 aveTop(center.X(),cc->Max().Y(),center.Z());
            float topLimit=obj->FutureVisualState().PositionModelToWorld(aveTop).Y() + OBJ_BELOW_LIMIT; // topLimit is given as the maximum y of the object plus OBJ_BELOW_LIMIT
            // if top center of component is more that OBJ_BELOW_LIMIT below ground do not rasterize it.
            if ((topLimit < surf1) && (topLimit  < surf2) && (topLimit < surf3) && (topLimit < surf4)) continue;

            // rasterize for both soldiers and vehicles
            // if the component y is below surface + maxHeight, process rasterization
            // ave.Y() <= surf + OBJ_HEIGHT_SOLDIER => surf >= ave.Y() - OBJ_HEIGHT_SOLDIER
            float minSurf = ave.Y() - OBJ_HEIGHT_SOLDIER;
            if (surf1 >= minSurf || surf2 >= minSurf || surf3 >= minSurf || surf4 >= minSurf)
            { // at least some corner is near surface
              // checkHeight - part of the component is high, check for each field if surf >= ave.Y() - OBJ_HEIGHT_SOLDIER
              bool checkHeight = surf1 < minSurf || surf2 < minSurf || surf3 < minSurf || surf4 < minSurf;
              // make sure the "avoid" border is thick enough when non-zero
              // (note: soldiers prefer the avoid border when in combat)
              float avoid = OBJ_AVOID_SOLDIER>0 ? floatMax(OBJ_AVOID_SOLDIER,OperItemGrid) : 0;
              Vector3 *rectPtr = 0;
#if ALLOW_OBB
              Vector3 rect[4];
              cc->CalcOBB(rect, avoid, avoid);
              rectPtr = rect;
#endif

              Rasterize(obj, cc->MinMax(), rectPtr, type, avoid, avoid, true, checkHeight, minSurf, ClearanceNormal);
            }

            minSurf = ave.Y() - OBJ_HEIGHT;
            if (surf1 >= minSurf || surf2 >= minSurf || surf3 >= minSurf || surf4 >= minSurf)
            {
              // at least some corner is near surface
              bool checkHeight = surf1 < minSurf || surf2 < minSurf || surf3 < minSurf || surf4 < minSurf;
              Rasterize(obj, cc->MinMax(), NULL, type, OBJ_AVOID, OBJ_AVOID, false, checkHeight, minSurf, ClearanceNormal);
            }
          }
/*
          if (geometry >= 0) obj->Deanimate(geometry, false);
*/
        }
      }

  // space
  for (ix=_x-1; ix<=_x+1;ix++)
    for (iz=_z-1; iz<=_z+1;iz++) 
      if (InRange(ix, iz))
      {
        MAKE_OBJECT_LIST_XXL(list, GLandscape, ix, iz)
        for (i=0;i<list.Size();i++)
        {
          obj = list[i];
          if (obj == NULL) continue;

          LODShape *shape=obj->GetShape();
          if (!shape) continue;

          if (obj->Static())
          {
            if ((mask & MASK_AVOID_OBJECTS)== 0) continue;
          }
          else if (obj->GetType() == TypeVehicle )
          {
            if( (mask & MASK_AVOID_VEHICLES) == 0) continue;
          }

          // test bounding sphere
          Object::ProtectedVisualState<const ObjectVisualState> pvs = obj->FutureVisualStateScope();

          Vector3Val center = pvs->Position();
          float radius = shape->BoundingSphere() * pvs->Scale();
/*
if (stricmp(shape->GetName(), "ca\\misc\\pytle.p3d") == 0)
{
  LogF("*** Object %s", cc_cast(obj->GetDebugName()));
  LogF(" - rasterizing %d, %d", _x, _z);
  LogF(" - found in square %d, %d - expected in square %d, %d", ix, iz, toIntFloor(center.X() * InvLandGrid), toIntFloor(center.Z() * InvLandGrid));

  LogF("   - position %.1f, %.1f", center.X(), center.Z());
  LogF("   - radius %.1f", radius);
  LogF("   - square x (%.1f ... %.1f), z (%.1f ... %.1f)",
    _x * LandGrid, (_x + 1) * LandGrid, _z * LandGrid, (_z + 1) * LandGrid);
}
*/
          if (center.X() + radius <= _x * LandGrid) continue;
          if (center.X() - radius >= (_x + 1) * LandGrid) continue;
          if (center.Z() + radius <= _z * LandGrid) continue;
          if (center.Z() - radius >= (_z + 1) * LandGrid) continue;

          OperItemType type = OITSpace;
          OperItemType typeHigh = OITSpace;
          bool canLimitClearance = obj->CanLimitClearance();
          float bottomOfHigh = 1e10; // where typeHigh should start
          float forceXBorder = 0;
          if (obj->GetType() == Network)
          {
            // note: roads do not have any components

            if (!obj->HasGeometry() || shape->FindGeometryLevel()<0 || shape->GeometryLevel()->NFaces()==0)
            {
              Rasterize(obj, shape->MinMax(), NULL, OITRoad, ROAD_SPACE_SOLDIER, ROAD_SPACE_SOLDIER, true, false, 0, ClearanceNormal);
              Rasterize(obj, shape->MinMax(), NULL, OITRoad, ROAD_SPACE, ROAD_SPACE, false, false, 0, ClearanceNormal);
              continue;
            }
#if RASTERIZE_BRIDGES
            // if there is geometry, there may be some components as well
            // if it has components, it is probably a bridge
            float width = shape->Max().X()-shape->Min().X();
            // leave only minimal area around roadway center
            float xBorder = (width - 1.0)*0.5f;
            float zBorder = OBJ_SPACE+1.5;
            saturateMin(xBorder,6);

            Rasterize(obj, shape->MinMax(), OITRoadForced, -xBorder, zBorder, true, false, 0, ClearanceNormal);
            Rasterize(obj, shape->MinMax(), OITRoadForced, -xBorder, zBorder, false, false, 0, ClearanceNormal);
            forceXBorder = 5.0;

            type = OITSpaceRoad;
            Shape *road = shape->RoadwayLevel();
            if (road) bottomOfHigh = road->Max().Y();
#else
            if (obj->IsPassable() ) type = OITSpaceBush;
            else if (obj->GetDestructType() == DestructTree  || obj->GetDestructType() == DestructTent)
            {
              if (obj->GetMass() < 200)
                type = OITSpaceHardBush;
              else
                type = OITSpaceTree;
            }
#endif
          }
#if _ARMA3_TERRAIN_DECALS
          else if (obj->GetType() == LandDecal) 
          {
            continue;
          }
#endif
          else
          {
            if (!obj->HasGeometry()) continue;

            if (obj->IsPassable() ) type = OITSpaceBush;
            else if (obj->GetDestructType() == DestructTree || obj->GetDestructType() == DestructTent)
            {
              if (obj->GetMass() < 200)
                type = OITSpaceHardBush;
              else
                type = OITSpaceTree;
            }
          }

#if _ENABLE_CHEATS
          #if 0
          int startCC = -1;
          int endCC = -1;
          if (obj->ID()==7) // green sea HouseV_1T + other house
          {
            __asm nop;
            startCC = 3;
            endCC = 3;
          }
          if (obj->ID()==12) // green sea HouseV_1T + other house
          {
            __asm nop;
            startCC = 0;  // 7: 2..3
            endCC = INT_MAX;
          }
          if (obj->ID()==222965) // Chernarus - housev2_03b
          {
            __asm nop;
            startCC = 0;
            endCC = INT_MAX;
          }
          if (obj->ID()==222966) // Chernarus - housev2_04
          {
            // components 13 and 14 interesting
            __asm nop;
            startCC = 13;
            endCC = 14;
          }
          #else
          const int startCC = -1;
          const int endCC = -1;
          #endif
#endif
        
          // render all convex components
          const ConvexComponents &ccs=shape->GetGeomComponents();
          for( int c=0; c<ccs.Size(); c++ )
          {
            const ConvexComponent *cc=ccs.Get(c);
            #if _ENABLE_CHEATS
            if (cc->GetNameIndex()>=startCC && cc->GetNameIndex()<=endCC)
            {
              __asm nop;
            }
            #endif
            Vector3 center = 0.5f * (cc->Min() + cc->Max());
            // bottom center point of the object
            Vector3 aveDown(center.X(),cc->Min().Y(),center.Z());
            Vector3Val ave=obj->FutureVisualState().PositionModelToWorld(aveDown);

            // y coordinate is used for cover only
            // adjust it to indicate prone, crouch, stand
            // BoundingCenter is placed at original (0,0,0)
            
            Vector3 pt00 = obj->FutureVisualState().PositionModelToWorld(Vector3(cc->Min().X(), center.Y(), cc->Min().Z()));
            Vector3 pt01 = obj->FutureVisualState().PositionModelToWorld(Vector3(cc->Min().X(), center.Y(), cc->Max().Z()));
            Vector3 pt10 = obj->FutureVisualState().PositionModelToWorld(Vector3(cc->Max().X(), center.Y(), cc->Min().Z()));
            Vector3 pt11 = obj->FutureVisualState().PositionModelToWorld(Vector3(cc->Max().X(), center.Y(), cc->Max().Z()));

            Vector3 aveTop(center.X(),cc->Max().Y(),center.Z());
            float topLimit=obj->FutureVisualState().PositionModelToWorld(aveTop).Y() + OBJ_BELOW_LIMIT; // topLimit is given as the maximum y of the object plus OBJ_BELOW_LIMIT

            // landscape height in the corners of component
            float surf00 = GLandscape->SurfaceY(pt00.X(), pt00.Z());
            float surf01 = GLandscape->SurfaceY(pt01.X(), pt01.Z());
            float surf10 = GLandscape->SurfaceY(pt10.X(), pt10.Z());
            float surf11 = GLandscape->SurfaceY(pt11.X(), pt11.Z());

            // rasterize for both soldiers and vehicles
            OperItemType cType = type;
            float fXBorder = forceXBorder;
            float maxBorder = 100;
            /**/
            if (cc->Max().Y()>bottomOfHigh+OBJ_HEIGHT)
            {
              cType = typeHigh;
              fXBorder = 0;
              //LogF("%s: high component %d",shape->Name(),c);
              maxBorder = 0.2;
            }
            /**/
            float minSurfSoldier = ave.Y() - OBJ_HEIGHT_SOLDIER;
            float minSurfSoldierStand = ave.Y() - OBJ_HEIGHT_SOLDIER_STAND;
            float minSurfXY = floatMin(surf00,surf01,surf10,surf11);
            float maxSurfXY = floatMax(surf00,surf01,surf10,surf11);
            if (maxSurfXY>=minSurfSoldierStand  && topLimit > minSurfXY)
            {
              float xRes = floatMin(maxBorder,floatMax(forceXBorder,OBJ_SPACE_SOLDIER));
              float zRes = floatMin(maxBorder,OBJ_SPACE_SOLDIER);
              Vector3 *rectPtr = 0;
              #if ALLOW_OBB
                Vector3 rect[4];
                cc->CalcOBB(rect, xRes, zRes);
                rectPtr = rect;
              #endif
              if (maxSurfXY >= minSurfSoldier)
              { // obstacle even for a soldier
                bool checkHeight = minSurfXY < minSurfSoldier;
                Rasterize(obj, cc->MinMax(), rectPtr, cType, xRes, zRes, true, checkHeight, minSurfSoldier, ClearanceNormal);
                // where height did not pass, we still may have a clearance limit to rasterize
                if (checkHeight && canLimitClearance)
                {
                  Rasterize(obj, cc->MinMax(), rectPtr, OITNormal, xRes, zRes, true, checkHeight, minSurfSoldierStand, ClearanceLow);
                }
              }
              else if (canLimitClearance)
              {
                // obstacle for a vehicle, but not for a soldier (low clearance allowed)
                bool checkHeight = minSurfXY < minSurfSoldierStand;
                Rasterize(obj, cc->MinMax(), rectPtr, OITNormal, xRes, zRes, true, checkHeight, minSurfSoldierStand, ClearanceLow);
              }
              }
            float minSurfVehicle = ave.Y() - OBJ_HEIGHT;
            if ((type == OITSpaceRoad || maxSurfXY >= minSurfVehicle) && topLimit  > minSurfXY)
            {
              bool checkHeight = type != OITSpaceRoad && minSurfXY < minSurfVehicle;
              Rasterize
              (
                obj, cc->MinMax(), NULL, cType,
                floatMin(maxBorder,floatMax(forceXBorder,OBJ_SPACE)),
                floatMin(maxBorder, OBJ_SPACE), false, checkHeight, minSurfVehicle, ClearanceNormal
              );
            }
          }
/*
          if (geometry >= 0) obj->Deanimate(geometry, false);
*/
        }
      }
}

void OperFieldFull::PrepareForPathing()
{
  PROFILE_SCOPE_EX(pthPP, aiPath);

  AssertMainThread();
  
  if (_pathingReady) return;
  
  _pathingReady = true;
  AddCover();
  AddHouses();
  RemoveFalseCover();
  CloseCoverList();
}

void OperFieldFull::ProcessCoverEdge(Vector3Par pt00, Vector3Par pt01, float surf1, float surf2, Vector3Par out, int score)
{
  
  float objHeading = atan2(out.X(),out.Z());
    // if the edge is long enough, insert as many cover candidates as needed
  float dist = pt00.Distance(pt01);
    // preferred distance between wall cover segments
    float minDistBetweenSeg = 4.0f;
    int segments = toIntFloor(dist*(1.0f/minDistBetweenSeg));
    if (segments<1) segments = 1;
    float invSegments = 1.0f/segments;
    Vector3 edgeStep = (pt01-pt00)*invSegments;
    float surfStep = (surf2-surf1)*invSegments;
    
    // create as many edge / surface steps as needed, in the middle of each segment
    Vector3 edge = pt00+edgeStep*0.5f;
    float surf = surf1+surfStep*0.5f;
  float operItemGrid = OperItemGrid;
    for (int i=0; i<segments; i++)
    {
      Vector3 adjEdge = edge; // we need a copy, as we modify it, but edge is used in the loop
      CoverType ct = AdjustCoverHeightEdge(adjEdge[1],surf);
      int xx,zz;
    if (ct>CoverNone && adjEdge[1]>=surf && InsideOperField(adjEdge+out*operItemGrid,_x,_z,xx,zz))
      {
        _cover.Add(OperCover(adjEdge,ct,score,objHeading,xx,zz));
      }
#if DIAG_FALSE_COVER // for diags add cover even when we know it is false
    else if (InsideOperField(adjEdge+out*operItemGrid,_x,_z,xx,zz))
      {
        if (ct==CoverNone) ct = CoverWall;
        _cover.Add(OperCover(adjEdge,ct,score,objHeading,xx,zz));
        _cover.Last()._valid = OperCover::BadHeight;
      }
#endif
      edge += edgeStep;
      surf += surfStep;
    }
  }

void OperFieldFull::ProcessCoverCorner(CoverType ct, Vector3 &pt, float surf1, Vector3 out, int score)
{
  float objHeading = atan2(out.X(),out.Z());
  float operItemGrid = OperItemGrid;
  
  int xx,zz;
  if (ct>CoverNone && pt.Y()>=surf1 && InsideOperField(pt+out*operItemGrid,_x,_z,xx,zz))
  {
    _cover.Add(OperCover(pt,ct,score,objHeading,xx,zz));
  }
#if DIAG_FALSE_COVER // for diags add cover even when we know it is false
  else if (InsideOperField(pt+out*operItemGrid,_x,_z,xx,zz))
  {
    if (ct==CoverNone) ct = CoverLeftCornerHigh;
    _cover.Add(OperCover(pt,ct,score,objHeading,xx,zz));
    _cover.Last()._valid = OperCover::BadHeight;
  }
#endif
}

void OperFieldFull::AddHouses()
{
  for (int ix=_x-1; ix<=_x+1; ix++)
    for (int iz=_z-1; iz<=_z+1; iz++) 
      if (InRange(ix, iz))
      {
        MAKE_OBJECT_LIST_XXL(list, GLandscape, ix, iz)
        for (int i=0;i<list.Size();i++)
        {
          Object *obj = list[i];
          if (obj == NULL) continue;

          LODShape *shape = obj->GetShape();
          if (!shape) continue;

          // contain paths
          const IPaths *house = obj->GetIPaths();
          if (!house) continue;

          // test bounding sphere
          Object::ProtectedVisualState<const ObjectVisualState> pvs = obj->FutureVisualStateScope();

          Vector3Val center = pvs->Position();
          float radius = shape->BoundingSphere() * pvs->Scale();
          if (center.X() + radius <= _x * LandGrid) continue;
          if (center.X() - radius >= (_x + 1) * LandGrid) continue;
          if (center.Z() + radius <= _z * LandGrid) continue;
          if (center.Z() - radius >= (_z + 1) * LandGrid) continue;

          // number of exits
          int exitsCount = house->NExits();
          // number of all house points
          int posCount = house->NPoints();

          // when there are no paths, no need to consider it as as house
          if (exitsCount<=0 && posCount<=0) continue;

          // add all houses to be sure target house is in OperMap even when no exit is in its range
          OperHouse &item = _houses.Append();
          item._house = obj;
          item._exits.Realloc(posCount);

          for (int i=0; i<posCount; i++)
          {
#if _ENABLE_REPORT
            int exit = -1;
            for (int j=0; j<exitsCount; j++)
            {
              if (house->GetExit(j) == i)
              {
                exit = j;
                break;
              }
            }
#endif
            Vector3Val pos = house->GetPosition(i);
            float y = GLandscape->SurfaceY(pos.X(), pos.Z());
            if (fabs(y - pos.Y()) > 0.5)
            {
              static int WrongHeightCount = 0;
              static int MaxUnaccessibleCount = 100;
#if _ENABLE_REPORT
              if (exit >= 0 && (WrongHeightCount++) < MaxUnaccessibleCount)
              {
                void PositionToAA11(Vector3Val pos, char *buffer);
                char buffer[16];
                PositionToAA11(pos, buffer);
                RptF("Warning: Unaccessible (wrong height) house exit %s, exit \"pos%d\", position [%.0f, %.0f] (%s on %s)", cc_cast(obj->GetDebugName()), exit+1, pos.X(), pos.Z(), buffer, cc_cast(GLandscape->GetName()));
              }
#endif
              continue;
            }
            int x = toIntFloor(pos.X() * InvOperItemGrid);
            int z = toIntFloor(pos.Z() * InvOperItemGrid);

            // register the exit
            OperHouseExit &subItem = item._exits.Append();
            subItem._pos = i;
            subItem._x = x;
            subItem._z = z;

            // check if exit is inside field
            int xx = x / OperItemRange;
            int zz = z / OperItemRange;
            if (xx == _x && zz == _z)
            {
              // check the exit accessibility
              OperItem &item = _items[z - zz * OperItemRange][x - xx * OperItemRange];
              item.SetHouseExitPresent(true);
#if _ENABLE_REPORT
              if (exit >= 0)
              {
                static int OccupiedCount = 0;
                static int MaxUnaccessibleCount = 100;

                switch (item.GetTypeSoldier())
                {
                  case OITSpaceHardBush:
                  case OITSpaceTree:
                  case OITSpace:
                  case OITSpaceRoad:
                    {
                      if ((OccupiedCount++) < MaxUnaccessibleCount)
                      {
                        void PositionToAA11(Vector3Val pos, char *buffer);
                        char buffer[16];
                        PositionToAA11(pos, buffer);
                        RptF("Warning: Unaccessible (occupied) house exit, %s, exit \"in%d\", position [%.0f, %.0f] (%s on %s)", cc_cast(obj->GetDebugName()), exit+1, pos.X(), pos.Z(), buffer, cc_cast(GLandscape->GetName()));
                      }
                    }
                    continue;
                }
              }
#endif
            }
          }
          item._exits.Compact();
        }
      }
  _houses.Compact();
}

static inline bool IsAccessibleForSoldier(OperItemType type)
{
  switch (type)
  {
    // case OITSpaceBush:
  case OITWater:
  case OITSpaceHardBush:
  case OITSpaceTree:
  case OITSpace:
  case OITSpaceRoad:
    return false;
  default:
    return true;
  }
}

struct CompareCover
{
  int operator () (const OperCover *a, const OperCover *b) const
  {
    int d = a->_z-b->_z;
    if (d) return d;
    d = a->_x-b->_x;
    return d;
  }
};

void OperFieldFull::CloseCoverList()
{
#if 1
  // remove cover candidates which are placed on obviously invalid positions
  // move any candidates placed on an inaccessible fields to nearest accessible ones
  // this will allow A* to access them while searching
  AutoArray<OperCover, MemAllocLocal<OperCover,64,AllocAlign16> > cover(_cover);
  _cover.Resize(0);
  for (int src=0; src<cover.Size(); src++)
  {
    // check if the field or any of its neighbours is accessible
    //int x = cover[src]._x;
    //int z = cover[src]._z;

    Vector3 dir = Matrix3(MRotationY,-cover[src]._heading).Direction();
    if (cover[src]._type.GetEnumValue()<CoverEdgeMiddle)
    {
      // for each corner cover, we want two entries to be placed, leftmost and rightmost
      // TODO: handle also covers from border areas (1 OperItemGrid overlap)
      // make sure we traverse only the valid part of the field
      // compute basis for "is suitable" evaluation
      Vector3 dirsR[]={
        Vector3(dir.Z(),0,-dir.X())*0.5f,
        Vector3(dir.Z(),0,-dir.X()),
        Vector3(dir.X()+dir.Z(),0,dir.Z()-dir.X())*0.5
      };

      Vector3 dirsL[]={
        Vector3(-dir.Z(),0,dir.X())*0.5f,
        Vector3(-dir.Z(),0,dir.X()),
        Vector3(dir.X()-dir.Z(),0,dir.X()+dir.Z())*0.5
      };

      auto placeEntry = [this, &cover, src] (Vector3Val dir, const char *name, int i) -> bool
      {
        Vector3 pos = cover[src]._pos+dir*(OperItemGrid*H_SQRT2);
        int xx = toIntFloor(pos.X()*InvOperItemGrid)-_x*OperItemRange;
        int zz = toIntFloor(pos.Z()*InvOperItemGrid)-_z*OperItemRange;
        if (xx<0 || xx>=OperItemRange || zz<0 || zz>=OperItemRange) return false;
        #if _ENABLE_CHEATS
          Vector3 ePos(_x*LandGrid + xx*OperItemGrid + 0.5*OperItemGrid, 0, _z*LandGrid + zz*OperItemGrid + 0.5*OperItemGrid);
          ePos[1] = GLandscape->SurfaceY(ePos) + 0.1f*i;
        #endif
        // remember the best accessible field for this cover
        OperItemType type = _items[zz][xx].GetTypeSoldier();
        // cover is always used by soldiers - test accessibility by them
        if (IsAccessibleForSoldier(type))
        {
          // place cover always on some accessible field
          OperCover &c = _cover.Append();
          c = cover[src];
          c._x = xx;
          c._z = zz;
          // for each cover we want to place at most 1 representation for each side
          #if _ENABLE_CHEATS
          if (CHECK_DIAG(DECostMap))
          {
            int handle = 0;
            GDiagsShown.ShowArrow(handle,0,30,Format("Entr %d:%s%d",src,name,i),ePos,VUp,0.6f,Color(0.4,0.8,0.0));
          }
          #endif
          return true;
        }
        else
        {
          #if _ENABLE_CHEATS
          if (CHECK_DIAG(DECostMap))
          {
            int handle = 0;
            GDiagsShown.ShowArrow(handle,0,30,Format("Busy %d:%s%d",src,name,i),ePos,VUp,0.6f,Color(0.8,0.4,0.0));
          }
          #endif
        }
        return false;
      };

      bool placedR = false;
      bool placedL = false;
      bool placed = false;
      for (int i=0; i<lenof(dirsR); i++)
      {
        if (placeEntry(dirsR[i],"R",i)) {placed=placedR=true;break;}
      }
      for (int i=0; i<lenof(dirsL); i++)
      {
        if (placeEntry(dirsL[i],"L",i)) {placed=placedL=true;break;}
      }
      // if some side did not place an entry, place an entry from the middle
      if (!placedL || !placedR)
      {
        if (placeEntry(dir,"C",0)) placed = true;
      }

      #if DIAG_FALSE_COVER
      if (!placed)
      {
        OperCover &c = _cover.Append();
        c = cover[src];
        //LogF("Cover at %d,%d removed as unaccessible",c._x,c._z);
        // leave x, z as is
        c._valid = OperCover::Unaccessible;
      }
      #endif
    }
    else
    {
      // edge or wall type cover - only one entry possible
      Vector3 pos = cover[src]._pos+dir*(OperItemGrid*H_SQRT2);
      int xx = toIntFloor(pos.X()*InvOperItemGrid)-_x*OperItemRange;
      int zz = toIntFloor(pos.Z()*InvOperItemGrid)-_z*OperItemRange;
      if (xx>=0 && xx<OperItemRange && zz>=0 && zz<OperItemRange)
      {
        // remember the best accessible field for this cover
        OperItemType type = _items[zz][xx].GetTypeSoldier();
        // cover is always used by soldiers - test accessibility by them
        if (IsAccessibleForSoldier(type))
        {
          // place cover always on some accessible field
          OperCover &c = _cover.Append();
          c = cover[src];
          c._x = xx;
          c._z = zz;
        }
        #if DIAG_FALSE_COVER
        else
        {
          OperCover &c = _cover.Append();
          c = cover[src];
          // leave x, z as is
          c._valid = OperCover::Unaccessible;
        }
        #endif
      }
    }
  }
#endif
  _cover.Compact();
  // sort all items based on z/x, to make sure items present in one grid are continuous

  QSort(_cover,CompareCover());

  // now build the _coverIndex
  for (OperFieldIndex z(0); z<OperItemRange; z++) for (OperFieldIndex x(0); x<OperItemRange; x++)
  {
    CoverField field = GetCoverField(x, z);
    if (!field.IsEmpty())
    {
      _coverIndex.Set(x,z,field);
    }
  }
}


struct FaceInfo
{
  Offset offset;
  int index;
  int component;
  
  FaceInfo(){}
  FaceInfo(Offset offset, int index, int component):offset(offset),index(index),component(component){}
  
  bool operator == (const FaceInfo &info) const {return offset==info.offset;}
};
TypeIsSimple(FaceInfo)

typedef std::pair<int,int> intPair;
TypeIsSimple(intPair);

struct EdgePosInfo
{
  /// never allow items smaller than a certain limit, they would cause OffTree to be too deep
  /** we perform lookups with 5 cm overlaps anyway - no need to be more precise */
  static float Eps() {return 0.025f;}
  
  Vector3 beg,end;
  int index;
  OffTreeRegion<float> region;
  
  EdgePosInfo(Vector3 beg, Vector3 end, int index)
  :beg(beg),end(end),index(index),region(
    floatMin(beg.X(),end.X()),floatMin(beg.Z(),end.Z()),
    floatMax(beg.X(),end.X()),floatMax(beg.Z(),end.Z())
  )
  {
    region._endX = floatMax(region._endX,region._begX+Eps());
    region._endZ = floatMax(region._endZ,region._begZ+Eps());
  }
  
  const OffTreeRegion<float> &GetRegion() const {return region;}

  bool IsIntersection(const OffTreeRegion<float> &q) const {return region.IsIntersection(q);}
  
  bool operator == (const EdgePosInfo &w){return index==w.index;}
};

TypeIsMovableZeroed(EdgePosInfo)

static inline std::pair<int,int> MakeEdge(int vCur, int vNxt)
{
  return vCur<vNxt ? std::pair<int,int>(vCur,vNxt) : std::pair<int,int>(vNxt,vCur);
}

/** @return (iUnderJ, iAboveJ) */
std::pair<bool,bool> CheckFaceAgainstPlane(const Poly &pi, const Plane &nj, const Shape *geom)
{
  bool iUnderJ = false, iAboveJ = false;
  for (int pii=0; pii<pi.N(); pii++) 
  {
    float dist = nj.Distance(geom->Pos(pi.GetVertex(pii)));
    const float distEps = 0.05f;
    if (dist<-distEps) iUnderJ = true;
    if (dist>+distEps) iAboveJ = true;
  }
  return std::pair<bool,bool>(iUnderJ,iAboveJ);
}


static bool CheckLineOverlapp(Vector3Par i10, Vector3Par i11, Vector3Par j10, Vector3Par j11, float maxDist)        
{
  float distF10ToE = NearestPointDistance(i10,i11,j10);
  float distF11ToE = NearestPointDistance(i10,i11,j11);
  // this handles only one special case of overlap: one edge part of the other one
  return distF10ToE<maxDist && distF11ToE<maxDist;
}

void CoverModel::Build(const LODShape *shape)
{
  const Shape *geom = shape->GeometryLevel();
  if (!geom) return;

  PROFILE_SCOPE_EX(cmBld, aiPath);
  if (PROFILE_SCOPE_NAME(cmBld).IsActive())
  {
    PROFILE_SCOPE_NAME(cmBld).AddMoreInfo(shape->GetName());
  }

  float objSize = Object::CollisionSizeEstimate(shape);
  // compute time penalty based on object radius
  // large penalty for very small objects
  // 0.1 -> 2.5
  // middle penalty for middle objects
  // 0.5 -> 1.0
  // strong preference for very big objects
  // 5.0 -> 0
  float objScore = objSize<0.5f ? InterpolativC(objSize,0.1f,0.5f,2.5f,1.0f) : InterpolativC(objSize,0.5f,5.0f,1.0f,0);
  if (objScore>=2.5f) return;

  const ConvexComponents &ccs=shape->GetGeomComponents();
  {
    PROFILE_SCOPE_DETAIL_EX(cmPEC, aiPath);
    for( int c=0; c<ccs.Size(); c++ )
    {
      const ConvexComponent *cc=ccs.Get(c);
      Vector3 center = 0.5f * (cc->Min() + cc->Max());
      // bottom center point of the object
      Vector3 aveDown(center.X(),cc->Min().Y(),center.Z());
      Vector3 aveTop(center.X(),cc->Max().Y(),center.Z());

      if (cc->Max().Y()-cc->Min().Y()>0.5f) // the component is high enough to provide some cover
      {
        Vector3 pt00(cc->Min().X(), cc->Max().Y(), cc->Min().Z());
        Vector3 pt01(cc->Min().X(), cc->Max().Y(), cc->Max().Z());
        Vector3 pt10(cc->Max().X(), cc->Max().Y(), cc->Min().Z());
        Vector3 pt11(cc->Max().X(), cc->Max().Y(), cc->Max().Z());

        // check component size and penetrability
        // while several small components can compose a good cover
        // much more common situation is such component is not good
        // note: this is especially true when the whole object is small
        float ccSize = floatMax(cc->Max().X()-cc->Min().X(),cc->Max().Z()-cc->Min().Z());

        // TODO: check penetrability as well
        float ccScore = InterpolativC(ccSize,0,0.2f,2.5f,0)+objScore;
        // if the delay is too big, ignore it
        if (ccScore>=2.5f) continue;
        int score = toInt(ccScore*10);
        saturate(score,0,UCHAR_MAX);

#if 1
        // if the component is low and long, insert edge candidates
        // edge candidates can be found along the edges, i.e. 00..01, 01..11, 11..10, 10..00
        Vector3 out00_01(-1,0,0);
        Vector3 out01_11(0,0,1);
        Vector3 out11_10(1,0,0);
        Vector3 out10_00(0,0,-1);
        const float minEdge = 1.5f;
        if (pt00.Distance2(pt01)>Square(minEdge)) _edges.Add(CoverModelEdge(pt00, pt01, out00_01, score, c));
        if (pt01.Distance2(pt11)>Square(minEdge)) _edges.Add(CoverModelEdge(pt01, pt11, out01_11, score, c));
        if (pt11.Distance2(pt10)>Square(minEdge)) _edges.Add(CoverModelEdge(pt11, pt10, out11_10, score, c));
        if (pt10.Distance2(pt00)>Square(minEdge)) _edges.Add(CoverModelEdge(pt10, pt00, out10_00, score, c));
#endif

#if 0
        // for corner covers we need to make sure the point we are checking is already well covered
        // compute heading of "normals" outside of the component corresponding to pt00...pt11
        // compute heading of "out of cover" vectors for corner covers
        Vector3 out00(-H_INVSQRT2,0,-H_INVSQRT2);
        Vector3 out01(-H_INVSQRT2,0,+H_INVSQRT2);
        Vector3 out10(+H_INVSQRT2,0,-H_INVSQRT2);
        Vector3 out11(+H_INVSQRT2,0,+H_INVSQRT2);

        _corners.Add(CoverModelCorner(pt00, out00, score, c));
        _corners.Add(CoverModelCorner(pt01, out01, score, c));
        _corners.Add(CoverModelCorner(pt10, out10, score, c));
        _corners.Add(CoverModelCorner(pt11, out11, score, c));

#endif

      }
    }
  }

  /*
  https://dev.bistudio.com/svn/pgm/Colabo/data/ArmA2/Re_Hledat_úkryty_jinak-pøes_body_a_facy_1.txt
  */
  // gather faces from interesting components
  AutoArray<FaceInfo> faces;
  AutoArray<int> scores(ccs.Size(),INT_MAX);
  {
    PROFILE_SCOPE_DETAIL_EX(cmPCF, aiPath);
    for( int c=0; c<ccs.Size(); c++ )
    {
      const ConvexComponent *cc=ccs.Get(c);
      #if _ENABLE_CHEATS
        if (cc->GetNameIndex()==32 && !strcmpi(shape->GetName(),"Ca\\Structures_E\\HouseC\\House_C_2_EP1.p3d"))
        {
          __asm nop;
        }
      #endif
      if (cc->Max().Y()-cc->Min().Y()>0.5f) // the component is high enough to provide some cover
      {
        // check component size and penetrability
        // while several small components can compose a good cover
        // much more common situation is such component is not good
        // note: this is especially true when the whole object is small
        float ccSize = floatMax(cc->Max().X()-cc->Min().X(),cc->Max().Z()-cc->Min().Z());

        // TODO: check penetrability as well
        float ccScore = InterpolativC(ccSize,0,0.2f,2.5f,0)+objScore;
        // if the delay is too big, ignore it
        if (ccScore>=2.5f) continue;
        int score = toInt(ccScore*10);
        saturate(score,0,UCHAR_MAX);
        scores[c] = score;

        for (int i=0; i<cc->NPlanes(); i++)
        {
          faces.Add(FaceInfo(cc->GetFaceOffset(geom,i),cc->GetPlaneIndex(i),c));
        }
      }

    }
  }

  FindArrayKey< std::pair<int,int> > edges;
  AutoArray< AutoArray<FaceInfo> > faceOnEdge;
  {
    PROFILE_SCOPE_DETAIL_EX(cmGVE, aiPath);
    // gather edges vertical enough
    for (int i=0; i<faces.Size(); i++)
    {
      const Poly &face = geom->Face(faces[i].offset);
      for (int vi=0; vi<face.N(); vi++)
      {
        int vCur = face.GetVertex(vi);
        int vNxt = face.GetVertex((vi+1)%face.N());
        Vector3Val posCur = geom->Pos(vCur);
        Vector3Val posNxt = geom->Pos(vNxt);
        float minY = floatMin(posCur.Y(),posNxt.Y())+shape->BoundingCenterOnLoad().Y();
        if (minY<2.3f)
        {
          Vector3 edgeDir = (posCur-posNxt).Normalized();
        const float verticalLimit = 0.86602540378f; // cos 30 degree
          // ignore edges which are high enough to have no influence (e.g. roofs)
          
        if (fabs(edgeDir.Y())>verticalLimit)
        {
          int edgeIndex = edges.FindOrAdd(MakeEdge(vCur,vNxt));
          faceOnEdge.Access(edgeIndex);
          faceOnEdge[edgeIndex].Add(faces[i]);
        }
      }
    }
  }
  }

  #if 1
  OffTree<EdgePosInfo> edgePos;
  {
    PROFILE_SCOPE_DETAIL_EX(cmBET, aiPath);
    edgePos.Dim(
      geom->Min().X(),geom->Min().Z(),
      // enlarge epsilon to avoid assertions on rounding error
      floatMax(geom->Max().X()-geom->Min().X(),geom->Max().Z()-geom->Min().Z())+EdgePosInfo::Eps()*1.1f
    );
    for (int i=0; i<edges.Size(); i++)
    { 
      edgePos.Add(EdgePosInfo(geom->Pos(edges[i].first),geom->Pos(edges[i].second),i));
    }
  }
  #endif
  
  // check which edges are actually the same edge
  // build adjacency matrix, will be recomputed to 
  Array2D<bool> sameEdgeMatrix;
  {
    PROFILE_SCOPE_DETAIL_EX(cmBES, aiPath);
    sameEdgeMatrix.Dim(edges.Size(),edges.Size());
    for (int i=0; i<edges.Size(); i++) for (int j=0; j<edges.Size(); j++)
    {
      sameEdgeMatrix(i,j) = i==j;
    }
    for (int i=0; i<edges.Size(); i++)
    {
      Vector3Val i10 = geom->Pos(edges[i].first);
      Vector3Val i11 = geom->Pos(edges[i].second);
      EdgePosInfo iInfo(i10,i11,i);
      OffTree<EdgePosInfo>::QueryResult res;
      const float maxDist = 0.05f;
      edgePos.Query(res,iInfo.region._begX-maxDist,iInfo.region._begZ-maxDist,iInfo.region._endX+maxDist,iInfo.region._endZ+maxDist);
      for (int r=0; r<res.regions.Size(); r++)
      {
        int j = res.regions[r].index;
      
        Vector3Val j10 = geom->Pos(edges[j].first);
        Vector3Val j11 = geom->Pos(edges[j].second);
        
        if (CheckLineOverlapp(i10,i11,j10,j11,maxDist))
        {
          sameEdgeMatrix(i,j) = true;
          sameEdgeMatrix(j,i) = true;
        }
      }
    }
  }

  {
    PROFILE_SCOPE_DETAIL_EX(cmBEW, aiPath);
    // transitive enclose: Warschall's algorithm (http://datastructures.itgo.com/graphs/transclosure.htm)
    for(int i = 0;i <edges.Size(); i++) for(int j = 0;j < edges.Size(); j++)
    {
      if(sameEdgeMatrix(i,j))
      {
        for(int k = 0; k < edges.Size(); k++)
        {
          if(sameEdgeMatrix(j,k)) sameEdgeMatrix(i,k) = true;
        }
      }
    }
  }
  // each connected component is now a clique

  AutoArray< AutoArray<int> > edgeSets;
  {
    PROFILE_SCOPE_DETAIL_EX(cmBMS, aiPath);
    {
      AutoArray<bool> edgeDone(edges.Size(),false);
      // now build edge sets (group of equivalent edges)
      for(int i = 0;i <edges.Size(); i++)
      {
        // list all edges connected to this one
        if (edgeDone[i]) continue;
        int setIndex = edgeSets.Add();
        AutoArray<int> &edgeSet = edgeSets[setIndex];
        for(int j = 0;j < edges.Size(); j++)
        {
          if (sameEdgeMatrix(i,j)) edgeSet.Add(j),edgeDone[j] = true;
        }
        // each edge lists at least itself
        Assert(edges.Size()>0);
      }
    }
  }

  AutoArray< FindArrayKey<FaceInfo> > faceOnEdgeSet;
  {
    PROFILE_SCOPE_DETAIL_EX(cmBMF, aiPath);
    // merge adjacent face info for each edge set
    faceOnEdgeSet.Resize(edgeSets.Size());
    for (int i=0; i<edgeSets.Size(); i++)
    {
      for (int e = 0; e<edgeSets[i].Size(); e++)
      {
        faceOnEdgeSet[i].MergeUnique(faceOnEdge[edgeSets[i][e]]);
      }
    }
  }


  AutoArray<bool> edgeSetUsable(edgeSets.Size(),false);
  AutoArray<int> edgeSetComponent(edgeSets.Size(),-1);
  AutoArray<Vector3> edgeSetOut; // direction "out" from the edge
  {
    PROFILE_SCOPE_DETAIL_EX(cmBPF, aiPath);
    edgeSetOut.Resize(edgeSets.Size());
    // for each point check angle between every pair of faces
    // if some faces make the point unusable as a cover (angle too shallow), we mark it as unusable
    for (int e=0; e<edgeSets.Size(); e++)
    {
      // all faces here share the same "edge" - call it axis
      int edge = edgeSets[e][0];
      Vector3 axisPoint = geom->Pos(edges[edge].first);
      Vector3 axisDir = geom->Pos(edges[edge].second)-geom->Pos(edges[edge].first);
      // we know the axis is close to vertical
      Matrix4 fromAxis(MUpAndDirection,axisDir,VForward);
      fromAxis.SetPosition(axisPoint);
      Matrix4 toAxis(MInverseRotation,fromAxis);
      
      bool usable = true;
      // find the edge with the flattest angle
      float flattestAngle = 0;
      int flattestI = -1, flattestJ = -1;
      for (int fi = 0; fi<faceOnEdgeSet[e].Size(); fi++) for (int fj = 0; fj<fi; fj++)
      {
        const Plane &ni = geom->GetPlane(faceOnEdgeSet[e][fi].index);
        const Plane &nj = geom->GetPlane(faceOnEdgeSet[e][fj].index);
        // consider only "vertical enough" (i.e. side) planes here
        const float planeLimit = 0.5f; // cos 60 degree
        if (fabs(ni.Normal().Y())>planeLimit) continue;
        if (fabs(nj.Normal().Y())>planeLimit) continue;
        // now what?
        const Poly &pi = geom->Face(faceOnEdgeSet[e][fi].offset);
        const Poly &pj = geom->Face(faceOnEdgeSet[e][fj].offset);
        #if _DEBUG
          LogicalPoly pli;
          LogicalPoly plj;
          for (int i=0; i<pi.N(); i++) pli.Add(geom->Pos(pi.GetVertex(i)));
          for (int i=0; i<pj.N(); i++) plj.Add(geom->Pos(pj.GetVertex(i)));
       #endif
        // compute any point lying inside of the face - average will do
        Vector3 piCenter = pi.GetAverage(*geom);
        Vector3 pjCenter = pj.GetAverage(*geom);
        // convert into axis space
        Vector3 piCenterRel = toAxis * piCenter;
        Vector3 pjCenterRel = toAxis * pjCenter;
        // compute cross product of the center position with the normal
        Vector3 piNormalRel = toAxis.Orientation() * ni.Normal();
        Vector3 pjNormalRel = toAxis.Orientation() * nj.Normal();
        float piSign = piNormalRel.CrossProduct(piCenterRel).Y();
        float pjSign = pjNormalRel.CrossProduct(pjCenterRel).Y();
        if (piSign*pjSign>=0)
          continue; // both pointing in the same direction - not interesting
        //
        float iHeading = atan2(piCenterRel.X(),piCenterRel.Z());
        float jHeading = atan2(pjCenterRel.X(),pjCenterRel.Z());
        if (pjSign<0) Swap(iHeading,jHeading);
        float angle = AngleDifference(jHeading,iHeading);
        if (angle<-0.1f) angle += 2*H_PI; // <-0.1: assume almost full circle is never reached and this is rather a numerical error
            if (flattestAngle<angle)
            {
              flattestAngle = angle;
              flattestI = fi;
              flattestJ = fj;
            }
      } // for (int fi)
      Assert(!edgeSetUsable[e]); // each edge should be processed only once
      if (usable && flattestI>=0 && flattestAngle<HDegree(120))
      {
        edgeSetUsable[e] = true;
        const Plane &ni = geom->GetPlane(faceOnEdgeSet[e][flattestI].index);
        const Plane &nj = geom->GetPlane(faceOnEdgeSet[e][flattestJ].index);
        edgeSetComponent[e] = faceOnEdgeSet[e][flattestI].component;
        Vector3 outEdge = ni.Normal()+nj.Normal();
        outEdge[1] = 0; // for purpose of covering ignore the vertical component
        outEdge.Normalize();
        edgeSetOut[e] = -outEdge;
      }
    }
  }

  {
    PROFILE_SCOPE_DETAIL_EX(cmBAC, aiPath);
    // for each usable edge set use only the longest one as a representative one
    for (int e=0; e<edgeSets.Size(); e++)
    {
      if (!edgeSetUsable[e]) continue;
      float longestSize2 = 0;
      int  longestEI = -1;
      for (int ee=0; ee<edgeSets[e].Size(); ee++)
      {
        int ei = edgeSets[e][ee];
        Vector3Val ei0 = geom->Pos(edges[ei].first);
        Vector3Val ei1 = geom->Pos(edges[ei].second);
        float size2 = ei0.Distance2(ei1);
        if (longestSize2<size2) longestSize2 = size2, longestEI = ei;
      }
      if (longestSize2>0)
      {
        Vector3Val e0 = geom->Pos(edges[longestEI].first);
        Vector3Val e1 = geom->Pos(edges[longestEI].second);
        Vector3Val eTop = e0.Y()>e1.Y() ? e0 : e1;
        int ci = edgeSetComponent[e];
        _corners.Add(CoverModelCorner(eTop,edgeSetOut[e],scores[ci],ci));
      }
    }
  }

  _edges.Compact();
  _corners.Compact();
}

void OperFieldFull::AddCover()
{
  // based on CreateField
  
  PROFILE_SCOPE_EX(pthAC, aiPath);

  Object *obj;

  // space
  for (int ix=_x-1; ix<=_x+1;ix++)
    for (int iz=_z-1; iz<=_z+1;iz++) 
      if (InRange(ix, iz))
      {
        MAKE_OBJECT_LIST_XXL(list, GLandscape, ix, iz)
        for (int i=0;i<list.Size();i++)
        {
          obj = list[i];
          if (obj == NULL) continue;

          LODShape *shape=obj->GetShape();
          if (!shape) continue;
          if (!obj->Static()) continue;

          // test bounding sphere
          Object::ProtectedVisualState<const ObjectVisualState> pvs = obj->FutureVisualStateScope();

          Vector3Val center = pvs->Position();
          float radius = shape->BoundingSphere() * pvs->Scale();
          if (center.X() + radius <= _x * LandGrid) continue;
          if (center.X() - radius >= (_x + 1) * LandGrid) continue;
          if (center.Z() + radius <= _z * LandGrid) continue;
          if (center.Z() - radius >= (_z + 1) * LandGrid) continue;

          if (!obj->HasGeometry()) continue;
          if (!IsVertical(obj->FutureVisualState().DirectionUp())) continue; // the cover needs to be placed mostly vertically

          const CoverModel &coverModel = obj->GetShape()->GetCoverModel();
          
          #if 1
          const ConvexComponents &ccs=shape->GetGeomComponents();
          for (int c=0; c<coverModel._edges.Size(); c++)
          {
            const CoverModelEdge &cover = coverModel._edges[c];
            const ConvexComponent *cc = ccs[cover.component];
            Vector3 center = 0.5f * (cc->Min() + cc->Max());
            Vector3 aveDown(center.X(),cc->Min().Y(),center.Z());
            Vector3 aveTop(center.X(),cc->Max().Y(),center.Z());
            
            float topLimit = obj->FutureVisualState().PositionModelToWorld(aveTop).Y() + OBJ_BELOW_LIMIT; // topLimit is given as the maximum y of the object plus OBJ_BELOW_LIMIT
            float botLimit = obj->FutureVisualState().PositionModelToWorld(aveDown).Y()-0.2f; // we may allow a small gap on the ground level, rather than rejecting a valid cover
            // y coordinate is used for cover only
            // adjust it to indicate prone, crouch, stand
            // BoundingCenter is placed at original (0,0,0)
            
            Vector3 pt00 = obj->FutureVisualState().PositionModelToWorld(cover.pos1);
            Vector3 pt01 = obj->FutureVisualState().PositionModelToWorld(cover.pos2);

            // landscape height in the corners of component
            float surf00 = GLandscape->SurfaceY(pt00.X(), pt00.Z());
            float surf01 = GLandscape->SurfaceY(pt01.X(), pt01.Z());

            /**/
            if (topLimit > surf00 || topLimit > surf01)
            {
              if (surf00>=botLimit || surf01>=botLimit) // check if it provides some cover at the ground level
              {
                #if 1
                Vector3 out00_01 = obj->FutureVisualState().DirectionModelToWorld(cover.out);
                ProcessCoverEdge(pt00, pt01, surf00, surf01, out00_01, cover.score);
                #endif
              }
            }
          }
          
          for (int c=0; c<coverModel._corners.Size(); c++)
          {
            const CoverModelCorner &cover = coverModel._corners[c];
            const ConvexComponent *cc=ccs.Get(cover.component);
            Vector3 center = 0.5f * (cc->Min() + cc->Max());
            // bottom center point of the object
            
            Vector3 aveDown(center.X(),cc->Min().Y(),center.Z());
            Vector3 aveTop(center.X(),cc->Max().Y(),center.Z());
            
            // y coordinate is used for cover only
            // adjust it to indicate prone, crouch, stand
            // BoundingCenter is placed at original (0,0,0)
            
            Vector3 pt00 = obj->FutureVisualState().PositionModelToWorld(cover.pos);

            float topLimit = obj->FutureVisualState().PositionModelToWorld(aveTop).Y() + OBJ_BELOW_LIMIT; // topLimit is given as the maximum y of the object plus OBJ_BELOW_LIMIT
            float botLimit = obj->FutureVisualState().PositionModelToWorld(aveDown).Y()-0.2f; // we may allow a small gap on the ground level, rather than rejecting a valid cover

            // landscape height in the corners of component
            float surf00 = GLandscape->SurfaceY(pt00.X(), pt00.Z());

            /**/
            if (topLimit > surf00)
            {
              // we may allow a small gap on the ground level, rather than rejecting a valid cover
              if (surf00>=botLimit) // check if it provides some cover at the ground level
              {
                // for corner covers we need to make sure the point we are checking is already well covered
                pt00[1] -= OperCover::UpperGap();

                #if 1
                // adjust for known cover heights
                CoverType ct00 = AdjustCoverHeight(pt00[1],surf00);
                // build cover candidates
                // compute heading of "normals" outside of the component corresponding to pt00...pt11

                Vector3 out00 = obj->FutureVisualState().DirectionModelToWorld(cover.out);
                ProcessCoverCorner(ct00, pt00, surf00, out00, cover.score);

                #endif
                
              }
            }
          
          }
          #endif
        }
      }
}

void OperFieldFull::RemoveFalseCover()
{
  // perform visibility testing to remove any cover candidates which are known to be useless
  PROFILE_SCOPE_DETAIL_EX(pthFC,aiPath);

  #if DIAG_FALSE_COVER && _ENABLE_PERFLOG>1
    int totalWall = 0, totalCorn = 0, totalEdge = 0;
    if (PROFILE_SCOPE_NAME(pthFC).IsActive())
    {
      // gather stats about covers: total and validated
      for (int i=0; i<_cover.Size(); i++)
      {
        if (_cover[i]._valid!=OperCover::Unchecked)
        {
          DoAssert(_cover[i]._valid>=OperCover::BadHeight);
          continue;
        }
        switch (_cover[i]._type)
        {
          case CoverNone: break;
          case CoverWall: totalWall++;break;
          case CoverEdgeLow: 
          case CoverEdgeMiddle: totalEdge++;break;
          default: totalCorn++;break;
        }
      }
    }
  #endif

  int src, dst;
  for (src=dst=0; src<_cover.Size(); src++)
  {
#if 0 // _ENABLE_CHEATS
    // [2883.37,2900.77,0.00146294]
    static Vector3 debugPos = Vector3(2883.37,0,2900.77);
    if (_cover[src]._pos.DistanceXZ2(debugPos)<Square(3))
    {
      __asm nop;
    }
#endif
    bool cover = false;
    
    #if DIAG_FALSE_COVER
    if (_cover[src]._valid!=OperCover::Unchecked)
    {
      // if validity already known, do not touch it
      Assert(_cover[src]._valid>=OperCover::BadHeight);
    }
    else 
    #endif
    if (_cover[src]._type.GetEnumValue()<CoverEdgeMiddle) // corner cover
    {
      PROFILE_SCOPE_DETAIL_EX(pthCC,aiPath);
      // the assumed cover position is based on a collision geometry
      // we need to check its relation to fire geometry and if they do not match, then adjust somehow
      
      // we would like to find some edge in the fire geometry which would correspond to this point
      
      // start by casting a ray from the assumed cover position
      // TODO: detect the cover very low above the ground
      Matrix3 orient(MRotationY,-_cover[src]._heading);
      // cast ray from left to right. If there is any intersection on any side, the cover is assumed to be useless
      Vector3Val norm = orient.Direction();
      Vector3Val dir = orient.DirectionAside();
      // pull the collision test slightly out to make sure we do not fail
      Vector3Val pos = _cover[src]._pos + norm*0.1f;
      bool collision = false;
      {
        CollisionBuffer col;
        // we want to have at least 1m free at each side, angled pi/4 (following the corner "tangent")
        GLandscape->ObjectCollisionLine(0,col,Landscape::FilterNone(),pos-dir,pos+2.0f*dir,0,ObjIntersectFire,ObjIntersectView, CF_ONLY_STATIC|CF_FIRST_CONTACT);
        if(col.Size())
            collision = true;
      }
      if (!collision)
      {

        // first try if ray parallel to the free look would hit
        {
          CollisionBuffer col;
          // pull the collision test slightly in to make sure we do intersect ("fail")
          Vector3Val insidePos = _cover[src]._pos - norm*0.1f;
          GLandscape->ObjectCollisionLine(0,col,Landscape::FilterNone(),insidePos-dir,insidePos+2.0f*dir,0,ObjIntersectFire,ObjIntersectView, CF_ONLY_STATIC|CF_FIRST_CONTACT);
          if(col.Size())
            collision = true;
        }
#if 0
        // experiments have shown many trunks do not pass even following test
        // because the collision and fire geometries are too different
        if (!cover)
        {
          // cast test inside of the geometry to catch tree trunks and other relatively small covers
          // such cover may easily be missed by the first test
          CollisionBuffer col;
          // if there is no hit soon enough
          GLandscape->ObjectCollision(col,NULL,NULL,pos,pos-norm,0,ObjIntersectFire,ObjIntersectView);
          // ignore collisions with moving entities
          // TODO: pass filter to ObjectCollision instead
          for (int i=0; i<col.Size(); i++)
          {
            const CollisionInfo &info = col[i];
            // if the cover is not close enough, we cannot use it
            if (info.under>0.3f) continue;
            if (info.dirOut.SquareSize()>Square(0.2f))
            { // if the hit is wide enough, we know the cover will do
              cover = true;
              break;
            }
            else
            { 
              if (info.hierLevel>0) continue;
              if (info.geomLevel<0) continue;
              LODShape *lodShape = info.object->GetShape();
              const ConvexComponents *ccs = lodShape->GetConvexComponents(info.geomLevel);
              const ConvexComponent *cc = (*ccs)[info.component];
              const float requiredSize = 0.2f;
              // if the cover is not wide enough, we cannot use it
              if (cc->Max().X()-cc->Min().X()<requiredSize && cc->Max().Z()-cc->Min().Z()<requiredSize) continue;
              cover = true;
              break;
            }
          }
        }
#endif
      }
      
      #if DIAG_FALSE_COVER
      if (cover)
      {
        _cover[src]._valid = OperCover::Valid;
      }
      else if (!collision)
      {
        _cover[src]._valid = OperCover::NoCollision;
      }
      else
      {
        _cover[src]._valid = OperCover::NoFreeSpace;
      }
      #endif
    }
    else if (_cover[src]._type.GetEnumValue()<=CoverEdgeLow) // edge cover
    {
      PROFILE_SCOPE_DETAIL_EX(pthCE,aiPath);
      // additional verification for the horizontal edge cover?
      Matrix3 orient(MRotationY,-_cover[src]._heading);
      Vector3Val norm = orient.Direction();
      Vector3Val pos = _cover[src]._pos + norm*0.1f;

      // we need to be covered, and we need to be able to fire on the top
      bool collision = false;
      {
        CollisionBuffer col;
        // check if there is a cover - we do fail the collision test
        GLandscape->ObjectCollisionLine(0,col,Landscape::FilterNone(),pos,pos-2.0f*norm,0,ObjIntersectFire,ObjIntersectView, CF_ONLY_STATIC|CF_FIRST_CONTACT);
        if(col.Size())
          collision = true;
      }
      if (collision)
      {
        cover = true;
        // bottom cover exists, check for fire opportunity
        {
          CollisionBuffer col;
          Vector3Val upPos = pos+VUp*0.5f;
          // pull the collision test slightly up to make sure we do not "fail"
          GLandscape->ObjectCollisionLine(0,col,Landscape::FilterNone(),upPos,upPos-5.0*norm,0,ObjIntersectFire,ObjIntersectView, CF_ONLY_STATIC|CF_FIRST_CONTACT);
          if(col.Size())
            collision = true;
        }
        if (!cover && _cover[src]._type==CoverEdgeMiddle)
        {
          // if collision, high enough, but no firing opportunity, we still pass as a wall cover
          _cover[src]._type = CoverWall;
          cover = true;
        }
      }
      #if DIAG_FALSE_COVER
      if (cover)
      {
        _cover[src]._valid = OperCover::Valid;
      }
      else if (!collision)
      {
        _cover[src]._valid = OperCover::NoCollision;
      }
      else
      {
        _cover[src]._valid = OperCover::NoFreeSpace;
      }
      #endif
    }
    else // wall cover
    {
      // no visibility expected for walls
      cover = true;
      #if DIAG_FALSE_COVER
      if (cover)
      {
        _cover[src]._valid = OperCover::Valid;
      }
      #endif
    }
    #if !DIAG_FALSE_COVER // when doing diags, we do not remove cover, we only mark it
    if (cover)
    #endif
    {
      _cover[dst++] = _cover[src];
    }
  }
  
  #if DIAG_FALSE_COVER && _ENABLE_PERFLOG>2
    if (PROFILE_SCOPE_NAME(pthFC).IsActive())
    {
      // gather stats about covers: total and validated
      int valid = 0;
      int validWall = 0, validCorn = 0, validEdge = 0;
      for (int i=0; i<dst; i++)
      {
        if (_cover[i]._valid!=OperCover::Valid) continue;
        valid ++;
        switch (_cover[i]._type)
        {
          case CoverNone: break;
          case CoverWall: validWall++;break;
          case CoverEdgeLow: 
          case CoverEdgeMiddle: validEdge++;break;
          default: validCorn++;break;
        }
      }
      PROFILE_SCOPE_NAME(pthFC).AddMoreInfo(Format(
        "Total: %d (W:%d,C:%d,E:%d), valid %d (W:%d,C:%d,E:%d)",
        _cover.Size(),totalWall,totalCorn,totalEdge,
        valid,validWall,validCorn,validEdge
      ));
    }
  #endif
  _cover.Resize(dst);
}

CoverField OperFieldFull::GetCoverField(OperFieldIndex x, OperFieldIndex z) const
{
  // if someone is asking for the cover, the cover needs to be ready
  Assert(_pathingReady);
  // list all covers with the given xs/zs 
  // thanks to CloseCoverList they are guaranteed to be continuous in the array
  CoverField field;
  for (int i=0; i<_cover.Size(); i++)
  {
    const OperCover &cover = _cover[i];
    if (cover._z==z && cover._x==x)
    {
      if (field.beg>=field.end) field.beg = i;
      field.end = i+1;
    }
    else
    {
      // if list was already started and we are no longer in it, we are done
      // if we have already passed the list in z, we are done
      if (field.beg<field.end || cover._z>z) break;
    }
  }
  return field;

}
const OperCover *OperFieldFull::GetCover(OperFieldIndex x, OperFieldIndex z, int &nCover) const
{
  // if someone is asking for the cover, the cover needs to be ready
  Assert(_pathingReady);
  const CoverField &field = _coverIndex.Get(x, z);
  if (field.IsEmpty())
  {
    nCover = 0;
    return NULL;
  }
  nCover = field.end-field.beg;
  return _cover.Data()+field.beg;
}

const OperCover *OperFieldFull::GetCover(int &nCover) const
{
  // if someone is asking for the cover, the cover needs to be ready
  Assert(_pathingReady);
  nCover = _cover.Size();
  return _cover.Data();
}

bool OperFieldFull::IsSimple(OperItem &item) const
{
#if STORE_OBJ_ID
  // if there are some objects participating, the field cannot be represented by a simple field
  if (_objs.Size()>0) return false;
#endif
  if (_cover.Size()>0) return false;
  item = _items[0][0];
  for (int j=0; j<OperItemRange; j++)
    for (int i=0; i<OperItemRange; i++)
    {
      OperItem value = _items[i][j];
      if (value.GetType() != item.GetType() || value.GetTypeSoldier() != item.GetTypeSoldier()) return false;
    }
  return true;
}

// OperCache

TypeIsSimple(OperField*)

// note: class definition is in cpp file, as interface ILockCache
// is used in other sources

class OperCache: public IOperCache, public MemoryFreeOnDemandHelper
{
private:
	FramedMemoryControlledList< OperField, FramedMemoryControlledListTraitsRef<OperField> > _fields;
	// fast operField searching
  QuadTreeExRoot<OperField *> _index;

	void RemoveOld(float limit, int untilSize); // return some removed entry for recycling

public:
	OperField* GetOperField(LandIndex x, LandIndex z, int mask);
	void RemoveField(OperField *fld);
	void RemoveField(LandIndex x, LandIndex z);
  void Update();

	OperCache(Landscape *land);
	~OperCache();

	//@{ implementation of MemoryFreeOnDemandHelper interface
	virtual size_t FreeOneItem();
	virtual float Priority() const;
  virtual size_t MemoryControlled() const {return _fields.MemoryControlled();}
  virtual size_t MemoryControlledRecent() const {return _fields.MemoryControlledRecent();}
  virtual RString GetDebugName() const;
  virtual void MemoryControlledFrame() {_fields.Frame();}
  //@}

	int NFields() const {return _fields.ItemCount();}
  /// direct access to the cache (diagnostics)
  const OperField *CheckField(LandIndex x, LandIndex z) const
  {
    if (!InRange(z, x)) return NULL;
    return _index(x, z);
  }
};

IOperCache *CreateOperCache(Landscape *land){return new OperCache(land);}

// DEFINE_FAST_ALLOCATOR(QuadTreeExRoot<OperField *>::QuadTreeType)

// TODO: make these values scalable
// static (based on system memory)
// dynamic (based on free system memory)

const int OperCacheMinSize = 64 * 8;
const int OperCacheNormalSize = 256 * 8;
const int OperCacheMaxSize = 1024 * 8;

OperField* OperCache::GetOperField(LandIndex x, LandIndex z, int mask)
{
  // MASK_FOR_PATHING should be handled on higher levels - not expected here
  Assert((mask&MASK_FOR_PATHING)==0);
	if (!InRange(z, x))
		return NULL;
	OperField *fld = _index(x,z);
	if (fld)
	{
		Assert( fld->_x == x);
		Assert( fld->_z == z);
		Ref<OperField> temp = fld;
		_fields.Refresh(fld); // remove from current location
		fld->_lastUsed = Glob.time;
		return fld;
	}
	
	// _data is sorted by time when it was used
	int count = _fields.ItemCount();
	if (count >= OperCacheMinSize)
	{
		// we have more entries than we would like - remove only very old entries
		RemoveOld(60,OperCacheMinSize);
		if (count >= OperCacheNormalSize)
		{
			// we have to much entries - remove more recent entries
			RemoveOld(5,OperCacheNormalSize);
			if (count >= OperCacheMaxSize)
			{
				// we must release something - memory usage critical
				// we have to remove oldest field
				RemoveField(_fields.First());
			}
		}
	}

	Ref<OperFieldFull> entryFull = new OperFieldFull(x, z, mask);
  OperField *entry = entryFull;
  OperItem item;
  if (entryFull->IsSimple(item)) entry = new OperFieldSimple(x, z, item);

	// create a new entry
	_fields.Add(entry);
	// _index(x,z) = entry;
  _index.Set(x, z, entry);
	entry->_lastUsed = Glob.time;
	return entry;
}

void OperCache::Update()
{
  // remove entries which are too old - tides may make them useless
  // 30 minutes sounds quite safe - it is quite long, but tides should not change much meanwhile
  RemoveOld(60*30,OperCacheMinSize);
}

void OperCache::RemoveField(OperField*fld)
{
  _index.Set(fld->_x, fld->_z, NULL);
	_fields.Delete(fld);
}

void OperCache::RemoveField(LandIndex x, LandIndex z)
{
	if (!InRange(z, x)) return;
	OperField *fld = _index(x,z);
	if (!fld) return;
	RemoveField(fld);
}

void OperCache::RemoveOld(float limit, int untilSize)
{
	// start with the oldest entry
	for
	(
		OperField *fld =_fields.First(), *prv;
		fld;
		fld = prv
	)
	{
		prv = _fields.Next(fld);
		// if we find some too new entry, we may be sure all other will be newer
		if (fld->_lastUsed >= Glob.time - limit) break;
		RemoveField(fld);
		// check if we have reached the limit
		if (_fields.ItemCount()<=untilSize ) break;
	}
}

OperCache::OperCache(Landscape *land)
: _index(land->GetLandRange(), land->GetLandRange(), NULL)
{
	RegisterFreeOnDemandMemory(this);
}

OperCache::~OperCache()
{
  _fields.Clear();
}

// implementation of MemoryFreeOnDemandHelper abstract interface

const int OperFieldMemSize = sizeof(OperField);

size_t OperCache::FreeOneItem()
{
  // FIX: see BankArrayCached::FreeOneItem
//	OperField *fld =_fields.Last();
  OperField *fld =_fields.First();
	if (!fld) return 0;
	RemoveField(fld);
	return OperFieldMemSize;
}

float OperCache::Priority() const
{
	return 1.0f;
}

RString OperCache::GetDebugName() const
{
  return "Path finding";
}

#endif // _ENABLE_AI
