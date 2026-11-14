#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SHOTS_HPP
#define _SHOTS_HPP

#include "vehicle.hpp"
#include "lights.hpp"
#if _VBS2 // suppression
#include "transport.hpp"
#include "person.hpp"
#endif

#include "weapons.hpp"
#include "smokes.hpp"

#include <Es/Memory/normalNew.hpp>

typedef Object DirectHitType;

struct WeaponModePtr;

/// basic shot simulation
/**
Provides common functionality for various ammo types,
including shells, bullets, grenades, missiles.
*/
class Shot: public Entity
{
  typedef Vehicle base;

protected:
  /// who fired this shot
  OLinkO(EntityAI) _parent;
  /// sound currently being played
  RefAbstractWave _sound;
  /// how much time should the shot exist before it disappears
  /**
  This is used mostly as optimization to avoid stray bullets
  being simulated when no longer needed.
  Some simulation classes may use it for different purposes (explosion timing)
  */
  float _timeToLive;

public:
  Shot(EntityAI *owner, const AmmoType *type);
  /// check who fired this shot
  EntityAI *GetOwner() const {return _parent;}
  const AmmoType *Type() const {return static_cast<const AmmoType *>(GetNonAIType());}

  // no geometry collision testing for shots
  bool HasGeometry() const override {return false;}
  virtual bool CanBeHit() const override {return false;}

  float Rigid() const override {return 0.0f;}
  bool OcclusionFire() const override {return false;}
  bool OcclusionView() const override {return false;}

  Visible VisibleStyle() const override;
  float VisibleSize(ObjectVisualState const& vs) const override {return 0.0f;}
  void Sound(bool inside, float deltaT) override;
  void UnloadSound() override;
  // can this shot penetrate freely through an object with the given penetrability? (for examples leaves are peentrable this way)
  bool CanPenetrateFreely(float penetrability) const;
  
  // assume shot simulation is always ready
  virtual bool SimulationReady(SimulationImportance prec) const override {return true;}

  // direction that should be marked on UI as a source of the damage (most shots have the origin backside)
  Vector3 OriginDirection() const {return -FutureVisualState().Direction();}
  
#if _VBS3
  void OnShotEvent(AmmoEvent event, EntityAI *shoter, Entity *hit, Vector3 pos, Vector3 velocity,
    AutoArray<RString> &component, bool explode=true, Vector3 normal=VZero);
#endif
  
  LSError Serialize(ParamArchive &ar) override;

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  static Shot *CreateObject(NetworkMessageContext &ctx);
  TMError TransferMsg(NetworkMessageContext &ctx) override;
  float CalculateError(NetworkMessageContextWithError &ctx) override;
  float GetMaxPredictionTime() const override;
  virtual float GetTimeToLive() {return _timeToLive;}
  virtual bool IsRemote() const {return false;}

  USE_CASTING(base)
};

/// satchel changed - both timed and remote controlled
class PipeBomb: public Shot
{
  typedef Shot base;
protected:
  bool _explosion;
public:
  PipeBomb(EntityAI *parent, const AmmoType *type);
  void Simulate(float deltaT, SimulationImportance prec);
  void Explode() {_explosion = true;}
  
  void SetTimer(float ttl=20.0f) {_timeToLive = ttl;}
  float GetTimer() const {return _timeToLive;}
  
  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  virtual void PerformAction(const Action *action, AIBrain *unit) override;
  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;

  // enable collision testing to enable manipulation
  virtual bool HasGeometry() const override {return Entity::HasGeometry();}
  virtual bool OcclusionFire() const override {return Entity::OcclusionFire();}
  virtual bool OcclusionView() const override {return Entity::OcclusionView();}

  LSError Serialize(ParamArchive &ar);
  USE_CASTING(base)
};

/// proximity mine - explodes when a specific object is near
class Mine: public Shot
{
  typedef Shot base;
protected:
  /// mine can be deactivated by some soldier types (engineers)
  bool _active;
public:
  Mine(EntityAI *parent, const AmmoType *type);
  void Simulate(float deltaT, SimulationImportance prec);
  bool IsActive() const {return _active;}
  void SetActive(bool active) {_active = active;}

  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  virtual void PerformAction(const Action *action, AIBrain *unit) override;
  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;

  // enable collision testing to enable manipulation
  virtual bool HasGeometry() const override {return Entity::HasGeometry();}
  virtual bool OcclusionFire() const override {return Entity::OcclusionFire();}
  virtual bool OcclusionView() const override {return Entity::OcclusionView();}

  LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);
  
  USE_CASTING(base)
};

/// tank shell, hand grenade
class ShotShell: public Shot
{
  typedef Shot base;

protected:
  float _airFriction;
  float _initDelay;
  /// the distance to travel left until releasing the fuse (see also AmmoType::fuseDistanceLeft)
  float _fuseDistanceLeft;
  float _timeToExplosion;
#if _VBS2 // suppression
  ManCargo _potentialSuppressed;
  AutoArray<int> _timesToSuppress;
  Vector3 _startingLocation;
#endif
  /// set once shot is terminated - obstacle is reached
  bool _terminate;
  /// only real shots can cause any damage - false is used for MP "fake/remote" shots
  bool _real;

public:
  ShotShell(EntityAI *parent, const AmmoType *type, bool real);
  void Simulate(float deltaT, SimulationImportance prec);
  LSError Serialize(ParamArchive &ar);

  bool IsRemote() const override {return !_real;}

#if _VBS2 // suppression
  void BuildSuppressionList();
  void CheckForSuppression();
  void SetStartingLocation(Vector3 pos);
#endif

protected:
  void SimulateMovement(float deltaT, bool ignoreNearCollision=false, bool ignoreDeflection=false);

  bool SimulateDeflection(const CollisionInfo *info, Vector3Par normal, Vector3Par isect, float dist, float deltaT, float maxDeflectionAngleCos);
  bool SimulatePenetration(const CollisionInfo *info, Vector3Par normal, Vector3Par isect, float dist, float deltaT);
  // returns true when explosion was done and shell should be deleted (which is not the case in SmokeShell)
  virtual bool SimulateHit(const CollisionInfo *info, Vector3Par pos, Vector3Par surfNormal=VZero, Vector3Par newSpeed=VZero);

  // update position and speed of the shot with respect to the air friction and gravitation, eventual collision surface is treated too
  void UpdatePosAndSpeed(float deltaT, Vector3Par newPos, float penetrability=1.0f);
  // variant for debugging, do not use in production code
  void UpdatePosAndSpeed(const char *debugString, float deltaT, Vector3Par newPos, float penetrability=1.0f);
  // fix position and speed of the shot after collision, so the shot wouldn't go inside the object anymore
  void FixPosAndSpeedAfterCollision(Vector3Par surfNormal);
  // for debugging of shot shell simulation; if action is NULL, _debugPrefix is used
  void DebugShotShell(const CollisionInfo *info, float deltaT, Vector3Par oldSpeed, Vector3Par newSpeed, const char *format, ...);

  virtual void AmmoChangeExpectedFailed();

  /// can this shot make any damage? (only real shot can)
  bool CanDamage() const;
  Visible VisibleStyle() const override;
  /// some shots may be timed or have fuse, so they don't explode just on impact
  bool CanExplodeImmediately() const;

  void Sound(bool inside, float deltaT);
  
  void StartFrame();
  /// used to handle segment rendering on deflection and penetration
  virtual void TerminateSegment(bool stopSuppression) {}

public:
  USE_CASTING(base)
  USE_FAST_ALLOCATOR
};

/// machine gun, rifle or pistol bullet
/**
Has no model, tracer is rendered instead.
Rendering is done using line drawing, and can be done in several segments in one frame,
because of possible deflection or direction change when doing through something.
*/
class ShotBullet: public ShotShell
{
  typedef ShotShell base;

  //@{ tracer rendering
  int _nSegs;
  Vector3 _beg;
  enum {MaxSegs=4};
  Vector3 _segEnd[MaxSegs]; // world space line
  //@}
  
  //@{ suppressive fire trace rasterization
  /// how long distance is left to trace the suppression
  float _suppressionDistanceLeft;
  
  /// position to which we have already performed the suppressive fire tracing
  Vector3 _posSuppressTraced;
  //@}

protected:
  /// Time to ignite the tracer
  float _tracerStart;
  /// Time the tracer ends
  float _tracerEnd; //TODO: not implemented yet
  ///do not extend the tracer shape beyond the muzzlePos
  Vector3 _muzzlePos;
  /// toggling tracers on/off possible real time - useful for diagnostics
  bool _showTracer;

public:
  ShotBullet(EntityAI *parent, const AmmoType *type, bool real, bool showTracer=false);

  float GetMass() const {return Type()->GetMass();}
  float GetInvMass() const {return Type()->GetInvMass();}

  void Simulate(float deltaT, SimulationImportance prec);
  void UnloadSound() {}

  Visible VisibleStyle() const override;

  /// start frame - record a position of tracer beginning
  void StartFrame();
  /// end frame - record a position of tracer end
  void EndFrame();
  //! Actual length of the tracer
  float TracerLength(ObjectVisualState const& vs) const;

  //@{
  /** we need to override complexity/area because model is not used and the tracer has different characteristics */
  // do not consider bullets in scene density management
  int GetComplexity(int level, const Matrix4 &pos) const {return 0;}
  virtual bool IgnoreComplexity() const {return true;}
  virtual float EstimateArea() const;

  // alpha pass
  int PassNum(int lod) {return 2;}

  virtual void Draw(int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    ) override;

  AnimationStyle IsAnimated(int level) const {return AnimatedGeometry;}
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2) {}
  void Deanimate(int level, bool setEngineStuff) {}

	virtual float GetCollisionRadius(const ObjectVisualState &vs) const;
  float ClippingInfo(Vector3 *minMax, ClippingType clip) const;
  void AnimatedBSphere(int level, Vector3 &bCenter, float &bRadius, Matrix4Par pos, float posMaxScale) const;
  // TODO: ?? LSError Serialize(ParamArchive &ar);

protected:
  virtual void TerminateSegment(bool stopSuppression);
  void SetBeg(Vector3Val beg);
  void SetEnd(Vector3Val end);

  USE_CASTING(base)
  USE_FAST_ALLOCATOR
};

//shotgun ammo
struct SmallShell
{
  Vector3P _pos;
  Vector3P _speed;
  bool _active;
  SmallShell();
  SmallShell(const SmallShell &par);
  SmallShell(Vector3Par pos, Vector3Par speed);
};
TypeIsSimple(SmallShell)

class ShotSpread : public ShotBullet
{
  typedef ShotBullet base;

public:
  ShotSpread(EntityAI *parent, const AmmoType *type, const WeaponModePtr *ptr, bool real);
  virtual void Simulate(float deltaT, SimulationImportance prec);
  virtual void SimulateMovement(float deltaT);
  virtual void DrawDiags();
protected:

  void CreateSmallShels();

  AutoArray<SmallShell> _shells;
  float _spreadAngle;

  USE_CASTING(base)
    USE_FAST_ALLOCATOR
};

/// guided or unguided mission or rocket (has active power source)
class Missile: public Shot 
{
  typedef Shot base;

public:
  enum EngineState {Init, Thrust, Fly};
  enum LockState {Locked, Lost};

private:
  RefAbstractWave _soundEngine;
  EffectsSource _effects;

  float _initTime, _thrustTime;

  float _thrust;
  EngineState _engine;
  /// guided missile may lose their lock
  LockState _lock;
  /// target this missile is locked to
  OLink(Object) _target;

  /// manual missile control - direction
  Vector3 _controlDirection; 
  /// manual control activated
  bool _controlDirectionSet;
  
public:
  Missile(EntityAI *parent, const AmmoType *type, Object *target);
  ~Missile() {}
  void Sound(bool inside, float deltaT);
  void UnloadSound();

  Visible VisibleStyle() const override {return ObjVisibleFast;} /// a missile is visible even when moving fast
  virtual void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  virtual void Deanimate(int level, bool setEngineStuff) {}
  virtual void Simulate(float deltaT, SimulationImportance prec);
  /// control direction - used for manual missile control
  void SetControlDirection(Vector3Par dir);
  void UnlockMissile() {_lock = Lost; _target = NULL;}
  bool HasTarget() {return (_target != NULL);}

  LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  float CalculateError(NetworkMessageContextWithError &ctx);
  float GetMaxPredictionTime() const;
  
  virtual bool RemoteStateSupported() const {return true;}

  USE_CASTING(base)
};

/// flare or other illuminating shells
class Flare: public ShotShell
{
  typedef ShotShell base;
protected:
  Color _lightColor;
  /// value of LightPointVisible::_size
  float _size;
  float _radius;
  Ref<LightPointOnVehicle> _light;
public:
  Flare(EntityAI *parent, const AmmoType *type);
  void Simulate(float deltaT, SimulationImportance prec);
  LSError Serialize(ParamArchive &ar);
  DECLARE_NETWORK_OBJECT
  USE_CASTING(base)
  USE_NORMAL_ALLOCATOR
};

/// signal light
class MarkerLightShell: public ShotShell
{
  typedef ShotShell base;
  OLinkO(EntityAI) _nvTarget;

protected:
  Time _activation;
  
public:
  MarkerLightShell(EntityAI *parent, const AmmoType *type);

public:
  void Simulate(float deltaT, SimulationImportance prec);
  virtual bool SimulateHit(const CollisionInfo *info, Vector3Par pos, Vector3Par surfNormal = VZero, Vector3Par newSpeed = VZero);
  LSError Serialize(ParamArchive &ar);
  
  DECLARE_NETWORK_OBJECT
  USE_CASTING(base)
  USE_NORMAL_ALLOCATOR

private:
  bool Init();
};

/// smoke generating shells
class SmokeShell: public ShotShell
{
  typedef ShotShell base;
protected:
  EffectsSourceRT _effects;
  bool _smoking;
  float _smokeColor[4];

  // ShotShell overload
  virtual bool SimulateHit(const CollisionInfo *info, Vector3Par pos, Vector3Par surfNormal=VZero, Vector3Par newSpeed=VZero)
  { // explosion means we should start emitting smoke
    _smoking = true;
    return false;
  }
public:
  SmokeShell(EntityAI *parent, const AmmoType *type);
  void Simulate(float deltaT, SimulationImportance prec);

  LSError Serialize(ParamArchive &ar);

  USE_CASTING(base)
  USE_NORMAL_ALLOCATOR
};

/// flare or other illuminating shells
class CounterMessure: public ShotShell
{
  typedef ShotShell base;
  EffectsSourceRT _effects;
  float _startTime;
  bool _smoking;
  float _thrustTime;
  float _maxControlRange;
  float _initTime;

public:
  CounterMessure(EntityAI *parent, const AmmoType *type);
  void Simulate(float deltaT, SimulationImportance prec);
  void SimulateCMMovement(float deltaT);
  LSError Serialize(ParamArchive &ar);
  float GetTimeToLive() {return floatMin(_timeToLive,_thrustTime);}

  DECLARE_NETWORK_OBJECT
  USE_CASTING(base)
  USE_NORMAL_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

/// create a new shot of any type
Shot *NewShot(EntityAI *parent, const AmmoType *type, const WeaponModePtr *ptr, Object *target, bool real=true, bool showTracer=false);


#define CREATE_SHOT_MSG(MessageName, XX) \
  XX(MessageName, RString, type, NDTString, RString, NCTDefault, DEFVALUE(RString, ""), DOC_MSG("Entity type"), TRANSF) \
  XX(MessageName, OLinkO(EntityAI), parent, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("Owner of shot"), TRANSF_REF) \
  XX(MessageName, float, timeToLive, NDTFloat, float, NCTNone, DEFVALUE(float, 10), DOC_MSG("Time to live (in seconds)"), TRANSF) \
  XX(MessageName, Vector3, createPos, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, temp), DOC_MSG("Initial position"), TRANSF) \
  XX(MessageName, Vector3, createSpeed, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, temp), DOC_MSG("Initial speed"), TRANSF) \
  XX(MessageName, Matrix3, createOrient, NDTMatrix, Matrix3, NCTMatrixOrientation, DEFVALUE(Matrix3, tempM), DOC_MSG("Initial orientation"), TRANSF)

DECLARE_NET_INDICES_EX(CreateShot, NetworkObject, CREATE_SHOT_MSG)

#define UPDATE_SHOT_MSG(MessageName, XX)

DECLARE_NET_INDICES_EX_ERR(UpdateShot, UpdateVehicle, UPDATE_SHOT_MSG)

#endif
