#include "wpch.hpp"
#include "camShake.hpp"
#include "paramFileExt.hpp"
#include "El/Common/randomGen.hpp"
#include "global.hpp"
#include "world.hpp"
#include "AI/ai.hpp"

// =================================================
// CameraShakeSource implementation
// =================================================
CameraShakeSource::CameraShakeSource()
: _type(CameraShakeSource::CSNone), _power(0), _duration(0), _frequency(0), _curPower(0), _curTime(0)  {}
CameraShakeSource::CameraShakeSource(CameraShakeSource::ShakeType type, float power, float duration, float frequency, 
                                     CameraShakeSource::TVehicleLink vehicle)
:_type(type), _power(power), _duration(duration), _frequency(frequency), _vehicle(vehicle), _curPower(0), _curTime(0)  {}

void CameraShakeSource::Simulate(float deltaT)
{
  _curTime += deltaT;
  _curPower = ComputeCurPower();
}

float CameraShakeSource::ComputeCurPower() const
{
  if (_duration <= 0 || IsExpired())
    return 0;

  if (CSVehicle == _type || CSDamage == _type)
  {
    // do not diminish power caused by vehicle shake
    return _power;
  }

  float timeCoef = _curTime/_duration;
  return (1.0f - timeCoef*timeCoef) * _power;
}

/// resets parameters for vehicle and restarts the shake
void CameraShakeSource::ResetVehicleParams(float power, float duration, float frequency)
{
  _power = power;
  _duration = duration;
  _frequency = frequency;
  _curTime = 0;
  _curPower = ComputeCurPower();
}


// =================================================
// CameraShakeManager implementation
// =================================================
/// coef used to multiply power to get positional change (in m)
float CameraShakeManager::PosChangeCoef = 0.005f;
/// coef used to multiply power to get rotation change in X (in degrees)
float CameraShakeManager::RotXChangeCoef = 0.5f;
/// coef used to multiply power to get rotation change in Y (in degrees)
float CameraShakeManager::RotYChangeCoef = 0.5f;
/// coef used to multiply power to get rotation change in Z (in degrees)
float CameraShakeManager::RotZChangeCoef = 0.5f;
/// if linear interpolation should be used between 2 camera positions
bool CameraShakeManager::PerformLERP = true;

/// if default values should be used when no values are present in config
bool CameraShakeManager::UseDefaultValues = true;

/// default power
float CameraShakeManager::DefaultPower = 12;
/// default duration (in s)
float CameraShakeManager::DefaultDuration = 1.0f;
/// default max distance (in m)
float CameraShakeManager::DefaultMaxDistance = 120;
/// default frequency of matrix update (number of updates per second)
float CameraShakeManager::DefaultFrequency = 20;
/// default min speed of vehicle to cause shake
float CameraShakeManager::DefaultMinSpeed = 0;
/// default min mass of vehicle to cause shake 
float CameraShakeManager::DefaultMinMass = 10000;
/// coef used to multiply caliber of ammo to get the coef for power and duration of shake used when the player is hit by ammo
float CameraShakeManager::DefaultCaliberCoefPlayerHit = 0.5f;
/// coef used to multiply caliber of ammo to get the coef for power and duration of shake used when the weapon is fired
float CameraShakeManager::DefaultCaliberCoefWeaponFire = 0.2f;
/// default coef used to multiply default power of shake of vehicles passing near player
float CameraShakeManager::DefaultPassingVehicleCoef = 0.05f;
/// time between two updates of shake of passing vehicles
float CameraShakeManager::DefaultPassingVehicleUpdateTime = 1.0f;
/// default vehicle shake attenuation coef
float CameraShakeManager::DefaultVehicleAttenuationCoef = 0.2f;


/// global enable/disable of camera shakes
bool CameraShakeManager::ShakeEnabled = true;
/// is the params were read from config
bool CameraShakeManager::ParamsRead = false;


CameraShakeManager::CameraShakeManager()
:_bestSourceIdx(InvalidSourceIdx), _modMatrix(M4Identity), _startPos(0,0,0), _startRot(0,0,0,1),
_endPos(0,0,0), _endRot(0,0,0,1), _nextCompMatrixTime(0)
{
}


void CameraShakeManager::Simulate(float deltaT)
{
  if (0.0f == deltaT)
    return;

  _bestSourceIdx = InvalidSourceIdx;
  float bestPower = 0;

  // advance all sources in time, remove expired sources and find best current source
  int checkCount = 100000; //just for detecting potentional infinite loop
  for (int i=0; i< _sources.Size() && checkCount > 0; --checkCount)
  {
    CameraShakeSource& source = _sources[i];
    source.Simulate(deltaT);
    if (source.IsExpired())
    {
      // remove expired source
      _sources.Delete(i);
    }
    else
    {
      if (source.GetCurPower() > bestPower)
      {
        bestPower = source.GetCurPower();
        _bestSourceIdx = i;
      }
      ++i;
    }
  }

  Assert(checkCount > 0 && "Possible infinite loop in CameraShakeManager::Simulate");

  ComputeModMatrix(deltaT);
}

void CameraShakeManager::ComputeModMatrix(float deltaT)
{
  if (InvalidSourceIdx == _bestSourceIdx)
  {
    //no source for shake
    ResetMatrix();
    return; 
  }

  Assert(_bestSourceIdx >= 0 && _bestSourceIdx < _sources.Size());
  if (_bestSourceIdx < 0 || _bestSourceIdx >= _sources.Size())
  {
    ResetMatrix();
    return;
  }

  const CameraShakeSource &source = _sources[_bestSourceIdx];

  // compute time between two updates
  float maxUpdateTime = 1.0f; //to prevent division by zero and other problems we set this to 1 by default
  float frequency = source.GetFrequency();
  Assert(frequency > 0);
  if (frequency > 0)
    maxUpdateTime = 1.0f/frequency;

  // advance time
  _nextCompMatrixTime -= deltaT;
  if (_nextCompMatrixTime < 0)
  {
    _nextCompMatrixTime += maxUpdateTime;

    // it is time to compute new end values
    _startPos = _endPos;
    _startRot = _endRot;

    // angle changes (in radians)
    float angleDeltaX = source.GetCurPower() * RotXChangeCoef * H_PI/180.0f * (GRandGen.RandomValue() - 0.5f);
    float angleDeltaY = source.GetCurPower() * RotYChangeCoef * H_PI/180.0f * (GRandGen.RandomValue() - 0.5f);
    float angleDeltaZ = source.GetCurPower() * RotZChangeCoef * H_PI/180.0f * (GRandGen.RandomValue() - 0.5f);

    // compute rotations
    _endRot.FromEuler(angleDeltaX, angleDeltaY, angleDeltaZ);
    //Quaternion<float>(VAside, angleDeltaX) * 
    //Quaternion<float>(VUp, angleDeltaY) * 
    //Quaternion<float>(VForward, angleDeltaZ);

    // position change
    float posDelta = source.GetCurPower() * PosChangeCoef;
    // set position change
    _endPos = Vector3(
      posDelta * (GRandGen.RandomValue() - 0.5f), 
      posDelta * (GRandGen.RandomValue() - 0.5f), 
      posDelta * (GRandGen.RandomValue() - 0.5f)
      );
  }

  // compute current mod matrix
  if (PerformLERP)
  {
    // current time coef
    float timeCoef = (maxUpdateTime-_nextCompMatrixTime)/maxUpdateTime;
    saturate(timeCoef, 0, 1);
    timeCoef = 1.0f - 0.5f*(cosf(timeCoef * H_PI) + 1.0f); //cos to smooth the curve
    saturate(timeCoef, 0, 1);

    // perform LERP to compute current matrix
    _modMatrix.SetOrientation(_startRot.Slerp(_endRot, timeCoef));
    _modMatrix.SetPosition(_startPos * (1.0f-timeCoef) + _endPos * timeCoef);
  }
  else
  {
    _modMatrix.SetOrientation(_endRot);
    _modMatrix.SetPosition(_endPos);
  }
}

void CameraShakeManager::UpdateCameraTransform(Matrix4& transform)
{
  if (!IsEnabled())
    return;

  if (InvalidSourceIdx == _bestSourceIdx)
    return; //no source for shake

  transform = transform * _modMatrix;
}

void CameraShakeManager::AddSource(CameraShakeSource::ShakeType type, float power, float duration, 
                                   float frequency, CameraShakeSource::TVehicleLink vehicle)
{
  if (!IsEnabled())
    return;

  if (CameraShakeSource::CSVehicle == type || CameraShakeSource::CSDamage == type)
  {
    // vehicle type
    // just check if vehicle is already there
    for (int i=0; i<_sources.Size(); ++i)
    {
      CameraShakeSource& source = _sources[i];
      if (source.GetVehicle().GetLink() == vehicle.GetLink() && source.GetType() == type)
      {
        // do not add new source, just update old on
        source.ResetVehicleParams(power, duration, frequency);
        return;
      }
    }
  }
  _sources.Add(CameraShakeSource(type, power, duration, frequency, vehicle));
}

void CameraShakeManager::AddSource(const CameraShakeParams& params)
{
  if (!IsEnabled())
    return;

  Assert(GWorld);
  if (!GWorld)
    return;

  // get focused unit
  const AIBrain* player = GWorld->FocusOn();
  if (!player)
    return;

  // get ammo
  const AmmoType* ammo = params._ammoType;
  if (params._shakeType != CameraShakeSource::CSVehicle && params._shakeType != CameraShakeSource::CSDamage)
  {
    if (!ammo && !params._ammoTypeName.IsEmpty())
    {
      // get ammo type by name
      Ref<EntityType> ammoType = VehicleTypes.New(params._ammoTypeName);
      if (!ammoType.IsNull())
      {
        ammo = dynamic_cast<AmmoType *>(ammoType.GetRef());
        Assert(ammo);
      }
    }

    if (!ammo)
      return; //no shake without ammo
  }

  // maximum distance of shake
  float maxDistance = DefaultMaxDistance;
  // max power of shake
  float maxPower = DefaultPower;
  // max duration of shake
  float maxDuration = DefaultDuration;
  // frequency of camera changes shake
  float frequency = DefaultFrequency;
  // minimum speed (only for vehicles causing shake)
  float minSpeed = DefaultMinSpeed;

  // attenuation coefficient
  float attentuationCoef = 1.0f;

  if (player->GetVehicleIn())
  {
    // player is inside vehicle
    const EntityAIType* type = player->GetVehicleIn()->GetType();
    Assert(type);
    if (type)
    {
      const TransportType* ttype = dynamic_cast<const TransportType*>(type);
      Assert(ttype);
      if (ttype)
        attentuationCoef = ttype->GetCamShakeCoef();
    }
  }

  switch (params._shakeType)
  {
  case CameraShakeSource::CSExplosion:
    {
      // explosion of ammo
      if (ammo->_camShakeExplode.IsValid())
      {
        // values were read from config
        if (!ammo->_camShakeExplode.HasPower() || !ammo->_camShakeExplode.HasDistance())
        {
          return; //no shake
        }
        maxDistance = ammo->_camShakeExplode.GetDistance();
        maxPower = ammo->_camShakeExplode.GetPower();
        maxDuration = ammo->_camShakeExplode.GetDuration();
        frequency = ammo->_camShakeExplode.GetFrequency();
      }
      else if (UseDefaultValues)
      {
        // values were not read from config and default values should be used
        // we can use values for ear whistling
        if (ammo->_whistleDist <= 0 || 1 == ammo->_whistleOnFire)
          return; // no shake

        // whistleDist is often too low, so take DefaultMaxDistance if it is higher
        maxDistance = floatMax(ammo->_whistleDist, DefaultMaxDistance);
      }
      else
        return;
    }
    break;
  case CameraShakeSource::CSWeaponFire:
    {
      // general fire from weapon
      if (ammo->_camShakeFire.IsValid())
      {
        // values were read from config
        if (!ammo->_camShakeFire.HasPower() || !ammo->_camShakeFire.HasDistance())
        {
          return; //no shake
        }
        maxDistance = ammo->_camShakeFire.GetDistance();
        maxPower = ammo->_camShakeFire.GetPower();
        maxDuration = ammo->_camShakeFire.GetDuration();
        frequency = ammo->_camShakeFire.GetFrequency();
      }
      else if (UseDefaultValues)
      {
        // values were not read from config and default values should be used
        // we can use values for ear whistling
        if (ammo->_whistleDist <= 0 || 0 == ammo->_whistleOnFire)
          return; // no shake
        maxDistance = std::max(ammo->_whistleDist, DefaultMaxDistance);
      }
      else
        return;
    }
    break;
  case CameraShakeSource::CSPlayerWeaponFire:
    // player fires from weapon

    if (ammo->_camShakePlayerFire.IsValid())
    {
      // values were read from config
      if (!ammo->_camShakePlayerFire.HasPower())
      {
        return; //no shake
      }
      maxDistance = ammo->_camShakePlayerFire.GetDistance();
      maxPower = ammo->_camShakePlayerFire.GetPower();
      maxDuration = ammo->_camShakePlayerFire.GetDuration();
      frequency = ammo->_camShakePlayerFire.GetFrequency();
    }
    else if (UseDefaultValues)
    {
      // values were not read from config and default values should be used
      // use values based on ammo simulation and caliber
      switch (ammo->_simulation)
      {
      case AmmoShotBullet:
      case AmmoShotSpread:
        {
          /// compute coef based on ammo parameters
          float coef = ammo->caliber * DefaultCaliberCoefWeaponFire;
          saturate(coef, 0.1, 1);
          maxPower = DefaultPower * coef;
          maxDuration = DefaultDuration * coef;
        }
        break;
      default:
        return; //no shake
      }
    }
    else
      return;

    if (player->GetPerson())
    {
      // attenuate by cam shake coef for movement
      float moveShakeCoef = player->GetPerson()->GetCamShakeFireCoef();
      maxPower *= moveShakeCoef;
      maxDuration *= moveShakeCoef;
    }
    break;
  case CameraShakeSource::CSPlayerHit:
    // player hit by ammo
    if (ammo->_camShakeHit.IsValid())
    {
      // values were read from config
      if (!ammo->_camShakeHit.HasPower())
      {
        return; //no shake
      }
      maxDistance = ammo->_camShakeHit.GetDistance();
      maxPower = ammo->_camShakeHit.GetPower();
      maxDuration = ammo->_camShakeHit.GetDuration();
      frequency = ammo->_camShakeHit.GetFrequency();
    }
    else if (UseDefaultValues)
    {
      // values were not read from config and default values should be used
      // use values based on ammo simulation and caliber
      switch (ammo->_simulation)
      {
      case AmmoShotBullet:
      case AmmoShotSpread:
        {
          /// compute coef based on ammo parameters
          float coef = ammo->caliber * DefaultCaliberCoefPlayerHit;
          saturate(coef, 0.1, 1);
          maxPower = DefaultPower * coef;
          maxDuration = DefaultDuration * coef;
        }
        break;
      default:
        return; //no shake
      }
    }
    else
      return;
    break;
  case CameraShakeSource::CSVehicle:
    {
      // vehicle causing shake
      if (!params._vehicle.GetLink())
        return;// no vehicle

      Transport* vehicle = dynamic_cast<Transport*>(params._vehicle.GetLink());
      Assert(vehicle);
      if (!vehicle)
        return; //no vehicle

      Assert(vehicle->GetType());
      if (!vehicle->GetType())
        return;
           
      const TransportType* transportType = dynamic_cast<const TransportType*>(vehicle->GetType());
      Assert(transportType);
      if (!transportType)
        return;

      float curSpeed = vehicle->FutureVisualState().Speed().Size();

      if (transportType->GetCamShake().IsValid())
      {
        // values were read from config
        maxDistance = transportType->GetCamShake().GetDistance();
        maxPower = transportType->GetCamShake().GetPower();
        frequency = transportType->GetCamShake().GetFrequency();
        minSpeed = transportType->GetCamShake().GetMinSpeed();
        maxDuration = DefaultPassingVehicleUpdateTime;
      }
      else if (UseDefaultValues)
      {
        // values were not read from config and default values should be used
        if (vehicle->GetMass() < DefaultMinMass)    
          return; //too light
        maxPower = DefaultPower * DefaultPassingVehicleCoef;
        maxDuration = DefaultPassingVehicleUpdateTime;
      }
      else
        return;

      if (curSpeed < minSpeed)
        return; //not fast enough
      
      // compute speed coef
      float maxSpeed = transportType->GetMaxSpeedMs();
      float speedCoef = 1.0f;
      if (maxSpeed > 0.0f)
      {
        speedCoef = 1.0f - curSpeed/maxSpeed;
        saturate(speedCoef, 0.0f, 1.0f);
        speedCoef = 1.0f - speedCoef*speedCoef;
        saturate(speedCoef, 0.0f, 1.0f);
      }

      // modify values based on speed
      maxPower *= speedCoef;
    }
    break;
  case CameraShakeSource::CSDamage:
    {
      Transport* vehicle = dynamic_cast<Transport*>(params._vehicle.GetLink());
      if(!vehicle) return;
      if(player->GetVehicleIn() != vehicle) return;
      maxPower = vehicle->GetDamageShakePower();
      maxDuration = CameraShakeManager::DefaultPassingVehicleUpdateTime * 2;
      frequency = DefaultFrequency;
      maxDistance = -1;
      attentuationCoef = 1;
      if(maxPower < 0.01) return;
    }
    break;
  default:
    Assert(!"Invalid camera shake type.");
    return;
  }

  /// small number for testing
  static const float epsilon = 0.0001f;

  /// check is power is forced
  if (params._forcePower > epsilon)
    maxPower = params._forcePower;
  /// check is duration is forced
  if (params._forceDuration > epsilon)
    maxDuration = params._forceDuration;

  if (maxPower < epsilon || maxDuration < epsilon)
    return;  // no power or duration

  // distance coef
  float distCoef = 1.0f;
  if (maxDistance > 0 && params._shakeType != CameraShakeSource::CSPlayerWeaponFire && params._shakeType != CameraShakeSource::CSPlayerHit)
  {
    // distance from player
    float playerDistance = player->Position(player->GetFutureVisualState()).Distance(params._pos);

    if (playerDistance > maxDistance)
      return; //out of range

    // compute distance coef
    distCoef = 1.0f - playerDistance / maxDistance;
    distCoef *= distCoef; //quadratic falloff
    saturate(distCoef, 0.05f, 1.0f); //just limit minimal shake so that it is not 0
  }


  float power = maxPower * distCoef * attentuationCoef;
  float duration = maxDuration * distCoef * attentuationCoef;
  if (CameraShakeSource::CSVehicle == params._shakeType || CameraShakeSource::CSDamage == params._shakeType)
  {
    duration = maxDuration; //do not diminish duration with distance for vehicles
  }

  AddSource(params._shakeType, power, duration, frequency, params._vehicle);
}


void CameraShakeManager::Clear()
{
  _sources.Clear();
  _bestSourceIdx = InvalidSourceIdx;
  ResetMatrix();
}

void CameraShakeManager::DebugTestVehicles()
{
  RptF("CameraShakeManager - testing vehicles started.");
  for (int i=0; i<_sources.Size(); ++i)
  {
    CameraShakeSource& source = _sources[i];
    if (source.GetType() == CameraShakeSource::CSVehicle)
    {
      if (source.GetVehicle())
        RptF("CameraShakeManager - vehicle is OK");
      else
        RptF("CameraShakeManager - vehicle is NULL");
    }
  }
  RptF("CameraShakeManager - testing vehicles finished.");
}

/// resets mod matrix and all needed values for its computation
void CameraShakeManager::ResetMatrix()
{
  _modMatrix.SetIdentity();
  _startPos = Vector3(0,0,0);
  _startRot = Quaternion<float>(0,0,0,1);
  _endPos = Vector3(0,0,0);
  _endRot = Quaternion<float>(0,0,0,1);
  _nextCompMatrixTime = 0;
}

void CameraShakeManager::ReadParams(ParamEntryPar cfg)
{
  if (ParamsRead)
    return;

  ParamsRead = true;

  ConstParamEntryPtr parVal = cfg.FindEntry("CfgCameraShake");
  if (!parVal)
    return;

  if (!parVal->IsClass())
    return;

  ConstParamEntryPtr par = parVal->FindEntry("posChangeCoef");    
  if (par)
  {
    PosChangeCoef = *par;    
  }
  par = parVal->FindEntry("rotXChangeCoef");    
  if (par)
  {
    RotXChangeCoef = *par;    
  }
  par = parVal->FindEntry("rotYChangeCoef");    
  if (par)
  {
    RotYChangeCoef = *par;    
  }
  par = parVal->FindEntry("rotZChangeCoef");    
  if (par)
  {
    RotZChangeCoef = *par;    
  }
  par = parVal->FindEntry("performLERP");    
  if (par)
  {
    PerformLERP = *par;    
  }

  par = parVal->FindEntry("useDefaultValues");    
  if (par)
  {
    UseDefaultValues = *par;    
  }

  par = parVal->FindEntry("defaultPower");    
  if (par)
  {
    DefaultPower = *par;    
  }
  par = parVal->FindEntry("defaultDuration");    
  if (par)
  {
    DefaultDuration = *par;    
  }
  par = parVal->FindEntry("defaultMaxDistance");    
  if (par)
  {
    DefaultMaxDistance = *par;    
  }    
  par = parVal->FindEntry("defaultFrequency");    
  if (par)
  {
    DefaultFrequency = *par;    
  }    
  par = parVal->FindEntry("defaultMinSpeed");    
  if (par)
  {
    DefaultMinSpeed = *par;    
  }    
  par = parVal->FindEntry("defaultMinMass");    
  if (par)
  {
    DefaultMinMass = *par;    
  }    
  par = parVal->FindEntry("defaultCaliberCoefPlayerHit");    
  if (par)
  {
    DefaultCaliberCoefPlayerHit = *par;    
  }    
  par = parVal->FindEntry("defaultCaliberCoefWeaponFire");    
  if (par)
  {
    DefaultCaliberCoefWeaponFire = *par;    
  }    
  par = parVal->FindEntry("defaultPassingVehicleCoef");    
  if (par)
  {
    DefaultPassingVehicleCoef = *par;    
  }    
  par = parVal->FindEntry("defaultPassingVehicleUpdateTime");    
  if (par)
  {
    DefaultPassingVehicleUpdateTime = *par;    
  }    
  par = parVal->FindEntry("defaultVehicleAttenuationCoef");    
  if (par)
  {
    DefaultVehicleAttenuationCoef = *par;    
  }    
}

bool CameraShakeManager::IsEnabled()
{
  return ShakeEnabled && Glob.config.IsEnabled(DTCameraShake);
}

