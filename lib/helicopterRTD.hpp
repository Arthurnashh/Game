#ifdef _MSC_VER
#pragma once
#endif


#ifndef _HELICOPTER_FDM_HPP
#define _HELICOPTER_FDM_HPP

#include <Es/Types/lLinks.hpp>
#include <El/ParamArchive/paramArchive.hpp>

#if _HELISIM_ROTORLIB

#if _DUMMY_ROTORLIB
typedef void GenHeli600Arma;
#else

#if _DEBUG
// RTD uses STL - we have to use normal new 
#include <Es/Memory/normalNew.hpp>
#endif

#pragma warning( push )
#pragma warning(disable:4530)

#include "RotorLibFDM/GenHeli600.h"
#include "RotorLibFDM/FCSHelicopterAutoTrim100.h"
#include "Util/smFilter.h"

#if !_DUMMY_ROTORLIB
#include "RTD/ConstantRPMEngine.h" 
#include "RTD/FGForce.h"

#include "RTD/FCSRotationDamper.h"
#include "RTD/InputData.h"

#include "RTD/LookupTable.h"
#include "RTD/FGRotorBladeElementTheory.h"

#include "RTD/FGGroundContact200.h"
#include "RTD/FGGroundContact300.h" 
#include "RotorLibFDM/FGHelicopterStabilizerWing.h"
#endif

#if _DEBUG
#include <Es/Memory/debugNew.hpp>
#endif

#pragma warning( pop ) 
class ForceGeneratorRTD
{
public:
  bool _enabled;
  Vector3 _strength;
  Vector3 _position;
  Vector3 _radius;
  Vector3 _orientation;
  Ref<Object> _attachedTo;
  int _type;

  ForceGeneratorRTD();

  ForceGeneratorRTD(bool _enabled, Vector3 strength, Vector3 position, Vector3 radius,
    Vector3 orientation, Ref<Object> attachedTo,int type);

  LSError Serialize(ParamArchive &ar);

};
TypeIsMovable(ForceGeneratorRTD)

class GenHeli600Arma : public RotorLibFDM::GenHeli600
{

  typedef RotorLibFDM::GenHeli600 base;

private:
  // arma helicopter parent
  HelicopterRTD *_parent;
  //wings rotated with collective
  RotorLibFDM::FGHelicopterStabilizerWing *_horizontalStabilizerRight;
  RotorLibFDM::FGHelicopterStabilizerWing *_horizontalStabilizerLeft;

  //engine settings
  AutoArray<float> _engineMaxPower;
  AutoArray<float>  _targetRPM;
  AutoArray<float>  _throttle;
  //stabilizers size
  AutoArray<float>  _stabilizersSize;
  //max engines torque
  AutoArray<float> _engineMaxTorque;

  //starting time
  AutoArray<float> _startingTime;
  AutoArray<float> _rpmChangePerSec;
  AutoArray<float> _wantedRPM;

  AutoArray<float> _engineControlleroffset;
  //brakes
  float _brakesLeft;
  float _brakesRight;
  float _rotorBrake;

#if !_DUMMY_ROTORLIB
  RTD::FGForce *externalForce;
  RTD::FGForce *loadForce;
#endif

  //delivered torque
  float _engineTorque;
  // set  & 1 for damaged transmission, & 2 for damaged rotors
  int _driveTrainMainDamaged, _driveTrainTailDamaged;

  //superCharge is used when we need to increase RTD RPM in very very short time
  //is enabled when simulation is switching from arma to rtd
  bool _superCharging;

  Vector3 _enginesDamage;

  /**
  Constructor of the class.
  \param world	:	An instance of the RTD::World class which stores environmental information like wind or gravity.                                

  \param name			A unique name for the helicopter. If there is another vehicle with the same name
  within the same world, the newly created helicopter will not be added to the world.
  Note that, even if an entity is deleted, its name remains reserved until World::reset()
  is called.

  \param params	:	\e params stores the configuration parameters of the helicopter. The \e params
  pointer will \e not be deleted automatically.
  */
public:

  GenHeli600Arma(	RTD::World* world, HelicopterRTD *parent, const std::string& name, ConfigParams* params);

  /**
  Constructor of the class.
  \param world	:	An instance of the RTD::World class which stores environmental information like wind or gravity.                                

  \param name			A unique name for the helicopter. If there is another vehicle with the same name
  within the same world, the newly created helicopter will not be added to the world.
  Note that, even if an entity is deleted, its name remains reserved until World::reset()
  is called.

  \param parameterFile :	Name of an XML based configuration file. When this constructor is called, GenHeli600
  will be initialized with this configuration file. If there is a problem in reading the file,
  helicopter will be disabled. The format of this configuration file is explained in the
  RotorLib FDM section of the Programmer's Guide. 

  */
  GenHeli600Arma( RTD::World* world, HelicopterRTD *parent, const std::string& name, const std::string& parameterFile);
  

  //remove force generators, tell parent if RTD deleted me (can happen if _rtdWorld is destroyed)
  ~GenHeli600Arma();

  void Init(const std::string& name);

  void RemoveForces();
  void SetSlingLoadForce(Vector3 slingloadForce);

  // enable RTD simulation
  virtual void setEnabled( bool enabled );
  // RTD sim. calls (used for faster simulation when airborne heli is inserted)
  void update(smTime timestep);
  void preUpdate( smTime stepTime );
  void postUpdate( smTime stepTime );
  // arma simulation
  void Simulate(float deltaT, EntityAI *parent, bool elPower, int starter, float fuel, bool superCharge = false);

  //serialize
  LSError Serialize(ParamArchive &ar);

  virtual void EngineOn();
  virtual void EngineOn(float quickStart);
  virtual void EngineOff();
  bool EngineIsOn();
  bool EngineIsOn(int index);
  
  // instant engines and rotorts stop
  void InstantStop();
  // wanted engine RPM
  void SetWantedEngineRPM(float wantedEngineRPM, float time, int index);
  __forceinline float GetEngineTargetRPM(int index ) const { return (index >=0 && index< _targetRPM.Size())? _targetRPM[0] : 1.0;}

  //sets RTD helicopter engine parameters
  void SetThrottle(float value, int index);
  __forceinline float GetThrottle(int index) const {return (index>=0 && index< _throttle.Size())?  _throttle[index]: 0;}
  __forceinline float GetTargetRPM(int index) const {return (index>=0 && index< _targetRPM.Size())?  _targetRPM[index]: 0;}
  __forceinline void GetThrottle(AutoArray<float> &values) const {values = _throttle;};
  __forceinline float GetMaxThrottle() const {float maxT = 0; for(int i = 0; i< _throttle.Size(); i++ ) {maxT = floatMax(_throttle[i], maxT);} return maxT;};

  __forceinline void GetTargetRPM(AutoArray<float> &values) const  {values = _targetRPM;};
  float GetDeliveredTorque() const;

  //wings
  void SetWingForceScale(float scale, int index);
  void GetWingsPosition(AutoArray<Vector3> &values);
  void GetWingsOrientation(AutoArray<Vector3> &values);

  void SetEnginesDamage(float engine1, float engine2, float engine3);
  void SetTransmissionDamage(float damage);
  void SetMainRotorDamage(float valueMain);
  void SetTailRotorDamage(float valueTail);
  void SetWingDamage(float damage, int index);
  
  void SetWingsOrientation(float angle);

  //brakes
  __forceinline void SetWheelsBrake(float brakeValue, int index) { if (index & 1) _brakesLeft = brakeValue; if (index & 2) _brakesRight = brakeValue; };
  __forceinline void SetRotorBrake(float value) {_rotorBrake = value;}; 
  __forceinline float GetBrakeLeft() const {return _brakesLeft;}
  __forceinline float GetBrakeRight() const {return  _brakesRight;}
  __forceinline float GetRotorBrake() const {return  _rotorBrake;}

  //returns RTD helicopter parameters
  int GetNumberOfEngines() const ;
  float GetEnginesRpm(int engineIndex) const ;
  void GetEnginesRpm(AutoArray<float> &values) const ;
  float GetAverageRPMScaled() const ;
  void GetEnginesOn(AutoArray<bool> &values) const ;
  void GetEnginesPower(AutoArray<float> &values) const ;
  void GetEnginesTorque(AutoArray<float> &values) const ;
  Vector3 GetRotorsRpm() const ;
  void GetRotorsForces(AutoArray<Vector3> &values);
  Vector3 GetRotorsForcesSize();
  void GetWingsForces(AutoArray<Vector3> &values) ;
  void GetWingsMaxForcesSize(AutoArray<float> &values);

  float GetMainBladeRadius() const ;
  float GetTailBladeRadius() const ;
  bool IsLandContact() const ;

  // RTD damage
  virtual void setDamageState(const DamageState& dstate)
  {
    //disabled - damage simulated in helicopterRTD::simulateDamage
  };
};

class ConstantRPMEngineArma : public RTD::ConstantRPMEngine
{

private:
  typedef  RTD::ConstantRPMEngine base; 

public:
  //class RTDYNAMICS_EXPORT ConfigParams : public ConstantRPMEngine::ConfigParams
  //{
  //private:
  //  friend class ConstantRPMEngineArma;

  //public:
  //  ConfigParams()
  //  {
  //    ConstantRPMEngine::ConfigParams::ConfigParams();
  //  }

  //  virtual bool  readParamStr( const std::string& parameterString , std::string& error )
  //  {
  //    ConstantRPMEngine::ConfigParams::readParamStr(parameterString, error);
  //  }
  //};

  ConstantRPMEngineArma(RTD::VehicleBase *parentVh, const std::string &name, const ConfigParams *params);

  //class name
  virtual std::string getType() const;

  static CreatorFuncType getCreatorFunc (const std::string &className) ;
  static ParametersCreatorFuncType  getParametersCreatorFunc (const std::string &className);

  static ModelComponentBase* createClass( RTD::VehicleBase* parentVehicle , const std::string& name , const ModelComponentBase::ConfigParams* params  );
  static ModelComponentBase::ConfigParams* createParameters();

  //serialize
  LSError Serialize(ParamArchive &ar);

};

class FGGroundContact200Arma : public RTD::FGGroundContact200
{

private:
  typedef  RTD::FGGroundContact200 base; 
  smVec3d _hitSpeed;

public:
  //class RTDYNAMICS_EXPORT ConfigParams : public ConstantRPMEngine::ConfigParams
  //{
  //private:
  //  friend class ConstantRPMEngineArma;

  //public:
  //  ConfigParams()
  //  {
  //    ConstantRPMEngine::ConfigParams::ConfigParams();
  //  }

  //  virtual bool  readParamStr( const std::string& parameterString , std::string& error )
  //  {
  //    ConstantRPMEngine::ConfigParams::readParamStr(parameterString, error);
  //  }
  //};

  FGGroundContact200Arma(RTD::VehicleBase *parentVh, const std::string &name, const ConfigParams *params);

  virtual bool checkTerrainCollision( const smVec3d& wheelPos , smVec3d& hitXYZ ,  smVec3d& hitNormal ,  smVec3d& hitVelocity , smReal& frictionFactor );


  //class name
  virtual std::string getType() const;

  static CreatorFuncType getCreatorFunc (const std::string &className) ;
  static ParametersCreatorFuncType  getParametersCreatorFunc (const std::string &className);

  static ModelComponentBase* createClass( RTD::VehicleBase* parentVehicle , const std::string& name , const ModelComponentBase::ConfigParams* params  );
  static ModelComponentBase::ConfigParams* createParameters();

  //serialize
  LSError Serialize(ParamArchive &ar);

};

class FCSAutoHoverTOH : public RTD::FCSRotationDamper 
{

private:
  typedef  RTD::FCSRotationDamper base; 
  float  _correctionP, _correctionR;
  bool _isTouchingGound;


public:
  //class RTDYNAMICS_EXPORT ConfigParams : public FCSRotationDamper::ConfigParams
  //{
  //private:
  //  friend class FCSAutoHoverTOH;

  //public:
  //  ConfigParams()
  //  {
  //    FCSRotationDamper::ConfigParams::ConfigParams();
  //  }

  //  virtual bool  readParamStr( const std::string& parameterString , std::string& error )
  //  {
  //    FCSRotationDamper::ConfigParams::readParamStr(parameterString, error);
  //  }
  //};

  FCSAutoHoverTOH(RTD::VehicleBase *parentVh, const std::string &name, const ConfigParams *params);
  //class name
  virtual std::string getType() const;

  static CreatorFuncType getCreatorFunc (const std::string &className);
  static ParametersCreatorFuncType  getParametersCreatorFunc (const std::string &className);

  static ModelComponentBase* createClass( RTD::VehicleBase* parentVehicle , const std::string& name , const ModelComponentBase::ConfigParams* params  );
  static ModelComponentBase::ConfigParams* createParameters();

  virtual void update (smTime  stepTime);

  virtual void setCorrection(float p, float r) {_correctionP =p;  _correctionR =r;}
  void SetIsTouchingGound(bool touching) {_isTouchingGound = touching;};
};


#endif

#endif

#endif
