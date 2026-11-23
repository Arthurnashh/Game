// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"
#include "helicopter.hpp"
#include "parachute.hpp"
#include "AI/ai.hpp"
#include "shots.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "keyInput.hpp"
#include "lights.hpp"
#include "stringtableExt.hpp"
#include "paramArchiveExt.hpp"
#include "txtPreload.hpp"
#include "diagModes.hpp"
#include "dikCodes.h"

#include "tlVertex.hpp"

#include "UI/uiActions.hpp"

#include <El/Common/randomGen.hpp>
#include <El/Common/perfProf.hpp>

#include "Network/network.hpp"

#include "engine.hpp"
#include "Shape/specLods.hpp"

#include <El/Enum/enumNames.hpp>

#include "mbcs.hpp"
#include "progress.hpp"


#if _HELISIM_ROTORLIB && !_DUMMY_ROTORLIB

#pragma warning( push )
#pragma warning(disable:4530)

#include "RTD/FGGroundContact200.h"
#include "RTD/FGGroundContact300.h" 
#include "RTD/WindsSystem.h"

#pragma warning( pop )

#endif

#if _ENABLE_CHEATS
  #define ARROWS 1
#else
  #define ARROWS 0
#endif

#if _ENABLE_CHEATS
#pragma optimize("",off)
#endif

#define DGR2RAD(x) (3.141592f /180.0f)*x

static float HeliSimStepInactive = 0.100f;
static float HeliSimStepInactiveAI = 0.100f;

static float HeliSimStepActive = 0.050f;
static float HeliSimStepActiveAI = 0.050f;

static float HeliSimStepPriority = 0.025f;
static float HeliSimStepPriorityAI = 0.050f;

DEFINE_CASTING(Helicopter)
Helicopter::Helicopter( const EntityAIType *name, Person *pilot, bool fullCreate)
:base(name,pilot, fullCreate),

_rotorSpeedWanted(0), // turning motor on/off
_mainRotorWanted(0),
_backRotor(0),_backRotorWanted(0),

_pilotGear(true),

_cyclicAsideWanted(0),
_cyclicForwardWanted(0),
_rotorDiveWanted(0),

_lastAngVelocity(VZero),
_turbulence(VZero),
_lastTurbulenceTime(Glob.time),

_rndFrequency(1-GRandGen.RandomValue()*0.05), // do not use same sound frequency
_missileLRToggle(false),_rocketLRToggle(false),
_gunPosIndexToogle(0)
{
  if (pilot && QIsManual(pilot->Brain()))
    EngineOn();
  SetSimulationPrecision(HeliSimStepActive,1.0/10);
  RandomizeSimulationTime();
  SetTimeOffset(floatMax(floatMax(HeliSimStepActive, HeliSimStepInactive), HeliSimStepPriority));

  _dustEffect.Init(Pars >> Type()->_dustEffect, this, EVarSet_Effect);
  _waterEffect.Init(Pars >> Type()->_waterEffect, this, EVarSet_Effect);
  _damageEffect.Init(Pars >> Type()->_damageEffect, this, EVarSet_Damage);
  // by default we are suspended - MakeAirborne or impulse will wake up us as needed
  _isStopped = true;
  _cmIndexToggle = 0;

  //exhausts
  int n = Type()->_exhausts.Size();
  if (n > 0)
  {
    _exhausts.Realloc(n);
    _exhausts.Resize(n);
    for (int i=0; i<n; i++)
    {
      RString name = Type()->_exhausts[i].effect;
      _exhausts[i].Init(Pars >> name, this, EVarSet_Exhaust);
    }
  }
}

HelicopterVisualState::HelicopterVisualState(HelicopterType const& type)
:base(type),
_cyclicAside(0),
_cyclicForward(0),
_rotorPosition(0),
_rotorPositionDelta(0),
_backRotorPosition(0),
_rotorDive(0),
_gearsUp(0),
_rotorSpeed(0),
_mainRotor(0),
_wheelPhase(0), _wheelPhaseDelta(0)
{
}


void HelicopterVisualState::Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const
{
  base::Interpolate(obj, t1state, t, interpolatedResult);

  HelicopterVisualState const& s = static_cast<HelicopterVisualState const&>(t1state);
  HelicopterVisualState& res = static_cast<HelicopterVisualState&>(interpolatedResult);

  #define INTERPOLATE(x) res.x = x + t*(s.x - x)

  INTERPOLATE(_cyclicAside);  // -1..1 (actually -cyclicAsideMaxVal..-cyclicAsideMaxVal, but it's 1)
  INTERPOLATE(_cyclicForward);  // -1..1
  INTERPOLATE(_rotorDive);  // -1..1
  INTERPOLATE(_gearsUp);  // -1..1
  INTERPOLATE(_backRotorPosition);  // continuous (maybe should be mod k*pi)
  res._rotorPosition = fastFmod(_rotorPosition + t * s._rotorPositionDelta, 60*H_PI);  // mod 60pi: assume that `t1state` was created by Simulate() from `*this`
  INTERPOLATE(_rotorSpeed);  // 0..1
  INTERPOLATE(_mainRotor);  // ?
  res._wheelPhase = fastFmod(_wheelPhase + t * s._wheelPhaseDelta, 1.0f);  // mod 1: assume that `t1state` was created by Simulate() from `*this`

  #undef INTERPOLATE
}


Helicopter::~Helicopter()
{
}


HelicopterType::HelicopterType(ParamEntryPar param)
:base(param)
{
  _pilotPos=VZero;  

  _hullHit = -1;
  _engineHit = -1;
  _rotorHHit = -1;
  _rotorVHit = -1;
  _avionicsHit = -1;
  _missiles = -1;
  _glassRHit = -1;
  _glassLHit = -1;

#if _HELISIM_ROTORLIB
  _hitBatteries = -1;
  _hitLight = -1;
  _hitHydraulics = -1;
  _hitTransmission = -1;
  _hitGear = -1;
  _hitFuel = -1;
  _hitHStabilizerL1 = -1;
  _hitHStabilizerR1 = -1;
  _hitVStabilizer1 = -1;
  _hitTail = -1;
  _hitPitotTube = -1;
  _hitStaticPort = -1;
  _hitEngine2 = -1;
  _hitEngine3 = -1;

#endif
}

HelicopterType::~HelicopterType()
{
}

bool LoadSoundEntryPars(ParamEntryPtr entry, const char* name, SoundPars &intPars, SoundPars &extPars)
{
  ParamEntryPtr subEntry = entry->FindEntry(name);
  if (subEntry)
  {
    if (subEntry->IsArray() && subEntry->GetSize() >= 2)
    {
      RString intParsName = (*subEntry)[0];
      if (!GetValue(intPars, *entry >> intParsName))
        return false;

      RString extParsName = (*subEntry)[1];
      if (!GetValue(extPars, *entry >> extParsName))
        return false;

      return true;
    }
  }

#if _ENABLE_REPORT
  LogF("Sound: Error: entry %s not found !!!", name);
#endif

  return false;
}

void HelicopterType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  _enableSweep = par>>"enableSweep";
  _gearRetracting = par>>"gearRetracting";

  float t = par >> "gearDownTime";
  _gearDownSpeed = t > 0 ? 1.0f / t : 0;
  t = par >> "gearUpTime";
  _gearUpSpeed = t > 0 ? 1.0f / t : 0;

  _mainRotorSpeed = par>>"mainRotorSpeed";
  _backRotorSpeed = par>>"backRotorSpeed";

  _altFullForce = par.ReadValue("altFullForce",1000);
  _altNoForce = par.ReadValue("altNoForce",3000);

  _minMainRotorDive = HDegree(par>>"minMainRotorDive");
  _maxMainRotorDive = HDegree(par>>"maxMainRotorDive");
  _neutralMainRotorDive = HDegree(par>>"neutralMainRotorDive");


  _minBackRotorDive = HDegree(par>>"minBackRotorDive");
  _maxBackRotorDive = HDegree(par>>"maxBackRotorDive");
  _neutralBackRotorDive = HDegree(par>>"neutralBackRotorDive");

  _dustEffect =  par >> "dustEffect";
  _waterEffect =  par >> "waterEffect";
  _damageEffect = par >> "damageEffect";

  // load Envelope from config
  ParamEntryVal envelope = par>>"envelope";
  _envelope.Realloc(envelope.GetSize());
  _envelope.Resize(envelope.GetSize());
  for (int i=0; i<_envelope.Size(); i++)
    _envelope[i] = envelope[i];

  SetHardwareAnimation(par);

#if  _HELISIM_ROTORLIB
  _loadedRTD = false;
  if(par.FindEntry("RotorLibHelicopterProperties"))
  {
    ParamEntryVal entryRotorLib = par >> "RotorLibHelicopterProperties";
    if (!entryRotorLib.IsClass()) goto default_RotorLib_Helicopter;
    //RTD configuration   
    _configNameRTD = entryRotorLib >> "RTDconfig";

#if !_DUMMY_ROTORLIB
    std::string error;
    if(_configNameRTD.GetLength() > 0)
    {
      // open the file
      QIFStreamB in;
      AutoArray<char> _data;

      in.AutoOpen(_configNameRTD);
      int len = in.rest()+1;
      _data.Realloc(len);
      _data.Resize(len);
      in.read(_data.Data(), len-1);
      if (len) _data[len-1]=0; // Append the null terminator character.
      in.close();
      if (!in.fail())
      {
        _configRTD.readParamStr(_data.Data(), error);
        _loadedRTD = true;
      }
    }
#endif

    //battery properties
    _maxBatteryCharge = entryRotorLib >> "maxBatteryCharge";
    _starterBatteryDrain = entryRotorLib >> "starterBatteryDrain";
    _electricityBatteryDrain = entryRotorLib >> "electricityBatteryDrain";
    _engineBatteryRecharge = entryRotorLib >> "engineBatteryRecharge";
    _APU = entryRotorLib >> "hasAPU";

    // stress and damage
    _maxTorque = entryRotorLib >> "maxTorque";
    _maxEngineTemperature = entryRotorLib >> "maxEngineTemperature";
    _hotStartTemperature= entryRotorLib >> "hotStartTemperature";
    _retreatBladeStallWarningSpeed = entryRotorLib >> "retreatBladeStallWarningSpeed";
    _maxMainRotorStress = entryRotorLib >> "maxMainRotorStress";
    _maxTailRotorStress = entryRotorLib >> "maxTailRotorStress";
    _maxHorizontalStabilizerLeftStress = entryRotorLib >> "maxHorizontalStabilizerLeftStress";
    _maxHorizontalStabilizerRightStress = entryRotorLib >> "maxHorizontalStabilizerRightStress";
    _maxVerticalStabilizerStress = entryRotorLib >> "maxVerticalStabilizerStress";

    _defaultCollective = entryRotorLib >> "defaultCollective";

    _horizontalWingsAngleCollMin = entryRotorLib >> "horizontalWingsAngleCollMin";
    _horizontalWingsAngleCollMax = entryRotorLib >> "horizontalWingsAngleCollMax";
    _horizontalWingsAngleCollMin = (_horizontalWingsAngleCollMin/180.0f) * H_PI;
    _horizontalWingsAngleCollMax = (_horizontalWingsAngleCollMax/180.0f) * H_PI;

    _stressDamagePerSec = entryRotorLib >> "stressDamagePerSec";

    ParamEntryVal correction=entryRotorLib>>"autoHoverCorrection";
    _autoHoverCorrection[0]=correction[0];
    _autoHoverCorrection[1]=correction[1];


    ParamEntryVal entryTemperature = entryRotorLib >> "temperatureEffects";
    if(entryTemperature.GetSize() == 3)
    {
      const IParamArrayValue &entryTemperaturevaluesOff = entryTemperature[0];
      for (int j = 0; j < entryTemperaturevaluesOff.GetItemCount(); j++)
      {
        _temperatureProgressOff.Add(entryTemperaturevaluesOff[j].GetFloat());
      }

      const IParamArrayValue &entryTemperaturevaluesIdle = entryTemperature[1];
      for (int j = 0; j < entryTemperaturevaluesIdle.GetItemCount(); j++)
      {
        _temperatureProgressIdle.Add(entryTemperaturevaluesIdle[j].GetFloat());
      }

      const IParamArrayValue &entryTemperaturevaluesFull = entryTemperature[2];
      for (int j = 0; j < entryTemperaturevaluesFull.GetItemCount(); j++)
      {
        _temperatureProgressFull.Add(entryTemperaturevaluesFull[j].GetFloat());
      }
    }
  }
  else
  {
default_RotorLib_Helicopter:
    // stress and damage
    _maxTorque = 800;
    _maxEngineTemperature = 1000;
    _hotStartTemperature = 1000;
    _retreatBladeStallWarningSpeed = 69.4;
    _maxMainRotorStress = 10000;
    _maxTailRotorStress = 10000;
    _maxHorizontalStabilizerLeftStress = 10000;
    _maxHorizontalStabilizerRightStress = 10000;
    _maxVerticalStabilizerStress = 10000;
    _stressDamagePerSec = 0.01;
    _defaultCollective = 0.7f;

    _horizontalWingsAngleCollMin = 0.0f;
    _horizontalWingsAngleCollMax = 0.0f;

    //battery properties
    _maxBatteryCharge = 100.0;
    _starterBatteryDrain = 2.0;
    _electricityBatteryDrain = 0.1;
    _engineBatteryRecharge = 0.5;

    _APU = false;
  }
  //we need 8 numbers (see definition)
  for(int i =_temperatureProgressIdle.Size(); i < 4; i++) _temperatureProgressIdle.Add(0);
  for(int i =_temperatureProgressFull.Size(); i < 4; i++) _temperatureProgressFull.Add(0);
  for(int i =_temperatureProgressOff.Size();  i < 4; i++) _temperatureProgressOff.Add(0);

#endif

#if _HELISIM_SOUNDS
  /* Load individual sound and store them in groups. */
  ParamEntryPtr entryExt = par.FindEntry("SoundsExt");
  if (entryExt)
  {
    ParamEntryPtr soundsEntry = entryExt->FindEntry("Sounds");
    if (soundsEntry)
    {
      LoadSubmixGroups(*entryExt, "Sounds", _spe._submixPars);
    }

    LoadSoundEntryPars(entryExt, "starterClick", _spe._starterClickPars[0], _spe._starterClickPars[1]);
    LoadSoundEntryPars(entryExt, "idleThrottle", _spe._idleThrottlePars[0], _spe._idleThrottlePars[1]);
    LoadSoundEntryPars(entryExt, "rotorDamage", _spe._rotorDamagePars[0], _spe._rotorDamagePars[1]);
    
    ParamEntryPtr extNoiseEntry = entryExt->FindEntry("RotorNoiseExt");
    if (extNoiseEntry)
    {
      Ref<ExternNoisePars> exNoisePars = new ExternNoisePars;
      if (exNoisePars->Load(*extNoiseEntry))
        _spe._externNoisePars = exNoisePars;
    }

    // Load apu
    ParamEntryPtr apuEntry = entryExt->FindEntry("Apu");
    if (apuEntry)
    {
      _spe._apuSoundPars = new StartStopSoundPars;
      _spe._apuSoundPars->Load(*apuEntry, Transport::SoundControllers(), Transport::SoundControllersCount());
    }

    ParamEntryPtr starterEntry = entryExt->FindEntry("Starter");
    if (starterEntry)
    {
      // Load starter data
      _spe._starterSoundPars = new StartStopSoundPars;
      _spe._starterSoundPars->Load(*starterEntry, Transport::SoundControllers(), Transport::SoundControllersCount());
    }

    ParamEntryPtr starterEntry1 = entryExt->FindEntry("Starter1");
    if (starterEntry1)
    {
      // Load starter data
      _spe._starterSoundPars1 = new StartStopSoundPars;
      _spe._starterSoundPars1->Load(*starterEntry1, Transport::SoundControllers(), Transport::SoundControllersCount());
    }

    _spe._landingSoundPars = new LandingSoundsPars;
    if (!_spe._landingSoundPars->Load(*entryExt))
      _spe._landingSoundPars.Free();
  }

  ParamEntryPtr noiseEntry = par.FindEntry("SoundNoises");
  if (noiseEntry)
  {
    _spe._noiseSoundPars = new NoiseSoundPars;
    _spe._noiseSoundPars->Load(*noiseEntry, Transport::SoundControllers(), Transport::SoundControllersCount());
  }
#endif

  ParamEntryPtr gearEntry = par.FindEntry("soundLandingGear");
  if (gearEntry)
  {
    GetValue(_soundLandingGearPars, *gearEntry);
  }

}

#include <Es/Common/delegate.hpp>

AnimationSource *HelicopterType::CreateAnimationSource(const AnimationType *type, RStringB source)
{
#if  _HELISIM_ROTORLIB
  static const struct AnimSourceDesc
  {
    const char *name;
    float (HelicopterRTD::*func)(ObjectVisualState const&) const;
  } desc[]=
  {
    {"rotorH",&HelicopterRTD::GetCtrlRotorH},
    {"rotorV",&HelicopterRTD::GetCtrlRotorV},
    {"rotorHDive",&HelicopterRTD::GetCtrlRotorHDive},
    {"rotorVDive",&HelicopterRTD::GetCtrlRotorVDive},
    {"gear",&HelicopterRTD::GetCtrlGear},
    {"cyclicForward",&HelicopterRTD::GetCtrlCyclicFwd},
    {"cyclicAside",&HelicopterRTD::GetCtrlCyclicAside},
    {"airspeedIndicatorRTD",&HelicopterRTD::GetAirspeedIndicatorRTD},
    {"altBaroRTD",&HelicopterRTD::GetAltBaroRTD},
    {"vertSpeedRTD",&HelicopterRTD::GetVertSpeedRTD},
    {"attitudeBankRTD",&HelicopterRTD::GetAttitudeBankRTD},
    {"attitudeDiveRTD",&HelicopterRTD::GetAttitudeDiveRTD},
    {"skidRTD",&HelicopterRTD::GetSkidRTD},
    {"altRadarRTD",&HelicopterRTD::GetAltRadarRTD},
    {"rpm1RTD",&HelicopterRTD::GetRpm1RTDScaled},
    {"rpm2RTD",&HelicopterRTD::GetRpm2RTDScaled},
    {"rpm3RTD",&HelicopterRTD::GetRpm3RTDScaled},
    {"temp1RTD",&HelicopterRTD::GetTemp1RTDScaled},
    {"temp2RTD",&HelicopterRTD::GetTemp2RTDScaled},
    {"temp3RTD",&HelicopterRTD::GetTemp3RTDScaled},
    {"torqueRTD",&HelicopterRTD::GetTorqueRTDScaled},
    {"torqueEng1RTD",&HelicopterRTD::GetTorqueEng1RTDScaled},
    {"torqueEng2RTD",&HelicopterRTD::GetTorqueEng2RTDScaled},
    {"torqueEng3RTD",&HelicopterRTD::GetTorqueEng3RTDScaled},
    {"mainRotorRPMRTD",&HelicopterRTD::GetMainRotorRPMRTDScaled},
    {"tailRotorRPMRTD",&HelicopterRTD::GetTailRotorRPMRTDScaled},
    {"throttleRTD1",&HelicopterRTD::GetThrottle1},
    {"throttleRTD2",&HelicopterRTD::GetThrottle2},
    {"throttleRTD3",&HelicopterRTD::GetThrottle3},
    {"collectiveRTD",&HelicopterRTD::GetCollective},
    {"rudderRTD",&HelicopterRTD::GetRudder},
    {"bateryChargeRTD",&HelicopterRTD::GetBatteryChargeScaled},
    {"wheel",&HelicopterRTD::GetCtrlWheelRot},
  };
#else
  static const struct AnimSourceDesc
  {
    const char *name;
    float (Helicopter::*func)(ObjectVisualState const&) const;
  } desc[]=
  {
    {"rotorH",&Helicopter::GetCtrlRotorH},
    {"rotorV",&Helicopter::GetCtrlRotorV},
    {"rotorHDive",&Helicopter::GetCtrlRotorHDive},
    {"rotorVDive",&Helicopter::GetCtrlRotorVDive},
    {"gear",&Helicopter::GetCtrlGear},
    {"cyclicForward",&Helicopter::GetCtrlCyclicFwd},
    {"cyclicAside",&Helicopter::GetCtrlCyclicAside},
  };
#endif

  for (int i=0; i<sizeof(desc)/sizeof(desc[0]); i++)
  {
    if (!strcmpi(source,desc[i].name))
      return _animSources.CreateAnimationSource(desc[i].func);
  }
  return base::CreateAnimationSource(type,source);
}

//-----------------------------------------------------------------------------
void HelicopterType::InitDummyPosAndDir()
{
  _gunPosArray.Resize(1);
  _gunDirArray.Resize(1);
  
  _gunPosArray[0] = VZero;
  _gunDirArray[0] = VForward;
}

//-----------------------------------------------------------------------------
Vector3Val HelicopterType::GetGunPos(int index) const
{
  bool checkRange = index<_gunPosArray.Size() && index>=0;
  DoAssert(checkRange);  

  return checkRange? _gunPosArray[index] : VZero;
}

//-----------------------------------------------------------------------------
Vector3Val HelicopterType::GetGunDir(int index) const
{
  bool checkRange = index<_gunDirArray.Size() && index>=0;
  DoAssert(checkRange);  

  return checkRange? _gunDirArray[index] : VForward;
}

//-----------------------------------------------------------------------------
bool HelicopterType::ComputePosAndDirFromBegEnd
                        (const RString &gunBeg,const RString &gunEnd, int index)
{
  bool checkPosSize = index<_gunPosArray.Size() && index>=0;
  bool checkDirSize = index<_gunDirArray.Size() && index>=0;
  DoAssert(checkPosSize);
  DoAssert(checkDirSize);
  DoAssert (_shape);
  
  if (!checkPosSize || !checkDirSize)
    return false;
    
  if (_shape &&
      _shape->MemoryPointExists(gunBeg) &&
      _shape->MemoryPointExists(gunEnd))
  {
    Vector3Val beg  = _shape->MemoryPoint(gunBeg);
    Vector3Val end  = _shape->MemoryPoint(gunEnd);
    Vector3 pos = (beg+end)*0.5f;
    _gunPosArray[index] = pos;
    Vector3 dir = beg-end;
    dir.Normalize();
    if (dir == VZero)
      dir = VForward;
    _gunDirArray[index] = dir;  
  }
  else
  {
    _gunPosArray[index] = VZero;
    _gunDirArray[index] = VForward;
    
    return false;
  }

  return true;    
}                                        

//-----------------------------------------------------------------------------
void HelicopterType::InitTurretsPosAndDir()
{
  const ParamEntry &par=*_par;

  DoAssert (_shape);

  // init pos and dir to default VZero values
  InitDummyPosAndDir();

  // first try to load memoryPoint directly
  ParamEntryPtr memoryPointGun = par.FindEntry("memoryPointGun");
  if(memoryPointGun)
  {
    // we have to split single value and array
    if (memoryPointGun->IsArray())
    {
      int n = memoryPointGun->GetSize();
      _gunPosArray.Resize(n);
      _gunDirArray.Resize(n);
      for (int i=0; i<n; i++)
      {
        RString pointPos = memoryPointGun->operator [](i);
        _gunPosArray[i] = _shape ? _shape->MemoryPoint(pointPos) : VZero;
        _gunDirArray[i] = VForward;
      }
    }
    else
    {
      // single value
      _gunPosArray[0] = _shape ? 
        _shape->MemoryPoint(memoryPointGun->operator RString()) :
        VZero;    
    }  
  }
  
  // try to load pos and dir from gunBeg and gunEnd
  ParamEntryPtr gunBegArr = par.FindEntry("gunBeg");
  ParamEntryPtr gunEndArr = par.FindEntry("gunEnd");

  if (!gunBegArr || !gunEndArr)
    return;
  
  // single values only  
  if (!gunBegArr->IsArray() && !gunEndArr->IsArray())  
  {
    ComputePosAndDirFromBegEnd(
      gunBegArr->operator RString(),gunEndArr->operator RString(),0);
  }  
  else if (gunBegArr->GetSize() == gunEndArr->GetSize())
  {
    // both are arrays.. compare pos and dir arrays size
    int n = gunBegArr->GetSize();
    _gunPosArray.Resize(n);
    _gunDirArray.Resize(n);
    
    // iterate array and compute pos&dir
    for (int i=0; i<n; i++)
    {
      RString gunBegStr = gunBegArr->operator [](i);
      RString gunEndStr = gunEndArr->operator [](i);
      ComputePosAndDirFromBegEnd(gunBegStr,gunEndStr,i);      
    }    
  }  
}

#if _HELISIM_SOUNDS

bool StartStopSoundPars::Load(ParamEntryPar cfg, const RString *pars, int count)
{
  if (!GetValue(_startStopPars[0], cfg >> "startInt"))
    return false;
  
  if (!GetValue(_startStopPars[1], cfg >> "startExt"))
    return false;

  if (!GetValue(_startStopPars[2], cfg >> "stopInt"))
    return false;
    
  if (!GetValue(_startStopPars[3], cfg >> "stopExt"))
    return false;

  if (cfg.FindEntry("damageInt"))
    GetValue(_startStopPars[4], cfg >> "damageInt");

  if (cfg.FindEntry("damageExt"))
    GetValue(_startStopPars[5], cfg >> "damageExt");

  return true;
}

bool LandingSoundsPars::Load(ParamEntryPar cfg)
{
  if (!cfg.FindEntry("landingSoundInt"))
    return false;
  else
    _intPars.Load(cfg , "landingSoundInt");

  if (!cfg.FindEntry("landingSoundOut"))
    return false;
  else
    _extPars.Load(cfg, "landingSoundOut");

  return true;
}

bool ExternNoisePars::Load(ParamEntryPar entry)
{
  if (entry.IsClass())
  {
    if (!GetValue(_pars._pars, entry >> "sound"))
      return false;
    
    RString frequency = entry >> "frequency";
    RString volume = entry >> "volume";

    if (!_pars._freqExpr.Compile(frequency, Transport::SoundControllers(), Transport::SoundControllersCount()))
    {
      LogF("Error: Loading extern noise frequency controller failed, %s", cc_cast(frequency));
      return false;
    }

    if (!_pars._volExpr.Compile(volume, Transport::SoundControllers(), Transport::SoundControllersCount()))
    {
      LogF("Error: Loading extern noise volume controller failed, %s", cc_cast(volume));
      return false;
    }

    _pars._coneUsed = false;
    ConstParamEntryPtr coneEntry = entry.FindEntry("cone");
    if (coneEntry.NotNull())
    {
      if (coneEntry->IsArray() && coneEntry->GetSize() == 4)
      {
        _pars._cone._angleIn = (*coneEntry)[0];
        _pars._cone._angleOut = (*coneEntry)[1];
        _pars._cone._volIn = (*coneEntry)[2];
        _pars._cone._volOut = (*coneEntry)[3];

        _pars._coneUsed = true;
      }
    }
    
    return true;
  }

  return false;
}

bool NoiseSoundPars::Load(ParamEntryPar cfg, const RString *pars, int count)
{
  bool result = false;

  // list of sounds + probabilities
  _randomSound.Load(cfg, "sounds");

  // limit
  _limitUp = cfg >> "limitUp";
  _limitDown = cfg >> "limitDown";
  _timeDelay = cfg >> "timeDelay";

  // 
  RString exprString = cfg >> "expression";

  result = _expr.Compile(exprString, pars, count);
  if (!result)
  {
    RptF("Error: Sound frequency expression: %s", cc_cast(exprString));
    return false;
  }

  return true;
}

#endif

void HelicopterType::InitShape()
{
  const ParamEntry &par=*_par;
  base::InitShape();

  DoAssert (_shape);

  _hRotorStill.Init(_shape,(par>>"selectionHRotorStill").operator RString(),NULL);
  _hRotorMove.Init(_shape,(par>>"selectionHRotorMove").operator RString(),NULL);
  _vRotorStill.Init(_shape,(par>>"selectionVRotorStill").operator RString(),NULL);
  _vRotorMove.Init(_shape,(par>>"selectionVRotorMove").operator RString(),NULL);
  _missileLPos=_shape->MemoryPoint((par>>"memoryPointLMissile").operator RString());
  _missileRPos=_shape->MemoryPoint((par>>"memoryPointRMissile").operator RString());
  _rocketLPos=_shape->MemoryPoint((par>>"memoryPointLRocket").operator RString());
  _rocketRPos=_shape->MemoryPoint((par>>"memoryPointRRocket").operator RString());
  

  InitTurretsPosAndDir();

  ProgressRefresh();
  
  if(par.FindEntry("memoryPointCM"))
  {
    int n = (par >> "memoryPointCM").GetSize();
    _cmPos.Resize(n);
    for (int i=0; i<n; i++)
    {
      RString cmPos = (par >> "memoryPointCM")[i];
      _cmPos[i] = _shape->MemoryPoint(cmPos);
    }
  }
  if (_cmPos.Size() == 0) _cmPos.Add(Vector3(0,0,0));

  if(par.FindEntry("memoryPointCMDir"))
  {
    int n = (par >> "memoryPointCMDir").GetSize();
    _cmDir.Resize(n);
    for (int i=0; i<n; i++)
    {
      RString cmDir = (par >> "memoryPointCMDir")[i];
      _cmDir[i] = _shape->MemoryPoint(cmDir);
    }
  }
  if (_cmDir.Size() == 0) _cmDir.Add(Vector3(1,0,0));


  ProgressRefresh();
  
  int level=_shape->FindLevel(VIEW_GUNNER);
  if (level >= 0)
    _shape->Level(level)->MakeCockpit();
  ProgressRefresh();

  RString namePilotPos = par >> "memoryPointPilot";

  level=_shape->FindLevel(VIEW_PILOT);
  if( level>=0 )
  {
    _shape->Level(level)->MakeCockpit();
    _pilotPos=_shape->NamedPoint(level,namePilotPos);
  }
  ProgressRefresh();
  
  level=_shape->FindLevel(VIEW_CARGO);
  if( level<0 )
    level=_shape->FindLevel(VIEW_PILOT);
  if( level>=0 )
    _shape->Level(level)->MakeCockpit();
  ProgressRefresh();

  if( _pilotPos.SquareSize()<0.1 )
    _pilotPos=_shape->MemoryPoint(namePilotPos);

  _hullHit = FindHitPoint("HitHull");
  _engineHit = FindHitPoint("HitEngine");
  _avionicsHit = FindHitPoint("HitAvionics");
  _rotorVHit = FindHitPoint("HitVRotor");
  _rotorHHit = FindHitPoint("HitHRotor");
  _missiles = FindHitPoint("HitMissiles");
  _glassRHit = FindHitPoint("HitRGlass");
  _glassLHit = FindHitPoint("HitLGlass");

#if _HELISIM_ROTORLIB
  {
    _hitBatteries = FindHitPoint("HitBatteries");
    _hitLight = FindHitPoint("HitLight");
    _hitHydraulics = FindHitPoint("HitHydraulics");
    _hitTransmission = FindHitPoint("HitTransmission");
    _hitGear = FindHitPoint("HitGear");
    _hitFuel = FindHitPoint("HitFuel");
    _hitHStabilizerL1 = FindHitPoint("HitHStabilizerL1");
    _hitHStabilizerR1 = FindHitPoint("HitHStabilizerR1");
    _hitVStabilizer1 = FindHitPoint("HitVStabilizer1");
    _hitTail = FindHitPoint("HitTail");
    _hitPitotTube = FindHitPoint("HitPitotTube");
    _hitStaticPort = FindHitPoint("HitStaticPort");
    _hitEngine2 = FindHitPoint("HitEngine2");
    _hitEngine3 = FindHitPoint("HitEngine3");

    _hitStarter1 = FindHitPoint("HitStarter1");
    _hitStarter2 = FindHitPoint("HitStarter2");
    _hitStarter3 = FindHitPoint("HitStarter3");
  }

  _centerOffsetRTD = _shape ? _shape->MemoryPoint("RTD_center") : VZero;
#endif

  //hitzones texture info 
  _hitZoneTextureUV.Add(7);_hitZoneTextureUV.Add(8);
  _hitZoneTextureUV.Add(11);_hitZoneTextureUV.Add(12);
  _hitZoneTextureUV.Add(10);

  {
    Ref<WoundInfo> damageInfo = new WoundInfo;
    damageInfo->Load(_shape,par>>"dammageHalf");
    _glassDamageHalf.Init(_shape,damageInfo,NULL,NULL);
  }
  {
    Ref<WoundInfo> damageInfo = new WoundInfo;
    damageInfo->Load(_shape,par>>"dammageFull");
    _glassDamageFull.Init(_shape,damageInfo,NULL,NULL);
  }

  //exhausts
  _exhausts.Clear();
  ParamEntryVal array = par >> "Exhausts";
  for (int i=0; i<array.GetEntryCount(); i++)
  {
    ParamEntryVal cls = array.GetEntry(i);
    RString pos = cls >> "position";
    if (!_shape->MemoryPointExists(pos)) continue;
    Vector3 beg = _shape->MemoryPoint(pos);
    Vector3 end = _shape->MemoryPoint((cls >> "direction").operator RString());
    int index = _exhausts.Add();
    _exhausts[index].position = end;
    _exhausts[index].direction = (end - beg).Normalized();
    _exhausts[index].effect = cls >> "effect";
  }
  _exhausts.Compact();

  ProgressRefresh();
}

void HelicopterType::DeinitShape()
{
  base::DeinitShape();
}

void HelicopterType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);

  _glassDamageHalf.InitLevel(_shape,level);
	_glassDamageFull.InitLevel(_shape,level);
	
	_hRotorStill.InitLevel(_shape,level);
	_hRotorMove.InitLevel(_shape,level);
	_vRotorStill.InitLevel(_shape,level);
	_vRotorMove.InitLevel(_shape,level);
}

void HelicopterType::DeinitShapeLevel(int level)
{
  _glassDamageHalf.DeinitLevel(_shape,level);
	_glassDamageFull.DeinitLevel(_shape,level);

	_hRotorStill.DeinitLevel(_shape,level);
	_hRotorMove.DeinitLevel(_shape,level);
	_vRotorStill.DeinitLevel(_shape,level);
	_vRotorMove.DeinitLevel(_shape,level);

  base::DeinitShapeLevel(level);
}


Object* HelicopterType::CreateObject(bool unused) const
{ 
#if _HELISIM_ROTORLIB
   return new HelicopterRTD(this, NULL, unused);
#else
   return new HelicopterAuto(this, NULL, unused);
#endif
}


static Vector3 BodyFriction(Vector3Val speed, float rotorEff, bool large)
{
  Vector3 friction;
  friction.Init();
  float stabXY=rotorEff*0.6f+0.4f;
  static float coefX2 = 3.0f;
  static float coefX1 = 50.0f;
  static float coefX0 = 2.0f;
  friction[0] = speed[0]*fabs(speed[0])*coefX2+speed[0]*coefX1+fSign(speed[0])*coefX0;
  if (!large)
  {
    static float coefZ2 = 2.05f;
    static float coefZ1 = 50.0f;
    static float coefZ0 = 2.0f;

    static float coefY2 = 3.00f;
    static float coefY1 = 500.0f;
    static float coefY0 = 2.0f;
    friction[1] = speed[1]*fabs(speed[1])*coefY2+speed[1]*coefY1+fSign(speed[1])*coefY0;
    friction[2] = speed[2]*fabs(speed[2])*coefZ2+speed[2]*coefZ1+fSign(speed[2])*coefZ0;
  }
  else
  {
    friction[1] = speed[1]*fabs(speed[1])*4+speed[1]*500+fSign(speed[1])*15;
    friction[2] = speed[2]*fabs(speed[2])*1 + speed[2]*100+fSign(speed[2])*6 + speed[2]*speed[2]*speed[2]*0.05f;
  }
  friction[0] *= stabXY;
  friction[1] *= stabXY;
  return friction;
}

/// limit rotor forces in high altitude
static float AltLimit(float alt, float altFullForce, float altNoForce)
{
  if (alt>altFullForce)
  {
    if (alt>altNoForce)
      return 0;
    else
      return 1-(alt-altFullForce)*(1/(altNoForce-altFullForce));
  }
  return 1;
}

float HelicopterType::GetStopDistance() const
{
  return base::GetStopDistance();
}


Vector3 HelicopterType::RotorUpForce(Vector3Val speed, float coef, float alt, float groundEff, bool diag) const
{
  Vector3 force(VZero);
  float spd1=speed[1]+3-18*coef*(1+groundEff);
  saturateMax(spd1,-5);
  // rotor lift envelope
  
  float speedRel = speed.SizeXZ()/(GetMaxSpeedMs()*1.4f);

  const int maxI=_envelope.Size()-1;
  float fSpeed=maxI*speedRel;

  // 
  float liftFactor = 0;
  int iLow=toIntFloor(fSpeed);
  float frac=fSpeed-iLow;
  if( iLow>=maxI )
    liftFactor = _envelope[maxI-1];
  else if( iLow<0 )
    liftFactor = 0; // no up force
  else
  {
    float fLow=_envelope[iLow];
    float fHigh=_envelope[iLow+1];
    liftFactor = fLow+(fHigh-fLow)*frac;
  }
  
  static float liftAdjustMove = 4000.0f;
  static float liftAdjustStat1 = 400.0f;
  static float liftAdjustStat2 = 6000.0f;
  force[1] += liftAdjustMove*liftFactor;

  force[1] -= spd1*fabs(spd1)*liftAdjustStat1+spd1*liftAdjustStat2;
  // avoid too strong negative force
  saturateMax(force[1],-10000);
  #if _ENABLE_CHEATS
    if (diag)
    {
      DIAG_MESSAGE(
        500,"Speed %.1f (rel %.3f), Lift factor %.2f, rotor %.2f, force %5g, ge %.2f",
        speed.SizeXZ()*3.6f, fSpeed, liftFactor, coef, force[1], groundEff
      );
    }
  #endif
  
  /*
  float fwdCoef=speed[2]*(1.0/20);
  saturate(fwdCoef,-1,1);
  force[2]+=force[1]*0.1*fwdCoef;
  */
  return force*AltLimit(alt,_altFullForce,_altNoForce);
}



#if _HELISIM_IMPERIAL_SYSTEM
float HelicopterType::ConvertDistance(float meters, int &stringIDS) const 
{
  if(Glob.config.useImperialSystem > 0)
  {
    if(meters >= 1853) 
    {
      stringIDS = IDS_UI_POSITION_DISTANCE_NAUTICAL_MILES;
      return METER_TO_NAUTICAL_MILE(meters);
    }
    else
    {
      stringIDS = IDS_UI_POSITION_DISTANCE_FEET;
      return METER_TO_FEET(meters);
    }
  }
  else
  {
    if(meters > 1000) 
    {
      stringIDS = IDS_UI_POSITION_DISTANCE_KILOMETERS;
      return METER_TO_KILOMETER(meters);
    }
    else
    {
      stringIDS = IDS_UI_POSITION_DISTANCE;
      return meters;
    }
  }
};


float HelicopterType::ConvertSpeed(float metersPerSecond) const 
{
  return Glob.config.useImperialSystem > 0? METER_TO_KNOT(metersPerSecond) : METER_TO_KM_PER_HOUR(metersPerSecond);
};

float HelicopterType::ConvertHeight(float meters) const 
{
  return Glob.config.useImperialSystem > 0? METER_TO_FEET(meters) : meters;
};

#endif

#if _HELISIM_ROTORLIB
void HelicopterType::ReloadRTDConfig() const
{
#if !_DUMMY_ROTORLIB
  std::string error;

  if(_configNameRTD.GetLength() > 0)
  {
    // open the file
    QIFStreamB in;
    AutoArray<char> _data;

    in.AutoOpen(_configNameRTD);
    int len = in.rest();
    _data.Realloc(len);
    _data.Resize(len);
    in.read(_data.Data(), len);
    in.close();
    if (!in.fail())
    {
      _configRTD.readParamStr(_data.Data(), error);
    }
  }
#endif
}
#endif

#define FAST_COEF (1.0f/25) // use fast/slow simulation mode

float Helicopter::GetBackRotorMalfunction() const
{
  // Square(Square(...)) is here to make sure small damage causes little trouble
  // maps 0.5 to 0.0625
  return Square(Square(floatMin(1,1.3f*GetHitCont(Type()->_rotorVHit))));
}

float Helicopter::FuelLeakRate() const
{
#if _VBS3 //only leak fuel if Hull is severely damaged
  if(GetHitCont(Type()->_hullHit) < 0.7) return 0.0f;
#endif
  // typical heli fuel is about 1000 l
  // we want this to last about 30 sec with 0.8 damage
  const float factor = 1000.0f/30/0.64f;
  return Square(GetHitCont(Type()->_hullHit))*factor;
}

/// assume typical heli has 1000 l, and is able to fly about 2-3 hours
#if  !_HELISIM_ROTORLIB
const float HeliFuelConsumption = 0.1f;
#endif

float Helicopter::FuelTimeLeft() const
{
#if  _HELISIM_ROTORLIB
  float fuelConsumeRate = FuelLeakRate()+Type()->GetFuelConsumptionRate();
#else
  float fuelConsumeRate = FuelLeakRate()+HeliFuelConsumption;
#endif
  return FutureVisualState()._fuel/fuelConsumeRate; 
}

/// acquire current bank based on transformation matrix
static inline float BankFromOrientation(Matrix3Par orient)
{
  float invDive = orient.Direction().InvSizeXZ();
  float asideUp = orient.DirectionAside().Y();
  // if bottom-up, return 1 (nonsense)
  if (invDive<=0)
    return fSign(asideUp);
  return asideUp*invDive;
}

static inline float TailRotateFactor(Vector3Par speed)
{
  return Square(speed[2])*Square(FAST_COEF)*0.3f+fabs(speed[2])*FAST_COEF*0.7f;
}

/// speed at which auto-rotation starts to be seen
const float MinAutoRotateSpeed = 2.0f;
/// speed at which auto-rotation is at max
const float MaxAutoRotateSpeed = 10.0f;

/*
Auto-rotation is expected to be about 7.6 m/s - 1500 ft/min - http://helicopterflight.net/autorotation.htm:
 
while in autorotation, your sink rate will vary between 1,300 and 1,800 feet-per-minute.
Most commonly between 1,500 and 1,800 feet-per-minute.

See also http://www.cybercom.net/~copters/pilot/autorotation.html
*/

void Helicopter::Simulate(float deltaT, SimulationImportance prec)
{
  VisualState &vs = FutureVisualState();

  _isDead = IsDamageDestroyed();

  if( vs._rotorSpeed>0 )
  {
#if  _HELISIM_ROTORLIB
    ConsumeFuel(deltaT*Type()->GetFuelConsumptionRate());
#else
    ConsumeFuel(deltaT*HeliFuelConsumption);
#endif
  }

  ConsumeFuel(deltaT*FuelLeakRate());
  
  //Vector3Val position=Position();

  // calculate all forces, frictions and torques
  Vector3Val speed=vs.ModelSpeed();
  Vector3 force(VZero),friction(VZero);
  Vector3 torque(VZero),torqueFriction(VZero);
  
  // world space center of mass
  Vector3 wCenter(VFastTransform,vs.ModelToWorld(),GetCenterOfMass());

#if _EXT_CTRL //ext. Controller like HLA, AAR
  if(IsExternalControlled())
{
    bool engineOn = speed.Size() > 0.1;
     if(engineOn)
     {
       if(vs._rotorSpeed < 0.9)
       {
        vs._rotorSpeed += 0.3f * deltaT;
        saturateMin(vs._rotorSpeed, 1);
       }
     }else
       if(vs._rotorSpeed > 0)
       {
         vs._rotorSpeed -= 0.1f * deltaT;
         saturateMax(vs._rotorSpeed, 0);
       }
  }
#endif
  // we consider following forces
  // gravity
  // main rotor (in direction of rotor axis, changed by cyclic, controlled by thrust)
  // back rotor (in direction of rotor axis - controlled together with cyclic)
  // body aerodynamics (air friction)
  // main rotor aerodynamics (air friction and "up" force)
  // body and rotor torque friction (air friction)

  Vector3 pForce(VZero); // partial force
  Vector3 pCenter(VZero); // partial force application point
  
  float delta;

  if( _isDead )
  {
    if( _landContact || _waterContact || _objectContact )
      StopRotor();
  }
  if( vs._fuel<=0 )
  {
    // warn about engine malfunction
    if (_rotorSpeedWanted>0 && !IsDamageDestroyed())
    {
      _showDmg = true;
      _showDmgValid = Glob.time + 5.0f;
    }
    StopRotor();
  }

  vs._rotorPositionDelta = vs._rotorSpeed*deltaT*20;

  float backWorking = 1-GetBackRotorMalfunction();

  vs._backRotorPosition += vs._rotorSpeed*backWorking*deltaT*20;
  // keep number small, is equivalent mod 2 (see usage of _rotorPosition)
  vs._rotorPosition=fastFmod(vs._rotorPosition + vs._rotorPositionDelta, 4*3*5*H_PI);

  bool doServo=false;

  // change gears
  float gearWanted = 1-GetPilotGear();
  //if( _gearDamage ) gearWanted=-0.66f;
  if (!Type()->_gearRetracting)
    gearWanted = 0;
  float oldGears = vs._gearsUp;
  delta = gearWanted - oldGears;
  if( fabs(delta)>0.01 )
    doServo=true;

  Limit(delta,-Type()->_gearDownSpeed * deltaT,+Type()->_gearUpSpeed * deltaT);
  vs._gearsUp+=delta; 
  Limit(vs._gearsUp,-1,1);

  const float tholdUp = 0.95f;
  const float tholdDown = 0.05f;
  float gears = vs._gearsUp;
  if (gears >= tholdUp && oldGears < tholdUp) OnEvent(EEGear,false);
  if (gears <= tholdDown && oldGears > tholdDown) OnEvent(EEGear,true);

  // wake up the simulation as needed
  if (vs._rotorSpeed>0 || _rotorSpeedWanted>0.1f || vs._speed.SquareSize() > 0.1f || _angVelocity.SquareSize() > 0.1f)
    OnMoved();
  if( _impulseForce.SquareSize() + _impulseTorque.SquareSize()>0 )
    OnMoved();

  if (GetStopped())
  {
    // reset impulse - avoid cumulation
    _impulseForce = VZero;
    _impulseTorque = VZero;
    vs._modelSpeed = VZero;
    vs._speed = VZero;
    _angVelocity = VZero;
    _angMomentum = VZero;
  }
  
  if( !GetStopped() && !CheckPredictionFrozen() && !IsAttached())
  {
    saturateMin(_rotorSpeedWanted,1-GetHit(Type()->_engineHit));    

    if (_isDead)
      vs._rotorSpeed -= 0.2f * deltaT;
    else
    {
      // main engine
      if(Type()->_rotorHHit >=0 && _hit[Type()->_rotorHHit] > 0.95)
      {
        _rotorSpeedWanted = 0;
        vs._rotorSpeed = 0;
      }
      delta= _rotorSpeedWanted-vs._rotorSpeed; 

      // if we apply rotor up, rotor will stop somewhat faster
      // rotor has significant inertia and it does not stop fast
      float brakeRotor = floatMax(vs._mainRotor*0.2f,0);
      // auto-rotate - when going down, rotor will move faster
      float goingDown = (-vs.ModelSpeed().Y()-MinAutoRotateSpeed)*1.0f/(MaxAutoRotateSpeed-MinAutoRotateSpeed);
      saturate(goingDown,0,1);
      saturateMax(brakeRotor,-goingDown);
      // when airborne, we can assume the pilot wants to make the rotor moving fast
      if (goingDown>0 && !_landContact && _rotorSpeedWanted<=0.1f)
      {
        // auto-rotation may make rotor even moving faster, depending on how fast it moves now
        float tgtSpeed = floatMin(goingDown,1.0f);
        delta = (tgtSpeed-vs._rotorSpeed)*0.04f*deltaT;
#if 0 //_DEBUG || _PROFILE
        DIAG_MESSAGE(1000,Format("Rotor %.3f->%.3f, going down %.2f, MR %.2f->%.2f",vs._rotorSpeed,tgtSpeed,goingDown,vs._mainRotor,_mainRotorWanted));
#endif
      }
      float maxDown = -0.025f - brakeRotor * 0.37f;
      Limit(delta,maxDown*deltaT,+0.05f*deltaT);      
      vs._rotorSpeed+=delta;
    }

    Limit(vs._rotorSpeed,0,1);

    if (IsLocal() && vs._rotorSpeed>0.1f && (_landContact || _objectContact || _waterContact) && vs.DirectionUp()[1] < -0.7f)
    {
      // landed upside down - we need to explode
      float damage = 10.0f;
      LocalDamageMyself(VZero,damage*deltaT,GetRadius());
      _isDead = true;
    }

    // main rotor thrust
    // change main rotor force
    float mrw = floatMin(_mainRotorWanted,vs._rotorSpeed);
    delta = mrw-vs._mainRotor;
    Limit(delta,-0.25f*deltaT,+0.25f*deltaT);
    vs._mainRotor+=delta;
    Limit(vs._mainRotor,-0.2f,vs._rotorSpeed);

    float brw = _backRotorWanted;
    // change back rotor force
    delta = brw-_backRotor;
    Limit(delta,-10*deltaT,+10*deltaT);
    _backRotor+=delta;
    Limit(_backRotor,-1,+1);
    
    // change cyclic
    Limit(_cyclicAsideWanted,-2,+2);
#define DIAG_HELI_CYCLIC_ASIDE 0
#if DIAG_HELI_CYCLIC_ASIDE
    char diagTxt[1024]; sprintf(diagTxt,"_cyclicAsideWanted = %.2f", _cyclicAsideWanted);
    DIAG_MESSAGE(100, diagTxt);
#endif
    delta=_cyclicAsideWanted-vs._cyclicAside;
    //static float cyclicAsideSpeed = 10;
    static float cyclicAsideSpeed = 4;
    static float cyclicForwardSpeed = 10;
    Limit(delta,-cyclicAsideSpeed*deltaT,cyclicAsideSpeed*deltaT);
    static float cyclicAsideMaxVal = 1.0f; //1.0f //TODO - check it
    vs._cyclicAside+=delta;
    Limit(vs._cyclicAside,-cyclicAsideMaxVal,cyclicAsideMaxVal);

    delta=_cyclicForwardWanted-vs._cyclicForward;
    Limit(delta,-cyclicForwardSpeed*deltaT,cyclicForwardSpeed*deltaT);
    vs._cyclicForward+=delta;
    Limit(vs._cyclicForward,-1,1);

    Limit(_rotorDiveWanted,-1,+1);
    delta=_rotorDiveWanted-vs._rotorDive;
    Limit(delta,-0.15f*deltaT,0.15f*deltaT);
    vs._rotorDive+=delta;
    Limit(vs._rotorDive,-1,1);

    // apply aerodynamics of the main rotor
    float rotorSpeed=vs._rotorSpeed;
    if( _isDead )
      // dirty hack - when heli is destroyed, we want it to fall down
      rotorSpeed *= 0.1;
    if (rotorSpeed>0.01f)
    {
      float massCoef=GetMass()*(1.0f/3000);
      bool diag = CHECK_DIAG(DEPath) && GWorld->CameraOn()==this;

      float rotorDiameter = _shape->BoundingSphere()*1.5f; // assume rotor diameter is approximately the heli radius
      float groundEff = 0;
      if (rotorDiameter>0)
      {
        float height = vs.Position().Y()-GLandscape->SurfaceYAboveWater(vs.Position()[0],vs.Position()[2]);
        float relHeight = height/rotorDiameter;
        // approximation of function from http://www.cybercom.net/~copters/aero/ground_effect.html
        groundEff = Square(floatMinMax(1.2f-relHeight,0,1))*0.25f;
      }

      pForce = (Type()->RotorUpForce(speed,vs._mainRotor*rotorSpeed,vs.Position().Y(),groundEff,diag)*(vs._rotorSpeed*vs._rotorSpeed)) * massCoef;
      // rotate force by main rotor orientation
      if (fabs(vs._rotorDive)>1e-3)
      {
        Matrix3 rotDive(MRotationX,vs._rotorDive);
        pForce = rotDive * pForce;
      }
    }
    else
      pForce=VZero;

    float tailRotateFactor = TailRotateFactor(speed);
    
    //Vector3 sideOffset(VZero);
    pCenter.Init();
    
    // debugging back-rotor causing yaw
    //static bool checkNoTorque = true;
    const bool checkNoTorque = false;

    {// main rotor force - "lift"
      // force applied to the center of mass
      pCenter = VZero;
      
      // instead of modifying the force center, we apply a separate torque instead
      // this way we can control the helicopter even when no or negative rotor force is applied
      
#if ARROWS
        AddForce(vs.DirectionModelToWorld(pCenter)+wCenter, vs.DirectionModelToWorld(pForce)*InvMass(), Color(1,1,0));
#endif
      force+=pForce;
    }

    { // cyclic torque simulation
      // instead of modifying the force center, we apply a separate torque instead
      // this way we can control the helicopter even when no or negative rotor force is applied

      float size = _shape->BoundingSphere();

      static float cyclicTorque = 2.11f;
      pForce = Vector3(0,cyclicTorque*GetMass(),0)*Square(rotorSpeed)*AltLimit(vs.Position().Y(),Type()->_altFullForce,Type()->_altNoForce);

      // force applied to the center of the rotor
      static float cAsideFactor = 0.6f;
      static float cForwardFactor = -1.6f;
      pCenter[0] = vs._cyclicAside*cAsideFactor*size;
      pCenter[2] = vs._cyclicForward*cForwardFactor*size;
      pCenter[1] = 0;
      
#if ARROWS
        AddForce(vs.DirectionModelToWorld(pCenter)+wCenter, vs.DirectionModelToWorld(pForce)*InvMass(), Color(0,1,0));
#endif
      //force+=pForce;
      if (!checkNoTorque) torque+=pCenter.CrossProduct(pForce);
      
#if 0
      // apply a stabilizing force
      // we want rotor center to stay approximately fixed
      float maxY = _shape->MinMax()[1].Y();
      float comY = GetCenterOfMass().Y();
      static float maxComFactor = +0.82f;
      float rotCenterY = (maxY-comY)*maxComFactor;
      // for each torque we need to build a force which will fix the point
      static float cAsideFix = +0.1f;
      static float cForwardFix = +0.1f;

      // with high speeds the cyclic is counteracted by y-force and rotational friction
      // without the correction it was possible to reach very high speed
      pForce[0] = pForce[1]*pCenter[0]*rotCenterY*cAsideFix*floatMax(1-tailRotateFactor,0);
      pForce[1] = 0;
      pForce[2] = pForce[1]*pCenter[2]*rotCenterY*cForwardFix*floatMax(1-tailRotateFactor,0);
      
      #if ARROWS
        AddForce(
          DirectionModelToWorld(pCenter)+wCenter,
          DirectionModelToWorld(pForce)*InvMass(),
          Color(0,1,1)
        );
      #endif
      force+=pForce;
#endif
    }
    
    // rotate helicopter when in free fall mode
    if( !_landContact && vs._rotorSpeed<0.1 )
    {
      if (!checkNoTorque)
        torque+=Vector3(0.5,3.2,1.3).CrossProduct(Vector3(0,GetMass(),0));
    }
    
    float backRadius=_shape->BoundingSphere()*0.95;
    // when moving fast, side bank causes torque
    float bank=BankFromOrientation(vs.Orientation());
    
    // without following line the response to back rotor was too strong in mid-speed flight (80 km/h)
    float sizeFactor=backRadius*(1.0/12);
    
    float backMalfunc = GetBackRotorMalfunction();
    float backMalfuncTorque = 5.0f * backMalfunc *vs._rotorSpeed*floatMax(vs._mainRotor+0.1,0.05);
    // avoid applying back rotor in high speed to avoid instability
    float fastAtten = 1-floatMin(fabs(speed[2])*(1.0f/80),1);

    // bank turn should correspond to asin(bank)
    
    float asinBank = bank + (1.0/6)*bank*bank*bank + (3.0/40)*bank*bank*bank*bank*bank;
    //x+(x^3)/6+3/40*(x^5)
    
//    if (this==GWorld->CameraOn())
//      DIAG_MESSAGE(100,Format("TailFactor %.3f, asinBank %.3f",tailRotateFactor,asinBank));
    static float bankTurnCoef = 0.7f;
    pForce=Vector3(
      (asinBank*bankTurnCoef*tailRotateFactor +
        // when flying fast, back rotor has almost zero influence
        (_backRotor*8*(1-backMalfunc) - backMalfuncTorque*4)*fastAtten)
      *(vs._rotorSpeed*vs._rotorSpeed*GetMass()*sizeFactor),
      0,0
    );

    // back rotor should be causing torque only, no force
    //pCenter=Vector3(0,+backRadius*0.08,-backRadius);
    //static float backROffset = +0.11f;
    static float backROffset = +0.008f;
    pCenter=Vector3(0,+backRadius*backROffset,-backRadius);
    torque+=pCenter.CrossProduct(pForce);
    
#if ARROWS
      AddForce(vs.DirectionModelToWorld(pCenter)+wCenter, vs.DirectionModelToWorld(pForce)*InvMass());
#endif
    
    // tail causes turning the chopper into the direction of the flight
    // the force depends only on model-speed X/Y and on forward speed
    static float tailRotX1 = -0.500f;
    static float tailRotX2 = -0.010f;
    static float tailRotY1 = -0.005f;
    static float tailRotY2 = -0.0003f;
    pForce = Vector3(
      tailRotateFactor*(speed[0]*tailRotX1+speed[0]*fabs(speed[0])*tailRotX2)*GetMass(),
      tailRotateFactor*(speed[1]*tailRotY1+speed[1]*fabs(speed[1])*tailRotY2)*GetMass(),
      0
    );
    pCenter=Vector3(0,0,-backRadius);
    if (!checkNoTorque)
      torque+=pCenter.CrossProduct(pForce);
    
#if ARROWS
      AddForce(vs.DirectionModelToWorld(pCenter)+wCenter, vs.DirectionModelToWorld(pForce)*InvMass());
#endif
    
    // convert forces to world coordinates

    vs.DirectionModelToWorld(torque,torque);
    vs.DirectionModelToWorld(force,force);
    
    // angular velocity causes also some angular friction
    // this should be simulated as torque
    //static float damp = 1.0f;
    //static float dampSpeed  = 2.0f;
    static float damp = 2.5f;
    static float dampSpeed  = 0.5f;
    // we want the torque friction to be simulated by real aerodynamic
    // we should do this by sampling a few points and let them generate torques
    if (!checkNoTorque)
      torqueFriction = _angMomentum * ((0.2f+vs._rotorSpeed)*(damp+dampSpeed*tailRotateFactor));
    
    // calculate new position
    VisualState moveTrans = PredictPos<VisualState>(deltaT);

    ApplyRemoteStateAdjustSpeed(deltaT,moveTrans);

    // model space turbulence calculation

    if( Glob.time>_lastTurbulenceTime+2 )
    {
      _lastTurbulenceTime=Glob.time;
      const float maxXT=1;
      const float maxYT=1;
      const float maxZT=1;
      float tx=(GRandGen.RandomValue()-0.5)*(maxXT*2);
      float ty=(GRandGen.RandomValue()-0.5)*(maxYT*2);
      float tz=(GRandGen.RandomValue()-0.5)*(maxZT*2);
      _turbulence=Vector3(tx,ty,tz);
    }

    // body air friction
    Vector3 wind = GLandscape->GetWindSpeed()+_turbulence;
    Vector3 airSpeed = speed + DirectionWorldToModel(wind);
    friction=BodyFriction(airSpeed, vs._rotorSpeed*vs._rotorSpeed, Type()->_maxMainRotorDive>0.01);
    vs.DirectionModelToWorld(friction,friction);

#if ARROWS
      AddForce(wCenter, -friction*InvMass(), Color(0.5,0,0));
#endif
    
    // gravity - no torque
    pForce=Vector3(0,-1,0)*(GetMass()*G_CONST);
    force+=pForce;
#if ARROWS
      AddForce(wCenter,pForce*InvMass());
#endif

    
    // recalculate COM to reflect change of position
    wCenter.SetFastTransform(moveTrans.ModelToWorld(),GetCenterOfMass());
    if( deltaT>0 )
    {
      bool wasLandContact = _landContact;
      _objectContact=false;
      _landContact=false;
      _waterContact=false;

      Vector3 totForce(VZero);

      float crash=0;
      Vector3 crashPos;

      if( prec<=SimulateVisibleFar && IsLocal())
      {
        CollisionBuffer collision;
        GLOB_LAND->ObjectCollision(collision,this,moveTrans);
        #define MAX_IN 0.4
        #define MAX_IN_FORCE 0.1
        #define MAX_IN_FRICTION 0.4

        for( int i=0; i<collision.Size(); i++ )
        {
          // info.pos is relative to object
          CollisionInfo &info=collision[i];
          if (info.object && !info.object->IsPassable())
          {
            info.object->SimulationNeeded(GetVisualStateAge(FutureVisualState()));
            _objectContact=true;
            float cFactor=1;
            if( info.object->GetMass()<60 )
              continue;
            if (dyn_cast<Person, Object>(info.object))
            {
              CheckPersonRotorCollision(info.object);
              continue; // avoid collision with person
            }
            if( info.object->GetType()==Primary )
              cFactor=1;
            else
            {
              cFactor=info.object->GetMass()*GetInvMass();
              saturate(cFactor,0,5);
            }

            if( cFactor>0.05 )
            {
              Vector3Val pos=info.pos;
              Vector3Val dirOut=info.dirOut;
              // create a force pushing "out" of the collision
              float forceIn=floatMin(info.under,MAX_IN_FORCE);
              Vector3 pForce=dirOut*GetMass()*40*forceIn;
              // apply proportional part of force in place of impact
              pCenter=pos-wCenter;
              totForce+=pForce;
              torque+=pCenter.CrossProduct(pForce);
              
              Vector3Val objSpeed=info.object->ObjectSpeed();
              Vector3 colSpeed=vs._speed-objSpeed;

              // if info.under is bigger than MAX_IN, move out
              if( info.under>MAX_IN )
              {
                Vector3 newPos=moveTrans.Position();
                float moveOut=info.under-MAX_IN;
                if( moveOut>0.1 ) 
                {
                  float newCrash = 0.1 * Type()->GetArmor();
                  if (crash < newCrash)
                  {
                    crash = newCrash;
                    crashPos = pos;
                  }
                }
                newPos+=dirOut*moveOut*0.1;
                moveTrans.SetPosition(newPos);
              }

              float newCrash;
              if (moveTrans.DirectionUp()[1] > 0)
                newCrash = (colSpeed.SquareSize()-25.0f)*2.0f;
              else
                // If helicopter is upside down destroy it more
                newCrash = Type()->GetArmor() * 10.0f;

              if (crash < newCrash)
              {
                crash = newCrash;
                crashPos = pos;
              }
              //saturateMax(crash, (colSpeed.SquareSize()-25)*2);

              const float maxRelSpeed=5;
              if( colSpeed.SquareSize()>Square(maxRelSpeed) )
              {
                // adapt _speed to match criterion
                colSpeed.Normalize();
                colSpeed*=maxRelSpeed;
                // only slow down
                float oldSize = vs._speed.Size();
                vs._speed = colSpeed+objSpeed;
                if (vs._speed.SquareSize()>Square(oldSize))
                  vs._speed = vs._speed.Normalized()*oldSize;
              }

              // second is "land friction" - causing no momentum

              float frictionIn=floatMin(info.under,MAX_IN_FRICTION);
              pForce[0]=fSign(speed[0])*20000;
              pForce[1]=speed[1]*fabs(speed[1])*1000+speed[1]*8000+fSign(speed[1])*10000;
              pForce[2]=speed[2]*fabs(speed[2])*150+speed[2]*250+fSign(speed[2])*2000;

              pForce=vs.DirectionModelToWorld(pForce)*GetMass()*(4.0/10000)*frictionIn;
              //saturateMin(pForce[1],0);
              //torque-=pCenter.CrossProduct(pForce);
#if ARROWS
                AddForce(wCenter+pCenter,-pForce*InvMass());
#endif
              friction+=pForce;
              torqueFriction+=_angMomentum*0.15;
            }
          }
        }
      } // if( object collisions enabled )
      
      GroundCollisionBuffer gCollision;
      bool enableLandcontact = true;
      if (wasLandContact && vs.DirectionUp().Y()<0.985f) // cos 10 deg
        enableLandcontact = false;
      int nContactPoint = GLOB_LAND->GroundCollision(gCollision, this, moveTrans, 0.05f, 0, enableLandcontact);

      if( gCollision.Size()>0 )
      {
        // it is hard to have more than 3 contact points
        saturateMin(nContactPoint,5);
        //saturateMax(nContactPoint,gCollision.Size());

        Vector3 gFriction(VZero);
        float maxUnder=0;
        float maxUnderWater = 0;
        #define MAX_UNDER 0.1f
        #define MAX_UNDER_FORCE 0.1f
        
        const float nPointCoef = 3.0f/nContactPoint;
        for( int i=0; i<gCollision.Size(); i++ )
        {
          // info.pos is world space
          UndergroundInfo &info=gCollision[i];
          if( info.under<0 ) continue;
          // we consider two forces
          //ReportFloat("land",info.under);
          float under;
          if( info.type==GroundWater )
          {
            under=floatMin(info.under,3)*0.001f;
            if( maxUnderWater<info.under ) maxUnderWater=info.under;
            _waterContact=true;
            if( vs._speed.SquareSize()>Square(8) ) 
            {              
              float newCrash = 1 * Type()->GetArmor();
              if (crash < newCrash)
              {
                crash = newCrash;
                crashPos = info.pos;
              }
            }
          }
          else
          {
            _landContact=true;
            // we consider two forces
            //ReportFloat("land",info.under);
            if( maxUnder<info.under )
              maxUnder=info.under;

            under=floatMin(info.under,MAX_UNDER_FORCE);
          }
          // one is ground "pushing" everything out - causing some momentum
          Vector3 dirOut=Vector3(0,info.dZ,1).CrossProduct(Vector3(1,info.dX,0)).Normalized();
          pForce=dirOut*GetMass()*40.0f*nPointCoef*under;
          pCenter=info.pos-wCenter;
          torque+=pCenter.CrossProduct(pForce);
          // to do: analyze ground reaction force
          totForce+=pForce;

#if ARROWS
          AddForce(wCenter+pCenter,pForce*under*InvMass());
#endif
          
          // second is "land friction" - causing momentum
          pForce[0]=speed[0]*5000+fSign(speed[0])*10000;
          pForce[1]=speed[1]*fabs(speed[1])*1000+speed[1]*8000+fSign(speed[1])*10000;
          pForce[2]=speed[2]*fabs(speed[2])*150+speed[2]*250+fSign(speed[2])*5000;
          
          pForce=vs.DirectionModelToWorld(pForce)*GetMass()*nPointCoef*(1.0f/10000);
#if ARROWS
          AddForce(wCenter+pCenter,-pForce*InvMass());
#endif
          friction+=pForce;

          // torque applied if speed is big enough
          if( fabs(speed[0])<1 ) pForce[0]=0;
          if( fabs(speed[1])<1 ) pForce[1]=0;
          if( fabs(speed[2])<1 ) pForce[2]=0;
          torque-=pCenter.CrossProduct(pForce); // sub: it is friction

          torqueFriction+=_angMomentum*info.under*15;
        }
        if (maxUnder > 0 || maxUnderWater>0)
        {
          //torqueFriction=_angMomentum*1.0;   
          float newCrash;
          float dX,dZ;
          GLandscape->RoadSurfaceYAboveWater(vs.Position(),&dX,&dZ);
          Vector3 normal(-dX,1,-dZ);
          if (vs.DirectionUp()[1] > 0.0f)
          {
            if ( vs.DirectionUp()*normal.Normalized() > 0.866) // (cos 30)
            {
              // we can assume heli is able to slide forward
              // TODO: handle rotating gear?
              Vector3 slide = vs.ModelSpeed();
              // with gears up we take much more damage
              slide[2] *= vs._gearsUp + (1-vs._gearsUp)*0.2f;
              newCrash = (slide.SquareSize()-25.0f);
            }
            else
            {
              newCrash = vs._speed.SquareSize();
            }
          }
          else
          {
            // If helicopter is upside down destroy it more
            newCrash = Type()->GetArmor() * 10.0f;
          }

          if (crash < newCrash)
          {
            crash = newCrash;            
            crashPos = vs.PositionModelToWorld(GetCenterOfMass());
          }

          if( maxUnder>MAX_UNDER )
          {
            // it is necessary to move object immediately
            Vector3 newPos=moveTrans.Position();
            float moveUp=maxUnder-MAX_UNDER;
            newPos[1]+=moveUp;
            moveTrans.SetPosition(newPos);

            if (vs._speed.SquareSize()>Square(10))
            {
              vs._speed.Normalize();
              vs._speed *= 10;
            }
            saturateMax(vs._speed[1],-0.1f);
          }
        }

        const float maxFord=1.1f;
        if( maxUnderWater>maxFord && IsLocal())
        {
          float damage=(maxUnderWater-maxFord)*0.5f;
          saturateMin(damage,0.2f);
          LocalDamageMyself(VZero,damage*deltaT,GetRadius());
        }
        // if too much submerged, we are dead
        float tooMuchY = (_shape->Max().Y()-_shape->Min().Y())*0.5f;
        if (maxUnderWater>tooMuchY && IsLocal())
          LocalDamageMyself(VZero,5.0f*deltaT,GetRadius());
      }
      force+=totForce;
      if (crash>0)
      {
        if( Glob.time>_disableDamageUntil )
        {
          _disableDamageUntil = Glob.time+0.5f;
          // crash boom bang state - impact speed too high
          _doCrash=CrashLand;
          if( _objectContact )
            _doCrash=CrashObject;
          if( _waterContact )
            _doCrash=CrashWater;
          _crashVolume=crash;
          //
          
          // Do crash damage around crashPos
          PositionWorldToModel(crashPos,crashPos);
          CrashDammage(crash, 0.7 * crashPos + 0.3 * GetCenterOfMass(), 2);
          // If chopper is upside down rather explode... 
          if (IsLocal() && (crash * GetType()->GetInvArmor() >5))           
          {
            // set some explosion
            float maxTime = 5-crash * GetType()->GetInvArmor()*0.2f;
            maxTime *=0.2; //make helicopter explode earlier 
            saturate(maxTime,0.5f,3);
            if (_explosionTime>Glob.time+maxTime)
              _explosionTime=Glob.time+GRandGen.Gauss(0,maxTime*0.3f,maxTime);
          }
        }
      }
    }

    bool stopCondition=false;
    if( _landContact && !_waterContact && !_objectContact )
    {
      // apply static friction
      float maxSpeed=Square(0.7f);
      if( !Driver() ) maxSpeed=Square(1.2f);
      if( vs._speed.SquareSize()<maxSpeed && _angVelocity.SquareSize()<maxSpeed*0.3f )
        stopCondition=true;
    }
    if( stopCondition)
      StopDetected();
    else
      OnMoved();

    // apply all forces

    _lastAngVelocity=_angVelocity; // helper for prediction
    ApplyForces(deltaT,force,torque,friction,torqueFriction);

    //LogF("ang vel %.3f,%.3f,%.3f",_angVelocity[0],_angVelocity[1],_angVelocity[2]);
    //LogF("ang mom %.3f,%.3f,%.3f",_angMomentum[0],_angMomentum[1],_angMomentum[2]);

    StabilizeTurrets(vs.Transform().Orientation(), moveTrans.Orientation());

    Move(moveTrans);
    vs.DirectionWorldToModel(vs._modelSpeed, vs._speed);
  }
  if (_isDead && ( _landContact || _objectContact))
  {
    NeverDestroy();
    IExplosion *smoke = GetSmoke();
    if (smoke)
    {
      smoke->SetExplosion(100,100);
      smoke->Explode();
    }
    // note: high speed during contact causes explosion
  }

  if (EnableVisualEffects(prec))
  {
    // effects simulation
    float surfaceY = GLandscape->SurfaceY(vs.Position().X(), vs.Position().Z());
    bool water = surfaceY < GLandscape->GetSeaLevel();
    float height = vs.Position().Y() - (water ? GLandscape->GetSeaLevel() : surfaceY);
    const float maxHeight = 30.0f;
    saturate(height, 0, maxHeight);
    float factor = vs._rotorSpeed * (maxHeight - height) / maxHeight;
    const float invMaxSpeed = 1.0f / (200.0f / 3.6f);
    float alpha = factor - fabs(vs.ModelSpeed().Z()) * invMaxSpeed;
    if (alpha > 0.01)
    {
      float rotorDammage = (Type()->_rotorHHit < 0 || _hit[Type()->_rotorHHit] < 0.999f) ? 1.0 : 0.0;
      float speed = (20.0f * factor);
      float col = _mainRotorWanted;
      float effectValues[] = {col * rotorDammage * speed, col * rotorDammage *alpha, 0, 0, 0};
      if (water)
        _waterEffect.Simulate(deltaT, prec, effectValues, lenof(effectValues));
      else
        _dustEffect.Simulate(deltaT, prec, effectValues, lenof(effectValues));
    }

    float damage = GetTotalDamage();
    if (vs._rotorSpeed>0 && damage >= 0.4)
    {
      float damageValues[] = {damage, vs._rotorSpeed, 0, 0, 0};
      _damageEffect.Simulate(deltaT, prec, damageValues, lenof(damageValues));
    }

    SimulateExhaust(deltaT, prec);
  }
  base::Simulate(deltaT,prec);
  SimulatePost(deltaT,prec);
}

//return change of thrust, used for exhaust
float Helicopter::GetMoreThrust()
{
  return _mainRotorWanted - FutureVisualState()._mainRotor;
}

void Helicopter::SimulateExhaust(float deltaT, SimulationImportance prec)
{
  if( !EnableVisualEffects(prec) ) return;
  if (!EngineIsOn()) return;
  // simulate exhaust smoke
  int n = _exhausts.Size();
  for (int i=0; i<n; i++)
  {
    // damage
    float damage=GetTotalDamage();

    // intensity
    float moreThrust= GetMoreThrust();

    saturate(moreThrust,0,0.2);
    float intensity=Thrust()*0.3+moreThrust*3+0.1;
    // if( _gearSound ) intensity=1;     // gear change smoke
    intensity+=damage*0.2;
    intensity*=1+damage;

    // Calculate smoke position in model space
    Vector3 exhaustPos = Type()->_exhausts[i].position;

    // Calculate smoke speed in world space
    Vector3 cSpeed;
    float exhaustSize = GetMass() * (1.0 / 16000);
    cSpeed = FutureVisualState().DirectionModelToWorld(Type()->_exhausts[i].direction);
    cSpeed *= 9 * floatMin(1, 0.3 * exhaustSize);
    cSpeed += FutureVisualState()._speed;
    float exhaustValues[] = {intensity, damage, exhaustPos.X(), exhaustPos.Y(), exhaustPos.Z(), cSpeed.X(), cSpeed.Y(), cSpeed.Z()};
    _exhausts[i].Simulate(deltaT, prec, exhaustValues, lenof(exhaustValues));
  }
}

bool HelicopterAuto::EngineIsOn() const
{
  return _rotorSpeedWanted>0.5f;
}

bool HelicopterAuto::IRSignatureOn() const
{
  return _rotorSpeedWanted>0.05f || FutureVisualState()._rotorSpeed>0.05f;
}

void HelicopterAuto::EngineOn()
{
  if (FutureVisualState()._fuel>0)
  {
    _rotorSpeedWanted=1;
    base::EngineOn();
  }
}
void HelicopterAuto::EngineOff()
{
  // Explicit engine stop (SQF "engineOn false", user action or damage).
  // Transport::EngineOff only toggles the flag; the rotor has to be told to
  // spin down as well, otherwise the helicopter stays airborne.
  _rotorSpeedWanted=0;
  base::EngineOff();
}
void HelicopterAuto::EngineOffAction()
{
  StopRotor();
}

float HelicopterAuto::MakeAirborne(bool playerIn)
{
  EngineOn();
  FutureVisualState()._rotorSpeed=_rotorSpeedWanted=1.0;
  FutureVisualState()._mainRotor=_mainRotorWanted=0.7;
  _pilotHeight=_defPilotHeight;
  _collisionLight = true;
#if _VBS3
  if(!_pilotHeightAGL)
  {
    float surfaceY = GLandscape->SurfaceYAboveWater(Position()[0],Position()[2]);
    _pilotHeight += surfaceY;
  }
#endif
  _landContact=false;
  _objectContact=false;
  FutureVisualState()._gearsUp = 1;
  SetPilotGear(false);
  CancelStop();
  return _defPilotHeight;
}

void HelicopterAuto::SetFlyingHeight(float val)
{
  _defPilotHeight = val;
#if _VBS3
  if(Glob.config.IsEnabled(DTSimplifiedHeliModel))
  {
    if(!_pilotHeightAGL)
    {
      _pilotHeight=_defPilotHeight;
      float surfaceY = GLandscape->SurfaceYAboveWater(Position()[0],Position()[2]);
      _pilotHeight += surfaceY;
    }
  }
#endif
}

//returns the relative wanted height from the current height if Contour is off
float HelicopterAuto::GetFlyingHeight()
{
#if _VBS3
  bool enabled = Glob.config.IsEnabled(DTSimplifiedHeliModel);
#else
  bool enabled = true;
#endif
  if (enabled && !_pilotHeightAGL)
    return _pilotHeight - FutureVisualState().Position()[1];
  else
    return _defPilotHeight;
}

float Helicopter::GetGearPos() const
{
  const HelicopterType *type = Type();
  if (!type->_gearRetracting)
    return 0;
  return FutureVisualState()._gearsUp;
}


bool Helicopter::Airborne() const
{
  return !_landContact;
}

void Helicopter::StopRotor()
{
  _rotorSpeedWanted=0;
  base::EngineOff();
}

float Helicopter::GetEngineVol(float &freq) const
{
  freq=_rndFrequency*FutureVisualState()._rotorSpeed*(1-FutureVisualState()._mainRotor*0.1);
  return 1;
}

float Helicopter::GetEnvironVol(float &freq) const
{
  freq=1;
  return FutureVisualState()._speed.Size()/Type()->GetMaxSpeedMs();
}

const CursorTextureInfo *Helicopter::GetCursorTexture(Person *person)
{
  return base::GetCursorTexture(person);
}

const CursorTextureInfo *Helicopter::GetCursorAimTexture(Person *person)
{
  // check if target designator is active
  return base::GetCursorAimTexture(person);
}

#if _HELISIM_SOUNDS

bool SoundHelper::Switch(TransportSoundContex &tsc, const SoundPars& pars, RefAbstractWave &wave, AbstractSubmix *submix, bool loop)
{
  // switch internal / external sample
  float played = wave->GetCurrPosition();
  DestroyWave(wave, submix);

  wave = loop ? CreateLoopWave(pars, tsc, submix) : CreateWave(pars, tsc, submix);

  if (wave)
  {
    if (!loop && played > 0.0f)
      wave->SetStartOffset(played);

    return true;
  }

  return false;
}

void StartStopSound::Initialize()
{
  _state = 0;

  // allow sound cones, fluent volume control etc.
  _submix = GSoundsys->CreateSubmix();
  _submix->SetOutputVoiceSound(CustomFilterOutput);
  _submix->EnableDirectionalSound(false);
}

void StartStopSound::DeInitialize()
{
  helper::DestroyWave(_startWave, _submix.GetRef());
  helper::DestroyWave(_stopWave, _submix.GetRef());

  _submix.Free();
}

struct StartStopWaveCallback: public RefCount
{
  StartStopSound &_ssSound;
  bool _start;
  StartStopWaveCallback(StartStopSound &ssSound, bool start): _ssSound(ssSound), _start(start) {}
  void Execute(AbstractWave *wave)
  {
    if (_start)
      _ssSound.UnmarkStarRunning();
    else
      _ssSound.UnmarkStopRunning();
  }
};

void IntExtSoundHolder::Update(TransportSoundContex &tsc, float &vol, float &distance, int &count)
{
  if (DestroyWanted())
  {
    helper::DestroyWave(_wave, _submix.GetRef());
    _state = 0;
  }

  if (CreateWanted())
  {
    const SoundPars &pars = tsc._inside ? _intPars : _extPars;
    // create wave for selected pars
    _wave = _loop ? helper::CreateLoopWave(pars, tsc, _submix.GetRef()) : helper::CreateWave(pars, tsc, _submix.GetRef());
    // remember state
    _inside = tsc._inside;
    UnrequestWave();
  }

  if (_wave)
  {
    if (_wave->IsTerminated())
    {
      helper::DestroyWave(_wave, _submix.GetRef());
      _state = 0;
    }
    else
    {
      // switch internal <-> external
      if (_allowSwitch && (_inside != tsc._inside))
      {
        _inside = tsc._inside;
        const SoundPars &pars = _inside ? _intPars : _extPars;

        // switch samples
        helper::Switch(tsc, pars, _wave, _submix.GetRef(), _loop);
      }

      const SoundPars &pars = tsc._inside ? _intPars : _extPars;

      // update position & speed
      _wave->SetPosition(tsc._pos, tsc._speed);

      vol += pars.vol;
      distance += pars.distance;
      count++;
    }
  }
}

void StartStopSound::PlayStartSound(TransportSoundContex &ctx)
{
  // remember state
  _inside = ctx._inside;
  const SoundPars &pars = SelectPars(true, ctx._inside);

  // open wave
  _startWave = helper::CreateWave(pars, ctx, _submix);

  if (_startWave)
  {
    float startOffset = 0.0f;
    if (IsStopRunning())
    {
      DoAssert(_stopWave.NotNull());
      if (_stopWave)
      {
        startOffset = 1.0f - _stopWave->GetCurrPosition();
        helper::DestroyWave(_stopWave, _submix); // terminate stop sound
        UnmarkStopRunning();
      }
    }

    if (_startWave)
    {
      if (startOffset > 0.01f)
        _startWave->SetStartOffset(startOffset);

      MarkStarRunning();
      _startWave->SetOnTerminate(StartStopSound::WaveCallback, new StartStopWaveCallback(*this, true));
    }
  }

  UnmarkStartWanted();
}

void StartStopSound::PlayStopSound(TransportSoundContex &ctx)
{
  _inside = ctx._inside;
  const SoundPars &pars = SelectPars(false, ctx._inside);

  _stopWave = helper::CreateWave(pars, ctx, _submix);

  if (_stopWave)
  {
    float startOffset = 0.0f;
    if (IsStarRunning())
    {
      DoAssert(_startWave.NotNull());

      if (_startWave)
      {
        startOffset = 1.0f - _startWave->GetCurrPosition();
        helper::DestroyWave(_startWave, _submix); // terminate start sound
        UnmarkStarRunning();
      }
    }

    if (_stopWave)
    {
      if (startOffset > 0.0f)
        _stopWave->SetStartOffset(startOffset);

      MarkStopRunning();
      _stopWave->SetOnTerminate(StartStopSound::WaveCallback, new StartStopWaveCallback(*this, false));
    }
  }

  UnmarkStopWanted();
}

void StartStopSound::Update(TransportSoundContex &ctx, float &distance, float &volume, int &count)
{
  if (IsStartWanted())
    PlayStartSound(ctx);

  // START internal <-> external switch
  if (_startWave)
  {
    if (IsStarCompleted())
    {
      helper::DestroyWave(_startWave, _submix.GetRef());
      _state = 0;
    }
    else
    {
      const SoundPars &pars = SelectPars(true, ctx._inside);

      // switch internal / external sample
      if (ctx._inside != _inside)
      {
        _inside = ctx._inside;

        if (helper::Switch(ctx, pars, _startWave, _submix.GetRef()))
          _startWave->SetOnTerminate(StartStopSound::WaveCallback, new StartStopWaveCallback(*this, true));
      }

      // wave is played, update pars
      _startWave->SetObstruction(ctx._obstruction, ctx._occlusion);
      _startWave->SetPosition(ctx._pos, ctx._speed);

      volume += pars.vol;
      distance += pars.distance;
      count++;
    }
  }

  if (IsStopWanted())
    PlayStopSound(ctx);

  // STOP internal <-> external switch
  if (_stopWave)
  {
    if (IsStopCompleted())
    {
      helper::DestroyWave(_stopWave, _submix.GetRef());
      _state = 0;
    }
    else
    {
      const SoundPars &pars = SelectPars(false, ctx._inside);

      // switch internal / external sample
      if (ctx._inside != _inside)
      {
        _inside = ctx._inside;

        if (helper::Switch(ctx, pars, _stopWave, _submix))
          _stopWave->SetOnTerminate(StartStopSound::WaveCallback, new StartStopWaveCallback(*this, false));
      }

      // update sound pars
      _stopWave->SetObstruction(ctx._obstruction, ctx._obstruction);
      _stopWave->SetPosition(ctx._pos, ctx._speed);

      volume += pars.vol;
      distance += pars.distance;
      count++;
    }
  }

  /*if (_submix && _submix->WaveCount() > 0)
  {
    float frequency = _submix->Calculate3D(ctx._inside, ctx._pos, ctx._speed, distance, vol);

    if (_startWave)
      _startWave->SetVolume(_startWave->GetVolume(), frequency, true);
    
    if (_stopWave)
      _stopWave->SetVolume(_stopWave->GetVolume(), frequency, true);
  }*/
}

void StartStopSound::UnloadSound()
{
  DeInitialize();
  _state = 0;
}

void StartStopSound::WaveCallback(AbstractWave *wave, RefCount *context)
{
  StartStopWaveCallback *cb = static_cast<StartStopWaveCallback *>(context);

  if (cb)
    cb->Execute(wave);
}

void LandingSounds::Update(TransportSoundContex &ctx, float &distance, float &volume, int &count)
{
  // wait until sound released
  if (_soundWanted && !_wave)
  {
    // select sample pars
    const SoundPars &pars = _pars.GetPars(ctx._inside, GRandGen.RandomValue());

    _wave = helper::CreateWave(pars, ctx, _submix);
    _soundWanted = false;
  }

  // update wave
  if (_wave)
  {
    _wave->SetPosition(ctx._pos, ctx._speed);
    _wave->SetObstruction(ctx._obstruction, ctx._occlusion);

    if (_wave->IsTerminated())
      helper::DestroyWave(_wave, _submix.GetRef());
    else
    {
      volume += _wave->GetVolume();
      distance += _wave->GetMaxDistance();
      count++;
    }
  }
}

void ExternNoise::Update(TransportSoundContex &ctx, Vector3Val front, Vector3Val top)
{
  const SoundParsEx &p = _pars.GetPars();

  float volume = 0, frequency = 0;
 
  p._freqExpr.Evaluate(frequency, ctx._ctrl, ctx._ctrlCount);
  p._volExpr.Evaluate(volume, ctx._ctrl, ctx._ctrlCount);

  if (!ctx._inside && p._pars.name.GetLength() > 0 && volume > 0.0f && frequency > 0.0f)
  {
    if (_wave)
    {
      // update
      if (_submix)
      {
         _wave->SetPosition(ctx._pos, ctx._speed);
        _submix->SetOrientation(top, front);

        float doppler = _submix->Calculate3D(true, ctx._pos, ctx._speed, p._pars.distance, p._pars.vol);
        _wave->SetVolume(volume, doppler);
      }
    }
    else
    {
      // create new wave
      _wave = GSoundScene->OpenAndPlay(p._pars.name, ctx._pos, ctx._speed, volume, frequency, p._pars.distance);

      if (_submix)
        _submix->Include(_wave);
    }
  }
  else
  {
    if (_submix)
      _submix->Exclude(_wave);

    _wave.Free();
  }
}

void NoisesSound::Update(TransportSoundContex &ctx)
{
  if (Glob.time < _pause)
    return;

  float result;
  if (!_pars._expr.Evaluate(result, ctx._ctrl, ctx._ctrlCount))
    return;

  // start new wave
  if (ctx._speed.SquareSize() > 10.0f && (result > _pars._limitUp || result < _pars._limitDown))
  {
    if (!_wave)
    {
      const SoundPars pars = _pars.GetPars(GRandGen.RandomValue(ctx._pos.X(), ctx._pos.Z()));

      _wave = GSoundScene->OpenAndPlayOnce(
        pars.name, ctx._transport, ctx._inside, ctx._pos, ctx._speed, 1.0f, 1.0f, pars.distance);
    }
  }

  // terminate wave
  if (_wave && _wave->IsTerminated())
  {
    _wave.Free();
    _pause = Glob.time + _pars._timeDelay;
  }
}

void NoisesSound::UnloadSound()
{
  if (_wave)
    _wave.Free();

  _pause = Time(0);
}

#endif

void Helicopter::Sound(bool inside, float deltaT)
{
  PROFILE_SCOPE_EX(snHel,sound);
  base::Sound(inside,deltaT);
}

void Helicopter::UnloadSound()
{
  base::UnloadSound();
}

bool Helicopter::HasFlares(CameraType camType) const
{
  return camType!=CamInternal && camType!=CamGunner;
}

float Helicopter::GetHitForDisplay(int kind) const
{
  // see InGameUI::DrawTankDirection
  switch (kind)
  {
  case 0: return GetHitCont(Type()->_hullHit);
  case 1: return GetHitCont(Type()->_engineHit);
  case 2:return GetHitCont(Type()->_rotorVHit);
  case 3:return GetHitCont(Type()->_rotorHHit);
  case 4:return GetHitCont(Type()->_avionicsHit);

  default: return 0;
  }
}

Vector3 Helicopter::CameraPointRel(CameraType camType) const
{
  return GetCenterOfMass();
  /*
  // this code binds camera into the rotor mast - the result is pendulum-like impression
  float maxY = _shape->MinMax()[1].Y();
  Vector3 com = GetCenterOfMass();
  static float maxFactor = 0.82f;
  return com + Vector3(0,(maxY-com.Y())*maxFactor,0);
  */
  //return _shape->AimingCenter();
}

Vector3 Helicopter::CameraPosition() const
{
  return RenderVisualState().PositionModelToWorld(CameraPointRel(CamExternal));
}

const float WindEmitRadius = 40;

bool Helicopter::EmittingWind(Vector3Par pos, float radius) const
{
  float dist2 = pos.Distance2(RenderVisualState().Position());
  return dist2<Square(radius+WindEmitRadius) && RenderVisualState()._rotorSpeed>1e-3f;
}

Vector3 Helicopter::WindEmit(Vector3Par pos, Vector3Par wind) const
{
  VisualState const& vs = RenderVisualState();

  // calculate how much can we effect the wind at given point
  // assume the rotor is quite high above the model center
  Vector3 relPos = vs.PositionWorldToModel(pos)-Vector3(0,2,0);

  // wind is not affects on the sides
  if (fabs(relPos.Y())<1e-2)
    return wind;
  
  if (relPos.SquareSize()>Square(WindEmitRadius))
    return wind;
  
  // how much is helicopter above the point
  float above = -relPos.Y()*0.5;
  //float aside = relPos.SizeXZ();
  float dist = relPos.Size();
  
  float vDist = relPos.Y()*0.05f;
  saturate(vDist,-1,+1);
  // effect depends on how much the rotor is rotating
  float strenght = Square(vs._rotorSpeed)*(1-fabs(vDist))*40*(1-dist/WindEmitRadius);
  // it depends also on rotor blades position
  float bladeCoef = vs._mainRotor*5;
  saturate(bladeCoef,0.5,1.5);
  strenght *= bladeCoef;

  // the more outside, the more we will turn the direction outside
  Vector3 outDir = relPos;
  outDir.Normalize();
  
  // reaching end of the influence area, the effect needs to be weaker
  
  // the more we are under, the more the wind is going outside
  float aboveSign = above;
  saturate(aboveSign,0,+1);
  // TODO: check the relation to the surface as well - surfaces acts as a deflector
  return wind + vs.DirectionModelToWorld(outDir)*(strenght*aboveSign);
}


AnimationStyle Helicopter::IsAnimated(int level) const {return AnimatedGeometry;}

// TODO: move broken glass to Transport class
inline float Helicopter::GetGlassBroken() const
{
  const HelicopterType *type = Type();
  float glassDamage = GetHitCont(type->_hullHit);
  saturateMax(glassDamage,GetTotalDamage());
  saturateMax(glassDamage,GetHitCont(type->_glassLHit));
  saturateMax(glassDamage,GetHitCont(type->_glassRHit));
  return glassDamage;
}

void Helicopter::DamageAnimation(AnimationContext &animContext, int level, float dist2 )
{
  const HelicopterType *type = Type();
  // scan corresponding wound

  float glassDamage = GetGlassBroken();
  if (glassDamage>=0.6)
    // TODO: convert to 3-step wounds
    type->_glassDamageFull.Apply(animContext, _shape, level, 2, dist2);
  else if (glassDamage>=0.3)
    type->_glassDamageHalf.Apply(animContext, _shape, level, 1, dist2);
}

void Helicopter::DamageDeanimation(int level)
{
}

float HelicopterVisualState::GetRotorH(const HelicopterType* type) const
{
  return fastFmod((_rotorPosition * type->_mainRotorSpeed)/(H_PI*2),1);
}

float HelicopterVisualState::GetGear(const HelicopterType* type) const
{
  if (!type->_gearRetracting)
    return 0;
  return _gearsUp;
}


float HelicopterVisualState::GetRotorV(const HelicopterType* type) const
{
  return fastFmod((_backRotorPosition * type->_backRotorSpeed)/(H_PI*2),1);
}

float HelicopterVisualState::GetRotorHDive(const HelicopterType* type) const
{
  return -_rotorDive - type->_neutralMainRotorDive;
}

float HelicopterVisualState::GetRotorVDive(const HelicopterType* type) const
{
  return -_rotorDive - type->_neutralBackRotorDive;
}

void Helicopter::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  if (animContext.GetVisualState().cast<Helicopter>()._rotorSpeed < 0.6f)
    Type()->_hRotorMove.Hide(animContext, _shape, level);
  else
    Type()->_hRotorStill.Hide(animContext, _shape, level);
  if (animContext.GetVisualState().cast<Helicopter>()._rotorSpeed < 0.6f || GetBackRotorMalfunction()>0.5f)
    Type()->_vRotorMove.Hide(animContext, _shape, level);
  else
    Type()->_vRotorStill.Hide(animContext, _shape, level);
  
  // if drawing shadows, old style animation is required?

  if (_weaponsState.ShowFire())
  {
    Type()->_weapons._animFire.Unhide(animContext, _shape, level);
    Type()->_weapons._animFire.SetPhase(animContext, _shape, level, _weaponsState._mGunFirePhase);
  }
  else
    Type()->_weapons._animFire.Hide(animContext, _shape, level);

  // set gun and turret to correct position
  // calculate animation transformation
  // turret transformation

  DamageAnimation(animContext, level, dist2);

  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);
//  SelectTexture(level,FutureVisualState()._rotorSpeed);
  // TODO: animate min-max box
  // set minmax box and sphere
  ShapeUsed shape = GetShape()->Level(level);
  float factor = 1.1;
  Vector3 min = shape->Min();
  Vector3 max = shape->Max();
  Vector3 bCenter = shape->BSphereCenter();
  float bRadius = shape->BSphereRadius();
  // enlarge twice to be sure soldier will fit it
  min = bCenter+(min-bCenter)*factor;
  max = bCenter+(max-bCenter)*factor;
  bRadius *=factor;
  animContext.SetMinMax(min, max, bCenter, bRadius);
}

void Helicopter::Deanimate(int level, bool setEngineStuff)
{
  DamageDeanimation(level);
  base::Deanimate(level,setEngineStuff);
}

bool Helicopter::IsPossibleToGetIn() const
{
  if (GetHit(Type()->_engineHit)>=0.7f)
    return false;
  return base::IsPossibleToGetIn();
}

bool Helicopter::EjectNeeded() const
{
  // prefer emergency landing over ejecting
  //if (GetHitCont(Type()->_engineHit)>=0.8f) return true;
  if (GetTotalDamage()>0.8)
    return true;
  return false;
}

bool Helicopter::IsWorking() const
{
  // FIX: Enable player get into the helicopter even when out of fuel
  // For AI, IsAbleToMove() is checked

  // if (GetHit(Type()->_engineHit)>=0.7f) return false;
  // if (FuelTimeLeft()<30) return false;
  
  return base::IsWorking();
}

float Helicopter::GetFleeDamage() const
{
  float flee = base::GetFleeDamage();
  // if we are low on fuel, we need to retreat
  saturateMax(flee,InterpolativC(FuelTimeLeft(),120,240,1,0));
  // if the back rotor is too damaged, we need to retreat
  saturateMax(flee,GetBackRotorMalfunction());
  return flee;
}

bool Helicopter::IsAbleToMove() const
{
  if (GetHit(Type()->_engineHit)>=0.7f)
    return false;

  // without working backrotor we cannot fly far
  // TODO: implement escape flying with broken rotor for AI 
  if (GetBackRotorMalfunction()>=0.7f)
    return false;
  // we are nearly out of fuel, we need to eject / get out
  if (FuelTimeLeft()<30)
    return false;
  
  // landed upside down heli is not able to move
  if ((_landContact || _objectContact) && FutureVisualState().DirectionUp()[1] < -0.7)
    return false;
  return base::IsAbleToMove();
}

const char **Helicopter::HUDMode(int &mask) const
{
  mask = 0;
  if (!EngineIsOn())
    return NULL;
  float malfunction = GetHitCont(Type()->_avionicsHit);
  if (malfunction>0)
  {
    // assume 10 phases per second
    int phase = Glob.time.ModMs(10000)/100;
    float random = GRandGen.RandomValue(phase);
    // HUD may malfunction more or less
    if (random<=malfunction) return NULL;
  }
  static const char *conditions[]=
  {
    "on","ils","mgun","missile","aamissile","atmissile","rocket","bomb",NULL
  };
  mask |= 1<<0; // on is always on
  if (GetPilotGear()) mask |= 1<<1; // ILS is on when gear is down

  // TODO: which turret?
  TurretContext context;
  if (GetPrimaryGunnerTurret(context))
  {
    WeaponModePtr ptr = context._weapons->ValidatedCurrentWeapon();
    if (ptr.IsValid())
    {
      const AmmoType *ammo = ptr.GetAmmoType();
      if (ammo)
      {
        if (ammo->_simulation == AmmoShotBullet || ammo->_simulation == AmmoShotSpread || ammo->_simulation == AmmoShotShell)
        {
          mask |= 1<<2; //mgun
        }
        else
        {
          mask |= 1<<3; //missile // back compatibility
          if (ammo->_simulation == AmmoShotMissile) 
          {
            if  (ammo->maxSpeed<150) //its not nice, but here is no other option, LGBs are also bombs but maneuvrability > 10 and maxControlRange > 10
            {
              mask |= 1<<7; //bomb
            }
            else
            {
              if  (ammo->maxControlRange>10) // guided missile
              {
                if (ammo->irLock)
                {
                  if (ammo->airLock == AmmoLockAirTT || ammo->airLock == AmmoLockAirGroundTT)
                    mask |= 1<<4; //aamissile
                  if (ammo->airLock == AmmoLockGroundTT || ammo->airLock == AmmoLockAirGroundTT)
                    mask |= 1<<5; //atmissile
                }
              }
              else
              {
                mask |= 1<<6; //rocket
              }
            }
          }
        }
      }
    }
  }
  return conditions;
}

DEFINE_CASTING(HelicopterAuto)

HelicopterAuto::HelicopterAuto( const EntityAIType *name, Person *pilot, bool fullCreate)
:Helicopter(name,pilot,fullCreate),_pilotHeading(0),_pilotSpeed(VZero),_pilotHeight(2.5),
_pilotHeadingSet(false),_pilotDiveSet(false),_pilotBankSet(false),
_hoveringAutopilot(false),
_dirCompensate(0.5),
_defPilotHeight(50),
_bankWanted(0),_diveWanted(0), _minAttackDive(0),
_forceDive(10),_forceBank(10),

_pilotAvoidHigh(TIME_MIN),
_pilotAvoidHighHeight(0),
_pilotAvoidLow(TIME_MIN),
_pilotAvoidLowHeight(100000),

_pilotDive(0), // dive set by pilot
_pilotHeightHelper(true),
_pilotSpeedHelper(true),
_pilotDirHelper(true),
_pilotVSpeedHelper(false),
_pilotCyclicDirect(false),
_pilotHeightAGL(true),
_state(AutopilotFar),
_analogueThrottleTime(TIME_MIN),
_thrustHelperTime(TIME_MIN),
_actualCollectiveHelperTime(TIME_MAX),
_lastForward(FLT_MAX),
_lastThrotleDirect(FLT_MAX),
_pressedUp(false),_pressedDown(false),
#if _VBS3_AVRS
_heliVector(VZero),
#endif
_sweepDelay(Glob.time),
_sweepState(SweepDisengage)
{
}

/**
@return above ground level
@param minAboveObstacle accumulate height above "most risky" obstacle
*/
static void MinSafeAltitude(Vector3Val estPos, Vector3Val oldPos, float surfaceY, float &minAboveObstacle)
{
  // first approximation: objects 40 m tall everywhere

  // check the object list
  // if the object is not loaded yet, we do not care about it
  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,estPos,oldPos, 40);
  // never fly closer than a certain limit, to stay safe of unexpected objects

  for (int z=zMin; z<=zMax; z++) for (int x=xMin; x<=xMax; x++)
  {
    // TODO: quick reject objects which lie too far from our trajectory
    const ObjectList &list = GLandscape->GetObjects(z, x);
    //always be at least 30m above any building
    minAboveObstacle = floatMin(minAboveObstacle, (estPos.Y() - (list.GetMax().Y() + 30)));
  }
}

const float MaxBank = 0.90f;

void HelicopterAuto::Move(Matrix4Par transform)
{
  base::Move(transform);
  Matrix3 modelToWorld(MUpAndDirection,VUp,transform.Direction());
  _worldToRelative = modelToWorld.InverseRotation();
}

void HelicopterAuto::Move(Vector3Par position)
{
  base::Move(position);
}

void HelicopterAuto::Init( Matrix4Par pos, bool init )
{
  base::Init(pos, init);
  Matrix3 modelToWorld(MUpAndDirection,VUp,pos.Direction());
  _worldToRelative = modelToWorld.InverseRotation();
}

void HelicopterAuto::Simulate(float deltaT, SimulationImportance prec)
{
  // switch to wreck simulation and drawing if needed
  if (!_isWreck)
    _isWreck = GetDestructType() == DestructWreck && _isDead && _landContact;

  VisualState &vs = FutureVisualState();
  if (_isWreck)
  {
    // switch off the engine - very fast
    _rotorSpeedWanted = 0;
    vs._rotorSpeed -= 2.0f * deltaT;
    saturateMax(vs._rotorSpeed,0);
    SimulateWreck(deltaT, prec);
    return;
  }

  PROFILE_SCOPE_EX(simHe,sim);
  //if( !_driver && !_isDead ) StopRotor();

  // get simple approximations of bank and dive
  // we must consider current angular velocity
  float massCoef=GetMass()*(1.0/3000);
  saturate(massCoef,1,3);

  static float minEstM = 0.5f,maxEstM = 0.5f;
  static float minEstT = 1.0f,maxEstT = 1.0f;
  
  float dirEstT = Interpolativ(massCoef,minEstM,maxEstM,minEstT,maxEstT);
  
  const Matrix3 &orientation=vs.Orientation();

  //Vector3Val angAcceleration=(_angVelocity-_lastAngVelocity)/deltaT;
  //Vector3Val avgAngVelocity = _angVelocity+angAcceleration*0.5*dirEstT;
  Vector3Val avgAngVelocity = _angVelocity;
  Matrix3Val derOrientation = avgAngVelocity.Tilda()*orientation;

  Matrix3Val estOrientation=orientation+derOrientation*dirEstT;
  
  Vector3Val estDirection=estOrientation.Direction().Normalized();
  Vector3Val estDirectionUp=estOrientation.DirectionUp().Normalized();
  Vector3Val estDirectionAside=estOrientation.DirectionAside().Normalized();
  
  Matrix3 estOrientationNorm;
  estOrientationNorm.SetDirection(estDirection);
  estOrientationNorm.SetDirectionUp(estDirectionUp);
  estOrientationNorm.SetDirectionAside(estDirectionAside);

  float bank=BankFromOrientation(estOrientationNorm);
  float dive=estDirection.Y();

#if _ENABLE_CHEATS
  if (this==GWorld->CameraOn() && CHECK_DIAG(DEPath))
  {
    const char *state = FindEnumName(_state);
    DIAG_MESSAGE(500,"PHeight %.1f, PSpeed %.1f, Bank %.2f->%.2f, Dive %.3f->%.3f, APState %s",
      _pilotHeight,_pilotSpeed[2]*3.6f,bank,_bankWanted,dive,_diveWanted,state);
  }
#endif
  float avoidGroundMRW=_mainRotorWanted;

  //if pilot or copilot is alive
  bool driverAlive = 
    (_driver && _driver->Brain() && _driver->Brain()->LSIsAlive()) ||
    (_copilot && _copilot->Brain() && _copilot->Brain()->LSIsAlive());
  bool autorotating = false;

  float belowFlightLevel = 0;
  if (driverAlive)
  {
    if( _pilotHeightHelper)
    {
      Vector3Val speed = vs._speed;
      // autopilot: convert simple _pilot control
      // to advanced simulation model
      
      // use acceleration to estimate change of position
      // estimate vertical acceleration
      //float estY=Position.Y()+_pilotSpeed*EST_DELTA+_acceleration*EST_DELTA*EST_DELTA*0.5;
      // estimate position after 
      const float estT=2;
      const float accPredict=0.2; // estimate acceleration only partially
      Vector3 estPos=vs.Position()+speed*estT+0.5*accPredict*estT*estT*vs._acceleration;
      
      float minEstHeight=estPos.Y();
      float minAboveObstacle = FLT_MAX;
      float estSurfaceY = GLandscape->SurfaceYAboveWaterNoWaves(estPos.X(),estPos.Z());
      // we may need to consider nearby objects as well
      float estHeight=estPos.Y()-estSurfaceY;
      float targetHeight = _pilotHeight;
      if (_pilotHeightAGL)
      {
        // first AGL sample
        minEstHeight = estHeight;

        if( speed.SquareSizeXZ()>Square(5) )
        {
          // predict terrain
          Vector3 oldPos = estPos;
          for( int t=1; t<4; t++ )
          {
            float estT=t*0.8f;
            Vector3 estPos=vs.Position()+speed*estT+0.5f*estT*estT*vs._acceleration;
            float estSurfaceY = GLandscape->SurfaceYAboveWaterNoWaves(estPos.X(),estPos.Z());
            MinSafeAltitude(estPos,oldPos,estSurfaceY,minAboveObstacle);
            oldPos = estPos;
            float estHeight=estPos.Y()-estSurfaceY;
            saturateMin(minEstHeight,estHeight);
          }
        }
        else
          MinSafeAltitude(estPos,estPos,estSurfaceY,minAboveObstacle);

        // we need to keep minEstHeight above zero at all cost
        // otherwise we fly into a crash
        // if we are ordered to fly low, we need to obey, even if we risk a collision?
        const float minHeight=10;
        float flightLevelWarning = floatMax(targetHeight-5.0f,minHeight);
        if( _pilotHeight>=minHeight && (_stopMode==SMNone || _state!=AutopilotReached))
        {
          // always assume obstacles minHeight tall may exist
          saturateMin(minAboveObstacle,estPos.Y()-(estSurfaceY+minHeight));
          // no need to be afraid until we could be too near to the ground
          // slight flying under assigned level should not matter
          const float separationNeeded = GetCollisionRadius(vs)*0.66f;
          if (minAboveObstacle<separationNeeded)
          {
            float avoidGroundCAY=(separationNeeded-minAboveObstacle)/separationNeeded;
            avoidGroundMRW = vs._mainRotor+floatMax(avoidGroundCAY,0);
          }
          saturateMax(targetHeight,estHeight+separationNeeded-minAboveObstacle);
          saturateMax(flightLevelWarning,estHeight+separationNeeded-minAboveObstacle);
          //DIAG_MESSAGE(1000,Format("TgtH %.1f EstH %.1f MinAb %.1f",targetHeight,estHeight,minAboveObstacle));
        }
        belowFlightLevel = flightLevelWarning-estHeight;
      }

      float changeAY = (targetHeight-minEstHeight)*0.1f;
      
      // when flying low, avoid diving too fast
      const float maxSpeed = Type()->GetMaxSpeedMs();
      float minYSpeed = Interpolativ(minEstHeight,maxSpeed*0.2f,maxSpeed*0.4f,-maxSpeed*0.06f,-maxSpeed*0.25f);
      
      float minChangeAY = minYSpeed-speed[1];
      saturateMax(changeAY,minChangeAY);
      
      // it has no sense to change _mainRotorWanted faster than _mainRotor can be changed
      // it would only cause excessive traffic
      _mainRotorWanted=floatMinMax(changeAY,-0.25f*deltaT,+0.25f*deltaT)+vs._mainRotor;
      // if rotor is off, we need to auto-rotate
      if (!QIsManual(EffectivePilotingUnit()) && !_landContact && !_objectContact && !_waterContact)
      {
        {
          // when flying fast, never let rotor wanted drop too much - we will need it soon
          float fast = (fabs(_pilotSpeed[2])+fabs(vs.ModelSpeed()[2]))*0.5f;
          float minRotor = InterpolativC(fast,0,Type()->GetMaxSpeedMs()*0.7f,-1,0.5);
          saturateMax(_mainRotorWanted,minRotor);
        }

        // AI auto-rotation
        if (_rotorSpeedWanted<0.1f ||
          // autorotate when we need landing and are close enough to the suitable landing position
          GetBackRotorMalfunction()>=0.7f && fabs(vs.Speed().SizeXZ())<Type()->GetMaxSpeedMs()*0.2f &&
          _stopMode!=SMNone && _stopPosition.DistanceXZ2(vs.Position())<Square(50.0f)
        )
        {
          // for optimal auto-rotation we keep steady descend speed
          // if _rotorSpeed goes down, we should dive faster
          float descend = MinAutoRotateSpeed+(MaxAutoRotateSpeed-MinAutoRotateSpeed)*(1-vs._rotorSpeed);

          autorotating = true;

          const float autoRotateLandHeight = 10.0f;
          if (estHeight<autoRotateLandHeight)
          {
            _rotorSpeedWanted = 0;
            // once we are close to ground, we want to slow down the descent
            descend = InterpolativC(estHeight,0,autoRotateLandHeight,0.5,descend);
            _mainRotorWanted = vs._mainRotor + -vs.ModelSpeed().Y()-descend;
            SetPilotGear(true);
          }
          else
            _mainRotorWanted = floatMin(vs._mainRotor + -vs.ModelSpeed().Y()-descend,0);
#if 0 // _DEBUG || _PROFILE
          DIAG_MESSAGE(500,Format("AI autorotate descend %.2f, MR %.2f->%.2f",descend,vs._mainRotor,_mainRotorWanted));
#endif
        }
      }
      // we need to be able to pass _mainRotorWanted higher than 1 to other parts of the autopilot
      // we will apply saturation during rotor simulation anyway
      saturate(_mainRotorWanted,-1,2.0f);
    }
    else if (_pilotVSpeedHelper)
    {
      // autopilot: convert simple _pilot control
      // to advanced simulation model
      
      // use acceleration to estimate change of position
      // estimate vertical acceleration
      //float estY=Position.Y()+_pilotSpeed*EST_DELTA+_acceleration*EST_DELTA*EST_DELTA*0.5;
      // estimate position after 
      const float estT=0.5f;
      float estSpeedY=vs._speed.Y()+estT*vs._acceleration.Y();
      float changeAY=(_pilotSpeed[1]-estSpeedY)*0.1f;
      // this may mean descending even faster than _pilotSpeed[1] dictates us
      _mainRotorWanted=changeAY+vs._mainRotor;

      // if helicopter is reversed, we need to apply reversed rotor as well
      // if helicopter is banked, we may need to apply more thrust
      // but this we ignore for now
      // we want to avoid abrupt changes, therefore we apply a smooth transition
      float reversedRotor = InterpolativC(vs.DirectionUp().Y(),-0.2f,-0.0f,-1,+1);
      _mainRotorWanted *= reversedRotor;
      saturate(_mainRotorWanted,-1,1);
    }
  }

  const Vector3 relSpeed=vs.ModelSpeed();
  float zPilotSpeed=_pilotSpeed[2];

  // changeAccel is required for speed helper and dirhelper
  float estAccT=3.0;
  // the speed we want is given in the horizontal plane
  // we want to rotate it azimut-wise only
  // if there is singularity here, we cannot control anything, the helicopter is aiming up or down
  Matrix3 rotSpeed(MUpAndDirection,VUp,vs.Direction());
  Vector3 absSpeedWanted = rotSpeed * _pilotSpeed;

  if (_rotorSpeedWanted<0.1f || autorotating)
  {
    // prevent moving too fast on landing
    const float maxAutorotateSpeedWanted = 6.0f;
    if (absSpeedWanted.SquareSizeXZ()>Square(maxAutorotateSpeedWanted))
    {
      absSpeedWanted = absSpeedWanted*(absSpeedWanted.InvSizeXZ()*maxAutorotateSpeedWanted);
    }
  }

  static float speedF = 1.0f;
  static float accF = 0.2f;
  Vector3 speedDif = DirectionWorldToRelative(absSpeedWanted-vs._speed);
  Vector3 changeAccel=(absSpeedWanted-vs._speed)*(1/estAccT)*speedF-vs._acceleration*accF;
  float bankLimit=0.80f;

  if( _pilotSpeedHelper && driverAlive)
  {
    // if we need to accelerate forward, move cyclic forward
    changeAccel = DirectionWorldToRelative(changeAccel);
    
    // depending on the change desired, we will limit max. angles as well
    static float maxDiveF = 0.08f;
    // note: some dive is also needed to maintain current speed
    //float curSpeedDive = Interpolativ(_pilotSpeed[2],0,Type()->GetMaxSpeedMs(),0,0.1f);
    // max allowable dive at max speed is ~ 0.15
    // max allowable dive at max speed*0.9 is ~ 0.2
    
    static const float speedLint[]={0, 0.33, 0.8,  1.0};
    static const float diveLint []={1, 0.3,  0.19, 0.13};
    COMPILETIME_COMPARE(lenof(speedLint),lenof(diveLint));
    
    float curSpeedMaxDive = Lint(speedLint,diveLint,lenof(speedLint),relSpeed[2]/Type()->GetMaxSpeedMs());
    
    float maxDive = fabs(speedDif[2])*maxDiveF+0.05f; // max. nose up
    float minDive = -curSpeedMaxDive; // max. nose down
    // if we need to apply more rotor power that we have
    // we have to limit cyclic movement
    if( avoidGroundMRW>=1.0f )
    { // limit dive - we need to climb to avoid ground collision
      // if we are moving forward, we may want to climb up using dive
      float avoidW=floatMin(avoidGroundMRW-1,1);
      float speedDL=fabs(relSpeed[2])*0.025f;
      // negative dive is accelerating forward
      if( relSpeed[2]>5 )
        minDive=Lerp(minDive,floatMin(+speedDL,+0.25f),avoidW);
      else if( relSpeed[2]<-5 )
        maxDive=Lerp(maxDive,floatMax(-speedDL,-0.25f),avoidW);
      else
      {
        // low speed climb - disable dive
        minDive = Lerp(minDive,-0.2f,avoidW);
        maxDive = Lerp(maxDive,+0.2f,avoidW);
      }
      bankLimit=1.0f-0.8f*avoidW;
    }

    // if we are under the assigned flight level, we may need to avoid diving more when moving fast
    if (_pilotHeightHelper)
    {
      static float climbByDiveMul = +0.05f;
      static float climbByDiveAdd = -0.10f;
      float flightLevelDive = floatMin(belowFlightLevel*climbByDiveMul+climbByDiveAdd,0.7f);
      
      // limiting depends on speed - when moving slow we need not limit diving that much
      flightLevelDive = Interpolativ(relSpeed.Z(),0,20,0,flightLevelDive);
      
      // once at flight level or only slightly below, apply no limitations
      // if we are not at full rotor power, relax limitation
      float relaxPowerReserve = 1-_mainRotorWanted*1.5f;
      float relaxHighEnough = InterpolativC(belowFlightLevel,0,-2,1,-0.5f);
      
      float minFlightLevelDive = floatMin(-relaxPowerReserve,-relaxHighEnough,flightLevelDive);


      if (minDive<minFlightLevelDive)
        minDive = minFlightLevelDive;
    }

    // when moving slow, dive corresponds to acceleration
    // when moving fast, dive corresponds to speed
    if( fabs(_forceDive)<0.9f )
    {
      _diveWanted=_forceDive;
      if (CHECK_DIAG(DEPath) && GWorld->CameraOn()==this)
        DIAG_MESSAGE(100,"dive forced %.2f (min %.2f, max %.2f, speed %.2f), below %.1f",
          _diveWanted,minDive,maxDive,-curSpeedMaxDive,belowFlightLevel);
      saturate(_diveWanted,minDive,maxDive);
    }
    else
    {
      static float fastDiveFactor = 1.0f;
      float fastDive=fabs(zPilotSpeed)*(fastDiveFactor/Type()->GetMaxSpeedMs());
      //  when moving fast, we want the change to be slower
      Limit(fastDive,0,0.9f);
      _diveWanted = dive + changeAccel[2]*-0.15f*(1-fastDive);

      if (CHECK_DIAG(DEPath) && GWorld->CameraOn()==this)
        DIAG_MESSAGE(
          100,"_diveWanted %.2f (min %.2f, max %.2f, speed %.2f, ca %.2f), fastDive %.2f, below %.1f",
          _diveWanted,minDive,maxDive,-curSpeedMaxDive,
          changeAccel[2],fastDive,belowFlightLevel
        );
      saturate(_diveWanted,minDive,maxDive);
    }

    if (!QIsManual() && !_landContact && !_objectContact && !_waterContact)
    {
      // AI auto-rotation - make sure there is some wind into the rotor
      if (_rotorSpeedWanted<0.1f)
      {
        // we may need to slow down a little bit
        // landing autopilot is active at this time, we only want to correct it, not to disable it
        saturate(_diveWanted,-0.05f,0.2f);
      }
    }
  }

  // when moving slow, do side slip
  float bankInTurn=fabs(zPilotSpeed)*FAST_COEF; // probably in turn
  saturateMin(bankInTurn,1);

  float turnByBank=(fabs(relSpeed[2])+fabs(_pilotSpeed[2])-4)*0.05f;
  saturate(turnByBank,0,1);
  saturateMax(turnByBank,bankInTurn);
  
  if (_pilotDirHelper && driverAlive)
  {
    _bankWanted = 0;
    //float dirC = _dirCompensate;
    static float dirC = 1.0f;
    //float dirC = 0.9f;
    //float dirC = 0.0f;
    //0.5f;
    Vector3 direction=vs.Direction()*(1-dirC)+estDirection*dirC;
    float curHeading=atan2(direction[0],direction[2]);
    static float chCoef = 4.0f;
    float changeHeadingAng=AngleDifference(_pilotHeading,curHeading)*chCoef;
    Limit(changeHeadingAng,-1,1);
    // when heading change is low, we want to make it even lower
    float slowChH = changeHeadingAng * fabs(changeHeadingAng);

    // we want more of the slow (quadratic) change
    float changeHeading = Lerp(changeHeadingAng,slowChH,0.8f);

    if (CHECK_DIAG(DEPath) && GWorld->CameraOn()==this)
    {
      #if _ENABLE_REPORT
        float curHeadingNoC = atan2(vs.Direction()[0],vs.Direction()[2]);
        float changeHeadingNoC = AngleDifference(_pilotHeading,curHeadingNoC)*chCoef;
      #endif
      DIAG_MESSAGE(
        100,
        "changeHeading %.2f (ang %.2f, slow %.2f, noC %.2f)",
        changeHeading*(180/H_PI),changeHeadingAng*(180/H_PI),slowChH*(180/H_PI),changeHeadingNoC*(180/H_PI)
      );
    }

    // when slow, use back rotor
    float fastTurn=floatMax(fabs(relSpeed[2])-6,0)*FAST_COEF;
    Limit(fastTurn,0,1);
    _backRotorWanted=(-changeHeading*2)*(1-fastTurn);
    saturate(_backRotorWanted,-1,+1);

    // when moving fast, turn by banking
    float bankWantedFast=-changeHeading*turnByBank;

    _bankWanted += bankWantedFast;
  }
  
  if (_pilotSpeedHelper && driverAlive)
  {
    float bankWantedSlow = bank+changeAccel[0]*(-1.0/20);

    static float maxBankF = 0.05f;
    float maxSpeedBank = fabs(speedDif[0])*maxBankF;
    saturate(bankWantedSlow,-maxSpeedBank,+maxSpeedBank);
    // avoid using extreme bank for side-slips
    saturate(bankWantedSlow,-0.5f,0.5f);
    _bankWanted = Lerp(bankWantedSlow,_bankWanted,turnByBank);
  }

  if( fabs(_forceBank)<1 )
    _bankWanted=_forceBank;
  saturate(_bankWanted,-MaxBank,MaxBank);

  if (_pilotSpeedHelper && driverAlive)
  {
    // bank is limited 
    //if( fabs(_diveWanted)>0.3 ) 
    // if diving, limit bank
    saturate(_diveWanted,-0.7f,0.6f);
    float diveLimitBank=MaxBank-floatMax(fabs(_diveWanted),fabs(dive))*1.3f;
    saturateMax(diveLimitBank,0.1f);
    saturateMin(bankLimit,diveLimitBank);
    saturate(_bankWanted,-bankLimit,+bankLimit);
  }

  if ((!_driver || _driver->IsDamageDestroyed()) && (!_copilot || _copilot->IsDamageDestroyed()))
  {
    if (!_landContact)
    {
      _cyclicAsideWanted=-0.1;
      _cyclicForwardWanted=0.1;
      _backRotorWanted=-0.1;
      _mainRotorWanted=0.1;
    }
    else
    {
      _cyclicAsideWanted=0;
      _cyclicForwardWanted=0;
      _backRotorWanted=0;
      _mainRotorWanted=0;
    }
  }
  else
  {
    if (_pilotSpeedHelper)
#define DIAG_HELI_BANK 0
#if DIAG_HELI_BANK
      char diagtxt[1024]; sprintf(diagtxt, "heli _bankWanted = %.2f", _bankWanted);
      DIAG_MESSAGE(100, diagtxt);
#endif
    if( vs._rotorSpeed>=0.3 )
    {
      if( !_landContact )
      {
        static float cFactorSlow = 1.0f;
        static float cFactorFast = 3.0f;
        
        // when moving fast, we need to apply cyclic faster as well
        
        float tailRotateFactor = TailRotateFactor(relSpeed);
        
        float cFactor = tailRotateFactor*cFactorFast+(1-tailRotateFactor)*cFactorSlow;

        float fastTurn = floatMax(fabs(relSpeed[2])-6,0)*FAST_COEF;
        Limit(fastTurn,0,1);
        
        _cyclicAsideWanted = +(_bankWanted-bank)*cFactor;

        float minRotorDive = floatMin(Type()->_minMainRotorDive,Type()->_minBackRotorDive);
        float maxRotorDive = floatMax(Type()->_maxMainRotorDive,Type()->_maxBackRotorDive);

        float diveByRotor = _diveWanted;
        
        float adjustedDive = 0;
        if(_sweepState==SweepEngage && fabs(_minAttackDive) > 0)
        {
          //limit attack dive
          _minAttackDive = fabs(_minAttackDive) > fabs(_diveWanted) ? _diveWanted:_minAttackDive;
          // _rotorDiveWanted should be opposite to _diveWanted or _minAttackDive
          // as in _rotorDiveWanted = -_diveWanted;
          // and so before any actual adjustedDive can happen, we should set _rotorDiveWanted to min/max
          _rotorDiveWanted = (_minAttackDive > 0) ? minRotorDive: maxRotorDive;
          adjustedDive = _minAttackDive;
          _minAttackDive = 0;
        }
        else
        {
          saturate(diveByRotor,minRotorDive,maxRotorDive);
          _rotorDiveWanted = -diveByRotor;
          adjustedDive = _diveWanted - diveByRotor;
        }

        _cyclicForwardWanted =- (adjustedDive-dive)*16.0f*cFactor;
        Limit(_cyclicForwardWanted,-1,+1);
      }
      else
      {
        _cyclicAsideWanted=0;
        _cyclicForwardWanted=0;
        _backRotorWanted=0;
        _rotorDiveWanted = 0;
      }
    }
    else
    {
      // no controls available - no engine power
      _cyclicAsideWanted=0;
      _cyclicForwardWanted=0;
      _backRotorWanted=0;
      _mainRotorWanted=0.3;
      _rotorDiveWanted = 0;
    }

    if( _angVelocity.SquareSize()>=10*10 )
    { // if we are rotating fast, leave all controls in neutral position
      _cyclicAsideWanted=0;
      _cyclicForwardWanted=0;
      _backRotorWanted=0;
      _mainRotorWanted=0.3;
      _rotorDiveWanted = 0;
    }
  }

  // perform advanced simulation
  MoveWeapons(deltaT);
  base::Simulate(deltaT,prec);

  // set simulation precision for a next step
  if(GetStopped())
    // when suspended, no need for frequent simulation
    SetSimulationPrecision(HeliSimStepInactive,HeliSimStepInactiveAI);
  else if (QIsManual() || _state>=AutopilotAlign)
    // sometimes we want extra stability, this is always true for player, and sometimes for AI as well (landing, precise maneuvring)
    SetSimulationPrecision(HeliSimStepPriority,HeliSimStepPriorityAI);
  else
    SetSimulationPrecision(HeliSimStepActive,HeliSimStepActiveAI);

}

bool HelicopterAuto::AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction)
{
  WeaponModePtr selected = context._weapons->FindValidWeapon(ptr);
  if (!selected.IsValid()) return false;

  context._weapons->SelectWeapon(this, selected);
  // move turret/gun accordingly to direction
  Vector3 relDir(VMultiply, context._parentTrans.InverseRotation(), direction);
  // calculate current gun direction
  // compensate for neutral gun position

  if (context._turret && context._turret->Aim(*context._turretVisualState, *context._turretType, relDir, context._parentTrans))
    CancelStop();
  return true;
}

bool HelicopterAuto::CalculateAimWeaponPos(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &pos, float &timeToLead, Target *target, bool exact) const
{
  WeaponModePtr selected = context._weapons->FindValidWeapon(ptr);
  if (!selected.IsValid()) return false;

  if ( target->idExact.IsNull())
    return false;
  if (!CheckTargetKnown(context,target))
    return false;

  Vector3 tgtPos, tgtSpd;
  if (exact)
  {
    tgtPos = target->idExact->AimingPosition(target->idExact->FutureVisualState());
    tgtSpd = target->idExact->FutureVisualState().Speed();
  }
  else
  {
    AIBrain *gunnerBrain = context._gunner ? context._gunner->Brain() : NULL;
    tgtPos = target->LandAimingPosition(gunnerBrain);
    tgtSpd = target->GetSpeed(gunnerBrain);
  }

  const Magazine *magazine = ptr.GetMagazine();
  const MagazineType *aInfo = magazine ? magazine->Type() : NULL;
  const AmmoType *ammo = aInfo ? aInfo->_ammo : NULL;
  float dist = tgtPos.Distance(FutureVisualState().Position());
  float time = aInfo ? dist * aInfo->_invInitSpeed : 0.0f;
  float fall = 0.0f;
  bool rockets = false;
  if (ammo)
  {
    switch (ammo->_simulation)
    {
    case AmmoShotMissile: case AmmoShotRocket:
      if (ammo->maxControlRange<10)
      {
        Vector3 pos = FutureVisualState().PositionModelToWorld((Type()->_missileLPos + Type()->_missileRPos) / 2.0f);
        time = PreviewRocketBallistics(tgtPos, FutureVisualState().PositionModelToWorld(Type()->_missileLPos), aInfo, ammo, fall);
        rockets = true;
        if (time>=FLT_MAX) return false;
      }
      else
      {
        time = 0.0f;
        tgtSpd = VZero;
      }
      break;
    default:
      time = PreviewBallistics(tgtPos, FutureVisualState().Position(), aInfo, ammo, fall);
      if (time>=FLT_MAX) return false;
      break;
  }

  }
  
  timeToLead = time;
  
  tgtPos[1] += fall; // consider ballistics
  if (aInfo)
  {
    // for rockets this speed correction aprox. is not working properly
    Vector3 speedEst = rockets? tgtSpd : tgtSpd - FutureVisualState().Speed();
    float maxSpeedEst = aInfo->_maxLeadSpeed;
    if (speedEst.SquareSize() > Square(maxSpeedEst))
      speedEst = speedEst.Normalized() * maxSpeedEst;
    float predTime = floatMax(time + 0.1f, 0.25f);
    tgtPos += speedEst * predTime;
  }

  pos = tgtPos;
  return true;
}

bool HelicopterAuto::CalculateAimWeapon(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &dir, float &timeToLead, Target *target, bool exact ) const
{
  Vector3 tgtPos;
  bool ret = CalculateAimWeaponPos(context, ptr,tgtPos,timeToLead,target,exact);
  if (ret)
  {
    const TurretType* turretType = context._turretType;
    Vector3 weaponPos = VZero;

    // hotfix: news:hs16tn$u2q$1@new-server.localdomain
    bool gunPosSet = false;

    if (turretType)
    {
      weaponPos = turretType->GetTurretAimCenter(Type());
      gunPosSet = true;
    }
    else if (context._turret) 
    {
      weaponPos = Type()->GetGunPos(context._turret->_gunPosIndexToogle);
      gunPosSet = true;
    }

    // try to set first position from turretType positionArray
    if (turretType && !gunPosSet) 
    {
      int PosArrSize = turretType->_gunPosArray.Size();
      
      if (PosArrSize>0)
        weaponPos = Type()->GetGunPos(0);
    }
    
    Vector3 myPos=FutureVisualState().PositionModelToWorld(weaponPos);
    dir = tgtPos-myPos;
  }
  return ret;
}

bool HelicopterAuto::AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target)
{
  Vector3 dir;
  float timeToLead;
  if (!CalculateAimWeapon(context, ptr,dir,timeToLead,target,false)) return false;

  return AimWeapon(context, ptr, dir);
}

Vector3 HelicopterAuto::GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  if (!context._turretType)
    return vs.Direction();

  const AmmoType *ammo = ptr.GetAmmoType();
  if (ammo)
  {
    if (ammo->_simulation == AmmoShotMissile || ammo->_simulation == AmmoShotRocket)
      return (ammo->maxControlRange < 10) ? vs.Direction() : vs.Transform().Rotate(context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType)));
  }

  return vs.Transform().Rotate(context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType)));
}

Vector3 HelicopterAuto::GetWeaponManualControlDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  if (!context._turretType)
    return vs.Direction();
  return vs.Transform().Rotate(context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType)));
}

Vector3 HelicopterAuto::GetWeaponManualControlPoint(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  if (!context._turretType)
    return vs.Direction();
  return context._turretType->GetTurretPos(GunTurretTransform(vs, *context._turretType));
}

Vector3 HelicopterAuto::GetWeaponDirectionWanted(const TurretContextEx &context, const WeaponModePtr &ptr) const
{
  if (!context._turret)
    return FutureVisualState().Direction();
  if (!context._turretType)
    return FutureVisualState().Direction();

  const AmmoType *ammo = ptr.GetAmmoType();
  if (ammo)
  {
    if (ammo->_simulation == AmmoShotMissile)
      return FutureVisualState().Direction();
    else if (ammo->_simulation == AmmoShotRocket)
      return FutureVisualState().Direction();
  }

  Matrix3Val aim = context._turret->GetAimWantedWorld(context._parentTrans);
  Vector3 dir = context._turretType->GetTurretDir(aim);
  return dir;
}

Vector3 HelicopterAuto::GetWeaponPoint(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  if (!ptr.IsValid())
    return VZero;
  //const Magazine *magazine = GetMagazineSlot(weapon)._magazine;
  //if (!magazine) return VZero;
  const AmmoType *ammo = ptr.GetAmmoType();
  if (!ammo)
    return VZero;

#if _VBS3
  // return the turret rotation, when laser target is on.
  if(context._weapons->_laserTargetOn)
  {
    if (!context._turretType) return VZero;
    return context._turretType->GetTurretPos(GunTurretTransform(vs, *context._turretType));
  }
#endif

  switch (ammo->_simulation )
  {
    case AmmoShotRocket:
      return !_rocketLRToggle ? Type()->_rocketLPos : Type()->_rocketRPos;
    case AmmoShotMissile:
    {
      int count = GetLastMissileIndex(context, ptr);
      bool found;
      Vector3 pos = FindMissilePos(count,found, ammo->maverickweaponIndexOffset).Position();
      if (!found)
        pos = !_missileLRToggle ? Type()->_missileLPos : Type()->_missileRPos;
      return pos;
    }
    case AmmoShotBullet:
    case AmmoShotSpread:
      if (!context._turretType)
        return VZero;
      return context._turretType->GetTurretPos(GunTurretTransform(vs, *context._turretType));
    case AmmoShotShell:
      if (!context._turretType)
        return VZero;
      return context._turretType->GetTurretPos(GunTurretTransform(vs, *context._turretType));
  }
  return VZero;
}

Vector3 HelicopterAuto::GetWeaponCenter(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  if (!context._turret)
    return VZero;
  const AmmoType *ammo = ptr.GetAmmoType();
  if (ammo)
  {
    if (ammo->_simulation == AmmoShotMissile || ammo->_simulation == AmmoShotRocket)
      return base::GetWeaponCenter(vs, context, ptr);
  }
  return context._turret->GetCenter(Type(), *context._turretType);
}

float HelicopterAuto::GetAimed(const TurretContext &context, const WeaponModePtr &ptr, Target *target, bool exact, bool checkLockDelay) const
{
  if( !target )
    return 0;
  if( !target->idExact )
    return 0;
  // check if weapon is aimed
  if (!ptr.IsValid())
    return 0;

  const Magazine *magazine = ptr.GetMagazine();
  if (!magazine || !magazine->Type())
    return 0;
  const MagazineType *aInfo = magazine->Type();
  if (!aInfo)
    return 0;
  const AmmoType *ammo = aInfo->_ammo;
  if (!ammo)
    return 0;
  if (!CheckTargetKnown(context,target))
    return 0;
  if (!context._gunner)
    return 0;

  Vector3 speed;
  Vector3 ap;
  float seenBefore;
  float visible = GetAimingPosition(ap, speed, seenBefore, exact, context, target, ammo);
  
  float lock = 1;
  if (ammo->_simulation == AmmoShotMissile)
  {
    if (ammo->thrustTime>0 && ammo->maxControlRange>10)
    {
      // guided missile 
      Vector3 relPos=PositionWorldToModel(ap);
      // check if target is in front of us
      if( relPos.Z()<=50 ) return 0; // missile fire impossible
      // check if target position is withing missile lock cone
      if( fabs(relPos.X())>relPos.Z() ) return 0;
      if( fabs(relPos.Y())>relPos.Z() ) return 0;
      // the nearer we are, the more precise lock required
      float invRZ=1.0/relPos.Z();
      float lockX=1-fabs(relPos.X())*invRZ;
      float lockY=1-fabs(relPos.Y())*invRZ;
      lock=floatMin(lockX,lockY);
      saturate(lock,0,1);
      // we can fire
#if _ENABLE_CHEATS
      if (CHECK_DIAG(DECombat) && this==GWorld->CameraOn() )
        DIAG_MESSAGE(100,Format("Lock precision %.3f"));
#endif
      if( lock<0.5f )
        lock=0;

    }
    else if (ammo->maxControlRange>10)
    {
      // free fall bomb
      // predict where the bomb will fall
      // assume it is guided - larger error is acceptable

      Vector3 wPos = FutureVisualState().PositionModelToWorld(GetWeaponCenter(FutureVisualState(), context, ptr));
      Vector3 wDir = GetWeaponDirection(FutureVisualState(), context, ptr);

      // CCIP calculation
      // assume air friction decreased acceleration
      // weapon direction is not important here
      float a = G_CONST*0.9f;

      float dist = ap.Distance(wPos);

      // calculate time to impact at the target
      float aboveSurface = FutureVisualState().Position().Y()-ap.Y();

      // d = v*t+0.5*a*t*t
      // special case: v=0: d = 0.5*a*t*t
      // d = 1/2*a*t*t
      // sqrt(d*2/a) = t

      float t = sqrt(2*aboveSurface/a);

      // assume minor no horizontal deceleration
      Vector3 hit=wPos + FutureVisualState()._speed*0.98f*t - Vector3(0,a*0.5f,0)*t*t;
      Vector3 targetSpeed = exact ? target->idExact->FutureVisualState().Speed() : target->GetSpeed(context._gunner->Brain());
      Vector3 estPos = ap+targetSpeed*t;

      Vector3 hError=hit-estPos;
#if _ENABLE_CHEATS
      if (CHECK_DIAG(DECombat) /*&& this==GWorld->CameraOn()*/)
      {
        DIAG_MESSAGE_ID(
          100,ID().Encode(),"Time to impact %.2f, error %.1f (%.1f,%.1f,%.1f)",
          t,hError.Size(),hError[0],hError[1],hError[2]
        );
      }
#endif
      /**/

      hError[1] *= 0.3f;

      float error = hError.Size();
      float aimPrecision=(GetInvAbility(AKAimingAccuracy)-1)*1.5f+1;

      saturateMax(aimPrecision,1);

      float tgtSize=target->idExact->AimingSize()*aimPrecision;
      tgtSize += dist*aimPrecision*0.02f+ammo->indirectHitRange;

      // assume Laser guiding can correct a lot
      tgtSize += t*10;

      if( error<=tgtSize ) return visible;
      if( error>=tgtSize*2 ) return 0;
      return ( 2*tgtSize-error )/tgtSize*visible;
    }
    else
    {
      return CheckAimed(ap, speed, context, ptr, target, exact, ammo) *visible;
    }
  }
  else
  {
    if (ammo->_simulation == AmmoShotBullet || ammo->_simulation == AmmoShotSpread || ammo->_simulation == AmmoShotShell)
    {
      lock = CheckAimed(ap, speed, context, ptr, target, exact, ammo);
    }
    else
    {
      // predict shot result
      float dist=ap.Distance(FutureVisualState().Position());
      float time=aInfo ? dist*aInfo->_invInitSpeed : 0;
      Vector3 targetSpeed = exact ? target->idExact->FutureVisualState().Speed() : target->GetSpeed(context._gunner->Brain());
      Vector3 leadSpeed = targetSpeed-FutureVisualState().Speed();
      Vector3 estPos = ap+leadSpeed*time;
      Vector3 wDir=GetWeaponDirection(FutureVisualState(), context, ptr);
      Vector3 wPos=FutureVisualState().PositionModelToWorld(GetWeaponCenter(FutureVisualState(), context, ptr));
      float eDist=wPos.Distance(estPos);
      Vector3 hit=wPos+wDir*eDist;
      hit[1]-=G_CONST*time*time*0.5f;
      Vector3 hError=hit-estPos;
#if _ENABLE_CHEATS
      float distHit = hit.Distance(wPos);
#endif

      hError[1]*=2;
      float error=hError.Size()*0.5f;

      float tgtSize=target->idExact->AimingSize();
      float maxError = tgtSize+ammo->indirectHitRange*0.3f;
      maxError+=dist*ptr.SumDispersionModifiers(context._gunner);

      if (ammo->_simulation != AmmoShotBullet && ammo->_simulation != AmmoShotSpread )
        maxError *=2;

      if (leadSpeed.SquareSize()>Square(10)) maxError *= 2;

#if _ENABLE_CHEATS
      if (CHECK_DIAG(DECombat) && this==GWorld->CameraOn())
        DIAG_MESSAGE(2000,Format("Error %.1f, tgtSize %.1f, time %.2f, ePos %.1f,%.1f,%.1f, distErr %.1f",
        error,tgtSize,time, hError[0],hError[1],hError[2], distHit-eDist));
#endif

      lock = error < maxError;
    }
  }
  if (lock<0.1f)
    return 0;
  if (exact)
    return lock;

  // 0.6 visibility means 0.8 unaimed
  visible=1-(1-visible)*0.5f;
  return lock*visible;
}

// manual control
void HelicopterAuto::DamageCrew( EntityAI *killer, float howMuch, RString ammo)
{
  AIBrain *commander = CommanderUnit();
  if (commander)
  {
    if (GetRawTotalDamage()>=0.7f && commander->GetCombatModeLowLevel()>=CMCombat)
    {
      Time goTime = Glob.time + GRandGen.PlusMinus(2.0f,1.0f);
      if (goTime<_getOutAfterDamage)
        _getOutAfterDamage = goTime;
    }
  }
  base::DamageCrew(killer,howMuch,ammo);
}

void HelicopterAuto::Eject(AIBrain *unit, Vector3Val diff)
{
  // check height
  float surfaceY=GLOB_LAND->SurfaceYAboveWaterNoWaves(FutureVisualState().Position()[0],FutureVisualState().Position()[2]);
  float height = FutureVisualState().Position().Y() - surfaceY;
  bool parachute = (height>30);
  unit->ProcessGetOut(true, parachute, diff);
}

void HelicopterAuto::FakePilot(float deltaT)
{
  _forceDive=1;
}

void HelicopterAuto::AutohoverOn()
{
  //if (_hoveringAutopilot) return;
  _hoveringAutopilot = true;
  //_pilotHeading = atan2(Direction()[0],Direction()[2]);
}

void HelicopterAuto::AutohoverOff()
{
  //if (_hoveringAutopilot) return;
  _hoveringAutopilot = false;
  //_pilotHeading = atan2(Direction()[0],Direction()[2]);
}


void HelicopterAuto::SuspendedPilot(AIBrain *unit, float deltaT )
{
#if _VBS3 //with simplified flight model, don't change the values
  if(Glob.config.IsEnabled(DTSimplifiedHeliModel))
    return;
#endif

  float surfaceY=GLOB_LAND->SurfaceYAboveWaterNoWaves(FutureVisualState().Position()[0],FutureVisualState().Position()[2]);
  _pilotHeight=FutureVisualState().Position().Y()-surfaceY;
  _pilotSpeed=VZero;
}


void HelicopterAuto::SwitchPilotHeightAGL(bool agl)
{
  if (_pilotHeightAGL==agl)
    return;
  float surfaceY = GLandscape->SurfaceYAboveWaterNoWaves(FutureVisualState().Position()[0],FutureVisualState().Position()[2]);
  if (agl)
    // switch from ASL to AGL
    _pilotHeight -= surfaceY;
  else
    // switch from AGL to ASL
    _pilotHeight += surfaceY;
  _pilotHeightAGL = agl; // manual controls mean no automatic terrain following
}

void HelicopterAuto::KeyboardPilot(AIBrain *unit, float deltaT )
{ 
  if (!unit->GetPerson() || unit->GetPerson()->KeyboardUsedInFrame()) return;
  Vector3Val position=FutureVisualState().Position();
  float surfaceY=GLOB_LAND->RoadSurfaceYAboveWater(position);

  float curHeightAGL=position.Y()-surfaceY;

  float curBottomHeight = curHeightAGL + _shape->GeometryLevel()->Min().Y();
  
  _dirCompensate=0; // low heading compensation
  _forceDive=10;
  _forceBank=10;

  // automatic gear control
  if (curBottomHeight>30 || curBottomHeight>20 && FutureVisualState()._speed.SquareSizeXZ()>Square(25))
    SetPilotGear(false);
  if (FutureVisualState()._speed.SquareSizeXZ()<Square(23) || (FutureVisualState()._rotorSpeed < 0.9)) // (_rotorSpeed < 0.9)) - when main rotor failure open gear earlier
  {
    if (curBottomHeight<5 || curBottomHeight<25 && (FutureVisualState()._speed.SquareSizeXZ()<Square(8) || (FutureVisualState()._rotorSpeed < 0.9)))
      SetPilotGear(true);
  }

  //Helicopter thrust pilot
#if _VBS3
    // enable a simplified Control mode, where the speed and altitude are automatically set
    // following the contour is a seperate difficulty setting
  // enable a simplified Control mode, where the speed and altitude are automatically set
  // following the contour is a separate difficulty setting
    // enable a simplified Control mode, where the speed and altitude are automatically set
    // following the contour is a seperate difficulty setting

    SwitchPilotHeightAGL(Glob.config.IsEnabled(DTFollowContour));

    if(Glob.config.IsEnabled(DTSimplifiedHeliModel))
    {
      _pilotHeightHelper = true; // keyboard helper activated
      _pilotVSpeedHelper = true; // keyboard helper activated
//      _pilotSpeedHelper = true; // keyboard helper activated
//      _pilotDirHelper = true;

      if(FutureVisualState()._rotorSpeed > 0.5)
      {
        if(_pilotHeightAGL)
          _defPilotHeight += (GInput.GetAction(UAHeliCollectiveRaise)-GInput.GetAction(UAHeliCollectiveLower)) * .3;
        else
          _pilotHeight += (GInput.GetAction(UAHeliCollectiveRaise)-GInput.GetAction(UAHeliCollectiveLower)) * .3;
      }

      if(_pilotHeightAGL) _pilotHeight=_defPilotHeight;

      //prevent to go under ground
      float surfaceY = GLandscape->SurfaceYAboveWater(Position()[0],Position()[2]);
      if(!_pilotHeightAGL) _pilotHeight -= surfaceY;

      if(_pilotSpeed[2] > 11) // don't allow to go below 11m unless speed is below 11m/s
        saturateMax(_pilotHeight, 11);
      else
        saturateMax(_pilotHeight, -.2); //don't go under ground

      if(!_pilotHeightAGL) _pilotHeight += surfaceY;
    }
    else
    {
    _pilotHeightHelper = false; // keyboard helper activated
    _pilotVSpeedHelper = true; // keyboard helper activated
    }
    
#else
    _pilotHeightHelper = false; // keyboard helper activated
    _pilotVSpeedHelper = true; // keyboard helper activated
    SwitchPilotHeightAGL(false);
#endif

  // when using analogue throttle, do not use thrust autopilot
  // switch between analogue / autopilot based on last input
  
  float forward= GInput.GetAction(UAHeliCollectiveRaise)-GInput.GetAction(UAHeliCollectiveLower);

  float throttleDirect = GInput.GetAction(UAHeliCollectiveRaiseCont)-GInput.GetAction(UAHeliCollectiveLowerCont);
  
#if _HELISIM_ROTORLIB
  if(!IsRTD())
  {
#endif
    if (fabs(forward-_lastForward)>0.1f)
    {
      if (_lastForward<FLT_MAX) _thrustHelperTime = Glob.time; // ignore initial settings
      _lastForward = forward;
    }
    if (fabs(throttleDirect-_lastThrotleDirect)>0.1f)
    {
      if (_lastThrotleDirect<FLT_MAX) _analogueThrottleTime = Glob.time; // ignore initial settings
      _lastThrotleDirect = throttleDirect;
    }
#if _HELISIM_ROTORLIB
  }
#endif

  // reset the older input - input methods are not compatible, only one must be used at any given time
  _pilotVSpeedHelper = _analogueThrottleTime<=_thrustHelperTime;

  // Q (collective raise) starts the engine. EngineOn() latches
  // _rotorSpeedWanted = 1, so after the key is released the engine keeps
  // running; holding Q only controls the climb and Z controls the descent.
  if (forward>0 || throttleDirect>0)
  {
#if _HELISIM_ROTORLIB
    if (!IsRTD() && !EngineIsOn()) EngineOn();
#else
    if (!EngineIsOn()) EngineOn();
#endif
  }
  // Keep transport engine state (sound/HUD/SQF isEngineOn) in sync with the
  // latched rotor. Only an explicit engine-off action clears the latch.
  if (EngineIsOn() && _engineOff) Transport::EngineOn();


  if (!_pilotVSpeedHelper)
    {
    _mainRotorWanted = throttleDirect;
    }
  else
  {
    throttleDirect = 0;
#if _VBS3 //increase 
    _pilotSpeed[1]=forward*20.0f;
#else
    _pilotSpeed[1]=forward*10.0f;
#endif
  }
  

  _pilotSpeed[0]=0; // no side slip

  //Helicopter direction pilot
  {
    // TODO: when speed is low and controls was not touched for a long time
    // activate hovering autopilot
    {

      float turn = 0;
      float backRotor = 0;

      //if (!GWorld->HasMap())
      {
        // aim and turn with aiming actions
        float aimX(0), aimY(0);
        if (unit->LookAroundEnabled())
        {
          aimX = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UAAimRight, UAAimLeft);
          aimY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UAAimUp, UAAimDown);
        }
        if(!_copilot)//is looking around in elsewhere
          LookAround(aimX, aimY, deltaT, &Type()->_viewPilot, unit);

        // turn keys control bank
        backRotor = GInput.GetAction(UAHeliRudderLeft)-GInput.GetAction(UAHeliRudderRight);
        //joystick was too sensitive - cyclicLRcoef fixes it
        //but keyboard sensitivity affected too: GetAction third parameter fixes it
        const float cyclicLRcoef = 0.7;
        static float cyclicKeyboard = 1.0f;
        turn = (GInput.GetAction(UAHeliCyclicLeft,true,cyclicKeyboard)-GInput.GetAction(UAHeliCyclicRight,true,cyclicKeyboard))*Input::MouseRangeFactor*cyclicLRcoef;
        float mouseLR = (GInput.GetAction(UAHeliLeft)-GInput.GetAction(UAHeliRight))*Input::MouseRangeFactor;
        static float coefLR = 1.0f;
#if _ENABLE_CHEATS
        if (CHECK_DIAG(DEMouseSensitivity))
        {
          if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPADPLUS))
            coefLR -= 0.1;
          else if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPADMINUS))
            coefLR += 0.1;
          saturate(coefLR,0.1, 50);
          DIAG_MESSAGE(500, Format("Helicopter: mouseSensitivity = %.1f", coefLR));
        }
#endif
        mouseLR *= coefLR;
        float fastFactor = floatMax(0,fabs(FutureVisualState().ModelSpeed().Z())-4)*FAST_COEF;
        saturate(fastFactor,0,1);
        // if we are already banked, allow banking as well
        // otherwise we cannot bank back
        // allow banking back only using mouse
        static float bankBackFactor = 6.0f;
        float bank = floatMin(FutureVisualState().DirectionAside().Y()*bankBackFactor,1);
        if (bank*mouseLR>0) bank = 0;
        float bankFactor = floatMax(fabs(bank),fastFactor);
        backRotor += mouseLR*(1-bankFactor);
        turn += mouseLR*bankFactor;
        forward = GInput.GetAction(UAHeliCyclicForward)*Input::MouseRangeFactor + GInput.GetActionHeliFastForward()*2 - GInput.GetAction(UAHeliCyclicBack)*Input::MouseRangeFactor;
      }

      // always turn using back rotor
      _backRotorWanted = backRotor;
      saturate(_backRotorWanted,-1,+1);

      if (!_hoveringAutopilot)
      {
        bool realistic = true;
#if _VBS3 //simplified helicopter flight model (set speed and altitude)
        if(Glob.config.IsEnabled(DTSimplifiedHeliModel))
        {
          _pilotSpeedHelper = true; // keyboard helper activated
          _pilotSpeed[2] += forward * 0.1f; 
//          if(_defPilotHeight < 10) // reduce speed close to the ground
//            saturateMin(_pilotSpeed[2], ceil(_defPilotHeight));

          realistic = false;
        }
        else
#endif
        _pilotSpeedHelper = false; // keyboard helper activated

        _pilotDirHelper = false;

        if (realistic)
        {
          static float turnCyclicFactor=1.0f;
          static float diveCyclicFactor=1.0f;
          _pilotCyclicDirect = true;
          _cyclicAsideWanted = turn*turnCyclicFactor;
          _cyclicForwardWanted = forward*diveCyclicFactor;
          //
        }
        else
        {
          _pilotCyclicDirect = false;
          static float turnFactor=1.0f;
          static float diveFactor=1.0f;
          // avoid too fast bank controls

          float massCoef=GetMass()*(1.0/3000);
          saturate(massCoef,1,3);
          float dirEstT = massCoef*0.3f;
          static float minEstT = 0.1f;
          static float maxEstT = 0.5f;
          saturate(dirEstT,minEstT,maxEstT);
          
          const Matrix3 &orientation=FutureVisualState().Orientation();
          Matrix3Val derOrientation=_angVelocity.Tilda()*orientation;
          Matrix3Val estOrientation=orientation+derOrientation*dirEstT;
          
          // remember bank, only adjust it slightly
          float dive = FutureVisualState().Direction().Y() - FutureVisualState()._rotorDive;
          float bank = BankFromOrientation(FutureVisualState().Orientation());
          float estBank = BankFromOrientation(estOrientation);
          float estDive = estOrientation.Direction().Y() - FutureVisualState()._rotorDive;
          (void)dive,(void)estBank;
          
          if (fabs(turn)>0.1f)
          {
            _bankWanted = bank+turn*turnFactor;
            _pilotBankSet = false;
          }
          else if (!_pilotBankSet)
          {
            _pilotBankSet = true;
            _bankWanted = bank;
          }
          saturate(_bankWanted,-MaxBank,+MaxBank);
          
#if _VBS3
          if(!_pilotSpeedHelper)
          {
#endif
          if (fabs(forward)>0.1f)
          {
            _pilotDive = estDive-forward*diveFactor;
            _pilotDiveSet = false;
          }
          else if (!_pilotDiveSet)
          {
            _pilotDiveSet = true;
            _pilotDive = estDive;
          }
          saturate(_pilotDive,-0.7f,+0.7f);

          _diveWanted = _pilotDive;
        }
#if _VBS3
        }
#endif
      }
      else
      {
        _pilotDirHelper = false;

        // control hovering with keyboard
        _pilotSpeedHelper = true; // keyboard helper activated
        _pilotCyclicDirect = false;
        
        // strafe
        _pilotSpeed[0] = (GInput.GetAction(UAHeliCyclicRight)-GInput.GetAction(UAHeliCyclicLeft))*6*Input::MouseRangeFactor;
        float mouseLR = (GInput.GetAction(UAHeliLeft)-GInput.GetAction(UAHeliRight))*Input::MouseRangeFactor;
        static float coefLR = 6.0f;
#if _ENABLE_CHEATS
        if (CHECK_DIAG(DEMouseSensitivity))
        {
          if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPADPLUS))
            coefLR -= 0.1;
          else if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPADMINUS))
            coefLR += 0.1;
          saturate(coefLR,0.1, 50);
          DIAG_MESSAGE(500, Format("HeliAutoHovering: mouseSensitivity = %.1f", coefLR));
        }
#endif
        _pilotSpeed[0] += coefLR*mouseLR;
        static float SpeedFactor=1.0f; //1.0f //TODO - check it
        _pilotSpeed[0] *= SpeedFactor;
        _pilotSpeed[2] = forward*3.5f;
        _pilotDive = 0;
        _bankWanted = 0;
        
      }
    }
    Limit(_pilotSpeed[2],-10,+Type()->GetMaxSpeedMs());
  }
}

void HelicopterAuto::AvoidGround(float minHeight)
{
  Point3 estimate=FutureVisualState().Position();
  float maxUnder=0;
  for (int i=0; i<2; i++)
  {
    float estY=GLOB_LAND->SurfaceYAboveWaterNoWaves(estimate.X(),estimate.Z());
    estY+=minHeight;
    float estUnder=estY-estimate.Y();
    saturateMax(maxUnder,estUnder);
    estimate+=FutureVisualState()._speed*1.5f;
  }
  static float limitSpeedUnder = 5.0f;
  if (maxUnder>limitSpeedUnder)
  {
    float maxSpeed=Interpolativ(maxUnder,limitSpeedUnder,10,Type()->GetMaxSpeedMs(),0);
    Limit(_pilotSpeed[2],-maxSpeed,maxSpeed);
  }
}

void HelicopterAuto::AvoidCollision()
{
  if (!Airborne())
    return;
  AIBrain *agent = PilotUnit();
  if( !agent )
    return;

  if (agent->GetPlanningMode() == AIBrain::LeaderDirect)
    return;
  // if we are stopped, do not try to avoid
  if( FutureVisualState()._rotorSpeed<=0.1 )
    return;

  // avoid collisions
  float mySize=floatMax(15,CollisionSize()*2); // assume vehicle is not round
  //float mySpeedSize=fabs(ModelSpeed()[2]);
  float gap = mySize*4;
  //float frontGap = mySize*8;
  // check if we are on collision course
  VehicleCollisionBuffer ret;

  const float maxSpeed = GetType()->GetMaxSpeedMs();
  const float mySpeed = FutureVisualState().ModelSpeed().Z();
  const float maxTime = 4;
  const float maxDist = floatMax(mySize*4,floatMax(mySpeed,maxSpeed*0.2)*maxTime);
  GLOB_LAND->PredictCollision(ret,this,maxTime,gap,maxDist);
  if( ret.Size()<=0 ) return;

  // some collision predicted

  //AIGroup *myGroup = unit->GetGroup();

  // precalculate
  //float invMaxSpeed=1/maxSpeed;

  //Vector3 mySpeed=Speed();

  for( int i=0; i<ret.Size(); i++ )
  {
    const VehicleCollision &info=ret[i];
    const EntityAI *who=info.who;

    if (!who->Airborne()) continue;
    // something is near
    // some vehicle
    // determine who should slow down
    // if who is in front of us, slow down to his speed
    // if we are heave and he is enemy soldier, ignore him

#if _ENABLE_CHEATS
    if( CHECK_DIAG(DECombat) )
    {
      Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),VISITOR_NO_ID);
      obj->SetPosition(info.pos);
      Color color(1,1,0,0.3);
      //color=Color(1,-1,0)*danger+Color(0,1,0);
      obj->SetScale(info.distance<gap ? 1 : 0.3);
      obj->SetConstantColor(PackedColor(color));
      GScene->ShowObject(obj);
    }
#endif


#if DIAG_COL
    //if( this==GWorld->CameraOn() )
    {
      LogF("%s vs %s",(const char *)GetDebugName(),(const char *)who->GetDebugName());
    }
#endif

    Vector3 relPos = PositionWorldToModel(who->FutureVisualState().Position());

    bool iAmInFrontOfHim=who->PositionWorldToModel(FutureVisualState().Position()).Z()>0;
    bool heIsInFrontOfMe=relPos.Z()>0;


    if (heIsInFrontOfMe && iAmInFrontOfHim)
    {
      // who is higher should climb
      float heIsHigher = who->FutureVisualState().Position().Y()-FutureVisualState().Position().Y();
      if (heIsHigher<0)
      {
        float surfaceY = GLandscape->SurfaceYAboveWaterNoWaves(FutureVisualState().Position().X(),FutureVisualState().Position().Z());
        saturateMax(_pilotAvoidHighHeight,who->FutureVisualState().Position().Y()-surfaceY+mySize);
        Time until = Glob.time+10;
        if (_pilotAvoidHigh<until)
          _pilotAvoidHigh=until;

#if DIAG_COL
        //if( this==GWorld->CameraOn() )
        {
          LogF("  avoid hi %.2f",_pilotAvoidHighHeight);
        }
#endif
      }
      else if (heIsHigher>0)
      {
        float surfaceY = GLandscape->SurfaceYAboveWaterNoWaves(FutureVisualState().Position().X(),FutureVisualState().Position().Z());
        saturateMin(_pilotAvoidLowHeight,who->FutureVisualState().Position().Y()-surfaceY-mySize);
        Time until = Glob.time+10;
        if (_pilotAvoidLow<until) _pilotAvoidLow=until;

#if DIAG_COL
        //if( this==GWorld->CameraOn() )
        {
          LogF("  avoid lo %.2f",_pilotAvoidLowHeight);
        }
#endif
      }
    }

    if( info.distance>gap )
      continue;

    if (info.distance<mySize)
    {
      // collision imminent - who is back needs to brake
      // note: this will make the heli climbing at the same time
      // check if he is in front of me
      if (heIsInFrontOfMe)
      {

        {
          Vector3Val hisRelSpeed = DirectionWorldToRelative(who->FutureVisualState().Speed());
          // slow down, but it has no sense to slow down much more than his relative speed
          // - we still may want to be a little slower to increase the gap
          _pilotSpeed = Vector3(0,0,floatMax(0,hisRelSpeed.Z()-2.0f));
        }
      }
    }
  } // for(i)
}

// AI autopilot

const float MaxBrakeSpeed = 20;

void HelicopterAuto::BrakingManeuver()
{
  // before we start braking, we need to slow down so that we can turn by back rotor
  // maintain forward dive
  _pilotHeading = atan2(-FutureVisualState().Speed().X(),-FutureVisualState().Speed().Z());
  float curHeading = atan2(FutureVisualState().Direction().X(),FutureVisualState().Direction().Z());
  float headChange = AngleDifference(_pilotHeading,curHeading);

  // once chopper is turned is desired direction, it may start braking
  float brakeDive = floatMax(0,1-headChange*(0.5f/H_PI));
  _forceDive = brakeDive;
  saturate(_forceDive,-0.6f,-0.1f);
}

Vector3 HelicopterAuto::GetStopPosition() const
{
  // FIX: return _stopPosition only when valid
  if (_stopMode == SMNone)
    return base::GetStopPosition();
  return _stopPosition;
}

void HelicopterAuto::Autopilot(float deltaT, Vector3Par targetCur, Vector3Par targetEst, Vector3Par tgtSpeed, Vector3Par direction)
{
  float maxSpeed = Type()->GetMaxSpeedMs();
  // point we would like to reach
  float avoidGround=0;
  Vector3Val position=FutureVisualState().Position();
  Vector3 absDistance=targetCur-position;
  Vector3 distance=DirectionWorldToRelative(absDistance);

  float bank=BankFromOrientation(FutureVisualState().Orientation());
  float dive=FutureVisualState().Direction().Y();

  float sizeXZ2=distance.SquareSizeXZ();
  switch( _state )
  {
    default: //case AutopilotFar:
    {
      Vector3 relTgtPos = DirectionWorldToRelative(absDistance);
      float timeToReach = absDistance.SizeXZ()/maxSpeed;
      
      // "lead target" - est. target position - based on estimated time to reach, limited to max 30 seconds
      Vector3 absDirection = absDistance + floatMin(30, timeToReach)*tgtSpeed;
      _pilotHeading=atan2(absDirection.X(),absDirection.Z());
      avoidGround=30;
      _pilotHeight=avoidGround;
      Vector3 relSpeed=DirectionWorldToRelative(FutureVisualState()._speed-tgtSpeed);
      // use special maneuvre for fast braking
      // before using it we need to slow down, though
      #define BRAKE_SEC 3.0
      if
      (
        tgtSpeed.SquareSize()<Square(10) // target is slow
        && (FutureVisualState()._speed.SquareSize()-tgtSpeed.SquareSize())>Square(MaxBrakeSpeed*0.5f) // we will need to brake
      )
      {
        if( distance[2]-relSpeed[2]*BRAKE_SEC<0 && sizeXZ2<Square(450) && FutureVisualState().ModelSpeed().Z()<MaxBrakeSpeed)
          // we are approaching the target, start braking
          _state=GetBackRotorMalfunction()<0.6f ? AutopilotBrake : AutopilotNear;
      }
      else if( sizeXZ2<200*200 )
        _state=AutopilotNear;
      
      _pilotSpeed[0]=0; // no side slips
      _pilotSpeed[1]=0; // vertical speed is ignored anyway
      if (tgtSpeed.SquareSize()<Square(5))
      {
        float fast=sqrt(sizeXZ2)*(1.0f/1000);
        Limit(fast,0,0.8f);

        _pilotSpeed[2]=maxSpeed*fast+1;
      }
      else
      {
        static float timeToReach = 10.0f;
        // adjust speed so that you reach target at given time
        _pilotSpeed[2] = relTgtPos.Z()*(1.0/timeToReach);
      }
      // target height
    }
    break;
    case AutopilotBrake:
    {
      Vector3 relSpeed=DirectionWorldToRelative(FutureVisualState()._speed-tgtSpeed);
      // wait until loosing speed
      _pilotSpeed[0]=0; // no side slips
      _pilotSpeed[1]=0; // vertical speed is ignored anyway
      avoidGround=30;
      _pilotHeight=avoidGround;
      BrakingManeuver();
      
      if (sizeXZ2>Square(550))
        _state = AutopilotFar;
      else if (tgtSpeed.SquareSize()>40*40) // target is too fast
        _state=AutopilotNear;
      else if (FutureVisualState().Speed().SquareSize()<Square(maxSpeed*0.25f))
        // if we are moving slow, we may stop braking
        _state = AutopilotNear;
    }
    break;
    case AutopilotNear:
    {
      Vector3 absDirection = targetEst-FutureVisualState().Position(); // "lead target" - est. target position
      
      
      _pilotHeading=atan2(absDirection.X(),absDirection.Z());
      if (tgtSpeed.SquareSize()>Square(1))
      { // to avoid overshooting with moving target, we limit the angle from the target velocity
        float aside = NearestPointInfiniteDistance(targetCur,targetCur+tgtSpeed,position);
        // limit less when far aside from the target line
        float maxAngle = Interpolativ(aside,maxSpeed*0.01f,maxSpeed*0.2f,H_PI*0.05f,H_PI);
        // limit more when target is moving fast
        float maxDiff = Interpolativ(tgtSpeed.SizeXZ(),0,maxSpeed*0.5f,maxAngle,maxAngle*0.25f);
        float tgtHeading = atan2(tgtSpeed.X(),tgtSpeed.Z());
        float headingDiff = AngleDifference(_pilotHeading,tgtHeading);
        #if _ENABLE_CHEATS
          if (this==GWorld->CameraOn() && CHECK_DIAG(DEPath))
          {
            DIAG_MESSAGE(
              500,"Aside %.1f, maxAngle %.1f, maxDiff %.1f",aside,maxAngle*(180/H_PI),maxDiff*(180/H_PI)
            );
          }
        #endif
        saturate(headingDiff,-maxDiff,+maxDiff);
        _pilotHeading = tgtHeading + headingDiff;
      }
      
      float speedSize=fabs(FutureVisualState().ModelSpeed().Z());
      
      float avoidGroundStatic = floatMinMax(_defPilotHeight,8,30);
      avoidGround = floatMax(avoidGroundStatic,speedSize * 0.35f);
      
      
      float targetAbove=targetCur.Y()-GLOB_LAND->SurfaceYAboveWaterNoWaves(position[0],position[2]);
      // autopilot is used to reach some place (e.g. for landing) or to keep in formation
      // when keeping in formation it makes no sense to avoid ground depending on the target height
      if (tgtSpeed.SquareSize()<Square(1))
      {
        saturateMax(avoidGround,  targetAbove);
      }

      _pilotHeight = floatMax(avoidGround,targetAbove);
      _pilotSpeed[0]=0; // no side slips
      _pilotSpeed[1]=0; // vertical speed is ignored anyway
      // slow down near the target
      // select speed so that you will reach target in 10 sec
      if (tgtSpeed.SquareSize()<Square(1))
      {
        // target is static
        // if we are moving very fast, we need to brake
        float fast=sqrt(sizeXZ2)*(1.0/500);
        Limit(fast,0,1);
        float wantedSpeed = maxSpeed*fast*0.5+1;
        float currentSpeed = FutureVisualState().ModelSpeed()[2];
        _pilotSpeed[2] = wantedSpeed;
        if
        (
          sizeXZ2<Square(50) // near enough
          // never assume aligned when target is moving fast
          && FutureVisualState()._speed.Distance2(tgtSpeed)<Square(10) // speed difference is not too high
        )
          _state=AutopilotAlign;
        else if (currentSpeed>wantedSpeed*1.5 && currentSpeed>45)
          // moving too fast - initiate braking maneuver
          _state=GetBackRotorMalfunction()<0.6f ? AutopilotBrake : AutopilotNear;
      }
      else
      {
        // moving target
        // target is the place we should be right now
        Vector3 relTgtPos = DirectionWorldToRelative(targetCur-FutureVisualState().Position());
        //if (fabs(relTgtPos[2])<1.0f) relTgtPos[2] = 0; // avoid oscillation with small offsets
        static float tgtSpeed1 = 0.2f;
        // adjust speed so that you reach target at given time
        static float tgtSpeed2 = 0.005f;
        _pilotSpeed[2] = relTgtPos.Z()*tgtSpeed1 + relTgtPos.Z()*floatMin(fabs(relTgtPos.Z()),30)*tgtSpeed2;
        // note: small adjustments need to be done more carefully
        
        #if _ENABLE_CHEATS
          if (this==GWorld->CameraOn() && CHECK_DIAG(DEPath))
          {
            DIAG_MESSAGE(
              500,"relTgtPos %.2f, relSpd %.2f, spd1 %.2f, spd2 %.2f",
              relTgtPos[2],_pilotSpeed[2],relTgtPos.Z()*tgtSpeed1,relTgtPos.Z()*fabs(relTgtPos.Z())*tgtSpeed2
            );
          }
        #endif
        if (distance[2]>fabs(distance[0])*2 && sizeXZ2>300*300)
          // far away, heading to target, target is moving slow
          _state=AutopilotFar;
      }
    }
    break;
    case AutopilotAlign: case AutopilotReached:
    {
      _pilotHeading=atan2(direction.X(),direction.Z());
      float sizeXZ=sqrt(sizeXZ2);
      
      float speedSize=fabs(FutureVisualState().ModelSpeed().Z());
      // never fly higher than AutopilotNear has forced us
      float avoidGroundStatic = floatMinMax(_defPilotHeight,8,30);
      float avoidGroundNear = floatMax(avoidGroundStatic,speedSize * 0.35f);
      
      float highX=Interpolativ(sizeXZ,5,50,2,avoidGroundNear);
      float highZ=highX;
      // control to be there in estT sec
      const float estT=4.0;
      Vector3Val estSpeed=DirectionWorldToRelative(FutureVisualState()._speed+2.0*FutureVisualState()._acceleration);
      float estSpeedZ = estSpeed.Z();
      float estSpeedX = estSpeed.X();
      Vector3Val estPosA = FutureVisualState().Position()+FutureVisualState()._speed*estT+0.5*estT*estT*FutureVisualState()._acceleration;
      Vector3Val estTgt = targetCur+tgtSpeed*estT;
      
      Vector3 tgtPos=DirectionWorldToRelative(estTgt-estPosA);

#if _ENABLE_CHEATS
      if( CHECK_DIAG(DEPath) )
      {
        Vector3Val estPos=FutureVisualState().Position()+FutureVisualState()._speed*estT; //+0.5*estT*estT*_acceleration;
        {
          Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),VISITOR_NO_ID);
          obj->SetPosition(estPos);
          obj->SetScale(1);
          obj->SetConstantColor(PackedColor(Color(1,0,0)));
          GScene->ShowObject(obj);
        }
        {
          Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),VISITOR_NO_ID);
          obj->SetPosition(estPosA);
          obj->SetScale(0.75);
          obj->SetConstantColor(PackedColor(Color(1,1,0)));
          GScene->ShowObject(obj);
        }
      }
#endif

      // if we are nearly aligned and stable, tolerate some inaccuracy
      if( FutureVisualState()._speed.SquareSizeXZ()<2 && fabs(dive)<0.1 && fabs(bank)<0.1 )
      {
        if( fabs(tgtPos[2])<2 ) tgtPos[2]=0,highZ=0;
        if( fabs(tgtPos[0])<2 ) tgtPos[0]=0,highX=0;
      }
      

      float high=floatMax(highX,highZ);
      if( high<4.0f && FutureVisualState()._speed.Distance2(tgtSpeed)<Square(2.5f)) _state=AutopilotReached;


      if( tgtSpeed.SquareSize()<Square(1) )
      {
        float high=floatMax(highX,highZ);
        float highSpeed=Interpolativ(FutureVisualState()._speed.Distance(tgtSpeed),0.2,20,2,30);
        saturateMax(high,highSpeed);
      }
      else
        _state = AutopilotNear;

      // once we are reached, stay reached, stay low

      float targetAbove=targetCur.Y()-GLOB_LAND->SurfaceYAboveWaterNoWaves(position[0],position[2]);

      if( _state==AutopilotReached )
      {
        avoidGround=-0.5;
        _pilotHeight = targetAbove;
      }
      else
      {
        avoidGround=high;
        _pilotHeight=floatMax(high,targetAbove);
      }
      
      float apDive=dive-tgtPos.Z()*0.05;
      float apBank=bank-tgtPos.X()*0.05;

      // if speed is high enough, do not accelerate
      const float maxZSpd=1;
      const float maxXSpd=1;
      if( FutureVisualState().ModelSpeed().Z()>+maxZSpd ) saturateMax(apDive,0); // no negative
      if( FutureVisualState().ModelSpeed().Z()<-maxZSpd ) saturateMin(apDive,0); // no positive
      if( FutureVisualState().ModelSpeed().X()>+maxXSpd ) saturateMax(apBank,0);
      if( FutureVisualState().ModelSpeed().X()<-maxXSpd ) saturateMin(apBank,0);

      if( estSpeedZ>0 ) saturateMax(apDive,-0.1); // little negative
      if( estSpeedZ<0 ) saturateMin(apDive,+0.1); // little positive
      if( estSpeedX>0 ) saturateMax(apBank,-0.1);
      if( estSpeedX<0 ) saturateMin(apBank,+0.1);

      float posLimit=floatMax(tgtPos.SizeXZ()*(1.0f/4)-0.2f,0);
      float spdLimit=floatMax(tgtSpeed.SizeXZ()*(1.0f/4)-0.2f,0);
      float maxDive=spdLimit*0.3f+posLimit*0.2f;
      saturateMin(maxDive,0.5f);

      saturate(apDive,-maxDive,+maxDive);
      saturate(apBank,-maxDive,+maxDive);

      _forceDive=apDive;
      _forceBank=apBank;

      // pilotSpeed is not used (forceDive and forceBank is used instead)
      // but it may be used in MP prediction
      // it should be low: setting it to zero should give good results
      _pilotSpeed = VZero;

#if _ENABLE_CHEATS
      if( CHECK_DIAG(DEPath) && GLOB_WORLD->CameraOn()==this )
        DIAG_MESSAGE(500,Format("spd %.1f,%.1f, mspd %.1f,%.1f espd %.1f,%.1f, tPos %.1f,%.1f d %.2f->ad %.2f, b %.2f->ab %.2f",
          _pilotSpeed[0],_pilotSpeed[2],FutureVisualState().ModelSpeed()[0],FutureVisualState().ModelSpeed()[2],estSpeed[0],estSpeed[2],tgtPos.X(),tgtPos.Z(),dive,apDive,bank,apBank));
#endif
      
      if( sizeXZ2>Square(90))
        _state=AutopilotNear;
    }
    break;
  }
  // if target is flying a turn, we still want to fly at its full speed, not its speed relative to us
  // if he is fast, we need to consider his horizontal speed instead instead of DirectionWorldToRelative
  // because once we lose forward speed, we will hardly recover it
  
  Vector3 addSpeed = DirectionWorldToRelative(tgtSpeed);
  
  float wantedSpeed = tgtSpeed.SizeXZ();
  float fastTarget = Interpolativ(wantedSpeed,maxSpeed*0.1f,maxSpeed*0.5f,0,1);
  // do this only when our and wanted speed direction agree (more or less)
  // caution: avoid factor 1, as this could make formation flying unstable (overrunning the leader)
  float directionAgrees = InterpolativC(addSpeed.CosAngle(VForward),0.5f,0.7f,0,0.99f);
  saturateMax(addSpeed[2],fastTarget*directionAgrees*wantedSpeed);
  
  _pilotSpeed += addSpeed;
  Limit(_pilotSpeed[2],-10,maxSpeed);
  if( avoidGround>0 )
    AvoidGround(avoidGround);
}

void HelicopterAuto::ResetAutopilot()
{
  // We set state to near. It will go to far automatically (if necessary).
  _state=AutopilotFar;
  //_apDive=0,_apBank=0;
}

bool HelicopterAuto::ProcessFireWeapon(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target,
  const RemoteFireWeaponInfo *remoteInfo, RemoteFireWeaponInfo *localInfo, bool forceLock)
{
  const MagazineType *aInfo = magazine ? magazine->Type() : NULL;
  const AmmoType *ammo = ptr.GetAmmoType();
  VisualState &vs = FutureVisualState();
  if (!ammo)
    return false;
  if (localInfo)
  {
    localInfo->_position = vs.Position();
    localInfo->_visible=floatMax(ammo->visibleFire, ammo->audibleFire);
  }
  switch (ammo->_simulation )
  {
  case AmmoShotRocket:
    if (remoteInfo)
      return false;
    {
      _rocketLRToggle=!_rocketLRToggle;
      Vector3Val pos=( _rocketLRToggle ? Type()->_rocketLPos : Type()->_rocketRPos );
      localInfo->_direction = vs.Direction();
      return FireMissile(context, ptr,pos,localInfo->_direction,Vector3(0,0,aInfo->_initSpeed),target, true, false, forceLock);
    }
  case AmmoShotMissile:
    if (remoteInfo) return false;
    {
      int count = GetLastMissileIndex(context, ptr);
      bool found;
      Vector3 pos = FindMissilePos(count,found, ammo->maverickweaponIndexOffset).Position();
      if (!found)
      {
        pos=( _missileLRToggle ? Type()->_missileLPos : Type()->_missileRPos );
        _missileLRToggle=!_missileLRToggle;
      }
      localInfo->_direction = vs.Direction();
      return FireMissile(context, ptr, pos,localInfo->_direction,Vector3(0,0,aInfo->_initSpeed),target, true, false, forceLock);
    }
  case AmmoShotShell:
    {
      Matrix4Val shootTrans = context._turretType ? GunTurretTransform(vs, *context._turretType) : M4Identity;
      Vector3Val pos = context._turretType ? context._turretType->GetTurretPos(shootTrans) : VZero;
      bool fired;
      Vector3 dir;
      if (remoteInfo)
      {
        dir = remoteInfo->_direction;
        fired = FireShell(context, ptr, pos, dir, target, false, true);
      }
      else
      {
        Vector3 aimtDirection =  context._turretType ? context._turretType->GetTurretDir(shootTrans) : VForward;
        vs.DirectionModelToWorld(dir, aimtDirection);
        if (GIsManual(context._gunner))
        {
          // get fov from the current camera settings
          //CameraType camType = GWorld->GetCameraType();
          //float fov = GScene->GetCamera()->Left();
          float fov = GetCameraFOV();
          // let vehicle adjust the weapon depending on fov
          dir = AdjustWeapon(context, ptr,fov,dir);
        }

        localInfo->_direction = dir;
        fired = FireShell(context, ptr, pos, dir, target, false, false);
      }
      // compute the momentum change, assume a projectile mass 0.25kg

      // make the recoil proportional to muzzle velocity - this is done to reduce recoil for grenade launchers
      float massEst = ammo->hit*(0.25f/13.0f); // assume hit 13 equals to 0.5 kg, hit 650 -> 12 kg
      float forceSize = massEst*aInfo->_initSpeed;
      Vector3 force=-dir*forceSize;
      Vector3 forcePos=vs.DirectionModelToWorld(pos-_shape->CenterOfMass());
      // simulated both remote and local - do not transfer
      AddImpulse(force,forcePos.CrossProduct(force));
      return fired;
    }
  case AmmoShotBullet:
  case AmmoShotSpread:
    {
      Matrix4Val shootTrans = context._turretType ? GunTurretTransform(vs, *context._turretType) : M4Identity;
      Vector3 pos;
      Vector3 dir;
      if (remoteInfo) dir = remoteInfo->_direction;
      if (ptr.GetWeaponType()->_shotFromTurret)
      {
        if (context._turretType)
          pos = context._turretType->
            GetTurretPos(shootTrans);
        else if (context._turret)
          pos = Type()->GetGunPos(context._turret->_gunPosIndexToogle);
        else
          pos = VZero;        
          
        if (!remoteInfo)
        {
          Vector3 aimtDirection = context._turretType ? 
            context._turretType->
              GetTurretDir(shootTrans) : 
            VForward;
            
          vs.DirectionModelToWorld(dir, aimtDirection);
          if (GIsManual(context._gunner))
          {
            // get fov from the current camera settings
            //CameraType camType = GWorld->GetCameraType();
            //float fov = GScene->GetCamera()->Left();
            float fov = GetCameraFOV();
            // let vehicle adjust the weapon depending on fov
            dir = AdjustWeapon(context, ptr, fov, dir);
          }
          localInfo->_direction = dir;
        }
      }
      else
      { 
        size_t pointsCnt = 0;
        if (Type()) 
          pointsCnt = Type()->_gunPosArray.Size();                       
        
        if (context._turretType)
          pos = shootTrans.FastTransform(context._turretType->_gunPos);
        else
        {
          size_t pointsCnt = Type()->_gunPosArray.Size();         
          pos = pointsCnt>0 ? 
            shootTrans.FastTransform(Type()->_gunPosArray[_gunPosIndexToogle]) :
            shootTrans.FastTransform(VZero);          
        }        
             
        //pos = shootTrans.FastTransform(context._turretType ? context._turretType->_gunPos : Type()->_gunPos);
        if (!remoteInfo)
        {
          Vector3Val gunDir = Type()->_gunDirArray[_gunPosIndexToogle];
          Vector3 aimtDirection = shootTrans.Rotate(context._turretType ? context._turretType->_gunDir : gunDir);
          vs.DirectionModelToWorld(dir , aimtDirection);
          if (GIsManual(context._gunner))
          {
            // get fov from the current camera settings
            //CameraType camType = GWorld->GetCameraType();
            //float fov = GScene->GetCamera()->Left();
            float fov = GetCameraFOV();
            // let vehicle adjust the weapon depending on fov
            dir = AdjustWeapon(context, ptr, fov, dir);
          }
          localInfo->_direction = dir;
        }
        
        // update toggle
        if (pointsCnt>0)
          _gunPosIndexToogle = (++_gunPosIndexToogle)%pointsCnt;
      }

      return FireMGun(context, ptr, pos, dir, target, false, remoteInfo!=NULL);
    }
    break;
  case AmmoNone:
    if (localInfo) localInfo->_direction = VForward;
    break;
  default:
    if (localInfo) localInfo->_direction = VForward;
    Fail("Unknown ammo used.");
    break;
  }
  return false;
}

bool HelicopterAuto::FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock)
{
  if (GetNetworkManager().IsControlsPaused())
    return false;

  if (!ptr.IsValid())
    return false;
  if( !GetWeaponLoaded(*context._weapons, ptr) )
    return false;
  if( !IsFireEnabled(context) )
    return false;

  const Magazine *magazine = ptr.GetMagazine();
  if (!magazine)
    return false;
 
  RemoteFireWeaponInfo local;
  bool fired = ProcessFireWeapon(context, ptr, magazine, target, NULL, &local, forceLock);
  if( fired )
  {
    base::PostFireWeapon(context, ptr, target, local);
    return true;
  }
  return false;
}

bool HelicopterAuto::FireWeaponEffects(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine,EntityAI *target, const RemoteFireWeaponInfo *remoteInfo)
{
  const AmmoType *ammo = ValidateFireWeaponWithAmmo(ptr, magazine);
  if (!ammo) return false;

  if (EnableVisualEffects(SimulateVisibleNear)) switch (ammo->_simulation)
  {
    case AmmoShotRocket:
    case AmmoShotMissile:
    case AmmoNone:
      break;
    case AmmoShotShell:
      {
        context._weapons->_gunClouds.Start(1.5f);
        context._weapons->_gunFire.Start(0.1f,1,false);
      }
      break;
    case AmmoShotBullet:
    case AmmoShotSpread:
      context._weapons->_mGunClouds.Start(0.1);
      {
        float duration = 0.1f;
        float intensity = 0.06f;
        const WeaponType *weapon = ptr.GetWeaponType();
        if (weapon)
        {
          duration = weapon->_fireLightDuration;
          intensity = weapon->_fireLightIntensity;
        }
        if (duration > 0 && intensity > 0)
          context._weapons->_mGunFire.Start(duration, intensity, true);
      }
      context._weapons->_mGunFireUntilFrame = Glob.frameID+1;
      context._weapons->_mGunFireTime = Glob.time;
      int newPhase;
      while ((newPhase = toIntFloor(GRandGen.RandomValue() * 3)) == context._weapons->_mGunFirePhase)
        ;
      context._weapons->_mGunFirePhase = newPhase;
      break;
  }
  return base::FireWeaponEffects(context, ptr, magazine,target,remoteInfo);
}

// AI interface

float HelicopterType::GetFieldCost(const GeographyInfo &info, CombatMode mode) const
{
  return 1;
}

float HelicopterType::GetBaseCost(const GeographyInfo &geogr, bool operative, bool includeGradient) const
{
  float cost=GetMinCost(); // basic speed
  // water is low and therefore following water is usually safer and quicker
  if (geogr.u.minWaterDepth>0) cost*=0.90f; // water everywhere
  else if (geogr.u.maxWaterDepth>0) cost*=0.95f; // water somewhere
  // penalty for objects
  //cost *= 1 + geogr.howManyObjects*0.1;
  if (includeGradient)
  {
    // avoid steep hills
    // penalty for hills
    int grad = geogr.u.gradient;
    Assert(grad <= 7);
    static const float gradPenalty[8]={1.0,1.02,1.05,1.2,1.3,1.5,1.7,2.0};
    cost *= gradPenalty[grad];
  }
  return cost;
}

float HelicopterType::GetAirCost(float x, float z) const
{
  float maxHeight = 0;
  saturateMax(x,1); saturateMin(x,LandRange-2);
  saturateMax(z,1); saturateMin(z,LandRange-2);

  for(int i = -2; i <= 2; i++) for(int j = -2; j <= 2; j++)
  {
    const ObjectList &list = GLandscape->GetObjects(z+j, x+i);
    maxHeight = floatMax(maxHeight,list.GetMax().Y() - list.GetMin().Y());
  }
  // heuristic cost, ignore building < 70m
  // tweaked for Seattle Downtown (so far only place, where this is needed)
  float heightCoef = maxHeight * 0.015f;
  saturate(heightCoef, 1.0f, 3.0f);

  return heightCoef;
};

float HelicopterType::GetGradientPenalty(float gradient) const
{
  // TODO: continuous function, sign of gradient
  gradient = fabs(gradient);

  if (gradient >= 0.95f) return 2.0f; // level 7
  else if (gradient >= 0.60f) return 1.7f; // level 6
  else if (gradient >= 0.40f) return 1.5f; // level 5
  else if (gradient >= 0.25f) return 1.3f; // level 4
  else if (gradient >= 0.15f) return 1.2f; // level 3
  else if (gradient >= 0.10f) return 1.05f; // level 2
  else if (gradient >= 0.05f) return 1.02f; // level 1
  return 1.0f; // level 0
}

float HelicopterType::GetCostTurn(int difDir, int &debet, bool forceFullTurn) const
{ // in sec
  if( difDir==0 )
    return 0;
  float aDir=fabs(difDir);
  float cost=aDir*0.15+aDir*aDir*0.02;
  if( difDir<0 )
    return cost*0.8;
  return cost;
}


float HelicopterAuto::FireInRange(const TurretContext &context, const WeaponModePtr &ptr, float &timeToAim, const Target &target) const
{
  WeaponModePtr selected = context._weapons->ValidateWeapon(ptr);
  if (selected.IsValid())
  {
    const Magazine *magazine = ptr.GetMagazine();
    const MagazineType *aInfo = magazine ? magazine->Type() : NULL;
    const AmmoType *ammo = aInfo ? aInfo->_ammo : NULL;

    if (ammo)
    {
      Vector3 relPos = target.AimingPosition()-FutureVisualState().Position();
      // for rockets heli should prefer targets that are near the center
      float cosAngle = relPos.CosAngle(FutureVisualState().Direction());
      // it should prefer targets not very near, as aiming them is difficult
      float distance = relPos.Size();

      // for MGun or unguided rockets ideal target distance is 1000
      // add 10 sec for each 1000 m for distance above 1000
      // to prefer guided missiles for long range attack
      float timeToAimDistance = distance>1000 ? (distance-1000)*(10.0f/1000) : 0;
      saturate(timeToAimDistance,0,30);

      // add X sec for 90 degree or more turn
      float timeToAimAngle = (1-floatMax(cosAngle,0));

      switch (ammo->_simulation)
      {
      case AmmoShotMissile:
      case AmmoShotRocket:
        if  (ammo->maxControlRange>10) // guided missile
        {
          // for guided missiles we don't need to turn Helicopter,
          // but it can take some time to lock missile
          timeToAim = 0;
        }
        else
        {
          // add 15 sec for 90 degree or more turn
          // added 0.05 to prefer weapon aim-able weapons 
          timeToAim = 15 * floatMax(timeToAimAngle,0.05) + timeToAimDistance;
        }
        break;
      case AmmoShotShell:
      case AmmoShotBullet:
        timeToAim = 0;
        // add 15 sec for 90 degree or more turn if turret cannot turn
        if(!context._turretType ||  fabs(context._turretType->_minTurn) < 0.01) timeToAim = 15 * floatMax(timeToAimAngle,0.05);
        break;
      }

      WeaponModePtr current = context._weapons->ValidatedCurrentWeapon();

      if (!current.IsValid() || ptr.GetMuzzleType() != current.GetMuzzleType())
      {
        // assume switching weapon takes a little time
        timeToAim += 0.15f;
      }

      if (selected != current)
      {
        // assume switching mode takes a little time
        timeToAim += 0.1f;
      }

    }
  }
  //return GetAimed(weapon,target.idExact);
  return 1;
}

float HelicopterAuto::FireAngleInRange(const TurretContext &context, const WeaponModePtr &ptr, Vector3Par rel) const
{
  // helicopter cannot fire high, can fire slight low
  if( rel.Y()>0 )
    return 0;
  float size2=rel.SquareSizeXZ();
  float y2=Square(rel.Y());
  const float maxY=0.25;
  if( y2>size2*Square(maxY) )
    return 0;
  // nearly same level
  float invSize=InvSqrt(size2);
  return 1-rel.Y()*invSize*(1/maxY);
}

void HelicopterAuto::MoveWeapons(float deltaT)
{
  MoveCrewHead();
}

#if _ENABLE_AI

void HelicopterAuto::AIPilot(AIBrain *unit, float deltaT )
{
  Assert(unit);
  // AI: activate all helpers
  _pilotSpeedHelper=true;
  _pilotCyclicDirect = false;
  _pilotDirHelper=true;
  _pilotHeightHelper=true;
  _pilotVSpeedHelper= false;
  SwitchPilotHeightAGL(true);

  _dirCompensate=1;
  // if aiming for fire, we need quick reactions
  /*
  if( _fireMode>=0 )
  {
    if( !_firePrepareOnly ) _dirCompensate=0.1;
    else _dirCompensate=0.3;
  }
  */

  Vector3 steerPos=SteerPoint(2.0,4.0);

  VisualState &vs = FutureVisualState();

  Vector3 steerWant=PositionWorldToModel(steerPos);
  
  float headChange=atan2(steerWant.X(),steerWant.Z());
  float speedWanted=0;
  
  float inCombat=2-_nearestEnemy*(1.0/200);
  saturate(inCombat,0,1);
  if (unit->GetCombatModeLowLevel() <= CMSafe)
    inCombat = 0;

  speedWanted=Type()->GetMaxSpeedMs()*inCombat*0.4;
  
  Target *assigned = unit->GetTargetAssigned();
  if (assigned && _sweepTarget!=assigned && Type()->_enableSweep)
  {
    _sweepTarget=assigned;
    _sweepState=SweepDisengage;
    _sweepDelay=Glob.time+10;
  }

  if (_sweepTarget)
  {
    EntityAI *swAI = _sweepTarget->idExact;
    if (unit->GetCombatModeLowLevel()<=CMSafe || !unit->IsFireEnabled(_sweepTarget))
      _sweepTarget = NULL;
    // check if target is alive
    else if
    (
      // destroyed
      !swAI || swAI->IsDamageDestroyed()
      // or not enemy and not ordered to fire
      || _sweepTarget->State(unit)<TargetEnemy && unit->GetEnableFireTarget()!=_sweepTarget
    )
      _sweepTarget=NULL;
  }

  _forceDive=10;
  _forceBank=10;
  _minAttackDive = 0;

  UpdateStopMode(unit);

  bool autopilot=false;
  if (GetPilotGear() && !_landContact)
  {
    float curSurfaceY=GLOB_LAND->RoadSurfaceYAboveWater(vs.Position().X(),vs.Position().Z());

    if (vs.Position().Y()>curSurfaceY+5 && _rotorSpeedWanted>0.5f && vs._speed.Y()>=0)
    {
      // once we are high enough, climbing and not auto-rotating, retract gear
      SetPilotGear(false);
    }
  }
  // safety measure - when on the ground, we need the gear down
  if (_landContact) SetPilotGear(true);
  
  if( unit->GetState()==AIBrain::Stopping || unit->GetState()==AIBrain::Stopped )
  {
    // special handling of stop state
    // landing position is in _stopPositon

    Vector3 sPos = _stopPosition;
    if (_stopMode==SMGetIn)
      sPos[1] += 1.0f;
    else if (_stopMode==SMGetOut)
      sPos[1] += 1.5f;

    // direction - opposite to wind
    Vector3Val windDir = GLandscape->GetWindSpeed();

    float windSize = windDir.Size();

    Vector3 landDir = FutureVisualState().Direction();
    if (windSize>1.5f)
    {
      float windFactor = windSize*0.2f;
      saturate(windFactor,0,0.5f);

      landDir = -windDir*windFactor+FutureVisualState().Direction()*(1-windFactor);
      landDir[1] = 0;
      landDir.Normalize();
    }

  
    Autopilot(deltaT,sPos,sPos,VZero,landDir);
    
    if (_state>=AutopilotAlign)
    {
      SetPilotGear(true);
    }
    
    speedWanted=0;

    // check if heli is already landed
    if (_landContact)
    {
      if (_stopMode==SMLand)
      {
        StopRotor();
        if( FutureVisualState()._rotorSpeed<0.7 )
        {
          // note: Pilot may get out - Brain may be NULL
          unit->OnStepCompleted();
          if( unit->IsFreeSoldier() )
            return;
        }
      }
      if (unit->GetState()==AIBrain::Stopping && vs.Speed().SquareSize()<Square(10))
      {
        // note: Pilot may get out - Brain may be NULL
        unit->OnStepCompleted();
        if( unit->IsFreeSoldier() )
          return;
      }
    }

    float bottomY = FutureVisualState().Position().Y()+_shape->GeometryLevel()->Min().Y();
    switch (_stopMode)
    {
      case SMLand:
        autopilot=true;
        if( _state==AutopilotReached )
        {
          float curSurfaceY=GLOB_LAND->RoadSurfaceYAboveWater(FutureVisualState().Position().X(),FutureVisualState().Position().Z());
          if( FutureVisualState().Position().Y()<=curSurfaceY+2.5f || bottomY<curSurfaceY+0.5f)
            StopRotor();
        }
        break;
      case SMGetIn:
      case SMGetOut:
        if (_state == AutopilotReached)
        {
          if (_landContact || FutureVisualState().Position().Y()<=sPos.Y()+0.5f)
          {
            if (unit->GetState() == AIBrain::Stopping)
            {
              unit->OnStepCompleted();
              Assert(!unit->IsFreeSoldier());
            }
          }
        }
        autopilot=true;
        break;
    }
  }
  else if( !_landContact && _sweepTarget && Type()->_enableSweep && IsAbleToMove())
  {
    bool laserTarget = _sweepTarget->idExact->GetType()->GetNvTarget() || _sweepTarget->idExact->GetType()->GetLaserTarget();
    speedWanted=laserTarget ? 0 : Type()->GetMaxSpeedMs()*0.5f;
    Vector3 tgtPos;

    // use exact weapon aiming calculation
    // TODO: which weapons?
    TurretContext context;
    bool valid = GetPrimaryGunnerTurret(context);
    DoAssert(valid);
    float timeToLead;
    WeaponModePtr ptr = context._weapons->ValidatedCurrentWeapon();
    if (!CalculateAimWeaponPos(context, ptr,tgtPos,timeToLead,_sweepTarget,false))
    {
      // if exact calculation failed, used approximate calculation
      float tgtDist = _sweepTarget->AimingPosition().Distance(FutureVisualState().Position());
      float tgtTime = tgtDist*(1.0f/100);
      tgtPos = _sweepTarget->AimingPosition()+_sweepTarget->GetSpeed(NULL)*tgtTime;
    }

    // when the position is not known accurately enough, search the target by circling its assumed position
    Vector3 flyToPos = tgtPos;
    
    float posAccuracy = _sweepTarget->FadingPosAccuracy();
    if (posAccuracy>25.0f)
    {
      flyToPos += unit->ScanTargetOffset(_sweepTarget);
    }
    
    Vector3 relPos = tgtPos-FutureVisualState().Position();
    float distXZ=relPos.SizeXZ();
    //const float safeDistance = laserTarget ? 400 : 250;
    const float safeDistance = laserTarget ? 600 : 400;
    if( _sweepState==SweepDisengage )
    {
      Vector3 aimDir=PositionWorldToModel(tgtPos);
      headChange=atan2(aimDir.X(),aimDir.Z());

      // move - to be hard target
      if( distXZ>safeDistance || fabs(headChange)<0.5f )
      {
        _sweepState=SweepEngage;
        _sweepDelay=Glob.time+25;
      }
      headChange=0;
    }
    else if( _sweepState==SweepEngage )
    {
      Vector3 aimDir=PositionWorldToModel(flyToPos);
      headChange=atan2(aimDir.X(),aimDir.Z());
      // reduce speed while turning - we will gain it during the sweep anyway
      // avoid slowing down too much - this would cause us climbing
      float minSpeed = FutureVisualState().ModelSpeed().Z()*0.96f;
      speedWanted = floatMax(minSpeed,speedWanted*0.5f);
      
      // move - to be hard target
      if( distXZ<100 )
      {
        _sweepState=SweepFire;
        _sweepDelay=Glob.time+5; // start immediately
        _sweepDir=relPos;
      }

      if( aimDir.Z()>20 && fabs(headChange)<0.2f )
      { // if the target is horizontally aimed, make vertical adjust
        float minSpeed=laserTarget ? Type()->GetMaxSpeedMs()*0.3f : 0;
        float speed=FutureVisualState().ModelSpeed()[2];
        if( speed>minSpeed )
        { // we are flying fast enough
          // actual aiming (using speed)
          _forceDive=(relPos.Y()-3)*relPos.InvSizeXZ();
          saturate(_forceDive,-0.7f,0);
          _minAttackDive = _forceDive;
        }
      }
    }
    else if( _sweepState==SweepFire )
    {
      // we are very close - continue flying in the last direction
      Vector3 aimDir=PositionWorldToModel(flyToPos);
      Vector3 relSweepDir=DirectionWorldToRelative(_sweepDir);
      headChange=atan2(relSweepDir.X(),relSweepDir.Z());
      if( fabs(headChange)>0.6f && ( distXZ<50 || distXZ>150 ) )
      {
        _sweepState=SweepDisengage;
        _sweepDelay=Glob.time+10; // start immediately
      } 
      else
      {
        float minSpeed=laserTarget ? Type()->GetMaxSpeedMs()*0.3f : 0;
        float speed=FutureVisualState().ModelSpeed()[2];
        if( speed>minSpeed && aimDir.Z()>20)
        { // we are flying fast enough
          // actual aiming (using speed)
          _forceDive=(relPos.Y()-3)*relPos.InvSizeXZ();
          // limit diving
          // we want to stay at least 25 m above the target
          // i.e. -relPos.Y()>=25
          float above = -relPos.Y()-25;
          float limitDive = InterpolativC(above,0,10,-0.1f,-0.7f);
          saturate(_forceDive,limitDive,0);
          _minAttackDive = _forceDive;
        }
      }
    }

    // sweep target should be slightly below
    float wantAbove=distXZ*0.1f;
    saturate(wantAbove,30,50);
    float wantY=tgtPos.Y()+wantAbove;

    float curSurfaceY=GLOB_LAND->SurfaceYAboveWaterNoWaves(FutureVisualState().Position().X(),FutureVisualState().Position().Z());
    _pilotHeight=wantY-curSurfaceY;
    
    // when firing at laser target, do not fly too high
    saturate(_pilotHeight,30,laserTarget ? 60 : 200);

    if( Glob.time>_sweepDelay )
      _sweepTarget=NULL;
  }
  {
    // leader unit

    speedWanted=floatMin(_limitSpeed, _limitSpeedForced); // go faster
    
    #if DIAG_SPEED
    if( this==GWorld->CameraOn() )
      LogF("Basic speed %.1f",speedWanted*3.6f);
    #endif

    bool isStopWaypoint = false;

    // check path position
    const Path &path=unit->GetPath();
    if( path.Size()>=2 )
    {
      // wait until path is old enough, this prevents starting engine because of path which disappears quickly again
      // Cf. news:hf99a3$slb$2@new-server.localdomain
      if (path.GetSearchTime()<Glob.time-2)
      EngineOn();

      if (unit->GetPlanningMode() != AIBrain::LeaderDirect)
      {
        float precision=GetPrecision();
        //_moveMode=gotoNormal;
        float cost=path.CostAtPos(FutureVisualState().Position());
        Vector3 pos=path.PosAtCost(cost,0,FutureVisualState().Position(),cost);

        float distEnd2=FutureVisualState().Position().DistanceXZ2(path.End());
        float dist2=FutureVisualState().Position().DistanceXZ2(pos);
        // check if we have first point of Plan complete

#if _VBS3_AVRS
        if(_heliVector == Vector3(0.0f,0.0f,0.0f) || _wPPos != path.End())
        {
          _heliVector = path.End()-FutureVisualState().FutureVisualState().Position();
          _wPPos = path.End();
        }

        // this should give about 40 degree error rate when
        // approaching the waypoint. if it fails then we continue to 
        // the next one.
        if( _heliVector.CosAngle(path.End()-FutureVisualState().FutureVisualState().Position()) < 0.2f && _stopMode == SMNone && _landing == LMNone)
        {
          int pathSize = path.Size();         
          unit->OnStepCompleted();
                    
          // re-do calculation only, when path changes
          if(pathSize != path.Size())
            _heliVector = Vector3(0.0,0.0,0.0);
        }
        else
        {
#endif
          // when the waypoint will not slow us down, we may want to decrease its precision
          if(!isStopWaypoint)
          {
            //higher speed -> higher WP tolerance
            float factor = FutureVisualState().Speed().Size()/2.0f;
            saturate(factor,1.0f,10.0f);
            precision *= factor;
          }

          if( distEnd2 < Square(precision) || cost>path.EndCost() )
          {
#if _VBS3_AVRS
            // we've reached the waypoint
            _heliVector = Vector3(0.0,0.0,0.0);
#endif
            unit->OnStepCompleted();
          //        unit->ForceReplan();
          }
          if( dist2>Square(precision*3) )
            unit->OnStepTimedOut();
#if _VBS3_AVRS
        }
#endif
      }
      _pilotHeight=_defPilotHeight;
    }
    else
    {
      //_moveMode=gotoWait;
      speedWanted=0;
      saturateMin(_pilotHeight,_defPilotHeight);

      /*
      Autopilot(deltaT,_stopPosition,VZero,Direction());
      speedWanted=0;
      */
      // do not turn engine on
    }

    saturate(speedWanted,-_limitSpeed,+_limitSpeed); // move max. by given speed
    saturate(speedWanted,-_limitSpeedForced,+_limitSpeedForced); // move max. by given speed - scripted limit

    // when we want to rotate, avoid accelerating

    float limSpeed = Interpolativ(fabs(headChange),0,H_PI/2,Type()->GetMaxSpeedMs(),FutureVisualState().ModelSpeed().Z());

    saturateMin(speedWanted,limSpeed);
    
    if( inCombat>=0.3f && _pilotHeight>=25)
    { // when flying low, do not speed in combat
      float combatSpeed=Type()->GetMaxSpeedMs()*inCombat*0.5f;
      saturateMax(speedWanted,combatSpeed);
    }
  }

  Assert(unit);

  AvoidCollision();

  if (_stopMode==SMNone)
  {
  if (Glob.time<_pilotAvoidHigh)
    saturateMax(_pilotHeight,_pilotAvoidHighHeight);
  else
    _pilotAvoidHighHeight = 0;
  if (Glob.time<_pilotAvoidLow)
      saturateMin(_pilotHeight,floatMax(_pilotAvoidLowHeight,30));
  else
    _pilotAvoidLowHeight = 100000;
  }
  /*
  if( unit->GetState()!=AIUnit::Stopping && unit->GetState()!=AIUnit::Stopped )
  {
    EngineOn();
  }
  */
  
  if( !autopilot )
  {
    float speedSize = fabs(FutureVisualState()._modelSpeed.Z());
    float avoidGround = floatMax(0.5f, speedSize * 0.35f);
    if (avoidGround>_pilotHeight) avoidGround = _pilotHeight;

    _pilotSpeed=Vector3(0,0,speedWanted);

    if( avoidGround > 0.0f)
      AvoidGround(avoidGround);

    // if we need to decrease speed significantly, we may want to use braking maneuver

    if (FutureVisualState().ModelSpeed().Z()-speedWanted>Type()->GetMaxSpeedMs()*0.5f && speedWanted>=0 && FutureVisualState().ModelSpeed().Z()<MaxBrakeSpeed)
      BrakingManeuver();
    else
    {
      float maxTurn=H_PI;
      if( inCombat>0.1f )
      {
        const float maxTurnInCombat=H_PI*0.4f;
        const float maxTurnNoCombat=H_PI;
        maxTurn=(maxTurnInCombat-maxTurnNoCombat)*inCombat+maxTurnNoCombat;
      }
      saturate(headChange,-maxTurn,+maxTurn);
      float curHeading=atan2(FutureVisualState().Direction()[0],FutureVisualState().Direction()[2]);
      _pilotHeading=curHeading+headChange;
    }

    if (_rotorSpeedWanted<0.1f)
    {
      // when the engine is off, we may be either sitting on the ground, or the engine is malfunctioning
      // in the 2nd case we need to auto-rotate to avoid crash
      _pilotHeight = 0;
      if (!GetPilotGear())
      {
        float surfY=GLOB_LAND->RoadSurfaceYAboveWater(FutureVisualState().Position());
        // once close to the ground, lower the gear
        SetPilotGear(surfY+30>FutureVisualState().Position().Y());
      }
    }
  }

#if _VBS3
  AIBrain *leader = unit->GetFormationLeader();
  if (unit == leader && unit->GetForceSpeed()>=0)
    saturateMin(_pilotSpeed[2], unit->GetForceSpeed()/3.6);
#endif

#if 0
  if( this==GWorld->CameraOn() )
    LogF("InCombat %.2f wSpd %.1f pSpd %.1f spd %.1f",inCombat,speedWanted*3.6,_pilotSpeed[2]*3.6,ModelSpeed()[2]*3.6);
#endif

#if 0
    if( this==GWorld->CameraOn() && _fireMode>=0 )
      GEngine->ShowMessage(100,"Heli mode %d state %d, delay %.1f spd %.1f",_fireMode,_fireState,_fireStateDelay-Glob.time,speedWanted);
#endif

  // aim to current target
  // if( !_isDead ) AIFire(unit, deltaT);
}
#endif // _ENABLE_AI

void HelicopterAuto::DrawDiags()
{
  if (CHECK_DIAG(DECombat))
  {
  #define DRAW_OBJ(obj) GScene->DrawObject(obj)
    if (CommanderUnit())
    {
      LODShape *forceArrow=GScene->ForceArrow();

      if( _stopPosition.SquareSize()>0.5 )
      {
        Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),VISITOR_NO_ID);
        obj->SetPosition(_stopPosition);
        obj->SetScale(2);
        obj->SetConstantColor(PackedColor(Color(1,0,1)));
        GScene->ShowObject(obj);
      }

  #if 1
        // draw pilot diags
        {
          Matrix3 rotY(MRotationY,-_pilotHeading);
          Vector3 pilotDir=rotY.Direction();
          Ref<ObjectColored> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);
          Point3 pos=RenderVisualState().Position()+Vector3(0,5,0);

          float size=0.6;
          arrow->SetPosition(pos);
          arrow->SetOrient(pilotDir,VUp);
          arrow->SetPosition
          (
            arrow->RenderVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size)
          );
          arrow->SetScale(size);
          arrow->SetConstantColor(PackedColor(Color(1,1,0)));

          DRAW_OBJ(arrow);
        }
  #endif
    }
  }
  base::DrawDiags();
}

RString HelicopterAuto::DiagText() const
{
  RString text=base::DiagText();
  char buf[256];
  sprintf(buf," pSpd=%.1f, MRW=%.2f, ",_pilotSpeed[2]*3.6,_mainRotorWanted);
  if (_state!=AutopilotNear)
    text = text + FindEnumName(_state);
  return text+RString(buf);
}

void Helicopter::GetAimActions(float &aimX, float &aimY) const
{
  aimX = GInput.GetActionWithCurve(InputInterface::CurveFovMouse, UAAimRight, UAAimLeft);
  aimY = GInput.GetActionWithCurve(InputInterface::CurveFovMouse, UAAimUp, UAAimDown);
}

UserAction Helicopter::GetManualFireAction() const
{
  return UAHeliManualFire;
}


#define SERIAL_DEF_VISUALSTATE(name, value) CHECK(ar.Serialize(#name, FutureVisualState()._##name, 1, value))

LSError Helicopter::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  // TODO: serialize other members

  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    SERIAL_DEF_VISUALSTATE(cyclicForward, 0);
    SERIAL_DEF_VISUALSTATE(cyclicAside, 0);
    SERIAL_DEF_VISUALSTATE(rotorPosition, 0);
    SERIAL_DEF_VISUALSTATE(rotorDive, 0);
    SERIAL_DEF_VISUALSTATE(gearsUp, 0);
    SERIAL_DEF_VISUALSTATE(rotorSpeed,1);SERIAL_DEF(rotorSpeedWanted,1);
    SERIAL_DEF(backRotor,0);SERIAL_DEF(backRotorWanted,0);
    SERIAL_DEF_VISUALSTATE(mainRotor,0);SERIAL_DEF(mainRotorWanted,0);
    SERIAL_DEF(cyclicForwardWanted,0);
    SERIAL_DEF(cyclicAsideWanted,0);
    SERIAL_DEF(rotorDiveWanted,0);
    SERIAL_DEF(pilotGear, true);
  }
/*
  SERIAL_DEF(gunYRot,0);SERIAL_DEF(gunYRotWanted,0);
  SERIAL_DEF(gunXRot,0);SERIAL_DEF(gunXRotWanted,0);
  SERIAL_DEF(gunXSpeed,0);SERIAL_DEF(gunYSpeed,0);
*/

  return LSOK;
}

#undef SERIAL_DEF_VISUALSTATE

static const EnumName AutopilotStateNames[]=
{
  EnumName(AutopilotFar, "FAR"),
  EnumName(AutopilotBrake, "BRAKE"),
  EnumName(AutopilotNear, "NEAR"),
  EnumName(AutopilotAlign, "ALIGN"),
  EnumName(AutopilotReached, "REACHED"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(AutopilotState dummy)
{
  return AutopilotStateNames;
}

#pragma warning(disable:4060)


bool HelicopterAuto::GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const
{
  switch (action->GetType())
  {
    case ATAutoHover:
    case ATAutoHoverCancel:
      // No parameters
      return true;
  }
  return base::GetActionParams(params, action, unit);
}

void HelicopterAuto::PerformAction(const Action *action, AIBrain *unit)
{
  switch (action->GetType())
  {
    case ATAutoHover:
      AutohoverOn();
      return;
    case ATAutoHoverCancel:
      AutohoverOff();
      return;
    default:
      base::PerformAction(action,unit);
      break;
  }
}

void HelicopterAuto::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  if (unit && (unit==DriverBrain() || unit==CopilotBrain()) && QIsManual())
  {
    if( (!PilotUnit() || !PilotUnit()->IsPlayer()) || !CopilotUnit())
    {
      UIAction action(new ActionBasic(_hoveringAutopilot ? ATAutoHoverCancel : ATAutoHover, this));
      add(action);
    }
  }
  
  if(unit)
  {
    bool isPlayer = unit->IsPlayer();
    if (_enableCopilots && isPlayer && unit->GetVehicleIn() == this)
    {
      //pilot unlocks / takes control back
      if (unit == PilotUnit())
      {
        if(_copilot || _unLockedControls)
        {
          UIAction action(new ActionBasic(ATLockVehicleControl, this));
          add(action);
        }
        else
        {
          HasCopilot func;
          if(ForEachTurret(func))
          {
            UIAction action(new ActionBasic(ATUnlockVehicleControl, this));
            add(action);
          }
        }
      }

      //copilot release control
      if (CopilotBrain() == unit)
      {
        UIAction action(new ActionBasic(ATSuspendVehicleControl, this));
        add(action);
      }

      TurretContext context;
      if((_unLockedControls || !PilotUnit() || !PilotUnit()->IsAnyPlayer()) && unit->GetPerson() && FindTurret(unit->GetPerson(),context))
      {
        if (context._turretType && context._turretType->_isCopilot && unit != PilotUnit() && !_copilot)
        {
          UIAction action(new ActionBasic(ATTakeVehicleControl, this));
          add(action);
        }
      }
    }
  }

  base::GetActions(add, unit);
}

#if _HELISIM_PRECISE_INTERACTION
void HelicopterAuto::GetDrawActions3D(UIActions &actions, AIBrain *unit)
{
  if (unit && (unit==DriverBrain() || unit==CopilotBrain()) && QIsManual())
  {
    if( (!PilotUnit() || !PilotUnit()->IsPlayer()) || !CopilotUnit())
    {
      UIAction action(new ActionBasic(_hoveringAutopilot ? ATAutoHoverCancel : ATAutoHover, this));
      actions.Add(action);
    }
  }

  if(unit)
  {
    bool isPlayer = unit->IsPlayer();
    if (isPlayer && unit->GetVehicleIn() == this)
    {
      //pilot unlocks / takes control back
      if (unit == PilotUnit())
      {
        if(_copilot || _unLockedControls)
        {
          UIAction action(new ActionBasic(ATLockVehicleControl, this));
          actions.Add(action);
        }
        else
        {
          HasCopilot func;
          if(ForEachTurret(func))
          {
            UIAction action(new ActionBasic(ATUnlockVehicleControl, this));
            actions.Add(action);
          }
        }
      }

      //copilot release control
      if (CopilotBrain() == unit)
      {
        UIAction action(new ActionBasic(ATSuspendVehicleControl, this));
        actions.Add(action);
      }

      TurretContext context;
      if((_unLockedControls || !PilotUnit() || !PilotUnit()->IsAnyPlayer()) && unit->GetPerson() && FindTurret(unit->GetPerson(),context))
      {
        if (context._turretType && context._turretType->_isCopilot && unit != PilotUnit() && !_copilot)
        {
          UIAction action(new ActionBasic(ATTakeVehicleControl, this));
          actions.Add(action);
        }
      }
    }
  }

  base::GetDrawActions3D(actions, unit);
}
#endif

bool HelicopterAuto::IsStopped() const
{
  // check manual heli stopped
  // check height above surface
  // caution: not every object contact means we are stopped
  if (_landContact)
  {
    return base::IsStopped();
  }

  // high flying heli cannot be stopped
  float surfaceY=GLOB_LAND->RoadSurfaceYAboveWater(FutureVisualState().Position());
  float height = FutureVisualState().Position().Y()-surfaceY;
  if (height>5) return false;

  return base::IsStopped();
}

LSError HelicopterAuto::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    CHECK(ar.Serialize("defPilotHeight", _defPilotHeight, 1, 50))
    CHECK(ar.Serialize("_pilotHeight", _pilotHeight, 1, 2.5))
    CHECK(::Serialize(ar, "_pilotSpeed", _pilotSpeed, 1, VZero))
    CHECK(::Serialize(ar, "_stopPosition", _stopPosition, 1, VZero))
    CHECK(ar.SerializeEnum("stopMode", _stopMode, 1, SMNone))
    CHECK(ar.SerializeEnum("stopResult", _stopResult, 1, SPNotReady))
    CHECK(ar.SerializeEnum("state", _state, 1, AutopilotNear))
    CHECK(ar.Serialize("hoveringAutopilot", _hoveringAutopilot, 1, false))
  }
  // TODO: serialize other members

  return LSOK;
}

#define HELICOPTER_MSG_LIST(XX) \
  XX(Create, CreateHelicopter) \
  XX(UpdateGeneric, UpdateHelicopter) \
  XX(UpdatePosition, UpdatePositionHelicopter)

DEFINE_NETWORK_OBJECT(HelicopterAuto, base, HELICOPTER_MSG_LIST)

#define CREATE_HELICOPTER_MSG(MessageName, XX) \
  XX(MessageName, float, rotorSpeed, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 0), DOC_MSG("Initial rotor speed"), TRANSF)

DECLARE_NET_INDICES_EX(CreateHelicopter, CreateVehicleAI, CREATE_HELICOPTER_MSG)
DEFINE_NET_INDICES_EX(CreateHelicopter, CreateVehicleAI, CREATE_HELICOPTER_MSG)

#define UPDATE_HELICOPTER_MSG(MessageName, XX) \
  XX(MessageName, float, rotorSpeedWanted, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 0), DOC_MSG("Wanted rotor speed"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, int, state, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, 0), DOC_MSG("Autopilot state"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, float, pilotHeight, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Height, wanted by pilot"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, Vector3, pilotSpeed, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, VZero), DOC_MSG("Speed, wanted by pilot"), TRANSF, ET_ABS_DIF, 1) \
  XX(MessageName, int, stopMode, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, 0), DOC_MSG("Landing type"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, int, stopResult, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, PlaneOrHeli::SPNotReady), DOC_MSG("Landing result"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, Vector3, stopPosition, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, VZero), DOC_MSG("Landing position"), TRANSF, ET_ABS_DIF, 0.1) \
  XX(MessageName, bool, pilotSpeedHelper, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("pilotSpeed is valid"), TRANSF, ET_NOT_EQUAL, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, bool, pilotHeightHelper, NDTBool, bool, NCTNone, DEFVALUE(bool, true), DOC_MSG("pilotHeight is valid"), TRANSF, ET_NOT_EQUAL, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, bool, pilotDirHelper, NDTBool, bool, NCTNone, DEFVALUE(bool, true), DOC_MSG("pilotHeading is valid"), TRANSF, ET_NOT_EQUAL, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, bool, pilotGear, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Position of gear, wanted by pilot"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, hoveringAutopilot, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Hovering autopilot on/off toggle"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE)

DECLARE_NET_INDICES_EX_ERR(UpdateHelicopter, UpdateTransport, UPDATE_HELICOPTER_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateHelicopter, UpdateTransport, UPDATE_HELICOPTER_MSG, NoErrorInitialFunc)

#define UPDATE_POSITION_HELICOPTER_MSG(MessageName, XX) \
  XX(MessageName, float, backRotorWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted back rotor control"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, mainRotorWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted main rotor control"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, cyclicForwardWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted forward cyclic position"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, cyclicAsideWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted aside cyclic position"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, rotorDiveWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted rotor dive"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, bankWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted bank position"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, diveWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted dive"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, pilotHeading, NDTFloat, float, NCTFloatAngle, DEFVALUE(float, 0), DOC_MSG("Heading, wanted by pilot"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, pilotDive, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Dive, wanted by pilot"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR)

DECLARE_NET_INDICES_EX_ERR(UpdatePositionHelicopter, UpdatePositionVehicle, UPDATE_POSITION_HELICOPTER_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionHelicopter, UpdatePositionVehicle, UPDATE_POSITION_HELICOPTER_MSG, NoErrorInitialFunc)

NetworkMessageFormat &HelicopterAuto::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCCreate:
    base::CreateFormat(cls, format);
    CREATE_HELICOPTER_MSG(CreateHelicopter, MSG_FORMAT)
    break;
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_HELICOPTER_MSG(UpdateHelicopter, MSG_FORMAT_ERR)
    break;
  case NMCUpdatePosition:
    base::CreateFormat(cls, format);
    UPDATE_POSITION_HELICOPTER_MSG(UpdatePositionHelicopter, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

HelicopterAuto *HelicopterAuto::CreateObject(NetworkMessageContext &ctx)
{
  Entity *veh = Entity::CreateObject(ctx);
  HelicopterAuto *heli = dyn_cast<HelicopterAuto>(veh);
  if (!heli) return NULL;
  heli->TransferMsg(ctx);
  return heli;
}

TMError HelicopterAuto::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    if (ctx.IsSending())
    {
      TMCHECK(base::TransferMsg(ctx))
    }
    {
      PREPARE_TRANSFER(CreateHelicopter)
      TRANSF_EX(rotorSpeed, FutureVisualState()._rotorSpeed)
    }
    break;
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateHelicopter)

      TRANSF(rotorSpeedWanted)

      TRANSF_ENUM(state)
      TRANSF(pilotHeight)
      TRANSF(pilotSpeed)
      TRANSF_ENUM(stopMode)
      TRANSF_ENUM(stopResult)
      TRANSF(stopPosition)
      TRANSF(pilotSpeedHelper)
      TRANSF(pilotHeightHelper)
      TRANSF(pilotDirHelper)
      TRANSF(pilotGear)
      TRANSF(hoveringAutopilot)
      
      if (!ctx.IsSending() && (ctx.GetInitialUpdate() || GetNetworkManager().GetClientState()<NCSGameLoaded))
        FutureVisualState()._rotorSpeed = _rotorSpeedWanted;
    }
    break;
  case NMCUpdatePosition:
    {
      PREPARE_TRANSFER(UpdatePositionHelicopter)

      Matrix3 oldTrans = FutureVisualState().Orientation();
      TMCHECK(base::TransferMsg(ctx))
      StabilizeTurrets(oldTrans, FutureVisualState().Orientation());
      TRANSF(backRotorWanted)
      TRANSF(mainRotorWanted)
      TRANSF(cyclicForwardWanted)
      TRANSF(cyclicAsideWanted)
      TRANSF(rotorDiveWanted)
      TRANSF(bankWanted)
      TRANSF(diveWanted)
      TRANSF(pilotHeading)
      TRANSF(pilotDive)
      if (!ctx.IsSending() && (ctx.GetInitialUpdate() || GetNetworkManager().GetClientState()<NCSGameLoaded))
        FutureVisualState()._mainRotor = _mainRotorWanted;

    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

#define MAKE_FINITE(x) \
  if (!_finite(x)) \
  { \
    RptF("%s(%d) : Infinite value %s==%g",__FILE__,__LINE__,#x,x); \
    x = 1000; \
  }


float HelicopterAuto::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += base::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdateHelicopter)

      ICALCERR_ABSDIF(float, rotorSpeedWanted, ERR_COEF_MODE)
      ICALCERR_NEQ(int, state, ERR_COEF_MODE)
      ICALCERR_ABSDIF(float, pilotHeight, ERR_COEF_VALUE_MAJOR)
      ICALCERR_DIST(pilotSpeed, 1)
      ICALCERR_NEQ(int, stopMode, ERR_COEF_MODE)
      ICALCERR_NEQ(int, stopResult, ERR_COEF_MODE)
      ICALCERR_DIST(stopPosition, 0.1)

      ICALCERR_NEQ(bool, pilotSpeedHelper, ERR_COEF_VALUE_MAJOR)
      ICALCERR_NEQ(bool, pilotHeightHelper, ERR_COEF_VALUE_MAJOR)
      ICALCERR_NEQ(bool, pilotDirHelper, ERR_COEF_VALUE_MAJOR)
      ICALCERR_NEQ(bool, pilotGear, ERR_COEF_MODE)

      ICALCERR_NEQ(bool, hoveringAutopilot, ERR_COEF_MODE)
    }
    break;
  case NMCUpdatePosition:
    {
      error +=  base::CalculateError(ctx);
      PREPARE_TRANSFER(UpdatePositionHelicopter)

      MAKE_FINITE(_backRotorWanted)
      MAKE_FINITE(_mainRotorWanted)
      MAKE_FINITE(_cyclicForwardWanted)
      MAKE_FINITE(_cyclicAsideWanted)
      MAKE_FINITE(_bankWanted)
      MAKE_FINITE(_diveWanted)
      MAKE_FINITE(_pilotHeading)
      MAKE_FINITE(_pilotDive)

      ICALCERR_ABSDIF(float, backRotorWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, mainRotorWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, cyclicForwardWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, cyclicAsideWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, rotorDiveWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, bankWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, diveWanted, ERR_COEF_VALUE_MAJOR)

      ICALCERR_ABSDIF(float, pilotHeading, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, pilotDive, ERR_COEF_VALUE_MAJOR)
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  DoAssert(_finite(error));
  return error;
}

#if _HELISIM_ROTORLIB

//rtd constructor need unique ID
static int RtdHelicopterCounter = 0;

#if !_DUMMY_ROTORLIB
// ---------------------- RotorLib collisions ---------------------- //
class ArmaTerrainQueryCallback : public RTD::TerrainQueryCallback
{
public:

  ArmaTerrainQueryCallback( RTD::World* world):RTD::TerrainQueryCallback( world )
  {

  }

  /**
  The vehicle or the bot calls this method periodically with changing start and end vectors. These vectors
  define a linear intersector's start and end points in world coordinate system. Developers have to
  compute intersection of the terrain with this intersector line and pass the information back to
  the vehicle in hitXYZ, hitNormal, hitVelocity.

  Note that Simple3D is RTDynamics' 3D engine for demo purposes. In your applications you will need
  to use your own graphics system calls. Most graphics engines (Vega Prime, OSG, Performer etc... ) provide
  necessary functionality to make line intersections with terrain.
  */
  virtual bool query( const smVec3d& start, const smVec3d& end ,
    smVec3d& hitXYZ , smVec3d& hitNormal, smVec3d& hitVelocity,
    smReal& frictionFactor, bool& isLiquid)
  {
    PROFILE_SCOPE_EX(rtdCb, *);

    bool testingGroundEffect = end.z() < -500;
    smVec3r tmpXYZ;
    frictionFactor = 1;
 
    Vector3 lPos = Vector3(start[0],start[2],start[1]);
    Vector3 lDir = Vector3(end[0] - start[0],end[2] - start[2],end[1] - start[1]);
    float lDirSize = lDir.Size();
    Vector3 lDirN = lDir.Normalized();

    Vector3 tmpEnd;

    float tGround;  
    // --- UGLY HOTFIX --- RTD is unable to fix this in short period of time //
    //end.z == -1000 is vector used for ground effect -> we must test water
    //others are for land contacts - ignore water so we can sink
    if(testingGroundEffect)
    {
      tGround = GLandscape->IntersectWithGroundOrSea(&tmpEnd, lPos, lDirN, 0, lDirSize);
    }
    else tGround = GLandscape->IntersectWithGround(&tmpEnd, lPos, lDirN, 0, lDirSize);
    
    Vector3 tmpNormal = GLandscape->SurfaceNormal(tmpEnd);
    float tGround2 = Square(tGround);

    float MaxDist = floatMin(150,tGround);

#if _HELISIM_ROTORLIB_SIM_THREAD
    CollisionBuffer result(false);
#else
    CollisionBuffer result;
#endif

    GLandscape->ObjectCollisionLine(0, result, Landscape::FilterIgnoreOne(GWorld->CameraOn()), lPos, lPos + lDirN * MaxDist, 0/*0.5f*/, ObjIntersectGeom, ObjIntersectView);
    
    Vector3 speed = VZero;
    Object *hitObject = NULL;
    // select the nearest
    if (result.Size() > 0)
    {
      float tmp2;
      for (int i = 0; i < result.Size(); i++)
      {
        CollisionInfo &collInfo = result[i];
        if(!result[i].HitObject()) 
          continue;

        tmp2 = lPos.Distance2(collInfo.pos);
        if (tmp2 < tGround2) 
        {
          tmpEnd = collInfo.pos;
          tmpNormal = collInfo.dirOutNotNorm;
          if(!collInfo.entry) tmpNormal = -lDirN;
          tGround2 = tmp2;
          hitObject = result[i].HitObject();
        }
      }
      tGround = sqrtf(tGround2);
    }
    
    if (tGround <= lDirSize)
    {
      #if 0 // _ENABLE_CHEATS // hotfix - reduce ground effect (to be removed once RTD fixes the issue properly)
        if (testingGroundEffect)
        {
          // RTD seems to be adding 100 m to the position helicopter 
          static float full = 100.0f; // full ground effect
          static float none = 107.5f; // result 108 means almost no ground effect
          static float fullResult = 101.5f;
          static float geOffset = 1.0f;
          tGround += geOffset; // done to prevent extreme ground effect when very close to the ground
          if (tGround<none)
          {
            tGround = Interpolativ(tGround,full,none,fullResult,none);
          }
        }
      #endif
      // cannot use tmpEnd, as tGround may be artificially adjusted
      Vector3 ret = lPos+tGround*lDirN;
      hitXYZ.set(ret.X(),ret.Z(),ret.Y());
      hitNormal.set(tmpNormal.X(),tmpNormal.Z(),tmpNormal.Y());
      //if(hitObject)
      //{
      //  Vehicle *vehicle = dyn_cast<Vehicle>(hitObject);
      //  if(vehicle) speed = vehicle->FutureVisualState().Speed();
      //}
      hitVelocity.set( 0, 0, 0); //rtd - marine operations not licenced
      //hitVelocity.set( speed.X() , speed.Z() , speed.Y());  
      return true; // Since intersector hits the terrain return true.
    }
    return false; // If execution reaches here we do not have a hit. Return false.
  }
};
#endif

// ---------------------- Engine temperature ---------------------- //
EngineTemperature::EngineTemperature(bool airbourne)
{
  _temperature;
  _currentElementTemperature.Realloc(2);
  _currentElementTemperature.Resize(2);
  for(int i=0; i<_currentElementTemperature.Size(); i++) _currentElementTemperature[i] = 0;
}

//NOTE - this method has no physical background and it is just fake to get expected values
void EngineTemperature::Simulate(float deltaT, Ref<HelicopterRTD> helicopter, int engineIndex)
{
  //RPM - decrease temperature
  //throttle - increase temperature
  float scaleRPM = helicopter->GetMainRotorRPMRTDScaled(helicopter->FutureVisualState());
  saturate(scaleRPM, 0.0f,1.0f);

  //throttle - increase temperature
  float throttle = 0;
  float rpm = 0;

  float heating, increseSpeed, cooling, decreseSpeed;
  if(helicopter->IsRTDSimulated())
  {
    rpm = helicopter->GetRPM(engineIndex);
    throttle = (helicopter->GetFuel() > 0.001)? helicopter->GetThrottleRTD(engineIndex) : 0;
    if(throttle < 0.001 || rpm < 100)
    {
      heating =  helicopter->Type()->_temperatureProgressOff[0];
      increseSpeed =  helicopter->Type()->_temperatureProgressOff[1];
      cooling =  helicopter->Type()->_temperatureProgressOff[2];
      decreseSpeed =  helicopter->Type()->_temperatureProgressOff[3];
    }
    else if(throttle < 0.6) 
    {
      heating =  helicopter->Type()->_temperatureProgressIdle[0];
      increseSpeed =  helicopter->Type()->_temperatureProgressIdle[1];
      cooling =  helicopter->Type()->_temperatureProgressIdle[2];
      decreseSpeed =  helicopter->Type()->_temperatureProgressIdle[3];
    }
    else
    {
      heating =  helicopter->Type()->_temperatureProgressFull[0];
      increseSpeed =  helicopter->Type()->_temperatureProgressFull[1];
      cooling =  helicopter->Type()->_temperatureProgressFull[2];
      decreseSpeed =  helicopter->Type()->_temperatureProgressFull[3];
    }
  }
  else
  { //Arma model / AI
    if(helicopter->EngineIsOn())
    {
      rpm = 200;
      throttle = 1;
      heating =  helicopter->Type()->_temperatureProgressFull[0];
      increseSpeed =  helicopter->Type()->_temperatureProgressFull[1];
      //cannot ensure that AI will wait for engine cool down - instant increase engine colling effectiveness
      _currentElementTemperature[0] = floatMin(helicopter->Type()->_maxEngineTemperature-1, _currentElementTemperature[0]);
      _currentElementTemperature[1] = cooling =  helicopter->Type()->_temperatureProgressFull[2];
      decreseSpeed =  helicopter->Type()->_temperatureProgressFull[3];
    }
    else
    {
      rpm = 0;
      throttle = 0;
      heating =  cooling =  0;
      increseSpeed =  helicopter->Type()->_temperatureProgressOff[1];
      decreseSpeed =  helicopter->Type()->_temperatureProgressOff[3];
    }
  }

  //max values
  float maxHeating = helicopter->Type()->_temperatureProgressFull[0];
  float maxCooling = helicopter->Type()->_temperatureProgressFull[2];
 
  bool engineON = helicopter->EngineIsOn(engineIndex);
  //cooling
  if(_currentElementTemperature[1] > cooling && engineON)
  {
    _currentElementTemperature[1] += decreseSpeed * deltaT;
    //_currentElementTemperature[1] should be negative
    saturate(_currentElementTemperature[1], maxCooling, _currentElementTemperature[0]); 
  }
  else if(engineON)
  { //decrease cooling effect (slowly, rotor is spinning)
    _currentElementTemperature[1] -= decreseSpeed * deltaT;
    //_currentElementTemperature[1] should be negative
    saturate(_currentElementTemperature[1],  maxCooling, _currentElementTemperature[0]); 
  }
  else
  { //decrease cooling effect (fast, rotor is not spinning)
    _currentElementTemperature[1] -= 25 * decreseSpeed * deltaT;
    //_currentElementTemperature[1] should be negative
    saturate(_currentElementTemperature[1],  maxCooling, _currentElementTemperature[0]); 
  }

  //engine heating
  if(throttle < 0.001) // throttle == 0 OFF
  {
    //slowly decrease temperature, if throttle is off
    if(engineON) _currentElementTemperature[0] += deltaT * increseSpeed; 
    else _currentElementTemperature[0] += deltaT * 0.1f * increseSpeed; //cool down much slower, when low RPM
    saturateMax(_currentElementTemperature[0], 0); 
  }
  else 
  {// throttle == FULL or IDLE
    if(_currentElementTemperature[0] > heating + _currentElementTemperature[1])
    { //decrease temperature
      _currentElementTemperature[0] += deltaT * decreseSpeed; //cooling based on RPM
      saturateMax(_currentElementTemperature[0], 0); 
    }
    else
    { //increase temperature
      _currentElementTemperature[0] += deltaT * increseSpeed;
      saturate(_currentElementTemperature[0], 0, maxHeating); 
    } 
  }

  _temperature = floatMax(0, _currentElementTemperature[0]);
}

void EngineTemperature::SetHot(Ref<HelicopterRTD> helicopter, int engineIndex)
{
  //set cooling to max
  float cooling =  helicopter->Type()->_temperatureProgressFull[2];
  _currentElementTemperature[1] = cooling;
  _currentElementTemperature[0] = floatMin(helicopter->Type()->_maxEngineTemperature-1, _currentElementTemperature[0]);
  _temperature = floatMax(0, _currentElementTemperature[0]);
}


float EngineTemperature::GetTemperature() const
{
  return _temperature;
}

LSError EngineTemperature::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeArray("currentElementTemperature", _currentElementTemperature, 1))
  CHECK(ar.Serialize("temperature", _temperature, 1, 0.0))

   return LSOK;
}

// ---------------------- RotorLib helicopter ---------------------- //
DEFINE_CASTING(HelicopterRTD)

HelicopterRTD::HelicopterRTD( const EntityAIType *name, Person *pilot, bool fullCreate)
:HelicopterAuto(name,pilot,fullCreate),
_rtdStateAge(0.0f),
_cyclicAsideAnimation(0),
_cyclicForwardAnimation(0)
{
#if !_DUMMY_ROTORLIB
  // World class contains the environment conditions like gravity and wind.
  RTD::World *rtdWorld = GWorld->GetRTDWorld();
#endif
  _autoTrimEnabled = false;
  _autoTrimEnabledState = false;
  _controllsTrim = false;
  _isAutoStartUpEnabledRTD = true;

  _cyclicSideRTDOffset = 0;
  _cyclicForwardRTDOffset = 0;
  _backRotorRTDOffset = 0;
  _delayAfterTrimming = TIME_MIN;

  _cyclicSideRTD = 0;
  _cyclicForwardRTD = 0; 
  _pedals = 0; 

  _starter = _starterWanted = 0;
  _starterTime = VZero;
  _APU = 0.0f;
  _batteryOn = false;
  _batteryCharge = Type()->_maxBatteryCharge;
  _isRTDGearBrakeAble = false;

  //APU starting
  _APUStartingTime = 0.0f;
  _APUwanted = 0.0f;
  _APUchange = 0.0f;

  _rudderWanted = 0;
  _collectiveWanted = 0;
  _actualCollectiveRTD = 0.0f;

  _lastRotorColAngle = 0.0f;

  _crewWeight = _fuselageWeight = _customWeight = _ammoWeight = 0;
  _isRTDSimulated = false;
  _isRTDSimulatedWanted = false;
  _timeToForcedChange = Glob.time;

  _centerOffsetRTD = Type()->_centerOffsetRTD;

  _cyclicAsideAnimation = _cyclicForwardAnimation = 0;


#if !_DUMMY_ROTORLIB
  _autoHoverRTD = NULL;
  // Geographic coordinates of the Munich Airport. Select this position close
  // to your area of interest to minimize floating point errors.
  smVec3d llaMunichAirport (0.0f, 0.0f, 0.0f);

  if(Type()->_loadedRTD)
  {
    _enabledRTDSimulation = true;
    char heliName[32];
    sprintf(heliName, "HeliRTD%d", RtdHelicopterCounter++);		

    //char curDirTmp[4096]; //hopefully long enough
    //RString curDir =  "C:\\DataO\\HeliSim\\HelisimCfg\\Example-GenHeli500-DesktopSim.xml";
    // Create the helicopter dynamics model using an xml configuration file.
    _rtdHelicopter = new GenHeli600Arma( rtdWorld, this,  heliName, &Type()->_configRTD);
    //_rtdHelicopter = new GenHeli600Arma( rtdWorld,  heliName, curDir.Data() /* &Type()->_configRTD*/);
    _rtdHelicopter->reset();
    if(!_rtdHelicopter->isVehicleCorrectlyConfigured() || _rtdHelicopter->getMainRotor() == NULL || _rtdHelicopter->getTailRotor() == NULL )
    {
      _rtdHelicopter->remove();
      _rtdHelicopter = NULL;
    }
    else
    {

      // Set the initial position of the helicopter slightly away from the airport.
      // Note that the LLA difference to the Local Coordinate System center shall be really small
      // and needs compensation in other axes since the earth is not flat.
      // for example: smVec3d( 0.0, -0.00001, 15)
      smVec3d initLLAPosition = llaMunichAirport + smVec3d( 0.0, 0, 0);

      // We can also specify an xyz offset.
      Vector3 centerOffsetRTDRotated = FutureVisualState().Transform().Rotate(_centerOffsetRTD);
      smVec3r positionOffset(0.0f + centerOffsetRTDRotated.X(), 0.0f + centerOffsetRTDRotated.Z(), 0.0f + centerOffsetRTDRotated.Y());
      _rtdHelicopter->setPosition(positionOffset);

      // If required,set the initial rotation and velocities.
      smPositionVec initVel( 0.0f , 0.0f , 0.0f );
      smAngleDVec initRot(0.0f,0.0f,0.0f );

      _rtdHelicopter->setRotation( initRot );
      _rtdHelicopter->setVelocity( initVel ) ;
      // Set initial collective
      _rtdHelicopter->getPilotInputData()->m_collective = 0.0f ;

      _rtdHelicopter->setRotation( initRot );
      _rtdHelicopter->setVelocity( initVel ) ;
      // Set initial collective
      _rtdHelicopter->getPilotInputData()->m_collective = 0.0f ;

      _rtdHelicopter->setTerrainQueryCallback( new ArmaTerrainQueryCallback( rtdWorld) );
      _rtdHelicopter->EngineOff();

      _fuselageWeight = _rtdHelicopter->getMBDBody().getMass();
      _rtdHelicopter->setEnabled(_isRTDSimulated);


      //set auto hover correction (for some helicopters, angle pitch roll == 0 is not stable)
      if(_rtdHelicopter->getNumCSComponents() > 0)
      {
        //set autohover
        if(strcmpi(_rtdHelicopter->getCSComponent(0)->getType().data(), "FCSAutoHoverTOH") == 0)
        {
          FCSAutoHoverTOH* autoHoverRTD = (FCSAutoHoverTOH*)_rtdHelicopter->getCSComponent(0);
          if ( autoHoverRTD )
          {
            autoHoverRTD->enable(false);
            autoHoverRTD->setCorrection(Type()->_autoHoverCorrection.X(),Type()->_autoHoverCorrection.Y());
            _autoHoverRTD = autoHoverRTD;
          }
        }

        //set auto trim (true/false is based on difficulty)
        SetAutoTrim(false);
        int countContr = intMin(2, _rtdHelicopter->getNumCSComponents());
        for(int i = 0; i< countContr; i++)
        {
          if(strcmpi(_rtdHelicopter->getCSComponent(i)->getType().data(), "FCSHelicopterAutoTrim100") == 0)
          { 
            RotorLibFDM::FCSHelicopterAutoTrim100* autoTrimRTD = (RotorLibFDM::FCSHelicopterAutoTrim100*)_rtdHelicopter->getCSComponent(i);
            if ( autoTrimRTD ) _autoTrimEnabledState = autoTrimRTD->isEnabled();
            break;
          }
        }
      }
      ResetHelicopterRTD();
    }
  }
  else 
#endif
  {
    _enabledRTDSimulation = false;
    _rtdHelicopter = NULL;
  }

  for(int i = 0; i< _hit.Size(); i++) _lastHit.Add(0);

  memset(&_rpmStarter[0], 0, sizeof(float) * lenof(_rpmStarter));
  memset(&_rpmStarterOffTime[0], 0, sizeof(float) * lenof(_rpmStarterOffTime));
}

void HelicopterRTD::Init( Matrix4Par pos, bool init )
{
  base::Init(pos, init);
  _rtdStateAge = 0;
#if !_DUMMY_ROTORLIB
  GWorld->GetRTDWorld()->ClearAccumulatedSimulationTime();
  if(_rtdHelicopter)
  {
    //only for init, if deserialized, position is already synchronized and rtd::setposition sets velocity to 0 
    if(init) CopyTransformArmaToRTD();
  
    for(int i =0; i< _rtdHelicopter->GetNumberOfEngines(); i++)
    {
      _enginesTemperature.Add(EngineTemperature(false));
    }

    for(int i =0; i< _rtdHelicopter->getNumGroundContacts(); i++)
    {
      if(_rtdHelicopter->getGroundContact(i)->isBrakable() == true)
      {
        _isRTDGearBrakeAble = true;
        break;
      }
    }
  }
#endif
}

// -- network --

#define HELICOPTER_RTD_MSG_LIST(XX) \
  XX(Create, CreateHelicopterRTD) \
  XX(UpdateGeneric, UpdateHelicopterRTD) \
  XX(UpdatePosition, UpdatePositionHelicopterRTD)

DEFINE_NETWORK_OBJECT(HelicopterRTD, HelicopterAuto, HELICOPTER_RTD_MSG_LIST)

#define CREATE_HELICOPTER_RTD_MSG(MessageName, XX) 

DECLARE_NET_INDICES_EX(CreateHelicopterRTD, CreateHelicopter, CREATE_HELICOPTER_RTD_MSG)
DEFINE_NET_INDICES_EX(CreateHelicopterRTD, CreateHelicopter, CREATE_HELICOPTER_RTD_MSG)

#define UPDATE_HELICOPTER_RTD_MSG(MessageName, XX) \
  XX(MessageName, int, starterWanted, NDTInteger, int, NCTNone, DEFVALUE(int, 0), DOC_MSG("starters on/off toggle"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_NORMAL) \
  XX(MessageName, bool, batteryOn, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("batteryOn on/off toggle"), TRANSF, ET_NOT_EQUAL, ERR_COEF_VALUE_NORMAL) \
  XX(MessageName, float, batteryCharge, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("battery Charge"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_NORMAL) \
  XX(MessageName, float, APU, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("APU state"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_NORMAL) \
  XX(MessageName, float, customWeight, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("custom weight"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_NORMAL) \

DECLARE_NET_INDICES_EX_ERR(UpdateHelicopterRTD, UpdateHelicopter, UPDATE_HELICOPTER_RTD_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateHelicopterRTD, UpdateHelicopter, UPDATE_HELICOPTER_RTD_MSG, NoErrorInitialFunc)

#define UPDATE_POSITION_HELICOPTER_RTD_MSG(MessageName, XX) \
  XX(MessageName, float, actualCollectiveRTD, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("collective"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_NORMAL) \
  XX(MessageName, float, rudderWanted, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("rudder"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_NORMAL) \
  XX(MessageName, float, cyclicForwardAnimation, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("cyclicForward"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_NORMAL) \
  XX(MessageName, float, cyclicAsideAnimation, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("cyclicAside"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_NORMAL) \


DECLARE_NET_INDICES_EX_ERR(UpdatePositionHelicopterRTD, UpdatePositionHelicopter, UPDATE_POSITION_HELICOPTER_RTD_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionHelicopterRTD, UpdatePositionHelicopter, UPDATE_POSITION_HELICOPTER_RTD_MSG, NoErrorInitialFunc)

NetworkMessageFormat &HelicopterRTD::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCCreate:
    base::CreateFormat(cls, format);
    CREATE_HELICOPTER_RTD_MSG(CreateHelicopterRTD, MSG_FORMAT)
      break;
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_HELICOPTER_RTD_MSG(UpdateHelicopterRTD, MSG_FORMAT_ERR)
      break;
  case NMCUpdatePosition:
    base::CreateFormat(cls, format);
    UPDATE_POSITION_HELICOPTER_RTD_MSG(UpdatePositionHelicopterRTD, MSG_FORMAT_ERR)
      break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

HelicopterRTD *HelicopterRTD::CreateObject(NetworkMessageContext &ctx)
{
  Entity *veh = Entity::CreateObject(ctx);
  HelicopterRTD *heli = dyn_cast<HelicopterRTD>(veh);
  if (!heli) return NULL;
  heli->TransferMsg(ctx);
  return heli;
}

TMError HelicopterRTD::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    if (ctx.IsSending())
    {
      TMCHECK(base::TransferMsg(ctx))
    }
    {
      PREPARE_TRANSFER(CreateHelicopterRTD)
    }
    break;
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
        PREPARE_TRANSFER(UpdateHelicopterRTD)

        TRANSF(starterWanted)
        TRANSF(batteryOn)
        TRANSF(APU)
        TRANSF(customWeight)
        TRANSF(batteryCharge)

    }
    break;
  case NMCUpdatePosition:
    {
      PREPARE_TRANSFER(UpdatePositionHelicopterRTD)
      TMCHECK(base::TransferMsg(ctx))

      TRANSF(rudderWanted)
      TRANSF(actualCollectiveRTD)
      TRANSF(cyclicForwardAnimation)
      TRANSF(cyclicAsideAnimation)

    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float HelicopterRTD::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += base::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdateHelicopterRTD)

        ICALCERR_NEQ(int, starterWanted, ERR_COEF_VALUE_NORMAL)
        ICALCERR_NEQ(bool, batteryOn, ERR_COEF_VALUE_NORMAL)

        ICALCERR_ABSDIF(float, APU, ERR_COEF_VALUE_NORMAL)
        ICALCERR_ABSDIF(float, customWeight, ERR_COEF_VALUE_NORMAL)
        ICALCERR_ABSDIF(float, batteryCharge, ERR_COEF_VALUE_NORMAL)

    }
    break;
  case NMCUpdatePosition:
    {
      error +=  base::CalculateError(ctx);
      PREPARE_TRANSFER(UpdatePositionHelicopterRTD)

      ICALCERR_ABSDIF(float, rudderWanted, ERR_COEF_VALUE_NORMAL)
      ICALCERR_ABSDIF(float, actualCollectiveRTD, ERR_COEF_VALUE_NORMAL)
      ICALCERR_ABSDIF(float,cyclicForwardAnimation, ERR_COEF_VALUE_NORMAL)
      ICALCERR_ABSDIF(float,cyclicAsideAnimation, ERR_COEF_VALUE_NORMAL)

    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  DoAssert(_finite(error));
  return error;
}

// -- simulation --

float HelicopterRTD::FuelLeakRate() const
{
  if(Type()->_hitFuel < 0) return 0.0;
  //self-sealing
  float capacity = GetType()->GetFuelCapacity();
  float fuelScale =  FutureVisualState().GetFuel() / capacity;
  if(fuelScale < (1 - _hit[Type()->_hitFuel])) return 0.0;
  // typical heli fuel is about 1000 l
  // we want this to last about 30 sec with 0.8 damage
  const float factor = capacity/80/0.64f;
  return Square(GetHitCont(Type()->_hitFuel))*factor;
}


#define INCREASE_STRESS_DAMAGE(hit, stress, maxStress) \
  {if(stress > maxStress) \
  hit = floatMin(1, hit + stressDamagePerSec * Square(stress / maxStress) *deltaT);}


#define HIDE_SIMULATION_PEEKS   5

void HelicopterRTD::SimulateDamage(float deltaT, SimulationImportance prec)
{
  bool stressDamage = 
    (Glob.config.StressDamageEnabled() && 
    _isRTDSimulated && GWorld->IsStressDamageEnabled() &&
    Glob.time > _timeToForcedChange + HIDE_SIMULATION_PEEKS); // there might some peaks when simulation is changed - 5 sec protection 

  const HelicopterType* type = Type();

  //--  BATTERY DAMAGE AND CHARGE --//
  float batteryDamage = (type->_hitBatteries >= 0)? 1 - _hit[type->_hitBatteries] : 1;
  float stressDamagePerSec = type->_stressDamagePerSec;

  //damage decrease max capacity and recharge speed
  if (_batteryOn) _batteryCharge -= type->_electricityBatteryDrain * deltaT;
  if (_APU <= 0 && (_starter > 0)) _batteryCharge -= _batteryOn ? type->_starterBatteryDrain * deltaT : 0;
#if !_DUMMY_ROTORLIB
  float rpmChargeScale = _rtdHelicopter ? _rtdHelicopter->GetAverageRPMScaled(): FutureVisualState()._rotorSpeed;
#else
  float rpmChargeScale = FutureVisualState()._rotorSpeed;
#endif
  if(_APU > 0.25) rpmChargeScale = 1;
  _batteryCharge += batteryDamage * type->_engineBatteryRecharge * deltaT * rpmChargeScale;

  if(IsElectricity() == false && _APUwanted > 0) SetAPU(0,2);

  saturate(_batteryCharge, 0, batteryDamage * type->_maxBatteryCharge);

  //lights
  //if(Type()->_hitLight >= 0 && _hit[Type()->_hitLight] < 0.9f)
  //{

  //}
  //if(_batteryCharge <= 0.001)
  //{

  //}
  //--  TAIL --//
  //if tail is destroyed, tail rotor and wings are destroyed as well
  if (type->_hitTail >= 0 && _hit[type->_hitTail] > 0.98f)
  {
    if(type->_hitHStabilizerL1 >= 0)  _hit[type->_hitHStabilizerL1] = 1.0f;
    if(type->_hitHStabilizerR1 >= 0)  _hit[type->_hitHStabilizerR1] = 1.0f;
    if(type->_hitVStabilizer1 >= 0)  _hit[type->_hitVStabilizer1] = 1.0f;
    if(type->_rotorVHit >= 0)       _hit[type->_rotorVHit]      = 1.0f;
  }

#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) 
  {
    //-- WRECK --//
    if(_isDead) _rtdHelicopter->setEnabled(false);


    //--  ENGINE --//
    _rtdHelicopter->SetEnginesDamage(
      type->_engineHit >=  0 ? 1 - _hit[type->_engineHit]  : 1,
      type->_hitEngine2 >= 0 ? 1 - _hit[type->_hitEngine2] : 1,
      type->_hitEngine3 >= 0 ? 1 - _hit[type->_hitEngine3] : 1
      );

    //-- STARTERS --//
    if(stressDamage)
    {
      if(type->_hitStarter1 >= 0)
      {
        if(_starter & 1) _starterTime[0] += deltaT;
        else _starterTime[0] -= 2 * deltaT;
        if (_starterTime[0] > 60 && _hit[type->_hitStarter1] < 0.5)  
        {
          _hit[type->_hitStarter1] = 1.0;
          PlayStarterSound(false, 0);
        }
      }

      if(type->_hitStarter2 >= 0)
      {
        if(_starter & 2) _starterTime[1] += deltaT;
        else _starterTime[1] -= 2 * deltaT;
        if (_starterTime[1] > 60 && _hit[type->_hitStarter2] < 0.5) 
        {
          _hit[type->_hitStarter2] = 1.0;
          PlayStarterSound(false, 1);
        }
      }

      if(type->_hitStarter3 >= 0)
      {
        if(_starter & 4) _starterTime[2] += deltaT;
        else _starterTime[2] -= 2 * deltaT;
        if (_starterTime[2] > 60 && _hit[type->_hitStarter3] < 0.5)  
        {
          _hit[type->_hitStarter3] = 1.0;
          PlayStarterSound(false, 2);
        }
      }
      SaturateMax(_starterTime, VZero);
    }

    //--  WING DAMAGE AND STRESS --//
    AutoArray<float> forces;
    _rtdHelicopter->GetWingsMaxForcesSize(forces);

    if(_rtdHelicopter->isEnabled())
    {
      //LEFT
      if(type->_hitHStabilizerL1 >= 0 && forces.Size() > 0)
      {
        if(stressDamage) INCREASE_STRESS_DAMAGE(_hit[type->_hitHStabilizerL1], forces[0], type->_maxHorizontalStabilizerLeftStress);
        if(fabs(_hit[type->_hitHStabilizerL1] - _lastHit[type->_hitHStabilizerL1]) > 0.0001f)
        {
          _rtdHelicopter->SetWingDamage(1-_hit[type->_hitHStabilizerL1],0);
          _lastHit[type->_hitHStabilizerL1] = _hit[type->_hitHStabilizerL1];
        }
      }
      //RIGHT
      if(type->_hitHStabilizerR1 >= 0&& forces.Size() > 1)
      {
        if(stressDamage) INCREASE_STRESS_DAMAGE(_hit[type->_hitHStabilizerR1], forces[1],type->_maxHorizontalStabilizerRightStress); 
        if(fabs(_hit[type->_hitHStabilizerR1] - _lastHit[type->_hitHStabilizerR1]) > 0.0001f)
        {
          _rtdHelicopter->SetWingDamage(1-_hit[type->_hitHStabilizerR1],1);
          _lastHit[type->_hitHStabilizerR1] = _hit[type->_hitHStabilizerR1];
        }
      }
      //VERTICAL
      if(type->_hitVStabilizer1 >= 0 && forces.Size() > 2 )
      {
        if(stressDamage) INCREASE_STRESS_DAMAGE(_hit[type->_hitVStabilizer1], forces[2], type->_maxHorizontalStabilizerLeftStress);
        if(fabs(_hit[type->_hitVStabilizer1] - _lastHit[type->_hitVStabilizer1]) > 0.0001f)
        {
          _rtdHelicopter->SetWingDamage(1-_hit[type->_hitVStabilizer1],2);
          _lastHit[type->_hitVStabilizer1] = _hit[type->_hitVStabilizer1];
        }
      }
    }

    //--  ROTORS DAMAGE AND STRESS --//
    if (FutureVisualState()._rotorSpeed > 0.1 && type->_rotorHHit >= 0 && _hit[type->_rotorHHit] < 0.999f)
    {//main rotor collision - only for player

      smVec3r rotorPosition = _rtdHelicopter->getMainRotor()->getMountPosition();
      float angleBeg = 0.69813170f  * _lastRotorColAngle ; //(test nine angles)
      float angleEnd = angleBeg + 2.09439510f; 

      float radius =  _rtdHelicopter->GetMainBladeRadius();
      // Vector3 centerOffsetRTDRotated = FutureVisualState().Transform().Rotate(_centerOffsetRTD);
      Vector3 center = Vector3(rotorPosition[0],rotorPosition[2],rotorPosition[1]) + _centerOffsetRTD;
      Vector3 beg = Vector3( cos(angleBeg) * radius,0, sin(angleBeg) * radius) + center;
      Vector3 end = Vector3( cos(angleEnd) * radius,0, sin(angleEnd) * radius) + center;

      beg = FutureVisualState().PositionModelToWorld(beg);
      end = FutureVisualState().PositionModelToWorld(end);
      _lastRotorColAngle = (_lastRotorColAngle + 1) % 9;

      Vector3 tmpEnd;
      Vector3 lDirN = end - beg;
      float tGround = GLandscape->IntersectWithGround(&tmpEnd, beg, lDirN.Normalized(), 0, 1.1*radius);

      if (tGround < radius)
      {
        //ground collision
        SetHitContRaw(type->_rotorHHit, 1.0);
        PlayRotorDamageSound();
        ShowDamage(type->_rotorHHit, this);
      }
      else
      {
        CollisionBuffer result;
        GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(), result, Landscape::FilterIgnoreOne(this), beg, end, 0/*0.25f*/, ObjIntersectGeom, ObjIntersectView);
        //object collision
        for(int i = 0; i< result.Size(); i++)
        {
          if(dyn_cast<Parachute>(result[i].HitObject())) continue;

          SetHitContRaw(type->_rotorHHit, 1.0);
          PlayRotorDamageSound();
          ShowDamage(type->_rotorHHit, this);
          break;
        }
      }
    }

    if(_rtdHelicopter->isEnabled())
    {
      //--  TRANSMISSION STRESS --//
      float torque = _rtdHelicopter->GetDeliveredTorque();
      if(type->_hitTransmission >= 0 && _rtdHelicopter->isEnabled())
      {
        if(stressDamage) INCREASE_STRESS_DAMAGE(_hit[type->_hitTransmission], torque, type->_maxTorque);
        if(fabs(_hit[type->_hitTransmission] - _lastHit[type->_hitTransmission]) > 0.0001f)
        {
          _rtdHelicopter->SetTransmissionDamage(1-_hit[type->_hitTransmission]);
          _lastHit[type->_hitTransmission] = _hit[type->_hitTransmission];
        }
      }

      Vector3 rotorForces = _rtdHelicopter->GetRotorsForcesSize();
      //MAIN
      if(type->_rotorHHit >= 0 && _rtdHelicopter->isEnabled())
      {
        if(stressDamage) INCREASE_STRESS_DAMAGE(_hit[type->_rotorHHit], rotorForces[0], type->_maxMainRotorStress);
        if(fabs(_hit[type->_rotorHHit] - _lastHit[type->_rotorHHit]) > 0.0001f)
        {
          _rtdHelicopter->SetMainRotorDamage(1-_hit[type->_rotorHHit]);
          _lastHit[type->_rotorHHit] = _hit[type->_rotorHHit];
        }
      }
      //TAIL
      if(type->_rotorVHit >= 0 && _rtdHelicopter->isEnabled())
      {
        if(stressDamage) INCREASE_STRESS_DAMAGE(_hit[type->_rotorVHit], rotorForces[1], type->_maxTailRotorStress);
        if(fabs(_hit[type->_rotorVHit] - _lastHit[type->_rotorVHit]) > 0.0001f)
        {
          _rtdHelicopter->SetTailRotorDamage(1-_hit[type->_rotorVHit]);
          _lastHit[type->_rotorVHit] = _hit[type->_rotorVHit];
        }
      }

      //PILOT TUBE
      if(type->_hitPitotTube >= 0)
      {
        if(fabs(_hit[type->_hitPitotTube] - _lastHit[type->_hitPitotTube]) > 0.0001f) 
        {
          RTD::InsGenAirspeedIndicator100* asi = _rtdHelicopter->getAirspeedIndicator();
          asi->setBlockfactor(_hit[type->_hitPitotTube]);
          _lastHit[type->_hitPitotTube] = _hit[type->_hitPitotTube];
        }
      }

      //STATIC PORT
      if(type->_hitStaticPort >= 0)
      {
        if(fabs(_hit[type->_hitStaticPort] - _lastHit[type->_hitStaticPort]) > 0.0001f) 
        {
          RTD::InsGenVerticalSpeedIndicator100* vsi = _rtdHelicopter->getVSI();
          vsi->setStaticPortBlockFactor(_hit[type->_hitStaticPort]);
          _lastHit[type->_hitStaticPort] = _hit[type->_hitStaticPort];
        }
      }
    }
  }
#endif

  //engine temperature
  {
    for(int i = 0; i< _enginesTemperature.Size(); i++) _enginesTemperature[i].Simulate(deltaT, this, i);

    if(stressDamage)
    {
      if(_enginesTemperature.Size() > 0 && _enginesTemperature[0].GetTemperature() > Type()->_maxEngineTemperature * 1.1f) 
          _hit[type->_engineHit] = 1.0;
      if(_enginesTemperature.Size() > 1 && _enginesTemperature[1].GetTemperature() > Type()->_maxEngineTemperature * 1.1f) 
        _hit[type->_hitEngine2] = 1.0;
      if(_enginesTemperature.Size() > 2 && _enginesTemperature[2].GetTemperature() > Type()->_maxEngineTemperature * 1.1f) 
        _hit[type->_hitEngine3] = 1.0;
    }
  }
}

void HelicopterRTD::CheckPersonRotorCollision(Object *object)            
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _rtdHelicopter->getTailRotor() && FutureVisualState()._rotorSpeed > 0.1f)
  {
    smVec3r rotorPosition = _rtdHelicopter->getTailRotor()->getMountPosition();
    Vector3 center = Vector3(rotorPosition[0],rotorPosition[2],rotorPosition[1]) + _centerOffsetRTD;
    //float radius =  _rtdHelicopter->GetTailBladeRadius();
    Vector3 personPosition = FutureVisualState().PositionWorldToModel(object->AimingPosition(object->RenderVisualState()));

    //1m is for soldier size, 1m is for blade size
    if(personPosition.Distance2(center) < 2) object->SetDamage(1);
  }
#endif
}

void HelicopterRTD::Simulate(float deltaT, SimulationImportance prec)
{
  //simulate damage of helicopter parts
  if(IsLocal()) SimulateDamage(deltaT, prec);
  UpdateStarter();

  if(IsLocal())
  { //AI is piloting
    float delta=_cyclicAsideWanted-_cyclicAsideAnimation;
    Limit(delta,-0.45f*deltaT, 0.45f*deltaT);
    _cyclicAsideAnimation += delta; 
    saturate(_cyclicAsideAnimation, -1,1);

    delta=_cyclicForwardWanted-_cyclicForwardAnimation;
    Limit(delta,-0.45f*deltaT, 0.45f*deltaT);
    _cyclicForwardAnimation += delta; 
    saturate(_cyclicForwardAnimation, -1,1);
  }

#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && !_isDead )
  {
    if(((PilotUnit() && PilotUnit()->IsPlayer()) || (CopilotUnit() && CopilotUnit()->IsPlayer())) && IsLocal())
    {
////////////////////////////////////////////////////////////
//////// HELICOPTERAUTO SIMULATION - LITE START ////////////

      if(!_isRTDSimulated) OnSimulationChanged(true);
      //if simulation cannot be switched, use ARMA model
      if(!_isRTDSimulated) 
      {
        base::Simulate(deltaT, prec);
        _rtdHelicopter->Simulate(deltaT, this, true , 255, 1.0, true);
      }
      else
      {
        PROFILE_SCOPE_EX(simHe,sim);

      RTD::WindsSystem* windSystem =_rtdHelicopter->getWindsSystem();
      if(windSystem)
      {
        windSystem->getMeanWind()->setWindVelocityInWCS( GWorld->GetRTDWorld()->GetWindSpeed());
      }

      
      //if pilot or copilot is alive
      bool driverAlive = 
        (_driver && _driver->Brain() && _driver->Brain()->LSIsAlive()) ||
        (_copilot && _copilot->Brain() && _copilot->Brain()->LSIsAlive());

      if( !driverAlive) EngineOff();

      if ((!_driver || _driver->IsDamageDestroyed()) && (!_copilot || _copilot->IsDamageDestroyed()))
      {
        if (!_landContact)
        {
          _cyclicAsideWanted=-0.1;
          _cyclicForwardWanted=0.1;
          _backRotorWanted=-0.1;
          _mainRotorWanted=0.1;
        }
        else
        {
          _cyclicAsideWanted=0;
          _cyclicForwardWanted=0;
          _backRotorWanted=0;
          _mainRotorWanted=0;
        }
      }
      // perform advanced simulation
      MoveWeapons(deltaT);
      //base::Simulate(deltaT,prec);


//////// HELICOPTERAUTO SIMULATION - LITE END   ////////////
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
//////// HELICOPTER SIMULATION - LITE START ////////////////

      _isDead = IsDamageDestroyed();

      // calculate new position
      VisualState moveTrans = PredictPos<VisualState>(deltaT);

      ApplyRemoteState(deltaT,moveTrans);

      if( FutureVisualState()._rotorSpeed>0 && _rtdHelicopter->GetMaxThrottle() > 0.0001f)
        ConsumeFuel(deltaT*Type()->GetFuelConsumptionRate());

      ConsumeFuel(deltaT*FuelLeakRate());

      float delta;
      if( _isDead )
      {
        if( _landContact || _waterContact || _objectContact )
          StopRotor();
      }
      if( FutureVisualState()._fuel<=0 )
      {
        // warn about engine malfunction
        if (_rotorSpeedWanted>0 && !IsDamageDestroyed())
        {
          _showDmg = true;
          _showDmgValid = Glob.time + 5.0f;
        }
        StopRotor();
      }

      bool doServo=false;

      // change gears
      float gearWanted = 1-GetPilotGear();
      //if( _gearDamage ) gearWanted=-0.66f;
      if (!Type()->_gearRetracting)
        gearWanted = 0;
      float oldGears = FutureVisualState()._gearsUp;
      delta = gearWanted - oldGears;
      if( fabs(delta)>0.01 )
        doServo=true;

      Limit(delta,-Type()->_gearDownSpeed * deltaT,+Type()->_gearUpSpeed * deltaT);
      FutureVisualState()._gearsUp+=delta; 
      Limit(FutureVisualState()._gearsUp,-1,1);

      const float tholdUp = 0.95f;
      const float tholdDown = 0.05f;
      float gears = FutureVisualState()._gearsUp;
      if (gears >= tholdUp && oldGears < tholdUp) OnEvent(EEGear,false);
      if (gears <= tholdDown && oldGears > tholdDown) OnEvent(EEGear,true);

      // wake up the simulation as needed
      if (FutureVisualState()._rotorSpeed>0 || FutureVisualState()._speed.SquareSize() > 0.1f || _angVelocity.SquareSize() > 0.1f)
        OnMoved();


      if( !GetStopped() && !CheckPredictionFrozen())
      {
//        saturateMin(_rotorSpeedWanted,1-GetHit(Type()->_engineHit));    


        if (IsLocal() && FutureVisualState()._rotorSpeed>0.1f && (_landContact || _objectContact || _waterContact) && FutureVisualState().DirectionUp()[1] < -0.7f)
        {
          // landed upside down - we need to explode
          float damage = 10.0f;
          LocalDamageMyself(VZero,damage*deltaT,GetRadius());
          _isDead = true;
        }

        //if( _isDead )
        //  // dirty hack - when heli is destroyed, we want it to fall down
        //  rotorSpeed *= 0.1;


        // recalculate COM to reflect change of position
        // wCenter.SetFastTransform(moveTrans.ModelToWorld(),GetCenterOfMass());
        if( deltaT>0 )
        {
          bool wasLandContact = _landContact;
          _objectContact=false;
          _landContact=false;
          _waterContact=false;

          Vector3 totForce(VZero);

          float crash=0;
          Vector3 crashPos;

          if( /*prec<=SimulateVisibleFar &&*/ IsLocal())
          {
            CollisionBuffer collision;
            GLOB_LAND->ObjectCollision(collision,this,moveTrans);
#define MAX_IN 0.4
#define MAX_IN_FORCE 0.1
#define MAX_IN_FRICTION 0.4

            for( int i=0; i<collision.Size(); i++ )
            {
              // info.pos is relative to object
              CollisionInfo &info=collision[i];
              if (info.object && !info.object->IsPassable())
              {
                _objectContact=true;
                float cFactor=1;
                if( info.object->GetMass()<60 )
                  continue;
                if (dyn_cast<Person, Object>(info.object))
                {
                  CheckPersonRotorCollision(info.object);
                  continue; // avoid collision with person
                }
                if( info.object->GetType()==Primary )
                  cFactor=1;
                else
                {
                  cFactor=info.object->GetMass()*GetInvMass();
                  saturate(cFactor,0,5);
                }

                if( cFactor>0.05 )
                {
                  Vector3Val pos=info.pos;
                  Vector3Val dirOut=info.dirOut;
                  // create a force pushing "out" of the collision
                  float forceIn=floatMin(info.under,MAX_IN_FORCE);
                  Vector3 pForce=dirOut*GetMass()*40*forceIn;
                  // apply proportional part of force in place of impact

                  Vector3Val objSpeed=info.object->ObjectSpeed();
                  Vector3 colSpeed=FutureVisualState()._speed-objSpeed;

                 //  if info.under is bigger than MAX_IN, move out
                  if( info.under>MAX_IN )
                  {
                    Vector3 newPos=moveTrans.Position();
                    float moveOut=info.under-MAX_IN;
                    if( moveOut>0.1 ) 
                    {
                      float newCrash = 0.1 * Type()->GetArmor();
                      if (crash < newCrash)
                      {
                        crash = newCrash;
                        crashPos = pos;
                      }
                    }
                    newPos+=dirOut*moveOut*0.1;
                    moveTrans.SetPosition(newPos);
                  }

                  float newCrash;
                  if (moveTrans.DirectionUp()[1] > 0)
                  { 
                    if(Glob.config.RoughLandingEnabled())
                    {
                      newCrash = (floatMax(colSpeed.SquareSizeXZ() - 75 ,0) + floatMax(colSpeed.Y()*colSpeed.Y() - 500,0)) * 2.0f;;
                    }
                    else
                    {
                      float skidDamage = 0; 
                      if (Type()->_hitGear >= 0)
                      {
                        skidDamage = (1 - _hit[Type()->_hitGear]) * 4; //skid damage can take up to 4m/s 
                        float skidCrash = (colSpeed.Y() - 25.0f) / 16;
                        saturate(skidCrash,0,1);
                        if(skidCrash > 0.05)
                        {
                          SetHitContRaw(Type()->_hitGear, skidCrash + _hit[Type()->_hitGear]);
                          ShowDamage(Type()->_hitGear, this);
                        }
                      }
                      newCrash = (colSpeed.SquareSize()- pow(5.0f + skidDamage, 2))*2.0f;

#if _HELISIM_SOUNDS
                      if (colSpeed.SquareSize() > 2.0f)
                        PlayLandingSound(!IsDamageDestroyed() && FutureVisualState()._gearsUp < 0.1f && colSpeed.Size() < 4.0f);
#endif
                    }
                  }
                  else
                    // If helicopter is upside down destroy it more
                    newCrash = Type()->GetArmor() * 10.0f;

                  if (crash < newCrash)
                  {
                    crash = newCrash;
                    crashPos = pos;
                  }
                  //saturateMax(crash, (colSpeed.SquareSize()-25)*2);

                   const float maxRelSpeed=5;
                  if( colSpeed.SquareSize()>Square(maxRelSpeed) )
                  {
                    // adapt _speed to match criterion
                    colSpeed.Normalize();
                    colSpeed*=maxRelSpeed;
                    // only slow down
                    float oldSize = FutureVisualState()._speed.Size();
                    FutureVisualState()._speed = colSpeed+objSpeed;
                    if (FutureVisualState()._speed.SquareSize()>Square(oldSize))
                      FutureVisualState()._speed = FutureVisualState()._speed.Normalized()*oldSize;
                  }
                }
              }
            }
          } // if( object collisions enabled )

          GroundCollisionBuffer gCollision;
          bool enableLandcontact = true;
          if (wasLandContact && FutureVisualState().DirectionUp().Y()<0.985f) // cos 10 deg
            enableLandcontact = false;
          int nContactPoint = GLOB_LAND->GroundCollision(gCollision, this, moveTrans, 0.05f, 0, enableLandcontact);

          if( gCollision.Size()>0 )
          {
            // it is hard to have more than 3 contact points
            saturateMin(nContactPoint,5);
            //saturateMax(nContactPoint,gCollision.Size());

            Vector3 gFriction(VZero);
            float maxUnder=0;
            float maxUnderWater = 0;
#define MAX_UNDER 0.1f
#define MAX_UNDER_FORCE 0.1f

            for( int i=0; i<gCollision.Size(); i++ )
            {
              // info.pos is world space
              UndergroundInfo &info=gCollision[i];
              if( info.under<0 ) continue;
              // we consider two forces
              //ReportFloat("land",info.under);
              float under;
              if( info.type==GroundWater )
              {
                under=floatMin(info.under,3)*0.001f;
                if( maxUnderWater<info.under ) maxUnderWater=info.under;
                _waterContact=true;
                if( FutureVisualState()._speed.SquareSize()>Square(8) ) 
                {              
                  float newCrash = 1 * Type()->GetArmor();
                  if (crash < newCrash)
                  {
                    crash = newCrash;
                    crashPos = info.pos;
                  }
                }
              }
              else
              {
                _landContact=true;
                // we consider two forces
                //ReportFloat("land",info.under);
                if( maxUnder<info.under )
                  maxUnder=info.under;

                under=floatMin(info.under,MAX_UNDER_FORCE);
              }
            }
            if (maxUnder > 0 || maxUnderWater>0)
            {
              //torqueFriction=_angMomentum*1.0;   
              float newCrash;
              float dX,dZ;
              GLandscape->RoadSurfaceYAboveWater(FutureVisualState().Position(),&dX,&dZ);
              Vector3 normal(-dX,1,-dZ);
              if (FutureVisualState().DirectionUp()[1] > 0.0f)
              {
                if ( FutureVisualState().DirectionUp()*normal.Normalized() > 0.866) // (cos 30)
                {
                    // we can assume heli is able to slide forward
                    // TODO: handle rotating gear?
                    Vector3 slide = FutureVisualState().ModelSpeed();
                    // with gears up we take much more damage
                    slide[2] *= FutureVisualState()._gearsUp + (1-FutureVisualState()._gearsUp)*0.2f;

                    if(Glob.config.RoughLandingEnabled())
                    {
                      newCrash = (floatMax(slide.SquareSizeXZ() - 100,0) + floatMax(slide.Y()*slide.Y() - 500,0)) * 2.0f;;
                    }
                    else
                    {
                      float skidDamage = 0;
                      if (Type()->_hitGear >= 0)
                      {
                        skidDamage = (1 - _hit[Type()->_hitGear]) * 4; //skid damage can take up to 4m/s 
                        float skidCrash = (pow(slide.Y(),2)- 25.0f) / 16;
                        saturate(skidCrash,0,1);
                        if(skidCrash > 0.05)
                        {
                          SetHitContRaw(Type()->_hitGear, skidCrash + _hit[Type()->_hitGear]);
                          ShowDamage(Type()->_hitGear, this);
                        }
                      }
                      newCrash = (slide.SquareSize()- pow(5.0f + skidDamage, 2))*2.0f;
                    }

#if _HELISIM_SOUNDS
                    if (slide.SquareSize() > 2.0f)
                      PlayLandingSound(!IsDamageDestroyed() && FutureVisualState()._gearsUp < 0.1f && slide.Size() < 4.0f);
#endif
                }
                else
                {
                  newCrash = FutureVisualState()._speed.SquareSize();
                }
              }
              else
              {
                // If helicopter is upside down destroy it more
                newCrash = Type()->GetArmor() * 10.0f;
              }

              if (crash < newCrash)
              {
                crash = newCrash;            
                crashPos = FutureVisualState().PositionModelToWorld(GetCenterOfMass());
              }

              if( maxUnder>MAX_UNDER )
              {
                // it is necessary to move object immediately
                Vector3 newPos=moveTrans.Position();
                float moveUp=maxUnder-MAX_UNDER;
                newPos[1]+=moveUp;
                moveTrans.SetPosition(newPos);

                if (FutureVisualState()._speed.SquareSize()>Square(10))
                {
                  FutureVisualState()._speed.Normalize();
                  FutureVisualState()._speed *= 10;
                }
                saturateMax(FutureVisualState()._speed[1],-0.1f);
              }
            }

            const float maxFord=1.1f;
            if( maxUnderWater>maxFord && IsLocal())
            {
              float damage=(maxUnderWater-maxFord)*0.5f;
              saturateMin(damage,0.2f);
              LocalDamageMyself(VZero,damage*deltaT,GetRadius());
            }
            // if too much submerged, we are dead
            float tooMuchY = (_shape->Max().Y()-_shape->Min().Y())*0.5f;
            if (maxUnderWater>tooMuchY && IsLocal())
              LocalDamageMyself(VZero,5.0f*deltaT,GetRadius());
          }
          // force+=totForce;
          if (crash>0)
          {
            if( Glob.time>_disableDamageUntil )
            {
              _disableDamageUntil = Glob.time+0.5f;
              // crash boom bang state - impact speed too high
              _doCrash=CrashLand;
              if( _objectContact )
                _doCrash=CrashObject;
              if( _waterContact )
                _doCrash=CrashWater;
              _crashVolume=crash;
              //

              // Do crash damage around crashPos
              PositionWorldToModel(crashPos,crashPos);
              CrashDammage(crash, 0.7 * crashPos + 0.3 * GetCenterOfMass(), 2);
              // If chopper is upside down rather explode... 
              if (IsLocal() && (crash * GetType()->GetInvArmor() >5))           
              {
                
                // set some explosion
                float maxTime = 5-crash * GetType()->GetInvArmor()*0.2f;
                maxTime *=0.2; //make helicopter explode earlier 
                saturate(maxTime,0.5f,3);
                if (_explosionTime>Glob.time+maxTime)
                  _explosionTime=Glob.time+GRandGen.Gauss(0,maxTime*0.3f,maxTime);
              }
            }
          }
        }

        bool stopCondition=false;
        if( _landContact && !_waterContact && !_objectContact )
        {
          // apply static friction
          float maxSpeed=Square(0.7f);
          if( !Driver() ) maxSpeed=Square(1.2f);
          if( FutureVisualState()._speed.SquareSize()<maxSpeed && _angVelocity.SquareSize()<maxSpeed*0.3f )
            stopCondition=true;
        }
        if( stopCondition)
          StopDetected();
        else
          OnMoved();
      }
      if (_isDead && ( _landContact || _objectContact))
      {
        NeverDestroy();
        IExplosion *smoke = GetSmoke();
        if (smoke)
        {
          smoke->SetExplosion(100,100);
          smoke->Explode();
        }
        // note: high speed during contact causes explosion
      }

      float surfaceY = GLandscape->SurfaceY(FutureVisualState().Position().X(), FutureVisualState().Position().Z());
      if (RenderVisualState().Position().Y() + 20 < surfaceY)
      { //if rtd simulation gets under ground  destroy it
        SetDamage(1.0);
      }

      if (EnableVisualEffects(prec))
      {
        // effects simulation
        bool water = surfaceY < GLandscape->GetSeaLevel();
        float height = FutureVisualState().Position().Y() - (water ? GLandscape->GetSeaLevel() : surfaceY);
        const float maxHeight = 30.0f;
        saturate(height, 0, maxHeight);
        float factor = FutureVisualState()._rotorSpeed * (maxHeight - height) / maxHeight;
        const float invMaxSpeed = 1.0f / (200.0f / 3.6f);
        float alpha = factor - fabs(FutureVisualState().ModelSpeed().Z()) * invMaxSpeed;
        if (alpha > 0.01)
        {
          float rotorDammage = (Type()->_rotorHHit < 0 || _hit[Type()->_rotorHHit] < 0.999f) ? 1.0 : 0.0;
          //effect is influenced by collective and destruction
#if !_DUMMY_ROTORLIB
          float col = _rtdHelicopter->getPilotInputData()->m_collective ;
#else
          float col = 0;
#endif
          float speed = (20.0f * factor );
          float effectValues[] = {col * rotorDammage * speed, col * rotorDammage *alpha, 0, 0, 0};
          if (water)
            _waterEffect.Simulate(deltaT, prec, effectValues, lenof(effectValues));
          else
            _dustEffect.Simulate(deltaT, prec, effectValues, lenof(effectValues));
        }

        float damage = GetTotalDamage();
        if (FutureVisualState()._rotorSpeed>0 && damage >= 0.4)
        {
          float damageValues[] = {damage, FutureVisualState()._rotorSpeed, 0, 0, 0};
          _damageEffect.Simulate(deltaT, prec, damageValues, lenof(damageValues));
        }

        SimulateExhaust(deltaT, prec);
      }

      Vector3 voidVector = Vector3(0,0,0);
      ApplyForces(deltaT,voidVector,voidVector,voidVector,voidVector);
      /*Vector3 voidVector = Vector3(0,0,0);
      ApplyForces(deltaT,voidVector,voidVector,voidVector,voidVector);
      PlaneOrHeli::Simulate(deltaT, prec);
      SimulatePost(deltaT, prec);*/

//////// HELICOPTER SIMULATION - LITE END   ////////////////
////////////////////////////////////////////////////////////

      // turn wheels
      float speedZ= IsLandContact() ? FutureVisualState().ModelSpeed().Z() : 0;
      float wheelLen=Type()->_wheelCircumference;
      FutureVisualState()._wheelPhaseDelta = speedZ*(1/wheelLen)*deltaT;
      FutureVisualState()._wheelPhase = fastFmod(FutureVisualState()._wheelPhase + FutureVisualState()._wheelPhaseDelta, 1);


      bool elPower = (((_batteryOn && _batteryCharge > 0)  || _APU > 0)) ? true : false;
      _rtdHelicopter->SetSlingLoadForce(_slingloadForce);
      _rtdHelicopter->Simulate(deltaT, this, elPower ,_starter, GetFuel());

      if( !_engineOff != EngineIsOn()) 
      { //check engine on event
        if(_engineOff) Transport::EngineOn();
        else Transport::EngineOff();
      }

      _rtdStateAge -= GWorld->GetRTDWorld()->AccumulatedSimulationTime();
      GWorld->GetRTDWorld()->ClearAccumulatedSimulationTime();  // assume there's only one RTD chopper so we can reset it here

      float interpCoeff;

      //wing rotation
      _rtdHelicopter->SetWingsOrientation(Lerp(Type()->_horizontalWingsAngleCollMin,Type()->_horizontalWingsAngleCollMax, _actualCollectiveRTD));

      if (_rtdStateAge >= -deltaT || _rtdStateAge < -1.0f) {  // resynchronization needed?
        interpCoeff = 1.0f;
        LogF("RTD resync: age = %f (-> %f)", _rtdStateAge, -deltaT);
        _rtdStateAge = -deltaT;
      }
      else {
        interpCoeff = deltaT / -_rtdStateAge;
        //LogF("RTD age = %f, interpCoeff = %f", _rtdStateAge, interpCoeff);
        _rtdStateAge += deltaT;
      }

      CopyTransformRTDToArma(interpCoeff);

      smVec3r velocity = _rtdHelicopter->getVelocity();
      FutureVisualState().SetSpeed( (1-interpCoeff) * FutureVisualState().Speed() + interpCoeff * Vector3(velocity[0],velocity[2],velocity[1]) );

      HelicopterVisualState *helicopterVisualState = &FutureVisualState();
      if(helicopterVisualState)
      {
        RTD::RotorModelBase* mainRotor = _rtdHelicopter->getMainRotor();
        RTD::RotorModelBase* tailRotor = _rtdHelicopter->getTailRotor();

        //cyclic controller
        helicopterVisualState->_cyclicForward = (1-interpCoeff) * helicopterVisualState->_cyclicForward  + interpCoeff * _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[1];
        helicopterVisualState->_cyclicAside = (1-interpCoeff) * helicopterVisualState->_cyclicAside  + interpCoeff * _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[0];

        _cyclicForwardAnimation =  helicopterVisualState->_cyclicForward;
        _cyclicAsideAnimation =  helicopterVisualState->_cyclicAside;

        smVec3r acceleration = _rtdHelicopter->getAcceleration();
        helicopterVisualState->_acceleration = (1-interpCoeff) * helicopterVisualState->_acceleration  + interpCoeff * Vector3(acceleration[0],acceleration[2],acceleration[1]);

        //avoid float errors
        float rotorRpmMain = (fabs(mainRotor->getRPM()) < 1)? 0 : mainRotor->getRPM();
        FutureVisualState()._rotorSpeed = _rotorSpeedWanted = (Type()->_rotorHHit >=0 && _hit[Type()->_rotorHHit] > 0.99)? 0 : (rotorRpmMain / mainRotor->getDesignRPM());
        
        rotorRpmMain =  H_2PI * (rotorRpmMain / 60.0f);

        float rotorRpmTail = (fabs(tailRotor->getRPM()) < 1)? 0 : tailRotor->getRPM();
        rotorRpmTail =  H_2PI * (rotorRpmTail / 60.0f);

        helicopterVisualState->_rotorPositionDelta = deltaT * rotorRpmMain;
        helicopterVisualState->_rotorPosition += helicopterVisualState->_rotorPositionDelta;
        helicopterVisualState->_backRotorPosition += deltaT * rotorRpmTail;
      }

      if(_weightUpdateTime < Glob.time)
      {
        UpdateWeight();
        //update just once in several seconds
        _weightUpdateTime = Glob.time + 5;
      }

      PlaneOrHeli::Simulate(deltaT, prec);
      SimulatePost(deltaT, prec);
    } 
    }
    else
    {
      if(_isRTDSimulated) OnSimulationChanged(false);

      if(_driver && _driver->Brain() && !_driver->Brain()->IsAnyPlayer())
      {
        if(_rotorSpeedWanted > 0.05) _rotorSpeedWanted = 1;
        if(Type()->_APU && _APU > 0.0) SetAPU(0,1);
        if(_rotorSpeedWanted > 0.05 && !_batteryOn) SetBatteryOn(true); 

        //AI using starter (just play sound for a moment)
        if(_rotorSpeedWanted > 0.9f && FutureVisualState()._rotorSpeed < 0.22)
        {
          if(_starterWanted == 0) SetStarterWanted(true, -1);
        }
        else if(_starterWanted > 0) SetStarterWanted(false, -1);

        _rtdHelicopter->EngineOn(-1);
        SetRotorBrake(0.0f);
      }
      base::Simulate(deltaT, prec);
    }

    //disable autohover, when on ground
    if(_autoHoverRTD)
    {
      _autoHoverRTD->SetIsTouchingGound(IsLandContact());
    }
  }
  else base::Simulate(deltaT, prec);
#else
  base::Simulate(deltaT, prec);
#endif    

  /// camera shake - vehicle damage
  if (!_engineOff)
  {
    if(GWorld->FocusOn() && GWorld->FocusOn()->GetVehicleIn() == this)
    {
      if (Glob.time < _lastCamDamageShakeTime || (Glob.time - _lastCamDamageShakeTime) > CameraShakeManager::DefaultPassingVehicleUpdateTime)
      {
        GWorld->GetCameraShakeManager().AddSource(CameraShakeParams(CameraShakeSource::CSDamage, FutureVisualState().Position(), 0, this));
        _lastCamDamageShakeTime = Glob.time;
      }
    }
  }

  //APU change
  if(_APUStartingTime > 0)
  {
    _APUStartingTime -=deltaT;
    if(_APUStartingTime < 0) _APU = _APUwanted;
    else _APU += _APUchange * deltaT;
  }

  // rpm starter
  for (int i = 0; i < lenof(_rpmStarterOffTime); i++)
  {
    if (!GetStarter(i) && _rpmStarterOffTime[i] > 0.0f)
      _rpmStarterOffTime[i] -= deltaT;
  }
}

void HelicopterRTD::OnSimulationChanged(bool changeToRTD)
{
#if !_DUMMY_ROTORLIB
  //if changing to RTD simulation, give RTD 5 seconds to sync. state
  if(_isRTDSimulatedWanted == false && changeToRTD == true)
  {
    _timeToForcedChange = Glob.time + 5;
    //for case, that enable/disable rotorlib ignored or rewriten some changes
    if(!_rtdHelicopter->isEnabled()) for (int i=0; i < _lastHit.Size(); i++) _lastHit[i] = 0;
  }

  if(_rtdHelicopter)
  {
    //change from ARMA simulation to RTDsimulation
    if (changeToRTD )
    {
      _rtdStateAge = 0;
      GWorld->GetRTDWorld()->ClearAccumulatedSimulationTime();

      if(EngineIsOn())
      {
        //reset engines temperatures
        for(int i = 0; i< _enginesTemperature.Size(); i++) _enginesTemperature[i].SetHot(this, i);

        //if in air, set collective
        if(IsLandContact())
        {
          SetActualCollective(0.0f);

          if(FutureVisualState()._rotorSpeed > 0.1)
          {
            SetBatteryOn(true);
            float timeDelay =  0.25;
            if(_isRTDSimulatedWanted && _rtdHelicopter->getMainRotor()->getRPM() > (0.35f * _rtdHelicopter->getMainRotor()->getDesignRPM()) && _timeToForcedChange - Glob.time > timeDelay + 0.01f)
            {
              _timeToForcedChange = Glob.time + timeDelay;
              SetActualCollective(0);
              _rtdHelicopter->getPilotInputData()->m_collective = _actualCollectiveRTD = ((_collectiveWanted > 0.05) ? _collectiveWanted : Type()->_defaultCollective);
            }

            if(_isRTDSimulatedWanted &&  Glob.time > _timeToForcedChange)
            {
              _isRTDSimulated = _isRTDSimulatedWanted;
              _timeToForcedChange = Glob.time;
              GWorld->GetRTDWorld()->ClearAccumulatedSimulationTime();
            }

            _rtdHelicopter->setVelocity(smVec3r(RenderVisualState().Speed().X(), RenderVisualState().Speed().Z(), RenderVisualState().Speed().Y()));
            _rtdHelicopter->setAcceleration(smVec3r(0,0,0));
            _rtdHelicopter->setRotVelocityBCS(smVec3r(0,0,0));
            _rtdHelicopter->setRotAccelerationBCS(smVec3r(0,0,0));
            SetAutoTrim(_autoTrimEnabled);

            if(_isRTDSimulatedWanted == false)
            {
              EngineOn(true);//quick engine start
              _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[0]     = 0;
              _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[1]     = 0;
              _rtdHelicopter->getPilotInputData()->m_pedals                             = 0;
              _isRTDSimulatedWanted = changeToRTD;
            }
          }
          else
          {
            _isRTDSimulated = _isRTDSimulatedWanted = changeToRTD;
          }
        }
        else
        {
          float timeDelay =  0.5;
          if(_isRTDSimulatedWanted && _rtdHelicopter->getMainRotor()->getRPM() > (0.85f * _rtdHelicopter->getMainRotor()->getDesignRPM()) && _timeToForcedChange - Glob.time > timeDelay + 0.01f)
          {
            _timeToForcedChange = Glob.time + timeDelay;
            SetActualCollective((_collectiveWanted > 0.05) ? _collectiveWanted : Type()->_defaultCollective);
            _rtdHelicopter->getPilotInputData()->m_collective = _actualCollectiveRTD = ((_collectiveWanted > 0.05) ? _collectiveWanted : Type()->_defaultCollective);
          }

          if(_isRTDSimulatedWanted &&  Glob.time > _timeToForcedChange)
          {
            _isRTDSimulated = _isRTDSimulatedWanted;
            _timeToForcedChange = Glob.time;
            GWorld->GetRTDWorld()->ClearAccumulatedSimulationTime();
          }

          _rtdHelicopter->setVelocity(smVec3r(RenderVisualState().Speed().X(), RenderVisualState().Speed().Z(), RenderVisualState().Speed().Y()));
          _rtdHelicopter->setAcceleration(smVec3r(0,0,0));
          _rtdHelicopter->setRotVelocityBCS(smVec3r(0,0,0));
          _rtdHelicopter->setRotAccelerationBCS(smVec3r(0,0,0));
          SetAutoTrim(_autoTrimEnabled);


          if(_isRTDSimulatedWanted == false)
          {
            EngineOn(true);//quick engine start
            _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[0]     = 0;
            _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[1]     = 0;
            _rtdHelicopter->getPilotInputData()->m_pedals                             = 0;
            _isRTDSimulatedWanted = changeToRTD;
          }
        }  
      }
      else
      {
        if(FutureVisualState()._rotorSpeed < 0.01) ResetHelicopterRTD();//quick engine stop
        EngineOff();
        
        SetActualCollective(0.0f);
        _isRTDSimulated = _isRTDSimulatedWanted = changeToRTD;;
        _timeToForcedChange = Glob.time;
      }

    }
    //change from RTD simulation to ARMA simulation
    else
    {
      _isRTDSimulatedWanted = changeToRTD;;
      _timeToForcedChange = Glob.time;
      bool AIPilot = (_driver && _driver->Brain() && !_driver->Brain()->IsAnyPlayer());
      //skip start up procedure and everything on/off8
      if(EngineIsOn())
      {
       
        RTD::FGRotorBladeElementTheory* mainRotor = (RTD::FGRotorBladeElementTheory*)_rtdHelicopter->getMainRotor();
        FutureVisualState()._rotorSpeed = mainRotor->getRPM() / mainRotor->getConfigParams()->m_designRPM;
        if(AIPilot)
        {
          _rotorSpeedWanted = 1;
          if(!_batteryOn) SetBatteryOn(true);
          SetStarterWanted(false, -1);
          if(Type()->_APU && _APU > 0.0) SetAPU(0,1);
        }
        else
        {
          if( _rtdHelicopter->GetThrottle(0) < 0.5) _rotorSpeedWanted = 0.65;
          else _rotorSpeedWanted = 1.0;
        }
      }
      else
      {
        RTD::FGRotorBladeElementTheory* mainRotor = (RTD::FGRotorBladeElementTheory*)_rtdHelicopter->getMainRotor();
        FutureVisualState()._rotorSpeed = mainRotor->getRPM() / mainRotor->getConfigParams()->m_designRPM;
        _mainRotorWanted = mainRotor->getRPM() / mainRotor->getConfigParams()->m_designRPM;
        _rotorSpeedWanted = 0;

        if(AIPilot)
        {
          if(_batteryOn) SetBatteryOn(false);
          SetStarterWanted(false, -1);
          if(Type()->_APU && _APU > 0.0) SetAPU(0,1);
        }

      }
      _isRTDSimulated = _isRTDSimulatedWanted;
    }
    _rtdHelicopter->setEnabled(_isRTDSimulatedWanted);
  }
#endif
}

#define ACTION_EVENT(EVENT) { \
  (unit->GetPerson()->OnEvent(EESelectedRotorLibActionPerformed, this, (float) EVENT - ATEnginesOnAuto, -1)); \
  (GWorld->OnMissionEvent(MESelectedRotorLibActionPerformed, unit->GetPerson(), this, (float) EVENT - ATEnginesOnAuto, -1)); \
} \

void HelicopterRTD::SimulateWeaponActivity( float deltaT, SimulationImportance prec )
{
  //RTD helicopter has separated collision lights and reflector
  AIBrain *unit = GWorld->FocusOn();
  if (unit && unit == CommanderUnit() && !unit->IsInCargo() && !GWorld->GetCameraEffect())
  {
    if (GInput.GetActionToDo(UAHeadlights))
    {
      _pilotLight=!_pilotLight;
    }
  }

  base::SimulateWeaponActivity(deltaT, prec);
}

void HelicopterRTD::KeyboardPilot(AIBrain *unit, float deltaT )
{ 
  if (!unit->GetPerson() || unit->GetPerson()->KeyboardUsedInFrame()) return;
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter)
  {
      //Vector3Val position=FutureVisualState().Position();
      //float surfaceY=GLOB_LAND->RoadSurfaceYAboveWater(position);

      // float curHeightAGL=position.Y()-surfaceY;
      // float curBottomHeight = curHeightAGL + _shape->GeometryLevel()->Min().Y();

      //// automatic gear control
      //gear is now action based
      //if (curBottomHeight>30 || curBottomHeight>20 && FutureVisualState()._speed.SquareSizeXZ()>Square(25))
      //  _pilotGear = false;
      //if (FutureVisualState()._speed.SquareSizeXZ()<Square(23) || (FutureVisualState()._rotorSpeed < 0.9)) // (_rotorSpeed < 0.9)) - when main rotor failure open gear earlier
      //{
      //  if (curBottomHeight<5 || curBottomHeight<25 && (FutureVisualState()._speed.SquareSizeXZ()<Square(8) || (FutureVisualState()._rotorSpeed < 0.9)))
      //    _pilotGear = true;
      //}


      //user actions
      if(GInput.GetActionToDo(UAHelicopterTrimOn)) 
      {
        _cyclicSideRTDOffset = _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[0];
        _cyclicForwardRTDOffset = _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[1];
        _backRotorRTDOffset =  _rtdHelicopter->getPilotInputData()->m_pedals;

        _delayAfterTrimming = Glob.time + 1;
        _controllsTrim = true;
      }
      if(GInput.GetActionToDo(UAHelicopterTrimOff)) 
      {
        _cyclicSideRTDOffset = 0;
        _cyclicForwardRTDOffset = 0;
        _backRotorRTDOffset = 0;
        _controllsTrim = false;
      }

      //Helicopter direction pilot
      {
        // aim and turn with aiming actions
        float aimX(0), aimY(0);
        if (unit->LookAroundEnabled())
        {
          aimX = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UAAimRight, UAAimLeft);
          aimY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UAAimUp, UAAimDown);
        }
        if(!_copilot)//is looking around in elsewhere
          LookAround(aimX, aimY, deltaT, &Type()->_viewPilot, unit);
      }

      if(_delayAfterTrimming < Glob.time)
      {
        const float cyclicLRcoef = 1.0;
        const float cyclicKeyboard = 1.0f;

        float backRotorRTD = (GInput.GetAction(UAHeliRudderRight) - GInput.GetAction(UAHeliRudderLeft));
        float _wantedCyclicSideRTD = (GInput.GetAction(UAHeliCyclicRight,true,cyclicKeyboard) - GInput.GetAction(UAHeliCyclicLeft,true,cyclicKeyboard))*Input::MouseRangeFactor*cyclicLRcoef;
        _wantedCyclicSideRTD -= (GInput.GetAction(UAHeliLeft)-GInput.GetAction(UAHeliRight))*Input::MouseRangeFactor;
        float _wantedCyclicForwardRTD =  GInput.GetAction(UAHeliCyclicForward)*Input::MouseRangeFactor - GInput.GetAction(UAHeliCyclicBack)*Input::MouseRangeFactor;

        saturate(backRotorRTD, -1.0, 1.0);
        saturate(_wantedCyclicSideRTD, -1.0, 1.0);
        saturate(_wantedCyclicForwardRTD, -1.0, 1.0);

        //hydralics damage
        float hydraulics =  (Type()->_hitHydraulics >= 0) ? (1 - _hit[Type()->_hitHydraulics]) : 1;
        //max controller change (affected by hydraulics)
        float deltaControl = deltaT * pow(hydraulics,2) * 20;

        if(_controllsTrim)
        {
          _wantedCyclicSideRTD += _cyclicSideRTDOffset;
          _wantedCyclicForwardRTD += _cyclicForwardRTDOffset;
          backRotorRTD += _backRotorRTDOffset;
        }

        float delta = (_wantedCyclicSideRTD - _cyclicSideRTD);
        Limit(delta, -deltaControl, +deltaControl);
        _cyclicSideRTD +=  delta;

        delta = (_wantedCyclicForwardRTD - _cyclicForwardRTD);
        Limit(delta, -deltaControl, +deltaControl);
        _cyclicForwardRTD +=  delta;

        delta = (backRotorRTD - _pedals);
        Limit(delta, -deltaControl, +deltaControl);
        _pedals +=  delta;
        //scaling tail rotor force to 0 caused some extreme rotation
        //and just breaking drive train connection still allows some small control 
        if(Type()->_rotorVHit>=0 && _hit[Type()->_rotorVHit] > 0.9 ) 
          _pedals = 0;

        _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[0]     = _cyclicSideRTD;
        _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[1]     = _cyclicForwardRTD;
        _rtdHelicopter->getPilotInputData()->m_pedals                             = _pedals;

        _rtdHelicopter->getPilotInputData()->m_wheelBrakesLeft =  _rtdHelicopter->GetBrakeLeft();
        _rtdHelicopter->getPilotInputData()->m_wheelBrakesRight = _rtdHelicopter->GetBrakeRight();

        _rtdHelicopter->getPilotInputData()->m_rotorBrake =  _rtdHelicopter->GetRotorBrake();

        _rtdHelicopter->getPilotInputData()->m_extendGears = GetPilotGear();

      }

      float throttleRTD = 0.5f * (GInput.GetAction(UAHeliCollectiveRaiseCont)-GInput.GetAction(UAHeliCollectiveLowerCont) +1);
      float forwardRTD = GInput.GetAction(UAHeliCollectiveRaise) - GInput.GetAction(UAHeliCollectiveLower);

      float _actualCollectiveCoef =  Glob.time - _actualCollectiveHelperTime;
      saturate(_actualCollectiveCoef, 0.15f, 1.15f);
      _actualCollectiveCoef *= _actualCollectiveCoef;

      if(forwardRTD > 0.0) _actualCollectiveRTD += 0.45f*deltaT * _actualCollectiveCoef ;
      else if(forwardRTD < 0.0) _actualCollectiveRTD -= 0.45f*deltaT * _actualCollectiveCoef;
      else _actualCollectiveHelperTime = Glob.time;
      saturate(_actualCollectiveRTD, 0.0f, 1.0f);

      //is airbourne, use forwardRTD - it has 0.5 as default value
      if (fabs(forwardRTD-_lastForward)>0.1f || GInput.gameFocusLost > 0)
      {
        if (_lastForward<FLT_MAX) _thrustHelperTime = Glob.time; // ignore initial settings
        _lastForward = forwardRTD;
      }
      if (fabs(throttleRTD-_lastThrotleDirect)>0.1f && GInput.gameFocusLost <= 0)
      {
        if (_lastThrotleDirect<FLT_MAX) _analogueThrottleTime = Glob.time; // ignore initial settings
        _lastThrotleDirect = throttleRTD;
      }

      float throttleDirectRTD = (_thrustHelperTime >= _analogueThrottleTime)? _actualCollectiveRTD : throttleRTD;
      saturate(throttleDirectRTD, -1.0, 1.0);
      //setting collective
      if(_isRTDSimulated) _rtdHelicopter->getPilotInputData()->m_collective = throttleDirectRTD;
      else if(!_isRTDSimulatedWanted) _rtdHelicopter->getPilotInputData()->m_collective = 0.7f;


      _rudderWanted = _rtdHelicopter->getPilotInputData()->m_pedals;
      _actualCollectiveRTD = throttleDirectRTD;

      SetAutoTrim(_autoTrimEnabled);
      // Apply deadband filter to eliminate joystick errors
      // - disabled, arma already has its own solution
      //smDeadBandFilter::filter( _rtdHelicopter->getPilotInputData()->m_pedals.getValue(),     0.12f, 1.0f , 0.0f );
      //smDeadBandFilter::filter( _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[0],  0.1f, 1.0f , 0.0f );
      //smDeadBandFilter::filter( _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[1],  0.1f, 1.0f , 0.0f );

      if(!_isRTDSimulated) base::KeyboardPilot(unit, deltaT);
  }
  else base::KeyboardPilot(unit, deltaT);

  if(GInput.GetActionToDo(UAHeliThrottleIncrease1))
  {
    float throttle = _rtdHelicopter->GetThrottle(0);
    if(throttle < 0.1f) //idle
      ACTION_EVENT(ATThrottleIdle1)
    else 
      ACTION_EVENT(ATThrottleFull1)
  }

  if(GInput.GetActionToDo(UAHeliThrottleDecrease1))
  {
    float throttle = _rtdHelicopter->GetThrottle(0);
    if(throttle > 0.25f) //idle
      ACTION_EVENT(ATThrottleIdle1)
    else 
      ACTION_EVENT( ATThrottleOff1)
  }

  if(GInput.GetActionToDo(UAHeliThrottleIncrease2) && _rtdHelicopter && _rtdHelicopter->GetNumberOfEngines() > 0)
  {
    float throttle = _rtdHelicopter->GetThrottle(1);
    if(throttle < 0.1f) //idle
      ACTION_EVENT(ATThrottleIdle2)
    else 
      ACTION_EVENT(ATThrottleFull2)
  }

  if(GInput.GetActionToDo(UAHeliThrottleDecrease2) && _rtdHelicopter && _rtdHelicopter->GetNumberOfEngines() > 0)
  {
    float throttle = _rtdHelicopter->GetThrottle(1);
    if(throttle > 0.25f) //idle
      ACTION_EVENT(ATThrottleIdle2)
    else 
      ACTION_EVENT(ATThrottleOff2)
  }

  if(GInput.GetActionToDo(UAHeliThrottleIncrease3) && _rtdHelicopter && _rtdHelicopter->GetNumberOfEngines() > 1)
  {
    float throttle = _rtdHelicopter->GetThrottle(2);
    if(throttle < 0.1f) //idle
      ACTION_EVENT(ATThrottleIdle3)
    else 
      ACTION_EVENT(ATThrottleFull3)
  }

  if(GInput.GetActionToDo(UAHeliThrottleDecrease3) && _rtdHelicopter && _rtdHelicopter->GetNumberOfEngines() > 1)
  {
    float throttle = _rtdHelicopter->GetThrottle(2);
    if(throttle > 0.25f) //idle
      ACTION_EVENT(ATThrottleIdle3)
    else 
      ACTION_EVENT(ATThrottleOff3)
  }

  // --- Starter 1 --- //
  if(GInput.GetActionToDo(UAHeliStarterToggle1))
  {
    if(_starterWanted & 1)
    {
      SetStarterWanted(false, 0);
      ACTION_EVENT(ATStarterOff1)
    }
    else
    {
      SetStarterWanted(true, 0);
      ACTION_EVENT(ATStarterOn1)
    }
  }

  // --- Starter 2 --- //
  if(GInput.GetActionToDo(UAHeliStarterToggle2) && _rtdHelicopter && _rtdHelicopter->GetNumberOfEngines() > 0)
  {    
    if(_starterWanted & 2)
    {
      SetStarterWanted(false, 1);
      ACTION_EVENT(ATStarterOff2)
    }
    else
    {
      SetStarterWanted(true, 1);
      ACTION_EVENT(ATStarterOn2)
    }
  }

  // --- Starter 3 --- //
  if(GInput.GetActionToDo(UAHeliStarterToggle3) && _rtdHelicopter && _rtdHelicopter->GetNumberOfEngines() > 1)
  {
    if(_starterWanted & 4)
    {
      SetStarterWanted(false, 2);
      ACTION_EVENT(ATStarterOff3)
    }
    else
    {
      SetStarterWanted(true, 2);
      ACTION_EVENT(ATStarterOn3)
    }
  }

  // --- Battery --- //
  if(GInput.GetActionToDo(UAHeliBatteriesToggle))
  {
    if(_batteryOn)
    {
      SetBatteryOn(false);
      ACTION_EVENT(ATBatteriesOff)
    }
    else
    {
      SetBatteryOn(true);
      ACTION_EVENT(ATBatteriesOn)
    }
  }

  // --- Rotor brake --- //
  if(GInput.GetActionToDo(UAHeliRotorBrakeToggle) && _rtdHelicopter)
  {
    if(_rtdHelicopter->GetRotorBrake() > 0.0001)
    {
      SetRotorBrake(0.0f);
      ACTION_EVENT(ATRotorBrakeOff)
    }
    else
    {
      SetRotorBrake(1.0f);
      ACTION_EVENT(ATRotorBrakeOn)
    }
  }

  // --- Engine On/Off (auto) --- //
  if(GInput.GetActionToDo(UAHeliEnginesToggle))
  {
    if( EngineIsOn()) ACTION_EVENT(ATEnginesOffAuto)
    else ACTION_EVENT(ATEnginesOnAuto)
  }

  // --- APU --- //
  if(GInput.GetActionToDo(UAHeliAPUToggle) && Type()->_APU)
  {
    if(_APU > 0.0001)
    {
#if _HELISIM_SOUNDS
      PlayApuSound(false);
#endif
      ACTION_EVENT(ATAPUOff)
    }
    else
    {
#if _HELISIM_SOUNDS
      PlayApuSound(true);
#endif
      ACTION_EVENT(ATAPUOn)
    }
  }

  // --- Engine On/Off (auto) --- //
  if(_rtdHelicopter && GInput.GetActionToDo(UAHeliWheelsBrake))
  {
    if(_rtdHelicopter->GetBrakeLeft() > 0.0001 || _rtdHelicopter->GetBrakeRight() > 0.0001)
    { 
      ACTION_EVENT(ATWheelsBrakeOff)
      SetWheelsBrakeRTD(0.0,3); 
    }
    else
    {
      ACTION_EVENT(ATWheelsBrakeOn)
      SetWheelsBrakeRTD(1.0,3); 
    }
  }

  //manual trimming
  if(GInput.GetActionToDo(UAHeliTrimLeft))
  {
    _cyclicSideRTDOffset -= 0.025f;
    saturate(_cyclicSideRTDOffset, -1.0f, 1.0f);
    _controllsTrim = true;
  }

  if(GInput.GetActionToDo(UAHeliTrimRight))
  {
    _cyclicSideRTDOffset += 0.025f;
    saturate(_cyclicSideRTDOffset, -1.0f, 1.0f);
    _controllsTrim = true;
  }

  if(GInput.GetActionToDo(UAHeliTrimBackward))
  {
    _cyclicForwardRTDOffset -= 0.025f;
    saturate(_cyclicForwardRTDOffset, -1.0f, 1.0f);
    _controllsTrim = true;
  }

  if(GInput.GetActionToDo(UAHeliTrimForward))
  {
    _cyclicForwardRTDOffset += 0.025f;
    saturate(_cyclicForwardRTDOffset, -1.0f, 1.0f);
    _controllsTrim = true;
  }

  if(GInput.GetActionToDo(UAHeliTrimRudderLeft))
  {
    _backRotorRTDOffset -= 0.025f;
    saturate(_backRotorRTDOffset, -1.0f, 1.0f);
    _controllsTrim = true;
  }

  if(GInput.GetActionToDo(UAHeliTrimRudderRight))
  {
    _backRotorRTDOffset += 0.025f;
    saturate(_backRotorRTDOffset, -1.0f, 1.0f);
    _controllsTrim = true;
  }
#endif
}

bool HelicopterRTD::SetAutoTrim(bool autoTrim)
{
#if !_DUMMY_ROTORLIB
  bool autoTrimWanted = autoTrim || _hoveringAutopilot || Glob.config.AutoTrimEnabled() || (_isRTDSimulatedWanted && !_isRTDSimulated);
  _autoTrimEnabled = autoTrim;

  int count = intMin(2,_rtdHelicopter->getNumCSComponents());
  if(_rtdHelicopter && autoTrimWanted != _autoTrimEnabledState)
  {
    //auto trim can be on first or second pos. (first can be autohover)
    for(int i = 0; i< count; i++)
    {
      if(strcmpi(_rtdHelicopter->getCSComponent(i)->getType().data(), "FCSHelicopterAutoTrim100") == 0)
      { 
        RotorLibFDM::FCSHelicopterAutoTrim100* autoTrimRTD = (RotorLibFDM::FCSHelicopterAutoTrim100*)_rtdHelicopter->getCSComponent(i);
        if ( autoTrimRTD ) 
        {
          autoTrimRTD->enable( autoTrimWanted);
          _autoTrimEnabledState = autoTrimWanted;
          return autoTrimWanted;
        }
      }
    }
  }
#endif
  return false;
}

bool HelicopterRTD::SetAutoHover(bool autoHover)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _rtdHelicopter->getNumCSComponents() > 0)
  {
    if(strcmpi(_rtdHelicopter->getCSComponent(0)->getType().data(), "FCSAutoHoverTOH") == 0)
    { 
      FCSAutoHoverTOH* autoHoverRTD = (FCSAutoHoverTOH*)_rtdHelicopter->getCSComponent(0);
      if ( autoHoverRTD ) 
      {
        autoHoverRTD->enable( autoHover);
        return autoHover;
      }
    }
  }
#endif
  return false;
}


Vector3 HelicopterRTD::GetTrimOffsets()
{
  Vector3 offsets = Vector3(_cyclicForwardRTDOffset,_cyclicSideRTDOffset,_backRotorRTDOffset);
  return offsets;
}


float HelicopterRTD::GetEngineTargetRPM(int engineIndex)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter)  return _rtdHelicopter->GetTargetRPM(engineIndex);
#endif
  return 0;
}

void HelicopterRTD::GetEngineTargetRPM(AutoArray<float> &RPMs)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->GetTargetRPM(RPMs);
#endif
}

#define FUEL_DENSITY 0.8f
void HelicopterRTD::GetWeight(AutoArray<float> &values)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) 
  {
    values.Add(_fuselageWeight);
    values.Add(_crewWeight);
    values.Add(GetFuel() * FUEL_DENSITY);
    values.Add(_customWeight);
    values.Add(_ammoWeight);
  }
  else
#endif
  {
    values.Add(0.0);
    values.Add(0.0);
    values.Add(0.0);
    values.Add(0.0);
    values.Add(0.0);
  }
}

//at this moment only RTD helicopter is affected by weight
void HelicopterRTD::AddCrewWeight()
{
  _crewWeight += 100;
  UpdateWeight();
}

void HelicopterRTD::RemoveCrewWeight()
{
  _crewWeight -= 100;
  saturateMax(_crewWeight, 0.0f);
  UpdateWeight();
}


void HelicopterRTD::UpdateWeight()
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter)
  {
    _ammoWeight = 0;
    TurretContext context;
    if (GetPrimaryGunnerTurret(context))
    {
      context._weapons->ForEachMagazine([this](Magazine *mag) -> bool
      {
        if (!mag->Type() || mag->Type()->_maxAmmo <= 0) return false; // continue
        _ammoWeight += (mag->GetAmmo() / (float)mag->Type()->_maxAmmo) * mag->Type()->_mass;
        return false; // continue
      });

      context._weapons->ForEachWeapon([this](Weapon *weapon) -> bool
      {
        const WeaponType *type = weapon->Type();
        if (type) _ammoWeight += type->_mass;
        return false; // continue
      });
    }

    float oldWeight = _rtdHelicopter->getMainBodyMass();
    float newWeight = _fuselageWeight + _crewWeight + _customWeight + _ammoWeight + GetFuel() * FUEL_DENSITY;
    float coef = newWeight / oldWeight;

    //new Moment Of Inertia
    smVec4d MoI;
    MoI = _rtdHelicopter->getMainBodyMomentsOfInertia();
    MoI = smVec4d(MoI[0] * coef, MoI[1] * coef, MoI[2] * coef, MoI[3] * coef);

    //set new values
    _rtdHelicopter->setMainBodyMass(newWeight);
    _rtdHelicopter->setMainBodyMomentsOfInertia(MoI);
  }
#endif
}

void HelicopterRTD::SetThrottleRTD(float value, int index)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->SetThrottle(value, index);
#endif
}

//startup methods
void HelicopterRTD::UpdateStarter()
{ 
#if !_DUMMY_ROTORLIB
  const HelicopterType* type = Type();
  int newValue = 0;
  //disable starter if battery is drained  
  float batteryCharge = _batteryCharge / Type()->_maxBatteryCharge;
  if(batteryCharge < 0.05 && !_APU) _starterWanted = 0;

  if ((_batteryOn || _APU) && _rtdHelicopter &&  !_rtdHelicopter->GetRotorBrake()) newValue = _starterWanted;
  else newValue = 0;
  
  if( type->_hitStarter1 >= 0 && _hit[type->_hitStarter1] > 0.95) newValue = newValue & 254;
  if( type->_hitStarter2 >= 0 && _hit[type->_hitStarter2] > 0.95) newValue = newValue & 253;
  if( type->_hitStarter3 >= 0 && _hit[type->_hitStarter3] > 0.95) newValue = newValue & 251;

  int oldValue = _starter;
  _starter = newValue;

  //DIAG_MESSAGE(100, Format("Starter 0: %d, 1: %d, 2: %d",
  //  GetStarter(0) ? 1 : 0,
  //  GetStarter(1) ? 1 : 0,
  //  GetStarter(2) ? 1 : 0));

  for (int i = 0; i < lenof(_rpmStarter); i++)
  {
    bool oldState = GetStarter(oldValue, i);
    bool newState = GetStarter(i);

    if (oldState != newState)
    {
      if (newState)
      {
        // set to ON
        PlayStarterSound(true, i);
      }
      else
      {
        // set to OFF
        PlayStarterSound(false, i);

        float currRPM = GetRPM(i);
        float maxStarterRPM = _rtdHelicopter ? _rtdHelicopter->GetEngineTargetRPM(i) * 0.250f : 0;

        _rpmStarter[i] = floatMin(currRPM, toInt(maxStarterRPM));
        _rpmStarterOffTime[i] = (_rpmStarter[i] / maxStarterRPM) *  10.0f;
      }
    }
  }
#endif
};

void HelicopterRTD::SetStarterWanted(bool starter, int index) 
{ 
#if !_DUMMY_ROTORLIB
  if(index >= 0 && index < 3)
  {
    if(starter) _starterWanted = _starterWanted | (1 << index);
    else  _starterWanted = _starterWanted & (255 -  (1 << index));

  }
  else if(index < 0)
  {
    for (int i = 0; i < _rtdHelicopter->GetNumberOfEngines(); i++)
    {
      if(starter) _starterWanted = _starterWanted | (1 << i);
      else  _starterWanted = _starterWanted & (255 -  (1 << i));
    }
  }
#endif
};

void HelicopterRTD::SetAPU(float APU, float time) 
{ 
  if(!Type()->_APU) return;
  //no elctricity, no APU
  if(IsElectricity() == false && APU > 0) return;
#if _HELISIM_SOUNDS
  PlayApuSound(APU > 0.0f);
#endif
  if(time <= 0) 
  {
    _APU = APU;
    _APUwanted = APU;
    _APUStartingTime = 0;
    return;
  }
  _APUwanted = APU; _APUStartingTime = time; _APUchange = (APU - _APU) / time; 
};

void HelicopterRTD::SetBatteryOn(bool batteryOn) 
{ 
  _batteryOn = batteryOn;
};

HelicopterRTD::~HelicopterRTD()
{
#if !_DUMMY_ROTORLIB
  //remove RTD helicopter from world and delete it
  if(_rtdHelicopter)
  {
     /** Deletes this world participant. Classes that are derived from WorldParticipant can not be 
			deleted with delete operator. Instead, this method must be used.

			In case deleting is dangerous (e.g. deleted participant might be referred from others), deletion
			might fail. In this case, false is returned.

			Note that all world participants will be deleted automatically when World is deleted. Hence,
			most of the time deleting explicitly is not necessary. 
		*/
    _rtdHelicopter->remove(); 
    /*
    _rtdHelicopter is set to NULL by ~GenHeli600Arma()
    */
    _autoHoverRTD = NULL;
  }
#endif
}

#if !_DUMMY_ROTORLIB
//remove force generators, tell parent if RTD deleted me (can happen if _rtdWorld is destroyed)
GenHeli600Arma::~GenHeli600Arma()
{
  RemoveForces();
  if(_parent) _parent->RemoveRTDHelicopter();
}
#endif

#if !_DUMMY_ROTORLIB
#include <limits>
#endif

LSError HelicopterRTD::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("actualCollectiveRTD", _actualCollectiveRTD, 1, 0.0))
  CHECK(ar.Serialize("collectiveWanted", _collectiveWanted, 1, 0.0))
  CHECK(ar.Serialize("autoTrimEnabled", _autoTrimEnabled, 1, false))
  CHECK(ar.Serialize("controllsTrim", _controllsTrim, 1, false))

  CHECK(ar.Serialize("cyclicSideRTDOffset", _cyclicSideRTDOffset, 1, 0.0))
  CHECK(ar.Serialize("cyclicForwardRTDOffset", _cyclicForwardRTDOffset, 1, 0.0))
  CHECK(ar.Serialize("backRotorRTDOffset", _backRotorRTDOffset, 1, 0.0))

  CHECK(ar.Serialize("enginesTemperature", _enginesTemperature, 1))


  if(ar.IsLoading())
  {
    _rtdStateAge = 0;
#if !_DUMMY_ROTORLIB
    GWorld->GetRTDWorld()->ClearAccumulatedSimulationTime();
#endif
    _delayAfterTrimming = TIME_MIN;
    _lastHit.Clear();
    if(ar.GetPass() ==  ParamArchive::PassSecond)
    {
      for(int i = 0; i< _hit.Size(); i++) _lastHit.Add(0);

#if !_DUMMY_ROTORLIB
      if(_rtdHelicopter)
      {
        int countContr = intMin(2, _rtdHelicopter->getNumCSComponents());
        for(int i = 0; i< countContr; i++)
        {
          if(strcmpi(_rtdHelicopter->getCSComponent(i)->getType().data(), "FCSHelicopterAutoTrim100") == 0)
          { 
            RotorLibFDM::FCSHelicopterAutoTrim100* autoTrimRTD = (RotorLibFDM::FCSHelicopterAutoTrim100*)_rtdHelicopter->getCSComponent(i);
            if ( autoTrimRTD ) _autoTrimEnabledState = autoTrimRTD->isEnabled();
            break;
          }
        }
      }
#endif
    }
  }

  base::Serialize(ar);
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->Serialize(ar);
#endif

 CHECK(ar.Serialize("starter", _starter, 1, 0))
 CHECK(ar.Serialize("starterWanted", _starterWanted, 1, 0))
 CHECK(::Serialize(ar, "starterTime", _starterTime, 1, VZero))

 CHECK(ar.Serialize("APU", _APU, 1, 0.0))
 CHECK(ar.Serialize("batteryOn" , _batteryOn, 1, false))
 CHECK(ar.Serialize("batteryCharge" , _batteryCharge, 1, 1.0))
 CHECK(ar.Serialize("isAutoStartUpEnabledRTD",_isAutoStartUpEnabledRTD, 1, true));

  //APU starting
 CHECK(ar.Serialize("APUStartingTime", _APUStartingTime, 1, 0.0))
 CHECK(ar.Serialize("APUwanted", _APUwanted, 1, 0.0))
 CHECK(ar.Serialize("APUchange", _APUchange, 1, 0.0))

 CHECK(ar.Serialize("fuselageWeight", _fuselageWeight, 1, 0.0))
 CHECK(ar.Serialize("crewWeight",_crewWeight, 1, 0.0))
 CHECK(ar.Serialize("customWeight",_customWeight, 1, 0.0))
 CHECK(ar.Serialize("ammoWeight",_ammoWeight, 1, 0.0))

 CHECK(ar.Serialize("isRTDSimulated",_isRTDSimulated, 1, false))
 CHECK(ar.Serialize("isRTDSimulatedWanted",_isRTDSimulatedWanted, 1, false))
 //CHECK(ar.Serialize("timeToForcedChange",_timeToForcedChange, 1, 0))
#if !_DUMMY_ROTORLIB
 if(ar.IsLoading() && _rtdHelicopter) _rtdHelicopter->setEnabled(_isRTDSimulated);
#endif
 //CHECK(ar.Serialize("rpmStarterOffTime", _rpmStarterOffTime, 1, 0))

  return LSOK;
}

void HelicopterRTD::SetTotalDamageHandleDead(float damage, bool setHitpoints)
{
  base::SetTotalDamageHandleDead(damage,setHitpoints);
#if !_DUMMY_ROTORLIB
  if (_rtdHelicopter)
  {
    if(damage < 0.9f) 
    {
      _rtdHelicopter->setDamageState(RTD::VehicleBase::Undamaged);
    }
    else
    {
      _rtdHelicopter->setDamageState(RTD::VehicleBase::Crashed);
      EngineOff();
    }
  }
#endif
}

bool HelicopterRTD::EngineIsOn() const
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    return _rtdHelicopter->EngineIsOn();
  }
#endif
  return _rotorSpeedWanted>0.5f;
}

bool HelicopterRTD::EngineIsOn(int index) const
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    return _rtdHelicopter->EngineIsOn(index);
  }
#endif
  return _rotorSpeedWanted>0.5f;
}

void HelicopterRTD::EngineOn()
{
  base::EngineOn();
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->EngineOn();
#endif
}

void HelicopterRTD::EngineOn(bool quickStart)
{
  base::EngineOn();
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->EngineOn(quickStart? -1.0f : 15);
#endif
}

void HelicopterRTD::EngineOff()
{
  _rotorSpeedWanted=0;
  PlaneOrHeli::EngineOff();
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->EngineOff();
#endif
}

void HelicopterRTD::EngineOffAction()
{
  //player has to switch engine off manually
  if(!_rtdHelicopter || !_driver || !_driver->Brain() || !_driver->Brain()->IsAnyPlayer())
  {
    base::EngineOffAction();
#if !_DUMMY_ROTORLIB
    if(_rtdHelicopter) _rtdHelicopter->EngineOff();
#endif
  }
}

void HelicopterRTD::Diag_quickStartRTDHeli()
{
  base::EngineOn();
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->EngineOn(-1.0f);
#endif
}

float HelicopterRTD::MakeAirborne(bool playerIn)
{ 
  _actualCollectiveRTD = Type()->_defaultCollective;
  _collectiveWanted = Type()->_defaultCollective;;
  SetBatteryOn(true);
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter /*&& playerIn*/)
  {
    _rtdHelicopter->getPilotInputData()->m_collective = _actualCollectiveRTD;
    //smVec3r pos = _rtdHelicopter->getPosition();
    // !! trim take up to 5 seconds !!
    //_rtdHelicopter->trim(smVec3d(pos[0],pos[1],pos[2]), smVec3r(0,0,5),30);
    //_rtdHelicopter->setEnabled(_isRTDSimulated);
  }
#endif
  EngineOn(true);
  return base::MakeAirborne(playerIn);
}

void HelicopterRTD::ResetHelicopterRTD() {
  _rotorSpeedWanted=0; _engineOff = true; if(_holder) { _holder->Unload(); _holder->SetEngineState(_engineOff);}
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->InstantStop();
#endif
  CopyTransformArmaToRTD();
  //reset damage 
  for (int i=0; i< _lastHit.Size(); i++) _lastHit[i] = 0;
};

/// returns true if object is touching ground
bool HelicopterRTD::IsLandContact()
{
#if !_DUMMY_ROTORLIB
  if(!_rtdHelicopter || !_isRTDSimulated) return _landContact;
  else return  _landContact || _rtdHelicopter->IsLandContact();
#else
  return _landContact;
#endif
};

float HelicopterRTD::GetRadarAltMeterPosition()
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated && _rtdHelicopter->getNumGroundContacts() > 0) 
  {
    const RTD::FGGroundContactBase* groundContact = _rtdHelicopter->getGroundContact(0);
    RTD::FGGroundContact200* groundContact200 = (RTD::FGGroundContact200*) groundContact;

    if ( groundContact200 )  return groundContact200->getWheelPositionWCS().z(); 
  }
#endif
  return FutureVisualState().Position().Y();
}

int HelicopterRTD::GetNumberOfEnginesRTD()
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) return _rtdHelicopter->GetNumberOfEngines();
#endif
  return 0.0f;
};


float HelicopterRTD::GetAverageRPMScaled()
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated) return _rtdHelicopter->GetAverageRPMScaled();
#endif
  return 0;
};

float HelicopterRTD::GetThrottleRTD(int engineIndex)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) return _rtdHelicopter->GetThrottle(engineIndex);
#endif
  return 0;
}

void HelicopterRTD::GetThrottleRTD(AutoArray<float> &values)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->GetThrottle(values);
#endif
};

void HelicopterRTD::GetEnginesTemperatureRTD(AutoArray<float> &values)
{
  for (int i=0; i< _enginesTemperature.Size(); i++)
  {
      values.Add(_enginesTemperature[i].GetTemperature());
  } 
};

void HelicopterRTD::DecreasetEngineTemperatureRTD()
{
  for (int i=0; i< _enginesTemperature.Size(); i++)
  {
    _enginesTemperature[i].SetHot(this, i);
  } 
};

void HelicopterRTD::GetEnginesOnRTD(AutoArray<bool> &values)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->GetEnginesOn(values);
#endif
};

void HelicopterRTD::GetEnginesPowerRTD(AutoArray<float> &values)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->GetEnginesPower(values);
#endif
};

void HelicopterRTD::GetEnginesTorqueRTD(AutoArray<float> &values)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->GetEnginesTorque(values);
#endif
};

Vector3 HelicopterRTD::GetRotorsRpmRTD()
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) return _rtdHelicopter->GetRotorsRpm();
#endif
  return Vector3();
};

void HelicopterRTD::GetRotorsForcesRTD(AutoArray<Vector3> &values)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->GetRotorsForces(values);
#endif
};

void HelicopterRTD::GetWingsForcesRTD(AutoArray<Vector3> &values)
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->GetWingsForces(values);
#endif
};


// overwritten object methods
void HelicopterRTD::Move(Matrix4Par transform)
{
  base::Move(transform);
  CopyTransformArmaToRTD();
}

void HelicopterRTD::Move(Vector3Par position)
{
  base::Move(position);
  CopyTransformArmaToRTD();
}

void HelicopterRTD::SetOrient(Matrix3 const& dir)
{
  base::SetOrient(dir);
  CopyRotationArmaToRTD();
}

void HelicopterRTD::SetDirectionAndUp( Vector3PPar dir, Vector3PPar up )
{
  base::SetDirectionAndUp(dir, up);
  CopyRotationArmaToRTD();
}

void HelicopterRTD::SetOrientation( Matrix3P const& m ) 
{   
  base::SetOrientation(m);
  CopyRotationArmaToRTD();
}

void HelicopterRTD::SetUpAndDirection( Vector3PPar up, Vector3PPar dir ) 
{ 
  base::SetUpAndDirection(up, dir);
  CopyRotationArmaToRTD();
}

#define HSIM_SOUNDS_CONE 1

void HelicopterRTD::Sound(bool inside, float deltaT)
{
#if _HELISIM_SOUNDS && !_DUMMY_ROTORLIB
  if (_rtdHelicopter)
  {
    TransportSoundContex tsc;
    tsc._transport = this;
    tsc._ctrl = _soundControllers.Data();
    tsc._ctrlCount = _soundControllers.Size();
    tsc._inside = inside;
    tsc._deltaT = deltaT;
    tsc._pos = RenderVisualState().PositionModelToWorld(GetEnginePos());
    tsc._speed = RenderVisualState().Speed();
    tsc._obstruction = 1.0f;
    tsc._occlusion = 1.0f;

    //bool doorsOpened = false;

    //// internal sounds when doors are opened
    //if (tsc._inside)
    //{
    //  const DoorsHolder *doors = Type()->GetDoors();

    //  if (doors)
    //  {
    //    int doorsCount = doors->Count();

    //    for (int i = 0; i < doorsCount; i++)
    //    {
    //      InitPtr<AnimationSourceDoor> dPtr = (*doors)[i];

    //      if (dPtr.NotNull())
    //      {
    //        float phase = RenderVisualState().GetDoorState(dPtr->GetIndex());

    //        if (phase > 0.5f)
    //        {
    //          doorsOpened = true;
    //          tsc._inside = false;
    //          break;
    //        }
    //      }
    //    }
    //  }
    //}

    //GSoundsys->SetCustomVoiceFilter(doorsOpened);

    // we currently assume the engine sound is correct outside however we do not want to occlude it inside,
    // we only change its volume instead
    if (!tsc._inside)
      GWorld->CheckSoundObstruction(this, tsc._inside, tsc._obstruction, tsc._occlusion); 

    // initialize soundExt stuff
    if (!_se._submix)
    {
      _se.Destroy();

      _se._submix = GSoundsys->CreateSubmix();
      if (_se._submix)
      {
        _se._submix->SetOutputVoiceSound(CustomFilterOutput);

        const HelicopterType::SoundParsExt &spe = Type()->_spe;

        if (spe._apuSoundPars)
          _se._apuSound = new StartStopSound(*(spe._apuSoundPars.GetRef()), _se._submix);

        if (spe._starterSoundPars)
          _se._starterSound = new StartStopSound(*(spe._starterSoundPars.GetRef()), _se._submix);

        if (spe._starterSoundPars1)
          _se._starterSound1 = new StartStopSound(*(spe._starterSoundPars1.GetRef()), _se._submix);

        if (spe._noiseSoundPars)
          _se._noisesSound = new NoisesSound(*(spe._noiseSoundPars.GetRef()));

        if (spe._landingSoundPars)
          _se._landingSound = new LandingSounds(*(spe._landingSoundPars.GetRef()), _se._submix);

        if (spe._starterClickPars)
          _se._starterClickSound = new IntExtSoundHolder(spe._starterClickPars[0], spe._starterClickPars[1], _se._submix, true, true);

        if (spe._idleThrottlePars)
        {
          _se._idleThrottleSound = new IntExtSoundHolder(spe._idleThrottlePars[0], spe._idleThrottlePars[1], _se._submix);
          _se._idleThrottleSound1 = new IntExtSoundHolder(spe._idleThrottlePars[0], spe._idleThrottlePars[1], _se._submix);
        }

        if (spe._rotorDamagePars)
          _se._rotorDamageSound = new IntExtSoundHolder(spe._rotorDamagePars[0], spe._rotorDamagePars[1], _se._submix, true, false);

        if (spe._externNoisePars)
          _se._externNoise = new ExternNoise(*(spe._externNoisePars.GetRef()));

        _se._gearUpDownSoundWanted = false;
      }
    }

    float volume = 0.0f;
    float distance = 0.0f;
    int sCount = 0;

    if (_se._starterSound)
      _se._starterSound->Update(tsc, distance, volume, sCount);

    if (_se._starterSound1)
      _se._starterSound1->Update(tsc, distance, volume, sCount);

    if (_se._apuSound)
      _se._apuSound->Update(tsc, distance, volume, sCount);

    if (_se._noisesSound)
      _se._noisesSound->Update(tsc);

    if (_se._landingSound)
      _se._landingSound->Update(tsc, distance, volume, sCount);

    if (_se._starterClickSound)
      _se._starterClickSound->Update(tsc, volume, distance, sCount);

    if (_se._idleThrottleSound)
      _se._idleThrottleSound->Update(tsc, volume, distance, sCount);

    if (_se._idleThrottleSound1)
      _se._idleThrottleSound1->Update(tsc, volume, distance, sCount);

    if (_se._gearUpDownSoundWanted)
    {
      const SoundPars &pars = Type()->_soundLandingGearPars;

      _se._gearUpDownSound = GSoundScene->OpenAndPlayOnce(cc_cast(pars.name), tsc._transport, tsc._inside, tsc._pos,
        tsc._speed, pars.vol, pars.freq);

      _se._gearUpDownSoundWanted = false;
    }

    if (_se._submix && sCount)
      _se._submix->Calculate3D(!tsc._inside, tsc._pos, tsc._speed, distance / (float) sCount, volume / (float) sCount);

#if HSIM_SOUNDS_CONE
    ExternNoise *en = _se._externNoise.GetRef();
    if (en && (!tsc._inside || en->IsPlaying()))
    {
      if (GSoundScene->CanBeAudible(WaveEffect, tsc._pos, en->AudibleDistance()))
      {
        Vector3 camPos = GScene->GetCamera()->Position();
        Matrix4 invModelToWorld = FutureVisualState().ModelToWorld().InverseRotation();

        Vector3 modelPos = invModelToWorld.FastTransform(camPos);
        Vector3 fDir = FutureVisualState().DirectionModelToWorld(Vector3(modelPos.X(), modelPos.Y(), 0));
        fDir.Normalize();
        Vector3 fTop = FutureVisualState().DirectionModelToWorld(Vector3(modelPos.Y(), -modelPos.X(), 0));        
        fTop.Normalize();

        /*AddForce(tsc._pos, 10* fDir, Color(1,0,0));
        AddForce(tsc._pos, 10*fTop, Color(0,1,0));*/

        en->Update(tsc, fDir, fTop);
      }
    }
#endif

    /* ----------------------------------------------- */

    // audio controllers initialization

    const AutoArray<SubmixPars> &submixPars = Type()->_spe._submixPars;

    if (_se._sounds.Size() == 0)
    {
      int subParsCount = submixPars.Size();

      if (subParsCount)
      {
        _se._sounds.Realloc(subParsCount);
        _se._sounds.Resize(subParsCount);

        for (int i = 0; i < _se._sounds.Size(); i++)
        {
          _se._sounds[i] = new SoundsHolder(submixPars[i].Size());
          
          if (_se._sounds[i])
            _se._sounds[i]->SetOutputVoiceSound(CustomFilterOutput);
        }
      }
    }

    for (int i = 0; i < _se._sounds.Size(); i++)
    {
      if (_se._sounds[i])
        _se._sounds[i]->UpdateSubmixWaves(submixPars[i], tsc);
    }
  }
#endif

  base::Sound(inside, deltaT);
}

void HelicopterRTD::UnloadSound()
{
#if _HELISIM_SOUNDS
  _se.Destroy();
#endif

  base::UnloadSound();
}

void HelicopterRTD::SetSpeed(Vector3Par speed)
{
  base::SetSpeed(speed); 
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter)
    _rtdHelicopter->setVelocity(smVec3r(speed[0],speed[2],speed[1]));
#endif
} // camera needs to know object speed

void HelicopterRTD::DisableSimulation(bool disable)
{
#if !_DUMMY_ROTORLIB
  if (_rtdHelicopter)
  {
    if(disable) _rtdHelicopter->setEnabled(false);
    else _rtdHelicopter->setEnabled(_isRTDSimulated);
  }
#endif
  base::DisableSimulation(disable);
}

float HelicopterRTD::StepSimulated(float step) 
{
#if !_DUMMY_ROTORLIB
  return _isRTDSimulated? GWorld->GetRTDWorld()->GetTimeToSimulateRest()  : step;
#else
  return step;
#endif
};

void HelicopterRTD::CopyTransformArmaToRTD()
{
#if !_DUMMY_ROTORLIB
  if(!_rtdHelicopter) return;
  Matrix4Par armaTransform = FutureVisualState().Transform();

  smMatrix rtdRotation = _rtdHelicopter->getRotation();
  smVec3r rtdPosition;

  rtdRotation[2][0] = armaTransform.DirectionUp().X();
  rtdRotation[2][2] = armaTransform.DirectionUp().Y();
  rtdRotation[2][1] = armaTransform.DirectionUp().Z();

  rtdRotation[1][0] = armaTransform.Direction().X();
  rtdRotation[1][2] = armaTransform.Direction().Y();
  rtdRotation[1][1] = armaTransform.Direction().Z();

  rtdRotation[0][0] = armaTransform.DirectionAside().X();
  rtdRotation[0][2] = armaTransform.DirectionAside().Y();
  rtdRotation[0][1] = armaTransform.DirectionAside().Z();

  Vector3 centerOffsetRTDRotated = FutureVisualState().Transform().Rotate(_centerOffsetRTD);

  rtdPosition[0] = armaTransform.Position().X() + centerOffsetRTDRotated.X();
  rtdPosition[2] = armaTransform.Position().Y() + centerOffsetRTDRotated.Y();
  rtdPosition[1] = armaTransform.Position().Z() + centerOffsetRTDRotated.Z();

  _rtdHelicopter->setPosition(rtdPosition);
  smAngleDVec angle;
  rtdRotation.getHPR(angle);
  _rtdHelicopter->setRotation(angle);
#endif
}


// t = interpolation coefficient: 0 - use old FutureVisualState, 1 = copy plain RTD state
void HelicopterRTD::CopyTransformRTDToArma(float t)
{
#if !_DUMMY_ROTORLIB
  if(!_rtdHelicopter) return;
  smMatrix rota = _rtdHelicopter->getRotation();

  smVec3r upRtd, forwardRtd;
  _rtdHelicopter->getForwardVector(forwardRtd);
  _rtdHelicopter->getUpwardVector(upRtd);

  Vector3 up = Vector3( upRtd[0],upRtd[2], upRtd[1]);
  Vector3 dir = Vector3( forwardRtd[0],forwardRtd[2], forwardRtd[1]);
  Matrix4 newPosMatrix;
  newPosMatrix.SetDirectionAndUp(dir, up);

  smVec3r posRTD = _rtdHelicopter->getPosition();
  Vector3 centerOffsetRTDRotated = newPosMatrix.Rotate(_centerOffsetRTD);

  Vector3 newPosition = Vector3(
    posRTD[0] - centerOffsetRTDRotated.X(),
    posRTD[2] - centerOffsetRTDRotated.Y(),
    posRTD[1] - centerOffsetRTDRotated.Z());

  newPosMatrix.SetPosition(newPosition);

  Matrix4 interpolatedResult;
  VisualState &vs = FutureVisualState();

  // Interpolate position.
  interpolatedResult.SetPosition(vs.Position() + (newPosMatrix.Position() - vs.Position()) * t);

  // Interpolate rotation. Assume an orthonormal xform.
  if (vs.Orientation() != newPosMatrix.Orientation())
  {
    Quaternion<float> q0(vs.Orientation()), q1(newPosMatrix.Orientation());  // convert to quaternions, lerp + normalize, convert back to matrices
    Matrix3 interpolated;
    q0.Slerp(q1,t).UnitToMatrix(interpolated);
    interpolatedResult.SetOrientation(interpolated);
  }
  else  // states are equal
  {
    interpolatedResult.SetOrientation(vs.Orientation());
  }

  OnMoved();

  StabilizeTurrets(vs.Transform().Orientation(), interpolatedResult.Orientation());
  if (IsLocal()) base::Move(interpolatedResult);
  else GetNetworkManager().AskForMove(this,interpolatedResult);

  base::OnPositionChanged();
  GScene->GetShadowCache().ShadowChanged(this);
#endif
}

void HelicopterRTD::CopyRotationArmaToRTD()
{
#if !_DUMMY_ROTORLIB
  if(!_rtdHelicopter) return;
  Matrix4Par armaTransform = FutureVisualState().Transform();
  smMatrix rtdRotation = _rtdHelicopter->getRotation();

  rtdRotation[2][0] = armaTransform.DirectionUp().X();
  rtdRotation[2][2] = armaTransform.DirectionUp().Y();
  rtdRotation[2][1] = armaTransform.DirectionUp().Z();

  rtdRotation[1][0] = armaTransform.Direction().X();
  rtdRotation[1][2] = armaTransform.Direction().Y();
  rtdRotation[1][1] = armaTransform.Direction().Z();

  rtdRotation[0][0] = armaTransform.DirectionAside().X();
  rtdRotation[0][2] = armaTransform.DirectionAside().Y();
  rtdRotation[0][1] = armaTransform.DirectionAside().Z();

  smAngleDVec angle;
  rtdRotation.getHPR(angle);
  _rtdHelicopter->setRotation(angle);
#endif
}


bool HelicopterRTD::IsRTDSimulationWanted()
{ //rtd simulation should be used, but it is not yet (RTD - ARMA sim. change not yet done)
#if !_DUMMY_ROTORLIB
  if(!_rtdHelicopter) return false;
  return ((PilotUnit() && PilotUnit()->IsPlayer()) || (CopilotUnit() && CopilotUnit()->IsPlayer()));
#else
  return false;
#endif
}

void HelicopterRTD::AutohoverOn()
{
  base::AutohoverOn();
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter)
  {
    int countContr = intMin(2, _rtdHelicopter->getNumCSComponents());
    for(int i = 0; i< countContr; i++)
    {
      if(strcmpi(_rtdHelicopter->getCSComponent(i)->getType().data(), "FCSHelicopterAutoTrim100") == 0)
      { 
        RotorLibFDM::FCSHelicopterAutoTrim100* autoTrimRTD = (RotorLibFDM::FCSHelicopterAutoTrim100*)_rtdHelicopter->getCSComponent(i);
        if ( autoTrimRTD ) 
        {
          autoTrimRTD->enable( true );
          _autoTrimEnabledState = true;
          SetAutoHover(true);
        }
        break;
      }
    }
  }
#endif
}

void HelicopterRTD::AutohoverOff()
{
  base::AutohoverOff();
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter)
  {
    int countContr = intMin(2, _rtdHelicopter->getNumCSComponents());
    for(int i = 0; i< countContr; i++)
    {
      if(strcmpi(_rtdHelicopter->getCSComponent(i)->getType().data(), "FCSHelicopterAutoTrim100") == 0)
      { 
        RotorLibFDM::FCSHelicopterAutoTrim100* autoTrimRTD = (RotorLibFDM::FCSHelicopterAutoTrim100*)_rtdHelicopter->getCSComponent(i);
        if ( autoTrimRTD ) 
        {
          autoTrimRTD->enable(_autoTrimEnabled || Glob.config.AutoTrimEnabled());
          _autoTrimEnabledState = false;
          SetAutoHover(false);
        }
        break;
      }
    }
  }
#endif
}

void HelicopterRTD::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  if(
    ((PilotUnit() && PilotUnit()->IsPlayer()) || (CopilotUnit() && CopilotUnit()->IsPlayer()) )
    )
  {

    if( (!PilotUnit() || !PilotUnit()->IsPlayer()) || !CopilotUnit())
    {
#if !_DUMMY_ROTORLIB
      if(_rtdHelicopter)
      {
        if (Type()->_gearRetracting /*&& !_landContact*/)
        {
          UIAction action(new ActionBasic( GetPilotGear() ? ATLandGearUp : ATLandGear, this));
          add(action);
        }

        if(_isAutoStartUpEnabledRTD)
        {//action is scripted, _isAutoStartUpEnabledRTD is used start up is in progress
          if(EngineIsOn())
          {
            UIAction action(new ActionBasic(ATEnginesOffAuto, this));
            add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATEnginesOnAuto, this));
            add(action);
          }
        }
        //controls trim
        if(_controllsTrim)
        {
          UIAction action(new ActionBasic(ATHelicopterTrimOff, this));
          add(action);
        }
        else
        {
          UIAction action(new ActionBasic(ATHelicopterTrimOn, this));
          add(action);
        }
        //battery
        if(_batteryOn)
        {
          UIAction action(new ActionBasic(ATBatteriesOff, this));
          add(action);
        }
        else
        {
          UIAction action(new ActionBasic(ATBatteriesOn, this));
          add(action);
        }
        //APU
        if(Type()->_APU)
        {
          if(_APU > 0.0001)
          {
            UIAction action(new ActionBasic(ATAPUOff, this));
            add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATAPUOn, this));
            add(action);
          }
        }
        //rotor brake
        if(_rtdHelicopter->GetRotorBrake() > 0.0001)
        {
          UIAction action(new ActionBasic(ATRotorBrakeOff, this));
          add(action);
        }
        else
        {
          UIAction action(new ActionBasic(ATRotorBrakeOn, this));
          add(action);
        }


        if(_isRTDGearBrakeAble)
        { 
          //wheels brake
          if(_rtdHelicopter->GetBrakeLeft() > 0.0001 || _rtdHelicopter->GetBrakeRight() > 0.0001)
          {
            UIAction action(new ActionBasic(ATWheelsBrakeOff, this));
            add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATWheelsBrakeOn, this));
            add(action);
          }
          //gears up / down
          //UIAction action(new ActionBasic(FutureVisualState().GetGearPos(Type()) > 0.5f ? ATLandGear : ATLandGearUp, this));
          //actions.Add(action);
        }
        //----------------------------- engine 1 -----------------------------
        if(_rtdHelicopter->GetNumberOfEngines() > 0)
        {

          if(_starterWanted & 1)
          {
            UIAction action(new ActionBasic(ATStarterOff1, this));
            add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATStarterOn1, this));
            add(action);
          }

          float throttle = _rtdHelicopter->GetThrottle(0);
          if(throttle < 0.25f && throttle > 0.05f) //idle
          {
            UIAction action(new ActionBasic(ATThrottleOff1, this));
            add(action);
            UIAction action2(new ActionBasic(ATThrottleFull1, this));
            add(action2);
          }
          else if(throttle > 0.25f) //full
          {
            UIAction action(new ActionBasic(ATThrottleIdle1, this));
            add(action);
          }
          else if(throttle < 0.0001f) //off
          {
            UIAction action(new ActionBasic(ATThrottleIdle1, this));
            add(action);
          }

        }

        //----------------------------- engine 2 -----------------------------
        if(_rtdHelicopter->GetNumberOfEngines() > 1)
        {
          if(_starterWanted & 2)
          {
            UIAction action(new ActionBasic(ATStarterOff2, this));
            add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATStarterOn2, this));
            add(action);
          }

          float throttle = _rtdHelicopter->GetThrottle(1);
          if(throttle < 0.25f && throttle > 0.05f) //idle
          {
            UIAction action(new ActionBasic(ATThrottleOff2, this));
            add(action);
            UIAction action2(new ActionBasic(ATThrottleFull2, this));
            add(action2);
          }
          else if(throttle > 0.25f) //full
          {
            UIAction action(new ActionBasic(ATThrottleIdle2, this));
            add(action);
          }
          else if(throttle < 0.0001f) //off
          {
            UIAction action(new ActionBasic(ATThrottleIdle2, this));
            add(action);
          }

        }
        //----------------------------- engine 3 -----------------------------
        if(_rtdHelicopter->GetNumberOfEngines() > 2)
        {
          if(_starterWanted & 4)
          {
            UIAction action(new ActionBasic(ATStarterOff3, this));
            add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATStarterOn3, this));
            add(action);
          }

          float throttle = _rtdHelicopter->GetThrottle(2);
          if(throttle < 0.25f && throttle > 0.05f) //idle
          {
            UIAction action(new ActionBasic(ATThrottleOff3, this));
            add(action);
            UIAction action2(new ActionBasic(ATThrottleFull3, this));
            add(action2);
          }
          else if(throttle > 0.25f) //full
          {
            UIAction action(new ActionBasic(ATThrottleIdle3, this));
            add(action);
          }
          else if(throttle < 0.0001f) //off
          {
            UIAction action(new ActionBasic(ATThrottleIdle3, this));
            add(action);
          }
        }
      }
#endif
      if (unit->GetVehicle() == this && !unit->IsInCargo())
      {
        if (_collisionLight)
        {
          UIAction action(new ActionBasic(ATCollisionLightOff, this));
          add(action);
        }
        else
        {
          UIAction action(new ActionBasic(ATCollisionLightOn, this));
          add(action);
        }
      }
    }
  }



  base::GetActions(add, unit);
}
#if _HELISIM_PRECISE_INTERACTION
void HelicopterRTD::GetDrawActions3D(UIActions &actions, AIBrain *unit)
{

  if(unit->GetVehicle() == this) 
  {
    if ((PilotUnit() && PilotUnit()->IsPlayer()) || (CopilotUnit() && CopilotUnit()->IsPlayer()))
    {
      if( (!PilotUnit() || !PilotUnit()->IsPlayer()) || !CopilotUnit())
      {

        if (unit->GetVehicle() == this && !unit->IsInCargo())
        {
          if (_collisionLight)
          {
            UIAction action(new ActionBasic(ATCollisionLightOff, this));
            actions.Add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATCollisionLightOn, this));
            actions.Add(action);
          }
        }

#if !_DUMMY_ROTORLIB
        if(_rtdHelicopter)
        {

          if (Type()->_gearRetracting /*&& !_landContact*/)
          {
            UIAction action(new ActionBasic( GetPilotGear() ? ATLandGearUp : ATLandGear, this));
            actions.Add(action);
          }

          if(_controllsTrim)
          {
            UIAction action(new ActionBasic(ATHelicopterTrimOff, this));
            actions.Add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATHelicopterTrimOn, this));
            actions.Add(action);
          }


          if(EngineIsOn())
          {
            UIAction action(new ActionBasic(ATEnginesOffAuto, this));
            actions.Add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATEnginesOnAuto, this));
            actions.Add(action);
          }

          if(_autoTrimEnabled)
          {
            UIAction action(new ActionBasic(ATHelicopterAutoTrimOff, this));
            actions.Add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATHelicopterAutoTrimOn, this));
            actions.Add(action);
          }

          if(_batteryOn)
          {
            UIAction action(new ActionBasic(ATBatteriesOff, this));
            actions.Add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATBatteriesOn, this));
            actions.Add(action);
          }

          if(Type()->_APU)
          {
            if(_APU > 0.0001)
            {
              UIAction action(new ActionBasic(ATAPUOff, this));
              actions.Add(action);
            }
            else
            {
              UIAction action(new ActionBasic(ATAPUOn, this));
              actions.Add(action);
            }
          }

          if(_rtdHelicopter->GetRotorBrake() > 0.0001)
          {
            UIAction action(new ActionBasic(ATRotorBrakeOff, this));
            actions.Add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATRotorBrakeOn, this));
            actions.Add(action);
          }

                  if(_isRTDGearBrakeAble)
        { 
          //wheels brake
          if(_rtdHelicopter->GetBrakeLeft() > 0.0001 || _rtdHelicopter->GetBrakeRight() > 0.0001)
          {
            UIAction action(new ActionBasic(ATWheelsBrakeOff, this));
            actions.Add(action);
          }
          else
          {
            UIAction action(new ActionBasic(ATWheelsBrakeOn, this));
            actions.Add(action);
          }
          //gears up / down
          //UIAction action(new ActionBasic(FutureVisualState().GetGearPos(Type()) > 0.5f ? ATLandGear : ATLandGearUp, this));
          //actions.Add(action);
        }

          //----------------------------- engine 1 -----------------------------
          if(_rtdHelicopter->GetNumberOfEngines() > 0)
          {

            if(_starterWanted & 1)
            {
              UIAction action(new ActionBasic(ATStarterOff1, this));
              actions.Add(action);
            }
            else
            {
              UIAction action(new ActionBasic(ATStarterOn1, this));
              actions.Add(action);
            }

            float throttle = _rtdHelicopter->GetThrottle(0);
            if(throttle < 0.25f && throttle > 0.05f) //idle
            {
              UIAction action(new ActionBasic(ATThrottleOff1, this));
              actions.Add(action);
              UIAction action2(new ActionBasic(ATThrottleFull1, this));
              actions.Add(action2);
            }
            else if(throttle > 0.25f) //full
            {
              UIAction action(new ActionBasic(ATThrottleIdle1, this));
              actions.Add(action);
            }
            else if(throttle < 0.0001f) //off
            {
              UIAction action(new ActionBasic(ATThrottleIdle1, this));
              actions.Add(action);
            }

          }

          //----------------------------- engine 2 -----------------------------
          if(_rtdHelicopter->GetNumberOfEngines() > 1)
          {
            if(_starterWanted & 2)
            {
              UIAction action(new ActionBasic(ATStarterOff2, this));
              actions.Add(action);
            }
            else
            {
              UIAction action(new ActionBasic(ATStarterOn2, this));
              actions.Add(action);
            }

            float throttle = _rtdHelicopter->GetThrottle(1);
            if(throttle < 0.25f && throttle > 0.05f) //idle
            {
              UIAction action(new ActionBasic(ATThrottleOff2, this));
              actions.Add(action);
              UIAction action2(new ActionBasic(ATThrottleFull2, this));
              actions.Add(action2);
            }
            else if(throttle > 0.25f) //full
            {
              UIAction action(new ActionBasic(ATThrottleIdle2, this));
              actions.Add(action);
            }
            else if(throttle < 0.0001f) //off
            {
              UIAction action(new ActionBasic(ATThrottleIdle2, this));
              actions.Add(action);
            }

          }
          //----------------------------- engine 3 -----------------------------
          if(_rtdHelicopter->GetNumberOfEngines() > 2)
          {
            if(_starterWanted & 4)
            {
              UIAction action(new ActionBasic(ATStarterOff3, this));
              actions.Add(action);
            }
            else
            {
              UIAction action(new ActionBasic(ATStarterOn3, this));
              actions.Add(action);
            }

            float throttle = _rtdHelicopter->GetThrottle(2);
            if(throttle < 0.25f && throttle > 0.05f) //idle
            {
              UIAction action(new ActionBasic(ATThrottleOff3, this));
              actions.Add(action);
              UIAction action2(new ActionBasic(ATThrottleFull3, this));
              actions.Add(action2);
            }
            else if(throttle > 0.25f) //full
            {
              UIAction action(new ActionBasic(ATThrottleIdle3, this));
              actions.Add(action);
            }
            else if(throttle < 0.0001f) //off
            {
              UIAction action(new ActionBasic(ATThrottleIdle3, this));
              actions.Add(action);
            }
          }
        }
#endif
      }
    }
  }

  base::GetDrawActions3D(actions, unit);
}
#endif

bool HelicopterRTD::GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const
{
  switch (action->GetType())
  {
    case ATCollisionLightOn:
    case ATCollisionLightOff:
    case ATEnginesOnAuto:
    case ATEnginesOffAuto:
    case ATHelicopterAutoTrimOn:
    case ATHelicopterAutoTrimOff:
    case ATHelicopterTrimOn:
    case ATHelicopterTrimOff:
    case ATBatteriesOn:
    case ATBatteriesOff:
    case ATAPUOn:
    case ATAPUOff:
    case ATRotorBrakeOn:
    case ATRotorBrakeOff:
    case ATStarterOn1:
    case ATStarterOff1:
    case ATStarterOn2:
    case ATStarterOff2:
    case ATStarterOn3:
    case ATStarterOff3:
    case ATThrottleOff1:
    case ATThrottleIdle1:
    case ATThrottleFull1:
    case ATThrottleOff2:
    case ATThrottleIdle2:
    case ATThrottleFull2:
    case ATThrottleOff3:
    case ATThrottleIdle3:
    case ATThrottleFull3:
    case ATWheelsBrakeOn:
    case ATWheelsBrakeOff:
    case ATLandGear:
    case ATLandGearUp:
      // No parameters
      return true;
  }
  return base::GetActionParams(params, action, unit);
}

void HelicopterRTD::PerformAction(const Action *action, AIBrain *unit)
{
  switch (action->GetType())
  {
    case ATLandGear:
      if(!_landContact) 
      {
        SetPilotGear(true);
        PlayGearUpDownSound(true);
      }

      return;
    case ATLandGearUp:
      if(!_landContact) 
      {
        SetPilotGear(false);
        PlayGearUpDownSound(false);
      }
      return;
    case ATLightOn:
      SetPilotLight(true);
      return;
    case ATLightOff:
      SetPilotLight(false);
      return;
    case ATCollisionLightOn:
      SetCollisionLight(true);
      return;
    case ATCollisionLightOff:
      SetCollisionLight(false);
      return;
    case ATHelicopterTrimOn:
      {
#if !_DUMMY_ROTORLIB
        _cyclicSideRTDOffset = _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[0];
        _cyclicForwardRTDOffset = _rtdHelicopter->getPilotInputData()->m_rollPitchControl.getValue()[1];
        _backRotorRTDOffset =  _rtdHelicopter->getPilotInputData()->m_pedals;
#endif
        _delayAfterTrimming = Glob.time + 1;
        _controllsTrim = true;
        return;
      }
    case ATHelicopterTrimOff:
      {
        _cyclicSideRTDOffset = 0;
        _cyclicForwardRTDOffset = 0;
        _backRotorRTDOffset = 0;
        _controllsTrim = false;
        return;
      }
    case ATEnginesOnAuto:
      {
        //scripted event
        return;
      }
    case ATEnginesOffAuto:
      {
        //scripted event
        return;
      }
    case ATHelicopterAutoTrimOn:
      {
        SetAutoTrim(true);
        return;
      }
    case ATHelicopterAutoTrimOff:
      {
        SetAutoTrim(false);
        return;
      }
    case ATBatteriesOn:
      {
        SetBatteryOn(true);
        return;
      }
    case ATBatteriesOff:
      {
        SetBatteryOn(false);
        return;
      }
    case ATAPUOn:
      {
        if(IsElectricity() == false) return;
#if _HELISIM_SOUNDS
        PlayApuSound(true);
#endif
        return;
      }
    case ATAPUOff:
      {
#if _HELISIM_SOUNDS
        PlayApuSound(false);
#endif
        return;
      }
    case ATRotorBrakeOn:
      {
        SetRotorBrake(1.0f);
        return;
      }
    case ATRotorBrakeOff:
      {
        SetRotorBrake(0.0f);
        return;
      }
    case ATStarterOn1:
      {
        SetStarterWanted(true, 0);
        return;
      }
    case ATStarterOff1:
      {
        SetStarterWanted(false, 0);
        return;
      }
    case ATStarterOn2:
      {
        SetStarterWanted(true, 1);
        return;
      }
    case ATStarterOff2:
      {
        SetStarterWanted(false, 1);
        return;
      }
    case ATStarterOn3:
      {
        SetStarterWanted(true, 2);
        return;
      }
    case ATStarterOff3:
      {
        SetStarterWanted(false, 2);
        return;
      }
    case ATThrottleOff1:
      {
        //scripted event
        return;
      }
    case ATThrottleIdle1:
      {
#if _HELISIM_SOUNDS
        if(GetFuel() > 0.001) PlayIdleThrottleSound(0);
#endif
        return;
      }
    case ATThrottleFull1:
      {
        //scripted event
        return;
      }
    case ATThrottleOff2:
      {
        //scripted event
        return;
      }
    case ATThrottleIdle2:
      {
#if _HELISIM_SOUNDS
        if(GetFuel() > 0.001) PlayIdleThrottleSound(1);
#endif
        return;
      }
    case ATThrottleFull2:
      {
        //scripted event
        return;
      }
    case ATThrottleOff3:
      {
        //scripted event
        return;
      }
    case ATThrottleIdle3:
      {
#if _HELISIM_SOUNDS
        if(GetFuel() > 0.001) PlayIdleThrottleSound(2);
#endif
        return;
      }
    case ATThrottleFull3:
      {
        //scripted event
        return;
      }

    case ATWheelsBrakeOn:
      {
        SetWheelsBrakeRTD(1.0,3); 
        return;
      }
    case ATWheelsBrakeOff:
      {
        SetWheelsBrakeRTD(0.0,3); 
        return;
      }
  }
  base::PerformAction(action, unit);
}

//get keyboard or joystick collective (what was last used)
void HelicopterRTD::SetActualCollective(float value)
{
  //change only for keyboard
  if(_analogueThrottleTime <= _thrustHelperTime)
  {
    _thrustHelperTime = Glob.time;
    _actualCollectiveRTD = value;
  }
  _collectiveWanted = value;
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter) _rtdHelicopter->getPilotInputData()->m_collective = _actualCollectiveRTD;
#endif
};


//@{ rotorlib related animation controllers
float HelicopterRTD::GetAirspeedIndicatorRTD(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    RTD::InsGenAirspeedIndicator100* asi = _rtdHelicopter->getAirspeedIndicator();
    if(asi) 
    { 
      float speed = asi->getIndicatedAirspeed_mps() ;
      return _isnan(speed)? 0 : speed;
    }
  }
#endif
  return vs.cast<Entity>().ModelSpeed().Z() ;
}


float HelicopterRTD::GetAltBaroRTD(ObjectVisualState const& vs) const
{
  float coef = Type()->_hitStaticPort >= 0 ? 1 - _hit[Type()->_hitStaticPort] : 1;
  return coef * vs.Position().Y();
}

float HelicopterRTD::GetVertSpeedRTD(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    RTD::InsGenVerticalSpeedIndicator100* vsi = _rtdHelicopter->getVSI();
    if(vsi)
    {     
      float speed = vsi->getVerticalSpeed_mps() ;
      return _isnan(speed)? 0 : speed;
    }
  }
#endif
  return vs.cast<Entity>().ModelSpeed().Y();
}


float HelicopterRTD::GetAttitudeBankRTD(ObjectVisualState const& unused) const 
{
  return -RenderVisualState().GetHorizonBank();
}

float HelicopterRTD::GetAttitudeDiveRTD(ObjectVisualState const& unused) const 
{
  return RenderVisualState().GetHorizonDive();
}

void HelicopterRTD::GetAttitude(ObjectVisualState const& unused, float &pitch, float &bank) 
{
  pitch = RenderVisualState().GetHorizonDive();
  bank = -RenderVisualState().GetHorizonBank();
}

float HelicopterRTD::GetSkidRTD(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    RTD::InsGenTurnIndicator100* skd = _rtdHelicopter->getTurnIndicator();
    if(skd) return skd->getTurnRate();
  }
#endif
  return vs.cast<Entity>().ModelSpeed().X();
}

float HelicopterRTD::GetBatteryChargeScaled(ObjectVisualState const& vs) const 
{
  return _batteryCharge / Type()->_maxBatteryCharge;
}

float HelicopterRTD::GetAltRadarRTD(ObjectVisualState const& vs) const 
{
  return vs.Position().Y() - GLOB_LAND->RoadSurfaceYAboveWater(vs.Position());
}

float HelicopterRTD::GetAccelerometer(ObjectVisualState const& vs) const 
{
  Vector3 gravity = Vector3(0, -G_CONST, 0 );
  Vector3 gmeter = -vs.cast<Entity>().GetAcceleration() + gravity;

  return (gmeter.Size() / G_CONST) * sign( gmeter * gravity);
}


float HelicopterRTD::GetRpm1RTDScaled(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
      return _rtdHelicopter->GetEnginesRpm(0) / _rtdHelicopter->GetEngineTargetRPM(0);
  }
#endif
  return vs.cast<HelicopterRTD>()._rotorSpeed;
}

float HelicopterRTD::GetRpm2RTDScaled(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    return _rtdHelicopter->GetEnginesRpm(1) / _rtdHelicopter->GetEngineTargetRPM(1);
  }
#endif
  return vs.cast<HelicopterRTD>()._rotorSpeed;
}

float HelicopterRTD::GetRpm3RTDScaled(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    return _rtdHelicopter->GetEnginesRpm(2) / _rtdHelicopter->GetEngineTargetRPM(2);
  }
#endif
  return vs.cast<HelicopterRTD>()._rotorSpeed;
}

float HelicopterRTD::GetTemp1RTDScaled(ObjectVisualState const& unused) const 
{
  //temperature 0-1 is green, 1 - 1.3 is yellow/red
  return (_enginesTemperature.Size() > 0) ? floatMin(_enginesTemperature[0].GetTemperature() / Type()->_maxEngineTemperature,1.3f) : 0;
}

float HelicopterRTD::GetTemp2RTDScaled(ObjectVisualState const& unused) const 
{
  //temperature 0-1 is green, 1 - 1.3 is yellow/red
  return (_enginesTemperature.Size() > 1) ? floatMin(_enginesTemperature[1].GetTemperature() / Type()->_maxEngineTemperature,1.3f) : 0;
}

float HelicopterRTD::GetTemp3RTDScaled(ObjectVisualState const& unused) const 
{
  //temperature 0-1 is green, 1 - 1.3 is yellow/red
  return (_enginesTemperature.Size() > 2) ? floatMin(_enginesTemperature[2].GetTemperature() / Type()->_maxEngineTemperature,1.3f) : 0;
}

float HelicopterRTD::GetTorqueRTD(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated && Glob.time > _timeToForcedChange + HIDE_SIMULATION_PEEKS ) //_timeToForcedChange to hide peek after sim. chage
  {
    if(_rtdHelicopter->getMainRotor()) 
      return _rtdHelicopter->GetDeliveredTorque();
  } // not simulated - app. value
#endif
  return floatMin(1,vs.cast<HelicopterRTD>()._rotorSpeed) * 0.7 * Type()->_maxTorque;
}

float HelicopterRTD::GetTorqueRTDScaled(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    if(_rtdHelicopter->getMainRotor()) 
      return _rtdHelicopter->GetDeliveredTorque() / (Type()->_maxTorque * _rtdHelicopter->getNumEngines());
  } // not simulated - app. value
#endif
  return vs.cast<HelicopterRTD>()._rotorSpeed * 0.7;
}

float HelicopterRTD::GetTorqueEng1RTDScaled(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    if(_rtdHelicopter->getNumEngines()>0 && _rtdHelicopter->getEngine(0)) 
      return _rtdHelicopter->getEngine(0)->getTorque() / Type()->_maxTorque;
  } // not simulated - app. value
#endif
  return vs.cast<HelicopterRTD>()._rotorSpeed * 0.7;
}

float HelicopterRTD::GetTorqueEng2RTDScaled(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    if(_rtdHelicopter->getNumEngines()>1 && _rtdHelicopter->getEngine(1)) 
      return _rtdHelicopter->getEngine(1)->getTorque() / Type()->_maxTorque;
  } // not simulated - app. value
#endif
  return vs.cast<HelicopterRTD>()._rotorSpeed * 0.7;
}

float HelicopterRTD::GetTorqueEng3RTDScaled(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    if(_rtdHelicopter->getNumEngines()>2 && _rtdHelicopter->getEngine(2)) 
    return _rtdHelicopter->getEngine(2)->getTorque() / Type()->_maxTorque ;
  } // not simulated - app. value
#endif
  return vs.cast<HelicopterRTD>()._rotorSpeed * 0.7;
}

float HelicopterRTD::GetMainRotorRPMRTDScaled(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    if(_rtdHelicopter->getMainRotor() && _rtdHelicopter->getMainRotor()->getConfigParams()) 
      return  _rtdHelicopter->getMainRotor()->getRPM() / _rtdHelicopter->getMainRotor()->getDesignRPM();
  }
#endif
  return vs.cast<HelicopterRTD>()._rotorSpeed;
}

float HelicopterRTD::GetTailRotorRPMRTDScaled(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {
    if(_rtdHelicopter->getTailRotor() && _rtdHelicopter->getTailRotor()->getConfigParams()) 
      return _rtdHelicopter->getTailRotor()->getRPM() / _rtdHelicopter->getTailRotor()->getDesignRPM();
  }
#endif
  return vs.cast<HelicopterRTD>()._rotorSpeed;
}

float HelicopterRTD::GetThrottle1(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated && _rtdHelicopter->getNumEngines() > 0)
  {
    return _rtdHelicopter->GetThrottle(0);
  }
#endif
  return (vs.cast<HelicopterRTD>()._rotorSpeed > 0) ? 1:0;
}

float HelicopterRTD::GetThrottle2(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated && _rtdHelicopter->getNumEngines() > 1)
  {
    return _rtdHelicopter->GetThrottle(1);
  }
#endif
  return (vs.cast<HelicopterRTD>()._rotorSpeed > 0) ? 1:0;
}

float HelicopterRTD::GetThrottle3(ObjectVisualState const& vs) const 
{
#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated && _rtdHelicopter->getNumEngines() > 2)
  {
    return _rtdHelicopter->GetThrottle(2);
  }
#endif
  return (vs.cast<HelicopterRTD>()._rotorSpeed > 0) ? 1:0;
}
//@}

float HelicopterRTD::GetGMeterZ() const
{
#if !_DUMMY_ROTORLIB
  if (_rtdHelicopter)
  {
    return GetAccelerometer(RenderVisualState());
  }
#endif
  return base::GetGMeterZ();
}

float HelicopterRTD::Thrust() const
{
#if !_DUMMY_ROTORLIB
  if (_rtdHelicopter) 
  {
    return GetTorqueRTDScaled(FutureVisualState());
  }
#endif
  return base::Thrust();
}

float HelicopterRTD::GetRPM(int idx) const
{ 
#if !_DUMMY_ROTORLIB
  if (!_rtdHelicopter) 
    return 0;
  
  if(_isRTDSimulated)
    return _rtdHelicopter->GetEnginesRpm(idx);
  else
  {
    return RenderVisualState()._rotorSpeed * _rtdHelicopter->GetTargetRPM(idx);
  }
#else
  return 0;
#endif
}

float HelicopterRTD::GetMotorDamage() const
{
#if !_DUMMY_ROTORLIB
  if (!_rtdHelicopter)
    return 0.0f;

  float hit = _hit[Type()->_engineHit];
  hit += _hit[Type()->_hitEngine2];
  hit += _hit[Type()->_hitEngine3];

  hit /= (float) _rtdHelicopter->GetNumberOfEngines();
  saturate(hit, 0.0f, 1.0f);
  return hit;
#else
  return 0;
#endif
}

float HelicopterRTD::GetTransmissionDamage() const
{
#if !_DUMMY_ROTORLIB
  if (!_rtdHelicopter)
    return 0.0f;

  return (Type()->_hitTransmission >= 0) ? _hit[Type()->_hitTransmission] : 0.0f;
#else
  return 0;
#endif
}

float HelicopterRTD::GetDamageShakePower() 
{
  float damage = 0;
  float speedScale = RenderVisualState().ModelSpeed().Z() / 27.7f; //100km/h
  saturate(speedScale, 0, 1);
  damage +=  RenderVisualState()._rotorSpeed * Type()->_hitTransmission>=0 ?  2 * _hit[Type()->_hitTransmission]  : 0;
  damage +=  speedScale * Type()->_hitHStabilizerL1>=0 ?  2 * _hit[Type()->_hitHStabilizerL1] : 0;
  damage +=  speedScale * Type()->_hitHStabilizerR1>=0 ?  2 * _hit[Type()->_hitHStabilizerR1] : 0;
  damage +=  speedScale * Type()->_hitVStabilizer1>=0  ?  2 * _hit[Type()->_hitVStabilizer1]  : 0;

  saturate(damage, 0 , 2);
  return damage;
};

float HelicopterRTD::GetRPMStarter(int idx) const
{
#if !_DUMMY_ROTORLIB
  if (!_rtdHelicopter) 
    return 0.0f;

  if (GetStarter(idx))
    return floatMin(GetRPM(idx), toInt(_rtdHelicopter->GetEngineTargetRPM(idx) * 0.250f));
  else
    return InterpolativC(_rpmStarterOffTime[idx], 0.0f, 10.0f, 0.0f, _rpmStarter[idx]);
#else
  return 0;
#endif
}

float HelicopterRTD::GetEngineMaxDamage() const
{
  return floatMax( floatMax(
    (Type()->_engineHit >=0) ? _hit[Type()->_engineHit] : 0, 
    (Type()->_hitEngine2 >=0) ? _hit[Type()->_hitEngine2] : 0), 
   (Type()->_hitEngine3 >=0) ? _hit[Type()->_hitEngine3] : 0);
};

float HelicopterRTD::GetEngineRPMState() const 
{

#if !_DUMMY_ROTORLIB
  if(_rtdHelicopter && _isRTDSimulated)
  {  
    float maxRPM = 0;
    for(int i = 0; i< _rtdHelicopter->GetNumberOfEngines(); i++)
    {
      maxRPM = floatMax(maxRPM, _rtdHelicopter->GetEnginesRpm(i) / _rtdHelicopter->GetTargetRPM(i));
    }
    return maxRPM;
  }   
#endif
  return RenderVisualState()._rotorSpeed;
}

float HelicopterRTD::GetMaxEngineTemperatureScaled() const
{
  float maxTemp = 0;
  if(_isRTDSimulated)
  {  
    for(int i = 0; i< _enginesTemperature.Size(); i++)
    {
      maxTemp = floatMax(maxTemp, _enginesTemperature[i].GetTemperature() / Type()->_maxEngineTemperature);
    }
  }   
  return maxTemp;
}

bool HelicopterRTD::IsHotStart() const
{
#if !_DUMMY_ROTORLIB
  for(int i = 0; i< _enginesTemperature.Size(); i++)
  {
    if(
      GetStarter(i) && 
      _rtdHelicopter->GetThrottle(0) < 0.01f && 
      _enginesTemperature[i].GetTemperature() > Type()->_hotStartTemperature) return true;
  }
#endif
  
  return false;
}

float HelicopterRTD::GetCtrlCyclicFwd(ObjectVisualState const& vs) const 
{ 
  return _cyclicForwardAnimation; 
}

float HelicopterRTD::GetCtrlCyclicAside(ObjectVisualState const& vs) const 
{ 
  return _cyclicAsideAnimation; 
}

float HelicopterRTD::GetCtrlWheelRot(ObjectVisualState const& vs) const 
{
  return static_cast_checked<VisualState const&>(vs).GetWheelRot(); 
}

#endif
