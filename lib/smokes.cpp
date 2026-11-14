// smokes (Cloudlets) simulation

#include "wpch.hpp"

#if _PROFILE
# pragma optimize( "", off )
#endif

#include "smokes.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "camera.hpp"
#include "vehicleAI.hpp"
#include "house.hpp"
#include "AI/ai.hpp"
#include "global.hpp"
#include <El/Common/randomGen.hpp>
#include "weapons.hpp"
#include "Network/network.hpp"
#include "gameStateExt.hpp"
#include "paramArchiveExt.hpp"
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>
#include "fileLocator.hpp"
#include "txtPreload.hpp"
#include "Shape/material.hpp"
#include "dynSound.hpp"
#include <El/Evaluator/express.hpp>

#define DEBUG_NAME(x) (x) ? (const char *)(x)->GetDebugName() : "<null>"

//DEFINE_FAST_ALLOCATOR(Smoke)

inline const EntityType *SmokeType()
{
  return GWorld->Preloaded(ETypeSmoke);
}

static RString GetCloudletShape(RString name)
{
  ConstParamEntryPtr entry = (Pars >> "CfgCloudletShapes").FindEntry(name);
  if (entry) name = *entry;
  return GetShapeName(name);
}

Smoke::Smoke( LODShape *shape, const EntityType *type, float duration, float loopedDuration )
:Vehicle(shape,type,CreateObjectId()),
_animation(0),_animationSpeed(duration>0 ? 1/duration : 1e5),_firstLoop(true),
_timeToLive(loopedDuration ? loopedDuration : 0 ),
_alpha(1.0),_fadeValue(0),
_fadeIn(0),_fadeInTime(0),_fadeInInv(1000),
_fadeOut(0),_fadeOutTime(0),_fadeOutInv(1000)
{
  Object::_type=TypeTempVehicle;
  _constantColor.SetA(0);
  SetSimulationPrecision(1.0/5);
  RandomizeSimulationTime();
  SetTimeOffset(0);
}

Smoke::~Smoke()
{
}

/*
SmokeSource::SmokeSource( LODShape *shape, float density, float size )
:CloudletSource(shape,0.3/density),
_density(density),
_size(size),
_sourceSize(size),
_inOutDensity(1),
_timeToLive(30),
_color(HWhite),
_in(1),_inTime(0),_inInv(1),
_out(1),_outTime(1),_outInv(1),
_speed(Vector3(0,1.5+size*2,0))
{
  // avoid particles being too large
  const float maxSize = 1.5;
  if (size>maxSize)
  {
    // increase density instead?
    //SetInterval(GetInterval()*maxSize/_size);
    size = maxSize;
  }
  _size = size;
  base::SetSize(size);
  base::SetFades(0.3,0.5,2.0);
}
*/

SmokeSource::SmokeSource(ParamEntryPar cls)
: CloudletSource(),
_inOutDensity(1),
_speed(VZero)
{
  Load(cls);
}

void SmokeSource::SetSize(float size)
{
  // effects equal to passing size to old constructor
  _sourceSize = size;
  _speed[1] = 1.5 + 2.0 * size;
  // avoid particles being too large
  const float maxSize = 1.5;
  saturateMin(size, maxSize);
  _size = size;
  base::SetSize(size);
}

void SmokeSource::SetDensity(float density)
{
  if (density > 0)
  {
    _density = density;
    SetInterval(0.3 / density);
  }
}

const float MinGeneralize=1.0;

bool SmokeSource::Simulate(
  Vector3Par pos, Vector3Par speed, float deltaT, SimulationImportance prec
)
{
  if( _inTime>0 )
  {
    _inTime-=deltaT;
    _inOutDensity=1-_inTime*_inInv;
  }
  else if( _timeToLive>0 )
  {
    _timeToLive-=deltaT;
    _inOutDensity=1;
  }
  else if( _outTime>0 )
  {
    _outTime-=deltaT;
    _inOutDensity=_outTime*_outInv;
  }
  else
  {
    return true;
  }
  if( EnableVisualEffects(pos,prec) )
  {
    _nextTime-=deltaT;
    while( _nextTime<=0 )
    {
      const Camera &camera=*GLOB_SCENE->GetCamera();
      float dist = camera.Position().Distance(pos);
      float invZoom = camera.Left();

      float generalize = dist*invZoom * GScene->GetSmokeGeneralization();

      saturate(generalize,MinGeneralize,3);
      _nextTime += _interval*generalize*generalize;
      float size05=0.5*_sourceSize;
      Vector3Val windSpeed=GLandscape->GetWindSpeed()*0.5;
      Vector3 speed
      (
        GRandGen.RandomValue()*(1+_sourceSize)-(0.5+size05)+_speed[0],
        GRandGen.RandomValue()-0.5+_speed[1],
        GRandGen.RandomValue()*(1+_sourceSize)-(0.5+size05)+_speed[2]
      );
      Vector3 offset
      (
        (GRandGen.RandomValue()*2-1)*_sourceSize,
        (GRandGen.RandomValue()*2-1)*_sourceSize,
        (GRandGen.RandomValue()*2-1)*_sourceSize
      );
      CloudletSource::SetSize
      (
        (GRandGen.RandomValue()+2)*_size*2
      );
      CloudletSource::SetAlpha
      (
        _inOutDensity*_density*0.7
      );
      //Log("Next time %f, speed %f, off %f",_nextTime,speed.Size(),offset.Size());
      // simulate cloudlet source
      Cloudlet *cloudlet=Drop(pos+offset,speed,generalize);
      if (cloudlet)
      {
        cloudlet->SetSpeed(speed+windSpeed);
        Assert(cloudlet->FutureVisualState().Position().SquareSize()>0.01);
        GWorld->AddCloudlet(cloudlet);
      }
    }
  }
  return false;
}

LSError Smoke::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("animation", _animation, 1))
  CHECK(ar.Serialize("animationSpeed", _animationSpeed, 1))
  CHECK(ar.Serialize("timeToLive", _timeToLive, 1))
  CHECK(ar.Serialize("firstLoop", _firstLoop, 1))
  CHECK(ar.Serialize("alpha", _alpha, 1))
  CHECK(ar.Serialize("fadeValue", _fadeValue, 1))
  CHECK(ar.Serialize("fadeIn", _fadeIn, 1))
  CHECK(ar.Serialize("fadeInTime", _fadeInTime, 1))
  CHECK(ar.Serialize("fadeOut", _fadeOut, 1))
  CHECK(ar.Serialize("fadeOutTime", _fadeOutTime, 1))
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    _fadeInInv = (_fadeIn > 0.0f) ? 1.0f / _fadeIn : 1e9f;
    _fadeOutInv = (_fadeOut > 0.0f) ? 1.0f / _fadeOut : 1e9f;
  }
  return LSOK;
}

void SmokeSource::Load(ParamEntryPar cls)
{
  base::Load(cls);

  _density = cls >> "density";
  _size = cls >> "cloudletSize";
  _sourceSize = cls >> "size";
  _timeToLive = cls >> "timeToLive";
  float in = cls >> "in"; SetIn(in);
  float out = cls >> "out"; SetOut(out);
  _speed[1] = cls >> "initYSpeed";
}

LSError SmokeSource::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(::Serialize(ar, "speed", _speed, 1))
  CHECK(ar.Serialize("density", _density, 1))
  CHECK(ar.Serialize("size", _size, 1))
  CHECK(ar.Serialize("sourceSize", _sourceSize, 1, 0))
  CHECK(ar.Serialize("inOutDensity", _inOutDensity, 1, 1))
  CHECK(ar.Serialize("timeToLive", _timeToLive, 1))
  CHECK(ar.Serialize("in", _in, 1))
  CHECK(ar.Serialize("inTime", _inTime, 1))
  CHECK(ar.Serialize("out", _out, 1))
  CHECK(ar.Serialize("outTime", _outTime, 1))
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    _inInv = (_in > 0.0f) ? 1.0f / _in : 1e9f;
    _outInv = (_out > 0.0f) ? 1.0f / _out : 1e9f;
  }
  return LSOK;
}

IExplosion::IExplosion()
: _minExplosionFactor(0.5), _maxExplosionFactor(0.75),
_exploded(false),
_explosionTime(TIME_MAX)
{
}

void IExplosion::Explode(Time time)
{
  // if there is already an explosion scheduled in a near future, do not change it
  if (_explosionTime > time + 30)
  {
    _explosionTime = time;
  }
  SimulateExplosion();
}

//DEFINE_FAST_ALLOCATOR(SmokeSourceVehicle)
DEFINE_CASTING(SmokeSourceVehicle)

SmokeSourceVehicle::SmokeSourceVehicle
(
  ParamEntryPar cls, float density, float size,
  EntityAI *owner  
)
:Vehicle(NULL,VehicleTypes.New("#smokesource"),CreateObjectId()),
//_fire(GLOB_SCENE->Preloaded(CloudletFire),0.05),
_darkFire(GLOB_SCENE->Preloaded(CloudletBasic),0.05),
_lightTime(0),
_owner(owner),
SmokeSource(cls)
{
  SmokeSource::SetDensity(size);
  SmokeSource::SetSize(size);

  Object::_type=TypeTempVehicle;
  SetSimulationPrecision(_interval);
  SetTimeOffset(0);

  _fire.Load(Pars >> "CfgCloudlets" >> "Explosion");
  _fire.SetSize(size);
  _darkFire.SetSize(size);
  _darkFire.SetColor(Color(0.15,0.15,0.10));
}

void SmokeSourceVehicle::Sound( bool inside, float deltaT )
{
}

void SmokeSourceVehicle::UnloadSound()
{
}

#pragma warning(disable:4723)

LSError SmokeSourceVehicle::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(SmokeSource::Serialize(ar))
  // TODO: ?? serialize _light
  CHECK(ar.Serialize("_lightTime", _lightTime, 1))
  CHECK(ar.SerializeRef("_owner", _owner, 1))
  CHECK(ar.Serialize("_fire", _fire, 1))
  CHECK(ar.Serialize("_darkFire", _darkFire, 1))
  CHECK(ar.Serialize("_minExplosionFactor", _minExplosionFactor, 1, 0.5))
  CHECK(ar.Serialize("_maxExplosionFactor", _maxExplosionFactor, 1, 0.75))
  CHECK(ar.Serialize("_exploded", _exploded, 1))
  CHECK(::Serialize(ar, "_explosionTime", _explosionTime, 1, TIME_MIN));

  return LSOK;
}

static const Color ExploColor(1,1,1);
static const Color ExploAmbient(0.2,0.2,0.2);

static const Color FireColor(1,0.85,0.4);
static const Color FireAmbient(0,0,0);

void SmokeSourceVehicle::SimulateExplosion()
{
  //PROFILE_SCOPE(expSE);
  if (!_exploded && _explosionTime<=Glob.time )
  {
    /*
    LogF("Explosion of %s - time %.3f",DEBUG_NAME(GetObject()),Glob.time.toFloat());
    */
    _exploded=true;
    Ref<EntityType> vType=VehicleTypes.New("FuelExplosion");
    AmmoType *aType=dynamic_cast<AmmoType *>(vType.GetRef());
    if (!aType)
    {
      Fail("No explosion type");
      return;
    }
    // WIP:EXTEVARS:FIXME fill hitInfo properly (normal, dir)
    HitInfo hitInfo(aType, FutureVisualState().Position(), VZero, VUp);
    if (GetObject())
    {
      float gauss=GRandGen.RandomValue()+GRandGen.RandomValue();
      float randomFactor=gauss*(_maxExplosionFactor-_minExplosionFactor)+_minExplosionFactor;

      float hit=GetObject()->GetExplosives()*randomFactor;
      float indirectRatio=hitInfo._indirectHit/hitInfo._hit;
      hitInfo._hit=hit;
      hitInfo._indirectHit=indirectRatio*hit;
      /*
      LogF("%s: Explosion hit %.1f, iHit %.1f, iHitRange %.1f",
        (const char *)GetObject()->GetDebugName(),
        hitInfo._hit,hitInfo._indirectHit,hitInfo._indirectHitRange);
      */
    }
    if (hitInfo._hit>50)
    {
      // change fire and darkFire parameters based on hitInfo._hit value
      float fireSize=hitInfo._hit*0.001;
      float fireTime=hitInfo._hit*0.002;
      saturate(fireSize,0.25,1.25);
      saturate(fireTime,0.25,1.25);
      //LogF("Explosion size %.1f time %.1f",fireSize,fireTime);
      _fire.SetSize(fireSize);
      _darkFire.SetSize(fireSize);
      _fire.Start(fireTime);
      _darkFire.Start(fireTime);
      float rndTime=GRandGen.RandomValue()*0.4+0.8;
      SetSourceTimes(0,40*rndTime,40*rndTime);
      // sound of explosion
      ParamEntryVal par=Pars>>"CfgDestroy">>"EngineHit";
      SoundPars soundPars;
      GetValue(soundPars, par>>"sound");
      float rndFreq=GRandGen.RandomValue()*0.1+0.95;
      AbstractWave *sound=GSoundScene->OpenAndPlayOnce(soundPars.name,NULL,false,FutureVisualState().Position(),VZero,soundPars.vol,soundPars.freq*rndFreq, soundPars.distance);
      if (sound)
      {
        GSoundScene->SimulateSpeedOfSound(sound);
        GSoundScene->AddSound(sound);
      }
      Assert (_owner);
      if (IsLocal())
        // explosion has always a full energy
        GLOB_LAND->ExplosionDamage(_owner, VZero, GetObject(), VZero, hitInfo);
    }
  }
}

void SmokeSourceVehicle::DoResults()
{
}

void SmokeSourceVehicle::UndoResults()
{
}

void SmokeSourceVehicle::Simulate( float deltaT, SimulationImportance prec )
{
  SimulateExplosion();
  bool canDelete=false;
  if( SmokeSource::Simulate(FutureVisualState().Position(),FutureVisualState().Speed(),deltaT,prec) )
  {
    canDelete=true;
  }
  if( _fire.Active() || _darkFire.Active())
  {
    _fire.Simulate(FutureVisualState().Position(),FutureVisualState().Speed(),0.8,deltaT);
    _darkFire.Simulate(FutureVisualState().Position(),FutureVisualState().Speed(),0.8,deltaT);
  }
  else if( _exploded )
  {
    if( canDelete ) SetDeleteRaw();
  }

  if( _fire.Active() && prec<=SimulateInvisibleNear)
  {
    if( !_light )
    {
      _light=new LightPoint(HBlack,HBlack);
      GLOB_SCENE->AddLight(_light);
    }
  }
  if( _light )
  {
    _lightTime+=deltaT*1.0;
    float intensity=(1-fabs(_lightTime-1))*_size;
    saturateMax(intensity,0);
    _light->SetPosition(FutureVisualState().Position());
    _light->SetDiffuse(ExploColor*intensity);
    _light->SetAmbient(ExploAmbient*intensity);
    if( _lightTime>=2.0 )
    {
      _light.Free();
    }
  }
}


//DEFINE_FAST_ALLOCATOR(SmokeSourceOnVehicle)

SmokeSourceOnVehicle::SmokeSourceOnVehicle
(
  ParamEntryPar cls, float density, float size, EntityAI *owner,
  Object *vehicle, Vector3Par position
 )
:SmokeSourceVehicle(cls,density,size,owner),
AttachedOnVehicle(vehicle,position,Vector3(0,0,1))
{
}

void SmokeSourceOnVehicle::CleanUp()
{
  AttachedOnVehicle::Detach();
}

void SmokeSourceOnVehicle::UpdatePosition()
{
  if( _vehicle!=NULL )
  {

    Matrix4 toWorld = _vehicle->WorldTransform(_vehicle->RenderVisualState());
    Matrix4 transf;
    transf.SetPosition(toWorld.FastTransform(_pos));
    transf.SetDirectionAndUp
    (
      toWorld.Rotate(_dir),toWorld.DirectionUp()
    );
    Move(transf);
  }
}

LSError SmokeSourceOnVehicle::Serialize(ParamArchive &ar)
{
  // TODO: CHECK(AttachedOnVehicle::Serialize(ar))
  CHECK(SmokeSourceVehicle::Serialize(ar))

  return LSOK;
}

//DEFINE_FAST_ALLOCATOR(ObjectDestructed)


DEFINE_CASTING(ObjectDestructed)

const float DestroyerTimeToLive=2;

ObjectDestructed::ObjectDestructed(LODShape *shape)
:Vehicle(shape,VehicleTypes.New("#objectdestructed"),CreateObjectId()),
_destroy(NULL),_anim(0),_speed(1.0 / DestroyerTimeToLive)
{
  SetTimeOffset(0);
  //SetSimulationPrecision(0,0);
}


/*!
  \patch 1.30 Date 11/05/2001 by Ondra.
  - Fixed: Bush destruction sound was delayed (since 1.29)
*/
ObjectDestructed::ObjectDestructed(Object *destroy, float timeToLive)
:Vehicle(NULL,VehicleTypes.New("#objectdestructed"),CreateObjectId()),
_destroy(destroy),_anim(0),_speed(1/timeToLive)
{
  SetTimeOffset(0);
  //SetSimulationPrecision(0,0);
}

ObjectDestructed::~ObjectDestructed()
{
}

void ObjectDestructed::Simulate( float deltaT, SimulationImportance prec )
{
  if( !EnableVisualEffects(prec) ) _anim=1;
  if (!_destroy || !_destroy->GetShape())
  {
    SetDeleteRaw();
    return;
  }
  _anim += _speed*deltaT;
  if( _anim>=1 )
  {
    _anim=1;
    SetDeleteRaw();
  }
  // movement should be accelerated, almost quadratic
  const float quadF = 0.8f;
  // interpolate between quadratic and linear, mostly quadratic
  float phase = Square(_anim)*quadF+_anim*(1-quadF);
  // check object destruction class
  // apply special destruction for trees
  // we can never fix the object, we can only break it
  if (_destroy->GetDestroyed()<phase)
  {
    _destroy->SetDestroyed(phase);
  }
}

LSError ObjectDestructed::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.SerializeRef("destroy", _destroy, 1))
  CHECK(ar.Serialize("anim", _anim, 1))
  CHECK(ar.Serialize("destSpeed", _speed, 1))
  return LSOK;
}

#define OBJECT_DESTRUCTED_MSG_LIST(XX) \
  XX(Create, CreateObjectDestructed) \
  XX(UpdateDamage, None)

DEFINE_NETWORK_OBJECT(ObjectDestructed, base, OBJECT_DESTRUCTED_MSG_LIST)

#define CREATE_OBJECT_DESTRUCTED_MSG(MessageName, XX) \
  XX(MessageName, OLink(Object), destroy, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("Destroying object"), TRANSF_REF) \
  XX(MessageName, float, timeToLive, NDTFloat, float, NCTNone, DEFVALUE(float, 2), DOC_MSG("Time to live"), TRANSF) \

DECLARE_NET_INDICES_EX(CreateObjectDestructed, CreateVehicle, CREATE_OBJECT_DESTRUCTED_MSG)
DEFINE_NET_INDICES_EX(CreateObjectDestructed, CreateVehicle, CREATE_OBJECT_DESTRUCTED_MSG)

NetworkMessageFormat &ObjectDestructed::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCCreate:
    base::CreateFormat(cls, format);
    CREATE_OBJECT_DESTRUCTED_MSG(CreateObjectDestructed, MSG_FORMAT)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError ObjectDestructed::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    if (ctx.IsSending())
    {
      TMCHECK(base::TransferMsg(ctx))

      PREPARE_TRANSFER(CreateObjectDestructed)

      TRANSF_REF(destroy)
      float timeToLive = 1.0 / _speed;
      TRANSF_EX(timeToLive, timeToLive)
    }
    break;
  default:
    TMCHECK(base::TransferMsg(ctx))
    break;
  }
  return TMOK;
}

ObjectDestructed *ObjectDestructed::CreateObject(NetworkMessageContext &ctx)
{
  base *veh = base::CreateObject(ctx);
  ObjectDestructed *destroyer = dyn_cast<ObjectDestructed>(veh);
  if (!destroyer) return NULL;

  PREPARE_TRANSFER(CreateObjectDestructed)

  float timeToLive;
  if (TRANSF_REF_BASE(destroy, destroyer->_destroy) != TMOK) return NULL;
  if (TRANSF_BASE(timeToLive, timeToLive) != TMOK) return NULL;

  destroyer->_speed = 1.0 / timeToLive;
  return destroyer;
}



void Cloudlet::Draw(
  int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
  const InstanceParameters &ip, float dist2, const PositionRender &frame, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
)
{
  Assert( level!=LOD_INVISIBLE );
  DrawDecal(cb,level,clipFlags,frame);
}

bool Cloudlet::CanBeInstanced(int level, float distance2, const DrawParameters &dp) const
{
  return true;
}

void Cloudlet::SetupInstances(int cb, const Shape &sMesh, const ObjectInstanceInfo *instances, int nInstances)
{
  DoAssert(sMesh.GetVertexDeclaration() == VD_Tex0);
  GEngine->SetupInstancesSprite(cb,instances, nInstances);
}


void Smoke::Simulate( float deltaT, SimulationImportance prec )
{
  _animation+=deltaT*_animationSpeed;
  if( _animation>=1.0f )
  {
    // too large number have no sense, they are result of bad input parameters - avoid them
    saturate(_animation,0,1000);
    _animation = fastFmod(_animation,1.0f);
    _firstLoop=false;
  }
  if( _fadeInTime>0 )
  {
    _fadeValue=1-_fadeInTime*_fadeInInv;
    _fadeInTime-=deltaT;
  }
  else if( _timeToLive>0 )
  {
    _fadeValue=1.0;
    _timeToLive-=deltaT;
  }
  else if( _fadeOutTime>0 )
  {
    _fadeValue=_fadeOutTime*_fadeOutInv;
    _fadeOutTime-=deltaT;
    _firstLoop=false; // if fading is valid, we never wait for loop
  }
  else
  {
    if( _firstLoop ) _fadeValue=1;
    else
    {
      _fadeValue=0;
      _invisible=true;
      SetDeleteRaw();
    }
  }
  float alpha01 = _alpha*_fadeValue;
  saturate(alpha01,0,1);
  _constantColor.SetA(alpha01);
}

void Smoke::DisappearASAP(float time)
{
  // no time to live, start disappearing
  _timeToLive = 0;

  const float maxFadeOutTime = time;
  if (_fadeInTime>maxFadeOutTime)
  {
    _fadeValue=1-_fadeInTime*_fadeInInv;
    _fadeInTime = 0;
    _fadeOutTime = maxFadeOutTime;
    _fadeOutInv = _fadeValue/maxFadeOutTime;
    _firstLoop=false; // if fading is valid, we never wait for loop
  }
  if( _fadeOutTime>maxFadeOutTime )
  {
    _fadeValue=_fadeOutTime*_fadeOutInv;
    
    _fadeOutTime = maxFadeOutTime;
    _fadeOutInv = _fadeValue/maxFadeOutTime;
    
    _firstLoop=false; // if fading is valid, we never wait for loop
  }
}

AnimationStyle Smoke::IsAnimated( int level ) const {return AnimatedGeometry;}

void Smoke::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  // animate all animated textures
  // normal smoke is single-LOD single face object
  // all relevant information should be contained in sections
  Assert(_shape->IsLevelLocked(level));
  const Shape *shape = _shape->GetLevelLocked(level);
  if (shape->NSections() <= 0) return;

  PolyProperties &sec = animContext.SetSection(shape, 0);
  if( sec.Special()&::IsAnimated )
  {
    sec.AnimateTexture(_animation);
  }
}

void Smoke::Deanimate( int level , bool setEngineStuff)
{
  // unanimated smoke is nonsense and should never exist
}

LSError CloudletTItem::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("maxT", maxT, 1, 0))
  CHECK(::Serialize(ar, "color", color, 1, Color(HBlack)))
  return LSOK;
}

int CloudletTTable::Add(float maxT, Color color)
{
  int index = base::Add();
  Set(index).maxT = maxT;
  Set(index).color = color;
  return index;
}

Color CloudletTTable::GetColor(float t) const
{
  // linear interpolation in table
  if (Size() == 0) return HWhite;

  if (t <= Get(0).maxT) return Get(0).color;
  int index = -1;
  for (int i=1; i<Size(); i++)
  {
    if (t <= Get(i).maxT)
    {
      index = i;
      break;
    }
  }
  if (index < 0) return Get(Size() - 1).color;
  float t0 = Get(index - 1).maxT;
  float t1 = Get(index).maxT;
  float coef = (t - t0) / (t1 - t0);
  return Get(index - 1).color * (1.0 - coef) + Get(index).color * coef;
}

CloudletType::CloudletType(ConstParamEntryPtr entry)
{
  _entry = entry;

  _cloudletTTable = new CloudletTTable();
  if (entry)
  {
    ParamEntryVal table = (*entry) >> "Table";
    int n = table.GetEntryCount();
    _cloudletTTable->Realloc(n);
    _cloudletTTable->Resize(n);
    for (int i=0; i<n; i++)
    {
      ParamEntryVal item = table.GetEntry(i);
      _cloudletTTable->Set(i).maxT = item >> "maxT";
      _cloudletTTable->Set(i).color = ::GetColor(item >> "color");
    }
  }
  else
  {
    _cloudletTTable->Clear();
  }
}

static BankArray<CloudletType> CloudletTypes;

DEFINE_FAST_ALLOCATOR(Cloudlet)

// basic element from which smoke trails are built
Cloudlet::Cloudlet( LODShape *shape, float duration, float loopedDuration )
:Smoke(shape,SmokeType(),duration,loopedDuration),
_size(1),_growSize(0),
_growUpTime(0),_growUpInv(1),
_accY(-0.8),
_minYSpeed(0),_maxYSpeed(5),
_xSpeed0(0),_zSpeed0(0),
_xFriction(0),_zFriction(0),
_t(0),_dt(0),
_cloudletColor(HWhite)
{
  if (shape)
  {
    shape->OrSpecial(IsColored|IsAlpha|IsAlphaFog|IsAlphaOrdered);
  }
}

SimulationImportance Cloudlet::WorstImportance() const
{
  return SimulateVisibleFar;
}

float Cloudlet::CloudletClippingCoef() const
{
  return 10;
}

SimulationImportance Cloudlet::BestImportance() const
{
  return SimulateVisibleFar;
}

void Cloudlet::Simulate( float deltaT, SimulationImportance prec )
{
  if( _growUpTime>0 )
  {
    _growSize=1-_growUpTime*_growUpInv;
    _growUpTime-=deltaT;
  }
  else _growSize=1;
  // simulate acceleration
  FutureVisualState()._speed[1]+=_accY*deltaT;
  Friction(FutureVisualState()._speed[0],_xFriction,0,deltaT);
  Friction(FutureVisualState()._speed[2],_zFriction,0,deltaT);
  Limit(FutureVisualState()._speed[1],_minYSpeed,_maxYSpeed);
  SetOrientScaleOnly(_growSize*_size);
  SetPosition(FutureVisualState().Position()+FutureVisualState()._speed*deltaT);
  //GLOB_LAND->MoveObject(this,Position()+FutureVisualState()._speed*deltaT);

  _t += _dt * deltaT;
  _constantColor = _cloudletColor * _cloudletTTable->GetColor(_t);

  Smoke::Simulate(deltaT,prec);
/*  
  //  int alpha = _constantColor.A8();
  int alpha = toIntFloor(256 * _alpha);
  saturate(alpha, 0, 255);
  _constantColor = PackedColorRGB(_cloudletColor * _cloudletTTable->GetColor(_t), alpha);
*/
}

void Cloudlet::SetTemperature(float t, float dt, CloudletTTable *table)
{
  _t = t;
  _dt = dt;
  _cloudletTTable = table;
}

float Cloudlet::VisibleSize(ObjectVisualState const& vs) const
{
  return vs.Scale()*0.5f;
}
CloudletSource::CloudletSource( LODShape *shape, float interval )
: // cloudlet generator
_cloudletShape(shape),
_interval(interval),_nextTime(0),

// cloudlet parameters
_cloudletDuration(1.0),_cloudletAnimPeriod(3.0),
_cloudletSize(1),_cloudletAlpha(1),
_cloudletGrowUp(1.0),_cloudletFadeIn(0.5),_cloudletFadeOut(2.0),
_cloudletSpeed(VZero),
_cloudletColor(HWhite),
_cloudletInitT(0),
_cloudletDeltaT(0),

_lastPositionValid(false)
{
  SetClimbRate(0,-10,+10);
  _type = CloudletTypes.New(NULL);
}

Cloudlet *CloudletSource::Drop( Vector3Par pos, Vector3Par speed, float relativeSize )
{
  if (!_cloudletShape) return NULL;
  Cloudlet *cloudlet=new Cloudlet(_cloudletShape,_cloudletAnimPeriod,_cloudletDuration);
  cloudlet->SetFades(_cloudletFadeIn,_cloudletFadeOut);
  cloudlet->SetGrowUp(_cloudletGrowUp,_cloudletSize*relativeSize);
  cloudlet->SetAlpha(_cloudletAlpha);
  cloudlet->SetSpeed(_cloudletSpeed+speed);
  cloudlet->SetClimbRate(_cloudletAccY,_cloudletMinYSpeed,_cloudletMaxYSpeed);
  Color colorA0=_cloudletColor;
  colorA0.SetA(0);
  cloudlet->SetColor(PackedColor(colorA0));
  float delta = _cloudletDeltaT * (0.7 + 0.6 * GRandGen.RandomValue());
  cloudlet->SetTemperature(_cloudletInitT, delta, _type->GetTable());
  cloudlet->SetPosition(pos);
  cloudlet->SetOrientScaleOnly(0.0f);
  return cloudlet;
}

void CloudletSource::Simulate(Vector3Par pos, Vector3Par speed, float deltaT)
{
  _nextTime-=deltaT;
  if( _nextTime<=0 )
  {
    const Camera &camera=*GLOB_SCENE->GetCamera();
    float dist=camera.Position().Distance(pos);
    float invZoom =camera.Left();

    float generalize = dist*invZoom * GScene->GetSmokeGeneralization()*0.2f;
    saturate(generalize,MinGeneralize,3);

    _nextTime=_interval*generalize*generalize;
    Cloudlet *cloudlet=Drop(pos,speed,generalize);
    if (cloudlet)
    {
      Assert(cloudlet->FutureVisualState().Position().SquareSize()>0.01);
      GWorld->AddCloudlet(cloudlet);
    }
  }
}

bool NewSprite(const char *name, ShapeParameters pars, Ref<LODShape> &lsws, bool setMaterial, bool forceLoad)
{
  ShapeParameters newPars = pars | (setMaterial ? ShapeModified : ShapeNormal);
  Ref<LODShape> lodShape;
  if (forceLoad)
  {
    lodShape = Shapes.New(name, newPars);
  }
  else
  {
    if (!Shapes.Preload(FileRequestPriority(200), name, newPars, lodShape)) return false;
  }

  lodShape->DisableStreaming();
  if (setMaterial)
  {
    // Finish in case the LODShape has not just one level
    if (lodShape->NLevels() != 1)
    {
      RptF("Error: Model %s cannot be used as a sprite - it has not just one LOD level", name);
      return false;
    }

    // TODO: check if modification of shape is legal
    ShapeUsed lock = lodShape->Level(0);
    Assert(lock.NotNull());
    Shape *shape = lock.InitShape();

    // Consider the first level, if it has got not just one section or just 4 vertices, finish
    if (shape->NSections() != 1)
    {
      RptF("Error: Model %s cannot be used as a sprite - it has not just 1 section", name);
      return false;
    }
    if (shape->NVertex() != 4)
    {
      RptF("Error: Model %s cannot be used as a sprite - it has not just 4 vertices", name);
      return false;
    }

    // Get the RFAddBlend flag from the old material
    const TexMaterial *oldMaterial = shape->GetSection(0).GetMaterialExt();
    bool oldRenderFlagAddBlend = false;
    if (oldMaterial)
    {
      oldRenderFlagAddBlend = oldMaterial->GetRenderFlag(RFAddBlend);
    }

    // Get the proper material
    Ref<TexMaterial> material = GlobPreloadMaterial(oldRenderFlagAddBlend ? SpriteMaterialAddBlend : SpriteMaterial);

    // Set uniform sprite properties
    shape->SetUniformMaterial(material);
    shape->VertexDeclarationNeedsUpdate();
    shape->DisablePushBuffer();
  }

  lodShape->SetAnimationType(AnimTypeNone);
  lsws = lodShape;
  return true;
}

void CloudletSource::Load(ParamEntryPar cls)
{
  _interval = cls >> "interval";
  _cloudletDuration = cls >> "cloudletDuration";
  _cloudletAnimPeriod = cls >> "cloudletAnimPeriod";
  _cloudletSize = cls >> "cloudletSize";
  _cloudletAlpha = cls >> "cloudletAlpha";
  _cloudletGrowUp = cls >> "cloudletGrowUp";
  _cloudletFadeIn = cls >> "cloudletFadeIn";
  _cloudletFadeOut = cls >> "cloudletFadeOut";
  _cloudletAccY = cls >> "cloudletAccY";
  _cloudletMinYSpeed = cls >> "cloudletMinYSpeed";
  _cloudletMaxYSpeed = cls >> "cloudletMaxYSpeed";
  _cloudletColor = ::GetColor(cls >> "cloudletColor");
  _cloudletInitT = cls >> "initT";
  _cloudletDeltaT = cls >> "deltaT";
/*
  ParamEntryVal table = cls >> "Table";
  int n = table.GetEntryCount();
  _cloudletTTable->Realloc(n);
  _cloudletTTable->Resize(n);
  for (int i=0; i<n; i++)
  {
    ParamEntryVal item = table.GetEntry(i);
    _cloudletTTable->Set(i).maxT = item >> "maxT";
    _cloudletTTable->Set(i).color = ::GetColor(item >> "color");
  }
*/
  _type = CloudletTypes.New(cls.GetClassInterface());
  RString cloudletName = cls>>"cloudletShape";
  if (cloudletName.GetLength()>0)
  {
    NewSprite(GetCloudletShape(cls >> "cloudletShape"), ShapeNormal, _cloudletShape);
  }
  else
  {
    _cloudletShape = NULL;
  }
}

static RString PathFirstFolder(const char *path)
{
  const char *next = strchr(path,'/');
  if (!next) return "";
  return RString(path,next-path);
}

RString ConvertContextToCfgPath(RString path)
{
  RString base = PathFirstFolder(path);
  if (!strcmpi(base,Pars.GetName()) )
  {
    RString stripBase = path.Substring(base.GetLength(),INT_MAX);
    return RString("cfg")+stripBase;
  }
  return path;  
}

/*!
  \patch 1.30 Date 11/01/2001 by Ondra.
  - Fixed: Smoke was white after loading game state.
*/
LSError CloudletSource::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("interval", _interval, 1))
  CHECK(ar.Serialize("nextTime", _nextTime, 1))

  // parameters of cloudlets
  if (ar.IsSaving())
  {
    RString name;
    if (_cloudletShape) name = _cloudletShape->Name();
    CHECK(ar.Serialize("cloudletShape", name, 1))
  }
  else if (ar.GetPass() == ParamArchive::PassFirst)
  {
    RString name;
    CHECK(ar.Serialize("cloudletShape", name, 1))
    if (name.GetLength() > 0)
      NewSprite(name, ShapeShadow, _cloudletShape);
    else
      _cloudletShape = NULL;
  }

/*
  if (ar.IsSaving())
  {
    CHECK(ar.Serialize("cloudletTTable", *_cloudletTTable, 1))
  }
  else if (ar.GetPass() == ParamArchive::PassFirst)
  {
    _cloudletTTable = new CloudletTTable;
    CHECK(ar.Serialize("cloudletTTable", *_cloudletTTable, 1))
  }
*/
  if (ar.IsSaving())
  {
    ConstParamEntryPtr entry = _type->GetName();
    RString name;
    RString ConvertContextToCfgPath(RString path);
    if (entry) name = ConvertContextToCfgPath(entry->GetContext());
    CHECK(ar.Serialize("config", name, 1, RString()))
  }
  else if (ar.GetPass() == ParamArchive::PassFirst)
  {
    RString name;
    CHECK(ar.Serialize("config", name, 1, RString()))
    ConstParamEntryPtr entry = NULL;
    ConstParamEntryPtr FindConfigParamEntry(const char *path);
    if (name.GetLength() > 0) entry = FindConfigParamEntry(name);
    _type = CloudletTypes.New(entry);
  }

  CHECK(ar.Serialize("cloudletDuration", _cloudletDuration, 1))
  CHECK(ar.Serialize("cloudletAnimPeriod", _cloudletAnimPeriod, 1))
  CHECK(ar.Serialize("cloudletSize", _cloudletSize, 1))
  CHECK(ar.Serialize("cloudletAlpha", _cloudletAlpha, 1))
  CHECK(ar.Serialize("cloudletGrowUp", _cloudletGrowUp, 1))
  CHECK(ar.Serialize("cloudletFadeIn", _cloudletFadeIn, 1))
  CHECK(ar.Serialize("cloudletFadeOut", _cloudletFadeOut, 1))
  CHECK(ar.Serialize("cloudletAccY", _cloudletAccY, 1))
  CHECK(ar.Serialize("cloudletMinYSpeed", _cloudletMinYSpeed, 1))
  CHECK(ar.Serialize("cloudletMaxYSpeed", _cloudletMaxYSpeed, 1))
  CHECK(::Serialize(ar, "cloudletColor", _cloudletColor, 1))
  CHECK(::Serialize(ar, "cloudletSpeed", _cloudletSpeed, 1))
  CHECK(ar.Serialize("cloudletInitT", _cloudletInitT, 1, 0))
  CHECK(ar.Serialize("cloudletDeltaT", _cloudletDeltaT, 1, 0))
  
  /* TODO: ?? serialize
  Vector3 _lastPosition;
  bool _lastPositionValid;
  */
  return LSOK;
}

void DustSource::Init()
{
  SetClimbRate(-2,-3,3);
  SetColor(Color(0.51,0.46,0.33));
  SetFades(0.2,0.2,1);
  SetTimes(0.5,1.0);
  SetAlpha(0.6);
  SetSize(0.5);
  _generalizeFactor=1;
  _maxGeneralize=3;
  _windCoef = 0.5;
}

DustSource::DustSource( LODShape *shape, float interval )
:CloudletSource(shape,interval)
{
  Init();
}

DustSource::DustSource( float interval )
:CloudletSource(GLOB_SCENE->Preloaded(CloudletBasic),interval),_maxGeneralize(3)
{
  Init();
}

void DustSource::Load(ParamEntryPar cls)
{
  base::Load(cls);
  _size = cls >> "size";
  _sourceSize = cls >> "sourceSize";
}

LSError DustSource::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("size", _size, 1))
  CHECK(ar.Serialize("sourceSize", _sourceSize, 1))
  CHECK(ar.Serialize("maxGeneralize", _maxGeneralize, 1))
  CHECK(ar.Serialize("windCoef", _windCoef, 1))
  // TODO: ?? serialize _generalizeFactor
  return LSOK;
}

WaterSource::WaterSource( LODShape *shape, float interval )
:DustSource(shape,interval)
{
  SetClimbRate(-3,-0.1,2);
  SetColor(HWhite);
  SetFades(0.5,0.2,1.3);
  SetTimes(0.5,1.0);
  SetSize(0.1);
}

WaterSource::WaterSource( float interval )
:DustSource(GLOB_SCENE->Preloaded(CloudletWater),interval)
{
  SetClimbRate(-3,-0.1,2);
  SetColor(HWhite);
  SetFades(0.5,0.2,1.3);
  SetTimes(0.5,1.0);
  SetSize(0.1);
}

void ExhaustSource::Init()
{
  SetClimbRate(-2,-2,2);
  Color color(0.2,0.2,0.5);
  SetColor(color);
  base::SetSize(0.3,0.1);
  // DustSource values
  //SetFades(0.2,0.2,1);
  //SetTimes(0.5,1.0);
  SetFades(0.1,0.2,1);
  SetTimes(0.1,1.0);
}

ExhaustSource::ExhaustSource( LODShape *shape, float interval )
:base(shape,interval)
{
  Init();
}

ExhaustSource::ExhaustSource( float interval )
:base(GLOB_SCENE->Preloaded(CloudletBasic),interval)
{
  Init();
}

void ExhaustSource::SetSize( float size )
{
  base::SetSize(0.3*size,0.1*size);
}

void DustSource::Simulate
(
  Vector3Par pos, Vector3Par speed, float density, float deltaT
)
{
  if( density<0.01 ) return; // no dust
  _nextTime-=deltaT;
  while( _nextTime<=0 )
  {
    const Camera &camera=*GLOB_SCENE->GetCamera();
    float dist=camera.Position().Distance(pos);
    float invZoom =camera.Left();

    float generalize = dist*invZoom * GScene->GetSmokeGeneralization();

    saturate(generalize,MinGeneralize,_maxGeneralize);
    _nextTime += _interval*generalize*generalize;
    const float size2=2*_size;
    const float size05=0.5*_size;
    Vector3Val windSpeed=GLandscape->GetWindSpeed()*_windCoef;
    Vector3 cSpeed(
      (GRandGen.RandomValue()*(1+_size)-(0.5+size05))*2,
      GRandGen.RandomValue()*1+size2*1,
      (GRandGen.RandomValue()*(1+_size)-(0.5+size05))*2
    );
    Vector3 offset(
      (GRandGen.RandomValue()*2-1)*_sourceSize,
      (GRandGen.RandomValue()*2-1)*_sourceSize,
      (GRandGen.RandomValue()*2-1)*_sourceSize
    );
    float cSize=floatMax(_size,_size*3*density);
    CloudletSource::SetSize
    (
      (GRandGen.RandomValue()+2)*cSize
    );
    // simulate cloudlet source
    Cloudlet *cloudlet=Drop(pos+offset, cSpeed*density*0.5+windSpeed+speed, generalize);
    if (cloudlet)
    {
      cloudlet->SetClimbRate(_cloudletAccY,_cloudletMinYSpeed,_cloudletMaxYSpeed);
      cloudlet->SetSideSpeed(0,0,1.5,1.5);
      Assert(cloudlet->FutureVisualState().Position().SquareSize()>0.01);
      GWorld->AddCloudlet(cloudlet);
    }
  }
}

void WeaponCloudsSource::Init()
{
  _generalizeFactor=2;
  SetSize(0.2,0.2);
  SetClimbRate(+0.4,+0.2,+0.8);
  SetFades(0.1,0.01,0.3);
  SetTimes(0.3,1.0);
}

WeaponCloudsSource::WeaponCloudsSource( LODShape *shape, float interval )
:DustSource(shape,interval),_timeToLive(-1)
{
  Init();
}
WeaponCloudsSource::WeaponCloudsSource( float interval )
:DustSource(GLOB_SCENE->Preloaded(CloudletBasic),interval),_timeToLive(-1)
{
  Init();
}


void WeaponCloudsSource::Start( float time )
{
  _timeToLive=time;
}

void WeaponCloudsSource::Simulate
(
  Vector3Par pos, Vector3Par speed, float density, float deltaT
)
{
  if( _timeToLive<0 ) return;
  _timeToLive-=deltaT;

  if( density<0.01 ) return; // no dust
  _nextTime-=deltaT;
  while( _nextTime<=0 )
  {
    const Camera &camera=*GLOB_SCENE->GetCamera();
    float dist=camera.Position().Distance(pos);
    float invZoom=camera.Left();

    float generalize = dist*invZoom * GScene->GetSmokeGeneralization();

    saturate(generalize,MinGeneralize,_maxGeneralize);
    _nextTime += _interval*generalize*generalize;

    // TODO: remove obsolete code
    
    const float size = GetSize();
    const float size2=2*size;
    const float size05=0.5f*size;
    Vector3 cSpeed(
      (GRandGen.RandomValue()*(1+size)-(0.5+size05))*2,
      GRandGen.RandomValue()*1+size2*1,
      (GRandGen.RandomValue()*(1+size)-(0.5+size05))*2
    );
    const float sourceSize = GetSourceSize();
    Vector3 offset(
      (GRandGen.RandomValue()*2-1)*sourceSize,
      (GRandGen.RandomValue()*2-1)*sourceSize,
      (GRandGen.RandomValue()*2-1)*sourceSize
    );
    float cSize=floatMax(size,size*3*density);
    CloudletSource::SetSize
    (
      (GRandGen.RandomValue()+2)*cSize
    );
    // simulate cloudlet source
    Cloudlet *cloudlet=Drop(pos+offset,speed,generalize);
    if (cloudlet)
    {
      float newSize = GetSize() * generalize * (0.5f + 1.0f * GRandGen.RandomValue());
      cloudlet->SetGrowUp(_cloudletGrowUp, newSize);
      Vector3 dir = cloudlet->FutureVisualState().Position() - pos;
      saturateMax(dir[1], 0);

      float speed2 = 4.0f * dir.Size();
      saturateMin(speed2, 25);
      dir.Normalize();

      cloudlet->SetSpeed(speed + speed2 * dir);

      cloudlet->SetClimbRate(_cloudletAccY*density*1.5f,_cloudletMinYSpeed,_cloudletMaxYSpeed);
      cloudlet->SetSideSpeed(0,0,1.5f,1.5f);
      Assert(cloudlet->FutureVisualState().Position().SquareSize()>0.01);
      GWorld->AddCloudlet(cloudlet);
    }
    
  }

}

LSError WeaponCloudsSource::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("timeToLive", _timeToLive, 1))
  return LSOK;
}

WeaponLightSource::WeaponLightSource()
{
  Init();
}

void WeaponLightSource::Init()
{
  _timeToLight=-1;
  _invTotalTimeToLight = -1;
  _lightIntensity = 0;
  _lightMGun = false;
}

void WeaponLightSource::Start(float time, float intensity, bool mGun)
{
  float lightTime=time*3;
  _timeToLight=lightTime;
  _invTotalTimeToLight=1/lightTime;
  _lightIntensity=intensity;
  _lightMGun=mGun;
}

bool WeaponLightSource::Active() const
{
  return _timeToLight>=0 || _light;
}

void WeaponLightSource::Simulate(Vector3Par pos, float deltaT)
{
  if( _timeToLight<0 )
  {
    _light.Free();
  }
  else
  {
    _timeToLight-=deltaT;
    // simulate light
    if( !_light )
    {
      _light=new LightPoint(FireColor,FireAmbient);
      GLOB_SCENE->AddLight(_light);
    }
    if( _light )
    {
      float animation=_timeToLight*_invTotalTimeToLight;
      float intensity=_lightIntensity;
      if( _lightMGun )
      {
        intensity*=GRandGen.PlusMinus(0.8,0.2);
      }
      else
      {
        intensity*=(0.5-fabs(animation-0.5))*2.0*_lightIntensity;
      }

      saturateMax(intensity,0);
      _light->SetPosition(pos);
      _light->SetBrightness(intensity);
    }
  }
}

LSError WeaponLightSource::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("timeToLight", _timeToLight, 1))
  CHECK(ar.Serialize("invTotalTimeToLight", _invTotalTimeToLight, 1))
  CHECK(ar.Serialize("lightIntensity", _lightIntensity, 1))
  
  // TODO: ?? serialize Ref<LightPoint> _light;
  return LSOK;
}

WeaponFireSource::WeaponFireSource( float interval )
:WeaponCloudsSource(GLOB_SCENE->Preloaded(CloudletFire),interval)
{
  Init();
}


WeaponFireSource::WeaponFireSource( LODShape *shape, float interval )
:WeaponCloudsSource(shape,interval)
{
  Init();
}

void WeaponFireSource::Init()
{
  _generalizeFactor=1;
  SetSize(0.5,0.5);
  SetClimbRate(+0.4,+0.2,+0.8);
  SetFades(0.2,0.01,0.5);
  SetTimes(0.2,1.0);
}

LSError WeaponFireSource::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("light",_light,1))
  return LSOK;
}

void WeaponFireSource::Simulate
(
  Vector3Par pos, Vector3Par speed, float density, float deltaT
)
{
  // simulate light
  _light.Simulate(pos,deltaT);
  // simulate clouds
  base::Simulate(pos,speed,density,deltaT);
}

void WeaponFireSource::Start( float time, float intensity, bool lightMGun )
{
  _light.Start(time, intensity, lightMGun);
  base::Start(time);
}

bool WeaponFireSource::Active() const
{
  return _timeToLive>=0 || _light.Active();
}

#if _VBS3_CRATERS_LIMITCOUNT

//! Manager of craters - this class will ensure only limited set of craters can exist in the scene
class CraterManager
{
private:
  //! List of craters in the scene
  TListBidir<Crater, TLinkBidirD, SimpleCounter<Crater> > _craters;
public:
  //! Destructor
  ~CraterManager()
  {
    // Remove all objects from the array (this is rather formal step, as objects probably don't exist in moment of destruction CraterManager)
    while (!_craters.Empty())
    {
      Crater *item = _craters.Start();
      _craters.Delete(item);
    }
  }
  //! Register new crater (must be called only once, in crater constructor)
  void Register(Crater *crater)
  {
    // Possible removal of old craters (typically one crater)
    while (_craters.Size() >= Glob.config.maxPermanentCraters)
    {
      // Get last item from the list
      Crater *item = _craters.End();

      // Remove item from the list
      _craters.Delete(item);

      // Remove item from the scene (this code may or may not invoke the destructor - both should be handled properly)
      item->SetDelete();
    }

    // Add new crater in the list
    _craters.Insert(crater);
  }
  //! Remove crater from the queue (must be used in crater destructor)
  void DeleteItem(Crater *crater)
  {
    if (crater->IsInList()) _craters.Delete(crater);
  }
} GCraterManager;

#endif

#if _VBS3_CRATERS_BIAS

//! Bias manager of craters - this class ensures that different craters use different biases, neighbor craters have biases in reasonable distance to each other
class CraterBiasManager
{
  //! Index to the table of biases
  int _biasIndex;
public:
  //! Constructor
  CraterBiasManager() : _biasIndex(0) {}
  //! Return new bias
  int GetBias()
  {
    // Table of biases with reasonable distance between neighbors
    const int biasTable[10] = {1, 4, 7, 10, 2, 5, 8, 3, 6, 9};

    // Get the result
    int result = biasTable[_biasIndex++];

    // Move the _biasIndex to the beginning, if required
    if (_biasIndex >= lenof(biasTable)) _biasIndex = 0;

    // Return result
    return result;
  }
} GCraterBiasManager;

#endif

//DEFINE_FAST_ALLOCATOR(Crater)
DEFINE_CASTING(Crater)

Crater::Crater(const Matrix4 &transform, LODShape *shape, const EntityType *type,
  RString effectsType, EffectVariables &evars, float effectsTimeToLive, float timeToLive, float size)
#if _VBS3_CRATERS_LATEINIT
  : Smoke(NULL, type, timeToLive, timeToLive)
#else
  : Smoke(shape, type, timeToLive, timeToLive)
#endif
{
  PROFILE_SCOPE_GRF_EX(cratC,*,SCOPE_COLOR_BLUE);

  // Set the crater transformation
  SetTransform(transform);

#if _VBS3_CRATERS_DEFORM_TERRAIN

  // Make sure the transformation is not scaled
  DoAssert(transform.Scale() > 0.99 && transform.Scale() < 1.01);

  // Remember the original shape
  _originalShape = shape;

  // Remember the scale
  _desiredScale = size;

#if _VBS3_CRATERS_LIMITCOUNT
  // If life of the crater should be greater than certain limit, register it in the manager
  static float timeToLiveLimitTreshold = 20.0f;
  if (timeToLive > timeToLiveLimitTreshold) GCraterManager.Register(this);
#endif

#if _VBS3_CRATERS_LATEINIT
  // Register object for late initialization
  {
    // Get distance of a crater from camera position
    const Camera &camera=*GScene->GetCamera();
    float dist = camera.Position().Distance(Position());

    // Creation priority depends on object size and distance to camera
    float priority = size / dist;

    // Register object
    GWorld->RegisterLateInitObject(this, priority);
  }
#else
  // Modify shape to align to ground
  AlignWithGround();
#endif

  // This is here in order to test vehicle collision with craters - without this line is crater of type TypeTempVehicle (see label CRATERCOLISION)
  Object::_type=TypeVehicle;

#endif

#if _VBS3_CRATERS_BIAS
  // Set the bias
  _bias = GCraterBiasManager.GetBias();
#endif

  // Initialize the crater behaviour
  Init(effectsType, evars, effectsTimeToLive, timeToLive, size);
}

Crater::~Crater()
{
#if _VBS3_CRATERS_LIMITCOUNT
  // Remove crater from the list
  GCraterManager.DeleteItem(this);
#endif
}

#if _VBS3_CRATERS_LATEINIT
void Crater::LateInit()
{
  // Modify shape to align to ground
  AlignWithGround();
}
#endif

/**
particle config most often does something like interval = "0.02 * interval + 0.02" or interval = "interval"
in the 1st case limiting interval from below will most likely be not noticeable at all
in the 2nd case it can prevent spawning excessive amounts of particles
*/
static float LimitParticleInterval(float effectsTimeToLive, float interval)
{
  const float particleTimeToLive = 0.1f; // estimated lower bound of a particle life time
  const int maxParticlesPerEffect = 1000; // prevent more then 1000 for one effect
  //if (floatMin(particleTimeToLive/interval,effectsTimeToLive/interval)>maxParticlesPerEffect)
  if (floatMin(particleTimeToLive,effectsTimeToLive)>maxParticlesPerEffect*interval) // optimized to avoid division
  {
    interval = floatMin(particleTimeToLive,effectsTimeToLive)/maxParticlesPerEffect;
  }
  return interval;
}

void Crater::Init(RString effectsType, EffectVariables &evars, float effectsTimeToLive, float timeToLive, float size)
{
  _effectsType = effectsType;
  _effectsTimeToLive = effectsTimeToLive;
  _size = size;
  SetFades(0.1, timeToLive * 0.6);
  if (_effectsType.GetLength() > 0)
  {
    float intensity = 60 * size;
    float interval = 0.03 * intensity;
    interval = LimitParticleInterval(effectsTimeToLive, interval);
    float fireIntensity = 30 * size;
    float fireInterval = 0.03 * fireIntensity;
    PROFILE_SCOPE_EX(cratI,*);
    _effects.Init(_effectsType, this, evars, intensity, interval, fireIntensity, fireInterval, _effectsTimeToLive);
  }
}

void Crater::Simulate( float deltaT, SimulationImportance prec )
{
  PROFILE_SCOPE_EX(simCr,sim)
#if !_VBS3_CRATERS_DEFORM_TERRAIN
  // consider: server-side simulation?
  if( prec>SimulateVisibleFar )
  {
    SetDeleteRaw();
  }
#endif
  _effects.Simulate(deltaT, prec);
  Smoke::Simulate(deltaT, prec);
}

#if _VBS3_CRATERS_DEFORM_TERRAIN

void Crater::AlignWithGround()
{
  if (_originalShape.NotNull())
  {
    // Create copy out of the original shape, forget the current shape
    _shape = new LODShape(*_originalShape, false);

    // Update the shape pointer inside the sort object, as it could point to released now (in case this is the second time we run the AlignWithGround on this object)
    if (_inList.NotNull()) _inList->shape = _shape;

    // This is here to create _faceIndexToOffset array. If aligning takes care about that, perhaps this could be removed
    //_shape->CopyGeometryRelevant(*shape);
    for (int i = 0; i < _shape->NLevels(); i++)
    {
      Shape *s = _shape->InitLevelLocked(i);
      s->BuildFaceIndexToOffset();
      //s->InvalidateNormals();
    }

    // We don't want streaming to be enabled for shapes aligned with ground
    _shape->DisableStreaming();

    // Rescale object
    _shape->InternalTransform(Matrix4(MScale, _desiredScale), false);

    // Align whole shape with ground, use bias value to do shift in Y direction
    _shape->AlignWithGround(GLandscape, *this, _bias);

    // Update the clutter map in the vicinity
    GLandscape->ForceClutterUpdate(this);
    GLandscape->UpdateGrassMap(this);
  }
}

#endif

#if _VBS3_CRATERS_BIAS

int Crater::Bias() const
{
  return _bias;
}

#endif

LSError Crater::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("effectsType", _effectsType, 1, RString()))
  CHECK(ar.Serialize("effectsTimeToLive", _effectsTimeToLive, 1))
  CHECK(ar.Serialize("size", _size, 1))
  CHECK(ar.Serialize("effects", _effects, 1))
  return LSOK;
}

#define CRATER_MSG_LIST(XX) \
  XX(Create, CreateCrater) \
  XX(UpdateGeneric, None) \
  XX(UpdateDamage, None)

DEFINE_NETWORK_OBJECT(Crater, base, CRATER_MSG_LIST)

#define CREATE_CRATER_MSG(MessageName, XX) \
  XX(MessageName, float, timeToLive, NDTFloat, float, NCTNone, DEFVALUE(float, 20), DOC_MSG("Time to live"), TRANSF) \
  XX(MessageName, float, size, NDTFloat, float, NCTNone, DEFVALUE(float, 1), DOC_MSG("Size"), TRANSF) \
  XX(MessageName, RString, effectsType, NDTString, RString, NCTNone, DEFVALUE(RString, RString()), DOC_MSG("Config class with effect description"), TRANSF) \
  XX(MessageName, float, effectsTimeToLive, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("How long effects takes"), TRANSF) \
  XX(MessageName, float, alpha, NDTFloat, float, NCTNone, DEFVALUE(float, 1), DOC_MSG("Transparency"), TRANSF)

DECLARE_NET_INDICES_EX(CreateCrater, CreateVehicle, CREATE_CRATER_MSG)
DEFINE_NET_INDICES_EX(CreateCrater, CreateVehicle, CREATE_CRATER_MSG)

NetworkMessageFormat &Crater::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCCreate:
    base::CreateFormat(cls, format);
    CREATE_CRATER_MSG(CreateCrater, MSG_FORMAT);
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError Crater::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    if (ctx.IsSending())
    {
      TMCHECK(base::TransferMsg(ctx))

      PREPARE_TRANSFER(CreateCrater)

      TRANSF(timeToLive)
      TRANSF(size)
      TRANSF(effectsType)
      TRANSF(effectsTimeToLive)
      TRANSF(alpha)
    }
    break;
  default:
    TMCHECK(base::TransferMsg(ctx))
    break;
  }
  return TMOK;
}

Crater *Crater::CreateObject(NetworkMessageContext &ctx)
{
  Vehicle *veh = base::CreateObject(ctx);
  Crater *crater = dyn_cast<Crater>(veh);
  if (!crater) return NULL;

  PREPARE_TRANSFER(CreateCrater)

  float timeToLive, size, effectsTimeToLive, alpha;
  RString effectsType;
  if (TRANSF_BASE(timeToLive, timeToLive) != TMOK) return NULL;
  if (TRANSF_BASE(size, size) != TMOK) return NULL;
  if (TRANSF_BASE(effectsType, effectsType) != TMOK) return NULL;
  if (TRANSF_BASE(effectsTimeToLive, effectsTimeToLive) != TMOK) return NULL;
  if (TRANSF_BASE(alpha, alpha) != TMOK) return NULL;

  crater->SetTimeToLive(timeToLive);
  crater->_animationSpeed = timeToLive > 0 ? 1.0 / timeToLive : 1e5;
#if !_VBS3_CRATERS_DEFORM_TERRAIN
  crater->SetAlpha(alpha);
#endif
  // WIP:EXTEVARS:FIXME: get the evars parameters via TRANSF_BASE?
  EffectVariables evars(EVarSet_Bullet);
  crater->Init(effectsType, evars, effectsTimeToLive, timeToLive, size);
  return crater;
}

//DEFINE_FAST_ALLOCATOR(CraterOnVehicle)
DEFINE_CASTING(CraterOnVehicle)

CraterOnVehicle::CraterOnVehicle(
  LODShape *shape, const EntityType *type, RString effectsType,
  EffectVariables &evars, float effectsTimeToLive, float timeToLive, float size,
  Object *vehicle, Vector3Par position, Vector3Par direction)
: Crater(M4Identity, shape, type, effectsType, evars, effectsTimeToLive, timeToLive, size),
  AttachedOnVehicle(vehicle, position, direction)
{
  if( _vehicle!=NULL )
  {

    Matrix4 toWorld = _vehicle->WorldTransform(_vehicle->RenderVisualState());
    Matrix4 transf;    
    Vector3 dirWorld = toWorld.Rotate(_dir);
    if (fabs(dirWorld * toWorld.DirectionUp()) < 0.9f) // numerical stability
    {    
      transf.SetDirectionAndUp( dirWorld,toWorld.DirectionUp());
    }
    else
    {
      transf.SetDirectionAndUp(toWorld.DirectionUp(), toWorld.Direction());
      //transf.SetDirectionUp(toWorld.Direction());
      //transf.SetDirectionAside(-toWorld.DirectionAside());       
    }    
    transf.SetScale(_size);
    transf.SetPosition(toWorld.FastTransform(_pos));

    Vehicle::SetTransform(transf);
  }
}

// used for serialization only
CraterOnVehicle::CraterOnVehicle(LODShape *shape, const EntityType *type, EffectVariables& evars)
: Crater(M4Identity, shape, type, RString(), evars, 0.1), //VarSetBulletEffectVariables used because of GCC error: no matching function for call to Crater::Crater...
  AttachedOnVehicle(NULL,VZero,VForward)
{
  if( _vehicle!=NULL )
  {

    Matrix4 toWorld = _vehicle->WorldTransform(_vehicle->RenderVisualState());
    Matrix4 transf;
    Vector3 dirWorld = toWorld.Rotate(_dir);
    if (fabs(dirWorld * toWorld.DirectionUp()) < 0.9f) // numerical stability
    {    
      transf.SetDirectionAndUp( dirWorld,toWorld.DirectionUp());
    }
    else
    {
      transf.SetDirectionAndUp(toWorld.DirectionUp(), toWorld.Direction());
      //transf.SetDirectionUp(toWorld.Direction());
      //transf.SetDirectionAside(-toWorld.DirectionAside());      
    }    
    transf.SetScale(_size);
    transf.SetPosition(toWorld.FastTransform(_pos));
    Vehicle::SetTransform(transf);
  }
}


void CraterOnVehicle::UpdatePosition()
{
  if( _vehicle!=NULL )
  {

    Matrix4 toWorld = _vehicle->WorldTransform(_vehicle->RenderVisualState());
    Matrix4 transf;
    Vector3 dirWorld = toWorld.Rotate(_dir);
    if (fabs(dirWorld * toWorld.DirectionUp()) < 0.9f) // numerical stability
    {    
      transf.SetDirectionAndUp( dirWorld,toWorld.DirectionUp());
    }
    else
    {
      transf.SetDirectionAndUp(toWorld.DirectionUp(),toWorld.Direction());
      //transf.SetDirectionUp(toWorld.Direction());
      //transf.SetDirectionAside(-toWorld.DirectionAside());      
    }    
    transf.SetScale(_size);
    transf.SetPosition(toWorld.FastTransform(_pos));

    Move(transf);
  }
}

LSError CraterOnVehicle::Serialize(ParamArchive &ar)
{
  CHECK(AttachedOnVehicle::Serialize(ar))
  CHECK(Crater::Serialize(ar))
  return LSOK;
}

#define CRATER_ON_VEHICLE_MSG_LIST(XX) \
  XX(Create, CreateCraterOnVehicle)

DEFINE_NETWORK_OBJECT(CraterOnVehicle, base, CRATER_ON_VEHICLE_MSG_LIST)

#define CREATE_CRATER_ON_VEHICLE_MSG(MessageName, XX) \
  XX(MessageName, OLink(Object), vehicle, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("Attached vehicle (where this crater is)"), TRANSF_REF) \
  XX(MessageName, Vector3, pos, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, VZero), DOC_MSG("Position on vehicle"), TRANSF) \
  XX(MessageName, Vector3, dir, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, VUp), DOC_MSG("Orientation on vehicle"), TRANSF)

DECLARE_NET_INDICES_EX(CreateCraterOnVehicle, CreateCrater, CREATE_CRATER_ON_VEHICLE_MSG)
DEFINE_NET_INDICES_EX(CreateCraterOnVehicle, CreateCrater, CREATE_CRATER_ON_VEHICLE_MSG)

NetworkMessageFormat &CraterOnVehicle::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCCreate:
    base::CreateFormat(cls, format);
    CREATE_CRATER_ON_VEHICLE_MSG(CreateCraterOnVehicle, MSG_FORMAT)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError CraterOnVehicle::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    if (ctx.IsSending())
    {
      TMCHECK(base::TransferMsg(ctx))
    }
    {
      PREPARE_TRANSFER(CreateCraterOnVehicle)

      TRANSF_REF(vehicle)
      TRANSF(pos)
      TRANSF(dir)
    }
    break;
  default:
    TMCHECK(base::TransferMsg(ctx))
    break;
  }
  return TMOK;
}

CraterOnVehicle *CraterOnVehicle::CreateObject(NetworkMessageContext &ctx)
{
  base *veh = base::CreateObject(ctx);
  CraterOnVehicle *crater = dyn_cast<CraterOnVehicle>(veh);
  if (!crater) return NULL;

  if (crater->TransferMsg(ctx) != TMOK) return NULL;
  return crater;
}

float ProjectileDecalOnVehicle::MIN_DEPTH = 0.1f;
float ProjectileDecalOnVehicle::MAX_DEPTH = 0.45f;
ProjectileDecalOnVehicle::ProjectileDecalOnVehicle(Object *vehicle, LODShape *shape, const EntityType *type, 
                                                   float hitDepth, SkeletonIndex bone, Vector3Par hitPosLcl,
                                                   Vector3Par hitDirLcl, Vector3Par hitDirUpLcl)
:base(shape, type, CreateObjectId()),
_localTrans(MIdentity),
_vehicle(vehicle),
_bone(bone),
_depth(hitDepth)
{
  const Vector3 localDir(hitDirLcl.Normalized());
  const Vector3 localDirUp(hitDirUpLcl.Normalized());

  _localTrans.SetDirectionAndUp(localDir, localDirUp);
  _localTrans.SetScale(1.f);
  _localTrans.SetPosition(hitPosLcl + localDir * _depth);

  const Matrix4 worldTransf = GetFutureTrans(true);
  Vehicle::SetTransform(worldTransf);
}

void ProjectileDecalOnVehicle::Simulate(float deltaT, SimulationImportance prec)
{
  const Matrix4 modelTrans(GetFutureTrans());
  SetAttachMatrix(modelTrans.Orientation());
  SetAttachedOffset(modelTrans.Position());

  //const ObjectVisualState &vs = _vehicle->FutureVisualState();
  //Vector3 unskinnedPos(_vehicle->FutureVisualState().PositionModelToWorld(_attachedOffset));
  //Vector3 unskinnedDir(_vehicle->FutureVisualState().DirectionModelToWorld(_attachedTrans.Direction()));
  //Vector3 unskinnedDirUp(_vehicle->FutureVisualState().DirectionModelToWorld(_attachedTrans.DirectionUp()));
  //Vector3 unskinnedDirSide(_vehicle->FutureVisualState().DirectionModelToWorld(_attachedTrans.DirectionAside()));
  //enf::Shape::CreateSphere(unskinnedPos, 0.02, 0x001f5f3f, enf::SS_ONCE|enf::SS_WIREFRAME);
  //enf::Shape::CreateLine(unskinnedPos - 0.05f * unskinnedDir, unskinnedPos + 0.4f * unskinnedDir, 0x00cc0000, enf::SS_ONCE);
  //enf::Shape::CreateLine(unskinnedPos - 0.02f * unskinnedDirUp, unskinnedPos + 0.2f * unskinnedDirUp, 0x0000cc00, enf::SS_ONCE);
  //enf::Shape::CreateLine(unskinnedPos - 0.02f * unskinnedDirSide, unskinnedPos + 0.2f * unskinnedDirSide, 0x000000cc, enf::SS_ONCE);
  //enf::Shape::CreatePlane(unskinnedPos, unskinnedDir, 0.06, 0x003f8f6f, enf::SS_ONCE|enf::SS_DOUBLESIDE);
}

Matrix4 ProjectileDecalOnVehicle::GetFutureTrans(bool inWorldSpace /*= false*/) const
{
  if (_vehicle == NULL)
  {
    return MIdentity;
  }

  const ObjectVisualState &vs = _vehicle->FutureVisualState();
  Matrix4 futureTrans = _localTrans;

  if (SkeletonIndexValid(_bone))
  {
    const LODShape *lShape = _vehicle->GetShape();
    const int fireGeom = lShape->FindFireGeometryLevel();
    if (fireGeom>=0)
    {
      Matrix4 modelTrans = MIdentity;
      _vehicle->AnimateBoneMatrix(modelTrans, vs, fireGeom, _bone);
      futureTrans = modelTrans * _localTrans;
    }
  }

  if (inWorldSpace)
  {
    futureTrans = vs.ModelToWorld() * futureTrans;
  }

  return futureTrans;
}

//DEFINE_FAST_ALLOCATOR(Slop)
DEFINE_CASTING(Slop)

Slop::Slop(LODShape *shape, const EntityType *type, const Matrix4 &trans, float timeToLive, float size)
:Smoke(shape, type, timeToLive, timeToLive)
{
  _growSize = 0;
  _origTransform = trans;

  Init(timeToLive, size);
}

void Slop::Init(float timeToLive, float size)
{
  float fadeIn = 0;
  float fadeOut = 0.25 * timeToLive;
  SetFades(fadeIn, fadeOut);
  
  float growUp = 0.11 * timeToLive;
  SetGrowUp(growUp, size);

  SetTimeToLive(timeToLive - fadeIn - fadeOut);
}

void Slop::Simulate(float deltaT, SimulationImportance prec)
{
  _animation += deltaT * _animationSpeed;
  if (_animation >= 1.0) _animation = 1.0;

  if (_growUpTime > 0)
  {
    _growSize = 1 - _growUpTime * _growUpInv;
    _growUpTime -= deltaT;
  }
  else _growSize=1;
  
  float scale = _growSize * _size;
  saturateMax(scale, 0.1);

  Matrix4 trans;
  trans.SetOrientation(_origTransform.Orientation() * scale);
  trans.SetPosition(_origTransform.Position());
  SetTransform(trans);

  if( _fadeInTime>0 )
  {
    _fadeValue=1-_fadeInTime*_fadeInInv;
    _fadeInTime-=deltaT;
  }
  else if( _timeToLive>0 )
  {
    _fadeValue=1.0;
    _timeToLive-=deltaT;
  }
  else if( _fadeOutTime>0 )
  {
    _fadeValue=_fadeOutTime*_fadeOutInv;
    _fadeOutTime-=deltaT;
    _firstLoop=false; // if fading is valid, we never wait for loop
  }
  else
  {
    if( _firstLoop ) _fadeValue=1;
    else
    {
      _fadeValue=0;
      _invisible=true;
      SetDeleteRaw();
    }
  }
  float alpha01 = _alpha*_fadeValue;
  saturate(alpha01, 0, 1);
  _constantColor.SetA(alpha01);

  Entity::Simulate(deltaT, prec);
}

//DEFINE_FAST_ALLOCATOR(Explosion)

static const RStringB ExplosionName="#explosion";

Explosion::Explosion(LODShape *shape, RString effectsName, Vehicle *owner, float duration)
:base(NULL, VehicleTypes.New(ExplosionName), CreateObjectId())
{
  SetSimulationPrecision(1.0/5);
  SetTimeOffset(0);

  if (effectsName.GetLength() > 0)
  {
    float intensity = 0.6 * duration;
    float interval = 0.03 * intensity;
    float fireIntensity = 0.3 * duration;
    float fireInterval = 0.03 * fireIntensity;
    float lifeTime = duration;
    interval = LimitParticleInterval(duration, interval);
    // WIP:EXTEVARS:FIXME: is it enough to pass "empty" evars here?
    EffectVariables evars(EVarSet_Bullet);
    _effects.Init(effectsName, this, evars, intensity, interval, fireIntensity, fireInterval, lifeTime);
  }
}

Explosion::~Explosion()
{
}


void Explosion::Simulate(float deltaT, SimulationImportance prec)
{
  if (prec > SimulateInvisibleNear)
  {
    SetDeleteRaw();
  }
  else if (_effects.Simulate(deltaT, prec))
  {
    SetDeleteRaw();
  }
}


void Explosion::Sound( bool inside, float deltaT  )
{
}

LSError Explosion::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("effects", _effects, 1))
  return LSOK;
}

//--------------------------------------------------------------------------

DEFINE_ENUM(ParticleType, PT, PARTICLE_TYPE_ENUM)

RString GetString(ParamEntryPar cls, const char *name);

DEFINE_FAST_ALLOCATOR(CParticle)

CParticle::CParticle(const CParticleParams &params, const CParticleRandomization &randomization, float relativeSize)
: base(params._shape, SmokeType(), CreateObjectId())
{
  // Create random orientation
  Matrix3 rx(MRotationX, ((float)rand()/RAND_MAX) * 2.0f * H_PI );
  Matrix3 ry(MRotationY, ((float)rand()/RAND_MAX) * 2.0f * H_PI );
  Matrix3 rz(MRotationZ, ((float)rand()/RAND_MAX) * 2.0f * H_PI );
  
  Matrix3 orientation = rz * ry * rx;

  _relativeSize = relativeSize;
  
  // Create random axis to rotate around
  Vector3 dir(((float)rand())/RAND_MAX, ((float)rand())/RAND_MAX, ((float)rand())/RAND_MAX);
  Vector3 up;
  // The unlikely case - all 3 scalars are set to 0 - we will take a x axis (or anything else)
  if ((dir[0] == 0.0f) && (dir[1] == 0.0f) && (dir[2] == 0.0f)) {
    dir[0] = 1.0f;
    up = Vector3(0.0f, 1.0f, 0.0f);
  }
  else {
    up = dir.CrossProduct(Vector3(1.0f, 0.0f, 0.0f));
    // The unlikely case - Up lies on the x axis - we will use y axis for the scalar product (or anything else)
    if ((up[0] == 0.0f) && (up[1] == 0.0f) && (up[2] == 0.0f)) {
      up = dir.CrossProduct(Vector3(0.0f, 1.0f, 0.0f));
    }
  }
  Matrix3 rotOrientation0;
  rotOrientation0.SetDirectionAndUp(dir, up);
  _invRotOrientation0 = rotOrientation0.InverseRotation();
  _rotOrientation0_m_Orientation = rotOrientation0 * orientation;

  // Assign type and randomization
  _type = params._type;
  _randomization = randomization;

  _timerPeriod = _type->_timerPeriod;
  _rubbing = _type->_rubbing;
  _weight = _type->_weight;
  _volume = _type->_volume;
  _sizeCoef = _type->_sizeCoef;
  _colorCoef = _type->_colorCoef;
  _animationSpeedCoef = _type->_animationSpeedCoef;

  // Animation
  if (params._animationName.GetLength() > 0)
  {
    Fail("Skeletal animation not supported for particles");
  }

  // Alpha setting
  if (_type->_type==PTBillboard)
  {
    const int cloudletSpec=ClampU|ClampV|NoZWrite|IsAlpha|IsAlphaFog|NoShadow|IsColored;
    params._shape->OrSpecial(cloudletSpec);
  }

  // Initialize rest of the values
  _age = 0.0f;
  _rotation = 0.0f;
  _animationPhase = 0.0f;
  _lastOnTimerScriptCalling = 0.0f;
  _frameSource = params._frameSource;

  // handle a special case - huge animation speed with non-looped animation is used to skip a few frames.
  float speed = _type->_animationSpeed.Size()>0 ? _type->_animationSpeed[0] : 0;
  if (speed>=1000 && !_frameSource._loop)
  {
    _animationPhase = 1.0f;
  }

  UpdateRenderingState(0);
}

CParticle::~CParticle()
{
}

/*!
  This macro will return value of index (index*(aa->Size()-1)). The value will be linearly
  interpolated from 2 values around. That's why this macro needs items in the
  AutoArray to be allowed to add by themselves and multiply by a float.
  Note that this macro requires at least one item to be in the AutoArray.
  \param aa AutoArray to return value from.
  \param index Index of the desired item from the range <0,1>
  \param value Variable to return a value in.
*/
#define AA_GETVALUELINEAR(aa,index,value) \
{ \
  int lastItemIndex = aa.Size() - 1; \
  Assert(lastItemIndex >= 0); \
  if (lastItemIndex >= 0) \
  { \
    float floatIndex = (float)lastItemIndex*(index); \
    int i = toIntFloor(floatIndex); \
    float fractionPart = floatIndex - i; \
    if (i < lastItemIndex) \
    { \
    value = aa[i] + (aa[i + 1] - aa[i]) * fractionPart; \
    } \
    else \
    { \
    value = aa[i]; \
    } \
  } \
}


void CParticle::UpdateRenderingState(float shrinkedAge)
{
  // Size
  float auxFloat = 1.0f;
  AA_GETVALUELINEAR(_type->_size, shrinkedAge, auxFloat);
  auxFloat *= _sizeCoef * _randomization._sizeCoef;
  // avoid zero size
  saturateMax(auxFloat,1e-6);

  // Rotation
  Matrix3 rotZ;
  rotZ.SetRotationZ(_rotation * 2 * H_PI);
  Matrix3 orientation = _rotOrientation0_m_Orientation * rotZ * _invRotOrientation0;
  // --- Apply all transformations ---
  Matrix4 trans;
  trans.SetIdentity();
  trans.SetOrientation(orientation * auxFloat * _relativeSize);
  trans.SetPosition(_randomization._position);
  SetTransform(trans);

  // --- Set color ---
  Color auxColor = Color(0, 0, 0, 0);
  AA_GETVALUELINEAR(_type->_color, shrinkedAge, auxColor);
  auxColor.SetR(auxColor.R() * _colorCoef.R() * _randomization._colorCoef.R());
  auxColor.SetG(auxColor.G() * _colorCoef.G() * _randomization._colorCoef.G());
  auxColor.SetB(auxColor.B() * _colorCoef.B() * _randomization._colorCoef.B());
  auxColor.SetA(auxColor.A() * _colorCoef.A() * _randomization._colorCoef.A());
  // we want to allow alpha to be negative as well
  auxColor.SaturateRGBMinMax(0,1);
  _constantColor = auxColor;
}

void CParticle::Simulate(float deltaT, SimulationImportance prec)
{
  ADD_COUNTER(pcount,1);

  float shrinkedAge = _randomization._lifeTime > 0 ? _age / _randomization._lifeTime : 0.5f;

  // --- Update age and return in case it exceeded lifetime. ---
  _age += deltaT;
  if (_age > _randomization._lifeTime)
  {
    // BeforeDestroy script calling
    if (_type->_beforeDestroyScript.GetLength() > 0)
    {
      // Create parameters of the script
      GameArrayType GA(3);
      GA.Resize(3);
      GA[0] = _randomization._position[0];
      GA[1] = _randomization._position[2];
      GA[2] = _randomization._position[1] - GLandscape->SurfaceYAboveWater(_randomization._position.X(), _randomization._position.Z());
      GameValue GV(GA);
      // Create a instance of the script and call it
#if USE_PRECOMPILATION
      const char *ext = strrchr(_type->_beforeDestroyScript, '.');
      if (ext && stricmp(ext, ".sqf") == 0)
      {
        RString FindScript(RString name);
        RString fullName = FindScript(_type->_beforeDestroyScript);
        if (fullName.GetLength() > 0)
        {
          ScriptVM *script = new ScriptVM(fullName, GV, GWorld->GetMissionNamespace()); // mission namespace
          GWorld->AddScriptVM(script, false);
        }
      }
      else
#endif
      {
        Script *script = new Script(_type->_beforeDestroyScript, GV, GWorld->GetMissionNamespace()); // mission namespace
        GWorld->AddScript(script, false);
      }
    }
    SetDelete();
    return;
  }

  // --- OnTimer script calling ---
  float age_Minus_TimerPeriod = _age - _timerPeriod;
  while (_lastOnTimerScriptCalling < age_Minus_TimerPeriod) {
    
    if (_timerPeriod < 0.001)
    {
      RptF("Warning: Script %s not executed, param 'timePeriod' value is to small(%.4f)", cc_cast(_type->_onTimerScript), _timerPeriod);
      break;
    }

    if (_type->_onTimerScript.GetLength() > 0)
    {
      // Create parameters of the script
      GameArrayType GA(3);
      GA.Resize(3);
      GA[0] = _randomization._position[0];
      GA[1] = _randomization._position[2];
      GA[2] = _randomization._position[1] - GLandscape->SurfaceYAboveWater(_randomization._position.X(), _randomization._position.Z());
      GameValue GV(GA);
      // Create a instance of the script and call it
#if USE_PRECOMPILATION
      const char *ext = strrchr(_type->_onTimerScript, '.');
      if (ext && stricmp(ext, ".sqf") == 0)
      {
        RString FindScript(RString name);
        RString fullName = FindScript(_type->_onTimerScript);
        if (fullName.GetLength() > 0)
        {
          ScriptVM *script = new ScriptVM(fullName, GV, GWorld->GetMissionNamespace()); // mission namespace
          GWorld->AddScriptVM(script, false);
        }
      }
      else
#endif
      {
        Script *script = new Script(_type->_onTimerScript, GV, GWorld->GetMissionNamespace()); // mission namespace
        GWorld->AddScript(script, false);
      }
    }
    _lastOnTimerScriptCalling += _timerPeriod;
  }

  // --- Update position ---
  // ... assign and prevent overflowing ...

  Vector3 windVelocity = GLandscape->GetWindSpeed();
  Vector3 windRelVelocity = _randomization._moveVelocity-windVelocity;

  // Gravity
  windRelVelocity += Vector3(0.0f, - G_CONST * deltaT, 0.0f);

  // Wind and rubbing
  // friction can never reverse movement direction
  float rubbing = _rubbing / (1.0f + _weight);
  Friction(windRelVelocity, windRelVelocity * windRelVelocity.Size() * rubbing, VZero, deltaT);

  // Lift vector using lift force
  float density = _volume / _weight;
  windRelVelocity += Vector3(0.0f, (1.275 * density) * G_CONST * deltaT, 0.0f);

  // make sure particle never moves way too fast
  const float maxSpeed = 1e4f;
  if (windRelVelocity.SquareSize()>Square(maxSpeed))
  {
    LogF(
      "Fast moving particle %s: %g,%g,%g",
      cc_cast(GetDebugName()),windRelVelocity.X(),windRelVelocity.Y(),windRelVelocity.Z()
    );
    windRelVelocity *= windRelVelocity.InvSize() * maxSpeed;
  }
  Vector3 moveVelocity = windVelocity + windRelVelocity;
  
  // Random value
  if (_randomization._randomDirectionPeriod > 0 && ((float)rand()/RAND_MAX) < (deltaT/_randomization._randomDirectionPeriod)) {
    float rIntensity = _randomization._randomDirectionIntensity;
    moveVelocity[0] += ((float)rand()/RAND_MAX) * rIntensity * 2 - rIntensity;
    moveVelocity[1] += ((float)rand()/RAND_MAX) * rIntensity * 2 - rIntensity;
    moveVelocity[2] += ((float)rand()/RAND_MAX) * rIntensity * 2 - rIntensity;
  }
  // Update position
  _randomization._position += moveVelocity * deltaT;
  _randomization._moveVelocity = moveVelocity;

  // --- Update orientation ---
  // ...assign ...
  float rotationVelocity = _randomization._rotationVelocity;
  // Rubbing and weight
  rotationVelocity += (-rotationVelocity * rubbing) * deltaT;
  // Update rotation
  _rotation += rotationVelocity * deltaT;
  _randomization._rotationVelocity = rotationVelocity;

  // --- Update animation phase ---
  float speed = 0.0f;
  AA_GETVALUELINEAR(_type->_animationSpeed, shrinkedAge, speed);
  speed *= _animationSpeedCoef;
  _animationPhase += speed * deltaT;
  if (_frameSource._loop)
  {
    _animationPhase = _animationPhase - toIntFloor(_animationPhase); // Make sure the phase is in <0, 1) interval
  }
  else
  {
    saturateMin(_animationPhase, 1.0f);
  }

  // Orientation - this will set orientation of the particle in the direction of movement
  ///Orientation.SetDirectionAndUp(_MoveVelocity, Vector3(0,1,0));
  //Orientation.Orthogonalize();


  UpdateRenderingState(shrinkedAge);
}

AnimationStyle CParticle::IsAnimated(int level) const
{
  switch (_type->_type)
  {
    case PTBillboard:
      return AnimatedGeometry;
    default:
      return base::IsAnimated(level);
  }
}

void CParticle::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  switch (_type->_type)
  {
    case PTBillboard:
      return;
    default:
      base::Animate(animContext,level,setEngineStuff,parentObject,dist2);
      return;
  }
}

void CParticle::Deanimate(int level, bool setEngineStuff)
{
  switch (_type->_type)
  {
    case PTBillboard:
      return;
    default:
      base::Deanimate(level,setEngineStuff);
      return;
  }
}

bool CParticle::CanBeInstanced(int level, float distance2, const DrawParameters &dp) const
{
  switch (_type->_type)
  {
    case PTBillboard:
      return true;
    default:
      return base::CanBeInstanced(level, distance2, dp);
  }
}

void CParticle::SetupInstances(int cb, const Shape &sMesh, const ObjectInstanceInfo *instances, int nInstances)
{
  switch (_type->_type)
  {
    case PTBillboard:
      DoAssert(sMesh.GetVertexDeclaration() == VD_Tex0);
      GEngine->SetupInstancesSprite(cb,instances, nInstances);
      break;
    default:
      DoAssert(sMesh.GetVertexDeclaration() != VD_Tex0);
      return base::SetupInstances(cb,sMesh, instances, nInstances);
  }
}

void CParticle::Draw(
  int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
  const InstanceParameters &ip, float dist2, const PositionRender &frame, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
  , int drawListId
#endif
)
{
  ShapeUsed sShape = _shape->Level(level);

  Assert(level != LOD_INVISIBLE);
  switch (_type->_type)
  {
  case PTBillboard:
    DoAssert(sShape->GetVertexDeclaration() == VD_Tex0);
    DrawDecal(cb,level, clipFlags, frame);
    break;
  default:
    DoAssert(sShape->GetVertexDeclaration() != VD_Tex0);
    base::Draw(cb,level, clipFlags, dp, ip, dist2, frame, oi
#if _HELISIM_RENDER_TO_TEXTURE
      , drawListId
#endif
      );
  }
}

bool CParticle::GetFrameSource(FrameSource &frameSource) const
{
  frameSource = _frameSource;
  return true;
}

float CParticle::GetAnimationPhase() const
{
  return _animationPhase;
}

float CParticle::VisibleSize(ObjectVisualState const& vs) const
{
  // assume solid objects are too small or too fast to be interesting for us
  if (_type->_type == PTSpaceObject)
    return 0.0f;
  // when the lifetime is low, it is highly unlikely any real "occlusion" can be done
  // low lifetime is typical for temporary visual effects like ground
  if (_type->_lifeTime < 1.0f)
    return 0.0f;
  // for billboard style particles scale is a diameter - we need to check radius here, cf. VSSprite
  return vs.Scale() * 0.5f;
}

bool ExpressionCodeVector::Compile(ParamEntryPar entry, const RString *varNames, int varCount)
{
  if (entry.GetSize() != 3)
  {
    // error recovery - VZero
    _data[0].Init(0);
    _data[1].Init(0);
    _data[2].Init(0);
    return false;
  }
  bool ok = true;
  if (!_data[0].Compile(entry[0], varNames, varCount)) ok = false;
  if (!_data[1].Compile(entry[1], varNames, varCount)) ok = false;
  if (!_data[2].Compile(entry[2], varNames, varCount)) ok = false;
  return ok;
}

bool ExpressionCodeVector::Evaluate(Vector3 &result, const float *varValues, int varCount) const
{
  bool ok = true;
  float val;
  if (!_data[0].Evaluate(val, varValues, varCount)) ok = false;
  result[0] = val;
  if (!_data[1].Evaluate(val, varValues, varCount)) ok = false;
  result[1] = val;
  if (!_data[2].Evaluate(val, varValues, varCount)) ok = false;
  result[2] = val;
  return ok;
}

bool ExpressionCodeColor::Compile(ParamEntryPar entry, const RString *varNames, int varCount)
{
  bool ok = true;
  if (entry.GetSize() == 4)
  {
    // alpha given as expression
    if (!_data[3].Compile(entry[3], varNames, varCount)) ok = false;
  }
  else if (entry.GetSize() == 3)
  {
    // alpha == 1
    _data[3].Init(1);
  }
  else
  {
    // error recovery - HWhite
    _data[0].Init(1);
    _data[1].Init(1);
    _data[2].Init(1);
    _data[3].Init(1);
    return false;
  }
  // RGB items
  if (!_data[0].Compile(entry[0], varNames, varCount)) ok = false;
  if (!_data[1].Compile(entry[1], varNames, varCount)) ok = false;
  if (!_data[2].Compile(entry[2], varNames, varCount)) ok = false;
  return ok;
}

bool ExpressionCodeColor::Compile(const IParamArrayValue &entry, const RString *varNames, int varCount)
{
  bool ok = true;
  if (entry.GetItemCount() == 4)
  {
    // alpha given as expression
    if (!_data[3].Compile(entry[3], varNames, varCount)) ok = false;
  }
  else if (entry.GetItemCount() == 3)
  {
    // alpha == 1
    _data[3].Init(1);
  }
  else
  {
    // error recovery - HWhite
    _data[0].Init(1);
    _data[1].Init(1);
    _data[2].Init(1);
    _data[3].Init(1);
    return false;
  }
  // RGB items
  if (!_data[0].Compile(entry[0], varNames, varCount)) ok = false;
  if (!_data[1].Compile(entry[1], varNames, varCount)) ok = false;
  if (!_data[2].Compile(entry[2], varNames, varCount)) ok = false;
  return ok;
}

bool ExpressionCodeColor::Evaluate(Color &result, const float *varValues, int varCount) const
{
  bool ok = true;
  float val;
  if (!_data[0].Evaluate(val, varValues, varCount)) ok = false;
  result.SetR(val);
  if (!_data[1].Evaluate(val, varValues, varCount)) ok = false;
  result.SetG(val);
  if (!_data[2].Evaluate(val, varValues, varCount)) ok = false;
  result.SetB(val);
  if (!_data[3].Evaluate(val, varValues, varCount)) ok = false;
  result.SetA(val);
  return ok;
}


// ParticleSourceType identifier: name and corresponding set of variables
struct ParticleSourceName
{
  RString name;
  EVarSet evarSet;
};

typedef const ParticleSourceName &ParticleSourceNamePar;


/// structure containing shared parameters of CParticleSourceBase, cached in the bank
struct ParticleSourceType : public RefCount
{
  ParticleSourceName _pname;

  // Fixed members
  // SetParams
  RString _shapeName;
  int _particleFSNtieth;
  int _particleFSIndex;
  int _particleFSFrameCount;
  bool _particleFSLoop;
  RString _animationName;
  // Values
  ParticleType _particleType;
  RString _onTimerScript;
  RString _beforeDestroyScript;
  bool _onSurface;
  // SetRandom
  float _angleVar;
  float _sizeVar;

  // Expressions
  // SetInterval
  ExpressionCode _interval;
  // SetCircle
  ExpressionCode _circleRadius;
  ExpressionCodeVector _circleVelocity;
  // Values
  ExpressionCode _timerPeriod;
  ExpressionCode _lifeTime;
  ExpressionCodeVector _moveVelocity;
  ExpressionCode _rotationVelocity;
  ExpressionCode _weight;
  ExpressionCode _volume;
  ExpressionCode _rubbing;
  ExpressionCode _angle;
  AutoArray<ExpressionCode> _size;
  AutoArray<ExpressionCodeColor> _color;
  AutoArray<ExpressionCode> _animationSpeed;
  ExpressionCode _randomDirectionPeriod;
  ExpressionCode _randomDirectionIntensity;
  // SetRandom
  ExpressionCode _lifeTimeVar;
  ExpressionCodeVector _positionVar;
  ExpressionCodeVector _moveVelocityVar;
  ExpressionCode _rotationVelocityVar;
  ExpressionCodeColor _colorVar;
  ExpressionCode _randomDirectionPeriodVar;
  ExpressionCode _randomDirectionIntensityVar;

  ParticleSourceType(ParticleSourceNamePar pname);
  ParticleSourceNamePar GetName() const {return _pname; }
};


static inline ParamEntryVal Check(ParamEntryVal cls, RString name)
{
  ParamEntryVal entry = cls >> name;
#if _ENABLE_CHEATS
  if (entry.IsArray())
  {
    RptF("Simple expressions: Polygons no longer supported in %s", cc_cast(cls.GetContext(name)));
  }
#endif
  return entry;
}

static inline ParamEntryVal CheckVector(ParamEntryVal cls, RString name)
{
  ParamEntryVal entry = cls >> name;
#if _ENABLE_CHEATS
  if (!entry.IsArray() || entry.GetSize() != 3)
  {
    RptF("Simple expressions: array of 3 expressions expected in %s", cc_cast(cls.GetContext(name)));
  }
  for (int i=0; i<entry.GetSize(); i++)
  {
    if (entry[i].IsArrayValue())
    {
      RptF("Simple expressions: Polygons no longer supported in %s[i]", cc_cast(cls.GetContext(name)), i);
    }
  }
#endif
  return entry;
}

static inline ParamEntryVal CheckColor(ParamEntryVal cls, RString name)
{
  ParamEntryVal entry = cls >> name;
#if _ENABLE_CHEATS
  if (!entry.IsArray() || entry.GetSize() != 3 && entry.GetSize() != 4)
  {
    RptF("Simple expressions: array of 3 or 4 expressions expected in %s", cc_cast(cls.GetContext(name)));
  }
  for (int i=0; i<entry.GetSize(); i++)
  {
    if (entry[i].IsArrayValue())
    {
      RptF("Simple expressions: Polygons no longer supported in %s[i]", cc_cast(cls.GetContext(name)), i);
    }
  }
#endif
  return entry;
}

static inline ParamEntryVal CheckArray(ParamEntryVal cls, RString name)
{
  ParamEntryVal entry = cls >> name;
#if _ENABLE_CHEATS
  if (!entry.IsArray())
  {
    RptF("Simple expressions: array of expressions expected in %s", cc_cast(cls.GetContext(name)));
  }
  //LogF("CheckArray(): %s has size %d", cc_cast(cls.GetContext(name)), entry.GetSize());
  for (int i=0; i<entry.GetSize(); i++)
  {
    if (entry[i].IsArrayValue())
    {
      RptF("Simple expressions: Polygons no longer supported in %s[%d]", cc_cast(cls.GetContext(name)), i);
    }
  }
#endif
  return entry;
}

static inline ParamEntryVal CheckArrayColor(ParamEntryVal cls, RString name)
{
  ParamEntryVal entry = cls >> name;
#if _ENABLE_CHEATS
  if (!entry.IsArray())
  {
    RptF("Simple expressions: array of colors expected in %s", cc_cast(cls.GetContext(name)));
  }
  //LogF("CheckArrayColor(): %s has size %d", cc_cast(cls.GetContext(name)), entry.GetSize());
  for (int i=0; i<entry.GetSize(); i++)
  {
    const IParamArrayValue &value = entry[i];
    if (!value.IsArrayValue() || value.GetItemCount() != 3 && value.GetItemCount() != 4)
    {
      RptF("Simple expressions: array of 3 or 4 expressions expected in %s[%d]", cc_cast(cls.GetContext(name)), i);
      for (int j=0; j<value.GetItemCount(); j++)
      {
        if (value[j].IsArrayValue())
        {
          RptF("Simple expressions: Polygons no longer supported in %s[%d][%d]", cc_cast(cls.GetContext(name)), i, j);
        }
      }
    }
  }
#endif
  return entry;
}

// compile helpers
#define COMPILE_EXPR(NAME, CHECKFN) \
  if (!_##NAME.Compile(CHECKFN(cls, #NAME), varNames, varCount)) \
    RptF("Error during compilation of %s", cc_cast(cls.GetContext(#NAME)))
#define COMPILE_EXPR_ARRAY(NAME, CHECKFN) \
  ParamEntryVal array_##NAME = CHECKFN(cls, #NAME); \
  int n_##NAME = array_##NAME.GetSize(); \
  _##NAME.Realloc(n_##NAME); \
  _##NAME.Resize(n_##NAME); \
  for (int i = 0; i < n_##NAME; i++) \
    if (!_##NAME[i].Compile(array_##NAME[i], varNames, varCount)) \
      RptF("Error during compilation of element %d of %s", i, cc_cast(cls.GetContext(#NAME)))

ParticleSourceType::ParticleSourceType(ParticleSourceNamePar pname)
: _pname(pname)
{
  ParamEntryVal cls = Pars >> "CfgCloudlets" >> pname.name;

  // SetParams
  _shapeName = GetShapeName(cls >> "particleShape");
  _particleFSNtieth = cls.ReadValue("particleFSNtieth", 1);
  _particleFSIndex = cls.ReadValue("particleFSIndex", 0);
  _particleFSFrameCount = cls.ReadValue("particleFSFrameCount", 1);
  _particleFSLoop = cls.ReadValue("particleFSLoop", true);
  _animationName = cls >> "animationName";
  // Values
  RString type = cls >> "particleType";
  _particleType = GetEnumValue<ParticleType>(cc_cast(type));
  _onTimerScript = cls >> "onTimerScript";
  _beforeDestroyScript = cls >> "beforeDestroyScript";
  _onSurface = cls.ReadValue("onSurface", true);
  // SetRandom
  _angleVar = cls >> "angleVar";
  _sizeVar = cls >> "sizeVar";

  EVarSet evarSet = pname.evarSet;
  const RString *varNames = EffectVariables::GetNames(evarSet);
  int varCount = EffectVariables::GetCount(evarSet);
  // SetInterval
  COMPILE_EXPR(interval, Check);
  // SetCircle
  COMPILE_EXPR(circleRadius, Check);
  COMPILE_EXPR(circleVelocity, CheckVector);
  // Values
  COMPILE_EXPR(timerPeriod, Check);
  COMPILE_EXPR(lifeTime, Check);
  COMPILE_EXPR(moveVelocity, CheckVector);
  COMPILE_EXPR(rotationVelocity, Check);
  COMPILE_EXPR(weight, Check);
  COMPILE_EXPR(volume, Check);
  COMPILE_EXPR(rubbing, Check);
  COMPILE_EXPR(angle, Check);
  COMPILE_EXPR_ARRAY(size, CheckArray);
  COMPILE_EXPR_ARRAY(color, CheckArrayColor);
  COMPILE_EXPR_ARRAY(animationSpeed, CheckArray);
  COMPILE_EXPR(randomDirectionPeriod, Check);
  COMPILE_EXPR(randomDirectionIntensity, Check);
  // SetRandom
  COMPILE_EXPR(lifeTimeVar, Check);
  COMPILE_EXPR(positionVar, CheckVector);
  COMPILE_EXPR(moveVelocityVar, CheckVector);
  COMPILE_EXPR(rotationVelocityVar, Check);
  COMPILE_EXPR(colorVar, CheckColor);
  COMPILE_EXPR(randomDirectionPeriodVar, Check);
  COMPILE_EXPR(randomDirectionIntensityVar, Check);
}

template<>
struct BankTraits<ParticleSourceType>: public DefBankTraits<ParticleSourceType>
{
  typedef const ParticleSourceName &NameType;
  static int CompareNames(ParticleSourceNamePar n1, ParticleSourceNamePar n2)
  {
    int d = strcmpi(n1.name, n2.name);
    return d ? d : n1.evarSet - n2.evarSet;
  }
};

typedef BankArray<ParticleSourceType> ParticleSourceTypeBank;

ParticleSourceTypeBank ParticleSourceTypes;


CParticleSourceBase::CParticleSourceBase()
{
  _interval = 0;
  _nextTime = 0;

  _circleRadius = 0;
  _circleVelocity = VZero;

  _oldPosition = VZero;
  _isOldPositionInitialized = false;

  _variance.Init();
}

void CParticleSourceBase::SetParams(const CParticleParams &params)
{
  _params = params;
}

void CParticleSourceBase::SetRandom(const CParticleRandomization &values)
{
  _variance = values;
}

void CParticleSourceBase::SetCircle(float circleRadius, Vector3Par circleVelocity)
{
  _circleRadius = circleRadius;
  _circleVelocity = circleVelocity;
}

void CParticleSourceBase::AttachObject(Object *object, Vector3Par pos)
{
  Assert(_params._type);
  if (!_params._type) return;
  _params._type->_isObject = true;
  _params._type->_object = object;
  _params._type->_position = pos;
}

void CParticleSourceBase::AttachObject(Object *object)
{
  Assert(_params._type);
  if (!_params._type) return;
  Assert(_params._type->_isObject);
  if (_params._type->_isObject) _params._type->_object = object;
}

void CParticleSourceBase::DetachObject()
{
  Assert(_params._type);
  if (!_params._type) return;
  _params._type->_isObject = false;
  _params._type->_object = 0;
  _params._type->_position = Vector3(0.0f, 0.0f, 0.0f);
}

Vector3 CParticleSourceBase::EstimateDropPosition(Entity *source) const
{
  if (_params._type && _params._type->_isObject)
  {
    Assert(_params._type->_object);
    return _params._type->_object->GetCurrentPosition();
  }
  else if (source)
  {
    return source->FutureVisualState().Position();
  }
  else
  {
    Fail("No source?");
    return VZero;
  }
}

Vector3 CParticleSourceBase::TransformPositionModelToWorld(const Entity* source, Vector3Par position)
{
  if (_params._type->_isObject)
  {
    Object *obj = _params._type->_object;
    // apply skinning, if any
    auto skinning = _params._type->_skinning;
    if (skinning.bones.Size()>0)
    {
      // TODO: LOD independent skinning
      LODShape *lShape = obj->GetShape();
      int fireGeom = lShape->FindFireGeometryLevel();
      if (fireGeom>=0)
      {
        // TODO: implement full skinning, not only one bone
        Matrix4 mat = MIdentity;
        obj->AnimateBoneMatrix(mat, obj->FutureVisualState(),fireGeom,skinning.bones[0].bone);
        Vector3Val animatedPos = mat.FastTransform(skinning.originalPosition)+_params._type->_position;
        return obj->FutureVisualState().PositionModelToWorld(animatedPos);
      }
      
    }
    return obj->GetCurrentPosition();
  }
  else if (source)
  {
    return source->FutureVisualState().PositionModelToWorld(position);
  }
  return position;
}

Vector3 CParticleSourceBase::TransformDirectionModelToWorld(const Entity* source, Vector3Par direction)
{
  if (_params._type->_isObject)
  {
    Assert(_params._type->_object);
    return _params._type->_object->FutureVisualState().DirectionModelToWorld(direction);
  }
  else if (source)
  {
    return source->FutureVisualState().DirectionModelToWorld(direction);
  }
  return direction;
}

Vector3 CParticleSourceBase::TransformSpeed(const Entity* source, Vector3Par speed)
{
  if (_params._type->_isObject)
  {
    Assert(_params._type->_object);
    return speed + _params._type->_object->ObjectSpeed();
  }
  else if (source)
  {
    return speed + source->FutureVisualState().Speed();
  }
  return speed;
}

#define RANDOMIZE(name) \
  randomization.name = _params._type->name + _variance.name * (2.0f * GRandGen.RandomValue() - 1.0f);

/**
@param relativeSize used to enlarge particles when "generalizing" sources
*/
void CParticleSourceBase::Drop(Entity *source, float relativeSize, Vector3Par previousPosition, float positionFactor, Vector3Par sourceSpeed)
{
  Assert(_params._type);
  // randomization
  CParticleRandomization randomization;
  RANDOMIZE(_lifeTime)

  Vector3 position = _params._type->_position;

  RANDOMIZE(_moveVelocity[0])
  RANDOMIZE(_moveVelocity[1])
  RANDOMIZE(_moveVelocity[2])
  RANDOMIZE(_rotationVelocity)
  RANDOMIZE(_randomDirectionPeriod)
  RANDOMIZE(_randomDirectionIntensity)
  RANDOMIZE(_angle)
  // calculate factor for randomization of size and color
  float rnd = _variance._sizeCoef * (2.0f * GRandGen.RandomValue() - 1.0f);
  randomization._sizeCoef = exp(rnd);
  rnd = _variance._colorCoef.R() * (2.0f * GRandGen.RandomValue() - 1.0f);
  randomization._colorCoef.SetR(exp(rnd));
  rnd = _variance._colorCoef.G() * (2.0f * GRandGen.RandomValue() - 1.0f);
  randomization._colorCoef.SetG(exp(rnd));
  rnd = _variance._colorCoef.B() * (2.0f * GRandGen.RandomValue() - 1.0f);
  randomization._colorCoef.SetB(exp(rnd));
  rnd = _variance._colorCoef.A() * (2.0f * GRandGen.RandomValue() - 1.0f);
  randomization._colorCoef.SetA(exp(rnd));
  
  // Update position (including blending with previous position)
  position = TransformPositionModelToWorld(source, position) * positionFactor + previousPosition * (1.0f - positionFactor);

  // Randomize position
  randomization._position[0] = position[0] + _variance._position[0] * (2.0f * GRandGen.RandomValue() - 1.0f);
  randomization._position[1] = position[1] + _variance._position[1] * (2.0f * GRandGen.RandomValue() - 1.0f);
  randomization._position[2] = position[2] + _variance._position[2] * (2.0f * GRandGen.RandomValue() - 1.0f);

  // Fix: transforming velocity is a nonsense - (scale, up direction etc.)
/*
  // Update direction
  randomization._moveVelocity = TransformDirectionModelToWorld(source, randomization._moveVelocity);
*/
  if (_params._type->_skinning.bones.Size()>0)
  {
    randomization._moveVelocity += sourceSpeed;
  }

  if (_circleRadius > 0)
  {
    Vector3 &pos = randomization._position;

    // move particle to circle
    float angle = 2.0f * H_PI * GRandGen.RandomValue();
    Vector3 dir(sin(angle), 0, cos(angle));
    float height = 0; //pos.Y() - GLandscape->SurfaceYAboveWater(pos.X(), pos.Z());
    pos += _circleRadius * dir;
    float dX, dZ;
    float posY = GLandscape->SurfaceYAboveWater(pos.X(), pos.Z(), &dX, &dZ) + height;
    
    if (_params._onSurface)
    {
      // used for helicopter dust
      pos[1] = posY;
    }
    else
    {
      // used for explosions or object destructions
      // avoid being under the surface
      if (pos[1]<posY) pos[1] = posY;
    }
    
    // update velocity
    Vector3 up(-dX, 1, -dZ);
    Matrix3 space; space.SetUpAndDirection(up, dir);
    randomization._moveVelocity += space * _circleVelocity;
  }

  // if has no sense to create extremely fast particles - the only effect they have is numerical problems 
  if (randomization._moveVelocity.SquareSize()<Square(1e4))
  {
    CParticle *particle = new CParticle(_params, randomization, relativeSize);
    Assert(particle->FutureVisualState().Position().SquareSize()>0.01);
    GWorld->AddCloudlet(particle);
  }
}

void CParticleSourceBase::Simulate(Entity *source, float deltaT, SimulationImportance prec)
{
  if (_interval > 0)
  {
    if (!_params._type) return;
    if (_params._type->_isObject && !_params._type->_object) return;

    if (!_params._shape) NewSprite(_params._shapeName, ShapeNormal, _params._shape, _params._type->_type == PTBillboard, false);

    Vector3 pos = EstimateDropPosition(source);
    
    if (!EnableVisualEffects(pos,prec)) return;
    
    const Camera &camera=*GScene->GetCamera();
    float dist = camera.Position().Distance(pos);
    float invZoom = camera.Left();
    
    float generalize = dist*invZoom * GScene->GetSmokeGeneralization()*0.3f;
    
    //GlobalShowMessage(100,"Generalize %g, dist %g",generalize,dist);
    
    saturate(generalize,MinGeneralize,3);

    // Calculate number of particles to drop
    int particlesCount = 0;
    _nextTime -= deltaT;
    // absolute requirement: never allow more than CloudletCountLimitSoft in one step
    // to be safe we allow a lot less
    // note: for a small _interval a division would probably be a lot faster
    while (_nextTime <= 0 && particlesCount<1000)
    {
      _nextTime += _interval*generalize*generalize;
      particlesCount++;
    }
    // if we have terminated early, ignore the rest
    if (_nextTime<=0) _nextTime = 0;
    
    Vector3 newPosition = TransformPositionModelToWorld(source, _params._type->_position);
    if (_isOldPositionInitialized)
    {

      if (particlesCount > 0)
      {
        // does not work well, source object simulation is not synchronized, interpolation would be necessary
        //Vector3 sourceSpeed = (newPosition-_oldPosition)/deltaT;
        Vector3 sourceSpeed = TransformSpeed(source,VZero);
        // Go through all the particles and spread them between old and new position
        float dFactor = 1.0f / particlesCount;
        float currFactor = dFactor;
        for (int i = 0; i < particlesCount; i++)
        {
          if (_params._shape)
          {
            Drop(source, generalize, _oldPosition, currFactor, sourceSpeed);
          }
          currFactor += dFactor;
        }
      }
    }
    else
    {
      // Drop all particles (at the same place)
      for (int i = 0; i < particlesCount; i++)
      {
        if (_params._shape)
        {
          Drop(source, generalize, VZero, 1.0f, VZero);
        }
      }
      _isOldPositionInitialized = true;
    }
    _oldPosition = newPosition;
  }
}

LSError CParticleSourceBase::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("interval", _interval, 1))
  CHECK(ar.Serialize("nextTime", _nextTime, 1))
  CHECK(ar.Serialize("circleRadius", _circleRadius, 1))
  CHECK(::Serialize(ar, "circleVelocity", _circleVelocity, 1))

  // values
  bool hasType;
  if (ar.IsSaving())
  {
    hasType = _params._type != NULL;
    CHECK(ar.Serialize("hasType", hasType, 1))
  }
  else if (ar.GetPass() == ParamArchive::PassFirst)
  {
    CHECK(ar.Serialize("hasType", hasType, 1, true));
    _params._type = hasType ? new CParticleType : NULL;
  }

  // type
  if (_params._type)
  {
    CHECK(ar.SerializeEnum("particleType", _params._type->_type, 1))
    CHECK(ar.Serialize("timerPeriod", _params._type->_timerPeriod, 1))
    CHECK(ar.Serialize("lifeTime", _params._type->_lifeTime, 1))
    CHECK(::Serialize(ar, "position", _params._type->_position, 1))
    CHECK(::Serialize(ar, "moveVelocity", _params._type->_moveVelocity, 1))
    CHECK(ar.Serialize("rotationVelocity", _params._type->_rotationVelocity, 1))
    CHECK(ar.Serialize("weight", _params._type->_weight, 1))
    CHECK(ar.Serialize("volume", _params._type->_volume, 1))
    CHECK(ar.Serialize("rubbing", _params._type->_rubbing, 1))
    CHECK(ar.Serialize("angle", _params._type->_angle, 1))
    CHECK(ar.SerializeArray("size", _params._type->_size, 1))
    CHECK(::SerializeArray(ar, "color", _params._type->_color, 1))
    CHECK(ar.SerializeArray("animationSpeed", _params._type->_animationSpeed, 1))
    CHECK(ar.Serialize("randomDirectionPeriod", _params._type->_randomDirectionPeriod, 1))
    CHECK(ar.Serialize("randomDirectionIntensity", _params._type->_randomDirectionIntensity, 1))
    CHECK(ar.Serialize("onTimerScript", _params._type->_onTimerScript, 1))
    CHECK(ar.Serialize("beforeDestroyScript", _params._type->_beforeDestroyScript, 1))
    CHECK(ar.Serialize("isObject", _params._type->_isObject, 1))
    CHECK(ar.SerializeRef("object", _params._type->_object, 1))
  }
  // other params
  CHECK(ar.Serialize("particleShape", _params._shapeName, 1))
  CHECK(ar.Serialize("particleFSNtieth", _params._frameSource._ntieth, 1, 1))
  CHECK(ar.Serialize("particleFSIndex", _params._frameSource._index, 1, 0))
  CHECK(ar.Serialize("particleFSFrameCount", _params._frameSource._frameCount, 1, 1))
  CHECK(ar.Serialize("particleFSLoop", _params._frameSource._loop, 1, true))
  CHECK(ar.Serialize("animationName", _params._animationName, 1))

  // variance
  CHECK(ar.Serialize("lifeTimeVar", _variance._lifeTime, 1))
  CHECK(::Serialize(ar, "positionVar", _variance._position, 1))
  CHECK(::Serialize(ar, "moveVelocityVar", _variance._moveVelocity, 1))
  CHECK(ar.Serialize("rotationVelocityVar", _variance._rotationVelocity, 1))
  CHECK(ar.Serialize("angle", _variance._angle, 1, 0))
  CHECK(ar.Serialize("sizeCoef", _variance._sizeCoef, 1, 0))
  CHECK(::Serialize(ar, "colorCoef", _variance._colorCoef, 1, Color(0, 0, 0, 0)))
  CHECK(ar.Serialize("randomDirectionPeriodVar", _variance._randomDirectionPeriod, 1))
  CHECK(ar.Serialize("randomDirectionIntensityVar", _variance._randomDirectionIntensity, 1))
  
  return LSOK;
}

// evaluate helpers
#define EVALUATE_EXPR(WHAT, RESULT) \
  if (!WHAT.Evaluate(RESULT, varValues, varCount)) \
    RptF("Error during evaluation of expression " #WHAT)
#define EVALUATE_EXPR_SOURCE(PART, RESULT) \
  if (!sourceType->PART.Evaluate(RESULT, varValues, varCount)) \
    RptF("Error during evaluation of expression " #PART " in %s", cc_cast(name))
#define EVALUATE_EXPR_SOURCE_ARRAY(NAME) \
  int n##NAME = sourceType->NAME.Size(); \
  _params._type->NAME.Realloc(n##NAME); \
  _params._type->NAME.Resize(n##NAME); \
  for (int i = 0; i < n##NAME; i++) \
    EVALUATE_EXPR_SOURCE(NAME[i], _params._type->NAME[i])


void CParticleSourceBase::Load(RString name, EffectVariablesPar evars)
{
  _params._sourceTypeName = name;
  ParticleSourceName pname = {name, evars.GetSet()};
  ParticleSourceType *sourceType = ParticleSourceTypes.New(pname);

  // SetParams
  _params._shapeName = sourceType->_shapeName;
  _params._frameSource._ntieth = sourceType->_particleFSNtieth;
  _params._frameSource._index = sourceType->_particleFSIndex;
  _params._frameSource._frameCount = sourceType->_particleFSFrameCount;
  _params._frameSource._loop = sourceType->_particleFSLoop;
  _params._animationName = sourceType->_animationName;
  // Values
  _params._type = new CParticleType;
  _params._type->_type = sourceType->_particleType;
  _params._type->_onTimerScript = sourceType->_onTimerScript;
  _params._type->_beforeDestroyScript = sourceType->_beforeDestroyScript;
  _params._onSurface = sourceType->_onSurface;
  // SetRandom
  _variance._angle = sourceType->_angleVar;
  _variance._sizeCoef = sourceType->_sizeVar;

  const float *varValues = evars.GetValues();
  int varCount = evars.GetCount();
  // SetInterval
  EVALUATE_EXPR_SOURCE(_interval, _interval);
  // SetCircle
  EVALUATE_EXPR_SOURCE(_circleRadius, _circleRadius);
  EVALUATE_EXPR_SOURCE(_circleVelocity, _circleVelocity);
  // Values
  EVALUATE_EXPR_SOURCE(_timerPeriod, _params._type->_timerPeriod);
  EVALUATE_EXPR_SOURCE(_lifeTime, _params._type->_lifeTime);
  EVALUATE_EXPR_SOURCE(_moveVelocity, _params._type->_moveVelocity);
  EVALUATE_EXPR_SOURCE(_rotationVelocity, _params._type->_rotationVelocity);
  EVALUATE_EXPR_SOURCE(_weight, _params._type->_weight);
  EVALUATE_EXPR_SOURCE(_volume, _params._type->_volume);
  EVALUATE_EXPR_SOURCE(_rubbing, _params._type->_rubbing);
  EVALUATE_EXPR_SOURCE(_angle, _params._type->_angle);
  EVALUATE_EXPR_SOURCE_ARRAY(_size);
  EVALUATE_EXPR_SOURCE_ARRAY(_color);
  EVALUATE_EXPR_SOURCE_ARRAY(_animationSpeed);
  EVALUATE_EXPR_SOURCE(_randomDirectionPeriod, _params._type->_randomDirectionPeriod);
  EVALUATE_EXPR_SOURCE(_randomDirectionIntensity, _params._type->_randomDirectionIntensity);
  // SetRandom
  EVALUATE_EXPR_SOURCE(_lifeTimeVar, _variance._lifeTime);
  EVALUATE_EXPR_SOURCE(_positionVar, _variance._position);
  EVALUATE_EXPR_SOURCE(_moveVelocityVar, _variance._moveVelocity);
  EVALUATE_EXPR_SOURCE(_rotationVelocityVar, _variance._rotationVelocity);
  EVALUATE_EXPR_SOURCE(_colorVar, _variance._colorCoef);
  EVALUATE_EXPR_SOURCE(_randomDirectionPeriodVar, _variance._randomDirectionPeriod);
  EVALUATE_EXPR_SOURCE(_randomDirectionIntensityVar, _variance._randomDirectionIntensity);
}


CParticleSource::CParticleSource(const EntityType *type)
: Entity(NULL, type, CreateObjectId())
{
}

void CParticleSource::Simulate(float deltaT, SimulationImportance prec)
{
  if (EnableVisualEffects(prec))
  {
    CParticleSourceBase::Simulate(this, deltaT, prec);
  }
}

LSError CParticleSource::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(CParticleSourceBase::Serialize(ar))
  return LSOK;
}

DEFINE_CASTING(CParticleSource)

bool GetVector3(Vector3 &ret, GameValuePar oper)
{
  const GameArrayType &array = oper;
  if (array.Size() != 3) return false;
  if (array[0].GetType() != GameScalar ||
      array[1].GetType() != GameScalar ||
      array[2].GetType() != GameScalar)
  {
    return false;
  }
  ret[0] = array[0];
  ret[1] = array[2];
  ret[2] = array[1];
  return true;
}

bool GetColor(Color &ret, GameValuePar oper)
{
  const GameArrayType &array = oper;
  if (array.Size() != 4) return false;
  if (array[0].GetType() != GameScalar ||
      array[1].GetType() != GameScalar ||
      array[2].GetType() != GameScalar ||
      array[3].GetType() != GameScalar)
  {
    return false;
  }
  ret = ColorP(array[0], array[1], array[2], array[3]);
  return true;
}

// Declaration of existing function
bool GetPos(Vector3 &, GameValuePar);

void CParticleRandomization::Init()
{
  _lifeTime = 0;
  _position = VZero;
  _moveVelocity = VZero;
  _rotationVelocity = 0;
  _angle = 0.0f;
  _sizeCoef = 0;
  _colorCoef = Color(0, 0, 0, 0);
  _randomDirectionPeriod = 0;
  _randomDirectionIntensity = 0;
}

void CParticleRandomization::Init(const CParticleType *type)
{
  _lifeTime = type->_lifeTime;
  _position = type->_position;
  _moveVelocity = type->_moveVelocity;
  _rotationVelocity = type->_rotationVelocity;
  _angle = type->_angle;
  _sizeCoef = 1;
  _colorCoef = Color(1, 1, 1, 1);
  _randomDirectionPeriod = type->_randomDirectionPeriod;
  _randomDirectionIntensity = type->_randomDirectionIntensity;
}

bool CParticleRandomization::Init(const GameState *state, GameValuePar oper1)
{
  const GameArrayType &array = oper1;

  // In case number of parameters doesn't match required number.
  if ((array.Size() < 8) || (array.Size() > 9))
  {
    state->SetError(EvalDim, array.Size(), 8);
    return false;
  }

  // _lifeTime
  if (array[0].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[0].GetType());
    return false;
  }
  _lifeTime = array[0];

  // _position
  if (array[1].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[1].GetType());
    return false;
  }
  if (!GetVector3(_position, array[1]))
  {
    state->SetError(EvalGen);
    return false;
  }

  // _moveVelocity
  if (array[2].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[2].GetType());
    return false;
  }
  if (!GetVector3(_moveVelocity, array[2]))
  {
    state->SetError(EvalGen);
    return false;
  }

  // _rotationVelocity
  if (array[3].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[3].GetType());
    return false;
  }
  _rotationVelocity = array[3];

  // _sizeCoef
  if (array[4].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[4].GetType());
    return false;
  }
  _sizeCoef = array[4];

  // _colorCoef
  if (array[5].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[5].GetType());
    return false;
  }
  if (!GetColor(_colorCoef, array[5]))
  {
    state->SetError(EvalGen);
    return false;
  }

  // _randomDirectionPeriod
  if (array[6].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[6].GetType());
    return false;
  }
  _randomDirectionPeriod = array[6];

  // _randomDirectionIntensity
  if (array[7].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[7].GetType());
    return false;
  }
  _randomDirectionIntensity = array[7];

  // Optional parameter: _angle
  if (array.Size() >= 9)
  {
    if (array[8].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, array[8].GetType());
      return false;
    }
    _angle = array[8];
  }
  else
  {
    _angle = 0.0f;
  }

  return true;
}

bool CParticleParams::Init(const GameState *state, GameValuePar oper1, bool relativePos)
{
  const GameArrayType &array = oper1;

  // In case number of parameters doesn't match required number
  if (array.Size() < 19 || array.Size() > 21)
  {
    state->SetError(EvalDim, array.Size(), 19);
    return false;
  }

  // Match the required types of parameters and get its values
  Ref<CParticleType> type = new CParticleType;

  // ShapeName
  if ((array[0].GetType() != GameString) && (array[0].GetType() != GameArray))
  {
    state->SetError(EvalType, "Parameter should be either string or array"/*, array[0].GetType()*/); // cannot pass non-POD type in ...
    return false;
  }

  // Process either string or array
  if (array[0].GetType() == GameString)
  {
    _shapeName = GetShapeName(array[0]);
    if (_shapeName.GetLength() <= 0)
    {
      return false;
    }

    // Pass default values
    _frameSource._ntieth = 1;
    _frameSource._index = 0;
    _frameSource._frameCount = 1;
    _frameSource._loop = true;
  }
  else
  {
    const GameArrayType &frameSource = array[0];

    // In case number of parameters doesn't match required number
    if ((frameSource.Size() < 4) || (frameSource.Size() > 5))
    {
      state->SetError(EvalDim, frameSource.Size(), 4);
      return false;
    }

    // _shapeName
    _shapeName = GetShapeName(frameSource[0]);
    if (_shapeName.GetLength() <= 0)
    {
      return false;
    }

    // _ntieth
    if (frameSource[1].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, frameSource[1].GetType());
      return false;
    }
    float ntieth = frameSource[1];
    _frameSource._ntieth = toInt(ntieth);

    // _index
    if (frameSource[2].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, frameSource[2].GetType());
      return false;
    }
    float index = frameSource[2];
    _frameSource._index = toInt(index);

    // _frameCount
    if (frameSource[3].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, frameSource[3].GetType());
      return false;
    }
    float frameCount = frameSource[3];
    _frameSource._frameCount = toInt(frameCount);
    saturateMax(_frameSource._frameCount, 1);

    // Optional parameter: _loop
    if (frameSource.Size() >= 5)
    {
      if (frameSource[4].GetType() != GameScalar)
      {
        state->TypeError(GameScalar, frameSource[4].GetType());
        return false;
      }
      float loop = frameSource[4];
      _frameSource._loop = (toInt(loop) != 0);
    }
    else
    {
      _frameSource._loop = true;
    }
  }

  // AnimationName
  if (array[1].GetType() != GameString)
  {
    state->TypeError(GameString, array[1].GetType());
    return false;
  }
  _animationName = array[1];

  // _Type
  if (array[2].GetType() != GameString)
  {
    state->TypeError(GameString, array[2].GetType());
    return false;
  }
  RString name = array[2];
  type->_type = GetEnumValue<ParticleType>((const char *)name);
  if (type->_type == INT_MIN)
  {
    state->SetError(EvalGen);
    return false;
  }

  // _TimerPeriod
  if (array[3].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[3].GetType());
    return false;
  }
  type->_timerPeriod = array[3];

  // _LifeTime
  if (array[4].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[4].GetType());
    return false;
  }
  type->_lifeTime = array[4];

  // _MoveVelocity
  if (array[6].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[6].GetType());
    return false;
  }
  if (!GetVector3(type->_moveVelocity, array[6]))
  {
    state->SetError(EvalGen);
    return false;
  }

  // _RotationVelocity
  if (array[7].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[7].GetType());
    return false;
  }
  type->_rotationVelocity = array[7];

  // _Weight
  if (array[8].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[8].GetType());
    return false;
  }
  type->_weight = array[8];

  // _Volume
  if (array[9].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[9].GetType());
    return false;
  }
  type->_volume = array[9];

  // _Rubbing
  if (array[10].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[10].GetType());
    return false;
  }
  type->_rubbing = array[10];

  // _Size
  if (array[11].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[11].GetType());
    return false;
  }
  const GameArrayType &arraySize = array[11];
  int n = arraySize.Size();
  type->_size.Realloc(n);
  type->_size.Resize(n);
  for (int i=0; i<n; i++)
  {
    if (arraySize[i].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, arraySize[i].GetType());
      return false;
    }
    type->_size[i] = arraySize[i];
  }

  // _Color
  if (array[12].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[12].GetType());
    return false;
  }
  const GameArrayType &arrayColor = array[12];
  n = arrayColor.Size();
  type->_color.Realloc(n);
  type->_color.Resize(n);
  for (int i=0; i<n; i++)
  {
    if (arrayColor[i].GetType() != GameArray)
    {
      state->TypeError(GameArray, arrayColor[i].GetType());
      return false;
    }
    Color color;
    if (!GetColor(color, arrayColor[i]))
    {
      state->SetError(EvalGen);
      return false;
    }
    type->_color[i] = color;
  }

  // _AnimationSpeed
  if (array[13].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[13].GetType());
    return false;
  }
  const GameArrayType &arrayAnimationSpeed = array[13];
  n = arrayAnimationSpeed.Size();
  type->_animationSpeed.Realloc(n);
  type->_animationSpeed.Resize(n);
  for (int i=0; i<n; i++)
  {
    if (arrayAnimationSpeed[i].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, arrayAnimationSpeed[i].GetType());
      return false;
    }
    type->_animationSpeed[i] = arrayAnimationSpeed[i];
  }

  // _RandomDirectionPeriod
  if (array[14].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[14].GetType());
    return false;
  }
  type->_randomDirectionPeriod = array[14];

  // _RandomDirectionIntensity
  if (array[15].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[15].GetType());
    return false;
  }
  type->_randomDirectionIntensity = array[15];

  // OnTimer
  if (array[16].GetType() != GameString)
  {
    state->TypeError(GameString, array[16].GetType());
    return false;
  }
  type->_onTimerScript = array[16];

  // BeforeDestroy
  if (array[17].GetType() != GameString)
  {
    state->TypeError(GameString, array[17].GetType());
    return false;
  }
  type->_beforeDestroyScript = array[17];

    
  if (array[18].GetType() == GameArray)
  { // Object with skinning
    const GameArrayType &objArray = array[18];
    if (objArray.Size()!=2)
    {
      state->SetError(EvalDim, objArray.Size(), 2);
      return false;
    }
    if (objArray[0].GetType() != GameObject)
    {
      state->TypeError(GameObject, array[0].GetType());
      return false;
    }
    if (objArray[1].GetType() != GameArray)
    {
      state->TypeError(GameArray, array[1].GetType());
      return false;
    }
    type->_object = static_cast<GameDataObject *>(objArray[0].GetData())->GetObject();
    if (type->_object == NULL) return false;
    if (!GetObjectPosition(array, type->_position, type->_object, state))
    {
      return false;
    }

    type->_isObject = true;
    const GameArrayType &skinArray = objArray[1];
    // parse the skinning information
    // first entry should be the original (non-animated) model space position
    {
      const GameArrayType &posArray = skinArray[0];
      float x = posArray[0];
      float z = posArray[1];
      float y = posArray[2];
      type->_skinning.originalPosition = Vector3(x,y,z);
    }
    for (int i=1; i<skinArray.Size(); i++)
    {
      if (skinArray[i].GetType()!=GameArray)
      {
        state->TypeError(GameArray, skinArray[i].GetType());
        return false;
      }
      const GameArrayType &skinBone = skinArray[i];
      if (skinBone.Size()!=2)
      {
        state->SetError(EvalDim, skinBone.Size(), 2);
        return false;
      }
      if (skinBone[0].GetType() != GameString)
      {
        state->TypeError(GameString, skinBone[0].GetType());
        return false;
      }
      if (skinBone[1].GetType() != GameScalar)
      {
        state->TypeError(GameScalar, skinBone[1].GetType());
        return false;
      }
      if (!type->_object) continue;
      RString bone = skinBone[0];
      bone.Lower();
      float weight = skinBone[1];
      LODShape *objShape = type->_object->GetShape();
      if (!objShape) continue;
      SkeletonIndex bIndex = objShape->GetSkeleton()->FindBone(bone);

      type->_skinning.bones.Add(BlendBone(bIndex,bone,weight));
    }
    
  }
  else if (array[18].GetType() == GameObject)
  { // Object
    type->_object = static_cast<GameDataObject *>(array[18].GetData())->GetObject();
    if (type->_object == NULL) return false;
    if (!GetObjectPosition(array, type->_position, type->_object, state))
    {
      return false;
    }
    type->_isObject = true;
  }
  else
  {
    // In case there is no object assigned and position reference is not an array...
    if (array[5].GetType() != GameArray)
    {
      state->TypeError(GameArray, array[5].GetType());
      return false;
    }
    else if (relativePos)
    {
      if (!GetVector3(type->_position, array[5]))
      {
        state->SetError(EvalGen);
        return false;
      }
    }
    else
    {
      if (!GetPos(type->_position, array[5]))
      {
        state->SetError(EvalGen);
        return false;
      }
    }
    type->_isObject = false;
  }

  // Optional parameter: _angle
  if (array.Size() >= 20)
  {
    if (array[19].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, array[19].GetType());
      return false;
    }
    type->_angle = array[19];
  }
  else
  {
    type->_angle = 0.0f;
  }

  if (array.Size() >= 21)
  {
    if (array[20].GetType() != GameBool)
    {
      state->TypeError(GameBool, array[20].GetType());
      return false;
    }
    _onSurface = array[20];
  }
  
  _type = type;
  return true;
}

bool CParticleParams::GetObjectPosition( const GameArrayType &array, Vector3 &pos, Object *obj, const GameState *state )
{
  if (array[5].GetType() == GameArray)
  {
    if (!GetVector3(pos, array[5]))
    {
      state->SetError(EvalGen);
      return false;
    }
  }
  else if (array[5].GetType() == GameString)
  {
    RString sPosition;
    sPosition = array[5];
    pos = obj->GetShape()->MemoryPoint(sPosition);
  }
  else
  {
    state->TypeError(GameArray, array[5].GetType());
    return false;
  }
  return true;
}

/*!
\patch 1.93 Date 8/26/2003 by Ondra
- Fixed: Scripting command drop could cause crash when used with NULL object.
*/

GameValue ParticleDrop(const GameState *state, GameValuePar oper1)
{
  CParticleParams params;
  if (!params.Init(state, oper1, false)) return GameValue();
  Assert(params._type);

  // Create the sprite shape
  NewSprite(params._shapeName, ShapeNormal, params._shape, params._type->_type == PTBillboard, false);

  // If shape is ready then create the cloudlet
  if (params._shape)
  {
    if (params._type->_isObject)
    {
      Assert(params._type->_object);
      params._type->_position = params._type->_object->FutureVisualState().PositionModelToWorld(params._type->_position);
      params._type->_moveVelocity = params._type->_object->FutureVisualState().DirectionModelToWorld(params._type->_moveVelocity);
    }

    CParticleRandomization randomization;
    randomization.Init(params._type);
    // if has no sense to create extremely fast particles - the only effect they have is numerical problems 
    if (randomization._moveVelocity.SquareSize()<Square(1e4))
    {
      CParticle *particle = new CParticle(params, randomization, 1);
      Assert(particle->FutureVisualState().Position().SquareSize()>0.01);
      GWorld->AddCloudlet(particle);
    }
  }
  
  return GameValue();
}

Object *GetObject(GameValuePar oper);

GameValue ParticleSetParams(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  CParticleSource *source = dyn_cast<CParticleSource>(obj);
  if (!source) return GameValue();

  CParticleParams params;
  if (!params.Init(state, oper2, true)) return GameValue();

  // Create the sprite shape
  Assert(params._type);
  NewSprite(params._shapeName, ShapeNormal, params._shape, params._type->_type == PTBillboard, false);

  source->SetParams(params);
  return GameValue();
}

GameValue ParticleSetRandom(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  CParticleSource *source = dyn_cast<CParticleSource>(obj);
  if (!source) return GameValue();

  CParticleRandomization values;
  if (!values.Init(state, oper2)) return GameValue();

  source->SetRandom(values);
  return GameValue();
}

GameValue ParticleSetCircle(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  CParticleSource *source = dyn_cast<CParticleSource>(obj);
  if (!source) return GameValue();

  // In case number of parameters doesn't match required number.
  const GameArrayType &array = oper2;
  if (array.Size() != 2)
  {
    state->SetError(EvalDim, array.Size(), 2);
    return GameValue();
  }

  // Match the required types of parameters and get its values
  // circleRadius
  if (array[0].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[0].GetType());
    return GameValue();
  }
  float circleRadius = array[0];
  // circleVelocity
  if (array[1].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[1].GetType());
    return GameValue();
  }
  Vector3 circleVelocity;
  if (!GetVector3(circleVelocity, array[1]))
  {
    state->SetError(EvalGen);
    return GameValue();
  }

  source->SetCircle(circleRadius, circleVelocity);
  return GameValue();
}

GameValue ParticleSetDropInterval(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  CParticleSource *source = dyn_cast<CParticleSource>(obj);
  if (!source) return GameValue();

  float interval = oper2;
  source->SetInterval(interval);
  return GameValue();
}

static Vector3 GetPosition(LODShape *shape, RStringB name, const Vector3 *defPos=NULL)
{
  if (!shape) return VZero;
  if (name.GetLength() == 0)
  {
    // if there is a default position provided, use it
    if (defPos) return *defPos;
    return shape->CenterOfMass();
  }

  int memory = shape->FindMemoryLevel();
  if (memory < 0) return shape->CenterOfMass();

  ShapeUsedGeometryLock<> lod(shape,memory);
  int index = shape->PointIndex(lod, name);
  if (index < 0) return shape->CenterOfMass();

  return lod->Pos(index);
}

static Vector3 GetPosition(LODShape *shape, ParamEntryVal entry)
{
  if (entry.IsArray())
  {
    if (entry.GetSize() != 3) return VZero;
    return Vector3(entry[0], entry[2], entry[1]);
  }

  RString name = entry;
  return GetPosition(shape, name);
}

DEF_RSB(light)
DEF_RSB(sound)
DEF_RSB(particles)
DEF_RSB(destroy)
DEF_RSB(ruin)
DEF_RSB(damageAround)

struct EffectType
{
  RStringB _type;

  float _intensity;
  float _interval;
  float _lifeTime;
};

struct LightEffectType : public EffectType
{
  // position is given either as a memory point or an offset
  RStringB _positionName;
  Vector3 _position;
};
TypeIsMovableZeroed(LightEffectType)

struct ParticleEffectType : public EffectType
{
  // position is given either as a memory point or an offset
  RStringB _positionName;
  Vector3 _position;
};
TypeIsMovableZeroed(ParticleEffectType)

struct SoundEffectType : public EffectType
{

};
TypeIsMovableZeroed(SoundEffectType)

/// structure containing shared parameters of EffectsSource, cached in the bank
struct EffectsSourceType : public RefCount
{
  RString _name;
  AutoArray<LightEffectType> _lightEffects;
  AutoArray<ParticleEffectType> _particleEffects;
  AutoArray<SoundEffectType> _soundEffects;

  EffectsSourceType(RString name);
  RString GetName() const {return _name;}
};

EffectsSourceType::EffectsSourceType(RString name)
  : _name(name)
{
  // access the class listing the effects
  ParamEntryVal cls = Pars >> name;
  for (int i=0; i<cls.GetEntryCount(); i++)
  {
    ParamEntryVal entry = cls.GetEntry(i);
    if (!entry.IsClass()) continue;

    // read all parameters of the effect
    RStringB simulation = entry >> "simulation";
    if (simulation == RSB(light))
    {
      int index = _lightEffects.Add();
      LightEffectType &info = _lightEffects[index];

      info._type = entry >> "type";
      info._intensity = entry >> "intensity";
      info._interval = entry >> "interval";
      info._lifeTime = entry >> "lifeTime";
      ParamEntryVal position = entry >> "position";
      if (position.IsArray())
        info._position = (position.GetSize() == 3) ? Vector3(position[0], position[2], position[1]) : VZero;
      else
      {
        info._position = VUndefined;
        info._positionName = position;
      }
    }
    else if (simulation == RSB(particles))
    {
      int index = _particleEffects.Add();
      ParticleEffectType &info = _particleEffects[index];

      info._type = entry >> "type";
      info._intensity = entry >> "intensity";
      info._interval = entry >> "interval";
      info._lifeTime = entry >> "lifeTime";
      ParamEntryVal position = entry >> "position";
      if (position.IsArray())
        info._position = (position.GetSize() == 3) ? Vector3(position[0], position[2], position[1]) : VZero;
      else
      {
        info._position = VUndefined;
        info._positionName = position;
      }
    }
    else if (simulation == RSB(sound))
    {
      int index = _soundEffects.Add();
      SoundEffectType &info = _soundEffects[index];

      info._type = entry >> "type";
      info._intensity = entry >> "intensity";
      info._interval = entry >> "interval";
      info._lifeTime = entry >> "lifeTime";
    }
  }

  _lightEffects.Compact();
  _particleEffects.Compact();
  _soundEffects.Compact();
}

typedef BankArray<EffectsSourceType> EffectsSourceTypeBank;

EffectsSourceTypeBank EffectsSourceTypes;


void EffectsSource::Init(RString type, Entity *entity, EffectVariables &evars, float intensity, float interval, float fireIntensity, float fireInterval, float lifeTime)
{
  if (!entity) return;

  _entity = entity;

  EffectsSourceType *sourceType;
  {
    PROFILE_SCOPE_EX(fxScN,sim);
    sourceType = EffectsSourceTypes.New(type);
  }
  if (!sourceType) return;

  PROFILE_SCOPE_EX(fxScL,sim);

  // light effects
  int n = sourceType->_lightEffects.Size();
  _lights.Realloc(n);
  _lights.Resize(n);
  for (int i = 0; i < n; i++)
  {
    const LightEffectType &type = sourceType->_lightEffects[i];
    evars.UpdateBulletParameters(
      intensity * type._intensity, interval * type._interval,
      fireIntensity * type._intensity, fireInterval * type._interval,
      lifeTime * type._lifeTime);
    LightEffect &info = _lights[i];
    info._light = new LightPoint();
    info._light->Load(Pars >> "CfgLights" >> type._type, evars);
    info._pos = (type._position == VUndefined) ? ::GetPosition(entity->GetShape(), type._positionName) : type._position;
    info._timeToLive = lifeTime * type._lifeTime;
    GScene->AddLight(info._light);
  }

  // particle effects
  n = sourceType->_particleEffects.Size();
  _particleSources.Realloc(n);
  _particleSources.Resize(n);
  for (int i = 0; i < n; i++)
  {
    const ParticleEffectType &type = sourceType->_particleEffects[i];
    evars.UpdateBulletParameters(
      intensity * type._intensity, interval * type._interval,
      fireIntensity * type._intensity, fireInterval * type._interval,
      lifeTime * type._lifeTime);
    ParticlesEffect &info = _particleSources[i];
    info._source.Load(type._type, evars);
    Vector3 pos = (type._position == VUndefined) ? ::GetPosition(entity->GetShape(), type._positionName) : type._position;
    info._source.AttachObject(_entity, pos);
    info._timeToLive = lifeTime * type._lifeTime;
  }

  // sound effects
  n = sourceType->_soundEffects.Size();
  _sounds.Realloc(n);
  _sounds.Resize(n);
  for (int i = 0; i < n; i++)
  {
    const SoundEffectType &type = sourceType->_soundEffects[i];
    SoundEffect &info = _sounds[i];
    info._sound = new DynSoundObject(type._type);
    info._timeToLive = lifeTime * type._lifeTime;
  }
}

bool EffectsSource::Simulate(float deltaT, SimulationImportance prec)
{
  if (!_entity) return true;

  // simulate all effects
  bool finished = true;
  for (int i=0; i<_lights.Size(); i++)
  {
    LightEffect &info = _lights[i];
    info._timeToLive -= deltaT;
    if (info._timeToLive > 0)
    {
      if (info._light)
      {
        info._light->SetPosition(_entity->FutureVisualState().PositionModelToWorld(info._pos));
        finished = false;
      }
    }
    else
    {
      info._light = NULL;
    }
  }
  for (int i=0; i<_particleSources.Size(); i++)
  {
    ParticlesEffect &info = _particleSources[i];
    info._timeToLive -= deltaT;
    if (info._timeToLive > 0)
    {
      info._source.Simulate(_entity, deltaT, prec);
      finished = false;
    }
  }
  for (int i=0; i<_sounds.Size(); i++)
  {
    SoundEffect &info = _sounds[i];
    info._timeToLive -= deltaT;
    if (info._timeToLive > 0)
    {
      if (info._sound)
      {
        info._sound->Simulate(_entity, deltaT, prec);
        finished = false;
      }
    }
    else
    {
      info._sound = NULL;
    }
  }

  return finished;
}

LSError EffectsSource::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeRef("entity", _entity, 1))
  CHECK(ar.Serialize("particleSources", _particleSources, 1))
  CHECK(ar.Serialize("sounds", _sounds, 1))
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassSecond)
  {
    // activate lights
    for (int i=0; i<_lights.Size(); i++)
    {
      GScene->AddLight(_lights[i]._light);
    }
  }

  return LSOK;
}

void DestructionEffectType::Load(ParamEntryVal cls)
{
  _simulation = cls >> "simulation";
  _type = cls >> "type";
  _position = cls >> "position";

  _intensity = cls >> "intensity";
  _interval = cls >> "interval";
  _lifeTime = cls >> "lifeTime";
}

DEFINE_CASTING(DestructionEffects)

DestructionEffects::DestructionEffects()
: Entity(NULL, VehicleTypes.New("#destructioneffects"), CreateObjectId()), _operCacheHandled(false)
{
  SetTimeOffset(0);
  //SetSimulationPrecision(0,0);
}

DestructionEffects::DestructionEffects(
  const AutoArray<DestructionEffectType> &effectTypes, EntityAI *owner, Object *obj,
  float intensity, float density, float fireIntensity, float fireDentensity, float lifeTime,
  const Vector3 *defPos, bool doDamage
)
: Entity(NULL, VehicleTypes.New("#destructioneffects"), CreateObjectId()), _operCacheHandled(false)
{
  SetTimeOffset(0);
  //SetSimulationPrecision(0,0);
  Init(effectTypes, owner, obj, intensity, density, fireIntensity, fireDentensity, lifeTime, defPos,doDamage);
}

void DestructionEffects::Init(
  const AutoArray<DestructionEffectType> &effectTypes, EntityAI *owner, Object *obj,
  float intensity, float interval, float fireIntensity, float fireInterval, float lifeTime,
  const Vector3 *defPos, bool doDamage)
{
  PROFILE_SCOPE_EX(dsefI,*);
  if (owner && owner->GetObjectId().IsObject())
  {
    RptF("Warning: Destruction of %s owned by a static object (%s)",cc_cast(obj->GetDebugName()),cc_cast(owner->GetDebugName()));
  }
  _owner = owner;
  _doDamage = doDamage;
  
  // check: owner must be something which can be attributed for the damage (something with identity)
  // it must never be a static object
  
  _object = obj;

  _intensity = intensity;
  _interval = interval;
  _fireIntensity = fireIntensity;
  _fireInterval = fireInterval;
  _lifeTime = lifeTime;

  if (obj)
  {
    // WIP:EXTEVARS:FIXME: is it enough to pass "empty" evars here?
    EffectVariables evars(EVarSet_Bullet);
    for (int i=0; i<effectTypes.Size(); i++)
    {
      const DestructionEffectType &effType = effectTypes[i];
      evars.UpdateBulletParameters(intensity * effType._intensity, interval * effType._interval,
        fireIntensity * effType._intensity, fireInterval * effType._interval, lifeTime * effType._lifeTime);

      if (effType._simulation == RSB(light))
      {
        LightEffect &info = _lights.Append();
        info.Load(effType, evars, obj->GetShape(), defPos, lifeTime);
      }
      else if (effType._simulation == RSB(particles))
      {
        ParticlesEffect &info = _particleSources.Append();
        info.Load(effType, evars, _object, defPos, lifeTime);
      }
      else if (effType._simulation == RSB(sound))
      {
        SoundEffect &info = _sounds.Append();
        info.Load(effType, lifeTime);
      }
      else if (effType._simulation == RSB(destroy))
      {
        DestructPosEffect &info = _destructPos.Append();
        info.Load(Pars>>"CfgDestructPos">>effType._type, evars);
      }
      else if (effType._simulation == RSB(ruin))
      {
        RuinsEffect &info = _ruins.Append();
        // we need to get ruins name somewhere
        // we use the type as the model name
        // we could spawn 
        info.Load(effType._type, evars);
      }
      else if (effType._simulation == RSB(damageAround))
      {
        DamageAroundEffect &info = _damageAround.Append();
        // we need to get ruins name somewhere
        // we use the type as the model name
        // we could spawn 
        info.Load(Pars>>"CfgDamageAround">>effType._type, evars);
      }
      else
      {
        RptF("Unknown destruction effect %s for %s",cc_cast(effType._simulation),cc_cast(obj->GetDebugName()));
      }
    }
    _lights.Compact();
    _particleSources.Compact();
    _sounds.Compact();
    _destructPos.Compact();
    _damageAround.Compact();
    _ruins.Compact();
  }
}

bool DestructionEffects::SpawnRuins(bool &finished, bool async)
{
  if (_ruins.Size()>0)
  {
    Fail("Ruins no longer supported in DayZ");
  }
  finished = true;
  return false;
}

void DestructionEffects::Simulate(float deltaT, SimulationImportance prec)
{
  if (!_object)
  {
    SetDelete();
    return;
  }
  PROFILE_SCOPE_EX(dsefS,sim);
  Move(_object->WorldTransform(_object->RenderVisualState()));

  SimulateExplosion();

  bool underWater = (
    GLandscape->GetSeaLevelNoWave() > _object->COMPosition(_object->FutureVisualState()).Y() // low enough
    && GLandscape->SurfaceY(_object->COMPosition(_object->FutureVisualState()))<GLandscape->GetSeaLevelNoWave() // there is a water in that place
  );
  

  if (_exploded)
  {
    // simulate all effects
    bool finished = true;
    for (int i=0; i<_lights.Size(); i++)
    {
      LightEffect &info = _lights[i];
      
      // effect enable under water
      info._timeToLive = (underWater && !info._doUnderWater)  ? 0.0f : info._timeToLive - deltaT;

      if (info._timeToLive > 0)
      {
        if (info._light)
        {
          info._light->SetPosition(_object->FutureVisualState().PositionModelToWorld(info._pos));
          finished = false;
        }
      }
      else
      {
        info._light = NULL;
      }
    }
    for (int i=0; i<_particleSources.Size(); i++)
    {
      ParticlesEffect &info = _particleSources[i];

      // effect enable under water
      info._timeToLive = (underWater && !info._doUnderWater) ? 0.0f : info._timeToLive - deltaT;

      if (info._timeToLive > 0)
      {
        info._source.Simulate(this, deltaT, prec);
        finished = false;
      }
    }
    for (int i=0; i<_sounds.Size(); i++)
    {
      SoundEffect &info = _sounds[i];

      // if effect disabled under water
      if (underWater && !info._doUnderWater) info._timeToLive = 0.0f;

      if (info._timeToLive > 0)
      {
        info._timeToLive -= deltaT;
        if (info._sound)
        {
          info._sound->Simulate(_object, deltaT, prec);
          finished = false;
        }
      }
      else
      {
        info._sound = NULL;
      }
    }

    // we cannot start destruction until ruins are spawned
    // otherwise we would use the transform matrix which is wrong
    SpawnRuins(finished);
    
    SetDelete();
  }
}

void DestructionEffects::DoResults()
{
  bool finished = true;
  // spawn ruins synchronously - we have no background processing here
  SpawnRuins(finished,false);
}

/*!
\patch 5164 Date 6/6/2007 by Ondra
- Fixed: Building ruins were not visible for JIP-ed players.
*/
void DestructionEffects::SimulateResults()
{
  // make sure object state corresponds to the fact the ruins are already spawned
  _exploded = true;

  for (int i=0; i<_lights.Size(); i++)
  {
    LightEffect &info = _lights[i];
    info._timeToLive = 0;
  }
  for (int i=0; i<_particleSources.Size(); i++)
  {
    ParticlesEffect &info = _particleSources[i];
    info._timeToLive = 0;
  }
  for (int i=0; i<_sounds.Size(); i++)
  {
    SoundEffect &info = _sounds[i];
    info._timeToLive = 0;
  }
  _damageAround.Clear();
  for (int i=0; i<_destructPos.Size(); i++)
  {
    DestructPosEffect &hide = _destructPos[i];
    hide._delay = 0;
    hide._time = 0;
  }


  bool finished = true;
  // spawn ruins synchronously - we have no background processing here
  SpawnRuins(finished,false);
  if (finished)
  {
    // if there were no results, we are safe to destroy
    SetDelete();
  }
}

void DestructionEffects::UndoResults()
{
  for (int i=0; i<_ruins.Size(); i++)
  {
    // if there are any ruins, we can never be finished, because we may be forced to destroy the ruins
    RuinsEffect &ruin = _ruins[i];
    if (ruin._ruin)
    {
      // force the ruin to delete itself
      ruin._ruin->SetDelete();
      ruin._ruin = NULL;
    }
  }
}

void DestructionEffects::SimulateExplosion()
{
  if (!_exploded && _explosionTime <= Glob.time)
  {
    // activate all effects
    for (int i=0; i<_lights.Size(); i++)
      GScene->AddLight(_lights[i]._light);

    _exploded = true;

    Ref<EntityType> vType = VehicleTypes.New("FuelExplosion");
    AmmoType *aType = dynamic_cast<AmmoType *>(vType.GetRef());
    if (!aType)
    {
      Fail("No explosion type");
      return;
    }
    // WIP:EXTEVARS:FIXME fill hitInfo properly (normal, dir)
    HitInfo hitInfo(aType, FutureVisualState().Position(), VZero, VUp);
    if (_object)
    {
      float gauss = GRandGen.RandomValue() + GRandGen.RandomValue();
      float randomFactor = gauss * (_maxExplosionFactor - _minExplosionFactor) + _minExplosionFactor;

      float hit = _object->GetExplosives() * randomFactor;
      float indirectRatio = hitInfo._indirectHit / hitInfo._hit;
      hitInfo._hit = hit;
      hitInfo._indirectHit = indirectRatio * hit;
    }
    if (hitInfo._hit > 50)
    {
      Assert(_owner);
      if (IsLocal())
        // explosion has always a full energy
        GLandscape->ExplosionDamage(_owner, VZero, _object, VZero, hitInfo);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

LSError DestructEffect::Serialize(ParamArchive &ar)
{ 
  CHECK(ar.Serialize("doUnderWater", _doUnderWater, 1));
  return LSOK; 
}

void LightEffect::Load(const DestructionEffectType &effType, EffectVariablesPar evars, LODShape *shape, const Vector3 *defPos, float lifeTime)
{
  base::Load();

  _light = new LightPoint();
  _light->Load(Pars >> "CfgLights" >> effType._type, evars);
  _pos = ::GetPosition(shape, effType._position, defPos);
  _timeToLive = lifeTime * effType._lifeTime;
}

LSError LightEffect::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);

  CHECK(ar.Serialize("light", _light, 1))
  CHECK(::Serialize(ar, "effectPos", _pos, 1))
  CHECK(ar.Serialize("effectTTL", _timeToLive, 1))

  return LSOK;
}

void ParticlesEffect::Load(const DestructionEffectType &effType, EffectVariablesPar evars, Object *obj, const Vector3 *defPos, float lifeTime)
{
  base::Load();

  _source.Load(effType._type, evars);
  _source.AttachObject(obj, ::GetPosition(obj->GetShape(), effType._position, defPos));
  _timeToLive = lifeTime * effType._lifeTime;
}

LSError ParticlesEffect::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);

  CHECK(_source.Serialize(ar))
  CHECK(ar.Serialize("effectTTL", _timeToLive, 1))
  return LSOK;
}

void SoundEffect::Load(const DestructionEffectType &effType, float lifeTime)
{
  base::Load();

  _sound = new DynSoundObject(effType._type);
  _timeToLive = lifeTime * effType._lifeTime;
}

LSError SoundEffect::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);

  if (ar.IsSaving())
  {
    RString name;
    if (_sound) name = _sound->GetName();
    CHECK(ar.Serialize("sound", name, 1))
  }
  else if (ar.GetPass() == ParamArchive::PassFirst)
  {
    RString name;
    CHECK(ar.Serialize("sound", name, 1))
    if (name.GetLength() > 0) _sound = new DynSoundObject(name);
  }
  CHECK(ar.Serialize("effectTTL", _timeToLive, 1))
  return LSOK;
}

LSError DestructPosEffect::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);

  CHECK(ar.Serialize("delay", _delay, 1))
  CHECK(ar.Serialize("duration", _duration, 1))
  CHECK(ar.Serialize("time", _time, 1))
  return LSOK;
}

void DestructPosEffect::Load(ParamEntryPar cls, EffectVariablesPar evars)
{
  base::Load();

  _delay = evars.GetValue(cls>>"timeBeforeHiding");
  _duration = evars.GetValue(cls>>"hideDuration");
  _time = _duration;
}

LSError RuinsEffect::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);

  CHECK(ar.Serialize("shape", _shape, 1))
  return LSOK;
}

// FIXME: evars are not used here, remove that parameter?
void RuinsEffect::Load(RStringB shapeName, EffectVariablesPar evars)
{
  base::Load();

  _shape = GetShapeName(shapeName);
}

LSError DamageAroundEffect::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);

  CHECK(ar.Serialize("radiusRatio", radiusRatio, 1))
  CHECK(ar.Serialize("indirectHit", indirectHit, 1))
  return LSOK;
}

void DamageAroundEffect::Load(ParamEntryPar cls, EffectVariablesPar evars)
{
  base::Load();

  radiusRatio = evars.GetValue(cls>>"radiusRatio");
  indirectHit = evars.GetValue(cls>>"indirectHit");
}

//////////////////////////////////////////////////////////////////////////

LSError DestructionEffects::Serialize(ParamArchive &ar)
{
  // Entity serialization
  CHECK(base::Serialize(ar))

  // IExplosion serialization
  CHECK(ar.Serialize("minExplosionFactor", _minExplosionFactor, 1))
  CHECK(ar.Serialize("maxExplosionFactor", _maxExplosionFactor, 1))
  CHECK(::Serialize(ar, "explosionTime", _explosionTime, 1))
  CHECK(ar.Serialize("exploded", _exploded, 1))

  CHECK(ar.SerializeRef("object", _object, 1))
  CHECK(ar.SerializeRef("owner", _owner, 1))
  CHECK(ar.Serialize("particleSources", _particleSources, 1))
  CHECK(ar.Serialize("sounds", _sounds, 1))
  CHECK(ar.Serialize("destructPos", _destructPos, 1))
/*
  AutoArray<LightEffect> _lights;
*/

  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassSecond)
  {
    // process explosion effects
    if (_exploded)
    {
      // activate all effects
      for (int i=0; i<_lights.Size(); i++)
      {
        GScene->AddLight(_lights[i]._light);
      }
    }
  }

  return LSOK;
}

#define DESTRUCTION_EFFECTS_MSG_LIST(XX) \
  XX(Create, CreateDestructionEffects) \
  XX(UpdateDamage, None)

DEFINE_NETWORK_OBJECT(DestructionEffects, base, DESTRUCTION_EFFECTS_MSG_LIST)

#define CREATE_DESTRUCTION_EFFECTS_MSG(MessageName, XX) \
  XX(MessageName, OLink(Object), object, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("Object effects are attached to"), TRANSF_REF) \
  XX(MessageName, OLinkO(Object), owner, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("Entity which caused the destruction"), TRANSF_REF) \
  XX(MessageName, float, intensity, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Smoke intensity"), TRANSF) \
  XX(MessageName, float, interval, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Smoke inverse density"), TRANSF) \
  XX(MessageName, float, fireIntensity, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Fire intensity"), TRANSF) \
  XX(MessageName, float, fireInterval, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Fire inverse density"), TRANSF) \
  XX(MessageName, float, lifeTime, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Time to live"), TRANSF) \

DECLARE_NET_INDICES_EX(CreateDestructionEffects, CreateVehicle, CREATE_DESTRUCTION_EFFECTS_MSG)
DEFINE_NET_INDICES_EX(CreateDestructionEffects, CreateVehicle, CREATE_DESTRUCTION_EFFECTS_MSG)

NetworkMessageFormat &DestructionEffects::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCCreate:
    base::CreateFormat(cls, format);
    CREATE_DESTRUCTION_EFFECTS_MSG(CreateDestructionEffects, MSG_FORMAT)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError DestructionEffects::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    if (ctx.IsSending())
    {
      TMCHECK(base::TransferMsg(ctx))

      PREPARE_TRANSFER(CreateDestructionEffects)

      TRANSF_REF(object)
      TRANSF_REF(owner)
      TRANSF(intensity)
      TRANSF(interval)
      TRANSF(fireIntensity)
      TRANSF(fireInterval)
      TRANSF(lifeTime)
    }
    break;
  default:
    TMCHECK(base::TransferMsg(ctx))
    break;
  }
  return TMOK;
}

DestructionEffects *DestructionEffects::CreateObject(NetworkMessageContext &ctx)
{
  Ref<base> veh = base::CreateObject(ctx);
  DestructionEffects *effects = dyn_cast<DestructionEffects>(veh.GetRef());
  if (!effects) return NULL;

  PREPARE_TRANSFER(CreateDestructionEffects)

  OLink(Object) object;
  OLinkO(EntityAI) owner;
  float intensity;
  float interval;
  float fireIntensity;
  float fireInterval;
  float lifeTime;

  if (TRANSF_REF_BASE(object, object) != TMOK) return NULL;
  if (TRANSF_REF_BASE(owner, owner) != TMOK) return NULL;
  if (TRANSF_BASE(intensity, intensity) != TMOK) return NULL;
  if (TRANSF_BASE(interval, interval) != TMOK) return NULL;
  if (TRANSF_BASE(fireIntensity, fireIntensity) != TMOK) return NULL;
  if (TRANSF_BASE(fireInterval, fireInterval) != TMOK) return NULL;
  if (TRANSF_BASE(lifeTime, lifeTime) != TMOK) return NULL;

  if (!object) return NULL;
  const EntityType *entityType = object->GetEntityType();
  if (!entityType || !entityType->_destructionEffectsValid) return NULL;

  // remote effects never doing any damage
  effects->Init(entityType->_destructionEffects, owner, object, intensity, interval, fireIntensity, fireInterval, lifeTime, NULL, false);
  object->SetSmoke(effects);
  object->ExplodeSmoke();
  return effects;
}

void LightEffectRT::Init(RString name, EVarSet evarSet)
{
  // Create new light, add it to scene and switch it off by default
  _light = new LightPoint();
  GScene->AddLight(_light);
  _light->Switch(false);

  ParamEntryVal cls = Pars >> "CfgLights" >> name;
  const RString *varNames = EffectVariables::GetNames(evarSet);
  int varCount = EffectVariables::GetCount(evarSet);
  COMPILE_EXPR(position, CheckVector);
  COMPILE_EXPR(diffuse, CheckColor);
  COMPILE_EXPR(ambient, CheckColor);
  COMPILE_EXPR(brightness, Check);
}

void LightEffectRT::Simulate(Entity *entity, float deltaT, SimulationImportance prec, const float *varValues, int varCount, bool lightOn)
{
  if (!entity) return;

  Vector3 pos;
  EVALUATE_EXPR(_position, pos);
  Color diffuse;
  EVALUATE_EXPR(_diffuse, diffuse);
  Color ambient;
  EVALUATE_EXPR(_ambient, ambient);
  float brightness;
  EVALUATE_EXPR(_brightness, brightness);

  const ObjectVisualState &vs = entity->RenderVisualState();
  
  _light->SetPosition(vs.PositionModelToWorld(pos));
  _light->SetDiffuse(diffuse);
  _light->SetAmbient(ambient);
  _light->SetBrightness(brightness);
  _light->Switch(lightOn);
}

void ParticlesEffectRT::Init(RString name, EVarSet evarSet)
{
  EffectVariables evars(evarSet, 0.0f);
  _source.Load(name, evars);

  ParamEntryVal cls = Pars >> "CfgCloudlets" >> name;
  const RString *varNames = EffectVariables::GetNames(evarSet);
  int varCount = EffectVariables::GetCount(evarSet);

  COMPILE_EXPR(interval, Check);
  COMPILE_EXPR(circleRadius, Check);
  COMPILE_EXPR(circleVelocity, CheckVector);

  COMPILE_EXPR(timerPeriod, Check);
  COMPILE_EXPR(lifeTime, Check);
  COMPILE_EXPR(position, CheckVector);
  COMPILE_EXPR(moveVelocity, CheckVector);
  COMPILE_EXPR(rotationVelocity, Check);
  COMPILE_EXPR(weight, Check);
  COMPILE_EXPR(volume, Check);
  COMPILE_EXPR(rubbing, Check);
  COMPILE_EXPR(angle, Check);
  COMPILE_EXPR(sizeCoef, Check);
  COMPILE_EXPR(colorCoef, CheckColor);
  COMPILE_EXPR(animationSpeedCoef, Check);
  COMPILE_EXPR(randomDirectionPeriod, Check);
  COMPILE_EXPR(randomDirectionIntensity, Check);

  COMPILE_EXPR(lifeTimeVar, Check);
  COMPILE_EXPR(positionVar, CheckVector);
  COMPILE_EXPR(moveVelocityVar, CheckVector);
  COMPILE_EXPR(rotationVelocityVar, Check);
  COMPILE_EXPR(colorVar, CheckColor);
  COMPILE_EXPR(randomDirectionPeriodVar, Check);
  COMPILE_EXPR(randomDirectionIntensityVar, Check);
}

void ParticlesEffectRT::Simulate(Entity *entity, float deltaT, SimulationImportance prec, const float *varValues, int varCount)
{
  if (!entity) return;

  float interval;
  EVALUATE_EXPR(_interval, interval);
  _source.SetInterval(interval);

  float circleRadius;
  Vector3 circleVelocity;
  EVALUATE_EXPR(_circleRadius, circleRadius);
  EVALUATE_EXPR(_circleVelocity, circleVelocity);
  _source.SetCircle(circleRadius, circleVelocity);

  CParticleType *type = _source.AccessType();
  Assert(type);
  EVALUATE_EXPR(_timerPeriod, type->_timerPeriod);
  EVALUATE_EXPR(_lifeTime, type->_lifeTime);
  EVALUATE_EXPR(_position, type->_position);
  EVALUATE_EXPR(_moveVelocity, type->_moveVelocity);
  EVALUATE_EXPR(_rotationVelocity, type->_rotationVelocity);
  EVALUATE_EXPR(_weight, type->_weight);
  EVALUATE_EXPR(_volume, type->_volume);
  EVALUATE_EXPR(_rubbing, type->_rubbing);
  EVALUATE_EXPR(_angle, type->_angle);
  EVALUATE_EXPR(_randomDirectionPeriod, type->_randomDirectionPeriod);
  EVALUATE_EXPR(_randomDirectionIntensity, type->_randomDirectionIntensity);
  EVALUATE_EXPR(_sizeCoef, type->_sizeCoef);
  EVALUATE_EXPR(_colorCoef, type->_colorCoef);
  EVALUATE_EXPR(_animationSpeedCoef, type->_animationSpeedCoef);

  CParticleRandomization &variance = _source.AccessVariance();
  EVALUATE_EXPR(_lifeTimeVar, variance._lifeTime);
  EVALUATE_EXPR(_positionVar, variance._position);
  EVALUATE_EXPR(_moveVelocityVar, variance._moveVelocity);
  EVALUATE_EXPR(_rotationVelocityVar, variance._rotationVelocity);
  EVALUATE_EXPR(_colorVar, variance._colorCoef);
  EVALUATE_EXPR(_randomDirectionPeriodVar, variance._randomDirectionPeriod);
  EVALUATE_EXPR(_randomDirectionIntensityVar, variance._randomDirectionIntensity);

  // simulate the effect now
  _source.Simulate(entity, deltaT, prec);
}

void SoundEffectRT::Init(ParamEntryPar cls)
{
  RStringB type = cls >> "type";
  _sound = new DynSoundObject(type);
}

void SoundEffectRT::Simulate(Entity *entity, float deltaT, SimulationImportance prec)
{
  _sound->Simulate(entity, deltaT, prec);
}

void EffectsSourceRT::Init(ParamEntryPar cls, Entity *entity, EVarSet evarSet)
{
  if (!entity) return;

  _entity = entity;
  _evarSet = evarSet;

  for (int i=0; i<cls.GetEntryCount(); i++)
  {
    ParamEntryVal entry = cls.GetEntry(i);
    if (!entry.IsClass()) continue;

    RStringB simulation = entry >> "simulation";
    RStringB type = entry >> "type";

    if (simulation == RSB(light))
    {
      int index = _lights.Add();
      LightEffectRT &info = _lights[index];
      info.Init(type, evarSet);
    }
    else if (simulation == RSB(particles))
    {
      int index = _particleSources.Add();
      ParticlesEffectRT &info = _particleSources[index];
      info.Init(type, evarSet);
      info._source.AttachObject(_entity, VZero);
    }
    else if (simulation == RSB(sound))
    {
      int index = _sounds.Add();
      SoundEffectRT &info = _sounds[index];
      info.Init(entry);
    }
  }
  _lights.Compact();
  _particleSources.Compact();
  _sounds.Compact();
}

void EffectsSourceRT::Simulate(float deltaT, SimulationImportance prec, const float *varValues, int varCount, bool lightOn)
{
  if (!_entity) return;

  Assert(varCount == EffectVariables::GetCount(_evarSet));

  // simulate all effects
  for (int i=0; i<_lights.Size(); i++)
  {
    LightEffectRT &info = _lights[i];
    info.Simulate(_entity, deltaT, prec, varValues, varCount, lightOn);
  }

  // Do simulate sound and particles only when light is on
  if (lightOn)
  {
    for (int i=0; i<_particleSources.Size(); i++)
    {
      ParticlesEffectRT &info = _particleSources[i];
      info.Simulate(_entity, deltaT, prec, varValues, varCount);
    }
    for (int i=0; i<_sounds.Size(); i++)
    {
      SoundEffectRT &info = _sounds[i];
      info.Simulate(_entity, deltaT, prec);
    }
  }
}
