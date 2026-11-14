#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CAR_HPP
#define _CAR_HPP

#include "tankOrCar.hpp"
#include "transport.hpp"
#include "shots.hpp"
#include "rtAnimation.hpp"
#include "Marina/dampers.hpp"
#include "wounds.hpp"
#include "global.hpp"

struct ScudProxy
{
  OLink(Object) obj;
  int selection; // copied over from proxyObject

  ScudProxy() : obj(NULL) {selection = -1;}
  bool IsPresent() const {return selection >= 0;}
};
TypeContainsOLink(ScudProxy)


class CarType;

class CarVisualState : public TankOrCarVisualState  // TODO: dampers
{
  typedef TankOrCarVisualState base;
  friend class Car;

protected:
  float _wheelPhase, _wheelPhaseDelta;  // delta is for interpolation
  float _scudState;  /// 0..1 = init, 1..2 = launching, 2..3 = ready, 3..4 = starting 
  float _drivingWheelTurn; 
  float _turnBySpeed;

  DamperState _dampers;

public:
  CarVisualState(CarType const& type);
  virtual void Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const;
  virtual void Copy(ObjectVisualState const& src) {*this=static_cast_checked<const CarVisualState &>(src);}
  virtual CarVisualState *Clone() const {return new CarVisualState(*this);}

  float GetWheelRot() const { return _wheelPhase; }
  float GetSCUD() const;
  float GetDrivingWheelPos() const { return floatMinMax(_drivingWheelTurn + _turnBySpeed, -1.0f, +1.0f); }

  virtual float GetDamperState(int index) const { return (index < _dampers.Count()) ? _dampers[index] : 0.0f; }
};


/// properties common to all cars of one type
class CarType: public TankOrCarType
{
  typedef TankOrCarType base;
  friend class Car;
  friend class CarVisualState;
protected:
  float _invCenterToFrontWheelsLength;
  float _turningCenterOffset;
  float _turnCoef;
  float _terrainCoef;

  // dampers
  DamperHolder _dampers;

  WoundTextureSelections _glassDamageHalf;
  WoundTextureSelections _glassDamageFull;

  PlateInfos _plateInfos;

  int _glassRHit;
  int _glassLHit;
  int _bodyHit;
  int _fuelHit;

  /// max. possible turn in one step
  /**
  Used for path planning, 128 = 180 deg
  Derived from _turnCoef
  */
  int _maxTurnOneStep;

  int _wheelLFHit;
  int _wheelRFHit;

  int _wheelLF2Hit;
  int _wheelRF2Hit;

  int _wheelLMHit;
  int _wheelRMHit;

  int _wheelLBHit;
  int _wheelRBHit;

  SoundPars _scudSound;
  SoundPars _scudSoundElevate;

  Ref<LODShape> _scudModel;
  Ref<LODShape> _scudModelFire;

  bool _holdOffroadFormation;

public:
  CarType( ParamEntryPar param );
  virtual void Load(ParamEntryPar par, const char *shape);
  AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source);
  void InitShape();
  void DeinitShape();
  void InitWheels();

  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);

  virtual Object* CreateObject(bool unused) const;
  virtual EntityVisualState* CreateVisualState() const { return new CarVisualState(*this); }

  bool AbstractOnly() const {return false;}

  float GetPathCost(const GeographyInfo &info, float dist, CombatMode mode) const;

  float GetFieldCost(const GeographyInfo &info, CombatMode mode) const;
  float GetBaseCost(const GeographyInfo &info, bool operative, bool includeGradient) const;
  float GetGradientPenalty(float gradient) const;
  float GetCostTurn(int difDir, int &debet, bool forceFullTurn) const;
  FieldCost GetTypeCost(OperItemType type, CombatMode mode, Clearance clearance) const;

  bool HoldOffroadFormation() const {return _holdOffroadFormation;}
};


/// simulation of wheeled vehicle
class Car: public TankOrCar
{
  typedef TankOrCar base;
  
protected:
  Link<AbstractWave> _hornSound;
  RefAbstractWave _scudSound;

  EffectsSourceRT _scudSmoke;
  Vector3 _scudPos;
  Vector3 _scudSpeed;

  RString _plateNumber;
  
  /// when reacting to a collision avoidance, reversing is needed
  bool _reverseNeeded;
  
  /// while turning wheel we want to avoid accelerating
  float _turningWheels;
  /// force reverse state for some time
  float _reverseTimeLeft;
  /// force forward state for some time
  float _forwardTimeLeft;

  TrackOptimizedFour _track;
  
  // simulate gear box
  
  // thrust is relative, in range (-1.0,1.0)
  // actually it is engine RPM?
  float _thrustWanted,_thrust; // accelerate 

  bool _doGearSound;
  /// true when too deep in the water (engine drowned)
  bool _waterDrown;

  /*
  assume steering point the middle of front wheel axis (see CarType)
  _turn specifies side offset of steering point when car moves 1 m forward
  and therefore it can be viewed as tan of angle of front wheels
  */

  float _turnWanted,_turn; // turn
  float _turnIncreaseSpeed,_turnDecreaseSpeed; // keyboard different from stick/auto

  bool _maxTurnReached; 
  /// alternate steering wheel simulation, controlled by controlling the wheel speed
  float _turnSpeedWanted;
  /// used to dim cursor
  Time _turnSpeedWantedLastActive;

  // Driving wheel simulation. It is smoothed over DRIVING_WHEEL_MEMORY interval.
  // /sa VisualState::_drivingWheelTurn
#define DRIVING_WHEEL_MEMORY 10
  float _drivingWheelTurnOld[DRIVING_WHEEL_MEMORY];
  
  /// +1 - forward, -1 backward (cancel)
  float _scudStateDir;
    
public:
  Car(const EntityAIType *name, Person *driver, bool fullCreate = true);

  const CarType *Type() const {return static_cast<const CarType *>(GetType());}

  /// @{ Visual state
  CarVisualState typedef VisualState;

  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}

  /// @{ Visual controllers (\sa Type()->CreateAnimationSource())
  float GetCtrlWheelRot(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetWheelRot(); }
  float GetCtrlSCUD(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetSCUD(); }
  virtual float GetCtrlDrivingWheelPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetDrivingWheelPos(); }
  /// @}

public:
  Vector3 Friction(Vector3Par speed);

  // how much energy is transfered in collision
  float Rigid() const {return 0.1f;}
  bool IsBlocked() const {return FutureVisualState()._scudState >= 1 && FutureVisualState()._scudState < 4;}

  float GetScudState() const {return FutureVisualState()._scudState;}

  virtual void PlaceOnSurface(Matrix4 &trans);
  void MoveWeapons(float deltaT)
  {
    MoveCrewHead();
  }
  void StabilizeTurrets(Matrix3Val oldTrans, Matrix3Val newTrans);

  void Simulate(float deltaT, SimulationImportance prec);

  virtual bool RemoteStateSupported() const {return true;}

  virtual Pair<Vector3,float> ShowDrivingCursor(CameraType camType, Person *person) const;

  Matrix4 InsideCamera(CameraType camType) const;
  Vector3 ExternalCameraPosition(CameraType camType) const;
  // int InsideLOD( CameraType camType ) const;
  bool IsGunner(CameraType camType) const;
#ifndef _VBS2 //we want to have the same optics behavior like tanks
  bool IsContinuous(CameraType camType) const {return false;}
#endif
  bool HasFlares(CameraType camType) const;
  void SimulateHUD(CameraType camType,float deltaT) {}
  virtual void Draw(int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    ) override;
  void PrepareProxiesForDrawing(Object *rootObj, int rootLevel, const AnimationContext &animContext,
    const PositionRender &transform, int level, ClipFlags clipFlags, SortObject *so);

  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  virtual void PerformAction(const Action *action, AIBrain *unit) override;
  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;

  float GetHitForDisplay(int kind) const;

  float OutsideCameraDistance(CameraType camType) const {return 20.0f;}

  float GetGlassBroken() const;

  void DamageAnimation(AnimationContext &animContext, int level, float dist2);
  void DamageDeanimation(int level) {}

  // appearance changed with Animate
  AnimationStyle IsAnimated(int level) const {return AnimatedGeometry;}
  // shadow changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate(int level, bool setEngineStuff);

  virtual const char *GetPlateNumber() const {return _plateNumber;}
  virtual void SetPlateNumber(RString plate) {_plateNumber=plate;}

  void Sound(bool inside, float deltaT);
  void UnloadSound();
  
  /// start tooting
  virtual bool BlowHorn(const TurretContext &context, const WeaponModePtr &ptr);

  bool IsHitEnough() const;
  bool IsAbleToMove() const {return !IsHitEnough() && base::IsAbleToMove();}
  bool IsPossibleToGetIn() const {return !IsHitEnough() && base::IsPossibleToGetIn();}
  bool IsCautious() const;

  float GetEngineVol(float &freq) const;
  float GetEnvironVol(float &freq) const;

  virtual float Thrust() const {return fabs(_thrust);}
  float ThrustWanted() const {return fabs(_thrustWanted);}

  float TrackingSpeed() const {return 80.0f;}

  void PerformFF(FFEffects &eff);
  void Eject(AIBrain *unit, Vector3Val diff=VZero)
  {
    base::Eject(unit, diff);
  }
  void SuspendedPilot(AIBrain *unit, float deltaT);
  void KeyboardPilot(AIBrain *unit, float deltaT);
  void FakePilot(float deltaT);
#if _ENABLE_AI
  void AIPilot(AIBrain *unit, float deltaT);
  virtual bool ReverseLeftRightAIPilot() {return true;}
#endif

  virtual Vector3 SteerPoint(float spdTime, float costTime, Ref<PathAction> *action=NULL, OLink(Object) *building=NULL, Clearance *clearance=NULL, bool updateCost=false);

  RString DiagText() const;

  Vector3 LookIntoTurn(CameraType camType) const;
  Vector3 GetCameraDirection(CameraType camType) const;
  Matrix4 InternalCameraTransform(Matrix4 &baseTr, CameraType camType) const;

  //! Internal state changes when camera change from internal to external or vice versa
  virtual void CamIntExtSwitched();

  virtual LSError Serialize(ParamArchive &ar);
  
  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);   

  USE_CASTING(base)
};

#endif
