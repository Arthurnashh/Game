#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PARACHUTE_HPP
#define _PARACHUTE_HPP

#include "transport.hpp"

#include "rtAnimation.hpp"
#include "global.hpp"


class ParachuteType;

class ParachuteVisualState : public TransportVisualState
{
  typedef TransportVisualState base;
  friend class Parachute;
  friend class ParachuteAuto;

protected:
  float _openState;  /// 0..1: closed (init)<1, 1..2: opening, 2..3: dropping

public:
  ParachuteVisualState(ParachuteType const& type);
  virtual void Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const;
  virtual void Copy(ObjectVisualState const& src) {*this=static_cast_checked<const ParachuteVisualState &>(src);}
  virtual ParachuteVisualState *Clone() const {return new ParachuteVisualState(*this);}
};


class ParachuteType: public TransportType
{
  typedef TransportType base;
  friend class Parachute;
  friend class ParachuteAuto;

  protected:
//  Vector3 _lightPos,_lightDir;
  
  Vector3 _pilotPos; // neutral neck and head positions

/*
  Vector3 _gunPos,_gunDir; // to do: gun position and direction
  Vector3 _gunAxis;
  float _neutralGunXRot;


  float _minGunTurn,_maxGunTurn; // gun movement
  float _minGunElev,_maxGunElev;
*/


  Ref<AnimationRT> _open;
  Ref<AnimationRT> _drop;
  
  public:
  ParachuteType( ParamEntryPar param );
  virtual void Load(ParamEntryPar par, const char *shape);
  void InitShape();
  void DeinitShape();
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);
  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;
  virtual EntityVisualState* CreateVisualState() const { return new ParachuteVisualState(*this); }

  bool CanBeAirborne() const {return true;}

  float GetFieldCost( const GeographyInfo &info, CombatMode mode ) const;
  float GetBaseCost(const GeographyInfo &info, bool operative, bool includeGradient) const;
  float GetGradientPenalty(float gradient) const;
  float GetCostTurn(int difDir, int &debet, bool forceFullTurn) const;
#if _HELISIM_IMPERIAL_SYSTEM
  virtual float ConvertDistance(float meters, int &stringIDS) const;
  virtual float ConvertSpeed(float metersPerSecond) const;
  virtual float ConvertHeight(float height) const;
#endif

};


class Parachute: public Transport
{
  typedef Transport base;

protected:
  // pilot can directly coordinate these parameters
  float _backRotor,_backRotorWanted;

/*
  float _gunYRot,_gunYRotWanted;
  float _gunXRot,_gunXRotWanted;
  float _gunXSpeed,_gunYSpeed;
*/

  Vector3 _turbulence;
  Time _lastTurbulenceTime;

public:
  Parachute( const EntityAIType *name, Person *pilot );
  ~Parachute();
  
  const ParachuteType *Type() const
  {
    return static_cast<const ParachuteType *>(GetType());
  }

  /// @{ Visual state
public:
  friend class ParachuteVisualState;
  ParachuteVisualState typedef VisualState;

  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}

  AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate( int level, bool setEngineStuff );
  bool PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level);

  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;
#if _HELISIM_PRECISE_INTERACTION
  void GetDrawActions3D(UIActions &actions, AIBrain *unit);
#endif

  virtual bool RemoteStateSupported() const {return true;}

  void Simulate( float deltaT, SimulationImportance prec );
  void Sound( bool inside, float deltaT );
  void UnloadSound();

  float GetEngineVol( float &freq ) const;
  float GetEnvironVol( float &freq ) const;

  float Rigid() const {return 0.1;} // how much energy is transfered in collision
  float MaxDeflectionAngleCos() const {return 0;} // never deflect
  bool IsVirtual( CameraType camType ) const {return true;}
  bool IsContinuous( CameraType camType ) const {return true;}
  bool HasFlares( CameraType camType ) const;

  Matrix4 InsideCamera( CameraType camType ) const;
  int InsideLOD( CameraType camType ) const;
  void InitVirtual( CameraType camType, float &heading, float &dive, float &fov ) const;
  void LimitVirtual( CameraType camType, float &heading, float &dive, float &fov ) const;
  float TrackingSpeed() const {return 100;}

  float GetCombatHeight() const {return 30;}
  float GetMinCombatHeight() const {return 30;}
  float GetMaxCombatHeight() const {return 100;}

  bool IsAbleToMove() const;
  bool IsPossibleToGetIn() const;
  bool Airborne() const;

  const char *GetControllerScheme() const {return "Aircraft";}
  void GetAimActions(float &aimX, float &aimY) const override;

  LSError Serialize(ParamArchive &ar);
};

class ParachuteAuto: public Parachute
{
  typedef Parachute base;

  protected:
  float _pilotHeading;
  float _dirCompensate;  // how much we compensate for estimated change
  // esp. when landing

  // helpers for keyboard pilot
  bool _pilotHelper; // keyboard helper activated

  // non-helper interface (joystick)
  Vector3 _lastAngVelocity; // helper for prediction

  public:
  ParachuteAuto( const EntityAIType *name, Person *pilot );

  void Simulate( float deltaT, SimulationImportance prec );
  void AvoidGround( float minHeight );

  void Eject(AIBrain *unit, Vector3Val diff = VZero);

  float MakeAirborne(bool playerIn);

  virtual bool FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock) override;
  virtual bool FireWeaponEffects(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine,EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo) override;

  float FireValidTime() const {return 45;}

  void SuspendedPilot(AIBrain *unit, float deltaT );
  void KeyboardPilot(AIBrain *unit, float deltaT );
  void FakePilot( float deltaT );

  // AI interface
  Matrix4 GunTransform() const;

  virtual bool AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction) override;
  virtual bool AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target) override;
  virtual Vector3 GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual float GetAimed(const TurretContext &context, const WeaponModePtr &ptr, Target *target, bool exact = false, bool checkLockDelay = false) const override;

  virtual float FireInRange(const TurretContext &context, const WeaponModePtr &ptr, float &timeToAim, const Target &target) const override;
  virtual float FireAngleInRange(const TurretContext &context, const WeaponModePtr &ptr, Vector3Par rel) const override;

  void MoveWeapons( float deltaT );

#if _ENABLE_AI
  void AIGunner(TurretContextEx &context, float deltaT );
  void AIPilot(AIBrain *unit, float deltaT );
#endif

  void DrawDiags();
  RString DiagText() const;

  LSError Serialize(ParamArchive &ar);

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

