// pathPlanner.cpp: implementation of the pathPlanner class.
//
//////////////////////////////////////////////////////////////////////

#include "../wpch.hpp"

#if _ENABLE_AI

#include "pathPlanner.hpp"
//#include "engine.hpp"
#include <El/Common/perfLog.hpp>
#include "../vehicleAI.hpp"
#include "../world.hpp"
#include "../landscape.hpp"
#include "ai.hpp"

#include "aiDefs.hpp"

#include "../roads.hpp"

#include "../paramArchiveExt.hpp"

#pragma optimize ("t",on)

// A* algoritm for searching the best path
//#define ITER_PER_CYCLE						50
const float TimePerCycle = 0.003f;
#define Directions								20
#define direction_delta						directions20

#define FieldsRange		(LandRange / BigFieldSize)

// note: class definition is in cpp file, as interface ILockCache
// is used in other sources

#define CHECK_PERFORMANCE	0
/*
#undef LOG_STRAT
#define LOG_STRAT 1
*/

#include <El/HiResTime/hiResTime.hpp>

#define USE_BAD_COST_FUNCTION	0

#include <Es/Algorithms/aStar.hpp>

#if DIAG_A_STAR
#include "../global.hpp"
#endif

union ASSField
{
  /// the whole key encoded to a single member
  unsigned __int64 _key;
  struct
  {
    /// strategic coordinates
    short int _x;
    /// strategic coordinates
    short int _z;
    /// road we are on
    const RoadLink *_road;
  } coord;
  ASSField(int x, int z, const RoadLink *road) {coord._x = x; coord._z = z; coord._road = road;}
  bool operator == (const ASSField &with) const {return with._key == _key;}
  unsigned __int64 GetKey() const {return _key;}
#if DIAG_A_STAR >= 2
  RString GetDebugName() const
  {
    return Format("[%d, %d]", coord.x, coord.z);
  }
#endif
};

typedef AStarNode<ASSField> ASSNode;
TypeIsSimple(ASSNode *)
DEFINE_FAST_ALLOCATOR(ASSNode)

class ASSCostFunction
{
protected:
  CostFunction _costFunction;
  void *_param;
  OLink(EntityAI) _vehicle;

public:
  ASSCostFunction(CostFunction costFunction, void *param, EntityAI *vehicle)
  {
    _costFunction = costFunction;
    _param = param;
    _vehicle = vehicle;
  }
  float operator () (const ASSField &field1, const ASSField &field2) const;

protected:
  float GetFieldCost(int x, int z, bool road, bool includeGradient = true, float gradient = 0) const {return _costFunction(x, z, road, includeGradient, gradient, _param);}
};

float ASSCostFunction::operator () (const ASSField &field1, const ASSField &field2) const
{
  // gradient calculation
  Vector3 pos1, pos2;
  if (field1.coord._road) pos1 = field1.coord._road->AIPosition();
  else
  {
    pos1[0] = (field1.coord._x + 0.5f) * LandGrid;
    pos1[2] = (field1.coord._z + 0.5f) * LandGrid;
    pos1[1] = GLandscape->SurfaceYAboveWaterNoWaves(pos1[0], pos1[2]);
  }
  if (field2.coord._road) pos2 = field2.coord._road->AIPosition();
  else
  {
    pos2[0] = (field2.coord._x + 0.5f) * LandGrid;
    pos2[2] = (field2.coord._z + 0.5f) * LandGrid;
    pos2[1] = GLandscape->SurfaceYAboveWaterNoWaves(pos2[0], pos2[2]);
  }
  float distXZ2 = pos1.DistanceXZ2(pos2);
  float distY = pos2.Y() - pos1.Y();
  float gradient = distXZ2 > 0 ? distY * InvSqrt(distXZ2) : 0; // distY / distXZ

  // special handling of roads 
  if (field1.coord._road || field2.coord._road)
  {
    if (field1.coord._road && field2.coord._road)
    {
      // moving on road
      float avgCost = 0.5f * (
        GetFieldCost(field1.coord._x, field1.coord._z, true, false, gradient) +
        GetFieldCost(field2.coord._x, field2.coord._z, true, false, gradient));
      float dist2 = distXZ2 * Square(InvLandGrid); // distance in LandGrids
      // optimization, sqrt is too slow
      return avgCost * sqrt(dist2); 
    }
    else
    {
      // moving to / from road
      int xc = field1.coord._x, zc = field1.coord._z;
      Assert(xc == field2.coord._x && zc == field2.coord._z);
      float cost = GetFieldCost(xc, zc, false, false, gradient);
      float dist2 = distXZ2 * Square(InvLandGrid); // distance in LandGrids
      // Note: optimization using InvSqrt removed as dist2 happened to be ZERO sometimes
      return cost * sqrt(dist2); 
    }
  }

  const float H_SQRT5 = 2.2360679775;
  const float H_SQRT5_4 = 0.25 * H_SQRT5;
  const float H_SQRT2_2 = 0.5 * H_SQRT2;

  int xf = field1.coord._x;
  int zf = field1.coord._z;
  int xt = field2.coord._x;
  int zt = field2.coord._z;

  int dx = xt - xf;
  int dz = zt - zf;

  float result = GetFieldCost(xf, zf, false, false, gradient) + GetFieldCost(xt, zt, false, false, gradient);
#if USE_BAD_COST_FUNCTION
  switch (dx)
  {
  case -2:
    switch (dz)
    {
    case -1:
      result += GetFieldCost(xf - 1, zf, false, false, gradient);
      result *= H_SQRT5_4;
      break;
    case 0:
      result += GetFieldCost(xf - 1, zf, false, false, gradient);
      result *= 0.5;
      break;
    case 1:
      result += GetFieldCost(xf - 1, zf, false, false, gradient);
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
      result += GetFieldCost(xf, zf - 1, false, false, gradient);
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
      result += GetFieldCost(xf, zf + 1, false, false, gradient);
      result *= H_SQRT5_4;
      break;
    }
    break;
  case 0:
    switch (dz)
    {
    case -2:
      result += GetFieldCost(xf, zf - 1, false, false, gradient);
      result *= 0.5;
      break;
    case -1:
      result *= 0.5;
      break;
    case 1:
      result *= 0.5;
      break;
    case 2:
      result += GetFieldCost(xf, zf + 1, false, false, gradient);
      result *= 0.5;
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
      result += GetFieldCost(xf, zf - 1, false, false, gradient);
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
      result += GetFieldCost(xf, zf + 1, false, false, gradient);
      result *= H_SQRT5_4;
      break;
    }
    break;
  case 2:
    switch (dz)
    {
    case -1:
      result += GetFieldCost(xf + 1, zf, false, false, gradient);
      result *= H_SQRT5_4;
      break;
    case 0:
      result += GetFieldCost(xf + 1, zf, false, false, gradient);
      result *= 0.5;
      break;
    case 1:
      result += GetFieldCost(xf + 1, zf, false, false, gradient);
      result *= H_SQRT5_4;
      break;
    case -2:
    case 2:
      Fail("Unused");
      break;
    }
    break;
  }
#else
  switch (dx)
  {
  case -2:
    switch (dz)
    {
    case -1:
      result += GetFieldCost(xf - 1, zf - 1, false, false, gradient);
      result += GetFieldCost(xf - 1, zf, false, false, gradient);
      result *= H_SQRT5_4;
      break;
    case 0:
      result *= 0.5;
      result += GetFieldCost(xf - 1, zf, false, false, gradient);
      break;
    case 1:
      result += GetFieldCost(xf - 1, zf, false, false, gradient);
      result += GetFieldCost(xf - 1, zf + 1, false, false, gradient);
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
      result += GetFieldCost(xf - 1, zf - 1, false, false, gradient);
      result += GetFieldCost(xf, zf - 1, false, false, gradient);
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
      result += GetFieldCost(xf - 1, zf + 1, false, false, gradient);
      result += GetFieldCost(xf, zf + 1, false, false, gradient);
      result *= H_SQRT5_4;
      break;
    }
    break;
  case 0:
    switch (dz)
    {
    case -2:
      result *= 0.5;
      result += GetFieldCost(xf, zf - 1, false, false, gradient);
      break;
    case -1:
      result *= 0.5;
      break;
    case 1:
      result *= 0.5;
      break;
    case 2:
      result *= 0.5;
      result += GetFieldCost(xf, zf + 1, false, false, gradient);
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
      result += GetFieldCost(xf, zf - 1, false, false, gradient);
      result += GetFieldCost(xf + 1, zf - 1, false, false, gradient);
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
      result += GetFieldCost(xf, zf + 1, false, false, gradient);
      result += GetFieldCost(xf + 1, zf + 1, false, false, gradient);
      result *= H_SQRT5_4;
      break;
    }
    break;
  case 2:
    switch (dz)
    {
    case -1:
      result += GetFieldCost(xf + 1, zf - 1, false, false, gradient);
      result += GetFieldCost(xf + 1, zf, false, false, gradient);
      result *= H_SQRT5_4;
      break;
    case 0:
      result *= 0.5;
      result += GetFieldCost(xf + 1, zf, false, false, gradient);
      break;
    case 1:
      result += GetFieldCost(xf + 1, zf, false, false, gradient);
      result += GetFieldCost(xf + 1, zf + 1, false, false, gradient);
      result *= H_SQRT5_4;
      break;
    case -2:
    case 2:
      Fail("Unused");
      break;
    }
    break;
  }
#endif

  return result;
}

class ASSHeuristicFunction
{
protected:
  float _coef;

public:
  ASSHeuristicFunction(float coef) {_coef = coef;}
  float operator () (const ASSField &field1, const ASSField &field2) const
  {
    int dx = abs((int)field1.coord._x - field2.coord._x);
    int dz = abs((int)field1.coord._z - field2.coord._z);
/*
    float minD = (dx + dz - fabs(dx - dz)) * 0.5;
    float maxD = (dx + dz + fabs(dx - dz)) * 0.5;
    return _coef * ((maxD - minD) + H_SQRT2 * minD);
*/
    // optimization
    float dif = fabs(dx - dz);
    return _coef * (dif + (0.5f * H_SQRT2) * (dx + dz - dif));
  }
};

class ASSReadyFunction
{
public:
  bool operator () (const ASSField &field) const {return true;}
};

#if 1

class ASSInterruptFunction
{
protected:
  SectionTimeHandle _sectionTime;
  float _maxTime;

public:
  ASSInterruptFunction(float maxTime) {_maxTime = maxTime;}
  void Start() {_sectionTime = StartSectionTime();}
  void Iteration() {}
  bool operator () () {return GetSectionTime(_sectionTime) >= _maxTime;}
};

#else

// this variant enable to better observe diagnostics (slow animation)
// switch to the map, enable Path diagnostics in the debug console

#include "../global.hpp"
class ASSInterruptFunction
{
protected:
  Time _nextTime;

public:
  ASSInterruptFunction(float maxTime) {_nextTime = Time(0);}
  void Start() {}
  void Iteration() {}
  bool operator () ()
  {
    if (Glob.time >= _nextTime)
    {
      _nextTime = Glob.time + 0.2f;
      return false; // process a single step
    }
    return true;
  }
};

#endif

class ASSIterator
{
protected:
  int _x, _z;
  const RoadLink *_road;
  /// negative _index is used for roads, 0 and positive for regular fields
  int _index;
  int _maxIndex;

public:
  ASSIterator() {_index = 0; _maxIndex = 0; _x = -1; _z = -1; _road = NULL;}
  ASSIterator(const ASSField &field, void *context = NULL);
  operator bool () const {return _index < _maxIndex;}
  void operator ++ () {_index++;}
  operator ASSField ();
};

ASSIterator::ASSIterator(const ASSField &field, void *context)
{
  _x = field.coord._x; _z = field.coord._z; _road = field.coord._road;
  if (_road)
  {
    _index = -_road->NConnections();
    _maxIndex = _road->IsBridge() ? 0 : 1;
  }
  else
  {
    const RoadNet *roadNet = GLandscape->GetRoadNet();
    if (roadNet)
    {
      _index = -roadNet->GetRoadList(_x, _z).Size();
    }
    else _index = 0;
    _maxIndex = Directions;
  }
}

ASSIterator::operator ASSField ()
{
  if (_road)
  {
    // from the road
    while (_index < 0)
    {
      // to connected road
      const RoadLink *road = _road->Connections()[-_index - 1];
      if (!road)
      {
        _index++;
        continue;
      }
      int x = toIntFloor(road->Transform().Position().X() * InvLandGrid);  // (need not to use AIPosition() - Y is not used)
      int z = toIntFloor(road->Transform().Position().Z() * InvLandGrid);
      return ASSField(x, z, road);
    }
    // to the center of field
    Assert(_index == 0);
    Assert(!_road->IsBridge());
    return ASSField(_x, _z, NULL);
  }
  else
  {
    // from the center of field
    while (_index < 0)
    {
      // to road
      const RoadNet *roadNet = GLandscape->GetRoadNet();
      Assert(roadNet);
      const RoadLink *road = roadNet->GetRoadList(_x, _z)[-_index - 1];
      if (road == _road)
      {
        // skip link to itself
        _index++;
        continue;
      }
      return ASSField(_x, _z, road);
    }
    // to neighbor field
    return ASSField(_x + direction_delta[_index][0], _z + direction_delta[_index][1], NULL);
  }
}

struct ASSOpenListTraits
{
  static bool IsLess(const ASSNode *a, const ASSNode *b) {return a->_f < b->_f;}
  static bool IsLessOrEqual(const ASSNode *a, const ASSNode *b){return a->_f <= b->_f;}
};
class ASSOpenList : public HeapArray<ASSNode *, MemAllocD, ASSOpenListTraits>
{
typedef HeapArray<ASSNode *, MemAllocD, ASSOpenListTraits> base;
public:
  void UpdateUp(ASSNode *node) {base::HeapUpdateUp(node);}
  void Add(ASSNode *node) {base::HeapInsert(node);}
  bool RemoveFirst(ASSNode *&node) {return base::HeapRemoveFirst(node);}
};

class ASSNodeRef : public SRef<ASSNode>
{
  typedef SRef<ASSNode> base;

public:
  ASSNodeRef() {}
  ASSNodeRef(ASSNode *node) : base(node) {}
  unsigned __int64 GetKey() const {return (*this)->_field._key;}
};
TypeIsMovableZeroed(ASSNodeRef)

struct ASSClosedListTraits: public DefMapClassTraits<ASSNodeRef>
{
  //! key type
  typedef unsigned __int64 KeyType;
  //! calculate hash value
  static unsigned int CalculateHashValue(KeyType key)
  {
    return (unsigned int)((key >> 32) ^ (key & 0xffffffff));
  }

  //! compare keys, return negative when k1<k2, positive when k1>k2, zero when equal
  static int CmpKey(KeyType k1, KeyType k2)
  {
    if (k1 < k2) return -1;
    if (k1 > k2) return +1;
    return 0;
  }
  
  static KeyType GetKey(const ASSNodeRef &item) {return item.GetKey();}
};
class ASSClosedList : public MapStringToClass<ASSNodeRef, AutoArray<ASSNodeRef>, ASSClosedListTraits>
{
typedef MapStringToClass<ASSNodeRef, AutoArray<ASSNodeRef>, ASSClosedListTraits> base;

public:
  ASSNode *AddNew(const ASSField &field, ASSNode *parent, float g, float f)
  {
    ASSNode *node = new ASSNode(field, parent, g, f);
    base::Add(node);
    return node;
  }
};

class ASSParams
{
public:
  static float LimitCost() {return GET_UNACCESSIBLE;}
  static float BestNodeCoef() {return 0.5;}
};

struct ASSCostFunctions: public AStarCostFunctionsDef<ASSField>
{
  typedef ASSCostFunction CostFunction;
  typedef ASSHeuristicFunction HeuristicFunction;
};

typedef AStarCostFunctionsWrapped<ASSCostFunctions> ASSCostFunctionsWrapped;

typedef AStar<
  ASSField,
  ASSCostFunctions, ASSReadyFunction, ASSInterruptFunction, AStarEndDefault<ASSField>,
  ASSIterator,
  ASSClosedList, ASSOpenList,
  ASSParams
> AStarStrategic;

class AIPathPlanner : public IAIPathPlanner
{
protected:
  // strategic plan
  AutoArray<FieldPassing> _plan;

  // ultimate destination
  PlanPosition _destination;

  // implementation of A*
  SRef<AStarStrategic> _algorithm;
  int _iterTotal;
  float _maxIters;

  float _heuristic;
  bool _searching;

  CostFunction _costFunction;
  void *_param;
  OLink(EntityAI) _vehicle;

#if CHECK_PERFORMANCE
  float _timeTotal;
#endif

public:
  AIPathPlanner(CostFunction func, void *param);
  void Init();
  LSError Serialize(ParamArchive &ar);

  bool IsSearching() const {return _searching;}

  int GetPlanSize() const {return _plan.Size();}
  float GetTotalCost() const;

  int FindBestIndex(Vector3Par pos) const;
  bool GetPlanPosition(int index, PlanPosition &pos) const;
  FieldPassing::Mode GetPlanMode(int index) const;
  bool IsOnPath(int x, int z, int from, int to) const;

  bool StartSearching(AI::ThinkImportance prec, EntityAI *veh, Vector3Par ptStart, const PlanPosition &ptEnd);
  void StopSearching();
  bool ProcessSearching();

  // functions for diagnostics
  const PlanPosition &GetDestination() const {return _destination;}
  Vector3 GetBest() const;
  void GetOpenList(AutoArray<Vector3> &list) const;
  void GetClosedList(AutoArray<Vector3> &list) const;

protected:
  float CalculateHeuristic(int xs, int zs, int xe, int ze);
  float GetFieldCost(int x, int z, bool road) {return _costFunction(x, z, road, true, 0, _param);}
  bool FindNearestSafe(int &x, int &z);

  bool IsSafe(int x, int z)
  {
    return GetFieldCost(x, z, false) < GET_UNACCESSIBLE;
  }

  float Distance(int index, Vector3Par pos) const;
};

IAIPathPlanner *CreateAIPathPlanner(CostFunction func, void *param)
{
  return new AIPathPlanner(func,param);
}

///////////////////////////////////////////////////////////////////////////////
// class AIPathPlanner

AIPathPlanner::AIPathPlanner(CostFunction func, void *param)
{
  _costFunction = func;
  _param = param;

  _iterTotal = 0;

  Init();
}

inline float Heuristic( float dx, float dz )
{
  dx=fabs(dx);
  dz=fabs(dz);
  float minD=(dx+dz-fabs(dx-dz))*0.5;
  float maxD=(dx+dz+fabs(dx-dz))*0.5;
  return ((maxD-minD)+H_SQRT2*minD);
}

void AIPathPlanner::Init()
{
  _plan.Clear();
  _destination = PlanPosition(VZero,false);
  _searching = false;
}

static const EnumName FPModeNames[]=
{
  EnumName(FieldPassing::Move, "MOVE"),
  EnumName(FieldPassing::MoveOnRoad, "ROAD"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(FieldPassing::Mode dummy)
{
  return FPModeNames;
}

LSError FieldPassing::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeEnum("mode", _mode, 1, Move))
  CHECK(::Serialize(ar, "pos", _pos, 1, VZero))
  CHECK(ar.Serialize("cost", _cost, 1))
  return LSOK;
}

LSError AIPathPlanner::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("Plan", _plan, 1))
  CHECK(::Serialize(ar, "destination", safe_cast<Vector3 &>(_destination), 1))
  CHECK(::Serialize(ar, "destinationCover", _destination._cover, 1, false))
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    // do not save/load partial searching
    _searching = false;
    // _costFunction, _param already initialized
  }
  return LSOK;
}

/*!
\patch 5089 Date 11/21/2006 by Jirka
- Fixed: AI sometimes did not found path around mountains
*/

bool AIPathPlanner::StartSearching(AI::ThinkImportance prec, EntityAI *veh, Vector3Par ptStart, const PlanPosition &ptEnd)
{
  if (!veh)
  {
    _searching = false;
    return false;
  }
  _vehicle = veh;
  _destination = ptEnd;

  Assert(!_searching);
  if
  (
    prec == AI::LevelCommands ||
    (
      ptStart.Distance2(ptEnd) < Square(DIST_MIN_OPER) &&
      !veh->GetType()->IsKindOf(GWorld->Preloaded(VTypeAir)) &&
      !veh->GetType()->IsKindOf(GWorld->Preloaded(VTypeShip)) &&
      GLandscape->GetRoadNet()->IsOnRoadReady(_destination) // avoid waiting for files
    )
  )
  {
    // no path will be searched
    _plan.Resize(2);
    _plan[0]._pos = ptStart;
    _plan[0]._mode = FieldPassing::Move;
    _plan[0]._cost = 0;
    _plan[1]._pos = ptEnd;
    _plan[1]._mode = FieldPassing::Move;
    // TODO: perform some cost estimation
    _plan[1]._cost = 0;

    _searching = false;
#if LOG_STRAT
LogF(" - short distance, simple path processed");
#endif
    return true;
  }
  else
  {
    MapCoord xs = toIntFloor(ptStart.X() * InvLandGrid);
    MapCoord zs = toIntFloor(ptStart.Z() * InvLandGrid);
    MapCoord xe = toIntFloor(ptEnd.X() * InvLandGrid);
    MapCoord ze = toIntFloor(ptEnd.Z() * InvLandGrid);

#if LOG_STRAT
    LogF
      (
      "Start searching: %s, from %.0f, %.0f to %.0f, %.0f",
      (const char *)_vehicle->GetDebugName(),
      xs * LandGrid + 0.5 * LandGrid,
      zs * LandGrid + 0.5 * LandGrid,
      xe * LandGrid + 0.5 * LandGrid,
      ze * LandGrid + 0.5 * LandGrid
      );
#endif

    if (GetFieldCost(xs, zs, false) >= GET_UNACCESSIBLE)
    {
#if LOG_POSITION_PROBL
LogF
(
  "Problem: %s, starting point %.0f, %.0f unaccessible",
  (const char *)_vehicle->GetDebugName(),
  xs * LandGrid + 0.5 * LandGrid,
  zs * LandGrid + 0.5 * LandGrid
);
#endif
      if (!FindNearestSafe(xs, zs))
      {
#if LOG_POSITION_PROBL
LogF
(
  "Problem: %s, no accessible point found",(const char *)_vehicle->GetDebugName()
);
#endif
        _searching = false;
#if LOG_STRAT
        LogF(" - unaccessible, no solution");
#endif
        return false; // no solution
      }
#if LOG_POSITION_PROBL
LogF
(
  "Problem: %s, new starting point %.0f, %.0f accessible",
  (const char *)_vehicle->GetDebugName(),
  xs * LandGrid + 0.5 * LandGrid,
  zs * LandGrid + 0.5 * LandGrid
);
#endif
    }
    if (GetFieldCost(xe, ze, false) >= GET_UNACCESSIBLE)
    {
#if LOG_POSITION_PROBL
LogF
(
  "Problem: %s, end point %.0f, %.0f unaccessible",
  (const char *)_vehicle->GetDebugName(),
  xe * LandGrid + 0.5 * LandGrid,
  ze * LandGrid + 0.5 * LandGrid
);

#endif
      if (!FindNearestSafe(xe, ze))
      {
#if LOG_POSITION_PROBL
LogF
(
  "Problem: %s, no accessible point found",(const char *)_vehicle->GetDebugName()
);
#endif
        _searching = false;
#if LOG_STRAT
        LogF(" - unaccessible, no solution");
#endif
        return false; // no solution
      }
#if LOG_POSITION_PROBL
LogF
(
  "Problem: %s, end point %.0f, %.0f accessible",
  (const char *)_vehicle->GetDebugName(),
  xe * LandGrid + 0.5 * LandGrid,
  ze * LandGrid + 0.5 * LandGrid
);
#endif
      // new destination
      _destination[0] = xe * LandGrid + 0.5 * LandGrid;
      _destination[2] = ze * LandGrid + 0.5 * LandGrid;
      _destination[1] = 0;	// not used
    }

    _heuristic = CalculateHeuristic(xs, zs, xe, ze);

    _maxIters = Square(Heuristic(xe - xs, ze - zs)) + 6000.0f;
//LogF("Distance %.0f, max %.0f iterations", ptEnd.Distance(ptStart), _maxIters);

    ASSField start(xs, zs, NULL);
    ASSField end(xe, ze, NULL);
    ASSCostFunctionsWrapped costFcs(
      ASSCostFunction(_costFunction, _param, _vehicle), ASSHeuristicFunction(_heuristic)
    );
    _algorithm = new AStarStrategic
    (
      start, end,
      costFcs,
      ASSReadyFunction(), ASSInterruptFunction(TimePerCycle), AStarEndDefault<ASSField>(end)
    );
    
    // continue witch searching
    _searching = true;
    _iterTotal = 0;
#if CHECK_PERFORMANCE
  _timeTotal = 0;
#endif
#if LOG_STRAT
  LogF(" - searching started");
#endif
#if DIAG_A_STAR
  bool diags = GWorld->CameraOn() == veh;
  if (diags)
  {
    LogF("%.2f %s starts searching of strategic path", Glob.time.toFloat(), cc_cast(veh->GetDebugName()));
  }
#endif
    return true;
  }
}

void AIPathPlanner::StopSearching()
{
  _searching = false;
  _algorithm = NULL;
}
/**
@return true when searching is not finished
*/

bool AIPathPlanner::ProcessSearching()
{
  if (!_vehicle)
  {
    _searching = false;
#if LOG_STRAT
      LogF
      (
        "Strategic path not found - vehicle destroyed"
        );
#endif
    _algorithm = NULL;
    return false;
  }
#if LOG_STRAT
  LogF(
    "Continue with searching: %s, from %.0f, %.0f to %.0f, %.0f",
    (const char *)_vehicle->GetDebugName());
#endif

#if CHECK_PERFORMANCE
  SectionTimeHandle sectionTime = StartSectionTime();
#endif

#if DIAG_A_STAR
  bool diags = GWorld->CameraOn() == _vehicle;
  _iterTotal += _algorithm->Process(NULL, diags);
  if (diags)
  {
    LogF(
      "%.2f %s is searching strategic path, %d iteration processed",
      Glob.time.toFloat(), cc_cast(_vehicle->GetDebugName()), _iterTotal);
  }
#else
  _iterTotal += _algorithm->Process();
#endif

#if CHECK_PERFORMANCE
  float time = GetSectionTime(sectionTime);
  _timeTotal += time;
  if (true)
  {
    LogF(" - strategic path planning takes %.4f s", time);
    LogF(" - total after %d iterations %.3f s", _iterTotal, _timeTotal);
  }
#endif
  if (_algorithm->IsDone())
  {
#if DIAG_A_STAR
    if (diags)
    {
      LogF(
        "%.2f %s finished searching of strategic path",
        Glob.time.toFloat(), cc_cast(_vehicle->GetDebugName()));
    }
#endif
    // make sure all data are loaded
    if (!GLandscape->GetRoadNet()->IsOnRoadReady(_destination))
    {
      return true;	// continue with searching
    }
    _searching = false;
    if (_algorithm->IsFound())
    {
      const ASSNode *last = _algorithm->GetLastNode();

      int depth = 0;
      for (const ASSNode *cur=last; cur!=NULL; cur=cur->_parent) depth++;
      _plan.Resize(depth);
      int i = depth - 1;
      for (const ASSNode *cur=last; cur != NULL; cur=cur->_parent)
      {
        FieldPassing &info = _plan[i--];
        if (cur->_field.coord._road)
        {
          info._pos = cur->_field.coord._road->AIPosition();
          info._mode = FieldPassing::MoveOnRoad;
        }
        else
        {
          info._pos[0] = LandGrid * cur->_field.coord._x + 0.5 * LandGrid;
          info._pos[2] = LandGrid * cur->_field.coord._z + 0.5 * LandGrid;
          info._pos[1] = GLandscape->SurfaceY(info._pos[0], info._pos[2]);
          info._mode = FieldPassing::Move;
        }
        info._cost = cur->_g;
      }

#if LOG_STRAT
      LogF
      (
        "Strategic path found: %s, length %d, cost %.0f (in %d steps, time %.3f):",
        (const char *)_vehicle->GetDebugName(), depth, last->_g, _iterTotal, _timeTotal
      );
      // for (int i=0; i<depth; i++)
      //	LogF("  %d, %d: %.2f", _plan[i]._x, _plan[i]._z, _plan[i]._cost);
#endif

      // patch the last item
      int n = _plan.Size();
      if (n > 0)
      {
        _plan[n - 1]._pos = _destination;
        float size = _vehicle->GetShape() ? _vehicle->GetShape()->GeometrySphere() : 0;
        if (GLandscape->GetRoadNet()->IsOnRoad(_destination, size, size))
          _plan[n - 1]._mode = FieldPassing::MoveOnRoad;
        else
          _plan[n - 1]._mode = FieldPassing::Move;
      }

      _algorithm = NULL;
      return true;
    }
    else
    {
#if LOG_STRAT
      LogF
      (
        "Strategic path not found: %s (in %d steps, time %.3f)",
        (const char *)_vehicle->GetDebugName(), _iterTotal, _timeTotal
      );
#endif
      _algorithm = NULL;
      return false;
    }
  }
  else if (_iterTotal >= _maxIters)
  {
#if DIAG_A_STAR
    if (diags)
    {
      LogF(
        "%.2f %s did not found strategic path in %d iterations",
        Glob.time.toFloat(), cc_cast(_vehicle->GetDebugName()), _iterTotal);
    }
#endif
#if LOG_STRAT
      LogF
      (
        "Strategic path not found - iterations limit reached: %s (in %d steps, time %.3f)",
        (const char *)_vehicle->GetDebugName(), _iterTotal, _timeTotal
      );
#endif
    _searching = false;
    _algorithm = NULL;
    return false;
  }
  else return true;	// continue with searching
}

float AIPathPlanner::CalculateHeuristic(int xs, int zs, int xe, int ze)
{
  float dx = xe - xs;
  float dz = ze - zs;
  int incz = zs < ze ? 1 : -1;
  int incx = xs < xe ? 1 : -1;
  float minCost = FLT_MAX;
  float maxCost = -FLT_MAX;
  float sumCost = 0;
  int count = 0;

// to make roads selectable, we need to set heuristics low enough
#define FIELD_COST(x, z) floatMin(GetFieldCost(x, z, false), GetFieldCost(x, z, true))
  if (fabs(dx) < fabs(dz))
  {
    float invabsdz = 1.0 / fabs(dz);
    dx *= invabsdz;
    float x = xs + 0.5;
    for (int z = zs; z != ze; z+=incz)
    {
      x += dx;
      if (toIntFloor(x) != xs)
      {
        Assert(toIntFloor(x) == xs + incx);
        float cost1 = FIELD_COST(xs, z);
        float cost2 = FIELD_COST(xs + incx, z);
        if (cost1 < GET_UNACCESSIBLE)
        {
          saturateMax(maxCost, cost1);
          saturateMin(minCost, cost1);
          sumCost += cost1;
          count++;
        } 
        if (cost2 < GET_UNACCESSIBLE)
        {
          saturateMax(maxCost, cost2);
          saturateMin(minCost, cost2);
          sumCost += cost2;
          count++;
        }
        xs += incx;
      }
      else
      {
        float cost = FIELD_COST(xs, z);
        if (cost < GET_UNACCESSIBLE)
        {
          saturateMax(maxCost, cost);
          saturateMin(minCost, cost);
          sumCost += cost;
          count++;
        }
      }
    }
  }
  else if (fabs(dx)>0)
  {
    float invabsdx = 1.0 / fabs(dx);
    dz *= invabsdx;
    float z = zs + 0.5;
    for (int x = xs; x != xe; x+=incx)
    {
      z += dz;
      if (toIntFloor(z) != zs)
      {
        Assert(toIntFloor(z) == zs + incz);
        float cost1 = FIELD_COST(x, zs);
        float cost2 = FIELD_COST(x, zs + incz);
        if (cost1 < GET_UNACCESSIBLE)
        {
          saturateMax(maxCost, cost1);
          saturateMin(minCost, cost1);
          sumCost += cost1;
          count++;
        } 
        if (cost2 < GET_UNACCESSIBLE)
        {
          saturateMax(maxCost, cost2);
          saturateMin(minCost, cost2);
          sumCost += cost2;
          count++;
        }
        zs += incz;
      }
      else
      {
        float cost = FIELD_COST(x, zs);
        if (cost < GET_UNACCESSIBLE)
        {
          saturateMax(maxCost, cost);
          saturateMin(minCost, cost);
          sumCost += cost;
          count++;
        }
      }
    }
  }
  else
  {
    //LogF("Singular search");
  }

  if (count <= 0)
  {
/*
    for (int i=0; i<NUnits(); i++)
    {
      AIUnit *unit = GetUnit(i);
      if (!unit || unit->GetInCargo())
        continue;
      float costU = unit->GetVehicle()->GetType()->GetMinCost();
      if (costU > maxCost) maxCost = costU;
    }
    Assert(maxCost < GET_UNACCESSIBLE);
    maxCost *= LandGrid;
    count = 1;
*/
    maxCost = _vehicle->GetType()->GetMinCost();
    Assert(maxCost < GET_UNACCESSIBLE);
    maxCost *= LandGrid;
    minCost = sumCost = maxCost;
    count = 1;
  }
	float vehMinCost = _vehicle->GetType()->GetMinCost()*LandGrid;
	float limitHeuristics = 50*floatMax(minCost,vehMinCost);
	// knowing minCost, avgCost and maxCost
	return floatMin(0.9 * minCost + 0.1 * maxCost, limitHeuristics);
}

bool AIPathPlanner::FindNearestSafe(int &x, int &z)
{
  if (IsSafe(x, z))
    return true;
  int rMax = 10; // do not too far (max 500 m)
  int i, r, xt, zt;
  for (r=1; r<rMax; r++)
  {
    for (i=0; i<r; i++)
    {
      xt = x - i; zt = z - r;
      if (IsSafe(xt, zt)) goto SafeFieldFound;
      xt = x + i; zt = z - r;
      if (IsSafe(xt, zt)) goto SafeFieldFound;
      xt = x + r; zt = z - i;
      if (IsSafe(xt, zt)) goto SafeFieldFound;
      xt = x + r; zt = z + i;
      if (IsSafe(xt, zt)) goto SafeFieldFound;
      xt = x + i; zt = z + r;
      if (IsSafe(xt, zt)) goto SafeFieldFound;
      xt = x - i; zt = z + r;
      if (IsSafe(xt, zt)) goto SafeFieldFound;
      xt = x - r; zt = z + i;
      if (IsSafe(xt, zt)) goto SafeFieldFound;
      xt = x - r; zt = z - i;
      if (IsSafe(xt, zt)) goto SafeFieldFound;
    }
    xt = x - r; zt = z - r;
    if (IsSafe(xt, zt)) goto SafeFieldFound;
    xt = x + r; zt = z - r;
    if (IsSafe(xt, zt)) goto SafeFieldFound;
    xt = x + r; zt = z + r;
    if (IsSafe(xt, zt)) goto SafeFieldFound;
    xt = x - r; zt = z + r;
    if (IsSafe(xt, zt)) goto SafeFieldFound;
  }
  return false;
SafeFieldFound:
  x = xt;
  z = zt;
  return true;
}

float AIPathPlanner::Distance(int index, Vector3Par pos) const
{
  PlanPosition beg, end;
  GetPlanPosition(index, beg);
  GetPlanPosition(index + 1, end);
  Vector3 e = end - beg;
  Vector3 p = pos - beg;
  float denom = e.SquareSizeXZ();
  if (denom<=0)
  {
    return (beg-pos).SquareSizeXZ();
  }
  
  float tNext=(e*p)/denom;
  saturate(tNext,0,1);
  Vector3 nearest=beg+tNext*e;
  float dist2Next=(nearest-pos).SquareSizeXZ();
  return dist2Next;
}

int AIPathPlanner::FindBestIndex(Vector3Par pos) const
{
  int n = _plan.Size();
  if (n == 0) return -1;

  int planIndex = -1;
  float dist2Min = FLT_MAX;
  for (int i=0; i<n-1; i++)
  {
    float dist2 = Distance(i, pos);
    if (dist2 < dist2Min)
    {
      dist2Min = dist2;
      planIndex = i;
    }
  }
  planIndex+=2;
  saturate(planIndex, 0, _plan.Size() - 1);
  return planIndex;
}

bool AIPathPlanner::GetPlanPosition(int index, PlanPosition &pos) const
{
  int n = _plan.Size(); 
  if (n <= 0 || index>=n-1)
  {
    pos = _destination;
    return true;
  }

  saturate(index, 0, n - 1);
  pos = PlanPosition(_plan[index]._pos,false);
  return index == n - 1;
}

FieldPassing::Mode AIPathPlanner::GetPlanMode(int index) const
{
  if (index < 0 || index >= _plan.Size())
  {
    Fail("Out of plan");
    return FieldPassing::Move;
  }

  return (FieldPassing::Mode)_plan[index]._mode;
}

float AIPathPlanner::GetTotalCost() const
{
  int index = _plan.Size() - 1;
  if (index < 0) return 0;

  return _plan[index]._cost;
}

bool AIPathPlanner::IsOnPath(int x, int z, int from, int to) const
{
  if (from < 0 || from >= _plan.Size()) return false;
  if (to < 0 || to >= _plan.Size()) return false;

  for (int i=from; i<=to; i++)
  {
    const FieldPassing &info = _plan[i];
    if (toIntFloor(info._pos.X() * InvLandGrid) == x && toIntFloor(info._pos.Z() * InvLandGrid) == z) return true;
  }
  return false;
}

Vector3 AIPathPlanner::GetBest() const
{
  if (!_algorithm) return VZero;
  const ASSNode *best = _algorithm->GetBest();
  if (!best) return VZero;
  if (best->_field.coord._road)
  {
    return best->_field.coord._road->AIPosition();
  }
  else
  {
    Vector3 pos;
    pos[0] = LandGrid * best->_field.coord._x + 0.5 * LandGrid;
    pos[1] = 0;
    pos[2] = LandGrid * best->_field.coord._z + 0.5 * LandGrid;
    return pos;
  }
}

void AIPathPlanner::GetOpenList(AutoArray<Vector3> &array) const
{
  if (!_algorithm)
  {
    array.Clear();
    return;
  }
  const ASSOpenList &list = _algorithm->GetOpenList();
  int n = list.Size();
  array.Realloc(n);
  array.Resize(n);
  for (int i=0; i<n; i++)
  {
    ASSNode *node = list[i];
    DoAssert(node->_open == 0);
    if (node->_field.coord._road)
    {
      array[i] = node->_field.coord._road->AIPosition();
    }
    else
    {
      array[i][0] = LandGrid * node->_field.coord._x + 0.5 * LandGrid;
      array[i][1] = 0;
      array[i][2] = LandGrid * node->_field.coord._z + 0.5 * LandGrid;
    }
  }
}

void AIPathPlanner::GetClosedList(AutoArray<Vector3> &array) const
{
  array.Clear();
  if (!_algorithm) return;
  const ASSClosedList &list = _algorithm->GetClosedList();
  for (int i=0; i<list.NTables(); i++)
  {
    AutoArray<ASSNodeRef> &table = list.GetTable(i);
    for (int j=0; j<table.Size(); j++)
    {
      ASSNode *node = table[j];
      if (node->_open == 0) continue;
      int k = array.Add();
      if (node->_field.coord._road)
      {
        array[k] = node->_field.coord._road->AIPosition();
      }
      else
      {
        array[k][0] = LandGrid * node->_field.coord._x + 0.5 * LandGrid;
        array[k][1] = 0;
        array[k][2] = LandGrid * node->_field.coord._z + 0.5 * LandGrid;
      }
    }
  }
}

#endif //_ENABLE_AI
