// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"

#include "ship.hpp"
#include "AI/ai.hpp"

#include "shots.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "keyInput.hpp"
#include <El/Common/randomGen.hpp>
#include <El/Common/perfLog.hpp>
#include "Shape/specLods.hpp"
#include "stringtableExt.hpp"
#include "paramFileExt.hpp"

#include "car.hpp" // In simulation in object collision is needed to distinguish if object is car


#include <El/ParamFile/paramFile.hpp>
#include "paramArchiveExt.hpp"

#include "Network/network.hpp"

#include <El/Enum/enumNames.hpp>

#if _ENABLE_CHEATS && _PROFILE
# pragma optimize("",off)
#endif


#if _ENABLE_CHEATS
  #define ARROWS 1
#endif

ShipType::ShipType( ParamEntryPar param )
:base(param)
{
  _pilotPos = VZero;
  _gunnerPilotPos = VZero;
  _commanderPilotPos = VZero;
}

void ShipType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  _leftEngine =  par >> "leftEngineEffect";
  _rightEngine =  par >> "rightEngineEffect";

  SetHardwareAnimation(par);
}

#include <Es/Common/delegate.hpp>

AnimationSource *ShipType::CreateAnimationSource(const AnimationType *type, RStringB source)
{
  static const struct AnimSourceDesc
  {
    const char *name;
    float (Ship::*func)(ObjectVisualState const&) const;
  } desc[]=
  {
    {"rotor",&Ship::GetCtrlRotorPos},
    {"rotorR",&Ship::GetCtrlRotorPosR},
    {"rotorL",&Ship::GetCtrlRotorPosL},
  };
  for (int i=0; i<sizeof(desc)/sizeof(desc[0]); i++)
  {
    if (!strcmpi(source,desc[i].name))
    {
      return _animSources.CreateAnimationSource(desc[i].func);
    }
  }
  return base::CreateAnimationSource(type,source);
}

void ShipType::InitShape()
{
  base::InitShape();

  const ParamEntry &par=*_par;

  int level;

  // camera positions
  level=_shape->FindLevel(VIEW_PILOT);
  if( level>=0 )
  {
    ShapeUsed cockpit=_shape->Level(level);
    cockpit->MakeCockpit();

    _pilotPos = _shape->NamedPoint(level,(par >> "pointPilot").operator RString());
    _commanderPilotPos = _shape->NamedPoint(level,(par >> "pointCommander").operator RString());
  }

  _lEnginePos=_shape->MemoryPoint((par >> "memoryPointsLeftEngineEffect").operator RString());
  _rEnginePos=_shape->MemoryPoint((par >> "memoryPointsRightEngineEffect").operator RString());

  _hitZoneTextureUV.Add(29);
}

void ShipType::DeinitShape()
{
  base::DeinitShape();
}

void ShipType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);

  if (level==_shape->FindSpecLevel(VIEW_CARGO))
  {
    ShapeUsed cockpit=_shape->Level(level);
    cockpit->MakeCockpit();
  }

  if (level==_shape->FindSpecLevel(VIEW_GUNNER))
  {
    ShapeUsed cockpit=_shape->Level(level);
    cockpit->MakeCockpit();

    _gunnerPilotPos = _shape->NamedPoint(level,"pilot");
  }

}
void ShipType::DeinitShapeLevel(int level)
{
  base::DeinitShapeLevel(level);
}


Object* ShipType::CreateObject(bool unused) const
{ 
  return new ShipWithAI(this, NULL, unused);
}


Ship::Ship( const EntityAIType *name, Person *driver, bool fullCreate)
:base(name,driver,fullCreate),
// pilot controls
_thrustLWanted(0),_thrustRWanted(0),
_turnSpeedWanted(0),_turnSpeedWantedLastActive(TIME_MIN),
_lastAngVelocity(VZero),
_sink(0),
_randFrequency(1-GRandGen.RandomValue()*0.05) // do not use same sound frequency  
{
  _leftEngine.Init(Pars >> Type()->_leftEngine, this, EVarSet_Water);
  _rightEngine.Init(Pars >> Type()->_rightEngine, this, EVarSet_Water);
}


ShipVisualState::ShipVisualState(ShipType const& type)
:base(type),
_rotorPosition(0),
_rotorPositionL(0),
_rotorPositionR(0),
_thrustL(0), _thrustR(0)
{
}


void ShipVisualState::Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const
{
  base::Interpolate(obj, t1state, t, interpolatedResult);

  ShipVisualState const& s = static_cast<ShipVisualState const&>(t1state);
  ShipVisualState& res = static_cast<ShipVisualState&>(interpolatedResult);

  #define INTERPOLATE(x) res.x = x + t*(s.x - x)

  INTERPOLATE(_rotorPositionL);  // continuous (maybe should be mod k*pi)
  INTERPOLATE(_rotorPositionR);  // continuous
  INTERPOLATE(_rotorPosition);  // continuous

  #undef INTERPOLATE
}


float Ship::GetEngineVol( float &freq ) const
{ 
  if (_engineOff)
  {
    freq = 1;
    return 0;
  }
  float thrust=(fabs(FutureVisualState()._thrustL)+fabs(FutureVisualState()._thrustR))*0.5;
  freq=_randFrequency*1.2*(thrust*0.7+0.5);
  return fabs(thrust)*0.5+0.5;
}

/*!
\patch 5129 Date 2/20/2007 by Ondra
- Fixed: Reduced water splash volume when boats are not moving.
*/
float Ship::GetEnvironVol( float &freq ) const
{
  float speedFactor = Type()->GetMaxSpeedMs()>0 ? FutureVisualState()._speed.Size()/Type()->GetMaxSpeedMs() : 0;
  static float freqStopped = 0.7f;
  static float freqFullSpeed = 1.3f;
  freq=speedFactor*freqFullSpeed+(1-speedFactor)*freqStopped;
  // even with low speed we want some environmental sound
  static float volStopped = 0.1f;
  static float volFullSpeed = 1.0f;
  return speedFactor*volFullSpeed+(1-speedFactor)*volStopped;
}

bool Ship::IsVirtual( CameraType camType ) const
{
  return true;
}

Vector3 Ship::DragFriction( Vector3Par speed )
{
  Vector3 friction;
  friction.Init();
  //friction[0]=speed[0]*fabs(speed[0])*70+speed[0]*2000+fSign(speed[0])*50;
  friction[1]=speed[1]*fabs(speed[1])*70+speed[1]*1100+fSign(speed[1])*20;
  //friction[2]=speed[2]*fabs(speed[2])*70+speed[2]*50+fSign(speed[2])*5;
  friction[0]=fSign(speed[0])*50;
  //friction[1]=speed[1]*550+fSign(speed[1])*20;
  friction[2]=fSign(speed[2])*5;
  return friction*GetMass()*(1.0/60000);
}

Vector3 Ship::DragForce( Vector3Par speed )
{
  Vector3 friction;
  friction.Init();
  friction[0]=-speed[0]*fabs(speed[0])*3000-speed[0]*30000;
  //friction[0]=-speed[0]*fabs(speed[0])*150-speed[0]*4000;
  friction[1]=0;
  friction[2]=-speed[2]*fabs(speed[2])*70-speed[2]*50;
  return friction*GetMass()*(1.0/60000);
}

static const Color ShipLightColor(0.9,0.8,0.8);
static const Color ShipLightAmbient(0.1,0.1,0.1);

void Ship::MoveWeapons(float deltaT)
{
  MoveCrewHead();
}

Pair<Vector3,float> Ship::ShowDrivingCursor(CameraType camType, Person *person) const
{
  if (person!=Driver()) return MakePair(VZero,_cursorY);
  Vector3 dir = Matrix3(MRotationY,-_turnSpeedWanted).Direction();
  float dimByAge = floatMinMax((10-Glob.time.Diff(_turnSpeedWantedLastActive))*0.3f,0,1);
  float dimByState = floatMinMax(fabs(dir.X())*100,0,0.5f);
  float dim = floatMax(dimByAge,dimByState);
  return MakePair(FutureVisualState().DirectionModelToWorld(dir*dim),_cursorY);
}


void Ship::StabilizeTurrets(Matrix3Val oldTrans, Matrix3Val newTrans)
{
  if (fabs(_turnSpeedWanted)>0.01f)
  {
    Vector3 turnDir = Matrix3(MRotationY,-_turnSpeedWanted).Direction();
    Vector3 turnDirNew = newTrans.InverseRotation()*oldTrans*turnDir;
    _turnSpeedWanted = atan2(turnDirNew.X(),turnDirNew.Z());

  }


  base::StabilizeTurrets(oldTrans, newTrans);
}

/*!
\patch 1.26 Date 10/05/2001 by Ondra
- Fixed: Sinked ship is removed. This will avoid unnecessary updates in MP.
\patch 1.27 Date 10/16/2001 by Ondra
- Fixed: Water splash was drawn aside of the ship.
\patch 1.30 Date 11/01/2001 by Ondra
- Fixed: Boats were shaking (since 1.29)
\patch 5101 Date 12/11/2006 by Ondra
- Fixed: Boats were able to drive on shore quite fast.
\patch 5101 Date 12/11/2006 by Ondra
- Fixed: Boats were able to sink into the terrain once destroyed.
*/

void Ship::Simulate( float deltaT, SimulationImportance prec )
{
  if( _isDead )
  {
    IExplosion *smoke = GetSmoke();
    if( smoke ) smoke->Explode();
    NeverDestroy();
  }

  Vector3Val speed=FutureVisualState().ModelSpeed();
  float speedSize=fabs(speed.Z());

  float delta;
    
  MoveWeapons(deltaT);

  #if 0
  if( _isStopped )
  {
    _doCrash=CrashNone;
    _servoVol=0;
    return; // no simulation if stable
  }
  #endif


  if( _isDead || _isUpsideDown ) _engineOff=true;
  if( _engineOff ) _pilotBrake=true;
  if( _pilotBrake ) _thrustLWanted=_thrustRWanted=0;

  const float baseMass=8584999;
  float sizeCoef=GetShape()->GeometrySphere()*(1.0/80);
  bool smallShip=GetMass()<baseMass*0.1;
  
  // simulate left/right engine
  delta=floatMinMax(_thrustLWanted-_turnSpeedWanted,-1,+1)-FutureVisualState()._thrustL;
  Limit(delta,-1*deltaT,+1*deltaT);
  FutureVisualState()._thrustL+=delta;
  Limit(FutureVisualState()._thrustL,-1.0,1.0);

  delta=floatMinMax(_thrustRWanted+_turnSpeedWanted,-1,+1)-FutureVisualState()._thrustR;
  Limit(delta,-1*deltaT,+1*deltaT);
  FutureVisualState()._thrustR+=delta;
  Limit(FutureVisualState()._thrustR,-1.0,1.0);
  
  // slow steering autocentering
  static float centerSpeedFast = 0.05f;
  static float centerSpeedSlow = 0.00f;
  static float centerAngleFast = 0.05f;
  static float centerAngleSlow = 0.15f;
  float centerSpeed = Interpolativ(fabs(_turnSpeedWanted),centerAngleFast,centerAngleSlow,centerSpeedFast,centerSpeedSlow);
  _turnSpeedWanted = TowardZero(_turnSpeedWanted,deltaT*centerSpeed);

  FutureVisualState()._rotorPosition += (FutureVisualState()._thrustL+FutureVisualState()._thrustR)*deltaT;
  FutureVisualState()._rotorPositionL += FutureVisualState()._thrustL*deltaT;
  FutureVisualState()._rotorPositionR += FutureVisualState()._thrustR*deltaT;

  if (!CheckPredictionFrozen() && !IsAttached())
  {
    // calculate all forces, frictions and torques
    Vector3 force(VZero),friction(VZero);
    Vector3 torque(VZero),torqueFriction(VZero);

    Vector3 pForce(VZero); // partial force
    Vector3 pCenter(VZero); // partial force application point

    Vector3 wCenter(VFastTransform,FutureVisualState().ModelToWorld(),GetCenterOfMass());

    // fuel
    if (!_engineOff)
    {
      ConsumeFuel(deltaT*(0.01+(fabs(FutureVisualState()._thrustR)+fabs(FutureVisualState()._thrustL))*0.01));
      if (FutureVisualState()._fuel <= 0) _engineOff = true;
    }
    
    // apply left/right thrust

    //bool gearChanged=false;
    if (!_engineOff)
    {
      if( _landContact || _objectContact || _waterContact )
      {
        //const float minInvSpeed=1.8;
        float power=Type()->GetMaxSpeed()*(1.0/50)*0.13;
        //float rpmEff=rpm*0.5+0.5;
        // water movement is much less efficient
        if( smallShip ) power*=3;
        if (!_waterContact)
        {
          // no water means we only simulate pushing on the ground
          // engine power needs to be significantly reduced for this
          power *= 0.3f;
        }
        // 
        // the thrust efficiency is much lower when close to (or over max speed)
        float invMaxSpeed = 1.0f/Type()->GetMaxSpeedMs();
        static float thrSpdF = 3.0f;
        static float maxSpdF = 2.0f;
        float diffLThrust = thrSpdF*FutureVisualState()._thrustL-FutureVisualState().ModelSpeed().Z()*invMaxSpeed*maxSpdF;
        float diffRThrust = thrSpdF*FutureVisualState()._thrustR-FutureVisualState().ModelSpeed().Z()*invMaxSpeed*maxSpdF ;
        // as the model/friction  was balanced with efficiency 1
        // we want to be around 1 when full forward in maxSpeed
        // this should be around 3 when full reverse in maxSpeed
        // (Note: we can only reach half-reverse)
        saturate(diffLThrust,-3.0f,3.0f);
        saturate(diffRThrust,-3.0f,3.0f);
        float lAccel=diffLThrust*power;
        float rAccel=diffRThrust*power;
        pForce=Vector3(0,0,lAccel*GetMass());
        force+=pForce;
        static float turnFactor = 40.0f;
        float turnSize= turnFactor*sizeCoef;
        pCenter=Vector3(+turnSize,0,0); // relative to the center of mass
        torque+=pCenter.CrossProduct(pForce);
        #if ARROWS
          AddForce
          (
            FutureVisualState().DirectionModelToWorld(pCenter)+wCenter,
            FutureVisualState().DirectionModelToWorld(pForce*InvMass()),
            Color(0,1,0,0.5)
          );
        #endif

        pForce=Vector3(0,0,rAccel*GetMass());
        force+=pForce;
        pCenter=Vector3(-turnSize,0,0); // relative to the center of mass
        torque+=pCenter.CrossProduct(pForce);
        #if ARROWS
          AddForce
          (
            FutureVisualState().DirectionModelToWorld(pCenter)+wCenter,
            FutureVisualState().DirectionModelToWorld(pForce*InvMass()),
            Color(0,1,0,0.5)
          );
        #endif
      }
    }

    // convert forces to world coordinates
    FutureVisualState().DirectionModelToWorld(torque,torque);
    FutureVisualState().DirectionModelToWorld(force,force);

    // apply gravity
    pForce=Vector3(0,-G_CONST*GetMass(),0);
    force+=pForce;
    
    #if ARROWS
      AddForce(wCenter,pForce*InvMass(),Color(0,0,1,0.5));
    #endif

    // angular velocity causes also some angular friction
    // this should be simulated as torque
    //torqueFriction=_angMomentum*2.5;
    ///LogF("AngMom %g,%g,%g",_angMomentum.X(),_angMomentum.Y(),_angMomentum.Z());
    static float torqueFrictionF = 0.8f;
    torqueFriction=_angMomentum*torqueFrictionF;

    if (_isDead && _waterContact)
    {
      _sink += deltaT * 0.33;
      saturateMin(_sink,50);
      if (_sink>3)
      {
        // stop smoking - we are under water
        DeleteSmoke();
      }
      // if ship sinked completely, delete it
      if (_sink>=_shape->BoundingSphere()*2 && !_landContact)
      {
        SetDelete();
      }
    }
    else if (!_isDead && _sink>0)
    {
      _sink -= deltaT * 0.33;
      saturateMin(_sink,0);
    }

    // simulate draconic force
    // (force which causes any movement energy to be transfered to front-back axis)
    // front-back component of friction

    FutureVisualState().DirectionModelToWorld(pForce,DragForce(speed));
    
    force += pForce;
    #if ARROWS
      AddForce(wCenter,pForce*InvMass(),Color(1,1,0,0.5));
    #endif

    // calculate new position
    //LogF("Ship av %.1f,%.1f,%.1f",_angVelocity[0],_angVelocity[1],_angVelocity[2]);
    //LogF("Ship am %.1f,%.1f,%.1f",_angMomentum[0],_angMomentum[1],_angMomentum[2]);
    VisualState moveTrans = PredictPos<VisualState>(deltaT);

    ApplyRemoteStateAdjustSpeed(deltaT,moveTrans);
    // body air friction
    FutureVisualState().DirectionModelToWorld(friction,DragFriction(speed));
    //friction=Vector3(0,0,0);
    #if ARROWS
      AddForce(wCenter,friction*InvMass(),Color(1,0,0,0.5));
    #endif
    
    //wCenter=_moveTrans.PositionModelToWorld();
    // recalculate COM to reflect change of position
    wCenter.SetFastTransform(moveTrans.ModelToWorld(),GetCenterOfMass());

    float soft=0,dust=0;
    if( deltaT>0 )
    {
      // check collision on new position
      Vector3 totForce(VZero);

      float crash=0;
      #if 1

      CollisionBuffer collision;
      GLOB_LAND->ObjectCollision(collision,this,moveTrans);
      _objectContact=false;
      if( collision.Size()>0  )
      {
        #define MAX_IN 0.2f
        #define MAX_IN_FORCE 0.1f
        #define MAX_IN_FRICTION 0.2f

        _objectContact=true;
        for( int i=0; i<collision.Size(); i++ )
        {
          // info.pos is relative to object
          CollisionInfo &info=collision[i];
          if( info.object )
          {
            info.object->SimulationNeeded(GetVisualStateAge(FutureVisualState()));
            if(!info.object->IsPassable() && info.object->GetMass()>=100)
            {
              Point3 pos=info.pos;
              Vector3 dirOut=info.dirOut;
              // create a force pushing "out" of the collision
              float forceIn=floatMin(info.under,MAX_IN_FORCE);
              //Vector3 pForce=dirOut*GetMass()*100*forceIn; // Changed by Fehy, to big force.
              Vector3 pForce=dirOut*GetMass()*40*forceIn;
              // apply proportional part of force in place of impact
              pCenter=pos-wCenter;
              totForce+=pForce;
              torque+=pCenter.CrossProduct(pForce)*0.5f;
              
              // if info.under is bigger than MAX_IN, move out
              if( info.under>MAX_IN && !dyn_cast<Car,Object>(info.object))
              {

                Matrix4 transform=moveTrans.Transform();
                Point3 newPos=transform.Position();
                float moveOut=info.under-MAX_IN;
                Vector3 move=dirOut*moveOut*0.1f;
                newPos+=move;
                transform.SetPosition(newPos);
                moveTrans.SetTransform(transform);
                const float crashLimit=0.3f;
                if( moveOut>crashLimit ) crash+=moveOut-crashLimit;

                Vector3Val objSpeed=info.object->ObjectSpeed();
                Vector3 colSpeed=FutureVisualState()._speed-objSpeed;

                float potentialGain=move[1]*GetMass();
                float oldKinetic=GetMass()*colSpeed.SquareSize()*0.5f; // E=0.5*m*v^2
                // kinetic to potential conversion is not 100% effective
                float crashFactor=(moveOut-crashLimit)*4+1.5f;
                saturateMax(crashFactor,2.5f);
                float newKinetic=oldKinetic-potentialGain*crashFactor;
                float newSpeedSize2=newKinetic*InvMass()*2;
                if( newSpeedSize2<=0 || oldKinetic<=0 ) colSpeed=VZero;
                else colSpeed*=sqrt(newSpeedSize2*colSpeed.InvSquareSize());
                // limit relative speed to object we crashed into
                const float maxRelSpeed=2;
                if( colSpeed.SquareSize()>Square(maxRelSpeed) )
                {
                  // adapt _speed to match criterion
                  crash+=(colSpeed.Size()-maxRelSpeed)*0.3f;
                  colSpeed.Normalize();
                  colSpeed*=maxRelSpeed;
                }
                FutureVisualState()._speed=objSpeed+colSpeed;
              }

              // second is "land friction" - causing little momentum
              float frictionIn=floatMin(info.under,MAX_IN_FRICTION);
              pForce[0]=fSign(speed[0])*10000;
              pForce[1]=speed[1]*fabs(speed[1])*1000+speed[1]*8000+fSign(speed[1])*10000;
              pForce[2]=speed[2]*fabs(speed[2])*150+speed[2]*250+fSign(speed[2])*2000;

              pForce=FutureVisualState().DirectionModelToWorld(pForce)*GetMass()*(4.0f/10000)*frictionIn;
              #if ARROWS
                AddForce(wCenter+pCenter,-pForce*InvMass(),Color(1,1,0,0.5));
              #endif
              friction+=pForce;
              torqueFriction+=_angMomentum*0.3f;

            }
          }
        }
      }
      #endif

      {
        Vector3 upWanted=VUp;
        /**/
        if( smallShip )
        {
#if _VBS3_BOATPHYSICS
					float displacementSpeed = Type()->GetMaxSpeed() *0.28 / 4.0; //4.5[km/h] * sqrt(Boatlength)
					float power=Type()->GetMaxSpeed()*(1.0/50)*0.13;
					if( smallShip ) power*=5;
//					float lAccel=FutureVisualState()._thrustL*power;
//					float rAccel=FutureVisualState()._thrustR*power;
					
					float turnA = torque[1] / GetMass() / 10;
					saturate(turnA,-0.5f,+0.5f);
					float asideWanted;

					if(speed[2] > displacementSpeed)
					{
						upWanted[2]=-speed[2]*0.01;
						asideWanted = turnA*0.6f*fabs(ModelSpeed().Z());
					}
					else
					{
						upWanted[2]=-speed[2]*0.1;
						asideWanted = turnA*0.6f*fabs(ModelSpeed().Z());
					}

					saturate(asideWanted,-0.2f,+0.2);
					upWanted[0] = asideWanted;

					saturate(upWanted[2],-0.3,0.1);
					Matrix3 orient;
					orient.SetUpAndDirection(VUp,Direction());
					upWanted=orient*upWanted.Normalized();
#else

          upWanted[2]=-speed[2]*0.008;
          saturate(upWanted[2],-0.3,0.1);
          // upWanted is model space now
          // convert to horizontally aligned world space
          Matrix3 orient;
          orient.SetUpAndDirection(VUp,FutureVisualState().Direction());
          upWanted=orient*upWanted.Normalized();
#endif
        }

        // predict direction up in some time
        float dirEstT = 1.0f;
        //Vector3Val angAcceleration=(_angVelocity-_lastAngVelocity)/deltaT;
        //Vector3Val avgAngVelocity=_angVelocity+angAcceleration*0.5*dirEstT;
        //Matrix3Val derOrientation=avgAngVelocity.Tilda()*orientation;
        const Matrix3 &orientation=FutureVisualState().Orientation();
        Matrix3 derOrientation=_angVelocity.Tilda()*orientation;
        Matrix3Val estOrientation=orientation+derOrientation*dirEstT;

        Vector3Val estDirectionUp=estOrientation.DirectionUp().Normalized();
        //Vector3Val estDirectionUp=moveTrans.DirectionUp();

        // estDirectionUp is world space here
        Vector3 stabilize=upWanted-estDirectionUp;
        stabilize[1]=0;
        static float stabFactorX = 0.2f;
        FutureVisualState().DirectionWorldToModel(stabilize,stabilize);
        saturate(stabilize[0],-0.3,+0.3);
        saturate(stabilize[2],-0.3,+0.3);
        stabilize[0]*=stabFactorX;
        FutureVisualState().DirectionModelToWorld(stabilize,stabilize);

        #if ARROWS
          // 
          AddForce(wCenter+Vector3(0,2,0),upWanted*10,Color(0,0,0,0.5));
          AddForce(wCenter+Vector3(0,2,0),stabilize*10,Color(0,1,1,0.5));
        #endif

        /**/
        GroundCollisionBuffer gCollision;
        const float aboveBias = 0.5f; 
        // simulate sinking when dead
        // only add sink for detected water contacts
        GLandscape->GroundCollision(gCollision,this,moveTrans,aboveBias,1);
        ADD_COUNTER(wGndC,gCollision.Size());
        _landContact=false;
        _waterContact=false;
        if( gCollision.Size()>0 )
        {
          #define MAX_UNDER 4.0
          #define MAX_UNDER_FORCE 2.0

          Vector3 gFriction(VZero);
          float maxUnder=0;
          for( int i=0; i<gCollision.Size(); i++ )
          {
            // info.pos is world space
            UndergroundInfo &info=gCollision[i];
            // we consider two forces
            info.under -= aboveBias;
            if( info.under<0 ) continue;
            if( info.type==GroundWater )
            {
              // simulate sinking by gradually reducing the threshold for the penalty force
              info.under -= _sink;
              if( info.under<0 ) continue;
              _waterContact=true;
              // simulate swimming force
              const float coefNPoints=12.0/12.0;
              // first is water is "pushing" everything up - causing some momentum
              pForce=Vector3(0,(1200000/baseMass)*GetMass()*info.under*coefNPoints,0);
              if( smallShip )
              {
                pForce*=20+floatMin(floatMax(speed[2],0)*6,100);
              }
              pCenter=info.pos-wCenter;
              torque+=pCenter.CrossProduct(pForce*0.2);
              totForce+=pForce;

              #if ARROWS
                AddForce(wCenter+pCenter,pForce*InvMass(),Color(1,0,1,0.5));
              #endif
              
              // second is "water friction" - causing no momentum
              pForce[0]=speed[0]*fabs(speed[0])*0.50*info.under+speed[0]*info.under*4;
              pForce[1]=speed[1]*fabs(speed[1])*40+speed[1]*30;
              pForce[2]=(fabs(speed[2])*0.02+0.06)*speed[2]*info.under;

              if( _pilotBrake ) pForce*=8;
              if( smallShip )
              {
                pForce[0]*=8;
                pForce*=200;
              }

              pForce=FutureVisualState().DirectionModelToWorld(pForce*info.under)*GetMass()*(1.0/7000);
              //torque+=pForce.CrossProduct(Direction()*-0.25);
              #if ARROWS
                AddForce(wCenter+pCenter,-pForce*InvMass(),Color(1,0,1,0.5));
              #endif
              friction+=pForce*coefNPoints;
              if( smallShip )
              {
                Vector3 tf=_angMomentum*0.7*info.under;
                tf[1]*=0.05;
                torqueFriction+=tf;
              }
              else torqueFriction+=_angMomentum*0.001*info.under;
            }
            else
            {
              _landContact=true;
              if( maxUnder<info.under ) maxUnder=info.under;
              float under=floatMin(info.under,MAX_UNDER_FORCE);

              //const float coefNPoints=12.0/4.0;
              const float coefNPoints=12.0/12.0;
              // one is ground "pushing" everything out - causing some momentum
              Vector3 dirOut=Vector3(0,info.dZ,1).CrossProduct(Vector3(1,info.dX,0)).Normalized();
              pForce=dirOut*GetMass()*40.0*under*coefNPoints;
              pCenter=info.pos-wCenter;
              torque+=pCenter.CrossProduct(pForce);
              // to do: analyze ground reaction force
              totForce+=pForce;

              #if ARROWS
                AddForce(wCenter+pCenter,pForce*under*InvMass(),Color(1,0,1,0.5));
              #endif
              
              // second is "land friction" - causing no momentum
              // x^2 member makes sure that as speed grows, the friction should grow very fast
              pForce[0]=speed[0]*fabs(speed[0])*300+speed[0]*5000+fSign(speed[0])*20000;
              pForce[1]=speed[1]*8000+fSign(speed[1])*5000;
              pForce[2]=speed[2]*fabs(speed[2])*300+speed[2]*1000+fSign(speed[2])*5000;
              if( smallShip ) pForce*=2;

              pForce=FutureVisualState().DirectionModelToWorld(pForce)*GetMass()*(1.0/40000);
              #if ARROWS
                AddForce(wCenter+pCenter,-pForce*InvMass(),Color(1,0,1,0.5));
              #endif
              friction+=pForce*coefNPoints;
              torqueFriction+=_angMomentum*4*info.under;

              // some friction is caused by moving the land aside
              // this applies only to soft surfaces
              if( info.texture )
              {
                soft=info.texture->Roughness()*0.7;
                dust=info.texture->Dustness()*0.7;
                saturateMin(soft,1);
                saturateMin(dust,1);
              }
              float landMoved=info.under;
              saturateMin(landMoved,0.1);
              pForce[0]=speed[0]*6500*landMoved*soft;
              pForce[1]=0;
              pForce[2]=speed[2]*1500*landMoved*soft;
              pForce=FutureVisualState().DirectionModelToWorld(pForce)*GetMass()*(1.0/1000);
              #if ARROWS
                AddForce(wCenter+pCenter,-pForce*InvMass(),Color(1,0,1,0.5));
              #endif
              friction+=pForce;
            }
          }
          if (_waterContact && _sink<2)
          {
            const SurfaceInfo &info = GLandscape->GetWaterSurface();
            soft = info._roughness * 2.5;
            dust = info._dustness * 2.5;
            saturateMin(soft,1);
            saturateMin(dust,1);
          }
          if( maxUnder>MAX_UNDER )
          {
            // it is necessary to move object immediately
            Matrix4 transform=moveTrans.Transform();
            Point3 newPos=transform.Position();
            float moveUp=maxUnder-MAX_UNDER;
            newPos[1]+=moveUp;
            transform.SetPosition(newPos);
            moveTrans.SetTransform(transform);
            // we move up - we have to maintain total energy
            // what potential energy will gain, kinetic must loose
            const float crashLimit=0.3;
            if( moveUp>crashLimit ) crash+=moveUp-crashLimit;
            float potentialGain=moveUp*GetMass();
            float oldKinetic=GetMass()*FutureVisualState()._speed.SquareSize()*0.5f; // E=0.5*m*v^2
            // kinetic to potential conversion is not 100% effective
            float crashFactor=(moveUp-crashLimit)*4+1.5;
            saturateMax(crashFactor,2.5);
            float newKinetic=oldKinetic-potentialGain*crashFactor;
            //float newSpeedSize=sqrt(newKinetic*InvMass()*2);
            float newSpeedSize2=newKinetic*InvMass()*2;
            // _speed=_speed*_speed.InvSize()*newSpeedSize
            if( newSpeedSize2<=0 || oldKinetic<=0 ) FutureVisualState()._speed=VZero;
            else FutureVisualState()._speed*=sqrt(newSpeedSize2*FutureVisualState()._speed.InvSquareSize());
          }
        }
        if( _waterContact )
        {
          static float stabilizeF = 800.0f;
          torque+=Vector3(0,stabilizeF*sizeCoef*GetMass(),0).CrossProduct(stabilize);
        }
        
      }

      force+=totForce;
      float crashTreshold=30*GetMass(); // 3G
      float forceCrash=0;
      if( totForce.SquareSize()>Square(crashTreshold) )
      {
        forceCrash=(totForce.Size()-crashTreshold)*InvMass()*(1.0f/100);
      }
      crash+=forceCrash;
      if( crash>0.1 )
      {
        // crash boom bang state - impact speed too high
        _doCrash=CrashWater;
        if( _objectContact ) _doCrash=CrashObject;
        else if( _landContact ) _doCrash=CrashLand;
        else crash*=0.1;
        _crashVolume=crash*0.5;
        //GLOB_ENGINE->ShowMessage(1000,"%s crash %d, %f",_name,_doCrash,_crashVolume);
      }
      //DirectDammage(this,Position(),forceCrash*0.1);
    }

    _lastAngVelocity = _angVelocity;

    // apply all forces
    ApplyForces(deltaT,force,torque,friction,torqueFriction);

    if( _pilotBrake )
    {
      if( ( _landContact || _waterContact ) && !_objectContact )
      {
        // apply static friction
        if( FutureVisualState()._speed.SquareSizeXZ()<0.2 )
        {
          FutureVisualState()._speed[0]=FutureVisualState()._speed[2]=0;
        }
      }
    }

    // simulate track drawing   
    if( EnableVisualEffects(prec) && FutureVisualState().DirectionUp().Y()>=0.3 )
    {
      if( _waterContact )
      {
        float offset=speed[2];
        saturate(offset,-1,1);
        Vector3 lPos = Type()->_lWaterPos;
        Vector3 rPos = Type()->_rWaterPos;
        lPos[2]*=offset; //when ship move backward create water effect on rear 
        rPos[2]*=offset;
        lPos=FutureVisualState().PositionModelToWorld(lPos);
        rPos=FutureVisualState().PositionModelToWorld(rPos);
        Vector3 lEngPos=FutureVisualState().PositionModelToWorld(Type()->_lEnginePos);
        Vector3 rEngPos=FutureVisualState().PositionModelToWorld(Type()->_rEnginePos);

        // place on water surface
        lPos[1] = GLandscape->WaterSurfaceY(lPos)-0.1f;
        rPos[1] = GLandscape->WaterSurfaceY(rPos)-0.1f;
        lEngPos[1] = GLandscape->WaterSurfaceY(lEngPos)-0.1f;
        rEngPos[1] = GLandscape->WaterSurfaceY(rEngPos)-0.1f;
        // TODO: some more effective way to avoid too much conversions
        lPos = PositionWorldToModel(lPos);
        rPos = PositionWorldToModel(rPos);
        lEngPos = PositionWorldToModel(lEngPos);
        rEngPos = PositionWorldToModel(rEngPos);

        float dens=floatMin(speedSize*0.1,0.7);
        float size=floatMin(speedSize*0.1,0.7);
        float densEngL=fabs(FutureVisualState()._thrustL);
        float densEngR=fabs(FutureVisualState()._thrustR);
        float side=floatMax(0.1,speed[2]*0.2);
        Vector3 sideV=FutureVisualState().DirectionAside()*side;
        Vector3 spdL=FutureVisualState().Speed()*0.4+sideV;
        Vector3 spdR=FutureVisualState().Speed()*0.4-sideV;
        Vector3 spdEngL=FutureVisualState().Direction()*(-3.0*FutureVisualState()._thrustL)+FutureVisualState().Speed()*0.2;
        Vector3 spdEngR=FutureVisualState().Direction()*(-3.0*FutureVisualState()._thrustR)+FutureVisualState().Speed()*0.2;
        if (dens > 0.1)
        {
          float leftWaterValues[] = {dens, size, lPos.X(), lPos.Y(), lPos.Z(),
            spdL.X(), spdL.Y(), spdL.Z()};
          _leftWater.Simulate(deltaT, prec, leftWaterValues, lenof(leftWaterValues));
          float rightWaterValues[] = {dens, size, rPos.X(), rPos.Y(), rPos.Z(),
            spdR.X(), spdR.Y(), spdR.Z()};
          _rightWater.Simulate(deltaT, prec, rightWaterValues, lenof(rightWaterValues));
        }
        if (densEngL > 0.1)
        {
          float leftEngineValues[] = {densEngL, 0.12f, lEngPos.X(), lEngPos.Y(), lEngPos.Z(),
            spdEngL.X(), spdEngL.Y(), spdEngL.Z()};
          _leftEngine.Simulate(deltaT, prec, leftEngineValues, lenof(leftEngineValues));
        }
        if (densEngR > 0.1)
        {
          float rightEngineValues[] = {densEngR, 0.12f, rEngPos.X(), rEngPos.Y(), rEngPos.Z(),
            spdEngR.X(), spdEngR.Y(), spdEngR.Z()};
          _rightEngine.Simulate(deltaT, prec, rightEngineValues, lenof(rightEngineValues));
        }
      }
    }

    StabilizeTurrets(FutureVisualState().Transform().Orientation(), moveTrans.Orientation());
    Move(moveTrans);
    DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
  } // if (!frozen)

  base::Simulate(deltaT,prec);
  SimulatePost(deltaT,prec);

}


AnimationStyle Ship::IsAnimated( int level ) const {return AnimatedGeometry;}

float Ship::GetHitForDisplay(int kind) const
{
  // see InGameUI::DrawTankDirection
  switch (kind)
  {
  case 1: return GetHitCont(Type()->_engineHit);
  default: return 0;
  }
}

void Ship::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  if( _shape->Level(level).IsNull() ) return;

  // what was this good for?
  //Shape *geom=_shape->GeometryLevel();
  //geom->CalculateMinMax();
  
  if (_weaponsState.ShowFire())
  {
    Type()->_weapons._animFire.Unhide(animContext, _shape, level);
    Type()->_weapons._animFire.SetPhase(animContext, _shape, level, _weaponsState._mGunFirePhase);
  }
  else
  {
    Type()->_weapons._animFire.Hide(animContext, _shape, level);
  }

  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);
}
void Ship::Deanimate( int level, bool setEngineStuff )
{
  if( _shape->Level(level).IsNull() ) return;

  base::Deanimate(level,setEngineStuff);
}

float Ship::ThrustWanted() const
{
  return (fabs(_thrustLWanted)+fabs(_thrustRWanted))*0.5f;
}

void Ship::Eject(AIBrain *unit, Vector3Val diff )
{
  base::Eject(unit,diff);
}

/*!
\patch 1.42 Date 1/10/2002 by Jirka
- Fixed: AI units cannot get in ships since 1.40
*/
/*!
\patch 5137 Date 3/7/2007 by Jirka
- Fixed: Different selection of points where to get in / get out the ship
*/

GetInPoint Ship::GetDriverGetInPos(Person *person, Vector3Par upos) const
{
  // instead of spot nearest to upos, select the place with the most shallow water
  GetInPoint best;
  float maxHeight = -FLT_MAX;
  int n = Type()->NDriverGetInPos();
  for (int i=0; i<n; i++)
  {
    GetInPoint pt = AnimateGetInPoint(Type()->GetDriverGetInPos(i));
    RenderVisualState().PositionModelToWorld(pt._pos, pt._pos);
    float height = GLandscape->SurfaceY(pt._pos.X(), pt._pos.Z());
    if (height > maxHeight)
    {
      maxHeight = height;
      best = pt;
    }
  }
  if (maxHeight > -FLT_MAX)
    RenderVisualState().DirectionModelToWorld(best._dir, best._dir);
  else
    best = GetDefaultGetInPos(person, upos);

  best._pos[1] = GLandscape->RoadSurfaceYAboveWater(best._pos[0], best._pos[2]);
  return best;
}

GetInPoint Ship::GetCommanderGetInPos(Person *person, Vector3Par upos) const
{
  // instead of spot nearest to upos, select the place with the most shallow water
  GetInPoint best;
  float maxHeight = -FLT_MAX;

  TurretContext context;
  if (GetPrimaryObserverTurret(context) && context._turret)
  {
    int n = context._turretType->NGunnerGetInPos();
    for (int i=0; i<n; i++)
    {
      GetInPoint pt = AnimateGetInPoint(context._turretType->GetGunnerGetInPos(i));
      RenderVisualState().PositionModelToWorld(pt._pos, pt._pos);
      float height = GLandscape->SurfaceY(pt._pos.X(), pt._pos.Z());
      if (height > maxHeight)
      {
        maxHeight = height;
        best = pt;
      }
    }
  }
  if (maxHeight > -FLT_MAX)
    RenderVisualState().DirectionModelToWorld(best._dir, best._dir);
  else
    best = GetDefaultGetInPos(person, upos);

  best._pos[1] = GLandscape->RoadSurfaceYAboveWater(best._pos[0], best._pos[2]);
  return best;
}

GetInPoint Ship::GetGunnerGetInPos(Person *person, Vector3Par upos) const
{
  // instead of spot nearest to upos, select the place with the most shallow water
  GetInPoint best;
  float maxHeight = -FLT_MAX;

  TurretContext context;
  if (GetPrimaryGunnerTurret(context) && context._turret)
  {
    int n = context._turretType->NGunnerGetInPos();
    for (int i=0; i<n; i++)
    {
      GetInPoint pt = AnimateGetInPoint(context._turretType->GetGunnerGetInPos(i));
      RenderVisualState().PositionModelToWorld(pt._pos, pt._pos);
      float height = GLandscape->SurfaceY(pt._pos.X(), pt._pos.Z());
      if (height > maxHeight)
      {
        maxHeight = height;
        best = pt;
      }
    }
  }
  if (maxHeight > -FLT_MAX)
    RenderVisualState().DirectionModelToWorld(best._dir, best._dir);
  else
    best = GetDefaultGetInPos(person, upos);

  best._pos[1] = GLandscape->RoadSurfaceYAboveWater(best._pos[0], best._pos[2]);
  return best;
}

GetInPoint Ship::GetTurretGetInPos(Turret *turret, Person *person, Vector3Par upos) const
{
  // instead of spot nearest to upos, select the place with the most shallow water
  GetInPoint best;
  float maxHeight = -FLT_MAX;

  TurretContext context;
  if (FindTurret(turret, context) && context._turret)
  {
    int n = context._turretType->NGunnerGetInPos();
    for (int i=0; i<n; i++)
    {
      GetInPoint pt = AnimateGetInPoint(context._turretType->GetGunnerGetInPos(i));
      RenderVisualState().PositionModelToWorld(pt._pos, pt._pos);
      float height = GLandscape->SurfaceY(pt._pos.X(), pt._pos.Z());
      if (height > maxHeight)
      {
        maxHeight = height;
        best = pt;
      }
    }
  }
  if (maxHeight > -FLT_MAX)
    RenderVisualState().DirectionModelToWorld(best._dir, best._dir);
  else
    best = GetDefaultGetInPos(person, upos);

  best._pos[1] = GLandscape->RoadSurfaceYAboveWater(best._pos[0], best._pos[2]);
  return best;
}

GetInPoint Ship::GetCargoGetInPos(int cargoIndex, Person *person, Vector3Par upos) const
{
  // instead of spot nearest to upos, select the place with the most shallow water
  GetInPoint best;
  float maxHeight = -FLT_MAX;
  int n = Type()->NCargoGetInPos(cargoIndex);
  for (int i=0; i<n; i++)
  {
    GetInPoint pt = AnimateGetInPoint(Type()->GetCargoGetInPos(cargoIndex, i));
    RenderVisualState().PositionModelToWorld(pt._pos, pt._pos);
    float height = GLandscape->SurfaceY(pt._pos.X(), pt._pos.Z());
    if (height > maxHeight)
    {
      maxHeight = height;
      best = pt;
    }
  }
  if (maxHeight > -FLT_MAX)
    RenderVisualState().DirectionModelToWorld(best._dir, best._dir);
  else
    best = GetDefaultGetInPos(person, upos);

  best._pos[1] = GLandscape->RoadSurfaceYAboveWater(best._pos[0], best._pos[2]);
  return best;
}

void Ship::FakePilot( float deltaT )
{
}


void Ship::SuspendedPilot(AIBrain *unit, float deltaT )
{
}

#if _ENABLE_CHEATS
#include "dikCodes.h"
#endif

/*!
  \patch 5101 Date 12/13/2006 by Ondra
  - Fixed: Boat can now be controlled with keyboard while looking around using a mouse.
  \patch 5122 Date 1/24/2007 by Bebul
  - Fixed: Boat can now be controlled by mouse
*/
void Ship::KeyboardPilot(AIBrain *unit, float deltaT )
{
  if (!unit->GetPerson() || unit->GetPerson()->KeyboardUsedInFrame()) return;
  CancelStop();
  float aimX(0), aimY(0);
  if (unit->LookAroundEnabled())
  {
    aimX = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UACarAimRight, UACarAimLeft);
    aimY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UACarAimUp, UACarAimDown);
  }
  else if (Glob.config.IsEnabled(DTWeaponCursor) && (GWorld->GetCameraType()==CamExternal || GWorld->GetCameraType()==CamInternal))
  {
    float aimCursorY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UACarAimUp, UACarAimDown);
    // part of aimY will be used to move the cursor, the rest is to move the head

    static float vertFactor = 0.15f;
    _cursorY -= aimCursorY*vertFactor;

    float moveAimY = 0;

    static float vertYMin = 0.1f;
    static float vertYMax = 0.9f;
    if (_cursorY>vertYMax) moveAimY -= _cursorY-vertYMax, _cursorY = vertYMax;
    if (_cursorY<vertYMin) moveAimY -= _cursorY-vertYMin, _cursorY = vertYMin;

    static float headFactor = 60.0f;
    aimY = moveAimY*headFactor;
  }
  
  float forward=(GInput.GetAction(UACarForward)-GInput.GetAction(UACarBack))*0.75;
  forward+=GInput.GetActionCarFastForward();
  forward+=GInput.GetAction(UACarSlowForward)*0.33;
  _thrustRWanted=_thrustLWanted=forward;
  
  //UACarAimLeft and UACarLeft are separate now, so looking around and turning can be processed simultaneously
  // turn with arrows, mouse steering - more left, more right
  float turnSpeedWanted = (GInput.GetAction(UACarWheelRight)-GInput.GetAction(UACarWheelLeft))*Input::MouseRangeFactor;
  if (fabs(turnSpeedWanted)>0.01f) _turnSpeedWantedLastActive = Glob.time;

  static float steeringFact = 0.25f;

#if _ENABLE_CHEATS
  if (GInput.keyPressed[DIK_LCONTROL] && GInput.keyPressed[DIK_X])
  {
    steeringFact += 0.00025;
    DIAG_MESSAGE(1000, Format("Wheel factor: %.5f", steeringFact));
  }

  if (GInput.keyPressed[DIK_LCONTROL] && GInput.keyPressed[DIK_C])
  {
    steeringFact -= 0.00025;
    DIAG_MESSAGE(1000, Format("Wheel factor: %.5f", steeringFact));
  }
#endif

  _turnSpeedWanted += (turnSpeedWanted * steeringFact * deltaT);
  Limit(_turnSpeedWanted, -1.0f, +1.0f);
  
  float turnWanted = 0;

  //if (!GWorld->HasMap())
  {
    // aim and turn with aiming actions
    LookAround(aimX, aimY, deltaT, &Type()->_viewPilot, unit, false, true);
    turnWanted = (GInput.GetAction(UACarRight)-GInput.GetAction(UACarLeft))*Input::MouseRangeFactor;
  }

  _thrustLWanted -= turnWanted;
  _thrustRWanted += turnWanted;
  Limit(_thrustLWanted,-0.5,1);
  Limit(_thrustRWanted,-0.5,1);

  if
  (
    fabs(_thrustLWanted+_turnSpeedWanted)+fabs(_thrustRWanted-_turnSpeedWanted)<0.1 && fabs(FutureVisualState()._modelSpeed.Z())<4.0
  )
  {
    _pilotBrake=true;
  }
  else
  {
    _pilotBrake=false;
    CancelStop();
    if (_engineOff) EngineOn();
  }

/*
  if( GInput.userHeadlights>=0 && GInput.keysToDo[GInput.userHeadlights] )
  {
    _pilotLight=!_pilotLight;
    GInput.keysToDo[GInput.userHeadlights]=false;
  }
*/
}

Vector3 ShipWithAI::GetStopPosition() const
{
  if (_stopState == SSNone) return base::GetStopPosition();
  return _stopPosition;
}

/*!
\patch 5137 Date 3/6/2007 by Jirka
- Fixed: Ships - searching a place where to go for get in / get out improved
\patch 5138 Date 3/9/2007 by Jirka
- Fixed: Ships - faster movement in shallow water
*/

bool ShipWithAI::FindStopPosition()
{
  // search for the place near current position neighbor to position under water
  // only centers of strategic fields are tested
#define FIELD_HEIGHT(x, z) (0.5 * (GLandscape->GetHeight((z), (x) + 1) + GLandscape->GetHeight((z) + 1, (x))))

  float seaLevel = GLandscape->GetSeaLevelNoWave();

  int x = toIntFloor(FutureVisualState().Position().X() * InvTerrainGrid);
  int z = toIntFloor(FutureVisualState().Position().Z() * InvTerrainGrid);
  float y = FIELD_HEIGHT(x, z);
  if (y >= seaLevel)
  {
    _stopPosition = FutureVisualState().Position();
    return true;
  }

  int maxRange = (int)floatMax
  (
    floatMax(x, TerrainRange - 2 - x),
    floatMax(z, TerrainRange - 2 - z)
  );
  // limit search to near vicinity
  if (maxRange*TerrainGrid>500)
  {
    maxRange = (int)(500/TerrainGrid);
    LogF("Limit landing spot search range %d",maxRange);
  }

  // search in squares around current position
  for (int range=1; range<=maxRange; range++)
  {
    // go through edge
    for (int i=-range; i<range; i++)
    {
      // check all four edges
      int xx = x + i;
      int zz = z - range;
      float y = FIELD_HEIGHT(xx, zz);
      if (y >= seaLevel)
      // if (y >= yMin && y < yMax)
      {
        // last field in water
        zz++; if (i == -range) xx++;

        _stopPosition[0] = (xx + 0.5f) * TerrainGrid;
        _stopPosition[2] = (zz + 0.5f) * TerrainGrid;
        _stopPosition[1] = FutureVisualState().Position().Y();
        return true;
      }
      xx = x + range;
      zz = z + i;
      y = FIELD_HEIGHT(xx, zz);
      if (y >= seaLevel)
      {
        // last field in water
        xx--; if (i == -range) zz++;

        _stopPosition[0] = (xx + 0.5f) * TerrainGrid;
        _stopPosition[2] = (zz + 0.5f) * TerrainGrid;
        _stopPosition[1] = FutureVisualState().Position().Y();
        return true;
      }
      xx = x - i;
      zz = z + range;
      y = FIELD_HEIGHT(xx, zz);
      if (y >= seaLevel)
      {
        // last field in water
        zz--; if (i == -range) xx--;

        _stopPosition[0] = (xx + 0.5f) * TerrainGrid;
        _stopPosition[2] = (zz + 0.5f) * TerrainGrid;
        _stopPosition[1] = FutureVisualState().Position().Y();
        return true;
      }
      xx = x - range;
      zz = z - i;
      y = FIELD_HEIGHT(xx, zz);
      if (y >= seaLevel)
      // if (y >= yMin && y < yMax)
      {
        // last field in water
        xx++; if (i == -range) zz--;

        _stopPosition[0] = (xx + 0.5f) * TerrainGrid;
        _stopPosition[2] = (zz + 0.5f) * TerrainGrid;
        _stopPosition[1] = FutureVisualState().Position().Y();
        return true;
      }
    }
  }

  Fail("No stop position");
  return true;
}

const float DriverReactionTime = 1.0;

void ShipWithAI::Autopilot(AIBrain *unit, SteerInfo &info)
{
#if _ENABLE_AI
  switch (_stopState)
  {
    case SSNone:
      {
        Vector3 from = FutureVisualState().Position() + FutureVisualState().Speed() * DriverReactionTime;
        FindStopPosition();
        unit->SetWantedPosition(PlanPosition(_stopPosition,false), 0, AIBrain::VehiclePlanned, true);
        // following is needed, because without it CreateStrategicPath may fail
        // as it can be called with destination+planningMode not set yet
        unit->SetWantedDestination(PlanPosition(_stopPosition,false), AIBrain::VehiclePlanned, true);
        _stopState = SSFindPath;
        break;
      }
    case SSFindPath:
      // strategic planning
      {
        unit->CreateStrategicPath(AI::LevelOperative);
        const IAIPathPlanner &planner = unit->GetPlanner();
        if (!planner.IsSearching())
        {
          unit->CopyPath(planner);
          _stopState = SSMove;
        }
      }
      break;
    case SSMove:
      {
        // TODO: simplify

        // check path position
        const Path &path=unit->GetPath();
        if( path.Size()>=2 )
        {
          Vector3 steerPos=SteerPoint(GetSteerAheadSimul(), GetSteerAheadPlan(), NULL, NULL, NULL, true);
          Vector3Val steerPredict=SteerPoint(GetPredictTurnSimul(),GetPredictTurnPlan());

          float hcOffset=0;

          float spdFactor=FutureVisualState().ModelSpeed()[2]*(1.0/15);
          saturate(spdFactor,0,1);

          Vector3 steerWant;
          if( spdFactor>0 )
          {
            steerWant=PositionWorldToModel(steerPos);

            if( steerWant.Z()>0 )
            {
              hcOffset=steerWant.X()*0.02;
              saturate(hcOffset,-0.25,+0.25);
            }
          }

          steerPos += FutureVisualState().DirectionAside()*_avoidAside;

          steerWant=PositionWorldToModel(steerPos);
          info.headChange=atan2(steerWant.X(),steerWant.Z())+hcOffset*spdFactor;

          Vector3Val steerPredictRel=PositionWorldToModel(steerPredict);
          info.turnPredict=atan2(steerPredictRel.X(),steerPredictRel.Z());
          
          if (_moveMode == VMMBackward)
            info.headChange=atan2(-steerWant.X(),-steerWant.Z());
          else
            info.headChange=atan2(steerWant.X(),steerWant.Z())+hcOffset*spdFactor;
          
          EngineOn();
          //_moveMode=gotoNormal;
          float cost=path.CostAtPos(FutureVisualState().Position());
          AILockerSavedState state;
          PerformUnlock(&state);
          info.speedWanted=path.SpeedAtCost(cost,false);
          PerformRelock(state);

          Vector3Val pos=path.PosAtCost(cost,0,FutureVisualState().Position(),cost);

          float distPath2=(FutureVisualState().Position()-pos).SquareSizeXZ();
          float distEnd2=(FutureVisualState().Position()-path.End()).SquareSizeXZ();

          float precision=GetPrecision();
          float tholdDist2=Square(floatMax(precision*0.9,1));
          if( FutureVisualState().Position().Distance2(pos)>tholdDist2 )
          {
            saturateMax(info.speedWanted,GetType()->GetMaxSpeedMs()*0.25);
          }

          if( distEnd2<Square(precision) )
          {
            // HERE IS DIFFERENCE FROM FORMATION PILOT
            _stopState = SSStop;
          }
          else if( distPath2>Square(GetPrecision()*2) )
          {
            // HERE IS DIFFERENCE FROM FORMATION PILOT
            _stopState = SSNone;
            return;
          }
          else
          {
            // check if we are in valid region
            if (path.GetReplanCost() > 0 && cost >= path.GetReplanCost())
            {
              // HERE IS DIFFERENCE FROM FORMATION PILOT
              _stopState = SSNone;
              return;
            }
          }
        }
        else
        {
          float finalDist2 = (_stopPosition - FutureVisualState().Position()).SquareSizeXZ();
          float prec = GetPrecision();
          if (finalDist2 < Square(prec))
          {
            _stopState = SSStop;
          }
          // path maybe was not planned yet
          else if (path.GetSearchTime()<Glob.time-5 && unit->GetState() != AIBrain::Planning)
          {
            // HERE IS DIFFERENCE FROM FORMATION PILOT
            _stopState = SSNone;
            return;
          }
          info.speedWanted=0;
          info.headChange=0;
        }

        // strategic target known
        float finalDist2=(_stopPosition - FutureVisualState().Position()).SquareSizeXZ();
        float prec=GetPrecision();
        float maxSpeed=GetType()->GetMaxSpeedMs();
        if( finalDist2<Square(prec*3) )
        {
          if( finalDist2<Square(prec*1) )
          {
            info.speedWanted=0;
            if (FutureVisualState().Speed().SquareSizeXZ()<Square(1))
            {
              // bug hotfix: if we are there and moving slow, finish this stage
              _stopState = SSStop;
              return;
            }
          }
          else
          {
            float minSpd=maxSpeed*0.25;
            saturateMax(minSpd,1);
            saturate(info.speedWanted,-minSpd,+minSpd);
          }
        }   

        AIBrain *commander = CommanderUnit();
        if (commander)
        {
          saturateMin(info.speedWanted,_limitSpeed); // move max. by given speed
          saturateMin(info.speedWanted,_limitSpeedForced); // move max. by given speed
        }

        if( Glob.time<_avoidSpeedTime )
        {
          saturate(info.speedWanted,-_avoidSpeed,+_avoidSpeed);
        }
      }
      break;
    case SSStop:
      {
        Vector3Val speed = FutureVisualState().ModelSpeed();
        if (fabs(speed[2]) < 1)
        {
          unit->OnStepCompleted();
          _stopState = SSNone;
        }
        info.speedWanted=0;
        info.headChange=0;
      }
      break;
  }
#endif
}

#if _ENABLE_AI
void ShipWithAI::AIPilot(AIBrain *unit, float deltaT )
{
  Assert( unit );

  Vector3Val speed = FutureVisualState().ModelSpeed();
  
  SteerInfo info;
  info.headChange=0;
  info.speedWanted=0;
  info.turnPredict=0;

  if( unit->GetState()==AIBrain::Stopping )
  {
    // special handling of stop state
    Autopilot(unit, info);
  }
  else
  {
    _stopState = SSNone;
    if( unit->GetState()==AIBrain::Stopped )
    {
      // special handling of stop state
      info.speedWanted=0;
      info.headChange=0;
    }
    else
    { // subgroup leader -
      // if we are near the target we have to operate more precisely
      LeaderPilot(info);
    }
  }

  #if DIAG_SPEED
  if( this==GWorld->CameraOn() )
  {
    LogF("Pilot %.1f",speedWanted*3.6);
  }
  #endif

  AvoidCollision(deltaT,info.speedWanted,info.headChange);


  #if DIAG_SPEED
  if( this==GWorld->CameraOn() )
  {
    LogF("AvoidCollision %.1f",speedWanted*3.6);
  }
  #endif

  float curHeading=atan2(FutureVisualState().Direction()[0],FutureVisualState().Direction()[2]);
  float wantedHeading=curHeading+info.headChange;

  float estDirT = 2.0;
  // estimate inertial orientation change
  /*
  Matrix3Val orientation=Orientation();
  Matrix3Val derOrientation=_angVelocity.Tilda()*orientation;
  Matrix3Val estOrientation=orientation+derOrientation*estDirT;
  Vector3Val estDirection=estOrientation.Direction();
  */
  const Matrix3 &orientation = FutureVisualState().Orientation();

  Vector3Val angAcceleration=(_angVelocity-_lastAngVelocity)*(1/deltaT);
  Vector3Val avgAngVelocity=_angVelocity+angAcceleration*0.5*estDirT;
  Matrix3Val derOrientation=avgAngVelocity.Tilda()*orientation;
  //Matrix3 derOrientation=_angVelocity.Tilda()*orientation;
  Matrix3Val estOrientation=orientation+derOrientation*estDirT;

  //LogF("anga %g,%g,%g",angAcceleration[0],angAcceleration[0],angAcceleration[0]);

  Vector3Val estDirection=estOrientation.Direction().Normalized();

  float estHeading=atan2(estDirection[0],estDirection[2]);

  info.headChange=AngleDifference(wantedHeading,estHeading);

  {
    float maxSpeed=GetType()->GetMaxSpeedMs();
    float limitSpeed=Interpolativ(fabs(info.turnPredict),H_PI/16,H_PI/4,maxSpeed,3);
    float limitSpeedC=Interpolativ(fabs(info.headChange),H_PI/16,H_PI/4,maxSpeed,0);
    #if DIAG_SPEED
    if( this==GWorld->CameraOn() )
    {
      LogF("Turn limit %.1f (%.3f, turn %.3f)",limitSpeed,info.headChange,info.turnPredict);
    }
    #endif

    saturate(info.speedWanted,-limitSpeed,+limitSpeed);
    saturate(info.speedWanted,-limitSpeedC,+limitSpeedC);
  }

  if( fabs(info.speedWanted)>0.5f ) EngineOn();

  Vector3 relAccel=DirectionWorldToModel(FutureVisualState()._acceleration);
  float changeAccel=(info.speedWanted-speed.Z())*(1/0.5f)-relAccel.Z();
  // some thrust is needed to keep speed
  float isSlow=1-fabs(speed.Z())*(1.0f/17);
  saturate(isSlow,0.2f,1);

  changeAccel*=isSlow;
  float thrustOld=(FutureVisualState()._thrustL+FutureVisualState()._thrustR)*0.5f;
  float thrust=thrustOld+changeAccel*0.33f;
  Limit(thrust,-1,1);

  // when the heading change is small, we want the turning to be even slower
  const float rotCoef=InterpolativC(fabs(info.headChange),0,H_PI/4,1.0f,10.0f);
  float rotWanted = info.headChange*rotCoef;

  // note: to prevent oscillation we need to compensate for the momentum somewhat
  float currentRot = _angVelocity.Y();
  // with low angular velocity we assume lower inertia?
  float assumedAngInertia = InterpolativC(fabs(currentRot),0,H_PI/2,0.1f,1.0f);

  float rotThrust = rotWanted - currentRot*assumedAngInertia;
  _thrustLWanted=thrust-rotThrust;
  _thrustRWanted=thrust+rotThrust;
  Limit(_thrustLWanted,-0.5f,1);
  Limit(_thrustRWanted,-0.5f,1);

  /*
  GlobalShowMessage
  (
    100,"l %5.2f, r %5.2f rot %5.2f, lw %5.2f, rw %5.2f, hcW %5.2f",
    FutureVisualState()._thrustL,FutureVisualState()._thrustR,rotOld,
    _thrustLWanted,_thrustRWanted,headChange
  );
  */

  #if DIAG_SPEED
  if( this==GWorld->CameraOn() )
  {
    LogF("Thrust %.1f L %.1f R %.1f",thrust,_thrustLWanted,_thrustRWanted);
  }
  #endif

  if( fabs(info.headChange)<0.2 && fabs(info.speedWanted)<0.5 )
  {
    if( fabs(speed[2])<0.5 ) _thrustLWanted=_thrustRWanted=0;
    _pilotBrake=true;
  }
  else if( fabs(speed[2])<5 && fabs(info.speedWanted)<0.5 && fabs(info.headChange)<0.5 )
  {
    _pilotBrake=true;
  }
  else
  {
    _pilotBrake=false;
  }
}
#endif //_ENABLE_AI

void ShipWithAI::Simulate( float deltaT, SimulationImportance prec )
{
  // if damaged or upside down, tank is dead
  _isUpsideDown=FutureVisualState().DirectionUp().Y()<0.3;
  _isDead = IsDamageDestroyed();

  base::Simulate(deltaT,prec);
}

#pragma warning(disable:4355)
#pragma warning(disable:4065)

ShipWithAI::ShipWithAI( const EntityAIType *name, Person *driver,bool fullCreate)
:Ship(name,driver, fullCreate),
_stopPosition(VZero)
{
  _stopState = SSNone;
}

ShipWithAI::~ShipWithAI()
{
}


float ShipType::GetFieldCost( const GeographyInfo &info, CombatMode mode ) const
{
  return 1;
}

void ShipType::GetPathPrecision(float speedAtCost, float &safeDist, float &badDist) const
{
  // for a boat it is quite normal to stray off the path slightly
  if (GetShape()->GeometrySphere()<15)
  {
    float maxSpeed = GetMaxSpeedMs();
    float factor = maxSpeed>0 ? speedAtCost / maxSpeed : 1.0f;

    // when the path is planned fast, we want to tolerate more straying off it
    safeDist = Lerp(OperItemGrid*0.5f,OperItemGrid*2.0f,factor);
    badDist = Lerp(OperItemGrid*1.5f,OperItemGrid*4.0f,factor);
  }
  else
  {
    // assume only strategic field planning is done
    safeDist = LandGrid*0.4f;
    badDist = LandGrid*0.8f;
  }
}

bool ShipType::StrategicPlanningOnly() const
{
  return GetShape()->GeometrySphere()>=15;
}

#if _HELISIM_IMPERIAL_SYSTEM
float ShipType::ConvertDistance(float meters, int &stringIDS) const 
{
  if(Glob.config.useImperialSystem == 2)
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

float ShipType::ConvertSpeed(float metersPerSecond) const 
{
  return Glob.config.useImperialSystem == 2? METER_TO_KNOT(metersPerSecond) : METER_TO_KM_PER_HOUR(metersPerSecond);
};
#endif

/*!
\patch 5137 Date 3/6/2007 by Jirka
- Fixed: AI Ships now can reach shallow water (but very carefully)
\patch 5142 Date 3/21/2007 by Ondra
- Fixed: AI boat movement precision and path finding improved.
*/

float ShipType::GetBaseCost(const GeographyInfo &geogr, bool operative, bool includeGradient) const
{
  float cost=GetMinCost(); // basic speed is 13 m/s
  // are we ship or boat?
  if (!GetShape() || GetShape()->GeometrySphere() < 15)
  { // boats
    // if there is no water, we need to avoid the place
    if (geogr.u.maxWaterDepth<1)
    {
      return SET_UNACCESSIBLE; // no water
    }
    if (geogr.u.minWaterDepth<=0)
    {
      // very close to the shore - move very slow
      cost *= 4.0f;
    }
    else if (geogr.u.minWaterDepth<=1)
    {
      // near shore - some shallow water, be careful
      cost *= 2.0f;
    }
    else if (geogr.u.maxWaterDepth<2)
    {
      // near shore - some non that deep water, be careful
      cost *= 1.5f;
    }
    // avoid places with many objects - navigating there could be very hard
    cost *= 1 + geogr.u.howManyObjects;
    cost *= 1 + geogr.u.howManyHardObjects*4;
    cost *= 1 + geogr.u.full*20;
  }
  else
  { // large ships
    if (geogr.u.maxWaterDepth<3)
    {
      return SET_UNACCESSIBLE; // no deep water - do not try to enter
    }
    if (geogr.u.minWaterDepth<=1)
    {
      // some land or very shallow water - be extra careful
      cost *= 16;
    }
    else if (geogr.u.minWaterDepth<=2)
    {
      // some shallow water - be careful
      cost *= 4;
    }
    cost *= 1 + geogr.u.howManyObjects*2;
    cost *= 1 + geogr.u.howManyHardObjects*8;
    cost *= 1 + geogr.u.full*100;
  }
  // avoid any water
  // when shallow/deep water is mixed, we may want to adjust the estimations
  // penalty for objects
  // TODO: some flag for water besides of shallow water

  return cost;
}

float ShipType::GetGradientPenalty(float gradient) const
{
  return 1.0f;
}

float ShipType::GetCostTurn(int difDir, int &debet, bool forceFullTurn) const
{ // in sec
  //return 0;
  if( difDir==0 ) return 0;
  // convert argument into -8..7 range (old direction representation)
  float aDir=fabs(difDir)*(float(15)/255);
  if (GetShape()->GeometrySphere()<15)
  {
    float cost=aDir*10+aDir*aDir*0.5;
    if( difDir<0 ) return cost*0.8;
    return cost;
  }
  else
  {
    float cost=aDir*2.5f+aDir*aDir*0.1f;
    if( difDir<0 ) return cost*0.8f;
    return cost;
  }
}

FieldCost ShipType::GetTypeCost(OperItemType type, CombatMode mode, Clearance clearance) const
{
  if (type==OITWater) return FieldCost(1);
  return FieldCost(SET_UNACCESSIBLE);
}

void Ship::ResetStatus()
{
  base::ResetStatus();
  _sink = 0;
}

void Ship::GetAimActions(float &aimX, float &aimY) const
{
  aimX = GInput.GetActionWithCurve(InputInterface::CurveFovMouse, UACarAimRight, UACarAimLeft);
  aimY = GInput.GetActionWithCurve(InputInterface::CurveFovMouse, UACarAimUp, UACarAimDown);
}

float Ship::GetSteerAheadSimul() const
{
  // avoid functionality in TankOrCar
  return Transport::GetSteerAheadSimul();
}

float Ship::GetSteerAheadPlan() const
{
  // avoid functionality in TankOrCar
  return Transport::GetSteerAheadPlan();
}

float Ship::GetPrecision() const
{
  // avoid functionality in TankOrCar
  return Transport::GetPrecision();
}

#define SERIAL_DEF_VISUALSTATE(name, value) CHECK(ar.Serialize(#name, FutureVisualState()._##name, 1, value))

LSError Ship::Serialize(ParamArchive &ar)
{
  SERIAL_BASE
  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    SERIAL_BITBOOL(pilotBrake, false)
    // WeaponFireSource _mGunFire;
    // WeaponCloudsSource _mGunClouds;
    // WaterSource _leftEngine,_rightEngine;
    // WaterSource _leftWater,_rightWater;
    SERIAL_DEF_VISUALSTATE(thrustL, 0)
    SERIAL_DEF(thrustLWanted, 0)
    SERIAL_DEF_VISUALSTATE(thrustR, 0)
    SERIAL_DEF(thrustRWanted, 0)
    SERIAL_DEF(sink, 0)
    SERIAL_DEF(turnSpeedWanted, 0)
  }
  return LSOK;
}

#undef SERIAL_DEF_VISUALSTATE

static const EnumName StopStateNames[]=
{
  EnumName(ShipWithAI::SSNone, "NONE"),
  EnumName(ShipWithAI::SSFindPath, "FIND PATH"),
  EnumName(ShipWithAI::SSMove, "MOVE"),
  EnumName(ShipWithAI::SSStop, "STOP"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(ShipWithAI::StopState dummy)
{
  return StopStateNames;
}

LSError ShipWithAI::Serialize(ParamArchive &ar)
{
  SERIAL_BASE
  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    SERIAL_EXT_DEF(stopPosition, VZero)
    SERIAL_ENUM(stopState, SSNone)
  }
  return LSOK;
}

#define SHIP_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateShip) \
  XX(UpdatePosition, UpdatePositionShip)

DEFINE_NETWORK_OBJECT(ShipWithAI, base, SHIP_MSG_LIST)

#define UPDATE_SHIP_MSG(MessageName, XX) \
  XX(MessageName, float, thrustLWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted thrust of left engine"), TRANSF, ET_NONE, 0) \
  XX(MessageName, float, thrustRWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted thrust of right engine"), TRANSF, ET_NONE, 0) \
  XX(MessageName, Vector3, stopPosition, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, VZero), DOC_MSG("Anchor position"), TRANSF, ET_NONE, 0) \
  XX(MessageName, int, stopState, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, SSNone), DOC_MSG("Anchor state"), TRANSF, ET_NONE, 0) \
  XX(MessageName, float, sink, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Ship is sinked"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR)

DECLARE_NET_INDICES_EX_ERR(UpdateShip, UpdateTankOrCar, UPDATE_SHIP_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateShip, UpdateTankOrCar, UPDATE_SHIP_MSG, NoErrorInitialFunc)

#define UPDATE_POSITION_SHIP_MSG(MessageName, XX) \

DECLARE_NET_INDICES_EX_ERR(UpdatePositionShip, UpdatePositionVehicle, UPDATE_POSITION_SHIP_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionShip, UpdatePositionVehicle, UPDATE_POSITION_SHIP_MSG, NoErrorInitialFunc)

NetworkMessageFormat &ShipWithAI::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_SHIP_MSG(UpdateShip, MSG_FORMAT_ERR)
    break;
  case NMCUpdatePosition:
    base::CreateFormat(cls, format);
    UPDATE_POSITION_SHIP_MSG(UpdatePositionShip, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError ShipWithAI::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateShip)

      // float _randFrequency;
      // WeaponFireSource _mGunFire;
      // WeaponCloudsSource _mGunClouds;
      // WaterSource _leftEngine,_rightEngine;
      // WaterSource _leftWater,_rightWater;
//      ITRANSF(thrustL)
      TRANSF(thrustLWanted)
//      ITRANSF(thrustR)
      TRANSF(thrustRWanted)
      TRANSF(stopPosition)
      TRANSF_ENUM(stopState)
      TRANSF(sink)
    }
    break;
  case NMCUpdatePosition:
    {
      PREPARE_TRANSFER(UpdatePositionShip)

      Matrix3 oldTrans = FutureVisualState().Orientation();
      TMCHECK(base::TransferMsg(ctx))
      StabilizeTurrets(oldTrans, FutureVisualState().Orientation());
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float ShipWithAI::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += base::CalculateError(ctx);
    {
      //PREPARE_TRANSFER(UpdateShip)
  
      // TODO: implementation
    }
    break;
  case NMCUpdatePosition:
    {
    // TODO: implementation
      error +=  base::CalculateError(ctx);
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}
