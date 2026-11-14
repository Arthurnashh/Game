#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PLANE_OR_HELI_HPP
#define _PLANE_OR_HELI_HPP

/*!
\file
interface for PlaneOrHeli class
*/

#include "transport.hpp"

class PlaneOrHeli: public Transport
{
  public:
  enum StopPositionResult
  {
    SPFound,
    SPNotFound,
    SPNotReady
  };
  enum StopMode
  {
    SMNone,
    SMLand,
    SMGetIn,
    SMGetOut
  };


  private:
  typedef Transport base;

  protected:
  Vector3 _stopPosition;
  StopMode _stopMode;
  StopPositionResult _stopResult;
  
  public:
  PlaneOrHeli(const EntityAIType *name, Person *driver, bool fullCreate = true);
  StopPositionResult FindStopPosition();
  void UpdateStopMode(AIBrain *unit);

  int GetLastMissileIndex(const TurretContext &context, const WeaponModePtr &ptr) const;

  StopPositionResult GetLandResult() const {return _stopResult;}
  virtual void LandStarted(LandingMode landing);
  virtual void ResetAutopilot(){}

  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;
#if _HELISIM_PRECISE_INTERACTION
  void GetDrawActions3D(UIActions &actions, AIBrain *unit);
#endif
};

/// functor searching copilots
class HasCopilot : public ITurretFunc
{
public:
  bool operator () (EntityAIFull *entity, TurretContext &context);
};
#endif
