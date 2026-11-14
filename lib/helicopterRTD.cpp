#include "wpch.hpp"

#include "scene.hpp"
#include "world.hpp"
#include "keyInput.hpp"

#include "stringtableExt.hpp"
#include "paramArchiveExt.hpp"

#include "diagModes.hpp"
#include "dikCodes.h"

#include <El/Common/randomGen.hpp>
#include <El/Common/perfProf.hpp>

#include "Network/network.hpp"

#include <El/Enum/enumNames.hpp>

#include "mbcs.hpp"
#include "progress.hpp"

#if _HELISIM_ROTORLIB && !_DUMMY_ROTORLIB

#include "helicopterRTD.hpp"


/*********************************************************************************************************/
ForceGeneratorRTD::ForceGeneratorRTD()
{
  _strength = Vector3(0,0,0);
  _position = Vector3(0,0,0);
  _radius = Vector3(0,0,0);
  _orientation = Vector3(1,0,0);
  _attachedTo = NULL;
  _type = 0;
  _enabled = false;
}

ForceGeneratorRTD::ForceGeneratorRTD(bool _enabled, Vector3 strength, Vector3 position, Vector3 radius,
               Vector3 orientation, Ref<Object> attachedTo,int type)
{
  _enabled = false;
  _strength = strength;
  _position = position;
  _radius = radius;
  _orientation = orientation;
  _attachedTo = attachedTo;
  _type = type;
}

LSError ForceGeneratorRTD::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("enabled",_enabled,1,true))
  CHECK(::Serialize(ar, "strength", _strength, 1))
  CHECK(::Serialize(ar, "position", _position, 1))
  CHECK(::Serialize(ar, "radius", _radius, 1))
  CHECK(::Serialize(ar, "orientation", _orientation, 1))

  CHECK(ar.SerializeRef("attachedTo",_attachedTo,1))
  CHECK(ar.Serialize("type",_type,1,0))
  return LSOK;
}
/*********************************************************************************************************/

//derived GenHeli600 with added Arma serialization and network layer
GenHeli600Arma::GenHeli600Arma(	RTD::World* world, HelicopterRTD *parent, const std::string& name, ConfigParams* params)
:GenHeli600(world, name, params)
{
  _parent = parent;
  Init(name);
}

GenHeli600Arma::GenHeli600Arma( RTD::World* world, HelicopterRTD *parent, const std::string& name, const std::string& parameterFile)
:GenHeli600(world, name, parameterFile)
{
  _parent = parent;
  Init(name);
}


void GenHeli600Arma::Init(const std::string& name)
{
  _horizontalStabilizerRight = NULL;
  _horizontalStabilizerLeft = NULL;

  // copy engine default settings, set engines off
  for (int i=0; i< getNumEngines(); i++)
  {
    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
    if(engine)
    {
      _engineMaxPower.Add(engine->getConfigParams()->m_maxPower);
      _targetRPM.Add(engine->getConfigParams()->m_targetRPM);
      _throttle.Add(0.0f);

      _engineMaxTorque.Add(engine->getConfigParams()->m_maxTorque);
      //hot fix for inserting airbourne helicopter
     // engine->getConfigParams()->m_maxTorque = 100000;

      _engineControlleroffset.Add(engine->getConfigParams()->m_offset);

      engine->getConfigParams()->m_maxPower = 0;
      engine->getConfigParams()->m_offset = 0;
      engine->setTargetRPM(0);

      _startingTime.Add(0.0f);
      _rpmChangePerSec.Add(0.0f);
      _wantedRPM.Add(0.0f);
    }
  }
  _brakesLeft = 1.0f; //brakes on
  _brakesRight = 1.0f;
  _rotorBrake = 0.0f;
  _driveTrainMainDamaged = _driveTrainTailDamaged = 0;

  _engineTorque = 0.0f;
  _superCharging = false;

  // copy wings default settings, set engines off
  for(int i=0; i< getNumStabilizers(); i++)
  {
    if(getStabilizer(i))
    {
      _stabilizersSize.Add(getStabilizer(i)->getConfigParams()->surfaceArea);

      if(getStabilizer(i)->getName() == "HorizontalStabilizerRight") _horizontalStabilizerRight = getStabilizer(i) ;
      if(getStabilizer(i)->getName() == "HorizontalStabilizerLeft")  _horizontalStabilizerLeft  = getStabilizer(i);
    }
    else _stabilizersSize.Add(0);

  }

  //force generatos
  char forceName[32];
  strcpy(forceName,name.data());	
  strcat (forceName,"ext");
  externalForce = new RTD::FGForce(forceName, this);

  strcpy(forceName,name.data());	
  strcat (forceName,"int");
  loadForce = new RTD::FGForce(forceName, this);

  m_forceGens.add(externalForce); //for designers
  m_forceGens.add(loadForce); //sling load

  if(getMainRotor()) getMainRotor()->reset();
  if(getTailRotor()) getTailRotor()->reset();

  _enginesDamage = Vector3(1.0f,1.0f,1.0f);

  setEnabled(false);  //enable only for player
}

void GenHeli600Arma::RemoveForces()
{
  m_forceGens.removeAll();

  delete loadForce;
  delete externalForce;
}

void GenHeli600Arma::SetSlingLoadForce(Vector3 slingloadForce)
{
  if(loadForce)
  {
    loadForce->setForce(smVec3r(slingloadForce.X(),slingloadForce.Z(),slingloadForce.Y()));
  }
}

void GenHeli600Arma::setEnabled( bool enabled )
{
  GenHeli600::setEnabled(enabled);
}

void GenHeli600Arma::update(smTime timestep)
{
  if (_superCharging)
  {
    GenHeli600::update(timestep);
    GenHeli600::update(timestep);
    GenHeli600::update(timestep);
  }
  else GenHeli600::update(timestep);
}

void GenHeli600Arma::preUpdate( smTime stepTime )
{
  if (_superCharging) 
  {
    GenHeli600::preUpdate(stepTime);
    GenHeli600::preUpdate(stepTime);
    GenHeli600::preUpdate(stepTime);
  }
  else GenHeli600::preUpdate(stepTime);
}

void GenHeli600Arma::postUpdate( smTime stepTime )
{
  if (_superCharging)
  {
    GenHeli600::postUpdate(stepTime);
    GenHeli600::postUpdate(stepTime);
    GenHeli600::postUpdate(stepTime);
  }
  else GenHeli600::postUpdate(stepTime);
}

#define STARTER_POWER 0.25f
void GenHeli600Arma::Simulate(float deltaT, EntityAI *parent,  bool elPower, int starter, float fuel,  bool superCharge)
{
  //we need fuel or by electricity driven starter
  int hasFuel = (fuel > 0) ? 1:0;
  float averageEngineDamage = (_enginesDamage[0] + _enginesDamage[1] + _enginesDamage[2]) / 3.0f;
  if(!getMainRotor()) return;
  float mainRotorRPMScale =  getMainRotor()->getRPM() / getMainRotor()->getDesignRPM();
  _superCharging = superCharge;;

  // 3 engines are maximum
  if(_superCharging)
  {
    for (int i=0; i< getNumEngines() && i < 3; i++)
    {
      RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
      if(!engine) continue;

      engine->getConfigParams()->m_maxPower = _engineMaxPower[i] ;
      engine->getConfigParams()->m_offset = _engineControlleroffset[i];

      engine->setTargetRPM(_targetRPM[i]);
      engine->getConfigParams()->m_maxTorque =  _engineMaxTorque[i];

      _startingTime[i] = -1;
    }
  }
  else
  {
    // 3 engines are maximum
    for (int i=0; i< getNumEngines() && i < 3; i++)
    {
      RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
      if(!engine) continue;
      bool starterOn = (elPower && (starter & (1 << i)) > 0);

      engine->getConfigParams()->m_maxTorque = _engineMaxTorque[i];
      float power = floatMin(_engineMaxPower[i] , hasFuel * _throttle[i] * _engineMaxPower[i] + (starterOn ? (_engineMaxPower[i] * STARTER_POWER):0) );
      //force using of starter to gain some RPM
      if(mainRotorRPMScale < 0.1 && !starterOn) power = 0;

      //apply engine damage - (at this moment we have only 3 hit-zones for engines)
      if (i < 3) power *= _enginesDamage[i];
      else power *= averageEngineDamage; //if over, use average (if hit1-3 is destroyed, destroy 4-n)

      engine->getConfigParams()->m_maxPower = power;
      engine->getConfigParams()->m_offset = (power / _engineMaxPower[i]) * _engineControlleroffset[i];

      if (_startingTime[i] < 0) continue;
      _startingTime[i] -= deltaT;

      //no fuel, only starter running - limit max rpm
      if((!hasFuel || _throttle[i] < 0.001) && (_wantedRPM[i] > _targetRPM[i] * (STARTER_POWER+ 0.01))) 
        SetWantedEngineRPM(_targetRPM[i] * STARTER_POWER, 1, i);

      float wantedRPM = 0;
      if (_startingTime[i] < 0) wantedRPM = _wantedRPM[i];
      else  wantedRPM = engine->getTargetRPM() + _rpmChangePerSec[i] * deltaT;
      engine->setTargetRPM(wantedRPM);      
    }
  }

  if(externalForce)
  {
    const AutoArray<ForceGeneratorRTD> &forces = GWorld->GetExternalForceGeneratorsRTD();
    Vector3 finalForce = Vector3(0,0,0);
    for (int i = 0; i< forces.Size(); i++)
    {
      ForceGeneratorRTD force = forces[i];
      if(!force._enabled) continue;

      if(force._attachedTo == parent)
      {
        finalForce+= force._strength;
      }
      else if(force._type == 1)
      {
        float distance = force._position.Distance(parent->FutureVisualState().Position());
        if (distance > force._radius[0] || force._radius[0]<=0) continue;

        Vector3 dir = parent->FutureVisualState().Position() - force._position; dir[2] = 0;
        dir.Normalize();
        Vector3 forceRot = Vector3(); 

        forceRot[0] =  force._strength.X() * dir.X() + force._strength.Z() * dir.Z();
        forceRot[1] =  force._strength.Y();
        forceRot[2] =  force._strength.X() * dir.Z() - force._strength.Z() * dir.X();

        finalForce+= ((force._radius[0] - distance) / force._radius[0]) * forceRot;
      }
      else if(force._type == 2)
      {
        float distance = force._position.Distance(parent->FutureVisualState().Position());
        if (distance > force._radius[0] || force._radius[0]<=0) continue;
        finalForce+= ((force._radius[0] - distance) / force._radius[0]) * force._strength;
      }
    }
    externalForce->setForce(smVec3r(finalForce[0],finalForce[2],finalForce[1]));
  }
}


LSError GenHeli600Arma::Serialize(ParamArchive &ar)
{
  std::vector<unsigned char> result;
  std::vector<unsigned char> remainer;

  CHECK(ar.SerializeArray("engineMaxPower", _engineMaxPower, 1))
  CHECK(ar.SerializeArray("engineMaxTorque", _engineMaxTorque, 1))

  CHECK(ar.SerializeArray("targetRPM", _targetRPM, 1))
  CHECK(ar.SerializeArray("stabilizersSize", _stabilizersSize, 1))

  CHECK(ar.SerializeArray("engineControlleroffset", _engineControlleroffset, 1))

  CHECK(ar.SerializeArray("rpmChangePerSec", _rpmChangePerSec, 1))
  CHECK(ar.SerializeArray("wantedRPM",_wantedRPM,1))
  CHECK(ar.SerializeArray("startingTime",_startingTime,1))
  CHECK(ar.Serialize("brakesLeft",_brakesLeft,1,0))
  CHECK(ar.Serialize("brakesRight",_brakesRight,1,0))
  CHECK(ar.Serialize("rotorBrake",_rotorBrake,1,0))

  CHECK(ar.Serialize("driveTrainMainDamaged", _driveTrainMainDamaged, 1, 0))
  CHECK(ar.Serialize("driveTrainTailDamaged", _driveTrainTailDamaged, 1, 0))

  CHECK(::Serialize(ar, "enginesDamage", _enginesDamage, 1))

  if(ar.IsLoading())
  {
    if(ar.GetPass() == ParamArchive::PassFirst)
    {
      AutoArray<int> dataRTD;
      CHECK(ar.SerializeArray("dataRTD", dataRTD, 1))

      int size =  (dataRTD.Size() * sizeof(int)) / sizeof(unsigned char) + 1;
      result.reserve(size);
      result.resize(size);

      memset(&result[0], 0, size * sizeof(unsigned char));
      memcpy(&result[0], dataRTD.Data(), dataRTD.Size() * sizeof(int));
      base::deserialize(result, remainer);

      AutoArray<float> engineControlleroffsetActual;
      CHECK(ar.SerializeArray("engineControlleroffsetActual", engineControlleroffsetActual, 1))

      AutoArray<float> targetRPMActual;
      CHECK(ar.SerializeArray("targetRPMActual", targetRPMActual, 1))
      CHECK(ar.SerializeArray("throttleRTD",  _throttle, 1))

      for (int i=0; i< getNumEngines(); i++)
      {
        RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
        if(!engine) continue;

        engine->setTargetRPM(targetRPMActual[i]);
        engine->getConfigParams()->m_maxPower = _throttle[i] * _engineMaxPower[i];
        engine->getConfigParams()->m_offset = engineControlleroffsetActual[i];
      }

      AutoArray<float> stabilizersSizeActual;
      CHECK(ar.SerializeArray("engineControlleroffsetActual", stabilizersSizeActual, 1))

        // copy wings default settings, set engines off
        for(int i=0; i< getNumStabilizers() && i < stabilizersSizeActual.Size(); i++)
        {
          if(getStabilizer(i)) getStabilizer(i)->getConfigParams()->surfaceArea = stabilizersSizeActual[i];
        }
    }
  }
  else
  {
    //save
    base::serialize(result);
    AutoArray<int> dataRTD;
    int size =  (result.size() * sizeof(unsigned char)) / sizeof(int) + 1;
    dataRTD.Realloc(size);
    dataRTD.Resize(size);
 
    memset(dataRTD.Data(), 0, size * sizeof(int));
    memcpy(dataRTD.Data(), &result[0],  result.size() * sizeof(unsigned char));
    CHECK(ar.SerializeArray("dataRTD", dataRTD, 1))

    AutoArray<float> engineControlleroffsetActual;
    AutoArray<float> targetRPMActual;
    for (int i=0; i< getNumEngines(); i++)
    {
      RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
      if(!engine) continue;
      engineControlleroffsetActual.Add(engine->getConfigParams()->m_offset);
      targetRPMActual.Add(engine->getTargetRPM());
    }
    CHECK(ar.SerializeArray("engineControlleroffsetActual", engineControlleroffsetActual, 1))
    CHECK(ar.SerializeArray("targetRPMActual", targetRPMActual, 1))
    CHECK(ar.SerializeArray("throttleRTD",  _throttle, 1))

    AutoArray<float>  stabilizersSizeActual;
    // copy wings default settings, set engines off
    for(int i=0; i< getNumStabilizers(); i++)
    {
      if(getStabilizer(i)) stabilizersSizeActual.Add( getStabilizer(i)->getConfigParams()->surfaceArea);
      else stabilizersSizeActual.Add(0);
    }

    CHECK(ar.SerializeArray("engineControlleroffsetActual", stabilizersSizeActual, 1))

  }
  return LSOK;
}

void GenHeli600Arma::EngineOn()
{
  //for (int i=0; i< getNumEngines(); i++)
  //{
  //  RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
  //  if(!engine) continue;
  //}
}

void GenHeli600Arma::EngineOn(float quickStart)
{
  for (int i=0; i< getNumEngines(); i++)
  {
    _startingTime[i] = quickStart;
    _wantedRPM[i] = _targetRPM[i];
    _rpmChangePerSec[i] = (quickStart > 0)? _targetRPM[i] / quickStart : _targetRPM[i];

    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
    if(!engine) continue;

    //engine->start();

    if(quickStart<=0)
    {
      _throttle[i] = 1.0f;
      engine->setRPM(0.95 * _wantedRPM[i]);
      engine->setTargetRPM(_wantedRPM[i]);
      engine->getConfigParams()->m_maxPower = _throttle[i] * _engineMaxPower[i];
    }
    else
    {
        _throttle[i] = 1.0f;
        engine->getConfigParams()->m_maxPower = _throttle[i] * _engineMaxPower[i];
    }
  }
}

void GenHeli600Arma::EngineOff()
{
  for (int i=0; i< getNumEngines(); i++)
  {
    _startingTime[i] = 0.0;
    _wantedRPM[i] = 0.0f;
    _rpmChangePerSec[i] = 0.0f;

    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
    if(!engine) continue;

    //engine->stop();
    _throttle[i] = 0.0f;
    engine->getConfigParams()->m_maxPower = _throttle[i] * _engineMaxPower[i];
    engine->getConfigParams()->m_offset = 0;
    engine->setTargetRPM(0);
  }

  //StopRotorsRTD(10.0f);
}

bool GenHeli600Arma::EngineIsOn()
{
  for (int i=0; i< getNumEngines(); i++)
  {
    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
    if(!engine) continue;
    if(engine->getConfigParams()->m_maxPower > 0.0001) return true;
  }
  return false;
}

bool GenHeli600Arma::EngineIsOn(int index)
{
  if(index >=0 && index < getNumEngines())
  {
    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(index);
    if(!engine) return false;
    if(engine->getConfigParams()->m_maxPower > 0.0001) return true;
  }
  return false;
}

///set engine throttle
void GenHeli600Arma::SetThrottle(float value, int index) 
{
  saturate(value, 0.0, 1.0);
  if(index>=0 && index<getNumEngines())
  {
    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(index);
    if(engine) 
    {
      _throttle[index] = value;
      engine->getConfigParams()->m_maxPower = value * _engineMaxPower[index];
    }
  }
  else if(index < 0)
  {
    for (int i =0; i<= getNumEngines(); i++)
    {
      RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
      if(engine) 
      {
        engine->getConfigParams()->m_maxPower =  value * _engineMaxPower[i];
        _throttle[i] = value;
      }
    }
  }
};


void GenHeli600Arma::SetWantedEngineRPM(float wantedEngineRPM, float time, int index)
{
  if(index < 0)
  {
    for (int i = 0; i<  getNumEngines(); i++)
    {
      RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
      if(!engine) continue;

      _wantedRPM[i] = wantedEngineRPM;
      saturate(_wantedRPM[i], 0, _targetRPM[i]);
      _startingTime[i] = time;

      _rpmChangePerSec[i] = (wantedEngineRPM - engine->getTargetRPM()) / time;  
    }
  }
  else if(index < _targetRPM.Size())
  {
    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(index);
    if(engine)
    {
      _wantedRPM[index] = wantedEngineRPM;
      saturate(_wantedRPM[index], 0, _targetRPM[index]);
      _startingTime[index] = time;

      _rpmChangePerSec[index] = (wantedEngineRPM - engine->getTargetRPM()) / time;  
    }
  }
}

void GenHeli600Arma::InstantStop() 
{
  bool enabled = isEnabled();
  EngineOff();
  reset();
  setEnabled(enabled);
};



///set rotors effectivity 
void GenHeli600Arma::SetMainRotorDamage(float valueMain) 
{
  if(valueMain >= 0 && getMainRotor())
  {
    RTD::GenericDriveTrain* driveTrain = static_cast<RTD::GenericDriveTrain*>( getDriveTrain(0) );
    RTD::FGRotorBladeElementTheory* mainRotor = (RTD::FGRotorBladeElementTheory*)getMainRotor();

    if(driveTrain && mainRotor)
    {
      if(valueMain < 0.1) 
      {
        mainRotor->setForceScale(0.0);
        mainRotor->setTorqueScale(0.0);
        _driveTrainMainDamaged = _driveTrainMainDamaged | 2;
      }
      else if(valueMain < 0.5)
      {    
        mainRotor->setForceScale(1.0);
        mainRotor->setTorqueScale(1.0);
        _driveTrainMainDamaged = _driveTrainMainDamaged | 2;
      }
      else
      {
        mainRotor->setForceScale(1.0);
        mainRotor->setTorqueScale(1.0);
        _driveTrainMainDamaged = _driveTrainMainDamaged & 1;
      }

      if(_driveTrainMainDamaged >  0 && driveTrain->getDriveTrainConnectionStatus("MainRotor") == true) driveTrain->setDriveTrainConnectionStatus("MainRotor" , false );
      if(_driveTrainMainDamaged == 0 && driveTrain->getDriveTrainConnectionStatus("MainRotor") == false) driveTrain->setDriveTrainConnectionStatus("MainRotor" , true );
    }
  }
};

void GenHeli600Arma::SetTailRotorDamage(float valueTail) 
{
  if(valueTail >= 0 && getTailRotor())
  {
    RTD::GenericDriveTrain* driveTrain = static_cast<RTD::GenericDriveTrain*>( getDriveTrain(0) );

    if(driveTrain)
    {
      if(valueTail < 0.5)
      {
        _driveTrainTailDamaged = _driveTrainTailDamaged | 2;
      }
      else
      {
        _driveTrainTailDamaged = _driveTrainTailDamaged & 1;
      }

 
      if(_driveTrainTailDamaged > 0 && driveTrain->getDriveTrainConnectionStatus("TailRotor") == true) 
      {
        driveTrain->setDriveTrainConnectionStatus("TailRotor" , false );
      }
      if(_driveTrainTailDamaged == 0 && driveTrain->getDriveTrainConnectionStatus("TailRotor") == false) 
      {
        driveTrain->setDriveTrainConnectionStatus("TailRotor" , true );
      }
    }
  }

};

void GenHeli600Arma::SetTransmissionDamage(float damage)
{
  RTD::GenericDriveTrain* driveTrain = static_cast<RTD::GenericDriveTrain*>( getDriveTrain(0) );
  if(driveTrain)
  {
    //not best solution, but i don't have better
    if(damage < 0.1)
    {
      _driveTrainMainDamaged = _driveTrainMainDamaged | 1;
      _driveTrainTailDamaged = _driveTrainTailDamaged | 1;
    }
    else
    {
      _driveTrainMainDamaged = _driveTrainMainDamaged & 2;
      _driveTrainTailDamaged = _driveTrainTailDamaged & 2;
    }

    if(_driveTrainMainDamaged >  0 && driveTrain->getDriveTrainConnectionStatus("MainRotor") == true) driveTrain->setDriveTrainConnectionStatus("MainRotor" , false );
    if(_driveTrainMainDamaged == 0 && driveTrain->getDriveTrainConnectionStatus("MainRotor") == false) driveTrain->setDriveTrainConnectionStatus("MainRotor" , true );

    if(_driveTrainTailDamaged > 0 && driveTrain->getDriveTrainConnectionStatus("TailRotor") == true) driveTrain->setDriveTrainConnectionStatus("TailRotor" , false );
    if(_driveTrainTailDamaged == 0 && driveTrain->getDriveTrainConnectionStatus("TailRotor") == false) driveTrain->setDriveTrainConnectionStatus("TailRotor" , true );
  }
}

///set wings effectivity 
void GenHeli600Arma::SetWingForceScale(float scale, int index) 
{
  if(index < 0)
  {
    for(int i = 0; i < getNumStabilizers(); i++)
    {
      if(getStabilizer(i)) getStabilizer(i)->setForceScale(scale);
    }
  }
  else if(index < getNumStabilizers() && getStabilizer(index)) getStabilizer(index)->setForceScale(scale); 
};

///set wings damage 
void GenHeli600Arma::SetWingDamage(float damage, int index) 
{
  switch(index)
  {
  case 0:
    {
      for(int i = 0; i < getNumStabilizers(); i++)
      {
        if (i < _stabilizersSize.Size()  && getStabilizer(i)->getMountPosition().x() < -0.1f)
          getStabilizer(i)->getConfigParams()->surfaceArea = floatMax(0, damage) * _stabilizersSize[i];
      }
      break;
    }
  case 1:
    {
      for(int i = 0; i < getNumStabilizers(); i++)
      {
        if (i < _stabilizersSize.Size()  && getStabilizer(i)->getMountPosition().x() > 0.1f)
          getStabilizer(i)->getConfigParams()->surfaceArea = floatMax(0, damage) * _stabilizersSize[i];
      }
      break;
    }
  case 2:
    {
      for(int i = 0; i < getNumStabilizers(); i++)
      {
        if (i < _stabilizersSize.Size()  && getStabilizer(i)->getMountPosition().x() < -0.1f && getStabilizer(i)->getMountPosition().x() > 0.01f)
          getStabilizer(i)->getConfigParams()->surfaceArea = floatMax(0, damage) * _stabilizersSize[i];
      }
      break;
    }
  }
};


void GenHeli600Arma::SetWingsOrientation(float angle)
{
  smVec3r upSM = smVec3r(0, -sin(angle), cos(angle));
  smVec3r frontSM = smVec3r(0, cos(angle), sin(angle));


  if (_horizontalStabilizerLeft)
  {
    _horizontalStabilizerLeft->getConfigParams()->up = upSM;
    _horizontalStabilizerLeft->getConfigParams()->front = frontSM;
  }

  if (_horizontalStabilizerRight)
  {
    _horizontalStabilizerRight->getConfigParams()->up = upSM;
    _horizontalStabilizerRight->getConfigParams()->front = frontSM;
  }

}


void GenHeli600Arma::SetEnginesDamage(float engine1, float engine2, float engine3)
{
  saturate(engine1, 0.0f, 1.0f);
  saturate(engine2, 0.0f, 1.0f);
  saturate(engine3, 0.0f, 1.0f);
  //engine efficiency is decreasing with square
  _enginesDamage[0] = pow(engine1,2);
  _enginesDamage[1] = pow(engine2,2);
  _enginesDamage[2] = pow(engine3,2);
}

void GenHeli600Arma::GetWingsPosition(AutoArray<Vector3> &values) 
{
  for (int i = 0; i< getNumStabilizers(); i++)
  {
    smPositionVec posRTD = getStabilizer(i)->getMountPosition();
    Vector3 pos = Vector3(posRTD[0],posRTD[2],posRTD[1]);
    values.Add(pos);
  }
}

void GenHeli600Arma::GetWingsOrientation(AutoArray<Vector3> &values)
{
  smVec3r direction; 

  RotorLibFDM::FGHelicopterStabilizerWing* wing = _horizontalStabilizerLeft;
  if(!wing) wing = _horizontalStabilizerRight;

    if (wing )
    {
      direction = wing->getConfigParams()->up;
      Vector3 pos = Vector3(direction[0],direction[2],direction[1]);
      values.Add(pos);

      direction = wing->getConfigParams()->front;
      pos = Vector3(direction[0],direction[2],direction[1]);
      values.Add(pos);
    }

}

///number of engines
int GenHeli600Arma::GetNumberOfEngines() const 
{
  return base::getNumEngines();
}

float GenHeli600Arma::GetEnginesRpm(int engineIndex) const 
{
  if(engineIndex >= 0 && engineIndex < GetNumberOfEngines() && getEngine(engineIndex))
  {
    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(engineIndex);
    if(!engine) return 0;
    return (engineIndex < 3 && _enginesDamage[engineIndex] < 0.02)?  0 : floatMin(engine->getRPM(), engine->getTargetRPM());
  }
  return 0;
}

///engines rpm - limited by target RPM
void GenHeli600Arma::GetEnginesRpm(AutoArray<float> &values) const 
{
  for (int i = 0; i < getNumEngines(); i++)
  {
    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
    if(!engine) continue;
    values.Add((i < 3 && _enginesDamage[i] < 0.02)?  0: floatMin(engine->getRPM(), engine->getTargetRPM()));
  }
}

float GenHeli600Arma::GetAverageRPMScaled() const 
{
  float enginesRpm = 0;
  for (int i = 0; i < getNumEngines(); i++)
  {
    if(getEngine(i))
    {
      enginesRpm += ((i < 3 && _enginesDamage[i] < 0.02)?  0 : getEngine(i)->getRPM()) / _targetRPM[i];
    }
  }
  return enginesRpm / getNumEngines();
}


void GenHeli600Arma::GetEnginesOn(AutoArray<bool> &values) const 
{
  for (int i=0; i< getNumEngines(); i++)
  {
    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
    if(!engine) continue;
    if(engine->getConfigParams()->m_maxPower > 0.0001) values.Add(true);
    else  values.Add(false);
  }
}

void GenHeli600Arma::GetEnginesPower(AutoArray<float> &values) const
{
  for (int i=0; i< getNumEngines(); i++)
  {
    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
    if(!engine) continue;
    values.Add(engine->getPowerWatt());
  }
}

void GenHeli600Arma::GetEnginesTorque(AutoArray<float> &values) const
{
  for (int i=0; i< getNumEngines(); i++)
  {
    RTD::ConstantRPMEngine *engine = (RTD::ConstantRPMEngine*) getEngine(i);
    if(!engine) continue;
    values.Add(engine->getTorque());
  }
}

///rotors rpm
Vector3 GenHeli600Arma::GetRotorsRpm() const 
{
  Vector3 rotorsRpm;
  rotorsRpm[0] = getMainRotor()? getMainRotor()->getRPM() : 0;
  rotorsRpm[1] = getTailRotor()? getTailRotor()->getRPM() : 0;
  return rotorsRpm;
}

///force produced by rotors
void GenHeli600Arma::GetRotorsForces(AutoArray<Vector3> &values) 
{
  const smForceVec smForceVecMain = getMainRotor()->getForce();
  Vector3 main = Vector3(smForceVecMain[0], smForceVecMain[2], smForceVecMain[1]);

  const smForceVec smForceVecTail = getTailRotor()->getForce();
  Vector3 tail = Vector3(smForceVecTail[0], smForceVecTail[2], smForceVecTail[1]);

  values.Add(main);
  values.Add(tail);
}

Vector3 GenHeli600Arma::GetRotorsForcesSize() 
{
  Vector3 rotorsForces;
  const smForceVec smForceVecMain = getMainRotor()->getForce();
  rotorsForces[0] = smForceVecMain.length();

  const smForceVec smForceVecTail = getTailRotor()->getForce();
  rotorsForces[1] = smForceVecTail.length();

  return rotorsForces;
}

///force produced by wings
void GenHeli600Arma::GetWingsForces(AutoArray<Vector3> &values) 
{
  for(int i=0; i< getNumStabilizers(); i++)
  {
    const smForceVec smForceVec = getStabilizer(i)->getForce();
    Vector3 force = Vector3(smForceVec[0], smForceVec[2], smForceVec[1]);
    values.Add(force);
  }
}

//if any wing exceeds max. stress force, add damage
void GenHeli600Arma::GetWingsMaxForcesSize(AutoArray<float> &values) 
{
  values.Add(0); //left
  values.Add(0); //right
  values.Add(0); //center

  for(int i=0; i< getNumStabilizers(); i++)
  {
    const smForceVec smForceVec = getStabilizer(i)->getForce();
    Vector3 force = Vector3(smForceVec[0], smForceVec[2], smForceVec[1]);

    if (i < _stabilizersSize.Size()  && getStabilizer(i)->getMountPosition().x() < -0.01f)
      values[0] = floatMax(values[0], force.Size());

    if (i < _stabilizersSize.Size()  && getStabilizer(i)->getMountPosition().x() > 0.01f)
      values[1] = floatMax(values[1], force.Size());

    if (i < _stabilizersSize.Size()  && getStabilizer(i)->getMountPosition().x() < -0.01f && getStabilizer(i)->getMountPosition().x() > 0.01f)
      values[2] = floatMax(values[2], force.Size());

    values.Add(force.Size());
  }
}

float GenHeli600Arma::GetMainBladeRadius() const 
{
  RTD::FGRotorBladeElementTheory* mainRotor = (RTD::FGRotorBladeElementTheory*)getMainRotor();
  if(mainRotor) return mainRotor->getConfigParams()->m_bladeRadius;
  else return 3.0f;
}

float GenHeli600Arma::GetTailBladeRadius() const 
{
  RTD::FGRotorBladeElementTheory* tailRotor = (RTD::FGRotorBladeElementTheory*)getTailRotor();
  if(tailRotor) return tailRotor->getConfigParams()->m_bladeRadius;
  else return 1.0f;
}

float GenHeli600Arma::GetDeliveredTorque() const 
{
  float torque = 0;
  for(int i = 0; i< getNumEngines(); i++) if(getEngine(i)) torque+=getEngine(i)->getTorque();
  return torque;
};

bool GenHeli600Arma::IsLandContact() const
{
  for(int i = 0; i< getNumGroundContacts(); i++)
  {
     if(getGroundContact(i)->isTouchingGround()) return true;
  }
  return false;
}

/*********************************************************************************************************/

//derived ConstantRPMEngine with added Arma serialization and network layer
ConstantRPMEngineArma::ConstantRPMEngineArma(RTD::VehicleBase *parentVh, const std::string &name, const ConfigParams *params)
:ConstantRPMEngine(parentVh, name, params), SimulationVariableUser(name,parentVh)
{

};

std::string ConstantRPMEngineArma::getType() const
{
  RString typeName = "ConstantRPMEngineArma";
  return typeName.Data();
};

RTD::ModelComponentBase::CreatorFuncType  ConstantRPMEngineArma::getCreatorFunc (const std::string &className) 
{
  return &createClass;
}

RTD::ModelComponentBase::ParametersCreatorFuncType  ConstantRPMEngineArma::getParametersCreatorFunc (const std::string &className)
{
  return &createParameters;
}

RTD::ModelComponentBase* ConstantRPMEngineArma::createClass( RTD::VehicleBase* parentVehicle , const std::string& name , const ModelComponentBase::ConfigParams* params  )
{
  ConstantRPMEngine::ConfigParams *config =   (ConstantRPMEngine::ConfigParams *)params;
  if(config)
    return new ConstantRPMEngineArma(parentVehicle, name , config );
  return NULL;
}

RTD::ModelComponentBase::ConfigParams* ConstantRPMEngineArma::createParameters()
{
    return base::createParameters();
}

//serialize
LSError ConstantRPMEngineArma::Serialize(ParamArchive &ar)
{
  return LSOK;
}

/*********************************************************************************************************/

//derived ConstantRPMEngine with added Arma serialization and network layer
FGGroundContact200Arma::FGGroundContact200Arma(RTD::VehicleBase *parentVh, const std::string &name, const ConfigParams *params)
:FGGroundContact200(parentVh, name, params), SimulationVariableUser(name,parentVh)
{
  _hitSpeed = smVec3d(0,0,0);
};

bool FGGroundContact200Arma::checkTerrainCollision( const smVec3d& wheelPos , smVec3d& hitXYZ ,  smVec3d& hitNormal ,  smVec3d& hitVelocity , smReal& frictionFactor )
{
  bool hit = base::checkTerrainCollision( wheelPos, hitXYZ , hitNormal , hitVelocity , frictionFactor);
  //reset velocity - our RTDLib does not support moving ground contact
  _hitSpeed = hitVelocity;
  hitVelocity = smVec3d(0,0,0);
  return hit;
}


std::string FGGroundContact200Arma::getType() const
{
  RString typeName = "FGGroundContact200Arma";
  return typeName.Data();
};

RTD::ModelComponentBase::CreatorFuncType  FGGroundContact200Arma::getCreatorFunc (const std::string &className) 
{
  return &createClass;
}

RTD::ModelComponentBase::ParametersCreatorFuncType  FGGroundContact200Arma::getParametersCreatorFunc (const std::string &className)
{
  return &createParameters;
}

RTD::ModelComponentBase* FGGroundContact200Arma::createClass( RTD::VehicleBase* parentVehicle , const std::string& name , const ModelComponentBase::ConfigParams* params  )
{
  FGGroundContact200::ConfigParams *config =   (FGGroundContact200::ConfigParams *)params;
  if(config)
    return new FGGroundContact200Arma(parentVehicle, name , config );
  return NULL;
}

RTD::ModelComponentBase::ConfigParams* FGGroundContact200Arma::createParameters()
{
  return base::createParameters();
}

//serialize
LSError FGGroundContact200Arma::Serialize(ParamArchive &ar)
{
  return LSOK;
}

/*********************************************************************************************************/

FCSAutoHoverTOH::FCSAutoHoverTOH(RTD::VehicleBase *parentVh, const std::string &name, const ConfigParams *params)
:FCSRotationDamper(parentVh, name, params), SimulationVariableUser(name,parentVh)
{
  base::enable(false);
  _correctionP = 0;
  _correctionR = 0;
  _isTouchingGound = false;
}

//class name
std::string FCSAutoHoverTOH::getType() const
{
  std::string name = "FCSAutoHoverTOH";
  return name;
}

RTD::ModelComponentBase::CreatorFuncType FCSAutoHoverTOH::getCreatorFunc (const std::string &className)
{
  return &createClass;
}

RTD::ModelComponentBase::ParametersCreatorFuncType  FCSAutoHoverTOH::getParametersCreatorFunc (const std::string &className)
{
  return &createParameters;
}

RTD::ModelComponentBase* FCSAutoHoverTOH::createClass( RTD::VehicleBase* parentVehicle , const std::string& name , const ModelComponentBase::ConfigParams* params  )
{
  FCSRotationDamper::ConfigParams *config =   (FCSRotationDamper::ConfigParams *)params;
  if(config)
    return new FCSAutoHoverTOH(parentVehicle, name , config );
  return NULL;
}

RTD::ModelComponentBase::ConfigParams* FCSAutoHoverTOH::createParameters()
{
  return base::createParameters();
}

static bool enableSt = false;
static int corX = 0;
static int corY = 0;

void FCSAutoHoverTOH::update(smTime steptime)  
{
  RTD::PilotInputData* controlOutput  = (RTD::PilotInputData*)m_controlOutput;
  RTD::PilotInputData* controlInput = NULL;
  if ( m_previousStage )
  {
    controlInput = (RTD::PilotInputData*)m_previousStage->getControlOutput();
  }
  else
  {
    controlInput = (RTD::PilotInputData*)m_attachedVehicle->getPilotInputData();
  }


  // Not entering following loop means that either the input or the output is not available.
  // Both of these cases are possibly an indication of a bug.
  if  ((controlOutput) && (controlInput))
  {
    // First copy the input to output
    m_controlOutput->copy( controlInput );

    // If enabled go on and compute output values.
    if  (isEnabled() && !_isTouchingGound)
    {
      smVec3r localSpeed = m_attachedVehicle->getState().getSpeed();
      const smMatrix rotIn = m_attachedVehicle->getState().getRotMatInv();
      localSpeed = rotIn * localSpeed;

      smVec2r  input = controlInput->m_rollPitchControl;
      smVec3r rotation = m_attachedVehicle->getRotationAsHPR();

      saturate( localSpeed[0],-20,35);
      saturate( localSpeed[1],-20,20);
      
      rotation[1] =  rotation[1]  - localSpeed[1] - _correctionP;   //pitch
      rotation[2] = -rotation[2]  - localSpeed[0] - _correctionR;   //roll

      smReal pitchOffset = -m_pitchController->update( -rotation[1] * 0.5 , steptime );      
      smReal pitch = smMath::clamp( input[1] + controlInput->m_pitchTrim + pitchOffset , -1.0f , 1.0f );

      smReal rollOffset =  -m_rollController->update( -rotation[2] * 0.5 , steptime );
      smReal roll = smMath::clamp( input[0] + controlInput->m_rollTrim + rollOffset, -1.0f , 1.0f  );

      controlOutput->m_pedals =  controlInput->m_pedals; //yaw - do not change, leave it for auto trim
 
      smVec2r  output = smVec2r(roll, pitch);
      controlOutput->m_rollPitchControl = output;

    }
  }
}


#endif