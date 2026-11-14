// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"

#include "planeOrHeli.hpp"
#include "landscape.hpp"
#include "world.hpp"
#include "AI/ai.hpp"

PlaneOrHeli::PlaneOrHeli(const EntityAIType *name, Person *driver, bool fullCreate)
:base(name, driver, fullCreate),
_stopMode(SMNone),
_stopPosition(VZero),
_stopResult(SPNotReady)
{
}

struct IsHelipadContext
{
  AIBrain *unit;
};
static bool IsHelipad(Object *obj, float dist2, void *context)
{
  Entity *veh = dyn_cast<Entity>(obj);
  if (!veh) return false;
  const VehicleNonAIType *type = veh->GetNonAIType();

  //VBS3 should be more efficient to check the type before checking if it's empty
  if (!type->IsKindOf(GWorld->Preloaded(VTypeHelipad))) return false;

  // check for locks only - ignore cost, the one who placed the H probably knows what is he doing
  IsHelipadContext *ctx = (IsHelipadContext *)context;
  EntityAI *vehAI = ctx->unit->GetVehicle();
  
  AILockerSavedState state;
  vehAI->PerformUnlock(&state);

  int x = toIntFloor(obj->FutureVisualState().Position().X() * InvOperItemGrid);
  int z = toIntFloor(obj->FutureVisualState().Position().Z() * InvOperItemGrid);

  bool ret = !GLOB_LAND->LockingCache()->IsLocked(OperMapIndex(x), OperMapIndex(z), false);
    vehAI->PerformRelock(state);

  return ret;
}

PlaneOrHeli::StopPositionResult PlaneOrHeli::FindStopPosition()
{
  AIBrain *unit = DriverBrain();
  if (!unit)
  {
    Fail("No pilot");
    return SPFound;
  }

  StopPositionResult found = SPNotFound;

  Vector3 bestPos=FutureVisualState().Position();

  Vector3 preferredPos = _stratGoToPos;
  float maxStopDistance = floatMax(Type()->GetStopDistance()*1.3f,500);
  if (preferredPos.SquareSize() <= 0.1f || preferredPos.Distance2(bestPos) > Square(maxStopDistance))
  {
    // preferred position lies in front of us
    preferredPos = FutureVisualState().Position()+FutureVisualState().Speed()*6;
  }

  {
    IsHelipadContext ctx;
    ctx.unit = unit;
    bool ready = true;
    OLink(Object)  obj = GLandscape->NearestObjectAsync(ready,true,preferredPos,500,IsHelipad,&ctx,true);
    if (!ready) return SPNotReady;
    if (obj)
    {
      bestPos = obj->FutureVisualState().Position();
      found = SPFound;
    }
  }

  if (found == SPNotFound)
  {
    // unlock while searching
    AILockerSavedState state;
    PerformUnlock(&state);

    float bestCost=1e10;
    int range=2;
    do
    {
      // check neighbourhood
      // increase range if necessary
      int x=toIntFloor(preferredPos.X()*InvLandGrid);
      int z=toIntFloor(preferredPos.Z()*InvLandGrid);
      for( int xx=x-range; xx<=x+range; xx++ )
      for( int zz=z-range; zz<=z+range; zz++ )
      {
        GeographyInfo info=GLOB_LAND->GetGeography(xx,zz);
        float cost=0;
        if( info.u.minWaterDepth>1 ) continue;
        if( info.u.full ) continue;
        if( info.u.howManyObjects>0 ) continue;
        int grad=info.u.gradient;
        if (_stopMode == SMLand)
        {
          if( grad>=4 ) continue;
          const static int gradPenalty[4]={0,5,10,30};
          cost+=gradPenalty[grad];
          if (info.u.road) cost+=10;
        }
        else
        {
          if( grad>=5 ) continue;
          const static int gradPenalty[5]={0,0,2,5,10};
          cost+=gradPenalty[grad];
          if (info.u.road) cost+=2;
        }
        // some penalty for a water present
        cost += info.u.maxWaterDepth*5;
        cost+=sqrt((xx-x)*(xx-x)+(zz-z)*(zz-z))*0.5; // small penalty for distance
        Vector3 pos,normal;
        pos.Init();
        normal.Init();
        pos[0]=xx*LandGrid+LandGrid*0.5,pos[2]=zz*LandGrid+LandGrid*0.5;
        pos[1]=GLOB_LAND->RoadSurfaceYAboveWater(pos[0],pos[2]);
        // check for object collision at given place
        if( bestCost>cost )
        {
          //AIUnit *unit=DriverBrain();
          if( !AIBrain::FindFreePosition(pos,normal,false,GetType()) ) continue;
          bestPos=pos;
          bestCost=cost;
          found = SPFound;
        }
      }
    } while( bestCost>10 && (range+=2)<8 );

    PerformRelock(state);
  }

  if ((_stopPosition-bestPos).SquareSize()>Square(4))
  {
    // reset only when there is significant change
    ResetAutopilot();
  }
  _stopPosition=bestPos;
  return found;
}

void PlaneOrHeli::UpdateStopMode(AIBrain *unit)
{
  if (unit->GetState() == AIBrain::Stopping || unit->GetState() == AIBrain::Stopped)
  {
    bool supply = _supply && GetSupplyUnits().Size() > 0;
    // if we are not a helicopter, we always need a full stop (VTOL plane)
    StopMode mode = SMNone;
    if (_landing == LMLand) mode = SMLand;
    else if (supply || _landing == LMGetIn) mode = SMGetIn;
    else if (_landing == LMGetOut) mode = SMGetOut;

    if
    (
      mode != _stopMode ||
      unit->GetState() == AIBrain::Stopping && !unit->CheckEmpty(_stopPosition)
    )
    {
      switch (FindStopPosition())
      {
      case SPFound:
        _stopMode = mode;
        _stopResult = SPFound;
        break;
      case SPNotFound:
        // we will report a failure, hence we could cancel the landing - however we leave this up to the script
        //_landing = LMNone;
        unit->OnStepTimedOut();
        _stopResult = SPNotFound;
        break;
      case SPNotReady:
        // try it later
        break;
      }
    }
  }
  else
  {
    _stopMode = SMNone;
  }
}

void PlaneOrHeli::LandStarted(LandingMode landing)
{
  base::LandStarted(landing);
  if (landing != LMNone) _stopResult = SPNotReady;
}

int PlaneOrHeli::GetLastMissileIndex(const TurretContext &context, const WeaponModePtr &ptr) const
{
  const Magazine *currentMag = ptr.GetMagazine();
  if (!currentMag || !currentMag->Type() || !currentMag->Type()->_ammo) return -1;

  int total = 0;
  bool found = context._weapons->ForEachMagazine([&total, currentMag](Magazine *mag) -> bool
  {
    const MagazineType *type = mag->Type();
    if (!type) return false; // continue
    const AmmoType *ammo = type->_ammo;
    if (!ammo) return false; // continue
    if (ammo->_simulation != AmmoShotMissile) return false; // continue
    if (ammo->maxControlRange < 10) return false; // continue
    
    if(currentMag->Type()->_ammo->maverickweaponIndexOffset != ammo->maverickweaponIndexOffset) return false; // continue

    if (currentMag==mag)
    {
      total += mag->GetAmmo();
      return true; // found
    }

    total+=mag->Type()->_maxAmmo;
    return false; // continue
  });

  return found ? total : -1;
}

void PlaneOrHeli::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  base::GetActions(add, unit);
  if (!unit) return;

  if ( !unit->IsInCargo())
  {
    if ((unit==DriverBrain() || unit==CopilotBrain()) && QIsManual())
    {
      if( (!PilotUnit() || !PilotUnit()->IsPlayer()) || !CopilotUnit())
      {
        // lights on / off
        if (unit->IsPlayer() && _lights.SomeLights())
        {
          if (_pilotLight)
          {
            UIAction action(new ActionBasic(ATLightOff, this));
            add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATLightOn, this));
            add(action);
          }
        }
      }
    }
  }
}

#if _HELISIM_PRECISE_INTERACTION
void PlaneOrHeli::GetDrawActions3D(UIActions &actions, AIBrain *unit)
{
  if (!unit) return;

  if (!unit->IsInCargo())
  {
    if ((unit==DriverBrain() || unit==CopilotBrain()) && QIsManual())
    {
      if( (!PilotUnit() || !PilotUnit()->IsPlayer()) || !CopilotUnit())
      {
        // lights on / off
        if (unit->IsPlayer() && _reflectors.SomeLights())
        {
          if (_pilotLight)
          {
            UIAction action(new ActionBasic(ATLightOff, this));
            actions.Add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATLightOn, this));
            actions.Add(action);
          }
        }
      }
    }
  }

  base::GetDrawActions3D(actions, unit);
}
#endif

static const EnumName StopPositionResultNames[]=
{
  EnumName(PlaneOrHeli::SPFound, "Found"),
  EnumName(PlaneOrHeli::SPNotFound, "NotFound"),
  EnumName(PlaneOrHeli::SPNotReady, "NotReady"),
  EnumName()
};

template<>
const EnumName *GetEnumNames(PlaneOrHeli::StopPositionResult dummy)
{
  return StopPositionResultNames;
}

static const EnumName StopModeNames[]=
{
  EnumName(PlaneOrHeli::SMNone, "None"),
  EnumName(PlaneOrHeli::SMLand, "Land"),
  EnumName(PlaneOrHeli::SMGetIn, "GetIn"),
  EnumName(PlaneOrHeli::SMGetOut, "GetOut"),
  EnumName()
};

template<>
const EnumName *GetEnumNames(PlaneOrHeli::StopMode dummy)
{
  return StopModeNames;
}


bool HasCopilot::operator () (EntityAIFull *entity, TurretContext &context)
{
  if (!context._turret) return false; // continue
  if (!context._gunner) return false; // continue
  if(context._turretType->_isCopilot
    && context._gunner->Brain()
    && !context._gunner->Brain()->IsPlayer()
    && context._gunner->Brain()->IsAnyPlayer()) return true;

  return false; // continue
};
