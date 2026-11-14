#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SHIP_HPP
#define _SHIP_HPP

#include "transport.hpp"
#include "tankOrCar.hpp"
#include "shots.hpp"
#include "global.hpp"

#include <El/ParamFile/paramFile.hpp>


class ShipType;

class ShipVisualState: public TankOrCarVisualState  // TODO: dampers
{
  typedef TankOrCarVisualState base;
  friend class Ship;
  friend class ShipWithAI;

protected:
  float _rotorPosition;
  float _rotorPositionL;
  float _rotorPositionR;
  float _thrustL, _thrustR; // accelerate (left), accelerate (right)

public:
  ShipVisualState(ShipType const& type);
  virtual void Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const;
  virtual void Copy(ObjectVisualState const& src) {*this=static_cast_checked<const ShipVisualState &>(src);}
  virtual ShipVisualState *Clone() const {return new ShipVisualState(*this);}

  float GetRotorPos() const { return _rotorPosition; }
  float GetRotorPosR() const { return _rotorPositionR; }
  float GetRotorPosL() const { return _rotorPositionL; }

  float GetRPM() const { return (fabs(_thrustL) + fabs(_thrustR)) * 0.5f; }
  float GetDrivingWheelPos() const { return 0.5 * (_thrustR - _thrustL); }
};


class ShipType: public TankOrCarType
{
  typedef TankOrCarType base;
  friend class Ship;
  friend class ShipWithAI;

  protected:
  Point3 _pilotPos; // camera position  
  Point3 _gunnerPilotPos; // camera position - gunner view
  Point3 _commanderPilotPos;

  RString _leftEngine, _rightEngine; // propeller effect source
  Vector3 _lEnginePos, _rEnginePos; // propeller effect position

  public:
  ShipType( ParamEntryPar param );
  virtual void Load(ParamEntryPar par, const char *shape);
  AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source);
  void InitShape();
  void DeinitShape();
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);
  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;
  virtual EntityVisualState* CreateVisualState() const { return new ShipVisualState(*this); }

  float GetFieldCost( const GeographyInfo &info, CombatMode mode ) const;
  float GetBaseCost(const GeographyInfo &info, bool operative, bool includeGradient) const;
  float GetGradientPenalty(float gradient) const;
  float GetCostTurn(int difDir, int &debet, bool forceFullTurn) const;
  FieldCost GetTypeCost(OperItemType type, CombatMode mode, Clearance clearance) const;
  void GetPathPrecision(float speedAtCost, float &safeDist, float &badDist) const;
  virtual bool StrategicPlanningOnly() const;
  virtual bool CanUseRoads() const {return false;}

#if _HELISIM_IMPERIAL_SYSTEM
  virtual float ConvertDistance(float meters, int &stringIDS) const;
  virtual float ConvertSpeed(float metersPerSecond) const;
#endif
};


//! all ships (big and small)
class Ship: public TankOrCar
{
  typedef TankOrCar base;
  
protected:
  Vector3 _lastAngVelocity;
  float _randFrequency;

  EffectsSourceRT _leftEngine, _rightEngine;
    
  float _thrustLWanted,_thrustRWanted; // accelerate (left), accelerate (right)
  /// alternate steering simulation, controlled by controlling the wheel speed
  float _turnSpeedWanted;
  /// used to dim cursor
  Time _turnSpeedWantedLastActive;

  float _sink; // current sink status

public:
  Ship( const EntityAIType *name, Person *driver, bool fullCreate = true );

  const ShipType *Type() const
  {
    return static_cast<const ShipType *>(GetType());
  }
  
  /// @{ Visual state
public:
  ShipVisualState typedef VisualState;

  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }
  
  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}

  /// @{ Sound controllers (\sa Transport::Sound())
  virtual float GetRotorPos() const { return FutureVisualState().GetRotorPos(); }
  virtual float Thrust() const { return (fabs(FutureVisualState()._thrustL)+fabs(FutureVisualState()._thrustR))*0.5f; }
  virtual float GetRPM() const { return FutureVisualState().GetRPM(); }
  virtual float GetRenderVisualStateRPM() const { return RenderVisualState().GetRPM(); }
  /// @}

  /// @{ Visual controllers (\sa Type()->CreateAnimationSource())
public:
  float GetCtrlRotorPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRotorPos(); }
  float GetCtrlRotorPosR(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRotorPosR(); }
  float GetCtrlRotorPosL(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRotorPosL(); }
  float GetCtrlRPM(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRPM(); }
  virtual float GetCtrlDrivingWheelPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetDrivingWheelPos(); }
  /// @}

public:
  Vector3 DragFriction( Vector3Par speed );
  Vector3 DragForce( Vector3Par speed );

  void MoveWeapons(float deltaT);
  float Rigid() const {return 0.1;}

  virtual bool CanFloat() const {return true;}

  virtual bool RemoteStateSupported() const {return true;}

  Pair<Vector3,float> ShowDrivingCursor(CameraType camType, Person *person) const;

  void Simulate( float deltaT, SimulationImportance prec );
  virtual void StabilizeTurrets(Matrix3Val oldTrans, Matrix3Val newTrans);

  float GetHitForDisplay(int kind) const;

  float GetEngineVol( float &freq ) const;
  float GetEnvironVol( float &freq ) const;

  bool IsVirtual( CameraType camType ) const;

  float OutsideCameraDistance( CameraType camType ) const
  {
    return floatMax(20,GetShape()->GeometrySphere()*2);
  }

  AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate( int level, bool setEngineStuff );

  float TrackingSpeed() const {return 80;}

  float ThrustWanted() const;

  void Eject(AIBrain *unit, Vector3Val diff = VZero);

  GetInPoint GetDriverGetInPos(Person *person, Vector3Par pos) const;
  GetInPoint GetCommanderGetInPos(Person *person, Vector3Par pos) const;
  GetInPoint GetGunnerGetInPos(Person *person, Vector3Par pos) const;
  GetInPoint GetTurretGetInPos(Turret *turret, Person *person, Vector3Par pos) const;
  GetInPoint GetCargoGetInPos(int cargoIndex, Person *person, Vector3Par pos) const;
  
  void SuspendedPilot(AIBrain *unit, float deltaT );
  void KeyboardPilot(AIBrain *unit, float deltaT );
  void FakePilot( float deltaT );
#if _ENABLE_AI
  void AIPilot(AIBrain *unit, float deltaT ){}
#endif

  virtual float GetSteerAheadSimul() const;
  virtual float GetSteerAheadPlan() const;
  virtual float GetPrecision() const;

  bool HasHUD() const {return true;}
  const char *GetControllerScheme() const {return "Ground";}
  void GetAimActions(float &aimX, float &aimY) const override;

  virtual void ResetStatus();

  LSError Serialize(ParamArchive &ar);
};

class ShipWithAI: public Ship
{
  typedef Ship base;

public:
  enum StopState
  {
    SSNone,
    SSFindPath,
    SSMove,
    SSStop
  };

protected:
  Vector3 _stopPosition;
  StopState _stopState;

public:
  ShipWithAI( const EntityAIType *name, Person *driver, bool fullCreate = true );
  ~ShipWithAI();

  void Simulate( float deltaT, SimulationImportance prec );
#if _ENABLE_AI
  void AIPilot(AIBrain *unit, float deltaT );
#endif

  Vector3 GetStopPosition() const;

  LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);

protected:
  bool FindStopPosition();
  void Autopilot(AIBrain *unit, SteerInfo &info);
};

#endif
