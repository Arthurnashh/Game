#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TANK_HPP
#define _TANK_HPP

#include "tankOrCar.hpp"
//#include "ai.hpp"
#include "shots.hpp"
#include "global.hpp"

#include <El/ParamFile/paramFile.hpp>


/// damper animation - links bones to damper simulation
class AnimationSourceTankDamper: public AnimationSource
{
  typedef AnimationSource base;
  
  friend class TankDamperHolder;
  
  /// which bone is this one bound to
  SkeletonIndex _bone;
  /// center of the selection
  Vector3 _center;
  
  public:
  AnimationSourceTankDamper();
  //@{ AnimationSource implementation
  virtual float GetPhase(const ObjectVisualState &vs, const Animated *obj) const;
  virtual void SetPhaseWanted(Animated *obj, float phase) {}
  //@}
};


/// container for damper properties - type (shared between instances)
class TankDamperHolder
{
  /// total count of dampers
  AutoArray< InitPtr<AnimationSourceTankDamper> > _sources;
	
  /// total count of dampers
  public:
  TankDamperHolder();
  
  AnimationSource *CreateAnimationSource();

  void InitLandcontact(
    LODShape *shape, int level,
    const AnimationHolder &anims, const AnimationSourceHolder &sources
  );
  void InitShapeLevel(
    LODShape *shape, int level,
    const AnimationHolder &anims, const AnimationSourceHolder &sources
  );
  void Clear();
  
	int Count() const {return _sources.Size();}
};


class TankType;

class TankVisualState : public TankOrCarVisualState  // TODO: dampers
{
  typedef TankOrCarVisualState base;
  friend class Tank;
  friend class TankWithAI;

public:
  TankVisualState(TankType const& type);
  virtual void Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const;
  virtual void Copy(ObjectVisualState const& src) {*this=static_cast_checked<const TankVisualState &>(src);}
  virtual TankVisualState *Clone() const {return new TankVisualState(*this);}
};


/// tank type shader information
class TankType: public TankOrCarType
{
  typedef TankOrCarType base;
  friend class Tank;
  friend class TankWithAI;

protected:
  Point3 _leftLightPos,_rightLightPos;
  Vector3 _leftLightDir,_rightLightDir;
  
  Vector3 _cargoLightPos; // light in cargo
  Color _cargoLightDiffuse;
  Color _cargoLightAmbient;
  float _cargoLightRadius;

  TankDamperHolder _dampers;

  bool _gunnerHasFlares;

  float _turnCoef;

  int _hullHit;
  int _trackLHit,_trackRHit;

  RString _fireDust;

public:
  
  TankType( ParamEntryPar param );
  virtual void Load(ParamEntryPar par, const char *shape);
  void InitShape();
  void DeinitShape();
  
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);
  AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source);
  
  Object* CreateObject(bool unused) const;
  virtual EntityVisualState* CreateVisualState() const { return new TankVisualState(*this); }
  
  bool AbstractOnly() const {return false;}

  float GetFieldCost( const GeographyInfo &info, CombatMode mode) const;
  float GetBaseCost(const GeographyInfo &info, bool operative, bool includeGradient) const;
  float GetGradientPenalty(float gradient) const;
  float GetCostTurn(int difDir, int &debet, bool forceFullTurn) const;
  FieldCost GetTypeCost(OperItemType type, CombatMode mode, Clearance clearance) const;
};


/*!
\patch 1.17 Date 08/14/2001 by Jirka
- Fixed: fire weapon effects in multiplayer
*/
class Tank: public TankOrCar
{
  typedef TankOrCar base;

  // basic caterpillar vehicle (tank/APC) simulation
protected:
  
  float _randFrequency;

  Time _collisionSimUntil;

  //! forward key is used for braking
  bool _forwardUsedAsBrake;
  //! backward key is used for braking
  bool _backwardUsedAsBrake;
  //! braking when vehicle is almost stopped
  bool _parkingBrake;

  bool _doGearSound;

  EffectsSourceRT _fireDust;
    
  TrackOptimized _track;

  Ref<LightPointOnVehicle> _cargoLight;

  // thrust is relative, in range (-1.0,1.0)
  // actually it is engine RPM?
  float _thrustLWanted,_thrustRWanted; // accelerate (left), accelerate (right)
  float _thrustL,_thrustR; // accelerate (left), accelerate (right)

  /// alternate steering wheel simulation, controlled by controlling the wheel speed
  float _turnSpeedWanted;
  /// used to dim cursor
  Time _turnSpeedWantedLastActive;

public:
  Tank( const EntityAIType *name, Person *driver, bool fullCreate = true );

  const TankType *Type() const
  {
    return static_cast<const TankType *>(GetType());
  }

  /// @{ Visual state
public:
  TankVisualState typedef VisualState;
  
  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}

  void ObjectContactStatic //< Object collision detection for static tanks
  (
    float deltaT, //< sim time
    float &crash //< return amount of crash done by objects
  );

  void ObjectContact
  (
    ObjectVisualState &moveTrans,
    Vector3Par wCenter,
    float deltaT,
    Vector3 &torque, Vector3 &friction, Vector3 &torqueFriction,
    Vector3 &totForce, float &crash, CrashType &doCrash
  );

  void LandFriction(
    Vector3 &friction, Vector3 &torqueFriction, Vector3 &torque,
    bool brakeFriction, Vector3Par fSpeed, Vector3Par speed, Vector3Par pCenter,
    float coefNPoints, const Texture *texture
  );

  
  Vector3 Friction( Vector3Par speed );

  /// @{ Visual controllers (\sa Type()->CreateAnimationSource())
  float GetCtrlDamper() const {return 0;}
  /// @}

  void MoveWeapons(float deltaT);

  bool IsAbleToMove() const;
  bool IsAbleToFire(const TurretContext &context) const;
  bool IsPossibleToGetIn() const;

  void OnTurretUpdate(Turret *turret, Matrix3Par oldOrient, Matrix3Par newOrient);

  //! Returns contact point, that means point under center of mass that touches ground, in model coordinates
  Vector3 GetLandContactPoint() const;

  void PlaceOnSurface(Matrix4 &trans);
  virtual bool RemoteStateSupported() const;
  void Simulate( float deltaT, SimulationImportance prec );
  virtual void StabilizeTurrets(Matrix3Val oldTrans, Matrix3Val newTrans);
  float GetEngineVol( float &freq ) const;
  float GetEnvironVol( float &freq ) const;

  virtual float IsGrassFlatenner(Vector3Par pos, float radius, Vector3 &flattenPos) const;
  virtual float FlattenGrass(float &skewX, float &skewZ, Vector3Par pos, float radius) const;
  
  void PerformFF( FFEffects &effects );
  Pair<Vector3,float> ShowDrivingCursor(CameraType camType, Person *person) const;

  float GetHitForDisplay(int kind) const;
/*
  void GetNetState( UNIT_STATE &state );
  void SetNetState( const UNIT_STATE &state );
*/

  void Sound( bool inside, float deltaT );
  void UnloadSound();

  bool IsTurret( CameraType camType ) const;
  bool HasFlares( CameraType camType ) const;

  void InitVirtual
  (
    CameraType camType, float &heading, float &dive, float &fov
  ) const;
  void LimitVirtual
  (
    CameraType camType, float &heading, float &dive, float &fov
  ) const;

  float OutsideCameraDistance( CameraType camType ) const {return 25;}

  virtual bool IsTurretLocked(const TurretContextV &context) const;

  AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate( int level, bool setEngineStuff );

  virtual void PreloadGetIn(UIActionType position);
  //! Internal state changes when camera change from internal to external or vice versa
  virtual void CamIntExtSwitched();

  RString DiagText() const;

  // @{ sound controllers
  Vector3 GetAngularVelocity() const { return _angVelocity; }
  float Thrust() const {return (fabs(_thrustL)+fabs(_thrustR))*0.5;}
  // @}

  float ThrustWanted() const {return (fabs(_thrustLWanted)+fabs(_thrustRWanted))*0.5;}
  float TrackingSpeed() const {return 80;}

  void Eject(AIBrain *unit, Vector3Val diff = VZero);
  void SuspendedPilot(AIBrain *unit, float deltaT );
  void KeyboardPilot(AIBrain *unit, float deltaT );
  void FakePilot( float deltaT );
#if _ENABLE_AI
  void AIPilot(AIBrain *unit, float deltaT ){}
#endif

  bool HasHUD() const {return true;}    

  /** Damage vehicle and crew according to impulse
  * @param owner - Owner of damage.
  * @param impulse - Impulse.
  * @param modelPos - Point where impulse is applied in model coord.
  */
  virtual void DamageOnImpulse(EntityAI *owner, float impulse, Vector3Par modelPos);

  virtual void SimulateImpulseDamage(
    DoDamageResult &result, EntityAI *owner, float impulse, Vector3Par modelPos
  );
};

class TankWithAI: public Tank
{
  typedef Tank base;

  public:
  TankWithAI( const EntityAIType *name, Person *driver, bool fullCreate = true);
  ~TankWithAI();

  void Simulate( float deltaT, SimulationImportance prec );
#if _ENABLE_AI
  void AIPilot(AIBrain *unit, float deltaT );
  void AIGunner(TurretContextEx &context, float deltaT );
#endif

  virtual LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);
};

#endif

