#ifdef _MSC_VER
#pragma once
#endif

#ifndef _HOUSE_HPP
#define _HOUSE_HPP

#include "transport.hpp"
#include "dynSound.hpp"

#include "pathAction.hpp"
#include "Cloth/ClothObject.h"

struct Ladder
{
  int _top,_bottom;
};

TypeIsSimple(Ladder)

struct WeaponProxy
{
  OLink(Object) obj;
  int selection; // copied over from proxyObject

  WeaponProxy() {selection = -1;}
  bool IsPresent() const {return selection >= 0;}
};
TypeContainsOLink(WeaponProxy)

struct BuildingPointInfo
{
  FindArray<int> _connections;
  Ref<PathAction> _action;
};
TypeIsMovable(BuildingPointInfo)

// temporary solution of problems with Transport derived buildings
class BuildingType: public EntityAIType
{
  friend class Building;
  friend class IPaths; // give access to path information

  typedef EntityAIType base;

  AutoArray<int> _exits;
  AutoArray<int> _positions;
  AutoArray<BuildingPointInfo> _points;
  AutoArray<Ladder> _ladders;

  float _coefInside;  // we must go slowly inside houses 
  float _coefInsideHeur;  // estimation of cost based on in/out points

  /// there are some subclases of MarkerLights - simulation is needed
  bool _lightsExists;

  // windsock proxy -- simulation is needed
  bool _windSockExist;

  /// selections where to write house number
  PlateInfos _houseNumbers;

  /// what object will be building replaced by when damaged
  RString _replaceDamaged;
  /// limit for damage of hitpoint from the list below when the building is replaced
  float _replaceDamagedLimit;
  /// list of hitpoints to check for limit
  AutoArray<RString> _replaceDamagedHitpoints; 

public:
  BuildingType(ParamEntryPar param);
  void LoadExt(ParamEntryPar par, const char *shape, bool enableHWAnim);
  virtual void Load(ParamEntryPar par, const char *shape);
  
  virtual void InitShape(); // after shape is loaded
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);
  
  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;

  Vector3Val GetPosition(int index) const;
  PathAction *GetPathAction(int index) const;
  const FindArray<int> &GetConnections(int index) const;
  /// cost coef. for motion control - used to slow down inside of buildings
  float GetCoefInside() const {return _coefInside;}
  /// cost coef. for path search
  float GetCoefInsideHeur() const {return _coefInsideHeur;}
  int FindNearestExit(Vector3Par pos, Vector3 &ret) const;
  int FindNearestPosition(Vector3Par pos, Vector3 &ret) const;
  
  const Ladder &GetLadder(int i) const {return _ladders[i];}
  int GetLadderCount() const {return _ladders.Size();}

  bool IsPersistenceRequired() const;
  bool IsSimulationRequired() const;
  bool IsSoundRequired() const;
  float GetFeatureSize(Vector3Par pos) const;

protected:
  int FindNearestPoint(Vector3Par pos, Vector3 &ret) const;
};

class FountainType: public BuildingType
{
  friend class Fountain;

  typedef BuildingType base;

  RString _sound;
  float _animSpeed;

  public:
  FountainType(ParamEntryPar param);
  virtual void Load(ParamEntryPar par, const char *shape);
  
  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;

  bool IsPersistanceRequired() const {return true;}
  bool IsSimulationRequired() const {return true;}
  bool IsSoundRequired() const {return true;}
};

class ChurchType : public BuildingType
{
  friend class Church;
  typedef BuildingType base;

  public:
  ChurchType(ParamEntryPar param);
  virtual void Load(ParamEntryPar par, const char *shape);

  void InitShape(); // after shape is loaded
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);
  
  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;

  bool IsPersistanceRequired() const {return true;}
  bool IsSimulationRequired() const {return true;}
  bool IsSoundRequired() const {return true;}
};

class IPaths
{
protected:
  AutoArray<int> _locks; 

public:
  virtual const BuildingType *GetBType() const = 0;
  virtual const Object *GetObject() const = 0;
  
  /// get world space position including animations
  Vector3 GetPosition(int index) const;
  /// get model space position, ignore any animations
  Vector3 GetModelStaticPosition(int index) const {return GetBType()->GetPosition(index);}
  
  const FindArray<int> &GetConnections(int index) const;
  int FindNearestExit(Vector3Par pos, Vector3 &ret) const;
  int FindNearestPosition(Vector3Par pos, Vector3 &ret) const;
  int FindNearestPoint(Vector3Par pos, Vector3 &ret,float maxDist2=FLT_MAX) const;

  int NExits() const {return GetBType()->_exits.Size();}
  int NPos() const {return GetBType()->_positions.Size();}
  int NPoints() const {return GetBType()->_points.Size();}
  int GetExit(int index) const {return (index >= 0 && index < NExits()) ? GetBType()->_exits[index] : -1;}
  int GetPos(int index) const {return (index >= 0 && index < NPos()) ? GetBType()->_positions[index] : -1;}
  PathAction *GetPathAction(int index) const {return GetBType()->GetPathAction(index);}

  int IsLocked(int index) const
  {
    if (index >= _locks.Size()) return 0;
    return _locks[index];
  }
  void Lock(int index)
  {
    int n = _locks.Size();
    if (index >= n)
    {
      _locks.Resize(index + 1);
      for (int i=n; i<=index; i++) _locks[i] = 0;
    }
    _locks[index]++;
  }
  void Unlock(int index)
  {
    DoAssert(index < _locks.Size());
    _locks[index]--;
  }
};

class WindSockType: public EntityType
{
  typedef EntityType base;

  friend class WindSock;

  SRef<ClothObjectType> _clothType;

  AnimationSection _fabric;

public:
  explicit WindSockType(ParamEntryPar param);

  bool AbstractOnly() const {return false;}

  void InitShape();
  void DeinitShape();
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);

  virtual Object* CreateObject(bool unused) const;

  void InitFabric();
};

class WindSock : public Entity
{
protected:
  typedef Entity base;
  SRef<ClothObject> _cloth;


public:
  WindSock(const EntityType *name, const CreateObjectId &id);
  ~WindSock();

  const WindSockType *Type() const {return static_cast<const WindSockType *>(GetNonAIType());}

  /// non-blocking init, returns true once completed
  bool InitAsync(Matrix4Par pos);
  void Init(Matrix4Par pos, bool init);
  void FlagSimulate( Matrix4Par pos,float deltaT, SimulationImportance prec );

  AnimationStyle IsAnimated(int level) const {return AnimatedGeometry;}

  virtual void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  virtual void Deanimate(int level, bool setEngineStuff);

  USE_CASTING(base)
};


class Building: public EntityAI, public IPaths
{
  typedef EntityAI base;
  /// avoid starting multiple animators
  bool _animatorRunning;

  /// house number based on building index
  RString _houseNumber;

  /// destructible replacement of this object
  Ref<EntityAI> _damagedBuilding;

  /// animator which preloads damaged building and call replacement procedure when needed
  Ref<Entity> _replaceByDamaged;

  /// some object which are normally placed as on-surface may have this feature disabled
  /** implemented to allow HeliH placed on a carrier deck */
  bool _disableOnSurface;

  /// list of proxies with special simulation WindSock
  RefArray<WindSock> _windSocks;

public:
  Building(const EntityAIType *name, const CreateObjectId &id, bool fullCreate = true);
  
  /// set building index - used for house numbers
  virtual void SetBuildingIndex(int index) {_houseNumber = Format("%d", index + 1);}

  // get interface
  virtual const IPaths *GetIPaths() const {return this;}
  virtual const Object *GetObject() const {return this;}
  
  const BuildingType *Type() const
  {
    return static_cast<const BuildingType *>(GetType());
  }

  const BuildingType *GetBType() const
  {
    return static_cast<const BuildingType *>(GetType());
  }

  void Destroy( EntityAI *owner, float overkill, float minExp, float maxExp );
  virtual const EntityType *CanDestroyUsingEffects() const;

  void ReactToDamage();
  /// replace the building by a destructible variant
  void ReplaceByDamaged();
  
  virtual void ExplodeSmoke();
  
  virtual void SetNotOnSurface(bool value) {_disableOnSurface=value;}
  virtual int GetObjSpecial(int shapeSpecial) const {return _disableOnSurface ? shapeSpecial&~OnSurface : shapeSpecial;}

  void BroadcastDamage(bool immediate);
  
  bool SimulationReady(SimulationImportance prec) const;
  void Simulate( float deltaT, SimulationImportance prec );
  void SimulatePost(float deltaT, SimulationImportance prec);

  bool IsAnimatorRequired() const;

  virtual void ResetStatus();

  virtual void Draw(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    ) override;

  virtual bool MustBeSaved() const;
  virtual LSError Serialize(ParamArchive &ar);

  virtual bool EngineIsOn() const {return !IsDamageDestroyed();}

  void OnAnimationStarted();
  void OnAnimatorTerminated();

  // building are usually empty
  void Sound( bool inside, float deltaT ) {}
  void UnloadSound() {}

  void DrawDiags();
  bool QIsManual() const {return false;}
  bool IsMoveTarget() const;
  
  Matrix4 InsideCamera( CameraType camType ) const;
  int InsideLOD( CameraType camType ) const;

  // no get-in to buildings
  bool QCanIGetIn( Person *who = NULL ) const {return false;}

  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const;
  bool CanBeInstancedShadow(int level) const;
  AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate

  Vector3 GetLadderPos( int ladder, bool up );
  Vector3 GetLadderForward(int ladderIndex);

  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  virtual void PerformAction(const Action *action, AIBrain *unit) override;

  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;

  bool NeedsPrepareProxiesForDrawing() const {return true;}
  void PrepareProxiesForDrawing(
    Object *rootObj, int rootLevel, const AnimationContext &animContext,
    const PositionRender &transform, int level, ClipFlags clipFlags, SortObject *so
  );
  int GetProxyComplexity(int level, const Matrix4 &pos, float dist2) const;
  
  virtual void SimulateImpulseDamage(DoDamageResult &result, EntityAI *owner, float impulse, Vector3Par modelPos);

  //TargetSide GetTargetSide() const {return TCivilian;}
  INHERIT_SOFTLINK(Building,base);

  USE_CASTING(base)
};

class Church: public Building
{
  typedef Building base;

  int _ringSmall,_ringBig;
  float _nextRing;
  float _badTime; // no clock is exact

  public:
  Church(const EntityAIType *name, const CreateObjectId &id);
  
  void Simulate( float deltaT, SimulationImportance prec );

  void Sound( bool inside, float deltaT );
  void UnloadSound();

  void ResetStatus();

  const ChurchType *Type() const
  {
    return static_cast<const ChurchType *>(GetType());
  }
};

class Fountain: public Building
{
  Ref<SoundObject> _sound;

  typedef Building base;

  public:
  Fountain(const EntityAIType *name, const CreateObjectId &id);

  const FountainType *Type() const
  {
    return static_cast<const FountainType *>(GetType());
  }
  
  void Simulate( float deltaT, SimulationImportance prec );

  void Sound( bool inside, float deltaT );
  void UnloadSound();
};

#endif
