#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DOOR_HPP
#define _DOOR_HPP

#include "tankOrCar.hpp"

// #include "transport.hpp"
// #include "shots.hpp"
// 
// #include "rtAnimation.hpp"
// 
// #include "Marina/dampers.hpp"
// 
// #include "wounds.hpp"

/// invisible soldier container - used to simulate building doors
class DoorType: public TransportType
{
  typedef TransportType base;
  friend class Door;

protected:

  public:
  DoorType( ParamEntryPar param );
  virtual void Load(ParamEntryPar par, const char *shape);

  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;

  virtual bool ScanTargets() const {return false;}
  virtual bool ScannedAsTarget() const {return false;}
  virtual bool ShowGetIn() const {return false;}
};

/// simulation of wheeled vehicle
// TODO: should have a VisualState?
class Door: public Transport
{
  typedef Transport base;
  
  public:
  Door( const EntityAIType *name, Person *driver );

  const DoorType *Type() const
  {
    return static_cast<const DoorType *>(GetType());
  }

  float Rigid() const {return 0.1;} // how much energy is transfered in collision

  void Simulate( float deltaT, SimulationImportance prec );

  void SuspendedPilot(AIBrain *unit, float deltaT );
  void KeyboardPilot(AIBrain *unit, float deltaT );
  void FakePilot( float deltaT );
#if _ENABLE_AI
  void AIPilot(AIBrain *unit, float deltaT );
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

  USE_CASTING(base)
};

#endif

