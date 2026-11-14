#include "wpch.hpp"

#include "house.hpp"
#include "engine.hpp"
#include <El/Common/randomGen.hpp>
#include <El/Common/perfProf.hpp>
#include "keyInput.hpp"
#include "keyInput.hpp"
#include "AI/ai.hpp"
#include "person.hpp"
#include "world.hpp"
#include "smokes.hpp"
#include "txtPreload.hpp"
#include "UI/uiActions.hpp"
#include "stringtableExt.hpp"
#include "mbcs.hpp"
#include "diagModes.hpp"
#include "fireplace.hpp"
#include "fileLocator.hpp"

#include "camera.hpp"

#include "Network/network.hpp"

class PathActionLadderTop : public PathAction
{
protected:
  int _ladderIndex;

public:
  PathActionLadderTop(int ladderIndex) {_ladderIndex = ladderIndex;}

  PathActionType GetType() const {return PATLadderTop;}
  int GetLadderIndex() const {return _ladderIndex;}
};

class PathActionLadderBottom : public PathAction
{
protected:
  int _ladderIndex;

public:
  PathActionLadderBottom(int ladderIndex) {_ladderIndex = ladderIndex;}

  PathActionType GetType() const {return PATLadderBottom;}
  int GetLadderIndex() const {return _ladderIndex;}
};

class PathUserActionBegin : public PathAction
{
protected:
  RString _actionName;

public:
  PathUserActionBegin(RString actionName):_actionName(actionName){}

  PathActionType GetType() const {return PATUserActionBegin;}
  RString GetActionName() const {return _actionName;}
};

class PathUserActionEnd : public PathAction
{
protected:
  RString _actionName;

public:
  PathUserActionEnd(RString actionName):_actionName(actionName){}

  PathActionType GetType() const {return PATUserActionEnd;}
  RString GetActionName() const {return _actionName;}
};

BuildingType::BuildingType(ParamEntryPar param)
:base(param)
{
  _lightsExists = false;
  _windSockExist = false;
}

void BuildingType::LoadExt(ParamEntryPar par, const char *shape, bool enableHWAnim)
{
  //LogF("Type %s Load",(const char *)par.GetName());
  base::Load(par,shape);
  _coefInside = par >> "coefInside";
  _coefInsideHeur = par >> "coefInsideHeur";

  _windSockExist = par >> "windSockExist";
  // no HW animation for non-animated objects
  // no HW animation for objects which require keep height
  // for this we need to know the _shape
  if (_shapeAnimated!=AnimTypeNone && enableHWAnim)
  {
    SetHardwareAnimation(par);
  }

  ParamEntryVal list = par >> "MarkerLights";
  for (int i=0; i<list.GetEntryCount(); i++)
  {
    ParamEntryVal entry = list.GetEntry(i);
    if (entry.IsClass())
    {
      _lightsExists = true;
      break;
    }
  }
  if (!_lightsExists)
  {
    ParamEntryVal list = par >> "Reflectors";
    for (int i=0; i<list.GetEntryCount(); i++)
    {
      ParamEntryVal entry = list.GetEntry(i);
      if (entry.IsClass())
      {
        _lightsExists = true;
        break;
      }
    }
  }

  _replaceDamaged = par >> "replaceDamaged";
  _replaceDamagedLimit = par >> "replaceDamagedLimit";
  int n = (par >> "replaceDamagedHitpoints").GetSize();
  _replaceDamagedHitpoints.Realloc(n); 
  _replaceDamagedHitpoints.Resize(n); 
  for (int i=0; i<n; i++)
  {
    _replaceDamagedHitpoints[i] = (par >> "replaceDamagedHitpoints")[i];
  }
}

void BuildingType::Load(ParamEntryPar par, const char *shape)
{
  LoadExt(par,shape,true);
}

bool BuildingType::IsPersistenceRequired() const
{
  // TODO: no persistence is needed because of lights
  return IsSupply() || _lightsExists || IsSoundRequired();
}

float BuildingType::GetFeatureSize(Vector3Par pos) const
{
  // first check config
  float ret = base::GetFeatureSize(pos);
  if (ret>0) return ret;
  // when there is no config, attempt autodetection
  // if it is a feature, it must have a light - otherwise it is not tall enough to be significant
  if (_lightsExists)
  {
    // decide based on above sea level - being elevated increases the chance of becoming a feature?
    // include also on-shore or in-water features
    if (pos.Y()>300 || pos.Y()<5) return 1.0f; // use small value - real size will be used for autodetected features
  }
  return ret;
}

bool BuildingType::IsSimulationRequired() const
{
  return IsSupply() || _lightsExists || _windSockExist;
}
bool BuildingType::IsSoundRequired() const
{
  return false;
}

void BuildingType::InitShape()
{
  base::InitShape();
  if( !_shape ) return;

  _exits.Clear();
  _positions.Clear();

  const Shape *paths = _shape->PathsLevel();
  if (paths)
  {
    char buffer[256];
    for (int i=1; ; i++)
    {
      sprintf(buffer, "In%d", i);
      int index = _shape->PointIndex(paths,buffer);
      if (index < 0) break;
      int pt = paths->VertexToPoint(index);
      DoAssert(pt >= 0);
      DoAssert( paths->PointToVertex(pt)>=0 );
      _exits.Add(pt);
    }
    for (int i=1; ; i++)
    {
      sprintf(buffer, "Pos%d", i);
      int index = _shape->PointIndex(paths,buffer);
      if (index < 0) break;
      int pt = paths->VertexToPoint(index);
      DoAssert(pt >= 0);
      DoAssert( paths->PointToVertex(pt)>=0 );
      _positions.Add(pt);
    }
    _points.Resize(0);
    for (Offset f=paths->BeginFaces(), e=paths->EndFaces(); f<e; paths->NextFace(f))
    {
      const Poly &face=paths->Face(f);
      int n = face.N();
      if (n < 2) continue;
      for (int v=0; v<n; v++)
      {
        int index = paths->VertexToPoint(face.GetVertex(v));
        DoAssert(index >= 0);
        DoAssert( paths->PointToVertex(index)>=0 );
        if (_points.Size() <= index) _points.Resize(index + 1);
        BuildingPointInfo &point = _points[index];
        if (v == 0)
        {
          int pindex = paths->VertexToPoint(face.GetVertex(n - 1));
          DoAssert( pindex>=0 );
          DoAssert( paths->PointToVertex(pindex)>=0 );
          point._connections.AddUnique(pindex);
        }
        else
        {
          int pindex = paths->VertexToPoint(face.GetVertex(v - 1));
          DoAssert( pindex>=0 );
          DoAssert( paths->PointToVertex(pindex)>=0 );
          point._connections.AddUnique(pindex);
        }
        if (v == n - 1)
        {
          int pindex = paths->VertexToPoint(face.GetVertex(0));
          DoAssert( pindex>=0 );
          DoAssert( paths->PointToVertex(pindex)>=0 );
          point._connections.AddUnique(pindex);
        }
        else
        {
          int pindex = paths->VertexToPoint(face.GetVertex(v+1));
          DoAssert( pindex>=0 );
          DoAssert( paths->PointToVertex(pindex)>=0 );
          point._connections.AddUnique(pindex);
        }
      }
    }
  }

  const ParamEntry &par = (*_par);

  const Shape *mem = _shape->MemoryLevel();
  _ladders.Clear();
  if (mem)
  {
    ParamEntryVal ladders = par>>"ladders";
    for (int i=0; i<ladders.GetSize(); i++)
    {
#if _VBS3
      if(!ladders[i].IsArrayValue())
      {
        RptF("Error: in class:%s, expected array but found:%s",par.GetName().Data(),ladders[i].GetValueRaw().Data());
        // break out of loop nothing more to do.
        break;
      }
#endif

      RStringB ladderBottomName = ladders[i][0];
      RStringB ladderTopName = ladders[i][1];
      int ladderTop = _shape->PointIndex(mem, ladderTopName);
      if (ladderTop < 0)
      {
        RptF("Error: selection %s missing in memory LOD of model %s", cc_cast(ladderTopName), cc_cast(_shape->GetName()));
        continue;
      }
      int ladderBottom = _shape->PointIndex(mem, ladderBottomName);
      if (ladderBottom < 0)
      {
        RptF("Error: selection %s missing in memory LOD of model %s", cc_cast(ladderBottomName), cc_cast(_shape->GetName()));
        continue;
      }
      Ladder &ladder = _ladders.Append();
      ladder._top = ladderTop; 
      ladder._bottom = ladderBottom;

      if (paths)
      {
        // try to add ladder to paths
        Vector3 memTopPos = mem->Pos(ladder._top);
        Vector3 memBottomPos = mem->Pos(ladder._bottom);
        Vector3 pathTopPos, pathBottomPos;
        int pathTop = FindNearestPoint(memTopPos, pathTopPos);
        int pathBottom = FindNearestPoint(memBottomPos, pathBottomPos);
        if (pathTop >= 0 && pathBottom >= 0)
        {
          const float prec2 = Square(1.0f);
/*
LogF("Building %s, ladder %d", (const char *)_shape->GetName(), i);
LogF(" - top precision %.3f", pathTopPos.Distance2(memTopPos));
LogF(" - bottom precision %.3f", pathBottomPos.Distance2(memBottomPos));
*/
          if (pathTopPos.Distance2(memTopPos) <= prec2 && pathBottomPos.Distance2(memBottomPos) <= prec2)
          {          
            BuildingPointInfo &pointTop = _points[pathTop];
            pointTop._connections.Add(pathBottom);
            Assert(!pointTop._action);
            pointTop._action = new PathActionLadderTop(i);

            BuildingPointInfo &pointBottom = _points[pathBottom];
            pointBottom._connections.Add(pathTop);
            Assert(!pointBottom._action);
            pointBottom._action = new PathActionLadderBottom(i);
          }
        }
      }
    }
  }

  if (paths)
  {
    BString<128> buffer;
    for (int i=1; ; i++)
    {
      sprintf(buffer, "ActionBegin%d", i);
      int index = _shape->FindNamedSel(paths,buffer);
      if (index < 0) break;

      RString actionName;
      ConstParamEntryPtr entry = par.FindEntry(buffer);
      if (entry) actionName = *entry;

      const NamedSelection::Access sel(paths->NamedSel(index),_shape,paths);
      for (int j=0; j<sel.Size(); j++)
      {
        int vx = sel[j];
        int pt = paths->VertexToPoint(vx);
        DoAssert(pt >= 0);
        DoAssert(paths->PointToVertex(pt) >= 0);

        BuildingPointInfo &point = _points[pt];
        if (point._action)
          RptF("Warning: Unaccessible ladder point for AI (already used for %s) in %s",cc_cast(sel.GetName()), cc_cast(_shape->GetName()));

        point._action = new PathUserActionBegin(actionName);
      }
    }
    for (int i=1; ; i++)
    {
      sprintf(buffer, "ActionEnd%d", i);
      int index = _shape->FindNamedSel(paths,buffer);
      if (index < 0) break;

      RString actionName;
      ConstParamEntryPtr entry = par.FindEntry(buffer);
      if (entry) actionName = *entry;

      const NamedSelection::Access sel(paths->NamedSel(index),_shape,paths);
      for (int j=0; j<sel.Size(); j++)
      {
        int vx = sel[j];
        int pt = paths->VertexToPoint(vx);
        DoAssert(pt >= 0);
        DoAssert(paths->PointToVertex(pt) >= 0);

        BuildingPointInfo &point = _points[pt];
        if (point._action)
          RptF("Warning: Unaccessible ladder point for AI (already used for %s) in %s",cc_cast(sel.GetName()), cc_cast(_shape->GetName()));

        point._action = new PathUserActionEnd(actionName);
      }
    }
  }

  _ladders.Compact();
  _exits.Compact();
  _positions.Compact();

  // compact _points array
  _points.Compact();
  for (int i=0; i<_points.Size(); i++) _points[i]._connections.Compact();

  ConstParamEntryPtr entry = par.FindEntry("HouseNumbers");
  if (entry) _houseNumbers.Init(_shape, *entry, GetFontID(Pars >> "fontPlate"));
  else _houseNumbers.Init(_shape); // only dim the array
}


void BuildingType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);

  _houseNumbers.InitLevel(_shape, level);
}

void BuildingType::DeinitShapeLevel(int level)
{
  base::DeinitShapeLevel(level);

  _houseNumbers.DeinitLevel(_shape, level);
}



Object* BuildingType::CreateObject(bool unused) const
{
  Object* v;
  
  if (!stricmp(_simName,"fire")) v = new Fireplace(this);
  else if(!stricmp(_simName,"house") || !stricmp(_simName,"breakablehouseanimated")) v = new Building(this, VISITOR_NO_ID, unused);
  else 
  {
    LogF("Unknown vehicle simulation %s: type %s (BuildingType called)",
      cc_cast(_simName), cc_cast(GetName())
      );
    v = NULL;
  }

  return v;
}



Vector3Val BuildingType::GetPosition(int index) const
{
  Assert(_shape);
  const Shape *paths = _shape->PathsLevel();
  Assert(paths);
  // hot-fix: prevent crash with wrong index
  // was implemented during ArmA Release Candidate testing
  saturate(index,0,paths->NPoints()-1);
  int vertex = paths->PointToVertex(index);
  saturate(vertex,0,paths->NVertex()-1);
  return paths->Pos(vertex);
}

PathAction *BuildingType::GetPathAction(int index) const
{
  return _points[index]._action;
}

const FindArray<int> &BuildingType::GetConnections(int index) const
{
  return _points[index]._connections;
}

int BuildingType::FindNearestExit(Vector3Par pos, Vector3 &ret) const
{
  int best = -1;
  float dist2Min = FLT_MAX;
  for (int i=0; i<_exits.Size(); i++)
  {
    Vector3Val pt = GetPosition(_exits[i]);
    float dist2 = pt.Distance2(pos);
    if (dist2 < dist2Min)
    {
      dist2Min = dist2;
      best = _exits[i];
      ret = pt;
    }
  }
  return best;
}

int BuildingType::FindNearestPosition(Vector3Par pos, Vector3 &ret) const
{
  int best = -1;
  float dist2Min = FLT_MAX;
  for (int i=0; i<_positions.Size(); i++)
  {
    Vector3Val pt = GetPosition(_positions[i]);
    float dist2 = pt.Distance2(pos);
    if (dist2 < dist2Min)
    {
      dist2Min = dist2;
      best = _positions[i];
      ret = pt;
    }
  }
  return best;
}

int BuildingType::FindNearestPoint(Vector3Par pos, Vector3 &ret) const
{
  int best = -1;
  float dist2Min = FLT_MAX;
  for (int i=0; i<_points.Size(); i++)
  {
    Vector3Val pt = GetPosition(i);
    float dist2 = pt.Distance2(pos);
    if (dist2 < dist2Min)
    {
      dist2Min = dist2;
      best = i;
      ret = pt;
    }
  }
  return best;
}

Vector3 IPaths::GetPosition(int index) const
{
  // based on BuildingType::GetPosition
  const Object *object = GetObject();
  int paths = object->GetShape()->FindPaths();
  Assert (paths>=0);
  const Shape *pathsLevel = object->GetShape()->PathsLevel();
  int vIndex = pathsLevel->PointToVertex(index);
  Vector3Val pos = object->AnimatePoint(object->FutureVisualState(),paths,vIndex);
  return object->FutureVisualState().PositionModelToWorld(pos);
}

const FindArray<int> &IPaths::GetConnections(int index) const
{
  const BuildingType *type = GetBType();
  return type->GetConnections(index);
}

int IPaths::FindNearestExit(Vector3Par pos, Vector3 &ret) const
{
  Vector3 posModel, retModel;

  const Object *object = GetObject();
  Matrix4Val invTrans = object->FutureVisualState().GetInvTransform();
  posModel = invTrans.FastTransform(pos);

  const BuildingType *type = GetBType();
  int index = type->FindNearestExit(posModel, retModel);
  if (index < 0)
  {
    ret = VZero;
  }
  else
  {
    int paths = object->GetShape()->FindPaths();
    int vIndex = object->GetShape()->PathsLevel()->PointToVertex(index);
    ret = object->FutureVisualState().PositionModelToWorld(object->AnimatePoint(object->FutureVisualState(),paths,vIndex));
  }
  return index;
}

int IPaths::FindNearestPosition(Vector3Par pos, Vector3 &ret) const
{
  Vector3 posModel, retModel;

  const Object *object = GetObject();
  Matrix4Val invTrans = object->FutureVisualState().GetInvTransform();
  posModel = invTrans.FastTransform(pos);

  const BuildingType *type = GetBType();
  int index = type->FindNearestPosition(posModel, retModel);
  if (index < 0)
  {
    ret = VZero;
  }
  else
  {
    int paths = object->GetShape()->FindPaths();
    int vIndex = object->GetShape()->PathsLevel()->PointToVertex(index);
    ret = object->FutureVisualState().PositionModelToWorld(object->AnimatePoint(object->FutureVisualState(),paths,vIndex));
  }
  return index;
}

int IPaths::FindNearestPoint(Vector3Par pos, Vector3 &ret, float maxDist2) const
{
  Vector3 posModel, retModel;
  const Object *object = GetObject();
  Matrix4Val invTrans = object->FutureVisualState().GetInvTransform();
  posModel = invTrans.FastTransform(pos);

  const BuildingType *type = GetBType();

  int paths = object->GetShape()->FindPaths();
  const Shape *pathsLevel = object->GetShape()->PathsLevel();

  int index=-1;
///

  int best1 = -1, best2 = -1;
  float dist2Min = maxDist2;
  int n = type->_points.Size();
  for (int i=0; i<n; i++)
  {
    // note: i is "point" index
    // specific point index need not be used
    int vIndex = pathsLevel->PointToVertex(i);
    if (vIndex<0) continue;
    const FindArray<int> &array = type->_points[i]._connections;
    int m = array.Size();
    if (m<=0) continue;
    Vector3 b = object->FutureVisualState().PositionModelToWorld(object->AnimatePoint(object->FutureVisualState(),paths,vIndex));
    Vector3 p = pos - b;
    for (int j=0; j<m; j++)
    {
      int index = array[j];
      Vector3 e = GetPosition(index) - b;
      float eLen2 = e.SquareSize();
      Vector3 nearest;
      if (eLen2>0)
      {
        float t = (e * p) / eLen2;
        saturate(t, 0, 1);
        nearest = b + t * e;
      
      }
      else
      {
        nearest = b;
      }
      
      float dist2 = nearest.Distance2(pos);
      if (dist2 < dist2Min)
      {
        dist2Min = dist2;
        best1 = i;
        best2 = index;
      }
    }
  }
  if (best1 < 0 || best2 < 0) return -1;
  Vector3 nearest1 = GetPosition(best1);
  Vector3 nearest2 = GetPosition(best2);
  if (nearest2.Distance2(pos) < nearest1.Distance2(pos))
  {
    ret = nearest2;
    index = best2;
  }
  else
  {
    ret = nearest1;
    index = best1;
  }

///

  //int index = type->FindNearestPoint(posModel, retModel,maxDist2);
  if (index < 0)
  {
    ret = VZero;
  }
  else
  {
    int vIndex = pathsLevel->PointToVertex(index);
    int paths = object->GetShape()->FindPaths();
    ret = object->FutureVisualState().PositionModelToWorld(object->AnimatePoint(object->FutureVisualState(),paths,vIndex));
  }
  return index;
}

DEFINE_CASTING(Building)

Building::Building( const EntityAIType *name, const CreateObjectId &id, bool fullCreate)
:base(name, fullCreate)
{
  float precision = 0.5;
  if (GetType()->_lights.SomeLights() || Type()->_windSockExist)
  {
    // we need to update lights position
    // simulate each frame (buildings are never simulated more frequently)
    // TODO (Rrrola): this is a hack and will be sacrificed to the Blood God as such
    precision = 0.03f;
  }
  SetSimulationPrecision(precision);
  RandomizeSimulationTime();
  SetTimeOffset(0);

  Assert( _shape );
  _destrType=GetType()->GetDestructType();
  if( _destrType==DestructDefault ) _destrType=DestructBuilding;
  _static=true;
  _animatorRunning = false;
  _disableOnSurface = false;
  SetType(Primary);
  SetID(id);

  int n = NPos();
  _locks.Resize(n);
  for (int i=0; i<n; i++) _locks[i] = 0;

}

Matrix4 Building::InsideCamera( CameraType camType ) const
{
  return MIdentity;
}

int Building::InsideLOD( CameraType camType ) const
{
  return 0;
}

void Building::ExplodeSmoke()
{
  // simulate what Building::Destroy did where the object was local
  IExplosion *explosion = GetSmoke();
  if (explosion)
  {
    explosion->Explode();
  }
}

/*!
\patch 1.45 Date 2/23/2002 by Ondra
- Fixed: Fuel stations now explode when destroyed.
*/
void Building::Destroy( EntityAI *owner, float overkill, float minExp, float maxExp )
{
  base::Destroy(owner,overkill,minExp,maxExp);

  if (_damagedBuilding)
  {
    _damagedBuilding->Destroy(owner,overkill,minExp,maxExp);
  }

  IExplosion *smoke = GetSmoke();
  if( smoke )
  {
    smoke->Explode();
  }
  NeverDestroy();
}

const EntityType *Building::CanDestroyUsingEffects() const
{
  const EntityType *entityType = base::CanDestroyUsingEffects();
  if (entityType)
  {
    // if there is a replacement, we need to destroy the replacement only
    if (_damagedBuilding)
      return NULL;
  }
  return entityType;
}

void Building::ReplaceByDamaged()
{
  // destroy the animator
  if (_replaceByDamaged) _replaceByDamaged->SetDelete();
  _replaceByDamaged = NULL;

  // create the replacement
  _damagedBuilding = GWorld->NewVehicleWithID(Type()->_replaceDamaged);
  if (_damagedBuilding)
  {
    // inherit all needed parameters from myself

    // adjust bounding center to align properly
    Vector3 bCenter = FutureVisualState().PositionModelToWorld(-GetShape()->BoundingCenter());
    Matrix4 trans = FutureVisualState().Transform();
    trans.SetPosition(bCenter + FutureVisualState().DirectionModelToWorld(_damagedBuilding->GetShape()->BoundingCenter()));
    _damagedBuilding->SetTransform(trans);

    _damagedBuilding->SetTotalDamage(GetRawTotalDamage(), false);
    // check and copy state of hitpoints
    const HitPointList &srcList = Type()->GetHitPoints();
    for (int i=0; i<srcList.Size(); i++)
    {
      RString name = srcList[i].GetName();
      int index = _damagedBuilding->Type()->FindHitPoint(name);
      if (index >= 0) _damagedBuilding->ChangeHit(NULL, index, _hit[i], false);
      else
      {
        RptF("Error: Hitpoint %s not found in the model %s (original model %s)",
          cc_cast(name), cc_cast(_damagedBuilding->GetShape()->GetName()), cc_cast(GetShape()->GetName()));
      }
    }
    // for sure, check also the second direction
#if _ENABLE_CHEATS
    const HitPointList &dstList = _damagedBuilding->Type()->GetHitPoints();
    for (int i=0; i<dstList.Size(); i++)
    {
      RString name = dstList[i].GetName();
      int index = Type()->FindHitPoint(name);
      if (index < 0)
      {
        RptF("Error: Hitpoint %s not found in the model %s (target model %s)",
          cc_cast(name), cc_cast(GetShape()->GetName()), cc_cast(_damagedBuilding->GetShape()->GetName()));
      }
    }
#endif
    _damagedBuilding->SetPilotLight(IsPilotLight());
    _damagedBuilding->SetCollisionLight(IsCollisionLight());
    // the state of user animations
    for (int i=0; i<Type()->_animSources.Size(); i++)
    {
      const AnimationSource *src = Type()->_animSources[i];
      int srcState = src->GetStateIndex();
      if (srcState < 0) continue; // not a user animation
      RString name = src->GetName();

      int index = _damagedBuilding->Type()->_animSources.FindName(name);
      if (index < 0) continue; // animation not found in a target object
      const AnimationSource *dst = _damagedBuilding->Type()->_animSources[index];
      int dstState = dst->GetStateIndex();
      if (dstState >= 0)
      {
        // corresponding user animation found, copy the state
        _damagedBuilding->SetAnimationInstancePhase(dstState, GetAnimationInstancePhase(FutureVisualState(), srcState));
        _damagedBuilding->SetAnimationInstancePhaseWanted(dstState, GetAnimationInstancePhaseWanted(srcState));
      }
    }
    // add the building to the world
    GWorld->AddSlowVehicle(_damagedBuilding);
    GWorld->OnEntityMovedFar(_damagedBuilding);
    GetNetworkManager().CreateVehicle(_damagedBuilding, VLTSlowVehicle, RString(), -1);

    // hide the current building
    SetDestroyed(1.0f);
    BroadcastDamage(true);
  }
}

void Building::BroadcastDamage(bool immediate)
{
  // regular update for non-static objects
  GetNetworkManager().UpdateObject(this);
}

/// helper class to preload destructible model and replace the original object when ready
class PreloadDamagedBuilding : public Entity
{
protected:
  OLink(Building) _owner;
  RString _typeName;
  Ref<EntityType> _type;

public:
  PreloadDamagedBuilding(Building *owner, RString typeName);

  virtual void Simulate(float deltaT, SimulationImportance prec);

  /// we do not want to serialize
  virtual bool MustBeSaved() const {return false;}
  /// we do not want to transfer over network
  virtual NetworkMessageType GetNMType(NetworkMessageClass cls) const {return NMTNone;}
};

PreloadDamagedBuilding::PreloadDamagedBuilding(Building *owner, RString typeName)
: Entity(NULL, VehicleTypes.New("#preloaddamagedbuilding"), CreateObjectId()), _owner(owner), _typeName(typeName)
{
}

void PreloadDamagedBuilding::Simulate(float deltaT, SimulationImportance prec)
{
  if (!_owner)
  {
    // nobody is waiting for the result
    SetDelete();
    return;
  }

  // preload the type
  if (!_type)
  {
    if (!VehicleTypes.Request(_typeName)) return;
    _type = VehicleTypes.New(_typeName);
    if (!_type)
    {
      RptF("Cannot create the destructible building of type %s", cc_cast(_typeName));
      SetDelete();
      return;
    }
  }

  // preload the shape
  FileRequestPriority prior = FileRequestPriority(1000);
  if (!_type->PreloadShape(prior)) return;
  
  // all preloaded, object can be created
  _owner->ReplaceByDamaged();
}

void Building::ReactToDamage()
{
  _isDead = GetRawTotalDamage() >= 1.0f;
  const BuildingType *buildingType = Type();

  if (!_damagedBuilding && !_replaceByDamaged && (buildingType->_replaceDamaged.GetLength() > 0))
  {
    // only one client can create a damaged house:
    // - me in SP
    // - owner for regular network objects
    // - server for shared (static) network objects
    bool local = true;
    int player = GetNetworkManager().GetPlayer();
    if (player != 0)
    { // MP
      CreatorId creator = GetNetworkId().creator;
      // if shared (static) network object, check for BOT_CLIENT, otherwise it is regular network object
      local = (creator == CreatorId(STATIC_OBJECT)) ? (player == 2) : IsLocal();
    }

    // if already dead, do not create the damaged building, as it would have to die immiediately anyway
    if (local)
    {
      if (!_isDead)
      {
        float limit = buildingType->_replaceDamagedLimit;
        // check all hitpoints listed in the config
        for (int h = 0; h < buildingType->_replaceDamagedHitpoints.Size(); h++)
        {
          RString hitpointName = buildingType->_replaceDamagedHitpoints[h];
          int hitpointIndex = buildingType->FindHitPoint(hitpointName);
          if (hitpointIndex < 0)
          {
            RptF("Error: Hitpoint %s listed in replaceDamagedHitpoints not found in the model %s", cc_cast(hitpointName), cc_cast(GetShape()->GetName()));
            continue;
          }
          // if any of these hitpoints is damaged enough, replace the building
          if (_hit[hitpointIndex] >= limit)
          {
            _replaceByDamaged = new PreloadDamagedBuilding(this, buildingType->_replaceDamaged);
            _replaceByDamaged->SetPosition(FutureVisualState().Position());
            GWorld->AddAnimal(_replaceByDamaged);
            break;
          }
        }
      }
      else
      {
        // normally PreloadDamagedBuilding calls ReplaceByDamaged, which then calls BroadcastDamage
        BroadcastDamage(false);
      }
    }
  }
  

  base::ReactToDamage();
}

bool Building::SimulationReady(SimulationImportance prec) const
{
  return true;
}

void Building::OnAnimationStarted()
{
  if (!_animatorRunning && IsAnimatorRequired())
  {
    // this building has no simulation - it is not listed in the vehicles lists
    // we need to make sure it is simulated
    StartAnimator();
    _animatorRunning=true;
  }
}

void Building::OnAnimatorTerminated()
{
  _animatorRunning = false;
}

void Building::Simulate( float deltaT, SimulationImportance prec )
{
  PROFILE_SCOPE_EX(simBu,sim)
  _pilotLight = !IsDamageDestroyed();
  _collisionLight = !IsDamageDestroyed();
  base::Simulate(deltaT,prec);

  SimulatePost(deltaT,prec);
}

void Building::SimulatePost(float deltaT, SimulationImportance prec)
{
  if (Type()->_windSockExist && prec<=SimulateVisibleFar)
  {
    int level = 0;
    ShapeUsed lock = _shape->Level(level);
    for (int i=GetType()->GetProxyCountForLevel(level)-1; i>=0; --i)
    {
      const ProxyObjectTyped &proxy = GetType()->GetProxyForLevel(level,i);
      WindSock *windSock = dyn_cast<WindSock>(proxy.obj.GetRef());
      if (windSock)
      {
        SkeletonIndex boneIndex = GetType()->GetProxyBoneForLevel(level,i);
        Matrix4 proxyTransform = AnimateProxyMatrix(RenderVisualState(), level,proxy.GetTransform(),boneIndex);

        Vector3 wind = GLandscape->GetWindSpeed();
        wind[1] = 0.0f;

        Matrix4 pTransform = FutureVisualState().Transform();
        pTransform.SetUpAndAside(pTransform.DirectionUp(),wind);
        pTransform= pTransform* proxyTransform;

        if (_windSocks.Size()<proxy.id+1)
          _windSocks.Resize(proxy.id+1);

        if (!_windSocks[proxy.id])
        {
          RString shapeName = ::GetShapeName(proxy.name);
          // copy flag - create a new one with the same type, but different shape
          Ref<Entity> newWindSock = GWorld->NewNonAIVehicleWithID(windSock->GetNonAIType()->GetName(), shapeName);
          _windSocks[proxy.id] = dyn_cast<WindSock,Entity>(newWindSock);
          if (!_windSocks[proxy.id])
          {
            RptF("No flag - %s,%s,%s",(const char *)newWindSock->GetDebugName(),(const char *)shapeName,
              (const char *)windSock->GetNonAIType()->GetName());
            Fail("No flag");
          }
        }

        // proxy may be animation
        if (_windSocks[proxy.id] && _windSocks[proxy.id]->InitAsync(pTransform))
        {
          _windSocks[proxy.id]->SetTransformPossiblySkewed(pTransform);
          _windSocks[proxy.id]->FlagSimulate(pTransform,deltaT, prec);
        }
      }
    }
  }
  base::SimulatePost(deltaT,prec);
}

bool Building::IsAnimatorRequired() const
{
  // use animator only for "slow" buildings
  return (SimulationPrecision() > 0.0f);
}

void Building::ResetStatus()
{
  base::ResetStatus();
  _replaceByDamaged = NULL;
  _damagedBuilding = NULL;
  _animatorRunning = false; // animator will no longer be running, as all entities are destroyed in Landscape::ResetState
}

bool Building::MustBeSaved() const
{
  if (Object::MustBeSaved())
    return true;
  return _supply || _userActions.Size() > 0 ||
    _activities.Size() > 0 || FutureVisualState()._animationStates.Size() > 0 || _damagedBuilding;
}

LSError Building::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.SerializeRef("DamagedBuilding", _damagedBuilding, 1))
  if (ar.IsLoading()) _replaceByDamaged = NULL;
  return LSOK;
}

bool Building::CanBeInstanced(int level, float distance2, const DrawParameters &dp) const
{
  if (Type()->_showWeaponCargo)
  {
    return false;
  }
  // skip Entity proxy instancing detection
  return ObjectTyped::CanBeInstanced(level, distance2, dp);
}

bool Building::CanBeInstancedShadow(int level) const
{
  if (Type()->_showWeaponCargo)
  {
    return false;
  }
  // skip Entity proxy instancing detection
  return ObjectTyped::CanBeInstancedShadow(level);
}

AnimationStyle Building::IsAnimated( int level ) const
{
  AnimationStyle anim = Entity::IsAnimated(level);
  // TODO: handle somehow in EntityAI
  AnimationStyle animHide = Type()->_hiddenSelections.Size()>0 ? AnimatedTextures : NotAnimated;
  return anim>animHide ? anim : animHide;
}

void Building::Draw(
  int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
  const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
  , int drawListId
#endif
  )
{
#if _HELISIM_RENDER_TO_TEXTURE
  base::Draw(cb,level, clipFlags, dp, ip, dist2, pos, oi, drawListId);
#else
  base::Draw(cb,level, clipFlags, dp, ip, dist2, pos, oi);
#endif
  Type()->_houseNumbers.Draw(cb,level, clipFlags, pos, _houseNumber);
}

void Building::PrepareProxiesForDrawing(Object *rootObj, int rootLevel, const AnimationContext &animContext,
  const PositionRender &transform, int level, ClipFlags clipFlags, SortObject *so)
{
  if (Type()->_showWeaponCargo || Type()->_windSockExist)
  {
    const VisualState &vs = RenderVisualState();
    float dist2 = so->distance2;
    int nProxies = Type()->GetProxyCountForLevel(level);

    // iterator for going through cargo
    bool magazines = false;
    int index = 0;

    // convert shape proxies to my proxies
    for (int i=0; i<nProxies; i++)
    {
      const ProxyObjectTyped &proxy = Type()->GetProxyForLevel(level, i);
      Object *obj = proxy.obj;
      const VehicleNonAIType *type = obj->GetEntityType();
      if (!type) continue;

      if (Type()->_windSockExist)
      {
        WindSock *windSock = dyn_cast<WindSock>(proxy.obj.GetRef());
        if (windSock)
        {
          LODShape *lodShape = windSock->GetShape();

          SkeletonIndex boneIndex = Type()->GetProxyBoneForLevel(level, i);
          Matrix4 proxyTransform = AnimateProxyMatrix(RenderVisualState(), level, proxy.GetTransform(), boneIndex);

          Vector3 wind = GLandscape->GetWindSpeed();
          wind[1] = 0.0f;

          Matrix4 pTransform = transform.position;
          pTransform.SetUpAndAside(pTransform.DirectionUp(),wind);
          pTransform= pTransform* proxyTransform;

          int pLevel = GScene->LevelFromDistance2(lodShape, dist2, pTransform.Scale());
          if (_windSocks.Size()>proxy.id && _windSocks[proxy.id] && pLevel != LOD_INVISIBLE)
          {
            _windSocks[proxy.id]->SetTransform(pTransform); // store the matrix, it is needed from WindSock::Animate
            GScene->ProxyForDrawing(_windSocks[proxy.id], rootObj, pLevel, rootLevel, 0, dist2, pTransform);
          }
        }
      }
      else if (Type()->_showWeaponCargo)
      {
        RString simulation = type->_simName;
        if (!stricmp(simulation, "proxyweapon") || !stricmp(simulation, "proxysecweapon"))
        {
          // select shape (use the iterator)
          LODShape *pshape = NULL;
          const Weapon *weapon = NULL;
          const Magazine *magazine = NULL;
          if (!magazines)
          {
            int skipped = 0; // weapons counter
            ForEachWeaponCargo([&skipped, &index, &pshape, &weapon](const Weapon *weaponI) -> bool
            {
              // find the weapon # index
              if (skipped < index)
              {
                skipped++;
                return false; // continue
              }
              // weapon found
              pshape = weaponI->GetShape();
              index++;
              if (!pshape) return false; // continue - get other one
              weapon = weaponI;
              return true; // done
            });

            if (!pshape)
            {
              // no more weapons found, go to magazines
              magazines = true;
              index = 0;
            }
          }
          if (magazines)
          {
            int skipped = 0; // magazines counter
            ForEachMagazineCargo([&skipped, &index, &pshape, &magazine](const Magazine *magazineI) -> bool
            {
              // find the magazine # index
              if (skipped < index)
              {
                skipped++;
                return false; // continue
              }
              // magazine found
              pshape = magazineI->GetShape();
              index++;
              if (!pshape) return false; // continue - get other one
              magazine = magazineI;
              return true; // done
            });
            }
          if (pshape)
          {
            SkeletonIndex boneIndex = Type()->GetProxyBoneForLevel(level,i);
            Matrix4 proxyTransform = obj->FutureVisualState().Transform();

            AnimateBoneMatrix(proxyTransform, vs, level, boneIndex);

            // smart clipping par of obj->Draw
            Matrix4Val pTransform = transform * proxyTransform;

            // LOD detection
            int level = GScene->LevelFromDistance2(
              pshape, dist2, pTransform.Scale()
              );
            if (level != LOD_INVISIBLE)
            {
              if (weapon)
              {
                unconst_cast(weapon)->SetAnimPhase(-1);
                GScene->ProxyForDrawing(unconst_cast(weapon), rootObj, level, rootLevel, clipFlags, dist2, pTransform);
              }
              else if (magazine)
              {
                GScene->ProxyForDrawing(unconst_cast(magazine), rootObj, level, rootLevel, clipFlags, dist2, pTransform);
              }
            }
          }
        }
      }
    }
  }
  else
  {
    ObjectTyped::PrepareProxiesForDrawing(rootObj, rootLevel, animContext, transform,level,clipFlags,so);
  }
}

int Building::GetProxyComplexity(int level, const Matrix4 &pos, float dist2) const
{
  return Object::GetProxyComplexity(level,pos,dist2);
}

void Building::DrawDiags()
{
  base::DrawDiags();
  if (CHECK_DIAG(DEPath))
  {
    // draw diagnostics only for building near to camera
    float distance = FutureVisualState().Position().Distance(GScene->GetCamera()->Position());
    if (distance <= 50.0f + _boundingSphere)
    {
      #define DRAW_OBJ(obj) GScene->DrawObject(obj)
      //GScene->DrawCollisionStar(Position(),3);

      // draw all exits
      if (NExits() > 0 || NPos() > 0)
      {
        LODShape *shape=GScene->Preloaded(SphereModel);
        for (int i=0; i<NExits(); i++)
        {
        Vector3 pos = IPaths::GetPosition(GetExit(i));
          Ref<Object> obj = new ObjectColored(shape, VISITOR_NO_ID);
          float y = GLandscape->SurfaceY(pos.X(), pos.Z());

          obj->SetConstantColor(PackedColor(Color(1,1,0,1)));
          if (fabs(y - pos.Y()) > 0.5)
          {
            obj->SetConstantColor(PackedColor(Color(1,0,0,1)));
            if (pos.Y()<y) pos = Vector3(pos.X(),y,pos.Z());
          }
          else 
          {
            LandIndex xLand(toIntFloor(pos.X() * InvLandGrid));
            LandIndex zLand(toIntFloor(pos.Z() * InvLandGrid));

            int x = toIntFloor(pos.X() * InvOperItemGrid);
            int z = toIntFloor(pos.Z() * InvOperItemGrid);
            int xx = x / OperItemRange;
            int zz = z / OperItemRange;
            Ref<OperField> fld = GLOB_LAND->OperationalCache()->GetOperField(xLand, zLand, MASK_AVOID_OBJECTS | MASK_USE_BUFFER);
            if (fld)
            {
              const OperItem &item = fld->GetItem((OperFieldIndex)(x - xx * OperItemRange),(OperFieldIndex)(z - zz * OperItemRange));

              OperItemType _itemType =  item.GetTypeSoldier();
              if (_itemType == OITSpaceHardBush || _itemType == OITSpaceTree ||_itemType == OITSpace ||_itemType == OITSpaceRoad)
              {
                obj->SetConstantColor(PackedColor(Color(0.5,0,1,1)));
                if (pos.Y()<y) pos = Vector3(pos.X(),y,pos.Z());
              }
            }
          }

          obj->SetPosition(pos);
          obj->SetScale(0.1f);
          DRAW_OBJ(obj);
        }

        for (int i=0; i<NPos(); i++)
        {
        Vector3Val pos = IPaths::GetPosition(IPaths::GetPos(i));
          Ref<Object> obj = new ObjectColored(shape, VISITOR_NO_ID);
          obj->SetPosition(pos);
          obj->SetScale(0.1f);
          if (IsLocked(i))
            obj->SetConstantColor(PackedColor(Color(1,0.5,0.5,0.5)));
          else
            obj->SetConstantColor(PackedColor(Color(0,1,0,1)));
          DRAW_OBJ(obj);
        }
      }
      // draw lines from all instead of all connections
      const Shape *paths = _shape->PathsLevel();
      if (paths)
      {
        LODShape *shape=GScene->Preloaded(SphereModel);
        for (int i=0; i<paths->NPos(); i++)
        {
          Vector3Val pos = FutureVisualState().PositionModelToWorld(paths->Pos(i));
          Ref<Object> obj = new ObjectColored(shape, VISITOR_NO_ID);
          obj->SetPosition(pos);
          obj->SetScale(0.025f);
          obj->SetConstantColor(PackedColor(Color(0,0.5,0,1)));
          DRAW_OBJ(obj);
        }

  #if _HELISIM_RENDER_TO_TEXTURE
      DoAssert(CheckMainThread());
  #endif
      Draw(-1, _shape->FindPaths(), ClipAll, DrawParameters::_default, InstanceParameters::_default, 0.1f, GetFrameBase(), NULL
  #if _HELISIM_RENDER_TO_TEXTURE
        , 0
  #endif
        );
    }
  }
#if 0
  {
    // OBB & AABB box diag
    LODShape *shape=GScene->Preloaded(RectangleModel);
    LODShape *currShape = GetShape();

    if (currShape)
    {
      // OBB & AABB box diag
      LODShape *shape=GScene->Preloaded(RectangleModel);
      LODShape *currShape = GetShape();

      if (currShape)
      {
        int geomLevel = currShape->FindGeometryLevel();
        ConvexComponents *cComponents = currShape->GetConvexComponents(geomLevel);

        for (int i = 0; i < cComponents->Size(); i++)
        {
          const Ref<ConvexComponent> &cc = cComponents->Get(i);
          if (cc.IsNull()) continue;

          const Vector3 *shMinMax = shape->MinMax();
          float sx1 = 1.0f / (shMinMax[1].X() - shape->GeometryCenter().X());
          float sz1 = 1.0f / (shMinMax[1].Z() - shape->GeometryCenter().Z());

          Ref<Object> obj = new ObjectColored(shape, VISITOR_NO_ID);
          Ref<Object> obj1 = new ObjectColored(shape, VISITOR_NO_ID);
          float sx, sz, angle;
          cc->GetOBB(sx, sz, angle);
          Matrix4 scale(MScale, sx1 * sx, 1.0f, sz1 * sz);
        
          Matrix4 rotate;
          rotate.SetIdentity();
          rotate.SetRotationY(angle);

          static float MaxDistance = 50.0f;
          CameraEffect *ce = GWorld->GetCameraEffect();
          if (ce)
          {
            float distance = ce->GetTransform().Position().Distance(Position());
            if (distance > MaxDistance) continue;
          }
          else
          {
            AIBrain *brain = GWorld->FocusOn();
            if (brain)
            {
              float distance = brain->Position().Distance(Position());
              if (distance > MaxDistance) continue;
            }
          }

          Matrix4 transl;
          transl.SetIdentity();
          transl.SetPosition(Vector3(cc->GetOBBCenter().X(), cc->GetCenter().Y(), cc->GetOBBCenter().Z()));
          obj->SetTransform(WorldTransform() * transl * rotate * scale);
          obj->SetConstantColor(PackedColor(Color(1.0f, 1.0f, 0.5f, 1.0f)));
          DRAW_OBJ(obj);

          const Vector3 *minMax = cc->MinMax();
          Matrix4 scale1(MScale, sx1 * (cc->GetCenter().X() - minMax->X()), 1.0f, sz1 * (cc->GetCenter().Z() - minMax->Z()));

          Matrix4 transl1;
          transl1.SetIdentity();
          transl1.SetPosition(cc->GetCenter() + Vector3(0.0f, -0.1f, 0.0f));

          obj1->SetTransform(WorldTransform() * transl1 * scale1);
          obj1->SetConstantColor(PackedColor(Color(1.0f, 0.0f, 0.5f, 1.0f)));
          DRAW_OBJ(obj1);
        }
      }
    } 

  #endif

    // draw lines instead of all connections
  }

}

bool Building::IsMoveTarget() const
{
  // any "big" building may be move target
  // this is trick to avoid ammo crates etc..
  return GetMass()>(10*1000);
}


Vector3 Building::GetLadderPos( int ladder, bool up )
{
  const BuildingType *type = Type();
  const Ladder &lad = type->GetLadder(ladder);
  int mem = _shape->FindMemoryLevel();
  return FutureVisualState().PositionModelToWorld(AnimatePoint(FutureVisualState(),mem,up ? lad._top : lad._bottom));
}

Vector3 Building::GetLadderForward(int ladderIndex)
{
  const BuildingType *type = Type();
  const Ladder &ladder = type->GetLadder(ladderIndex);
  int mem = GetShape()->FindMemoryLevel();
  Vector3 top = AnimatePoint(FutureVisualState(), mem, ladder._top);
  Vector3 bottom = AnimatePoint(FutureVisualState(), mem, ladder._bottom);
  Vector3 ladderDir = top - bottom;
  Vector3 ladderAside = ladderDir.CrossProduct(VUp).Normalized();
  if (ladderAside.SquareSize()<0.1f)
  {
    ladderAside = Vector3(1, 0, 0);
    RptF("Ladder in %s is vertical", GetShape()->Name());
  }
  return VUp.CrossProduct(ladderAside).Normalized();
}

bool Building::GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const
{
  switch (action->GetType())
  {
    case ATLadderOnUp:
    case ATLadderUp:
    case ATLadderOnDown:
    case ATLadderDown:
    case ATLadderOff:
      // No parameters
      return true;
  }
  return base::GetActionParams(params, action, unit);
}

void Building::PerformAction(const Action *action, AIBrain *unit)
{
  switch (action->GetType())
  {
    case ATLadderUp:
    case ATLadderDown:
    case ATLadderOnUp:
    case ATLadderOnDown:
    if (!IsDamageDestroyed())
    {
      Person *person = unit->GetPerson();
      //LogF
      //(
      //  "catch %s, %d %d",
      //  (const char *)FindEnumName(action.type),action.param,action.param2
      //);
      const ActionLadderPos *actionTyped = static_cast<const ActionLadderPos *>(action);
      person->CatchLadder(this, actionTyped->GetLadder(), actionTyped->GetLadderPos() != 0);
    }
    break;
    case ATLadderOff:
    {
      Person *person = unit->GetPerson();
      const ActionLadder *actionTyped = static_cast<const ActionLadder *>(action);
      person->DropLadder(this, actionTyped->GetLadder());
    }
    break;
    default:
      base::PerformAction(action,unit);
    break;
  }
}

void Building::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  // check if we can climb some ladder
  if (unit->IsFreeSoldier())
  {
    Person *person = unit->GetPerson();
    for (int i=0; i<Type()->_ladders.Size(); i++)
    {
      const Ladder &ladder = Type()->_ladders[i];
      if (person->IsOnLadder(this,i))
      {
        UIAction action(new ActionLadder(ATLadderOff, this, i));
        add(action);
      }
      else if (!IsDamageDestroyed())
      {
        // param is ladder index
        // param2 is 0 for bottom, 1 for top
        {
          UIAction action(new ActionLadderPos(ATLadderOnUp, this, i, 0));
          action._memoryPoints.Add(ladder._bottom);
          action.highlight = person->IsActionHighlighted(ATLadderOnUp, this);
          add(action);
        }
        {
          UIAction action(new ActionLadderPos(ATLadderOnDown, this, i, 1));
          action._memoryPoints.Add(ladder._top);
          action.highlight = person->IsActionHighlighted(ATLadderOnUp, this);
          add(action);
        }
      }
    }
  }

  base::GetActions(add,unit);
}

void Building::SimulateImpulseDamage(DoDamageResult &result, EntityAI *owner, float impulse, Vector3Par modelPos)
{
}

ChurchType::ChurchType( ParamEntryPar param )
:base(param)
{
}

void ChurchType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);
}

void ChurchType::InitShape()
{
  base::InitShape();
}

void ChurchType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);
}
void ChurchType::DeinitShapeLevel(int level)
{
  base::DeinitShapeLevel(level);
}


Object* ChurchType::CreateObject(bool unused) const
{ 
  return new Church(this, VISITOR_NO_ID);
}


Church::Church( const EntityAIType *name, const CreateObjectId &id)
:base(name,id),
_ringSmall(0),_ringBig(0),
_nextRing(0)
//_object(object)
{
  SetSimulationPrecision(10.0458);
  RandomizeSimulationTime();
  SetTimeOffset(0);

  _badTime=GRandGen.RandomValue()*(1.0/24/60); // one minute non-exactness
  _static=true;
  SetType(Primary);
  _destrType=DestructBuilding;
}

void Church::Simulate( float deltaT, SimulationImportance prec )
{
  //if( !_object ) _delete=true;
  base::Simulate(deltaT, prec);
}

void Church::Sound( bool inside, float deltaT )
{
  if (_isDead) return;
  float time=Glob.clockWeather.GetTimeOfDay();
  #define TIME_OFFSET 4

  time+=TIME_OFFSET*(1.0/(24*60))+_badTime;
  int hour=toIntFloor(time*24);
  int minute=toIntFloor(time*(24*60)-hour*60);
  if (minute >= 60)
  {
    hour++;
    minute -= 60;
  }

  while( hour>12 ) hour-=12;
  while( hour<=0 ) hour+=12;
  switch( minute )
  {
    case 15+TIME_OFFSET-1: case 15+TIME_OFFSET:
      if( _ringBig==0 ) _ringSmall=1,_ringBig=0;
    break;
    case 30+TIME_OFFSET-1: case 30+TIME_OFFSET:
      if( _ringBig==0 ) _ringSmall=2,_ringBig=0;
    break;
    case 45+TIME_OFFSET-1: case 46+TIME_OFFSET:
      if( _ringBig==0 ) _ringSmall=3,_ringBig=0; break;
    break;
    case 0: case 1: case 2: case 3:
      _ringSmall=4,_ringBig=hour;
    break;
    default:
    if( _ringSmall>0 || _ringBig>0 )
    {
      _nextRing-=deltaT;
      if( _nextRing<=0 )
      {
        if( _ringSmall>0 )
        {
          SoundPars pars;
          GetValue(pars, Pars >> "CfgSFX" >> "Church" >> "smallBell");
          AbstractWave *sound=GSoundScene->OpenAndPlayOnce(
            pars.name,this, false,FutureVisualState().Position(),VZero
          );
          if( sound )
          {
            sound->SetMaxDistance(pars.distance);
            GSoundScene->SimulateSpeedOfSound(sound);
            GSoundScene->AddSound(sound);
          }
          _nextRing=2.0;
          --_ringSmall;
        }
        else if( _ringBig>0 )
        {
          SoundPars pars;
          GetValue(pars, Pars >> "CfgSFX" >> "Church" >> "largeBell");
          AbstractWave *sound=GSoundScene->OpenAndPlayOnce(
            pars.name,this,false,RenderVisualState().Position(),VZero
          );
          if( sound )
          {
            sound->SetMaxDistance(pars.distance);
            GSoundScene->SimulateSpeedOfSound(sound);
            GSoundScene->AddSound(sound);
          }
          _nextRing=2.0;
          --_ringBig;
        }
      }
    }
    break;
  }
}

void Church::ResetStatus()
{
  _ringBig=0;
  _ringSmall=0;
  base::ResetStatus();
}


void Church::UnloadSound()
{
}

Fountain::Fountain( const EntityAIType *name, const CreateObjectId &id)
:base(name,id)
{
  _sound = new SoundObject(Type()->_sound, this, true, true);
}

void Fountain::Simulate( float deltaT, SimulationImportance prec )
{
  if (_sound) _sound->Simulate(this,deltaT, prec);
}
void Fountain::Sound( bool inside, float deltaT )
{
}
void Fountain::UnloadSound()
{
}

FountainType::FountainType( ParamEntryPar param )
:base(param)
{
}

void FountainType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);
  _sound = par >> "sound";
}


Object* FountainType::CreateObject(bool unused) const
{ 
  return new Fountain(this, VISITOR_NO_ID);
}

WindSockType::WindSockType(ParamEntryPar param)
:base(param)
{
  ParamEntryVal config = Pars>>"CfgCloth">>(param >> "cloth").operator RString();

  _clothType = new ClothObjectType;
  _clothType->Init(config);
}

void WindSockType::InitShape()
{
  base::InitShape();

  ParamEntryVal par = *_par;

  _fabric.Init(_shape, (par >> "selectionFabric").operator RString(), NULL);
  // default size in case everything else fails
}


void WindSockType::InitFabric()
{
  if (_shape->NLevels()<=0) return;
  ShapeUsed shape = _shape->Level(0);
  int selIndex = _fabric.GetSelection(0);
  if (selIndex>=0)
  {
    ShapeUsedGeometryLock<> lock(_shape,0);

    // scan points of selection
    const NamedSelection::Access sel(shape->NamedSel(selIndex),_shape,shape);

    // flag base
    // calculate base 
    float minX = +1e10, minY = +1e10;
    float maxX = -1e10, maxY = -1e10;
    for (int i=0; i<sel.Size(); i++)
    {
      // for each point:
      int index = sel[i];
      Vector3Val val = shape->Pos(index);
      saturateMin(minX,val.X());
      saturateMin(minY,val.Y());
      saturateMax(maxX,val.X());
      saturateMax(maxY,val.Y());
    }
    float xSize = maxX-minX;
    float ySize = maxY-minY;

    _clothType->InitShape(minX,minY,xSize,ySize);
  }
}

void WindSockType::DeinitShape()
{
  base::DeinitShape();
  _fabric.Deinit(_shape);
}
void WindSockType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);
  _fabric.InitLevel(_shape,level);
}
void WindSockType::DeinitShapeLevel(int level)
{
  _fabric.DeinitLevel(_shape,level);
  base::DeinitShapeLevel(level);
}


Object* WindSockType::CreateObject(bool unused) const
{
  Object* v;

  if (!stricmp(_simName,"windsock") ) v = new WindSock(this,CreateObjectId());
  else 
  {
    LogF("Unknown vehicle simulation %s: type %s (WindSockType called)",
      cc_cast(_simName), cc_cast(GetName())
      );
    v = NULL;
  }
  return v;
}



DEFINE_CASTING(WindSock)

WindSock::WindSock(const EntityType *name, const CreateObjectId &id)
: base(name->GetShape(),name,id)
{
  // note: we need to simulate cloth quite often
  SetSimulationPrecision(0.1f);
  RandomizeSimulationTime();
  SetTimeOffset(0.1f);
}

bool WindSock::InitAsync(Matrix4Par pos)
{
  if (_cloth)
  {
    return true;
  }
  if (_shape->CheckLevelLoaded(0,true))
  {
    Init(pos, true);
    // after we initialized the flag, it would be strange if there is no cloth
    // this probably means model is wrong
    DoAssert(_cloth);
    return true;
  }
  return false;
}


void WindSock::Init(Matrix4Par pos, bool init)
{
  // make sure level 0 is loaded so that we can test the selection
  ShapeUsed level0 = _shape->Level(0);
  if (Type()->_fabric.GetSelection(0)>=0)
  {
    unconst_cast(Type())->InitFabric();
    _cloth = new ClothObject();
    _cloth->Init(*Type()->_clothType,pos);
  }
}

WindSock::~WindSock()
{
  _cloth.Free();
}


void WindSock::FlagSimulate(Matrix4Par pos, float deltaT, SimulationImportance prec)
{
  base::Simulate(deltaT,prec);
  if (_cloth)
  {
    Vector3 wind = GLandscape->GetWindSpeed();

    // calculate world space transformation of flag
    Vector3 inertia = VZero; // global external force 
    _cloth->Simulate(*Type()->_clothType,pos,VZero,deltaT,wind,inertia,prec);
  }
}


void WindSock::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  // when we are passed setEngineStuff=true, it means we should not do any shape animation
  // this is used in Scene.AdjustComplexity to check proxy complexity
  if (_cloth && !setEngineStuff)
  {
    // we need to know world space coordinates of the flag
    // scan cloth particles and animates points of flag animation as appropriate
    ShapeUsed shape = _shape->Level(level);

    int selIndex = Type()->_fabric.GetSelection(level);
    if (selIndex>=0)

    {
      // scan points of selection
      const NamedSelection::Access sel(shape->NamedSel(selIndex),_shape,shape);

      float xSize = Type()->_clothType->GetSizeX();
      float ySize = Type()->_clothType->GetSizeY();
      float xMin = Type()->_clothType->GetMinX();
      float yMin = Type()->_clothType->GetMinY();
      float invXSize = 1/xSize;
      float invYSize = 1/ySize;

      Matrix4 worldToModel = animContext.GetVisualState().cast<Flag>().GetInvTransform();
      for (int i=0; i<sel.Size(); i++)
      {
        // for each point:
        // estimate where in flag it is
        // 
        int index = sel[i];
        Vector3Val val = shape->Pos(index);

        float xIndexF = (val.X()-xMin)*invXSize;
        float yIndexF = (val.Y()-yMin)*invYSize;

        // get simulation result for point xIndexF, yIndexF
        Vector3 pos = _cloth->GetPosition(xIndexF,yIndexF);

        Vector3 mpos = worldToModel.FastTransform(pos);
        mpos[2] += val.Z();

        animContext.SetPos(shape, index) = mpos;
      }
      animContext.InvalidateNormals(shape);
    }
  }

  base::Animate(animContext, level, setEngineStuff, this, dist2);
}

void WindSock::Deanimate(int level, bool setEngineStuff)
{
  base::Deanimate(level, setEngineStuff);
}
