#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MOTORCYCLE_HPP
#define _MOTORCYCLE_HPP

#include "tankOrCar.hpp"

#include "transport.hpp"
#include "shots.hpp"

#include "rtAnimation.hpp"


#include "wounds.hpp"

#include "Marina/dampers.hpp"
#include "global.hpp"


class MotorcycleType;

class MotorcycleVisualState : public TankOrCarVisualState  // TODO: dampers
{
  typedef TankOrCarVisualState base;
  friend class Motorcycle;

protected:
  float _support;
  float _wheelPhase, _wheelPhaseDelta;  // delta: for interpolation
  float _thrust;
  float _turn, _turnBySpeed;

  DamperState _dampers;

public:
  MotorcycleVisualState(MotorcycleType const& type);
  virtual void Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const;
  virtual void Copy(ObjectVisualState const& src) {*this=static_cast_checked<const MotorcycleVisualState &>(src);}
  virtual MotorcycleVisualState *Clone() const {return new MotorcycleVisualState(*this);}

  float GetSupportPos() const { return _support; }
  float GetPedalPos(Ref<Person> driver) const { if (driver) return driver->GetLegPhase(); else return _wheelPhase; }
  float GetWheelRot() const { return _wheelPhase; }
  float GetRPM(MotorcycleType const& type) const;
  float GetDrivingWheelPos() const { return (1.0f / 30.0f) * floatMinMax(_turn + _turnBySpeed, -1, +1); }

  virtual float GetDamperState(int index) const { return (index < _dampers.Count()) ? _dampers[index] : 0.0f; }
};


/// type of motorcycle (used for bicycles as well)
class MotorcycleType: public TankOrCarType
{
  typedef TankOrCarType base;
  friend class Motorcycle;
  friend class MotorcycleVisualState;

  protected:
  
  float _turnCoef;
  float _terrainCoef;

  bool _isBicycle;

  DamperHolder _dampers;
  
  WoundTextureSelections _glassDamageHalf;
  WoundTextureSelections _glassDamageFull;

  PlateInfos _plateInfos;

  int _glassRHit;
  int _glassLHit;
  int _bodyHit;
  int _fuelHit;

  int _wheelFHit;
  int _wheelBHit;

  public:
  MotorcycleType( ParamEntryPar param );
  virtual void Load(ParamEntryPar par, const char *shape);
  void InitShape();
  void DeinitShape();
  
  AnimationSource *CreateAnimationSource(RStringB source);
  AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source);
  AnimationSource *CreateAnimationSourcePar(const AnimationType *type, ParamEntryPar entry);
  
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);
  
  virtual Object* CreateObject(bool unused) const;
  virtual EntityVisualState* CreateVisualState() const { return new MotorcycleVisualState(*this); }
  
  void InitWheels(int level);
  
  bool AbstractOnly() const {return false;}

  float GetPathCost(const GeographyInfo &info, float dist, CombatMode mode) const;

  float GetFieldCost( const GeographyInfo &info, CombatMode mode ) const;
  float GetBaseCost(const GeographyInfo &info, bool operative, bool includeGradient) const;
  float GetGradientPenalty(float gradient) const;
  float GetCostTurn(int difDir, int &debet, bool forceFullTurn) const;

};


/// basic two-wheel vehicle simulation
class Motorcycle: public TankOrCar
{
  typedef TankOrCar base;

protected:
  Link<AbstractWave> _hornSound;

  RString _plateNumber;
  
  float _reverseTimeLeft; // force reverse state
  float _forwardTimeLeft; // force forward state

  TrackOptimized _track;
  
  bool _waterDrown;
  // simulate gear box
  
  // thrust is relative, in range (-1.0,1.0)
  // actually it is engine RPM?
  float _thrustWanted; // accelerate 

  // assume steering point the middle of front wheel axe (see MotorcycleType)
  // _turn specifies side offset of steering point
  // when car moves 1 m forward - and therefore it can be viewed
  // as tangens of angle of front wheels
  // each time angular velocity change is calculated,
  // actual radial velocity of

  float _turnWanted; // turn

  float _turnIncreaseSpeed,_turnDecreaseSpeed; // keyboard different from stick/auto
  
  /// alternate steering wheel simulation, controlled by controlling the wheel speed
  float _turnSpeedWanted;
  /// used to dim cursor
  Time _turnSpeedWantedLastActive;

  Time _allowEjectAfter; // after get in does not eject drivers wait a while

public:
  Motorcycle( const EntityAIType *name, Person *driver, bool fullCreate = true);

  const MotorcycleType *Type() const
  {
    return static_cast<const MotorcycleType *>(GetType());
  }

  /// @{ Visual state
public:
  MotorcycleVisualState typedef VisualState;

  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}
  
  /// @{ Visual controllers (\sa Type()->CreateAnimationSource())
public:
  float GetCtrlSupportPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetSupportPos(); }
  float GetCrtlPedalPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetPedalPos(_driver); }
  float GetCtrlWheelRot(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetWheelRot(); }
  virtual float GetCtrlRPM(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRPM(*Type()); }
  virtual float GetCtrlDrivingWheelPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetDrivingWheelPos(); }
  /// @}

public:
  virtual float GetRPM() const { return FutureVisualState().GetRPM(*Type()); }
  virtual float GetRenderVisualStateRPM() const { return RenderVisualState().GetRPM(*Type()); }

  Vector3 Friction( Vector3Par speed );

  float Rigid() const {return 0.3;} // how much energy is transfered in collision
  bool IsBlocked() const;
  void PlaceOnSurface(Matrix4 &trans);

  virtual bool RemoteStateSupported() const {return true;}

  bool IsStopped() const;
  void MoveWeapons(float deltaT);
  void SimulateOneIter( float deltaT, SimulationImportance prec );
  void Simulate( float deltaT, SimulationImportance prec );
  virtual void StabilizeTurrets(Matrix3Val oldTrans, Matrix3Val newTrans);
  Pair<Vector3,float> ShowDrivingCursor(CameraType camType, Person *person) const;
  Matrix4 InsideCamera( CameraType camType ) const;
  Vector3 ExternalCameraPosition( CameraType camType ) const;
  // int InsideLOD( CameraType camType ) const;
  bool IsGunner( CameraType camType ) const;
  bool IsContinuous( CameraType camType ) const;
  bool HasFlares( CameraType camType ) const;
  void SimulateHUD(CameraType camType,float deltaT);
  virtual void Draw(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  ) override;

#if _HELISIM_PRECISE_INTERACTION
  void GetDrawActions3D(UIActions &actions, AIBrain *unit);
#endif

  //float OutsideCameraDistance( CameraType camType ) const {return 8.3;}
  float OutsideCameraDistance( CameraType camType ) const {return 20;}

  float GetGlassBroken() const;

  float GetHitForDisplay(int kind) const;

  void DamageAnimation(AnimationContext &animContext, int level, float dist2);
  void DamageDeanimation( int level );

  AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate( int level, bool setEngineStuff );

  virtual const char *GetPlateNumber() const {return _plateNumber;}
  virtual void SetPlateNumber( RString plate ) {_plateNumber=plate;}

  void Sound( bool inside, float deltaT );
  void UnloadSound();

  bool IsPossibleToGetIn() const;
  bool IsAbleToMove() const;
  bool IsCautious() const;

  float GetEngineVol( float &freq ) const;
  float GetEnvironVol( float &freq ) const;

  float DriverAnimSpeed() const;
  float CargoAnimSpeed(int position) const;

  float Thrust() const {return fabs(FutureVisualState()._thrust);}
  float ThrustWanted() const {return fabs(_thrustWanted);}

  float TrackingSpeed() const {return 80;}

  void Eject(AIBrain *unit, Vector3Val diff = VZero);
  void SuspendedPilot(AIBrain *unit, float deltaT );
  void KeyboardPilot(AIBrain *unit, float deltaT );
  void FakePilot( float deltaT );
#if _ENABLE_AI
  void AIPilot(AIBrain *unit, float deltaT );
#endif

  void GetInDriver( Person *driver, bool sound=true, bool isFastMoveIn = false);
  void GetInCargo( Person *driver, bool sound=true, int posIndex=-1, bool isFastMoveIn = false);
  //! Internal state changes when camera change from internal to external or vice versa
  virtual void CamIntExtSwitched();

  RString DiagText() const;

  virtual bool FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock) override;
  virtual bool ProcessFireWeapon(
    const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo, RemoteFireWeaponInfo *localInfo, bool forceLock
  ) override;
  virtual bool FireWeaponEffects(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine,EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo) override;
  Vector3 GetCameraDirection( CameraType camType ) const;

  float NeedsRefuel() const;

  // horn is always aimed
  virtual bool AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction) override;
  virtual bool AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target) override;
  virtual Vector3 GetWeaponDirectionWanted(const TurretContextEx &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponCenter(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;

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

