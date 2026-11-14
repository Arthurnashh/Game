// AI - implementation of Units

#include "../wpch.hpp"
#include "ai.hpp"
#include "aiRadio.hpp"
#include "../global.hpp"
#include "../scene.hpp"
#include "../person.hpp"
#include "../diagModes.hpp"

#include "../world.hpp"
#include "../landscape.hpp"
//#include "loadStream.hpp"
#include "../paramArchiveExt.hpp"

#include <El/Common/randomGen.hpp>
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>

#include <El/Evaluator/express.hpp>

//#include "strIncl.hpp"
#include "../stringtableExt.hpp"

#include "../roads.hpp"
#include "operMap.hpp"

#include "../house.hpp"
#include "../Network/network.hpp"
#include "../move_actions.hpp"

#include "../UI/uiActions.hpp"
#include "../UI/chat.hpp"

#include "../arcadeTemplate.hpp"

#include <El/Enum/enumNames.hpp>

#include <El/PreprocC/preprocC.hpp>

#include "../Protect/selectProtection.h"

#include <Es/Algorithms/qsort.hpp>
#include <Es/Algorithms/aStar.hpp>

#include "fsmScripted.hpp"
#include "../fsmEntity.hpp"
#include "../gameStateExt.hpp"

#include "../Inventory/weapon.hpp"

#if _ENABLE_CONVERSATION
#include "../KBExt/kbExt.hpp"
#endif

#if 0 //_PROFILE
#pragma optimize( "", off )
#endif

///////////////////////////////////////////////////////////////////////////////
// Parameters

#include "aiDefs.hpp"

// speed in nodes
#define SPEED_COEF  6.0F

// maximal length for operative path planning
#define MAX_OPER_PATH (6 * LandGrid)

/** dir is not used for strategic planning */
// A* algoritm for searching the best path
int directions20[20][3] =
{
  // dx,dz,dir
  {0,-1,0*16},
  {-1,-2,1*16},
  {-1,-1,2*16},
  {-2,-1,3*16},
  {-1,0,4*16},
  {-2,1,5*16},
  {-1,1,6*16},
  {-1,2,7*16},
  {0,1,8*16},
  {1,2,9*16},
  {1,1,10*16},
  {2,1,11*16},
  {1,0,12*16},
  {2,-1,13*16},
  {1,-1,14*16},
  {1,-2,15*16},
  {0,-2,0*16}, // directions 16..19 not used for operative planning
  {-2,0,4*16},
  {0,2,8*16},
  {2,0,12*16}
};

// exposures
#define VALID_EXPOSURE_CHANGE     10.0F

#define DIAG_PLANNING             0

///////////////////////////////////////////////////////////////////////////////
// class AILocker

// avoid inlined construct/destruct
AILocker::AILocker()
: _tempLocked(false),
_lockedSoldier(false),
_lockedBeg(VZero),
_lockedRadius(0),
_lockedSize(0),
_lastMovementTime(TIME_MIN)
{

} 

AILocker::~AILocker() {}

void AILocker::LockItem(OperMapIndex x, OperMapIndex z, bool soldier)
{
  if (x < 0 || x >= OperItemRange * LandRange || z < 0 || z >= OperItemRange * LandRange)
    return;

  SPLIT_INDICES(x, z, x0, z0, x1, z1);

  Ref<LockField> fld;
  for (int i=0; i<_fields.Size(); i++)
    if (_fields[i]->X() == x0 && _fields[i]->Z() == z0)
    {
      fld = _fields[i];
      break;
    }
  if (fld == NULL)
  // note: field may be locked twice with the same locker
  {
    fld = GLOB_LAND->LockingCache()->GetField(x0, z0, true);
    _fields.Add(fld);
  }
  fld->Lock(x1, z1, soldier, true);
}

void AILocker::UnlockItem(OperMapIndex x, OperMapIndex z, bool soldier)
{
  if (x < 0 || x >= OperItemRange * LandRange || z < 0 || z >= OperItemRange * LandRange)
    return;

  SPLIT_INDICES(x, z, x0, z0, x1, z1);

  LockField *fld = NULL;
  for (int i=0; i<_fields.Size(); i++)
    if (_fields[i]->X() == x0 && _fields[i]->Z() == z0)
    {
      fld = _fields[i];
      break;
    }
  if (fld == NULL)
  {
    Fail("Cannot unlock item that was not locked.");
    return;
  }
  fld->Lock(x1, z1, soldier, false);
}

const float ManLockRadius = 1.0;

void AILocker::Lock(Vector3Val pos, float radius, bool soldier, bool canWait, float size)
{
  bool waiting = canWait && Glob.time.Diff(_lastMovementTime)<20.0f; // 20 seconds is quite a long time inga,e
  // if already locked, decide if unlock / lock or keep current lock based on the position distance
  if (_tempLocked)
  {
    if (_lockedWaiting==waiting)
    {
    // avoid too frequent locking - do not lock when change is much less than OperItemGrid
    float dist2 = (pos - _lockedBeg).SquareSizeXZ();
    if (dist2 < Square(OperItemGrid * 0.4f)) return;
      _lastMovementTime = Glob.time;
    }
    Unlock();
  }
  Assert(!_tempLocked);

  float allRadius = soldier ? ManLockRadius : floatMax(4, radius * 0.6);

  // protect only write, called by the main thread only
  ScopeLockSection lock(_criticalSection);

  _lockedBeg = pos;
  _lockedRadius = radius;
  _lockedSize = size;
  _lockedSoldier = soldier;
  _lockedWaiting = waiting;

  // lock larger circle for vehicles only
  LockPosition(_lockedBeg, _lockedRadius, _lockedSoldier, _lockedWaiting, _lockedSize);
  // lock smaller circle for both soldiers and vehicles
  LockPositionAll(_lockedBeg, allRadius, _lockedSoldier, _lockedWaiting, _lockedSize);
  _tempLocked = true;
}

void AILocker::Move()
{
  _lastMovementTime = Glob.time;
}
void AILocker::Unlock(SavedState *state)
{
  if (_tempLocked)
  {
    float allRadius = _lockedSoldier ? ManLockRadius : floatMax(4, _lockedRadius * 0.6);

    // protect only write, called by the main thread only
    ScopeLockSection lock(_criticalSection);

    // unlock what was locked in both layers
    UnlockPositionAll(_lockedBeg, allRadius, _lockedWaiting, state);
    UnlockPosition(_lockedBeg, _lockedRadius, _lockedSoldier, _lockedWaiting, state);
    _tempLocked = false;
  }
}

void AILocker::Relock(SavedState &state)
{
  Assert(!_tempLocked);
  if (!_tempLocked)
  {
    float allRadius = _lockedSoldier ? ManLockRadius : floatMax(4, _lockedRadius * 0.6);

    // protect only write, called by the main thread only
    ScopeLockSection lock(_criticalSection);

    // lock larger circle for vehicles only
    RelockPosition(_lockedBeg, _lockedRadius, _lockedSoldier, _lockedWaiting, _lockedSize, state);
    // lock smaller circle for both soldiers and vehicles
    RelockPositionAll(_lockedBeg, allRadius, _lockedWaiting, state);
    _tempLocked = true;
  }
}

inline int MaxLockRange() {return toInt(50 * InvOperItemGrid);}

int AILocker::IsLocking(OperMapIndex x, OperMapIndex z, bool soldier) const
{
  // protect all, called from worker threads
  ScopeLockSection lock(_criticalSection);

  if (!_tempLocked) return 0;

  int locks = 0;
  
  OperMapIndex xBeg(toIntFloor(_lockedBeg.X() * InvOperItemGrid));
  OperMapIndex zBeg(toIntFloor(_lockedBeg.Z() * InvOperItemGrid));

  // locking by LockMaskAll
  if (x == xBeg && z == zBeg)
  {
    locks = 0x10;
  }
  else
  {
    float radius = _lockedSoldier ? ManLockRadius : floatMax(4, _lockedRadius * 0.6);

    int lockRange = toIntCeil(radius * InvOperItemGrid);
    if (lockRange < 0) lockRange = 0;
    if (!(lockRange<MaxLockRange())) {lockRange = MaxLockRange();}

    if (x >= xBeg - lockRange && x <= xBeg + lockRange && z >= zBeg - lockRange && z <= zBeg + lockRange)
    {
      float xr = (x + 0.5f) * OperItemGrid - _lockedBeg.X();
      float zr = (z + 0.5f) * OperItemGrid - _lockedBeg.Z();
      if (Square(xr) + Square(zr) < Square(radius)) locks = 0x10;
    }
  }

  // locking by LockMaskVeh (soldier can ignore)
  if (!soldier)
  {
    if (x == xBeg && z == zBeg)
    {
      locks |= 0x01;
    }
    else
    {
      float radius = _lockedRadius;

      int lockRange = toIntCeil(radius * InvOperItemGrid);
      if (lockRange < 0) lockRange = 0;
      if (!(lockRange<MaxLockRange())) {lockRange = MaxLockRange();}

      if (x >= xBeg - lockRange && x <= xBeg + lockRange && z >= zBeg - lockRange && z <= zBeg + lockRange)
      {
        float xr = (x + 0.5f) * OperItemGrid - _lockedBeg.X();
        float zr = (z + 0.5f) * OperItemGrid - _lockedBeg.Z();
        if (Square(xr) + Square(zr) < Square(radius)) locks |= 0x01;
      }
    }
  }
  
  return locks;
}

int AILocker::IsLocking(const RoadLink *road, bool soldier) const
{
  // protect all, called from worker threads
  ScopeLockSection lock(_criticalSection);

  if (!_tempLocked) return 0;

  int locks = 0;
  // locking by LockMaskAll
  for (int i=0; i<_roadsVehicles.Size(); i++)
  {
    if (_roadsVehicles[i] == road)
    {
      locks = 0x10;
      break;
    }
  }

  // locking by LockMaskVeh (soldier can ignore)
  if (!soldier)
  {
    for (int i=0; i<_roadsSoldiers.Size(); i++)
    {
      if (_roadsSoldiers[i] == road)
      {
        locks |= 0x01;
        break;
      }
    }
  }

  return locks;
}

int AILocker::IsLocking(const Object *house, int index) const
{
  // protect all, called from worker threads
  ScopeLockSection lock(_criticalSection);

  if (!_tempLocked) return 0;

  for (int i=0; i<_buildings.Size(); i++)
  {
    const BuildingLockInfo &info = _buildings[i];
    if (info.house == house && info.index == index) return 1;
  }
  return 0;
}

void AILocker::LockPosition( Vector3Val pos, float radius, bool soldier, bool waiting, float size)
{
  int lockRange = toIntCeil(radius * InvOperItemGrid);
  if (lockRange < 0) lockRange = 0;
  if (!(lockRange<MaxLockRange())) {lockRange = MaxLockRange();ErrF("Bad lock radius %g",radius);}

  // lock the fields
  {
    OperMapIndex x(toIntFloor(pos.X() * InvOperItemGrid));
    OperMapIndex z(toIntFloor(pos.Z() * InvOperItemGrid));
    for (OperMapIndex xx(x-lockRange); xx<=x+lockRange; xx++)
      for (OperMapIndex zz(z-lockRange); zz<=z+lockRange; zz++)
      {
        float xr=(xx+0.5f)*OperItemGrid-pos.X();
        float zr=(zz+0.5f)*OperItemGrid-pos.Z();
        if ( xr*xr+zr*zr<radius*radius || xx==x && zz==z )
        {
          LockItem(xx, zz, true);
          //LogF("DoLock  %d %d,%d",soldier, xx, zz);
        }
      }
  }

  // lock the roads
  {
    Assert( GLandscape->GetRoadNet() );
    LandIndex x(toIntFloor(pos.X() * InvLandGrid));
    LandIndex z(toIntFloor(pos.Z() * InvLandGrid));
    // lock road item if vehicle is inside "boundingSphere" + size
    for (LandIndex zz(z-1); zz<=z+1; zz++)
      for (LandIndex xx(x-1); xx<=x+1; xx++)
      {
        if (!InRange(xx, zz)) continue;
        const RoadList &roadList = GLandscape->GetRoadNet()->GetRoadList(xx, zz);
        int i;
        for (i=0; i<roadList.Size(); i++)
        {
          RoadLink *item = roadList[i];
          if (item->IsInside(pos, size, size))
          {
            item->Lock(true,waiting);
            _roadsSoldiers.Add(item);
          }
        }
      }
  }

  if (soldier)
  {
    // this is not clear - pos is AimingPosition, we need position on surface
    Vector3 realPos = pos - Vector3(0, 0, 0); // updated based on testing

    // lock position in house
    int xMin, xMax, zMin, zMax;
    ObjRadiusRectangle(xMin, xMax, zMin, zMax, pos, pos, 50);
    for (int x=xMin; x<=xMax; x++) for(int z=zMin; z<=zMax; z++)
    {
      const ObjectListUsed &list = GLandscape->UseObjects(x,z);
      int n = list.Size();
      for (int i=0; i<n; i++)
      {
        Object *obj = list[i];

        IPaths *building = unconst_cast(obj->GetIPaths());
        if (!building) continue;
        for (int j=0; j<building->NPos(); j++)
        {
          Vector3 dest = building->GetPosition(building->GetPos(j));
          float dist2 = dest.Distance2(realPos);
          if (dist2 <= Square(size))
          {
            building->Lock(j);
            int index = _buildings.Add();
            _buildings[index].house = unconst_cast(building->GetObject());
            _buildings[index].index = j;
          }
        }
      }
    }
  }
}

void AILocker::RelockPosition( Vector3Val pos, float radius, bool soldier, bool waiting, float size, SavedState &state)
{
  int lockRange = toIntCeil(radius * InvOperItemGrid);
  if (lockRange < 0) lockRange = 0;
  if (!(lockRange<MaxLockRange())) {lockRange = MaxLockRange();ErrF("Bad lock radius %g",radius);}

  OperMapIndex x(toIntFloor(pos.X() * InvOperItemGrid));
  OperMapIndex z(toIntFloor(pos.Z() * InvOperItemGrid));
  for (OperMapIndex xx(x-lockRange); xx<=x+lockRange; xx++)
    for(OperMapIndex zz(z-lockRange); zz<=z+lockRange; zz++)
    {
      float xr=(xx+0.5f)*OperItemGrid-pos.X();
      float zr=(zz+0.5f)*OperItemGrid-pos.Z();
      if ( xr*xr+zr*zr<radius*radius || xx==x && zz==z )
      {
        LockItem(xx, zz, true);
        //LogF("DoLock  %d %d,%d",soldier, xx, zz);
      }
    }

  _roadsSoldiers.Move(state._roadsSoldiers);

  // consider: we might avoid allocation, and move the array content instead
  // no need to search for roads - relock what was locked before
  for (int i=0; i<_roadsSoldiers.Size(); i++)
  {
    _roadsSoldiers[i]->Lock(true,waiting);
  }

  if (soldier)
  {
    _buildings.Move(state._buildings);

    // unlock position in house
    for (int i=0; i<_buildings.Size(); i++)
    {
      if (_buildings[i].house)
      {
        IPaths *building = unconst_cast(_buildings[i].house->GetIPaths());
        building->Lock(_buildings[i].index);
      }
    }
  }
}



void AILocker::UnlockPosition( Vector3Val pos, float radius, bool soldier, bool waiting, SavedState *state)
{
  int lockRange = toIntCeil(radius * InvOperItemGrid);
  if (lockRange < 0) lockRange = 0;
  if (!(lockRange<MaxLockRange())) {lockRange = MaxLockRange();ErrF("Bad lock radius %g",radius);}

  OperMapIndex x(toIntFloor(pos.X() * InvOperItemGrid));
  OperMapIndex z(toIntFloor(pos.Z() * InvOperItemGrid));
//Log("%s:Unlocking %d, %d", (const char *)GetDebugName(), x, z);
  for (OperMapIndex xx(x-lockRange); xx<=x+lockRange; xx++)
    for (OperMapIndex zz(z-lockRange); zz<=z+lockRange; zz++)
    {
      float xr=(xx+0.5f)*OperItemGrid-pos.X();
      float zr=(zz+0.5f)*OperItemGrid-pos.Z();
      if( xr*xr+zr*zr<radius*radius || xx==x && zz==z )
      {
        UnlockItem(xx, zz, true);
        //LogF("UnLock  %d %d,%d",soldier, xx, zz);
      }
    }

  int n = _roadsSoldiers.Size();
  for (int i=0; i<n; i++)
    if (_roadsSoldiers[i]) _roadsSoldiers[i]->Unlock(true,waiting);

  if (soldier)
  {
    if (_buildings.Size() > 0 && !_buildings.Data())
    {
      ErrF
      (
        "Error: AILocker::_buildings corrupted - data NULL, size %d when unlocking position [%.0f, %.0f], radius %.0f, soldier %s",
        _buildings.Size(), pos.X(), pos.Z(), radius, soldier ? "true" : "false"
      );
    }
    // unlock position in house
    for (int i=0; i<_buildings.Size(); i++)
    {
      if (_buildings[i].house)
      {
        IPaths *building = unconst_cast(_buildings[i].house->GetIPaths());
        building->Unlock(_buildings[i].index);
      }
    }
  }

  _fields.Resize(0);
  // TODO: some smart deallocation scheme
  // avoid dealocation
  //_fields.Clear();
  //_roads.Clear();
  if (state)
  {
    state->_roadsSoldiers.Move(_roadsSoldiers);
    state->_buildings.Move(_buildings);
  }
  else
  {
    _roadsSoldiers.Resize(0);
    _buildings.Resize(0);
  }
}

void AILocker::LockPositionAll( Vector3Val pos, float radius, bool soldier, bool waiting, float size)
{
  int lockRange = toIntCeil(radius * InvOperItemGrid);
  if (lockRange < 0) lockRange = 0;
  if (!(lockRange<MaxLockRange())) {lockRange = MaxLockRange();ErrF("Bad lock radius %g",radius);}

  {
    OperMapIndex x(toIntFloor(pos.X() * InvOperItemGrid));
    OperMapIndex z(toIntFloor(pos.Z() * InvOperItemGrid));
    for (OperMapIndex xx(x-lockRange); xx<=x+lockRange; xx++)
      for (OperMapIndex zz(z-lockRange); zz<=z+lockRange; zz++)
      {
        float xr=(xx+0.5f)*OperItemGrid-pos.X();
        float zr=(zz+0.5f)*OperItemGrid-pos.Z();
        if ( xr*xr+zr*zr<radius*radius || xx==x && zz==z )
        {
          //LogF("DoLockM %d %d,%d",false, xx, zz);
          LockItem(xx, zz, false);
        }
      }
  }

  if (!soldier) // soldier does not lock road for soldiers
  {
    Assert( GLandscape->GetRoadNet() );
    LandIndex x(toIntFloor(pos.X() * InvLandGrid));
    LandIndex z(toIntFloor(pos.Z() * InvLandGrid));
    // lock road item if vehicle is inside "boundingSphere" + size
    for (LandIndex zz(z-1); zz<=z+1; zz++)
      for (LandIndex xx(x-1); xx<=x+1; xx++)
      {
        if (!InRange(xx, zz)) continue;
        const RoadList &roadList = GLandscape->GetRoadNet()->GetRoadList(xx, zz);
        int i;
        for (i=0; i<roadList.Size(); i++)
        {
          RoadLink *item = roadList[i];
          if (item->IsInside(pos, size, size))
          {
            item->Lock(false,waiting);
            _roadsVehicles.Add(item);
          }
        }
      }
  }
}

void AILocker::RelockPositionAll( Vector3Val pos, float radius, bool waiting, SavedState &state)
{
  int lockRange = toIntCeil(radius * InvOperItemGrid);
  if (lockRange < 0) lockRange = 0;
  if (!(lockRange<MaxLockRange())) {lockRange = MaxLockRange();ErrF("Bad lock radius %g",radius);}

  OperMapIndex x(toIntFloor(pos.X() * InvOperItemGrid));
  OperMapIndex z(toIntFloor(pos.Z() * InvOperItemGrid));
  for (OperMapIndex xx(x-lockRange); xx<=x+lockRange; xx++)
    for (OperMapIndex zz(z-lockRange); zz<=z+lockRange; zz++)
    {
      float xr=(xx+0.5f)*OperItemGrid-pos.X();
      float zr=(zz+0.5f)*OperItemGrid-pos.Z();
      if ( xr*xr+zr*zr<radius*radius || xx==x && zz==z )
      {
        //LogF("DoLockM %d %d,%d",false, xx, zz);
        LockItem(xx, zz, false);
      }
    }

  // avoid allocation, and move the array content instead
  _roadsVehicles.Move(state._roadsVehicles);

  // no need to search for roads - relock what was locked before
  for (int i=0; i<_roadsVehicles.Size(); i++)
  {
    _roadsVehicles[i]->Lock(false,waiting);
  }
}

void AILocker::UnlockPositionAll( Vector3Val pos, float radius, bool waiting, SavedState *state)
{
  int lockRange = toIntCeil(radius * InvOperItemGrid);
  if (lockRange < 0) lockRange = 0;
  if (!(lockRange<MaxLockRange())) {lockRange = MaxLockRange();ErrF("Bad lock radius %g",radius);}

  OperMapIndex x(toIntFloor(pos.X() * InvOperItemGrid));
  OperMapIndex z(toIntFloor(pos.Z() * InvOperItemGrid));
  for (OperMapIndex xx(x-lockRange); xx<=x+lockRange; xx++)
    for (OperMapIndex zz(z-lockRange); zz<=z+lockRange; zz++)
    {
      float xr=(xx+0.5f)*OperItemGrid-pos.X();
      float zr=(zz+0.5f)*OperItemGrid-pos.Z();
      if ( xr*xr+zr*zr<radius*radius || xx==x && zz==z )
      {
        //LogF("UnLockM %d %d,%d",false, xx, zz);
        UnlockItem(xx, zz, false);
      }
    }

  int n = _roadsVehicles.Size();
  for (int i=0; i<n; i++)
    if (_roadsVehicles[i]) _roadsVehicles[i]->Unlock(false,waiting);

  // TODO: some smart deallocation scheme
  // avoid dealocation
  //_roads.Clear();
  if (state)
  {
    state->_roadsVehicles.Move(_roadsVehicles);
  }
  else
  {
    _roadsVehicles.Resize(0);
  }
}

///////////////////////////////////////////////////////////////////////////////
// class AIUnit

const Vector3 VUndefined(0, 1e9, 0);

static float AIUnitGetFieldCost(int x, int z, bool road, bool includeGradient, float gradient, void *param)
{
  if (!param) return SET_UNACCESSIBLE;
  AIBrain *unit = (AIBrain *)param;
  switch (unit->GetPlanningMode())
  {
  case AIBrain::DoNotPlan:
  case AIBrain::LeaderDirect:
  default:
    {
      Fail("planning mode");
      return SET_UNACCESSIBLE;
    }
  case AIBrain::LeaderPlanned:
  case AIBrain::VehiclePlanned:
    {
      // base cost
      EntityAI *veh = unit->GetVehicle();
      GeographyInfo geogr = GLandscape->GetGeography(x, z);
      geogr.u.road = road; // if we are searching on road is now given externally
      float cost = veh->GetBaseCost(geogr, false, includeGradient) * veh->GetFieldCost(geogr);
      if (!includeGradient) cost *= veh->GetGradientPenalty(gradient); // exact gradient is used
      cost *= LandGrid; // cost = time for distance == LandGrid
      saturateMin(cost, SET_UNACCESSIBLE); // avoid inf. in cost
      return cost;
    }
  }
}

DEFINE_ENUM(BehaviourFSMRole, BFSM, BEHAVIOUR_FSM_ENUM)
DEFINE_ENUM(ReactionFSMRole, RFSM, REACTION_FSM_ENUM)

#include <Es/Memory/normalNew.hpp>

class AIBrainFSMScripted : public FSMScriptedTyped<Person>
{
  typedef FSMScriptedTyped<Person> base;

protected:
  DECL_SERIALIZE_TYPE_INFO(AIBrainFSMScripted, FSM)

public:
  AIBrainFSMScripted(const FSMScriptedType *type, GameDataNamespace *globals, GameVarSpace *parentVars) : base(type, globals, parentVars) {}
  AIBrainFSMScripted(ForSerializationOnly dummy) : base(dummy) {}

  virtual void ApplyContext(GameState &state, void *context)
  {
    Person *person = reinterpret_cast<Person *>(context);
    state.VarSetLocal("_this", GameValueExt(person), true);
  }

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR(AIBrainFSMScripted)
DEFINE_SERIALIZE_TYPE_INFO(AIBrainFSMScripted, FSM)

FSM *AIBrain::CreateFSM(const EntityType *type, RString name)
{
  if (name.GetLength() == 0) return NULL;
  FSM *ret = NULL;
  if ((Pars >> "CfgFSMs").FindEntry(name))
  {
    FSMEntityType *fsmType = FSMEntityTypes.New(FSMEntityTypeName(type, name));
    if (!fsmType) return NULL;
    ret = new FSMEntity(fsmType);
  }
  else
  {
    FSMScriptedType *fsmType = FSMScriptedTypes.New(name);
    if (!fsmType) return NULL;
    ret = new AIBrainFSMScripted(fsmType, GWorld->GetMissionNamespace(), NULL); // mission namespace
  }
  if (_fsmAutoDebug>=2)
  {
    ret->Debug();
  }
  return ret;
}

DEFINE_ENUM(SuppressState, Suppress, SUPPRESS_ENUM)

#pragma warning(disable:4355)

AIBrain::AIBrain(Person *vehicle)
: _person(vehicle),
  _loadIdentityDelayed(true)
#if _ENABLE_AI
, _planner(CreateAIPathPlanner(AIUnitGetFieldCost, this))
#endif
{
  _lifeState = LifeStateAlive;
  _disabledAI = 0;
#if !_ENABLE_IDENTITIES
  _ability = 1.0;
#endif
#if DEBUG_FSM
  _fsmAutoDebug = 0;
#endif
  _combatModeMajor = CMAware;
  _semaphore = SemaphoreYellow;
  _dangerUntil = Glob.time - 60.0f;
  _state = Wait;
  _planningMode = DoNotPlan;
  _mode = Normal;
  _iter = 0;
  _completedReceived = false;
  _maxCostToCover=0;
  _maxDelayByCover=0;
  _enemyDirL = VZero;
  _enemyDirR = VZero;
  _delay = Time(0);

  _expPosition = PlanPosition(VZero,false);
  _from = VZero;
  _fromBusy = false;
  _toRoad = false;

  _wantedPositionWanted = PlanPosition(VUndefined,false);
  _planningModeWanted = DoNotPlan;
  _forceReplanWanted = false;

  _forceSpeed = -1; //no forceSpeed as default

#if _VBS3
  _allowSpeech = true;
#endif
  _wantedPosition = PlanPosition(VUndefined,false);
  _plannedPosition = PlanPosition(VUndefined,false);
  // by default path finding is enabled
  _planParametersSet = true;
  _wantedPrecision = 0;

  ClearStrategicPlan();

  SetPathCosts(0, 0);
  _pathId = 0;

  _nearestEnemyDist2 = 100;

  _captive = 0;

  _watchDirHead = VZero;
  _watchDir = VZero;
  _watchPos = VZero;
  _glancePos = VZero;
  _watchMode = WMNo;

  SetSpeaker(RString(), PITCH_DEFAULT);

#if _ENABLE_DIRECT_MESSAGES
  _selectedChannel = SCAuto;
  _gestureEnabled = GEAuto;
  _voiceEnabled = VEAuto;
#endif

#if _ENABLE_CONVERSATION
  _speaking = 0;
  _listening = 0;
#endif

  // Create long-term FSMs
  const EntityType *type = vehicle ? vehicle->GetType() : NULL;
  if (type)
  {
    const ParamClass *cls = type->GetParamEntry();
    if (cls)
    {
      for (int i=0; i<NBehaviourFSMRole; i++)
      {
        RString name = RString("fsm") + FindEnumName((BehaviourFSMRole)i);
        _behaviourFSMs[i] = CreateFSM(type, (*cls) >> name);
      }
    }
  }
}

const OperMap &AIBrain::GetOperMap() const
{
  if(_map.IsNull())
    _map = new OperMap(true);
  return *_map;
}

OperMap &AIBrain::GetOperMap()
{
  if(_map.IsNull())
    _map = new OperMap(true);
  return *_map;
}

void AIBrain::SetPathCosts(float costReplan, float costForceReplan)
{
  _path.SetReplanCost(costReplan);
  _path.SetForceReplanCost(costForceReplan);
  _path.ResetMinCost();
}

void AIBrain::SetPerson(Person *vehicle)
{
  _person = vehicle;

  // re-create long-term FSMs
  for (int i=0; i<NBehaviourFSMRole; i++)
    _behaviourFSMs[i] = NULL;

  const EntityType *type = vehicle ? vehicle->GetType() : NULL;
  if (type)
  {
    const ParamClass *cls = type->GetParamEntry();
    if (cls)
    {
      for (int i=0; i<NBehaviourFSMRole; i++)
      {
        RString name = RString("fsm") + FindEnumName((BehaviourFSMRole)i);
        _behaviourFSMs[i] = CreateFSM(type, (*cls) >> name);
      }
    }
  }
}

EntityAIFull *AIBrain::GetVehicle() const
{
  EntityAIFull *veh = _inVehicle;
  if (veh) return veh;
  else return _person;
}


bool AIBrain::FindFreePosition
(
  Vector3 &pos, Vector3 &normal, bool soldier, const EntityAIType *vehType,
  FindFreePositionCallback *isFree, void *context
)
{
  OperMap map;
  int mask = MASK_AVOID_OBJECTS | MASK_AVOID_VEHICLES;

  float xf = pos.X() * InvOperItemGrid;
  float zf = pos.Z() * InvOperItemGrid;
  OperMapIndex x(toIntFloor(xf));
  OperMapIndex z(toIntFloor(zf));
  LAND_INDICES(x, z, xLand, zLand);

  int border = 1;
  LandIndex xMin(xLand - border);
  LandIndex xMax(xLand + border + 1);
  LandIndex zMin(zLand - border);
  LandIndex zMax(zLand + border + 1);
  float dx,dz;
  if
  (
    xMax <= 0 || xMin >= LandRange ||
    zMax <= 0 || zMin >= LandRange
  )
  {
    pos[1] = GLandscape->SurfaceYAboveWater(pos[0], pos[2]);
    normal=VUp;
//pos[1] = GLOB_LAND->GetSeaLevel();
//  Log("Out of map");
    return false;
  }
  //const EntityAIType *vehType = veh->GetType();
  if (xMin < 0) xMin = LandIndex(0);
  if (xMax > LandRange) xMax = LandIndex(LandRange);
  if (zMin < 0) zMin = LandIndex(0);
  if (zMax > LandRange) zMax = LandIndex(LandRange);
  for (LandIndex zz(zMin); zz<zMax; zz++)
    for (LandIndex xx(xMin); xx<xMax; xx++)
      map.CreateField(xx, zz, mask, vehType, CMAware, soldier);

  bool ok = true;
  if
  (
    map.GetFieldCost(x, z, true, vehType, CMAware, soldier, false).cost >= GET_UNACCESSIBLE ||
    !isFree(pos,context)
  )
  {
    ok = map.FindNearestEmpty
    (
      x, z, xf, zf,
      OperMapIndex(xMin * OperItemRange), OperMapIndex(zMin * OperItemRange),
      OperMapIndex((xMax - xMin) * OperItemRange), OperMapIndex((zMax - zMin) * OperItemRange),
      true, vehType, soldier, true, isFree, context
    );

    if (ok)
    {
      Assert(map.GetFieldCost(x, z, true, vehType, CMAware, soldier, false).cost < GET_UNACCESSIBLE);
    }


    pos[0] = x * OperItemGrid + 0.5 * OperItemGrid;
    pos[2] = z * OperItemGrid + 0.5 * OperItemGrid;
    pos[1] = GLOB_LAND->RoadSurfaceYAboveWater(pos[0], pos[2], &dx, &dz);
  }
  else
  {
    // given field is accessible. Check neighbouring fields.
    // If all are accessible, do not change pos, at it is certainly valid
    // ignore locks
    if
    (
      map.GetFieldCost(OperMapIndex(x-1), OperMapIndex(z-1), false, vehType, CMAware, soldier, false).cost >= GET_UNACCESSIBLE ||
      map.GetFieldCost(x,                 OperMapIndex(z-1), false, vehType, CMAware, soldier, false).cost >= GET_UNACCESSIBLE ||
      map.GetFieldCost(OperMapIndex(x+1), OperMapIndex(z-1), false, vehType, CMAware, soldier, false).cost >= GET_UNACCESSIBLE ||
      map.GetFieldCost(OperMapIndex(x-1), z,                 false, vehType, CMAware, soldier, false).cost >= GET_UNACCESSIBLE ||
      map.GetFieldCost(OperMapIndex(x+1), z,                 false, vehType, CMAware, soldier, false).cost >= GET_UNACCESSIBLE ||
      map.GetFieldCost(OperMapIndex(x-1), OperMapIndex(z+1), false, vehType, CMAware, soldier, false).cost >= GET_UNACCESSIBLE ||
      map.GetFieldCost(x,                 OperMapIndex(z+1), false, vehType, CMAware, soldier, false).cost >= GET_UNACCESSIBLE ||
      map.GetFieldCost(OperMapIndex(x+1), OperMapIndex(z+1), false, vehType, CMAware, soldier, false).cost >= GET_UNACCESSIBLE
    )
    {
      pos[0] = x * OperItemGrid + 0.5 * OperItemGrid;
      pos[2] = z * OperItemGrid + 0.5 * OperItemGrid;
      pos[1] = GLOB_LAND->RoadSurfaceYAboveWater(pos[0], pos[2], &dx, &dz);

    }
    else
    {
      pos[1] = GLOB_LAND->RoadSurfaceYAboveWater(pos, &dx, &dz);
    }
  }

  map.ClearMap();
  if (soldier)
  {
    normal=VUp;
  }
  else
  {
    // use difference
    if (vehType->GetShape())
      pos[1] -= vehType->GetShape()->Min().Y();
    normal=Vector3(-dx,1,-dz);
  }
//  Log("Field %d, %d - OK = %d", x, z, ok);
  return ok;
}

/// when getting out of the vehicle, Gunner (optics) camera is never used

void MoveToGetInPos(AIBrain *unit, Transport &veh, const Vector3& shift, const GetInPoint &pt0)
{
  GetInPoint pt = pt0;

  Matrix4 trans;
  trans.SetUpAndDirection(VUp, pt._dir);

  Vector3 realShift = shift + Vector3(0,0,-0.4f);
  pt._pos += trans.Rotate(realShift) ;

  pt._pos[1] = GLandscape->RoadSurfaceYAboveWater(pt._pos+VUp*0.5f);

  trans.SetPosition(pt._pos);
  unit->GetPerson()->Move(trans);
  unit->SetWantedPosition(PlanPosition(pt._pos,false), 0, AIBrain::DoNotPlan, true);

#if 0
  // adjust camera position
  if (unit == GWorld->FocusOn())
    GWorld->InitCameraPars();
#else
  GWorld->FocusOn();
#endif
}


CameraType ValidateCamera(CameraType cam)
{
  if (cam == CamGunner) return CamInternal;
  return cam;
}

/*!
\patch_internal 1.03 Date 7/12/2001 by Jirka - changed placement of Czech CD Protection
- removed: Get in gunner position (CDPGunnerGetIn)
- removed: Heal (CDPHeal)
- added: Beginning of mission (DisplayMission::DisplayMission)
- added: Saving (World::SaveBin)
*/
void __cdecl CDPGunnerGetIn(Transport *veh, AIBrain *brain)
{
  SECUROM_MARKER_SECURITY_ON(1)
  Person *person = brain->GetPerson();

  DoAssert (veh->IsLocal());
  veh->GetInGunner(person);
  if (!veh->CanCancelStop())
  {
    Verify(brain->SetState(AIBrain::Stopped));
  }

  // fixed - remote getin can fail - do not switch camera yet (will be switched later)
  TurretContext context;
  bool valid = veh->GetPrimaryGunnerTurret(context);
  if (GLOB_WORLD->FocusOn() == brain && (veh->IsLocal() || (valid && context._turret && context._turret->IsLocal())))
  {
    GLOB_WORLD->SwitchCameraTo(veh, ValidateCamera(GLOB_WORLD->GetCameraType()), false);
    veh->StartFadeIn(brain);
  }

  SECUROM_MARKER_SECURITY_OFF(1)
}

/*!
\patch 1.01 Date 06/11/2001 by Jirka
- Fixed: respawn in MP - problems if player killed during get in animation
- do not continue with get in if unit is dead
- camera can switch to dead body
\patch 1.02 Date 07/12/2001 by Jirka
- Fixed: getin in MP - two players gets in concurrently
- camera has switched to vehicle on both players (but only one gets in)
*/

bool AIBrain::ProcessGetIn2(Transport *veh, UIActionType pos, Turret *turret, int cargoIndex)
{
  //LogF("AIUnit::ProcessGetIn2 %s to %s", (const char *)GetDebugName(), (const char *)veh->GetDebugName());
  // Transport *veh = VehicleAssigned();
  if (!veh) return false;

  // fixed - do not continue with get in if unit is dead
  if (!LSCanGetInGetOut()) return false;

  /*
  Person *person = GetPerson();
  RptF
  (
  "Processing get in: %s (%s, id %d:%d) into %s (%s, id %d:%d)",
  (const char *)person->GetDebugName(), person->IsLocal() ? "LOCAL" : "REMOTE",
  person->GetNetworkId().creator.CreatorInt(), person->GetNetworkId().id,
  (const char *)veh->GetDebugName(), veh->IsLocal() ? "LOCAL" : "REMOTE",
  veh->GetNetworkId().creator.CreatorInt(), veh->GetNetworkId().id
  );
  */

  switch (pos)
  {
  case ATGetInDriver:
  case ATGetInPilot:
    {
      if (veh->QCanIGetIn(GetPerson()))
      {
        veh->GetInDriver(GetPerson());
        if (!veh->CanCancelStop())
        {
          Verify(SetState(Stopped));
        }

        // fixed - remote getin can fail - do not switch camera yet (will be switched later)
        if (GLOB_WORLD->FocusOn() == this && veh->IsLocal())
        {
          GLOB_WORLD->SwitchCameraTo(veh, ValidateCamera(GLOB_WORLD->GetCameraType()), false);
          veh->StartFadeIn(this);
        }
        return true;
      }
      else
      {
        // cannot get in as driver
        return false;
      }
    }
  case ATGetInTurret:
    {
      if (veh->QCanIGetInTurret(turret, GetPerson()))
      {
//        if (veh->IsLocal())
        veh->GetInTurret(turret, GetPerson());
        if (!veh->CanCancelStop())
        {
          Verify(SetState(Stopped));
        }

        // fixed - remote getin can fail - do not switch camera yet (will be switched later)
        if (GLOB_WORLD->FocusOn() == this && (veh->IsLocal() || turret->IsLocal()))
        {
          GLOB_WORLD->SwitchCameraTo(veh, ValidateCamera(GLOB_WORLD->GetCameraType()), false);
          veh->StartFadeIn(this);
        }
        return true;
      }
      else
      {
        // cannot get into turret
        return false;
      }
    }
  default:
    Assert(pos == ATGetInCargo);
    {
      cargoIndex = veh->QCanIGetInCargo(GetPerson(), cargoIndex);
      if (cargoIndex >= 0)
      {
        veh->GetInCargo(GetPerson(), true, cargoIndex);
        //LogF(" - get in cargo processed");
        SetState(InCargo);

        // fixed - remote getin can fail - do not switch camera yet (will be switched later)
        if (GLOB_WORLD->FocusOn() == this && veh->IsLocal())
        {
          GLOB_WORLD->SwitchCameraTo(veh, ValidateCamera(GLOB_WORLD->GetCameraType()), false);
          veh->StartFadeIn(this);
        }
        return true;
      }
      else
      {
        // cannot get in as cargo
        return false;
      }
    }
  }
}

bool AIBrain::HasAI() const
{
  if (_inVehicle)
  {
    if (_inVehicle->QIsManual(this)) return false;
  }
  else
    if (_person->QIsManual()) return false;

  return !_person->IsRemotePlayer();
}

bool AIBrain::IsUnit() const
{
  if (!_inVehicle) return true;
  return _inVehicle->CommanderUnit() == this;
}

bool AIBrain::IsCommander() const
{
  Transport *vehIn = GetVehicleIn();
  return vehIn && vehIn->ObserverBrain() == this;
}

bool AIBrain::IsDriver() const
{
  Transport *vehIn = GetVehicleIn();
  return vehIn && vehIn->DriverBrain() == this;
}

bool AIBrain::IsGunner() const
{
  Transport *vehIn = GetVehicleIn();
  return vehIn && vehIn->GunnerBrain() == this;
}

void ReportUnit(const AIBrain *unit)
{
  RptF
  (
    "Unit %s (0x%x) - network ID %d:%d",
    (const char *)unit->GetDebugName(),
    unit,
    unit->GetNetworkId().creator.CreatorInt(), unit->GetNetworkId().id
  );
}

bool AIBrain::AssertValid() const
{
  if (!GetPerson())
  {
    // all units must be represented by some soldier
    ReportUnit(this);
    RptF(" - no person");
    return false;
  }
  return true;
}

void AIBrain::SetLifeState(LifeState state)
{
  _lifeState = state;
}

/*!
\patch 1.33 Date 11/28/2001 by Jirka
- Fixed: ErrorMessage when MP server has VoiceRH addon installed and client hasn't
*/
void AIBrain::SetSpeaker(RString speaker, float pitch)
{
  Ref<BasicSpeaker> basic = new BasicSpeaker(speaker);
  _speaker = new Speaker(basic, pitch);
}

void AIBrain::SetSpeaker()
{
  Person *person = GetPerson();
  if (person)
  {
    int dpid = -1;
    if ( person->IsRemotePlayer() ) dpid = person->GetRemotePlayer();
    else if (person->IsLocal() && IsPlayer()) dpid = GetNetworkManager().GetPlayer();
    if (dpid!=-1)
    {
      const PlayerIdentity *identity = GetNetworkManager().FindIdentity(dpid);
      if (identity)
      {
        SetSpeaker(person->SelectSpeaker(identity->speakerType), identity->pitch);
        return;
      }
    }
    if (person->GetInfo()._pitch<0.001f)
    {
      ErrF("Setting invalid pitch %.4f for %s",person->GetInfo()._pitch,cc_cast(person->GetDebugName()));
    }
    SetSpeaker(person->GetInfo()._speaker, person->GetInfo()._pitch);
  }
}

float AIBrain::GetRawAbility() const
{
  // returns from 1 (able) to 0.2 (unable)
  if (IsAnyPlayer() || Glob.config.IsEnabled(DTUltraAI)) return 1; // always maximal
#if _ENABLE_IDENTITIES
  return _person->GetIdentity().GetRawAbility(AKGeneral);
#else
  return _ability;
#endif
}

void AIBrain::SetRawAbility(float ability)
{
#if _ENABLE_IDENTITIES
  _person->GetIdentity().SetAllAbilities(ability);
#else
  Assert(ability > 0);
  _ability = ability;
#endif
}

static inline float CombinedAbility(float ability, float abilityCoef)
{
  float lowerBound = floatMinMax(Lerp(-0.2f,0.6f,abilityCoef),0.01f,1.0f);
  float upperBound = floatMinMax(Lerp(0.8f,1.0f,abilityCoef),0.01f,1.0f);
  // ability below minEditorSkill means manipulation using setSkill - in such case let us respect the intent
  const float minEditorSkill = 0.2f;
  if (ability>=minEditorSkill)
    return InterpolativC(ability,minEditorSkill,1,lowerBound,upperBound);
  else
    return InterpolativC(ability,0,minEditorSkill,0,lowerBound);
}

#if 0 // _DEBUG
  static void PrintCombinedAbilityTable()
  {
    static const float coef[]={0.4,0.6,0.85,1};
    static const float abss[]={0.2,0.6,1.0};
    for (int a=0; a<lenof(abss); a++)
    for (int i=0; i<lenof(coef); i++)
    {
      float abilityCoef = coef[i];
      float ability = abss[a];
      float combinedAbility = CombinedAbility(ability,abilityCoef);
      LogF("Difficulty %.2f, skill %.2f -> %.2f",abilityCoef,ability,combinedAbility);
    }
  }
#endif


/*!
\patch 1.33 Date 12/03/2001 by Ondra
- New: Difficulty option "Super AI". When this option is enabled,
all AI units have maximal skill.
*/
float AIBrain::GetAbility(AbilityKind kind) const
{
  if( IsAnyPlayer())
  {
    // player skill maximum, but still subject to ability curves
    return Glob.config.GetAbility(kind, 1);
  }
  // returns from 1 (able) to 0.2 (unable)
  float abilityCoef = 1;
#if _ENABLE_IDENTITIES
  float ability = _person->GetIdentity().GetRawAbility(kind);
#else
  float ability = _ability;
#endif
  //float modifiedAbility = ability * abilityCoef;

  // multiplication not good - GetAbility will saturate in range 0..1 anyway
  float combinedAbility = CombinedAbility(ability,abilityCoef);

  return Glob.config.GetAbility(kind, combinedAbility);
}

float AIBrain::GetInvAbility(AbilityKind kind) const
{ // returns from 1 (able) to 5 (unable)
  return 1.0f/GetAbility(kind);
}

bool AIBrain::IsPlayer() const
{
  return _person == GWorld->PlayerOn();
}

bool AIBrain::IsAnyPlayer() const
{
  if (IsPlayer()) return true;
  Person *person = GetPerson();
  return person && person->IsNetworkPlayer();
}

bool AIBrain::IsRemotePlayer() const
{
  Person *person = GetPerson();
  return person && person->IsRemotePlayer();
}

void AIBrain::SetUnitPositionCommanded(UnitPosition status)
{
  Person *person = GetPerson();
  if (person) person->SetUnitPositionCommanded(status);
}

void AIBrain::SetUnitPositionScripted(UnitPosition status)
{
  Person *person = GetPerson();
  if (person) person->SetUnitPositionScripted(status);
}

void AIBrain::SetUnitPositionFSM(UnitPosition status)
{
  Person *person = GetPerson();
  if (person) person->SetUnitPositionFSM(status);
}

UnitPosition AIBrain::GetUnitPosition() const
{
  Person *person = GetPerson();
  if (person) return person->GetUnitPosition();
  return UPAuto;
}

bool AIBrain::IsFiringWanted() const
{
  EntityAIFull *veh = GetVehicle();
  // TODO: which turret?
  TurretContext context;
  if (veh && veh->GetPrimaryGunnerTurret(context))
  {
    /* not applicable for DayZ
    if (false)
    {
      // if the current weapon is a binocular style, there is no need to force combat
      // binocular animations work in Safe/Aware as well
      bool combatNeeded = true;
      WeaponModePtr ptr = context._weapons->ValidateWeapon(context._weapons->_fire._weapon);
      const MuzzleType *muzzle = ptr.GetMuzzleType();
      if (muzzle && muzzle->_useAsBinocular) combatNeeded = false;

      if (combatNeeded) return true;
    }
    */
  }
  return false;  
}

CombatMode AIBrain::GetCombatModeLowLevel() const
{
  CombatMode mode = GetCombatMode();
  if (mode!=CMCareless)
  {
    if (mode<CMCombat && IsFiringWanted())
    {
      return CMCombat;
    }
  }
  return mode;
}

CombatMode AIBrain::GetCombatMode() const
{
  CombatMode mode = _combatModeMajor;
  if (mode!=CMCareless)
  {
    CombatModeRange cMode = GetCombatModeBasedOnCommand();
    if (mode<cMode.beg) mode = cMode.beg;
    if (mode>cMode.end) mode = cMode.end;
  }
  return mode;
}

void AIBrain::SetCombatModeMajorReactToChange(CombatMode mode)
{
  if (_combatModeMajor!=mode)
  {
    CombatMode oldCM = GetCombatMode();
    SetCombatModeMajor(mode);
    CombatMode newCM = GetCombatMode();
    if (oldCM!=newCM) OnCombatModeChanged(oldCM,newCM);
  }
}


CombatMode AIBrain::GetCombatModeAutoDetected() const
{
  EntityAIFull *vehicle = GetVehicle();
  if (!vehicle) return CMSafe;
  return vehicle->GetCombatModeAutoDetected(this);
}

void AIBrain::SetCombatModeMajor(CombatMode mode)
{
  _combatModeMajor = mode;
  #if 0 //_ENABLE_CHEATS
  if (GetUnit() && GetUnit()->IsSubgroupLeader())
  {
    LogF("%s: SetCombatModeMajor %s", cc_cast(GetDebugName()), cc_cast(FindEnumName(mode)));
}
  #endif
}

void AIBrain::OnCombatModeChanged(CombatMode oldMode, CombatMode newMode)
{
  // if we are executing some plan, we need to replan it
  
  AIBrain *updatePlan = this;
  if (updatePlan->_state!=Stopped && updatePlan->_state!=Stopping && updatePlan->_state!=InCargo)
  {
    updatePlan->UpdateOperativePlan();
    // disable planning until search parameters are updated
    updatePlan->_planParametersSet = false;
  }
  #if 0 //_ENABLE_CHEATS
  if (GetUnit() && GetUnit()->IsSubgroupLeader())
  {
    LogF("%s: Changed to %s", cc_cast(GetDebugName()), cc_cast(FindEnumName(newMode)));
  }
  #endif
}

bool AIBrain::IsDanger() const
{
  return Glob.time <= _dangerUntil;
}

void AIBrain::OnNewTarget(TargetNormal *target, Time knownSince)
{
  EntityAIFull * veh = GetVehicle();
  if (veh) veh->OnNewTarget(target,knownSince);
}

void AIBrain::SetDanger(DangerCause cause, Vector3Par position, const Target *causedBy, float until)
{
  PROFILE_SCOPE_EX(aiDng,ai);
  if (IsPlayer()) return;

  if (until < 0) until = GRandGen.PlusMinus(5.0f, 1.0f);

  EntityAI *causedByAI = causedBy ? causedBy->idExact : NULL;
  if (_reactionFSM && !IsAIDisabled(DAFSM))
  {
    // let the FSM know a new danger cause exists
    GameVarSpace *vars = _reactionFSM->GetVars();
    if (vars)
    {
      GameValue value = GGameState.CreateGameValue(GameArray);
      GameArrayType &array = value;
      array.Realloc(3);
      array.Resize(3);
      array[0] = position.X();
      array[1] = position.Z();
      array[2] = position.Y() - GLandscape->SurfaceYAboveWater(position.X(), position.Z());

      // add the event to the queue
      GameValue queue;
      // variable names must be in lowercase
      if (vars->VarGet("_queue", queue) && queue.GetType() == GameArray)
      {
        GameValue item = GGameState.CreateGameValue(GameArray);
        GameArrayType &array = item;

        array.Realloc(4);
        array.Resize(4);
        array[0] = (float)cause; // cause
        array[1] = value; // position
        array[2] = Glob.time.toFloat() + until; // until
        array[3] = GameValueExt(causedByAI);

        ((GameArrayType &)queue).Add(item);
      }
    }
  }
  else
  {
    const EntityType *type = _person ? _person->GetType() : NULL;
    if (type)
    {
      const ParamClass *cls = type->GetParamEntry();
      if (cls)
      {
        RString name = RString("fsm") + FindEnumName((ReactionFSMRole)RFSMDanger);
        _reactionFSM = CreateFSM(type, (*cls) >> name);
        if (_reactionFSM)
        {
          GameVarSpace *vars = _reactionFSM->GetVars();
          if (vars)
          {
            // variable names must be in lowercase
            vars->VarLocal("_dangercause");
            vars->VarLocal("_dangerpos");
            vars->VarLocal("_dangeruntil");
            vars->VarLocal("_dangercausedby");

            GameValue value = GGameState.CreateGameValue(GameArray);
            GameArrayType &array = value;
            array.Resize(3);
            array[0] = position.X();
            array[1] = position.Z();
            array[2] = position.Y() - GLandscape->SurfaceYAboveWater(position.X(), position.Z());

            vars->VarSet("_dangercause", (float)cause);
            vars->VarSet("_dangerpos", value);
            vars->VarSet("_dangeruntil", Glob.time.toFloat() + until);
            vars->VarSet("_dangercausedby", GameValueExt(causedByAI));

            GameValue queue = GGameState.CreateGameValue(GameArray);
            vars->VarLocal("_queue");
            vars->VarSet("_queue", queue);
          }
        }
      }
    }

    if (!(_reactionFSM || IsAIDisabled(DAFSM)) && cause != DCDeadBodyGroup && cause != DCDeadBody)
    {
      // no FSM created, force the default reaction
      switch (GetCombatMode())
      {
      case CMSafe:
      case CMAware:
        {
          bool wasDanger = IsDanger();
          _dangerUntil = Glob.time + until;
          if (!wasDanger)
          {
            EntityAIFull *veh = GetVehicle();
            if (veh) veh->OnDanger();
          }
        }
        break;
      // don't react in Combat mode
      }
    }
  }
}

bool AIBrain::IsHoldingFire() const
{
  return _semaphore == AI::SemaphoreBlue ||
    _semaphore == AI::SemaphoreGreen ||
    _semaphore == AI::SemaphoreWhite;
}

bool AIBrain::IsKeepingFormation() const
{
  return _semaphore == AI::SemaphoreBlue ||
    _semaphore == AI::SemaphoreGreen ||
    _semaphore == AI::SemaphoreYellow;
}

void AIBrain::SetSemaphore(Semaphore status)
{
  _semaphore = status;
}

/*!
\patch 5132 Date 2/23/2007 by Jirka
- Fixed: AI did now follow player over the bridge

@return 1 on road, 2 on bridge
*/
int AIBrain::IsOnRoad() const
{
  if (IsAnyPlayer())
  {
    float size = 0;
    EntityAI *vehicle = GetVehicle();
    if (vehicle && vehicle->GetShape()) size = vehicle->GetShape()->GeometrySphere();
    RoadLink *road = unconst_cast(GLandscape->GetRoadNet()->IsOnRoad(Position(GetFutureVisualState()), size, size));
    if (!road) return 0;
    return road->IsBridge() ? 2 : 1;
  }

  int index = _path.FindNext(Position(GetFutureVisualState()));
  int onRoad = 0;
  if (index >=0 && index < _path.Size())
  {
    // moving to / on the road
    if (_path[index]._type == NTBridge) return 2;
    if (_path[index]._type == NTRoad) onRoad = 1;
  }
  index--;
  if (index >=0 && index < _path.Size())
  {
    // leaving the road
    if (_path[index]._type == NTBridge) return 2;
    if (_path[index]._type == NTRoad) onRoad = 1;
  }
  return onRoad;
}

Target *AIBrain::FindTargetAll(TargetType * id, bool includeBuildings) const
{
  if (!id) return NULL;

  const TargetList *targetList = AccessTargetList();
  if (!targetList) return NULL;

  const TargetNormal *t = targetList->FindTarget(id);
  if (t)
  {
    return unconst_cast(t);
  }

  if (includeBuildings && id->GetLinkId().IsObject())
  {
    // it is very likely it is a building - try to confirm it
    const StaticEntityLink *info = GLandscape->FindBuildingInfo(id->GetLinkId());
    if (!info) return NULL;
    return info->GetVirtualTarget();
  }
  return NULL;
}

Target *AIBrain::FindTarget(TargetType *id, bool includeBuildings) const
{
  Target *target = FindTargetAll(id, includeBuildings);
  if (target && !target->IsKnown()) return NULL;
  return target;
}

/*!
\patch 1.45 Date 2/18/2002 by Ondra
- Fixed: When enemy was killed, killer was always detected,
even when there was no one able to detect him.
(He was detected by the dead unit).
*/

Target *AIBrain::AddTarget(
  EntityAI *object, float accuracy, float sideAccuracy, float delay,
  const Vector3 *pos, AIBrain *sensor, float sensorDelay
  )
{
  TargetList *targetList = AccessTargetList();
  if (!targetList) return NULL;

  // check if target is already in list
  TargetNormal *target=NULL;
  const TargetNormal *t = targetList->FindTarget(object);
  bool isKnown = false;

  if( !t )
  {
    // is it normal, or minimal?
    // check if object is a building
    if (object->GetObjectId().IsObject())
    {
      // if it is from landscape, it might be a building
      // building may be used as a command target
      const StaticEntityLink *info = GLandscape->FindBuildingInfo(object->GetObjectId());
      if (info)
      {
        return info->GetVirtualTarget();
      }
    }
    const SideInfo *side = targetList->GetSideInfo(this);
    if (!side)
    {
      Fail("No side - cannot Add target");
      return NULL;
    }
    target = targetList->CreateTarget(this);
    target->idExact=object;
    //target->dammage=0;
    target->side=TSideUnknown;
    target->sideChecked = false;
    target->type=GWorld->Preloaded(VTypeAllVehicles);
    target->timeReported = TIME_MIN;

    targetList->Add(*side,target);
  }
  else
  {
    // we will set a new accuracy
    target=unconst_cast(t);
    isKnown = target->IsKnownNoDelay();
  }
  // find first type description so that accuracy matches
  if (!isKnown)
  {
    const SideInfo *side = targetList->GetSideInfo(this);
    if (side)
    {
      target->Reveal(*side,targetList,sideAccuracy);
    }
    if( !pos )
    {
      Vector3 apos=object->AimingPosition(object->FutureVisualState());
      // introduce some random error
      target->position=apos;
      target->RandomPosError();
      if (accuracy<1)
      {
        // the one who caused the explosion is expected to be not very far
        target->LimitError(100,100);
      }
      else if (accuracy<3.5f)
      {
        // as the accuracy is quite high, we know the approximate position
        target->LimitError(5,5);
      }
      else
      {
        // we know the almost exact position
        target->LimitError(0.05f,0.05f);
      }
    }
    else
    {
      target->position=*pos;
      // we will expect the target is most likely someplace near the explosion it has caused
      // we may be wrong, but we need somewhere to start from
      target->LimitError(200,200);
    }
    target->speed=VZero; // speed not known
    if (sensor)
    {
      if (sensorDelay>delay) sensorDelay = delay;
      target->lastSeen=Glob.time-10+sensorDelay;
      target->delaySensor = Glob.time+sensorDelay;
      target->delay = Glob.time+delay;
      target->idSensor = sensor;
    }
    else
    {
      target->lastSeen=Glob.time-10+delay;
      target->delay=Glob.time+delay;
    }
    // we assume we have seen it, but not very recently
    // we set lastSeen so that target is known only for 5 sec
    // that should be enough to activate defensive reaction
    // usually some Attack is issued
  }
  else
  {
    Time delayUntil=Glob.time+delay;
    if( target->delay>delayUntil ) target->delay=delayUntil;

    if (!target->idSensor && sensor)
    {
      target->idSensor = sensor;
    }

    if (!pos && accuracy>=3.5f)
    {
      target->LimitError(0.1f,0.1f);
    }
  }
  const VehicleType *type = object->GetTypeAtLeast(accuracy);
  accuracy = type->GetAccuracy();
  if( target->FadingAccuracy()<accuracy )
  {
    target->accuracy=accuracy;
    target->type=type;
  }
  if( target->FadingSideAccuracy()<sideAccuracy )
  {
    target->sideAccuracy=sideAccuracy;
    target->side=object->GetTargetSide(sideAccuracy);
    Assert( target->sideAccuracy<3 || target->side!=TSideUnknown );
    target->sideChecked = sideAccuracy>=1.5f;
  }
  float spotability=floatMin(accuracy*2.5,1);
  if( target->FadingSpotability()<spotability )
  {
    target->spotability=spotability;
    target->spotabilityTime=Glob.time;
  }
  return target;
}

/// ghost target is used to control unit fire sectors or suppressive fire
/**
Ghost target has very little properties - type and position is all we need.

Ghost target is always known only by one unit.
*/

class TargetGhost: public Target
{
  typedef Target base;

protected:
  /// owner of this target
  OLinkPermNO(AIBrain) _owner;

  Vector3 _position;

  Ref<const EntityAIType> _type;

  TargetSide _side;
  //@{ no accuracy fading for ghosts
  float _posAccuracy;
  float _sideAccuracy;
  //@}

  public:

  /// empty constructor used for serialization
  TargetGhost();
  TargetGhost(
    AIBrain *owner, const EntityAIType *type, Vector3Par position,
    TargetSide side, float posAccuracy, float sideAccuracy
  );

  ~TargetGhost();

  virtual const EntityAIType *GetType() const {return _type;}
  virtual Vector3 GetSpeed(const AIBrain *who) const {return VZero;}

  virtual bool IsVanishedOrDestroyed() const {return false;}
  virtual bool IsDestroyed() const {return false;}
  virtual bool IsVanished() const {return false;}
  virtual bool HasDiclosedUs() const {return false;}
  virtual TargetSide GetSide() const {return _side;}
  virtual bool GetSideChecked() const {return true;}
  virtual Time GetLastSeen() const;
  virtual float Distance(Vector3Par pos) const {return _position.Distance(pos);}
  virtual Vector3 GetPosition() const {return _position;}
  virtual Vector3 GetDirection() const;
  virtual float GetSubjectiveCost() const {return 0;}
  virtual bool IsKindOf(const EntityType *supertype) const {return _type->IsKindOf(supertype);}
  virtual void UpdateRemotePosition(EntityAI *ent) {}

  virtual AIBrain *SeenBy() const;
  virtual void MarkKiller(EntityAI *by) {}

  virtual float FadingSideAccuracy() const {return _sideAccuracy;}
  virtual float FadingAccuracy() const {return _sideAccuracy;}
  virtual float FadingSpotability() const {return 4;}
  virtual float FadingPosAccuracy() const {return _posAccuracy;}

  virtual void UpdatePosition(Vector3Par pos, float typeAccuracy, float posAccuracy)
  {
    _position = pos; _sideAccuracy = typeAccuracy; _posAccuracy = posAccuracy;
  }

  virtual bool IsKnownBy( const AIBrain *unit ) const {return unit==_owner;}
  virtual bool IsKnownByAll() const {return false;}
  virtual bool IsKnownBySome() const {return true;}
  virtual void Reveal(const SideInfo &side, TargetList *list, float accuracy) {}
  virtual void RevealInVehicle(Transport *veh) {}
  virtual void DeleteFromOwner();
  virtual bool AccurateEnoughForHit(const AmmoType *ammo, AIBrain *brain) const;
  virtual float PositionAccuracy(const AIBrain *brain) const;
  virtual bool SeenRecently(AIBrain *brain, float maxAge=10.0f) const;

  virtual bool IsKnown() const {return true;}

  virtual float VisibleSize() const;
  virtual Vector3 AimingPosition() const {return _position;}
  virtual Vector3 LandAimingPosition() const {return _position;}
  virtual Vector3 LandAimingPosition(const AIBrain *who, const AmmoType *ammo=NULL, bool head=false, bool *isExact=NULL) const
  {
    return _position;
  }

  virtual TargetState State(AIBrain *sensor) const
  {
    // TODO: based on side we may select a different TargetState
    return TargetEnemyEmpty;
  }

  /// during serialization we need to know if the target is minimal or not
  virtual bool IsMinimal() const;

  static TargetGhost *CreateObject(ParamArchive &ar);
  virtual LSError Serialize(ParamArchive &ar);
  virtual LSError SaveRef(ParamArchive &ar);
};

TargetGhost::TargetGhost()
{
}

TargetGhost::~TargetGhost()
{
  if (_type)
  {
    _type->VehicleRelease();
  }
}

TargetGhost::TargetGhost(
  AIBrain *owner, const EntityAIType *type, Vector3Par position,
  TargetSide side, float posAccuracy, float sideAccuracy
):_owner(owner),_type(type),
_position(position),_side(side),
_posAccuracy(posAccuracy),_sideAccuracy(sideAccuracy)
{
  type->VehicleAddRef();
}

Vector3 TargetGhost::GetDirection() const
{
  Fail("Ghost targets cannot be queried for orientation");
  return VForward;
}

Time TargetGhost::GetLastSeen() const
{
  return Glob.time;
}

bool TargetGhost::IsMinimal() const
{
  return false;
}

float TargetGhost::PositionAccuracy(const AIBrain *brain) const
{
  return _posAccuracy;
}

bool TargetGhost::AccurateEnoughForHit(const AmmoType *ammo, AIBrain *brain) const
{
  // ghost targets have no memory error - ignore brain
  return _posAccuracy < ammo->indirectHitRange*2+0.5f;
}

bool TargetGhost::SeenRecently(AIBrain *brain, float maxAge) const
{
  return true;
}

float TargetGhost::VisibleSize() const
{
  LODShape *shape = _type->GetShape();
  if (shape)
  {
    return shape->GeometrySphere();
  }
  return 1.0f;
}

void TargetGhost::DeleteFromOwner()
{
  Assert(_owner->GetGhostTarget()==this);
  _owner->SetGhostTarget(NULL);
}

AIBrain *TargetGhost::SeenBy() const
{
  return _owner;
}

LSError TargetGhost::SaveRef(ParamArchive &ar)
{
  // there is only one ghost target in any unit - one reference is enough
  RString type("ghost");
  CHECK(ar.SerializeRef("owner",_owner,1))
  return LSOK;
}

TargetGhost *TargetGhost::CreateObject(ParamArchive &ar)
{
  return new TargetGhost();
}

LSError TargetGhost::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeRef("owner", _owner, 1));

  CHECK(::Serialize(ar, "type", _type, 1))

  if (ar.IsLoading() && ar.GetPass()==ParamArchive::PassFirst)
  {
    // we did not perform VehicleAddRef in constructor - we need to do it now
    _type->VehicleAddRef();
  }

  CHECK(::Serialize(ar,"position", _position, 1, VZero));

  CHECK(ar.SerializeEnum("side", _side, 1, TCivilian));

  CHECK(ar.Serialize("posAccuracy", _posAccuracy, 1, 0));
  CHECK(ar.Serialize("sideAccuracy", _sideAccuracy, 1, 4));

  // idExact is always NULL and base::Serialize will do nothing in current version
  return base::Serialize(ar);
}

Target *LoadTargetGhostRef(ParamArchive &ar)
{
  OLinkPermNO(AIBrain) owner;
  if(ar.SerializeRef("owner",owner,1)==LSOK)
  {
    Assert(owner->GetGhostTarget());
    return owner->GetGhostTarget();
  }
  Fail("Bad ghost target owner");
  return NULL;
}


void AIBrain::SetGhostTarget(TargetGhost *tgt){_ghostTarget=tgt;}
TargetGhost *AIBrain::GetGhostTarget() const {return _ghostTarget;}

Target *AIBrain::CreateGhostTarget(
	const EntityAIType *type, Vector3Par pos,
	float typeAccuracy, float posAccuracy, float sensorDelay
)
{
  // create a dummy object in the landscape
  // add the object into the list
  // note: TargetGhost increases vehicle ref. count of type
  _ghostTarget = new TargetGhost(
    this,type,pos,type->GetTypicalSide(),posAccuracy,typeAccuracy
  );
  /// owner of this target
  return _ghostTarget;
}

void AIBrain::MoveTarget(Target *tgt, Vector3Par pos, float typeAccuracy, float posAccuracy)
{
  tgt->UpdatePosition(pos,typeAccuracy,posAccuracy);
}

void AIBrain::DeleteGhostTarget(Target *tgt)
{
  Assert(tgt==_ghostTarget)
  _ghostTarget = NULL;
}

void AIBrain::SetWatchDirection( Vector3Val dir )
{
  _watchDirHead = _watchDir = dir;
  _watchMode = WMDir;
  _watchDirSet = Glob.time;
  _targetAssigned = TargetToFire();
}

void AIBrain::SetWatchAround()
{
  _watchDirHead = _watchDir = GetVehicle()->FutureVisualState().Direction();
  _watchMode = WMAround;
  _watchDirSet = Glob.time;
  _targetAssigned = TargetToFire();
}

void AIBrain::SetNoWatch()
{
  _watchTgt = NULL, _watchMode = WMNo, _glanceTgt = NULL;
  _targetAssigned = TargetToFire();
}

void AIBrain::SetWatchPosition( Vector3Val pos, bool glanceOnly)
{
  if (glanceOnly)
  {
    _watchTgt = NULL;
    _watchMode = WMPosGlance;
    _glancePos = pos;
    Assert(_glancePos.IsFinite());
  }
  else
  {
    _watchPos = pos;
    _glancePos = VZero;
    _watchMode = WMPos;
  }
  _targetAssigned = TargetToFire();
}

void AIBrain::SetWatchTarget(Target *tgt, bool glanceOnly)
{
  if (!tgt)
  {
    if (glanceOnly)
    {
      // cancel glance only, leave the other one untouched
      _glanceTgt = NULL;
    }
    else
    {
      _glanceTgt = NULL;
      _watchTgt = NULL;
    }
    // WMTgt falls through to WMNo when there is no target
    _watchMode = WMTgt;
  }
  else if (glanceOnly)
  {
    _glanceTgt = tgt;
    _watchMode = WMTgtGlance;
  }
  else
  {
    _glanceTgt = NULL;
    _watchTgt = tgt;
    _watchMode= WMTgt;
  }
}

/*!
\patch 1.24 Date 9/20/2001 by Ondra
- New: Ejection from plane is real ejection, not only jumping out.
\patch 1.27 Date 10/09/2001 by Ondra
- Fixed: MP: Ejection on client problems fixed. This could also cause player duplication.
*/

void AIBrain::DoGetOut(Transport *veh, bool eject, bool parachute, Vector3Val diff)
{
  bool isFocused = (GLOB_WORLD->FocusOn() == this);

  Person *person = GetPerson();
  Vector3 ejectSpeed = person->FutureVisualState().Speed();
#define GET_OUT_TIME 5.0f
  person->DisableDamageFromObj(GET_OUT_TIME);
  /*
  RptF
  (
  "Processing get out: %s (%s, id %d:%d) from %s (%s, id %d:%d)",
  (const char *)person->GetDebugName(), person->IsLocal() ? "LOCAL" : "REMOTE",
  person->GetNetworkId().creator.CreatorInt(), person->GetNetworkId().id,
  (const char *)veh->GetDebugName(), veh->IsLocal() ? "LOCAL" : "REMOTE",
  veh->GetNetworkId().creator.CreatorInt(), veh->GetNetworkId().id
  );
  */

  // calculate get out position
  GetInPoint pt = veh->GetUnitGetOutPos(this);
  Matrix4 transform;
  transform.SetPosition(pt._pos);
  transform.SetUpAndDirection(VUp, -pt._dir);

  if (diff != VZero)
  {
    Vector3 diffR = transform.Rotate(diff);
    transform.SetPosition(pt._pos + diffR);
  }

  if (veh->DriverBrain() == this) // driver
  {
    veh->GetOutDriver(transform,eject);
  }
  else
  {
    TurretContext context;
    if (veh->FindTurret(person, context))
    {
      // turrets
      veh->GetOutTurret(context, transform);
    }
    else
    {
      // cargo
      veh->GetOutCargo(person, transform);

      // the only place where reset InCargo flag
      if (IsLocal())
      {
        Assert(IsInCargo());
        SetInCargo(false);
      }
    }
  }

  // FIX: check the state
  if (_state == Stopping || _state == Stopped)
  {
    Verify(SetState(Wait));
  }

  if (parachute)
  {
    const char *paraName = "ParachuteWest";
    TargetSide side = GetSide();
    if (side==TEast) paraName = "ParachuteEast";
    else if (side==TGuerrila) paraName = "ParachuteG";
    else if (side==TCivilian) paraName = "ParachuteC";

    Ref<EntityAI> ent = GWorld->NewVehicleWithID(paraName,"");
    Ref<Transport> getOutTo = dyn_cast<Transport,EntityAI>(ent);
    if (getOutTo)
    {
      getOutTo->SetAllowDammage(person->GetAllowDammage());
      if (getOutTo->Driver())
      {
        ErrF("%s already has a driver",(const char *)getOutTo->GetDebugName());
      }
      else
      {
        //Log("Getting out to %s",(const char *)getOutTo->GetDebugName());
        Vector3 aimCenter = getOutTo->GetShape()->AimingCenter();
        // aimCenter should be on pos position

        //getOutTo->AimingPosition();

        Matrix4 trans = person->FutureVisualState().Transform();
        trans.SetPosition(trans.Position()+getOutTo->FutureVisualState().Speed()*0.3f-aimCenter);

        getOutTo->SetSpeed(veh->FutureVisualState().Speed()+ejectSpeed);
        getOutTo->SetTransform(trans);

        GWorld->AddVehicle(getOutTo);
        GetNetworkManager().CreateVehicle(getOutTo, VLTVehicle, "", -1);

        DoAssert(getOutTo->IsLocal());

        getOutTo->GetInDriver(person,false);
      }
    }

  }
  else
  {
    // set him into freefall mode.
    person->FreeFall(1.0f);
  }

  if (isFocused)
  {
    GLOB_WORLD->SwitchCameraTo(GetVehicle(), ValidateCamera(GLOB_WORLD->GetCameraType()), false);
    person->StartFadeIn(this);
  }
}

bool AIBrain::ProcessGetOut(bool eject, bool parachute, Vector3Val diff)
{
  // if (!IsLocal()) return false;
  if (IsFreeSoldier()) return false;

  Transport *veh = GetVehicleIn();
  if (!veh) return false;

  if (!LSCanGetInGetOut())
  {
    // when get out is impossible, do not longer wait and fail the command
    return false;
  }

  Turret *turret = NULL;
  if (GetPerson() != veh->Driver())
  {
    TurretContext context;
    if (veh->FindTurret(GetPerson(), context)) turret = context._turret;
  }

  DoGetOut(veh, eject, parachute);

  return true;
}

void AIBrain::Load(const IdentityInfo &info)
{
  Person *soldier = GetPerson();
  if (soldier)
  {
    // avoid identity overwrite when delay EH processed
    LoadIdentityDelayed();

    AIUnitInfo &unitInfo = soldier->GetInfo();
    unitInfo._fullName = info._name;
    unitInfo._face = info._face;
    unitInfo._glasses = info._glasses;
    soldier->SetFace(unitInfo._face);
    soldier->SetGlasses(unitInfo._glasses);
    unitInfo._speaker = info._speaker;
    unitInfo._pitch = info._pitch;
    
    if (info._speaker.GetLength() <= 0)
    {
      RptF("No speaker given for %s",cc_cast(info._name));
    }

    SetSpeaker(unitInfo._speaker, unitInfo._pitch);
  }
}

bool AIBrain::CanHear(ChatChannel channel, AIBrain *sender) const
{
#if _ENABLE_DIRECT_MESSAGES
  if (channel == CCDirectSpeaking)
  {
    const float limit2 = Square(50.0f);
    float dist2 = Position().Distance2(sender->Position());
    return dist2 <= limit2;
  }
#endif
  return true;
}

float AIBrain::GetTimeToLive() const
{
  // assume we can live max. 24 hours
  float maxTimeToLive = 3600 * 24.0;
  // when in combat, assume we can live max. 1 hour
  if (GetCombatMode() >= CMCombat) maxTimeToLive = 3600;

  return maxTimeToLive;
}

bool AIBrain::CheckEmpty(Vector3Par pos )
{
  EntityAI *veh = GetVehicle();
  AILockerSavedState state;
  veh->PerformUnlock(&state);

  OperMapIndex xe(toIntFloor(pos.X() * InvOperItemGrid));
  OperMapIndex ze(toIntFloor(pos.Z() * InvOperItemGrid));

  LAND_INDICES(xe, ze, xei, zei);

  const int border = 0;

  OperMap map;
  map.CreateMap(veh, LandIndex(xei - border), LandIndex(zei - border), LandIndex(xei + border), LandIndex(zei + border));

  bool ret = map.GetFieldCost(xe, ze, true, GetVehicle()->GetType(), CMAware, IsSoldier(), false).cost < GET_UNACCESSIBLE;
  veh->PerformRelock(state);
  return ret;
}

bool AIBrain::NearestEmptyReady(Vector3Par pos) const
{
  LandIndex xe(toIntFloor(pos.X() * InvLandGrid));
  LandIndex ze(toIntFloor(pos.Z() * InvLandGrid));

  // FindNearestEmpty will search in +-1 range
  // CreateFiled may use 1 more field
  // RoadSurfaceY may use 1 more field - this is handled inside of OperFieldRangeReady
  LandIndex xMin(xe - 2), xMax(xe + 2);
  LandIndex zMin(ze - 2), zMax(ze + 2);
  return OperMap::OperFieldRangeReady(xMin, xMax, zMin, zMax);
}

bool AIBrain::FindNearestEmpty(Vector3 &pos, int mask)
{
  PROFILE_SCOPE_EX(aiFNE,aiPath);
  return FindNearestEmpty(pos, IsSoldier(), GetVehicle(), mask);
}

bool AIBrain::FindNearestEmpty(Vector3 &pos, bool soldier, EntityAI *veh, int mask)
{
  AILockerSavedState state;
  veh->PerformUnlock(&state);

  float xef = pos.X() * InvOperItemGrid;
  float zef = pos.Z() * InvOperItemGrid;
  OperMapIndex xe(toIntFloor(xef));
  OperMapIndex ze(toIntFloor(zef));

  LAND_INDICES(xe, ze, xei, zei);

  const int border = 1;

  const EntityAIType *vehType = veh->GetType();
  OperMap map;
  map.CreateMap(veh, LandIndex(xei - border), LandIndex(zei - border), LandIndex(xei + border), LandIndex(zei + border), mask);

  if (map.GetFieldCost(xe, ze, true, vehType, CMAware, soldier, false).cost >= GET_UNACCESSIBLE)
  {
    OperMapIndex oxe(xe), oze(ze);
    if (map.FindNearestEmpty(
      xe, ze, xef, zef,
      OperMapIndex((xei - border) * OperItemRange), OperMapIndex((zei - border) * OperItemRange),
      OperMapIndex((1 + 2 * border) * OperItemRange), OperMapIndex((1 + 2 * border) * OperItemRange),
      true, vehType, soldier, true, DefFindFreePositionCallback, NULL))
    {
      // xEnd, zEnd must be changed (is forced into last path point)
      if( xe!=oxe || ze!=oze )
      {
        pos[0]= xe * OperItemGrid + 0.5 * OperItemGrid;
        pos[2] = ze * OperItemGrid + 0.5 * OperItemGrid;
      }
    }
    else
    {
#if LOG_PROBL
      Log("Unit %s: End point not found", (const char *)veh->GetDebugName());
      map.LogMap(xei - border, xei + border, zei - border, zei + border, xe, xe, ze, ze, false);
#endif
      veh->PerformRelock(state);
      return false;
    }
  }
  veh->PerformRelock(state);
  return true;
}

bool AIBrain::FindEmptyArea(Vector3 &center, float radius, float maxDistance, const EntityAIType *vehType)
{
  OperMap map;
  return map.FindEmptyArea(center, radius, maxDistance, vehType);
}

bool AIBrain::FindEmptyInRect(
  Vector3 &emptyPos, const FrameBase &pos, const Vector3 *minmax, Vector3Par nearTo, int mask
)
{
  PROFILE_SCOPE_EX(aiFNE,aiPath);
  return FindEmptyInRect(emptyPos, IsSoldier(), GetVehicle(), pos, minmax, nearTo, mask);
}

bool AIBrain::FindEmptyInRect(
  Vector3 &emptyPos, bool soldier, EntityAI *veh,
  const FrameBase &pos, const Vector3 *minmax, Vector3Par nearTo, int mask
)
{
  AILockerSavedState state;
  veh->PerformUnlock(&state);

  float xef = pos.Position().X() * InvOperItemGrid;
  float zef = pos.Position().Z() * InvOperItemGrid;
  OperMapIndex xe(toIntFloor(xef));
  OperMapIndex ze(toIntFloor(zef));

  LAND_INDICES(xe, ze, xei, zei);

  // start with no border - will be extended as needed (on demand)
  const int border = 0;

  const EntityAIType *vehType = veh->GetType();
  OperMap map;
  map.CreateMap(veh, LandIndex(xei - border), LandIndex(zei - border), LandIndex(xei + border), LandIndex(zei + border), mask);

  // provide a default value in case search is not needed
  emptyPos = pos.Position();

  OperMapIndex oxe(xe), oze(ze);
  if(
    map.FindEmptyInRect(
      xe, ze,
      pos,minmax,nearTo, true, vehType, soldier, true,
      DefFindFreePositionCallback, NULL
    )
  )
  {
    // xEnd, zEnd must be changed (is forced into last path point)
    if( xe!=oxe || ze!=oze )
    {
      emptyPos[0]= xe * OperItemGrid + 0.5 * OperItemGrid;
      emptyPos[2] = ze * OperItemGrid + 0.5 * OperItemGrid;
    }
  }
  else
  {
    veh->PerformRelock(state);
    return false;
  }
  veh->PerformRelock(state);
  return true;
}


void AIBrain::ClearOperativePlan()
{
  //DoAssert(GetVehicle()->PlanChangePossible());
  _path.Clear();
  SetPathCosts(0, 0);
}

void AIBrain::ClearStrategicPlan()
{
  _noPath = true;
  _updatePath = false;
  _lastPlan = false;

#if _ENABLE_AI
  _attemptPlan = 0;
  _planner->Init();
  _waitWithPlan = Time(0);
  _completedTime = Glob.clock.GetTimeInYear();
#endif

  if
  (
    GetState() != InCargo &&
    GetState() != Stopping && GetState() != Stopped
  )
  Verify(SetState(Wait));
}

void AIBrain::RefreshStrategicPlan()
{
#if _ENABLE_AI
  if (_noPath)
  {
    ClearStrategicPlan();
  }
  else
  {
    Assert(_planner->GetPlanSize() > 0);
    _planner->StopSearching();
    _updatePath = true;
  }
#endif
}

void AIBrain::ForceReplan(bool clear)
{
  _plannedPosition = _wantedPosition;
  if (clear)
  {
    ClearStrategicPlan();
    ClearOperativePlan();
  }
  else
    RefreshStrategicPlan();
}

void AIBrain::GetExpectedDestination(PlanPosition &destination, float &precision, PlanningMode &planningMode, bool &forceReplan)
{
  destination = _wantedPositionWanted;
  precision = _wantedPrecision;
  planningMode = _planningModeWanted;
  forceReplan = _forceReplanWanted;

  // Fix: avoid re-planning multiple times
  _forceReplanWanted = false;
}

bool AIBrain::CheckReplanNeeded(float baseCoef, PlanningMode planningMode, bool forceReplan) const
{
  if (!forceReplan)
  {
    float error2 = _plannedPosition.Distance2(_wantedPosition);
    float limit2 = baseCoef * Position(GetFutureVisualState()).Distance2(_wantedPosition);
    forceReplan = error2 > limit2;
  }

  return forceReplan;
}

void AIBrain::SetWantedDestination(const PlanPosition &destination, PlanningMode planningMode, bool forceReplan)
{
  PlanningMode oldPlanningMode = _planningMode;
  _planningMode = planningMode;
  _wantedPosition = destination;
  _completedReceived = false;
  _planParametersSet = true;
  #if 0 //_ENABLE_CHEATS
  if (GetUnit() && GetUnit()->IsSubgroupLeader())
  {
    LogF("%s: SetWantedDestination, planning mode %s", cc_cast(GetDebugName()), cc_cast(FindEnumName(planningMode)));
  }
  #endif
  
  // LogF("Unit %s: SetWantedDestination [%.0f, %.0f]", (const char *)GetDebugName(), pos.X(), pos.Z());
#if DIAG_WANTED_POSITION
if (GWorld->FocusOn() && GWorld->FocusOn()->GetVehicle() == GetVehicle())
{
  LogF(
    "Unit %s: received wanted destination %s to %.0f, %.0f",
    cc_cast(GetDebugName()), cc_cast(FindEnumName(planningMode)),
    destination.X(), destination.Z());
}
#endif

  if (planningMode!=DoNotPlan && planningMode!=DoNotPlanFormation) GetVehicle()->GoToStrategic(destination);

  switch (planningMode)
  {
  case DoNotPlan: case  DoNotPlanFormation:
    // continue with plan
    if (forceReplan)
    {
      ClearStrategicPlan();
      ClearOperativePlan();
    }
    break;
  case LeaderPlanned:
  case VehiclePlanned:
    {
      // check if simple path is possible
      EntityAIFull *veh = GetVehicle();
      if (veh)
      {
        Vector3Val from = veh->FutureVisualState().Position();
        if (IsSimplePathReady(from, destination))
        {
          float simpleCost, simpleHeurCost;
          if (IsSimplePath(from, destination, simpleCost, simpleHeurCost))
          {
            // create the simple path directly
            _plannedPosition = destination;
            _expPosition = destination;
            _noPath = false;
            _updatePath = false;
            _lastPlan = true;

            _path.Resize(2);
            _path[0]._pos = from;
            _path[0]._cost = 0;
            _path[0]._type = NTGrid;
            _path[1]._pos = GLandscape->PointOnSurface(destination[0], veh->GetCombatHeight(), destination[2]);
            _path[1]._cost = simpleCost;
            _path[1]._type = NTGrid;
            SetPathCosts(simpleCost, simpleCost);
            _path.SetSearchTime(Glob.time);
            _path.SetCover(CoverInfo(),false);
            ++_pathId;

            Verify(SetState(Busy));
            break;
          }
        }
      }
    }

    //    if (_state == Completed) SetState(Wait);
#if 0
if (forceReplan)
{
  LogF("Force replan passed");
}
#endif
    forceReplan = CheckReplanNeeded(0.3f, planningMode, forceReplan);

    if (forceReplan)
    {
      _plannedPosition = _wantedPosition;
      _noPath = true;
      // when _noPath, RefreshStrategicPlan will call ClearStrategicPlan and ClearOperativePlan
      RefreshStrategicPlan();
    }
    // else replan only when _wantedPosition differs from _plannedPosition enough
    break;
  case LeaderDirect:
    // force replan
    if (forceReplan || planningMode!=oldPlanningMode || _plannedPosition.Distance2(destination)>Square(1.0f))
    {
      _plannedPosition = PlanPosition(destination,false);
      _expPosition = _plannedPosition;
      ClearStrategicPlan();
      ClearOperativePlan();
    }
    break;
  }
}

void AIBrain::SetPlanningMode(PlanningMode planningMode, bool forceReplan)
{
  _planningModeWanted = _planningMode = planningMode;
  if (planningMode != DoNotPlan && planningMode!=DoNotPlanFormation) GetVehicle()->GoToStrategic(_wantedPosition);

  switch (planningMode)
  {
  case DoNotPlan: case DoNotPlanFormation:
    // continue with plan
    if (forceReplan)
    {
      ClearStrategicPlan();
      ClearOperativePlan();
    }
    break;
  case LeaderPlanned:
  case VehiclePlanned:
    //    if (_state == Completed) SetState(Wait);
    forceReplan = CheckReplanNeeded(0.1f, planningMode, forceReplan);

    if (forceReplan)
    {
      _plannedPosition = _wantedPosition;
      _noPath = true;
      RefreshStrategicPlan();
    }
    // else replan only when _wantedPosition differs from _plannedPosition enough
    break;
  case LeaderDirect:
    // force replan
    _plannedPosition = _wantedPosition;
    _expPosition = _wantedPosition;
    ClearStrategicPlan();
    ClearOperativePlan();
    break;
  }
}

void AIBrain::SetWantedPosition(const PlanPosition &pos, float precision, PlanningMode mode, bool forceReplan)
{
#if DIAG_PLANNING
  LogF
    (
    "%.3f, %s: SetWantedPosition to %.0f, %.0f, mode %s %s",
    Glob.time.toFloat(), (const char *)GetDebugName(),
    pos.X(), pos.Z(), (const char *)FindEnumName(mode),
    forceReplan ? ", force replan" : ""
    );
#endif

#if DIAG_WANTED_POSITION
if (GWorld->FocusOn() && GWorld->FocusOn()->GetVehicle() == GetVehicle())
{
  LogF(
    "Unit %s: received destination %s to %.0f, %.0f",
    cc_cast(GetDebugName()), cc_cast(FindEnumName(mode)),
    pos.X(), pos.Z());
}
#endif

  _wantedPositionWanted = pos;
  _wantedPrecision = precision;
  _planningModeWanted = mode;
  _forceReplanWanted = forceReplan;

  // when changing planning mode to DoNotPlan, we need to be sure no more planning will be done
  // therefore we set not only wanted values, but also real ones
  bool setDirectly = !_behaviourFSMs[BFSMFormation] || mode==DoNotPlan || mode==DoNotPlanFormation || IsAIDisabled(DAFSM);
  if (!setDirectly)
  {
    EntityAIFull *vehicle = GetVehicle();
    if (!vehicle) setDirectly = true;
    else
    {
      if (vehicle->PilotUnit() != this)
      {
        // set directly for non-drivers
        setDirectly = true;
      }
      else
      {
        // set directly for the subgroup leader
        //AIBrain *commander = vehicle->CommanderUnit();
        //setDirectly = commander->GetUnit() && commander->GetUnit()->IsSubgroupLeader();
      }
    }
  }

  if (setDirectly)
  {
    // no interface FSM - set the destination immediately
    SetWantedDestination(pos, mode, forceReplan);
  }
}

/*!
\patch_internal 5095 Date 12/4/2006 by Jirka
- Fixed: Avoid using simple path when moving onto / out of bridge
*/

bool AIBrain::IsSimplePath(Vector3Val from, Vector3Val pos, float &simpleCost, float &simpleHeurCost) const
{
  // if FSM requested cover searching, we must always enforce it - simple test can never pass
  if (_maxCostToCover>0) return false;
  
  const float maxDist=MAX_OPER_PATH;
  float dist2=(from-pos).SquareSizeXZ();
  if( dist2>Square(maxDist) ) return false;

  EntityAIFull *vehicle = GetVehicle();

  // check if not starting / ending on the bridge
  if (!GLandscape->GetRoadNet()->IsOnRoadReady(from)) return false;
  if (!GLandscape->GetRoadNet()->IsOnRoadReady(pos)) return false;
  
  float size = vehicle->GetShape() ? vehicle->GetShape()->GeometrySphere() : 0;
  const RoadLink *road = unconst_cast(GLandscape->GetRoadNet()->IsOnRoad(from, size, size));
  if (road)
  {
    // check if the road is a bridge
    if (road->IsBridge()) return false;
  }
  road = GLandscape->GetRoadNet()->IsOnRoad(pos, size, size);
  if (road)
  {
    // check if the road is a bridge
    if (road->IsBridge()) return false;
  }

  AILockerSavedState state;
  vehicle->PerformUnlock(&state);

  OperMapIndex xs(toIntFloor(from.X() * InvOperItemGrid));
  OperMapIndex zs(toIntFloor(from.Z() * InvOperItemGrid));
  OperMapIndex xe(toIntFloor(pos.X() * InvOperItemGrid));
  OperMapIndex ze(toIntFloor(pos.Z() * InvOperItemGrid));

  LAND_INDICES(xs, zs, xMin, zMin);
  LAND_INDICES(xe, ze, xMax, zMax);
  if (xMax < xMin) Swap(xMax,xMin);
  if (zMax < zMin) Swap(zMax,zMin);

  OperMap map;
  if ((xMax-xMin)>50 || (zMax-zMin)>50)
  {
    LogF("xMax %d, xMin %d",xMax,xMin);
    LogF("zMax %d, zMin %d",xMax,xMin);
    LogF("pos %.2f,%.2f,%.2f",pos[0],pos[1],pos[2]);
    LogF("from %.2f,%.2f,%.2f",from[0],from[1],from[2]);
  }
  map.CreateMap(vehicle, xMin, zMin, xMax, zMax);
  bool retVal = map.IsSimplePath(this, xs, zs, xe, ze, simpleCost, simpleHeurCost);

  vehicle->PerformRelock(state);
  return retVal;
}

bool AIBrain::IsSimplePathReady(Vector3Val from, Vector3Val pos) const
{
  const float maxDist=MAX_OPER_PATH;
  float dist2=(from-pos).SquareSizeXZ();
  if( dist2>Square(maxDist) ) return true;

  OperMapIndex xs(toIntFloor(from.X() * InvOperItemGrid));
  OperMapIndex zs(toIntFloor(from.Z() * InvOperItemGrid));
  OperMapIndex xe(toIntFloor(pos.X() * InvOperItemGrid));
  OperMapIndex ze(toIntFloor(pos.Z() * InvOperItemGrid));

  if (xs == xe && zs == ze) return true;

  LAND_INDICES(xs, zs, xMin, zMin);
  LAND_INDICES(xe, ze, xMax, zMax);
  if (xMax < xMin) Swap(xMax,xMin);
  if (zMax < zMin) Swap(zMax,zMin);

  EntityAIFull *vehicle = GetVehicle();
  AILockerSavedState state;
  vehicle->PerformUnlock(&state);

  bool retVal = OperMap::OperFieldRangeReady(xMin, xMax, zMin, zMax);

  vehicle->PerformRelock(state);
  return retVal;
}

// plan the path as far as possible
Vector3 AIBrain::NearestIntersection(Vector3Val from, Vector3Val pos)
{
  const float maxDist=MAX_OPER_PATH;
  float dist2=(from-pos).SquareSizeXZ();
  if( dist2>Square(maxDist) ) return from;

  EntityAI *veh = GetVehicle();
  AILockerSavedState state;
  veh->PerformUnlock(&state);

  float xsf(from.X() * InvOperItemGrid);
  float zsf(from.Z() * InvOperItemGrid);
  float xef(pos.X() * InvOperItemGrid);
  float zef(pos.Z() * InvOperItemGrid);

  OperMapIndex xMinO(toIntFloor(floatMin(xsf,xef)));
  OperMapIndex zMinO(toIntFloor(floatMin(zsf,zef)));
  OperMapIndex xMaxO(toIntCeil(floatMax(xsf,xef)));
  OperMapIndex zMaxO(toIntCeil(floatMax(zsf,zef)));
  
  LAND_INDICES(xMinO, zMinO, xMin, zMin);
  LAND_INDICES(xMaxO, zMaxO, xMax, zMax);
  Assert(xMin<=xMax);
  Assert(zMin<=zMax);
  
  OperMap map;
  if ((xMax-xMin)>50 || (zMax-zMin)>50)
  {
    LogF("xMax %d, xMin %d",xMax,xMin);
    LogF("zMax %d, zMin %d",xMax,xMin);
    LogF("pos %.2f,%.2f,%.2f",pos[0],pos[1],pos[2]);
    LogF("from %.2f,%.2f,%.2f",from[0],from[1],from[2]);
  }
  map.CreateMap(GetVehicle(), xMin, zMin, xMax, zMax);
  float dist = map.FirstIntersection(xsf, zsf, xef, zef, veh->GetType(), GetCombatMode(), IsSoldier());
  Assert(dist*OperItemGrid<=from.Distance(pos)*1.001f+OperItemGrid);
  
  veh->PerformRelock(state);
  if (dist<=0) return from;
  if (dist>=FLT_MAX) return pos;
  if (Square(dist*OperItemGrid)>=from.Distance2(pos)) return pos;

  Vector3 endDir = pos-from;
  endDir.Normalize();
  Vector3 retVal = from+endDir*dist*OperItemGrid;
  return retVal;
}

#if _ENABLE_AI


/// distance in OperItemGrid to avoid searching
const float DistNotSearch = 2.0f;



#define ROAD_DIAGS 0
#define HOUSE_DIAGS 0

bool AIBrain::VerifyPath(bool waiting, float timeAhead) const
{
  // do not verify fresh path
  if (_path.GetSearchTime()>Glob.time-0.5 && !waiting)
  {
    // assume fresh path is always valid
    return true;
  }

  bool ret = true;
  EntityAI *veh=GetVehicle();
  //if (_path.GetOnRoad())
  {
    AILockerSavedState state;
    veh->PerformUnlock(&state);
    int index = _path.FindNext(veh->FutureVisualState().Position());
    float startCost = _path.CostAtPos(veh->FutureVisualState().Position());
    bool isSoldier = IsFreeSoldier();
    for (int i=index; i<_path.Size(); i++)
    {
      const OperInfoResult &pathI = _path[i];
      if (pathI._type==NTRoad || pathI._type==NTBridge)
      {
        if (pathI._road && (waiting ? pathI._road->IsLockedByWaiting(isSoldier) : pathI._road->IsLocked(isSoldier)))
        {
          ret = false;
          break;
    }
      }
      if (pathI._cost>startCost+timeAhead) // do not sample behind the point which is interesting for us
        break;
      if (pathI._cost>_path.GetReplanCost()) // do not sample behind the point which is interesting for us
        break;
    }
    veh->PerformRelock(state);
  }
  #if 0
  else
  {
    Vector3 lastPos = Position();
    {
      OperMap map;
      int x = toIntFloor(lastPos.X() * InvOperItemGrid);
      int z = toIntFloor(lastPos.Z() * InvOperItemGrid);
      int xx = x / OperItemRange;
      int zz = z / OperItemRange;
      AILockerSavedState state;
      veh->PerformUnlock(&state);
      map.CreateMap(this, xx, zz, xx, zz);
      bool result = map.GetFieldCost(x, z, true, veh, IsSoldier()) >= GET_UNACCESSIBLE;
      veh->PerformRelock(state);
      if (result)
        return true;  // cannot find valid path - don't try it
    }

    int index = _path.GetOperIndex();
    for (int i=index; i<_path.GetMaxIndex(); i++)
    {
      if (!IsSimplePath(lastPos, _path[i]._pos))
      {
        /*
        LogF
        (
        "Unit %s: path is not valid - index %d/%d",
        (const char *)GetDebugName(), i, _path.GetMaxIndex() - 1
        );
        */
        return false;
      }
      lastPos = _path[i]._pos;
    }
  }
#endif
  return ret;
}

void AIBrain::CopyPath(const IAIPathPlanner &planner)
{
  EntityAI *veh = GetVehicle();
  float combatHeight = veh->GetCombatHeight();

  int planIndex = planner.FindBestIndex(Position(GetFutureVisualState()));
  int i, n = planner.GetPlanSize() - planIndex;
  if (planner.GetPlanSize() > 0 && n > 0)
  {
    float sumCost = 0;
    Point3 lastPos = Position(GetFutureVisualState());

    _path.Resize(n + 1);
    _path[0]._pos = lastPos;
    _path[0]._cost = sumCost;
    _path[0]._type = NTGrid;

    for (i=0; i<n; i++)
    {
      PlanPosition pos;
      planner.GetPlanPosition(planIndex + i, pos);
      int x = toIntFloor(pos.X() * InvLandGrid);
      int z = toIntFloor(pos.Z() * InvLandGrid);
      GeographyInfo geogr = GLOB_LAND->GetGeography(x, z);
      float cost = veh->GetBaseCost(geogr, false, true) * veh->GetFieldCost(geogr);
      if (cost >= GET_UNACCESSIBLE)
        cost = 1.0;
      cost *= (pos - lastPos).SizeXZ();
      sumCost += cost;
      lastPos = GLandscape->PointOnSurface(pos[0],combatHeight,pos[2]);
      _path[i + 1]._pos = lastPos;
      _path[i + 1]._cost = sumCost;
      _path[i + 1]._type = NTGrid;
    }
  }
  else
  {
    int x = toIntFloor(Position(GetFutureVisualState()).X() * InvLandGrid);
    int z = toIntFloor(Position(GetFutureVisualState()).Z() * InvLandGrid);
    GeographyInfo geogr = GLOB_LAND->GetGeography(x, z);
    float cost = veh->GetBaseCost(geogr, false, true) * veh->GetFieldCost(geogr);
    if (cost >= GET_UNACCESSIBLE)
      cost = 1.0;
    cost *= (_expPosition - Position(GetFutureVisualState())).SizeXZ();
    _path.Resize(2);
    _path[0]._pos = Position(GetFutureVisualState());
    _path[0]._cost = 0;
    _path[0]._type = NTGrid;
    _path[1]._pos = GLandscape->PointOnSurface(_expPosition[0],combatHeight,_expPosition[2]);
    _path[1]._cost = cost;
    _path[1]._type = NTGrid;
  }
  if (_path.Size() >= 2)
    SetPathCosts(_path[1]._cost, _path[_path.Size() - 1]._cost);
  else
    SetPathCosts(0, 0);
  _path.SetSearchTime(Glob.time);
  _path.SetCover(CoverInfo(),false);
  ++_pathId;
  // once we are finished with strategic plan, we have nothing more to do
  _lastPlan = _path.Size()<=2;
}

void AIBrain::CreateStrategicPath(ThinkImportance prec)
{
  PROFILE_SCOPE_EX(pthSt, aiPath);

  const PlanPosition &dest = _plannedPosition;
  if (_noPath && _waitWithPlan <= Glob.time)
  {
#if LOG_THINK
    Log("  - Searching plan (strategic) to %.1f, %.1f (%d,%d).",
      dest.X(), dest.Z(), xe, ze);
#endif
    if (!_planner->IsSearching())
    {
      // begin new search
      if (!_planner->StartSearching(prec, GetVehicle(), Position(GetFutureVisualState()), dest))
        OnStrategicPathNotFound(false);
      else if (!_planner->IsSearching())
        OnStrategicPathFound();
    }
    if (_planner->IsSearching())
    {
      // continue with searching
      if (!_planner->ProcessSearching())
        OnStrategicPathNotFound(false);
      else if (!_planner->IsSearching())
        OnStrategicPathFound();
    }
  }
  else if (_updatePath && _waitWithPlan <= Glob.time)
  {
#if LOG_THINK
    Log("  - Update plan (strategic) to %.1f, %.1f (%d,%d).",
      dest.X(), dest.Z(), xe, ze);
#endif
    if (!_planner->IsSearching())
    {
      // begin new search
      if (!_planner->StartSearching(prec, GetVehicle(), Position(GetFutureVisualState()), dest))
        OnStrategicPathNotFound(true);
      else if (!_planner->IsSearching())
        OnStrategicPathFound();
    }
    if (_planner->IsSearching())
    {
      // continue with searching
      if (!_planner->ProcessSearching())
        OnStrategicPathNotFound(true);
      else if (!_planner->IsSearching())
        OnStrategicPathFound();
    }
  }
}

void AIBrain::OnStrategicPathFound()
{
  _noPath = false;
  _updatePath = false;
  _lastPlan = false;
  State state = GetState();
  if
    (
    state != Delay && state != InCargo &&
    state != Stopping && state != Stopped
    )
    Verify(SetState(Wait));
  _iter = 0;

  float totalCost = _planner->GetTotalCost();
  _completedTime = Glob.clock.GetTimeInYear() + totalCost * OneSecond;
}

void AIBrain::OnStrategicPathNotFound(bool update)
{
  _waitWithPlan = Glob.time + 10.0F;

  if (update)
  {
#if LOG_POSITION_PROBL
    LogF("Did not found path in update.");
#endif
    _completedTime = Glob.clock.GetTimeInYear() + OneDay; // set conditions for timeout
  }
}

bool AIBrain::OperPath(ThinkImportance prec)
{
/*
  EntityAI *veh = GetVehicle();
*/

  SetHouse(NULL, -1);
  Vector3Val pos = Position(GetFutureVisualState());
  float dist2;

  if
  (
    _state != Stopped &&
    _state != Stopping &&
    _state != Delay &&
    _state != InCargo
  )
  {
    if (!_noPath)
    {
      // GetCoverUsed returns false as long as there is some cover and the cover was not consumed yet
      if (!GLandscape->GetRoadNet()->IsOnRoadReady(pos)) return false; // wait until files are loaded

      // construct path by strategic plan
      int planIndex = _planner->FindBestIndex(pos);
      int minPlanIndex = planIndex;
      do
      {
        _lastPlan = _planner->GetPlanPosition(planIndex, _expPosition);
        dist2 = (_expPosition - pos).SquareSizeXZ();
      }
      while (dist2 < Square(DIST_MIN_OPER) && ++planIndex < _planner->GetPlanSize());


      if (_iter == 1)
      {
        if (planIndex > minPlanIndex)
        {
          planIndex--;
          _lastPlan = _planner->GetPlanPosition(planIndex, _expPosition);
        }
      }
      else if (_iter == 2)
      {
        if (planIndex < _planner->GetPlanSize() - 1)
        {
          planIndex++;
          _lastPlan = _planner->GetPlanPosition(planIndex, _expPosition);
        }
      }

      //DoAssert(!_lastPlan || _expPosition.Distance2(_plannedPosition)<Square(1));

      saturateMin(planIndex, _planner->GetPlanSize() - 1);

      if (_lastPlan)
      {
        if (prec <= LevelFastOperative)
        {
          SetMode(AIBrain::Exact);
        }
        else
        {
          SetMode(AIBrain::DirectExact);
        }
      }
      else
      {
        if (prec <= LevelFastOperative)
        {
          SetMode(AIBrain::Normal);
        }
        else
        {
          SetMode(AIBrain::DirectNormal);
        }
      }

      Verify(SetState(AIBrain::Init));
    }
  }
  else
  {
    _expPosition = PlanPosition(pos,false);
    _lastPlan = false;
    // do not change state (Stopping, Stopped, InCargo, Delay)
  }
  return true;
}

bool AIBrain::CreateOperativePath()
{
  PROFILE_SCOPE_EX(pthOp, aiPath);

  EntityAIFull *veh = GetVehicle();

  // new attempt
  if (_state == Delay && _delay <= Glob.time)
  {
    Verify(SetState(Wait));
    return false;
  }

  if (_state == Planning)
  {
    // planning in progress
    AILockerSavedState state;
    veh->PerformUnlock(&state);
    bool ok = GetOperMap().IsDestinationNotLocked(this) || GetOperMap().IsDone();
    bool done = false;
    if (ok)
      done = GetOperMap().ProcessFindPath(this);
    veh->PerformRelock(state);
    if (!ok)
    {
      // replan
#if DIAG_A_STAR
      if (diags)
      {
        LogF(
          "%.2f %s destination is locked - restart searching",
          Glob.time.toFloat(), cc_cast(GetVehicle()->GetDebugName()));
      }
#endif
      SetState(Init);
      return false;
    }
    if (done)
    {
#if DIAG_A_STAR
      if (diags)
      {
        if (GetOperMap().IsDone())
        {
          LogF(
            "%.2f %s finished searching of operative path",
            Glob.time.toFloat(), cc_cast(GetVehicle()->GetDebugName())
          );
        }
        else
        {
          LogF(
            "%.2f %s did not found operative path",
            Glob.time.toFloat(), cc_cast(GetVehicle()->GetDebugName())
          );
        }
      }
#endif
#if DIAG_DELAY
      float delay = 0.001f * (GlobalTickCount() - GetOperMap()._findStarted);
      if (GetGroup() && GetGroup()->Leader() == this)
      {
        LogF("*** %s - path %s, %s in %.3f s (%d iterations)", cc_cast(GetDebugName()),
          GetOperMap().IsDone() ? "done" : "not done", GetOperMap().IsFound() ? "found" : "not found",
          delay, GetOperMap()._iterOp);
      }
#endif
      bool result = GetOperMap().ExportPath(this);
      if (result) OnOperativePathFound();
      else OnOperativePathNotFound();
      FinishOperativePath(result);
      // avoid too early replan when searching of path takes too long
      GetPath().SetSearchTime(Glob.time);
      return true;
    }
    return true;
  }

  if (_state != Init)
  {
    // check if current path is valid
    if (!VerifyPath(false,2.0f))
    {
      Verify(SetState(Replan));
    }
    // do not plan
    return false;
  }

  if( veh->GetType()->StrategicPlanningOnly())
  {
    if (_mode == Exact)
      _mode = DirectExact;
    else if (_mode == Normal)
      _mode = DirectNormal;
  }
  if (_mode == DirectExact || _mode == DirectNormal)
  {
    // creates path from Position() to _expPosition if none in _planner
    CopyPath(*_planner);
    Verify(SetState(Busy));
    return false;
  }
  else
  {
    // check if plan is already completed (we are near destination point)
    // FIX: for subordinated driver, this situation is valid (commander must send command first)
    if (IsCommander())
    {
      // FIX: was SquareSizeXZ() - problems with searching inside buildings
      float dist2=(Position(GetFutureVisualState()) - _expPosition).SquareSize();
      if (dist2 < Square(DistNotSearch*OperItemGrid))
      {
        if (_lastPlan)
        {
          float precision = veh->GetPrecision();
          if (_wantedPosition.Distance(_plannedPosition) > Square(precision))
            ForceReplan();
          else
          {
            _completedReceived = false; // enforce state change
            Verify(SetState(Completed));
            // mark search time to avoid quickly repeating it
            _path.SetSearchTime(Glob.time);
          }
        }
        else
        {
          if (veh->PlanChangePossible())
          {
            Verify(SetState(Replan));
          }
        }
        return false;
      }
    }
    //_state = Busy;
    {
      // check if we need some plan
      // if not, simply report the completition
      // motivation of this is: when unit completes a waypoint because it has reached the completition circle
      // we want to avoid it planning while it may be waiting for other units
      Vector3 planFrom = veh->PositionToPlanFrom();
#if 0 // the following code is not necessary - if the unit has completed, it will not plan any more
      if (_wantedPrecision>0 && _lastPlan)
      {
        if (_expPosition.Distance2(planFrom)<Square(_wantedPrecision))
        {
          // pretend the search is waiting for something
          if (_state!=Completed)
          {
            _state = Init;
          }
          return false;
        }
      }
#endif

      bool result = CreateOperativePath(planFrom, _expPosition);
      if (_state==InitFailed)
      {
        _state = Init;
        return false;
      }
      {
        if (_state != Planning) FinishOperativePath(result);
      }
    }
    return true;
  }
}

void AIBrain::FinishOperativePath(bool found)
{
  if (!found || _path.Size() == 0)
  {
    // path not found
    LogF("%s: path not found - iteration %d", (const char *)GetDebugName(), _iter);
    if (++_iter > 2)
    {
      if (_planningMode==LeaderPlanned)
      {
        _completedReceived = false; // enforce state change - we can do it because command will be failed
        Verify(SetState(Completed));
      }
      else
      {
        // pathfinding failed for a subordinate - wait a while before retrying
        _delay = Glob.time + 5.0;
        Verify(SetState(Delay));
      }
      _iter = 0;
    }
    else
    {
      _delay = Glob.time + 5.0;
      Verify(SetState(Delay));
    }
    return;
  }

  _iter = 0;

  if (!_path.IsIntoCover())
  {
    // add strategic plan to path
    int index = -1;
    int n = _planner->GetPlanSize();
    // FIX: at least the first point is covered by the operative map
    for (int i=1; i<n; i++)
    {
      PlanPosition pos;
      _planner->GetPlanPosition(i, pos);
      if ((pos - _expPosition).SquareSizeXZ() < 1.0f)
      {
        index = i;
        break;
      }
    }
    if (index >= 0)
    {
      EntityAI *veh = GetVehicle();
      float combatHeight = veh->GetCombatHeight();

      int j = _path.Size() - 1;
      float lastCost = _path[j]._cost;
      Point3 lastPos = GLandscape->PointOnSurface(_expPosition[0],combatHeight,_expPosition[2]);
      for (int i=index+1; i<n; i++)
      {
        PlanPosition pos;
        _planner->GetPlanPosition(i, pos);
        int x = toIntFloor(pos.X() * InvLandGrid);
        int z = toIntFloor(pos.Z() * InvLandGrid);
        GeographyInfo geogr = GLOB_LAND->GetGeography(x, z);
        float cost = veh->GetBaseCost(geogr, false, true) * veh->GetFieldCost(geogr);
        if (cost >= GET_UNACCESSIBLE)
          cost = 1.0;
        cost *= (pos - lastPos).SizeXZ();
        lastCost += cost;
        lastPos = GLandscape->PointOnSurface(pos[0],combatHeight,pos[2]);
        j = _path.Add();
        _path[j]._pos = lastPos;
        _path[j]._cost = lastCost;
        _path[j]._type = NTGrid;
        //_path[j]._dir = 0xff;
      }
    }
  }
  Verify(SetState(Busy));
}

static bool InsideBuildingReady(Vector3Val pos)
{
  PROFILE_SCOPE_EX(pthBR, aiPath);

  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,pos,pos,50);
  return GWorld->CheckObjectsReady(xMin,xMax,zMin,zMax,false);
}

static const IPaths *InsideBuilding(Vector3Val pos)
{
  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,pos,pos,50);
  int x,z;
  for( x=xMin; x<=xMax; x++ ) for( z=zMin; z<=zMax; z++ )
  {
    const ObjectListUsed &list=GLandscape->UseObjects(x,z);
    int n=list.Size();
    for( int i=0; i<n; i++ )
    {
      Object *obj=list[i];

      if (!obj->GetShape()) continue;
      if (obj->GetShape()->FindPaths() < 0) continue;
      // vehicle with paths - must be building
      float dist2 = obj->FutureVisualState().Position().Distance2(pos);
      if (dist2 > Square(obj->GetShape()->BoundingSphere())) continue;
      // pos is inside vehicle's bounding

      const IPaths *building = obj->GetIPaths();
      if (!building) continue;

      // check if point is near of some path in building
      Vector3 nearest;
      int nearestIndex = building->FindNearestPoint(pos,nearest,Square(2.5));
      if( nearestIndex>=0 )
      {
        return building;
      }
      // test if pos is inside building
      // test1
      /*
      for (int j=0; j<building->NPoints(); j++)
      {
      Vector3 bpos = building->GetPosition(j);
      if (bpos.Distance2(pos) < Square(2.5f))
      return building;
      }
      */
      // test2
      /*
      CollisionBuffer result;
      obj->Intersect(result, pos, pos + Vector3(0, 0.5, 0), 0);
      if (result.Size() > 0)
      {
      LogF
      (
      "Point %.1f,%.1f,%.1f is inside %s",
      pos.X(), pos.Y(), pos.Z(),
      (const char *)obj->GetDebugName()
      );
      return building;
      }
      */
    }
  }
  //LogF("Point %.1f,%.1f,%.1f is outside", pos.X(), pos.Y(), pos.Z());
  return NULL;
}


#pragma warning(disable:4101) // prevent warning C4101: 'AllowBPMemLock' : unreferenced local variable
bool AIBrain::CreateOperativePath(Vector3Par from, const PlanPosition &to)
{
  #if 0 // _DEBUG || _PROFILE
  extern int AllowBPMemLock;
  extern void StartBPScope();
  struct ScopeBP: private NoCopy
  {
    ScopeBP(){StartBPScope();AllowBPMemLock++;}
    ~ScopeBP(){AllowBPMemLock--;}
  } allowBPInScope;
  #endif
  //DoAssert(GetVehicle()->PlanChangePossible());
  //LogF("Time %.1f: Searching path (house = %x)", Glob.time.toFloat(), _house);
  
  // this function sets path.time to time of last search only if searched
  // when data are not ready, state is set to InitFailed

  if (IsFreeSoldier() && GetPerson()->IsLadderActionInProgress())
  {
    // do not replan now
    _state = InitFailed;
    return false;
  }

  bool result = BeginOperativePath(from, to);
  if (_state == InitFailed)
  {
    return false;
  }
  if (_state != Planning)
  {
    _path.SetSearchTime(Glob.time);
    return result;
  }
  
  return false;
}
#pragma warning(default:4101)

/// compute integer coordinates and ranges needed for path-finding
static inline void CalculateOperMapRange(
 OperMapIndex &xs, OperMapIndex &zs, OperMapIndex &xe, OperMapIndex &ze,
 float &xsf, float &zsf, float &xef, float &zef,
 LandIndex &xMin, LandIndex &xMax, LandIndex &zMin, LandIndex &zMax,
 Vector3Par begPos, Vector3Par endPos
)
{
  xsf = begPos.X() * InvOperItemGrid;
  zsf = begPos.Z() * InvOperItemGrid;
  xef = endPos.X() * InvOperItemGrid;
  zef = endPos.Z() * InvOperItemGrid;

  xs = OperMapIndex(toIntFloor(xsf));
  zs = OperMapIndex(toIntFloor(zsf));
  xe = OperMapIndex(toIntFloor(xef));
  ze = OperMapIndex(toIntFloor(zef));
  
  LandIndices(xs, zs, xMin, zMin);
  LandIndices(xe, ze, xMax, zMax);
  if (xMax < xMin) Swap(xMax, xMin);
  if (zMax < zMin) Swap(zMax, zMin);
}


/*!
\patch_internal 5100 Date 12/11/2006 by Jirka
- Fixed: Path item type was sometimes uninitialized, causing warnings in serialization
\patch 5142 Date 3/19/2007 by Jirka
- Fixed: AI - path planning improved (more smooth transition between plans)
*/


#define REUSE_PATH 1
#define DIAG_REUSE_PATH 0

bool AIBrain::BeginOperativePath(Vector3Par fromSrc, const PlanPosition &pos)
{
  #if 0 // _PROFILE
  if (GetUnit() && GetUnit()->ID()==2)
  {
    LogF("%s: search to %.1f,%.1f",cc_cast(GetDebugName()),pos.X(),pos.Z());
  }
  #endif
  Vector3 from = fromSrc;
  // from will be updated, make own copy

  PROFILE_SCOPE_EX(pthBe, aiPath);
  DoAssert(_state != Planning);

  // store search parameters
  _from = from;
  _fromBusy = false;
  _to = pos;
  
  _toRoad = false;
  // keep original position in _from, manipulate with from (and set _fromBusy when changing)
#if DIAG_REUSE_PATH
  LogF("%.3f Path searching for %s started", Glob.time.toFloat(), cc_cast(GetDebugName()));
#endif

  GetPath().SetSearchTime(Glob.time);
  _oldPath.Clear();

  float dist2 = _to.DistanceXZ2(from);
  if (dist2 > Square(MAX_OPER_PATH))
  {
    LogF("Error: Unit %s is searching path per distance %.0f", (const char *)GetDebugName(), sqrt(dist2));
    ClearOperativePlan();
    return false;
  }

  EntityAI *veh = GetVehicle();
  bool isSoldier = IsFreeSoldier();
  const EntityAIType *vehType = veh->GetType();
  float combatHeight = veh->GetCombatHeight();
  float vehWidth = 0;
  if (!isSoldier)
  {
    LODShape *shape = vehType->GetShape();
    if (shape) vehWidth = shape->Max().X() - shape->Min().X();
  }

  AILockerSavedState state;
  veh->PerformUnlock(&state);

  // calculate the original direction (we can change it later when reusing the old path)
  int dir;
  if (GetVehicleIn() && GetVehicleIn()->GetMoveMode() == VMMBackward)
    dir = CalcDirection(-Direction(GetFutureVisualState()));
  else
    dir = CalcDirection(Direction(GetFutureVisualState()));

  
  {
#if REUSE_PATH
    if (_path.Size() >= 2)
    {
      // valid path
      //int next = _path.FindNext(from);
      
      float curCost = _path.CostAtPos(from);
      float maxCost = _path.GetForceReplanCost();
      // avoid reusing too short path, could create a singular path
      // note: on roads even a short (even a singular) path can be useful
      if (curCost < maxCost)
      {
        // possible to reuse part of path
        bool allowReuse = true;
        static float reuseCost = 3.0f;
        float fromCost = floatMin(curCost + reuseCost, curCost + 0.75f * (maxCost - curCost));
        
        int endIndex = _path.FindNextCost(fromCost);
        int startIndex = intMax(_path.FindNextCost(curCost)-1,0);
        
        // once on road, reuse the road part only, and only the part which is not locked
        bool roadOnly = _path[startIndex]._type==NTRoad || _path[startIndex]._type==NTBridge;
        if (roadOnly && _path[startIndex]._road && _path[startIndex]._road->IsLocked(isSoldier))
          {
          allowReuse = false;
        }
        else for (int i=startIndex+1; i<=endIndex; i++)
            {
          if (_path[i]._type==NTRoad || _path[i]._type==NTBridge)
          {
            if (_path[i]._road && _path[i]._road->IsLocked(isSoldier))
            {
              endIndex = i-1;
              fromCost = _path[endIndex]._cost;
              break;
            }
            roadOnly = true;
          }
          else if (roadOnly)
          {
            // no longer on the road - stop reusing
            endIndex = i-1;
            fromCost = _path[endIndex]._cost;
            break;
        }
          else if (_path[i]._type==NTGrid)
          {
            // check the lock
            ILockCache *locks = GLandscape->LockingCache();
        
            float invOperItemGrid = InvOperItemGrid;
            OperMapIndex x(toIntFloor(_path[i]._pos[0]*invOperItemGrid)); // inverse of _x * OperItemGrid + 0.5f * OperItemGrid
            OperMapIndex z(toIntFloor(_path[i]._pos[2]*invOperItemGrid)); // inverse of _z * OperItemGrid + 0.5f * OperItemGrid

            if (locks->IsLocked(x,z,isSoldier)) // do not copy through locked items
        {
              endIndex = i-1;
              fromCost = _path[endIndex]._cost;
              break;
            }
          }
        }

        if (allowReuse)
        {

          if (_path[endIndex]._type == NTGrid)
          {
          _from = _path.PosAtCost(fromCost); // interpolation enabled only in the free landscape
        }
        else
        {
          // in case of roads always select the previous node to make sure we are on the path
            const OperInfoResult &onPath = _path[endIndex];
          _from = onPath._pos;
          fromCost = onPath._cost;
          // adjust infoFrom to be on the road
        }

          if (endIndex > 0) dir = CalcDirection(_path[endIndex]._pos - _path[endIndex - 1]._pos);

        from = _from;
        _fromBusy = false;

        // if the difference is small and the copied path would be very short, do not copy it
        // but still adjust from
        if (fromCost-curCost>1e-3f)
        {
          // copy the segment of the path to _oldPath
          for (int i=0; i<_path.Size(); i++)
          {
              if (_path[i]._cost < curCost) continue; // skip the part before us
            if (_oldPath.Size() == 0)
            {
              // add the first node
              if (i == 0)
              {
                _oldPath.Add(_path[0]);
                continue;
              }
              if (_path[i]._cost > curCost)
              {
                _oldPath.Add(_path[i - 1]);
                _oldPath[0]._cost = 0;
                _oldPath[0]._action = NULL;
                // interpolate the position by cost
                float denom = _path[i]._cost - _path[i - 1]._cost;
                if (denom > 0)
                {
                  float factor = (curCost - _path[i - 1]._cost) / denom;
                  _oldPath[0]._pos = (1.0f - factor) * _path[i - 1]._pos + factor * _path[i]._pos;
                }
              }
            }
            if (_path[i]._cost < fromCost)
            {
              // add the intermediate node
              int index = _oldPath.Add(_path[i]);
              _oldPath[index]._cost -= curCost;
            }
            else
            {
              // add the last node
              Assert(i > 0);
              int index = _oldPath.Add(_path[i]);
              _oldPath[index]._cost = fromCost - curCost;
              _oldPath[index]._action = NULL;
              // interpolate the position by cost
              float denom = _path[i]._cost - _path[i - 1]._cost;
              if (denom > 0)
              {
                float factor = (fromCost - _path[i - 1]._cost) / denom;
                _oldPath[index]._pos = (1.0f - factor) * _path[i - 1]._pos + factor * _path[i]._pos;
              }
              break;
            }
          }
          _oldPath.Compact();
        }
      }
      }
#if DIAG_REUSE_PATH
      if (curCost < maxCost)
      {
        LogF(" - path section %.3f ... %.3f can be used", curCost, maxCost);
      }
      else
      {
        LogF(" - no path section can be used - %.3f >= %.3f", curCost, maxCost);
      }
#endif
    }
#if DIAG_REUSE_PATH
    else
    {
      LogF(" - no valid old path");
    }
#endif
#endif // REUSE_PATH
  }

  // starting and ending node info (this structure contains all needed info)
  OperInfo infoFrom, infoTo;

  // store the info about the road we are on for later use
  RoadNode roadNodeFrom(NULL, -1), roadNodeTo(NULL, -1);

  // check if moving from road / bridge
  if (veh->GetType()->CanUseRoads())
  {
    DoAssert(GLandscape->GetRoadNet()->IsOnRoadReady(from));
    float size = veh->GetShape() ? veh->GetShape()->GeometrySphere() : 0;
    RoadLink *road = unconst_cast(GLandscape->GetRoadNet()->IsOnRoad(from, size, size));
    if (road && !road->IsLocked(isSoldier))
    {
      roadNodeFrom = road->FindBestNode(isSoldier, vehWidth, from, veh->FutureVisualState().Direction());
      if (roadNodeFrom.IsValid())
      {
        // check if this is a bridge or the plain road
        if (road->IsBridge())
        {
          // force the end position on the bridge
          infoFrom._road = roadNodeFrom;
          infoFrom._type = NTBridge;
          from = roadNodeFrom.Position(isSoldier, vehWidth);
          _fromBusy = true; // use prepending to be sure vehicle is not far from the path
        }
        else
        {
          Vector3 roadPos = roadNodeFrom.Position(isSoldier,vehWidth);
          float abovePath = veh->GetCombatHeight();
          float dist2 = roadPos.DistanceXZ2(from-Vector3(0,abovePath,0));
          // Prepending removed when not necessary, final path looked strange often (acute angles)
          // when planning on road, place vehicle on road
          // when we have a perfect match, we will use it
          if (dist2<Square(0.05f))
          {
            infoFrom._road = roadNodeFrom;
            infoFrom._type = NTRoad;
            from = roadPos;
            _fromBusy = true; // use prepending to be sure vehicle is not far from the path
          }
        }
      }
    }
  }

  // check if moving to the house
  if (isSoldier && _house)
  {
    // target is the house position
    const IPaths *paths = _house->GetIPaths();
    if (paths)
    {
      // what position
      int posTo = paths->GetPos(_housePos);
      if (posTo < 0)
      {
        // try alternate (any) position
        posTo = paths->GetPos(0);
      }
      if (posTo >= 0)
      {
        // position found, store it
        infoTo._type = NTHouse;
        infoTo._house = _house;
        infoTo._housePos = posTo;
        // set the exact destination position
        _to = PlanPosition(paths->GetPosition(posTo),false);
      }
    }
  }
  // check if moving to road / bridge
  if (infoTo._type != NTHouse && veh->GetType()->CanUseRoads())
  {
    if (!GLandscape->GetRoadNet()->IsOnRoadReady(_to))
    {
      // wait with the planning until roads are loaded
      veh->PerformRelock(state);
      _state = InitFailed;
      return false;
    }
    
    // adjust ending node to be on the road net when possible
    float size = veh->GetShape() ? veh->GetShape()->GeometrySphere() : 0;
    RoadLink *road = unconst_cast(GLandscape->GetRoadNet()->IsOnRoad(_to, size, size));
    // Avoid planning to a locked road segment - IsDestinationFree will fail and the searching will be restarted repeatedly
    if (road && !road->IsLocked(isSoldier))
    {
      roadNodeTo = road->FindBestNode(isSoldier, vehWidth, _to, (_to - from).Normalized());
      // check if this is a bridge or the plain road
      if (road->IsBridge())
      {
        // force the end position on the bridge
        infoTo._road = roadNodeTo;
        infoTo._type = NTBridge;
        _to = PlanPosition(roadNodeTo.Position(isSoldier, vehWidth),false);
        _toRoad = true;
      }
      else if (!_lastPlan || roadNodeTo.Position(isSoldier,vehWidth).DistanceXZ2(_to)<OperItemGrid)
      {
        infoTo._road = roadNodeTo;
        infoTo._type = NTRoad;
        // when path end on this road, do not change target to _to
        _toRoad = true;
      }
      /*
      else if (veh->IsMovingInConvoy() && !isSoldier)
      {
        infoTo._road = roadNodeTo;
        infoTo._type = NTRoad;
        // when path end on this road, do not change target to _to
        _toRoad = true;
      }
      */
    }
  }


  // calculate the range of operative map
  float xsf, zsf, xef, zef;
  OperMapIndex xs(-1), zs(-1), xe(-1), ze(-1);
  LandIndex xMin(-1), xMax(-1), zMin(-1), zMax(-1);
  CalculateOperMapRange(xs, zs, xe, ze, xsf, zsf, xef, zef, xMin, xMax, zMin, zMax, from, _to);

  // create the operative map if possible
  if (!GetOperMap().OperFieldRangeReady(xMin, xMax, zMin, zMax))
  {
    // wait with the planning until objects are loaded
    veh->PerformRelock(state);
    _state = InitFailed;
    return false;
  }
  // we need the fields to be ready for pathing here
  GetOperMap().CreateMap(
    veh, xMin, zMin, xMax, zMax, MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER | MASK_FOR_PATHING
  );
  Assert(GetOperMap()._jobState == Job::JSNone || GetOperMap()._jobState == Job::JSCanceled || GetOperMap()._jobState == Job::JSDone);

#if 0
  // memory usage
  LogF("FindPath statistics:");
  LogF(
    "\t%s is searching path from %d, %d to %d,%d - distance %.0f",
    (const char *)veh()->GetType()->GetDisplayName(), xs, zs, xe, ze, sqrt(dist2));
  LogF(
    "\tusing %d fields, memory %d",
    GetOperMap()._fields.Size(), GetOperMap()._fields.Size() * sizeof(OperField));
  LogF(
    "\ttotal in cache %d(soldiers) + %d(others)",
    GLandscape->OperationalCacheSoldiers()->NFields() * sizeof(OperField),
    GLandscape->OperationalCache()->NFields() * sizeof(OperField));
#endif

  // check if starting place is accessible
  if (infoFrom._type != NTHouse && infoFrom._type != NTBridge && GetOperMap().GetFieldCost(xs, zs, true, vehType, CMAware, isSoldier, false).cost >= GET_UNACCESSIBLE)
  {
    if (isSoldier)
    {
      // first look if we are in some (passable) building
      if (!InsideBuildingReady(from))
      {
        veh->PerformRelock(state);
        _state = InitFailed;
        return false;
      }
      const IPaths *paths = InsideBuilding(from);
      if (paths)
      {
        Vector3 dummy;
        int posFrom = paths->FindNearestPoint(from - Vector3(0, combatHeight, 0), dummy);
        if (posFrom < 0)
        {
          // try alternate (any) position
          posFrom = paths->GetPos(0);
        }
        if (posFrom >= 0)
        {
          // position found, store it
          infoFrom._type = NTHouse;
          infoFrom._house = unconst_cast(paths->GetObject());
          infoFrom._housePos = posFrom;
        }
      }
    }
    if (infoFrom._type != NTHouse)
    {
      bool found = GetOperMap().FindNearestEmpty(xs, zs, xsf, zsf,
        OperMapIndex(xMin * OperItemRange), OperMapIndex(zMin * OperItemRange),
        OperMapIndex((xMax - xMin + 1) * OperItemRange), OperMapIndex((zMax - zMin + 1) * OperItemRange),
        true, vehType, isSoldier, false, DefFindFreePositionCallback, NULL);

      bool useRoad = false;
      if (found)
      {
        if (roadNodeFrom.IsValid())
        {
          Vector3 road = roadNodeFrom.Position(isSoldier, vehWidth);
          Vector3 nearest;
          nearest[0] = xs * OperItemGrid + 0.5 * OperItemGrid;
          nearest[1] = 0;
          nearest[2] = zs * OperItemGrid + 0.5 * OperItemGrid;
          useRoad = road.DistanceXZ2(from) < nearest.DistanceXZ2(from);
        }
      }
      else
      {
        // each fall-back is good enough
        useRoad = roadNodeFrom.IsValid();
      }

      if (useRoad)
      {
        infoFrom._road = roadNodeFrom;
        infoFrom._type = NTRoad;
        from = roadNodeFrom.Position(isSoldier, vehWidth);
        _fromBusy = true; // use prepending to be sure vehicle is not far from the path

        xs = OperMapIndex(toIntFloor(from.X() * InvOperItemGrid));
        zs = OperMapIndex(toIntFloor(from.Z() * InvOperItemGrid));
      }
      else if (found)
      {
        // mark the path is searched from a different position
        _fromBusy = true;
        /*
        // from must be changed (is forced into first path point)
        _from[0] = xs * OperItemGrid + 0.5 * OperItemGrid;
        _from[2] = zs * OperItemGrid + 0.5 * OperItemGrid;
        */
      }
      else
      {
        LogF("Unit %s: Start point not found", (const char *)GetDebugName());
#if LOG_PROBL
        GetOperMap().LogMap(xMin, xMax, zMin, zMax, xs, xe, zs, ze, false);
#endif
        GetOperMap().ClearMap();
        veh->PerformRelock(state);
        return false;
      }
    }
  }

  // check if ending place is accessible
  if (infoTo._type != NTHouse && infoTo._type != NTBridge && GetOperMap().GetFieldCost(xe, ze, true, vehType, CMAware, isSoldier, false).cost >= GET_UNACCESSIBLE)
  {
    if (isSoldier)
    {
      // first look if we are in some (passable) building
      if (!InsideBuildingReady(_to))
      {
        veh->PerformRelock(state);
        _state = InitFailed;
        return false;
      }
      const IPaths *paths = InsideBuilding(_to);
      if (paths)
      {
        Vector3 dummy;
        int posTo = paths->FindNearestPoint(_to - Vector3(0, combatHeight, 0), dummy);
        if (posTo < 0)
        {
          // try alternate (any) position
          posTo = paths->GetPos(0);
        }
        if (posTo >= 0)
        {
          // position found, store it
          infoTo._type = NTHouse;
          infoTo._house = unconst_cast(paths->GetObject());
          infoTo._housePos = posTo;
        }
      }
    }

    if (infoTo._type != NTHouse && (infoTo._type!=NTGrid || !_to._cover))
    {
      bool found = GetOperMap().FindNearestEmpty(xe, ze, xef, zef,
        OperMapIndex(xMin * OperItemRange), OperMapIndex(zMin * OperItemRange),
        OperMapIndex((xMax - xMin + 1) * OperItemRange), OperMapIndex((zMax - zMin + 1) * OperItemRange),
        true, vehType, isSoldier, false, DefFindFreePositionCallback, NULL);

      bool useRoad = false;
      if (found)
      {
        if (roadNodeTo.IsValid())
        {
          Vector3 road = roadNodeTo.Position(isSoldier, vehWidth);
          Vector3 nearest;
          nearest[0] = xe * OperItemGrid + 0.5 * OperItemGrid;
          nearest[1] = 0;
          nearest[2] = ze * OperItemGrid + 0.5 * OperItemGrid;
          useRoad = road.DistanceXZ2(_to) < nearest.DistanceXZ2(_to);
        }
      }
      else
      {
        // each fall-back is good enough
        useRoad = roadNodeTo.IsValid();
      }

      if (useRoad)
      {
        infoTo._road = roadNodeTo;
        infoTo._type = NTRoad;
        _to = PlanPosition(roadNodeTo.Position(isSoldier, vehWidth),false);
        _toRoad = true;
        xe = OperMapIndex(toIntFloor(_to.X() * InvOperItemGrid));
        ze = OperMapIndex(toIntFloor(_to.Z() * InvOperItemGrid));
      }
      else if (found)
      {
        // pos must be changed (is forced into last path point)
        // ignore vehicle locks for alternate targets - enable moving of soldiers near other soldiers or vehicles
        _to[0] = xe * OperItemGrid + 0.5 * OperItemGrid;
        _to[2] = ze * OperItemGrid + 0.5 * OperItemGrid;
        if (pos.Distance2(_expPosition) < Square(1))
        {
          // update also _expPosition to ensure unit reach "Complete" state
          _expPosition = PlanPosition(_to,_expPosition._cover);
        }
      }
      else
      {
        LogF("Unit %s: End point not found", (const char *)GetDebugName());
#if LOG_PROBL
        GetOperMap().LogMap(xMin - border, xMax + border, zMin - border, zMax + border, xs, xe, zs, ze, false);
#endif
        GetOperMap().ClearMap();
        veh->PerformRelock(state);
        return false;
      }
    }
  }

  // path inside the same field
  if (infoFrom._type != NTHouse && infoTo._type != NTHouse && xe == xs && ze == zs)
  {
    // trivial path
    GetOperMap()._path.Resize(2);
      
    GetOperMap()._path[0]._x = xs;
    GetOperMap()._path[0]._z = zs;
    GetOperMap()._path[0]._cost = 0;
    GetOperMap()._path[0]._house = NULL;
    GetOperMap()._path[0]._housePos = -1;
    GetOperMap()._path[0]._road = infoFrom._road;
    GetOperMap()._path[0]._type = infoFrom._type;
    
    int x = toIntFloor(from.X() * InvLandGrid);
    int z = toIntFloor(from.Z() * InvLandGrid);
    GeographyInfo info = GLOB_LAND->GetGeography(x, z);

    GetOperMap()._path[1]._x = xe;
    GetOperMap()._path[1]._z = ze;
    GetOperMap()._path[1]._cost = floatMax(veh->GetBaseCost(info, false, true) * _to.DistanceXZ(from), 0.1); // avoid zero cost
    GetOperMap()._path[1]._house = NULL;
    GetOperMap()._path[1]._housePos = -1;
    GetOperMap()._path[1]._road = infoTo._road;
    GetOperMap()._path[1]._type = infoTo._type;

    // mark the path as not leading into any cover
    GetOperMap()._pathToOpenCover = false;
    GetOperMap()._pathToCover = CoverInfo();
    
    GetOperMap().SetAlternateGoal(infoTo._type != NTBridge);

    veh->PerformRelock(state);
    OnOperativePathFound();

    // FIX: clear after OperMap::ResultPath called from OnOperativePathFound (roads info is needed)
    GetOperMap().ClearMap();
    return true;
  }

  // path inside the house - the same position
  if (infoFrom._type == NTHouse && infoTo._type == NTHouse && infoFrom._house == infoTo._house && infoFrom._housePos == infoTo._housePos)
  {
    // trivial path
    GetOperMap()._path.Resize(2);

    GetOperMap()._path[0]._x = xs;
    GetOperMap()._path[0]._z = zs;
    GetOperMap()._path[0]._cost = 0;
    GetOperMap()._path[0]._house = infoFrom._house;
    GetOperMap()._path[0]._housePos = infoFrom._housePos;
    GetOperMap()._path[0]._road = RoadNode(NULL, -1);
    GetOperMap()._path[0]._type = NTHouse;

    GetOperMap()._path[1]._x = xe;
    GetOperMap()._path[1]._z = ze;
    GetOperMap()._path[1]._cost = 0.1; // avoid zero cost
    GetOperMap()._path[1]._house = infoTo._house;
    GetOperMap()._path[1]._housePos = infoTo._housePos;
    GetOperMap()._path[1]._road = RoadNode(NULL, -1);
    GetOperMap()._path[1]._type = NTHouse;

    // mark the path as not leading into any cover
    GetOperMap()._pathToOpenCover = false;
    GetOperMap()._pathToCover = CoverInfo();
    
    GetOperMap().SetAlternateGoal(true);

    veh->PerformRelock(state);
    OnOperativePathFound();

    // FIX: clear after OperMap::ResultPath called from OnOperativePathFound (roads info is needed)
    GetOperMap().ClearMap();
    return true;
  }

  // finish the infoFrom, infoTo
  infoFrom._x = xs;
  infoFrom._z = zs;
  infoTo._x = xe;
  infoTo._z = ze;

  CoverDisposition dispo;
  dispo._maxCost = _maxCostToCover;
  dispo._maxDelay = _maxDelayByCover;
  GetHideFrom(dispo._hideFromL,dispo._hideFromR);
  // for the formation leader or an agent we assume we are moving to a planned destination
  dispo._movingTo = pos;

  if (GetOperMap().InitFindPath(this, infoFrom, infoTo, dir, true, dispo))
  {
    // simple path found
    veh->PerformRelock(state);
    OnOperativePathFound();
    return true;
  }

  SetState(Planning);
  veh->PerformRelock(state);
  return false;
}

/*!
\patch 5101 Date 12/12/2006 by Jirka
- Fixed: AI pathfinding improved (unexpected leaving of roads will happen less frequently)
*/

void AIBrain::OnOperativePathFound()
{
  EntityAI *veh = GetVehicle();
  AILockerSavedState state;
  veh->PerformUnlock(&state);

  GetOperMap().ResultPath(this,_fromBusy,_from);
  ++_pathId;

  #if 0 // _PROFILE
  if (GetUnit() && GetUnit()->ID()==2)
  {
    LogF("%s: found, id %d",cc_cast(GetDebugName()),_pathId);
  }
  #endif
  
  // 
  if (
    _lastPlan && GetOperMap().IsAlternateGoal() && GetOperMap().CanReplanLater()
    && !_to._cover // planning into cover: whatever we find is the final destination, we will never find a better one
  )
  {
    // real destination was not found yet, we need to prevent command completing
    // one exception is when we fit the requirements given by the command
    if (_wantedPrecision>0 && _path.Size()>=2 && _plannedPosition.Distance2(_path.Last()._pos)<Square(_wantedPrecision))
    {
      // even when alternate, the path is precise enough for the current command
    }
    else
    {
      // this will make sure next planning is done later
      // note: this is used for cover as well, not only for alternate destinations
      _lastPlan = false;
    }
  }



#if DIAG_REUSE_PATH
  LogF("%.3f Path searching for %s finished", Glob.time.toFloat(), cc_cast(GetDebugName()));
  LogF(" - current position %.2f, %.2f", Position().X(), Position().Z());
  LogF(" - planned from %.2f, %.2f", _from.X(), _from.Z());
#endif

#if REUSE_PATH
  int oldSize = _oldPath.Size();
  if (oldSize > 0 && _path.Size() > 0)
  {
    // check for shortcut
    for (int i=0; i<oldSize - 1; i++)
    {
      if (_oldPath[i]._pos.Distance2(_path[0]._pos) < 0.001f)
      {
        oldSize = i + 1;
        break;
      }
    }

    if (_oldPath[0]._pos.Distance2(_path[0]._pos)<Square(0.1f))
    {
      LogF("Reusing strange path - singularity");
    }
    else if (_oldPath[oldSize-1]._pos.Distance2(_path[0]._pos)<Square(0.1f))
    {
      //LogF("Reusing created a singular path (%s)",cc_cast(GetDebugName()));
    }
    
    float prefixCost = _oldPath[oldSize - 1]._cost;
    _path.InsertMultiple(0, oldSize - 1); // avoid duplicity
    for (int i=0; i<oldSize-1; i++) _path[i] = _oldPath[i];
    for (int i=oldSize-1; i<_path.Size(); i++) _path[i]._cost += prefixCost;

#if DIAG_REUSE_PATH
    LogF(" - part of old path used:");
    for (int i=0; i<oldPath.Size(); i++)
    {
      LogF("   Position %.2f, %.2f; cost %.3f", oldPath[i]._pos.X(), oldPath[i]._pos.Z(), oldPath[i]._cost);
    }
#endif
  }
#endif

  float combatHeight = veh->GetCombatHeight();

  // FIX: avoid moving of path origin at all - can cause problems when vehicle moves during the planning
  /*
  // move path origin - cost recalculation is needed
  // FIX: do not recalculate if unit starts in house
  if (_path.Size() >= 2 && !_path[1]._house)
  {
    float originalDistance = _path[1]._pos.Distance(_path[0]._pos);
    _path[0]._pos = GLandscape->PointOnSurface(_from[0], combatHeight, _from[2]);
    float newDistance = _path[1]._pos.Distance(_path[0]._pos);
    if (originalDistance > 0)
    {
      float costChange = _path[1]._cost * (newDistance - originalDistance) / originalDistance;
      for (int i=1; i<_path.Size(); i++)
        _path[i]._cost += costChange;
    }
    else
    {
      int x = toIntFloor(_from.X() * InvLandGrid);
      int z = toIntFloor(_from.Z() * InvLandGrid);
      GeographyInfo info = GLOB_LAND->GetGeography(x, z);
      float costChange = veh->GetCost(info) * newDistance - _path[1]._cost;
      for (int i=1; i<_path.Size(); i++)
        _path[i]._cost += costChange;
    }
  }
  */

  // move path target - cost recalculation is needed
  if (!GetOperMap().IsAlternateGoal())
  {
    int last = _path.Size() - 1;
    Assert(last > 0);
    // do not force the path target when moving to house or want to stay on road
    if (last > 0 && !_path[last]._house && !(_toRoad && _path[last]._type == NTRoad))
    {
      float originalDistance = _path[last]._pos.Distance(_path[last - 1]._pos);
      Vector3Val toPos = GLandscape->PointOnSurface(_to[0], combatHeight, _to[2]);
#if _ENABLE_REPORT
      float distanceMoved = _path[last]._pos.Distance(toPos);
      if (distanceMoved>5)
      {
        LogF(
          "%s: path end point moved %.1f m: %.1f,%.1f to %.1f,%.1f",
          cc_cast(GetDebugName()),distanceMoved,_path[last]._pos.X(),_path[last]._pos.Z(),toPos.X(),toPos.Z()
        );
      }
#endif
      _path[last]._pos = toPos;
      float newDistance = _path[last]._pos.Distance(_path[last - 1]._pos);
      if (originalDistance > 0)
      {
        float costChange = (_path[last]._cost - _path[last - 1]._cost) * (newDistance - originalDistance) / originalDistance;
        _path[last]._cost += costChange;
      }
      else
      {
        // insert some non-zero cost
        float costLast = newDistance * veh->GetType()->GetMinCost();
        _path[last]._cost = _path[last - 1]._cost + costLast;
      }
    }
  }
  /**/

  const float costReplanAbs = 10.0f;
  const float costReplanRel = 0.45f;
  const float costForceReplanAbs = 2.0f;
  const float costForceReplanRel = 0.05f;

  int n = _path.Size();
  if (n >= 2)
  {
    float totalCost = _path[n - 1]._cost;
    // when planning into a cover or into a last position, the whole path is valid
    if (_lastPlan || _path.IsIntoCover())
    {
      /*
      LogF("*** Last plan limit %.2f / %.2f", totalCost, totalCost);
      */
      SetPathCosts(totalCost, totalCost);
    }
    else
    {
      // mark end of the path so that we start replanning soon enough
      float costReplan = totalCost - floatMin(costReplanAbs, totalCost * costReplanRel);
      float costForceReplan = totalCost - floatMin(costForceReplanAbs, totalCost * costForceReplanRel);
      /*
      LogF("*** New limit max(%.2f, %.2f) = %.2f / max(%.2f, %.2f) = %.2f",
      totalCost - costReplanAbs, totalCost * (1.0f - costReplanRel), costReplan,
      totalCost - costForceReplanAbs, totalCost * (1.0f - costForceReplanRel), costForceReplan);
      */
      SetPathCosts(costReplan, costForceReplan);
    }
  }
  else
  {
    RptF(
      "Error %s: Invalid path from [%.2f, %.2f, %.2f] to [%.2f, %.2f, %.2f].",
      (const char *)GetDebugName(),
      _from.X(), _from.Y(), _from.Z(),
      _to.X(), _to.Y(), _to.Z());
    SetPathCosts(0, 0);
  }

  // ????????
  if (GetOperMap().IsAlternateGoal())
  {
    // searching path to _expPosition?
    // DoAssert(_expPosition.Distance2(_to) < 1); - _to may be changed
    // update _expPosition to avoid state switching loop
    int last = _path.Size() - 1;
    _expPosition = PlanPosition(_path[last]._pos,_expPosition._cover);
  }
  // ????????

  GetOperMap().ClearMap();
  veh->PerformRelock(state);

  DoAssert (VerifyPath());
}

void AIBrain::OnOperativePathNotFound()
{
  LogF("Unit %s: Operative path not found", (const char *)GetDebugName());
#if LOG_PROBL
  GetOperMap().LogMap(xMin, xMax, zMin, zMax, xs, xe, zs, ze, false);
#endif
  GetOperMap().ClearMap();
}

#endif

void AIBrain::RemoveAction(const PathAction *action)
{
  for (int i=0; i<_path.Size(); i++)
  {
    if (_path[i]._action == action)
    {
      _path[i]._action = NULL;
      break;
    }
  }
}

/*!
\patch 5137 Date 3/7/2007 by Bebul
- Fixed: Freelook still forced when changing from cargo to secondary turret 
*/
/// Functor changing the AIBrain::_state of gunner
class SetGunnerStateFunc : public ITurretFunc
{
protected:
  AIBrain::State _state;

public:
  SetGunnerStateFunc(AIBrain::State state) : _state(state) {}
  virtual bool operator ()(EntityAIFull *entity, TurretContext &context)
  {
    if (!context._gunner || !context._turret) return false; // continue
    AIBrain *unit = context._gunner->Brain();
    if (unit)
    {
      //if (state == Wait) ClearOperativePlan();
      unit->_state = _state;
    }
    return false; // for each    
  }
};

/// Functor clearing operative map of gunner
class ClearGunnerMapFunc : public ITurretFunc
{
public:
  ClearGunnerMapFunc() {}
  virtual bool operator ()(EntityAIFull *entity, TurretContext &context)
  {
    if (!context._gunner || !context._turret) return false; // continue
    AIBrain *unit = context._gunner->Brain();
    if (unit) unit->GetOperMap().ClearMap();
    return false; // for each    
  }
};

bool AIBrain::SetState(State state, bool vehChanged)
{
#if _ENABLE_REPORT
  if (_state == Stopping && state != Stopping && state!=Stopped)
  {
    RptF("# Vehicle %s - stopping state changed to %s", cc_cast(GetVehicle()->GetDebugName()), cc_cast(FindEnumName(state)));
  }
#endif

#if DIAG_A_STAR
  bool diags = GWorld->CameraOn() == GetVehicle();
  if (diags && (state == Wait || state == Replan))
  {
    LogF(
      "%.2f %s operative path replan needed",
      Glob.time.toFloat(), cc_cast(GetVehicle()->GetDebugName()));
  }
#endif

  if (_state == Planning && state != Planning)
  {
    // planning was interrupted, delete working space
    if (_inVehicle)
    {
      AIBrain *unit;
      unit = _inVehicle->PilotUnit();
      if (unit) unit->GetOperMap().ClearMap();
      ClearGunnerMapFunc clearGunnerMap;
      _inVehicle->ForEachTurret(clearGunnerMap);
    }
    else
    {
      GetOperMap().ClearMap();
    }
  }

  if (state == Completed)
  {
#if DIAG_WANTED_POSITION
if (GWorld->FocusOn() && GWorld->FocusOn()->GetVehicle() == GetVehicle())
{
  LogF(
    "Unit %s: state changed to completed",
    cc_cast(GetDebugName()));
}
#endif

    if (_planningMode == DoNotPlan || _planningMode==DoNotPlanFormation) return true;

    if (_completedReceived) return true;
    _completedReceived = true;
    //LogF("Unit %s: Completed received", (const char *)GetDebugName());

    /*
    if (IsSubgroupLeaderVehicle() && GetSubgroup()->HasCommand())
    {
    Command *cmd = GetSubgroup()->GetCommand();
    LogF("%s: Completed (%d)", (const char *)GetDebugName(), _planningMode);
    LogF("  position: %.0f, %.0f", Position().X(), Position().Z());
    LogF("  planned: %.0f, %.0f", _plannedPosition.X(), _plannedPosition.Z());
    LogF("  wanted: %.0f, %.0f", _wantedPosition.X(), _wantedPosition.Z());
    LogF("  command: %.0f, %.0f", cmd->_destination.X(), cmd->_destination.Z());
    if (cmd->_destination.Distance2(_wantedPosition) > Square(1))
    {
    Fail("  Error !!!");
    }
    if (cmd->_destination.Distance2(Position()) > Square(10))
    {
    Fail("  Error !!!");
    }
    }
    */
  }


  if (state == InCargo || state == Planning)
  {
    _state = state;
    return true;
  }

  // all units in the vehicle should share the state
  if (_inVehicle)
  {
    
    AIBrain *unit = _inVehicle->PilotUnit();
    if (unit && unit->GetState()!=AIBrain::Stopping && (unit->GetState()!=AIBrain::Stopped || _inVehicle->CanCancelStop()))
    {
      //if (state == Wait) ClearOperativePlan();
      unit->_state = state;
    }
    
    SetGunnerStateFunc func(state);
    _inVehicle->ForEachTurret(func);

    // lines above may set state for other units in the vehicle, but may skip "this"
    // we need to make sure we always set it for the one we are calling it for
    _state = state;
  }
  else
  {
    if (!vehChanged)
    {
      Assert(_state != InCargo);
      if (_state == InCargo) return false;
    }
    //if (state == Wait) ClearOperativePlan();
    _state = state;
  }
  return true;
}



bool AIBrain::IsPlanning() const
{
  return _noPath || _updatePath || _state == Init || _state == Planning || _state == Delay;
}

void AIBrain::UpdateOperativePlan()
{
  if (IsAnyPlayer()) return;
#if DIAG_A_STAR
  bool diags = GWorld->CameraOn() == GetVehicle();
  if (diags)
  {
    LogF(
      "%.2f %s operative path replan forced",
      Glob.time.toFloat(), cc_cast(GetVehicle()->GetDebugName()));
  }
#endif

  ClearOperativePlan();
  if (_state != Planning && _state!=InCargo) SetState(Wait); // if planning is already in progress, finish it
}

void AIBrain::OnStepTimedOut()
{
  if (_state == Stopping)
  {
    // Get out/land/get in cannot be processed
    Transport *veh = dyn_cast<Transport>(GetVehicle());
    Assert(veh);
    if (veh)
    {
      // if anyone wanted to get out, we cancel the get out order
      // we cancel the stopping procedure if possible
      if (veh->CanCancelStop())
      {
        AIBrain *commander = veh->CommanderUnit();
        if (commander) commander->SetState(Wait); // valid pass Stopped -> Wait
      }
    }
    else
    {
      Verify(SetState(Wait)); // TODO: BUG
    }
  }
  else if (_state == AIBrain::Busy)
  {
    // in analogy to OnStepCompleted, we want to avoid replanning for formation members
    Verify(SetState(Replan));
  }
}

void AIBrain::OnStepCompleted()
{
  if (_state == Stopping)
  {
    SetState(Stopped);
    Transport *veh = dyn_cast<Transport>(GetVehicle());
    if (veh)
    {
      veh->LandFinished();
    }
  }
  else if (_state == AIBrain::Busy)
  {
    // Direct pilot cannot complete path planned for command
    Assert (_planningMode != LeaderDirect);
    if (_lastPlan)
    {
      EntityAI *veh = GetVehicle();
      float precision = veh->GetPrecision();
      if (_wantedPosition.Distance(_plannedPosition) > Square(precision))
      {
        ForceReplan();
      }
      else
      {
        Vector3 pos = _path[_path.Size() - 1]._pos;
        if (pos.DistanceXZ2(_expPosition) > Square(1))
        {
          // try to find better path
          _iter = 0;
          Verify(SetState(Wait));
        }
        else
        {
          Verify(SetState(Completed));
        }
      }
    }
    else
    {
      // even for formation movement we need to replan when the plan did not lead into the destination
      // _plannedPosition will prevent issuing a new planning to the same position, but the unit will never get there
      Verify(SetState(Replan));
    }
  }
}

bool AIBrain::IsFireEnabled(Target *tgt) const
{
  // check if fire enabled at given target
  if (!IsHoldingFire()) return true;
  if (tgt && tgt==GetEnableFireTarget()) return true;
  return false;
}

void AIBrain::Disclose(DangerCause cause, Vector3Par position, EntityAI *causedBy, bool discloseGroup)
{
  // no effect on player group
  if (IsPlayer()) return;
  if (!IsLocal()) return;
  // dead person can no longer disclose
  if (GetPerson() && GetPerson()->IsDamageDestroyed()) return;
  
  const TargetList *tgtList = AccessTargetList();
  const Target *tgt = tgtList ? tgtList->FindTarget(causedBy) : NULL;
  SetDanger(cause, position, tgt, GRandGen.PlusMinus(1.0f, 0.2f));
}

/// "interest" value relative to given unit
float HowMuchInteresting(AIBrain *unit, const Target *tgt)
{
  EntityAI *veh = unit->GetVehicle();
  Vector3Val unitDir = veh->FutureVisualState().Direction();
  Vector3Val unitPos = veh->FutureVisualState().Position();
  float interesting = tgt->HowMuchInteresting();
  // we get some points when unknown
  TargetSide side = tgt->GetSide();
  if (side==TSideUnknown) interesting += 10;
  // we get many points for being enemy
  if (unit->IsEnemy(side)) interesting += 50;
  // we loose a lot for being out of natural focus area
  Vector3 tgtDir = tgt->GetPosition()-unitPos;
  float cosAlpha = tgtDir.CosAngle(unitDir);
  float focus = floatMax(cosAlpha,0.2);
  return interesting * focus;
}

Target *SelectInterestingTarget(AIBrain *unit, Target *thn)
{
  PROFILE_SCOPE_EX(aiSIT,ai);
  Vector3Val uPos = unit->Position(unit->GetFutureVisualState());

  // find something known that is near
  const TargetList *list = unit->AccessTargetList();
  if (!list) return NULL;
  // never select target less interesting than certain limit
  float thnInteresting = 20;
  float thnDist = 50;
  if (thn)
  {
    // increase old target interest to maintain some stability
    thnInteresting = HowMuchInteresting(unit,thn)*3.0f;
    thnDist = uPos.Distance(thn->GetPosition());
  }

  // scan enemies - we are interested only in a few most important enemy targets
  {
    int maxTargets = 6;
    for (int i=0; i<list->EnemyCount(); i++)
    {
      TargetNormal *tgt = list->GetEnemy(i);
      if (tgt->idExact && tgt->idExact->Static()) continue;
      if (tgt->idExact==unit->GetPerson()) continue;
      if (tgt->idExact==unit->GetVehicleIn()) continue;
      if (!tgt->IsKnownBy(unit)) continue;
      if (tgt->type->IsSlow()) continue; // static object or inventory item
      // check if we are allowed to check more targets
      if (tgt->vanished) continue;
      if (--maxTargets<0) break;

      float tgtDist = tgt->position.Distance(uPos);
      float tgtInteresting = HowMuchInteresting(unit,tgt);
      if (tgtInteresting*thnDist>thnInteresting*tgtDist)
      {
        thn = tgt;
        thnDist = tgtDist;
        thnInteresting = tgtInteresting;
      }
    }
  }

  //TODO: friendly side is not used anymore. Remove all relevant code
/*  if (unit->GetCombatMode()<CMCombat)
  {
    // not combat - we are interested only in a few most important friendly targets
    // we want to track more targets in cut-scenes, for more reliable behaviour
    int maxTargets = GWorld->CheckCutscene() ? 20 : 10;
    for (int i=0; i<list->FriendlyCount(); i++)
    {
      TargetNormal *tgt = list->GetFriendly(i);
      if (tgt->idExact && tgt->idExact->Static()) continue;
      if (tgt->idExact==unit->GetPerson()) continue;
      if (tgt->idExact==unit->GetVehicleIn()) continue;
      if (!tgt->IsKnownBy(unit)) continue;
      if (tgt->vanished) continue;
      if (tgt->type->IsSlow()) continue; // static object or inventory item
      // check if we are allowed to check more targets
      if (--maxTargets<0) break;

      float tgtDist = tgt->position.Distance(uPos);
      float tgtInteresting = HowMuchInteresting(unit,tgt);
      if (tgtInteresting*thnDist>thnInteresting*tgtDist)
      {
        thn = tgt;
        thnDist = tgtDist;
        thnInteresting = tgtInteresting;
      }
    }
  }*/
  return thn;
}

Vector3 AIBrain::ScanTargetOffset(const Target * tgt) const
{
  if (IsAnyPlayer())
  {
    // for a player a different visualization is needed
    return VZero;
  }
  float accuracy = tgt->PositionAccuracy(this);
  // TODO: limit might be FOV based here
  if (accuracy>5.0f)
  {
    // perform time based area scan
    // circle around the area, this converted to 1D
    // should give an impression of sweeping left and right

    const int duration = 31000;
    float dir = Glob.time.ModMs(duration)*(1.0f/duration);

    // per unit randomization
    dir += (RandomSeed()&0xf)*(1.0f/15);

    // convert number to angle (note: some approximation could work well)
    float x = sin(dir*H_PI*2);
    float z = cos(dir*H_PI*2);

    // consider: we may want follow terrain with the eye target
    return Vector3(x*accuracy,0,z*accuracy);
  }
  return VZero;
}

/*!
  \patch 5092 Date 11/29/2006 by Ondra
  - Fixed: AI more actively scanning visually for enemy when his position is not known.
*/
Vector3 AIBrain::ScanTarget(Target * tgt)
{
  return tgt->LandAimingPosition(this)+ScanTargetOffset(tgt);
}

void AIBrain::WatchTarget()
{
  _watchPos = ScanTarget(_watchTgt);

  // we assume we watch from the head (i.e. eye) of the target
  _watchDir = _watchPos - GetVehicle()->EyePosition(GetVehicle()->FutureVisualState());


  _watchDir.Normalize();
  _watchDirHead = _watchDir;
}

void AIBrain::WatchFormation()
{
  _watchDirHead = _watchDir = Direction(GetFutureVisualState());
}

void AIBrain::GlanceRandomDirection()
{
  // we need to set watch direction as well (similar to GlanceAt)
  // we know there is no target, otherwise GlanceAt would be called
  // therefore we can always call WatchFormation
  WatchFormation();

  if ((_disabledAI&DAAutoTarget)==0)
  {
    // we base our direction on time, so that it is continuous
    const int duration = 20000;
    float dir = Glob.time.ModMs(duration)*(1.0f/duration);

    // add some randomization, so that all units do not perform the same movement
    dir += (RandomSeed()&0xf)*(1.0f/15);

    dir = fabs(dir-0.5f)*2;

    Vector3 relDir(((dir-0.5f))*3,0,1);
    relDir.Normalize();
    _watchDirHead = GetVehicle()->FutureVisualState().DirectionModelToWorld(relDir);
  }
  else
  {
    _watchDirHead = GetVehicle()->FutureVisualState().Direction();
  }
}

void AIBrain::GlanceAt(Target *tgt)
{
  if (_watchTgt)
  {
    WatchTarget();
  }
  else
  {
    WatchFormation();
  }
  // if auto targeting is disabled, we must not be curious
  // mission designer has full control over us

  if (tgt)
  {
    // we see something interesting - watch it
    _glanceTgt = tgt;
    // if target position is not accurate, scan the area where the target could be

    _glancePos = ScanTarget(_glanceTgt);
    if (tgt->idExact)
    {
      // make correction for different aiming/camera position
      if (_glancePos.Distance2(tgt->idExact->AimingPosition(tgt->idExact->FutureVisualState()))<0.5f)
      {
        _glancePos = tgt->idExact->EyePosition(tgt->idExact->FutureVisualState());
      }
    }
    _watchDirHead = _glancePos - GetVehicle()->EyePosition(GetVehicle()->FutureVisualState());
    _watchDirHead.Normalize();
  }
}

/*!
\patch 1.52 Date 4/20/2002 by Ondra
- New: All men are now interested in what is happening around them.
*/

void AIBrain::SetWatch()
{
  // consider formation and watch mode
  if (!IsPlayer() && !GetPerson()->Invisible())
  {
    // we might have started watching before getting in
    if (_glanceTgt.IdExact()==GetVehicle()) _glanceTgt = NULL;
    if (_watchTgt.IdExact()==GetVehicle()) _watchTgt = NULL;
    if (_glanceTgt.IdExact() && !_glanceTgt.IdExact()->IsInLandscape()) _glanceTgt = NULL;
    if (_watchTgt.IdExact() && !_watchTgt.IdExact()->IsInLandscape()) _watchTgt = NULL;

    switch (_watchMode)
    {
    case WMAround:
      {
        float time = Glob.time - _watchDirSet;
        Matrix3 rotY(MRotationY, time *((H_PI*2)/30));
        _watchDirHead = rotY * _watchDirHead;
        _watchDirHead[1] = 0; // TODO: adapt to terrain around us
        _watchDirHead.Normalize();
        _watchDirSet = Glob.time;
        // change body direction in large steps to prevent small movements which look ugly
        const float cos60 = 0.5f;
        if (_watchDir.DotProduct(_watchDirHead)<cos60 || !IsFreeSoldier())
        {
          _watchDir = _watchDirHead;
        }
      }
      break;
    case WMPos:
      _watchDir = _watchPos - GetVehicle()->EyePosition(GetVehicle()->FutureVisualState());
      _watchDir.Normalize();
      _watchDirHead = _watchDir;
      break;
    case WMPosGlance:
      if (_watchTgt)
      {
        WatchTarget();
      }
      else
      {
        WatchFormation();
      }
      _watchDirHead = _glancePos - GetVehicle()->EyePosition(GetVehicle()->FutureVisualState());
      _watchDirHead.Normalize();
      break;
    case WMTgt:
      if (_watchTgt)
      {
        WatchTarget();
        break;
      }

      // fall trough from case WMTgt to case WMNo
    case WMNo:
      {
        // if auto targeting is disabled, we must not be curious
        // mission designer has full control over us
        Target *tgt = NULL;
        if ((_disabledAI&DAAutoTarget)==0)
        {
          // when soldier is idle, automatically select "interesting" targets
          tgt = _glanceTgt;
          if (!tgt) tgt = _watchTgt;
          tgt = SelectInterestingTarget(this, tgt);
        }
        if (tgt)
        {
          GlanceAt(tgt);
        }
        else
        {
          GlanceRandomDirection();
        }
        break;
      }
    case WMTgtGlance:
      GlanceAt(_glanceTgt);
      break;
    }
  }
}

#if _ENABLE_CHEATS

void AIBrain::DebugFSM()
{
#if DEBUG_FSM
  for (int i=0; i<NBehaviourFSMRole; i++)
  {
    if (_behaviourFSMs[i]) _behaviourFSMs[i]->Debug();
  }
  if (_reactionFSM) _reactionFSM->Debug();

# if _ENABLE_CONVERSATION
    if (_conversationFSM) _conversationFSM->Debug();
# endif

#endif
}

void AIBrain::DebugFSM(int level)
{
#if DEBUG_FSM
  _fsmAutoDebug = level;
#endif
}


#endif

LSError AIBrain::Serialize(ParamArchive &ar)
{
  if (ar.IsSaving())
  {
    RString type = GetAgentType();
    CHECK(ar.Serialize("agentType", type, 1))
  }

  // structure
  if (ar.IsSaving())
  {
    CHECK(ar.SerializeRef("Person", _person, 1))
  }
  else if (ar.GetPass() == ParamArchive::PassSecond)
  {
    OLinkPermO(Person) person;
    CHECK(ar.SerializeRef("Person", person, 1))
    SetPerson(person);
  }
  CHECK(ar.SerializeRef("InVehicle", _inVehicle, 1))

  // info
  //  CHECK(ar.Serialize("speaker", _speaker, 1, -1))
  if (ar.IsSaving())
  {
    if (_person)
      CHECK(ar.Serialize("Info", _person->GetInfo(), 1))
  }
  else if (ar.GetPass() == ParamArchive::PassSecond)
  {
    if (_person)
    {
      AIUnitInfo &info = _person->GetInfo();
      ar.FirstPass();
      CHECK(ar.Serialize("Info", info, 1))
      ar.SecondPass();
      CHECK(ar.Serialize("Info", info, 1))

      if (info._face.GetLength() > 0)
        _person->SetFace(info._face);

      if (info._glasses.GetLength() > 0)
        _person->SetGlasses(info._glasses);

      if (info._speaker.GetLength() > 0)
        SetSpeaker(info._speaker, info._pitch);
    }
  }
#if !_ENABLE_IDENTITIES
  CHECK(ar.Serialize("ability", _ability, 1, 0.2))
#endif

  CHECK(ar.SerializeEnum("semaphore", _semaphore, 1, SemaphoreYellow))
  CHECK(ar.SerializeEnum("combatModeMajor", _combatModeMajor, 1, CMAware))
  CHECK(::Serialize(ar, "dangerUntil", _dangerUntil, 1, Time(0)))
  CHECK(ar.Serialize("captive", _captive, 1, 0))

  CHECK(ar.SerializeEnum("lifeState", _lifeState, 1, LifeStateAlive))
  CHECK(ar.Serialize("disabledAI", _disabledAI, 1, 0));

  CHECK(ar.SerializeRef("TargetAssigned", _targetAssigned._tgt, 1))
  CHECK(ar.SerializeEnum("TargetAssignedState", _targetAssigned._initState, 1, TargetFireMin))
  CHECK(ar.SerializeRef("TargetEnableFire", _targetEnableFire, 1))

  CHECK(ar.Serialize("nearestEnemyDist2", _nearestEnemyDist2, 1))

  // actual instructions
  CHECK(ar.Serialize("Path", _path, 1))
  CHECK(ar.SerializeRef("House", _house, 1))
  CHECK(ar.Serialize("housePos", _housePos, 1, -1))
  CHECK(ar.Serialize("pathId", _pathId, 1, 0))

  CHECK(::Serialize(ar, "wantedPositionWanted", safe_cast<Vector3 &>(_wantedPositionWanted), 1, VUndefined))
  CHECK(::Serialize(ar, "wantedPositionWantedCover", _wantedPositionWanted._cover, 1, false))
  CHECK(ar.Serialize("wantedPrecision", _wantedPrecision, 1, 0.0f))
  CHECK(ar.SerializeEnum("planningModeWanted", _planningModeWanted, 1, DoNotPlan))
  CHECK(ar.Serialize("forceReplanWanted", _forceReplanWanted, 1, false))

  CHECK(::Serialize(ar, "wantedPosition", safe_cast<Vector3 &>(_wantedPosition), 1, VUndefined))
  CHECK(::Serialize(ar, "wantedPositionCover", _wantedPosition._cover, 1, false))
  CHECK(::Serialize(ar, "plannedPosition", safe_cast<Vector3 &>(_plannedPosition), 1, VUndefined))
  CHECK(::Serialize(ar, "plannedPositionCover", _plannedPosition._cover, 1, false))
  CHECK(ar.SerializeEnum("planningMode", _planningMode, 1, DoNotPlan))
  CHECK(ar.Serialize("completedReceived", _completedReceived, 1, false))

  CHECK(::Serialize(ar, "watchDirection", _watchDir, 1, VZero))
  CHECK(::Serialize(ar, "watchDirectionHead", _watchDirHead, 1, VZero))
  CHECK(::Serialize(ar, "watchPosition", _watchPos, 1, VZero))
  CHECK(::Serialize(ar, "glancePosition", _glancePos, 1, VZero))
  CHECK(ar.SerializeRef("watchTarget", _watchTgt, 1))
  CHECK(ar.SerializeRef("glanceTarget", _glanceTgt, 1))
  CHECK(ar.SerializeEnum("watchMode", _watchMode, 1, WMNo))
  

  CHECK(::Serialize(ar, "expPosition", safe_cast<Vector3 &>(_expPosition), 1))
  CHECK(::Serialize(ar, "expPositionCover", _expPosition._cover, 1, false))

  CHECK(ar.SerializeEnum("state", _state, 1, Busy))
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    if (_state == Planning)
    {
      // working space for pathfinding is not saved
      _state = Init;
    }
  }
  CHECK(ar.SerializeEnum("mode", _mode, 1, Normal))

  // counter for trying of finding path
  CHECK(::Serialize(ar, "delay", _delay, 1))
  CHECK(ar.Serialize("iter", _iter, 1, 0))

  // strategic plan
  if (ar.GetArVersion() >= 8)
  {
#if _ENABLE_AI
    CHECK(ar.Serialize("Planner", *_planner, 1))
    CHECK(ar.Serialize("completedTime", _completedTime, 1)) // not used - why ??
    CHECK(::Serialize(ar, "waitWithPlan", _waitWithPlan, 1))
    CHECK(ar.Serialize("attemptPlan", _attemptPlan, 1, 0))
#endif

    CHECK(ar.Serialize("lastPlan", _lastPlan, 1, false))
    CHECK(ar.Serialize("noPath", _noPath, 1, false))
    CHECK(ar.Serialize("updatePath", _updatePath, 1, false))
  }

#if _ENABLE_CONVERSATION
  CHECK(ar.Serialize("kbCenter", _kbCenter, 1))
  CHECK(ar.SerializeRef("conversingWith", _conversingWith, 1))
#endif
  // behavior FSMs
  if (ar.IsSaving())
  {
    for (int i=0; i<NBehaviourFSMRole; i++)
    {
      FSM *fsm = _behaviourFSMs[i];
      if (!fsm) continue;

      RString name = RString("FSM") + FindEnumName((BehaviourFSMRole)i);
      // FSM should already exist
      CHECK(ar.Serialize(name, *fsm, 1))
    }

  }
  else if (ar.GetPass() == ParamArchive::PassSecond)
  {
    // _person is valid now, we can serialize FSMs
    ar.FirstPass();
    for (int i=0; i<NBehaviourFSMRole; i++)
    {
      FSM *fsm = _behaviourFSMs[i];
      if (!fsm) continue;

      RString name = RString("FSM") + FindEnumName((BehaviourFSMRole)i);
      // FSM should already exist
      CHECK(ar.Serialize(name, *fsm, 1))
    }

    ar.SecondPass();
    for (int i=0; i<NBehaviourFSMRole; i++)
    {
      FSM *fsm = _behaviourFSMs[i];
      if (!fsm) continue;

      RString name = RString("FSM") + FindEnumName((BehaviourFSMRole)i);
      // FSM should already exist
      CHECK(ar.Serialize(name, *fsm, 1))
    }
  }

  // serialize also short-term reaction FSM
  // TODO: serialization of EntityFSM, not only ScriptedFSM
  CHECK(ar.Serialize("reactionFSM",_reactionFSM, 1))
#if _ENABLE_CONVERSATION
  CHECK(ar.Serialize("conversationFSM",_conversationFSM, 1))

  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassSecond)
  {
    // avoid talking to other person when reacting
    if (_conversationFSM) ChangeListening(+1);
  }

// _talkEvents
  KBSaveContext context(this, &GGameState);
  void *oldParams = ar.SetParams(&context);
  CHECK(ar.Serialize("talkEvents",_talkEvents, 1));
  ar.SetParams(oldParams);
#endif

  CHECK(ar.Serialize("ghostTarget",_ghostTarget,1));

  return LSOK;
}

#if _ENABLE_CONVERSATION
LSError KBMessageInfo::Serialize(ParamArchive &ar)
{
  if (ar.IsSaving())
  {
    CHECK(ar.Serialize("topic", _topic,1))

    const KBMessageTemplate *type = _message->GetType();
    RString sentenceId = type->GetName();
    CHECK(ar.Serialize("sentenceId", sentenceId, 1))

    // during serialization of GameValue, GameState* is expected as a context
    KBSaveContext *context = (KBSaveContext *)ar.GetParams();
    ar.SetParams(context->_gameState);
    CHECK(ar.Serialize("Arguments", unconst_cast(_message.GetRef())->GetArguments(), 1))
    ar.SetParams(context);
  }
  else if (ar.GetPass()==ParamArchive::PassSecond)
  {
    // we cannot load message content until the Person is known, which is in the 2nd pass
    // note: this may somehow limit the possibilities of what is contained in the message
    // we need to pretend we are in the first pass, otherwise the Serialize of RString would do nothing
    ar.FirstPass();
    RString sentenceId;
    CHECK (ar.Serialize("sentenceId", sentenceId, 1));

    // get topic based on current brain
    KBSaveContext *context = (KBSaveContext *)ar.GetParams();
    const KBCenter *kbCenter = context->_sender->GetKBCenter();
    if (!kbCenter)
    {
      RptF("No KB found for %s",cc_cast(context->_sender->GetDebugName()));
      return LSStructure;
    }
    const KBCenter *kbParent = context->_sender->GetKBParent();
    KBTopic *kbTopic = kbCenter->GetTopic(_topic,kbParent);
    if (!kbTopic)
    {
      RptF("Topic %s not found",cc_cast(_topic));
      return LSStructure;
    }
    const KBMessageTemplate *type = kbTopic->FindMessageTemplate(sentenceId);
    if (!type)
    {
      RptF("Message template %s not found in %s",cc_cast(sentenceId),cc_cast(_topic));
      return LSStructure;
    }
    _message = new KBMessage(type);

    // during serialization of GameValue, GameState* is expected as a context
    ar.SetParams(context->_gameState);

    // serialize both passes
    CHECK(ar.Serialize("Arguments", unconst_cast(_message.GetRef())->GetArguments(), 1))
    ar.SecondPass();
    CHECK(ar.Serialize("Arguments", unconst_cast(_message.GetRef())->GetArguments(), 1))

    ar.SetParams(context);
  }
  return LSOK;
}
KBMessageInfo *KBMessageInfo::CreateObject(ParamArchive &ar)
{
  RString topic;
  if (ar.Serialize("topic",topic,1)!=LSOK) return NULL;
  // create an empty info, will be filled in the 2nd pass
  return new KBMessageInfo(topic, NULL);
}

LSError AIBrain::TalkEvent::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeRef("from",_from,1));
  // more verbose, but cleaner than (Ref<KBMessageInfo> &)_event
  Ref<KBMessageInfo> event = unconst_cast(_event.GetRef());
  CHECK(ar.Serialize("event",event,1));
  _event = event;
  return LSOK;
}
#endif

AIBrain *AIBrain::CreateObject(ParamArchive &ar)
{
  RString type;
  if (ar.Serialize("agentType", type, 1, "Unit") != LSOK) return NULL;
#if _ENABLE_INDEPENDENT_AGENTS
  if (stricmp(type, "Agent") == 0) return new AIAgent(NULL);
#endif

  ErrF("Unknown agent type: %s", cc_cast(type));
  return NULL;
}

AIBrain *AIBrain::LoadRef(ParamArchive &ar)
{
  // get it from person
  Object *obj = Person::LoadRef(ar);
  Person *person = dyn_cast<Person>(obj);
  if (!person) return NULL;
  return person->Brain();
}

LSError AIBrain::SaveRef(ParamArchive &ar) const
{
  if (_person)
  {
    CHECK(_person->SaveRef(ar))
  }
  else
  {
    // save the NULL object
    ObjectId id;
    CHECK(id.Serialize(ar, "oid", 1))
  }
  return LSOK;
}


#if _ENABLE_CONVERSATION

void AIBrain::AddKBTopic(RString name, RString filename, RString taskType, GameValuePar eventHandler)
{
  if (!_kbCenter) _kbCenter = new KBCenter();
  KBTopic *topic = new KBTopicExt();
  topic->Init(name, filename, taskType, eventHandler);
  _kbCenter->AddTopic(topic);
}

void AIBrain::RemoveKBTopic(RString name)
{
  if (_kbCenter) _kbCenter->RemoveTopic(name);
}

bool AIBrain::HasKBTopic(RString name)
{
  const KBCenter *kbCenter = GetKBCenter();
  if (!kbCenter) return false;
  const KBCenter *kbParent = GetKBParent();
  return kbCenter->GetTopic(name,kbParent) != NULL;
}

KBMessageInfo *AIBrain::CreateKBMessage(RString topic, GameValuePar sentence)
{
  const KBCenter *kbCenter = GetKBCenter();
  if (!kbCenter) return NULL;
  const KBCenter *kbParent = GetKBParent();
  return kbCenter->CreateMessage(topic, sentence,kbParent);
}

void AIBrain::SetKBMessageArgument(KBMessageInfo *message, GameValuePar argument)
{
  const KBCenter *kbCenter = GetKBCenter();
  if (kbCenter) kbCenter->SetMessageArgument(message, argument);
}

void AIBrain::KBTell(AIBrain *to, KBMessageInfo *message, bool forceRadio, ChatChannel chan)
{
  if (!IsLocal())
  {
    RptF("Conversation - need to be initiated by the speaker owner");
    return;
  }

  // replace the special words
  const KBCenter *kbCenter = GetKBCenter();
  const KBCenter *kbParent = GetKBParent();
  if (kbCenter) kbCenter->CheckMessage(message, kbParent);
  void KBTell(AIBrain *askingUnit, AIBrain *askedUnit, const KBMessageInfo *info, bool forceRadio, ChatChannel chan);
  KBTell(this, to, message, forceRadio, chan);
}

void AIBrain::KBReact(AIBrain *sender, const KBMessageInfo *question)
{
  if (!IsLocal())
  {
    RptF("Conversation - reacting person need to be local");
    return;
  }

  // TODO: handle multiple agents we may be conversing with at the same time
  _conversingWith = sender;

  if (IsPlayer())
  {
    // Start conversation UI
    void ProcessConversation(AIBrain *askingUnit, AIBrain *askedUnit, const KBMessageInfo *question);
    ProcessConversation(this, sender, question);
  }
  else
  {
    const KBCenter *kbCenter = GetKBCenter();
    if (kbCenter)
    {
      if (!question) return;
      RString topic = question->GetTopic();
      if (topic.GetLength() == 0) return;
      #ifdef GetMessage
      #undef GetMessage
      #endif
      const KBMessage *message = question->GetMessage();
      if (!message) return;
      const KBMessageTemplate *messageType = message->GetType();
      if (!messageType) return;

      const KBCenter *kbParent = GetKBParent();
      RString handler = kbCenter->GetHandlerAI(question, kbParent);
      if (handler.GetLength() == 0) return;

  #if _ENABLE_INDEPENDENT_AGENTS
      FSMScripted *fsm = NULL;

      AITeamMember *teamMember = GetTeamMember();
      if (teamMember)
      {
        const AITaskType *taskType = teamMember->GetTaskType(handler);
        if (taskType)
        {
          // Task creation
          fsm = CreateTeamMemberFSMScripted(taskType->GetFSMType(), teamMember->GetVars());
          const float priority = 100.0f;
          AITask *task = new AITask(teamMember, teamMember, NULL, taskType, fsm, priority);
          teamMember->AddTask(task);
        }
      }
      else
  #endif
      {
        TalkEvent &ev = _talkEvents.Append();
        ev._from = sender;
        ev._event = question;
      }
    }
  }
}

const KBCenter *AIBrain::GetKBParent() const
{
  // if there is no explicit primary, type is primary and there is no parent (secondary)
  if (!_kbCenter) return NULL;
  // if there was primary, return based on type
  if (!_person) return NULL;
  const EntityAIType *type = _person->GetType();
  return type->GetKBCenter();
}

const KBCenter *AIBrain::GetKBCenter() const
{
  if (_kbCenter) return _kbCenter;
  // no explicit primary - check type
  if (!_person) return NULL;
  const EntityAIType *type = _person->GetType();
  return type->GetKBCenter();
}

ConversationContext::ConversationContext(AIBrain *askingUnit, AIBrain *askedUnit, const KBMessageInfo *question)
{
  _askingUnit = askingUnit;
  _askedUnit = askedUnit;
  _question = question;

  if (_askingUnit) _askingUnit->ChangeSpeaking(+1);
  if (_askedUnit) _askedUnit->ChangeListening(+1);

  _center = _askingUnit->GetKBCenter();
  const KBCenter *kbParent = _askingUnit->GetKBParent();
  KBCenter::ParentList list(kbParent);

  // decide if old (class UI) or new (event handlers) method will be used
  bool someHandler = false;
  if (question)
  {
    _topic = question->GetTopic();

    // ask the received topic for the reaction
    GameValue handler = _center->GetHandlerPlayer(question, list);
    if (!handler.GetNil())
    {
      someHandler = true;
      _handlerResult = ExecuteHandler(handler, question->GetTopic(), question->GetMessage());
      if (_handlerResult.GetNil()) _handlerResult = GGameState.CreateGameValue(GameArray);
    }
  }
  else
  {
    // collect reactions
    int topics = list.TopicCount(_center);
    for (int i=0; i<topics; i++)
    {
      const KBTopic *topic = list.GetTopic(_center, i);
      const GameValue &handler = topic->GetHandlerPlayer();
      if (!handler.GetNil())
      {
        if (!someHandler)
        {
          // first handler found, initialization
          someHandler = true;
          _handlerResult = GGameState.CreateGameValue(GameArray);
        }
        GameValue result = ExecuteHandler(handler, topic->GetName(), NULL);
        if (!result.GetNil())
        {
          const GameArrayType &srcArray = result;
          GameArrayType &dstArray = _handlerResult;
          for (int j=0; j<srcArray.Size(); j++) dstArray.Add(srcArray[j]);
        }
      }
    }
  }

  if (!someHandler) _current = _center->GetUIRoot(question, list);
}

ConversationContext::~ConversationContext()
{
  if (_askingUnit) _askingUnit->ChangeSpeaking(-1);
  if (_askedUnit) _askedUnit->ChangeListening(-1);
}

GameValue ConversationContext::ExecuteHandler(GameValuePar handler, RString topic, const KBMessage *message)
{
  PROFILE_SCOPE_EX(cnvEH, *);
  if (PROFILE_SCOPE_NAME(cnvEH).IsActive())
  {
    PROFILE_SCOPE_NAME(cnvEH).AddMoreInfo(Format("%s->%s:%s",
      _askingUnit ? cc_cast(_askingUnit->GetDebugName()) : "null", 
      _askedUnit ? cc_cast(_askedUnit->GetDebugName()) : "null",
      cc_cast(topic)));
  }

  GameVarSpace local(false);
  int gshandle = GGameState.BeginContext(&local);

  // attention, all variable names must be in lower case

  // player himself
  Person *me = _askingUnit ? _askingUnit->GetPerson() : NULL;
  local.VarLocal("_this");
  local.VarSet("_this", GameValueExt(me));
  // person we are reacting to
  Person *you = _askedUnit ? _askedUnit->GetPerson() : NULL;
  local.VarLocal("_from");
  local.VarSet("_from", GameValueExt(you));
  // conversation topic
  local.VarLocal("_topic");
  local.VarSet("_topic", GameValue(topic));
  // sentence
  RString sentenceId;
  if (message)
  {
    const KBMessageTemplate *messageType = message->GetType();
    if (messageType) sentenceId = messageType->GetName();
  }
  local.VarLocal("_sentenceid");
  local.VarSet("_sentenceid", GameValue(sentenceId));
  // sentence arguments
  if (message)
  {
    for (int i=0; i<message->NArguments(); i++)
    {
      const KBArgument &argument = message->GetArgument(i);
      RString name = RString("_") + argument._name;
      name.Lower();
      local.VarLocal(name);
      local.VarSet(name, argument._value);
    }
  }

  // execute the handler
  GameValue result;
#if USE_PRECOMPILATION
  if (handler.GetType() == GameCode)
  {
    GameDataCode *code = static_cast<GameDataCode *>(handler.GetData());
    if (code->IsCompiled() && code->GetCode().Size() > 0)
      result = GGameState.Evaluate(code->GetString(), code->GetCode(), GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
  }
  else
#endif
  if (handler.GetType() == GameString)
  {
    // make sure string is not destructed while being evaluated
    RString code = handler;
    if (code.GetLength() > 0)
      result = GGameState.EvaluateMultiple(code, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
  }

  GGameState.EndContext(gshandle);

  // return the result (nil when empty or unexpected)
  if (result.GetNil()) return GameValue();
  if (result.GetType() != GameArray) return GameValue();
  const GameArrayType &array = result;
  if (array.Size() == 0) return GameValue();
  return result;
}

#endif // _ENABLE_CONVERSATION


#if _ENABLE_DIRECT_MESSAGES

bool AIBrain::IsGestureEnabled(ChatChannel channel) const
{
  switch (_gestureEnabled)
  {
  case GEDisabled:
    return false;
  case GEEnabled:
  default:
    return true;
  }
}

bool AIBrain::IsVoiceEnabled(ChatChannel channel) const
{
  switch (_voiceEnabled)
  {
  case VEDisabled:
    return false;
  case GEEnabled:
    return true;
  default:
    return GetCombatMode() != CMStealth;
  }
}

#endif

float AIBrain::GetSERank() const
{
  Person *person = GetPerson();
  if (!person) return 0;
  AIUnitInfo &info = person->GetInfo();
  // 0 for RankPrivate, 1 for max. rank
  return (float)info._rank / (NRanks - 1);
}

float AIBrain::GetSECaptive() const
{
  return _captive != 0 ? 1.0f : 0.0f;
}

float AIBrain::GetSESafe() const
{
  return GetCombatMode() == CMSafe ? 1.0f : 0.0f;
}

float AIBrain::GetSECombat() const
{
  return GetCombatMode() == CMCombat ? 1.0f : 0.0f;
}

float AIBrain::GetSEStealth() const
{
  return GetCombatMode() == CMStealth ? 1.0f : 0.0f;
}

float AIBrain::GetSEMorale() const
{
  // interpolate from interval <-1, 1> to <0, 1>
  float morale = 0.5f * (GetMorale() + 1.0f);
  saturate(morale, 0.0f, 1.0f);
  return morale;
}

void AIBrain::AddExp(float exp)
{
  Person *person = GetPerson();
  AIUnitInfo &info = person->GetInfo();
  float limit = floatMin(info._experience, 0);
  info._experience += exp;
  if (person != GLOB_WORLD->GetRealPlayer())
    saturateMax(info._experience, limit);
}

void AIBrain::IncreaseExperience(const VehicleType& type, TargetSide side)
{
  if (type.IsAnimal()) return;

  float coef;

  if (type.IsSlow()) // static object or inventory item
  {
    coef = ExperienceDestroyStatic;
  }
  else
  {
    if (side == TCivilian)
      coef = ExperienceDestroyCivilian;
    else if (IsEnemy(side) || IsEnemy(type.GetTypicalSide()))
      coef = ExperienceDestroyEnemy;
    else
      coef = ExperienceDestroyFriendly;
  }

  if (!IsPlayer() && coef <= 0) return;

  int i, n = ExperienceDestroyTable.Size();
  float base = 0;
  float cost = type.GetCost();
  if (cost > ExperienceDestroyTable[n - 2].maxCost)
  {
    base = ExperienceDestroyTable[n - 1].exp;
  }
  else
    for (i=0; i<n-1; i++)
    {
      if (cost <= ExperienceDestroyTable[i].maxCost)
      {
        base = ExperienceDestroyTable[i].exp;
        break;
      }
    }

  AddExp(coef * base);
}

#if _ENABLE_IDENTITIES

LSError AITaskType::Serialize(ParamArchive &ar)
{
  if (ar.IsSaving())
  {
    CHECK(ar.Serialize("name", _name, 1))
  }
  CHECK(ar.Serialize("instancesCreated", _instancesCreated, 1, 0))
  CHECK(ar.Serialize("instancesDeleted", _instancesDeleted, 1, 0))
  return LSOK;
}

AITaskType *AITaskType::CreateObject(ParamArchive &ar)
{
  AITaskTypeName name;
  if (ar.Serialize("name", name, 1) != LSOK) return NULL;
  return AITaskTypes.New(name);
}

AITaskTypeInfo::AITaskTypeInfo(AITaskType *type)
{
  _type = type;
  _created = 0;
}

LSError AITaskTypeInfo::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("type", _type, 1));
  CHECK(ar.Serialize("created", _created, 1, 0));
  return LSOK;
}

#include "fsmScripted.hpp"

static ConstParamEntryPtr FindParamEntry(RString parent, RString name)
{
  const ParamFile *path[] = {&ExtParsMission, &Pars};
  for (int i=0; i<lenof(path); i++)
  {
    ConstParamEntryPtr cls = path[i]->FindEntry(parent);
    if (cls)
    {
      ConstParamEntryPtr entry = cls->FindEntry(name);
      if (entry) return entry;
    }
  }
  WarningMessage("Class %s.%s not found", cc_cast(parent), cc_cast(name));
  return ConstParamEntryPtr();
}

AITaskType::AITaskType(const AITaskTypeName &name)
{
  _instancesCreated = 0;
  _instancesDeleted = 0;

  _name = name;
  ConstParamEntryPtr cls = FindParamEntry("CfgTasks", name);
  _parClass = cls.GetClass();
  if (cls)
  {
    RString FindScript(RString name);

    const ParamEntry &par = *cls;
    _typeName = par >> "name";
    RString fsmFilename = FindScript(par >> "fsm");
    if (fsmFilename.GetLength() > 0)
    {
      _fsmType = FSMScriptedTypes.New(fsmFilename);
    }
    RString conditionFilename = par >> "condition";
    if (conditionFilename.GetLength() > 0)
    {
      conditionFilename = FindScript(conditionFilename);
      if (conditionFilename.GetLength() > 0)
      {
        FilePreprocessor preproc(true);
        QOStrStream processed;
        if (preproc.Process(&processed, conditionFilename))
        {
          _condition = RString(processed.str(), processed.pcount());
#if USE_PRECOMPILATION
          GameState *state = GWorld->GetGameState();
          GameVarSpace local(state->GetContext(), false);
          int gshandle = state->BeginContext(&local);
          SourceDoc doc; doc._content = _condition;
          SourceDocPos pos(doc);
          state->CompileMultiple(doc, pos, _conditionCompiled, false, GWorld->GetMissionNamespace()); // mission namespace
          state->EndContext(gshandle);
#endif
        }
      }
    }
    _descriptionExp = par >> "description";

    ConstParamEntryPtr entry = par.FindEntry("descriptionShort");
    if (entry) _descriptionShortExp = *entry;
    else _descriptionShortExp = _descriptionExp;

    entry = par.FindEntry("descriptionHUD");
    if (entry) _descriptionHUDExp = *entry;
    else _descriptionHUDExp = _descriptionShortExp;

    _destinationExp = par >> "destination";

    ParamEntryVal resources = par >> "resources";
    int n = resources.GetSize();
    _resources.Realloc(n);
    _resources.Resize(n);
    for (int i=0; i<n; i++) _resources[i] = resources[i];
  }
}

const FSMScriptedType *AITaskType::GetFSMType() const
{
  return _fsmType;
}

RString AITaskType::GetSentence(RString name) const
{
  if (!_parClass) return RString();
  ConstParamEntryPtr entry = _parClass->FindEntry(name);
  if (!entry) return RString();
  return *entry;
}

AITaskTypeBank AITaskTypes;

#if _ENABLE_INDEPENDENT_AGENTS
AITask::AITask(
   AITeamMember *owner, AITeamMember *sender, Task *parent, const AITaskType *type,
   FSM *fsm, float priority)
   : Task(parent, type ? type->GetTypeName() : RString())
{
  _owner = owner;
  _sender = sender;
  Assert(type);
  _type = type;
  _fsm = fsm;
  _priority = priority;
  _running = false;
}
#else
AITask::AITask(
   AIBrain *owner, Task *parent, const AITaskType *type,
   FSM *fsm, float priority)
   : Task(parent, type ? type->GetTypeName() : RString())
{
  _owner = owner;
  Assert(type);
  _type = type;
  _fsm = fsm;
  _priority = priority;
  _running = false;
}

#endif

bool AITask::GetDescription(RString &result) const
{
  if (!_type || !_fsm) return false;
  RString expression = _type->GetDescriptionExp();
  if (expression.GetLength() == 0) return false;
  GameValue value;
  if (!_fsm->Evaluate(value, expression)) return false;
  if (value.GetType() != GameString) return false;
  result = value;
  return true;
}

bool AITask::GetDescriptionShort(RString &result) const
{
  if (!_type || !_fsm) return false;
  RString expression = _type->GetDescriptionShortExp();
  if (expression.GetLength() == 0) return false;
  GameValue value;
  if (!_fsm->Evaluate(value, expression)) return false;
  if (value.GetType() != GameString) return false;
  result = value;
  return true;
}

bool AITask::GetDescriptionHUD(RString &result) const
{
  if (!_type || !_fsm) return false;
  RString expression = _type->GetDescriptionHUDExp();
  if (expression.GetLength() == 0) return false;
  GameValue value;
  if (!_fsm->Evaluate(value, expression)) return false;
  if (value.GetType() != GameString) return false;
  result = value;
  return true;
}

bool AITask::GetDestination(Vector3 &result) const
{
  if (!_type || !_fsm) return false;
  RString expression = _type->GetDestinationExp();
  if (expression.GetLength() == 0) return false;
  GameValue value;
  if (!_fsm->Evaluate(value, expression)) return false;

  // avoid type error in GetPos() - check to ensure value is an array
  if (value.GetType() != GameArray) return false;

  GameState *state = GWorld->GetGameState();
  bool GetPos(const GameState *state, Vector3 &ret, GameValuePar oper);
  return GetPos(state, result, value);
}

AITask *AITask::CreateObject(ParamArchive &ar)
{
  return new AITask(SerializeConstructor);
}

LSError AITask::Serialize(ParamArchive &ar)
{
  CHECK(Task::Serialize(ar))
  CHECK(ar.SerializeRef("owner", _owner, 1))
  if (ar.IsSaving())
  {
    RString typeName = _type ? _type->GetTypeName() : RString();
    CHECK(ar.Serialize("type", typeName, 1))
  }
  else if (ar.GetPass() == ParamArchive::PassFirst)
  {
    // create type and fsm
    RString typeName;
    CHECK(ar.Serialize("type", typeName, 1))
    if (typeName.GetLength() > 0)
    {
#if _ENABLE_INDEPENDENT_AGENTS
      AITeamMember *teamMember = reinterpret_cast<AITeamMember *>(ar.GetParams());
      _type = teamMember->GetTaskType(typeName);
      if (_type) _fsm = CreateTeamMemberFSMScripted(_type->GetFSMType(), teamMember->GetVars());
#endif
    }
  }
  if (_fsm)
  {
    CHECK(ar.Serialize("FSM", *_fsm, 1))
  }
  CHECK(ar.Serialize("priority", _priority, 1, 0))
  CHECK(ar.Serialize("running", _running, 1, false))
  return LSOK;
}

AITask *AITask::LoadRef(ParamArchive &ar)
{
#if _ENABLE_INDEPENDENT_AGENTS
  AITeamMemberLink owner = AITeamMember::LoadTeamMemberRef(ar);
  if (!owner) return NULL;

  int taskId;
  if (ar.Serialize("taskId", taskId, 1, -1) != LSOK) return NULL;
  return owner->FindTask(taskId);
#else
  return NULL;
#endif
}

LSError AITask::SaveRef(ParamArchive &ar)
{
  CHECK(base::SaveRef(ar))
#if _ENABLE_INDEPENDENT_AGENTS
  DoAssert(_owner);
  CHECK(_owner->SaveTeamMemberRef(ar))
  CHECK(ar.Serialize("taskId", _id, 1, -1))
#endif
  return LSOK;
}

DEFINE_SERIALIZE_TYPE_REF_INFO(AITask, Task);

#endif

#if _ENABLE_INDEPENDENT_AGENTS

AITeamMember::~AITeamMember()
{
#if _ENABLE_IDENTITIES
  // remove tasks that are assigned to the team
  _tasks.Clear();
#endif
}

LSError AITeamMemberRef::Serialize(ParamArchive &ar)
{
  if (ar.IsSaving() || ar.GetPass() == ParamArchive::PassSecond)
  {
    DoAssert(_ref);
    if (_ref)
    {
      CHECK(_ref->Serialize(ar))
    }
  }
  else
  {
    _ref = AITeamMember::CreateObject(ar);
    DoAssert(_ref);
    if (_ref)
    {
      CHECK(_ref->Serialize(ar))
    }
  }
  return LSOK;
}

AITeamMember *AITeamMember::CreateObject(ParamArchive &ar)
{
  RString type;
  if (ar.Serialize("teamMemberType", type, 1) != LSOK) return NULL;

  // only teams are serialized this way
  if (stricmp(type, "Team") == 0)
    return new AITeam();

  return NULL;
}

LSError SaveRefT(AITeamMemberRef &value, ParamArchive &arRef)
{
  return value->SaveTeamMemberRef(arRef);
}

LSError LoadRefT(AITeamMemberRef &value, ParamArchive &arRef)
{
  value = AITeamMember::LoadTeamMemberRef(arRef);
  return LSOK;
}

LSError SaveRefT(AITeamMemberLink &value, ParamArchive &arRef)
{
  return value->SaveTeamMemberRef(arRef);
}

LSError LoadRefT(AITeamMemberLink &value, ParamArchive &arRef)
{
  value = AITeamMember::LoadTeamMemberRef(arRef);
  return LSOK;
}

AITeamMember *AITeamMember::LoadTeamMemberRef(ParamArchive &ar)
{
  RString type;
  if (ar.Serialize("teamMemberType", type, 1) != LSOK) return NULL;

  if (stricmp(type, "Team") == 0)
    return AITeam::LoadRef(ar);
  else if (stricmp(type, "Agent") == 0)
  {
    AIBrain *brain = AIAgent::LoadRef(ar);
    if (!brain) return NULL;
    return brain->GetTeamMember();
  }

  return NULL;
}

LSError AITeamMember::SaveLinkRef(ParamArchive &ar, AITeamMemberLink &link)
{
  return link->SaveTeamMemberRef(ar);
}

LSError AITeamMember::LoadLinkRef(ParamArchive &ar, AITeamMemberLink &link)
{
  link = LoadTeamMemberRef(ar);
  return LSOK;
}

LSError AITeamMember::Serialize(ParamArchive &ar)
{
#if _ENABLE_IDENTITIES
  CHECK(ar.Serialize("TaskTypes", _taskTypes, 1))
  // to search the type by the name, we need to access team member in task
  {
    void *old = ar.GetParams();
    ar.SetParams(this);
    LSError result = ar.Serialize("Tasks", _tasks, 1);
    ar.SetParams(old);
    if (result != LSOK) return result;
  }
  CHECK(ar.Serialize("nextTaskId", _nextTaskId, 1))
#endif

  CHECK(ar.SerializeArray("Resources", _resources, 1))
  CHECK(ar.Serialize("fromEditor", _fromEditor, 1))

  return LSOK;
}

AITeamMember *AITeamMemberLinkTraits::GetObject(TrackSoftLinks<IdTraits> *tracker)
{
  NetworkObject *object = static_cast<NetworkObject *>(tracker->GetObject());
  if (!object) return NULL;
  return object->GetTeamMember();
}

RString AITeamMember::GetCallSignShort() const
{
  const GameVarSpace *vars = GetVars();
  if (!vars) return RString();

  GameValue value;
  if (!vars->VarGet("callSignShort", value)) return RString();
  return value;
}

RString AITeamMember::GetCallSign() const
{
  const GameVarSpace *vars = GetVars();
  if (!vars) return RString();

  GameValue value;
  if (!vars->VarGet("callSign", value)) return RString();
  return value;
}

RString AITeamMember::GetCallSignLong() const
{
  const GameVarSpace *vars = GetVars();
  if (!vars) return RString();

  GameValue value;
  if (!vars->VarGet("callSignLong", value)) return RString();
  return value;
}

#if _ENABLE_IDENTITIES

static int CmpAITasks(const RefR<AITask> *t1, const RefR<AITask> *t2)
{
  const AITask *task1 = *t1;
  const AITask *task2 = *t2;

  // sort by priorities (lexicographically)
  AUTO_STATIC_ARRAY(float, priorities1, 16);
  for (const Task *t=task1; t; t=t->GetParent())
    priorities1.Add(t->GetPriority());
  AUTO_STATIC_ARRAY(float, priorities2, 16);
  for (const Task *t=task2; t; t=t->GetParent())
    priorities2.Add(t->GetPriority());

  int i1 = priorities1.Size() - 1;
  int i2 = priorities2.Size() - 1;
  while (i1 >= 0 && i2 >= 0)
  {
    float diff = priorities2[i2] - priorities1[i1];
    if (diff != 0) return sign(diff);
    i1--; i2--;
  }
  if (i1 >= 0) return -1;
  if (i2 >= 0) return 1;

  // keep running task running
  if (task1->IsRunning())
  {
    if (!task2->IsRunning()) return -1;
  }
  else
  {
    if (task2->IsRunning()) return 1;
  }

  // sort by name
  int diff = stricmp(task1->GetName(), task2->GetName());
  if (diff != 0) return diff;

  return task1->GetId() - task2->GetId();
}

#endif

void AITeamMember::SimulateAI()
{
#if _ENABLE_IDENTITIES
  GameVarSpace *vars = GetVars();
  if (!vars) return;

  // task types simulation
  GameState *state = GWorld->GetGameState();
  int gshandle = state->BeginContext(vars);
  vars->VarLocal("_this");
  vars->VarLocal("_tasktype");
  vars->VarLocal("_thiscreated");
  vars->VarLocal("_thisrunning");
  vars->VarLocal("_totalcreated");
  vars->VarLocal("_totalrunning");
  vars->VarLocal("_task");
  vars->VarSet("_this", GameValueExt(this), true);
  for (int i=0; i<_taskTypes.Size(); i++)
  {
    AITaskTypeInfo &info = _taskTypes[i];
    AITaskType *taskType = info._type;
    RString condition = taskType->GetCondition();
    if (condition.GetLength() == 0) continue;

    vars->VarSet("_tasktype", GameValue(taskType->GetTypeName()), true);
    vars->VarSet("_thiscreated", GameValue((float)info._created), true);
    int count = 0;
    for (int j=0; j<_tasks.Size(); j++)
    {
      if (_tasks[j]->GetType() == taskType) count++;
    }
    vars->VarSet("_thisrunning", GameValue((float)count), true);
    vars->VarSet("_totalcreated", GameValue((float)taskType->GetInstancesCreated()), true);
    vars->VarSet("_totalrunning", GameValue((float)(taskType->GetInstancesCreated() - taskType->GetInstancesDeleted())), true);

#if USE_PRECOMPILATION
    state->Execute(condition, taskType->GetConditionCompiled(), GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
#else
    state->Execute(condition, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
#endif
  }

  // task manager
  // sort tasks by priorities
  QSort(_tasks.Data(), _tasks.Size(), CmpAITasks);
  // resume tasks by priorities and resources
  AUTO_STATIC_ARRAY(char, states, 32);
  static const char Available = 0;
  static const char Assigned = 1;
  static const char Testing = 2;
  int n = _resources.Size();
  states.Resize(n);
  for (int i=0; i<n; i++) states[i] = Available;

  for (int i=0; i<_tasks.Size(); i++)
  {
    AITask *task = _tasks[i];
    Assert(task);
    const AITaskType *type = task->GetType();
    if (!type)
    {
      task->Run(false);
      continue;
    }

    bool ok = true;
    const AutoArray<AITaskResource> &required = type->GetResources();
    for (int j=0; j<required.Size(); j++)
    {
      // check if this resource is available
      ok = false;
      for (int k=0; k<n; k++)
      {
        if (states[k] == Available && stricmp(_resources[k], required[j]) == 0)
        {
          states[k] = Testing;
          ok = true;
          break;
        }
      }
      if (!ok) break;
    }
    task->Run(ok);
    if (ok)
    {
      for (int k=0; k<n; k++)
      {
        if (states[k] == Testing) states[k] = Assigned;
      }
    }
    else
    {
      for (int k=0; k<n; k++)
      {
        if (states[k] == Testing) states[k] = Available;
      }
    }
  }

  // tasks simulation
  for (int i=0; i<_tasks.Size();)
  {
    AITask *task = _tasks[i];
    Assert(task);
    FSM *fsm = task->GetFSM();
    if (!fsm)
    {
      if (task->GetType()) task->GetType()->OnInstanceDeleted();
      _tasks.Delete(i);
      continue;
    }
    if (!task->IsRunning())
    {
      i++;
      continue;
    }

    AITeamMemberContext ctx(this);
    GameValue CreateGameTask(Task *task);
    vars->VarSet("_task", CreateGameTask(task), true);
    if (fsm->Update(&ctx))
    {
      // FSM finished
      if (task->GetType()) task->GetType()->OnInstanceDeleted();
      GameVarSpace *vars = fsm->GetVars();
      if (vars) vars->_vars.Clear();
      _tasks.Delete(i);
      continue;
    }
    i++;
  }
  vars->VarSet("_task", GameValue(), true);
  state->EndContext(gshandle);
#endif
}

void AITeamMember::SendMsg(RadioMessage *message)
{
#if _ENABLE_DIRECT_MESSAGES
  // TODO: better selection of channel
  // now, direct speaking is always used
  AIBrain *GetSpeaker(AITeamMember *member);
  AIBrain *speaker = GetSpeaker(this);
  if (!speaker) return;
  Person *person = speaker->GetPerson();
  if (!person) return;
  RadioChannel *channel = person->GetRadio();
  if (!channel) return;

  channel->Transmit(message, speaker->GetLanguage());
#endif
}

#if _ENABLE_IDENTITIES

const char *FindArrayKeyTraits<AITaskTypeInfo>::GetKey(const AITaskTypeInfo &a)
{
  return a._type->GetTypeName();
}

bool AITeamMember::RegisterTaskType(AITaskType *taskType)
{
  return _taskTypes.AddUnique(AITaskTypeInfo(taskType)) >= 0;
}

bool AITeamMember::UnregisterTaskType(RString name)
{
  return _taskTypes.DeleteKey(name);
}

const AITaskType *AITeamMember::GetTaskType(RString name) const
{
  int index = _taskTypes.FindKey(name);
  if (index < 0) return NULL;
  return _taskTypes[index]._type;
}

int AITeamMember::AddTask(AITask *task)
{
  task->SetId(_nextTaskId++);

  // increase number of created tasks
  const AITaskType *type = task->GetType();
  if (type)
  {
    // local
    for (int i=0; i<_taskTypes.Size(); i++)
    {
      if (_taskTypes[i]._type == type)
      {
        _taskTypes[i]._created++;
        break;
      }
    }
    // total
    type->OnInstanceCreated();
  }

  return _tasks.Add(task);
}

AITask *AITeamMember::FindTask(int id)
{
  for (int i=0; i<_tasks.Size(); i++)
  {
    if (_tasks[i]->GetId() == id) return _tasks[i];
  }
  return NULL;
}

AITask *AITeamMember::GetTask(int index)
{
  return _tasks[index];
}

const AITaskType *AITeamMember::GetTaskType(int index)
{
  return _taskTypes[index]._type;
}

#endif

// AIAgent implementation

/*!
\patch_disabled 5088 Date 11/15/2006 by Bebul
- Fixed: cfgVoices contain voices for man and femaleVoices for woman
*/

static void CreateIdentity(IdentityInfo &result, const Person *person, int &index)
{
  // the list of usable faces
  AUTO_STATIC_ARRAY(RStringB, faces, 100);
  RString faceType = person->GetFaceType();
  if (faceType.GetLength() > 0)
  {
    ParamEntryVal cfgFaces = Pars >> "CfgFaces" >> person->GetFaceType();
    for (int i=0; i<cfgFaces.GetEntryCount(); i++)
    {
      ParamEntryVal cfgFace = cfgFaces.GetEntry(i);
      if (!cfgFace.IsClass()) continue;

      // check if some identity type of this face is matching with the person
      ParamEntryVal identityTypes = cfgFace >> "identityTypes";
      bool found = false;
      for (int j=0; j<identityTypes.GetSize(); j++)
      {
        if (person->IsIdentityEnabled(identityTypes[j]))
        {
          found = true;
          break;
        }
      }
      if (!found) continue; // face is not matching this person

      faces.Add(cfgFace.GetName());
    }
  }
  if (faces.Size() == 0)
  {
    // avoid empty array - random selection is processed later
    faces.Add("Default");
  }

  // random glasses
  AUTO_STATIC_ARRAY(RStringB, glasses, 32);
  AUTO_STATIC_ARRAY(float, glassesWeight, 32);
  ParamEntryVal cfgGlasses = Pars >> "CfgGlasses";
  for (int i=0; i<cfgGlasses.GetEntryCount(); i++)
  {
    ParamEntryVal cls = cfgGlasses.GetEntry(i);
    if (!cls.IsClass()) continue;
    // decide weight by the identity type
    ParamEntryVal identityTypes = cls >> "identityTypes";
    if (identityTypes.GetSize() % 2 != 0)
    {
      RptF("%s.identityTypes[] - even number of items expected (pairs [name, weight])");
    }
    else
    {
      for (int j=0; j<identityTypes.GetSize(); j+=2) // pairs [type, weight]
      {
        if (person->IsIdentityEnabled(identityTypes[j]))
        {
          // matching identity type found
          glasses.Add(cls.GetName());
          glassesWeight.Add(identityTypes[j + 1]);
          break;
        }
      }
    }
  }
  if (glasses.Size() == 0)
  {
    // avoid empty array - random selection is processed later
    glasses.Add("None");
    glassesWeight.Add(1.0f);
  }

  // the list of usable voices
  AUTO_STATIC_ARRAY(RStringB, voices, 100);

  RString voiceStr;

  ParamEntryVal cls = Pars >> "CfgVoice";
  for (int i=0; i<cls.GetEntryCount(); i++)
  {
    ParamEntryVal voice = cls.GetEntry(i);
    if (! voice.IsClass()) continue;

    ConstParamEntryPtr scope = voice.FindEntry("scope");
    if (!scope || !(bool)*scope) continue;

    ConstParamEntryPtr entry = voice.FindEntry("disabled");
    if (entry && (bool)*entry) continue;

    // check if some identity type of this voice is matching with the person
    ParamEntryVal identityTypes = voice >> "identityTypes";
    bool found = false;
    for (int j=0; j<identityTypes.GetSize(); j++)
    {
      if (person->IsIdentityEnabled(identityTypes[j]))
      {
        found = true;
        break;
      }
    }
    if (!found) continue; // face is not matching this person

    voices.Add(voice.GetName());
  }

  // identities config 
  RString names = person->GetGenericNames();
  ParamEntryVal cfg = Pars >> "CfgWorlds" >> "GenericNames" >> names;

  for (int j=0; j<100; j++) // check max. 100 identities
  {
    int seed = 100;
    // first name
    int nFirst = (cfg >> "FirstNames").GetEntryCount();
    int nLast = (cfg >> "LastNames").GetEntryCount();
    if (nFirst>0 && nLast>0)
    {
      int iFirst = toIntFloor(nFirst * GRandGen.RandomValue(CurrentTemplate->RandomSeed(), index, seed++));
      int iLast = toIntFloor(nLast * GRandGen.RandomValue(CurrentTemplate->RandomSeed(), index, seed++));
      saturate(iFirst, 0, nFirst - 1);
      saturate(iLast, 0, nLast - 1);
      RString firstName = (cfg >> "FirstNames").GetEntry(iFirst);
      RString lastName = (cfg >> "LastNames").GetEntry(iLast);
      // name
      result._name = firstName + RString(" ") + lastName;
    }

    // face
    int nFace = faces.Size();
    int iFace = toIntFloor(nFace * GRandGen.RandomValue(CurrentTemplate->RandomSeed(), index, seed++));
    saturate(iFace, 0, nFace - 1);
    result._face = faces[iFace];

    // glasses
    float totalWeight = 0;
    for (int i=0; i<glassesWeight.Size(); i++) totalWeight += glassesWeight[i];
    float weight = totalWeight * GRandGen.RandomValue(CurrentTemplate->RandomSeed(), index, seed++);

    int iGlasses = glasses.Size() - 1;
    for (int i=0; i<glassesWeight.Size(); i++)
    {
      weight -= glassesWeight[i];
      if (weight <= 0)
      {
        iGlasses = i; break;
      }
    }
    result._glasses = glasses[iGlasses];

    // speaker
    int voicesCount = voices.Size();
    if (voicesCount > 0)
    {
      int iVoice = toIntFloor(voicesCount * GRandGen.RandomValue(CurrentTemplate->RandomSeed(), index, seed++));
      saturate(iVoice, 0, voicesCount - 1);
      result._speaker = voices[iVoice];
    }

    // pitch
    result._pitch = 0.95f + 0.1f * GRandGen.RandomValue(CurrentTemplate->RandomSeed(), index, seed++);

    // check if not dead
    index++;
  }
  // return the last one
}

AIAgent::AIAgent(Person *vehicle)
: base(vehicle)
{
  _expensiveThinkFrac = toIntFloor(GRandGen.RandomValue() * 1000);
  // perform first expensive think as soon as possible
  _expensiveThinkTime = Glob.time;

  // assign random identity
  if (vehicle)
  {
    int index = INT_MAX * GRandGen.RandomValue();
    IdentityInfo info;
    CreateIdentity(info, vehicle, index);
    Load(info);
  }
}

inline void AIAgent::ExpensiveThinkDone()
{
  // calculate time of next expensive think
  // find start of this second
  // advance to start of next second, advance to our frac
  _expensiveThinkTime = Glob.time.Floor().AddMs(1000 + _expensiveThinkFrac);
}

void AIAgent::ApplyContext(GameState &state)
{
  state.VarSetLocal("_agent", GameValueExt(GetPerson()), true);
}

bool AIAgent::IsInCargo() const
{
  // TODO: inline release version
  return _state == InCargo;
}

void AIAgent::SetInCargo(bool set)
{
  if (set)
  {
    SetState(InCargo);
  }
  else
  {
    // enable change InCargo => Wait
    SetState(Wait, true);
    ClearOperativePlan();
  }
}

void AIAgent::SetSearchTime(Time time)
{
  GetPath().SetSearchTime(time);
}

TargetSide AIAgent::GetSide() const
{
  if (_person) return _person->GetTargetSide();
  return TSideUnknown;
}

bool AIAgent::IsEnemy(TargetSide side) const
{
  if (!_person) return false;
  TargetSide mySide = _person->GetTargetSide();
// Try to avoid center
/*
  AICenter *center = GWorld->GetCenter(mySide);
  if (center) return center->IsEnemy(side);
*/
  return side == TEnemy || side != mySide && side != TFriendly && side != TCivilian && side >= 0 && side < TSideUnknown;
}

bool AIAgent::IsNeutral( TargetSide side) const
{
// Try to avoid center
/*
  if (!_person) return false;
  TargetSide mySide = _person->GetTargetSide();
  AICenter *center = GWorld->GetCenter(mySide);
  if (center) return center->IsNeutral(side);
*/
  return false;
}

bool AIAgent::IsFriendly(TargetSide side) const
{
  if (!_person) return true;
  TargetSide mySide = _person->GetTargetSide();
// Try to avoid center
/*
  AICenter *center = GWorld->GetCenter(mySide);
  if (center) return center->IsFriendly(side);
*/
  // no center
  return side == TFriendly || side == TCivilian || side == mySide && side != TEnemy && side >= 0 && side <TSideUnknown;
}

int AIAgent::RandomSeed() const
{
  OIDInt objId = GetPerson()->GetObjectId().Encode();
  return 233 * objId;
}

float AIAgent::GetMorale() const
{
  // TODO: implement some morale simulation
  return 0;
}

CombatModeRange AIAgent::GetCombatModeBasedOnCommand() const
{
  // TODO: check tasks here to check if any of them is enforcing a combat mode
  return CombatModeRange(CMSafe,CMStealth);
}

CautionRange AIAgent::GetCautionBasedOnCommand() const
{
  // TODO: check tasks here to check if any of them is enforcing a combat mode
  return CautionRange();
}

bool AIAgent::GetCoveringBasedOnCommand() const
{
  return true;
}

bool AIAgent::ForEachTarget(const TargetFunctor &func) const
{
  // TODO: implement ForEach in TargetList
  for (int i=0; i<_targetList.AnyCount(); i++)
  {
    if (func(_targetList.GetAny(i))) return true;
  }
#if _ENABLE_CONVERSATION
  for (int i=0; i<_targetKB.Size(); i++)
  {
    if (func(_targetKB[i])) return true;
  }
#endif
  return false;
}

void AIAgent::OnTargetDeleted(Target *t)
{
#if _ENABLE_CONVERSATION
  // the target may already be deleted before and therefore known
  if (!_targetKB.Find(t))
  {
  _targetKB.Add(new TargetKnowledge(t));
  }
#endif
}

#if _ENABLE_CHEATS

void AIAgent::DebugFSM()
{
  base::DebugFSM();

#if _ENABLE_IDENTITIES && DEBUG_FSM
  for (int i=0; i<_tasks.Size(); i++)
  {
    AITask *task = _tasks[i];
    if (task && task->GetFSM()) task->GetFSM()->Debug();
  }
#endif
}

#endif

void AIAgent::SimulateAI()
{
  if (!_person || _person->IsSimulationDisabled()) return;

  if (Glob.time > _expensiveThinkTime)
  {
    ExpensiveThinkDone();
    CreateTargetList();
    CheckEnemies();
  }

  // now only pathfinding is performed, add more functionality later
  if (!IsLocal()) return;

  if (!LSIsAlive()) return;

#if LOG_THINK
  Log("    Agent %s think.", (const char *)GetDebugName());
#endif

  SetWatch();

  EntityAIFull *veh = GetVehicle();

  // if some enemy is near, we have to track near vicinity continuously
  float nearEnemy = 100;
  if (veh && (veh->CommanderUnit() == this || veh->GunnerUnit() == this))
  {
    // if we are not fighting, there is no need for frequent updates
    if (GetCombatMode() > CMSafe && veh->HasSomeWeapons())
    {
      if (_nearestEnemyDist2 < Square(nearEnemy))
      {
        veh->TrackNearTargets(_targetList,nearEnemy);
      }
      veh->TrackLockedTargets(_targetList);
    }
  }

  //TEMP: dummy Zombie AI test.
/*
  if(GetPerson()->Type()->GetName() == "zombieEngine")
  {
	  for(int n = 0; n < GetTargetList().AnyCount(); n++)
	  {
	    const TargetNormal* targ = GetTargetList().GetAny(n);
	    if(!(targ->GetType()->GetName() == "zombieEngine"))
	    {
	      SetWantedPosition(PlanPosition(targ->GetPosition(),false), 1.5f, AIBrain::LeaderPlanned, true);
	      
	      break;
	    }
	  }
    return;
  }
  */
  AITeamMember::SimulateAI();

  // fixed show-term AI reaction
  bool fsmProcessed = false;
  if (HasAI())
  {
    if (_reactionFSM && !IsAIDisabled(DAFSM))
    {
      PROFILE_SCOPE_EX(aiFSM,ai);
      if (PROFILE_SCOPE_NAME(aiFSM).IsActive()) PROFILE_SCOPE_NAME(aiFSM).AddMoreInfo(_reactionFSM->GetDebugState());
      bool done = _reactionFSM->Update(GetPerson());
#if 0
      LogF("%s %.2f: State %s (danger)", cc_cast(GetDebugName()), Glob.time.toFloat(), cc_cast(_reactionFSM->GetStateName()));
      // LogF(" - assigned target %s, watch mode %s", GetTargetAssigned() ? cc_cast(GetTargetAssigned()->GetDebugName()) : "NULL", cc_cast(FindEnumName(_watchMode)));
#endif
      if (done)
      {
        // finished
        _reactionFSM = NULL;
      }
      else
      {
        fsmProcessed = _reactionFSM->IsExclusive(GetPerson());
      }
    }
  }

  if (veh->PilotUnit() != this) return;

  if (!HasAI()) return;

#if _ENABLE_AI

  if (_state == Stopping || _state == Stopped) return;

  ThinkImportance prec = AI::LevelOperative;

  switch (_planningMode)
  {
  case DoNotPlan: case DoNotPlanFormation:
    // do not create new plan (if exist, can continue with the old one)
    if (_noPath) return;
    else
    {
      // operative planning target
      if (_state == Wait || _state == Replan)
      {
        if (!OperPath(prec)) return;
      }

      // operative planning
      CreateOperativePath();
      return;
    }
  case LeaderDirect:
    {
      SetMode(AIBrain::DirectExact);
      Verify(SetState(AIBrain::Init));
      _iter = 0;
      _lastPlan = true;

      // operative planning
      CreateOperativePath();
      return;
    }
  case LeaderPlanned:
  case VehiclePlanned:
    {
      // check strategic target
      float error2 = _plannedPosition.Distance2(_wantedPosition);
      float limit2 = Square(0.1) * Position(GetFutureVisualState()).Distance2(_wantedPosition);
      // replan if target position changed by 10% of total distance
      if (error2 > limit2)
      {
        _plannedPosition = _wantedPosition;
        _noPath = true;
        RefreshStrategicPlan();
      }

      // strategic planning
      CreateStrategicPath(prec);

      // operative planning target
      if (_state == Wait || _state == Replan)
      {
        if (!OperPath(prec)) return;
      }

      // operative planning
      CreateOperativePath();
      return;
    }
  default:
    Fail("Planning mode");
    return;
  }

#else
  return;
#endif //_ENABLE_AI
}

// NetworkObject implementation
RString AIAgent::GetDebugName() const
{
  // TODO:
  return _person ? Format("Agent 0x%x", this) : "<No person>";
}

LSError AIAgent::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(AITeamMember::Serialize(ar))

  if (ar.IsSaving())
  {
    RString type = "Agent";
    CHECK(ar.Serialize("teamMemberType", type, 1))
  }

  CHECK(ar.Serialize("expensiveThinkFrac", _expensiveThinkFrac, 1, 0));
  CHECK(::Serialize(ar, "expensiveThinkTime", _expensiveThinkTime, 1, Time(0)));

  // targets
  CHECK(_targetList.Serialize(ar, "targetList", 1))
#if _ENABLE_CONVERSATION
  CHECK(ar.Serialize("targetKB", _targetKB, 1))
#endif

  return LSOK;
}

LSError AIAgent::SaveTeamMemberRef(ParamArchive &ar)
{
  RString type = "Agent";
  CHECK(ar.Serialize("teamMemberType", type, 1))
  return SaveRef(ar);
}

bool AIAgent::CreateTargetList(bool initialize, bool report)
{
  if (!LSCanTrackTargets()) return false;

  EntityAIFull *vehicle = GetVehicle();
  // if (vehicle && !vehicle->GetType()->ScanTargets()) return false;
  
  if (vehicle && (vehicle->CommanderUnit() == this || vehicle->GunnerUnit() == this))
  {
    vehicle->WhatIsVisible(_targetList, initialize);
  }

  _targetList.Manage(this);

  int enemiesDetected = 0;
  int unknownsDetected = 0;
  bool someTarget = false;

  // TODO: use Unknown and Enemy instead of Any
  int n = _targetList.AnyCount();
  for (int i=0; i<n; i++)
  {
    TargetNormal *vtar = _targetList.GetAny(i);

    if (vtar->IsKnownBySome())
    {
      if (vtar->side == TSideUnknown)
      {
        someTarget = true;
        unknownsDetected++;
      }
      else if (IsEnemy(vtar->side))
      {
        enemiesDetected++;
        someTarget=true;
      }
    }
  }

	// prepare subjective costs
	// TODO: subject. cost - separate Enemy/Friendly
	for (int i=0; i<_targetList.AnyCount(); i++)
	{
		TargetNormal *tar = _targetList.GetAny(i);

    // tar->State will return at most TargetAlive for anything not IsEnemy
		if(
			!tar->idExact || !tar->IsKnown() ||
			tar->destroyed || tar->vanished ||
			tar->State(this)<TargetEnemy
		)
		{

		  if( tar->side==TSideUnknown )
		  {
			  tar->subjectiveCost=0;
		  }
		  else if (tar->side!=TCivilian && IsFriendly(tar->side))
		  {
			  // friendly - no need to test it
			  tar->subjectiveCost=-1e10;
		  }
		  else
		  {
		    // we want emoty/neutral to be listed before friendly
			  // reset subjective cost
			  tar->subjectiveCost=-1e5;
		  }

			continue;
		}
		else
		{
		  DoAssert (IsEnemy(tar->side));
		  // calculate damagePerMinute and subjectiveCost
		  tar->subjectiveCost=GetSubjectiveCost(tar);
		}


	}

  // sort
  _targetList.QSort(TargetNormal::BetterTarget());
  _targetList.CompactIfNeeded();

// LogF("Agent %s, %d targets, %d enemies, %d unknown", cc_cast(GetDebugName()), _targetList.Size(), enemiesDetected, unknownsDetected);

  return someTarget;
}

float AIAgent::GetSubjectiveCost(TargetNormal *tar) const
{
  const VehicleType *type = tar->type;
  float baseCost = type->GetCost();
  // consider cost of danger from the target here, or other benefits of destroying it
  return baseCost;
}

void AITeam::SimulateAI()
{
  AITeamMember::SimulateAI();
}

void AITeam::ApplyContext(GameState &state)
{
  state.VarSetLocal("_team", GameValueExt(this), true);
}

LSError AITeam::Serialize(ParamArchive &ar)
{
  CHECK(AITeamMember::Serialize(ar))

  if (ar.IsSaving())
  {
    RString type = "Team";
    CHECK(ar.Serialize("teamMemberType", type, 1))
  }

  CHECK(ar.SerializeRef("TeamLeader", _teamLeader, 1))
  CHECK(ar.Serialize("typeName", _typeName, 1))
  CHECK(ar.Serialize("name", _name, 1))
  CHECK(ar.Serialize("formation", _formation, 1))
  CHECK(ar.Serialize("id", _id, 1))

  // variables
  {
    void *old = ar.GetParams();
    ar.SetParams(&GGameState);
    DoAssert(_vars.IsSerializationEnabled());
    LSError result = ar.Serialize("Variables", _vars._vars, 1);
    ar.SetParams(old);
    if (result != LSOK) return result;
  }

  CHECK(ar.SerializeRefs("TeamMembers", _teamMembers, 1))

  return LSOK;
}

LSError AITeam::SaveTeamMemberRef(ParamArchive &ar)
{
  RString type = "Team";
  CHECK(ar.Serialize("teamMemberType", type, 1))
  CHECK(ar.Serialize("id", _id, 1))
  return LSOK;
}

AITeamMember *AITeam::LoadRef(ParamArchive &ar)
{
  int id;
  if (!ar.Serialize("id", id, 1)) return NULL;
  return GWorld->GetTeam(id);
}

#else

NetworkMessage *CreateNetworkMessageUpdateAIAgent() {return NULL;}
void UpdateMessageFormatUpdateAIAgent(NetworkMessageFormatBase *,int &) {}
NetworkMessage *CreateNetworkMessageCreateAIAgent() {return NULL;}
void UpdateMessageFormatCreateAIAgent(NetworkMessageFormatBase *,int &) {}

NetworkMessage *CreateNetworkMessageUpdateAITeam() {return NULL;}
void UpdateMessageFormatUpdateAITeam(NetworkMessageFormatBase *,int &) {}
NetworkMessage *CreateNetworkMessageCreateAITeam() {return NULL;}
void UpdateMessageFormatCreateAITeam(NetworkMessageFormatBase *,int &) {}

#endif

static const EnumName UnitModeNames[]=
{
  EnumName(AIBrain::DirectNormal, "DIRECT NORMAL"),
  EnumName(AIBrain::DirectExact, "DIRECT EXACT"),
  EnumName(AIBrain::Normal, "NORMAL"),
  EnumName(AIBrain::Exact, "EXACT"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(AIBrain::Mode dummy)
{
  return UnitModeNames;
}

static const EnumName UnitPlanningModeNames[]=
{
  EnumName(AIBrain::DoNotPlan, "DoNotPlan"),
  EnumName(AIBrain::DoNotPlanFormation, "DoNotPlanFormation"),
  EnumName(AIBrain::LeaderPlanned, "LEADER PLANNED"),
  EnumName(AIBrain::LeaderDirect, "LEADER DIRECT"),
  EnumName(AIBrain::VehiclePlanned, "VEHICLE PLANNED"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(AIBrain::PlanningMode dummy)
{
  return UnitPlanningModeNames;
}

static const EnumName UnitStateNames[]=
{
  EnumName(AIBrain::Wait, "WAIT"),
  EnumName(AIBrain::Init, "INIT"),
  EnumName(AIBrain::InitFailed, "INITFAILED"),
  EnumName(AIBrain::Busy, "BUSY"),
  EnumName(AIBrain::Completed, "OK"),
  EnumName(AIBrain::Delay, "DELAY"),
  EnumName(AIBrain::InCargo, "CARGO"),
  EnumName(AIBrain::Stopping, "STOPPING"),
  EnumName(AIBrain::Replan, "REPLAN"),
  EnumName(AIBrain::Stopped, "STOPPED"),
  EnumName(AIBrain::Planning, "PLANNING"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(AIBrain::State dummy)
{
  return UnitStateNames;
}

static const EnumName UnitWatchModeNames[]=
{
  EnumName(AIBrain::WMNo, "NO"),
  EnumName(AIBrain::WMDir, "DIR"),
  EnumName(AIBrain::WMPos, "POS"),
  EnumName(AIBrain::WMPosGlance, "POS-GLANCE"),
  EnumName(AIBrain::WMTgt, "TGT"),
  EnumName(AIBrain::WMTgtGlance, "TGT-GLANCE"),
  EnumName(AIBrain::WMAround, "AROUND"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(AIBrain::WatchMode dummy)
{
  return UnitWatchModeNames;
}

static const EnumName LifeStateNames[]=
{
  EnumName(LifeStateAlive, "ALIVE"),
  EnumName(LifeStateDead, "DEAD"),
  EnumName(LifeStateAsleep, "ASLEEP"),
  EnumName(LifeStateUnconscious, "UNCONSCIOUS"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(LifeState dummy)
{
  return LifeStateNames;
}

static const EnumName DisabledAINames[]=
{
  EnumName(AIBrain::DATarget, "TARGET"),
  EnumName(AIBrain::DAAutoTarget, "AUTOTARGET"),
  EnumName(AIBrain::DAMove, "MOVE"),
  EnumName(AIBrain::DAAnim, "ANIM"),
#if _VBS3
  EnumName(AIBrain::DAPathPlan, "PATHPLAN"),
#endif
  EnumName(AIBrain::DAFSM, "FSM"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(AIBrain::DisabledAI dummy)
{
  return DisabledAINames;
}

LSError AIUnitInfo::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("name", _fullName, 1))
  CHECK(ar.Serialize("experience", _experience, 1, 0))
  CHECK(ar.SerializeEnum("rank", _rank, 1, RankPrivate))
  CHECK(ar.Serialize("face", _face, 1, ""))
  CHECK(ar.Serialize("glasses", _glasses, 1, "None"))
  CHECK(ar.Serialize("speaker", _speaker, 1, ""))
  CHECK(ar.Serialize("pitch", _pitch, 1, 1.0))
  return LSOK;
}

struct CompareFloatChE
{
  int operator () (const float *a, const float *b) const {return sign(*a-*b);}
};

void AIBrain::CheckEnemies()
{
  const TargetList *list = AccessTargetList();
  Vector3Val pos = Position(GetFutureVisualState());
  // find smallest range of two direction which will encompass all known dangerous enemies
  // this means: find a biggest angle where there is no dangerous enemy
  // or: find a largest angle between two dangerous enemies
  
  // TODO: if there are too many enemies, we might consider selecting only some of them
  // or, we could somehow discretize the headings so that we have an upper bound
  // we start by sorting enemy headings - we do not care about anything else
  AutoArray<float, MemAllocLocal<float,256> > headings;
  for (int i=0; i<list->EnemyCount(); i++)
  {
    TargetNormal *tgt = list->GetEnemy(i);
    if (!tgt->IsKnownBy(this)) continue;
    if (tgt->IsVanishedOrDestroyed()) continue;
    if (tgt->GetSubjectiveCost()<=0) continue;
    Vector3 offset = tgt->AimingPosition()-pos;
    // if we are out of range of his  weapons, we do not care about him
    // note: following will be inaccurate for units which changed their weapon layouts
    float maxRange = tgt->type->GetMaxWeaponRange();
    if (offset.SquareSize()>Square(maxRange)) continue;
    float heading = atan2(offset.X(),offset.Z());
    headings.Add(heading);
  }
  
  QSort(headings,CompareFloatChE());
  
  // now traverse the whole list, and compute angular differences
  if (headings.Size()<=0)
  {
    // set to none
    _enemyDirL = _enemyDirR = VZero;
  }
  else
  {
    // when cycling across the list boundary, the difference is one full circle
    float prev = headings.Last()-2*H_PI;
    float maxDiff = -FLT_MAX;
    float headingL = 0;
    float headingR = 0;
    for (int i=0; i<headings.Size(); i++)
    {
      float curr = headings[i];
      // calculate left-right angular difference. Result in range <0,2*ip>
      float diff = curr-prev;
      if (maxDiff<diff)
      {
        maxDiff = diff;
        // prev..next is left..right of the no-enemy zone, which means right..left of the enemy zone
        headingR = prev;
        headingL = curr;
      }
      prev = curr;
    }
    _enemyDirR = Matrix3(MRotationY,-headingR).Direction();
    _enemyDirL = Matrix3(MRotationY,-headingL).Direction();
  }
}


int AIBrain::NBehaviourDiags() const
{
  return NBehaviourFSMRole;
}
RString AIBrain::BehaviourDiags(int i) const
{
  if (_behaviourFSMs[i])
  {
    RString fsmName = _behaviourFSMs[i]->GetDebugName();
    return fsmName + " - " + _behaviourFSMs[i]->GetStateName();
  }
  return RString();
}

/*!
Function should return true during ratio*period time.
Average one decision change should be made per period.
Function requires no storage.
*/

bool AIBrain::RandomDecision(float period, float ratio) const
{
  int seed = RandomSeed()&0x7fff;
  float timeMod = fastFmod(Glob.time.toFloat()+seed,period);
  return timeMod<ratio*period;
}

bool DefFindFreePositionCallback(Vector3Par pos, void *context)
{
  return true;
}

bool AIBrain::LookAroundEnabled() const
{
  Object::Zoom *zoom = nullptr;

  if (GetVehicleIn()==nullptr)
  {
    zoom = GetPerson()->GetZoom();
  }
  else
  {
    zoom = GetVehicleIn()->GetZoom(this);
  }
  if (zoom->_lookAroundEnabled) return true;

  if(GetVehicleIn() && Glob.config.vehicleFreelook > 0) 
  {
    if(GetVehicleIn()->PilotUnit() == this || GetVehicleIn()->CopilotUnit()) return true; 
  }
  return IsInCargo();
}

bool AIBrain::ActionDriverFreeLook() const
{
  Object::Zoom *zoom = nullptr;

  if (GetVehicleIn()==nullptr)
  {
    zoom = GetPerson()->GetZoom();
  }
  else
  {
    zoom = GetVehicleIn()->GetZoom(this);
  }
  if (zoom->_lookAroundEnabled) return true;

  if (GetVehicleIn() && Glob.config.vehicleFreelook == 2) 
  {
    if (GetVehicleIn()->PilotUnit() == this || GetVehicleIn()->CopilotUnit() || IsInCargo()) return true;
    TurretContext context;
    if (GetPerson() && GetVehicleIn()->FindTurret(GetPerson(),context))
    {
      if (!context._weapons->ValidatedCurrentWeapon().IsValid()) return true;
    }
  }

  return false;
}
