// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"

#if _PROFILE // _ENABLE_CHEATS
#pragma optimize("",off)
#endif

#include "airplane.hpp"
#include "shots.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "soldierOld.hpp"
#include "keyInput.hpp"
#include "global.hpp"
#include "lights.hpp"
#include "AI/ai.hpp"
#include "dikCodes.h"
#include <El/Common/randomGen.hpp>
#include <El/Common/perfProf.hpp>
#include "camera.hpp"
#include "diagModes.hpp"

#include "Network/network.hpp"
#include "UI/uiActions.hpp"
#include "stringtableExt.hpp"
#include "paramArchiveExt.hpp"

#if _DEBUG
  #define ARROWS 1
#elif _ENABLE_CHEATS
  #define ARROWS 1
#else
  #define ARROWS 0
#endif

#if _ENABLE_CHEATS
  #define LOG_SWEEP 1
#endif

DEFINE_CASTING(Airplane)
Airplane::Airplane( const EntityAIType *name, Person *pilot, bool fullCreate)
:base(name,pilot,true),
_rotorSpeed(0),

_lastAngVelocity(VZero),

_thrust(0),_thrustWanted(0), // turning motor on/off
_elevatorWanted(0),
_elevatorTrim(0),
_rudderWanted(0),
_aileronWanted(0),
_openCabin(false),_thrustVectorWanted(0),
_rndFrequency(1-GRandGen.RandomValue()*0.05), // do not use same sound frequency
_wheelSpeed(0),

_pilotGear(true),
_rocketLRToggle(false),_vtolMode(false),
_pilotFlaps(0),
_servoVol(0),

_gunYRot(0),_gunYRotWanted(0),
_gunXRot(0),_gunXRotWanted(0),
_gunXSpeed(0),_gunYSpeed(0),

_gearDamage(false),

_pilotBrake(1)
{
  SetSimulationPrecision(1.0/15,1.0/15);
  RandomizeSimulationTime();
  SetTimeOffset(1.0/15);

  _leftDust.Init(Pars >> Type()->_leftDust, this, EVarSet_Dust);
  _rightDust.Init(Pars >> Type()->_rightDust, this, EVarSet_Dust);
  _dustEffect.Init(Pars >> Type()->_dustEffect, this, EVarSet_Effect);
  _waterEffect.Init(Pars >> Type()->_waterEffect, this, EVarSet_Effect);
  _damageEffect.Init(Pars >> Type()->_damageEffect, this, EVarSet_Damage);

  // initialize shared land-contact info in the type
  if (!Type()->_landContactPlaneValid)
  {
    // this cannot be done directly in the type, as some functions of Airplane are needed
    // LandContactMinY is a function of Entity, and it relies heavily on Animate
    // still by its meaning it belongs to the type
    // we know gears are down as default (see init above)
    Assert(FutureVisualState()._gearsUp==0);
    Type()->_landContactPlane = LandContactMinY(_shape);
    Type()->_landContactPlaneValid = true;
  }

  // by default we are suspended - MakeAirborne or impulse will wake up us as needed
  _isStopped = true;
  
  _cmIndexToggle = 0;
  
  // strict VTOL - while stopped, make sure we are vectored
  if (Type()->_vtol>=3)
  {
    FutureVisualState()._thrustVector = _thrustVectorWanted = 0.8;
  }
}

Airplane::~Airplane()
{
}


AirplaneVisualState::AirplaneVisualState(AirplaneType const& type)
:base(type),
_flaps(0),
_gearsUp(0),
_rotorPosition(0),
_elevator(0),
_rudder(0),
_thrustVector(0),
_aileron(0),
_brake(0),
_wheelPhase(0),
_cabinPos(0),
_rpm(0)
{
}


void AirplaneVisualState::Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const
{
  base::Interpolate(obj, t1state, t, interpolatedResult);

  AirplaneVisualState const& s = static_cast<AirplaneVisualState const&>(t1state);
  AirplaneVisualState& res = static_cast<AirplaneVisualState&>(interpolatedResult);

  #define INTERPOLATE(x) res.x = x + t*(s.x - x)

  INTERPOLATE(_flaps);  // 0..1
  INTERPOLATE(_gearsUp);  // -1..1
  INTERPOLATE(_rudder);  // -1..1
  INTERPOLATE(_aileron);  // -1..1
  INTERPOLATE(_elevator);  // -1..1
  INTERPOLATE(_cabinPos);  // 0..1
  INTERPOLATE(_brake);  // 0..1
  INTERPOLATE(_thrustVector);  // -1..1
  INTERPOLATE(_rotorPosition);  // continuous (maybe should be mod k*pi)
  INTERPOLATE(_wheelPhase);  // continuous (maybe should be mod k*pi)
  INTERPOLATE(_rpm);  // continuous

  #undef INTERPOLATE
}


AirplaneType::AirplaneType( ParamEntryPar param )
:base(param)
{
  _gunPos = VZero;

  // 0.04 = tan (2.29 deg)
  //_gunDir = Vector3(0,-0.04f,1).Normalized();
  float aimDown = param >> "gunAimDown";
  _gunDir = Vector3(0,-aimDown,1).Normalized();
  _rocketDir = _gunDir;
  //_gunDir = VForward;

  _landContactPlaneValid = false;

  _hullHit = -1;
}

const float BoostVTOL = 1.85f;

void AirplaneType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  _minGunElev=HDegree(par>>"minGunElev");
  _maxGunElev=HDegree(par>>"maxGunElev");
  _minGunTurn=HDegree(par>>"minGunTurn");
  _maxGunTurn=HDegree(par>>"maxGunTurn");

  _altFullForce = par.ReadValue("altFullForce",5000);
  _altNoForce = par.ReadValue("altNoForce",13000);

  _gearRetracting = par>>"gearRetracting";
  _cabinOpening = par>>"cabinOpening";
  _vtol = par>>"vtol";

  _lightOnGear = par>>"lightOnGear";

  float t = par >> "gearDownTime";
  _gearDownSpeed = t > 0 ? 1.0f / t : 0;
  t = par >> "gearUpTime";
  _gearUpSpeed = t > 0 ? 1.0f / t : 0;

  _leftDust =  par >> "leftDustEffect";
  _rightDust =  par >> "rightDustEffect";
  _damageEffect = par >> "damageEffect";

  _dustEffect =  par >> "dustEffect";
  _waterEffect =  par >> "waterEffect";

  // load Envelope from config
  ParamEntryVal envelope = par>>"envelope";
  _envelope.Realloc(envelope.GetSize());
  _envelope.Resize(envelope.GetSize());
  for (int i=0; i<_envelope.Size(); i++)
  {
    _envelope[i] = envelope[i];
  }
  
  _aileronSensitivity = par>>"aileronSensitivity";
  _elevatorSensitivity = par>>"elevatorSensitivity";
  _wheelSteeringSensitivity = par>>"wheelSteeringSensitivity";
  _landingSpeed = par>>"landingSpeed";
  if (_landingSpeed<1)
  {
    _landingSpeed = GetMaxSpeedMs()*0.33f;
    saturateMax(_landingSpeed,120/3.6f);
  }
  else
  {
    _landingSpeed *= 1.0f/3.6f;
  }
  if (_landingSpeed>21)
  {
    _stallSpeed = _landingSpeed*0.65f;
  }
  else
  {
    _stallSpeed = _landingSpeed*0.87f;
  }
  _takeOffSpeed = floatMin(_landingSpeed,65);

  _flapsFrictionCoef = par>>"flapsFrictionCoef";
  _hasFlaps = par>>"flaps";
  _hasAirbrake = par>>"airBrake";
  _landingAoa = par>>"landingAoa";
  
  /// units will eject once the plane is damage too much
  _ejectDamageLimit = par>>"ejectDamageLimit";

  _ejectSpeed = Vector3
  (
    (par>>"ejectSpeed")[0],
    (par>>"ejectSpeed")[1],
    (par>>"ejectSpeed")[2]
  );

  if (_vtol==1)
  {
    // auto-detect STOVL variant
    float maxSpeed = GetMaxSpeedMs();
    float boost0 = BoostVTOL;
    float boost0Needed = (13/0.4f)*InvSqrt(maxSpeed)*1.05f;
    if (boost0<boost0Needed)
    {
      // not enough power for vertical takeoff - mark as STOVL
      _vtol = 2;
    }
  }

  #if 0 // _DEBUG
      // force strict VTOL
    if (_vtol) _vtol = 3;
  #endif
  SetHardwareAnimation(par);
}

#include <Es/Common/delegate.hpp>

/*!
\patch 5148 Date 3/26/2007 by Ondra
- Fixed: New animation controller noseWheelTurn.
*/

static const struct AnimSourceDesc{
    const char *name;
    float (Airplane::*func)(ObjectVisualState const&) const;
} desc[]=
{
    {"gear",&Airplane::GetCtrlGearPos},
    {"flap",&Airplane::GetCtrlFlapPos},
    {"elevator",&Airplane::GetCtrlElevatorPos},
    {"rotor",&Airplane::GetCtrlRotorPos},
    {"rudder",&Airplane::GetCtrlRudderPos},
    {"aileronT",&Airplane::GetCtrlAileronTPos},
    {"aileronB",&Airplane::GetCtrlAileronBPos},
    {"aoa",&Airplane::GetCtrlAOA},
    {"aileron",&Airplane::GetCtrlAileron},
    {"speedBrake",&Airplane::GetCtrlSpeedBrake},
    {"wheel",&Airplane::GetCtrlWheelPos},
    {"cabin",&Airplane::GetCtrlCabinOpen},
    {"thrustVector",&Airplane::GetCtrlThrustVectorPos},
    {"noseWheelTurn",&Airplane::GetCtrlNoseWheelPos},
};
AnimationSource *AirplaneType::CreateAnimationSource(const AnimationType *type, RStringB source)
{
 for (int i=0; i<lenof(desc); i++)
  {
    if (!strcmpi(source,desc[i].name))
    {
      return _animSources.CreateAnimationSource(desc[i].func);
    }
  }
  return base::CreateAnimationSource(type,source);
}

/*!
\patch_internal 1.06 Date 7/18/2001 by Jirka
- Added: alternate name for airplane flap axis

\patch 1.43 Date 1/24/2002 by Ondra
- New: Support for plane models with multiple propellers.
*/

void AirplaneType::InitShape()
{
  base::InitShape();

  ParamEntryVal par = *_par;

  _rotorStill.Init(_shape,(par >> "selectionRotorStill").operator RString(),NULL);
  _rotorMove.Init(_shape,(par >> "selectionRotorMove").operator RString(),NULL);

  _gunPos=_shape->MemoryPoint((par >> "memoryPointGun").operator RString());

  _rocketLPos=_shape->MemoryPoint((par >> "memoryPointLRocket").operator RString());
  _rocketRPos=_shape->MemoryPoint((par >> "memoryPointRRocket").operator RString());

  _lDust=_shape->MemoryPoint((par >> "memoryPointLDust").operator RString());
  _rDust=_shape->MemoryPoint((par >> "memoryPointRDust").operator RString());

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
  if (_cmDir.Size() == 0) 
    _cmDir.Add(Vector3(1,0,0));

  int level;
  level=_shape->FindLevel(1100);
  if( level>=0 )
  {
    _shape->Level(level)->MakeCockpit();
  }
  level=_shape->FindLevel(1000);
  if( level>=0 )
  {
    _shape->Level(level)->MakeCockpit();
  }

  _hullHit = FindHitPoint("HitHull");
  _hitZoneTextureUV.Add(1);
}

void AirplaneType::DeinitShape()
{
  base::DeinitShape();
}

void AirplaneType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);

  _rotorStill.InitLevel(_shape,level);
  _rotorMove.InitLevel(_shape,level);
}

void AirplaneType::DeinitShapeLevel(int level)
{
  _rotorStill.DeinitLevel(_shape,level);
  _rotorMove.DeinitLevel(_shape,level);
  
  base::DeinitShapeLevel(level);
}

Object* AirplaneType::CreateObject(bool unused) const
{ 
  return new AirplaneAuto(this, NULL, unused);
}


float AirplaneType::GetStopDistance() const
{
  float stopConfig = base::GetStopDistance();
  // we assume all airplanes have a similar braking capabilities
  // assume we may be able to slow down about 1 G
  // experiments with F35B have shown 0.5 G is enough, but 1 G is not enough
  // for MV22 even 0.5 was not enough
  float a = 0.33*G_CONST;
  // time to slow down is (cruising speed-stopped speed)/a
  float cruiseSpeed = floatMax(GetMaxSpeedMs()*0.75f,_landingSpeed);
  float stopSpeed = 0;
  float timeToSlowDown = (cruiseSpeed-stopSpeed)/a;
  float avgSpeed = (cruiseSpeed+stopSpeed)*0.5f;
  // distance to slow down from cruising speed to a stopped speed is:
  float distToBrake = timeToSlowDown*avgSpeed;
  // avoid having the distance too large - in such case it is better to circle around a bit
  saturateMin(distToBrake,3000);
  // average speed is: (cruising speed+stall speed)*0.5
  return floatMax(stopConfig,distToBrake);
}

float AirplaneType::UpForce(float speed, float aoa, float maxSpeed, float flaps, float groundEff) const
{
  // ignore fly envelope - simulate low flight only
  Vector3 force(VZero);
  // speed -> up force diagram
  // speed in knots
  // force in G
  const int maxI=_envelope.Size()-1;
  const float maxAoa=18*H_PI/180;
  const float minAoa=-10*H_PI/180;
  const float zeroAoA = 2*H_PI/180;
  aoa += zeroAoA;

  // flaps add to AoA partially, but they make stall speed lower as well
  aoa += flaps*(1*H_PI/180);

  float speedRel = speed*0.8f/maxSpeed;
  
  float nearStall = InterpolativC(speedRel,0.22f,0.4f,1,0);
  aoa += flaps*(4*H_PI/180)*nearStall;
 
  if( aoa>maxAoa )
  {
    aoa=2*maxAoa-aoa; 
    if( aoa<0 ) aoa=0;
  }
  if( aoa<minAoa )
  {
    aoa=minAoa;
  }
  aoa *=  1/maxAoa;
  //aoa *= fabs(aoa);

  // base on airplane maxspeed 
  float fSpeed=maxI*speedRel;

  // 
  int iLow=toIntFloor(fSpeed);
  float frac=fSpeed-iLow;
  float ret = 0;
  if( iLow>=0 )
  {
    if( iLow>=maxI ) ret = _envelope[maxI-1];
    else
    {
      float fLow=_envelope[iLow];
      float fHigh=_envelope[iLow+1];
      float speedCoef = fLow+(fHigh-fLow)*frac;
      ret = speedCoef*aoa;
    }
  }
  #if 0
    GlobalShowMessage
    (
      100,"fSpeed %.3f, speedCoef %.1f, Up coef %.1f, aoa %.1f",
      fSpeed,speedCoef,ret,aoa
    );
  #endif
  #if 0
  LogF
  (
    "fSpeed %.3f, speedCoef %.1f, Up coef %.1f, aoa %.1f",
    fSpeed,speedCoef,ret,aoa
  );
  #endif
  if (ret>=0) ret *= 1+groundEff;
  return ret;
}

#if _HELISIM_IMPERIAL_SYSTEM
float AirplaneType::ConvertDistance(float meters, int &stringIDS) const 
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

float AirplaneType::ConvertSpeed(float metersPerSecond) const 
{
  return Glob.config.useImperialSystem > 0? METER_TO_KNOT(metersPerSecond) : METER_TO_KM_PER_HOUR(metersPerSecond);
};

float AirplaneType::ConvertHeight(float meters) const 
{
  return Glob.config.useImperialSystem > 0? METER_TO_FEET(meters) : meters;
};
#endif

// Visual state functions

float AirplaneVisualState::GetGearPos(const AirplaneType *type) const
{
  if (!type->_gearRetracting)
  {
    return 0;
  }
  return _gearsUp;
}

float AirplaneVisualState::GetNoseWheelPos(Vector3Par modelSpeed) const
{
  const float noSteer = 90/3.6f; // speed where wheel steering is no longer applied
  float noseSteerFactor = InterpolativC(fabs(modelSpeed.Z()),0,noSteer,1,0);
  return noseSteerFactor*_rudder;
}


float AirplaneVisualState::GetRotorPos() const
{
  return (1.0f / (2.0f * H_PI)) * fmod(_rotorPosition, 2.0f * H_PI);
}

float AirplaneVisualState::GetAileronTPos() const
{
  float abOff = _brake * 1.5f;
  float tAileron = floatMin(_aileron * 0.5f + abOff, +1.4f);
  return tAileron;
}

float AirplaneVisualState::GetAileronBPos() const
{
  float abOff = _brake * 1.5f;
  float bAileron = floatMax(_aileron * 0.5f - abOff, -1.4f);
  return bAileron;
}

/// angle of incidence - difference between forward and airfold chord line
const float AngleOfIndicence = 3*H_PI/180;

/// angle of attack - angle between airflow and airfold (wing) chord line
inline float CalculateAOA(Vector3Par speed)
{
  float speedSize2 = speed.SquareSize();
  return speedSize2>1e-6 ? -speed.Y()*InvSqrt(speedSize2)+AngleOfIndicence : 0;
}

float AirplaneVisualState::GetAOA(Vector3Par speed) const
{
  return CalculateAOA(speed);
}

#define FV(x) (x)[0],(x)[1],(x)[2]
#define FFV "[%.0f,%.0f,%.0f]"

void AirplaneAuto::DrawDiags()
{
  if (CHECK_DIAG(DECombat))
  {

    LODShape *forceArrow=GScene->ForceArrow();
    #define DRAW_OBJ(obj) GScene->DrawObject(obj)
  
    // draw artificial horizon vector
    {
      Vector3 asideDir; //= orient.DirectionAside();
      asideDir.Init();
      asideDir[0] = +RenderVisualState().Direction().Z();
      asideDir[1] = 0;
      asideDir[2] = -RenderVisualState().Direction().X();
  
      Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);
      arrow->SetPosition(RenderVisualState().PositionModelToWorld(Vector3(0,0,15)));
      float size=0.5;
      arrow->SetScale(size);
      arrow->SetOrient(asideDir,VUp);
      arrow->SetPosition(arrow->RenderVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size));
      arrow->SetConstantColor(PackedColor(Color(1,0,1)));
      DRAW_OBJ(arrow);
    }
    // draw ILS position and direction
  

    Vector3 ilsPos = RenderVisualState().Position(),ilsDir = RenderVisualState().Direction();
    bool ilsSTOL = false;
    GWorld->GetILS(ilsPos,ilsDir,ilsSTOL,_targetSide,Type()->_vtol>0,PreferredAirport());
    {
      Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);

      float size=3;
      arrow->SetPosition(ilsPos);
      arrow->SetOrient(VUp,VForward);
      arrow->SetPosition(arrow->RenderVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size));
      arrow->SetScale(size);
      arrow->SetConstantColor(PackedColor(Color(0,0,0)));
      DRAW_OBJ(arrow);
    }

    //if (!QIsManual())
    {
      Vector3 dir = Matrix3(MRotationY,-_pilotHeading).Direction();

      Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);

      float size=0.3f;
      arrow->SetPosition(FutureVisualState().Position()+VUp*2);
      arrow->SetOrient(dir,VUp);
      arrow->SetPosition
      (
        arrow->RenderVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size)
      );
      arrow->SetScale(size);
      arrow->SetConstantColor(PackedColor(Color(0,1,0)));
      DRAW_OBJ(arrow);
    }

    {
      Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);

      float size=1;
      arrow->SetPosition(ilsPos+ilsDir*10);
      arrow->SetOrient(VUp,VForward);
      arrow->SetPosition
      (
        arrow->RenderVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size)
      );
      arrow->SetScale(size);
      arrow->SetConstantColor(PackedColor(Color(0,0,0)));
      DRAW_OBJ(arrow);

    }

    if( _sweepTarget )
    {
      {
        Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);

        float size=5;
        arrow->SetPosition(_sweepTarget->AimingPosition());
        arrow->SetOrient(VUp,VForward);
        arrow->SetPosition
        (
          arrow->RenderVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size)
        );
        arrow->SetScale(size);
        arrow->SetConstantColor(PackedColor(Color(1,0,0)));
        DRAW_OBJ(arrow);

      }
      {
        Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);

        float size=5;
        Vector3 pos=RenderVisualState().Position()+RenderVisualState().Direction()*10;
        arrow->SetPosition(pos);
        arrow->SetOrient(_sweepTarget->AimingPosition()-pos,VUp);
        arrow->SetPosition
        (
          arrow->RenderVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size)
        );
        arrow->SetScale(size);
        arrow->SetConstantColor(PackedColor(Color(1,0,0)));
        DRAW_OBJ(arrow);

      }
    }
  }

  base::DrawDiags();
}

RString AirplaneAuto::DiagText() const
{
  RString text=base::DiagText();
  char buf[256];
  sprintf(buf," h=%.1f, d=%.2f",_pilotHeight,_forceDive);
  if( _sweepTarget )
  {
    sprintf(buf+strlen(buf)," sweep %d",_sweepState);
  }
  return text+RString(buf);
}

inline const float DecreaseAbs(float x, float y)
{
  if (fabs(x)<y) return 0;
  if (x>0) return x-y;
  return x+y;
}

float Airplane::DetectStall() const
{
  if (_landContact) return 0;
  float aoa = CalculateAOA(FutureVisualState().ModelSpeed());

  //float landingSpeed = Type()->_landingSpeed;
  float stallSpeed = Type()->_stallSpeed;

  // with VTOL we do not want to stall, but thrust vectoring prevents stall only when there is sufficient thrust
  float vtolFactor = Type()->_vtol ? 1-floatMax(FutureVisualState()._thrustVector*InterpolativC(_thrust,0,0.5,0,1),0) : 1;
  stallSpeed *= vtolFactor;
  if (stallSpeed<=0.1f) return 0;
  
  float stall = 1.5-FutureVisualState().ModelSpeed().Z()/stallSpeed;
  saturateMax(stall,0);
  stall += 0.3f;
  saturateMin(stall,1);

  const float maxAoa = 15*H_PI/180;
  const float okAoa = 10*H_PI/180;
  
  float aoaStall = (fabs(aoa)-okAoa)/maxAoa;
  saturate(aoaStall,0,1);

  //GlobalShowMessage(100,"Stall %.3f, %.3f",stall,aoaStall);
  float stallResult = stall*aoaStall;
  
  stallResult *= vtolFactor;
  return stallResult;
  
}

void Airplane::PerformFF(FFEffects &eff)
{
  base::PerformFF(eff);
  // stall warning
  eff.engineFreq = 7;
  // stall warning
  eff.engineMag = DetectStall()*0.25f;
  eff.stiffnessX = 0.6f;
  eff.stiffnessY = 0.6f;
}

float Airplane::GetHitForDisplay(int kind) const
{
  // see InGameUI::DrawTankDirection
  switch (kind)
  {
  case 0: return GetHitCont(Type()->_hullHit);
  default: return 0;
  }
}

// torque applied if speed is big enough, and only in same direction
            
static void LimitFriction(float &friction, float speed)
{
  float maxFriction = speed*10;
  if (speed>0)
  {
    if (friction>maxFriction) friction = maxFriction;
    if (friction<0) friction = 0;
  }
  else if (speed<0)
  {
    if (friction<maxFriction) friction = maxFriction;
    if (friction>0) friction = 0;
  }
}


/*!
\patch 1.02 Date 7/10/2001 by Ondra.
- Fixed: Plane - less speed loss while turning.
\patch 1.22 Date 8/30/2001 by Ondra.
- Fixed: Planes bouncing when falling in water.
\patch 1.24 Date 9/20/2001 by Ondra.
- Fixed: Airplane ground target engaging improved.
\patch 1.30 Date 10/20/2001 by Ondra.
- Fixed: MP: Airplane gear could be damaged on remote computer.
\patch 1.43 Date 1/25/2002 by Ondra
- Fixed: Airplane climbing in high altitudes limited.
Maximum possible altitude is now about 12000 m.
\patch 1.58 Date 5/19/2002 by Ondra
- Fixed: Airplane jumping after crash fixed.
\patch 1.89 Date 10/22/2002 by Ondra
- New: Adjustable plane wheel steering (wheelSteeringSensitivity).
Should make big plane taxiing possible.
\patch 1.90 Date 10/30/2002 by Ondra
- Fixed: Airplane engine produces less thrust when speed is near plane top speed.
\patch 1.90 Date 10/30/2002 by Ondra
- Improved: Better airplane AI dogfighting.
\patch 1.90 Date 11/1/2002 by Ondra
- Fixed: Keyboard rudder control enabled while taxiing.
\patch 1.90 Date 11/1/2002 by Ondra
- Fixed: Airplanes were turning south when climbing while flying east or west.
\patch 5127 Date 2/9/2007 by Ondra
- Added: New animation controller cabin for airplanes.
\patch 5128 Date 2/12/2007 by Ondra
- Change: airplane fuel consumption dependent on thrust. 
\patch 5137 Date 3/1/2007 by Ondra
- Fixed: Increased thrust when hovering stable in VTOL mode.
*/
void Airplane::Simulate( float deltaT, SimulationImportance prec )
{
  _isDead = IsDamageDestroyed();

  if( FutureVisualState()._rpm>0 )
    ConsumeFuel(deltaT*0.3f*(Square(_thrust)+0.1f));
  
#if _VBS2 //problem with IWV where wheels and props were still animated after moveTo  
  if (!HasSomePilot())
  {
    _pilotBrake = true;
    //_engineOff = true;
  }

  AIBrain *driver = PilotUnit();
  if (driver && !driver->LSIsAlive())
  {
    // driver is dead - make the plane slowing down, sometimes make random control changes
    _thrustWanted -= deltaT*0.1f;
    if (_thrustWanted<=0)
    {
      _pilotBrake = true;
      _thrustWanted = 0;
    }
  }
#endif

  float delta;

  if( _isDead ) _engineOff=true,_pilotBrake=1;
  if( FutureVisualState()._fuel<=0 ) _engineOff=true;
  if( _engineOff )
  {
    _thrustWanted=0;
    if( _landContact ) _pilotBrake=1;
  }

  // main engine
  delta=!_engineOff-FutureVisualState()._rpm;
  Limit(delta,-0.1f*deltaT,+0.1f*deltaT);
  FutureVisualState()._rpm+=delta;
  Limit(FutureVisualState()._rpm,0,1);

  delta=_thrustWanted-_thrust;
  Limit(delta,-0.7f*deltaT,+0.4f*deltaT);
  _thrust+=delta;
  Limit(_thrust,0,1);

  _rotorSpeed = 10.0f * FutureVisualState()._rpm * (0.4f + _thrust);
  FutureVisualState()._rotorPosition += _rotorSpeed * deltaT * 20.0f;

  Vector3Val speed=FutureVisualState().ModelSpeed();
  //float landingSpeed = Type()->_landingSpeed;
  //float stallSpeed = Type()->_stallSpeed;

  float altCoef = 1;
  float alt = FutureVisualState().Position().Y();

  float altFullForce = Type()->_altFullForce;
  float altNoForce = Type()->_altNoForce;
  if (alt>altFullForce)
  {
    if (alt>altNoForce) altCoef = 0;
    else altCoef = 1-(alt-altFullForce)*(1/(altNoForce-altFullForce));
  }

  float flapEff = fabs(speed[2])/(Type()->_landingSpeed*0.65f)-0.5f;
  saturate(flapEff,0,1);
  flapEff *= altCoef;
  //GlobalShowMessage
  //(
  //  100,"Eff %.2f, speed %.2f, stall %.2f",
  //  flapEff,speed[2],stallSpeed
  //);

  // coordinate rudder with bank
  // control rudder only when moving slow
  // auto coordination of the turn
  // apply the rudder to counteract the slip while in turn
  static float autoCoordSlow = -0.4f; // speed 300 km/h
  static float autoCoordFast = -0.1f; // speed 700 km/h
  float autoCoord = InterpolativC(FutureVisualState().ModelSpeed().Z(),300/3.6,700/3.6,autoCoordSlow,autoCoordFast);
  float rudderAuto = FutureVisualState().ModelSpeed().X()*autoCoord;
  // give manual rudder preference over an automatic one
  // when it is in the opposite direction, use only the manual
  // when in the same direction, apply both
  float rudderWanted = _rudderWanted*rudderAuto>=0 ? rudderAuto+_rudderWanted : _rudderWanted;
  
  //float rudderWanted = bank*flapEff+_rudderWanted; //*(1-flapEff);
  const float fullElevSpeed = floatMin(25,Type()->_landingSpeed*0.5f);
  const float noElevSpeed = fullElevSpeed*0.5f;
  if( speed[2]<fullElevSpeed && _landContact )
  {
    float maxElev = (speed[2]-13.5f)/(fullElevSpeed-noElevSpeed);
    saturate(maxElev,0,1);
    // do not use ailerons and elevators when ground steering
    saturate(_aileronWanted,-maxElev,+maxElev);
    saturate(_elevatorWanted,-maxElev,+maxElev);
    //_aileronWanted=0;
    //_elevatorWanted=0;
  }

  // change rudder
  delta=rudderWanted-FutureVisualState()._rudder;
  Limit(delta,-4*deltaT,4*deltaT);
  FutureVisualState()._rudder+=delta;
  Limit(FutureVisualState()._rudder,-1,+1);

  // change aileron
  delta=_aileronWanted-FutureVisualState()._aileron;
  Limit(delta,-4*deltaT,4*deltaT);
  FutureVisualState()._aileron+=delta;
  Limit(FutureVisualState()._aileron,-1,+1);
  
  // change elevator
  delta=_elevatorWanted-FutureVisualState()._elevator;
  Limit(delta,-4*deltaT,4*deltaT);
  FutureVisualState()._elevator+=delta;
  Limit(FutureVisualState()._elevator,-1,+1);
  //DIAG_MESSAGE(100,Format("Elevator = %0.2f", _elevator));

  bool doServo=false;

  // change gears
  float gearWanted = 1-_pilotGear;
  if( _gearDamage ) gearWanted=-0.66f;
  if (!Type()->_gearRetracting) gearWanted = 0;
  float oldGears = FutureVisualState()._gearsUp;
  delta = gearWanted - oldGears;
  if( fabs(delta)>0.01 ) doServo=true;

  Limit(delta,-Type()->_gearDownSpeed * deltaT,+Type()->_gearUpSpeed * deltaT);
  FutureVisualState()._gearsUp+=delta; 
  Limit(FutureVisualState()._gearsUp,-1,1);

  if (Type()->_cabinOpening)
  {
    float cabinPosWanted = FutureVisualState()._cabinPos;
    // we always respect _openCabin true as non respecting it could cause get-in/out activity stuck forever
    if (!IsDamageDestroyed() || _openCabin)
      cabinPosWanted = _openCabin;
    // when somebody is in, decide when cabin canopy should close/open
    if (CommanderUnit())
    {
      // avoid oscillation
      if (FutureVisualState()._rpm<=0.1f && _landContact && FutureVisualState()._speed.SquareSize()<Square(1)) cabinPosWanted = 1;
      if ((FutureVisualState()._rpm>=0.7f && !IsStopped()) || !_landContact || FutureVisualState()._speed.SquareSize()>Square(2)) cabinPosWanted = 0;
    }
    delta = cabinPosWanted-FutureVisualState()._cabinPos;
    Limit(delta,-deltaT*0.5f,deltaT*0.5f);
    FutureVisualState()._cabinPos += delta;
    Limit(FutureVisualState()._cabinPos,0,1);
  }


  const float tholdUp = 0.95f;
  const float tholdDown = 0.05f;
  float gears = FutureVisualState()._gearsUp;
  if (gears >= tholdUp && oldGears < tholdUp) OnEvent(EEGear,false);
  if (gears <= tholdDown && oldGears > tholdDown) OnEvent(EEGear,true);
  
  // change flaps
  if (Type()->_hasFlaps)
  {
    delta=_pilotFlaps*0.5-FutureVisualState()._flaps;
    if( fabs(delta)>0.01 ) doServo=true;
    Limit(delta,-0.33*deltaT,0.33*deltaT);
    FutureVisualState()._flaps+=delta;
    Limit(FutureVisualState()._flaps,0,1);
  }

  delta=doServo-_servoVol;
  Limit(delta,-2*deltaT,2*deltaT);
  _servoVol+=delta;

  // _brake controls wheel brake as well
  // change brakes
  float brakeWanted=_pilotBrake;
  if( _landContact && fabs(speed[2])<10 ) brakeWanted=0;
  if( _isDead ) brakeWanted=0;
  delta=brakeWanted-FutureVisualState()._brake;
  Limit(delta,-1*deltaT,1*deltaT);
  FutureVisualState()._brake+=delta;
  Limit(FutureVisualState()._brake,0,1);

  if (Type()->_vtol && EngineIsOn())
  {
    float wanted = floatMinMax(_thrustVectorWanted,0,1);
    // strict VTOL - while stopped, make sure we are highly vectored 
    if (Type()->_vtol>=3 && _landContact)
    {
      if (!QIsManual(EffectivePilotingUnit())) wanted = 1;
      else saturateMax(wanted,0.8f);
    }

    if (wanted<=0) wanted = _vtolMode ? 0 : -1;
    else if (wanted>1) wanted = 1;
    delta = wanted-FutureVisualState()._thrustVector;
    float animSpeed = 0.5f;
    if (FutureVisualState()._thrustVector<=0) animSpeed = 1;
    Limit(delta,-animSpeed*deltaT,animSpeed*deltaT);
    FutureVisualState()._thrustVector += delta;
    Limit(FutureVisualState()._thrustVector,-1,1);
  }

  if (!_landContact)
  {
    float wheelSpeedWanted = 0;
    float delta = wheelSpeedWanted-_wheelSpeed;
    Limit(delta,-10*deltaT,+10*deltaT);
    _wheelSpeed += delta;
  }
  else
  {
    float wheelSpeedWanted = FutureVisualState().ModelSpeed().Z();
    float delta = wheelSpeedWanted-_wheelSpeed;
    Limit(delta,-100*deltaT,+100*deltaT);
    _wheelSpeed += delta;
  }
  
  FutureVisualState()._wheelPhase += _wheelSpeed*deltaT;

  // wake up the simulation as needed
  if (fabs(_thrust)>0 && FutureVisualState()._rpm>0 || FutureVisualState()._speed.SquareSize() > 0.1f || _angVelocity.SquareSize() > 0.1f)
    OnMoved();
  if( _impulseForce.SquareSize() + _impulseTorque.SquareSize()>0 )
    OnMoved();

  if (GetStopped())
  {
    // reset impulse - avoid cumulation
    _impulseForce = VZero;
    _impulseTorque = VZero;
    FutureVisualState()._modelSpeed = VZero;
    FutureVisualState()._speed = VZero;
    _angVelocity = VZero;
    _angMomentum = VZero;
  }

  // do not predict too much (MP)
  if (!GetStopped() && !CheckPredictionFrozen() && !IsAttached())
  {
    float dx,dz;
    float height = FutureVisualState().Position()[1] - GLandscape->RoadSurfaceYAboveWater(FutureVisualState().Position(),&dx,&dz);

    // calculate all forces, frictions and torques
    Vector3 force(VZero),friction(VZero);
    Vector3 torque(VZero),torqueFriction(VZero);
    
    // world space center of mass
    Vector3 wCenter(VFastTransform,FutureVisualState().ModelToWorld(),GetCenterOfMass());

    // partial force and application point
    Vector3 pForce(VZero),pCenter(VZero);
    float maxSpeed = GetType()->GetMaxSpeedMs();

    // implement VTOL vectoring
    float zFactor = 1;
    float yFactor = 0;
    
    if (Type()->_vtol)
    {
      if (FutureVisualState()._thrustVector>0)
      {
        float thrustAngle = FutureVisualState()._thrustVector*(H_PI/2);
        zFactor = cos(thrustAngle);
        yFactor = sin(thrustAngle);
      }

      // we need artificially increased thrust because with thrust only vectored STOL was not possible
      // adjusting the whole flight model for realistic thrust values would be too much work
      float boost0 = BoostVTOL;
      if (Type()->_vtol>=3)
      {
        // if the plane is strict VTOL, we need the thrust to be strong enough to take off
        /*
        // thrust computations:
        float spdCoef = Interpolativ(speed.Z(),maxSpeed*0.66f,maxSpeed*1.15f,1,0);
        float thrust = _thrust*FutureVisualState()._rpm*sqrt(maxSpeed)*(0.4*G_CONST/13)*altCoef*spdCoef;
        pForce = Vector3(0,thrust*yFactor*GetMass(),thrust*zFactor*GetMass());
        // substituted: zFactor = 0, yFactor = boost, speed.Z()=0, _thrust=1, FutureVisualState()._rpm=1, altCoef = 1
        pForce.Y() = sqrt(maxSpeed)*(0.4*G_CONST/13)*boost*GetMass()
        boost = pForce.Y()/(sqrt(maxSpeed)*(0.4*G_CONST/13)*GetMass())
        // We need pForce.Y()>=GetMass()*G_CONST
        boost = 1/(sqrt(maxSpeed)*(0.4/13))
        */
        float boost0Needed = (13/0.4f)*InvSqrt(maxSpeed)*1.05f;
        if (boost0<boost0Needed)
          boost0 = boost0Needed;
      }
      // as the speed goes up, reduce vectoring
      float boost = Interpolativ(speed.Z(),maxSpeed*0.2f,maxSpeed*0.5f,boost0,1.0f);
      yFactor *= boost;
      zFactor *= boost; 
    }

    // apply main engine force in COM
    // once top speed is reached, engine becomes ineffective
    float spdCoef = Interpolativ(speed.Z(),maxSpeed*0.66f,maxSpeed*1.15f,1,0);
    
    float thrust = _thrust*FutureVisualState()._rpm*sqrt(maxSpeed)*(0.4*G_CONST/13)*altCoef*spdCoef;
    pForce = Vector3(0,thrust*yFactor*GetMass(),thrust*zFactor*GetMass());
    
    force += pForce;
    
    // real aoa - angle between plane axis and plane speed direction
    // assume x==sin(x) for small x
    float aoa = CalculateAOA(FutureVisualState().ModelSpeed());

    float wingSpan = _shape->BoundingSphere()*1.5f; // assume wing span is related to the airplane radius
    float groundEff = 0;
    if (wingSpan>0)
    {
      float relHeight = height/wingSpan;
      // 10 % at most - see http://en.wikipedia.org/wiki/Ground_effect_(aircraft)
      groundEff = Square(floatMinMax(1.5f-relHeight,0,1))*0.1f;
    }

    float upForce=Type()->UpForce(fabs(speed[2]),aoa,maxSpeed,FutureVisualState()._flaps,groundEff)*altCoef;
    // apply wing up force
    force[1]+=upForce*G_CONST*GetMass();
    
    if (Type()->_vtol && FutureVisualState()._thrustVector>0.1f)
    {
      // nozzle vectored - use reaction control system
      float fullRSC = Type()->_stallSpeed*0.8;
      float noRSC = Type()->_landingSpeed;
      float aerodyn = Interpolativ(FutureVisualState().Speed().SquareSize(),Square(fullRSC),Square(noRSC),1,0);
      // bank / dive control
      static float hoverX = +2.5f;
      static float hoverZ = -2.5f;
      pForce = Vector3(0,GetMass()*2.0f,0);
      pCenter = Vector3(FutureVisualState()._aileron*hoverX,0,FutureVisualState()._elevator*hoverZ);
      torque += pCenter.CrossProduct(pForce)*aerodyn;
      // rotation control
      static float hoverY = -2.5f;
      pForce = Vector3(GetMass()*2.0f*FutureVisualState()._rudder,0,0);
      pCenter = Vector3(0,0,hoverY);
      torque += pCenter.CrossProduct(pForce)*aerodyn;
    }
    #if ARROWS
      // gravity + wing - green
      AddForce(wCenter, FutureVisualState().DirectionModelToWorld(force)*InvMass(), Color(0,1,0));
    #endif
    Assert( force.Size()<G_CONST*GetMass()*10 );
    {
      const float noSteer = 90/3.6f; // speed where wheel steering is no longer applied
      float absSpeed = fabs(speed[2]);
      if( _landContact && absSpeed<noSteer && FutureVisualState().DirectionUp().Y()>0)
      {
        // steering engaged
        pCenter=Vector3(0,0,0.75);
        const float bestSteer = 6.67f; // m/s -> 24 km/h
        float steerEff = 1;
        if (absSpeed<bestSteer)
          steerEff = absSpeed/bestSteer;
        else
          steerEff = (noSteer-absSpeed)/(noSteer-bestSteer);
        saturateMin(steerEff,40);
        steerEff *= Type()->_wheelSteeringSensitivity*40;
        pForce=Vector3(-FutureVisualState()._rudder*GetMass()*steerEff,0,0);
        torque+=pCenter.CrossProduct(pForce);
        Assert(torque.IsFinite());
        #if ARROWS
          AddForce(FutureVisualState().DirectionModelToWorld(pCenter)+wCenter, FutureVisualState().DirectionModelToWorld(pForce)*InvMass());
        #endif
      }
    }
    // apply rudder/ailerons
    // ailerons are applied always, even when ground steering is active
    float sizeCoef = GetRadius()*(1.0/6)*Type()->_aileronSensitivity;
    pCenter = Vector3(5,0,0);
    pForce = Vector3(0,FutureVisualState()._aileron*GetMass()*flapEff*G_CONST*0.8*sizeCoef,0);
    torque += pCenter.CrossProduct(pForce);
    Assert(torque.IsFinite());
    #if ARROWS
      // ailerons - grey
      AddForce(FutureVisualState().DirectionModelToWorld(pCenter)+wCenter, FutureVisualState().DirectionModelToWorld(pForce)*InvMass(), Color(0.5,0.5,0.5));
    #endif
    // apply elevator
    static float coefElev = 1.0f;
#if _ENABLE_CHEATS
    if (CHECK_DIAG(DEMouseSensitivity))
    {
      if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPADSLASH)) coefElev -= 0.05;
      else if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPAD8)) coefElev += 0.05;
      saturate(coefElev,0.05, 50);
      DIAG_MESSAGE(500, Format("ElevatorSensitivity = %.2f", coefElev*Type()->_elevatorSensitivity));
    }
#endif
    const float elCoef = 0.6f;
    pForce=Vector3(0,(FutureVisualState()._elevator+_elevatorTrim)*GetMass()*flapEff*G_CONST*elCoef*Type()->_elevatorSensitivity*coefElev,0);
    pCenter=Vector3(0,0,-5);
    torque+=pCenter.CrossProduct(pForce);
    Assert(torque.IsFinite());
    #if ARROWS
      // elevator - blue
      AddForce(FutureVisualState().DirectionModelToWorld(pCenter)+wCenter, FutureVisualState().DirectionModelToWorld(pForce)*InvMass(), Color(0,0,1));
    #endif

    // rotate plane when in free fall mode
    if (_isDead)
    {
      torque+=Vector3(1.5,3.2,-2.5).CrossProduct(Vector3(0,GetMass(),0));
      Assert(torque.IsFinite());
    }
    // detect stall
    else if(fabs(FutureVisualState().ModelSpeed().Z())<Type()->_landingSpeed*0.65f && !_landContact)
    {
      float stall = DetectStall();
      torque+=Vector3(1.5,3.2,-2.5).CrossProduct(Vector3(0,GetMass()*stall,0));
      Assert(torque.IsFinite());
    }

    // many aerodynamic forces may be ignored when the plane is not moving    
    if (speed.SquareSize()>1)
    {
      // rudder causes less torque once the plane is moving sideways
      float rudderEff = 1;
      if (speed.X()*FutureVisualState()._rudder>0)
      {
        rudderEff = fabs(speed.Z())*InvSqrt(Square(speed.X())+Square(speed.Z()));
        const float noRudderEff = 0.99619469809f; // cos 5 deg
        rudderEff = InterpolativC(rudderEff,noRudderEff,1,0,1);
      }
      // when moving fast, bank causes torque
      //LogF("bank %.2f, rudder %.2f",bank,_rudder);
      static float rudderCoef = 1.0f;
      float rudderZ = GetRadius()*0.76f;
      //GlobalShowMessage(100,"rz %.2f",rudderZ);
      pForce=Vector3(FutureVisualState()._rudder*GetMass()*flapEff*rudderEff*rudderCoef,0,0);
      // rudder force dependent on size
      pCenter=Vector3(0,-0.25,-rudderZ); // rudder causes some change in bank as well
      torque+=pCenter.CrossProduct(pForce);
      Assert(torque.IsFinite());
      #if ARROWS
        // rudder turning force - cyan
        AddForce(FutureVisualState().DirectionModelToWorld(pCenter)+wCenter, FutureVisualState().DirectionModelToWorld(pForce)*InvMass(), Color(0,1,1));
      #endif

      float stall = DetectStall();
      // there are forces keeping plane aligned with its speed direction
      // one of them is lift
      // here we calculate a "horizontal lift" ("draconic" force)
      float speedInvSize = speed.InvSize();
      // size of the force depends on the angle
      float dragXArea = fabs(speed[0]*speedInvSize);
      float dragYArea = fabs(speed[1]*speedInvSize);
      Vector3 speedNormal = speed*speedInvSize;
      // the force is divided to aside force and drag
      float forceXSize = (speed.SquareSize()*0.005f+speed.Size()*0.02f)*dragXArea;
      float forceYSize = (speed.SquareSize()*0.020f+speed.Size()*0.20f)*dragYArea;
      // divide between aside and front depending on components ratio
      static float draconicForceXCoef = 7.5f;
      static float draconicForceYCoef = 1.0f;
      static float draconicForceZCoef = 1.0f;
      pForce[0] = -speedNormal[0]*forceXSize*draconicForceXCoef;
      pForce[1] = -speedNormal[1]*forceYSize*draconicForceYCoef;
      pForce[2] = stall * -speedNormal[2]*(forceXSize*0.15f+forceYSize*0.05f)*draconicForceZCoef;
      pCenter=VZero;
      pForce *= altCoef*GetMass();
      //torque+=pCenter.CrossProduct(pForce);
      force += pForce;
      #if ARROWS 
        // draconic force - yellow
        AddForce(FutureVisualState().DirectionModelToWorld(pCenter)+wCenter, FutureVisualState().DirectionModelToWorld(pForce)*InvMass(), Color(1,1,0));
      #endif
      // "draconic torque" - apply torque that will align plane so that it flies forward
      static float draconicTorqueXCoef = 0.15f;
      static float draconicTorqueYCoef = 1.0f;
      float torqueFactor = (stall*0.3f + 0.04f);
      pForce[0] = -speed[0]*torqueFactor*GetMass()*draconicTorqueXCoef;
      pForce[1] = -speed[1]*torqueFactor*GetMass()*draconicTorqueYCoef;
      pForce[2] = 0;
      
      const float draconicOffsetZ = -4;
      pCenter = Vector3(0,0,draconicOffsetZ);
      torque+=pCenter.CrossProduct(pForce);
      Assert(torque.IsFinite());
      #if ARROWS 
        // draconic torque - blue
        AddForce(
          FutureVisualState().DirectionModelToWorld(pCenter)+wCenter,
          FutureVisualState().DirectionModelToWorld(pForce)*InvMass(),
          Color(0,0,1)
        );
      #endif
      
      
      static float trimFactor = 0.1f;
      static float minTrim = 0.00f;
      static float maxTrim = 0.10f;

      // "previewing" the aerodynamic stability is quite hard, it would probably require a few iterations,
      // or solving a complicated system of equations
      
      // automatic trim: when there is no elevator input, gradually change the elevator trim so that pitch change is zero
      // avoid negative trim (i.e. when plane is falling, like in turns, or slow speed, do not trim)
      if (QIsManual(EffectivePilotingUnit()) && fabs(FutureVisualState()._elevator)<1e-3f)
      {
        // based on current "draconic torque", trim the aircraft
        // consider: limit to no bank, or no climb only?
        _elevatorTrim -= deltaT * torque[0]*InvMass() * trimFactor;
      }
      // prevent aircraft being trimmed while flying slow
      // this could happen if plane was trimmed in fast level flight, slowed down and continuously used elevator since then
      float trimBySpeed = Interpolativ(speed[2],Type()->_landingSpeed,Type()->_landingSpeed*1.5f,0,1);
      saturate(_elevatorTrim,minTrim,maxTrim*trimBySpeed);
      #if 0 // _PROFILE
        DIAG_MESSAGE(1000,"By speed %.2f, trim %.4f",trimBySpeed,_elevatorTrim);
      #endif
    }

    // TODO: for low angle of attack we want to compute "drag"
    // drag is applied in a direction opposite to the velocity
    // drag coefficient as a function of angle of attack

    // for high angle of attack we compute per-axis friction

    // aerodynamic body friction  
    friction[0]=speed[0]*fabs(speed[0])*0.00100f+speed[0]*0.100f;
    friction[1]=speed[1]*fabs(speed[1])*0.00050f+speed[1]*0.050f;
    float brake = 0;
    if (Type()->_hasAirbrake)
    {
      brake = FutureVisualState()._brake;
    }
    else
    {
      // if we have no air-brake, simulate rotor drag with higher speeds
      float noPowerBrake = Interpolativ(FutureVisualState().ModelSpeed().Z(),0,maxSpeed,0,2);
      brake = noPowerBrake*(1-_thrust*FutureVisualState()._rpm);
    }
    
    float flaps = Type()->_hasFlaps ? FutureVisualState()._flaps : 0.0f;
    float flapsCoef = (1.0+flaps*Type()->_flapsFrictionCoef+fabs(1-gears)*0.5+3*brake);
    friction[2]=speed[2]*fabs(speed[2])*0.00006f+speed[2]*0.006f;
    friction[2] *= flapsCoef;
    // apply all elements
    
    static float xFric = 0.1f;
    static float yFric = 0.1f;
    static float zFric = 1.0f;
    
    friction[0] *= xFric;
    friction[1] *= yFric;
    friction[2] *= zFric;
    
    friction*=GetMass()*altCoef;
    
    #if _ENABLE_CHEATS
      if (CHECK_DIAG(DEForce) && GWorld->CameraOn()==this)
      {
        DIAG_MESSAGE(100,Format("AoA %+4.1f, Vectoring %.2f, stall %.2f",CalculateAOA(speed),FutureVisualState()._thrustVector,DetectStall()));
        DIAG_MESSAGE(100,Format("Velocity %+8.3f,%+8.3f,%+8.3f, size %+8.3f",speed[0],speed[1],speed[2],speed.Size()));
        DIAG_MESSAGE(100,Format("Lift     %+8.3f,%+8.3f,%+8.3f",0.0f,upForce*G_CONST,0.0f));
        DIAG_MESSAGE(100,Format("Thrust   %+8.3f,%+8.3f,%+8.3f",0.0f,thrust*yFactor,thrust*zFactor));
        DIAG_MESSAGE(100,Format("Friction %+8.3f,%+8.3f,%+8.3f, size %+8.3f",friction[0]*GetInvMass(),friction[1]*GetInvMass(),friction[2]*GetInvMass(),friction.Size()*GetInvMass()));
        DIAG_MESSAGE(100,Format("Forces   %+8.3f,%+8.3f,%+8.3f, size %+8.3f",force[0]*GetInvMass(),force[1]*GetInvMass(),force[2]*GetInvMass(),force.Size()*GetInvMass()));
        Vector3 accel = DirectionWorldToModel(FutureVisualState()._acceleration);
        DIAG_MESSAGE(100,Format("Accel    %+8.3f,%+8.3f,%+8.3f, size %+8.3f",accel[0],accel[1],accel[2],accel.Size()));
      }
    #endif

    #if ARROWS
      AddForce(wCenter,FutureVisualState().DirectionModelToWorld(friction)*InvMass(),Color(1,0,0));
    #endif

    // convert forces to world coordinates
    FutureVisualState().DirectionModelToWorld(torque,torque);
    FutureVisualState().DirectionModelToWorld(force,force);
    FutureVisualState().DirectionModelToWorld(friction,friction);

    // angular velocity causes also some angular friction, this should be simulated as torque
    torqueFriction = _angMomentum * 2.0f;
    
    // calculate new position
    VisualState moveTrans = PredictPos<VisualState>(deltaT);

    ApplyRemoteStateAdjustSpeed(deltaT,moveTrans);

    // body air friction
    #if ARROWS
      AddForce(wCenter,-friction*InvMass());
    #endif
    
    // gravity - no torque
    pForce=Vector3(0,-1,0)*(GetMass()*G_CONST);
    force+=pForce;
    #if ARROWS
      AddForce(wCenter,pForce*InvMass());
    #endif


    float totalPressure=0;
    

    // recalculate COM to reflect change of position
    wCenter.SetFastTransform(moveTrans, GetCenterOfMass());

    float soft=0,dust=0;
    if( deltaT>0 )
    {
      bool wasLandContact = _landContact;
      _objectContact=false;
      _landContact=false;
      _waterContact=false;

      Vector3 totForce(VZero);

      // check collision on new position

      float crash = 0;
      float crashVolume = 0;
      if (IsLocal())
      {
        CollisionBuffer collision;
        GLandscape->ObjectCollision(collision,this,moveTrans);
        if( collision.Size()>0  )
        {
          #define MAX_IN 0.4
          #define MAX_IN_FORCE 0.2
          #define MAX_IN_FRICTION 0.4

          _objectContact=true;
          for( int i=0; i<collision.Size(); i++ )
          {
            // info.pos is relative to object
            CollisionInfo &info=collision[i];
            if (info.object)
            {
              info.object->SimulationNeeded(GetVisualStateAge(FutureVisualState()));
              if( info.object->GetMass()>=500 && !info.object->IsPassable())
              {
                Point3 pos=info.pos;
                Vector3 dirOut=info.dirOut;
                // create a force pushing "out" of the collision
                float forceIn=floatMin(info.under,MAX_IN_FORCE);
                Vector3 pForce=dirOut*GetMass()*30*forceIn;
                // apply proportional part of force in place of impact
                pCenter=pos-wCenter;
                totForce+=pForce;
                torque+=pCenter.CrossProduct(pForce);
                Assert(torque.IsFinite());
                
                Vector3Val objSpeed=info.object->ObjectSpeed();
                Vector3 colSpeed=FutureVisualState()._speed-objSpeed;
                // if info.under is bigger than MAX_IN, move out
                if( info.under>MAX_IN )
                {
                  Point3 newPos=moveTrans.Position();
                  float moveOut=info.under-MAX_IN;
                  newPos+=dirOut*moveOut*0.1;
                  moveTrans.SetPosition(newPos);
                }

                float crashP = (colSpeed.SquareSize()-25.0f)*(1.0f/25);
                saturateMax(crash, crashP);
                saturateMax(crashVolume, crashP);

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
                  {
                    FutureVisualState()._speed = FutureVisualState()._speed.Normalized()*oldSize;
                  }
                }

                // second is "land friction" - causing no momentum

                float frictionIn=floatMin(info.under,MAX_IN_FRICTION);
                pForce[0]=fSign(speed[0])*20000;
                pForce[1]=speed[1]*fabs(speed[1])*1000+speed[1]*8000+fSign(speed[1])*10000;
                pForce[2]=speed[2]*fabs(speed[2])*150+speed[2]*250+fSign(speed[2])*2000;

                pForce=FutureVisualState().DirectionModelToWorld(pForce)*GetMass()*(1.0/1000)*frictionIn;
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
        }
      }           


      // optimization - avoid ground collision detection when flying high
      const float ignoreLandCollOverHeight = 20.0f;
      if (height < ignoreLandCollOverHeight)
      {
        GroundCollisionBuffer gCollision;
        // if contact is with gears, land drag is much lower
        bool upsideDown = FutureVisualState().DirectionUp().Y()<0;
        // when angle above certain limit, do not use landcontact unless it was used successfully before (cos 20 deg)
        // also, when gear is up, use geometry for detection collision
        bool enableLandcontact = FutureVisualState().DirectionUp().Y()>0.93969262079f && GetGearPos() < 0.9f;
        bool gearContact = !upsideDown && GetGearPos() < 0.9f;

        if (wasLandContact)
        {
          Assert(Type()->_landContactPlaneValid);
          // compare how much we are aligned with the normal still up direction
          Vector3 up(-dx,1,-dz);
          Vector3 worldUp = FutureVisualState().DirectionModelToWorld(Type()->_landContactPlane.Normal());
          float cosAlpha = worldUp.CosAngle(up);
          enableLandcontact = cosAlpha>0.96592582629; // cos 15 deg
          // if the plane is banked too much or reversed, contact cannot be with gears
          if (FutureVisualState().DirectionUp().Y()<0.2f)
            gearContact = false;
        }
      
        int nContactPoint = GLOB_LAND->GroundCollision(gCollision,this,moveTrans,0.05,1,enableLandcontact);

        if( gCollision.Size()>0 )
        {
          // it is hard to have more than 3 contact points
          saturateMin(nContactPoint,3);
          saturateMax(nContactPoint,gCollision.Size());
          Vector3 gFriction(VZero);
          float maxUnder = 0;
          float maxUnderLand = 0;
#define MAX_UNDER 0.1
#define MAX_UNDER_FORCE 0.1
          const float nPointCoef = 3.0f/nContactPoint;
          for( int i=0; i<gCollision.Size(); i++ )
          {
            // info.pos is world space
            UndergroundInfo &info=gCollision[i];
            if( info.under<0 ) continue;
            float under;
            if( info.type==GroundWater )
            {
              under=floatMin(info.under,2)*0.00025*nPointCoef;
              // calculate water friction
              // similar to aerodynamic body friction, but much stronger
              pForce[0] = speed[0]*fabs(speed[0])*0.0028f+speed[0]*0.28f;
              pForce[1] = speed[1]*fabs(speed[1])*0.0050f+speed[1]*0.50f;
              pForce[2] = speed[2]*fabs(speed[2])*0.0006f+speed[2]*0.20f;

              pForce = FutureVisualState().DirectionModelToWorld(pForce)*GetMass()*nPointCoef;
              pCenter = info.pos-wCenter;
              
              torque -= pCenter.CrossProduct(pForce);
              friction += pForce;

              _waterContact=true;
              // make sure crash is important
              if( FutureVisualState()._speed.SquareSize()>Square(8) )
              {
                float crashP = 0.8f*(FutureVisualState()._speed.SquareSize()/Square(8)-1);
                crash += crashP*Type()->GetArmor();
                crashVolume += crashP;
              }
              if (info.under>2)
              {
                // no sound based on depth, only damage
                crash += (info.under-2)*0.8f*Type()->GetArmor()*nPointCoef;
                // engines cannot operate under the water
                FutureVisualState()._rpm = 0;
                _engineOff = true;
              }
              crash = floatMin(crash,10*Type()->GetArmor());
              if( maxUnder<info.under ) maxUnder=info.under;
            }
            else
            {
              _landContact=true;
              // we consider two forces
              //ReportFloat("land",info.under);
              if( maxUnder<info.under ) maxUnder=info.under;
              if (maxUnderLand<info.under) maxUnderLand=info.under;
              under=floatMin(info.under,MAX_UNDER_FORCE);
              if (CHECK_DIAG(DECollision) || CHECK_DIAG(DEForce))
              {
                static int handle;
                static int uniqueId;
                uniqueId++;
                GDiagsShown.ShowArrow(handle,uniqueId,1.0,"Col",info.pos,VUp,1.0,Color(1,0,0));
              }
            }

            // one is ground "pushing" everything out - causing some momentum
            // dX, dZ is in world space
            Vector3 infoNormal(-info.dX,1,-info.dZ);
            infoNormal.Normalize();
            // with large value here the simulation is numerically unstable with lower fps
            static float outCoef = 80.0;
            pForce = infoNormal*(GetMass()*outCoef*under*nPointCoef);
            pCenter = info.pos-wCenter;
            torque += pCenter.CrossProduct(pForce);
            Assert(torque.IsFinite());
            //torque+=pCenter.CrossProduct(pForce*0.5);
            // to do: analyze ground reaction force
            totForce += pForce;

#if ARROWS
            AddForce(wCenter+pCenter,pForce*InvMass());
#endif

            // surface normal is dirOut
            // ignore vertical speed size
            Vector3 infoNormalM = DirectionWorldToModel(infoNormal);
            // "vertical" part of the speed
            Vector3 vSpeed = infoNormalM*(infoNormalM*speed);
            // "horizontal" part of the speed
            Vector3 sSpeed = speed-vSpeed;
            // second is "wheel and land friction" - causing momentum
            // apply wheel friction
            float pressure=under*10*nPointCoef;
            pForce[0] = fSign(sSpeed[0])*100000*pressure;
            // both Y and Z act as forward now
            pForce[1] = sSpeed[1]*300*pressure+fSign(sSpeed[1])*500*pressure;
            pForce[2] = sSpeed[2]*300*pressure+fSign(sSpeed[2])*500*pressure;

            if( info.texture )
            {
              soft=info.texture->Roughness()*2.5;
              dust=info.texture->Dustness()*2.5;
            }

            // when we are on soft surface, there is more friction
            pForce[0] += pressure*soft*sSpeed[0]*10000;
            pForce[1] += pressure*soft*sSpeed[1]*1000;
            pForce[2] += pressure*soft*sSpeed[2]*1000;

            if (IsDamageDestroyed() || !gearContact)
            {
              // very strong friction for moving damaged plane
              if (fabs(sSpeed[2])>2)
                pForce[2]+=fSign(sSpeed[2])*80000*pressure;
              else
                pForce[2]+=fSign(sSpeed[2])*40000*pressure;
            }
            else if (gears>0.5f)
            { // gear up - we are sliding a plane body, high friction
              pForce[2] += fSign(sSpeed[2])*20000*pressure;
            }
            else
            { // gear down
              // avoid applying full brake in high speed - brake gradually instead
              // increase brake amount between 200 km/h and 90 km/h
              float noBrake = floatMax(maxSpeed*0.2f,130/3.6f);
              float fullBrake = floatMax(maxSpeed*0.1f,80/3.6f);
              float brakeFactor = InterpolativC(fabs(sSpeed[2]),fullBrake,noBrake,1,0);
              // apply wheel brake only when moving slow
              pForce[2] += fSign(sSpeed[2])*20000*pressure*_pilotBrake*brakeFactor;
            }

            totalPressure+=pressure;

            pForce *= (1.0/10000);
            LimitFriction(pForce[0],sSpeed[0]);
            LimitFriction(pForce[1],sSpeed[1]);
            LimitFriction(pForce[2],sSpeed[2]);
            
            pForce *= nPointCoef*GetMass();
            pForce=FutureVisualState().DirectionModelToWorld(pForce);
            friction+=pForce;


#if ARROWS
            AddForce(wCenter+pCenter+Vector3(0,0.2,0),-pForce*InvMass());
#endif
            torque -= pCenter.CrossProduct(pForce)*0.5f; // sub: it is friction
            Assert(torque.IsFinite());

            // vertical friction to avoid jumping - causes no torque?
            //const float vertCoef = 2.4f;
            const float vertCoef = 1.5f;
            pForce[0] = fSign(vSpeed[0])*vertCoef*pressure;
            pForce[1] = fSign(vSpeed[1])*vertCoef*pressure;
            pForce[2] = fSign(vSpeed[2])*vertCoef*pressure;
            pForce = FutureVisualState().DirectionModelToWorld(pForce)*nPointCoef*GetMass();
            friction += pForce;
            torqueFriction+=_angMomentum*0.1*nPointCoef;
          }
          if (_waterContact)
          {
            const SurfaceInfo &info = GLandscape->GetWaterSurface();
            soft = info._roughness * 2.5;
            dust = info._dustness * 2.5;
          }
          
          if (maxUnder > 0)
          {
            // gears down
            // check velocity relative to the surface

            float dX,dZ;
            GLandscape->RoadSurfaceYAboveWater(FutureVisualState().Position(),&dX,&dZ);
            Vector3 normal(-dX,1,-dZ);
            Matrix3 groundOrient(MUpAndDirection,normal,FutureVisualState().Direction());

            /// speed, Y relative to the ground
            Vector3 groundVel = groundOrient.InverseRotation()*FutureVisualState()._speed;
            // check gears
            // with gears we can handle much higher Z speed
            if (gears>0.5 || upsideDown)
            {
              // gears up
              Vector3 crashSpeed = groundVel;
              crashSpeed[0] = DecreaseAbs(crashSpeed[0],2);
              crashSpeed[1] = DecreaseAbs(crashSpeed[1],1);
              crashSpeed[2] = DecreaseAbs(crashSpeed[2],Type()->_landingSpeed*0.7f);
              crashSpeed[2] *= 0.5f;
              crashSpeed[1] *= 3.0f;
              float crashP = crashSpeed.SquareSize()*(1.0f/50);
              saturateMax(crash, crashP*Type()->GetArmor());
              saturateMax(crashVolume, crashP);
            }
            else if (gears>-0.5)
            {
              // gears down
              Vector3 crashSpeed = groundVel;
              crashSpeed[0] = DecreaseAbs(crashSpeed[0],2);
              crashSpeed[1] = DecreaseAbs(crashSpeed[1],3);
              crashSpeed[2] = DecreaseAbs(crashSpeed[2],Type()->_landingSpeed*1.2f);
              crashSpeed[2] *= 0.1f;
              float crashP = crashSpeed.SquareSize()*(1.0f/50);
              saturateMax(crash, crashP*Type()->GetArmor());
              saturateMax(crashVolume, crashP);

              // if there is more than certain limit, gear becomes damaged
              if (crashP>0.2f && IsLocal())
                _gearDamage=true;
            }
            else
            {
              // gears damaged
              Vector3 crashSpeed = groundVel;
              crashSpeed[0] = DecreaseAbs(crashSpeed[0],2);
              crashSpeed[1] = DecreaseAbs(crashSpeed[1],2);
              crashSpeed[2] = DecreaseAbs(crashSpeed[2],Type()->_landingSpeed);
              crashSpeed[1] *= 1.5f;
              crashSpeed[2] *= 0.3f;
              float crashP = crashSpeed.SquareSize()*(1.0f/50);
              saturateMax(crash, crashP*Type()->GetArmor());
              saturateMax(crashVolume, crashP);
            }
            if( maxUnderLand>MAX_UNDER )
            {
              // it is necessary to move object immediately
              Point3 newPos=moveTrans.Position();
              float moveUp=maxUnderLand-MAX_UNDER;
              newPos[1]+=moveUp;
              moveTrans.SetPosition(newPos);

              if (FutureVisualState()._speed.SquareSize()>Square(70))
              {
                FutureVisualState()._speed.Normalize();
                FutureVisualState()._speed *= 70;

              }
              saturateMax(FutureVisualState()._speed[1],-0.1);
            }
          }
        }
      }
      force += totForce;

      if (crash>0)
      {
        if( Glob.time>_disableDamageUntil )
        {
          _disableDamageUntil = Glob.time+0.5;
          // crash boom bang state - impact speed too high
          //LogF("Heli Crash %g",crash);
          _doCrash=CrashLand;
          if( _objectContact ) _doCrash=CrashObject;
          if( _waterContact ) _doCrash=CrashWater;
          _crashVolume=crashVolume;

          CrashDammage(crash);
          const float minCrashExplo = 5;
          if (IsLocal() && crash>minCrashExplo*Type()->GetArmor())
          {
            // set some explosion
            float maxTime = 5-crash*(1/minCrashExplo)*Type()->GetInvArmor();
            saturate(maxTime,0.5,3);
            if (_explosionTime>Glob.time+maxTime)
              _explosionTime=Glob.time+GRandGen.Gauss(0,maxTime*0.3,maxTime);
          }
        }
      }
    }

    bool stopCondition=false;
    if( _pilotBrake>0.5f && _landContact )
    {
      // apply static friction
      if( FutureVisualState()._speed.SquareSize()<0.5f )
      {
        FutureVisualState()._speed=VZero;
        stopCondition=true;
      }
    }

    if( stopCondition) StopDetected();
    else OnMoved();

    // apply all forces
    _lastAngVelocity=_angVelocity; // helper for prediction
    ApplyForces(deltaT,force,torque,friction,torqueFriction);

    if (EnableVisualEffects(prec))
    {
      if (_landContact && FutureVisualState().DirectionUp().Y()>=0.3 && fabs(FutureVisualState().ModelSpeed()[2])>1)
      {
        //_track.Update(RenderVisualState(),deltaT,!_landContact);
        float dSoft=floatMax(dust,0.001)*totalPressure;
        // density depends on mass
        float density=fabs(FutureVisualState().ModelSpeed()[2])*(1.0/10)*dSoft*(2+_pilotBrake*8);
        density *= GetMass()*(1.0/40000);
        if( density>=0.002 )
        {
          Vector3 lPos = Type()->_lDust + 0.1f * FutureVisualState().ModelSpeed();
          Vector3 rPos = Type()->_rDust + 0.1f * FutureVisualState().ModelSpeed();
          Vector3 cSpeed = 0.66f * FutureVisualState()._speed;
          saturate(density,0,1);
          float dustColor=dSoft*8;
          saturate(dustColor,0,1);

          if (density > 0.2f)
          {
            float leftDustValues[] = {density, dustColor, lPos.X(), lPos.Y(), lPos.Z(),
              cSpeed.X(), cSpeed.Y(), cSpeed.Z()};
            _leftDust.Simulate(deltaT, prec, leftDustValues, lenof(leftDustValues));
            float rightDustValues[] = {density, dustColor, rPos.X(), rPos.Y(), rPos.Z(),
              cSpeed.X(), cSpeed.Y(), cSpeed.Z()};
            _rightDust.Simulate(deltaT, prec, rightDustValues, lenof(rightDustValues));
          }
        }
      }

      if (FutureVisualState()._thrustVector>0)
      {
        // effects simulation
        float surfaceY = GLandscape->SurfaceY(FutureVisualState().Position().X(), FutureVisualState().Position().Z());
        bool water = surfaceY < GLandscape->GetSeaLevel();
        float height = FutureVisualState().Position().Y() - (water ? GLandscape->GetSeaLevel() : surfaceY);
        const float maxHeight = 30.0f;
        saturate(height, 0, maxHeight);
        float factor = _thrust * FutureVisualState()._thrustVector * (maxHeight - height) / maxHeight;
        const float invMaxSpeed = 1.0f / (200.0f / 3.6f);
        float alpha = factor - fabs(FutureVisualState().ModelSpeed().Z()) * invMaxSpeed;
        if (alpha > 0.01)
        {
          float speed = 20.0f * factor + 6.0f;
          float effectValues[] = {speed, alpha, 0.0f, 0.0f, 0.0f};
          if (water)
            _waterEffect.Simulate(deltaT, prec, effectValues, lenof(effectValues));
          else
            _dustEffect.Simulate(deltaT, prec, effectValues, lenof(effectValues));
        }
      }

      float damage = GetTotalDamage();
      if (EngineIsOn() && damage >= 0.4)
      {
        float damageValues[] = {damage, FutureVisualState()._rpm, 0.0f, 0.0f, 0.0f};
        _damageEffect.Simulate(deltaT, prec, damageValues, lenof(damageValues));
      }
    }


    if( _isDead && (_landContact || _objectContact) )
    {
      NeverDestroy();
      IExplosion *smoke = GetSmoke();
      if (smoke) smoke->Explode();
    }

    StabilizeTurrets(FutureVisualState().Transform().Orientation(),moveTrans.Orientation());

    Move(moveTrans); // finally apply move
    DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
  } // if (!CheckPredictionFrozen())

  // machine gun light and clouds
  if (EnableVisualEffects(prec))
  {
    if (_weaponsState._mGunClouds.Active() || _weaponsState._mGunFire.Active())
    {
      Matrix4Val toWorld = FutureVisualState().Transform();
      Vector3Val dir = toWorld.Direction();
      Vector3 gunPos(VFastTransform,toWorld,Type()->_gunPos);
      _weaponsState._mGunClouds.Simulate(gunPos,FutureVisualState().Speed()*0.7+dir*5.0,0.35,deltaT);
      _weaponsState._mGunFire.Simulate(gunPos,deltaT);
    }
  }

  base::Simulate(deltaT,prec);
  SimulatePost(deltaT,prec);
}

void AirplaneAuto::SetTotalDamageHandleDead(float damage, bool setHitpoints)
{
  base::SetTotalDamageHandleDead(damage,setHitpoints);
  // gear damage is not done using hit points - force fixing it here
  if (damage<=0)
    _gearDamage = false;
}

void AirplaneAuto::Repair(float amount)
{
  base::Repair(amount);
  // one the plane is reasonably healthy, repair the gear as well
  if (GetTotalDamage()<0.25f)
    _gearDamage = false;
}

bool Airplane::IsContinuous(CameraType camType ) const
{
  return camType==CamGunner || camType==CamGroup;
}

bool Airplane::Airborne() const {return !_landContact;}

const float WindEmitRadius = 40;

// TODO: small planes and helis make less wind than big ones

bool Airplane::EmittingWind(Vector3Par pos, float radius) const
{
  float dist2 = pos.Distance2(RenderVisualState().Position());
  return dist2<Square(radius+WindEmitRadius) && RenderVisualState()._rpm>0.01f;
}

Vector3 Airplane::WindEmit(Vector3Par pos, Vector3Par wind) const
{
  VisualState const& vs = RenderVisualState();
  // calculate how much can we effect the wind at given point
  // assume the rotor is quite high above the model center

  // based on vectoring plane may be emitting wind behind or below
  // assume center is the source of the wind

  //Vector3 relPos = vs.PositionWorldToModel(pos)-Vector3(0,2,0);
  Vector3 relPos = vs.PositionWorldToModel(pos);

  if (relPos.SquareSize()>Square(WindEmitRadius)) return wind;
  
  //if (_thrustVector>0)
  // rotate based on vectoring
  float thrustAngle = -H_PI/2+floatMax(vs._thrustVector,0)*(H_PI/2);
  
  Vector3 relPosVectored = Matrix3(MRotationX,thrustAngle) * relPos;

  // wind is not affecting on the sides or above
  if (relPosVectored.Y()>-0.01f) return wind;
  
  float dist = relPos.Size();
  float vDist = relPosVectored.Y()*0.05f;
  saturate(vDist,-1,+1);
  
  // effect depends on how much the rotor is rotating
  float strength = Square(vs._rpm)*(1-fabs(vDist))*40*(1-dist/WindEmitRadius);
  // it depends also on rotor blades position
  float bladeCoef = _thrust*2;
  saturate(bladeCoef,0.2f,1.5f);
  
  strength *= bladeCoef;

  // the more outside, the more we will turn the direction outside
  Vector3 outDir = relPos;
  outDir.Normalize();
  
  // reaching end of the influence area, the effect needs to be weaker
  
  // the more we are under, the more the wind is going outside
  float aboveSign = -relPosVectored.Y()*0.5;
  saturate(aboveSign,0,+1);
  // TODO: check the relation to the surface as well - surfaces acts as a deflector
  return wind + vs.DirectionModelToWorld(outDir)*(strength*aboveSign);
}

const char **Airplane::HUDMode(int &mask) const
{
  mask = 0;
  if (!EngineIsOn()) return NULL;
  static const char *conditions[]=
  {
    "on","ils","mgun","missile","aamissile","atmissile","rocket","bomb","flaps",NULL
  };
  mask |= 1<<0; // on is always on
  if (_pilotGear) mask |= 1<<1; // ILS is on when gear is down

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
        if (ammo->_simulation == AmmoShotBullet || ammo->_simulation==AmmoShotBullet )
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
  if (_pilotFlaps>=1) mask |= 1<<8; //flaps
  return conditions;
}

void AirplaneAuto::EngineOn()
{
  base::EngineOn();
}
void AirplaneAuto::EngineOff()
{
  _thrustWanted=0;
  base::EngineOff();
}

/*!
\patch 5129 Date 2/19/2007 by Ondra
- Fixed: Airplane landing light may be now off when gear is up.
*/
bool AirplaneAuto::IsLightOn(ObjectVisualState const& vs) const
{
  if (Type()->_lightOnGear && vs.cast<AirplaneAuto>()._gearsUp>0.01f) return false;
  return base::IsLightOn(vs);
}

bool AirplaneAuto::LightsNeeded() const
{
  if (QIsManual(EffectivePilotingUnit())) return false;
  // lights always on while taxiing or landing, even when in combat
  // TODO: consider not using lights in combat / stealth
  return _planeState<Flight || _planeState>=Landing;
}

float Airplane::GetEngineVol( float &freq ) const
{
  freq=_rndFrequency*(_thrust+0.5)*FutureVisualState()._rpm;
  return (_thrust+0.5);
}

float Airplane::GetEnvironVol( float &freq ) const
{
  freq=1;
  return FutureVisualState()._speed.Size()/Type()->GetMaxSpeedMs();
}

void Airplane::Sound( bool inside, float deltaT )
{
  PROFILE_SCOPE_EX(snAir,sound);
  if( _servoVol>0.001 )
  {
    const VisualState &vs = RenderVisualState();
    const SoundPars &pars=Type()->GetServoSound();
    if( !_servoSound )
    {
      _servoSound=GSoundScene->OpenAndPlay(
        pars.name,this,true,vs.Position(),vs.Speed(), pars.vol, pars.freq, pars.distance
      );
    }
    if( _servoSound )
    {
      float vol=pars.vol*_servoVol;
      float freq=pars.freq;
      _servoSound->SetVolume(vol,freq); // volume, frequency
      _servoSound->Set3D(!inside);
      _servoSound->SetPosition(vs.Position(),vs.Speed());
    }
  }
  else
  {
    _servoSound.Free();
  }

  base::Sound(inside,deltaT);
}

void Airplane::UnloadSound()
{
  base::UnloadSound();
}

AnimationStyle Airplane::IsAnimated( int level ) const {return AnimatedGeometry;}

void Airplane::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  if (_rotorSpeed < 1.0)
  {
    Type()->_rotorMove.Hide(animContext, _shape, level);
  }
  else
  {
    Type()->_rotorStill.Hide(animContext, _shape, level);
  }

  if (_weaponsState.ShowFire())
  {
    Type()->_weapons._animFire.Unhide(animContext, _shape, level);
    Type()->_weapons._animFire.SetPhase(animContext, _shape, level, _weaponsState._mGunFirePhase);
  }
  else
  {
    Type()->_weapons._animFire.Hide(animContext, _shape, level);
  }

  float oldGears = animContext.GetVisualState().cast<Airplane>().GetGearPos(Type());
  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);
  float gears = animContext.GetVisualState().cast<Airplane>().GetGearPos(Type());

  const float tholdUp = 0.95f;
  const float tholdDown = 0.05f;
  if (gears >= tholdUp && oldGears < tholdUp) OnEvent(EEGear, false);
  if (gears <= tholdDown && oldGears > tholdDown) OnEvent(EEGear, true);
}

void Airplane::Deanimate( int level, bool setEngineStuff )
{
  base::Deanimate(level,setEngineStuff);
}


// basic autopilot

AirplaneAuto::AirplaneAuto( const EntityAIType *name, Person *pilot, bool fullCreate)
:Airplane(name,pilot, fullCreate),_pilotHeading(0),_pilotSpeed(0),_pilotHeight(0),_defPilotHeight(100),
_pilotVertSpeed(0),_pilotSpeedX(0),

_pilotAvoidHigh(TIME_MIN),
_pilotAvoidHighHeight(0),
_pilotAvoidLow(TIME_MIN),
_pilotAvoidLowHeight(100000),
_analogueThrottleTime(TIME_MIN),
_thrustHelperTime(TIME_MIN),
_lastForward(FLT_MAX),
_lastThrotleDirect(FLT_MAX),
_dirCompensate(0.5),
_pressedForward(false),_pressedBack(false),
_pilotBank(0),
_pilotHelperHeight(true),_pilotHelperDir(true),
_pilotHelperThrust(true),_pilotHelperBankDive(true),
_forceDive(1),
_homeAirport(-1),_landAtAirport(-1),
_planeState(TaxiIn) // get ready for take-off
{
}

inline float AirplaneType::MarshallDist() const
{
  // Camel: 90 km/h -> 2000
  // Su34: 250 km/h -> 3000
  return _landingSpeed*23+1400;
}

inline float AirplaneType::ApproachDist() const
{
  // Camel: 90 km/h -> 1400
  // Su34: 250 km/h -> 2000
  return _landingSpeed*15+1000;
}

inline float AirplaneType::FinalDist() const
{
  // Camel: 90 km/h -> 1000
  // Camel: 250 km/h -> 1000
  // always 1000 - we need to capture the glide slope
  return 1000;
}


/// compute thrust vector so that thrust is a vertical as possible for given orientation

static float NeutralThrustVector(bool vtol, Matrix3Par orient)
{
  // in VTOL mode it is the responsibility of the pilot to orient as needed
  if (vtol) return 0;
  
  if (orient.DirectionUp().Y()<=0)
  {
    // 
    return -1;
  }
  return asin(orient.Direction().Y())*(-1.0f/(H_PI/2));
}

float AirplaneAuto::EstimateAboveTerrain(float estT) const
{
  // scan surface before you
  float minHeight = FLT_MAX;

  Vector3 tPos = FutureVisualState().Position();
  const int nSamples = 4;
  Vector3 step = FutureVisualState()._speed*estT*(1.0f/nSamples);
  for (int s=nSamples; --s>=0;)
  {
    tPos += step; // current state is not interesting - too late to correct it anyway
    const float surfY = GLOB_LAND->SurfaceYAboveWaterNoWaves(tPos.X(),tPos.Z());
    saturateMin(minHeight,tPos.Y()-surfY); 
    
  }
  return minHeight;
}

/*!
\patch 1.16 Date 8/13/2001 by Ondra.
- Fixed: Landing autopilot now work for all planes both on Everon and Malden.
\patch 1.33 Date 11/29/2001 by Ondra.
- Fixed: Dead AI pilot was sometimes able to continue flying.
\patch 1.42 Date 1/10/2002 by Ondra
- Fixed: Cessna AI pilot swinging up and down when flying slow.
\patch 1.43 Date 1/25/2002 by Ondra
- Fixed: Cessna flight model and AI pilot improved.
(Thanks to WKK Gimbal for a suggestion how to fix it.)
\patch 1.44 Date 2/12/2002 by Ondra
- Fixed: Airplane turned engine off on bumpy surface during takeoff,
making takeoff with joystick impossible outside the airport.
\patch 5128 Date 2/13/2007 by Ondra
- Fixed: Improved airplane landing autopilot.
\patch 5128 Date 2/13/2007 by Ondra
- Fixed: STOVL ability added (use autohover action to toggle VTOL mode).
\patch 5149 Date 3/27/2007 by Ondra
- Fixed: AI planes unable to land on Sahrani airport.
\patch 5161 Date 5/25/2007 by Ondra
- Fixed: Increased rudder authority for airplanes, improved AI using rudder while engaging.
*/

void AirplaneAuto::Simulate( float deltaT, SimulationImportance prec )
{
  // switch to wreck simulation and drawing if needed
  if (!_isWreck)
  {
    _isWreck = GetDestructType() == DestructWreck && _isDead && _landContact;
  }

  VisualState &vs = FutureVisualState();
  if (_isWreck)
  {
    // switch off the engine
    EngineOff();
    _pilotBrake = 1;
    
    _thrust -= 2.0f*deltaT;
    vs._rpm -= 2.0f*deltaT;
    saturateMax(_thrust,0);
    saturateMax(vs._rpm,0);

    SimulateWreck(deltaT, prec);
    return;
  }

  // autopilot: convert simple _pilot control
  // to advanced simulation model
  if (!_isDead && IsLocal())
  {
    if ((PilotUnit() && PilotUnit()->LSIsAlive()) || (CopilotUnit() && CopilotUnit()->LSIsAlive()))
    {
      // change airplane state when landed
      if( _planeState==Flight && _landContact && vs.DirectionUp().Y()>0.5 )
      {
        if (!QIsManual(EffectivePilotingUnit()))
        {
          _planeState=TaxiOff;
        }
        else
        {
          _planeState=Takeoff;
        }
        // touched the ground - force setting home airport
        int airportId = GWorld->CheckAirport(vs.Position(),vs.Direction());
        // and call the event handler for the airport
        OnEvent(EELandedTouchDown,float(airportId));
        // cancel any landing requests
        _landAtAirport = -1;
      }
    
      float surfaceY=GLOB_LAND->SurfaceYAboveWaterNoWaves(vs.Position().X(),vs.Position().Z());

      //if( _pilotHelper )
      {
        bool setControls = _pilotHelperBankDive;
        float diveWanted=0;
        float bankWanted = 0;
        bool diveWantedSet=false;

        // check whether pilot need to move somewhere
        bool wantToMove = false;
        AIBrain *unit = CopilotUnit() ? CopilotUnit() : PilotUnit();

        const Path &path = unit->GetPath();
        wantToMove = path.Size()>=2 && path.GetSearchTime()<Glob.time-2;

        // get simple approximations of bank and dive
        // we must consider current angular velocity
        Matrix3Val orientation=vs.Orientation();

        // when aiming at the target, we need to change orientation quickly
        const float dirEstT = _sweepTarget ? _dirCompensate*0.3f : _dirCompensate;
        Matrix3Val derOrientation=_angVelocity.Tilda()*orientation;
        Matrix3 estOrientation=orientation+derOrientation*dirEstT;
        estOrientation.Orthogonalize();
        
        float bank=estOrientation.DirectionAside().Y();
        float dive=estOrientation.Direction().Y();
        if (estOrientation.DirectionUp().Y()<0)
          bank = fSign(bank); // plane upside down, pretend full bank in given direction
        float landingSpeed = Type()->_landingSpeed;
        float stallSpeed = Type()->_stallSpeed;
        static float shortTakeoffVector = 0.7f;
        float aoa = CalculateAOA(vs.ModelSpeed());
        float height=vs.Position().Y()-surfaceY;
        //if (_pilotHelperThrust)
        {
          const float est_delta = 2.0f;
          // estimate vertical acceleration
          float relSpeed=vs.ModelSpeed().Z();
          float speedWanted=_pilotSpeed;
          // use acceleration to estimate change of position
          Vector3 accel=vs.DirectionWorldToModel(vs._acceleration);
          
          // the faster we fly, the less we want the vectoring to prevent us from descending
          // otherwise landing is impossible
          
          // 0.26 means 15 degree AoA
          // in VTOL (autohover) mode we always want to have a vertical speed zero
          float desiredSpeedY = 0;
          if (Type()->_vtol && !_vtolMode) // AoA control of the VTOL aircraft in non-VTOL mode
          {
            if (QIsManual(EffectivePilotingUnit()))
            {
              // simple heuristics: when player is moving at the landing speed, he is landing
              if (relSpeed<landingSpeed)
              {
                // when moving very slow, do not assume landing - when landing, player would use VTOL mode
                desiredSpeedY = InterpolativC(relSpeed,landingSpeed*0.5,landingSpeed,0,-relSpeed*0.22f);
              }
              else
              {
                desiredSpeedY = InterpolativC(relSpeed,landingSpeed,landingSpeed*1.5,-relSpeed*0.22f,0);
              }
              // to make takeoff easier, when grounded, always assume only very slow dive speed
              if (_landContact) 
              {
                saturateMax(desiredSpeedY,-relSpeed*0.05f);
              }
            }
            else if (_planeState==Landing || _planeState==Touchdown) // unless AI is landing, maintain AoA 0
            {
              desiredSpeedY = InterpolativC(relSpeed,stallSpeed*0.5,stallSpeed,0,-relSpeed*0.22f);
            }
          }
          
          float changeAccel = (_pilotSpeed-relSpeed)*(1/est_delta)-accel[2];
          float changeAccelX = (_pilotSpeedX-vs.ModelSpeed().X())*(1/est_delta)-accel[0];
          
          if (!QIsManual(EffectivePilotingUnit()) && (_planeState==TaxiIn || _planeState==Takeoff) && Type()->_vtol>=3)
          {
            if (!wantToMove)
            {
              // try to keep some height over landscape
              const float height = 2.0f;
              float diff = vs.Position().Y() - (surfaceY + height);
              _pilotVertSpeed = -diff * 0.3f;
              saturate(_pilotVertSpeed, -2, +2);
            }
            else _pilotVertSpeed = 3; // slow climb seems appropriate

            _pilotSpeedX = 0;
            // we do not care much about acceleration now, we are climbing for takeoff, not landing
            float ignoreAccel = 1.0f;
            changeAccelX = TowardZero(changeAccelX, ignoreAccel);
            changeAccel = TowardZero(changeAccel, ignoreAccel);
            if (vs.Position().Y()-surfaceY>40)
            {
              // once we are high enough, we may start accelerating
              _pilotSpeed = Type()->_landingSpeed*1.2f;
              if (vs.ModelSpeed().Z()>Type()->_landingSpeed)
              {
                _planeState = Flight;
              }
            }
            // start rotating in the direction of expected movement
          }

          float changeAccelY = (_pilotVertSpeed+desiredSpeedY-vs.ModelSpeed().Y())*(1/est_delta)-accel[1];

          if (!QIsManual(EffectivePilotingUnit()) && _vtolMode)
          {
            // control bank and dive so that we reach desired speed in both X and Z direction
            bankWanted = bank - changeAccelX*0.1f;
            // we want to avoid banking more than 30 degree in VTOL mode
            saturate(bankWanted,-0.5f,+0.5f);
            // unless we really need to climb a lot, we want to avoid nose up too much
            // positive dive is up, positive z is forward
            diveWanted = dive - changeAccel*0.1f;
            float above = vs.Position().Y()-surfaceY-_pilotHeight;
            float maxDive = InterpolativC(above,0,10,0.5f,0);
            saturateMin(diveWanted,maxDive);
            diveWantedSet = true;
          } 

          if (QIsManual(EffectivePilotingUnit()) && Type()->_vtol==2 && _vtolMode && _pilotVertSpeed>0 && (_landContact || vs.Position().Y()-surfaceY<GetRadius()))
          {
            // the engine has not enough power to climb
            // when user pressed Up, he must mean something else - probably forward
            // vector once we want to be airborne
            if (_pilotHelperThrust)
            {
              _thrustVectorWanted = shortTakeoffVector+NeutralThrustVector(_vtolMode,vs.Orientation());
              // the only thing we can do is to apply full power and wait
              _thrustWanted = 1.0;
              _pilotBrake = 0;
            }
          }
          else if (Type()->_vtol>=3 && (!_vtolMode || !_pilotHelperThrust)  && QIsManual(EffectivePilotingUnit()) && (_landContact || vs.Position().Y()-surfaceY<GetRadius()))
          {
            // manual STOL taxiing or near ground control of strictly VTOL aircraft (like Osprey)
            float neutral = NeutralThrustVector(_vtolMode,vs.Orientation());
            if (_pilotHelperThrust)
            {
            _pilotBrake = floatMax(-changeAccelY*0.1f,0); // apply brakes to slow down
            
            // prevent takeoff until speed is high enough
            if (_landContact)
            {
              // if we do not need to change speed, do not change the vectoring, this keeps rotor position stable unless needed
              if (_pilotSpeed-relSpeed>2)
              {
                _thrustVectorWanted = 0.8+neutral;
              }
              // otherwise do not vector, control speed only using thrust
              if (fabs(_pilotSpeed)<0.1)
              {
                // special case - breaking on the ground
                _thrustWanted = 0;
              }
              else
              {
                _thrustWanted = _thrust + changeAccel*0.1f;
                // for this we limit thrust
                // as we are moving faster, gradually allow full thrust for takeoff
                float thrustLimit = Interpolativ(fabs(relSpeed),stallSpeed*0.3f,stallSpeed*0.7f,0.6f,1.0f);
                saturate(_thrustWanted,0,thrustLimit);
                // handle a special case - moving fast and accelerating: must be a takeoff
                if (relSpeed>landingSpeed && _pilotSpeed-relSpeed>2)
                {
                  _thrustWanted = 1;
                }
              }
            }
            else
            {
              // vector as needed to accelerate
              _thrustVectorWanted = 1.0-changeAccel*0.1f + neutral;
              // while airborne, apply enough thrust to keep altitude
              _thrustWanted = _thrust + changeAccelY*0.1f;
            }
            }
            saturate(_thrustVectorWanted,0.8f+neutral,1.0f+neutral);
          }
          else if (
            Type()->_vtol && _vtolMode ||
            // strict VTOL (like Osprey) - until airborne, force vectoring
            Type()->_vtol>=3 && (_landContact || vs.Position().Y()-surfaceY<GetRadius())
          )
          {
            // hovering mode - full vectoring, thrust controls climb/dive
            if (_pilotHelperThrust && EngineIsOn())
            {
              _thrustVectorWanted = 1  + NeutralThrustVector(_vtolMode,vs.Orientation());
              // we always apply air-brake to maintain as low speed as possible while hovering
              _pilotBrake = 1;
              if (_landContact)
              {
                // standing still - no thrust needed at all
                _thrustWanted = 0;
              }
              else
              {
                // when we want to climb, on the ground we can start by applying full thrust
                // once there is vertical speed, _landContact will switch to false and normal handling will take place
                _thrustWanted = 1.0;
              }
            }
            else
            {
              // in the air - control thrust as needed
              _thrustWanted = _thrust + changeAccelY*0.1f;
            }            
          }
          else if (_pilotHelperThrust)
          {
            // hysteresis when switching vectoring on off
            // this was implemented because ArmA 2 F35B vectoring required opening many animated parts
            // we do not want them to open/close repeatedly
            float vtolOnLimit = Type()->_stallSpeed*2.0f;
            float vtolOffLimit = Type()->_stallSpeed*2.2f;
            if (
              Type()->_vtol && !_landContact && _planeState!=AfterTouchdown &&
              (relSpeed<vtolOnLimit || relSpeed<vtolOffLimit && vs._thrustVector>0)
            )
            {
              //float slowVector = InterpolativC(fabs(relSpeed),Type()->_stallSpeed*0.75f,Type()->_stallSpeed*1.5f,1,0);
              // normal flight, however getting close to stall
              // vectoring may be needed
              float neutral = NeutralThrustVector(_vtolMode,vs.Orientation());
              float minVectorNeeded = InterpolativC(fabs(relSpeed),stallSpeed,landingSpeed,0.8f,0.0f)+neutral;

              // automatic VTOL flight - vectoring controls velocity
              // power controls vertical speed

              // we want:
              // y-speed = zero (climb/dive controlled only by plane dive)
              // control forward acceleration by applying air-brake / forward thrust

              
              float thrustAngle = floatMax(vs._thrustVector,0)*(H_PI/2);
              float zFactor = cos(thrustAngle);
              float yFactor = sin(thrustAngle);

              // adjust horizontal / vertical thrust
              float horiThrustWanted = changeAccel*0.1f+_thrust*zFactor;
              float vertThrustWanted = changeAccelY*0.1f+_thrust*yFactor;

              // values outside of 0..1 are impossible anyway
              saturate(horiThrustWanted,0,1.0f);
              // however, there is no need to keep y strictly 1
              // - 0.99 will allow 8 degree vectoring, while thrust reduction is negligible
              saturate(vertThrustWanted,0,0.99f);

              float size2 = Square(horiThrustWanted)+Square(vertThrustWanted);
              if (size2==0)
              {
                // special case - keep vectoring as it is, only reduce thrust
                _thrustWanted = 0;
              }
              else
              {
                if (size2>1)
                {
                  // when solution lies outside of the possible envelope, clamp it
                  // while clamping, strongly prefer y (keeping altitude is primary)
                  float vectorAngle = asin(vertThrustWanted);
                  _thrustVectorWanted = vectorAngle*(1.0f/(H_PI/2))+neutral;
                  _thrustWanted = 1;
                }
                else if (horiThrustWanted==0)
                {
                  _thrustVectorWanted = 1+neutral;
                  _thrustWanted = vertThrustWanted;
                }
                else
                {
                  float vectorAngle = atan(vertThrustWanted/horiThrustWanted);
                  _thrustVectorWanted = vectorAngle*(1.0f/(H_PI/2))+neutral;
                  _thrustWanted = sqrt(size2);
                }
              }

              // control brake as well
              // vectoring based on speed (low speed requires high vectoring)
              if (relSpeed-speedWanted>5 )
              { // slowing down
                float slowDown = (relSpeed-speedWanted)*0.2f;
                saturate(slowDown,0,1);
                _pilotBrake = slowDown;
              }
              else if (fabs(speedWanted)<2)
              {
                float slowDown = (relSpeed-speedWanted)*0.2f;
                saturate(slowDown,0,1);
                _pilotBrake=1;
              }
              else
              {
                _pilotBrake=0;
              }

              if (_thrustVectorWanted<minVectorNeeded) _thrustVectorWanted = minVectorNeeded;
              
            }
            else
            { // normal flight - power controls velocity
              if (QIsManual(EffectivePilotingUnit()) || _planeState!=Takeoff && _planeState!=AfterTouchdown && _planeState!=Touchdown)
              {
              _thrustVectorWanted = 0;
              }

              if (fabs(speedWanted)<2)
              {
                _thrustWanted=0;
                _pilotBrake=1;
              }
              else if (relSpeed-speedWanted>5 )
              {
                _thrustWanted=0;
                _pilotBrake = (relSpeed-speedWanted)*0.2f;
                saturate(_pilotBrake,0,1);
              }
              else
              {
                _thrustWanted=changeAccel*0.1f+_thrust;
                _pilotBrake=0;
              }
            }
          }
        else
        {
            _thrustVectorWanted = 0;
          }
        }

        //float lastBank=orientation.DirectionAside().Y();
        Vector3Val estDirection=estOrientation.Direction().Normalized();
        
        // we are interested not only in plane attitude (heading), but also in velocity (flight vector)
        // otherwise rudder will cause heading change, but real flight direction will never catch up

        float curVector=atan2(vs._speed[0], vs._speed[2]);
        float curHeading=atan2(estDirection[0],estDirection[2]);
        static float vectorFactor = 0.6f;
        float usedHeading = curHeading + AngleDifference(curVector,curHeading)*vectorFactor;
        // with rudder we want to change the actual heading
        // note: if someone changes changeHeading, he has to change changeHeadingRudder as well
        float changeHeadingRudder = AngleDifference(_pilotHeading,curHeading);
        float changeHeading = AngleDifference(_pilotHeading,usedHeading);
        #if 0
          float curRHeading=atan2(Direction()[0],Direction()[2]);
          LogF("  est chh %.2f",changeHeading);
          LogF("  raw chh %.2f",AngleDifference(_pilotHeading,curRHeading));
        #endif

        const float estT0 = 2.0f;
        const float accelFactor = 0.25f;
        Vector3 estPos0 = vs.Position()+vs._speed*estT0+vs._acceleration*Square(estT0)*accelFactor;
        float estPos0Y = GLOB_LAND->SurfaceYAboveWaterNoWaves(estPos0.X(),estPos0.Z());
        float estHeight = estPos0.Y()-estPos0Y;
        // dive needed to keep with the landscape slope
        float estHeightMin = EstimateAboveTerrain(2.0f);

        //GlobalShowMessage(100,"max %.0f, y %.0f",maxSurfaceY,estSurfY);


        //float minHeight=ModelSpeed()[2];
        float minHeight=vs.ModelSpeed()[2]*0.5f+10;
        saturateMin(minHeight,20);
        saturateMax(_pilotHeight,minHeight);

        // when we see altitude would become too low, trigger collision avoidance
        if (estHeightMin<_pilotHeight && estHeightMin<estHeight)
        {
          // following is a continous alternative of a simple, but discreete estHeight = estHeightMin;
          float minFlyHeight = Interpolativ(estHeightMin,_pilotHeight-10,_pilotHeight,estHeightMin,estHeight);
          if (estHeight<minFlyHeight) estHeight = minFlyHeight;
        }
    

        // turn using bank
        bool elevatorSet=false;
        if (_planeState==TaxiIn && Type()->_vtol>=3)
        {
          if (wantToMove)
          {
            // we need to take off first, once we are done, we can start flying
            _thrustVectorWanted = 1;
            _thrustWanted = 1;
          }
          // we want to keep level until we gain enough altitude
          diveWanted = 0;
          diveWantedSet = true;
          // 
          _pilotBank = 0;
          if (vs.Position().Y()-surfaceY>40)
          {
            // once we are high enough, we may start accelerating
            _planeState = Flight;
          }
        }
        else if (_planeState==AutoLanding)
        {
          // landing assistance
          // select most suitable airstrip based on current position
          Vector3 ilsPos = vs.Position(), ilsDir = vs.Direction();
          bool ilsSTOL = false;
          GWorld->GetILS(ilsPos,ilsDir,ilsSTOL,TSideUnknown,Type()->_vtol>0);
          // automatically control flaps and gear
          AutoFlaps(1.75f);
          AutoGear(true);
          // perform landing autopilot
          AirplaneState state = LandingAutopilot(ilsPos,ilsDir,ilsSTOL,changeHeading,curHeading,diveWanted,diveWantedSet,setControls);
          if (state==AfterTouchdown)
          {
            Vector3 ilsCPos=ilsPos-ilsDir*700;
            Vector3 relPos=ilsCPos-vs.Position();
            _pilotHeading=atan2(relPos.X(),relPos.Z());
            changeHeading=AngleDifference(_pilotHeading,curHeading);
            Limit(changeHeading,-0.2,+0.2);
            _thrustWanted=0;
            _pilotSpeed=0;
            _pilotBrake=1;
            diveWanted=0;
            diveWantedSet=true;

            _planeState = Takeoff;
          }
          else if (!CheckLandingAvailable(false))
          {
            if (_landContact) _planeState = Takeoff;
            else _planeState = Flight;
          }
          changeHeadingRudder = changeHeading;
        }
        else if (_planeState!=Flight)
        {
          AutomaticLanding(changeHeading, curHeading, changeHeadingRudder, landingSpeed, setControls, diveWanted, diveWantedSet, elevatorSet);
        }

        saturateMin(_pilotSpeed,GetType()->GetMaxSpeedMs()*1.5f);

        float useRudder = 0;
        if (_pilotHelperDir)
        {
          // once we are in VTOL mode and moving very slow, we need to change direction using rudder only
          // bank controls side speed instead
          if (_vtolMode)
          {
            _rudderWanted = changeHeading*20;
            saturate(_rudderWanted,-1,+1);
          }
          else
          {
            float bankFactor = Interpolativ(vs.ModelSpeed().Z(),landingSpeed,landingSpeed*2,0.33f,1.0f);
            const float minBankAngle = 0.01f;
            const float maxBankAngle = 0.025f;
            if (
              QIsManual(EffectivePilotingUnit()) || fabs(changeHeading)>maxBankAngle ||
              // if sweeping and heading change is very small, all we need is to apply some rudder
              // we should not do this when sweep target is still too far (more than 15 seconds of flight)
              _sweepTarget.IsNull() || _sweepTarget->GetPosition().Distance2(vs.Position())>Type()->_landingSpeed*15
            )
            {
              bankWanted = -changeHeading*4*bankFactor;
            }
            else
            {
              float factorRudder = InterpolativC(fabs(changeHeading),minBankAngle,maxBankAngle,0,1);
              bankWanted = -changeHeading*4*bankFactor*factorRudder;
              useRudder = 1-factorRudder;
            }
          }
        }
        else
        {
          bankWanted = _pilotBank;
        }
        if (_pilotHelperHeight)
        {
          if (vs.ModelSpeed().Z()<stallSpeed*2)
          {
            float fast = (vs.ModelSpeed().Z()-stallSpeed)/stallSpeed;
            saturate(fast,0,1);
            float maxBank = fast*0.55f+0.4f;
            saturate(bankWanted,-maxBank,+maxBank);
          }
        }
        saturate(bankWanted,-0.95f,0.95f);


        // no ailerons until full takeoff
        float maxAilerons = 1;

        if( _planeState==Takeoff )
        {
          // special handling of take off condition
          // leave flaps setting as it is
          if (!_pilotHelperDir)
          {
            if (!_landContact)
            {
              if (!QIsManual(EffectivePilotingUnit()) || height>=2)
              {
                _planeState = Flight;
              }
            }
          }
          if (!QIsManual(EffectivePilotingUnit()))
          {
            const World::AirportInfo *airport = GWorld->GetAirport(vs.Position(),vs.Direction(),TSideUnknown,Type()->_vtol>0);
            static float beforeEnd = 200.0f;
            float toEnd = (airport->_runwayEnd-vs.Position())*-airport->_ilsDir;

            const float takeOffSpeed = Type()->_takeOffSpeed;
            float actDive = vs.Direction().Y();
            if( vs.ModelSpeed()[2]>=takeOffSpeed || _thrustVectorWanted>0.5f && vs.ModelSpeed()[2]>=takeOffSpeed*0.3f || toEnd<beforeEnd)
            {
              float diveWanted=10*H_PI/180;
              if (!_landContact) diveWanted=13*H_PI/180;
              elevatorSet=true;
              _elevatorWanted=(actDive-diveWanted)*10;
              if (Type()->_hasFlaps) _pilotFlaps = 1;
              if (!_landContact && height>=10)
              {
                maxAilerons = 0.3f;
              }
              else
              {
                maxAilerons = 0;
                if (toEnd<beforeEnd && Type()->_hasFlaps) _pilotFlaps = 2;
              }
              // use flaps for takeoff
            }
            else if (vs.ModelSpeed()[2]>=takeOffSpeed*0.4f)
            {
              float diveWanted=0*H_PI/180;
              if (vs.ModelSpeed()[2]>=takeOffSpeed*0.7f)
              {
                diveWanted=7*H_PI/180;
                // starting to rotate - extend the flaps
                if (Type()->_hasFlaps) _pilotFlaps = 1;
              }
              elevatorSet=true;
              _elevatorWanted=(actDive-diveWanted)*10;
              maxAilerons = 0;
            }
            else
            {
              elevatorSet=true;
              _elevatorWanted=0;
            }
          }

          // height>=50: if plane is taking off near stall, if it climbs high enough, consider take off finished
          if (!_landContact && (height>=10 && vs.ModelSpeed().Z()>landingSpeed || height>=50))
          {
            _pilotGear=false;
            if (!_landContact && height>=30)
            {
              _planeState=Flight;
              if (!QIsManual(EffectivePilotingUnit()))
              {
                _pilotHeight=height+5;
                _pilotFlaps=0;
              }
            }
          }
        }

        // calculate flap efficiency
        float flapEff=vs.ModelSpeed()[2]*(1.0/40);
        float rudderEff = flapEff;
        saturate(flapEff,0.1,1);
        float invFlapEff=1/flapEff;
        
        if (setControls)
        {
          if (_pilotHelperDir)
          {
            // when on the ground, rudder is controlling the wheel
            // therefore there is no need to apply any drastic values
            // when the speed is low, the rudder is inefficient anyway and there is no use boosting it
            const float lowEff = 0.3f;
            if (rudderEff<lowEff)
            {
              // convert efficiency 0 to 10 to reduce slow speed oscillation
              // convert lowEff to lowEff
              rudderEff = InterpolativC(rudderEff,0,lowEff,10,lowEff);
            }
            // once we are flying fast and not aiming at the target, let the auto-coordination do the job
            if (_landContact || _vtolMode)
            {
              _rudderWanted =- 200*changeHeadingRudder/rudderEff;
            }
            else
            {
              _rudderWanted = -200*changeHeadingRudder*useRudder/rudderEff;
            }
            saturate(_rudderWanted,-1,+1);
          }
          if (!_landContact && fabs(changeHeading)>H_PI/4 && vs.ModelSpeed().Z()<_pilotSpeed+20 && fabs(bank)>0.5f)
          {
            // do not brake in tight banked turns unless we are moving very fast
            // we will loose enough energy without braking
            _pilotBrake = 0;
          }
        }

        // trim when flying slow
        if (setControls && !elevatorSet)
        {

          if( !diveWantedSet )
          {

            if( fabs(_forceDive)<0.99f )
            {
              // forced dive (attacking etc)
              diveWanted=_forceDive;
            }
            else
            {
              // when in VTOL mode, dive controls speed, not altitude
              if (_pilotHelperHeight && EngineIsOn()) // EngineIsOn to avoid automatic flaps on inactive attached planes
              {
                AutoFlaps();
                AutoGear(false);
                if (!_vtolMode)
                {
                  float diveChange = _pilotHeight-estHeight;

                  float diveFactor = 0.5f;
                  if (vs.ModelSpeed().Z()>1)
                  {
                    diveFactor /= vs.ModelSpeed().Z();
                  }

                  diveWanted = dive+diveChange*diveFactor;

                  // with VTOL plane we want to avoid positive (nose up) dive until lift is established
                  if (Type()->_vtol)
                  {
                    // and in any way we want to avoid dive which would be too extreme
                    float maxDive = InterpolativC(vs.ModelSpeed().Z(),Type()->_stallSpeed*0.5,Type()->_stallSpeed,0,0.9f);
                    saturateMin(diveWanted,maxDive);
                  }
                  // positive dive is up
                  float limitDive = floatMin(fabs(bank)*1.4f-1,-0.05f);
                  saturateMax(diveWanted,limitDive);

                  //max dive / bank depending on AoA (stall recovery)
                  if (!_landContact && !_waterContact && !_objectContact)
                  {
                    // detect actual stall
                    float stall = 0;
                    // with VTOL planes in VTOL mode we do not care much about stalling
                    // however as closing to stall, we want to limit bank as well for them
                    const float maxNormalAoa = 8*H_PI/180;
                    const float maxStallAoa = 30*H_PI/180;
                    if(aoa>maxNormalAoa)
                    {
                      saturateMax(stall, (aoa-maxNormalAoa)*(1.0/(maxStallAoa-maxNormalAoa)));
                    }
                    if (vs.ModelSpeed().Z()<stallSpeed*2)
                    {
                      // detect stall possibility due to low speed
                      saturateMax(stall,(stallSpeed*2-vs.ModelSpeed().Z())/stallSpeed);
                    }
                    saturate(stall,0,1);
                    float maxBank = 0.65f*(1-stall)+0.3f;
                    if (GetType()->GetMaxSpeed()<450 && !_sweepTarget)
                    {
                      // patch: limit turning in slow planes like Cessna
                      // even for them, if in combat, more aggressive maneuvers are enabled
                      saturateMin(maxBank,0.7f);
                    }

                    
                    Limit(bankWanted,-maxBank,+maxBank);

                    // when VTOL plane is going slow, we do not mind - it can vector thrust if needed
                    if (!Type()->_vtol)
                    {
                      if (_planeState!=Final) // no stall recovery by using thrust when landing
                      {
                        saturateMax(_thrustWanted,stall);
                      }
                      if (stall>0.3f)
                      {
                        _pilotBrake=0;
                      }
                    }
                    float maxDive = 0.7f*(1-stall);
                    // limit diving too steep when flying low and fast
                    float refHeight = vs.ModelSpeed().Z()*3.6f; // for 300 km/h refHeight is 300 m
                    float minDiveByHeight = Interpolativ(height,refHeight*0.25f,refHeight,-0.1f,-0.7f);
                    saturateMax(diveWanted,minDiveByHeight);
                    Limit(diveWanted,minDiveByHeight,maxDive);
                  }
                }
              }
              else
              {
                diveWanted = 0;
              }
            }
            Limit(diveWanted,-0.7,+0.7);

          }
          float bankElevator = floatMax(0,fabs(bank)-0.4);
          // when making small adjustments
          // we need to make elevator movements stronger
          float elevChange = (diveWanted-dive)*invFlapEff*4;
          if (!QIsManual(EffectivePilotingUnit()) && _sweepTarget)
          {
            const float diveEnhancerMax = 0.1f;
            if (fabs(elevChange)<diveEnhancerMax)
            {
              float factor = floatMin(10,fabs(elevChange)*300+1);
              /*
              LogF
              (
                "diveWanted %.3f, dive %.3f, elevChange %.3f, factor %.3f",
                diveWanted,dive,elevChange,factor
              );
              */
              elevChange *= factor;
              saturate(elevChange,-diveEnhancerMax,+diveEnhancerMax);
            }
          }

          _elevatorWanted = -elevChange - bankElevator;
        } // pilot helper full
        #if 0
        LogF
        (
          " bank %.2f->%.2f, dive %.2f->%.2f, rudderW %.2f, elevW %.2f",
          bank,bankWanted,dive,diveWanted,_rudderWanted,_elevatorWanted
        );
        #endif
        // if plane is lower that wanted, limit dive to enable climbing
        if (setControls)
        {
          if (_pilotHelperHeight)
          {
            const float loSafeHeight = _defPilotHeight;
            const float hiSafeHeight = _defPilotHeight+100;
            const float loSafeFactor = 10;
            const float hiSafeFactor = 40;
            float heightScale = loSafeFactor;
            if (height>hiSafeHeight) heightScale = hiSafeFactor;
            else if (height<loSafeHeight) heightScale = loSafeFactor;
            else
            {
              float factor = (height-loSafeFactor)*1/(hiSafeFactor-loSafeFactor);
              heightScale = factor*hiSafeFactor + loSafeFactor - factor*loSafeFactor;
            }
            float lowWarning=(_pilotHeight-height)/heightScale;
            float heightSafe=1-lowWarning*0.5f;
            saturate(heightSafe,0.1f,1);
            if (_planeState==Takeoff)
            {
              // special limit during take-off
              saturateMin(heightSafe, InterpolativC(height, 10, 30, 0.1f, 1.0f));
            }
            Limit(bankWanted,-0.95f*heightSafe,+0.95f*heightSafe);
          }
          // in VTOL mode the plane is less reactive - use stronger inputs
          float factor = _vtolMode ? 4.0f : 2.0f;
          _aileronWanted = +(bankWanted-bank)*factor;

          if( _angVelocity.SquareSize()>=10*10 )
          { // if we are spinning fast, leave all controls in neutral position
            _rudderWanted=0;
            _aileronWanted=0;
            _elevatorWanted=0;
            _thrustWanted=0.5;
          }
        }
        Limit(_aileronWanted,-maxAilerons,+maxAilerons);

        #if _ENABLE_CHEATS
        if (CHECK_DIAG(DECombat) && GLOB_WORLD->CameraOn()==this )
        {
          const char *state="???";
          switch( _planeState )
          {
            case Flight: state="Flight";break;
            case Takeoff: state="Takeoff";break;
            case TaxiIn: state="TaxiIn";break;
            case Marshall: state="Marshall";break;
            case Approach: state="Approach";break;
            case Final: state="Final";break;
            case Landing: state="Landing";break;
            case AutoLanding: state="AutoLanding";break;
            case Touchdown: state="Touchdown";break;
            case AfterTouchdown: state="AfterTouchdown";break;
            case WaveOff: state="WaveOff";break;
            case TaxiOff: state="TaxiOff";break;
          }
          /*
          GlobalShowMessage
          (
            100,
            "%s: AoA %.1f, v=%.0f, dive=%.2f->%.2f, thr %.2f->%.2f, bank %.2f->%.2f",
            state,aoa*(180/H_PI),ModelSpeed()[2]*3.6,
            dive,diveWanted,
            _thrust,_thrustWanted,bank,bankWanted
          );
          */
            DIAG_MESSAGE(100,Format(
              "AoA %.1f, dive=%.2f->%.2f, thr %.2f, height %.2f (est %.1f,min %.1f, climb %.2f), %s",
              aoa*(180/H_PI),dive,diveWanted,
              _thrust,_pilotHeight,estHeight,estHeightMin,vs._speed.Y(),state
            ));
        }
        #endif
      }

    }
    else if (IsStopped())
    {
      _rudderWanted=0;
      _aileronWanted=0;
      _elevatorWanted=0;
      _thrustWanted=0;
    }
    else // vehicle moving, no pilot, or pilot is dead
    {
      _rudderWanted=0.1;
      _aileronWanted=0.1;
      _elevatorWanted=0;
      _thrustWanted=0.3;
    }
  }
  
  saturate(_thrustWanted,0,1);

  MoveWeapons(deltaT);

  // perform advanced simulation
  base::Simulate(deltaT,prec);
/*
LogF("Speed %.3f, Pilot speed %.3f", ModelSpeed()[2], _pilotSpeed);
*/
}


bool AirplaneAuto::AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction)
{
  WeaponModePtr selected = context._weapons->FindValidWeapon(ptr);
  if (!selected.IsValid()) return false;
  context._weapons->SelectWeapon(this, selected);

  // always aim gun - to be ready if needed
  // move turret/gun accordingly to direction
  Vector3 relDir(VMultiply,DirWorldToModel(),direction);
  // calculate current gun direction
  // compensate for neutral gun position
  _gunYRotWanted=-atan2(relDir.X(),relDir.Z());
  //float neutralXRot=atan2(_missileDir.Y(),_missileDir.Z());
  float sizeXZ=sqrt(Square(relDir.X())+Square(relDir.Z()));
  //const WeaponInfo &info=GetWeapon(weapon);
  _gunXRotWanted=atan2(relDir.Y(),sizeXZ); //-Type()->_neutralGunXRot;

  float gunXRotWanted=_gunXRotWanted;
  float gunYRotWanted=_gunYRotWanted;

  Limit(_gunXRotWanted,Type()->_minGunElev,Type()->_maxGunElev);
  Limit(_gunYRotWanted,Type()->_minGunTurn,Type()->_maxGunTurn);
  float xOffRange=fabs(gunXRotWanted-_gunXRotWanted);
  float yOffRange=fabs(gunYRotWanted-_gunYRotWanted);
  float xToAim=fabs(_gunXRotWanted-_gunXRot);
  float yToAim=fabs(_gunYRotWanted-_gunYRot);
  if( xToAim+yToAim>1e-6 ) CancelStop(); // enable simulation

  const AmmoType *ammo = selected.GetAmmoType();
  if (!ammo) return false;
  if (ammo->_simulation == AmmoShotMissile && ammo->maxControlRange>10) return true;
  if( xOffRange+yOffRange<0.001 ) return true;
  return false;
}

bool AirplaneAuto::CalculateAimWeaponPos(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &pos, float &timeToLead, Target *target, bool exact ) const
{
  WeaponModePtr selected = context._weapons->FindValidWeapon(ptr);
  if (!selected.IsValid()) return false;

  if ( target->idExact.IsNull()) return false;
  if (!CheckTargetKnown(context,target))
  {
    return false;
  }

  Vector3 tgtPos, tgtSpd;
  if (exact)
  {
    tgtPos = target->idExact->AimingPosition(target->idExact->RenderVisualState());
    tgtSpd = target->idExact->RenderVisualState().Speed();
  }
  else
  {
    AIBrain *gunnerBrain = context._gunner ? context._gunner->Brain() : NULL;
    tgtPos = target->LandAimingPosition(gunnerBrain);
    tgtSpd = target->GetSpeed(gunnerBrain);
  }

  pos = tgtPos;
  
  const Magazine *magazine = selected.GetMagazine();
  const MagazineType *aInfo = magazine ? magazine->Type() : NULL;
  const AmmoType *ammo = aInfo ? aInfo->_ammo : NULL;
  if (ammo && (ammo->_simulation != AmmoShotMissile || ammo->maxControlRange<10))
  {
    // calculate gun ballistics
    if (aInfo)
    {
      // we need to predict somewhat in the future - it will take some time
      // until we actually fire
      const float latency = 0.050f;
      Vector3 relSpeed = RenderVisualState().Speed()-tgtSpd;
      Vector3Val myPos = RenderVisualState().Position();
      // v - bullet velocity
      // A - airplane position, a - airplane velocity
      // M - target position, m - target velocity
      // k - gun direction, b - bullet muzzle speed
      // v = Ak.b + a - m
      // d = M.pos - A.pos
      // t = v*d/(|v|*|v|)
      // v = d/t , Ak = (d/t - a + m)/b
      Vector3 d = tgtPos-myPos - relSpeed*latency;
      Vector3 v ( // v
        RenderVisualState().Speed()
        +RenderVisualState().DirectionModelToWorld(Type()->_gunDir*aInfo->TravelSpeed())
        -tgtSpd
      );
      float t = (v*d)*v.InvSquareSize();
      // if the target is behind the current aim, there is no solution
      if (t<0) t = 0;
      pos = tgtPos - relSpeed*t;
      pos[1] += 0.5f*G_CONST*Square(t);
    }
  }

  return true;
}

bool AirplaneAuto::CalculateAimWeapon(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &dir, float &timeToLead, Target *target, bool exact ) const
{
  Vector3 tgtPos;
  bool ret = CalculateAimWeaponPos(context, ptr, tgtPos, timeToLead, target, exact);
  if (ret)
  {
    Vector3 weaponPos=Type()->_gunPos;
    Vector3 myPos=FutureVisualState().PositionModelToWorld(weaponPos);
    dir = tgtPos-myPos;
  }
  return ret;
}

bool AirplaneAuto::AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target)
{
  Vector3 dir;
  float timeToLead;
  if (!CalculateAimWeapon(context,ptr,dir,timeToLead,target,false)) return false;

  return AimWeapon(context, ptr, dir);
}

Matrix4 AirplaneAuto::GunTransform() const
{
  return MIdentity;
}

Vector3 AirplaneAuto::GetWeaponManualControlDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  if (!context._turretType)
    return vs.Direction();
  return vs.Transform().Rotate(context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType)));
}

Vector3 AirplaneAuto::GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  const AmmoType *ammo = ptr.GetAmmoType();
  if (!ammo)
  {
    if (context._turretType)
    {
      return vs.Transform().Rotate(context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType)));
    }
    return vs.Direction();
  }
#if _VBS3
  if (!ptr.IsValid()) return vs.Direction();
  if (context._turretType && context._weapons->_laserTargetOn)
  {
    // return the turret rotation, when laser target is on.
    return vs.Transform().Rotate(context._turretType->GetTurretDir(GunTurretTransform(*context._turretType)));
  }
#endif
  switch (ammo->_simulation)
  {
    case AmmoShotBullet:
    case AmmoShotSpread:
    {
      if(context._turretType)
      {
        return vs.Transform().Rotate(context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType)));
      }
      
      Matrix4Val shootTrans=GunTransform();
      Vector3 wepDir = shootTrans.Rotate(Type()->_gunDir);
      return vs.DirectionModelToWorld(wepDir);
      
    };
    case AmmoShotRocket:
    case AmmoShotMissile:
      if (ammo->maxControlRange<10)
      {
        return vs.DirectionModelToWorld(Type()->_rocketDir);
      }
      else
      {
        if(context._turretType)
        {
          return vs.Transform().Rotate(context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType)));
        }
        return vs.Direction();
      }
    default:
      return vs.Direction();
  }
}

#if _VBS3
Vector3 AirplaneAuto::GetWeaponDirectionWanted(const TurretContextEx &context, const WeaponModePtr &ptr) const
{
  if (!ptr.IsValid() || !context._turret) return FutureVisualState().Direction();
  if (!context._turretType) return FutureVisualState().Direction();
  const AmmoType *ammo = ptr.GetAmmoType();
  if (!ammo ) return FutureVisualState().Direction();
  switch(ammo->_simulation)
  {
    case AmmoShotBullet:
    {
      Matrix3Val aim = context._turret->GetAimWantedWorld(context._parentTrans);
      Vector3 dir = context._turretType->GetTurretDir(aim);
      return dir;
    }
    default:
      return Direction();
  }
}
#endif

/*!
\patch 1.33 Date 11/28/2001 by Ondra.
- Fixed: AI: Airplane laser guided bomb aiming improved.
\patch 1.42 Date 1/9/2002 by Ondra
- Fixed: Debugging message shown when AI preparing to release LG bomb.
\patch 5147 Date 3/24/2007 by Ondra
- Fixed: Airplanes were firing rockets even when not aimed properly.
*/
float AirplaneAuto::GetAimed(const TurretContext &context, const WeaponModePtr &ptr, Target *target, bool exact, bool checkLockDelay) const
{
/*
  // check if weapon is aimed
  return 1.0;
*/

  if( !target ) return 0;
  if( !target->idExact ) return 0;
  if (!CheckTargetKnown(context,target)) return 0;
  if (!context._gunner) return 0;
  // check if weapon is aimed
  if (!ptr.IsValid()) return 0;

  AIBrain *unit = context._gunner ? context._gunner->Brain() : NULL;
  if (!unit) return 0.0f;

  const Magazine *magazine = ptr.GetMagazine();
  const MagazineType *aInfo = magazine ? magazine->Type() : NULL;
  const AmmoType *ammo = aInfo ? aInfo->_ammo : NULL;
  if (!ammo) return 0;

  float visible = 0.0f;
  bool headOnly = false;
  if (exact)
  {
    visible = 1.0f;
  }
  else if (target->SeenRecently(unit))
  {
    visible=_visTracker.Value(headOnly,this, context, ptr, target->idExact, 0.9);
    // 0.6 visibility means 0.8 unaimed
    visible=1-(1-visible)*0.5f;
  }

  Vector3 ap = exact ? target->idExact->AimingPosition(target->idExact->FutureVisualState()) : target->LandAimingPosition(context._gunner->Brain(),ammo);

  if (ammo->_simulation == AmmoShotMissile && ammo->maxControlRange>10)
  {
    if (ammo->thrustTime>0)
    {
      // guided/non-guided missile  
      Vector3 relPos=PositionWorldToModel(ap);
      // check if target is in front of us
      if( relPos.Z()<=100 ) return 0; // missile fire impossible
      // check if target position is withing missile lock cone
      float rYPos=relPos.Y();
      if( fabs(relPos.X())>relPos.Z() ) return 0;
      if( fabs(rYPos)>relPos.Z() ) return 0;
      // the nearer we are, the more precise lock required
      float invRZ=0.5/relPos.Z();
      float lockX=1-fabs(relPos.X())*invRZ;
      float lockY=1-fabs(rYPos)*invRZ;
      float lock=floatMin(lockX,lockY);
      saturate(lock,0,1);
      lock*=visible;
      if( lock<0.5 )
        lock=0;


      return lock;
    }
    else
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
  }
  else
  {
    // predict shot result
    Vector3 wPos = FutureVisualState().PositionModelToWorld(GetWeaponCenter(FutureVisualState(), context, ptr));
    Vector3 tgtSpd = exact ? target->idExact->FutureVisualState().Speed() : target->GetSpeed(context._gunner->Brain());

    float dist=ap.Distance(wPos);

    Vector3 d = ap-wPos;
    Vector3 v ( // v
      FutureVisualState().Speed()
      +FutureVisualState().DirectionModelToWorld(Type()->_gunDir*aInfo->TravelSpeed())
      -tgtSpd
    );
    float time = (v*d)*v.InvSquareSize();

    Vector3 leadSpeed = tgtSpd-FutureVisualState().Speed();
    Vector3 estPos = ap+leadSpeed*time;

    Vector3 wDir = GetWeaponDirection(FutureVisualState(), context, ptr);
    float eDist=estPos.Distance(wPos);

    Vector3 hit = wPos+wDir*eDist;
    hit[1] -= 0.5f*G_CONST*time*time;
    Vector3 hError=hit-estPos;
#if _ENABLE_CHEATS
    float distHit = hit.Distance(wPos);
#endif

    hError[1] *= 3;

    float error = hError.Size();
    float invAimingAccuracy = GetInvAbility(AKAimingAccuracy);
    float aimPrecision=(invAimingAccuracy-1)*1.5f+1;
    saturateMax(aimPrecision,1);

    float tgtSize =
    (
      target->idExact->GetShape()->GeometrySphere()*aimPrecision*0.25f
      + ammo->indirectHitRange*3
    );
    const MuzzleType *muzzle = ptr.GetMuzzleType();
    float aiDispersionCoefX,aiDispersionCoefY;
    muzzle->GetDispersionCoefs(aiDispersionCoefX,aiDispersionCoefY,floatMax(invAimingAccuracy,1));
    float dCoef=floatMax(aiDispersionCoefX,aiDispersionCoefY);
    tgtSize+=dist*ptr.SumDispersionModifiers(unit->GetPerson())*dCoef*aimPrecision*0.5f;

    if (leadSpeed.SquareSize()>Square(10)) tgtSize *= 2;

#if _ENABLE_CHEATS
    if ((Object *)this==GWorld->CameraOn() && CHECK_DIAG(DECombat))
    {
      DIAG_MESSAGE(2000,Format(
        "Error %.1f, tgtSize %.1f, time %.2f, "
        "ePos %.1f,%.1f,%.1f, distErr %.1f",
        error,tgtSize,time,
        hError[0],hError[1],hError[2],
        distHit-eDist
      ));
    }
#endif

    if( error<=tgtSize ) return visible;
    if( error>=tgtSize*2 ) return 0;
    return ( 2*tgtSize-error )/tgtSize*visible;

  }
}

/**
@param factor can be used to force flaps being used to slow down during landing
*/
void AirplaneAuto::AutoFlaps(float factor)
{
  float landingSpeed = Type()->_landingSpeed;
  float zSpeed = FutureVisualState().ModelSpeed().Z();
  if (zSpeed<landingSpeed*1.15f*factor)
  {
    _pilotFlaps = 2;
  }
  else if (zSpeed>landingSpeed*1.2f*factor && zSpeed<landingSpeed*1.4f*factor)
  {
    _pilotFlaps = 1;
  }
  else if (zSpeed>landingSpeed*1.5f*factor)
  {
    _pilotFlaps = 0;
  }
}

void AirplaneAuto::AutoGear(bool extend)
{
  float landingSpeed = Type()->_landingSpeed;
  float zSpeed = FutureVisualState().ModelSpeed().Z();
  if (extend && zSpeed<landingSpeed*1.3)
  {
    _pilotGear = true;
  }
  else if (zSpeed>landingSpeed*1.5f)
  {
    _pilotGear = false;
  }
}


bool AirplaneAuto::PrepareGetOut(AIBrain *unit)
{
  if (Type()->_cabinOpening)
  {
    // request cabin opening simulation
    _openCabin = true;
    return (FutureVisualState()._cabinPos>0.99f && base::PrepareGetOut(unit));
  }
  return base::PrepareGetOut(unit);
}
void AirplaneAuto::FinishGetOut(AIBrain *unit)
{
  _openCabin = false;
  base::FinishGetOut(unit);
}
bool AirplaneAuto::PrepareGetIn(UIActionType pos, Turret *turret, int cargoIndex)
{
  if (Type()->_cabinOpening)
  {
    // request cabin opening simulation
    _openCabin = true;
    return (FutureVisualState()._cabinPos>0.99f && base::PrepareGetIn(pos, turret, cargoIndex));
  }
  return base::PrepareGetIn(pos, turret, cargoIndex);
}
void AirplaneAuto::FinishGetIn(UIActionType pos, Turret *turret, int cargoIndex)
{
  if (pos!=ATGetInCargo)
    _openCabin = false; // if there was any request to open the cabin, cancel it now
  // make sure we have a proper state once we need it
  if (pos==ATGetInPilot || pos==ATGetInDriver)
  _planeState = TaxiIn;
  base::FinishGetIn(pos, turret, cargoIndex);
}


/**
@param init true when we went to initiate, false when maintaining
*/
bool AirplaneAuto::CheckLandingAvailable(bool init) const
{
  float landingSpeed = Type()->_landingSpeed;
  float zSpeed = FutureVisualState().ModelSpeed().Z();

  float relaxed = init ? 1.0f : 1.5f;
  Vector3 ilsPos = FutureVisualState().Position(),ilsDir = FutureVisualState().Direction();
  bool ilsSTOL = false;
  if (GWorld->GetILS(ilsPos,ilsDir,ilsSTOL,TSideUnknown,Type()->_vtol>0)<0) return false;
  const float maxDistance = 3000;
  float dist = FutureVisualState().Position().Distance(ilsPos);
  if (dist>maxDistance*relaxed) return false;
  // check init specific conditions
  if (init)
  {
    // we need some distance to adjust
    if (dist<200) return false;
    // if we are moving too fast given the distance, do not init
    float maxSpeed = floatMax(1,(dist-200)/1000+1);
    if (zSpeed>landingSpeed*maxSpeed) return false;
    // max. altitude - avoid diving too much
    // this is very important - otherwise we cannot break
    float maxAlt = dist*0.1f;
    float alt=FutureVisualState().Position().Y()-ilsPos.Y();
    if (alt>maxAlt) return false;
  }
  else
  {
    // maintain only -
    // if we are very close, we cannot cancel any more
    if (dist<100) return true;
  }
  // calculate our position in the ILS cone
  Vector3 relDir = FutureVisualState().Position()-ilsPos;

  float cosAngle = relDir.CosAngle(ilsDir);

  float maxAngle = HDegree(20)*relaxed;
  const float minCosAngle = cos(maxAngle);
  if (cosAngle<minCosAngle) return false;

  // check general direction toward the ILS pos
  Vector3 dir2D(FutureVisualState().Direction().X(),0,FutureVisualState().Direction().Z());
  Vector3 relDir2D(-relDir.X(),0,-relDir.Z());
  float cosAlign = relDir2D.CosAngle(dir2D);

  float maxAlignAngle = (dist-200)/200+5;
  saturate(maxAlignAngle,5,15);

  float maxAlign = HDegree(maxAlignAngle)*relaxed;
  const float minCosAlign = cos(maxAlign);
  if (cosAlign<minCosAlign) return false;

  return true;
}

static float CosAngleFlat(Vector3Par a, Vector3Par b)
{
  Vector3 aFlat(a.X(),0,a.Z());
  Vector3 bFlat(b.X(),0,b.Z());
  return aFlat.CosAngle(bFlat);
}
/*!
\patch 5161 Date 5/29/2007 by Ondra
- New: Event handlers LandedTouchDown and LandedStopped
*/

AirplaneState AirplaneAuto::LandingAutopilot(
  Vector3Par ilsPos, Vector3Par ilsDir, bool ilsSTOL,
  float &changeHeading, float curHeading,
  float &diveWanted, bool &diveWantedSet, bool &setControls
)
{
  _pilotHelperHeight = false;
  _pilotHelperThrust = true;
  _pilotHelperBankDive = true;
  // desired aoa is reached
  // once we touch the ground, break
  if  (_landContact)
  {
    _pilotSpeed = 0;
    // touched the ground - check which airport it was
    int airportId = GWorld->CheckAirport(FutureVisualState().Position(),FutureVisualState().Direction());
    // and call the event handler for the airport
    OnEvent(EELandedTouchDown,float(airportId));
    // cancel any landing requests
    _landAtAirport = -1;
    return AfterTouchdown;
  }

  _pilotSpeed = Type()->_landingSpeed;

  AirplaneState ret = _planeState;

  // check if we are in ILS corridor
  float distance = (FutureVisualState().Position()-ilsPos).DotProduct(ilsDir);
  Vector3 ilsCPos = ilsPos+Vector3(0,1.5f,0)+ilsDir*distance;
  Vector3 relPos = (ilsPos + ilsCPos)*0.5f-FutureVisualState().Position();
  // once we are very near the runway, align with it
  if (ret==Touchdown || distance<100)
  {
    // follow runway
    float y=ilsCPos[1];
    ilsCPos=ilsPos-ilsDir*500;
    ilsCPos[1]=y;
    relPos=ilsCPos-FutureVisualState().Position();
  }
  // convert direction relative to the ilsDirection
  if (ilsSTOL)
  {
    // as we are closing the touchdown, slow down
    _pilotSpeed *= Interpolativ(distance,0,Type()->FinalDist()*0.8f,0.3f,1.0f);
  }
  
  Matrix3 ilsFrame(MDirection,ilsDir,VUp);
  Matrix3 ilsFrameInv(MInverseRotation,ilsFrame);
  // we need to correct any side errors as soon as possible
  // relative position in the ILS frame
  Vector3 relPosIls = ilsFrameInv*relPos;
  _pilotHeading=atan2(relPos.X(),relPos.Z());
  changeHeading = AngleDifference(_pilotHeading,curHeading);
  if (fabs(changeHeading)>H_PI*0.5f)
  {
    // once we passed the ILS point, head away from it
    changeHeading = AngleDifference(H_PI,changeHeading);
  }
  Limit(changeHeading,-0.2f,+0.2f);
  const float landDescent = ilsDir[1]*FutureVisualState()._speed.SizeXZ();
  const float landingSpeed = Type()->_landingSpeed;
  // check position in ILS corridor
  float touchdownY=ilsPos[1];
  float ilsY=ilsCPos.Y()-touchdownY;
  saturateMin(ilsY,100);
  ilsY += touchdownY;
  float above=FutureVisualState().Position().Y()-ilsY-2.0f;
  float aboveTouchdown = FutureVisualState().Position().Y()-touchdownY;
  /// check for wave off before touchdown
  if (distance<200)
  {
    const float cos10deg = 0.98480775301f;
    const float cos6deg = 0.994521895368f;
    float cosDir = CosAngleFlat(-FutureVisualState().Direction(),ilsDir);
    //float cosSpd = CosAngleFlat(-FutureVisualState().Speed(),ilsDir);
    static const float minHeightTouchdown = 6.0f;
    float heightTouchdown = floatMax(landDescent*4,minHeightTouchdown); // allow at least 4 seconds to touchdown
    if (FutureVisualState().Position().Y()-touchdownY>40)
    {
      ret = WaveOff;
      LogF("WaveOff far: too high %.1f",FutureVisualState().Position().Y()-touchdownY);
    }
    else if (fabs(relPosIls.X()>25) || cosDir<cos10deg)
    {
      ret = WaveOff;
      LogF("WaveOff far: pos.x %.1f, cosDir %.4f",relPosIls.X(),cosDir);
    }
    else if( ret!=Touchdown && (distance<90 || aboveTouchdown<heightTouchdown))
    {
      // check if we are aligned
      if (fabs(relPosIls.X()<13) && cosDir>cos6deg) ret = Touchdown;
      else if (distance<50 || aboveTouchdown<5 || fabs(relPosIls.X()>25))
      {
        ret = WaveOff;
        LogF("WaveOff: pos.x %.1f, cosDir %.4f",relPosIls.X(),cosDir);
      }
    }
  }
  // estimate descent rate
  //const float estT = 3.0f;
  static float estT = 0.5f;
  float estDescent=-FutureVisualState()._speed[1]-FutureVisualState()._acceleration[1]*estT;
  // control descent to keep in ILS
  // descent rate should be set so that at current velocity
  // we would descent in direction given by -ilsDir
  float descent=landDescent+above*0.5f;
  float maxDescent = landDescent*1.5f;
  float maxClimb = -landDescent*0.5f;
  if( ret==Touchdown )
  {
    const float normDescent = ilsDir[1]*landingSpeed;
    Limit(descent,normDescent*0.25f,normDescent*0.5f);
    Limit(changeHeading,-0.05f,+0.05f);
  }
  else if (!ilsSTOL)
  {
    Limit(descent,maxClimb,maxDescent);
  }
  // keep AoA constant
  // use thrust to control AoA
  float aoaWanted = Type()->_landingAoa;

  float aoa = CalculateAOA(FutureVisualState().ModelSpeed());
  float surfaceY=GLOB_LAND->SurfaceYAboveWaterNoWaves(FutureVisualState().Position().X(),FutureVisualState().Position().Z());
  float height=FutureVisualState().Position().Y()-surfaceY;
  _pilotHeight = height;


  
  float dive = FutureVisualState().Direction().Y();
  diveWantedSet=true;
  diveWanted = dive - (descent-estDescent)*0.05f;
  // use dive to control descent rate
  static float maxLandingNoseUp = 0.15f;
  static float maxTouchdownNoseUp = 0.20f;
  saturate(diveWanted,-0.2f,ret==Touchdown ? maxTouchdownNoseUp : maxLandingNoseUp);

  if (ilsSTOL && Type()->_vtol && FutureVisualState().ModelSpeed()[2]<landingSpeed*1.1f)
  {
    // as we are closing the touchdown, slow down
    _pilotSpeed = landingSpeed*Interpolativ(distance,Type()->FinalDist()*0.4f,Type()->FinalDist()*0.9f,0.3f,1.0f);
    Vector3 accel = DirectionWorldToModel(FutureVisualState()._acceleration);
    Vector3Val relSpeed = FutureVisualState().ModelSpeed();
    const float est_delta = 2.0f;
    float changeAccel = (_pilotSpeed-relSpeed.Z())*(1/est_delta)-accel[2];
    float changeAccelY = (-descent-FutureVisualState().Speed().Y())*(1/est_delta)-accel[1];
    
    // both AoA and speed can be controlled using thrust vectoring

    float neutral = NeutralThrustVector(false,FutureVisualState().Orientation());
    _thrustVectorWanted = 1.0-changeAccel*0.1f + neutral;
    // when vectored, use the thrust to control vertical speed
    _thrustWanted = Lerp(_thrust+changeAccel*0.1f, _thrust + changeAccelY*0.1f, FutureVisualState()._thrustVector);
    _pilotBrake = 1.0f;
    // once flying slow, we no longer need any dive for descent, set zero dive instead
    diveWanted *= Interpolativ(relSpeed[2],landingSpeed,Type()->_stallSpeed,0,1);
  }
  else
  {
    _thrustVectorWanted = 0;

  // when AoA differs 1 degree, we want 1m/s^2 acceleration
  float accelWanted = (aoa-aoaWanted)*(180/H_PI);
    // no matter what AoA we have, avoid going too fast
    float speedThrustLimit = (landingSpeed-FutureVisualState().ModelSpeed().Z())*0.4f;
  // to change acceleration about 1 m/s we want to change thrust about 0.2
  // as we are using the current value anyway, this should not matter much
  //_thrustWanted = floatMin(_thrust+(_acceleration.Z()-accelWanted)*0.2f,speedThrustLimit);
  _thrustWanted = floatMin(_thrust+accelWanted*0.2f,speedThrustLimit);
  // note: real pilots climb/dive by changing speed, i.e. changing target AoA

    // final phase of touch-down
    if (ret==Touchdown && FutureVisualState().Position().Y()-touchdownY<2.5f)
  {
      // apply brake just when touching ground
      _thrustWanted = -1;
    }
    // apply air brake when necessary
    if (_thrustWanted<0) _pilotBrake = -_thrustWanted;
    else _pilotBrake = 0;
  }

  #if _ENABLE_CHEATS
    if (CHECK_DIAG(DECombat) && this==GWorld->CameraOn())
    {
      DIAG_MESSAGE(100,Format(
        "dive %.2f->%.2f, descent %.2f, est %.2f, above %.1f, aboveTouchdown %.1f",
        dive,diveWanted,descent,estDescent,above,aboveTouchdown
      ));
    }
  #endif

  // LANDING DIAG
  #if 0
  LogF
  (
    "Descent %.2f (cur %.2f, est %.2f, land %.2f), above %.2f, t %.2f->%.2f, dive %.2f"
    "Speed %.2f (land %.2f,%.2f)",
    descent,-_speed[1],estDescent,landDescent,above,_thrust,_thrustWanted,diveWanted,
    ModelSpeed().Z(),landingSpeed,stallSpeed
  );
  #endif

  #if 0
  const char *state="???";
  switch( _planeState )
  {
    case Flight: state="Flight";break;
    case Takeoff: state="Takeoff";break;
    case TaxiIn: state="TaxiIn";break;
    case Marshall: state="Marshall";break;
    case Approach: state="Approach";break;
    case Final: state="Final";break;
    case Landing: state="Landing";break;
    case AutoLanding: state="AutoLanding";break;
    case Touchdown: state="Touchdown";break;
    case WaveOff: state="WaveOff";break;
    case TaxiOff: state="TaxiOff";break;
  }
  GlobalShowMessage
  (
    100,
    "%s: Dive %.2f->%.2f, surfY %.1f"
    "descent %.1f->%.1f (norm %.1f, above %.1f), thrust %.2f->%.2f",
    state,Direction().Y()*(180/H_PI),diveWanted*(180/H_PI),
    Position().Y()-touchdownY,
    estDescent,descent,landDescent,above,
    _thrust,_thrustWanted
  );
  #endif
  setControls = true;
  _rudderWanted = 0;
  return ret;
}
// manual control


/*!
\patch 5161 Date 5/29/2007 by Ondra
- Fixed: Flap actions no longer present in the action menu for plane with no flaps.
*/
void AirplaneAuto::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  // if we are stopped, we may leave it to base class handling
  // it will add get-out, eject and any more default actions
  base::GetActions(add, unit);

  // if we are not stopped, eject and land
  if (unit && unit->GetVehicleIn() == this && DriverBrain()==unit)
  {

    switch (_planeState)
    {
      case Flight:
        // human controlled plane may landed only when withing ILS corridor
        //if (QIsManual(unit) && CheckLandingAvailable(true))
        {
          UIAction action(new ActionBasic(ATLand, this));
          add(action);
        }
        break;
      case Marshall: case Approach: case Final: case Landing: case AutoLanding:
        {
          UIAction action(new ActionBasic(ATCancelLand, this));
          add(action);
        }
        break;
    }
    // only for manual plane - gear and flaps
    if (QIsManual(EffectivePilotingUnit()))
    {
      if (!_gearDamage && Type()->_gearRetracting && !_landContact)
      {
        UIAction action(new ActionBasic(FutureVisualState().GetGearPos(Type()) > 0.5f ? ATLandGear : ATLandGearUp, this));
        add(action);
      }
      if (!_pilotHelperThrust && _vtolMode)
      {
        if (_thrustVectorWanted>0.01f)
        {
          UIAction action(new ActionBasic(ATFlapsUp, this));
          add(action);
        }
        if (_thrustVectorWanted<0.99f)
        {
          UIAction action(new ActionBasic(ATFlapsDown, this));
          add(action);
        }
      }
      else if (Type()->_hasFlaps)
      {
        if (FutureVisualState()._flaps>0.25f)
        {
          UIAction action(new ActionBasic(ATFlapsUp, this));
          add(action);
        }
        if (FutureVisualState()._flaps<0.75f)
        {
          UIAction action(new ActionBasic(ATFlapsDown, this));
          add(action);
        }
      }
      if (Type()->_vtol)
      {
        {
        UIAction action(new ActionBasic(_vtolMode ? ATAutoHoverCancel : ATAutoHover, this));
        add(action);
      }
    }
  }
  }

  //copilot taking control over plane actions 
  if(unit)
  {
    bool isPlayer = unit->IsPlayer();
    if (_enableCopilots && isPlayer && unit->GetVehicleIn() == this)
    {
      //pilot takes control back
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

}

bool AirplaneAuto::GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const
{
  switch (action->GetType())
  {
    case ATLand:
    case ATCancelLand:
    case ATFlapsDown:
    case ATFlapsUp:
    case ATAutoHover:
    case ATAutoHoverCancel:
      // No parameters
      return true;
  }
  return base::GetActionParams(params, action, unit);
}

void AirplaneAuto::PerformAction(const Action *action, AIBrain *unit)
{
  switch (action->GetType())
  {
    case ATLandGear:
      _pilotGear = true;
      break;
    case ATLandGearUp:
      _pilotGear = false;
      break;
    case ATFlapsDown:
      if (_vtolMode && !_pilotHelperThrust)
      {
        _thrustVectorWanted = fastCeil(FutureVisualState()._thrustVector*6+0.51f)/6;
        saturate(_thrustVectorWanted,0,1);
      }
      else
      {
      if (_pilotFlaps<2) _pilotFlaps++;
      }
      break;
    case ATFlapsUp:
      if (_vtolMode && !_pilotHelperThrust)
      {
        _thrustVectorWanted = fastFloor(FutureVisualState()._thrustVector*6-0.51f)/6;
        saturate(_thrustVectorWanted,0,1);
      }
      else
      {
      if (_pilotFlaps>0) _pilotFlaps--;
      }
      break;
    case ATAutoHover:
      _vtolMode = true;
      saturateMax(_thrustVectorWanted,1.0f/6); // when switching into autohover, we probably want some vectoring. Move one notch right away
      return;
    case ATAutoHoverCancel:
      _vtolMode = false;
      break;
    default:
      base::PerformAction(action,unit);
      break;
  }
}


bool AirplaneAuto::IsStopped() const
{
  if (!_landContact && !_objectContact) return false;
  ProtectedVisualState<const VisualState> vs = FutureVisualStateScope(true); // intentional clash - we do not mind about a little delay here
  if (vs->_speed.SquareSize()>Square(2) ) return false;
  if (QIsManual(EffectivePilotingUnit()) && EngineIsOn() && vs->_speed.SquareSize()>Square(0.5f)) return false;
  return base::IsStopped();
}

bool AirplaneAuto::CheckEject() const
{
  ProtectedVisualState<const VisualState> vs = FutureVisualStateScope(true); // intentional clash - we do not mind about a little delay here
  // if the place is not suitable for ejection, wait
  if (!ManType::CheckEject(vs->Position(),30)) return false;
  if (Type()->_ejectSpeed.SquareSize()<Square(2))
  {
    // no ejection seat - 
    // if we are on the land, wait until we are reasonably slow
    if (_landContact)
    {
      return vs->Speed().SquareSize()<Square(7);
    }
    float height = vs->Position().Y()-GLandscape->RoadSurfaceYAboveWater(vs->Position().X(),vs->Position().Z());
    // if we are flying too low, wait
    if (height<60) return false;
  }
  return ManType::CheckEject(vs->Position()+vs->Speed()*3,100);
}

void AirplaneAuto::DamageCrew( EntityAI *killer, float howMuch, RString ammo )
{
  AIBrain *unit = PilotUnit();
  if (unit)
  {
    if (GetRawTotalDamage()>=Type()->_ejectDamageLimit && unit->GetCombatModeLowLevel()>=CMCombat)
    {
      Time goTime = Glob.time + GRandGen.PlusMinus(1.0f,0.5f);
      if (goTime<_getOutAfterDamage)  _getOutAfterDamage = goTime;
    }
  }
  
  base::DamageCrew(killer,howMuch,ammo);
}

/*!
\patch 1.22 Date 8/30/2001 by Ondra
- Fixed: parachute ejected from plane or helicopter was always considered western.
\patch_internal 1.24 Date 9/20/2001 by Ondra
- Fixed: added Civilian and Resistance (in addition to previous West and East)
*/

void AirplaneAuto::Eject(AIBrain *unit, Vector3Val diff)
{
  // check height
  bool parachute = !IsStopped() && (
    Type()->_ejectSpeed.SquareSize()>Square(2) ||
    FutureVisualState().Position().Y()>GLOB_LAND->SurfaceYAboveWaterNoWaves(FutureVisualState().Position()[0],FutureVisualState().Position()[2])+30
  );
  if (parachute)
  {
    unit->GetPerson()->SetSpeed(FutureVisualState().DirectionModelToWorld(Type()->_ejectSpeed));
  }
  unit->ProcessGetOut(true, parachute, diff);
}

void AirplaneAuto::Land()
{
  if (QIsManual(EffectivePilotingUnit()) && CheckLandingAvailable(true))
  {
    if( _planeState==Flight )
    {
      _planeState=AutoLanding;
      Vector3 ilsPos = FutureVisualState().Position(), ilsDir = FutureVisualState().Direction();
      bool ilsSTOL = false;
      _landAtAirport = GWorld->GetILS(ilsPos,ilsDir,ilsSTOL,TSideUnknown,Type()->_vtol>0);
  }
  }
  else
  {
    if( _planeState==Flight )
    {
      _planeState=Marshall;
      Vector3 ilsPos = FutureVisualState().Position(), ilsDir = FutureVisualState().Direction();
      bool ilsSTOL = false;
      _landAtAirport = GWorld->GetILS(ilsPos,ilsDir,ilsSTOL,TSideUnknown,Type()->_vtol>0);
  }
}
}

void AirplaneAuto::LandAtAirport(int index)
{
  if (QIsManual(EffectivePilotingUnit())) return;
  
  // when early enough, we may cancel the landing and initiate a new one
  if (_planeState==Flight || _planeState>=Marshall && _planeState<=Landing)
  {
    // it is not quite sure if canceling landing is really possible during land phase
    // but as there is probably nothing worse than landing, we do not care much
    // the mission designer should rather know what he is doing
    _landAtAirport = index;
    // initiate landing
    _planeState=Marshall;
  }

}

void AirplaneAuto::AssignToAirport(int index)
{
  _homeAirport = index;
}

void AirplaneAuto::CancelLanding()
{
  if (QIsManual(EffectivePilotingUnit())) return;
  
  // when early enough, we may cancel the landing
  if (_planeState>=Marshall && _planeState<=Landing)
  {
    _planeState = Flight;
    // leave home airport untouched
  }
  _landAtAirport = -1;
}

void AirplaneAuto::CancelLand()
{
  switch (_planeState)
  {
    case Marshall: case Approach: case Final:
      _planeState = Flight;
      break;
    case Landing: case AutoLanding:
      _planeState = Flight;
      break;
  }
}

/*!
\patch 1.80 Date 8/2/2002 by Ondra
- New: Scripting: flyInHeight now affects not only helicopters, but also planes.
*/

void AirplaneAuto::SetFlyingHeight(float val)
{
  _defPilotHeight = val;
}

void AirplaneAuto::FakePilot( float deltaT )
{
}

inline float JAdj(float x)
{
  return x*fabs(x);
}

void AirplaneAuto::KeyboardAny(AIBrain *unit, float deltaT)
{
}


/*!
\patch 1.44 Date 2/12/2002 by Ondra
- Fixed: Airplane: Autodetection of joystick / keyboard thrust was wrong.
After using joystick thrust it was impossible to switch back to keyboard thrust.
\patch 1.89 Date 10/23/2002 by Ondra
- Improved: Slow plane takeoff improved, added more adjustable config parameters.
\patch 5101 Date 12/11/2006 by Bebul
- Fixed: Plane mouse control do not affect cyclic while looking around
*/

void AirplaneAuto::KeyboardPilot(AIBrain *unit, float deltaT )
{
  if (!unit->GetPerson() || unit->GetPerson()->KeyboardUsedInFrame()) return;
  switch (_planeState)
  {
    case Flight:
    case Takeoff:
    case TaxiIn: case TaxiOff:
      // no autopilot - normal settings
      break;
    default:
      // same parameters as AIPilot
      _dirCompensate=0.9;

      //_pilotHelper = true;
      _pilotHelperHeight = true;
      _pilotHelperDir = true;
      _pilotHelperBankDive = true;
      _rudderWanted = 0;

      _forceDive=1;
      
      float aimX(0), aimY(0);
      if (unit->LookAroundEnabled())
      {
        aimX = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse,UAAimRight, UAAimLeft);
        aimY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse,UAAimUp, UAAimDown);
      }
      if(!_copilot)//is looking around in elsewhere
        LookAround(aimX, aimY, deltaT, &Type()->_viewPilot, unit);
      return;
  }
  _dirCompensate=0.5; // low heading compensation
  _forceDive=1;
  if (FutureVisualState()._rpm>0 && _thrust>0)
  {
    CancelStop();
  }

  //Airplane direction pilot
  {
    //_pilotHelper=true;
    _pilotHelperHeight = false;
    _forceDive = 1;

    // keyboard driving
    {
      _pilotHelperDir = false;

      _forceDive=1;
      _pilotHelperDir = false;
      _pilotHelperHeight = false;
      _pilotHelperBankDive = false;
      _pilotBank = 0;        
      //if (!GWorld->HasMap())
      {
        // aim and turn with aiming actions
        float aimX(0), aimY(0);
        if (unit->LookAroundEnabled())
        {
          aimX = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse,UAAimRight, UAAimLeft);
          aimY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse,UAAimUp, UAAimDown);
        }

        if(!_copilot)//is looking around in elsewhere
          LookAround(aimX, aimY, deltaT, &Type()->_viewPilot, unit);
        //rudder
        _rudderWanted = GInput.GetAction(UAHeliRudderLeft)-GInput.GetAction(UAHeliRudderRight);
        float mouseLR = (GInput.GetAction(UAHeliLeft)-GInput.GetAction(UAHeliRight));
        static float coefLRRudder = 1.0f;
#if _ENABLE_CHEATS
        if (CHECK_DIAG(DEMouseSensitivity))
        {
          if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPADPLUS)) coefLRRudder -= 0.1;
          else if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPADMINUS)) coefLRRudder += 0.1;
          saturate(coefLRRudder,0.1, 50);
          DIAG_MESSAGE(500, Format("AirplaneRudder: mouseSensitivity = %.1f", coefLRRudder));
        }
#endif
        _rudderWanted += mouseLR * coefLRRudder;
        //aileron
        _aileronWanted = floatMinMax(GInput.GetAction(UAAirBankLeft)-GInput.GetAction(UAAirBankRight),-1,+1);
        float const aileronCoef = 0.5f;
        _aileronWanted *= aileronCoef; //joystick, keyboard too sensitive
        static float coefLRAileron = 1.0f;
#if _ENABLE_CHEATS
        if (CHECK_DIAG(DEMouseSensitivity))
        {
          if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPADSTAR)) coefLRAileron -= 0.1;
          else if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPAD9)) coefLRAileron += 0.1;
          saturate(coefLRAileron,0.1, 50);
          DIAG_MESSAGE(500, Format("AirplaneAileron: mouseSensitivity = %.1f", coefLRAileron));
        }
#endif
        _aileronWanted += mouseLR * coefLRAileron * 0.25f;
        static float elevatorSpeed = 1.5f;
        _elevatorWanted = elevatorSpeed * Input::MouseRangeFactor * floatMinMax(GInput.GetAction(UAHeliFastForward)+GInput.GetAction(UAHeliForward)-GInput.GetAction(UAHeliBack),-1,+1);
      }
    }
  }

  //Airplane thrust pilot
  {
    
    // when using analogue throttle, do not use thrust autopilot
    // switch between analogue / autopilot based on last input
    
    
    _pilotSpeedX = 0;

    Vector3Val relSpeed=FutureVisualState().ModelSpeed();

    float forward= floatMinMax(GInput.GetAction(UAHeliUp)-GInput.GetAction(UAHeliDown),-1,+1);
    float throttle = GInput.GetAction(UAHeliThrottlePos);
    float brake = GInput.GetAction(UAHeliThrottleNeg);
    float throttleDirect = throttle-brake;
    
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
    // reset the older input - input methods are not compatible, only one must be used at any given time
    _pilotHelperThrust = _analogueThrottleTime<=_thrustHelperTime;

    if (!_pilotHelperThrust)
    {
      _thrustWanted = floatMinMax(throttle,0,1);
      _pilotBrake = floatMinMax(brake,0,1);
    }
    else
    {
      throttle = 0;
    }
    
    if (forward>0 || throttle>0) EngineOn();
    if (Type()->_vtol && _vtolMode)
    {
      // keep the speed as it is
      _pilotSpeed = FutureVisualState().ModelSpeed().Z();
      // in vtolmode we are controlling descend/climb rate instead
      _pilotVertSpeed = forward*5;
    }
    else
    {
      float landingSpeed = Type()->_landingSpeed;
      // when moving slow (landing/taking off) in vtolmode we are controlling descend/climb rate as well
      float landingFactor = Interpolativ(relSpeed.Z(),landingSpeed*0.75f,landingSpeed*1.25f,1,0);
      _pilotVertSpeed = forward*5*landingFactor;
      if( forward<0.1 )
      {
        // automatically stop when moving very slow
        // note: for VTOL aircraft we need to allow negative values when in VTOL mode
        // otherwise controlled descent is not possible
        if( _pilotSpeed<5 ) _pilotSpeed=0;
      }
      if( forward>0 )
      {
        EngineOn();
        if( !_pressedForward ) _pilotSpeed=relSpeed[2],_pressedForward=true;
        _pilotSpeed+=deltaT*20*forward;
      }
      else
      {
        if( _pressedForward ) _pilotSpeed=relSpeed[2],_pressedForward=false;
      }
      if( forward<0 )
      {
        if( !_pressedBack ) _pilotSpeed=relSpeed[2],_pressedBack=true;
        _pilotSpeed+=deltaT*20*forward;
      }
      else
      {
        if( _pressedBack ) _pilotSpeed=relSpeed[2],_pressedBack=false;
      }
    }
    // pilot may want some negative speed - simulation will saturate it when not possible (quite common)
    Limit(_pilotSpeed,-10,GetType()->GetMaxSpeedMs()*1.5f);
  }
  
  Limit(_pilotHeight,0,250);
  
  //Log("Plane v=%f m/s, h=%fm",_pilotSpeed[2],_pilotHeight);

  /**/
  // manual plane does not have taxiing autopilot
  if( _planeState==TaxiIn || _planeState==TaxiOff )
  {
    _planeState=Takeoff;
  }
}

// AI autopilot
class Vector3Path
{
  const Vector3 *_path;
  int _nPath;

  public:
  Vector3Path( const Vector3 *path, int nPath )
  :_path(path),_nPath(nPath)
  {
  }
  float GetCost( Vector3Val pos, Vector3Val dir ) const;
  Vector3 GetPos( float cost, bool &end ) const;
};


/**
@return cost is distance from the beginnig of the path
*/

float Vector3Path::GetCost( Vector3Val pos, Vector3Val dir ) const
{
  if (_nPath<2) return 0;

  float minCost = 1e10f;
  Vector3 nearestPos = pos;
  int nearestI = 0;
  for (int i=1; i<_nPath; i++)
  {
    Vector3Val b = _path[i-1];
    Vector3Val e = _path[i];
    // calculate distance from line path[i-1] path[i]
    Vector3 nearest = NearestPoint(b,e,pos);
    float dist = nearest.Distance(pos);
    float cosAngle = (e-b).CosAngle(dir);
    // having bad orientation is considered a bad penalty
    float cost = InterpolativC(cosAngle,-1,+1,60,0)+dist;
    if (minCost>cost)
    {
      minCost = cost;
      nearestPos = nearest;
      nearestI = i;
    }
  }
  float distAlongPath = 0;
  // calculate cost to point nearestI-1
  for (int i=0; i<nearestI-1; i++)
  {
    Vector3Val b = _path[i];
    Vector3Val e = _path[i+1];
    distAlongPath += b.Distance(e);
  }
  Vector3Val prevPoint = _path[nearestI-1];
  distAlongPath += nearestPos.Distance(prevPoint);
  return distAlongPath;
}

Vector3 Vector3Path::GetPos( float cost, bool &end ) const
{
  for (int i=1; i<_nPath; i++)
  {
    Vector3Val b = _path[i-1];
    Vector3Val e = _path[i];
    float dist = e.Distance(b);
    if (cost<dist)
    {
      end = false;
      return b+(e-b)*(cost/dist);
    }
    cost-=dist;
  }
  end = true;
  return _path[_nPath-1];
}

AirplaneAuto::PathResult AirplaneAuto::PathAutopilot(const Vector3 *path, int nPath)
{
  _pilotGear=true;

  if (nPath<2)
  {
    return PathFinished;
  }

  Vector3Path vpath(path,nPath);

#if _ENABLE_CHEATS
  if (CHECK_DIAG(DEPath))
  {
    for (int i=0; i<nPath; i++)
    {
      Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),VISITOR_NO_ID);
      obj->SetPosition(path[i]);
      obj->SetScale(0.2);
      obj->SetConstantColor(PackedColor(Color(1,1,0)));
      GScene->ShowObject(obj);      
    }
  }
#endif

  float cost = vpath.GetCost(FutureVisualState().Position(),FutureVisualState().Direction());

  bool endOfPath = false;
  Vector3 pos = vpath.GetPos(cost+22,endOfPath);

  bool endOfPathPredict = false;
  Vector3 predTurnPos = vpath.GetPos(cost+220,endOfPathPredict);

  // advance in direction of 

#if _ENABLE_CHEATS
  if (CHECK_DIAG(DEPath))
  {
    {
      Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),VISITOR_NO_ID);
      obj->SetPosition(pos);
      obj->SetScale(0.5);
      obj->SetConstantColor(Color(0,1,1));
      GScene->ShowObject(obj);      
    }

    {
      Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),VISITOR_NO_ID);
      obj->SetPosition(predTurnPos);
      obj->SetScale(0.5);
      obj->SetConstantColor(Color(0,1,0));
      GScene->ShowObject(obj);      
    }
  }
#endif

  // 
  Vector3 relPos=pos-FutureVisualState().Position();

  float distance = predTurnPos.Distance(FutureVisualState().Position());
  // adjust thrust to maintain some speed
  // speed is dependent on relPos size and angle

  PathResult ret = PathGoing;
  if (endOfPath && distance<20)
  {
    _pilotSpeed=0;
    _thrustWanted=0;
    _pilotBrake=1;
    ret = PathFinished;
  }
  else
  {
    _pilotHeading=atan2(relPos.X(),relPos.Z());

    float distFactor =distance>210 ? 1 : distance*(1.0/210);
    _pilotSpeed = 28*distFactor;
    saturateMin(_pilotSpeed,Type()->_landingSpeed*0.5f); // safe under landing speed to prevent unintended take-off
    Vector3 relPredPos = PositionWorldToModel(predTurnPos);

    float turn = 0.9f;
    if (relPredPos.Z()>1)
    {
      turn = fabs(relPredPos.X()*0.1f);


      // predicted turn never requires really low speed
      saturateMin(turn,0.9f);
    }

    // actual turn may required very low speed
    float curHeading = atan2(FutureVisualState().Direction().X(),FutureVisualState().Direction().Z());
    saturateMax(turn,fabs(_pilotHeading-curHeading)*4);

    saturateMin(turn,1);

    saturateMin(_pilotSpeed,turn * 3 + (1-turn)*30);
    
    // avoid too low speed until finished, as speed autopilot would stop the plane
    saturateMax(_pilotSpeed,2.1f);
  }

  _elevatorWanted=0;
  _pilotFlaps=0;

  // check for collision / locked field
  

  float mySize=CollisionSize(); // assume vehicle is not round
  float mySpeedSize=fabs(FutureVisualState().ModelSpeed()[2]);
  // check if we are on collision course
  VehicleCollisionBuffer col;
  float gap = mySize*3;
  float maxDist = Square(mySpeedSize)*0.5+gap;
  float maxTime = 3.5+mySpeedSize*0.2;

  //LogF("%s: Predict gap %.1f, maxDist %.1f",(const char *)GetDebugName(),gap,maxDist);

  GLOB_LAND->PredictCollision(col,this,maxTime,gap,maxDist);
  if( col.Size()>0 )
  {
    // stop and wait until road is clear
    // if vehicle's engine is off stop and terminate
    bool stop = false;

    for (int i=0; i<col.Size(); i++)
    {
      const VehicleCollision &info=col[i];
      const EntityAI *who=info.who;
      if (!who) continue;
      #if COL_DIAG
      LogF
      (
        "%s: col %s",
        (const char *)GetDebugName(),
        (const char *)who->GetDebugName()
      );
      #endif
      // check if vehicle is in front of us
      float relDist = PositionWorldToModel(who->FutureVisualState().Position()).Z();
      if (relDist<=0)
      {
      #if COL_DIAG
        LogF("  not in front");
      #endif
        continue;
      }
      if (!who->EngineIsOn())
      {
      #if COL_DIAG
        LogF("  engine off");
        LogF("  my speed %.1f",ModelSpeed().Z());
        LogF("  tw %.2f, t %.2f",_thrustWanted,_thrust);
      #endif
        stop=true;
        break;
      }
      // we want to keep gap between us
      float relSpeed = DirectionWorldToModel(who->FutureVisualState().Speed()).Z()-2;

      float slower = floatMax((relDist-gap)*0.5,0);
      if (relSpeed>0) relSpeed=0;

      #if COL_DIAG
      LogF("  relSpeed %.1f, slower %.1f",relSpeed,slower);
      #endif
      relSpeed -=slower;
      saturateMax(relSpeed,0);
      if (relSpeed<5) relSpeed=0;

      if (relSpeed<_avoidSpeed || Glob.time>_avoidSpeedTime)
      {
        // if we decide to brake, we will brake for some time
        _avoidSpeed = relSpeed;
        _avoidSpeedTime = Glob.time+1;
      #if COL_DIAG
        LogF("  avoid speed %.2f",_avoidSpeed);
      #endif
      }
    }
    if (stop)
    {
      ret = PathAborted;
      _pilotSpeed=0;
      _thrustWanted=0;
      _pilotBrake=1;

      _avoidSpeed = 0;
      _avoidSpeedTime = Glob.time+3;
    }
  }

  if( Glob.time<_avoidSpeedTime )
  {
    saturateMin(_pilotSpeed,_avoidSpeed);
  }

  /*
        int operX=toInt(Position().X()*InvOperItemGrid);
        int operZ=toInt(Position().Z()*InvOperItemGrid);
        LockCache *locks=GLandscape->LockingCache();
        const int range=50;
          bool lockV=locks->IsLocked(x,z,false);
          bool lockS=locks->IsLocked(x,z,true);
  */

  return ret;
}

bool AirplaneAuto::TaxiOffAutopilot()
{
  // construct path relative to ILS position
  const AutoArray<Vector3> &path = GWorld->GetTaxiOffPath(_targetSide,FutureVisualState().Position(),FutureVisualState().Direction());

  return PathAutopilot(path.Data(),path.Size())<=PathAborted;
}

bool AirplaneAuto::TaxiInAutopilot()
{
  
  const AutoArray<Vector3> &path = GWorld->GetTaxiInPath(_targetSide,FutureVisualState().Position(),FutureVisualState().Direction());
  return PathAutopilot(path.Data(),path.Size())<=PathFinished;
}

float AirplaneAuto::MakeAirborne(bool playerIn)
{
  return MakeAirborne(FutureVisualState().Direction(), playerIn);
}

float AirplaneAuto::MakeAirborne(Vector3Par dir, bool playerIn)
{
  _pilotSpeed=floatMax(GetType()->GetMaxSpeedMs()*0.66f,Type()->_landingSpeed);
  _pilotHeight=_defPilotHeight;
  FutureVisualState()._rpm=1;
  _thrust=_thrustWanted=0.5;

  FutureVisualState()._speed=dir*_pilotSpeed;
  // make sure model speed is set, so that 1st simulation step has already correct values
  FutureVisualState()._modelSpeed = Vector3(0,0,_pilotSpeed);
  FutureVisualState()._flaps=0;
  FutureVisualState()._gearsUp = 1;
  _planeState=Flight;
  _pilotGear=false;
  _pilotFlaps = 0;
  FutureVisualState()._cabinPos = 0;
  _landContact = false;

  EngineOn();
  CancelStop();

  return _pilotHeight;
}

/*!
\patch 1.02 Date 7/11/2001 by Ondra.
- Fixed: Mavericks released from correct position.
\patch 1.07 Date 7/20/2001 by Ondra.
- New: Rocket support for airplanes.
*/

bool AirplaneAuto::ProcessFireWeapon(
  const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target,
  const RemoteFireWeaponInfo *remoteInfo, RemoteFireWeaponInfo *localInfo, bool forceLock
)
{
  const MagazineType *aInfo = magazine ? magazine->Type() : NULL;
  const AmmoType *ammo = ptr.GetAmmoType();
  if (!ammo) return false;
  const VisualState &vs = RenderVisualState();
  if (localInfo)
  {
    localInfo->_position = vs.Position();
    localInfo->_visible=floatMax(ammo->visibleFire,ammo->audibleFire);
  }
  switch (ammo->_simulation)
  {
  case AmmoShotRocket:
  case AmmoShotMissile:
    if (remoteInfo) return false;
    if (ammo->maxControlRange<10)
    {
      _rocketLRToggle=!_rocketLRToggle;
      Vector3Val pos=( _rocketLRToggle ? Type()->_rocketLPos : Type()->_rocketRPos );
      vs.DirectionModelToWorld(localInfo->_direction, Type()->_rocketDir);
      return FireMissile(
        context, ptr,
        pos,localInfo->_direction,Vector3(0,0,aInfo->_initSpeed),
        target, true, false, forceLock
        );
    }
    else
    {
      // find corresponding proxy position
      int count = GetLastMissileIndex(context, ptr);
      bool found;
      Matrix4Val pos = FindMissilePos(count,found, ammo->maverickweaponIndexOffset);
      vs.DirectionModelToWorld(localInfo->_direction, pos.Direction());
      return FireMissile(
        context, ptr,
        pos.Position(),localInfo->_direction,Vector3(0,0,aInfo->_initSpeed),
        target, true, false, forceLock
        );
    }
  case AmmoShotBullet:
  case AmmoShotSpread:
    {
      {
        Matrix4Val shootTrans = context._turretType ? GunTurretTransform(vs, *context._turretType) : M4Identity;
        Vector3 pos;
        Vector3 dir;
        if (remoteInfo) dir = remoteInfo->_direction;
        if (ptr.GetWeaponType()->_shotFromTurret)
        {
          pos = context._turretType ? context._turretType->GetTurretPos(shootTrans) : Type()->_gunPos;
          if (!remoteInfo)
          {
            vs.DirectionModelToWorld(dir, context._turretType ? context._turretType->GetTurretDir(shootTrans) : VForward);
            localInfo->_direction = dir;
          }
        }
        else
        {
          pos = shootTrans.FastTransform(context._turretType ? context._turretType->_gunPos : Type()->_gunPos);
          if (!remoteInfo)
          {
            vs.DirectionModelToWorld(dir , shootTrans.Rotate(context._turretType ? context._turretType->_gunDir : Type()->_gunDir));
            localInfo->_direction = dir;
          }
        }

        return FireMGun(context, ptr, pos, dir, target, false, remoteInfo!=NULL);
      }
/*      
      Matrix4Val shootTrans = GunTransform();
      if (remoteInfo)
      {
        Vector3 aimtDirection = remoteInfo->_direction;

        return FireMGun(
          context, weapon,
          shootTrans.FastTransform(Type()->_gunPos),
          aimtDirection,
          target, false, true
        );
      }
      else
      {
        Vector3 aimtDirection = shootTrans.Rotate(Type()->_gunDir);

        vs.DirectionModelToWorld(localInfo->_direction, aimtDirection);
        return FireMGun(
          context, weapon,
          shootTrans.FastTransform(Type()->_gunPos),
          localInfo->_direction,
          target, false, false
        );
      }
*/  
    }
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

bool AirplaneAuto::FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock)
{
  if (GetNetworkManager().IsControlsPaused()) return false;

  if (!ptr.IsValid()) return false;
  if( !GetWeaponLoaded(*context._weapons, ptr) ) return false;
  if( !IsFireEnabled(context) ) return false;

  const Magazine *magazine = ptr.GetMagazine();
  if (!magazine) return false;

  RemoteFireWeaponInfo local;
  bool fired = ProcessFireWeapon(context, ptr, magazine, target, NULL, &local, forceLock);
  if( fired )
  {
    base::PostFireWeapon(context, ptr, target, local);
    return true;
  }
  return false;
}

bool AirplaneAuto::FireWeaponEffects(
  const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target, const RemoteFireWeaponInfo *remoteInfo
)
{
  const AmmoType *ammo = ValidateFireWeaponWithAmmo(ptr, magazine);
  if (!ammo) return false;

  if (EnableVisualEffects(SimulateVisibleNear)) switch (ammo->_simulation)
  {
    case AmmoShotRocket:
    case AmmoShotMissile:
    case AmmoNone:
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
        if (duration > 0 && intensity > 0) context._weapons->_mGunFire.Start(duration, intensity, true);
      }
      context._weapons->_mGunFireUntilFrame = Glob.frameID+1;
      context._weapons->_mGunFireTime = Glob.time;
      int newPhase;
      while ((newPhase = toIntFloor(GRandGen.RandomValue() * 3)) == context._weapons->_mGunFirePhase);
      context._weapons->_mGunFirePhase = newPhase;
      break;
  }

  return base::FireWeaponEffects(context, ptr, magazine,target,remoteInfo);
}

// AI interface

float AirplaneType::GetFieldCost( const GeographyInfo &info, CombatMode mode ) const
{
  float cost=1;

  // slightly prefer roads
  if( info.u.road ) cost*=1/1.05f;
  return cost;
}

float AirplaneType::GetBaseCost(const GeographyInfo &geogr, bool operative, bool includeGradient) const
{
  float cost=GetMinCost(); // basic speed
  // water is low and therefore following water is usually safer and quicker
  if (geogr.u.minWaterDepth>0) cost*=0.8f; // water everywhere
  else if (geogr.u.maxWaterDepth>0) cost*=0.9f; // water somewhere
  if (includeGradient)
  {
    // penalty for hills
    int grad = geogr.u.gradient;
    if( grad>5 ) grad=5;
    static const float gradPenalty[6]={1.0,1.1,1.2,1.3,1.6,2.5};
    cost *= gradPenalty[grad];
  }
  return cost;
}


float AirplaneType::GetAirCost(float x, float z) const
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

float AirplaneType::GetGradientPenalty(float gradient) const
{
  // TODO: continuous function, sign of gradient
  gradient = fabs(gradient);

  if (gradient >= 0.40f) return 2.5f; // level 5
  else if (gradient >= 0.25f) return 1.6f; // level 4
  else if (gradient >= 0.15f) return 1.3f; // level 3
  else if (gradient >= 0.10f) return 1.2f; // level 2
  else if (gradient >= 0.05f) return 1.1f; // level 1
  return 1.0f; // level 0
}

float AirplaneType::GetCostTurn(int difDir, int &debet, bool forceFullTurn) const
{ // in sec
  if( difDir==0 ) return 0;
  // convert argument into -8..7 range (old direction representation)
  float aDir=fabs(difDir)*(float(15)/255);
  float cost=aDir*0.15+aDir*aDir*0.02;
  if( difDir<0 ) return cost*0.8;
  return cost;
}

float AirplaneAuto::FireInRange(const TurretContext &context, const WeaponModePtr &ptr, float &timeToAim, const Target &target) const
{
  Vector3 relPos = target.AimingPosition()-FutureVisualState().Position();
  // plane should prefer targets that are near the center
  float cosAngle = relPos.CosAngle(FutureVisualState().Direction());
  // it should prefer targets not very near, as aiming them is difficult
  float distance = relPos.Size();

  // for MGun or unguided rockets ideal target distance is 1000
  // add 10 sec for each 1000 m for distance above 1000
  // add 30 sec for each 1000 m for distance below 1000 (30 sec for dist 0)
  // TODO: relax for guided missiles?
  float timeToAimDistance = distance>1000 ? (distance-1000)*(10.0f/1000) : (1000-distance)*(30.0f/1000);
  saturate(timeToAimDistance,0,30);
  // add 15 sec for 90 degree or more turn
  float timeToAimAngle = (1-floatMax(cosAngle,0))*15;
  timeToAim = timeToAimAngle+timeToAimDistance;

  if (ptr != context._weapons->ValidatedCurrentWeapon())
  {
    // assume switching mode takes a little time
    timeToAim += 0.25f;
  }

  return 1;
}

#if _ENABLE_AI

void AirplaneAuto::AIGunner(TurretContextEx &context, float deltaT )
{
}

#endif //_ENABLE_AI

void AirplaneAuto::MoveWeapons( float deltaT )
{
  float delta;
  float speed;
  speed=(_gunXRotWanted-_gunXRot)*8;
  const float maxA=10;
  const float maxV=5;
  delta=speed-_gunXSpeed;
  Limit(delta,-maxA*deltaT,+maxA*deltaT);
  _gunXSpeed+=delta;
  Limit(_gunXSpeed,-maxV,+maxV);
  _gunXRot+=_gunXSpeed*deltaT;
  Limit(_gunXRot,Type()->_minGunElev,Type()->_maxGunElev);

  speed=AngleDifference(_gunYRotWanted,_gunYRot)*6;
  delta=speed-_gunYSpeed;
  Limit(delta,-maxA*deltaT,+maxA*deltaT);
  _gunYSpeed+=delta;
  Limit(_gunYSpeed,-maxV,+maxV);
  _gunYRot+=_gunYSpeed*deltaT;
  _gunYRot=AngleDifference(_gunYRot,0);
  Limit(_gunYRot,Type()->_minGunTurn,Type()->_maxGunTurn);
  
  MoveCrewHead();
}

#if _ENABLE_AI

/**
Runaway pilot used when pilot wants to eject.
Pilot may either decide to return to base
or find a safe spot nearby.
*/

Vector3 AirplaneAuto::FindRunawayPlace(AIBrain *unit)
{
  // check how urgent the damage is
  // if it is not very urgent, we should head home and attempt landing or eject there
  Vector3 bestPos = FutureVisualState().Position();
  Vector3 ilsPos = bestPos, ilsDir = FutureVisualState().Direction();
  bool ilsSTOL;
  if (GWorld->GetILS(ilsPos,ilsDir,ilsSTOL,_targetSide,Type()->_vtol>0,PreferredAirport())>=0)
  {
    // if we do not find any better place, we will head to the nearest safe airport
    bestPos = ilsPos;
  }
  float rtbLimit = 0.5f+Type()->_ejectDamageLimit*0.5f;
  if (GetTotalDamage()>rtbLimit)
  {
    // if we are damaged too much, we need to find a place very soon
    // we will try to find some nice spot, preferably in front of us
    // we start by finding a nearest suitable place in front of us
    int range = 10;
    Vector3 predictPos = FutureVisualState().Position()+FutureVisualState().Speed().Normalized()*LandGrid*range;
    int scanX = toInt(predictPos.X()*InvLandGrid);
    int scanZ = toInt(predictPos.Z()*InvLandGrid);
    // we calculate a "tendency" based on how much good/bad places are found
    // as we are scanning only geography and exposure, the loop should be quite quick
    Vector3 sumGoodPos = VZero;
    float sumGoodWeight = 0;
    for (int xx=scanX-range; xx<=scanX+range; xx++)
    for (int zz=scanZ-range; zz<=scanZ+range; zz++)
    {
      GeographyInfo geogr = GLandscape->GetGeography(xx,zz);
      // avoid water
      if (geogr.u.maxWaterDepth>0) continue;
      
      // the better the place, the more it influences the result
      // try to find a place which is generally safe for both the plane and the pilot
      float exposure = 0;
      // we do not like ejecting into the forest
      float cost = geogr.u.forest ? 3 : 1;

      // prefer safe areas      
      cost += exposure*GetType()->GetInvArmor();
      
      float suitable = 1/cost;
      
      Vector3 pos(xx*LandGrid,0,zz*LandGrid);
      sumGoodPos += pos*suitable;
      sumGoodWeight += suitable;
      
    }
    if (sumGoodWeight>0)
    {
      bestPos = sumGoodPos*(1.0f/sumGoodWeight);
    }
    
  }
  return bestPos;
}

/*!
Avoid collision. Current plane route determined by _pilotHeading and _pilotSpeed
*/

#define DIAG_COL 0

/*!
\patch 1.90 Date 10/30/2002 by Ondra
- Fixed: Airplanes try to avoid mid-air collisions.
*/

void AirplaneAuto::AvoidCollision()
{
  if (!Airborne()) return;
  AIBrain *agent = PilotUnit();
  if( !agent ) return;

  if (agent->GetPlanningMode() == AIBrain::LeaderDirect) return;
  // if we are stopped, do not try to avoid
  // TODO: some vehicles (esp. men) should go out of way of tanks
  const float maxSpeedStopped=0.05;
  if( fabs(_pilotSpeed)<=maxSpeedStopped ) return;

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
        if (_pilotAvoidHigh<until) _pilotAvoidHigh=until;

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

    if( info.distance>gap ) continue;

    if (info.distance<mySize)
    {
      // collission imminent - who is back needs to brake
      // check if he is in front of me
      if (heIsInFrontOfMe)
      {
        // slow down as much as possible
        _pilotSpeed = Type()->_landingSpeed;
      }
    }
  } // for(i)
}

/**
@return above ground level
@param minAboveObstacle accumulate height above "most risky" obstacle
*/
static void MinSafeAltitude(Vector3Val estPos, Vector3Val oldPos, float &minAboveObstacle)
{
  // first approximation: objects 40 m tall everywhere

  // check the object list
  // if the object is not loaded yet, we do not care about it
  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,estPos,oldPos, 25);
  // never fly closer than a certain limit, to stay safe of unexpected objects

  for (int z=zMin; z<=zMax; z++) for (int x=xMin; x<=xMax; x++)
  {
    // TODO: quick reject objects which lie too far from our trajectory
    const ObjectList &list = GLandscape->GetObjects(z, x);
    minAboveObstacle = floatMax(minAboveObstacle, (list.GetMax().Y() - list.GetMin().Y() + 30));
  }
}

void AirplaneAuto::AvoidObstacles()
{
  // use acceleration to estimate change of position
  // estimate vertical acceleration
  //float estY=Position.Y()+_pilotSpeed*EST_DELTA+_acceleration*EST_DELTA*EST_DELTA*0.5;
  // estimate position after 
  const float estT=2;
  const float accPredict=0.2; // estimate acceleration only partially
  Vector3Val speed = FutureVisualState()._speed;
  Vector3 estPos=FutureVisualState().Position()+speed*estT+0.5*accPredict*estT*estT*FutureVisualState()._acceleration;

  float minAboveObstacle = -FLT_MAX;

  if( speed.SquareSizeXZ()>Square(5) )
  {
    // predict terrain
    Vector3 oldPos = estPos;
    for( int t=1; t<4; t++ )
    {
      float estT=t;
      Vector3 estPos= FutureVisualState().Position()+speed*estT+0.5f*estT*estT*FutureVisualState()._acceleration;
      MinSafeAltitude(estPos,oldPos,minAboveObstacle);
      oldPos = estPos;
    }
  }
  else
    MinSafeAltitude(estPos,estPos,minAboveObstacle);

  saturateMax(_pilotHeight, minAboveObstacle);
}

static int RandomID(AIBrain *agent)
{
  RString name = agent->GetPerson()->GetInfo()._fullName;
  return CalculateStringHashValue(name);
}

static int RandomManeuver(AIBrain *unit, int nManeuvers, float duration, float &manPhase)
{
  int seed = RandomID(unit);
  const float loopDuration = duration*nManeuvers;
  float phase = GRandGen.RandomValue(seed)*loopDuration;
  float t = (Glob.time.toFloat()+phase)*(1/duration);
  Assert(t>=0);
  int tFloor = toIntFloor(t);
  manPhase = t-tFloor;
  return toInt(GRandGen.RandomValue(tFloor)*100000+tFloor)%nManeuvers;
}

/// acquire current bank based on transformation matrix
static inline float BankFromOrientation(Matrix3Par orient)
{
  float invDive = orient.Direction().InvSizeXZ();
  float asideUp = orient.DirectionAside().Y();
  // if bottom-up, return 1 (nonsense)
  if (invDive<=0) return fSign(asideUp);
  return asideUp*invDive;
}

void AirplaneAuto::ResetAutopilot()
{
  // We set state to near. It will go to far automatically (if necessary).
  _state=AutopilotNear;
  //_apDive=0,_apBank=0;
}


void AirplaneAuto::Autopilot(float deltaT, Vector3Par target, Vector3Par direction)
{
  // point we would like to reach
  Vector3Val position=FutureVisualState().Position();
  Vector3 absDistance=target-position;
  //Vector3 absDirection=absDistance+tgtSpeed*4; // "lead target" - est. target position
  Vector3 distance=DirectionWorldToModel(absDistance);

  float bank=BankFromOrientation(FutureVisualState().Orientation());
  float dive=FutureVisualState().Direction().Y();

  const float NearDist = 1000.0f;
  const float AlignedDist = 50.0f;
  
  float sizeXZ2=distance.SquareSizeXZ();
  switch( _state )
  {
    default: //case AutopilotFar:
    {
      Vector3 absDirection=absDistance;
      _pilotHeading=atan2(absDirection.X(),absDirection.Z());
      _pilotHeight=floatMin(100,_defPilotHeight);
      Vector3 relSpeed=DirectionWorldToModel(FutureVisualState()._speed);
      // use special maneuvre for fast braking
      // before using it we need to slow down, though
      if (sizeXZ2<Square(NearDist))
      {
        _state=AutopilotNear;
      }
      
      _pilotSpeedX=0; // no side slips
      _pilotVertSpeed=0; // vertical speed is ignored anyway
      // we assume all planes are braking with approx. the same acceleration
      // this means speed depends only on distance
      // we want 40 m/s at 1000 m distance
      float wantedSpeed = sqrt(sizeXZ2)*(40.0f/1000);
      // make sure we are moving fast enough so that we can perform banked turn
      saturate(wantedSpeed,Type()->_landingSpeed,Type()->GetMaxSpeedMs());
      _pilotSpeed = wantedSpeed;
    }
    break;
    case AutopilotBrake:
      _state = AutopilotNear;
      break;
    case AutopilotNear:
    {
      float dist = sqrt(sizeXZ2);
      Vector3 absDirection=absDistance;
      // slow down near the target
      _pilotHeading=atan2(absDirection.X(),absDirection.Z());

      _pilotSpeedX = 0; // no side slips
      // as we are closing to the target, fly lower
      _pilotHeight = InterpolativC(dist,AlignedDist,NearDist,50,100);
      if (_pilotHeight>_defPilotHeight) _pilotHeight = _defPilotHeight;
      // we can move slow - we will turn mostly using the rudders
      _pilotSpeed = InterpolativC(dist,AlignedDist,NearDist,10,Type()->_stallSpeed);
      // to make sure we are not circling around, when heading difference is too high, we reduce the speed
      // we will rather perform a slow hovering turn, than fly away
      float cosHeadChange = absDirection.CosAngle(FutureVisualState().Direction());
      float reduceTurnSpeed = InterpolativC(cosHeadChange,0,0.5f,0,1);
      _pilotSpeed *= reduceTurnSpeed;
      float surfaceY = GLandscape->SurfaceYAboveWaterNoWaves(FutureVisualState().Position().X(),FutureVisualState().Position().Z());
      _pilotVertSpeed = (_pilotHeight - (FutureVisualState().Position().Y()-surfaceY))*0.2; // vertical speed is ignored anyway
      saturate(_pilotVertSpeed,-2,+2);
      // near enough, speed is not too high
      if (sizeXZ2>Square(NearDist*1.5f))
      {
        _state = AutopilotFar;
      }
      if (sizeXZ2<Square(AlignedDist) && FutureVisualState()._speed.SquareSize()<Square(Type()->_stallSpeed*0.5f))
      {
        _state=AutopilotAlign;
      }
      if (FutureVisualState()._speed.SquareSize()<Square(Type()->_landingSpeed))
      {
        // VTOL planes can turn quite well using rudder controlled thrusters
        // once we are moving slow enough, we want to use VTOL mode controls (bank to control speed)
        // in VTOL mode we will use very efficient braking - using dive
        _vtolMode = true;
      }
    }
    break;
    case AutopilotAlign: case AutopilotReached:
    {
      _pilotHeading=atan2(direction.X(),direction.Z());
      float sizeXZ=sqrt(sizeXZ2);
      float high=Interpolativ(sizeXZ,5,AlignedDist,2,50);
      // control to be there in estT sec
      const float estT=4.0f;
      Vector3Val estPosA=FutureVisualState().Position()+FutureVisualState()._speed*estT+0.5f*estT*estT*FutureVisualState()._acceleration;
      Vector3Val estTgt=target;
      
      //Vector3 tgtPos=DirectionWorldToModel(estTgt-estPos);
      Vector3 tgtPos=DirectionWorldToModel(estTgt-estPosA);

      // if we are nearly aligned and stable, tolerate some inaccuracy
      if( FutureVisualState()._speed.SquareSizeXZ()<2 && fabs(dive)<0.1f && fabs(bank)<0.1f )
      {
        bool both = true;
        if( fabs(tgtPos[2])<2 ) tgtPos[2]=0;
        else both = false;
        if( fabs(tgtPos[0])<2 ) tgtPos[0]=0;
        else both = false;
        if (both) high = 0;
      }
      tgtPos[1] += high;

      if( high<4.0f && FutureVisualState()._speed.SquareSize()<Square(5)) _state=AutopilotReached;

      float highSpeed=Interpolativ(FutureVisualState()._speed.Size(),0.2,20,2,30);
      saturateMax(high,highSpeed);

      // once we are reached, stay reached, stay low

      // always used VTOL mode controls
      _vtolMode = true;
      // wanted speed is derived from the relative position
      _pilotSpeed = tgtPos.Z()*0.1f;
      _pilotSpeedX = tgtPos.X()*0.1f;
      _pilotVertSpeed = tgtPos.Y()*0.3f;
      saturate(_pilotVertSpeed,-2,+2);
      // we may need a significant forward speed, as we are landing into the wind
      saturate(_pilotSpeed,-4,+10);
      saturate(_pilotSpeedX,-4,+4);
      
      if (_state==AutopilotReached && high<4.0f)
      {
        // in position - make sure we are descending
        saturateMin(_pilotVertSpeed,-0.5f);
      }
      
      if( sizeXZ2>Square(90))
      {
        _state=AutopilotNear;
      }
    }
    break;
  }
  Limit(_pilotSpeed,-10,Type()->GetMaxSpeedMs());
}

/*!
\patch 1.08 Date 7/20/2001 by Ondra.
- Fixed: AI Airplane target engaging improved.
\patch 1.33 Date 11/29/2001 by Ondra.
- Fixed: AI Airplane machine gun and LGB target engaging improved.
\patch 1.58 Date 5/17/2002 by Ondra
- Fixed: AI Airplane maneuvering when aiming significantly improved. 
\patch 1.78 Date 7/16/2002 by Ondra
- Improved: AI Airplane machine gun engaging improved.
\patch 5128 Date 2/8/2007 by Ondra
- Fixed: AI sometimes did not takeoff when placed on the runway.
\patch 5148 Date 3/26/2007 by Ondra
- Fixed: AI planes not engaging targets.
\patch 5151 Date 3/28/2007 by Ondra
- Fixed: Improved AI plane reaching waypoints.
\patch 5191 Date 12/3/2007 by Ondra
- Fixed: Airplanes bombing laser designated targets.
*/
void AirplaneAuto::AIPilot(AIBrain *unit, float deltaT )
{
  _dirCompensate=0.9;

  //_pilotHelper = true;
  _pilotHelperHeight = true;
  _pilotHelperBankDive = true;
  _rudderWanted = 0;
  _pilotHelperDir = true;
  _pilotHelperThrust = true;
  _vtolMode = false;

  _forceDive=1;
  // unless overridden, do not vector thrust
  _thrustVectorWanted = 0;

  if (Type()->_vtol>=3)
  {
    UpdateStopMode(unit);
  }
  
  //if( unit->GetState()==AIUnit::Stopping && _landContact || (_planeState!=TaxiOff && _planeState!=AfterTouchdown && _planeState!=TaxiIn) )
  if( unit->GetState()==AIBrain::Stopping )
  {
    if (Type()->_vtol>=3)
    {
      // perform vertical landing at given spot

      // special handling of stop state
      // landing position is in _stopPositon

      Vector3 sPos = _stopPosition;

      // direction - opposite to wind
      Vector3Val windDir = GLandscape->GetWindSpeed();

      float windSize = windDir.Size();

      Vector3 landDir = FutureVisualState().Direction();
      if (windSize>0.5f)
      {
        float windFactor = windSize*0.3f;
        saturate(windFactor,0,0.5f);

        landDir = windDir*windFactor+FutureVisualState().Direction()*(1-windFactor);
        landDir[1] = 0;
        landDir.Normalize();
      }

    
      Autopilot(deltaT,sPos,landDir);
    
      if (_state>=AutopilotAlign)
      {
        _pilotGear = true;
      }
    
      // check if aircraft is already landed
      if (_landContact)
      {
        // make sure we have a proper state once we need it
        _planeState = TaxiIn;
        if (_stopMode == SMLand)
        {
          EngineOff();
          if( !EngineIsOn() )
          {
            // note: Pilot may get out - Brain may be NULL
            unit->OnStepCompleted();
            if( unit->IsFreeSoldier() ) return;
          }
        }
        if (unit->GetState()==AIBrain::Stopping)
        {
          // note: Pilot may get out - Brain may be NULL
          unit->OnStepCompleted();
          if( unit->IsFreeSoldier() ) return;
        }
      }
    }
    else
    {
      // normal (non-VTOL) airplane handling
      if( _landContact && FutureVisualState()._speed.SquareSize()<0.1 && (EngineOff(),!EngineIsOn()))
      {
        unit->OnStepCompleted();
        // note: Brain may be NULL now
        return;
      }
      if( _planeState==Flight )
      {
        // autoselect a suitable airport
        _planeState=Marshall;
        Vector3 ilsPos = FutureVisualState().Position(), ilsDir = FutureVisualState().Direction();
        bool ilsSTOL = false;
        _landAtAirport = GWorld->GetILS(ilsPos,ilsDir,ilsSTOL,TSideUnknown,Type()->_vtol>0);
      }
    }  
  }
  else if (unit->GetState() == AIBrain::Stopped)
  {
    if (Type()->_vtol >= 3 && _stopMode == SMLand)
    {
      EngineOff();
    }
  }
  else if( _planeState==Takeoff )
  {
    // make sure we start the engine so that we can take off
    EngineOn();
    _pilotGear=true;

    Vector3 ilsPos = FutureVisualState().Position(), ilsDir = FutureVisualState().Direction();
    bool ilsSTOL = false;
    // we need to take off from the current airport
    const World::AirportInfo *airport = GWorld->GetAirport(ilsPos,ilsDir,TSideUnknown,Type()->_vtol>0);
    if (airport)
    {
      ilsPos = airport->_ilsPos;
      ilsDir = airport->_ilsDir;
      ilsSTOL = airport->STOL();
    }
    Vector3 azimutDir = FutureVisualState().Direction();
    ilsDir[1] = 0; // glideslope aims into the ground, we are interested about the runway direction instead
    azimutDir[1] = 0; // some planes (Camel) aim upwards or downwards, we want to ignore this
    float cosAngle = -ilsDir.CosAngle(azimutDir);
    const float cos7deg = 0.99254615164132203498f;
    // until aligned, move only slowly
    // but once moving fast, do not slow down to realign
    float speed = FutureVisualState().Speed().Size();
    _pilotSpeed=!_landContact || speed>20 || cosAngle>cos7deg ? Type()->_landingSpeed*2 : 5;

    // aim to a sliding position along the runway centerline
    Vector3 nearestOnCenterline = NearestPointInfinite(ilsPos,ilsPos-ilsDir,FutureVisualState().Position());
    Vector3 ilsCPos = nearestOnCenterline-ilsDir*200;
    Vector3 relPos = ilsCPos-FutureVisualState().Position();
    _pilotHeading=atan2(relPos.X(),relPos.Z());
    if (Type()->_vtol)
    {
      // use thrust vectoring to short takeoff
      // TODO: detect if STOL is necessary
      _thrustVectorWanted = ilsSTOL ? 0.66f : 0;
    }
  }
  else if (_planeState==TaxiIn || _planeState==TaxiOff)
  {
    bool wantToMove = true;
    if (Type()->_vtol >= 3)
    {
      // check whether pilot can stay landed
      const Path &path = unit->GetPath();
      wantToMove = path.Size() >= 2 && path.GetSearchTime()<Glob.time-2;
    }

    // AI try repeatedly start engine when no fuel, fixed: news:hr7pt5$267$1@new-server.localdomain
    if (wantToMove && FutureVisualState()._fuel > 0.0f)
    {
      // Airplane::Simulate autopilot handles this condition
      EngineOn();
      _pilotGear = true;
    }
  }
  else if (_planeState==Flight)
  {
    _pilotGear=false;

    // if we decide we need to eject, we should find a suitable spot
    if (GetRawTotalDamage()>=Type()->_ejectDamageLimit && unit->GetCombatModeLowLevel()>=CMCombat)
    {
      #if _PROFILE
        GlobalShowMessage(100,"%s running away",cc_cast(GetDebugName()));
      #endif
      // we want to eject - find a suitable place
      Vector3 runaway = FindRunawayPlace(unit);
      
      Vector3 runawayDiff = runaway-FutureVisualState().Position();
      _pilotHeading = atan2(runawayDiff.X(),runawayDiff.Z());
      // move as fast as possible
      _pilotSpeed = Type()->GetMaxSpeedMs()*1.5f;

      float minHeight = Type()->GetMaxSpeedMs()<80 ? 30 : 50;
      
      // climb, but keep good speed
      // the faster we fly, the higher we want to be
      _pilotHeight = Interpolativ(
        FutureVisualState().ModelSpeed().Z(),
        Type()->_landingSpeed,Type()->GetMaxSpeedMs(),
        minHeight,_defPilotHeight
      );
      // turn to be a hard target

      float duration = 8.0f;
      int seed = RandomID(unit);
      float phase = GRandGen.RandomValue(seed)*duration;
      float manPhase = Glob.time.Mod(duration)/duration;

      _pilotHeading += sin(manPhase*H_PI*2-phase)*0.15f;
    }
    else if( _sweepTarget )
    {
      EntityAI *sweepTargetAI =  _sweepTarget->idExact;

      float surfaceY=GLOB_LAND->SurfaceYAboveWaterNoWaves(FutureVisualState().Position().X(),FutureVisualState().Position().Z());
      float height=FutureVisualState().Position().Y()-surfaceY;

      _pilotHeight=height;
      saturate(_pilotHeight,50,400);


      _pilotSpeed=Type()->GetMaxSpeedMs()*0.6f;

      Vector3 relPos;
      // use exact weapon aiming calculation
      // TODO: which weapons?
      TurretContext context;
      bool valid = GetPrimaryGunnerTurret(context);
      DoAssert(valid);
      float timeToLead;
      WeaponModePtr ptr = context._weapons->ValidatedCurrentWeapon();
      if (!CalculateAimWeapon(context, ptr,relPos,timeToLead,_sweepTarget,false))
      {
        // if exact calculation failed, used approximate calculation
        float tgtDist = _sweepTarget->AimingPosition().Distance(FutureVisualState().Position());
        float tgtTime = tgtDist*(1.0f/100);
        Vector3 tgtPos = _sweepTarget->AimingPosition()+_sweepTarget->GetSpeed(NULL)*tgtTime;
        relPos = tgtPos-FutureVisualState().Position();
      }
      float distXZ=relPos.SizeXZ();

      // detect situation: he is targeting me
      // if his speed can be compared with mine, dogfighting is required
      bool evade = false;
      // if engaging fast moving targets, different tactics is required
      bool dogfight = sweepTargetAI && sweepTargetAI->FutureVisualState().Speed().SquareSize()>=FutureVisualState().Speed().SquareSize()*Square(0.5);
      // fast planes cannot go too low
      float minHeight = Type()->GetMaxSpeedMs()<80 ? 30 : 50;
      if (dogfight)
      {
        // he is on my six
        Vector3 myPosToHim = FutureVisualState().Position()-sweepTargetAI->FutureVisualState().Position();
        float hisCosAngle = sweepTargetAI->FutureVisualState().Direction().CosAngle(myPosToHim);
        if (hisCosAngle>0.75f && myPosToHim.SquareSizeXZ()<Square(300))
        {
          evade = true;
        }
      }

      bool specManeuver = false;
      float headChange=0;
      if (evade && distXZ<600)
      {
        // when dogfighting, we can do various evasive maneuvers to get some tactical advantage
        // we do it randomly based on time intervals
        float manPhase;
        int maneuver = RandomManeuver(unit,10,10,manPhase);
        #if _ENABLE_CHEATS
          if (GWorld->CameraOn()==this)
          {
            static const char *name[]=
            {
              "Break climb","Break climb",
              "Break dive","Break dive",
              "Turn dive","Turn dive",
              "Climb stall","Climb stall",
              "Run away",""
            };
            GlobalShowMessage(
              500,"%s Man %d (%s): %.3f",
              cc_cast(GetDebugName()),maneuver,name[maneuver],manPhase
            );
          }
        #endif

        // when evading, always move as fast as possible
        // turning will probably slow you down anyway
        _pilotSpeed = Type()->GetMaxSpeedMs();
        switch(maneuver)
        {
          case 0: case 1:// climb and break
            _pilotHeight = floatMin(height+10,500);
            headChange = (maneuver&1) ? +1.3f : -1.3f;
            specManeuver = true;
            break;
          case 2: case 3:// dive and break
            _pilotHeight = floatMax(minHeight,height-10);
            headChange = (maneuver&1) ? +1.5f : -1.5f;
            specManeuver = true;
            break;
          case 4: case 5: // dive and turn
            _pilotHeight = floatMax(minHeight,height-10);
            headChange = (maneuver&1) ? +0.5f : -0.5f;
            specManeuver = true;
            break;
          case 6: case 7:// climb, when near stall, turn
            _forceDive = 0.95f;
            _pilotHeight = floatMin(height+10,500);
            if (FutureVisualState().ModelSpeed().Z()>Type()->_stallSpeed && manPhase<0.6f)
            {
              headChange = (maneuver&1) ? +0.7f : -0.7f;
            }
            else
            {
              headChange = (maneuver&1) ? +2.0f : -2.0f;
            }
            specManeuver = true;
            break;
          case 8: // dive and change direction (run away)
            _pilotHeight = floatMax(minHeight,height-30);
            headChange = sin(manPhase*H_PI*6)*0.3f;
            specManeuver = true;
            break;
        }
      }

      if (CHECK_DIAG(DECombat) && this==GWorld->CameraOn())
      {
        Vector3 toTarget = DirectionWorldToModel(relPos);
        DIAG_MESSAGE(
          100,"Sweep state %d, to target %.1f,%.1f,%.1f, above 2:%.1f,4:%.1f,8:%.1f",
          _sweepState,toTarget[0],toTarget[1],toTarget[2],
          EstimateAboveTerrain(2.0f),EstimateAboveTerrain(4.0f),EstimateAboveTerrain(8.0f)
        );
      
      }


      if (!specManeuver)
      {
        // estimated distance need to turn based on max. speed
        float safeDistance;
        if (dogfight)
        {
          // target is moving anyway - no use to disengage very far
          safeDistance = Type()->GetMaxSpeedMs()*2.0f+100;
        }
        else
        {
          // we need very accurate alignment to hit the target
          safeDistance = Type()->GetMaxSpeedMs()*5.0f+700;
        }
        if( _sweepState==SweepDisengage )
        {
          // select a speed suitable for disengaging


          // consider: limit speed before turning?
          float speedAdd = Interpolativ(distXZ,0,2000,-1,Type()->GetMaxSpeedMs()*0.5f);
          _pilotSpeed = speedAdd;
          saturateMax(_pilotSpeed,Type()->_landingSpeed*1.2f);

          // this is 
          Vector3 tgtDir=PositionWorldToModel(_sweepTarget->AimingPosition());
          float headChangeToTarget=atan2(tgtDir.X(),tgtDir.Z());
          // you can disengage in any direction
          // TODO: select suitable disengage direction
          if (dogfight)
          {
            // disengage to his left
            Vector3 aimDir = DirectionWorldToModel(-sweepTargetAI->FutureVisualState().DirectionAside());
            headChange=atan2(aimDir.X(),aimDir.Z());
          }
          else
          {
            headChange = 0;
          }
          
          {// avoid flying straight, as this would make us an easy target
            int seed = RandomID(unit);
            const float period = 4.0f;
            // compute -1..1 saw with given period 
            // fabs computes 0..1 saw, *2-1 makes -1..1 of it
            float saw = fabs(fastFmod(Glob.time.toFloat()+seed,period)*(2.0f/period)-1)*2-1;
            // do not turn much, only a little bit
            headChange += saw*0.3f;
          }

          const float predictTime = 2;
          Vector3 predictedPos = _sweepTarget->GetPosition()+_sweepTarget->GetSpeed(NULL)*predictTime;
          float predictedDistXZ2 = predictedPos.DistanceXZ2(FutureVisualState().Position());

          // while disengaging gain altitude slowly
          _pilotHeight = floatMin( minHeight+distXZ*(1.0f/500)*(100-minHeight),500);

          float maxHeadingChange = dogfight ? 0.5f : 0.25f;
          if( predictedDistXZ2>Square(safeDistance) || fabs(headChangeToTarget)<maxHeadingChange || evade)
          {
            if (!dogfight || evade || distXZ>=200 || fabs(headChangeToTarget)<maxHeadingChange)
            {
              _sweepState=SweepEngage;
              _sweepDelay=Glob.time+25;
              #if _ENABLE_REPORT
              if (evade)
              {
                LogF("%s: evading %s",(const char *)GetDebugName(),(const char *)sweepTargetAI->GetDebugName());
              }
              #endif
              #if LOG_SWEEP
                LogF
                (
                  "%s: %.1f sweep engage (%s), dist %.0f, dogfight %d, evade %d",
                  (const char *)GetDebugName(),Glob.time-Time(0),
                  sweepTargetAI ? (const char *)sweepTargetAI->GetDebugName() : "<null>",
                  distXZ,dogfight,evade
                );
              #endif
            }
          }
        }
        else if( _sweepState==SweepEngage )
        {
          WeaponModePtr ptr = context._weapons->ValidatedCurrentWeapon();
          // target speed required
          // in dogfight avoid overshooting (getting in front of him)

          _sweepDir = relPos;
          Vector3 wantedDir = _sweepDir;
          Vector3 diveDir = _sweepDir;
          if (dogfight)
          {
            _pilotHeight=_sweepTarget->GetPosition().Y()-surfaceY;
            saturateMax(_pilotHeight,minHeight);
          }

          Matrix3 orientUp;
          orientUp.SetUpAndDirection(VUp,FutureVisualState().Direction());
          Vector3Val aimDir = orientUp.InverseRotation()*wantedDir;

          headChange=atan2(aimDir.X(),aimDir.Z());

          // if we are far, apply more than target speed
          float slowDownInTurn = InterpolativC(headChange,0,H_PI/2,0,1);
          float speedAdd = Interpolativ(distXZ,0,2000,-1,Type()->GetMaxSpeedMs()*(0.3f+0.2f*slowDownInTurn));
          _pilotSpeed = _sweepTarget->GetSpeed(NULL).Size()+speedAdd;
          // if we are turning, limit the speed
          saturateMax(_pilotSpeed,Type()->_landingSpeed*1.2f);
          
          // check if target is in front  of us
          const float offTreshold = H_PI*0.1f;
          if (dogfight && fabs(headChange)>offTreshold)
          {
            // target not in front of us - try some offensive maneuver
            float manPhase;
            int maneuver = RandomManeuver(unit,4,6,manPhase);
            #if _ENABLE_CHEATS
              if (GWorld->CameraOn()==this)
              {
                static const char *name[]=
                {
                  "High yoyo",
                  "Low yoyo",
                  "Turn",
                  ""
                };
                GlobalShowMessage(
                  500,"Off %d (%s): %.3f",
                  maneuver,name[maneuver],manPhase
                );
              }
            #endif
            switch (maneuver)
            {
              case 0: // high yoyo
                headChange += fSign(headChange)*floatMin(1,(fabs(headChange)-offTreshold)*4);
                _pilotHeight = height+15;
              break;
              case 1: // low yoyo
                headChange += fSign(headChange)*floatMin(1,(fabs(headChange)-offTreshold)*4);
                _pilotHeight = floatMax(minHeight,height-15);
                break;
              case 2: // level overturn
                headChange += fSign(headChange)*floatMin(1,(fabs(headChange)-offTreshold)*4);
                break;
            }
          }
          // when plane turn, it is banking
          // this would change relative position in x-direction
          // to counteract this, we create orientation from direction and vertical
          //LogF("Sweep headc %.2f, aimDir %.2f,%.2f",headChange,aimDir.X(),aimDir.Z());
          
          // aggresive maneuver
          // if we are far, maneuver a little bit mor aggesive

          saturate(headChange,-H_PI*0.8f,+H_PI*0.8f);

          // move - to be hard target
          if( distXZ<200 && !dogfight)
          {
            _sweepState=SweepFire;
            _sweepDelay=Glob.time+5; // start immediatelly

            #if LOG_SWEEP
              LogF
              (
                "%s: %.1f sweep fire (%s)",
                (const char *)GetDebugName(),Glob.time-Time(0),
                sweepTargetAI ? (const char *)sweepTargetAI->GetDebugName() : "<null>"
              );
            #endif
          }

          float minZDist = dogfight ? 30 : 100;
          if
          (
            aimDir.Z()>minZDist &&
            ( fabs(aimDir.X())<12.0f || fabs(aimDir.X())<aimDir.Z()*0.035f )
          )
          {
            // if the target is horizontally aimed, make vertical adjust
            // check if evasion is necessary - avoid equal opportunity situations
            if (dogfight && evade && distXZ<200)
            {
              // need to evade - otherwise crash or being hit is possible
              _sweepState=SweepDisengage;
              _sweepDelay=Glob.time+10; // start immediately
            }
            // if we are high enough, aim vertically
            // check if we are in weapons and visibility range
            float minFireDist = Glob.config.horizontZ;
            WeaponModePtr ptr = context._weapons->ValidatedCurrentWeapon();
            if (ptr.IsValid())
            {
              const WeaponModeType *mode = ptr.GetWeaponModeType();
              if (mode)
              {
                saturateMin(minFireDist,mode->maxRange);
              }
            }
          

            float engageDive = (1.0f/500)*(130-minHeight);
            float desiredAGL = floatMin( minHeight+20+distXZ*engageDive,500);
            // compensate for different terrain ASL on the approach path
            Vector3 tgtPos = relPos+FutureVisualState().Position();
            Vector3 halfToSafePoint = tgtPos-safeDistance*0.5f*relPos.Normalized();
            float desiredASL = tgtPos.Y()+desiredAGL;
            _pilotHeight = floatMax(desiredASL-GLandscape->SurfaceY(halfToSafePoint),minHeight+distXZ*engageDive*0.25f);
            
            if (aimDir.Z()<minFireDist && height>minHeight && sweepTargetAI)
            { // we are flying high enough
              // actual aiming
              bool headOnly = false;
              WeaponModePtr ptr = context._weapons->ValidatedCurrentWeapon();
              float visible = _visTracker.KnownValue(
                headOnly,
                this, context, ptr, sweepTargetAI
              );
              if (visible>0.6f)
              {
                // adapt forceDive to aim target

                float rawDive = diveDir.Y()*diveDir.InvSizeXZ();
                _forceDive = rawDive - Type()->_gunDir.Y();
                
                {
                  // slight randomization in up/down direction
                  // to counteract systematic errors
                  // we prefer firing slightly below - we are moving forward
                  // and the bullet impact point moves forward/up as well
                  // this is particularly true for planes with gun aiming slightly down (A10)
                  const float adjustM = 0.002f;
                  const float adjustA = -0.001f;
                  float perAngle = Glob.time.ModMs(2000)*((1.0f/2000)*(H_PI*2));
                  float randomFactor = sin(perAngle+RandomID(unit)*(H_PI/4)); 
                  float adjustAngle = randomFactor*adjustM+adjustA;
                  // we need to consider the fact plane model speed might be misaligned (aiming up when flaps are used)
                  float speedDive = -FutureVisualState().ModelSpeed().Y()/FutureVisualState().ModelSpeed().Z();
                  _forceDive += adjustAngle + speedDive*(randomFactor*0.8f+0.2f);
                }
                // 
                //LogF("Dive raw %.3f, act %.3f",rawDive,_forceDive);
                
                float minDive = dogfight ? -0.7f : -0.4f;
                // avoid diving too steep, especially when flying fast and close to the ground
                float refHeight = FutureVisualState().ModelSpeed().Z()*3.6f; // for 300 km/h refHeight is 300 m
                float minDiveByHeight = Interpolativ(height,refHeight*0.25f,refHeight,-0.1f,-0.7f);
                // avoid climbing too steep to avoid stalling
                float maxDive = dogfight ? +0.3f : +0.1f; 
                // positive dive is up
                saturate(_forceDive,floatMax(minDive,minDiveByHeight),maxDive);

                //LogF("  force dive %.2f, %.2f",_forceDive,Direction().Y());
                // sweep target aligned: force it as fire target
              // override stall avoidance
                _pilotHeight = height;
            }
          }
          }
          else
          { // we are far from the target yet, and not using LGB
            // while far we prefer to overreact slightly in the heading
            // to make sure we are aligned soon
            headChange *= 1.5f;
            // but still keep the change reasonable
            saturate(headChange,-H_PI*0.8f,+H_PI*0.8f);
            if (distXZ>1200)
            {
              // climb while you can
              _pilotHeight = 200;
            }
            // FIX
            else if (height>minHeight)
            {
              // avoid diving  - we need elevator for smooth turn
              if (!dogfight)
              {
                _forceDive = 0;
              }
              //LogF("  force no dive %.2f",_forceDive);
            }
          }
        }
        else if( _sweepState==SweepFire )
        {
          Matrix3 orientUp;
          orientUp.SetUpAndDirection(VUp,FutureVisualState().Direction());
          Vector3Val aimDir = orientUp.InverseRotation()*_sweepDir;

          headChange=atan2(aimDir.X(),aimDir.Z());
          _pilotSpeed = _sweepTarget->GetSpeed(NULL).Size();
          saturateMax(_pilotSpeed,Type()->_landingSpeed*1.2f);

          if(
            !evade
            && (
              fabs(headChange)>0.6f && ( distXZ<100 || distXZ>300 ) // no aligned and not within a good distance
              || FutureVisualState().Direction().CosAngle(relPos)<0 // passed over
              || EstimateAboveTerrain(4.0f)<20 // terrain collision imminent
            )
          )
          {
            _sweepState=SweepDisengage;
            _sweepDelay=Glob.time+10; // start immediately

            #if LOG_SWEEP
              LogF
              (
                "%s: %.1f sweep disengage (%s)",
                (const char *)GetDebugName(),Glob.time-Time(0),
                sweepTargetAI ? (const char *)sweepTargetAI->GetDebugName() : "<null>"
              );
            #endif
          } 
          else
          {
            // TODO: if the target is already out of reach (e.g. too low), no need to dive for it any more
            float minSpeed=Type()->_landingSpeed*1.2f;
            float speed=FutureVisualState().ModelSpeed()[2];
            if( speed>minSpeed && aimDir.Z()>20 && fabs(headChange)<0.1f && height>minHeight)
            { // we are flying fast enough, high enough, and we are aligned well enough
              // actual aiming (using dive)
              _forceDive=(relPos.Y()-3)*relPos.InvSizeXZ();
              // limit diving
              // we want to stay at least minHeight above the target
              // as closing in height, we want to limit the dive, depending on our current speed
              float fullDive = FutureVisualState().ModelSpeed().Z()*3.6f;
              // i.e. -relPos.Y()>=minHeight
              float above = -relPos.Y()-minHeight;
              float limitDive = Interpolativ(above,0,fullDive,-0.1f,-0.7f);
              saturate(_forceDive,limitDive,0);
            }
          }
          // limit heading change - we do not want to perform any crazy maneuvers
          // if we are not aligned, we have no chance of hitting anyway
          saturate(headChange,-0.1f,+0.1f);

        }
        // if we are flying too fast and unable to brake, we need to limit diving
        float maxSpeed = Type()->GetMaxSpeedMs();
        if (!Type()->_hasAirbrake && FutureVisualState().ModelSpeed().Z()>maxSpeed)
        {
          float maxDive = Interpolativ(FutureVisualState().ModelSpeed().Z(),maxSpeed,maxSpeed*1.1,1,0);
          saturateMax(_forceDive,-maxDive);
        }
      }
      float actHeading = atan2(FutureVisualState().Direction().X(),FutureVisualState().Direction().Z());
      _pilotHeading=actHeading+headChange;
      /*
      LogF
      (
        "PH %.2f, AH %.2f, PH-AH %.2f",
        _pilotHeading,actHeading,AngleDifference(_pilotHeading,actHeading)
      );
      */

      // no limits during sweep
      _limitSpeed=GetType()->GetMaxSpeedMs()*1.5f;
      _limitSpeedForced=GetType()->GetMaxSpeedMs()*1.5f;
      // check if target is alive
      if (!sweepTargetAI || _sweepTarget->State(unit)<TargetAlive)
      {
        _sweepTarget=NULL;
      }
      if( Glob.time>_sweepDelay ) _sweepTarget=NULL;
    } // sweep 
    else
    {
      float speedWanted=0;
      // normal flight
      Vector3 destination=VZero;
      {
        // formation - leader

        speedWanted=floatMin(_limitSpeed, _limitSpeedForced); // go faster
#if _VBS3
        if (unit->GetForceSpeed()>=0)
        {
          saturateMin(speedWanted, unit->GetForceSpeed()/3.6);
        }
#endif        
        #if DIAG_SPEED
        if( this==GWorld->CameraOn() )
        {
          LogF("Basic speed %.1f",speedWanted*3.6);
        }
        #endif

        // take path destination
        destination=SteerPoint(4.0,6.0);
        //AvoidCollision(steerPos);
        
        // check path position
        const Path &path=unit->GetPath();
        if( path.Size()>=2 )
        {
          if (path.GetSearchTime()<Glob.time-2)
            EngineOn();

          if (unit->GetPlanningMode() != AIBrain::LeaderDirect)
          {
            float cost=path.CostAtPos(FutureVisualState().Position());
            Vector3 pos=path.PosAtCost(cost,0,FutureVisualState().Position(),cost);

            float dist2 = FutureVisualState().Position().DistanceXZ2(pos);
            float distEnd2 = FutureVisualState().Position().DistanceXZ2(path.End());
            float precision = GetPrecision();
            // check if we have first point of Plan complete
            if (distEnd2<Square(precision) || cost>path.EndCost())
            {
              unit->OnStepCompleted();
              //            unit->ForceReplan();
            }
            if (dist2>Square(600))
            {
              unit->OnStepTimedOut();
            }
          }
        }
        Vector3 relDest=destination-FutureVisualState().Position();
        // avoid slowing down when target is close - angles are ill defined in such case
        if (relDest.SquareSize()>GetPrecision()*0.75f)
        {
          // if we need to turn, we should slow down
          Vector3 turnVec = DirectionWorldToModel(relDest);
          float turnRate = fabs(turnVec.X())<turnVec.Z() ? fabs(turnVec.X())/turnVec.Z() : 1;
          float turnSpeed = Lerp(Type()->GetMaxSpeedMs()*1.5f,Type()->_landingSpeed*1.5f,turnRate);
          if (speedWanted>turnSpeed) speedWanted = turnSpeed;
        }
        float minSpeed = Type()->_landingSpeed*0.87f;
        saturate(speedWanted,minSpeed,Type()->GetMaxSpeedMs()*1.5f);
        _pilotSpeed=speedWanted;
        
        _pilotHeading=atan2(relDest.X(),relDest.Z());
        _pilotHeight=_defPilotHeight;
      }
    } // normal flight

    //avoid skycrapers (Seattle downtown)
    AvoidObstacles();
    // basic collision avoidance
    AvoidCollision();

    if (Glob.time<_pilotAvoidHigh)
    {
      saturateMax(_pilotHeight,_pilotAvoidHighHeight);
    }
    else
    {
      _pilotAvoidHighHeight = 0;
    }
    if (Glob.time<_pilotAvoidLow)
    {
      saturateMin(_pilotHeight,_pilotAvoidLowHeight);
      saturateMax(_pilotHeight,30);
    }
    else
    {
      _pilotAvoidLowHeight = 100000;
    }


  }
}
#endif // _ENABLE_AI

void Airplane::GetAimActions(float &aimX, float &aimY) const
{
  aimX = GInput.GetActionWithCurve(InputInterface::CurveFovMouse,UAAimRight, UAAimLeft);
  aimY = GInput.GetActionWithCurve(InputInterface::CurveFovMouse,UAAimUp, UAAimDown);
}

UserAction Airplane::GetManualFireAction() const
{
  return UAHeliManualFire;
}

#define SERIAL_DEF_VISUALSTATE(name, value) CHECK(ar.Serialize(#name, FutureVisualState()._##name, 1, value))

LSError Airplane::Serialize(ParamArchive &ar)
{
  SERIAL_BASE

  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    SERIAL_DEF_VISUALSTATE(thrustVector, 0);
    SERIAL_DEF_VISUALSTATE(rotorPosition, 0);
    SERIAL_DEF_VISUALSTATE(elevator, 0);
    SERIAL_DEF_VISUALSTATE(rudder, 0);
    SERIAL_DEF_VISUALSTATE(aileron, 0);
    SERIAL_DEF_VISUALSTATE(flaps, 0);
    SERIAL_DEF_VISUALSTATE(gearsUp, 0);
    SERIAL_DEF_VISUALSTATE(brake, 0);
    SERIAL_DEF_VISUALSTATE(cabinPos, 0);
    SERIAL_DEF_VISUALSTATE(rpm, 1); // on/off
    SERIAL_DEF(thrust, 0);
    SERIAL_DEF(thrustWanted, 0); // turning motor on/off
    SERIAL_DEF(thrustVectorWanted, 0);
    SERIAL_DEF(rotorSpeed, 0);
    SERIAL_DEF(elevatorWanted, 0);
    SERIAL_DEF(rudderWanted, 0);
    SERIAL_DEF(aileronWanted, 0);
    SERIAL_DEF(pilotFlaps, 0);
    SERIAL_DEF(pilotBrake, 0);
    SERIAL_DEF(pilotGear, true);
    SERIAL_DEF(vtolMode, true);
  }
  return LSOK;
}

#undef SERIAL_DEF_VISUALSTATE

LSError AirplaneAuto::Serialize(ParamArchive &ar)
{
  SERIAL_BASE
  // TODO: serialize _planeState using EnumName
  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    CHECK(ar.Serialize("planeState", *(int *)&_planeState, 1, 1 ))
    SERIAL_DEF(homeAirport, -1);
    SERIAL_DEF(landAtAirport, -1);
    SERIAL_EXT_DEF(pilotAvoidHigh, TIME_MIN);
    SERIAL_DEF(pilotAvoidHighHeight, 0);
    SERIAL_EXT_DEF(pilotAvoidLow, TIME_MIN);
    SERIAL_DEF(pilotAvoidLowHeight, 100000);

    CHECK(::Serialize(ar, "stopPosition", _stopPosition, 1, VZero))
    CHECK(ar.SerializeEnum("stopMode", _stopMode, 1, SMNone))
    CHECK(ar.SerializeEnum("stopResult", _stopResult, 1, SPNotReady))

    CHECK(ar.Serialize("defPilotHeight", _defPilotHeight, 1, 50))
    CHECK(ar.Serialize("pilotHeight", _pilotHeight, 1, 2.5))
  }

  return LSOK;
}

#define AIRPLANE_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateAirplane) \
  XX(UpdatePosition, UpdatePositionAirplane)

DEFINE_NETWORK_OBJECT(AirplaneAuto, base, AIRPLANE_MSG_LIST)

//! network message indices for AirplaneAuto class
/*!
\patch 1.01 Date 6/26/2001 by Ondra.
- Fixed: Added more transfered properties in MP for Airplanes.
*/

#define UPDATE_AIRPLANE_MSG(MessageName, XX) \
  XX(MessageName, int, pilotFlaps, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, 0), DOC_MSG("Position of flaps, wanted by pilot"), TRANSF, ET_ABS_DIF, ERR_COEF_MODE) \
  XX(MessageName, bool, gearDamage, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Gear is damaged"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, pilotGear, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Position of gear, wanted by pilot"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, vtolMode, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("VTOL mode on/off toggle"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, int, planeState, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, TaxiIn), DOC_MSG("Autopilot state"), TRANSF_ENUM, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, int, stopMode, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, 0), DOC_MSG("Landing type"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, int, stopResult, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, PlaneOrHeli::SPNotReady), DOC_MSG("Landing result"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, Vector3, stopPosition, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, VZero), DOC_MSG("Landing position"), TRANSF, ET_ABS_DIF, 0.1) \


DECLARE_NET_INDICES_EX_ERR(UpdateAirplane, UpdateTransport, UPDATE_AIRPLANE_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateAirplane, UpdateTransport, UPDATE_AIRPLANE_MSG, NoErrorInitialFunc)

#define UPDATE_POSITION_AIRPLANE_MSG(MessageName, XX) \
  XX(MessageName, float, thrustWanted, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 0), DOC_MSG("Wanted engine thrust"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, thrustVectorWanted, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 0), DOC_MSG("Wanted thrust vectoring"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, elevatorWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted elevator position"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, rudderWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted rudder position"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, aileronWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted aileron position"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, pilotBrake, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 0), DOC_MSG("State of brake, wanted by pilot"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, pilotSpeed, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Speed wanted by pilot"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR)

DECLARE_NET_INDICES_EX_ERR(UpdatePositionAirplane, UpdatePositionVehicle, UPDATE_POSITION_AIRPLANE_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionAirplane, UpdatePositionVehicle, UPDATE_POSITION_AIRPLANE_MSG, NoErrorInitialFunc)

NetworkMessageFormat &AirplaneAuto::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_AIRPLANE_MSG(UpdateAirplane, MSG_FORMAT_ERR)
    break;
  case NMCUpdatePosition:
    base::CreateFormat(cls, format);
    UPDATE_POSITION_AIRPLANE_MSG(UpdatePositionAirplane, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}


TMError AirplaneAuto::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateAirplane)

      TRANSF(pilotFlaps);
      
      TRANSF(gearDamage);
      TRANSF(pilotGear);

      TRANSF(vtolMode);
      
      TRANSF_ENUM(planeState);
  
      TRANSF_ENUM(stopMode)
      TRANSF_ENUM(stopResult)
      TRANSF(stopPosition)
    }
    break;
  case NMCUpdatePosition:
    {
      PREPARE_TRANSFER(UpdatePositionAirplane)

      Matrix3 oldTrans = FutureVisualState().Orientation();
      TMCHECK(base::TransferMsg(ctx))
      StabilizeTurrets(oldTrans, FutureVisualState().Orientation());

      TRANSF(thrustWanted);
      TRANSF(thrustVectorWanted);
      TRANSF(elevatorWanted);
      TRANSF(rudderWanted);
      TRANSF(aileronWanted);
      TRANSF(pilotBrake);
      TRANSF(pilotSpeed);
/*
      if (!ctx.IsSending())
      {
        LogF("Update arrive: pilotSpeed %.3f, pilotBrake %.3f", _pilotSpeed, _pilotBrake);
      }
*/
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float AirplaneAuto::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += base::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdateAirplane)

      ICALCERR_ABSDIF(int, pilotFlaps, ERR_COEF_MODE)
      ICALCERR_NEQ(bool, gearDamage, ERR_COEF_MODE)
      ICALCERR_NEQ(bool, pilotGear, ERR_COEF_MODE)
      ICALCERR_NEQ(int, planeState, ERR_COEF_MODE)
      ICALCERR_NEQ(bool, vtolMode, ERR_COEF_MODE)

      ICALCERR_NEQ(int, stopMode, ERR_COEF_MODE)
      ICALCERR_NEQ(int, stopResult, ERR_COEF_MODE)
      ICALCERR_DIST(stopPosition, 0.1)
      // TODO: implementation
    }
    break;
  case NMCUpdatePosition:
    {
      error +=  base::CalculateError(ctx);
      PREPARE_TRANSFER(UpdatePositionAirplane)

      ICALCERR_ABSDIF(float, thrustWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, thrustVectorWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, elevatorWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, rudderWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, aileronWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, pilotBrake, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, pilotSpeed, ERR_COEF_VALUE_MAJOR)
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}

void AirplaneAuto::AutomaticLanding( float &changeHeading, float curHeading, float &changeHeadingRudder, float landingSpeed, bool &setControls, float &diveWanted, bool &diveWantedSet, bool &elevatorSet )
{
  Vector3 ilsPos = FutureVisualState().Position(), ilsDir = FutureVisualState().Direction();
  bool ilsSTOL = false;
  GWorld->GetILS(ilsPos,ilsDir,ilsSTOL,_targetSide,Type()->_vtol>0,PreferredAirport());
  // automatic landing, including approach pattern
  if( _planeState==Marshall )
  {
    _pilotHelperThrust = true;
    _thrustVectorWanted = 0;

    // fly to marshal point - prepare for approach
    // fly left or right traffic or straight approach depending on current position
    const float marshallDist = Type()->MarshallDist(); // with landing speed 70 we want 3000

    Vector3 approachPosBase = ilsPos + ilsDir*marshallDist;
    Vector3 relApproach = approachPosBase - FutureVisualState().Position();

    float headingToApproach = atan2(relApproach.X(),relApproach.Z());

    float runwayHeading = atan2(-ilsDir.X(),-ilsDir.Z());
    // relative inbound heading - 0 = runway direction
    float inbound = AngleDifference(headingToApproach,runwayHeading);
    saturate(inbound,-H_PI*0.5f,+H_PI*0.5f);
    // assume 1.5 km is enough to perform 90 degree turn
    // we need to know a direction perpendicular to ilsDir, i.e. rotated 90 deg around Y axis
    Vector3 ilsAside(+ilsDir.Z(),ilsDir.Y(),-ilsDir.X());
    float tooHigh = floatMax(FutureVisualState().Position().Y()-approachPosBase.Y()-500,0);
    Vector3 approachPos = approachPosBase+ilsAside*(marshallDist*0.3f*inbound)+ilsDir*tooHigh*0.85f;
    Vector3 relPos=approachPos-FutureVisualState().Position();
    _pilotHeading=atan2(relPos.X(),relPos.Z());
    changeHeading=AngleDifference(_pilotHeading,curHeading);
    Limit(changeHeading,-0.7f,+0.7f);
    changeHeadingRudder = changeHeading;
    _pilotHeight=100;
    _pilotSpeed=landingSpeed*2;
    if( relPos.SquareSizeXZ()<Square(floatMax(marshallDist*0.15f,200)) ) _planeState=Approach;
    setControls = true;
    _rudderWanted = 0;
  }
  else if( _planeState==Approach )
  {
    _pilotHelperThrust = true;
    _thrustVectorWanted = 0;
    const float approachDist = Type()->ApproachDist(); // with landing speed 70 we want 2000

    Vector3 approachPosBase = ilsPos + ilsDir*approachDist;
    Vector3 relApproach = approachPosBase - FutureVisualState().Position();

    float headingToApproach = atan2(relApproach.X(),relApproach.Z());
    float runwayHeading = atan2(-ilsDir.X(),-ilsDir.Z());
    // relative inbound heading - 0 = runway direction
    float inbound = AngleDifference(headingToApproach,runwayHeading);
    // inbound should be relatively small now
    // if not and if we are close, we probably need to fly to Marshall again
    float distance = (FutureVisualState().Position()-ilsPos).DotProduct(ilsDir); //.Distance(ilsPos);
    if (fabs(inbound)>H_PI*0.5f && distance<approachDist*0.9f)
    {
      _planeState = Marshall;
    }
    // as we already passed Marshall state
    Vector3 ilsAside(+ilsDir.Z(),ilsDir.Y(),-ilsDir.X());
    Vector3 approachPos = approachPosBase+ilsAside*(approachDist*0.1f*inbound);

    Vector3 relPos=approachPos-FutureVisualState().Position();
    _pilotHeading=atan2(relPos.X(),relPos.Z());
    changeHeading=AngleDifference(_pilotHeading,curHeading);
    Limit(changeHeading,-0.7f,+0.7f);
    changeHeadingRudder = changeHeading;
    _pilotHeight=120;
    _pilotSpeed=landingSpeed*1.5f;
    if( relPos.SquareSizeXZ()<Square(approachDist*0.25f) || !Type()->_hasAirbrake)
    {
      _pilotSpeed=landingSpeed*1.2f;
    }
    if( relPos.SquareSizeXZ()<Square(floatMax(approachDist*0.1f,100)) )
    {
      static float maxFinalAlt = 300.0f;
      if  (relPos.Y()>-maxFinalAlt)
        _planeState=Final;
      else
        _planeState = Marshall; // when extremely high, fly to marshall again until we descend enough
    }
    setControls = true;
    _rudderWanted = 0;
  }
  else if( _planeState==Final )
  {
    _pilotHelperThrust = true;
    _thrustVectorWanted = 0;

    // fly to final approach
    const float finalDist = Type()->FinalDist(); // with landing speed 70 we want 1000
    Vector3 finalPos=ilsPos+ilsDir*finalDist;
    Vector3 relPos=finalPos-FutureVisualState().Position();
    _pilotHeading=atan2(relPos.X(),relPos.Z());
    changeHeading=AngleDifference(_pilotHeading,curHeading);
    Limit(changeHeading,-0.5f,+0.5f);
    changeHeadingRudder = changeHeading;
    // height should be based on ILS corridor
    float distance = (FutureVisualState().Position()-ilsPos).DotProduct(ilsDir); //.Distance(ilsPos);
    float aboveRunway = 1.5f+ilsDir.Y()*distance;
    // it should not be more than 92 m above the runway
    saturate(aboveRunway,0,92);
    float ilsASL = ilsPos.Y()+aboveRunway;
    float surfaceY = GLandscape->SurfaceYAboveWaterNoWaves(FutureVisualState().Position().X(),FutureVisualState().Position().Z());
    // ignore sea waves - always assume some 
    float curHeight = FutureVisualState().Position().Y()-surfaceY;
    _pilotHeight = ilsASL-surfaceY;
    // make sure we climb/dive only slowly
    saturate(_pilotHeight,curHeight-20,curHeight+20);
    // maintain reasonable height above ground
    saturate(_pilotHeight,40,300);

    _pilotSpeed=landingSpeed*1.0f;
    // check if we are close to the "final" point or if we perhaps have already passed it
    if( relPos.SquareSizeXZ()<Square(floatMax(finalDist*0.2f,100)))
    {
      // check direction.XZ() relative to ilsDirection.XZ()
      const float cos45deg = 0.7f;
      float cosDir = FutureVisualState().Direction().X()*ilsDir.X()+FutureVisualState().Direction().Z()*ilsDir.Z();
      if (FutureVisualState().ModelSpeed()[2]<landingSpeed*2 && cosDir<-cos45deg)
      {
        _planeState=Landing;
      }
      else
      {
        // we are flying too fast, or misaligned
        _planeState=Marshall;
      }
    }
    else if (distance<finalDist*0.8f)
    {
      // it seems we have missed the "final" pos
      _planeState=Marshall;
    }
    setControls = true;
    _rudderWanted = 0;
  }
  else if (_planeState==Landing || _planeState==Touchdown)
  {
    // _thrustVectorWanted is controlled by LandingAutopilot here
    _pilotFlaps = 2;
    _pilotGear = true;
    _planeState = LandingAutopilot(ilsPos,ilsDir,ilsSTOL,changeHeading,curHeading,diveWanted,diveWantedSet,setControls);
    changeHeadingRudder = changeHeading;
  }
  else if (_planeState==AfterTouchdown)
  {
    // first reduce thrust, then stop vectoring
    if (_thrust<=0.2f)
    {
    _thrustVectorWanted = 0;
    }
    else
    {
      _thrustVectorWanted = FutureVisualState()._thrustVector;
    }
    _pilotHelperHeight = false;
    _pilotHelperThrust = true;
    _pilotHelperBankDive = true;
    _pilotSpeed=landingSpeed;
    Vector3 ilsCPos=ilsPos-ilsDir*700;
    Vector3 relPos=ilsCPos-FutureVisualState().Position();
    _pilotHeading=atan2(relPos.X(),relPos.Z());
    changeHeading=AngleDifference(_pilotHeading,curHeading);
    Limit(changeHeading,-0.2,+0.2);
    changeHeadingRudder = changeHeading;
    _thrustWanted=0;
    _pilotSpeed=0;
    _pilotBrake=1;
    diveWanted=0;
    diveWantedSet=true;
    if( fabs(FutureVisualState().ModelSpeed()[2]<20) && fabs(FutureVisualState().ModelSpeed()[2]<Type()->_landingSpeed*0.5f) && FutureVisualState()._thrustVector<=0.2f )
    {
      _pilotFlaps=0;
      // wait until you come to the right point of runway to taxi-off
      _planeState=TaxiOff;

    }
    setControls = true;
    _rudderWanted = 0;
  }
  else if( _planeState==WaveOff )
  {
    // TODO: depending on speed thrust vectoring might be needed, if it was used during approach
    _thrustVectorWanted = 0;
    _pilotHeight=fabs(FutureVisualState().ModelSpeed()[2])*1.5-30;
    saturateMax(_pilotHeight,10);
    _pilotSpeed=Type()->_landingSpeed;
    Vector3 ilsCPos=ilsPos-ilsDir*700;
    Vector3 relPos=ilsCPos-FutureVisualState().Position();
    _pilotHeading=atan2(relPos.X(),relPos.Z());
    changeHeading=AngleDifference(_pilotHeading,curHeading);
    Limit(changeHeading,-0.5,+0.5);
    changeHeadingRudder = changeHeading;
    if( relPos*ilsDir>0 || relPos.Y()<-500.0f) _planeState=Marshall;
    setControls = true;
    _rudderWanted = 0;
  }
  else if (_planeState==TaxiOff)
  {
    _thrustVectorWanted = 0;
    // move out of runway
    // handle random touch down
    if (!_landContact && FutureVisualState().ModelSpeed()[2]>60)
    {
      _planeState = Flight;
    }
    else
    {
      // use path autopilot
      if (TaxiOffAutopilot())
      {
        // even handler used
        EngineOff();
        // touched the ground - check which airport it was
        int airportId = GWorld->CheckAirport(FutureVisualState().Position(),FutureVisualState().Direction());
        OnEvent(EELandedStopped,float(airportId));
        _planeState = TaxiIn;
      }
      else if (Type()->_vtol==3)
      {
        _planeState = TaxiIn;
      }

      changeHeading=AngleDifference(_pilotHeading,curHeading);
      Limit(changeHeading,-0.4,+0.4);
      changeHeadingRudder = changeHeading;
      _elevatorWanted = 0;
      elevatorSet=true;
    }
    setControls = true;
    _rudderWanted = 0;
  }
  else if (_planeState==TaxiIn)
  {
    _thrustVectorWanted = 0;
    if (!_landContact && FutureVisualState().ModelSpeed()[2]>60)
    {
      _planeState = Flight;
    }
    else
    {
      // use path autopilot
      if (TaxiInAutopilot())
      {
        _planeState = Takeoff;
      }

      changeHeading=AngleDifference(_pilotHeading,curHeading);
      Limit(changeHeading,-0.4,+0.4);
      changeHeadingRudder = changeHeading;
      elevatorSet=true;
    }
    setControls = true;
    _rudderWanted = 0;
  }
}

