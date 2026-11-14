// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"
#include "keyInput.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "AI/ai.hpp"
#include <El/Common/randomGen.hpp>
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>
#include "diagModes.hpp"
#include "dikCodes.h"
#include "fileLocator.hpp"

#include "door.hpp"
#include "Network/network.hpp"
#include "Shape/specLods.hpp"

#include "UI/uiActions.hpp"
#include "stringtableExt.hpp"
#include "camera.hpp"
#include "mbcs.hpp"

#include "paramArchiveExt.hpp"

DoorType::DoorType( ParamEntryPar param )
:base(param)
{
}

void DoorType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  SetHardwareAnimation(par);
}


Object* DoorType::CreateObject(bool unused) const
{
  return new Door(this, NULL);
}


DEFINE_CASTING(Door)

Door::Door( const EntityAIType *name, Person *driver )
:base(name,driver)
{
  SetSimulationPrecision(0.5,0.5); // Door simulation can be very coarse
  RandomizeSimulationTime();
  SetTimeOffset(0.5);
  
  _isStopped = true;

}

/*!
  \patch 5101 Date 12/12/2006 by Ondra
  - Fixed: Door-ground collision response improved.
  \patch 5117 Date 1/11/2007 by Ondra
  - Fixed: Doors (esp. small ones) were able to for a water too deep.
*/

void Door::Simulate( float deltaT, SimulationImportance prec )
{
  PROFILE_SCOPE_EX(simDr,sim);

  _isUpsideDown=false;
  _isDead=false;

  base::Simulate(deltaT,prec);  
  _engineOff = true;

  if (!_isStopped && !CheckPredictionFrozen())
  {

    StopDetected();
  }

  SimulatePost(deltaT,prec);  
}

void Door::FakePilot( float deltaT )
{
}

void Door::SuspendedPilot(AIBrain *unit, float deltaT )
{
}

void Door::KeyboardPilot(AIBrain *unit, float deltaT )
{
}

#if _ENABLE_AI

void Door::AIPilot(AIBrain *unit, float deltaT )
{
  if( unit->GetState()==AIBrain::Stopping )
  {
    unit->OnStepCompleted();
  }
  else if( unit->GetState()==AIBrain::Stopped )
  {
  }
}

#endif // _ENABLE_AI


LSError Door::Serialize(ParamArchive &ar)
{
  CHECK( base::Serialize(ar) );
  
  return LSOK;
}

#define DOOR_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateDoor) \
  XX(UpdatePosition, UpdatePositionDoor)

DEFINE_NETWORK_OBJECT(Door, base, DOOR_MSG_LIST)

#define UPDATE_DOOR_MSG(MessageName, XX)

DECLARE_NET_INDICES_EX_ERR(UpdateDoor, UpdateTransport, UPDATE_DOOR_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateDoor, UpdateTransport, UPDATE_DOOR_MSG, NoErrorInitialFunc)

#define UPDATE_POSITION_DOOR_MSG(MessageName, XX)

DECLARE_NET_INDICES_EX_ERR(UpdatePositionDoor, UpdatePositionVehicle, UPDATE_POSITION_DOOR_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionDoor, UpdatePositionVehicle, UPDATE_POSITION_DOOR_MSG, NoErrorInitialFunc)

NetworkMessageFormat &Door::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_DOOR_MSG(UpdateDoor, MSG_FORMAT_ERR)
    break;
  case NMCUpdatePosition:
    base::CreateFormat(cls, format);
    UPDATE_POSITION_DOOR_MSG(UpdatePositionDoor, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError Door::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateDoor)
    }
    break;
  case NMCUpdatePosition:
    {
      PREPARE_TRANSFER(UpdatePositionDoor)
      TMCHECK(base::TransferMsg(ctx))
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float Door::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += base::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdateDoor)
    }
    break;
  case NMCUpdatePosition:
    {
      error +=  base::CalculateError(ctx);

      PREPARE_TRANSFER(UpdatePositionDoor)
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}
