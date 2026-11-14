#include "wpch.hpp"

#include "thing.hpp"
#include "AI/ai.hpp"
#include "shots.hpp"
#include "landscape.hpp"
#include "global.hpp"
#include <El/Common/randomGen.hpp>
#include "world.hpp"
#include "diagModes.hpp"

#include <El/Common/perfProf.hpp>

//#include "scene.hpp"

#if _DEBUG
  #define ARROWS 0
#endif

ThingType::ThingType( ParamEntryPar param )
:base(param)
{
}

ThingType::~ThingType()
{
}

void ThingType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);
  _submerged = par>>"submerged";
  _submergeSpeed = par>>"submergeSpeed";
  _timeToLive = par>>"timetolive";  
  _disappearAtContact = par>>"disappearAtContact";

  GetValue(_fricCoef2,par>>"airFriction2");
  GetValue(_fricCoef1,par>>"airFriction1");
  GetValue(_fricCoef0,par>>"airFriction0");
  
  _airRotation = par>>"airRotation";
  _gravityFactor = par>>"gravityFactor";
  
  // check if aerodynamics are important enough
  _aerodynamics = (
    _fricCoef2.SquareSize()>Square(0.02f) ||
    _fricCoef1.SquareSize()>Square(0.02f) ||
    _fricCoef0.SquareSize()>Square(0.02f)
  );
  
  _minHeight = par>>"minHeight";
  _avgHeight = par>>"avgHeight";
  _maxHeight = par>>"maxHeight";
}


Object* ThingType::CreateObject(bool unused) const
{ 
  return new Thing(this);  // for _simName="thing" or "breakablehousepart"
}  

DEFINE_CASTING(Thing)

Thing::Thing(const EntityAIType *name)
:base(name),
_doCrash(CrashNone)
{
  _submerged = Type()->_submerged;
  _isCloudlet = false;

  _isStopped = true;
  _objectContact = true;
  _landContact = true;
  SetSimulationPrecision(1.0f/15);
  RandomizeSimulationTime();
  SetTimeOffset(1.0f/15);
  _destrType=GetType()->GetDestructType();
}

Vector3 Thing::Friction(Vector3Par speed)
{
  Vector3 friction;
  friction.Init();
  friction[0]=speed[0]*fabs(speed[0])*25+speed[0]*20+fSign(speed[0])*30;
  friction[1]=speed[1]*fabs(speed[1])*25+speed[1]*20+fSign(speed[1])*20;
  friction[2]=speed[2]*fabs(speed[2])*5+speed[2]*20+fSign(speed[2])*10;
  return friction*GetMass()*(1.0/1700);
}

void Thing::Simulate(float deltaT, SimulationImportance prec)
{
  _submerged += Type()->_submergeSpeed*deltaT;
  _isUpsideDown = FutureVisualState().DirectionUp().Y()<0.3;
  _isDead = IsDamageDestroyed();
  if (_isDead)
  {
    IExplosion *smoke = GetSmoke();
    if (smoke)
    {
      float explosionDelay = GRandGen.Gauss(0.2f,0.5f,1.5f);
      Time explosionTime = Glob.time+explosionDelay;
      smoke->Explode(explosionTime);
    }
    NeverDestroy();
  }
  base::Simulate(deltaT,prec);

#if _ENABLE_ATTACHED_OBJECTS
  if (IsAttached())
  {
    ApplyRemoteState(deltaT);
    return;
  }
#endif

#define SIM_STEP_LIMIT 1

#if SIM_STEP_LIMIT
  float rest = deltaT;
  float simStep = 0.05;
  while (rest>0)
#endif
  {
#if SIM_STEP_LIMIT
    float deltaT = floatMin(rest,simStep);
    rest -= simStep;
#endif

    Vector3Val speed = FutureVisualState().ModelSpeed();

    // calculate all forces, frictions and torques
    Vector3 force(VZero),friction(VZero);
    Vector3 torque(VZero),torqueFriction(VZero);

    Vector3 pForce(VZero); // partial force
    Vector3 pCenter(VZero); // partial force application point

    // simulate left/right engine
    if ((!_landContact || Type()->_submergeSpeed>0) && !_objectContact)
    {
      // it is not touching anything - we need simulation
      // or it is submerging
      OnMoved();
    }

    {
      // handle impulse
      float impulse2=_impulseForce.SquareSize();
      if (GetStopped() && (impulse2>Square(GetMass()*0.001) || FutureVisualState()._speed.SquareSize() > Square(0.1) || _angVelocity.SquareSize() > Square(0.1)))
        OnMoved();
      if( impulse2>Square(GetMass()*3) && IsLocal())
      {
        // too strong impulse - damage
        float contact=sqrt(impulse2)/(GetMass()*3);
        // contact>0
        saturateMin(contact,5);
        if (contact>0.1)
        {
          float radius = GetRadius();
          LocalDamageMyself(VZero, contact * 0.1f, radius * 0.3f);
        }
      }
    }

    if (GetStopped())
    {
      // reset impulse - avoid cumulation
      _impulseForce = VZero;
      _impulseTorque = VZero;
    }

    if( !_isStopped )
    {
      Vector3 wCenter(VFastTransform,FutureVisualState().ModelToWorld(),GetCenterOfMass());

      // apply gravity
      pForce=Vector3(0,-G_CONST*Type()->_gravityFactor*GetMass(),0);
      force+=pForce;
      
#if ARROWS
      AddForce(wCenter,pForce*InvMass(),Color(1,1,0,0.1));
#endif
      
      // angular velocity causes also some angular friction, this should be simulated as torque
      torqueFriction = _angMomentum * ((_landContact || _objectContact) ? 0.2f : 0.05f);

      // calculate new position
      VisualState moveTrans = PredictPos<VisualState>(deltaT);

      ApplyRemoteStateAdjustSpeed(deltaT,moveTrans);

      // body air friction
      FutureVisualState().DirectionModelToWorld(friction,Friction(speed));
      //friction=Vector3(0,0,0);
#if ARROWS
        AddForce(wCenter,friction*InvMass(),Color(0,1,1));
#endif
      
      wCenter.SetFastTransform(moveTrans.ModelToWorld(),GetCenterOfMass());
      
      AUTO_STATIC_ARRAY(ContactPoint,contacts,128);

      if (deltaT>0)
      {
        float above = 0.05-floatMax(_submerged,0);
        ScanContactPoints(contacts,moveTrans,prec,above);
      }

      if (deltaT>0 && contacts.Size()>0)
      {
        AUTO_STATIC_ARRAY(FrictionPoint,frictions,128);

        float crash=0;
        float maxColSpeed2=0;

#if 0
        // air friction causes some force small torque
        // add friction point
        {
          FrictionPoint &fp = frictions.Append();
          fp.accel0h = 0; // 0.5 G
          fp.accel0v = 0; // 0.5 G
          fp.accel1 = 0.1; // 0.5 G
          fp.obj = NULL;
          fp.outDir = VZero;
          fp.pos = wCenter + Vector3(1,1,1); // any point will do
          fp.angularFriction = 0.001;
        }
#endif

        Vector3 offset;
        ConvertContactsToFrictions(contacts,frictions,moveTrans,offset,force,torque,crash,maxColSpeed2);

        //LogF("accelSum %.2f",accelSum);
        //torqueFriction=_angMomentum*1.0;
#if 1
        if (offset.SquareSize()>=Square(0.01))
        {
          //LogF("Offset %.2f: %.2f,%.2f,%.2f",offset.Size(),offset[0],offset[1],offset[2]);
          // it is necessary to move object immediately
          Matrix4 transform=moveTrans.Transform();
          Vector3 newPos=transform.Position();
          newPos += offset;
          transform.SetPosition(newPos);
          moveTrans.SetTransform(transform);
          // we move up - we have to maintain total energy
          // what potential energy will gain, kinetic must loose
          const float crashLimit=0.3;
          float moveOut = offset.Size();
          if (moveOut > crashLimit)
            crash += moveOut - crashLimit;
          /**/
          // limit speed to avoid getting deeper

          Vector3 offsetDir = offset.Normalized();
          float speedOut = offsetDir*FutureVisualState()._speed;
          const float minSpeedOut = -0.5;
          if (speedOut<minSpeedOut)
          {
            // limit speed
            float addSpeedOut = minSpeedOut-speedOut;
            //LogF("Add speed %.2f",addSpeedOut);
            FutureVisualState()._speed += addSpeedOut*offsetDir;
            //LogF("new speed %.2f,%.2f,%.2f",_speed[0],_speed[1],_speed[2]);
          }
          /**/
        }
#endif
        if( crash>0.1 )
        {
          float speedCrash=maxColSpeed2*Square(1.0/7);
          if( speedCrash<0.1 ) speedCrash=0;
          if( Glob.time>_disableDamageUntil )
          {
            // crash boom bang state - impact speed too high
            /**/
            _doCrash=CrashLand;
            if( _objectContact ) _doCrash=CrashObject;
            if( _waterContact ) _doCrash=CrashWater;
            _crashVolume=crash*0.5;
            saturateMin(crash,speedCrash);
            CrashDammage(crash*4); // 1g -> 5 mm dammage
            /**/
          }
        }
        // apply all forces
        ApplyForcesAndFriction(deltaT,force,torque,frictions.Data(),frictions.Size());
      }
      else
        // apply all forces
        ApplyForcesAndFriction(deltaT,force,torque,NULL,0);


      bool stopCondition=false;
      if( ( _landContact || _objectContact ) && !_waterContact )
      {
        // apply static friction
        float maxSpeed=Square(0.5);
        if (FutureVisualState()._speed.SquareSize()<maxSpeed && _angVelocity.SquareSize()<maxSpeed)
          stopCondition = true;
      }
      if (stopCondition)
        StopDetected();
      else
        OnMoved();

      // note: it might be Cloudlet
      // if it is cloudlet, it is not in landscape and it should use

      if (_isCloudlet)
        SetTransform(moveTrans);
      else
        Move(moveTrans);

      // patch: when simulation goes very wrong, delete the object or fix it
      const float maxSpeed = 100;
      // catch both high speed and NaN speed
      if (!(FutureVisualState()._speed.SquareSize()<Square(maxSpeed)))
      {
        LogF("Patch: high speed Thing %s, %g,%g,%g", (const char *)GetDebugName(),FutureVisualState()._speed[0],FutureVisualState()._speed[1],FutureVisualState()._speed[2]);
        FutureVisualState()._speed.Normalize();
        FutureVisualState()._speed *= maxSpeed;
      }
      if (_angMomentum.SquareSize()>1e6)
      {
        LogF("Patch: high momentum Thing %s, %g,%g,%g", (const char *)GetDebugName(), _angMomentum[0],_angMomentum[1],_angMomentum[2]);
        _angMomentum.Normalize();
        _angMomentum *= 0.9e3;
      }
      DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
    }
  }

#undef SIM_STEP_LIMIT

  if (_landContact && _submerged>0 && _submerged>GetRadius()*2)
    SetDelete();

  //ApplyRemoteState(deltaT);
  SimulatePost(deltaT,prec);
}

void Thing::CrashDammage(float ammount, const Vector3 &pos)
{
  if (!IsLocal())
    return;
  ammount*=GetType()->GetInvArmor();
  LocalDamageMyself(pos,ammount,GetRadius());
}

void Thing::Sound(bool inside, float deltaT)
{
  if( _doCrash!=CrashNone && Glob.time>_timeCrash+3.0 )
  {
    _timeCrash=Glob.time;
    const SoundPars *pars=NULL;
    switch( _doCrash )
    {
      case CrashObject:
        pars=&GetType()->GetCrashSound();
      break;
      case CrashLand:
        pars=&GetType()->GetLandCrashSound();
      break;
      case CrashWater:
        pars=&GetType()->GetWaterCrashSound();
      break;
    }
    if( pars )
    {
      const VisualState &vs = RenderVisualState();
      float volume=pars->vol*_crashVolume;
      float freq=pars->freq;
      AbstractWave *sound=GSoundScene->OpenAndPlayOnce(
        pars->name,this,false,vs.Position(),vs.Speed(),volume,freq,pars->distance
      );
      if( sound )
      {
        GSoundScene->SimulateSpeedOfSound(sound);
        GSoundScene->AddSound(sound);
      }
    }
  }
  _doCrash=CrashNone;
}

void Thing::UnloadSound()
{
}

void Thing::DrawDiags()
{
  base::DrawDiags();
}

Matrix4 Thing::InsideCamera( CameraType camType ) const
{
  return base::InsideCamera(camType);
}

int Thing::InsideLOD( CameraType camType ) const
{
  return base::InsideLOD(camType);
}

AnimationStyle Thing::IsAnimated( int level ) const {return NotAnimated;}

DEFINE_CASTING(ThingEffect)

ThingEffect::ThingEffect( const EntityAIType *name )
:base(name)
{
  _objectContact=false;
}



#if 1

#if _ENABLE_REPORT
  int ThingEffectLight::_instanceCount = 0;
  int ThingEffectLight::_maxInstanceCount = 0;
#endif


DEFINE_FAST_ALLOCATOR(ThingEffectLight)

DEFINE_CASTING(ThingEffectLight)

ThingEffectLight::ThingEffectLight(const ThingType *name)
:base(name->GetShape(),name,CreateObjectId()),
_doCrash(CrashNone),
_forceTI(false)
{
  _submerged = Type()->_submerged;
  _timeToLive = Type()->_timeToLive;

  _isCloudlet = false;

  //_isStopped = true;
  _objectContact = false;
  _landContact = false;
  SetSimulationPrecision(0.2); // no real limit
  RandomizeSimulationTime();
  SetTimeOffset(0); // simulation is light anyway, interpolation is not any lighter

  #if _ENABLE_REPORT
    _instanceCount++;
    if (_instanceCount>_maxInstanceCount) _maxInstanceCount = _instanceCount;
  #endif
  
}
ThingEffectLight::~ThingEffectLight()
{
  #if _ENABLE_REPORT
    _instanceCount--;
  #endif
}

void ThingEffectLight::SetAmbient(IAmbLifeArea *area, Vector3Par camPos)
{

  base::SetAmbient(area,camPos);
  
  const ThingType *type = Type();


  // make orientation random
  
  Matrix4 trans;
  trans.SetOrientation(
    Matrix3(MRotationZ,GRandGen.RandomValue()*(H_PI*2))*
    Matrix3(MRotationY,GRandGen.RandomValue()*(H_PI*2))*
    Matrix3(MRotationX,GRandGen.RandomValue()*(H_PI*2))
  );
  
  float ry = GRandGen.Gauss(type->_minHeight,type->_avgHeight,type->_maxHeight);
  
  Vector3 pos = FutureVisualState().Position();
  pos[1] += ry;
  float windSpeed = GLandscape->GetWindSpeed().Size();
  Vector3 randomVel(
    GRandGen.Gauss(0.1,0.2,0.3),
    GRandGen.Gauss(0.1,0.2,0.3),
    GRandGen.Gauss(0.1,0.2,0.3)
  );
  SetSpeed(GLandscape->GetWindSpeed()+randomVel*windSpeed);
  trans.SetPosition(pos);
  
  Move(trans);
}

Vector3 ThingEffectLight::Friction(Vector3Par speed) const
{
  Vector3Val fricCoef2 = Type()->_fricCoef2;
  Vector3Val fricCoef1 = Type()->_fricCoef1;
  Vector3Val fricCoef0 = Type()->_fricCoef0;
  Vector3 friction;
  friction.Init();
  friction[0]=speed[0]*fabs(speed[0])*fricCoef2[0]+speed[0]*fricCoef1[0]+fSign(speed[0])*fricCoef0[0];
  friction[1]=speed[1]*fabs(speed[1])*fricCoef2[1]+speed[1]*fricCoef1[1]+fSign(speed[1])*fricCoef0[1];
  friction[2]=speed[2]*fabs(speed[2])*fricCoef2[2]+speed[2]*fricCoef1[2]+fSign(speed[2])*fricCoef0[2];
  return friction;
}

static void LimitFriction(Vector3 &pForce, Vector3Par speed, float deltaT)
{
  if (deltaT<=0) return;
  // friction can cause movement, but it cannot change a sign
  Vector3 speedDiff = pForce*deltaT;
  for (int c=0; c<3; c++)
  {
    if (pForce[c]*speed[c]<0 && fabs(speedDiff[c])>fabs(speed[c]))
      pForce[c] = -speed[c]/deltaT;
  }
}


void ThingEffectLight::Simulate(float deltaT, SimulationImportance prec)
{
  PROFILE_SCOPE(ttSim);

  _submerged += Type()->_submergeSpeed*deltaT;
  _timeToLive -= deltaT;

  base::Simulate(deltaT,prec);

  #if 1 //def _XBOX
    #define SIMPLE_SIM 1
  #else
    #define SIMPLE_SIM 0
  #endif
  
  #if SIMPLE_SIM
    #define SIM_STEP_LIMIT 0
  #else
    #define SIM_STEP_LIMIT 1
  #endif

  #if _PROFILE
    static bool switchCam = false;
    if (switchCam)
    {
      GWorld->SwitchCameraTo(this,CamExternal,true);
      switchCam = false;
    }
  #endif

  #if SIM_STEP_LIMIT
  float rest = deltaT;
  float simStep = 0.05;
  while (rest>0)
  #endif
  {
    #if SIM_STEP_LIMIT
    float deltaT = floatMin(rest,simStep);
    rest -= simStep;
    #endif

    Vector3Val speed=FutureVisualState().ModelSpeed();

    // calculate all forces, frictions and torques
    Vector3 force(VZero),friction(VZero);
    Vector3 torque(VZero),torqueFriction(VZero);

    Vector3 pForce(VZero); // partial force
    Vector3 pCenter(VZero); // partial force application point

    // simulate left/right engine
    Vector3 wCenter(VFastTransform,FutureVisualState().ModelToWorld(),GetCenterOfMass());

    // apply gravity
    pForce=Vector3(0,-G_CONST*Type()->_gravityFactor*GetMass(),0);
    force+=pForce;
    
    #if ARROWS
      AddForce(wCenter,pForce*InvMass(),Color(1,1,0,0.1));
    #endif
    
    // angular velocity causes also some angular friction, this should be simulated as torque
    torqueFriction = _angMomentum * ((_landContact || _objectContact) ? 0.2f : 0.05f);
    
    // calculate new position
    VisualState moveTrans = PredictPos<VisualState>(deltaT);
    ApplyRemoteState(deltaT,moveTrans);

    // body air friction
    Vector3 wind = GLandscape->GetWindSpeed();
    
    if (Type()->_aerodynamics)
    {
      //float bump = GLandscape->CalculateBump(Position().X(),Position().Z(),1);
      /*
      float turbulence = 0.2;
      int 
      int seed = GLandscape.GetRandomValueForCoord();
      Vector3 turbulence(
        (GRandGen.RandomValue()*turbulence*2+1-turbulence),
        (GRandGen.RandomValue()*turbulence*2+1-turbulence),
        (GRandGen.RandomValue()*turbulence*2+1-turbulence)
      );
      */

      // if necessary, simulate near-ground phenomenons
      // only important when wind is not blowing up
      const float topGroundEffect = 0.5f;
      const float zeroGroundEffect = 0.2f;
      const float groundEffectWind = 0.3f;
      
      if (wind.Y()<groundEffectWind)
      {
        float dX,dZ;
        Texture *texture;
        float bump;
        float surfaceY = GLandscape->BumpySurfaceY(
          FutureVisualState().Position().X(),FutureVisualState().Position().Z(),dX,dZ,texture,1,bump
        );
        float altitude = FutureVisualState().Position().Y()-(surfaceY+bump);
        
        // the closer the ground, the more turbulent the wind is
        // if wind is blowing down, it can turn up near the ground
        if (altitude<topGroundEffect)
        {
          float high = (altitude-zeroGroundEffect)/(topGroundEffect-zeroGroundEffect);
          wind[1] = wind.Y()*high + groundEffectWind*wind.Size()*(1-high);
        }
      }
    }
    
    Vector3 relAirSpeed = DirectionWorldToModel(wind) - speed;

    


    
    // if you wish draconic (sail-like) simulation, i.e. consider only force in direction of the model
    // provide small force coefficients for x,y coordinates
    
    // model air friction not as a friction, but rather as a force
    // it can cause acceleration as well
    pForce = Friction(relAirSpeed);

    #if _ENABLE_CHEATS
      // draw simulation diagnostics
      if (CHECK_DIAG(DEAmbient) && _ambient)
      {
        // blue - wind difference
        AddForce(wCenter,(wind-FutureVisualState()._speed),Color(0,0,1,0.5));
        // yellow - direction
        AddForce(wCenter,FutureVisualState().Direction()*0.2f,Color(1,1,0,0.5));
        // red - force
        //AddForce(wCenter,DirectionModelToWorld(pForce)*0.1f,Color(1,0,0,0.5));
      }
    #endif

    // avoid air "friction" being too large, as if could cause oscillation
    // to counteract this, we avoid changing sign of the speed by the friction force itself
    LimitFriction(pForce,speed,deltaT);
    
    if(pForce.SquareSize()>Square(1000))
    {
      //__asm nop;
    }
    
    pForce = FutureVisualState().DirectionModelToWorld(pForce)*GetMass();
    force += pForce;

//    Log(
//      "%p: Speed %g,%g,%g, diff %g,%g,%g, accel %g,%g,%g",
//      this,_speed.X(),_speed.Y(),_speed.Z(),
//      relAirSpeed.X(),relAirSpeed.Y(),relAirSpeed.Z(),
//      pForce.X()*InvMass(),pForce.Y()*InvMass(),pForce.Z()*InvMass()
//    );
    
    //friction=Vector3(0,0,0);
    #if ARROWS
      AddForce(wCenter,pForce*InvMass(),Color(0,1,1));
    #endif
    
    if (Type()->_airRotation>0)
    {
      const float invSqrt3 = 0.57735026919f;
      float offset = invSqrt3 * _shape->BoundingSphere()*Type()->_airRotation;
      pCenter = Vector3(offset,offset,offset);
      pForce = relAirSpeed*Type()->_airRotation * GetMass();
      Vector3 pTorque = pCenter.CrossProduct(pForce);
      
      torque += FutureVisualState().DirectionModelToWorld(pTorque);
    }
    
    wCenter.SetFastTransform(moveTrans.ModelToWorld(),GetCenterOfMass());
    
    #if !SIMPLE_SIM
    AUTO_STATIC_ARRAY(ContactPoint,contacts,128);

    if( deltaT>0 )
    {
      float above = 0.05-floatMax(_submerged,0);

      _landContact = _objectContact = false;
      ScanContactPoints(contacts,moveTrans,prec,above,true);
    }

    if( deltaT>0 && contacts.Size()>0)
    {
      _landContact = _objectContact = true;
      AUTO_STATIC_ARRAY(FrictionPoint,frictions,128);

      float crash=0;
      float maxColSpeed2=0;

      Vector3 offset;
      ConvertContactsToFrictions(
        contacts,frictions,
        moveTrans,offset,force,torque,crash,maxColSpeed2
      );


      #if 1
      if (offset.SquareSize()>=Square(0.01))
      {
        //LogF("Offset %.2f: %.2f,%.2f,%.2f",offset.Size(),offset[0],offset[1],offset[2]);
        // it is neccessary to move object immediatelly
        Matrix4 transform=moveTrans.Transform();
        Vector3 newPos=transform.Position();
        newPos += offset;
        transform.SetPosition(newPos);
        moveTrans.SetTransform(transform);
        // we move up - we have to maintain total energy
        // what potential energy will gain, kinetic must loose
        const float crashLimit=0.3;
        float moveOut = offset.Size();
        if( moveOut>crashLimit ) crash+=moveOut-crashLimit;
        /**/
        // limit speed to avoid getting deeper

        Vector3 offsetDir = offset.Normalized();
        float speedOut = offsetDir*_speed;
        const float minSpeedOut = -0.5;
        if (speedOut<minSpeedOut)
        {

          // limit speed
          float addSpeedOut = minSpeedOut-speedOut;
          //LogF("Add speed %.2f",addSpeedOut);
          _speed += addSpeedOut*offsetDir;
          //LogF("new speed %.2f,%.2f,%.2f",_speed[0],_speed[1],_speed[2]);

        }
        /**/
      }
      #endif
      if( crash>0.1 )
      {
        float speedCrash=maxColSpeed2*Square(1.0/7);
        if( speedCrash<0.1 ) speedCrash=0;
        if( Glob.time>_disableDamageUntil )
        {
          // crash boom bang state - impact speed too high
          /**/
          _doCrash=CrashLand;
          if( _objectContact ) _doCrash=CrashObject;
          if( _waterContact ) _doCrash=CrashWater;
          _crashVolume=crash*0.5;
          /**/
        }
      }
      // apply all forces
      ApplyForcesAndFriction(deltaT,force,torque,frictions.Data(),frictions.Size());
    }
    else
    #endif
    {
      // calculate ground based friction
      if (_landContact)
      {
        friction += Vector3(FutureVisualState()._speed[0],0,FutureVisualState()._speed[2])*1.1f;
      }

      if(force.SquareSize()>Square(GetMass()*1000))
      {
        //__asm nop;
      }
      
      // apply all forces
      ApplyForces(deltaT,force,torque,friction,torqueFriction,0);

      if(FutureVisualState()._speed.SquareSize()>1e4)
      {
        //__asm nop;
      }

      _landContact = false;
      if (_ambient)
      {
        Vector3 pos = moveTrans.Position();
        float surfaceY = GLandscape->SurfaceY(pos.X(),pos.Z());
        float radius = GetRadius();
        // avoid getting under ground
        if (moveTrans.Position().Y()<surfaceY+radius)
        {
          pos[1] = surfaceY+radius;
          moveTrans.SetPosition(pos);
          // deflect if necessary
          if (FutureVisualState()._speed[1]<0)
          {
            FutureVisualState()._speed[1] = 0;
          }
          _landContact = true;
        }
      }
    }


    // note: it might be Cloudlet
    // if it is cloudlet, it is not in landscape and it should use SetTransform

    if (_isCloudlet)
    {
      SetTransform(moveTrans);
    }
    else
    {
      Move(moveTrans);
    }
    DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);

    if
    (
      FutureVisualState()._speed.SquareSize()>1e6 || _angMomentum.SquareSize()>1e6
    )
    {
      LogF("Patch: high speed object %s",(const char *)GetDebugName());
      SetDelete();
      #if SIM_STEP_LIMIT
      break;
      #endif
    }
  }

  #if SIMPLE_SIM
    float surfaceY = GLandscape->SurfaceY(FutureVisualState().Position().X(),FutureVisualState().Position().Z());
    float radius = GetRadius();
    if (!_ambient)
    {
      // delete once we are in the contact with a ground
      if (FutureVisualState().Position().Y()+radius<surfaceY)
      {
        // we are completely under the ground
        SetDelete();
      }
    }
  #else  
  if
  (
    _landContact && _submerged>0 && _submerged>GetRadius()*2
    || _timeToLive<0
    || Type()->_disappearAtContact && _landContact
  )
  {
    SetDelete();
  }
  #endif

  // patch: when simulation goes very wrong, delete the object
  if (!FutureVisualState().Transform().IsFinite())
  {
    LogF("Patch: infinite object %s",(const char *)GetDebugName());
    SetDelete();
  }
}

void ThingEffectLight::Sound( bool inside, float deltaT )
{
  if( _doCrash!=CrashNone && Glob.time>_timeCrash+3.0 )
  {
    _timeCrash=Glob.time;
    const SoundPars *pars=NULL;
    switch( _doCrash )
    {
      case CrashObject:
        pars=&Type()->GetCrashSound();
      break;
      case CrashLand:
        pars=&Type()->GetLandCrashSound();
      break;
      case CrashWater:
        pars=&Type()->GetWaterCrashSound();
      break;
    }
    if( pars )
    {
      const VisualState &vs = RenderVisualState();
      float volume=pars->vol*_crashVolume;
      float freq=pars->freq;
      AbstractWave *sound=GSoundScene->OpenAndPlayOnce(
        pars->name,NULL,false,vs.Position(),vs.Speed(),volume,freq,pars->distance
      );
      if( sound )
      {
        GSoundScene->SimulateSpeedOfSound(sound);
        GSoundScene->AddSound(sound);
      }
    }
  }
  _doCrash=CrashNone;
}

void ThingEffectLight::UnloadSound()
{
}

// no get-in to buildings

AnimationStyle ThingEffectLight::IsAnimated( int level ) const
{
  return NotAnimated;
}
void ThingEffectLight::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
}
void ThingEffectLight::Deanimate( int level, bool setEngineStuff )
{
}


#endif



/*
static const ThingEffectItem TEGroundList[]=
{
  ThingEffectItem("FxExploGround1",0.5),
  ThingEffectItem("FxExploGround2",10), // any probability rest
  ThingEffectItem()
};
*/

static const ThingEffectItem TEArmorList[]=
{
  ThingEffectItem("FxExploArmor1",0.25),
  ThingEffectItem("FxExploArmor2",0.25),
  ThingEffectItem("FxExploArmor3",0.25),
  ThingEffectItem("FxExploArmor4",10), // any probability rest
/*
  ThingEffectItem("FxExploGround1",0.1),
  ThingEffectItem("FxExploGround2",10), // any probability rest
*/
  ThingEffectItem()
};

static const ThingEffectItem TECartridgeList[]=
{
  ThingEffectItem("FxCartridge",10), // any probability rest
  ThingEffectItem()
};

static const ThingEffectItem *SelectThingEffect
(
  const ThingEffectItem *list, float value
)
{
  // value 0..1
  while (list->_probab>=0)
  {
    value -= list->_probab;
    if (value<0) return list;
    list++;
  }
  Fail("No corresponding effect");
  return NULL;
}


Entity *CreateThingEffect(
  ThingEffectKind kind, // kind
  Matrix4Val pos, Vector3Val vel // position and velocity
)
{
  #if 0
  return NULL;
  #else
  const ThingEffectItem *list = NULL;
  switch (kind)
  {
    default:
      Fail("Bad effect kind");
      // fall through
    case TEGround:
/*
      list = TEGroundList;
      break;
*/
      return NULL;
    case TEArmor:
      list = TEArmorList;
      break;
    case TECartridge:
      list = TECartridgeList;
      break;
  }
  const ThingEffectItem *item = SelectThingEffect(list,GRandGen.RandomValue());
  if (!item) return NULL;
  Ref<EntityType> type=VehicleTypes.New(item->_type);
  // check if given shape is ready - if not, ignore the effect
  if (!type->IsReadyForVehicle())
  {
    return NULL;
  }
  
  // create a corresponding entity
  Ref<Entity> veh = GWorld->NewNonAIVehicleWithID(type);
  if (!veh) return veh;
  veh->SetTransform(pos);
  veh->SetSpeed(vel);
  //veh->OnMoved();
  ThingEffectLight *thing = dyn_cast<ThingEffectLight,Entity>(veh);
  if (thing)
  {
    thing->SetCloudlet(true);
    GWorld->AddCloudlet(thing);
  }
  else if (veh)
  {
    GWorld->AddAnimal(veh);
  }
  return veh;
  #endif
} 

Entity *CreateThing(
  VehicleType *type, // kind
  Matrix4Val pos, Vector3Val vel, // position and velocity
  bool forceTI
)
{
  Ref<Entity> veh = GWorld->NewNonAIVehicleWithID(type->GetName(),RString());
  if (!veh) return veh;
  veh->SetTransform(pos);
  veh->SetSpeed(vel);
  //veh->OnMoved();
  ThingEffectLight *thing = dyn_cast<ThingEffectLight,Entity>(veh);
  if (thing)
  {
    thing->SetForceTI(forceTI);
    thing->SetCloudlet(true);
    GWorld->AddCloudlet(thing);
  }
  else if (veh)
  {
    GWorld->AddAnimal(veh);
  }
  return veh;
} 
